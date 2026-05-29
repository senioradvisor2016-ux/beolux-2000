/*  BugFixTests — targeted regression tests för buggar rapporterade av
    Christoffer Berg (macOS 14 / Logic):
      - Phono producerar endast missljud
      - Bias-knappen har ingen audibel effekt
      - Multiplay producerar missljud / kornighet

    Bygg och kör:
      cmake --build build --config Release --target BC2000DL_BugFixTests
      ./build/BC2000DL_BugFixTests_artefacts/Release/BC2000DL_BugFixTests
*/

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>

#include "../dsp/SignalChain.h"
#include "../dsp/Multiplay.h"
#include "../dsp/TapeSaturation.h"

#include <cmath>
#include <cstdio>
#include <vector>

using namespace bc2000dl;
using namespace bc2000dl::dsp;

static int gPass = 0;
static int gFail = 0;

static void report (const char* name, bool ok, const char* detail = "")
{
    if (ok) ++gPass; else ++gFail;
    std::printf ("%s  %-58s  %s\n", ok ? "[PASS]" : "[FAIL]", name, detail);
}

static constexpr double kSR    = 48000.0;
static constexpr int    kMaxBlock = 512;

static void fillSineMono (float* dst, int n, double& phase, double freq, float amp)
{
    const double inc = juce::MathConstants<double>::twoPi * freq / kSR;
    for (int i = 0; i < n; ++i)
    {
        dst[i] = static_cast<float> (std::sin (phase) * amp);
        phase += inc;
        if (phase >= juce::MathConstants<double>::twoPi)
            phase -= juce::MathConstants<double>::twoPi;
    }
}

// =======================================================================
//  BUG 1: Phono scratch-buffer-size mismatch
//  Regression: phonoScratch allokeras till kMaxBlock men host kan skicka
//  mindre block.  Före fix processade GE/RIAA hela kMaxBlock → samples
//  [n..kMaxBlock) är stale data → RIAA-IIR-state desyncar → missljud.
//  Verifierar: phono-output ska vara identiskt vid (a) full block och
//  (b) tre olika små blockstorlekar i följd, för samma input.
// =======================================================================
static void testPhonoVariableBlockSize()
{
    std::printf ("\n── Bug 1: Phono med variabel block-storlek ─────────────────\n");

    auto runCase = [] (int blockSize, std::vector<float>& outL) -> bool
    {
        SignalChain chain;
        chain.prepare (kSR, kMaxBlock);    // host annonserar max 512

        SignalChain::Parameters p;
        p.micGain = 0.0f;  p.micGainR = 0.0f;
        p.phonoGain = 1.0f;  p.phonoGainR = 1.0f;   // bara phono aktiv
        p.radioGain = 0.0f;  p.radioGainR = 0.0f;
        p.masterVolume = 0.85f;
        p.monitorMode = 1;
        p.monitorTrack1 = true;  p.monitorTrack2 = true;
        p.bypassTape = false;
        p.phonoMode = 1;            // H magnetisk + RIAA
        p.wowFlutterAmount = 0.0f;  // ta bort wow så vi mäter deterministiskt
        p.multiplayGen = 1;         // off
        chain.setParameters (p);

        juce::AudioBuffer<float> buf (2, blockSize);
        double phase = 0.0;

        // Värm upp 20 block med samma blockSize
        for (int b = 0; b < 20; ++b)
        {
            fillSineMono (buf.getWritePointer (0), blockSize, phase, 1000.0, 0.3f);
            buf.copyFrom (1, 0, buf, 0, 0, blockSize);
            chain.process (buf);
            for (int i = 0; i < blockSize; ++i)
                if (! std::isfinite (buf.getReadPointer (0)[i])) return false;
        }

        // Samla 5 block till outL för RMS-mätning
        outL.clear();
        for (int b = 0; b < 5; ++b)
        {
            fillSineMono (buf.getWritePointer (0), blockSize, phase, 1000.0, 0.3f);
            buf.copyFrom (1, 0, buf, 0, 0, blockSize);
            chain.process (buf);
            for (int i = 0; i < blockSize; ++i)
            {
                if (! std::isfinite (buf.getReadPointer (0)[i])) return false;
                outL.push_back (buf.getReadPointer (0)[i]);
            }
        }
        return true;
    };

    auto rmsdBFS = [] (const std::vector<float>& v) -> float
    {
        double s = 0.0;
        for (float x : v) s += static_cast<double> (x) * static_cast<double> (x);
        const double rms = std::sqrt (s / std::max ((size_t) 1, v.size()));
        return 20.0f * std::log10 (static_cast<float> (rms) + 1e-9f);
    };

    std::vector<float> outFull, outHalf, outQtr, outOdd;
    bool okFull = runCase (kMaxBlock, outFull);
    bool okHalf = runCase (kMaxBlock / 2, outHalf);
    bool okQtr  = runCase (kMaxBlock / 4, outQtr);
    bool okOdd  = runCase (123, outOdd);

    const float rmsFull = rmsdBFS (outFull);
    const float rmsHalf = rmsdBFS (outHalf);
    const float rmsQtr  = rmsdBFS (outQtr);
    const float rmsOdd  = rmsdBFS (outOdd);

    // RMS-värdena ska ligga inom ±1 dB av varandra — annars indikerar det
    // att scratch-buffer-storleken påverkar signalen (gamla buggen).
    const float refRms = rmsFull;
    bool stable = okFull && okHalf && okQtr && okOdd
               && std::abs (rmsHalf - refRms) < 1.0f
               && std::abs (rmsQtr  - refRms) < 1.0f
               && std::abs (rmsOdd  - refRms) < 1.0f;

    char detail[128];
    std::snprintf (detail, sizeof (detail),
        "RMS @512=%.1f /256=%.1f /128=%.1f /123=%.1f dBFS",
        rmsFull, rmsHalf, rmsQtr, rmsOdd);
    report ("Phono stabilt över olika block-storlekar", stable, detail);
}

// =======================================================================
//  BUG 2: Bias-amount har ingen audibel effekt
//  Verifierar att signal-output ÄNDRAS audibelt när biasAmount sweepas
//  från 0.5 (under-biased = mer mättnad) till 1.5 (över-biased = mjukare).
//  Mätning: THD vid biasAmount=0.5 ska vara HÖGRE än vid biasAmount=1.5.
// =======================================================================
static float measureThdPercent (SignalChain& chain, int blockSize)
{
    const int N = 8192;
    std::vector<float> y;  y.reserve (N);
    juce::AudioBuffer<float> buf (2, blockSize);
    double phase = 0.0;
    // Värm upp 20 block
    for (int b = 0; b < 20; ++b)
    {
        fillSineMono (buf.getWritePointer (0), blockSize, phase, 1000.0, 0.5f);
        buf.copyFrom (1, 0, buf, 0, 0, blockSize);
        chain.process (buf);
    }
    while ((int) y.size() < N)
    {
        fillSineMono (buf.getWritePointer (0), blockSize, phase, 1000.0, 0.5f);
        buf.copyFrom (1, 0, buf, 0, 0, blockSize);
        chain.process (buf);
        for (int i = 0; i < blockSize && (int) y.size() < N; ++i)
            y.push_back (buf.getReadPointer (0)[i]);
    }
    auto dftAmp = [&] (double f) -> float
    {
        double re = 0, im = 0;
        const double step = juce::MathConstants<double>::twoPi * f / kSR;
        for (int n = 0; n < N; ++n)
        { re += y[n] * std::cos (step * n);  im += y[n] * std::sin (step * n); }
        return static_cast<float> (std::sqrt (re*re + im*im) / N);
    };
    const float f1 = dftAmp (1000.0);
    const float h2 = dftAmp (2000.0), h3 = dftAmp (3000.0);
    const float h4 = dftAmp (4000.0), h5 = dftAmp (5000.0);
    const float hs = std::sqrt (h2*h2 + h3*h3 + h4*h4 + h5*h5);
    return (f1 > 1e-6f) ? hs / f1 * 100.0f : 0.0f;
}

static void testBiasAudibility()
{
    std::printf ("\n── Bug 2: Bias-amount audibel effekt ────────────────────────\n");

    auto runWithBias = [] (float bias) -> float
    {
        SignalChain chain;
        chain.prepare (kSR, kMaxBlock);
        SignalChain::Parameters p;
        p.micGain = 0.7f;  p.micGainR = 0.7f;
        p.phonoGain = 0.0f;  p.phonoGainR = 0.0f;
        p.radioGain = 0.0f;  p.radioGainR = 0.0f;
        p.masterVolume = 0.85f;
        p.monitorMode = 1;
        p.monitorTrack1 = true;  p.monitorTrack2 = true;
        p.bypassTape = false;
        p.wowFlutterAmount = 0.0f;
        p.multiplayGen = 1;
        p.saturationDrive = 1.5f;   // pusha drive så bias-skillnad blir tydlig
        p.saturationDriveR = 1.5f;
        p.biasAmount = bias;
        chain.setParameters (p);
        return measureThdPercent (chain, kMaxBlock);
    };

    const float thdUnder   = runWithBias (0.5f);   // under-biased
    const float thdNominal = runWithBias (1.0f);   // nominal
    const float thdOver    = runWithBias (1.5f);   // over-biased

    // Under-bias ska ge MARKANT mer THD än over-bias.  Skillnad >0.5 % h-summa
    // är hörbart i mätförhållanden, >1 % är tydligt audibelt.
    const bool monotonic = thdUnder > thdNominal && thdNominal >= thdOver;
    const bool audibleDelta = (thdUnder - thdOver) > 0.5f;

    char detail[128];
    std::snprintf (detail, sizeof (detail),
        "THD @ bias 0.5=%.2f%% 1.0=%.2f%% 1.5=%.2f%% (Δ=%.2f)",
        thdUnder, thdNominal, thdOver, thdUnder - thdOver);
    report ("Bias har monotont avtagande THD (under→över)", monotonic, detail);
    report ("Bias-Δ tydligt audibel (>0.5 % THD-skillnad)", audibleDelta, detail);
}

// =======================================================================
//  BUG 3: Multiplay-filter resetar varje block → klick-transient
//  Verifierar att Multiplay::setGeneration med samma generation som tidigare
//  INTE klipper filter-state.  Före fix: setGeneration anropades varje block
//  från setParameters, vilket gjorde hfFilter.reset() varje block → IIR-state
//  alltid 0 vid blockstart → klick-transient @ blockgräns → "kornighet".
//  Vi mäter: efter värmning, kontinuerlig signal ska INTE ha större
//  block-gräns-diskontinuitet än inom-block-variation.
// =======================================================================
static void testMultiplayContinuity()
{
    std::printf ("\n── Bug 3: Multiplay-filter-state kontinuitet ────────────────\n");

    SignalChain chain;
    chain.prepare (kSR, kMaxBlock);
    SignalChain::Parameters p;
    p.micGain = 0.7f;  p.micGainR = 0.7f;
    p.phonoGain = 0.0f;
    p.radioGain = 0.0f;
    p.masterVolume = 0.85f;
    p.monitorMode = 1;
    p.monitorTrack1 = true;  p.monitorTrack2 = true;
    p.wowFlutterAmount = 0.0f;
    p.multiplayGen = 5;   // max generation = max HF-rolloff (4 kHz)
    chain.setParameters (p);

    const int blockSize = 256;
    juce::AudioBuffer<float> buf (2, blockSize);
    double phase = 0.0;

    // Värm upp
    for (int b = 0; b < 30; ++b)
    {
        fillSineMono (buf.getWritePointer (0), blockSize, phase, 1000.0, 0.3f);
        buf.copyFrom (1, 0, buf, 0, 0, blockSize);
        chain.process (buf);
    }

    // Mät max-diff mellan sista sample i block N och första sample i block N+1.
    // Vid kontinuerlig signal ska denna diff ligga i samma storleksordning
    // som typisk intra-block max-diff.
    float maxBoundaryDiff = 0.0f;
    float maxIntraDiff = 0.0f;
    float lastSampleOfPrev = 0.0f;
    bool firstBlock = true;
    for (int b = 0; b < 40; ++b)
    {
        fillSineMono (buf.getWritePointer (0), blockSize, phase, 1000.0, 0.3f);
        buf.copyFrom (1, 0, buf, 0, 0, blockSize);
        chain.process (buf);
        const auto* d = buf.getReadPointer (0);

        if (! firstBlock)
        {
            const float diff = std::abs (d[0] - lastSampleOfPrev);
            maxBoundaryDiff = std::max (maxBoundaryDiff, diff);
        }
        firstBlock = false;

        for (int i = 1; i < blockSize; ++i)
        {
            const float diff = std::abs (d[i] - d[i - 1]);
            maxIntraDiff = std::max (maxIntraDiff, diff);
        }
        lastSampleOfPrev = d[blockSize - 1];
    }

    // Block-gräns-diff ska inte vara mer än 3x typisk intra-block-diff.
    // Före fix var det 10-50x större pga reset varje block.
    const float ratio = (maxIntraDiff > 1e-9f) ? maxBoundaryDiff / maxIntraDiff : 0.0f;
    const bool continuous = ratio < 3.0f;

    char detail[128];
    std::snprintf (detail, sizeof (detail),
        "intra=%.5f boundary=%.5f ratio=%.2f",
        maxIntraDiff, maxBoundaryDiff, ratio);
    report ("Multiplay-filter kontinuerligt över blockgräns", continuous, detail);
}

int main()
{
    std::printf ("═══════════════════════════════════════════════════════════════\n");
    std::printf ("  BC2000DL Bug-fix regression tests   (Christoffer/Logic)\n");
    std::printf ("═══════════════════════════════════════════════════════════════\n");

    testPhonoVariableBlockSize();
    testBiasAudibility();
    testMultiplayContinuity();

    std::printf ("\n═══════════════════════════════════════════════════════════════\n");
    std::printf ("  RESULTAT:  %d godkända   %d underkända\n", gPass, gFail);
    std::printf ("═══════════════════════════════════════════════════════════════\n");
    return gFail > 0 ? 1 : 0;
}
