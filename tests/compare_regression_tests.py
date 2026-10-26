import csv
import importlib.util
import pathlib
import tempfile
import unittest
import wave


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
MODULE_PATH = PROJECT_ROOT / "tools" / "compare_regression.py"
SPEC = importlib.util.spec_from_file_location("compare_regression", MODULE_PATH)
compare_regression = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(compare_regression)


def write_wav(path, samples):
    with wave.open(str(path), "wb") as stream:
        stream.setnchannels(2)
        stream.setsampwidth(2)
        stream.setframerate(44_100)
        frames = bytearray()
        for sample in samples:
            frames.extend(int(sample).to_bytes(2, "little", signed=True))
            frames.extend(int(sample).to_bytes(2, "little", signed=True))
        stream.writeframes(frames)


def write_manifest(path, rows):
    with path.open("w", newline="", encoding="ascii") as stream:
        writer = csv.DictWriter(
            stream,
            fieldnames=["frame", "game_state", "vdp1", "vdp2", "audio_sample"],
        )
        writer.writeheader()
        writer.writerows(rows)


class CompareRegressionTests(unittest.TestCase):
    def test_audio_alignment_accepts_leading_offset(self):
        with tempfile.TemporaryDirectory() as directory:
            directory = pathlib.Path(directory)
            expected_path = directory / "expected.wav"
            actual_path = directory / "actual.wav"
            samples = [((index * 7919) % 60000) - 30000 for index in range(5000)]
            write_wav(expected_path, samples)
            write_wav(actual_path, [0] * 25 + samples)

            offset, worst = compare_regression.compare_audio(
                expected_path,
                actual_path,
                max_offset_ms=10,
                max_length_difference_ms=1,
                tolerance=0.0,
            )

            self.assertEqual(offset, -25)
            self.assertEqual(worst, 0.0)

    def test_audio_comparison_rejects_truncated_capture(self):
        with tempfile.TemporaryDirectory() as directory:
            directory = pathlib.Path(directory)
            expected_path = directory / "expected.wav"
            actual_path = directory / "actual.wav"
            samples = [((index * 7919) % 60000) - 30000 for index in range(5000)]
            write_wav(expected_path, samples)
            write_wav(actual_path, samples[:-1000])

            with self.assertRaisesRegex(ValueError, "audio lengths differ"):
                compare_regression.compare_audio(
                    expected_path,
                    actual_path,
                    max_offset_ms=10,
                    max_length_difference_ms=1,
                    tolerance=0.0,
                )

    def test_audio_comparison_rejects_empty_capture(self):
        with tempfile.TemporaryDirectory() as directory:
            directory = pathlib.Path(directory)
            expected_path = directory / "expected.wav"
            actual_path = directory / "actual.wav"
            write_wav(expected_path, [])
            write_wav(actual_path, [])

            with self.assertRaisesRegex(ValueError, "no aligned samples"):
                compare_regression.compare_audio(
                    expected_path,
                    actual_path,
                    max_offset_ms=10,
                    max_length_difference_ms=1,
                    tolerance=0.0,
                )

    def test_manifest_comparison_rejects_hash_mismatch(self):
        with tempfile.TemporaryDirectory() as directory:
            directory = pathlib.Path(directory)
            expected_path = directory / "expected.csv"
            actual_path = directory / "actual.csv"
            row = {
                "frame": 7,
                "game_state": "01",
                "vdp1": "02",
                "vdp2": "03",
                "audio_sample": 100,
            }
            write_manifest(expected_path, [row])
            write_manifest(actual_path, [{**row, "vdp2": "04"}])

            with self.assertRaisesRegex(ValueError, "frame 7 differs"):
                compare_regression.compare_manifests(expected_path, actual_path)


if __name__ == "__main__":
    unittest.main()
