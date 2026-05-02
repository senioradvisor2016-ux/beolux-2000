/*  VUMeter — animerad vintage-VU med 300 ms ballistik och röd record-zon.

    Tar input från en atomic float-source (uppdaterad från audio-thread).
    Återges som klassisk B&O-stil: vit/cream glas, svarta graderingar, röd
    HIGH-zon, mekanisk nål.

    Plats: plugin/juce/Source/ui/VUMeter.h
*/

#pragma once

#include <atomic>
#include <juce_gui_basics/juce_gui_basics.h>
#include "BCColours.h"

namespace bc2000dl::ui
{
    class VUMeter : public juce::Component, private juce::Timer
    {
    public:
        VUMeter (const juce::String& label = "L");
        ~VUMeter() override = default;

        /** Anropa från audio-thread vid varje block. Buffrar level-värdet. */
        void pushLevel (float dbFs);

        /** Aktivera röd belysning bakom (record-arm). */
        void setRecording (bool isRec)  { recording = isRec; repaint(); }

        /** Sätt VU-meter active-state — när inactive dimmas amber-glow (UAD VU-On/Off-pattern). */
        void setActive (bool isActive)  { active = isActive; repaint(); }

        void paint (juce::Graphics&) override;

    private:
        void timerCallback() override;

        juce::String label;
        std::atomic<float> targetLevel { -60.0f };  // dBFS
        float currentLevel { -60.0f };               // smoothat med ballistik
        bool recording { false };
        bool active    { true };   // false = dim amber-glow (transport stopped)

        // 300 ms RC-time-constant @ 30 Hz refresh
        static constexpr float kSmoothCoef = 0.10f;
    };
}
