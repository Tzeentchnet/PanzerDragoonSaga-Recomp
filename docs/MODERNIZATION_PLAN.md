# Modernization and Accuracy Plan

This document is the current execution plan for the Panzer Dragoon Saga
reimplementation. It supersedes the initial audit checklist where the status
below is more specific.

For a plain-language explanation of each area and its role in the project,
see [Modernization Areas and Responsibilities](MODERNIZATION_AREAS.md).

## Goals and constraints

- Support Windows x64 with Visual Studio 2022 as the release path.
- Keep builds reproducible, bounded, and free of writes to the source tree.
- Update dependencies only when their compatibility gates pass.
- Prefer small hardware- or test-backed corrections over subsystem rewrites.
- Keep promising but unverified timing and rendering changes default-off.
- Never add game discs, captures, generated assets, or other disc-derived
  content to the repository.
- Do not copy implementation code from incompatible or unclear licenses.

## Current status

| Area | Status | Result |
|---|---|---|
| Build foundation | Complete | CMake 3.21 baseline, VS 2022 presets, target-scoped first-party settings, binary-tree shader generation, and fixed Shipping entry point. |
| Test foundation | Complete | Eight data-independent CTest entries plus Debug/Release ASan configurations and pull-request CI. |
| SDL | Accepted | Updated to 3.4.18. |
| Tracy | Accepted | Updated to 0.14.1. |
| Dear ImGui / ImGuizmo | Accepted | Updated to 1.92.9b / 1.10 and adapted to the new ImGuizmo layout. |
| GLM | Accepted | Updated to 0.9.9.8; the larger GLM 1.x migration remains deferred. |
| bgfx bundle | Retained | The newer bundle compiles, but runtime D3D and representative PDS capture gates could not be run without local game data. The proven bundle remains pinned. |
| Fixed-point / DIV1 | Complete | Host undefined behavior was replaced with explicit SH-2-width operations and boundary tests. |
| SCSP safety / DSP | Complete for identified defects | Reserved MADRS writes are bounded and focused DSP semantics are tested. |
| VDP2 color calculation | Complete | Ratio mode, NBG1 register selection, and saturating additive mode were corrected and exhaustively tested. |
| Replay / regression capture | Complete | Replay v2, legacy reading, connection transitions, state/audio verification, image/WAV capture, and comparison tooling are present. |
| RBG0 coefficient addressing | Experimental | The table-base correction and trace path are default-off pending deterministic PDS and hardware evidence. |
| NTSC frame cadence | Experimental | Rational 59.826 Hz raster pacing is default-off pending long gameplay routes. |
| Audio-core licensing migration | Deferred | Immediate memory-safety fixes are complete; replacing historical non-commercial aoSDK components remains a separate project. |

## Accepted implementation

### Build and test graph

- Every generated shader header has one producer and is written under the
  build directory.
- `pds_tests` is the aggregate target for all seven native test executables;
  the eighth CTest entry validates the Python comparison utility.
- Preset builds target `PDS` and `pds_tests` with at most four parallel jobs.
- CI fails if CTest discovers zero tests.
- First-party compiler options no longer leak indiscriminately into vendored
  targets.

### Deterministic regression boundary

Replay v2 uses fixed-width little-endian records rather than host structs.
Each frame can carry:

- both controller reports and explicit connection state;
- a deterministic state/video hash; and
- the mixed-audio sample position.

The capture path can emit manifests, aligned stereo WAV, final compositor
images, VDP1 images, RBG0 images, and an optional RBG0 coefficient trace.
The comparison tool rejects missing or truncated evidence rather than
returning a success-shaped fallback.

### Correctness corrections

- SH-2-width wrapping and sign extension are explicit in fixed-point and
  DIV1 operations.
- SCSP writes in `0x7C0-0x7FF` no longer index past `MADRS[31]`.
- SCSP DSP tests cover delayed RAM operations, persistent state, 26-bit
  accumulator wrapping, all 128 steps, RBP bit 6, NOFL, and EFREG state.
- The sound timing contract is named explicitly: 44,100 samples/second,
  256 M68000 cycles/sample, and an 11,289,600 Hz sound CPU clock.
- VDP2 ratio mode uses 32 as the denominator, NBG1 reads the high byte of
  CCRNA, and additive mode directly adds and saturates both inputs.

## Experimental work

### RBG0 coefficient addressing

`PDS_EXPERIMENTAL_RBG0_COEFFICIENT_ADDRESSING` is default-off. It adds the
KTAOF-derived table base to per-pixel coefficient reads with explicit
wrapping arithmetic. `--capture-rbg0-coefficients` records frame, scanline,
pixel, parameter A/B, per-pixel state, accumulator, table base, and final VRAM
address.

Promotion requires:

1. stable deterministic PDS routes that exercise the horizon effects;
2. agreement with original-hardware address or output evidence; and
3. no regressions in parameter switching, signed traversal, or VRAM wrapping.

### NTSC cadence

`PDS_EXPERIMENTAL_NTSC_FRAME_CADENCE` is default-off. It models a 59.826 Hz
raster using rational nanosecond accumulation. PDS currently presents after
two VBlanks, producing approximately 29.913 game frames/second.

Promotion requires long deterministic routes with:

- identical game-state progression across host refresh rates;
- stable VBlank-OUT and VDP1 completion ordering;
- exactly 44,100 mixed samples per emulated second; and
- no pacing-induced audio or input drift.

## Deferred work

### License-compatible audio core

The compiled SSF path still includes historical aoSDK components whose
licenses include non-commercial MAME-era terms. Replacement must use a narrow
SCSP/M68000 boundary and a permissively licensed implementation. It must pass
register/RAM traces and aligned PCM comparisons for music, SFX, DSP, loops,
pitch, FM, pause/restart, and long runs before becoming the default.

This work is intentionally not mixed into the current renderer/build changes.

### Larger migrations

The following remain out of scope until requested and independently gated:

- ImGui 1.93 rendering-semantic changes;
- GLM 1.x;
- Emscripten 6.x and restoration of the web path;
- PDS_Tool restoration and its dormant copied-header dependencies;
- wholesale adoption of another Saturn recompilation framework; and
- a full dual-SH-2/SCU/cache/bus scheduler.

## Build-efficiency policy

Local validation must have one build owner. Do not start a parent and agent
matrix at the same time or edit source files during an active build.

During development:

1. Reuse one build tree.
2. Build the exact target affected by the change.
3. Cap parallelism at four jobs.
4. Disable MSBuild node reuse.
5. Run a full configuration matrix only at a stable phase boundary.

Example:

```powershell
$env:MSBUILDDISABLENODEREUSE = "1"
cmake --build build_vs2022 --config Debug --target pds_tests --parallel 4 -- /nr:false
ctest --test-dir build_vs2022 -C Debug --output-on-failure --no-tests=error
```

## Validation state

The retained bgfx bundle builds `PDS.exe` in Debug after the dependency and
accuracy changes. All eight Debug CTest entries pass. Earlier phase gates also
passed Debug, Release, Shipping, Release_Tracy, and Release_Asan before the
latest experimental tests were added.

A fresh final multi-configuration matrix and runtime capture routes remain
future release gates; they were not repeated merely to produce documentation.
