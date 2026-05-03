/*  NativeEditor — clean-slate native JUCE-UI för BC2000DL v29.8.

    Filosofi:
      - Bara JUCE-primitives (Slider, TextButton, ComboBox)
      - Ingen custom LookAndFeel, ingen 3D, inga needle-physics
      - Modern flat design: dark bg, clear typography, predictable behavior
      - Allt funkar — DSP är validerad (21/21 PASS), UX är ny.

    Filen ersätter både PluginEditor.cpp (gammal native) och WebEditor.cpp (laggig WebView).
*/

#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

namespace bc2000dl
{
    // -------- Liten VU-bar-komponent (horisontell, gradient L→R) --------
    class VUBar : public juce::Component
    {
    public:
        VUBar() = default;

        void setLevel (float dbfs)
        {
            // Smooth attack/release: snabb upp, långsam ner (12 dB/s)
            const auto target = juce::jlimit (-60.0f, 6.0f, dbfs);
            if (target > current)
                current = target;                              // attack: instant
            else
                current = juce::jmax (target, current - 0.5f); // release: 0.5 dB/frame @ 30 Hz = 15 dB/s
            repaint();
        }

        void setRecording (bool r) { if (r != recording) { recording = r; repaint(); } }

        void paint (juce::Graphics& g) override
        {
            const auto r = getLocalBounds().toFloat();

            // Bakgrund (recess)
            g.setColour (juce::Colour (0xff0d0d0f));
            g.fillRoundedRectangle (r, 3.0f);
            g.setColour (juce::Colour (0xff222226));
            g.drawRoundedRectangle (r, 3.0f, 1.0f);

            // Bar-fyllning (mappa -60..+6 dBFS → 0..1 av bredden)
            const float pos = juce::jlimit (0.0f, 1.0f, (current + 60.0f) / 66.0f);
            if (pos > 0.005f)
            {
                auto bar = r.reduced (2.0f);
                bar.setWidth (bar.getWidth() * pos);

                // Tre-zons gradient: grön → gul → röd
                juce::ColourGradient grad (
                    juce::Colour (0xff3ad07a), bar.getX(),     bar.getCentreY(),
                    juce::Colour (0xffd03a3a), bar.getRight(), bar.getCentreY(), false);
                grad.addColour (0.7,  juce::Colour (0xffd0c43a)); // gul vid -18 dBFS
                grad.addColour (0.85, juce::Colour (0xffd07a3a)); // orange vid -8 dBFS
                g.setGradientFill (grad);
                g.fillRoundedRectangle (bar, 2.0f);
            }

            // dB-ticks: -40, -20, -10, -3, 0
            g.setColour (juce::Colours::white.withAlpha (0.25f));
            for (int db : { -40, -20, -10, -3, 0 })
            {
                const float x = r.getX() + r.getWidth() * (db + 60.0f) / 66.0f;
                g.drawLine (x, r.getY() + 2, x, r.getBottom() - 2, 0.5f);
            }

            // Numerisk readout
            g.setColour (juce::Colours::white.withAlpha (0.85f));
            g.setFont (juce::FontOptions (10.0f));
            g.drawText (juce::String (current, 1) + " dBFS",
                        r.reduced (6, 0), juce::Justification::centredRight, false);

            // REC-blink
            if (recording)
            {
                g.setColour (juce::Colour (0xffd03a3a));
                g.fillEllipse (r.getRight() - 16, r.getY() + 4, 6, 6);
            }
        }

    private:
        float current { -60.0f };
        bool recording { false };
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
    juce::TooltipWindow tooltipWindow { this, 500 };

    // ---- VU meters ----
    bc2000dl::VUBar vuL, vuR;
    juce::Label vuL_lbl { {}, "L" }, vuR_lbl { {}, "R" };

    // ---- 5 dual-faders (10 sliders) ----
    struct DualFader {
        juce::Slider l, r;
        juce::Label  caption, lLbl, rLbl;
    };
    DualFader radio, phono, mic, drive, echo;

    // ---- 7 knobs ----
    juce::Slider knob_bias, knob_bass, knob_treble, knob_wow, knob_mult, knob_balance, knob_master;
    juce::Label  lbl_bias, lbl_bass, lbl_treble, lbl_wow, lbl_mult, lbl_balance, lbl_master;

    // ---- 5 selectors ----
    juce::ComboBox cb_speed, cb_monitor, cb_phono, cb_radio, cb_formula;
    juce::Label    lbl_speed, lbl_monitor, lbl_phono, lbl_radio, lbl_formula;

    // ---- 16 toggle buttons ----
    juce::TextButton btn_echo, btn_bypass, btn_speaker, btn_sync;
    juce::TextButton btn_loz, btn_pa, btn_sos, btn_pause;
    juce::TextButton btn_rec1, btn_rec2, btn_trk1, btn_trk2;
    juce::TextButton btn_spkA, btn_spkB, btn_mute;

    // ---- Header ----
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

    // ---- Statusrad ----
    juce::String statusText;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NativeEditor)
};
