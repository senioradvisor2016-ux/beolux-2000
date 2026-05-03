/*  PhonoPreamp 8904002 — UW0029 + 2N2613 + HS-switch (H=magnetisk, L=keramisk).

    H-läge: RIAA-aktiverat (LF-boost ~+20 dB @ 50 Hz, HF-cut)
    L-läge: nästan flat (keramiska har inbyggd RIAA-approximation)

    Plats: plugin/juce/Source/dsp/PhonoPreamp.h
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "Constants.h"
#include "Ge2N2613Stage.h"
#include "GeLowNoiseStage.h"

namespace bc2000dl::dsp
{
    enum class PhonoMode { H, L };  // H=magnetic, L=ceramic

    class PhonoPreamp
    {
    public:
        PhonoPreamp() = default;

        void prepare (double sampleRate,
                      float channelAsymmetry = 0.0f,
                      std::uint32_t baseSeed = 0);
        void reset();
        void setMode (PhonoMode m);
        void setChannelAsymmetry (float offset);

        void process (juce::AudioBuffer<float>& buffer, int channel);

    private:
        double sampleRate { 48000.0 };
        PhonoMode mode { PhonoMode::H };

        GeLowNoiseStage uw0029;
        Ge2N2613Stage   n2613;
        juce::dsp::IIR::Filter<float> riaaLf, riaaHf;

        void updateRIAA();
    };
}
