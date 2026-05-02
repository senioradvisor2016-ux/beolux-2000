# BC2000DL — UX Designer Brief (v3 — SINGLE-VIEW ALL-IN-ONE)

**Projekt:** VST3/AU plugin som emulerar Bang & Olufsen BeoCord 2000 De Luxe (1968)
**Målgrupp:** Music producers, mixing engineers, sound designers
**Plattform:** macOS (Logic, Pro Tools, Live, Reaper) + Windows (FL, Cubase)
**Fönsterstorlek:** 1300×500 px (kan utökas till 1500×550 vid behov)
**Status:** Funktionell prototyp finns ([`plugin/juce/`](juce/)) — UX behöver visuell upgradering till "fotorealistisk Hybrid Hero"-nivå

---

## ⚡ DESIGNPRINCIP — ALLT VISAS PÅ EN YTA

**Inga flikar, inga modala dialoger, inga "advanced"-paneler som glöms.** Alla 31 kontroller är **synliga och åtkomliga direkt** från första sekund. Användaren ska aldrig behöva leta.

**Konsekvens:** UI:n blir tätt packad men varje element har sin plats. Layout-strategin är **3 funktionella zoner + tape-deck** istället för flera vyer.

---

## 1. Designspråk

**Estetisk identitet:** Jacob Jensen 1968, dansk modernism, B&O-DNA.

**Tre nyckelord:**
- **Minimalism** — negativ rymd är ett designelement
- **Materialkontrast** — varm teak vs. kall mattsvart vs. kall brushed silver
- **Funktionell skönhet** — varje element har en funktion, ingen dekoration

**Tre förbjudna ord:**
- "Modern" UI-trender (rounded glassmorphism, neumorphism)
- "Skeuomorphism" som blir överdriven (limmade trä-textur, fake-skuggor)
- "Cluttered" — om något ser fullt ut är det för mycket

---

## 2. Originalbilder — visuell referens

**Använd dessa som primär källa:**

1. **`/tmp/auctionet_large_item_4447864_aaaa50174e.jpg`** — top-down vy av faktisk BeoCord 2000 DL
2. **`/tmp/auctionet_large_item_4447864_9ea511348c.jpg`** — 3/4-vy med träram
3. **`/tmp/auctionet_large_item_4447864_4c72216145.jpg`** — close-up tape-deck + B&O-logo
4. **`/tmp/anvisning_side1.jpg`** — manualens 26 numrerade kontroller
5. **DBA / Auctionet** — sök "BeoCord 2000 De Luxe" för fler vinklar

**Kontextuell inspiration** (publika foton):
- B&O Museum Struer (Danmark)
- Vintage HiFi-bloggar
- Jacob Jensen retrospektiv-böcker

---

## 3. Materialpalett (exakta hex-värden)

```
TRÄ:
#9C5F35  Teak (huvudfärg på cabinet)
#7A4722  Teak Dark (skuggor, kant)
#B07845  Teak Light (highlight, träfiber-ljus)

PANEL:
#1F1F1F  Anodized Alu (frontpanel)
#141414  Alu Dark (skuggor, embossed)
#2C2C2C  Alu Light (gradient toppen)

METALL:
#B5B5B5  Brushed Silver (tape-deck-yta, slider-toppar)
#D0D0D0  Silver Highlight
#A8A8A8  Push-button satin

VU & ACCENT:
#0A0A0A  VU-glas svart (modern)
#F0E8D8  VU-glas cream (klassisk alternativ)
#C42820  Record red / VU HIGH-zon (B&O-röd)

BAS:
#080808  Plinth (svart bas under cabinet)

TAPE:
#8A4A35  BASF brun-röd (oxiderad tape)
#6E3D2A  AGFA mörkbrun (oxiderad tape)
#C02828  BASF röd label
#2860A0  AGFA blå label
```

---

## 4. Typografi

**Familj:** Helvetica Neue / Univers Light (B&O använde Univers 1968)

**Storlekar:**
- Logo "BC2000DL · DANISH TAPE 2000": 14pt, bold, letter-spacing +0.5em, silver embossed
- Knapp-text: 9pt, bold, versaler
- Skala-numbers (0–10): 7pt, regular
- Rotary-labels (TREBLE/BASS/...): 8pt, bold, versaler
- Tape-counter siffror: 12pt, bold, sans-serif

---

## 5. Layout — single-view (1300×500 px)

```
┌──────────────────────────────────────────────────────────────────────────┐
│  TEAK FRAME (28 px) — radial gradient                                     │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │  TAPE-DECK ZONE (brushed silver, 30% av panel-höjd ≈ 140px)        │ │
│  │                                                                     │ │
│  │     ┌─────────┐         ┌─────────┐         ┌─────────┐           │ │
│  │     │ SPOLE L │         │  HEAD   │         │ SPOLE R │           │ │
│  │     │ BASF-   │         │  COVER  │         │ AGFA-   │           │ │
│  │     │ röd     │         │   ♛     │         │ blå     │           │ │
│  │     └─────────┘         └─────────┘         └─────────┘           │ │
│  │                                                                     │ │
│  │       [BC2000DL  ·  DANISH TAPE 2000]   silver embossed            │ │
│  ├────────────────────────────────────────────────────────────────────┤ │
│  │  CONTROL PANEL (svart anodiserad, ~290px)                          │ │
│  │                                                                     │ │
│  │  ┌──────────────┐  ┌─────────────┐  ┌──────────────────────────┐  │ │
│  │  │              │  │             │  │                           │  │ │
│  │  │   ZON A      │  │   ZON B     │  │   ZON C                   │  │ │
│  │  │  (340px)     │  │  (380px)    │  │  (~400px)                 │  │ │
│  │  │              │  │             │  │                           │  │ │
│  │  │  Push-grupp  │  │  5 dual-    │  │  Rotary-rad 1: TREBLE BASS│  │ │
│  │  │  2×4         │  │  faders     │  │                           │  │ │
│  │  │              │  │             │  │  Rotary-rad 2: BAL VOL    │  │ │
│  │  │  VU L  VU R  │  │  ♭ ○ ♂ S ◁ │  │                           │  │ │
│  │  │              │  │             │  │  Rotary-rad 3: BIAS MULT  │  │ │
│  │  │  Push-rad 3  │  │  Skala 0-10 │  │                           │  │ │
│  │  │  (advanced)  │  │  vänster    │  │  ‹‹ Y ››   ▢▢▢            │  │ │
│  │  │  Push-rad 4  │  │  per slits  │  │  Lever     Counter        │  │ │
│  │  │  (modes)     │  │             │  │                           │  │ │
│  │  │              │  │             │  │                           │  │ │
│  │  │  Combo-rad 1 │  │             │  │                           │  │ │
│  │  │  Combo-rad 2 │  │             │  │                           │  │ │
│  │  └──────────────┘  └─────────────┘  └──────────────────────────┘  │ │
│  └────────────────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────────────────┘
```

---

## 6. Komplett kontroll-inventering — alla 31 element synliga

### TAPE-DECK ZONE (5 element, alltid synliga)

| # | Element | Position | Funktion |
|---|---|---|---|
| 1 | Spole L (animerad) | Vänster, deck-zone mitten | Roterar vid input >0 |
| 2 | Spole R (animerad) | Höger, deck-zone mitten | Roterar vid input >0 |
| 3 | Tape-head-cover | Center, deck-zone | Statisk svart rektangel + ♛ |
| 4 | "BC2000DL · DANISH TAPE 2000"-logo | Mellan deck och panel | Silver embossed |
| 5 | Topp-trim (röd 3px) | Toppen av cabinet | B&O-accent |

### ZON A — Vänster (340 px, 16 element)

**Push-grupp 1 (2 rader × 4 knappar — kompakt, ovanför VU):**

| # | Knapp | Manual-knap | Funktion |
|---|---|---|---|
| 6 | ECHO | #18 | Echo enable |
| 7 | REC 1 | #24 | Record arm spår 1 |
| 8 | REC 2 | #25 | Record arm spår 2 |
| 9 | SYNC | #26 | Synchroplay |
| 10 | BYPASS | #23 | Amp-alone (bypass tape) |
| 11 | TRK 1 | #19 | Monitor track 1 |
| 12 | TRK 2 | #20 | Monitor track 2 |
| 13 | S/S | — | Sound-on-Sound enable |

**VU-mätare (under push-grupp):**

| # | Element | Funktion |
|---|---|---|
| 14 | VU L (#16) | 300 ms ballistik, atomic feed |
| 15 | VU R (#17) | 300 ms ballistik, atomic feed |

**Push-rad 3 (sekundära, advanced):**

| # | Knapp | Funktion |
|---|---|---|
| 16 | SPEAKER | Power-amp engagerad |
| 17 | MUTE | Speaker mute (#6) |
| 18 | PAUSE | Momentanstop (#11) |
| 19 | P.A. | Public Address (duckar phono+radio) |

**Push-rad 4 (modes):**

| # | Knapp | Funktion |
|---|---|---|
| 20 | SPK A | Speaker EXT (#4) |
| 21 | SPK B | Speaker INT (#5) |
| 22 | Lo-Z | Mic input mode |

**Combo-rader:**

| # | Combo | Funktion |
|---|---|---|
| 23 | SPEED (4-pos) | 4.75 / 9.5 / 19 cm/s + (off, dolt) — knap #1 |
| 24 | MONITOR (Source/Tape) | knap #22 |
| 25 | PHONO mode (H/L) | RIAA on/off |
| 26 | RADIO mode (L/H) | Sensitivity |

### ZON B — Mitten (380 px, 5 dual-faders)

**5 dual-slide-faders i ordning vänster→höger (matchar B&O-original):**

| # | Symbol | Knap | Funktion |
|---|---|---|---|
| 27 | ♭ (Radio) | #13 | L+R input gain |
| 28 | ○ (Phono/Grammofon) | #14 | L+R input gain |
| 29 | ♂ (Mic) | #15 | L+R input gain |
| 30 | S (S-on-S/Saturation) | — | Saturation drive L+R |
| 31 | ◁ (Echo) | #10a | Echo amount L+R |

**Skala 0–10 vänster om varje slits.** Symboler ovan slitsen.

### ZON C — Höger (~400 px, 12 element)

**Rotary i 3 rader × 2:**

| # | Rotary | Manual-knap | Funktion |
|---|---|---|---|
| 32 | TREBLE | #8 | -12...+12 dB |
| 33 | BASS | #9 | -12...+12 dB |
| 34 | BAL | #10 | Stereo balance |
| 35 | VOL | #12 | Master volume |
| 36 | BIAS | (advanced) | Tape-bias 0.5–1.5 |
| 37 | MULT | (advanced) | Multiplay-gen 1–5 |

**Transport + counter:**

| # | Element | Funktion |
|---|---|---|
| 38 | Transport-lever (Y-form) | knap #2 (5-state) |
| 39 | Tape-counter (3-digit mekanisk) | knap #3 |

---

## 7. Komponentdetaljer

### 7.1 Push-knapp (toggle)
```
Storlek:    56×22 px
Bg neutral: #A8A8A8 (satin grå)
Bg active:  #C42820 (B&O-röd)
Text:       9pt bold versaler, vit på röd / svart på grå
Skugga:     1px nedanför vid neutral
Pressed:    rörs 2px neråt
```

### 7.2 Slide-fader (DUAL — kritisk!)
```
Slits-bredd:    75 px
Slits-höjd:     180–220 px
Spår:           mörk grå/svart, 6px bred
Knopp L:        #D0D0D0 (ljusare silver), 25×12 px, 3 grip-lines
Knopp R:        #B5B5B5 (standard silver), 25×12 px
Skala:          0–10 numbers vänster
Symbol:         OVAN slits, 10pt bold (♭/○/♂/S/◁)
```

### 7.3 Rotary
```
Storlek:        50×50 px (utan label)
Knab:           #A8A8A8 satin
Pekare:         liten svart prick på yttre kant
Rotation:       225° (mid = 12)
Label:          8pt bold versaler under
Drag:           vertical (upp = +)
```

### 7.4 VU-mätare
```
Storlek:        110×52 px
Glas:           svart (#1F1F1F)
Ram:            #B5B5B5 silver
Skala:          10 vita ticks längs botten
Röd HIGH-zon:   röd streck längs sista 30%
L/R-label:      hörn topp-vänster, 9pt bold silver
Nål:            vit silver, 1.4px tjock
Ballistik:      300 ms attack/release
Record-glow:    röd halo runt VU vid record-arm
```

### 7.5 Tape-counter (mekanisk)
```
Storlek:        80×24 px
3 separata digit-boxes med 1px gap
Bg:             #080808
Siffra:         12pt bold sans-serif, vit silver
Animation:      "rullar" upp/ner vid räkning
```

### 7.6 Transport-lever (Y-form)
```
Storlek:        140×40 px
‹‹ pil:         14pt bold silver vänster
›› pil:         14pt bold silver höger
Y-skaft:        4px tjocka silver-linjer
Kula på topp:   silver, 10×10, röd vid PLAY
States:         STOP / PLAY / FF / REW
Klick:          mid = play/stop, vänster = REW, höger = FF
```

### 7.7 Spole (animerad)
```
Yttre cirkel:   klart-vit, lätt transparent
Tape-band:      brun ring mellan hub och yttre kant
Label:          BASF röd / AGFA blå (cirkulär)
Hub:            svart liten cirkel
Ekrar:          3 tunna silver-streck
Rotation:       aktiv när någon input-fader > 0
                snabbare vid 19 cm/s
                reverse vid REW
```

### 7.8 Combo-väljare
```
Storlek:        110×22 px
Bg:             #2C2C2C (mörk grå)
Text:           silver, 9pt
Pil:            silver triangel höger
Hover:          subtle glow
Dropdown:       svart bakgrund, silver text, röd highlight på vald
```

---

## 8. Interaktioner & Animationer

### 8.1 Hover-states
- Push-knappar: lätt highlight (+10% lightness)
- Rotary/fader: cursor → hand
- Combo: pil-färg ljusnar

### 8.2 Click-feedback
- Push: 2px neråt, skugga döljs
- Toggle ON: röd fyllning fade 50ms
- Toggle OFF: tillbaka till satin

### 8.3 Drag-states
- Faders: real-time DSP-update
- Rotary: vertical drag (sensitivity 0.5%/px)
- Modifier: Cmd/Ctrl-drag = 0.1%/px (fine-grain)

### 8.4 State-animationer
- Speed-byte: subtle färg-flash på combo (~200ms)
- Record-arm aktiv: VU får röd halo (fade-in 200ms)
- Echo aktiv: subtle pulse på echo-fader (1.5 Hz, 5%)
- Spol-rotation: hastighetsberoende RPM, reverse vid REW

---

## 9. Asset-deliverables för designer

### 9.1 Figma-fil med:
- Komplett 1300×500 layout (single-view, alla 31 kontroller)
- Alla 8 komponent-typer som components (variants för states)
- Color tokens
- Typography tokens
- Annotations på interaktioner

### 9.2 Asset-export:
- SVG för vektor-grafik (logo, ikoner, fader-symboler ♭○♂S◁)
- PNG @2x för raster-element (träfiber, BASF/AGFA-labels)
- JSON för animation-tweens (Lottie om det används)

### 9.3 State-screenshots (PNG @2x):
1. **Idle** — alla faders 0, ingen aktivitet
2. **Playing** — speed=19, mic-fader 70%, transport=play
3. **Recording** — REC 1 active, röd VU-glow
4. **Echo on** — echo-knapp + echo-fader 60%
5. **Bypass** — bypass-knapp aktiv, tape-zon dimmed
6. **Speaker mode** — speaker-knapp aktiv
7. **PA mode** — PA-knapp aktiv, mic uppvriden, phono dimmed

### 9.4 Hover/click-prototyper
För minst 4 huvudkontroller (transport, fader, push-button, rotary).

---

## 10. Förbjudet / undvik

- ❌ B&O-logo, "BeoCord"-namn (varumärkesintrång)
- ❌ UAD/Waves/Slate-style copy
- ❌ Onödig dekoration (skuggor på skuggor, gloss på allt)
- ❌ Modernt mjukt UI (glasmorphism, rounded everything)
- ❌ Färger utanför paletten ovan
- ❌ Typografi-mix (bara Helvetica/Univers-familjen)
- ❌ Animationer >300ms
- ❌ **Tabs eller dolda paneler** — allt synligt på en yta

---

## 11. Inspiration (juridiskt rena)

- Foton av riktig BeoCord 2000 DL (auctionet, dba.dk, ebay-vintage)
- Jacob Jensen-böcker
- B&O Museum Struer 1960–1980-arkiv
- Studer A800 hardware-foton (samma estetik-era)
- Designmuseum Danmark — modernistisk industri

---

## 12. Tidsestimat

**Optimal:** 5–8 dagar för senior UX-designer med audio-plugin-erfarenhet.
**Minimum:** 3 dagar (Figma-mockup + 7 state-screenshots).

**Budget:**
- Figma master: ~16 timmar
- Asset-export & cleanup: ~4 timmar
- Iteration efter dev-feedback: ~8 timmar

**Total:** ~28 timmar.

---

## 13. Baseline (vad nuvarande har)

Min nuvarande implementation finns på:
- [`plugin/juce/Source/ui/HybridHeroPanel.cpp`](juce/Source/ui/HybridHeroPanel.cpp) — träram + tape-deck + spolar
- [`plugin/juce/Source/ui/BCLookAndFeel.cpp`](juce/Source/ui/BCLookAndFeel.cpp) — kontrollrendering
- [`plugin/juce/Source/ui/VUMeter.cpp`](juce/Source/ui/VUMeter.cpp) — atomic VU
- [`plugin/juce/Source/ui/DualSlideFader.cpp`](juce/Source/ui/DualSlideFader.cpp) — dubla-knopp-faders
- [`plugin/juce/Source/ui/TransportLever.cpp`](juce/Source/ui/TransportLever.cpp) — Y-form + counter

**Kvalitetsnivå nu:** funktionell prototyp, vektor-baserad.

**Mål för designer:** lyfta till "fotorealistisk Hybrid Hero" — single-view med subtila gradienter, korrekta material-textures, hand-tweaked detaljer, **alla 31 kontroller synliga utan scroll/tabs**.

---

## Kontakt

För frågor om DSP/funktionalitet:
- Plan-fil: [`~/.claude/plans/analysera-hela-appens-filer-purring-toucan.md`](../../.claude/plans/analysera-hela-appens-filer-purring-toucan.md)
- Specs: [`plugin/specs.md`](specs.md)
- Manualen: `/tmp/beocord-2000-instruktionsbog-dansk.pdf` (B&O 1968)
