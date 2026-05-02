"""Process audio file — dra in en WAV-fil och få ut BC2000DL-processed version.

Användning:
    python3 process_file.py input.wav output.wav [options]

Eller programmatiskt:
    from process_file import process_wav
    process_wav("input.wav", "output.wav", speed="19", echo=True, ...)
"""
from __future__ import annotations
import argparse
from pathlib import Path
import numpy as np
import soundfile as sf

from chain import SignalChain, ChainConfig, MonitorMode
from eq_din1962 import TapeSpeed
from preamps import MicInputMode


_SPEED_MAP = {
    "19": TapeSpeed.SPEED_19,
    "9.5": TapeSpeed.SPEED_9_5,
    "4.75": TapeSpeed.SPEED_4_75,
}


def process_wav(input_path: str | Path,
                output_path: str | Path,
                *,
                speed: str = "19",
                input_bus: str = "mic",
                input_gain: float = 0.5,
                # Tape parametrar
                bias_amount: float = 1.0,
                saturation_drive: float = 1.0,
                wow_flutter_amount: float = 1.0,
                # Effekter
                echo: bool = False,
                echo_amount: float = 0.5,
                multiplay_generation: int = 1,
                # Tone + balance
                bass_db: float = 0.0,
                treble_db: float = 0.0,
                balance: float = 0.0,
                master_volume: float = 0.75,
                # Output
                speaker_monitor: bool = False,
                bypass_tape: bool = False,
                normalize_db: float = -1.0):
    """Processa en WAV-fil genom BC2000DL-kedjan."""
    input_path = Path(input_path)
    output_path = Path(output_path)

    # Ladda
    audio, sr = sf.read(str(input_path))
    print(f"  Loaded: {input_path.name}  ({audio.shape}, {sr} Hz)")

    # Mono?
    if audio.ndim == 1:
        l_in = r_in = audio.astype(np.float64)
        is_mono = True
    else:
        l_in = audio[:, 0].astype(np.float64)
        r_in = audio[:, 1].astype(np.float64)
        is_mono = False

    # Bygg config
    cfg = ChainConfig(
        sample_rate=sr,
        speed=_SPEED_MAP[speed],
        bias_amount=bias_amount,
        saturation_drive=saturation_drive,
        wow_flutter_amount=wow_flutter_amount,
        echo_enabled=echo,
        echo_amount=echo_amount,
        multiplay_enabled=(multiplay_generation > 1),
        multiplay_generation=multiplay_generation,
        bass_db=bass_db,
        treble_db=treble_db,
        balance=balance,
        master_volume=master_volume,
        speaker_monitor=speaker_monitor,
        bypass_tape=bypass_tape,
    )
    if input_bus == "mic":
        cfg.mic_gain = input_gain
    elif input_bus == "phono":
        cfg.phono_gain = input_gain
    elif input_bus == "radio":
        cfg.radio_gain = input_gain
    else:
        raise ValueError(f"Unknown bus: {input_bus}")

    # Kör genom kedjan
    chain = SignalChain(cfg)
    if input_bus == "mic":
        l_out, r_out = chain.process_stereo(mic_lr=(l_in, r_in))
    elif input_bus == "phono":
        l_out, r_out = chain.process_stereo(phono_lr=(l_in, r_in))
    else:
        l_out, r_out = chain.process_stereo(radio_lr=(l_in, r_in))

    # Normalisera och spara
    stereo = np.stack([l_out, r_out], axis=-1)
    peak = np.max(np.abs(stereo))
    target = 10 ** (normalize_db / 20)
    if peak > 0:
        stereo = stereo * (target / peak)
    sf.write(str(output_path), stereo, sr, subtype="PCM_24")
    print(f"  Saved:  {output_path.name}  (peak normalized to {normalize_db} dBFS)")


def main():
    parser = argparse.ArgumentParser(
        description="BC2000DL DSP-prototyp — processa en audio-fil")
    parser.add_argument("input", help="Input WAV-fil")
    parser.add_argument("output", help="Output WAV-fil")
    parser.add_argument("--speed", choices=["19", "9.5", "4.75"], default="19",
                        help="Tape-hastighet i cm/s (default: 19)")
    parser.add_argument("--bus", choices=["mic", "phono", "radio"], default="mic",
                        help="Input-buss (default: mic)")
    parser.add_argument("--gain", type=float, default=0.5,
                        help="Input-gain 0–1 (default: 0.5)")
    parser.add_argument("--bias", type=float, default=1.0,
                        help="Bias-mängd (default: 1.0 = nominell)")
    parser.add_argument("--saturation", type=float, default=1.0,
                        help="Saturation-drive (default: 1.0)")
    parser.add_argument("--wf", type=float, default=1.0,
                        help="Wow & flutter-mängd (default: 1.0)")
    parser.add_argument("--echo", action="store_true",
                        help="Aktivera echo (per-speed delay-tid)")
    parser.add_argument("--echo-amount", type=float, default=0.5,
                        help="Echo level 0–1 (default: 0.5)")
    parser.add_argument("--multiplay-gen", type=int, default=1,
                        help="Multiplay-generation 1–4 (default: 1=ren)")
    parser.add_argument("--bass", type=float, default=0.0,
                        help="Bass dB (-12...+12)")
    parser.add_argument("--treble", type=float, default=0.0,
                        help="Treble dB (-12...+12)")
    parser.add_argument("--balance", type=float, default=0.0,
                        help="Balance -1...+1 (default: 0)")
    parser.add_argument("--master", type=float, default=0.75,
                        help="Master-volume 0–1 (default: 0.75)")
    parser.add_argument("--speaker", action="store_true",
                        help="Speaker-monitor-läge (power-amp aktiv)")
    parser.add_argument("--bypass-tape", action="store_true",
                        help="Bypass tape (amp-alone-läge)")

    args = parser.parse_args()
    process_wav(
        args.input, args.output,
        speed=args.speed, input_bus=args.bus, input_gain=args.gain,
        bias_amount=args.bias, saturation_drive=args.saturation,
        wow_flutter_amount=args.wf,
        echo=args.echo, echo_amount=args.echo_amount,
        multiplay_generation=args.multiplay_gen,
        bass_db=args.bass, treble_db=args.treble,
        balance=args.balance, master_volume=args.master,
        speaker_monitor=args.speaker, bypass_tape=args.bypass_tape,
    )


if __name__ == "__main__":
    main()
