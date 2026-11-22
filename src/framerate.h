// ghwor - frame rates above 60, and the count of a song's frames by the vertical blanks each took (src/framerate.cpp).
#pragma once

namespace ghwor {

// A song's music stopped (src/song_sync.cpp's hooks of SAP_Stop, SAP_Unload and SAP_DeInit): its frames by the blanks
// they took, in the log.
void FrameBlanksSongEnded();

}  // namespace ghwor
