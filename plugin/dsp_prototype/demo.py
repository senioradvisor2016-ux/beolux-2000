"""Demo: kör en sinus + speech-liknande signal genom byggstenarna.

Producerar:
- demo_dry.wav         — original-signal
- demo_2n2613.wav      — efter en Ge2N2613Stage (gain 20 dB)
- demo_uw0029_2n2613.wav — efter UW0029 + 2N2613 cascade (mic-preamp simulering)
- demo_spectrum.png    — frekvensspektrum-jämförelse

Kör:
    python3 demo.py
"""
from pathlib import Path
import numpy as np
import matplotlib.pyplot as plt
import soundfile as sf

from ge_stages import (
    Ge2N2613Stage,
    GeLowNoiseStage,
    GeStageType,
)

SR = 48_000
OUT_DIR = Path(__file__).parent / "output"
OUT_DIR.mkdir(exist_ok=True)


def make_test_signal(duration_s: float = 4.0) -> np.ndarray:
    """Sinus 1 kHz med varierande amplitud — testar både linjär och soft-clip-zon."""
    t = np.arange(int(SR * duration_s)) / SR
    # Tre amplitudsteg: -20 dBFS, -6 dBFS, -3 dBFS
    sig = np.zeros_like(t)
    seg_len = len(t) // 3

    sig[:seg_len]            = 10**(-20/20) * np.sin(2 * np.pi * 1000 * t[:seg_len])
    sig[seg_len:2*seg_len]   = 10**(-6/20)  * np.sin(2 * np.pi * 1000 * t[seg_len:2*seg_len])
    sig[2*seg_len:]          = 10**(-3/20)  * np.sin(2 * np.pi * 1000 * t[2*seg_len:])

    # Mjuk crossfade mellan segment för att undvika klick
    fade_len = 480  # 10 ms
    for boundary in [seg_len, 2*seg_len]:
        ramp = np.linspace(0, 1, fade_len)
        sig[boundary:boundary+fade_len] *= ramp
        sig[boundary-fade_len:boundary] *= (1 - ramp)
    return sig


def render_spectrum_plot(signals: dict, sample_rate: int, out_path: Path):
    """Plot frekvensspektrum för alla signaler i samma figur."""
    fig, ax = plt.subplots(figsize=(11, 5))
    fig.patch.set_facecolor("#F4EFE4")
    ax.set_facecolor("#F4EFE4")

    colors = ["#1A1A1A", "#C42820", "#9A9A9A"]

    for (name, sig), color in zip(signals.items(), colors):
        # Sista 0.5 s (där amplituden är -3 dBFS)
        n = sample_rate // 2
        x = sig[-n:]
        spec = np.abs(np.fft.rfft(x * np.hanning(n)))
        freqs = np.fft.rfftfreq(n, 1.0 / sample_rate)
        spec_db = 20 * np.log10(spec / np.max(spec) + 1e-12)
        ax.semilogx(freqs, spec_db, label=name, color=color, linewidth=1.4)

    ax.set_xlim(20, 20_000)
    ax.set_ylim(-100, 5)
    ax.set_xlabel("FREKVENS  [Hz]", fontsize=10, fontweight="bold")
    ax.set_ylabel("AMPLITUD  [dB]", fontsize=10, fontweight="bold")
    ax.set_title("BC2000DL — DSP-PROTOTYP  ·  HARMONICS-SPEKTRUM @ -3 dBFS, 1 kHz",
                 fontsize=11, fontweight="bold", color="#1A1A1A", loc="left")
    ax.grid(True, which="both", color="#C8C0B0", linewidth=0.4, alpha=0.6)
    leg = ax.legend(loc="upper right", frameon=False, fontsize=9)
    for t in leg.get_texts():
        t.set_color("#1A1A1A")

    for spine in ["top", "right"]:
        ax.spines[spine].set_visible(False)

    plt.tight_layout()
    plt.savefig(out_path, dpi=150, facecolor="#F4EFE4")
    plt.close()


def thd_pct(x, freq, sr=SR, n_harmonics=10):
    """Total Harmonic Distortion via FFT (procent)."""
    n = len(x)
    spec = np.abs(np.fft.rfft(x * np.hanning(n)))
    bin_hz = sr / n
    fund_bin = int(round(freq / bin_hz))
    fundamental = spec[fund_bin]
    harm_sum_sq = 0.0
    for h in range(2, n_harmonics + 1):
        b = fund_bin * h
        if b < len(spec):
            window = spec[max(0, b - 3):b + 4]
            harm_sum_sq += np.max(window) ** 2
    return 100.0 * np.sqrt(harm_sum_sq) / fundamental


def main():
    print("=" * 60)
    print("BC2000DL DSP-prototyp · Demo")
    print("=" * 60)

    # 1. Generera test-signal
    print("\n→ Genererar test-signal (4 s, 1 kHz, 3 amplitudsteg)")
    sig = make_test_signal(duration_s=4.0)

    # 2. Run-through 1: bara Ge2N2613Stage (gain 20 dB)
    print("\n→ Stage 1: Ge2N2613Stage (gain 20 dB)")
    stage_2n = Ge2N2613Stage(gain_db=20.0, sample_rate=SR, noise_seed=42)
    out_2n = stage_2n.process(sig)

    # 3. Run-through 2: cascade UW0029 → 2N2613 (motsvarar mic-preamp 8904004)
    print("\n→ Cascade: UW0029 (gain 30 dB) → 2N2613 (gain 20 dB) — mic-preamp")
    stage_uw = GeLowNoiseStage(GeStageType.UW0029, gain_db=30.0,
                                sample_rate=SR, noise_seed=10)
    stage_2n_after = Ge2N2613Stage(gain_db=20.0, sample_rate=SR, noise_seed=11)
    out_cascade = stage_2n_after.process(stage_uw.process(sig))

    # 4. Spara WAV-filer
    # Normalisera så vi inte klipper i 16-bit
    def save(path, x, max_db=-1.0):
        peak = np.max(np.abs(x))
        target_peak = 10 ** (max_db / 20)
        if peak > 0:
            x_norm = x * (target_peak / peak)
        else:
            x_norm = x
        sf.write(str(path), x_norm, SR, subtype="PCM_24")
        print(f"   Sparade: {path.name}  ({rms(x):.4f} RMS in, peak normalized to {max_db} dBFS)")

    def rms(x):
        return float(np.sqrt(np.mean(x ** 2)))

    print("\n→ Sparar WAV-filer")
    save(OUT_DIR / "demo_dry.wav", sig)
    save(OUT_DIR / "demo_2n2613.wav", out_2n)
    save(OUT_DIR / "demo_uw0029_2n2613.wav", out_cascade)

    # 5. Mätningar
    print("\n→ Mätningar (sista 0.5 s @ -3 dBFS):")
    n = SR // 2
    for name, x in [("DRY", sig), ("2N2613 only", out_2n), ("UW0029→2N2613", out_cascade)]:
        seg = x[-n:]
        thd = thd_pct(seg, 1000)
        rms_db = 20 * np.log10(rms(seg) + 1e-12)
        print(f"   {name:25s}  RMS: {rms_db:+6.1f} dBFS    THD: {thd:5.2f} %")

    # 6. Spektrum-plot
    print("\n→ Renderar frekvensspektrum (output/demo_spectrum.png)")
    render_spectrum_plot(
        {"DRY (input)": sig,
         "2N2613 (1 stage, 20 dB)": out_2n,
         "UW0029 → 2N2613 (mic-preamp)": out_cascade},
        SR,
        OUT_DIR / "demo_spectrum.png",
    )

    print("\n" + "=" * 60)
    print(f"KLAR. Output i: {OUT_DIR}")
    print("=" * 60)


if __name__ == "__main__":
    main()
