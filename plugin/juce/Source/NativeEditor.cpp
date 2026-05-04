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
#include <melatonin_blur/melatonin_blur.h>

namespace
{
    using LnF = bc2000dl::ui::InstructionCardLnF;

    constexpr int kEditorW    = 1220;
    constexpr int kEditorH    = 482;    // even tighter — full horizontal strip
    constexpr int kTeakW      = 32;     // wood end-cap width (each side)
    constexpr int kInnerW     = kEditorW - 2 * kTeakW;
    constexpr int kAluH       = 156;    // black-metal deck zone height
    constexpr int kDivH       = 3;      // metallic divider height
    constexpr int kPresetH    = 40;     // preset/nav bar height
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
        if (id == "speaker_mute")      return "Mute alla högtalare";
        if (id == "stereo_asymmetry")  return "L/R Ge-stage mismatch · 0 = symmetric · 0.02 = 1968 authentic";
        return {};
    }
}

//=============================================================================
//  ReelDeck — two animated bullseye reels + head assembly
//=============================================================================
namespace bc2000dl
{
    void ReelDeck::onVBlank()
    {
        if (! isActive)
        {
            // Decay angular velocity for motion-blur falloff
            angVelL *= 0.85f;
            angVelR *= 0.85f;
            if (angVelL < 0.001f && angVelR < 0.001f) return;
            repaint();
            return;
        }

        // Real tape physics:
        //  - linearSpeed (cm/s) is constant for a given tape speed
        //  - angularSpeed = linearSpeed / currentRadius
        //  - so as supply reel shrinks → it spins faster
        //    and as takeup reel grows → it spins slower
        const float linearSpeed = 0.02f * speedFactor;       // arbitrary unit
        const float minR = 0.32f, maxR = 1.0f;               // normalised reel radii
        const float supplyR = juce::jmap (1.0f - tapeAmount, minR, maxR);
        const float takeupR = juce::jmap (tapeAmount, minR, maxR);

        angVelL = linearSpeed / supplyR;        // supply reel — feeds tape (CW)
        angVelR = linearSpeed / takeupR;        // takeup reel — winds tape (CCW visually)

        angleL += angVelL;
        angleR += angVelR;
        const auto twoPi = juce::MathConstants<float>::twoPi;
        if (angleL > twoPi)  angleL -= twoPi;
        if (angleR > twoPi)  angleR -= twoPi;

        repaint();
    }

    void ReelDeck::setTapePosition (double posSeconds)
    {
        // Standard C60 tape at 19 cm/s fills 1800 s (30 min per side).
        // We model the reel as cycling every 90 minutes so the visual never
        // "stalls" at the end — matches real-world session workflow.
        constexpr double kCycleSecs = 5400.0;   // 90 min
        const double wrapped = std::fmod (posSeconds, kCycleSecs);
        tapeAmount = static_cast<float> (wrapped / kCycleSecs);
    }

    void ReelDeck::paint (juce::Graphics& g)
    {
        auto bounds = getLocalBounds();

        const int reelDiam = juce::jmin (bounds.getHeight() - 6, (int) (bounds.getWidth() * 0.36f));
        const int reelY    = bounds.getCentreY() - reelDiam / 2;

        const juce::Rectangle<int> leftReel  (bounds.getX(),                  reelY, reelDiam, reelDiam);
        const juce::Rectangle<int> rightReel (bounds.getRight() - reelDiam,   reelY, reelDiam, reelDiam);

        // Motion-blur scales with angular velocity (peaks at small reels)
        // typical max angVel ≈ 0.06 → motionAmount ≈ 1.0
        const float supplyFill = 1.0f - tapeAmount;
        const float takeupFill = tapeAmount;
        const float motionL = juce::jlimit (0.0f, 1.0f, angVelL * 18.0f);
        const float motionR = juce::jlimit (0.0f, 1.0f, angVelR * 18.0f);

        ui::InstructionCardLnF::drawReel (g, leftReel,  angleL,         isActive, supplyFill, motionL, wowIntensity);
        ui::InstructionCardLnF::drawReel (g, rightReel, -angleR * 1.0f, isActive, takeupFill, motionR, wowIntensity);

        // (tape path removed — cleaner look)
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

        // Melatonin InnerShadow — depth ring that makes the face look recessed
        {
            juce::Path facePath;
            facePath.addRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 3.0f);
            static thread_local melatonin::InnerShadow vuInner {
                { juce::Colours::black.withAlpha (0.45f), 5, { 0, 3 } }
            };
            vuInner.render (g, facePath);
        }

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

    //=========================================================================
    //  Spectrum analyser — pulls samples from chain FIFO, FFT, smooth render
    //=========================================================================
    void SpectrumAnalyser::timerCallback()
    {
        if (srcBuffer == nullptr || srcWriteIdx == nullptr || srcSize <= 0)
            return;

        // Copy the most recent kFftSize samples (in chronological order) into
        // fftData, applying a Hann window.
        const int writeIdx = srcWriteIdx->load (std::memory_order_acquire);
        const int start = (writeIdx - kFftSize + srcSize) & (srcSize - 1);
        for (int i = 0; i < kFftSize; ++i)
            fftData[i] = srcBuffer[(start + i) & (srcSize - 1)];

        std::fill (fftData + kFftSize, fftData + kFftSize * 2, 0.0f);
        window.multiplyWithWindowingTable (fftData, kFftSize);
        forwardFFT.performFrequencyOnlyForwardTransform (fftData);

        // Map FFT bins to our N display bins logarithmically (50 Hz - 20 kHz).
        const int halfFFT = kFftSize / 2;
        for (int b = 0; b < kNumBins; ++b)
        {
            const float t = (float) b / (float) (kNumBins - 1);
            const float minIdx = std::log10 (4.0f);                  // ~50 Hz
            const float maxIdx = std::log10 ((float) halfFFT);
            const float idx = std::pow (10.0f, minIdx + t * (maxIdx - minIdx));
            const int   i0  = juce::jlimit (0, halfFFT - 1, (int) std::floor (idx));
            const int   i1  = juce::jlimit (0, halfFFT - 1, i0 + 1);
            const float frac = idx - (float) i0;
            const float mag = juce::jmap (fftData[i0] + frac * (fftData[i1] - fftData[i0]),
                                            0.0f, (float) kFftSize, 0.0f, 1.0f);
            // Convert to dB and smooth (peak-hold style)
            const float db = juce::Decibels::gainToDecibels (mag, -90.0f);
            const float v  = juce::jlimit (0.0f, 1.0f, (db + 80.0f) / 80.0f);
            // Smooth attack-fast / release-slow
            magnitudes[b] = v > magnitudes[b]
                              ? magnitudes[b] + (v - magnitudes[b]) * 0.50f
                              : magnitudes[b] + (v - magnitudes[b]) * 0.10f;
        }
        repaint();
    }

    void SpectrumAnalyser::paint (juce::Graphics& g)
    {
        const auto r = getLocalBounds().toFloat();

        // Build a smooth path from bin magnitudes
        juce::Path curve;
        curve.startNewSubPath (r.getX(), r.getBottom());
        for (int b = 0; b < kNumBins; ++b)
        {
            const float x = r.getX() + (r.getWidth() * (float) b / (float) (kNumBins - 1));
            const float y = r.getBottom() - r.getHeight() * magnitudes[b];
            curve.lineTo (x, y);
        }
        curve.lineTo (r.getRight(), r.getBottom());
        curve.closeSubPath();

        // Filled glow gradient (amber, fades down)
        juce::ColourGradient grad (
            juce::Colour (0xFFE8B040).withAlpha (0.55f), r.getCentreX(), r.getY(),
            juce::Colour (0xFFE8B040).withAlpha (0.05f), r.getCentreX(), r.getBottom(), false);
        g.setGradientFill (grad);
        g.fillPath (curve);

        // Stroke the curve on top with brighter amber
        juce::Path stroke;
        stroke.startNewSubPath (r.getX(), r.getBottom());
        for (int b = 0; b < kNumBins; ++b)
        {
            const float x = r.getX() + (r.getWidth() * (float) b / (float) (kNumBins - 1));
            const float y = r.getBottom() - r.getHeight() * magnitudes[b];
            if (b == 0) stroke.startNewSubPath (x, y);
            else        stroke.lineTo (x, y);
        }
        // Real Gaussian glow under the line (melatonin)
        static thread_local melatonin::DropShadow specGlow {
            { juce::Colour (0xFFE8B040).withAlpha (0.85f), 6, { 0, 0 }, 1 }
        };
        specGlow.render (g, stroke, juce::PathStrokeType (1.4f));

        // Crisp top line
        g.setColour (juce::Colour (0xFFFFD080));
        g.strokePath (stroke, juce::PathStrokeType (1.2f));

        // Frequency ruler — three amber tick-labels at bottom edge.
        // Positions assume 44.1 kHz SR (log scale 86 Hz .. 22 kHz);
        // they read well at 48 kHz too (shift is < 5 % visual).
        {
            struct FLbl { float t; const char* text; };
            constexpr FLbl freqLbls[] = { { 0.03f, "100" }, { 0.44f, "1k" }, { 0.86f, "10k" } };
            g.setFont (bc2000dl::ui::InstructionCardLnF::sectionFont (5.5f));
            for (const auto& fl : freqLbls)
            {
                const float x = r.getX() + r.getWidth() * fl.t;
                // faint tick
                g.setColour (juce::Colour (0xFFE8B040).withAlpha (0.28f));
                g.drawLine (x, r.getBottom() - 4.0f, x, r.getBottom(), 0.6f);
                // amber label
                g.setColour (juce::Colour (0xFFE8B040).withAlpha (0.48f));
                g.drawText (fl.text,
                    juce::Rectangle<float> (x - 7.0f, r.getBottom() - 8.5f, 14.0f, 8.0f)
                        .toNearestInt(),
                    juce::Justification::centred, false);
            }
        }
    }
}

//=============================================================================
//  NativeEditor
//=============================================================================
NativeEditor::NativeEditor (BC2000DLProcessor& p)
    : juce::AudioProcessorEditor (p), audioProc (p)
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
    // Preset bar: subtle drop-shadow → preset strip "floats" above panel
    presetShadow.setShadowProperties (
        juce::DropShadow (juce::Colours::black.withAlpha (0.50f), 5, { 0, 2 }));

    vuInL.setComponentEffect (&vuShadow);
    vuInR.setComponentEffect (&vuShadow);
    vuOut.setComponentEffect (&vuShadow);
    reelDeck.setComponentEffect (&reelShadow);

    addAndMakeVisible (reelDeck);
    addAndMakeVisible (vuInL);
    addAndMakeVisible (vuInR);
    addAndMakeVisible (vuOut);

    // ---- Spectrum analyser (live FFT strip on deck) ----
    spectrum.setSource (audioProc.getChain().spectrumBuffer,
                         &audioProc.getChain().spectrumWriteIdx,
                         bc2000dl::dsp::SignalChain::kSpecBufSize);
    spectrum.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (spectrum);

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
            // Scroll wheel: 0.04 per notch normal, Shift held → fine 0.005
            sl->setRotaryParameters (0.0f, juce::MathConstants<float>::twoPi, true);
            sl->addMouseListener (&sliderMenu, false);
            addAndMakeVisible (sl);
        }
        f.l.setTooltip (tooltipFor (idL));
        f.r.setTooltip (tooltipFor (idR));
        sAtts.push_back (std::make_unique<SAtt> (audioProc.apvts, idL, f.l));
        sAtts.push_back (std::make_unique<SAtt> (audioProc.apvts, idR, f.r));

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
        s.addMouseListener (&sliderMenu, false);
        addAndMakeVisible (s);
        sAtts.push_back (std::make_unique<SAtt> (audioProc.apvts, id, s));
        creamLbl (l, cap);
        l.setInterceptsMouseClicks (false, false);
    };
    setupKnob (knob_treble,  lbl_treble,  "treble_db",        "TREBLE dB");
    setupKnob (knob_bass,    lbl_bass,    "bass_db",          "BASS dB");
    setupKnob (knob_balance, lbl_balance, "balance",          "BAL L\xe2\x86\x94R");
    setupKnob (knob_asym,    lbl_asym,    "stereo_asymmetry", "ASYM");
    setupKnob (knob_bias,    lbl_bias,    "bias_amount",      "BIAS %");
    setupKnob (knob_wow,     lbl_wow,     "wow_flutter",      "WOW %");
    setupKnob (knob_mult,    lbl_mult,    "multiplay_gen",    "MULT x");
    setupKnob (knob_master,  lbl_master,  "master_volume",    "VOL dB");

    // Engineering-unit popup readout for every knob.
    // textFromValueFunction is a public std::function on juce::Slider (JUCE 7+).
    knob_asym.textFromValueFunction = [] (double v)
    {
        if (v < 0.001)  return juce::String ("symmetric");
        return juce::String (v * 100.0, 1) + "% mismatch";
    };

    knob_treble.textFromValueFunction  = [] (double v)
        { return (v >= 0 ? "+" : "") + juce::String (v, 1) + " dB"; };
    knob_bass.textFromValueFunction    = [] (double v)
        { return (v >= 0 ? "+" : "") + juce::String (v, 1) + " dB"; };
    knob_balance.textFromValueFunction = [] (double v)
    {
        if (std::abs (v) < 0.01)  return juce::String ("Centre");
        return juce::String (int (std::abs (v) * 100.0 + 0.5)) + (v < 0.0 ? "% L" : "% R");
    };
    knob_bias.textFromValueFunction    = [] (double v)
    {
        const double pct = (v - 1.0) * 100.0;
        if (std::abs (pct) < 0.5)  return juce::String ("nominal");
        return (pct >= 0 ? "+" : "") + juce::String (pct, 0) + "% bias";
    };
    knob_wow.textFromValueFunction     = [] (double v)
        { return juce::String (v * 100.0, 0) + "% W+F"; };
    knob_mult.textFromValueFunction    = [] (double v)
        { return "gen " + juce::String (juce::roundToInt (v)); };
    knob_master.textFromValueFunction  = [] (double v)
    {
        if (v < 0.001)  return juce::String ("-inf dB");
        return juce::String (20.0 * std::log10 (v), 1) + " dBFS";
    };

    // Double-click resets to the hardware-default position + marks the detent.
    knob_asym.setDoubleClickReturnValue    (true, 0.02);   // 1968 authentic default
    knob_treble.setDoubleClickReturnValue  (true, 0.0);
    knob_bass.setDoubleClickReturnValue    (true, 0.0);
    knob_balance.setDoubleClickReturnValue (true, 0.0);
    knob_bias.setDoubleClickReturnValue    (true, 1.0);
    knob_wow.setDoubleClickReturnValue     (true, 0.3);
    knob_mult.setDoubleClickReturnValue    (true, 1.0);
    knob_master.setDoubleClickReturnValue  (true, 0.85);

    // ---- 5 selectors ----
    auto setupCombo = [&] (juce::ComboBox& c, juce::Label& l, const juce::String& id,
                            const juce::String& cap,
                            std::initializer_list<const char*> items)
    {
        int idx = 1;
        for (auto* it : items) c.addItem (it, idx++);
        c.setTooltip (tooltipFor (id));
        addAndMakeVisible (c);
        cAtts.push_back (std::make_unique<CAtt> (audioProc.apvts, id, c));

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
        bAtts.push_back (std::make_unique<BAtt> (audioProc.apvts, id, b));
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
        bAtts.push_back (std::make_unique<BAtt> (audioProc.apvts, id, b));
    };
    setupKey (k_rec1, "REC 1", "rec_arm_1");
    setupKey (k_rec2, "REC 2", "rec_arm_2");
    setupKey (k_trk1, "TRK 1", "track_1");
    setupKey (k_trk2, "TRK 2", "track_2");
    setupKey (k_spkA, "SPK A", "speaker_ext");
    setupKey (k_spkB, "SPK B", "speaker_int");
    setupKey (k_mute, "MUTE",  "speaker_mute");

    // ---- Preset header — UAD-style browser ----
    {
        // Preset name button opens the floating browser overlay
        btn_preset_name.setButtonText ("FACTORY");
        btn_preset_name.onClick = [this]
        {
            presetBrowser.openBrowser (currentPresetIdx);
        };
        addAndMakeVisible (btn_preset_name);

        // Browser overlay: full-width below the preset bar
        presetBrowser.onPresetSelected = [this] (int idx)
        {
            currentPresetIdx = idx;
            btn_preset_name.setButtonText (juce::String (bc2000dl::kPresets[idx].name));
            applyPreset (idx);
        };
        addAndMakeVisible (presetBrowser);
        presetBrowser.setVisible (false);
    }

    auto setupNav = [&] (juce::TextButton& b, int delta)
    {
        addAndMakeVisible (b);
        b.onClick = [this, delta]
        {
            const int next = juce::jlimit (0, bc2000dl::kNumPresets - 1,
                                           currentPresetIdx + delta);
            currentPresetIdx = next;
            btn_preset_name.setButtonText (juce::String (bc2000dl::kPresets[next].name));
            applyPreset (next);
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
            store   = audioProc.apvts.copyState();
            slotIsA = isA;
            auto& load = slotIsA ? stateA : stateB;
            if (load.isValid()) audioProc.apvts.replaceState (load);
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
                .withTitle ("Beolux 2000 · v60.1")
                .withMessage ("BEOLUX 2000 — Danish Tape Emulation\n"
                              "by SOUNDBOYS\n\n"
                              "Inspired by the Bang & Olufsen Beocord 2000\n"
                              "De Luxe reel-to-reel (1968-69).\n\n"
                              "DSP: Jiles-Atherton tape hysteresis · 8× oversampling\n"
                              "Validated: pluginval VST3+AU (strict 5)\n"
                              "Signal stability: 67/67 PASS (signal_test.py)\n\n"
                              "UI: native JUCE · hardware-accurate aesthetic\n"
                              "Teak frame · black-metal deck · 3D bullseye reels\n"
                              "Analog VU meters · melatonin Gaussian shadows")
                .withButton ("OK"), nullptr);
    };
    addAndMakeVisible (btn_about);

    // Preset bar shadow (floats the bar above the black panel)
    for (auto* c : { static_cast<juce::Component*> (&btn_preset_name),
                     static_cast<juce::Component*> (&btn_prev),
                     static_cast<juce::Component*> (&btn_next),
                     static_cast<juce::Component*> (&btn_a),
                     static_cast<juce::Component*> (&btn_b),
                     static_cast<juce::Component*> (&btn_about) })
        c->setComponentEffect (&presetShadow);

    stateA = audioProc.apvts.copyState();
    startTimerHz (30);

    // melatonin_inspector — enable keyboard focus so Cmd+Shift+I toggles it
    setWantsKeyboardFocus (true);
}

NativeEditor::~NativeEditor()
{
    setLookAndFeel (nullptr);
}

//=============================================================================
//  SliderContextMenu — UAD-style RMB menu (Reset / Type-in / Copy / Paste)
//=============================================================================
void NativeEditor::SliderContextMenu::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isRightButtonDown()) return;
    auto* slider = dynamic_cast<juce::Slider*> (e.eventComponent);
    if (slider == nullptr) return;

    juce::PopupMenu menu;
    menu.addItem (1, "Reset to default");
    menu.addItem (2, "Type-in value…");
    menu.addSeparator();
    menu.addItem (3, "Copy value");
    const auto clipboard = juce::SystemClipboard::getTextFromClipboard();
    const bool clipIsNumber = clipboard.containsAnyOf ("0123456789.-");
    menu.addItem (4, "Paste value", clipIsNumber);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (slider),
        [slider] (int result)
        {
            if (result == 1)
            {
                slider->setValue (slider->getDoubleClickReturnValue(),
                                   juce::sendNotification);
            }
            else if (result == 2)
            {
                auto* aw = new juce::AlertWindow ("Type Value",
                    "Enter value (range " + juce::String (slider->getMinimum(), 2)
                        + " – " + juce::String (slider->getMaximum(), 2) + "):",
                    juce::MessageBoxIconType::QuestionIcon);
                aw->addTextEditor ("v", juce::String (slider->getValue(), 3));
                aw->addButton ("OK",     1, juce::KeyPress (juce::KeyPress::returnKey));
                aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
                aw->enterModalState (true,
                    juce::ModalCallbackFunction::create (
                        [aw, slider] (int r) {
                            if (r == 1)
                                slider->setValue (aw->getTextEditorContents ("v").getDoubleValue(),
                                                  juce::sendNotification);
                            delete aw;
                        }));
            }
            else if (result == 3)
            {
                juce::SystemClipboard::copyTextToClipboard (juce::String (slider->getValue(), 4));
            }
            else if (result == 4)
            {
                slider->setValue (juce::SystemClipboard::getTextFromClipboard().getDoubleValue(),
                                   juce::sendNotification);
            }
        });
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
                     "BEOLUX 2000", "SOUNDBOYS · DANISH TAPE EMULATION · v60.1");

    // Counter (bottom-centre of deck, just below the VU row)
    {
        constexpr int cW = 76, cH = 18;
        const juce::Rectangle<int> counterRect (inner.getCentreX() - cW / 2,
                                                aluZone.getBottom() - cH - 4,
                                                cW, cH);
        LnF::drawCounter (g, counterRect, counterText);
        // Melatonin InnerShadow — LCD panel recessed into deck
        {
            juce::Path cp;
            cp.addRoundedRectangle (counterRect.toFloat(), 2.0f);
            static thread_local melatonin::InnerShadow counterInner {
                { juce::Colours::black.withAlpha (0.55f), 4, { 0, 2 } }
            };
            counterInner.render (g, cp);
        }
    }

    // ===== Recording LED (red dot to the LEFT of the counter, lights when running) =====
    {
        const float lr = 3.5f;
        const float lx = (float) inner.getCentreX() - 76.0f;
        const float ly = (float) (aluZone.getBottom() - 13);
        if (recLedOn)
        {
            // Melatonin Gaussian glow — replaces manual halo with real blur
            {
                juce::Path ledGlowPath;
                ledGlowPath.addEllipse (lx - lr, ly - lr, lr * 2, lr * 2);
                static thread_local melatonin::DropShadow ledGlow {
                    { juce::Colour (0xFFFF3020).withAlpha (0.75f), 10, { 0, 0 } }
                };
                ledGlow.render (g, ledGlowPath);
            }
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
            g.setFont (LnF::sectionFont (10.5f));
            // engraved-into-metal effect: dark shadow below, bright silver on top
            g.setColour (juce::Colours::black.withAlpha (0.85f));
            g.drawText (h.text, h.r.translated (0, 1), juce::Justification::centred, false);
            g.setColour (juce::Colour (0xFFE0E0E4));
            g.drawText (h.text, h.r, juce::Justification::centred, false);
        }
    }

    // ===== Tape-speed indicator LEDs (3 amber dots next to TAPE SPEED label) =====
    // Positions derived from the layout constants; stable across repaints.
    {
        const int speedIdx = (int) audioProc.apvts.getRawParameterValue ("speed")->load();

        // lbl_speed sits at y = kAluH+kDivH+kPresetH+11+3, h=10  →  centre ≈ y+5
        constexpr int lcdY   = kAluH + kDivH + kPresetH + 11 + 3 + 5;
        constexpr int dotsX0 = kTeakW + kLeftColW - 14;   // right side of left column
        constexpr float dotR = 2.5f, dotGap = 8.0f;

        for (int i = 0; i < 3; ++i)
        {
            const float dx = (float) dotsX0 - (float) (2 - i) * dotGap;
            const float dy = (float) lcdY;
            const bool  on = (i == speedIdx);
            if (on)
            {
                // amber glow halo
                g.setColour (juce::Colour (0xFFE8B040).withAlpha (0.32f));
                g.fillEllipse (dx - dotR * 2.4f, dy - dotR * 2.4f, dotR * 4.8f, dotR * 4.8f);
                // LED body
                juce::ColourGradient ledGrad (
                    juce::Colour (0xFFFFD070), dx - dotR * 0.4f, dy - dotR * 0.5f,
                    juce::Colour (0xFFA07018), dx + dotR * 0.4f, dy + dotR * 0.5f, false);
                g.setGradientFill (ledGrad);
                g.fillEllipse (dx - dotR, dy - dotR, dotR * 2, dotR * 2);
                // specular
                g.setColour (juce::Colours::white.withAlpha (0.80f));
                g.fillEllipse (dx - dotR * 0.40f, dy - dotR * 0.65f, dotR * 0.60f, dotR * 0.40f);
            }
            else
            {
                g.setColour (juce::Colour (0xFF1E1A10));
                g.fillEllipse (dx - dotR, dy - dotR, dotR * 2, dotR * 2);
                g.setColour (juce::Colour (0xFF3A3020).withAlpha (0.7f));
                g.drawEllipse (dx - dotR, dy - dotR, dotR * 2, dotR * 2, 0.5f);
            }
        }
        // Speed-value captions below each dot (4.75 / 9.5 / 19)
        g.setFont (LnF::sectionFont (5.0f));
        g.setColour (juce::Colour (0xFF907850).withAlpha (0.65f));
        const char* spLbls[] = { "4.75", "9.5", "19" };
        for (int i = 0; i < 3; ++i)
        {
            const float dx = (float) dotsX0 - (float) (2 - i) * dotGap;
            g.drawText (spLbls[i],
                juce::Rectangle<float> (dx - 8.0f, (float) lcdY + dotR + 1.5f, 16.0f, 7.0f)
                    .toNearestInt(),
                juce::Justification::centred, false);
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

    // Melatonin InnerShadow on teak/panel boundary — frame holds the panel
    {
        juce::Path innerEdge;
        innerEdge.addRectangle (inner.toFloat());
        static thread_local melatonin::InnerShadow teakInner {
            { juce::Colours::black.withAlpha (0.42f), 8, { 0, 0 } }
        };
        teakInner.render (g, innerEdge);
    }

    // Melatonin InnerShadow on center DualFader column — groove depth for faders
    {
        const auto faderCol = juce::Rectangle<int> (
            kTeakW + kLeftColW,
            kAluH + kDivH + kPresetH,
            kCenterColW,
            bounds.getHeight() - kAluH - kDivH - kPresetH).toFloat();
        juce::Path faderPath;
        faderPath.addRectangle (faderCol);
        static thread_local melatonin::InnerShadow faderInner {
            { juce::Colours::black.withAlpha (0.30f), 12, { 0, 0 } }
        };
        faderInner.render (g, faderPath);
    }

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
        // Positioned dynamically: measure the actual title font width so the
        // badge always lands 20 px clear of the last glyph, regardless of
        // platform font metrics.
        const float br = 13.0f;
        const float titleTextW = (float) juce::GlyphArrangement::getStringWidthInt (
                                             lnf.logoFont (22.0f), "BEOLUX 2000");
        const float bx = (float)(kTeakW + 14) + titleTextW + 20.0f + br;
        const float by = 6.0f;

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

    // ===== Top deck zone: reel deck + analog VU pair + spectrum strip =====
    auto deckZone = inner.withHeight (kAluH).withTrimmedTop (20).reduced (4, 1);
    reelDeck.setBounds (deckZone);

    // Spectrum strip — thin glowing band at the bottom of the deck zone,
    // spans full inner width, just above the chrome divider line.
    {
        const int strH = 18;
        spectrum.setBounds (kTeakW + 4, kAluH - strH - 1, kInnerW - 8, strH);
    }

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

    btn_preset_name.setBounds (presetBar.reduced (0, 2));

    // Browser overlay — full inner width, from below preset bar to bottom
    const int browserY  = kAluH + kDivH + kPresetH;
    const int browserH  = bounds.getHeight() - browserY;
    presetBrowser.setBounds (kTeakW, browserY, kInnerW, browserH);

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

        // Row 1: TREBLE / BASS / BAL / ASYM  (4 × cell4)
        layoutKnob (knob_treble,  lbl_treble,  row1, cell4);
        layoutKnob (knob_bass,    lbl_bass,    row1, cell4);
        layoutKnob (knob_balance, lbl_balance, row1, cell4);
        layoutKnob (knob_asym,    lbl_asym,    row1, cell4);

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
    auto& chain = audioProc.getChain();
    vuInL.setLevel (chain.inputLevelL_dBFS.load());
    vuInR.setLevel (chain.inputLevelR_dBFS.load());
    // OUT meter: max of L/R post-processing (most useful for clip-detection)
    vuOut.setLevel (juce::jmax (chain.meterLevelL_dBFS.load(),
                                 chain.meterLevelR_dBFS.load()));

    // Reel spin when any input gain > threshold OR active output signal
    const float inputAny = audioProc.apvts.getRawParameterValue ("mic_gain")->load()
                         + audioProc.apvts.getRawParameterValue ("phono_gain")->load()
                         + audioProc.apvts.getRawParameterValue ("radio_gain")->load();
    const float outLvl = juce::jmax (chain.meterLevelL_dBFS.load(),
                                      chain.meterLevelR_dBFS.load());
    const bool tapeRunning = (inputAny > 0.05f) || outLvl > -40.0f;
    reelDeck.setActive (tapeRunning);

    // Sync ReelDeck tape position directly from the DSP pipeline
    reelDeck.setTapePosition (chain.tapePositionSeconds.load (std::memory_order_relaxed));
    reelDeck.setWowIntensity (chain.wowCurrentAmp.load (std::memory_order_relaxed));

    // Record LED — blinks at 2 Hz when tape is running
    const bool ledTarget = tapeRunning &&
        (((juce::Time::getMillisecondCounter() / 250) & 1) != 0);
    if (ledTarget != recLedOn)
    {
        recLedOn = ledTarget;
        repaint (juce::Rectangle<int> (kTeakW, kAluH - 22, kInnerW, 22));
    }

    const int speedIdx = (int) audioProc.apvts.getRawParameterValue ("speed")->load();
    reelDeck.setSpeed (speedIdx);

    // Repaint the speed-indicator LED strip whenever the selected speed changes.
    if (speedIdx != prevSpeedIdx)
    {
        prevSpeedIdx = speedIdx;
        // Repaint the left-column region that contains the three speed LEDs.
        repaint (juce::Rectangle<int> (kTeakW, kAluH + kDivH, kLeftColW, kPresetH + 60));
    }

    // Counter: ticks up while tape "runs" — speed-aware (more tape passes per second
    // at higher speed, exactly like the mechanical Beocord drum counter).
    // rate: 4.75 cm/s = ×1, 9.5 = ×2, 19 = ×4 — matches the capstan linear velocity ratio.
    // Rolls at 9999 ≈ 41 min at 4.75 cm/s / ≈ 10 min at 19 cm/s — same as real machine.
    if (tapeRunning)
    {
        const double tapePos = chain.tapePositionSeconds.load (std::memory_order_relaxed);
        const double rate = (speedIdx == 0 ? 1.0 : speedIdx == 1 ? 2.0 : 4.0);
        const int counts = static_cast<int> (tapePos * 4.0 * rate) % 10000;
        juce::String c = juce::String (counts).paddedLeft ('0', 4);
        if (c != counterText)
        {
            counterText = c;
            repaint (juce::Rectangle<int> (kTeakW, kAluH - 28, kInnerW, 28));
        }
    }

    const double sr = audioProc.getSampleRate();
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
//  Presets — data-driven, validated, smooth tween
//=============================================================================
void NativeEditor::applyPreset (int idx)
{
    // Guard: clamp to valid range
    idx = juce::jlimit (0, bc2000dl::kNumPresets - 1, idx);
    const auto& p = bc2000dl::kPresets[idx];
    auto& v = audioProc.apvts;

    // Build a list of (paramId, fromNormalized, toNormalized) tweens
    struct Tween { juce::String id; float fromN, toN; };
    auto targets = std::make_shared<std::vector<Tween>>();

    auto plan = [&] (const juce::String& id, float val)
    {
        if (auto* prm = v.getParameter (id))
        {
            const float toN   = prm->convertTo0to1 (val);
            const float fromN = prm->getValue();
            targets->push_back ({ id, fromN, toN });
        }
    };

    // --- All parameters explicitly from preset data (no "baseline + delta") ---
    plan ("speed",              (float) p.speed);
    plan ("mic_gain",           p.mic_gain);
    plan ("mic_gain_r",         p.mic_gain_r);
    plan ("phono_gain",         p.phono_gain);
    plan ("phono_gain_r",       p.phono_gain_r);
    plan ("radio_gain",         p.radio_gain);
    plan ("radio_gain_r",       p.radio_gain_r);
    plan ("tape_formula",       (float) p.tape_formula);
    plan ("saturation_drive",   p.saturation_drive);
    plan ("saturation_drive_r", p.saturation_drive_r);
    plan ("bias_amount",        p.bias_amount);
    plan ("wow_flutter",        p.wow_flutter);
    plan ("multiplay_gen",      (float) p.multiplay_gen);
    plan ("bass_db",            p.bass_db);
    plan ("treble_db",          p.treble_db);
    plan ("balance",            p.balance);
    plan ("master_volume",      p.master_volume);
    plan ("echo_enabled",       p.echo_enabled ? 1.0f : 0.0f);
    plan ("echo_amount",        p.echo_amount);
    plan ("echo_amount_r",      p.echo_amount_r);
    // Presets never activate these — always reset to safe defaults
    plan ("bypass_tape",        0.0f);
    plan ("speaker_monitor",    0.0f);
    plan ("synchroplay",        0.0f);
    plan ("pause",              0.0f);
    plan ("sos_enabled",        0.0f);
    plan ("pa_enabled",         0.0f);
    plan ("mic_loz",            0.0f);

    // ---- Smooth 250 ms tween (15 frames at ~60 Hz) ----
    // Capture `this` (not &vRef) — vRef is a local stack variable that would
    // dangle after applyPreset() returns.  `this` stays alive for the editor's
    // full lifetime, which is always longer than the 250 ms animation.
    constexpr int kFrames = 15;
    for (int f = 1; f <= kFrames; ++f)
    {
        const float t  = (float) f / (float) kFrames;
        const float e  = t * t * (3.0f - 2.0f * t);   // smoothstep
        const bool last = (f == kFrames);

        juce::Timer::callAfterDelay (16 * f,
            [this, targets, e, last]
            {
                for (const auto& tw : *targets)
                    if (auto* prm = audioProc.apvts.getParameter (tw.id))
                    {
                        const float val = last
                            ? tw.toN
                            : juce::jlimit (0.0f, 1.0f,
                                tw.fromN + (tw.toN - tw.fromN) * e);
                        prm->setValueNotifyingHost (val);
                    }
            });
    }
}
