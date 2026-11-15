// ghwor - the 3D scene drawn every frame (ghwor_scene_every_frame; off: the console's way, the scene over two frames).
//
// The engine splits its 3D scene over two frames ("split render mode", GH5's too; not in GH Metallica, GH Van Halen or
// GH Aerosmith): the render state's mode (+1896) is 1 from its init (sub_8242B988 stores it at 0x829BBAB8, the render
// thread's copy of the state, 0x829BB350; the main thread's copy 0x829C0870 carries it too: both read 1 in a running
// game). The script function SetSplitRenderMode (sub_8250A418: "mode" -> +1896, 0x039F4780 -> +1908, 0xBD09544C ->
// +1912) is called once, by the boot's script 0xFE5B0111, with 0xBD09544C = 7 only: the split's render stage.
// - The main thread's frame (sub_8242C450) counts the frames while the mode is on: counter +1904 = (counter + 1) & 3,
//   the frame's kind +1900 = (counter & 1) + 1; with it off 7 and 1.
// - The scene renderer (sub_8255A3C8): in a kind 1 frame the main scene runs the render stages (the table 0x8286B968)
//   up to the split's (+1912, 7) and resolves the scene (sub_8255AFF8 at 0x8255A990); in a kind 2 frame it runs them
//   from that stage on, the image restored; each scene's mask (+160) names the kinds of frame it is drawn in. The main
//   thread's collection of the objects to draw (sub_82558BC0, sub_82558E50, sub_825559E0) skips kind 2 frames, and
//   IsPrimaryFrame (sub_82160268) is true with the mode off or in a kind 1 frame.
// So each half of the 3D image is drawn every other frame: 30 times a second on the console at 60 FPS, half the chosen
// frame rate here; the note highway and the HUD every frame. With the mode off the renderer runs every stage of every
// scene in each frame (the engine's own path), and IsPrimaryFrame is always true (the scripts that align a camera cut
// or a musician's change to a kind of frame, src/framerate.cpp's kFrameExactScripts, then wait no frame or always one).
//
// Measured (2026-11-15, Party Play, the mode written to 0 in a running game): every frame about 1060-1090 indexed draws
// (468 and 416 in turn before), 0 failed draws, SMAA on both of each frame's scene copies (1200 in 600 frames, was
// 900); at 160 FPS with a 3440x1440 internal resolution 155-159 FPS, the process 2.60 cores (was 1.74), NativeXRenderer's
// thread 80 % of a core (was 44 %), the GPU about 65 % (was about 42 %).

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <cstdint>
#include <cstring>

REXCVAR_DEFINE_BOOL(ghwor_scene_every_frame, false, "GHWoR",
                    "The 3D scene (the venue, the crowd and the band) drawn in every frame: the engine's split render "
                    "mode off. false = as on the console: half of the scene in one frame, the other half in the next, "
                    "so the scene is drawn at half the frame rate (the note highway and the HUD at the full rate)");

namespace {

// The split render mode (+1896) in the render thread's copy of the render state and in the main thread's.
constexpr uint32_t kRenderModeRender = 0x829BBAB8;
constexpr uint32_t kRenderModeMain = 0x829C0FD8;

uint8_t* GuestBase() {
  auto* kernel = rex::system::kernel_state();
  return kernel && kernel->memory() ? kernel->memory()->virtual_membase() : nullptr;
}

}  // namespace

// [[midasm_hook]] 0x8242C46C (sub_8242C450, the main thread's frame: "lwz r11,1896(r31)", the split render mode read
// before the frame's kind is counted). Written every frame, so a later SetSplitRenderMode cannot turn it back on.
void GHWoRSplitRenderHook() {
  if (!REXCVAR_GET(ghwor_scene_every_frame)) {
    return;
  }
  uint8_t* base = GuestBase();
  if (!base) {
    return;
  }
  uint32_t was;
  std::memcpy(&was, base + kRenderModeMain, 4);
  static bool logged = false;
  if (!logged) {
    logged = true;
    REXLOG_INFO("[split render] off: the 3D scene is drawn in every frame (ghwor_scene_every_frame; the game's mode was "
                "{})",
                __builtin_bswap32(was));
  }
  const uint32_t off = 0;
  std::memcpy(base + kRenderModeMain, &off, 4);
  std::memcpy(base + kRenderModeRender, &off, 4);
}
