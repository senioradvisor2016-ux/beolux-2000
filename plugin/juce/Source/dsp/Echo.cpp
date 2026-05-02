/*  Echo implementation. */

#include "Echo.h"
#include <cmath>

namespace bc2000dl::dsp
{
    void Echo::prepare (double sr)
    {
        sampleRate = sr;
        const int maxDelay = static_cast<int> (sr * 0.35);  // 350 ms max
        buf.assign (static_cast<size_t> (maxDelay), 0.0f);
        writeIdx = 0;

        hfLossFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (
            sr, 8000.0f);
        hfLossFilter.reset();

        setSpeed (TapeSpeed::Speed19);
    }

    void Echo::reset()
    {
        std::fill (buf.begin(), buf.end(), 0.0f);
        writeIdx = 0;
        hfLossFilter.reset();
    }

    void Echo::setSpeed (TapeSpeed speed)
    {
        switch (speed)
        {
            case TapeSpeed::Speed19:  delayMs = static_cast<float> (kEchoTime_ms_Speed19);  break;
            case TapeSpeed::Speed95:  delayMs = static_cast<float> (kEchoTime_ms_Speed95);  break;
            case TapeSpeed::Speed475: delayMs = static_cast<float> (kEchoTime_ms_Speed475); break;
        }
        delaySamples = std::min (
            static_cast<int> (sampleRate * delayMs / 1000.0f),
            static_cast<int> (buf.size()) - 1);
    }

    float Echo::processSample (float x)
    {
        if (! enabled || amount < 1e-6f)
            return x;

        // Self-oscillation-mappning (manual §d-varning):
        //   amount  0.0…0.70  → fb 0.0…0.78   (avtagande echo)
        //   amount  0.70…0.85 → fb 0.78…0.96  (långa, men avtagande)
        //   amount  0.85…1.00 → fb 0.96…1.04  (sustained → divergent self-osc)
        // Tape-loop-recordens self-osc kommer från att fb går just över unity.
        // För DSP-stabilitet kläms den hårda klippen via tanh på återmatningen.
        const float fbCurve = amount * (0.92f + 0.12f * amount);  // mjuk ramp till ~1.04
        const float feedback = juce::jmin (fbCurve, 1.04f);

        // Read-pointer
        int readIdx = writeIdx - delaySamples;
        if (readIdx < 0) readIdx += static_cast<int> (buf.size());
        const float delayed = buf[static_cast<size_t> (readIdx)];

        // Output = input + feedback × delayed (våt + torr)
        const float y = x + delayed * feedback;

        // Återmatning genom record-amp-kedjan modelleras här som:
        //   1. HF-loss per pass (tape-formuleringens HF-roll-off vid playback)
        //   2. Soft-clip-tanh (ersätter recAmp+ac126-cascadens mjuka mättnad)
        //      → garanterar att self-osc ELASTISK växer mot stabil amplitud
        //        istället för att divergera till numerisk overflow.
        float fbSignal = hfLossFilter.processSample (y);
        // Soft-clip vid ±1.6 → vid sustained osc landar amplituden ~±1.5
        constexpr float kClipKnee = 1.6f;
        fbSignal = kClipKnee * std::tanh (fbSignal / kClipKnee);

        buf[static_cast<size_t> (writeIdx)] = fbSignal;
        writeIdx = (writeIdx + 1) % static_cast<int> (buf.size());

        return y;
    }

    void Echo::process (juce::AudioBuffer<float>& buffer, int channel)
    {
        if (! enabled || amount < 1e-6f) return;
        auto* data = buffer.getWritePointer (channel);
        const int n = buffer.getNumSamples();
        for (int i = 0; i < n; ++i)
            data[i] = processSample (data[i]);
    }
}
