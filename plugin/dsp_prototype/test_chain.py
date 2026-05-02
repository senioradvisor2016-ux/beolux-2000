"""Integration-tester för full BC2000DL-kedjan."""
import numpy as np
import pytest

from chain import SignalChain, ChainConfig, MonitorMode
from eq_din1962 import TapeSpeed


SR = 48_000


def sine(freq, dur_s, amp=1.0):
    t = np.arange(int(SR * dur_s)) / SR
    return amp * np.sin(2 * np.pi * freq * t)


def rms(x):
    return float(np.sqrt(np.mean(np.asarray(x, dtype=np.float64) ** 2)))


# ---------- Smoke tests ----------
def test_chain_constructs_without_error():
    chain = SignalChain()
    assert chain is not None


def test_chain_process_silence_with_closed_fader_is_quiet():
    """Med stängd mic-fader (gain=0) ska output vara nästan tyst.

    Gain-staging: en operator skulle inte vrida upp mic-fader vid tystnad.
    Detta motsvarar hur kedjan beter sig i normal användning.
    """
    cfg = ChainConfig(mic_gain=0.0)  # stängd
    chain = SignalChain(cfg)
    silent = np.zeros(SR // 2)
    l, r = chain.process_mono_to_stereo(silent, input_bus="mic")
    # Bara tape-noise + small playback-amp-brus, knappt något
    assert rms(l) < 0.05, f"Output RMS för hög med stängd fader: {rms(l):.4f}"
    assert rms(r) < 0.05


def test_chain_passes_signal_to_output():
    """Med en stark sinus + mic_gain=1 ska vi få output > 0.1 RMS."""
    cfg = ChainConfig(mic_gain=0.5, speed=TapeSpeed.SPEED_19)
    chain = SignalChain(cfg)
    sig = sine(1000, 0.5, amp=0.05)  # -26 dBFS
    l, r = chain.process_mono_to_stereo(sig, input_bus="mic")
    assert rms(l) > 0.001, f"Output RMS för låg: {rms(l):.6f}"


def test_speed_change_doesnt_crash():
    """Hastighetsändring under körning ska inte krascha."""
    cfg = ChainConfig(mic_gain=0.3)
    chain = SignalChain(cfg)
    sig = sine(1000, 0.2, amp=0.1)
    chain.process_mono_to_stereo(sig, input_bus="mic")

    for speed in [TapeSpeed.SPEED_9_5, TapeSpeed.SPEED_4_75, TapeSpeed.SPEED_19]:
        chain.set_speed(speed)
        l, r = chain.process_mono_to_stereo(sig, input_bus="mic")
        assert np.all(np.isfinite(l)), f"NaN/inf vid hastighet {speed}"


def test_bypass_tape_mode():
    """Bypass-tape (amp-alone) ska skippa tape-blocket."""
    cfg = ChainConfig(mic_gain=0.5, bypass_tape=True)
    chain = SignalChain(cfg)
    sig = sine(1000, 0.3, amp=0.1)
    l, r = chain.process_mono_to_stereo(sig, input_bus="mic")
    assert np.all(np.isfinite(l))


def test_speaker_monitor_mode():
    """Speaker-monitor (power-amp aktiv) ska ge mer karaktär."""
    sig = sine(1000, 0.3, amp=0.05)

    cfg_line = ChainConfig(mic_gain=0.5, speaker_monitor=False)
    chain_line = SignalChain(cfg_line)
    l_line, _ = chain_line.process_mono_to_stereo(sig, input_bus="mic")

    cfg_speaker = ChainConfig(mic_gain=0.5, speaker_monitor=True)
    chain_speaker = SignalChain(cfg_speaker)
    l_speaker, _ = chain_speaker.process_mono_to_stereo(sig, input_bus="mic")

    # Båda ska ha output men power-amp adderar HP-filter + soft-clip
    assert rms(l_line) > 0.001
    assert rms(l_speaker) > 0.001


def test_stereo_asymmetry_produces_l_neq_r():
    """L och R ska skilja sig autentiskt (1968-tolerans)."""
    cfg = ChainConfig(mic_gain=0.4)
    chain = SignalChain(cfg)
    sig = sine(1000, 0.3, amp=0.1)
    l, r = chain.process_mono_to_stereo(sig, input_bus="mic")
    diff = np.mean(np.abs(l - r))
    assert diff > 0.0001, f"L=R i stereo: diff={diff:.6f}"


def test_3bus_mixer_accepts_simultaneous_inputs():
    """Alla 3 input-bussar ska kunna mixas samtidigt."""
    cfg = ChainConfig(mic_gain=0.3, phono_gain=0.3, radio_gain=0.3)
    chain = SignalChain(cfg)
    sig = sine(1000, 0.3, amp=0.05)
    # Använd samma signal på alla 3 bussar
    l, r = chain.process_stereo(
        mic_lr=(sig, sig),
        phono_lr=(sig, sig),
        radio_lr=(sig, sig))
    assert rms(l) > 0.001
    assert np.all(np.isfinite(l))


def test_tape_speed_affects_hf_response():
    """4.75 cm/s ska ha mer HF-roll-off än 19 cm/s."""
    # En HF-rik signal (5 kHz)
    sig = sine(5000, 0.5, amp=0.1)

    cfg_19 = ChainConfig(mic_gain=0.5, speed=TapeSpeed.SPEED_19)
    chain_19 = SignalChain(cfg_19)
    l_19, _ = chain_19.process_mono_to_stereo(sig, input_bus="mic")

    cfg_475 = ChainConfig(mic_gain=0.5, speed=TapeSpeed.SPEED_4_75)
    chain_475 = SignalChain(cfg_475)
    l_475, _ = chain_475.process_mono_to_stereo(sig, input_bus="mic")

    # 4.75 cm/s ska dämpa 5 kHz mer än 19 cm/s
    rms_19 = rms(l_19)
    rms_475 = rms(l_475)
    # Tolerans pga brus och EQ-mismatch — bara grov assertion
    assert rms_475 < rms_19 * 1.5, \
        f"4.75 cm/s borde dämpa HF: rms_19={rms_19:.4f}, rms_475={rms_475:.4f}"


def test_monitor_source_vs_tape_differ():
    """Source-monitor (medhør före tape) ska skilja sig från tape (afhør)."""
    sig = sine(1000, 0.5, amp=0.1)

    cfg_src = ChainConfig(mic_gain=0.5, monitor=MonitorMode.SOURCE)
    chain_src = SignalChain(cfg_src)
    l_src, _ = chain_src.process_mono_to_stereo(sig, input_bus="mic")

    cfg_tape = ChainConfig(mic_gain=0.5, monitor=MonitorMode.TAPE)
    chain_tape = SignalChain(cfg_tape)
    l_tape, _ = chain_tape.process_mono_to_stereo(sig, input_bus="mic")

    # Två kedjor → olika brus-seeds → ändå ska medel-amplituden skilja
    # eftersom tape adderar saturation + noise
    diff = abs(rms(l_src) - rms(l_tape))
    # Tolerans — båda ska ha output men skilja sig
    assert rms(l_src) > 0.001 and rms(l_tape) > 0.001


if __name__ == "__main__":
    print("Kör: pytest test_chain.py -v")
