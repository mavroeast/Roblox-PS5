// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "pal/pal_result.hpp"

#include <cstddef>
#include <span>
#include <string_view>

namespace rbxps5::pal {

struct StorageLayout {
    std::string_view root;
    std::string_view logs_dir;
    std::string_view config_dir;
    std::string_view cache_dir;
    std::string_view apk_dir;
};

constexpr StorageLayout kDefaultPs5Storage{
    "/download0/roblox-ps5",
    "/download0/roblox-ps5/logs",
    "/download0/roblox-ps5/config",
    "/download0/roblox-ps5/cache",
    "/download0/roblox-ps5/apk",
};

// Note: Does NOT call access() because access() returns EPERM in native titles
// on some PS5 firmwares (verified on 12.09). Uses open()/stat()/mkdir() directly.
PalResult ensure_directory(std::string_view path) noexcept;
PalResult init_storage_layout(const StorageLayout& layout) noexcept;

PalResult read_small_file(std::string_view path,
                          std::span<char> out_buf,
                          size_t& out_bytes_read) noexcept;

PalResult write_file_atomic_like(std::string_view path,
                                 std::span<const std::byte> data) noexcept;

PalResult probe_persistent_rw(std::string_view directory) noexcept;

} // namespace rbxps5::pal
