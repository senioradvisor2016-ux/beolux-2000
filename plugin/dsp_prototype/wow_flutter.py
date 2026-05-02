"""WowFlutter — hastighetsbaserad pitch-modulation.

Wow (0.5–6 Hz) + flutter (6–100 Hz) modulerar tape-läsning genom delay-line
med Lagrange-interpolation. Mer wow/flutter vid lägre hastighet (mindre
mekanisk stabilitet per längdmm).

Referens: planfil §2 Block 4 + specs.md §7.
"""
from __future__ import annotations
from dataclasses import dataclass
import numpy as np
from scipy import signal

from eq_din1962 import TapeSpeed


@dataclass
class _WFSpec:
    wow_amount: float    # 0.05 % = 0.0005
    flutter_amount: float
    wow_freq_hz: float
    flutter_freq_hz: float


# Mått från specs.md §7
_WF_SPECS = {
    TapeSpeed.SPEED_19:   _WFSpec(0.0008, 0.0006, 1.5, 30),
    TapeSpeed.SPEED_9_5:  _WFSpec(0.0013, 0.0010, 1.5, 30),
    TapeSpeed.SPEED_4_75: _WFSpec(0.0020, 0.0016, 1.5, 30),
}


class WowFlutter:
    """Wow & flutter-modulation via delay-line med variabel delay.

    Implementeras som ringbuffer + Lagrange 3:e ordningens interpolation
    (avvägning mellan kvalitet och CPU-cost för v1).
    """

    def __init__(self,
                 sample_rate: int = 48_000,
                 speed: TapeSpeed = TapeSpeed.SPEED_19,
                 *,
                 amount: float = 1.0,    # 1.0 = nominell, 0 = av
                 seed: int | None = None):
        self.sample_rate = sample_rate
        self.amount = amount

        # Ring-buffer med några ms maximal delay (dimensionerat för max wow-djup)
        self._buf_len = int(sample_rate * 0.05)  # 50 ms — räcker väl
        self._buf = np.zeros(self._buf_len, dtype=np.float64)
        self._write_idx = 0

        # LFO-faser
        self._wow_phase = 0.0
        self._flutter_phase = 0.0
        self._rng = np.random.default_rng(seed)
        # Slumpfasiga små perturbationer (mekanisk slack)
        self._noise_phase = 0.0

        self.set_speed(speed)

    def set_speed(self, speed: TapeSpeed):
        spec = _WF_SPECS[speed]
        self._spec = spec
        self._speed = speed

    def reset(self):
        self._buf.fill(0)
        self._write_idx = 0
        self._wow_phase = 0.0
        self._flutter_phase = 0.0

    def _lagrange_interp(self, idx_float: float) -> float:
        """3:e ordningens Lagrange-interpolation från ring-bufferten.

        idx_float: floating-point read-position (lägre värde = nyare sample).
        """
        idx_int = int(np.floor(idx_float))
        frac = idx_float - idx_int

        # 4 samples runt punkten (för 3:e ordningens Lagrange)
        # Använd mod för cirkulär indexering
        i0 = (self._write_idx - idx_int - 1) % self._buf_len
        i1 = (self._write_idx - idx_int) % self._buf_len
        i2 = (self._write_idx - idx_int + 1) % self._buf_len
        i3 = (self._write_idx - idx_int + 2) % self._buf_len
        x0 = self._buf[i0]
        x1 = self._buf[i1]
        x2 = self._buf[i2]
        x3 = self._buf[i3]

        # Lagrange 3:e ordningen vid frac (0..1 mellan x1 och x2)
        c0 = -frac * (frac - 1) * (frac - 2) / 6.0
        c1 = (frac + 1) * (frac - 1) * (frac - 2) / 2.0
        c2 = -(frac + 1) * frac * (frac - 2) / 2.0
        c3 = (frac + 1) * frac * (frac - 1) / 6.0

        return c0 * x0 + c1 * x1 + c2 * x2 + c3 * x3

    def process(self, x: np.ndarray) -> np.ndarray:
        x = np.asarray(x, dtype=np.float64)
        n = len(x)
        out = np.empty(n, dtype=np.float64)

        spec = self._spec
        wow_omega = 2 * np.pi * spec.wow_freq_hz / self.sample_rate
        flutter_omega = 2 * np.pi * spec.flutter_freq_hz / self.sample_rate

        # Bas-delay (en konstant offset så vi har headroom åt båda hållen)
        base_delay = self._buf_len * 0.5

        for i in range(n):
            # Skriv input till buffer
            self._buf[self._write_idx] = x[i]
            self._write_idx = (self._write_idx + 1) % self._buf_len

            # Beräkna instant delay-modulation
            # Pitch-skift = relative speed-variation
            wow_mod = spec.wow_amount * np.sin(self._wow_phase) * self.amount
            flutter_mod = spec.flutter_amount * np.sin(self._flutter_phase) * self.amount

            # Delay-mod (i samples). Pitch-skift × bas-delay ≈ delay-modulation
            # Det är en approximation; för riktig pitch-shift krävs varierande sample-rate
            delay_samples = base_delay + (wow_mod + flutter_mod) * self.sample_rate * 0.005

            # Läs interpolerat
            out[i] = self._lagrange_interp(delay_samples)

            # Avancera LFO-faser
            self._wow_phase += wow_omega
            self._flutter_phase += flutter_omega
            if self._wow_phase > 2 * np.pi:
                self._wow_phase -= 2 * np.pi
            if self._flutter_phase > 2 * np.pi:
                self._flutter_phase -= 2 * np.pi

        return out
