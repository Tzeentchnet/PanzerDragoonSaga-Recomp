#!/usr/bin/env python3

import argparse
import array
import csv
import glob
import hashlib
import math
import pathlib
import sys
import wave


def read_manifest(path):
    with path.open(newline="", encoding="ascii") as stream:
        return {
            int(row["frame"]): (
                row["game_state"],
                row["vdp1"],
                row["vdp2"],
            )
            for row in csv.DictReader(stream)
        }


def compare_manifests(expected_path, actual_path):
    expected = read_manifest(expected_path)
    actual = read_manifest(actual_path)
    if expected.keys() != actual.keys():
        missing = sorted(expected.keys() - actual.keys())
        extra = sorted(actual.keys() - expected.keys())
        raise ValueError(f"manifest frames differ; missing={missing[:8]} extra={extra[:8]}")

    for frame in expected:
        if expected[frame] != actual[frame]:
            raise ValueError(
                f"frame {frame} differs: expected={expected[frame]} actual={actual[frame]}"
            )


def read_stereo_wav(path):
    with wave.open(str(path), "rb") as stream:
        if (
            stream.getnchannels() != 2
            or stream.getsampwidth() != 2
            or stream.getframerate() != 44_100
            or stream.getcomptype() != "NONE"
        ):
            raise ValueError(f"{path} must be uncompressed 44.1 kHz 16-bit stereo PCM")
        samples = array.array("h", stream.readframes(stream.getnframes()))
    if sys.byteorder != "little":
        samples.byteswap()
    return samples


def stereo_mono(samples):
    return array.array(
        "i",
        (
            int(samples[index]) + int(samples[index + 1])
            for index in range(0, len(samples), 2)
        ),
    )


def offset_error(expected, actual, offset, sample_count):
    expected_start = max(0, offset)
    actual_start = max(0, -offset)
    count = min(
        sample_count,
        len(expected) - expected_start,
        len(actual) - actual_start,
    )
    if count <= 0:
        return math.inf
    stride = max(1, count // 20_000)
    squared_error = 0
    points = 0
    for index in range(0, count, stride):
        delta = expected[expected_start + index] - actual[actual_start + index]
        squared_error += delta * delta
        points += 1
    return squared_error / points


def find_best_offset(expected, actual, max_offset):
    sample_count = min(len(expected), len(actual), 44_100 * 5)
    coarse_step = 8
    coarse = min(
        range(-max_offset, max_offset + 1, coarse_step),
        key=lambda offset: offset_error(expected, actual, offset, sample_count),
    )
    lower = max(-max_offset, coarse - coarse_step)
    upper = min(max_offset, coarse + coarse_step)
    return min(
        range(lower, upper + 1),
        key=lambda offset: offset_error(expected, actual, offset, sample_count),
    )


def compare_audio(
    expected_path,
    actual_path,
    max_offset_ms,
    max_length_difference_ms,
    tolerance,
):
    expected = stereo_mono(read_stereo_wav(expected_path))
    actual = stereo_mono(read_stereo_wav(actual_path))
    if not expected or not actual:
        raise ValueError("audio streams contain no aligned samples")
    max_offset = round(44_100 * max_offset_ms / 1000)
    offset = find_best_offset(expected, actual, max_offset)
    expected_start = max(0, offset)
    actual_start = max(0, -offset)
    expected_count = len(expected) - expected_start
    actual_count = len(actual) - actual_start
    max_length_difference = round(44_100 * max_length_difference_ms / 1000)
    if abs(expected_count - actual_count) > max_length_difference:
        raise ValueError(
            "audio lengths differ after alignment: "
            f"expected={expected_count} actual={actual_count} samples, "
            f"allowed difference={max_length_difference}"
        )
    count = min(expected_count, actual_count)
    if count <= 0:
        raise ValueError("audio streams contain no aligned samples")
    window = 441
    worst = 0.0
    for start in range(0, count, window):
        length = min(window, count - start)
        if length == 0:
            continue
        squared_error = 0
        for index in range(length):
            delta = (
                expected[expected_start + start + index]
                - actual[actual_start + start + index]
            )
            squared_error += delta * delta
        worst = max(worst, math.sqrt(squared_error / length) / 65_536.0)
    if worst > tolerance:
        raise ValueError(
            f"audio differs: offset={offset} samples, worst 10 ms normalized RMS="
            f"{worst:.6f}, tolerance={tolerance:.6f}"
        )
    return offset, worst


def file_hash(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def compare_images(expected_prefix, actual_prefix):
    expected_files = {
        pathlib.Path(path).name[len(expected_prefix.name) :]: pathlib.Path(path)
        for path in glob.glob(str(expected_prefix) + "_f*_*.tga")
    }
    actual_files = {
        pathlib.Path(path).name[len(actual_prefix.name) :]: pathlib.Path(path)
        for path in glob.glob(str(actual_prefix) + "_f*_*.tga")
    }
    if expected_files.keys() != actual_files.keys():
        raise ValueError("captured image sets differ")
    for suffix, expected_path in expected_files.items():
        if file_hash(expected_path) != file_hash(actual_files[suffix]):
            raise ValueError(f"captured image differs: {suffix}")


def parse_args():
    parser = argparse.ArgumentParser(
        description="Compare two deterministic PDS regression capture prefixes."
    )
    parser.add_argument("expected", type=pathlib.Path)
    parser.add_argument("actual", type=pathlib.Path)
    parser.add_argument("--max-audio-offset-ms", type=int, default=100)
    parser.add_argument("--max-audio-length-difference-ms", type=int, default=10)
    parser.add_argument("--audio-tolerance", type=float, default=0.02)
    parser.add_argument("--compare-images", action="store_true")
    return parser.parse_args()


def main():
    args = parse_args()
    compare_manifests(
        args.expected.with_name(args.expected.name + "_frames.csv"),
        args.actual.with_name(args.actual.name + "_frames.csv"),
    )
    offset, worst = compare_audio(
        args.expected.with_name(args.expected.name + "_audio.wav"),
        args.actual.with_name(args.actual.name + "_audio.wav"),
        args.max_audio_offset_ms,
        args.max_audio_length_difference_ms,
        args.audio_tolerance,
    )
    if args.compare_images:
        compare_images(args.expected, args.actual)
    print(f"captures match; audio offset={offset} samples, worst 10 ms RMS={worst:.6f}")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, wave.Error) as error:
        print(f"regression comparison failed: {error}", file=sys.stderr)
        raise SystemExit(1)
