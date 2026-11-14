// ghwor - the online menus as on the PlayStation 3: "online", not "Xbox LIVE", and no Xbox LIVE Party item.
//
// Online play here is between friends through Steam (the SDK's online_play), not Xbox Live. The band lobby's scripts
// pick their texts by platform (band_lobby_setup_strings: band_lobby_xenon_strings if IsXenonOrWinDX, else
// band_lobby_ps3_strings, in band_lobby_data.qb; update_game_rule_box and the others the same way), and every
// language's pack on the Xbox disc has both (qs, qs_f, qs_g, qs_i, qs_s.pak.xen): "FIND Xbox LIVE PLAYERS" and the
// PlayStation 3's "FIND ONLINE PLAYERS" ("BUSCAR JUGADORES EN LÍNEA", "TROUVER DES JOUEURS EN LIGNE", ...). The game
// turns a text's key into its text in the player's language with sub_823D7288 (r3 = the key; a hash lookup,
// sub_82521418, in the table at 0x82994AC0; called by the qs component of GetText, sub_82395F90, its wide twin
// sub_823965A0, and 38 other sites): asked for one of the Xbox LIVE texts below, it is given the key of its PlayStation
// 3 twin. The texts that name Xbox LIVE without a neutral twin (the sign-in and privilege messages, the downloadable
// content warning; their PS3 texts name the PlayStation Network) are left as they are. (GH Metallica's
// src/online_text.cpp does the same for its main menu's item, in its own text function, sub_823AA990.)
//
// The Options menu's "Xbox LIVE PARTY" item (the console's party chat, Xbox LIVE Party: XamShowCommunitySessionsUI, which
// fails here) is added by ui_create_options only `if IsXenonOrWinDX`; the script function IsXENONorWINDX (sub_82504670,
// a leaf: 1 when the platform at 0x829A78F4 is 6 or 10) answers 0 inside that script, which then builds the PS3's
// menu, without the item (its other IsXenonOrWinDX test, can_saveload's, only adds the console's guest check on the
// first controller, which never applies here). The script functions are called by the interpreter with r3 = their
// parameters and r4 = the script object (sub_823A4820: mr r4,r31; bctrl), the running script's name at +244. The
// band lobby's "INVITE Xbox LIVE PARTY" hint needs nothing: it is shown only when XamPartyGetUserList finds party
// members, and the SDK answers that there is no party.
//
// The sharing prompt: once the boot's save icon notice is accepted, UIstate_social_networks_setup runs
// ui_create_social_networks_setup (scripts\guitar\menu\states\layouts\ui_social_networks_setup.qb). For a profile
// signed in to Xbox Live at its first online start (social_networks_is_first_boot), it waits up to 6 s for Demonware's
// lobby ("connecting"), then asks "Do you want to login to your Facebook or Twitter accounts to share?" (qs 3DD423A8)
// unless both are registered. Those services are gone. The script's first test is the developers' own skip,
// `if ($g_skip_social_networks = 1 OR ...)` (its only reader), which goes to the main menu at once: that global reads
// as 1 inside it (src/debug_menu.cpp's "$name" resolve hook), so the prompt and its wait never come. The Options menu's
// Share item is left as it is.

#include "online_text.h"

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>

REXCVAR_DEFINE_BOOL(ghwor_online_text, true, "GHWoR",
                    "The online menus as on the PlayStation 3, as online play here is through Steam: \"FIND ONLINE "
                    "PLAYERS\" and the other lobby texts that name Xbox LIVE in the game's PS3 words (in its language), "
                    "and no Xbox LIVE Party item in Options; with online play, no Facebook / Twitter prompt at a "
                    "profile's first online start (false: as on the Xbox 360)");

namespace {

struct TextTwin {
  uint32_t xbox;
  uint32_t ps3;
};

// The Xbox LIVE texts and their PlayStation 3 twins (English texts).
constexpr std::array<TextTwin, 7> kTwins{{
    {0xB7FBD401, 0x1602010C},  // the band lobby's button: "FIND Xbox LIVE PLAYERS" -> "FIND ONLINE PLAYERS"
    {0x541C0676, 0xF9000FFE},  // the lobby's ticker (more_players): "...find players on Xbox LIVE." -> "online"
    {0x6098B11A, 0x5567CB79},  // update_game_rule_box: "...try and find players on Xbox LIVE." -> "online"
    {0x79E058EC, 0x5E58CC98},  // update_game_rule_box: "...or go on to Xbox LIVE." -> "...or go online."
    {0xEBA184F9, 0x93A3822E},  // handle_matchmaking_handoff_failed: "...connecting to an Xbox LIVE game." -> "online"
    {0x4E216451, 0x366E10C6},  // net_party_attempted_join_result: "Cannot join the Xbox LIVE party. Insufficient slots"
    {0x3C913DD8, 0x33923E7F},  // the same, bad_state: "Cannot join the Xbox LIVE party. Session no longer available."
}};

constexpr uint32_t kUiCreateOptions = 0xBB979C0C;  // the Options menu's items
constexpr uint32_t kScriptName = 244;               // the running script's name in the interpreter's script object
constexpr uint32_t kSkipSocialNetworks = 0x9F4A94FA;          // the script global g_skip_social_networks
constexpr uint32_t kUiCreateSocialNetworksSetup = 0x66219041;  // the boot's sharing prompt

std::array<std::atomic<bool>, kTwins.size()> g_logged{};
std::atomic<bool> g_party_logged{false};
std::atomic<bool> g_social_logged{false};

bool Enabled() {
  static const bool enabled = REXCVAR_GET(ghwor_online_text);
  return enabled;
}

uint8_t* GuestBase() {
  auto* kernel = rex::system::kernel_state();
  return kernel && kernel->memory() ? kernel->memory()->virtual_membase() : nullptr;
}

uint32_t Load32(const uint8_t* base, uint32_t address) {
  uint32_t value;
  std::memcpy(&value, base + address, 4);
  return __builtin_bswap32(value);
}

}  // namespace

namespace ghwor {

bool SocialPromptSkipActive() {
  static const bool active = Enabled() && rex::cvar::Query<bool>("online_play");
  return active;
}

bool OnlineReadsAsOne(uint32_t script_name, uint32_t global) {
  if (global != kSkipSocialNetworks || script_name != kUiCreateSocialNetworksSetup || !SocialPromptSkipActive()) {
    return false;
  }
  if (!g_social_logged.exchange(true)) {
    REXLOG_INFO("[online_text] the Facebook / Twitter sharing prompt skipped (g_skip_social_networks read as 1 in "
                "ui_create_social_networks_setup; ghwor_online_text)");
  }
  return true;
}

}  // namespace ghwor

// [[midasm_hook]] at 0x823D7298 (mr r4,r3) in sub_823D7288, a text's key -> its text in the player's language: r3 =
// the key.
void GHWoRTextKeyHook(PPCRegister& r3) {
  if (!Enabled()) {
    return;
  }
  for (size_t i = 0; i < kTwins.size(); ++i) {
    if (r3.u32 == kTwins[i].xbox) {
      r3.u64 = kTwins[i].ps3;
      if (!g_logged[i].exchange(true)) {
        REXLOG_INFO("[online_text] text {:08X} (Xbox LIVE) shown as the game's PS3 text {:08X} (ghwor_online_text)",
                    kTwins[i].xbox, kTwins[i].ps3);
      }
      return;
    }
  }
}

// [[midasm_hook]] at 0x82504670 (the entry of sub_82504670, the script function IsXENONorWINDX), return_on_true: r3 =
// its parameters, r4 = the script object. Inside ui_create_options it answers 0 (r3), so the Options menu is built
// without the Xbox LIVE Party item; everywhere else it runs.
bool GHWoRPlatformHook(PPCRegister& r3, PPCRegister& r4) {
  if (!Enabled()) {
    return false;
  }
  const uint8_t* base = GuestBase();
  if (!base || !r4.u32 || Load32(base, r4.u32 + kScriptName) != kUiCreateOptions) {
    return false;
  }
  r3.u64 = 0;
  if (!g_party_logged.exchange(true)) {
    REXLOG_INFO("[online_text] the Options menu built without its Xbox LIVE Party item (ghwor_online_text)");
  }
  return true;
}
