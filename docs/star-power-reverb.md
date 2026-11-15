# Guitar Hero World Tour's Star Power sound

**Status: played, off by default (2026-11-15).** On 2026-11-15 songs were played with the option on and the Star
Power echo sounded right by ear. Before that it was checked in the game (see "What was checked"): the stack works as
World Tour's, and with World Tour's settings its sound is about 13 dB under the mix. Two settings raise it (see
"Settings to try"). **Option, off by default:** the launcher's Settings → Game → "World Tour's Star Power sound", or
`ghwor_star_power_reverb = true` (read once at the start).

**Symptom:** in Guitar Hero World Tour, a player's Star Power puts his own instrument through an echo timed to the
song, a reverb and a high pass, set for each instrument and for one player or several. In Warriors of Rock, Star Power
only raises the player's send to the venue's reverb, with no echo. **Cause:** Warriors of Rock still has World Tour's
scripts, under new names, and the three effects' settings, but no longer creates World Tour's effects nor sends to
them. Its engine has everything World Tour used. **Fix:** `src/star_power_reverb.cpp` creates World Tour's effects at
the boot and gives Star Power World Tour's settings, through the engine's own functions. **Off:** the option off
(`ghwor_star_power_reverb = false`): every hook returns at once and the game runs as on the console.

## What World Tour did

Read from World Tour's `default.xex` and its scripts (`qb.pab.xen`, named by its `dbg.pak.xen`); Warriors of Rock's
scripts from its own disc.

- **The effects.** `preload_song` (`scripts\guitar\guitar_song.qb`) creates a send stack for each song on
  `starpower_sendeffect_slot` (1), except in the jam session and the training: `CreateEffectSendStack effects =
  [Starpower_Echo, Starpower_Reverb, HighPass_StarPower]`, one after the other. `end_song` destroys it.
  `PauseGh3Sounds` and `UnpauseGh3Sounds` mute and unmute it. The three structs
  (`scripts\game\audio\audio_effects_settings.qb`):
  - `Starpower_Echo`: echo, name `starpowerecho`, delay 600, DecayRatio 0.15, MaxChannels 6, drymix 1, WetMix 1.
  - `Starpower_Reverb`: sfxreverb, name `starpowerreverb`, DryLevel -10000, Room -100, RoomHF -698,
    RoomRollOffFactor 10, DecayTime 3.24, DecayHFRatio 0.33, Reflectionslevel -1166, ReflectionsDelay 0.02,
    ReverbLevel 18, ReverbDelay 0.05, diffusion 100, Density 100, HFReference 5000.
  - `HighPass_StarPower`: HighPass, name `starpowerhighpass`, cutoff 250 Hz, resonance 1.
- **Star Power on.** `GuitarEvent_StarPowerOn` calls `GH_Star_Power_Verb_On` (`0xDF84B375`,
  `scripts\game\audio\global_sound_logic.qb`). It sets the echo's delay from the song's tempo (`get_song_tempo_cfunc`,
  a beat in ms): 1.5 beats if that is 600 ms or less, else the beat halved while above 400 ms, three times at most,
  and 400 at most. Then the echo's mix and the reverb (with `time = 0.1`) by the player's instrument, and the send of
  the player's own stream, at once (`SongSetStarpowerEffectsSendVolume`, 2^(dB / 6), at most 4).
- **Star Power off.** `GH_Star_Power_Verb_Off` (`0x4CB84F65`) cuts that send (-100 dB) and clears the flag
  `Star_power_verb_is_on`, which the on sets.
- Only the player's own instrument goes through it: not the band, not the crowd. The vocals have their own settings
  (`VocalDSPSetParams`), the same in Warriors of Rock.

World Tour's settings for each case (the echo: WetMix, drymix, DecayRatio; the reverb: DryLevel, ReverbLevel,
Reflectionslevel, Room, RoomRollOffFactor, ReverbDelay, DecayTime, RoomHF; the send in dB):

| Case | Echo | Reverb | Send |
|---|---|---|---|
| Guitar, single player | 1, 0.7, 0.25 | -1500, -500, 0, -1000, 18, 0.1, 0.1, -1 | +4 |
| Bass, single player | 0.3, 0.7, 0.25 | -1400, 900, 0, -2000, 18, 0.1, 0.1, -1 | +8 |
| Drums, single player | 0, 0.2, 0 | -2000, 1700, -10000, -2000, 18, 0.1, 3, -1 | +12 |
| Guitar, multiplayer | 0, 0.2, 0 | -1000, 2000, 1000, -2000, 18, 0.1, 3, -500 | +6 |
| Bass, multiplayer | 0, 0.2, 0 | -1000, 2000, 1000, -2000, 18, 0.1, 3, -500 | +8 |
| Drums, multiplayer | 0, 0.2, 0 | -1000, 2000, 1000, -2000, 18, 0.1, 3, -500 | +7 |
| Guitar, multiplayer, flag already on | 0, 0.2, 0 | -1000, 2000, 1000, -2000, 18, 0.1, 3, -500 | +6 |
| Bass, multiplayer, flag already on | 0.3, 0.7, 0.25 | -1400, 900, 0, -2000, 18, 0.1, 0.1, -1 | +8 |
| Drums, multiplayer, flag already on | 0, 0.2, 0 | -1000, 2000, 1000, -2000, 18, 0.1, 3, -500 | +7 |

With the flag already on in single player, the script changes nothing. So a single guitarist hears mostly an echo on
the beat with a short reverb, a single drummer a 3 second reverb, and several players a large 3 second reverb with no
echo. The stack is one for all the players: the echo's and the reverb's settings are the last activation's, the send
is each player's.

## What Warriors of Rock does

The same event calls `audio_gameplay_star_power_verb_on` (`0x0FF8F452`) and `audio_gameplay_star_power_verb_off`
(`0xE9628649`). The first still works out the echo's delay and does not use it; both call
`SAP_SetVenueEffectSendVolume player = ... vol = ...`: 0 dB for the guitar, -10 for the bass, -3 for the drums, -100 at
the end (not in the jam session, only while `playing_song` is 1). The three structs are still there with World
Tour's values (without their names), and no script of the disc uses them. The only send stack is the venue's, created
once at the boot on slot 1 (`audio_effects_create_settings_by_venue`, a single sfxreverb named `VenueVerb`) and never
destroyed.

The engine is FMOD Ex linked into the executable, so all of it runs as recompiled code; the SDK only receives the
mixed sound. It has the echo, sfxreverb and highpass types (the table at `0x828587D8`), two send stack slots
(`0x829939A0`, 12 bytes each: the stack's first effect, its return connection, its return volume), of which slot 2 is
never used, and two sends on every track of the song player (SAP).

## How the option works

Five hooks (`ghwor_functions.toml`), each a `[[midasm_hook]]`:

- **The stack, at the boot** (`0x82512980`, in `sub_82512900`, the script function `CreateEffectSendStack`, after it
  created a stack; `r30` = its slot). When the boot creates the venue's stack on slot 1, World Tour's is created on
  slot 2 through the same script function, with World Tour's three structs and names. Before, the effect pool
  (`0x82858D40`, 64 nodes) must have three free nodes: with it full, the engine goes on with no node and writes and
  reads through it. That check covers the stack's own three nodes only, which it then keeps: the song player takes
  its nodes later (at its init and at each song's load: a mixer for each track, its effect chains, two more for each
  guitar or bass player), and sounds and busses take theirs from the same pool. So the pool's peak is followed from
  then on, with a warning when it leaves three nodes or fewer (see "The log"). After, each of the three effects must
  be there with its FMOD DSP
  (FMOD's own memory cannot be checked before), and the stack's return connection. Otherwise the option stays off
  for the session, with the reason in the log.
- **Star Power on and off** (`0x820D7148`, the entry of `sub_820D7148`: `r3` = the SAP, `r4` = the player, `f1` = the
  send's volume). Its one caller is `SAP_SetVenueEffectSendVolume`'s call with a player, which only the two scripts
  above make. The player's tracks are chosen as it chooses them (the stem object's track for guitar and bass, the
  SAP's kick, snare, tom and cymbal tracks for drums). On (`f1` above 0): World Tour's order, the echo's delay, the
  echo's mix, the reverb, through the engine's script function `SetSoundBussEffects` (`sub_82512818`), then slot 2's
  send on the player's tracks at World Tour's level (`sub_820D95C0`, at once); the venue's send is left as it was.
  The echo's delay comes from player 1's beat in ms (`[player + 340]`, what `get_song_tempo_cfunc` hands the script as
  `beat_duration`; its `tempo` is 60000 / it), and is left as it was when that beat is 0.
  World Tour's flag (the "already on" presets) is kept as World Tour does, except for the vocals (see "Limits"). Off:
  slot 2's send cut on the tracks the player's Star Power sent, then the game's own call (the venue's send to 0, as on
  the console).
- **The send applied at once** (`0x820D9688`, in `sub_820D95C0`). After storing a slot's new send, the engine applies
  it at once with the next slot's volume (`rlwinm r11,r29,5` then `lfs f1,76(r11)`): slot 1's with slot 2's, slot 2's
  with a word past the track. On the console slot 2 is always 0, so slot 1's send is applied at once with 0 (its
  connection removed). The frame's update (`sub_820D96B8`) makes it again with the right volume on the tracks it is
  called for, which leaves out two: the guitar and bass tracks that `SAP_SetVenueEffectSendVolume track = guitar` or
  `bass` reaches (SAP + 372 and SAP + 504). The venue send the freeplay snapshots give those two (house: 0 and +2 dB,
  stage: -11 and -5 dB) is never applied on the console, and stays so: with slot 2 at 0, the engine's read is left
  alone. Slot 2 is used only on the tracks a Star Power sends (the stems' and the drums', updated each frame). There,
  with the engine's read, a slot 1 send set during a drums' Star Power (a freeplay snapshot) would take the drums'
  Star Power level into the venue's reverb for a frame, and a track's destruction (`sub_820D9878`) would leave its
  connections on. With the option on and the stack there, the volume comes from the slot being set: for slot 2
  always, for slot 1 while slot 2's volume is not 0.
- **The song's clean-up** (`0x820D6B30`, the entry of `sub_820D6B30`, which destroys the players' stems). It runs before
  a song is loaded, at the song player's init and at its deinit. `SAP_Unload`, at a song's end, does not reach it.
  Slot 2's send is cut on every track a Star Power sent that is still there (a song can end with Star Power on: the
  scripts that end it can skip the off). At a song's load that is before the stems' tracks are destroyed (the clean-up
  itself destroys them), and the drums' tracks live on from the init to the deinit. At the init the drums' tracks, and
  at the deinit all of them, were destroyed before it: their destruction removed their sends (`sub_820D9878`, through
  the fix above). World Tour's flag is cleared (World Tour's `start_gem_scroller` clears it at each song), and slot 2's
  return is muted so that what is left of the echo and the reverb does not ring into the next song. The next Star Power
  unmutes it, as World Tour's stack started each song anew. The effect pool's peak is checked here too.
- **The pause** (`0x823BECF0`, the entry of `sub_823BECF0`, `MuteEffectSend` and `UnMuteEffectSend`). When the game
  mutes or unmutes the venue's slot, slot 2 follows (an unmute waits for the next Star Power after a clean-up).

The structs are written once into guest memory from the SDK's system heap, laid out as the script compiler lays out a
struct in a script (a struct's header, then each component: its type, its key, its value and the next component, an
array's structs after its own component), with their pointers already absolute. The game's code is called from the
hooks only when they run on the thread's own context, on that context below the live frames, as the SDK runs an APC
(the context handed back as it was after), and only after the pointers it is given were found in the game's memory.
Anything not as expected leaves the game's own behaviour, with a line in the log.

## What was checked

**The first run** (guitar, one player, 120 FPS, `logs\ghwor_060.log`, 2026-11-15). The stack was made at the boot (62
effect nodes free before it, the pool's peak 26 of 64 in the songs), and each Star Power sent the guitar to it and cut
the send at its end. By ear: during Star Power the sound changed, somewhat stronger, as in World Tour, but no echo and
no reverb stood out, and World Tour's effect is remembered as much more noticeable.

What was wrong: the echo's delay. `[player + 340]` is the length of the song's beat in ms (about 441, 385 and 616 in
that run's songs), which `get_song_tempo_cfunc` hands the script as `beat_duration` (World Tour's cfunc does the same).
The hook took it for a tempo in BPM and worked from 60000 / it: delays of 146 to 236 ms, a short slap that blends into
the guitar, where World Tour's rule gives 220, 577 and 308 ms for those songs. Fixed: the delay now comes from the
beat itself, and the log gives the beat in ms and its BPM.

**Read in the engine's code** (Warriors of Rock's against World Tour's) and found right:

- The stack's wiring with three effects, never used by this game (the venue's stack has one). `sub_823BBBB0` keeps
  the effects in the struct's order (+532 links each to the next), `sub_823BEB68` connects each to the next (the next
  one's `addInput` of the one before: echo, sfxreverb, high pass) and the last to the return, and the send goes into
  the first (`sub_823BEE00`). So a track's sound goes echo, sfxreverb, high pass, then the return into the head of the
  Master buss (`[0x82992CBC]`, the buss named `Master`). World Tour's `0x822C5720` and `0x822C59B8` do the same.
- The settings reach FMOD: each name maps to FMOD's index for its type (the tables at `0x82858A48` and `0x82858C80`,
  their names filled at the start), integers are read as floats, `time = 0.1` fades the reverb's settings over
  100 ms through a list (`0x829938D4`, 64 entries) the sound system's update carries on each frame, and with no time
  a setting is made at once. An sfxreverb's settings reach its reverb through a request FMOD carries out on its next
  pass, its dry level at once.
- Each DSP is made active, not bypassed, with all its speakers on (`sub_823BBD70`), and the return's volume is 1.
- World Tour's Star Power on did nothing more for the sound than this: the rest are sound events and claps.

The reading did not settle the level, nor whether the stack processes at all (the echo passes its input through
untouched when it has more channels than it was set for, and connections are made by FMOD's mixer after they are
asked for).

**In the game without a listener** (2026-11-15, night). Testing builds forced Star Power on for 12 s and off for 8 s,
captured the game's sound output to WAV files, and logged the stack's state read from the game's memory at each
window's off. Those aids are not in this commit. The song was the debug menu's "Again" (155 BPM) with the bot on
guitar, RTX A2000, one run of about 150 s each. In that song the SAP has a single player, a vocalist with no stem
output, and the guitar plays on the SAP's own guitar track (SAP + 372), which the forced windows sent. Runs: A, World
Tour's single guitar settings. B, the send at 0 (the baseline). C, the stack's creation settings (a 600 ms echo into a
3.24 s reverb, wet only). A6, A with `ghwor_star_power_reverb_send_db = 6`. M, A with
`ghwor_star_power_reverb_multi_preset = true`.

The state at the first window's off (run A, `logs\ghwor_079.log`, and the same in the run measured below,
`ghwor_084`): the three DSPs active and not bypassed, wired source, echo, sfxreverb, high pass, then into the head of
the Master buss, which the dry guitar also reaches. The echo works on the 6 channels FMOD mixes and has its buffer
(delay 580.5 ms, decay ratio 0.25, dry 0.70, wet 1.00). World Tour's settings are there, the reverb's 0.1 s fade over
(so the sound system's update carries it). The send's mix is World Tour's 1.587 (+4 dB) and the track's own volume 1.
Nothing is lost on the way: both paths reach the soundcard's DSP with the same mixes.

The captures, each against B (aligned on the music, the first window only: the runs differ later, see below):

| Run | Stack's sound against the whole mix | Echo (correlation of the difference with the mix) | After the off |
|---|---|---|---|
| A, World Tour | 13.1 dB under | a peak at 580.5 ms (0.14), the logged delay | the echo for 0.6 s, then gone |
| A6, +6 dB send | 8.9 dB under | 580.5 ms (0.16) | the same, 5 dB higher |
| M, multiplayer settings | 16.7 dB under | none (the echo stage only lets 0.2 through) | near the floor |
| C, creation settings | 9.1 dB under | (no dry sound to compare) | about 17 dB a second over 3 s: the 3.24 s reverb |

How far to trust them: C against B in an off window leaves 46.5 dB under the mix (the two runs play the same sound to
the sample there), A, A6 and M against B about 20 dB under at the start. From the third window on the runs drift
apart (about 10 dB under the mix in off windows), so only the first window was used. The mono fold FL + FR + 0.707
(C + SL + SR) was measured.

The cause, then: nothing is broken. The first run heard a short slap (146 to 236 ms, the wrong delay) at World Tour's
level, which puts the stack's whole sound about 13 dB under the mix. World Tour taps its send at the same place
(before the part's volume: its part's stream goes through a flat ParamEQ, `0x820E65C8`, the send leaves from it and
the volume is the mix of its output, `0x820E62A4`), so if World Tour sounded stronger it was its own mix around it
(the band's and the guitar's levels), which is not World Tour's code to copy. The settings below are for that.

**By ear** (2026-11-15). Songs were played with the option on, and the Star Power echo sounded right.

## The log

In `logs\ghwor_NNN.log`:

- At the boot: `[star power reverb] World Tour's stack on slot 2: echo, sfxreverb, highpass (...; N effect nodes
  were free before it, T of 64 taken at once so far)`, or a warning with the reason the option stays off
  (`no stack on slot 2: ...`, `the stack on slot 2 lacks ...`).
- Each Star Power: `[star power reverb] player 1 on (guitar): World Tour's guitar, single player preset, echo delay 300
  ms (a beat of 600 ms, 100 BPM), send +4 dB (1.587) on 1 of 1 track`, and `player 1 off: slot 2's send cut on 1 of
  the 1 track sent`. In single player, an on while World Tour's flag is still on says that nothing changed, as World
  Tour does.
- At the next song's load, when a song ended with Star Power on: `the song player's clean-up: slot 2's send cut on X
  of the N tracks still sent`.
- A warning each time the effect pool's peak rises with 3 nodes or fewer left: `the effect pool's peak: T of 64 nodes
  taken at once (3 of them World Tour's stack), L left: ...`. The option's three nodes are then as many as were left,
  or more, and a full pool crashes the game: turn the option off if it does.
- Anything that fell back to the game's own behaviour: a warning saying why.

## Settings to try (ghwor.toml or the command line)

Two settings, not in the launcher, for when World Tour's sound is there but too faint. Both are read once at the
start, do nothing without `ghwor_star_power_reverb = true`, and are World Tour's sound at their defaults. They go in
`ghwor.toml` (or on the command line), not in `ghwor_options.toml`: the launcher writes that file whole whenever a
setting changes, so a line added to it by hand is lost.

- `ghwor_star_power_reverb_send_db = 0.0`: dB added to World Tour's send of the player's instrument into the echo and
  the reverb. The send is the level of everything the stack gives back (the echoes and the reverb), not of the dry
  instrument. The engine stops a send at 4x (+12 dB): a single guitarist's +4 dB can go up 8 dB, a single bassist's
  +8 up 4, a single drummer's +12 is already there.
- `ghwor_star_power_reverb_multi_preset = false`: `true` gives one player World Tour's settings for several players:
  no echo (its stage lets 0.2 of the sound through, wet 0), then a large 3 s reverb (room -2000, reverb +2000,
  reflections +1000, its dry sound at -10 dB), and the multiplayer send (guitar +6 dB, bass +8, drums +7).

What to try, by ear, in a song you know with Star Power in a quieter passage (guitar, one player):

1. `ghwor_star_power_reverb = true` alone: World Tour's settings. The guitar repeated 1.5 beats later (about 580 ms at
   155 BPM), each repeat a quarter of the last, with a short room; faint, the stack about 13 dB under the mix.
2. Then `ghwor_star_power_reverb_send_db = 6`: the same, measured 4 dB louder (9 dB under the mix): the repeats clearly
   heard.
3. `ghwor_star_power_reverb_send_db = 8`: the most for a guitar (+12 dB, the engine's limit), about 2 dB more.
4. `ghwor_star_power_reverb_multi_preset = true` with `ghwor_star_power_reverb_send_db = 6`: no echo, a large 3 s
   reverb on the guitar, about 11 dB under the mix (reckoned: alone, with no gain, it measured 16.7 dB under, too
   faint; with the gain it was not measured).

The values that sound right can stay in `ghwor.toml`; World Tour's are 0 and false.

## Limits

- The stack is created once at the boot and kept (as the venue's), not once a song as in World Tour. Its tail is
  muted at the song's clean-up instead, which a song's end (`SAP_Unload`) does not reach: when a song ends with Star
  Power still on, the tail fades by itself (the music has stopped) until the next song's load.
- The stack keeps three of the effect pool's 64 nodes for the whole session, which the game cannot use. The boot's
  check only sees that they are free then, not the song player's peak later: the log warns when that margin is 3
  nodes or fewer, and a four player song has not been measured yet.
- The drums are four tracks here (kick, snare, tom, cymbal), each sent at World Tour's level, where World Tour had one
  stream: the same sum.
- Two players on the same instrument share its stem object, so one player's Star Power off cuts the other's send, as
  the game does with the venue's send.
- World Tour's flag (the "already on" presets) is the hook's own: cleared at each Star Power off (any part's) and at
  each song, as World Tour does, but set only by a guitar's, a bass's or the drums' Star Power on that reaches
  `sub_820D7148` with tracks to send. World Tour set it at any part's on, the vocals' too, and here a vocalist's on
  never reaches the hook (Warriors of Rock's script only calls `VocalDSPSetParams` for it). So with only a vocalist's
  Star Power on, a bass player's that follows takes "bass, multiplayer" (no echo, the 3 s reverb) where World Tour
  took "bass, multiplayer, flag already on" (the bass's echo and short reverb). The guitar and the drums are not
  affected: their two presets are the same.
- Three more effects in FMOD's mix, which runs on its own thread; `audio_keep_time` lines in the log would show a PC
  that cannot keep up (`docs/audio-crackling.md`).
