# Documentation

Short notes on each problem found while making the game run, and what was done about it. Each one gives
the symptom, the cause, the fix and how to turn the fix off, so that you can check it on your own machine.

| Note | Status |
|---|---|
| [The fixes ported from Guitar Hero 5](fixes-from-gh5.md) | In testing |
| [NativeXRenderer in Guitar Hero: Warriors of Rock](native-x-renderer-ghwor.md) | In testing, the default renderer: SMAA and FXAA on the scene's copy |
| [Downloadable content](dlc.md) | Working: the player's DLC folder, read in place (0.1.1-beta.2); Guitar Hero 5's Megadeth Track Pack adds its songs to Quickplay+ |
| [Stutters the first time something is shown](compiling-shaders.md) | Fixed: a pipeline list ships with the game; the launcher compiles it for your graphics card at the first Play ("Compiling shaders"; NativeXRenderer) |
| [Crackling in songs on slower PCs](audio-crackling.md) | Fixed in the SDK (change 82): the audio callback's spacing counted from its start, so a slower CPU keeps up |
| [The fixes from GH Metallica of 2026-11-11 to 13](fixes-from-gh-metallica.md) | Built, not tried in the game yet: the game's debug menu, the logos ended once the game has loaded and the intro movie left out, drums on five or four lanes, Settings > Audio and the graphics card; online play and the tap notes' trail not ported (this game draws no trail) |
| [The mouse cursor](mouse-cursor.md) | Hidden over the game after 2 s without moving it (SDK change 103) |

Building the project, its configuration and how the SDK is changed: [`development.md`](development.md).
