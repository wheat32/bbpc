// SPDX-License-Identifier: GPL-2.0-or-later
// bbport: the guest memory that packets queued in the draw pipe still read (stage B binds it, the
// GPU may read it in place), for the CPU writes stage A makes meanwhile: a constant RAM dump waits
// for the newest packet reading the bytes it replaces (Rasterizer::WaitForPendingReads).
#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <memory>
#include <vector>

#include "common/types.h"

namespace Vulkan
{

/// The newest draw pipe packet that reads guest memory, hashed per granule: 256 bytes for reads up
/// to 4 KiB, 16 KiB up to 256 KiB, 1 MiB up to 4 MiB, 8 MiB above (a note costs a few granules, a
/// lookup checks all four). A collision only makes a write wait longer.
///
/// The constant engine's dumps all go to a ring of a few hundred KiB (Bloodborne: 190 KiB, ~13,000
/// dumps a second) that the noted reads hardly ever touch, but reads elsewhere share the dumps'
/// slots: at a busy spot ~110 dumps a second waited for the GPU to run such a read (stage A
/// stopped, then the GPU ran dry: 108 instead of 118 FPS). So stage A opens a window of
/// WINDOW_BYTES around the first write outside the others (OpenWindow, up to MAX_WINDOWS): the
/// reads noted from then on are kept there per WINDOW_GRANULE, without hashing, and a write within
/// it looks there; the tables still answer for the reads noted before it opened.
struct PendingReads
{
    // Granules by the size noted: big buffers (12 MiB ones read by every draw in another game)
    // cost a few slots, not one per MiB.
    static constexpr u64 SHIFTS[4] = {8, 14, 20, 23};
    static constexpr u64 LIMITS[3] = {4_KB, 256_KB, 4_MB};
    static constexpr u32 BITS = 16;
    static constexpr u64 WINDOW_BYTES = 1_MB;
    static constexpr u64 WINDOW_GRANULE = 64;
    static constexpr u32 MAX_WINDOWS = 4;
    // Atomic: resolve workers note reads and look up writes while stage A notes them.
    std::array<std::vector<std::atomic<u64>>, 4> tables{
        std::vector<std::atomic<u64>>(1u << BITS), std::vector<std::atomic<u64>>(1u << BITS),
        std::vector<std::atomic<u64>>(1u << BITS), std::vector<std::atomic<u64>>(1u << BITS)};
    static u64 Slot(u64 granule, u32 level) { return ((granule * 4 + level) * 0x9E3779B97F4A7C15ull) >> (64 - BITS); }
    /// Ranges past Limits[2] (another game's vertex buffers span whole heaps, the same few every
    /// draw): one entry each while they repeat; one pushed out goes into the tables first.
    /// Written by stage A only.
    struct Big
    {
        std::atomic<VAddr> begin{0}, end{0};
        std::atomic<u64> packet{0};
    };
    std::array<Big, 16> big{};
    u32 big_next = 0;
    /// Opened by stage A in order, never moved or closed.
    struct Window
    {
        /// The first byte; 0 while not open. Stored last (release).
        std::atomic<VAddr> begin{0};
        /// The first packet whose reads are all noted here: stage A's next packet when it opened.
        u64 since = 0;
        std::unique_ptr<std::atomic<u64>[]> packets;
    };
    std::array<Window, MAX_WINDOWS> windows{};

    static void Raise(std::atomic<u64>& slot, u64 packet)
    {
        u64 seen = slot.load(std::memory_order_relaxed);
        while (seen < packet && slot.compare_exchange_weak(seen, packet, std::memory_order_relaxed) == false)
        {
        }
    }
    static bool Holds(VAddr begin, VAddr address, u64 size)
    {
        return begin <= address && address + size <= begin + WINDOW_BYTES;
    }
    void Note(VAddr address, u64 size, u64 packet)
    {
        if (size > LIMITS[2])
        {
            for (Big& b : big)
            {
                if (b.begin.load(std::memory_order_relaxed) == address &&
                    b.end.load(std::memory_order_relaxed) == address + size)
                {
                    b.packet.store(std::max(b.packet.load(std::memory_order_relaxed), packet),
                                   std::memory_order_relaxed);
                    return;
                }
            }
            Big& out = big[big_next++ % big.size()];
            const VAddr old_end = out.end.load(std::memory_order_relaxed);
            if (old_end != 0)
            {
                const VAddr begin = out.begin.load(std::memory_order_relaxed);
                NoteTables(begin, old_end - begin, out.packet.load(std::memory_order_relaxed));
            }
            out.packet.store(packet, std::memory_order_relaxed);
            out.begin.store(address, std::memory_order_relaxed);
            out.end.store(address + size, std::memory_order_release);
            return;
        }
        NoteTables(address, size, packet);
    }
    void NoteTables(VAddr address, u64 size, u64 packet)
    {
        const u32 level = size <= LIMITS[0] ? 0 : size <= LIMITS[1] ? 1 : size <= LIMITS[2] ? 2 : 3;
        auto& table = tables[level];
        for (u64 g = address >> SHIFTS[level]; g <= (address + size - 1) >> SHIFTS[level]; ++g)
        {
            Raise(table[Slot(g, level)], packet);
        }
        for (Window& window : windows)
        {
            const VAddr begin = window.begin.load(std::memory_order_acquire);
            // Opened in order: a packet a worker resolves while this one is not open to it was made
            // before it opened (and before any later one), and the tables answer for it.
            if (begin == 0) return;
            const VAddr first = std::max(address, begin);
            const VAddr end = std::min(address + size, begin + WINDOW_BYTES);
            if (first >= end) continue;
            for (u64 g = (first - begin) / WINDOW_GRANULE; g <= (end - 1 - begin) / WINDOW_GRANULE; ++g)
            {
                Raise(window.packets[g], packet);
            }
        }
    }
    /// Stage A, before a write: a window around it, unless one holds it already or all are open.
    /// `next_packet`: the number the next packet handed to the draw pipe gets.
    void OpenWindow(VAddr address, u64 size, u64 next_packet)
    {
        if (size == 0 || size > WINDOW_BYTES / 2) return;
        for (Window& window : windows)
        {
            const VAddr begin = window.begin.load(std::memory_order_relaxed);
            if (begin != 0)
            {
                if (Holds(begin, address, size)) return;
                continue;
            }
            // Centered on the write: the rest of a ring around it falls inside too.
            const VAddr start = address & ~(WINDOW_GRANULE - 1);
            window.since = next_packet;
            window.packets = std::make_unique<std::atomic<u64>[]>(WINDOW_BYTES / WINDOW_GRANULE);
            window.begin.store(start > WINDOW_BYTES / 2 ? start - WINDOW_BYTES / 2 : WINDOW_GRANULE,
                               std::memory_order_release);
            return;
        }
    }
    u64 Newest(VAddr address, u64 size) const
    {
        u64 newest = 0;
        for (const Big& b : big)
        {
            if (b.begin.load(std::memory_order_relaxed) < address + size &&
                address < b.end.load(std::memory_order_acquire))
            {
                newest = std::max(newest, b.packet.load(std::memory_order_relaxed));
            }
        }
        u64 hashed = 0;
        for (u32 level = 0; level < 4; ++level)
        {
            for (u64 g = address >> SHIFTS[level]; g <= (address + size - 1) >> SHIFTS[level]; ++g)
            {
                hashed = std::max(hashed, tables[level][Slot(g, level)].load(std::memory_order_relaxed));
            }
        }
        for (const Window& window : windows)
        {
            const VAddr begin = window.begin.load(std::memory_order_acquire);
            if (begin == 0) break;
            if (Holds(begin, address, size) == false) continue;
            for (u64 g = (address - begin) / WINDOW_GRANULE; g <= (address + size - 1 - begin) / WINDOW_GRANULE; ++g)
            {
                newest = std::max(newest, window.packets[g].load(std::memory_order_relaxed));
            }
            // The reads noted before it opened, from the tables (whose slots may hold a later read
            // elsewhere: that one counts as noted just before the window opened).
            return std::max(newest, std::min(hashed, window.since - 1));
        }
        return std::max(newest, hashed);
    }
};

} // namespace Vulkan
