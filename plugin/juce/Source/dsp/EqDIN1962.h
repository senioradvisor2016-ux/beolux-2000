/*  EqDIN1962 — Pre-emphasis (record) och de-emphasis (playback) per hastighet.

    3 koeff-set per modul, byts via setSpeed(). Implementeras som LF-shelf +
    HF-shelf via JUCE:s juce::dsp::IIR::Filter (RBJ cookbook).

    Mismatchen mellan pre och de-emphasis är AVSIKTLIG — ger "tape glow".

    Plats: plugin/juce/Source/dsp/EqDIN1962.h
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "Constants.h"

namespace bc2000dl::dsp
{
    /** Bas-klass för 2-shelf-EQ. */
    class SwitchedShelfEq
    {
    public:
        struct Config
        {
            float lfCornerHz;
            float lfBoostDb;
            float hfCornerHz;
            float hfGainDb;
        };

        void prepare (double sampleRate);
        void reset();
        void setConfig (const Config& cfg);

        float processSample (float x);
        void process (juce::AudioBuffer<float>& buffer, int channel);

    private:
        double sampleRate { 48000.0 };
        Config currentCfg { 50.0f, 0.0f, 10000.0f, 0.0f };
        juce::dsp::IIR::Filter<float> lfFilter, hfFilter;
    };

    /** Playback-EQ: per-hastighet de-emphasis (LF-boost + HF-cut). */
    class PlaybackEqDIN1962 : public SwitchedShelfEq
    {
    public:
        void setSpeed (TapeSpeed speed);
    };

    /** Record-EQ: per-hastighet pre-emphasis (HF-boost). */
    class PreEmphasisDIN1962 : public SwitchedShelfEq
    {
    public:
        void setSpeed (TapeSpeed speed);
    };
}
