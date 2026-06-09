#!/usr/bin/env python3
"""spectral_guard.py — aliasing-budget + samplerate-konsistens som regression.

Två vakter (Fas 2, UAD-paritet):

  §A Aliasing: het 15 kHz-ton @ 19 cm/s, 48 kHz. Övertonerna (30/45 kHz)
     ligger över Nyquist → ALLT icke-harmoniskt innehåll är aliasing/brus.
     Uppmätt efter ADAA på Ge-stegen: −28,8 dBc (före: −8,2 dBc).
     Gräns: ≤ −24 dBc (5 dB plattformsmarginal).

  §B SR-konsistens: frekvensgång (rel 1 kHz) vid 44,1/48/96 kHz ska inte
     skilja mer än 2,0 dB per ton mellan någon av raterna. Uppmätt max-
     avvikelse vid införandet: 1,1 dB (100 Hz, 44,1 vs 96).

Kör:  python3 spectral_guard.py [sökväg-till-vst3]
Pass-kriterium (ctest-regex): "SPECTRAL GUARD: ALLT OK"
"""
import os, sys
import numpy as np
import pedalboard

PLUGIN_PATH = (sys.argv[1] if len(sys.argv) > 1
               else os.path.expanduser("~/Library/Audio/Plug-Ins/VST3/Beolux 2000.vst3"))
BLOCK = 512

g_fail = 0


def report(name, ok, measured, target):
    global g_fail
    tag = "✅ PASS" if ok else "❌ FAIL"
    print(f"  {tag}  {name:<46} measured={measured:<18} target={target}")
    if not ok:
        g_fail += 1
def render_tone(P, sr, freq, amp, seconds=4, speed="19 cm/s"):
    P.reset()
    P.mic_gain_l = P.mic_gain_r = 0.5
    P.monitor_mode = "Tape"
    P.bypass_tape = False
    P.wow_flutter = 0.0
    P.tape_noise = 0.0
    P.tape_speed = speed
    n = int(sr * seconds)
    t = np.arange(n) / sr
    x = np.stack([(amp * np.sin(2 * np.pi * freq * t)).astype(np.float32)] * 2)
    outs = []
    for off in range(0, n, BLOCK):
        outs.append(P.process(x[:, off:off + BLOCK], sr, reset=False))
    o = np.concatenate(outs, axis=1)[0]
    return o[int(sr):]   # hoppa settling


def test_aliasing(P):
    print("\n── §A Aliasing-budget (15 kHz het @ 19 cm/s, 48 kHz) ──")
    sr = 48000
    f0 = 15000.0
    # Het drive (gain 1.0) så Ge-knees + J-A engageras ordentligt
    P.reset()
    P.mic_gain_l = P.mic_gain_r = 1.0
    P.monitor_mode = "Tape"; P.bypass_tape = False
    P.wow_flutter = 0.0; P.tape_noise = 0.0; P.tape_speed = "19 cm/s"
    n = sr * 4
    t = np.arange(n) / sr
    x = np.stack([(0.7 * np.sin(2 * np.pi * f0 * t)).astype(np.float32)] * 2)
    outs = []
    for off in range(0, n, BLOCK):
        outs.append(P.process(x[:, off:off + BLOCK], sr, reset=False))
    o = np.concatenate(outs, axis=1)[0][sr:]

    w = np.hanning(len(o))
    F = np.abs(np.fft.rfft(o * w))
    fr = np.fft.rfftfreq(len(o), 1 / sr)
    pk = F[np.argmin(np.abs(fr - f0))]
    mask = (np.abs(fr - f0) > 100) & (fr > 500) & (fr < 22000)
    worst = 20 * np.log10(F[mask].max() / pk + 1e-15)
    wf = fr[mask][np.argmax(F[mask])]
    report("Värsta icke-harmoniska topp", worst <= -24.0,
           f"{worst:.1f} dBc @ {wf:.0f} Hz", "≤ −24 dBc")


def test_sr_consistency():
    print("\n── §B Samplerate-konsistens (44,1 / 48 / 96 kHz) ──")
    tones = [100, 1000, 5000, 10000, 15000]
    rel = {}
    for sr in (44100, 48000, 96000):
        # Egen instans per SR — pedalboard cachear prepare-state aggressivt
        P = pedalboard.load_plugin(PLUGIN_PATH)
        resp = {}
        for f in tones:
            o = render_tone(P, sr, f, 0.05, seconds=2)
            w = np.hanning(len(o))
            F = np.abs(np.fft.rfft(o * w))
            frq = np.fft.rfftfreq(len(o), 1 / sr)
            i = np.argmin(np.abs(frq - f))
            resp[f] = 20 * np.log10(F[max(0, i - 3):i + 4].max() + 1e-12)
        rel[sr] = {f: resp[f] - resp[1000] for f in tones}

    for f in tones:
        if f == 1000:
            continue
        vals = [rel[sr][f] for sr in (44100, 48000, 96000)]
        spread = max(vals) - min(vals)
        report(f"Tonkaraktär @ {f} Hz konsistent över SR", spread <= 2.0,
               f"spridning {spread:.2f} dB", "≤ 2,0 dB")


def main():
    print("══════════════════════════════════════════════════════")
    print("  Beolux 2000 — spectral guard (aliasing + SR-konsistens)")
    print("══════════════════════════════════════════════════════")
    P = pedalboard.load_plugin(PLUGIN_PATH)
    test_aliasing(P)
    test_sr_consistency()
    print()
    if g_fail == 0:
        print("SPECTRAL GUARD: ALLT OK")
    else:
        print(f"SPECTRAL GUARD: {g_fail} FAIL")
        sys.exit(1)


if __name__ == "__main__":
    main()
