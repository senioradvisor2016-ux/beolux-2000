/*  WowFlutter — hastighetsbaserad pitch-modulation via delay-line.

    Wow (~1.5 Hz) + flutter (~30 Hz) modulerar tape-läsningen med
    Lagrange 3:e-ordningens interpolation. Mer modulation vid lägre
    hastighet (mindre mekanisk stabilitet per längdmm).

    Plats: plugin/juce/Source/dsp/WowFlutter.h
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Constants.h"

namespace bc2000dl::dsp
{
    class WowFlutter
    {
    public:
        WowFlutter() = default;

        void prepare (double sampleRate);
        void reset();
        void setSpeed (TapeSpeed speed);
        void setAmount (float a) { amount = juce::jlimit (0.0f, 2.0f, a); }

        float processSample (float x);
        void process (juce::AudioBuffer<float>& buffer, int channel);

    private:
        double sampleRate { 48000.0 };
        float amount { 1.0f };

        // Per-speed parametrar
        float wowAmount     { 0.0008f };
        float flutterAmount { 0.0006f };
        float wowFreqHz     { 1.5f };
        float flutterFreqHz { 30.0f };

        // Ring-buffer för delay-line (~50 ms max)
        std::vector<float> buf;
        int writeIdx { 0 };

        // LFO-faser
        float wowPhase     { 0.0f };
        float flutterPhase { 0.0f };
    };
}
