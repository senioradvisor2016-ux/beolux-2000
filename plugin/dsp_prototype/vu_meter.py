"""VU-meter — 300 ms ballistik, sort/röd-zonering enligt manualens spec.

Manualen s.5: 'Instrumentet skal normalt arbejde i det sorte område og må
kun sporadisk bevæge sig ind i det røde felt. Fuldt udslag vil medføre
forvrængede optagelser.'

Implementation: integrerar |x| med 300 ms RC-time-constant. Output i dBu
relativt referensen 0 dBu = 0.775 V RMS.

Referens: planfil §1D punkt m + specs.md §5.
"""
from __future__ import annotations
import numpy as np


class VUMeter:
    """VU-meter med 300 ms attack/release-ballistik.

    Output:
    - level_db: instant-värde i dBFS
    - peak_db:  peak-hold-värde (1.5 s decay)
    - in_red_zone: True om level > +3 dBFS
    """

    REFERENCE_LEVEL = 0.775  # 0 dBu = 0.775 V RMS

    def __init__(self, sample_rate: int = 48_000,
                 ballistic_ms: float = 300.0,
                 peak_hold_ms: float = 1500.0):
        self.sample_rate = sample_rate
        # RC-time-constant: tau = ballistic_ms / 1000
        # 1-pole IIR: y[n] = α·x[n] + (1-α)·y[n-1] med α = dt/tau
        self._alpha = 1.0 / (1.0 + sample_rate * (ballistic_ms / 1000) / np.log(0.9))
        self._alpha = abs(self._alpha)
        # Peak-decay alpha
        self._peak_alpha = 1.0 / (sample_rate * peak_hold_ms / 1000)
        self._integrated = 0.0
        self._peak = 0.0

    def reset(self):
        self._integrated = 0.0
        self._peak = 0.0

    def process(self, x: np.ndarray) -> tuple[float, float, bool]:
        """Mata in audio-buffer, få ut level_db, peak_db, in_red_zone."""
        x = np.asarray(x, dtype=np.float64)
        # RMS i bufferten (snabb approx)
        if len(x) == 0:
            return self._integrated, self._peak, False

        # Sample-by-sample RC-integrering på |x|^2 (då tar vi sqrt för RMS)
        a = self._alpha
        for v in np.abs(x):
            self._integrated = a * v + (1 - a) * self._integrated
            if v > self._peak:
                self._peak = v

        # Peak-decay
        decay_factor = 1 - self._peak_alpha * len(x)
        self._peak *= max(decay_factor, 0.0)

        level_db = 20 * np.log10(self._integrated + 1e-12)
        peak_db = 20 * np.log10(self._peak + 1e-12)
        # Red zone: > -3 dBFS (motsvarar manualens "fuldt udslag" varning)
        in_red = level_db > -3.0

        return level_db, peak_db, in_red

    @property
    def level_db(self) -> float:
        return 20 * np.log10(self._integrated + 1e-12)

    @property
    def peak_db(self) -> float:
        return 20 * np.log10(self._peak + 1e-12)
