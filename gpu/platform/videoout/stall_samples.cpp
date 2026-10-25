// SPDX-License-Identifier: GPL-2.0-or-later
// bbport: BB_STALL_SAMPLES (stall_samples.h). The sampling thread allocates nothing and takes no lock
// another thread may hold for long: the stalls it is for were threads waiting for the malloc
// arena's lock and the address space lock (an allocating sampler waited with them).

#include "platform/videoout/stall_samples.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <dlfcn.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "common/thread.h"
#include "common/types.h"

namespace BbVideoOut::StallSamples
{

namespace
{

constexpr auto SAMPLE_EVERY = std::chrono::milliseconds(1);
constexpr s64 SAMPLE_AFTER_MS = 25;
constexpr double REPORT_OVER_MS = 40.0; ///< the stall report's threshold (frame_stats.cpp)
constexpr size_t MAX_SHOWN = 24;
constexpr size_t MAX_ENTRIES = 1024;
constexpr size_t NAME_BYTES = 16; ///< /proc/<tid>/comm: 15 characters and the terminator
constexpr size_t READ_BYTES = 256;
constexpr size_t DIRECTORY_BYTES = 16384;
constexpr int SYSCALL_FIELDS = 9; ///< number, six arguments, stack pointer, user-space address
constexpr s64 NS_PER_MS = 1'000'000;
constexpr long RUNNING = -2; ///< "running": on a CPU (in user space or the kernel)
constexpr long OUTSIDE = -1; ///< blocked, not in a system call ("-1 sp pc")
constexpr long FUTEX = 202;
/// __lll_lock_wait_private's code (glibc's own locks: the malloc arena's).
constexpr unsigned long long LOCK_WAIT_BYTES = 0x100;
/// linux_dirent64: d_ino (8), d_off (8), then d_reclen (2), d_type (1) and the name.
constexpr size_t DIRENT_RECLEN_AT = 16;
constexpr size_t DIRENT_NAME_AT = 19;

/// The system calls a thread sleeps in: one that slept through the whole stall is left out.
constexpr std::array<long, 9> SLEEPING_CALLS = {7, 23, 35, FUTEX, 230, 232, 270, 271, 281};

struct NamedCall
{
    long number;
    const char* name;
};
constexpr std::array<NamedCall, 23> CALL_NAMES = {{
    {0, "read"},          {1, "write"},          {7, "poll"},
    {9, "mmap"},          {10, "mprotect"},      {11, "munmap"},
    {12, "brk"},          {16, "ioctl"},         {17, "pread64"},
    {18, "pwrite64"},     {23, "select"},        {25, "mremap"},
    {28, "madvise"},      {35, "nanosleep"},     {74, "fsync"},
    {75, "fdatasync"},    {FUTEX, "futex"},      {230, "clock_nanosleep"},
    {232, "epoll_wait"},  {257, "openat"},       {270, "pselect6"},
    {281, "epoll_pwait"}, {319, "memfd_create"},
}};

struct Entry
{
    u32 tid;
    long number; ///< system call, RUNNING or OUTSIDE
    unsigned long long pc;
    std::array<unsigned long long, 3> args;
    std::array<char, NAME_BYTES> name;
    u32 count;
};

std::atomic<s64> last_flip_ns{0};
std::mutex mutex;
std::array<Entry, MAX_ENTRIES> entries{};
size_t num_entries = 0;
u32 rounds = 0;
unsigned long long lock_wait_begin = 0;

s64 NowNs()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

/// Up to `size - 1` bytes of a small file, without the trailing newline; empty when unreadable.
size_t ReadSmall(const char* path, char* out, size_t size)
{
    const int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0)
    {
        out[0] = '\0';
        return 0;
    }
    const ssize_t got = read(fd, out, size - 1);
    close(fd);
    size_t length = got > 0 ? size_t(got) : 0;
    while (length != 0 && (out[length - 1] == '\n' || out[length - 1] == ' '))
    {
        --length;
    }
    out[length] = '\0';
    return length;
}

const char* CallName(long number)
{
    for (const NamedCall& call : CALL_NAMES)
    {
        if (call.number == number)
        {
            return call.name;
        }
    }
    return nullptr;
}

bool LockWait(const Entry& entry)
{
    return entry.number == FUTEX && lock_wait_begin != 0 && entry.pc >= lock_wait_begin &&
           entry.pc < lock_wait_begin + LOCK_WAIT_BYTES;
}

void Count(const Entry& sample)
{
    std::scoped_lock lock{mutex};
    for (size_t i = 0; i < num_entries; ++i)
    {
        Entry& entry = entries[i];
        if (entry.tid == sample.tid && entry.number == sample.number && entry.pc == sample.pc)
        {
            ++entry.count;
            entry.args = sample.args;
            return;
        }
    }
    if (num_entries < entries.size())
    {
        entries[num_entries] = sample;
        entries[num_entries].count = 1;
        ++num_entries;
    }
}

/// One round: every thread's state (/proc/self/task/<tid>/syscall).
void SampleRound(int self)
{
    static std::array<char, DIRECTORY_BYTES> directory;
    const int dir = open("/proc/self/task", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (dir < 0)
    {
        return;
    }
    while (true)
    {
        const long got = syscall(SYS_getdents64, dir, directory.data(), directory.size());
        if (got <= 0) break;
        for (long at = 0; at < got;)
        {
            u16 length;
            std::memcpy(&length, directory.data() + at + DIRENT_RECLEN_AT, sizeof(length));
            const char* name = directory.data() + at + DIRENT_NAME_AT;
            at += length;
            if (name[0] == '.') continue;
            const u32 tid = u32(std::strtoul(name, nullptr, 10));
            if (int(tid) == self) continue;
            char path[READ_BYTES];
            char state[READ_BYTES];
            std::snprintf(path, sizeof(path), "/proc/self/task/%u/syscall", tid);
            if (ReadSmall(path, state, sizeof(state)) == 0) continue;
            Entry sample{tid, RUNNING, 0, {}, {}, 0};
            if (std::strncmp(state, "running", std::strlen("running")) != 0)
            {
                std::array<unsigned long long, SYSCALL_FIELDS - 1> fields{};
                if (std::sscanf(state, "%ld %llx %llx %llx %llx %llx %llx %llx %llx", &sample.number, &fields[0],
                                &fields[1], &fields[2], &fields[3], &fields[4], &fields[5], &fields[6], &fields[7]) < 1)
                {
                    continue;
                }
                // Not in a system call: "-1 sp pc".
                sample.pc = sample.number == OUTSIDE ? fields[1] : fields[SYSCALL_FIELDS - 2];
                sample.args = {fields[0], fields[1], fields[2]};
            }
            std::snprintf(path, sizeof(path), "/proc/self/task/%u/comm", tid);
            ReadSmall(path, sample.name.data(), sample.name.size());
            Count(sample);
        }
    }
    close(dir);
    std::scoped_lock lock{mutex};
    ++rounds;
}

void Loop()
{
    Common::SetCurrentThreadName("bb:StallSample");
    const int self = int(gettid());
    while (true)
    {
        std::this_thread::sleep_for(SAMPLE_EVERY);
        const s64 flip = last_flip_ns.load(std::memory_order_relaxed);
        if (flip != 0 && NowNs() - flip >= SAMPLE_AFTER_MS * NS_PER_MS)
        {
            SampleRound(self);
        }
    }
}

/// "module+offset" for an address, from /proc/self/maps (read at report time only).
std::string Locate(unsigned long long address, const std::vector<std::string>& maps)
{
    for (const std::string& line : maps)
    {
        unsigned long long begin = 0, end = 0, offset = 0;
        char path[READ_BYTES] = {};
        if (std::sscanf(line.c_str(), "%llx-%llx %*s %llx %*s %*s %255s", &begin, &end, &offset, path) < 3)
        {
            continue;
        }
        if (address >= begin && address < end)
        {
            const char* slash = std::strrchr(path, '/');
            const char* file = path[0] == '\0' ? "anonymous" : slash != nullptr ? slash + 1 : path;
            char text[READ_BYTES];
            std::snprintf(text, sizeof(text), "%s+%#llx", file,
                          path[0] == '/' ? address - begin + offset : address - begin);
            return text;
        }
    }
    return "?";
}

void Print(const Entry& entry, const std::vector<std::string>& maps)
{
    const char* call = CallName(entry.number);
    char what[READ_BYTES];
    if (entry.number == RUNNING)
    {
        std::snprintf(what, sizeof(what), "running");
    }
    else if (LockWait(entry))
    {
        std::snprintf(what, sizeof(what), "waiting for a glibc lock (the malloc arena's)");
    }
    else if (entry.number == OUTSIDE)
    {
        std::snprintf(what, sizeof(what), "blocked outside a system call at %#llx [%s]", entry.pc,
                      Locate(entry.pc, maps).c_str());
    }
    else if (call != nullptr)
    {
        std::snprintf(what, sizeof(what), "%s(%#llx, %#llx, %#llx) from %#llx [%s]", call, entry.args[0], entry.args[1],
                      entry.args[2], entry.pc, Locate(entry.pc, maps).c_str());
    }
    else
    {
        std::snprintf(what, sizeof(what), "system call %ld(%#llx, %#llx, %#llx) from %#llx [%s]", entry.number,
                      entry.args[0], entry.args[1], entry.args[2], entry.pc, Locate(entry.pc, maps).c_str());
    }
    std::printf("         %4u  %s (%u): %s\n", entry.count, entry.name.data(), entry.tid, what);
}

} // namespace

void OnFlip(double frame_ms)
{
    static const bool enabled = []
    {
        const char* env = std::getenv("BB_STALL_SAMPLES");
        return env != nullptr && env[0] == '1';
    }();
    if (enabled == false)
    {
        return;
    }
    static std::once_flag started;
    std::call_once(started,
                   []
                   {
                       lock_wait_begin =
                           reinterpret_cast<unsigned long long>(dlsym(RTLD_DEFAULT, "__lll_lock_wait_private"));
                       std::thread(Loop).detach();
                   });
    std::vector<Entry> report;
    u32 report_rounds = 0;
    {
        std::scoped_lock lock{mutex};
        last_flip_ns.store(NowNs(), std::memory_order_relaxed);
        if (frame_ms > REPORT_OVER_MS && rounds != 0)
        {
            report_rounds = rounds;
            for (size_t i = 0; i < num_entries; ++i)
            {
                const Entry& entry = entries[i];
                // A thread asleep in the same place the whole time took no part in it.
                const bool sleeping =
                    std::find(SLEEPING_CALLS.begin(), SLEEPING_CALLS.end(), entry.number) != SLEEPING_CALLS.end();
                if (sleeping && LockWait(entry) == false && entry.count >= rounds) continue;
                report.push_back(entry);
            }
        }
        num_entries = 0;
        rounds = 0;
    }
    if (report.empty())
    {
        return;
    }
    std::sort(report.begin(), report.end(),
              [](const Entry& a, const Entry& b)
              {
                  return a.count > b.count;
              });
    std::vector<std::string> maps;
    FILE* file = std::fopen("/proc/self/maps", "r");
    if (file != nullptr)
    {
        char line[READ_BYTES * 2];
        while (std::fgets(line, sizeof(line), file) != nullptr)
        {
            maps.emplace_back(line);
        }
        std::fclose(file);
    }
    std::printf("       thread samples (%u rounds 1 ms apart from %lld ms into the frame; threads asleep "
                "throughout left out):\n",
                report_rounds, static_cast<long long>(SAMPLE_AFTER_MS));
    for (size_t i = 0; i < report.size() && i < MAX_SHOWN; ++i)
    {
        Print(report[i], maps);
    }
}

} // namespace BbVideoOut::StallSamples
