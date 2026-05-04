/*  SignalChain implementation — full pipeline. */

#include "SignalChain.h"
#include <cmath>

namespace bc2000dl::dsp
{
    void SignalChain::prepareChannel (ChannelChain& ch, double sr,
                                      float asymOffset, std::uint32_t baseSeed)
    {
        // Reducerade per-stage gains för att modellera real-hårdvarans
        // feedback-limited beteende. Tidigare 30+20+12+10 = 72 dB kaskad
        // saturerade alla stages oavsett input. 4+2+3+2 = 11 dB låter tape-
        // blocket dominera färgningen (autentiskt — ej preamp-saturation).
        // Validering 2026-04-30 visade att detta ger:
        //   Kanalseparation: 45.6 dB ✓ (var 3.0)
        //   S/N: 46 dB (var 2.8, target 55)
        //   THD i nominell drift: signifikant lägre än med 72 dB-cascade
        ch.micTrafo.prepare (sr);
        ch.micUw0029.prepare (sr, GeStageType::UW0029, 4.0,
                              asymOffset, baseSeed + 100);
        ch.micN2613.prepare (sr, 2.0, asymOffset, baseSeed + 101);

        ch.phono.prepare (sr, asymOffset, baseSeed + 150);

        ch.radioUw0029.prepare (sr, GeStageType::UW0029, 4.0,
                                asymOffset, baseSeed + 170);
        ch.radioN2613.prepare (sr, 2.0, asymOffset, baseSeed + 171);

        ch.recEq.prepare (sr);
        ch.ac126_1.prepare (sr, GeStageType::AC126, 3.0,
                             asymOffset, baseSeed + 200);
        ch.ac126_2.prepare (sr, GeStageType::AC126, 2.0,
                             asymOffset, baseSeed + 201);
        ch.tape.prepare (sr, baseSeed + 300);
        ch.multiplay.prepare (sr, baseSeed + 350);
        ch.wowFlutter.prepare (sr);
        ch.playEq.prepare (sr);
        ch.tone.prepare (sr);
        ch.powerAmp.prepare (sr);

        // DC-block (HP @ 20 Hz)
        const auto coef = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 20.0f);
        ch.dcBlock.coefficients = coef;
        ch.dcBlock.reset();

        // Per-source noise init (different seeds for L/R to decorrelate noise floors)
        ch.radioHumPhase    = 0.0f;
        ch.phonoRumbleState = 0.0f;
        ch.noiseSeed        = baseSeed ^ 0xDEADBEEFu;
    }

    void SignalChain::prepare (double sr, int maxBlock)
    {
        sampleRate = sr;
        const int blockSize = juce::jmax (maxBlock, 64);

        // Allokera scratch-buffrar för parallell-input-mixern (RT-safe)
        phonoScratch.setSize (2, blockSize, false, true, true);
        radioScratch.setSize (2, blockSize, false, true, true);

        prepareChannel (L, sr, +kAsymmetryAmount, 1000);
        prepareChannel (R, sr, -kAsymmetryAmount, 2000);

        echoL.prepare (sr);
        echoR.prepare (sr);
        balanceMaster.prepare (sr, blockSize);

        // Initial speed-config
        L.recEq.setSpeed (params.speed);  R.recEq.setSpeed (params.speed);
        L.playEq.setSpeed (params.speed); R.playEq.setSpeed (params.speed);
        L.tape.setSpeed (params.speed);   R.tape.setSpeed (params.speed);
        L.wowFlutter.setSpeed (params.speed); R.wowFlutter.setSpeed (params.speed);
        echoL.setSpeed (params.speed);    echoR.setSpeed (params.speed);
        lastSpeed = params.speed;
    }

    void SignalChain::reset()
    {
        for (auto* ch : { &L, &R })
        {
            ch->micTrafo.reset();
            ch->micUw0029.reset();
            ch->micN2613.reset();
            ch->phono.reset();
            ch->radioUw0029.reset();
            ch->radioN2613.reset();
            ch->recEq.reset();
            ch->ac126_1.reset(); ch->ac126_2.reset();
            ch->tape.reset();
            ch->multiplay.reset();
            ch->wowFlutter.reset();
            ch->playEq.reset();
            ch->tone.reset();
            ch->powerAmp.reset();
            ch->dcBlock.reset();
        }
        echoL.reset(); echoR.reset();
        balanceMaster.reset();
        phonoScratch.clear();
        radioScratch.clear();
        // Reset per-source noise and modulation phases
        L.radioHumPhase = R.radioHumPhase = 0.0f;
        L.phonoRumbleState = R.phonoRumbleState = 0.0f;

        // Reset tape-transport counters so the UI counter returns to 0000
        // when the DAW transport rewinds or the processor is re-initialised.
        tapePositionSeconds.store (0.0, std::memory_order_relaxed);
    }

    void SignalChain::setParameters (const Parameters& p)
    {
        params = p;

        if (p.speed != lastSpeed)
        {
            L.recEq.setSpeed (p.speed);  R.recEq.setSpeed (p.speed);
            L.playEq.setSpeed (p.speed); R.playEq.setSpeed (p.speed);
            L.tape.setSpeed (p.speed);   R.tape.setSpeed (p.speed);
            L.wowFlutter.setSpeed (p.speed); R.wowFlutter.setSpeed (p.speed);
            echoL.setSpeed (p.speed);    echoR.setSpeed (p.speed);
            lastSpeed = p.speed;
        }

        L.tape.setBiasAmount (p.biasAmount);     R.tape.setBiasAmount (p.biasAmount);
        // Per-kanal saturation-drive (dubbla skydepotentiometre)
        L.tape.setSaturationDrive (p.saturationDrive);  R.tape.setSaturationDrive (p.saturationDriveR);
        L.wowFlutter.setAmount (p.wowFlutterAmount);    R.wowFlutter.setAmount (p.wowFlutterAmount);
        L.powerAmp.setEnabled (p.speakerMonitor);       R.powerAmp.setEnabled (p.speakerMonitor);
        L.multiplay.setGeneration (p.multiplayGen);     R.multiplay.setGeneration (p.multiplayGen);
        L.multiplay.setEnabled (p.multiplayGen > 1);    R.multiplay.setEnabled (p.multiplayGen > 1);

        // Per-kanal echo-amount
        echoL.setEnabled (p.echoEnabled); echoR.setEnabled (p.echoEnabled);
        echoL.setAmount (p.echoAmount);   echoR.setAmount (p.echoAmountR);

        L.tone.setBassDb (p.bassDb);     R.tone.setBassDb (p.bassDb);
        L.tone.setTrebleDb (p.trebleDb); R.tone.setTrebleDb (p.trebleDb);

        // PhonoPreamp-mode (H/L)
        L.phono.setMode (p.phonoMode == 0 ? PhonoMode::L : PhonoMode::H);
        R.phono.setMode (p.phonoMode == 0 ? PhonoMode::L : PhonoMode::H);

        // Radio input sensitivity: L → high preamp gain, H → backed-off (cleaner)
        const float radioPreampGainDb = (p.radioMode == 1) ? -2.0f : 4.0f;
        L.radioUw0029.setGain (radioPreampGainDb);
        R.radioUw0029.setGain (radioPreampGainDb);

        // Tape-formel (plan §7)
        const auto tf = (p.tapeFormula == 1 ? TapeFormula::BASF :
                        p.tapeFormula == 2 ? TapeFormula::Scotch : TapeFormula::Agfa);
        L.tape.setFormula (tf); R.tape.setFormula (tf);

        // Print-through (specs §10)
        L.tape.setPrintThrough (p.printThrough); R.tape.setPrintThrough (p.printThrough);

        // Stereo-asymmetri (spec §10) — L +asym, R −asym på alla Ge-stages
        {
            const double la =  static_cast<double> (p.stereoAsymmetry);
            const double ra = -static_cast<double> (p.stereoAsymmetry);
            L.micUw0029.setChannelAsymmetry (la);    R.micUw0029.setChannelAsymmetry (ra);
            L.micN2613.setChannelAsymmetry  (la);    R.micN2613.setChannelAsymmetry  (ra);
            L.phono.setChannelAsymmetry     (static_cast<float> (la));
            R.phono.setChannelAsymmetry     (static_cast<float> (ra));
            L.radioUw0029.setChannelAsymmetry (la);  R.radioUw0029.setChannelAsymmetry (ra);
            L.radioN2613.setChannelAsymmetry  (la);  R.radioN2613.setChannelAsymmetry  (ra);
            L.ac126_1.setChannelAsymmetry   (la);    R.ac126_1.setChannelAsymmetry   (ra);
            L.ac126_2.setChannelAsymmetry   (la);    R.ac126_2.setChannelAsymmetry   (ra);
        }

        // Notera: mixer.setGains används bara för totalgain (för bypass-mode).
        // Per-kanal-gain hanteras i processChannelChain.
        mixer.setGains (p.micGain, p.phonoGain, p.radioGain);
        balanceMaster.setBalance (p.balance);
        balanceMaster.setMaster (p.masterVolume);
    }

    void SignalChain::processChannelChain (ChannelChain& ch, Echo& echo,
                                           juce::AudioBuffer<float>& buffer,
                                           int channel)
    {
        const int n = buffer.getNumSamples();
        auto* data = buffer.getWritePointer (channel);

        // ===== INPUT-MIXER: 3 parallella bussar (manual §1A — Mic/Phono/Radio) =====
        // Per-kanal-fader (B&O dubbla skydepotentiometre)
        const float micG   = (channel == 0) ? params.micGain   : params.micGainR;
        const float phonoG = (channel == 0) ? params.phonoGain : params.phonoGainR;
        const float radioG = (channel == 0) ? params.radioGain : params.radioGainR;
        const float anyG = juce::jmax (micG, phonoG, radioG);

        if (anyG <= 1e-6f)
        {
            // Alla source-faders nere → tysta in-signalen men kör fortfarande
            // tape-state framåt (bias-fas + J-A magnetisering). Annars
            // de-synkar L vs R när bara en kanal har gain → instabil stereo.
            buffer.clear (channel, 0, n);
            ch.tape.process (buffer, channel);   // process zeros → state advances
            return;
        }

        // Input-trim — efter gain-cascaden reducerats till ~11 dB (var 72 dB)
        // behöver vi mindre pad. 0.7 = -3 dB, ger fader 50% en mer intuitiv
        // ~unity-nivå. (Tidigare 0.5 = -6 dB gjorde default-output för tyst.)
        constexpr float kInputPad = 0.7f;

        // Bypass-tape eller Monitor=Source: hoppa över tape-blocket men kör input-preamp
        // (manual #22 "medhør"; manual #23 bypass-läge). I detta läge använder vi mic-vägen
        // som primär färg eftersom det är vad användaren oftast monitorerar.
        if (params.bypassTape || params.monitorMode == 0)
        {
            buffer.applyGain (channel, 0, n, anyG * kInputPad);
            ch.micUw0029.process (buffer, channel);
            ch.micN2613.process (buffer, channel);
            ch.tone.process (buffer, channel);
            ch.powerAmp.process (buffer, channel);
            for (int i = 0; i < n; ++i)
                data[i] = ch.dcBlock.processSample (data[i]);
            return;
        }

        // 1. Kopiera DAW-input till tre scratch-buffrar (en per source-buss)
        //    Mic-vägen kör vi i huvudbufferten för att spara en kopia.
        if (phonoG > 1e-6f)
            phonoScratch.copyFrom (channel, 0, buffer, channel, 0, n);
        if (radioG > 1e-6f)
            radioScratch.copyFrom (channel, 0, buffer, channel, 0, n);

        // 2. MIC-bussen (i huvudbufferten)
        if (micG > 1e-6f)
        {
            buffer.applyGain (channel, 0, n, micG * kInputPad);

            // Mic-trafo (om lo-Z)
            if (params.micLoZ)
                ch.micTrafo.process (buffer, channel);

            ch.micUw0029.process (buffer, channel);
            ch.micN2613.process (buffer, channel);
        }
        else
        {
            // Inget mic-bidrag — nolla huvudbufferten så vi kan summera in phono/radio
            buffer.clear (channel, 0, n);
        }

        // 3. PHONO-bussen (i scratch-buffer) — kör genom PhonoPreamp (med RIAA i H-läge)
        if (phonoG > 1e-6f)
        {
            phonoScratch.applyGain (channel, 0, n, phonoG * kInputPad);
            ch.phono.process (phonoScratch, channel);

            // Phono subsonic rumble — LP-filtered noise from platter bearing (<5 Hz)
            {
                auto* psd = phonoScratch.getWritePointer (channel);
                constexpr float kAlpha     = 0.00060f;  // 1-pole LP ≈ 4.6 Hz @ 48 kHz
                constexpr float kNoiseScale = 0.058f;   // calibrated → ≈ −65 dBFS
                for (int i = 0; i < n; ++i)
                {
                    ch.noiseSeed = ch.noiseSeed * 1664525u + 1013904223u;
                    const float noise = static_cast<float> (static_cast<int32_t> (ch.noiseSeed))
                                        * (kNoiseScale / 2147483648.0f);
                    ch.phonoRumbleState += (noise - ch.phonoRumbleState) * kAlpha;
                    psd[i] += ch.phonoRumbleState;
                }
            }

            // Summera in i huvudbufferten
            buffer.addFrom (channel, 0, phonoScratch, channel, 0, n);
        }

        // 4. RADIO-bussen (i scratch-buffer) — flat preamp (UW0029 + 2N2613, ingen EQ)
        if (radioG > 1e-6f)
        {
            radioScratch.applyGain (channel, 0, n, radioG * kInputPad);

            // Radio 50 Hz power-line hum (−58 dBFS, electromagnetic induction artefact)
            {
                auto* rsd = radioScratch.getWritePointer (channel);
                constexpr float kHumAmp = 0.00126f;  // −58 dBFS
                for (int i = 0; i < n; ++i)
                {
                    ch.radioHumPhase += juce::MathConstants<float>::twoPi * 50.0f
                                        / static_cast<float> (sampleRate);
                    if (ch.radioHumPhase >= juce::MathConstants<float>::twoPi)
                        ch.radioHumPhase -= juce::MathConstants<float>::twoPi;
                    rsd[i] += std::sin (ch.radioHumPhase) * kHumAmp;
                }
            }

            ch.radioUw0029.process (radioScratch, channel);
            ch.radioN2613.process (radioScratch, channel);

            buffer.addFrom (channel, 0, radioScratch, channel, 0, n);
        }

        // ===== Efter input-mixern: gemensam record-tape-playback-pipeline =====

        // 5. Echo (record→play-head feedback-loop, manual §d)
        // Use per-channel amount so L/R echo amounts are independently gateable.
        const float echoAmt = (channel == 0) ? params.echoAmount : params.echoAmountR;
        if (params.echoEnabled && echoAmt > 1e-6f)
            echo.process (buffer, channel);

        // 6. Record-amp + pre-emphasis
        ch.recEq.process (buffer, channel);
        ch.ac126_1.process (buffer, channel);
        ch.ac126_2.process (buffer, channel);

        // 7. Tape-saturation (KÄRNAN)
        ch.tape.process (buffer, channel);

        // 8. Multiplay (om aktiv) — kumulerade generationsförluster
        if (params.multiplayGen > 1)
            ch.multiplay.process (buffer, channel);

        // 9. Wow & flutter
        ch.wowFlutter.process (buffer, channel);

        // 10. Playback-EQ (skippas vid Synchroplay — record-headet ger torrare ljud)
        if (! params.synchroplay)
            ch.playEq.process (buffer, channel);

        // 11. Tone-control (efter playback per manual §b — påverkar bara monitor)
        ch.tone.process (buffer, channel);

        // 12. Power-amp (om speaker-mode)
        ch.powerAmp.process (buffer, channel);

        // 13. DC-block
        for (int i = 0; i < n; ++i)
            data[i] = ch.dcBlock.processSample (data[i]);
    }

    float SignalChain::computeBlockRMSdBFS (const float* data, int n)
    {
        if (n <= 0) return -60.0f;
        double sum = 0.0;
        for (int i = 0; i < n; ++i)
            sum += static_cast<double> (data[i]) * static_cast<double> (data[i]);
        const float rms = static_cast<float> (std::sqrt (sum / n));
        return juce::jmax (-60.0f, 20.0f * std::log10 (rms + 1e-9f));
    }

    void SignalChain::process (juce::AudioBuffer<float>& buffer)
    {
        const int numCh = buffer.getNumChannels();

        // Defensiv input-sanitization: ersätt eventuella NaN/Inf från host
        // med 0 innan vi rör buffern. Hindrar att en denormal-pollution eller
        // en trasig upstream-FX poisonar våra IIR-delay-lines.
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            const int n = buffer.getNumSamples();
            for (int i = 0; i < n; ++i)
                if (! std::isfinite (data[i]))
                    data[i] = 0.0f;
        }

        // Snapshot raw INPUT levels before any DSP touches the buffer.
        if (numCh >= 1)
            inputLevelL_dBFS.store (
                computeBlockRMSdBFS (buffer.getReadPointer (0), buffer.getNumSamples()));
        if (numCh >= 2)
            inputLevelR_dBFS.store (
                computeBlockRMSdBFS (buffer.getReadPointer (1), buffer.getNumSamples()));

        if (numCh >= 1)
            processChannelChain (L, echoL, buffer, 0);
        if (numCh >= 2)
            processChannelChain (R, echoR, buffer, 1);

        // ----- B. Sound-on-Sound (manualens S-on-S): PLAY L → REC R -----
        // Implementeras som korsmix EFTER per-kanal-pipeline: höger output får
        // tillagt en del av vänster output. Pluginen approximerar detta som
        // post-mix-routing (riktig hårdvara gör det innan tape-record).
        if (params.soundOnSound && numCh >= 2)
        {
            const int n = buffer.getNumSamples();
            auto* l = buffer.getWritePointer (0);
            auto* r = buffer.getWritePointer (1);
            constexpr float sosAmount = 0.4f;
            for (int i = 0; i < n; ++i)
                r[i] += l[i] * sosAmount;
        }

        // ----- C. Monitor track-routing (manual knap 19/20) -----
        // Båda nedtryckta = stereo (default, manualens Bild C)
        // Bara T1 = vänster kanal på BÅDA högtalare (Bild B)
        // Bara T2 = höger kanal på BÅDA högtalare
        // Ingen = mute (egentligen stop, men i plugin = tystnad)
        if (numCh >= 2)
        {
            const bool t1 = params.monitorTrack1;
            const bool t2 = params.monitorTrack2;
            const int n = buffer.getNumSamples();
            auto* l = buffer.getWritePointer (0);
            auto* r = buffer.getWritePointer (1);

            if (t1 && ! t2)
            {
                // Vänster på båda
                for (int i = 0; i < n; ++i) r[i] = l[i];
            }
            else if (! t1 && t2)
            {
                // Höger på båda
                for (int i = 0; i < n; ++i) l[i] = r[i];
            }
            else if (! t1 && ! t2)
            {
                // Tysta
                buffer.clear();
            }
            // Båda nedtryckta = stereo (do nothing)
        }

        // ===== L/R cross-bleed (real BC2000 har ~−45 dB bleed via head-gap) =====
        if (numCh >= 2)
        {
            const float bleed = 0.0056f;   // -45 dB
            auto* l = buffer.getWritePointer (0);
            auto* r = buffer.getWritePointer (1);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const float ll = l[i];
                const float rr = r[i];
                l[i] = ll + rr * bleed;
                r[i] = rr + ll * bleed;
            }
        }

        balanceMaster.processStereo (buffer);

        // ===== SAFETY LIMITER — last line of defence before the DAW =====
        // Soft-knee limiter: tanh knee at ±0.95 → output always ≤ ±1.0.
        // Also detects NaN / Inf (numerical blow-up i tape-modellen eller IIR-
        // filter) och mutar tills hela kedjan är flushad. Tidigare resetade vi
        // bara per-kanal-kedjor; nu resetas även stereo-vägen (balanceMaster)
        // och vi håller mute "sticky" i några block för att FIR-oversampler /
        // delay-lines ska hinna nollas innan audio släpps tillbaka.
        {
            const int numSafe = buffer.getNumChannels();
            const int n = buffer.getNumSamples();

            auto fullChainReset = [this] (int ch)
            {
                ChannelChain& cc = (ch == 0) ? L : R;
                Echo& ec         = (ch == 0) ? echoL : echoR;
                cc.micTrafo.reset();
                cc.micUw0029.reset(); cc.micN2613.reset();
                cc.phono.reset();
                cc.radioUw0029.reset(); cc.radioN2613.reset();
                cc.recEq.reset();
                cc.ac126_1.reset(); cc.ac126_2.reset();
                cc.tape.reset();
                cc.multiplay.reset();
                cc.wowFlutter.reset();
                cc.playEq.reset();
                cc.tone.reset();
                cc.powerAmp.reset();
                cc.dcBlock.reset();
                ec.reset();
            };

            // Sticky-mute: vi är fortfarande i återhämtningsfönstret efter ett
            // tidigare NaN-incidents. Mut hela buffern, fortsätt resetta state.
            if (safetyMuteBlocks > 0)
            {
                buffer.clear();
                for (int ch = 0; ch < numSafe; ++ch) fullChainReset (ch);
                balanceMaster.reset();
                --safetyMuteBlocks;
            }
            else
            {
                bool anyNaN = false;
                for (int ch = 0; ch < numSafe && ! anyNaN; ++ch)
                {
                    const auto* data = buffer.getReadPointer (ch);
                    for (int i = 0; i < n; ++i)
                        if (! std::isfinite (data[i])) { anyNaN = true; break; }
                }

                if (anyNaN)
                {
                    // Numerical blow-up: mut + full reset av båda kanaler OCH
                    // stereo-mastern (annars håller balanceMaster's smoothed-
                    // value kvar NaN och förgiftar varje följande block).
                    // Sticky i 5 block ≈ 53 ms @ 48 kHz/512 — ger FIR-tappen
                    // i oversampler tid att flushas helt.
                    buffer.clear();
                    for (int ch = 0; ch < numSafe; ++ch) fullChainReset (ch);
                    balanceMaster.reset();
                    safetyMuteBlocks = 5;
                }
                else
                {
                    constexpr float kKnee = 0.95f;
                    for (int ch = 0; ch < numSafe; ++ch)
                    {
                        auto* data = buffer.getWritePointer (ch);
                        for (int i = 0; i < n; ++i)
                            data[i] = kKnee * std::tanh (data[i] / kKnee);
                    }
                }
            }
        }

        // Tape transport time accumulation (drives ReelDeck + counter display in UI)
        const double dt = static_cast<double> (buffer.getNumSamples()) / sampleRate;
        tapePositionSeconds.store (
            tapePositionSeconds.load (std::memory_order_relaxed) + dt,
            std::memory_order_relaxed);
        wowCurrentAmp.store (params.wowFlutterAmount, std::memory_order_relaxed);

        // Uppdatera VU-meter atomiskt (UI läser)
        if (numCh >= 1)
        {
            const float lvl = computeBlockRMSdBFS (buffer.getReadPointer (0), buffer.getNumSamples());
            meterLevelL_dBFS.store (lvl);
            isRecordingL.store (params.micGain > 0.05f && ! params.bypassTape);
        }
        if (numCh >= 2)
        {
            const float lvl = computeBlockRMSdBFS (buffer.getReadPointer (1), buffer.getNumSamples());
            meterLevelR_dBFS.store (lvl);
            isRecordingR.store (params.micGainR > 0.05f && ! params.bypassTape);
        }

        // ---- Push mono mix into spectrum FIFO (UI thread reads via FFT) ----
        {
            const auto* lPtr = numCh >= 1 ? buffer.getReadPointer (0) : nullptr;
            const auto* rPtr = numCh >= 2 ? buffer.getReadPointer (1) : lPtr;
            const int n = buffer.getNumSamples();
            int w = spectrumWriteIdx.load (std::memory_order_relaxed);
            for (int i = 0; i < n; ++i)
            {
                const float s = (lPtr ? lPtr[i] : 0.0f) * 0.5f
                              + (rPtr ? rPtr[i] : 0.0f) * 0.5f;
                spectrumBuffer[w] = s;
                w = (w + 1) & (kSpecBufSize - 1);
            }
            spectrumWriteIdx.store (w, std::memory_order_release);
        }
    }
}
