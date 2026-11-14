cmake_minimum_required(VERSION 3.20)
project(RobloxPS5HostTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_executable(roblox_ps5_host_tests
    tests/test_pal_host.cpp
    src/pal/pal_log.cpp
    src/pal/pal_fs.cpp
)

target_include_directories(roblox_ps5_host_tests PRIVATE src)
target_compile_definitions(roblox_ps5_host_tests PRIVATE ROBLOX_PS5_HOST_BUILD=1)
target_compile_options(roblox_ps5_host_tests PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -fno-exceptions -fno-rtti
)

add_test(NAME pal_host_tests COMMAND roblox_ps5_host_tests)
