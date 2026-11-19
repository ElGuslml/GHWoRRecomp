// ghwor - a song's music started with its notes (ghwor_song_start_sync; off: the game's own arithmetic).
//
// The notes follow the song's clock, a double in seconds at 0x82993A58: ResetSongTimer (sub_823D0B38, through the script
// reset_song_time, which divides its milliseconds by 1000) sets it, and sub_823D1490 adds each main-thread frame's
// vertical blanks to it. The music is a stream of the song player: SongAudioLoad loads it (SAP_Load, sub_820D3EC0) and,
// for a song started later than its beginning (a practice section), seeks it there (SAP_Seek, sub_820D4538); the
// script begin_song_after_intro (0x647F97EE: "GetSongTimeMS, if time >= starttimeafterintro break, wait 1 gameframe")
// plays it (SongAudioPlay -> SAP_Play, sub_820D3800) in its first pass with the clock at the song's start or past it.
// From then on clock and music run together, so the clock must be before the start when the stream waits: the music
// then starts within a pass of it.
//
// A song that follows another without the menus (start_gem_scroller with loading_transition = 1: a chapter's next part
// or a playlist's next song, after the results screen or a cinematic) resets the clock, after the stream was loaded, to
// 0 - transition_time - 2 x transition_lag_offset, where transition_lag_offset = 0 - video_offset - audio_offset (the
// calibration's; the first song after the menus subtracts it once, from its 3 s intro). The Rush chapter's parts 2 to 7
// have a 100 ms intro (g_RushIntro0), so with an audio calibration more than 50 ms above the video's the clock starts
// past 0: +140 ms with 0 / 120, +300 ms with 0 / 200. The first pass plays the stream from its beginning and the notes
// stay that far ahead of the music until a pause (PauseAudio seeks the stream to the clock). Measured with a probe
// (Gustavo's report, 2026-11-18): the notes 172 ms ahead of the music for a whole part with 0 / 120, 333 ms with
// 0 / 200; a chapter's first part and a playlist's songs (3 s intros) 4 to 22 ms, as any song.
//
// With ghwor_song_start_sync (on):
// - A stream "waits" from a SAP_Load that loaded it to SAP_Play, or to SAP_Stop, SAP_Unload, SAP_DeInit or SAP_UnPause
//   (the practice menu's preview plays its stream by unpausing it). Its start is 0, or the time of a SAP_Seek made
//   while it waits.
// - Rule A: a ResetSongTimer while a stream waits, to a time later than 50 ms before its start (and less than 1 s after
//   it: the transitions' arithmetic gives at most +0.4 s), sets the clock 100 ms before the start instead: as the
//   console with no calibration (the Rush intro's own length). The game's poll then starts the music as in any song.
//   50 ms: a reset is followed by a few frames before the poll's first pass (3 in the Rush chapter: 18.8 ms at 160 FPS,
//   50 at 60).
// - Rule B, for any other path: at SAP_Play, while a stream waits, a clock more than 50 ms past its start is set to it
//   (written as SetSongTime, sub_823D0C68, writes it). With rule A it does not act in the Rush chapter.
// - begin_song_after_intro runs every frame above 60 FPS (src/framerate.cpp), so the music starts within a frame of the
//   song's start (6.25 ms at 160 FPS), not within a 60th of a second.
// Left alone: the pause (its own seek), practice loops (no load), the whammy rewind, jam sessions (no song player), the
// calibration screen (no begin_song_after_intro).
// Every song's start is logged with or without the option ("[song sync]": the clock against the stream's start at
// SAP_Play, and what the rules did). docs/song-sync.md; local_docs/61, 62, 63.

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>

REXCVAR_DEFINE_BOOL(ghwor_song_start_sync, true, "GHWoR",
                    "A song's music started with its notes: a song that follows another (a chapter's next part, after "
                    "a cinematic) no longer starts its clock past the music's start with an audio calibration larger "
                    "than the video's (the notes then stayed ahead of the music until a pause), and above 60 FPS the "
                    "music starts within a frame of its time. false = the game's own arithmetic");

namespace {

// The song's clock: a double, seconds.
constexpr uint32_t kSongClock = 0x82993A58;

// Rule A: a reset later than the start less kResetEarly, and earlier than the start plus kResetFar, is moved to the
// start less kResetBefore.
constexpr double kResetEarly = 0.050;
constexpr double kResetBefore = 0.100;
constexpr double kResetFar = 1.0;
// Rule B: a clock more than this past the start at SAP_Play.
constexpr double kPlayLate = 0.050;

struct WaitingStream {
  bool waiting = false;
  double start = 0.0;  // seconds
  bool reset_moved = false;
  double reset_from = 0.0;
};

std::mutex g_mutex;
WaitingStream g_stream;

uint8_t* GuestBase() {
  auto* kernel = rex::system::kernel_state();
  return kernel && kernel->memory() ? kernel->memory()->virtual_membase() : nullptr;
}

double LoadDouble(const uint8_t* base, uint32_t address) {
  uint64_t bits;
  std::memcpy(&bits, base + address, 8);
  bits = __builtin_bswap64(bits);
  double value;
  std::memcpy(&value, &bits, 8);
  return value;
}

void StoreDouble(uint8_t* base, uint32_t address, double value) {
  uint64_t bits;
  std::memcpy(&bits, &value, 8);
  bits = __builtin_bswap64(bits);
  std::memcpy(base + address, &bits, 8);
}

void EndWaiting() {
  std::lock_guard lock(g_mutex);
  g_stream.waiting = false;
}

}  // namespace

// [[midasm_hook]] 0x820D3FCC (SAP_Load, sub_820D3EC0: after "bl 0x820D6BE8", the song player's load): r3 = loaded.
void GHWoRSongLoadHook(PPCRegister& r3) {
  std::lock_guard lock(g_mutex);
  g_stream = {};
  g_stream.waiting = (r3.u32 & 0xFF) != 0;
}

// [[midasm_hook]] 0x820D45F8 (SAP_Seek, sub_820D4538: "bl 0x820D7930", the song player's seek): r4 = the time, ms.
void GHWoRSongSeekHook(PPCRegister& r4) {
  std::lock_guard lock(g_mutex);
  if (g_stream.waiting) {
    g_stream.start = double(r4.u32) / 1000.0;
  }
}

// [[midasm_hook]] 0x823D0B74 (ResetSongTimer, sub_823D0B38: "addi r31,r10,14936", before f0, the new time in seconds,
// is stored to the clock and to its start at +8). Rule A.
void GHWoRSongResetHook(PPCRegister& f0) {
  if (!REXCVAR_GET(ghwor_song_start_sync)) {
    return;
  }
  std::lock_guard lock(g_mutex);
  const double time = f0.f64;
  if (!g_stream.waiting || !(time > g_stream.start - kResetEarly) || !(time < g_stream.start + kResetFar)) {
    return;
  }
  g_stream.reset_moved = true;
  g_stream.reset_from = time;
  f0.f64 = g_stream.start - kResetBefore;
  REXLOG_INFO("[song sync] the song's clock reset to {:+.1f} ms, past the music's start at {:.3f} s while it waits: "
              "set to {:+.1f} ms (ghwor_song_start_sync)",
              (time - g_stream.start) * 1000.0, g_stream.start, -kResetBefore * 1000.0);
}

// [[midasm_hook]] 0x820D3800 (the entry of SAP_Play, sub_820D3800: SongAudioPlay starts the song's stream). Rule B and
// the song's log line.
void GHWoRSongPlayHook() {
  WaitingStream stream;
  {
    std::lock_guard lock(g_mutex);
    stream = g_stream;
    g_stream.waiting = false;
  }
  if (!stream.waiting) {
    return;
  }
  uint8_t* base = GuestBase();
  if (!base) {
    return;
  }
  const bool on = REXCVAR_GET(ghwor_song_start_sync);
  const double late = LoadDouble(base, kSongClock) - stream.start;
  const bool set_back = on && late > kPlayLate;
  if (set_back) {
    StoreDouble(base, kSongClock, stream.start);
  }
  REXLOG_INFO("[song sync] the music starts from {:.3f} s with the notes' clock {:+.1f} ms from it{}{}{}",
              stream.start, late * 1000.0,
              stream.reset_moved ? fmt::format("; its reset to {:+.1f} ms was moved (rule A)",
                                               (stream.reset_from - stream.start) * 1000.0)
                                 : std::string(),
              set_back ? "; the clock set back to the start (rule B)" : "",
              on ? "" : " (ghwor_song_start_sync off)");
}

// The stream no longer waits: stopped, unloaded, the song player shut down, or played by unpausing it.
// [[midasm_hook]] 0x820D3848 (the entry of SAP_Stop, sub_820D3848).
void GHWoRSongStopHook() {
  EndWaiting();
}

// [[midasm_hook]] 0x820D4000 (the entry of SAP_Unload, sub_820D4000).
void GHWoRSongUnloadHook() {
  EndWaiting();
}

// [[midasm_hook]] 0x820D4488 (the entry of SAP_DeInit, sub_820D4488).
void GHWoRSongDeInitHook() {
  EndWaiting();
}

// [[midasm_hook]] 0x820D395C (SAP_UnPause, sub_820D38E8: "bl 0x820D78D8", the stream unpaused).
void GHWoRSongUnPauseHook() {
  EndWaiting();
}
