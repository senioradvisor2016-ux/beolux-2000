/*  Multiplay implementation. */

#include "Multiplay.h"
#include <cmath>

namespace bc2000dl::dsp
{
    void Multiplay::prepare (double sr, std::uint32_t seed)
    {
        sampleRate = sr;
        rng.seed (seed != 0 ? seed : std::random_device {} ());
        updateForGeneration();
    }

    void Multiplay::reset()
    {
        hfFilter.reset();
    }

    void Multiplay::setGeneration (int gen)
    {
        generation = juce::jlimit (1, 5, gen);
        updateForGeneration();
    }

    void Multiplay::updateForGeneration()
    {
        // HF-corner per generation
        // Gen 1 = 10 kHz, 2 = 8 kHz, 3 = 6 kHz, 4 = 5 kHz, 5 = 4 kHz
        const float corners[] = { 10000.0f, 8000.0f, 6000.0f, 5000.0f, 4000.0f };
        const int idx = juce::jlimit (0, 4, generation - 1);
        hfFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass (
            sampleRate, corners[idx]);
        hfFilter.reset();

        // Brus per generation: +1 dB/gen, bas -60 dBFS
        const float noiseDb = -60.0f + (static_cast<float> (generation) - 1.0f);
        noiseAmpLin = std::pow (10.0f, noiseDb / 20.0f);
    }

    float Multiplay::processSample (float x)
    {
        if (! enabled || generation <= 1) return x;
        // 1. HF-roll-off per generation
        float y = hfFilter.processSample (x);
        // 2. Adderat brus
        y += noiseDist (rng) * noiseAmpLin;
        return y;
    }

    void Multiplay::process (juce::AudioBuffer<float>& buffer, int channel)
    {
        if (! enabled || generation <= 1) return;
        auto* data = buffer.getWritePointer (channel);
        const int n = buffer.getNumSamples();
        for (int i = 0; i < n; ++i)
            data[i] = processSample (data[i]);
    }
}
