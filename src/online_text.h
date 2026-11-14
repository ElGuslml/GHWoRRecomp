// ghwor - the online menus as on the PlayStation 3, and no Facebook / Twitter prompt (src/online_text.cpp).

#pragma once

#include <cstdint>

namespace ghwor {

// Whether ghwor_online_text and online_play are on: src/debug_menu.cpp's hooks on the script interpreter run then (the
// sharing prompt is asked only of a profile signed in to Xbox Live).
bool SocialPromptSkipActive();

// Whether the script global `global`, read by the running script `script_name` (src/debug_menu.cpp's "$name" resolve
// hook), reads as the integer 1: g_skip_social_networks in ui_create_social_networks_setup, while
// SocialPromptSkipActive().
bool OnlineReadsAsOne(uint32_t script_name, uint32_t global);

}  // namespace ghwor
