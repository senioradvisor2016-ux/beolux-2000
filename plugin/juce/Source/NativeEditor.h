/*  NativeEditor — clean-slate native JUCE-UI för BC2000DL v29.8.

    Aestetik: Beocord 2000 De Luxe operating-instructions card
    (cream-coloured paper diagram with German labels, two reels at top,
     slide-faders below, transport keys at bottom — the schematic that
     lived under the lid of the real machine).

    Inspired by Juno60AF's procedural-drawing approach + custom L&F.
    All drawing in BC2000LookAndFeel — no bitmap assets, retina-clean.
*/

#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "ui/BC2000LookAndFeel.h"

namespace bc2000dl
{
    /** Reel-pair component — the two big circles at top of the card.
        Driven by the timer when input gain > threshold. */
    class ReelDeck : public juce::Component, private juce::Timer
    {
    public:
        ReelDeck();
        ~ReelDeck() override;

        void setActive (bool active) { isActive = active; }
        void setSpeed (int speedIdx)  { speedFactor = (speedIdx == 0 ? 0.5f : speedIdx == 1 ? 1.0f : 2.0f); }
        void paint (juce::Graphics&) override;

    private:
        void timerCallback() override;
        bool isActive { false };
        float speedFactor { 1.0f };
        float angleL { 0.0f }, angleR { 0.0f };
    };

    /** Horisontal VU-bar with calibrated -60..+6 dBFS range. */
    class VUBar : public juce::Component
    {
    public:
        VUBar() = default;
        void setLevel (float dbfs);
        void setRecording (bool r) { if (r != recording) { recording = r; repaint(); } }
        void paint (juce::Graphics&) override;
    private:
        float current { -60.0f };
        bool recording { false };
    };

    /** Analog VU meter with curved scale + needle on cream face.
        - Authentic 2nd-order ballistics (300ms attack/decay + overshoot)
        - Boot calibration sweep on first 1.5s (premium "device awakens" feel)
        - Amber peak-hold marker that holds for 1.5s then decays at -20dB/s */
    class AnalogVU : public juce::Component
    {
    public:
        explicit AnalogVU (const juce::String& chLabel = "L") : channel (chLabel)
        {
            bootStart = juce::Time::getMillisecondCounter();
        }
        void setLevel (float dbfs);              // smoothed
        void paint (juce::Graphics&) override;
        float getCurrentDb()  const { return current; }
        float getPeakHoldDb() const { return peakHoldDb; }
    private:
        float current     { -20.0f };
        float velocity    { 0.0f };
        float peakHoldDb  { -20.0f };
        int   peakHoldFrames { 0 };
        juce::uint32 bootStart { 0 };
        bool  peaking  { false };
        juce::String channel;
    };
}

class NativeEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit NativeEditor (BC2000DLProcessor&);
    ~NativeEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void applyPreset (int presetIndex);

    BC2000DLProcessor& processor;
    bc2000dl::ui::InstructionCardLnF lnf;
    juce::TooltipWindow tooltipWindow { this, 500 };

    // Real Gaussian shadows (JUCE-native, applied via setComponentEffect)
    juce::DropShadowEffect vuShadow, reelShadow;

    // ---- Top deck zone: reels + 3 analog VU meters (Left, Right, Output) ----
    bc2000dl::ReelDeck reelDeck;
    bc2000dl::AnalogVU vuInL { "VU" };
    bc2000dl::AnalogVU vuInR { "VU" };
    bc2000dl::AnalogVU vuOut { "VU" };
    juce::String       counterText { "0000" };
    double             counterSeconds { 0.0 };  // animated tape counter
    bool               recLedOn { false };       // record-LED state

    // ---- 5 dual-faders (10 sliders) — instruction-card slide-faders ----
    struct DualFader
    {
        juce::Slider l, r;
        juce::Label  caption;
    };
    DualFader radio, phono, mic, drive, echo;

    // ---- 7 knobs ----
    juce::Slider knob_bias, knob_bass, knob_treble, knob_wow, knob_mult, knob_balance, knob_master;
    juce::Label  lbl_bias, lbl_bass, lbl_treble, lbl_wow, lbl_mult, lbl_balance, lbl_master;

    // ---- 5 selectors ----
    juce::ComboBox cb_speed, cb_monitor, cb_phono, cb_radio, cb_formula;
    juce::Label    lbl_speed, lbl_monitor, lbl_phono, lbl_radio, lbl_formula;

    // ---- Toggle buttons ----
    juce::ToggleButton t_echo, t_bypass, t_speaker, t_sync;
    juce::ToggleButton t_loz, t_pa, t_sos, t_pause;

    // ---- Transport piano-keys (TextButtons styled by L&F) ----
    juce::TextButton k_rec1, k_rec2, k_trk1, k_trk2;
    juce::TextButton k_spkA, k_spkB, k_mute;

    // ---- Header (preset + A/B + about) ----
    juce::ComboBox  cb_preset;
    juce::TextButton btn_prev { "<" }, btn_next { ">" };
    juce::TextButton btn_a { "A" }, btn_b { "B" };
    juce::TextButton btn_about { "?" };
    juce::ValueTree stateA, stateB;
    bool slotIsA { true };

    // ---- APVTS attachments ----
    using SAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SAtt>> sAtts;
    std::vector<std::unique_ptr<BAtt>> bAtts;
    std::vector<std::unique_ptr<CAtt>> cAtts;

    juce::String statusText;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NativeEditor)
};
