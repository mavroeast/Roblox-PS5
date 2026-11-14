// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "pal/pal_result.hpp"

#include <cstdint>
#include <string_view>

namespace rbxps5::gfx {

enum class BackendKind : uint8_t {
    None = 0,
    Ps5VulkanStatic = 1, // Milestone 2: mpereiraesaa/ps5-vulkan (libps5vk.a)
    Ps5Radv = 2,         // Optional fallback: mihawk-99/PS5_Vulkan (Mesa RADV)
};

struct SwapchainDesc {
    uint32_t width{1920};
    uint32_t height{1080};
    bool vsync{true};
};

// Pure abstract graphics backend boundary. Keeps Milestone 2 (ps5-vulkan) and
// Milestone 5 (Nightcap Android Vulkan WSI adapter) decoupled from each other.
class IGraphicsBackend {
public:
    virtual ~IGraphicsBackend() noexcept = default;

    [[nodiscard]] virtual BackendKind kind() const noexcept = 0;
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    virtual pal::PalResult init(const SwapchainDesc& desc) noexcept = 0;
    virtual void shutdown() noexcept = 0;

    // Exposes static Vulkan dispatch lookup for Nightcap's libvulkan.so adapter
    // without requiring a system Vulkan ICD loader.
    [[nodiscard]] virtual void* get_instance_proc_addr_fn() const noexcept = 0;

    virtual pal::PalResult begin_frame() noexcept = 0;
    virtual pal::PalResult present_frame() noexcept = 0;
};

} // namespace rbxps5::gfx
