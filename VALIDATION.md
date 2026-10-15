# Validation notes

The Windows x64 foundation builds and starts BOTW's recompiled code. Visual inspection confirms the opening screen and introductory text. The final capture is uniformly yellow: rendering of Link and the environment remains incomplete, and the renderer rejects vertex programs.

**Tooling at the last recorded check:** the Release build and tests below were completed with MSVC 2022. The installation at `C:\Program Files\Microsoft Visual Studio\2022\Community` was subsequently absent, and `vswhere` found no Visual Studio installation. A new CMake configuration failed for that reason. Existing binaries are retained; rebuilding requires the Visual Studio 2022 C++ tools. The `doctor` command reports their absence.

## Inputs and isolation

- European game: title `00050000101C9500`, update v208, and DLC `0005000C101C9500` v80.
- RPX SHA-256: `BA58DA5B95CE929E005D058CEB08B9B2788D1AB2BBC8A6C189BBADCA0BB34D30`, matching the input expected by the CafeRecomp hooks.
- The three NUS titles were copied and extracted into `E:\BOTW Compiled\workspace\game`. Source files and their NUS copies had matching sizes and modification times when checked.
- Only `portable/mlc01/sys/title` was copied for system titles. No Cemu saves were copied, and the active Cemu directory was not modified.
- The launcher uses the copied data, save paths beneath `game/update/save`, its own logs, and a separate MLC.

## Completed checks

- Release MSVC x64 build of the translator and full runtime: successful. PowerPC sources and objects remain local; running the port does not depend on the Cemu process.
- Full CTest suite: **24 of 24 tests passed**. Coverage includes RPX loading, a 10,000-block boot, GPU formats and targets, shaders, audio, memory, imports, and North American/European title identity.
- Independent component suite: **12 of 12 tests passed**.
- DolRecomp: **5 targeted tests passed** (`rpx`, `opcodes`, `c_execute`, `fpscr`, `float_semantics`).
- Presentation created a real hidden 640×360 window using the requested configuration. Presentation controls do not change game render-target resolution.
- HLSL overrides were checked with GPU pixel readback: red before replacement, green afterward. Invalid source and a mismatched original are rejected; rejected draws preserve the target.
- Guards rejected a synthetic unsupported RPX and overlapping source/copy directories. PowerShell syntax and `git diff --check` were checked.

Generating v208 produces 2,259 C chunks. A real defect was fixed: the generic backend emitted UGQR accesses through a helper that treated them as privileged. Post-processing reconstructs the raw instruction and routes it through the existing Espresso handler, resuming at the next PC. This affects 51 chunks. The test reproducing instruction `0x03E27924` failed before the fix and passes afterward without changing its assertion. Read/write encodings and post-processing idempotence were also checked using a synthetic file.

## Game probe

A port process ran hidden and muted for approximately 120 seconds, with capture and a diagnostic logical Start press. The launcher's timer then stopped it; this was not a normal game exit. The internal counter reached 2,760 presents, which is not an FPS or playability measurement.

The opening logo and background actually rendered, and the text "Breath of the Wild" was observed during the introduction. The final capture is a solid yellow screen. The log reports several `vertex-program` rejections, including at internal frames 1,724, 2,415, and 2,432. These programs must be fixed, followed by checks of characters, environments, textures, depth, and effects in a real scene before claiming playable gameplay or validated graphics improvements.

Opening screen capture: `first-render.png` (local, excluded from the source repository).

Final diagnostic capture showing the remaining rendering defect: `runtime-capture.png` (local, excluded from the source repository).

## Finding the results

Detailed logs remain in `E:\BOTW Compiled\workspace\logs`, including `native-tests-final.log`, `runtime.log`, and `runtime-probe.json`. Captures and programs remain in `captures`; local HLSL files are in `shader-overrides` under the same workspace. Binaries are in `E:\BOTW Compiled\build\native\bin`; the project launcher finds this path through `native-build.json`.

## Relocation on October 15, 2026

The project and its local data are grouped under `E:\BOTW Compiled`: sources at the root, data in `workspace`, binaries and objects in `build`, and the WiiCompiled reference in `references`. The workspace copy contained 65,610 files (39,332,929,294 bytes). Comparison after copying found no missing, different, or extra files. The RPX and binary checksums were verified.

All 24 CTest tests passed after relocation. The launcher run from this directory uses the new paths and completes a hidden, muted 10,000-block diagnostic normally, with exit code 0 and without a timer stop. This confirms launching from the new location; it does not validate complete scene rendering.

Old paths are junctions to the new directory, preserving absolute paths retained in the build. Pre-relocation sources and data are kept at the old locations with the `.before-relocation` suffix. Logs from the earlier long rendering probe are preserved in `workspace/logs/before-relocation`. New checks are recorded in `workspace/logs/relocation-tests.log`, `runtime.log`, `runtime-probe.json`, and the `relocation-*.log` files. The active Cemu files and saves were not involved in this relocation.

## Custom logo on October 15, 2026

The supplied PNG is retained unchanged at `custom-assets/textures/title-logo.png`. `tools/install_logo.py` fits a copy into the 382×300 RGBA8 sRGB texture while preserving proportions and transparency. It replaces `timg/Logo_W_00^l.bflim` within `Layout/BootUp.sblarc` and `Layout/Title.sblarc`, only in the isolated update archives. Texture headers and decompressed layout sizes remain identical. Other archive members are compared after reopening.

Independent decoding with [BFLIM Extractor](https://github.com/aboood40091/BFLIM-Extractor) confirmed every RGBA pixel of the converted texture. Restoration of both archives was verified against their original checksums, then the custom logo was reinstalled; a second installation produced identical checksums. The copied PNG retains the supplied file's checksum.

A port process then actually rendered the custom logo on the opening background: "Compiled" is visible in `workspace/logo-validation/custom-logo-in-game.png`. This hidden, muted probe was stopped by its timer after approximately 50 seconds; it does not establish a normal game exit or complete 3D rendering. Its logs are in `workspace/logo-validation`. The custom logo remains installed, with original archives retained in `workspace/custom-assets-backup/title-logo`.
