// SPDX-License-Identifier: GPL-2.0-or-later
// bbport: error codes the PS4 system libraries return (the values games compare against).
#pragma once

#include "common/types.h"

namespace BbErr
{

inline constexpr s32 OK = 0;
inline constexpr s32 FAIL = -1;

// Kernel (errno values with the 0x8002 prefix).
inline constexpr s32 KERNEL_BAD_FD = s32(0x80020009);        // EBADF
inline constexpr s32 KERNEL_INVALID = s32(0x80020016);       // EINVAL
inline constexpr s32 KERNEL_NOT_FOUND = s32(0x80020002);     // ENOENT
inline constexpr s32 KERNEL_NO_MEMORY = s32(0x8002000C);     // ENOMEM
inline constexpr s32 KERNEL_BUSY = s32(0x80020010);          // EBUSY
inline constexpr s32 KERNEL_TIMED_OUT = s32(0x8002003C);     // ETIMEDOUT
inline constexpr s32 KERNEL_AGAIN = s32(0x80020023);         // EAGAIN
inline constexpr s32 KERNEL_FAULT = s32(0x8002000E);         // EFAULT
inline constexpr s32 KERNEL_NAME_TOO_LONG = s32(0x8002003F); // ENAMETOOLONG

// errno values (posix_* calls return -1 and set these).
inline constexpr int POSIX_NO_ENTRY = 2;   // ENOENT
inline constexpr int POSIX_BAD_FD = 9;     // EBADF
inline constexpr int POSIX_NO_MEMORY = 12; // ENOMEM

// libSceGnmDriver.
inline constexpr s32 GNM_FAILURE = s32(0x8EEE00FF);
inline constexpr s32 GNM_INVALID = s32(0x80D11000); ///< bad submission arguments
inline constexpr s32 GNM_FLIP_QUEUE_FULL = s32(0x80D11081);
inline constexpr s32 GNM_VALIDATION_OFF = s32(0x80D13FFF);
inline constexpr s32 GNM_RAZOR_NOT_LOADED = s32(0x80D15001);
inline constexpr s32 GNM_CAPTURE_FAILED = s32(0x80D1500F);
inline constexpr s32 GNM_QUEUE_BAD_PIPE = s32(0x80D17000);
inline constexpr s32 GNM_QUEUE_BAD_QUEUE = s32(0x80D17001);
inline constexpr s32 GNM_QUEUE_BAD_RING_SIZE = s32(0x80D17002);
inline constexpr s32 GNM_QUEUE_BAD_RING_BASE = s32(0x80D17003);
inline constexpr s32 GNM_QUEUE_BAD_READ_PTR = s32(0x80D17004);

} // namespace BbErr
