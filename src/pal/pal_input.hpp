// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "pal/pal_result.hpp"

#include <cstdint>

namespace rbxps5::pal {

class PadService {
public:
    PadService() noexcept = default;
    ~PadService() noexcept;

    PadService(const PadService&) = delete;
    PadService& operator=(const PadService&) = delete;

    PalResult init() noexcept;
    void shutdown() noexcept;

    // Reads raw state into an opaque aligned buffer (Milestone 3 will decode
    // full DualSense fields once ScePadData layout is verified on SDK headers).
    PalResult poll_raw_buttons(uint32_t& out_buttons) noexcept;

    [[nodiscard]] int32_t initial_user_id() const noexcept { return user_id_; }
    [[nodiscard]] int32_t pad_handle() const noexcept { return pad_handle_; }
    [[nodiscard]] bool is_open() const noexcept { return pad_handle_ >= 0; }

private:
    bool user_service_inited_{false};
    int32_t user_id_{-1};
    int32_t pad_handle_{-1};
};

} // namespace rbxps5::pal
