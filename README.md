# Azel: Reverse engineering of Panzer Dragoon Saga

> [!NOTE]
> This repository is a maintained fork of the upstream
> [yaz0r/Azel](https://github.com/yaz0r/Azel) project. It picks up that work
> with a focus on updating the project, increasing platform and dependency
> compatibility, improving accuracy and reliability, and expanding
> functionality while preserving the original reverse-engineering effort.

# Build
Only Windows x64 builds are supported right now. Install Visual Studio Build
Tools with the x64 C++ tools and Windows SDK, CMake 4.4 or newer, and Ninja
1.13 or newer. Visual Studio project generation is not required.

* Clone the repository. No submodules are required.
* Put CD1 game data in the `data` folder. The launcher accepts:
  * the extracted CD1 files;
  * a BIN/CUE pair;
  * a cooked or raw `.iso` image; or
  * a `.zip` or `.rar` archive containing any of the above.
* Run the version-neutral build wrapper from a regular command prompt:

```bat
windows-build.bat Debug
```

The wrapper locates the newest supported MSVC Build Tools installation,
configures Ninja Multi-Config, builds `PDS` and the tests, and runs CTest.
The three core modes are `Debug`, `Release`, and `Shipping`.

- `Debug` is unoptimized and enables assertions, runtime checks, STL
  hardening, and the debug CRT.
- `Release` is optimized for representative testing while retaining developer
  UI and private debug symbols.
- `Shipping` enables interprocedural optimization and compiles out developer
  windows, trace logging, and debug input paths.

Two optional Release-based diagnostics are available:

```bat
windows-build.bat Asan
windows-build.bat Tracy
```

`Asan` instruments first-party code for memory errors. `Tracy` enables the
Tracy profiling client and requires the matching external profiler. Neither
diagnostic is a distributable build.

From an x64 Developer PowerShell, the equivalent preset commands are:

```bat
cmake --preset windows-msvc-x64
cmake --build --preset windows-msvc-x64-debug
ctest --preset windows-msvc-x64-debug
```

# Configuration and launch tool

Run the Windows configuration tool from anywhere with:

```bat
configure-and-play.bat
```

The launcher does not require a separate build. It provides:

- build selection for `Debug`, `Release`, and `Shipping`;
- automatic CD1 discovery and preparation from loose files, BIN/CUE, ISO,
  ZIP, or RAR data;
- renderer selection for `auto`, Direct3D 12, Vulkan, Direct3D 11, and the
  Direct3D 11/WARP software adapter;
- window size, fullscreen, VSync, volume, and frame-limit settings;
- keyboard remapping for every Saturn digital control;
- gamepad remapping for face buttons, Start/Back, stick clicks, shoulders,
  triggers, and the D-pad, plus optional left-stick directional input;
- the existing GPU VDP1, combined dual-plane, smooth Gouraud, and direct ray
  rendering diagnostic toggles;
- `Save`, `Save & Launch`, and `Build & Launch` actions.

Settings are stored per user at:

```text
%LOCALAPPDATA%\PanzerDragoonSaga-Recomp\settings.ini
```

The checked-in [example configuration](config/pds-settings.example.ini)
documents the version 1 format and current defaults. The game loads the
per-user file automatically. An alternate file can be selected with
`--config <path>`. A command-line `--renderer` value overrides the renderer
saved in the configuration file; other existing recording, replay, and
capture arguments continue to work.

`Save & Launch` requires an existing executable under
`out\build\windows-msvc-x64\PDS\<Configuration>\PDS.exe`. `Build & Launch`
runs `windows-build.bat` for the selected core configuration first.

Before launch, the tool looks for extracted `COMMON.DAT` data or a CD1 source.
Names containing `Disc 1`, `Disk 1`, or `CD1` are preferred when several
disc sources exist. Windows `tar.exe` reads ZIP and RAR containers; encrypted
or damaged archives fail with the underlying extraction error. BIN/CUE
metadata is honored, including raw 2352-byte MODE1 and MODE2 sectors. If a
CUE references a missing `.iso` while a same-named `.bin` exists, the matching
BIN is used.

Prepared files are cached under `data\.prepared` using the source file's
metadata, so the original image or archive is never modified and later
launches do not extract it again. The first archive launch can take a few
minutes and needs temporary space for the archived image plus the extracted
CD1 files. The preparation layer validates archive paths, ISO9660 extents,
and output names before writing files.

# Tests
The first-party core tests are data-independent and do not need game discs,
SDL, bgfx, a display, or audio hardware. Build all test executables through
the aggregate `pds_tests` target or as individual projects under the `Tests`
folder. The CTest suite covers fixed-point and SH-2 DIV1 behavior,
replay/input recording, SCSP registers and DSP execution, VDP2 color
calculation, RBG0 coefficient addressing, frame timing, renderer and game
configuration, the launcher schema, game-data preparation, and the
regression-capture comparison tool.

The build presets compile both `PDS` and `pds_tests`. Matching CTest
presets enable failure output and treat discovering zero tests as an error:

```bat
cmake --preset windows-msvc-x64
cmake --build --preset windows-msvc-x64-debug
ctest --preset windows-msvc-x64-debug
```

Replace `debug` with `release` or `shipping` to exercise the other core
configurations. The optional diagnostic presets are
`windows-msvc-x64-asan` and `windows-msvc-x64-tracy`. To build only the
asset-free tests, use:

```bat
cmake --build out\build\windows-msvc-x64 --config Debug --parallel 4 --target pds_tests
ctest --test-dir out\build\windows-msvc-x64 -C Debug --output-on-failure --no-tests=error
```

The build presets cap parallelism at four jobs. The AddressSanitizer preset
verifies at compile time that test executables are instrumented and reports
that instrumentation when they run.

# Renderer selection

The renderer can be selected in `configure-and-play.bat`. The default `auto`
mode requests Direct3D 12 on Windows and permits bgfx to fall back if that
backend is unavailable. Explicit command-line selections disable backend
fallback and take precedence over the saved setting:

```bat
PDS.exe --renderer d3d12
PDS.exe --renderer vulkan
PDS.exe --renderer d3d11
PDS.exe --renderer software
```

`d3d11` remains available for diagnostics and as the host API for the
`software` mode. On Windows, `software` selects and validates the Microsoft
Basic Render Driver (WARP). Startup logs report the requested backend,
selected backend, vendor, and device. Direct3D 12 uses embedded DXIL shaders,
Vulkan uses SPIR-V, and Direct3D 11/WARP uses DXBC.

Apple Silicon and Metal are a separate future effort; they are not currently
supported build targets.

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
cmake -S . -B out\build\rbg0 -G "Ninja Multi-Config" ^
  -DPDS_EXPERIMENTAL_RBG0_COEFFICIENT_ADDRESSING=ON
cmake --build out\build\rbg0 --config Debug --parallel 4 --target PDS
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
