# BC2000DL — Figma UX-prompt

> Prompt designad för Figma AI / Figma Make. Klipp och klistra hela
> dokumentet vid första iterationen. Texten är medvetet konkret —
> mått, hex-värden, typsnitt och komponentnamn — så outputen blir
> direkt-byggbar i JUCE/Skia istället för stylade mockups.

---

## 1. Brief

Designa frontpanelen till en **VST3/AU-pluginemulering av en
Bang & Olufsen BeoCord 2000 De Luxe (1967–69)** — en 3-hastighets
stereo-rullbandspelare med germaniumtransistorer, separata
record/playback-huvuden, 100 kHz bias och DIN-1962-EQ.

Produktnamn: **BC2000DL — Danish Tape 2000**.
Inte affilierat med B&O. Får inte använda B&O-logo eller "BeoCord".

Designspråken är **"Hybrid Hero"**: foto-realistiska material
(teakträ, brushed silver, anodiserad aluminium, VU-glas) i
bakgrunden; skarpa vektor-kontroller (skalbara på HiDPI) ovanpå.
Estetiken är **dansk, hifi 1968, Jensen-skola** — lugn, symmetrisk,
inga onödiga visuella ornament. **Inte retro-pastisch** — det här
är ett professionellt instrument med 1968-rötter.

Målgrupp: producenter och mixingenjörer i Logic Pro, Pro Tools,
Ableton Live, Reaper. Pluginen ska kännas **lika trovärdig som UAD,
Soundtoys, Plugin Alliance** — inte som en bedroom-GitHub-utgåva.

---

## 2. Frame och proportioner

- **Canvas**: 1340 × 640 px (1×). Måste skala 0.75× till 2× utan
  layout-omberäkning (auto-layout med fasta sektionsbredder).
- **Resolution**: 1×, 1.5×, 2× export (PNG + SVG för vektorer).

Ramverk inifrån och ut:

| Lager | Höjd / Bredd | Material |
|---|---|---|
| Yttre teak-ram | 22 px (alla sidor) | Teak träfiber, gradient `#9C5F35 → #B07845` med radial-light |
| Topp-trim | 1 px topp | B&O-röd `#C42820` (signaturlinje) |
| Header-bar | 44 px topp | Anodiserad svart `#141414` |
| Alu-frontpanel | resten | Anodiserad svart `#1F1F1F → #2C2C2C` (top-light gradient) |
| Status-bar | 26 px botten | Samma som header `#141414` |

**Innesluten kontroll-yta** (alu-panelen) delas i tre vertikala sektioner:

| Sektion | Bredd | Innehåll |
|---|---|---|
| INPUT · MIXER | 360 px | Source-faders + input-mode-selectors |
| TAPE · TRANSPORT | 420 px | VU-meters + transport + tape-deck-grafik + tape-parametrar |
| OUTPUT · TONE | 420 px | Tone-rotaries + master + speaker-routing |

Sektioner separeras av **mycket subtila vertikala silver-linjer**
(1 px, opacitet 10 %). Inga ramar runt sektioner.

---

## 3. Header-bar (1340 × 44, top)

Bakgrund: `#141414`.
Höger 1 px topp-line i `#C42820`.

| Område | Position | Innehåll |
|---|---|---|
| Vänster | 32 → 350 px | Logo "BC2000DL" 15 pt Helvetica Bold, silver `#D0D0D0` + " · DANISH TAPE 2000" 10 pt regular, 65 % silver |
| Center | 360 → 800 px | Preset-navigation: `<` button (24×22) — **PresetMenu** ComboBox (380×22) — `>` button. Text: "Manual #1 — Mono Mic 1968" |
| Höger | 1240 → 1308 px | A/B-compare-knappar: "A" (36×22, kan vara röd `#C42820` när vald), "B" (36×22, samma). 4 px gutter |

Preset-meny innehåller:
- "— Init —"
- "Manual #1 — Mono Mic 1968"
- "Manual #2 — Mono Mic + Echo"
- "Manual #5 — Stereo Mic"
- "Manual #7 — FM Radio + Echo"
- "Manual #15 — Mono Phono"
- "Manual #22 — Slap Echo (19 cm/s)"
- "Bus — Drum Crush"
- "Bus — Vocal Warmth"
- "Bus — Lo-fi (4.75 cm/s)"

---

## 4. Tape-deck (TAPE · TRANSPORT-sektionens topp, ~28 % av panelhöjden)

Brushed silver `#B5B5B5` med horisontella grain-linjer (2 px spacing,
35 % alpha) över hela sektions-bredden.

I mitten två **stora spolar** (radie ~70 px):
- Vänster spole: BASF-röd label-cirkel `#C02828`, brun tape `#8A4A35`
- Höger spole: AGFA-blå label-cirkel `#2860A0`, mörkbrun tape `#6E3D2A`
- Yttre transparent plast-disc med tunn mörk kant
- Tape-band visualiseras som koncentriska elliptiska ringar
- 3 svarta ekrar i hub (animerade — roterar när transport = PLAY)
- Centerlogo: silver "♛"-krona på liten svart label-bricka
  (stiliserad B&O-arv utan att bryta varumärkesregler)

Mellan spolarna: liten **head-cover-rektangel** 40×60 px,
mörk alu `#141414`, rundad 2 px, med "♛"-symbol i 14 pt Helvetica.

---

## 5. INPUT · MIXER-sektion (vänster, 360 px)

Sektions-rubrik högst upp: `INPUT · MIXER` 9.5 pt Helvetica Bold,
silver 55 % alpha, vänsterjusterad.

### 5a. Fem dual-slide-faders (B&O skydepotentiometre)

Layout: 5 vertikala faders sida vid sida.

| Fader | Etikett ovanför | Funktion |
|---|---|---|
| 1 | RADIO | Radio-input gain (L+R som dubbla knoppar) |
| 2 | PHONO | Grammofon-input gain |
| 3 | MIC   | Mikrofon-input gain |
| 4 (gutter 12 px) | SAT | Tape-saturation drive |
| 5 | ECHO | Echo-amount |

Varje fader är 64 px bred × 200 px hög och innehåller:
- Etikett ovanför (RADIO, PHONO, etc) — 10 pt Helvetica Bold,
  silver `#D0D0D0`
- **Gemensam track** i mitten: 6 px bred, ljus alu `#2C2C2C`,
  rundad 2 px
- **Två knoppar** i samma track:
  - L-knopp (vänster halva, 27 px bred, 12 px hög, ljus silver
    `#D0D0D0`) med 3 horisontella grip-lines
  - R-knopp (höger halva, mörkare silver `#B5B5B5`)
  - "L"/"R"-mikrobokstav 7 pt bredvid varje knopp
- **Gradering 0–10 på vänster sida** — små streckmarkörer + siffror

### 5b. Mode-selectors (under faders)

Två rader med ComboBox:

```
[ PHONO INPUT ]    [ RADIO INPUT ]
[ L (ceramic)  v]  [ L (3 mV)    v]    ← 160 × 22 px varje
```

Etikett ovanför 10 px, ComboBox 22 px hög, anodiserad alu
`#2C2C2C` med silver border 1 px och text `#D0D0D0` 10 pt.

### 5c. Tre toggle-buttons under

```
[ Lo-Z ] [ P.A. ] [ S/S ]      ← 56×22 px varje, gutter 4 px
```

Toggle-buttons:
- Off-state: `#2C2C2C` bg, silver text `#B5B5B5`
- On-state: `#C42820` (B&O-röd) bg, vit text
- Border 0.5 px alu-mörk när off, ingen när on
- Tryckanimation: 1 px y-offset på pressed

---

## 6. TAPE · TRANSPORT-sektion (center, 420 px)

Sektions-rubrik: `TAPE · TRANSPORT` 9.5 pt Helvetica Bold, silver 55 %, centrerad.

### 6a. Topprad: VU-meters + transport + tape-counter

Layout horisontell:

```
[ VU L ]  [ VU R ]   [ TRANSPORT ]   [ COUNTER ]
130×60     130×60      110×60          70×60
```

**VU-meter (130 × 60 px)**:
- Glas-bakgrund: cream `#F0E8D8` med 70 % gradient → `#E0D7C2`
  i nedre kant
- Skala 0..7 dB (svart område), 7..+3 dB (röd zon `#C42820`)
- Nål: svart, mekanisk-fysisk ballistik (300 ms attack/release —
  beskriv i animations-anteckningar nedan)
- "VU" och kanalnamn ("L"/"R") i 8 pt Helvetica i nedre vänster hörn
- **Röd record-belysning bakgrund** när record-arm är aktiv
  (subtila röda highlights bakom glaset, alpha 30 %)

**Transport-lever (110 × 60 px)**:
- 4-positions mekanisk lever: REW, STOP, PLAY, FF
- Gjuten metall-stil med fysisk skugga
- Position-indikator: liten LED `#C42820` när PLAY/FF/REW

**Tape-counter (70 × 60 px)**:
- Mekaniskt 4-siffrigt mätar-fönster (svart bakgrund, vit/silver
  Univers Mono-text, 14 pt)
- Räknar "0000" → "9999" och rullar (animerad numerisk rullning
  när transport = PLAY)
- "× 100"-text under i 7 pt

### 6b. Tre ComboBoxar i rad

```
[ TAPE SPEED ] [ MONITOR    ] [ TAPE FORMULA ]
[ 19 cm/s   v] [ Tape       v] [ Agfa         v]
   120×22 var, gutter 8 px
```

### 6c. Två rotary-rattar

```
[ BIAS ]   [ MULT ]
70×80      70×80
```

Rotary-rattar:
- Diameter 60 px, mörkt brushed silver `#A8A8A8`
- 11 graderingar (0–10) som små vit-silver streck runt utsidan
- Pekare: röd `#C42820` 2 px linje från centrum till kant
- Etikett ovanför: 8.5 pt Helvetica Bold silver
- Talrutan under (50×12, anodiserad svart med vit text 9 pt)

### 6d. Tape-knappar (2 rader × 5 knappar)

```
[REC 1] [REC 2] [TRK 1] [TRK 2] [PAUSE]
[ECHO ] [SYNC ] [BYPASS]
56×22 each, gutter 3 px
```

REC 1/2: när armed → röd bg + röd punkt-indikator
TRK 1/2: när active → silver-light bg
ECHO/SYNC/BYPASS: röd när aktiv

---

## 7. OUTPUT · TONE-sektion (höger, 420 px)

Sektions-rubrik: `OUTPUT · TONE` 9.5 pt Helvetica Bold, silver 55 %, högerjusterad.

### 7a. Fyra rotary-rattar i rad (TREBLE, BASS, BAL, VOL)

```
[ TREBLE ] [ BASS ]   [ BAL ]    [ VOL ]
70×80       70×80      70×80     70×80
```

Samma styling som BIAS/MULT-knapparna i centrum-sektionen.

VOL-knappen är **lite större** (74 px) och har en silver-rim
runt sig — den är "hero"-knappen.

### 7b. Speaker-routing-knappar

```
[ SPK A ] [ SPK B ] [ MUTE ] [ SPEAKER ]
56×22 each
```

SPEAKER-knappen aktiverar AD139 power-amp-modellen. När den är
av: ren line-ut. När på: visuell glow under knappen.

---

## 8. Status-bar (1340 × 26, botten)

Bakgrund: `#141414`.

| Område | Position | Innehåll |
|---|---|---|
| Vänster | 32 px in | "Independent emulation. Not affiliated with Bang & Olufsen A/S." 9.5 pt Helvetica, silver 70 % alpha |
| Höger | 32 px in från höger | "19 cm/s · 48.0 kHz" — speed + sample-rate, 9.5 pt silver 60 % alpha |

---

## 9. Komponentbibliotek (Figma Components)

Skapa följande som **återanvändbara komponenter med variants**:

### 9.1 `Slider/Vertical`
Variants: `state=default | hover | dragging`, `position=0..100 (slider)`
Ovärldsligt enkel — bara visuell. Ingen interaktivitet.

### 9.2 `Slider/Dual` (B&O skydepotentiometer)
Två `Slider/Vertical` sida vid sida + gemensam track-bg + label-prop.
Props: `label`, `leftValue`, `rightValue`.

### 9.3 `Knob/Rotary`
Variants: `size=small (60px) | hero (74px)`, `value=0..100`
Props: `label`, `unit`, `value`.

### 9.4 `Button/Toggle`
Variants: `state=off | on`, `width=auto | wide`
Props: `label`.
Klippt-rektangel 22 px hög, padding 8/4.

### 9.5 `ComboBox/Dropdown`
Variants: `state=closed | open | hover`
Props: `label` (extern, 10 px ovanför), `value`, `items[]`.

### 9.6 `VU-Meter`
Variants: `channel=L | R`, `level=0..100`, `recording=false | true`
Props: `label`.
Använd Figma "Smart Animate" för nålrörelse.

### 9.7 `TapeReel`
Variants: `side=left | right`, `loaded=true | false`,
`rotating=false | true`
Props: `tapeType=BASF | AGFA | Maxell`.

### 9.8 `TransportLever`
Variants: `position=REW | STOP | PLAY | FF`.

### 9.9 `Section/Header` (3 sektionsrubriker)
Props: `text`, `align=left | center | right`.

---

## 10. Färg-palett (skapa Figma color styles)

```
Tokens:
─ Wood/teak-dark        #7A4722
─ Wood/teak             #9C5F35
─ Wood/teak-light       #B07845

─ Metal/alu-dark        #141414
─ Metal/alu             #1F1F1F
─ Metal/alu-light       #2C2C2C

─ Metal/silver          #B5B5B5
─ Metal/silver-light    #D0D0D0
─ Metal/satin           #A8A8A8

─ Glass/vu              #F0E8D8
─ Glass/vu-darker       #E0D7C2

─ Accent/red            #C42820   ← B&O-signaturröd, sparsamt!
─ Accent/red-dim        #8B1C16   ← hover/pressed för red

─ Type/ink              #1A1A1A
─ Type/paper            #F4EFE4   (cream)
─ Type/soft-gray        #9A9A9A
─ Type/line-gray        #C8C0B0
```

**Användningsregel**: röd `#C42820` är reserverad för:
1. Topp-trim-linje (1 px)
2. Active state för record/echo/sync-knappar
3. VU-meter "overload"-zon (>+3 dB)
4. A/B-compare aktiv slot
5. Pekarens linje på rotaries

Inget annat ska vara rött. Den ska vara sällsynt och därmed
visuellt kraftig.

---

## 11. Typografi

| Style | Font | Size | Weight | Color | Usage |
|---|---|---|---|---|---|
| Logo-Mark | Helvetica | 15 pt | Bold | silver-light | Header logotyp |
| Logo-Subtitle | Helvetica | 10 pt | Regular | silver 65 % | "DANISH TAPE 2000" |
| Section-Header | Helvetica | 9.5 pt | Bold | silver 55 % | "INPUT · MIXER" |
| Control-Label | Helvetica | 8.5 pt | Bold | silver-light | "TREBLE", "BIAS" |
| Combo-Label | Helvetica | 8.5 pt | Bold | silver-light | "TAPE SPEED" ovanför ComboBox |
| Fader-Label | Helvetica | 10 pt | Bold | silver-light | "RADIO", "MIC" ovanför fader |
| Button-Label | Helvetica | 10 pt | Medium | silver / vit | Toggle-knappar |
| VU-Label | Helvetica | 8 pt | Regular | ink | "VU", "L"/"R" |
| Counter | Helvetica Mono | 14 pt | Regular | silver | "0042" tape-counter |
| Status | Helvetica | 9.5 pt | Regular | silver 70 % | Footer-text |
| Tooltip | Helvetica | 11 pt | Regular | ink på paper-bg | Hover-tooltips |

Helvetica används överallt. Om Figma inte har Helvetica → fallback
**Inter Tight**. Aldrig Roboto, aldrig San Francisco UI, aldrig
Arial — det förstör 1968-Jensen-känslan.

---

## 12. Animationer & micro-interactions

| Element | Animation |
|---|---|
| Spolar | Roterar konstant 0.06 rad/frame (≈ 17 RPM @ 30 fps) när TRANSPORT=PLAY/FF/REW. Riktning byter vid REW. |
| Tape-counter | Räknar upp 1 enhet/sec när PLAY |
| VU-meter-nål | Easing: 300 ms attack, 300 ms release. Använd Smart Animate cubic-bezier `(0.2, 0.8, 0.2, 1.0)` |
| Toggle-buttons | Press: 1 px y-offset + bg-mörkning under 80 ms |
| Rotary-rattar | Drag-rotation linjär, ingen easing |
| Faders | Drag linjär. Hover: 4 % bg-light increase |
| Preset-menu | Open: 120 ms slide-down från 0 till full höjd |
| A/B switch | 200 ms cross-fade på knapparna; ingen state-animation på själva pluginen (instant swap) |

---

## 13. Tooltips (alla kontroller)

Visa efter 600 ms hover. Tooltip-rektangel: cream paper `#F4EFE4`,
1 px ink `#1A1A1A` border, 4 px padding, 11 pt Helvetica.

Maximalt **två rader text**, bryts vid 280 px bredd.

Texten finns i koden på Editor-sidan; replikera samma texter i
Figma som plain-text-noder i en separat frame "Tooltips".

---

## 14. Skalningsbeteende

Pluginen ska kunna skalas av användaren via DAW (75 % – 200 %).

I Figma: testa renderingen i tre frames `BC2000DL @ 0.75x`,
`@ 1x`, `@ 2x`. Auto-layout med constraints så texten alltid är
center-aligned i sina containrar och rotaries skalas
proportionellt.

Inga rasterbilder! All grafik vektor (även tape-spolarnas
gradienter via Figma-effects).

---

## 15. Out-of-scope (Figma-iteration 1 inkluderar INTE)

- Funktionalitet (interaktivitet) — bara visuella mockups
- Settings-panel / about-dialog
- Manual-PDF-design
- Installer-grafik
- App-icon
- Sociala media-assets

Allt detta tas i **iteration 2** efter att frontpanelen är
godkänd.

---

## 16. Leveranser från Figma

1. **`BC2000DL_Frontpanel_v1.fig`** — komplett canvas, layered, namngiven
2. PNG-export 1×, 2× av hela frontpanelen
3. SVG-export per komponent (för JUCE Drawable-import)
4. Komponentbibliotek (Library) med alla components
5. Stilguide-frame: typografi-stack, färgpalett-swatches, spacing-grid

---

## 17. Acceptance-kriterier

- [ ] Pluginpanelen ser ut som ett **kommersiellt VST/AU**, inte en
      hobbyfront — jämförbar med UAD, Soundtoys, Plugin Alliance
- [ ] Alla 26 manualkontroller är synliga och tydligt labellade
- [ ] Tre sektionerna (INPUT, TAPE, OUTPUT) är visuellt distinkta
      utan att fragmentera designen
- [ ] Material-känsla: teak och alu och silver känns olika när man
      tittar på dem (gradient + texture på rätt sätt)
- [ ] B&O-röd är begränsad till de 5 use-cases listade ovan
- [ ] 1968-känsla utan att vara karikatyr eller pastisch
- [ ] Skalning 0.75× / 1× / 2× fungerar visuellt utan layout-buggar
- [ ] Tooltipsen är konsekvent designade och läsbara

---

## 18. Referensmaterial (skicka separat till designern)

- `plan.md` (full DSP-/UX-plan)
- `UX_BRIEF.md` (V3-versionen)
- `mockups/bc2000dl_mockup_full.png` (existerande approximation)
- Auctionet-fotografi av riktig BeoCord 2000 (för material-referens)
- DIN-1962 EQ-kurvor (cream/röd, för att visa designens "skola")
- Service-manualens layout-diagram (s.1) — för att förstå B&O:s
  egna spacing och typografi-val 1968

---

**Slutligen** — designern bör lyssna på pluginen i 10 minuter med
hörlurar innan första iteration. Ljudet är produkten; UI:n är hur
användaren förhandlar med ljudet. Om man inte vet hur det låter
kan man inte designa hur det ska kännas.
