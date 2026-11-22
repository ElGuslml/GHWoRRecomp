# The game ending in a song: the lighting job on the wrong scene

**Status: fixed (2026-11-21), `ghwor_lighting_job_wait`, on by default.** Measured with a temporary probe and the
bot; the results are below, under "Measured".

## Symptom

On a busy CPU, the game can end in the middle of a song. The log shows a job worker's crash:

```
Unhandled guest access violation: read of guest 0x00000070 ... on thread 0xF80000xx
[CRASH] guest thread F80000xx lr=82648B60
```

It came after 13 and 17 minutes of a bot playing songs, with the game held to four efficiency cores and no frame rate
limit. Two to twenty-four times in a song, on any PC, the same cause gave a frame whose lights were built from another
scene's data. That is too short to see.

## Cause

The engine draws each scene through a list of 19 render stages. Stage 3 queues a lighting job (`sub_82648B08`) on a
job worker, and stage 9 waits for it and draws the light volume the job built. The job does not get its scene from the
stage. It reads the scene the render thread is drawing, a global the render thread sets before each scene.

The engine splits a frame's 3D work over two frames: stages 1-7 in one, 8-19 in the next. In a first-half frame nothing
waits for the job before the render thread goes on to the next scene and sets the global. On the console a worker
started the job within microseconds, while stages 4-7 issued their draws for milliseconds. Here those stages take the
render thread well under a millisecond. A job that starts later than that reads the next scene. If that scene has no
light block, the job reads address 0x70 and the game ends.

## Fix

`src/lighting_job.cpp` adds a hook where every scene's stages end, before the next scene (`0x8255A994`,
`hooks/lighting_job.toml`). If the lighting job is still queued or running, the render thread calls the game's own
wait for its job group, as stages 3 and 9 do. It runs the queued job itself or waits for the worker that has it, so
the scene does not change under the job. The job is short (about 0.04-0.07 ms), so the wait is rare and short.

## Measured

The probe stamped each job's queueing, its two reads of the scene, its end, the scene changes and the hook's waits.
"Wrong scene" counts jobs that read a scene other than the one that queued them.

| Run | Jobs | Wrong scene, before the fix | Wrong scene, with it | The hook waited (ms: median / longest) |
|---|---|---|---|---|
| Bot, four cores, no limit, 30 min | 306,018 | 2.7-3.9 %, a crash at 13 and 17 min | 0, no crash | 35,968 times (0.013 / 8.3), 2.6 s in all |
| A song at 60 FPS, four cores | 7,562 | 2-4 | 0 | 6 times (0.015 / 0.033) |
| A song at 120 FPS, four cores | 14,926 | 8-10 | 0 | 20 times (0.018 / 0.31) |
| A song at 160 FPS, V-Sync | 19,831 | 6-24 | 0 | 31 times (0.013 / 0.028) |
| A song with no limit | 81,072 | not measured | 0 | 208 times (0.002 / 0.22) |
| A song at 160 FPS, the venue every frame | 39,658 | 0 (stage 9 waits in the same frame) | 0 | never |

Before the fix the counts come from the probe runs of the same day. The job's length was 0.04-0.07 ms at the
median and under 0.1 ms at the 99th percentile. Outside the bot, the hook waited mostly in menus and loads; in a
song it waited at most once. Unchanged by the fix:
- the render thread's time per frame, at the median and the 99th percentile, in every run;
- the frame rate with no limit (four cores 249.9 FPS, against 249.0-254.1 before; all cores 584.6, against
  586.7-587.2);
- the frame times at 60 and 120 FPS on four cores.

With the setting off, a song with no limit on four cores again had 289 jobs on the wrong scene, and the hook did
not wait.

## How to turn it off

Set `ghwor_lighting_job_wait = false` in `ghwor.toml`. The game then behaves as before, race included.
