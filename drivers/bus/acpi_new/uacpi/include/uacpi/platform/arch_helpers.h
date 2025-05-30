#pragma once

#ifdef UACPI_OVERRIDE_ARCH_HELPERS
#include "uacpi_arch_helpers.h"
#else

#include <uacpi/platform/atomic.h>

#ifndef UACPI_ARCH_FLUSH_CPU_CACHE
#define UACPI_ARCH_FLUSH_CPU_CACHE() do {} while (0)
#endif

typedef unsigned long uacpi_cpu_flags;

typedef void *uacpi_thread_id;

/*
 * Replace as needed depending on your platform's way to represent thread ids.
 * uACPI offers a few more helpers like uacpi_atomic_{load,store}{8,16,32,64,ptr}
 * (or you could provide your own helpers)
 */
#ifndef UACPI_ATOMIC_LOAD_THREAD_ID
#define UACPI_ATOMIC_LOAD_THREAD_ID(ptr) ((uacpi_thread_id)uacpi_atomic_load_ptr(ptr))
#endif

#ifndef UACPI_ATOMIC_STORE_THREAD_ID
#define UACPI_ATOMIC_STORE_THREAD_ID(ptr, value) uacpi_atomic_store_ptr(ptr, value)
#endif

/*
 * A sentinel value that the kernel promises to NEVER return from
 * uacpi_kernel_get_current_thread_id or this will break
 */
#ifndef UACPI_THREAD_ID_NONE
#define UACPI_THREAD_ID_NONE ((uacpi_thread_id)-1)
#endif

#endif
