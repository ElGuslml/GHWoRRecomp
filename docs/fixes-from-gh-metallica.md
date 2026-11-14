# The fixes from GH Metallica of 2026-11-11 to 13

**Status: built, not tried in the game yet (2026-11-13).** GH Metallica 1.0.0-beta.1's game code and launcher, without
its online play, as Guitar Hero 5 got them the same day (GH5Recomp `74d690f`): this game is on the same SDK (`4348a34`),
NativeXRenderer (`c8e3e5c`) and launcher (3.21.1). Each site was found again in this game's own code and scripts (GH5's
map, `tools/port/map2.pkl`, then the instructions compared one by one; the scripts from the disc's `qb.pab.xen`, named
by `dbg.pak.xen`). Where this game differs from GH5, the section says how.

## The game's debug menu

**Option, off by default:** the launcher's Settings → Game → "Game debug menu", or `ghwor_debug_menu = true`. The main
menu gets a "(debug menu)" item after Options and the pause menu a "Debug Menu" item (not in the tutorials):
Neversoft's debug menu (In Game: Play Song, Skip Into Song, Change Venue, Select Camera Cut, the lag tests, Force Score,
Character Select, Complete All Current Challenges; HUD & UI: Show Measures, Show Song Stars, Show Song Time, the band's,
crowd's and stage's visibility; Band Setup; Tools: Check CAS Assets, Dump Heaps, Autosave, Play Credits, Difficulty
Analyzer; Note-Tracking Debug; Screenshot Utilities; Add 100 Quickplay Stars; Strict Small Pools; Unlock All). Its
texts are in English, some items need the developers' tools, and Unlock All and Add 100 Quickplay Stars change the
save. **How:** the game adds these items when the script global `enable_button_cheats` is 1; its startup sets it to 0
on a retail build (`if RetailBuild`, as GH5's). `src/debug_menu.cpp` makes it read as 1 only in the two scripts that
build those menus (`create_main_menu_elements`, `ui_create_pausemenu`); everywhere else it stays 0
(`guitar_startup_systems` would otherwise start the PC viewer, and `cameracuts_iterator` would print "No Cameras/Perf
File for this Song" in some songs). Hooks: the script interpreter's two expression evaluations in `sub_823A4820`
(0x823A49B8 / 0x823A49C0, 0x823A4D20 / 0x823A4D24) and the `$name` resolve at the entry of `sub_8239B040`; the running
script's name is at +244 of the interpreter's script, as in GH5. None of these scripts is one of the 98 the game runs
compiled to native code. The log says "[debug menu] script ... offers the debug menu" for each menu.

## Shorter videos at the start

**Option, off by default:** Settings → Game → "Skip the intro videos", or `ghwor_skip_intro = true`. This game's start
is neither GH Metallica's chain of videos nor GH5's single loading video: it plays its logos (`logo_intro`, 19.4 s) as
the loading screen while it loads, waits for them to end (no button ends them), then plays its intro movie
(`initial_boot_movie`, 53.1 s, Start skips it) unless the developers' global `g_skip_intro_movie` is set, and goes on
to the autosave notice and the main menu. The logos cannot be refused (the loading screen waits for them to play before
the game loads on), so `src/intro.cpp` makes two globals read as 1, through the same hooks as the debug menu: the
developers' `autolaunch_go` in `destroy_loading_screen`, which then kills the logos the moment the game has loaded,
and `g_skip_intro_movie` in `guitar_startup_spawned`, which leaves the intro movie out. The globals themselves keep
their values. The log says "[intro] the game has loaded: the logos end" and "[intro] the intro movie is not played".

## Drums, five or four lanes

GH Metallica's of 2026-11-11 (SDK change 87, launcher 3.19.1; its `docs/drums.md`): Controls → a controller or the
keyboard → Instrument: Drums, then Drum kit: 5 lanes (Guitar Hero) or 4 lanes (the game's Rock Band highway, as with a
Rock Band drum kit on the console). This game has the same code as GH5: `sub_823CF718` makes a drum kit (XInput subtype
8) the game's type 11 and keeps its capability flags' bit 0, `sub_823CF980` turns 11 into 10 (five lanes) at its first
state read unless that bit was set, and the script function `UseFourLaneHighway` (`sub_8217F7F0` / `sub_8217F6C8`)
plays type 11 on a four-lane highway (its four-lane positions in `generate_pos_table`, the `4d` gems in
`setup_button_up_models`, `touch_glow_props_rb_drum`). So launcher 3.21.1's `kHasFourLaneDrums` stays true here, though
its changelog guessed this engine had five lanes only. The keyboard's drum keys are its own (D F J K L, four lanes K
green; Space the kick, Backspace Star Power; `launcher/game.h`). Not played through a song yet. **Off:** Instrument:
Guitar.

## Other things that came with the move

- The mouse cursor hides over the game after 2 seconds without moving it (SDK change 103): `docs/mouse-cursor.md`.
- Settings → Audio (launcher 3.19, SDK change 85: the output device by name, the channels, the volume, mute while
  another window is in front; low latency audio moved there from Game) and Settings → Graphics → the graphics card
  (SDK change 86, NativeXRenderer D141). Nothing of the game's own.
- NativeXRenderer D142 with SDK change 88 (on by default; GH Metallica's `docs/crowd-stutters.md`): a signal the game
  waits for waits only for the renderer's work submitted before it, not for everything queued since (in GH Metallica
  up to 2.6 ms a frame in a crowded venue). **Off:** `native_x_renderer_precise_gate = false`.

## Not ported

- **Online play** (GH Metallica's experimental online modes through Steam): not for this game now. The SDK's
  `online_play` is off by default and the launcher shows no Online category (`kHasOnline` false).
- **The tap notes' trail at ultrawide** (GH Metallica's `GHMetallicaTapTrailHook`): this game draws none, as GH5. Its
  scripts still define `createguitartaptrail` and `destroyguitartaptrail` (`highway_2d.qb`), but no script calls them
  and no code names them (their QBKeys appear nowhere in the executable).
- **The red border over the whole width at ultrawide**: GH5's own fix, already here (`ghwor_ultrawide_overlays`,
  `docs/fixes-from-gh5.md`).
