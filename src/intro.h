// ghwor - the boot's videos shortened: the logos end as soon as the game has loaded, and the intro movie after them
// is not played (src/intro.cpp).

#pragma once

#include <cstdint>

namespace ghwor {

// Whether ghwor_skip_intro is on and the boot has not reached its intro movie yet: src/debug_menu.cpp's hooks on the
// script interpreter run while it is.
bool IntroSkipPending();

// Whether the script global `global`, read by the running script `script_name` (src/debug_menu.cpp's "$name" resolve
// hook), reads as the integer 1: autolaunch_go in destroy_loading_screen and g_skip_intro_movie in
// guitar_startup_spawned, while the intro is skipped.
bool IntroReadsAsOne(uint32_t script_name, uint32_t global);

}  // namespace ghwor
