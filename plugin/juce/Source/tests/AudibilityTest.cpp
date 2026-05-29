/*  AudibilityTest — verifierar att VARJE parameter har mätbar effekt
    på audio-output.  Letar efter "döda kontroller" (som bias-buggen).

    Strategi: rendera 8192 samples med signal-A-konfig, sedan med
    signal-B-konfig (en parameter skiljer).  Räkna RMS-skillnad mellan
    de två outputs.  < kAudibleThreshold dB = misstänkt död kontroll.

    Bygg och kör:
      cmake --build build --config Release --target BC2000DL_AudibilityTest
      ./build/BC2000DL_AudibilityTest_artefacts/Release/BC2000DL_AudibilityTest
*/

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>

#include "../dsp/SignalChain.h"

#include <cmath>
#include <cstdio>
#include <functional>
#include <vector>
#include <string>

using namespace bc2000dl;
using namespace bc2000dl::dsp;

static int gPass = 0;
static int gFail = 0;
static int gSkip = 0;

static void report (const std::string& name, bool ok, const char* detail = "", bool skip = false)
{
    if (skip) { ++gSkip; std::printf ("[SKIP]  %-52s  %s\n", name.c_str(), detail); return; }
    if (ok) ++gPass; else ++gFail;
    std::printf ("%s  %-52s  %s\n", ok ? "[PASS]" : "[FAIL]", name.c_str(), detail);
}

static constexpr double kSR    = 48000.0;
static constexpr int    kBlock = 256;
static constexpr int    kCaptureSamples = 8192;

// "Audibelt" tröskelvärde: ≥0.3 dB skillnad i RMS, ELLER ≥0.005 i normaliserad
// signal-skillnad RMS.  För dyamiskt skillnaden räcker den ena.
static constexpr float kMinDbDelta = 0.3f;
static constexpr float kMinDiffRms = 0.005f;

static SignalChain::Parameters baselineParams()
{
    SignalChain::Parameters p;
    p.speed = TapeSpeed::Speed19;
    p.micGain = 0.7f;   p.micGainR = 0.7f;
    p.phonoGain = 0.0f; p.phonoGainR = 0.0f;
    p.radioGain = 0.0f; p.radioGainR = 0.0f;
    p.bassDb = 0.0f;
    p.trebleDb = 0.0f;
    p.balance = 0.0f;
    p.masterVolume = 0.85f;
    p.biasAmount = 1.0f;
    p.saturationDrive = 1.0f;
    p.saturationDriveR = 1.0f;
    p.wowFlutterAmount = 0.0f;     // off så vi mäter deterministiskt
    p.echoEnabled = false;
    p.echoAmount = 0.0f;
    p.echoAmountR = 0.0f;
    p.bypassTape = false;
    p.speakerMonitor = false;
    p.synchroplay = false;
    p.multiplayGen = 1;
    p.micLoZ = true;
    p.soundOnSound = false;
    p.publicAddress = false;
    p.monitorTrack1 = true;
    p.monitorTrack2 = true;
    p.monitorMode = 1;             // Tape
    p.phonoMode = 1;
    p.tapeFormula = 0;             // Agfa
    p.printThrough = 0.0f;
    p.stereoAsymmetry = 0.0f;
    p.radioMode = 0;
    return p;
}

/* Rendera kCaptureSamples med given parameter-uppsättning.
   stereoMode = false: mono signal duplicerat på L+R.
   stereoMode = true:  L = sin(f), R = sin(f * 1.5) → kan särskilja routing-buggar.
   warmupBlocks: > 300 om printThrough ska mätas (1.5 s buffer).  */
static std::vector<float> render (SignalChain::Parameters p,
                                   double freqHz = 1000.0,
                                   float amp = 0.5f,
                                   int channel = 0,
                                   bool stereoMode = false,
                                   int warmupBlocks = 30)
{
    SignalChain chain;
    chain.prepare (kSR, kBlock);
    chain.setParameters (p);

    juce::AudioBuffer<float> buf (2, kBlock);
    double phaseL = 0.0, phaseR = 0.0;
    const double incL = juce::MathConstants<double>::twoPi * freqHz / kSR;
    const double incR = juce::MathConstants<double>::twoPi * (freqHz * 1.5) / kSR;

    auto fillBlock = [&] ()
    {
        auto* L = buf.getWritePointer (0);
        auto* R = buf.getWritePointer (1);
        for (int i = 0; i < kBlock; ++i)
        {
            L[i] = static_cast<float> (std::sin (phaseL) * amp);
            R[i] = stereoMode ? static_cast<float> (std::sin (phaseR) * amp)
                              : L[i];
            phaseL += incL;
            phaseR += incR;
            if (phaseL >= juce::MathConstants<double>::twoPi) phaseL -= juce::MathConstants<double>::twoPi;
            if (phaseR >= juce::MathConstants<double>::twoPi) phaseR -= juce::MathConstants<double>::twoPi;
        }
    };

    for (int b = 0; b < warmupBlocks; ++b)
    {
        fillBlock();
        chain.process (buf);
    }

    std::vector<float> out;
    out.reserve (kCaptureSamples);
    while ((int) out.size() < kCaptureSamples)
    {
        fillBlock();
        chain.process (buf);
        const auto* d = buf.getReadPointer (channel);
        for (int i = 0; i < kBlock && (int) out.size() < kCaptureSamples; ++i)
            out.push_back (d[i]);
    }
    return out;
}

static float rmsDb (const std::vector<float>& v)
{
    double s = 0.0;
    for (float x : v) s += (double) x * x;
    const float rms = std::sqrt (static_cast<float> (s / std::max ((size_t) 1, v.size())));
    return 20.0f * std::log10 (rms + 1e-9f);
}

static float diffRms (const std::vector<float>& a, const std::vector<float>& b)
{
    const size_t n = std::min (a.size(), b.size());
    double s = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        const double d = (double) a[i] - (double) b[i];
        s += d * d;
    }
    return std::sqrt (static_cast<float> (s / std::max ((size_t) 1, n)));
}

/* Kör test: en parameter ska ändra output mätbart.
   Returnerar (passed, detail-string). */
static void sweepParam (const std::string& paramName,
                        std::function<void (SignalChain::Parameters&)> setA,
                        std::function<void (SignalChain::Parameters&)> setB,
                        double freqHz = 1000.0,
                        float amp = 0.5f,
                        int channel = 0,
                        bool stereoMode = false,
                        int warmupBlocks = 30)
{
    auto pA = baselineParams();  setA (pA);
    auto pB = baselineParams();  setB (pB);

    auto outA = render (pA, freqHz, amp, channel, stereoMode, warmupBlocks);
    auto outB = render (pB, freqHz, amp, channel, stereoMode, warmupBlocks);

    const float rmsA = rmsDb (outA);
    const float rmsB = rmsDb (outB);
    const float dDb  = std::abs (rmsA - rmsB);
    const float dRms = diffRms (outA, outB);

    const bool audible = (dDb >= kMinDbDelta) || (dRms >= kMinDiffRms);

    char detail[160];
    std::snprintf (detail, sizeof (detail),
        "A=%.2f dB  B=%.2f dB  ΔdB=%.2f  ΔRMS=%.5f",
        rmsA, rmsB, dDb, dRms);
    report (paramName, audible, detail);
}

int main()
{
    std::printf ("══════════════════════════════════════════════════════════════════\n");
    std::printf ("  BC2000DL Audibility Test  —  varje parameter ska påverka audio\n");
    std::printf ("══════════════════════════════════════════════════════════════════\n");
    std::printf ("Kriterium: ΔRMS ≥ %.4f  ELLER  ΔdB ≥ %.2f dB\n\n", kMinDiffRms, kMinDbDelta);

    // ===== Input-fader (L/R) =====
    sweepParam ("mic_gain (L)",
        [](auto& p){ p.micGain = 0.3f; },
        [](auto& p){ p.micGain = 0.9f; });
    sweepParam ("mic_gain_r (R, mätt på R)",
        [](auto& p){ p.micGainR = 0.3f; },
        [](auto& p){ p.micGainR = 0.9f; }, 1000.0, 0.5f, 1);
    sweepParam ("phono_gain (L)",
        [](auto& p){ p.micGain = 0.0f; p.phonoGain = 0.3f; },
        [](auto& p){ p.micGain = 0.0f; p.phonoGain = 0.9f; });
    sweepParam ("phono_gain_r (R)",
        [](auto& p){ p.micGain = 0.0f; p.micGainR = 0.0f; p.phonoGainR = 0.3f; },
        [](auto& p){ p.micGain = 0.0f; p.micGainR = 0.0f; p.phonoGainR = 0.9f; }, 1000.0, 0.5f, 1);
    sweepParam ("radio_gain (L)",
        [](auto& p){ p.micGain = 0.0f; p.radioGain = 0.3f; },
        [](auto& p){ p.micGain = 0.0f; p.radioGain = 0.9f; });
    sweepParam ("radio_gain_r (R)",
        [](auto& p){ p.micGain = 0.0f; p.micGainR = 0.0f; p.radioGainR = 0.3f; },
        [](auto& p){ p.micGain = 0.0f; p.micGainR = 0.0f; p.radioGainR = 0.9f; }, 1000.0, 0.5f, 1);

    // ===== Tone control =====
    sweepParam ("bass_db (-12 vs +12 @ 100 Hz)",
        [](auto& p){ p.bassDb = -12.0f; },
        [](auto& p){ p.bassDb = 12.0f; }, 100.0);
    sweepParam ("treble_db (-12 vs +12 @ 8 kHz)",
        [](auto& p){ p.trebleDb = -12.0f; },
        [](auto& p){ p.trebleDb = 12.0f; }, 8000.0);

    // ===== Balance / master =====
    sweepParam ("balance (-1 vs +1, mätt L-kanal)",
        [](auto& p){ p.balance = -1.0f; },
        [](auto& p){ p.balance = +1.0f; });
    sweepParam ("master_volume",
        [](auto& p){ p.masterVolume = 0.2f; },
        [](auto& p){ p.masterVolume = 0.9f; });

    // ===== Tape parameters =====
    sweepParam ("bias_amount (0.5 vs 1.5)",
        [](auto& p){ p.biasAmount = 0.5f; p.saturationDrive = 1.5f; p.saturationDriveR = 1.5f; },
        [](auto& p){ p.biasAmount = 1.5f; p.saturationDrive = 1.5f; p.saturationDriveR = 1.5f; });
    sweepParam ("saturation_drive (L)",
        [](auto& p){ p.saturationDrive = 0.5f; },
        [](auto& p){ p.saturationDrive = 2.0f; });
    sweepParam ("saturation_drive_r (R)",
        [](auto& p){ p.saturationDriveR = 0.5f; },
        [](auto& p){ p.saturationDriveR = 2.0f; }, 1000.0, 0.5f, 1);
    sweepParam ("wow_flutter (0 vs 2.0 @ 1 kHz)",
        [](auto& p){ p.wowFlutterAmount = 0.0f; },
        [](auto& p){ p.wowFlutterAmount = 2.0f; });

    // ===== Echo =====
    sweepParam ("echo_enabled (off vs on@0.5)",
        [](auto& p){ p.echoEnabled = false; p.echoAmount = 0.5f; p.echoAmountR = 0.5f; },
        [](auto& p){ p.echoEnabled = true;  p.echoAmount = 0.5f; p.echoAmountR = 0.5f; });
    sweepParam ("echo_amount (L, 0.1 vs 0.9)",
        [](auto& p){ p.echoEnabled = true; p.echoAmount = 0.1f; p.echoAmountR = 0.1f; },
        [](auto& p){ p.echoEnabled = true; p.echoAmount = 0.9f; p.echoAmountR = 0.1f; });
    sweepParam ("echo_amount_r (R)",
        [](auto& p){ p.echoEnabled = true; p.echoAmount = 0.1f; p.echoAmountR = 0.1f; },
        [](auto& p){ p.echoEnabled = true; p.echoAmount = 0.1f; p.echoAmountR = 0.9f; }, 1000.0, 0.5f, 1);

    // ===== Mode flags =====
    sweepParam ("bypass_tape",
        [](auto& p){ p.bypassTape = false; },
        [](auto& p){ p.bypassTape = true; });
    sweepParam ("speaker_monitor (with int speaker)",
        [](auto& p){ p.speakerMonitor = false; },
        [](auto& p){ p.speakerMonitor = true; });
    sweepParam ("synchroplay",
        [](auto& p){ p.synchroplay = false; },
        [](auto& p){ p.synchroplay = true; });

    // ===== Multiplay =====
    sweepParam ("multiplay_gen (1 vs 5)",
        [](auto& p){ p.multiplayGen = 1; },
        [](auto& p){ p.multiplayGen = 5; });

    // ===== Monitor routing — kräver olika L/R för att synas =====
    sweepParam ("monitor_track1 only (L → R, stereo-input)",
        [](auto& p){ p.monitorTrack1 = true; p.monitorTrack2 = true; },
        [](auto& p){ p.monitorTrack1 = true; p.monitorTrack2 = false; },
        1000.0, 0.5f, 1, /*stereo=*/true);
    sweepParam ("monitor_track2 only (R → L, stereo-input)",
        [](auto& p){ p.monitorTrack1 = true; p.monitorTrack2 = true; },
        [](auto& p){ p.monitorTrack1 = false; p.monitorTrack2 = true; },
        1000.0, 0.5f, 0, /*stereo=*/true);
    sweepParam ("monitor_mode (Source vs Tape)",
        [](auto& p){ p.monitorMode = 0; },
        [](auto& p){ p.monitorMode = 1; });

    // ===== Sound-on-Sound (L → R cross-mix) =====
    sweepParam ("sos_enabled (L→R cross-mix, R-kanal mätt)",
        [](auto& p){ p.soundOnSound = false; p.micGainR = 0.0f; },
        [](auto& p){ p.soundOnSound = true;  p.micGainR = 0.0f; }, 1000.0, 0.5f, 1);

    // ===== P.A. Mode — duckning sker i PluginProcessor, inte SignalChain =====
    report ("pa_enabled (PluginProcessor-level, ej testbart här)",
            true, "skipped — PA-ducking i updateChainParameters()", true);

    // ===== Input modes =====
    sweepParam ("mic_loz (high-Z vs low-Z)",
        [](auto& p){ p.micLoZ = false; },
        [](auto& p){ p.micLoZ = true; });
    sweepParam ("phono_mode (L ceramic vs H mag/RIAA)",
        [](auto& p){ p.micGain = 0.0f; p.phonoGain = 0.7f; p.phonoMode = 0; },
        [](auto& p){ p.micGain = 0.0f; p.phonoGain = 0.7f; p.phonoMode = 1; }, 100.0);
    sweepParam ("radio_mode (L vs H sensitivity)",
        [](auto& p){ p.micGain = 0.0f; p.radioGain = 0.7f; p.radioMode = 0; },
        [](auto& p){ p.micGain = 0.0f; p.radioGain = 0.7f; p.radioMode = 1; });

    // ===== Tape formula =====
    sweepParam ("tape_formula (Agfa vs Scotch)",
        [](auto& p){ p.tapeFormula = 0; },
        [](auto& p){ p.tapeFormula = 2; });
    sweepParam ("tape_formula (BASF vs Scotch)",
        [](auto& p){ p.tapeFormula = 1; },
        [](auto& p){ p.tapeFormula = 2; });

    // ===== Speed =====
    sweepParam ("speed (19 vs 4.75 @ 8 kHz)",
        [](auto& p){ p.speed = TapeSpeed::Speed19; },
        [](auto& p){ p.speed = TapeSpeed::Speed475; }, 8000.0);

    // ===== Print-through — print-buffer är 1.5 s lång → kräver långt warmup =====
    sweepParam ("print_through (0 vs 0.05, lång warmup)",
        [](auto& p){ p.printThrough = 0.0f; },
        [](auto& p){ p.printThrough = 0.05f; },
        1000.0, 0.5f, 0, /*stereo=*/false, /*warmupBlocks=*/350);

    // ===== Stereo asymmetry — by design subtle, hot signal för att synas =====
    sweepParam ("stereo_asymmetry (0 vs 0.05, hot signal)",
        [](auto& p){ p.stereoAsymmetry = 0.0f; p.saturationDrive = 2.0f; p.saturationDriveR = 2.0f; },
        [](auto& p){ p.stereoAsymmetry = 0.05f; p.saturationDrive = 2.0f; p.saturationDriveR = 2.0f; },
        1000.0, 0.9f, 1);

    // ===== v60.3 — NORMAL/HIGH bias (schema 9224002) — HF-fokus (CrO₂ = bättre HF) =====
    sweepParam ("bias_type (NORMAL vs HIGH CrO₂) @ 8 kHz",
        [](auto& p){ p.biasType = 0; },
        [](auto& p){ p.biasType = 1; }, 8000.0);

    // ===== v60.3 — Track-width 1/4 vs 1/2 (schema 9224003) — LF-fokus (head-bump) =====
    sweepParam ("track_width (1/4 vs 1/2 stereo) @ 100 Hz",
        [](auto& p){ p.trackWidth = 0; },
        [](auto& p){ p.trackWidth = 1; }, 100.0);

    std::printf ("\n══════════════════════════════════════════════════════════════════\n");
    std::printf ("  RESULTAT:  %d godkända   %d underkända   %d hoppade\n", gPass, gFail, gSkip);
    std::printf ("══════════════════════════════════════════════════════════════════\n");
    return gFail > 0 ? 1 : 0;
}
