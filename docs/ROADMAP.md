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
3. **DLSS in every build.** Local builds get it with `DLSS_SDK_ROOT` (a 60 MB partial checkout of
   github.com/NVIDIA/DLSS: `include/`, `lib/Linux_x86_64/libnvsdk_ngx.a`,
   `lib/Linux_x86_64/rel/libnvidia-ngx-dlss.so.*`, `LICENSE.txt`); the CI and the AppImage should
   fetch the same files. The launcher should say when DLSS falls back to FSR 3.1: it happened
   silently.
4. **PGO.** A profile from a few minutes of play cuts stage A's time per draw by ~5%
   (`BB_PGO=generate`, see `docs/parallel_gpu.md`). It needs the game to collect, so only local
   builds can have it.

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
     GPU waits for work: the next measurement.
4. **The upscaler.** DLSS Quality costs ~1 ms on this GPU; lower presets mainly shrink the scene
   passes, which are a small part of the frame.

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
| 1.3 DLSS in every build, fallback shown | build | DLSS where the player chose it |
| 2.1 Render-target copies (done) | GPU, no emulation | -0.85 ms GPU per frame, 120 to 134 FPS |
| 2.2 Idle gaps between submissions (measured) | GPU | none measurable: ~0.3 ms idle left |
| 2.3 Translated shader costs (shadow maps, `fefebf9f`, NVIDIA workarounds measured) | GPU | none so far: shadow maps unchanged, `fefebf9f` at most +2.5%, NVIDIA workarounds ~0.1 ms each (kept for accuracy) |
| 2.3 Passes without targets (depth decompressions, measured) | GPU | none: skipping them changed nothing |
| 2.5 Where the GPU waits for work (`BB_TIMELINE`) | measurement | decides what limits the frame rate |
| 3.x Graphics API translation | CPU, no emulation | most of stage A |
| 4.x Install-time conversion | no emulation | load times, stutter, less code |
