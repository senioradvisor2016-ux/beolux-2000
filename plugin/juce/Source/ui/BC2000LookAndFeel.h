/*  BC2000LookAndFeel — hardware-accurate aesthetic for BC2000DL.

    Visual language matches the physical Beocord 2000 De Luxe (1968-69):
      - Teak/walnut end-caps (#3A1F0E → #7A4A2A grain)
      - Brushed silver-aluminium top deck (horizontal stripe noise)
      - Black control panel (#141416) for all knobs/faders/buttons
      - RED BULLSEYE reels (black rim → cream band → red ring → cream → red dot)
      - Cream-amber (#D4C89A) text on dark surfaces
      - Bang & Olufsen amber for counter / VU readouts

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

        //==================== Colour palette ======================
        // --- Paper / popup colours (kept cream for popup menus / tooltips) ---
        static juce::Colour paper()        { return juce::Colour (0xFFEDE5D4); }
        static juce::Colour paperDarker()  { return juce::Colour (0xFFD8CFB8); }
        static juce::Colour paperShadow()  { return juce::Colour (0xFFC9BEA4); }

        // --- Dark panel colours (main surface) ---
        static juce::Colour blackPanel()   { return juce::Colour (0xFF141416); }
        static juce::Colour panelMid()     { return juce::Colour (0xFF242428); }
        static juce::Colour panelEdge()    { return juce::Colour (0xFF383840); }

        // --- Brushed aluminium (top deck) ---
        static juce::Colour aluHi()        { return juce::Colour (0xFFD2D2D0); }
        static juce::Colour aluMid()       { return juce::Colour (0xFFB4B4B2); }
        static juce::Colour aluLo()        { return juce::Colour (0xFF909090); }

        // --- Teak / walnut end-caps ---
        static juce::Colour teakDark()     { return juce::Colour (0xFF3A1F0E); }
        static juce::Colour teakMid()      { return juce::Colour (0xFF5C3118); }
        static juce::Colour teakLight()    { return juce::Colour (0xFF7A4A2A); }

        // --- Text on dark panel ---
        static juce::Colour creamLabel()   { return juce::Colour (0xFFD4C89A); }
        static juce::Colour creamDim()     { return juce::Colour (0xFF908868); }

        // --- Ink (general dark) ---
        static juce::Colour ink()          { return juce::Colour (0xFF1E1E1C); }
        static juce::Colour inkSoft()      { return juce::Colour (0xFF4A4A48); }

        // --- Accents ---
        static juce::Colour amber()        { return juce::Colour (0xFFD0A040); }
        static juce::Colour amberHot()     { return juce::Colour (0xFFE0B860); }
        static juce::Colour redAccent()    { return juce::Colour (0xFFC23A2A); }

        // --- Legacy (kept for callers that still reference them) ---
        static juce::Colour woodDark()     { return teakDark(); }
        static juce::Colour woodLight()    { return teakLight(); }
        static juce::Colour metalHi()      { return aluHi(); }
        static juce::Colour metalLo()      { return aluLo(); }

        //==================== Typography ==========================
        static juce::Font labelFont   (float sizePx);
        static juce::Font sectionFont (float sizePx);
        static juce::Font monoFont    (float sizePx);
        static juce::Font logoFont    (float sizePx);

        //==================== L&F overrides =======================
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

        juce::Font getComboBoxFont  (juce::ComboBox&) override;
        juce::Font getLabelFont     (juce::Label&) override;
        juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
        juce::Font getPopupMenuFont () override;

        void getIdealPopupMenuItemSize (const juce::String&, bool isSeparator,
                                        int standardHeight, int& w, int& h) override;
        void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area,
                                bool isSeparator, bool isActive,
                                bool isHighlighted, bool isTicked,
                                bool hasSubMenu, const juce::String& text,
                                const juce::String& shortcutKeyText,
                                const juce::Drawable* icon,
                                const juce::Colour* textColour) override;

        //==================== Panel painters ======================

        /** Brushed-aluminium top-deck panel. */
        static void drawPaperPanel (juce::Graphics&, juce::Rectangle<int>);

        /** Warm teak/walnut end-caps. */
        static void drawWoodEndCap (juce::Graphics&, juce::Rectangle<int>, bool isLeft);

        /** Solid dark control-panel area (bottom zone). */
        static void drawBlackPanel (juce::Graphics&, juce::Rectangle<int>);

        /** Section box: subtle dark-outline frame with optional title tab. */
        static void drawSectionBox (juce::Graphics&, juce::Rectangle<int>,
                                    const juce::String& title = {});

        /** Beocord bullseye reel: black rim → cream → red ring → cream → red dot. */
        static void drawReel (juce::Graphics&, juce::Rectangle<int>,
                              float rotationRad = 0.0f, bool active = false);

        /** Tape head/erase-head pictogram. */
        static void drawHeadAssembly (juce::Graphics&, juce::Rectangle<int>);

        /** Transport piano-key style button background. */
        static void drawTransportKey (juce::Graphics&, juce::Rectangle<int>,
                                      bool down, bool active,
                                      juce::Colour accent = redAccent());

        /** Slide-fader cap (dark chrome with red centre line). */
        static void drawSlideFaderCap (juce::Graphics&, juce::Rectangle<float>,
                                        bool bright = true);

        /** Slide-fader channel (recessed slot with tick marks). */
        static void drawSlideChannel (juce::Graphics&, juce::Rectangle<float>);

        /** Section label with small arrow. */
        static void drawArrowLabel (juce::Graphics&, juce::Rectangle<int>,
                                    const juce::String& text, bool arrowRight = true);

        /** Title / logo area (renders on aluminium background). */
        static void drawTitle (juce::Graphics&, juce::Rectangle<int>,
                               const juce::String& title, const juce::String& subtitle);

        /** Mechanical counter readout (4-digit, amber digits on dark window). */
        static void drawCounter (juce::Graphics&, juce::Rectangle<int>,
                                 const juce::String& text);
    };
}
