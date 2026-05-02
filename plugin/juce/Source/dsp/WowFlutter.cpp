/*  WowFlutter implementation. */

#include "WowFlutter.h"
#include <cmath>

namespace bc2000dl::dsp
{
    void WowFlutter::prepare (double sr)
    {
        sampleRate = sr;
        buf.assign (static_cast<size_t> (sr * 0.05), 0.0f);
        writeIdx = 0;
        wowPhase = flutterPhase = 0.0f;
    }

    void WowFlutter::reset()
    {
        std::fill (buf.begin(), buf.end(), 0.0f);
        writeIdx = 0;
        wowPhase = flutterPhase = 0.0f;
    }

    void WowFlutter::setSpeed (TapeSpeed speed)
    {
        switch (speed)
        {
            case TapeSpeed::Speed19:
                wowAmount = 0.0008f; flutterAmount = 0.0006f; break;
            case TapeSpeed::Speed95:
                wowAmount = 0.0013f; flutterAmount = 0.0010f; break;
            case TapeSpeed::Speed475:
                wowAmount = 0.0020f; flutterAmount = 0.0016f; break;
        }
    }

    float WowFlutter::processSample (float x)
    {
        if (amount < 1e-6f || buf.empty()) return x;

        // Skriv input till ring-buffer
        buf[static_cast<size_t> (writeIdx)] = x;
        writeIdx = (writeIdx + 1) % static_cast<int> (buf.size());

        // Beräkna instant delay-modulation
        const float wowMod     = wowAmount * std::sin (wowPhase) * amount;
        const float flutterMod = flutterAmount * std::sin (flutterPhase) * amount;

        const float baseDelay  = static_cast<float> (buf.size()) * 0.5f;
        const float delaySamps = baseDelay
            + (wowMod + flutterMod) * static_cast<float> (sampleRate) * 0.005f;

        // Lagrange 3:e-ordningens interpolation
        const int idxInt = static_cast<int> (std::floor (delaySamps));
        const float frac = delaySamps - static_cast<float> (idxInt);

        const int sz = static_cast<int> (buf.size());
        const int i0 = ((writeIdx - idxInt - 1) % sz + sz) % sz;
        const int i1 = ((writeIdx - idxInt    ) % sz + sz) % sz;
        const int i2 = ((writeIdx - idxInt + 1) % sz + sz) % sz;
        const int i3 = ((writeIdx - idxInt + 2) % sz + sz) % sz;
        const float x0 = buf[static_cast<size_t> (i0)];
        const float x1 = buf[static_cast<size_t> (i1)];
        const float x2 = buf[static_cast<size_t> (i2)];
        const float x3 = buf[static_cast<size_t> (i3)];

        const float c0 = -frac * (frac - 1.0f) * (frac - 2.0f) / 6.0f;
        const float c1 = (frac + 1.0f) * (frac - 1.0f) * (frac - 2.0f) / 2.0f;
        const float c2 = -(frac + 1.0f) * frac * (frac - 2.0f) / 2.0f;
        const float c3 = (frac + 1.0f) * frac * (frac - 1.0f) / 6.0f;

        const float y = c0 * x0 + c1 * x1 + c2 * x2 + c3 * x3;

        // Avancera LFO-faser
        wowPhase     += juce::MathConstants<float>::twoPi * wowFreqHz / static_cast<float> (sampleRate);
        flutterPhase += juce::MathConstants<float>::twoPi * flutterFreqHz / static_cast<float> (sampleRate);
        if (wowPhase     > juce::MathConstants<float>::twoPi) wowPhase     -= juce::MathConstants<float>::twoPi;
        if (flutterPhase > juce::MathConstants<float>::twoPi) flutterPhase -= juce::MathConstants<float>::twoPi;

        return y;
    }

    void WowFlutter::process (juce::AudioBuffer<float>& buffer, int channel)
    {
        if (amount < 1e-6f) return;
        auto* data = buffer.getWritePointer (channel);
        const int n = buffer.getNumSamples();
        for (int i = 0; i < n; ++i)
            data[i] = processSample (data[i]);
    }
}
