# Speakers and the game's Dolby Digital

**Symptom.** The game always believed it was connected through Dolby Digital 5.1: its Options > Audio switch did
nothing heard, and every player had the timing the game gives Dolby Digital.

**Cause.** The SDK's `XAudioGetSpeakerConfig` always answered Dolby Digital. The game mixes the same six channels either
way (measured in a song: centre and LFE empty, the rears about a third of the fronts, whatever it is told), and the
SDK folds them to stereo on a stereo device. What Dolby Digital changes is the game's timing: its scripts add
`default_lag_settings_dolbydigital`, -30 ms to the notes', the input's and the drums' timing, at every song start
(`setlagoffsets`), on top of the player's calibration.

**Fix.** `audio_speaker_config_from_output = true` (the SDK, set in `src/startup.cpp`): the game is told Dolby Digital
only when the sound plays in 5.1. GH Warriors of Rock Launcher's Settings > Audio > Channels:
- **Automatic** (the default): 5.1 on a device with surround speakers, two speakers otherwise;
- **5.1**: always Dolby Digital, as before (a stereo device gets the 5.1 mix folded to stereo);
- **Stereo** and **Mono**: two speakers.

**The timing stays.** `ghwor_dolby_timing` (on; `src/mods.cpp`) moves Dolby Digital's -30 ms into the scripts' base
offsets (`default_lag_settings.xenon`: -75, -91, +23 ms) and makes the Dolby set add nothing, so every choice keeps the
timing every player had, and no one calibrates again. `ghwor_dolby_timing = false`: a console's timing, the -30 ms only
with 5.1.

**How to turn it off.** `audio_speaker_config_from_output = false` in `ghwor.toml`, or 5.1 in the launcher.
