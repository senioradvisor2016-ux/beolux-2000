/*  BC2000DL — DSP-konstanter

    Speglar specs.md §12. Härledda från 2N2613-databladet, B&O-servicemanual
    och komponentnivå-analys.

    Plats: plugin/juce/Source/dsp/Constants.h
*/

#pragma once

namespace bc2000dl::dsp
{
    // ---------- Germanium-transistor-konstanter ----------
    constexpr double kVT_25C        = 0.02585;   // V (kT/q vid 25 °C)

    // Saturation currents
    constexpr double kIs_2N2613     = 1.0e-7;
    constexpr double kIs_UW0029     = 0.7e-7;    // utvald lågbrus
    constexpr double kIs_AC126      = 0.7e-7;    // NPN motsvarighet

    // Brus (input-refererat, V RMS över 20 Hz–20 kHz).
    // Kalibrerat v56.0 — 2.8× lägre än v55 → chain S/N ≥ 55 dB (servicemanualens spec).
    // Validering: S/N 46 dB → 55 dB uppmätt med −20 dBFS testsignal,
    // full 13-stage pipeline aktiv (inkl. GE-cascade + tape + DC-block).
    constexpr double kNoiseVrms_2N2613 = 2.9e-6;  // ×0.36 — tuned for 55 dB chain S/N
    constexpr double kNoiseVrms_UW0029 = 1.8e-6;  // ×0.36 — tuned for 55 dB chain S/N
    constexpr double kNoiseVrms_AC126  = 2.1e-6;  // ×0.36 — tuned for 55 dB chain S/N

    // Asymmetri-bias för waveshaper. Reducerade värden (oktober 2026) — tidigare
    // 0.10 × 3.5 = 0.35 effektiv asymmetri per stage gav 30+ % cascade-THD.
    // Nu 0.025 × 1.0 = 0.025 per stage → ~1 % h2 per stage → cascade 3-5 %
    // (matchar databladets 2N2613 typ-värde och Studio-Sound 1968 mätspec).
    constexpr double kAsymmetryPNP = +0.025;
    constexpr double kAsymmetryNPN = -0.025;
    constexpr double kAsymmetryGain = 1.0;

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

    // Tape-bandbredd (HF-corner, Hz)
    constexpr double kTapeHF_Speed19  = 20000.0;
    constexpr double kTapeHF_Speed95  = 12000.0;
    constexpr double kTapeHF_Speed475 = 6000.0;

    // Tape-egenbrus (dBFS) per hastighet — kalibrerat så total chain S/N ≥ 55 dB.
    constexpr double kTapeNoise_dB_Speed19  = -82.0;
    constexpr double kTapeNoise_dB_Speed95  = -76.0;
    constexpr double kTapeNoise_dB_Speed475 = -70.0;

    // ---------- Bias ----------
    constexpr double kBiasFreq_Hz       = 100000.0;
    constexpr double kBiasNominal_mA    = 2.3;
    constexpr double kEraseFreq_Hz      = 100000.0;
    constexpr double kEraseNominal_mA   = 45.0;

    // ---------- Output ----------
    constexpr double kReferenceLevel_dBu = 0.0;     // 0 dBu = 0.775 V RMS
    constexpr double kReferenceVoltage   = 0.775;
    constexpr double kAutomatsikring_dBu = 14.0;    // soft-clip-tröskel
}
