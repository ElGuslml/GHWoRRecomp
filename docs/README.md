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

Building the project, its configuration and how the SDK is changed: [`development.md`](development.md).
