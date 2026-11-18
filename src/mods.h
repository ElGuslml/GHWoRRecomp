// ghwor - the launcher's Mods page (src/mods.cpp).

#pragma once

namespace ghwor::mods {

// The mods' changes of the game's script data, from their cvars (src/script_globals.h); once, before the script
// interpreter's hooks first run (src/debug_menu.cpp).
void Configure();

}  // namespace ghwor::mods
