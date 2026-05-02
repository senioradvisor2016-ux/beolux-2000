"""Input preamps — Mic / Phono / Radio.

Sätter ihop befintliga byggstenar (transformer + Ge-stages) till kompletta
input-preamp-modeller motsvarande 8904004 / 8904002 / 8904003.

Referens: planfil §1F (mic) + §1G (phono) + §1J (radio).
"""
from __future__ import annotations
from enum import Enum
import numpy as np
from scipy import signal

from ge_stages import Ge2N2613Stage, GeLowNoiseStage, GeStageType
from transformer import MicTransformer8012003


class MicInputMode(Enum):
    LO_Z = "lo_z"   # 50–200 Ω genom trafo
    HI_Z = "hi_z"   # 500 kΩ förbi trafo


class PhonoMode(Enum):
    H = "h"  # Magnetisk (Beogram 1000 VF) — RIAA-aktiverat
    L = "l"  # Keramisk (Beogram 1000 V) — flat


class RadioMode(Enum):
    L = "l"  # 3 mV / 47 kΩ low-output
    H = "h"  # 100 mV / 100 kΩ line-level


# ---------- MicPreamp 8904004 ----------
class MicPreamp8904004:
    """Komplett mic-preamp: Trafo + UW0029 + 2N2613.

    Lo-Z-läge (default): signal går genom trafo + båda stages.
    Hi-Z-läge: signal går *förbi* trafo, direkt till UW0029.

    Stereo: två separata kedjor med autentisk per-kanal-asymmetri.
    """

    def __init__(self,
                 sample_rate: int = 48_000,
                 mode: MicInputMode = MicInputMode.LO_Z,
                 channel_asymmetry: float = 0.0,
                 *,
                 noise_seed: int | None = None):
        self.sample_rate = sample_rate
        self.mode = mode

        self.transformer = MicTransformer8012003(sample_rate=sample_rate)
        # OBS: kraftigt reducerade gains för att matcha feedback-limited verkligt
        # beteende (real BC2000DL har lokal NFB i varje stage). Tidigare 30+20 dB
        # cascade fick varje stage att saturera oavsett input — nu 4+2 dB ger
        # linjär kärna med tape-saturation som dominerande färgningskälla.
        self.stage_uw0029 = GeLowNoiseStage(
            GeStageType.UW0029, gain_db=4.0,
            sample_rate=sample_rate,
            noise_seed=(noise_seed or 0) * 2 + 100,
            channel_asymmetry=channel_asymmetry)
        self.stage_2n2613 = Ge2N2613Stage(
            gain_db=2.0,
            sample_rate=sample_rate,
            noise_seed=(noise_seed or 0) * 2 + 101,
            channel_asymmetry=channel_asymmetry)

    def reset(self):
        self.transformer.reset()

    def process(self, x: np.ndarray) -> np.ndarray:
        # Lo-Z: trafo aktiverad → step-up + filter + saturation
        # Hi-Z: trafo förbikopplad
        if self.mode == MicInputMode.LO_Z:
            y = self.transformer.process(x)
        else:
            # Hi-Z: extra +20 dB nominell input-gain (kompenserar för avsaknad
            # av trafo-step-up). Plus subtil HF-roll-off från Miller-effekt.
            y = x * 5.0  # ~+14 dB
            # Mild Miller-roll-off vid ~50 kHz (per specs §1E)
            nyq = self.sample_rate / 2
            if 50_000 / nyq < 0.95:
                b, a = signal.butter(1, 50_000 / nyq, btype="lowpass")
                y = signal.lfilter(b, a, y)

        y = self.stage_uw0029.process(y)
        y = self.stage_2n2613.process(y)
        return y

    @classmethod
    def stereo_pair(cls, sample_rate: int = 48_000,
                    mode: MicInputMode = MicInputMode.LO_Z,
                    asymmetry_amount: float = 0.075,
                    seed_l: int | None = 200,
                    seed_r: int | None = 201
                    ) -> tuple["MicPreamp8904004", "MicPreamp8904004"]:
        return (
            cls(sample_rate=sample_rate, mode=mode,
                channel_asymmetry=+asymmetry_amount, noise_seed=seed_l),
            cls(sample_rate=sample_rate, mode=mode,
                channel_asymmetry=-asymmetry_amount, noise_seed=seed_r),
        )


# ---------- PhonoPreamp 8904002 ----------
def _design_riaa_inverse(sample_rate: int) -> tuple[np.ndarray, np.ndarray]:
    """B&O-approximation av RIAA-equalisering för phono H-läge.

    Inte exakt RIAA — B&O hade en egen implementation. Approximation:
    - LF-boost ~+20 dB vid 50 Hz (RIAA t1=3180 µs)
    - Flat region kring 1 kHz
    - HF-cut -3 dB över ~12 kHz (RIAA t3=75 µs)
    """
    # Förenklad: kombinera LF-shelf + HF-shelf
    nyq = sample_rate / 2
    # LF-shelf vid 50 Hz, +20 dB boost
    from eq_din1962 import _design_lf_shelf, _design_hf_shelf
    b_lf, a_lf = _design_lf_shelf(50, 20, sample_rate)
    b_hf, a_hf = _design_hf_shelf(12_000, -2, sample_rate)
    # Cascade
    b = np.convolve(b_lf, b_hf)
    a = np.convolve(a_lf, a_hf)
    return b, a


class PhonoPreamp8904002:
    """Phono-preamp: UW0029 + 2N2613 + HS-switch (H/L)."""

    def __init__(self,
                 sample_rate: int = 48_000,
                 mode: PhonoMode = PhonoMode.H,
                 channel_asymmetry: float = 0.0,
                 *,
                 noise_seed: int | None = None):
        self.sample_rate = sample_rate
        self.mode = mode

        # Reducerade gains (se MicPreamp-kommentar) — feedback-limited approx.
        gain_uw = 4.0 if mode == PhonoMode.H else 2.0
        gain_2n = 2.0

        self.stage_uw0029 = GeLowNoiseStage(
            GeStageType.UW0029, gain_db=gain_uw,
            sample_rate=sample_rate,
            noise_seed=(noise_seed or 0) * 2 + 110,
            channel_asymmetry=channel_asymmetry)
        self.stage_2n2613 = Ge2N2613Stage(
            gain_db=gain_2n,
            sample_rate=sample_rate,
            noise_seed=(noise_seed or 0) * 2 + 111,
            channel_asymmetry=channel_asymmetry)

        # RIAA-filter för H-läge
        if mode == PhonoMode.H:
            self._b_riaa, self._a_riaa = _design_riaa_inverse(sample_rate)
            self._zi_riaa = signal.lfilter_zi(self._b_riaa, self._a_riaa) * 0.0
        else:
            self._b_riaa = self._a_riaa = self._zi_riaa = None

    def reset(self):
        if self._zi_riaa is not None:
            self._zi_riaa = signal.lfilter_zi(self._b_riaa, self._a_riaa) * 0.0

    def process(self, x: np.ndarray) -> np.ndarray:
        x = np.asarray(x, dtype=np.float64)
        # 1. UW0029
        y = self.stage_uw0029.process(x)
        # 2. RIAA-EQ (om H-läge) — i FB-nätet, men för enkelhet placerad mellan stagen
        if self._b_riaa is not None:
            y, self._zi_riaa = signal.lfilter(
                self._b_riaa, self._a_riaa, y, zi=self._zi_riaa)
        # 3. 2N2613
        y = self.stage_2n2613.process(y)
        return y


# ---------- RadioPreamp 8904003 ----------
class RadioPreamp8904003:
    """Radio-preamp: UW0029 + 2N2613 (flat — ingen RIAA).

    L-läge: 3 mV/47 kΩ low-output radio (~50 dB total gain)
    H-läge: 100 mV/100 kΩ line-level radio (~30 dB total gain)
    """

    def __init__(self,
                 sample_rate: int = 48_000,
                 mode: RadioMode = RadioMode.L,
                 channel_asymmetry: float = 0.0,
                 *,
                 noise_seed: int | None = None):
        self.sample_rate = sample_rate
        self.mode = mode

        gain_uw = 4.0 if mode == RadioMode.L else 2.0
        gain_2n = 2.0 if mode == RadioMode.L else 1.0

        self.stage_uw0029 = GeLowNoiseStage(
            GeStageType.UW0029, gain_db=gain_uw,
            sample_rate=sample_rate,
            noise_seed=(noise_seed or 0) * 2 + 120,
            channel_asymmetry=channel_asymmetry)
        self.stage_2n2613 = Ge2N2613Stage(
            gain_db=gain_2n,
            sample_rate=sample_rate,
            noise_seed=(noise_seed or 0) * 2 + 121,
            channel_asymmetry=channel_asymmetry)

    def reset(self):
        pass

    def process(self, x: np.ndarray) -> np.ndarray:
        y = self.stage_uw0029.process(x)
        y = self.stage_2n2613.process(y)
        return y
