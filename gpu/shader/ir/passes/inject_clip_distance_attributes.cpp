// SPDX-License-Identifier: GPL-2.0-or-later
#include "shader/ir/ir_emitter.h"
#include "shader/ir/passes/ir_passes.h"
#include "shader/ir/program.h"
#include "shader/info.h"

namespace Shader
{

void InjectClipDistanceAttributes(IR::Program& program, RuntimeInfo& runtime_info)
{
    HwFragmentRuntimeInfo& fs = runtime_info.hw.fs;
    if (fs.clip_distance_emulation == false || program.info.sw_stage != SwStage::Fragment)
    {
        return;
    }
    // After the prologue and the instruction following it.
    IR::Block* const entry = program.blocks.front();
    auto it = std::ranges::find(*entry, IR::Opcode::Prologue, &IR::Inst::GetOpcode);
    ASSERT(it != entry->end());
    ++it;
    ASSERT(it != entry->end());
    ++it;
    IR::IREmitter ir{*entry, it};
    // The vertex shader's clip distances arrive as one more parameter. Up to 4 are assumed (of
    // 8) to spare an export slot.
    const IR::Attribute attribute = IR::Attribute::Param0 + fs.num_inputs;
    for (u32 comp = 0; comp < MAX_EMULATED_CLIP_DISTANCES; ++comp)
    {
        const IR::F32 distance = ir.GetAttribute(attribute, comp);
        const IR::U1 outside = ir.FPLessThan(distance, ir.Imm32(0.0f));
        ir.Discard(outside);
    }
    ++fs.num_inputs;
}

} // namespace Shader
