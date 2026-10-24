// SPDX-License-Identifier: GPL-2.0-or-later
// bbport memory: which guest pages the GPU side watches, and the protections that make the
// guest's accesses to them known. A page is watched for writes by each image over it and by the
// buffer tracker while its CPU copy is clean; for reads while only the GPU has its newest data
// (readbacks). Faults on watched pages go to the rasterizer.
//
// How a watched page is made known depends on the memory model: with the game's memory in place
// the runtime's write watch is set (the writers that check it report), page protection
// (BB_WRITE_TRACKING=1) or userfaultfd write-protection (BB_UFFD=1) otherwise. Images also set
// write traps (BB_IMAGE_TRAPS), so that writes nobody announced still reach the texture cache.
#pragma once

#include <memory>

#include "common/types.h"
#include "memory/page_bits.h"

namespace Vulkan
{
class Rasterizer;
}

namespace BbMem
{

inline constexpr u32 PAGE_SHIFT = 12;
inline constexpr u64 PAGE_SIZE = 1ull << PAGE_SHIFT;
/// Pages are tracked in regions of 4 MiB, each with its lock and its bits.
inline constexpr u32 REGION_SHIFT = 22;
inline constexpr u64 REGION_SIZE = 1ull << REGION_SHIFT;
inline constexpr size_t PAGES_PER_REGION = REGION_SIZE / PAGE_SIZE;
/// The guest's address space.
inline constexpr u32 ADDRESS_BITS = 40;
inline constexpr size_t NUM_PAGES = 1ull << (ADDRESS_BITS - PAGE_SHIFT);
inline constexpr size_t NUM_REGIONS = 1ull << (ADDRESS_BITS - REGION_SHIFT);

using RegionPages = PageBits<PAGES_PER_REGION>;

constexpr VAddr PageStart(VAddr address)
{
    return address & ~(PAGE_SIZE - 1);
}
/// The start of the page after the one holding `address`.
constexpr VAddr NextPage(VAddr address)
{
    return PageStart(address) + PAGE_SIZE;
}

class PageWatch
{
public:
    explicit PageWatch(Vulkan::Rasterizer* rasterizer);
    ~PageWatch();
    PageWatch(const PageWatch&) = delete;
    PageWatch& operator=(const PageWatch&) = delete;

    /// GPU memory mapped or unmapped (userfaultfd registers it).
    void OnGpuMap(VAddr address, u64 size);
    void OnGpuUnmap(VAddr address, u64 size);

    /// An image over [address, address + size) is watched for writes (`watch`), or no longer.
    template <bool watch>
    void WatchImage(VAddr address, u64 size);
    /// The pages of `mask` in the region at `region`: one watcher more (`watch`) or less, for
    /// writes or (`reads`) for reads.
    template <bool watch, bool reads>
    void WatchRegion(VAddr region, const RegionPages& mask);

    struct Impl;

private:
    std::unique_ptr<Impl> impl;
};

} // namespace BbMem
