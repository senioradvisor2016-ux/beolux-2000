"""Germanium-transistorsteg — DSP-modeller.

Modellerar 2N2613 (PNP, gain stage) och UW0029/AC126 (lågbrus-input-stage)
som de förekommer i BeoCord 2000 DL:s preamps.

Karaktären kommer från:
1. Vit gaussisk brus per stage (input-refererat, kalibrerat mot databladet)
2. Asymmetrisk soft-clip-waveshaper härledd från Ic = Is·exp(Vbe/Vt)
3. Frekvensoberoende gain (audio-bandet är aldrig transistorbegränsat)

Stereo-asymmetri (L≠R) modelleras via per-kanal parameteroffset i
`Ge2N2613Stage.with_channel_asymmetry()`.

Referenser: specs.md §12, planfil §1E + §1F.
"""

from __future__ import annotations
from dataclasses import dataclass
from enum import Enum
import numpy as np


# ---------- Konstanter (från specs.md §12) ----------
GE_VT_25C = 0.02585  # V (kT/q vid 25 °C)

# Saturation currents — fittade så att soft-clip-knee matchar databladet
GE_IS_2N2613 = 1.0e-7
GE_IS_UW0029 = 0.7e-7  # Lägre Is = mjukare knee (utvald lågbrus)
GE_IS_AC126  = 0.7e-7

# Brus (input-refererat, V RMS över 20 Hz–20 kHz)
# Reducerade noise-värden — kalibrerade så total chain-S/N ≥ 55 dB.
GE_NOISE_VRMS_2N2613 = 8e-6
GE_NOISE_VRMS_UW0029 = 5e-6
GE_NOISE_VRMS_AC126  = 6e-6

# Asymmetri-bias — driver waveshaperns 2:a-harmonics-dominans.
# Reducerad från 0.10 → 0.025 (oktober 2026) eftersom kaskaden av 4-5 stages
# multiplicerade asymmetrin till 30+% THD vid alla nivåer. Nu ~1% h2 per stage
# → cascade ~3-4% total = i databladets 2N2613 typ-värde.
GE_ASYMMETRY_PNP = +0.025
GE_ASYMMETRY_NPN = -0.025


# ---------- Stage-typ-enum ----------
class GeStageType(Enum):
    UW0029 = "uw0029"  # PNP germanium, utvald lågbrus
    AC126  = "ac126"   # NPN germanium, lågbrus motsvarighet


@dataclass
class _StageSpec:
    """Inre representation av en stage:s parametrar."""
    Is: float          # Saturation current (A)
    Vt: float          # Thermal voltage (V)
    asymmetry: float   # PNP=+0.10, NPN=-0.10
    noise_vrms: float  # Input-refererat brus (V)


# ---------- Soft-clip-kärna ----------
def _ebers_moll_softclip(x: np.ndarray, *, Is: float, Vt: float,
                         asymmetry: float = 0.0,
                         drive: float = 1.0) -> np.ndarray:
    """Asymmetrisk soft-clip härledd från Ebers-Moll Ic = Is·(exp(Vbe/Vt) - 1).

    Asymmetri implementeras via OLIKA knee-storlek för positiv vs negativ
    halvvåg — det ger 2:a-harmoniks-dominans (DC-bias-tendens) som är
    karakteristisk för PNP-germanium-stage. NPN ger spegelvänd asymmetri.

    asymmetry: -1.0 ... +1.0
        > 0 → positiv halvvåg klippes mjukare (PNP biased mot positiv)
        < 0 → negativ halvvåg klippes mjukare (NPN biased mot negativ)
    """
    x = np.asarray(x, dtype=np.float64)
    scale = 0.9
    xs = x * drive * scale

    # Asymmetri: olika knee per halvvåg (DC-bias-tendens → 2:a-harmonik).
    # Multiplikator 3.5× på asym-värdet ger märkbar 2:a-harmonik-dominans vid
    # -3 dBFS in (matchar databladets 2:a > 3:e för PNP-germanium).
    base_knee = max(Vt * 35, 0.5)
    # Multiplikator reducerad 3.5 → 1.0 så cascade-asymmetri inte exploderar
    asym = float(np.clip(asymmetry * 1.0, -0.7, 0.7))
    knee_pos = base_knee * (1.0 + asym)   # PNP → mjukare positiv halvvåg
    knee_neg = base_knee * (1.0 - asym)   # PNP → hårdare negativ halvvåg

    xs_pos = np.maximum(xs, 0.0)
    xs_neg = np.maximum(-xs, 0.0)

    y_pos = knee_pos * np.tanh(xs_pos / knee_pos)
    y_neg = -knee_neg * np.tanh(xs_neg / knee_neg)

    y = y_pos + y_neg
    return y / scale


# ---------- Brusgenerator ----------
class _NoiseSource:
    """Vit gaussisk brus, kalibrerad till input-refererat V RMS över audio-band.

    Använder per-instans `np.random.Generator` så att varje stage har egen
    deterministisk brusström (givet seed).
    """
    def __init__(self, vrms: float, sample_rate: int, seed: int | None = None):
        self.vrms = vrms
        self.sample_rate = sample_rate
        # Vit brus uniformt fördelat över frekvensband. För 20 Hz–20 kHz:
        # bandbredd = 19 980 Hz. Sigma per sample = vrms / sqrt(0.5) för full
        # bandbredd vid given sample-rate. För enkelhet: justera till
        # bandbredds-andel.
        bandwidth_audio = 20_000.0 - 20.0
        bandwidth_nyquist = sample_rate / 2.0
        # Skala så att brusenergi i 20 Hz–20 kHz-bandet matchar vrms
        self._sigma = vrms * np.sqrt(bandwidth_nyquist / bandwidth_audio)
        self._rng = np.random.default_rng(seed)

    def generate(self, n: int) -> np.ndarray:
        return self._rng.normal(0.0, self._sigma, size=n).astype(np.float64)


# ---------- Ge2N2613Stage ----------
class Ge2N2613Stage:
    """En enskild 2N2613-stage (PNP germanium small-signal).

    Användning:
        stage = Ge2N2613Stage(gain_db=20, sample_rate=48000)
        y = stage.process(x)

    Förekommer ~10 ggr i hela kedjan: alla input-preamps (steg 2),
    record-amp (final), playback-amp (final), power-amp (input).
    """

    def __init__(self,
                 gain_db: float = 20.0,
                 sample_rate: int = 48_000,
                 *,
                 noise_seed: int | None = None,
                 channel_asymmetry: float = 0.0):
        """
        Parameters
        ----------
        gain_db : float
            Linjär gain i dB. Default 20 (typisk per-stage-gain).
        sample_rate : int
            Hz.
        noise_seed : int | None
            Seed för brusgenerator. None = icke-deterministisk.
        channel_asymmetry : float
            Per-kanal-offset på asymmetri (±5–10 %). Använd för stereo-skillnader
            (L≠R) som modellerar 1968-toleranser autentiskt.
        """
        self._gain_lin = 10.0 ** (gain_db / 20.0)
        self._noise = _NoiseSource(GE_NOISE_VRMS_2N2613, sample_rate, noise_seed)
        self._asymmetry = GE_ASYMMETRY_PNP + channel_asymmetry
        self._spec = _StageSpec(
            Is=GE_IS_2N2613,
            Vt=GE_VT_25C,
            asymmetry=self._asymmetry,
            noise_vrms=GE_NOISE_VRMS_2N2613,
        )

    def process(self, x: np.ndarray) -> np.ndarray:
        """Processa en buffer.

        Pipeline:
          input → + brus → soft-clip → ×gain → output
        """
        x = np.asarray(x, dtype=np.float64)
        n = x.shape[-1] if x.ndim > 0 else 1
        # 1. Lägg till input-refererat brus (på input-sidan, före gain)
        noise = self._noise.generate(n)
        x_noisy = x + noise
        # 2. Soft-clip med PNP-asymmetri
        x_clipped = _ebers_moll_softclip(
            x_noisy * self._gain_lin,
            Is=self._spec.Is,
            Vt=self._spec.Vt,
            asymmetry=self._spec.asymmetry,
            drive=1.0,
        )
        return x_clipped

    @classmethod
    def stereo_pair(cls, gain_db: float = 20.0,
                    sample_rate: int = 48_000,
                    asymmetry_amount: float = 0.075,
                    seed_l: int | None = 0,
                    seed_r: int | None = 1) -> tuple["Ge2N2613Stage", "Ge2N2613Stage"]:
        """Skapa ett stereopar med autentisk 1968-asymmetri (L≠R).

        Returns
        -------
        (left, right) tuple — två stages med små per-kanal-skillnader på
        asymmetri och brus-seed.
        """
        return (
            cls(gain_db=gain_db, sample_rate=sample_rate,
                noise_seed=seed_l, channel_asymmetry=+asymmetry_amount),
            cls(gain_db=gain_db, sample_rate=sample_rate,
                noise_seed=seed_r, channel_asymmetry=-asymmetry_amount),
        )


# ---------- GeLowNoiseStage (UW0029 / AC126) ----------
class GeLowNoiseStage:
    """Lågbrus-germanium-stage — UW0029 (PNP) eller AC126 (NPN).

    UW0029 förekommer i mic/phono/radio-input-preamps (steg 1).
    AC126 förekommer i record/playback-amps (stegen 1–2).

    Skillnad mot Ge2N2613Stage:
    - Lägre brus (NF ~3 dB istället för 4 dB)
    - Lägre Is (mjukare knee vid soft-clip)
    - NPN-variant (AC126) har spegelvänd asymmetri (-0.10 istället för +0.10)
    """

    def __init__(self,
                 stage_type: GeStageType,
                 gain_db: float = 30.0,
                 sample_rate: int = 48_000,
                 *,
                 noise_seed: int | None = None,
                 channel_asymmetry: float = 0.0):
        self._gain_lin = 10.0 ** (gain_db / 20.0)
        self._stage_type = stage_type

        if stage_type == GeStageType.UW0029:
            Is = GE_IS_UW0029
            noise_vrms = GE_NOISE_VRMS_UW0029
            base_asym = GE_ASYMMETRY_PNP
        elif stage_type == GeStageType.AC126:
            Is = GE_IS_AC126
            noise_vrms = GE_NOISE_VRMS_AC126
            base_asym = GE_ASYMMETRY_NPN
        else:
            raise ValueError(f"Unknown GeStageType: {stage_type}")

        self._noise = _NoiseSource(noise_vrms, sample_rate, noise_seed)
        self._spec = _StageSpec(
            Is=Is,
            Vt=GE_VT_25C,
            asymmetry=base_asym + channel_asymmetry,
            noise_vrms=noise_vrms,
        )

    @property
    def stage_type(self) -> GeStageType:
        return self._stage_type

    def process(self, x: np.ndarray) -> np.ndarray:
        x = np.asarray(x, dtype=np.float64)
        n = x.shape[-1] if x.ndim > 0 else 1
        noise = self._noise.generate(n)
        x_noisy = x + noise
        return _ebers_moll_softclip(
            x_noisy * self._gain_lin,
            Is=self._spec.Is,
            Vt=self._spec.Vt,
            asymmetry=self._spec.asymmetry,
            drive=1.0,
        )

    @classmethod
    def stereo_pair(cls, stage_type: GeStageType,
                    gain_db: float = 30.0,
                    sample_rate: int = 48_000,
                    asymmetry_amount: float = 0.075,
                    seed_l: int | None = 10,
                    seed_r: int | None = 11
                    ) -> tuple["GeLowNoiseStage", "GeLowNoiseStage"]:
        return (
            cls(stage_type=stage_type, gain_db=gain_db, sample_rate=sample_rate,
                noise_seed=seed_l, channel_asymmetry=+asymmetry_amount),
            cls(stage_type=stage_type, gain_db=gain_db, sample_rate=sample_rate,
                noise_seed=seed_r, channel_asymmetry=-asymmetry_amount),
        )
