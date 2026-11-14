// SPDX-License-Identifier: GPL-3.0-or-later
#include "pal/pal_fs.hpp"
#include "pal/pal_log.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <unistd.h>

int main() {
    namespace fs = std::filesystem;
    using namespace rbxps5::pal;

    const std::string base = "/tmp/roblox_ps5_pal_test_" + std::to_string(::getpid());
    const StorageLayout layout{
        base,
        base + "/logs",
        base + "/config",
        base + "/cache",
        base + "/apk",
    };

    if (!init_storage_layout(layout).ok()) {
        std::fprintf(stderr, "init_storage_layout failed\n");
        return 1;
    }

    if (!probe_persistent_rw(layout.logs_dir).ok()) {
        std::fprintf(stderr, "probe_persistent_rw failed\n");
        return 1;
    }

    Logger logger{};
    const std::string log_path = base + "/logs/test.log";
    if (!logger.init(LogConfig{log_path, {}, 0})) {
        std::fprintf(stderr, "Logger::init failed\n");
        return 1;
    }

    logger.write(LogLevel::Info, "TEST", "hello host logger");
    logger.writef(LogLevel::Warn, "TEST", "value=%d", 42);
    logger.shutdown();

    char buf[512]{};
    size_t n = 0;
    if (!read_small_file(log_path, buf, n).ok() || n == 0) {
        std::fprintf(stderr, "read_small_file failed\n");
        return 1;
    }

    std::error_code ec;
    fs::remove_all(base, ec);
    return 0;
}
