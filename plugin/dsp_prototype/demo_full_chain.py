"""Demo: full BC2000DL signal-chain på en testsignal.

Processar en sammansatt signal (sinus-multitone + impulser) genom 3 olika
hastigheter och sparar WAV per scenario. Visar pluginens karaktär över
hela kedjan input → mixer → record → tape → playback → out.

Output i output/demo_full/:
- 01_dry.wav             — input-signal
- 02_19cms_lineout.wav   — 19 cm/s, line-out
- 03_19cms_speaker.wav   — 19 cm/s, speaker-mode (med power-amp)
- 04_95cms_lineout.wav   — 9.5 cm/s
- 05_475cms_lineout.wav  — 4.75 cm/s
- 06_3bus_mix.wav        — alla 3 bussar mixade samtidigt
- 07_bypass_tape.wav     — bypass-tape (amp-alone)
- spectrum.png           — spektrum-jämförelse
"""
from pathlib import Path
import numpy as np
import matplotlib.pyplot as plt
import soundfile as sf

from chain import SignalChain, ChainConfig
from eq_din1962 import TapeSpeed


SR = 48_000
OUT_DIR = Path(__file__).parent / "output" / "demo_full"
OUT_DIR.mkdir(parents=True, exist_ok=True)


def make_test_signal(dur_s: float = 5.0) -> np.ndarray:
    """Sammansatt signal:
    - 0-1.5 s: sinus-sweep 100 Hz → 8 kHz
    - 1.5-3 s: 1 kHz-impulser (testar transient-response)
    - 3-5 s: multitone (200 + 800 + 3000 + 8000 Hz)
    """
    t = np.arange(int(SR * dur_s)) / SR
    sig = np.zeros_like(t)

    # 0-1.5 s: log-sweep
    n_sweep = int(SR * 1.5)
    f0, f1 = 100.0, 8000.0
    t_sweep = np.arange(n_sweep) / SR
    phase = 2 * np.pi * f0 * 1.5 * (np.exp(t_sweep / 1.5 * np.log(f1 / f0)) - 1) / np.log(f1 / f0)
    sig[:n_sweep] = 0.3 * np.sin(phase)

    # 1.5-3 s: impulser
    impulse_start = int(SR * 1.5)
    impulse_end = int(SR * 3.0)
    for i in range(impulse_start, impulse_end, SR // 4):  # 4 impulser
        sig[i:i+200] = 0.5 * np.exp(-np.arange(200) / 50.0)

    # 3-5 s: multitone
    multi_start = int(SR * 3.0)
    multi_end = int(SR * 5.0)
    t_multi = np.arange(multi_end - multi_start) / SR
    sig[multi_start:multi_end] = 0.1 * (
        np.sin(2 * np.pi * 200 * t_multi) +
        np.sin(2 * np.pi * 800 * t_multi) +
        np.sin(2 * np.pi * 3000 * t_multi) +
        np.sin(2 * np.pi * 8000 * t_multi))

    # Mjuk fade-in/out
    fade = 100
    sig[:fade] *= np.linspace(0, 1, fade)
    sig[-fade:] *= np.linspace(1, 0, fade)
    return sig


def save_wav(path: Path, l: np.ndarray, r: np.ndarray, peak_db: float = -1.0):
    """Spara stereo WAV, normaliserat till peak_db."""
    stereo = np.stack([l, r], axis=-1)
    peak = np.max(np.abs(stereo))
    target = 10 ** (peak_db / 20)
    if peak > 0:
        stereo = stereo * (target / peak)
    sf.write(str(path), stereo, SR, subtype="PCM_24")


def rms(x):
    return float(np.sqrt(np.mean(x ** 2)))


def render_spectrum(scenarios: dict, out_path: Path):
    """Plot spektrum för utvalda scenarier."""
    fig, ax = plt.subplots(figsize=(11, 5))
    fig.patch.set_facecolor("#F4EFE4")
    ax.set_facecolor("#F4EFE4")
    colors = ["#1A1A1A", "#C42820", "#7A4722", "#9A9A9A", "#2C2C2C"]

    for (name, sig), color in zip(scenarios.items(), colors):
        # Använd sista 1 s (där multitone körs)
        seg = sig[-SR:]
        spec = np.abs(np.fft.rfft(seg * np.hanning(SR)))
        freqs = np.fft.rfftfreq(SR, 1.0 / SR)
        spec_db = 20 * np.log10(spec / np.max(spec) + 1e-12)
        ax.semilogx(freqs, spec_db, label=name, color=color, linewidth=1.2,
                    alpha=0.85)

    ax.set_xlim(20, 20_000)
    ax.set_ylim(-90, 5)
    ax.set_xlabel("FREKVENS  [Hz]", fontsize=10, fontweight="bold")
    ax.set_ylabel("AMPLITUD  [dB]", fontsize=10, fontweight="bold")
    ax.set_title(
        "BC2000DL FULL CHAIN  ·  HASTIGHETSJÄMFÖRELSE @ MULTITONE 200/800/3k/8k Hz",
        fontsize=11, fontweight="bold", color="#1A1A1A", loc="left")
    ax.grid(True, which="both", color="#C8C0B0", linewidth=0.4, alpha=0.6)
    leg = ax.legend(loc="lower left", frameon=False, fontsize=8.5)
    for t in leg.get_texts():
        t.set_color("#1A1A1A")
    for s in ["top", "right"]:
        ax.spines[s].set_visible(False)
    plt.tight_layout()
    plt.savefig(out_path, dpi=150, facecolor="#F4EFE4")
    plt.close()


def main():
    print("=" * 60)
    print("BC2000DL  ·  FULL CHAIN DEMO")
    print("=" * 60)

    sig = make_test_signal(5.0)

    save_wav(OUT_DIR / "01_dry.wav", sig, sig)
    print(f"  01 dry              RMS: {rms(sig):.4f}")

    scenarios = {}
    scenarios["DRY"] = sig

    # 02 — 19 cm/s, line-out (default monitor=tape)
    cfg = ChainConfig(mic_gain=0.5, speed=TapeSpeed.SPEED_19)
    chain = SignalChain(cfg)
    l, r = chain.process_mono_to_stereo(sig, input_bus="mic")
    save_wav(OUT_DIR / "02_19cms_lineout.wav", l, r)
    print(f"  02 19 cm/s line     RMS: {rms(l):.4f}")
    scenarios["19 cm/s · line-out"] = (l + r) / 2

    # 03 — 19 cm/s, speaker-mode
    cfg = ChainConfig(mic_gain=0.5, speed=TapeSpeed.SPEED_19, speaker_monitor=True)
    chain = SignalChain(cfg)
    l, r = chain.process_mono_to_stereo(sig, input_bus="mic")
    save_wav(OUT_DIR / "03_19cms_speaker.wav", l, r)
    print(f"  03 19 cm/s speaker  RMS: {rms(l):.4f}")

    # 04 — 9.5 cm/s
    cfg = ChainConfig(mic_gain=0.5, speed=TapeSpeed.SPEED_9_5)
    chain = SignalChain(cfg)
    l, r = chain.process_mono_to_stereo(sig, input_bus="mic")
    save_wav(OUT_DIR / "04_95cms_lineout.wav", l, r)
    print(f"  04 9.5 cm/s line    RMS: {rms(l):.4f}")
    scenarios["9.5 cm/s · line-out"] = (l + r) / 2

    # 05 — 4.75 cm/s
    cfg = ChainConfig(mic_gain=0.5, speed=TapeSpeed.SPEED_4_75)
    chain = SignalChain(cfg)
    l, r = chain.process_mono_to_stereo(sig, input_bus="mic")
    save_wav(OUT_DIR / "05_475cms_lineout.wav", l, r)
    print(f"  05 4.75 cm/s line   RMS: {rms(l):.4f}")
    scenarios["4.75 cm/s · line-out"] = (l + r) / 2

    # 06 — 3-buss mixer (alla samtidigt)
    cfg = ChainConfig(mic_gain=0.3, phono_gain=0.3, radio_gain=0.3,
                      speed=TapeSpeed.SPEED_19)
    chain = SignalChain(cfg)
    l, r = chain.process_stereo(
        mic_lr=(sig, sig),
        phono_lr=(sig * 0.5, sig * 0.5),
        radio_lr=(sig * 0.7, sig * 0.7))
    save_wav(OUT_DIR / "06_3bus_mix.wav", l, r)
    print(f"  06 3-buss mix       RMS: {rms(l):.4f}")
    scenarios["3-buss mixerpult"] = (l + r) / 2

    # 07 — bypass tape (amp-alone)
    cfg = ChainConfig(mic_gain=0.5, bypass_tape=True, speaker_monitor=True)
    chain = SignalChain(cfg)
    l, r = chain.process_mono_to_stereo(sig, input_bus="mic")
    save_wav(OUT_DIR / "07_bypass_tape.wav", l, r)
    print(f"  07 bypass tape      RMS: {rms(l):.4f}")
    scenarios["BYPASS-TAPE"] = (l + r) / 2

    # 08 — Echo @ 19 cm/s (slap-back ~75 ms)
    cfg = ChainConfig(mic_gain=0.5, speed=TapeSpeed.SPEED_19,
                      echo_enabled=True, echo_amount=0.5)
    chain = SignalChain(cfg)
    l, r = chain.process_mono_to_stereo(sig, input_bus="mic")
    save_wav(OUT_DIR / "08_echo_19cms_slapback.wav", l, r)
    print(f"  08 echo 19 cm/s     RMS: {rms(l):.4f}  (75 ms slap-back)")

    # 09 — Echo @ 4.75 cm/s (long delay ~300 ms)
    cfg = ChainConfig(mic_gain=0.5, speed=TapeSpeed.SPEED_4_75,
                      echo_enabled=True, echo_amount=0.6)
    chain = SignalChain(cfg)
    l, r = chain.process_mono_to_stereo(sig, input_bus="mic")
    save_wav(OUT_DIR / "09_echo_475cms_long.wav", l, r)
    print(f"  09 echo 4.75 cm/s   RMS: {rms(l):.4f}  (300 ms long delay)")

    # 10 — Echo self-osc-zon (varning från manualen)
    cfg = ChainConfig(mic_gain=0.4, speed=TapeSpeed.SPEED_19,
                      echo_enabled=True, echo_amount=0.92)
    chain = SignalChain(cfg)
    l, r = chain.process_mono_to_stereo(sig, input_bus="mic")
    save_wav(OUT_DIR / "10_echo_self_osc_zone.wav", l, r)
    print(f"  10 echo self-osc    RMS: {rms(l):.4f}  ('drejes for meget op')")

    # 11 — Multiplay 3:e generationen (manualens varning om brus + HF-loss)
    cfg = ChainConfig(mic_gain=0.5, speed=TapeSpeed.SPEED_19,
                      multiplay_enabled=True, multiplay_generation=3)
    chain = SignalChain(cfg)
    l, r = chain.process_mono_to_stereo(sig, input_bus="mic")
    save_wav(OUT_DIR / "11_multiplay_gen3.wav", l, r)
    print(f"  11 multiplay gen 3  RMS: {rms(l):.4f}  (kumulerad brus + HF-loss)")

    # 12 — Tone control extremes (bass +12, treble -12 = lo-fi vintage)
    cfg = ChainConfig(mic_gain=0.5, speed=TapeSpeed.SPEED_9_5,
                      bass_db=+8.0, treble_db=-8.0)
    chain = SignalChain(cfg)
    l, r = chain.process_mono_to_stereo(sig, input_bus="mic")
    save_wav(OUT_DIR / "12_lo_fi_tone.wav", l, r)
    print(f"  12 lo-fi tone       RMS: {rms(l):.4f}  (bass +8, treble -8)")

    # Spektrum-plot
    print("\n→ Renderar spektrum-jämförelse...")
    render_spectrum(scenarios, OUT_DIR / "spectrum.png")

    print("\n" + "=" * 60)
    print(f"KLAR. Output i: {OUT_DIR}")
    print(f"  Spektrum:  {OUT_DIR / 'spectrum.png'}")
    print(f"  WAVs:      7 st @ 24-bit, 48 kHz, stereo")
    print("=" * 60)


if __name__ == "__main__":
    main()
