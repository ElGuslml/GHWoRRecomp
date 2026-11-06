// ghwor - the profile follows the controller that starts the game, and each slot's kind of controller follows
// the one played.
//
// On the console the controller that presses a button at the start of the game plays with its own profile. Here there
// is one profile, the first controller's (the order Windows connected them in), and a guitar that started the game with
// a controller connected before it was the profile's guest. This game has no title screen: the intro video, the autosave
// notice, then the main menu. So the SDK's primary press window opens with the game (src/quit_prompt.cpp, each frame) and
// the controller that presses first, wherever that is, becomes user 0, the profile's; the others are its guests ("Name
// (1)", the SDK's signin_guests, set in src/startup.cpp), as when guests sign in on the console. While the window is open
// every new press on another controller takes user 0, so the first press from anyone closes it for good, as GH5's. A
// press on user 0's own controller is seen here; on another one, the SDK's read makes it user 0's, and its next read shows
// it here.
//
// The game asks a controller port for its kind (sub_823CF718: a guitar, the drums, or the singer for a plain gamepad)
// only when the port's controller connects. The keyboard's pad shares slot 0 with the first controller, and the SDK
// answers for the one in the player's hands, which at the start is the controller: with a controller set to sing in
// the launcher (or a kind the game gives the vocals) and the keyboard playing guitar, the keyboard's player was the
// singer (a player's report in GH Metallica, 2026-11-02). When the SDK finds the slot played on another kind of
// controller, the port's update asks again, outside a song.

#include "players.h"

#include <rex/cvar.h>
#include <rex/input/input_system.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/runtime.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <cstdint>
#include <cstring>

#include "widescreen.h"

REXCVAR_DEFINE_BOOL(ghwor_profile_follows_press, true, "GHWoR",
                    "The first controller to press a button when the game starts plays with the profile; off: the "
                    "first controller connected does, as before");
REXCVAR_DEFINE_BOOL(ghwor_kind_follows_controller, true, "GHWoR",
                    "A slot played on another kind of controller than the game was told of (the keyboard playing "
                    "guitar where the slot's controller sings) is asked for its kind again, outside a song; off: "
                    "the kind of the controller the slot had when it connected, as before");

namespace {

// sub_823D02A0's port: the guest user it reads, and its state (1: waiting for its controller, which the game then
// asks for its kind; 5: one more read; 2: reading it each frame).
constexpr uint32_t kPortUser = 12;
constexpr uint32_t kPortState = 228;
constexpr uint32_t kStateWaiting = 1;
constexpr uint32_t kStateReading = 2;

// The buttons user 0 holds, with the keyboard's: its controller after a swap too.
uint16_t UserZeroButtons() {
  auto* kernel = rex::system::kernel_state();
  if (!kernel || !kernel->emulator()) {
    return 0;
  }
  auto* input = static_cast<rex::input::InputSystem*>(kernel->emulator()->input_system());
  rex::input::X_INPUT_STATE state{};
  using rex::X_RESULT;
  if (!input || input->GetStateForUI(0, &state) != X_ERROR_SUCCESS) {
    return 0;
  }
  return static_cast<uint16_t>(state.gamepad.buttons);
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

void Store32(uint8_t* base, uint32_t address, uint32_t value) {
  value = __builtin_bswap32(value);
  std::memcpy(base + address, &value, 4);
}

}  // namespace

// [[midasm_hook]] 0x823D02A0: the entry of sub_823D02A0, a controller port's update (r3 = the port). A port reading its
// controller goes back to waiting, so that this update reads it again and asks for its kind; its input is not read for
// two frames.
void GHWoRControllerPortHook(PPCRegister& r3) {
  uint8_t* base = GuestBase();
  if (!base || !REXCVAR_GET(ghwor_kind_follows_controller) || ghwor::SongShown()) {
    return;
  }
  if (Load32(base, r3.u32 + kPortState) != kStateReading) {
    return;
  }
  const uint32_t user = Load32(base, r3.u32 + kPortUser);
  if (!rex::input::TakeDeviceKindChange(user)) {
    return;
  }
  Store32(base, r3.u32 + kPortState, kStateWaiting);
  REXLOG_INFO("[players] slot {}: the game asks for its kind of controller again", user);
}

namespace ghwor {

void UpdatePressWindow(bool dialog) {
  // kPaused: a dialog box takes the presses, and the window comes back after it.
  enum class Window { kWaiting, kOpen, kPaused, kDone };
  static Window window = Window::kWaiting;
  static uint16_t held = 0xFFFF;

  if (window == Window::kDone) {
    return;
  }
  if (!REXCVAR_GET(ghwor_profile_follows_press)) {
    window = Window::kDone;
    return;
  }
  const bool open = window == Window::kOpen;
  if (open) {
    // A button held when the window opened is no press until it is let go of, as in the SDK.
    const uint16_t buttons = UserZeroButtons();
    const uint16_t pressed = static_cast<uint16_t>(buttons & ~held);
    held = buttons;
    if (pressed != 0) {
      rex::input::SetPrimaryPressWindow(false);
      window = Window::kDone;
      REXLOG_INFO("[players] the first press is user 0's, the profile's (buttons {:04X})", pressed);
      return;
    }
  }
  if (dialog) {
    if (open) {
      rex::input::SetPrimaryPressWindow(false);
      window = Window::kPaused;
    }
    return;
  }
  if (!open) {
    held = 0xFFFF;
    rex::input::SetPrimaryPressWindow(true);
    window = Window::kOpen;
    REXLOG_INFO("[players] the first controller to press takes the profile");
  }
}

}  // namespace ghwor
