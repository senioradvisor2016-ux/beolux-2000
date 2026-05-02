/*  DualSlideFader — två knoppar i samma slits (B&O skydepotentiometre).

    Manual-knap 13/14/15 har två separata faders för L och R kanaler i
    samma fysiska slits. Den vänstra knoppen styr L, den högra styr R.

    UAD-inspired UX:
      • Double-click → reset till default-värde
      • Mouse wheel → ±0.01 step
      • Cmd+drag → fine-mode (10× långsammare)
      • Right-click → snap-menu (-∞/-20/-10/0/+3)
      • Reference mark vid unity/default på track
      • Editable text-popup när man trycker Enter

    Plats: plugin/juce/Source/ui/DualSlideFader.h
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "BCColours.h"

namespace bc2000dl::ui
{
    class DualSlideFader : public juce::Component
    {
    public:
        DualSlideFader (const juce::String& sym = "");

        juce::Slider& getLeftSlider()  { return sliderL; }
        juce::Slider& getRightSlider() { return sliderR; }

        /** Sätt default-värde för båda kanalerna (visas som reference mark + dubbelklick reset). */
        void setDefaultValue (double defaultValue);

        /** Konfigurera snap-points som visas i right-click menyn (ex. {-INFINITY, -20, -10, 0, 3}). */
        void setSnapPoints (const juce::Array<double>& points);

        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        juce::String symbol;
        double       defaultVal { 0.5 };
        juce::Array<double> snapPoints;

        // Custom slider subclass — overrider mouseDown för right-click snap-menu
        class PremiumSlider : public juce::Slider
        {
        public:
            PremiumSlider (DualSlideFader& parent) : owner (parent) {}
            void mouseDown (const juce::MouseEvent& e) override;
            void mouseDrag (const juce::MouseEvent& e) override;
        private:
            DualSlideFader& owner;
        };

        PremiumSlider sliderL { *this }, sliderR { *this };
        juce::Label  symbolLabel;

        // Custom L&F för att rendera knopparna sida vid sida
        class DualLook : public juce::LookAndFeel_V4
        {
        public:
            DualLook (bool isLeft) : isLeftKnob (isLeft) {}
            void drawLinearSlider (juce::Graphics&,
                                   int x, int y, int w, int h,
                                   float sliderPos,
                                   float minSliderPos,
                                   float maxSliderPos,
                                   juce::Slider::SliderStyle,
                                   juce::Slider&) override;
        private:
            bool isLeftKnob;
        };
        DualLook lookL { true };
        DualLook lookR { false };

        // Hjälpfunktion: konfigurera UAD-stil mouse/keyboard på en juce::Slider
        void configurePremiumSliderUX (juce::Slider& s);
        void showSnapMenu (juce::Slider& s);

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DualSlideFader)
    };
}
