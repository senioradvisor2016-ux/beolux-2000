"""BC2000DL DSP-prototyp — Fas 1 byggstenar.

Klasser:
- Ge2N2613Stage: PNP germanium small-signal amp, brus + asymm. soft-clip
- GeLowNoiseStage: UW0029 (PNP) / AC126 (NPN) lågbrus-input-stage

Konstanter och målspecifikationer i ../specs.md.
"""
from .ge_stages import (
    Ge2N2613Stage,
    GeLowNoiseStage,
    GeStageType,
    GE_VT_25C,
)

__all__ = [
    "Ge2N2613Stage",
    "GeLowNoiseStage",
    "GeStageType",
    "GE_VT_25C",
]

__version__ = "0.1.0"
