// The x86-64-v3 executable (CMakeLists.txt's GHWOR_CPU_LEVEL, local_docs/77) on a processor without what it is built
// for: a message instead of a crash at the first AVX2 instruction. The launcher starts this executable only where the
// processor has all of it; this is for one started by hand. Compiled for x86-64-v2 (CMakeLists.txt), and run before
// the C++ initializers of the rest of the executable (the compiler's segment, .CRT$XCC), which are built for v3.
#include <intrin.h>
#include <windows.h>

#include "project.h"

#pragma warning(disable : 4073)
#pragma init_seg(compiler)

namespace {

bool HasX86_64V3() {
  int r[4];
  __cpuid(r, 0);
  if (r[0] < 7) return false;
  __cpuid(r, 1);
  const unsigned ecx1 = unsigned(r[2]);
  // FMA, MOVBE, OSXSAVE, AVX, F16C.
  const unsigned need1 = (1u << 12) | (1u << 22) | (1u << 27) | (1u << 28) | (1u << 29);
  if ((ecx1 & need1) != need1) return false;
  // The system saves the YMM registers.
  if ((_xgetbv(0) & 6) != 6) return false;
  __cpuidex(r, 7, 0);
  // BMI1, AVX2, BMI2.
  const unsigned need7 = (1u << 3) | (1u << 5) | (1u << 8);
  if ((unsigned(r[1]) & need7) != need7) return false;
  __cpuid(r, 0x80000001);
  // LZCNT.
  return (unsigned(r[2]) & (1u << 5)) != 0;
}

struct CpuCheck {
  CpuCheck() {
    if (HasX86_64V3()) return;
    MessageBoxW(nullptr,
                L"This copy of the game is built for processors with AVX2 (Intel Haswell, AMD Zen and later), and "
                L"this one does not have it.\n\nStart the game from its launcher, which chooses the copy that runs "
                L"here, or start " GAME_EXE L".",
                L"" GAME_TITLE, MB_OK | MB_ICONERROR);
    ExitProcess(1);
  }
} cpu_check;

}  // namespace
