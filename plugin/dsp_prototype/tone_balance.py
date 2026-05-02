"""Tone control (Baxandall) + Balance + Master volume.

VIKTIG enligt manualen s.5:
   'Tonekontrollerne (8) ♭ og (9) ♮. Disse stillinger indvirker ikke på optagelsen.'

→ Tone-kontrollen ligger EFTER playback-amp, INTE före record. SignalChain
respekterar detta.

Referens: planfil §1D punkt b + §2 Block 6.
"""
from __future__ import annotations
import numpy as np
from scipy import signal


class ToneControl:
    """2-bands Baxandall-tone (knap 8 = treble, knap 9 = bass).

    Implementeras som LF-shelf @ 100 Hz + HF-shelf @ 10 kHz, ±12 dB range.
    """

    def __init__(self,
                 sample_rate: int = 48_000,
                 *,
                 bass_db: float = 0.0,
                 treble_db: float = 0.0,
                 bass_corner_hz: float = 100.0,
                 treble_corner_hz: float = 10_000.0):
        self.sample_rate = sample_rate
        self._bass_corner = bass_corner_hz
        self._treble_corner = treble_corner_hz
        self._b_bass = self._a_bass = None
        self._zi_bass = None
        self._b_treble = self._a_treble = None
        self._zi_treble = None
        self.set_bass(bass_db)
        self.set_treble(treble_db)

    def set_bass(self, gain_db: float):
        self._bass_db = float(np.clip(gain_db, -12, 12))
        self._b_bass, self._a_bass = self._design_lf_shelf(
            self._bass_corner, self._bass_db)
        self._zi_bass = signal.lfilter_zi(self._b_bass, self._a_bass) * 0.0

    def set_treble(self, gain_db: float):
        self._treble_db = float(np.clip(gain_db, -12, 12))
        self._b_treble, self._a_treble = self._design_hf_shelf(
            self._treble_corner, self._treble_db)
        self._zi_treble = signal.lfilter_zi(self._b_treble, self._a_treble) * 0.0

    def _design_lf_shelf(self, corner_hz: float, gain_db: float):
        if abs(gain_db) < 0.01:
            return np.array([1.0, 0.0, 0.0]), np.array([1.0, 0.0, 0.0])
        A = 10 ** (gain_db / 40)
        w0 = 2 * np.pi * corner_hz / self.sample_rate
        cos_w0 = np.cos(w0)
        sin_w0 = np.sin(w0)
        S = 1.0
        alpha = sin_w0 / 2 * np.sqrt((A + 1/A) * (1/S - 1) + 2)
        sqrtA_2alpha = 2 * np.sqrt(A) * alpha
        b0 =    A*((A + 1) - (A - 1)*cos_w0 + sqrtA_2alpha)
        b1 =  2*A*((A - 1) - (A + 1)*cos_w0)
        b2 =    A*((A + 1) - (A - 1)*cos_w0 - sqrtA_2alpha)
        a0 =       (A + 1) + (A - 1)*cos_w0 + sqrtA_2alpha
        a1 =   -2*((A - 1) + (A + 1)*cos_w0)
        a2 =       (A + 1) + (A - 1)*cos_w0 - sqrtA_2alpha
        return (np.array([b0/a0, b1/a0, b2/a0]),
                np.array([1.0, a1/a0, a2/a0]))

    def _design_hf_shelf(self, corner_hz: float, gain_db: float):
        if abs(gain_db) < 0.01:
            return np.array([1.0, 0.0, 0.0]), np.array([1.0, 0.0, 0.0])
        A = 10 ** (gain_db / 40)
        w0 = 2 * np.pi * corner_hz / self.sample_rate
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
                np.array([1.0, a1/a0, a2/a0]))

    def reset(self):
        self._zi_bass = signal.lfilter_zi(self._b_bass, self._a_bass) * 0.0
        self._zi_treble = signal.lfilter_zi(self._b_treble, self._a_treble) * 0.0

    def process(self, x: np.ndarray) -> np.ndarray:
        x = np.asarray(x, dtype=np.float64)
        y, self._zi_bass = signal.lfilter(self._b_bass, self._a_bass, x, zi=self._zi_bass)
        y, self._zi_treble = signal.lfilter(self._b_treble, self._a_treble, y, zi=self._zi_treble)
        return y


class BalanceMaster:
    """Stereo balance + master volume — sista stage före output.

    balance: -1.0 (full L) ... 0.0 (centrerad) ... +1.0 (full R)
    master: 0.0 ... 1.0 (linjär, mappar till -∞ ... 0 dB)
    """

    def __init__(self, *, balance: float = 0.0, master: float = 0.75):
        self.balance = float(np.clip(balance, -1, 1))
        self.master = float(np.clip(master, 0, 1))

    def process(self, l: np.ndarray, r: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
        # Balance som equal-power-pan (cosinus-kurva)
        bal = self.balance
        l_gain = np.cos((bal + 1) * np.pi / 4)  # 1.0 vid bal=-1, 0.707 vid 0, 0 vid +1
        r_gain = np.sin((bal + 1) * np.pi / 4)  # 0 vid bal=-1, 0.707 vid 0, 1.0 vid +1

        # Master som dB-mapping: master=1.0 → 0 dB, master=0.5 → -12 dB, master=0 → mute
        if self.master < 1e-6:
            master_lin = 0.0
        else:
            master_lin = self.master ** 2  # square-law för naturlig fader-känsla

        return (l * l_gain * master_lin,
                r * r_gain * master_lin)
