// ghwor - the game's own debug menu, offered in the main menu and the pause menu; and the script interpreter's hooks
// that src/intro.cpp shares.
//
// The game ships Neversoft's debug menu (scripts\guitar\debug_menu_data.qb and debug_menu_funcs.qb): UIstate_debug
// (its layout in ui_debug.pak) opens debug_submenus' "main" menu: In Game (Play Song, Skip Into Song, Change Venue,
// Select Camera Cut, the lag tests, Force Score, Character Select, Complete All Current Challenges, ...), HUD & UI (Show
// Measures, Show Song Stars, Show Song Time, Show Song Name, Show Input Log, the band's, crowd's and stage's
// visibility, ...), Band Setup (Bot Instructions), Tools (Check CAS Assets, Dump Heaps, Autosave, Play Credits,
// Difficulty Analyzer, ...), Note-Tracking Debug, Screenshot Utilities, Add 100 Quickplay Stars, Strict Small Pools,
// Unlock All. The main menu (create_main_menu_elements, scripts\guitar\guitar_main_menu.qb) adds its item,
// "(debug menu)" (g_mainmenu_text's debugmenu, from mainmenu_populate_menu_options_global), when the script global
// enable_button_cheats is 1 and enable_debug_menus is not 0 (1, never changed); the pause menu (ui_create_pausemenu)
// adds "Debug Menu" when it is 1, outside the tutorials. This game's guitar.qb sets enable_button_cheats to 1 and
// guitar_startup_initialize sets it to 0 "if DemoBuild" and "if RetailBuild", as GH5's. The only other scripts that
// read it are guitar_startup_systems (LaunchViewer, the PC viewer, qbr_callback_setup and select_shift) and
// cameracuts_iterator (a "No Cameras/Perf File for this Song" text in a song without one). None of these scripts is
// one of the 98 the game runs compiled (scriptstocompile, src/framerate.cpp): the interpreter runs them all.
//
// So the global keeps its value and only the menus' two scripts read it as 1. A "$name" in a script is a component of
// type 13 (or 26, a global naming another) with the name's key at +8, which sub_8239B040 replaces by the global's value
// (an integer: type 1 at +2, the value at +8). The script interpreter's statement (sub_823A4820, r31 = the script)
// evaluates its expressions with sub_823E6600 at two places; hooks there keep, per thread, the scripts whose
// expressions are being evaluated, and the running script's name is at +244 of it (as GH5's; GH Metallica's +216; the
// same offset sub_823A4820 itself reads, and src/framerate.cpp's). GH5's sub_823B3220, sub_8254BB50 and sub_823AAB98
// (tools/port/map2.pkl), matched instruction by instruction. Analysis: GH Metallica's local_docs/43-debug-menu.md,
// GH5's local_docs/23-gh-metallica-1.0-port.md, this repository's local_docs/17-gh-metallica-1.0-port.md.

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <set>

#include "intro.h"

REXCVAR_DEFINE_BOOL(ghwor_debug_menu, false, "GHWoR",
                    "The game's own debug menu (Neversoft's): an item in the main menu and the pause menu that opens "
                    "it; false = as on the console");

namespace {

constexpr uint32_t kEnableButtonCheats = 0x2AF92804;      // the script global enable_button_cheats
constexpr uint32_t kEnableDebugMenus = 0x69E6470E;        // the script global enable_debug_menus (the main menu's)
constexpr uint32_t kCreateMainMenuElements = 0x01B68F5B;  // the main menu's items
constexpr uint32_t kUiCreatePauseMenu = 0x09E116BF;       // the pause menu's items
constexpr uint8_t kTypeInteger = 1;
constexpr uint8_t kTypeName = 13;
constexpr uint8_t kTypeGlobalAlias = 26;
constexpr uint32_t kScriptName = 244;  // the running script's name in the interpreter's script object

bool DebugMenu() {
  static const bool enabled = REXCVAR_GET(ghwor_debug_menu);
  return enabled;
}

// Whether the hooks have anything to do: the debug menu, or the intro's skip until the boot's intro movie.
bool Active() { return DebugMenu() || ghwor::IntroSkipPending(); }

uint8_t* GuestBase() {
  auto* kernel = rex::system::kernel_state();
  return kernel && kernel->memory() ? kernel->memory()->virtual_membase() : nullptr;
}

uint32_t Load32(const uint8_t* base, uint32_t address) {
  uint32_t value;
  std::memcpy(&value, base + address, 4);
  return __builtin_bswap32(value);
}

void Store32(uint8_t* base, uint32_t address, uint32_t value) {
  value = __builtin_bswap32(value);
  std::memcpy(base + address, &value, 4);
}

// The scripts whose expressions this thread is evaluating, innermost last (an expression can run another script).
struct Evaluations {
  std::array<uint32_t, 64> scripts{};
  uint32_t depth = 0;
};
thread_local Evaluations t_evaluations;

void LogOnce(uint32_t script, bool shown) {
  static std::mutex mutex;
  static std::set<uint32_t> logged;
  std::lock_guard lock(mutex);
  if (logged.size() < 32 && logged.insert(script).second) {
    if (shown) {
      REXLOG_INFO("[debug menu] script {:08X} offers the debug menu (enable_button_cheats read as 1)", script);
    } else {
      REXLOG_INFO("[debug menu] script {:08X} reads enable_button_cheats: left at 0", script);
    }
  }
}

// Whether `global`, read by the script named `script`, reads as 1 for the debug menu.
bool DebugMenuReadsAsOne(uint32_t script, uint32_t global) {
  if (!DebugMenu() || (global != kEnableButtonCheats && global != kEnableDebugMenus)) {
    return false;
  }
  const bool menu = global == kEnableDebugMenus
                        ? script == kCreateMainMenuElements
                        : script == kCreateMainMenuElements || script == kUiCreatePauseMenu;
  if (global == kEnableButtonCheats) {
    LogOnce(script, menu);
  }
  return menu;
}

}  // namespace

// [[midasm_hook]] 0x823A49B8 and 0x823A4D20 (sub_823A4820, before each bl sub_823E6600): r31 = the script.
void GHWoRScriptEvalEnterHook(PPCRegister& r31) {
  if (!Active()) {
    return;
  }
  Evaluations& e = t_evaluations;
  if (e.depth < e.scripts.size()) {
    e.scripts[e.depth] = r31.u32;
  }
  ++e.depth;
}

// [[midasm_hook]] 0x823A49C0 and 0x823A4D24 (sub_823A4820, after each bl sub_823E6600).
void GHWoRScriptEvalLeaveHook() {
  if (!Active()) {
    return;
  }
  Evaluations& e = t_evaluations;
  if (e.depth > 0) {
    --e.depth;
  }
}

// [[midasm_hook]] 0x8239B040 (the entry of sub_8239B040: a "$name" component resolved to the global's value),
// return_on_true: r3 = the component. enable_button_cheats read by the menus' scripts, enable_debug_menus by the main
// menu's (with ghwor_debug_menu), and autolaunch_go and g_skip_intro_movie by the boot's scripts (with
// ghwor_skip_intro, src/intro.cpp) become the integer 1.
bool GHWoRGlobalResolveHook(PPCRegister& r3) {
  if (!Active()) {
    return false;
  }
  uint8_t* base = GuestBase();
  const uint32_t component = r3.u32;
  if (!base || !component) {
    return false;
  }
  const uint8_t type = base[component + 2];
  if (type != kTypeName && type != kTypeGlobalAlias) {
    return false;
  }
  const Evaluations& e = t_evaluations;
  if (e.depth == 0 || e.depth > e.scripts.size() || !e.scripts[e.depth - 1]) {
    return false;
  }
  const uint32_t global = Load32(base, component + 8);
  const uint32_t script = Load32(base, e.scripts[e.depth - 1] + kScriptName);
  if (!DebugMenuReadsAsOne(script, global) && !ghwor::IntroReadsAsOne(script, global)) {
    return false;
  }
  base[component + 2] = kTypeInteger;
  Store32(base, component + 8, 1);
  return true;
}
