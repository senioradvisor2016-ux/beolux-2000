/*  TransportLever + TapeCounter implementation. */

#include "TransportLever.h"

namespace bc2000dl::ui
{
    using namespace bc2000dl::ui::colours;

    void TransportLever::setState (State s)
    {
        if (state != s)
        {
            state = s;
            if (onStateChange) onStateChange (s);
            repaint();
        }
    }

    void TransportLever::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY();

        // ‹‹ (REW) till vänster
        g.setColour (state == REW ? kRecordRed : kSilverLight);
        g.setFont (juce::Font (juce::FontOptions (16.0f).withName ("Helvetica")).boldened());
        g.drawText (juce::CharPointer_UTF8 ("\xe2\x80\xb9\xe2\x80\xb9"),  // ‹‹
                     bounds.withRight (cx - 8.0f),
                     juce::Justification::centredRight, false);

        // ›› (FF) till höger
        g.setColour (state == FF ? kRecordRed : kSilverLight);
        g.drawText (juce::CharPointer_UTF8 ("\xe2\x80\xba\xe2\x80\xba"),  // ››
                     bounds.withLeft (cx + 8.0f),
                     juce::Justification::centredLeft, false);

        // Y-skaft (tjock silver-linje) — flyttas baserat på state
        float shaftAngle = 0.0f;
        switch (state)
        {
            case Stop: shaftAngle = 0.0f; break;
            case Play: shaftAngle = -0.4f; break;
            case FF:   shaftAngle = -0.8f; break;
            case REW:  shaftAngle = +0.8f; break;
        }

        const float shaftLen = 12.0f;
        const float topX = cx + std::sin (shaftAngle) * shaftLen;
        const float topY = cy - std::cos (shaftAngle) * shaftLen + 4.0f;
        const float botY = cy + 8.0f;

        g.setColour (kSilverLight);
        juce::Path shaft;
        shaft.startNewSubPath (cx, botY);
        shaft.lineTo (topX, topY);
        g.strokePath (shaft, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

        // Kula på toppen
        const float ballR = 5.0f;
        g.setColour (state == Play ? kRecordRed : kSilverLight);
        g.fillEllipse (topX - ballR, topY - ballR, ballR * 2.0f, ballR * 2.0f);
        g.setColour (kAluDark);
        g.drawEllipse (topX - ballR, topY - ballR, ballR * 2.0f, ballR * 2.0f, 0.5f);
    }

    void TransportLever::mouseDown (const juce::MouseEvent& e)
    {
        // Klick → cycla mellan Stop → Play → Stop → ...
        // Drag åt sida → REW/FF (förenklad i v0.1)
        const float relX = e.position.x - getWidth() * 0.5f;

        if (relX < -getWidth() * 0.3f)
            setState (REW);
        else if (relX > getWidth() * 0.3f)
            setState (FF);
        else if (state == Play)
            setState (Stop);
        else
            setState (Play);
    }

    // ---------- TapeCounter ----------
    TapeCounter::TapeCounter()
    {
        startTimerHz (10);  // 10 Hz update
    }

    void TapeCounter::timerCallback()
    {
        if (running)
        {
            counter = (counter + 1) % 1000;  // 3-digit wrap
            repaint();
        }
    }

    void TapeCounter::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();

        // Dark recess "fönster" runt mekanisk räkneverket
        juce::ColourGradient bgGrad (
            juce::Colour (0xff1a140d), bounds.getX(), bounds.getY(),
            juce::Colour (0xff0d0a07), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill (bgGrad);
        g.fillRoundedRectangle (bounds, 4.0f);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

        // Etched "COUNTER"-label överst
        const auto labelArea = bounds.reduced (8.0f, 4.0f).withHeight (10.0f);
        g.setColour (kSoftGray);
        g.setFont (juce::Font (juce::FontOptions (8.0f).withName ("Helvetica").withStyle ("Bold")));
        g.drawText ("COUNTER", labelArea, juce::Justification::centredLeft, false);

        // MEM-label + lamp t.h.
        const auto memArea = labelArea.withTrimmedLeft (labelArea.getWidth() - 30.0f);
        g.setColour (kSoftGray);
        g.drawText ("MEM", memArea, juce::Justification::centredRight, false);

        // 4 cream-mekaniska digit-celler (matchar Ferroflux-designen)
        // Vi har 3-digit i nuvarande counter, men ritar 4 för att matcha
        // (första alltid 0)
        const float digitsAreaY = bounds.getY() + 18.0f;
        const float digitsH = bounds.getHeight() - 24.0f;
        const float digitGap = 1.5f;
        const auto digitsArea = juce::Rectangle<float> (
            bounds.getX() + 6.0f, digitsAreaY,
            bounds.getWidth() - 12.0f, digitsH);

        // Inner-pocket (svart, för digits)
        g.setColour (juce::Colour (0xff050403));
        g.fillRoundedRectangle (digitsArea, 1.0f);

        const float digitW = (digitsArea.getWidth() - digitGap * 3.0f) / 4.0f;
        const auto text = juce::String::formatted ("%04d", counter);

        for (int i = 0; i < 4; ++i)
        {
            const float dx = digitsArea.getX() + 1.0f + (digitW + digitGap) * i;
            const auto cell = juce::Rectangle<float> (
                dx, digitsArea.getY() + 1.0f, digitW, digitsArea.getHeight() - 2.0f);

            // Cream digit-cell (mekanisk roll)
            juce::ColourGradient cellGrad (
                kCream2, cell.getX(), cell.getY(),
                kCream3.darker (0.1f), cell.getX(), cell.getBottom(), false);
            cellGrad.addColour (0.45, kCream1);
            cellGrad.addColour (0.55, kCream1);
            g.setGradientFill (cellGrad);
            g.fillRect (cell);

            // Center-seam (mekanisk räkneverk-skugga)
            g.setColour (kInk.withAlpha (0.18f));
            g.drawLine (cell.getX(), cell.getCentreY(), cell.getRight(), cell.getCentreY(), 0.5f);

            // Inre skugga
            g.setColour (kInk.withAlpha (0.4f));
            g.drawLine (cell.getX(), cell.getY() + 1, cell.getRight(), cell.getY() + 1, 0.6f);

            // Siffra (Helvetica Bold ink)
            g.setColour (kInk);
            g.setFont (juce::Font (juce::FontOptions (cell.getHeight() * 0.65f)
                                    .withName ("Helvetica").withStyle ("Bold")));
            g.drawText (juce::String::charToString (text[i]), cell,
                        juce::Justification::centred, false);
        }
    }
}
