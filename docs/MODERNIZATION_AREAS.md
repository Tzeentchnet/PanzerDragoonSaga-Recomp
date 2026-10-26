# Modernization Areas and Responsibilities

This document explains every area listed in the
[modernization plan](MODERNIZATION_PLAN.md): what the area is, what it does in
this project, why it matters, and its current status.

## How the areas fit together

The major runtime paths are:

```text
SDL input -> translated PDS input/SMPC state -> game logic
                                                |
                                                +-> VDP1/VDP2 state -> bgfx -> SDL window
                                                |
                                                +-> SCSP/DSP state -> SoLoud -> SDL audio

Replay and regression capture observe input, game state, video, and audio.
CMake, presets, tests, sanitizers, and CI build and validate every path.
```

The areas are intentionally separated so that a dependency update, a Saturn
accuracy correction, and a validation-tool change can be tested
independently.

## Status meanings

| Status | Meaning |
|---|---|
| **Complete** | The planned correction or infrastructure work is implemented and covered by the available focused validation. |
| **Accepted** | A dependency update passed its compatibility gates and is now the selected revision. |
| **Retained** | The existing dependency was kept because a proposed update did not pass every required acceptance gate. |
| **Experimental** | The implementation exists and is tested in isolation, but remains default-off until stronger PDS or hardware evidence is available. |
| **Deferred** | The work is understood but intentionally left for a separate project or release phase. |

## Area summary

| Area | What it does | Current status |
|---|---|---|
| Build foundation | Turns the source tree and pinned dependencies into reproducible Visual Studio projects and executable/test outputs. | Complete |
| Test foundation | Runs asset-free semantic checks, sanitizers, and CI gates so regressions fail visibly. | Complete |
| SDL | Provides the native window, keyboard/gamepad input, timing services, and host audio/device integration. | Accepted: 3.4.18 |
| Tracy | Profiles CPU work and timing in the dedicated profiling configuration. | Accepted: 0.14.1 |
| Dear ImGui / ImGuizmo | Implements developer/debug interfaces and interactive transformation gizmos. | Accepted: 1.92.9b / 1.10 |
| GLM | Supplies host-side vector, matrix, quaternion, and camera math. | Accepted: 0.9.9.8 |
| bgfx bundle | Compiles shaders and presents the translated Saturn renderer through the host graphics API. | Proven bundle retained |
| Fixed-point / DIV1 | Reproduces SH-2-width arithmetic and division-step behavior consistently on the host CPU. | Complete |
| SCSP safety / DSP | Models Saturn sound registers, DSP execution, sound RAM access, and sound-CPU timing. | Complete for identified defects |
| VDP2 color calculation | Combines Saturn background layers with hardware-style ratios, priorities, and additive color behavior. | Complete |
| Replay / regression capture | Replays deterministic input and compares state, video, and audio at game-owned boundaries. | Complete |
| RBG0 coefficient addressing | Calculates rotation-background coefficient-table addresses used by PDS horizon and perspective effects. | Experimental |
| NTSC frame cadence | Separates Saturn raster progression from host presentation and paces the emulated timeline without drift. | Experimental |
| Audio-core licensing migration | Replaces restrictive historical SCSP/M68000 components with a permissively licensed equivalent. | Deferred |

## Build foundation

### What it is

The build foundation is the CMake layer, Visual Studio presets, generation
scripts, target definitions, and dependency wiring used to produce `PDS.exe`
and the test executables.

### What it does

- Selects Windows x64 and Visual Studio 2022 as the supported release path.
- Builds first-party code and exact pinned dependency revisions.
- Generates embedded shader headers under the build directory.
- Applies configuration-specific definitions for Debug, Release, Shipping,
  Tracy, and AddressSanitizer builds.
- Keeps first-party compiler and linker settings from leaking into vendored
  targets.
- Exposes exact `PDS` and `pds_tests` targets so routine builds do not compile
  the entire solution.

### Why it matters

A recompilation project cannot distinguish a code regression from a stale or
duplicated generated file if the build graph is not deterministic. The
foundation makes a clean checkout and an incremental build describe the same
inputs and outputs.

### Main touchpoints

- `CMakeLists.txt`
- `CMakePresets.json`
- `AzelLib/CMakeLists.txt`
- `PDS/CMakeLists.txt`
- `tests/CMakeLists.txt`
- `cmake/`
- `vs2022.bat`

## Test foundation

### What it is

The test foundation is the collection of native unit-style executables,
Python comparison tests, CTest registration, sanitizer configurations, and
GitHub Actions gates.

### What it does

- Runs eight data-independent CTest entries.
- Validates fixed-point and DIV1 behavior.
- Checks replay serialization and input transitions.
- Exercises SCSP register bounds and DSP microprograms.
- Exhausts VDP2 blend ratios and checks RBG0 address arithmetic.
- Checks rational frame pacing and completion ordering.
- Verifies regression manifests, images, and aligned audio comparisons.
- Fails CI when no tests are discovered.
- Provides MSVC AddressSanitizer configurations for memory defects.

### Why it matters

Most Saturn accuracy bugs are bit- or ordering-sensitive. Fast, asset-free
tests let developers verify those rules without launching the game or
depending on a particular host refresh rate.

## SDL

### What it is

SDL is the host-platform abstraction used by the Windows executable.

### What it does here

- Creates and manages the application window.
- Reports keyboard and gamepad state.
- Supplies controller connection, button, stick, and trigger information.
- Provides high-resolution monotonic time and precise delay functions.
- Supports the host audio/device path used by SoLoud.
- Provides the native window handle passed to bgfx.

### What it does not do

SDL does not emulate Saturn input, VDP graphics, or SCSP audio semantics. It
only supplies host events and devices; the project translates them into
Saturn-facing state.

## Tracy

### What it is

Tracy is an instrumentation profiler enabled by the `Release_Tracy`
configuration.

### What it does here

- Records instrumented CPU zones and frame timing.
- Tracks selected allocations when Tracy is enabled.
- Helps locate host-side stalls and unexpectedly expensive translated paths.
- Lets developers distinguish emulation work from rendering, capture, or host
  waiting.

Tracy is not part of normal Shipping behavior and does not change Saturn
timing semantics.

## Dear ImGui and ImGuizmo

### What they are

Dear ImGui provides immediate-mode developer interfaces. ImGuizmo adds
interactive translate/rotate/scale controls rendered through ImGui.

### What they do here

- Display debug windows and internal state.
- Host game-status, flag, field, battle, and rendering diagnostics.
- Provide developer controls without changing the game-data format.
- Render through the project's custom bgfx ImGui integration.

These tools are primarily for development. They are not Saturn UI emulation
and should not become a source of gameplay state unless an explicit debug
action requests it.

## GLM

### What it is

GLM is a header-only graphics mathematics library.

### What it does here

- Represents host-side vectors, matrices, and quaternions.
- Supports camera and rendering transformations.
- Supplies predictable graphics-math helpers used around the translated
  renderer.

GLM does not replace the game's fixed-point Saturn math. SH-2-visible
calculations remain in explicit fixed-width project code so a host library
update cannot silently change emulated arithmetic.

## bgfx bundle

### What it is

The bgfx bundle consists of `bgfx.cmake`, bgfx, bimg, and bx at a lockstep set
of revisions.

### What it does here

- Compiles project shaders into embedded platform bytecode.
- Creates GPU programs, textures, frame buffers, and render targets.
- Submits translated VDP1 and VDP2 drawing work.
- Presents the final compositor output through the host Direct3D backend.
- Renders the custom ImGui integration.

### Why the existing bundle was retained

The newer candidate could configure, build shaderc, generate shaders, and
compile `PDS.exe`. It also changed dependency acquisition, shader output
naming, and native-window/swap-chain APIs. Compile success did not prove D3D
startup, resize, textures, ImGui, or representative gameplay output.

The proven bundle was therefore restored. A future update must pass runtime
and deterministic capture gates as one lockstep migration.

## Fixed-point and DIV1

### What they are

Fixed-point values represent fractional Saturn-era calculations with integer
bit patterns. DIV1 is the SH-2's one-step division instruction, whose output
depends on operand bits and the Q, M, and T status flags.

### What they do here

- Reproduce SH-2-width wrapping rather than native C++ overflow.
- Make signed interpretation and sign extension explicit.
- Preserve truncation behavior in translated geometry and game calculations.
- Reproduce DIV1 flag transitions one instruction step at a time.
- Avoid compiler-dependent negative shifts and host-width assumptions.

### Why they matter

A mathematically close floating-point or host-integer answer can still send
translated game logic down a different branch. Bit-identical arithmetic is a
core recompilation contract.

## SCSP safety and DSP

### What it is

The SCSP is the Saturn's sound processor. Its DSP performs programmable
effects and accesses sound RAM while a Motorola 68000-class sound CPU drives
the broader audio system.

### What it does here

- Stores and interprets SCSP registers.
- Executes 128-step DSP programs.
- Performs delayed sound-RAM reads and writes.
- Maintains accumulators, effect registers, and persistent DSP state.
- Produces 44.1 kHz stereo samples for the host audio path.
- Advances the sound CPU by 256 cycles per output sample.

### Safety correction

The register window contains 32 MADRS entries at `0x780-0x7BF`.
`0x7C0-0x7FF` is reserved and must not index beyond `MADRS[31]`. The corrected
write path protects this boundary, while tests cover accumulator width,
sign-extension, RBP, NOFL, EFREG, delayed memory operations, and all 128
steps.

## VDP2 color calculation

### What it is

VDP2 is the Saturn video processor responsible for background planes,
rotation backgrounds, priorities, color calculation, and display
composition.

### What it does here

- Selects visible background layers by priority.
- Applies five-bit color-calculation ratios.
- Chooses each layer's ratio register.
- Performs saturating additive color calculation.
- Composites VDP2 backgrounds with VDP1 output.

### Corrected behavior

Ratio mode uses:

```text
(top * (31 - ratio) + second * (ratio + 1)) / 32
```

NBG1 reads the high byte of CCRNA, and additive mode adds both inputs directly
with saturation. The tests cover every ratio and supported layer/channel
combination.

## Replay and regression capture

### What it is

This area provides deterministic input playback and observable evidence for
game-state, rendering, and audio comparisons.

### What it does here

- Records fixed-width little-endian replay v2 files.
- Preserves legacy `PDSR` playback.
- Stores both controllers and explicit connection transitions.
- Stores per-frame state hashes and mixed-audio positions.
- Rejects unknown metadata and frame flags.
- Captures manifests, stereo WAV, and selected final/VDP1/RBG0 images.
- Compares manifests by named field.
- Aligns audio within a bounded search window.
- Rejects empty or truncated captures rather than treating them as matches.

### Why it matters

Visual inspection alone cannot reveal the first divergent frame or distinguish
a host callback shift from emulated audio drift. Replay and capture create a
repeatable boundary for accepting renderer, timing, input, and audio changes.

## RBG0 coefficient addressing

### What it is

RBG0 is VDP2's rotation-background layer. Coefficient tables can alter the
transformation per scanline or pixel, enabling perspective-like terrain and
horizon effects used by PDS.

### What the experiment does

- Adds the KTAOF-derived table base to per-pixel coefficient addresses.
- Makes accumulator addition and VRAM wrapping explicit.
- Tracks parameter A/B selection.
- Can emit a CSV row for each relevant coefficient fetch.

### Why it remains experimental

The old per-pixel path was inconsistent with the scanline path, but the exact
fractional carry, signed traversal, table switching, and wrapping behavior
still need deterministic PDS and original-hardware corroboration. The
experiment therefore remains default-off.

## NTSC frame cadence

### What it is

This area models Saturn raster/VBlank progression separately from host
presentation timing.

### What the experiment does

- Represents the NTSC raster rate as approximately 59.826 Hz.
- Models the current PDS game-frame boundary at two VBlanks, approximately
  29.913 game frames per second.
- Uses rational nanosecond accumulation to avoid per-frame rounding drift.
- Makes VBlank and VDP1 completion ordering explicit.
- Uses SDL's monotonic nanosecond clock and precise delay for host waiting.

### Why it remains experimental

Focused arithmetic and ordering tests pass, but promotion requires long PDS
routes to demonstrate identical state progression, correct audio sample
counts, and host-refresh-independent behavior.

## Audio-core licensing migration

### What it is

This is the planned replacement of historical aoSDK SSF components whose
license inventory includes non-commercial MAME-era terms.

### What it will do

- Place the current SCSP/M68000 behavior behind a narrow interface.
- Evaluate a permissively licensed replacement independently.
- Compare register and RAM traces.
- Compare aligned PCM for music, SFX, DSP effects, loops, pitch, FM, pause,
  restart, and long runs.
- Remove restrictive compiled components only after behavioral parity.

### What has already been done

Immediate SCSP bounds and DSP correctness defects have been fixed and tested.
Those safety corrections do not change the historical components' license,
so the complete migration remains deferred.

## Deferred larger migrations

The plan also names several changes that are not current modernization areas:

| Deferred item | What it would affect | Why it is separate |
|---|---|---|
| ImGui 1.93 | Debug UI rendering and backend semantics | Broader behavior change than the accepted 1.92 patch update. |
| GLM 1.x | Host graphics math APIs and results | Major-version migration requiring camera/matrix capture comparison. |
| Emscripten 6.x | Web build, browser platform code, and deployment | The existing web path is old enough to require end-to-end restoration. |
| PDS_Tool restoration | Dormant tooling and copied header dependencies | Support status must be decided before updating its dependencies. |
| Another recomp framework | Project architecture and translated-code model | A wholesale migration would replace rather than incrementally improve current work. |
| Full SH-2/SCU/cache/bus scheduler | Machine timing and dual-CPU interaction | High complexity; should be added only for behavior PDS demonstrably observes. |

## Where to read next

- [Modernization plan](MODERNIZATION_PLAN.md) for status and promotion gates.
- [Modernization findings](FINDINGS.md) for the evidence behind decisions.
- [Saturn recomp findings](SATURN_RECOMP_FINDINGS.md) for accuracy methodology.
- [Dependency inventory](DEPENDENCIES.md) for exact revisions and licensing
  notes.
