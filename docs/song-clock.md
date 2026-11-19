# The notes drifting from the music during a song

**Status: fixed on branch `fix/song-sync` (2026-11-19), measured; Gustavo's play test to come.** Three causes, each
with its own switch. The start of a song is another note: [song-sync.md](song-sync.md).

## Symptom

Measured with a probe that compares the notes' clock with the music's position in every frame (the bot playing):

- **V-Sync off** (480 Hz, about 300 FPS): the notes gained on the music the whole song: +35 ms in 97 s in one run,
  +73 ms and +28 ms in 186 s in two others. The amount changed from run to run.
- **A hitch of the main thread** (150 ms every 5 s): the notes stepped 3 to 10 ms either way at each one, wandering
  over 24 ms in a song.
- **A late audio device callback**: the music stayed 10 ms later after each one, for good. Seen twice in one song
  with no cause of ours, and reproduced by holding the callback 30 ms.

With V-Sync on and no hitch there was no drift, before or after.

## Causes and fixes

**1. The SDK's guest clock gained on the host's (SDK change, every game).** Two threads reading the guest's time base
at once could count the time between their two readings twice. It was rare with V-Sync on. With V-Sync off the
vertical blank timer spins on that clock while the game's threads read it, and the time base ran 200 to 260 ppm fast
(about 14 ms a minute). The host's count is now read with the clock's mutex held.

**2. The game's own sum of the vertical blanks (`src/song_clock.cpp`, `ghwor_song_clock_exact`, on).** The song's clock
adds, in each frame, the intervals of the blanks that passed, taken from a ring of the last 10. The sum starts at the
ring's oldest entry, walks the ring twice when a frame lasts more than 10 blanks, and can count a blank twice after a
hitch. With even blanks (V-Sync) none of it shows: 2 ms in a 191 s song. With a timer's uneven blanks it was -110 to
+608 ppm from run to run. The clock now adds the time between the blanks themselves, kept by a hook in the vblank
callback. The frames' logic is the game's own, so the notes move as before.

**3. A gap in the audio device's output (SDK change, `audio_keep_time_device_gaps`, on).** When the device's callback
comes late and the device runs dry, it plays nothing for a while and then goes on from where it was. No frame was
missing from the SDK's queue, so `audio_keep_time` saw only part of it. The frames given to the device are now
compared with the time that passed; when they fall 4 ms or more behind and stay there, as many frames of the game's
sound are dropped.

| Case | Before | After |
|---|---|---|
| V-Sync off at 480 Hz | notes +28 to +73 ms in 186 s; +60 ms in 97 s with the first fix alone | -0.2 ms in 97 s |
| Main thread held 150 ms every 5 s, 97 s | between +7 and -16 ms | +8.0 ms, within 0.5 |
| Audio callback held 30 ms, four times | music +50 ms later | back within 3 s each time |
| V-Sync at 160 and at 60 FPS | steady | steady |

Not changed: the clock runs one vertical blank ahead or not, depending on how the frames meet the blanks (a 6.25 ms
step at 160 Hz when it changes). That is the engine's design; at 60 Hz it is always one ahead.

## In the log

At a song's end: `[song sync] the song's clock over 191.3 s of vertical blanks, 30575 updates: ... the game's own sums
+0.01 ms from the blanks' times`. With `ghwor_song_clock_exact` off that figure is the clock's error in the song. A
device gap: `audio_keep_time: the device's output fell 20.0 ms behind the frames it was given`.

## Turning it off

`ghwor_song_clock_exact = false` and `audio_keep_time_device_gaps = false` in `ghwor.toml`. The SDK's clock fix has no
switch.
