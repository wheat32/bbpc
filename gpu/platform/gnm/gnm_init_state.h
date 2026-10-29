// SPDX-FileCopyrightText: Copyright 2026 bbport contributors
// SPDX-License-Identifier: GPL-2.0-or-later
// bbport: the register state the Gnm driver gives the GPU before a game's first draw, by SDK
// generation (the driver changed its defaults in 1.75, 2.00 and 3.50), and the state it resets a
// context to. Values are what the console's driver writes; packets are built at compile time.
// Register offsets: SH from 0x2C00, context from 0xA000, uconfig from 0xC000 (dword indices).
#pragma once

#include "platform/gnm/gnm_pm4.h"

namespace BbGnm::InitState
{

using namespace Pm4;

/// Driver generations whose defaults differ.
enum class Generation
{
    Base,
    Sdk175,
    Sdk200,
    Sdk350
};

/// The size of the packets sceGnmDraw/DispatchInitDefaultHardwareState write.
inline constexpr u32 HW_INIT_WORDS = 0x100;

inline constexpr auto CONTEXT_RESET =
    Join(Packet(Op::ContextControl, 0x80000000u, 0x80000000u), Packet(Op::ClearState, 0u));
/// CONTEXT_CONTROL, CLEAR_STATE, then ACQUIRE_MEM of everything: how the driver starts the state.
inline constexpr auto CLEAR_AND_SYNC =
    Join(CONTEXT_RESET, Packet(Op::AcquireMem, 0x2EC47FC0u, 0xFFFFFFFFu, 0u, 0u, 0u, 10u));

inline constexpr auto COMPUTE_DEFAULTS = Join( //
    Sh(0x216, 0xFFFFFFFFu),                    // COMPUTE_STATIC_THREAD_MGMT_SE0
    Sh(0x217, 0xFFFFFFFFu),                    // COMPUTE_STATIC_THREAD_MGMT_SE1
    Sh(0x215, 0u));                            // COMPUTE_RESOURCE_LIMITS

inline constexpr auto RASTER_DEFAULTS = Join( //
    Ctx(0x2F9, 0x2Du),                        // PA_SU_VTX_CNTL
    Ctx(0x282, 8u),                           // PA_SU_LINE_CNTL
    Ctx(0x280, 0x80008u),                     // PA_SU_POINT_SIZE
    Ctx(0x281, 0xFFFF0000u),                  // PA_SU_POINT_MINMAX
    Ctx(0x204, 0u),                           // PA_CL_CLIP_CNTL
    Ctx(0x206, 0x43Fu),                       // PA_CL_VTE_CNTL
    Ctx(0x83, 0xFFFFu),                       // PA_SC_CLIPRECT_RULE
    Ctx(0x317, 0x10u),                        // VGT_OUT_DEALLOC_CNTL
    Ctx(0x2FA, 0x3F800000u),                  // PA_CL_GB_VERT_CLIP_ADJ = 1.0
    Ctx(0x2FC, 0x3F800000u),                  // PA_CL_GB_HORZ_CLIP_ADJ
    Ctx(0x2FB, 0x3F800000u),                  // PA_CL_GB_VERT_DISC_ADJ
    Ctx(0x2FD, 0x3F800000u),                  // PA_CL_GB_HORZ_DISC_ADJ
    Ctx(0x202, 0xCC0010u),                    // CB_COLOR_CONTROL
    Ctx(0x30E, 0xFFFFFFFFu),                  // PA_SC_AA_MASK_X0Y0_X1Y0
    Ctx(0x30F, 0xFFFFFFFFu),                  // PA_SC_AA_MASK_X0Y1_X1Y1
    Packet(Op::NumInstances, 1u));

/// SPI_SHADER_PGM_RSRC3 of each stage (CU masks, wave limits).
inline constexpr auto SHADER_LIMITS_BASE = Join( //
    Sh(0x7, 0x1FFu),                             // PS
    Sh(0x46, 0x1FFu),                            // VS
    Sh(0x87, 0x1FFu),                            // GS
    Sh(0xC7, 0x1FFu),                            // ES
    Sh(0x107, 0u),                               // HS
    Sh(0x147, 0x1FFu));                          // LS
inline constexpr auto SHADER_LIMITS200 = Join(   //
    Sh(0x7, 0x1701FFu),                          // PS
    Sh(0x46, 0x1701FDu),                         // VS
    Sh(0x87, 0x1701FFu),                         // GS
    Sh(0xC7, 0x1701FDu),                         // ES
    Sh(0x107, 0x17u),                            // HS
    Sh(0x147, 0x1701FDu),                        // LS
    Sh(0x47, 0x1Cu));                            // SPI_SHADER_LATE_ALLOC_VS

inline constexpr auto VERTEX_HEAD = Join(            //
    Ctx(0x1B1, 2u),                                  // SPI_VS_OUT_CONFIG
    Ctx(0x101, 0u),                                  // VGT_MIN_VTX_INDX
    Ctx(0x100, 0xFFFFFFFFu),                         // VGT_MAX_VTX_INDX
    Ctx(0x103, 0u),                                  // VGT_MULTI_PRIM_IB_RESET_INDX
    Ctx(0x284, 0u),                                  // VGT_OUTPUT_PATH_CNTL
    Ctx(0x290, 0u),                                  // VGT_GS_MODE
    Ctx(0x2AE, 0u));                                 // VGT_VTX_CNT_EN
inline constexpr auto INDEX_OFFSET = Ctx(0x102, 0u); // VGT_INDX_OFFSET (3.50 on)
/// PA_SC_MODE_CNTL_0, then _1 (1.75 changed it).
inline constexpr auto MODE_CONTROL_BASE = Join(Ctx(0x292, 0u), Ctx(0x293, 0x6000000u));
inline constexpr auto MODE_CONTROL175 = Join(Ctx(0x292, 0u), Ctx(0x293, 0x6020000u));
inline constexpr auto VERTEX_TAIL = Join(                      //
    Ctx(0x2F8, 0u),                                            // PA_SC_AA_CONFIG
    Ctx(0x2DE, 0x1E9u),                                        // PA_SU_POLY_OFFSET_DB_FMT_CNTL
    Ctx(0x295, 0x100u, 0x100u, 4u));                           // VGT_GS_PER_ES, VGT_ES_PER_GS, VGT_GS_PER_VS
inline constexpr auto BROADCAST = Uconfig(0x200, 0xE0000000u); // GRBM_GFX_INDEX: all SEs/SHs
inline constexpr auto MULTI_VGT_PARAM = Ctx(0x2AA, 0xFFu);     // IA_MULTI_VGT_PARAM

/// The hardware state of each generation (what follows ClearAndSync).
inline constexpr auto HW_BASE =
    Join(COMPUTE_DEFAULTS, RASTER_DEFAULTS, SHADER_LIMITS_BASE, VERTEX_HEAD, MODE_CONTROL_BASE, VERTEX_TAIL, BROADCAST);
inline constexpr auto HW175 =
    Join(COMPUTE_DEFAULTS, RASTER_DEFAULTS, SHADER_LIMITS_BASE, VERTEX_HEAD, MODE_CONTROL175, VERTEX_TAIL, BROADCAST);
inline constexpr auto HW200 =
    Join(COMPUTE_DEFAULTS, RASTER_DEFAULTS, SHADER_LIMITS200, VERTEX_HEAD, MODE_CONTROL175, VERTEX_TAIL, BROADCAST);
inline constexpr auto HW350 = Join(COMPUTE_DEFAULTS, RASTER_DEFAULTS, SHADER_LIMITS200, VERTEX_HEAD, INDEX_OFFSET,
                                   MODE_CONTROL175, VERTEX_TAIL, BROADCAST, MULTI_VGT_PARAM);

/// What sceGnmDrawInitToDefaultContextState(400) write before their padding.
inline constexpr u32 CONTEXT_WORDS = 0x20;
inline constexpr u32 CONTEXT400_WORDS = 0x100;
inline constexpr auto CONTEXT = Join(CONTEXT_RESET, Packet(Op::NumInstances, 1u), INDEX_OFFSET, Ctx(0x202, 0xCC0010u));
inline constexpr auto CONTEXT400 =
    Join(CONTEXT_RESET, RASTER_DEFAULTS, VERTEX_HEAD, INDEX_OFFSET, MODE_CONTROL175, VERTEX_TAIL, MULTI_VGT_PARAM);

static_assert(CLEAR_AND_SYNC.size() + HW350.size() < HW_INIT_WORDS);
static_assert(CONTEXT.size() < CONTEXT_WORDS && CONTEXT400.size() < CONTEXT400_WORDS);

} // namespace BbGnm::InitState
