// SPDX-License-Identifier: GPL-3.0-or-later
#include "pal/pal_fs.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace rbxps5::pal {

namespace {

bool copy_cstr(std::string_view src, char* dst, size_t dst_cap) noexcept {
    if (src.empty() || src.size() >= dst_cap) {
        return false;
    }
    std::memcpy(dst, src.data(), src.size());
    dst[src.size()] = '\0';
    return true;
}

} // namespace

PalResult ensure_directory(std::string_view path) noexcept {
    char buf[512];
    if (!copy_cstr(path, buf, sizeof(buf))) {
        return PalResult::failure(EINVAL);
    }

    if (::mkdir(buf, 0777) == 0) {
        ::chmod(buf, 0777);
        return PalResult::success();
    }

    if (errno == EEXIST) {
        struct stat st{};
        if (::stat(buf, &st) == 0 && S_ISDIR(st.st_mode)) {
            return PalResult::success();
        }
        return PalResult::failure(ENOTDIR);
    }

    return PalResult::failure(errno);
}

PalResult init_storage_layout(const StorageLayout& layout) noexcept {
    const std::string_view dirs[] = {
        layout.root,
        layout.logs_dir,
        layout.config_dir,
        layout.cache_dir,
        layout.apk_dir,
    };

    for (std::string_view dir : dirs) {
        PalResult res = ensure_directory(dir);
        if (!res.ok()) {
            return res;
        }
    }
    return PalResult::success();
}

PalResult read_small_file(std::string_view path,
                          std::span<char> out_buf,
                          size_t& out_bytes_read) noexcept {
    out_bytes_read = 0;
    if (out_buf.empty()) {
        return PalResult::failure(EINVAL);
    }

    char path_buf[512];
    if (!copy_cstr(path, path_buf, sizeof(path_buf))) {
        return PalResult::failure(EINVAL);
    }

    int fd = ::open(path_buf, O_RDONLY);
    if (fd < 0) {
        return PalResult::failure(errno);
    }

    size_t total = 0;
    const size_t max_read = out_buf.size() - 1;
    while (total < max_read) {
        ssize_t n = ::read(fd, out_buf.data() + total, max_read - total);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            int err = errno;
            ::close(fd);
            return PalResult::failure(err);
        }
        if (n == 0) {
            break;
        }
        total += static_cast<size_t>(n);
    }

    ::close(fd);
    out_buf[total] = '\0';
    out_bytes_read = total;
    return PalResult::success();
}

PalResult write_file_atomic_like(std::string_view path,
                                 std::span<const std::byte> data) noexcept {
    char path_buf[512];
    if (!copy_cstr(path, path_buf, sizeof(path_buf))) {
        return PalResult::failure(EINVAL);
    }

    int fd = ::open(path_buf, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        return PalResult::failure(errno);
    }

    size_t written = 0;
    while (written < data.size()) {
        ssize_t n = ::write(fd,
                            reinterpret_cast<const char*>(data.data()) + written,
                            data.size() - written);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            int err = errno;
            ::close(fd);
            return PalResult::failure(err);
        }
        written += static_cast<size_t>(n);
    }

    ::fchmod(fd, 0666);
    ::fsync(fd);
    ::close(fd);
    return PalResult::success();
}

PalResult probe_persistent_rw(std::string_view directory) noexcept {
    char probe_path[512];
    int n = std::snprintf(probe_path,
                          sizeof(probe_path),
                          "%.*s/.rw_probe.tmp",
                          static_cast<int>(directory.size()),
                          directory.data());
    if (n <= 0 || static_cast<size_t>(n) >= sizeof(probe_path)) {
        return PalResult::failure(EINVAL);
    }

    static constexpr char kMagic[] = "ROBLOX_PS5_RW_OK";
    PalResult wr = write_file_atomic_like(
        probe_path,
        std::span<const std::byte>{reinterpret_cast<const std::byte*>(kMagic), sizeof(kMagic) - 1});
    if (!wr.ok()) {
        return wr;
    }

    char readback[32]{};
    size_t bytes_read = 0;
    PalResult rd = read_small_file(probe_path, std::span<char>{readback}, bytes_read);
    ::unlink(probe_path);

    if (!rd.ok()) {
        return rd;
    }
    if (bytes_read != sizeof(kMagic) - 1 || std::memcmp(readback, kMagic, bytes_read) != 0) {
        return PalResult::failure(EIO);
    }
    return PalResult::success();
}

} // namespace rbxps5::pal
