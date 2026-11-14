// ghwor - the boot's videos shortened: the logos end as soon as the game has loaded, and the intro movie after them is
// not played: the autosave notice, then the main menu, right after the loading.
//
// GH Metallica's boot is a chain of UI states with a video each (its fix makes the script function PlayMovie find none
// of them), GH5's one video played as the loading screen (its fix ends it once the game has loaded, through
// end_movie_early). This game's boot is neither (read from the decoded scripts, scripts\guitar\guitar.qb and
// scripts\guitar\menu\loading_screen.qb): guitar_startup_spawned plays the logos, logo_intro
// (data\movies\logo_intro.bik.xen: 580 frames at 29.97 fps, 19.4 s), as its loading screen (create_loading_screen
// {loading_screen_type=bink, bink='logo_intro'}), loads the game behind them, then calls destroy_loading_screen
// {wait_for_unload, wait_for_end=1}: destroy_loading_screen_spawned waits while the logos play (no button ends them:
// this game has no end_movie_early) and kills the movie. Then, "if ($g_skip_intro_movie = 0)", it plays the game's
// intro, initial_boot_movie (data\moviestreams\initial_boot_movie.bik: 1591 frames, 53.1 s), with PlayMovieAndWait
// (Start or the green fret ends it), and goes on to the autosave notice (UIState_save_icon_warning) and the main menu.
// There is no legal notice to wait for.
//
// The logos cannot be refused as GH Metallica's videos are: create_bink_loading_screen_spawned starts them and then
// waits until they are playing (begin if IsMoviePlaying break ... repeat) before the startup goes on, so a video not
// found would stop the boot. Instead two script globals read as 1 in the boot's scripts, through src/debug_menu.cpp's
// hooks on the script interpreter (the "$name" resolve, one hook per address); the globals themselves keep their values:
// - autolaunch_go (-1 unless the developers' autolaunch runs) in destroy_loading_screen, whose "if ($autolaunch_go !=
//   -1) <wait_for_end = 0" then lets destroy_loading_screen_spawned kill the logos at once, as the developers'
//   autolaunch does: the logos end the moment the game has loaded. Only the boot passes wait_for_end; any other
//   loading screen destroyed meanwhile already has it at 0.
// - g_skip_intro_movie (0, the developers' own switch, read by guitar_startup_spawned only) in guitar_startup_spawned:
//   the intro movie is not played.
// After the second read the hooks are left alone again (unless the debug menu is on). If the logos end before the game
// has loaded (a slow disk), destroy_loading_screen_spawned finds no movie playing and nothing changes.

#include "intro.h"

#include <rex/cvar.h>
#include <rex/logging.h>

#include <atomic>
#include <cstdint>

REXCVAR_DEFINE_BOOL(ghwor_skip_intro, false, "GHWoR",
                    "Shorten the videos at the game's start: the logos (played while the game loads) end as soon as the "
                    "game has loaded, and the intro movie after them is not played: the autosave notice and the main "
                    "menu right after the loading; false = as on the console");

namespace {

constexpr uint32_t kDestroyLoadingScreen = 0xF3896918;  // destroy_loading_screen
constexpr uint32_t kGuitarStartupSpawned = 0x244002D2;  // guitar_startup_spawned
constexpr uint32_t kAutolaunchGo = 0x6BF94BA8;          // the script global autolaunch_go (-1)
constexpr uint32_t kSkipIntroMovie = 0x52B70BAA;        // the script global g_skip_intro_movie (0)

std::atomic<bool> g_logos_logged{false};
std::atomic<bool> g_done{false};

bool Enabled() {
  static const bool enabled = REXCVAR_GET(ghwor_skip_intro);
  return enabled;
}

}  // namespace

namespace ghwor {

bool IntroSkipPending() { return Enabled() && !g_done.load(std::memory_order_relaxed); }

bool IntroReadsAsOne(uint32_t script_name, uint32_t global) {
  if (!IntroSkipPending()) {
    return false;
  }
  if (global == kAutolaunchGo && script_name == kDestroyLoadingScreen) {
    if (!g_logos_logged.exchange(true)) {
      REXLOG_INFO("[intro] the game has loaded: the logos end (autolaunch_go read as 1 in destroy_loading_screen)");
    }
    return true;
  }
  if (global == kSkipIntroMovie && script_name == kGuitarStartupSpawned) {
    if (!g_done.exchange(true)) {
      REXLOG_INFO("[intro] the intro movie is not played (g_skip_intro_movie read as 1)");
    }
    return true;
  }
  return false;
}

}  // namespace ghwor
