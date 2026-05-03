/*  NativeEditor implementation — instruction-card layout.

    Layout (1180×760, paper-card aspect ratio matching the eBay reference):
      ┌─ wood end-cap ─────────────── PAPER CARD ─────────────── wood end-cap ─┐
      │                                                                         │
      │  TITLE (top-left)              [Preset · A/B · About] (top-right)      │
      │                                                                         │
      │  ┌─ DECK ZONE ──────────────────────────────────────────────────────┐  │
      │  │   ●●●  REEL L   [head assembly + counter]   REEL R  ●●●          │  │
      │  │   ↑ "Tape direction"           ↑ "Heads"          ↑ "Take-up"     │  │
      │  └─────────────────────────────────────────────────────────────────┘  │
      │                                                                         │
      │  ┌─ INPUT FADERS ──┐  ┌─ TONE & METER ────────────────────────────┐  │
      │  │ ║ ║  ║ ║  ║ ║  │  │ VU L ─────────────                          │  │
      │  │ RADIO PHONO MIC │  │ VU R ─────────────                          │  │
      │  └─────────────────┘  │ ◐BIAS ◐BASS ◐TREBLE ◐WOW ◐MULT ◐BAL ◐MAST │  │
      │                       └────────────────────────────────────────────┘  │
      │  ┌─ DRIVE ECHO ────┐  ┌─ MODE SELECTORS ─────────────────────────┐  │
      │  │ ║ ║  ║ ║         │  │ SPEED MONITOR PHONO RADIO FORMULA          │  │
      │  └─────────────────┘  └────────────────────────────────────────────┘  │
      │                                                                         │
      │  ┌─ TRANSPORT (piano keys) ────────────────────────────────────────┐  │
      │  │ [REC1][REC2][TRK1][TRK2]    [SPKa][SPKb][MUTE]   [ECHO][BYP][SP][SY]│  │
      │  │                                                  [LO-Z][P.A][SOS][PA]│  │
      │  └─────────────────────────────────────────────────────────────────┘  │
      └────────────────────────────────────────────────────────────────────────┘
*/

#include "NativeEditor.h"

namespace
{
    using LnF = bc2000dl::ui::InstructionCardLnF;

    constexpr int kEditorW   = 1180;
    constexpr int kEditorH   = 760;
    constexpr int kWoodCap   = 36;
    constexpr int kPadding   = 16;

    juce::String tooltipFor (const juce::String& id)
    {
        if (id == "speed")              return "Bandhastighet (4.75 / 9.5 / 19 cm/s)";
        if (id == "monitor_mode")       return "Source = pre-tape · Tape = post-tape";
        if (id == "phono_mode")         return "L = ceramic · H = magnetic (RIAA)";
        if (id == "radio_mode")         return "L = 3 mV · H = 100 mV linenivå";
        if (id == "tape_formula")       return "Agfa / BASF / Scotch tape-emulering";
        if (id == "saturation_drive")   return "Tape-saturation drive";
        if (id == "echo_amount")        return "Echo-mängd (high = self-oscillation)";
        if (id == "bias_amount")        return "Bias-ström (under nominal = mer 3rd harm)";
        if (id == "wow_flutter")        return "Wow & flutter-amplitud";
        if (id == "multiplay_gen")      return "Multiplay-generation 1-5";
        if (id == "master_volume")      return "Master output";
        if (id == "balance")            return "L/R-balans";
        if (id == "bass_db")            return "Bass (Baxandall, post-playback)";
        if (id == "treble_db")          return "Treble (Baxandall, post-playback)";
        if (id == "echo_enabled")       return "Aktivera echo-loop";
        if (id == "bypass_tape")        return "Bypass tape — ren input + tone";
        if (id == "speaker_monitor")    return "AD139 power-amp + soft-clip";
        if (id == "synchroplay")        return "Record-headet som playback";
        if (id == "mic_loz")            return "Lo-Z mic via ingångstrafo";
        if (id == "pa_enabled")         return "P.A.: mic duckar phono+radio";
        if (id == "sos_enabled")        return "Sound-on-Sound";
        if (id == "pause")              return "Tysta output";
        if (id == "rec_arm_1")          return "Arma spår 1";
        if (id == "rec_arm_2")          return "Arma spår 2";
        if (id == "track_1")            return "Lyssna spår 1";
        if (id == "track_2")            return "Lyssna spår 2";
        if (id == "speaker_ext")        return "Extern högtalare A";
        if (id == "speaker_int")        return "Intern högtalare B";
        if (id == "speaker_mute")       return "Mute alla högtalare";
        return {};
    }
}

//=============================================================================
//  ReelDeck — animated reels (two thin-line circles)
//=============================================================================
namespace bc2000dl
{
    ReelDeck::ReelDeck() { startTimerHz (30); }
    ReelDeck::~ReelDeck() = default;

    void ReelDeck::timerCallback()
    {
        if (isActive)
        {
            const float delta = 0.10f * speedFactor;
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
        const int reelSize = juce::jmin (bounds.getWidth() / 3 - 8, bounds.getHeight() - 8);
        const int gap = 60;

        // Center the pair
        const int totalW = reelSize * 2 + gap;
        const int startX = bounds.getCentreX() - totalW / 2;
        const int reelY = bounds.getCentreY() - reelSize / 2;

        // Left reel
        const juce::Rectangle<int> leftReel (startX, reelY, reelSize, reelSize);
        ui::InstructionCardLnF::drawReel (g, leftReel, angleL, isActive);

        // Head assembly between reels
        const juce::Rectangle<int> heads (
            startX + reelSize + 4, reelY + reelSize / 3,
            gap - 8, reelSize / 3);
        ui::InstructionCardLnF::drawHeadAssembly (g, heads);

        // Right reel
        const juce::Rectangle<int> rightReel (startX + reelSize + gap, reelY, reelSize, reelSize);
        ui::InstructionCardLnF::drawReel (g, rightReel, -angleR, isActive); // counter-rotation

        // Subtle "tape path" line connecting reels via heads
        g.setColour (ui::InstructionCardLnF::ink().withAlpha (0.35f));
        const float tapeY = (float) (reelY + reelSize / 2);
        g.drawLine ((float) leftReel.getRight() - reelSize * 0.05f, tapeY,
                    (float) heads.getX(), tapeY, 0.8f);
        g.drawLine ((float) heads.getRight(), tapeY,
                    (float) rightReel.getX() + reelSize * 0.05f, tapeY, 0.8f);
    }

    //=========================================================================
    //  VU bar
    //=========================================================================
    void VUBar::setLevel (float dbfs)
    {
        const auto target = juce::jlimit (-60.0f, 6.0f, dbfs);
        if (target > current)
            current = target;                              // attack: instant
        else
            current = juce::jmax (target, current - 0.6f); // release: ~18 dB/s @ 30 Hz
        repaint();
    }

    void VUBar::paint (juce::Graphics& g)
    {
        const auto r = getLocalBounds().toFloat();

        // Recessed window
        g.setColour (ui::InstructionCardLnF::ink());
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (ui::InstructionCardLnF::ink().darker (0.3f));
        g.drawRoundedRectangle (r, 3.0f, 0.8f);

        // Bar fill
        const float pos = juce::jlimit (0.0f, 1.0f, (current + 60.0f) / 66.0f);
        if (pos > 0.005f)
        {
            auto bar = r.reduced (2.5f);
            bar.setWidth (bar.getWidth() * pos);

            juce::ColourGradient grad (
                juce::Colour (0xff3ad07a), bar.getX(),     bar.getCentreY(),
                juce::Colour (0xffd03a3a), r.getRight(),   bar.getCentreY(), false);
            grad.addColour (0.65, juce::Colour (0xffd0c43a));
            grad.addColour (0.85, juce::Colour (0xffd07a3a));
            g.setGradientFill (grad);
            g.fillRoundedRectangle (bar, 1.5f);
        }

        // Tick marks at -40, -20, -10, -3, 0
        g.setColour (ui::InstructionCardLnF::amber().withAlpha (0.45f));
        for (int db : { -40, -20, -10, -3, 0 })
        {
            const float x = r.getX() + r.getWidth() * (db + 60.0f) / 66.0f;
            g.drawLine (x, r.getY() + 1, x, r.getBottom() - 1, 0.5f);
        }

        // dB readout
        g.setColour (ui::InstructionCardLnF::amberHot());
        g.setFont (ui::InstructionCardLnF::monoFont (9.5f));
        g.drawText (juce::String (current, 1) + " dBFS",
                    r.reduced (6, 0).toNearestInt(),
                    juce::Justification::centredRight, false);

        // REC blink
        if (recording)
        {
            g.setColour (ui::InstructionCardLnF::redAccent());
            g.fillEllipse (r.getRight() - 14, r.getY() + 4, 5, 5);
        }
    }
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

    // ---- Top deck zone ----
    addAndMakeVisible (reelDeck);
    addAndMakeVisible (vuL);
    addAndMakeVisible (vuR);
    auto styleVuLbl = [&] (juce::Label& l, const juce::String& t)
    {
        l.setText (t, juce::dontSendNotification);
        l.setFont (LnF::sectionFont (10.0f));
        l.setColour (juce::Label::textColourId, LnF::redAccent());
        l.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (l);
    };
    styleVuLbl (vuL_lbl, "L");
    styleVuLbl (vuR_lbl, "R");

    // ---- 5 dual-faders ----
    auto setupDual = [&] (DualFader& f, const juce::String& cap,
                           const juce::String& idL, const juce::String& idR)
    {
        for (auto* sl : { &f.l, &f.r })
        {
            sl->setSliderStyle (juce::Slider::LinearVertical);
            sl->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            sl->setDoubleClickReturnValue (true, 0.0);
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
        addAndMakeVisible (f.caption);
    };
    setupDual (radio, "RADIO", "radio_gain",       "radio_gain_r");
    setupDual (phono, "PHONO", "phono_gain",       "phono_gain_r");
    setupDual (mic,   "MIC",   "mic_gain",         "mic_gain_r");
    setupDual (drive, "DRIVE", "saturation_drive", "saturation_drive_r");
    setupDual (echo,  "ECHO",  "echo_amount",      "echo_amount_r");

    // ---- 7 knobs ----
    auto setupKnob = [&] (juce::Slider& s, juce::Label& l, const juce::String& id,
                           const juce::String& cap)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                juce::MathConstants<float>::pi * 2.8f, true);
        s.setTooltip (tooltipFor (id));
        addAndMakeVisible (s);
        sAtts.push_back (std::make_unique<SAtt> (processor.apvts, id, s));

        l.setText (cap, juce::dontSendNotification);
        l.setFont (LnF::sectionFont (8.5f));
        l.setColour (juce::Label::textColourId, LnF::ink());
        l.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (l);
    };
    setupKnob (knob_bias,    lbl_bias,    "bias_amount",    "BIAS");
    setupKnob (knob_bass,    lbl_bass,    "bass_db",        "BASS");
    setupKnob (knob_treble,  lbl_treble,  "treble_db",      "TREBLE");
    setupKnob (knob_wow,     lbl_wow,     "wow_flutter",    "WOW");
    setupKnob (knob_mult,    lbl_mult,    "multiplay_gen",  "MULT");
    setupKnob (knob_balance, lbl_balance, "balance",        "BAL");
    setupKnob (knob_master,  lbl_master,  "master_volume",  "MASTER");

    // ---- 5 selectors ----
    auto setupCombo = [&] (juce::ComboBox& c, juce::Label& l, const juce::String& id,
                            const juce::String& cap, std::initializer_list<const char*> items)
    {
        int idx = 1;
        for (auto* it : items) c.addItem (it, idx++);
        c.setTooltip (tooltipFor (id));
        addAndMakeVisible (c);
        cAtts.push_back (std::make_unique<CAtt> (processor.apvts, id, c));

        l.setText (cap, juce::dontSendNotification);
        l.setFont (LnF::sectionFont (8.5f));
        l.setColour (juce::Label::textColourId, LnF::ink());
        l.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (l);
    };
    setupCombo (cb_speed,   lbl_speed,   "speed",        "TAPE SPEED",  { "4.75", "9.5", "19 cm/s" });
    setupCombo (cb_monitor, lbl_monitor, "monitor_mode", "MONITOR",     { "Source", "Tape" });
    setupCombo (cb_phono,   lbl_phono,   "phono_mode",   "PHONO IN",    { "L (ceramic)", "H (magnetic)" });
    setupCombo (cb_radio,   lbl_radio,   "radio_mode",   "RADIO IN",    { "L (3 mV)", "H (100 mV)" });
    setupCombo (cb_formula, lbl_formula, "tape_formula", "TAPE FORMULA",{ "Agfa", "BASF", "Scotch" });

    // ---- Toggle buttons (small square checkboxes with red fill when ON) ----
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

    // ---- Transport keys (TextButton with toggle, styled by L&F) ----
    auto setupKey = [&] (juce::TextButton& b, const juce::String& cap, const juce::String& id)
    {
        b.setButtonText (cap);
        b.setClickingTogglesState (true);
        b.setTooltip (tooltipFor (id));
        if (cap.startsWith ("REC")) b.setName ("REC"); // L&F uses this for red strip
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

    // ---- Header ----
    int pid = 1;
    for (auto* name : { "FACTORY", "BERG · DEPECHE PAD", "BERG · DELTA SYNTH", "BERG · PROTON CRUNCH",
                         "BERG · REEL VOCALS", "FERRO+", "CHROME", "METAL IV",
                         "BBC J", "NAGRA", "CASSETTE 70s", "BROADCAST",
                         "VISCONTI '74", "BUTLER MIX", "KRAFTWERK '78", "RHODES WARM",
                         "DRUM SMASH", "VOCAL SILK", "BASS WEIGHT", "LO-FI POP",
                         "DUB ECHO", "GLUE MASTER", "SHOEGAZE", "CRT NIGHTMARE" })
        cb_preset.addItem (name, pid++);
    cb_preset.setText ("FACTORY", juce::dontSendNotification);
    cb_preset.onChange = [this]
    {
        const int sel = cb_preset.getSelectedId() - 1;
        if (sel >= 0) applyPreset (sel);
    };
    addAndMakeVisible (cb_preset);

    auto setupNav = [&] (juce::TextButton& b, int delta)
    {
        addAndMakeVisible (b);
        b.onClick = [this, delta]
        {
            const int n = cb_preset.getNumItems();
            int curr = cb_preset.getSelectedId();
            if (curr < 1) curr = 1;
            int next = ((curr - 1 + delta + n) % n) + 1;
            cb_preset.setSelectedId (next);
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
            store = processor.apvts.copyState();
            slotIsA = isA;
            auto& load = slotIsA ? stateA : stateB;
            if (load.isValid())
                processor.apvts.replaceState (load);
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
                .withTitle ("BC2000DL · v29.8 NATIVE")
                .withMessage ("Bang & Olufsen Beocord 2000 De Luxe (1968-69)\n\n"
                              "DSP: Jiles-Atherton hysteresis, 8x oversampling,\n"
                              "21/21 PASS vs Studio-Sound + Service Manual.\n\n"
                              "UI: native JUCE — instruction-card aesthetic\n"
                              "(inspired by the operating-instructions inlay).")
                .withButton ("OK"),
            nullptr);
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
//  Paint — paper card + wood end-caps + section frames + decorative labels
//=============================================================================
void NativeEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds();

    // Wood end-caps
    LnF::drawWoodEndCap (g, bounds.withWidth (kWoodCap), true);
    LnF::drawWoodEndCap (g, bounds.withTrimmedLeft (bounds.getWidth() - kWoodCap), false);

    // Paper panel between
    auto card = bounds.reduced (kWoodCap, 0);
    LnF::drawPaperPanel (g, card);

    // Title (top-left)
    auto header = card.reduced (kPadding, kPadding).removeFromTop (60);
    LnF::drawTitle (g, header.removeFromLeft (380),
                     "BC2000DL", "DANISH TAPE 2000 · v29.8 NATIVE");

    // Status (header right side, just text — actual controls drawn elsewhere)
    g.setColour (LnF::inkSoft());
    g.setFont (LnF::labelFont (10.0f));
    g.drawText (statusText, bounds.getRight() - 320, 28, 180, 16,
                juce::Justification::right, false);

    // Re-derive section rectangles to draw frames
    auto card_inner = card.reduced (kPadding, kPadding);
    card_inner.removeFromTop (60); // skip header

    // Deck zone (reels + heads + counter + VU)
    const int deckH = 200;
    auto deck = card_inner.removeFromTop (deckH);
    LnF::drawSectionBox (g, deck.reduced (2, 2), "DECK");

    card_inner.removeFromTop (8);

    // Mid section: faders + (tone+knobs)
    const int midH = 240;
    auto midRow = card_inner.removeFromTop (midH);

    // Faders: 5 dual-faders take the full width's 60%
    auto fadersBox = midRow.removeFromLeft ((int) (midRow.getWidth() * 0.58f));
    LnF::drawSectionBox (g, fadersBox.reduced (2, 2), "INPUT · BUS");

    midRow.removeFromLeft (8);
    auto rightBox = midRow;
    LnF::drawSectionBox (g, rightBox.reduced (2, 2), "TONE · METER");

    card_inner.removeFromTop (8);

    // Selectors
    const int selH = 56;
    auto selBox = card_inner.removeFromTop (selH);
    LnF::drawSectionBox (g, selBox.reduced (2, 2), "MODE");

    card_inner.removeFromTop (8);

    // Transport row
    auto transBox = card_inner.removeFromTop (80);
    LnF::drawSectionBox (g, transBox.reduced (2, 2), "TRANSPORT");

    // Footer hairline
    g.setColour (LnF::inkSoft().withAlpha (0.3f));
    g.drawHorizontalLine (bounds.getBottom() - 16, (float) kWoodCap + 8.0f,
                          (float) bounds.getRight() - kWoodCap - 8.0f);
}

//=============================================================================
//  Layout
//=============================================================================
void NativeEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromLeft (kWoodCap);
    bounds.removeFromRight (kWoodCap);

    auto card_inner = bounds.reduced (kPadding, kPadding);

    // ===== Header =====
    auto header = card_inner.removeFromTop (60);
    header.removeFromLeft (380); // title-zon (reserved by paint())

    btn_about.setBounds (header.removeFromRight (32).reduced (2));
    header.removeFromRight (8);
    btn_b.setBounds (header.removeFromRight (32).reduced (2));
    header.removeFromRight (4);
    btn_a.setBounds (header.removeFromRight (32).reduced (2));
    header.removeFromRight (12);

    auto presetW = juce::jmin (320, header.getWidth());
    auto presetArea = header.removeFromRight (presetW);
    btn_prev.setBounds (presetArea.removeFromLeft (24).reduced (1, 4));
    presetArea.removeFromLeft (4);
    btn_next.setBounds (presetArea.removeFromRight (24).reduced (1, 4));
    presetArea.removeFromRight (4);
    cb_preset.setBounds (presetArea.reduced (0, 4));

    // ===== Deck zone (200 px) =====
    const int deckH = 200;
    auto deck = card_inner.removeFromTop (deckH).reduced (12, 16);
    // Reels take left 2/3, VU+counter takes right 1/3
    auto reelArea = deck.removeFromLeft ((int) (deck.getWidth() * 0.62f));
    reelDeck.setBounds (reelArea);

    deck.removeFromLeft (12);
    // VU stack on right
    auto vuStack = deck;
    auto vuLArea = vuStack.removeFromTop (vuStack.getHeight() / 2).reduced (4);
    vuL_lbl.setBounds (vuLArea.removeFromLeft (16));
    vuL.setBounds (vuLArea);

    auto vuRArea = vuStack.reduced (4);
    vuR_lbl.setBounds (vuRArea.removeFromLeft (16));
    vuR.setBounds (vuRArea);

    card_inner.removeFromTop (8);

    // ===== Mid row (240 px): faders | tone+knobs =====
    const int midH = 240;
    auto midRow = card_inner.removeFromTop (midH);

    auto fadersBox = midRow.removeFromLeft ((int) (midRow.getWidth() * 0.58f));
    auto rightBox = midRow;
    rightBox.removeFromLeft (8);

    // Layout faders: 5 dual-faders, each is 2 vertical sliders + caption
    fadersBox.reduce (12, 16);
    const int dualW = fadersBox.getWidth() / 5;
    auto layoutDual = [&] (juce::Rectangle<int> area, DualFader& f)
    {
        f.caption.setBounds (area.removeFromTop (16));
        area.removeFromBottom (4);
        const int sw = area.getWidth() / 2;
        f.l.setBounds (area.removeFromLeft (sw).reduced (4, 0));
        f.r.setBounds (area.reduced (4, 0));
    };
    layoutDual (fadersBox.removeFromLeft (dualW), radio);
    layoutDual (fadersBox.removeFromLeft (dualW), phono);
    layoutDual (fadersBox.removeFromLeft (dualW), mic);
    layoutDual (fadersBox.removeFromLeft (dualW), drive);
    layoutDual (fadersBox.removeFromLeft (dualW), echo);

    // Layout right box: 7 knobs in a single row
    rightBox.reduce (12, 16);
    const int knobW = rightBox.getWidth() / 7;
    auto layoutKnob = [&] (juce::Rectangle<int> area, juce::Slider& s, juce::Label& l)
    {
        l.setBounds (area.removeFromTop (12));
        s.setBounds (area.reduced (6, 4));
    };
    layoutKnob (rightBox.removeFromLeft (knobW), knob_bias,    lbl_bias);
    layoutKnob (rightBox.removeFromLeft (knobW), knob_bass,    lbl_bass);
    layoutKnob (rightBox.removeFromLeft (knobW), knob_treble,  lbl_treble);
    layoutKnob (rightBox.removeFromLeft (knobW), knob_wow,     lbl_wow);
    layoutKnob (rightBox.removeFromLeft (knobW), knob_mult,    lbl_mult);
    layoutKnob (rightBox.removeFromLeft (knobW), knob_balance, lbl_balance);
    layoutKnob (rightBox.removeFromLeft (knobW), knob_master,  lbl_master);

    card_inner.removeFromTop (8);

    // ===== Selectors row (56 px) =====
    auto selBox = card_inner.removeFromTop (56).reduced (12, 8);
    const int cellW = selBox.getWidth() / 5;
    auto layoutCombo = [&] (juce::Rectangle<int> area, juce::ComboBox& c, juce::Label& l)
    {
        l.setBounds (area.removeFromTop (12));
        c.setBounds (area.reduced (2, 2));
    };
    layoutCombo (selBox.removeFromLeft (cellW), cb_speed,   lbl_speed);
    layoutCombo (selBox.removeFromLeft (cellW), cb_monitor, lbl_monitor);
    layoutCombo (selBox.removeFromLeft (cellW), cb_phono,   lbl_phono);
    layoutCombo (selBox.removeFromLeft (cellW), cb_radio,   lbl_radio);
    layoutCombo (selBox.removeFromLeft (cellW), cb_formula, lbl_formula);

    card_inner.removeFromTop (8);

    // ===== Transport (80 px) =====
    auto transBox = card_inner.removeFromTop (80).reduced (12, 8);
    auto trKeyRow = transBox.removeFromTop (32);
    transBox.removeFromTop (4);
    auto toggleRow = transBox.removeFromTop (32);

    // Transport keys: 7 piano-key-style buttons evenly distributed
    auto layoutKeyRow = [&] (juce::Rectangle<int> row,
                              std::initializer_list<juce::TextButton*> btns)
    {
        const int n = (int) btns.size();
        const int gap = 4;
        const int btnW = (row.getWidth() - gap * (n - 1)) / n;
        for (auto* b : btns)
        {
            b->setBounds (row.removeFromLeft (btnW));
            row.removeFromLeft (gap);
        }
    };
    layoutKeyRow (trKeyRow, { &k_rec1, &k_rec2, &k_trk1, &k_trk2,
                              &k_spkA, &k_spkB, &k_mute });

    // Toggle row: 8 small-checkbox-with-label
    const int n = 8;
    const int gap = 4;
    const int tW = (toggleRow.getWidth() - gap * (n - 1)) / n;
    juce::ToggleButton* toggles[] = { &t_echo, &t_bypass, &t_speaker, &t_sync,
                                       &t_loz, &t_pa, &t_sos, &t_pause };
    for (auto* tb : toggles)
    {
        tb->setBounds (toggleRow.removeFromLeft (tW));
        toggleRow.removeFromLeft (gap);
    }
}

//=============================================================================
//  Timer
//=============================================================================
void NativeEditor::timerCallback()
{
    auto& chain = processor.getChain();
    vuL.setLevel (chain.meterLevelL_dBFS.load());
    vuR.setLevel (chain.meterLevelR_dBFS.load());
    vuL.setRecording (chain.isRecordingL.load());
    vuR.setRecording (chain.isRecordingR.load());

    // Reels turn when there's signal moving through
    const float inputAny = processor.apvts.getRawParameterValue ("mic_gain")->load()
                         + processor.apvts.getRawParameterValue ("phono_gain")->load()
                         + processor.apvts.getRawParameterValue ("radio_gain")->load();
    reelDeck.setActive (inputAny > 0.05f);
    const int speedIdx = (int) processor.apvts.getRawParameterValue ("speed")->load();
    reelDeck.setSpeed (speedIdx);

    const auto sr = processor.getSampleRate();
    if (sr > 0.0)
    {
        const char* speedStr = (speedIdx == 0 ? "4.75" : speedIdx == 1 ? "9.5" : "19");
        juce::String s = juce::String (speedStr) + " cm/s · "
                       + juce::String (sr / 1000.0, 1) + " kHz";
        if (s != statusText)
        {
            statusText = s;
            repaint (juce::Rectangle<int> (getWidth() - 360, 24, 280, 24));
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

    // Reset baseline
    set ("speed", 2);
    set ("mic_gain", 0.5f); set ("mic_gain_r", 0.5f);
    set ("phono_gain", 0); set ("phono_gain_r", 0);
    set ("radio_gain", 0); set ("radio_gain_r", 0);
    set ("saturation_drive", 1.0f); set ("saturation_drive_r", 1.0f);
    set ("echo_amount", 0); set ("echo_amount_r", 0);
    set ("bias_amount", 1.0f);
    set ("wow_flutter", 0.3f);
    set ("multiplay_gen", 1);
    set ("bass_db", 0); set ("treble_db", 0);
    set ("balance", 0); set ("master_volume", 0.85f);
    setBool ("echo_enabled", false);
    setBool ("bypass_tape", false);
    setBool ("speaker_monitor", false);
    setBool ("synchroplay", false);
    setBool ("pause", false);
    setBool ("sos_enabled", false);
    setBool ("pa_enabled", false);
    setBool ("mic_loz", false);

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
