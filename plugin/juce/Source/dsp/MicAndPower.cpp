/*  MicTransformer + PowerAmp implementation. */

#include "MicAndPower.h"
#include <cmath>

namespace bc2000dl::dsp
{
    // ---------- MicTransformer8012003 ----------
    void MicTransformer8012003::prepare (double sr)
    {
        sampleRate = sr;
        hpFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass (sr, 25.0f);
        const double hfCorner = std::min (30000.0, sr * 0.45);
        lpFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass (sr, static_cast<float> (hfCorner));
        hpFilter.reset();
        lpFilter.reset();
    }

    void MicTransformer8012003::reset()
    {
        hpFilter.reset();
        lpFilter.reset();
    }

    float MicTransformer8012003::processSample (float x)
    {
        // 1. Step-up
        float y = x * turnsRatio;
        // 2. LF-roll-off
        y = hpFilter.processSample (y);
        // 3. Mjuk kärn-saturation
        const float scale = kSatThreshold * kSatSoftness;
        y = scale * std::tanh (y / scale);
        // 4. HF-roll-off
        y = lpFilter.processSample (y);
        return y;
    }

    void MicTransformer8012003::process (juce::AudioBuffer<float>& buffer, int channel)
    {
        auto* data = buffer.getWritePointer (channel);
        const int n = buffer.getNumSamples();
        for (int i = 0; i < n; ++i)
            data[i] = processSample (data[i]);
    }

    // ---------- PowerAmp8004014 ----------
    void PowerAmp8004014::prepare (double sr)
    {
        sampleRate = sr;
        // Intern-speaker-emulering: bandpass ≈ 90 Hz – 6 kHz med cabinet-peak vid 1 kHz.
        // Hörbar färg som "monitor genom liten högtalare" — inte placebo.
        hpFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass (sr, 90.0f);
        lpFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass  (sr, 6000.0f);
        peakFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            sr, 1000.0f, 1.2f, juce::Decibels::decibelsToGain (3.0f));
        hpFilter.reset();
        lpFilter.reset();
        peakFilter.reset();
    }

    void PowerAmp8004014::reset()
    {
        hpFilter.reset();
        lpFilter.reset();
        peakFilter.reset();
    }

    float PowerAmp8004014::crossoverDistortion (float x) const
    {
        // Tidigare hard-knee `mag < thr ? mag*0.9 : mag` skapade discontinuitet
        // i derivatan vid mag = thr → varje noll-genomgång genererade massiv
        // harmonik (38 % THD vid -3 dBFS). Smooth knee via exp-täppning bevarar
        // crossover-karaktären (10 % attenuering nära noll) men är derivativ-
        // kontinuerlig → aliasar inte längre upp till 30+ % THD.
        const float thr  = kCrossoverThreshold;
        const float sign = (x > 0.0f) - (x < 0.0f);
        const float mag  = std::abs (x);
        // Vid mag → 0 :  attenuering ~10 % (slätt minskar mot 0)
        // Vid mag >> thr: attenuering ~0 % (full transparens)
        const float kSoftness = 0.10f;
        const float dent = kSoftness * std::exp (-mag / thr);
        return sign * mag * (1.0f - dent);
    }

    float PowerAmp8004014::processSample (float x)
    {
        if (! enabled) return x;
        // 1. Smooth crossover (germanium AC127/132 class-AB-mismatch)
        float y = crossoverDistortion (x);
        // 2. Cabinet-peak +3 dB @ 1 kHz (intern-speaker "honk")
        y = peakFilter.processSample (y);
        // 3. AUTOMATSIKRING soft-clip — knee 1.0 → mjuk kompression vid 0 dBFS.
        y = kAutomatsikring * std::tanh (y / kAutomatsikring);
        // 4. Speaker HP @ 90 Hz (cabinet-roll-off, ingen sub-bas)
        y = hpFilter.processSample (y);
        // 5. Speaker LP @ 6 kHz (cone HF-cutoff)
        y = lpFilter.processSample (y);
        return y;
    }

    void PowerAmp8004014::process (juce::AudioBuffer<float>& buffer, int channel)
    {
        if (! enabled) return;
        auto* data = buffer.getWritePointer (channel);
        const int n = buffer.getNumSamples();
        for (int i = 0; i < n; ++i)
            data[i] = processSample (data[i]);
    }
}
