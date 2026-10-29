// SPDX-FileCopyrightText: Copyright 2026 bbport contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#include <algorithm>
#include "shim/bb_env.h"
#include <cctype>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string_view>

#include "common/logging/log.h"
#include "core/libraries/libs.h"
#include "platform/avplayer/avplayer.h"
#include "platform/avplayer/player.h"
#include "platform/errors.h"

namespace BbAvPlayer
{

namespace
{

constexpr s32 ERROR_INVALID_PARAMS = s32(0x806A0001);
constexpr s32 ERROR_OPERATION_FAILED = s32(0x806A0002);
constexpr s32 ERROR_NOT_SUPPORTED = s32(0x806A0004);

using Handle = Player*;

/// BB_AVPLAYER_TRACE=1 (diagnostics): the game's calls in order, with their results and the time
/// since the first (the opening movie's crash on Windows, 2026-10-27).
void Trace(const char* format, ...)
{
    static const bool on = bb_env_on("BB_AVPLAYER_TRACE");
    if (on == false)
    {
        return;
    }
    static const auto start = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    char text[256];
    va_list args;
    va_start(args, format);
    std::vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    std::printf("AvPlayer trace %.1f ms: %s\n", ms, text);
}

/// The player that plays now (the game has one at a time): a key press stops it (bbgpu_video_skip).
std::mutex active_mutex;
Player* active_player = nullptr;

void NoteCreated(Player* player)
{
    std::scoped_lock lock{active_mutex};
    active_player = player;
}

/// From the file name's extension (a URI's query and fragment ignored).
AvPlayerSourceType SourceTypeOf(std::string_view path)
{
    if (path.find("://") != std::string_view::npos)
    {
        path = path.substr(0, path.find_first_of("?#"));
    }
    const std::size_t dot = path.rfind('.');
    if (dot == std::string_view::npos)
    {
        return AvPlayerSourceType::Unknown;
    }
    std::string_view extension = path.substr(dot);
    extension = extension.substr(0, extension.find('/'));
    const auto is = [extension](std::string_view name)
    {
        return std::ranges::equal(extension, name,
                                  [](char a, char b)
                                  {
                                      return std::tolower(u8(a)) == std::tolower(u8(b));
                                  });
    };
    if (is(".mp4") || is(".m4v") || is(".m3d") || is(".m4a") || is(".mov"))
    {
        return AvPlayerSourceType::FileMp4;
    }
    return is(".m3u8") ? AvPlayerSourceType::Hls : AvPlayerSourceType::Unknown;
}

bool HasAllocators(const AvPlayerMemAllocator& allocator)
{
    return allocator.allocate != nullptr && allocator.deallocate != nullptr && allocator.allocate_texture != nullptr &&
           allocator.deallocate_texture != nullptr;
}

s32 Result(bool ok)
{
    return ok ? BbErr::OK : ERROR_OPERATION_FAILED;
}

s32 AddSource(Handle player, std::string_view path, AvPlayerSourceType type)
{
    if (type == AvPlayerSourceType::Unknown)
    {
        type = SourceTypeOf(path);
    }
    if (type == AvPlayerSourceType::Hls)
    {
        LOG_ERROR(Lib_AvPlayer, "HTTP live streaming is not supported");
        return ERROR_NOT_SUPPORTED;
    }
    return Result(player->AddSource(path));
}

Handle PS4_SYSV_ABI sceAvPlayerInit(AvPlayerInitData* data)
{
    if (data == nullptr || HasAllocators(data->memory_replacement) == false)
    {
        LOG_ERROR(Lib_AvPlayer, "sceAvPlayerInit needs all four allocators");
        return nullptr;
    }
    BbAvPlayer::Player* player = new Player(*data);
    NoteCreated(player);
    Trace("Init -> %p (auto start %d, events %d)", (void*)player, int(data->auto_start),
          data->event_replacement.event_callback != nullptr);
    return player;
}

s32 PS4_SYSV_ABI sceAvPlayerInitEx(const AvPlayerInitDataEx* data, Handle* player)
{
    if (data == nullptr || player == nullptr || HasAllocators(data->memory_replacement) == false)
    {
        return ERROR_INVALID_PARAMS;
    }
    AvPlayerInitData init{};
    init.memory_replacement = data->memory_replacement;
    init.file_replacement = data->file_replacement;
    init.event_replacement = data->event_replacement;
    init.default_language = data->default_language;
    init.num_output_video_framebuffers = data->num_output_video_framebuffers;
    init.auto_start = data->auto_start;
    *player = new Player(init);
    NoteCreated(*player);
    Trace("InitEx -> %p (auto start %d)", (void*)*player, int(data->auto_start));
    return BbErr::OK;
}

s32 PS4_SYSV_ABI sceAvPlayerPostInit(Handle player, AvPlayerPostInitData* data)
{
    return player != nullptr && data != nullptr ? BbErr::OK : ERROR_INVALID_PARAMS; // decoder choices: FFmpeg's
}

s32 PS4_SYSV_ABI sceAvPlayerClose(Handle player)
{
    if (player == nullptr)
    {
        return ERROR_INVALID_PARAMS;
    }
    {
        std::scoped_lock lock{active_mutex};
        if (active_player == player)
        {
            active_player = nullptr;
        }
    }
    Trace("Close %p", (void*)player);
    delete player;
    return BbErr::OK;
}

s32 PS4_SYSV_ABI sceAvPlayerAddSource(Handle player, const char* path)
{
    if (player == nullptr || path == nullptr)
    {
        return ERROR_INVALID_PARAMS;
    }
    const s32 result = AddSource(player, path, AvPlayerSourceType::Unknown);
    Trace("AddSource %p %s -> %#x", (void*)player, path, unsigned(result));
    return result;
}

s32 PS4_SYSV_ABI sceAvPlayerAddSourceEx(Handle player, AvPlayerUriType uri_type, AvPlayerSourceDetails* details)
{
    if (player == nullptr || uri_type != AvPlayerUriType::Source || details == nullptr)
    {
        return ERROR_INVALID_PARAMS;
    }
    return AddSource(player, std::string_view(details->uri.name, details->uri.length), details->source_type);
}

s32 PS4_SYSV_ABI sceAvPlayerStreamCount(Handle player)
{
    if (player == nullptr)
    {
        return ERROR_INVALID_PARAMS;
    }
    const s32 count = player->StreamCount();
    Trace("StreamCount %p -> %d", (void*)player, count);
    return count < 0 ? ERROR_OPERATION_FAILED : count;
}

s32 PS4_SYSV_ABI sceAvPlayerGetStreamInfo(Handle player, u32 index, AvPlayerStreamInfo* info)
{
    if (player == nullptr || info == nullptr)
    {
        return ERROR_INVALID_PARAMS;
    }
    const s32 result = Result(player->StreamInfo(index, *info));
    const char* language = info->type == AvPlayerStreamType::Audio   ? info->details.audio.language_code
                           : info->type == AvPlayerStreamType::Video ? info->details.video.language_code
                                                                     : info->details.subs.language_code;
    Trace("GetStreamInfo %p %u -> %#x: type %d, duration %llu, language '%.4s'", (void*)player, index, unsigned(result),
          int(info->type), (unsigned long long)info->duration, language);
    return result;
}

s32 PS4_SYSV_ABI sceAvPlayerEnableStream(Handle player, u32 index)
{
    const s32 result = player != nullptr ? Result(player->EnableStream(index)) : ERROR_INVALID_PARAMS;
    Trace("EnableStream %p %u -> %#x", (void*)player, index, unsigned(result));
    return result;
}

/// Not supported: every enabled stream plays.
s32 PS4_SYSV_ABI sceAvPlayerDisableStream(Handle player, u32)
{
    return player != nullptr ? BbErr::OK : ERROR_INVALID_PARAMS;
}

s32 PS4_SYSV_ABI sceAvPlayerChangeStream()
{
    return BbErr::OK;
}

s32 PS4_SYSV_ABI sceAvPlayerStart(Handle player)
{
    const s32 result = player != nullptr ? Result(player->Start()) : ERROR_INVALID_PARAMS;
    Trace("Start %p -> %#x", (void*)player, unsigned(result));
    return result;
}

s32 PS4_SYSV_ABI sceAvPlayerStop(Handle player)
{
    const s32 result = player != nullptr ? Result(player->Stop()) : ERROR_INVALID_PARAMS;
    Trace("Stop %p -> %#x", (void*)player, unsigned(result));
    return result;
}

s32 PS4_SYSV_ABI sceAvPlayerPause(Handle player)
{
    return player != nullptr ? Result(player->Pause()) : ERROR_INVALID_PARAMS;
}

s32 PS4_SYSV_ABI sceAvPlayerResume(Handle player)
{
    return player != nullptr ? Result(player->Resume()) : ERROR_INVALID_PARAMS;
}

/// Not supported: playback goes on from where it is.
s32 PS4_SYSV_ABI sceAvPlayerJumpToTime(Handle player, u64 time_ms)
{
    LOG_WARNING(Lib_AvPlayer, "jumping (to {} ms) is not supported", time_ms);
    return player != nullptr ? BbErr::OK : ERROR_INVALID_PARAMS;
}

s32 PS4_SYSV_ABI sceAvPlayerSetTrickSpeed(Handle player, s32 speed)
{
    LOG_WARNING(Lib_AvPlayer, "trick speed {} is not supported", speed);
    return player != nullptr ? BbErr::OK : ERROR_INVALID_PARAMS;
}

s32 PS4_SYSV_ABI sceAvPlayerSetAvSyncMode(Handle player, AvPlayerAvSyncMode mode)
{
    if (player == nullptr)
    {
        return ERROR_INVALID_PARAMS;
    }
    player->SetSyncMode(mode);
    return BbErr::OK;
}

s32 PS4_SYSV_ABI sceAvPlayerSetLooping(Handle player, bool looping)
{
    return player != nullptr ? Result(player->SetLooping(looping)) : ERROR_INVALID_PARAMS;
}

bool PS4_SYSV_ABI sceAvPlayerGetAudioData(Handle player, AvPlayerFrameInfo* info)
{
    return player != nullptr && info != nullptr && player->GetAudioData(*info);
}

bool PS4_SYSV_ABI sceAvPlayerGetVideoData(Handle player, AvPlayerFrameInfo* info)
{
    return player != nullptr && info != nullptr && player->GetVideoData(*info);
}

bool PS4_SYSV_ABI sceAvPlayerGetVideoDataEx(Handle player, AvPlayerFrameInfoEx* info)
{
    return player != nullptr && info != nullptr && player->GetVideoData(*info);
}

bool PS4_SYSV_ABI sceAvPlayerIsActive(Handle player)
{
    const bool active = player != nullptr && player->IsActive();
    // Said when it changes (the game asks every frame).
    static Handle last_player = nullptr;
    static int last = -1;
    if (player != last_player || int(active) != last)
    {
        last_player = player;
        last = int(active);
        Trace("IsActive %p -> %d", (void*)player, int(active));
    }
    return active;
}

u64 PS4_SYSV_ABI sceAvPlayerCurrentTime(Handle player)
{
    return player != nullptr ? player->CurrentTime() : u64(s64(ERROR_INVALID_PARAMS));
}

/// The library's own log goes nowhere here.
s32 PS4_SYSV_ABI sceAvPlayerSetLogCallback(AvPlayerLogCallback, void*)
{
    return BbErr::OK;
}

s32 PS4_SYSV_ABI sceAvPlayerVprintf(const char*, va_list)
{
    return BbErr::OK;
}

} // namespace

void RegisterLib(Core::Loader::SymbolsResolver* sym)
{
    const auto fn = [](auto* function)
    {
        return reinterpret_cast<u64>(function);
    };
    const std::pair<const char*, u64> exports[] = {
        {"KMcEa+rHsIo", fn(sceAvPlayerAddSource)},      {"x8uvuFOPZhU", fn(sceAvPlayerAddSourceEx)},
        {"buMCiJftcfw", fn(sceAvPlayerChangeStream)},   {"NkJwDzKmIlw", fn(sceAvPlayerClose)},
        {"wwM99gjFf1Y", fn(sceAvPlayerCurrentTime)},    {"BOVKAzRmuTQ", fn(sceAvPlayerDisableStream)},
        {"ODJK2sn9w4A", fn(sceAvPlayerEnableStream)},   {"Wnp1OVcrZgk", fn(sceAvPlayerGetAudioData)},
        {"d8FcbzfAdQw", fn(sceAvPlayerGetStreamInfo)},  {"o3+RWnHViSg", fn(sceAvPlayerGetVideoData)},
        {"JdksQu8pNdQ", fn(sceAvPlayerGetVideoDataEx)}, {"aS66RI0gGgo", fn(sceAvPlayerInit)},
        {"o9eWRkSL+M4", fn(sceAvPlayerInitEx)},         {"UbQoYawOsfY", fn(sceAvPlayerIsActive)},
        {"XC9wM+xULz8", fn(sceAvPlayerJumpToTime)},     {"9y5v+fGN4Wk", fn(sceAvPlayerPause)},
        {"HD1YKVU26-M", fn(sceAvPlayerPostInit)},       {"w5moABNwnRY", fn(sceAvPlayerResume)},
        {"k-q+xOxdc3E", fn(sceAvPlayerSetAvSyncMode)},  {"eBTreZ84JFY", fn(sceAvPlayerSetLogCallback)},
        {"OVths0xGfho", fn(sceAvPlayerSetLooping)},     {"av8Z++94rs0", fn(sceAvPlayerSetTrickSpeed)},
        {"ET4Gr-Uu07s", fn(sceAvPlayerStart)},          {"ZC17w3vB5Lo", fn(sceAvPlayerStop)},
        {"hdTyRzCXQeQ", fn(sceAvPlayerStreamCount)},    {"yN7Jhuv8g24", fn(sceAvPlayerVprintf)},
    };
    for (const auto& [nid, address] : exports)
    {
        sym->AddSymbol(nid, "libSceAvPlayer", "libSceAvPlayer", Core::Loader::SymbolType::Function, address);
    }
}

} // namespace BbAvPlayer

// A video playing now (the opening movie, the endings): the keyboard's Enter and Escape and the
// gamepad's Options stop it (src/runtime_pad.c). From the SrShadowy fork.
extern "C" int bbgpu_video_active(void)
{
    std::scoped_lock lock{BbAvPlayer::active_mutex};
    return BbAvPlayer::active_player != nullptr && BbAvPlayer::active_player->IsActive() ? 1 : 0;
}

extern "C" int bbgpu_video_skip(void)
{
    std::scoped_lock lock{BbAvPlayer::active_mutex};
    if (BbAvPlayer::active_player == nullptr || BbAvPlayer::active_player->IsActive() == false)
    {
        return 0;
    }
    LOG_INFO(Lib_AvPlayer, "video skipped by the player");
    return BbAvPlayer::active_player->Stop() ? 1 : 0;
}
