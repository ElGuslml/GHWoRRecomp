# NativeXRenderer in Guitar Hero: Warriors of Rock

The game's side of NativeXRenderer (`native_x_renderer/`, the library that hooks a game's Direct3D calls and draws
on the host). The hooks and the profile are in [`src/native_x_renderer_ghwor.cpp`](../src/native_x_renderer_ghwor.cpp);
the library's checklist is `native_x_renderer/docs/integrating-a-game.md`.

**Status (2026-11-07):** GH5's side, mapped to this game's library; FXAA and SMAA. The default renderer, as in GH5
(`kNativeXRendererDefault`: a first run draws with NativeXRenderer and SMAA); the launcher's Xenos card or
`--native_x_renderer=false` turns it off.

## The addresses

- The game links the XDK Direct3D library of 2.0.11164 (GH5 2.0.8276) at `0x822D02A0`–`0x822EFCD0`. Each of GH5's
  hooked functions was found by the function matcher over both games' instruction indexes and compared instruction by
  instruction (`tools/port/nxrcmp.py`, gitignored): every one has GH5's instruction count except Resolve (961 → 1030),
  Present (396 → 410) and SetTexture (95 → 99), and they differ only in device offsets.
- **The device layout** is GH5's up to the vertex declaration: the six dirty masks, the command buffer's write pointer,
  the fetch, VS, PS and bool/loop constants, the register shadow, the fence and write-back block and the flags, each
  used as often in both libraries. From the vertex declaration on every field is 168 bytes later (SetVertexDeclaration's
  store, SetIndices', SetRenderTarget's targets and depth, SetStreamSource's `addi 3159` = +12636, SetTexture's `addi
  3180` = +12720, the viewport's floats, the shaders, the predication mask, BeginTiling's tile count); the draw state at
  the end 176 to 212 (the inline draw end `[dev+13652]`, GH5's 13444). The resource objects are GH5's.
- The device pointer is at `0x829BBA74`: the renderer's block `0x829BB350` + 1828 (GH5's + 2032).
- GH5's DrawIndexedVerticesUP and BeginIndexedVertices are not in this executable. The engine's three BeginVertices /
  EndVertices pairs (`sub_822C3F78`, `sub_822C53A0`, `sub_822C9948`) call EndVertices (`sub_822D1890`, `[dev+48] =
  [dev+13652]`): `inline_draw_end` covers them and EndVertices is not hooked, as in GH5.
- **Antialiasing: FXAA and SMAA** (`native_x_renderer_antialiasing`, SMAA by default; the launcher's renderer card).
  Every resolve goes through the render targets' helper `sub_8255AC80`; `scene_resolve_caller = 0x8255AD1C`, its
  color resolve. The game alternates two kinds of frame, both shown, which copy the scene into the same 1280x720
  texture the screen effects' composite samples, from different places: one through the scene renderer
  `sub_8255A3C8` after the scene's draws (`< 0x8255A994`), the other from inside the composite `sub_82641110` after the
  bloom's passes (`< 0x82641558`), with an earlier copy of the stage and the crowd (`< 0x8264C1B8`). With the first
  outer caller only, every other frame was left as it was (applied 300 times in 600 frames). So no outer caller:
  every 1280x720 copy through the helper is antialiased, 900 times in 600 frames (in the menus too); the helper's
  smaller copies (the effects' quarter sizes, the lobby's 400x400 portraits) are left out by their size. The HUD and
  the note highway are drawn over the composite: they stay sharp. No DLAA: the game's views are not marked
  (`native_x_renderer::BeginView`), so DLAA would turn SMAA and FXAA off and DLSS never run; the launcher (RecompLauncher
  3.6, `game::kHasDlaa = false`) offers SMAA, FXAA and none, and reads an options file's `dlaa` as SMAA.

## Checked (2026-11-02, Game.ps1, keyboard)

- The observer (`--native_x_renderer_verify`, F10): the main menu, 522 of 522 draw calls match the rebuilt state; a
  Quickplay+ song over three frames, 602 of 602 of the draws the GPU executed in them. The unmatched packets are the
  library's own (Clear, Resolve). In the song most of the CPU's draw calls had no execution inside the observed frames
  (933 of 1535): their packets ran after the window (the scene is drawn into another part of the command buffer); none
  of the executed ones differed.
- `--native_x_renderer=true`, 1280x720: the main menu, the band lobby, a song (the venue, the band solid, the highway,
  the HUD, the red border, the depth of field); 0 failed draws, 0 placeholder textures; 16.67 ms frames, the renderer
  about 6 ms of CPU a frame; the process 1.84 cores in the main menu and 2.24 in a song (the Xenos path 1.46 and 2.03).
- 2560x1080 with `ghwor_ultrawide`: rendered at 2560x1080 (`render scale 2.0000 x 1.5000`), the 3D views widened, the
  2D layer at 16:9; in a song the stage over the whole width with the highway, the HUD and the red border in place.
  Outside a song (the menus, the fail screen and the band's fail scene before it) black bars cover the sides, as on the
  Xenos path.

## Not done yet

- **A resolve into a 3D texture** (`resolve to a texture of dimension 2: not supported yet`): the engine's
  "LightVolume" render stage (its stage table at `0x8286B9B0`, gated by `[0x82858FF0]`, 1 in the image) builds a volume
  of 16 slices from the scene's lights with bit 4 set (`sub_825F0CE0`: per slice a draw, then a resolve into the slice
  of the renderer's texture 44) and draws with it at once (`sub_825F0668` -> `sub_825EF978`, the volume as sampler 3).
  It ran in two frames of one run of five (16 resolves skipped in each, entering the band lobby and the song), never in
  the others: a light effect of a frame. NativeXRenderer skips 1D and 3D resolve destinations, so that frame draws its
  light volume from a texture nobody filled.
- Frame rates above 60 and larger outputs (3440x1440, 160 FPS) with NativeXRenderer, a real play test.
- **Shadow quality** (the launcher's row, NativeXRenderer's D92; GH5's `cdec72f`): off. GH5's shadow filter fix does not
  carry over: its nine samples come from `sub_825CCE98`, with a switch on 4, 9 or 16 samples that nothing in this
  executable has, and the stage that called it is here the light volume's (`sub_825F14C8`). Before the row is turned on
  the log must show NativeXRenderer finding this game's shadow maps in a song. At startup, with
  `native_x_renderer_shadow_scale = 4`, it takes a 640x360 depth surface drawn with no color target (EDRAM base 1488)
  for one: a screen fraction, not a shadow map, which the shadow scale must leave alone.

## Stalls on a first play (NativeXRenderer D126)

A graphics pipeline new to the driver can take most of a second to compile, and D101 made draws into offscreen targets
wait for theirs: in GH5 the first play of a song stopped for up to 787 ms. D126 skips instead of waiting when the target
has been drawn into on more than 16 frames in a row (it keeps its last frame and mends itself); other offscreen draws
still wait. Measured in GH5 (its `docs/first-play-stalls.md`); in this game Test-Games ran clean with it on a window size
new to the driver. **Off:** `native_x_renderer_skip_steady_passes = false` in `ghwor.toml`.

## The depth of field's lattice (NativeXRenderer D127)

At 3440x1440 the background the depth of field blurs showed a grid of fine lines and doubled edges (Gustavo, 2026-11-08,
Create > rocker creator > the character screen); at 1280x720 it was smooth, and turning the depth of field off took it
away. The screen effects' reduced copies of the screen (640x360, 320x180, 160x90) were drawn at the render scale
(860x360 for 320x180), and their blurs step by constants written for the game's texels, so their taps skipped host texels.
The depth of field's blurred copy of the scene (320x180, resolved into `0x8436000`) had the stripes before the composite,
measured in a frame dump. GH Metallica's fix (`unscaled_screen_fractions`, D127) is set in this profile: those copies and
the textures resolved from them are made at the game's resolution, as on the console. It also covers the CoC map (scene
objects drawn at 320x180, `0x84AE000`), the bloom's passes and the ambient occlusion's 640x360 passes. The blur is then as
soft as the console's, and the boundary between sharp and blurred parts has the console's resolution. The characters,
the scene and the HUD stay at the output's resolution.

**Off:** `native_x_renderer_unscaled_screen_fractions = false` in `ghwor.toml`.

The issue's author had said this engine's games (GH5, Band Hero, Warriors of Rock) did not need it; GH5 was checked on
its title screen's bloom only.
