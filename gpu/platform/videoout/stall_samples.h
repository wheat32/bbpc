// SPDX-License-Identifier: GPL-2.0-or-later
// bbport: BB_STALL_SAMPLES=1 (with BB_FRAME_STATS): what the process's threads do while a frame runs
// long. Once a frame is past SAMPLE_AFTER_MS, a thread reads /proc/self/task/*/syscall every
// millisecond: the system call each thread is in (with its arguments and the user-space address
// it was made from), or that it is running. A frame over the stall report's 40 ms prints what
// the threads did that was not sleeping the whole time.
#pragma once

namespace BbVideoOut::StallSamples
{

/// At each flip (frame statistics): starts the sampler the first time, prints what it saw when
/// the frame took over 40 ms, then starts over.
void OnFlip(double frame_ms);

} // namespace BbVideoOut::StallSamples
