/*  SignalChain implementation — full pipeline. */

#include "SignalChain.h"
#include <cmath>

namespace bc2000dl::dsp
{
    void SignalChain::prepareChannel (ChannelChain& ch, double sr,
                                      float asymOffset, std::uint32_t baseSeed)
    {
        // Per-stage gains (v62.4 — kalibrerad mot DIN 1962 pre-emphasis):
        // mic 4+2 = 6 dB (inkapsling) + ac126 0+0 = 0 dB (efter recEq).
        // Tidigare ac126 3+2 = 5 dB lade signaler i J-A-mättnad efter pre-
        // emphasis-shelf @ 4.75 cm/s (+22 dB shelf vid 1326 Hz drev 3 kHz
        // tape-input till 0 dBFS), och J-A-saturationen kompresserade
        // fundamental → spec §6 tape-glow-mätning visade DIP istället för
        // PEAK.  Med ac126 = 0 dB håller tape-input -5 dBFS även vid HF
        // pre-emphasis-toppar → J-A linjär region → fundamental bevaras.
        ch.micTrafo.prepare (sr);
        ch.micUw0029.prepare (sr, GeStageType::UW0029, 4.0,
                              asymOffset, baseSeed + 100);
        ch.micN2613.prepare (sr, 2.0, asymOffset, baseSeed + 101);

        ch.phono.prepare (sr, asymOffset, baseSeed + 150);

        ch.radioUw0029.prepare (sr, GeStageType::UW0029, 4.0,
                                asymOffset, baseSeed + 170);
        ch.radioN2613.prepare (sr, 2.0, asymOffset, baseSeed + 171);

        // Per-källa-tonsignatur — gör Mic/Phono/Radio TYDLIGT audibelt olika.
        // (Mic lämnas NEUTRAL — full-range referens; spec mäts via mic-vägen.)
        // Skillnaderna måste överleva tape-steget + ev. inblandad mic, så de är
        // medvetet kraftiga (≈±6 dB) → phono och radio hamnar ~20 dB isär i
        // ton-tilt och blir omisskännligt olika.
        //
        // Phono: rejält bas-lyft + tydligt dämpad diskant → varm/mörk vinyl.
        ch.phonoToneLf.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf (
            sr, 220.0f, 0.70f, juce::Decibels::decibelsToGain (+5.0f));
        ch.phonoToneHf.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
            sr, 3000.0f, 0.70f, juce::Decibels::decibelsToGain (-6.0f));
        // Radio: BANDBEGRÄNSAD "FM-tuner" — högpass + lågpass skär bort djupbas
        // och luft → mittenfokuserad, "boxig" radiokaraktär. Bandbegränsning är
        // irreversibel (tape kan inte lägga tillbaka borttaget innehåll), så
        // skillnaden överlever tape-steget OCH när radion mixas med mic.
        ch.radioToneLf.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (
            sr, 170.0f, 0.707f);                       // skär djupbas
        ch.radioToneHf.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (
            sr, 6500.0f, 0.707f);                      // skär luft/diskant
        ch.phonoToneHf.reset(); ch.phonoToneLf.reset();
        ch.radioToneHf.reset(); ch.radioToneLf.reset();

        ch.recEq.prepare (sr);
        ch.ac126_1.prepare (sr, GeStageType::AC126, 0.0,
                             asymOffset, baseSeed + 200);
        ch.ac126_2.prepare (sr, GeStageType::AC126, 0.0,
                             asymOffset, baseSeed + 201);
        ch.tape.prepare (sr, baseSeed + 300);
        ch.multiplay.prepare (sr, baseSeed + 350);
        ch.wowFlutter.prepare (sr);
        // L (asymOffset>0): weave +, R: weave − (motfas). Scrape-seed per
        // kanal — pitch-jittret i övrigt delar sekvens (samma fysiska band).
        ch.wowFlutter.setStereoRole (asymOffset >= 0.0f ? 1.0f : -1.0f,
                                     baseSeed * 2654435761u + 17u);
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

        // Bypass/Source-vägen får samma fasta delay som tape-vägens
        // oversampler + wow/flutter-basdelay → latensen är lägesoberoende
        // och PDC-rapporten (getLatencySamples()) gäller alla lägen.
        const int totalLatency = getLatencySamples();
        L.bypassDelay.prepare (totalLatency);
        R.bypassDelay.prepare (totalLatency);

        // Dry-väg för MIX: samma latens som wet-vägen → fasriktig blandning
        dryScratch.setSize (2, blockSize, false, true, true);
        dryDelayL.prepare (totalLatency);
        dryDelayR.prepare (totalLatency);

        smInitialised = false;   // första blocket snappar till APVTS-targets

        echoL.prepare (sr);
        echoR.prepare (sr);
        balanceMaster.prepare (sr, blockSize);
        sosSmooth.reset (sr, 0.015);   // 15 ms on/off-ramp för Sound-on-Sound
        sosSmooth.setCurrentAndTargetValue (0.0f);

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
            ch->bypassDelay.reset();
            ch->phonoToneHf.reset(); ch->phonoToneLf.reset();
            ch->radioToneHf.reset(); ch->radioToneLf.reset();
        }
        echoL.reset(); echoR.reset();
        balanceMaster.reset();
        phonoScratch.clear();
        radioScratch.clear();
        dryScratch.clear();
        dryDelayL.reset(); dryDelayR.reset();
        // Reset per-source noise and modulation phases
        L.radioHumPhase = R.radioHumPhase = 0.0f;
        L.phonoRumbleState = R.phonoRumbleState = 0.0f;

        // Reset tape-transport counters so the UI counter returns to 0000
        // when the DAW transport rewinds or the processor is re-initialised.
        tapePositionSeconds.store (0.0, std::memory_order_relaxed);

        smInitialised = false;   // snap (inte ramp) efter reset
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

        // Kontinuerliga parametrar (bias, drive, wow, ton, echo, gains, trim)
        // appliceras INTE här — de utjämnas per block i advanceSmoothedParams()
        // (anropas från process()) för klickfri automation.
        L.powerAmp.setEnabled (p.speakerMonitor);       R.powerAmp.setEnabled (p.speakerMonitor);
        L.multiplay.setGeneration (p.multiplayGen);     R.multiplay.setGeneration (p.multiplayGen);
        L.multiplay.setEnabled (p.multiplayGen > 1);    R.multiplay.setEnabled (p.multiplayGen > 1);

        echoL.setEnabled (p.echoEnabled); echoR.setEnabled (p.echoEnabled);

        // ===== S-on-S har TVÅ samverkande mekanismer (dokumenterat 2026-06) =====
        // sos_enabled aktiverar BÅDA:
        //   (a) Echo-ping-pong: echo läser partner-kanalens delay-buffer (nedan).
        //       Hörs BARA om Echo är ON + echo-fadern > 0. echo-fadern styr alltså
        //       hur mycket cross-fed eko man hör — INTE S-on-S-lagernivån.
        //   (b) Direkt symmetrisk L↔R-korsmix, fast 0.4 (sist i process(), rad ~600).
        //       Detta ÄR själva S-on-S-lagringen och är OBEROENDE av echo-fadern.
        // Att stapla SOS + Echo + hög Multiplay korskopplar genom germanium-
        // mättnaden → avsiktligt grungigt (autentiskt tape-S-on-S/dub). Magnituden
        // ska kalibreras mot fysisk maskin (Fas 5), inte gissas bort som "bugg".
        // Verifierat STABIL: ingen runaway/NaN, peak bounded (SosEchoStability-test).
        if (p.soundOnSound)
        {
            echoL.setCrossFeedSource (&echoR);
            echoR.setCrossFeedSource (&echoL);
        }
        else
        {
            echoL.setCrossFeedSource (nullptr);
            echoR.setCrossFeedSource (nullptr);
        }
        // S-on-S korsmix-nivå (smoothad i process() → ingen klick vid toggle)
        sosSmooth.setTargetValue (p.soundOnSound ? 0.4f : 0.0f);

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

        // Bias-typ (NORMAL/HIGH) + track-width (1/4 vs 1/2) — schema 9224002/3
        L.tape.setBiasType (p.biasType);     R.tape.setBiasType (p.biasType);
        L.tape.setTrackWidth (p.trackWidth); R.tape.setTrackWidth (p.trackWidth);

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

        balanceMaster.setBalance (p.balance);
        balanceMaster.setMaster  (p.masterVolume);
        balanceMaster.setMasterR (p.masterVolumeR);
    }

    void SignalChain::advanceSmoothedParams (int numSamples)
    {
        smPrev = sm;

        // En-pol mot target; alpha=1 (snap) på första blocket efter
        // prepare()/reset() så kalibrerade tester förblir deterministiska.
        const float alpha = smInitialised
            ? 1.0f - std::exp (-static_cast<float> (numSamples)
                               / (kParamSmoothSec * static_cast<float> (sampleRate)))
            : 1.0f;

        auto step = [alpha] (float& cur, float target)
        {
            cur += (target - cur) * alpha;
            if (std::abs (target - cur) < 1.0e-5f) cur = target;
        };

        step (sm.micGain,          params.micGain);
        step (sm.micGainR,         params.micGainR);
        step (sm.phonoGain,        params.phonoGain);
        step (sm.phonoGainR,       params.phonoGainR);
        step (sm.radioGain,        params.radioGain);
        step (sm.radioGainR,       params.radioGainR);
        step (sm.bassDb,           params.bassDb);
        step (sm.trebleDb,         params.trebleDb);
        step (sm.biasAmount,       params.biasAmount);
        step (sm.biasAmountR,      params.biasAmountR);
        step (sm.saturationDrive,  params.saturationDrive);
        step (sm.saturationDriveR, params.saturationDriveR);
        step (sm.wowFlutterAmount, params.wowFlutterAmount);
        step (sm.echoAmount,       params.echoAmount);
        step (sm.echoAmountR,      params.echoAmountR);
        step (sm.echoTimeMs,       params.echoTimeMs);
        step (sm.echoFeedback,     params.echoFeedback);
        step (sm.inputTrimDb,      params.inputTrimDb);
        step (sm.outputTrimDb,     params.outputTrimDb);
        step (sm.printThrough,     params.printThrough);
        step (sm.mainsHum,         params.mainsHum);
        step (sm.mix,              params.mix);
        step (sm.tapeNoise,        params.tapeNoise);

        const bool firstBlock = ! smInitialised;
        if (firstBlock)
            smPrev = sm;
        smInitialised = true;

        // Applicera mot DSP-modulerna — bara vid faktisk ändring, eftersom
        // flera settrar räknar om filterkoefficienter (ToneControl allokerar).
        auto changed = [&] (float ContParams::* m)
        {
            return firstBlock || sm.*m != smPrev.*m;
        };

        if (changed (&ContParams::biasAmount))       L.tape.setBiasAmount (sm.biasAmount);
        if (changed (&ContParams::biasAmountR))      R.tape.setBiasAmount (sm.biasAmountR);
        if (changed (&ContParams::saturationDrive))  L.tape.setSaturationDrive (sm.saturationDrive);
        if (changed (&ContParams::saturationDriveR)) R.tape.setSaturationDrive (sm.saturationDriveR);
        if (changed (&ContParams::printThrough))
        {
            L.tape.setPrintThrough (sm.printThrough);
            R.tape.setPrintThrough (sm.printThrough);
        }
        if (changed (&ContParams::tapeNoise))
        {
            L.tape.setNoiseLevel (sm.tapeNoise);
            R.tape.setNoiseLevel (sm.tapeNoise);
        }
        if (changed (&ContParams::wowFlutterAmount))
        {
            L.wowFlutter.setAmount (sm.wowFlutterAmount);
            R.wowFlutter.setAmount (sm.wowFlutterAmount);
        }
        if (changed (&ContParams::bassDb))   { L.tone.setBassDb (sm.bassDb);     R.tone.setBassDb (sm.bassDb); }
        if (changed (&ContParams::trebleDb)) { L.tone.setTrebleDb (sm.trebleDb); R.tone.setTrebleDb (sm.trebleDb); }
        if (changed (&ContParams::echoAmount))   echoL.setAmount (sm.echoAmount);
        if (changed (&ContParams::echoAmountR))  echoR.setAmount (sm.echoAmountR);
        // ECHO TIME/FEEDBACK åsidosätter auto-från-speed — Echo glider själv
        // internt (~30 ms), utjämningen här tar bara bort blockstegen.
        if (changed (&ContParams::echoTimeMs))   { echoL.setTimeMs (sm.echoTimeMs);     echoR.setTimeMs (sm.echoTimeMs); }
        if (changed (&ContParams::echoFeedback)) { echoL.setFeedback (sm.echoFeedback); echoR.setFeedback (sm.echoFeedback); }

        // mixer.setGains används bara för totalgain (bypass-mode)
        mixer.setGains (sm.micGain, sm.phonoGain, sm.radioGain);
    }

    void SignalChain::processChannelChain (ChannelChain& ch, Echo& echo,
                                           juce::AudioBuffer<float>& buffer,
                                           int channel)
    {
        const int n = buffer.getNumSamples();
        auto* data = buffer.getWritePointer (channel);

        // ===== INPUT-MIXER: 3 parallella bussar (manual §1A — Mic/Phono/Radio) =====
        // Per-kanal-fader (B&O dubbla skydepotentiometre). Utjämnade värden
        // (sm) + ramp från föregående block (smPrev) → klickfri automation.
        const float micG   = (channel == 0) ? sm.micGain   : sm.micGainR;
        const float phonoG = (channel == 0) ? sm.phonoGain : sm.phonoGainR;
        const float radioG = (channel == 0) ? sm.radioGain : sm.radioGainR;
        const float micG0   = (channel == 0) ? smPrev.micGain   : smPrev.micGainR;
        const float phonoG0 = (channel == 0) ? smPrev.phonoGain : smPrev.phonoGainR;
        const float radioG0 = (channel == 0) ? smPrev.radioGain : smPrev.radioGainR;
        const float anyG  = juce::jmax (micG, phonoG, radioG);
        const float anyG0 = juce::jmax (micG0, phonoG0, radioG0);

        // v60.5 — Christoffer-feedback: när echo aktivt med alla faders=0 ska
        // feedbacken FORTSÄTTA (inte klippas av).  Tidigare early-return tystade
        // hela kedjan inkl. echo-delaylinjen, vilket bröt klassiskt "tape-feedback-
        // jam" där man maxar echo + drar ner ingångar för att höra delay-loopen
        // dö ut / själv-oscillera.  Nu: skip-shortcut bara om echo också är av.
        const float echoAmtL = sm.echoAmount;
        const float echoAmtR = sm.echoAmountR;
        const bool echoActive = params.echoEnabled && (echoAmtL > 1e-6f || echoAmtR > 1e-6f);
        if (anyG <= 1e-6f && anyG0 <= 1e-6f && ! echoActive)
        {
            // Alla source-faders nere OCH echo av → tysta in-signalen men kör
            // fortfarande tape-state framåt (bias-fas + J-A magnetisering).
            buffer.clear (channel, 0, n);
            ch.tape.process (buffer, channel);   // process zeros → state advances
            ch.wowFlutter.process (buffer, channel);  // flusha delay-tail, håll latensvägen aktiv
            return;
        }

        // Input-trim.  Christoffer-Berg-testen (2026-05-12, macOS 14/Logic)
        // upplevde ingångarna som för låga jämfört med hårdvaran.  Real B&O
        // Beocord 2000 vid full slider gav lätt +10–15 dB drive in i band.
        // Med pad=0.7 + 6 dB mic-preamp landade max-slider på endast +3 dB —
        // för "tämt" känsla.  Bumpas till 1.2 (+4.7 dB):
        //   fader 0.5 (default): mic +1.6 dB, phono +4.5 dB    (säkert)
        //   fader 1.0 (max):     mic +7.6 dB, phono +10.5 dB   (hardware-ish)
        // Phono klär sig själv via RIAA-shelf (+6 dB på bas), så +10.5 dB
        // är realistiskt drive — inte clipping i normal program-material.
        // v61.1 — sänkt 1.2 → 0.85 (−3 dB).  v60.x-omkalibreringen drev tape-
        // steget för hett: THD @ −3 dBFS spräckte 3 %-spec:en (≈4.5 %) och
        // utgången gick över 0 dBFS vid het input.  −3 dB drar referensnivå-THD
        // under spec + ger utgångstak utan att tappa tape-karaktär vid 0 dBFS.
        constexpr float kInputPad = 0.85f;

        // Bypass-tape eller Monitor=Source: hoppa över tape-blocket men kör input-preamp
        // (manual #22 "medhør"; manual #23 bypass-läge). I detta läge använder vi mic-vägen
        // som primär färg eftersom det är vad användaren oftast monitorerar.
        if (params.bypassTape || params.monitorMode == 0)
        {
            buffer.applyGainRamp (channel, 0, n, anyG0 * kInputPad, anyG * kInputPad);
            ch.micUw0029.process (buffer, channel);
            ch.micN2613.process (buffer, channel);
            ch.tone.process (buffer, channel);
            ch.powerAmp.process (buffer, channel);
            // bypassDelay matchar tape-vägens latens → klickfri växling
            // Source/Tape och korrekt PDC i båda lägena.
            for (int i = 0; i < n; ++i)
                data[i] = ch.dcBlock.processSample (ch.bypassDelay.processSample (data[i]));
            return;
        }

        // 1. Kopiera DAW-input till tre scratch-buffrar (en per source-buss)
        //    Mic-vägen kör vi i huvudbufferten för att spara en kopia.
        //    Bussen hålls aktiv tills BÅDE nuvarande och föregående gain är 0
        //    så gain-rampen hinner landa innan vägen stängs av.
        if (phonoG > 1e-6f || phonoG0 > 1e-6f)
            phonoScratch.copyFrom (channel, 0, buffer, channel, 0, n);
        if (radioG > 1e-6f || radioG0 > 1e-6f)
            radioScratch.copyFrom (channel, 0, buffer, channel, 0, n);

        // 2. MIC-bussen (i huvudbufferten)
        if (micG > 1e-6f || micG0 > 1e-6f)
        {
            buffer.applyGainRamp (channel, 0, n, micG0 * kInputPad, micG * kInputPad);

            // Mic-trafo (om lo-Z)
            if (params.micLoZ)
                ch.micTrafo.process (buffer, channel);

            ch.micUw0029.process (buffer, channel);
            ch.micN2613.process (buffer, channel);
            // Mic lämnas tonneutral — referens (spec mäts via mic-vägen).
        }
        else
        {
            // Inget mic-bidrag — nolla huvudbufferten så vi kan summera in phono/radio
            buffer.clear (channel, 0, n);
        }

        // 3. PHONO-bussen (i scratch-buffer) — kör genom PhonoPreamp (med RIAA i H-läge)
        if (phonoG > 1e-6f || phonoG0 > 1e-6f)
        {
            phonoScratch.applyGainRamp (channel, 0, n, phonoG0 * kInputPad, phonoG * kInputPad);
            ch.phono.process (phonoScratch, channel);

            // Phono-tonsignatur (varm/fyllig vinyl) — utöver RIAA
            {
                auto* pd = phonoScratch.getWritePointer (channel);
                for (int i = 0; i < n; ++i)
                    pd[i] = ch.phonoToneHf.processSample (ch.phonoToneLf.processSample (pd[i]));
            }

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
        if (radioG > 1e-6f || radioG0 > 1e-6f)
        {
            radioScratch.applyGainRamp (channel, 0, n, radioG0 * kInputPad, radioG * kInputPad);

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

            // Radio-tonsignatur (ljus/ren)
            auto* rd = radioScratch.getWritePointer (channel);
            for (int i = 0; i < n; ++i)
                rd[i] = ch.radioToneLf.processSample (ch.radioToneHf.processSample (rd[i]));

            buffer.addFrom (channel, 0, radioScratch, channel, 0, n);
        }

        // ===== Efter input-mixern: gemensam record-tape-playback-pipeline =====

        // 5. Echo (record→play-head feedback-loop, manual §d)
        // Use per-channel amount so L/R echo amounts are independently gateable.
        const float echoAmt = (channel == 0) ? sm.echoAmount : sm.echoAmountR;
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
        const int n     = buffer.getNumSamples();

        // Utjämna kontinuerliga parametrar (~30 ms en-pol) mot APVTS-targets
        // och applicera mot DSP-modulerna — klickfri automation (UAD-krav).
        advanceSmoothedParams (n);

        // KRITISK: scratch-buffrarna allokeras till maxBlock i prepare(), men
        // host:en kan skicka mindre block (Logic gör så vid sample-accurate
        // automation eller bara olika I/O-buffer-storlek vs samplesPerBlock).
        // Om vi inte synkar scratch.getNumSamples() till faktiskt n så kommer
        // GE-stages, RIAA-filter och rumble-loop:en processa hela allokeringen
        // → samples [n..blockSize) är stale audio som GE-saturerar + IIR-state
        // (RIAA-shelfen) desyncar från audio-strömmen → output blir noise.
        // Symptom: phono-vägen producerade endast missljud (Christoffer/Logic).
        // setSize med avoidReallocating=true är O(1) när n ≤ allokerat — säkert
        // i RT-tråden eftersom prepare() har redan reserverat tillräckligt.
        if (phonoScratch.getNumSamples() != n)
            phonoScratch.setSize (2, n, false, false, true);
        if (radioScratch.getNumSamples() != n)
            radioScratch.setSize (2, n, false, false, true);
        if (dryScratch.getNumSamples() != n)
            dryScratch.setSize (2, n, false, false, true);

        // ----- Dry-väg för MIX: fånga rå input (före trim/DSP) och fördröj
        // med pluginens rapporterade latens → fasriktig dry/wet-blandning.
        // Delayn matas ALLTID (även vid mix=1.0) så den är i fas när
        // användaren drar ner mix-ratten.
        for (int ch = 0; ch < juce::jmin (numCh, 2); ++ch)
        {
            dryScratch.copyFrom (ch, 0, buffer, ch, 0, n);
            auto& dly = (ch == 0) ? dryDelayL : dryDelayR;
            auto* d = dryScratch.getWritePointer (ch);
            for (int i = 0; i < n; ++i)
                d[i] = dly.processSample (d[i]);
        }

        // Snapshot raw INPUT levels before any DSP touches the buffer.
        if (numCh >= 1)
            inputLevelL_dBFS.store (
                computeBlockRMSdBFS (buffer.getReadPointer (0), buffer.getNumSamples()));
        if (numCh >= 2)
            inputLevelR_dBFS.store (
                computeBlockRMSdBFS (buffer.getReadPointer (1), buffer.getNumSamples()));

        // ----- A0. Input trim (PLUGIN UTILITY · pre-DSP gain) -----
        // Mätaren ovan visar rå input; trimmet appliceras före hela kedjan så
        // det driver tape/preamp-saturationen (autentisk gain-staging).
        if (sm.inputTrimDb != 0.0f || smPrev.inputTrimDb != 0.0f)
            buffer.applyGainRamp (0, buffer.getNumSamples(),
                                  juce::Decibels::decibelsToGain (smPrev.inputTrimDb),
                                  juce::Decibels::decibelsToGain (sm.inputTrimDb));

        if (numCh >= 1)
            processChannelChain (L, echoL, buffer, 0);
        if (numCh >= 2)
            processChannelChain (R, echoR, buffer, 1);

        // ----- B. Sound-on-Sound (manualens S-on-S: bouncing/lagring) -----
        // Tidigare la vi BARA in vänster i höger (r += l*a). Det gjorde signalen
        // lopsided: höger blev högre + bildens tyngdpunkt drogs åt höger, och en
        // mono-källa fick +3 dB bara i R → lät trasigt ("knas"). S-on-S ska låta
        // som tätare/lagrat material, inte panorerat fel.
        // Nu: SYMMETRISK korsmix (båda kanalerna får en del av den andra) som
        // mjukt fäller ihop mot mono — med makeup-gain så NIVÅN inte hoppar.
        // SMOOTHAD on/off (15 ms) → inga klick. Balanserad bild, tätare ljud.
        if (numCh >= 2 && (params.soundOnSound || sosSmooth.isSmoothing()))
        {
            const int n = buffer.getNumSamples();
            auto* l = buffer.getWritePointer (0);
            auto* r = buffer.getWritePointer (1);
            for (int i = 0; i < n; ++i)
            {
                const float a  = sosSmooth.getNextValue();      // 0..0.4
                const float mk = 1.0f / (1.0f + a);             // makeup → konstant nivå
                const float l0 = l[i], r0 = r[i];
                l[i] = (l0 + a * r0) * mk;
                r[i] = (r0 + a * l0) * mk;
            }
        }
        else
        {
            sosSmooth.skip (buffer.getNumSamples());
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

        // ===== L/R cross-bleed (real BC2000 har ~−46 dB bleed via head-gap) =====
        if (numCh >= 2)
        {
            // v60.5 — Christoffer Berg-feedback: "mic in L är inte 100 %
            // panorerad, den skall vara hard Left".  Sänkt från 0.005 (−46 dB)
            // till 0.0002 (−74 dB) → praktiskt taget inaudibelt.  Behåller
            // teknisk "head-bleed"-karaktär men håller hard-pan-känslan vid
            // monitoring.  Fortfarande över spec §8 ≥45 dB med marginal.
            const float bleed = 0.0002f;   // -74 dB (var -46 dB)
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

        // ----- Output trim (PLUGIN UTILITY · post-DSP makeup-gain) -----
        if (sm.outputTrimDb != 0.0f || smPrev.outputTrimDb != 0.0f)
            buffer.applyGainRamp (0, buffer.getNumSamples(),
                                  juce::Decibels::decibelsToGain (smPrev.outputTrimDb),
                                  juce::Decibels::decibelsToGain (sm.outputTrimDb));

        // ----- MAINS HUM — 1968-amparnas "slight unobtrusive mains hum" -----
        // Fundamental + 3:e harmonik (nät-trafo-mättnad ger udda övertoner).
        // Konservativ skalning: mainsHum∈[0,0.1] → max ≈ −34 dBFS, subtilt.
        if (sm.mainsHum > 1.0e-6f && numCh >= 1)
        {
            const double inc = juce::MathConstants<double>::twoPi
                                 * (double) params.mainsHumFreqHz / sampleRate;
            const float  amp = sm.mainsHum * 0.2f;
            auto* l = buffer.getWritePointer (0);
            auto* r = numCh >= 2 ? buffer.getWritePointer (1) : nullptr;
            for (int i = 0; i < n; ++i)
            {
                const float hum = amp * (float) (std::sin (mainsHumPhase)
                                       + 0.3 * std::sin (3.0 * mainsHumPhase));
                l[i] += hum;
                if (r != nullptr) r[i] += hum;
                mainsHumPhase += inc;
                if (mainsHumPhase >= juce::MathConstants<double>::twoPi)
                    mainsHumPhase -= juce::MathConstants<double>::twoPi;
            }
        }

        // ----- MIX (dry/wet) — equal-gain-crossfade mot latensjusterad dry --
        if (sm.mix < 1.0f || smPrev.mix < 1.0f)
        {
            const float m0 = smPrev.mix, m1 = sm.mix;
            const float invN = 1.0f / static_cast<float> (juce::jmax (1, n - 1));
            for (int ch = 0; ch < juce::jmin (numCh, 2); ++ch)
            {
                auto* wet = buffer.getWritePointer (ch);
                const auto* dry = dryScratch.getReadPointer (ch);
                for (int i = 0; i < n; ++i)
                {
                    const float m = m0 + (m1 - m0) * static_cast<float> (i) * invN;
                    wet[i] = wet[i] * m + dry[i] * (1.0f - m);
                }
            }
        }

        // ----- Output safety: NaN/Inf-vakt (SJÄLVLÄKANDE) + hård clamp -----
        // KRITISKT: jlimit() fångar INTE NaN (NaN<lo och NaN>hi är båda false →
        // NaN passerar). Utan detta kunde en enda NaN — t.ex. om Jiles-Atherton-
        // tape-state divergerar under ihållande drive, eller echo-feedback rusar —
        // latcha i de rekursiva tillstånden (IIR-filter, echo-delay, tape) och
        // tysta pluggen PERMANENT (host:en mutar). Symptom: "Ableton tystar appen
        // efter en tids användning". Nu: upptäck icke-ändligt → RESET hela kedjan
        // + mut några block medan allt flushas → pluggen läker automatiskt.
        bool nonFinite = false;
        for (int ch = 0; ch < numCh && ! nonFinite; ++ch)
        {
            const auto* d = buffer.getReadPointer (ch);
            for (int i = 0; i < n; ++i)
                if (! std::isfinite (d[i])) { nonFinite = true; break; }
        }
        if (nonFinite)
        {
            reset();                  // nollar echo-buffert, IIR-states, tape-state …
            safetyMuteBlocks = 4;     // ~4 block tystnad medan kedjan settlar
        }

        // Hård gräns ±4.0 (≈ +12 dBFS) + per-sample-sanering: ALDRIG NaN/Inf ut,
        // och inga absurda nivåer (skyddar högtalare/öron). Transparent under ±4.
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* d = buffer.getWritePointer (ch);
            for (int i = 0; i < n; ++i)
            {
                const float v = d[i];
                d[i] = std::isfinite (v) ? juce::jlimit (-4.0f, 4.0f, v) : 0.0f;
            }
        }

        // Sticky safety-mute: håll tyst N block efter en NaN-händelse så att
        // FIR-oversampler / IIR-states / delay-lines hinner flushas rent.
        if (safetyMuteBlocks > 0)
        {
            for (int ch = 0; ch < numCh; ++ch)
                buffer.clear (ch, 0, n);
            --safetyMuteBlocks;
        }

        // Tape transport time accumulation (drives ReelDeck + counter display in UI)
        const double dt = static_cast<double> (buffer.getNumSamples()) / sampleRate;
        tapePositionSeconds.store (
            tapePositionSeconds.load (std::memory_order_relaxed) + dt,
            std::memory_order_relaxed);
        wowCurrentAmp.store (params.wowFlutterAmount, std::memory_order_relaxed);

        // Uppdatera VU-meter + peak-overload-LED atomiskt (UI läser).
        // Peak-LED: schema 9224002 "-22V" indikator-lampor.  Tröskel −3 dBFS
        // motsvarar real hardware där lampan tändes när rec-signalen närmade
        // sig saturation.  Set-only här; UI-thread hanterar 500 ms decay.
        constexpr float kPeakThresholdDb = -3.0f;
        auto peakDb = [] (const float* d, int n) -> float
        {
            float p = 0.0f;
            for (int i = 0; i < n; ++i) p = std::max (p, std::abs (d[i]));
            return 20.0f * std::log10 (p + 1e-9f);
        };
        if (numCh >= 1)
        {
            const int nBlk = buffer.getNumSamples();
            const float lvl = computeBlockRMSdBFS (buffer.getReadPointer (0), nBlk);
            meterLevelL_dBFS.store (lvl);
            isRecordingL.store (params.micGain > 0.05f && ! params.bypassTape);
            if (peakDb (buffer.getReadPointer (0), nBlk) > kPeakThresholdDb)
                peakOverloadL.store (true, std::memory_order_release);
        }
        if (numCh >= 2)
        {
            const int nBlk = buffer.getNumSamples();
            const float lvl = computeBlockRMSdBFS (buffer.getReadPointer (1), nBlk);
            meterLevelR_dBFS.store (lvl);
            isRecordingR.store (params.micGainR > 0.05f && ! params.bypassTape);
            if (peakDb (buffer.getReadPointer (1), nBlk) > kPeakThresholdDb)
                peakOverloadR.store (true, std::memory_order_release);
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
