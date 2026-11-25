# GHWoRRecomp - static recompilation of Guitar Hero: Warriors of Rock (Xbox 360), built on the ReXGlue SDK, drawn by the SDK's Xenos
# emulation or by NativeXRenderer (native_x_renderer/, a submodule).
#
# The SDK is the thirdparty/rexglue-sdk git submodule: ElGuslml/rexglue-sdk, branch recomp, the official
# nightly of 2026-10-02 (v0.10.0 and 24 later commits) with the changes these projects and NativeXRenderer
# need, one commit each (RECOMP.md there).
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(ghwor LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Same CPU baseline as the official SDK builds (its win-amd64 preset): x86-64-v2, i.e. SSE4.2 and
# POPCNT on top of the SSSE3/SSE4.1 the SDK code requires. Not -march=native, so the binaries run on
# any x86-64-v2 CPU.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
    add_compile_options(-march=x86-64-v2)
endif()

# Build the SDK from the submodule. RECOMP.md marks the recomp branch: a checkout of the official SDK lacks
# its changes and would not build this project.
set(REXSDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/rexglue-sdk" CACHE PATH
    "Path to the rexglue-sdk source tree" FORCE)
if(NOT EXISTS "${REXSDK_DIR}/RECOMP.md")
    message(FATAL_ERROR
        "The ReXGlue SDK submodule (${REXSDK_DIR}) is missing or not on the recomp branch of "
        "ElGuslml/rexglue-sdk. Run: git submodule sync && git submodule update --init --recursive --force")
endif()

# Both graphics backends: D3D12 and Vulkan.
if(WIN32)
    set(REXGLUE_USE_D3D12 ON CACHE BOOL "Enable the D3D12 graphics backend")
endif()
set(REXGLUE_USE_VULKAN ON CACHE BOOL "Enable the Vulkan graphics backend")

include(generated/rexglue.cmake)

# The first build of a fresh tree runs the codegen, which writes generated/default/sources.cmake;
# CMake has to configure again to pick up the generated sources (otherwise the link fails with
# PPCImageConfig). A marker file holds the sources.cmake this configure saw and is a configure
# dependency; it is refreshed with copy_if_different after the codegen, so it only changes (and
# triggers a new configure) when the list of generated sources changes. Ninja brings configure
# dependencies up to date before building anything else, so the first build runs the codegen,
# configures again and goes on within the same command. (The codegen stamp itself cannot be the
# dependency: the codegen touches it on every run.)
set(GHWOR_CODEGEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated/default")
set(GHWOR_SOURCES_MARKER "${GHWOR_CODEGEN_DIR}/sources.cmake.configured")
if(EXISTS "${GHWOR_CODEGEN_DIR}/sources.cmake")
    file(COPY_FILE "${GHWOR_CODEGEN_DIR}/sources.cmake" "${GHWOR_SOURCES_MARKER}" ONLY_IF_DIFFERENT)
elseif(NOT EXISTS "${GHWOR_SOURCES_MARKER}")
    file(WRITE "${GHWOR_SOURCES_MARKER}" "")
endif()
add_custom_command(
    OUTPUT "${GHWOR_SOURCES_MARKER}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${GHWOR_CODEGEN_DIR}/sources.cmake" "${GHWOR_SOURCES_MARKER}"
    DEPENDS "${GHWOR_CODEGEN_DIR}/codegen.build.stamp"
    COMMENT "Checking the list of generated sources"
    VERBATIM)
add_custom_target(ghwor_sources_marker DEPENDS "${GHWOR_SOURCES_MARKER}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${GHWOR_SOURCES_MARKER}")

# The executable's own code (the generated code, src/, NativeXRenderer and DiligentCore) can be built for a higher
# CPU level and with a profile (the performance plan's Phase 5, local_docs/77). The SDK, included above, keeps the
# baseline, and its DLLs are shared by every build.
# - GHWOR_CPU_LEVEL: x86-64-v2 (the default, any CPU since about 2009) or x86-64-v3 (AVX2, FMA, BMI1/2, MOVBE,
#   LZCNT: Haswell, Zen 1 and later).
# - GHWOR_PGO: empty (none), "generate" (an instrumented build that writes the profile it collects; never shipped;
#   src/pgo_profile.cpp) or the path of a merged profile (.profdata) to optimize with.
set(GHWOR_CPU_LEVEL "x86-64-v2" CACHE STRING "The CPU level of the executable's own code: x86-64-v2 or x86-64-v3")
set_property(CACHE GHWOR_CPU_LEVEL PROPERTY STRINGS x86-64-v2 x86-64-v3)
# A Release build takes the game's profile when there is one (tools/pgo/ghwor.profdata, kept out of git as the pipeline
# list is: docs/development.md, "Releases"); -DGHWOR_PGO= (empty) builds without it.
set(_ghwor_profile "${CMAKE_CURRENT_SOURCE_DIR}/tools/pgo/ghwor.profdata")
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND EXISTS "${_ghwor_profile}")
    set(_ghwor_pgo_default "${_ghwor_profile}")
else()
    set(_ghwor_pgo_default "")
endif()
set(GHWOR_PGO "${_ghwor_pgo_default}" CACHE STRING "Profile-guided optimization: empty, generate, or the path of a .profdata")
# On by default: the profile comes from guitar songs, and the drums', the microphone's or the rocker creator's code it
# never ran is then optimized as without a profile, not for size (local_docs/77: the same gains either way).
set(GHWOR_PGO_COLD_AS_BASE ON CACHE BOOL
    "With a profile, code the profile never ran is optimized as without one (-pgso=false), not for size")
set(GHWOR_EXTRA_SOURCES "")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    get_directory_property(_ghwor_options COMPILE_OPTIONS)
    if(NOT GHWOR_CPU_LEVEL STREQUAL "x86-64-v2")
        # The same arithmetic as the baseline's: no a * b + c fused where the code did not ask for it (the generated
        # code's fmadd and its kin call fma, the same result either way).
        list(REMOVE_ITEM _ghwor_options -march=x86-64-v2)
        list(APPEND _ghwor_options -march=${GHWOR_CPU_LEVEL} -ffp-contract=off)
        # A message instead of a crash on a processor without it, built for the baseline (src/cpu_check.cpp).
        list(APPEND GHWOR_EXTRA_SOURCES src/cpu_check.cpp)
        set_source_files_properties(src/cpu_check.cpp PROPERTIES COMPILE_OPTIONS "-march=x86-64-v2")
    endif()
    if(GHWOR_PGO STREQUAL "generate")
        list(APPEND _ghwor_options -fprofile-instr-generate)
        add_link_options(-fprofile-instr-generate)
        list(APPEND GHWOR_EXTRA_SOURCES src/pgo_profile.cpp)
    elseif(GHWOR_PGO)
        if(NOT EXISTS "${GHWOR_PGO}")
            message(FATAL_ERROR "GHWOR_PGO: no profile at ${GHWOR_PGO}")
        endif()
        # A function the profile does not know (changed since, or never run) is only reported in the summary.
        list(APPEND _ghwor_options -fprofile-instr-use=${GHWOR_PGO} -Wno-profile-instr-unprofiled
             -Wno-profile-instr-out-of-date -Wno-profile-instr-missing)
        if(GHWOR_PGO_COLD_AS_BASE)
            list(APPEND _ghwor_options "SHELL:-mllvm -pgso=false")
        endif()
    endif()
    set_directory_properties(PROPERTIES COMPILE_OPTIONS "${_ghwor_options}")
    message(STATUS "GHWoR: the executable's code for ${GHWOR_CPU_LEVEL}, profile: ${GHWOR_PGO}")
endif()

# Sources
set(GHWOR_SOURCES
    src/camera_cuts.cpp
    src/debug_menu.cpp
    src/controller_layout.cpp
    src/drums.cpp
    src/fence_stamps.cpp
    src/framerate.cpp
    src/highway_images.cpp
    src/intro.cpp
    src/keyboard.cpp
    src/lighting_job.cpp
    src/main.cpp
    src/matchmaking.cpp
    src/mods.cpp
    src/highway_effects.cpp
    src/native_x_renderer_ghwor.cpp
    src/online_text.cpp
    src/options.cpp
    src/players.cpp
    src/quit_prompt.cpp
    src/screen_fx.cpp
    src/script_globals.cpp
    src/song_clock.cpp
    src/song_sync.cpp
    src/split_render.cpp
    src/star_power_reverb.cpp
    src/startup.cpp
    src/thread_waits.cpp
    src/timer.cpp
    src/widescreen.cpp
    ${GHWOR_EXTRA_SOURCES}
)

if(WIN32)
    add_executable(ghwor WIN32 ${GHWOR_SOURCES})
else()
    add_executable(ghwor ${GHWOR_SOURCES})
endif()

rexglue_setup_target(ghwor GPU_PLUGINS xenos)
add_dependencies(ghwor ghwor_sources_marker)

# NativeXRenderer: the native renderer (native_x_renderer/README.md). The game side (hooks, profile) is in
# src/native_x_renderer_ghwor.cpp; the game draws on the Xenos path while native_x_renderer is off.
add_subdirectory(native_x_renderer)
target_link_libraries(ghwor PRIVATE native_x_renderer)
# NVIDIA's DLSS runtime next to the executable.
native_x_renderer_copy_runtime(ghwor)

# The target and the app keep the short name "ghwor" (ghwor.toml, logs\ghwor_NNN.log); only the
# executable gets a descriptive name.
set_target_properties(ghwor PROPERTIES OUTPUT_NAME "GHWoRRecomp")

# With the SDK built from source, rexruntime does not pass the Dear ImGui include directory on to the
# host target (the installed SDK ships imgui.h in include/); rex_app.cpp and ghwor_app.h need it. The
# symbols still come from rexruntime.dll.
target_include_directories(ghwor PRIVATE "${REXSDK_DIR}/thirdparty/imgui")

# Game files: assets/ next to GHWoRRecomp.exe (release layout) or, if absent, the repository's assets/.
target_compile_definitions(ghwor PRIVATE
    GHWOR_DEFAULT_GAME_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/assets")

# GH Warriors of Rock Launcher: the options, the first run from the player's ISO, the achievements and the updates, in a separate
# executable so that GHWoRRecomp.exe starts right away (for example from Steam Big Picture); "Play" starts the game, and the
# launcher comes back when it closes. It writes ghwor_options.toml next to GHWoRRecomp.exe. The launcher's code is the
# RecompLauncher submodule (recomp_launcher/, the same in every game); the game's own names, files and options are
# launcher/game.h and launcher/game.cpp, its colors launcher/palette.h, its art launcher/art/, its resources
# res/launcher.rc. RECOMP_LAUNCHER_DIR points elsewhere to try a working copy of it.
set(RECOMP_LAUNCHER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/recomp_launcher" CACHE PATH "The RecompLauncher tree")
if(NOT EXISTS "${RECOMP_LAUNCHER_DIR}/RecompLauncher.cmake")
    message(FATAL_ERROR "The RecompLauncher submodule (${RECOMP_LAUNCHER_DIR}) is missing: git submodule update --init")
endif()
if(WIN32)
    include("${RECOMP_LAUNCHER_DIR}/RecompLauncher.cmake")
    recomp_launcher_add(ghwor_launcher
        OUTPUT_NAME "GH Warriors of Rock Launcher"
        GAME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/launcher"
        RC "${CMAKE_CURRENT_SOURCE_DIR}/res/launcher.rc"
        SDK_DIR "${REXSDK_DIR}"
        INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/res"
        DEFAULT_GAME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    # The game reads which of the launchers' profiles it runs with (src/startup.cpp, ConfigureProfile).
    recomp_launcher_add_profiles(ghwor SDK_DIR "${REXSDK_DIR}")
endif()

# Windows resources: file properties (res/version.h) and the icon, res/icon.ico, when it exists. The
# executable's first icon is also the one SDL and the launcher use for their windows.
if(WIN32)
    target_sources(ghwor PRIVATE res/game.rc)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/res/icon.ico")
        target_compile_definitions(ghwor PRIVATE RECOMP_HAS_ICON)
        target_compile_definitions(ghwor_launcher PRIVATE RECOMP_HAS_ICON)
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/res/icon.ico")
    endif()
endif()
