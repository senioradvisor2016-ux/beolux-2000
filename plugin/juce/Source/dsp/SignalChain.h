/*  SignalChain — full BC2000DL DSP-pipeline (C++ port).

    Komplett kedja:
      input → mic-trafo (om lo-Z) → 3-buss-mixer → input-preamp (UW0029+2N2613)
        → echo (med per-speed delay) → record-amp + pre-emphasis → tape-saturation
        → wow & flutter → playback-amp + de-emphasis → tone-control
        → power-amp (om speaker-mode) → DC-block → balance/master

    Plus VU-meter-feed via atomic floats (för UI-thread).

    Plats: plugin/juce/Source/dsp/SignalChain.h
*/

#pragma once

#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "Constants.h"
#include "Ge2N2613Stage.h"
#include "GeLowNoiseStage.h"
#include "EqDIN1962.h"
#include "TapeSaturation.h"
#include "Echo.h"
#include "ToneAndOutput.h"
#include "WowFlutter.h"
#include "MicAndPower.h"
#include "Multiplay.h"
#include "PhonoPreamp.h"

namespace bc2000dl::dsp
{
    class SignalChain
    {
    public:
        SignalChain() = default;

        struct Parameters
        {
            TapeSpeed speed         { TapeSpeed::Speed19 };
            // Per-kanal mixer-gain (B&O dubbla skydepotentiometre)
            float micGain           { 0.0f };  // L
            float micGainR          { 0.0f };
            float phonoGain         { 0.0f };  // L
            float phonoGainR        { 0.0f };
            float radioGain         { 0.0f };  // L
            float radioGainR        { 0.0f };
            float bassDb            { 0.0f };
            float trebleDb          { 0.0f };
            float balance           { 0.0f };
            float masterVolume      { 0.75f };
            float biasAmount        { 1.0f };
            float saturationDrive   { 1.0f };  // L
            float saturationDriveR  { 1.0f };
            float wowFlutterAmount  { 1.0f };
            bool  echoEnabled       { false };
            float echoAmount        { 0.0f };  // L
            float echoAmountR       { 0.0f };
            bool  bypassTape        { false };
            bool  speakerMonitor    { false };
            bool  synchroplay       { false };
            int   multiplayGen      { 1 };
            bool  micLoZ            { true };

            // Manual-funktioner som tidigare missades
            bool  soundOnSound      { false };  // Manual S-on-S — PLAY L → REC R
            bool  publicAddress     { false };  // Manual #18 — duckar phono/radio när mic aktiv
            bool  monitorTrack1     { true };   // Manual #19 — lyssna på spår 1
            bool  monitorTrack2     { true };   // Manual #20 — lyssna på spår 2
                                                // (båda nedtryckta = stereo, bara 1 = L-only, bara 2 = R-only)

            // Monitor source/tape (manual #22):
            // 0 = Source (lyssna före tape — för rec-medhør)
            // 1 = Tape   (lyssna efter tape — default)
            int   monitorMode       { 1 };

            // Phono-input-mode (HS-switch i 8904002):
            // 0 = L (keramisk, flat respons)
            // 1 = H (magnetisk, RIAA-aktiverad)
            int   phonoMode         { 1 };

            // Tape-formel (plan §7): 0=Agfa, 1=BASF, 2=Scotch
            int   tapeFormula       { 0 };

            // Print-through (specs §10): geisterande pre-echo / post-echo från angränsande varv.
            // 0 = off, 0.01 = subtil, 0.05 = maximum (−26 dB ghost)
            float printThrough      { 0.0f };

            // Stereo-asymmetri (spec §10) — L/R Ge-stage-mismatch.
            // 0 = perfekt symmetri, 0.02 = autentisk 1968-tolerans (default).
            // L-kanal får +asym, R-kanal -asym → subtilt skild distortionsprofil.
            float stereoAsymmetry   { 0.02f };

            // Radio AUX input sensitivity (hardware sensitivity switch)
            // 0 = L (3 mV ceramic/low-level) → high preamp gain → more character
            // 1 = H (100 mV line/tuner)      → lower preamp gain → cleaner
            int   radioMode         { 0 };
        };

        void prepare (double sampleRate, int maximumBlockSize);
        void reset();
        void process (juce::AudioBuffer<float>& buffer);
        void setParameters (const Parameters& p);

        // VU-meter atomic feed (UI-thread läser)
        std::atomic<float> inputLevelL_dBFS  { -60.0f };
        std::atomic<float> inputLevelR_dBFS  { -60.0f };
        std::atomic<float> meterLevelL_dBFS  { -60.0f };
        std::atomic<float> meterLevelR_dBFS  { -60.0f };
        std::atomic<bool>  isRecordingL { false };
        std::atomic<bool>  isRecordingR { false };

        // ---- Spectrum-analyser FIFO (UI-thread reads, audio-thread writes) ----
        // Lock-free ring buffer of recent post-processing samples (mono mix).
        static constexpr int kSpecBufSize = 2048;
        float spectrumBuffer[kSpecBufSize] {};
        std::atomic<int> spectrumWriteIdx { 0 };

        // ---- ReelDeck / UI animation state (UI-thread reads, audio-thread writes) ----
        std::atomic<double> tapePositionSeconds { 0.0 };
        std::atomic<float>  wowCurrentAmp       { 0.0f };

    private:
        double sampleRate { 48000.0 };
        Parameters params;
        TapeSpeed lastSpeed { TapeSpeed::Speed19 };

        struct ChannelChain
        {
            // ----- Per-source input-preamps (3 parallella bussar enligt plan §1A) -----
            // Mic preamp: 8904004 (trafo + UW0029 + 2N2613)
            MicTransformer8012003 micTrafo;
            GeLowNoiseStage       micUw0029;
            Ge2N2613Stage         micN2613;

            // Phono preamp: 8904002 (UW0029 + RIAA + 2N2613)
            PhonoPreamp           phono;

            // Radio preamp: 8904003 (UW0029 + 2N2613, flat)
            GeLowNoiseStage       radioUw0029;
            Ge2N2613Stage         radioN2613;

            // ----- Record-amp efter input-mixern -----
            PreEmphasisDIN1962  recEq;
            GeLowNoiseStage     ac126_1;
            GeLowNoiseStage     ac126_2;

            // ----- Tape-block + playback + output -----
            TapeSaturation      tape;
            Multiplay           multiplay;
            WowFlutter          wowFlutter;
            PlaybackEqDIN1962   playEq;
            ToneControl         tone;
            PowerAmp8004014     powerAmp;
            juce::dsp::IIR::Filter<float> dcBlock;

            // Per-source noise generators (RT-safe, no allocation, no init cost)
            float        radioHumPhase    { 0.0f };
            float        phonoRumbleState { 0.0f };
            std::uint32_t noiseSeed       { 0u };
        };

        ChannelChain L, R;

        // Scratch-buffrar för parallell-mixern (allokeras i prepare(), RT-safe)
        juce::AudioBuffer<float> phonoScratch;  // 2 kanaler
        juce::AudioBuffer<float> radioScratch;

        Mixer3Bus mixer;
        Echo echoL, echoR;
        BalanceMaster balanceMaster;

        // Sticky safety-mute: när NaN/Inf har detekterats i output, fortsätter
        // vi muta i N block för att låta hela kedjan (FIR-oversampler, IIR-states,
        // delay lines) flushas innan vi börjar släppa igenom audio igen.
        int safetyMuteBlocks { 0 };

        // Initial asymmetri vid prepare() — matchas sedan av setParameters() via APVTS.
        // 0.008 ger ~46 dB kanalseparation per spec §8 (>45 dB target).
        // Tidigare 0.02 → 44.8 dB, 0.012 → 44.9 dB, 0.008 → ~46 dB.
        static constexpr float kAsymmetryAmount = 0.008f;

        void prepareChannel (ChannelChain& ch, double sr,
                             float asymOffset, std::uint32_t baseSeed);
        void processChannelChain (ChannelChain& ch, Echo& echo,
                                  juce::AudioBuffer<float>& buffer, int channel);
        float computeBlockRMSdBFS (const float* data, int n);
    };
}
