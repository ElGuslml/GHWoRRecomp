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

## Many packages: "Disc Read Error" at start (fixed in 0.1.1-beta.1, SDK change 78)

A player's collection of 190 packages (806 songs) showed the "Disc Read Error" dialog a few seconds after starting,
during "Checking for downloadable content...". The game lists its content one package at a time with the XDK's
asynchronous `XEnumerate`, which ends each call with `ObDereferenceObject` on the enumerator. The SDK looked that
object up through a handle it keeps in a dispatch header, which an enumerator does not have: after the first call it
released whatever object had taken a freed handle, sometimes a file the game had just opened, and the game's next read
of it failed. Now the object is found by its guest address. Reproduced with 190 packages (copies of three): 7 runs of 7
before, none of 5 after. Details: GHMetalRecomp's `local_docs/39-dlc-disc-read-error.md`.

## Which packages the game reads (launcher 3.15.0)

The game's DLC script (`5A4B6A0C`) resolves every content folder as Guitar Hero 5's does: it knows a package by the
QBKey of its display name (lower case, any other character than a letter or digit an underscore, the first 45
characters; `sub_823B0A48`) and reads it if the key is in the content index on its disc (815 keys in 399 entries,
Guitar Hero 5's DLC and Death Magnetic among them: `launcher/dlc_disc_names.h`), in a manifest of another package, or
if the package carries `cmanifest_<key>.pak.xen`. The Megadeth Track Pack has no `cmanifest`; the disc's index names
it. A folder it cannot resolve it reports damaged ("Content package %a is damaged or unreadable"). In Guitar Hero 5,
where the same check turns off every DLC, a player's packages of another game did that (GH5Recomp's `docs/dlc.md`).

GH Warriors of Rock Launcher 3.15.0 checks each installed package the same way (`kDlcRule` in `launcher/game.h`) and
turns off the ones the game cannot resolve: their folders move to `userdata\0000000000000000\41560883\Disabled DLC`,
where the game does not look. Each package has a switch; one turned on by the player stays on. Checked against the
packages at hand (Megadeth's and a Death Magnetic stand-in kept on, Guitar Hero II's turned off), not in the game:
this game logs no request for a missing manifest.

## Turning it off

Turn the package off on the DLC page (or remove it), or move its folder out of `00000002`. `kHasDlcPage` in
`launcher/game.h` hides the page.
