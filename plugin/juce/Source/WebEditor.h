/*  BC2000DLWebEditor — Plugin-editor som hostar Ferroflux HTML/JS-frontenden
    via JUCE 8 WebBrowserComponent.

    - HTML laddas från embeddad BinaryData (juce_add_binary_data, BC2000DL_WebAssets)
    - JS pushar parameter-ändringar via window.__JUCE__.backend.emitEvent("paramChange",…)
    - C++ pushar meter-värden 30 Hz via emitEventIfBrowserIsVisible("meters", {L,R,peakL,peakR})
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <unordered_map>

#include "PluginProcessor.h"

class BC2000DLWebEditor : public juce::AudioProcessorEditor,
                          private juce::Timer,
                          private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit BC2000DLWebEditor (BC2000DLProcessor& p);
    ~BC2000DLWebEditor() override;

    void resized() override;

private:
    void timerCallback() override;
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void onParamChangeFromJS (const juce::var& payload);
    void onPresetChangeFromJS (const juce::var& payload);
    void pushFullStateToJS();
    void pushPresetListToJS();
    void pushOneParamToJS (const juce::String& id, float value01);

    BC2000DLProcessor& audioProc;
    juce::WebBrowserComponent webView;

    // Peak-hold state för VU
    float peakL { -20.0f }, peakR { -20.0f };

    // Track last value sent FROM JS to suppress echo when listener
    // fires for our own UI-originated change (avoids drag snap-back).
    std::unordered_map<juce::String, float> lastJSValue;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BC2000DLWebEditor)
};
