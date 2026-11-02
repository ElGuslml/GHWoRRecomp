// ghwor - NativeXRenderer for Guitar Hero: Warriors of Rock (see native_x_renderer_ghwor.cpp).

#pragma once

#include <native_x_renderer/native_x_renderer.h>

namespace ghwor {

// Warriors of Rock's D3D library: the XDK 2.0.11164 device and object layouts.
const native_x_renderer::Profile& NativeXRendererProfile();

}  // namespace ghwor
