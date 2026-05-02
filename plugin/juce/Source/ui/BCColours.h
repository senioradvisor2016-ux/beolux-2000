/*  BC2000DL — Ferroflux-inspired färgpalett (warm cream + walnut + amber).

    Ersätter den tidigare svart/silver/röd-paletten med ett designspråk
    nära "Tape Deck Plugin.html" från handoff-bundle:
      - Warm cream brushed metal (face)
      - Walnut wood (yttre cheeks)
      - Amber lamps + ink-svart text
      - Sparsam rec-red för record/peak
*/

#pragma once

#include <juce_graphics/juce_graphics.h>

namespace bc2000dl::ui::colours
{
    // ----- Cream face (brushed-metal) -----
    inline const juce::Colour kCream1     { 0xffefe6cf };
    inline const juce::Colour kCream2     { 0xffe6d9b8 };
    inline const juce::Colour kCream3     { 0xffd8c79c };
    inline const juce::Colour kCreamShadow{ 0xff8a7a52 };

    // ----- Walnut (cheeks) -----
    inline const juce::Colour kWalnut1    { 0xff5a3520 };
    inline const juce::Colour kWalnut2    { 0xff3a1f10 };
    inline const juce::Colour kWalnutHi   { 0xff8a5a3a };
    inline const juce::Colour kWalnutDark { 0xff2a1408 };

    // ----- Ink (engraved text) -----
    inline const juce::Colour kInk        { 0xff1a1612 };
    inline const juce::Colour kInkSoft    { 0xff3a3128 };
    inline const juce::Colour kFaceLine   { 0xff7a6a48 };

    // ----- Amber (signal lamps) -----
    inline const juce::Colour kAmber      { 0xffffb14a };
    inline const juce::Colour kAmberGlow  { 0xffffd58a };
    inline const juce::Colour kAmberDim   { 0xff5a4824 };

    // ----- Red (record / peak / overload) -----
    inline const juce::Colour kRecordRed  { 0xffd8442a };
    inline const juce::Colour kPeakRed    { 0xffb8341c };
    inline const juce::Colour kRoomBg     { 0xff1d1916 };

    // ----- Dark recess (bezels / level bank / counter) -----
    inline const juce::Colour kRecessTop  { 0xff1a1410 };
    inline const juce::Colour kRecessBot  { 0xff0d0a07 };

    // ----- Compatibility aliases (gamla namn — i bruk i existerande kod) -----
    inline const juce::Colour kTeakDark   = kWalnutDark;
    inline const juce::Colour kTeak       = kWalnut1;
    inline const juce::Colour kTeakLight  = kWalnutHi;

    inline const juce::Colour kAluDark    = kRecessBot;
    inline const juce::Colour kAlu        = kRecessTop;
    inline const juce::Colour kAluLight   { 0xff2a2218 };

    inline const juce::Colour kSilver     { 0xffb8a472 };
    inline const juce::Colour kSilverLight{ 0xffe6d9b8 };

    inline const juce::Colour kKnabSatin  { 0xffd6c08a };

    inline const juce::Colour kVuGlass    { 0xffead9a8 };
    inline const juce::Colour kVuInk      = kInk;

    inline const juce::Colour kPlinth     { 0xff080604 };
    inline const juce::Colour kPaper      = kCream1;
    inline const juce::Colour kSoftGray   { 0xff9a8a5e };
    inline const juce::Colour kLineGray   = kFaceLine;
}
