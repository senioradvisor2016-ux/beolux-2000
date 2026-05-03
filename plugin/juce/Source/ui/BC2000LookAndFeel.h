/*  BC2000LookAndFeel — instruction-card aesthetic for BC2000DL.

    Inspired by Juno60AF's procedural-drawing approach, but skinned to match
    the iconic Beocord 2000 DL operating-instructions card (the cream-coloured
    paper diagram that lived under the lid, in German, showing how every
    control mapped to a section of the deck).

    Visual language:
      - Cream paper panel (#EDE5D4)
      - Black hairlines as section frames
      - Helvetica Neue Condensed Bold for labels (tech-drawing style)
      - Sparse Bang & Olufsen amber for highlights
      - Roland-red mapped to "active state" (REC armed, parameter changed, etc.)
      - Two thin-line "reels" at the top, slide-faders below, transport at bottom

    All drawing is procedural. No bitmaps. Bundle stays tiny.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace bc2000dl::ui
{
    class InstructionCardLnF : public juce::LookAndFeel_V4
    {
    public:
        InstructionCardLnF();

        //==================== Color palette ===================
        static juce::Colour paper()       { return juce::Colour (0xFFEDE5D4); }
        static juce::Colour paperDarker() { return juce::Colour (0xFFD8CFB8); }
        static juce::Colour paperShadow() { return juce::Colour (0xFFC9BEA4); }
        static juce::Colour ink()         { return juce::Colour (0xFF1E1E1C); }
        static juce::Colour inkSoft()     { return juce::Colour (0xFF4A4A48); }
        static juce::Colour amber()       { return juce::Colour (0xFFD0A040); }
        static juce::Colour amberHot()    { return juce::Colour (0xFFE0B860); }
        static juce::Colour redAccent()   { return juce::Colour (0xFFC23A2A); } // REC, critical
        static juce::Colour woodDark()    { return juce::Colour (0xFF3B2620); }
        static juce::Colour woodLight()   { return juce::Colour (0xFF5B3A30); }
        static juce::Colour metalHi()     { return juce::Colour (0xFFD0D0CE); }
        static juce::Colour metalLo()     { return juce::Colour (0xFF7C7C7A); }

        //==================== Typography ======================
        static juce::Font labelFont   (float sizePx);  // body labels
        static juce::Font sectionFont (float sizePx);  // section headers (kerned)
        static juce::Font monoFont    (float sizePx);  // counter / readouts
        static juce::Font logoFont    (float sizePx);

        //==================== L&F overrides ===================
        void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h,
                               float sliderPos, float minPos, float maxPos,
                               juce::Slider::SliderStyle, juce::Slider&) override;

        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                               float sliderPos, float angleStart, float angleEnd,
                               juce::Slider&) override;

        void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                               bool highlighted, bool down) override;

        void drawButtonBackground (juce::Graphics&, juce::Button&,
                                    const juce::Colour& backgroundColour,
                                    bool highlighted, bool down) override;

        void drawComboBox (juce::Graphics&, int width, int height,
                            bool isButtonDown, int buttonX, int buttonY,
                            int buttonW, int buttonH, juce::ComboBox&) override;

        juce::Font getComboBoxFont (juce::ComboBox&) override;
        juce::Font getLabelFont    (juce::Label&) override;
        juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
        juce::Font getPopupMenuFont() override;

        void getIdealPopupMenuItemSize (const juce::String&, bool isSeparator,
                                        int standardHeight, int& w, int& h) override;
        void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area,
                                bool isSeparator, bool isActive,
                                bool isHighlighted, bool isTicked,
                                bool hasSubMenu, const juce::String& text,
                                const juce::String& shortcutKeyText,
                                const juce::Drawable* icon,
                                const juce::Colour* textColour) override;

        //==================== Panel painters (public helpers) =====
        /** Draw the entire cream paper panel with subtle paper-grain shadow. */
        static void drawPaperPanel (juce::Graphics&, juce::Rectangle<int>);

        /** Wood end-cap (left/right book-ends, like real Beocord). */
        static void drawWoodEndCap (juce::Graphics&, juce::Rectangle<int>, bool isLeft);

        /** Section box: thin black hairline frame with optional title in upper-left tab. */
        static void drawSectionBox (juce::Graphics&, juce::Rectangle<int>,
                                    const juce::String& title = {});

        /** Stylised reel: two concentric circles, three-spoke hub, "tape" rim. */
        static void drawReel (juce::Graphics&, juce::Rectangle<int>,
                              float rotationRad = 0.0f, bool active = false);

        /** Tape head/erase head pictogram (between reels). */
        static void drawHeadAssembly (juce::Graphics&, juce::Rectangle<int>);

        /** Transport piano-key style button background (used by drawButtonBackground). */
        static void drawTransportKey (juce::Graphics&, juce::Rectangle<int>,
                                      bool down, bool active, juce::Colour accent = amber());

        /** Slide-fader cap (silver with red center line). */
        static void drawSlideFaderCap (juce::Graphics&, juce::Rectangle<float>,
                                        bool bright = true);

        /** Slide-fader channel (recessed black slot with tick marks). */
        static void drawSlideChannel (juce::Graphics&, juce::Rectangle<float>);

        /** Section label with small arrow (instruction-card style). */
        static void drawArrowLabel (juce::Graphics&, juce::Rectangle<int>,
                                    const juce::String& text, bool arrowRight = true);

        /** Title with optional subtitle (Beocord 2000 De Luxe / DANISH TAPE 2000). */
        static void drawTitle (juce::Graphics&, juce::Rectangle<int>,
                               const juce::String& title, const juce::String& subtitle);

        /** Counter readout (4-digit mechanical-style). */
        static void drawCounter (juce::Graphics&, juce::Rectangle<int>,
                                 const juce::String& text);
    };
}
