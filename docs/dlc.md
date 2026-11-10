# Downloadable content (DLC)

**Status: working (2026-11-10).** The player's DLC folder: the game reads Xbox 360 content packages where they are
(0.1.1-beta.2 on; before, the launcher extracted each package). A package made for Guitar Hero 5, the Megadeth Track
Pack, adds its three songs to Quickplay+: 113 songs with it, 110 without.

## Your DLC folder (0.1.1-beta.2)

GH Warriors of Rock Launcher's DLC page takes the folder where the player keeps their packages (STFS files: LIVE, PIRS
or CON, in it and its subfolders), as many games' as they like; the other ElGus Recomps launchers can use the same
folder. The launcher reads each package's header and the names at its root, checks it against this game's rule (below,
"Which packages the game reads"), and writes the game's list,
`userdata\0000000000000000\41560883\content_packages.txt`, with the packages on. The game (the SDK's change 80) lists
those packages as its downloadable content and reads them from their files: nothing is copied, written or deleted. The
page groups the packages by the game they were made for, each with a switch (a group's switch turns all of it); those
the game cannot resolve come turned off. The folder and the player's choices are in `dlc_library.txt` beside the list;
the folder is read again each time the launcher opens (`kDlcMode = kFolder` in `launcher/game.h`, launcher 3.17).

Packages installed by an earlier launcher (extracted into `userdata\0000000000000000\41560883\00000002\<package>\` by
`GHWoRRecomp.exe --install_content=<package>`) keep working and show as "Installed copies". One that is also in the DLC
folder (the same file name, or the same name and size) shows once, as the copy the game reads; "Remove copies" deletes
those copies after a confirmation, and the folder's packages take their place.

Checked with a collection of 191 packages on a hard disk (Guitar Hero 5's, World Tour's and the community's): 154 for
this game, each mounted from its file when the game asks (6-11 ms for the community's packs).

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
QBKey of its display name (lower case, any other character than a letter or digit an underscore; `sub_823B0A48`) and
reads it if the key is in the content index on its disc (815 keys in 399 entries,
Guitar Hero 5's DLC and Death Magnetic among them: `launcher/dlc_disc_names.h`), in a manifest of another package, or
if the package carries `cmanifest_<key>.pak.xen`. The Megadeth Track Pack has no `cmanifest`; the disc's index names
it. A folder it cannot resolve it reports damaged ("Content package %a is damaged or unreadable"). In Guitar Hero 5,
where the same check turns off every DLC, a player's packages of another game did that (GH5Recomp's `docs/dlc.md`).

GH Warriors of Rock Launcher 3.15.0 checks each installed package the same way (`kDlcRule` in `launcher/game.h`) and
turns off the ones the game cannot resolve: their folders move to `userdata\0000000000000000\41560883\Disabled DLC`,
where the game does not look. Each package has a switch; one turned on by the player stays on. Checked against the
packages at hand (Megadeth's and a Death Magnetic stand-in kept on, Guitar Hero II's turned off), not in the game:
this game logs no request for a missing manifest.

The name is cut at 42 characters, XCONTENT's display name length: the community's packs with longer names carry the
manifest of their first 42 ("All Nightmare Long - GHWT", "Free Bird Wii Edition"), so the rule uses 42
(`sub_823B0A48` copies 45).

## A different song count at each start: the community's packs need the Customs Database (2026-11-10)

A player with 190 packages, the community's among them, saw 800, 802, 806 and 891 songs in Quickplay+ on five starts
with nothing changed in between. Another player's collection gave 874 every time: it had the community's
"GH:WoR Customs Database" package (the "custom cache"). With that package in the DLC folder the first player's count
stays the same too (the players' check and Gustavo's).

The game resolves its content folders in a random order (script `5A4B6A0C`): it picks an unresolved folder at random
(`sub_823B12F0`, the game's own random numbers seeded from the clock), loads its `cmanifest`, and repeats until none
is left or until a manifest resolves nothing new; any folder still unresolved is marked damaged. The order changes at
every start, on a console too. The Customs Database (`cmanifest_a6844ca7.pak.xen`, `cdl2000000000.pak.xen`,
`cdl2000000000_text.pak.xen`: 955 album and song titles) resolves no other folder: it brings text that the
community's packs refer to. Why the count varies without it is not known: in the collection at hand every package
resolves in any order. Candidates: a file table limited to 17035 entries, filled in resolution order
(`sub_823B21B8`); songs whose titles only the Customs Database defines; song data that loads faster here than from
the console's disk.

Guitar Hero II and III packages are never read: they are not in the disc's index, carry no manifest, and no other
package names them. The game marks them damaged whatever the order, which is why the launcher turns them off.

If this is reported again, ask whether the Customs Database is in the DLC folder. Details:
`local_docs/15-dlc-inconsistent-loading.md` (not published).

## Turning it off

Turn the package (or its game's group) off on the DLC page. Without a DLC folder chosen, the game's list is not
written and the game reads only what is extracted into `00000002`. `kHasDlcPage` in `launcher/game.h` hides the page;
`kDlcMode = kInstall` brings back the page that extracts the packages.
