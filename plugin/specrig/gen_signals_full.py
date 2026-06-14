#!/usr/bin/env python3
"""gen_signals_full.py — utökad kalibreringssignal med feature-segment.

Skriver signals/calib_full.wav + signals/manifest_full.json (48 kHz, 24-bit stereo).
Bygger på calib_pass-segmenten och lägger till signaler för echo / wow / SOS.

Segment:
  THD 1 kHz @ -20/-12/-6/0 dBFS   → THD vs nivå + hård drive (via param input_trim)
  log-sweep 20 Hz–20 kHz @ -12     → frekvensgång / tape-formula / multiplay
  tystnad 3 s                      → brusgolv / S/N
  1 kHz @ -10 dBFS endast L        → kanalseparation
  echo_burst: 1 kHz 40 ms @ -6 + 2,96 s tystnad → echo delay/decay
  wow_tone: 3150 Hz @ -12, 5 s     → wow/flutter FM-sidband (DIN-standardfrekvens)
  sos_sustain: 1 kHz @ -18, 6 s    → Sound-on-Sound nivå-uppbyggnad över tid

Endast numpy.  Kör:  python3 gen_signals_full.py
"""
from __future__ import annotations
import json, struct, wave
from pathlib import Path
import numpy as np

SR = 48_000
HERE = Path(__file__).parent
OUT = HERE / "signals"
OUT.mkdir(exist_ok=True)

GUARD_S = 0.4
TONE_S = 2.0
SWEEP_S = 6.0
SILENCE_S = 3.0


def amp(db): return 10.0 ** (db / 20.0)


def tone(freq, dur, db, fade_ms=5.0):
    n = int(SR * dur); t = np.arange(n) / SR
    sig = amp(db) * np.sin(2 * np.pi * freq * t)
    e = int(fade_ms / 1000 * SR)
    if e > 0 and n > 2 * e:
        sig[:e] *= np.linspace(0, 1, e); sig[-e:] *= np.linspace(1, 0, e)
    return sig


def log_sweep(f0, f1, dur, db):
    n = int(SR * dur); t = np.arange(n) / SR
    k = np.log(f1 / f0)
    phase = 2 * np.pi * f0 * dur / k * (np.exp(t / dur * k) - 1.0)
    win = np.ones(n); edge = int(0.01 * SR)
    win[:edge] = np.linspace(0, 1, edge); win[-edge:] = np.linspace(1, 0, edge)
    return amp(db) * np.sin(phase) * win


def guard(): return np.zeros(int(SR * GUARD_S))


def main():
    segs = []

    def add(name, typ, L, R, **meta): segs.append((name, typ, L, R, meta))

    for db in (-20, -12, -6, 0):
        s = tone(1000.0, TONE_S, db); add(f"thd_1k_{db}", "thd", s, s, freq=1000.0, level_dbfs=db)

    sw = log_sweep(20.0, 20000.0, SWEEP_S, -12)
    add("sweep", "fr", sw, sw, f0=20.0, f1=20000.0, level_dbfs=-12)

    sil = np.zeros(int(SR * SILENCE_S)); add("silence", "noise", sil, sil)

    sep = tone(1000.0, TONE_S, -10); add("sep_Lonly", "separation", sep, np.zeros_like(sep),
                                         freq=1000.0, level_dbfs=-10, driven="L")

    # echo: kort burst följt av tystnad så repetitionerna syns
    burst = np.concatenate([tone(1000.0, 0.04, -6), np.zeros(int(SR * 2.96))])
    add("echo_burst", "echo", burst, burst, freq=1000.0, burst_ms=40.0)

    # wow/flutter: 3150 Hz DIN-standard, lång stabil ton → FM-sidband
    w = tone(3150.0, 5.0, -12); add("wow_tone", "wow", w, w, freq=3150.0, level_dbfs=-12)

    # SOS: lång låg ton → mät nivå-uppbyggnad över tid
    ss = tone(1000.0, 6.0, -18); add("sos_sustain", "sos", ss, ss, freq=1000.0, level_dbfs=-18)

    Lparts, Rparts, manifest = [guard()], [guard()], []
    cursor = len(Lparts[0])
    for name, typ, L, R, meta in segs:
        start = cursor
        Lparts.append(L); Rparts.append(R); cursor += len(L)
        manifest.append({"name": name, "type": typ, "start": int(start),
                         "end": int(cursor), "dur_s": len(L) / SR, **meta})
        Lparts.append(guard()); Rparts.append(guard()); cursor += len(Lparts[-1])

    Lc, Rc = np.concatenate(Lparts), np.concatenate(Rparts)
    stereo = np.stack([Lc, Rc], axis=1)

    path = OUT / "calib_full.wav"; write_wav24(path, stereo, SR)
    man = {"sr": SR, "guard_s": GUARD_S, "total_s": len(Lc) / SR, "segments": manifest}
    (OUT / "manifest_full.json").write_text(json.dumps(man, indent=2))
    print(f"Skrev {path}  ({len(Lc)/SR:.2f}s, {len(manifest)} segment)")

    # ── calib_fr.wav: stegade toner för PÅLITLIG frekvensgång (ej sweep) ──
    FREQS = [30, 50, 80, 125, 200, 315, 500, 800, 1250, 2000,
             3150, 5000, 8000, 12500, 16000, 20000]
    TONE_FR_S = 0.6
    fL, fR, fman = [guard()], [guard()], []
    cur = len(fL[0])
    for fq in FREQS:
        s = tone(float(fq), TONE_FR_S, -12.0, fade_ms=20.0)
        st = cur; fL.append(s); fR.append(s); cur += len(s)
        fman.append({"name": f"fr_{fq}", "type": "fr_step", "start": int(st),
                     "end": int(cur), "freq": float(fq), "level_dbfs": -12})
        fL.append(guard()); fR.append(guard()); cur += len(fL[-1])
    fLc, fRc = np.concatenate(fL), np.concatenate(fR)
    fpath = OUT / "calib_fr.wav"; write_wav24(fpath, np.stack([fLc, fRc], axis=1), SR)
    (OUT / "manifest_fr.json").write_text(json.dumps(
        {"sr": SR, "guard_s": GUARD_S, "total_s": len(fLc) / SR, "segments": fman}, indent=2))
    print(f"Skrev {fpath}  ({len(fLc)/SR:.2f}s, {len(fman)} toner)")


def write_wav24(path, data, sr):
    d = np.clip(data, -1.0, 1.0); ints = (d * (2**23 - 1)).astype(np.int32)
    frames = bytearray()
    for s in ints.reshape(-1): frames += struct.pack("<i", int(s))[0:3]
    with wave.open(str(path), "w") as w:
        w.setnchannels(2); w.setsampwidth(3); w.setframerate(sr); w.writeframes(bytes(frames))


if __name__ == "__main__":
    main()
