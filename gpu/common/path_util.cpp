// SPDX-FileCopyrightText: Copyright 2026 bbport contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#include <array>
#include "shim/bb_env.h"
#include <cstdlib>
#include <mutex>

#include <limits.h>

#include "common/logging/log.h"
#include "common/path_util.h"

namespace Common::FS
{
namespace fs = std::filesystem;
namespace
{

constexpr std::size_t NUM_TYPES = std::size_t(PathType::CustomModulesDir) + 1;

/// Sub-folder of the user folder for each type ("" = the user folder itself).
constexpr std::array<const char*, NUM_TYPES> SUB_FOLDERS = {
    "",         "log",      "screenshots", "shader",  "temp",           "data",          "sys_modules",
    "download", "captures", "cheats",      "patches", "game_data",      "custom_trophy", "custom_configs",
    "cache",    "fonts",    "trophy",      "home",    "custom_modules",
};

struct Paths
{
    std::mutex mutex;
    std::array<fs::path, NUM_TYPES> paths;
    std::array<bool, NUM_TYPES> ready{};

    Paths()
    {
        const char* env = bb_env("BB_GPU_USER_DIR");
        const fs::path user = env != nullptr && *env ? fs::path{env} : fs::current_path() / "user";
        for (std::size_t i = 0; i < NUM_TYPES; ++i)
        {
            paths[i] = *SUB_FOLDERS[i] ? user / SUB_FOLDERS[i] : user;
        }
    }
};

Paths& Instance()
{
    static Paths paths;
    return paths;
}

} // namespace

bool ValidatePath(const fs::path& path)
{
    if (path.empty())
    {
        LOG_ERROR(Common_Filesystem, "empty path");
        return false;
    }
    if (path.native().size() >= PATH_MAX)
    {
        LOG_ERROR(Common_Filesystem, "path too long: {}", path.string());
        return false;
    }
    return true;
}

std::string PathToUTF8String(const fs::path& path)
{
    const std::u8string text = path.u8string();
    return std::string{text.begin(), text.end()};
}

const fs::path& GetUserPath(PathType type)
{
    Paths& state = Instance();
    const std::size_t index = std::size_t(type);
    std::scoped_lock lock{state.mutex};
    if (state.ready[index] == false)
    {
        std::error_code ec;
        fs::create_directories(state.paths[index], ec);
        state.ready[index] = true;
    }
    return state.paths[index];
}

std::string GetUserPathString(PathType type)
{
    return PathToUTF8String(GetUserPath(type));
}

void SetUserPath(PathType type, const fs::path& path)
{
    if (fs::is_directory(path) == false)
    {
        LOG_ERROR(Common_Filesystem, "not a directory: {}", path.string());
        return;
    }
    Paths& state = Instance();
    std::scoped_lock lock{state.mutex};
    state.paths[std::size_t(type)] = path;
    state.ready[std::size_t(type)] = true;
}

std::optional<fs::path> FindGameByID(const fs::path& dir, const std::string& game_id, int max_depth)
{
    if (max_depth < 0)
    {
        return std::nullopt;
    }
    const auto eboot_in = [](const fs::path& root) -> std::optional<fs::path>
    {
        std::error_code ec;
        if (fs::is_directory(root, ec) && fs::exists(root / "sce_sys" / "param.sfo", ec) &&
            fs::exists(root / "eboot.bin", ec))
        {
            return root / "eboot.bin";
        }
        return std::nullopt;
    };
    if (dir.filename() == game_id)
    {
        auto found = eboot_in(dir);
        if (found != std::nullopt)
        {
            return found;
        }
    }
    auto inside = eboot_in(dir / game_id);
    if (inside != std::nullopt)
    {
        return inside;
    }
    std::error_code ec;
    for (const std::filesystem::directory_entry& entry : fs::directory_iterator(dir, ec))
    {
        std::error_code entry_ec;
        if (entry.is_directory(entry_ec))
        {
            auto found = FindGameByID(entry.path(), game_id, max_depth - 1);
            if (found != std::nullopt)
            {
                return found;
            }
        }
    }
    return std::nullopt;
}

} // namespace Common::FS
