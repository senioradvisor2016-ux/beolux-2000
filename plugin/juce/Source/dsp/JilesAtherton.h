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
            // NaN-guard på input — annars förorenar M/H_prev permanent
            if (! std::isfinite (H))
                H = 0.0f;

            const double Hd = static_cast<double> (H);
            const double H_eff = Hd + params.alpha * M;
            const double L = langevin (H_eff / params.a);
            const double M_an = params.Ms * L;

            const double delta = (Hd >= H_prev) ? 1.0 : -1.0;

            // Robustare denom-guard.  Vid små k (BASF=0.07) och stora |M_an-M|
            // kan denom byta tecken över ett sample, vilket ger explosiv
            // dM/dH och numerisk drift.  Använd k som golv (≥ k/2).
            const double rawDenom = params.k * delta - params.alpha * (M_an - M);
            const double minDenom = params.k * 0.5;  // tillräckligt för stabilitet
            double denom = rawDenom;
            if (std::abs (denom) < minDenom)
                denom = (denom >= 0.0 ? minDenom : -minDenom);

            const double dM_irr_dH = (M_an - M) / denom;
            const double dH = Hd - H_prev;
            const double dMnew = dM_irr_dH * dH;

            // Begränsa per-sample-förändring för stabilitet
            const double dMclamped = std::max (-0.5, std::min (0.5, dMnew));
            M += dMclamped;

            // Klippa till fysikaliska gränser
            if (M >  params.Ms) M =  params.Ms;
            if (M < -params.Ms) M = -params.Ms;

            const double M_rev = params.c * (M_an - M);
            double out = M + M_rev;

            // Slutlig NaN-guard — om något ändå läcker igenom, returnera
            // 0 och nollställ state så nästa sample kan börja från ren grund.
            if (! std::isfinite (out))
            {
                out = 0.0;
                M = 0.0;
            }

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
