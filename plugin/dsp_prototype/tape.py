"""TapeSaturation — kärnblocket i pluginen.

Jiles-Atherton-approximation av magnetisk hysteres + 100 kHz bias-injektion.
Bias-signalen modulerar audio genom B-H-kurvan så att audio-bandet effektivt
återstår linjärt vid normal nivå men mjukt mättas vid hög nivå.

Hastighetsberoende:
- HF-bandbredd (19 cm/s = 20 kHz, 9.5 cm/s = 12 kHz, 4.75 cm/s = 6 kHz)
- Egen brus-nivå (lägre hastighet = mer brus pga lägre signal-amplitud per längdmm)
- Modulationsdjup (lägre hastighet = mer modulationsbrus)

Referens: planfil §1I + specs.md §2 §3 §6.
"""
from __future__ import annotations
from dataclasses import dataclass
import numpy as np
from scipy import signal

from eq_din1962 import TapeSpeed
from jiles_atherton import JilesAtherton, TAPE_PRESETS


@dataclass
class _TapeSpeedSpec:
    """Per-hastighet tape-egenskaper."""
    hf_corner_hz: float
    noise_db: float           # Tape-egenbrus (dBFS)
    modulation_depth: float    # 0–0.05 typiskt
    head_bump_freq: float      # Resonansbump från head-induktans
    head_bump_q: float
    head_bump_gain_db: float


_SPEED_SPECS = {
    # Brus -82/-76/-70 dBFS — kalibrerat så total chain S/N ≥ 55 dB (spec).
    TapeSpeed.SPEED_19:   _TapeSpeedSpec(20_000, -82, 0.005, 70, 1.5, 0.5),
    TapeSpeed.SPEED_9_5:  _TapeSpeedSpec(12_000, -76, 0.012, 80, 1.5, 0.7),
    TapeSpeed.SPEED_4_75: _TapeSpeedSpec(6_000,  -70, 0.025, 90, 1.5, 1.0),
}


def _design_peak(freq_hz: float, q: float, gain_db: float,
                 sample_rate: int) -> tuple[np.ndarray, np.ndarray]:
    """RBJ peaking EQ — för head-bump-resonans."""
    if abs(gain_db) < 0.01:
        return np.array([1.0, 0.0, 0.0]), np.array([1.0, 0.0, 0.0])
    A = 10 ** (gain_db / 40)
    w0 = 2 * np.pi * freq_hz / sample_rate
    cos_w0 = np.cos(w0)
    sin_w0 = np.sin(w0)
    alpha = sin_w0 / (2 * q)

    b0 = 1 + alpha * A
    b1 = -2 * cos_w0
    b2 = 1 - alpha * A
    a0 = 1 + alpha / A
    a1 = -2 * cos_w0
    a2 = 1 - alpha / A
    return (np.array([b0/a0, b1/a0, b2/a0]),
            np.array([1.0, a1/a0, a2/a0]))


class TapeSaturation:
    """Tape-saturation-block: hysteres + bias + hastighetsspecifika filter.

    Pipeline:
      audio + bias → [B-H-hysteres] → [HF-roll-off per hastighet] → [head-bump] → +tape-noise → output

    Bias är 100 kHz dither-likt signal som linjäriserar B-H-kurvan i audio-bandet.
    """

    def __init__(self,
                 sample_rate: int = 48_000,
                 speed: TapeSpeed = TapeSpeed.SPEED_19,
                 *,
                 bias_amount: float = 1.0,         # 1.0 = nominell (2.3 mA)
                 saturation_drive: float = 1.0,    # Hur snabbt vi når B-H-knee
                 print_through: float = 0.0,       # 0–0.05
                 noise_seed: int | None = None,
                 oversample: int = 8,              # Internt oversampling för
                                                   # bias-injection (100 kHz)
                                                   # + nonlinjär anti-alias
                 tape_formula: str = "Agfa"):       # "Agfa" / "BASF" / "Scotch"
        self.sample_rate = sample_rate
        self.bias_amount = bias_amount
        self.saturation_drive = saturation_drive
        self.print_through = print_through
        self.oversample = max(1, int(oversample))
        self._rng = np.random.default_rng(noise_seed)

        # Jiles-Atherton hysteres-modell — riktig stateful magnetisering
        # (ersätter tidigare memoryless tanh-approximation)
        ja_params = TAPE_PRESETS.get(tape_formula, TAPE_PRESETS["Agfa"])
        self._hysteresis_model = JilesAtherton(**ja_params)

        # Bias-oscillator: 100 kHz sinus injicerad i hysteres-blocket vid
        # oversample-rate (kräver SR_internal ≥ 250 kHz för Nyquist).
        self._bias_phase = 0.0
        self._bias_freq_hz = 100_000.0

        # State för print-through (en cirkulär buffer som speglar tape-spol)
        # Vid 18 cm-spol och 19 cm/s = ~1 varv per ~6 s → använd en stor delay
        self._print_delay = int(sample_rate * 1.5)  # 1.5 s
        self._print_buffer = np.zeros(self._print_delay, dtype=np.float64)
        self._print_idx = 0

        self._b_hf = None; self._a_hf = None; self._zi_hf = None
        self._b_bump = None; self._a_bump = None; self._zi_bump = None

        self.set_speed(speed)

    def set_speed(self, speed: TapeSpeed):
        spec = _SPEED_SPECS[speed]
        self._spec = spec
        self._speed = speed

        # HF-roll-off — 2:a ordningens Butterworth
        nyq = self.sample_rate / 2
        hf_norm = min(spec.hf_corner_hz / nyq, 0.95)
        self._b_hf, self._a_hf = signal.butter(2, hf_norm, btype="lowpass")
        self._zi_hf = signal.lfilter_zi(self._b_hf, self._a_hf) * 0.0

        # Head-bump (peaking EQ vid LF)
        self._b_bump, self._a_bump = _design_peak(
            spec.head_bump_freq, spec.head_bump_q, spec.head_bump_gain_db,
            self.sample_rate)
        self._zi_bump = signal.lfilter_zi(self._b_bump, self._a_bump) * 0.0

        # Brus-skalning per hastighet
        # noise_db = -55 → linjär 10^(-55/20) = 0.00178
        self._noise_amp = 10 ** (spec.noise_db / 20)

        # Modulationsbrus-frekvens (1–10 Hz för wow-band, här breddat)
        self._mod_depth = spec.modulation_depth

    @property
    def speed(self) -> TapeSpeed:
        return self._speed

    def reset(self):
        self._zi_hf = signal.lfilter_zi(self._b_hf, self._a_hf) * 0.0
        self._zi_bump = signal.lfilter_zi(self._b_bump, self._a_bump) * 0.0
        self._print_buffer.fill(0)
        self._print_idx = 0

    def _hysteresis_oversampled(self, x: np.ndarray) -> np.ndarray:
        """Oversamplad J-A-hysteres med 100 kHz bias som riktig signal.

        Pipeline:
          1. Upsample x med factor=oversample (FIR-interpolation)
          2. Generera 100 kHz bias-sinus vid oversample-rate
          3. Summera: H_total = audio + bias_amplitude × sin(2π·100k·t)
          4. Köra J-A sample-för-sample (history-dependent)
          5. LP-filter @ 25 kHz (tar bort bias + alias-energi)
          6. Downsample tillbaka till SR
        """
        K = self.oversample
        if K == 1:
            return self._hysteresis_model.process(x * self.saturation_drive)

        SR_int = self.sample_rate * K
        # 1. Upsample (zero-insertion + FIR-interp)
        n = len(x)
        x_up = signal.resample_poly(x, K, 1, padtype="constant")

        # 2. Bias-sinus vid oversample-rate. Real BC2000DL bias är ~2.3 mA i
        # record-headet — i normaliserad audio-domän motsvarar det ~3 % av
        # peak-signalnivå. Tidigare 40 % drev J-A in i full-saturation och
        # läckte till audio-bandet via intermod.
        n_up = len(x_up)
        t = np.arange(n_up) / SR_int + self._bias_phase / self._bias_freq_hz
        bias_amplitude = 0.03 * self.bias_amount
        bias_sig = bias_amplitude * np.sin(2 * np.pi * self._bias_freq_hz * t)

        # Uppdatera bias-fas för nästa block (kontinuitet)
        self._bias_phase = (self._bias_phase + n_up / K * self._bias_freq_hz / self.sample_rate) % 1.0

        # 3. Summera audio + bias
        H = x_up * self.saturation_drive + bias_sig

        # 4. Kör J-A sample-för-sample vid SR_int
        y_up = self._hysteresis_model.process(H)

        # 5. LP @ 25 kHz för att ta bort bias-residue + alias
        nyq_int = SR_int / 2
        hf_norm = min(25_000.0 / nyq_int, 0.95)
        b_lp, a_lp = signal.butter(6, hf_norm, btype="lowpass")
        y_filt = signal.filtfilt(b_lp, a_lp, y_up)

        # 6. Downsample
        y = signal.resample_poly(y_filt, 1, K, padtype="constant")
        # Trimma längd om resample lade till samples
        if len(y) > n:
            y = y[:n]
        elif len(y) < n:
            y = np.pad(y, (0, n - len(y)))
        return y

    def _add_modulation_noise(self, x: np.ndarray) -> np.ndarray:
        """Additivt modulationsbrus — proportionellt mot signalnivå."""
        if self._mod_depth < 1e-6:
            return x
        n = len(x)
        # Lågfrekvent modulation (typ wow-band men breddat)
        mod_lf = self._rng.normal(0, 1, n) * self._mod_depth
        # Lågpass-filter modulationen så den blir korrelerad
        b, a = signal.butter(2, 200 / (self.sample_rate / 2), btype="lowpass")
        mod_filtered = signal.lfilter(b, a, mod_lf)
        return x * (1.0 + mod_filtered)

    def _add_print_through(self, x: np.ndarray) -> np.ndarray:
        """Print-through: dämpat eko från tidigare lager på spolen."""
        if self.print_through < 1e-6:
            return x
        n = len(x)
        out = np.empty_like(x)
        for i in range(n):
            # Läs gammalt sample och blanda
            old = self._print_buffer[self._print_idx]
            out[i] = x[i] + old * self.print_through
            # Skriv nuvarande sample till bufferten
            self._print_buffer[self._print_idx] = x[i]
            self._print_idx = (self._print_idx + 1) % self._print_delay
        return out

    def process(self, x: np.ndarray) -> np.ndarray:
        """Kärn-pipeline."""
        x = np.asarray(x, dtype=np.float64)
        n = len(x)

        # 1. J-A hysteres med oversamplad bias-injection (riktig 100 kHz sine)
        y = self._hysteresis_oversampled(x)

        # 2. Modulationsbrus (signal-proportionellt)
        y = self._add_modulation_noise(y)

        # 3. Print-through (dämpat eko)
        if self.print_through > 1e-6:
            y = self._add_print_through(y)

        # 4. HF-roll-off (per hastighet)
        y, self._zi_hf = signal.lfilter(self._b_hf, self._a_hf, y, zi=self._zi_hf)

        # 5. Head-bump (LF-resonans från head-induktans)
        y, self._zi_bump = signal.lfilter(self._b_bump, self._a_bump, y, zi=self._zi_bump)

        # 6. Tape-egenbrus (additiv)
        noise = self._rng.normal(0, self._noise_amp, n)
        y = y + noise

        return y
