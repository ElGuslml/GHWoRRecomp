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
  color resolve. The game draws its scene over two kinds of frame
  ([the split render](fixes-from-gh5.md#the-split-render)), which copy it into the same 1280x720 texture the screen
  effects' composite samples, from different places: one through the scene renderer `sub_8255A3C8` after the stage's and the crowd's draws (`< 0x8255A994`; that frame shows
  the other's image again), the other the whole scene before the post-processing (`< 0x8264C1B8`) and from inside the
  composite `sub_82641110` after the bloom's passes (`< 0x82641558`). With the first outer caller only, every other
  frame was left as it was (applied 300 times in 600 frames). So no outer caller: every 1280x720 copy through the
  helper is antialiased, 900 times in 600 frames (in the menus too); the helper's smaller copies (the effects' quarter
  sizes, the lobby's 400x400 portraits) are left out by their size. The HUD and the note highway are drawn over the
  composite: they stay sharp.
- **DLAA, DLSS and FSR 3.1: with the split render on, once a pair of frames (NativeXRenderer's D146), DLSS ran smooth
  and without ghosting in a song; the launcher offers them** ([below](#dlaa-dlss-and-fsr-31)).

## DLAA, DLSS and FSR 3.1

**Status (2026-11-11): with the split render on they run once a pair of frames (NativeXRenderer's D146, below). In a
song (`ghwor_027`, `ghwor_028`: DLSS Quality and Performance, CNN, 3840x2160, the RTX A2000) Panchovix saw the motion as
smooth as with SMAA, no ghosting on the band and 60 FPS held, and in one shot a flicker of a building's lit windows far
away, since gone (NativeXRenderer's D148, below). DLAA, FSR 3.1 and the Transformer model ran the same way, once a pair.
Before D146 they ran with the split off, with far fewer artifacts than with the split as it was, though the 3D looked
less smooth. The launcher offers them** (`game::kHasDlaa`: DLAA on the antialiasing row, FSR 3.1 and DLSS on the
Upscaling row, and DLSS's model, `game::kHasDlssModel`).

**The flicker of far lit details (2026-11-11, NativeXRenderer's D148).** A building's lit windows far away (Flyleaf's
Again, the Metallic Lab venue) flickered with DLSS Quality and Performance, the CNN and Transformer models, DLAA and FSR
3.1, with the split on or off and with the depth of field on or off, and never with SMAA: what those have in common is
the jitter. After the upscale point the bloom (from `0x826387C4` on) draws the stage's lit parts again into the scene's
target, testing the scene's depth, which the view drew jittered, and blurs them into its glow: without the jitter a
window drawn over its wall passed and failed that test in turn. The profile's `jitter_after_view`
(`src/native_x_renderer_ghwor.cpp`) gives those draws the view's jitter. Panchovix's A/B in that shot (DLSS Quality,
2026-11-11): with `ghwor_fx_bloom = false`, no flicker and the windows dimmer; with the bloom and D148, no flicker, and
the same windows lit with SMAA, DLAA and DLSS Quality. Two
other leads were ruled out first: the depth of field's blurred copy read the image before DLSS (`< 0x8263A234` through
the scene's 640x360 4x MSAA alias; fixed by D147, the flicker stayed), and 3D draws before the view's start, without the
jitter (none in that song).

**How (NativeXRenderer's D146, 2026-11-10).** The game marks no view (`native_x_renderer::BeginView`):
NativeXRenderer finds it (auto views, D119), from the first depth-writing perspective draw of the output's size to a
copy of the output's size through the render targets' helper's color resolve (the profile's `upscale_resolve_caller =
0x8255AD1C`), where DLSS runs and the copy reads the upscaled image: only the copy from `sub_8264BB88`, the main view's
last stage (`upscale_resolve_outer_caller = 0x8264C1B8`), the whole 3D scene before the screen effects, the note
highway and the HUD; not the composite's copy after the effects (`< 0x82641558`), nor stage 12's or a material's in the
middle of the scene (`0x8264A54C`, `0x82620798`, `0x826207D8`). [The split render](fixes-from-gh5.md#the-split-render)
stays on, as on the console, and DLSS's view spans each pair of frames as the main view does
(`view_pause_outer_caller = 0x8255A994`):
- The stage's frame (kind 1) starts the view at its first perspective draw and draws the stage and the crowd jittered,
  at the reduced scale in the DLSS modes. At its copy `< 0x8255A994` (the helper reached by a tail call from
  `0x8255A990`) the view pauses, without DLSS: that copy, and the depth copy before it (`< 0x826493C8`), go into
  reduced variants of their textures (`0xAACE87D8`, `0xAACE880C`), 1:1, with the depth's words (D64) written. The rest
  of the frame, the last pair's composite redrawn from `[0x829BE118]` and the HUD, is drawn at the render scale.
- The band's frame (kind 2) starts with the reduced targets and the same jitter, restores the stage from those copies
  (the color, and the depth through D113's alias), takes the view up at its first perspective draw with the scene's
  depth bound, writing depth or only testing it (the lights and sprites drawn before the band; the restore binds no
  depth), or, in a shot that draws none (no band), at its copy `< 0x8264C1B8`, and runs DLSS once there; the screen
  effects, the composite and the HUD at the render scale.
- The jitter changes once a pair, DLSS's history goes from one pair to the next (33 ms apart), and the draws' matching
  (D48) and the camera vote (D118) compare each frame's draws with the same kind's two frames back. The main view's
  camera is set in the stage's frame and kept in the band's (a probe of the render thread in a song, `ghwor_017`:
  changed in 449 stage frames, the same in all 447 band frames, alternating): one camera per pair, one vote.
- Limits: the stage's own animation (the crowd) gets the camera's motion only (its motion draws are its frame's), and
  the band is skinned by the CPU into vertex buffers that move every frame, which D48's match by address likely misses:
  the camera's motion only there too, so the musicians' limbs may trail. The band's frame's copy is conditional
  (`0x8264C1A8`): `sub_8252F3E8(..., 4407)` must find one of the screen effects 0x1137 on (motion blur, bloom, god
  rays, tone mapping, the color correction, 0x100, 0x1000) after `src/screen_fx.cpp` takes out those turned off
  ([turning off the screen effects](fixes-from-gh5.md#turning-off-the-screen-effects)). Without it the composite copies
  the scene itself (`< 0x8264146C`), which the profile does not name: DLSS does not run then (the image at the render
  scale without antialiasing; the same with the split off, where before D146 the first copy's rule took that one). A
  pair without it leaves the next one at the render scale, unjittered, and DLSS starts its history again the pair
  after. The main menu has no main 3D view (DLSS ran in none of its frames, with the split off too): its stage's
  frames still pause, without effect. Frame generation (D123) takes each frame's view and with a pair generates
  nothing: with it `src/options.cpp` turns the split off.

The log says at startup "DLSS ...: the main 3D view may span two frames (D146): ..." and every 600 frames "the view
over two frames (D146): paused N times at the first frame's copy, DLSS ... applied M times in the second frame of a
pair (T of them taken up at its copy ...) and K times within one frame"; in a song with the split on, N and M about 300
and K 0, with the usual lines ("applied ... times", D48's matches, the vote) counting per pair. Measured (`ghwor_027`,
`ghwor_028`): 300 and 300 in every 600 frames once under way, none taken up at the copy, the vote in 300 of 300 views,
265 to 355 draws matched per application. The main menu has no main 3D view: there DLSS runs in no frame, as with
the split off. `ghwor_split_render = false` by hand draws the scene whole in every frame, DLSS on each frame as
measured below (K about 600; with D146, 587 to 600 in a song, the 3D less smooth than with the split).

Before D146 `src/options.cpp` turned the split off for these modes: with the split on DLSS ran on both kinds of frame
(the F9 trace of a song with `dlss_quality`, `ghwor_009`): at `< 0x8255A994` on the stage without the band, at
`< 0x8264C1B8` on the whole scene, so its history alternated the two (the band ghosting) and D48 matched no draws (each
frame draws the other half of the scene). With the split off the main view's 19 stages are drawn in every frame and the
finished 3D scene is copied once a frame at `< 0x8264C1B8`; the stage's half-scene copy, the restore and the redraw of
`[0x829BE118]` are gone.

**Measured with the split off** (RTX A2000, 3840x2160, a song, 2026-11-10; `ghwor_012`: DLSS Quality from 2560x1440,
`ghwor_013`: DLSS Performance from 1920x1080):
- DLSS applied, in the 600-frame lines after the one it started in (479 and 484 there), 585, 600 and 600 times in
  Quality and 600 in every line in Performance; D118's camera vote in all of those views but 2 (583 of 585), 114 to 240
  offers each; 214 to 346 draws matched per application (D48).
- Every frame of their F9 dumps (4 frames in `ghwor_012`, 6 in `ghwor_013`) copies at 1280x720 the depth
  (`0x8255AD64 < 0x826493C8`), the scene (`0x8255AD1C < 0x8264C1B8`), the composite (`0x8255AD1C < 0x82641558`) and
  the front buffer (`0x82433274 < 0x82433478`) once each, and nothing at `< 0x8255A994`.
- The A2000 is held by its GPU at that size with the Transformer models (DLSS 310.5.3 in those two runs): 29 ms a
  frame in Quality, of which the renderer waited 20 to 22 ms for the GPU, and 23.5 ms in Performance (13 to 17 ms
  waited). With the CNN model (preset F, DLSS 310.9.16) Performance held 16.67 ms a frame with no wait for the GPU, as
  did SMAA with the split on and with it off (the same song, the same day). Each frame now draws the whole scene, where
  the split drew half of it; not run on a faster card.
- NGX reset DLSS's history 4 times in `ghwor_012` ("NeedHistoryReset 1"), all while the view started (lines with
  frames without it): none at the camera's cuts, so its history ran across every cut of the song. Hence the reset
  below.

**The camera's cuts (2026-11-10, NativeXRenderer's D145).** DLSS and FSR 3.1 fetch the last frames' image
where the motion vectors say each pixel came from; across a cut the camera's reprojection joins two unrelated shots,
and the last one shows through the new one for a few frames (GH Metallica hid DLSS in its 0.3.0 for that). The game
knows its cuts: the cut script `0xA5BB57BB` (CameraCuts_StartCallback; the names are the QB keys of the game's
`qb.pab.xen`) starts each shot of a song. It picks the shot's camera, calls the C function
BandManager_NotifyAllOfCameraCut (`sub_8215F4F8`; it marks each musician, `+57`), then plays the camera with
PlayIGCCam, whose exit script is the cut script again for the next shot. Of the game's 9287 scripts only it calls that
function, once, with no wait, and nothing in the executable calls it but the script functions' table: one call per new
shot, none for a camera's moves within one. `src/camera_cuts.cpp` (three hooks) marks the cut there, carries it with
the frame the main thread hands to the render thread (`sub_82559FD0` at `0x8255A0FC`), and asks NativeXRenderer for
`ResetTemporalHistory` as the render thread takes that frame (`sub_825533F8` at `0x8255343C`), after the last frame's
present and before any of its draws: called from the script itself it could reset the frame the render thread is still
drawing, the one before the cut, and the cut frame would keep the last shot. Two frames are reset, the cut's
and the next: whether the new camera is placed in the frame the script ran in or the next was not traced. With the
split on (D146) a request is held until the pair's application, in its second frame, and the camera is set in the
first frame of a pair only, so three frames are reset, the cut's and the two after it: a cut in a pair's first frame
whose camera is placed in the next frame (the pair's second, which keeps the first's camera) is first upscaled in the
next pair, which two requests would have missed. Either way two applications are reset, the cut's pair and the next.
The log says `[camera cut] N: a new shot ...` for the first five cuts; NativeXRenderer's line every 600 frames counts
the applications reset at the game's request (two a cut): 10 to 12 in each 600 frames of `ghwor_028`'s song, all
asked for by the game. **Off:** `ghwor_camera_cut_reset = false`.

**The motion with the split off (2026-11-10, open).** With the split render off, which DLSS and FSR 3.1 needed here
before D146,
Panchovix saw the band, the stage and the camera's slow moves (a slow approach to a musician) less smooth than with
the split on, on 60 Hz and 120 Hz displays, while the HUD and the note highway stayed smooth. Measured per frame in
the main menu's demonstration song: the game's clock counts 1 vertical blank and a dt of 16.67 ms in 892 of 900
frames; the main view's camera advances an even step each frame (its step over the local median: quartiles 1.000,
1.000 and 1.007, with no correlation with the main thread's frame interval); and the skinned models' vertex buffers
are rewritten with new contents every frame, with no 2-frame pattern. So the 3D advances evenly per game frame, and
the cause is open: the time each frame is shown at is the leading suspect (the notes follow the song's audio clock,
the 3D a fixed step per frame). Hence D146 (above): DLSS with the split on, the console's motion, which Panchovix
played as smooth (`ghwor_027`, `ghwor_028`), and which the launcher now offers.

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

## The render scale in the shaders' constants (NativeXRenderer D128, 2026-11-08)

With the SDK's recomp change 73, the translated shaders read the render scale from the draw's system constants instead
of having it written into them. Before, a window size, internal resolution or FSR 1 / NIS setting not used before gave
the driver new shaders to compile, and its pipeline cache kept a copy of each pipeline per scale; now one translation
serves every scale. The first start after the change compiles every pipeline once. A draw computes what it computed
(GH Metallica's and the other games' stored translations checked, NativeXRenderer's decisions.md D128).
**Off:** `--native_x_renderer_scale_in_constants=false` writes the scale into the shaders as before.
