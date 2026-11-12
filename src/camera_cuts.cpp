// ghwor - the camera's cuts: DLSS's and FSR 3.1's history starts again at each new shot (NativeXRenderer's D145).
//
// DLSS and FSR 3.1 keep the last frames' image and fetch it where the motion vectors say each pixel came from. Across a
// cut the camera's reprojection (D118's vote here: the game marks no view) joins two unrelated shots, and the last shot
// shows through the new one for a few frames: GH Metallica hid DLSS in its 0.3.0 for that ("the stage ghosts at the
// camera's cuts (no reset there yet)"). DLSS runs through a song with no frame missing its view (on every frame with
// the split render off, src/split_render.cpp; once a pair with it on, NativeXRenderer's D146), so nothing else resets
// its history there: NGX reported NeedHistoryReset 4 times in ghwor_012 (the split off), all while the view started,
// none in the rest of the song, through which the camera kept cutting.
//
// Where the game cuts. The songs' shots come from the camera cuts' scripts (in the game's qb.pab.xen, one script file,
// key 0x01CA86CD; a script's name is its QB key, the CRC-32 of the name in lower case without the final inversion, as
// IsPrimaryFrame = 0x316A336A and Band_PlayClip = 0x01DE43A7 in src/framerate.cpp; the C functions keep their names in
// the executable). The cut script 0xA5BB57BB, CameraCuts_StartCallback, starts each shot: it returns at once while the
// cuts are off (the global cameracuts_enabled, 0x5F9AE663, false), else picks the shot's camera from a camera array
// (CameraCut_GetArraySize, GetRandomValue, CameraCut_GetCameraIndex, CameraCut_SaveSelectedParams), spawns the shot's
// screen effects (0xF93E0168, after 0xCAD261DF and a game frame), calls the C function
// BandManager_NotifyAllOfCameraCut, and ends with PlayIGCCam { time = the shot's length, viewport = bg_viewport, ...,
// exitscript = CameraCuts_StartCallback, name = 0xE59E43FD }: when the shot ends the camera's exit script runs it again
// for the next one. It has no wait. 0xF7DE45C4 calls it once when the cuts start; 0x0183A6EF stops them
// (killspawnedscript of it, KillCamAnim of 0xE59E43FD). Of the game's 9287 scripts only CameraCuts_StartCallback names
// BandManager_NotifyAllOfCameraCut (0x22136D59), once, outside any if and past its only return, and only it, 0xF7DE45C4
// and 0x0183A6EF name CameraCuts_StartCallback. PlayIGCCam itself (sub_82515570) is not the place: 18 other scripts
// call it, outside the songs' cuts.
// BandManager_NotifyAllOfCameraCut is sub_8215F4F8 (the script functions' table, its entry at 0x82810EAC, its name at
// 0x820064D4): it calls sub_8215E858 on the band manager ([0x828B07C8]), which sets the byte +57 of each of its 8
// members (the musicians told of the cut), and returns 1. Nothing in the executable calls sub_8215F4F8 but the table,
// nor sub_8215E858 but it. So it runs once per new shot, on the thread that runs the scripts (the main thread), and
// never for a camera's own moves within a shot (those are PlayIGCCam's animation).
//
// When. The main thread hands its frames to the render thread one at a time: sub_8242C450 (the main thread's frame)
// waits until the render thread has taken the last one ([0x82A81CF8 + 136] back to 0, 0x8242C514), then sub_82559FD0
// hands over the frame it built (+136 = +132, 0x8255A0FC; 0 when there is none) inside the critical section
// 0x82A81CDC. The render thread's loop (sub_824333F8) takes it in sub_82432110 (sub_825533F8: +136 to +140, 0x8255343C,
// in the same critical section), draws it (sub_82432EC0) and presents it (sub_82433138). So while the scripts run the
// cut for frame F the render thread can still be drawing frame F - 1, whose D3D calls NativeXRenderer captures:
// ResetTemporalHistory called from the cut would ride on the next command it captures, F - 1's, and reset F - 1's
// DLSS; the cut frame would then still take the last shot as its history. The hooks carry the cut with the frame
// instead: the cut marks the frame being built, its handing over marks it handed, and the render thread asks for the
// reset as it takes that frame, after F - 1's present and before any of F's draws, so the request rides on one of F's
// commands. The handing over and the taking are in one critical section, so the mark cannot reach the frame before.
// Two frames are reset: F and the next. Whether the new shot's camera is placed in F (the camera's update after the
// scripts in the main thread's frame) or in F + 1 was not traced. A reset where there is no cut costs that frame's
// history (one frame of DLSS's first image); a reset that comes before the cut does nothing.
// With the split render on (D146) DLSS runs in the second frame of each pair and holds a request until then, and the
// main view's camera is set in the first frame of a pair only (the render thread's probe in ghwor_017: changed in 449
// first frames, the same in all 447 second ones). Two frames left one case open: a cut frame F that starts a pair has
// both requests land on that pair's one application, and when the new camera is placed in F + 1 (the pair's second
// frame, which keeps the first one's camera) it is first drawn in F + 2 and upscaled in F + 3, the next pair, with the
// last shot as its history. So with the split on three frames are reset, F, F + 1 and F + 2: always two applications,
// the cut's pair and the next. A cut F that ends a pair resets that pair (the last shot's camera still, a pair's
// history lost) and the next, the new shot's first wherever its camera was placed. The cvar is read at the cut (the
// SDK's settings can change it; the game's own SetSplitRenderMode 0 draws whole frames, where the third reset costs one
// frame's history).

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>

#include <native_x_renderer/native_x_renderer.h>

#include <atomic>
#include <cstdint>
#include <string>

#include "options.h"

// src/split_render.cpp's: the main view's 3D scene over two frames (D146's pairs).
REXCVAR_DECLARE(bool, ghwor_split_render);

REXCVAR_DEFINE_BOOL(ghwor_camera_cut_reset, true, "GHWoR",
                    "With NativeXRenderer's DLAA, DLSS or FSR 3.1: their history starts again at each camera cut of "
                    "a song (the cut script CameraCuts_StartCallback), so the last shot does not ghost over the new "
                    "one (false = kept across the cuts, to compare)");

namespace {

// Frames reset per cut (above: the cut's frame and the next; with the split render on, D146, the two after it too).
constexpr int kResetFrames = 2;
constexpr int kResetFramesSplit = 3;
// Cuts logged; the next ones only counted (NativeXRenderer's line every 600 frames counts the applications reset at the
// game's request: kResetFrames a cut with the split render off; two with it on, D146, where a cut's three requests
// land on two pairs' applications).
constexpr uint32_t kLoggedCuts = 5;

// The temporal mode NativeXRenderer draws with ("" for none: the Xenos path, SMAA, FXAA). Both are fixed at startup.
const std::string& TemporalMode() {
  static const std::string mode = [] {
    if (!native_x_renderer::Renders()) {
      return std::string();
    }
    std::string aa = rex::cvar::Query<std::string>("native_x_renderer_antialiasing");
    return ghwor::IsTemporalAntialiasing(aa) ? aa : std::string();
  }();
  return mode;
}

std::atomic<uint32_t> g_cuts{0};         // cuts so far (numbered from 1)
std::atomic<uint32_t> g_cut_built{0};    // the last cut, in the frame the main thread is building (0 = none)
std::atomic<uint32_t> g_cut_handed{0};   // the last cut, in the frame handed to the render thread (0 = none)
int g_resets_left = 0;                   // the render thread's: frames it takes that still reset the history

}  // namespace

// [[midasm_hook]] 0x8215F4F8 (entry of sub_8215F4F8, the script function BandManager_NotifyAllOfCameraCut, called only
// by the cut script CameraCuts_StartCallback, once per shot).
void GHWoRCameraCutHook() {
  if (!REXCVAR_GET(ghwor_camera_cut_reset) || TemporalMode().empty()) {
    return;
  }
  g_cut_built.store(g_cuts.fetch_add(1, std::memory_order_relaxed) + 1, std::memory_order_relaxed);
}

// [[midasm_hook]] 0x8255A0FC (sub_82559FD0, the main thread handing its frame to the render thread: before "stw
// r8,136(r31)", in the critical section 0x82A81CDC; r8 = the frame, 0 when none was built). Once a frame.
void GHWoRFrameHandOffHook(PPCRegister& r8) {
  if (r8.u32 != 0 && g_cut_built.load(std::memory_order_relaxed) != 0) {
    g_cut_handed.store(g_cut_built.exchange(0, std::memory_order_relaxed), std::memory_order_relaxed);
  }
}

// [[midasm_hook]] 0x8255343C (sub_825533F8, the render thread taking the frame handed over: before "stw r9,140(r31)",
// in the same critical section). Once a frame, before the frame's draws.
void GHWoRFrameTakenHook() {
  if (g_cut_handed.load(std::memory_order_relaxed) != 0) {
    const uint32_t cut = g_cut_handed.exchange(0, std::memory_order_relaxed);
    if (cut != 0) {
      const bool split = REXCVAR_GET(ghwor_split_render);
      g_resets_left = split ? kResetFramesSplit : kResetFrames;
      if (cut <= kLoggedCuts) {
        REXLOG_INFO("[camera cut] {}: a new shot (the cut script 0xA5BB57BB, CameraCuts_StartCallback, called "
                    "BandManager_NotifyAllOfCameraCut): {}'s history starts again in this frame and the next{} "
                    "(NativeXRenderer's ResetTemporalHistory, D145){}",
                    cut, TemporalMode(), split ? " two (the split render: two pairs, D146)" : "",
                    cut == kLoggedCuts ? "; the next cuts are not logged (NativeXRenderer's line every 600 frames "
                                         "counts the resets the game asked for, two a cut)"
                                       : "");
      }
    }
  }
  if (g_resets_left > 0) {
    --g_resets_left;
    native_x_renderer::ResetTemporalHistory();
  }
}
