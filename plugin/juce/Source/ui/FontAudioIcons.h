/*  FontAudioIcons — JUCE 8-clean wrapper around fontaudio.ttf.

    Loads the font once (lazy, thread-safe) and exposes:
      - icons::Play, icons::Stop, icons::Record, ... (PUA codepoints from fontaudio)
      - drawIcon (g, glyph, bounds, colour, size) — single-call draw helper

    Why a custom wrapper instead of the upstream fontaudio JUCE module?
      - Upstream uses deprecated APIs (float_Pi, Font::setHeight, using-namespace
        in a public header) that don't compile cleanly on JUCE 8.
      - We only need ~10 glyphs — no need for the IconHelper image-cache layer.
*/

#pragma once

#include <juce_graphics/juce_graphics.h>
#include "BinaryData.h"

namespace bc2000dl::ui::icons
{
    // ----- Glyphs (PUA codepoints, source: fontaudio Icons.h) -----
    inline const juce::String Play        = juce::String::charToString (juce::juce_wchar (0xf169));
    inline const juce::String Stop        = juce::String::charToString (juce::juce_wchar (0xf18d));
    inline const juce::String Pause       = juce::String::charToString (juce::juce_wchar (0xf166));
    inline const juce::String Record      = juce::String::charToString (juce::juce_wchar (0xf176));
    inline const juce::String Forward     = juce::String::charToString (juce::juce_wchar (0xf13a));
    inline const juce::String Backward    = juce::String::charToString (juce::juce_wchar (0xf114));
    inline const juce::String Microphone  = juce::String::charToString (juce::juce_wchar (0xf157));
    inline const juce::String Headphones  = juce::String::charToString (juce::juce_wchar (0xf13e));
    inline const juce::String Speaker     = juce::String::charToString (juce::juce_wchar (0xf189));
    inline const juce::String Mute        = juce::String::charToString (juce::juce_wchar (0xf162));
    inline const juce::String Powerswitch = juce::String::charToString (juce::juce_wchar (0xf16b));
    inline const juce::String CaretDown   = juce::String::charToString (juce::juce_wchar (0xf116));
    inline const juce::String CaretLeft   = juce::String::charToString (juce::juce_wchar (0xf117));
    inline const juce::String CaretRight  = juce::String::charToString (juce::juce_wchar (0xf118));
    inline const juce::String CaretUp     = juce::String::charToString (juce::juce_wchar (0xf119));

    // Source-group + transport extras
    inline const juce::String Diskio       = juce::String::charToString (juce::juce_wchar (0xf12a));
    inline const juce::String Waveform     = juce::String::charToString (juce::juce_wchar (0xf198));
    inline const juce::String Loop         = juce::String::charToString (juce::juce_wchar (0xf155));
    inline const juce::String Prev         = juce::String::charToString (juce::juce_wchar (0xf170));
    inline const juce::String Next         = juce::String::charToString (juce::juce_wchar (0xf163));
    inline const juce::String Sliderhandle2= juce::String::charToString (juce::juce_wchar (0xf185));

    // 7-segment digital glyphs — for vintage Nixie/LCD-style counters & displays
    inline const juce::String Digital0     = juce::String::charToString (juce::juce_wchar (0xf120));
    inline const juce::String Digital1     = juce::String::charToString (juce::juce_wchar (0xf121));
    inline const juce::String Digital2     = juce::String::charToString (juce::juce_wchar (0xf122));
    inline const juce::String Digital3     = juce::String::charToString (juce::juce_wchar (0xf123));
    inline const juce::String Digital4     = juce::String::charToString (juce::juce_wchar (0xf124));
    inline const juce::String Digital5     = juce::String::charToString (juce::juce_wchar (0xf125));
    inline const juce::String Digital6     = juce::String::charToString (juce::juce_wchar (0xf126));
    inline const juce::String Digital7     = juce::String::charToString (juce::juce_wchar (0xf127));
    inline const juce::String Digital8     = juce::String::charToString (juce::juce_wchar (0xf128));
    inline const juce::String Digital9     = juce::String::charToString (juce::juce_wchar (0xf129));

    /** Map ASCII digit '0'..'9' to its 7-segment glyph. */
    inline const juce::String& digitGlyph (char c)
    {
        switch (c)
        {
            case '0': return Digital0;
            case '1': return Digital1;
            case '2': return Digital2;
            case '3': return Digital3;
            case '4': return Digital4;
            case '5': return Digital5;
            case '6': return Digital6;
            case '7': return Digital7;
            case '8': return Digital8;
            case '9': return Digital9;
            default:  return Digital0;
        }
    }

    /** Singleton typeface — built once on first call. */
    inline juce::Typeface::Ptr getTypeface()
    {
        static juce::Typeface::Ptr tf =
            juce::Typeface::createSystemTypefaceFor (BinaryData::fontaudio_ttf,
                                                     (size_t) BinaryData::fontaudio_ttfSize);
        return tf;
    }

    /** Font configured for icon use (no kerning, no ligatures). */
    inline juce::Font getFont (float pixelHeight)
    {
        return juce::Font (juce::FontOptions (getTypeface()).withHeight (pixelHeight));
    }

    /** Centred icon draw — handles colour, size, and bounds in one call. */
    inline void drawIcon (juce::Graphics& g,
                          const juce::String& glyph,
                          juce::Rectangle<float> bounds,
                          juce::Colour colour,
                          float sizeOverride = 0.0f)
    {
        const float size = sizeOverride > 0.0f
                           ? sizeOverride
                           : juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.85f;
        g.setColour (colour);
        g.setFont   (getFont (size));
        g.drawText  (glyph, bounds, juce::Justification::centred, false);
    }

    /** Integer-bounds overload. */
    inline void drawIcon (juce::Graphics& g,
                          const juce::String& glyph,
                          juce::Rectangle<int> bounds,
                          juce::Colour colour,
                          float sizeOverride = 0.0f)
    {
        drawIcon (g, glyph, bounds.toFloat(), colour, sizeOverride);
    }
}
