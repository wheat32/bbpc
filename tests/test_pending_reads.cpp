// SPDX-License-Identifier: GPL-2.0-or-later
// PendingReads (gpu/render/pending_reads.h): a write never waits for less than the newest noted
// read of its bytes; within a window it waits for nothing else (to the granule).
#include "render/pending_reads.h"

#include <cassert>
#include <cstdio>
#include <memory>
#include <random>
#include <vector>

namespace
{

using Vulkan::PendingReads;

constexpr VAddr RING = 0x1055a24000; ///< where Bloodborne's constant engine dumps
constexpr VAddr DUMP = RING + 0x1490;
constexpr u64 DUMP_BYTES = 512;
constexpr u32 LEVEL_16K = 1; ///< the tables' level for reads of 4 to 256 KiB
constexpr u32 SHIFT_16K = 14;
constexpr u64 RANDOM_READS = 20000;
constexpr u64 WRITE_EVERY = 50;
constexpr u64 RANDOM_SPAN = 4_MB;
constexpr u64 SEED = 1234;
constexpr u64 DUMP_ALIGN = 16; ///< dumps and their sizes come in 16-byte steps
constexpr u64 SIZE_KINDS = 8;  ///< RandomSize: one in eight of each larger kind

struct Read
{
    VAddr address;
    u64 size;
    u64 packet;
};

bool Overlaps(VAddr a, u64 a_size, VAddr b, u64 b_size)
{
    return a < b + b_size && b < a + a_size;
}

/// The newest read of these bytes: what a write must wait for.
u64 Exact(const std::vector<Read>& reads, VAddr address, u64 size)
{
    u64 newest = 0;
    for (const Read& read : reads)
    {
        if (Overlaps(read.address, read.size, address, size))
        {
            newest = std::max(newest, read.packet);
        }
    }
    return newest;
}

/// The same with the write widened to whole window granules: the most a window may answer.
u64 ToTheGranule(const std::vector<Read>& reads, VAddr address, u64 size)
{
    const VAddr first = address & ~(PendingReads::WINDOW_GRANULE - 1);
    const VAddr end = (address + size + PendingReads::WINDOW_GRANULE - 1) & ~(PendingReads::WINDOW_GRANULE - 1);
    return Exact(reads, first, end - first);
}

/// A read past the window whose 16 KiB granule shares the table slot of the dump's.
VAddr CollidingRead()
{
    const u64 slot = PendingReads::Slot(DUMP >> SHIFT_16K, LEVEL_16K);
    for (u64 g = (DUMP + PendingReads::WINDOW_BYTES) >> SHIFT_16K;; ++g)
    {
        if (PendingReads::Slot(g, LEVEL_16K) == slot)
        {
            return g << SHIFT_16K;
        }
    }
}

u64 RandomSize(std::mt19937_64& rng)
{
    switch (rng() % SIZE_KINDS)
    {
        case 0:
            return 4_KB + rng() % (256_KB - 4_KB);
        case 1:
            return 256_KB + rng() % (4_MB - 256_KB);
        case 2:
            return 4_MB + 1 + rng() % 4_MB; // a big entry, pushed out after 16 others
        default:
            return 1 + rng() % 4_KB;
    }
}

} // namespace

int main()
{
    // A read elsewhere sharing the dump's slot: the tables alone make the dump wait for it; a
    // window opened before it was noted does not (at most for the packet before it opened).
    const VAddr other = CollidingRead();
    {
        auto table = std::make_unique<PendingReads>();
        table->Note(other, 128_KB, 7);
        assert(table->Newest(DUMP, DUMP_BYTES) == 7);
    }
    {
        auto table = std::make_unique<PendingReads>();
        table->OpenWindow(DUMP, DUMP_BYTES, 5);
        table->Note(other, 128_KB, 7);
        assert(table->Newest(DUMP, DUMP_BYTES) == 4);
        // A real read of the dump's bytes still counts, and one past its last granule does not.
        table->Note(DUMP + DUMP_BYTES + PendingReads::WINDOW_GRANULE, 64, 8);
        assert(table->Newest(DUMP, DUMP_BYTES) == 4);
        table->Note(DUMP + 256, 16, 9);
        assert(table->Newest(DUMP, DUMP_BYTES) == 9);
    }
    // Reads noted before the window opened are still seen, through the tables.
    {
        auto table = std::make_unique<PendingReads>();
        table->Note(DUMP + 16, 32, 3);
        table->OpenWindow(DUMP, DUMP_BYTES, 10);
        assert(table->Newest(DUMP, DUMP_BYTES) == 3);
    }
    // A big read over the window, then pushed out of the big entries into the tables and windows.
    {
        auto table = std::make_unique<PendingReads>();
        table->OpenWindow(DUMP, DUMP_BYTES, 1);
        table->Note(DUMP - 1_MB, 8_MB, 4);
        assert(table->Newest(DUMP, DUMP_BYTES) == 4);
        for (u64 i = 0; i < table->big.size(); ++i)
        {
            table->Note(RING + (i + 2) * 16_MB, 8_MB, 5 + i);
        }
        assert(table->Newest(DUMP, DUMP_BYTES) == 4);
    }
    // Random reads around the ring, and writes in a window opened before them: never less than the
    // exact answer, never more than the reads of the same granules.
    {
        std::mt19937_64 rng(SEED);
        auto table = std::make_unique<PendingReads>();
        table->OpenWindow(DUMP, DUMP_BYTES, 1);
        const VAddr begin = table->windows[0].begin.load();
        std::vector<Read> reads;
        u64 checked = 0;
        for (u64 packet = 1; packet <= RANDOM_READS; ++packet)
        {
            const u64 size = RandomSize(rng);
            const VAddr address = DUMP - RANDOM_SPAN / 2 + rng() % RANDOM_SPAN;
            table->Note(address, size, packet);
            reads.push_back({address, size, packet});
            if (packet % WRITE_EVERY != 0) continue;
            const u64 bytes = DUMP_ALIGN * (1 + rng() % (4_KB / DUMP_ALIGN));
            const VAddr write = begin + DUMP_ALIGN * (rng() % ((PendingReads::WINDOW_BYTES - bytes) / DUMP_ALIGN));
            const u64 got = table->Newest(write, bytes);
            assert(got >= Exact(reads, write, bytes));
            assert(got <= ToTheGranule(reads, write, bytes));
            ++checked;
        }
        assert(checked == RANDOM_READS / WRITE_EVERY);
    }
    // Writes outside every window (all four open elsewhere): the tables, never less than exact.
    {
        std::mt19937_64 rng(SEED + 1);
        auto table = std::make_unique<PendingReads>();
        for (u32 i = 0; i < PendingReads::MAX_WINDOWS; ++i)
        {
            table->OpenWindow(RING + (i + 1) * 64_MB, DUMP_BYTES, 1);
        }
        table->OpenWindow(DUMP, DUMP_BYTES, 1);
        assert(table->windows[PendingReads::MAX_WINDOWS - 1].begin.load() != 0);
        std::vector<Read> reads;
        for (u64 packet = 1; packet <= RANDOM_READS; ++packet)
        {
            const u64 size = RandomSize(rng);
            const VAddr address = DUMP - RANDOM_SPAN / 2 + rng() % RANDOM_SPAN;
            table->Note(address, size, packet);
            reads.push_back({address, size, packet});
            if (packet % WRITE_EVERY != 0) continue;
            const VAddr write = DUMP - RANDOM_SPAN / 2 + DUMP_ALIGN * (rng() % (RANDOM_SPAN / DUMP_ALIGN));
            assert(table->Newest(write, DUMP_BYTES) >= Exact(reads, write, DUMP_BYTES));
        }
    }
    std::puts("Pending reads: writes wait for every read of their bytes, and in a window for no other PASS");
}
