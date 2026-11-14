// SPDX-License-Identifier: GPL-3.0-or-later
#include "pal/pal_log.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

namespace rbxps5::pal {

namespace {

void write_all_fd(int fd, const char* data, size_t len) noexcept {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = ::write(fd, data + sent, len - sent);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) {
                continue;
            }
            break;
        }
        sent += static_cast<size_t>(n);
    }
}

int connect_tcp_best_effort(std::string_view host, uint16_t port) noexcept {
    if (host.empty() || port == 0 || host.size() >= 64) {
        return -1;
    }

    char host_buf[64]{};
    std::memcpy(host_buf, host.data(), host.size());

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, host_buf, &addr.sin_addr) != 1) {
        return -1;
    }

    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }

    struct timeval tv{};
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    if (::connect(fd, reinterpret_cast<const struct sockaddr*>(&addr), sizeof(addr)) != 0) {
        ::close(fd);
        return -1;
    }

    return fd;
}

} // namespace

Logger::~Logger() noexcept {
    shutdown();
}

bool Logger::init(const LogConfig& cfg) noexcept {
    shutdown();

    if (!cfg.file_path.empty() && cfg.file_path.size() < 512) {
        char path_buf[512]{};
        std::memcpy(path_buf, cfg.file_path.data(), cfg.file_path.size());
        file_fd_ = ::open(path_buf, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    }

    if (!cfg.tcp_host.empty() && cfg.tcp_port != 0) {
        sock_fd_ = connect_tcp_best_effort(cfg.tcp_host, cfg.tcp_port);
    }

    return file_fd_ >= 0 || sock_fd_ >= 0;
}

void Logger::shutdown() noexcept {
    if (file_fd_ >= 0) {
        ::fsync(file_fd_);
        ::close(file_fd_);
        file_fd_ = -1;
    }
    if (sock_fd_ >= 0) {
        ::close(sock_fd_);
        sock_fd_ = -1;
    }
}

std::string_view Logger::level_str(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warn:
        return "WARN";
    case LogLevel::Error:
        return "ERROR";
    case LogLevel::Debug:
        return "DEBUG";
    }
    return "INFO";
}

void Logger::write_raw(std::string_view line) noexcept {
    if (file_fd_ >= 0) {
        write_all_fd(file_fd_, line.data(), line.size());
    }
    if (sock_fd_ >= 0) {
        ssize_t n = ::send(sock_fd_, line.data(), line.size(), 0);
        if (n < 0 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
            ::close(sock_fd_);
            sock_fd_ = -1;
        }
    }
}

void Logger::write(LogLevel level, std::string_view tag, std::string_view message) noexcept {
    struct timespec ts{};
    ::clock_gettime(CLOCK_MONOTONIC, &ts);

    char buf[1024];
    const auto lvl = level_str(level);
    int prefix_len = std::snprintf(
        buf,
        sizeof(buf),
        "[%lld.%03ld][%.*s][%.*s] %.*s\n",
        static_cast<long long>(ts.tv_sec),
        ts.tv_nsec / 1000000L,
        static_cast<int>(lvl.size()),
        lvl.data(),
        static_cast<int>(tag.size()),
        tag.data(),
        static_cast<int>(message.size()),
        message.data());

    if (prefix_len <= 0) {
        return;
    }

    size_t len = static_cast<size_t>(prefix_len);
    if (len >= sizeof(buf)) {
        len = sizeof(buf) - 1;
        buf[len - 1] = '\n';
    }
    write_raw(std::string_view{buf, len});
}

void Logger::writef(LogLevel level, std::string_view tag, const char* fmt, ...) noexcept {
    char msg[768];
    va_list args;
    va_start(args, fmt);
    int n = std::vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    if (n < 0) {
        return;
    }
    size_t msg_len = static_cast<size_t>(n);
    if (msg_len >= sizeof(msg)) {
        msg_len = sizeof(msg) - 1;
    }
    write(level, tag, std::string_view{msg, msg_len});
}

void Logger::flush() noexcept {
    if (file_fd_ >= 0) {
        ::fsync(file_fd_);
    }
}

} // namespace rbxps5::pal
