# Specrig — Reaper-arbetsflöde

Kalibreringsfilen `signals/calib_pass.wav` ersätter en synt/tongenerator: en
enda 24-bit/48 kHz stereo-WAV (22,2 s) med alla testsegment. Rendera den genom
Germanium 2000 Deluxe i Reaper och analysera med `analyze.py`.

## Segment i calib_pass.wav

| Segment | Tid | Mäter |
|---|---|---|
| 1 kHz @ −20/−12/−6/0 dBFS | 0,4–9,6 s | THD vs nivå + h2/h3-profil |
| Log-sweep 20 Hz–20 kHz @ −12 | 10,0–16,0 s | Frekvensgång (−3 dB-edges) |
| Tystnad 3 s | 16,4–19,4 s | Brusgolv → S/N |
| 1 kHz @ −10 dBFS, endast L | 19,8–21,8 s | Kanalseparation |

(0,4 s tystnad mellan segment för settle + segmentdetektering.)

## Steg i Reaper

1. Nytt projekt, **48000 Hz** (Project Settings → Sample rate).
2. Dra in `signals/calib_pass.wav` som media-item på ett spår, vid t=0.
3. Lägg **Germanium 2000 Deluxe** (VST3) på spåret. Inga andra plugins på spår/master.
4. Sätt **Tape Speed** (19 / 9.5 / 4.75 cm/s) för den körning du vill mäta.
5. **File → Render:**
   - Sample rate **48000**, Channels **Stereo**
   - **WAV, 24-bit PCM** ⚠️ (INTE 32-bit float — `analyze.py` läser PCM)
   - Bounds: **Entire project** (hela 22,2 s)
   - Filnamn: `renders/out_19.wav` (resp. `out_95.wav`, `out_475.wav`)
6. Analysera:
   ```
   python3 analyze.py --render renders/out_19.wav --speed 19
   python3 analyze.py --render renders/out_95.wav --speed 9.5
   python3 analyze.py --render renders/out_475.wav --speed 4.75
   ```

## Fällor

- **Bit-djup:** rendera 24-bit PCM. 32-bit float misstolkas av läsaren.
- **Latens:** `analyze.py` align:ar automatiskt (envelope-korr, ±60 ms), så
  plugin-latens i Reaper är inget problem.
- **En hastighet per render:** sätt Tape Speed → rendera → analysera med matchande
  `--speed`. Filen är samma för alla tre.
- **Trösklar** ligger i `specs.json` (justerbara pass/fail-gränser).

## Baslinje (default-params, speed 19, via pedalboard — nuvarande bygge v0.63.0)

```
THD −20/−12/−6/0 dBFS : 0,13 / 0,43 / 1,24 / 3,79 %   (alla i spec)
S/N                   : 73,6 dB   (≥55)
Kanalseparation       : 59,0 dB   (≥45)
h2>h3 @ −12 dBFS       : h2 0,33 % > h3 0,28 %   (germanium-signatur OK)
```
