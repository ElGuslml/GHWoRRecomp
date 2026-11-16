# D3D's fence stamps and the renderers' page watches

**Symptom.** At high frame rates (240 FPS in ultrawide, the venue drawn every frame) the renderer's thread could not
keep up in busy songs: about 230 FPS on average, stretches under 200. NativeXRenderer spent a quarter of its time
checking and copying guest memory that the game had "changed" every frame.

**Cause.** When `SetStreamSource` or `SetIndices` unbinds a vertex or index buffer, the game's Direct3D writes the
device's current fence into the buffer's header (`stw r11,8(r28)` at `0x822D4D50`, `stw r11,8(r30)` at `0x822D4E94`),
to know later when the GPU is done with it. The game keeps those headers in physical memory, right before their
vertex data and inside textures' unused packed mip tiles, and both renderers watch physical memory for the game's
writes. Each stamp on a watched page cost a fault on the game's render thread and made the page (and the textures
there) look changed; a page stamped every frame was left unwatched and compared at every use. With every page
watched, 85 % of the writes that faulted in a song were these two stores.

**Fix** (`src/fence_stamps.cpp`, two `[[midasm_hook]]`s in `ghwor_functions.toml`). The hooks write the same fence
into the same memory through the host view of physical memory that the watches do not protect (the one the SDK's own
GPU-side writes use), and skip the game's store. Direct3D reads the fence back through its view as before; no GPU work
reads a resource's header (a check over a whole song: none of 42 million vertex and index ranges the draws read held
a word stamped in the last 20 ms); a real write to the page still faults and uploads it. In Gustavo's Quest song at
240 FPS, 3440x1440, the venue every frame: copies into NativeXRenderer's memory 644 -> 231 a frame, its thread 3.65 ->
3.00 ms a frame, 231 -> 239 FPS on average, the slowest stretch 198 -> 225. With V-Sync off and the frame rate at 480
(measurement only): 260 -> 320 FPS. Test-Games OK on both renderers.

**Turning it off.** `ghwor_fence_stamps_unwatched = false` in `ghwor.toml`: the game's own stores.
