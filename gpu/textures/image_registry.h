// SPDX-FileCopyrightText: Copyright 2026 bbport contributors
// SPDX-License-Identifier: GPL-2.0-or-later
// bbport textures: which images lie over each 1 MiB page of the guest's address space, as a
// two-level table (1024 x 1024 pages over 40 bits; second-level blocks made on first use).
#pragma once

#include <array>
#include <memory>

#include <boost/container/small_vector.hpp>

#include "common/types.h"
#include "textures/types.h"

namespace VideoCore
{

class ImageRegistry
{
public:
    static constexpr u32 PAGE_BITS = 20;
    using Ids = boost::container::small_vector<ImageId, 16>;

    /// The images over `page`, or null when no image ever was.
    Ids* Find(u64 page)
    {
        Block* block = blocks[(page >> BLOCK_BITS) & (NUM_BLOCKS - 1)].get();
        return block != nullptr ? &(*block)[page & (PAGES_PER_BLOCK - 1)] : nullptr;
    }
    /// The images over `page` (its block made if needed).
    Ids& At(u64 page)
    {
        auto& block = blocks[(page >> BLOCK_BITS) & (NUM_BLOCKS - 1)];
        if (block == nullptr)
        {
            block = std::make_unique<Block>();
        }
        return (*block)[page & (PAGES_PER_BLOCK - 1)];
    }

    /// Calls `func(ids)` for the pages of [address, address + size) that hold images, until it
    /// returns true.
    template <typename Func>
    void ForEachPage(VAddr address, u64 size, Func&& func)
    {
        const u64 last = (address + size - 1) >> PAGE_BITS;
        for (u64 page = address >> PAGE_BITS; page <= last; ++page)
        {
            Ids* ids = Find(page);
            if (ids != nullptr && ids->empty() == false && func(*ids))
            {
                return;
            }
        }
    }
    /// Calls `func(page)` for every page of [address, address + size).
    template <typename Func>
    static void ForEachPageIndex(VAddr address, u64 size, Func&& func)
    {
        const u64 last = (address + size - 1) >> PAGE_BITS;
        for (u64 page = address >> PAGE_BITS; page <= last; ++page)
        {
            func(page);
        }
    }

private:
    static constexpr u32 BLOCK_BITS = 10;
    static constexpr u64 PAGES_PER_BLOCK = 1ull << BLOCK_BITS;
    static constexpr u64 NUM_BLOCKS = 1ull << (40 - PAGE_BITS - BLOCK_BITS);
    using Block = std::array<Ids, PAGES_PER_BLOCK>;

    std::array<std::unique_ptr<Block>, NUM_BLOCKS> blocks{};
};

} // namespace VideoCore
