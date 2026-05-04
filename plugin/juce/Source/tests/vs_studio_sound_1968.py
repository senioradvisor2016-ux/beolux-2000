#!/usr/bin/env python3
"""
vs_studio_sound_1968.py — exakt spec-compliance-test mot plugin/specs.md
(härledd från Studio Sound 1968 + B&O servicemanual + 2N2613-databladet).

Varje rad i §2/§3/§4/§5/§6/§7/§8/§9/§10/§11 mäts mot ett konkret target
och printar PASS/FAIL med delta. Målet: alla rader gröna.

Kör:  python3 plugin/juce/Source/tests/vs_studio_sound_1968.py [/path/to/Beolux.vst3]
"""

import sys, math, time
import numpy as np
import pedalboard

PLUGIN_PATH = (sys.argv[1] if len(sys.argv) > 1
               else "/Users/senioradvisor/Library/Audio/Plug-Ins/VST3/Beolux 2000.vst3")
SR = 48000
BLOCK = 512

g_pass = 0
g_fail = 0
fails = []

# Ladda plugin EN gång — pedalboard vill inte ha för många instanser.
PLUGIN = pedalboard.load_plugin(PLUGIN_PATH)


def report(spec, ok, measured, target):
    global g_pass, g_fail
    tag = "✅ PASS" if ok else "❌ FAIL"
    print(f"  {tag}  {spec:<48}  measured={measured:<22} target={target}")
    if ok:
        g_pass += 1
    else:
        g_fail += 1
        fails.append(spec)


def configure(speed="9.5 cm/s", formula="Agfa", echo=False, echo_amt=0.0,
              wow=0.0):
    """Reset and configure plugin for next test.

    wow=0.0 by default — frequenzgangs-mätningar via dot-product-correlation
    vid en specifik frekvens fungerar inte korrekt om signalen är frekvens-
    modulerad (PM från wow/flutter sprider energi till sidband, så
    correlation vid carrier-frekvensen underskattar amplitude med upp till
    -12 dB vid β=3 enligt Bessel J0).  Real-tape-FR-mätningar kompenserar
    för detta med tracking-filter eller bredare bin-bandbredd; pedalboard
    har ingen sådan funktion.  Sätt wow=0 för spec-test, och testa wow
    separat i §10 (om implementerat).
    """
    PLUGIN.reset()
    PLUGIN.tape_formula = formula
    PLUGIN.tape_speed = speed
    PLUGIN.bypass_tape = False
    PLUGIN.echo_enabled = echo
    PLUGIN.monitor_mode = "Tape"
    if echo and hasattr(PLUGIN, 'echo_amount'):
        PLUGIN.echo_amount = echo_amt
    if hasattr(PLUGIN, 'master_volume'):
        PLUGIN.master_volume = 0.85
    if hasattr(PLUGIN, 'wow_flutter'):
        PLUGIN.wow_flutter = wow
    if hasattr(PLUGIN, 'print_through'):
        PLUGIN.print_through = 0.0


def process_blocks(signal):
    """Process signal in BLOCK chunks via the singleton PLUGIN."""
    out = np.zeros_like(signal)
    n = signal.shape[1]
    for off in range(0, n, BLOCK):
        end = min(off + BLOCK, n)
        out[:, off:end] = PLUGIN.process(signal[:, off:end], SR, reset=False)
    return out


def warm_then_measure(signal, warm_blocks=20):
    """Warm with same signal first to settle filters, then process for measurement."""
    n = signal.shape[1]
    warm_n = warm_blocks * BLOCK
    if warm_n > n:
        warm_n = n
    # Warm with first segment
    process_blocks(signal[:, :warm_n])
    # Now do the real measurement pass
    return process_blocks(signal)


def make_sine(freq, n_samples, amp_dbfs):
    amp = 10 ** (amp_dbfs / 20)
    t = np.arange(n_samples) / SR
    s = (amp * np.sin(2 * np.pi * freq * t)).astype(np.float32)
    return np.stack([s, s])


def make_silence(n_samples):
    return np.zeros((2, n_samples), dtype=np.float32)


def thd_percent(out_mono, fundamental):
    n = len(out_mono)
    skip = n // 2
    x = out_mono[skip:] * np.hanning(n - skip).astype(np.float32)
    n2 = len(x)
    fft = np.fft.rfft(x)
    fund_bin = int(np.round(fundamental * n2 / SR))
    fund_peak = np.max(np.abs(fft[max(1, fund_bin-5):fund_bin+5]))
    harm_sum_sq = 0.0
    for h in range(2, 11):
        hb = h * fund_bin
        if hb < len(fft) - 5:
            harm_sum_sq += np.max(np.abs(fft[hb-3:hb+3])) ** 2
    if fund_peak < 1e-12:
        return float('inf')
    return 100.0 * math.sqrt(harm_sum_sq) / fund_peak


def rms_dbfs(x):
    rms = math.sqrt(float(np.mean(x ** 2))) + 1e-30
    return 20 * math.log10(rms)


def freq_response_db(out_mono, freq):
    n = len(out_mono)
    skip = n // 2
    x = out_mono[skip:]
    n2 = len(x)
    t = np.arange(n2) / SR
    re = float(np.dot(x, np.cos(2*np.pi*freq*t)))
    im = float(np.dot(x, np.sin(2*np.pi*freq*t)))
    amp = 2 * math.sqrt(re**2 + im**2) / n2
    return 20 * math.log10(amp + 1e-30)


# ════════════════════════════════════════════════════════════════════

def test_bandwidth():
    print("\n── §2 Bandbredd per hastighet ──")
    targets = [
        ("19 cm/s",   "19 cm/s",   30,  20000, 3),
        ("9.5 cm/s",  "9.5 cm/s",  40,  12000, 3),
        ("4.75 cm/s", "4.75 cm/s", 50,  6000,  3),
    ]
    for label, speed, lf, hf, tol in targets:
        configure(speed=speed)
        ref = make_sine(1000, SR * 2, -20)
        ref_out = warm_then_measure(ref)
        ref_db = freq_response_db(ref_out[0], 1000)

        configure(speed=speed)
        out_lf = warm_then_measure(make_sine(lf, SR * 2, -20))
        configure(speed=speed)
        out_hf = warm_then_measure(make_sine(hf, SR * 2, -20))
        lf_rel = freq_response_db(out_lf[0], lf) - ref_db
        hf_rel = freq_response_db(out_hf[0], hf) - ref_db

        report(f"{label} LF @ {lf} Hz", lf_rel >= -tol,
               f"{lf_rel:+.1f} dB", f"≥-{tol} dB rel 1k")
        report(f"{label} HF @ {hf} Hz", hf_rel >= -tol,
               f"{hf_rel:+.1f} dB", f"≥-{tol} dB rel 1k")


def test_thd():
    print("\n── §4 THD-mål ──")
    for formula in ("Agfa", "BASF", "Scotch"):
        configure(speed="19 cm/s", formula=formula)
        out = warm_then_measure(make_sine(1000, SR * 2, -3))
        thd = thd_percent(out[0], 1000)
        report(f"THD @ -3 dBFS, 1 kHz ({formula})", thd < 3.0,
               f"{thd:.2f}%", "<3%")
    for formula in ("Agfa", "BASF", "Scotch"):
        configure(speed="19 cm/s", formula=formula)
        out = warm_then_measure(make_sine(1000, SR * 2, 0))
        thd = thd_percent(out[0], 1000)
        report(f"THD @  0 dBFS, 1 kHz ({formula})", thd < 8.0,
               f"{thd:.2f}%", "<8%")


def test_noise_floor():
    print("\n── §5 Brusgolv (S/N) ──")
    targets = [
        ("19 cm/s",   "19 cm/s",   -55.0),
        ("9.5 cm/s",  "9.5 cm/s",  -53.0),
        ("4.75 cm/s", "4.75 cm/s", -50.0),
    ]
    for label, speed, target_db in targets:
        configure(speed=speed)
        out = warm_then_measure(make_silence(SR * 2))
        n2 = out.shape[1] // 2
        noise = rms_dbfs(out[:, n2:])
        report(f"Output noise (silence in) @ {label}",
               noise < target_db,
               f"{noise:.1f} dBFS",
               f"<{target_db} dBFS")


def test_tape_glow():
    print("\n── §6 'Tape glow'-resonans ──")
    glow = [
        ("19 cm/s",   "19 cm/s",   [5000, 6000, 7000, 8000],   3.0),
        ("9.5 cm/s",  "9.5 cm/s",  [4000, 5000, 6000, 7000],   5.0),
        ("4.75 cm/s", "4.75 cm/s", [3000, 4000, 5000],         6.0),
    ]
    for label, speed, freqs, target in glow:
        configure(speed=speed)
        ref_out = warm_then_measure(make_sine(1000, SR * 2, -20))
        ref_db = freq_response_db(ref_out[0], 1000)
        peaks = []
        for f in freqs:
            configure(speed=speed)
            o = warm_then_measure(make_sine(f, SR * 2, -20))
            peaks.append(freq_response_db(o[0], f) - ref_db)
        max_peak = max(peaks)
        report(f"Tape glow @ {label} ({freqs[0]}–{freqs[-1]} Hz)",
               max_peak >= target - 2.0,
               f"+{max_peak:.1f} dB", f"≥+{target-2:.0f} dB")


def test_separation():
    print("\n── §8 Kanalseparation ──")
    configure()
    sig = make_sine(1000, SR * 2, -10)
    sig[1] = 0
    out = warm_then_measure(sig)
    n2 = out.shape[1] // 2
    L = rms_dbfs(out[0, n2:])
    R = rms_dbfs(out[1, n2:])
    sep = L - R
    report("L→R isolation", sep >= 45.0, f"{sep:.1f} dB", "≥45 dB")

    configure()
    sig = make_sine(1000, SR * 2, -10)
    sig[0] = 0
    out = warm_then_measure(sig)
    L = rms_dbfs(out[0, n2:])
    R = rms_dbfs(out[1, n2:])
    sep = R - L
    report("R→L isolation", sep >= 45.0, f"{sep:.1f} dB", "≥45 dB")


def test_echo():
    print("\n── §9 Echo bounded vid 0.95 ──")
    configure(echo=True, echo_amt=0.95)
    out = warm_then_measure(make_sine(800, SR * 2, -12))
    pk = float(np.max(np.abs(out)))
    report("Echo @ amount=0.95", pk < 1.5, f"peak={pk:.3f}", "<1.5")


def test_cpu():
    print("\n── §11 Performance ──")
    configure()
    sig = (np.random.default_rng(0).standard_normal((2, SR * 5)).astype(np.float32) * 0.2)
    block = 128
    PLUGIN.process(sig[:, :block], SR, reset=True)
    n = sig.shape[1]
    t0 = time.perf_counter()
    for off in range(0, n, block):
        end = min(off + block, n)
        PLUGIN.process(sig[:, off:end], SR, reset=False)
    t1 = time.perf_counter()
    cpu_pct = 100 * (t1 - t0) / (n / SR)
    report("CPU @ 48 kHz / 128 smp / stereo (5 s)",
           cpu_pct < 5.0, f"{cpu_pct:.1f}%", "<5%")


def _process_with_reset(signal):
    """Process signal med reset=True på första blocket."""
    PLUGIN.process(signal[:, :BLOCK], SR, reset=True)
    return process_blocks(signal)


def flutter_sideband_ratio(out_mono, carrier_hz, side_hz=50):
    """Energi i sidoljud (±2–side_hz Hz) relativt total energi kring carrier."""
    n = len(out_mono)
    skip = n // 2
    x = out_mono[skip:] * np.hanning(n - skip).astype(np.float32)
    n2 = len(x)
    fft = np.fft.rfft(x)
    freqs = np.fft.rfftfreq(n2, 1 / SR)
    dist = np.abs(freqs - carrier_hz)
    carrier_mask = dist < 2
    side_mask = (dist >= 2) & (dist <= side_hz)
    ec = float(np.sum(np.abs(fft[carrier_mask]) ** 2))
    es = float(np.sum(np.abs(fft[side_mask]) ** 2))
    return es / (ec + es + 1e-30)


def test_print_through():
    print("\n── §3 Print-through ──")
    # Print-through-buffern är 1.5 s; signal i 2 s fyller buffern,
    # sedan 1 s tystnad → spöket från 1.5 s sedan ska höras (eller ej).
    sig = make_sine(1000, SR * 2, -20)
    sil = make_silence(SR)

    configure(wow=0.0)
    PLUGIN.print_through = 0.0
    _process_with_reset(sig)           # fyll print-buffer med signal
    out_off = process_blocks(sil)      # mät tystnaden
    ghost_off = rms_dbfs(out_off[0])
    report("Print-through OFF — tyst under silence",
           ghost_off < -70.0, f"{ghost_off:.1f} dBFS", "<-70 dBFS")

    configure(wow=0.0)
    PLUGIN.print_through = 0.05       # max
    _process_with_reset(sig)           # fyll print-buffer med signal
    out_on = process_blocks(sil)       # mät tystnaden
    ghost_on = rms_dbfs(out_on[0])
    # Spöket ska vara minst 3 dB tydligare än brus-golvet (OFF-läget)
    delta = ghost_on - ghost_off
    report("Print-through ON (0.05) — spöke ≥3 dB över noise floor",
           delta >= 3.0,
           f"Δ={delta:.1f} dB (ON={ghost_on:.1f}, OFF={ghost_off:.1f})",
           "≥+3 dB")


def test_formula_separation():
    print("\n── §7 Tape-formel-separation ──")
    # BASF (hfCorner×1.40) ska vara märkbart ljusare än Scotch (×0.65) vid 12 kHz.
    results = {}
    for formula in ("BASF", "Agfa", "Scotch"):
        configure(speed="9.5 cm/s", formula=formula)
        ref_out = warm_then_measure(make_sine(1000, SR * 2, -20))
        ref_db = freq_response_db(ref_out[0], 1000)
        configure(speed="9.5 cm/s", formula=formula)
        hf_out = warm_then_measure(make_sine(12000, SR * 2, -20))
        results[formula] = freq_response_db(hf_out[0], 12000) - ref_db

    delta_bs = results["BASF"] - results["Scotch"]
    report("BASF ljusare än Scotch @ 12 kHz (Δ ≥ 3 dB)",
           delta_bs >= 3.0,
           f"{delta_bs:+.1f} dB (BASF {results['BASF']:+.1f}, Scotch {results['Scotch']:+.1f})",
           "≥+3 dB")
    report("BASF ljusare än Agfa @ 12 kHz",
           results["BASF"] > results["Agfa"],
           f"BASF {results['BASF']:+.1f} dB vs Agfa {results['Agfa']:+.1f} dB",
           "BASF > Agfa")
    report("Scotch mörkare än Agfa @ 12 kHz",
           results["Scotch"] < results["Agfa"],
           f"Scotch {results['Scotch']:+.1f} dB vs Agfa {results['Agfa']:+.1f} dB",
           "Scotch < Agfa")


def test_wow_flutter():
    print("\n── §10 Wow & Flutter ──")
    # Vid 5 kHz är β = 2π × 5000 × Δmax / SR ≈ 1.0 vid wow=0.5 →
    # tydliga sidoljud inom ±50 Hz.  Vid wow=0 ska inga sidoljud höras.
    sig = make_sine(5000, SR * 4, -20)   # 4 s → 0.25 Hz FFT-upplösning

    configure(speed="9.5 cm/s", wow=0.0)
    out_flat = warm_then_measure(sig)
    ratio_flat = flutter_sideband_ratio(out_flat[0], 5000)
    report("Wow=0.0 — inga sidoljud vid 5 kHz",
           ratio_flat < 0.01,
           f"{ratio_flat * 100:.2f}% sideband",
           "<1%")

    configure(speed="9.5 cm/s", wow=0.5)
    out_wow = warm_then_measure(sig)
    ratio_wow = flutter_sideband_ratio(out_wow[0], 5000)
    report("Wow=0.5 — mätbar flutter vid 5 kHz",
           ratio_wow > 0.02,
           f"{ratio_wow * 100:.2f}% sideband",
           ">2%")


def main():
    print("══════════════════════════════════════════════════════════════════")
    print("  Beolux 2000 — Studio Sound 1968 spec compliance")
    print(f"  pedalboard {pedalboard.__version__}  •  SR={SR}  •  block={BLOCK}")
    print("══════════════════════════════════════════════════════════════════")

    test_bandwidth()
    test_print_through()
    test_thd()
    test_noise_floor()
    test_tape_glow()
    test_formula_separation()
    test_separation()
    test_echo()
    test_wow_flutter()
    test_cpu()

    total = g_pass + g_fail
    print("\n══════════════════════════════════════════════════════════════════")
    print(f"  RESULTAT: {g_pass}/{total} spec-rader matchar Studio Sound 1968")
    print("══════════════════════════════════════════════════════════════════")
    if g_fail > 0:
        print(f"\n  ❌ {g_fail} spec-rader avviker:")
        for f in fails:
            print(f"     • {f}")
        sys.exit(1)
    else:
        print("\n  ✅ Allt grönt — exact 1968 match")


if __name__ == '__main__':
    main()
