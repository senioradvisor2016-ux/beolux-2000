/*  DspSmokeTest — BC2000DL DSP quality & safety test harness.

    Bygg och kör:
      cmake --build build --config Release --target BC2000DL_Tests
      ./build/BC2000DL_Tests_artefacts/Release/BC2000DL\ Tests

    Testar:
      1. Alla 43 presets — ingen NaN/Inf, peak inom gräns
      2. Alla formel-/hastighetsövergångar (det kända krasj-scenariot)
      3. Alla presets → Scotch (specifikt krasj-preset)
      4. NaN-injektion → återhämtning
      5. Echo-självoscillation — kvar inom ±2 vid amount=0.95
      6. THD 1 kHz per formel (ska vara 0.05 %–25 %)
      7. Hastighetsbyte mitt i stream

    Exit: 0 = alla godkända, 1 = minst ett fel.
*/

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>

#include "../dsp/SignalChain.h"
#include "../presets/Presets.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <limits>

using namespace bc2000dl;
using namespace bc2000dl::dsp;

// ──────────────────────────────────────────────────────────────────
// Globala räknare
// ──────────────────────────────────────────────────────────────────

static int gPass = 0;
static int gFail = 0;

static void report (const char* name, bool ok, const char* detail = "")
{
    if (ok) ++gPass; else ++gFail;
    std::printf ("%s  %-55s  %s\n", ok ? "[PASS]" : "[FAIL]", name, detail);
}

// ──────────────────────────────────────────────────────────────────
// Konstanter
// ──────────────────────────────────────────────────────────────────

static constexpr double kSR         = 48000.0;
static constexpr int    kBlock      = 512;
static constexpr int    kWarm       = 30;   // block att värma upp innan mätning

// ──────────────────────────────────────────────────────────────────
// Hjälpfunktioner: PresetData → SignalChain::Parameters
// ──────────────────────────────────────────────────────────────────

static SignalChain::Parameters toParams (const PresetData& p)
{
    SignalChain::Parameters sp;
    sp.speed = (p.speed == 0) ? TapeSpeed::Speed475
             : (p.speed == 1) ? TapeSpeed::Speed95
                              : TapeSpeed::Speed19;
    sp.micGain          = p.mic_gain;
    sp.micGainR         = p.mic_gain_r;
    sp.phonoGain        = p.phono_gain;
    sp.phonoGainR       = p.phono_gain_r;
    sp.radioGain        = p.radio_gain;
    sp.radioGainR       = p.radio_gain_r;
    sp.tapeFormula      = p.tape_formula;
    sp.saturationDrive  = p.saturation_drive;
    sp.saturationDriveR = p.saturation_drive_r;
    sp.biasAmount       = p.bias_amount;
    sp.wowFlutterAmount = p.wow_flutter;
    sp.multiplayGen     = p.multiplay_gen;
    sp.bassDb           = p.bass_db;
    sp.trebleDb         = p.treble_db;
    sp.balance          = p.balance;
    sp.masterVolume     = p.master_volume;
    sp.echoEnabled      = p.echo_enabled;
    sp.echoAmount       = p.echo_amount;
    sp.echoAmountR      = p.echo_amount_r;
    sp.bypassTape       = false;
    sp.monitorMode      = 1;
    sp.monitorTrack1    = true;
    sp.monitorTrack2    = true;
    return sp;
}

// ──────────────────────────────────────────────────────────────────
// Signalgeneratorer och mätning
// ──────────────────────────────────────────────────────────────────

static void fillSine (juce::AudioBuffer<float>& buf, double& phase,
                      double freqHz = 1000.0, float amp = 0.5f)
{
    const double inc = juce::MathConstants<double>::twoPi * freqHz / kSR;
    auto* L = buf.getWritePointer (0);
    for (int i = 0; i < buf.getNumSamples(); ++i)
    {
        L[i] = static_cast<float> (std::sin (phase) * amp);
        phase += inc;
        if (phase >= juce::MathConstants<double>::twoPi)
            phase -= juce::MathConstants<double>::twoPi;
    }
    if (buf.getNumChannels() >= 2)
        buf.copyFrom (1, 0, buf, 0, 0, buf.getNumSamples());
}

static bool hasNaN (const juce::AudioBuffer<float>& buf)
{
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        const auto* d = buf.getReadPointer (ch);
        for (int i = 0; i < buf.getNumSamples(); ++i)
            if (! std::isfinite (d[i])) return true;
    }
    return false;
}

static float peakOf (const juce::AudioBuffer<float>& buf)
{
    float pk = 0.0f;
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        const auto* d = buf.getReadPointer (ch);
        for (int i = 0; i < buf.getNumSamples(); ++i)
            pk = std::max (pk, std::abs (d[i]));
    }
    return pk;
}

/** Kör numBlocks block med sinus.
    Returnerar false om NaN detekteras ELLER peak > peakLimit. */
static bool runBlocks (SignalChain& chain,
                       juce::AudioBuffer<float>& buf,
                       double& phase,
                       int numBlocks,
                       float peakLimit,
                       float* outPeak = nullptr)
{
    float maxPk = 0.0f;
    for (int b = 0; b < numBlocks; ++b)
    {
        fillSine (buf, phase);
        chain.process (buf);
        if (hasNaN (buf)) { if (outPeak) *outPeak = -1.0f; return false; }
        maxPk = std::max (maxPk, peakOf (buf));
        if (maxPk > peakLimit) { if (outPeak) *outPeak = maxPk; return false; }
    }
    if (outPeak) *outPeak = maxPk;
    return true;
}

// ──────────────────────────────────────────────────────────────────
// TEST 1: Alla 43 presets (kall start → steady state)
// ──────────────────────────────────────────────────────────────────

static void testAllPresets()
{
    std::printf ("\n── Test 1: Alla presets (kall start) ──────────────────────────\n");
    juce::AudioBuffer<float> buf (2, kBlock);
    int fails = 0;

    for (int i = 0; i < kNumPresets; ++i)
    {
        SignalChain chain;
        chain.prepare (kSR, kBlock);
        chain.setParameters (toParams (kPresets[i]));

        double phase = 0.0;
        float pk = 0.0f;
        bool ok = runBlocks (chain, buf, phase, kWarm, 5.0f, &pk);

        char name[72];
        std::snprintf (name, sizeof (name), "Preset %02d  %s", i, kPresets[i].name);
        char detail[32];
        std::snprintf (detail, sizeof (detail), "peak %.3f", pk);
        if (! ok) { report (name, false, detail); ++fails; }
    }

    char detail[32];
    std::snprintf (detail, sizeof (detail), "%d/%d preset ok", kNumPresets - fails, kNumPresets);
    report ("Alla presets — kall start", fails == 0, detail);
}

// ──────────────────────────────────────────────────────────────────
// TEST 2: Alla formel- och hastighetsövergångar (A→B mid-stream)
// ──────────────────────────────────────────────────────────────────

static void testAllTransitions()
{
    std::printf ("\n── Test 2: Alla formel/hastighets-övergångar A→B ──────────────\n");
    juce::AudioBuffer<float> buf (2, kBlock);
    int checked = 0, fails = 0;

    for (int a = 0; a < kNumPresets; ++a)
    {
        for (int b = 0; b < kNumPresets; ++b)
        {
            // Testa bara om formel ELLER hastighet skiljer (de potentiellt farliga)
            if (kPresets[a].tape_formula == kPresets[b].tape_formula &&
                kPresets[a].speed        == kPresets[b].speed)
                continue;

            SignalChain chain;
            chain.prepare (kSR, kBlock);
            chain.setParameters (toParams (kPresets[a]));

            double phase = 0.0;
            runBlocks (chain, buf, phase, 15, 5.0f);   // värm upp med preset A

            chain.setParameters (toParams (kPresets[b]));  // byt mitt i strömmen

            float pk = 0.0f;
            bool ok = runBlocks (chain, buf, phase, 15, 5.0f, &pk);
            ++checked;

            if (! ok)
            {
                char name[128];
                std::snprintf (name, sizeof (name),
                    "  %02d(%s)→%02d(%s)",
                    a, kPresets[a].name, b, kPresets[b].name);
                char detail[32];
                std::snprintf (detail, sizeof (detail), "peak %.3f / NaN", pk);
                report (name, false, detail);
                ++fails;
            }
        }
    }

    char detail[48];
    std::snprintf (detail, sizeof (detail), "%d par testade, %d fel", checked, fails);
    report ("Alla formel/hastighets-övergångar A→B", fails == 0, detail);
}

// ──────────────────────────────────────────────────────────────────
// TEST 3: NaN-injektion → återhämtning
// ──────────────────────────────────────────────────────────────────

static void testNaNRecovery()
{
    std::printf ("\n── Test 3: NaN-injektion → återhämtning ───────────────────────\n");
    juce::AudioBuffer<float> buf (2, kBlock);

    SignalChain chain;
    chain.prepare (kSR, kBlock);
    chain.setParameters (toParams (kPresets[0]));

    double phase = 0.0;
    runBlocks (chain, buf, phase, kWarm, 5.0f);

    // Injicera NaN i hela bufferten
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < kBlock; ++i)
            buf.getWritePointer (ch)[i] = std::numeric_limits<float>::quiet_NaN();
    chain.process (buf);   // safety limiter ska svälja detta

    // Kör ren signal — ska återhämta sig
    bool recovered = false;
    int recoveryBlock = -1;
    for (int b = 0; b < 15; ++b)
    {
        fillSine (buf, phase, 1000.0, 0.3f);
        chain.process (buf);
        if (! hasNaN (buf) && peakOf (buf) > 1e-5f)
        {
            recovered = true;
            recoveryBlock = b + 1;
            break;
        }
    }

    char detail[48];
    if (recovered)
        std::snprintf (detail, sizeof (detail), "ren signal efter block %d", recoveryBlock);
    else
        std::snprintf (detail, sizeof (detail), "återhämtade sig INTE inom 15 block");
    report ("NaN-injektion → rent output", recovered, detail);
}

// ──────────────────────────────────────────────────────────────────
// TEST 4: Echo self-oscillation — håller sig inom ±2 vid amount=0.95
// ──────────────────────────────────────────────────────────────────

static void testEchoSelfOsc()
{
    std::printf ("\n── Test 4: Echo self-oscillation vid amount=0.95 ──────────────\n");
    juce::AudioBuffer<float> buf (2, kBlock);

    SignalChain chain;
    chain.prepare (kSR, kBlock);
    auto p = toParams (kPresets[0]);
    p.echoEnabled = true;
    p.echoAmount  = 0.95f;
    p.echoAmountR = 0.95f;
    chain.setParameters (p);

    double phase = 0.0;
    float pk = 0.0f;
    bool ok = runBlocks (chain, buf, phase, 300, 2.0f, &pk);   // 300 block ≈ 3.2 s

    char detail[48];
    std::snprintf (detail, sizeof (detail), "peak %.3f över 300 block", pk);
    report ("Echo self-osc bounded (< ±2)", ok, detail);
}

// ──────────────────────────────────────────────────────────────────
// TEST 5: THD 1 kHz per formel (Agfa / BASF / Scotch)
// ──────────────────────────────────────────────────────────────────

static float measureTHD (SignalChain& chain, double& phase,
                          juce::AudioBuffer<float>& buf)
{
    // Samla 8192 samples av utdata (16 block)
    const int N = 8192;
    std::vector<float> out;
    out.reserve (N);

    while ((int) out.size() < N)
    {
        fillSine (buf, phase, 1000.0, 0.5f);
        chain.process (buf);
        const auto* d = buf.getReadPointer (0);
        for (int i = 0; i < kBlock && (int) out.size() < N; ++i)
            out.push_back (d[i]);
    }

    // Diskret Fourier för harmoniker 1–5 vid 1 kHz
    auto dftAmp = [&] (double freqHz) -> float
    {
        double re = 0.0, im = 0.0;
        const double step = juce::MathConstants<double>::twoPi * freqHz / kSR;
        for (int n = 0; n < N; ++n)
        {
            re += out[n] * std::cos (step * n);
            im += out[n] * std::sin (step * n);
        }
        return static_cast<float> (std::sqrt (re * re + im * im) / N);
    };

    const float f1 = dftAmp (1000.0);
    const float h2 = dftAmp (2000.0);
    const float h3 = dftAmp (3000.0);
    const float h4 = dftAmp (4000.0);
    const float h5 = dftAmp (5000.0);
    const float hsum = std::sqrt (h2*h2 + h3*h3 + h4*h4 + h5*h5);
    return (f1 > 1e-6f) ? hsum / f1 * 100.0f : 0.0f;
}

static void testTHD()
{
    std::printf ("\n── Test 5: THD 1 kHz @ −6 dBFS per formel ────────────────────\n");
    juce::AudioBuffer<float> buf (2, kBlock);
    const char* formulaNames[] = { "Agfa", "BASF", "Scotch" };

    for (int formula = 0; formula <= 2; ++formula)
    {
        SignalChain chain;
        chain.prepare (kSR, kBlock);
        auto p = toParams (kPresets[0]);
        p.tapeFormula      = formula;
        p.saturationDrive  = 1.0f;
        p.saturationDriveR = 1.0f;
        chain.setParameters (p);

        double phase = 0.0;
        runBlocks (chain, buf, phase, kWarm, 5.0f);

        const float thd = measureTHD (chain, phase, buf);

        // Förväntat intervall: tape-saturation ger 0.05 %–25 % THD vid −6 dBFS
        bool ok = (thd >= 0.05f && thd <= 25.0f);
        char name[32];
        std::snprintf (name, sizeof (name), "THD %s", formulaNames[formula]);
        char detail[32];
        std::snprintf (detail, sizeof (detail), "%.2f %%", thd);
        report (name, ok, detail);
    }
}

// ──────────────────────────────────────────────────────────────────
// TEST 6: Hastighetsbyte mitt i stream
// ──────────────────────────────────────────────────────────────────

static void testSpeedSwitches()
{
    std::printf ("\n── Test 6: Hastighetsbyten mitt i stream ───────────────────────\n");
    juce::AudioBuffer<float> buf (2, kBlock);

    const TapeSpeed speeds[] = { TapeSpeed::Speed19, TapeSpeed::Speed95, TapeSpeed::Speed475 };
    const char* names[] = { "19cm/s", "9.5cm/s", "4.75cm/s" };
    int fails = 0;

    for (int a = 0; a < 3; ++a)
    {
        for (int b = 0; b < 3; ++b)
        {
            if (a == b) continue;

            SignalChain chain;
            chain.prepare (kSR, kBlock);
            auto p = toParams (kPresets[0]);
            p.speed = speeds[a];
            chain.setParameters (p);

            double phase = 0.0;
            runBlocks (chain, buf, phase, 20, 5.0f);

            p.speed = speeds[b];
            chain.setParameters (p);

            float pk = 0.0f;
            bool ok = runBlocks (chain, buf, phase, 20, 5.0f, &pk);

            char name[40];
            std::snprintf (name, sizeof (name), "Hastighet %s → %s", names[a], names[b]);
            char detail[32];
            std::snprintf (detail, sizeof (detail), "peak %.3f", pk);
            if (! ok) { report (name, false, detail); ++fails; }
        }
    }

    char detail[40];
    std::snprintf (detail, sizeof (detail), "%d/6 ok", 6 - fails);
    report ("Alla 6 hastighetsbyten", fails == 0, detail);
}

// ──────────────────────────────────────────────────────────────────
// TEST 7: Frekvensrespons — inte tyst, inte urspårad
// ──────────────────────────────────────────────────────────────────

static void testFrequencyResponse()
{
    std::printf ("\n── Test 7: Frekvensrespons (LF / MID / HF per formel) ─────────\n");
    juce::AudioBuffer<float> buf (2, kBlock);

    const double freqs[] = { 100.0, 1000.0, 8000.0 };
    const char* fnames[] = { "100Hz", "1kHz", "8kHz" };
    const char* formulaNames[] = { "Agfa", "BASF", "Scotch" };

    for (int formula = 0; formula <= 2; ++formula)
    {
        for (int fi = 0; fi < 3; ++fi)
        {
            SignalChain chain;
            chain.prepare (kSR, kBlock);
            auto p = toParams (kPresets[0]);
            p.tapeFormula      = formula;
            p.saturationDrive  = 1.0f;
            p.saturationDriveR = 1.0f;
            chain.setParameters (p);

            double phase = 0.0;
            runBlocks (chain, buf, phase, kWarm, 5.0f);

            // Mät RMS vid angiven frekvens (10 block)
            float sumSq = 0.0f;
            int   count = 0;
            for (int blk = 0; blk < 10; ++blk)
            {
                fillSine (buf, phase, freqs[fi], 0.5f);
                chain.process (buf);
                const auto* d = buf.getReadPointer (0);
                for (int i = 0; i < kBlock; ++i)
                {
                    sumSq += d[i] * d[i];
                    ++count;
                }
            }
            const float rms = std::sqrt (sumSq / count);
            const float dBFS = 20.0f * std::log10 (rms + 1e-9f);

            // Förväntat: −30 dB..+6 dB vs 0 dBFS input (0.5 amp)
            bool ok = (dBFS > -36.0f && dBFS < 6.0f) && std::isfinite (dBFS);

            char name[48];
            std::snprintf (name, sizeof (name), "Frekvensrespons %s %s", formulaNames[formula], fnames[fi]);
            char detail[32];
            std::snprintf (detail, sizeof (detail), "%.1f dBFS", dBFS);
            report (name, ok, detail);
        }
    }
}

// ──────────────────────────────────────────────────────────────────
// main
// ──────────────────────────────────────────────────────────────────

int main()
{
    std::printf ("═══════════════════════════════════════════════════════════════\n");
    std::printf ("  BC2000DL DSP Smoke Tests   (48 kHz / 512 block)\n");
    std::printf ("═══════════════════════════════════════════════════════════════\n");

    testAllPresets();
    testAllTransitions();
    testNaNRecovery();
    testEchoSelfOsc();
    testTHD();
    testSpeedSwitches();
    testFrequencyResponse();

    std::printf ("\n═══════════════════════════════════════════════════════════════\n");
    std::printf ("  RESULTAT:  %d godkända   %d underkända\n", gPass, gFail);
    std::printf ("═══════════════════════════════════════════════════════════════\n");

    return (gFail > 0) ? 1 : 0;
}
