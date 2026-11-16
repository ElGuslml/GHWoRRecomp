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
  effects' composite samples, from different places: one through the scene renderer `sub_8255A3C8` after the stage's and
  the crowd's draws (`< 0x8255A994`; that frame shows the other's image again), the other the whole scene before the
  post-processing (`< 0x8264C1B8`) and from inside the composite `sub_82641110` after the bloom's passes
  (`< 0x82641558`). With the first outer caller only, every other frame was left as it was (applied 300 times in 600
  frames). So no outer caller: every 1280x720 copy through the helper is antialiased, 900 times in 600 frames (in the
  menus too); the helper's smaller copies (the effects' quarter sizes, the lobby's 400x400 portraits) are left out by
  their size. The HUD and the note highway are drawn over the composite: they stay sharp.
- **DLAA, DLSS and FSR 3.1: with the split render on, once a pair of frames (NativeXRenderer's D159), DLSS ran smooth
  and without ghosting in a song; the launcher offers them** ([below](#dlaa-dlss-and-fsr-31)).

## DLAA, DLSS and FSR 3.1

**Status (2026-11-11): with the split render on they run once a pair of frames (NativeXRenderer's D159, below). In a
song (`ghwor_027`, `ghwor_028`: DLSS Quality and Performance, CNN, 3840x2160, the RTX A2000) Panchovix saw the motion as
smooth as with SMAA, no ghosting on the band and 60 FPS held, and in one shot a flicker of a building's lit windows far
away, since gone (NativeXRenderer's D161, below). DLAA, FSR 3.1 and the Transformer model ran the same way, once a pair.
Before D159 they ran with the split off, with far fewer artifacts than with the split as it was, though the 3D looked
less smooth. The launcher offers them** (`game::kHasDlaa`: DLAA on the antialiasing row, FSR 3.1 and DLSS on the
Upscaling row, and DLSS's model, `game::kHasDlssModel`).

**The flicker of far lit details (2026-11-11, NativeXRenderer's D161).** A building's lit windows far away (Flyleaf's
Again, the Metallic Lab venue) flickered with DLSS Quality and Performance, the CNN and Transformer models, DLAA and FSR
3.1, with the split on or off and with the depth of field on or off, and never with SMAA: what those have in common is
the jitter. After the upscale point the bloom (from `0x826387C4` on) draws the stage's lit parts again into the scene's
target, testing the scene's depth, which the view drew jittered, and blurs them into its glow: without the jitter a
window drawn over its wall passed and failed that test in turn. The profile's `jitter_after_view`
(`src/native_x_renderer_ghwor.cpp`) gives those draws the view's jitter. Panchovix's A/B in that shot (DLSS Quality,
2026-11-11): with `ghwor_fx_bloom = false`, no flicker and the windows dimmer; with the bloom and D161, no flicker, and
the same windows lit with SMAA, DLAA and DLSS Quality. Two
other leads were ruled out first: the depth of field's blurred copy read the image before DLSS (`< 0x8263A234` through
the scene's 640x360 4x MSAA alias; fixed by D160, the flicker stayed), and 3D draws before the view's start, without the
jitter (none in that song).

**How (NativeXRenderer's D159, 2026-11-10).** The game marks no view (`native_x_renderer::BeginView`):
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
  scale without antialiasing; the same with the split off, where before D159 the first copy's rule took that one). A
  pair without it leaves the next one at the render scale, unjittered, and DLSS starts its history again the pair
  after. The main menu has no main 3D view (DLSS ran in none of its frames, with the split off too): its stage's
  frames still pause, without effect. Frame generation (D123) takes each frame's view and with a pair generates
  nothing: with it `src/options.cpp` turns the split off.

The log says at startup "DLSS ...: the main 3D view may span two frames (D159): ..." and every 600 frames "the view
over two frames (D159): paused N times at the first frame's copy, DLSS ... applied M times in the second frame of a
pair (T of them taken up at its copy ...) and K times within one frame"; in a song with the split on, N and M about 300
and K 0, with the usual lines ("applied ... times", D48's matches, the vote) counting per pair. Measured (`ghwor_027`,
`ghwor_028`): 300 and 300 in every 600 frames once under way, none taken up at the copy, the vote in 300 of 300 views,
265 to 355 draws matched per application. The main menu has no main 3D view: there DLSS runs in no frame, as with
the split off. Gustavo's `ghwor_scene_every_frame = true` ([scene-every-frame.md](scene-every-frame.md)) draws the
scene whole in every frame, DLSS on each frame as measured below (K about 600; with D159, 587 to 600 in a song,
measured then with an earlier switch that held the same mode at 0; the 3D less smooth than with the split).

Before D159 `src/options.cpp` turned the split off for these modes: with the split on DLSS ran on both kinds of frame
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

**The camera's cuts (2026-11-10, NativeXRenderer's D158).** DLSS and FSR 3.1 fetch the last frames' image
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
split on (D159) a request is held until the pair's application, in its second frame, and the camera is set in the
first frame of a pair only, so three frames are reset, the cut's and the two after it: a cut in a pair's first frame
whose camera is placed in the next frame (the pair's second, which keeps the first's camera) is first upscaled in the
next pair, which two requests would have missed. Either way two applications are reset, the cut's pair and the next.
The log says `[camera cut] N: a new shot ...` for the first five cuts; NativeXRenderer's line every 600 frames counts
the applications reset at the game's request (two a cut): 10 to 12 in each 600 frames of `ghwor_028`'s song, all
asked for by the game. **Off:** `ghwor_camera_cut_reset = false`.

**The motion with the split off (2026-11-10, open).** With the split render off, which DLSS and FSR 3.1 needed here
before D159,
Panchovix saw the band, the stage and the camera's slow moves (a slow approach to a musician) less smooth than with
the split on, on 60 Hz and 120 Hz displays, while the HUD and the note highway stayed smooth. Measured per frame in
the main menu's demonstration song: the game's clock counts 1 vertical blank and a dt of 16.67 ms in 892 of 900
frames; the main view's camera advances an even step each frame (its step over the local median: quartiles 1.000,
1.000 and 1.007, with no correlation with the main thread's frame interval); and the skinned models' vertex buffers
are rewritten with new contents every frame, with no 2-frame pattern. So the 3D advances evenly per game frame, and
the cause is open: the time each frame is shown at is the leading suspect (the notes follow the song's audio clock,
the 3D a fixed step per frame). Hence D159 (above): DLSS with the split on, the console's motion, which Panchovix
played as smooth (`ghwor_027`, `ghwor_028`), and which the launcher now offers.

**The upscaler after the bloom and the depth of field (2026-11-15, `ghwor_upscale_after_effects`, off by default).**
At 3840x2160 with DLSS Performance (CNN) on the RTX A2000 the GPU holds the frame (uncapped, about 113 FPS, the renderer
blocked in Present), and NativeXRenderer's pass timer (D162, `--native_x_renderer_gpu_passes=true`) puts the band
frame's composite (`sub_82641110`) first: its pass before the copy `< 0x82641558` (the rectangle drawn from
`0x826414DC`, pixel shader `08C299A3FD3AB2B0`) takes 2.7 ms at 3840x2160, once a pair, and the one after it
(`0x8264161C`, `24980613F7EE48A3`, the color pass) 1.4 ms; DLSS itself 3.1 ms a pair. With
`ghwor_upscale_after_effects = true` the profile's view ends at `< 0x82641558` instead of `< 0x8264C1B8`: the bloom,
the depth of field and that first pass are drawn at DLSS's input size inside the view, DLSS takes their image, and the
color pass, the note highway and the HUD stay at the output's size. Measured in Flyleaf's Again with the bot, uncapped,
one binary (2026-11-15, `ghwor_082` off, `ghwor_083` on): 112.9 FPS off, 129.7 on; the first pass 1.32 ms a frame at
3840x2160, 0.34 at 1920x1080; DLSS still once a pair. What it changes in the picture: DLSS now takes the image after
the bloom, the depth of field and the composite's first pass, instead of before them; the pass after the copy stays
after DLSS. Which of the effects each pass applies was not read. No artifact was seen in screenshots, but none were
of the same frame. Played on 2026-11-15 on the RTX A2000 at 3840x2160 with DLSS Performance (CNN) and the 120 FPS
setting, on and off: the gain is visible, and the bloom and the depth of field, drawn at the upscaler's input size,
look different, which is why it is a switch, off by default. Off: as before.

What the composite's two passes do (read from their microcode, dumped with `--dump_shaders`, 2026-11-15; screen_fx.cpp
names the passes' variants as GH5's `_Uber_ScreenFX` Pass0 and Pass1):
- The first, `08C299A3FD3AB2B0` (drawn from `0x826414DC`, before the copy `< 0x82641558`): a motion blur of three taps
  of the scene's copy (fetch constant 4) along the velocity read from fetch constant 9, the depth of field (the blurred
  copy at fetch constant 3 mixed in by a factor from the depth at fetch constant 1, the focus range in c9 and c10), the
  bloom added (fetch constant 2) and the color grading through a 3D lookup texture (fetch constant 7, `tfetch3D`).
  With the bloom, the depth of field, the grain and the vignette off (`E382DF5F7770B581`) only the motion blur and the
  3D lookup are left.
- The second, `24980613F7EE48A3` (from `0x8264161C`, after the copy, reading it): the motion blur again (three taps of
  the copy along the same velocity) and the film grain, a hash of the screen position (`dp2add` with c110, `frcs`)
  mixed in with the color and amount in c3; it also fetches fetch constant 0 into a register it overwrites before use.
  With the grain and the vignette off (`1686D6EB88E37034`) only the motion blur is left. No vignette term was seen in
  this variant (the shot had none on).
So with `ghwor_upscale_after_effects` DLSS is given the image after the first motion blur pass, the depth of field, the
bloom and the color grading, and the film grain and the second motion blur pass come after it, at the output's size:
DLSS does not smooth the grain. It does see the motion blur (NVIDIA advises it after the upscaler) and an image already
graded, where without the option it got the scene before both.

**The launcher's rows (2026-11-15).** Settings > Game has two rows, both off by default: "Screen effects before the
upscaler" (`ghwor_upscale_after_effects`) and "Even frame times with DLSS" (NativeXRenderer's D164,
`native_x_renderer_upscale_late`: DLSS evaluated at the start of the next frame, DLSS upscaling only, not with frame
generation; with both on at 3840x2160 and the 120 FPS cap the RTX A2000's band and stage frames took 7.0 and 6.7 ms of
GPU instead of 9.4 and 4.8). The two are independent switches: the late evaluation does not need
`ghwor_upscale_after_effects`, and `src/options.cpp` logs an `[options]` line saying which way it runs. With an
antialiasing other than DLSS upscaling it logs that the setting is ignored, and with `ghwor_scene_every_frame = true`
(or frame generation, which sets it) it turns it off. Without `ghwor_upscale_after_effects` the bloom and the depth of
field are drawn from the newer frame over the 3D image of the pair before, so by the reasoning the glow around lights
and the blurred background can sit off the objects while the camera moves, and a camera cut shows one pair of the old
shot under the new shot's glow. Played on 2026-11-15 on the RTX A2000 at 3840x2160 with DLSS Performance (CNN) and the
120 FPS setting, each row on and off: the gain is visible, and the late evaluation alone looked right, that offset was
not noticed.

**DLAA left out (2026-11-16, NativeXRenderer 0237995).** With DLAA and the late evaluation on, the 3D image fed back on
itself within seconds: in a song at 3840x2160 with the 120 FPS setting on the RTX A2000 it went to flat colors about
three seconds in and then to a flat light grey that never came back, with either model and with the record thread on or
off. DLAA has no reduced targets, so its input is the scene's own target, also its output, which the frame draws into
after the upscale point, and the late evaluation read the frame's last image instead of the view (a diagnostic build
counted a clear and about 225 draws into the held color after every upscale point). No run before had DLAA with the late
evaluation on (the rows were played with DLSS Performance), and NativeXRenderer now applies it only to a DLSS mode that
upscales and logs that it is not applied with DLAA.

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
- **Shadow quality** (the launcher's row, NativeXRenderer's D92; GH5's `cdec72f`): on since 2026-11-13 (below).

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

## Shadow quality (NativeXRenderer D143, D144, 2026-11-13)

The launcher's Shadow quality row (Original, High, Ultra: `native_x_renderer_shadow_scale` 0, 2, 4; D92) is on. In a
song NativeXRenderer finds GH5's shadow map, the 400x400 depth surface at EDRAM base 1709 (1600x1600 at Ultra). Two
things kept the row off before:
- **The filter.** The characters' shadows are read by pixel shaders that take nine samples of the map at fetch constant
  5, each compared with the pixel's depth, at offsets from c10-c13 = +-1 / 400 (seven shaders in a song's dump,
  `5C47D288D8AA5C4E` and six more). With the map four times as large those offsets put the samples four host texels
  apart: stepped copies of each edge, the lattice GH5 showed on skin. GH5 divides them in its renderer's code
  (`GH5ShadowFilterHook`); this game has no such function (the map's size and texel, 400.0 and 0.0025, are constants of
  the image at 0x8200FEF0). The profile lists c10-c13 with fetch constant 5 (`src/native_x_renderer_ghwor.cpp`), and the
  library divides them by the map's host size over its guest size at each draw (D143; the log: "the pixel shader's
  c10-c13 hold sample offsets in texels ... (-0.0025 = -1.00 texels of its 400x400): divided by 4.0000").
  `native_x_renderer_texel_offset_constants = false` keeps the game's offsets (A/B).
- **A false shadow map.** A 640x360 depth pass drawn with no color target (EDRAM base 1488, from the boot on) was taken
  for one at the render scale 1 (a 1280x720 window); above it D127 keeps that surface native. D144: a reduced copy of
  the screen is never a shadow map in a game with `unscaled_screen_fractions`.

Checked: 1280x720 and 2560x1440 with the shadow scale 4, Quickplay+'s first song, no error, the images normal. A
110x207 depth surface at base 1488 (pitch 160) is also drawn at the shadow scale in songs, as before (what it is not
known). Not judged on screen: the shadows' edges at Ultra against Original (Quickplay+'s venue is random and dark):
Gustavo's eye.

## Pixel shaders specialized per draw (NativeXRenderer D146, 2026-11-15)

The translated pixel shaders read each texture's sign modes, swizzle and render-scale bit (and the flags) from the
system constants at run time and sample every texture twice to pick one view. D146 compiles, behind each pipeline, a
variant with those values made constants and draws with it once ready (`native_x_renderer_specialize`, on; false for
the old behaviour). Measured in GH Metallica first (captures, a pyro song: GHMetalRecomp's `local_docs/54`). Here, the
main menu's animated background at 2560x1440 (~900 draws, 160 FPS, SMAA): the GPU 3.61-3.69 -> 2.75-2.83 ms a frame
(-23 %), peaks 4.0-4.3 -> 2.9-3.4 ms, every draw on its variant (224 pipelines from 114 specialized shaders), the
images the same; Test-Games (`runs/2026-11-15_ghwor_d146`): both renderers OK, the observer 115/115, no error.

## Old texture and resolve memory watched as any other (NativeXRenderer D147, 2026-11-15)

The game reuses memory that held textures or its bloom chain's resolves (allocated anew each frame) for vertex
buffers, and SetStreamSource stamps a buffer's fence there at every unbind. NativeXRenderer kept such pages always
watched (D97), so in a busy song with the scene every frame each stamp faulted and each request protected the page
again: ~240 faults and ~295 re-protections a frame. D147 watches them as any other page once no texture asks for them
and no resolved texture covers them. Gustavo's Quest song (How You Remind Me) at 160 FPS, 3440x1440: faults 239 -> 45
a frame, NativeXRenderer's thread 5.70 -> 4.76 us a draw, the slowest stretch 146 -> 156 FPS; Test-Games OK.

## Textures loaded again for their packed tail's free space (NativeXRenderer D148, 2026-11-15)

After D147, ~22 textures a frame were still loaded again in that song, each with its image unchanged: the game keeps
objects in the unused blocks of a texture's packed mip tail (the 32x32-block tile that holds its smallest levels), and
SetStreamSource stamps their fences there, so the hash of the whole mip range changed every frame. D148 hashes only the
bytes the image is made of: loads 22.4 -> 1.0 a frame, NativeXRenderer's thread 4.92 -> 4.56 us a draw (-7.3 %), the
slowest stretch 156 -> 159 FPS; Test-Games OK.

## 240 FPS: the renderer thread (NativeXRenderer D149-D151, 2026-11-15)

At 240 FPS in ultrawide with the scene every frame, NativeXRenderer's thread was the limit in busy songs (4-5.4 ms a
frame against 4.17). A third of it went to checking the memory pages the game rewrites every frame (vertex data, and
pages holding the fences SetStreamSource stamps) and copying the changed ones. D149 drops the barrier DiligentCore put
between every two of those copies, D150 splits the checks with up to three worker threads
(`native_x_renderer_memory_check_threads`), D151 skips a draw's vertex requests its batch already made. Gustavo's
Quest song at 240 FPS: 220.7 -> 232.4 FPS average, the slowest stretch 174.5 -> 201.6, no stretch under 200; the
results screen at 239; at 160 FPS the song holds 160. Test-Games OK. What is left is mostly DiligentCore's and the
driver's cost per draw.

## 240 FPS: the index requests and the fence stamps (2026-11-15, branch `perf-240`)

The game's fence stamps are written where the page watches do not see them ([fence-stamps.md](fence-stamps.md)):
the song at 240 holds 239 FPS on average, its slowest stretch 225. NativeXRenderer D152 (SDK change 116) skips a
draw's index request when its batch made it: +4.7 % FPS at 480 with V-Sync off (measured inside one run). With V-Sync
off the song runs at 310-330 FPS (260 before these changes); the renderer thread is still the limit, the GPU at ~70 %.

## The renderer in two threads (NativeXRenderer D153, D154, 2026-11-15, branch `perf-240`)

NativeXRenderer's calls to DiligentCore go through an ordered queue (D153), and a thread of their own records them
while the renderer's thread prepares the next commands (D154, D155: larger batches, the frame's Present on that
thread). On by default in this game since 2026-11-16 (`src/startup.cpp`, after Gustavo played with it at 160 and 420);
`native_x_renderer_record_thread = false` in `ghwor.toml` turns it off. Gustavo's Quest song with the venue every
frame at 3440x1440: at 240 FPS every stretch at 230 or more (the slowest 233.6, 225.7 without it); with V-Sync off at
480, 319 -> 370 FPS on average, the GPU then 88 % busy. Test-Games OK with it on and off.
