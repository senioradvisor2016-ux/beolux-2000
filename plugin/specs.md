# BC2000DL — Numeriska specifikationer (Fas 0 closure)

Mätbara mål härledda från B&O servicemanual + dansk användarmanual + 2N2613-databladet + komponentnivå-analys av alla 7 moduler. Plugin-DSP ska valideras mot dessa.

## 1. Audio I/O

| Parameter | Värde |
|---|---|
| Sample rates | 44.1 / 48 / 88.2 / 96 / 176.4 / 192 kHz |
| Bit depth (intern) | 32-bit float |
| Bit depth (in/ut) | 16/24/32 (DAW-styrt) |
| Latens | 0 samples (line out) / ~70 ms tape-monitor (utan Synchroplay) |
| Oversampling internal | 4× för bias-modell + tape-saturation |
| Channel count | Stereo in / Stereo ut |
| Plugin-format | VST3 + AU (+ AAX i v1.1) |

## 2. Hastigheter och tape-mekanik

| Hastighet | ips | Tape-bandbredd | Echo-delay | S/N-mål |
|---|---|---|---|---|
| 19 cm/s | 7½ | 30 Hz – 20 kHz (-3 dB) | 70–80 ms | >55 dB |
| 9.5 cm/s | 3¾ | 40 Hz – 12 kHz (-3 dB) | 140–160 ms | >50 dB |
| 4.75 cm/s | 1⅞ | 50 Hz – 6 kHz (-3 dB) | 280–320 ms | >45 dB |

## 3. Bias & erase

| Parameter | Värde |
|---|---|
| Bias-frekvens | 100 kHz (idealiserad sinus i plugin) |
| Bias-amplitud (default) | 2.3 mA-ekvivalent → optimal headroom |
| Bias-justeringsintervall | -50 % till +50 % från default |
| Erase-frekvens | 100 kHz |
| Erase-amplitud | 45 mA-ekvivalent |

## 4. Distorsion (THD)

| Stage | Test-villkor | Mål |
|---|---|---|
| Tape-saturation @ -3 dBFS in, 1 kHz | Default bias | <3 % THD |
| Tape-saturation @ 0 dBFS in, 1 kHz | Default bias | <8 % THD (mjuk knee) |
| Power-amp @ +14 dBu (motsvarar 5 W/4 Ω) | Speaker-mode | <1 % THD |
| Input-stage 2N2613 @ -3 dBFS | Default | 2:a-harmonik dominant över 3:e (PNP-asymmetri) |

## 5. Brusgolv

| Position | Mål |
|---|---|
| Mic-preamp 8904004 @ Mic-gain max | -60 dB FS (ekvivalent ~50 µV in-refererat) |
| Phono-preamp 8904002 @ H-läge | -65 dB FS |
| Radio-preamp 8904003 @ L-läge | -70 dB FS |
| Playback-amp 8004006 (3-stegs) | Dominerande brus i kedjan: -55 dB FS |
| Tape-noise (egen brus) | -60 dB FS @ 19 cm/s, -52 dB @ 4.75 cm/s |
| Total ut @ -inf dBFS in | >55 dB S/N (matchar specens 55 dB @ 19 cm/s) |

## 6. Frekvensgång (totalkedja, in→ut)

Mål: ±2 dB i specifierat band per hastighet (matchar manualens DIN-1962-mätning).

**19 cm/s:** flat 30 Hz–20 kHz ±2 dB, +3 dB resonans @ 5–8 kHz ("tape glow"), -3 dB @ 25 Hz, -3 dB @ 22 kHz
**9.5 cm/s:** flat 40 Hz–12 kHz ±2 dB, +5 dB @ 4–7 kHz, -3 dB @ 35 Hz, -3 dB @ 13 kHz
**4.75 cm/s:** flat 50 Hz–6 kHz ±3 dB, +6 dB @ 3–5 kHz, -3 dB @ 45 Hz, -3 dB @ 7 kHz

## 7. Wow & Flutter

| Hastighet | Wow (0.5–6 Hz) | Flutter (6–100 Hz) |
|---|---|---|
| 19 cm/s | 0.05–0.10 % | 0.05–0.08 % |
| 9.5 cm/s | 0.10–0.15 % | 0.08–0.12 % |
| 4.75 cm/s | 0.15–0.25 % | 0.12–0.20 % |

## 8. Kanalseparation

>45 dB enligt servicemanualen. Plugin: per-kanal-asymmetri i stereo (L/R) modelleras explicit (5–10 % parameteroffset på `kIs` och gain) — det är feature, inte bug.

## 9. Echo-funktioner

| Funktion | Plugin-beteende |
|---|---|
| Echo enable (knap 18) | Aktiverar feedback-loop PLAY → REC |
| Echo level (10a) | 0–100 %. Self-osc-tröskel @ ~85 % |
| Echo-tid | Auto från Speed: 70 ms / 140 ms / 280 ms |
| S-on-S | Stereomatrix: PLAY L → REC R |
| Multiplay | Bounce-loop med +1 dB brus/gen, HF-roll-off 10 → 8 → 6 kHz vid gen 1/2/3 (19 cm/s) |
| Synchroplay | Monitor-tap byts från PLAY-head till REC-head (0 ms tape-delay i monitor) |

## 10. UI-Inventory (26 kontroller)

Mappade direkt från manualens "Anvisning Side 1":

| # | Kontroll | Plugin-parameter | Range | Default |
|---|---|---|---|---|
| 1 | Speed selector | `Speed` | 4.75 / 9.5 / 19 cm/s + Off | 19 cm/s |
| 2 | Geartstang | `Transport` | Stop / Play / FF / REW / Pause | Stop |
| 3 | Counter | `TapeCounter` | 0–999 | 000 (resetbar) |
| 4 | Speaker EXT | `MonitorEXT` | bool | true |
| 5 | Speaker INT | `MonitorINT` | bool | false |
| 6 | Speaker mute | `SpeakerMute` | bool | false |
| 7 | Headphone jack | (ingen) | UI-only | — |
| 8 | Diskantkontrol | `Tone.Treble` | -12 ...+12 dB | 0 |
| 9 | Baskontrol | `Tone.Bass` | -12 ...+12 dB | 0 |
| 10 | Balance | `Balance` | L=-1 ... +1=R | 0 |
| 10a | Echo level | `EchoAmount` | 0...100 % | 0 |
| 11 | Momentanstop | `Pause` | bool | false |
| 12 | Master playback volume | `MasterVolume` | 0...100 % | 75 % |
| 13 | Radio recording level | `RadioBus.Gain` | 0...100 % | 0 |
| 14 | Phono recording level | `PhonoBus.Gain` | 0...100 % | 0 |
| 15 | Mic recording level | `MicBus.Gain` | 0...100 % | 0 |
| 16 | VU L | `MeterL` (output) | -40 ...+6 dBFS | — |
| 17 | VU R | `MeterR` (output) | -40 ...+6 dBFS | — |
| 18 | Echo button | `EchoEnabled` | bool | false |
| 19 | Track 1 (L) | `TrackSelect.L` | bool | true |
| 20 | Track 2 (R) | `TrackSelect.R` | bool | true |
| 21 | DIN connectors | (ingen) | UI-only | — |
| 22 | Monitor switch | `MonitorMode` | Source / Tape | Tape |
| 23 | Bypass tape (amp-alone) | `BypassTape` | bool | false |
| 24 | Record arm 1 | `RecordArm1` | bool | false |
| 25 | Record arm 2 | `RecordArm2` | bool | false |
| 26 | Synchroplay | `Synchroplay` | bool | false |

**Plus dolda parametrar (advanced tab):**

| Parameter | Range | Default |
|---|---|---|
| `BiasAmount` | -50%...+50% från nominell 2.3 mA | 0 |
| `TapeFormulation` | Agfa / BASF / Scotch / 1968-default | 1968-default |
| `WowFlutterAmount` | 0...200 % av spec | 100 % |
| `PrintThrough` | 0...5 % | 0 |
| `MicInputMode` | Lo-Z (50–200 Ω) / Hi-Z (500 kΩ) | Lo-Z |
| `PhonoMode` | H (RIAA) / L (keramic) | H |
| `RadioMode` | L (3 mV) / H (100 mV) | L |
| `StereoAsymmetry` | 0...100 % | 100 % (autentisk) |

## 11. Performance-mål

| Test | Mål |
|---|---|
| CPU-belastning | <5 % på M1 Pro @ 48 kHz / 128 samples / stereo |
| Latens (intern) | <2 ms (oversampling-pipeline) |
| RAM | <50 MB per instance |
| Plugin-validatorer | Auval clean (AU), Steinberg `validator` clean (VST3) |
| Hosts | Logic 11+, Live 12+, Reaper 7+, Pro Tools 2024+ |

## 12. 2N2613 / UW0029 / AC126 — DSP-konstanter

Härledda från databladet (2N2613) + funktionell motsvarighet (UW0029 = utvald lågbrus-PNP, AC126 = NPN motsvarighet).

```python
# Ebers-Moll-konstanter @ 25 °C
GE_VT_25C   = 0.02585     # V (kT/q vid 25 °C)
GE_IS_2N2613 = 1.0e-7    # A (saturation current, fittad mot transferkurva)
GE_IS_UW0029 = 0.7e-7    # A (lägre brus, lägre Is)
GE_IS_AC126  = 0.7e-7    # A (~UW0029-class)

# Brusparametrar
GE_NF_2N2613 = 4.0       # dB max NF @ 1 kHz, Rg=1 kΩ
GE_NF_UW0029 = 2.5       # dB typ (utvald lågbrus)
GE_NF_AC126  = 3.0       # dB typ (NPN low-noise audio)

# Input-refererat brus per stage @ 20 Hz–20 kHz
GE_NOISE_VRMS_2N2613 = 50e-6   # V RMS (databladet)
GE_NOISE_VRMS_UW0029 = 30e-6   # V RMS (lower NF)
GE_NOISE_VRMS_AC126  = 35e-6   # V RMS (estimat)

# Bandwidth (audio band aldrig begränsat av transistor)
GE_FT_HZ = 10e6          # 10 MHz typ för 2N2613/UW0029/AC126

# Asymmetri-bias för waveshaper
GE_ASYMMETRY_PNP = +0.10  # PNP biased mot positiv → 2:a-harmonics-dominans
GE_ASYMMETRY_NPN = -0.10  # NPN spegelvänd
```

## 13. DIN-1962 EQ-koefficienter (Fas 1 placeholder)

Per hastighet — fittas exakt från SPICE-simulering av 8004005 + 8004006-kretsen i Fas 1. För prototypen approximeras med 2-pol IIR-shelves:

```python
DIN1962_PLAYBACK_19_CM = {  # de-emphasis
    "lf_corner_hz": 50,
    "lf_boost_db": 12,
    "hf_corner_hz": 12000,
    "hf_cut_db": -2,
}
DIN1962_PLAYBACK_95_CM = {
    "lf_corner_hz": 50, "lf_boost_db": 14,
    "hf_corner_hz": 7500, "hf_cut_db": -3,
}
DIN1962_PLAYBACK_475_CM = {
    "lf_corner_hz": 50, "lf_boost_db": 16,
    "hf_corner_hz": 4000, "hf_cut_db": -4,
}

DIN1962_RECORD_19_CM = {  # pre-emphasis
    "hf_boost_corner_hz": 15000,
    "hf_boost_db": 6,
}
DIN1962_RECORD_95_CM = {
    "hf_boost_corner_hz": 9000,
    "hf_boost_db": 9,
}
DIN1962_RECORD_475_CM = {
    "hf_boost_corner_hz": 5000,
    "hf_boost_db": 12,
}
```

**Kritiskt:** Pre+de-emphasis är **inte** exakta inverser. Mismatchen i 3–8 kHz ger "tape glow". Renormalisera inte bort.

## 14. Validerings-protokoll

**Per-block (Fas 1, automatiserat):**
1. Sweep 20 Hz–20 kHz @ -20 dBFS → frekvensrespons inom ±1 dB av §6
2. Sinus 1 kHz vid -20/-12/-6/0 dBFS → THD enligt §4
3. Pink noise → S/N enligt §5
4. Wow/flutter med Doerfler-mätare (Python-impl): ≤ §7
5. 2:a-harmonik > 3:e-harmonik vid -3 dBFS in (asymmetri-check)

**Plugin-validering (Fas 4):**
- `auval -v aufx <code> <manuf>` clean
- Steinberg `validator` clean
- Inläsning utan glitch i Logic 11, Live 12, Reaper 7, Pro Tools 2024+
- Realtidsbelastning <5 % CPU @ M1 Pro / 48 kHz / 128 samples / stereo

**Subjektivt (Fas 3):**
- A/B mot referensinspelningar gjorda på riktig BeoCord 2000 DL — minst 5 lyssnare ska identifiera "samma karaktär"
