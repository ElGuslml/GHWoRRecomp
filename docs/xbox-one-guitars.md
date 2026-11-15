# Xbox One guitars (PDP Riffmaster)

**Symptom.** An Xbox One or Xbox Series guitar, such as the PDP Riffmaster for Xbox, did nothing in the game or in
the launcher, in every recomp. The game's log listed no controller (no `SDL JoystickAdded` line), and its slot 0 was
`(None)`: the empty controller the SDK gives a game so that it does not wait for one.

**Cause.** These guitars (the PDP Riffmaster and Jaguar, the Mad Catz Stratocaster, the CRKD / RedOctane Stage Tour
guitar, and the drum kits) speak the Xbox One protocol (GIP) through their dongle. Windows makes only Xbox One
gamepads XInput controllers; a guitar is neither XInput nor HID. Microsoft GameInput is the one way to read it. SDL,
which reads the controllers for the SDK and the launcher, reads these guitars through GameInput's raw reports only when
it is built with GameInput's API 3, and the SDK built it with the Windows SDK's header, API 0.

**Fix** (SDK change 110, RecompLauncher 3.23).
- SDL is built with GameInput's API 3 header (Microsoft.GameInput 3.5.283, MIT; the SDK's `thirdparty/gameinput`).
- The guitar plays as an Xbox 360 guitar. Its frets are A, B, Y, X and LB, strum is the D-pad, the whammy bar is the
  right stick's X, and Star Power is Back.
- Its tilt is the right stick's Y, as on an Xbox 360 guitar. SDL gives the tilt only as the RB button, so the SDK
  reads the tilt itself from the guitar's report. It goes from 0x00 (level) to 0xD0 (tilted). RB, the pickup selector
  and the lower frets' trigger are left out.
- The launcher's Controls page reads the tilt the same way: its meter, and Assign, which finds it as `gip`.
- GameInput is a part of Windows that Microsoft ships separately ("Microsoft GameInput"). The game needs version 3 or
  newer of it. A release includes its installer, `GameInputRedist.msi`. The setup offers it on its last page when the
  PC lacks it. The launcher's Controls page lists "Xbox One guitars · GameInput missing · Install" while it is
  missing. Windows asks for an administrator's permission to install it.

**Every other controller once.** With GameInput's raw reports on, SDL also lists a plain controller through
GameInput, so the same controller came twice, in two slots (a virtual Xbox 360 controller in slots 0 and 1). The SDK
and the launcher leave out a controller read through GameInput unless SDL types it a guitar or a drum kit; the game's
log says "through GameInput left alone".

The game's log says at the start whether GameInput 3 answers (`GameInput 3: on` / `not available`). When such a
guitar connects, the log names it, and it notes the tilt's values as they widen.

**Turning it off:** `gamepad_gameinput = false` in `ghwor.toml`; those guitars then go unread. Another range for the
tilt: `gamepad_tilt_calibration`, with `gip` as the source (the launcher's Assign writes it).

**Status:** built and checked without such a guitar. GameInput 3 answers in the game and the launcher on a PC with the
redistributable, and the launcher's row and question were seen with GameInput hidden. The guitar itself waits for a
player's test.
