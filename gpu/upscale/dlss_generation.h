// SPDX-License-Identifier: GPL-2.0-or-later
// bbport: the DLSS generation players know ("DLSS 4.5") that NVIDIA's Super Resolution library
// runs, from its version (libnvidia-ngx-dlss.so.<version>) and the quality mode. The bridge asks
// for the default preset; what that is comes from the SDK's own headers (nvsdk_ngx_defs.h,
// NVSDK_NGX_DLSS_Hint_Render_Preset, at each release):
//   310.5 and later  K (transformer: DLSS 4) for DLAA, Quality and Balanced; M and L (second
//                    transformer generation: DLSS 4.5) for Performance and Ultra Performance
//   310.3, 310.4     K for DLAA to Performance; F (CNN: DLSS 3) for Ultra Performance
//   310.1, 310.2     the transformer in beta, not the default: the CNN (DLSS 3)
//   3.x, 2.x, 1.x    DLSS 3, 2 and 1
// A newer library than these is not named: its defaults are not known here.
// Header only: the launcher names it too (launcher/core/upscalerAssets.cpp).
#pragma once

#include <array>
#include <charconv>
#include <string_view>
#include <system_error>

namespace DlssGeneration
{

/// Quality modes as Vulkan::Dlss::QualityForScale numbers them (0 DLAA .. 4 Ultra Performance).
inline constexpr int QUALITY_PERFORMANCE = 3;
inline constexpr int QUALITY_ULTRA_PERFORMANCE = 4;
/// The DLSS 4 SDK's major version, and the minor versions that changed the default preset.
inline constexpr int TRANSFORMER_MAJOR = 310;
inline constexpr int TRANSFORMER_DEFAULT_MINOR = 3;
inline constexpr int SECOND_TRANSFORMER_MINOR = 5;
/// Before the transformer: the library's major version is its generation.
inline constexpr std::array<std::string_view, 4> CNN_GENERATIONS{"", "1", "2", "3"};

/// The first two numbers of a version ("310.9.16": 310 and 9); zero where there is none.
inline std::array<int, 2> MajorMinor(std::string_view version)
{
    std::array<int, 2> parts{};
    const char* at = version.data();
    const char* const end = version.data() + version.size();
    for (int& part : parts)
    {
        const auto [next, error] = std::from_chars(at, end, part);
        if (error != std::errc{}) break;
        at = next != end && *next == '.' ? next + 1 : end;
    }
    return parts;
}

/// "4.5", "4", "3", "2" or "1"; empty for a version not known here.
inline std::string_view Of(std::string_view library_version, int quality)
{
    const auto [major, minor] = MajorMinor(library_version);
    if (major > 0 && major < int(CNN_GENERATIONS.size()))
    {
        return CNN_GENERATIONS[major];
    }
    if (major != TRANSFORMER_MAJOR)
    {
        return {};
    }
    if (minor < TRANSFORMER_DEFAULT_MINOR)
    {
        return "3";
    }
    if (minor < SECOND_TRANSFORMER_MINOR)
    {
        return quality == QUALITY_ULTRA_PERFORMANCE ? "3" : "4";
    }
    return quality >= QUALITY_PERFORMANCE ? "4.5" : "4";
}

} // namespace DlssGeneration
