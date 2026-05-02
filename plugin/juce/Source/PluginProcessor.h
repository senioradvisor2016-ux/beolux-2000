/*  BC2000DL — AudioProcessor

    Top-level VST3/AU-plugin. Exponerar 26 kontroller från manualens
    Anvisning Side 1 som JUCE AudioProcessorValueTreeState-parametrar.

    Plats: plugin/juce/Source/PluginProcessor.h
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/SignalChain.h"

class BC2000DLProcessor : public juce::AudioProcessor
{
public:
    BC2000DLProcessor();
    ~BC2000DLProcessor() override = default;

    // ----- AudioProcessor interface -----
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>& buffer,
                       juce::MidiBuffer& midi) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                  { return true; }

    const juce::String getName() const override      { return "BC2000DL"; }
    bool acceptsMidi() const override                { return false; }
    bool producesMidi() const override               { return false; }
    bool isMidiEffect() const override               { return false; }
    double getTailLengthSeconds() const override     { return 0.4; }  // echo-tail vid 4.75 cm/s

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ----- Public: state-tree + chain accessor (for editor) -----
    juce::AudioProcessorValueTreeState apvts;
    bc2000dl::dsp::SignalChain& getChain() { return chain; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateChainParameters();

    bc2000dl::dsp::SignalChain chain;  // public-accessed via getChain()
    int currentProgramIndex { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BC2000DLProcessor)
};
