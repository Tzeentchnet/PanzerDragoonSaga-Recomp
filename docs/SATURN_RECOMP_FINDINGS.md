# Saturn Recomp Process Findings

## Purpose

This document records the Saturn-specific methods and behavioral findings
that are useful to this project. It deliberately separates observable
hardware behavior from implementation code found in emulators or other
recompilation projects.

The project is best treated as a native translation with explicit Saturn
contracts at its subsystem boundaries. Importing a complete emulator
scheduler would be both architecturally disruptive and unnecessary for the
current game-specific goals.

## Evidence hierarchy

Use evidence in this order:

1. Official Sega hardware documentation and unambiguous bit-level formulas.
2. Reproducible original-hardware traces or captures.
3. Deterministic PDS routes with state, layer, and audio evidence.
4. Agreement among independent, actively maintained Saturn implementations.
5. A single emulator implementation or historical project behavior.

Levels 4 and 5 are leads, not sufficient proof for a PDS default change.
When levels 1-3 are unavailable, keep the behavior behind a default-off
experiment and expose enough trace data to test it.

## Source and licensing discipline

Recent recompilation work and maintained Saturn implementations are valuable
for identifying missing edge cases. They are not automatically safe source
dependencies.

- Check the license of the exact file and revision, not only the repository
  landing page.
- Treat GPL, historical non-commercial MAME, and unlicensed implementations
  as behavioral oracles only.
- Re-derive formulas from documentation and independently authored tests.
- Record provenance for any permissively licensed code that is actually
  incorporated.
- Never mix disc-derived code, data, images, or audio with redistributable
  test fixtures.

This pass consulted recent Saturn recompilation work and maintained emulator,
FPGA, and documentation projects to cross-check behaviors. No incompatible
implementation was copied.

## Recompilation workflow that scales

### 1. Define the machine-width contract

Native C++ must not rely on the host compiler to accidentally reproduce SH-2
behavior. For each translated operation, state:

- input width;
- signedness;
- where wrapping occurs;
- whether a shift is logical or arithmetic;
- how sign extension occurs; and
- which condition bits are inputs and outputs.

Use unsigned fixed-width operations for wrapping, `std::bit_cast` where a
signed interpretation is required, and widened intermediates where the
hardware operation has a larger product.

The DIV1 instruction is a useful template: test every Q/M/T transition,
signed extrema, zero, carry/borrow boundaries, and one-step state changes
rather than validating only final division results.

### 2. Build data-independent micro-oracles

Subsystem tests should run without game assets, graphics, audio hardware, or
host timing. This pass added focused oracles for:

- fixed-point conversion and arithmetic;
- SH-2 DIV1;
- SCSP register boundaries and DSP microprograms;
- VDP2 color calculation for every ratio;
- RBG0 coefficient-address arithmetic; and
- frame cadence and completion ordering.

These tests isolate semantics. They cannot prove that translated game code
sets every register at the right time, so they complement rather than replace
game routes.

### 3. Capture game-owned boundaries

Host callbacks are poor deterministic boundaries. Capture at the point where
PDS considers a frame complete and include:

- a selected game-state hash;
- relevant VDP1/VDP2 state;
- final, VDP1, and RBG0 images where needed;
- mixed-audio sample position and aligned PCM; and
- explicit controller connection/report state.

Compare the same game frame or emulated tick, not the nearest host refresh or
audio callback.

### 4. Promote in stages

Use three states:

- **Accepted**: documentation or hardware evidence plus deterministic tests.
- **Experimental**: credible model and focused tests, but missing a PDS or
  hardware gate.
- **Oracle only**: an observed behavior worth investigating, with no project
  implementation yet.

Do not let a broad "accuracy improvement" label bypass these states.

## SH-2 and translated integer behavior

The largest portability risk in translated SH-2 code is C++ undefined or
implementation-defined behavior:

- signed overflow;
- left-shifting negative values;
- right-shifting negative signed values;
- implicit narrowing;
- host `long` width; and
- division overflow such as `INT_MIN / -1`.

The adopted pattern makes wrap points explicit and tests bit patterns rather
than relying on numeric intuition. A full dual-CPU cache/bus model is not
required unless PDS is shown to observe those effects; maintained emulators
remain useful for constructing narrow ordering oracles.

## SCSP and sound CPU

### Register map

The DSP region is not a single contiguous array:

| Range | Meaning |
|---|---|
| `0x700-0x77F` | COEF |
| `0x780-0x7BF` | 32 MADRS entries |
| `0x7C0-0x7FF` | Reserved/read-zero |
| `0x800+` | MPRO |

Reserved writes must not index MADRS. Exhaustive register-window tests are
cheap and should be run under ASan when the configuration is available.

### DSP semantics worth testing

Small programs are more useful than long music comparisons when isolating:

- delayed memory reads and writes;
- 26-bit accumulator wrapping;
- sign extension at field boundaries;
- persistence across sample steps;
- all 128 instruction steps;
- RBP bit 6;
- NOFL behavior; and
- EFREG persistence.

Audio comparison then verifies the integrated result with bounded alignment.

### Clock relation

The current project contract is exact and should remain named:

```text
44,100 samples/second * 256 M68000 cycles/sample = 11,289,600 cycles/second
```

Replacing this with a fractional accumulator would be less accurate for the
existing model.

## VDP2 color calculation

For supported ratio-mode blending, the Saturn five-bit ratio gives:

```text
topWeight    = 31 - ratio
secondWeight = ratio + 1
result       = (top * topWeight + second * secondWeight) / 32
```

Important consequences:

- ratio 0 is not 100% top; it includes 1/32 of the second image;
- ratio 31 selects the second image;
- division by 31 is not equivalent; and
- additive mode is direct saturating addition, not a ratio-scaled sum.

Register selection is layer-specific. In this project NBG0 uses CCRNA low
bits, NBG1 uses CCRNA high bits, and RBG0 uses CCRR low bits. Exhaust all 32
ratio values and channel endpoints in tests.

## RBG0 coefficient addressing

PDS-specific horizon effects use coefficient-table behavior that is easy to
approximate incorrectly. The project had an internal inconsistency:

- per-scanline fetches included the KTAOF-derived table base;
- per-pixel fetches did not.

The experiment now computes the per-pixel address from the fixed accumulator
plus table base with explicit wrapping. It also logs enough information to
compare models without inferring addresses from the final image.

Open questions before promotion:

- exact fractional carry behavior;
- negative traversal and wrapping;
- parameter A/B switching from coefficient MSBs;
- nonzero parameter-B KTAOF; and
- agreement with original hardware on PDS scenes.

The focused arithmetic test proves the implemented formula, not that the
formula is universally correct hardware behavior.

## VBlank, VDP1 completion, and host pacing

Separate three concepts:

1. Saturn raster/VBlank progression;
2. PDS's game-owned frame boundary; and
3. host presentation delay.

Conflating them makes gameplay dependent on monitor refresh and can reorder
VDP1 completion relative to VBlank. The current experiment models a 59.826 Hz
raster and keeps presentation waiting in a drift-free rational accumulator.
Because PDS presents every second VBlank, the resulting game rate is about
29.913 frames/second.

The experiment should become default only after long routes show identical
state and exactly 44,100 mixed audio samples per emulated second on different
host refresh rates.

## Replay and input reports

Raw host-struct recordings are not durable recompilation evidence. A useful
format needs:

- an explicit version and fixed byte order;
- fixed-width fields;
- controller type/mode and connection state;
- per-frame connection transitions;
- deterministic state hashes; and
- audio sample positions.

Unknown flags must be rejected. Silent interpretation of a future recording
as an older format can create believable but invalid comparisons.

Detailed SMPC report emulation should be added only when translated PDS code
is shown to observe it. Explicit connection transitions are sufficient for
the currently demonstrated behavior.

## What was not adopted

- No wholesale recompilation framework migration.
- No full SH-2 cache, bus-contention, dual-CPU, or SCU scheduler.
- No emulator renderer or audio core copied into the project.
- No unverified RBG0 or cadence change enabled by default.
- No newer bgfx runtime accepted on compile success alone.
- No game assets used as committed fixtures.

These limits keep the project testable and reduce the chance that a broad
accuracy change fixes one route while silently changing another subsystem.

## Recommended next evidence

When lawful local game data and hardware evidence are available:

1. Record boot/title, name entry, camp/field, battle, and horizon-effect
   routes with replay v2.
2. Capture state, final/VDP1/RBG0 images, coefficient traces, and aligned WAV.
3. Run the same routes with each experiment independently enabled.
4. Compare at game-owned frame boundaries.
5. For RBG0, correlate fetch addresses with original-hardware evidence.
6. Promote only the experiment whose state and output evidence agree.
