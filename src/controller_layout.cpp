// ghwor - a controller plays a song as in Guitar Hero III, on the game's own controller code.
//
// Guitar Hero III played a plain controller on a layout of its own (read in Guitar Hero: Aerosmith's code), and Guitar
// Hero: World Tour's engine kept that code: Warriors of Rock still has it, but gives a plain controller the vocals, so
// the SDK tells the game a controller is a guitar (src/startup.cpp's gamepad_subtype) and the game played it as an Xbox
// 360 guitar (frets A, B, Y, X and LB, strumming on the D-pad). With gamepad_guitar_layout = "gh3" these hooks make
// the gameplay take such a controller for what it is, so the game's controller code plays it:
// - sub_82185C60 (GH III's sub_82108000; only from the input logic): the frets pressed this frame from LT (green), LB
//   (red), RB (yellow), RT (blue), A (orange) and B (open, a bass's), the ones held from the same buttons; a strum
//   whenever a fret is pressed (none on a release); nothing held marks the player released.
// - sub_82180BA0 (GuitarInputLogicPerFrame's per-player logic): a controller's rules, as GH III's: no tolerance strums
//   (disallow_tolerance_strums is a guitar's), one gem of the window for the anchoring, a fret-only hit after the
//   frets were let go.
// - check_for_star_power_CFunc (sub_821E77B0): Star Power on the D-pad.
// - ControlWhammyPitchShift (sub_8218C8A8): the whammy bar on the stick pushed furthest; and the whammy's movement on a
//   sustain (sub_82186FF8: star power, the sustain's look), either stick moved.
// - ButtonCheckerPerFrame (sub_8216D480, sub_82188020) and GetHeldPattern (sub_8217FA40): the frets held, the ones
//   lit on the highway.
// Each of them asks sub_8217F110(controller, port), true for a port of 4 or more or a controller of the game's device
// type 8 or 9 (guitar, bass); sub_8217FA40 asks inline. The menus and the lobby's scripts (IsGuitarController,
// GetControllerType, MonitorControllerStates) still see a guitar, so the controller can choose one.
//
// Extended sustains (Warriors of Rock's; Guitar Hero III had none): a chord played while an earlier note's sustain is
// still held. The controller code's chord rule (GH III's own, sub_821235E0's) counts a chord only after every fret was
// let go since the last hit (the player's flag +0x180E0, set by sub_82185C60 when nothing is held, cleared at each
// hit), so with the sustained fret held the chord never counted. A guitar's chord has no such rule and leaves the
// sustained frets out through the gem's mask (sub_821857B8), which a controller's chord then goes through too. Three
// more hooks ask instead that the chord's own frets were each let go since the last hit: a chord held on into the next
// still does not count, and a sustained fret outside the chord no longer stops it.
//
// And the sustain itself (sub_8218ED00, each frame): every sustained fret ends on its own when let go, and the whole
// sustain ends when the frets held among the current note's (its mask) are some of its frets but not all of them, the
// sustained fret of an earlier note with it. On a guitar the chord stays held and is strummed again; on the controller
// it is let go and pressed again, and a trigger and a button are never let go in the same frame (8 ms at 120 FPS), so
// the frame between ended the earlier note's sustain too. For the controller code, while an earlier note's fret still
// sustains, some of the current note's frets held count as all of them: the ones let go end on their own, the earlier
// one goes on. A fret pressed outside the note still ends it all, and a sustain without an earlier note's is GH III's.
// docs/controller-gh3-layout.md, local_docs/21-gh3-controller.md.

#include <rex/input/input_system.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>

namespace {

// The game's controller ports: XInput's users.
constexpr uint32_t kPorts = 4;

// Each port's last answer, for the log: -1 not asked yet.
std::array<std::atomic<int>, kPorts> g_logged = {-1, -1, -1, -1};

bool PlaysGh3Layout(uint32_t port) {
  if (port >= kPorts) {
    return false;  // no controller of this PC (a remote or computer player)
  }
  const bool gh3 = rex::input::PlaysGh3Layout(port);
  if (g_logged[port].exchange(gh3 ? 1 : 0) != (gh3 ? 1 : 0)) {
    REXLOG_INFO("[controller layout] port {}: {}", port,
                gh3 ? "a controller (or the keyboard) on Guitar Hero III's layout, played by the game's controller code"
                    : "a guitar (or a controller or the keyboard as one), played as one");
  }
  return gh3;
}

// A player's frets: whether the controller code reads them, the ones held at its last read, and the ones held at every
// read since the last hit (the game's patterns: green 0x10000, red 0x1000, yellow 0x100, blue 0x10, orange 0x1, open
// 0x100000).
struct Frets {
  uint32_t player = 0;  // the player object's guest address, 0: a free slot
  bool controller_code = false;
  uint32_t held = 0;
  uint32_t kept = 0;
};

// Every player whose frets the game reads (sub_82185C60); the oldest slot taken over if they ever run out.
constexpr size_t kSlots = 16;
std::mutex g_frets_mutex;
std::array<Frets, kSlots> g_frets;
size_t g_next_slot = 0;

Frets* FindFrets(uint32_t player) {
  for (Frets& f : g_frets) {
    if (f.player == player) {
      return &f;
    }
  }
  return nullptr;
}

Frets& SlotFor(uint32_t player) {
  if (Frets* f = FindFrets(player)) {
    return *f;
  }
  Frets& f = g_frets[g_next_slot];
  g_next_slot = (g_next_slot + 1) % kSlots;
  f = Frets{player};
  return f;
}

// The player's frets whose sustain still runs (sub_8218ED00's +0x181E0).
constexpr uint32_t kSustainingFrets = 0x181E0;

uint8_t* GuestBase() {
  auto* kernel = rex::system::kernel_state();
  return kernel && kernel->memory() ? kernel->memory()->virtual_membase() : nullptr;
}

uint32_t LoadWord(const uint8_t* base, uint32_t address) {
  uint32_t value;
  std::memcpy(&value, base + address, 4);
  return __builtin_bswap32(value);
}

}  // namespace

// [[midasm_hook]]s right after `bl sub_8217F110` (it leaves r4, the port, as it was) in the gameplay's functions: a
// controller on Guitar Hero III's layout is not a guitar there.
void GHWoRControllerLayoutHook(PPCRegister& r3, PPCRegister& r4) {
  if ((r3.u32 & 0xFF) != 0 && PlaysGh3Layout(r4.u32)) {
    r3.u64 = 0;
  }
}

// [[midasm_hook]] 0x8217FBE4 (sub_8217FA40, GetHeldPattern's: the inline check's result in r11, the port in r31).
void GHWoRControllerLayoutHeldHook(PPCRegister& r11, PPCRegister& r31) {
  if ((r11.u32 & 0xFF) != 0 && PlaysGh3Layout(r31.u32)) {
    r11.u64 = 0;
  }
}

// [[midasm_hook]] 0x82185CD8 (sub_82185C60, the frets' read, right after its `bl sub_8217F110`; the player in r28): as
// GHWoRControllerLayoutHook, and the player noted as read by the controller code or not.
void GHWoRControllerReadHook(PPCRegister& r3, PPCRegister& r4, PPCRegister& r28) {
  GHWoRControllerLayoutHook(r3, r4);
  std::lock_guard lock(g_frets_mutex);
  Frets& f = SlotFor(r28.u32);
  const bool controller_code = (r3.u32 & 0xFF) == 0;
  if (f.controller_code != controller_code) {
    f.controller_code = controller_code;
    f.held = f.kept = 0;
  }
}

// [[midasm_hook]] 0x82185F64 (sub_82185C60's controller path, once the frets are read: the player in r28, the frets
// held in r29, 0 for none).
void GHWoRControllerFretsHook(PPCRegister& r28, PPCRegister& r29) {
  std::lock_guard lock(g_frets_mutex);
  Frets& f = SlotFor(r28.u32);
  f.held = r29.u32;
  f.kept &= r29.u32;
}

// [[midasm_hook]] 0x821870B4 (sub_82186FF8, the whammy's movement on a sustain, right after its `bl sub_8217F168`,
// IsGuitarController, left a guitar for the menus; the player in r31): a player on the controller code is not a guitar
// there either. A guitar's bar is the right stick's X from the bar's rest (+212 of the player info), so a stick moved
// over most of its travel barely counted; a controller's is either stick moved, its X or its Y (the one off the centre,
// the left first), as Guitar Hero III's.
void GHWoRControllerWhammyHook(PPCRegister& r3, PPCRegister& r31) {
  if ((r3.u32 & 0xFF) == 0) {
    return;
  }
  std::lock_guard lock(g_frets_mutex);
  if (const Frets* f = FindFrets(r31.u32); f && f->controller_code) {
    r3.u64 = 0;
  }
}

// [[midasm_hook]] 0x82181AB4 (sub_82180BA0: a gem hit, where the game clears the player's flag; the player in r27).
void GHWoRControllerHitHook(PPCRegister& r27) {
  std::lock_guard lock(g_frets_mutex);
  if (Frets* f = FindFrets(r27.u32)) {
    f->kept = f->held;
  }
}

// [[midasm_hook]] 0x82181BD8 (sub_82180BA0: a controller's chord, the player's flag just read into r11; the gem's
// frets in r26, the player in r27): the flag counts as set when none of the chord's frets was held since the last hit.
void GHWoRControllerChordHook(PPCRegister& r11, PPCRegister& r26, PPCRegister& r27) {
  if (r11.u32 == 1) {
    return;
  }
  std::lock_guard lock(g_frets_mutex);
  const Frets* f = FindFrets(r27.u32);
  if (f && f->controller_code && (f->kept & r26.u32) == 0) {
    r11.u64 = 1;
  }
}

// [[midasm_hook]] 0x8218F1E4 (sub_8218ED00, each frame of a sustain, once each sustained fret let go has ended: r25 the
// frets held among the note's mask (r21), r26 the note's frets, r24 nonzero when the sustain is over, the player in
// r31).
void GHWoRControllerSustainHook(PPCRegister& r24, PPCRegister& r25, PPCRegister& r26, PPCRegister& r31) {
  const uint32_t held = r25.u32;
  const uint32_t note = r26.u32;
  if (r24.u32 != 0 || held == note || held == 0 || (held & ~note) != 0) {
    return;  // over, as the note, nothing of it, or a fret outside it: as the game decides
  }
  {
    std::lock_guard lock(g_frets_mutex);
    const Frets* f = FindFrets(r31.u32);
    if (!f || !f->controller_code) {
      return;
    }
  }
  const uint8_t* base = GuestBase();
  if (!base) {
    return;
  }
  const uint32_t sustaining = LoadWord(base, r31.u32 + kSustainingFrets);
  if ((sustaining & ~note) != 0) {
    r25.u64 = note;  // an earlier note's fret still sustains: some of this note's frets held count as all
  }
}
