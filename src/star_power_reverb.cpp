// ghwor - Guitar Hero World Tour's Star Power sound (ghwor_star_power_reverb, off: as on the console;
// docs/star-power-reverb.md).
//
// World Tour (its default.xex and its scripts, decoded from its qb.pab.xen and named by its dbg.pak.xen): preload_song
// (0x94323F7F, scripts\guitar\guitar_song.qb) creates a send stack for each song on starpower_sendeffect_slot (1),
// CreateEffectSendStack effects = [Starpower_Echo, Starpower_Reverb, HighPass_StarPower]
// (scripts\game\audio\audio_effects_settings.qb), end_song destroys it, and the pause mutes and unmutes it
// (PauseGh3Sounds, UnpauseGh3Sounds). Star Power on (GuitarEvent_StarPowerOn -> GH_Star_Power_Verb_On, 0xDF84B375,
// scripts\game\audio\global_sound_logic.qb) sets the echo's delay from the song's tempo (get_song_tempo_cfunc), then
// the echo's mix and the reverb for the player's instrument, single player or not, and sends the player's own stream to
// the stack (SongSetStarpowerEffectsSendVolume, at once: 2^(dB / 6), at most 4, World Tour's sub_820E2378); Star Power
// off (GH_Star_Power_Verb_Off, 0x4CB84F65) cuts that send (-100 dB). The band and the crowd do not go through it.
//
// Warriors of Rock calls the same scripts under new names (audio_gameplay_star_power_verb_on 0x0FF8F452 and _off
// 0xE9628649) and keeps the three structs with World Tour's values, unused: its Star Power only raises the player's
// send to the venue's reverb (SAP_SetVenueEffectSendVolume player vol = 0 dB guitar, -10 bass, -3 drums; -100 off), the
// one stack it creates, at the boot, on slot 1 (audio_effects_create_settings_by_venue: a sfxreverb named VenueVerb).
// Its engine (FMOD Ex, linked into the executable, so recompiled code) still has all World Tour used: the echo,
// sfxreverb and highpass types (the table 0x828587D8), two send stack slots (0x829939A0, 12 bytes each: the stack's
// first effect, its return connection, its return volume; sub_823BED88, sub_823BEDA8, sub_823BEC78, sub_823BECF0)
// of which slot 2 is never used, and two sends per track of the song player (sub_820D95C0, sub_820D96B8,
// sub_820D9878).
//
// With the option, the boot creates World Tour's stack on slot 2 right after the venue's (the hook in
// CreateEffectSendStack), and a player's Star Power sends his tracks to it with World Tour's settings instead of
// raising the venue's send (the hook at sub_820D7148, the one place both scripts' SAP_SetVenueEffectSendVolume player
// = ... reaches). The engine's own script functions apply everything: CreateEffectSendStack (sub_82512900) and
// SetSoundBussEffects (sub_82512818), given QB structs written in guest memory as a script would hand them, and the
// track's send (sub_820D95C0). The engine applied a send at once with the next slot's volume (fixed below for the
// tracks slot 2 sends, with the option on); slot 2's sends are cut at the song player's clean-up and removed by a
// track's destruction, and slot 2 follows the pause. The game's code is called from the hooks on the thread's own
// context, below its live frames, the context handed back as it was after (LiveContext, GuestCall), and only after
// every pointer it is given was found in the game's memory. Anything not as expected leaves the game's own behaviour,
// with a line in the log.
//
// Checked in the game on 2026-11-15 (docs/star-power-reverb.md). The first run (guitar, one player) found the echo's
// delay worked out from 60000 / the beat (the cfunc's tempo) instead of the beat itself (fixed in EchoDelay). Testing
// builds then showed the stack as World Tour's: wired send -> echo -> sfxreverb -> highpass -> the Master buss, each
// DSP active, World Tour's settings reached, the echo at 1.5 beats and the 3.24 s reverb measured in the sound. With
// World Tour's single guitar settings the stack's sound is about 13 dB under the whole mix, and
// ghwor_star_power_reverb_send_db and ghwor_star_power_reverb_multi_preset let the user raise it. Songs played with
// the option on had the Star Power echo sound right by ear.

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/memory/utils.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/thread_state.h>
#include <rex/system/xmemory.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

REXCVAR_DEFINE_BOOL(ghwor_star_power_reverb, false, "GHWoR",
                    "Guitar Hero World Tour's Star Power sound: while a player's Star Power is on, his instrument goes "
                    "through an echo timed to the song, a reverb and a high pass, with World Tour's settings for each "
                    "instrument; false = as on the console (Star Power raises his send to the venue's reverb)");
REXCVAR_DEFINE_DOUBLE(ghwor_star_power_reverb_send_db, 0.0, "GHWoR",
                      "With ghwor_star_power_reverb: dB added to World Tour's send of the player's instrument into the "
                      "echo and the reverb (0 = World Tour's level; +4 to +8 for more of the effect; the send stops at "
                      "4x, +12 dB, the engine's limit)");
REXCVAR_DEFINE_BOOL(ghwor_star_power_reverb_multi_preset, false, "GHWoR",
                    "With ghwor_star_power_reverb: one player's Star Power takes World Tour's settings for several "
                    "players (no echo, a large 3 s reverb) instead of the single player ones; false = World Tour's");

// The game's functions called from the hooks (REX_IMPORT, rex/hook.h: the generated code's own __imp__ symbols, called
// with an explicit context, GuestCall below). The two script functions do not read r4 (the script): 0 is passed.
REX_IMPORT(__imp__sub_82512900, CreateEffectSendStack, uint32_t(uint32_t, uint32_t));  // r3 = the struct
REX_IMPORT(__imp__sub_82512818, SetSoundBussEffects, uint32_t(uint32_t, uint32_t));    // r3 = the struct
REX_IMPORT(__imp__sub_820D95C0, SetTrackSend, uint32_t(uint32_t, uint32_t, float, float));  // track, slot, vol, s
REX_IMPORT(__imp__sub_823BECF0, MuteSendReturn, uint32_t(uint32_t, uint32_t));              // slot, mute

namespace {

// --- The engine's data, as the functions named read it ---

constexpr uint32_t kSlot = 2;       // the send stack slot used here: the engine's second, free in this game
constexpr uint32_t kVenueSlot = 1;  // venue_sendeffect_slot (0xE0EC7ED6), read into [SAP + 64] by sub_820D7D68
constexpr uint32_t kSlotTable = 0x829939A0;  // the send stack slots, 12 bytes each from slot 1
constexpr uint32_t kVenueStack = kSlotTable;           // slot 1's first effect (0: not created)
constexpr uint32_t kSlot2Stack = kSlotTable + 12;      // slot 2's first effect (0: not created)
constexpr uint32_t kSlot2Return = kSlotTable + 16;     // slot 2's return connection (sub_823BEB68, sub_823BECF0)
// The effects' node pool (sub_823BB2A8): +0 the next free node, +4 the nodes ever taken, +12 the nodes, +16 the
// capacity (max_sound_effects, 64). Full, sub_823BB2A8 goes on with no node and writes and reads through it.
constexpr uint32_t kEffectPool = 0x82858D40;
constexpr uint32_t kEffectNodeSize = 564;
// An effect (a node + 20, sub_823BB2A8): +4 its name (sub_823B9EE0), +532 the next one in its stack (sub_823BBBB0),
// +536 its FMOD DSP (sub_823BBD70; 0 when FMOD did not create it).
constexpr uint32_t kEffectSize = 544;
constexpr uint32_t kEffectName = 4;
constexpr uint32_t kEffectNext = 532;
constexpr uint32_t kEffectDsp = 536;
// The players (sub_821F1990, sub_821F18D0): their count and their infos, player 1 first; +156 is 1 for a player who
// plays (isSinglePlayerGame, sub_82174E48, counts them), +340 the length of the song's beat in ms, an integer (player
// 1's: get_song_tempo_cfunc 0x8215F6C0 hands it to the script as beat_duration, 0x0EBB7064, and 60000 / it as tempo,
// 0x8B4A1490, through sub_8215F630; World Tour's 0x82181F10 and 0x82181E50 do the same from their own field).
constexpr uint32_t kPlayerCount = 0x828BC510;
constexpr uint32_t kPlayerTable = 0x828BC514;
constexpr uint32_t kPlayerPlays = 156;
constexpr uint32_t kPlayerBeat = 340;
// The song player (SAP, [0x828AD820]) as sub_820D7148 reads it: +64 the venue's slot (-1 before its init), +1824+3132
// its state (sub_820D13A8; no send in states 1 and 5), +1824+3156 its players, 40 bytes each (sub_820D1360): +0 the
// part (sub_820D40D0: 1 guitar, 2 bass, 4 drums, 8 vocals), +16 the player's stem object (one of four, 380 bytes, at
// SAP + 1824 + 1604), +32 a byte, 0 for no player. The drums are the SAP's own four tracks.
constexpr uint32_t kSapVenueSlot = 64;
constexpr uint32_t kSapState = 1824 + 3132;
constexpr uint32_t kSapPlayers = 1824 + 3156;
constexpr uint32_t kSapPlayerSize = 40;
constexpr uint32_t kSapSize = kSapPlayers + 4 * kSapPlayerSize;
constexpr uint32_t kPlayerPart = 0;
constexpr uint32_t kPlayerStem = 16;
constexpr uint32_t kPlayerActive = 32;
constexpr uint32_t kStemTrack = 88;  // the stem object's track that sub_820D7148 sends (+220 is the other one)
constexpr std::array<uint32_t, 4> kSapDrumTracks = {1032, 636, 900, 768};  // kick, snare, tom, cymbal (sub_820D3720)
constexpr int32_t kPartGuitar = 1;
constexpr int32_t kPartBass = 2;
constexpr int32_t kPartDrums = 4;
// A track (sub_820D97A8, 132 bytes): +28 and +32 the first and the last effect of its own chain (sub_820D8FA0,
// sub_820D93E0; 0 before they are created and after its destruction, sub_820D95C0 then returning at once; the last
// one's DSP, +536, feeds its outputs and its sends), +36 and +40 its first output's DSP and the connection to it
// (sub_820D8F00; sub_820D93C0 is whether it has one), +52 and +56 its connections to slot 1's and slot 2's stacks,
// +60 and +92 the two slots' sends (32 bytes each: +16 the volume now, +28 a byte set once the send is set, which the
// frame's update sub_820D96B8 then applies, on the tracks it is called for).
constexpr uint32_t kTrackSize = 132;
constexpr uint32_t kTrackLastEffect = 32;
constexpr uint32_t kTrackOutput = 40;
constexpr uint32_t kTrackSlot2Volume = 92 + 16;

// A call's frames start this far below the live r1 (rex::ppc::ImportFunction's isolated call uses 0x70, an APC's
// FunctionDispatcher::Execute 176): the guest callee's parameter area stays clear of the live frame.
constexpr uint32_t kCallFrame = 0x80;

// Lines logged of each kind that repeats.
constexpr uint32_t kLoggedFallbacks = 20;
constexpr uint32_t kLoggedSends = 100;
constexpr uint32_t kLoggedCleanups = 20;

bool Enabled() {
  static const bool enabled = REXCVAR_GET(ghwor_star_power_reverb);
  return enabled;
}

// The two settings for the user (read once at the start).
double SendDbKnob() {
  static const double db = REXCVAR_GET(ghwor_star_power_reverb_send_db);
  return db;
}
bool MultiPresetKnob() {
  static const bool multi = REXCVAR_GET(ghwor_star_power_reverb_multi_preset);
  return multi;
}

// --- Guest memory ---

rex::memory::Memory* GuestMemory() {
  auto* kernel = rex::system::kernel_state();
  return kernel ? kernel->memory() : nullptr;
}

// Whether [address, address + size) is the game's memory, readable (and writable when asked), all in one heap.
bool GuestRange(uint32_t address, uint32_t size, bool writable) {
  rex::memory::Memory* memory = GuestMemory();
  if (!memory || address == 0 || size == 0 || address > UINT32_MAX - (size - 1)) {
    return false;
  }
  rex::memory::BaseHeap* heap = memory->LookupHeap(address);
  if (!heap) {
    return false;
  }
  const rex::memory::PageAccess access = heap->QueryRangeAccess(address, address + size - 1);
  return writable ? access == rex::memory::PageAccess::kReadWrite : access != rex::memory::PageAccess::kNoAccess;
}

uint32_t Load32(uint32_t address) {
  uint32_t value;
  std::memcpy(&value, GuestMemory()->TranslateVirtual(address), 4);
  return __builtin_bswap32(value);
}

uint8_t Load8(uint32_t address) { return *GuestMemory()->TranslateVirtual(address); }

void Store32(uint32_t address, uint32_t value) {
  value = __builtin_bswap32(value);
  std::memcpy(GuestMemory()->TranslateVirtual(address), &value, 4);
}

// --- Calling the game's code from a hook ---

// The thread's own guest context, when it is the one the hook runs in (the hook's r1 is that context's r1): the game's
// code is then called on it, below the live frames. Null otherwise (a context the SDK made for a call of its own), and
// the game's code is not called.
PPCContext* LiveContext(PPCRegister& r1) {
  rex::runtime::ThreadState* thread = rex::runtime::ThreadState::Get();
  PPCContext* ctx = thread ? thread->context() : nullptr;
  return ctx && &ctx->r1 == &r1 && r1.u32 > kCallFrame ? ctx : nullptr;
}

// A call of the game's code from a hook, made as the SDK's FunctionDispatcher::ExecuteTrap makes an APC's: on the
// thread's own context, its frames kCallFrame below the live r1, and the whole context with its floating-point state
// handed back as it was once the call object is gone. The thread's r1 so follows the callee, and what the SDK may run
// on the thread meanwhile from that context (an APC in an alertable wait, at r1 - 176) starts below the callee's
// frames instead of over them. The hook's registers are the context's: they hold the callee's values until then.
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
  uint32_t operator()(const Function& function, Args... args) {
    live_.r1.u64 = saved_.r1.u32 - kCallFrame;
    return function(live_, GuestMemory()->virtual_membase(), args...);
  }

 private:
  PPCContext& live_;
  PPCContext saved_;
};

// --- QB structs in guest memory ---

// A QB key: the CRC-32 of the name in lower case, without the final inversion.
constexpr uint32_t QbKey(std::string_view name) {
  uint32_t crc = 0xFFFFFFFF;
  for (char c : name) {
    crc ^= uint8_t(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
    }
  }
  return crc;
}
static_assert(QbKey("effects") == 0xB7F4C355 && QbKey("slot") == 0x53F1DF98 && QbKey("time") == 0x906B67BA);
static_assert(QbKey("starpowerecho") == 0x8ED352EE && QbKey("starpowerreverb") == 0xD7EF1990);
static_assert(QbKey("starpowerhighpass") == 0x6ABADB4C && QbKey("HighPass") == 0xD47A9AF8);

// A component of a QB struct: an integer, a float, a name (its key) or an array of structs. The types are the byte +2
// of a component (sub_82395C20, which also reads an integer where a float is asked for).
struct QbField;
using QbStruct = std::vector<QbField>;
struct QbField {
  enum class Type : uint32_t { kInteger = 1, kFloat = 2, kArray = 12, kName = 13 };
  uint32_t key;
  Type type;
  uint32_t value = 0;
  std::vector<QbStruct> structs{};
  uint32_t* address = nullptr;  // receives the guest address of the value written
};

QbField Int(std::string_view name, int32_t value) { return {QbKey(name), QbField::Type::kInteger, uint32_t(value)}; }
QbField Float(std::string_view name, float value, uint32_t* address = nullptr) {
  return {QbKey(name), QbField::Type::kFloat, std::bit_cast<uint32_t>(value), {}, address};
}
QbField Name(std::string_view name, std::string_view value) {
  return {QbKey(name), QbField::Type::kName, QbKey(value)};
}
QbField Structs(std::string_view name, std::vector<QbStruct> structs) {
  return {QbKey(name), QbField::Type::kArray, 0, std::move(structs)};
}

// Writes QB structs into a block of guest memory as the script compiler lays out a struct in a script (token 0x4A),
// each struct's header first, then its components in order, an array's structs after its own component, with its
// pointers already absolute (what the engine's sub_82394D80 makes of a script's struct when it loads it): a struct is
// 0x00000100 and its first component; a component is 0x0001TT00 (TT its type), its key, its value and the next
// component (0 for the last); an array is 0x00010A00, its count and its one struct or the list of them.
class QbWriter {
 public:
  QbWriter(uint32_t start, uint32_t size) : next_(start), end_(start + size) {}

  uint32_t Struct(const QbStruct& fields) {
    const uint32_t header = Alloc(8);
    Store(header, 0x00000100);
    uint32_t link = header + 4;
    for (const QbField& field : fields) {
      const uint32_t component = Alloc(16);
      Store(link, component);
      link = component + 12;
      Store(component, 0x00010000 | (uint32_t(field.type) << 8));
      Store(component + 4, field.key);
      if (field.type != QbField::Type::kArray) {
        Store(component + 8, field.value);
        if (field.address) {
          *field.address = component + 8;
        }
        continue;
      }
      const uint32_t array = Alloc(12);
      Store(component + 8, array);
      Store(array, 0x00010A00);
      Store(array + 4, uint32_t(field.structs.size()));
      if (field.structs.size() == 1) {
        Store(array + 8, Struct(field.structs[0]));
      } else {
        const uint32_t list = Alloc(4 * uint32_t(field.structs.size()));
        Store(array + 8, list);
        for (size_t i = 0; i < field.structs.size(); ++i) {
          Store(list + 4 * uint32_t(i), Struct(field.structs[i]));
        }
      }
    }
    return failed_ ? 0 : header;
  }

  bool failed() const { return failed_; }

 private:
  uint32_t Alloc(uint32_t size) {
    if (failed_ || end_ - next_ < size) {
      failed_ = true;
      return 0;
    }
    const uint32_t address = next_;
    next_ += size;
    return address;
  }
  void Store(uint32_t address, uint32_t value) {
    if (!failed_) {
      Store32(address, value);
    }
  }

  uint32_t next_;
  uint32_t end_;
  bool failed_ = false;
};

// World Tour's settings, in GH_Star_Power_Verb_On's words (scripts\game\audio\global_sound_logic.qb, 0xDF84B375),
// some of its floats written as integers (the same values: sub_82395C20 reads an integer where a float is asked for).
enum Echo { kEchoGuitarSingle, kEchoBass, kEchoOther, kEchoCount };
enum Reverb { kReverbGuitarSingle, kReverbBass, kReverbDrumsSingle, kReverbMulti, kReverbCount };

// setsoundbusseffects {effects = [{effect = echo name = starpowerecho WetMix drymix DecayRatio}]}
QbStruct EchoMix(QbField wet, QbField dry, QbField decay) {
  return {Structs("effects", {{Name("effect", "echo"), Name("name", "starpowerecho"), wet, dry, decay}})};
}

// setsoundbusseffects {effects = [{effect = sfxreverb name = starpowerreverb DryLevel ReverbLevel Reflectionslevel
// Room RoomRollOffFactor = 18 ReverbDelay = 0.1 DecayTime RoomHF}] time = 0.1}
QbStruct ReverbMix(int32_t dry, int32_t reverb, int32_t reflections, int32_t room, QbField decay_time,
                   int32_t room_hf) {
  return {Structs("effects", {{Name("effect", "sfxreverb"), Name("name", "starpowerreverb"), Int("drylevel", dry),
                               Int("reverblevel", reverb), Int("reflectionslevel", reflections), Int("room", room),
                               Int("roomrollofffactor", 18), Float("reverbdelay", 0.1f), decay_time,
                               Int("roomhf", room_hf)}}),
          Float("time", 0.1f)};
}

// The structs in guest memory (one block from the SDK's system heap, which zeroes it: a struct's last component links
// to 0; written once at the boot).
struct Blobs {
  uint32_t create = 0;
  uint32_t echo_delay = 0;
  uint32_t echo_delay_value = 0;  // the delay's float, written before each Star Power
  std::array<uint32_t, kEchoCount> echo{};
  std::array<uint32_t, kReverbCount> reverb{};
};
Blobs g_blobs;
constexpr uint32_t kBlobBlockSize = 0x1000;  // the nine structs take 1848 bytes

bool WriteBlobs() {
  rex::memory::Memory* memory = GuestMemory();
  const uint32_t block = memory ? memory->SystemHeapAlloc(kBlobBlockSize) : 0;
  if (!block || !GuestRange(block, kBlobBlockSize, true)) {
    return false;
  }
  QbWriter writer(block, kBlobBlockSize);
  Blobs blobs;
  // preload_song's CreateEffectSendStack slot = $starpower_sendeffect_slot effects = [{$Starpower_Echo}
  // {$Starpower_Reverb} {$HighPass_StarPower}] (audio_effects_settings.qb's structs, with their names), on slot 2.
  blobs.create = writer.Struct(
      {Int("slot", int32_t(kSlot)),
       Structs("effects",
               {{Name("effect", "echo"), Name("name", "starpowerecho"), Int("delay", 600), Float("decayratio", 0.15f),
                 Int("maxchannels", 6), Int("drymix", 1), Int("wetmix", 1)},
                {Name("effect", "sfxreverb"), Name("name", "starpowerreverb"), Int("drylevel", -10000),
                 Int("room", -100), Int("roomhf", -698), Int("roomrollofffactor", 10), Float("decaytime", 3.24f),
                 Float("decayhfratio", 0.33f), Int("reflectionslevel", -1166), Float("reflectionsdelay", 0.02f),
                 Int("reverblevel", 18), Float("reverbdelay", 0.05f), Int("diffusion", 100), Int("density", 100),
                 Int("hfreference", 5000)},
                {Name("effect", "HighPass"), Name("name", "starpowerhighpass"), Int("cutoff", 250),
                 Int("resonance", 1)}})});
  // setsoundbusseffects effects = [{effect = echo name = starpowerecho delay = <beat_duration>}]
  blobs.echo_delay = writer.Struct({Structs(
      "effects", {{Name("effect", "echo"), Name("name", "starpowerecho"),
                   Float("delay", 600.0f, &blobs.echo_delay_value)}})});
  blobs.echo[kEchoGuitarSingle] = writer.Struct(EchoMix(Int("wetmix", 1), Float("drymix", 0.7f),
                                                        Float("decayratio", 0.25f)));
  blobs.echo[kEchoBass] = writer.Struct(EchoMix(Float("wetmix", 0.3f), Float("drymix", 0.7f),
                                                Float("decayratio", 0.25f)));
  blobs.echo[kEchoOther] = writer.Struct(EchoMix(Int("wetmix", 0), Float("drymix", 0.2f), Int("decayratio", 0)));
  blobs.reverb[kReverbGuitarSingle] = writer.Struct(ReverbMix(-1500, -500, 0, -1000, Float("decaytime", 0.1f), -1));
  blobs.reverb[kReverbBass] = writer.Struct(ReverbMix(-1400, 900, 0, -2000, Float("decaytime", 0.1f), -1));
  blobs.reverb[kReverbDrumsSingle] = writer.Struct(ReverbMix(-2000, 1700, -10000, -2000, Int("decaytime", 3), -1));
  blobs.reverb[kReverbMulti] = writer.Struct(ReverbMix(-1000, 2000, 1000, -2000, Int("decaytime", 3), -500));
  if (writer.failed() || !blobs.echo_delay_value) {
    return false;
  }
  g_blobs = blobs;
  return true;
}

// --- World Tour's presets (GH_Star_Power_Verb_On) ---

struct Preset {
  const char* name;
  Echo echo;
  Reverb reverb;
  float send_db;  // SongSetStarpowerEffectsSendVolume vol
};
// The first activation (Star_power_verb_is_on = 0), single player.
constexpr Preset kGuitarSingle{"guitar, single player", kEchoGuitarSingle, kReverbGuitarSingle, 4.0f};
constexpr Preset kBassSingle{"bass, single player", kEchoBass, kReverbBass, 8.0f};
constexpr Preset kDrumsSingle{"drums, single player", kEchoOther, kReverbDrumsSingle, 12.0f};
// The first activation, multiplayer.
constexpr Preset kGuitarMulti{"guitar, multiplayer", kEchoOther, kReverbMulti, 6.0f};
constexpr Preset kBassMulti{"bass, multiplayer", kEchoOther, kReverbMulti, 8.0f};
constexpr Preset kDrumsMulti{"drums, multiplayer", kEchoOther, kReverbMulti, 7.0f};
// Star_power_verb_is_on already 1, multiplayer (the switch at the script's start; single player returns there).
constexpr Preset kGuitarAgain{"guitar, multiplayer, already on", kEchoOther, kReverbMulti, 6.0f};
constexpr Preset kBassAgain{"bass, multiplayer, already on", kEchoBass, kReverbBass, 8.0f};
constexpr Preset kDrumsAgain{"drums, multiplayer, already on", kEchoOther, kReverbMulti, 7.0f};

const Preset& PresetFor(int32_t part, bool single, bool already_on) {
  if (already_on) {
    return part == kPartGuitar ? kGuitarAgain : part == kPartBass ? kBassAgain : kDrumsAgain;
  }
  if (single) {
    return part == kPartGuitar ? kGuitarSingle : part == kPartBass ? kBassSingle : kDrumsSingle;
  }
  return part == kPartGuitar ? kGuitarMulti : part == kPartBass ? kBassMulti : kDrumsMulti;
}

// World Tour's dB to the send's volume (its sub_820E2378): 2^(dB / 6), at most 4.
float SendVolume(float db) { return std::fmin(std::exp2(db / 6.0f), 4.0f); }

// The echo's delay in ms (GH_Star_Power_Verb_On, from get_song_tempo_cfunc's beat_duration, player 1's beat in ms):
// 1.5 beats if that is 600 ms or less, else the beat halved while above 400 ms, three times at most, and 400 at most.
// The halving is done on floats here (the script's integer, if it halves it as one, differs by half a ms at most). 0
// when the beat is 0 or less (World Tour's script would then ask for a delay of 0, below the echo's 10 ms: the delay
// is left as it was). False when player 1's info is not the game's memory.
bool EchoDelay(float* delay_ms, int32_t* beat_out) {
  const uint32_t table = Load32(kPlayerTable);
  if (!GuestRange(table, 4, false)) {
    return false;
  }
  const uint32_t player1 = Load32(table);
  if (!GuestRange(player1, kPlayerBeat + 4, false)) {
    return false;
  }
  const int32_t beat_ms = int32_t(Load32(player1 + kPlayerBeat));
  *beat_out = beat_ms;
  if (beat_ms <= 0) {
    *delay_ms = 0.0f;
    return true;
  }
  float beat = float(beat_ms);
  const float one_and_a_half = beat * 1.5f;
  if (one_and_a_half > 600.0f) {
    for (int i = 0; i < 3; ++i) {
      if (beat > 400.0f) {
        beat = beat / 2.0f;
      }
    }
    if (beat > 400.0f) {
      beat = 400.0f;
    }
  } else {
    beat = one_and_a_half;
  }
  *delay_ms = beat;
  return true;
}

// isSinglePlayerGame (sub_82174E48 -> sub_821F1990(0, 0, 0) == 1): one player who plays. Empty when the players' table
// is not the game's memory.
std::optional<bool> SinglePlayer() {
  const int32_t count = int32_t(Load32(kPlayerCount));
  if (count <= 0) {
    return false;
  }
  const uint32_t table = Load32(kPlayerTable);
  if (count > 8 || !GuestRange(table, 4 * uint32_t(count), false)) {
    return std::nullopt;
  }
  int playing = 0;
  for (int32_t i = 0; i < count; ++i) {
    const uint32_t player = Load32(table + 4 * uint32_t(i));
    if (!GuestRange(player, kPlayerPlays + 4, false)) {
      return std::nullopt;
    }
    playing += Load32(player + kPlayerPlays) == 1 ? 1 : 0;
  }
  return playing == 1;
}

// --- The players' tracks ---

struct Tracks {
  std::array<uint32_t, 4> address{};
  uint32_t count = 0;
  int32_t part = 0;
};

// The tracks sub_820D7148 sends for `player` (1 to 4), chosen as it chooses them (0x820D715C to 0x820D725C): none in
// the SAP's states 1 and 5, before its init, for a player not there, or for vocals; the stem object's track for
// guitar and bass while it has its output (sub_820D93C0); the SAP's four drum tracks for drums. None either where a
// pointer is not the game's memory.
Tracks PlayerTracks(uint32_t sap, uint32_t player) {
  Tracks tracks;
  if (player < 1 || player > 4 || !GuestRange(sap, kSapSize, true)) {
    return tracks;
  }
  const uint32_t state = Load32(sap + kSapState);
  if (state == 5 || state == 1 || int32_t(Load32(sap + kSapVenueSlot)) == -1) {
    return tracks;
  }
  const uint32_t info = sap + kSapPlayers + kSapPlayerSize * (player - 1);
  if (Load8(info + kPlayerActive) == 0) {
    return tracks;
  }
  tracks.part = int32_t(Load32(info + kPlayerPart));
  if (tracks.part == kPartGuitar || tracks.part == kPartBass) {
    const uint32_t stem = Load32(info + kPlayerStem);
    const uint32_t track = stem + kStemTrack;
    if (stem && GuestRange(track, kTrackSize, true) && Load32(track + kTrackOutput) != 0) {
      tracks.address[tracks.count++] = track;
    }
  } else if (tracks.part == kPartDrums) {
    for (uint32_t offset : kSapDrumTracks) {
      tracks.address[tracks.count++] = sap + offset;
    }
  }
  return tracks;
}

// Whether a track is the game's memory and has its effect chain (sub_820D95C0 sets nothing on one without it, as
// after the track's destruction).
bool TrackLive(uint32_t track) {
  return GuestRange(track, kTrackSize, true) && Load32(track + kTrackLastEffect) != 0;
}

const char* PartName(int32_t part) {
  return part == kPartGuitar ? "guitar" : part == kPartBass ? "bass" : part == kPartDrums ? "drums" : "other";
}

// --- State ---

std::atomic<bool> g_stack_tried{false};
std::atomic<bool> g_ready{false};       // the stack on slot 2 is there, its three effects with their DSPs
std::atomic<bool> g_tail_muted{false};  // slot 2's return muted by the song's clean-up, until the next Star Power

// Under g_mutex, never held while the game's code runs.
std::mutex g_mutex;
std::array<std::vector<uint32_t>, 4> g_sent;  // each player's tracks sent to slot 2
bool g_verb_on = false;                        // World Tour's Star_power_verb_is_on

std::atomic<uint32_t> g_fallbacks{0};
std::atomic<uint32_t> g_ons{0};
std::atomic<uint32_t> g_offs{0};
std::atomic<uint32_t> g_cleanups{0};

void LogFallback(std::string_view what) {
  const uint32_t n = g_fallbacks.fetch_add(1, std::memory_order_relaxed);
  if (n < kLoggedFallbacks) {
    REXLOG_WARN("[star power reverb] {}: the game's own behaviour{}", what,
                n + 1 == kLoggedFallbacks ? " (the next such lines are not logged)" : "");
  }
}

// Whether the effect pool can give `count` more nodes: sub_823BB2A8's allocation, simulated (the next free node,
// then the free list it heads, or one more node while fewer than the capacity were ever taken). `free_nodes` gets how
// many it can give, up to the capacity.
bool PoolHasRoom(uint32_t count, uint32_t* free_nodes) {
  uint32_t next = Load32(kEffectPool + 0);
  uint32_t taken = Load32(kEffectPool + 4);
  const uint32_t nodes = Load32(kEffectPool + 12);
  const uint32_t capacity = Load32(kEffectPool + 16);
  uint32_t given = 0;
  while (given < capacity && given < 256) {
    if (next == taken) {
      if (taken >= capacity) {
        break;
      }
      next = ++taken;
    } else {
      const uint32_t node = nodes + next * kEffectNodeSize;
      if (next >= capacity || !GuestRange(node, 4, false)) {
        break;
      }
      next = Load32(node);
    }
    ++given;
  }
  *free_nodes = given;
  return given >= count;
}

// The stack on slot 2 as sub_823BEB68 left it: three effects named as asked, each with its DSP, and the return
// connection. Null when it is so; else what is missing.
const char* CheckStack() {
  static constexpr std::array<std::pair<uint32_t, const char*>, 3> kEffects = {{
      {QbKey("starpowerecho"), "the echo (starpowerecho)"},
      {QbKey("starpowerreverb"), "the reverb (starpowerreverb)"},
      {QbKey("starpowerhighpass"), "the high pass (starpowerhighpass)"},
  }};
  uint32_t effect = Load32(kSlot2Stack);
  for (const auto& [name, what] : kEffects) {
    if (!GuestRange(effect, kEffectSize, false) || Load32(effect + kEffectName) != name) {
      return what;
    }
    if (Load32(effect + kEffectDsp) == 0) {
      return "a DSP from FMOD (its memory pool, fmod_memory_size, may be full)";
    }
    effect = Load32(effect + kEffectNext);
  }
  if (effect != 0) {
    return "the end of the stack after its three effects";
  }
  if (Load32(kSlot2Return) == 0) {
    return "the stack's return connection";
  }
  return nullptr;
}

// Slot 2's send cut on `tracks` (those still with their effect chain: a destroyed track's destruction removed its
// connections), at once. The count of those cut.
uint32_t CutSends(GuestCall& call, const std::vector<uint32_t>& tracks) {
  uint32_t cut = 0;
  for (uint32_t track : tracks) {
    if (TrackLive(track)) {
      call(SetTrackSend, track, kSlot, 0.0f, 0.0f);
      ++cut;
    }
  }
  return cut;
}

// The effect pool's peak, followed from the boot's (World Tour's stack included). sub_823BB2A8 takes a freed node
// before a new one, so the nodes ever taken ([pool + 4]) are the most taken at once. The boot's check covers only the
// stack's own three nodes: the song player takes its nodes later (at its init and at each song's load: a mixer for
// each track, its effect chains, two more for each guitar or bass player, sub_820D7D68 and sub_820D8548) and the
// sounds and busses take theirs from the same pool, while with the pool full the engine writes through a null node
// (sub_823BB2A8 from 0x823BB4A0). A warning each time the peak rises with kPoolMargin nodes or fewer left: three more
// taken by the option are then what a bigger game could miss.
constexpr uint32_t kPoolMargin = 3;
std::atomic<uint32_t> g_pool_peak{0};

void LogPoolPeak() {
  const uint32_t taken = Load32(kEffectPool + 4);
  const uint32_t capacity = Load32(kEffectPool + 16);
  uint32_t peak = g_pool_peak.load(std::memory_order_relaxed);
  do {
    if (taken <= peak) {
      return;
    }
  } while (!g_pool_peak.compare_exchange_weak(peak, taken, std::memory_order_relaxed));
  const uint32_t left = capacity > taken ? capacity - taken : 0;
  if (left <= kPoolMargin) {
    REXLOG_WARN("[star power reverb] the effect pool's peak: {} of {} nodes taken at once (3 of them World Tour's "
                "stack), {} left: a game with more players may find it full with the option on (a crash); turn "
                "ghwor_star_power_reverb off if it does",
                taken, capacity, left);
  }
}

}  // namespace

// [[midasm_hook]] 0x82512980 (sub_82512900, the script function CreateEffectSendStack: after sub_823BED88 created the
// stack, r30 = the slot). The boot's audio_effects_create_settings_by_venue (from guitar.qb's startup) creates the
// venue's stack on slot 1 here, once: World Tour's stack is created on slot 2 right after it, through the same script
// function (its own call comes back here with r30 = 2). Before it, the effect pool must have three free nodes (full,
// the engine writes through a null node), which covers the stack's own nodes only: the song player takes its own
// later, and the pool's peak is followed from then on (LogPoolPeak). After it, each effect must have its DSP (FMOD's
// own pool has no check before). Anything else leaves the option off for the session, with the reason in the log.
void GHWoRStarPowerStackHook(PPCRegister& r1, PPCRegister& r30) {
  if (!Enabled() || r30.u32 != kVenueSlot || g_stack_tried.exchange(true)) {
    return;
  }
  uint32_t free_nodes = 0;
  const char* why = nullptr;
  PPCContext* live = LiveContext(r1);
  if (!live) {
    why = "the hook did not run in its thread's own context";
  } else if (Load32(kVenueStack) == 0) {
    why = "the venue's stack (slot 1) was not created";
  } else if (Load32(kSlot2Stack) != 0) {
    why = "slot 2 already has a stack";
  } else if (!PoolHasRoom(3, &free_nodes)) {
    why = "the effect pool (max_sound_effects) has fewer than 3 free nodes";
  } else if (!WriteBlobs()) {
    why = "no guest memory for the QB structs";
  }
  if (why) {
    REXLOG_WARN("[star power reverb] no stack on slot 2: {} ({} effect nodes free); the option stays off (the game's "
                "own Star Power sound)",
                why, free_nodes);
    return;
  }
  {
    GuestCall call(*live);
    call(CreateEffectSendStack, g_blobs.create, 0u);
  }
  const char* missing = Load32(kSlot2Stack) == 0 ? "everything: CreateEffectSendStack created nothing" : CheckStack();
  if (missing) {
    REXLOG_WARN("[star power reverb] the stack on slot 2 lacks {}: the option stays off (the game's own Star Power "
                "sound), the stack unused",
                missing);
    return;
  }
  g_pool_peak.store(Load32(kEffectPool + 4), std::memory_order_relaxed);
  g_ready.store(true, std::memory_order_release);
  REXLOG_INFO("[star power reverb] World Tour's stack on slot 2: echo, sfxreverb, highpass (starpowerecho, "
              "starpowerreverb, starpowerhighpass, each with its DSP; {} effect nodes were free before it, {} of {} "
              "taken at once so far); Star Power sends the player's tracks to it",
              free_nodes, Load32(kEffectPool + 4), Load32(kEffectPool + 16));
}

// [[midasm_hook]] 0x820D7148 (entry of sub_820D7148, return_on_true: r3 = the SAP, r4 = the player, 1 to 4, f1 = the
// send's volume; f2, its time in seconds, is not needed). Its one caller is SAP_SetVenueEffectSendVolume (0x820D3A88,
// at 0x820D3B14, the call with a player), which the disc's scripts call with a player only from
// audio_gameplay_star_power_verb_on (vol = 0, -10, -3 dB; sub_820D0828 makes them 2^(dB / 6): f1 = 1, 0.315, 0.707) and
// _verb_off (-100 dB: f1 = 0). The original sets the send of the venue's slot ([SAP + 64]) on the player's tracks and
// returns 1.
// Star Power on (f1 > 0): World Tour's GH_Star_Power_Verb_On instead, as it orders it: the echo's delay, the echo's
// mix, the reverb (time = 0.1), then slot 2's send on the player's tracks, at once; the venue's send is left as it
// was. Off: slot 2's send cut on the tracks the player's Star Power sent, then the original (the venue's send to 0, as
// on the console). With World Tour's flag: on sets it, off clears it, and an activation while it is set takes the
// "already on" presets in multiplayer and changes nothing in single player. The flag is the hook's own, set only by
// the ons that reach here with tracks to send: World Tour set it for any part, but a vocalist's on does not come here
// (audio_gameplay_star_power_verb_on only calls VocalDSPSetParams for vocals), while every part's off does. The
// echo's delay is left as it was for a beat of 0.
bool GHWoRStarPowerSendHook(PPCRegister& r1, PPCRegister& r3, PPCRegister& r4, PPCRegister& f1) {
  if (!Enabled()) {
    return false;
  }
  const uint32_t player = r4.u32;
  const bool on = f1.f64 > 0.0;
  if (!g_ready.load(std::memory_order_acquire)) {
    if (on) {
      LogFallback("Star Power on with no stack on slot 2 (the boot's line says why)");
    }
    return false;
  }
  PPCContext* live = LiveContext(r1);
  if (!live) {
    LogFallback("Star Power: the hook did not run in its thread's own context");
    return false;
  }
  if (!on) {
    std::vector<uint32_t> sent;
    {
      std::lock_guard lock(g_mutex);
      g_verb_on = false;
      if (player >= 1 && player <= 4) {
        sent.swap(g_sent[player - 1]);
      }
    }
    if (!sent.empty()) {
      GuestCall call(*live);
      const uint32_t cut = CutSends(call, sent);
      if (g_offs.fetch_add(1, std::memory_order_relaxed) < kLoggedSends) {
        REXLOG_INFO("[star power reverb] player {} off: slot 2's send cut on {} of the {} track{} sent{}", player,
                    cut, sent.size(), sent.size() == 1 ? "" : "s",
                    cut < sent.size() ? " (the others destroyed, their connections with them)" : "");
      }
    }
    return false;
  }

  const Tracks tracks = PlayerTracks(r3.u32, player);
  if (tracks.count == 0) {
    return false;  // the original sends nothing either (vocals, no player, the SAP not ready)
  }
  float delay_ms = 0.0f;
  int32_t beat_ms = 0;
  const std::optional<bool> single = SinglePlayer();
  if (!single || !EchoDelay(&delay_ms, &beat_ms) || !GuestRange(g_blobs.echo_delay_value, 4, true)) {
    LogFallback("Star Power on: the players' table or the guest structs are not the game's memory");
    return false;
  }
  // World Tour's single player settings, or its settings for several players with ghwor_star_power_reverb_multi_preset.
  const bool single_preset = *single && !MultiPresetKnob();
  bool already_on;
  {
    std::lock_guard lock(g_mutex);
    already_on = g_verb_on;
    if (!(already_on && single_preset)) {
      g_verb_on = true;
    }
  }
  if (already_on && single_preset) {
    // GH_Star_Power_Verb_On: "if ($Star_power_verb_is_on = 1) if not isSinglePlayerGame ... endif return".
    if (g_ons.fetch_add(1, std::memory_order_relaxed) < kLoggedSends) {
      REXLOG_INFO("[star power reverb] player {} on ({}): World Tour's flag still on in single player (its last "
                  "Star Power off was not seen): nothing changed, as World Tour does",
                  player, PartName(tracks.part));
    }
    r3.u64 = 1;
    return true;
  }
  const Preset& preset = PresetFor(tracks.part, single_preset, already_on);
  const float send_db = preset.send_db + float(SendDbKnob());
  const float volume = SendVolume(send_db);
  if (delay_ms > 0.0f) {
    Store32(g_blobs.echo_delay_value, std::bit_cast<uint32_t>(delay_ms));
  }
  uint32_t sent = 0;
  {
    GuestCall call(*live);
    if (delay_ms > 0.0f) {
      call(SetSoundBussEffects, g_blobs.echo_delay, 0u);
    }
    call(SetSoundBussEffects, g_blobs.echo[preset.echo], 0u);
    call(SetSoundBussEffects, g_blobs.reverb[preset.reverb], 0u);
    if (g_tail_muted.exchange(false) && Load32(kSlot2Return) != 0) {
      call(MuteSendReturn, kSlot, 0u);
    }
    for (uint32_t i = 0; i < tracks.count; ++i) {
      if (TrackLive(tracks.address[i])) {
        call(SetTrackSend, tracks.address[i], kSlot, volume, 0.0f);
        ++sent;
      }
    }
  }
  {
    std::lock_guard lock(g_mutex);
    std::vector<uint32_t>& recorded = g_sent[player - 1];
    for (uint32_t i = 0; i < tracks.count; ++i) {
      if (std::find(recorded.begin(), recorded.end(), tracks.address[i]) == recorded.end()) {
        recorded.push_back(tracks.address[i]);
      }
    }
  }
  if (g_ons.fetch_add(1, std::memory_order_relaxed) < kLoggedSends) {
    const std::string delay =
        delay_ms > 0.0f
            ? fmt::format("echo delay {:.0f} ms (a beat of {} ms, {:.0f} BPM)", delay_ms, beat_ms, 60000.0f / beat_ms)
            : fmt::format("echo delay left as it was (a beat of {} ms)", beat_ms);
    REXLOG_INFO("[star power reverb] player {} on ({}): World Tour's {} preset, {}, send {:+.1f} dB ({:.3f}) on {} of "
                "{} track{}",
                player, PartName(tracks.part), preset.name, delay, send_db, volume, sent, tracks.count,
                tracks.count == 1 ? "" : "s");
  }
  LogPoolPeak();  // the song's load is done by now
  r3.u64 = 1;
  return true;
}

// [[midasm_hook]] 0x820D9688 (sub_820D95C0, a track's send set: before "lfs f1,76(r11)", r11 = the track (r31) +
// 32 * the slot (r29)). After it stores the slot's new state at +60 + 32 * (slot - 1), sub_820D95C0 applies the send at
// once with sub_823BEF78(slot, ..., f1) (0 removes the connection, above 0 sets it or makes it), and f1 comes from
// +76 + 32 * slot (rlwinm r11,r29,5 at 0x820D9678): the next slot's volume now. On the console slot 2 is 0 on every
// track, so slot 1's send is applied at once with 0: the connection is removed, and the frame's update sub_820D96B8
// makes it again with the right volume on the tracks it is called for (sub_820D8360's nine, the stems' through
// sub_820D3240). It is not called for SAP + 372 and SAP + 504, the guitar and bass tracks of
// SAP_SetVenueEffectSendVolume track = guitar or bass (sub_820D3720: 4 and 5, sub_820D7278), so the venue send the
// freeplay snapshots give them (house: 0 and +2 dB, stage: -11 and -5) is never applied: kept so, the read is left
// alone while slot 2 is 0. With slot 2 used (only on the stems' and the drums' tracks, both updated each frame),
// slot 1's send would take slot 2's volume (the drums' Star Power send, up to 4, into the venue's reverb for a frame,
// from a freeplay snapshot), and a track's destruction (sub_820D9878: slot 1 then slot 2 set to 0 through here, then
// the connections forgotten) would leave slot 1's on at slot 2's level. Slot 2's own send reads +140, past the 132-byte
// track (+8 of the next one, the integer 1 sub_820D97A8 puts there: 1.4e-45): made near 0, and not removed by the
// destruction. With the option on and the stack there, the volume is read from the slot being set (r11 - 32): for
// slot 2 always, for slot 1 while slot 2's volume is not 0.
void GHWoRTrackSendFixHook(PPCRegister& r11, PPCRegister& r29, PPCRegister& r31) {
  if (!g_ready.load(std::memory_order_relaxed)) {
    return;
  }
  const uint32_t slot = r29.u32;
  if ((slot != 1 && slot != 2) || r11.u32 != r31.u32 + 32 * slot) {
    return;
  }
  if (slot == 1 && (Load32(r31.u32 + kTrackSlot2Volume) & 0x7FFFFFFF) == 0) {
    return;  // the console's read: slot 2's volume, 0
  }
  r11.u64 = r31.u32 + 32 * (slot - 1);
}

// [[midasm_hook]] 0x820D6B30 (entry of sub_820D6B30; r3, the SAP, is not needed: the tracks are those recorded): the
// song player's clean-up, which destroys its players' stem objects (sub_820D24E8: their tracks through sub_820D9878)
// and sets the drums' venue send to 0. It runs before a song is loaded (sub_820D8548 at 0x820D8588), at the song
// player's init (sub_820D7D68, after sub_820D6A90 destroyed the SAP's own tracks, the drums' among them) and at its
// deinit (sub_820D7840, after sub_820D1648 and sub_820D6A90 destroyed the stems' and the SAP's tracks). SAP_Unload, at
// a song's end, does not reach it. Slot 2's send is cut at once on every track a Star Power sent that is still there
// (Star Power may still be on: the scripts that end a song can kill the off): at a song's load, before the stems'
// tracks are destroyed, while the drums' tracks live on from the init to the deinit. A track already destroyed lost
// its connections with its destruction (sub_820D9878 through sub_820D95C0, fixed at 0x820D9688). World Tour's flag is
// cleared (its start_gem_scroller clears it at each song), and slot 2's return is muted so that what is left of the
// echo and the reverb does not ring into the next song: World Tour destroyed its stack at end_song. The next Star
// Power unmutes it, as a new song's stack started in World Tour. The effect pool's peak is checked (LogPoolPeak).
void GHWoRStarPowerCleanupHook(PPCRegister& r1) {
  if (!g_ready.load(std::memory_order_acquire)) {
    return;
  }
  std::vector<uint32_t> sent;
  {
    std::lock_guard lock(g_mutex);
    g_verb_on = false;
    for (std::vector<uint32_t>& tracks : g_sent) {
      for (uint32_t track : tracks) {
        if (std::find(sent.begin(), sent.end(), track) == sent.end()) {
          sent.push_back(track);
        }
      }
      tracks.clear();
    }
  }
  LogPoolPeak();
  PPCContext* live = LiveContext(r1);
  if (!live) {
    LogFallback("the song's clean-up: the hook did not run in its thread's own context (the tracks' destruction "
                "still removes their sends)");
    return;
  }
  GuestCall call(*live);
  const uint32_t cut = CutSends(call, sent);
  if (!g_tail_muted.exchange(true) && Load32(kSlot2Return) != 0) {
    call(MuteSendReturn, kSlot, 1u);
  }
  if (!sent.empty() && g_cleanups.fetch_add(1, std::memory_order_relaxed) < kLoggedCleanups) {
    REXLOG_INFO("[star power reverb] the song player's clean-up: slot 2's send cut on {} of the {} track{} still "
                "sent{}, slot 2's return muted until the next Star Power",
                cut, sent.size(), sent.size() == 1 ? "" : "s",
                cut < sent.size() ? " (the others destroyed, their connections with them)" : "");
  }
}

// [[midasm_hook]] 0x823BECF0 (entry of sub_823BECF0, r3 = the slot, r4 = mute: the script functions MuteEffectSend
// and UnMuteEffectSend, its only callers). The game mutes the venue's slot at the pause (PauseAudio) and unmutes it
// after (UnPauseAudio, freeplay_unpause_sound): slot 2 follows, as World Tour's stack did (PauseGh3Sounds,
// UnpauseGh3Sounds), except an unmute while the song's clean-up holds it muted. Its own call for slot 2 comes back
// here and is left alone.
void GHWoRStarPowerMuteHook(PPCRegister& r1, PPCRegister& r3, PPCRegister& r4) {
  if (r3.u32 != kVenueSlot || !g_ready.load(std::memory_order_acquire)) {
    return;
  }
  const bool mute = (r4.u32 & 0xFF) != 0;
  if ((!mute && g_tail_muted.load(std::memory_order_relaxed)) || Load32(kSlot2Return) == 0) {
    return;
  }
  PPCContext* live = LiveContext(r1);
  if (!live) {
    LogFallback("the pause: the hook did not run in its thread's own context (slot 2 not muted with slot 1)");
    return;
  }
  GuestCall call(*live);
  call(MuteSendReturn, kSlot, mute ? 1u : 0u);
}
