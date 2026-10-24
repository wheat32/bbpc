// SPDX-FileCopyrightText: Copyright 2026 IFreemz (shadps4_dlss bridge), bbport contributors
// SPDX-License-Identifier: MIT
//
// libbbport_dlss.so / bbport_dlss.dll: the only part of the project that uses the NVIDIA DLSS
// (NGX) SDK; the port loads it at run time. Linux: build.sh with DLSS_SDK_ROOT; Windows: MSVC
// (the SDK's Windows libraries are MSVC only), see CMakeLists.txt.
// Adapted from IFreemz/shadPS4-Bloodborne-DLSS-FSR (dlss_bridge).

#include <algorithm>
#include <array>
#include <cstdio>
#include <string>

#include "bbport_dlss_bridge.h" // Vulkan first: the NGX headers expect its types

#include <nvsdk_ngx_helpers.h>
#include <nvsdk_ngx_vk.h>
#include <nvsdk_ngx_helpers_vk.h>

namespace
{

constexpr char PROJECT_ID[] = "42359704-c9f3-4806-9fb5-d469000fdb8a";
#ifdef _WIN32
constexpr char ENGINE_VERSION[] = "bbport-windows";
#else
constexpr char ENGINE_VERSION[] = "bbport-linux";
#endif

// The bridge's answers (BbDlssApi): 1 done, 0 not.
constexpr int32_t BRIDGE_DONE = 1;
constexpr int32_t BRIDGE_FAILED = 0;
// BbDlssLogFn's first argument.
constexpr int MESSAGE_INFO = 0;
constexpr int MESSAGE_WARNING = 1;
constexpr size_t LOG_TEXT_SIZE = 1024;
// One GPU: NGX's creation and visibility node masks.
constexpr unsigned NODE_MASK = 1;
// Motion vectors come in render-resolution pixels; the frame is not pre-exposed.
constexpr float MOTION_SCALE = 1.0f;
constexpr float EXPOSURE_SCALE = 1.0f;
// BbDlssFeature::quality 0..4: DLAA, Quality, Balanced, Performance, Ultra Performance.
constexpr std::array QUALITIES{NVSDK_NGX_PerfQuality_Value_DLAA, NVSDK_NGX_PerfQuality_Value_MaxQuality,
                               NVSDK_NGX_PerfQuality_Value_Balanced, NVSDK_NGX_PerfQuality_Value_MaxPerf,
                               NVSDK_NGX_PerfQuality_Value_UltraPerformance};

struct State
{
    BbDlssLogFn log{};
    std::wstring dll_directory;
    std::wstring data_directory;
    const wchar_t* dll_path{};
    NVSDK_NGX_FeatureCommonInfo common{};
    NVSDK_NGX_FeatureDiscoveryInfo discovery{};
    NVSDK_NGX_Parameter* capabilities{};
    NVSDK_NGX_Parameter* parameters{};
    NVSDK_NGX_Handle* feature{};
    BbDlssFeature feature_desc{};
    VkDevice device{};
    bool configured{};
    bool initialized{};
} state;

template <typename... Args>
void Log(int warning, const char* format, Args... args)
{
    if (state.log == nullptr) return;
    if constexpr (sizeof...(Args) == 0)
    {
        state.log(warning, format); // a plain message (no format string without arguments)
    }
    else
    {
        std::array<char, LOG_TEXT_SIZE> text{};
        std::snprintf(text.data(), text.size(), format, args...);
        state.log(warning, text.data());
    }
}

bool Check(const char* operation, NVSDK_NGX_Result result)
{
    if (NVSDK_NGX_FAILED(result))
    {
        Log(MESSAGE_WARNING, "%s failed: 0x%08x", operation, static_cast<unsigned>(result));
        return false;
    }
    return true;
}

void NVSDK_CONV NgxLog(const char* message, NVSDK_NGX_Logging_Level, NVSDK_NGX_Feature)
{
    Log(MESSAGE_INFO, "NGX: %s", message != nullptr ? message : "");
}

int32_t Configure(const wchar_t* dll_directory, const wchar_t* data_directory, BbDlssLogFn log)
{
    state.log = log;
    state.dll_directory = dll_directory != nullptr ? dll_directory : L".";
    state.data_directory = data_directory != nullptr ? data_directory : L".";
    state.dll_path = state.dll_directory.c_str();
    state.common.PathListInfo = {&state.dll_path, 1};
    state.common.LoggingInfo = {NgxLog, NVSDK_NGX_LOGGING_LEVEL_OFF, false};
    state.discovery.SDKVersion = NVSDK_NGX_Version_API;
    state.discovery.FeatureID = NVSDK_NGX_Feature_SuperSampling;
    state.discovery.Identifier.IdentifierType = NVSDK_NGX_Application_Identifier_Type_Project_Id;
    state.discovery.Identifier.v.ProjectDesc = {PROJECT_ID, NVSDK_NGX_ENGINE_TYPE_CUSTOM, ENGINE_VERSION};
    state.discovery.ApplicationDataPath = state.data_directory.c_str();
    state.discovery.FeatureInfo = &state.common;
    state.configured = true;
    return BRIDGE_DONE;
}

int32_t InstanceExtensions(uint32_t* count, const VkExtensionProperties** extensions)
{
    if (state.configured == false)
    {
        return BRIDGE_FAILED;
    }
    VkExtensionProperties* required = nullptr;
    const bool queried =
        Check("Instance extension query",
              NVSDK_NGX_VULKAN_GetFeatureInstanceExtensionRequirements(&state.discovery, count, &required));
    if (queried == false)
    {
        return BRIDGE_FAILED;
    }
    *extensions = required;
    return BRIDGE_DONE;
}

int32_t DeviceExtensions(VkInstance instance, VkPhysicalDevice physical, uint32_t* count,
                         const VkExtensionProperties** extensions)
{
    if (state.configured == false)
    {
        return BRIDGE_FAILED;
    }
    NVSDK_NGX_FeatureRequirement support{};
    const bool supported_queried =
        Check("Feature requirement query",
              NVSDK_NGX_VULKAN_GetFeatureRequirements(instance, physical, &state.discovery, &support));
    if (supported_queried == false)
    {
        return BRIDGE_FAILED;
    }
    if (support.FeatureSupported != NVSDK_NGX_FeatureSupportResult_Supported)
    {
        Log(MESSAGE_WARNING, "DLSS is not supported on this GPU or driver (code 0x%x)",
            static_cast<unsigned>(support.FeatureSupported));
        return BRIDGE_FAILED;
    }
    VkExtensionProperties* required = nullptr;
    const bool extensions_queried = Check(
        "Device extension query",
        NVSDK_NGX_VULKAN_GetFeatureDeviceExtensionRequirements(instance, physical, &state.discovery, count, &required));
    if (extensions_queried == false)
    {
        return BRIDGE_FAILED;
    }
    *extensions = required;
    return BRIDGE_DONE;
}

int32_t Initialize(VkInstance instance, VkPhysicalDevice physical, VkDevice device,
                   PFN_vkGetInstanceProcAddr get_instance_proc, PFN_vkGetDeviceProcAddr get_device_proc)
{
    if (state.configured == false || state.initialized)
    {
        return BRIDGE_FAILED;
    }
    const bool initialized =
        Check("NGX initialization",
              NVSDK_NGX_VULKAN_Init_with_ProjectID(PROJECT_ID, NVSDK_NGX_ENGINE_TYPE_CUSTOM, ENGINE_VERSION,
                                                   state.data_directory.c_str(), instance, physical, device,
                                                   get_instance_proc, get_device_proc, &state.common));
    if (initialized == false)
    {
        return BRIDGE_FAILED;
    }
    state.initialized = true;
    state.device = device;
    if (Check("Capability query", NVSDK_NGX_VULKAN_GetCapabilityParameters(&state.capabilities)) == false)
    {
        return BRIDGE_FAILED;
    }
    int available = 0;
    int needs_driver = 0;
    NVSDK_NGX_Parameter_GetI(state.capabilities, NVSDK_NGX_Parameter_SuperSampling_Available, &available);
    NVSDK_NGX_Parameter_GetI(state.capabilities, NVSDK_NGX_Parameter_SuperSampling_NeedsUpdatedDriver, &needs_driver);
    if (needs_driver != 0)
    {
        Log(MESSAGE_WARNING, "DLSS needs a newer NVIDIA driver");
        return BRIDGE_FAILED;
    }
    if (available == 0)
    {
        Log(MESSAGE_WARNING, "DLSS Super Resolution is not available on this system");
        return BRIDGE_FAILED;
    }
    return BRIDGE_DONE;
}

void ReleaseFeature()
{
    if (state.feature != nullptr)
    {
        NVSDK_NGX_VULKAN_ReleaseFeature(state.feature);
        state.feature = nullptr;
    }
    if (state.parameters != nullptr)
    {
        NVSDK_NGX_VULKAN_DestroyParameters(state.parameters);
        state.parameters = nullptr;
    }
}

int32_t CreateFeature(VkCommandBuffer command, const BbDlssFeature* desc)
{
    if (state.initialized == false || state.capabilities == nullptr || desc == nullptr)
    {
        return BRIDGE_FAILED;
    }
    ReleaseFeature();
    if (Check("Parameter allocation", NVSDK_NGX_VULKAN_AllocateParameters(&state.parameters)) == false)
    {
        return BRIDGE_FAILED;
    }
    for (const char* key :
         {NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality,
          NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance,
          NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance,
          NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraQuality})
    {
        NVSDK_NGX_Parameter_SetUI(state.parameters, key, desc->preset);
    }
    NVSDK_NGX_DLSS_Create_Params settings{};
    settings.Feature.InWidth = desc->input_width;
    settings.Feature.InHeight = desc->input_height;
    settings.Feature.InTargetWidth = desc->output_width;
    settings.Feature.InTargetHeight = desc->output_height;
    settings.Feature.InPerfQualityValue = QUALITIES[std::clamp(desc->quality, 0, int32_t(QUALITIES.size()) - 1)];
    settings.InFeatureCreateFlags =
        NVSDK_NGX_DLSS_Feature_Flags_MVLowRes |
        (desc->depth_inverted ? NVSDK_NGX_DLSS_Feature_Flags_DepthInverted : 0) |
        (desc->hdr ? NVSDK_NGX_DLSS_Feature_Flags_IsHDR | NVSDK_NGX_DLSS_Feature_Flags_AutoExposure : 0);
    const NVSDK_NGX_Result result = NGX_VULKAN_CREATE_DLSS_EXT1(state.device, command, NODE_MASK, NODE_MASK,
                                                                &state.feature, state.parameters, &settings);
    if (Check("DLSS feature creation", result) == false || state.feature == nullptr)
    {
        state.feature = nullptr;
        return BRIDGE_FAILED;
    }
    state.feature_desc = *desc;
    Log(MESSAGE_INFO, "DLSS %ux%u -> %ux%u, quality %d, preset %u%s", desc->input_width, desc->input_height,
        desc->output_width, desc->output_height, desc->quality, desc->preset, desc->hdr != 0 ? ", HDR input" : "");
    return BRIDGE_DONE;
}

NVSDK_NGX_Resource_VK Wrap(const BbDlssImage& image, bool writable)
{
    return NVSDK_NGX_Create_ImageView_Resource_VK(image.view, image.image, image.range, image.format, image.width,
                                                  image.height, writable);
}

int32_t Evaluate(VkCommandBuffer command, const BbDlssEvaluate* evaluate)
{
    if (state.feature == nullptr || state.parameters == nullptr || evaluate == nullptr)
    {
        return BRIDGE_FAILED;
    }
    NVSDK_NGX_Resource_VK color = Wrap(evaluate->color, false);
    NVSDK_NGX_Resource_VK depth = Wrap(evaluate->depth, false);
    NVSDK_NGX_Resource_VK motion = Wrap(evaluate->motion, false);
    NVSDK_NGX_Resource_VK output = Wrap(evaluate->output, true);
    NVSDK_NGX_VK_DLSS_Eval_Params parameters{};
    parameters.Feature.pInColor = &color;
    parameters.Feature.pInOutput = &output;
    parameters.Feature.InSharpness = evaluate->sharpness;
    parameters.pInDepth = &depth;
    parameters.pInMotionVectors = &motion;
    parameters.InRenderSubrectDimensions = {state.feature_desc.input_width, state.feature_desc.input_height};
    parameters.InJitterOffsetX = evaluate->jitter_x;
    parameters.InJitterOffsetY = evaluate->jitter_y;
    parameters.InReset = evaluate->reset;
    parameters.InMVScaleX = parameters.InMVScaleY = MOTION_SCALE;
    parameters.InPreExposure = parameters.InExposureScale = EXPOSURE_SCALE;
    parameters.InFrameTimeDeltaInMsec = evaluate->frame_ms;
    const bool evaluated =
        Check("DLSS evaluation", NGX_VULKAN_EVALUATE_DLSS_EXT(command, state.feature, state.parameters, &parameters));
    return evaluated ? BRIDGE_DONE : BRIDGE_FAILED;
}

void Shutdown()
{
    ReleaseFeature();
    if (state.capabilities != nullptr)
    {
        NVSDK_NGX_VULKAN_DestroyParameters(state.capabilities);
        state.capabilities = nullptr;
    }
    if (state.initialized)
    {
        NVSDK_NGX_VULKAN_Shutdown1(state.device);
        state.initialized = false;
        state.device = VK_NULL_HANDLE;
    }
}

const BbDlssApi api{BBPORT_DLSS_BRIDGE_ABI, Configure, InstanceExtensions, DeviceExtensions, Initialize,
                    CreateFeature,          Evaluate,  ReleaseFeature,     Shutdown};

} // namespace

#ifdef _WIN32
#define BBPORT_DLSS_EXPORT __declspec(dllexport)
#else
#define BBPORT_DLSS_EXPORT __attribute__((visibility("default")))
#endif

extern "C" BBPORT_DLSS_EXPORT const BbDlssApi* BbDlssGetApi()
{
    return &api;
}
