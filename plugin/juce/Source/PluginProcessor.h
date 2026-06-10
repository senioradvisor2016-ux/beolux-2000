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

    const juce::String getName() const override      { return "Beolux 2000"; }
    bool acceptsMidi() const override                { return false; }
    bool producesMidi() const override               { return false; }
    bool isMidiEffect() const override               { return false; }
    double getTailLengthSeconds() const override     { return 1.5; }  // 300 ms echo + self-osc decay margin

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

    // ===== A/B-compare (UAD-paritet) =====
    // Två settings-slots. Det LIVE APVTS-trädet är den aktiva slottens
    // arbetskopia; den inaktiva hålls fryst. Persisteras i projektets state.
    void abRecall (int slot);        // växla aktiv slot (sparar live → gammal, laddar ny)
    void abCopyActiveToOther();      // kopiera aktiv → andra (matcha A och B)
    void abSwap();                   // byt A↔B-innehåll, ladda om aktiv
    int  getABSlot() const { return abSlot; }

    // ===== User-presets på disk =====
    static juce::File userPresetDirectory();          // skapas vid behov
    bool saveUserPreset (const juce::String& name);   // skriv live-state → <dir>/<name>.beolux
    bool loadUserPresetFile (const juce::File&);       // läs + applicera
    juce::Array<juce::File> listUserPresets();

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateChainParameters();

    // Applicera ett (saneringshärdat) state-träd på live-parametrarna.
    // Delad av setStateInformation + A/B-recall + user-preset-load.
    void applyStateTree (juce::ValueTree tree);

    bc2000dl::dsp::SignalChain chain;  // public-accessed via getChain()
    int currentProgramIndex { 0 };

    // A/B-slots (giltiga efter första abRecall/store; init = aktuellt state)
    juce::ValueTree abState[2];
    int abSlot { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BC2000DLProcessor)
};
