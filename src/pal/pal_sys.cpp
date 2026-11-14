// SPDX-License-Identifier: GPL-3.0-or-later
#include "pal/pal_sys.hpp"
#include "pal/ps5_abi.hpp"

#include <unistd.h>

namespace rbxps5::pal {

PalResult hide_splash_screen() noexcept {
#if defined(ROBLOX_PS5_HOST_BUILD)
    return PalResult::success();
#else
    int32_t rc = sceSystemServiceHideSplashScreen();
    return rc == 0 ? PalResult::success() : PalResult::failure(rc);
#endif
}

void sleep_ms(uint32_t ms) noexcept {
#if defined(ROBLOX_PS5_HOST_BUILD)
    ::usleep(ms * 1000U);
#else
    sceKernelUsleep(ms * 1000U);
#endif
}

} // namespace rbxps5::pal
