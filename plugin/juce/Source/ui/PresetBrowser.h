/*  PresetBrowser — UAD-premium preset browser för BC2000DL.

    Layout (1156 × 295px — overlayer the control zone when open):

      ┌─ SEARCH (🔍 + TextEditor) ───────────────────────── [✕] ───┐  32px
      ├── CAT COLUMN (120px) ──┬── PRESET LIST ──────── SCROLLBAR ─┤
      │  ● ALL          (43)   │  ● AGFA ARCHIVE  [AGFA][19]  ♥    │
      │    TAPE COLOR    (7)   │    description preview...          │
      │    VOCALS        (6)   │    BASF SATURATION [BASF][9.5] ♥  │
      │    DRUMS         (6)   │    ...                             │ 207px
      │    BASS          (5)   │                                    │
      │    SYNTH         (6)   │                                    │
      │    BROADCAST     (5)   │                                    │
      │    SPECIAL       (7)   │                                    │
      ├── INFO PANEL ─────────────────────────────────────────────  ┤  56px
      │  [name large]  [FORMULA] [IPS]  description text             │
      │  DRIVE ███████  WOW ██  BIAS █████  BASS ██  TREB ██  ECHO │
      └──────────────────────────────────────────────────────────────┘

    Interactions:
      - Click category → filter list
      - Click ♥        → toggle favorite (persists in session)
      - Click row       → apply preset + close
      - Hover row       → info panel updates (no sound change)
      - Type            → instant search filter
      - ↑↓              → keyboard navigate
      - Enter           → apply + close
      - Escape / ✕      → close (discard hover)
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../presets/Presets.h"
#include <set>

namespace bc2000dl::ui
{
    class PresetBrowser  : public juce::Component,
                           private juce::TextEditor::Listener
    {
    public:
        PresetBrowser();
        ~PresetBrowser() override = default;

        /** Fires when user confirms a preset (click or Enter).
            Index is into bc2000dl::kPresets[]. */
        std::function<void (int index)> onPresetSelected;

        /** Open the browser, centering the currently active preset. */
        void openBrowser (int currentPresetIndex);

        void paint          (juce::Graphics&) override;
        void resized        () override;
        void mouseDown      (const juce::MouseEvent&) override;
        void mouseMove      (const juce::MouseEvent&) override;
        void mouseExit      (const juce::MouseEvent&) override;
        void mouseWheelMove (const juce::MouseEvent&,
                             const juce::MouseWheelDetails&) override;
        bool keyPressed     (const juce::KeyPress&) override;

    private:
        // ----------------------------------------------------------
        // Layout zones (computed in resized)
        // ----------------------------------------------------------
        static constexpr int kSearchH   = 32;   // search bar height
        static constexpr int kInfoH     = 56;   // info panel height
        static constexpr int kCatW      = 120;  // category column width
        static constexpr int kScrollW   = 8;    // scrollbar width
        static constexpr int kRowH      = 38;   // preset row height

        juce::Rectangle<int> zSearch, zCat, zList, zInfo;

        // ----------------------------------------------------------
        // Child components
        // ----------------------------------------------------------
        juce::TextEditor searchBox;

        // ----------------------------------------------------------
        // State
        // ----------------------------------------------------------
        int   selectedCat   { 0 };
        int   currentPreset { 0 };
        int   hoverRow      { -1 };   // row under mouse (-1 = none)
        int   infoPreset    { 0 };    // which preset the info panel shows
        float scrollOffset  { 0.0f };
        juce::Array<int> filteredIndices;
        std::set<int>    favorites;   // preset indices marked favourite
        juce::String     searchText;

        // ----------------------------------------------------------
        // Category hit testing
        // ----------------------------------------------------------
        struct CatRow { int catIdx; juce::Rectangle<int> r; };
        juce::Array<CatRow> catRows;
        void buildCatRows();
        int  catAtPoint (juce::Point<int> p) const;
        int  presetCountForCat (int catIdx) const;
        void selectCategory (int idx);

        // ----------------------------------------------------------
        // Preset list helpers
        // ----------------------------------------------------------
        void rebuildFilteredList();
        int  rowAtY (int y) const;
        float maxScroll() const;
        void  clampScroll();
        void  scrollToRow (int row);

        // ----------------------------------------------------------
        // TextEditor::Listener
        // ----------------------------------------------------------
        void textEditorTextChanged    (juce::TextEditor&) override;
        void textEditorReturnKeyPressed (juce::TextEditor&) override;
        void textEditorEscapeKeyPressed (juce::TextEditor&) override;

        // ----------------------------------------------------------
        // Drawing
        // ----------------------------------------------------------
        void paintSearchBar  (juce::Graphics&) const;
        void paintCatColumn  (juce::Graphics&) const;
        void paintListArea   (juce::Graphics&) const;
        void paintInfoPanel  (juce::Graphics&) const;
        void paintRow        (juce::Graphics& g,
                              juce::Rectangle<int> rowR,
                              int presetIdx,
                              bool isSelected,
                              bool isHovered) const;
        void paintParamBar   (juce::Graphics& g,
                              juce::Rectangle<int> area,
                              const juce::String& label,
                              float norm,         // 0..1
                              juce::Colour barColour) const;
        static juce::Font nameFont();
        static juce::Font catFont();
        static juce::Font pillFont();
        static juce::Font descFont();
        static juce::Font infoNameFont();
        static juce::Font paramFont();

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBrowser)
    };
}
