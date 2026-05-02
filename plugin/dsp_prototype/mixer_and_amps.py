"""Mixer3Bus + Record/Playback amps + PowerAmp.

Sätter ihop preamps och tape-amps till kompletta mod-blocks som matchar
respektive 8004005 / 8004006 / 8004014.

Referens: planfil §1H + §1I + §1J.
"""
from __future__ import annotations
import numpy as np
from scipy import signal

from ge_stages import Ge2N2613Stage, GeLowNoiseStage, GeStageType
from eq_din1962 import PreEmphasisDIN1962, PlaybackEQ_DIN1962, TapeSpeed


# ---------- Mixer3Bus — manualens skydepotentiometre ----------
class Mixer3Bus:
    """3-kanals input-mixer (Mic + Phono + Radio).

    Manualens unika egenskap: alla tre kan mixas samtidigt med var sin gain.
    """

    def __init__(self,
                 mic_gain: float = 0.0,    # 0–1.0 (linjär)
                 phono_gain: float = 0.0,
                 radio_gain: float = 0.0):
        self.mic_gain = mic_gain
        self.phono_gain = phono_gain
        self.radio_gain = radio_gain

    def set_gains(self, *, mic: float = None, phono: float = None, radio: float = None):
        if mic is not None:
            self.mic_gain = float(np.clip(mic, 0, 1))
        if phono is not None:
            self.phono_gain = float(np.clip(phono, 0, 1))
        if radio is not None:
            self.radio_gain = float(np.clip(radio, 0, 1))

    def process(self,
                mic: np.ndarray | None = None,
                phono: np.ndarray | None = None,
                radio: np.ndarray | None = None) -> np.ndarray:
        """Summera 3 input-bussar med respektive gain."""
        # Hitta längd från första icke-None signal
        ref = next((s for s in (mic, phono, radio) if s is not None), None)
        if ref is None:
            return np.zeros(0)
        n = len(ref)
        out = np.zeros(n, dtype=np.float64)

        if mic is not None and self.mic_gain > 0:
            out += mic * self.mic_gain
        if phono is not None and self.phono_gain > 0:
            out += phono * self.phono_gain
        if radio is not None and self.radio_gain > 0:
            out += radio * self.radio_gain

        return out


# ---------- RecordAmp 8004005 ----------
class RecordAmp8004005:
    """Record-amp: pre-emphasis → AC126 + AC126 + 2N2613 → record-head."""

    def __init__(self,
                 sample_rate: int = 48_000,
                 speed: TapeSpeed = TapeSpeed.SPEED_19,
                 channel_asymmetry: float = 0.0,
                 *,
                 noise_seed: int | None = None):
        self.sample_rate = sample_rate
        self.pre_emphasis = PreEmphasisDIN1962(speed=speed, sample_rate=sample_rate)
        # 3-stegs cascade: AC126 + AC126 + 2N2613
        # Realistisk gain-staging: low gain per stage, ~30 dB total
        # Reducerade för att undvika cascade-saturation (se preamps.py)
        self.stage1 = GeLowNoiseStage(
            GeStageType.AC126, gain_db=3.0,
            sample_rate=sample_rate,
            noise_seed=(noise_seed or 0) * 3 + 300,
            channel_asymmetry=channel_asymmetry)
        self.stage2 = GeLowNoiseStage(
            GeStageType.AC126, gain_db=2.0,
            sample_rate=sample_rate,
            noise_seed=(noise_seed or 0) * 3 + 301,
            channel_asymmetry=channel_asymmetry)
        self.stage3 = Ge2N2613Stage(
            gain_db=1.0,
            sample_rate=sample_rate,
            noise_seed=(noise_seed or 0) * 3 + 302,
            channel_asymmetry=channel_asymmetry)

    def reset(self):
        self.pre_emphasis.reset()

    def set_speed(self, speed: TapeSpeed):
        self.pre_emphasis.set_speed(speed)

    def process(self, x: np.ndarray) -> np.ndarray:
        y = self.pre_emphasis.process(x)
        y = self.stage1.process(y)
        y = self.stage2.process(y)
        y = self.stage3.process(y)
        return y


# ---------- PlaybackAmp 8004006 ----------
class PlaybackAmp8004006:
    """Playback-amp: AC126 + AC126 + 2N2613 → de-emphasis (FB-nätet)."""

    def __init__(self,
                 sample_rate: int = 48_000,
                 speed: TapeSpeed = TapeSpeed.SPEED_19,
                 channel_asymmetry: float = 0.0,
                 *,
                 noise_seed: int | None = None):
        self.sample_rate = sample_rate
        # Playback-EQ är i FB-nätet → påverkar gain × frekvensgång
        self.de_emphasis = PlaybackEQ_DIN1962(speed=speed, sample_rate=sample_rate)
        # Realistisk gain-staging: ~30 dB total (matchar serviceman. 0.5 mV → ~15 mV)
        self.stage1 = GeLowNoiseStage(
            GeStageType.AC126, gain_db=3.0,
            sample_rate=sample_rate,
            noise_seed=(noise_seed or 0) * 3 + 400,
            channel_asymmetry=channel_asymmetry)
        self.stage2 = GeLowNoiseStage(
            GeStageType.AC126, gain_db=2.0,
            sample_rate=sample_rate,
            noise_seed=(noise_seed or 0) * 3 + 401,
            channel_asymmetry=channel_asymmetry)
        self.stage3 = Ge2N2613Stage(
            gain_db=1.0,
            sample_rate=sample_rate,
            noise_seed=(noise_seed or 0) * 3 + 402,
            channel_asymmetry=channel_asymmetry)

    def reset(self):
        self.de_emphasis.reset()

    def set_speed(self, speed: TapeSpeed):
        self.de_emphasis.set_speed(speed)

    def process(self, x: np.ndarray) -> np.ndarray:
        # Playback-EQ är i FB-nätet → applicerad efter stage 1 (förenklat)
        y = self.stage1.process(x)
        y = self.de_emphasis.process(y)
        y = self.stage2.process(y)
        y = self.stage3.process(y)
        return y


# ---------- PowerAmp 8004014 (förenklad) ----------
class PowerAmp8004014:
    """Klass-AB power-amp: 2N2613 → AC153 → AC127+AC132 → 2× AD139.

    För v1 förenklas detta till:
    - 2N2613-input (befintlig)
    - Generic voltage gain (AC153 → 2× AD139)
    - Crossover-distorsion-modell (AC127+AC132 mismatch)
    - AUTOMATSIKRING soft-clip vid +14 dBu
    - Cap-koppling till high-Z-load (HP-filter ~5 Hz)
    """

    def __init__(self,
                 sample_rate: int = 48_000,
                 *,
                 channel_asymmetry: float = 0.0,
                 noise_seed: int | None = None,
                 enabled: bool = True):
        self.sample_rate = sample_rate
        self.enabled = enabled

        # Input-stage gain 0 dB — power-amp ska vara nästan transparent när
        # signalen redan har korrekt nivå från preamp-kedjan.
        self.input_stage = Ge2N2613Stage(
            gain_db=0.0,
            sample_rate=sample_rate,
            noise_seed=(noise_seed or 0) * 2 + 500,
            channel_asymmetry=channel_asymmetry)

        # Cap-coupling HP @ 5 Hz
        nyq = sample_rate / 2
        self._b_hp, self._a_hp = signal.butter(1, 5.0 / nyq, btype="highpass")
        self._zi_hp = signal.lfilter_zi(self._b_hp, self._a_hp) * 0.0

        # Crossover-distorsion-amplitud (AC127/132-mismatch)
        self.crossover_threshold = 0.005
        # AUTOMATSIKRING-tröskel höjd 2.5 så power-amp är nästan transparent
        # vid -3 dBFS in (nominell ref-nivå); soft-clip kicks in bara vid
        # genuin overdrive ≥ +5 dBFS.
        self.automatsikring_threshold = 2.5

    def reset(self):
        self._zi_hp = signal.lfilter_zi(self._b_hp, self._a_hp) * 0.0

    def _crossover_distortion(self, x: np.ndarray) -> np.ndarray:
        """Smooth crossover-distorsion (germanium-class-AB AC127/132-mismatch).

        Tidigare hard-knee `where(mag<thr, mag*0.9, mag)` skapade en derivativ-
        diskontinuitet vid mag=thr → varje noll-genomgång → massiv harmonik.
        Exp-tappad smooth-knee bevarar crossover-färgningen men är C^∞.
        """
        thr = self.crossover_threshold
        sign = np.sign(x)
        mag = np.abs(x)
        kSoftness = 0.10
        dent = kSoftness * np.exp(-mag / thr)
        return sign * mag * (1.0 - dent)

    def _automatsikring(self, x: np.ndarray) -> np.ndarray:
        """Soft-clip vid +14 dBu (overload-skydd)."""
        thr = self.automatsikring_threshold
        return thr * np.tanh(x / thr)

    def process(self, x: np.ndarray) -> np.ndarray:
        if not self.enabled:
            return x
        # OBS: input_stage borttaget från process-pipelinen. Det dubblerade
        # GE-distorsion redan applicerad genom preamp-kedjan.
        # 1. Crossover-distorsion (germanium-class-AB-signatur)
        y = self._crossover_distortion(x)
        # 2. AUTOMATSIKRING soft-clip
        y = self._automatsikring(y)
        # 3. Cap-coupling HP
        y, self._zi_hp = signal.lfilter(self._b_hp, self._a_hp, y, zi=self._zi_hp)
        return y
