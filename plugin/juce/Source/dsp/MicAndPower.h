/*  MicTransformer8012003 + PowerAmp8004014.

    Två klasser, samlade här eftersom de är "kant"-block:
    - MicTransformer: input-trafo (lo-Z mic-mode), step-up + LF/HF + saturation
    - PowerAmp: klass-AB output med crossover + AUTOMATSIKRING
      (aktiveras när "speaker_monitor"-läge är på)

    Plats: plugin/juce/Source/dsp/MicAndPower.h
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "Constants.h"

namespace bc2000dl::dsp
{
    /** Ingångstransformator 8012003 (lo-Z mic). */
    class MicTransformer8012003
    {
    public:
        void prepare (double sampleRate);
        void reset();

        /** turns_ratio default 1.5 = +3.5 dB. Var 20 (= +26 dB step-up från
            mic-level), men i plugin-kontext med line-level-input gav det
            saturation före preamp ens nåtts. */
        void setTurnsRatio (float ratio) { turnsRatio = ratio; }

        float processSample (float x);
        void process (juce::AudioBuffer<float>& buffer, int channel);

    private:
        double sampleRate { 48000.0 };
        float turnsRatio  { 1.5f };

        juce::dsp::IIR::Filter<float> hpFilter;  // LF-roll-off ~25 Hz
        juce::dsp::IIR::Filter<float> lpFilter;  // HF-roll-off ~30 kHz

        // Saturation-parametrar
        static constexpr float kSatThreshold = 4.0f;
        static constexpr float kSatSoftness  = 1.5f;
    };

    /** Klass-AB power-amp 8004014. */
    class PowerAmp8004014
    {
    public:
        void prepare (double sampleRate);
        void reset();
        void setEnabled (bool e) { enabled = e; }

        float processSample (float x);
        void process (juce::AudioBuffer<float>& buffer, int channel);

    private:
        double sampleRate { 48000.0 };
        bool enabled { false };

        juce::dsp::IIR::Filter<float> hpFilter;  // Cap-coupling HP @ 5 Hz

        // Crossover-distorsion-parametrar
        static constexpr float kCrossoverThreshold = 0.005f;
        // AUTOMATSIKRING soft-clip-tröskel (~+14 dBu in pluginen)
        static constexpr float kAutomatsikring = 0.89f;  // 10^(-1/20)

        float crossoverDistortion (float x) const;
    };
}
