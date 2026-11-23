// This game's profile: everything the launcher knows about the game it belongs to. The launcher's code is the same for
// every game (the RecompLauncher submodule, recomp_launcher\, with game_types.h for the types below); game.h, game.cpp
// (the game's own texts) and palette.h (its colors) are each game's, with the art (launcher\art), the icon
// (res\icon.ico), the resource script (res\launcher.rc) and the version (res\version.h). The names and the options the
// game executable shares are src/project.h's.

#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "dlc_disc_names.h"
#include "game_types.h"
#include "project.h"
#include "version.h"

namespace launcher::game {

// --- Names ---

// The game, as the texts name it ("the .iso file of %s").
inline constexpr const char* kName = GAME_TITLE;
// The recompiled executable and the launcher, side by side; the launcher's name is its window's title.
inline constexpr const char* kRecompExecutable = GAME_EXE;
inline constexpr const char* kLauncherExecutable = GAME_LAUNCHER ".exe";
inline constexpr const char* kLauncherName = GAME_LAUNCHER;
// The game's settings next to it: its base file (src/startup.cpp writes the defaults) and what the launcher saves.
inline constexpr const char* kConfigFile = GAME_CONFIG;
inline constexpr const char* kOptionsFile = GAME_OPTIONS;
// The game's user data when its folder cannot be written: Documents\<this> (the SDK's default, the app's name).
inline constexpr const char* kDocumentsFolder = "ghwor";
// res\version.h.
inline constexpr const char* kVersion = RECOMP_VERSION_STR;

// --- The look ---

// The title the Play page shows over the art: a small line above, the name below.
inline constexpr const char* kArtKicker = "GUITAR HERO";
inline constexpr const char* kArtTitle = "Warriors of Rock";
// The mark in the header's logo while the game's icon is unknown: an icon's SVG paths in a 24-unit box, stroked as the
// launcher's icons are (ui.cpp). A guitar pick with a star (the mockup's, local_docs/launcher_design; GH5's too).
inline constexpr const char* kBrandIcon =
    "<path d='M12 21c-4-4-8-9-8-13 0-3 4-5 8-5s8 2 8 5c0 4-4 9-8 13z'/>"
    "<path d='M12 6.5l1.2 2.6 2.8.3-2.1 1.9.6 2.8-2.5-1.4-2.5 1.4.6-2.8-2.1-1.9 2.8-.3z'/>";
// The header's name beside the logo, on two lines (in capitals); an empty title: the art's (kArtTitle) alone.
inline constexpr const char* kBrandTitle = "GUITAR HERO";
inline constexpr const char* kBrandSubtitle = "Warriors of Rock";
// Where the art under the pages (launcher\art\background.svg, 1280 x 760) is cropped in a window of another shape: the
// point kept in view, as fractions of its width and height. The emblem (464..943 x 38..600): in a 21:9 window it is
// centered between the header and the footer; the sides are cropped evenly, as the mockup's cards and title sit.
inline constexpr float kArtFocusX = 0.5f;
inline constexpr float kArtFocusY = 0.2f;
// The logo's mark on a disc instead of a rounded square.
inline constexpr bool kBrandRound = false;
// The titles in capitals (the game's own texts that the launcher shows as titles).
inline constexpr bool kUppercaseTitles = false;

// --- The disc ---

// The disc's executable, at its root and in assets\ once extracted; extracted last, so that its presence marks a
// complete installation.
inline constexpr const char* kDiscExecutable = GAME_XEX;
// The disc this build was recompiled from (src/startup.cpp checks the same CRC at the game's start).
inline constexpr uint32_t kTitleId = 0x41560883;
inline constexpr uint32_t kExecutableCrc32 = 0x62201F03;
inline constexpr const char* kRegion = "USA";
// The game's files on that disc (1355 files): the space the first run asks for before an ISO is chosen.
inline constexpr uint64_t kInstalledBytes = 6385225225;
// The game's frame: the Xenos path's scales and NativeXRenderer's internal resolutions are multiples of it.
inline constexpr int kFrameWidth = GAME_FRAME_WIDTH;
inline constexpr int kFrameHeight = GAME_FRAME_HEIGHT;

// --- Updates ---

// This project's releases (updater.cpp): GitHub's API for them, newest first, and their page. Empty: the launcher
// does not look for updates, and its Updates page only shows the version. Set from the first release on, while the
// repository is still private (the check fails until it is public, and the releases made then are found).
inline constexpr const char* kReleasesApi = "https://api.github.com/repos/ElGuslml/GHWoRRecomp/releases?per_page=20";
inline constexpr const char* kReleasesPage = "https://github.com/ElGuslml/GHWoRRecomp/releases";

// --- The game's own options (a cvar each; empty: the game has no such option, and the launcher no row) ---

// NativeXRenderer draws this game (src/native_x_renderer_ghwor.cpp): the Graphics page's renderer cards, its
// internal resolution, antialiasing and image format. False: the game is drawn by Xenos only, and the launcher says
// nothing of NativeXRenderer.
inline constexpr bool kHasNativeXRenderer = GAME_HAS_NATIVE_X_RENDERER;
// The renderer chosen until the player picks one (an options file without native_x_renderer): NativeXRenderer once a
// full play test has gone through it (as in POTCAWE, its D56), the Xenos path until then. NativeXRenderer, as in GH5
// (its 575136e), with SMAA on the scene's copy.
inline constexpr bool kNativeXRendererDefault = true;
// The antialiasing row's DLAA (NativeXRenderer's D47: NVIDIA DLSS on the main 3D view, jittered), and with it the Video
// page's AMD FSR 3.1 and NVIDIA DLSS. The game's split render stays on with them (src/options.cpp, as on the console)
// and they run once a pair of frames, the view paused in the stage's frame and finished in the band's, at the whole
// scene's copy before the screen effects (NativeXRenderer's D159; each camera cut starts their history again,
// src/camera_cuts.cpp, D158). Played in a song (2026-11-10, docs/native-x-renderer-ghwor.md's DLAA, DLSS and FSR 3.1):
// DLSS Quality and Performance at 3840x2160, the motion as smooth as SMAA's, no ghosting on the band, 60 FPS held with
// the CNN model on an RTX A2000; DLAA and FSR 3.1 the same. Far lit windows that flickered with all of them, drawn
// again by the bloom after the upscale point, no longer do (NativeXRenderer's D161). Before D159 DLSS needed the split
// off, where the 3D looked less smooth.
inline constexpr bool kHasDlaa = true;
// The Video page's DLSS model row, after the upscaling quality while the upscaler is NVIDIA DLSS (RecompLauncher
// 3.24.0): Transformer, NVIDIA's own default, Transformer K, the first Transformer model in every mode (3.26.0,
// lighter than the default in Performance and Ultra Performance), or CNN (NativeXRenderer's
// native_x_renderer_dlss_model), lighter: on the RTX A2000 at 3840x2160 CNN held 60 FPS where the Transformer model
// did not.
inline constexpr bool kHasDlssModel = true;
// Resolutions below the game's 720 lines (RecompLauncher 3.27.0, NativeXRenderer's D174 and D175), for weaker graphics
// cards: the internal resolution also offers 540, 480 and 360 lines, FSR 1's and NIS's Quality goes below 720 too (on a
// 1080p screen Performance draws 540 lines, Ultra Performance 360), and "Lower resolution for" chooses what they lower:
// the 3D scene only (the venue, the band and the crowd, enlarged at the scene's copy before the screen effects, where
// DLSS runs; the note highway, the HUD and the effects at the screen's resolution) or the whole image (the lightest:
// at 3440x1440 the GPU's frame 2.3 -> 1.05 ms at 360 lines, against 2.05 ms for the scene alone at half).
inline constexpr bool kLowResolutions = true;

// The Graphics page's shadow quality row: NativeXRenderer draws the game's shadow maps at a scale of their own
// (native_x_renderer_shadow_scale, its D92): at least twice or four times the game's resolution. Only where it
// finds them: the depth passes with no color target that are not of the game's output size.
// Turned on once the log shows NativeXRenderer finding the game's shadow maps ("shadow map: ...") with
// native_x_renderer_shadow_scale = 4, and their image is right: here GH5's 400x400 map (EDRAM base 1709) in a song
// (2026-11-13), its filter's offsets scaled by the library (D143; src/native_x_renderer_ghwor.cpp), and the 640x360
// depth pass no longer taken for one at the render scale 1 (D144).
inline constexpr bool kShadowQuality = kHasNativeXRenderer && true;
// Settings > Graphics' Ambient occlusion and Texture detail rows (RecompLauncher 3.31; NativeXRenderer's
// native_x_renderer_ao and native_x_renderer_texture_lod_bias, found from the game's views at its upscale point): XeGTAO
// on the venue, the band and the crowd in place of the game's own (src/screen_fx.cpp turns that one off), its radius
// from the profile (src/native_x_renderer_ghwor.cpp, 0.5); the 3D view's textures half a mip level or a whole one
// sharper. The note highway and the HUD, drawn after the upscale point, are left as they are.
inline constexpr bool kAmbientOcclusion = kHasNativeXRenderer && true;
inline constexpr bool kTextureDetail = kHasNativeXRenderer && true;
// Settings > Audio's Channels row with 5.1 (RecompLauncher 3.31; src/startup.cpp's audio_speaker_config_from_output):
// the game told its speakers, Dolby Digital only with 5.1 (Automatic on a surround device); its timing kept the same
// (src/mods.cpp's ghwor_dolby_timing).
inline constexpr bool kSpeakerChoice = true;

// The Graphics page's frame rate row: kFrameRateCvar's choices (game_types.h's FrameRate) and the value without the
// key. Here src/framerate.cpp's ghwor_frame_rate, the frame rate itself: 60 as on the console, above it the emulated
// vblank's rate (src/options.cpp), which the game follows at its own speed.
inline constexpr const char* kFrameRateCvar = GAME_FRAME_RATE_CVAR;
inline constexpr bool kFrameRateIsBool = false;
// Above 240 (RecompLauncher 3.31, with V-Sync off; Gustavo, 2026-11-18): 300, 360, 480 and Unlimited (1000 Hz, the SDK's
// ceiling), experimental: the rings of src/framerate.cpp hold a 60th of a second at 1000 (local_docs/56).
// Between 60 and 120 (local_docs/73, 2.3): 72, 75, 90 and 100, for a PC that cannot hold 120. With V-Sync they are even
// on a monitor whose rate they fit (72 on 144 Hz, 75 on 150, 100 on 200), with V-Sync off or a variable refresh rate.
inline constexpr std::array<FrameRate, 14> kFrameRates{{{60, 60},
                                                        {72, 72},
                                                        {75, 75},
                                                        {90, 90},
                                                        {100, 100},
                                                        {120, 120},
                                                        {144, 144},
                                                        {160, 160},
                                                        {165, 165},
                                                        {240, 240},
                                                        {300, 300},
                                                        {360, 360},
                                                        {480, 480},
                                                        {1000, 0}}};
inline constexpr int kFrameRateDefault = 60;

// The Xenos path's MSAA turned off (src/msaa.cpp): the Graphics page's antialiasing row with Xenos, the game's own
// antialiasing (its label, "2× MSAA") or none.
inline constexpr const char* kNoMsaaCvar = "";
inline constexpr const char* kMsaaLabel = "";

// The game's cameras widened for a screen wider than 16:9 (src/widescreen.cpp), set with the Video page's image
// format; the Xenos path then stretches its image over the screen (present_letterbox = false). Empty: the format
// applies to NativeXRenderer only.
inline constexpr const char* kUltrawideCvar = GAME_ULTRAWIDE_CVAR;

// The languages the game has (game_types.h's Language): the Game page's language row, in this order. Empty: the game
// asks in its own menu, and the launcher has no row.
// The disc's text packs (DATA/COMPRESSED/PAK/qs_*.pak.xen): English and French, German, Italian, Spanish (Spain's);
// the game picks one from XGetLanguage at start (GH Van Halen's and GH Metallica's packs, the same here; checked by
// Gustavo, 2026-11-08).
inline constexpr std::array<Language, 5> kLanguages{{
    {3, "Deutsch", nullptr, nullptr},
    {1, "English", nullptr, nullptr},
    {5, "Español", nullptr, nullptr},
    {4, "Français", nullptr, nullptr},
    {6, "Italiano", nullptr, nullptr},
}};

// Switches of the game's own (src/), each a boolean cvar with a row in the Settings tab (game_types.h's Toggle); their
// titles, hints and help are game.cpp's (GameTexts::toggles and toggle_help, in this order). Their places are launcher
// 3.28's (Gustavo's "propuesta A v2"), all but the screen effects off by default:
// - Effects: the screen effects (src/screen_fx.cpp, docs/fixes-from-gh-van-halen.md), all on as on the console: the
//   grain only with the color correction, under it (GH Van Halen's rule: without that pass its grain was far
//   stronger). GH Van Halen's ink blotches are not part of GH5's composite, nor of this game's (the same as GH5's): no
//   switch.
// - Display > Smoothness: the 3D scene drawn in every frame, off as on the console (the engine's split render mode off,
//   src/split_render.cpp, docs/scene-every-frame.md).
// - Graphics > Upscaling, two for DLSS, DLAA and FSR 3.1 (docs/native-x-renderer-ghwor.md), each shown only where it
//   does something: the upscaler's view ended after the bloom and the depth of field, which are then drawn at its input
//   size (src/native_x_renderer_ghwor.cpp's ghwor_upscale_after_effects; also the 3D scene drawn smaller, NativeXRenderer's
//   D174), and NativeXRenderer's DLSS evaluation one frame late (its D164, native_x_renderer_upscale_late, DLSS's
//   upscaling modes only, not DLAA), which evens out the two kinds of frame the split render alternates. The two are
//   independent of each other: src/options.cpp logs what the second does with and without the first.
// - Game > Starting the game: GH Metallica's boot videos shortened (the logos ended once the game has loaded, the intro
//   movie not played; src/intro.cpp, docs/fixes-from-gh-metallica.md).
// - Mods > Sound: Guitar Hero World Tour's Star Power sound, the player's instrument through World Tour's echo, reverb
//   and high pass while their Star Power is on (src/star_power_reverb.cpp, docs/star-power-reverb.md).
// - Mods > Tools: the game's debug menu (src/debug_menu.cpp), before the launcher's Debug keys.
// - Mods > Stage (launcher 3.29): the crowd, hidden during songs by the game's own soundcheck cheat (src/mods.cpp,
//   docs/mods.md). Mods > Sound: the whammy bar's bend of a held sustain, on as on the console (src/mods.cpp).
// - Launcher 3.30's Mods tab (Gustavo's "mods-galeria" mockup): Sound in the mockup's order, the whammy's sound first,
//   then World Tour's Star Power sound, then the track lowered on a miss (on as on the console; its volume under it).
// - Launcher 3.32: Mods > Highway's notes at the console's speed whatever the perspective (off as on the console; under
//   the perspective's row, locked at 100 %; src/mods.cpp, local_docs/66).
inline constexpr std::array<Toggle, 15> kToggles{{
    {"ghwor_fx_bloom", true, ToggleGroup::kScreenEffects},
    {"ghwor_fx_depth_of_field", true, ToggleGroup::kScreenEffects},
    {"ghwor_fx_color", true, ToggleGroup::kScreenEffects},
    {"ghwor_fx_grain", true, ToggleGroup::kScreenEffects, "ghwor_fx_color"},
    {"ghwor_fx_vignette", true, ToggleGroup::kScreenEffects},
    {"ghwor_scene_every_frame", false, ToggleGroup::kSmoothness},
    {"ghwor_upscale_after_effects", false, ToggleGroup::kUpscaling, nullptr, ToggleShown::kSceneUpscaled},
    {"native_x_renderer_upscale_late", false, ToggleGroup::kUpscaling, nullptr, ToggleShown::kDlssUpscales},
    {"ghwor_skip_intro", false, ToggleGroup::kStart},
    {"ghwor_debug_menu", false, ToggleGroup::kModsTools},
    {"ghwor_whammy_sound", true, ToggleGroup::kModsSound},
    {"ghwor_star_power_reverb", false, ToggleGroup::kModsSound},
    {"ghwor_crowd", true, ToggleGroup::kModsStage},
    {"ghwor_miss_lower", true, ToggleGroup::kModsSound},
    {"ghwor_highway_keep_speed", false, ToggleGroup::kModsHighway, "ghwor_highway_perspective"},
}};

// Rows of the game's own with several values (game_types.h's Choice, launcher 3.29): Mods > Highway, the note
// highway's size with one player (80-120 % by 5, Gustavo's steps), its length in time (the console's highway continued
// in its own perspective, the notes seen earlier at the same speed: 100, 125, 150, 170 %), its perspective (25-150 % of
// the console's compression by 25: flatter or steeper) and its look (the original, or the community's black highway),
// each a change of the game's script data when it starts (src/mods.cpp, docs/mods.md). Mods > Sound: how much of the
// player's instrument track stays on a miss (0-100 % by 10, Gustavo's steps; 0 = muted as on the console). Their titles, hints, help and the styles'
// names are game.cpp's (GameTexts::choices, choice_help, choice_values).
inline constexpr const char* kHighwayScales[] = {"80", "85", "90", "95", "100", "105", "110", "115", "120"};
inline constexpr const char* kHighwayLengths[] = {"100", "125", "150", "170"};
inline constexpr const char* kHighwayPerspectives[] = {"25", "50", "75", "100", "125", "150"};
inline constexpr const char* kHighwayStyles[] = {"original", "dark"};
inline constexpr const char* kHighwayOpacities[] = {"100", "90", "80", "70", "60", "50", "40", "30"};
inline constexpr const char* kMissVolumes[] = {"0", "10", "20", "30", "40", "50", "60", "70", "80", "90", "100"};
inline constexpr std::array<Choice, 6> kChoices{{
    {"ghwor_highway_scale", kHighwayScales, "100", ToggleGroup::kModsHighway, " %"},
    {"ghwor_highway_length", kHighwayLengths, "100", ToggleGroup::kModsHighway, " %"},
    {"ghwor_highway_perspective", kHighwayPerspectives, "100", ToggleGroup::kModsHighway, " %"},
    {"ghwor_highway_style", kHighwayStyles, "original", ToggleGroup::kModsHighwayLook, "", true},
    {"ghwor_highway_opacity", kHighwayOpacities, "100", ToggleGroup::kModsHighwayLook, " %"},
    {"ghwor_miss_volume", kMissVolumes, "0", ToggleGroup::kModsSound, " %", false, "ghwor_miss_lower"},
}};

// Launcher 3.30: the Mods tab's Highway section, its gallery of the player's pictures (src/highway_images.cpp reads
// the cvars) and its preview, drawn from the installed game's own textures: Highway_Guitar_01 and the gems' parts in
// pak\highway\basic_gems\basic_gems.tex, inside data/compressed/ZONES/z_in_game.pak.xen (local_docs/52).
inline constexpr bool kHighwayPreview = true;
inline constexpr HighwayGallery kHighwayGallery{
    "ghwor_highway_folder",     "ghwor_highway_image",       "ghwor_highway_random_from", "ghwor_highway_favorites",
    "ghwor_highway_dim",        "ghwor_highway_image_dims",  L"Highways",
    "ghwor_highway_scale",      "ghwor_highway_length",      "ghwor_highway_perspective", "ghwor_highway_opacity",
    "ghwor_highway_style",      "dark",                      "ghwor_highway_keep_speed",
};
inline constexpr HighwayAssets kHighwayAssets{
    L"data/compressed/ZONES/z_in_game.pak.xen",
    0xF3FB8221,  // pak\highway\basic_gems\basic_gems.tex
    0xCC8983D6,  // Highway_Guitar_01
    0x388DD606,  // sidebar02
    0x2ED9AC77,  // string
    0x12871CA5,  // FretBar_Med01_DNC
    0x452CB6D1,  // button_grey_Collar01 (the strike line's rings)
    0xCFA3D7FA,  // button_grey_Base01 (a gem's rim)
    0x15517A60,  // Button_Grey_Gradient01 (a gem's colored body)
    0xA8561777,  // Button_grey_Cap01 (a gem's cap)
    0x8A1231A7,  // WhammyBAR_Grey_wide (a sustain)
    // guitar_tweaks.qb's highway_guitar1 and highway_scale_chart's first row (src/mods.cpp).
    655.0f, 350.0f, 160.0f, 512.0f, 1.00108, 1.00125, 1024, 1.5f, 50.0f, 1.5f,
};

// --- Downloadable content ---

// The DLC page (page_dlc.cpp, dlc.h; docs/dlc.md): the player's DLC folder, its packages read by the game where they
// are (SDK change 80, content_packages.txt); packages extracted by launchers before 3.16 still read. Checked with a
// collection of 191 packages on a hard disk (154 for this game) and a Guitar Hero 5 package: its songs show in
// Quickplay+.
inline constexpr bool kHasDlcPage = true;
inline constexpr DlcMode kDlcMode = DlcMode::kFolder;

// Which packages the game reads (docs/dlc.md; GH5Recomp's local_docs/20). Its script 5A4B6A0C resolves every content
// folder by a manifest, as Guitar Hero 5: a package is known by the QBKey of its display name (sub_823B0A48; its first
// 42 characters, XCONTENT's length: the community's packs with longer names carry the manifest of the first 42, "All
// Nightmare Long - GHWT" and "Free Bird Wii Edition") and must carry "cmanifest_<key>.pak.xen", be named in another
// package's, or be in the content index on the disc (dlc_disc_names.h: Guitar Hero 5's DLC among them). One it cannot
// resolve it reports damaged.
inline constexpr DlcRule kDlcRule{.manifest_prefix = "cmanifest_",
                                  .name_length = 42,
                                  .known_names = kDlcDiscNames,
                                  .foreign = DlcForeign::kDamaged};

// NativeXRenderer's pipeline list (its D129, pipelines.h): Play first runs the game's --native_x_renderer_prepare_pipelines
// ("Compiling shaders") when the executable, the list (<TITLEID>.pipelines.list beside it) or the graphics driver
// changed since the last time. On when the game's NativeXRenderer profile names its shader files.
inline constexpr bool kHasPipelineList = true;

// --- The keyboard ---

// The Keyboard page's defaults where they are not the SDK's (its "Reset to defaults", and a control the files do not
// set): keybind_* cvars and their keys, as src/startup.cpp writes them for the game. The keyboard as a guitar
// (Gustavo's frets): D, F, J, K, L; the arrows strum; Space or Backspace Star Power; the sticks' other keys off, as
// W A S D, F and K would move them with the frets. Empty: the SDK's (game_types.h's KeyDefault).
inline constexpr std::array<KeyDefault, 19> kKeyboardDefaults{{
    {"keybind_a", "D"},
    {"keybind_b", "F"},
    {"keybind_y", "J"},
    {"keybind_x", "K"},
    {"keybind_left_shoulder", "L"},
    {"keybind_dpad_up", "Up"},
    {"keybind_dpad_down", "Down"},
    {"keybind_back", "Space,Backspace"},
    {"keybind_start", "Return,X"},
    {"keybind_rstick_right", "Right"},
    {"keybind_rstick_up", ""},
    {"keybind_rstick_down", ""},
    {"keybind_rstick_left", ""},
    {"keybind_rstick_press", ""},
    {"keybind_lstick_up", ""},
    {"keybind_lstick_down", ""},
    {"keybind_lstick_left", ""},
    {"keybind_lstick_right", ""},
    {"keybind_lstick_press", ""},
}};
// The keyboard playing drums (kHasDrums): its keys apart from the guitar's, in the highway's order. Five lanes, a
// Guitar Hero kit (pads on B, Y, X and A, the orange cymbal on RB, the kick pedal on LB): D red, F yellow, J blue,
// K orange, L green, Space the kick; four lanes, a Rock Band kit (no orange): K green. Backspace is Star Power (Space
// is the kick); the right stick's arrow off (a Guitar Hero kit's sticks carry its pads' velocity). The others are the
// guitar's (kKeyboardDefaults), as the menus' keys. GH Metallica's (and GH5's).
inline constexpr std::array<KeyDefault, 8> kDrumKeyboardDefaults{{
    {"keybind_b", "D"},
    {"keybind_y", "F"},
    {"keybind_x", "J"},
    {"keybind_right_shoulder", "K"},
    {"keybind_a", "L"},
    {"keybind_left_shoulder", "Space"},
    {"keybind_back", "Backspace"},
    {"keybind_rstick_right", ""},
}};
inline constexpr std::array<KeyDefault, 8> kDrumKeyboardDefaultsFourLanes{{
    {"keybind_b", "D"},
    {"keybind_y", "F"},
    {"keybind_x", "J"},
    {"keybind_right_shoulder", ""},
    {"keybind_a", "K"},
    {"keybind_left_shoulder", "Space"},
    {"keybind_back", "Backspace"},
    {"keybind_rstick_right", ""},
}};
// The Keyboard page's switch that makes the keyboard's frets strum as they are pressed (the SDK's mnk_tap_target with
// src/startup.cpp's mnk_tap_sources, the frets): the control it presses when on, the D-pad (strumming); "": no switch.
// The same on each controller's page for the controllers playing guitar (gamepad_tap_target, gamepad_tap_sources).
// Its title and hint are game.cpp's (GameTexts::keyboard_tap_title, keyboard_tap_hint).
inline constexpr const char* kKeyboardTapTarget = "dpad_down";
// The whammy bar's row on each controller playing guitar (page_controls.cpp): how far the game reads the bar, and its
// calibration (the SDK's gamepad_whammy_calibration, recomp change 60: the axis it is on, at rest and all the way).
inline constexpr bool kHasWhammy = true;
// Guitar Hero III's controller layout for a controller playing guitar, on by default (src/startup.cpp's
// gamepad_guitar_layout = "gh3"; the SDK's change 112): this game gives a plain controller the vocals, so it is told
// a controller is a guitar, and in a song the controller plays as Guitar Hero III's did, on buttons of its own (the
// launcher's "In a song" rows, the SDK's gamepad_song_binds, change 113).
inline constexpr bool kHasGh3PadLayout = true;

// --- Singing ---

// The Controls tab's microphone and each controller's part (page_controls.cpp): a recording device of the PC as each of
// the game's two USB microphones (the SDK's usb_microphone_1 and _2, recomp change 55), and a controller playing the
// game's instrument (gamepad_subtype's guitar, src/startup.cpp) or singing (a plain gamepad, XInput subtype 1, which the
// game gives the vocals; the SDK's gamepad_device_subtypes, change 56). Real guitars and drum kits keep their own.
// Their names are GameTexts::part_instrument and part_vocals.
inline constexpr bool kHasMicrophones = true;
// The microphone's gain (usb_microphone_1_gain) until the player changes it, and the Microphone's Reset: src/startup.cpp's.
inline constexpr int kMicrophoneGainDefault = -12;

// --- Drums ---

// A controller or the keyboard playing drums (launcher 3.19.1, the Instrument row): the SDK's gamepad_device_subtypes
// gives it XInput's drum kit subtype, 8. This game has GH Metallica's and GH5's code for it
// (docs/fixes-from-gh-metallica.md): sub_823CF718 makes a drum kit the game's type 11 (and remembers its capability
// flags' bit 0 as 128 at +120 of the controller port), src/drums.cpp turns 11 into 10 (Guitar Hero's five lanes) at its
// first state read unless that flag is set (the game itself only while the kit held the left stick's click, a Guitar
// Hero kit's sign, so every other kit played on four lanes), and the script function UseFourLaneHighway (sub_8217F7F0 ->
// sub_8217F6C8) plays type 11 on the game's own four-lane highway, a Rock Band kit's (generate_pos_table's four lanes,
// setup_button_up_models' 4d materials, touch_glow_props_rb_drum). So the Drum kit row chooses five lanes, or four with
// the SDK's change 87 ("8:1").
inline constexpr bool kHasDrums = true;
// Launcher 3.21.1: this game does play a Rock Band kit on four lanes (above, the same code as GH5's), so the Drum kit
// row stays (the default; the changelog's guess that this engine has five lanes only does not hold here either).
inline constexpr bool kHasFourLaneDrums = true;

// --- Online ---

// Settings > Online (launcher 3.20): online play between friends (the SDK's online_play, its changes 89-103 and 106),
// through Steam (rexsteam.exe; kOnlineByAddress stays false, as in GH Metallica). docs/online-play.md.
inline constexpr bool kHasOnline = true;

// --- The controls in the game's words ---

// The game's functions (game_types.h's GameControl) in groups, in the launcher's language (game.cpp); empty: the pages show the Xbox 360 controls
// (texts.cpp). The Keyboard page lists after them, under "Other controls", the ones no function is on.
const std::vector<GameControlGroup>& GameControlsFor(uint32_t language);
// The same for a controller or the keyboard playing drums (kHasDrums), five or four lanes.
const std::vector<GameControlGroup>& GameDrumControlsFor(uint32_t language, bool four_lanes);

// --- Texts of the game's own (game.cpp) ---

struct GameTexts {
  // The first run's welcome, in the game's spirit; null: the launcher's own (texts.cpp).
  const char* welcome_title;
  const char* welcome_text;
  // kToggles' rows: a title and a hint each.
  std::array<const char*, kToggles.size() * 2> toggles;
  // kToggles' help, the Settings tab's panel for the row in focus; null: the hint.
  std::array<const char*, kToggles.size()> toggle_help{};
  // kChoices' rows (launcher 3.29): a title and a hint each, their help (null: the hint) and their values' names (null
  // or missing: the value and the row's suffix).
  std::array<const char*, kChoices.size() * 2> choices{};
  std::array<const char*, kChoices.size()> choice_help{};
  std::array<std::array<const char*, 2>, kChoices.size()> choice_values{};
  // The first run's last screen (the game installed), in the game's spirit; null: the launcher's own.
  const char* done_title = nullptr;
  const char* done_text = nullptr;
  // The DLC page: where the game shows what was installed; null: nothing said.
  const char* dlc_where = nullptr;
  // The first run's extraction screen (while it runs), in the game's spirit; null: the launcher's own.
  const char* extracting_title = nullptr;
  const char* extracting_text = nullptr;
  // The Controls tab's keyboard switch for kKeyboardTapTarget, and its tip in the help.
  const char* keyboard_tap_title = nullptr;
  const char* keyboard_tap_hint = nullptr;
  // kHasMicrophones: a controller's two parts, the game's instrument and the singer's; null: the launcher's words.
  const char* part_instrument = nullptr;
  const char* part_vocals = nullptr;
};
// In the language of the launcher's texts (an XLanguage); missing ones fall back to English.
const GameTexts& GameTextsFor(uint32_t language);

}  // namespace launcher::game
