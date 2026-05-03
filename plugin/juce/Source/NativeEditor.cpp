/*  NativeEditor — hardware-accurate BC2000DL v29.9.

    Layout mirrors the physical Beocord 2000 De Luxe:

      ┌─ teak ──── BRUSHED ALUMINIUM ──────────────────────── teak ─┐
      │              [BC2000DL]  [BEOCORD 2000 DE LUXE]              │
      │   ◉ REEL L         [counter]         REEL R ◉                │
      └────────────── (thin black metallic divider) ─────────────────┘
      ┌─ teak ──── BLACK PANEL ─────────────────────────── teak ─────┐
      │  [preset <  combo  > A B ?]                                   │
      │  ─────────────┬──────────────────────┬────────────────────── │
      │  VU L ──────  │  ║RADIO║PHONO║MIC║   │  TREBLE BASS BAL      │
      │  VU R ──────  │  ║     ║     ║   ║   │                       │
      │  SPEED combo  │  ║DRIVE║ECHO ║   ║   │  BIAS WOW MULT MASTER │
      │  MONITOR      │  ║     ║     ║   ║   │                       │
      │  PHONO        │                       │  [counter]            │
      │  RADIO        │                       │                       │
      │  FORMULA      │                       │                       │
      │  toggles ×8   │                       │                       │
      │  keys ×7      │                       │                       │
      └───────────────┴──────────────────────┴───────────────────────┘
*/

#include "NativeEditor.h"

namespace
{
    using LnF = bc2000dl::ui::InstructionCardLnF;

    constexpr int kEditorW    = 1220;
    constexpr int kEditorH    = 482;    // even tighter — full horizontal strip
    constexpr int kTeakW      = 32;     // wood end-cap width (each side)
    constexpr int kInnerW     = kEditorW - 2 * kTeakW;
    constexpr int kAluH       = 156;    // black-metal deck zone height
    constexpr int kDivH       = 3;      // metallic divider height
    constexpr int kPresetH    = 28;     // preset/nav bar height
    constexpr int kLeftColW   = 400;    // left column (selectors + toggles + transport)
    constexpr int kCenterColW = 400;    // center column (5 dual-faders)
    // right column width = kInnerW - kLeftColW - kCenterColW = 356

    juce::String tooltipFor (const juce::String& id)
    {
        if (id == "speed")            return "Bandhastighet (4.75 / 9.5 / 19 cm/s)";
        if (id == "monitor_mode")     return "Source = pre-tape · Tape = post-tape";
        if (id == "phono_mode")       return "L = ceramic · H = magnetic (RIAA)";
        if (id == "radio_mode")       return "L = 3 mV · H = 100 mV line";
        if (id == "tape_formula")     return "Agfa / BASF / Scotch tape-emulering";
        if (id == "saturation_drive") return "Tape-saturation drive";
        if (id == "echo_amount")      return "Echo-mängd (high = self-oscillation)";
        if (id == "bias_amount")      return "Bias-ström (under nominal = mer 3rd harm)";
        if (id == "wow_flutter")      return "Wow & flutter-amplitud";
        if (id == "multiplay_gen")    return "Multiplay-generation 1-5";
        if (id == "master_volume")    return "Master output";
        if (id == "balance")          return "L/R-balans";
        if (id == "bass_db")          return "Bass (Baxandall, post-playback)";
        if (id == "treble_db")        return "Treble (Baxandall, post-playback)";
        if (id == "echo_enabled")     return "Aktivera echo-loop";
        if (id == "bypass_tape")      return "Bypass tape — ren input + tone";
        if (id == "speaker_monitor")  return "AD139 power-amp + soft-clip";
        if (id == "synchroplay")      return "Record-headet som playback";
        if (id == "mic_loz")          return "Lo-Z mic via ingångstrafo";
        if (id == "pa_enabled")       return "P.A.: mic duckar phono+radio";
        if (id == "sos_enabled")      return "Sound-on-Sound";
        if (id == "pause")            return "Tysta output";
        if (id == "rec_arm_1")        return "Arma spår 1";
        if (id == "rec_arm_2")        return "Arma spår 2";
        if (id == "track_1")          return "Lyssna spår 1";
        if (id == "track_2")          return "Lyssna spår 2";
        if (id == "speaker_ext")      return "Extern högtalare A";
        if (id == "speaker_int")      return "Intern högtalare B";
        if (id == "speaker_mute")     return "Mute alla högtalare";
        return {};
    }
}

//=============================================================================
//  ReelDeck — two animated bullseye reels + head assembly
//=============================================================================
namespace bc2000dl
{
    ReelDeck::ReelDeck() { startTimerHz (30); }
    ReelDeck::~ReelDeck() = default;

    void ReelDeck::timerCallback()
    {
        if (isActive)
        {
            const float delta = 0.09f * speedFactor;
            angleL += delta;
            angleR += delta;
            if (angleL > juce::MathConstants<float>::twoPi) angleL -= juce::MathConstants<float>::twoPi;
            if (angleR > juce::MathConstants<float>::twoPi) angleR -= juce::MathConstants<float>::twoPi;
            repaint();
        }
    }

    void ReelDeck::paint (juce::Graphics& g)
    {
        auto bounds = getLocalBounds();

        // Reels are ~40% of total width each. Centre gap is reserved for the
        // analog VU meters (placed by the editor as separate components).
        const int reelDiam = juce::jmin (bounds.getHeight() - 6, (int) (bounds.getWidth() * 0.36f));
        const int reelY    = bounds.getCentreY() - reelDiam / 2;

        const juce::Rectangle<int> leftReel  (bounds.getX(),                  reelY, reelDiam, reelDiam);
        const juce::Rectangle<int> rightReel (bounds.getRight() - reelDiam,   reelY, reelDiam, reelDiam);

        ui::InstructionCardLnF::drawReel (g, leftReel,  angleL, isActive);
        ui::InstructionCardLnF::drawReel (g, rightReel, -angleR * 0.7f, isActive);

        // Tape path: thin metallic line spanning the gap (passing behind the VU pair)
        const float tapeY = (float) bounds.getCentreY();
        g.setColour (juce::Colour (0xFFA0A0A8).withAlpha (0.45f));
        g.drawLine ((float) leftReel.getRight() - reelDiam * 0.05f, tapeY,
                    (float) rightReel.getX()    + reelDiam * 0.05f, tapeY, 0.8f);
    }

    //=========================================================================
    //  Analog VU meter
    //=========================================================================
    void AnalogVU::setLevel (float dbfs)
    {
        // ---- Boot calibration sweep (first 1500 ms after open) ----
        // The needle sweeps from -20 → +3 → back to current, like a pro meter
        // self-test on power-up.
        const auto now = juce::Time::getMillisecondCounter();
        const auto bootElapsed = (int) (now - bootStart);
        if (bootElapsed < 1500)
        {
            const float t = (float) bootElapsed / 1500.0f;
            // Triangle wave: 0 → 1 → 0 over the 1.5s
            const float tri = (t < 0.5f ? t * 2.0f : (1.0f - t) * 2.0f);
            // Map 0..1 → -20..+3 dB
            current = -20.0f + tri * 23.0f;
            velocity = 0.0f;
            peakHoldDb = current;
            peakHoldFrames = 0;
            repaint();
            return;
        }

        // ---- Authentic VU ballistic: 2nd-order spring/damper ----
        const auto target = juce::jlimit (-30.0f, 6.0f, dbfs);
        const float spring  = 0.18f;
        const float damping = 0.50f;
        const float diff    = target - current;
        velocity += diff * spring;
        velocity *= (1.0f - damping);
        current  += velocity;

        // ---- Peak-hold needle: chase up instantly, hold ~1.5s, then decay ----
        if (current > peakHoldDb)
        {
            peakHoldDb     = current;
            peakHoldFrames = 0;
        }
        else
        {
            ++peakHoldFrames;
            if (peakHoldFrames > 45)              // 1.5s hold @ 30Hz
                peakHoldDb -= 20.0f / 30.0f;       // -20 dB/s decay
            peakHoldDb = juce::jmax (peakHoldDb, current);
        }

        const bool nowPeaking = current > 0.0f;
        if (nowPeaking != peaking) { peaking = nowPeaking; }
        repaint();
    }

    void AnalogVU::paint (juce::Graphics& g)
    {
        ui::InstructionCardLnF::drawAnalogVU (g, getLocalBounds(), current, peaking, channel);
        // Overlay: amber peak-hold needle
        if (peakHoldDb > -19.0f && std::abs (peakHoldDb - current) > 0.5f)
        {
            const auto bf = getLocalBounds().toFloat().reduced (1.0f);
            const float screwBuffer = juce::jmax (1.8f, juce::jmin (bf.getWidth(), bf.getHeight()) * 0.018f) * 2.0f + 6.0f;
            const auto face = bf.reduced (screwBuffer, screwBuffer * 0.7f);
            const float pivotX = face.getCentreX();
            const float pivotY = face.getBottom() + face.getHeight() * 0.35f;
            const float arcR   = face.getHeight() * 1.05f;
            const float ang0 = -juce::MathConstants<float>::pi * 0.21f;
            const float ang1 =  juce::MathConstants<float>::pi * 0.21f;
            const float vuNorm = juce::jlimit (0.0f, 1.0f, (peakHoldDb + 20.0f) / 23.0f);
            const float a = ang0 + vuNorm * (ang1 - ang0);
            const float r1 = arcR;
            const float r2 = arcR - 6.0f;
            g.setColour (juce::Colour (0xFFC2A050).withAlpha (0.85f));
            g.drawLine (pivotX + r1 * std::sin (a), pivotY - r1 * std::cos (a),
                        pivotX + r2 * std::sin (a), pivotY - r2 * std::cos (a), 1.4f);
        }
    }

    // (Old VUBar removed — replaced by AnalogVU above.)
}

//=============================================================================
//  NativeEditor
//=============================================================================
NativeEditor::NativeEditor (BC2000DLProcessor& p)
    : juce::AudioProcessorEditor (p), processor (p)
{
    setLookAndFeel (&lnf);
    setSize (kEditorW, kEditorH);
    setResizable (false, false);

    // Helper: white silkscreen label on black panel — never intercepts mouse.
    auto creamLbl = [&] (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setFont (LnF::sectionFont (9.0f));
        l.setColour (juce::Label::textColourId, juce::Colour (0xFFE0E0E4));
        l.setJustificationType (juce::Justification::centred);
        l.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (l);
    };

    // ---- Top deck zone: reels + 3 analog VU meters (IN L, IN R, OUT) ----
    // Real Gaussian drop-shadows under the meters and reel deck — sells
    // the "machined-into-a-recess" look at the bezels and reel rims.
    vuShadow.setShadowProperties (
        juce::DropShadow (juce::Colours::black.withAlpha (0.55f), 8, { 0, 3 }));
    reelShadow.setShadowProperties (
        juce::DropShadow (juce::Colours::black.withAlpha (0.45f), 12, { 0, 4 }));

    vuInL.setComponentEffect (&vuShadow);
    vuInR.setComponentEffect (&vuShadow);
    vuOut.setComponentEffect (&vuShadow);
    reelDeck.setComponentEffect (&reelShadow);

    addAndMakeVisible (reelDeck);
    addAndMakeVisible (vuInL);
    addAndMakeVisible (vuInR);
    addAndMakeVisible (vuOut);

    // ---- 5 dual-faders ----
    // Bulletproof drag config: snap to mouse, no velocity-mode (predictable),
    // tight sensitivity, popup readout on drag. Enabled mouse cursor confirms
    // the cap is grabbable.
    auto setupDual = [&] (DualFader& f, const juce::String& cap,
                           const juce::String& idL, const juce::String& idR)
    {
        for (auto* sl : { &f.l, &f.r })
        {
            sl->setSliderStyle (juce::Slider::LinearVertical);
            sl->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            sl->setDoubleClickReturnValue (true, 0.0);
            sl->setSliderSnapsToMousePosition (true);
            sl->setVelocityBasedMode (false);
            sl->setMouseDragSensitivity (140);
            sl->setScrollWheelEnabled (true);
            sl->setPopupDisplayEnabled (true, true, this, 1500);
            sl->setMouseCursor (juce::MouseCursor::PointingHandCursor);
            addAndMakeVisible (sl);
        }
        f.l.setTooltip (tooltipFor (idL));
        f.r.setTooltip (tooltipFor (idR));
        sAtts.push_back (std::make_unique<SAtt> (processor.apvts, idL, f.l));
        sAtts.push_back (std::make_unique<SAtt> (processor.apvts, idR, f.r));

        f.caption.setText (cap, juce::dontSendNotification);
        f.caption.setFont (LnF::sectionFont (10.5f));
        f.caption.setColour (juce::Label::textColourId, juce::Colour (0xFFE0E0E4));
        f.caption.setJustificationType (juce::Justification::centred);
        f.caption.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (f.caption);
    };
    setupDual (radio, "RADIO", "radio_gain",       "radio_gain_r");
    setupDual (phono, "PHONO", "phono_gain",       "phono_gain_r");
    setupDual (mic,   "MIC",   "mic_gain",         "mic_gain_r");
    setupDual (drive, "DRIVE", "saturation_drive", "saturation_drive_r");
    setupDual (echo,  "ECHO",  "echo_amount",      "echo_amount_r");

    // ---- 7 knobs — vertical-drag rotary (move up/down = turn) ----
    auto setupKnob = [&] (juce::Slider& s, juce::Label& l, const juce::String& id,
                           const juce::String& cap)
    {
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                juce::MathConstants<float>::pi * 2.8f, true);
        s.setVelocityBasedMode (false);
        s.setMouseDragSensitivity (160);
        s.setScrollWheelEnabled (true);
        s.setPopupDisplayEnabled (true, true, this, 1500);
        s.setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        s.setTooltip (tooltipFor (id));
        addAndMakeVisible (s);
        sAtts.push_back (std::make_unique<SAtt> (processor.apvts, id, s));
        creamLbl (l, cap);
        l.setInterceptsMouseClicks (false, false);
    };
    setupKnob (knob_treble,  lbl_treble,  "treble_db",      "TREBLE");
    setupKnob (knob_bass,    lbl_bass,    "bass_db",        "BASS");
    setupKnob (knob_balance, lbl_balance, "balance",        "BAL");
    setupKnob (knob_bias,    lbl_bias,    "bias_amount",    "BIAS");
    setupKnob (knob_wow,     lbl_wow,     "wow_flutter",    "WOW");
    setupKnob (knob_mult,    lbl_mult,    "multiplay_gen",  "MULT");
    setupKnob (knob_master,  lbl_master,  "master_volume",  "VOL");

    // ---- 5 selectors ----
    auto setupCombo = [&] (juce::ComboBox& c, juce::Label& l, const juce::String& id,
                            const juce::String& cap,
                            std::initializer_list<const char*> items)
    {
        int idx = 1;
        for (auto* it : items) c.addItem (it, idx++);
        c.setTooltip (tooltipFor (id));
        addAndMakeVisible (c);
        cAtts.push_back (std::make_unique<CAtt> (processor.apvts, id, c));

        l.setText (cap, juce::dontSendNotification);
        l.setFont (LnF::sectionFont (8.5f));
        l.setColour (juce::Label::textColourId, juce::Colour (0xFFB8B8BE));
        l.setJustificationType (juce::Justification::centredLeft);
        l.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (l);
    };
    setupCombo (cb_speed,   lbl_speed,   "speed",        "TAPE SPEED",  { "4.75", "9.5", "19 cm/s" });
    setupCombo (cb_monitor, lbl_monitor, "monitor_mode", "MONITOR",     { "Source", "Tape" });
    setupCombo (cb_phono,   lbl_phono,   "phono_mode",   "PHONO IN",    { "L (ceramic)", "H (magnetic)" });
    setupCombo (cb_radio,   lbl_radio,   "radio_mode",   "RADIO IN",    { "L (3 mV)", "H (100 mV)" });
    setupCombo (cb_formula, lbl_formula, "tape_formula", "TAPE FORMULA",{ "Agfa", "BASF", "Scotch" });

    // ---- Toggle buttons ----
    auto setupToggle = [&] (juce::ToggleButton& b, const juce::String& cap, const juce::String& id)
    {
        b.setButtonText (cap);
        b.setTooltip (tooltipFor (id));
        addAndMakeVisible (b);
        bAtts.push_back (std::make_unique<BAtt> (processor.apvts, id, b));
    };
    setupToggle (t_echo,    "ECHO",    "echo_enabled");
    setupToggle (t_bypass,  "BYPASS",  "bypass_tape");
    setupToggle (t_speaker, "SPEAKER", "speaker_monitor");
    setupToggle (t_sync,    "SYNC",    "synchroplay");
    setupToggle (t_loz,     "LO-Z",    "mic_loz");
    setupToggle (t_pa,      "P.A.",    "pa_enabled");
    setupToggle (t_sos,     "SOS",     "sos_enabled");
    setupToggle (t_pause,   "PAUSE",   "pause");

    // ---- Transport keys ----
    auto setupKey = [&] (juce::TextButton& b, const juce::String& cap, const juce::String& id)
    {
        b.setButtonText (cap);
        b.setClickingTogglesState (true);
        b.setTooltip (tooltipFor (id));
        if (cap.startsWith ("REC")) b.setName ("REC");
        addAndMakeVisible (b);
        bAtts.push_back (std::make_unique<BAtt> (processor.apvts, id, b));
    };
    setupKey (k_rec1, "REC 1", "rec_arm_1");
    setupKey (k_rec2, "REC 2", "rec_arm_2");
    setupKey (k_trk1, "TRK 1", "track_1");
    setupKey (k_trk2, "TRK 2", "track_2");
    setupKey (k_spkA, "SPK A", "speaker_ext");
    setupKey (k_spkB, "SPK B", "speaker_int");
    setupKey (k_mute, "MUTE",  "speaker_mute");

    // ---- Preset header ----
    {
        int pid = 1;
        for (auto* name : { "FACTORY", "BERG · DEPECHE PAD", "BERG · DELTA SYNTH",
                             "BERG · PROTON CRUNCH", "BERG · REEL VOCALS", "FERRO+",
                             "CHROME", "METAL IV", "BBC J", "NAGRA", "CASSETTE 70s",
                             "BROADCAST", "VISCONTI '74", "BUTLER MIX", "KRAFTWERK '78",
                             "RHODES WARM", "DRUM SMASH", "VOCAL SILK", "BASS WEIGHT",
                             "LO-FI POP", "DUB ECHO", "GLUE MASTER", "SHOEGAZE",
                             "CRT NIGHTMARE" })
            cb_preset.addItem (name, pid++);
        cb_preset.setText ("FACTORY", juce::dontSendNotification);
        cb_preset.onChange = [this] {
            const int sel = cb_preset.getSelectedId() - 1;
            if (sel >= 0) applyPreset (sel);
        };
        addAndMakeVisible (cb_preset);
    }

    auto setupNav = [&] (juce::TextButton& b, int delta)
    {
        addAndMakeVisible (b);
        b.onClick = [this, delta]
        {
            const int n    = cb_preset.getNumItems();
            int       curr = cb_preset.getSelectedId();
            if (curr < 1) curr = 1;
            cb_preset.setSelectedId (((curr - 1 + delta + n) % n) + 1);
        };
    };
    setupNav (btn_prev, -1);
    setupNav (btn_next, +1);

    auto setupAB = [&] (juce::TextButton& b, bool isA)
    {
        b.setClickingTogglesState (true);
        b.setRadioGroupId (777);
        addAndMakeVisible (b);
        b.onClick = [this, isA]
        {
            auto& store = slotIsA ? stateA : stateB;
            store   = processor.apvts.copyState();
            slotIsA = isA;
            auto& load = slotIsA ? stateA : stateB;
            if (load.isValid()) processor.apvts.replaceState (load);
        };
    };
    setupAB (btn_a, true);
    setupAB (btn_b, false);
    btn_a.setToggleState (true, juce::dontSendNotification);

    btn_about.setTooltip ("Om BC2000DL");
    btn_about.onClick = []
    {
        juce::AlertWindow::showAsync (
            juce::MessageBoxOptions()
                .withIconType (juce::MessageBoxIconType::InfoIcon)
                .withTitle ("Beolux 2000 · v33.0")
                .withMessage ("BEOLUX 2000 — Danish Tape Emulation\n"
                              "by SOUNDBOYS\n\n"
                              "Inspired by the Bang & Olufsen Beocord 2000\n"
                              "De Luxe reel-to-reel (1968-69).\n\n"
                              "DSP: Jiles-Atherton tape hysteresis · 8× oversampling\n"
                              "21/21 PASS vs Studio-Sound + Service Manual\n\n"
                              "UI: native JUCE · hardware-accurate aesthetic\n"
                              "Teak frame · black-metal deck · 3D bullseye reels\n"
                              "Analog VU meters · cream instruction-card panel")
                .withButton ("OK"), nullptr);
    };
    addAndMakeVisible (btn_about);

    stateA = processor.apvts.copyState();
    startTimerHz (30);

    // melatonin_inspector — enable keyboard focus so Cmd+Shift+I toggles it
    setWantsKeyboardFocus (true);
}

NativeEditor::~NativeEditor()
{
    setLookAndFeel (nullptr);
}

//=============================================================================
//  Paint
//=============================================================================
void NativeEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // ===== Teak end-caps (full height, each side) =====
    LnF::drawWoodEndCap (g, bounds.withWidth (kTeakW), true);
    LnF::drawWoodEndCap (g, bounds.withTrimmedLeft (bounds.getWidth() - kTeakW), false);

    // ===== Inner zone (between teak caps) =====
    auto inner = bounds.withTrimmedLeft (kTeakW).withTrimmedRight (kTeakW);

    // ===== Brushed aluminium top zone =====
    auto aluZone = inner.withHeight (kAluH);
    LnF::drawPaperPanel (g, aluZone);

    // Title (top-left of alu deck)
    LnF::drawTitle (g, aluZone.reduced (14, 3).removeFromTop (20),
                     "BEOLUX 2000", "SOUNDBOYS · DANISH TAPE EMULATION · v44.0");

    // Counter (bottom-centre of deck, just below the VU row)
    {
        constexpr int cW = 76, cH = 18;
        LnF::drawCounter (g,
            juce::Rectangle<int> (inner.getCentreX() - cW / 2,
                                  aluZone.getBottom() - cH - 4,
                                  cW, cH),
            counterText);
    }

    // ===== Recording LED (red dot to the LEFT of the counter, lights when running) =====
    {
        const float lr = 3.5f;
        const float lx = (float) inner.getCentreX() - 76.0f;
        const float ly = (float) (aluZone.getBottom() - 13);
        if (recLedOn)
        {
            // Glow halo
            g.setColour (juce::Colour (0xFFFF6050).withAlpha (0.55f));
            g.fillEllipse (lx - lr * 2.4f, ly - lr * 2.4f, lr * 4.8f, lr * 4.8f);
            // LED body
            juce::ColourGradient lg (
                juce::Colour (0xFFFFE070), lx - lr * 0.4f, ly - lr * 0.5f,
                juce::Colour (0xFFA01810), lx + lr * 0.4f, ly + lr * 0.5f, false);
            g.setGradientFill (lg);
            g.fillEllipse (lx - lr, ly - lr, lr * 2, lr * 2);
            g.setColour (juce::Colours::white.withAlpha (0.85f));
            g.fillEllipse (lx - lr * 0.4f, ly - lr * 0.7f, lr * 0.7f, lr * 0.5f);
        }
        else
        {
            g.setColour (juce::Colour (0xFF1A1A1A));
            g.fillEllipse (lx - lr, ly - lr, lr * 2, lr * 2);
            g.setColour (juce::Colour (0xFF505056).withAlpha (0.65f));
            g.drawEllipse (lx - lr, ly - lr, lr * 2, lr * 2, 0.6f);
        }
        // "REC" silkscreen label below
        g.setColour (juce::Colour (0xFFB8B8BE).withAlpha (0.85f));
        g.setFont (LnF::sectionFont (6.5f));
        g.drawText ("REC",
            juce::Rectangle<int> ((int) lx - 12, (int) ly + (int) lr + 2, 24, 8),
            juce::Justification::centred, false);
    }

    // VU-meter engraved headers (silver on black metal, above each meter)
    {
        struct Hdr { juce::Rectangle<int> r; const char* text; };
        const auto& vuLb = vuInL.getBounds();
        const auto& vuRb = vuInR.getBounds();
        const auto& vuOb = vuOut.getBounds();
        const int hY = juce::jmax (aluZone.getY(), vuLb.getY() - 18);
        const int hH = 14;

        Hdr hs[] = {
            { { vuLb.getX(), hY, vuLb.getWidth(), hH }, "LEFT"   },
            { { vuRb.getX(), hY, vuRb.getWidth(), hH }, "RIGHT"  },
            { { vuOb.getX(), hY, vuOb.getWidth(), hH }, "OUTPUT" }
        };
        for (auto& h : hs)
        {
            const auto rr = h.r.toFloat();
            g.setFont (LnF::sectionFont (10.5f));
            // engraved-into-metal effect: dark shadow below, bright silver on top
            g.setColour (juce::Colours::black.withAlpha (0.85f));
            g.drawText (h.text, h.r.translated (0, 1), juce::Justification::centred, false);
            g.setColour (juce::Colour (0xFFE0E0E4));
            g.drawText (h.text, h.r, juce::Justification::centred, false);
        }
    }

    // ===== Divider strip =====
    auto divRect = inner.withY (kAluH).withHeight (kDivH);
    g.setColour (juce::Colour (0xFF060608));
    g.fillRect (divRect);
    g.setColour (juce::Colour (0xFF404048).withAlpha (0.5f));
    g.drawHorizontalLine (divRect.getY() + 1, (float) divRect.getX(), (float) divRect.getRight());

    // ===== Black panel (bottom zone) =====
    auto blackZone = inner.withY (kAluH + kDivH)
                         .withHeight (bounds.getHeight() - kAluH - kDivH);
    LnF::drawBlackPanel (g, blackZone);

    // Column dividers — thin chrome lines on black panel
    const int leftEnd   = kTeakW + kLeftColW;
    const int centerEnd = leftEnd + kCenterColW;
    const int divTop    = kAluH + kDivH + kPresetH + 14;
    const int divBot    = bounds.getBottom() - 8;
    g.setColour (juce::Colour (0xFF383840).withAlpha (0.6f));
    g.drawVerticalLine (leftEnd,   (float) divTop, (float) divBot);
    g.drawVerticalLine (centerEnd, (float) divTop, (float) divBot);

    // White silkscreen section labels (Beocord 2400 style)
    const int lblY = kAluH + kDivH + kPresetH + 2;
    auto drawSilk = [&] (const char* text, juce::Rectangle<int> rect)
    {
        g.setFont (LnF::sectionFont (9.0f));
        // Tiny black shadow under (sells "printed silkscreen on the panel")
        g.setColour (juce::Colours::black.withAlpha (0.85f));
        g.drawText (text, rect.translated (0, 1), juce::Justification::left);
        g.setColour (juce::Colour (0xFFE0E0E4));
        g.drawText (text, rect, juce::Justification::left);
    };
    drawSilk ("INPUT",       { kTeakW + 8, lblY, kLeftColW - 12, 12 });
    drawSilk ("FADERS",      { leftEnd + 8, lblY, 80, 12 });
    drawSilk ("TONE · TAPE", { centerEnd + 8, lblY, 130, 12 });

    // ===== Chrome corner-screws (4 corners of the black panel — Beocord 2400 detail) =====
    {
        const int yTop = kAluH + kDivH + 6;
        const int yBot = bounds.getBottom() - 10;
        const int xLeft  = kTeakW + 8;
        const int xRight = bounds.getRight() - kTeakW - 8;
        const float sR = 4.5f;
        for (auto p : { juce::Point<float> ((float) xLeft,  (float) yTop),
                        juce::Point<float> ((float) xRight, (float) yTop),
                        juce::Point<float> ((float) xLeft,  (float) yBot),
                        juce::Point<float> ((float) xRight, (float) yBot) })
        {
            // Recess
            g.setColour (juce::Colours::black.withAlpha (0.85f));
            g.fillEllipse (p.x - sR - 0.5f, p.y - sR - 0.5f, (sR + 0.5f) * 2, (sR + 0.5f) * 2);
            // Chrome dome
            juce::ColourGradient sg (
                juce::Colour (0xFFE8E8EA), p.x - sR * 0.45f, p.y - sR * 0.55f,
                juce::Colour (0xFF40404A), p.x + sR * 0.45f, p.y + sR * 0.55f, false);
            sg.addColour (0.55, juce::Colour (0xFFA8A8AE));
            g.setGradientFill (sg);
            g.fillEllipse (p.x - sR, p.y - sR, sR * 2, sR * 2);
            g.setColour (juce::Colour (0xFF080808));
            g.drawEllipse (p.x - sR, p.y - sR, sR * 2, sR * 2, 0.6f);
            // Phillips slot (cross at slight angle for variety)
            g.setColour (juce::Colours::black.withAlpha (0.75f));
            const float a = juce::MathConstants<float>::pi * 0.15f;
            const float c = std::cos (a), s = std::sin (a);
            g.drawLine (p.x - sR * 0.6f * c, p.y - sR * 0.6f * s,
                        p.x + sR * 0.6f * c, p.y + sR * 0.6f * s, 0.8f);
            g.drawLine (p.x - sR * 0.6f * (-s), p.y - sR * 0.6f * c,
                        p.x + sR * 0.6f * (-s), p.y + sR * 0.6f * c, 0.8f);
            // Tiny highlight
            g.setColour (juce::Colours::white.withAlpha (0.55f));
            g.fillEllipse (p.x - sR * 0.4f, p.y - sR * 0.55f, sR * 0.4f, sR * 0.18f);
        }
    }

    // ===== Soundboys brand medallion (engraved on the black metal deck) =====
    // Small chrome-rimmed circular badge, just under the title text on the left.
    {
        const float bx = (float) kTeakW + 138.0f;
        const float by = 6.0f;
        const float br = 13.0f;

        // Recess shadow
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.fillEllipse (bx - br - 1.0f, by - 1.0f, (br + 1.0f) * 2, (br + 1.0f) * 2);

        // Chrome bezel ring
        juce::ColourGradient bezel (
            juce::Colour (0xFFE8E8EC), bx, by - br * 0.5f,
            juce::Colour (0xFF50505A), bx, by + br * 0.7f, false);
        bezel.addColour (0.5, juce::Colour (0xFFA8A8AC));
        g.setGradientFill (bezel);
        g.fillEllipse (bx - br, by, br * 2, br * 2);

        // Inner deep medallion face
        const float ir = br - 2.5f;
        juce::ColourGradient face (
            juce::Colour (0xFF202024), bx, by + br - ir,
            juce::Colour (0xFF050507), bx, by + br + ir, false);
        g.setGradientFill (face);
        g.fillEllipse (bx - ir, by + br - ir + (br - ir) * 0.0f, ir * 2, ir * 2);

        // Engraved "S" mark + "SBYS" microtext
        g.setColour (juce::Colour (0xFF8A8A92));
        g.setFont (LnF::logoFont (12.0f));
        g.drawText ("S",
            juce::Rectangle<float> (bx - ir, by + br - ir - 1.0f, ir * 2, ir * 2)
                .toNearestInt(),
            juce::Justification::centred, false);
        // small ring text
        g.setColour (juce::Colour (0xFFB0B0B6).withAlpha (0.55f));
        g.setFont (LnF::sectionFont (5.5f));
        g.drawText ("SOUNDBOYS",
            juce::Rectangle<float> (bx - br + 1.0f, by + br + br * 0.3f, (br - 1) * 2, 7.0f)
                .toNearestInt(),
            juce::Justification::centred, false);
        // tiny highlight glint on bezel
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.fillEllipse (bx - br * 0.4f, by + br * 0.05f, br * 0.4f, br * 0.18f);
    }

    // ===== Window vignette (last layer — sells "professional product photo") =====
    {
        const auto bf = getLocalBounds().toFloat();
        // Radial dim from a point above the window (suggests overhead studio light)
        juce::ColourGradient v (
            juce::Colours::transparentBlack, bf.getCentreX(), bf.getCentreY() * 0.6f,
            juce::Colours::black.withAlpha (0.32f), bf.getX(), bf.getBottom(), true);
        g.setGradientFill (v);
        g.fillRect (bf);
    }

    // Status bar (preset row) — silver silkscreen on black
    if (statusText.isNotEmpty())
    {
        g.setColour (juce::Colour (0xFFB8B8BE));
        g.setFont (LnF::monoFont (9.0f));
        g.drawText (statusText,
                    juce::Rectangle<int> (centerEnd - 220, kAluH + kDivH + 4, 210, kPresetH - 8),
                    juce::Justification::centredRight, false);
    }
}

//=============================================================================
//  Layout
//=============================================================================
void NativeEditor::resized()
{
    const auto bounds = getLocalBounds();
    auto inner = bounds.withTrimmedLeft (kTeakW).withTrimmedRight (kTeakW);

    // ===== Top deck zone: reel deck + analog VU pair =====
    auto deckZone = inner.withHeight (kAluH).withTrimmedTop (20).reduced (4, 1);
    reelDeck.setBounds (deckZone);

    // 3 analog VU meters in a single horizontal row, with engraved headers
    // above (LEFT / RIGHT / OUTPUT) and counter below.
    {
        const int reelDiam = juce::jmin (deckZone.getHeight() - 6,
                                          (int) (deckZone.getWidth() * 0.36f));
        const int gapL = deckZone.getX() + reelDiam + 8;
        const int gapR = deckZone.getRight() - reelDiam - 8;
        const int gapW = juce::jmax (300, gapR - gapL);

        const int meterW = (gapW - 24) / 3;
        // Reserve 12 px above for engraved header and 14 px below for counter.
        const int meterH = juce::jmin (deckZone.getHeight() - 28, 96);
        const int meterY = deckZone.getY() + 13;

        vuInL.setBounds (gapL,                       meterY, meterW, meterH);
        vuInR.setBounds (gapL +  meterW + 12,        meterY, meterW, meterH);
        vuOut.setBounds (gapL + (meterW + 12) * 2,   meterY, meterW, meterH);
    }

    // ===== Black panel zone =====
    auto blackZone = inner.withY (kAluH + kDivH)
                         .withHeight (bounds.getHeight() - kAluH - kDivH);

    // --- Preset / nav bar (full width at top of black panel) ---
    auto presetBar = blackZone.removeFromTop (kPresetH).reduced (8, 6);

    btn_prev.setBounds (presetBar.removeFromLeft (24).reduced (1, 2));
    presetBar.removeFromLeft (4);
    btn_next.setBounds (presetBar.removeFromLeft (24).reduced (1, 2));
    presetBar.removeFromLeft (8);

    btn_about.setBounds (presetBar.removeFromRight (28).reduced (1, 2));
    presetBar.removeFromRight (4);
    btn_b.setBounds (presetBar.removeFromRight (28).reduced (1, 2));
    presetBar.removeFromRight (4);
    btn_a.setBounds (presetBar.removeFromRight (28).reduced (1, 2));
    presetBar.removeFromRight (12);

    cb_preset.setBounds (presetBar.reduced (0, 2));

    // Skip section-label row (very tight: 11 px)
    blackZone.removeFromTop (11);

    // --- Three columns ---
    auto leftCol   = blackZone.removeFromLeft (kLeftColW).reduced (8, 3);
    auto centerCol = blackZone.removeFromLeft (kCenterColW).reduced (8, 3);
    auto rightCol  = blackZone.reduced (8, 3);

    // =====================================================================
    // LEFT COLUMN: combos at top → toggles + transport pinned at BOTTOM
    //   (so all three columns end at the same baseline)
    // =====================================================================
    {
        // ---- Pin transport keys at BOTTOM ----
        const int kRowH = 30;  // bigger transport keys (was 20)
        const int kGap  = 3;
        auto transportRow2 = leftCol.removeFromBottom (kRowH);
        leftCol.removeFromBottom (kGap);
        auto transportRow1 = leftCol.removeFromBottom (kRowH);
        leftCol.removeFromBottom (5);

        // ---- Toggles (2 rows of 4) just above transport ----
        const int tRowH = 24;  // bigger toggles (was 17)
        auto togRow2 = leftCol.removeFromBottom (tRowH);
        leftCol.removeFromBottom (kGap);
        auto togRow1 = leftCol.removeFromBottom (tRowH);
        leftCol.removeFromBottom (6);

        // ---- 5 combo boxes at top, expanded to fill remaining space ----
        auto layoutCombo = [&] (juce::ComboBox& c, juce::Label& l)
        {
            l.setBounds (leftCol.removeFromTop (10));
            c.setBounds (leftCol.removeFromTop (20).reduced (0, 1));
            leftCol.removeFromTop (2);
        };
        layoutCombo (cb_speed,   lbl_speed);
        layoutCombo (cb_monitor, lbl_monitor);
        layoutCombo (cb_phono,   lbl_phono);
        layoutCombo (cb_radio,   lbl_radio);
        layoutCombo (cb_formula, lbl_formula);

        // ---- Lay out toggle rows ----
        const int tW = togRow1.getWidth() / 4;
        juce::ToggleButton* r1[] = { &t_echo, &t_bypass, &t_speaker, &t_sync };
        juce::ToggleButton* r2[] = { &t_loz,  &t_pa,     &t_sos,     &t_pause };
        for (auto* tb : r1) { tb->setBounds (togRow1.removeFromLeft (tW).reduced (1, 1)); }
        for (auto* tb : r2) { tb->setBounds (togRow2.removeFromLeft (tW).reduced (1, 1)); }

        // ---- Lay out transport keys ----
        {
            const int kW = transportRow1.getWidth() / 4;
            k_rec1.setBounds (transportRow1.removeFromLeft (kW).reduced (2, 1));
            k_rec2.setBounds (transportRow1.removeFromLeft (kW).reduced (2, 1));
            k_trk1.setBounds (transportRow1.removeFromLeft (kW).reduced (2, 1));
            k_trk2.setBounds (transportRow1.reduced (2, 1));
        }
        {
            const int kW = transportRow2.getWidth() / 3;
            k_spkA.setBounds (transportRow2.removeFromLeft (kW).reduced (2, 1));
            k_spkB.setBounds (transportRow2.removeFromLeft (kW).reduced (2, 1));
            k_mute.setBounds (transportRow2.reduced (2, 1));
        }
    }

    // =====================================================================
    // CENTER COLUMN: 5 dual slide faders (full height)
    // =====================================================================
    {
        const int dualW = centerCol.getWidth() / 5;
        auto layoutDual = [&] (juce::Rectangle<int> area, DualFader& f)
        {
            f.caption.setBounds (area.removeFromTop (14));
            area.removeFromBottom (2);
            const int sw = area.getWidth() / 2;
            f.l.setBounds (area.removeFromLeft (sw).reduced (5, 0));
            f.r.setBounds (area.reduced (5, 0));
        };
        layoutDual (centerCol.removeFromLeft (dualW), radio);
        layoutDual (centerCol.removeFromLeft (dualW), phono);
        layoutDual (centerCol.removeFromLeft (dualW), mic);
        layoutDual (centerCol.removeFromLeft (dualW), drive);
        layoutDual (centerCol.removeFromLeft (dualW), echo);
    }

    // =====================================================================
    // RIGHT COLUMN: 7 knobs in 2 rows that fill the column to the bottom,
    //   so the right column ends at the same baseline as left + center.
    // =====================================================================
    {
        const int rowW  = rightCol.getWidth();
        const int cell3 = rowW / 3;
        const int cell4 = rowW / 4;

        auto layoutKnob = [&] (juce::Slider& s, juce::Label& l,
                                juce::Rectangle<int>& row, int cellW)
        {
            auto cell = row.removeFromLeft (cellW);
            l.setBounds (cell.removeFromTop (12));
            s.setBounds (cell.reduced (3, 2));
        };

        // Split the column into 2 equal rows that fill the entire vertical
        const int totalH = rightCol.getHeight();
        const int gap = 4;
        const int row1H = (totalH - gap) / 2;
        auto row1 = rightCol.removeFromTop (row1H);
        rightCol.removeFromTop (gap);
        auto row2 = rightCol;        // remainder fills to bottom

        // Row 1: TREBLE / BASS / BAL
        layoutKnob (knob_treble,  lbl_treble,  row1, cell3);
        layoutKnob (knob_bass,    lbl_bass,    row1, cell3);
        layoutKnob (knob_balance, lbl_balance, row1, cell3);

        // Row 2: BIAS / WOW / MULT / VOL — pinned to bottom
        layoutKnob (knob_bias,   lbl_bias,   row2, cell4);
        layoutKnob (knob_wow,    lbl_wow,    row2, cell4);
        layoutKnob (knob_mult,   lbl_mult,   row2, cell4);
        layoutKnob (knob_master, lbl_master, row2, cell4);
    }
}

//=============================================================================
//  Timer
//=============================================================================
void NativeEditor::timerCallback()
{
    auto& chain = processor.getChain();
    vuInL.setLevel (chain.inputLevelL_dBFS.load());
    vuInR.setLevel (chain.inputLevelR_dBFS.load());
    // OUT meter: max of L/R post-processing (most useful for clip-detection)
    vuOut.setLevel (juce::jmax (chain.meterLevelL_dBFS.load(),
                                 chain.meterLevelR_dBFS.load()));

    // Reel spin when any input gain > threshold OR active output signal
    const float inputAny = processor.apvts.getRawParameterValue ("mic_gain")->load()
                         + processor.apvts.getRawParameterValue ("phono_gain")->load()
                         + processor.apvts.getRawParameterValue ("radio_gain")->load();
    const float outLvl = juce::jmax (chain.meterLevelL_dBFS.load(),
                                      chain.meterLevelR_dBFS.load());
    const bool tapeRunning = (inputAny > 0.05f) || outLvl > -40.0f;
    reelDeck.setActive (tapeRunning);

    // Record LED — blinks at 2 Hz when tape is running
    const bool ledTarget = tapeRunning &&
        (((juce::Time::getMillisecondCounter() / 250) & 1) != 0);
    if (ledTarget != recLedOn)
    {
        recLedOn = ledTarget;
        repaint (juce::Rectangle<int> (kTeakW, kAluH - 22, kInnerW, 22));
    }

    const int speedIdx = (int) processor.apvts.getRawParameterValue ("speed")->load();
    reelDeck.setSpeed (speedIdx);

    // Counter: ticks up while tape "runs" — speed-aware (faster speed = faster
    // playback time consumed). Display: 0000 = quarters of a second since start
    // (rolls at 9999 ≈ 41 minutes — same look as the real Beocord 4-digit drum).
    if (tapeRunning)
    {
        const double rate = (speedIdx == 0 ? 1.0 : speedIdx == 1 ? 2.0 : 4.0);  // 4.75/9.5/19
        counterSeconds += (1.0 / 30.0) * rate;
        const int counts = ((int) (counterSeconds * 4.0)) % 10000;
        juce::String c = juce::String (counts).paddedLeft ('0', 4);
        if (c != counterText)
        {
            counterText = c;
            // Repaint just the counter band (small region, near deck bottom)
            repaint (juce::Rectangle<int> (kTeakW, kAluH - 28, kInnerW, 28));
        }
    }

    const double sr = processor.getSampleRate();
    if (sr > 0.0)
    {
        const char* sp = (speedIdx == 0 ? "4.75" : speedIdx == 1 ? "9.5" : "19");
        juce::String s = juce::String (sp) + " cm/s · " + juce::String (sr / 1000.0, 1) + " kHz";
        if (s != statusText)
        {
            statusText = s;
            repaint (juce::Rectangle<int> (kTeakW, kAluH + kDivH, kInnerW, kPresetH));
        }
    }
}

//=============================================================================
//  Presets
//=============================================================================
void NativeEditor::applyPreset (int idx)
{
    // Tween parameter values to their new targets over ~250 ms instead of
    // jumping. Uses a one-shot AnimationCallback running at 60Hz.
    auto& v = processor.apvts;
    struct Tween { juce::String id; float fromN, toN; };
    auto targets = std::make_shared<std::vector<Tween>>();

    auto plan = [&] (const juce::String& id, float val)
    {
        if (auto* prm = v.getParameter (id))
        {
            const float toN = prm->convertTo0to1 (val);
            const float fromN = prm->getValue();
            targets->push_back ({ id, fromN, toN });
        }
    };
    auto set = [&] (const juce::String& id, float val) { plan (id, val); };
    auto setBool = [&] (const juce::String& id, bool b) { plan (id, b ? 1.0f : 0.0f); };

    // Baseline reset
    set ("speed", 2);
    set ("mic_gain", 0.5f);   set ("mic_gain_r",         0.5f);
    set ("phono_gain", 0);    set ("phono_gain_r",        0);
    set ("radio_gain", 0);    set ("radio_gain_r",        0);
    set ("saturation_drive", 1.0f); set ("saturation_drive_r", 1.0f);
    set ("echo_amount", 0);   set ("echo_amount_r",       0);
    set ("bias_amount", 1.0f);
    set ("wow_flutter", 0.3f);
    set ("multiplay_gen", 1);
    set ("bass_db", 0);    set ("treble_db", 0);
    set ("balance", 0);    set ("master_volume", 0.85f);
    setBool ("echo_enabled",    false);
    setBool ("bypass_tape",     false);
    setBool ("speaker_monitor", false);
    setBool ("synchroplay",     false);
    setBool ("pause",           false);
    setBool ("sos_enabled",     false);
    setBool ("pa_enabled",      false);
    setBool ("mic_loz",         false);

    switch (idx)
    {
        case  1: set ("tape_formula", 1); set ("speed", 1);
                  set ("saturation_drive", 1.6f); set ("saturation_drive_r", 1.6f);
                  set ("echo_amount", 0.25f); set ("echo_amount_r", 0.25f);
                  setBool ("echo_enabled", true); set ("bass_db", 2.5f); break;
        case  2: set ("tape_formula", 2);
                  set ("saturation_drive", 1.85f); set ("saturation_drive_r", 1.85f);
                  set ("bias_amount", 0.85f); break;
        case  3: set ("tape_formula", 2); set ("speed", 0);
                  set ("saturation_drive", 1.95f); set ("saturation_drive_r", 1.95f);
                  set ("bias_amount", 0.75f); break;
        case  4: set ("tape_formula", 0); set ("speed", 2);
                  set ("saturation_drive", 1.3f); set ("saturation_drive_r", 1.3f);
                  set ("treble_db", 1.0f); break;
        case  5: set ("tape_formula", 0); set ("saturation_drive", 1.4f); break;
        case  6: set ("tape_formula", 1); break;
        case  7: set ("tape_formula", 2); set ("saturation_drive", 1.5f); break;
        case  8: set ("speed", 2); set ("bias_amount", 1.0f); break;
        case  9: set ("speed", 1); set ("wow_flutter", 0.5f); break;
        case 10: set ("tape_formula", 1); set ("speed", 0);
                  set ("wow_flutter", 0.8f); set ("saturation_drive", 1.6f); break;
        case 11: set ("speed", 2); break;
        case 12: set ("tape_formula", 1); set ("speed", 2);
                  set ("saturation_drive", 1.45f); set ("bass_db", 1.5f); break;
        case 13: set ("treble_db", 0.8f); set ("bass_db", 1.2f); break;
        case 14: set ("tape_formula", 1); set ("speed", 1);
                  set ("echo_amount", 0.4f); setBool ("echo_enabled", true); break;
        case 15: set ("treble_db", -1.0f); set ("bass_db", 1.5f);
                  set ("saturation_drive", 1.3f); break;
        case 16: set ("saturation_drive", 1.95f); set ("saturation_drive_r", 1.95f); break;
        case 17: set ("saturation_drive", 1.15f); set ("treble_db", 1.5f); break;
        case 18: set ("bass_db", 4.0f); set ("saturation_drive", 1.4f); break;
        case 19: set ("tape_formula", 2); set ("speed", 0); set ("wow_flutter", 1.2f); break;
        case 20: set ("echo_amount", 0.7f); set ("echo_amount_r", 0.7f);
                  setBool ("echo_enabled", true); set ("bass_db", 3.0f); break;
        case 21: set ("saturation_drive", 1.25f); break;
        case 22: set ("echo_amount", 0.55f); setBool ("echo_enabled", true);
                  set ("treble_db", -0.5f); break;
        case 23: set ("wow_flutter", 1.8f); set ("multiplay_gen", 4);
                  set ("saturation_drive", 1.7f); break;
        default: break;
    }

    // ---- Schedule a smooth 250 ms tween (15 frames at 60Hz) ----
    constexpr int totalFrames = 15;
    auto& vRef = processor.apvts;
    for (int f = 1; f <= totalFrames; ++f)
    {
        const float t = (float) f / (float) totalFrames;
        const float e = t * t * (3.0f - 2.0f * t);   // smoothstep
        const bool last = (f == totalFrames);
        juce::Timer::callAfterDelay (16 * f,
            [&vRef, targets, e, last]
            {
                for (const auto& tw : *targets)
                    if (auto* prm = vRef.getParameter (tw.id))
                    {
                        const float val = last ? tw.toN
                                                : juce::jlimit (0.0f, 1.0f,
                                                    tw.fromN + (tw.toN - tw.fromN) * e);
                        prm->setValueNotifyingHost (val);
                    }
            });
    }
}
