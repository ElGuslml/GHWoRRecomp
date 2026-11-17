# The venue at the full frame rate

**Status: built, off by default (2026-11-15); at 160 FPS busy songs can drop to about 130 (below).**

**What the console does:** Guitar Hero: Warriors of Rock (and Guitar Hero 5, the same engine; not GH Metallica, GH Van
Halen or GH Aerosmith) draws its 3D scene over two frames. In one frame the render stages up to the "split" one run and
the scene is copied; in the next the stages after it run over that copy, and each scene's mask names the frames it is
drawn in. In a song one frame holds the venue and the crowd, the other the band and the screen effects. The engine
calls this the split render mode (`SetSplitRenderMode`, `IsPrimaryFrame`). So the 3D image changes 30 times a second on
the console at 60 FPS, while the note highway and the HUD are drawn in every frame. Here the split counts frames too:
at 120 FPS the venue moves at 60, at 160 at 80.

**The option:** Settings > Display > Smoothness > "Venue at the full frame rate" (`ghwor_scene_every_frame`, off by default). On, a hook
at the start of the main thread's frame (`src/split_render.cpp`, `0x8242C46C` in `sub_8242C450`) writes the mode to 0
in both copies of the render state (`0x829C0FD8`, the main thread's; `0x829BBAB8`, the render thread's; 1 from the
renderer's init, `sub_8242B988`). The engine then takes its own path without the split: every render stage of every
scene in every frame, `IsPrimaryFrame` always true. The log says `[split render] off: the 3D scene is drawn in every
frame`.

**Measured** (Party Play, the mode written to 0 in a running game, then the build with the option): every frame about
1060-1090 indexed draws (468 and 416 in turn with the split), 0 failed draws, the image whole on both renderers, SMAA
on both scene copies of each frame (1200 in 600 frames, was 900). At 160 FPS and a 3440x1440 internal resolution:
155-159 FPS, the process 2.60 cores (1.74 with the split), NativeXRenderer's thread 80 % of a core (44 %), the GPU about
65 % (about 42 %). About twice the 3D work a frame: on a slower PC the frame rate can fall, hence off by default.

**At 160 FPS in a busy song** (2026-11-15, Gustavo's song on an i7-13700K and an RTX 4070 Ti, 3440x1440, SMAA): with
the option the song's frames held 973 draws (1,498 at most) and NativeXRenderer's thread took 5.5 ms a frame (7.2 at
most, ~5 us a draw) of the 6.25 ms a frame at 160 FPS: 11 of 31 ten-second stretches fell below 156 FPS, the worst to
132. The same song without it: 484 draws (665), 2.9 ms (3.8), 158-160 FPS throughout. The GPU had room both times
(3.5 and 1.9 ms). The limit is the processor's thread that draws, not the graphics card: at 120 FPS a frame has 8.33 ms,
more than the heaviest stretch took (not measured at 120). The launcher's help and the players' README say so.

**Off:** the launcher's switch, or `ghwor_scene_every_frame = false` in `ghwor.toml` (the default).
