#!/usr/bin/env python3
"""analyze_fr.py — PÅLITLIG frekvensgång från stegade toner (calib_fr.wav).

Mäter varje tons RMS i en render, normaliserar mot 1250 Hz, ger en kurva +
-3 dB band-edges. Ingen sweep-transfer → inga artefakter i band-extremer.
Kör:  python3 analyze_fr.py
"""
from __future__ import annotations
import json
from pathlib import Path
import numpy as np
from analyze import read_wav, bh7, estimate_lag

HERE = Path(__file__).parent
SIG = HERE / "signals"; REN = HERE / "renders"
MAN = json.loads((SIG / "manifest_fr.json").read_text())
SR = MAN["sr"]
INP, _ = read_wav(SIG / "calib_fr.wav")
TONES = [s for s in MAN["segments"] if s["type"] == "fr_step"]
REF_HZ = 1250.0


def tone_level_db(ch, s, lag):
    """RMS (dB) för en tons mittparti (trimmar attack/decay)."""
    a = s["start"] + lag + int(0.15 * SR)
    b = s["end"] + lag - int(0.1 * SR)
    seg = ch[max(0, a):min(len(ch), b)]
    if len(seg) < 64:
        return -120.0
    return 20 * np.log10(np.sqrt(np.mean(seg ** 2)) + 1e-20)


def curve(case):
    p = REN / f"out_{case}.wav"
    if not p.exists():
        return None
    out, _ = read_wav(p)
    L = out[:, 0]
    lag = estimate_lag(INP[:, 0], L, SR)
    levels = {s["freq"]: tone_level_db(L, s, lag) for s in TONES}
    ref = levels.get(REF_HZ, max(levels.values()))
    return {f: lv - ref for f, lv in levels.items()}


def edges(c):
    """-3 dB low/high re passbandsmax (200–5000 Hz)."""
    fs = sorted(c)
    pb = [c[f] for f in fs if 200 <= f <= 5000]
    pk = max(pb) if pb else 0.0
    lo = next((f for f in fs if c[f] - pk >= -3.0), fs[0])
    hi = next((f for f in reversed(fs) if c[f] - pk >= -3.0), fs[-1])
    return lo, hi


def main():
    cases = {
        "Hastighet": [("fr_speed_19", "19 cm/s"), ("fr_speed_95", "9.5 cm/s"), ("fr_speed_475", "4.75 cm/s")],
        "Tape-formula (speed 19)": [("fr_agfa", "Agfa"), ("fr_basf", "BASF"), ("fr_scotch", "Scotch")],
    }
    show_f = [50, 125, 315, 800, 1250, 3150, 8000, 12500, 16000, 20000]
    for group, items in cases.items():
        print(f"\n══ Frekvensgång — {group} (dB re 1250 Hz) ══")
        hdr = "  " + f"{'fall':<12}" + "".join(f"{f:>7}" for f in show_f) + "   -3dB-band"
        print(hdr); print("  " + "-" * (len(hdr) - 2))
        for case, label in items:
            c = curve(case)
            if not c:
                print(f"  {label:<12}  SAKNAS"); continue
            vals = "".join(f"{c[f]:>+7.1f}" for f in show_f)
            lo, hi = edges(c)
            print(f"  {label:<12}{vals}   {lo:.0f}–{hi:.0f} Hz")
    print()


if __name__ == "__main__":
    main()
