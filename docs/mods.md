# The Mods tab: the note highway, its pictures, the note effects, the crowd, two sounds, custom textures and mod packs

**Status (2026-11-17):**
- The highway, the crowd and the two sounds: played by Gustavo.
- The highway's pictures (one, or a random one at each song) and the see-through highway: built and checked in songs
  with the keyboard; his test to come.
- The launcher's **Mods** tab (launcher 3.30.0, Gustavo's "mods-galeria" mockup): sections **Highway**, **Stage**,
  **Sound** and **Tools**; the Highway section has the pictures' gallery and a preview drawn from the game's own
  textures. Checked in a scratch install (Spanish); his look to come.

Every option is off by default, which means the game is as on the console, and each one applies when the game starts.

| Row | cvar (`ghwor_options.toml`) | Values |
|---|---|---|
| Highway scale | `ghwor_highway_scale` | 80-120 % in steps of 5; 100 = the console's |
| Highway length | `ghwor_highway_length` | 100, 125, 150 or 170 %; 100 = the console's |
| Highway perspective | `ghwor_highway_perspective` | 25-150 % in steps of 25; 100 = the console's |
| Highway style | `ghwor_highway_style` | `"original"`, `"dark"` |
| Highway opacity | `ghwor_highway_opacity` | 30-100 % in steps of 10; 100 = the console's |
| Folder (pictures) | `ghwor_highway_folder` | a folder; the launcher's default is `Highways` in the game's user data folder |
| Image | `ghwor_highway_image` | `""` (the game's own), a file of the folder, `"<random>"` (one at each song) |
| Random among | `ghwor_highway_random_from` | `"all"`, `"favorites"` (`ghwor_highway_favorites`, names separated by `\|`) |
| Dim bright pictures | `ghwor_highway_dim` | `"off"`, `"auto"`, `"90"`-`"40"`; each picture's own in `ghwor_highway_image_dims` (`name:value`) |
| Infinite front-end (Gameplay) | `ghwor_infinite_front_end` | `false`, the console's; `true` on |
| Sustain drop leniency (Gameplay) | `ghwor_sustain_leniency_ms` | 0-200 ms in steps of 5; 0 = the console's |
| Crowd | `ghwor_crowd` | `true`; `false` hides it |
| Note effects: hit burst, Star Power burst, sustain sparks, open note and kick wave | `ghwor_fx_hit_burst`, `ghwor_fx_star_power_burst`, `ghwor_fx_sustain`, `ghwor_fx_open_note` | `true`; `false` hides it |
| Whammy sound effect | `ghwor_whammy_sound` | `true`; `false` keeps the pitch |
| Lower the track on a miss | `ghwor_miss_lower` | `true`, the console's; `false` never lowers it |
| Instrument volume on a miss | `ghwor_miss_volume` | 0-100 % in steps of 10; 0 = muted, the console's |
| (no row) Custom textures | `native_x_renderer_texture_replacements` | a folder, `"textures"` next to the game |

**How:** each option changes the game's own script data in memory when the game starts, before the game first reads
it (`src/mods.cpp`, through `src/script_globals.cpp`). The game then does the rest with its own code: the multiplayer
highways' scaling, the highway's geometry, and a cheat.

## Highway scale

- **What it does:** with one player, the highway is smaller or larger. It shrinks or grows from the strike line, and
  the gems, sidebars, rock meter and Star Power meter scale with it.
- **How:** it sets the first row of the game's `highway_scale_chart`. That table is what the game uses to shrink the
  highways for two to four players, so the same code applies the new size.
- **The highway's effects** (Star Power, band moments, revival, the Quest characters' flaming rails): the game's
  scripts make them as plain sprites at the highway's full size, in the rails' containers, which the scale moves and
  turns without scaling, at an angle computed with the full height. So they were drawn at the 100 % size and leaned
  off the rails (the console's two to four player highways do the same). `src/highway_effects.cpp` scales those
  sprites when they are made (`CreateScreenElement`) and animated (`SE_SetProps`), and turns the rail ones by the
  angle's error (3.8 degrees at 85 %). This happens only while one highway is on screen, the one-player row being the
  scale's. Checked in Star Power at 85 % (the debug menu's Star Power "Always", a bot): the glows on the rails.
- **Limit:** at 115-120 % the highway's right edge reaches the Star Power bar in the bottom right corner.

## Highway length and perspective

**How the game draws the highway:** in 2D, from its far end at y = 305 to the strike line at 655 of the game's 720,
160 px wide at the top and 512 at the bottom. `GeneratePosTable` (`sub_82182EA8`) cuts it into rows:
- The rows are linear in time. A note crosses the 1024 visible rows in `scroll_time - destroy_time`: 1.5 s on Expert.
- Their heights follow h[i] = (h[i-1] · `perspectivefact`)^`perspectiveexp`, which is ln h[i] = A (e^i − 1).
- The near rows are 9.4 times the far ones, and the width shrinks with the square root of that, as a real camera's
  would.
- Everything at the far end follows the top width: the gems' scale, the beat lines, the sustains.

**Highway length:**
- **What it does:** each note shows up that much earlier, as in Clone Hero. On Expert a note normally shows 1.5 s
  before it is played; at 150 %, 2.25 s.
- **How the extra time is drawn:** the console's highway continues beyond its far end in its own perspective. The
  console's part stays exactly where it was (the rows of the longer time are the console's rows extended), and every
  note reaches the strike line at the console's speed. So at 100 % perspective, 170 % only adds 61 px: the far end is
  at y = 244, 98 px wide, and the extra notes are small and come from far away.
- **What does not change:** the timing to hit the notes.
- **Who it affects:** every player. With two to four players each row of `highway_scale_chart` is changed the same
  way.
- **The time:** every difficulty's `scroll_time` and `destroy_time` are multiplied by the length. That is exact with the
  Hyperspeed cheat too.

**Highway perspective:**
- **What it does:** the projection's compression times p. Below 100 % it is flatter, as seen from higher: the far end
  is wider and its notes are larger. Above 100 % it is steeper.
- **What follows it:** the top width a camera gives with that compression, and the far end's sizes with it. The bottom
  stays the console's.
- **With the length:** a flatter perspective gives the longer part more of the screen. At 50 % with 170 %, the far end
  is at y = 176 and 195 px wide.
- **The notes' speed on screen:** a note takes the same time to cross the highway, but the rows near the strike line
  get shorter below 100 % and taller above it, so the notes arrive slower or faster in pixels:

  | Perspective | 25 % | 50 % | 75 % | 100 % | 125 % | 150 % |
  |---|---|---|---|---|---|---|
  | Speed at the strike line | 0.44 | 0.60 | 0.78 | 1 | 1.24 | 1.51 |

  The highway scale multiplies it too (0.9 at 90 %). The game's Hyperspeed (band lobby, the player's extras) makes up
  for it: on Expert its levels 1 to 5 are 1.25, 1.40, 1.58, 1.82 and 2.14 times the speed. With 75 % perspective and
  90 % scale, level 5 arrives at 1.51 times the console's speed, between the console's levels 2 and 3 (a player's
  report of 2026-11-19: "Hyperspeed does nothing").

**Notes at the console's speed** (`ghwor_highway_keep_speed`, off by default; the switch under the perspective in the
launcher, locked at 100 %):
- **What it does:** the times on the highway (every difficulty's `scroll_time` and `destroy_time`) are multiplied by
  the perspective's speed above, as the length multiplies them by its factor. The notes reach the strike line at the
  console's speed, and the Hyperspeed keeps its levels' ratios.
- **The cost:** below 100 % each note shows for less time. On Expert: 0.67 s at 25 %, 0.90 s at 50 %, 1.18 s at 75 %,
  1.86 s at 125 %, 2.26 s at 150 % (1.5 s on the console). The length gives it back: 75 % with 125 % length is 1.47 s.
- **What does not change:** the highway's picture keeps its look (its tiling follows the length only) and moves with
  the notes; the scale is left out (it shrinks the gems with the highway).
- **Two to four players:** one factor, the one-player highway's (the time is one per difficulty). Their highways' own
  perspectives make their notes 4-7 % faster than the console's at 75 %.
- **The log:** `[mods] highway perspective 75 %: the notes reach the strike line at the console's speed
  (ghwor_highway_keep_speed: the times on the highway x 0.784, 1.18 s on Expert instead of 1.50)`, or with the switch
  off `... at 0.78 of the console's speed`.
- **Checked** (2026-11-19, the bot in a Quickplay+ song on Expert): 75 % off and on, 25 % and 150 % on, 75 % on with
  150 % length; every note hit, 0 errors.

**Limits:**
- The far end stays below y = 50, also with the scale. A length that does not fit is shortened, in steps of 5 %, and the
  log says so: `[mods] highway length 170 % with perspective 25 % and scale 120 %: 155 %, the most that fits`.

**The first version (2026-11-17)** made the highway taller and multiplied only `scroll_time`. The highway looked as
seen from above, its far end too wide, and the notes came about 18 % slower at 150 %, because the crossing time is
`scroll_time - destroy_time`.

## Highway style

- **Dark** is the community's black highway. It changes `highway_normal` (the highway's color, a multiplier on its
  picture) from `[255 255 255 255]` to `[0 0 0 255]`, and `highway_starpower` (the color it returns to in Star Power
  after a miss) from `[64 255 255 255]` to `[64 0 0 255]`.
- **A see-through highway is its own option** (below), and combines with either style.

## Highway opacity

- **What it does:** below 100 %, the venue shows through the highway. The notes, the lines and the sidebars stay
  opaque.
- **How:** the alpha of `highway_normal`, `highway_starpower` and `highway_pulse` (a miss's flash) is set to the
  percentage.
- **The cut-out:** `create_highway_prepass` draws a highway-shaped sprite into the venue's viewport before the venue
  (its window has `viewportrenderpre`, the one script that names it), so nothing of the venue was drawn under the
  highway. A hook where a window element reads that property (`0x82581C7C`, the flag at +604) clears it for these
  windows, and the venue is drawn whole. Checked at 60 %: the floor and the band show through it.

## The highway's pictures

- **What it does:** your own picture on the note highway instead of the game's, from a folder (any size, stretched
  over the highway; PNG, JPEG, BMP or TGA). One picture, or **Random**: a different one at each song, from all of them
  or the favorites. **Dim bright pictures** darkens them toward black so that the notes stand out: for every picture,
  or each one its own (the gallery's Y). Automatic darkens a picture as much as it is bright (its average luminance
  brought down to 0.20, never below 35 % brightness). NativeXRenderer only.
- **How:** `src/highway_images.cpp` asks NativeXRenderer to draw the picture instead of the highway's texture
  (`38E266C86DB5FC0C`) through its D177 `ReplaceTexture`, decoded on a worker thread and darkened there. A song's
  start is `GeneratePosTable` (`sub_82182EA8`, a hook at its entry), which the game calls for each player's highway
  as a song starts: the first call after 3 seconds without one changes a random picture, and the next one is preloaded
  while the song plays.
- **The launcher:** Mods → Highway → Background. Folder (A: a list, open it in Explorer or change it), Image (A: the
  gallery), Random among. The
  gallery lists All (Original, Random, the pictures by name), Favorites and Recent (LB / RB); X marks a favorite, Y
  sets the picture's own darkening, A uses it. The preview at the right draws the highway animated with the game's
  own textures, read from the installed game (`data/compressed/ZONES/z_in_game.pak.xen`, its
  `pak\highway\basic_gems\basic_gems.tex`), with the length, perspective, scale, style, opacity and darkening
  chosen. It is an approximation (the gems are sprites, not the game's), and no file of the game ships with the
  launcher.
- **Moving the game:** the default folder is written whole; a launcher in another place that finds it gone uses its
  own default.

## Crowd

- **What it does:** off, the crowd is hidden during songs.
- **What stays:** the venue, the band and the lights, and the crowd's cheering.
- **How:** through the game's own Soundcheck cheat. At each song's start, `start_gem_scroller` passes
  `$cheat_soundcheckmode` to `options_cheats_set_soundcheck`, and that global is read as 1 in that script. That script
  then calls `hide_crowd_models`, which hides the 2D crowd and sets the 3D crowd's visibility mask to 1.
- **What else sees it:** the global itself keeps its value (-1), so no other script sees the cheat.
- **Why not `disable_crowd`:** that global, which practice mode uses, crashes the game here. The venues' crowd objects
  expect the 3D crowd system that it leaves out.

## Infinite front-end (launcher 3.38, Mods > Gameplay)

- **What it does:** as in Clone Hero and YARG, a hammer-on, pull-off or tap note fretted before it reaches the hit
  window counts as soon as it enters it. A HOPO still needs the note before it hit; a missed note or an overstrum
  clears it, so after a miss the fret is pressed again. Strummed notes are not changed.
- **How** (`src/gameplay.cpp`, `hooks/gameplay.toml`): a player's input in each frame (`sub_82180BA0`) makes a hammer
  candidate only when the frets held differ from the last frame's (player + `0x180D8`), and the notes take it only
  inside their window. Once the frets changed since the last hit or miss, each frame without a strum is made one with
  a change at `0x821814E4`; the game's own checks then build the candidate from what is held. A hit (`0x82181AB8`), a
  miss (`0x82180ED8`) and an overstrum (`0x82181DD8`) end it. Guitar Hero III's code (GH Aerosmith's `sub_821235E0`)
  has the same rule as Warriors of Rock: the players' "GH III infinite front-end" is not in it.

## Sustain drop leniency (launcher 3.38, Mods > Gameplay)

- **What it does:** as Clone Hero's setting of that name, a sustained note's fret let go is still held for that long.
  A sustain let go a moment early to reach the next note, or a fret lifted for an instant, is not dropped; pressed
  again within it, the sustain goes on; a sustain that ends within it counts as held to the end.
- **What it costs:** while a fret is forgiven the sustain scores as if held, at most the leniency's length each time
  (Clone Hero stops scoring until the fret returns).
- **How** (`src/gameplay.cpp`, `hooks/gameplay.toml`): a player's sustain in each frame (`sub_8218ED00`) reads the
  frets held and then ends each sustaining fret not held and, when what is held is not the note, the whole sustain.
  At `0x8218EFC8`, before both checks, the hook adds the sustaining frets let go less than the leniency ago. The open
  note needs no fret and is left out.

## Note effects (launcher 3.37)

- **What it does:** each switch hides one of the effects on the strike line; the notes, their points and the sound do
  not change.
  - **Hit burst:** the flames and sparks of each note hit.
  - **Star Power burst:** the blue burst of each hit while Star Power is on.
  - **Sustain sparks:** the sparks while a sustain is held, and a bass's open sustains' glow.
  - **Open note and kick wave:** the wave across the strike line on an open note or a kick.
- **How** (`src/note_effects.cpp`, `hooks/note_effects.toml`):
  - The bursts: `NoteFX` (`sub_821E63A8`) leaves right after reading `star`, before it emits, as it does after a reset.
  - The sparks: `WhammyFXOn` (`sub_821E6A20`) sees an empty pattern in its loop over the held frets, so no fret's
    effect starts.
  - The waves and the bass's open sustains: `Create2DParticleSystem` (`sub_822B78E0`) answers 0 for their materials
    (`mat_particle_kickhit_*`), as it does when its 44 systems are in use.

## Whammy sound effect

- **What it does:** off, the whammy bar no longer bends a held sustain's pitch.
- **What stays:** the bar still wobbles the note on the highway and still fills Star Power on Star Power notes.
- **How:** the whammy's work on a sustain (`sub_8218C8A8`, called only while a sustain is held) gives the audio manager
  the bend at `0x8218CB3C`. That is `sub_820D6CE0`, which sets the stream's pitch to the lowest of its players'
  values, starting from 1.0. Off, a hook gives it 1.0.
- **Not affected:** a remote online player's bend (`SongAudioSetWhammy`).

## Instrument volume on a miss

- **What it does:** when you miss a note or strum without one, the game mutes your instrument's track until your next
  hit. This keeps that much of it instead: 0 % mutes as on the console, 50 % halves it, 100 % keeps it as it was.
- **Which instruments:** guitar and bass. The drums mute through other code and are not changed.
- **How the game mutes:** the miss scripts (`GuitarEvent_MissedNote`, `GuitarEvent_UnnecessaryNote`) set the player's
  volume to 0 and call `UpdateGuitarVolume`. For each player that calls `sub_820D2A88`, which ramps the player's
  stream to its "on" or "off" target in 0 or 0.02 s.
- **How a miss is told from the game's other mutes** (a Freeplay player sitting out, practice, the first note's window,
  which use the same calls):
  - the last script to read a global on that thread is a miss's script (`src/script_globals.cpp`'s last reader);
  - and the player was on until then.
- **What the hook then does:** the function computes the "on" target, and the target is multiplied by the percentage
  with the console's 0.02 s ramp. The run showed 1.084 → 0.542 at 50 %. The game's scripts still see the volume at 0,
  so the next hit brings it back as on the console.
- **Limit:** with two players on the same part, which share its track through a crossfade table, only 100 % changes
  it.

## Custom textures (NativeXRenderer)

- **What it does:** any image in the `textures` folder next to the game replaces the game texture it is named for.
  This works with NativeXRenderer only (NativeXRenderer's D176).
- **The name:** the texture's hash, 16 hexadecimal digits, then anything. For example `38E266C86DB5FC0C my
  highway.png`. Formats: `.png`, `.jpg` or `.jpeg`. Subfolders are searched too.
- **The note highway's texture is `38E266C86DB5FC0C`:** 512x1024, the console's a dark pattern. An image of any size
  works; the same 1:2 shape keeps its proportions. It repeats along the highway (`ghighwaytiling`, 1.5 times its
  height) and scrolls with the notes, so its top and bottom should join.
- **The alpha:** the console's highway has an edge mask in its alpha (0 inside, bright at the sides), not its opacity.
  An image without alpha draws as expected. What Star Power's look does with it is still to be seen. Opacity is
  `ghwor_highway_opacity`'s.
- **Finding other textures:**
  - `native_x_renderer_texture_log = true` logs each texture's hash, format and size as it loads.
  - `native_x_renderer_texture_dump = true` writes each one as a DDS file named by its hash in `texture_dump` next to
    the game: DXT1, DXT3, DXT5 and 8_8_8_8 textures, 2D only. GIMP and Paint.NET open them.
- **Which textures:** those whose channels are plain RGBA: DXT1/3/5, 8_8_8_8 and the like. Others (1-channel, normal
  maps), cube and 3D textures are drawn as the game's.
- **When the images load:** they are decoded at the start, in the background (4 ms for a 512x1024 PNG). The log says
  `texture ...: ... ready` and, when it is first drawn, `drawn with ...`.

## Mod packs (launcher 3.36.0)

**Status (2026-11-24):** built and tested. The launcher's Packs section checked in a scratch install with a test
pack made for this game (11 patches made into the 11 files of its modified copy, SHA-256 equal). The game with that
pack on: its saves byte-identical to those of the full modified copy of the game data; a pack listed without its
files and lines that are not a folder's name left out with warnings; the command line's cvar used as it is; no list,
nothing changed.

- **What it does:** a player's mod, a folder of files, is read by the game in place of the game's own files at the
  same paths. Mods → Packs lists the packs of the Mods folder, each with a switch; the game's own files are never
  changed, and a pack switched off leaves the game as it was. They apply when the game starts.
- **How:** the launcher writes the packs on to `Mods\enabled.txt`. The game reads it when it starts
  (`src/startup.cpp`, `ConfigureModPacks`, from `OnConfigurePaths`; not in the executable's tool modes) and gives
  their `files\` folders to the SDK's `game_data_overlays` (change 127), whose `HostPathDevice::MergeOverlay` mounts
  each over the game data, a later folder's file over an earlier one's. An overlay's file is read, checked and deleted
  at its own path, and never renamed; every other file of the mount (the saves' too) behaves as before. The log: one
  `[mods] pack <folder> on: its files over the game's (<path>)` line a pack, and a warning for a line left out (a pack
  listed without a `files` folder, a line that is not one folder's name).
- **Nothing to pay without packs:** without a Mods folder the launcher reads nothing and logs nothing about them, and
  without `enabled.txt` the game changes nothing and logs nothing.

### The Mods folder

`Mods` in the game's user data folder (`userdata\Mods` next to the game, or `Documents\ghwor\Mods` where that cannot
be written; the launcher's and the game's choice are the same, as for the DLC folder; a `--user_data_root` given to
the launcher reaches the game, not the launcher), made when a first pack is added. A pack is a folder of it. Folders
whose name starts with a `.` are the launcher's own (a pack being added or replaced) and are not listed; one whose
name starts with `#`, or starts or ends with a space or a dot, is listed as unusable (`enabled.txt` could not name it).

### mod.toml

UTF-8, one `key = value` a line (a string in double quotes with `\"` and `\\`, or a bare value); no tables:

```toml
format = 1
id = "example-pack"
name = "Example pack"
version = "1"
author = "Its author"
description = "What it changes, in a line."
description_es = "Lo que cambia, en una línea."
title_id = "41560883"
```

- `format`: 1. Another is "made for a newer launcher".
- `id`: the folder the launcher installs it as: letters, digits, `-`, `_` and `.`, 1 to 64 of them, not starting with
  a `.`, not ending with a dot, not a device's name (`CON`, `PRN`, `AUX`, `NUL`, `COM1`-`COM9`, `LPT1`-`LPT9`, with an
  extension too), not `enabled.txt`.
- `title_id`: the game's, in hexadecimal: `41560883`. Another is "made for another game", and cannot be switched on.
- `name_<language>` and `description_<language>` (optional): the name and description in the launcher's language, its
  keys `en`, `de`, `fr`, `es`, `it`; `name` and `description` otherwise.
- A folder without a readable `mod.toml` (or one without `format`, `id` or `title_id`) is listed as broken, with its
  folder's name. A `mod.toml` that is there but that another program holds open (an antivirus) is "cannot be read right
  now": the pack is left as it was in `enabled.txt` until the next reading.

### files and patches

- `files\<path>`: a whole file, at the game data's relative path (`files\data\compressed\PAK\qb.pak.xen`).
- `patches\<path>.delta`: a patch that turns the player's own `<path>` into the pack's (below). The pack ships no copy
  of the game's files, only what it changes.
- A pack has either or both. One with patches is **prepared** when it is switched on: each patch is applied to the
  player's file (the game data the game reads: the launcher's `--game_data_root`, else its `assets`) and the result
  written to `files\<path>`. `prepared.txt` beside `files\` records each file made (the patch's size and time, the
  result's size and SHA-256) and the bases found to differ (with their size and time): a prepared file is known by
  its size and that record, without reading it, and a pack updated (another patch) is prepared again.
- A patch refused stops the preparation: the files made before it stay in `files\` (recorded, not made again at the
  next try), and the pack stays off. A pack can be on only once every one of its patches is prepared; one listed in
  `enabled.txt` that cannot be mounted as it is (to be prepared, made for other files or another game, broken, gone)
  is taken out of it when the launcher reads the folder.
- The launcher asks for the free space first (the results' sizes), works on a thread with the row's progress, and a
  press on the row cancels it (between files or within one). A preparation starts by deleting the temporaries an
  earlier one could not end left in `files\` (`*.part<digits>`, only those); such a file is never mounted.
- It never writes through a link: a `files` folder, a folder between it and a result, or a result that is a link (a
  symbolic link, a junction), or a result whose real path is not in the pack's folder, stops the preparation.
- A patch whose `result_size` is over 16 GiB (far above any file of the game) makes its pack broken; sums of sizes
  from the headers saturate.

### Adding a pack

Mods → Packs → Add mod… (a file dialog for a pack's `.zip`, or its `mod.toml` for a folder), or a `.zip` or a folder
dropped on the window. The `mod.toml` is at its top or one folder down; the pack is installed as `Mods\<id>` through
`Mods\.adding <id>`, so that a failure leaves no half pack. Every name in a zip is checked first: valid UTF-8, relative,
each part a name Windows takes (none of `<>:"|?*`, no control character, not a device's name, not ending with a dot or
a space). A folder of that id with another pack (or none) is left as it is; the same pack is replaced once the player
says so. A `.7z` or a `.rar` is refused with a line saying that a `.zip` or a folder is needed.

The zip reader is the updater's (`zip.cpp`): it reads the whole archive into memory and takes no ZIP64 (an archive under
4 GB with fewer than 65535 entries, each under 2 GB), nor encrypted entries. The test pack's zip is 170 MB. An entry
whose size no entry of its compressed size can have is refused before memory is taken, the zip's sizes are checked
against the free space first, and `\` in its names is read as `/` (PowerShell 5.1's Compress-Archive writes them).
Play pressed while a pack is added, or while the Mods folder is read, starts the game once they end.

### Deleting a pack

Delete on a pack's row (or X), asked first: the folder is renamed to `Mods\.removing <folder>` (Windows refuses while
a program has one of its files open, the game for one: nothing changes, and the line says to close it), taken out of
`enabled.txt`, then deleted without following a link inside it (a link is removed, never what it points to). A file
that cannot be deleted leaves the pack out of the list and its `.removing` folder behind; the next deletion tries it
again.

### The patch format (version 1)

All integers little-endian, nothing aligned:

| Offset | Size | Field |
|---|---|---|
| 0 | 8 | `RCMPDLT1` (ASCII) |
| 8 | 8 | `base_size` (u64) |
| 16 | 32 | `base_sha256` |
| 48 | 8 | `result_size` (u64) |
| 56 | 32 | `result_sha256` |
| 88 | ... | the operations, until the result is complete |

An operation is a byte `kind`, then:
- `0x00` COPY: u64 `base_offset`, u64 `length`: that range of the base is appended;
- `0x01` DATA: u64 `length`, then that many bytes, appended.

No count and no trailer: the lengths add up to exactly `result_size`, the file ends there, and no length is 0. The
launcher (`ApplyModPatch`) checks the base's size and SHA-256 before it writes anything, streams the result (the
largest here: a 642 MB base, an 803 MB result) to `<file>.part<process id>` in its folder, and renames it once its
SHA-256 is `result_sha256`. It refuses, leaving no file: another magic, a base of another size or SHA-256 ("made for
other game files", naming the file), a COPY outside the base, lengths beyond `result_size`, a length of 0, another
kind, a patch that ends early or has bytes after its last operation, a result with another SHA-256. Checked against a
reference applier's 20 cases (2 patches applied, 18 refused, each for its reason, no file left).

### enabled.txt

Written whole by the launcher (to `enabled.txt.tmp`, then put in place; a failed write puts the switch back and says
so): a comment line, then a pack's folder a line, in load order (by folder name; a later one's file wins over an
earlier one's). Two packs on with the same file: each row says "replaces files that <the other> also replaces".

```
# The mod packs on (the launcher's Mods > Packs): a folder of this one a line, in load order, a later one's file winning. Read by the game when it starts.
example-pack
```

The game takes a line only as one folder's name: valid UTF-8, at most 255 bytes, none of `/`, `\`, `:` and `|`, no
control character, not `.` or `..`. Another line is left out with a warning.

### The cvar

`game_data_overlays` (the SDK's, change 127): folders separated by `|`, each mounted over the game data in that
order. Which value the game uses:
1. given on the command line (`--game_data_overlays=...`): that one, and `enabled.txt` is not read;
2. else, with `enabled.txt`: the list's packs, even none, over a value of a `REX_` environment variable or of
   `ghwor.toml`;
3. else (no `enabled.txt`): the environment variable's or `ghwor.toml`'s, as for any cvar.

### Online

A pack on changes the game's data; a player online with others who do not have it may see the game behave
differently (the section's note says so).

## Turning them off

Set each row back to its default (Reset mods), or set the cvars to `100`, `100`, `100`, `"original"`, `true`, `true`,
`0` and `ghwor_highway_opacity = 100`; the note effects' switches `true`; `ghwor_sustain_leniency_ms = 0`; `ghwor_infinite_front_end = false`. Custom textures: no image in the folder, or `native_x_renderer_texture_replacements =
""`. Nothing is then changed: the interpreter's hooks do not run for the mods. Mod packs: switch them off in Mods →
Packs (or delete `Mods\enabled.txt`).
