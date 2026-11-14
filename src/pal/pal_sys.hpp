// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "pal/pal_result.hpp"

#include <cstdint>

namespace rbxps5::pal {

// Hides the PS5 launch splash screen once basic startup initialization finishes.
PalResult hide_splash_screen() noexcept;

// Sleeps without busy-spinning while keeping the native title alive for OS-mediated
// Close Game termination (per ps5-native-app-boilerplate lifecycle rule).
void sleep_ms(uint32_t ms) noexcept;

} // namespace rbxps5::pal
