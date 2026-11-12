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
- Presets: `win-amd64-release`, `win-amd64-relwithdebinfo`, `win-amd64-debug`.

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
`--ghwor_frame_rate=120` (60, 120, 144, 160, 165 or 240;
[frame rates above 60](fixes-from-gh5.md#frame-rates-above-60)), `--ghwor_ultrawide=true`
([ultrawide](fixes-from-gh5.md#ultrawide)), `--anisotropic_override=0` (off; 5 = 16×), `--user_language=4`
(French; [language](fixes-from-gh5.md#language)), `--quit_prompt=false` (B in the main menu does not ask to
quit), `--ghwor_split_render=false` (the main view's 3D scene drawn whole in every frame instead of over two, as on
the console; NativeXRenderer's frame generation sets it at startup unless it is given, and `true` keeps the split with
it; DLAA, DLSS and FSR 3.1 keep the split and run once a pair since NativeXRenderer's D146:
[the split render](fixes-from-gh5.md#the-split-render)), `--ghwor_camera_cut_reset=false`
(DLAA's, DLSS's and FSR 3.1's history kept across the camera's cuts, to compare:
[the camera's cuts](native-x-renderer-ghwor.md#dlaa-dlss-and-fsr-31)), `--gpu_adapter=10DE:2782` (the graphics card
by its PCI vendor and device IDs in hexadecimal, `"VVVV:DDDD"`, as the launcher's Settings → Graphics writes it; `""`
= automatic; the SDK's: its Direct3D 12 and Vulkan providers take it before `vulkan_device` and `d3d12_adapter`,
which then count only when no card has those IDs, and NativeXRenderer reads only `gpu_adapter` (its D141) and logs
`[native_x_renderer] gpu_adapter VVVV:DDDD: <name>`, or warns `not found among the Vulkan devices: the default one`;
two identical cards share their IDs, and the first is taken), `--native_x_renderer_dlss_model=cnn` (DLSS's CNN
model instead of the Transformer, NVIDIA's default; `transformer` or empty) and `--native_x_renderer_dlss_log=true`
(NGX's own log, verbose, in the game's: which DLSS preset it used). The DLSS ones need DLSS: the launcher's
Upscaling row (`launcher/game.h`, `kHasDlaa`), or `--native_x_renderer_antialiasing=dlss_quality` and the other modes.

## The launcher

GH Warriors of Rock Launcher is [RecompLauncher](https://github.com/ElGuslml/RecompLauncher) (`recomp_launcher/`, a
submodule: the launcher of every one of these recompilations, SDL3 + Dear ImGui) with this game's profile in
`launcher/`: `launcher/game.h` (names, the disc, the look, the options), `launcher/game.cpp` (the game's own texts),
`launcher/palette.h` (its colors) and `launcher/art/`, and its resources in `res/launcher.rc`.
`installer/ghwor.iss` is its Inno Setup installer. The icon is `res/icon.svg` and `res/icon_small.svg`;
`res/make_icon.mjs` builds `res/icon.ico` from them, and `installer/make_images.ps1` the installer's images.

## How the SDK is modified

`thirdparty/rexglue-sdk` is [ElGuslml/rexglue-sdk](https://github.com/ElGuslml/rexglue-sdk), branch `recomp`: the
official ReXGlue SDK's nightly of 2026-10-02 (v0.10.0 and 24 later commits) with the changes this project, GH5Recomp,
GHVanHRecomp, GHMetalRecomp, KameoRecomp, GRAWRecomp (Ghost Recon Advanced Warfighter), POTCAWERecomp (Pirates of the
Caribbean: At World's End) and [NativeXRenderer](https://github.com/ElGuslml/NativeXRenderer) need, one commit each.
None of the general fixes is specific to one game, and the options they add are off by default (this game sets the ones
it needs in `src/startup.cpp`); the ones for NativeXRenderer change nothing while it is off. The branch's `RECOMP.md`
lists them, and each commit message explains its change.

`native_x_renderer/` is NativeXRenderer, a submodule: a native renderer that hooks the game's Direct3D library. This
game's side of it (its hooks and the map of the game's Direct3D library) is in
`src/native_x_renderer_ghwor.cpp` and [`native-x-renderer-ghwor.md`](native-x-renderer-ghwor.md).
