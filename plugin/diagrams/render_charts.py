"""
Genererar tekniska diagram för retro-presentationen.

Output:
- output/din1962_eq.png       — 3 hastigheters EQ-kurvor (rec + play)
- output/tape_hysteresis.png  — B-H-kurva med bias-modulation
"""

import numpy as np
import matplotlib.pyplot as plt
import matplotlib as mpl
from pathlib import Path

# 1968 manualstil
PAPER  = "#F4EFE4"
INK    = "#1A1A1A"
RED    = "#C42820"
GRID   = "#C8C0B0"
SOFT   = "#9A9A9A"

mpl.rcParams.update({
    "font.family": "sans-serif",
    "font.sans-serif": ["Helvetica", "Arial"],
    "axes.edgecolor": INK,
    "axes.linewidth": 0.6,
    "axes.labelcolor": INK,
    "xtick.color": INK,
    "ytick.color": INK,
    "axes.facecolor": PAPER,
    "figure.facecolor": PAPER,
    "savefig.facecolor": PAPER,
    "axes.spines.top": False,
    "axes.spines.right": False,
})

OUT_DIR = Path(__file__).parent / "output"
OUT_DIR.mkdir(exist_ok=True)


def render_din1962_eq():
    """3 kurvor per hastighet — DIN-1962 inspired EQ-respons."""
    f = np.logspace(np.log10(20), np.log10(20_000), 800)
    fig, axes = plt.subplots(1, 2, figsize=(11, 4.4))

    speeds = [
        ("19 cm/s",   {"play_lf_corner": 50,   "play_lf_boost": 12,
                       "play_hf_corner": 12_000, "play_hf_cut": -2,
                       "rec_hf_boost_f": 15_000, "rec_hf_boost": 6}),
        ("9.5 cm/s",  {"play_lf_corner": 50,   "play_lf_boost": 14,
                       "play_hf_corner": 7_500, "play_hf_cut": -3,
                       "rec_hf_boost_f": 9_000, "rec_hf_boost": 9}),
        ("4.75 cm/s", {"play_lf_corner": 50,   "play_lf_boost": 16,
                       "play_hf_corner": 4_000, "play_hf_cut": -4,
                       "rec_hf_boost_f": 5_000, "rec_hf_boost": 12}),
    ]
    palette = [INK, RED, SOFT]

    # PLAYBACK EQ (de-emphasis)
    ax = axes[0]
    for (label, p), color in zip(speeds, palette):
        # Approximation: LF-shelf-boost + HF-roll-off
        lf = p["play_lf_boost"] / np.sqrt(1 + (f / p["play_lf_corner"])**2)
        hf = p["play_hf_cut"] * (f / p["play_hf_corner"])**2 / (1 + (f / p["play_hf_corner"])**2)
        eq = lf + hf
        ax.semilogx(f, eq, color=color, linewidth=1.6, label=label)

    ax.set_xlim(20, 20_000)
    ax.set_ylim(-10, 22)
    ax.set_xlabel("FREKVENS  [Hz]", fontsize=9, fontweight="bold")
    ax.set_ylabel("dB", fontsize=9, fontweight="bold")
    ax.set_title("PLAYBACK-EQ  (8004006 · de-emphasis)",
                 fontsize=11, fontweight="bold", color=INK,
                 loc="left", pad=10)
    ax.grid(True, which="both", color=GRID, linewidth=0.4, alpha=0.6)
    ax.axhline(0, color=INK, linewidth=0.5, alpha=0.7)
    leg = ax.legend(loc="upper right", frameon=False, fontsize=9)
    for text in leg.get_texts():
        text.set_color(INK)

    # RECORD EQ (pre-emphasis)
    ax = axes[1]
    for (label, p), color in zip(speeds, palette):
        # Pre-emphasis: HF-boost + svag LF-shelf
        hf_boost = p["rec_hf_boost"] * (f / p["rec_hf_boost_f"])**2 / (1 + (f / p["rec_hf_boost_f"])**2)
        eq = hf_boost
        ax.semilogx(f, eq, color=color, linewidth=1.6, label=label)

    ax.set_xlim(20, 20_000)
    ax.set_ylim(-3, 15)
    ax.set_xlabel("FREKVENS  [Hz]", fontsize=9, fontweight="bold")
    ax.set_ylabel("dB", fontsize=9, fontweight="bold")
    ax.set_title("RECORD-EQ  (8004005 · pre-emphasis)",
                 fontsize=11, fontweight="bold", color=INK,
                 loc="left", pad=10)
    ax.grid(True, which="both", color=GRID, linewidth=0.4, alpha=0.6)
    ax.axhline(0, color=INK, linewidth=0.5, alpha=0.7)
    leg = ax.legend(loc="upper left", frameon=False, fontsize=9)
    for text in leg.get_texts():
        text.set_color(INK)

    fig.suptitle("DIN-1962  ·  RECORD/PLAYBACK FREKVENSGÅNG  ·  3 HASTIGHETER",
                 fontsize=12, fontweight="bold", color=INK, y=0.99)

    fig.text(0.5, 0.01,
             "Källa: rekonstruktion ur servicemanual + komponentanalys §1I/§1J  ·  Approximation av RC-LC-fb-nät",
             fontsize=7.5, color=SOFT, ha="center", style="italic")

    plt.tight_layout(rect=[0, 0.03, 1, 0.96])
    out_path = OUT_DIR / "din1962_eq.png"
    plt.savefig(out_path, dpi=160, bbox_inches="tight")
    plt.close()
    print(f"OK  {out_path.name}")


def render_tape_hysteresis():
    """B-H-hysteres + bias-modulation — fysikboks-stil."""
    fig, axes = plt.subplots(1, 2, figsize=(11, 4.4))

    # Vänster: B-H-hysteres
    ax = axes[0]

    # Anhysteretisk kurva (referens, streckad)
    H = np.linspace(-3, 3, 600)
    B_anhyst = np.tanh(H * 1.0)
    ax.plot(H, B_anhyst, color=SOFT, linewidth=1.0, linestyle="--",
            label="Anhysteretic (referens)")

    # Hysteres-loop (huvudloop)
    # Modell: skift mellan upp- och ned-grenar
    coercivity = 0.6
    saturation = 1.0
    # Up-branch (H ökande)
    H_up = np.linspace(-3, 3, 400)
    B_up = saturation * np.tanh((H_up - coercivity) * 1.2)
    # Down-branch (H minskande)
    H_dn = np.linspace(3, -3, 400)
    B_dn = saturation * np.tanh((H_dn + coercivity) * 1.2)

    ax.plot(H_up, B_up, color=INK, linewidth=1.8)
    ax.plot(H_dn, B_dn, color=INK, linewidth=1.8)

    # Markera saturation och coercivity
    ax.axhline(0, color=INK, linewidth=0.5, alpha=0.7)
    ax.axvline(0, color=INK, linewidth=0.5, alpha=0.7)

    # Pilar (riktning av loopen)
    ax.annotate("", xy=(2.0, 0.85), xytext=(1.0, 0.75),
                arrowprops=dict(arrowstyle="->", color=RED, lw=1.2))
    ax.annotate("", xy=(-2.0, -0.85), xytext=(-1.0, -0.75),
                arrowprops=dict(arrowstyle="->", color=RED, lw=1.2))

    # Annoteringar
    ax.text(2.5, 0.1, "+B$_r$", fontsize=10, color=INK, fontweight="bold")
    ax.text(-2.8, -0.2, "-B$_r$", fontsize=10, color=INK, fontweight="bold")
    ax.text(0.7, -0.15, "H$_c$", fontsize=10, color=INK, fontweight="bold")

    ax.set_xlim(-3.2, 3.2)
    ax.set_ylim(-1.4, 1.4)
    ax.set_xlabel("MAGNETFÄLT  H  [A/m]", fontsize=9, fontweight="bold")
    ax.set_ylabel("MAGNETISERING  B  [T]", fontsize=9, fontweight="bold")
    ax.set_title("TAPE B-H-HYSTERES  (modellkärnan)",
                 fontsize=11, fontweight="bold", color=INK,
                 loc="left", pad=10)
    ax.grid(True, color=GRID, linewidth=0.4, alpha=0.6)
    leg = ax.legend(loc="upper left", frameon=False, fontsize=8)
    for text in leg.get_texts():
        text.set_color(SOFT)

    # Höger: Bias-linjarisering
    ax = axes[1]
    t = np.linspace(0, 0.001, 4000)  # 1 ms

    # Audio (1 kHz sin)
    audio = 0.5 * np.sin(2 * np.pi * 1000 * t)
    # Bias (100 kHz sin)
    bias = 0.8 * np.sin(2 * np.pi * 100_000 * t)

    # Composite
    composite = audio + bias

    # Genom hysteres (avg över bias-cykel ger linjariserad output)
    # Förenklad modell: tanh av composite, men medelvärdet över bias-perioden ger en linjarisering
    output_nonlinear = np.tanh(2.0 * audio)  # ren saturation utan bias
    output_with_bias = audio  # bias-linjariserat (idealfall)

    ax.plot(t * 1000, composite, color=GRID, linewidth=0.6,
            label="Audio + bias 100 kHz", alpha=0.7)
    ax.plot(t * 1000, audio, color=INK, linewidth=1.4, label="Audio 1 kHz (in)")
    ax.plot(t * 1000, output_nonlinear, color=SOFT, linewidth=1.2,
            linestyle="--", label="Utan bias (saturerad)")
    ax.plot(t * 1000, output_with_bias, color=RED, linewidth=1.6,
            label="Med bias (linjariserad)")

    ax.set_xlim(0, 1)
    ax.set_ylim(-1.5, 1.5)
    ax.set_xlabel("TID  [ms]", fontsize=9, fontweight="bold")
    ax.set_ylabel("AMPLITUD", fontsize=9, fontweight="bold")
    ax.set_title("BIAS-INJEKTION  (100 kHz · linjariserar B-H)",
                 fontsize=11, fontweight="bold", color=INK,
                 loc="left", pad=10)
    ax.grid(True, color=GRID, linewidth=0.4, alpha=0.6)
    ax.axhline(0, color=INK, linewidth=0.5, alpha=0.7)
    leg = ax.legend(loc="upper right", frameon=False, fontsize=7.5)
    for text in leg.get_texts():
        text.set_color(INK)

    fig.suptitle("TAPE-MODELL  ·  HYSTERES + 100 kHz BIAS-LINJARISERING",
                 fontsize=12, fontweight="bold", color=INK, y=0.99)

    fig.text(0.5, 0.01,
             "Källa: Jiles-Atherton-approximation  ·  Bias-strom 2.3 mA enligt servicemanual s.2",
             fontsize=7.5, color=SOFT, ha="center", style="italic")

    plt.tight_layout(rect=[0, 0.03, 1, 0.96])
    out_path = OUT_DIR / "tape_hysteresis.png"
    plt.savefig(out_path, dpi=160, bbox_inches="tight")
    plt.close()
    print(f"OK  {out_path.name}")


def main():
    render_din1962_eq()
    render_tape_hysteresis()


if __name__ == "__main__":
    main()
