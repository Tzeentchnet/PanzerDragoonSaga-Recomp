# Dependency Inventory

Dependencies are pinned to exact Git revisions. This inventory explains the
current update decisions; upstream license files remain authoritative.

## Accepted pins

| Dependency | Revision | Release / role | License summary |
|---|---|---|---|
| SDL | `829a65d769d935c4852f8159e964312c0957260a` | release-3.4.18 | zlib |
| Tracy | `30997d5ca6bb632cc10807a1da8a6d3de0aeeb3c` | v0.14.1 | BSD-3-Clause |
| Dear ImGui | `f1cc2ae15e53a861a874c3034aae6798fde194ab` | v1.92.9b | MIT |
| ImGuizmo | `b796ac3b861afc6e91ca74e4611effd9c9527367` | 1.10 | MIT |
| GLM | `bf71a834948186f4097caa076cd2663c69a10e1e` | 0.9.9.8 | MIT / upstream terms |
| bgfx.cmake | `f40f4ca5802c5011210af9d8031763617658ed3d` | v1.139.9155-513 wrapper | BSD-style; see nested projects |
| bgfx | `651c2031169bae79d81d5035295e1a4b886797ba` | renderer | BSD-2-Clause |
| bimg | `b43fea9eae0e6a98118454a6d17c6cb25f5e6403` | image library | BSD-2-Clause |
| bx | `36ad6131f4d292f62db64236d1d7f9f70eb26b13` | base library | BSD-2-Clause |
| SoLoud fork | `e9e863e8795359079f2c4974af10e5023675f734` | SDL3-capable project fork | zlib/libpng |

## Update decisions

### SDL and Tracy

Both were updated within their supported lines. SDL remains static and its
unused products are disabled. Tracy is built only for the profiling
configuration and should be paired with the matching 0.14.1 profiler client.

### Dear ImGui, ImGuizmo, and GLM

ImGui was kept on the 1.92 line to avoid combining this pass with the broader
1.93 rendering changes. ImGuizmo 1.10 moved its sources under `src/`, which is
reflected in the project CMake. GLM remains on 0.9.9.8; 1.x is a separate API
and behavior migration.

### bgfx lockstep bundle

The retained wrapper and its nested bx/bimg/bgfx revisions are one unit. Do
not update one nested component independently.

The evaluated candidate was:

| Component | Candidate revision |
|---|---|
| bgfx.cmake | `8c0a5f4b5f0c5ff94292435a5c8ccaef78688b47` (v1.167.9590-598) |
| bx | `b366b69f741853d54bb2ad55838573390f771d4a` |
| bimg | `9bfbd6493b48a15238f2d1374a718ec026524515` |
| bgfx | `0c595f8e12c1f0db0653922c8c838ad74976af0f` |

It passed configure, shaderc, shader generation, and Debug PDS compilation.
It was not accepted because D3D startup/reset and representative rendering
captures could not be tested without local game data. Candidate-specific
FetchContent, shader naming, and swap-chain adaptations were removed when the
retained bundle was restored.

### SoLoud

The project uses a fork containing required SDL3 work. It was already the
compatible line for this application, so changing upstreams would add risk
without an identified defect.

## Restricted or deferred dependencies

### aoSDK

The copied tree identifies itself as an Audio Overload SDK 1.4.8-era
development release. Its own `license.txt` lists a mixture of BSD, zlib, GPL,
and historical MAME licenses. In particular, the compiled SSF M68000 and SCSP
components are marked with the historical non-commercial MAME terms.

This is not a normal pin-update candidate. The safe path is a separately
validated, permissively licensed replacement behind a narrow audio-core
interface. Do not copy GPL, non-commercial, or unlicensed implementations
into that replacement.

### emsdk

The inactive web toolchain predates current Emscripten. Updating it requires
an end-to-end web-platform migration and is not included in the Windows
modernization pass.

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
