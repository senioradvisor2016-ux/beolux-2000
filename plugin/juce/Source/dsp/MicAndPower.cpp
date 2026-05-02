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
        hpFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass (sr, 5.0f);
        hpFilter.reset();
    }

    void PowerAmp8004014::reset()
    {
        hpFilter.reset();
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
        // 2. AUTOMATSIKRING soft-clip — höjd tröskel rejält så power-amp bara
        //    introducerar distorsion vid genuin overdrive. Vid -3 dBFS in i
        //    nominell drift ska power-amp vara nästan transparent.
        constexpr float kSoftClipKnee = 2.5f;
        y = kSoftClipKnee * std::tanh (y / kSoftClipKnee);
        // 3. Cap-coupling HP @ 5 Hz
        y = hpFilter.processSample (y);
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
