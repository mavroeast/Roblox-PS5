// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <cstdint>
#include <sys/types.h>

#if !defined(ROBLOX_PS5_HOST_BUILD)

// Minimal C ABI declarations for PS5 runtime .sprx exports resolved by
// ps5-payload-dev/sdk sce_stubs or runtime dlsym(). Kept isolated here so
// higher layers do not depend on raw Sony symbol names.
extern "C" {

// libkernel / libkernel_sys.sprx
int32_t sceKernelAllocateDirectMemory(off_t search_start,
                                      off_t search_end,
                                      size_t length,
                                      size_t alignment,
                                      int32_t memory_type,
                                      off_t* phys_addr_out);
int32_t sceKernelMapDirectMemory(void** virtual_addr,
                                 size_t length,
                                 int32_t protection,
                                 int32_t flags,
                                 off_t phys_addr,
                                 size_t alignment);
int32_t sceKernelReserveVirtualRange(void** virtual_addr,
                                     size_t length,
                                     int32_t flags,
                                     size_t alignment);
int32_t sceKernelMprotect(const void* addr, size_t size, int32_t prot);
int32_t sceKernelMunmap(void* addr, size_t size);
int32_t sceKernelReleaseDirectMemory(off_t phys_addr, size_t length);
size_t sceKernelGetDirectMemorySize(void);
int32_t sceKernelUsleep(uint32_t microseconds);

// libSceSystemService.sprx
int32_t sceSystemServiceHideSplashScreen(void);

// libSceUserService.sprx
int32_t sceUserServiceInitialize(const void* init_params);
int32_t sceUserServiceTerminate(void);
int32_t sceUserServiceGetInitialUser(int32_t* user_id_out);

// libScePad.sprx
int32_t scePadInit(void);
int32_t scePadOpen(int32_t user_id, int32_t type, int32_t index, const void* param);
int32_t scePadClose(int32_t handle);
int32_t scePadReadState(int32_t handle, void* pad_data);

} // extern "C"

#endif // !ROBLOX_PS5_HOST_BUILD
