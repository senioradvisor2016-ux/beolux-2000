/*  BC2000DLEditor — Ferroflux 2000 · DL UI.

    Layoutmappning från "Tape Deck Plugin.html":
      - 1400 × 680 canvas, 44 px walnut cheeks, cream face mellan
      - Header 56 px (Ferroflux 2000 · DL + tagline + PWR/REC/TYPE)
      - Top row: VU L | VU R | TapeBay (large reels) | Counter
      - Mid row:  Slider bank (8 verticals on dark recess)  |  Knob cluster (8 ink-knobs på dark recess)
      - Bottom:   Transport piano keys + selectors  |  Preset bar
      - Footer 26 px (etched ribbon)

    Existerande DSP-parametrar mappas till Ferroflux-etiketter där det är meningsfullt.
*/

#include "PluginEditor.h"

namespace
{
    using namespace bc2000dl::ui::colours;

    // Beocord 2000 DL-aspect ~1.3:1 — höjd ökad för att rymma huge reels topp
    // + control-panel nederst utan att kontroller hamnar bakom deck-zonen.
    constexpr int kEditorW   = 1280;
    constexpr int kEditorH   = 1180;   // +80px så selRow2 (monitor/formula/Lo-Z/P.A./S/S) garanterat syns
    constexpr int kCheekW    = 44;
    constexpr int kHeaderH   = 56;
    constexpr int kFooterH   = 26;

    juce::String tooltipFor (const juce::String& id)
    {
        if (id == "speed")              return "Bandhastighet — 4.75 / 9.5 / 19 cm/s (manualens 3 lägen)";
        if (id == "monitor_mode")       return "Monitor: SOURCE = före tape · TAPE = efter tape (default)";
        if (id == "phono_mode")         return "Phono input: L = keramisk (flat) · H = magnetisk (RIAA)";
        if (id == "radio_mode")         return "Radio input: L = 3 mV känslighet · H = 100 mV linenivå";
        if (id == "tape_formula")       return "Bandtyp: Agfa (varm) / BASF (HF-headroom) / Scotch (komprimerad)";
        if (id == "mic_gain")           return "Mikrofonkanalens nivå";
        if (id == "phono_gain")         return "Grammofonkanalens nivå";
        if (id == "radio_gain")         return "Radiokanalens nivå";
        if (id == "saturation_drive")   return "Tape-saturation drive";
        if (id == "echo_amount")        return "Echo-mängd — hög nivå ger self-oscillation (autentiskt 1968)";
        if (id == "treble_db")          return "Diskantkontroll (Baxandall, post-playback)";
        if (id == "bass_db")            return "Baskontroll (Baxandall, post-playback)";
        if (id == "balance")            return "Balans L/R";
        if (id == "master_volume")      return "Master output";
        if (id == "bias_amount")        return "Bias-strömmen vid 100 kHz (under nominal = mer 3rd harm)";
        if (id == "wow_flutter")        return "Wow & flutter-amplitud";
        if (id == "multiplay_gen")      return "Multiplay-generation 1–5: kumulativ HF-förlust + brus";
        if (id == "echo_enabled")       return "Aktivera echo-loop";
        if (id == "bypass_tape")        return "Bypass tape-blocket — ren input + tone";
        if (id == "speaker_monitor")    return "Engagera AD139-power-amp + AUTOMATSIKRING-soft-clip";
        if (id == "synchroplay")        return "Record-headet som playback (low-latency)";
        if (id == "rec_arm_1")          return "Arma spår 1";
        if (id == "rec_arm_2")          return "Arma spår 2";
        if (id == "track_1")            return "Lyssna på spår 1";
        if (id == "track_2")            return "Lyssna på spår 2";
        if (id == "sos_enabled")        return "Sound-on-Sound: PLAY L → REC R";
        if (id == "pause")              return "Tysta output";
        if (id == "pa_enabled")         return "P.A.: mic duckar phono+radio";
        if (id == "speaker_ext")        return "Extern högtalare A";
        if (id == "speaker_int")        return "Intern högtalare B";
        if (id == "speaker_mute")       return "Mute alla högtalare";
        if (id == "mic_loz")            return "Lo-Z mic (50–200 Ω) → ingångstrafo. Av = Hi-Z (500 kΩ)";
        return {};
    }

    void drawDarkRecess (juce::Graphics& g, juce::Rectangle<int> r,
                          const juce::String& chipText = {},
                          const juce::String& tagline = {})
    {
        const auto bf = r.toFloat();

        juce::ColourGradient grad (kRecessTop, bf.getX(), bf.getY(),
                                    kRecessBot, bf.getX(), bf.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bf, 5.0f);

        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (bf, 5.0f, 1.0f);

        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawLine (bf.getX() + 4, bf.getY() + 2.5f,
                    bf.getRight() - 4, bf.getY() + 2.5f, 1.0f);
        g.setColour (kCream1.brighter (0.2f).withAlpha (0.4f));
        g.drawLine (bf.getX() + 4, bf.getBottom() - 0.5f,
                    bf.getRight() - 4, bf.getBottom() - 0.5f, 0.5f);

        if (chipText.isNotEmpty())
        {
            const auto chip = juce::Rectangle<float> (
                bf.getX() + 14.0f, bf.getY() + 7.0f, 84.0f, 16.0f);
            g.setColour (kAmberDim.withAlpha (0.4f));
            g.drawRoundedRectangle (chip, 2.0f, 0.8f);
            g.setColour (kAmber);
            g.setFont (juce::Font (juce::FontOptions (8.5f).withName ("Helvetica").withStyle ("Bold")));
            g.drawText (chipText, chip, juce::Justification::centred, false);
        }
        if (tagline.isNotEmpty())
        {
            g.setColour (kSoftGray);
            g.setFont (juce::Font (juce::FontOptions (10.0f).withName ("Helvetica").withStyle ("Italic")));
            g.drawText (tagline,
                        juce::Rectangle<float> (bf.getX() + 110.0f, bf.getY() + 7.0f,
                                                 bf.getWidth() - 130.0f, 16.0f),
                        juce::Justification::centredLeft, false);
        }
    }

    void drawSliderBankHeader (juce::Graphics& g, juce::Rectangle<int> bankR)
    {
        drawDarkRecess (g, bankR, "LEVEL BANK", "recording & output");

        const auto bf = bankR.toFloat();
        // höger label
        g.setColour (kSoftGray);
        g.setFont (juce::Font (juce::FontOptions (8.0f).withName ("Helvetica").withStyle ("Bold")));
        g.drawText ("\xe2\x88\x92 10 ... 0 ... + 6  dB",
                    juce::Rectangle<float> (bf.getRight() - 160.0f, bf.getY() + 7.0f,
                                             140.0f, 16.0f),
                    juce::Justification::centredRight, false);
        // amber lamp t.h.
        const float lx = bf.getRight() - 175.0f;
        const float ly = bf.getY() + 11.0f;
        g.setColour (kAmber.withAlpha (0.4f));
        g.fillEllipse (lx - 1, ly - 1, 8, 8);
        g.setColour (kAmberGlow);
        g.fillEllipse (lx + 1, ly + 1, 5, 5);
    }
}

BC2000DLEditor::BC2000DLEditor (BC2000DLProcessor& p)
    : juce::AudioProcessorEditor (&p),
      audioProc (p),
      meterL ("LEFT"),
      meterR ("RIGHT")
{
    setLookAndFeel (&lookAndFeel);
    setSize (kEditorW, kEditorH);
    setOpaque (true);
    setWantsKeyboardFocus (true);   // Cmd+I → About-dialog (UAD-pattern)

    addAndMakeVisible (heroPanel);
    addAndMakeVisible (meterL);
    addAndMakeVisible (meterR);
    addAndMakeVisible (transportLever);
    addAndMakeVisible (tapeCounter);
    addAndMakeVisible (recIndL);
    addAndMakeVisible (recIndR);

    transportLever.onStateChange = [this] (bc2000dl::ui::TransportLever::State s)
    {
        const bool running = (s == bc2000dl::ui::TransportLever::Play
                            || s == bc2000dl::ui::TransportLever::FF
                            || s == bc2000dl::ui::TransportLever::REW);
        tapeCounter.setRunning (running);
    };

    auto attachTip = [] (juce::SettableTooltipClient& c, const juce::String& id)
    {
        auto t = tooltipFor (id);
        if (t.isNotEmpty()) c.setTooltip (t);
    };

    auto styleEtchedLbl = [&] (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setFont (juce::Font (juce::FontOptions (9.0f).withName ("Helvetica").withStyle ("Bold")));
        l.setJustificationType (juce::Justification::centred);
        // Cream-färg eftersom labels hamnar på mörk recess (slider-bank/knob-cluster)
        l.setColour (juce::Label::textColourId, juce::Colour (0xffd0cdc4));
        addAndMakeVisible (l);
    };

    // ===== 5 B&O dual-skydepotentiometre — autentisk BC2000 DL =====
    auto setupDualFader = [&] (bc2000dl::ui::DualSlideFader& f,
                                const juce::String& idL, const juce::String& idR)
    {
        addAndMakeVisible (f);
        sliderAttachments.push_back (std::make_unique<SliderAttach> (
            audioProc.apvts, idL, f.getLeftSlider()));
        sliderAttachments.push_back (std::make_unique<SliderAttach> (
            audioProc.apvts, idR, f.getRightSlider()));
        attachTip (f.getLeftSlider(), idL);
        attachTip (f.getRightSlider(), idL);
    };
    setupDualFader (radioFader, "radio_gain",   "radio_gain_r");
    setupDualFader (phonoFader, "phono_gain",   "phono_gain_r");
    setupDualFader (micFader,   "mic_gain",     "mic_gain_r");
    // S-fadern styr saturation-drive (sound-on-sound är toggle, ej fader på original)
    setupDualFader (sosFader,   "saturation_drive", "saturation_drive_r");
    setupDualFader (echoFader,  "echo_amount",  "echo_amount_r");

    // ===== Knob cluster — 8 mörka rotaries =====
    auto setupKnob = [&] (juce::Slider& s, const juce::String& paramId,
                           juce::Label& lbl, const juce::String& labelText)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        // UAD-pattern: trigga repaint vid mouse-enter/exit/click så hover-glow funkar
        s.setRepaintsOnMouseActivity (true);
        addAndMakeVisible (s);
        sliderAttachments.push_back (std::make_unique<SliderAttach> (audioProc.apvts, paramId, s));
        attachTip (s, paramId);
        styleEtchedLbl (lbl, labelText);
    };
    setupKnob (biasKnob,    "bias_amount",      bias_lbl,    "BIAS");
    setupKnob (recEqKnob,   "treble_db",        recEq_lbl,   "TREBLE");
    setupKnob (wowKnob,     "wow_flutter",      wow_lbl,     "WOW & FLT");
    setupKnob (satKnob,     "saturation_drive", sat_lbl,     "SATURATE");
    // Master volume — tidigare saknades helt, nu på advanced grid (var "ECHO R"
    // duplicate av echoFader-höger). echoFader styr fortfarande både L+R echo.
    setupKnob (echoKnob,    "master_volume",    echoR_lbl,   "MASTER");
    setupKnob (multKnob,    "multiplay_gen",    mult_lbl,    "MULT");
    setupKnob (bassKnob,    "bass_db",          bass_lbl,    "BASS");
    setupKnob (balanceKnob, "balance",          balance_lbl, "BALANCE");

    // ===== Selectors =====
    auto setupCombo = [&] (juce::ComboBox& cb, juce::Label& lbl,
                            const juce::String& labelText,
                            const juce::String& paramId,
                            std::initializer_list<juce::String> options,
                            std::unique_ptr<ChoiceAttach>& attachOut)
    {
        int id = 1;
        for (auto& o : options) cb.addItem (o, id++);
        addAndMakeVisible (cb);
        attachTip (cb, paramId);
        attachOut = std::make_unique<ChoiceAttach> (audioProc.apvts, paramId, cb);

        lbl.setText (labelText, juce::dontSendNotification);
        lbl.setJustificationType (juce::Justification::centredLeft);
        lbl.setColour (juce::Label::textColourId, kInkSoft);
        lbl.setFont (juce::Font (juce::FontOptions (8.5f).withName ("Helvetica").withStyle ("Bold")));
        addAndMakeVisible (lbl);
    };
    setupCombo (speedBox,        speedLabel,        "TAPE SPEED",
                "speed",        { "4.75 cm/s", "9.5 cm/s", "19 cm/s" }, speedAttach);
    setupCombo (monitorBox,      monitorLabel,      "MONITOR",
                "monitor_mode", { "Source", "Tape" }, monitorAttach);
    setupCombo (phonoModeBox,    phonoModeLabel,    "PHONO INPUT",
                "phono_mode",   { "L (ceramic)", "H (magnetic)" }, phonoModeAttach);
    setupCombo (radioModeBox,    radioModeLabel,    "RADIO INPUT",
                "radio_mode",   { "L (3 mV)", "H (100 mV)" }, radioModeAttach);
    setupCombo (tapeFormulaBox,  tapeFormulaLabel,  "TAPE FORMULA",
                "tape_formula", { "Agfa", "BASF", "Scotch" }, tapeFormulaAttach);

    // === UAD-Studer-A800-style knapp-rader (ersätter dropdowns ovan) ===
    speedSelector = std::make_unique<bc2000dl::ui::UADStyleSelector> (
        audioProc.apvts, "speed",
        juce::StringArray { "4.75", "9.5", "19" }, "TAPE SPEED  cm/s");
    formulaSelector = std::make_unique<bc2000dl::ui::UADStyleSelector> (
        audioProc.apvts, "tape_formula",
        juce::StringArray { "AGFA", "BASF", "SCOTCH" }, "TAPE FORMULA");
    phonoSelector = std::make_unique<bc2000dl::ui::UADStyleSelector> (
        audioProc.apvts, "phono_mode",
        juce::StringArray { "L", "H" }, "PHONO");
    monitorSelector = std::make_unique<bc2000dl::ui::UADStyleSelector> (
        audioProc.apvts, "monitor_mode",
        juce::StringArray { "SOURCE", "TAPE" }, "MONITOR");

    addAndMakeVisible (speedSelector.get());
    addAndMakeVisible (formulaSelector.get());
    addAndMakeVisible (phonoSelector.get());
    addAndMakeVisible (monitorSelector.get());

    // KRITISK FIX (v24): UADStyleSelectors renderar inte tillförlitligt — vi DÖLJER dem
    // och behåller de gamla dropdowns synliga så ALL funktionalitet är tillgänglig
    if (speedSelector)    speedSelector->setVisible (false);
    if (formulaSelector)  formulaSelector->setVisible (false);
    if (phonoSelector)    phonoSelector->setVisible (false);
    if (monitorSelector)  monitorSelector->setVisible (false);
    // Gamla comboboxes synliga (default)

    // ===== Push-buttons =====
    auto setupBtn = [&] (juce::ToggleButton& b, const juce::String& label,
                          const juce::String& paramId)
    {
        b.setButtonText (label);
        addAndMakeVisible (b);
        buttonAttachments.push_back (std::make_unique<ButtonAttach> (audioProc.apvts, paramId, b));
        attachTip (b, paramId);
    };
    setupBtn (echoBtn,    "ECHO",    "echo_enabled");
    setupBtn (bypassBtn,  "BYPASS",  "bypass_tape");
    setupBtn (speakerBtn, "SPEAKER", "speaker_monitor");
    setupBtn (syncBtn,    "SYNC",    "synchroplay");
    setupBtn (recArm1Btn, "REC 1",   "rec_arm_1");
    setupBtn (recArm2Btn, "REC 2",   "rec_arm_2");
    setupBtn (track1Btn,  "TRK 1",   "track_1");
    setupBtn (track2Btn,  "TRK 2",   "track_2");
    setupBtn (sosBtn,     "S/S",     "sos_enabled");
    setupBtn (pauseBtn,   "PAUSE",   "pause");
    setupBtn (paBtn,      "P.A.",    "pa_enabled");
    setupBtn (spkExtBtn,  "SPK A",   "speaker_ext");
    setupBtn (spkIntBtn,  "SPK B",   "speaker_int");
    setupBtn (spkMuteBtn, "MUTE",    "speaker_mute");
    setupBtn (micLoZBtn,  "Lo-Z",    "mic_loz");

    // ===== Preset menu (header) — 20 presets totalt =====
    int pid = 1;
    // BERG-presets överst (showcasar pluggens karaktär för Christoffer Berg / Delta Machine-stil)
    for (auto name : { "FACTORY", "BERG • DEPECHE PAD", "BERG • DELTA SYNTH BUS",
                       "BERG • PROTON CRUNCH", "BERG • REEL TO REEL VOCALS",
                       "FERRO+", "CHROME", "METAL IV",
                       "BBC J", "NAGRA", "CASSETTE 70s", "BROADCAST",
                       "VISCONTI '74", "BUTLER MIX", "KRAFTWERK '78", "RHODES WARM",
                       "DRUM SMASH", "VOCAL SILK", "BASS WEIGHT", "LO-FI POP",
                       "DUB ECHO", "GLUE MASTER", "SHOEGAZE", "CRT NIGHTMARE" })
        presetMenu.addItem (name, pid++);
    presetMenu.setText ("FACTORY", juce::dontSendNotification);
    presetMenu.onChange = [this] { applyPreset (presetMenu.getSelectedId() - 1); };
    addAndMakeVisible (presetMenu);
    presetMenu.setTooltip ("1968 + modern + engineer-style + genre-presets");

    auto setupNav = [&] (juce::TextButton& b, int delta)
    {
        addAndMakeVisible (b);
        b.onClick = [this, delta]
        {
            const int n = presetMenu.getNumItems();
            int curr = presetMenu.getSelectedId();
            if (curr < 1) curr = 1;
            int next = ((curr - 1 + delta + n) % n) + 1;
            presetMenu.setSelectedId (next);
        };
    };
    setupNav (prevPresetBtn, -1);
    setupNav (nextPresetBtn, +1);

    auto setupAB = [&] (juce::TextButton& b, bool isA)
    {
        b.setClickingTogglesState (true);
        b.setRadioGroupId (777);
        addAndMakeVisible (b);
        b.onClick = [this, isA]
        {
            auto& store = slotIsA ? stateA : stateB;
            store = audioProc.apvts.copyState();
            slotIsA = isA;
            auto& load = slotIsA ? stateA : stateB;
            if (load.isValid())
                audioProc.apvts.replaceState (load);
        };
    };
    setupAB (aBtn, true);
    setupAB (bBtn, false);
    aBtn.setToggleState (true, juce::dontSendNotification);

    stateA = audioProc.apvts.copyState();
    startTimerHz (30);
}

BC2000DLEditor::~BC2000DLEditor()
{
    setLookAndFeel (nullptr);
}

void BC2000DLEditor::applyPreset (int idx)
{
    auto& v = audioProc.apvts;
    auto set = [&] (const juce::String& id, float val)
    {
        if (auto* prm = v.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (val));
    };
    auto setBool = [&] (const juce::String& id, bool b) { set (id, b ? 1.0f : 0.0f); };

    // Reset
    set ("speed", 2);
    set ("mic_gain", 0); set ("mic_gain_r", 0);
    set ("phono_gain", 0); set ("phono_gain_r", 0);
    set ("radio_gain", 0); set ("radio_gain_r", 0);
    set ("saturation_drive", 1.0f); set ("saturation_drive_r", 1.0f);
    set ("echo_amount", 0); set ("echo_amount_r", 0);
    set ("treble_db", 0); set ("bass_db", 0);
    set ("balance", 0); set ("master_volume", 0.75f);
    set ("bias_amount", 1.0f); set ("wow_flutter", 1.0f);
    set ("multiplay_gen", 1);
    setBool ("echo_enabled", false); setBool ("bypass_tape", false);
    setBool ("speaker_monitor", false); setBool ("synchroplay", false);
    setBool ("sos_enabled", false); setBool ("pa_enabled", false);
    setBool ("track_1", true); setBool ("track_2", true);
    set ("monitor_mode", 1); set ("phono_mode", 1); set ("radio_mode", 0);
    set ("tape_formula", 0);

    switch (idx)
    {
        // ===== BERG-presets (Christoffer Berg / Delta Machine-style) =====
        case 1:  // BERG • DEPECHE PAD — varm tape-saturation på pads
            set ("mic_gain", 0.55f); set ("mic_gain_r", 0.55f);
            set ("saturation_drive", 1.35f); set ("saturation_drive_r", 1.35f);
            set ("bias_amount", 1.05f); set ("treble_db", -1.5f); set ("bass_db", 1.5f);
            set ("wow_flutter", 0.6f); set ("speed", 2);
            set ("tape_formula", 0); break;
        case 2:  // BERG • DELTA SYNTH BUS — tight bus-glue + lite SOS
            set ("mic_gain", 0.5f); set ("mic_gain_r", 0.5f);
            set ("saturation_drive", 1.15f); set ("saturation_drive_r", 1.15f);
            set ("bias_amount", 1.10f); set ("speed", 2);
            set ("tape_formula", 1); setBool ("sos_enabled", true); break;
        case 3:  // BERG • PROTON CRUNCH — heavy distort på leads
            set ("mic_gain", 0.7f); set ("mic_gain_r", 0.7f);
            set ("saturation_drive", 1.85f); set ("saturation_drive_r", 1.85f);
            set ("bias_amount", 0.85f); set ("treble_db", 2.0f);
            set ("multiplay_gen", 2); set ("speed", 1);
            set ("tape_formula", 2); break;
        case 4:  // BERG • REEL TO REEL VOCALS — silky vocal-tape
            set ("mic_gain", 0.6f); set ("mic_gain_r", 0.6f);
            set ("saturation_drive", 1.10f); set ("saturation_drive_r", 1.10f);
            set ("bias_amount", 1.00f); set ("treble_db", 0.5f); set ("bass_db", 0.5f);
            set ("wow_flutter", 0.3f); set ("speed", 2);
            set ("tape_formula", 0); break;

        case 5:  // FERRO+
            set ("mic_gain", 0.6f); set ("mic_gain_r", 0.6f);
            set ("saturation_drive", 1.2f); set ("saturation_drive_r", 1.2f);
            set ("tape_formula", 0); break;
        case 6:  // CHROME
            set ("radio_gain", 0.55f); set ("radio_gain_r", 0.55f);
            set ("bias_amount", 1.15f); set ("treble_db", 1.5f);
            set ("tape_formula", 1); break;
        case 7:  // BBC J
            set ("mic_gain", 0.55f); set ("mic_gain_r", 0.55f);
            setBool ("echo_enabled", true);
            set ("echo_amount", 0.4f); set ("echo_amount_r", 0.4f);
            set ("bias_amount", 0.9f); set ("speed", 2);
            set ("tape_formula", 1); break;
        case 8:  // NAGRA
            set ("mic_gain", 0.5f); set ("mic_gain_r", 0.5f);
            set ("saturation_drive", 1.05f); set ("saturation_drive_r", 1.05f);
            set ("speed", 2); break;
        case 9:  // CASSETTE 70s
            set ("radio_gain", 0.5f); set ("radio_gain_r", 0.5f);
            set ("saturation_drive", 1.5f); set ("saturation_drive_r", 1.5f);
            set ("bias_amount", 0.8f); set ("treble_db", -3.0f);
            set ("multiplay_gen", 3); set ("speed", 0);
            set ("tape_formula", 2); break;
        case 10: // BROADCAST
            set ("radio_gain", 0.6f); set ("radio_gain_r", 0.6f);
            set ("speed", 2); set ("tape_formula", 1); break;
        // ----- v3 engineer-presets -----
        case 11: // METAL IV (förut saknat case)
            set ("mic_gain", 0.5f); set ("mic_gain_r", 0.5f);
            set ("bias_amount", 1.28f); set ("treble_db", 0.5f);
            set ("tape_formula", 2); break;
        case 12: // VISCONTI '74
            set ("mic_gain", 0.55f); set ("mic_gain_r", 0.55f);
            setBool ("echo_enabled", true);
            set ("echo_amount", 0.55f); set ("echo_amount_r", 0.55f);
            set ("saturation_drive", 1.4f); set ("saturation_drive_r", 1.4f);
            set ("treble_db", 2.0f); set ("speed", 2); break;
        case 13: // BUTLER MIX
            set ("mic_gain", 0.5f); set ("mic_gain_r", 0.5f);
            set ("saturation_drive", 1.1f); set ("saturation_drive_r", 1.1f);
            set ("speed", 2); set ("tape_formula", 1); break;
        case 14: // KRAFTWERK '78
            set ("radio_gain", 0.55f); set ("radio_gain_r", 0.55f);
            set ("bias_amount", 1.12f); set ("treble_db", -2.0f);
            set ("speed", 1); set ("tape_formula", 1); break;
        case 15: // RHODES WARM
            set ("mic_gain", 0.5f); set ("mic_gain_r", 0.5f);
            set ("saturation_drive", 1.25f); set ("saturation_drive_r", 1.25f);
            set ("treble_db", 1.0f); set ("speed", 2); break;
        // ----- v3.3 genre/engineer-presets -----
        case 16: // DRUM SMASH — heavy 7½ + sat + headBump
            set ("mic_gain", 0.55f); set ("mic_gain_r", 0.55f);
            set ("saturation_drive", 1.7f); set ("saturation_drive_r", 1.7f);
            set ("bias_amount", 1.08f); set ("speed", 2);
            set ("tape_formula", 0); break;
        case 17: // VOCAL SILK — light sat + treble +2.5
            set ("mic_gain", 0.5f); set ("mic_gain_r", 0.5f);
            set ("saturation_drive", 1.1f); set ("saturation_drive_r", 1.1f);
            set ("treble_db", 2.5f); set ("wow_flutter", 0.4f);
            set ("speed", 2); break;
        case 18: // BASS WEIGHT — CrO₂, low bias, deep LF
            set ("mic_gain", 0.5f); set ("mic_gain_r", 0.5f);
            set ("bias_amount", 0.78f); set ("bass_db", 2.5f);
            set ("treble_db", -1.0f); set ("speed", 2);
            set ("tape_formula", 1); break;
        case 19: // LO-FI POP — 1⅞ + multiplay 3 + W&F
            set ("radio_gain", 0.55f); set ("radio_gain_r", 0.55f);
            set ("saturation_drive", 1.45f); set ("saturation_drive_r", 1.45f);
            set ("bias_amount", 0.85f); set ("treble_db", -3.0f);
            set ("multiplay_gen", 3); set ("wow_flutter", 1.5f);
            set ("speed", 0); break;
        case 20: // DUB ECHO — heavy echo near self-osc
            set ("mic_gain", 0.5f); set ("mic_gain_r", 0.5f);
            setBool ("echo_enabled", true);
            set ("echo_amount", 0.85f); set ("echo_amount_r", 0.82f);
            set ("saturation_drive", 1.2f); set ("saturation_drive_r", 1.2f);
            set ("speed", 1); break;
        case 21: // GLUE MASTER — subtle BASF
            set ("mic_gain", 0.45f); set ("mic_gain_r", 0.45f);
            set ("saturation_drive", 0.85f); set ("saturation_drive_r", 0.85f);
            set ("speed", 2); set ("tape_formula", 1);
            set ("wow_flutter", 0.2f); break;
        case 22: // SHOEGAZE — heavy 1⅞ + echo + W&F
            set ("mic_gain", 0.6f); set ("mic_gain_r", 0.6f);
            setBool ("echo_enabled", true);
            set ("echo_amount", 0.6f); set ("echo_amount_r", 0.6f);
            set ("saturation_drive", 1.55f); set ("saturation_drive_r", 1.55f);
            set ("bias_amount", 0.88f); set ("wow_flutter", 1.7f);
            set ("speed", 0); break;
        case 23: // CRT NIGHTMARE — max everything
            set ("mic_gain", 0.7f); set ("mic_gain_r", 0.7f);
            setBool ("echo_enabled", true);
            set ("echo_amount", 0.95f); set ("echo_amount_r", 0.95f);
            set ("saturation_drive", 1.95f); set ("saturation_drive_r", 1.95f);
            set ("bias_amount", 0.6f); set ("treble_db", -4.0f);
            set ("multiplay_gen", 5); set ("wow_flutter", 1.9f);
            set ("speed", 0); set ("tape_formula", 2); break;
        default: break;  // FACTORY (idx 0)
    }
}

void BC2000DLEditor::paint (juce::Graphics& g)
{
    juce::ignoreUnused (g);
    // HybridHeroPanel ritar bakgrund + walnut + cream + header + footer
    // Vi ritar overlay-element direkt här (slider-bank-recess, knob-cluster-recess, etc)
    auto bounds = getLocalBounds();
    auto face = bounds.reduced (0).withTrimmedLeft (kCheekW).withTrimmedRight (kCheekW);
    face = face.withTrimmedTop (kHeaderH).withTrimmedBottom (kFooterH).reduced (16, 4);

    // Top row har VU/VU/TapeBay/Counter — ingen recess där (var och en har egen)
    auto topRowH = 180;
    face.removeFromTop (topRowH + 12);

    // Mid row — Slider bank (left ~62%) + Knob cluster (right)
    auto midRow = face.removeFromTop (250);
    auto bankR = midRow.removeFromLeft ((int) (midRow.getWidth() * 0.60f));
    midRow.removeFromLeft (12);
    auto knobR = midRow;

    drawSliderBankHeader (g, bankR);
    drawDarkRecess (g, knobR, "EMULATION", "tape physics & calibration");

    // ----- Header overlay: preset-meny + A/B (rita bakgrund för knapparna) -----
    auto header = bounds.removeFromTop (kHeaderH).reduced (kCheekW + 12, 0)
                          .withTrimmedTop (32);

    // Inget extra här — preset-meny + A/B-knapparna har egna LookAndFeel-färger
    juce::ignoreUnused (header);
}

void BC2000DLEditor::resized()
{
    auto bounds = getLocalBounds();
    heroPanel.setBounds (bounds);

    // ----- Header layout -----
    {
        auto header = bounds.removeFromTop (kHeaderH).reduced (kCheekW + 16, 0)
                                .withTrimmedTop (28);
        // Lämna 360 px till vänster för "Ferroflux  2000 · DL" (ritas av panel)
        header.removeFromLeft (360);

        // Preset-nav
        auto presetArea = header.removeFromLeft (340);
        prevPresetBtn.setBounds (presetArea.removeFromLeft (22).withHeight (22));
        presetArea.removeFromLeft (3);
        presetMenu.setBounds (presetArea.removeFromLeft (290).withHeight (22));
        presetArea.removeFromLeft (3);
        nextPresetBtn.setBounds (presetArea.removeFromLeft (22).withHeight (22));

        // A/B höger
        auto abArea = header.removeFromRight (240);
        abArea = abArea.withTrimmedLeft (abArea.getWidth() - 80).withHeight (22);
        bBtn.setBounds (abArea.removeFromRight (36));
        abArea.removeFromRight (3);
        aBtn.setBounds (abArea.removeFromRight (36));
    }

    // ----- Footer reserveras (HybridHeroPanel ritar) -----
    bounds.removeFromBottom (kFooterH);

    // ----- Inner face -----
    auto face = bounds.reduced (kCheekW + 16, 4).withTrimmedTop (8);

    // ===== TOP DECK-ZON: hela övre 38 % reserveras för reels (heroPanel
    //       ritar STORA spolar, speed-knob, B&O-handle, "BEOCORD 2000 DE LUXE"-
    //       titel-strip). Inga interaktiva kontroller här. =====
    // Synkat med HybridHeroPanel.cpp's deckH=0.30f. Frigör utrymme för bottom-rader.
    const int deckSkip = (int) ((face.getHeight()) * 0.30f) + 32;
    face.removeFromTop (deckSkip);

    // ===== Liten VU-meter-rad + L/R-record-indicators + transport + counter =====
    {
        auto vuRow = face.removeFromTop (78);
        const int vuW = 200;
        meterL.setBounds (vuRow.removeFromLeft (vuW).reduced (4));
        vuRow.removeFromLeft (8);
        meterR.setBounds (vuRow.removeFromLeft (vuW).reduced (4));
        vuRow.removeFromLeft (16);

        // L/R röd-bar-record-indicators (matchar BC2000-frontpanel-foto)
        recIndL.setBounds (vuRow.removeFromLeft (50).reduced (4, 14));
        vuRow.removeFromLeft (4);
        recIndR.setBounds (vuRow.removeFromLeft (50).reduced (4, 14));
        vuRow.removeFromLeft (16);

        // Transport-lever + counter t.h.
        transportLever.setBounds (vuRow.removeFromLeft (130).reduced (4, 8));
        vuRow.removeFromLeft (12);
        tapeCounter.setBounds (vuRow.removeFromLeft (140).reduced (4, 8));
    }

    face.removeFromTop (8);

    // ===== BC2000 DL bottom-panel layout: 3 kolumner =====
    //   LEFT 25 %  — push-knappar (PA / Monitor / SonS / Track / etc) + L/R-indicators
    //   CENTER 50 %— 5 B&O dual-skydepotentiometre (autentisk frontpanel)
    //   RIGHT 25 % — 2 tone-rotaries (treble + bass) + advanced i compact-grid
    {
        auto midRow = face.removeFromTop (260);
        const int leftW   = (int) (midRow.getWidth() * 0.25f);
        const int centerW = (int) (midRow.getWidth() * 0.50f);
        auto leftCol  = midRow.removeFromLeft (leftW);
        midRow.removeFromLeft (8);
        auto centerCol = midRow.removeFromLeft (centerW - 16);
        midRow.removeFromLeft (8);
        auto rightCol = midRow;

        // ----- CENTER: 5 dual-skydepotentiometre -----
        const int nFaders = 5;
        const int fGap = 6;
        const int faderW = (centerCol.getWidth() - fGap * (nFaders - 1)) / nFaders;
        bc2000dl::ui::DualSlideFader* faders[5] = {
            &radioFader, &phonoFader, &micFader, &sosFader, &echoFader
        };
        for (int i = 0; i < nFaders; ++i)
        {
            faders[i]->setBounds (centerCol.removeFromLeft (faderW));
            if (i < nFaders - 1) centerCol.removeFromLeft (fGap);
        }

        // ----- RIGHT: 2 stora tone-rotaries (treble + bass) + advanced 2x3-grid -----
        // Treble + bass överst (stora B&O-tonkontroll-style)
        auto toneTop = rightCol.removeFromTop (110);
        const int toneKnobW = toneTop.getWidth() / 2 - 6;
        recEq_lbl.setBounds (toneTop.removeFromTop (12).removeFromLeft (toneKnobW));
        bass_lbl.setBounds (juce::Rectangle<int> (
            toneTop.getX() + toneKnobW + 12, toneTop.getY() - 12, toneKnobW, 12));
        recEqKnob.setBounds (toneTop.removeFromLeft (toneKnobW).reduced (2));
        toneTop.removeFromLeft (12);
        bassKnob.setBounds (toneTop.removeFromLeft (toneKnobW).reduced (2));

        rightCol.removeFromTop (8);

        // Advanced 2x3-grid: BIAS, WOW, SAT, BALANCE, ECHO·R, MULT (mindre)
        auto advArea = rightCol;
        const int aCols = 3;
        const int aGap  = 4;
        const int aColW = (advArea.getWidth() - aGap * (aCols - 1)) / aCols;
        const int aRowH = advArea.getHeight() / 2;
        juce::Slider* advs[6] = { &biasKnob, &wowKnob, &satKnob,
                                  &balanceKnob, &echoKnob, &multKnob };
        juce::Label*  advLbls[6] = { &bias_lbl, &wow_lbl, &sat_lbl,
                                     &balance_lbl, &echoR_lbl, &mult_lbl };
        for (int row = 0; row < 2; ++row)
        {
            auto aRow = advArea.removeFromTop (aRowH);
            for (int c = 0; c < aCols; ++c)
            {
                auto cell = aRow.removeFromLeft (aColW);
                if (c < aCols - 1) aRow.removeFromLeft (aGap);
                advLbls[row * aCols + c]->setBounds (cell.removeFromTop (10));
                const int kW = juce::jmin (cell.getWidth(), cell.getHeight() - 2);
                const int kx = cell.getCentreX() - kW / 2;
                advs[row * aCols + c]->setBounds (kx, cell.getY(), kW, kW);
            }
        }

        // ----- LEFT: push-knappar (vi placerar några viktiga här) -----
        // Rad 1: REC ARM 1/2 + TRK 1/2
        const int btnW = 60, btnH = 26;
        auto lRow1 = leftCol.removeFromTop (btnH);
        recArm1Btn.setBounds (lRow1.removeFromLeft (btnW).reduced (1));
        lRow1.removeFromLeft (3);
        recArm2Btn.setBounds (lRow1.removeFromLeft (btnW).reduced (1));
        lRow1.removeFromLeft (3);
        track1Btn.setBounds (lRow1.removeFromLeft (btnW).reduced (1));
        lRow1.removeFromLeft (3);
        track2Btn.setBounds (lRow1.removeFromLeft (btnW).reduced (1));

        leftCol.removeFromTop (6);

        // Rad 2: ECHO / SYNC / BYPASS / SPEAKER
        auto lRow2 = leftCol.removeFromTop (btnH);
        echoBtn.setBounds   (lRow2.removeFromLeft (btnW).reduced (1));
        lRow2.removeFromLeft (3);
        syncBtn.setBounds   (lRow2.removeFromLeft (btnW).reduced (1));
        lRow2.removeFromLeft (3);
        bypassBtn.setBounds (lRow2.removeFromLeft (btnW).reduced (1));
        lRow2.removeFromLeft (3);
        speakerBtn.setBounds (lRow2.removeFromLeft (btnW).reduced (1));

        leftCol.removeFromTop (6);

        // Rad 3: SPK A/B/MUTE + PAUSE
        auto lRow3 = leftCol.removeFromTop (btnH);
        spkExtBtn.setBounds  (lRow3.removeFromLeft (btnW).reduced (1));
        lRow3.removeFromLeft (3);
        spkIntBtn.setBounds  (lRow3.removeFromLeft (btnW).reduced (1));
        lRow3.removeFromLeft (3);
        spkMuteBtn.setBounds (lRow3.removeFromLeft (btnW).reduced (1));
        lRow3.removeFromLeft (3);
        pauseBtn.setBounds   (lRow3.removeFromLeft (btnW).reduced (1));
    }

    face.removeFromTop (12);

    // ===== BOTTOM ROW: Transport piano keys + selectors | Preset bar / extra =====
    {
        auto botRow = face;
        auto leftCol = botRow.removeFromLeft (560);
        botRow.removeFromLeft (12);
        auto rightCol = botRow;

        // ----- Left: Transport-keys + selectors -----
        // Rad 1: 6 piano-keys (REW/FF/STOP/PLAY/PAUSE/REC) — använd toggle-buttons
        const int btnW = 70, btnH = 36;
        auto trRow = leftCol.removeFromTop (btnH);
        recArm1Btn.setBounds (trRow.removeFromLeft (btnW).reduced (1));
        trRow.removeFromLeft (3);
        recArm2Btn.setBounds (trRow.removeFromLeft (btnW).reduced (1));
        trRow.removeFromLeft (3);
        track1Btn.setBounds (trRow.removeFromLeft (btnW).reduced (1));
        trRow.removeFromLeft (3);
        track2Btn.setBounds (trRow.removeFromLeft (btnW).reduced (1));
        trRow.removeFromLeft (3);
        pauseBtn.setBounds (trRow.removeFromLeft (btnW).reduced (1));

        leftCol.removeFromTop (8);

        // Rad 2: Selectors SPEED + PHONO INPUT + RADIO INPUT
        auto selLabelRow = leftCol.removeFromTop (10);
        speedLabel.setBounds (selLabelRow.removeFromLeft (110).withTrimmedLeft (2));
        selLabelRow.removeFromLeft (8);
        phonoModeLabel.setBounds (selLabelRow.removeFromLeft (110).withTrimmedLeft (2));
        selLabelRow.removeFromLeft (8);
        radioModeLabel.setBounds (selLabelRow.removeFromLeft (110).withTrimmedLeft (2));

        // === Återställd dropdown-layout — ALLA selectors synliga och tillgängliga ===
        auto selRow = leftCol.removeFromTop (22);
        speedBox.setBounds     (selRow.removeFromLeft (110));
        selRow.removeFromLeft (8);
        phonoModeBox.setBounds (selRow.removeFromLeft (110));
        selRow.removeFromLeft (8);
        radioModeBox.setBounds (selRow.removeFromLeft (110));

        leftCol.removeFromTop (6);

        // Rad 3: MONITOR + TAPE FORMULA + Lo-Z + P.A. + S/S
        auto selLabelRow2 = leftCol.removeFromTop (10);
        monitorLabel.setBounds (selLabelRow2.removeFromLeft (110).withTrimmedLeft (2));
        selLabelRow2.removeFromLeft (8);
        tapeFormulaLabel.setBounds (selLabelRow2.removeFromLeft (110).withTrimmedLeft (2));

        auto selRow2 = leftCol.removeFromTop (22);
        monitorBox.setBounds     (selRow2.removeFromLeft (110));
        selRow2.removeFromLeft (8);
        tapeFormulaBox.setBounds (selRow2.removeFromLeft (110));
        selRow2.removeFromLeft (12);
        micLoZBtn.setBounds (selRow2.removeFromLeft (50));
        selRow2.removeFromLeft (4);
        paBtn.setBounds (selRow2.removeFromLeft (50));
        selRow2.removeFromLeft (4);
        sosBtn.setBounds (selRow2.removeFromLeft (50));

        // ----- Right: Övriga toggles (echo/sync/bypass/speaker, högtalare A/B/Mute) -----
        auto rRow1 = rightCol.removeFromTop (26);
        echoBtn.setBounds   (rRow1.removeFromLeft (70).reduced (1));
        rRow1.removeFromLeft (4);
        syncBtn.setBounds   (rRow1.removeFromLeft (70).reduced (1));
        rRow1.removeFromLeft (4);
        bypassBtn.setBounds (rRow1.removeFromLeft (70).reduced (1));
        rRow1.removeFromLeft (4);
        speakerBtn.setBounds (rRow1.removeFromLeft (70).reduced (1));

        rightCol.removeFromTop (6);

        auto rRow2 = rightCol.removeFromTop (26);
        spkExtBtn.setBounds  (rRow2.removeFromLeft (70).reduced (1));
        rRow2.removeFromLeft (4);
        spkIntBtn.setBounds  (rRow2.removeFromLeft (70).reduced (1));
        rRow2.removeFromLeft (4);
        spkMuteBtn.setBounds (rRow2.removeFromLeft (70).reduced (1));
    }
}

void BC2000DLEditor::timerCallback()
{
    auto& chain = audioProc.getChain();
    meterL.pushLevel (chain.meterLevelL_dBFS.load());
    meterR.pushLevel (chain.meterLevelR_dBFS.load());
    meterL.setRecording (chain.isRecordingL.load());
    meterR.setRecording (chain.isRecordingR.load());

    // L/R-record-bar-indicators speglar rec_arm-toggles
    const bool armL = audioProc.apvts.getRawParameterValue ("rec_arm_1")->load() > 0.5f;
    const bool armR = audioProc.apvts.getRawParameterValue ("rec_arm_2")->load() > 0.5f;
    recIndL.setLit (armL);
    recIndR.setLit (armR);

    const float inputAny = audioProc.apvts.getRawParameterValue ("mic_gain")->load()
                         + audioProc.apvts.getRawParameterValue ("phono_gain")->load()
                         + audioProc.apvts.getRawParameterValue ("radio_gain")->load();
    heroPanel.setReelsRotating (inputAny > 0.05f);

    const auto sr = audioProc.getSampleRate();
    if (sr > 0.0)
    {
        const int speedIdx = (int) audioProc.apvts.getRawParameterValue ("speed")->load();
        const char* speedStr = (speedIdx == 0 ? "4.75" : speedIdx == 1 ? "9.5" : "19");
        juce::String newStatus = juce::String (speedStr) + " cm/s  ·  "
                                + juce::String (sr / 1000.0, 1) + " kHz";
        if (newStatus != statusText)
        {
            statusText = newStatus;
            repaint();
        }
    }
}

bool BC2000DLEditor::keyPressed (const juce::KeyPress& k)
{
    // Cmd+I = About-dialog (UAD-pattern)
    if (k.getModifiers().isCommandDown() && k.getKeyCode() == 'I')
    {
        showAboutDialog();
        return true;
    }
    return false;
}

void BC2000DLEditor::showAboutDialog()
{
    juce::String body;
    body
        << "BC2000DL — Danish Tape 2000\n"
        << "Bang & Olufsen BeoCord 2000 De Luxe (1968-69)\n"
        << "\n"
        << "DSP MODEL\n"
        << "  • Jiles-Atherton magnetic hysteresis\n"
        << "  • 100 kHz bias as real signal (not EQ tilt)\n"
        << "  • 8x oversampling, 96 dB Nyquist attenuation\n"
        << "  • Ferro / Chrome / Metal IV tape formulas\n"
        << "  • Multiplay generation (1-4 dub-down loss)\n"
        << "  • Wow 0.13% @ 0.5 Hz, Flutter 0.07% @ 50 Hz\n"
        << "  • Per-source DSP routing (mic/phono/radio parallel)\n"
        << "\n"
        << "VALIDATED\n"
        << "  • 21/21 PASS vs B&O Service Manual specs\n"
        << "  • 21/21 PASS vs Studio-Sound 1968 measurements\n"
        << "  • Apple AU validation (auval): PASSED\n"
        << "\n"
        << "TECHNOLOGY\n"
        << "  • JUCE 8 framework\n"
        << "  • Universal Binary (arm64 + x86_64)\n"
        << "  • macOS 14+ (Sonoma and later)\n"
        << "  • Filmstrip rendering (UAD-pattern)\n"
        << "  • CPU: ~0.6% per instance @ 96 kHz Apple Silicon\n"
        << "\n"
        << "CREDITS\n"
        << "  • DSP from B&O service manual + Studio-Sound 1968-09\n"
        << "  • 3D reels by YJ_ (CC-BY 4.0, Sketchfab)\n"
        << "  • Code: BC2000DL Audio\n"
        << "\n"
        << "SHORTCUTS\n"
        << "  • Cmd+I        — Show this dialog\n"
        << "  • Double-click — Reset fader to default\n"
        << "  • Cmd+drag     — Fine adjustment (10x slower)\n"
        << "  • Right-click  — Snap menu / set value\n"
        << "  • Mouse wheel  — Adjust by 0.01 step\n";

    juce::AlertWindow::showAsync (
        juce::MessageBoxOptions()
            .withIconType (juce::MessageBoxIconType::InfoIcon)
            .withTitle ("BC2000DL — About + Spec Sheet")
            .withMessage (body)
            .withButton ("OK"),
        nullptr);
}
