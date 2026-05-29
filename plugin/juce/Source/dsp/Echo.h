/*  Echo — Auto-3-speed tape-echo.

    Hastighetsknappen byter automatiskt echo-tid till 75/150/300 ms vid
    19/9.5/4.75 cm/s. Self-osc-tröskel @ ~85 % feedback (manualens varning).

    Plats: plugin/juce/Source/dsp/Echo.h
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "Constants.h"

namespace bc2000dl::dsp
{
    class Echo
    {
    public:
        Echo() = default;

        void prepare (double sampleRate);
        void reset();

        void setSpeed (TapeSpeed speed);
        void setEnabled (bool e)    { enabled = e; }
        void setAmount (float a)    { amount = a; }
        bool isEnabled() const      { return enabled; }
        float getAmount() const     { return amount; }
        float getDelayMs() const    { return delayMs; }

        // v60.5 — Sound-on-Sound cross-feedback (Christoffer-feedback):
        // Med SoS aktivt ska echo-feedbacken gå L→R och R→L istället för
        // L→L och R→R.  setCrossFeedSource() ger en pointer till "partner"-
        // Echo-instansens delay-buffer.  null → normal (egen buffer).
        void setCrossFeedSource (const Echo* partner) { feedbackSource = partner; }

        float processSample (float x);
        void process (juce::AudioBuffer<float>& buffer, int channel);

        // Read-only-access till delay-buffer för cross-feed-partner
        const std::vector<float>& getDelayBuffer() const { return buf; }
        int                       getWriteIdx()    const { return writeIdx; }

    private:
        double sampleRate { 48000.0 };
        bool   enabled    { false };
        float  amount     { 0.0f };
        float  delayMs    { 75.0f };
        int    delaySamples { 3600 };

        // Ring-buffer (max 350 ms)
        std::vector<float> buf;
        int writeIdx { 0 };

        // v60.5 — SoS cross-feedback: NULL = läs egen buf (normal echo);
        // != NULL = läs partnerns buf för delayed-sample (cross-feedback).
        const Echo* feedbackSource { nullptr };

        // HF-loss per pass (LP-filter)
        juce::dsp::IIR::Filter<float> hfLossFilter;

        // Wow modulation of delay-readback (authentic tape-echo pitch-wander)
        float echoWowPhase  { 0.0f };
        float echoWowDepth  { 0.0f };  // samples — set per speed in setSpeed()
        float echoWowFreqHz { 1.5f };  // Hz — dominant wow fundamental
    };
}
