# BOTW Compiled

Experimental native Windows x64 port of **Breath of the Wild for Wii U**, based on CafeRecomp. This repository contains the runtime and tools; game files and generated code remain in a separate local workspace.

PowerPC recompilation, the Cafe runtime, and the GX2/Latte shader translator are inherited from CafeRecomp. Rendering of Link and the environment remains incomplete. Building the program or passing GPU tests does not establish that the game is playable.

The recorded runtime suite passes 25 tests. Launching displays the opening screen and introductory text. Longer probes load MainField placement and terrain data, but character/environment rendering and playable movement remain unverified. See the [validation notes](VALIDATION.md). Captures remain local.

## Getting started

Requirements: MSVC C++ tools and a Windows SDK, CMake, and PowerShell. Use Visual Studio 2022 or Ninja from an initialized MSVC developer shell. Building the translator downloads zlib from its official distribution with a pinned SHA-256 checksum.

The local workspace now contains a recovered matching compiler, and fresh Ninja Release builds work. This compiler is a local dependency and is not distributed in the repository. For a new Ninja build, pass `-Generator Ninja`; existing directories retain their configured generator. MSBuild-specific arguments are used only for Visual Studio builds.

From this directory:

```powershell
.\botw.ps1 doctor
.\botw.ps1 components
.\botw.ps1 translator
.\botw.ps1 generate
.\botw.ps1 build
.\botw.ps1 run -Headless -Mute -Blocks 10000
```

The local project is located at `E:\BOTW Compiled`. The default data and log directory is the `workspace` subfolder, resolved relative to the project directory. Use `-WorkRoot` to override it for each command. Extracted titles belong in `game/base`, `game/update`, and `game/dlc` beneath that directory. Successful preparation writes `snapshot.complete.json`; launching requires this marker to avoid starting during extraction. The v208 RPX checksum is checked before generation and launch. A different checksum is rejected because the inherited hooks use fixed addresses.

The `components` and `translator` commands work without game files. `components` builds and tests shaders, textures, HDR targets, post-processing, memory, and audio using controlled inputs.

Generation writes several gigabytes of C into the workspace. By default, the full build produces `game-build/bin/botw_recomp.exe` and its DLLs. The default launch block limit is for diagnostics; `-Blocks 0` runs without that limit.

The retained native build is in `E:\BOTW Compiled\build\native`; copied game data and generated sources are in `E:\BOTW Compiled\workspace`. The successful build path is stored in `workspace/native-build.json` and reused by `doctor` and `run`. Junctions at the old locations preserve absolute paths embedded in the existing build. Original source and data copies are retained beside those junctions with the `.before-relocation` suffix. Use a new `-BuildRoot` for a clean rebuild. These directories are separate from the active Cemu installation.

`-BuildRoot` selects another native build directory. The development option `-PrecompiledObjects` can reuse existing Release objects built with the same compiler, options, and ABI; source and header timestamps are checked. The retained build used this cache to link the DLL on the SSD while recompiling modified sources.

Sources newer than their objects are recompiled normally. `-RecompileGenerated` forces a full rebuild, including after compiler or build-option changes.

`test` runs the complete runtime suite when the launcher has been built, or the component suite otherwise.

## Available settings

### Custom logo

Place a PNG in `custom-assets/textures/title-logo.png`, then run `python tools/install_logo.py` from this directory. This requires Python 3.10+ and the dependencies in `tools/requirements-assets.txt`. The tool preserves proportions and transparency and replaces the logo in both isolated update archives. Texture dimensions, format, layout offsets, and decompressed layout size are preserved; other archive members are verified.

The next launch uses the installed logo. `python tools/install_logo.py --restore` restores the original archives backed up in `workspace/custom-assets-backup/title-logo`. No active Cemu files are modified. The PNG and archives remain excluded from GitHub.

### Presentation

```powershell
.\botw.ps1 run -Width 1920 -Height 1080 -VSync -PresentationFilter linear -Blocks 0
```

`Width` and `Height` change the window's client dimensions and presentation. They do not yet increase the resolution of the game's render targets. VSync applies to the D3D11 swap chain. The `nearest` or `linear` filter affects only final presentation; game material samplers are preserved.

The planned graphics improvements will have separate settings: internal resolution, shadows, and draw distance; and lighting, effects, and textures. They remain to be implemented after validating the original scenes. No placeholder ray tracing, HD texture, FPS, or draw-distance controls are exposed.

### Local pixel shader overrides

`-ShaderOverrides` exports pixel shaders when they are first translated and compiled into the workspace's `shader-overrides` directory as `XXXXXXXX.original.hlsl`. To experiment, copy one to `XXXXXXXX.override.hlsl`, edit the copy, and relaunch. Already cached shaders are not reloaded immediately.

The full original file is compared before applying an override, preventing an identifier collision from selecting a different shader. The original file is never overwritten. Invalid source or a modified original blocks that shader and produces a diagnostic. Overrides must preserve the shader's resource and output interfaces. This feature covers only pixel shaders already supported by the translator; it does not implement missing Latte instructions or establish a validated lighting improvement in the game.

`-Capture` saves diagnostic captures in the same workspace. Game programs, shaders, and generated files remain local and are not added to the repository.

Run a time-limited probe with `run -Blocks 0 -Seconds 120 -Headless -Mute -Capture`. Only the process started by that command is stopped at the deadline; it does not close Cemu. Reaching this deadline is a diagnostic stop, not evidence of a successful full boot.

### Boot investigation

Optional process environment variables support reproducible local probes:

- `BOTW_TRACE_FILES=1`: up to 4,096 file-open results, independent of the normal log budget.
- `BOTW_WORKER_SLICE=128`: a bounded cooperative worker budget (1–256, default 32). Spinlock fairness rules still apply. This is a diagnostic scheduling option; it is not a frame-rate setting.
- `BOTW_TEST_START_SAMPLE=120` and `BOTW_TEST_START_PRO=1`: a three-poll logical A press on the Pro Controller after rendering begins.
- `BOTW_TEST_A_SAMPLES=120,2700,3000`: up to 32 explicit A presses, with neutral gaps. These operate through controller packets and do not change scene flags or actor positions.
- `BOTW_TRACE_DRAW_FRAME=yellow` together with `-Capture`: capture the material and before/after evidence for the first matching yellow draw. This is expensive and should stay disabled in ordinary launches.

With `-Capture`, the first rejection of each vertex-program signature records assembled inputs, uniform blocks, and failure context. `botw_latte_vertex_failure_replay` accepts the absolute capture prefix to reproduce supported inputs offline. Texture callbacks are explicitly reported as missing dependencies when their sampled data is not recorded; a replay cannot invent those values. Captures contain game-derived data and remain local.

## Data isolation

Preparation uses copies of the three NUS titles, extracted into the workspace. It does not include Cemu saves. The launcher uses only these extracted copies. Save paths inherited from the runtime resolve beneath `game/update/save` in the workspace; logs are in `logs/runtime.log`.

`BOTW_WIIU_MLC_ROOT` is set to `system/mlc01` inside this workspace. The launcher does not automatically search Cemu's MLC. Missing system files may still prevent a complete launch; they must come from a separate copy of the console dump.

The recorded preparation copied only `portable/mlc01/sys/title` from Cemu, without `usr/save`. `tools/prepare-game.ps1` can reproduce the copy and extraction of the three European titles using a user-supplied CDecrypt executable and a fresh workspace; it refuses an existing extraction.

The active Cemu directory and saves are unchanged. Do not redirect these paths into an active Cemu session.

The European v208 RPX has the checksum expected by the upstream hooks. When built through `botw.ps1 build`, the launcher reads title, group, and region from the copied metadata. Runtime title and DLC services use this identity instead of always reporting a North American title.

## Provenance

- [CafeRecomp](https://github.com/hardkiller2565123123/CafeRecomp), commit `922b852e56cd440d23ed8ede3e0b86495e68967f`: code base; licenses and notices retained.
- [WiiCompiled](https://github.com/patchzyy/wiicompiled), commit `bd219e38a3e9a1a5047ce41d8cd01210bef3af43`: reference for the PowerPC recompilation pipeline and checks. Its Wii/GX runtime is not a Wii U/GX2 backend. No code was imported.
- [CDecrypt](https://github.com/VitaSmith/cdecrypt): used locally to extract the NUS copy into a separate directory; not distributed with this project.

The historical `README` and `RELEASE-NOTES` describe upstream experiments, not tests performed for this project. `VALIDATION.md` records this project's checks.
