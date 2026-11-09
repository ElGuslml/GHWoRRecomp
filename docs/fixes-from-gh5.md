# The fixes ported from Guitar Hero 5

**Status: in testing (2026-11-01).** Guitar Hero: Warriors of Rock was built on Guitar Hero 5's engine, and
GH5Recomp's fixes (most of them GH Metallica's and GH Van Halen's before) are this game's too: its first boot had the
same problems (5.51 cores busy at the main menu, among others). They were copied with their names changed and every
address translated. This game was built with a newer compiler (XDK 2.0.11164; GH5 2.0.8276), so its code differs in
registers, scheduling, structure offsets and data: each hooked function was found with a function matcher over both
executables' instruction indexes and checked instruction by instruction. Where this game's code differs, the sections
below say how.

Checked here (2026-11-01, by the tools, not yet by a player): the main menu, Quickplay+ and a song on the Xenos path
(the sound not heard), the CPU, the timer fix, the quit prompt, ultrawide at 21:9, 120 FPS, two screen effects turned
off. NativeXRenderer: [native-x-renderer-ghwor.md](native-x-renderer-ghwor.md).

## The threads' waits (CPU)

**Symptom:** 5.51 cores busy at the main menu. **Cause:** the game's threads wait by checking a condition again and
again, which costs nothing on the console: the job system's three workers (`sub_825FF208`), the main thread at the
start of each frame (`sub_8242C450`, `sub_8242ABE8`), the render thread waiting for the vertical blank
(`sub_825840F0`), the main thread waiting for a group of jobs (`sub_82551028`), and the D3D library's waits for the GPU
(`sub_822D5D60`): the same code as GH5's. **Fix:** `src/thread_waits.cpp`, seven hooks. Measured: 1.46 cores at the
main menu, 2.03 in a song. **Off:** `ghwor_wait_smart = false` (slices only), `ghwor_wait_spin_us` /
`ghwor_wait_sleep_us`.

The generated code keeps the registers in local variables (the codegen's `*_as_local` options): setjmp `sub_82798F90`
and longjmp `sub_82798C70`, used only by libjpeg and libpng, share their registers; non-volatile vector registers stay
in the context, as in GH5.

## The audio drifts out of sync during a song

**Cause:** the engine's timer replaces the timebase's 50,000,000 Hz by 49,875,000 (the old consoles' real timebase);
ReXGlue's timebase runs at 50 MHz, so the game's clock ran 0.25 % fast and the audio fell behind the notes. **Fix:**
`src/timer.cpp`, a hook in `sub_823A0238` (0x823A02D4) keeps the reported frequency. **Off:** `ghwor_timer_fix =
false`. `ghwor.toml` also sets `audio_maxqframes = 4`, `audio_callback_min_interval_us = 3000`, `audio_keep_time` and
`audio_thread_priority`, as GH5's.

GH5's third part, FMOD's update on the SDK's audio worker (`src/fmod_mix.cpp`, off by default there because the music
stuttered with it), is **not ported**: the render client it hooks is not in this game, which submits its sound through
another path.

## Low latency audio and V-Sync

As GH5's: GH Warriors of Rock Launcher's Low latency audio (`audio_wasapi_exclusive`, off by default) and V-Sync
(`native_x_renderer_present_mode` and `vblank_follows_display`, on by default). Low latency audio checked by Gustavo
(2026-11-08).

## The guitar and the keyboard

As GH5's: a gamepad reported as a guitar (`gamepad_subtype = 6`), the whammy bar at rest settings, the keyboard as a
guitar (frets D F J K L, strum the arrows, Star Power Space, Start Enter), the strum on each fret only in a song
(`src/keyboard.cpp`, a hook at `sub_82180BA0`, a player's update in a song, which this game too runs only for the
players who play).

## A second controller

`signin_ui_cancel = true` and `signin_guests = true`, as GH5's. A guitar that started the game with a controller
connected before it was the profile's guest. This game has no title screen (the intro video, the autosave notice, then
the main menu), so the window of GH5's fix opens with the game: the first controller to press a button, wherever that
is, takes the profile and the others are its guests. It counts once, for good: a second player moves nothing
(`src/players.cpp`). **Off:** `ghwor_profile_follows_press = false`.

## GH Metallica's of 2026-11-03/05

Moved on 2026-11-05 to GH Metallica's versions: SDK `0a7c86c` (changes 69-72, on its `ghmetal` branch), NativeXRenderer
`6117e82` (D118-D125), RecompLauncher `1ee6325` (3.13.1). The game's side, with this game's addresses:

- **V-Sync with G-Sync or FreeSync** (GH Metallica's `docs/vsync.md`): the SDK's timer, for a display whose vertical
  blanks follow the game's frames, ticks within microseconds (`vblank_timer_spin_us = 700`, SDK change 70), 0.25 % under
  the display's exact rate; the render thread wakes on events the game's swap and vertical blank callbacks set
  (`src/thread_waits.cpp`, hooks at `0x8239FBD8` in `sub_8239FB98` and `0x8239FB80` in `sub_8239FB20`). With V-Sync off
  the game runs 0.25 % under the display's exact rate when the frame rate chosen is that rate (`src/options.cpp`).
  **Off:** `vblank_timer_spin_us = 0`.
- **Alt+Tab from fullscreen** no longer closes the game (NativeXRenderer D125).
- **Each slot's kind of controller follows the one played**: the game asks a port for its kind only when its controller
  connects, and the keyboard shares slot 0 with the first controller. When the SDK finds the slot played on another
  kind, a hook at the entry of the port's update (`sub_823D02A0`) has the game ask again, outside a song
  (`src/players.cpp`). Not tried with a second controller here. **Off:** `ghwor_kind_follows_controller = false`.
- **The keyboard's controls**: Shift, Ctrl and Alt as keys of their own, F1-F12 too; the SDK's developer keys are off
  (`debug_hotkeys = false`; the launcher's Settings > Game > Debug keys turns them on).
- **Guitars**: `guitar_ps3_protocol = true` (a PS3 Guitar Hero guitar, a CRKD guitar on its P3 setting); the launcher
  keeps every controller open and assigns any axis to the whammy bar and the tilt (SDK changes 71 and 72, launcher 3.12
  and 3.13). Not tried with those guitars here.
- **The guitars' strings above 60 FPS**: the game reads their frequency and fade once (`sub_821005F0`) and keeps them,
  so two hooks there (`0x821008E8`, `0x821008C0`) keep the 2 Hz wobble 60 FPS shows of their 62 Hz vibration, and the
  fade 0.99 per 60th of a second. There is no script placing them here (GH5's `0x02B91F04`).
- **The audio's spacing stays**: GH Metallica and GH Van Halen went to `audio_callback_min_interval_us = 0` because FMOD
  mixes on their audio worker. Here FMOD mixes on its own thread, which needs the 3 ms (their own note says so): 0 would
  bring the noise back. SDK change 69 (`audio_keep_time` never drops a frame the device is about to take) comes with the
  move.
- **Not ported**: GH Metallica's Havok step: the executable has no Havok (no `hkp` name in it; GH5 has 639).

## Frame rates above 60

**Experimental.** `ghwor_frame_rate` (60, 120, 144, 160, 165, 240; the launcher's frame rate row) sets the vertical
blank's rate; the game's clock is real time. `src/framerate.cpp` (`ghwor_frame_rate_fix`, on) keeps at the console's
speed what counts frames, as GH5's: the vblank clocks, the scripts' game-frame waits, the button presses scripts poll,
the whammy on a sustain and the loading screen's animations. Where this game differs: the whammy's hooks read the note
hold object from another register and its fields moved; the loading star's turn is multiplied by a new speed parameter;
19 scripts that measure their own time by the frame's are left to run every frame, as their waits would
make them slower; the jam studio's arrows (`0xCFDDF140`) stay held, as in GH5 (they also add 0.1 a pass to the time
that speeds the scroll up, so every frame they would speed up 2.67 times as soon at 160 FPS). Checked at 120 FPS: a song
failed with no player at the same point (11 %) as at 60.
**Off:** 60 in the launcher.

**Camera cuts (2026-11-07, GH5's fix).** The game draws the stage and the crowd in one frame and the band in the next
(`IsPrimaryFrame`), and two scripts align what follows to one kind of frame: `0xCAD261DF` and `0x6506F6C8` (GH5 has
only the first). They and the 22 scripts that call them (the band's clips, a musician changed and changed back a frame
later) go on in whole frames, and held to 60 passes a second their waits fell in either kind of frame: in GH5 each cut
showed 1 to 5 frames with no scene at 160 FPS (a 512x512 square in a corner, then the stage from a camera not yet
placed). They now run every frame (`kFrameExactScripts`): each is one shot or a loop on the song's time, so nothing
runs faster. Here the cut itself calls neither, and Party Play at 160 FPS showed no frame without a scene with or
without this (about 19,500 frames each, a per-frame trace): the scripts run as the game wrote them, a visible change is
not proven. GH5's drummer fix is not needed: this game's drummer script waits a game frame and already ran every frame.

**When the frame rate falls below the chosen one (2026-11-07, GH5's fix).** What the fix counts in frames took a frame
as one vertical blank at the chosen rate: the whammy's 5 frames, the strings' fade (0.99 a frame), the loading screen's
steps, and the margins that pick the frame a script wait ends in and the presses given to scripts. When the game could
not keep that rate (a frame that takes 5 ms at 240 Hz is shown for 8.3), those ran slower until it came back. They use
the frames' real length now, measured once a frame on the main thread (`GHWoRMainFrameHook` at `0x8242C57C`): the last
frame's for the strings' fade (applied where each frame uses it, `0x82100950`: this game keeps the fade it reads once),
the average of about the last ten for the rest; the loading screen's animations measure their own. The whammy's count
of frames in a row is compared with 5 for equality, so each run keeps the number of frames it began with.

**The frets falling back (2026-11-05, GH5's fix).** On a hit the fret heads rise 20 pixels and fall back in 0.1 s. Each
pass of the script `button_checker` (`0x716BADDC`, through `sub_8218BF88`) lowers them by the frame's time, and the fix
above held that script to 60 passes a second: at 160 FPS the fall took 0.27 s in GH5, where it was measured. It now
runs every frame (at 160 FPS here: 59 passes a second held, one a frame now).

## Ultrawide

The same as GH5's (`src/widescreen.cpp`, 9 hooks): every 16:9 perspective is widened (Hor+, `sub_824C6EE0`), and the 2D
layer is narrowed to 16:9 in the center; outside a song black bars cover the sides. This game's clip windows are
scaled by the 2D layer's zoom before the hook sees them, and the sustain tails' constant register is read from the byte
the game writes for it. Checked at 2560x1080: the bars in the menus, the stage over the whole window in a song with the
highway and HUD in the center. **Off:** 16:9 in the launcher (`ghwor_ultrawide = false`); `ghwor_ultrawide_hud =
false`; `ghwor_menu_bars = false`; `ghwor_ultrawide_overlays = false`.

**The black band beside the highway (2026-11-05, GH5's fix).** In ultrawide a soft black band ran along both sides of the
highway; at 16:9 there was none. The game starts its scene with a depth only quad in the highway's shape at the near
plane (`sub_822C53A0`, vertices in pixels of the 1280x720 canvas), so the stage is not drawn under the board. It went
through none of the 2D layer's hooks, so it stayed as wide as the stretched canvas while the board over it was narrowed:
the stage failed its depth test beside the highway, leaving the clear's black, blurred by the screen effects.
`GHWoRHighwayMaskHook` narrows it with the rest (factor 1.34: NativeXRenderer and the Xenos path at 1720x720, in a
Quickplay+ song). It follows `ghwor_ultrawide_hud`, as the board does.

## Turning off the screen effects

**Options, all on by default:** `ghwor_fx_bloom`, `_depth_of_field`, `_grain`, `_color`, `_vignette` (the same bits as
GH5's). This game's composite (`sub_82641110`, two passes of `_Uber_ScreenFX`) does not read the effects' flags itself
as GH5's did: every test of them, the composite's and the effects' own passes', goes through `sub_8252F3E8`, and one
hook there (`src/screen_fx.cpp`) takes the bits of the effects turned off out of each test. An effect turned off then
also skips its own passes, as when none of its instances is on. GH Van Halen's ink blotches are part of this composite
again (bit 0x400): no switch yet. With the color correction off the grain goes too (the launcher's rule). Checked: bloom
and depth of field off -> "effects turned off: flags 0x00A" in a song.

## Singing with a PC microphone

**Experimental, not tried in this game** (GH Metallica's): `usb_microphone_1` and the launcher's Controls tab.

## Quitting from the main menu

B in the main menu asks whether to quit (A quits, B cancels): `src/quit_prompt.cpp`, a hook in the main thread's frame
(`sub_8242C450`) looks up the main menu's element in the game's element table; a dialog box over the menu keeps it
away. GH5's main menu element does not exist in this game: its main menu is `0x55074978`, the one element there and
nowhere else. Checked: the main menu -> B -> the dialog -> B (cancelled); Options -> B goes back to the main menu with
no dialog; the main menu -> B -> A closes the game.

## Characters that never load

The SDK fix GH5 needed (`NtCancelIoFile`, recomp change 65) is in this game's SDK: the band lobby's characters load.

## Language

The game picks its text pack from `XGetLanguage` (English, French, German, Italian, Spanish); the launcher's language
row sets it. Checked by Gustavo (2026-11-08).

## The job queue's lock and wake-ups (2026-11-08, from GH Metallica)

GH Metallica's trace of a song at 160 FPS (its `docs/cpu-usage.md`): the main thread and the three job workers slept
and woke each other tens of thousands of times a second on the job queue's critical section, which the game creates
with no spin count, and each job queued woke all three waiting workers (a pulse of a manual-reset event) for one of them
to take it. The same engine here, so the same two fixes:
- `critical_section_spin_us = 20` (the SDK's change 75, set in `src/startup.cpp`): a thread finding a game lock taken
  watches it for up to 20 us before it sleeps. GH Metallica: 92,000 -> 31,300 switches a second, 1.84 -> 1.66 cores.
- `ghwor_job_wake_one` (on; `src/thread_waits.cpp`, the job event's pulse hooked): one waiting worker is woken per
  job. GH Metallica: each worker 5,300 -> 3,400 wakes a second, 1.98 -> 1.84 cores.
Not measured here yet. Off: `critical_section_spin_us = 0`, `ghwor_job_wake_one = false`.
