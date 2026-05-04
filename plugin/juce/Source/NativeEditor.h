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

#if JUCE_DEBUG
 #include <melatonin_inspector/melatonin_inspector.h>
#endif

namespace bc2000dl
{
    /** Reel-pair component — two photoreal tape reels with dynamic winding.
        - Supply (left) shrinks as audio plays; takeup (right) grows.
        - Rotation speed is angular-velocity = linearSpeed / currentRadius
          → smaller reel spins FASTER, just like real tape.
        - Motion blur applied at higher angular velocities.  */
    class ReelDeck : public juce::Component
    {
    public:
        ReelDeck() = default;

        void setActive (bool active) { isActive = active; }
        void setSpeed  (int speedIdx) { speedFactor = (speedIdx == 0 ? 0.5f : speedIdx == 1 ? 1.0f : 2.0f); }
        void setTapePosition (double posSeconds);   // DSP-coupled: drives tapeAmount from real time
        void setWowIntensity (float w) { wowIntensity = w; }
        void paint (juce::Graphics&) override;

    private:
        void onVBlank();
        juce::VBlankAttachment vblank { this, [this]{ onVBlank(); } };

        bool  isActive    { false };
        float speedFactor { 1.0f };
        float angleL      { 0.0f }, angleR { 0.0f };
        float tapeAmount  { 0.0f };
        float angVelL     { 0.0f }, angVelR { 0.0f };
        float wowIntensity { 0.0f };
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

    /** Live spectrum analyser — pulls samples from SignalChain FIFO,
        runs FFT, renders a glowing amber curve on a black background. */
    class SpectrumAnalyser : public juce::Component, private juce::Timer
    {
    public:
        SpectrumAnalyser() : forwardFFT (kFftOrder), window (kFftSize, juce::dsp::WindowingFunction<float>::hann)
        {
            startTimerHz (30);
        }
        void setSource (const float* buffer, std::atomic<int>* writeIdx, int bufSize)
        {
            srcBuffer = buffer;
            srcWriteIdx = writeIdx;
            srcSize = bufSize;
        }
        void paint (juce::Graphics&) override;
    private:
        void timerCallback() override;
        static constexpr int kFftOrder = 11;       // 2^11 = 2048
        static constexpr int kFftSize  = 1 << kFftOrder;
        static constexpr int kNumBins  = 96;
        juce::dsp::FFT forwardFFT;
        juce::dsp::WindowingFunction<float> window;
        float fftData[kFftSize * 2] {};
        float magnitudes[kNumBins] {};
        const float* srcBuffer { nullptr };
        std::atomic<int>* srcWriteIdx { nullptr };
        int srcSize { 0 };
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

    BC2000DLProcessor& audioProc;
    bc2000dl::ui::InstructionCardLnF lnf;
    juce::TooltipWindow tooltipWindow { this, 500 };

    // Real Gaussian shadows (JUCE-native, applied via setComponentEffect)
    juce::DropShadowEffect vuShadow, reelShadow, presetShadow;

    /** Right-click context menu for sliders — Reset / Type-in / Copy.
        Attached to every slider so any control supports the UAD-style RMB menu. */
    struct SliderContextMenu : public juce::MouseListener
    {
        void mouseDown (const juce::MouseEvent& e) override;
    };
    SliderContextMenu sliderMenu;

    // melatonin_inspector — Debug only. Cmd+Shift+I toggles inspector.
    // Compiled out in Release so it never appears in a DAW.
#if JUCE_DEBUG
    melatonin::Inspector inspector { *this };
#endif

    // ---- Top deck zone: reels + 3 analog VU meters + spectrum strip ----
    bc2000dl::ReelDeck reelDeck;
    bc2000dl::AnalogVU vuInL { "VU" };
    bc2000dl::AnalogVU vuInR { "VU" };
    bc2000dl::AnalogVU vuOut { "VU" };
    bc2000dl::SpectrumAnalyser spectrum;
    juce::String       counterText { "0000" };
    bool               recLedOn { false };       // record-LED state
    int                prevSpeedIdx { -1 };      // for speed-LED repaint detection

    // ---- 5 dual-faders (10 sliders) — instruction-card slide-faders ----
    struct DualFader
    {
        juce::Slider l, r;
        juce::Label  caption;
    };
    DualFader radio, phono, mic, drive, echo;

    // ---- 8 knobs ----
    juce::Slider knob_bias, knob_bass, knob_treble, knob_wow, knob_mult, knob_balance, knob_master;
    juce::Slider knob_asym;
    juce::Label  lbl_bias, lbl_bass, lbl_treble, lbl_wow, lbl_mult, lbl_balance, lbl_master;
    juce::Label  lbl_asym;

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
