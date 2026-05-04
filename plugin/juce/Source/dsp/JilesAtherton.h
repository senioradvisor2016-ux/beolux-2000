/*  JilesAtherton — magnetisk hysteres-modell för tape-saturation.

    Ersätter den tidigare memoryless tanh-baserade Langevin-approximationen
    i TapeSaturation med en SANT history-dependent magnetiseringsmodell:

        H_e   = H + α · M                         (effektivt fält)
        M_an  = M_s · (coth(H_e/a) − a/H_e)        (anhysteretisk)
        dM/dH = (M_an − M) / (k·δ − α(M_an−M))     (irreversibel)
        M_rev = c · (M_an − M)                    (reversibel)
        M_tot = M_irr + M_rev

    Karakteristik:
      - 3:e-harmonik-dominans (vs 2:a från memoryless tanh)
      - Inter-block-modulation från magnetiskt minne
      - Print-through-likt beteende automatiskt
      - Korrekt asymmetrisk attack/decay på transients

    Referens: Jiles & Atherton (1986).

    Plats: plugin/juce/Source/dsp/JilesAtherton.h
*/
#pragma once

#include <cmath>

namespace bc2000dl::dsp
{
    class JilesAtherton
    {
    public:
        struct Params
        {
            double Ms    = 1.0;     // saturation magnetization (normaliserad)
            double a     = 0.30;    // Langevin shape (mindre = mjukare knee)
            double k     = 0.10;    // pinning (mindre = mjukare hysteres)
            double alpha = 0.0016;  // mean-field coupling (svag för audio)
            double c     = 0.18;    // reversible-andel
        };

        // Tape-formel-presets (matchar Python jiles_atherton.TAPE_PRESETS)
        static Params presetAgfa()    { return { 1.00, 0.30, 0.10, 0.0016, 0.18 }; }
        static Params presetBASF()    { return { 1.00, 0.28, 0.07, 0.0014, 0.22 }; }
        static Params presetScotch()  { return { 1.00, 0.34, 0.13, 0.0020, 0.15 }; }

        JilesAtherton() = default;

        void setParams (const Params& p) { params = p; }

        void reset()
        {
            M = 0.0;
            H_prev = 0.0;
        }

        // Sample-för-sample-process (history-dependent)
        inline float processSample (float H) noexcept
        {
            // NaN/Inf guard — if either the input or our own state has gone bad,
            // reset silently and return zero.  Without this, a single NaN locks
            // the model permanently: H_eff = H + α·NaN = NaN → L = NaN → M = NaN.
            // The comparison-based Ms-clamp below won't catch NaN (NaN > x = false),
            // so NaN in M would propagate for ever.
            if (! std::isfinite (H) || ! std::isfinite (M))
            {
                M = 0.0;
                H_prev = 0.0;
                return 0.0f;
            }

            const double Hd = static_cast<double> (H);
            const double H_eff = Hd + params.alpha * M;
            const double L = langevin (H_eff / params.a);
            const double M_an = params.Ms * L;

            const double delta = (Hd >= H_prev) ? 1.0 : -1.0;

            double denom = params.k * delta - params.alpha * (M_an - M);
            // Tighter clamp (1e-6 vs old 1e-12): the wider clamp allowed
            // dM_irr_dH to reach ~2e12 producing enormous (though later clamped)
            // magnetisation swings.  1e-6 is still far below any physical regime
            // (k ≈ 0.07–0.13 normally keeps |denom| >> 0.001) but caps the ratio
            // at ~2e6 × dH, preventing extreme transients on parameter switches.
            if (std::abs (denom) < 1e-6)
                denom = (denom >= 0.0 ? 1e-6 : -1e-6);

            const double dM_irr_dH = (M_an - M) / denom;
            const double dH = Hd - H_prev;
            M += dM_irr_dH * dH;

            // Klippa till fysikaliska gränser
            if (M >  params.Ms) M =  params.Ms;
            if (M < -params.Ms) M = -params.Ms;

            const double M_rev = params.c * (M_an - M);
            const double out = M + M_rev;

            H_prev = Hd;
            return static_cast<float> (out);
        }

    private:
        Params params { presetAgfa() };
        double M { 0.0 };
        double H_prev { 0.0 };

        // L(t) = coth(t) − 1/t. Numeriskt stabil med Taylor nära 0.
        static inline double langevin (double t) noexcept
        {
            const double at = std::abs (t);
            if (at < 1e-4)
                return t / 3.0 - (t * t * t) / 45.0;          // Taylor
            if (at > 50.0)
                return (t > 0.0) ? 1.0 : -1.0;                // mättad
            return 1.0 / std::tanh (t) - 1.0 / t;
        }
    };
}
