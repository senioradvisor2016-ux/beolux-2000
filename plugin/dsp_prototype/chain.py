"""SignalChain — full BC2000DL DSP-pipeline.

Länkar samman alla byggstenar till en komplett kedja:

  3 input-bussar (Mic/Phono/Radio)
        ↓
  Mixer3Bus (skydepotentiometre)
        ↓
  RecordAmp 8004005 (pre-emphasis + AC126×2 + 2N2613)
        ↓
  TapeSaturation (B-H-hysteres + 100 kHz bias + hastighetsspecifika filter)
        ↓
  WowFlutter (3-speed delay-line modulation)
        ↓
  PlaybackAmp 8004006 (AC126×2 + 2N2613 + de-emphasis)
        ↓
  Monitor switch (Source / Tape)
        ↓
  PowerAmp 8004014 (eller bypass) → OUT

Stereo-asymmetri: två separata kedjor (L+R) med autentiska 1968-toleranser.
"""
from __future__ import annotations
from dataclasses import dataclass, field
from enum import Enum
import numpy as np

from scipy import signal as sig_filt

from preamps import (
    MicPreamp8904004, PhonoPreamp8904002, RadioPreamp8904003,
    MicInputMode, PhonoMode, RadioMode,
)
from mixer_and_amps import (
    Mixer3Bus, RecordAmp8004005, PlaybackAmp8004006, PowerAmp8004014,
)
from tape import TapeSaturation
from wow_flutter import WowFlutter
from eq_din1962 import TapeSpeed
from effects import Echo, SoundOnSound, Multiplay
from tone_balance import ToneControl, BalanceMaster


class MonitorMode(Enum):
    SOURCE = "source"  # Lyssna på input INNAN tape (medhør)
    TAPE   = "tape"    # Lyssna på output EFTER tape (afhør)


@dataclass
class ChainConfig:
    """Top-level konfiguration av kedjan."""
    sample_rate: int = 48_000
    speed: TapeSpeed = TapeSpeed.SPEED_19
    monitor: MonitorMode = MonitorMode.TAPE
    # Mixer-gains (0–1)
    mic_gain: float = 0.0
    phono_gain: float = 0.0
    radio_gain: float = 0.0
    # Input-mode-flaggor
    mic_mode: MicInputMode = MicInputMode.LO_Z
    phono_mode: PhonoMode = PhonoMode.H
    radio_mode: RadioMode = RadioMode.L
    # Tape-parametrar
    bias_amount: float = 1.0
    saturation_drive: float = 1.0
    print_through: float = 0.0
    wow_flutter_amount: float = 1.0
    # Output
    bypass_tape: bool = False     # Knap 23 — amp-alone-läge
    speaker_monitor: bool = False  # PowerAmp aktiv (annars line-out)
    # Per-kanal-asymmetri
    asymmetry: float = 0.075
    # Echo (knap 18 + 10a)
    echo_enabled: bool = False
    echo_amount: float = 0.0       # 0–1 (level)
    # S-on-S (separat knap)
    sos_enabled: bool = False
    sos_amount: float = 0.0
    # Multiplay
    multiplay_enabled: bool = False
    multiplay_generation: int = 1
    # Synchroplay (knap 26)
    synchroplay: bool = False
    # Tone control (knap 8 = treble, 9 = bass)
    treble_db: float = 0.0
    bass_db: float = 0.0
    # Balance + master volume (knap 10 + 12)
    balance: float = 0.0           # -1=L, 0=mid, +1=R
    master_volume: float = 0.75    # 0–1


class SignalChainMono:
    """En mono-kedja. Stereo görs av SignalChain (par av dessa)."""

    def __init__(self, config: ChainConfig, *, channel_offset: float = 0.0,
                 noise_seed: int | None = None):
        self.config = config
        self._asym = channel_offset  # +0.075 = vänster, -0.075 = höger

        sr = config.sample_rate

        # Input-preamps
        self.mic = MicPreamp8904004(
            sample_rate=sr, mode=config.mic_mode,
            channel_asymmetry=self._asym, noise_seed=noise_seed)
        self.phono = PhonoPreamp8904002(
            sample_rate=sr, mode=config.phono_mode,
            channel_asymmetry=self._asym, noise_seed=noise_seed)
        self.radio = RadioPreamp8904003(
            sample_rate=sr, mode=config.radio_mode,
            channel_asymmetry=self._asym, noise_seed=noise_seed)

        # Mixer
        self.mixer = Mixer3Bus(
            mic_gain=config.mic_gain,
            phono_gain=config.phono_gain,
            radio_gain=config.radio_gain)

        # Tape-kedjan
        self.record_amp = RecordAmp8004005(
            sample_rate=sr, speed=config.speed,
            channel_asymmetry=self._asym, noise_seed=noise_seed)
        self.tape = TapeSaturation(
            sample_rate=sr, speed=config.speed,
            bias_amount=config.bias_amount,
            saturation_drive=config.saturation_drive,
            print_through=config.print_through,
            noise_seed=noise_seed)
        self.wow_flutter = WowFlutter(
            sample_rate=sr, speed=config.speed,
            amount=config.wow_flutter_amount,
            seed=noise_seed)
        self.playback_amp = PlaybackAmp8004006(
            sample_rate=sr, speed=config.speed,
            channel_asymmetry=self._asym, noise_seed=noise_seed)

        # Power-amp (sista stage före OUT)
        self.power_amp = PowerAmp8004014(
            sample_rate=sr,
            channel_asymmetry=self._asym, noise_seed=noise_seed,
            enabled=config.speaker_monitor)

        # Echo (delay-line med per-speed-tid)
        self.echo = Echo(
            sample_rate=sr, speed=config.speed,
            amount=config.echo_amount, enabled=config.echo_enabled)

        # Multiplay (bounce-loop med generationsförluster)
        self.multiplay = Multiplay(
            sample_rate=sr,
            generation=config.multiplay_generation,
            enabled=config.multiplay_enabled)

        # Tone control (efter playback-amp, INTE före record per manual s.5)
        self.tone = ToneControl(
            sample_rate=sr,
            bass_db=config.bass_db,
            treble_db=config.treble_db)

        # Output DC-blocker (HP @ 20 Hz) — säkrar att asymmetri-DC inte
        # ackumuleras i utsignalen. Motsvarar slutlig kapacitiv koppling
        # till line-out i hårdvaran.
        nyq = sr / 2
        self._b_dc, self._a_dc = sig_filt.butter(2, 20.0 / nyq, btype="highpass")
        self._zi_dc = sig_filt.lfilter_zi(self._b_dc, self._a_dc) * 0.0

    def reset(self):
        self.mic.reset()
        self.phono.reset()
        self.radio.reset()
        self.record_amp.reset()
        self.tape.reset()
        self.wow_flutter.reset()
        self.playback_amp.reset()
        self.power_amp.reset()
        self.echo.reset()
        self.tone.reset()
        self._zi_dc = sig_filt.lfilter_zi(self._b_dc, self._a_dc) * 0.0

    def _dc_block(self, x: np.ndarray) -> np.ndarray:
        y, self._zi_dc = sig_filt.lfilter(self._b_dc, self._a_dc, x, zi=self._zi_dc)
        return y

    def set_speed(self, speed: TapeSpeed):
        self.record_amp.set_speed(speed)
        self.tape.set_speed(speed)
        self.wow_flutter.set_speed(speed)
        self.playback_amp.set_speed(speed)
        self.echo.set_speed(speed)

    def process(self,
                mic_in: np.ndarray | None = None,
                phono_in: np.ndarray | None = None,
                radio_in: np.ndarray | None = None) -> np.ndarray:
        """Processa en buffer genom hela kedjan.

        Minst en input måste vara non-None.
        """
        cfg = self.config

        # 1. Input-preamps (parallellt)
        mic_pre = self.mic.process(mic_in) if mic_in is not None else None
        phono_pre = self.phono.process(phono_in) if phono_in is not None else None
        radio_pre = self.radio.process(radio_in) if radio_in is not None else None

        # 2. 3-buss-mixer
        mixed = self.mixer.process(mic=mic_pre, phono=phono_pre, radio=radio_pre)

        # 3. Bypass-tape-mode (knap 23 — amp-alone)
        if cfg.bypass_tape:
            return self._dc_block(self.power_amp.process(mixed))

        # 4. Echo-injektion innan record-amp (om aktiv)
        if cfg.echo_enabled and cfg.echo_amount > 1e-6:
            recorded_input = self.echo.process(mixed)
        else:
            recorded_input = mixed

        # 5. Record-amp (pre-emphasis + 3 stages)
        recorded = self.record_amp.process(recorded_input)

        # 6. Tape (B-H-hysteres + 100 kHz bias + hastighetsfilter)
        on_tape = self.tape.process(recorded)

        # 7. Multiplay (bounce-loss om aktiv)
        if cfg.multiplay_enabled:
            on_tape = self.multiplay.process(on_tape)

        # 8. Wow & flutter (mekanisk modulation)
        modulated = self.wow_flutter.process(on_tape)

        # 9. Playback-amp (3 stages + de-emphasis)
        playback = self.playback_amp.process(modulated)

        # 10. Monitor-switch
        # Synchroplay = monitor-tap byts från PLAY-head till REC-head
        # → effektivt: skip wow/flutter + playback-EQ-delay i monitor
        if cfg.synchroplay:
            monitored = recorded  # tap från record-head
        elif cfg.monitor == MonitorMode.SOURCE:
            monitored = mixed
        else:
            monitored = playback

        # 11. Tone-kontroll EFTER playback-amp (per manual s.5)
        toned = self.tone.process(monitored)

        # 12. Power-amp (eller bypass)
        out = self.power_amp.process(toned)
        # 13. DC-block på output (motsvarar final cap-coupling till line-out)
        return self._dc_block(out)


class SignalChain:
    """Stereo BC2000DL signal-chain.

    Två SignalChainMono med per-kanal-asymmetri (autentisk 1968-tolerans).
    """

    def __init__(self, config: ChainConfig | None = None):
        self.config = config or ChainConfig()
        asym = self.config.asymmetry
        self.left  = SignalChainMono(self.config, channel_offset=+asym, noise_seed=1000)
        self.right = SignalChainMono(self.config, channel_offset=-asym, noise_seed=2000)
        # Balance + master volume (sista stage före DAW out)
        self.balance_master = BalanceMaster(
            balance=self.config.balance,
            master=self.config.master_volume)

    def reset(self):
        self.left.reset()
        self.right.reset()

    def set_speed(self, speed: TapeSpeed):
        self.config.speed = speed
        self.left.set_speed(speed)
        self.right.set_speed(speed)

    def process_stereo(self,
                       mic_lr: tuple[np.ndarray | None, np.ndarray | None] = (None, None),
                       phono_lr: tuple[np.ndarray | None, np.ndarray | None] = (None, None),
                       radio_lr: tuple[np.ndarray | None, np.ndarray | None] = (None, None)
                       ) -> tuple[np.ndarray, np.ndarray]:
        """Processa stereo-input → stereo-output."""
        l = self.left.process(
            mic_in=mic_lr[0], phono_in=phono_lr[0], radio_in=radio_lr[0])
        r = self.right.process(
            mic_in=mic_lr[1], phono_in=phono_lr[1], radio_in=radio_lr[1])
        # Balance + master volume som sista stage
        l, r = self.balance_master.process(l, r)
        return l, r

    def process_mono_to_stereo(self, x: np.ndarray, *,
                               input_bus: str = "mic"
                               ) -> tuple[np.ndarray, np.ndarray]:
        """Processa mono-signal genom vald input-buss → stereo-output."""
        if input_bus == "mic":
            return self.process_stereo(mic_lr=(x, x))
        elif input_bus == "phono":
            return self.process_stereo(phono_lr=(x, x))
        elif input_bus == "radio":
            return self.process_stereo(radio_lr=(x, x))
        else:
            raise ValueError(f"Unknown bus: {input_bus}")
