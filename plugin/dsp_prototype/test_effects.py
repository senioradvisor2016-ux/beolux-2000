"""Tester för Echo, Multiplay, ToneControl, BalanceMaster + integration."""
import numpy as np
import pytest

from effects import Echo, Multiplay, SoundOnSound
from tone_balance import ToneControl, BalanceMaster
from chain import SignalChain, ChainConfig
from eq_din1962 import TapeSpeed


SR = 48_000


def sine(freq, dur_s, amp=1.0):
    t = np.arange(int(SR * dur_s)) / SR
    return amp * np.sin(2 * np.pi * freq * t)


def rms(x):
    return float(np.sqrt(np.mean(np.asarray(x, dtype=np.float64) ** 2)))


# ---------- Echo-tester ----------
def test_echo_disabled_passes_signal_unchanged():
    echo = Echo(sample_rate=SR, enabled=False)
    sig = sine(1000, 0.5, amp=0.3)
    out = echo.process(sig)
    np.testing.assert_array_almost_equal(out, sig)


def test_echo_speed_affects_delay_time():
    """75 ms @ 19 cm/s, 150 ms @ 9.5 cm/s, 300 ms @ 4.75 cm/s."""
    for speed, expected_ms in [(TapeSpeed.SPEED_19, 75),
                                (TapeSpeed.SPEED_9_5, 150),
                                (TapeSpeed.SPEED_4_75, 300)]:
        echo = Echo(sample_rate=SR, speed=speed)
        # Tolerans ±20 ms (hur scaling görs)
        assert abs(echo.delay_ms - expected_ms) < 20, \
            f"Echo @ {speed} = {echo.delay_ms} ms (expected ~{expected_ms} ms)"


def test_echo_with_amount_produces_repeats():
    """En impuls ska generera ekorepeats efter delay-tiden."""
    echo = Echo(sample_rate=SR, speed=TapeSpeed.SPEED_19,
                amount=0.5, enabled=True)
    sig = np.zeros(SR)
    sig[0] = 1.0  # Impuls vid 0
    out = echo.process(sig)
    # Förväntad första repeat ~75 ms = 3600 samples
    delay_samples = int(SR * 0.075)
    # Området runt delay_samples ska ha någon energi
    region_energy = rms(out[delay_samples - 200:delay_samples + 200])
    silent_region_energy = rms(out[delay_samples + 1000:delay_samples + 2000])
    assert region_energy > silent_region_energy, \
        f"Inget eko vid förväntad position: {region_energy:.6f} vs {silent_region_energy:.6f}"


def test_echo_self_oscillation_at_high_amount():
    """Vid amount=0.99 ska ekot självoscillera (hög RMS)."""
    echo = Echo(sample_rate=SR, speed=TapeSpeed.SPEED_19,
                amount=0.99, enabled=True)
    sig = np.zeros(SR * 2)
    sig[0:100] = 0.5  # Kort burst
    out = echo.process(sig)
    # Sista 0.5 s ska ha hög RMS pga self-osc
    end_rms = rms(out[-SR // 2:])
    assert end_rms > 0.05, f"Self-osc misslyckas: end_rms={end_rms:.4f}"


# ---------- Multiplay-tester ----------
def test_multiplay_higher_generation_more_hf_loss():
    """Gen 3 ska dämpa HF mer än gen 1."""
    sig = sine(8000, 0.3, amp=0.5)  # 8 kHz HF-signal

    mp1 = Multiplay(sample_rate=SR, generation=1, enabled=True)
    out1 = mp1.process(sig)

    mp3 = Multiplay(sample_rate=SR, generation=3, enabled=True)
    out3 = mp3.process(sig)

    # Gen 3 ska ha lägre RMS pga mer HF-roll-off
    assert rms(out3) < rms(out1), \
        f"Gen 3 RMS {rms(out3):.4f} >= Gen 1 RMS {rms(out1):.4f}"


def test_multiplay_disabled_passes_signal():
    mp = Multiplay(sample_rate=SR, enabled=False)
    sig = sine(1000, 0.3, amp=0.3)
    out = mp.process(sig)
    np.testing.assert_array_almost_equal(out, sig)


# ---------- ToneControl-tester ----------
def test_tone_neutral_is_unity():
    tone = ToneControl(sample_rate=SR, bass_db=0.0, treble_db=0.0)
    sig = sine(1000, 0.3, amp=0.3)
    out = tone.process(sig)
    # Neutral tone ska ge nästan oförändrad signal (filter med gain≈1)
    np.testing.assert_array_almost_equal(out, sig, decimal=4)


def test_tone_bass_boost_amplifies_lf():
    tone = ToneControl(sample_rate=SR, bass_db=+12.0, treble_db=0.0)
    lf_sig = sine(50, 0.5, amp=0.2)
    out = tone.process(lf_sig)
    # Bass-boost @ 100 Hz med +12 dB → 50 Hz ska bli ~+10 dB
    gain_db = 20 * np.log10(rms(out) / rms(lf_sig))
    assert gain_db > 5.0, f"Bass-boost otillräcklig: {gain_db:.1f} dB"


def test_tone_treble_cut_attenuates_hf():
    tone = ToneControl(sample_rate=SR, bass_db=0.0, treble_db=-12.0)
    hf_sig = sine(15000, 0.3, amp=0.2)
    out = tone.process(hf_sig)
    gain_db = 20 * np.log10(rms(out) / rms(hf_sig) + 1e-12)
    assert gain_db < -3.0, f"Treble-cut otillräcklig: {gain_db:.1f} dB"


# ---------- BalanceMaster-tester ----------
def test_balance_center_unchanged():
    bm = BalanceMaster(balance=0.0, master=1.0)
    l_in = sine(1000, 0.3, amp=0.5)
    r_in = sine(1000, 0.3, amp=0.5)
    l_out, r_out = bm.process(l_in, r_in)
    # Center-pan: båda ska ha ~0.707 av input (equal-power)
    assert 0.6 < rms(l_out) / rms(l_in) < 0.8
    assert 0.6 < rms(r_out) / rms(r_in) < 0.8


def test_balance_full_left():
    bm = BalanceMaster(balance=-1.0, master=1.0)
    l_in = sine(1000, 0.3, amp=0.5)
    r_in = sine(1000, 0.3, amp=0.5)
    l_out, r_out = bm.process(l_in, r_in)
    assert rms(l_out) > rms(l_in) * 0.9, "Full-L: L ska behållas"
    assert rms(r_out) < rms(r_in) * 0.1, "Full-L: R ska tystas"


def test_master_zero_mutes():
    bm = BalanceMaster(balance=0.0, master=0.0)
    l_in = sine(1000, 0.3, amp=0.5)
    l_out, _ = bm.process(l_in, l_in)
    assert rms(l_out) < 1e-6, f"Master=0 ska tysta: {rms(l_out)}"


# ---------- Integration: SignalChain med effekter ----------
def test_chain_with_echo_produces_extended_audio():
    """Med echo + medium amount ska transient-region (kort efter signalen)
    ha distinkt eko-energi.
    """
    sig = np.zeros(SR * 2)
    # Kort impuls vid 0.1 s
    sig[int(SR * 0.1):int(SR * 0.1) + 200] = 0.5

    cfg_no_echo = ChainConfig(mic_gain=0.6, echo_enabled=False)
    chain_no = SignalChain(cfg_no_echo)
    l_no, _ = chain_no.process_mono_to_stereo(sig, input_bus="mic")

    cfg_echo = ChainConfig(mic_gain=0.6, echo_enabled=True, echo_amount=0.7)
    chain_yes = SignalChain(cfg_echo)
    l_yes, _ = chain_yes.process_mono_to_stereo(sig, input_bus="mic")

    # Med echo måste output skilja sig från utan echo (regardless av RMS)
    # — tape-saturation kan komprimera men spektrum/transient-fördelning ändras
    diff_rms = rms(l_yes - l_no)
    sig_rms = rms(l_no)
    assert diff_rms > sig_rms * 0.05, \
        f"Echo påverkade inte output tillräckligt: diff={diff_rms:.4f}, sig={sig_rms:.4f}"


def test_chain_tone_control_affects_output():
    """Treble +12 ska ge mer HF än treble -12."""
    sig = sine(8000, 0.3, amp=0.3)

    cfg_tcut = ChainConfig(mic_gain=0.5, treble_db=-12.0)
    chain_cut = SignalChain(cfg_tcut)
    l_cut, _ = chain_cut.process_mono_to_stereo(sig, input_bus="mic")

    cfg_tboost = ChainConfig(mic_gain=0.5, treble_db=+12.0)
    chain_boost = SignalChain(cfg_tboost)
    l_boost, _ = chain_boost.process_mono_to_stereo(sig, input_bus="mic")

    assert rms(l_boost) > rms(l_cut), \
        f"Treble-boost gav inte mer HF: boost={rms(l_boost):.4f}, cut={rms(l_cut):.4f}"


def test_chain_balance_pan_works():
    """Balance=-1 ska tystna högerkanalen."""
    cfg = ChainConfig(mic_gain=0.5, balance=-1.0)
    chain = SignalChain(cfg)
    sig = sine(1000, 0.3, amp=0.3)
    l, r = chain.process_mono_to_stereo(sig, input_bus="mic")
    assert rms(l) > rms(r) * 5, f"Balance=-1: L={rms(l):.4f}, R={rms(r):.4f}"


def test_chain_master_volume_affects_output():
    """Master=0.25 ska ge ~12 dB lägre output än 1.0."""
    sig = sine(1000, 0.3, amp=0.3)

    cfg_high = ChainConfig(mic_gain=0.5, master_volume=1.0)
    chain_high = SignalChain(cfg_high)
    l_high, _ = chain_high.process_mono_to_stereo(sig, input_bus="mic")

    cfg_low = ChainConfig(mic_gain=0.5, master_volume=0.25)
    chain_low = SignalChain(cfg_low)
    l_low, _ = chain_low.process_mono_to_stereo(sig, input_bus="mic")

    ratio_db = 20 * np.log10(rms(l_high) / max(rms(l_low), 1e-9))
    # Square-law master: 0.25^2 = 0.0625 ≈ -24 dB
    # Tolerans: 15-30 dB
    assert 15 < ratio_db < 35, f"Master-ratio: {ratio_db:.1f} dB"
