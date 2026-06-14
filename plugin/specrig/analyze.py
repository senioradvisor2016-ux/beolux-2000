#!/usr/bin/env python3
"""analyze.py — mäter en renderad plugin-output mot specs.json.

Tar WAV:en som renderats genom Germanium 2000 Deluxe (via Ableton offline-export
ELLER valfri host) och den ursprungliga calib_pass.wav, alignar dem, och kör:
  - THD vs nivå (1 kHz)
  - frekvensgång (sweep transfer-funktion → -3 dB band-edges)
  - brusgolv → S/N
  - kanalseparation
  - harmonik-profil (h2 vs h3)

Skriver en PASS/FAIL-tabell och en JSON med råmått.

Endast numpy. Kör:
  python analyze.py --render renders/out_19.wav --speed 19
  (input + manifest hittas automatiskt i signals/)
"""
from __future__ import annotations
import argparse
import json
import wave
from pathlib import Path
import numpy as np

HERE = Path(__file__).parent
SIG = HERE / "signals"


# ----------------------------- WAV I/O -----------------------------

def read_wav(path: Path) -> tuple[np.ndarray, int]:
    """Returnerar (data float [n, ch], sr). Stöder 16/24/32-bit PCM."""
    with wave.open(str(path), "rb") as w:
        ch, sw, sr, n = w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getnframes()
        raw = w.readframes(n)
    if sw == 2:
        a = np.frombuffer(raw, dtype="<i2").astype(np.float64) / 32768.0
    elif sw == 3:
        b = np.frombuffer(raw, dtype=np.uint8).reshape(-1, 3).astype(np.int32)
        v = (b[:, 0] | (b[:, 1] << 8) | (b[:, 2] << 16))
        v = np.where(v & 0x800000, v - 0x1000000, v).astype(np.float64) / (2**23)
        a = v
    elif sw == 4:
        a = np.frombuffer(raw, dtype="<i4").astype(np.float64) / (2**31)
    else:
        raise ValueError(f"sampwidth {sw} ej stödd")
    return a.reshape(-1, ch), sr


def bh7(n: int) -> np.ndarray:
    a = [0.27105140069342, 0.43329793923448, 0.21812299954311,
         0.06592544638803, 0.01081174209837, 0.00077658482522, 0.00001388721735]
    k = np.arange(n)
    return sum(((-1) ** i) * a[i] * np.cos(2 * np.pi * i * k / (n - 1)) for i in range(7))


def rms_db(x: np.ndarray) -> float:
    return 20.0 * np.log10(np.sqrt(np.mean(x ** 2)) + 1e-30)


# ----------------------------- alignment -----------------------------

def estimate_lag(ref: np.ndarray, sig: np.ndarray, sr: int, max_ms: float = 60.0) -> int:
    """Grov global lag (samples) som maximerar korr mellan envelopper."""
    def env(x):
        x = np.abs(x)
        k = sr // 1000
        return np.convolve(x, np.ones(k) / k, mode="same")[::k]
    re, se = env(ref), env(sig)
    m = min(len(re), len(se))
    re, se = re - re.mean(), se[:m] - se[:m].mean()
    re = re[:m]
    n = len(re)
    full = np.correlate(se, re, mode="full")
    lags = np.arange(-n + 1, n)
    maxlag = int(max_ms / 1000 * sr / (sr // 1000)) + 2
    mask = np.abs(lags) <= maxlag
    best = lags[mask][np.argmax(full[mask])]
    return int(best * (sr // 1000))


# ----------------------------- mätningar -----------------------------

def measure_thd(seg: np.ndarray, sr: int, f0: float) -> dict:
    x = seg * bh7(len(seg))
    sp = np.abs(np.fft.rfft(x))
    freqs = np.fft.rfftfreq(len(seg), 1 / sr)
    def amp_at(f):
        b = int(round(f / (sr / len(seg))))
        lo, hi = max(1, b - 3), min(len(sp), b + 4)
        return sp[lo:hi].max()
    h1 = amp_at(f0)
    harms = [amp_at(f0 * k) for k in range(2, 11) if f0 * k < sr / 2]
    thd = np.sqrt(np.sum(np.square(harms))) / (h1 + 1e-30)
    h2 = amp_at(f0 * 2) / (h1 + 1e-30)
    h3 = amp_at(f0 * 3) / (h1 + 1e-30)
    return {"thd_pct": 100 * thd, "h2_pct": 100 * h2, "h3_pct": 100 * h3}


def measure_fr(in_seg: np.ndarray, out_seg: np.ndarray, sr: int) -> dict:
    n = min(len(in_seg), len(out_seg))
    win = bh7(n)
    X = np.fft.rfft(in_seg[:n] * win)
    Y = np.fft.rfft(out_seg[:n] * win)
    freqs = np.fft.rfftfreq(n, 1 / sr)
    H = np.abs(Y) / (np.abs(X) + 1e-9)
    # bara där sweepen har energi
    valid = np.abs(X) > (np.abs(X).max() * 1e-3)
    # log-spaced smoothing
    fmin, fmax = 20.0, 20000.0
    band = valid & (freqs >= fmin) & (freqs <= fmax)
    f = freqs[band]
    h_db = 20 * np.log10(H[band] + 1e-9)
    # 1/6-oktav glättning
    lf = np.logspace(np.log10(fmin), np.log10(fmax), 200)
    sm = np.interp(lf, f, h_db)
    # normalisera mot 1 kHz
    ref = np.interp(1000.0, lf, sm)
    sm -= ref
    # -3 dB edges
    lo_edge = next((fr for fr in lf if np.interp(fr, lf, sm) >= -3.0), lf[0])
    hi_edge = next((fr for fr in reversed(lf) if np.interp(fr, lf, sm) >= -3.0), lf[-1])
    return {"f": lf, "db": sm, "lo_3db_hz": float(lo_edge), "hi_3db_hz": float(hi_edge)}


# ----------------------------- main -----------------------------

def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--render", required=True, help="renderad output-WAV")
    ap.add_argument("--input", default=str(SIG / "calib_pass.wav"))
    ap.add_argument("--manifest", default=str(SIG / "manifest.json"))
    ap.add_argument("--specs", default=str(HERE / "specs.json"))
    ap.add_argument("--speed", default="19", help="19 / 9.5 / 4.75 (för spec-grindar)")
    ap.add_argument("--json", default="", help="skriv råmått hit")
    args = ap.parse_args()

    man = json.loads(Path(args.manifest).read_text())
    specs = json.loads(Path(args.specs).read_text())
    indata, sr = read_wav(Path(args.input))
    out, sro = read_wav(Path(args.render))
    assert sr == sro == man["sr"], f"SR-mismatch in={sr} out={sro} man={man['sr']}"

    settle = int(specs["reference"]["settle_s"] * sr)
    tail = int(specs["reference"]["tail_trim_s"] * sr)

    inL = indata[:, 0]
    outL = out[:, 0]
    outR = out[:, 1] if out.shape[1] > 1 else out[:, 0]
    lag = estimate_lag(inL, outL, sr)
    print(f"[align] global lag = {lag} samples ({1000*lag/sr:+.1f} ms)")

    def slc(ch, start, end):
        a, b = start + lag + settle, end + lag - tail
        a, b = max(0, a), min(len(ch), b)
        return ch[a:b]

    results = {"speed": args.speed, "lag_samples": int(lag), "segments": {}}
    rows = []
    fr_res = None

    for s in man["segments"]:
        name, typ = s["name"], s["type"]
        if typ == "thd":
            seg = slc(outL, s["start"], s["end"])
            m = measure_thd(seg, sr, s["freq"])
            lvl = str(int(s["level_dbfs"]))
            lim = specs["thd_pct_max"].get(lvl)
            ok = (lim is None) or (m["thd_pct"] <= lim)
            results["segments"][name] = m
            rows.append((f"THD @ {lvl} dBFS", f"{m['thd_pct']:.3f} %",
                         f"≤ {lim} %" if lim else "—", ok))
        elif typ == "fr":
            seg_in = inData_slc = inL[s["start"]+settle:s["end"]-tail]
            seg_out = slc(outL, s["start"], s["end"])
            fr = measure_fr(seg_in, seg_out, sr)
            fr_res = fr
            g = specs["freq_response"].get(args.speed, {})
            lo_ok = fr["lo_3db_hz"] <= g.get("lo_3db_max_hz", 1e9)
            hi_ok = fr["hi_3db_hz"] >= g.get("hi_3db_min_hz", 0)
            results["segments"][name] = {"lo_3db_hz": fr["lo_3db_hz"], "hi_3db_hz": fr["hi_3db_hz"]}
            rows.append(("FR -3dB low", f"{fr['lo_3db_hz']:.0f} Hz",
                         f"≤ {g.get('lo_3db_max_hz','—')} Hz", lo_ok))
            rows.append(("FR -3dB high", f"{fr['hi_3db_hz']:.0f} Hz",
                         f"≥ {g.get('hi_3db_min_hz','—')} Hz", hi_ok))
        elif typ == "noise":
            seg = slc(outL, s["start"], s["end"])
            nf = rms_db(seg)
            snr = -nf
            lim = specs["snr_db_min"].get(args.speed, 55.0)
            ok = snr >= lim
            results["segments"][name] = {"noise_floor_dbfs": nf, "snr_db": snr}
            rows.append(("S/N (re 0 dBFS)", f"{snr:.1f} dB", f"≥ {lim} dB", ok))
        elif typ == "separation":
            segL = slc(outL, s["start"], s["end"])
            segR = slc(outR, s["start"], s["end"])
            sep = rms_db(segL) - rms_db(segR)
            lim = specs["channel_separation_db_min"]
            ok = sep >= lim
            results["segments"][name] = {"separation_db": sep}
            rows.append(("Kanalseparation", f"{sep:.1f} dB", f"≥ {lim} dB", ok))

    # harmonik-profil
    hp = specs.get("harmonic_profile", {})
    lvl = str(hp.get("test_level_dbfs", -6))
    seg_name = f"thd_1k_{lvl}"
    if seg_name in results["segments"]:
        m = results["segments"][seg_name]
        ok = m["h2_pct"] > m["h3_pct"] if hp.get("h2_gt_h3") else True
        rows.append((f"h2>h3 @ {lvl} dBFS",
                     f"h2={m['h2_pct']:.2f}% h3={m['h3_pct']:.2f}%", "h2 > h3", ok))

    # rapport
    print("\n  Mätning                       Uppmätt                 Spec            ")
    print("  " + "-" * 74)
    allok = True
    for label, meas, spec, ok in rows:
        allok &= ok
        flag = "PASS" if ok else "FAIL"
        print(f"  {label:<28} {str(meas):<22} {str(spec):<15} [{flag}]")
    print("  " + "-" * 74)
    print(f"  TOTAL: {'ALLA PASS' if allok else 'AVVIKELSER FINNS'}\n")

    if fr_res is not None:
        print("  FR (dB re 1 kHz):")
        for fq in (30, 50, 100, 1000, 5000, 8000, 12000, 16000, 18000):
            print(f"    {fq:>6} Hz : {np.interp(fq, fr_res['f'], fr_res['db']):+.1f} dB")

    if args.json:
        Path(args.json).write_text(json.dumps(
            {k: v for k, v in results.items()}, indent=2, default=float))
        print(f"\n  Råmått → {args.json}")

    raise SystemExit(0 if allok else 1)


if __name__ == "__main__":
    main()
