# Documentation

Short notes on each problem found while making the game run, and what was done about it. Each one gives
the symptom, the cause, the fix and how to turn the fix off, so that you can check it on your own machine.

| Note | Status |
|---|---|
| [The fixes ported from Guitar Hero 5](fixes-from-gh5.md) | In testing |
| [NativeXRenderer in Guitar Hero: Warriors of Rock](native-x-renderer-ghwor.md) | In testing, the default renderer: SMAA and FXAA on the scene's copy, and DLAA, DLSS and FSR 3.1 once a pair of frames with the split render (NativeXRenderer's D159) |
| [Downloadable content](dlc.md) | Working: the player's DLC folder, read in place (0.1.1-beta.2); Guitar Hero 5's Megadeth Track Pack adds its songs to Quickplay+ |
| [Stutters the first time something is shown](compiling-shaders.md) | Fixed: a pipeline list ships with the game; the launcher compiles it for your graphics card at the first Play ("Compiling shaders"; NativeXRenderer) |
| [Crackling in songs on slower PCs](audio-crackling.md) | Fixed in the SDK (change 82): the audio callback's spacing counted from its start, so a slower CPU keeps up |
| [The fixes from GH Metallica of 2026-11-11 to 13](fixes-from-gh-metallica.md) | Built, not tried in the game yet: the game's debug menu, the logos ended once the game has loaded and the intro movie left out, drums on five or four lanes (five lanes fixed 2026-11-15: the game wanted a Guitar Hero kit's sign, checked with the keyboard in Party Play), Settings > Audio and the graphics card; the tap notes' trail not ported (this game draws no trail); online play since in [its own note](online-play.md) |
| [The mouse cursor](mouse-cursor.md) | Hidden over the game after 2 s without moving it (SDK change 103) |
| [Xbox One guitars (PDP Riffmaster)](xbox-one-guitars.md) | Built, a player's test to come: read through Microsoft GameInput (SDK change 110), the tilt as an Xbox 360 guitar's, the launcher offering GameInput when it is missing (3.23) |
| [Online play](online-play.md) | In testing: two copies on one PC and two PCs through Steam played songs together, joined by invites (SDK changes 106-109 and 111: the party's key, socket numbers, no port without online play, the friends list kept up to date, an invite's join spent) |
| [The venue at the full frame rate](scene-every-frame.md) | Built, off by default: the engine's split of the 3D scene over two frames turned off (the venue and the band drawn in every frame, about twice the 3D work); checked in Party Play on both renderers |
| [D3D's fence stamps and the page watches](fence-stamps.md) | Branch `perf-240`, in testing: the stamps written where the watches do not see them; 240 FPS held in a busy song with the venue every frame |
| [A controller plays as in Guitar Hero III](controller-gh3-layout.md) | Played with a DualSense: the game's own GH III controller code, chosen by hooks (SDK change 112, the default; a switch in the launcher), extended sustains, and its own buttons in a song (SDK change 113: the launcher's "In a song" and "Navigation") |

Building the project, its configuration and how the SDK is changed: [`development.md`](development.md).
