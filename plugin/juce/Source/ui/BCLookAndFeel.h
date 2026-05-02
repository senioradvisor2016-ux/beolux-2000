/*  BC2000DL Look & Feel — Hybrid Hero (B&O 1968-stil).

    - Slide-faders: silver-block med grip-lines på svart spår
    - Rotary: rund knab med liten silver-prick som pekare
    - Push-knappar: ljusgrå rektanglar med tunn skugga, röd när "armed"
    - Toggle-knappar: rektanglar, B&O-röd när active

    Plats: plugin/juce/Source/ui/BCLookAndFeel.h
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "BCColours.h"

namespace bc2000dl::ui
{
    class BCLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        BCLookAndFeel();
        ~BCLookAndFeel() override = default;

        // Slider-rendering (rotary + linear)
        void drawRotarySlider (juce::Graphics&,
                               int x, int y, int w, int h,
                               float sliderPos,
                               float rotaryStartAngle,
                               float rotaryEndAngle,
                               juce::Slider&) override;

        void drawLinearSlider (juce::Graphics&,
                               int x, int y, int w, int h,
                               float sliderPos,
                               float minSliderPos,
                               float maxSliderPos,
                               juce::Slider::SliderStyle,
                               juce::Slider&) override;

        // Toggle-knapp
        void drawToggleButton (juce::Graphics&,
                               juce::ToggleButton&,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

        // TextButton-bakgrund (photo-foto sprite från GLB)
        void drawButtonBackground (juce::Graphics&,
                                   juce::Button&,
                                   const juce::Colour& backgroundColour,
                                   bool shouldDrawButtonAsHighlighted,
                                   bool shouldDrawButtonAsDown) override;

        // TextButton-text (mörk på chrome-rocker)
        void drawButtonText (juce::Graphics&,
                             juce::TextButton&,
                             bool shouldDrawButtonAsHighlighted,
                             bool shouldDrawButtonAsDown) override;

        // ComboBox (för speed-väljare)
        void drawComboBox (juce::Graphics&,
                           int width, int height,
                           bool isButtonDown,
                           int buttonX, int buttonY, int buttonW, int buttonH,
                           juce::ComboBox&) override;

        // Label
        juce::Font getLabelFont (juce::Label&) override;

        // Slider textbox-färg
        juce::Label* createSliderTextBox (juce::Slider&) override;
    };
}
