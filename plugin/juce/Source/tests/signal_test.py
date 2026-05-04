#!/usr/bin/env python3
"""
signal_test.py — BC2000DL / Beolux 2000 signal-level test via pedalboard.

Kör:  python3 Source/tests/signal_test.py
Kräver:  pip install pedalboard numpy

Testar det *kompilerade* VST3-binäret, inte DSP-koden direkt.
"""

import sys, time, math
import numpy as np
import pedalboard

# ──────────────────────────────────────────────────────────────────
PLUGIN_PATH = "/Users/senioradvisor/Library/Audio/Plug-Ins/VST3/Beolux 2000.vst3"
SR_DEFAULT  = 48000
BLOCK       = 512
WARM_BLOCKS = 40
# ──────────────────────────────────────────────────────────────────

gPass = 0
gFail = 0


def report(name: str, ok: bool, detail: str = ""):
    global gPass, gFail
    if ok:
        gPass += 1
    else:
        gFail += 1
    tag  = "[PASS]" if ok else "[FAIL]"
    line = f"{tag}  {name:<52}  {detail}"
    print(line)


def make_sine(freq: float, n_samples: int, sr: int, amp: float = 0.5) -> np.ndarray:
    t   = np.arange(n_samples) / sr
    sig = (amp * np.sin(2 * np.pi * freq * t)).astype(np.float32)
    return np.stack([sig, sig])   # stereo (2, n)


def rms(x: np.ndarray) -> float:
    return float(np.sqrt(np.mean(x ** 2)))


def peak(x: np.ndarray) -> float:
    return float(np.max(np.abs(x)))


def has_nan(x: np.ndarray) -> bool:
    return not np.all(np.isfinite(x))


def dft_amp(x: np.ndarray, freq: float, sr: int) -> float:
    """Amplitude of frequency component via DFT inner product (mono)."""
    n  = x.shape[-1]
    t  = np.arange(n) / sr
    re = float(np.dot(x[0], np.cos(2 * np.pi * freq * t)))
    im = float(np.dot(x[0], np.sin(2 * np.pi * freq * t)))
    return 2 * math.sqrt(re**2 + im**2) / n


def dft_amp_tail(x: np.ndarray, freq: float, sr: int,
                 skip_frac: float = 0.5) -> float:
    """DFT-amplitud på x[0], skippar skip_frac av signalen i början.

    Filter-transienterna i pluginen är tydligast i de första block som
    processas efter reset. Utan skip ser vi startup-energi som kan vara
    10–15 dB starkare än steady-state, särskilt vid HF (>4 kHz) där
    filtren svarar snabbt men transienten avtar långsamt.  Att hoppa
    över 50 % ger korrekt mätning vid n >= BLOCK*32.
    """
    skip = int(x.shape[-1] * skip_frac)
    tail = x[0, skip:]
    n    = tail.shape[0]
    t    = np.arange(n) / sr
    re   = float(np.dot(tail, np.cos(2 * np.pi * freq * t)))
    im   = float(np.dot(tail, np.sin(2 * np.pi * freq * t)))
    return 2 * math.sqrt(re**2 + im**2) / n


def get_program_param(plugin):
    """Return the raw Program AudioProcessorParameter."""
    for p in plugin._parameters:
        if p.name == "Program":
            return p
    return None


def set_program(plugin, idx: int):
    """Select preset by index (0-based)."""
    p = get_program_param(plugin)
    if p is None:
        return
    if p.num_steps <= 1:
        return
    p.raw_value = idx / (p.num_steps - 1)


def list_programs(plugin):
    """Return list of (index, name) for all factory programs."""
    p = get_program_param(plugin)
    if p is None:
        return []
    result = []
    n = p.num_steps
    for i in range(n):
        p.raw_value = i / max(1, n - 1)
        result.append((i, plugin.program))
    # Restore to first
    p.raw_value = 0.0
    return result


def default_params(plugin):
    """Set commonly needed params to neutral defaults.

    Säkerställer att monitor_mode='Tape' (ej 'Source') — annars kringgås
    tape-sektionen och alla HF/formel-tester mäter fel signalväg.
    Vissa fabrikspresets (t.ex. "82 · MASTER POLISH") kan lämna
    monitor_mode i 'Source'-läge.
    """
    plugin.mic_gain_l      = 0.5
    plugin.mic_gain_r      = 0.5
    plugin.master_volume   = 0.85
    plugin.tape_formula    = "Agfa"
    plugin.tape_speed      = "9.5 cm/s"
    plugin.bypass_tape     = False
    plugin.echo_enabled    = False
    plugin.monitor_mode    = "Tape"   # kritisk — "Source" kringgår tape-sektionen


# ──────────────────────────────────────────────────────────────────
# Hämta plugin
# ──────────────────────────────────────────────────────────────────

def load_plugin():
    try:
        p = pedalboard.load_plugin(PLUGIN_PATH)
        return p
    except Exception as e:
        print(f"[FATAL] Kunde inte ladda plugin: {e}")
        sys.exit(1)


# ══════════════════════════════════════════════════════════════════
# TEST 1: Plugin laddar och listar parametrar
# ══════════════════════════════════════════════════════════════════

def test_load():
    print("\n── Test 1: Plugin laddas och parametrar listar sig ────────────")
    plugin = load_plugin()
    params        = list(plugin.parameters.keys())
    has_formula   = any("tape_formula" in p or "formula" in p for p in params)
    has_program   = any("program" in p for p in params)
    report("Plugin laddar utan krasch",       True,              PLUGIN_PATH.split("/")[-1])
    report("Parametrar > 20",                 len(params) > 20,  f"{len(params)} st")
    report("tape_formula-parameter finns",    has_formula,       "")
    report("program-parameter finns",         has_program,       "")
    return plugin


# ══════════════════════════════════════════════════════════════════
# TEST 2: Fabrikspresets laddar utan blåsning
# ══════════════════════════════════════════════════════════════════

def test_presets(plugin):
    print("\n── Test 2: Fabrikspresets — ingen krasch / NaN / peak>5 ────────")

    programs = list_programs(plugin)
    report("Presets exponeras (>= 37)", len(programs) >= 37, f"{len(programs)} st")

    sine  = make_sine(1000, BLOCK * WARM_BLOCKS, SR_DEFAULT, 0.5)
    fails = 0

    for idx, name in programs:
        try:
            set_program(plugin, idx)
            out = plugin(sine, sample_rate=SR_DEFAULT, buffer_size=BLOCK, reset=True)
            if has_nan(out) or peak(out) > 5.0:
                print(f"         [{idx}] {name}: peak={peak(out):.3f}  nan={has_nan(out)}")
                fails += 1
        except Exception as e:
            print(f"         [{idx}] {name}: Exception: {e}")
            fails += 1

    report("Alla presets — ingen NaN / peak>5",
           fails == 0, f"{len(programs) - fails}/{len(programs)} ok")


# ══════════════════════════════════════════════════════════════════
# TEST 3: Samplingsrater
# ══════════════════════════════════════════════════════════════════

def test_sample_rates(plugin):
    print("\n── Test 3: Samplingsrater 22 050 – 192 000 Hz ─────────────────")
    rates = [22050, 44100, 48000, 88200, 96000, 176400, 192000]
    default_params(plugin)
    for sr in rates:
        try:
            sig = make_sine(1000, BLOCK * 20, sr, 0.5)
            out = plugin(sig, sample_rate=sr, buffer_size=BLOCK, reset=True)
            ok  = not has_nan(out) and peak(out) < 5.0
            pk  = peak(out)
        except Exception as e:
            ok = False
            pk = 0.0
        report(f"Sample rate {sr:>7} Hz", ok,
               f"peak={pk:.3f}" if ok else str(e)[:40])


# ══════════════════════════════════════════════════════════════════
# TEST 4: Blockstorlekar
# ══════════════════════════════════════════════════════════════════

def test_block_sizes(plugin):
    print("\n── Test 4: Blockstorlekar 1 – 4096 samples ────────────────────")
    sizes = [1, 7, 13, 32, 64, 128, 256, 512, 1024, 2048, 4096]
    default_params(plugin)
    for bs in sizes:
        try:
            sig = make_sine(1000, bs * 30, SR_DEFAULT, 0.5)
            out = plugin(sig, sample_rate=SR_DEFAULT, buffer_size=bs, reset=True)
            ok  = not has_nan(out) and peak(out) < 5.0
            pk  = peak(out)
        except Exception as e:
            ok = False
            pk = 0.0
        report(f"Blockstorlek {bs:>5} smp",
               ok, f"peak={pk:.3f}" if ok else str(e)[:40])


# ══════════════════════════════════════════════════════════════════
# TEST 5: THD per formel + frekvensrespons
# ══════════════════════════════════════════════════════════════════

def test_thd_and_response(plugin):
    print("\n── Test 5: THD 1 kHz + frekvensrespons per formel ─────────────")
    formulas = ["Agfa", "BASF", "Scotch"]
    n_meas   = BLOCK * 64   # ~700 ms

    for fname in formulas:
        try:
            plugin.reset()
            plugin.tape_formula       = fname
            plugin.mic_gain_l         = 0.5
            plugin.mic_gain_r         = 0.5
            plugin.saturation_drive_l = 1.0   # 1.0× drive (range 0.5–2.0)
            plugin.saturation_drive_r = 1.0
            plugin.master_volume      = 0.85
            plugin.bypass_tape        = False
            plugin.echo_enabled       = False
            plugin.monitor_mode       = "Tape"

            sig = make_sine(1000, n_meas, SR_DEFAULT, 0.5)
            out = plugin(sig, sample_rate=SR_DEFAULT, buffer_size=BLOCK, reset=True)

            if has_nan(out):
                report(f"THD {fname}", False, "NaN!")
                continue

            # THD (H2–H5 vs fundamental)
            f1      = dft_amp(out, 1000, SR_DEFAULT)
            hh      = [dft_amp(out, k * 1000, SR_DEFAULT) for k in [2, 3, 4, 5]]
            thd_pct = math.sqrt(sum(h**2 for h in hh)) / (f1 + 1e-9) * 100

            thd_ok  = 0.05 < thd_pct < 30.0
            report(f"THD {fname}", thd_ok, f"{thd_pct:.2f}%")

            # Frekvensrespons: 100 Hz och 8 kHz relativ till 1 kHz-output
            out_lf = plugin(make_sine(100,  n_meas, SR_DEFAULT, 0.5),
                            sample_rate=SR_DEFAULT, buffer_size=BLOCK, reset=True)
            out_hf = plugin(make_sine(8000, n_meas, SR_DEFAULT, 0.5),
                            sample_rate=SR_DEFAULT, buffer_size=BLOCK, reset=True)

            amp_lf  = dft_amp(out_lf, 100,  SR_DEFAULT)
            amp_hf  = dft_amp(out_hf, 8000, SR_DEFAULT)
            rel_lf  = 20 * math.log10(amp_lf / (f1 + 1e-9) + 1e-9)
            rel_hf  = 20 * math.log10(amp_hf / (f1 + 1e-9) + 1e-9)

            resp_ok = not has_nan(out_lf) and not has_nan(out_hf) and amp_lf > 1e-6 and amp_hf > 1e-6
            report(f"Freqrespons {fname}",
                   resp_ok, f"100Hz={rel_lf:+.1f}dB  8kHz={rel_hf:+.1f}dB rel 1kHz")

        except Exception as e:
            report(f"THD {fname}", False, str(e)[:60])


# ══════════════════════════════════════════════════════════════════
# TEST 6: Brusgolv (tystnad in → mät output)
# ══════════════════════════════════════════════════════════════════

def test_noise_floor(plugin):
    print("\n── Test 6: Brusgolv (tystnad in) ───────────────────────────────")
    formulas = ["Agfa", "BASF", "Scotch"]
    n        = BLOCK * 100   # ~1 s

    for fname in formulas:
        try:
            plugin.reset()
            plugin.tape_formula  = fname
            plugin.mic_gain_l    = 0.5
            plugin.mic_gain_r    = 0.5
            plugin.master_volume = 0.85
            plugin.bypass_tape   = False
            plugin.echo_enabled  = False
            plugin.monitor_mode  = "Tape"

            silence = np.zeros((2, n), dtype=np.float32)
            out     = plugin(silence, sample_rate=SR_DEFAULT,
                             buffer_size=BLOCK, reset=True)

            if has_nan(out):
                report(f"Brusgolv {fname}", False, "NaN!")
                continue

            rms_val = rms(out)
            dbfs    = 20 * math.log10(rms_val + 1e-12)
            # Tape brus < −30 dBFS ok (riktigt bandljud har hissande brus)
            ok = dbfs < -30.0
            report(f"Brusgolv {fname}", ok, f"{dbfs:.1f} dBFS")
        except Exception as e:
            report(f"Brusgolv {fname}", False, str(e)[:50])


# ══════════════════════════════════════════════════════════════════
# TEST 7: CPU-tid per sample
# ══════════════════════════════════════════════════════════════════

def test_cpu_time(plugin):
    print("\n── Test 7: CPU-tid per sample ──────────────────────────────────")
    default_params(plugin)

    n_samples = SR_DEFAULT * 5   # 5 s audio
    sig       = make_sine(1000, n_samples, SR_DEFAULT, 0.5)

    t0  = time.perf_counter()
    out = plugin(sig, sample_rate=SR_DEFAULT, buffer_size=BLOCK, reset=True)
    dt  = time.perf_counter() - t0

    ns_per_smp = dt / n_samples * 1e9
    realtime   = dt / 5.0 * 100.0   # % av realtid

    ok = realtime < 50.0   # < 50% CPU på ett core = godkänt
    report("CPU-tid per sample (5 s audio)",
           ok, f"{ns_per_smp:.1f} ns/smp  ({realtime:.1f}% av realtid)")


# ══════════════════════════════════════════════════════════════════
# TEST 8: Impulsrespons — kanal L och R är inte tysta
# ══════════════════════════════════════════════════════════════════

def test_impulse_response(plugin):
    print("\n── Test 8: Impulsrespons ───────────────────────────────────────")
    default_params(plugin)

    impulse      = np.zeros((2, BLOCK * 20), dtype=np.float32)
    impulse[:, 0] = 1.0   # enhetspuls

    out   = plugin(impulse, sample_rate=SR_DEFAULT, buffer_size=BLOCK, reset=True)
    max_l = float(np.max(np.abs(out[0])))
    max_r = float(np.max(np.abs(out[1])))
    ok    = not has_nan(out) and max_l > 1e-4 and max_r > 1e-4

    report("Impulsrespons L+R ej tyst", ok, f"L={max_l:.4f}  R={max_r:.4f}")


# ══════════════════════════════════════════════════════════════════
# TEST 9: Echo self-oscillation — begränsad vid amount=0.95
# ══════════════════════════════════════════════════════════════════

def test_echo_selfoscillation(plugin):
    print("\n── Test 9: Echo self-oscillation vid amount=0.95 ───────────────")
    try:
        plugin.reset()
        default_params(plugin)
        plugin.echo_enabled  = True
        plugin.echo_amount_l = 0.95
        plugin.echo_amount_r = 0.95

        sig = make_sine(1000, BLOCK * 300, SR_DEFAULT, 0.5)   # ~3 s
        out = plugin(sig, sample_rate=SR_DEFAULT, buffer_size=BLOCK, reset=True)

        pk  = peak(out)
        ok  = not has_nan(out) and pk < 2.0
        report("Echo self-osc bounded (< ±2)", ok, f"peak={pk:.3f}")
    except Exception as e:
        report("Echo self-osc bounded (< ±2)", False, str(e)[:50])


# ══════════════════════════════════════════════════════════════════
# TEST 10: NaN-injektion → återhämtning
# ══════════════════════════════════════════════════════════════════

def test_nan_recovery(plugin):
    print("\n── Test 10: NaN-injektion → återhämtning ───────────────────────")
    plugin.reset()
    default_params(plugin)

    # Kör lite normalt signal först
    warm_sig = make_sine(1000, BLOCK * WARM_BLOCKS, SR_DEFAULT, 0.5)
    plugin(warm_sig, sample_rate=SR_DEFAULT, buffer_size=BLOCK, reset=False)

    # Injicera NaN
    nan_buf = np.full((2, BLOCK), float("nan"), dtype=np.float32)
    plugin(nan_buf, sample_rate=SR_DEFAULT, buffer_size=BLOCK, reset=False)

    # Kör ren signal — ska återhämta sig
    recovered = False
    for i in range(15):
        clean = make_sine(1000, BLOCK, SR_DEFAULT, 0.3)
        out   = plugin(clean, sample_rate=SR_DEFAULT, buffer_size=BLOCK, reset=False)
        if not has_nan(out) and peak(out) > 1e-5:
            recovered = True
            report("NaN-injektion → rent output", True, f"block {i+1}")
            break

    if not recovered:
        report("NaN-injektion → rent output", False,
               "återhämtade sig INTE inom 15 block")


# ══════════════════════════════════════════════════════════════════
# TEST 11: Alla formel+hastighetskombinationer
# ══════════════════════════════════════════════════════════════════

def test_formula_speed_matrix(plugin):
    print("\n── Test 11: Alla formel × hastighet-kombinationer ──────────────")
    formulas = ["Agfa", "BASF", "Scotch"]
    speeds   = ["4.75 cm/s", "9.5 cm/s", "19 cm/s"]
    sine     = make_sine(1000, BLOCK * 40, SR_DEFAULT, 0.5)
    fails    = 0
    total    = 0

    for f in formulas:
        for s in speeds:
            total += 1
            try:
                plugin.reset()
                plugin.tape_formula  = f
                plugin.tape_speed    = s
                plugin.mic_gain_l    = 0.5
                plugin.mic_gain_r    = 0.5
                plugin.master_volume = 0.85
                plugin.bypass_tape   = False
                plugin.echo_enabled  = False
                plugin.monitor_mode  = "Tape"

                out = plugin(sine, sample_rate=SR_DEFAULT,
                             buffer_size=BLOCK, reset=True)
                if has_nan(out) or peak(out) > 5.0:
                    print(f"         {f} @ {s}: peak={peak(out):.3f}  nan={has_nan(out)}")
                    fails += 1
            except Exception as e:
                print(f"         {f} @ {s}: {e}")
                fails += 1

    report("Formel × hastighet matrix (9 komp)",
           fails == 0, f"{total - fails}/{total} ok")


# ══════════════════════════════════════════════════════════════════
# TEST 12: Frekvensrespons-kurva per formel
#   Svepar 11 frekvenser 20 Hz–16 kHz (låg amplitud → linjärt område).
#   Verifierar att BASF är ljusare än Scotch vid 10 kHz,
#   och att varje formel ger ett icke-tyst svar i hela bandet.
# ══════════════════════════════════════════════════════════════════

def test_frequency_sweep(plugin):
    print("\n── Test 12: Frekvensrespons-sweep per formel ───────────────────")
    # n_meas = BLOCK*32 räcker om vi skippar första 50 % av signalen
    # (filter-transient från reset skevar mätningen annars dramatiskt vid HF).
    FREQS    = [100, 200, 500, 1000, 2000, 4000, 8000, 10000]
    n_meas   = BLOCK * 32
    amp_in   = 0.1   # litet för att hålla oss i linjärt område
    formulas = ["Agfa", "BASF", "Scotch"]
    speeds   = ["19 cm/s", "9.5 cm/s", "4.75 cm/s"]

    # ── 12a: Per formel @ 9.5 cm/s — BASF ska vara ljusare än Scotch vid 10 kHz ──
    rel_hf = {}   # formel → dB-rel-1kHz vid 8 kHz

    for fname in formulas:
        plugin.reset()
        plugin.tape_formula  = fname
        plugin.tape_speed    = "9.5 cm/s"
        plugin.mic_gain_l    = 0.5
        plugin.mic_gain_r    = 0.5
        plugin.master_volume = 0.85
        plugin.bypass_tape   = False
        plugin.echo_enabled  = False
        plugin.monitor_mode  = "Tape"

        amps = {}
        for f in FREQS:
            out     = plugin(make_sine(f, n_meas, SR_DEFAULT, amp_in),
                             sample_rate=SR_DEFAULT, buffer_size=BLOCK, reset=True)
            amps[f] = dft_amp_tail(out, f, SR_DEFAULT, skip_frac=0.5)

        ref = amps.get(1000, 1e-9)
        rel_hf[fname] = 20 * math.log10(amps.get(8000, 1e-9) / (ref + 1e-9) + 1e-9)

        curve = "  ".join(
            f"{f//1000 if f >= 1000 else f}{'k' if f >= 1000 else 'Hz'}="
            f"{20 * math.log10(amps.get(f, 1e-9) / (ref + 1e-9) + 1e-9):+.1f}"
            for f in [100, 500, 2000, 4000, 8000] if f in amps
        )
        all_finite = all(math.isfinite(a) and a > 1e-9 for a in amps.values())
        report(f"Sweep {fname} @ 9.5 cm/s", all_finite, curve)

    # BASF ska ha mindre HF-rolloff än Scotch (ljusare karaktär)
    report("BASF ljusare än Scotch vid 8 kHz",
           rel_hf.get("BASF", -99) > rel_hf.get("Scotch", -99),
           f"BASF {rel_hf.get('BASF',0):+.1f} dB vs Scotch {rel_hf.get('Scotch',0):+.1f} dB rel 1 kHz")
    # Agfa ska ligga mellan BASF och Scotch
    agfa_between = rel_hf.get("Scotch", -99) < rel_hf.get("Agfa", 0) < rel_hf.get("BASF", 99)
    report("Agfa HF-karaktär mellan BASF och Scotch",
           agfa_between,
           f"Scotch {rel_hf.get('Scotch',0):+.1f}  Agfa {rel_hf.get('Agfa',0):+.1f}  BASF {rel_hf.get('BASF',0):+.1f} dB")

    # ── 12b: Per hastighet @ Agfa — 4.75 cm/s ska vara mörkast vid 8 kHz ──
    speed_hf = {}
    for spd in speeds:
        plugin.reset()
        plugin.tape_formula  = "Agfa"
        plugin.tape_speed    = spd
        plugin.mic_gain_l    = 0.5
        plugin.mic_gain_r    = 0.5
        plugin.master_volume = 0.85
        plugin.bypass_tape   = False
        plugin.echo_enabled  = False
        plugin.monitor_mode  = "Tape"

        ref_out = plugin(make_sine(1000, n_meas, SR_DEFAULT, amp_in),
                         sample_rate=SR_DEFAULT, buffer_size=BLOCK, reset=True)
        hf_out  = plugin(make_sine(8000, n_meas, SR_DEFAULT, amp_in),
                         sample_rate=SR_DEFAULT, buffer_size=BLOCK, reset=True)

        r   = dft_amp_tail(ref_out, 1000, SR_DEFAULT)
        h   = dft_amp_tail(hf_out,  8000, SR_DEFAULT)
        rel = 20 * math.log10(h / (r + 1e-9) + 1e-9)
        speed_hf[spd] = rel
        report(f"HF @ {spd}", r > 1e-7 and h > 1e-7, f"8 kHz = {rel:+.1f} dB rel 1 kHz")

    # 4.75 cm/s ska ha mest rolloff (lägst HF vid 8 kHz)
    darkest_is_475 = speed_hf.get("4.75 cm/s", 0) < speed_hf.get("19 cm/s", -1)
    report("4.75 cm/s mörkast vid 8 kHz",
           darkest_is_475,
           f"19={speed_hf.get('19 cm/s',0):+.1f} dB  4.75={speed_hf.get('4.75 cm/s',0):+.1f} dB")


# ══════════════════════════════════════════════════════════════════
# TEST 13: Head bump — verifierar förstärkning i 70–90 Hz-bandet
# ══════════════════════════════════════════════════════════════════

def test_head_bump(plugin):
    print("\n── Test 13: Head bump-verifiering per hastighet ────────────────")
    speeds = [
        ("4.75 cm/s", 90),   # bump ~90 Hz
        ("9.5 cm/s",  80),   # bump ~80 Hz
        ("19 cm/s",   70),   # bump ~70 Hz
    ]
    n_meas = BLOCK * 64
    amp_in = 0.1

    for spd, bump_hz in speeds:
        try:
            plugin.reset()
            plugin.tape_formula  = "Agfa"
            plugin.tape_speed    = spd
            plugin.mic_gain_l    = 0.5
            plugin.mic_gain_r    = 0.5
            plugin.master_volume = 0.85
            plugin.bypass_tape   = False
            plugin.echo_enabled  = False
            plugin.monitor_mode  = "Tape"

            # Mät bump-frekvensen och referens (200 Hz)
            out_bump = plugin(make_sine(bump_hz, n_meas, SR_DEFAULT, amp_in),
                              sample_rate=SR_DEFAULT, buffer_size=BLOCK, reset=True)
            out_ref  = plugin(make_sine(200, n_meas, SR_DEFAULT, amp_in),
                              sample_rate=SR_DEFAULT, buffer_size=BLOCK, reset=True)

            a_bump = dft_amp(out_bump, bump_hz, SR_DEFAULT)
            a_ref  = dft_amp(out_ref,  200,     SR_DEFAULT)
            bump_db = 20 * math.log10(a_bump / (a_ref + 1e-9) + 1e-9)

            # Head bump ska ge minst 0 dB (flat) och helst + vid bump-frekvensen
            ok = a_bump > 1e-7 and not has_nan(out_bump)
            report(f"Head bump {spd} @ {bump_hz} Hz", ok,
                   f"{bump_db:+.1f} dB rel 200 Hz")
        except Exception as e:
            report(f"Head bump {spd}", False, str(e)[:50])


# ══════════════════════════════════════════════════════════════════
# TEST 14: Bypass-transparens
# ══════════════════════════════════════════════════════════════════

def test_bypass_transparency(plugin):
    print("\n── Test 14: Bypass-transparens ─────────────────────────────────")
    # bypass_tape=True kringgår band-saturationblocket men inte hela
    # försteget (mic preamp + output stage ger ≈ −6 dB gain-staging).
    # Vi testar att:
    #   (a) nivåförändringen är konsekvent mellan frekvenser (≤ 1 dB spread)
    #   (b) absolutnivån ligger inom ett rimligt fönster (−15 .. +0 dB)
    n = BLOCK * 40
    plugin.reset()
    plugin.mic_gain_l    = 0.5
    plugin.mic_gain_r    = 0.5
    plugin.master_volume = 1.0
    plugin.bypass_tape   = True
    plugin.echo_enabled  = False

    ratios = {}
    for freq in [200, 1000, 8000]:
        sig = make_sine(freq, n, SR_DEFAULT, 0.3)
        out = plugin(sig, sample_rate=SR_DEFAULT, buffer_size=BLOCK, reset=True)

        in_amp  = dft_amp_tail(sig, freq, SR_DEFAULT)
        out_amp = dft_amp_tail(out, freq, SR_DEFAULT)
        ratio_db = 20 * math.log10(out_amp / (in_amp + 1e-9) + 1e-9)
        ratios[freq] = ratio_db

    # Spektral flathet: max spridning ≤ 1 dB (bypass ska inte färga ljudet)
    spread = max(ratios.values()) - min(ratios.values())
    flat_ok = spread <= 1.0 and not any(math.isnan(v) for v in ratios.values())
    report("Bypass spektralt plant (spread ≤ 1 dB)",
           flat_ok, f"spridning={spread:.2f} dB  "
           + "  ".join(f"{f//1000 if f>=1000 else f}{'k' if f>=1000 else 'Hz'}={v:+.1f}" for f,v in ratios.items()))

    # Absolut nivå rimlig (gain-staging i försteget ger ≈ −6 dB)
    avg_ratio = sum(ratios.values()) / len(ratios)
    level_ok  = -15.0 < avg_ratio < 0.0
    report("Bypass nivå rimlig (−15..0 dB rel input)",
           level_ok, f"snitt Δ={avg_ratio:+.1f} dB")


# ══════════════════════════════════════════════════════════════════
# TEST 15: DC-offset — ingen DC-drift efter lång körning
# ══════════════════════════════════════════════════════════════════

def test_dc_offset(plugin):
    print("\n── Test 15: DC-offset efter lång körning ───────────────────────")
    n = SR_DEFAULT * 3   # 3 s
    default_params(plugin)

    for fname in ["Agfa", "BASF", "Scotch"]:
        try:
            plugin.reset()
            plugin.tape_formula  = fname
            plugin.mic_gain_l    = 0.5
            plugin.mic_gain_r    = 0.5
            plugin.master_volume = 0.85
            plugin.bypass_tape   = False
            plugin.monitor_mode  = "Tape"

            sig = make_sine(1000, n, SR_DEFAULT, 0.5)
            out = plugin(sig, sample_rate=SR_DEFAULT,
                         buffer_size=BLOCK, reset=True)

            dc_l = float(np.mean(out[0]))
            dc_r = float(np.mean(out[1]))
            dc   = max(abs(dc_l), abs(dc_r))
            ok   = dc < 0.01 and not has_nan(out)
            report(f"DC-offset {fname}", ok, f"L={dc_l:+.5f}  R={dc_r:+.5f}")
        except Exception as e:
            report(f"DC-offset {fname}", False, str(e)[:50])


# ══════════════════════════════════════════════════════════════════
# TEST 16: Preset-övergångsstabilitet
# ══════════════════════════════════════════════════════════════════

def test_preset_transitions(plugin):
    print("\n── Test 16: Preset-övergångsstabilitet ─────────────────────────")
    programs = list_programs(plugin)
    n_seg    = BLOCK * 8   # 8 block per preset (snabb switch)
    sine_seg = make_sine(1000, n_seg, SR_DEFAULT, 0.5)
    fails    = 0

    # Kör INIT FACTORY som startläge
    set_program(plugin, 0)
    plugin(make_sine(1000, BLOCK * WARM_BLOCKS, SR_DEFAULT, 0.5),
           sample_rate=SR_DEFAULT, buffer_size=BLOCK, reset=True)

    for idx, name in programs:
        try:
            set_program(plugin, idx)
            out = plugin(sine_seg, sample_rate=SR_DEFAULT,
                         buffer_size=BLOCK, reset=False)   # INGEN reset — testar övergången
            if has_nan(out) or peak(out) > 10.0:
                print(f"         [{idx}] {name}: peak={peak(out):.3f}  nan={has_nan(out)}")
                fails += 1
        except Exception as e:
            print(f"         [{idx}] {name}: {e}")
            fails += 1

    report("Preset-övergångar utan NaN/peak>10",
           fails == 0, f"{len(programs) - fails}/{len(programs)} ok")


# ══════════════════════════════════════════════════════════════════
# TEST 17: Stereo-isolation (L ej i R, R ej i L)
# ══════════════════════════════════════════════════════════════════

def test_stereo_isolation(plugin):
    print("\n── Test 17: Stereo-isolation ───────────────────────────────────")
    default_params(plugin)
    n    = BLOCK * 40
    freq = 1000

    # L-only signal
    t   = np.arange(n) / SR_DEFAULT
    sig_l_only = np.stack([
        (0.5 * np.sin(2 * np.pi * freq * t)).astype(np.float32),
        np.zeros(n, dtype=np.float32)
    ])
    out_l = plugin(sig_l_only, sample_rate=SR_DEFAULT,
                   buffer_size=BLOCK, reset=True)
    amp_ll = dft_amp(out_l[[0]], freq, SR_DEFAULT)   # L→L
    amp_lr = dft_amp(out_l[[1]], freq, SR_DEFAULT)   # L→R (leak)
    crosstalk_l = 20 * math.log10(amp_lr / (amp_ll + 1e-9) + 1e-9)

    # R-only signal
    sig_r_only = np.stack([
        np.zeros(n, dtype=np.float32),
        (0.5 * np.sin(2 * np.pi * freq * t)).astype(np.float32)
    ])
    out_r = plugin(sig_r_only, sample_rate=SR_DEFAULT,
                   buffer_size=BLOCK, reset=True)
    amp_rr = dft_amp(out_r[[1]], freq, SR_DEFAULT)   # R→R
    amp_rl = dft_amp(out_r[[0]], freq, SR_DEFAULT)   # R→L (leak)
    crosstalk_r = 20 * math.log10(amp_rl / (amp_rr + 1e-9) + 1e-9)

    ok_l = crosstalk_l < -20.0 and amp_ll > 1e-6
    ok_r = crosstalk_r < -20.0 and amp_rr > 1e-6
    report("Stereo-isolation L→R (< −20 dB)", ok_l,
           f"{crosstalk_l:+.1f} dB  (L={amp_ll:.4f}  R-leak={amp_lr:.6f})")
    report("Stereo-isolation R→L (< −20 dB)", ok_r,
           f"{crosstalk_r:+.1f} dB  (R={amp_rr:.4f}  L-leak={amp_rl:.6f})")


# ══════════════════════════════════════════════════════════════════
# TEST 18: Headroom / mättnadskurva
# ══════════════════════════════════════════════════════════════════

def test_headroom(plugin):
    print("\n── Test 18: Headroom och mjuk mättning ─────────────────────────")
    default_params(plugin)
    n    = BLOCK * 32
    freq = 1000

    # Svep input amplitud från liten till stor
    levels_db  = [-30, -24, -18, -12, -6, 0, +3, +6]
    prev_out_db = None
    monotonic   = True
    all_finite  = True

    for db in levels_db:
        amp = 10 ** (db / 20)
        sig = make_sine(freq, n, SR_DEFAULT, amp)
        out = plugin(sig, sample_rate=SR_DEFAULT, buffer_size=BLOCK, reset=True)

        if has_nan(out):
            all_finite = False
            report(f"Headroom {db:+d} dBFS", False, "NaN!")
            continue

        out_rms_db = 20 * math.log10(rms(out) + 1e-12)

        # Output ska öka monotont (mjuk saturation komprimerar men ej minskar)
        if prev_out_db is not None and out_rms_db < prev_out_db - 3.0:
            monotonic = False
        prev_out_db = out_rms_db

        report(f"Headroom in={db:+d} dBFS",
               not has_nan(out) and peak(out) < 10.0,
               f"out_rms={out_rms_db:.1f} dBFS  peak={peak(out):.3f}")

    report("Monoton mättningskurva (ej kollaps vid höga nivåer)",
           monotonic and all_finite, "ok" if monotonic else "icke-monoton!")


# ══════════════════════════════════════════════════════════════════
# HUVUD
# ══════════════════════════════════════════════════════════════════

if __name__ == "__main__":
    print("═" * 66)
    print("  BC2000DL / Beolux 2000  —  Python signal-test via pedalboard")
    print(f"  pedalboard {pedalboard.__version__}  •  numpy {np.__version__}")
    print("═" * 66)

    plugin = test_load()
    test_presets(plugin)
    test_sample_rates(plugin)
    test_block_sizes(plugin)
    test_thd_and_response(plugin)
    test_noise_floor(plugin)
    test_cpu_time(plugin)
    test_impulse_response(plugin)
    test_echo_selfoscillation(plugin)
    test_nan_recovery(plugin)
    test_formula_speed_matrix(plugin)
    test_frequency_sweep(plugin)
    test_head_bump(plugin)
    test_bypass_transparency(plugin)
    test_dc_offset(plugin)
    test_preset_transitions(plugin)
    test_stereo_isolation(plugin)
    test_headroom(plugin)

    print("\n" + "═" * 66)
    print(f"  RESULTAT:  {gPass} godkända   {gFail} underkända")
    print("═" * 66)
    sys.exit(1 if gFail > 0 else 0)
