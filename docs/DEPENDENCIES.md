# Dependency Inventory

Dependencies are pinned to exact Git revisions. This inventory explains the
current update decisions; upstream license files remain authoritative.

## Accepted pins

| Dependency | Revision | Release / role | License summary |
|---|---|---|---|
| SDL | `829a65d769d935c4852f8159e964312c0957260a` | release-3.4.18 | zlib |
| Tracy | `30997d5ca6bb632cc10807a1da8a6d3de0aeeb3c` | v0.14.1 | BSD-3-Clause |
| Dear ImGui | `f1cc2ae15e53a861a874c3034aae6798fde194ab` | v1.92.9b | MIT |
| GLM | `bf71a834948186f4097caa076cd2663c69a10e1e` | 0.9.9.8 | MIT / upstream terms |
| bgfx.cmake | `8c0a5f4b5f0c5ff94292435a5c8ccaef78688b47` | v1.167.9590-598 wrapper | BSD-style; see nested projects |
| bgfx | `0c595f8e12c1f0db0653922c8c838ad74976af0f` | renderer | BSD-2-Clause |
| bimg | `9bfbd6493b48a15238f2d1374a718ec026524515` | image library | BSD-2-Clause |
| bx | `b366b69f741853d54bb2ad55838573390f771d4a` | base library | BSD-2-Clause |
| SoLoud fork | `e9e8aea38d65a6f3a565ccf6103a4883ae8bb9c6` | SDL3-capable project fork | zlib/libpng |

## Update decisions

### FetchContent delivery

The maintained dependencies above are fetched by CMake from exact commits.
They are no longer Git submodules, so a normal clone does not need recursive
submodule initialization. `FETCHCONTENT_SOURCE_DIR_<NAME>` overrides remain
available for local mirrors and disconnected builds.

Only the products used by PDS are enabled: static SDL, the bgfx libraries and
`shaderc`, and the first-party SoLoud wrapper. Tracy's client source is
compiled only when the optional `PDS_ENABLE_TRACY` diagnostic is enabled.

### SDL, Tracy, Dear ImGui, and GLM

SDL and Tracy were updated within their supported lines. SDL remains static
and its unused products are disabled. Tracy should be paired with the matching
0.14.1 profiler client. ImGui remains on the 1.92 line to avoid combining this
pass with broader 1.93 rendering changes. GLM remains on 0.9.9.8; 1.x is a
separate API and behavior migration.

### bgfx lockstep bundle

The wrapper and its nested bx/bimg/bgfx revisions are one unit. Do not update
one nested component independently.

The accepted bundle builds the current swap-chain API and generates all
runtime shader formats from one source set:

- DXIL for Direct3D 12;
- SPIR-V for Vulkan; and
- DXBC for Direct3D 11 and WARP.

Debug startup was verified for explicit Direct3D 12, Vulkan, and D3D11 WARP,
as well as automatic Direct3D 12 selection. Full representative frame
comparison remains blocked until local game data and a deterministic replay
route are supplied.

### SoLoud

The project uses a fork containing required SDL3 work. It was already the
compatible line for this application, so changing upstreams would add risk
without an identified defect.

## Removed dependencies

### ImGuizmo

Every first-party ImGuizmo reference was commented out. Its target wiring,
source checkout, and stale comments were removed rather than retaining an
unused dependency.

### emsdk

The inactive web toolchain had no root build integration and was removed.
Restoring a browser target requires a separate current-Emscripten effort.

## Restricted local dependency

### aoSDK

Only the Saturn SSF M68000/SCSP/DSP closure and its provenance/license
material remain under `ThirdParty/aosdk`. The original broader tree identified
itself as an Audio Overload SDK 1.4.8-era development release and mixed BSD,
zlib, GPL, and historical MAME licenses. In particular, the compiled SSF
M68000 and SCSP components retain historical non-commercial MAME terms.

This is not a normal pin-update candidate. The safe path is a separately
validated, permissively licensed replacement behind a narrow audio-core
interface. Do not copy GPL, non-commercial, or unlicensed implementations
into that replacement.

### Dormant PDS_Tool headers

Copied nlohmann/json and imgui memory-editor headers belong to a currently
unsupported tool path. Update or remove them only when the support status of
PDS_Tool is decided.

## Update procedure

For future dependency work:

1. Update one dependency group at a time.
2. Record the exact revision and release tag.
3. Review upstream migration notes and the exact license files.
4. Configure in a clean or known persistent build tree.
5. Build only the affected exact targets with at most four jobs.
6. Run all data-independent tests.
7. For renderer/input/audio changes, run deterministic PDS routes before
   accepting the pin.
8. Update this inventory with the decision and validation evidence.
