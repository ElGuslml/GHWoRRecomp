// ghwor - online matchmaking without its wait for strangers.
//
// Online play here is between friends (the SDK's online_play): the players are a party of friends, who follow its
// leader into the match at once. The game's matchmaking also waits for strangers to fill the match's empty places, as
// on Xbox Live: its matchmaker (its times set by sub_82519660, called by its two constructors sub_8251AFD0 and
// sub_8251B158) hosts the match for net_hosting_duration_secs (15, in the disc's net_live_settings) plus up to 4 random
// seconds (+92, in ms, from its start at +88), then searches for other hosts to merge with, up to
// net_reservation_max_cycles (6, +100) times (the cycles counted at +96), with net_hosting_game_acceptable_wait (30 s,
// +104, ms) as its other limit. sub_8251AB08 is its hosting tick (now - start past +92: a cycle). GH Metallica's
// matchmaker (sub_823F7800) is the same with its fields 4 bytes lower; there a band of friends waited about 46 s for
// no one.
//
// So the hosting time and the acceptable wait are made ghwor_online_host_wait seconds (and that + 2) when the matchmaker
// is built. Two separate groups of friends that want to find each other meet in that window: 0 keeps the game's own
// times.

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <algorithm>
#include <cstdint>
#include <cstring>

REXCVAR_DEFINE_INT32(ghwor_online_host_wait, 3, "GHWoR",
                     "Online play: the seconds the game's matchmaking hosts a match waiting for players who are not "
                     "in the party (strangers on Xbox Live; here, another group of friends). 0 = the game's own (15 s "
                     "plus up to 4, and up to 30 s in all), for two groups that look for each other");

namespace {

uint8_t* GuestBase() {
  auto* kernel = rex::system::kernel_state();
  return kernel && kernel->memory() ? kernel->memory()->virtual_membase() : nullptr;
}

uint32_t LoadWord(const uint8_t* base, uint32_t address) {
  uint32_t value;
  std::memcpy(&value, base + address, 4);
  return __builtin_bswap32(value);
}

void StoreWord(uint8_t* base, uint32_t address, uint32_t value) {
  value = __builtin_bswap32(value);
  std::memcpy(base + address, &value, 4);
}

// The matchmaker's fields (sub_82519660).
constexpr uint32_t kHostingDurationMs = 92;
constexpr uint32_t kAcceptableWaitMs = 104;

}  // namespace

// [[midasm_hook]] at 0x825196CC in sub_82519660 (the matchmaker's times, read from the settings), after its last store:
// r31 = the matchmaker.
void GHWoRMatchmakerHook(PPCRegister& r31) {
  const int32_t wait = REXCVAR_GET(ghwor_online_host_wait);
  uint8_t* base = GuestBase();
  if (wait <= 0 || !base || !r31.u32) {
    return;
  }
  const uint32_t hosting = LoadWord(base, r31.u32 + kHostingDurationMs);
  const uint32_t acceptable = LoadWord(base, r31.u32 + kAcceptableWaitMs);
  const uint32_t new_hosting = uint32_t(wait) * 1000;
  const uint32_t new_acceptable = new_hosting + 2000;
  if (new_hosting < hosting) {
    StoreWord(base, r31.u32 + kHostingDurationMs, new_hosting);
  }
  if (new_acceptable < acceptable) {
    StoreWord(base, r31.u32 + kAcceptableWaitMs, new_acceptable);
  }
  REXLOG_INFO("[matchmaking] the match hosted {} ms for players outside the party (the game's {} ms), {} ms in all "
              "(the game's {} ms)",
              std::min(new_hosting, hosting), hosting, std::min(new_acceptable, acceptable), acceptable);
}
