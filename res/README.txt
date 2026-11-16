GHWoRRecomp {VERSION} - Guitar Hero: Warriors of Rock (Xbox 360), for Windows
========================================================================

A static recompilation of the Xbox 360 game to native Windows. The game's code is already translated and
compiled into GHWoRRecomp.exe: you do not need to compile anything. You do need your own copy of the game,
because this package contains no game files.

This is a beta. Not affiliated with Microsoft, Activision or the game's developers.


REQUIREMENTS

- Windows 10 or 11, 64-bit.
- A GPU with up-to-date drivers that support Vulkan (NativeXRenderer, the default renderer) or
  Direct3D 12 (the Xenos renderer).
- A CPU with SSE4.2 (practically any 64-bit CPU from 2009 on).
- Microsoft Visual C++ Redistributable 2015-2022 (x64). Most PCs already have it. If Windows says that
  MSVCP140.dll or VCRUNTIME140.dll is missing, install it from:
  https://aka.ms/vs/17/release/vc_redist.x64.exe
- Your Guitar Hero: Warriors of Rock disc as an ISO file (tested with the disc sold in the USA; about 5.9 GB
  once installed).
- An Xbox 360 guitar. Any other controller works as a guitar too, and so does the keyboard. An Xbox One
  guitar (such as the PDP Riffmaster for Xbox) needs Microsoft GameInput: the installer offers it, and so does
  the launcher's Controls page while it is missing (Windows asks for permission to install it).
- To sing (experimental): any microphone connected to the PC, and a controller for the singer.


INSTALLATION

1. Either run the installer (GHWoRRecomp-{VERSION}-setup.exe: it installs for your Windows user, without
   administrator rights), or extract this zip to any folder, for example C:\Games\GHWoRRecomp.

2. Run "GH Warriors of Rock Launcher.exe". The first time, it asks for your Xbox 360 ISO, checks that it is
   the right game and extracts its files next to the launcher, then asks you to create your profile (your
   saves, achievements and playtime, shared with the other ElGus Recomps launchers). You only do this once.

   You can also copy the game files yourself instead: put the contents of the disc into the "assets"
   folder, so that assets\default.xex exists (extract-xiso, https://github.com/XboxDev/extract-xiso/releases,
   gets them out of an ISO).

3. Choose your options and press PLAY. Next time you can start GHWoRRecomp.exe directly: it uses the
   options you saved.

4. Calibrate: in the game's options, run the audio and video calibration once, so that the notes match what
   you hear on your own speakers or headphones.

If your default.xex is not the version this build was made from (CRC32 62201F03), the game warns you: a
different version will most likely crash.


"WINDOWS PROTECTED YOUR PC" / UNKNOWN PUBLISHER

The executables are not code-signed, so Windows SmartScreen warns about an unknown publisher the first time
you run them. To avoid it with the zip, unblock it before you extract it: right-click it, Properties, tick
"Unblock" at the bottom of the General tab, OK, and then extract it. Otherwise click "More info" and then
"Run anyway" in the warning (once for each program).


GH WARRIORS OF ROCK LAUNCHER

The launcher works with the mouse, the keyboard or a controller. Changes are saved automatically and apply
when you press PLAY.

- Play: starts the game, with your current settings, achievements and updates at a glance. The first time,
  and after an update of the game or of your graphics driver, it first shows "Compiling shaders" for a
  moment: it prepares the game's graphics for your graphics card, so that the game does not stutter the
  first time it shows something (with NativeXRenderer). Skip starts the game at once; it then compiles
  them as you play.
- Your profile (your picture at the top right): hours played, streaks, the week's activity, your games
  and your favorite achievements; edit your name, picture and color, or switch profiles.
- DLC: choose the folder where you keep your Xbox 360 packages (LIVE, PIRS or CON files, in it or its
  subfolders). The game reads them from there: nothing is copied, and the other ElGus Recomps launchers can
  use the same folder. The packages are listed by the game they were made for, each with a switch; those
  the game cannot read come turned off (with one on, the game would say a package is damaged). Guitar
  Hero 5's packages add their songs to Quickplay+'s song list too. DLC installed by an earlier version keeps
  working; if your folder has the same packages, "Remove copies" deletes the installed copies to free space
  (it asks first, and never touches your folder).
- Achievements: the game's 50 achievements, unlocked or not. In the game, F7 shows the same list.
- Controls: your guitars, controllers, the keyboard and the microphone.
  - A guitar or a controller: what each of its buttons does. A guitar's frets are A, B, Y, X and LB of an
    Xbox controller, strum is the D-pad, the whammy bar the right stick (a controller's, pushed to either
    side) and Star Power Back. A controller's Instrument: Guitar, Drums, or Vocals for the singer (see
    SINGING). With Drums, Drum kit chooses 5 lanes (Guitar Hero) or 4 lanes (the game's Rock Band highway, as
    with a Rock Band drum kit on the console); a real drum kit has the same choice.
    Play style, each controller its own: "Controller · GH III" (the default) plays songs as in Guitar Hero III, with the
    frets on LT, LB, RB, RT and A and the open note (bass) on B, no strum (pressing a fret plays its note),
    Star Power on the D-pad or Back, and the whammy bar on either stick; the game plays it with Guitar Hero
    III's own controller rules. Its buttons in a song are its own ("In a song": each fret, the open note,
    Star Power and the whammy bar on up to two buttons; a whammy bar on a button is all or nothing), apart
    from the menus' ("Navigation"). "Guitar · Xbox 360" plays it as an Xbox 360 guitar (for a guitar Windows
    sees as a controller, through an adapter), and "Strum when a fret is
    pressed" makes its frets strum as you press them, in songs. Each controller keeps its own assignments. Whammy bar: how far the game reads the bar.
    If it reads pressed with the bar at rest, or does nothing (some third-party guitars), press Assign,
    push the bar all the way and hold it, then let it go; X clears the assignment. Tilt: the same, if tilting
    gives you no Star Power (a Wii or PS3 guitar): press Assign, tilt the neck up as for Star Power and hold it,
    then go back to your playing position.
  - Keyboard (experimental): turn on "Play with keyboard and mouse" to play with the keyboard as a guitar
    (frets D F J K L, strum with the Up and Down arrows, Star Power with Space, Start with Enter), and change
    any key. "Strum when a fret is pressed" makes each fret strum as you press it, in songs. Shift, Ctrl
    and Alt can be keys of their own (press and release one alone): each side is a separate key, so Left
    Alt can be Star Power. F1 to F12 can be assigned too: a key on a control only presses that control.
    Instrument: Drums plays the keyboard as a drum kit with keys of its own (5 lanes: D red, F yellow,
    J blue, K orange, L green; 4 lanes: K green; Space the kick pedal, Backspace Star Power); your guitar
    keys stay as they were.
  - Microphone (experimental): the microphone you sing with, its channel and gain, and its level (see
    SINGING).
- Settings:
  - Video: the monitor, fullscreen, borderless or window, the resolution, the image format (fill a screen
    wider than 16:9, where the stage is seen wider while the note highway, the HUD and the menus stay 16:9
    in the center, or keep 16:9 with bars), the internal resolution the game is drawn at, upscaling (AMD
    FSR 1 or NVIDIA NIS, from fewer lines than the screen's, or AMD FSR 3.1 and NVIDIA DLSS, which rebuild
    the 3D from several frames; DLSS with its Transformer model or the lighter CNN) and sharpness, the
    shadows' quality (up to four times the original resolution), and V-Sync (on by default: no tearing, and
    no extra delay; leave it on with a G-Sync or FreeSync monitor too).
  - Graphics: the renderer, the graphics card, the frame rate, antialiasing and anisotropic filtering.
    NativeXRenderer (the default) draws the game natively with Vulkan; Xenos emulates the Xbox 360 GPU
    instead (DirectX 12 or Vulkan): choose it if something looks wrong with NativeXRenderer, and tell us
    where. Graphics card: on a PC with two (a laptop's integrated and dedicated ones), the one the game draws
    with. Frame rate: 60 FPS (original), and 120, 144, 160, 165 or 240 FPS where your monitor reaches them
    (experimental). The songs keep their speed and stay in sync at every frame rate. The game's screen effects
    can be turned off one by one.
  - Game: the launcher's language and the game's (English, French, German, Italian or Spanish; the game
    has no language menu of its own). Debug keys (off by default): F3, F4 and ` open the developer's windows
    in the game (frame times, settings, console). Venue at the full frame rate (off by default): the game
    draws its venue and its band in turns, each at half the frame rate (30 times a second on the console),
    while the note highway uses the full rate; on, the whole scene is drawn in every frame, for about twice
    the work for the processor and the graphics card: at 160 FPS even a fast PC can drop to about 130 in busy
    songs, while 120 FPS or lower leaves it room. Screen effects before the upscaler (off by default): with
    DLSS, DLAA or FSR 3.1 the glow and the blurred backgrounds are drawn at the upscaler's input resolution,
    faster at a high resolution and a little softer. Even frame times with DLSS (off by default): DLSS runs at
    the start of the next frame, so the venue's frames take about the same time, and the venue shows a pair of
    frames later (the note highway and the HUD are not delayed); DLSS upscaling only, not with DLAA, frame
    generation or the venue at the full frame rate. Skip the intro videos (off by default): at the start
    the logos end as soon as the game has loaded and the intro movie is not played, so the main menu comes
    sooner. Game debug menu (off by default): the debug menu its developers used, in the main menu and the
    pause menu (its texts in English; Unlock All and Add 100 Quickplay Stars change your save). World Tour's
    Star Power sound (off by default): Star Power puts your instrument through Guitar Hero World Tour's echo,
    reverb and high pass instead of the venue's reverb. Two more settings in ghwor.toml raise it:
    ghwor_star_power_reverb_send_db (dB added to its send, 0 by default) and
    ghwor_star_power_reverb_multi_preset (true: World Tour's settings for several players, a large reverb
    with no echo, for one player too).
  - Audio: the output device (Windows default, or another one), the channels (Automatic, Stereo or Mono:
    the same sound on both sides), the volume (0 to 200 %: above 100 % the game is louder than its own
    mix and its loudest moments can distort), mute while another window is in front, and Low latency audio
    (off by default): the game takes your audio device in exclusive mode and the sound comes sooner (about
    55 ms on a USB audio interface); other programs cannot play on that device while the game runs.
    Calibrate the audio again after changing it.
  - Online (experimental): online play with your Steam friends (see ONLINE PLAY).
- Updates: the launcher checks for a new version when it starts (you can turn that off) and installs it
  for you, keeping your saves, settings and game files. Get beta versions (off by default) offers the betas
  too.


SINGING (EXPERIMENTAL)

Any microphone connected to the PC works as the game's Xbox 360 USB microphone:
1. In the launcher's Controls tab, choose Microphone (under the keyboard) and pick your microphone, or the
   Windows default. With an audio interface, pick the channel of the input your microphone is on.
2. Sing as loud as you will in a song and watch the Level bar: it should reach the yellow without touching
   the red. Change the Gain if it does not (it starts at -12 dB).
3. Choose the controller that sings and set its Instrument to Vocals. In Quickplay+, that controller sings.
If the pitch line jumps around, lower the Gain; if the game does not hear you, raise it. Headphones help: with
speakers, the microphone hears the song too. While a microphone is chosen, Windows shows that the game is using
it.


ONLINE PLAY (EXPERIMENTAL)

The game's online modes with your Steam friends (Quickplay+ bands, the competitive modes and the rest),
without Xbox Live. Everyone needs Steam running and signed in, and this same version of the game.
1. In the launcher's Settings > Online, turn on Online play, on every PC (Through Steam turns on with it).
2. In the game, every band lobby is online, as on a console signed in to Xbox Live. Your Steam friends who
   have the game open are in the lobby's FRIENDS LIST: invite them from there.
3. An invite shows at the top of the screen: hold START (or Enter) for 2 seconds to join.
You can also invite a friend from Steam's friends list (Invite to game), or join a friend there (Join game):
the game takes you to their party. No port needs opening: Steam carries the game between you. Steam shows you
playing "Spacewar" (Valve's test app, which games without a Steam page of their own use). The music store,
leaderboards, Rock Record and Facebook / Twitter sharing needed Activision's servers, which are gone.


MORE THAN ONE CONTROLLER

The first controller to press a button once the game starts plays with your profile and its saves. The other
controllers join as its guests, as on the Xbox 360 ("Name (1)", "Name (2)"): they play without saves of their
own.


QUITTING THE GAME

In the main menu, press B (the red fret): the game asks whether to quit. A (the green fret, or Enter) quits, B
(or Escape) goes back.


STEAM BIG PICTURE

Add GHWoRRecomp.exe as a non-Steam game: it starts straight into the game with the options saved by
GH Warriors of Rock Launcher. You can also add "GH Warriors of Rock Launcher.exe" to change options with a
controller.


FILES

- ghwor.toml                Base settings. Created with the defaults this game needs if it is missing.
- ghwor_options.toml        What GH Warriors of Rock Launcher saves. Loaded after ghwor.toml.
- logs\                     Log files: the game's (ghwor_NNN.log) and the launcher's (launcher_NNN.log,
                            with a .dmp file when it stopped responding). Attach the latest ones when you report
                            a problem.
- userdata\                 The shader cache. Keep this folder if you move the game to another folder. If the
                            game folder cannot be written (for example under Program Files), it goes to
                            Documents\ghwor instead.
- Your saves, achievements and playtime are in your profile: %APPDATA%\ElGuslml Recomps.

Command line options (--name=value, for example in Steam launch options) win over both files, e.g.
--user_language=1 (English), --ghwor_frame_rate=120, --native_x_renderer=false (the Xenos renderer) or
--gpu_adapter=10DE:2782 (a graphics card by its PCI IDs, vendor:device in hexadecimal, as the launcher's
Settings > Graphics writes it).


REPORTING A PROBLEM

Tell us on Discord (https://discord.gg/XQtM5TmNs8) or open an issue on the project's page, with where in the
game it happened, a screenshot if you can, and the latest files in the logs folder (the game's and, if the
launcher misbehaved, the launcher's).


KNOWN ISSUES

- Guitar Hero: Warriors of Rock shares its engine with Guitar Hero 5, with GH5Recomp's, GHVanHRecomp's and
  GHMetalRecomp's fixes. Songs have been played with a guitar; drums and singing have not been tested in this
  game yet.
- Singing is experimental: if the pitch line jumps or the game misses your voice, change the microphone's
  Gain in the launcher.
- Some USB audio interfaces add delay in Windows' shared audio mode (a Focusrite Scarlett about 20 ms more
  than a motherboard's audio): calibrate the audio in the game's options.
- Above 60 FPS is experimental: something may still run too fast. Tell us what and where.
- Playing with the keyboard is experimental.
- Downloadable content: Guitar Hero 5's packages add their songs to Quickplay+; Warriors of Rock's own
  packages and the community's are read, but their songs have not been played yet. The community's custom
  songs need its "GH:WoR Customs Database" package in your DLC folder too: without it the number of songs can
  change from one start to the next.
- The first time an effect appears, it may show up a few frames late.
- Xbox One guitars (PDP Riffmaster, PDP Jaguar, Mad Catz Stratocaster) have not been played with yet: tell us
  how yours goes, with the game's latest log.
- Online play is experimental: Quickplay+ and a competitive mode have been played between two copies of the
  game; RockFest with more players and online Quest have not been tried yet.


MORE INFORMATION

{MORE_INFO}

GHWoRRecomp's own code is under the MIT license (LICENSE). It is built on the ReXGlue SDK and other
components under their own licenses: see THIRD_PARTY_NOTICES.md and the "licenses" folder.
