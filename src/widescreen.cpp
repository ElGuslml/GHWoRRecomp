// ghwor - ultrawide displays (see widescreen.h).

#include "widescreen.h"

#include <imgui.h>
#include <native_x_renderer/native_x_renderer.h>
#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>
#include <rex/ui/imgui_dialog.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>

REXCVAR_DEFINE_BOOL(ghwor_menu_bars, true, "GHWoR",
                    "With ultrawide: black bars at the sides outside a song (the menus, the loading screens and the "
                    "videos were made for 16:9); in a song the stage fills the screen");
REXCVAR_DEFINE_BOOL(ghwor_ultrawide, false, "GHWoR",
                    "Ultrawide on the Xenos path: the game's 3D views see as much more at the sides as the window is "
                    "wider than 16:9 (anamorphic, the presenter stretches the 1280x720 image; needs "
                    "present_letterbox = false). With NativeXRenderer, native_x_renderer_fill_screen does this instead");
REXCVAR_DEFINE_BOOL(ghwor_ultrawide_hud, true, "GHWoR",
                    "With ghwor_ultrawide: keep the HUD, the menus and the videos at 16:9 in the center "
                    "(false = stretched)");
REXCVAR_DEFINE_BOOL(ghwor_ultrawide_overlays, true, "GHWoR",
                    "With ultrawide, in a song: a 2D overlay over the whole screen (the red border while the rock meter "
                    "is red) covers the whole width; false = kept at 16:9 in the center with the HUD");

namespace ghwor {
namespace {

constexpr double kGuestAspect = 16.0 / 9.0;

// Factor the 16:9 views are widened by (1 = unchanged).
std::atomic<double> g_aspect_factor{1.0};

uint8_t* GuestBase() {
  auto* kernel = rex::system::kernel_state();
  return kernel && kernel->memory() ? kernel->memory()->virtual_membase() : nullptr;
}

// When the note highway's board and a picture element (GHWoRElementQuadHook) were last drawn (steady clock ticks,
// 0 if never). The board is drawn in every frame of a song from its first frame (the band's intro included) and while
// it is paused, and never in the menus (GH Metallica's run 264). The song's first frames run under the end of its
// loading screen (run 266), a picture element, which a song and its pause menu never draw (run 269).
std::atomic<int64_t> g_highway_drawn{0};
std::atomic<int64_t> g_picture_drawn{0};

bool DrawnRecently(const std::atomic<int64_t>& mark) {
  int64_t last = mark.load(std::memory_order_relaxed);
  auto now = std::chrono::steady_clock::now().time_since_epoch();
  return last != 0 && now.count() - last <
                          std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::milliseconds(250)).count();
}

bool InSong() { return DrawnRecently(g_highway_drawn) && !DrawnRecently(g_picture_drawn); }

// Black over the sides of the image, where the 2D layer is not (the 16:9 area in the middle, as wide as the image over
// the factor), while no song is drawn. The background draw list: under any dialog (the achievements' toasts).
class MenuBarsDialog : public rex::ui::ImGuiDialog {
 public:
  explicit MenuBarsDialog(rex::ui::ImGuiDrawer* drawer) : ImGuiDialog(drawer) {}
  bool WantsContinuousPaint() override { return false; }

 protected:
  void OnDraw(ImGuiIO& io) override {
    double factor = g_aspect_factor.load(std::memory_order_relaxed);
    if (factor <= 1.0 || !REXCVAR_GET(ghwor_menu_bars) || InSong()) {
      return;
    }
    // The image as the presentation places it: as wide as the factor says, as large as fits the window (NativeXRenderer
    // may draw another aspect ratio than the window's, native_x_renderer_aspect, with bars of its own); the bars cover
    // everything beside its 16:9 middle.
    float width = io.DisplaySize.x, height = io.DisplaySize.y;
    double image_height = std::min(double(height), double(width) / (kGuestAspect * factor));
    float bar = std::floor(float((double(width) - image_height * kGuestAspect) * 0.5));
    if (bar <= 0.0f) {
      return;
    }
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    draw->AddRectFilled(ImVec2(0.0f, 0.0f), ImVec2(bar, height), IM_COL32(0, 0, 0, 255));
    draw->AddRectFilled(ImVec2(width - bar, 0.0f), ImVec2(width, height), IM_COL32(0, 0, 0, 255));
  }
};

}  // namespace

bool SongShown() { return DrawnRecently(g_highway_drawn); }

void SetOutputSize(uint32_t width, uint32_t height) {
  double factor = 1.0;
  if (native_x_renderer::Renders()) {
    // NativeXRenderer renders at the output's aspect ratio when it fills it (the profile says these hooks widen the
    // cameras): the hooks follow its factor, whatever ghwor_ultrawide says. They handle wider outputs only.
    double native = native_x_renderer::AspectFactor();
    if (native > 1.0) {
      factor = native;
    } else if (native < 1.0) {
      static std::atomic<bool> warned{false};
      if (!warned.exchange(true)) {
        REXLOG_WARN("[ultrawide] NativeXRenderer fills an output taller than 16:9 ({:.4f}); the game's hooks only "
                    "widen views, so the image is stretched vertically (native_x_renderer_fill_screen = false "
                    "keeps 16:9 with bars)",
                    native);
      }
    }
  } else if (REXCVAR_GET(ghwor_ultrawide) && width > 0 && height > 0) {
    double aspect = double(width) / double(height);
    if (aspect > kGuestAspect) {
      factor = aspect / kGuestAspect;
    }
  }
  if (g_aspect_factor.exchange(factor) != factor) {
    REXLOG_INFO("[ultrawide] output {}x{}: aspect factor {:.4f}", width, height, factor);
  }
}

std::unique_ptr<rex::ui::ImGuiDialog> CreateMenuBars(rex::ui::ImGuiDrawer* drawer) {
  return drawer ? std::make_unique<MenuBarsDialog>(drawer) : nullptr;
}

}  // namespace ghwor

namespace {

float LoadFloat(const uint8_t* base, uint32_t address) {
  uint32_t bits;
  std::memcpy(&bits, base + address, 4);
  bits = __builtin_bswap32(bits);
  float value;
  std::memcpy(&value, &bits, 4);
  return value;
}

void StoreFloat(uint8_t* base, uint32_t address, float value) {
  uint32_t bits;
  std::memcpy(&bits, &value, 4);
  bits = __builtin_bswap32(bits);
  std::memcpy(base + address, &bits, 4);
}

uint32_t LoadWord(const uint8_t* base, uint32_t address) {
  uint32_t value;
  std::memcpy(&value, base + address, 4);
  return __builtin_bswap32(value);
}

void StoreWord(uint8_t* base, uint32_t address, uint32_t value) {
  value = __builtin_bswap32(value);
  std::memcpy(base + address, &value, 4);
}

}  // namespace

namespace {

// The D3D device ([0x829BBA74], the renderer block 0x829BB350 + 1828; GH5's 0x82D989C0 + 2032) and the fields the
// hooks read: render target 0 (+12616), a surface's size (+36: width - 1 in bits 18-31, height - 1 in bits 3-17), and
// what BeginVertices saved for the inline draw that EndVertices commits: its vertex data (+13656) and its size in
// dwords (+13664). This game's D3D library (XDK 2.0.11164) has another device layout than GH5's (2.0.8276: +12448,
// +13448, +13456); the VS constants and the surface's size did not move.
constexpr uint32_t kDevicePointer = 0x829BBA74;
constexpr uint32_t kRenderTarget0 = 12616;
constexpr uint32_t kVsConstants = 1920;  // VS float constants, c0 first
constexpr uint32_t kSurfaceSize = 36;
constexpr uint32_t kInlineVertexData = 13656;
constexpr uint32_t kInlineVertexDwords = 13664;
// The register the tails' vertex shader gave m_HeightConstants (a byte, see GHWoRTailHook).
constexpr uint32_t kTailHeightRegister = 0x829079CC;

// The factor the 2D layer is narrowed by, or 1 when it is not: ultrawide on, its HUD option on, and the draw going to
// the screen (render target 0 of 1280x720), not to a texture.
double NarrowFactor(const uint8_t* base) {
  double factor = ghwor::g_aspect_factor.load(std::memory_order_relaxed);
  if (factor == 1.0 || !REXCVAR_GET(ghwor_ultrawide_hud) || !base) {
    return 1.0;
  }
  uint32_t device = LoadWord(base, kDevicePointer);
  uint32_t surface = device ? LoadWord(base, device + kRenderTarget0) : 0;
  if (!surface) {
    return 1.0;
  }
  uint32_t size = LoadWord(base, surface + kSurfaceSize);
  uint32_t width = ((size >> 18) & 0x3FFF) + 1, height = ((size >> 3) & 0x7FFF) + 1;
  return width == 1280 && height == 720 ? factor : 1.0;
}

// Whether the vertices (x and y the first two floats of each, pixels of the 1280x720 canvas) are a piece of an
// overlay of the whole screen: at least a quarter of the canvas (half its width and half its height) reaching one of
// its sides and its top or bottom. GH5 draws the red border of a rock meter in the red as four such quarters, from
// each corner to the middle, or as one quad over the whole canvas, through the same sprite code (not yet seen here).
bool ScreenOverlayPiece(const uint8_t* base, uint32_t data, uint32_t count, uint32_t stride) {
  float x0 = 1e9f, x1 = -1e9f, y0 = 1e9f, y1 = -1e9f;
  for (uint32_t i = 0; i < count; ++i) {
    const float x = LoadFloat(base, data + i * stride), y = LoadFloat(base, data + i * stride + 4);
    x0 = std::min(x0, x);
    x1 = std::max(x1, x);
    y0 = std::min(y0, y);
    y1 = std::max(y1, y);
  }
  return (x0 <= 0.5f || x1 >= 1279.5f) && (y0 <= 0.5f || y1 >= 719.5f) && x1 - x0 >= 639.5f && y1 - y0 >= 359.5f;
}

// The vertices' x (the first float of each, in pixels of the 1280x720 canvas) narrowed around the center.
void NarrowVertices(uint8_t* base, double factor, uint32_t data, uint32_t count, uint32_t stride) {
  static std::atomic<bool> logged{false};
  if (!logged.exchange(true)) {
    REXLOG_INFO("[ultrawide] 2D layer kept at 16:9 in the center ({:.4f})", factor);
  }
  for (uint32_t i = 0; i < count; ++i) {
    uint32_t x = data + i * stride;
    StoreFloat(base, x, float(640.0 + (LoadFloat(base, x) - 640.0) / factor));
  }
}

}  // namespace

// The screen elements (the menus, the HUD, the note highway and its notes, the text) are drawn in pixels of a 1280x720
// canvas. Their vertices are narrowed around the center just before they are drawn, so the whole 2D layer stays 16:9 in
// the middle of the stretched image, whatever its positions came from: the note highway's gems and its dark backdrop
// are placed by the highway's own code, not through root_window, and narrowing root_window left them stretched.
//
// [[midasm_hook]] at 0x824D9498 in sub_824D9230, a sprite's quad: r6 = its 4 vertices (0x82A71F38), r7 = their stride
// (28 or 36), before DrawVerticesUP. In a song a sprite that is a piece of an overlay of the whole screen (GH5's red
// border while the rock meter is red: quarters of the canvas from each corner to the middle) is left as it is, so the
// overlay is stretched over the whole width instead of a red frame around the middle 16:9 (ghwor_ultrawide_overlays).
void GHWoRSpriteHook(PPCRegister& r6, PPCRegister& r7) {
  uint8_t* base = ghwor::GuestBase();
  double factor = NarrowFactor(base);
  if (factor == 1.0) {
    return;
  }
  if (REXCVAR_GET(ghwor_ultrawide_overlays) && ghwor::InSong() && ScreenOverlayPiece(base, r6.u32, 4, r7.u32)) {
    static std::atomic<bool> logged{false};
    if (!logged.exchange(true)) {
      REXLOG_INFO("[ultrawide] a song's overlays over the whole screen cover the whole width");
    }
    return;
  }
  NarrowVertices(base, factor, r6.u32, 4, r7.u32);
}

// [[midasm_hook]] at 0x82625AD0 in sub_826255D8, another screen element's quad (drawn by sub_82625BB8 for the loading
// screen's layers, sub_825F9F70, and for some menus' pictures, the render pass sub_82554E50): r6 = its 4 vertices (28
// bytes each, x first), r7 = their stride, before DrawVerticesUP. Without it GH5's loading screen was stretched over
// the whole screen. While such pictures are drawn a song does not count as one for the bars (InSong): its loading
// screen gets them.
void GHWoRElementQuadHook(PPCRegister& r6, PPCRegister& r7) {
  ghwor::g_picture_drawn.store(std::chrono::steady_clock::now().time_since_epoch().count(),
                                     std::memory_order_relaxed);
  uint8_t* base = ghwor::GuestBase();
  double factor = NarrowFactor(base);
  if (factor != 1.0) {
    NarrowVertices(base, factor, r6.u32, 4, r7.u32);
  }
}

// [[midasm_hook]] at 0x822C8974 in sub_822C8810, a 2D particle (the sparks of a hit note at the strike line): r5 = its
// 4 vertices (a quad list), r6 = their data (20 bytes each, x first, pixels of the 1280x720 canvas), r7 = the stride,
// before DrawVerticesUP (which this game skips when there are none). Drawn only when notes are hit; left as they were,
// GH5's sat outside their lanes (the green one's to the left).
void GHWoRParticleHook(PPCRegister& r5, PPCRegister& r6, PPCRegister& r7) {
  uint8_t* base = ghwor::GuestBase();
  double factor = NarrowFactor(base);
  if (factor != 1.0) {
    NarrowVertices(base, factor, r6.u32, r5.u32, r7.u32);
  }
}

// [[midasm_hook]] at 0x8265B65C in sub_8265B540, a batch of quads (the text): r5 = the vertex count ([0x82B545C4]),
// r6 = the vertices (0x82B4B5C0), r7 = their stride (36 or 28), before DrawVerticesUP.
void GHWoRSpriteBatchHook(PPCRegister& r5, PPCRegister& r6, PPCRegister& r7) {
  uint8_t* base = ghwor::GuestBase();
  double factor = NarrowFactor(base);
  if (factor != 1.0) {
    NarrowVertices(base, factor, r6.u32, r5.u32, r7.u32);
  }
}

// [[midasm_hook]] at 0x824D96D4 in sub_824D94A8, a screen element's clip window (used by the text and the sprites of
// elements that scroll or roll, as GH5's star rating's number in the score box): r4 = its rectangle {x1, y1, x2, y2}
// (ints, pixels of the 1280x720 target), before SetScissorRect. The vertices it clips are narrowed around the center,
// so the rectangle is narrowed as well; left as it was, it cut away what moved out of it (a small window, as the
// star's number, showed nothing). Unlike GH5's, this game's window is first moved by the 2D layer's zoom (the scale at
// 0x82864ED8 and the offset at 0x82A71BAC, as the sprites' positions), so it arrives in the pixels its vertices end in.
void GHWoRClipWindowHook(PPCRegister& r4) {
  uint8_t* base = ghwor::GuestBase();
  double factor = NarrowFactor(base);
  if (factor == 1.0) {
    return;
  }
  // The engine clamps the window to the target: one wholly off the screen arrives empty (1279 to 1279) and stays so.
  int32_t x1 = int32_t(LoadWord(base, r4.u32)), x2 = int32_t(LoadWord(base, r4.u32 + 8));
  int32_t n1 = int32_t(std::floor(640.0 + (x1 - 640.0) / factor));
  int32_t n2 = x2 > x1 ? int32_t(std::ceil(640.0 + (x2 - 640.0) / factor)) : n1;
  StoreWord(base, r4.u32, uint32_t(n1));
  StoreWord(base, r4.u32 + 8, uint32_t(n2));
}

// [[midasm_hook]] at 0x822C6704 in sub_822C5D20, a sustained note's tail (a triangle strip, before DrawVerticesUP: r5 =
// the vertex count, r6 = the vertices at 0x82901990, 24 bytes each, built by sub_822C4558). Its vertices are in the
// lane's own space (x: half the tail's width on each side of 0; y: along the highway), and its vertex shader places
// them on the canvas with the VS constant m_HeightConstants, which sub_822C5D20 writes for each tail just before:
// x = c.x (the lane's x at the far end) + x + c.w (the lane's x from there to the strike line) * c.z * (y - c.y).
// Narrowing that x around the center: the vertices' x divided by the factor, c.x moved toward 640, c.w divided. GH5's
// hook took c0; here the register is the game's own, the byte at 0x829079CC, which sub_822C5D20 looks up in its vertex
// shader (61; GH5's 74, in the game data) on its first tail, before it writes the constant.
void GHWoRTailHook(PPCRegister& r5, PPCRegister& r6, PPCRegister& r7) {
  uint8_t* base = ghwor::GuestBase();
  double factor = NarrowFactor(base);
  if (factor == 1.0) {
    return;
  }
  for (uint32_t i = 0; i < r5.u32; ++i) {
    uint32_t x = r6.u32 + i * r7.u32;
    StoreFloat(base, x, float(LoadFloat(base, x) / factor));
  }
  uint32_t reg = base[kTailHeightRegister];
  static std::atomic<bool> logged{false};
  if (!logged.exchange(true)) {
    REXLOG_INFO("[ultrawide] sustain tails: m_HeightConstants in VS c{}", reg);
  }
  uint32_t c = LoadWord(base, kDevicePointer) + kVsConstants + 16 * reg;
  StoreFloat(base, c, float(640.0 + (LoadFloat(base, c) - 640.0) / factor));
  StoreFloat(base, c + 12, float(LoadFloat(base, c + 12) / factor));
}

// [[midasm_hook]] at 0x822C4540 in sub_822C3F78, a sprite drawn as a grid of quads (the highway's board): r3 = the
// device, before EndVertices; the vertices BeginVertices handed out are 20 bytes each (x, y, color, u, v).
void GHWoRMeshSpriteHook(PPCRegister& r3) {
  ghwor::g_highway_drawn.store(std::chrono::steady_clock::now().time_since_epoch().count(),
                                     std::memory_order_relaxed);
  uint8_t* base = ghwor::GuestBase();
  double factor = NarrowFactor(base);
  if (factor != 1.0) {
    uint32_t data = LoadWord(base, r3.u32 + kInlineVertexData);
    uint32_t dwords = LoadWord(base, r3.u32 + kInlineVertexDwords);
    NarrowVertices(base, factor, data, dwords / 5, 20);
  }
}

// [[midasm_hook]] at 0x822C588C in sub_822C53A0, the highway's depth mask (GH5's sub_822FFA78): first thing in the
// scene, one quad in the highway's shape at the near plane, depth only, so the stage is not drawn under the board. Its
// vertices are x and y in pixels of the 1280x720 canvas (8 bytes each), written before EndVertices (r3 = the device).
// Left as it was it kept the width of the stretched canvas while the board drawn over it is narrowed: the stage stayed
// black on both sides of the highway, blurred with it by the screen effects.
void GHWoRHighwayMaskHook(PPCRegister& r3) {
  uint8_t* base = ghwor::GuestBase();
  double factor = NarrowFactor(base);
  if (factor != 1.0) {
    uint32_t data = LoadWord(base, r3.u32 + kInlineVertexData);
    uint32_t dwords = LoadWord(base, r3.u32 + kInlineVertexDwords);
    NarrowVertices(base, factor, data, dwords / 2, 8);
  }
}

// [[midasm_hook]] at the entry of sub_824C6EE0, which builds a perspective projection from its frustum at the near
// plane (f1, f2 = left, right; f3, f4 = bottom, top; the row-vector convention). Its callers (the views of
// sub_8253E778 and sub_8255CC88, among others) take the frustum's width from the camera's horizontal field of view and
// its height from the view's aspect ratio, its size (1280x720 for the screen's views): a wider screen would just
// stretch it. A 16:9 frustum is widened by the factor (Hor+: the vertical field of view kept, more seen at the sides),
// the same narrowing the 2D layer gets in the stretched image. Other shapes (the crowd's impostors, 1:2) are left
// alone.
void GHWoRFrustumHook(PPCRegister& f1, PPCRegister& f2, PPCRegister& f3, PPCRegister& f4) {
  double factor = ghwor::g_aspect_factor.load(std::memory_order_relaxed);
  double height = std::fabs(f4.f64 - f3.f64);
  if (factor == 1.0 || height == 0.0) {
    return;
  }
  double aspect = std::fabs(f2.f64 - f1.f64) / height;
  if (aspect < 1.70 || aspect > 1.86) {
    return;
  }
  static std::atomic<bool> logged{false};
  if (!logged.exchange(true)) {
    REXLOG_INFO("[ultrawide] 3D views widened ({:.4f})", factor);
  }
  f1.f64 *= factor;
  f2.f64 *= factor;
}
