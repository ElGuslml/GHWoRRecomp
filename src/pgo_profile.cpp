// The instrumented build's profile (GHWOR_PGO=generate in CMakeLists.txt; never shipped). Clang's profile runtime
// writes the counters when the process ends normally, and the game is usually closed or stopped otherwise: a thread
// writes them every 10 s instead, each time the whole run so far, to GHWOR_PGO_FILE (or ghwor.profraw next to the
// executable). The last write before the game ends is the profile; llvm-profdata merges the runs.
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <thread>

#include <windows.h>

extern "C" {
int __llvm_profile_write_file(void);
void __llvm_profile_set_filename(const char* name);
}

namespace {

struct ProfileWriter {
  ProfileWriter() {
    static std::string path;
    if (const char* file = std::getenv("GHWOR_PGO_FILE"); file && *file) {
      path = file;
    } else {
      wchar_t exe[MAX_PATH];
      GetModuleFileNameW(nullptr, exe, MAX_PATH);
      path = (std::filesystem::path(exe).parent_path() / "ghwor.profraw").string();
    }
    __llvm_profile_set_filename(path.c_str());
    std::thread([] {
      for (;;) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        __llvm_profile_write_file();
      }
    }).detach();
  }
} writer;

}  // namespace
