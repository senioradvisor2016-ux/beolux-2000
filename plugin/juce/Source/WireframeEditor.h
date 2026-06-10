/*  WireframeEditor — clean-slate native JUCE-UI för Beolux 2000 De Luxe.

    Estetik: B&O Beocord 2000 De Luxe operating-instructions card
    (cream-coloured paper diagram with thin black line-art, two reels,
     5 dual-fader-spår med skydepotentiometre (L+R = 10 individual caps),
     8 service-knobs i mörk strip för plugin-specifika tape-parametrar,
     manöverspak, räkneverk och hastighetsväljare på top deck.

    All paint i WireframeLookAndFeel — inga bitmap-assets, retina-clean.
    Layout-koordinater matchar design_inbox/beolux_2000_deluxe_wireframe.html (920×780).
*/

#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <melatonin_blur/melatonin_blur.h>
#include <melatonin_inspector/melatonin_inspector.h>
#include "PluginProcessor.h"
#include "ui/PresetBrowser.h"
#include "ui/FontAudioIcons.h"

// melatonin_perfetto är inte vendorerad i detta repo — TRACE_COMPONENT() är
// ren profilering. No-op:a den så WireframeEditor bygger utan perfetto-modulen.
#ifndef TRACE_COMPONENT
 #define TRACE_COMPONENT()
#endif

namespace bc2000dl::ui
{
    /** Wireframe palette + line-art LookAndFeel. */
    class WireframeLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        WireframeLookAndFeel();

        // Faders (LinearVertical) — slim chrome cap on thin track
        void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h,
                                float sliderPos, float minSliderPos, float maxSliderPos,
                                juce::Slider::SliderStyle, juce::Slider&) override;

        // Rotary knobs — circle with pointer line
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                                float sliderPos, float rotaryStartAngle,
                                float rotaryEndAngle, juce::Slider&) override;

        // Buttons — white/cream rectangle with thin stroke
        void drawButtonBackground (juce::Graphics&, juce::Button&,
                                    const juce::Colour&, bool, bool) override;
        void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
        void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;

        // ComboBox — flat panel with current-value text
        void drawComboBox (juce::Graphics&, int w, int h, bool, int, int, int, int,
                            juce::ComboBox&) override;
        void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
        juce::Font getComboBoxFont (juce::ComboBox&) override;
        juce::Font getLabelFont   (juce::Label&)    override;

        // Premium UAD-style tooltip — dark panel, chrome border, bold title + body
        juce::Rectangle<int> getTooltipBounds (const juce::String& tipText,
                                                  juce::Point<int> screenPos,
                                                  juce::Rectangle<int> parentArea) override;
        void drawTooltip (juce::Graphics&, const juce::String& text, int width, int height) override;

        // Photoreal palette — Beocord 2400 reference photo (warm teak) — UAD-grade
        static constexpr juce::uint32 kWoodTop    = 0xFFC8753E;  // warmer teak (top, reddish-brown)
        static constexpr juce::uint32 kWoodBot    = 0xFF6A3818;  // teak shadow (bottom)
        static constexpr juce::uint32 kWoodHi     = 0xFFE2A070;  // edge highlight (lighter top edge)
        static constexpr juce::uint32 kWoodGrain  = 0xFF5A2E10;  // darker grain streak (horizontal)
        static constexpr juce::uint32 kPanelBlk   = 0xFF161616;  // black anodized aluminium
        static constexpr juce::uint32 kPanelBlkHi = 0xFF2A2A2A;  // top edge highlight
        static constexpr juce::uint32 kPanelBlkLo = 0xFF0A0A0A;  // bottom shadow
        static constexpr juce::uint32 kAluHi      = 0xFFD8D3C5;  // brushed alu strip top
        static constexpr juce::uint32 kAluLo      = 0xFFA8A39A;  // brushed alu strip shadow
        static constexpr juce::uint32 kChromeHi   = 0xFFEEEEEE;
        static constexpr juce::uint32 kChromeMid  = 0xFFB8B8B8;
        static constexpr juce::uint32 kChromeLo   = 0xFF4E4E4E;
        static constexpr juce::uint32 kBtnFace    = 0xFFE8E5DC;  // off-white plastic
        static constexpr juce::uint32 kBtnShadow  = 0xFFB8B5AC;
        static constexpr juce::uint32 kSilk       = 0xFFEDEAE0;  // white silkscreen
        static constexpr juce::uint32 kSilkDim    = 0xFF8E8B82;
        static constexpr juce::uint32 kVuFace     = 0xFFE8DFC8;  // cream VU face
        static constexpr juce::uint32 kVuInk      = 0xFF1A1A1A;
        static constexpr juce::uint32 kRedLed     = 0xFFFF2826;
        static constexpr juce::uint32 kStroke     = 0xFF000000;
        static constexpr juce::uint32 kStrokeLite = 0xFF555555;

        // -------- Cached melatonin shadows for common LookAndFeel paint calls --------
        // Replaces procedural multi-stack ellipse shadows in drawRotarySlider /
        // drawButtonBackground.  Cached blur = smoother gradient + much faster
        // (~10× speedup on knob-heavy paint passes per perfetto traces).
        melatonin::DropShadow knobDropShadow   { juce::Colours::black.withAlpha (0.55f),  8, { 1, 3 } };
        melatonin::DropShadow knobInsetShadow  { juce::Colours::black.withAlpha (0.85f),  6, { 0, 2 } };
        melatonin::DropShadow buttonDropShadow { juce::Colours::black.withAlpha (0.45f),  6, { 0, 2 } };
    };

    /** Wireframe reel-deck with two animated reels (procedural line art). */
    class WireframeReelDeck : public juce::Component
    {
    public:
        WireframeReelDeck();
        void paint (juce::Graphics&) override;
        void setActive (bool a) { active = a; }
        void setSpeed  (int idx);
    private:
        void onVBlank();
        juce::VBlankAttachment vblank { this, [this] { onVBlank(); } };
        bool  active      { false };
        float angleL      { 0.0f };
        float angleR      { 0.0f };
        float speedFactor { 1.0f };
    };

    /** Wireframe VU — rectangular face + needle (procedural).
        Now includes a peak-hold marker: thin amber tick that tracks the most
        recent peak level and decays slowly back toward the current needle pos.
        Authentic 1960s broadcast/studio peak-indicator behaviour. */
    class WireframeVU : public juce::Component
    {
    public:
        explicit WireframeVU (const juce::String& chLabel);
        void setLevel (float dbfs);
        void paint   (juce::Graphics&) override;
    private:
        juce::String label;
        float current  { -60.0f };
        float peakHold { -60.0f };   // visible peak marker (decays toward current)
    };

    // Slider med fininställning: håll Shift eller Cmd/Ctrl under drag → 5×
    // lägre känslighet (precis ratt-trim). Dubbelklick-reset + värde-popup
    // sätts redan av setupFader/setupKnob. UAD/FabFilter-standard.
    class FineSlider : public juce::Slider
    {
    public:
        void mouseDown (const juce::MouseEvent& e) override
        {
            if (! baseCaptured) { baseSensitivity = getMouseDragSensitivity(); baseCaptured = true; }
            const bool fine = e.mods.isShiftDown() || e.mods.isCommandDown();
            setMouseDragSensitivity (fine ? baseSensitivity * 5 : baseSensitivity);
            juce::Slider::mouseDown (e);
        }
    private:
        int  baseSensitivity { 250 };
        bool baseCaptured     { false };
    };

    // TextButton med separat högerklicks-callback (user-preset-meny på
    // preset-namnet — vanlig DAW-idiom, kräver ingen extra panelyta).
    class MenuTextButton : public juce::TextButton
    {
    public:
        using juce::TextButton::TextButton;
        std::function<void()> onRightClick;
        void mouseDown (const juce::MouseEvent& e) override
        {
            if (e.mods.isPopupMenu() && onRightClick) { onRightClick(); return; }
            juce::TextButton::mouseDown (e);
        }
    };

    class WireframeEditor : public juce::AudioProcessorEditor,
                             private juce::Timer,
                             private juce::ComponentListener
    {
    public:
        explicit WireframeEditor (BC2000DLProcessor&);
        ~WireframeEditor() override;

        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        void timerCallback() override;
        void componentVisibilityChanged (juce::Component&) override;

        BC2000DLProcessor& audioProc;
        WireframeLookAndFeel lnf;
        juce::TooltipWindow tooltip { this, 600 };

        // ===== 10 individuella faders (5 källor × L+R) =====
        // 14 ♀ RADIO · 15 ◇ GRAM · 16 ◇ MIK · 12 EKO/SoS · 11 MASTER
        FineSlider faderRadioL, faderRadioR;
        FineSlider faderPhonoL, faderPhonoR;
        FineSlider faderMicL,   faderMicR;
        FineSlider faderEchoL,  faderEchoR;
        FineSlider faderMasterL, faderMasterR;

        // ===== 9 service-knobs + 2 selectors + 1 toggle (1968 service-manual auth) =====
        FineSlider knobBiasL, knobBiasR;                  // L/R bias trimmers (manual p.4)
        FineSlider knobSatL, knobSatR, knobWow;
        FineSlider knobPrintTh, knobStereoAsym, knobMultiplay;
        FineSlider knobMainsHum;                          // 50/60 Hz hum intensity
        juce::ComboBox cbTapeFormula;
        juce::ComboBox cbMainsHumFreq;                      // 50/60 Hz selector

        // ===== Höger zon =====
        juce::ComboBox cbSpeed;            // 28 (top-deck position; combo bound to choice)
        FineSlider   knobBass, knobTreble, knobBalance;  // #9 bas, #8 diskant, #10 balans
        FineSlider   knobInputTrim, knobOutputTrim;  // plugin-utility I/O trim
        FineSlider   knobMix, knobTapeNoise;         // Fas 2 plugin-utility (dry/wet + brus)
        juce::ToggleButton tEchoPluginOn;   // plugin-extension
        FineSlider   knobEchoTime, knobEchoFb;  // visuella plugin-knobs (ej APVTS-bundna)

        // ===== Vänster zon — rockers =====
        juce::ToggleButton btnTrack1, btnTrack2;          // 26, 27 → track_1, track_2
        juce::ToggleButton btnRecArm1, btnRecArm2;        // 23, 24 → rec_arm_1, rec_arm_2
        juce::ToggleButton btnSync, btnMoment;            // 25, 2  → synchroplay, pause
        juce::ToggleButton btnPA, btnForstarkare;         // 19, 21 → pa_enabled, bypass_tape
        juce::ToggleButton btnSoSOn;                       // 20    → sos_enabled
        juce::ToggleButton btnSpkInt, btnSpkExt, btnSpkMute; // 6   → speaker_int/ext/mute
        juce::ComboBox     cbMonitor;                      // 18    → monitor_mode (Source/Tape)

        // ===== Per-fader mode-tabs under faders =====
        juce::ComboBox cbRadioMode, cbPhonoMode;          // L/H
        juce::ComboBox cbMicMode;                          // 3-pos: LoZ50/LoZ200/HiZ

        // ===== Sub-components =====
        WireframeReelDeck reelDeck;
        // 4 narrow horizontal Kyoritsu VUs in a row: IN L · IN R · OUT L · OUT R
        WireframeVU       vuInL  { "IN L"  };
        WireframeVU       vuInR  { "IN R"  };
        WireframeVU       vuOutL { "OUT L" };
        WireframeVU       vuOutR { "OUT R" };

        // ===== Preset bank =====
        MenuTextButton   btnPresetName { "FACTORY" };
        juce::TextButton btnPresetPrev { "<" };
        juce::TextButton btnPresetNext { ">" };
        int  currentPresetIdx { 0 };
        void applyPreset (int idx);

        // ===== A/B-compare + user-preset-meny =====
        juce::TextButton btnAbA    { "A" };
        juce::TextButton btnAbB    { "B" };
        juce::TextButton btnAbCopy { juce::String::charToString ((juce::juce_wchar) 0x2192) }; // →
        void refreshAbButtons();
        void showUserPresetMenu();   // högerklick på preset-namnet

        // ===== Premium preset browser (UAD-style modal overlay) =====
        // Backdrop dims the case behind the browser and consumes clicks-outside.
        struct BrowserBackdrop : juce::Component
        {
            std::function<void()> onDismiss;
            BrowserBackdrop()
            {
                setInterceptsMouseClicks (true, false);
                setMouseCursor (juce::MouseCursor::PointingHandCursor);
            }
            void paint (juce::Graphics& g) override
            {
                // Soft radial vignette darker at edges → UAD-style modal scrim
                juce::ColourGradient grad (juce::Colours::black.withAlpha (0.55f),
                                            (float) getWidth()  * 0.5f,
                                            (float) getHeight() * 0.5f,
                                            juce::Colours::black.withAlpha (0.85f),
                                            0.0f, 0.0f, true);
                g.setGradientFill (grad);
                g.fillRect (getLocalBounds());
            }
            void mouseDown (const juce::MouseEvent&) override { if (onDismiss) onDismiss(); }
        };
        BrowserBackdrop browserBackdrop;
        bc2000dl::ui::PresetBrowser presetBrowser;

        // ===== Cached drop-shadows + inner-shadows (melatonin_blur) — UAD-grade depth =====
        melatonin::DropShadow  caseShadow         { juce::Colours::black.withAlpha (0.70f), 28, { 0, 10 } };
        melatonin::DropShadow  titleStripShadow   { juce::Colours::black.withAlpha (0.55f), 10, { 0,  4 } };
        melatonin::InnerShadow panelInnerShadow   { juce::Colours::black.withAlpha (0.92f), 12, { 0,  3 } };
        melatonin::InnerShadow counterInnerShadow { juce::Colours::black.withAlpha (0.95f),  4, { 0,  2 } };
        melatonin::DropShadow  reelShadow         { juce::Colours::black.withAlpha (0.60f), 20, { 0,  7 } };
        melatonin::DropShadow  vuBezelShadow      { juce::Colours::black.withAlpha (0.55f),  8, { 0,  3 } };
        melatonin::InnerShadow vuFaceInnerShadow  { juce::Colours::black.withAlpha (0.80f),  4, { 0,  2 } };
        melatonin::DropShadow  serviceStripShadow { juce::Colours::black.withAlpha (0.45f),  6, { 0,  3 } };
        melatonin::DropShadow  speedKnobShadow    { juce::Colours::black.withAlpha (0.45f),  6, { 0,  2 } };
        melatonin::DropShadow  manoverspakShadow  { juce::Colours::black.withAlpha (0.55f),  4, { 0,  2 } };
        melatonin::DropShadow  bAndOPlaqueShadow  { juce::Colours::black.withAlpha (0.50f),  5, { 0,  2 } };

        // ===== melatonin_inspector — Chrome DevTools-style overlay =====
        // ENDAST debug-build. Gated på JUCE_DEBUG → kompileras INTE in i release,
        // så den följer aldrig med pluginen till slutanvändare.
       #if JUCE_DEBUG
        melatonin::Inspector inspector { *this };
       #endif

        // ===== APVTS attachments =====
        using SAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
        using BAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
        using CAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
        std::vector<std::unique_ptr<SAtt>> sAtts;
        std::vector<std::unique_ptr<BAtt>> bAtts;
        std::vector<std::unique_ptr<CAtt>> cAtts;

        // ===== State =====
        int  prevSpeedIdx { -1 };
        juce::String counterText { "0000" };  // 4-digit räkneverk display (updated from tape position)

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WireframeEditor)
    };
} // namespace bc2000dl::ui
