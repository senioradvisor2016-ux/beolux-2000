/*  RecordIndicator — L/R röd bar (matchar BC2000 DL frontpanel).

    Två röda rektanglar som visar L/R record-arming-status.
    Lyser röda när motsvarande spår är armed för inspelning.
*/
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace bc2000dl::ui
{
    class RecordIndicator : public juce::Component
    {
    public:
        RecordIndicator (const juce::String& chLabel) : label (chLabel) {}

        void setLit (bool lit) { isLit = lit; repaint(); }

        void paint (juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat();

            // Mörk bezel
            g.setColour (juce::Colour (0xff2a2622));
            g.fillRoundedRectangle (bounds, 3.0f);
            g.setColour (juce::Colours::black);
            g.drawRoundedRectangle (bounds, 3.0f, 0.7f);

            // Inre bar — chrome med rött center när lit
            const auto inner = bounds.reduced (3.0f);
            juce::ColourGradient ig (
                juce::Colour (0xffe6e3da), inner.getX(), inner.getY(),
                juce::Colour (0xff8a8780), inner.getX(), inner.getBottom(), false);
            g.setGradientFill (ig);
            g.fillRoundedRectangle (inner, 2.0f);

            // Röd center-bar (lyser vid record-arm)
            const auto redBar = inner.reduced (4.0f, inner.getHeight() * 0.35f);
            if (isLit)
            {
                juce::ColourGradient rg (
                    juce::Colour (0xffff5a3a), redBar.getX(), redBar.getY(),
                    juce::Colour (0xffd8442a), redBar.getX(), redBar.getBottom(), false);
                g.setGradientFill (rg);
                g.fillRoundedRectangle (redBar, 1.0f);
                g.setColour (juce::Colour (0xffd8442a).withAlpha (0.4f));
                g.fillRoundedRectangle (redBar.expanded (3.0f), 2.0f);
            }
            else
            {
                g.setColour (juce::Colour (0xff5a2a18));
                g.fillRoundedRectangle (redBar, 1.0f);
            }

            // L/R-label nedanför
            g.setColour (juce::Colour (0xff141414));
            g.setFont (juce::Font (juce::FontOptions (8.0f).withName ("Helvetica").withStyle ("Bold")));
            g.drawText (label,
                        bounds.removeFromBottom (10.0f),
                        juce::Justification::centred, false);
        }

    private:
        juce::String label;
        bool isLit { false };
    };
}
