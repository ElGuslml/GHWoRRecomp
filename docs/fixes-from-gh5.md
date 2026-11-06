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
(`native_x_renderer_present_mode` and `vblank_follows_display`, on by default).

## The guitar and the keyboard

As GH5's: a gamepad reported as a guitar (`gamepad_subtype = 6`), the whammy bar at rest settings, the keyboard as a
guitar (frets D F J K L, strum the arrows, Star Power Space, Start Enter), the strum on each fret only in a song
(`src/keyboard.cpp`, a hook at `sub_82180BA0`, a player's update in a song, which this game too runs only for the
players who play).

## A second controller

`signin_ui_cancel = true` and `signin_guests = true`, as GH5's. The profile following the controller that presses at
the title screen (`src/players.cpp`) is **not active**: this game has no title screen (the intro video, the autosave
notice, then the main menu). The first controller connected plays with the profile.

## Frame rates above 60

**Experimental.** `ghwor_frame_rate` (60, 120, 144, 160, 165, 240; the launcher's frame rate row) sets the vertical
blank's rate; the game's clock is real time. `src/framerate.cpp` (`ghwor_frame_rate_fix`, on) keeps at the console's
speed what counts frames, as GH5's: the vblank clocks, the scripts' game-frame waits, the button presses scripts poll,
the whammy on a sustain and the loading screen's animations. Where this game differs: the whammy's hooks read the note
hold object from another register and its fields moved; the loading star's turn is multiplied by a new speed parameter;
19 scripts that measure their own time by the frame's (GH5: the star power drain only) are left to run every frame, as
their waits would make them slower. Checked at 120 FPS: a song failed with no player at the same point (11 %) as at 60.
**Off:** 60 in the launcher.

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
row sets it. Not checked in this game yet.
