// ghwor - the split render: the main view's 3D scene drawn whole in every frame (ghwor_split_render).
//
// The engine draws the main view's scene over two frames, as GH5's (NativeXRenderer's D113, from a capture of GH5).
// The script function SetSplitRenderMode (sub_8250A418; its parameters, each optional: mode 0x6835B854, an integer,
// ratio 0x039F4780, a float, and the split stage 0xBD09544C, an integer) stores them at +1896, +1908 and +1912 of the
// main thread's copy of the render settings (0x829C0870, 9728 bytes). The renderer's init (sub_82585328 ->
// sub_8242CB40 -> sub_8242B988) sets mode 1, ratio 1.0 (0x82000A78) and stage 8 in the render thread's copy
// (0x829BB350, the renderer's block), and sub_8242AF98 copies that to the main thread's: the split is on unless a
// script turns it off. At the start of each frame the main thread reads the mode (sub_8242C450, 0x8242C46C): with a mode other than 0
// and rendering enabled (sub_823D9D48) the counter at +1904 becomes (counter + 1) & 3 and the frame's kind at +1900
// (counter & 1) + 1, so 1, 2, 1, 2; with mode 0 the counter is 7 and the kind 1 (0x8242C4A8). The same function then
// copies the settings to a staging copy (sub_8242AF08 at 0x8242C530, 0x829BE250), which the render thread takes for its
// own (sub_8242B020, from sub_82432110). What reads the mode and the kind:
// - IsPrimaryFrame (the script function sub_82160268, 0x316A336A): mode == 0 || kind == 1.
// - The scene renderer (sub_8255A3C8, the render thread) marks the main view ("mainviewport", 0x1FD1D9C5) as the first
//   half (its byte +168: mode && kind 1, 0x8255A604) or the second (+169: mode && kind 2, 0x8255A634). The view's 19
//   stages (the table at 0x8286B970) run in order: the first half stops at the first stage past the split (0x8255A928)
//   and copies the partial scene (sub_8255AFF8 at 0x8255A990, the render targets' helper, its color resolve returning
//   to 0x8255AD1C, called from 0x8255A994); the second half skips the stages up to the split (0x8255A93C), after the
//   view's setup without its clear (sub_8264B568, clear = !+169) and sub_82554780 restoring the first half's color and
//   depth with a quad (0x8255A898; D113's depth through a k_8_8_8_8 alias). With ratio 1.0 and stage 8 (the bounds at
//   0x8255A8D8-0x8255A900): stages 1-8 in kind 1 (the stage and the crowd, the depth copy, the ambient occlusion),
//   9-19 in kind 2 (the light volume, the band's passes, the copy of the whole scene at 0x8264C1B8 and the
//   post-processing).
// - The "ui" viewport (0xD800B94F) redraws in kind 1 the previous kind 2's composite (sub_82554880 at 0x8255A72C: the
//   texture [0x829BE118], the renderer's + 11720, "FrameBuffer_SplitRender") and captures it in kind 2 (sub_82554810
//   at 0x8255A744, a Resolve returning to 0x82554870). The viewports flagged 0x340 are drawn in kind 2 only, from the
//   previous kind 1's list (0x8255A6D0, 0x8255A748, 0x8255A78C, sub_82554AB0). Stage 4's objects can be divided between
//   the kinds by the ratio (sub_82557E88's flags 0x10000000 and 0x20000000, sub_825568A0). The main thread skips some
//   updates in kind 2 (sub_825559E0, sub_82558BC0, sub_82558E50).
// So the 3D scene changes 30 times a second, and kind 1 shows the previous kind 2's image under a new HUD. The F9 trace
// of a song (dlss_quality, 36 frames): the two kinds strictly alternate, the stage's frame copying its scene at
// 0x8255A994 and redrawing 0x829BE118, the band's restoring it, copying at 0x8264C1B8 and capturing.
//
// Each of those tests reads the mode first, and with mode 0 each takes the one-frame path: every frame kind 1,
// IsPrimaryFrame always true; +168 and +169 0, so the main view runs its 19 stages in one frame with no cut and no copy
// at 0x8255A994, its setup clears and nothing is restored (sub_82554780 also tests mode && kind 2 itself); no redraw or
// capture of [0x829BE118] (0x8255A6A0 skips both); the viewports flagged 0x340 drawn every frame, as themselves
// (0x8255A78C; sub_82554AB0 clears both lists); all of stage 4's objects (neither flag, 0x825580BC); no update skipped
// on the main thread. The one other reader, sub_82554760 (mode ? kind : 0, called by the light volume's sub_82648B08),
// has its result dropped (r3 overwritten at 0x82648B48). Nothing else reads +1896 or +1900 of the three copies.
//
// With ghwor_split_render false the mode is held at 0 in the main thread's copy: at the start of each frame, before
// the main thread reads it (0x8242C46C: the renderer's 1, or a script's), and right after SetSplitRenderMode stores one
// (0x8250A4C4, its exit), so IsPrimaryFrame answers for the frame at once. SetSplitRenderMode stores the mode only when
// the call gives it (sub_823960A8 finding 0x6835B854: "lwz r11,80(r1); stw r11,1896(r30)" at 0x8250A460-0x8250A464;
// without it the code goes to 0x8250A468 and +1896 keeps the value the hooks held), and the ratio and the split stage
// each on their own: so the mode a script asks for is taken at that store (0x8250A464, r11), and a call without one
// leaves the game's mode as it was. The staging and the render thread's copies,
// made from it each frame after the hook, carry 0 too. The game's own mode is kept and put back if the cvar is turned
// on again (the SDK's settings). NativeXRenderer's DLSS and FSR 3.1 (auto views, D119: a view from the first
// depth-writing perspective draw of the output's size to the copy of the whole scene before the post-processing,
// 0x8264C1B8, the profile's outer caller) then see the whole scene in one view each frame; the ambient occlusion's
// copies through the same helper before the band (sub_8263FEE8, 640x360) are not of the output's size, and stage 12's
// (0x8264A54C, when objects of pass 18 exist) and a material's "framebuffer" parameter's (0x82620798, 0x826207D8)
// are not from 0x8264C1B8. With the split on (the default, with those modes too since NativeXRenderer's D146) their
// view spans the pair: it pauses at the first half's copy (0x8255A994) and the upscaler runs once, at 0x8264C1B8 in
// the second half (src/native_x_renderer_ghwor.cpp).
// The cost is CPU: the main view's 19 stages and the main thread's updates every frame, where the console split them
// over two.

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <atomic>
#include <bit>
#include <cstdint>
#include <cstring>
#include <mutex>

REXCVAR_DEFINE_BOOL(ghwor_split_render, true, "GHWoR",
                    "The main view's 3D scene: true = drawn over two frames, as on the console (the stage and the "
                    "crowd in one, the band and the post-processing in the next: the scene changes 30 times a "
                    "second; NativeXRenderer's DLAA, DLSS and FSR 3.1 run once a pair, D146); false = drawn whole in "
                    "every frame (more CPU; those then on each frame). With NativeXRenderer's frame generation it is "
                    "set to false at startup unless given");

namespace {

// The main thread's copy of the render settings (sub_8242C450's r31), and SetSplitRenderMode's fields in it.
constexpr uint32_t kSettings = 0x829C0870;
constexpr uint32_t kMode = kSettings + 1896;        // 0x829C0FD8: 0 = one frame
constexpr uint32_t kRatio = kSettings + 1908;       // a float: 1.0, 0.0, or stage 4's objects divided
constexpr uint32_t kSplitStage = kSettings + 1912;  // the last stage of the first half (with ratio 1.0)

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

// The mode the game asked for (the renderer's init sets 1; then each SetSplitRenderMode that gives one), and whether
// the hooks have replaced it by 0. The hooks run on the main thread, which runs the scripts.
std::atomic<uint32_t> g_game_mode{1};
std::atomic<bool> g_forced{false};
// Set by the hook on SetSplitRenderMode's store of the mode, taken by the hook at its exit: whether this call gave one.
std::atomic<bool> g_mode_given{false};

// Holds the mode at 0 while the split render is off; returns whether it replaced one.
bool HoldUnsplit(uint8_t* base, uint32_t mode) {
  if (REXCVAR_GET(ghwor_split_render) || mode == 0) {
    return false;
  }
  StoreWord(base, kMode, 0);
  g_forced.store(true, std::memory_order_relaxed);
  return true;
}

}  // namespace

// [[midasm_hook]] 0x8242C46C (sub_8242C450, the main thread's frame, before "lwz r11,1896(r31)": the mode it chooses
// the frame's kind by, r31 = 0x829C0870). Once a frame, before the settings are copied for the render thread
// (0x8242C530).
void GHWoRSplitRenderFrameHook() {
  uint8_t* base = GuestBase();
  if (!base) {
    return;
  }
  const bool split = REXCVAR_GET(ghwor_split_render);
  const uint32_t mode = LoadWord(base, kMode);
  static int logged = -1;  // only the main thread
  if (logged != int(split)) {
    logged = int(split);
    if (split) {
      // The game's mode: the one the hooks replaced, if they did.
      const bool held = g_forced.load(std::memory_order_relaxed) && mode == 0;
      REXLOG_INFO("[split render] on, as on the console: the main view's 3D scene over two frames while the game's "
                  "mode is not 0 (mode {})",
                  held ? g_game_mode.load(std::memory_order_relaxed) : mode);
    } else {
      REXLOG_INFO("[split render] off (ghwor_split_render = false): SetSplitRenderMode's mode held at 0 (0x{:08X}, the "
                  "game's {}): every frame a primary one, the main view's 19 stages in it and the whole scene copied "
                  "once before the post-processing",
                  kMode, mode);
    }
  }
  if (split) {
    // Turned on again: the game's mode back, unless a script has set one since.
    if (g_forced.exchange(false, std::memory_order_relaxed) && mode == 0) {
      StoreWord(base, kMode, g_game_mode.load(std::memory_order_relaxed));
    }
    return;
  }
  if (mode != 0) {
    g_game_mode.store(mode, std::memory_order_relaxed);  // the renderer's init's 1 at the first frame
  }
  HoldUnsplit(base, mode);
}

// [[midasm_hook]] 0x8250A464 (sub_8250A418, SetSplitRenderMode, "stw r11,1896(r30)": reached only when the call gives
// the mode 0x6835B854, r11 = that mode). The exit hook below holds it at 0 after this store if the cvar asks for it.
void GHWoRSplitRenderModeGivenHook(PPCRegister& r11) {
  g_game_mode.store(r11.u32, std::memory_order_relaxed);
  g_mode_given.store(true, std::memory_order_relaxed);
}

// [[midasm_hook]] 0x8250A4C4 (sub_8250A418, SetSplitRenderMode, at its exit "addi r1,r1,128", every path): after the
// stores of the parameters given. A call without the mode leaves +1896 as it was, which can be the 0 the hooks hold,
// not the game's: then nothing is held and the log says the mode was not given. Logs the first call and each change of
// the values (up to 32 lines: the values the scripts use in the menus and in a song are not known).
void GHWoRSplitRenderModeHook() {
  uint8_t* base = GuestBase();
  if (!base) {
    return;
  }
  const bool given = g_mode_given.exchange(false, std::memory_order_relaxed);
  const uint32_t mode = given ? g_game_mode.load(std::memory_order_relaxed) : LoadWord(base, kMode);
  const uint32_t ratio_bits = LoadWord(base, kRatio);
  const int32_t stage = int32_t(LoadWord(base, kSplitStage));
  const bool forced = given && HoldUnsplit(base, mode);
  static std::mutex mutex;
  static int lines = 0;
  static uint32_t last[4] = {~0u, ~0u, ~0u, ~0u};
  std::lock_guard lock(mutex);
  if (lines < 32 &&
      (last[0] != uint32_t(given) || last[1] != mode || last[2] != ratio_bits || last[3] != uint32_t(stage))) {
    ++lines;
    last[0] = uint32_t(given);
    last[1] = mode;
    last[2] = ratio_bits;
    last[3] = uint32_t(stage);
    if (given) {
      const char* outcome = forced ? " -> mode 0 (ghwor_split_render = false)" : mode == 0 ? " (no split)" : "";
      REXLOG_INFO("[split render] SetSplitRenderMode: mode {} ratio {:.3f} split stage {}{}", mode,
                  std::bit_cast<float>(ratio_bits), stage, outcome);
    } else {
      REXLOG_INFO("[split render] SetSplitRenderMode: mode not given (the game's stays {}) ratio {:.3f} split stage {}",
                  g_game_mode.load(std::memory_order_relaxed), std::bit_cast<float>(ratio_bits), stage);
    }
  }
}
