/*  GoogleTest-baserade DSP-enhetstester för Beolux 2000.
    Bygg/kör:  cmake --build build --target BC2000DL_GTest
               ./build/BC2000DL_GTest_artefacts/Release/BC2000DL_GTest
*/
#include <gtest/gtest.h>

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

    juce::AudioBuffer<float> render (const SignalChain::Parameters& p,
                                     const juce::AudioBuffer<float>& in, int warm = 20)
    {
        SignalChain c; c.prepare (kSR, kBlk); c.setParameters (p);
        juce::AudioBuffer<float> blk (2, kBlk);
        for (int w = 0; w < warm; ++w) { blk.clear(); c.process (blk); }
        const int N = in.getNumSamples();
        juce::AudioBuffer<float> out (2, N);
        for (int pos = 0; pos < N; )
        {
            const int n = std::min (kBlk, N - pos);
            juce::AudioBuffer<float> b (2, n);
            for (int ch = 0; ch < 2; ++ch) b.copyFrom (ch, 0, in, ch, pos, n);
            c.process (b);
            for (int ch = 0; ch < 2; ++ch) out.copyFrom (ch, 0, b, ch, 0, n);
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
        p.monitorMode = 1;
        return p;
    }
}

TEST (Stability, FiniteOutput)
{
    EXPECT_TRUE (allFinite (render (micParams(), makeSine (1000.0f, 12000))));
}

TEST (Stability, NaNRecovery)
{
    auto in = makeSine (1000.0f, 12000);
    in.setSample (0, 5000, std::numeric_limits<float>::quiet_NaN());
    in.setSample (1, 5001, std::numeric_limits<float>::infinity());
    auto out = render (micParams(), in);
    juce::AudioBuffer<float> tail (2, 4000);
    for (int ch = 0; ch < 2; ++ch) tail.copyFrom (ch, 0, out, ch, 8000, 4000);
    EXPECT_TRUE (allFinite (tail));
}

TEST (Stability, HotInputBounded)
{
    auto out = render (micParams (1.0f), makeSine (1000.0f, 12000, 1.0f));
    EXPECT_LT (out.getMagnitude (0, 0, out.getNumSamples()), 2.0f);
}

TEST (Echo, SelfOscBounded)
{
    auto p = micParams();
    p.echoEnabled = true; p.echoAmount = 0.95f; p.echoAmountR = 0.95f;
    auto out = render (p, makeSine (1000.0f, 48000), 5);
    EXPECT_LT (out.getMagnitude (0, 0, out.getNumSamples()), 2.0f);
}

TEST (Mixer, MasterLRIndependent)
{
    auto p = micParams();
    p.masterVolume = 0.85f; p.masterVolumeR = 0.2f;
    auto out = render (p, makeSine (1000.0f, 12000));
    EXPECT_LT (out.getRMSLevel (1, 2000, 8000), out.getRMSLevel (0, 2000, 8000) * 0.6f);
}

TEST (Sources, RadioPhonoMicDiffer)
{
    auto sig = makeSine (4000.0f, 12000);
    auto base = micParams (0.0f);
    SignalChain::Parameters mic = base;   mic.micGain = 0.6f;   mic.micGainR = 0.6f;
    SignalChain::Parameters pho = base;   pho.phonoGain = 0.6f; pho.phonoGainR = 0.6f;
    SignalChain::Parameters rad = base;   rad.radioGain = 0.6f; rad.radioGainR = 0.6f;
    auto oMic = render (mic, sig), oPho = render (pho, sig), oRad = render (rad, sig);
    EXPECT_GT (meanAbsDiff (oMic, oPho), 1.0e-4f);
    EXPECT_GT (meanAbsDiff (oMic, oRad), 1.0e-4f);
    EXPECT_GT (meanAbsDiff (oPho, oRad), 1.0e-4f);
}

TEST (Routing, BypassTapeChangesSound)
{
    auto sig = makeSine (1000.0f, 12000);
    auto normal = render (micParams(), sig);
    auto bp = micParams(); bp.bypassTape = true;
    EXPECT_GT (meanAbsDiff (normal, render (bp, sig)), 1.0e-4f);
}

TEST (Routing, SoundOnSoundFeedsLeftIntoRight)
{
    juce::AudioBuffer<float> sig (2, 12000); sig.clear();
    for (int i = 0; i < 12000; ++i)
        sig.setSample (0, i, 0.2f * std::sin (2.0 * juce::MathConstants<double>::pi * 1000.0 * i / kSR));
    auto p = micParams(); p.soundOnSound = true;
    auto out = render (p, sig, 40);
    EXPECT_GT (out.getRMSLevel (1, 4000, 6000), 1.0e-4f);
}
