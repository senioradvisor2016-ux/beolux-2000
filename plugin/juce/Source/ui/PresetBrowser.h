/*  PresetBrowser — UAD-style floating preset browser for BC2000DL.

    Layout (overlays the control zone when open):

      ┌── category tabs (horizontal) ────────────────────────────────┐
      │  ALL  TAPE COLOR  VOCALS  DRUMS  BASS  SYNTH  BROADCAST  SPE │
      ├──────────────────────────────────────────────────────────────┤
      │  ● AGFA ARCHIVE        [AGFA] [19 IPS]  Neutral archive...   │
      │    BASF SATURATION     [BASF] [9.5 IPS] Warm saturation...   │
      │    ...                                                         │
      └──────────────────────────────────────────────────────────────┘

    - Opens on click of preset name button
    - Closes on Escape, clicking outside, or selecting a preset
    - Current preset is marked with ● and amber background
    - Hover highlights in slightly lighter background
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../presets/Presets.h"

namespace bc2000dl::ui
{
    class PresetBrowser  : public juce::Component
    {
    public:
        PresetBrowser();
        ~PresetBrowser() override = default;

        /** Called when user selects a preset. Index matches kPresets[]. */
        std::function<void (int index)> onPresetSelected;

        /** Open the browser, highlighting the currently active preset. */
        void openBrowser (int currentPresetIndex);

        void paint            (juce::Graphics&) override;
        void resized          () override;
        void mouseDown        (const juce::MouseEvent&) override;
        void mouseMove        (const juce::MouseEvent&) override;
        void mouseExit        (const juce::MouseEvent&) override;
        void mouseWheelMove   (const juce::MouseEvent&,
                               const juce::MouseWheelDetails&) override;
        bool keyPressed       (const juce::KeyPress&) override;

    private:
        // --- Category filter ---
        int selectedCat { 0 };       // 0 = ALL
        void selectCategory (int idx);

        // Build filtered index list for current category
        void rebuildFilteredList();
        juce::Array<int> filteredIndices;  // kPresets indices matching category

        // --- Row metrics ---
        static constexpr int kTabH      = 34;
        static constexpr int kRowH      = 40;
        static constexpr int kScrollW   = 10;

        juce::Rectangle<int> listArea() const;
        int rowAtY (int y) const;         // returns filteredIndices row, or -1

        // --- State ---
        int currentPreset { 0 };
        int hoverRow      { -1 };
        float scrollOffset { 0.0f };      // px, top of list

        // --- Category tab hit-testing ---
        struct TabRect { int catIdx; juce::Rectangle<int> r; };
        juce::Array<TabRect> tabRects;
        void buildTabRects();
        int  tabAtPoint (juce::Point<int> p) const;

        // --- Scroll helpers ---
        float maxScroll() const;
        void  clampScroll();

        // --- Draw helpers ---
        void drawTabBar  (juce::Graphics&) const;
        void drawRow     (juce::Graphics& g,
                          juce::Rectangle<int> rowR,
                          int presetIdx,
                          bool isSelected,
                          bool isHovered) const;
        void drawPill    (juce::Graphics& g,
                          juce::Rectangle<int>& cursor,
                          const juce::String& label,
                          juce::Colour bg,
                          juce::Colour fg) const;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBrowser)
    };
}
