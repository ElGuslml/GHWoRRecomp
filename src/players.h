// ghwor - the profile follows the controller that starts the game (src/players.cpp).

#pragma once

namespace ghwor {

// Each frame (src/quit_prompt.cpp's hook), whether a dialog box is over the screen: from the start of the game until
// someone presses a button, the first controller to press becomes the profile's.
void UpdatePressWindow(bool dialog);

}  // namespace ghwor
