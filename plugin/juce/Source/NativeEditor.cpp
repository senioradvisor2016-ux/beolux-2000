/*  NativeEditor implementation — clean-slate native UI för BC2000DL v29.8.

    Layout (1100×640):
      Header   60px  : Title · Preset menu · A/B · About
      VU row   100px : VU L · VU R · status-text (speed/SR)
      Faders   240px : 5 dual-faders (Radio / Phono / Mic / Drive / Echo)
      Knobs    100px : 7 rotaries (Bias / Bass / Treble / Wow / Mult / Bal / Master)
      Combos    50px : Speed · Monitor · Phono · Radio · Formula
      Buttons   90px : 8 funktion-toggles + 7 mode-toggles
*/

#include "NativeEditor.h"

namespace
{
    // -------- Färger --------
    const auto kBg          = juce::Colour (0xff1a1a1c);
    const auto kPanel       = juce::Colour (0xff222226);
    const auto kPanelLight  = juce::Colour (0xff2a2a30);
    const auto kAccent      = juce::Colour (0xffd0a040);   // amber
    const auto kAccentDim   = juce::Colour (0xff8a6820);
    const auto kText        = juce::Colour (0xffe5e5e5);
    const auto kTextDim     = juce::Colour (0xff9a9aa0);

    constexpr int kEditorW = 1100;
    constexpr int kEditorH = 640;

    juce::String tooltipFor (const juce::String& id)
    {
        if (id == "speed")              return "Bandhastighet (4.75 / 9.5 / 19 cm/s)";
        if (id == "monitor_mode")       return "Source = pre-tape, Tape = post-tape";
        if (id == "phono_mode")         return "L = ceramic, H = magnetic (RIAA)";
        if (id == "radio_mode")         return "L = 3 mV, H = 100 mV linenivå";
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

    void styleVerticalSlider (juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::LinearVertical);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 36, 14);
        s.setColour (juce::Slider::backgroundColourId,  kPanel);
        s.setColour (juce::Slider::trackColourId,       kAccentDim);
        s.setColour (juce::Slider::thumbColourId,       kAccent);
        s.setColour (juce::Slider::textBoxTextColourId, kTextDim);
        s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        s.setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
        s.setNumDecimalPlacesToDisplay (2);
        s.setDoubleClickReturnValue (true, 0.0);
    }

    void styleRotary (juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 14);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                juce::MathConstants<float>::pi * 2.8f, true);
        s.setColour (juce::Slider::rotarySliderFillColourId,    kAccent);
        s.setColour (juce::Slider::rotarySliderOutlineColourId, kPanelLight);
        s.setColour (juce::Slider::thumbColourId,               kAccent);
        s.setColour (juce::Slider::textBoxTextColourId,         kTextDim);
        s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        s.setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
        s.setNumDecimalPlacesToDisplay (2);
    }

    void styleLabel (juce::Label& l, const juce::String& text,
                      juce::Colour c = kTextDim, float size = 10.0f, bool bold = true)
    {
        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setColour (juce::Label::textColourId, c);
        auto opts = juce::FontOptions (size).withName ("Helvetica");
        if (bold) opts = opts.withStyle ("Bold");
        l.setFont (juce::Font (opts));
    }

    void styleToggleBtn (juce::TextButton& b, const juce::String& text)
    {
        b.setButtonText (text);
        b.setClickingTogglesState (true);
        b.setColour (juce::TextButton::buttonColourId,      kPanel);
        b.setColour (juce::TextButton::buttonOnColourId,    kAccent);
        b.setColour (juce::TextButton::textColourOffId,     kText);
        b.setColour (juce::TextButton::textColourOnId,      juce::Colours::black);
        b.setColour (juce::ComboBox::outlineColourId,       kPanelLight);
    }

    void styleCombo (juce::ComboBox& c)
    {
        c.setColour (juce::ComboBox::backgroundColourId, kPanel);
        c.setColour (juce::ComboBox::textColourId,       kText);
        c.setColour (juce::ComboBox::outlineColourId,    kPanelLight);
        c.setColour (juce::ComboBox::arrowColourId,      kAccent);
    }
}

NativeEditor::NativeEditor (BC2000DLProcessor& p)
    : juce::AudioProcessorEditor (p), processor (p)
{
    setSize (kEditorW, kEditorH);
    setResizable (false, false);

    // ===== VU meters =====
    addAndMakeVisible (vuL);
    addAndMakeVisible (vuR);
    styleLabel (vuL_lbl, "L", kAccent, 14.0f, true);
    styleLabel (vuR_lbl, "R", kAccent, 14.0f, true);
    addAndMakeVisible (vuL_lbl);
    addAndMakeVisible (vuR_lbl);

    // ===== Dual-faders =====
    auto setupDual = [&] (DualFader& f, const juce::String& caption,
                           const juce::String& idL, const juce::String& idR)
    {
        styleVerticalSlider (f.l);
        styleVerticalSlider (f.r);
        f.l.setTooltip (tooltipFor (idL));
        f.r.setTooltip (tooltipFor (idR));
        addAndMakeVisible (f.l);
        addAndMakeVisible (f.r);
        sAtts.push_back (std::make_unique<SAtt> (processor.apvts, idL, f.l));
        sAtts.push_back (std::make_unique<SAtt> (processor.apvts, idR, f.r));

        styleLabel (f.caption, caption, kAccent, 11.0f, true);
        styleLabel (f.lLbl, "L", kTextDim, 9.0f);
        styleLabel (f.rLbl, "R", kTextDim, 9.0f);
        addAndMakeVisible (f.caption);
        addAndMakeVisible (f.lLbl);
        addAndMakeVisible (f.rLbl);
    };
    setupDual (radio, "RADIO", "radio_gain",       "radio_gain_r");
    setupDual (phono, "PHONO", "phono_gain",       "phono_gain_r");
    setupDual (mic,   "MIC",   "mic_gain",         "mic_gain_r");
    setupDual (drive, "DRIVE", "saturation_drive", "saturation_drive_r");
    setupDual (echo,  "ECHO",  "echo_amount",      "echo_amount_r");

    // ===== Knobs =====
    auto setupKnob = [&] (juce::Slider& s, juce::Label& l, const juce::String& id,
                           const juce::String& cap)
    {
        styleRotary (s);
        s.setTooltip (tooltipFor (id));
        addAndMakeVisible (s);
        sAtts.push_back (std::make_unique<SAtt> (processor.apvts, id, s));

        styleLabel (l, cap, kTextDim, 10.0f, true);
        addAndMakeVisible (l);
    };
    setupKnob (knob_bias,    lbl_bias,    "bias_amount",    "BIAS");
    setupKnob (knob_bass,    lbl_bass,    "bass_db",        "BASS");
    setupKnob (knob_treble,  lbl_treble,  "treble_db",      "TREBLE");
    setupKnob (knob_wow,     lbl_wow,     "wow_flutter",    "WOW");
    setupKnob (knob_mult,    lbl_mult,    "multiplay_gen",  "MULT");
    setupKnob (knob_balance, lbl_balance, "balance",        "BAL");
    setupKnob (knob_master,  lbl_master,  "master_volume",  "MASTER");

    // ===== Combo-boxes =====
    auto setupCombo = [&] (juce::ComboBox& c, juce::Label& l, const juce::String& id,
                            const juce::String& cap, std::initializer_list<const char*> items)
    {
        styleCombo (c);
        int idx = 1;
        for (auto* item : items) c.addItem (item, idx++);
        c.setTooltip (tooltipFor (id));
        addAndMakeVisible (c);
        cAtts.push_back (std::make_unique<CAtt> (processor.apvts, id, c));

        styleLabel (l, cap, kTextDim, 9.0f, true);
        l.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (l);
    };
    setupCombo (cb_speed,   lbl_speed,   "speed",        "SPEED",   { "4.75", "9.5", "19 cm/s" });
    setupCombo (cb_monitor, lbl_monitor, "monitor_mode", "MONITOR", { "Source", "Tape" });
    setupCombo (cb_phono,   lbl_phono,   "phono_mode",   "PHONO",   { "L (ceramic)", "H (magnetic)" });
    setupCombo (cb_radio,   lbl_radio,   "radio_mode",   "RADIO",   { "L (3 mV)", "H (100 mV)" });
    setupCombo (cb_formula, lbl_formula, "tape_formula", "FORMULA", { "Agfa", "BASF", "Scotch" });

    // ===== Toggle buttons =====
    auto setupBtn = [&] (juce::TextButton& b, const juce::String& cap, const juce::String& id)
    {
        styleToggleBtn (b, cap);
        b.setTooltip (tooltipFor (id));
        addAndMakeVisible (b);
        bAtts.push_back (std::make_unique<BAtt> (processor.apvts, id, b));
    };
    setupBtn (btn_echo,    "ECHO",    "echo_enabled");
    setupBtn (btn_bypass,  "BYPASS",  "bypass_tape");
    setupBtn (btn_speaker, "SPEAKER", "speaker_monitor");
    setupBtn (btn_sync,    "SYNC",    "synchroplay");
    setupBtn (btn_loz,     "LO-Z",    "mic_loz");
    setupBtn (btn_pa,      "P.A.",    "pa_enabled");
    setupBtn (btn_sos,     "SOS",     "sos_enabled");
    setupBtn (btn_pause,   "PAUSE",   "pause");
    setupBtn (btn_rec1,    "REC 1",   "rec_arm_1");
    setupBtn (btn_rec2,    "REC 2",   "rec_arm_2");
    setupBtn (btn_trk1,    "TRK 1",   "track_1");
    setupBtn (btn_trk2,    "TRK 2",   "track_2");
    setupBtn (btn_spkA,    "SPK A",   "speaker_ext");
    setupBtn (btn_spkB,    "SPK B",   "speaker_int");
    setupBtn (btn_mute,    "MUTE",    "speaker_mute");

    // ===== Header — preset menu, A/B, About =====
    styleCombo (cb_preset);
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
    cb_preset.setTooltip ("24 fabrikspresets · BERG-presets överst");
    addAndMakeVisible (cb_preset);

    auto styleNav = [&] (juce::TextButton& b, int delta)
    {
        b.setColour (juce::TextButton::buttonColourId, kPanel);
        b.setColour (juce::TextButton::textColourOffId, kText);
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
    styleNav (btn_prev, -1);
    styleNav (btn_next, +1);

    auto setupAB = [&] (juce::TextButton& b, bool isA)
    {
        b.setClickingTogglesState (true);
        b.setRadioGroupId (777);
        b.setColour (juce::TextButton::buttonColourId,   kPanel);
        b.setColour (juce::TextButton::buttonOnColourId, kAccent);
        b.setColour (juce::TextButton::textColourOffId,  kText);
        b.setColour (juce::TextButton::textColourOnId,   juce::Colours::black);
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

    btn_about.setColour (juce::TextButton::buttonColourId, kPanel);
    btn_about.setColour (juce::TextButton::textColourOffId, kText);
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
                              "Native JUCE UI — clean rebuild.")
                .withButton ("OK"),
            nullptr);
    };
    addAndMakeVisible (btn_about);

    stateA = processor.apvts.copyState();
    startTimerHz (30);
}

NativeEditor::~NativeEditor() = default;

// ============================================================================
//  Paint
// ============================================================================
void NativeEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);

    // Header-stripe
    auto header = getLocalBounds().removeFromTop (60).toFloat();
    g.setColour (kPanel);
    g.fillRect (header);
    g.setColour (kPanelLight);
    g.drawHorizontalLine (60, 0, (float) getWidth());

    // Title
    g.setColour (kText);
    g.setFont (juce::Font (juce::FontOptions (18.0f).withName ("Helvetica").withStyle ("Bold")));
    g.drawText ("BC2000DL", 16, 14, 200, 22, juce::Justification::left, false);
    g.setColour (kAccent);
    g.setFont (juce::Font (juce::FontOptions (10.0f).withName ("Helvetica").withStyle ("Bold")));
    g.drawText ("v29.8 NATIVE · DANISH TAPE 2000", 16, 36, 280, 14,
                juce::Justification::left, false);

    // Status (höger i header)
    g.setColour (kTextDim);
    g.setFont (juce::Font (juce::FontOptions (11.0f).withName ("Helvetica")));
    g.drawText (statusText, getWidth() - 240, 36, 180, 14,
                juce::Justification::right, false);

    // Section divider lines
    auto drawDiv = [&] (int y) {
        g.setColour (kPanelLight);
        g.drawHorizontalLine (y, 8.0f, (float) getWidth() - 8);
    };
    drawDiv (60 + 100);             // efter VU
    drawDiv (60 + 100 + 240);       // efter faders
    drawDiv (60 + 100 + 240 + 100); // efter knobs
    drawDiv (60 + 100 + 240 + 100 + 50); // efter combos
}

// ============================================================================
//  Layout
// ============================================================================
void NativeEditor::resized()
{
    auto bounds = getLocalBounds();

    // ----- Header -----
    auto header = bounds.removeFromTop (60).reduced (16, 14);
    header.removeFromLeft (300); // title-zon

    // Höger-side: About
    btn_about.setBounds (header.removeFromRight (32));
    header.removeFromRight (8);

    // A/B
    btn_b.setBounds (header.removeFromRight (32));
    header.removeFromRight (4);
    btn_a.setBounds (header.removeFromRight (32));
    header.removeFromRight (12);

    // Preset-rad: < [menu] >
    auto presetW = juce::jmin (320, header.getWidth());
    auto presetArea = header.removeFromRight (presetW);
    btn_prev.setBounds (presetArea.removeFromLeft (24));
    presetArea.removeFromLeft (4);
    btn_next.setBounds (presetArea.removeFromRight (24));
    presetArea.removeFromRight (4);
    cb_preset.setBounds (presetArea);

    // ----- VU row (100 px) -----
    auto vuRow = bounds.removeFromTop (100).reduced (16, 14);
    auto vuLeft = vuRow.removeFromLeft (vuRow.getWidth() / 2 - 10);
    vuRow.removeFromLeft (20);
    auto vuRight = vuRow;

    auto layoutVU = [&] (juce::Rectangle<int> area, juce::Label& lbl, bc2000dl::VUBar& bar)
    {
        lbl.setBounds (area.removeFromLeft (20));
        bar.setBounds (area.reduced (4, 16));
    };
    layoutVU (vuLeft,  vuL_lbl, vuL);
    layoutVU (vuRight, vuR_lbl, vuR);

    // ----- Fader bank (240 px) — 5 dual-faders -----
    auto faderRow = bounds.removeFromTop (240).reduced (16, 8);
    const int dualW = faderRow.getWidth() / 5;

    auto layoutDual = [&] (juce::Rectangle<int> area, DualFader& f)
    {
        f.caption.setBounds (area.removeFromTop (16));
        auto bottom = area.removeFromBottom (16);
        const int halfW = bottom.getWidth() / 2;
        f.lLbl.setBounds (bottom.removeFromLeft (halfW));
        f.rLbl.setBounds (bottom);
        // L och R sliders i två kolumner
        const int sw = area.getWidth() / 2;
        f.l.setBounds (area.removeFromLeft (sw).reduced (4, 0));
        f.r.setBounds (area.reduced (4, 0));
    };
    layoutDual (faderRow.removeFromLeft (dualW), radio);
    layoutDual (faderRow.removeFromLeft (dualW), phono);
    layoutDual (faderRow.removeFromLeft (dualW), mic);
    layoutDual (faderRow.removeFromLeft (dualW), drive);
    layoutDual (faderRow.removeFromLeft (dualW), echo);

    // ----- Knob row (100 px) — 7 rotaries -----
    auto knobRow = bounds.removeFromTop (100).reduced (16, 8);
    const int knobW = knobRow.getWidth() / 7;

    auto layoutKnob = [&] (juce::Rectangle<int> area, juce::Slider& s, juce::Label& l)
    {
        l.setBounds (area.removeFromTop (12));
        s.setBounds (area.reduced (8, 0));
    };
    layoutKnob (knobRow.removeFromLeft (knobW), knob_bias,    lbl_bias);
    layoutKnob (knobRow.removeFromLeft (knobW), knob_bass,    lbl_bass);
    layoutKnob (knobRow.removeFromLeft (knobW), knob_treble,  lbl_treble);
    layoutKnob (knobRow.removeFromLeft (knobW), knob_wow,     lbl_wow);
    layoutKnob (knobRow.removeFromLeft (knobW), knob_mult,    lbl_mult);
    layoutKnob (knobRow.removeFromLeft (knobW), knob_balance, lbl_balance);
    layoutKnob (knobRow.removeFromLeft (knobW), knob_master,  lbl_master);

    // ----- Combo row (50 px) — 5 selectors -----
    auto comboRow = bounds.removeFromTop (50).reduced (16, 8);
    const int cellW = comboRow.getWidth() / 5;

    auto layoutCombo = [&] (juce::Rectangle<int> area, juce::ComboBox& c, juce::Label& l)
    {
        l.setBounds (area.removeFromTop (12));
        c.setBounds (area.reduced (4, 2));
    };
    layoutCombo (comboRow.removeFromLeft (cellW), cb_speed,   lbl_speed);
    layoutCombo (comboRow.removeFromLeft (cellW), cb_monitor, lbl_monitor);
    layoutCombo (comboRow.removeFromLeft (cellW), cb_phono,   lbl_phono);
    layoutCombo (comboRow.removeFromLeft (cellW), cb_radio,   lbl_radio);
    layoutCombo (comboRow.removeFromLeft (cellW), cb_formula, lbl_formula);

    // ----- Buttons (resten ~90 px, 2 rader) -----
    auto btnArea = bounds.reduced (16, 8);
    auto row1 = btnArea.removeFromTop (32);
    btnArea.removeFromTop (6);
    auto row2 = btnArea.removeFromTop (32);

    auto layoutBtnRow = [&] (juce::Rectangle<int> row, std::initializer_list<juce::TextButton*> btns)
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
    // Rad 1: 8 funktion-toggles
    layoutBtnRow (row1, { &btn_echo, &btn_bypass, &btn_speaker, &btn_sync,
                          &btn_loz, &btn_pa, &btn_sos, &btn_pause });
    // Rad 2: 7 mode-toggles (7, så lite bredare)
    layoutBtnRow (row2, { &btn_rec1, &btn_rec2, &btn_trk1, &btn_trk2,
                          &btn_spkA, &btn_spkB, &btn_mute });
}

// ============================================================================
//  Timer (30 Hz)
// ============================================================================
void NativeEditor::timerCallback()
{
    auto& chain = processor.getChain();
    vuL.setLevel (chain.meterLevelL_dBFS.load());
    vuR.setLevel (chain.meterLevelR_dBFS.load());
    vuL.setRecording (chain.isRecordingL.load());
    vuR.setRecording (chain.isRecordingR.load());

    // Status: speed + sample rate
    const auto sr = processor.getSampleRate();
    if (sr > 0.0)
    {
        const int speedIdx = (int) processor.apvts.getRawParameterValue ("speed")->load();
        const char* speedStr = (speedIdx == 0 ? "4.75" : speedIdx == 1 ? "9.5" : "19");
        juce::String s = juce::String (speedStr) + " cm/s · "
                       + juce::String (sr / 1000.0, 1) + " kHz";
        if (s != statusText)
        {
            statusText = s;
            repaint (juce::Rectangle<int> (getWidth() - 260, 30, 260, 24));
        }
    }
}

// ============================================================================
//  Presets
// ============================================================================
void NativeEditor::applyPreset (int idx)
{
    auto& v = processor.apvts;
    auto set = [&] (const juce::String& id, float val)
    {
        if (auto* prm = v.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (val));
    };
    auto setBool = [&] (const juce::String& id, bool b) { set (id, b ? 1.0f : 0.0f); };

    // Reset till säker baseline
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

    // Preset-specifika overrides (förenklad version — fokuserar på distinkt karaktär)
    switch (idx)
    {
        case  1: // BERG DEPECHE PAD — BASF + slow saturation + lite echo
            set ("tape_formula", 1); set ("speed", 1);
            set ("saturation_drive", 1.6f); set ("saturation_drive_r", 1.6f);
            set ("echo_amount", 0.25f); set ("echo_amount_r", 0.25f);
            setBool ("echo_enabled", true);
            set ("bass_db", 2.5f); break;
        case  2: // BERG DELTA SYNTH — Scotch crunch
            set ("tape_formula", 2);
            set ("saturation_drive", 1.85f); set ("saturation_drive_r", 1.85f);
            set ("bias_amount", 0.85f); break;
        case  3: // BERG PROTON CRUNCH
            set ("tape_formula", 2); set ("speed", 0);
            set ("saturation_drive", 1.95f); set ("saturation_drive_r", 1.95f);
            set ("bias_amount", 0.75f); break;
        case  4: // BERG REEL VOCALS — silky
            set ("tape_formula", 0); set ("speed", 2);
            set ("saturation_drive", 1.3f); set ("saturation_drive_r", 1.3f);
            set ("treble_db", 1.0f); break;
        case  5: // FERRO+ — Agfa warmth
            set ("tape_formula", 0); set ("saturation_drive", 1.4f); break;
        case  6: // CHROME — BASF
            set ("tape_formula", 1); break;
        case  7: // METAL IV — Scotch + drive
            set ("tape_formula", 2); set ("saturation_drive", 1.5f); break;
        case  8: // BBC J — broadcast clean
            set ("speed", 2); set ("bias_amount", 1.0f); break;
        case  9: // NAGRA — field-recorder
            set ("speed", 1); set ("wow_flutter", 0.5f); break;
        case 10: // CASSETTE 70s
            set ("tape_formula", 1); set ("speed", 0);
            set ("wow_flutter", 0.8f); set ("saturation_drive", 1.6f); break;
        case 11: // BROADCAST
            set ("speed", 2); break;
        case 12: // VISCONTI '74 — Bowie's Berlin sound
            set ("tape_formula", 1); set ("speed", 2);
            set ("saturation_drive", 1.45f); set ("bass_db", 1.5f); break;
        case 13: // BUTLER MIX — UK soul
            set ("treble_db", 0.8f); set ("bass_db", 1.2f); break;
        case 14: // KRAFTWERK '78
            set ("tape_formula", 1); set ("speed", 1);
            set ("echo_amount", 0.4f); setBool ("echo_enabled", true); break;
        case 15: // RHODES WARM
            set ("treble_db", -1.0f); set ("bass_db", 1.5f);
            set ("saturation_drive", 1.3f); break;
        case 16: // DRUM SMASH
            set ("saturation_drive", 1.95f); set ("saturation_drive_r", 1.95f); break;
        case 17: // VOCAL SILK
            set ("saturation_drive", 1.15f); set ("treble_db", 1.5f); break;
        case 18: // BASS WEIGHT
            set ("bass_db", 4.0f); set ("saturation_drive", 1.4f); break;
        case 19: // LO-FI POP
            set ("tape_formula", 2); set ("speed", 0);
            set ("wow_flutter", 1.2f); break;
        case 20: // DUB ECHO
            set ("echo_amount", 0.7f); set ("echo_amount_r", 0.7f);
            setBool ("echo_enabled", true);
            set ("bass_db", 3.0f); break;
        case 21: // GLUE MASTER
            set ("saturation_drive", 1.25f); break;
        case 22: // SHOEGAZE
            set ("echo_amount", 0.55f); setBool ("echo_enabled", true);
            set ("treble_db", -0.5f); break;
        case 23: // CRT NIGHTMARE
            set ("wow_flutter", 1.8f); set ("multiplay_gen", 4);
            set ("saturation_drive", 1.7f); break;
        default: break; // 0 = FACTORY (redan reset)
    }
}
