// ghwor - the song's clock from the vertical blanks' own times (ghwor_song_clock_exact; off: the game's own sums), and
// each song's clock report in the log.
//
// The notes' clock (a double in seconds at 0x82993A58) is advanced once a main-thread frame by sub_823D1490:
// - c = the vertical blank counter (0x8298C1E0, sub_8239FDD0), L = the count at the last update (+16 of the clock);
// - if c = L (no blank since) it counts one in advance: the frame is for the next blank, so the notes move in every
//   frame (r29 = c + 1); else r29 = c. n = r29 - L, and L = r29 after;
// - the clock gets sub_8239FE30(n) seconds times the time scale: n intervals summed from a ring of the last 10 (the
//   vblank callback sub_8239FB20 writes each blank's interval in time base ticks at 0x8298BFA8 + 128 + 8 * index, the
//   blank's own time at +576, then the count at +568).
// What is wrong in that bookkeeping, each measured against the steady clock (local_docs/62 F5, F6; 64):
// - the sum starts at the ring's next slot, the oldest interval, not the newest: an interval of 10 blanks ago stands
//   for the newest of the n. Nothing while the blanks are even (V-Sync: 2 ms in a 191 s song). With V-Sync off the
//   blank is a timer, its intervals uneven and the frames 1 or 2 blanks long, and the sums no longer add up to the
//   time that passed: -110 to +608 ppm from run to run at 480 Hz (up to 36 ms a minute, the notes off the music).
// - n > 10: the ring holds 10 intervals and the sum walks it again, so a frame longer than 10 blanks adds the wrong
//   ones: the clock stepped by 3 to 10 ms either way at each 150 ms hitch of the main thread (24 ms of wander in a
//   song with one every 5 s).
// - c < L: an update can find the count behind the one it stored a frame ago (c + 1, counted in advance) when three run
//   inside one blank (seen after hitches). The game adds nothing (n = -1) but stores c, so the blank it had counted in
//   advance is counted again when it comes: one interval gained.
// (Part of the V-Sync off drift was not the game's: the SDK's guest clock gained on the host's, 200 to 260 ppm, fixed
// in the SDK. The blanks' times here are that clock's.)
//
// With ghwor_song_clock_exact (on) the clock's step is the time between the blanks themselves: a hook in the vblank
// callback keeps each blank's time (the game's own, +576, by its count), and the update adds T(r29) - T(the last
// update's), where T of a blank counted in advance is the newest blank's time plus the last interval (and the true time
// replaces it once the blank has come, so nothing adds up). The count never goes back (c < L: r29 = L, nothing added,
// as the game adds nothing). The frames' logic is the game's: one blank counted in advance when a frame finds none new,
// so the notes move in every frame as before, and with even blanks every step is what the game's sum gives.
// Not changed: the clock counted one blank in advance or not (a 6.25 ms step at 160 Hz when it changes, local_docs/62
// F3): it follows how the frames meet the blanks, the engine's design (always one in advance at 60 Hz).
//
// The report, with the option on or off: at a song's end "[song sync] the song's clock ..." gives the updates that
// counted a blank in advance, found the count behind or had more than 10 blanks at once, and how far the game's own
// sums are from the blanks' times: with the option off that is the clock's error in the song.
// docs/song-clock.md; local_docs/61, 62, 64.

#include "song_clock.h"

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>

REXCVAR_DEFINE_BOOL(ghwor_song_clock_exact, true, "GHWoR",
                    "The song's clock advanced by the time between the vertical blanks themselves: it no longer gains "
                    "on the music with V-Sync off (a blank counted twice when frames run faster than the blanks) nor "
                    "at a hitch longer than 10 blanks. false = the game's own sums of the last blanks' intervals");

namespace {

// The game's timing block: the time base's ticks a second (+32) and the vertical blank count (+568, 0x8298C1E0).
constexpr uint32_t kTiming = 0x8298BFA8;
constexpr uint32_t kTicksPerSecond = kTiming + 32;
constexpr uint32_t kBlankCountOffset = 568;
// The last update's count, +16 of the clock.
constexpr uint32_t kLastCountOffset = 16;

// The blanks' times kept: 16 s at 1000 Hz, 102 s at 160.
constexpr uint64_t kKept = 16384;

struct Blanks {
  std::mutex mutex;
  bool any = false;
  uint64_t newest = 0;
  uint64_t count[kKept] = {};
  int64_t ticks[kKept] = {};
};
Blanks g_blanks;

uint8_t* GuestBase() {
  auto* kernel = rex::system::kernel_state();
  return kernel && kernel->memory() ? kernel->memory()->virtual_membase() : nullptr;
}

uint64_t Load64(const uint8_t* base, uint32_t address) {
  uint64_t value;
  std::memcpy(&value, base + address, 8);
  return __builtin_bswap64(value);
}

// The time of blank `count`, in ticks: its own if it has come, the newest's plus the last interval for the next one.
// With g_blanks.mutex held.
bool BlankTime(uint64_t count, int64_t& ticks) {
  if (!g_blanks.any) {
    return false;
  }
  auto kept = [](uint64_t which, int64_t& out) {
    const uint64_t slot = which % kKept;
    if (g_blanks.count[slot] != which) {
      return false;
    }
    out = g_blanks.ticks[slot];
    return true;
  };
  if (count <= g_blanks.newest) {
    return kept(count, ticks);
  }
  int64_t newest, before;
  if (count != g_blanks.newest + 1 || !kept(g_blanks.newest, newest)) {
    return false;
  }
  ticks = newest + (kept(g_blanks.newest - 1, before) ? newest - before : 0);
  return true;
}

// The main thread's state between the update's two hooks, and from one update to the next.
struct Update {
  bool valid = false;       // `last` and `ticks` are the previous update's
  uint64_t last = 0;        // the count the previous update left
  int64_t ticks = 0;        // and its time
  bool step_valid = false;  // this update's step by the blanks' times
  bool step_used = false;   // and it replaces the game's sum
  double step = 0.0;        // seconds
};
Update g_update;

// One song's figures.
struct Report {
  bool active = false;
  uint64_t updates = 0, ahead = 0, behind = 0, many = 0, unmeasured = 0;
  double game = 0.0, blanks = 0.0;  // seconds: the game's sums, the blanks' times
};
std::mutex g_report_mutex;
Report g_report;

}  // namespace

// [[midasm_hook]] 0x8239FB54 (sub_8239FB20, the vblank callback: "addi r3,r8,1", after the blank's time, r9 ticks since
// the game's start, was stored at +576 of the timing block r11 and before the count at +568 advances): the blank's
// time kept by the count it is about to get, so it is there by the time an update can read that count.
void GHWoRSongClockBlankHook(PPCRegister& r9, PPCRegister& r11) {
  const uint8_t* base = GuestBase();
  if (!base) {
    return;
  }
  const uint64_t count = Load64(base, r11.u32 + kBlankCountOffset) + 1;
  std::lock_guard lock(g_blanks.mutex);
  const uint64_t slot = count % kKept;
  g_blanks.count[slot] = count;
  g_blanks.ticks[slot] = int64_t(r9.u64);
  g_blanks.newest = count;
  g_blanks.any = true;
}

// [[midasm_hook]] 0x823D14B0 (sub_823D1490, the clock's update: "ld r11,16(r31)", r3 = the vertical blank count, r31 =
// the clock; the game then sets r29 = r3, or r3 + 1 when it equals the last update's count). True = r29 is set here and
// the game's choice is skipped (jump to 0x823D14C0).
bool GHWoRSongClockUpdateHook(PPCRegister& r3, PPCRegister& r29, PPCRegister& r31) {
  g_update.step_valid = false;
  g_update.step_used = false;
  const uint8_t* base = GuestBase();
  if (!base) {
    return false;
  }
  const bool exact = REXCVAR_GET(ghwor_song_clock_exact);
  const uint64_t count = r3.u64;
  const uint64_t last = Load64(base, r31.u32 + kLastCountOffset);
  const bool ahead = count == last;
  const bool behind = int64_t(count - last) < 0;
  // The game's choice, and without the count going back.
  const uint64_t game = ahead ? count + 1 : count;
  const uint64_t chosen = exact && behind ? last : game;
  const bool many = int64_t(chosen - last) > 10;

  int64_t now = 0, before = 0;
  bool have_now, have_before;
  {
    std::lock_guard lock(g_blanks.mutex);
    have_now = BlankTime(chosen, now);
    // The previous update's time as it was counted then; after a reset of the clock (it stores the count of that
    // moment) or at the first update, the time of the count the clock holds.
    if (g_update.valid && g_update.last == last) {
      before = g_update.ticks;
      have_before = true;
    } else {
      have_before = BlankTime(last, before);
    }
  }
  uint64_t ticks_per_second = Load64(base, kTicksPerSecond);
  const bool measured = have_now && have_before && ticks_per_second != 0;
  if (measured) {
    g_update.step = double(now - before) / double(ticks_per_second);
    g_update.step_valid = true;
    g_update.step_used = exact;
  }
  // What the next update finds at +16: the count this one leaves.
  const uint64_t leaves = measured && exact ? chosen : game;
  g_update.valid = have_now && leaves == chosen;
  g_update.last = leaves;
  g_update.ticks = now;
  {
    std::lock_guard lock(g_report_mutex);
    if (g_report.active) {
      ++g_report.updates;
      g_report.ahead += ahead;
      g_report.behind += behind;
      g_report.many += many;
      g_report.unmeasured += !measured;
    }
  }
  if (!g_update.step_used) {
    return false;
  }
  r29.u64 = chosen;
  return true;
}

// [[midasm_hook]] 0x823D1524 (sub_823D1490: "frsp f13,f1", f1 = sub_8239FE30's sum of the update's blanks in seconds,
// before the time scale): the step by the blanks' times in its place, and both added up for the song's report.
void GHWoRSongClockStepHook(PPCRegister& f1) {
  if (!g_update.step_valid) {
    return;
  }
  {
    std::lock_guard lock(g_report_mutex);
    if (g_report.active) {
      g_report.game += f1.f64;
      g_report.blanks += g_update.step;
    }
  }
  if (g_update.step_used) {
    f1.f64 = g_update.step;
  }
}

namespace ghwor {

void SongClockSongEnded() {
  Report report;
  {
    std::lock_guard lock(g_report_mutex);
    report = g_report;
    g_report = {};
  }
  if (!report.active || report.updates == 0) {
    return;
  }
  const bool exact = REXCVAR_GET(ghwor_song_clock_exact);
  REXLOG_INFO("[song sync] the song's clock over {:.1f} s of vertical blanks, {} updates: {} counted a blank in "
              "advance, {} found the count behind, {} had more than 10 blanks{}; the game's own sums {:+.2f} ms from "
              "the blanks' times ({})",
              report.blanks, report.updates, report.ahead, report.behind, report.many,
              report.unmeasured ? fmt::format(", {} not measured", report.unmeasured) : std::string(),
              (report.game - report.blanks) * 1000.0,
              exact ? "not used: ghwor_song_clock_exact" : "the clock's: ghwor_song_clock_exact is off");
}

void SongClockSongStarted() {
  SongClockSongEnded();
  std::lock_guard lock(g_report_mutex);
  g_report = {};
  g_report.active = true;
}

}  // namespace ghwor
