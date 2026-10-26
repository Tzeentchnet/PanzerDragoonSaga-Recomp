# Azel: Reverse engineering of Panzer Dragoon Saga

# Build
Only Windows builds are supported right now. Install Visual Studio 2022 (or its
Build Tools) with the x64 C++ tools and CMake 3.21 or newer.

* Clone the repository and its submodules recursively.
* Put the contents of CD1 in the `data` folder.
* Configure and build with the checked-in presets:

```bat
cmake --preset vs2022-x64
cmake --build --preset vs2022-debug
```

Build presets are also provided for `vs2022-release`, `vs2022-shipping`,
`vs2022-release-tracy`, and the Debug, Release, and Shipping AddressSanitizer
configurations.

Alternatively, run `vs2022.bat` to generate `build_vs2022\PDS.sln`, then build
the `PDS` project in Visual Studio. Pass `--pause` when launching the script
from a temporary command window if you want it to wait before closing.
`vs2026.bat` retains the equivalent optional Visual Studio 2026 generation
path.

# Tests
The first-party core tests are data-independent and do not need game discs,
SDL, bgfx, a display, or audio hardware. Build all test executables through
the aggregate `pds_tests` target or as individual projects under the `Tests`
solution folder. The eight CTest entries cover fixed-point and SH-2 DIV1
behavior, replay/input recording, SCSP registers and DSP execution, VDP2
color calculation, RBG0 coefficient addressing, frame timing, and the
regression-capture comparison tool.

The build presets compile both `PDS` and `pds_tests`. Matching CTest
presets enable failure output and treat discovering zero tests as an error:

```bat
cmake --preset vs2022-x64
cmake --build --preset vs2022-debug
ctest --preset vs2022-debug
```

Replace `debug` with `release`, `shipping`, `release-tracy`, or
`release-asan` to exercise those configurations. To build only the asset-free
tests, use:

```bat
cmake --build build_vs2022 --config Debug --parallel 4 --target pds_tests
ctest --test-dir build_vs2022 --build-config Debug --output-on-failure --no-tests=error
```

The build presets cap parallelism at four jobs. For direct MSBuild
invocations, also disable node reuse with `MSBUILDDISABLENODEREUSE=1` and
`/nr:false` so interrupted builds do not leave compiler workers resident.
AddressSanitizer configurations verify at compile time that test executables
are instrumented and report that instrumentation when they run.

# Deterministic regression captures

Input recordings use a versioned, fixed-width little-endian format. Each frame
stores both controllers, a deterministic game/video-state hash, and the SCSP
audio sample position. Legacy `PDSR` recordings remain readable.

Use `--record` or `--replay` with an optional route name and metadata:

```bat
PDS.exe --record routes\name-entry.pdsr --route name-entry --region 0 --controller-mask 1
PDS.exe --replay routes\name-entry.pdsr
```

`--capture-dir` writes a per-frame hash manifest and aligned 44.1 kHz stereo
SCSP WAV. `--capture-frame` may be repeated, while `--capture-every` requests
regular final, VDP1, and RBG0 TGA captures:

```bat
PDS.exe --replay routes\name-entry.pdsr --route name-entry ^
  --capture-dir captures\candidate --capture-frame 120 --capture-frame 240
```

Captured files and recordings can contain data derived from a local game run;
keep them outside source control. Compare two route prefixes with the
standard-library-only helper:

```bat
python tools\compare_regression.py captures\reference\name-entry captures\candidate\name-entry --compare-images
```

The comparison requires exact state/layer hashes, aligns audio within a
configurable offset, and checks 10 ms audio windows rather than assuming host
audio callback timing is identical.

# Accuracy experiments

Two incompletely hardware-validated paths are deliberately default-off:

- `PDS_EXPERIMENTAL_RBG0_COEFFICIENT_ADDRESSING` includes the KTAOF-derived
  table base in per-pixel coefficient fetches. It also enables
  `--capture-rbg0-coefficients`, which requires `--capture-dir`.
- `PDS_EXPERIMENTAL_NTSC_FRAME_CADENCE` enables drift-free pacing at the
  approximately 59.826 Hz NTSC raster cadence. At the current two-VBlank game
  frame boundary this is approximately 29.913 presented game frames per
  second. Select it at runtime with `--experimental-ntsc-cadence`.

Enable an experiment only in a separate build tree, for example:

```bat
cmake -S . -B build_rbg0 -G "Visual Studio 17 2022" -A x64 ^
  -DPDS_EXPERIMENTAL_RBG0_COEFFICIENT_ADDRESSING=ON
cmake --build build_rbg0 --config Debug --parallel 4 --target PDS
```

These paths are for capture and comparison. They should not become defaults
without deterministic PDS evidence and, for RBG0 addressing, original-hardware
corroboration.

Project status and rationale are recorded in:

- [Modernization plan](docs/MODERNIZATION_PLAN.md)
- [Modernization areas and responsibilities](docs/MODERNIZATION_AREAS.md)
- [Findings](docs/FINDINGS.md)
- [Saturn recomp findings](docs/SATURN_RECOMP_FINDINGS.md)
- [Dependency inventory](docs/DEPENDENCIES.md)

# Status
* Still very early, but the beginning of the game is semi playable (with bugs) until the Arachnoth battle

# Screenshots
![image](https://github.com/user-attachments/assets/71f6a246-24cc-4d64-9673-53b9e4c3e4df)
![image](https://github.com/user-attachments/assets/adbfa9b0-bb1a-483a-8b15-4c7fdc198131)
![image](https://github.com/user-attachments/assets/864375eb-976a-403f-8d45-3dad03f83491)
![image](https://github.com/user-attachments/assets/4ec9797e-8bc2-4a00-8584-412fc0089c50)
