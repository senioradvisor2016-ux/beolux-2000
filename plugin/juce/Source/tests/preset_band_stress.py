#!/usr/bin/env python3
"""
preset_band_stress.py — försöker reproducera kraschbuggen där appen
"stängs av" vid preset- eller band-byte.

Strategi:
  - Hammra tape_formula-byten (Agfa/BASF/Scotch), processa block emellan,
    leta NaN och permanent silence.
  - Hammra preset-program-byten via Program-paramets raw_value.
  - Korsvist preset+formula.
  - För varje övergång: probe-block efter, mät RMS/peak/NaN/DC.

Fail-kriterier:
  - NaN i output
  - ≥3 block i rad (>= 32 ms) under -120 dBFS
  - Peak > 1.5 (output limiter borde klampa)
  - Plötslig DC-bias > 0.1
"""

import sys, math
import numpy as np
import pedalboard

PLUGIN = "/Users/senioradvisor/Library/Audio/Plug-Ins/VST3/Beolux 2000.vst3"
SR = 48000
BLOCK = 512
WARM_BLOCKS = 30
PROBE_BLOCKS = 20
SILENCE_DB = -120.0
PERMA_SILENCE_BLOCKS = 3

g_pass = 0
g_fail = 0
fails = []


def report(name, ok, detail=""):
    global g_pass, g_fail
    tag = "[PASS]" if ok else "[FAIL]"
    line = f"  {tag} {name}"
    if detail:
        line += f"  ({detail})"
    print(line)
    if ok:
        g_pass += 1
    else:
        g_fail += 1
        fails.append(name)


def get_program_param(plugin):
    for p in plugin._parameters:
        if p.name == "Program":
            return p
    return None


def set_program(plugin, idx):
    p = get_program_param(plugin)
    if p is None or p.num_steps <= 1:
        return False
    p.raw_value = idx / (p.num_steps - 1)
    return True


def setup_plugin():
    plugin = pedalboard.load_plugin(PLUGIN)
    plugin.tape_formula = "Agfa"
    plugin.tape_speed = "9.5 cm/s"
    plugin.bypass_tape = False
    plugin.echo_enabled = False
    plugin.monitor_mode = "Tape"
    return plugin


def make_signal(n_blocks):
    rng = np.random.default_rng(seed=42)
    sig = rng.standard_normal((2, n_blocks * BLOCK)).astype(np.float32) * 0.2
    return sig


def process_blocks(plugin, signal):
    out = np.zeros_like(signal)
    n = signal.shape[1]
    for off in range(0, n, BLOCK):
        end = min(off + BLOCK, n)
        out[:, off:end] = plugin.process(signal[:, off:end], SR, reset=False)
    return out


def block_stats(out, n_blocks):
    stats = []
    for i in range(n_blocks):
        seg = out[:, i*BLOCK:(i+1)*BLOCK]
        if not np.all(np.isfinite(seg)):
            stats.append(dict(rms_db=float('-inf'), peak=float('inf'), nan=True, dc=0.0))
            continue
        rms = np.sqrt(np.mean(seg**2)) + 1e-30
        rms_db = 20 * math.log10(rms)
        stats.append(dict(rms_db=rms_db, peak=float(np.max(np.abs(seg))),
                          nan=False, dc=float(np.mean(seg))))
    return stats


def consecutive_silent(stats):
    longest = cur = 0
    for s in stats:
        if s['nan'] or s['rms_db'] < SILENCE_DB:
            cur += 1
            longest = max(longest, cur)
        else:
            cur = 0
    return longest


def run_transitions(label, transitions, prefix):
    print(f"\n── {label} ──")
    plugin = setup_plugin()
    signal = make_signal(WARM_BLOCKS + PROBE_BLOCKS)

    worst_silence = 0
    nan_seen = 0
    peak_max = 0.0
    dc_max = 0.0
    transition_failures = []

    for i, t in enumerate(transitions):
        try:
            for k, v in t.items():
                if k == 'program':
                    if not set_program(plugin, v):
                        raise RuntimeError("no Program param")
                else:
                    setattr(plugin, k, v)
        except Exception as e:
            transition_failures.append(f"#{i+1} {t}: {e}")
            continue

        out = process_blocks(plugin, signal)
        stats = block_stats(out, WARM_BLOCKS + PROBE_BLOCKS)
        probe = stats[WARM_BLOCKS:]

        sil = consecutive_silent(probe)
        if sil > worst_silence:
            worst_silence = sil
            worst_at = (i+1, t)
        nan_seen += sum(1 for s in probe if s['nan'])
        for s in probe:
            peak_max = max(peak_max, s['peak'])
            dc_max = max(dc_max, abs(s['dc']))

    if transition_failures:
        report(f"{prefix} alla övergångar applicerades",
               False,
               f"{len(transition_failures)}/{len(transitions)} fail (första: {transition_failures[0]})")
    else:
        report(f"{prefix} alla övergångar applicerades",
               True, f"{len(transitions)} st")

    silence_ms = worst_silence * BLOCK / SR * 1000
    detail = f"längsta tysta = {worst_silence} block ({silence_ms:.1f} ms)"
    if worst_silence >= PERMA_SILENCE_BLOCKS:
        detail += f"  efter {worst_at[1]}"
    report(f"{prefix} ingen permanent silence (< {PERMA_SILENCE_BLOCKS} block)",
           worst_silence < PERMA_SILENCE_BLOCKS, detail)

    report(f"{prefix} ingen NaN i probe-output",
           nan_seen == 0, f"NaN-block = {nan_seen}")

    report(f"{prefix} peak ≤ 1.5 (limiter funkar)",
           peak_max <= 1.5, f"peak = {peak_max:.3f}")

    report(f"{prefix} DC-offset rimlig (|DC| < 0.1)",
           dc_max < 0.1, f"max |DC| = {dc_max:.4f}")


def main():
    print("══════════════════════════════════════════════════════════════════")
    print("  Beolux 2000 — Preset/Band Stress Test")
    print(f"  pedalboard {pedalboard.__version__}  •  SR={SR}  •  block={BLOCK}")
    print("══════════════════════════════════════════════════════════════════")

    # 1. tape_formula spam
    formula_t = []
    for _ in range(8):
        for f in ("Agfa", "BASF", "Scotch"):
            formula_t.append({'tape_formula': f})
    run_transitions("Test 1: tape_formula spam (24 byten)",
                    formula_t, prefix="formula")

    # 2. preset (program) spam — testa alla 37 fabrikspresets, 3 cyklar
    plugin = setup_plugin()
    p = get_program_param(plugin)
    n_programs = p.num_steps if p else 0
    print(f"\n  (info: {n_programs} programs hittade)")

    if n_programs > 0:
        prog_t = []
        for _ in range(3):
            for idx in range(n_programs):
                prog_t.append({'program': idx})
        run_transitions(f"Test 2: program spam ({len(prog_t)} byten över {n_programs} program)",
                        prog_t, prefix="preset")
    else:
        report("preset Program-param finns", False, "no Program param")

    # 3. korsvist
    cross = []
    for i in range(15):
        cross.append({'tape_formula': ("Agfa", "BASF", "Scotch")[i % 3]})
        if n_programs > 0:
            cross.append({'program': i % n_programs})
    run_transitions(f"Test 3: korsvist preset + formula ({len(cross)} byten)",
                    cross, prefix="cross")

    # Final
    print("\n══════════════════════════════════════════════════════════════════")
    print(f"  RESULTAT:  {g_pass} godkända   {g_fail} underkända")
    print("══════════════════════════════════════════════════════════════════")
    if g_fail > 0:
        print("\n  ✗ Reproducerade buggen!")
        for f in fails:
            print(f"    FAIL: {f}")
        sys.exit(1)
    else:
        print("\n  ✓ Inga problem detekterade i pedalboard-host.")
        print("    (Kan ändå vara UI-tråd/standalone-specifik — kör i DAW.)")


if __name__ == '__main__':
    main()
