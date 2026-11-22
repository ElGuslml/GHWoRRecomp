// The render thread does not leave a scene with its lighting job unfinished (docs/lighting-job.md).
//
// Stage 3 of a scene's render stages (sub_82554A40) queues the lighting job sub_82648B08 (job group 365, through
// sub_826492B0), and stage 9 (sub_82554D60 -> sub_825F14C8) waits for the group and draws the light volume the job
// built. The job does not get its scene from the stage: it reads the scene the render thread is drawing, a global that
// sub_8255A3C8 stores for each scene (0x829BE24C), at its entry and again before it loads the scene's light block
// (+116). With the split render (the default) stages 1-7 run in one frame and 8-19 in the next, so in a first-half
// frame nothing waits for the job before the render thread moves on to the next scene and stores it in the global. A
// job that starts after that reads the next scene: a light volume built from another scene's data for a frame, or,
// when that scene has no light block, a read of guest address 0x70 that ends the game (a job worker's "Unhandled guest
// access violation" with lr=82648B60). On the console a worker took the job within microseconds while stages 4-7
// issued their draws for milliseconds; here those stages cost the render thread well under a millisecond, and a busy
// processor can start the job later than that.
//
// The fix: where every way out of a scene's stages meets (0x8255A994, cmplwi cr6,r20,0, before the next scene), while
// group 365 has a job queued or running the render thread calls the game's own wait for the group, as stages 3 and 9
// do (sub_82551028(365)): it runs the queued job itself or waits for the worker that has it, so the scene does not
// change under the job. Stages 3 and 9 are untouched. With the scene drawn every frame (ghwor_scene_every_frame)
// stage 9 has waited already and the counts are 0 here. ghwor_lighting_job_wait = false: the game's own behaviour.

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/thread_state.h>
#include <rex/system/xmemory.h>

#include <atomic>
#include <cstdint>

REXCVAR_DEFINE_BOOL(ghwor_lighting_job_wait, true, "GHWoR",
                    "The render thread does not move on to the next scene while the lighting job it queued for this "
                    "one has not run: a late lighting job read the next scene (a wrong light volume for a frame, or "
                    "the game ending on a scene without one). false = the game's own behaviour");

// The game's wait for a job group (the one stages 3 and 9 call): r3 = the group, r4 = 30 as they pass it (unread).
REX_IMPORT(__imp__sub_82551028, WaitForJobGroup, uint32_t(uint32_t, uint32_t));

namespace {

// The job system (sub_82551028, sub_82646B00): at +260 its group table, 32 bytes a group; a group's +0 counts the
// jobs queued, its +28 the jobs running (a worker adds to the second before it takes from the first, and the wait
// reads the first, then the second).
constexpr uint32_t kGroupTable = 0x8286B210 + 260;
constexpr uint32_t kLightingGroup = 365;
constexpr uint32_t kGroupEntrySize = 32;
constexpr uint32_t kQueued = 0;
constexpr uint32_t kRunning = 28;
// The frame the call's stack starts below the live r1 at (as src/star_power_reverb.cpp's GuestCall).
constexpr uint32_t kCallFrame = 0x80;

uint8_t* GuestBase() {
  rex::system::KernelState* kernel = rex::system::kernel_state();
  return kernel && kernel->memory() ? kernel->memory()->virtual_membase() : nullptr;
}

uint32_t LoadAcquire(uint8_t* base, uint32_t address) {
  const uint32_t value =
      std::atomic_ref<uint32_t>(*reinterpret_cast<uint32_t*>(base + address)).load(std::memory_order_acquire);
  return __builtin_bswap32(value);
}

// The thread's own context, when the hook's r1 is it (src/star_power_reverb.cpp's LiveContext).
PPCContext* LiveContext(PPCRegister& r1) {
  rex::runtime::ThreadState* thread = rex::runtime::ThreadState::Get();
  PPCContext* ctx = thread ? thread->context() : nullptr;
  return ctx && &ctx->r1 == &r1 && r1.u32 > kCallFrame ? ctx : nullptr;
}

// A call of the game's code from a hook (src/star_power_reverb.cpp's GuestCall): on the thread's own context, its
// frames kCallFrame below the live r1, and the whole context with its floating-point state handed back as it was once
// the call object is gone, so no register the stage loop holds is changed.
class GuestCall {
 public:
  explicit GuestCall(PPCContext& live) : live_(live), saved_(live) {}
  ~GuestCall() {
    live_ = saved_;
    live_.fpscr.restoreGuestBits(saved_.fpscr.csr);
  }
  GuestCall(const GuestCall&) = delete;
  GuestCall& operator=(const GuestCall&) = delete;

  template <typename Function, typename... Args>
  uint32_t operator()(const Function& function, uint8_t* base, Args... args) {
    live_.r1.u64 = saved_.r1.u32 - kCallFrame;
    return function(live_, base, args...);
  }

 private:
  PPCContext& live_;
  PPCContext saved_;
};

std::atomic<bool> g_logged{false};
std::atomic<bool> g_no_context_logged{false};

}  // namespace

// [[midasm_hook]] 0x8255A994 (hooks/lighting_job.toml; sub_8255A3C8, before cmplwi cr6,r20,0): a scene's stages are
// done, the next scene is stored after this. The counts are read as the wait reads them; the call only when one is not
// 0 (most scenes: the job has finished already).
void GHWoRLightingJobWaitHook(PPCRegister& r1) {
  if (!REXCVAR_GET(ghwor_lighting_job_wait)) {
    return;
  }
  uint8_t* base = GuestBase();
  if (!base) {
    return;
  }
  const uint32_t table = LoadAcquire(base, kGroupTable);
  if (!table) {
    return;
  }
  const uint32_t entry = table + kLightingGroup * kGroupEntrySize;
  if (LoadAcquire(base, entry + kQueued) == 0 && LoadAcquire(base, entry + kRunning) == 0) {
    return;
  }
  PPCContext* ctx = LiveContext(r1);
  if (!ctx) {
    // Not expected (the render thread is a guest thread and r1 is its context's), but then the wait is not made:
    // said once, so the fix is never off without a word in the log.
    if (!g_no_context_logged.exchange(true, std::memory_order_relaxed)) {
      REXLOG_WARN("[lighting job] no guest context on the thread at 0x8255A994: the render thread did not wait for "
                  "a scene's lighting job (ghwor_lighting_job_wait has no effect while this lasts)");
    }
    return;
  }
  {
    GuestCall call(*ctx);
    call(WaitForJobGroup, base, kLightingGroup, 30u);
  }
  if (!g_logged.exchange(true, std::memory_order_relaxed)) {
    REXLOG_INFO("[lighting job] the render thread waited for a scene's lighting job before the next scene "
                "(ghwor_lighting_job_wait)");
  }
}
