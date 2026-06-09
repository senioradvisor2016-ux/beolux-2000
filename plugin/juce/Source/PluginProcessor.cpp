/*  BC2000DL — AudioProcessor implementation. */

#include "PluginProcessor.h"
#include "WireframeEditor.h"
#include "presets/Presets.h"
#include <cstring>

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
    constexpr auto kP_MasterR       = "master_volume_r";   // oberoende höger master
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
    constexpr auto kP_TapeFormula      = "tape_formula";      // 0=Agfa 1=BASF 2=Scotch
    constexpr auto kP_PrintThrough     = "print_through";     // 0..0.05 (specs §10)
    constexpr auto kP_StereoAsymmetry  = "stereo_asymmetry";  // 0..0.05 (spec §10)
    // v60.3 — nya hardware-switchar från schema 9224002/9224003
    constexpr auto kP_BiasType         = "bias_type";         // 0=NORMAL Fe₂O₃, 1=HIGH CrO₂
    constexpr auto kP_TrackWidth       = "track_width";       // 0=1/4 track, 1=1/2 track
    // WireframeEditor (DELUXE-UI) — params som UI:t binder till (porterade från v30.0).
    constexpr auto kP_EchoTime         = "echo_time";         // 30..350 ms (user override)
    constexpr auto kP_EchoFeedback     = "echo_feedback";     // 0..1.0 (user override)
    constexpr auto kP_BiasAmountR      = "bias_amount_r";     // L/R separate bias-trimmers
    constexpr auto kP_MicMode          = "mic_mode";          // 0=LoZ 50Ω · 1=LoZ 200Ω · 2=HiZ
    constexpr auto kP_MainsHum         = "mains_hum";         // 0..0.1 — mains-hum intensitet
    constexpr auto kP_MainsHumFreq     = "mains_hum_freq";    // 0=50Hz · 1=60Hz
    constexpr auto kP_InputTrim        = "input_trim";        // -24..+24 dB pre-DSP
    constexpr auto kP_OutputTrim       = "output_trim";       // -24..+24 dB post-DSP makeup
    constexpr auto kP_Mix              = "mix";               // 0..1 dry/wet (1 = wet)
    constexpr auto kP_TapeNoise        = "tape_noise";        // 0..2 bandbrus-skala (1 = spec)
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
        juce::ParameterID { kP_Master, 1 }, "Master Volume L",
        juce::NormalisableRange<float> { 0.0f, 1.0f }, 0.85f));
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_MasterR, 1 }, "Master Volume R",
        juce::NormalisableRange<float> { 0.0f, 1.0f }, 0.85f));

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

    // Print-through (specs §10 — geisterande pre/post-echo från angränsande varv)
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_PrintThrough, 1 }, "Print Through",
        juce::NormalisableRange<float> { 0.0f, 0.05f, 0.001f }, 0.0f));

    // Stereo-asymmetri (spec §10 — L/R Ge-stage-mismatch; 0.02 = autentisk 1968)
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_StereoAsymmetry, 1 }, "Stereo Asymmetry",
        juce::NormalisableRange<float> { 0.0f, 0.05f, 0.001f }, 0.02f));

    // v60.3 — Bias-typ NORMAL/HIGH (schema 9224002 "NORMAL HIGH" switch)
    // ASCII-only labels — JUCE-default-fonten saknar Unicode-subscripts.
    layout.add (std::make_unique<PC> (
        juce::ParameterID { kP_BiasType, 1 }, "Bias Type",
        juce::StringArray { "NORMAL (Fe2O3)", "HIGH (CrO2)" }, 0));

    // v60.3 — Track-width 1/4 vs 1/2 (schema 9224003 "1/2 1/4" selector)
    layout.add (std::make_unique<PC> (
        juce::ParameterID { kP_TrackWidth, 1 }, "Track Width",
        juce::StringArray { "1/4 track stereo", "1/2 track stereo" }, 0));

    // ===== DELUXE-UI-params (WireframeEditor) — porterade från v30.0 =====
    // Echo time + feedback (plugin-extension utöver auto-speed/curve)
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_EchoTime, 1 }, "Echo Time",
        juce::NormalisableRange<float> { 30.0f, 350.0f, 1.0f }, 150.0f));
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_EchoFeedback, 1 }, "Echo Feedback",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f }, 0.5f));
    // Bias R-trimmer — separat från bias_amount (behandlas nu som L)
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_BiasAmountR, 1 }, "Bias Amount R",
        juce::NormalisableRange<float> { 0.5f, 1.5f, 0.01f }, 1.0f));
    // Mic 3-pos mode (manual p.6 + Studio Sound 1968)
    layout.add (std::make_unique<PC> (
        juce::ParameterID { kP_MicMode, 1 }, "Mic Mode",
        juce::StringArray { "LoZ 50Ohm", "LoZ 200Ohm", "HiZ Crystal" }, 1));
    // Mains hum — 1968-amparna hade "slight unobtrusive mains hum"
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_MainsHum, 1 }, "Mains Hum",
        juce::NormalisableRange<float> { 0.0f, 0.1f, 0.001f }, 0.0f));
    layout.add (std::make_unique<PC> (
        juce::ParameterID { kP_MainsHumFreq, 1 }, "Mains Hum Frequency",
        juce::StringArray { "50 Hz", "60 Hz" }, 0));
    // Plugin-utility I/O-trim (modern gain-staging)
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_InputTrim, 1 }, "Input Trim",
        juce::NormalisableRange<float> { -24.0f, 24.0f, 0.1f }, 0.0f));
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_OutputTrim, 1 }, "Output Trim",
        juce::NormalisableRange<float> { -24.0f, 24.0f, 0.1f }, 0.0f));

    // ===== Fas 2 (UAD-paritet) =====
    // MIX — dry/wet med latensjusterad dry-väg (parallellbearbetning)
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_Mix, 1 }, "Mix",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f }, 1.0f));
    // TAPE NOISE — bandbrus-skala (0 = av, 1 = spec-nivå, 2 = sliten tape)
    layout.add (std::make_unique<P> (
        juce::ParameterID { kP_TapeNoise, 1 }, "Tape Noise",
        juce::NormalisableRange<float> { 0.0f, 2.0f, 0.01f }, 1.0f));

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

    // PDC: wow/flutter-basdelay + tape-oversamplerns gruppfördröjning.
    // Bypass/Source-vägen är delay-matchad i SignalChain → gäller alla lägen.
    setLatencySamples (chain.getLatencySamples());

    updateChainParameters();
}

void BC2000DLProcessor::releaseResources()
{
    chain.reset();
}

bool BC2000DLProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Stöd BÅDE mono och stereo (in måste matcha ut — ingen upp/ned-mix).
    // KRITISKT för Ableton Live: en plugin som BARA accepterade stereo nekades
    // på MONOSPÅR → Live kunde inte ge en giltig buss → spåret blev TYST.
    // Mono-vägen processar kanal 0 genom hela kedjan (DSP:n är kanal-säker:
    // SoS/monitor/cross-bleed är numCh>=2-gardade, och BalanceMaster applicerar
    // master-volym på kanal 0 i mono).
    const auto mainOut = layouts.getMainOutputChannelSet();
    const auto mainIn  = layouts.getMainInputChannelSet();
    if (mainOut != juce::AudioChannelSet::mono()
        && mainOut != juce::AudioChannelSet::stereo())
        return false;
    if (mainIn != mainOut)
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
    p.masterVolumeR    = getF (kP_MasterR);
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
    p.radioMode        = static_cast<int> (getF (kP_RadioMode));
    p.printThrough     = getF (kP_PrintThrough);
    p.stereoAsymmetry  = getF (kP_StereoAsymmetry);
    p.biasType         = static_cast<int> (getF (kP_BiasType));
    p.trackWidth       = static_cast<int> (getF (kP_TrackWidth));

    // ===== DELUXE-UI-params (WireframeEditor) =====
    p.inputTrimDb      = getF (kP_InputTrim);
    p.outputTrimDb     = getF (kP_OutputTrim);
    p.biasAmountR      = getF (kP_BiasAmountR);
    p.micMode          = static_cast<int> (getF (kP_MicMode));
    p.mainsHum         = getF (kP_MainsHum);
    p.mainsHumFreqHz   = getF (kP_MainsHumFreq) > 0.5f ? 60.0f : 50.0f;
    p.echoTimeMs       = getF (kP_EchoTime);
    p.echoFeedback     = getF (kP_EchoFeedback);
    p.mix              = getF (kP_Mix);
    p.tapeNoise        = getF (kP_TapeNoise);
    // DELUXE MIC-mode driver micLoZ-DSP:n: läge 0/1 = LoZ, 2 = HiZ Crystal.
    p.micLoZ           = (p.micMode < 2);

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
        // Pause/mute → master_volume * 0 (båda kanaler)
        p.masterVolume  = 0.0f;
        p.masterVolumeR = 0.0f;
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

    // Sync tape-transport counter to DAW playhead when available.
    // This makes the UI counter track the actual session time rather than
    // accumulated processing time — rewind in the DAW rewinds the reel counter.
    // In standalone / when no host time is available the chain's own accumulator
    // (advanced in chain.process) is used instead.
    if (auto* ph = getPlayHead())
    {
        if (const auto pos = ph->getPosition())
        {
            if (const auto t = pos->getTimeInSeconds())
                chain.tapePositionSeconds.store (*t, std::memory_order_relaxed);
        }
    }
}

juce::AudioProcessorEditor* BC2000DLProcessor::createEditor()
{
    // DELUXE-UI — WireframeEditor (matchar design_inbox/beolux_2000_deluxe_wireframe.html
    // + B&O Beocord 2000 De Luxe-manualen). De gamla editorerna
    // (NativeEditor/BC2000DLEditor/WebEditor) togs bort ur bygget i v62.
    return new bc2000dl::ui::WireframeEditor (*this);
}

int BC2000DLProcessor::getNumPrograms()  { return bc2000dl::kNumPresets; }
int BC2000DLProcessor::getCurrentProgram() { return currentProgramIndex; }

const juce::String BC2000DLProcessor::getProgramName (int index)
{
    if (index < 0 || index >= bc2000dl::kNumPresets) return {};
    return juce::String (bc2000dl::kPresets[index].name);
}

void BC2000DLProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= bc2000dl::kNumPresets) return;
    currentProgramIndex = index;
    const auto& p = bc2000dl::kPresets[index];

    auto set = [&] (const juce::String& id, float actualVal)
    {
        if (auto* prm = apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (actualVal));
    };

    set ("speed",              (float) p.speed);
    set ("tape_formula",       (float) p.tape_formula);
    set ("mic_gain",           p.mic_gain);
    set ("mic_gain_r",         p.mic_gain_r);
    set ("phono_gain",         p.phono_gain);
    set ("phono_gain_r",       p.phono_gain_r);
    set ("radio_gain",         p.radio_gain);
    set ("radio_gain_r",       p.radio_gain_r);
    set ("saturation_drive",   p.saturation_drive);
    set ("saturation_drive_r", p.saturation_drive_r);
    set ("bias_amount",        p.bias_amount);
    set ("wow_flutter",        p.wow_flutter);
    set ("multiplay_gen",      (float) p.multiplay_gen);
    set ("bass_db",            p.bass_db);
    set ("treble_db",          p.treble_db);
    set ("balance",            p.balance);
    set ("master_volume",      p.master_volume);
    set ("echo_enabled",       p.echo_enabled ? 1.0f : 0.0f);
    set ("echo_amount",        p.echo_amount);
    set ("echo_amount_r",      p.echo_amount_r);
    // Nollställ alltid transienta tillstånd — förhindrar kvar-mute efter pause
    set ("bypass_tape",     0.0f);
    set ("pause",           0.0f);
    set ("speaker_monitor", 0.0f);
    set ("synchroplay",     0.0f);
    set ("sos_enabled",     0.0f);
    set ("pa_enabled",      0.0f);
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
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            // HÄRDNING (steg 1): tvinga varje PARAM:s "value" till en ÄNDLIG double
            // INNAN replaceState. APVTS läser värdet internt (choice/int castar
            // NaN→int = UB i JUCE:s String-konvertering, och "nan"/"inf"-strängar
            // från en korrupt fil överlever annars). Genom att skriva tillbaka en
            // ren double ser APVTS aldrig ett icke-ändligt värde.
            auto tree = juce::ValueTree::fromXml (*xml);
            for (auto child : tree)
                if (child.hasType ("PARAM"))
                {
                    double v = (double) child["value"];
                    if (! std::isfinite (v)) v = 0.0;
                    child.setProperty ("value", v, nullptr);
                }

            apvts.replaceState (tree);

            // JUCE 8: replaceState() updates the ValueTree synchronously but the
            // AudioParameter getValue() can remain stale (atomic not yet propagated).
            // Read direkt från ValueTree-barnen och push till setValueNotifyingHost.
            // KRITISKT: child["value"] är det FAKTISKA (denormaliserade) värdet, men
            // setValueNotifyingHost förväntar NORMALISERAT 0..1 → konvertera först.
            // Utan convertTo0to1 korrumperades alla params med icke-0..1-range
            // (bias, trims, mains_hum, echo_time, ton-dB …) vid varje session-laddning.
            for (auto child : apvts.state)
            {
                if (child.hasType ("PARAM"))
                {
                    if (auto* prm = apvts.getParameter (child["id"].toString()))
                    {
                        // HÄRDNING: en korrupt projektfil / illvillig preset kan mata
                        // in NaN/inf (överlever t.o.m. xml-round-trip via strtod) eller
                        // absurda värden. Sanera → default vid icke-ändligt, klamp till
                        // [0,1] efter normalisering så DSP:n aldrig får ett NaN-param.
                        const float raw  = (float) child["value"];
                        const float norm = std::isfinite (raw)
                                             ? prm->convertTo0to1 (raw)
                                             : prm->getDefaultValue();
                        prm->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm));
                    }
                }
            }
        }
    }
}

// VST3 / AU entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BC2000DLProcessor();
}
