// ghwor - the game's post-processing effects, each of which can be turned off.
//
// The engine's screen effects (the scripts' ScreenFX_AddFXInstance and ScreenFX_UpdateFXInstanceParams; the
// instances are made by sub_82530A08) are created once per scene and then given new parameters by the scripts. All of
// them work on the 3D scene: the note highway and the HUD are drawn after them.
//
// The composite, sub_82641110, has GH5's two passes of the shader _Uber_ScreenFX (Pass0: _motionblur1, _bloom1, _dof1,
// _tonemap1, _color1; Pass1: _motionblur1, _noise1, _vignette1 and _inkblot1, where GH5 had _visualizer1), chosen from
// a word of flags with one bit per effect type that is on (+4 of the effects manager, rebuilt from the active
// instances by sub_8252FBE0: 0x1 MotionBlur, 0x2 Bloom2, 0x4 Godray (the bloom's variant too), 0x8 DOF3, 0x10 Tonemap,
// 0x20 Bright_Sat (the color), 0x40 Noise2 (the grain), 0x80 Vignette, 0x100 the type of GH5's visualizer (no variant
// here), 0x400 InkBlotch, 0x800 AmbientOcclusion, 0x200 and 0x1000 unnamed, 0x10000 the composite's gate). GH5's
// composite read that word itself, three times; this game's tests it through sub_8252F3E8 ((flags & a global mask the
// scripts set & the bits asked) != 0) for both passes' variants and for each pass's way of drawing (taking the scene
// and replacing it, or blending over it), and the effects' own passes test it the same way (the depth of field's and
// the bloom's among them). The hook takes the bits of the effects turned off out of the word in every test: the passes
// are then the variants without them, drawn the way the remaining effects need, and an effect's own passes run as when
// none of its instances is on (its constants are still set). The word itself and the global mask are left alone, so an
// effect turned back on is applied from the next frame.
//
// GH Van Halen's ink blotches are part of this game's composite again (InkBlotch, 0x400, Pass1's _inkblot1), which GH5
// did not have; no switch for them yet (GH Van Halen's ghvanhalen_fx_ink would be kInk = 0x400 here).

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>

#include <atomic>
#include <cstdint>

REXCVAR_DEFINE_BOOL(ghwor_fx_bloom, true, "GHWoR",
                    "Screen effects: the bloom, the glow around lights and bright surfaces; false = off");
REXCVAR_DEFINE_BOOL(ghwor_fx_depth_of_field, true, "GHWoR",
                    "Screen effects: the depth of field, the background blurred in some camera shots and behind the "
                    "menus and the pause menu; false = off");
REXCVAR_DEFINE_BOOL(ghwor_fx_grain, true, "GHWoR",
                    "Screen effects: the film grain over the image; false = off (always off while the color "
                    "correction is off)");
REXCVAR_DEFINE_BOOL(ghwor_fx_color, true, "GHWoR",
                    "Screen effects: the color correction (brightness, contrast, saturation): each venue's look, fades "
                    "to black, the image greyed while the rock meter is red; false = off, and the grain with it");
REXCVAR_DEFINE_BOOL(ghwor_fx_vignette, true, "GHWoR",
                    "Screen effects: the vignette, the corners darkened where a venue or a moment turns it on; "
                    "false = off");

namespace {

constexpr uint32_t kBloom = 0x2;
constexpr uint32_t kDepthOfField = 0x8;
constexpr uint32_t kColor = 0x20;
constexpr uint32_t kGrain = 0x40;
constexpr uint32_t kVignette = 0x80;

uint32_t EffectsOff() {
  uint32_t off = 0;
  off |= REXCVAR_GET(ghwor_fx_bloom) ? 0 : kBloom;
  off |= REXCVAR_GET(ghwor_fx_depth_of_field) ? 0 : kDepthOfField;
  off |= REXCVAR_GET(ghwor_fx_grain) ? 0 : kGrain;
  off |= REXCVAR_GET(ghwor_fx_color) ? 0 : kColor;
  off |= REXCVAR_GET(ghwor_fx_vignette) ? 0 : kVignette;
  // In GH Van Halen the color pass also toned the grain down, and without it the grain was far stronger, so the grain
  // goes with the color (the launcher's rule). Here, as in GH5, the grain is in the other pass: kept the same until it
  // is seen.
  if (off & kColor) {
    off |= kGrain;
  }
  return off;
}

}  // namespace

// [[midasm_hook]] 0x8252F428 (sub_8252F3E8, the effects' test), right after "lwz r10,4(r31)" and the global mask's
// "lwz r11,-28596(r11)" ([0x8285904C]): r10 = the effects' flags, before "and r9,r10,r11". Every test of the flags
// comes here: the composite's 14 (its variants and its ways of drawing) and 18 in 15 other render functions (the
// effects' own passes among them).
void GHWoRScreenFxFlagsHook(PPCRegister& flags) {
  const uint32_t off = EffectsOff();
  if (!off) {
    return;
  }
  static std::atomic<uint32_t> logged{0};
  if (logged.exchange(off) != off) {
    REXLOG_INFO("[screen fx] effects turned off: flags 0x{:03X} (ghwor_fx_*)", off);
  }
  flags.u32 &= ~off;
}
