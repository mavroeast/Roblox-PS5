// SPDX-License-Identifier: GPL-3.0-or-later
#include "pal/pal_input.hpp"
#include "pal/ps5_abi.hpp"

#include <cerrno>
#include <cstring>

namespace rbxps5::pal {

PadService::~PadService() noexcept {
    shutdown();
}

PalResult PadService::init() noexcept {
#if defined(ROBLOX_PS5_HOST_BUILD)
    return PalResult::failure(ENOSYS);
#else
    if (pad_handle_ >= 0) {
        return PalResult::success();
    }

    int32_t rc = sceUserServiceInitialize(nullptr);
    // Treat already-initialized user service as usable if GetInitialUser succeeds.
    if (rc == 0) {
        user_service_inited_ = true;
    }

    rc = sceUserServiceGetInitialUser(&user_id_);
    if (rc != 0 || user_id_ < 0) {
        shutdown();
        return PalResult::failure(rc != 0 ? rc : -1);
    }

    rc = scePadInit();
    if (rc != 0) {
        shutdown();
        return PalResult::failure(rc);
    }

    pad_handle_ = scePadOpen(user_id_, 0, 0, nullptr);
    if (pad_handle_ < 0) {
        int32_t err = pad_handle_;
        shutdown();
        return PalResult::failure(err);
    }

    return PalResult::success();
#endif
}

void PadService::shutdown() noexcept {
#if !defined(ROBLOX_PS5_HOST_BUILD)
    if (pad_handle_ >= 0) {
        scePadClose(pad_handle_);
        pad_handle_ = -1;
    }
    if (user_service_inited_) {
        sceUserServiceTerminate();
        user_service_inited_ = false;
    }
#endif
    user_id_ = -1;
}

PalResult PadService::poll_raw_buttons(uint32_t& out_buttons) noexcept {
    out_buttons = 0;
#if defined(ROBLOX_PS5_HOST_BUILD)
    return PalResult::failure(ENOSYS);
#else
    if (pad_handle_ < 0) {
        return PalResult::failure(ENODEV);
    }

    // Oversized 16-byte aligned buffer until exact PS5 ScePadData struct is
    // imported in Milestone 3; buttons word sits at offset 0 on Orbis/Prospero.
    alignas(16) uint8_t raw_pad[256]{};
    int32_t rc = scePadReadState(pad_handle_, raw_pad);
    if (rc != 0) {
        return PalResult::failure(rc);
    }

    std::memcpy(&out_buttons, raw_pad, sizeof(out_buttons));
    return PalResult::success();
#endif
}

} // namespace rbxps5::pal
