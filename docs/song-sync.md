# The notes ahead of the music when a song follows another

**Status: fixed on branch `fix/song-sync` (2026-11-18), played.** Measured with the bot in the Rush chapter at two
calibrations, in Quickplay+ (a playlist, a retry, a pause, 60 FPS), in practice and on the calibration screen. Gustavo
played the Rush chapter's first two parts and a Quickplay+ song with his 0 / 120: in sync.

## Symptom

Gustavo played the Rush chapter's songs one after another, with the game's cinematics between them (2026-11-17). Each
song after a cinematic started out of sync: the notes came before the sound for the whole song. A pause fixed it until
the next song. His calibration: 0 ms video, 120 ms audio.

## Cause

The notes follow the song's clock. The music is started by a script that waits until the clock reaches the song's
start, so the clock has to begin before that point. When a song follows another without going through the menus (a
chapter's next part, a playlist's next song), the game resets the clock to
`0 - transition_time - 2 x transition_lag_offset`, where `transition_lag_offset = 0 - video - audio` (the
calibration's). The first song after the menus takes the offset once and has a 3 s intro.

The Rush chapter's parts 2 to 7 have a 100 ms intro. With an audio calibration more than 50 ms above the video's, that
puts the clock past the song's start before the music has begun. The script then starts the music from its beginning
at once, and the notes stay that far ahead of it:

| Calibration | Clock at the reset | Notes ahead of the music (measured) |
|---|---|---|
| 0 / 120 | +140 ms | 172 ms for the whole part |
| 0 / 200 | +300 ms | 333 ms for the whole part |

A pause fixed it because the game seeks the music to the clock when it pauses. The same arithmetic is in the console's
scripts.

## Fix

`src/song_sync.cpp`, hooks in `hooks/song_sync.toml`. The song player's own calls tell when a song's music is loaded
and waiting to be played, and from which position (`SAP_Load`, `SAP_Seek`; `SAP_Play`, `SAP_Stop`, `SAP_Unload`,
`SAP_DeInit` and `SAP_UnPause` end the wait).

- **At the clock's reset:** while the music waits, a reset to later than 50 ms before its start (and less than 0.45 s
  after it) sets the clock 100 ms before it. That is what the console does with no calibration (the Rush intro's own
  length). The game's script then starts the music as in any other song.
- **At the music's start:** while the music waits, a clock more than 30 ms past its start is set back to it. This
  catches any other path that starts the music late, and a hitch right before the start.
- **Above 60 FPS:** the script that starts the music checks the clock every frame instead of 60 times a second, so the
  music starts within a frame of its time (6.25 ms at 160 FPS instead of 16.7 ms).

Each song's start is written to the log, for example `[song sync] the music starts from 0.000 s with the notes' clock
+0.1 ms from it; its reset to +140.0 ms was moved (rule A)`. The line is written with the fix off too, so a log shows
whether a song started late.

Measured with the bot at 160 FPS. The figure is the notes' clock less the music's position; the game's own steady
value is about +5 to +20 ms, and the same value right after a pause, when the game itself puts the music at the clock.

| | Before | After |
|---|---|---|
| Rush part 1 (after the menus), 0 / 120 and 0 / 200 | +4 to +22 ms | +4 and +13 ms |
| Rush parts 2 and 3, 0 / 120 | +172 ms | +15 and +14 ms |
| Rush part 2, 0 / 200 | +333 ms | +16 ms |
| A Quickplay+ playlist's second song | +4 to +22 ms | +14 ms (nothing moved) |

The fix never acted outside the Rush chapter's later parts. Practice, the practice loop, a pause, a retry, the
calibration screen and 60 FPS started as before, with the music within 6 ms of its time at 160 FPS.

## Turning it off

`ghwor_song_start_sync = false` in `ghwor.toml`.
