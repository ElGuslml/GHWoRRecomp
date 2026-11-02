# NativeXRenderer in Guitar Hero: Warriors of Rock

The game's side of NativeXRenderer (`native_x_renderer/`, the library that hooks a game's Direct3D calls and draws
on the host). The hooks and the profile are in [`src/native_x_renderer_ghwor.cpp`](../src/native_x_renderer_ghwor.cpp);
the library's checklist is `native_x_renderer/docs/integrating-a-game.md`.

**Status (2026-11-02):** GH5's side, mapped to this game's library. Not the default renderer:
`--native_x_renderer=true` or the launcher's NativeXRenderer card.

## The addresses

- The game links the XDK Direct3D library of 2.0.11164 (GH5 2.0.8276) at `0x822D02A0`–`0x822EFCD0`. Each of GH5's
  hooked functions was found by the function matcher over both games' instruction indexes and compared instruction by
  instruction (`tools/port/nxrcmp.py`, gitignored): every one has GH5's instruction count except Resolve (961 → 1030),
  Present (396 → 410) and SetTexture (95 → 99), and they differ only in device offsets.
- **The device layout** is GH5's up to the vertex declaration: the six dirty masks, the command buffer's write pointer,
  the fetch, VS, PS and bool/loop constants, the register shadow, the fence and write-back block and the flags, each
  used as often in both libraries. From the vertex declaration on every field is 168 bytes later (SetVertexDeclaration's
  store, SetIndices', SetRenderTarget's targets and depth, SetStreamSource's `addi 3159` = +12636, SetTexture's `addi
  3180` = +12720, the viewport's floats, the shaders, the predication mask, BeginTiling's tile count); the draw state at
  the end 176 to 212 (the inline draw end `[dev+13652]`, GH5's 13444). The resource objects are GH5's.
- The device pointer is at `0x829BBA74`: the renderer's block `0x829BB350` + 1828 (GH5's + 2032).
- GH5's DrawIndexedVerticesUP and BeginIndexedVertices are not in this executable. The engine's three BeginVertices /
  EndVertices pairs (`sub_822C3F78`, `sub_822C53A0`, `sub_822C9948`) call EndVertices (`sub_822D1890`, `[dev+48] =
  [dev+13652]`): `inline_draw_end` covers them and EndVertices is not hooked, as in GH5.
- **`scene_resolve_caller = 0`: no antialiasing yet**, as GH5. A traced song frame has the scene's 1280x720 copy after
  its 513 draws from `0x8255AD64 < 0x826493C8`, the render targets' helper every resolve goes through
  (`sub_8255AC80`): to be told apart in the menus too before it is set.

## Checked (2026-11-02, Game.ps1, keyboard)

- The observer (`--native_x_renderer_verify`, F10): the main menu, 522 of 522 draw calls match the rebuilt state; a
  Quickplay+ song over three frames, 602 of 602 of the draws the GPU executed in them. The unmatched packets are the
  library's own (Clear, Resolve). In the song most of the CPU's draw calls had no execution inside the observed frames
  (933 of 1535): their packets ran after the window (the scene is drawn into another part of the command buffer); none
  of the executed ones differed.
- `--native_x_renderer=true`, 1280x720: the main menu, the band lobby, a song (the venue, the band solid, the highway,
  the HUD, the red border, the depth of field); 0 failed draws, 0 placeholder textures; 16.67 ms frames, the renderer
  about 6 ms of CPU a frame; the process 1.84 cores in the main menu and 2.24 in a song (the Xenos path 1.46 and 2.03).
- 2560x1080 with `ghwor_ultrawide`: rendered at 2560x1080 (`render scale 2.0000 x 1.5000`), the 3D views widened, the
  2D layer at 16:9; in a song the stage over the whole width with the highway, the HUD and the red border in place.
  Outside a song (the menus, the fail screen and the band's fail scene before it) black bars cover the sides, as on the
  Xenos path.

## Not done yet

- **A resolve into a 3D texture** (`resolve to a texture of dimension 2: not supported yet`): once in three runs, on
  entering the band lobby the first time, from a 32x32 surface (a 32x32x32 volume, probably a color lookup table).
  NativeXRenderer skips it; nothing different was seen on screen. Not reproduced since.
- Frame rates above 60 and larger outputs (3440x1440, 160 FPS) with NativeXRenderer, a real play test.
