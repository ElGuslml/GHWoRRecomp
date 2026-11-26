// ghwor - the launcher's Mods > Gameplay: how the game takes the player's input (docs/mods.md, local_docs/82). Off by
// default, as on the console.
//
// - Sustain drop leniency (ghwor_sustain_leniency_ms, 0-200 ms; Clone Hero's setting of that name): a sustained note's
//   fret let go is still held for that long, so a sustain let go a moment early to reach the next note, or a fret
//   lifted for an instant, is not dropped; pressed again within it, the sustain goes on. sub_8218ED00 is a player's
//   sustain in each frame (local_docs/21): it reads the frets held (sub_82188020, anchored, into [r1 + 88]; 0 while
//   disable_note_input), keeps them in r29 and, from 0x8218EFC8 on, ends alone each sustaining fret (+0x181E0) not
//   held (sub_8218BB80, the loop at 0x8218F180), then ends the whole sustain when what is held is not the note
//   (0x8218F1E4..0x8218F290; finished = 0). The hook at 0x8218EFC8 adds to r29 the sustaining frets let go less than
//   the leniency ago, so both checks see them held; a sustain that reaches its end inside it ends as completed. The
//   open note (0x100000) needs no fret and is left out. While a fret is lenient the sustain goes on as if held: its
//   points and the whammy's Star Power go on too, at most the leniency's length each time.
// - Infinite front-end (ghwor_infinite_front_end; YARG's "HOPOs and Taps can be fretted before they enter the hit
//   window, and will be scored as they enter it", Clone Hero's too, a miss clearing it). sub_82180BA0, a player's input
//   in each frame, makes a hammer candidate only in a frame whose held frets ([r1 + 144]) differ from the last frame's
//   (player + 0x180D8, written back at 0x821815F4): the head note in a tap phrase needs that change (no chain), any
//   other HOPO also no strum ([r1 + 136]) and the last hit (+0x180C8, 0 after a miss or an overstrum) and a candidate
//   other than it; the notes then take it only inside the hammer window and with their own pattern. Guitar Hero III
//   (GH Aerosmith's sub_821235E0) has the same rule, its last frets written every frame (loc_82123DA0): the players'
//   "GH III infinite front-end" is not in that code. Here, once the
//   frets have changed since the player's last hit or miss ("armed"), every frame without a strum is made one with a
//   change at 0x821814E4 (the last frame's frets set to anything else), so the game's own checks build the candidate
//   from what is held and a HOPO or tap fretted early is hit as it enters the window. A hit (0x82181AB8, after
//   0x82181AB4 where every hit goes), a missed note (0x82180ED8) and an overstrum (0x82181DD8) disarm it, so frets
//   carried over from the last note never count again without a new change, and after a miss an early fret needs
//   fretting again. Strummed notes do not read the candidate. local_docs/82.

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <mutex>

REXCVAR_DEFINE_BOOL(ghwor_infinite_front_end, false, "GHWoR",
                    "HOPOs and taps fretted before they reach the hit window count as they enter it, as in Clone Hero "
                    "and YARG (a miss clears it); false = as on the console");
REXCVAR_DEFINE_INT32(ghwor_sustain_leniency_ms, 0, "GHWoR",
                     "Sustain drop leniency in milliseconds (0-200 by 5 in the launcher): a sustained note's fret let "
                     "go is still held for that long, so letting go a moment early or lifting a fret for an instant "
                     "does not drop the sustain; 0 = as on the console");

namespace {

constexpr uint32_t kSustaining = 0x181E0;  // the player's frets sustaining
constexpr uint32_t kFrets[5] = {0x1, 0x10, 0x100, 0x1000, 0x10000};

int32_t LeniencyMs() {
  static const int32_t ms = std::clamp(REXCVAR_GET(ghwor_sustain_leniency_ms), 0, 500);
  return ms;
}

uint8_t* Base() {
  auto* kernel = rex::system::kernel_state();
  return kernel && kernel->memory() ? kernel->memory()->virtual_membase() : nullptr;
}

uint32_t Load32(const uint8_t* base, uint32_t address) {
  uint32_t v;
  std::memcpy(&v, base + address, 4);
  return __builtin_bswap32(v);
}

double NowMs() {
  using namespace std::chrono;
  return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

// Each player's object and when each fret was last held.
struct Player {
  uint32_t object = 0;
  std::array<double, 5> held_at{-1e12, -1e12, -1e12, -1e12, -1e12};
};
std::mutex g_mutex;
std::array<Player, 8> g_players;

Player& PlayerFor(uint32_t object) {
  for (Player& p : g_players) {
    if (p.object == object) return p;
  }
  for (Player& p : g_players) {
    if (!p.object) {
      p.object = object;
      return p;
    }
  }
  g_players[0] = Player{};
  g_players[0].object = object;
  return g_players[0];
}

std::atomic<int> g_logged{0};

constexpr uint32_t kLastFrets = 0x180D8;  // the player's frets held in the last frame

bool FrontEnd() {
  static const bool on = REXCVAR_GET(ghwor_infinite_front_end);
  return on;
}

void Store32(uint8_t* base, uint32_t address, uint32_t value) {
  value = __builtin_bswap32(value);
  std::memcpy(base + address, &value, 4);
}

// Each player's "the frets changed since the last hit or miss".
struct Armed {
  uint32_t object = 0;
  bool armed = false;
};
std::array<Armed, 8> g_armed;

Armed& ArmedFor(uint32_t object) {
  for (Armed& a : g_armed) {
    if (a.object == object) return a;
  }
  for (Armed& a : g_armed) {
    if (!a.object) {
      a.object = object;
      return a;
    }
  }
  g_armed[0] = Armed{object, false};
  return g_armed[0];
}

void Disarm(uint32_t object) {
  if (!FrontEnd() || !object) return;
  ArmedFor(object).armed = false;
}

std::atomic<int> g_front_end_logged{0};

}  // namespace

// [[midasm_hook]] 0x821814E4 (sub_82180BA0, loc_821814E4: before the hammer candidate is made), r1 (the frets held at
// +144, the strum at +136), r27 = the player. Only this function (and the reset sub_82189520) reads +0x180D8, and the
// real frets are written back at 0x821815F4 in the same frame.
void GHWoRFrontEndHook(PPCRegister& r1, PPCRegister& r27) {
  if (!FrontEnd() || !r27.u32) return;
  uint8_t* base = Base();
  if (!base) return;
  const uint32_t held = Load32(base, r1.u32 + 144);
  const uint32_t last = Load32(base, r27.u32 + kLastFrets);
  Armed& player = ArmedFor(r27.u32);
  if (held != last) {
    player.armed = true;
    return;
  }
  if (!player.armed || Load32(base, r1.u32 + 136) != 0) return;
  Store32(base, r27.u32 + kLastFrets, ~held);
  if (g_front_end_logged.fetch_add(1) == 0) {
    REXLOG_INFO("[gameplay] infinite front-end: frets held since before the note are taken as it enters the window "
                "(ghwor_infinite_front_end)");
  }
}

// [[midasm_hook]] 0x82181AB8 (sub_82180BA0, right after 0x82181AB4 where every hit goes), r27 = the player.
void GHWoRFrontEndHitHook(PPCRegister& r27) { Disarm(r27.u32); }

// [[midasm_hook]] 0x82180ED8 (sub_82180BA0, a note missed: the last hit set to 0), r27 = the player.
void GHWoRFrontEndMissHook(PPCRegister& r27) { Disarm(r27.u32); }

// [[midasm_hook]] 0x82181DD8 (sub_82180BA0, an overstrum: the last hit set to 0), r27 = the player.
void GHWoRFrontEndOverstrumHook(PPCRegister& r27) { Disarm(r27.u32); }

// [[midasm_hook]] 0x8218EFC8 (sub_8218ED00, a player's sustain in a frame: loc_8218EFC8, mr r22,r29): r29 = the frets
// held, r31 = the player.
void GHWoRSustainHeldHook(PPCRegister& r29, PPCRegister& r31) {
  const int32_t leniency = LeniencyMs();
  if (leniency <= 0) return;
  const uint8_t* base = Base();
  if (!base || !r31.u32) return;
  const uint32_t held = r29.u32;
  const uint32_t sustaining = Load32(base, r31.u32 + kSustaining);
  const double now = NowMs();
  uint32_t kept = 0;
  double oldest = 0.0;
  {
    std::lock_guard lock(g_mutex);
    Player& player = PlayerFor(r31.u32);
    for (int i = 0; i < 5; ++i) {
      if (held & kFrets[i]) {
        player.held_at[i] = now;
      } else if ((sustaining & kFrets[i]) && now - player.held_at[i] <= leniency) {
        kept |= kFrets[i];
        oldest = std::max(oldest, now - player.held_at[i]);
      }
    }
  }
  if (!kept) return;
  r29.u64 = held | kept;
  if (g_logged.fetch_add(1) < 6) {
    REXLOG_INFO("[gameplay] a sustain's fret {:05X} let go {:.0f} ms ago still held (ghwor_sustain_leniency_ms {})",
                kept, oldest, leniency);
  }
}
