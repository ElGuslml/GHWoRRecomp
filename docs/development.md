# Building and developing GHWoRRecomp

GHWoRRecomp is a static recompilation of **Guitar Hero: Warriors of Rock** (Xbox 360, 2010) to native Windows, built on
the [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk). The game's PowerPC code is translated to C++ and compiled. It
is drawn by [NativeXRenderer](native-x-renderer-ghwor.md) (Vulkan), which draws the game's Direct3D calls on the PC's
GPU; ReXGlue's emulation of the Xbox 360 GPU, Xenos (derived from [Xenia](https://xenia.jp)), is the alternative. The
game shares its engine with Guitar Hero 5, and [GH5Recomp](https://github.com/ElGuslml/GH5Recomp)'s fixes were ported
to it with their addresses translated ([`fixes-from-gh5.md`](fixes-from-gh5.md)); the other notes in
[`docs/`](README.md) describe each problem found and what was done about it.

## Requirements

- Git, [CMake](https://cmake.org/) 3.25 or newer, [Ninja](https://ninja-build.org/),
  [LLVM/Clang](https://github.com/llvm/llvm-project/releases) (MSVC alone does not build this) and the Windows SDK.
- Python 3 (glslang needs it to configure the Vulkan backend).
- Your own copy of the game: extract the content of your disc or ISO with
  [extract-xiso](https://github.com/XboxDev/extract-xiso/releases) and copy all of it into `assets/`, so that
  `assets/default.xex` exists (title ID `41560883`, version 0.0.0.1, XDK 2.0.11164). The expected executable has
  CRC32 `62201F03`; `GHWoRRecomp.exe` warns at startup if yours is a different version. See
  [`assets/README.md`](../assets/README.md).

## Building

```powershell
git clone --recursive https://github.com/ElGuslml/GHWoRRecomp
cd GHWoRRecomp
# copy your game files into assets\ first: the build recompiles assets\default.xex
cmake --preset win-amd64-release
cmake --build --preset win-amd64-release
```

- If you cloned without `--recursive`: `git submodule update --init --recursive`.
- The first build translates `default.xex` into C++ (`generated/`) and builds the SDK: it takes a while. If it stops
  with "precompiled header ... has been modified", run the build command again.
- Presets: `win-amd64-release`, `win-amd64-release-avx2`, `win-amd64-relwithdebinfo`, `win-amd64-debug`.

### The two executables and the profile (releases)

A release ships the game twice: `GHWoRRecomp.exe` for any x86-64-v2 processor and `GHWoRRecomp-avx2.exe` for
x86-64-v3 ones (AVX2, FMA, BMI1/2, MOVBE, LZCNT: Intel Haswell, AMD Zen and later). The launcher starts the second
where the processor runs it (`launcher_avx2 = false` in `ghwor_options.toml` turns that off); started by hand on an
older processor it says so and closes. Both are built with profile-guided optimization from the game's profile,
`tools\pgo\ghwor.profdata` (out of git, as the pipeline list): a Release build takes it when it is there
(`-DGHWOR_PGO=` builds without it). Only the executable's own code changes (the generated code, `src\`,
NativeXRenderer); the SDK's DLLs are the same for both. On the plan's bench (an i7-13700K held to four efficiency
cores, and whole): the AVX2 build with the profile 5-8 % more frames uncapped and 1-8 % less CPU at 120 FPS
(`local_docs/77`). Code the profile never ran (drums, the microphone, the rocker creator) is optimized as without one
(`GHWOR_PGO_COLD_AS_BASE`, on).

```powershell
cmake --preset win-amd64-release      ; cmake --build --preset win-amd64-release
cmake --preset win-amd64-release-avx2 ; cmake --build --preset win-amd64-release-avx2
..\recomp-tools\tools\release\Package-Release.ps1 -Game ghwor -Version x.y.z -Avx2Build out\build\win-amd64-release-avx2 ...
```

The profile: an instrumented build (`-DGHWOR_PGO=generate`, in a tree of its own; never shipped) writes what it ran
every 10 s to `GHWOR_PGO_FILE` (`src\pgo_profile.cpp`); play a few songs with it (the split render and the venue
every frame), then `llvm-profdata merge -o tools\pgo\ghwor.profdata <files>.profraw`. A profile older than the code
still builds: the functions changed since are optimized without it. Make it again when much of the code has changed.

## Running

```powershell
.\out\build\win-amd64-release\"GH Warriors of Rock Launcher.exe"   # choose the options, then Play
.\out\build\win-amd64-release\GHWoRRecomp.exe                       # or start the game directly with the saved options
```

- Game files are found in `assets\` next to `GHWoRRecomp.exe`, otherwise in the repository's `assets\`.
  `--game_data_root=<folder>` points somewhere else. `"GH Warriors of Rock Launcher.exe" --launcher_first_run` installs
  them again from an ISO.
- The saves, the achievements and the playtime go to the current profile of the launchers' shared store
  (`%APPDATA%\ElGuslml Recomps`); the shader cache to `userdata\` next to `GHWoRRecomp.exe` (if that folder
  cannot be written, for example under Program Files, to `Documents\ghwor`). Logs go to `logs\`.

## Configuration

| File (next to `GHWoRRecomp.exe`) | What it holds |
|---|---|
| `ghwor.toml` | Base settings. Created with the defaults if it is missing; a default setting the file lacks is appended at startup. |
| `ghwor_options.toml` | What GH Warriors of Rock Launcher saves. Loaded after `ghwor.toml`. |

Command-line options (`--name=value`, for example Steam launch options) win over both files, for example
`--fullscreen=false --window_width=1920 --window_height=1080`, `--native_x_renderer=false` (the Xenos path),
`--ghwor_frame_rate=120` (60, 72, 75, 90, 100, 120, 144, 160, 165 or 240;
[frame rates above 60](fixes-from-gh5.md#frame-rates-above-60)), `--ghwor_ultrawide=true`
([ultrawide](fixes-from-gh5.md#ultrawide)), `--anisotropic_override=0` (off; 5 = 16×), `--user_language=4`
(French; [language](fixes-from-gh5.md#language)), `--quit_prompt=false` (B in the main menu does not ask to
quit), `--ghwor_scene_every_frame=true` (Gustavo's switch, [scene-every-frame.md](scene-every-frame.md): the main
view's 3D scene drawn whole in every frame instead of over two, as on the console; NativeXRenderer's frame
generation sets it at startup; DLAA, DLSS and FSR 3.1 keep the split and run once a pair since NativeXRenderer's
D159: [the split render](fixes-from-gh5.md#the-split-render)), `--ghwor_camera_cut_reset=false`
(DLAA's, DLSS's and FSR 3.1's history kept across the camera's cuts, to compare:
[the camera's cuts](native-x-renderer-ghwor.md#dlaa-dlss-and-fsr-31)), `--gpu_adapter=10DE:2782` (the graphics card
by its PCI vendor and device IDs in hexadecimal, `"VVVV:DDDD"`, as the launcher's Settings → Graphics writes it; `""`
= automatic; the SDK's: its Direct3D 12 and Vulkan providers take it before `vulkan_device` and `d3d12_adapter`,
which then count only when no card has those IDs, and NativeXRenderer reads only `gpu_adapter` (its D141) and logs
`[native_x_renderer] gpu_adapter VVVV:DDDD: <name>`, or warns `not found among the Vulkan devices: the default one`;
two identical cards share their IDs, and the first is taken), `--native_x_renderer_dlss_model=cnn` (DLSS's CNN
model instead of the Transformer, NVIDIA's default; `transformer` or empty; `k`, Transformer K, the first Transformer
model in every mode) and `--native_x_renderer_dlss_log=true` (NGX's own log, verbose, in the game's: which DLSS preset
it used). The DLSS ones need DLSS: the launcher's Upscaling row (`launcher/game.h`, `kHasDlaa`), or
`--native_x_renderer_antialiasing=dlss_quality` and the other modes.

## The launcher

GH Warriors of Rock Launcher is [RecompLauncher](https://github.com/ElGuslml/RecompLauncher) (`recomp_launcher/`, a
submodule: the launcher of every one of these recompilations, SDL3 + Dear ImGui) with this game's profile in
`launcher/`: `launcher/game.h` (names, the disc, the look, the options), `launcher/game.cpp` (the game's own texts),
`launcher/palette.h` (its colors) and `launcher/art/`, and its resources in `res/launcher.rc`.
`installer/ghwor.iss` is its Inno Setup installer. The icon is `res/icon.svg` and `res/icon_small.svg`;
`res/make_icon.mjs` builds `res/icon.ico` from them, and `installer/make_images.ps1` the installer's images.

## How the SDK is modified

`thirdparty/rexglue-sdk` is [ElGuslml/rexglue-sdk](https://github.com/ElGuslml/rexglue-sdk), branch `ghmetal` (the
`recomp` line and the changes made since for the Guitar Hero games): the
official ReXGlue SDK's nightly of 2026-10-02 (v0.10.0 and 24 later commits) with the changes this project, GH5Recomp,
GHVanHRecomp, GHMetalRecomp, KameoRecomp, GRAWRecomp (Ghost Recon Advanced Warfighter), POTCAWERecomp (Pirates of the
Caribbean: At World's End) and [NativeXRenderer](https://github.com/ElGuslml/NativeXRenderer) need, one commit each.
None of the general fixes is specific to one game, and the options they add are off by default (this game sets the ones
it needs in `src/startup.cpp`); the ones for NativeXRenderer change nothing while it is off. The branch's `RECOMP.md`
lists them, and each commit message explains its change.

`native_x_renderer/` is NativeXRenderer, a submodule: a native renderer that hooks the game's Direct3D library. This
game's side of it (its hooks and the map of the game's Direct3D library) is in
`src/native_x_renderer_ghwor.cpp` and [`native-x-renderer-ghwor.md`](native-x-renderer-ghwor.md).

## Contributing: branches and pull requests

The work lands in groups, so a pull request needs updating only when a group is merged, not after every push.

| Branch | What it is |
|---|---|
| `main` | Releases only (tags `vX.Y.Z`). |
| `dev` | The integration branch: it moves only by merges (a finished group of features, a pull request, a release's version commit). **Base pull requests on `dev`.** |
| `feat/<topic>` | The maintainer's work in progress, two or three features at a time. Pushed often and possibly rewritten: not a base for other work. Merged into `dev` once tested. |
| `fix/<topic>` | A fix players need now, merged into `dev` at once. |

Each merge into `dev` is announced with what it brings and the shared files it touched. The open `feat/` branches show
what is being worked on; to avoid working on the same files, open a draft pull request early.

**The submodules.** NativeXRenderer, RecompLauncher and the SDK fork share the branch `ghmetal` with the other
recompilations. A group that changes a submodule uses the same `feat/<topic>` there; when the group is done, that branch
is merged into `ghmetal` first, the game's branch records the merge, and the game's branch is merged into `dev`. A pull
request to a submodule targets `ghmetal`, and the game's pull request records the submodule pull request's commit.

**Rules that keep conflicts small:**
- **Numbers given at merge, in the order merged:** NativeXRenderer's decisions (`docs/decisions.md`: a pull request
  writes `D-next: <title>`, `D-next-2`, ..., in code comments too), the SDK's changes (`RECOMP.md`: `next`) and
  RecompLauncher's version (its `CHANGELOG.md` entry under `## Unreleased`, `launcher_version.h` untouched).
- **The shared indexes** (`README.md`'s tables, `res/README.txt`, `docs/README.md`): the maintainer updates them at
  merge. A pull request brings its own note, `docs/<topic>.md`, and says in its description what the indexes should
  say.
- **Hooks:** a new feature's `[[midasm_hook]]`s, functions and switch tables go in a file of their own,
  `hooks/<topic>.toml`, listed in the `includes` array at the top of `ghwor_functions.toml` (one line each). The
  codegen reads includes recursively and runs again when any of them changes. Only one hook can sit at an address
  (the codegen keeps the last one read and says so only in its debug log): search the address in every `.toml` first.
  The existing hooks stay where they are.
- **After every codegen** (a change to `ghwor_functions.toml`, a hook file or the code generator), run recomp-tools'
  `tools/codegen/check_switch.mjs` (every switch table decoded) and `tools/codegen/msr_sites.py --project <repo>`.
  `ghwor_functions.toml` sets `skip_msr = true`, so the XDK's inline Interlocked operations run as the host's atomics
  without the SDK's global lock; `msr_sites.py` checks by address that each of those sites is still a reserved load
  and store only, and fails when one is not.
- **Settings:** a new option is off by default and leaves the game as it was; its default goes in `src/startup.cpp`.
- **Updating a pull request:** merge `origin/dev` into its branch, or rebase with `git config rerere.enabled true`
  so that a conflict resolved once is not resolved again.
- **In the description:** what was tested (hardware, settings, the game's log), and what was not.
