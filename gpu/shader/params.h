// SPDX-License-Identifier: GPL-2.0-or-later
// bbport shader translator: what identifies a guest shader program to compile.
#pragma once

#include <span>

#include "common/types.h"

namespace Shader
{

struct ShaderParams
{
    static constexpr u32 NUM_SHADER_USER_DATA = 16;

    std::span<const u32, NUM_SHADER_USER_DATA> user_data;
    std::span<const u32> code;
    u64 hash;

    VAddr Base() const noexcept { return reinterpret_cast<VAddr>(code.data()); }
};

} // namespace Shader
