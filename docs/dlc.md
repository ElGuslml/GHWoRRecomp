# Downloadable content (DLC)

**Status: working (2026-11-07).** A package made for Guitar Hero 5, the Megadeth Track Pack, adds its three songs to
Quickplay+: 113 songs with it, 110 without.

GH Warriors of Rock Launcher's DLC page installs Xbox 360 content packages (STFS: LIVE, PIRS or CON) with the game
itself: `GHWoRRecomp.exe --install_content=<package>` extracts one into

    userdata\0000000000000000\41560883\00000002\<package name>\

## What was checked

The game's executable names Guitar Hero 5 (`41560840`), World Tour (`4156081A`) and `415607F7` as alternate titles, so
the SDK lists their content too, as the console does. It looks for `b%s_song.pak` in a package, Guitar Hero 5's song
files, and for manifests of its own (`cmanifest_%x.pak`).

- The Megadeth Track Pack, extracted as the page does into this game's folder: Quickplay+'s list shows 113 songs; the
  same route without it, 110. No errors in the log.
- Not tried: playing one of its songs, a package made for Warriors of Rock, and the other alternate titles' packages.

## Turning it off

Remove the package from the DLC page, or delete its folder. `kHasDlcPage` in `launcher/game.h` hides the page.
