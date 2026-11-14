// SPDX-License-Identifier: GPL-3.0-or-later
#include "gfx/gfx_backend.hpp"
#include "pal/pal_fs.hpp"
#include "pal/pal_input.hpp"
#include "pal/pal_log.hpp"
#include "pal/pal_mem.hpp"
#include "pal/pal_sys.hpp"

#include <array>
#include <cstdint>
#include <span>

int main() {
    using namespace rbxps5;

    // 1. Initialize persistent /download0/ directory tree first so logs can be saved.
    const pal::PalResult fs_res = pal::init_storage_layout(pal::kDefaultPs5Storage);

    pal::Logger log{};
    log.init(pal::LogConfig{
        "/download0/roblox-ps5/logs/m1_foundation.log",
        {},
        0,
    });

    log.write(pal::LogLevel::Info, "BOOT", "Roblox-PS5 Milestone 1 starting");
    log.writef(pal::LogLevel::Info,
               "FS",
               "init_storage_layout(/download0/roblox-ps5): rc=%d",
               fs_res.code);

    // 2. Verify read-only packaged asset under /app0/assets/.
    std::array<char, 128> asset_buf{};
    size_t asset_bytes = 0;
    const pal::PalResult asset_res = pal::read_small_file("/app0/assets/build_info.txt",
                                                          std::span<char>{asset_buf},
                                                          asset_bytes);
    log.writef(pal::LogLevel::Info,
               "FS",
               "read(/app0/assets/build_info.txt): rc=%d bytes=%zu text=\"%s\"",
               asset_res.code,
               asset_bytes,
               asset_res.ok() ? asset_buf.data() : "<unreadable>");

    // 3. Verify persistent read/write under /download0/.
    const pal::PalResult rw_res = pal::probe_persistent_rw(pal::kDefaultPs5Storage.logs_dir);
    log.writef(pal::LogLevel::Info, "FS", "probe_persistent_rw: rc=%d", rw_res.code);

    // 4. Query Direct Memory & run RW allocation test (4 MiB with high virtual hint).
    const size_t total_dmem = pal::query_direct_memory_size();
    log.writef(pal::LogLevel::Info,
               "MEM",
               "sceKernelGetDirectMemorySize=%zu bytes (%.2f GiB)",
               total_dmem,
               static_cast<double>(total_dmem) / (1024.0 * 1024.0 * 1024.0));

    pal::DirectMemoryBlock test_dmem{};
    const pal::PalResult dmem_res = pal::allocate_direct_memory_rw(4ULL * 1024ULL * 1024ULL,
                                                                   pal::kDefaultDmemAlign,
                                                                   pal::kHighDmemHint,
                                                                   false,
                                                                   test_dmem);
    log.writef(pal::LogLevel::Info,
               "MEM",
               "allocate_direct_memory_rw(4MiB): rc=%d vaddr=%p phys=0x%llx size=%zu",
               dmem_res.code,
               test_dmem.virtual_addr,
               static_cast<unsigned long long>(test_dmem.physical_addr),
               test_dmem.size);
    if (dmem_res.ok()) {
        pal::free_direct_memory(test_dmem);
    }

    // 5. Diagnostic RW -> RX permission transition probe (never assumed to pass).
    const pal::ExecProbeReport exec_probe = pal::probe_direct_memory_rx_capability();
    log.writef(pal::LogLevel::Info,
               "MEM",
               "exec_probe: alloc_rc=%d mprotect_rx_rc=%d executed=%d ret=0x%x",
               exec_probe.alloc_rc,
               exec_probe.mprotect_rx_rc,
               exec_probe.executed ? 1 : 0,
               exec_probe.return_value);

    // 6. Initialize UserService & DualSense Pad.
    pal::PadService pad{};
    const pal::PalResult pad_res = pad.init();
    log.writef(pal::LogLevel::Info,
               "INPUT",
               "PadService::init: rc=%d user_id=%d pad_handle=%d",
               pad_res.code,
               pad.initial_user_id(),
               pad.pad_handle());

    // 7. Milestone 1 has no graphics backend attached yet; Milestone 2 attaches ps5-vulkan.
    gfx::IGraphicsBackend* gfx_backend = nullptr;
    log.writef(pal::LogLevel::Info,
               "GFX",
               "Graphics backend slot ready (attached=%d)",
               gfx_backend != nullptr ? 1 : 0);

    // 8. Dismiss PS5 launch splash screen & flush logs before entering steady loop.
    const pal::PalResult splash_res = pal::hide_splash_screen();
    log.writef(pal::LogLevel::Info, "SYS", "hide_splash_screen: rc=%d", splash_res.code);
    log.flush();

    // 9. Keep main() alive for clean PS5 OS-mediated Close Game termination.
    uint32_t last_buttons = 0;
    uint32_t ticks = 0;
    for (;;) {
        if (pad.is_open()) {
            uint32_t buttons = 0;
            if (pad.poll_raw_buttons(buttons).ok() && buttons != last_buttons) {
                last_buttons = buttons;
                log.writef(pal::LogLevel::Debug, "INPUT", "raw_buttons=0x%08x", buttons);
                log.flush();
            }
        }

        if ((++ticks % 300U) == 0U) {
            log.writef(pal::LogLevel::Info, "HEARTBEAT", "alive ticks=%u", ticks);
            log.flush();
        }

        pal::sleep_ms(16);
    }
}
