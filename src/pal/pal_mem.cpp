// SPDX-License-Identifier: GPL-3.0-or-later
#include "pal/pal_mem.hpp"
#include "pal/ps5_abi.hpp"

#include <cerrno>
#include <cstring>

namespace rbxps5::pal {

namespace {

constexpr size_t align_up(size_t val, size_t align) noexcept {
    return (val + align - 1) & ~(align - 1);
}

#if !defined(ROBLOX_PS5_HOST_BUILD)
// Flag observed in PS5 title probes for sceKernelMapDirectMemory fixed placement.
constexpr int32_t kSceKernelMapFixed = 0x10;
// Memory type 0 = WB_ONION (standard CPU/GPU coherent cached memory on Orbis/Prospero).
constexpr int32_t kSceKernelMemTypeWbOnion = 0;
#endif

} // namespace

size_t query_direct_memory_size() noexcept {
#if defined(ROBLOX_PS5_HOST_BUILD)
    return 0;
#else
    return sceKernelGetDirectMemorySize();
#endif
}

PalResult allocate_direct_memory_rw(size_t size,
                                    size_t alignment,
                                    uintptr_t virtual_hint,
                                    bool exact_virtual_addr,
                                    DirectMemoryBlock& out_block) noexcept {
    out_block = DirectMemoryBlock{};

    if (size == 0) {
        return PalResult::failure(EINVAL);
    }

    const size_t effective_align = alignment < kPs5PageSize ? kDefaultDmemAlign : alignment;
    const size_t aligned_size = align_up(size, effective_align);

#if defined(ROBLOX_PS5_HOST_BUILD)
    (void)virtual_hint;
    (void)exact_virtual_addr;
    (void)aligned_size;
    return PalResult::failure(ENOSYS);
#else
    const size_t total_dmem = sceKernelGetDirectMemorySize();
    const off_t search_end = total_dmem > 0 ? static_cast<off_t>(total_dmem)
                                            : static_cast<off_t>(16ULL * 1024ULL * 1024ULL * 1024ULL);

    off_t phys_addr = -1;
    int32_t rc = sceKernelAllocateDirectMemory(0,
                                               search_end,
                                               aligned_size,
                                               effective_align,
                                               kSceKernelMemTypeWbOnion,
                                               &phys_addr);
    if (rc != 0) {
        return PalResult::failure(rc);
    }

    void* target_vaddr = reinterpret_cast<void*>(virtual_hint);
    bool reserved = false;
    int32_t map_flags = 0;

    if (virtual_hint != 0) {
        void* reserved_addr = target_vaddr;
        int32_t rsv_rc = sceKernelReserveVirtualRange(&reserved_addr, aligned_size, 0, effective_align);
        if (rsv_rc != 0) {
            sceKernelReleaseDirectMemory(phys_addr, aligned_size);
            return PalResult::failure(rsv_rc);
        }
        if (exact_virtual_addr && reserved_addr != target_vaddr) {
            sceKernelMunmap(reserved_addr, aligned_size);
            sceKernelReleaseDirectMemory(phys_addr, aligned_size);
            return PalResult::failure(EADDRINUSE);
        }
        target_vaddr = reserved_addr;
        reserved = true;
        map_flags = kSceKernelMapFixed;
    }

    rc = sceKernelMapDirectMemory(&target_vaddr,
                                  aligned_size,
                                  kMemProtRead | kMemProtWrite,
                                  map_flags,
                                  phys_addr,
                                  effective_align);
    if (rc != 0) {
        if (reserved) {
            sceKernelMunmap(target_vaddr, aligned_size);
        }
        sceKernelReleaseDirectMemory(phys_addr, aligned_size);
        return PalResult::failure(rc);
    }

    // Hardware finding: Direct Memory is not guaranteed zeroed by the kernel.
    std::memset(target_vaddr, 0, aligned_size);

    out_block.virtual_addr = target_vaddr;
    out_block.physical_addr = phys_addr;
    out_block.size = aligned_size;
    out_block.reserved_vrange = reserved;
    return PalResult::success();
#endif
}

void free_direct_memory(DirectMemoryBlock& block) noexcept {
#if !defined(ROBLOX_PS5_HOST_BUILD)
    if (block.virtual_addr != nullptr && block.size > 0) {
        sceKernelMunmap(block.virtual_addr, block.size);
    }
    if (block.physical_addr >= 0 && block.size > 0) {
        sceKernelReleaseDirectMemory(block.physical_addr, block.size);
    }
#endif
    block = DirectMemoryBlock{};
}

PalResult protect_memory(void* addr, size_t size, int32_t prot_bits) noexcept {
#if defined(ROBLOX_PS5_HOST_BUILD)
    (void)addr;
    (void)size;
    (void)prot_bits;
    return PalResult::failure(ENOSYS);
#else
    if (addr == nullptr || size == 0) {
        return PalResult::failure(EINVAL);
    }
    int32_t rc = sceKernelMprotect(addr, size, prot_bits);
    return rc == 0 ? PalResult::success() : PalResult::failure(rc);
#endif
}

ExecProbeReport probe_direct_memory_rx_capability() noexcept {
    ExecProbeReport report{};

    DirectMemoryBlock block{};
    PalResult alloc_res = allocate_direct_memory_rw(kPs5PageSize,
                                                    kDefaultDmemAlign,
                                                    kHighDmemHint,
                                                    false,
                                                    block);
    report.alloc_rc = alloc_res.code;
    if (!alloc_res.ok() || block.virtual_addr == nullptr) {
        return report;
    }

    // x86_64: mov eax, 0x1337; ret
    static constexpr uint8_t kStub[] = {0xB8, 0x37, 0x13, 0x00, 0x00, 0xC3};
    std::memcpy(block.virtual_addr, kStub, sizeof(kStub));

    PalResult prot_res = protect_memory(block.virtual_addr, kPs5PageSize, kMemProtRead | kMemProtExec);
    report.mprotect_rx_rc = prot_res.code;

    if (prot_res.ok()) {
        using Fn = uint32_t (*)();
        auto fn = reinterpret_cast<Fn>(block.virtual_addr);
        report.return_value = fn();
        report.executed = true;
    }

    free_direct_memory(block);
    return report;
}

} // namespace rbxps5::pal
