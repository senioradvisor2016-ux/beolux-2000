/*  Catch2-baserade DSP-enhetstester för Germanium 2000 Deluxe.
    Bygg/kör:  cmake --build build --target BC2000DL_Catch2
               ./build/BC2000DL_Catch2_artefacts/Release/BC2000DL_Catch2
*/
#include <catch2/catch_test_macros.hpp>

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../dsp/SignalChain.h"

#include <cmath>

using namespace bc2000dl::dsp;

namespace
{
    constexpr double kSR = 48000.0;
    constexpr int    kBlk = 512;

    juce::AudioBuffer<float> makeSine (float freq, int n, float amp = 0.2f)
    {
        juce::AudioBuffer<float> b (2, n);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < n; ++i)
                b.setSample (ch, i, amp * std::sin (2.0 * juce::MathConstants<double>::pi * freq * i / kSR));
        return b;
    }

    // Kör en hel buffer genom kedjan i 512-block, med uppvärmning.
    juce::AudioBuffer<float> render (const SignalChain::Parameters& p,
                                     const juce::AudioBuffer<float>& in, int /*unused*/ = 0)
    {
        SignalChain c; c.prepare (kSR, kBlk); c.setParameters (p);
        const int N = in.getNumSamples();
        juce::AudioBuffer<float> out (2, N);
        // Två pass av SAMMA kontinuerliga signal: första värmer upp kedjan
        // (silence-gate öppnar, J-A-state settlar), andra mäts. Kontinuerlig
        // fas → inga block-transienter som annars NaN:ar/gatar.
        for (int pass = 0; pass < 2; ++pass)
            for (int pos = 0; pos < N; )
            {
                const int n = std::min (kBlk, N - pos);
                juce::AudioBuffer<float> b (2, n);
                for (int ch = 0; ch < 2; ++ch) b.copyFrom (ch, 0, in, ch, pos, n);
                c.process (b);
                for (int ch = 0; ch < 2; ++ch) out.copyFrom (ch, pos, b, ch, 0, n);
                pos += n;
            }
        return out;
    }

    bool allFinite (const juce::AudioBuffer<float>& b)
    {
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            for (int i = 0; i < b.getNumSamples(); ++i)
                if (! std::isfinite (b.getSample (ch, i))) return false;
        return true;
    }

    float meanAbsDiff (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b, int ch = 0)
    {
        const int n = std::min (a.getNumSamples(), b.getNumSamples());
        double s = 0.0;
        for (int i = 0; i < n; ++i) s += std::abs (a.getSample (ch, i) - b.getSample (ch, i));
        return (float) (s / std::max (1, n));
    }

    SignalChain::Parameters micParams (float g = 0.5f)
    {
        SignalChain::Parameters p;
        p.micGain = g; p.micGainR = g; p.masterVolume = 0.85f; p.masterVolumeR = 0.85f;
        p.monitorMode = 1;  // Tape
        return p;
    }

    SignalChain::Parameters randomParams (juce::Random& r)
    {
        SignalChain::Parameters p;
        p.speed = (TapeSpeed) r.nextInt (3);
        p.micGain = r.nextFloat();   p.micGainR = r.nextFloat();
        p.phonoGain = r.nextFloat(); p.phonoGainR = r.nextFloat();
        p.radioGain = r.nextFloat(); p.radioGainR = r.nextFloat();
        p.bassDb = r.nextFloat()*24-12; p.trebleDb = r.nextFloat()*24-12;
        p.balance = r.nextFloat()*2-1;
        p.masterVolume = r.nextFloat(); p.masterVolumeR = r.nextFloat();
        p.biasAmount = 0.5f+r.nextFloat(); p.biasAmountR = 0.5f+r.nextFloat();
        p.saturationDrive = 0.5f+r.nextFloat()*1.5f; p.saturationDriveR = 0.5f+r.nextFloat()*1.5f;
        p.wowFlutterAmount = r.nextFloat()*2;
        p.echoEnabled = r.nextBool(); p.echoAmount = r.nextFloat(); p.echoAmountR = r.nextFloat();
        p.echoTimeMs = 30+r.nextFloat()*320; p.echoFeedback = r.nextFloat();
        p.bypassTape = r.nextBool(); p.speakerMonitor = r.nextBool();
        p.synchroplay = r.nextBool(); p.multiplayGen = 1+r.nextInt (5);
        p.soundOnSound = r.nextBool(); p.publicAddress = r.nextBool();
        p.tapeFormula = r.nextInt (3); p.biasType = r.nextInt (2); p.trackWidth = r.nextInt (2);
        p.printThrough = r.nextFloat()*0.05f; p.stereoAsymmetry = r.nextFloat()*0.05f;
        p.monitorMode = r.nextInt (2); p.phonoMode = r.nextInt (2); p.radioMode = r.nextInt (2);
        p.micMode = r.nextInt (3); p.mainsHum = r.nextFloat()*0.1f;
        p.inputTrimDb = r.nextFloat()*24-12; p.outputTrimDb = r.nextFloat()*24-12;  // ±12 (sane)
        p.monitorTrack1 = r.nextBool(); p.monitorTrack2 = r.nextBool();
        return p;
    }

    juce::AudioBuffer<float> randomAudio (juce::Random& r, int n)
    {
        juce::AudioBuffer<float> b (2, n);
        const int kind = r.nextInt (5);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < n; ++i)
            {
                float v = 0.0f;
                const double ph = 2.0 * juce::MathConstants<double>::pi * (200 + ch*300) * i / kSR;
                switch (kind)
                {
                    case 0: v = 0.0f; break;                                // tystnad
                    case 1: v = (r.nextFloat()*2-1) * 0.9f; break;          // hett brus
                    case 2: v = 0.5f; break;                                // DC
                    case 3: v = 1.5f * (float) std::sin (ph); break;        // över 0 dBFS
                    default: v = 0.1f * (float) std::sin (ph); break;       // mild ton
                }
                b.setSample (ch, i, v);
            }
        return b;
    }
}

TEST_CASE ("Fuzz: slumpade params + edge-audio → finit & bounded", "[fuzz]")
{
    juce::Random r (1234);
    for (int it = 0; it < 300; ++it)
    {
        auto out = render (randomParams (r), randomAudio (r, 4096));
        REQUIRE (allFinite (out));
        REQUIRE (out.getMagnitude (0, 0, out.getNumSamples()) < 8.0f);
    }
}

TEST_CASE ("Output- och input-meter speglar signalnivån", "[meters]")
{
    SignalChain c; c.prepare (kSR, kBlk); c.setParameters (micParams());
    for (int k = 0; k < 40; ++k) { auto b = makeSine (1000.0f, kBlk, 0.6f); c.process (b); }
    REQUIRE (std::isfinite (c.meterLevelL_dBFS.load()));
    REQUIRE (c.inputLevelL_dBFS.load()  > -40.0f);   // input-VU känner signalen
    REQUIRE (c.meterLevelL_dBFS.load()  > -40.0f);   // output-VU känner signalen
}

TEST_CASE ("Output is finite for a normal signal", "[stability]")
{
    auto out = render (micParams(), makeSine (1000.0f, 12000));
    REQUIRE (allFinite (out));
}

TEST_CASE ("NaN input recovers to finite output", "[stability]")
{
    auto in = makeSine (1000.0f, 12000);
    in.setSample (0, 5000, std::numeric_limits<float>::quiet_NaN());
    in.setSample (1, 5001, std::numeric_limits<float>::infinity());
    auto out = render (micParams(), in);
    // tail (efter NaN) ska vara finit
    juce::AudioBuffer<float> tail (2, 4000);
    for (int ch = 0; ch < 2; ++ch) tail.copyFrom (ch, 0, out, ch, 8000, 4000);
    REQUIRE (allFinite (tail));
}

TEST_CASE ("Hot input stays bounded (no blow-up)", "[stability]")
{
    auto out = render (micParams (1.0f), makeSine (1000.0f, 12000, 1.0f));
    REQUIRE (out.getMagnitude (0, 0, out.getNumSamples()) < 2.0f);
}

TEST_CASE ("Echo self-oscillation stays bounded at amount 0.95", "[echo]")
{
    auto p = micParams();
    p.echoEnabled = true; p.echoAmount = 0.95f; p.echoAmountR = 0.95f;
    auto out = render (p, makeSine (1000.0f, 48000), 5);
    REQUIRE (out.getMagnitude (0, 0, out.getNumSamples()) < 2.0f);
}

TEST_CASE ("Master L and R are independent", "[mixer]")
{
    auto p = micParams();
    p.masterVolume = 0.85f; p.masterVolumeR = 0.2f;
    auto out = render (p, makeSine (1000.0f, 12000, 0.6f));
    const float rmsL = out.getRMSLevel (0, 2000, 8000);
    const float rmsR = out.getRMSLevel (1, 2000, 8000);
    REQUIRE (rmsL > 1.0e-4f);        // utgång finns
    REQUIRE (rmsR < rmsL * 0.6f);    // höger tydligt lägre (oberoende)
}

TEST_CASE ("Radio, Phono and Mic produce DIFFERENT output (bug 1)", "[sources]")
{
    auto sig = makeSine (4000.0f, 12000);  // HF avslöjar tonsignaturerna
    auto base = micParams (0.0f);
    SignalChain::Parameters mic = base;   mic.micGain = 0.6f;   mic.micGainR = 0.6f;
    SignalChain::Parameters pho = base;   pho.phonoGain = 0.6f; pho.phonoGainR = 0.6f;
    SignalChain::Parameters rad = base;   rad.radioGain = 0.6f; rad.radioGainR = 0.6f;
    auto oMic = render (mic, sig), oPho = render (pho, sig), oRad = render (rad, sig);
    REQUIRE (meanAbsDiff (oMic, oPho) > 1.0e-4f);
    REQUIRE (meanAbsDiff (oMic, oRad) > 1.0e-4f);
    REQUIRE (meanAbsDiff (oPho, oRad) > 1.0e-4f);
}

TEST_CASE ("Bypass-tape changes the sound", "[routing]")
{
    auto sig = makeSine (1000.0f, 12000);
    auto normal = render (micParams(), sig);
    auto bp = micParams(); bp.bypassTape = true;
    auto bypassed = render (bp, sig);
    REQUIRE (meanAbsDiff (normal, bypassed) > 1.0e-4f);
}

TEST_CASE ("Sound-on-Sound feeds Left into Right", "[routing]")
{
    // L-only input: vänster ton, höger tyst
    juce::AudioBuffer<float> sig (2, 12000);
    sig.clear();
    for (int i = 0; i < 12000; ++i)
        sig.setSample (0, i, 0.6f * std::sin (2.0 * juce::MathConstants<double>::pi * 1000.0 * i / kSR));
    auto p = micParams();
    p.soundOnSound = true;
    auto out = render (p, sig);
    REQUIRE (out.getRMSLevel (1, 6000, 5000) > 1.0e-4f);  // höger får innehåll via SoS
}
