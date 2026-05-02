/*  TransportLever — Y-form transport-knapp med ‹‹ och ›› pilar.

    5 lägen: REW · STOP (mid) · PLAY · FF · PAUSE.
    Visuell B&O-stil — silver-skaft + kula på toppen.

    Plats: plugin/juce/Source/ui/TransportLever.h
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "BCColours.h"

namespace bc2000dl::ui
{
    class TransportLever : public juce::Component
    {
    public:
        enum State { Stop, Play, FF, REW };

        TransportLever() = default;

        void setState (State s);
        State getState() const { return state; }

        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent& e) override;

        std::function<void (State)> onStateChange;

    private:
        State state { Stop };
    };

    /** Tape-counter — 3-digit räkneverk i röd LCD-stil. */
    class TapeCounter : public juce::Component, private juce::Timer
    {
    public:
        TapeCounter();
        void paint (juce::Graphics&) override;

        void setRunning (bool r) { running = r; }
        void resetCounter() { counter = 0; repaint(); }

    private:
        void timerCallback() override;
        bool running { false };
        int counter { 0 };
    };
}
