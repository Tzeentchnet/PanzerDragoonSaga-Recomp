# Modernization Findings

## Executive summary

The project had no upstream project commits to merge, but its build graph,
validation coverage, and several dependencies had aged. The highest-value
work was therefore to make clean builds and tests trustworthy, update
compatible dependency lines, and fix narrow Saturn behavior with explicit
oracles.

The current result is a reproducible Visual Studio 2022 build with eight
asset-free tests, accepted SDL/Tracy/UI/math updates, hardened replay and
capture tooling, and tested fixes in SH-2 arithmetic, SCSP, and VDP2. Two
plausible accuracy changes remain default-off because they need game-specific
or original-hardware evidence.

## Build findings

### The apparent build loop was overlapping work

The long-running builds were not a CMake dependency cycle. The evidence
showed:

- parent and delegated validation matrices running at the same time;
- repeated broad `ALL_BUILD` invocations instead of exact targets;
- unbounded or independently bounded parallel build pools;
- source edits that invalidated active builds and triggered regeneration;
- retries after failed invocations; and
- 19 lingering MSBuild worker processes because node reuse was enabled.

Historical activity contained 68 `cmake --build` invocations, including 21
failed or retried commands. The combined behavior looked like a loop while
the tools were actually rebuilding overlapping graphs.

The adopted policy is one build owner, exact targets, at most four workers,
`MSBUILDDISABLENODEREUSE=1`, `/nr:false`, and no configuration matrix until a
phase is frozen.

### Shader generation was not a valid build graph

Generated headers were written into the source tree, duplicate rules claimed
some outputs, and clean checkouts depended on cached artifacts. Shader
generation now:

- emits configuration-specific headers under the binary tree;
- has one rule per output;
- records generated outputs on an explicit target; and
- leaves the source tree unchanged.

### The supported CMake contract was older than its dependencies

The old declared CMake minimum did not describe the actual wrapper
requirements. The project now declares CMake 3.21, uses VS 2022 presets, and
keeps first-party options and include paths target-scoped.

## Dependency findings

| Dependency | Decision | Finding |
|---|---|---|
| SDL 3.4.18 | Accepted | Same stable SDL3 line; compiled and passed project test gates. |
| Tracy 0.14.1 | Accepted | Updated independently; Release_Tracy passed its earlier build gate. |
| Dear ImGui 1.92.9b | Accepted | Compatible update; 1.93 was deliberately avoided because it carries broader rendering semantics. |
| ImGuizmo 1.10 | Accepted | Sources moved under `src/`; the build was adapted without compatibility shims. |
| GLM 0.9.9.8 | Accepted | Latest compatible 0.9.9 line; GLM 1.x remains a separate migration. |
| bgfx.cmake v1.167.9590-598 candidate | Rejected for now | Configured, built shaderc, generated all shaders, and compiled `PDS.exe`, but D3D startup and representative captures were unavailable. |
| bgfx.cmake v1.139.9155-513 | Retained | Proven bundle restored and verified with a Debug `PDS.exe` build. |
| SoLoud fork | Retained | Already carries the project-required SDL3 work; no safe benefit from changing forks. |
| aoSDK | Retained temporarily | Immediate bounds defects were fixed, but compiled SSF components include historical restrictive licenses. |
| emsdk | Deferred | The old inactive web path needs a full current-Emscripten migration rather than a pin-only update. |

Exact revisions are recorded in `DEPENDENCIES.md`.

### bgfx candidate details

The candidate was a real migration rather than a routine pin change:

- nested submodules were replaced by `FetchContent`;
- explicit bx/bimg/bgfx source overrides were needed for reproducibility;
- shader helper inclusion and shader output naming changed;
- Direct3D bytecode moved from `dx11`/`_dx11` naming to `dxbc`/`_dxbc`; and
- native-window state moved from `PlatformData` into the newer swap-chain API.

Compile compatibility was established. Runtime compatibility was not.
Accepting the candidate without D3D initialization, resize/fullscreen, ImGui,
textures, and representative PDS frame comparisons would weaken the agreed
gate, so the code and submodule were returned to the proven bundle.

## Correctness findings

### SH-2-width integer behavior

Several calculations relied on signed overflow, negative shifts, or host
integer width. Those are not reliable ways to reproduce SH-2 arithmetic.
Fixed-point and DIV1 paths now use fixed-width unsigned wrapping, explicit
sign extension, widened intermediates, and boundary tests.

### SCSP register map and DSP

The SCSP DSP map contains:

- COEF at `0x700-0x77F`;
- MADRS at `0x780-0x7BF`;
- reserved/read-zero space at `0x7C0-0x7FF`; and
- MPRO beginning at `0x800`.

The old write path treated the reserved range as more MADRS entries and could
write beyond a 32-entry array. The fix rejects those writes. Tests also cover
delayed RAM operations, accumulator width, persistent DSP state, 128-step
execution, RBP bit 6, NOFL, EFREG, and explicit sign extension.

The existing 256 M68000 cycles per 44.1 kHz output sample were correct. The
value is now represented as named timing constants, yielding an 11.2896 MHz
sound CPU clock.

### VDP2 color calculation

Ratio mode is:

```text
result = (top * (31 - ratio) + second * (ratio + 1)) / 32
```

Ratio 0 therefore retains 31/32 of the top image and 1/32 of the second;
ratio 31 selects the second image. NBG1 uses the high byte of CCRNA rather
than CCRNB. Additive mode directly adds both colors and saturates instead of
scaling the sum by a ratio. Tests cover all 32 ratios, channels, register
selection, priorities, and additive endpoints.

### RBG0 coefficient fetches

The scanline path used the KTAOF-derived table base while the per-pixel path
did not. That mismatch is a strong candidate for the visible PDS horizon
errors, but exact fractional carry and wrapping must be corroborated on
hardware. The corrected formula and CSV trace are therefore behind
`PDS_EXPERIMENTAL_RBG0_COEFFICIENT_ADDRESSING`.

### Frame cadence and completion

PDS presents after two VBlanks. A 59.826 Hz raster corresponds to
approximately 29.913 game frames/second. The experimental pacing path uses
rational nanosecond accumulation so it does not add one rounding error per
frame. Focused tests establish VBlank and VDP1 completion ordering, but long
gameplay routes are still required before this becomes default behavior.

## Replay and regression findings

The original recording path serialized host structs and did not carry enough
state to detect divergence. Replay v2 adds:

- a 64-byte fixed-width little-endian header;
- 64-byte frame records;
- controller metadata and per-frame connection flags;
- deterministic state hashes;
- mixed-audio positions;
- rejection of unknown capability/frame flags; and
- backward-compatible legacy `PDSR` playback.

The comparison utility aligns WAV streams within a bounded offset, rejects
empty or truncated data, compares named manifest fields, and optionally
compares captured images. This is a foundation for runtime gates; it does not
replace original-hardware evidence.

## Licensing finding

The copied aoSDK tree is not uniformly permissive. Its own inventory marks
the SSF M68000 and SCSP files with historical MAME/non-commercial terms, and
other unused engines include GPL components. The immediate safety fixes do
not resolve that distribution constraint.

A future audio replacement must be a clean, license-compatible integration.
GPL, non-commercial, and unlicensed emulator code may be used as behavioral
evidence where lawful, but must not be copied into the replacement.

## Validation completed

- Restored bgfx bundle: Debug `PDS.exe` builds successfully.
- Debug CTest: 8/8 pass.
- Python comparator cases: aligned audio, truncated audio, empty streams, and
  manifest mismatch all pass within the CTest entry.
- Earlier phase gates built Debug, Release, Shipping, Release_Tracy, and
  Release_Asan.
- The newer bgfx candidate configured, generated shaders, and compiled PDS,
  then was reverted because runtime gates were unavailable.

## Remaining limitations

- No local game data or disc-derived captures were read or committed.
- The newer bgfx bundle is compile-compatible but runtime-unverified.
- Experimental RBG0 and NTSC cadence paths remain off by default.
- The newest complete change set has not been repeated across the entire
  multi-configuration matrix; that is a release-boundary task, not a reason
  to repeat expensive builds after every small edit.
- The historical audio-core license migration remains unfinished.
