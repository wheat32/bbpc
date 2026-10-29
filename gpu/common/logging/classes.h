// SPDX-FileCopyrightText: Copyright 2026 bbport contributors
// SPDX-License-Identifier: GPL-2.0-or-later
// bbport: log classes (the part of the GPU library a message comes from).
#pragma once

namespace Common::Log::Class
{

constexpr const char* Common = "Common";
constexpr const char* Common_Filesystem = "Common.Filesystem";
constexpr const char* Common_Memory = "Common.Memory";
constexpr const char* Config = "Config";
constexpr const char* Core = "Core";
constexpr const char* Debug = "Debug";
constexpr const char* Frontend = "Frontend";
constexpr const char* ImGui = "ImGui";
constexpr const char* Input = "Input";
constexpr const char* Kernel = "Kernel";
constexpr const char* Kernel_Event = "Kernel.Event";
constexpr const char* KeyManager = "KeyManager";
constexpr const char* Lib = "Lib";
constexpr const char* Lib_AvPlayer = "Lib.AvPlayer";
constexpr const char* Lib_GnmDriver = "Lib.GnmDriver";
constexpr const char* Lib_Kernel = "Lib.Kernel";
constexpr const char* Lib_VideoOut = "Lib.VideoOut";
constexpr const char* Lib_Videodec = "Lib.Videodec";
constexpr const char* Loader = "Loader";
constexpr const char* Log = "Log";
constexpr const char* Render = "Render";
constexpr const char* Render_Recompiler = "Render.Recompiler";
constexpr const char* Render_Vulkan = "Render.Vulkan";
constexpr const char* Tty = "Tty";

// The online module (gpu/bbnet).
constexpr const char* Lib_Http = "Lib.Http";
constexpr const char* Lib_Http2 = "Lib.Http2";
constexpr const char* Lib_Net = "Lib.Net";
constexpr const char* Lib_NetCtl = "Lib.NetCtl";
constexpr const char* Lib_NpAuth = "Lib.NpAuth";
constexpr const char* Lib_NpCommerce = "Lib.NpCommerce";
constexpr const char* Lib_NpCommon = "Lib.NpCommon";
constexpr const char* Lib_NpManager = "Lib.NpManager";
constexpr const char* Lib_NpMatching = "Lib.NpMatching";
constexpr const char* Lib_NpMatching2 = "Lib.NpMatching2";
constexpr const char* Lib_NpPartner = "Lib.NpPartner";
constexpr const char* Lib_NpParty = "Lib.NpParty";
constexpr const char* Lib_NpProfileDialog = "Lib.NpProfileDialog";
constexpr const char* Lib_NpScore = "Lib.NpScore";
constexpr const char* Lib_NpSignaling = "Lib.NpSignaling";
constexpr const char* Lib_NpSnsFacebookDialog = "Lib.NpSnsFacebookDialog";
constexpr const char* Lib_NpTrophy = "Lib.NpTrophy";
constexpr const char* Lib_NpTus = "Lib.NpTus";
constexpr const char* Lib_NpWebApi = "Lib.NpWebApi";
constexpr const char* Lib_NpWebApi2 = "Lib.NpWebApi2";
constexpr const char* Lib_Ssl = "Lib.Ssl";
constexpr const char* Lib_Ssl2 = "Lib.Ssl2";
constexpr const char* NpHandler = "NpHandler";
constexpr const char* ShadNet = "ShadNet";

// More of shadPS4 0.20's classes (the online module, gpu/bbnet20).
constexpr const char* Core_Devices = "Core.Devices";
constexpr const char* Core_Linker = "Core.Linker";
constexpr const char* IPC = "IPC";
constexpr const char* Kernel_Fs = "Kernel.Fs";
constexpr const char* Kernel_Pthread = "Kernel.Pthread";
constexpr const char* Kernel_Sce = "Kernel.Sce";
constexpr const char* Kernel_Vmm = "Kernel.Vmm";
constexpr const char* Lib_Ajm = "Lib.Ajm";
constexpr const char* Lib_AppContent = "Lib.AppContent";
constexpr const char* Lib_Audio3d = "Lib.Audio3d";
constexpr const char* Lib_AudioIn = "Lib.AudioIn";
constexpr const char* Lib_AudioOut = "Lib.AudioOut";
constexpr const char* Lib_Camera = "Lib.Camera";
constexpr const char* Lib_CommonDlg = "Lib.CommonDlg";
constexpr const char* Lib_CompanionHttpd = "Lib.CompanionHttpd";
constexpr const char* Lib_CompanionUtil = "Lib.CompanionUtil";
constexpr const char* Lib_ContentExport = "Lib.ContentExport";
constexpr const char* Lib_DiscMap = "Lib.DiscMap";
constexpr const char* Lib_ErrorDialog = "Lib.ErrorDialog";
constexpr const char* Lib_Fiber = "Lib.Fiber";
constexpr const char* Lib_Font = "Lib.Font";
constexpr const char* Lib_FontFt = "Lib.FontFt";
constexpr const char* Lib_GameLiveStreaming = "Lib.GameLiveStreaming";
constexpr const char* Lib_Hmd = "Lib.Hmd";
constexpr const char* Lib_HmdSetupDialog = "Lib.HmdSetupDialog";
constexpr const char* Lib_Ime = "Lib.Ime";
constexpr const char* Lib_ImeDialog = "Lib.ImeDialog";
constexpr const char* Lib_InvitationDialog = "Lib.InvitationDialog";
constexpr const char* Lib_Jpeg = "Lib.Jpeg";
constexpr const char* Lib_LibcInternal = "Lib.LibcInternal";
constexpr const char* Lib_Mouse = "Lib.Mouse";
constexpr const char* Lib_Move = "Lib.Move";
constexpr const char* Lib_MsgDlg = "Lib.MsgDlg";
constexpr const char* Lib_Ngs2 = "Lib.Ngs2";
constexpr const char* Lib_NpUtility = "Lib.NpUtility";
constexpr const char* Lib_Pad = "Lib.Pad";
constexpr const char* Lib_PlayGo = "Lib.PlayGo";
constexpr const char* Lib_PlayGoDialog = "Lib.PlayGoDialog";
constexpr const char* Lib_Png = "Lib.Png";
constexpr const char* Lib_Random = "Lib.Random";
constexpr const char* Lib_RazorCpu = "Lib.RazorCpu";
constexpr const char* Lib_Remoteplay = "Lib.Remoteplay";
constexpr const char* Lib_Rtc = "Lib.Rtc";
constexpr const char* Lib_Rudp = "Lib.Rudp";
constexpr const char* Lib_SaveData = "Lib.SaveData";
constexpr const char* Lib_SaveDataDialog = "Lib.SaveDataDialog";
constexpr const char* Lib_Screenshot = "Lib.Screenshot";
constexpr const char* Lib_SharePlay = "Lib.SharePlay";
constexpr const char* Lib_SigninDialog = "Lib.SigninDialog";
constexpr const char* Lib_SysModule = "Lib.SysModule";
constexpr const char* Lib_SystemGesture = "Lib.SystemGesture";
constexpr const char* Lib_SystemService = "Lib.SystemService";
constexpr const char* Lib_Usbd = "Lib.Usbd";
constexpr const char* Lib_UserService = "Lib.UserService";
constexpr const char* Lib_Vdec2 = "Lib.Vdec2";
constexpr const char* Lib_Vdecsw = "Lib.Vdecsw";
constexpr const char* Lib_VideoRecording = "Lib.VideoRecording";
constexpr const char* Lib_Voice = "Lib.Voice";
constexpr const char* Lib_VrTracker = "Lib.VrTracker";
constexpr const char* Lib_WebBrowserDialog = "Lib.WebBrowserDialog";
constexpr const char* Lib_Zlib = "Lib.Zlib";

} // namespace Common::Log::Class
