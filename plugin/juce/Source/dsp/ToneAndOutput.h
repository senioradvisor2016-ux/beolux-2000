/*  ToneControl + BalanceMaster + Mixer3Bus.

    Tre output-stage-klasser samlade i en fil för enkelhet:
    - ToneControl: 2-bands Baxandall (LF + HF shelves)
    - BalanceMaster: stereo-balance + master volume (sista stage)
    - Mixer3Bus: summerar 3 input-bussar (Mic + Phono + Radio)

    Plats: plugin/juce/Source/dsp/ToneAndOutput.h
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

namespace bc2000dl::dsp
{
    /** 2-bands Baxandall — placeras EFTER playback-amp (manual s.5). */
    class ToneControl
    {
    public:
        void prepare (double sampleRate);
        void reset();
        void setBassDb (float db);
        void setTrebleDb (float db);

        float processSample (float x);
        void process (juce::AudioBuffer<float>& buffer, int channel);

    private:
        double sampleRate { 48000.0 };
        float bassDb   { 0.0f };
        float trebleDb { 0.0f };
        juce::dsp::IIR::Filter<float> bassFilter, trebleFilter;
        void updateBass();
        void updateTreble();
    };

    /** Stereo balance + master volume (sista stage). */
    class BalanceMaster
    {
    public:
        void setBalance (float bal);  // -1=L, 0=mid, +1=R
        void setMaster (float m);     // 0–1, square-law mapping

        void processStereo (juce::AudioBuffer<float>& buffer);

    private:
        float balance { 0.0f };
        float master  { 0.75f };
    };

    /** 3-buss-mixer — summerar Mic+Phono+Radio till en utgång. */
    class Mixer3Bus
    {
    public:
        void setGains (float micG, float phonoG, float radioG)
        {
            micGain = juce::jlimit (0.0f, 1.0f, micG);
            phonoGain = juce::jlimit (0.0f, 1.0f, phonoG);
            radioGain = juce::jlimit (0.0f, 1.0f, radioG);
        }

        /** För enkel pipeline använder vi just-in-place mixing.
            Pluginen har bara en input-kanal i v0.1; mixern viktar bara den. */
        float scaleByActiveBuses (float x) const
        {
            // I v0.1: vi har bara 1 input → använd summan av aktiva fader-positioner
            // som total gain (matchar pythonprototypens beteende när alla bussar matas
            // med samma signal).
            return x * (micGain + phonoGain + radioGain);
        }

        float getMicGain() const   { return micGain; }
        float getPhonoGain() const { return phonoGain; }
        float getRadioGain() const { return radioGain; }

    private:
        float micGain   { 0.0f };
        float phonoGain { 0.0f };
        float radioGain { 0.0f };
    };
}
