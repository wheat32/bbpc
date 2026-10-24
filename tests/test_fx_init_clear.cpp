// SPDX-License-Identifier: GPL-2.0-or-later
#include <array>
#include <cassert>
#include <cstdio>

#include "bbport_fx_init_clear.h"
#include "cp/cp_decoder.h"

namespace
{

// Captured from the main-thread FX initializer. Both stored FP16 channels
// are zero; alpha=1 has no stored channel in this RG target.
constexpr std::array<std::uint32_t, 11> CAPTURED{0x10dfe600, 0x00f0000f, 0x0000007f, 0, 0x00060714, 0x000001ce,
                                                 0x00020002, 0,          0,          0, 0};

constexpr std::array<float, 4> BLACK{0, 0, 0, 1};

} // namespace

int main()
{
    const auto plan = BbFxInit::Plan(CAPTURED, BLACK);
    assert(plan != std::nullopt && plan->address == 0x10dfe60000 && plan->bytes == 32768);
    const auto commands = BbFxInit::Commands(*plan);
    BbCp::Decoder decoder;
    decoder.Registers().Write(BbCp::Bank::SH + 0x244, 0x12345678);
    decoder.Registers().Write(BbCp::Bank::CONTEXT + 0x203, 0x87654321);
    BbCp::Sink sink;
    BbCp::Cursor cursor{decoder, commands, sink};
    const BbCp::Packet* packet = cursor.Next();
    assert(packet != nullptr && packet->Opcode() == BbCp::Op::WriteData);
    assert(packet->body.size() == 8195 && packet->Word(0) == 0x500);
    assert((std::uint64_t(packet->Word(2)) << 32 | packet->Word(1)) == plan->address);
    for (std::size_t i = 3; i < packet->body.size(); ++i)
    {
        assert(packet->body[i] == 0);
    }
    assert(cursor.Next() == nullptr && decoder.Stats().bad == 0);
    assert(decoder.Stats().register_packets == 0 && decoder.Stats().dispatches == 0);
    assert(decoder.Registers()[BbCp::Bank::SH + 0x244] == 0x12345678);
    assert(decoder.Registers()[BbCp::Bank::CONTEXT + 0x203] == 0x87654321);

    auto color = BLACK;
    color[0] = .5f;
    assert(((BbFxInit::Plan(CAPTURED, color)) == std::nullopt));
    color = BLACK;
    color[3] = 0;
    assert(((BbFxInit::Plan(CAPTURED, color)) == std::nullopt));
    assert(((BbFxInit::Plan(std::span{CAPTURED}.first(6), BLACK)) == std::nullopt));
    for (const auto [index, mask] : std::array<std::pair<unsigned, std::uint32_t>, 9>{{{3, 1},
                                                                                       {4, 1u << 13},
                                                                                       {4, 1u << 14},
                                                                                       {4, 1u << 28},
                                                                                       {4, 1u << 31},
                                                                                       {4, 1u << 11},
                                                                                       {5, 1u << 12},
                                                                                       {5, 1u << 15},
                                                                                       {7, 1}}})
    {
        auto target = CAPTURED;
        target[index] |= mask;
        assert(((BbFxInit::Plan(target, BLACK)) == std::nullopt));
    }
    auto target = CAPTURED;
    target[4] = (target[4] & ~(31u << 2)) | (12u << 2); // RGBA16F needs nonzero alpha
    assert(((BbFxInit::Plan(target, BLACK)) == std::nullopt));
    target = CAPTURED;
    target[2] = 0x3fffff; // forged oversized surface
    assert(((BbFxInit::Plan(target, BLACK)) == std::nullopt));
    target = CAPTURED;
    target[6] = 0x04380780; // full-size display target
    assert(((BbFxInit::Plan(target, BLACK)) == std::nullopt));
    target = CAPTURED;
    target[0] = 0;
    assert(((BbFxInit::Plan(target, BLACK)) == std::nullopt));
    std::puts("PASS: FX initialization: captured RG16F layout, zero WRITE_DATA decoding, unsafe clears rejected");
}
