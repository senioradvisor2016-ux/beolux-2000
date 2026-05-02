"""Jiles-Atherton magnetisk hysteres-modell för tape.

Ersätter den tidigare tanh-baserade anhysteretiska approximationen i
TapeSaturation med en SANT history-dependent magnetiserings-modell:

    H_e = H + α × M                         (effektivt fält)
    M_an = M_s × (coth(H_e/a) - a/H_e)      (anhysteretisk Langevin)
    dM/dH = (M_an − M) / (k × δ − α(M_an−M))   (irreversibel)
    M_rev = c × (M_an − M)                  (reversibel komponent)
    M_total = M_irr + M_rev

Detta ger:
- Autentisk 3:e-harmoniks-dominans (vs 2:a från memoryless tanh)
- Inter-block-modulation från magnetiskt minne
- Print-through-likt beteende automatiskt
- Korrekt asymmetrisk attack/decay på transients

Referens: Jiles & Atherton (1986), audio-rate-stabiliserad form från
Holman Audio papers + Universal Audio whitepapers.
"""
from __future__ import annotations
import numpy as np


class JilesAtherton:
    """5-parameters J-A-modell.

    Parametrar för 1968-formel-tape (Agfa-typ) som default:
      M_s   = 1.0    saturation magnetization (normaliserad)
      a     = 0.30   Langevin shape (mindre → mjukare knee)
      k     = 0.10   pinning factor (mindre → mjukare hysteres-loop)
      α     = 0.0016 mean-field coupling (svag, för audio-rates)
      c     = 0.18   reversible-andel (kombination irr/rev)
    """

    def __init__(self, *, Ms: float = 1.0, a: float = 0.30,
                 k: float = 0.10, alpha: float = 0.0016, c: float = 0.18):
        self.Ms = Ms
        self.a = a
        self.k = k
        self.alpha = alpha
        self.c = c
        self.M = 0.0
        self.H_prev = 0.0

    def reset(self):
        self.M = 0.0
        self.H_prev = 0.0

    @staticmethod
    def _langevin(t: float) -> float:
        """L(t) = coth(t) − 1/t  (Langevin function), numeriskt stabil."""
        if abs(t) < 1e-4:
            # Taylor-expansion runt 0: L(t) ≈ t/3 - t^3/45 + 2t^5/945
            return t / 3.0 - (t ** 3) / 45.0
        if abs(t) > 50.0:
            return 1.0 if t > 0 else -1.0
        return 1.0 / np.tanh(t) - 1.0 / t

    def process_sample(self, H: float) -> float:
        H_eff = H + self.alpha * self.M
        L = self._langevin(H_eff / self.a)
        M_an = self.Ms * L

        # Riktning av dH/dt avgör hysteres-direction
        delta = 1.0 if H >= self.H_prev else -1.0

        # Irreversibel komponent
        denom = self.k * delta - self.alpha * (M_an - self.M)
        if abs(denom) < 1e-12:
            denom = 1e-12 if denom >= 0 else -1e-12
        dM_irr_dH = (M_an - self.M) / denom

        dH = H - self.H_prev
        self.M += dM_irr_dH * dH

        # Klippa M till fysikaliska gränser (annars skenar)
        if self.M > self.Ms:  self.M = self.Ms
        if self.M < -self.Ms: self.M = -self.Ms

        # Reversibel komponent (ger den "snabba" delen av responsen)
        M_rev = self.c * (M_an - self.M)
        out = self.M + M_rev

        self.H_prev = H
        return out

    def process(self, x: np.ndarray) -> np.ndarray:
        out = np.empty_like(x)
        for i in range(len(x)):
            out[i] = self.process_sample(float(x[i]))
        return out


class JilesAthertonStereo:
    """Stereo-par med per-kanal-asymmetri (autentisk 1968-tolerans)."""

    def __init__(self, **kwargs):
        # Per-kanal-tolerans: ±2% på k och α (inom hardware-spec)
        ka = kwargs.copy()
        kb = kwargs.copy()
        ka["k"] = kwargs.get("k", 0.10) * 1.02
        kb["k"] = kwargs.get("k", 0.10) * 0.98
        ka["alpha"] = kwargs.get("alpha", 0.0016) * 1.02
        kb["alpha"] = kwargs.get("alpha", 0.0016) * 0.98
        self.left = JilesAtherton(**ka)
        self.right = JilesAtherton(**kb)


# ---------- Tape-formel-presets ----------
# Kalibrerade per formel-typ enligt plan.md §7
TAPE_PRESETS = {
    "Agfa":   dict(Ms=1.00, a=0.30, k=0.10, alpha=0.0016, c=0.18),  # varm
    "BASF":   dict(Ms=1.00, a=0.28, k=0.07, alpha=0.0014, c=0.22),  # raffinerad HF
    "Scotch": dict(Ms=1.00, a=0.34, k=0.13, alpha=0.0020, c=0.15),  # komprimerad
}
