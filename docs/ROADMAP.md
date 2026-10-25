# Roadmap: native speed, no emulation

bbpc has two goals from here:

1. **No emulation.** The game's own x86-64 code already runs directly on the PC's CPU, and the
   runtime (`src/`) replaces the PS4's system libraries the way Wine replaces Windows'. What is
   left is the part that behaves like the PS4's GPU: decoding its command packets, translating
   shaders while the game runs, copying render targets in and out of the PS4's shared memory, and
   tracking the game's writes to that memory. The target: the game's graphics calls translated
   straight to Vulkan, and everything PS4-specific (shaders, textures) converted once from the
   player's dump. The dump still provides the game's executable and its assets.
2. **150+ FPS at 1440p** on a strong PC. Reference machine: RTX 4080, Ryzen 9 7900X, a 180 Hz
   2560x1440 monitor.

Running without the game's executable (a decompilation of Bloodborne) is out of scope.

The two goals meet: most of the CPU time that limits the frame rate today is spent emulating the
PS4's GPU (phase 3).

## Measuring

`tools/bench.sh NAME [key=value...]` is the benchmark every step is measured with. It copies a
save and its settings into `out/bench/` (the player's own files are never written), applies the
`key=value` lines to that copy of `bbport.ini`, chooses Continue on the title, stands still in the
level for 40 s (`HOLD`), closes the game and prints FPS (average, median, 1% low), the spread of
frame times, CPU use per thread and GPU utilization. Two profilers go with it:

- `BB_SECTIONS=1`: CPU time per part of a draw on the two GPU threads (stage A, `bb:GpuComm`, and
  stage B, `bb:DrawRec`), printed every 5 s with the draw pipe statistics.
- `BB_GPU_PROFILE=1`: GPU time per render pass, dispatch and upscaler run.

Both cost a few percent themselves; measure FPS without them. A single run in one place varies by
a few FPS: compare at least two runs before trusting a small difference.

Start the benchmark where nothing polls the game's memory: the Claude desktop app reads the
memory map of every process its sessions start (1.5), which gives 40-90 ms frames that are not the
port's. `BB_STALL_SAMPLES=1` (with `BB_FRAME_STATS`) lists what every thread did during a frame
over 40 ms.

## Where it stands (2026-10-24)

Reference machine, `upscaler=dlss preset=1` (DLSS Quality), output 2560x1440, uncapped, standing
still where the reference save loads:

| Configuration | Average | Median | 1% low | Worst frame |
|---|---|---|---|---|
| Live resolution on (`live_resolution=auto` picks it), FSR 3.1 | 116 FPS | 134 FPS | 72 FPS | 89 ms |
| Live resolution off, FSR 3.1 | 129 FPS | 143 FPS | 78 FPS | 14 ms |
| Live resolution off, FSR 3.1, PGO build | 126 FPS | 150 FPS | 77 FPS | 14 ms |
| Live resolution off, DLSS Quality, PGO build | 119 FPS | 145 FPS | 74 FPS | 14 ms |
| The same, render-target copies on the GPU (2.1) | 134 FPS | 143 FPS | 84 FPS | 14 ms |

(The FSR 3.1 rows are DLSS settings that fell back: the build had no DLSS SDK yet.)

At 150 FPS a frame has 6.67 ms. Three limits sit near or past that:

| Limit | Now | |
|---|---|---|
| GPU time per frame | 7.44 ms (8.35 ms before 2.1) | RTX 4080 at 93-94% utilization |
| Stage A (`bb:GpuComm`): decodes the command packets, picks pipelines, prepares draws | 6.0-6.3 ms per frame, ~2.5 µs per draw, 2,400 draws | idle 2-3% |
| Stage B (`bb:DrawRec`): records the draws into Vulkan | ~2.1 µs per draw | ~73% busy |

GPU time per frame before 2.1 (live resolution off, DLSS Quality, `BB_GPU_PROFILE=1`):

| Work | ms/frame |
|---|---|
| Shadow maps (4096x4096 depth, ~24 passes) | 1.23 |
| G-buffer and depth passes at 1708x960 | ~1.5 |
| Upscaler (DLSS, 1708x960 to 2560x1440; the profiler labels it "RunScaled (FSR)") | 1.03 |
| Compute shader `fefebf9f` (53 dispatches of 1 to 1,024 groups; mostly overlaps other work, see 2.3) | 0.82 |
| Low-resolution passes (854x480 RGBA16F 0.54, 427x240 0.18) | 0.72 |
| **Copies of render targets to and from the PS4's memory** (image downloads and uploads) | **0.69** |
| **GPU idle between submissions** (67 submissions per frame) | **~0.5-0.8** |
| The rest (~100 smaller passes and dispatches) | ~1.6 |

The two bold rows exist only because of emulation.

The GPU is the limit that decides the average: 8.35 ms per frame allows 1000 / 8.35 = 120 FPS,
exactly what the benchmark measures. The CPU side makes a frame every 6-7 ms (the median's pace)
and runs ahead until the frames waiting for the GPU fill the queue; then every third frame waits
(10-15 ms). The Immediate present mode spreads the frame times more evenly but gives the same
120 FPS average: it is not a presentation stall.

GPU time per frame after 2.1 (the same settings, grouped by kind of work, 2026-10-24):

| Work | ms/frame |
|---|---|
| G-buffer passes at 1708x960 (six color targets, ~25 a frame) | 1.30 |
| Shadow maps (4096x4096 depth, ~27 passes a frame) | 1.30 |
| Other passes at 1708x960 (~52 a frame) | 1.03 |
| Upscaler (DLSS Quality) | 1.01 |
| Compute shader `fefebf9f` (53 dispatches; mostly overlaps other work, see 2.3) | 0.86 |
| Passes at 854x480 | 0.66 |
| GPU idle between submissions | 0.48 |
| Passes without a target: depth decompressions, no-ops here (~15 a frame; not real cost, see 2.3) | 0.36 |
| Other compute dispatches | 0.22 |
| The rest (passes at 2560x1440 and below 854x480, transfers) | ~0.24 |

7.44 ms per frame allows 134 FPS, the benchmark's average. The frame log tells the CPU side
apart: half the frames take 5.8 ms with 5.7 ms of stage A's time, a quarter take 11 ms with the
same 5.8 ms (they wait, every third frame). Were the CPU side and the GPU perfectly overlapped,
the CPU side would allow ~170 FPS and the GPU would have to lose ~0.8 ms per frame for 150 FPS.
They are not: GPU work taken out so far gave less FPS than that predicts, or none (2.3), so where
the two wait for each other comes first (`BB_TIMELINE`).

## Phase 1: settings and frame pacing

1. **Live resolution off by default.** Done (2026-10-24): `live_resolution=auto` used to turn it
   on for strong GPUs (a GPU check in `run.sh`). It keeps the game at 1920x1080 internally and
   copies scene targets back, which cost ~11% and caused 90 ms hitches here. "Auto" is gone from the
   launcher and the in-game menu, and the `auto` of older settings now means off; live changes
   (presets without a restart) stay an option.
2. **The every-third-frame pattern.** Explained (2026-10-24): not a stall but the GPU limit seen
   from the CPU side (see above). Nothing to fix by itself; phase 2 is what moves it.
3. **DLSS in every build.** Done for the packages (0.7.2): `tools/fetch_dlss_sdk.sh` fetches the
   files the build needs (a 60 MB partial checkout of github.com/NVIDIA/DLSS: `include/`,
   `lib/Linux_x86_64/libnvsdk_ngx.a`, `lib/Linux_x86_64/rel/libnvidia-ngx-dlss.so.*`,
   `LICENSE.txt`) for local builds, the CI and the AppImage. Still to do: the launcher should say
   when DLSS falls back to FSR 3.1; it happens silently.
4. **PGO.** A profile from a few minutes of play cuts stage A's time per draw by ~5%
   (`BB_PGO=generate`, see `docs/parallel_gpu.md`). It needs the game to collect, so only local
   builds can have it.
5. **The ~70 ms frames while standing still.** Explained (2026-10-25): another process reading the
   game's memory map. The Claude desktop app reads `/proc/<pid>/smaps_rollup` of the processes
   its sessions start, every ~55 s; for the game (13 GB resident) the kernel walks its page tables
   for ~105 ms with its address space lock held. Whatever in the game needs that lock for writing
   meanwhile waits: the malloc heap's `brk` (holding the one arena's lock, so every allocating
   thread waited too: stage A, the draw workers, the game's threads), the write traps' `mprotect`
   (layer, texture cache, the trap handler) and layer promotions. Runs started outside such a
   process tree had no frame over 19 ms in ~11 minutes of standing still; inside it nearly every
   run had one or two of 40-90 ms. A system monitor showing per-process memory would do the same.
   - Done: the heap no longer gives its top back to the kernel and takes it again over 110 times
     a second (glibc's 128 KiB trim threshold; `M_TRIM_THRESHOLD` 64 MiB and `M_TOP_PAD` 16 MiB
     in `src/probe.c`): no `brk` while standing still, and ~16,000 fewer page faults a second.
     Under the desktop app's reads, two runs of 120 s had no long frame (two and four before);
     FPS unchanged (the GPU is the limit).
   - Still open: the write traps and promotions change protections with `mprotect`, which needs
     the lock for writing. Five reads in a row still gave five 40-85 ms frames. Write protection
     through userfaultfd (`UFFDIO_WRITEPROTECT` needs the lock only for reading; the page watch
     has it as `BB_UFFD=1`) for the runtime's traps would remove them.
   - `BB_STALL_SAMPLES=1` samples every thread each millisecond once a frame is past 25 ms
     (`/proc/self/task/*/syscall`: the system call and where it was made, or running) and prints
     what was not asleep with the stall report. It allocates nothing, so it keeps sampling while
     the other threads wait for the malloc arena.

## Phase 2: GPU work (target: about -2 ms)

1. **Keep render targets on the GPU (0.69 ms).** Done (2026-10-24). The images were downloaded
   for one compute shader, `3d5ebf4e`, a dword copy the game runs on its render targets' memory
   (the scene depth and four color targets, every frame). The port already
   turned such copies into image copies when both sides were images (`IsComputeImageCopy`); now it
   also makes the destination image when the memory holds none (two of the copies are not read
   by any shader while playing), and takes the depth copy's stencil plane as trailing bytes.
   `BB_IMAGE_COPY_LOG=1` shows the copies and why one is left to the shader,
   `BB_COPY_DESTINATIONS=0` turns the new destinations off. GPU time 8.35 to 7.50 ms per frame;
   +11% average FPS, 1% low 75 to 84 FPS; screenshots of the level and the pause menu unchanged.
   `BB_TILE_BACK_LOG=1` now names the shader reading an image as a buffer, and
   `BB_WATCH_READS=<address,...>` prints every shader binding chosen addresses.
2. **Close the GPU's idle gaps.** Measured (2026-10-24), little left: after 2.1 the GPU is idle
   ~0.3 ms per frame (most of it after the frame's first submission), and recorded work already
   goes out at once when the GPU runs dry (`SubmitForSignalsNow`, `BB_HONEST_*`). Shorter batch
   intervals (`BB_HONEST_FLUSH_US=1000` and `500` instead of 2000) gave 135.2 and 133.2 FPS
   against 133.7: within run-to-run noise. Left as is.
3. **Shadow maps, the low-resolution passes and `fefebf9f`.** Compare the SPIR-V the shader
   translator makes for these shaders with what the GCN code needs (instruction counts, scalar
   loads turned into buffer loads, 64-wide wave lowering, missing FP16), and their pipeline state.
   Costs a native shader would not have are the target. The translated SPIR-V of every shader is
   in the data folder (`user/cache/CUSA03173/<hash>_<variant>.spv`; `spirv-cross` makes it
   readable).
   - Shadow maps (2026-10-24): their pixel shader (`a2108f5e`) is an empty stub, but leaving it
     out of the pipeline changed nothing on an RTX 4080 (0.915 against 0.885 ms per frame): the
     driver already treats it as depth-only. Their cost is the vertex work (skinned meshes, ~18
     passes). A general "the pixel shader does nothing" test cannot use the cached shader info:
     only `InfoPersistent` is stored, so `stores` (depth export) and `has_discard` come back
     empty. A first attempt dropped a depth-writing shader (`e820ee9d`) that way, and with it an
     854x480 effect pass; the GPU profile showed the missing pass.
   - `fefebf9f` (53 dispatches a frame, 0.82 ms) is the game's GPU memory copy (the game profile's
     `buffer_copy_shader`): each workgroup copies up to 64 dwords its control list names. Measured
     (2026-10-24): ~455 ranges and only ~1.6 MB a frame, so bandwidth is not the cost. Its source
     or destination (whichever binding spans 64 MiB or more) goes through the page table
     (`IsPagedBuffer`), which puts a full barrier before each dispatch; a paged destination's
     write (`AccessGlobal`), or a normal destination recorded as written over its whole bound range
     (tens of MiB), makes later commands wait for it too. The copies are almost never back to back
     (most have over 64 other commands between them), so skipping barriers between consecutive
     copies gains nothing (tried, removed). The other path, `BB_COPY_SHADER_NATIVE=0` (copy list
     read on the CPU, Vulkan buffer copies), costs more: ~1 ms, 130 FPS.
   - Barriers from the ranges the copy really touches (2026-10-24): not worth it. With the copy
     made invisible to the barrier tracker (unsafe, a measurement only), the benchmark gave 137.4
     FPS average and 87.6 FPS 1% low, against 133.6 and 134.6 FPS average, 83.6 and 82.7 FPS 1% low
     for two runs without it: at most +2.5% average and +5% 1% low, before the barriers a correct
     version keeps and the per-draw cost it adds to stage B (85-89% busy). The 0.82 ms the profiler
     charges to the copies mostly overlaps other work. A correct version is also bigger than it
     looked. The copy's footprint is exact: in all four of its variants (from the SPIR-V), each
     workgroup reads one 12-byte control entry `{x, y, z}`, then source dwords `y` to
     `y + min(z, 63)`, and writes destination dwords `x` to `x + min(z, 63)`; through the page
     table it reads a page's VRAM mirror where there is one and writes both the mirror and the
     game's memory. But several reads it would have to be checked against are checked before
     their command and never recorded after it: vertex buffers, indirect arguments and count
     buffers, `CopyFromGuestChunk` sources and the page table. Today the copy's unconditional
     barrier covers them. Vulkan's synchronization validation cannot check such a change either:
     it does not see accesses through device addresses (the page table), and it counts a storage
     buffer's whole bound range as accessed. Left as is.
   - Separately: vertex buffer reads are never recorded as accesses (index buffers are,
     `ResolveIndexBuffer`), so a GPU write to a vertex range after a draw that reads it, with no
     barrier between, gets none. Not seen to cause anything.
   - The two NVIDIA-only shader workarounds (2026-10-24), each measured with a switch that turned
     it off: manual interpolation (`needs_manual_interpolation`: 372 of the 376 pixel shaders read
     their inputs per vertex and interpolate them with the PS4's formula) costs ~0.08 ms per
     frame; the clip distance emulation (`needs_clip_distance_emulation`: 31 vertex shaders hand
     their clip distances to a pixel shader that discards) ~0.10 ms. FPS stayed within run-to-run
     noise for both. Both are accuracy fixes from shadPS4, and both stay. Manual interpolation was
     made for Bloodborne itself (shadPS4 #1644): instanced objects flickered on NVIDIA, because the
     game passes the instance index to the pixel shader as a float and NVIDIA's interpolation of
     three equal values is not exact. The clip distance emulation is for stripes on tilted
     polygons (shadPS4 #3958: NVIDIA's clip distances differ slightly from AMD's). Still frames
     with either one off matched the baseline within the noise between two baseline runs, but the
     flicker shows only in motion, and only in some areas.
   - The passes without a target (2026-10-24): ~15 a frame (the profiler shows them at
     32768x32768, the largest framebuffer size), each one rectangle from vertex shader `89cda9e4`
     without a pixel shader. A log of their registers: all are in-place depth/stencil
     decompressions (`DB_RENDER_CONTROL` 0x60: `DEPTH_COMPRESS_DISABLE` and
     `STENCIL_COMPRESS_DISABLE`, depth and stencil tests off, color off), over the scene depth,
     its half and quarter sizes and the 4096x4096 shadow map. The port never compresses depth, so
     they write nothing; their only effect is on the occlusion query the port keeps running, and
     across 34 logged frames (370 of them, 884 occlusion events) none fell between a query's
     "before" and "after" events (the game's results are the differences). Skipped like the FMASK
     decompression (`DrawPass::Skip`) they could be, exactly, when no query interval is open. A
     test skip (all of them, ~104,000 per run, none held back by an open interval) gave 132.9 and
     134.3 FPS against six baseline runs of 132.7 to 134.6, and the same screenshots: no gain.
     Their 0.36 ms did not come off the frame: the profiler charges a pass with the tail of the
     work before it, and in two of three windows the GPU's time waiting for submissions rose from
     ~0.26 to ~0.7 ms per frame. Left as is.
   - What these measurements have in common: GPU work taken out (the copy shader's barriers, the
     NVIDIA workarounds, these passes) gave much less FPS than the profiler's numbers promised, or
     none. The frame rate also depends on when the work reaches the GPU. `BB_TIMELINE=<file>`
     (guest submissions and waits, decoding, Vulkan submissions, GPU completion) shows where the
     GPU waits for work: item 5.
4. **The upscaler.** DLSS Quality costs ~1 ms on this GPU; lower presets mainly shrink the scene
   passes, which are a small part of the frame.
5. **Where the GPU waits for work (`BB_TIMELINE`).** Measured (2026-10-24) at a busier spot (the
   reference save moved there: 2,755 draws a frame, 104 FPS against 134 at the old spot). The GPU
   was idle at least 2.56 ms per frame, in ~10 gaps, while stage A never ran out of work and the
   game waited 7.7 ms a frame for the previous one. The gaps came from constant RAM dumps that
   waited for the GPU (`WaitForInPlaceReads`: ~138 a second, ~170 ms of stage A's time a second):
   each one let the GPU's queue run empty, and the GPU then waited for stages A and B to refill
   it. With that wait off (unsafe, a measurement only: `BB_CONSTRAM_GPU_WAIT=0`) the benchmark
   gave 117.2 FPS against 104.0, and the GPU's idle time fell to 0.11 ms per frame.
   - Why they waited: a probe naming each wait's read found none that overlapped the dump. 80%
     came from the pending-read table's hashing. The dumps all go to one ring (190 KiB, ~13,000
     dumps a second) that no noted read touched (0 of 30.7 million), but reads elsewhere, mostly
     170 KiB buffers, shared the table slots of the ring's granules: a dump found them and waited
     for the GPU to run them. The other 20% follow the end-of-submission task (`OnSubmit`: fault
     buffer, image downloads, garbage collection), which counts as reading memory it does not list
     (`untracked_reads_packet`).
   - Done (2026-10-24) for the first: around the dumps the table keeps reads exactly
     (`gpu/render/pending_reads.h`: windows of 1 MiB around the first dumps, 64-byte granules,
     no hashing; `BB_CONSTRAM_WINDOWS=0` turns them off). A dump there waits only for reads of its
     bytes; reads noted before a window opened still come from the hashed tables.
     `tests/test_pending_reads.cpp` checks that a write never waits for less than every read of
     its bytes (random reads and writes against an exact list) and, in a window, for nothing past
     its granules. Three alternating runs each: 117.5 FPS average against 107.7 (+9%), the median
     frame 8.1 against 9.2 ms, the GPU 94-95% busy against 85-90%, dumps waiting for the GPU 4-25
     a second against 97-147; screenshots of the spot unchanged. In the timeline the GPU is idle
     0.30 ms per frame (2.56 before), and frames come 8.3 ms apart with a spread of 0.65 ms (10.7
     and 1.3 ms before).
   - The benchmark's frame times (`BB_FRAME_LOG`, `bench_stats.py`) are taken when stage A hands
     a flip to the presenter, so they also show where stage A waits. With the windows alone their
     "1% low" fell (77 against 83 FPS) although the frames the game makes and the GPU finishes came
     more evenly than before: the dumps' remaining GPU waits (below) now held stage A for a whole
     frame of GPU work every few frames. The timeline's `FrameRetired` intervals show what reaches
     the screen.
   - Every run, before and after, has one or two ~70 ms frames while standing still: some with
     stage A 20-40 ms in the kernel without page faults or protection changes, some in the layer
     memory's traps (60 ms, 3 `mprotect` calls). Not explained yet.
   - Done (2026-10-25) for the second: the end-of-submission task (`OnSubmit`, then a flush)
     reads guest memory only on the recording thread. Its GPU wait came from the mark for reads a
     packet does not list, which the GPU wait was added for (draws whose GPU reads are not listed).
     A task reading guest memory (`RunInOrder`'s `reads_guest_memory`, only this task) now makes a
     dump wait for the recording thread to have run it (`task_reads_packet`), and draws keep the
     GPU wait. What the task hands the GPU that writes the game's memory (in-place image downloads,
     also the garbage collector's) still comes before the dump: `OnSubmit` notes the tick when it
     downloaded anything, and the dump waits for the GPU to have run it
     (`WaitForSubmissionWrites`; not needed once in these runs). `BB_CONSTRAM_TASK_GPU=1` brings the
     old wait back. Three alternating runs each: 120.4 FPS average against 119.6 (the GPU is the
     limit, 96% busy), but a 1% low of 92 against 77 FPS, the 99th percentile frame 10.2-11.9
     against 12.8-13.1 ms, and 19-44 frames over 12 ms against 252-380; dumps waiting for the GPU
     none against 9-30 a second; screenshots of the spot unchanged. Both parts together, against
     the start: 120.4 against 107.7 FPS average (+12%), 1% low 92 against 83 FPS.
   - Still open: none of these waits covers stages that read memory through pointers when the
     GPU runs them (`uses_dma`; `BB_EXP_DMA_UNTRACKED=1` is the experiment that would). None ran
     at this spot.

## Phase 3: translate the graphics API instead of the command packets

This is the main no-emulation step and the answer to the CPU limits. Bloodborne links Sony's
graphics library (libGnm) statically: its render threads (`GXRenderThread` and five `GXWorker`s,
~22% of a core each here) build PS4 command packets through 99 `sce::Gnm::*CommandBuffer`
methods, and stage A decodes those packets back into state and draws (with `BB_SECTIONS=1`:
decode 2.8 µs per packet). The original project found that 100% of the packets come from those
methods (`docs/EMULATION_REMOVAL_PLAN.ru.md` in bloodborne_pc).

1. **Inventory.** `BB_GNM_OBSERVE=1` (`gpu/shim/bbport_gnm_hooks.cpp`) runs the methods through
   observing stubs: list them with call counts and map each one to Vulkan state or a draw.
2. **Replace the methods.** Hooks that record the draw description directly, without packets,
   starting with the draws and the state they read. Packets that still arrive keep today's path,
   so the two coexist while methods move over.
3. **Resource descriptors.** The game still writes GCN descriptors (buffers, textures, samplers)
   into memory for its shaders: look them up by identity (address and memory generation) instead
   of per draw.

Expected: most of stage A's work goes (decoding and rebuilding register state), and so does the
packet encoding on the game's own render threads. Risks: command buffers the engine builds once
and replays, methods inlined into the engine, packets written outside the methods.

## Phase 4: convert once, at install time

1. **Shaders.** Translate every shader of the dump to SPIR-V and a pipeline cache during setup.
   The shader corpus test (`tests/test_shader_corpus.cpp`) already checks the translator against
   the game's shaders.
2. **Textures.** Untile the PS4's textures into PC formats at install. This needs the engine's
   texture file formats; the gain is load time and streaming stutter more than FPS.
3. **The old memory model (0.3).** Remove it once the PC memory model is confirmed on NVIDIA and
   Intel; the benchmarks above ran on the PC model.
4. **What remains of the hardware model.** The constant engine's RAM, the SRT walker (GCN shader
   code run on the CPU) and the tracking of GPU writes exist because the translator reads
   descriptors from memory at draw time; phase 3 removes the reason for each.

## Also planned

- **The resolution in the game's own settings.** A native output resolution option in the game's
  System menu, next to the entries bbpc already adds there (`gpu/shim/bbport_game_menu.cpp`).
  The startup patch sizes the game's render targets when it starts, so a change would apply
  through an automatic restart of the game (`runtime_restart`), as the in-game menu's "Apply and
  restart game" button does today.

## Order

| Step | Kind | Expected effect |
|---|---|---|
| 1.1 Live resolution off by default (done) | setting | +11% FPS, no 90 ms hitches |
| 1.3 DLSS in every build (done in 0.7.2), fallback shown | build | DLSS where the player chose it |
| 1.5 The ~70 ms frames (explained): malloc trim threshold (done) | heap | no `brk` or its page faults while playing; fewer stalls when something reads the memory map |
| 2.1 Render-target copies (done) | GPU, no emulation | -0.85 ms GPU per frame, 120 to 134 FPS |
| 2.2 Idle gaps between submissions (measured) | GPU | none measurable: ~0.3 ms idle left |
| 2.3 Translated shader costs (shadow maps, `fefebf9f`, NVIDIA workarounds measured) | GPU | none so far: shadow maps unchanged, `fefebf9f` at most +2.5%, NVIDIA workarounds ~0.1 ms each (kept for accuracy) |
| 2.3 Passes without targets (depth decompressions, measured) | GPU | none: skipping them changed nothing |
| 2.5 Where the GPU waits for work (`BB_TIMELINE`): exact dump checks, task reads waited for on stage B (done) | stage A, no emulation change | +12% FPS and 1% low 83 to 92 FPS at a busy spot; GPU idle 2.56 to 0.30 ms per frame |
| 3.x Graphics API translation | CPU, no emulation | most of stage A |
| 4.x Install-time conversion | no emulation | load times, stutter, less code |
