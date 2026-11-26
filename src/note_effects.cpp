// ghwor - the launcher's Mods > Highway > Note effects: the flames and sparks the game draws on the strike line, each
// hidden by a switch of its own (docs/mods.md, local_docs/82). All on as on the console.
//
// - The burst on a hit (ghwor_fx_hit_burst) and Star Power's blue one (ghwor_fx_star_power_burst): GuitarEvent_HitNote
//   (guitar_events.qb) spawns GuitarEvent_HitNote_Spawned, which calls the C function NoteFX (sub_821E63A8) with the
//   note's place, `star` = the player's star_power_used and the fret's name. NoteFX emits into five particle systems made
//   at the start (one per fret); NoteFX {reset} (qbr_callback_guitar_events_post) only asks for them to be reset at its
//   next call. Hidden: NoteFX leaves right after reading `star` (0x821E64A4), before emitting, as it leaves after a
//   reset (0x821E6A08).
// - A sustain's sparks (ghwor_fx_sustain): GuitarEvent_WhammyOn calls WhammyFXOn (sub_821E6A20) with the player and the
//   held pattern; it calls sub_822BC8E0 (left as it is), then for each fret of the pattern starts the strike line's
//   effect (sub_8219F5B0, sub_8219F3A0). Hidden: the pattern read as 0 by that loop (0x821E6AAC), so nothing starts;
//   WhammyFXOff stops what is not running. The bass's open sustains are GuitarEvent_Open_Note_Sustain's instead: a
//   particle system of mat_particle_kickhit_5l in Open_Note_Sustain_FX_Cont_p<n>, hidden with the next.
// - The open note's and the kick's wave (ghwor_fx_open_note): guitarevent_kick_drum_hit_note makes two 2D particle
//   systems (mat_particle_kickhit_ring01, then _4l or _5l) for 11 game frames and destroys them. Create2DParticleSystem
//   (sub_822B78E0) is asked for nothing then: it answers 0, as it does when its 44 systems are all in use (the scripts
//   already live with that), and fastdestroy2dparticlesystem finds no system of that id.

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

REXCVAR_DEFINE_BOOL(ghwor_fx_hit_burst, true, "GHWoR",
                    "The burst of flames and sparks on the strike line when you hit a note; false hides it. true = as on "
                    "the console");
REXCVAR_DEFINE_BOOL(ghwor_fx_star_power_burst, true, "GHWoR",
                    "The blue burst on a hit while your Star Power is on; false hides it. true = as on the console");
REXCVAR_DEFINE_BOOL(ghwor_fx_sustain, true, "GHWoR",
                    "The sparks on the strike line while you hold a sustained note (and the bass's open sustains' "
                    "glow); false hides them. true = as on the console");
REXCVAR_DEFINE_BOOL(ghwor_fx_open_note, true, "GHWoR",
                    "The wave across the highway when you hit an open note or a kick; false hides it. true = as on the "
                    "console");

namespace {

constexpr uint32_t QbKey(std::string_view name) {
  uint32_t crc = 0xFFFFFFFF;
  for (char c : name) {
    crc ^= uint8_t(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
    for (int bit = 0; bit < 8; ++bit) crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
  }
  return crc;
}

constexpr uint32_t kMaterial = QbKey("material");
constexpr uint32_t kParent = QbKey("parent");
constexpr uint32_t kKickRing = QbKey("mat_particle_kickhit_ring01");
constexpr uint32_t kKickFourLanes = QbKey("mat_particle_kickhit_4l");
constexpr uint32_t kKickFiveLanes = QbKey("mat_particle_kickhit_5l");
static_assert(kKickRing == 0x32910AD1 && kKickFourLanes == 0x1508AEA0 && kKickFiveLanes == 0x0C139FE1);
constexpr uint8_t kTypeName = 13;

bool HitBurst() {
  static const bool on = REXCVAR_GET(ghwor_fx_hit_burst);
  return on;
}
bool StarPowerBurst() {
  static const bool on = REXCVAR_GET(ghwor_fx_star_power_burst);
  return on;
}
bool Sustain() {
  static const bool on = REXCVAR_GET(ghwor_fx_sustain);
  return on;
}
bool OpenNote() {
  static const bool on = REXCVAR_GET(ghwor_fx_open_note);
  return on;
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

// The value of a params struct's checksum component of that name, or 0 (type at +2, name at +4, value at +8, next at
// +12; the first at the struct's +4).
uint32_t NameParam(const uint8_t* base, uint32_t params, uint32_t name) {
  if (!Plausible(params)) return 0;
  uint32_t c = Load32(base, params + 4);
  for (int guard = 0; Plausible(c) && guard < 64; ++guard) {
    if (Load32(base, c + 4) == name) {
      return base[c + 2] == kTypeName ? Load32(base, c + 8) : 0;
    }
    c = Load32(base, c + 12);
  }
  return 0;
}

// Open_Note_Sustain_FX_Cont_p<n>, the bass's open sustain's container (FormatText checksumname).
bool OpenSustainContainer(uint32_t key) {
  for (int p = 1; p <= 4; ++p) {
    if (key == QbKey("Open_Note_Sustain_FX_Cont_p" + std::to_string(p))) return true;
  }
  return false;
}

void LogOnce(std::atomic<bool>& logged, const char* what) {
  if (!logged.exchange(true)) {
    REXLOG_INFO("[note effects] {} hidden", what);
  }
}

std::atomic<bool> g_burst_logged{false};
std::atomic<bool> g_star_logged{false};
std::atomic<bool> g_sustain_logged{false};
std::atomic<bool> g_open_sustain_logged{false};
std::atomic<bool> g_open_logged{false};

}  // namespace

// [[midasm_hook]] 0x821E64A4 (NoteFX, sub_821E63A8: `star` read into r28), jump_address_on_true = 0x821E6A08 (its end,
// returning 1): the burst not emitted.
bool GHWoRNoteFxHook(PPCRegister& r28) {
  if (r28.u32 != 0) {
    if (StarPowerBurst()) return false;
    LogOnce(g_star_logged, "Star Power's burst on a hit (ghwor_fx_star_power_burst)");
    return true;
  }
  if (HitBurst()) return false;
  LogOnce(g_burst_logged, "the burst on a hit (ghwor_fx_hit_burst)");
  return true;
}

// [[midasm_hook]] 0x821E6AAC (WhammyFXOn, sub_821E6A20: after sub_822BC8E0, before the loop over the pattern's frets at
// [r1 + 84]): the pattern read as 0 by the loop, so no fret's effect starts.
void GHWoRWhammyFxHook(PPCRegister& r1) {
  if (Sustain()) return;
  if (uint8_t* base = Base()) {
    std::memset(base + r1.u32 + 84, 0, 4);
    LogOnce(g_sustain_logged, "a sustain's sparks (ghwor_fx_sustain)");
  }
}

// [[midasm_hook]] 0x822B78E0 (the entry of sub_822B78E0, Create2DParticleSystem; r3 = its params), return_on_true: the
// open note's and the kick's wave, and the bass's open sustain, not made (r3 = 0, as with no system free).
bool GHWoRParticleSystemHook(PPCRegister& r3) {
  if (OpenNote() && Sustain()) return false;
  const uint8_t* base = Base();
  if (!base) return false;
  const uint32_t material = NameParam(base, r3.u32, kMaterial);
  if (material != kKickRing && material != kKickFourLanes && material != kKickFiveLanes) return false;
  if (material == kKickFiveLanes && OpenSustainContainer(NameParam(base, r3.u32, kParent))) {
    if (Sustain()) return false;
    LogOnce(g_open_sustain_logged, "the bass's open sustain's glow (ghwor_fx_sustain)");
  } else {
    if (OpenNote()) return false;
    LogOnce(g_open_logged, "the open note's and the kick's wave (ghwor_fx_open_note)");
  }
  r3.u64 = 0;
  return true;
}
