// SPDX-FileCopyrightText: Copyright 2026 bbport contributors
// SPDX-License-Identifier: GPL-2.0-or-later
// bbport: observers in a game's own code (bbport_guest_code_hooks.h). The trampoline follows
// shadPS4's InstallGuestCodeHook (core/cpu_patches.cpp of the shadp2p/BBSC forks, GPL-2.0+); the
// trampoline memory and the patching are the port's: the game's code is mapped read and execute
// here, so the jump is written under a short write permission.

#include "bbport_map_near.h"
#include "bbport_guest_code_hooks.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <set>
#include <vector>
#include <sys/mman.h>
#include <unistd.h>

#include <xbyak/xbyak.h>
#include <Zydis/Zydis.h>

#include "common/decoder.h"

namespace Core
{
namespace
{

using namespace Xbyak::util;

constexpr std::size_t NEAR_JUMP_BYTES = 5;
constexpr std::size_t GUEST_RED_ZONE_BYTES = 128;
constexpr std::size_t AREA_SIZE = 4u << 20;      // trampolines (~400 bytes each)
constexpr std::int64_t JUMP_REACH = 0x7000'0000; // inside a rel32 jump's 2 GiB

std::mutex hook_mutex;
std::set<std::uintptr_t> hooked_sites; // first bytes of installed hooks

/// An executable area within reach of its code, mapped on first use.
struct Area
{
    std::uintptr_t start = 0;
    std::unique_ptr<Xbyak::CodeGenerator> gen;
};
std::vector<Area> areas;

std::uintptr_t MapNear(std::uintptr_t code)
{
    // Below the code first (the port maps the game's image at 0x800000000 with room under it),
    // then above, in 64 MiB steps; else the runtime's low region (bbport_map_near.h).
    return reinterpret_cast<std::uintptr_t>(BbMapNear::Map(
        code, code + 64, AREA_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, 64ull << 20, "Guest code hooks"));
}

Xbyak::CodeGenerator* AreaFor(std::uintptr_t code)
{
    for (Area& area : areas)
    {
        const std::int64_t distance = std::int64_t(area.start) - std::int64_t(code);
        if (distance > -JUMP_REACH && distance < JUMP_REACH && area.gen->getSize() + 1024 < AREA_SIZE)
        {
            return area.gen.get();
        }
    }
    const std::uintptr_t start = MapNear(code);
    if (start == 0)
    {
        return nullptr;
    }
    auto gen = std::make_unique<Xbyak::CodeGenerator>(AREA_SIZE, reinterpret_cast<void*>(start));
    areas.push_back({start, std::move(gen)});
    return areas.back().gen.get();
}

bool WholeAndAbsolute(const std::uint8_t* code, std::size_t size)
{
    std::size_t offset = 0;
    while (offset < size)
    {
        ZydisDecodedInstruction instruction{};
        std::array<ZydisDecodedOperand, ZYDIS_MAX_OPERAND_COUNT> operands{};
        const ZyanStatus status = Common::Decoder::Instance()->decodeInstruction(
            instruction, operands.data(), const_cast<std::uint8_t*>(code) + offset, size - offset);
        if (!ZYAN_SUCCESS(status) || instruction.length == 0 || instruction.length > size - offset)
        {
            std::fprintf(stderr, "Guest code hook at %p: not whole instructions\n", static_cast<const void*>(code));
            return false;
        }
        for (std::uint8_t i = 0; i < instruction.operand_count; ++i)
        {
            const ZydisDecodedOperand_& operand = operands[i];
            if ((operand.type == ZYDIS_OPERAND_TYPE_IMMEDIATE && operand.imm.is_relative) ||
                (operand.type == ZYDIS_OPERAND_TYPE_MEMORY &&
                 (operand.mem.base == ZYDIS_REGISTER_RIP || operand.mem.base == ZYDIS_REGISTER_EIP)))
            {
                std::fprintf(stderr, "Guest code hook at %p: relative addressing\n", static_cast<const void*>(code));
                return false;
            }
        }
        offset += instruction.length;
    }
    return true;
}

/// Writes `bytes` over the game's code (read and execute here) under a short write permission.
void WriteCode(std::uint8_t* code, const std::uint8_t* bytes, std::size_t size)
{
    const long page = sysconf(_SC_PAGESIZE);
    u8* first = reinterpret_cast<std::uint8_t*>(reinterpret_cast<std::uintptr_t>(code) & ~std::uintptr_t(page - 1));
    const std::size_t span = (code + size) - first;
    mprotect(first, span, PROT_READ | PROT_WRITE | PROT_EXEC);
    // The jump's last four bytes first, its opcode last: a thread running here meanwhile meets
    // either the old instruction or the whole jump.
    std::memcpy(code + 1, bytes + 1, size - 1);
    __atomic_store_n(code, bytes[0], __ATOMIC_SEQ_CST);
    mprotect(first, span, GuestCodeProtection());
}

std::atomic<bool> code_writable{false};

} // namespace

int GuestCodeProtection()
{
    return code_writable.load(std::memory_order_acquire) ? PROT_READ | PROT_WRITE | PROT_EXEC : PROT_READ | PROT_EXEC;
}

void KeepGuestCodeWritable()
{
    code_writable.store(true, std::memory_order_release);
}

namespace
{

constexpr std::size_t SNAPSHOT_OFFSET = 0;
constexpr std::size_t SCRATCH_R11_OFFSET = sizeof(GuestRegisterSnapshot);
constexpr std::size_t SCRATCH_RFLAGS_OFFSET = SCRATCH_R11_OFFSET + sizeof(std::uint64_t);
constexpr std::size_t FRAME_RESERVE =
    (SCRATCH_RFLAGS_OFFSET + sizeof(std::uint64_t) + GUEST_RED_ZONE_BYTES + 15) & ~std::size_t(15);

} // namespace

bool InstallGuestCodeHook(void* address, std::span<const std::uint8_t> expected, std::uint64_t tag, GuestCodeHook hook,
                          const std::uint8_t* skip_flag)
{
    u8* code = static_cast<std::uint8_t*>(address);
    if (hook == nullptr || expected.size() < NEAR_JUMP_BYTES)
    {
        return false;
    }
    std::scoped_lock lock{hook_mutex};
    if (std::memcmp(code, expected.data(), expected.size()) != 0)
    {
        std::fprintf(stderr, "Guest code hook at %p: the code differs (another build?)\n", address);
        return false;
    }
    if (WholeAndAbsolute(code, expected.size()) == false)
    {
        return false;
    }
    const uintptr_t begin = reinterpret_cast<std::uintptr_t>(code);
    const auto overlap = hooked_sites.lower_bound(begin > 16 ? begin - 16 : 0);
    if (overlap != hooked_sites.end() && *overlap < begin + expected.size())
    {
        std::fprintf(stderr, "Guest code hook at %p overlaps another one\n", address);
        return false;
    }
    Xbyak::CodeGenerator* t = AreaFor(begin);
    if (t == nullptr)
    {
        std::fprintf(stderr, "Guest code hook at %p: no trampoline memory in reach\n", address);
        return false;
    }
    const std::size_t start_offset = t->getSize();
    const u8* trampoline = t->getCurr();
    const auto snapshot = [](std::size_t field)
    {
        return qword[rsp + SNAPSHOT_OFFSET + field];
    };
    try
    {
        t->lea(rsp, ptr[rsp - FRAME_RESERVE]); // past the game's red zone
        t->mov(qword[rsp + SCRATCH_R11_OFFSET], r11);
        t->pushfq();
        t->pop(r11);
        t->mov(qword[rsp + SCRATCH_RFLAGS_OFFSET], r11);
        t->mov(r11, rsp);
        t->and_(rsp, -16);
        t->mov(snapshot(offsetof(GuestRegisterSnapshot, rax)), rax);
        t->mov(snapshot(offsetof(GuestRegisterSnapshot, rbx)), rbx);
        t->mov(snapshot(offsetof(GuestRegisterSnapshot, rcx)), rcx);
        t->mov(snapshot(offsetof(GuestRegisterSnapshot, rdx)), rdx);
        t->mov(snapshot(offsetof(GuestRegisterSnapshot, rsi)), rsi);
        t->mov(snapshot(offsetof(GuestRegisterSnapshot, rdi)), rdi);
        t->mov(snapshot(offsetof(GuestRegisterSnapshot, rbp)), rbp);
        t->mov(snapshot(offsetof(GuestRegisterSnapshot, r8)), r8);
        t->mov(snapshot(offsetof(GuestRegisterSnapshot, r9)), r9);
        t->mov(snapshot(offsetof(GuestRegisterSnapshot, r10)), r10);
        t->mov(snapshot(offsetof(GuestRegisterSnapshot, r12)), r12);
        t->mov(snapshot(offsetof(GuestRegisterSnapshot, r13)), r13);
        t->mov(snapshot(offsetof(GuestRegisterSnapshot, r14)), r14);
        t->mov(snapshot(offsetof(GuestRegisterSnapshot, r15)), r15);
        t->mov(rax, qword[r11 + SCRATCH_R11_OFFSET]);
        t->mov(snapshot(offsetof(GuestRegisterSnapshot, r11)), rax);
        t->mov(rax, qword[r11 + SCRATCH_RFLAGS_OFFSET]);
        t->mov(snapshot(offsetof(GuestRegisterSnapshot, rflags)), rax);
        t->lea(rax, ptr[r11 + FRAME_RESERVE]);
        t->mov(snapshot(offsetof(GuestRegisterSnapshot, rsp)), rax);
        t->stmxcsr(dword[rsp + SNAPSHOT_OFFSET + offsetof(GuestRegisterSnapshot, mxcsr)]);
        for (int i = 0; i < 16; ++i)
        {
            t->vmovdqu(yword[rsp + SNAPSHOT_OFFSET + offsetof(GuestRegisterSnapshot, ymm) + i * 32], Xbyak::Ymm(i));
        }
        // The aligned snapshot in a callee-saved register across the call.
        t->mov(r12, rsp);
        t->mov(rdi, tag);
        t->lea(rsi, ptr[r12 + SNAPSHOT_OFFSET]);
        t->mov(rax, reinterpret_cast<std::uint64_t>(hook));
        t->call(rax);
        t->mov(rsp, r12);
        for (int i = 0; i < 16; ++i)
        {
            t->vmovdqu(Xbyak::Ymm(i), yword[rsp + SNAPSHOT_OFFSET + offsetof(GuestRegisterSnapshot, ymm) + i * 32]);
        }
        t->ldmxcsr(dword[rsp + SNAPSHOT_OFFSET + offsetof(GuestRegisterSnapshot, mxcsr)]);
        t->mov(rbx, snapshot(offsetof(GuestRegisterSnapshot, rbx)));
        t->mov(rcx, snapshot(offsetof(GuestRegisterSnapshot, rcx)));
        t->mov(rdx, snapshot(offsetof(GuestRegisterSnapshot, rdx)));
        t->mov(rsi, snapshot(offsetof(GuestRegisterSnapshot, rsi)));
        t->mov(rdi, snapshot(offsetof(GuestRegisterSnapshot, rdi)));
        t->mov(rbp, snapshot(offsetof(GuestRegisterSnapshot, rbp)));
        t->mov(r8, snapshot(offsetof(GuestRegisterSnapshot, r8)));
        t->mov(r9, snapshot(offsetof(GuestRegisterSnapshot, r9)));
        t->mov(r10, snapshot(offsetof(GuestRegisterSnapshot, r10)));
        t->mov(r11, snapshot(offsetof(GuestRegisterSnapshot, r11)));
        t->mov(r12, snapshot(offsetof(GuestRegisterSnapshot, r12)));
        t->mov(r13, snapshot(offsetof(GuestRegisterSnapshot, r13)));
        t->mov(r14, snapshot(offsetof(GuestRegisterSnapshot, r14)));
        t->mov(r15, snapshot(offsetof(GuestRegisterSnapshot, r15)));
        t->mov(rax, snapshot(offsetof(GuestRegisterSnapshot, rflags)));
        t->push(rax);
        t->popfq();
        t->mov(rax, snapshot(offsetof(GuestRegisterSnapshot, rax)));
        t->mov(rsp, snapshot(offsetof(GuestRegisterSnapshot, rsp)));
        Xbyak::Label run_original;
        if (skip_flag != nullptr)
        {
            // Function entries only: [rsp] is the return address, flags are not live across a
            // call; a set flag (cleared here) returns from the game's function at once.
            t->push(rax);
            t->mov(rax, reinterpret_cast<std::uint64_t>(skip_flag));
            t->cmp(byte[rax], 0);
            t->mov(byte[rax], 0);
            t->pop(rax);
            t->je(run_original, Xbyak::CodeGenerator::LabelType::T_NEAR);
            t->ret();
        }
        t->L(run_original);
        t->db(expected.data(), expected.size());
        t->jmp(reinterpret_cast<const void*>(code + expected.size()), Xbyak::CodeGenerator::LabelType::T_NEAR);
    }
    catch (const Xbyak::Error& error)
    {
        t->setSize(start_offset);
        std::fprintf(stderr, "Guest code hook at %p: trampoline failed (%s)\n", address, error.what());
        return false;
    }
    // The site: a jump to the trampoline, the rest of the span filled with nops.
    std::vector<std::uint8_t> patch(expected.size(), 0x90);
    const std::int64_t rel =
        std::int64_t(reinterpret_cast<std::uintptr_t>(trampoline)) - std::int64_t(begin + NEAR_JUMP_BYTES);
    patch[0] = 0xe9;
    const std::int32_t rel32 = std::int32_t(rel);
    std::memcpy(patch.data() + 1, &rel32, 4);
    WriteCode(code, patch.data(), patch.size());
    hooked_sites.insert(begin);
    return true;
}

} // namespace Core
