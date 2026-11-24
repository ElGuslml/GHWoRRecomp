// GH Warriors of Rock: stamps the game writes into memory the GPU reads, written where the page watches do not see
// them.
//
// When SetStreamSource (sub_822D4CC0) or SetIndices (sub_822D4E68) unbinds a vertex or index buffer, D3D writes the
// device's current fence into the buffer's header (resource + 8), to know later when the GPU is done with it. The
// game keeps those headers in physical memory, next to vertex data and inside textures' packed mip tails, and the
// renderers watch physical memory for the game's writes: each stamp on a watched page faults (on the game's render
// thread) and invalidates the page, which is then uploaded, protected or hashed again; a page stamped every frame is
// left unwatched (NativeXRenderer D97) and compared at every request. With every page watched, 85 % of the writes that
// faulted in a song were these two stores (local_docs/31). A check over a song: none of 42 million vertex and index
// ranges the draws read held a word stamped in the last 20 ms.
//
// Two more (local_docs/76, step 3.1): a render object's last drawn frame (sub_8263D160 writes the frame number at
// object + 112 each time it draws the object) and SetTexture's fence in the previous texture's header (sub_822D8500,
// texture + 8). The render objects share pages with textures, so in a song nearly every write fault left on a
// texture's page was the object's stamp (14 a frame at 120 FPS with the split render, 26 at 160 with the venue every
// frame): each a fault on the game's thread, an invalidation, the page protected again and its textures checked. A
// check over two songs: none of the vertex and index ranges the draws read held a word these two stamped in the last
// 20 ms; textures' ranges did (9-18 a frame), each time in the free space of the texture's packed mip tail, which no
// image is made of (NativeXRenderer D148), and the one texture load a drawn frame left after D148 was this stamp on a
// texture hashed whole: written so, texture loads went from 0.5-1 a frame to none.
//
// The hooks write the stamp through the host view of physical memory the watches never protect (the one the SDK's
// data providers write through) and skip the game's store: the same bytes in the same memory, read back by the game
// through its view, without a fault or an invalidation. No GPU work reads a resource's header or a render object; a
// real write to the page (new vertex data) still faults and uploads it, the current stamp with it.
// ghwor_fence_stamps_unwatched = false (the D3D fences of vertex and index buffers) and
// ghwor_draw_stamps_unwatched = false (the render objects' frame and the textures' fence) store as the game does.

#include <rex/cvar.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <bit>
#include <cstdint>

REXCVAR_DEFINE_BOOL(ghwor_fence_stamps_unwatched, true, "GHWoR",
                    "The fence D3D writes into a vertex or index buffer's header when SetStreamSource or SetIndices "
                    "unbinds it is written where the renderers' page watches do not see it: no fault, and the page "
                    "is not uploaded or compared again for it. false = the game's own store");
REXCVAR_DEFINE_BOOL(ghwor_draw_stamps_unwatched, true, "GHWoR",
                    "The frame a render object was last drawn in, and the fence D3D writes into a texture's header when "
                    "SetTexture unbinds it, are written where the renderers' page watches do not see them: no fault on "
                    "the textures' pages they share. false = the game's own store");

namespace {

// Writes `value` at `address` (guest virtual) through the physical view; false if it is not in physical memory (no
// watch there: the game's store does it).
bool StoreUnwatched(bool enabled, uint32_t address, uint32_t value) {
  if (!enabled) {
    return false;
  }
  auto* kernel = rex::system::kernel_state();
  rex::memory::Memory* memory = kernel ? kernel->memory() : nullptr;
  if (!memory) {
    return false;
  }
  if ((address & 3) != 0) {
    return false;
  }
  const uint32_t physical = memory->GetPhysicalAddress(address);
  if (physical == UINT32_MAX) {
    return false;
  }
  *memory->TranslatePhysical<uint32_t*>(physical) = std::byteswap(value);  // guest memory is big-endian
  return true;
}

}  // namespace

// stw r11,8(r28) at 0x822D4D50 (SetStreamSource: the stream's previous vertex buffer).
bool GHWoRStreamFenceHook(PPCRegister& r11, PPCRegister& r28) {
  return StoreUnwatched(REXCVAR_GET(ghwor_fence_stamps_unwatched), r28.u32 + 8, r11.u32);
}

// stw r11,8(r30) at 0x822D4E94 (SetIndices: the previous index buffer).
bool GHWoRIndicesFenceHook(PPCRegister& r11, PPCRegister& r30) {
  return StoreUnwatched(REXCVAR_GET(ghwor_fence_stamps_unwatched), r30.u32 + 8, r11.u32);
}

// stw r11,8(r28) at 0x822D862C (SetTexture: the sampler's previous texture).
bool GHWoRTextureFenceHook(PPCRegister& r11, PPCRegister& r28) {
  return StoreUnwatched(REXCVAR_GET(ghwor_draw_stamps_unwatched), r28.u32 + 8, r11.u32);
}

// stw r11,112(r29) at 0x8263D35C (sub_8263D160: the frame the render object is drawn in).
bool GHWoRObjectFrameHook(PPCRegister& r11, PPCRegister& r29) {
  return StoreUnwatched(REXCVAR_GET(ghwor_draw_stamps_unwatched), r29.u32 + 112, r11.u32);
}
