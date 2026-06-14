#!/usr/bin/env python3
"""gen_signals.py — genererar kalibrerade test-signaler för spec-riggen.

Skriver:
  signals/calib_pass.wav   — EN konkatenerad stereo-WAV med alla testsegment
                             (1 export i Ableton räcker för en full mätomgång)
  signals/manifest.json    — segment-tabell (typ, start/slut-sample, nivå, kanal)

Segmenten (48 kHz, stereo, float→16/24-bit WAV):
  1kHz toner @ -20/-12/-6/0 dBFS  → THD vs nivå
  log-sweep 20 Hz–20 kHz @ -12     → frekvensgång
  tystnad                          → brusgolv / S/N
  1kHz @ -10 dBFS endast i L       → kanalseparation

Endast numpy. Kör:  python gen_signals.py
"""
from __future__ import annotations
import json
import struct
import wave
from pathlib import Path
import numpy as np

SR = 48_000
HERE = Path(__file__).parent
OUT = HERE / "signals"
OUT.mkdir(exist_ok=True)

GUARD_S = 0.4          # tystnad mellan segment (settle + segmentdetektering)
TONE_S = 2.0
SWEEP_S = 6.0
SILENCE_S = 3.0


def dbfs_to_amp(db: float) -> float:
    return 10.0 ** (db / 20.0)


def tone(freq: float, dur: float, db: float) -> np.ndarray:
    n = int(SR * dur)
    t = np.arange(n) / SR
    return dbfs_to_amp(db) * np.sin(2 * np.pi * freq * t)


def log_sweep(f0: float, f1: float, dur: float, db: float) -> np.ndarray:
    n = int(SR * dur)
    t = np.arange(n) / SR
    k = np.log(f1 / f0)
    phase = 2 * np.pi * f0 * dur / k * (np.exp(t / dur * k) - 1.0)
    win = np.ones(n)
    edge = int(0.01 * SR)
    win[:edge] = np.linspace(0, 1, edge)
    win[-edge:] = np.linspace(1, 0, edge)
    return dbfs_to_amp(db) * np.sin(phase) * win


def guard() -> np.ndarray:
    return np.zeros(int(SR * GUARD_S))


def main() -> None:
    segs = []          # (name, type, L, R, meta)
    g = guard()

    def add(name, typ, L, R, **meta):
        segs.append((name, typ, L, R, meta))

    for db in (-20, -12, -6, 0):
        s = tone(1000.0, TONE_S, db)
        add(f"thd_1k_{db}", "thd", s, s, freq=1000.0, level_dbfs=db)

    sw = log_sweep(20.0, 20000.0, SWEEP_S, -12)
    add("sweep", "fr", sw, sw, f0=20.0, f1=20000.0, level_dbfs=-12)

    sil = np.zeros(int(SR * SILENCE_S))
    add("silence", "noise", sil, sil)

    sep = tone(1000.0, TONE_S, -10)
    add("sep_Lonly", "separation", sep, np.zeros_like(sep), freq=1000.0, level_dbfs=-10, driven="L")

    # konkatenera med guard mellan
    Lparts, Rparts, manifest = [g.copy()], [g.copy()], []
    cursor = len(g)
    for name, typ, L, R, meta in segs:
        start = cursor
        Lparts.append(L); Rparts.append(R)
        cursor += len(L)
        manifest.append({"name": name, "type": typ,
                         "start": int(start), "end": int(cursor),
                         "dur_s": len(L) / SR, **meta})
        Lparts.append(g.copy()); Rparts.append(g.copy())
        cursor += len(g)

    Lc = np.concatenate(Lparts)
    Rc = np.concatenate(Rparts)
    stereo = np.stack([Lc, Rc], axis=1)

    # skriv 24-bit WAV
    path = OUT / "calib_pass.wav"
    write_wav24(path, stereo, SR)

    man = {"sr": SR, "guard_s": GUARD_S, "total_s": len(Lc) / SR, "segments": manifest}
    (OUT / "manifest.json").write_text(json.dumps(man, indent=2))
    print(f"Skrev {path}  ({len(Lc)/SR:.2f}s, {len(manifest)} segment)")
    print(f"Skrev {OUT / 'manifest.json'}")


def write_wav24(path: Path, data: np.ndarray, sr: int) -> None:
    """24-bit PCM stereo. data: float [-1,1], shape (n,2)."""
    d = np.clip(data, -1.0, 1.0)
    ints = (d * (2**23 - 1)).astype(np.int32)
    frames = bytearray()
    for s in ints.reshape(-1):
        frames += struct.pack("<i", int(s))[0:3]   # låg 3 byte (little-endian 24-bit)
    with wave.open(str(path), "w") as w:
        w.setnchannels(2)
        w.setsampwidth(3)
        w.setframerate(sr)
        w.writeframes(bytes(frames))


if __name__ == "__main__":
    main()
