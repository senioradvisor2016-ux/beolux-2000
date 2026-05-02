# Handoff: SOUNDBOYS BEOFLUX 2000DL — Tape Plugin UI

## Overview
BEOFLUX 2000DL is a VST3/AU/Standalone tape emulation plugin inspired by the Bang & Olufsen Beocord 2000 De Luxe (1968), built on a TEAC visual platform. The UI is a 1280px-wide plugin panel with warm walnut wood bezel, matte black control panel, polished chrome controls, and amber accents.

## About the Design Files
The HTML file in this bundle is a **design reference prototype** — a fully interactive mock showing exact look, layout, colors, interactions, and control behavior. It is NOT production code. The task is to **recreate this design in JUCE/C++** as PNG filmstrips and panel plates, matching the visual fidelity and interaction states shown. All 26+ controls are wired with state management, drag interactions, and live VU animation to demonstrate intended behavior.

## Fidelity
**High-fidelity (hifi)** — pixel-accurate mockup with final colors, typography, spacing, materials, and interactions. Recreate pixel-perfectly using JUCE filmstrip assets.

## Brand
- **App brand:** SOUNDBOYS (top-left of panel)
- **Product name:** BEOFLUX 2000 DL · 4-TRACK STEREO TAPE DECK (top-right)
- **Bottom brand:** BEOFLUX / 2000 · DELUXE

---

## Layout Structure (1280px wide, auto-height ~780px)

### Rows (top to bottom):
1. **Top Strip** (22px) — SOUNDBOYS logo left, BEOFLUX model name right, on walnut wood bezel
2. **Main Panel** (matte black, rounded 8px) containing:
   - **Upper Row** — 3-column grid: `140px | 1fr | 140px`
     - Left: Counter + Tonhead switches + Track-arm LEDs + ARM 1/2 + PAUSE
     - Center: Reel bay (250px tall) with twin 200px reels + animated head-bay with tape path canvas
     - Right: Transport lever + Headphone jack
   - **Meter Bridge** — 3× VU meters horizontally centered (210px × 72px each)
   - **Control Strip** (24px) — horizontal bar with: SPEED [4.75|9.5|19] · FORMULA [AGFA|BASF|SCOTCH] · PRESET [◀ FACTORY ▶] · A/B · MEM · PWR/REC LEDs · Ph [L|H] · Rad [L|H] · Mon [TAPE|SRC] · Out [ST|1|2]
   - **Middle Row** — 3-column grid: `180px | 1fr | 210px`
     - Left: EFFECTS buttons (ECHO/SYNC/BYP) + MONITOR buttons (SPK·A/B, MUTE, Lo-Z, P.A., S/S)
     - Center: 10-fader mixer array (5 pairs L/R)
     - Right: 8 rotary knobs in 4×2 grid
   - **Bottom Row** — DIN connectors + BEOFLUX brand + Mic jacks
3. **Hint Bar** (12px) — keyboard shortcut hints

### Auto-scaling
`transform: scale()` on the deck element scales to fit any viewport. No max cap — fills available space.

---

## Design Tokens

### Colors (CSS custom properties)
```
--wood-1: #6b3a1f          /* walnut bezel primary */
--wood-2: #4a240f          /* walnut dark */
--wood-grain: #2a1407      /* grain lines */
--panel-1: #1a1815         /* matte black panel */
--panel-2: #0f0d0b         /* panel dark */
--panel-edge: #2a2722      /* panel border */
--chrome-1: #f0ece4        /* chrome highlight */
--chrome-2: #b8b3a8        /* chrome mid */
--chrome-3: #6e6a62        /* chrome shadow */
--chrome-edge: #2a2722     /* chrome bezel */
--amber: #d8a23a           /* accent / active state */
--amber-glow: #e8b94a      /* bright amber */
--amber-dim: #6b4c1a       /* dim amber */
--red-led: #e63946         /* LED on */
--red-led-dim: #4a1418     /* LED off */
--green-led: #4ade80       /* PWR LED on */
--green-led-dim: #1a3a22   /* PWR LED off */
--ink: #ece4d0             /* primary text */
--ink-dim: #8c8474         /* secondary text */
--ink-faint: #50483c       /* tertiary text */
--silk: #d8c9a8            /* brand text */
```

### Typography
- **Brand (SOUNDBOYS):** Helvetica Neue, 800 weight, 16px, letter-spacing 0.34em, color #e8d9b8
- **Model name:** monospace, 11px, letter-spacing 0.32em, color #b8a888
- **Labels:** 7-8px, letter-spacing 0.12-0.24em, uppercase, weight 700
- **Counter digits:** monospace, 26px, bold, amber with glow
- **Knob labels:** 8px, letter-spacing 0.24em, uppercase
- **VU labels:** serif, 5-7px

### Spacing
- Panel padding: 10px
- Row gaps: 4px
- Button gaps: 3px
- Knob row gaps: 6px

### Shadows
- Deck: `0 30px 60px -15px rgba(0,0,0,0.7)`
- Panel inset: `0 14px 28px -10px rgba(0,0,0,0.6) inset`
- Buttons: `0 2px 0 rgba(0,0,0,0.4), 0 4px 8px rgba(0,0,0,0.4)`
- Knobs: `0 0 0 1px #1a1612, 0 0 0 4px #14110e, 0 0 0 5px #3a3228, 0 4px 8px rgba(0,0,0,0.5)`

---

## Complete Control Inventory

### Faders (5 pairs = 10 sliders, dual L/R per track)
| ID | Label | Range | Notes |
|---|---|---|---|
| radio-l / radio-r | Radio | -∞…+10 dB | Tuner input |
| phono-l / phono-r | Phono | -∞…+10 dB | Gram input |
| mic-l / mic-r | Mic | -∞…+10 dB | Microphone input |
| drive-l / drive-r | Drive | 0…+10 | Input drive into tape |
| echo-l / echo-r | Echo Send | 0…+10 | Echo send level |

**Thumb style:** Chrome multi-band (horizontal stripes), 18px tall, grab cursor. Shows amber value pill on drag.

### Rotary Knobs (8 total, 2×4 grid on right)
| ID | Label | Range | Symbol | Size |
|---|---|---|---|---|
| treble | Treble | ±12 dB | 𝄞 (G-clef) | 44px |
| bass | Bass | ±12 dB | 𝄢 (F-clef) | 44px |
| bias | Bias | -50…+50% | — | 34px |
| wow | Wow | 0…100% | — | 34px |
| sat | Sat | 0…100% | — | 34px |
| bal | Bal | L↔R | — | 34px |
| echor | Echo·R | 0…95% | — | 34px |
| mult | Mult | 1…3 | — | 34px |

**Style:** Polished chrome with concentric groove pattern, dark notch indicator. Drag vertical to rotate (-135° to +135°). Double-click resets to center (50).

### Toggle Buttons (20 total)
| ID | Label | Group | Location |
|---|---|---|---|
| track-rec | 2/4 TRACK RECORD | Tonhead | Upper-left |
| track-play | 2/4 TRACK PLAYBACK | Tonhead | Upper-left |
| arm1 | ARM 1 | Track | Upper-left |
| arm2 | ARM 2 | Track | Upper-left |
| pause | PAUSE | Track | Upper-left |
| echo | ECHO | Effects | Middle-left |
| sync | SYNC | Effects | Middle-left |
| bypass | BYP | Effects | Middle-left |
| spkA | SPK·A | Monitor | Middle-left |
| spkB | SPK·B | Monitor | Middle-left |
| mute | MUTE | Monitor | Middle-left |
| loz | Lo-Z | Monitor | Middle-left |
| pa | P.A. | Monitor | Middle-left |
| sos | S/S | Monitor | Middle-left |
| ab-a | A | Compare | Control strip |
| ab-b | B | Compare | Control strip |
| mem | MEM | Utility | Control strip |

**Exclusive groups:** track-rec/track-play (mutually exclusive), ab-a/ab-b (mutually exclusive)

### Mode Selectors (mutually exclusive per group)
| Group | Values | Location |
|---|---|---|
| speed | 475 / 950 / 1900 | Control strip |
| formula | AGFA / BASF / SCOTCH | Control strip |
| phono | L / H | Control strip |
| radio-in | L / H | Control strip |
| monitor | TAPE / SRC | Control strip |
| output | ST / 1 / 2 | Control strip |

### Transport
- **Lever** — click zones: left 25% = RW, center = PLAY/STOP toggle, right 25% = FF
- **States:** rw / stop / play / ff
- Reels animate: play = normal spin, rw = reverse fast, ff = fast, stop = stationary

### VU Meters (3)
| ID | Label | Source |
|---|---|---|
| vu-l | L · CH1 | Input fader sum L × drive L |
| vu-r | R · CH2 | Input fader sum R × drive R |
| vu-out | OUTPUT | Average of L+R |

**Style:** Cream face (#f0e4c8), dB scale (-20 to +3), red zone above 0VU (#c42820), serif numerals, thin needle with counterweight + shadow, chrome pivot, glass highlight overlay, chrome bezel. Ballistic: 180ms ease.

### Counter
- 3-digit mechanical counter (000-999)
- Ticks: +1/250ms during PLAY, -3/250ms during RW, +4/250ms during FF
- Click to reset to 000

### LEDs
| ID | Color | Condition |
|---|---|---|
| led-pwr | Green | Always on |
| led-rec | Red | arm1 OR arm2 active |
| led-trk1 | Red | arm1 active |
| led-trk2 | Red | arm2 active |

### Tape Path Canvas
- Animated brown oxide tape (#6b3a1f) threading through guide posts → ERS → REC → PLAY heads → capstan/pinch roller
- Moving dash pattern when playing (direction reverses for RW)
- Amber glow at head contact points during playback

---

## Interaction States

### Buttons (pill-btn, small-btn)
- **Idle:** Chrome gradient
- **Hover:** brightness(1.05)
- **Active/On:** translateY(2px), amber gradient, inset shadow
- **LED indicator:** 6px circle, red-dim when off, red+glow when on

### Faders
- **Drag:** vertical, value pill shows during drag
- **Range:** 0-100 mapped to bottom-top

### Knobs
- **Drag:** vertical (up = clockwise)
- **Double-click:** reset to 50 (center)
- **Rotation range:** -135° to +135°

### Keyboard
- **Space:** toggle play/stop
- **L:** toggle callout numbers overlay

---

## Files
- `BEOFLUX 2000DL TEAC.html` — Complete interactive prototype (2259 lines, self-contained, no external dependencies)

---

## Notes for JUCE Implementation
- All controls map 1:1 to AudioProcessorValueTreeState parameter IDs
- Fader filmstrips: render multi-band chrome thumb at 100 positions
- Knob filmstrips: 100 frames covering 270° rotation, same chrome-with-concentric-grooves geometry for all 8
- VU meter: animated needle with 180ms ballistic, cream face as background PNG, needle as separate rotated element
- Reels: 60-frame rotation filmstrip per reel
- Panel: single background PNG with walnut bezel + matte black panel
- Target: 1280px wide, ~780px tall, with scale-to-fit for different host window sizes
