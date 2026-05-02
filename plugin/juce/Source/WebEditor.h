/*  BC2000DLWebEditor — Plugin-editor som hostar Ferroflux HTML/JS-frontenden
    via JUCE 8 WebBrowserComponent.

    - HTML laddas från embeddad BinaryData (juce_add_binary_data, BC2000DL_WebAssets)
    - JS pushar parameter-ändringar via window.__JUCE__.backend.emitEvent("paramChange",…)
    - C++ pushar meter-värden 30 Hz via emitEventIfBrowserIsVisible("meters", {L,R,peakL,peakR})
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "PluginProcessor.h"

class BC2000DLWebEditor : public juce::AudioProcessorEditor,
                          private juce::Timer
{
public:
    explicit BC2000DLWebEditor (BC2000DLProcessor& p);
    ~BC2000DLWebEditor() override = default;

    void resized() override;

private:
    void timerCallback() override;
    void onParamChangeFromJS (const juce::var& payload);

    BC2000DLProcessor& processor;
    juce::WebBrowserComponent webView;

    // Peak-hold state för VU
    float peakL { -20.0f }, peakR { -20.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BC2000DLWebEditor)
};
