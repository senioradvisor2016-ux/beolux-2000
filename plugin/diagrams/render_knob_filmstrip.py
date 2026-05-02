"""
Procedural chrome-knob filmstrip generator.

Renders a single chrome dome knob with baked upper-left key-light
across 60 frames covering 0..270° rotation. Outputs a vertical
strip PNG (frame_w x frame_h*60) suitable for CSS background-position
animation a la UAD/Soundtoys filmstrip controls.

Lighting model:
- Single key light at upper-left @ 45° (matches BEOFLUX panel convention)
- Soft ambient fill (~25% strength)
- Specular highlight on the upper-left quadrant
- Self-shadow on the lower-right
- Concentric grooves on the cap (subtle radial dark rings)
- Pointer notch rotates with the value

The output is intentionally smaller-than-perfect by 3D-render standards,
but vastly better than CSS gradients alone.
"""

from PIL import Image, ImageDraw, ImageFilter
import math
import os

FRAME_W = 128         # px per frame
FRAME_H = 128
N_FRAMES = 60
ROT_START = -135      # degrees (full CCW)
ROT_END   =  135      # degrees (full CW)

OUT_DIR = os.path.dirname(os.path.abspath(__file__))
OUT_FILE = os.path.join(OUT_DIR, '..', 'juce', 'Source', 'assets', 'knob_chrome_filmstrip.png')


def lerp(a, b, t):
    return a + (b - a) * t


def chrome_color(angle_from_light_deg):
    """Map angle-from-light to a chrome shade. 0° = full highlight, 180° = full shadow."""
    a = abs(angle_from_light_deg) % 360
    if a > 180:
        a = 360 - a
    t = a / 180.0  # 0..1
    if t < 0.18:
        # Sparkle highlight
        return tuple(int(lerp(c1, c2, t * 5.5)) for c1, c2 in zip((250, 248, 240), (210, 205, 195)))
    elif t < 0.55:
        return tuple(int(lerp(c1, c2, (t - 0.18) / 0.37)) for c1, c2 in zip((210, 205, 195), (130, 125, 118)))
    else:
        return tuple(int(lerp(c1, c2, (t - 0.55) / 0.45)) for c1, c2 in zip((130, 125, 118), (40, 38, 34)))


def render_frame(angle_deg):
    """Render single chrome knob frame at given pointer angle (degrees from up)."""
    img = Image.new('RGBA', (FRAME_W, FRAME_H), (0, 0, 0, 0))
    cx, cy = FRAME_W // 2, FRAME_H // 2
    R = min(cx, cy) - 4

    # Outer bezel ring (dark)
    bezel = Image.new('RGBA', (FRAME_W, FRAME_H), (0, 0, 0, 0))
    bd = ImageDraw.Draw(bezel)
    bd.ellipse([cx - R, cy - R, cx + R, cy + R], fill=(20, 18, 16, 255))
    img.alpha_composite(bezel)

    # Chrome dome (radial baked lighting)
    dome_R = R - 6
    pixels = img.load()
    light_dir = (-1, -1)  # upper-left
    light_norm = math.hypot(*light_dir)
    light_dir = (light_dir[0] / light_norm, light_dir[1] / light_norm)
    for y in range(cy - dome_R, cy + dome_R + 1):
        for x in range(cx - dome_R, cx + dome_R + 1):
            dx = x - cx
            dy = y - cy
            r = math.hypot(dx, dy)
            if r > dome_R:
                continue
            # Surface normal of a sphere at (dx, dy, ~sqrt(R^2 - dx^2 - dy^2))
            nz = math.sqrt(max(0, dome_R * dome_R - dx * dx - dy * dy)) / dome_R
            nx = dx / dome_R
            ny = dy / dome_R
            # Diffuse from upper-left light, with z-component
            light_3d = (-0.6, -0.6, 0.5)
            ldn = math.sqrt(sum(c * c for c in light_3d))
            light_3d = tuple(c / ldn for c in light_3d)
            diff = max(0, nx * light_3d[0] + ny * light_3d[1] + nz * light_3d[2])
            # Specular (Phong-ish)
            r_vec = (2 * diff * light_3d[0] - nx * 0,
                     2 * diff * light_3d[1] - ny * 0,
                     2 * diff * light_3d[2] - nz * 0)
            spec = max(0, diff) ** 12
            # Ambient
            amb = 0.18
            # Brushed-chrome anisotropy: slight horizontal grain
            grain = 0.92 + 0.08 * (((y * 7 + x * 3) % 5) / 5.0)
            v = min(1.0, (amb + diff * 0.85 + spec * 0.55) * grain)
            cr = int(40 + v * 215)
            cg = int(38 + v * 210)
            cb = int(34 + v * 205)
            pixels[x, y] = (cr, cg, cb, 255)

    # Concentric grooves (subtle dark rings)
    overlay = Image.new('RGBA', (FRAME_W, FRAME_H), (0, 0, 0, 0))
    od = ImageDraw.Draw(overlay)
    for ring_r in (dome_R - 8, dome_R - 18, dome_R - 28):
        if ring_r < 6:
            continue
        od.ellipse([cx - ring_r, cy - ring_r, cx + ring_r, cy + ring_r],
                   outline=(0, 0, 0, 38), width=1)
    img.alpha_composite(overlay)

    # Pointer notch — dark vertical line from center toward edge, rotated by angle
    notch_layer = Image.new('RGBA', (FRAME_W, FRAME_H), (0, 0, 0, 0))
    nd = ImageDraw.Draw(notch_layer)
    notch_w = 3
    notch_len = dome_R - 8
    # Build vertical notch then rotate
    v_layer = Image.new('RGBA', (FRAME_W, FRAME_H), (0, 0, 0, 0))
    vd = ImageDraw.Draw(v_layer)
    vd.rectangle([cx - notch_w // 2, cy - notch_len, cx + notch_w // 2, cy - 4],
                 fill=(20, 14, 10, 255))
    v_layer = v_layer.rotate(-angle_deg, resample=Image.BICUBIC, center=(cx, cy))
    img.alpha_composite(v_layer)

    # Outer rim shadow (where bezel meets dome)
    rim = Image.new('RGBA', (FRAME_W, FRAME_H), (0, 0, 0, 0))
    rd = ImageDraw.Draw(rim)
    rd.ellipse([cx - dome_R, cy - dome_R, cx + dome_R, cy + dome_R],
               outline=(0, 0, 0, 110), width=2)
    rim = rim.filter(ImageFilter.GaussianBlur(radius=1.5))
    img.alpha_composite(rim)

    return img


def main():
    strip = Image.new('RGBA', (FRAME_W, FRAME_H * N_FRAMES), (0, 0, 0, 0))
    for i in range(N_FRAMES):
        t = i / (N_FRAMES - 1)
        angle = ROT_START + t * (ROT_END - ROT_START)
        frame = render_frame(angle)
        strip.paste(frame, (0, i * FRAME_H), frame)
        if i % 10 == 0:
            print(f"  frame {i:02d}/{N_FRAMES} @ {angle:+.1f}°")
    os.makedirs(os.path.dirname(OUT_FILE), exist_ok=True)
    strip.save(OUT_FILE, 'PNG', optimize=True)
    print(f"\nWrote: {OUT_FILE}")
    print(f"Size:  {FRAME_W}x{FRAME_H * N_FRAMES} ({N_FRAMES} frames)")


if __name__ == '__main__':
    main()
