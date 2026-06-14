/*  BC2000DL — DSP-konstanter

    Speglar specs.md §12. Härledda från 2N2613-databladet, B&O-servicemanual
    och komponentnivå-analys.

    Plats: plugin/juce/Source/dsp/Constants.h
*/

#pragma once

#include <cstdint>
#include <algorithm>   // std::fill/min/max/clamp — MSVC ger dem inte transitivt
#include <cmath>       // std::pow/exp/tanh m.fl. — samma skäl

namespace bc2000dl::dsp
{
    // ---------- Germanium-transistor-konstanter ----------
    constexpr double kVT_25C        = 0.02585;   // V (kT/q vid 25 °C)

    // Saturation currents (OBS: ej använd av GeLowNoiseStage-shapern — knäet
    // sätts av Vt·100, se GeSoftClip. Behålls som referens/för ev. framtida fit.)
    constexpr double kIs_2N2613     = 1.0e-7;
    constexpr double kIs_UW0029     = 0.7e-7;    // (germanium-fit; UW0029 är dock kisel — se nedan)
    constexpr double kIs_AC126      = 0.7e-7;    // germanium PNP (rec/play-amp, kort 8004005/6)

    // ---- UW0029 = KISEL-NPN (BC109-ekvivalent) ----
    // Studio Sound aug 1968 (Hellyer, s.354): tidiga Beocord-enheter använde
    // UW0029, senare BC109 i samma preamp-ingångsposition → samma komponent.
    // BC109 = kisel-NPN lågbrus. Modelleras därför renare än germanium-stegen:
    // lägre brus, symmetriskt knä (ingen h2-värme), bredare knä (kisels högre
    // ledningströskel → linjärt längre). Germanium-färgningen bor i 2N2613/AC126
    // nedströms — den rörs inte. Se specs.md §12 (✅ LÖST).
    // Enkel-ändat kisel-klass-A-steg har inneboende 2:a-harmonisk (exponentiell
    // transferkurva), men mindre än germanium → ~40 % av germaniums asymmetri.
    // (0.0 = perfekt symmetriskt vore för idealiserat och raderar germanium-
    //  signaturen i kedjan; verifierat mot specrig h2>h3-grinden @ -12 dBFS.)
    constexpr double kAsymmetrySi   = 0.002;     // kisel: svag h2 (vs germanium 0.005)
    constexpr double kSiKneeFactor  = 1.5;       // bredare/renare knä vs germanium (eff. Vt ×1.5)

    // Brus (input-refererat, V RMS över 20 Hz–20 kHz).
    // Kalibrerat v56.0 — 2.8× lägre än v55 → chain S/N ≥ 55 dB (servicemanualens spec).
    // Validering: S/N 46 dB → 55 dB uppmätt med −20 dBFS testsignal,
    // full 13-stage pipeline aktiv (inkl. GE-cascade + tape + DC-block).
    constexpr double kNoiseVrms_2N2613 = 2.9e-6;  // ×0.36 — tuned for 55 dB chain S/N
    constexpr double kNoiseVrms_UW0029 = 1.4e-6;  // kisel-lågbrus (BC109-klass, ~0.78× ger) — sänkt v0.63
    constexpr double kNoiseVrms_AC126  = 2.1e-6;  // ×0.36 — tuned for 55 dB chain S/N

    // Asymmetri-bias för waveshaper. Reducerade värden (v62.1) — tidigare
    // 0.025 × 1.0 = 0.025 per stage gav i 4-stage cascade ~5-8% h2 totalt.
    // Nu 0.005 × 1.0 = 0.005 per stage → ~0.5% h2 cascade (under §4 spec).
    // Knappt audibelt mismatch men tillräckligt för stereo-asymmetri-character.
    constexpr double kAsymmetryPNP = +0.005;
    constexpr double kAsymmetryNPN = -0.005;
    // kAsymmetryGain höjd från 1.0 → 3.0 (v60.2):  user-kontrollerad
    // stereoAsymmetry (range 0..0.05) hade tidigare ΔRMS < 0.002 vid max →
    // upplevdes som "död kontroll".  Med gain 3.0 ger max-settingen tydlig
    // L/R 2nd-harmonic-mismatch (autentisk 1968-hardware-tolerance), medan
    // baseline (0.005 × 3 = 0.015) håller h2-cascade < 1.5 % — fortfarande
    // inom §4 THD-spec.
    constexpr double kAsymmetryGain = 3.0;

    // ---------- Tape-hastigheter ----------
    enum class TapeSpeed
    {
        Speed19    = 0,    // 19 cm/s = 7½ ips
        Speed95    = 1,    // 9.5 cm/s = 3¾ ips
        Speed475   = 2     // 4.75 cm/s = 1⅞ ips
    };

    // Echo-tid i ms per hastighet (record→play-head-offset)
    constexpr double kEchoTime_ms_Speed19  = 75.0;
    constexpr double kEchoTime_ms_Speed95  = 150.0;
    constexpr double kEchoTime_ms_Speed475 = 300.0;

    // Tape-bandbredd (HF-corner, Hz).  Höjd (v62.1) från spec-edge (20/12/6 kHz)
    // till 30/16/8 kHz.  Tidigare gav 1:a-ord LP -3 dB exakt vid spec-frekvensen,
    // vilket åt upp hela §2-marginalen.  Nya värden ger ~-1.5 dB vid spec-edge,
    // och naturlig HF-roll-off ovanför.
    constexpr double kTapeHF_Speed19  = 30000.0;
    constexpr double kTapeHF_Speed95  = 16000.0;
    constexpr double kTapeHF_Speed475 = 8000.0;

    // Tape-egenbrus (dBFS) per hastighet — kalibrerat så total chain S/N ≥ 55 dB.
    constexpr double kTapeNoise_dB_Speed19  = -82.0;
    constexpr double kTapeNoise_dB_Speed95  = -76.0;
    constexpr double kTapeNoise_dB_Speed475 = -70.0;

    // ---------- Bias ----------
    // MANUAL-VERIFIERAT (servicemanual Technische Daten s.2 + schema s.3):
    //   "Löschfrequenz: 100 kHz", "Formagnetiseringsstrom Bias 2,3 mA".
    // Endast kBiasFreq_Hz används i DSP:n (TapeSaturation HF-bias). Övriga tre är
    // dokumentations-konstanter (lästes ingenstans) — hålls korrekta mot original-
    // schemat (B&O TYPE 4119, Form 5010 10-66).
    constexpr double kBiasFreq_Hz       = 100000.0;   // manual: exakt 100 kHz  [ANVÄND]
    constexpr double kBiasNominal_mA    = 2.3;         // manual: exakt 2,3 mA   [doc]
    constexpr double kEraseFreq_Hz      = 100000.0;   // manual: exakt 100 kHz   [doc]
    constexpr double kEraseNominal_mA   = 45.0;        // original-schema TYPE 4119: 45 mA [doc]

    // ---------- Output ----------
    constexpr double kReferenceLevel_dBu = 0.0;     // 0 dBu = 0.775 V RMS
    constexpr double kReferenceVoltage   = 0.775;
    constexpr double kAutomatsikring_dBu = 14.0;    // soft-clip-tröskel

    // ---- Fast audio-quality Gaussian noise (RT-safe, no allocation) ----
    // Replaces std::mt19937 + std::normal_distribution<double> in GE stages.
    // 4-sample CLT approximation: sum of 4 ×  Uniform[-0.5, 0.5] → σ ≈ 0.577.
    // Scaled by √3 ≈ 1.732 to give σ = 1.0.  Finite support ±3.46σ — fine for
    // audio noise floors.  Speed: ~0.5 ns/call vs ~15 ns for normal_distribution.
    namespace detail
    {
        inline float fastGaussNoise (std::uint32_t& state) noexcept
        {
            auto u = [&]() noexcept -> float {
                state = state * 1664525u + 1013904223u;
                return static_cast<float> (static_cast<int32_t> (state))
                       * (0.5f / 2147483648.0f);   // [-0.5, 0.5)
            };
            return (u() + u() + u() + u()) * 1.7320508f;   // σ ≈ 1.0
        }
    }
}
