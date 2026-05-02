"""MicTransformer8012003 — DSP-modell av B&O:s mic-trafo i 8904004.

LF-roll-off (kan inte passera DC), HF-roll-off (leakage-induktans + lindnings-
kapacitans), step-up-gain, och mjuk Jiles-Atherton-kärnsaturation vid hög nivå.

Step-up-ratio ~1:20 ger känslighet 50 µV → 1 mV, matchar manualens spec.

Referens: planfil §1F.
"""
from __future__ import annotations
import numpy as np
from scipy import signal


class MicTransformer8012003:
    """Ingångstransformator 8012003 (lo-Z mic).

    Modellerar:
    - Step-up gain (turns ratio 1:20)
    - LF-roll-off ~25 Hz (1:a ordningens HP)
    - HF-roll-off ~30 kHz (1:a ordningens LP)
    - Mjuk kärn-saturation vid hög nivå (Jiles-Atherton-approximation)
    """

    def __init__(self,
                 sample_rate: int = 48_000,
                 *,
                 turns_ratio: float = 1.5,
                 lf_corner_hz: float = 25.0,
                 hf_corner_hz: float = 30_000.0,
                 saturation_threshold_v: float = 4.0,
                 saturation_softness: float = 1.5):
        """
        Parameters
        ----------
        turns_ratio : float
            Sekundär/primär. 20.0 = +26 dB step-up.
        lf_corner_hz : float
            -3 dB LF-knee från trafons primärinduktans.
        hf_corner_hz : float
            -3 dB HF-knee från leakage + lindningskapacitans.
        saturation_threshold_v : float
            B-fält över detta börjar sättas. ~+15 dBu på sekundärsidan.
        saturation_softness : float
            > 1 = mjukare knee. 1.5 är typiskt B&O-trafo-värde.
        """
        self.sample_rate = sample_rate
        self.turns_ratio = turns_ratio
        self.saturation_threshold = saturation_threshold_v
        self.saturation_softness = saturation_softness

        # Bygg HP-LP-cascade (Butterworth 1:a ordningen) med b/a-koefficienter
        nyq = sample_rate / 2
        self._b_hp, self._a_hp = signal.butter(1, lf_corner_hz / nyq, btype="highpass")
        hf_norm = min(hf_corner_hz / nyq, 0.95)
        self._b_lp, self._a_lp = signal.butter(1, hf_norm, btype="lowpass")
        self._zi_hp = signal.lfilter_zi(self._b_hp, self._a_hp) * 0.0
        self._zi_lp = signal.lfilter_zi(self._b_lp, self._a_lp) * 0.0

    def reset(self):
        """Nollställ filter-state. Anropa mellan icke-kontinuerliga buffrar."""
        self._zi_hp = signal.lfilter_zi(self._b_hp, self._a_hp) * 0.0
        self._zi_lp = signal.lfilter_zi(self._b_lp, self._a_lp) * 0.0

    def _saturate(self, x: np.ndarray) -> np.ndarray:
        """Mjuk kärn-saturation (Jiles-Atherton-approximation via tanh).

        Vid amplituder under threshold är kurvan nästan linjär.
        Över threshold mjuk knee mot asymptot.
        """
        thr = self.saturation_threshold
        soft = self.saturation_softness
        # Skala så att |x| = thr ger ~70 % av asymptotiskt max
        scale = thr * soft
        return scale * np.tanh(x / scale)

    def process(self, x: np.ndarray) -> np.ndarray:
        """Processa primär-side input → sekundär-side output."""
        x = np.asarray(x, dtype=np.float64)
        # 1. Step-up
        y = x * self.turns_ratio
        # 2. LF-roll-off (kapacitiv koppling kan ej passera DC)
        y, self._zi_hp = signal.lfilter(self._b_hp, self._a_hp, y, zi=self._zi_hp)
        # 3. Kärn-saturation
        y = self._saturate(y)
        # 4. HF-roll-off (leakage-induktans + lindningskapacitans)
        y, self._zi_lp = signal.lfilter(self._b_lp, self._a_lp, y, zi=self._zi_lp)
        return y

    def frequency_response(self, n_fft: int = 4096) -> tuple[np.ndarray, np.ndarray]:
        """Returnera (frekvenser, magnitude_db) för diagnostik."""
        impulse = np.zeros(n_fft)
        impulse[0] = 1.0 / self.turns_ratio  # input som ger unity efter step-up
        self.reset()
        out = self.process(impulse)
        H = np.fft.rfft(out)
        freqs = np.fft.rfftfreq(n_fft, 1.0 / self.sample_rate)
        return freqs, 20 * np.log10(np.abs(H) + 1e-12)
