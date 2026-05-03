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
        phonoScratch.clear();
        radioScratch.clear();
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

        // Tape-formel (plan §7)
        const auto tf = (p.tapeFormula == 1 ? TapeFormula::BASF :
                        p.tapeFormula == 2 ? TapeFormula::Scotch : TapeFormula::Agfa);
        L.tape.setFormula (tf); R.tape.setFormula (tf);

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

            // Summera in i huvudbufferten
            buffer.addFrom (channel, 0, phonoScratch, channel, 0, n);
        }

        // 4. RADIO-bussen (i scratch-buffer) — flat preamp (UW0029 + 2N2613, ingen EQ)
        if (radioG > 1e-6f)
        {
            radioScratch.applyGain (channel, 0, n, radioG * kInputPad);
            ch.radioUw0029.process (radioScratch, channel);
            ch.radioN2613.process (radioScratch, channel);

            buffer.addFrom (channel, 0, radioScratch, channel, 0, n);
        }

        // ===== Efter input-mixern: gemensam record-tape-playback-pipeline =====

        // 5. Echo (record→play-head feedback-loop, manual §d)
        if (params.echoEnabled && params.echoAmount > 1e-6f)
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
            isRecordingR.store (params.micGain > 0.05f && ! params.bypassTape);
        }
    }
}
