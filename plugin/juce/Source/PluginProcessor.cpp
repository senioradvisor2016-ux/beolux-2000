/*  BC2000DL — AudioProcessor implementation. */

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "WebEditor.h"

namespace
{
    using namespace juce;

    // Parameter-IDs (matchar specs.md §10)
    constexpr auto kP_Speed         = "speed";          // 0=4.75, 1=9.5, 2=19
    constexpr auto kP_MicGain       = "mic_gain";       // Bakåtkompatibelt — används som L
    constexpr auto kP_PhonoGain     = "phono_gain";
    constexpr auto kP_RadioGain     = "radio_gain";
    constexpr auto kP_MicGainR      = "mic_gain_r";
    constexpr auto kP_PhonoGainR    = "phono_gain_r";
    constexpr auto kP_RadioGainR    = "radio_gain_r";
    constexpr auto kP_SatDriveR     = "saturation_drive_r";
    constexpr auto kP_EchoAmountR   = "echo_amount_r";
    constexpr auto kP_BassDb        = "bass_db";
    constexpr auto kP_TrebleDb      = "treble_db";
    constexpr auto kP_Balance       = "balance";
    constexpr auto kP_Master        = "master_volume";
    constexpr auto kP_BiasAmount    = "bias_amount";
    constexpr auto kP_SatDrive      = "saturation_drive";
    constexpr auto kP_WowFlutter    = "wow_flutter";
    constexpr auto kP_EchoEnabled   = "echo_enabled";
    constexpr auto kP_EchoAmount    = "echo_amount";
    constexpr auto kP_BypassTape    = "bypass_tape";
    constexpr auto kP_SpeakerMon    = "speaker_monitor";
    constexpr auto kP_Synchroplay   = "synchroplay";
    constexpr auto kP_MultiplayGen  = "multiplay_gen";
    // Routing & arm
    constexpr auto kP_RecArm1       = "rec_arm_1";
    constexpr auto kP_RecArm2       = "rec_arm_2";
    constexpr auto kP_Track1        = "track_1";
    constexpr auto kP_Track2        = "track_2";
    constexpr auto kP_MonitorMode   = "monitor_mode";    // Source / Tape
    constexpr auto kP_Pause         = "pause";
    constexpr auto kP_SpeakerExt    = "speaker_ext";
    constexpr auto kP_SpeakerInt    = "speaker_int";
    constexpr auto kP_SpeakerMute   = "speaker_mute";
    constexpr auto kP_SoundOnSound  = "sos_enabled";
    constexpr auto kP_PublicAddress = "pa_enabled";       // P.A. (manual #18)
    // Input modes
    constexpr auto kP_MicLoZ        = "mic_loz";
    constexpr auto kP_PhonoMode     = "phono_mode";
    constexpr auto kP_RadioMode     = "radio_mode";
    constexpr auto kP_TapeFormula   = "tape_formula";  // 0=Agfa 1=BASF 2=Scotch
}

juce::AudioProcessorValueTreeState::ParameterLayout
BC2000DLProcessor::createParameterLayout()
{
    using P  = juce::AudioParameterFloat;
    using PI = juce::AudioParameterInt;
    using PB = juce::AudioParameterBool;
    using PC = juce::AudioParameterChoice;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Tape speed (3 val)
    layout.add (std::make_unique<PC> (
        juce::ParameterID { kP_Speed, 1 }, "Tape Speed",
        juce::StringArray { "4.75 cm/s", "9.5 cm/s", "19 cm/s" }, 2));

    // Input-bussar — L och R separata (B&O skydepotentiometre, dubbla knoppar).
    // Mic-bussen defaultar TILL 0.5 så pluginen producerar signal direkt vid
    // load (alla källor 0 = "tystnad innan reglage rörts" som var förvirrande).
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_MicGain, 1 }, "Mic Gain L",
        juce::NormalisableRange<float> { 0.0f, 1.0f }, 0.5f));
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_MicGainR, 1 }, "Mic Gain R",
        juce::NormalisableRange<float> { 0.0f, 1.0f }, 0.5f));
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_PhonoGain, 1 }, "Phono Gain L",
        juce::NormalisableRange<float> { 0.0f, 1.0f }, 0.0f));
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_PhonoGainR, 1 }, "Phono Gain R",
        juce::NormalisableRange<float> { 0.0f, 1.0f }, 0.0f));
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_RadioGain, 1 }, "Radio Gain L",
        juce::NormalisableRange<float> { 0.0f, 1.0f }, 0.0f));
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_RadioGainR, 1 }, "Radio Gain R",
        juce::NormalisableRange<float> { 0.0f, 1.0f }, 0.0f));

    // Tone (-12...+12 dB)
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_BassDb, 1 }, "Bass",
        juce::NormalisableRange<float> { -12.0f, 12.0f, 0.1f }, 0.0f));
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_TrebleDb, 1 }, "Treble",
        juce::NormalisableRange<float> { -12.0f, 12.0f, 0.1f }, 0.0f));

    // Balance + master
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_Balance, 1 }, "Balance",
        juce::NormalisableRange<float> { -1.0f, 1.0f, 0.01f }, 0.0f));
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_Master, 1 }, "Master Volume",
        juce::NormalisableRange<float> { 0.0f, 1.0f }, 0.75f));

    // Tape parameters
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_BiasAmount, 1 }, "Bias Amount",
        juce::NormalisableRange<float> { 0.5f, 1.5f, 0.01f }, 1.0f));
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_SatDrive, 1 }, "Saturation Drive L",
        juce::NormalisableRange<float> { 0.5f, 2.0f, 0.01f }, 1.0f));
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_SatDriveR, 1 }, "Saturation Drive R",
        juce::NormalisableRange<float> { 0.5f, 2.0f, 0.01f }, 1.0f));
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_WowFlutter, 1 }, "Wow & Flutter",
        juce::NormalisableRange<float> { 0.0f, 2.0f, 0.01f }, 0.3f));

    // Echo
    layout.add (std::make_unique<PB> (
        juce::ParameterID { kP_EchoEnabled, 1 }, "Echo Enabled", false));
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_EchoAmount, 1 }, "Echo Amount L",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f }, 0.0f));
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_EchoAmountR, 1 }, "Echo Amount R",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f }, 0.0f));

    // Mode-flaggor
    layout.add (std::make_unique<PB> (
        juce::ParameterID { kP_BypassTape, 1 }, "Bypass Tape", false));
    layout.add (std::make_unique<PB> (
        juce::ParameterID { kP_SpeakerMon, 1 }, "Speaker Monitor", false));
    layout.add (std::make_unique<PB> (
        juce::ParameterID { kP_Synchroplay, 1 }, "Synchroplay", false));

    // Multiplay generation (1–5)
    layout.add (std::make_unique<PI> (
        juce::ParameterID { kP_MultiplayGen, 1 }, "Multiplay Generation", 1, 5, 1));

    // ----- Routing & arm-knappar (manual #19, 20, 24, 25) -----
    layout.add (std::make_unique<PB> (juce::ParameterID { kP_RecArm1, 1 }, "Record Arm 1", false));
    layout.add (std::make_unique<PB> (juce::ParameterID { kP_RecArm2, 1 }, "Record Arm 2", false));
    layout.add (std::make_unique<PB> (juce::ParameterID { kP_Track1, 1 },  "Track 1",      true));
    layout.add (std::make_unique<PB> (juce::ParameterID { kP_Track2, 1 },  "Track 2",      true));

    // Monitor source/tape (manual #22)
    layout.add (std::make_unique<PC> (
        juce::ParameterID { kP_MonitorMode, 1 }, "Monitor Mode",
        juce::StringArray { "Source", "Tape" }, 1));

    // Momentanstop (manual #11)
    layout.add (std::make_unique<PB> (juce::ParameterID { kP_Pause, 1 }, "Pause", false));

    // Speaker A/B/Mute (manual #4, 5, 6)
    layout.add (std::make_unique<PB> (juce::ParameterID { kP_SpeakerExt, 1 },  "Speaker EXT",  false));
    layout.add (std::make_unique<PB> (juce::ParameterID { kP_SpeakerInt, 1 },  "Speaker INT",  false));
    layout.add (std::make_unique<PB> (juce::ParameterID { kP_SpeakerMute, 1 }, "Speaker Mute", false));

    // S-on-S (manualens "S on S"-knapp) — kanal-korsmix (PLAY L → REC R)
    layout.add (std::make_unique<PB> (juce::ParameterID { kP_SoundOnSound, 1 }, "Sound on Sound", false));

    // P.A. Public Address (manual #18 s.8) — duckar phono/radio när mic aktiv
    layout.add (std::make_unique<PB> (juce::ParameterID { kP_PublicAddress, 1 }, "P.A. Mode", false));

    // Input modes
    layout.add (std::make_unique<PB> (juce::ParameterID { kP_MicLoZ, 1 }, "Mic Lo-Z", true));
    layout.add (std::make_unique<PC> (
        juce::ParameterID { kP_PhonoMode, 1 }, "Phono Mode",
        juce::StringArray { "L (ceramic)", "H (magnetic)" }, 1));
    layout.add (std::make_unique<PC> (
        juce::ParameterID { kP_RadioMode, 1 }, "Radio Mode",
        juce::StringArray { "L (3 mV)", "H (100 mV)" }, 0));

    // Tape-formel (plan §7)
    layout.add (std::make_unique<PC> (
        juce::ParameterID { kP_TapeFormula, 1 }, "Tape Formula",
        juce::StringArray { "Agfa", "BASF", "Scotch" }, 0));

    return layout;
}

BC2000DLProcessor::BC2000DLProcessor()
    : juce::AudioProcessor (BusesProperties()
                            .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                            .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BC2000DL", createParameterLayout())
{
}

void BC2000DLProcessor::prepareToPlay (double sr, int samplesPerBlock)
{
    chain.prepare (sr, samplesPerBlock);
    updateChainParameters();
}

void BC2000DLProcessor::releaseResources()
{
    chain.reset();
}

bool BC2000DLProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Stöd för stereo in/ut
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet()  != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void BC2000DLProcessor::updateChainParameters()
{
    using namespace bc2000dl::dsp;
    SignalChain::Parameters p;

    auto getF = [this] (juce::String id)
    {
        return apvts.getRawParameterValue (id)->load();
    };

    const int speedIdx = static_cast<int> (getF (kP_Speed));
    p.speed = (speedIdx == 0 ? TapeSpeed::Speed475 :
              (speedIdx == 1 ? TapeSpeed::Speed95 : TapeSpeed::Speed19));

    p.micGain          = getF (kP_MicGain);
    p.micGainR         = getF (kP_MicGainR);
    p.phonoGain        = getF (kP_PhonoGain);
    p.phonoGainR       = getF (kP_PhonoGainR);
    p.radioGain        = getF (kP_RadioGain);
    p.radioGainR       = getF (kP_RadioGainR);
    p.bassDb           = getF (kP_BassDb);
    p.trebleDb         = getF (kP_TrebleDb);
    p.balance          = getF (kP_Balance);
    p.masterVolume     = getF (kP_Master);
    p.biasAmount       = getF (kP_BiasAmount);
    p.saturationDrive  = getF (kP_SatDrive);
    p.saturationDriveR = getF (kP_SatDriveR);
    p.wowFlutterAmount = getF (kP_WowFlutter);
    p.echoEnabled      = getF (kP_EchoEnabled) > 0.5f;
    p.echoAmount       = getF (kP_EchoAmount);
    p.echoAmountR      = getF (kP_EchoAmountR);
    p.bypassTape       = getF (kP_BypassTape) > 0.5f;
    p.speakerMonitor   = getF (kP_SpeakerMon) > 0.5f;
    p.synchroplay      = getF (kP_Synchroplay) > 0.5f;
    p.multiplayGen     = static_cast<int> (getF (kP_MultiplayGen));
    p.micLoZ           = getF (kP_MicLoZ) > 0.5f;
    p.soundOnSound     = getF (kP_SoundOnSound) > 0.5f;
    p.publicAddress    = getF (kP_PublicAddress) > 0.5f;
    p.monitorTrack1    = getF ("track_1") > 0.5f;
    p.monitorTrack2    = getF ("track_2") > 0.5f;
    p.monitorMode      = static_cast<int> (getF (kP_MonitorMode));
    p.phonoMode        = static_cast<int> (getF (kP_PhonoMode));
    p.tapeFormula      = static_cast<int> (getF (kP_TapeFormula));

    // P.A. mode — duckar phono+radio när mic aktiv (-12 dB)
    if (p.publicAddress && (p.micGain > 0.05f || p.micGainR > 0.05f))
    {
        constexpr float duckGain = 0.25f;  // -12 dB
        p.phonoGain   *= duckGain;
        p.phonoGainR  *= duckGain;
        p.radioGain   *= duckGain;
        p.radioGainR  *= duckGain;
    }

    // Tysta output om alla speakers off + mute eller om Pause
    if (getF (kP_Pause) > 0.5f
        || getF (kP_SpeakerMute) > 0.5f
        || (getF (kP_SpeakerExt) < 0.5f && getF (kP_SpeakerInt) < 0.5f
            && getF (kP_SpeakerMute) < 0.5f && p.speakerMonitor))
    {
        // Pause/mute → master_volume * 0
        p.masterVolume = 0.0f;
    }

    chain.setParameters (p);
}

void BC2000DLProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);
    juce::ScopedNoDenormals noDenormals;

    updateChainParameters();
    chain.process (buffer);
}

juce::AudioProcessorEditor* BC2000DLProcessor::createEditor()
{
    // v29 BEOFLUX 2000DL — TEAC-platform WebView UI
    return new BC2000DLWebEditor (*this);
}

void BC2000DLProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void BC2000DLProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// VST3 / AU entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BC2000DLProcessor();
}
