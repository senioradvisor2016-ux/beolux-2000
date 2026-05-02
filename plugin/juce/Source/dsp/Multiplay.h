/*  Multiplay — bounce-loop med generationsförluster.

    Manualens varning (s.8): 'Grundstøjen i enhver optagelse adderes også
    og vil på et tidspunkt blive hørbar.' Plus HF-roll-off per pass.

    Plats: plugin/juce/Source/dsp/Multiplay.h
*/

#pragma once

#include <random>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

namespace bc2000dl::dsp
{
    class Multiplay
    {
    public:
        Multiplay() = default;

        void prepare (double sampleRate, std::uint32_t seed = 0);
        void reset();
        void setGeneration (int gen);
        void setEnabled (bool e) { enabled = e; }

        float processSample (float x);
        void process (juce::AudioBuffer<float>& buffer, int channel);

    private:
        double sampleRate { 48000.0 };
        bool enabled { false };
        int generation { 1 };

        juce::dsp::IIR::Filter<float> hfFilter;
        std::mt19937 rng;
        std::normal_distribution<float> noiseDist { 0.0f, 1.0f };
        float noiseAmpLin { 0.0f };

        void updateForGeneration();
    };
}
