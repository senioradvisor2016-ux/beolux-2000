#!/usr/bin/env python3
"""analyze_features.py — mäter funktions-renders (echo/SOS/wow/multiplay/formula/
drive/volym) från REAPER-matrisen mot calib_full.wav.

Förutsätter renders/out_<case>.wav från reaper-render-matrix.sh + manifest_full.json.
Kör:  python3 analyze_features.py
"""
from __future__ import annotations
import json
from pathlib import Path
import numpy as np
from analyze import read_wav, bh7, rms_db, estimate_lag

HERE = Path(__file__).parent
SIG = HERE / "signals"
REN = HERE / "renders"
MAN = json.loads((SIG / "manifest_full.json").read_text())
SR = MAN["sr"]
INP, _ = read_wav(SIG / "calib_full.wav")


def seg_bounds(name):
    for s in MAN["segments"]:
        if s["name"] == name:
            return s["start"], s["end"]
    raise KeyError(name)


def load_aligned(case):
    """Returnerar (L, R, lag) för en render, lag-justerad mot input."""
    p = REN / f"out_{case}.wav"
    if not p.exists():
        return None
    out, _ = read_wav(p)
    L = out[:, 0]; R = out[:, 1] if out.shape[1] > 1 else out[:, 0]
    lag = estimate_lag(INP[:, 0], L, SR)
    return L, R, lag


def seg(ch, name, lag, settle=0.0, tail=0.0):
    a, b = seg_bounds(name)
    i, j = a + lag + int(settle * SR), b + lag - int(tail * SR)
    return ch[max(0, i):min(len(ch), j)]


def band_energy_db(x, f0, sr, lo_off, hi_off):
    """Energi (dB) i [f0+lo_off, f0+hi_off] exkl. carrier-bin, re carrier."""
    w = bh7(len(x)); X = np.abs(np.fft.rfft(x * w))
    fr = np.fft.rfftfreq(len(x), 1 / sr)
    cb = int(round(f0 / (sr / len(x))))
    carrier = X[max(1, cb - 2):cb + 3].max()
    band = (fr >= f0 + lo_off) & (fr <= f0 + hi_off) & (np.abs(fr - f0) > 8)
    side = X[band].max() if band.any() else 1e-12
    return 20 * np.log10(side / (carrier + 1e-20))


def fr_at(Lout, lag, fq):
    """FR (dB re 1 kHz) vid fq via sweep-transferfunktion."""
    si = seg(INP[:, 0], "sweep", 0); so = seg(Lout, "sweep", lag)
    n = min(len(si), len(so)); w = bh7(n)
    X = np.fft.rfft(si[:n] * w); Y = np.fft.rfft(so[:n] * w)
    fr = np.fft.rfftfreq(n, 1 / SR)
    H = 20 * np.log10(np.abs(Y) / (np.abs(X) + 1e-9) + 1e-9)
    ref = np.interp(1000.0, fr, H)
    return float(np.interp(fq, fr, H) - ref)


def row(label, val, verdict):
    print(f"  {label:<40} {val:<26} {verdict}")


def main():
    base = load_aligned("speed_19")
    print("\n══ FUNKTIONSMATRIS — REAPER-renders vs calib_full ══\n")

    # ---- Hastigheter: HF-rolloff ----
    print("─ Tape-hastighet (HF-band-edge via sweep) ─")
    for case, fq in (("speed_19", 16000), ("speed_95", 12000), ("speed_475", 6000)):
        r = load_aligned(case)
        if not r: row(case, "SAKNAS", "—"); continue
        L, _, lag = r
        db = fr_at(L, lag, fq)
        row(f"{case} @ {fq} Hz", f"{db:+.1f} dB", "ok (bandkant)" if db > -8 else "kraftig rolloff")

    # ---- Echo ----
    print("\n─ Echo (post-burst-energi vs no-echo) ─")
    re = load_aligned("echo_on")
    if re and base:
        Le, _, lge = re; Lb, _, lgb = base
        # energi 60–2900 ms efter burst-start
        eb = lambda ch, lag: rms_db(seg(ch, "echo_burst", lag)[int(0.06*SR):int(2.9*SR)])
        ee, eo = eb(Le, lge), eb(Lb, lgb)
        row("Echo-svans (echo_on)", f"{ee:.1f} dBFS", "")
        row("Echo-svans (no-echo baseline)", f"{eo:.1f} dBFS", "")
        row("Echo aktivt (Δ ≥ 12 dB)", f"{ee-eo:+.1f} dB", "PASS" if ee - eo >= 12 else "SVAG")
    else:
        row("echo_on", "SAKNAS", "—")

    # ---- SOS: nivå-uppbyggnad ----
    print("\n─ Sound-on-Sound (RMS-uppbyggnad över sustain) ─")
    rs = load_aligned("sos_on")
    if rs:
        Ls, _, lg = rs
        s = seg(Ls, "sos_sustain", lg)
        first = rms_db(s[:int(SR)]); last = rms_db(s[-int(SR):])
        row("SOS RMS start → slut", f"{first:.1f} → {last:.1f} dBFS", "")
        row("SOS bygger upp (Δ ≥ +1 dB)", f"{last-first:+.1f} dB", "PASS" if last - first >= 1 else "FLAT")
    else:
        row("sos_on", "SAKNAS", "—")

    # ---- Wow/Flutter: FM-sidband på 3150 Hz ----
    print("\n─ Wow/Flutter (FM-sidband @ 3150 Hz) ─")
    for case in ("wow_off", "wow_high"):
        r = load_aligned(case)
        if not r: row(case, "SAKNAS", "—"); continue
        L, _, lag = r
        x = seg(L, "wow_tone", lag, settle=0.3, tail=0.3)
        sb = band_energy_db(x, 3150.0, SR, -300, 300)
        row(f"{case} sidband", f"{sb:+.1f} dB re carrier", "")
    print("  (förväntat: wow_high ≫ wow_off)")

    # ---- Multiplay: HF-förlust ----
    print("\n─ Multiplay (HF @ 8 kHz, gen5 vs gen1) ─")
    rm = load_aligned("multiplay_5")
    if rm and base:
        Lm, _, lgm = rm; Lb, _, lgb = base
        hm, hb = fr_at(Lm, lgm, 8000), fr_at(Lb, lgb, 8000)
        row("multiplay gen5 @ 8 kHz", f"{hm:+.1f} dB", "")
        row("baseline gen1 @ 8 kHz", f"{hb:+.1f} dB", "")
        row("Gen-förlust (gen5 < gen1)", f"{hm-hb:+.1f} dB", "PASS" if hm < hb - 1 else "SVAG")

    # ---- Tape-formula: ljushet ----
    print("\n─ Tape-formula (ljushet @ 12 kHz vs Agfa/baseline) ─")
    for case in ("formula_basf", "formula_scotch"):
        r = load_aligned(case)
        if not r: row(case, "SAKNAS", "—"); continue
        L, _, lag = r
        row(f"{case} @ 12 kHz", f"{fr_at(L, lag, 12000):+.1f} dB", "")
    if base: row("baseline (Agfa) @ 12 kHz", f"{fr_at(base[0], base[2], 12000):+.1f} dB", "")

    # ---- Hård drive: stabilitet ----
    print("\n─ HÅRD drive (input_trim +18 + sat 2.0) ─")
    rd = load_aligned("drive_hot")
    if rd:
        Ld, Rd, lg = rd
        full = np.concatenate([Ld, Rd])
        nan = not np.all(np.isfinite(full)); peak = float(np.max(np.abs(full)))
        thd_seg = seg(Ld, "thd_1k_0", lg, settle=0.5, tail=0.1)
        w = bh7(len(thd_seg)); sp = np.abs(np.fft.rfft(thd_seg * w))
        b = int(round(1000 / (SR / len(thd_seg))))
        h1 = sp[b-2:b+3].max()
        harms = [sp[int(round(1000*k/(SR/len(thd_seg))))-2:int(round(1000*k/(SR/len(thd_seg))))+3].max()
                 for k in range(2, 8)]
        thd = 100 * np.sqrt(np.sum(np.square(harms))) / (h1 + 1e-20)
        row("Ingen NaN/Inf", "ren" if not nan else "NaN!", "PASS" if not nan else "FAIL")
        row("Peak bounded (< 2.0)", f"{peak:.3f}", "PASS" if peak < 2.0 else "FAIL")
        row("THD @ 0 dBFS hårt driven", f"{thd:.1f} %", "mättad men stabil" if thd < 60 else "extrem")
    else:
        row("drive_hot", "SAKNAS", "—")

    # ---- Volym-skalning ----
    print("\n─ Volym (output-RMS vs baseline) ─")
    if base:
        bl = rms_db(seg(base[0], "thd_1k_-12", base[2], settle=0.5, tail=0.1))
        row("baseline (vol 0.85) @ -12", f"{bl:.1f} dBFS", "")
        for case in ("vol_low", "vol_high"):
            r = load_aligned(case)
            if not r: row(case, "SAKNAS", "—"); continue
            v = rms_db(seg(r[0], "thd_1k_-12", r[2], settle=0.5, tail=0.1))
            row(f"{case} @ -12", f"{v:.1f} dBFS ({v-bl:+.1f} vs base)", "")

    # ---- Bias-svep: THD @ -6 (under < optimal < över förväntat) ----
    print("\n─ Bias (THD @ -6 dBFS) ─")
    def thd6(case):
        r = load_aligned(case)
        if not r: return None
        L, _, lag = r; s = seg(L, "thd_1k_-6", lag, settle=0.5, tail=0.1)
        w = bh7(len(s)); sp = np.abs(np.fft.rfft(s * w)); bn = SR / len(s)
        b = int(round(1000 / bn)); h1 = sp[b-2:b+3].max()
        h = [sp[int(round(1000*k/bn))-2:int(round(1000*k/bn))+3].max() for k in range(2, 8)]
        return 100 * np.sqrt(np.sum(np.square(h))) / (h1 + 1e-20)
    for case, label in (("bias_lo", "under-bias 0.5"), ("speed_19", "optimal 1.0"), ("bias_hi", "över-bias 1.5")):
        t = thd6(case); row(label, f"{t:.2f} %" if t is not None else "SAKNAS", "")

    # ---- Mains hum: spektraltopp i tystnad ----
    print("\n─ Mains hum (topp @ hum-frekvens i tystnad) ─")
    def hum_at(case, f0):
        r = load_aligned(case)
        if not r: return None
        L, _, lag = r; s = seg(L, "silence", lag, settle=0.3, tail=0.3)
        w = bh7(len(s)); sp = np.abs(np.fft.rfft(s * w)); bn = SR / len(s)
        b = int(round(f0 / bn))
        return 20 * np.log10(sp[b-2:b+3].max() / (len(s) / 2) + 1e-20)
    for case, f0 in (("mains_hum_50", 50), ("mains_hum_60", 60), ("speed_19", 50)):
        v = hum_at(case, f0); row(f"{case} @ {f0} Hz", f"{v:.1f} dB" if v is not None else "SAKNAS", "")
    print("  (förväntat: hum-fall ≫ baseline vid sin frekvens)")

    # ---- Print-through: för-eko-spöke före burst ----
    print("\n─ Print-through (för-eko 200 ms före burst) ─")
    rp = load_aligned("print_through"); rb = load_aligned("speed_19")
    if rp and rb:
        def pre(ch, lag):
            a, _ = seg_bounds("echo_burst")
            return rms_db(ch[max(0, a + lag - int(0.2*SR)):a + lag - int(0.01*SR)])
        pv, bv = pre(rp[0], rp[2]), pre(rb[0], rb[2])
        row("print_through för-burst", f"{pv:.1f} dBFS", "")
        row("baseline för-burst", f"{bv:.1f} dBFS", "")
        row("Print-through aktivt (Δ > 3 dB)", f"{pv-bv:+.1f} dB", "PASS" if pv - bv > 3 else "SUBTIL")

    # ---- Synchroplay: stabilitet ----
    print("\n─ Synchroplay (stabil output) ─")
    rs2 = load_aligned("synchroplay")
    if rs2:
        full = np.concatenate([rs2[0], rs2[1]]); pk = float(np.max(np.abs(full)))
        ok = np.all(np.isfinite(full)) and pk < 2.0
        row("Synchroplay ändlig + bounded", f"peak {pk:.3f}", "PASS" if ok else "FAIL")
    else:
        row("synchroplay", "SAKNAS", "—")

    print()


if __name__ == "__main__":
    main()
