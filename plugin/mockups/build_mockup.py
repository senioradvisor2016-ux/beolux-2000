"""Generate full BC2000DL UX mockup as SVG (Figma-importerbar).

Renderar exakt 1300×500 enligt UX_BRIEF.md med alla 31 kontroller.
Använder samma hex-värden som JUCE-implementationen.

Output:
- mockup.svg     — vector source
- mockup.png     — PNG-rendering @2x för delning
"""
from pathlib import Path
import math

# ---------- Designtokens (matchar plugin/juce/Source/ui/BCColours.h) ----------
TEAK_DK   = "#7A4722"
TEAK      = "#9C5F35"
TEAK_LT   = "#B07845"
ALU_DK    = "#141414"
ALU       = "#1F1F1F"
ALU_LT    = "#2C2C2C"
SILVER    = "#B5B5B5"
SILVER_LT = "#D0D0D0"
KNAB      = "#A8A8A8"
VU_GLASS  = "#0A0A0A"
WHITE     = "#FFFFFF"
PAPER     = "#F4EFE4"
BLACK     = "#080808"
RECORD_RED = "#C42820"
SOFT_GRAY  = "#9A9A9A"
TAPE_BASF = "#8A4A35"
TAPE_AGFA = "#6E3D2A"
LABEL_RED = "#C02828"
LABEL_BLUE = "#2860A0"

W, H = 1300, 500
TEAK_THICK = 28


def svg_header():
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}" font-family="Helvetica, Arial, sans-serif">
<defs>
  <radialGradient id="teakGrad" cx="50%" cy="50%" r="60%">
    <stop offset="0%" stop-color="{TEAK_LT}"/>
    <stop offset="100%" stop-color="{TEAK_DK}"/>
  </radialGradient>
  <linearGradient id="aluGrad" x1="0" y1="0" x2="0" y2="1">
    <stop offset="0%" stop-color="{ALU_LT}"/>
    <stop offset="100%" stop-color="{ALU_DK}"/>
  </linearGradient>
  <linearGradient id="deckGrad" x1="0" y1="0" x2="0" y2="1">
    <stop offset="0%" stop-color="{SILVER_LT}"/>
    <stop offset="100%" stop-color="{SILVER}"/>
  </linearGradient>
  <linearGradient id="knabGrad" x1="0" y1="0" x2="0" y2="1">
    <stop offset="0%" stop-color="{KNAB}"/>
    <stop offset="100%" stop-color="#7A7A7A"/>
  </linearGradient>
  <linearGradient id="silverFaderKnobL" x1="0" y1="0" x2="0" y2="1">
    <stop offset="0%" stop-color="{SILVER_LT}"/>
    <stop offset="100%" stop-color="{SILVER}"/>
  </linearGradient>
  <pattern id="woodGrain" width="3" height="100%" patternUnits="userSpaceOnUse">
    <rect width="3" height="100%" fill="{TEAK}"/>
    <line x1="0" y1="0" x2="0" y2="100%" stroke="{TEAK_DK}" stroke-width="0.4" stroke-opacity="0.25"/>
  </pattern>
</defs>
'''


# ---------- Helpers ----------
def rect(x, y, w, h, fill, stroke=None, sw=0.5, rx=0):
    s = f'<rect x="{x}" y="{y}" width="{w}" height="{h}" fill="{fill}"'
    if stroke: s += f' stroke="{stroke}" stroke-width="{sw}"'
    if rx: s += f' rx="{rx}" ry="{rx}"'
    s += '/>'
    return s


def circ(cx, cy, r, fill, stroke=None, sw=0.5):
    s = f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="{fill}"'
    if stroke: s += f' stroke="{stroke}" stroke-width="{sw}"'
    s += '/>'
    return s


def text(x, y, content, size=10, color=WHITE, weight="normal", anchor="start", font="Helvetica"):
    s = f'<text x="{x}" y="{y}" font-size="{size}" fill="{color}" font-weight="{weight}" '
    s += f'text-anchor="{anchor}" font-family="{font}">{content}</text>'
    return s


def line(x1, y1, x2, y2, stroke, sw=1.0, opacity=1.0):
    return f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{stroke}" stroke-width="{sw}" stroke-opacity="{opacity}"/>'


# ---------- Komponenter ----------
def push_button(x, y, w=56, h=22, label="", active=False):
    """Push-toggle-knapp."""
    bg = RECORD_RED if active else KNAB
    txt_color = WHITE if active else ALU_DK
    out = []
    # Skugga undertill (om inactive)
    if not active:
        out.append(rect(x, y + h, w, 1.5, ALU_DK, rx=1))
    out.append(rect(x, y, w, h, bg, stroke=ALU_DK, sw=0.5, rx=2))
    out.append(text(x + w / 2, y + h / 2 + 3, label, size=9, color=txt_color,
                     weight="bold", anchor="middle"))
    return "\n".join(out)


def vu_meter(x, y, w=110, h=52, label="L", level=0.6, recording=False):
    """VU-mätare med svart bg, vit nål, röd HIGH-zon."""
    out = []
    # Record-glow
    if recording:
        out.append(rect(x - 3, y - 3, w + 6, h + 6, RECORD_RED, rx=4))
        out[-1] = out[-1].replace('fill="' + RECORD_RED + '"',
                                   f'fill="{RECORD_RED}" fill-opacity="0.55"')
    # Glas-bg svart
    out.append(rect(x, y, w, h, ALU_DK, stroke=SILVER, sw=1, rx=2))

    # L/R-label
    out.append(text(x + 8, y + 12, label, size=9, color=SILVER_LT,
                     weight="bold", anchor="start"))

    # Tick-marks längs botten
    for i in range(11):
        t = i / 10
        tx = x + 10 + (w - 20) * t
        is_red = t > 0.7
        h_tick = 6 if i % 5 == 0 else 3
        col = RECORD_RED if is_red else SILVER_LT
        out.append(line(tx, y + h - 12, tx, y + h - 12 + h_tick, col, sw=1))

    # Röd HIGH-zon-streck
    red_start = x + 10 + (w - 20) * 0.7
    out.append(line(red_start, y + h - 4, x + w - 10, y + h - 4, RECORD_RED, sw=2))

    # Nål
    pivot_x = x + w / 2
    pivot_y = y + h - 2
    angle = (level - 0.5) * 0.8 * math.pi
    needle_len = h * 0.78
    tip_x = pivot_x + math.sin(angle) * needle_len
    tip_y = pivot_y - math.cos(angle) * needle_len
    out.append(line(pivot_x, pivot_y, tip_x, tip_y, SILVER_LT, sw=1.4))
    out.append(circ(pivot_x, pivot_y, 2.5, SILVER))

    return "\n".join(out)


def dual_fader(x, y, w=75, h=240, symbol="", value_l=0.5, value_r=0.5):
    """5-fader-slits med 2 silver-knoppar (skydepotentiometre-stil)."""
    out = []

    # Symbol ovan slits
    out.append(text(x + w / 2, y + 10, symbol, size=12, color=SILVER_LT,
                     weight="bold", anchor="middle"))

    # Spår — mitt i slitsen
    track_x = x + w / 2 - 3
    track_top = y + 18
    track_bot = y + h - 12
    track_h = track_bot - track_top
    out.append(rect(track_x, track_top, 6, track_h, ALU_LT, rx=2))

    # Skala 0-10 vänster sida
    for i in range(11):
        ty = track_top + track_h * (i / 10)
        num = 10 - i
        out.append(line(track_x - 14, ty, track_x - 10, ty, SOFT_GRAY, sw=0.5))
        out.append(text(track_x - 16, ty + 3, str(num), size=7,
                         color=SILVER_LT, anchor="end"))

    # Knopp L (vänster knopp i slits)
    knob_w = 18
    knob_h = 12
    knob_l_x = x + w / 2 - knob_w - 2
    knob_l_y = track_top + (track_h * (1 - value_l)) - knob_h / 2
    out.append(rect(knob_l_x, knob_l_y, knob_w, knob_h, SILVER_LT,
                     stroke=ALU_DK, sw=0.5, rx=1.5))
    # Grip-lines
    for i in range(3):
        ly = knob_l_y + 3 + i * 2.5
        out.append(line(knob_l_x + 2, ly, knob_l_x + knob_w - 2, ly, ALU_DK, sw=0.4))
    # L-label
    out.append(text(knob_l_x - 6, knob_l_y + 8, "L", size=6,
                     color=SILVER_LT, weight="bold", anchor="middle"))

    # Knopp R (höger knopp i slits)
    knob_r_x = x + w / 2 + 2
    knob_r_y = track_top + (track_h * (1 - value_r)) - knob_h / 2
    out.append(rect(knob_r_x, knob_r_y, knob_w, knob_h, SILVER,
                     stroke=ALU_DK, sw=0.5, rx=1.5))
    for i in range(3):
        ly = knob_r_y + 3 + i * 2.5
        out.append(line(knob_r_x + 2, ly, knob_r_x + knob_w - 2, ly, ALU_DK, sw=0.4))
    out.append(text(knob_r_x + knob_w + 4, knob_r_y + 8, "R", size=6,
                     color=SILVER_LT, weight="bold", anchor="middle"))

    return "\n".join(out)


def rotary(cx, cy, r=22, label="", value_angle=45):
    """B&O-stil rotary med liten silver-prick som pekare."""
    out = []
    # Yttre knab
    out.append(circ(cx, cy, r, "url(#knabGrad)", stroke=ALU_DK, sw=0.5))
    # Pekare-prick på yttre kant
    rad = math.radians(value_angle - 90)
    px = cx + math.cos(rad) * r * 0.7
    py = cy + math.sin(rad) * r * 0.7
    out.append(circ(px, py, r * 0.16, ALU))
    # Label under
    out.append(text(cx, cy + r + 12, label, size=8, color=SILVER_LT,
                     weight="bold", anchor="middle"))
    return "\n".join(out)


def transport_lever(x, y, w=140, h=40):
    """Y-form transport-lever med ‹‹ och ›› pilar."""
    out = []
    cx = x + w / 2
    cy = y + h / 2

    # ‹‹ vänster pil
    out.append(text(x + 20, cy + 4, "‹‹", size=14, color=SILVER_LT,
                     weight="bold", anchor="middle"))
    # ›› höger pil
    out.append(text(x + w - 20, cy + 4, "››", size=14, color=SILVER_LT,
                     weight="bold", anchor="middle"))
    # Y-skaft
    out.append(line(cx, cy + 12, cx, cy + 4, SILVER_LT, sw=4))
    # Kula på toppen
    out.append(circ(cx, cy + 4, 5, SILVER_LT, stroke=ALU_DK, sw=0.6))
    return "\n".join(out)


def tape_counter(x, y, w=80, h=24, value=0):
    """Mekanisk 3-digit räkneverk."""
    out = []
    digit_w = (w - 2) / 3
    s = f"{value:03d}"
    for i in range(3):
        dx = x + (digit_w + 1) * i
        out.append(rect(dx, y, digit_w, h, BLACK, stroke=ALU_DK, sw=0.5, rx=1))
        out.append(text(dx + digit_w / 2, y + h / 2 + 4, s[i], size=12,
                         color=SILVER_LT, weight="bold", anchor="middle"))
    return "\n".join(out)


def reel(cx, cy, r=60, label_color=LABEL_RED, tape_color=TAPE_BASF):
    """Tape-spole med BASF/AGFA-stil label + brunt tape-band."""
    out = []
    # Yttre cirkel (klar plast — lätt transparent)
    out.append(f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="{PAPER}" fill-opacity="0.7" stroke="{ALU_DK}" stroke-width="0.8"/>')

    # Tape-band (oxiderad brun ring)
    tape_outer = r * 0.85
    tape_inner = r * 0.45
    out.append(circ(cx, cy, tape_outer, tape_color))
    out.append(f'<circle cx="{cx}" cy="{cy}" r="{tape_inner}" fill="{PAPER}" fill-opacity="0.7"/>')

    # Spiral-linjer (texture)
    rr = tape_inner
    while rr < tape_outer:
        out.append(f'<circle cx="{cx}" cy="{cy}" r="{rr}" fill="none" stroke="{tape_color}" stroke-width="0.3" stroke-opacity="0.4"/>')
        rr += 1.5

    # BASF/AGFA-label
    label_r = r * 0.35
    out.append(circ(cx, cy, label_r, label_color, stroke=PAPER, sw=0.6))

    # Hub
    hub_r = r * 0.16
    out.append(circ(cx, cy, hub_r, BLACK))

    # 3 ekrar
    for s in range(3):
        a = math.radians(s * 120)
        x1 = cx + math.cos(a) * hub_r * 1.5
        y1 = cy + math.sin(a) * hub_r * 1.5
        x2 = cx + math.cos(a) * label_r * 0.9
        y2 = cy + math.sin(a) * label_r * 0.9
        out.append(line(x1, y1, x2, y2, SILVER_LT, sw=1, opacity=0.4))

    return "\n".join(out)


def combo_box(x, y, w=110, h=22, value="", active=False):
    """Combo-väljare."""
    out = []
    bg = ALU_LT
    out.append(rect(x, y, w, h, bg, stroke=SILVER, sw=0.5, rx=2))
    out.append(text(x + 8, y + h / 2 + 3, value, size=9,
                     color=SILVER_LT, anchor="start"))
    # Pil till höger
    arrow_x = x + w - 12
    arrow_y = y + h / 2
    out.append(f'<polygon points="{arrow_x},{arrow_y - 2} {arrow_x + 6},{arrow_y - 2} {arrow_x + 3},{arrow_y + 2}" fill="{SILVER}"/>')
    return "\n".join(out)


# ---------- Build full mockup ----------
def build():
    parts = [svg_header()]

    # 1. Teak-ram (yttre)
    parts.append(rect(0, 0, W, H, "url(#teakGrad)"))
    parts.append(rect(0, 0, W, H, "url(#woodGrain)"))

    # 2. Inner edge (skugga vid teak-kanten)
    parts.append(rect(TEAK_THICK - 1, TEAK_THICK - 1,
                       W - 2 * TEAK_THICK + 2, H - 2 * TEAK_THICK + 2,
                       "none", stroke=TEAK_DK, sw=1))

    # 3. Inner panel (svart anodiserad)
    panel_x = TEAK_THICK
    panel_y = TEAK_THICK
    panel_w = W - 2 * TEAK_THICK
    panel_h = H - 2 * TEAK_THICK
    parts.append(rect(panel_x, panel_y, panel_w, panel_h, "url(#aluGrad)"))

    # 4. Tape-deck-zone (silver)
    deck_h = int(panel_h * 0.30)
    parts.append(rect(panel_x, panel_y, panel_w, deck_h, "url(#deckGrad)"))

    # Brushed grain (horisontella linjer)
    for i in range(0, deck_h, 2):
        parts.append(line(panel_x, panel_y + i, panel_x + panel_w, panel_y + i,
                           SILVER_LT, sw=0.4, opacity=0.3))

    # 5. Två spolar (BASF + AGFA)
    reel_r = int(deck_h * 0.36)
    reel_y = panel_y + deck_h // 2
    reel_l_cx = panel_x + int(panel_w * 0.22)
    reel_r_cx = panel_x + int(panel_w * 0.78)
    parts.append(reel(reel_l_cx, reel_y, reel_r, LABEL_RED, TAPE_BASF))
    parts.append(reel(reel_r_cx, reel_y, reel_r, LABEL_BLUE, TAPE_AGFA))

    # 6. Tape-head-cover (svart med krona)
    cover_w = int(panel_w * 0.12)
    cover_h = int(deck_h * 0.55)
    cover_x = panel_x + (panel_w - cover_w) // 2
    cover_y = panel_y + int(deck_h * 0.5) - cover_h // 2 + 10
    parts.append(rect(cover_x, cover_y, cover_w, cover_h, ALU_DK, rx=2))
    parts.append(text(cover_x + cover_w / 2, cover_y + 18, "♛", size=18,
                       color=SILVER_LT, anchor="middle"))

    # 7. Logo "BC2000DL · DANISH TAPE 2000"
    logo_y = panel_y + deck_h + 16
    parts.append(text(panel_x + panel_w / 2, logo_y,
                       "BC2000DL  ·  DANISH TAPE 2000",
                       size=12, color=SILVER_LT, weight="bold", anchor="middle"))

    # 8. Topp-trim röd 3px
    parts.append(rect(0, 0, W, 3, RECORD_RED))

    # ---------- Control panel ----------
    ctrl_y = logo_y + 20
    ctrl_h = panel_h - (deck_h + 36)
    ctrl_x = panel_x + 16

    # === ZON A — vänster (340 px) ===
    zone_a_x = ctrl_x
    zone_a_w = 340

    # Push-grupp 2×4 (8 huvudknappar)
    btn_w = 56; btn_h = 22
    btn_y1 = ctrl_y
    btn_y2 = btn_y1 + btn_h + 4
    main_btns_row1 = [("ECHO", True), ("REC 1", False), ("REC 2", False), ("SYNC", False)]
    main_btns_row2 = [("BYPASS", False), ("TRK 1", True), ("TRK 2", True), ("S/S", False)]
    for i, (lbl, active) in enumerate(main_btns_row1):
        parts.append(push_button(zone_a_x + i * (btn_w + 4), btn_y1, btn_w, btn_h, lbl, active))
    for i, (lbl, active) in enumerate(main_btns_row2):
        parts.append(push_button(zone_a_x + i * (btn_w + 4), btn_y2, btn_w, btn_h, lbl, active))

    # VU-mätare under push-grupp
    vu_y = btn_y2 + btn_h + 12
    parts.append(vu_meter(zone_a_x, vu_y, 110, 52, "L", level=0.6, recording=False))
    parts.append(vu_meter(zone_a_x + 114, vu_y, 110, 52, "R", level=0.5, recording=False))

    # Push-rad 3 (sekundära)
    btn_y3 = vu_y + 60
    sec_btns = [("SPEAKER", False), ("MUTE", False), ("PAUSE", False), ("P.A.", False)]
    for i, (lbl, active) in enumerate(sec_btns):
        parts.append(push_button(zone_a_x + i * (btn_w + 4), btn_y3, btn_w, btn_h, lbl, active))

    # Push-rad 4 (modes)
    btn_y4 = btn_y3 + btn_h + 4
    mode_btns = [("SPK A", False), ("SPK B", False), ("Lo-Z", True)]
    for i, (lbl, active) in enumerate(mode_btns):
        parts.append(push_button(zone_a_x + i * (btn_w + 4), btn_y4, btn_w, btn_h, lbl, active))

    # Combo-rader
    combo_y1 = btn_y4 + btn_h + 8
    parts.append(combo_box(zone_a_x, combo_y1, 110, 22, "19 cm/s ▼"))
    parts.append(combo_box(zone_a_x + 118, combo_y1, 110, 22, "Tape ▼"))
    combo_y2 = combo_y1 + 26
    parts.append(combo_box(zone_a_x, combo_y2, 110, 22, "H (magnetic) ▼"))
    parts.append(combo_box(zone_a_x + 118, combo_y2, 110, 22, "L (3 mV) ▼"))

    # === ZON B — mitten (380 px) — 5 dual-faders ===
    zone_b_x = zone_a_x + zone_a_w
    fader_w = 75
    fader_h = 240
    fader_y = ctrl_y

    fader_specs = [
        ("♭", 0.4, 0.4),   # Radio
        ("○", 0.6, 0.6),   # Phono
        ("♂", 0.7, 0.65),  # Mic
        ("S", 0.5, 0.5),   # Saturation
        ("◁", 0.3, 0.3),   # Echo
    ]
    for i, (sym, vl, vr) in enumerate(fader_specs):
        parts.append(dual_fader(zone_b_x + i * fader_w, fader_y, fader_w, fader_h, sym, vl, vr))

    # === ZON C — höger (~400 px) — rotary + transport + counter ===
    zone_c_x = zone_b_x + 5 * fader_w + 10
    rotary_specs = [
        ("TREBLE", 0),
        ("BASS",   0),
        ("BAL",    0),
        ("VOL",    60),
        ("BIAS",   0),
        ("MULT",   -45),
    ]
    rotary_r = 22
    rotary_w_each = 64
    rotary_y_row1 = ctrl_y + 20
    rotary_y_row2 = rotary_y_row1 + 70
    rotary_y_row3 = rotary_y_row2 + 70

    # Rad 1: TREBLE BASS BAL
    for i in range(3):
        cx = zone_c_x + rotary_w_each / 2 + i * rotary_w_each
        parts.append(rotary(cx, rotary_y_row1, rotary_r,
                             rotary_specs[i][0], rotary_specs[i][1]))

    # Rad 2: VOL BIAS MULT
    for i in range(3):
        cx = zone_c_x + rotary_w_each / 2 + i * rotary_w_each
        parts.append(rotary(cx, rotary_y_row2, rotary_r,
                             rotary_specs[i + 3][0], rotary_specs[i + 3][1]))

    # Rad 3: Transport-lever + Counter
    transport_y = rotary_y_row3
    parts.append(transport_lever(zone_c_x, transport_y, 140, 40))
    parts.append(tape_counter(zone_c_x + 152, transport_y + 8, 80, 24, value=42))

    parts.append("</svg>")
    return "\n".join(parts)


def main():
    out_dir = Path(__file__).parent
    out_dir.mkdir(exist_ok=True)
    svg_path = out_dir / "bc2000dl_mockup.svg"
    svg_path.write_text(build(), encoding="utf-8")
    print(f"OK  {svg_path}  ({svg_path.stat().st_size // 1024} KB)")


if __name__ == "__main__":
    main()
