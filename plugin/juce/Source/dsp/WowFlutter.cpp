/*  WowFlutter implementation. */

#include "WowFlutter.h"
#include <cmath>

namespace bc2000dl::dsp
{
    void WowFlutter::prepare (double sr)
    {
        sampleRate = sr;

        // Max modulationsdjup i samples:
        //   (wowAmount + flutterAmount)_max × amount_max × 0.005 × sr
        //   = (0.0424 + 0.001698) × 2.0 × 0.005 × sr ≈ 4.4e-4 × sr  (~21 @ 48 kHz)
        // 5e-4 ger marginal. Basdelay = djup + 8 samples Lagrange-marginal.
        // Hela delayn är fast och PDC-rapporteras via getLatencySamples() —
        // tidigare 50 ms-buffer med 25 ms centrum var ~100× större än nödvändigt
        // och rapporterades aldrig till värden.
        const int maxMod = static_cast<int> (std::ceil (sr * 5.0e-4)) + 1;
        baseDelay = maxMod + 8;
        buf.assign (static_cast<size_t> (baseDelay + maxMod + 8), 0.0f);
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
        // Amounts calibrated to hit spec §7 at WowFlutterAmount=100%:
        // % = wowAmount * 0.005 * 2π * freqHz * 100  (samples/sample → %)
        // Wow (1.5 Hz):    Speed19 0.075%, Speed95 0.125%, Speed475 0.200%
        // Flutter (30 Hz): Speed19 0.065%, Speed95 0.100%, Speed475 0.160%
        switch (speed)
        {
            case TapeSpeed::Speed19:
                wowAmount = 0.0159f; flutterAmount = 0.000690f; break;
            case TapeSpeed::Speed95:
                wowAmount = 0.0265f; flutterAmount = 0.001061f; break;
            case TapeSpeed::Speed475:
                wowAmount = 0.0424f; flutterAmount = 0.001698f; break;
        }
    }

    float WowFlutter::processSample (float x)
    {
        if (buf.empty()) return x;

        // Skriv input till ring-buffer
        buf[static_cast<size_t> (writeIdx)] = x;
        writeIdx = (writeIdx + 1) % static_cast<int> (buf.size());

        // Ingen early-return på amount — delayvägen är alltid aktiv så
        // latensen är konstant (PDC-korrekt) även när wow/flutter
        // automatiseras genom noll.
        const float wowMod     = wowAmount * std::sin (wowPhase) * amount;
        const float flutterMod = flutterAmount * std::sin (flutterPhase) * amount;

        const float delaySamps = juce::jlimit (
            3.0f, static_cast<float> (buf.size()) - 2.0f,
            static_cast<float> (baseDelay)
                + (wowMod + flutterMod) * static_cast<float> (sampleRate) * 0.005f);

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
        auto* data = buffer.getWritePointer (channel);
        const int n = buffer.getNumSamples();
        for (int i = 0; i < n; ++i)
            data[i] = processSample (data[i]);
    }
}
