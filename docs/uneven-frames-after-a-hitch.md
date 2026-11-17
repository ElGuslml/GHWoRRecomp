# Uneven frame times after a hitch (NativeXRenderer D172)

**Symptom.** With V-Sync, after a while (a load, a hitch) the game stays a frame behind:
- RTSS's frame time graph turns into a sawtooth for a whole song;
- on a slower PC, constant small stutters. A player on an RTX 4060 Laptop at 120 Hz saw them after two songs, until
  he quit; the game missed about one refresh a second.

Nothing in the picture is wrong besides the stutter.

**Cause.** The game makes one frame at each vertical blank and V-Sync shows one. One long frame on the way can leave a
second frame waiting for the display. Frames are then made and shown at the same rate, so the extra one never goes
away:
- every frame waits for the display's next refresh before it can be drawn into its image;
- the picture is a frame later;
- the times between frames alternate.

NativeXRenderer's log shows it in the 600-frame lines:
- "frames presented N, completed N-2" instead of N-1;
- "waiting for the GPU" at 1-7 ms a frame instead of 0.

It happens with the record thread (`native_x_renderer_record_thread`), which is on by default in this game.

**Fix.** NativeXRenderer D172 (with the record thread and V-Sync) watches for it. If the frame before is still
unfinished at 16 frames in a row, and the GPU is not simply too slow (under 80 % of the frame's time), it leaves one
frame unshown. The display then takes the waiting one, and the queue is back to one frame. The record thread's log
line counts these frames ("frames not shown, the display's queue a frame too deep").

Checked at 160 FPS (3440x1440) by stalling the record thread on purpose:
- without D172, a stall in a menu left the game a frame behind until the next load;
- with it, 2-3 frames went unshown after each stall, and the extra frame never stayed;
- in a song played normally, no frame went unshown.

**Turning it off.** `native_x_renderer_display_queue_drain = false` in `ghwor.toml`. Turning the record thread off
(`native_x_renderer_record_thread = false`) also avoids the state, at some cost in CPU time at high frame rates.
