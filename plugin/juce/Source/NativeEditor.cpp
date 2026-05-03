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
    constexpr int kEditorH    = 580;    // tight horizontal-strip proportions
    constexpr int kTeakW      = 36;     // wood end-cap width (each side)
    constexpr int kInnerW     = kEditorW - 2 * kTeakW;
    constexpr int kAluH       = 198;    // brushed-aluminium zone height
    constexpr int kDivH       = 5;      // metallic divider height
    constexpr int kPresetH    = 34;     // preset/nav bar height
    constexpr int kLeftColW   = 286;    // left column (VU + selectors + toggles)
    constexpr int kCenterColW = 490;    // center column (faders)
    // right column width = kInnerW - kLeftColW - kCenterColW = 320

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
        // Smooth attack/release ballistics (~300ms VU response)
        const auto target = juce::jlimit (-30.0f, 6.0f, dbfs);
        if (target > current) current += (target - current) * 0.35f;
        else                  current += (target - current) * 0.10f;

        const bool nowPeaking = current > 0.0f;
        if (nowPeaking != peaking) { peaking = nowPeaking; }
        repaint();
    }

    void AnalogVU::paint (juce::Graphics& g)
    {
        ui::InstructionCardLnF::drawAnalogVU (g, getLocalBounds(), current, peaking, channel);
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

    // Helper: dark-ink label on cream paper — never intercepts mouse.
    auto creamLbl = [&] (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setFont (LnF::sectionFont (9.0f));
        l.setColour (juce::Label::textColourId, LnF::ink());
        l.setJustificationType (juce::Justification::centred);
        l.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (l);
    };

    // ---- Top deck zone: reels + 4 analog VU meters (IN+OUT) ----
    addAndMakeVisible (reelDeck);
    addAndMakeVisible (vuInL);
    addAndMakeVisible (vuInR);
    addAndMakeVisible (vuOutL);
    addAndMakeVisible (vuOutR);

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
        f.caption.setColour (juce::Label::textColourId, LnF::ink());
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
        l.setColour (juce::Label::textColourId, LnF::ink().withAlpha (0.75f));
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
    LnF::drawTitle (g, aluZone.reduced (16, 6).removeFromTop (24),
                     "BEOLUX 2000", "SOUNDBOYS · DANISH TAPE EMULATION · v34.0");

    // Counter (bottom-centre of deck, just below the VU pair)
    {
        constexpr int cW = 76, cH = 18;
        LnF::drawCounter (g,
            juce::Rectangle<int> (inner.getCentreX() - cW / 2,
                                  aluZone.getBottom() - cH - 4,
                                  cW, cH),
            counterText);
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

    // Column dividers — thin black ink lines on cream paper
    const int leftEnd   = kTeakW + kLeftColW;
    const int centerEnd = leftEnd + kCenterColW;
    const int divTop    = kAluH + kDivH + kPresetH + 14;
    const int divBot    = bounds.getBottom() - 8;
    g.setColour (LnF::ink().withAlpha (0.45f));
    g.drawVerticalLine (leftEnd,   (float) divTop, (float) divBot);
    g.drawVerticalLine (centerEnd, (float) divTop, (float) divBot);

    // Section labels in dark ink
    const int lblY = kAluH + kDivH + kPresetH + 4;
    g.setFont (LnF::sectionFont (8.5f));
    g.setColour (LnF::ink().withAlpha (0.75f));
    g.drawText ("INPUT",    juce::Rectangle<int> (kTeakW + 8, lblY, kLeftColW - 12, 12), juce::Justification::left);
    g.drawText ("FADERS",   juce::Rectangle<int> (leftEnd + 8, lblY, 80, 12), juce::Justification::left);
    g.drawText ("TONE · TAPE", juce::Rectangle<int> (centerEnd + 8, lblY, 130, 12), juce::Justification::left);

    // Status bar (preset row) — dark ink on cream
    if (statusText.isNotEmpty())
    {
        g.setColour (LnF::ink().withAlpha (0.65f));
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
    auto deckZone = inner.withHeight (kAluH).withTrimmedTop (26).reduced (6, 3);
    reelDeck.setBounds (deckZone);

    // 4 analog VU meters in centre gap: row1 = IN L/R, row2 = OUT L/R.
    // Counter sits centred under the lower row.
    {
        const int reelDiam = juce::jmin (deckZone.getHeight() - 6,
                                          (int) (deckZone.getWidth() * 0.36f));
        const int gapL = deckZone.getX() + reelDiam + 8;
        const int gapR = deckZone.getRight() - reelDiam - 8;
        const int gapW = juce::jmax (220, gapR - gapL);

        const int colW = (gapW - 12) / 2;     // 2 columns (L, R)
        const int rowH = juce::jmin (74, (deckZone.getHeight() - 28) / 2);
        const int totalH = rowH * 2 + 6;
        const int topY = deckZone.getY() + (deckZone.getHeight() - totalH - 22) / 2;

        // Row 1: IN L | IN R
        vuInL .setBounds (gapL,                     topY,            colW, rowH);
        vuInR .setBounds (gapL + colW + 12,         topY,            colW, rowH);
        // Row 2: OUT L | OUT R
        vuOutL.setBounds (gapL,                     topY + rowH + 6, colW, rowH);
        vuOutR.setBounds (gapL + colW + 12,         topY + rowH + 6, colW, rowH);
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

    // Skip section-label row
    blackZone.removeFromTop (20);

    // --- Three columns ---
    auto leftCol   = blackZone.removeFromLeft (kLeftColW).reduced (8, 6);
    auto centerCol = blackZone.removeFromLeft (kCenterColW).reduced (8, 6);
    auto rightCol  = blackZone.reduced (8, 6);

    // =====================================================================
    // LEFT COLUMN: combos → toggles → transport keys
    //   (VU meters now live on the top black-metal deck — see deckZone above)
    // =====================================================================
    {
        // 5 combo boxes (compact)
        auto layoutCombo = [&] (juce::ComboBox& c, juce::Label& l)
        {
            l.setBounds (leftCol.removeFromTop (10));
            c.setBounds (leftCol.removeFromTop (18).reduced (0, 1));
            leftCol.removeFromTop (2);
        };
        layoutCombo (cb_speed,   lbl_speed);
        layoutCombo (cb_monitor, lbl_monitor);
        layoutCombo (cb_phono,   lbl_phono);
        layoutCombo (cb_radio,   lbl_radio);
        layoutCombo (cb_formula, lbl_formula);

        leftCol.removeFromTop (4);

        // 8 toggles in 2 rows of 4 (slim)
        const int tW = leftCol.getWidth() / 4;
        auto togRow1 = leftCol.removeFromTop (18);
        auto togRow2 = leftCol.removeFromTop (18);
        leftCol.removeFromTop (3);
        juce::ToggleButton* r1[] = { &t_echo, &t_bypass, &t_speaker, &t_sync };
        juce::ToggleButton* r2[] = { &t_loz,  &t_pa,     &t_sos,     &t_pause };
        for (auto* tb : r1) { tb->setBounds (togRow1.removeFromLeft (tW).reduced (1, 1)); }
        for (auto* tb : r2) { tb->setBounds (togRow2.removeFromLeft (tW).reduced (1, 1)); }

        leftCol.removeFromTop (3);

        // Transport: 4 keys row 1
        {
            auto kRow = leftCol.removeFromTop (22);
            const int kW = kRow.getWidth() / 4;
            k_rec1.setBounds (kRow.removeFromLeft (kW).reduced (2, 1));
            k_rec2.setBounds (kRow.removeFromLeft (kW).reduced (2, 1));
            k_trk1.setBounds (kRow.removeFromLeft (kW).reduced (2, 1));
            k_trk2.setBounds (kRow.reduced (2, 1));
        }
        leftCol.removeFromTop (3);
        // Transport: 3 keys row 2
        {
            auto kRow = leftCol.removeFromTop (22);
            const int kW = kRow.getWidth() / 3;
            k_spkA.setBounds (kRow.removeFromLeft (kW).reduced (2, 1));
            k_spkB.setBounds (kRow.removeFromLeft (kW).reduced (2, 1));
            k_mute.setBounds (kRow.reduced (2, 1));
        }
    }

    // =====================================================================
    // CENTER COLUMN: 5 dual slide faders (full height)
    // =====================================================================
    {
        const int dualW = centerCol.getWidth() / 5;
        auto layoutDual = [&] (juce::Rectangle<int> area, DualFader& f)
        {
            f.caption.setBounds (area.removeFromTop (18));
            area.removeFromBottom (4);
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
    // RIGHT COLUMN: 7 knobs in 2 rows (compressed)
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

        // Row 1: TREBLE / BASS / BAL (slightly bigger, the "tone" trio)
        const int totalH = rightCol.getHeight();
        const int row1H = juce::jlimit (70, 100, (int) (totalH * 0.50f));
        auto row1 = rightCol.removeFromTop (row1H);
        layoutKnob (knob_treble,  lbl_treble,  row1, cell3);
        layoutKnob (knob_bass,    lbl_bass,    row1, cell3);
        layoutKnob (knob_balance, lbl_balance, row1, cell3);

        rightCol.removeFromTop (4);

        // Row 2: BIAS / WOW / MULT / VOL
        const int row2H = juce::jmin (rightCol.getHeight(), juce::jlimit (60, 88, (int) (totalH * 0.42f)));
        auto row2 = rightCol.removeFromTop (row2H);
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
    vuInL .setLevel (chain.inputLevelL_dBFS .load());
    vuInR .setLevel (chain.inputLevelR_dBFS .load());
    vuOutL.setLevel (chain.meterLevelL_dBFS.load());
    vuOutR.setLevel (chain.meterLevelR_dBFS.load());

    // Reel spin when any input gain > threshold
    const float inputAny = processor.apvts.getRawParameterValue ("mic_gain")->load()
                         + processor.apvts.getRawParameterValue ("phono_gain")->load()
                         + processor.apvts.getRawParameterValue ("radio_gain")->load();
    reelDeck.setActive (inputAny > 0.05f);

    const int speedIdx = (int) processor.apvts.getRawParameterValue ("speed")->load();
    reelDeck.setSpeed (speedIdx);

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
    auto& v = processor.apvts;
    auto set = [&] (const juce::String& id, float val)
    {
        if (auto* prm = v.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (val));
    };
    auto setBool = [&] (const juce::String& id, bool b) { set (id, b ? 1.0f : 0.0f); };

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
}
