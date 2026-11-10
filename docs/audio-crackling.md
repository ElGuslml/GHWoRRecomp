# Crackling in songs on slower PCs

**Status: fixed (2026-11-10), SDK change 82.** Reproduced on the developer's PC by making the game's audio callback
slower. On the player's PC, `audio_callback_min_interval_us = 1000` with the old timing stopped the crackling, which
confirms the cause; the build with the fix is still to be tried there.

## Symptom

A player (Discord, 2026-11-09/10; Xeon E5-2640 v3, high-performance power plan): a hiss, then without FxSound a
crackle, through the whole of every song and never in the menus. The same PC plays Guitar Hero: Metallica cleanly. The
log has an `audio_keep_time: 1 frames (5 ms) ... dropped` line every few hundred ms of a song (307 in two minutes,
with Low latency audio on).

## Cause

The game's sound is made a frame (5.33 ms) at a time by its audio callback, on the SDK's audio worker. FMOD mixes on
a thread of its own and polls how far its ring has been read, so the calls are spaced at least 3 ms apart
(`audio_callback_min_interval_us = 3000`): without the spacing the host device's bursts of two frames made FMOD miss
blocks (the music as noise). The SDK counted the 3 ms from the end of the last call, so a frame cost the callback's
work plus 3 ms: past about 2.3 ms of work, which a slower CPU reaches in a song, the sound fell behind and
`audio_keep_time` dropped a frame to catch up, a click each time. Guitar Hero: Metallica spaces nothing (its FMOD mixes
on the audio worker), which is why it was clean on the same PC.

## Fix

The SDK counts the 3 ms from the start of the last call (change 82): a frame costs the longer of the two, so the
callback can take up to 5.3 ms, and two calls still start at least 3 ms apart for FMOD. Measured with the callback
made slower, 30-40 s of a Quickplay+ song each:

| Added to each call | Before (shared / exclusive output) | After (shared / exclusive output) |
|---|---|---|
| 2.5 ms | 955 / 2085 frames dropped | 0 / not run |
| 4 ms | not run | 0 / 0 |

Without anything added nothing changes on a fast PC (the callback takes 0.2-0.6 ms there).

## Turning it off

`audio_callback_min_interval_us = 0` in `ghwor.toml` turns the spacing off (FMOD's noise may come back);
`audio_keep_time = false` turns the catch-up off (the sound then stays late after a hold-up, until a pause).
