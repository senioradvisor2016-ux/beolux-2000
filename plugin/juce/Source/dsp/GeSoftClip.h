/*  GeSoftClip — delad Ebers-Moll-fit-waveshaper med ADAA (1:a ordningen).

    Germanium-stegen kör i BAS-samplerate (utanför tape-oversamplingen),
    så tanh-knee-distortionen aliasar utan motåtgärd. ADAA1 (antiderivative
    anti-aliasing) ersätter punktevalueringen med differenskvoten av
    antiderivatan — men appliceras ENDAST på den olinjära residualen
    g(u) = f(u) − u:

        y[n] = u[n] + (G(u[n]) − G(u[n−1])) / (u[n] − u[n−1]),
        G(u) = F(u) − u²/2

    Naiv ADAA på hela f skulle ge 2-taps-medelvärde (cos(πf/fs)) även i
    linjär region — −12 dB @ 20 kHz per steg, katastrofalt i en kaskad.
    Aliasing uppstår bara i residualen, så bara den behöver filtreringen;
    den linjära delen passerar med flat respons och noll fördröjning.

    Antiderivatan är analytisk per halvvåg:
        f(u) =  kneePos·tanh(u/kneePos)        u ≥ 0
        f(u) = −kneeNeg·tanh(−u/kneeNeg)       u < 0
        F(u) =  kneePos²·ln cosh(u/kneePos)    u ≥ 0
        F(u) =  kneeNeg²·ln cosh(u/kneeNeg)    u < 0
    (F är C¹-kontinuerlig i 0 eftersom f(0)=0 och F(0)=0 för båda grenarna.)

    Plats: plugin/juce/Source/dsp/GeSoftClip.h
*/

#pragma once

#include <algorithm>
#include <cmath>
#include "Constants.h"

namespace bc2000dl::dsp::detail
{
    // Overflow-säker ln(cosh(z)): för |z| > 20 är ln cosh z ≈ |z| − ln 2
    // (skillnaden < 1e-17) och std::cosh skulle annars överflöda vid ~710.
    inline double lnCosh (double z) noexcept
    {
        const double az = std::abs (z);
        return az > 20.0 ? az - 0.6931471805599453
                         : std::log (std::cosh (az));
    }

    struct GeSoftClipADAA
    {
        double prevU { 0.0 };

        static void computeKnees (double asym, double Vt,
                                  double& kPos, double& kNeg) noexcept
        {
            // Knee Vt*100 — se historik i Ge2N2613Stage.cpp (v62.5).
            const double baseKnee = std::max (Vt * 100.0, 0.5);
            const double a = std::clamp (asym * kAsymmetryGain, -0.7, 0.7);
            kPos = baseKnee * (1.0 + a);
            kNeg = baseKnee * (1.0 - a);
        }

        static double f (double u, double kPos, double kNeg) noexcept
        {
            return u >= 0.0 ?  kPos * std::tanh ( u / kPos)
                            : -kNeg * std::tanh (-u / kNeg);
        }

        static double F (double u, double kPos, double kNeg) noexcept
        {
            return u >= 0.0 ? kPos * kPos * lnCosh (u / kPos)
                            : kNeg * kNeg * lnCosh (u / kNeg);
        }

        // Olinjär residual och dess antiderivata
        static double g (double u, double kPos, double kNeg) noexcept
        {
            return f (u, kPos, kNeg) - u;
        }

        static double G (double u, double kPos, double kNeg) noexcept
        {
            return F (u, kPos, kNeg) - 0.5 * u * u;
        }

        double process (double x, double asym, double Vt) noexcept
        {
            constexpr double scale = 0.9;
            const double u = x * scale;

            double kPos, kNeg;
            computeKnees (asym, Vt, kPos, kNeg);

            const double du = u - prevU;
            const double res = (std::abs (du) > 1.0e-6)
                ? (G (u, kPos, kNeg) - G (prevU, kPos, kNeg)) / du
                : g (0.5 * (u + prevU), kPos, kNeg);

            prevU = u;
            return (u + res) / scale;
        }

        void reset() noexcept { prevU = 0.0; }
    };
}
