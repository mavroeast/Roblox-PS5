// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <string_view>

namespace rbxps5::pal {

enum class LogLevel : uint8_t {
    Info = 0,
    Warn = 1,
    Error = 2,
    Debug = 3,
};

struct LogConfig {
    std::string_view file_path{};
    std::string_view tcp_host{};
    uint16_t tcp_port{0};
};

class Logger {
public:
    Logger() noexcept = default;
    ~Logger() noexcept;

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    bool init(const LogConfig& cfg) noexcept;
    void shutdown() noexcept;

    void write(LogLevel level, std::string_view tag, std::string_view message) noexcept;
    void writef(LogLevel level, std::string_view tag, const char* fmt, ...) noexcept
        __attribute__((format(printf, 4, 5)));

    void flush() noexcept;

    [[nodiscard]] bool file_active() const noexcept { return file_fd_ >= 0; }
    [[nodiscard]] bool tcp_active() const noexcept { return sock_fd_ >= 0; }

private:
    void write_raw(std::string_view line) noexcept;
    static std::string_view level_str(LogLevel level) noexcept;

    int file_fd_{-1};
    int sock_fd_{-1};
};

} // namespace rbxps5::pal
