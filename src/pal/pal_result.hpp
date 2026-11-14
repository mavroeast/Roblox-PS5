// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>

namespace rbxps5::pal {

struct PalResult {
    int32_t code{0};

    [[nodiscard]] constexpr bool ok() const noexcept {
        return code == 0;
    }

    [[nodiscard]] static constexpr PalResult success() noexcept {
        return PalResult{0};
    }

    [[nodiscard]] static constexpr PalResult failure(int32_t err) noexcept {
        return PalResult{err == 0 ? -1 : err};
    }
};

} // namespace rbxps5::pal
