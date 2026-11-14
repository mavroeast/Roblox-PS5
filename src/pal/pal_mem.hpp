// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "pal/pal_result.hpp"

#include <cstddef>
#include <cstdint>
#include <sys/types.h>

namespace rbxps5::pal {

// PS5 host page size is 16 KiB (0x4000). Direct Memory physical offsets are
// typically aligned to 64 KiB (0x10000) or 2 MiB depending on use case.
inline constexpr size_t kPs5PageSize = 0x4000;
inline constexpr size_t kDefaultDmemAlign = 0x10000;
// High virtual address hint above 32-bit/AGC/libc.prx (0x80000000) reservations.
inline constexpr uintptr_t kHighDmemHint = 0x1000000000ULL;

// SCE/FreeBSD protection bits matching PROT_READ=1, PROT_WRITE=2, PROT_EXEC=4.
inline constexpr int32_t kMemProtRead = 0x1;
inline constexpr int32_t kMemProtWrite = 0x2;
inline constexpr int32_t kMemProtExec = 0x4;

struct DirectMemoryBlock {
    void* virtual_addr{nullptr};
    off_t physical_addr{-1};
    size_t size{0};
    bool reserved_vrange{false};
};

// Queries total Direct Memory reported by libkernel.
size_t query_direct_memory_size() noexcept;

// Reserves virtual range (when fixed/hinted), allocates Direct Memory, maps it
// read-write (never PROT_EXEC at map time, which PS5 titles reject), and zeroes
// the mapped memory (Direct Memory retains prior contents).
PalResult allocate_direct_memory_rw(size_t size,
                                    size_t alignment,
                                    uintptr_t virtual_hint,
                                    bool exact_virtual_addr,
                                    DirectMemoryBlock& out_block) noexcept;

void free_direct_memory(DirectMemoryBlock& block) noexcept;

// Changes protection via sceKernelMprotect. On PS5 titles, RW->RX/RWX support
// is firmware- and environment-dependent and must never be assumed silently.
PalResult protect_memory(void* addr, size_t size, int32_t prot_bits) noexcept;

// Diagnostic probe: allocates 1 page (16 KiB) of Direct Memory RW, writes a
// trivial x86_64 `mov eax, 0x1337; ret` stub, attempts sceKernelMprotect(..., R|X),
// and ONLY executes the stub if sceKernelMprotect returns 0.
struct ExecProbeReport {
    int32_t alloc_rc{0};
    int32_t mprotect_rx_rc{0};
    bool executed{false};
    uint32_t return_value{0};
};

ExecProbeReport probe_direct_memory_rx_capability() noexcept;

} // namespace rbxps5::pal
