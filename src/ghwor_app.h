// ghwor - the app: the SDK's virtual hooks (rex::ReXApp).

#pragma once

#include <rex/rex_app.h>

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/kernel/xam/module.h>
#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xthread.h>
#include <rex/ui/imgui_dialog.h>

#include "keyboard.h"
#include "options.h"
#include "project.h"
#include "startup.h"
#include "widescreen.h"

class GhworApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<GhworApp>(new GhworApp(ctx, "ghwor",
        PPCImageConfig));
  }

  // Other hooks the SDK offers (rex/rex_app.h): OnPostInitLogging, OnLoadXexImage, OnPostLoadXexImage,
  // OnCreateDialogs, OnShutdown.
  // Use the "xenos" GPU emulation (rexgpu-xenos.dll) unless another one was given.
  void OnPreSetup(rex::RuntimeConfig& config) override {
    if (config.gpu_plugin.empty()) {
      config.gpu_plugin = "xenos";
    }
  }

  // Game files: without --game_data_root, assets/ next to GHWoRRecomp.exe or the repository's assets/. User data
  // (shader cache, downloadable content; the saves and achievements before the profiles): userdata/ next to the
  // executable; the saves, settings and achievements in the launchers' current profile (src/startup.cpp).
  void OnConfigurePaths(rex::PathConfig& paths) override {
    if (paths.game_data_root.empty()) {
      paths.game_data_root = ghwor::DefaultGameRoot();
    }
    ghwor::ConfigureUserData(paths.user_data_root, paths.cache_root);
    ghwor::ConfigureProfile(paths.user_data_root);
  }

  // ghwor.toml with the built-in defaults if it is missing, then (after the SDK loads it) the game files check
  // and GH Warriors of Rock Launcher's options (separate executable, launcher/): ghwor_options.toml, before the window and the GPU
  // are created. The game shows no window of its own. No game files check when the launcher asks for the
  // achievements' list (--achievements_cache_only: no window, a question would wait unseen).
  bool SetupEnvironment() override {
    auto exe_dir = rex::filesystem::GetExecutableFolder();
    ghwor::EnsureDefaultConfig(exe_dir / GAME_CONFIG);
    if (!rex::ReXApp::SetupEnvironment()) return false;
    REXLOG_INFO("[startup] User data: {}", user_data_root().string());
    if (!rex::cvar::Query<bool>("achievements_cache_only") && !ghwor::CheckGameFiles(game_data_root())) return false;
    ghwor::LoadLauncherOptions(exe_dir / GAME_OPTIONS, ghwor::GpuPluginPath());
    return true;
  }

  // Window title (the SDK sets "<app name> <SDK build stamp>"), then ultrawide (src/widescreen.cpp): the cameras'
  // aspect ratio follows the window. NativeXRenderer is initialized here too, before the guest starts, once the game
  // has its side (native_x_renderer/docs/integrating-a-game.md, step 2):
  //   native_x_renderer::Initialize(ghwor::NativeXRendererProfile(),
  //                                 runtime() ? runtime()->graphics_system() : nullptr, window(),
  //                                 runtime() ? runtime()->cache_root() : std::filesystem::path());
  void OnPostSetup() override {
    if (window()) {
      window()->SetTitle(GAME_TITLE);
    }
    if (window()) {
      ghwor::SetOutputSize(window()->GetActualPhysicalWidth(), window()->GetActualPhysicalHeight());
    }
    ghwor::InitializeKeyboard();
  }
  void OnWindowPixelSizeChanged(uint32_t pixel_width, uint32_t pixel_height) override {
    ghwor::SetOutputSize(pixel_width, pixel_height);
  }

  // Ultrawide's black bars outside a song (src/widescreen.cpp): a dialog of the SDK's ImGui layer, gone before it.
  void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override { menu_bars_ = ghwor::CreateMenuBars(drawer); }
  void OnShutdown() override { menu_bars_.reset(); }

  // On exit, log whether the game asked to launch another executable.
  void OnGuestThreadExit(rex::system::XThread* thread) override {
    (void)thread;
    auto* rt = runtime();
    if (!rt || !rt->kernel_state()) return;
    auto xam = rt->kernel_state()->GetKernelModule<rex::kernel::xam::XamModule>("xam.xex");
    if (!xam) return;
    auto& ld = xam->loader_data();
    REXLOG_INFO("[ghwor] Title exited. launch_path='{}' flags=0x{:X} launch_data={} bytes",
                ld.launch_path, ld.launch_flags, ld.launch_data.size());
  }

 private:
  std::unique_ptr<rex::ui::ImGuiDialog> menu_bars_;
};
