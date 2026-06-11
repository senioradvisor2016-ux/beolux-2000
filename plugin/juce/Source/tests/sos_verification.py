#!/usr/bin/env python3
"""
sos_verification.py — verifiera Sound-on-Sound mot service-manualen §C (sid 10).

Manualens spec:
  "Multiplay oder 'Sound on Sound'-Aufnahmen setzen voraus, dass ein Monosignal
   (1. Stimme) auf der oberen Spur (L) aufgenommen ist. Vom Wiedergabekopf aus
   wird dieses Signal ... durch den 'S on S'-Druckknopf und die Regulierung zum
   rechten Aufnahmeverstärker zusammen mit der 2. Stimme geführt. Beide Signale
   werden auf der unteren Spur R eingespielt."

  → Post-tape L (playback-huvudet) → fader #12 → HÖGER record-amp → R-spåret.
  → Enkelriktad L→R.  Fader #12 (echo_amount_r) = "die Regulierung".

Kör:  python3 Source/tests/sos_verification.py
Kräver:  pip install pedalboard numpy  +  installerad VST3.
"""

import sys
import numpy as np
import pedalboard

PLUGIN_PATH = "/Users/senioradvisor/Library/Audio/Plug-Ins/VST3/Germanium 2000 Deluxe.vst3"
SR    = 48000
BLOCK = 512
WARM  = 40

results = []

def report(name, ok, detail=""):
    tag = "✅ PASS" if ok else "❌ FAIL"
    print(f"  {tag}  {name:<58} {detail}")
    results.append(ok)

def rms_db(x):
    r = float(np.sqrt(np.mean(x.astype(np.float64) ** 2)))
    return 20 * np.log10(r + 1e-12)

def make_sine(freq, n, sr, amp=0.3):
    t = np.arange(n) / sr
    return amp * np.sin(2 * np.pi * freq * t).astype(np.float32)

def run(plugin, audio_in):
    plugin.reset()
    warm = np.zeros((2, WARM * BLOCK), dtype=np.float32)
    plugin.process(warm, sample_rate=SR, buffer_size=BLOCK)
    return plugin.process(audio_in, sample_rate=SR, buffer_size=BLOCK)

print("══════════════════════════════════════════════════════════════════")
print("  Sound-on-Sound-verifiering mot service-manual §C (sid 10)")
print("══════════════════════════════════════════════════════════════════")

plugin = pedalboard.load_plugin(PLUGIN_PATH)
assert "sound_on_sound" in plugin.parameters, "param sound_on_sound saknas"
assert "echo_amount_r" in plugin.parameters, "param echo_amount_r saknas"

# Baseline: radio L som källa, allt annat nere.
plugin.master_volume = 0.85
plugin.mic_gain_l = 0.0
plugin.mic_gain_r = 0.0
plugin.phono_gain_l = 0.0
plugin.phono_gain_r = 0.0
plugin.radio_gain_l = 0.7
plugin.radio_gain_r = 0.0

n_samples = SR
audio = np.zeros((2, n_samples), dtype=np.float32)
audio[0] = make_sine(1000, n_samples, SR, amp=0.3)

# ── T1: SoS AV — kanal-isolation ─────────────────────────────────
print("── T1: SoS AV — kanal-isolation ──────────────────────────────────")
plugin.sound_on_sound = False
plugin.echo_amount_r = 0.5
out = run(plugin, audio)
l_db, r_db = rms_db(out[0]), rms_db(out[1])
report("L har signal (radio L matas)", l_db > -50.0, f"L = {l_db:+.1f} dBFS")
report("R är tyst (SoS AV)", r_db < -40.0, f"R = {r_db:+.1f} dBFS")
report("L→R isolation > 30 dB", (l_db - r_db) > 30.0,
       f"isolation = {l_db - r_db:.1f} dB")

# ── T2: SoS PÅ + fader 0.5 — bounce aktiv ────────────────────────
print("\n── T2: SoS PÅ + fader=0.5 — bounce L→R aktiv ─────────────────────")
plugin.sound_on_sound = True
plugin.echo_amount_r = 0.5
out = run(plugin, audio)
l_db, r_db = rms_db(out[0]), rms_db(out[1])
report("L oförändrad (ingen symmetrisk mix)", l_db > -50.0, f"L = {l_db:+.1f} dBFS")
report("R har bounce-content", r_db > -55.0, f"R = {r_db:+.1f} dBFS")

# ── T3: SoS PÅ + fader 0 — 'die Regulierung' stänger ─────────────
print("\n── T3: SoS PÅ + fader=0 — fader #12 stänger bouncen ──────────────")
plugin.echo_amount_r = 0.0
out = run(plugin, audio)
r_db = rms_db(out[1])
report("R tyst när fader #12 = 0 (manual §C)", r_db < -40.0, f"R = {r_db:+.1f} dBFS")

# ── T4: SoS PÅ + fader 1.0 — full bounce ─────────────────────────
print("\n── T4: SoS PÅ + fader=1.0 — full bounce ──────────────────────────")
plugin.echo_amount_r = 1.0
out = run(plugin, audio)
l_db, r_db = rms_db(out[0]), rms_db(out[1])
report("R-nivå inom 8 dB av L vid full fader", abs(r_db - l_db) < 8.0,
       f"L={l_db:+.1f}  R={r_db:+.1f}  Δ={r_db - l_db:+.1f} dB")

# ── T5: ENKELRIKTAD — R-only input får INTE läcka till L ─────────
print("\n── T5: Enkelriktning — R-källa läcker inte till L (manual: L→R) ──")
plugin.radio_gain_l = 0.0
plugin.radio_gain_r = 0.7
plugin.echo_amount_r = 0.8
audio_r = np.zeros((2, n_samples), dtype=np.float32)
audio_r[1] = make_sine(1000, n_samples, SR, amp=0.3)
out = run(plugin, audio_r)
l_db, r_db = rms_db(out[0]), rms_db(out[1])
report("R har signal", r_db > -50.0, f"R = {r_db:+.1f} dBFS")
report("L förblir tyst (bounce är enkelriktad L→R)", l_db < -40.0,
       f"L = {l_db:+.1f} dBFS")

# ── T6: Bounce-lagret passerar R:s tape-kedja (harmonik) ─────────
print("\n── T6: Bouncen passerar record-amp + tape (manual: Aufnahmeverstärker) ─")
plugin.radio_gain_l = 0.7
plugin.radio_gain_r = 0.0
plugin.echo_amount_r = 0.7
plugin.saturation_drive_l = 1.5
plugin.saturation_drive_r = 1.5
out = run(plugin, audio)

N = len(out[1])
spec_r = np.abs(np.fft.rfft(out[1] * np.hanning(N)))
freqs = np.fft.rfftfreq(N, 1.0 / SR)

def amp_at(f, tol_hz=15):
    mask = (freqs > f - tol_hz) & (freqs < f + tol_hz)
    return float(np.max(spec_r[mask])) if np.any(mask) else 0.0

fund, h2, h3 = amp_at(1000), amp_at(2000), amp_at(3000)
thd2_db = 20 * np.log10(h2 / fund + 1e-12)
thd3_db = 20 * np.log10(h3 / fund + 1e-12)
# Dubbel bandfärg (L-pass + R-pass): J-A-modellen ger odd-harmonik (H3) och
# kanalernas medvetna bias-asymmetri (±kAsymmetryAmount) ackumulerar even-
# harmonik (H2) över generationerna — det är 2-gen-bouncens signatur.
report("R-output har 3rd harmonic (full record→tape-kedja)",
       thd3_db > -70.0, f"H3 = {thd3_db:.1f} dB rel fund")
report("R-output har 2nd harmonic (asymmetri ackumulerad över 2 gen)",
       thd2_db > -70.0, f"H2 = {thd2_db:.1f} dB rel fund")

# ── Summering ────────────────────────────────────────────────────
n_pass, n_total = sum(results), len(results)
print()
print("══════════════════════════════════════════════════════════════════")
print(f"  RESULTAT: {n_pass}/{n_total} PASS")
print("══════════════════════════════════════════════════════════════════")
if n_pass == n_total:
    print("\n  ✅ S-on-S-routningen matchar service-manualen §C")
sys.exit(0 if n_pass == n_total else 1)
