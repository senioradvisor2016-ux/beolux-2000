/*  UADStyleSelector — UAD-Studer-A800-style horisontell knapp-rad
    som ersätter dropdown-meny för flera-val (tape-formula, IPS, CAL).

    Plats: plugin/juce/Source/ui/UADStyleSelector.h
*/
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

namespace bc2000dl::ui
{
    class UADStyleSelector  : public juce::Component
    {
    public:
        UADStyleSelector (juce::AudioProcessorValueTreeState& apvts,
                          const juce::String& paramId,
                          const juce::StringArray& labels,
                          const juce::String& title = "");
        ~UADStyleSelector() override;

        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        juce::AudioProcessorValueTreeState& apvts;
        juce::String paramID;
        juce::String titleText;
        juce::OwnedArray<juce::TextButton> buttons;
        juce::ComboBox hiddenCombo;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;

        void setSelectedIndex (int idx);
        void updateButtonsFromCombo();

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UADStyleSelector)
    };
}
