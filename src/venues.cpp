// ghwor - the launcher's Mods > Stage > Venues in Quickplay: the venues the game may pick at random (docs/mods.md,
// local_docs/82). All on by default, as on the console.
//
// quickplay_choose_random_venue (quickplay_menu_flow.qb) picks the venue of Quickplay+ (at the start and after each
// song), Party Play and online play: over LevelZoneArray (venuelist.qb), it keeps the venues the save has unlocked
// (is_venue_unlocked) that are not in g_levels_to_remove_from_random_rotation, the developers' own list of zones left out
// (empty in the retail data, read by this script only):
//   if StructureContains structure = $g_levels_to_remove_from_random_rotation ($LevelZones.<level_checksum>.zone)
// and picks one at random (none left: Default_Venue, CBGB's). A venue switched off is in that list: StructureContains
// (sub_820F1778) answers 1 for its zone when the running script (the script object's +244) is that one. The thirteen
// venues are LevelZones' entries without debug_only; their names are the same in every language. Party Play's pause
// menu and the lobby keep their own choice of a venue.

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <string_view>

REXCVAR_DEFINE_BOOL(ghwor_venue_beast_dethroned, true, "GHWoR", "Beast Dethroned in Quickplay's random venues");
REXCVAR_DEFINE_BOOL(ghwor_venue_cbgbs, true, "GHWoR", "CBGB's in Quickplay's random venues");
REXCVAR_DEFINE_BOOL(ghwor_venue_hypersphere, true, "GHWoR", "Hypersphere in Quickplay's random venues");
REXCVAR_DEFINE_BOOL(ghwor_venue_infernal_hall, true, "GHWoR", "Infernal Hall in Quickplay's random venues");
REXCVAR_DEFINE_BOOL(ghwor_venue_metallic_lab, true, "GHWoR", "Metallic Lab in Quickplay's random venues");
REXCVAR_DEFINE_BOOL(ghwor_venue_paramourn_manor, true, "GHWoR", "Paramourn Manor in Quickplay's random venues");
REXCVAR_DEFINE_BOOL(ghwor_venue_razorback_forest, true, "GHWoR", "Razorback Forest in Quickplay's random venues");
REXCVAR_DEFINE_BOOL(ghwor_venue_rush_2112, true, "GHWoR", "Rush - 2112 in Quickplay's random venues");
REXCVAR_DEFINE_BOOL(ghwor_venue_serpents_stronghold, true, "GHWoR",
                    "Serpent's Stronghold in Quickplay's random venues");
REXCVAR_DEFINE_BOOL(ghwor_venue_monument, true, "GHWoR", "The Monument in Quickplay's random venues");
REXCVAR_DEFINE_BOOL(ghwor_venue_stage_of_kings, true, "GHWoR", "The Stage of Kings in Quickplay's random venues");
REXCVAR_DEFINE_BOOL(ghwor_venue_thorn_fortress, true, "GHWoR", "Thorn Fortress in Quickplay's random venues");
REXCVAR_DEFINE_BOOL(ghwor_venue_wormhole, true, "GHWoR", "Wormhole in Quickplay's random venues");

namespace {

constexpr uint32_t QbKey(std::string_view name) {
  uint32_t crc = 0xFFFFFFFF;
  for (char c : name) {
    crc ^= uint8_t(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
    for (int bit = 0; bit < 8; ++bit) crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
  }
  return crc;
}

constexpr uint32_t kChooseRandomVenue = QbKey("quickplay_choose_random_venue");
static_assert(kChooseRandomVenue == 0xF4456474);
constexpr uint32_t kName = QbKey("name");
constexpr uint32_t kScriptName = 244;  // the running script's name in the interpreter's script object
constexpr uint8_t kTypeName = 13;

struct Venue {
  const char* title;
  uint32_t zone;  // LevelZones' zone
  bool on;
};

// Read once, at the first call (the cvars are set by then).
std::array<Venue, 13>& Venues() {
  static std::array<Venue, 13> venues = {{
      {"Beast Dethroned", QbKey("Z_credits"), REXCVAR_GET(ghwor_venue_beast_dethroned)},
      {"CBGB's", QbKey("Z_PunkStage"), REXCVAR_GET(ghwor_venue_cbgbs)},
      {"Hypersphere", QbKey("Z_Cube"), REXCVAR_GET(ghwor_venue_hypersphere)},
      {"Infernal Hall", QbKey("Z_HouseBand"), REXCVAR_GET(ghwor_venue_infernal_hall)},
      {"Metallic Lab", QbKey("Z_RiotHouse"), REXCVAR_GET(ghwor_venue_metallic_lab)},
      {"Paramourn Manor", QbKey("Z_MansionStudio"), REXCVAR_GET(ghwor_venue_paramourn_manor)},
      {"Razorback Forest", QbKey("Z_Festival"), REXCVAR_GET(ghwor_venue_razorback_forest)},
      {"Rush - 2112", QbKey("Z_Modular"), REXCVAR_GET(ghwor_venue_rush_2112)},
      {"Serpent's Stronghold", QbKey("Z_Barbarian"), REXCVAR_GET(ghwor_venue_serpents_stronghold)},
      {"The Monument", QbKey("Z_FinalBattle"), REXCVAR_GET(ghwor_venue_monument)},
      {"The Stage of Kings", QbKey("Z_Egypt"), REXCVAR_GET(ghwor_venue_stage_of_kings)},
      {"Thorn Fortress", QbKey("Z_EpicStage"), REXCVAR_GET(ghwor_venue_thorn_fortress)},
      {"Wormhole", QbKey("z_visualizer"), REXCVAR_GET(ghwor_venue_wormhole)},
  }};
  return venues;
}

// Whether any venue is left out: none, or all of them (then the console's choice, and a warning).
bool Active() {
  static const bool active = [] {
    std::string off;
    int count = 0;
    for (const Venue& v : Venues()) {
      if (!v.on) {
        off += off.empty() ? "" : ", ";
        off += v.title;
        ++count;
      }
    }
    if (count == int(Venues().size())) {
      REXLOG_WARN("[venues] every venue switched off: Quickplay picks among all of them, as on the console");
      return false;
    }
    if (count) {
      REXLOG_INFO("[venues] left out of the random venue in Quickplay, Party Play and online play: {}", off);
    }
    return count > 0;
  }();
  return active;
}

uint8_t* Base() {
  auto* kernel = rex::system::kernel_state();
  return kernel && kernel->memory() ? kernel->memory()->virtual_membase() : nullptr;
}

bool Plausible(uint32_t address) { return address >= 0x10000 && address < 0xFFFF0000 && !(address & 3); }

uint32_t Load32(const uint8_t* base, uint32_t address) {
  uint32_t v;
  std::memcpy(&v, base + address, 4);
  return __builtin_bswap32(v);
}

// StructureContains' key: its `name` parameter, else its first checksum without a name (sub_820F1778's own order).
uint32_t Key(const uint8_t* base, uint32_t params) {
  if (!Plausible(params)) return 0;
  uint32_t unnamed = 0;
  uint32_t c = Load32(base, params + 4);
  for (int guard = 0; Plausible(c) && guard < 64; ++guard) {
    if (base[c + 2] == kTypeName) {
      const uint32_t name = Load32(base, c + 4);
      if (name == kName) return Load32(base, c + 8);
      if (name == 0 && !unnamed) unnamed = Load32(base, c + 8);
    }
    c = Load32(base, c + 12);
  }
  return unnamed;
}

std::atomic<int> g_logged{0};

}  // namespace

// [[midasm_hook]] 0x820F1778 (the entry of sub_820F1778, the script function StructureContains; r3 = its params, r4 =
// the script object), return_on_true: inside quickplay_choose_random_venue, a zone switched off is in the list of zones
// left out (r3 = 1).
bool GHWoRVenueRotationHook(PPCRegister& r3, PPCRegister& r4) {
  if (!Active()) return false;
  const uint8_t* base = Base();
  if (!base || !Plausible(r4.u32) || Load32(base, r4.u32 + kScriptName) != kChooseRandomVenue) return false;
  const uint32_t key = Key(base, r3.u32);
  for (const Venue& v : Venues()) {
    if (v.zone == key && !v.on) {
      if (g_logged.fetch_add(1) < 13) {
        REXLOG_INFO("[venues] {} left out of the random choice", v.title);
      }
      r3.u64 = 1;
      return true;
    }
  }
  return false;
}
