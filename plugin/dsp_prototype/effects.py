"""Tape-baserade effekter — Echo, Sound-on-Sound, Multiplay, Synchroplay.

Echo är BC2000DL:s **mest karakteristiska effekt**: hastighetsknappen byter
automatiskt echo-tid till 70/140/280 ms vid 19/9.5/4.75 cm/s. Det är feature,
inte bug — pluginen ska göra detsamma.

S-on-S = stereomatrix (L → R-spår).
Multiplay = bounce-loop med generationsförluster (brus +1 dB/gen, HF-roll-off).
Synchroplay = monitor-tap byts från PLAY-head till REC-head (low-latency).

Referens: planfil §1D + §2 Block 7 + specs.md §9.
"""
from __future__ import annotations
import numpy as np
from scipy import signal as sig_filt

from eq_din1962 import TapeSpeed


# Echo-tid per hastighet (ms) — fast offset record→play-head ÷ tape-speed
_ECHO_TIMES_MS = {
    TapeSpeed.SPEED_19:   75.0,
    TapeSpeed.SPEED_9_5:  150.0,
    TapeSpeed.SPEED_4_75: 300.0,
}


class Echo:
    """Tape-echo med per-speed delay-tid + feedback (kan självoscillera).

    Manualen varnar: 'Drejes dette for meget op, således at optagelsen af
    ekkosignalet bliver kraftigere end det oprindelige signal, vil optagelsen
    blive ødelagt.' (s.7)

    Pluginen modellerar self-osc-tröskel @ ~85 % feedback.
    """

    def __init__(self,
                 sample_rate: int = 48_000,
                 speed: TapeSpeed = TapeSpeed.SPEED_19,
                 *,
                 amount: float = 0.0,    # 0–1 (knap 10a, echo level)
                 enabled: bool = False,   # knap 18 (echo button)
                 hf_loss_per_pass_db: float = -2.0):
        self.sample_rate = sample_rate
        self.amount = amount
        self.enabled = enabled
        self.hf_loss_per_pass_db = hf_loss_per_pass_db

        # Max delay = 350 ms (för 4.75 cm/s + lite headroom)
        self._max_delay_samples = int(sample_rate * 0.35)
        self._buf = np.zeros(self._max_delay_samples, dtype=np.float64)
        self._write_idx = 0

        # HF-roll-off per echo-pass (simulerar tape-band-genererings-förlust)
        nyq = sample_rate / 2
        # Mjukt LP @ 8 kHz — varje pass dämpar HF
        self._b_lp, self._a_lp = sig_filt.butter(1, 8000 / nyq, btype="lowpass")
        self._zi_lp = sig_filt.lfilter_zi(self._b_lp, self._a_lp) * 0.0

        self.set_speed(speed)

    def set_speed(self, speed: TapeSpeed):
        self._speed = speed
        delay_ms = _ECHO_TIMES_MS[speed]
        self._delay_samples = int(self.sample_rate * delay_ms / 1000)
        if self._delay_samples > self._max_delay_samples:
            self._delay_samples = self._max_delay_samples - 1

    @property
    def speed(self) -> TapeSpeed:
        return self._speed

    @property
    def delay_ms(self) -> float:
        return self._delay_samples * 1000.0 / self.sample_rate

    def reset(self):
        self._buf.fill(0)
        self._write_idx = 0
        self._zi_lp = sig_filt.lfilter_zi(self._b_lp, self._a_lp) * 0.0

    def process(self, x: np.ndarray) -> np.ndarray:
        x = np.asarray(x, dtype=np.float64)
        n = len(x)

        if not self.enabled or self.amount < 1e-6:
            return x

        out = np.empty(n, dtype=np.float64)
        # Feedback gain: amount mappas non-linjärt så self-osc inträffar runt 0.85
        # Vid amount=1.0: feedback = 1.02 → divergerar (självoscillerar)
        # Manualen varnar om detta beteende — det är feature, inte bug
        feedback = self.amount * 1.05  # Vid amount > ~0.95 → self-osc
        feedback = min(feedback, 1.05)  # Cap för numerisk säkerhet

        for i in range(n):
            # Läs gammal sample från delay-line (read-pointer)
            read_idx = (self._write_idx - self._delay_samples) % self._max_delay_samples
            delayed = self._buf[read_idx]

            # Mixa delayed med input
            out[i] = x[i] + delayed * feedback

            # Skriv (input + delayed*feedback) till bufferten — denna går runt
            # och blir nästa eko (med HF-loss vid varje pass)
            self._buf[self._write_idx] = x[i] + delayed * feedback
            self._write_idx = (self._write_idx + 1) % self._max_delay_samples

        # Applicera HF-loss på hela bufferten genom LP-filter (förenklat:
        # filter outsignalen så att ekot tappar HF; i verkligheten skulle
        # det göras *inne* i feedback-loopen men det är dyrt — gör det
        # som approximation post-loop)
        out, self._zi_lp = sig_filt.lfilter(self._b_lp, self._a_lp, out, zi=self._zi_lp)
        return out


class SoundOnSound:
    """S-on-S = stereomatrix (PLAY L → REC R).

    Pluginens implementation: kanal-korsmix där vänsterkanal från playback
    routas till högerkanals record-input. Användarens 5:e fader (knap 10a-mässig)
    styr mängden.
    """

    def __init__(self, *, amount: float = 0.0, enabled: bool = False):
        self.amount = amount
        self.enabled = enabled

    def process_stereo(self,
                       l_play: np.ndarray, r_play: np.ndarray,
                       l_rec_in: np.ndarray, r_rec_in: np.ndarray
                       ) -> tuple[np.ndarray, np.ndarray]:
        """Mata L-playback in i R-record-input (klassisk B&O S-on-S).

        Returnerar (l_rec_out, r_rec_out) — record-input för båda kanaler
        EFTER S-on-S-mix.
        """
        if not self.enabled or self.amount < 1e-6:
            return l_rec_in, r_rec_in
        # Mixa L-playback i R-record-input
        r_out = r_rec_in + l_play * self.amount
        return l_rec_in, r_out


class Multiplay:
    """Multiplay = bounce-loop med generationsförluster.

    Modellerar manualens varning: 'Grundstøjen i enhver optagelse adderes
    også og vil på et tidspunkt blive hørbar.' Varje generation lägger på:
    - +1 dB tape-noise
    - HF-roll-off från ~10 kHz (gen 1) → 6 kHz (gen 3) vid 19 cm/s
    - Subtle pitch-modulationsdrift
    """

    def __init__(self,
                 sample_rate: int = 48_000,
                 *,
                 generation: int = 1,    # 1 = original, 2 = bounce 1, ...
                 enabled: bool = False):
        self.sample_rate = sample_rate
        self.generation = max(1, generation)
        self.enabled = enabled

        nyq = sample_rate / 2
        # HF-roll-off per generation
        # Gen 1 = 10 kHz, Gen 2 = 8 kHz, Gen 3 = 6 kHz, Gen 4+ = 5 kHz
        hf_corners = [10_000, 8_000, 6_000, 5_000, 4_000]
        idx = min(self.generation - 1, len(hf_corners) - 1)
        self._b_lp, self._a_lp = sig_filt.butter(2, hf_corners[idx] / nyq, btype="lowpass")
        self._zi_lp = sig_filt.lfilter_zi(self._b_lp, self._a_lp) * 0.0

        # Brus per generation
        self._noise_amp = 10 ** ((self.generation - 1) * 1.0 / 20)  # +1 dB/gen
        self._rng = np.random.default_rng(seed=self.generation * 7919)

    def process(self, x: np.ndarray) -> np.ndarray:
        if not self.enabled:
            return x
        x = np.asarray(x, dtype=np.float64)
        # 1. HF-roll-off per generation
        y, self._zi_lp = sig_filt.lfilter(self._b_lp, self._a_lp, x, zi=self._zi_lp)
        # 2. Adderat brus (proportionellt mot generation)
        noise_floor = self._noise_amp * 1e-3  # ~-60 dBFS bas + per-gen
        y = y + self._rng.normal(0, noise_floor, len(y))
        return y


class Synchroplay:
    """Synchroplay = monitor-tap byts från PLAY-head till REC-head.

    Effekten i plugin: när aktiverad används input direkt (ingen tape-delay
    i monitor-vägen) — som "low-latency monitor"-knapp.

    Implementeras som bool-flagga som SignalChain använder för monitor-routing.
    """

    def __init__(self, *, enabled: bool = False):
        self.enabled = enabled
