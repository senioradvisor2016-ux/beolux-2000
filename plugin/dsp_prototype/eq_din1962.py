"""DIN-1962 EQ-modeller för record (pre-emphasis) och playback (de-emphasis).

3 koeff-set per modul, en per hastighet (19 / 9.5 / 4.75 cm/s).

Pre+de-emphasis är *inte* exakta inverser — mismatchen i 3–8 kHz ger den
karakteristiska "tape glow" som är en del av analog-tape-charmen.
Renormalisera inte bort.

Referens: planfil §1J + specs.md §6 + §13.
"""
from __future__ import annotations
from enum import Enum
from dataclasses import dataclass
import numpy as np
from scipy import signal


class TapeSpeed(Enum):
    SPEED_19   = "19"     # 19 cm/s = 7½ ips
    SPEED_9_5  = "9_5"    # 9.5 cm/s = 3¾ ips
    SPEED_4_75 = "4_75"   # 4.75 cm/s = 1⅞ ips


@dataclass
class _EQConfig:
    lf_corner: float
    lf_boost_db: float
    hf_corner: float
    hf_gain_db: float  # negativ för cut, positiv för boost


# Koefficient-set från specs.md §13 (approximation av RC-LC-fb-nät).
# LF-shelf reducerade 12/14/16 → 5/6/7 dB (oktober 2026) — tidigare gav den +
# head-bump en samlad +6.7 dB peak vid 50 Hz vilket fail:ade ±3 dB-spec.
# Den fysiska motivationen (kompensera tape-LF-känslighet) bevaras men
# kalibrerat så att net-respons är ±2 dB inom hela banden.
_PLAYBACK_CONFIGS = {
    TapeSpeed.SPEED_19:   _EQConfig(50, 5.0, 12_000, -2.0),
    TapeSpeed.SPEED_9_5:  _EQConfig(50, 6.0, 7_500, -3.0),
    TapeSpeed.SPEED_4_75: _EQConfig(50, 7.0, 4_000, -4.0),
}

_RECORD_CONFIGS = {
    # Record har bara HF-boost (pre-emphasis), ingen LF-shelf
    TapeSpeed.SPEED_19:   _EQConfig(0, 0.0, 15_000, 6.0),
    TapeSpeed.SPEED_9_5:  _EQConfig(0, 0.0, 9_000, 9.0),
    TapeSpeed.SPEED_4_75: _EQConfig(0, 0.0, 5_000, 12.0),
}


def _design_lf_shelf(corner_hz: float, gain_db: float, sample_rate: int) -> tuple[np.ndarray, np.ndarray]:
    """Lågfrekvens-shelf-filter (RBJ cookbook).

    Boostar/cuttar frekvenser under corner.
    """
    if abs(gain_db) < 0.01:
        return np.array([1.0, 0.0, 0.0]), np.array([1.0, 0.0, 0.0])
    A = 10 ** (gain_db / 40)
    w0 = 2 * np.pi * corner_hz / sample_rate
    cos_w0 = np.cos(w0)
    sin_w0 = np.sin(w0)
    S = 1.0  # shelf slope
    alpha = sin_w0 / 2 * np.sqrt((A + 1/A) * (1/S - 1) + 2)
    sqrtA_2alpha = 2 * np.sqrt(A) * alpha

    b0 =    A*((A + 1) - (A - 1)*cos_w0 + sqrtA_2alpha)
    b1 =  2*A*((A - 1) - (A + 1)*cos_w0)
    b2 =    A*((A + 1) - (A - 1)*cos_w0 - sqrtA_2alpha)
    a0 =       (A + 1) + (A - 1)*cos_w0 + sqrtA_2alpha
    a1 =   -2*((A - 1) + (A + 1)*cos_w0)
    a2 =       (A + 1) + (A - 1)*cos_w0 - sqrtA_2alpha

    return (np.array([b0/a0, b1/a0, b2/a0]),
            np.array([1.0,    a1/a0, a2/a0]))


def _design_hf_shelf(corner_hz: float, gain_db: float, sample_rate: int) -> tuple[np.ndarray, np.ndarray]:
    """Högfrekvens-shelf-filter (RBJ cookbook)."""
    if abs(gain_db) < 0.01:
        return np.array([1.0, 0.0, 0.0]), np.array([1.0, 0.0, 0.0])
    A = 10 ** (gain_db / 40)
    w0 = 2 * np.pi * corner_hz / sample_rate
    cos_w0 = np.cos(w0)
    sin_w0 = np.sin(w0)
    S = 1.0
    alpha = sin_w0 / 2 * np.sqrt((A + 1/A) * (1/S - 1) + 2)
    sqrtA_2alpha = 2 * np.sqrt(A) * alpha

    b0 =    A*((A + 1) + (A - 1)*cos_w0 + sqrtA_2alpha)
    b1 = -2*A*((A - 1) + (A + 1)*cos_w0)
    b2 =    A*((A + 1) + (A - 1)*cos_w0 - sqrtA_2alpha)
    a0 =       (A + 1) - (A - 1)*cos_w0 + sqrtA_2alpha
    a1 =    2*((A - 1) - (A + 1)*cos_w0)
    a2 =       (A + 1) - (A - 1)*cos_w0 - sqrtA_2alpha

    return (np.array([b0/a0, b1/a0, b2/a0]),
            np.array([1.0,    a1/a0, a2/a0]))


class _SwitchedEQ:
    """Bas-klass för switchad shelf-filter-EQ (LF-shelf + HF-shelf)."""

    def __init__(self, configs: dict[TapeSpeed, _EQConfig],
                 speed: TapeSpeed = TapeSpeed.SPEED_19,
                 sample_rate: int = 48_000):
        self.configs = configs
        self.sample_rate = sample_rate
        self._zi_lf = None
        self._zi_hf = None
        self._b_lf = None; self._a_lf = None
        self._b_hf = None; self._a_hf = None
        self.set_speed(speed)

    def set_speed(self, speed: TapeSpeed):
        cfg = self.configs[speed]
        self._b_lf, self._a_lf = _design_lf_shelf(cfg.lf_corner, cfg.lf_boost_db, self.sample_rate)
        self._b_hf, self._a_hf = _design_hf_shelf(cfg.hf_corner, cfg.hf_gain_db, self.sample_rate)
        self._zi_lf = signal.lfilter_zi(self._b_lf, self._a_lf) * 0.0
        self._zi_hf = signal.lfilter_zi(self._b_hf, self._a_hf) * 0.0
        self._speed = speed

    @property
    def speed(self) -> TapeSpeed:
        return self._speed

    def reset(self):
        self._zi_lf = signal.lfilter_zi(self._b_lf, self._a_lf) * 0.0
        self._zi_hf = signal.lfilter_zi(self._b_hf, self._a_hf) * 0.0

    def process(self, x: np.ndarray) -> np.ndarray:
        x = np.asarray(x, dtype=np.float64)
        y, self._zi_lf = signal.lfilter(self._b_lf, self._a_lf, x, zi=self._zi_lf)
        y, self._zi_hf = signal.lfilter(self._b_hf, self._a_hf, y, zi=self._zi_hf)
        return y


class PlaybackEQ_DIN1962(_SwitchedEQ):
    """Playback-EQ (de-emphasis) — 8004006 8s switchade FB-nät."""
    def __init__(self, speed: TapeSpeed = TapeSpeed.SPEED_19, sample_rate: int = 48_000):
        super().__init__(_PLAYBACK_CONFIGS, speed, sample_rate)


class PreEmphasisDIN1962(_SwitchedEQ):
    """Record-EQ (pre-emphasis) — 8004005's switch-deck 0854661."""
    def __init__(self, speed: TapeSpeed = TapeSpeed.SPEED_19, sample_rate: int = 48_000):
        super().__init__(_RECORD_CONFIGS, speed, sample_rate)
