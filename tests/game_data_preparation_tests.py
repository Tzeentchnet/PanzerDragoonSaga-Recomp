import argparse
import struct
import subprocess
import tempfile
import unittest
import zipfile
from pathlib import Path


SECTOR_SIZE = 2048


def both_endian_16(value: int) -> bytes:
    return struct.pack("<H", value) + struct.pack(">H", value)


def both_endian_32(value: int) -> bytes:
    return struct.pack("<I", value) + struct.pack(">I", value)


def directory_record(
    identifier: bytes,
    extent: int,
    size: int,
    *,
    is_directory: bool,
) -> bytes:
    padding = b"\0" if len(identifier) % 2 == 0 else b""
    length = 33 + len(identifier) + len(padding)
    record = bytearray(length)
    record[0] = length
    record[2:10] = both_endian_32(extent)
    record[10:18] = both_endian_32(size)
    record[18:25] = bytes((126, 1, 1, 0, 0, 0, 0))
    record[25] = 2 if is_directory else 0
    record[28:32] = both_endian_16(1)
    record[32] = len(identifier)
    record[33 : 33 + len(identifier)] = identifier
    return bytes(record)


def create_iso(
    path: Path,
    common_data: bytes = b"test COMMON.DAT",
    *,
    declared_extra_sectors: int = 0,
) -> bytes:
    root_sector = 20
    common_sector = 21
    volume_sectors = 22
    image = bytearray(volume_sectors * SECTOR_SIZE)

    pvd = memoryview(image)[16 * SECTOR_SIZE : 17 * SECTOR_SIZE]
    pvd[0] = 1
    pvd[1:6] = b"CD001"
    pvd[6] = 1
    pvd[8:40] = b"PDS TEST".ljust(32, b" ")
    pvd[40:72] = b"PANZER DRAGOON SAGA".ljust(32, b" ")
    pvd[80:88] = both_endian_32(volume_sectors + declared_extra_sectors)
    pvd[120:124] = both_endian_16(1)
    pvd[124:128] = both_endian_16(1)
    pvd[128:132] = both_endian_16(SECTOR_SIZE)
    pvd[132:140] = both_endian_32(10)
    pvd[140:144] = struct.pack("<I", 18)
    pvd[148:152] = struct.pack(">I", 19)
    pvd[156:190] = directory_record(
        b"\0",
        root_sector,
        SECTOR_SIZE,
        is_directory=True,
    )
    pvd[190:702] = b" " * 512
    pvd[702:813] = b" " * 111
    pvd[813:830] = b"2026010100000000\0"
    pvd[830:847] = b"2026010100000000\0"
    pvd[847:864] = b"0000000000000000\0"
    pvd[864:881] = b"2026010100000000\0"
    pvd[881] = 1

    terminator = memoryview(image)[17 * SECTOR_SIZE : 18 * SECTOR_SIZE]
    terminator[0] = 255
    terminator[1:6] = b"CD001"
    terminator[6] = 1

    little_path_table = memoryview(image)[18 * SECTOR_SIZE : 19 * SECTOR_SIZE]
    little_path_table[0:10] = (
        b"\x01\x00" + struct.pack("<I", root_sector) + struct.pack("<H", 1) + b"\0\0"
    )
    big_path_table = memoryview(image)[19 * SECTOR_SIZE : 20 * SECTOR_SIZE]
    big_path_table[0:10] = (
        b"\x01\x00" + struct.pack(">I", root_sector) + struct.pack(">H", 1) + b"\0\0"
    )

    root = memoryview(image)[root_sector * SECTOR_SIZE : (root_sector + 1) * SECTOR_SIZE]
    records = (
        directory_record(
            b"\0",
            root_sector,
            SECTOR_SIZE,
            is_directory=True,
        ),
        directory_record(
            b"\x01",
            root_sector,
            SECTOR_SIZE,
            is_directory=True,
        ),
        directory_record(
            b"COMMON.DAT;1",
            common_sector,
            len(common_data),
            is_directory=False,
        ),
    )
    offset = 0
    for record in records:
        root[offset : offset + len(record)] = record
        offset += len(record)

    image[
        common_sector * SECTOR_SIZE : common_sector * SECTOR_SIZE + len(common_data)
    ] = common_data
    path.write_bytes(image)
    return bytes(image)


def create_raw_bin(path: Path, cooked_iso: bytes) -> None:
    with path.open("wb") as output:
        for offset in range(0, len(cooked_iso), SECTOR_SIZE):
            output.write(b"\0" * 16)
            output.write(cooked_iso[offset : offset + SECTOR_SIZE])
            output.write(b"\0" * (2352 - 16 - SECTOR_SIZE))


class GameDataPreparationTests(unittest.TestCase):
    powershell: str
    script: Path

    def run_prepare(
        self,
        data_directory: Path,
        source: Path | None = None,
        *,
        expect_success: bool = True,
    ) -> subprocess.CompletedProcess[str]:
        command = [
            self.powershell,
            "-NoProfile",
            "-ExecutionPolicy",
            "Bypass",
            "-File",
            str(self.script),
            "-DataDirectory",
            str(data_directory),
        ]
        if source is not None:
            command.extend(("-SourcePath", str(source)))

        result = subprocess.run(
            command,
            check=False,
            capture_output=True,
            text=True,
            encoding="utf-8",
        )
        if expect_success and result.returncode != 0:
            self.fail(
                f"preparation failed ({result.returncode})\n"
                f"stdout:\n{result.stdout}\n"
                f"stderr:\n{result.stderr}"
            )
        if not expect_success and result.returncode == 0:
            self.fail(f"preparation unexpectedly succeeded:\n{result.stdout}")
        return result

    def prepared_path(self, result: subprocess.CompletedProcess[str]) -> Path:
        output_lines = [line.strip() for line in result.stdout.splitlines() if line.strip()]
        self.assertEqual(len(output_lines), 1, result.stdout)
        return Path(output_lines[0])

    def test_loose_data_is_used_without_copying(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            data_directory = Path(temporary) / "data"
            data_directory.mkdir()
            (data_directory / "COMMON.DAT").write_bytes(b"loose")

            prepared = self.prepared_path(self.run_prepare(data_directory))

            self.assertEqual(prepared.resolve(), data_directory.resolve())
            self.assertFalse((data_directory / ".prepared").exists())

    def test_cooked_iso_is_extracted_and_cached(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            data_directory = Path(temporary) / "data"
            data_directory.mkdir()
            iso_path = data_directory / "Game Disc 1.iso"
            create_iso(iso_path, b"cooked")

            first = self.prepared_path(
                self.run_prepare(data_directory, source=iso_path)
            )
            second = self.prepared_path(
                self.run_prepare(data_directory, source=iso_path)
            )
            iso_path.unlink()
            cache_only = self.prepared_path(self.run_prepare(data_directory))

            self.assertEqual(first.resolve(), second.resolve())
            self.assertEqual(first.resolve(), cache_only.resolve())
            self.assertEqual((first / "COMMON.DAT").read_bytes(), b"cooked")
            self.assertTrue((first / ".pds-data-source.json").is_file())

    def test_cue_resolves_mislabeled_raw_bin(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            data_directory = Path(temporary) / "data"
            data_directory.mkdir()
            cooked_path = data_directory / "source.iso"
            cooked = create_iso(
                cooked_path,
                b"raw cue",
                declared_extra_sectors=150,
            )
            cooked_path.unlink()

            bin_path = data_directory / "Game Disk 1.bin"
            create_raw_bin(bin_path, cooked)
            cue_path = data_directory / "Game Disk 1.cue"
            cue_path.write_text(
                'FILE "Game Disk 1.iso" BINARY\n'
                "  TRACK 01 MODE1/2352\n"
                "    INDEX 01 00:00:00\n",
                encoding="ascii",
            )

            prepared = self.prepared_path(
                self.run_prepare(data_directory, source=cue_path)
            )

            self.assertEqual((prepared / "COMMON.DAT").read_bytes(), b"raw cue")

    def test_raw_bin_layout_is_detected_without_cue(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            data_directory = Path(temporary) / "data"
            data_directory.mkdir()
            cooked_path = data_directory / "source.iso"
            cooked = create_iso(cooked_path, b"raw bin")
            cooked_path.unlink()
            bin_path = data_directory / "Game Disc 1.bin"
            create_raw_bin(bin_path, cooked)

            prepared = self.prepared_path(
                self.run_prepare(data_directory, source=bin_path)
            )

            self.assertEqual((prepared / "COMMON.DAT").read_bytes(), b"raw bin")

    def test_zip_wrapped_bin_and_cue_are_prepared(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            data_directory = Path(temporary) / "data"
            data_directory.mkdir()
            fixture_directory = Path(temporary) / "fixture"
            fixture_directory.mkdir()
            cooked_path = fixture_directory / "source.iso"
            cooked = create_iso(cooked_path, b"zip")
            cooked_path.unlink()
            bin_path = fixture_directory / "Game Disc 1.bin"
            create_raw_bin(bin_path, cooked)
            cue_text = (
                'FILE "Game Disc 1.bin" BINARY\n'
                "  TRACK 01 MODE1/2352\n"
                "    INDEX 01 00:00:00\n"
            )

            zip_path = data_directory / "Game Disc 1.zip"
            with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as archive:
                archive.write(bin_path, "disc/Game Disc 1.bin")
                archive.writestr("disc/Game Disc 1.cue", cue_text)

            prepared = self.prepared_path(self.run_prepare(data_directory))

            self.assertEqual((prepared / "COMMON.DAT").read_bytes(), b"zip")

    def test_unsafe_archive_path_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            temporary_path = Path(temporary)
            data_directory = temporary_path / "data"
            data_directory.mkdir()
            zip_path = data_directory / "Game Disc 1.zip"
            with zipfile.ZipFile(zip_path, "w") as archive:
                archive.writestr("../escaped.txt", "unsafe")

            result = self.run_prepare(
                data_directory,
                source=zip_path,
                expect_success=False,
            )

            self.assertIn("unsafe path", result.stderr.lower())
            self.assertFalse((temporary_path / "escaped.txt").exists())


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--powershell", required=True)
    parser.add_argument("--script", type=Path, required=True)
    arguments, unittest_arguments = parser.parse_known_args()

    GameDataPreparationTests.powershell = arguments.powershell
    GameDataPreparationTests.script = arguments.script.resolve()
    program = unittest.main(argv=[__file__, *unittest_arguments], exit=False)
    return 0 if program.result.wasSuccessful() else 1


if __name__ == "__main__":
    raise SystemExit(main())
