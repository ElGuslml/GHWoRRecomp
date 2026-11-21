// GH Warriors of Rock: more than about 700 songs in one song cache, without the crash.
//
// A cdl*_text.pak defines the songs' metadata (gh6_dlc_songlist_props: one struct per song); Onyx's song cache ("GH:WoR
// Customs Database", ghwor_custom_cache) is one such pak with every custom song in it. Its creator notes that slightly
// over 700 songs in one file crash the game; here they crash it while the pak loads, seconds after the content listing:
// "write of guest 0x00000000", lr 0x822A9880, stack 822A99A0 822A86B8 822A8DCC 823A4958. Two of the engine's fixed
// sizes run out, one after the other:
//
// - sub_822A9878 inserts a key in a script hash table (the table in r3: +0 the buckets, +4 their count, +8 the free
//   list of 16-byte nodes {key, two value words, next in the bucket}, +12 the nodes' block, +24 the count used; built by
//   sub_822B1B60). It takes the free list's head without a check: with the list empty the node is address 0. These
//   tables hold 2148 nodes. GHWoRHashPoolHook gives an empty list 4096 more nodes from the system heap. Nodes only go
//   back to the free list (sub_822B1BD0 clears a table that way, whatever block they came from), so the new ones are
//   reused, never freed.
// - The inserted value is copied into an item of a fixed-size pool (sub_822B1F48 -> sub_823D0510), 6400 items of 204
//   bytes; sub_823D0510 returns 0 from an empty pool and this caller does not check ("write of guest 0x00000070", lr
//   0x822CA368). A pool (sub_823D0570(pool, item size r4, count r5, ...): +84 its block, +88 the block's end, +92 the
//   count, +96 the item size, +108 the free list) cannot be grown later: its callers give an item back to it only if it
//   lies in [+84, +88) and to the main heap otherwise. GHWoRPoolCreateHook makes the pools named by ghwor_pool_counts
//   bigger when they are built, from the game's own heap. 20000 items hold the 1746 songs tested (one cache, 2.2 MB);
//   the 12-byte pool, which also runs out with them (its callers fall back to the heap), gets 60000.
//
// GHWoRPoolEmptyHook logs, once per pool, a pool that runs out: the value to add to ghwor_pool_counts. The game's
// small-block pools (20 to 56 bytes) run out in normal play and fall back to the heap; those are logged too.

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <set>
#include <string>

REXCVAR_DEFINE_BOOL(ghwor_hash_pool_grow, true, "GHWoR",
                    "A script hash table out of nodes gets 4096 more instead of crashing the game (more than about "
                    "700 songs in one song cache). false = as on the console");
REXCVAR_DEFINE_STRING(ghwor_pool_counts, "204:6400=20000,12:20000=60000", "GHWoR",
                      "Fixed-size pools built bigger, \"item size:count=new count\" separated by commas (the 204-byte "
                      "items hold the songs' metadata: more than about 700 songs in one song cache). Empty = as on "
                      "the console");

namespace {

constexpr uint32_t kNodeSize = 16;
constexpr uint32_t kNodesPerGrow = 4096;

rex::memory::Memory* GuestMemory() {
  auto* kernel = rex::system::kernel_state();
  return kernel ? kernel->memory() : nullptr;
}

uint32_t LoadBE32(const uint8_t* host) {
  uint32_t value;
  std::memcpy(&value, host, 4);
  return std::byteswap(value);  // guest memory is big-endian
}

void StoreBE32(uint8_t* host, uint32_t value) {
  value = std::byteswap(value);
  std::memcpy(host, &value, 4);
}

}  // namespace

// [[midasm_hook]] 0x822A9878 (sub_822A9878's first instruction, mflr r12; the table in r3).
void GHWoRHashPoolHook(PPCRegister& r3) {
  rex::memory::Memory* memory = GuestMemory();
  if (!REXCVAR_GET(ghwor_hash_pool_grow) || !memory || r3.u32 == 0) {
    return;
  }
  uint8_t* table = memory->TranslateVirtual(r3.u32);
  if (LoadBE32(table + 8) != 0) {
    return;
  }
  const uint32_t block = memory->SystemHeapAlloc(kNodeSize * kNodesPerGrow);
  if (block == 0) {
    REXLOG_WARN("[script pools] hash table {:08X} out of nodes, and no system memory for more", r3.u32);
    return;
  }
  uint8_t* nodes = memory->TranslateVirtual(block);
  std::memset(nodes, 0, kNodeSize * kNodesPerGrow);
  for (uint32_t i = 0; i + 1 < kNodesPerGrow; ++i) {
    StoreBE32(nodes + i * kNodeSize, block + (i + 1) * kNodeSize);
  }
  StoreBE32(table + 8, block);
  REXLOG_INFO("[script pools] hash table {:08X} out of nodes ({} used): {} more at {:08X}", r3.u32,
              LoadBE32(table + 24), kNodesPerGrow, block);
}

// [[midasm_hook]] 0x823D0570 (sub_823D0570's first instruction, mflr r12; the pool in r3, its item size in r4, its
// count in r5).
void GHWoRPoolCreateHook(PPCRegister& r3, PPCRegister& r4, PPCRegister& r5) {
  const std::string spec = REXCVAR_GET(ghwor_pool_counts);
  for (size_t at = 0; at < spec.size();) {
    size_t end = spec.find(',', at);
    if (end == std::string::npos) {
      end = spec.size();
    }
    unsigned size = 0, count = 0, grown = 0;
    if (std::sscanf(spec.substr(at, end - at).c_str(), " %u:%u=%u", &size, &count, &grown) == 3 && size == r4.u32 &&
        count == r5.u32 && grown > count) {
      REXLOG_INFO("[script pools] pool {:08X}: {} items of {} bytes instead of {}", r3.u32, grown, size, count);
      r5.u64 = grown;
      return;
    }
    at = end + 1;
  }
}

// [[midasm_hook]] 0x823D0548 (sub_823D0510's li r3,0: the pool in r11 is empty).
void GHWoRPoolEmptyHook(PPCRegister& r11) {
  static std::mutex mutex;
  static std::set<uint32_t> logged;
  rex::memory::Memory* memory = GuestMemory();
  if (!memory) {
    return;
  }
  {
    std::lock_guard lock(mutex);
    if (!logged.insert(r11.u32).second) {
      return;
    }
  }
  const uint8_t* pool = memory->TranslateVirtual(r11.u32);
  const uint32_t count = LoadBE32(pool + 92), size = LoadBE32(pool + 96);
  REXLOG_WARN("[script pools] pool {:08X} ran out ({} items of {} bytes; ghwor_pool_counts \"{}:{}=<more>\" builds it "
              "bigger)",
              r11.u32, count, size, size, count);
}
