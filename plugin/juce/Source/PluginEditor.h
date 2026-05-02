/*  BC2000DLEditor — Ferroflux 2000 · DL UI.
    Layout efter "Tape Deck Plugin.html" från handoff-bundle.
*/

#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "ui/BCLookAndFeel.h"
#include "ui/HybridHeroPanel.h"
#include "ui/VUMeter.h"
#include "ui/TransportLever.h"
#include "ui/DualSlideFader.h"
#include "ui/RecordIndicator.h"
#include "ui/UADStyleSelector.h"

class BC2000DLEditor : public juce::AudioProcessorEditor,
                       private juce::Timer
{
public:
    explicit BC2000DLEditor (BC2000DLProcessor& p);
    ~BC2000DLEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    void timerCallback() override;
    void applyPreset (int presetIndex);
    void showAboutDialog();

    BC2000DLProcessor& processor;
    bc2000dl::ui::BCLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltipWindow { this, 600 };
    bc2000dl::ui::HybridHeroPanel heroPanel;
    bc2000dl::ui::VUMeter meterL, meterR;
    bc2000dl::ui::TransportLever transportLever;
    bc2000dl::ui::TapeCounter tapeCounter;
    bc2000dl::ui::RecordIndicator recIndL { "L" }, recIndR { "R" };

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ChoiceAttach = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    // ----- B&O 5 dual-skydepotentiometre (autentisk BC2000 DL frontpanel) -----
    bc2000dl::ui::DualSlideFader radioFader { juce::String (juce::CharPointer_UTF8 ("\xe2\x99\xad")) };
    bc2000dl::ui::DualSlideFader phonoFader { juce::String (juce::CharPointer_UTF8 ("\xe2\x97\x8b")) };
    bc2000dl::ui::DualSlideFader micFader   { juce::String (juce::CharPointer_UTF8 ("\xe2\x99\x82")) };
    bc2000dl::ui::DualSlideFader sosFader   { "S" };
    bc2000dl::ui::DualSlideFader echoFader  { juce::String (juce::CharPointer_UTF8 ("\xe2\x97\x81")) };

    // ----- Behåller enkelfader-medlemmar (oanvända men kvar för kompabilitet) -----
    juce::Slider radioL, radioR, phonoL, phonoR, micL, micR;
    juce::Slider outputSlider, echoSlider;
    juce::Label radioL_lbl, radioR_lbl, phonoL_lbl, phonoR_lbl;
    juce::Label micL_lbl, micR_lbl, output_lbl, echo_lbl;

    // ----- Knob cluster (8 mörka rotaries) -----
    juce::Slider biasKnob, recEqKnob, wowKnob, satKnob;
    juce::Slider echoKnob, multKnob, bassKnob, balanceKnob;
    juce::Label bias_lbl, recEq_lbl, wow_lbl, sat_lbl;
    juce::Label echoR_lbl, mult_lbl, bass_lbl, balance_lbl;

    // ----- Selektorer (mörka segmented) -----
    juce::ComboBox speedBox, monitorBox, phonoModeBox, radioModeBox, tapeFormulaBox;
    juce::Label    speedLabel, monitorLabel, phonoModeLabel, radioModeLabel, tapeFormulaLabel;

    // ----- UAD-Studer-A800-style knapp-rader (ersätter dropdowns) -----
    std::unique_ptr<bc2000dl::ui::UADStyleSelector> speedSelector, formulaSelector,
                                                     phonoSelector, monitorSelector;

    // ----- Push-buttons (transport + funktion) -----
    juce::ToggleButton echoBtn, bypassBtn, speakerBtn, syncBtn;
    juce::ToggleButton recArm1Btn, recArm2Btn, track1Btn, track2Btn;
    juce::ToggleButton sosBtn, pauseBtn, paBtn;
    juce::ToggleButton spkExtBtn, spkIntBtn, spkMuteBtn, micLoZBtn;

    // ----- Preset/A/B header -----
    juce::ComboBox presetMenu;
    juce::TextButton prevPresetBtn { "<" }, nextPresetBtn { ">" };
    juce::TextButton aBtn { "A" }, bBtn { "B" };
    juce::ValueTree stateA, stateB;
    bool slotIsA { true };

    // ----- Attachments -----
    std::vector<std::unique_ptr<SliderAttach>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttach>> buttonAttachments;
    std::unique_ptr<ChoiceAttach> speedAttach, monitorAttach, phonoModeAttach,
                                  radioModeAttach, tapeFormulaAttach;

    juce::String statusText;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BC2000DLEditor)
};
