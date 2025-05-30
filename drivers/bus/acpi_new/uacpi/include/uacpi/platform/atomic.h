#pragma once

/*
 * Most of this header is a giant workaround for MSVC to make atomics into a
 * somewhat unified interface with how GCC and Clang handle them.
 *
 * We don't use the absolutely disgusting C11 stdatomic.h header because it is
 * unable to operate on non _Atomic types, which enforce implicit sequential
 * consistency and alter the behavior of the standard C binary/unary operators.
 *
 * The strictness of the atomic helpers defined here is assumed to be at least
 * acquire for loads and release for stores. Cmpxchg uses the standard acq/rel
 * for success, acq for failure, and is assumed to be strong.
 */

#ifdef UACPI_OVERRIDE_ATOMIC
#include "uacpi_atomic.h"
#else

#include <uacpi/platform/compiler.h>

#if defined(_MSC_VER) && !defined(__clang__)

#include <intrin.h>

// mimic __atomic_compare_exchange_n that doesn't exist on MSVC
#define UACPI_MAKE_MSVC_CMPXCHG(width, type, suffix)                            \
    static inline int uacpi_do_atomic_cmpxchg##width(                           \
        type volatile *ptr, type volatile *expected, type desired               \
    )                                                                           \
    {                                                                           \
        type current;                                                           \
                                                                                \
        current = _InterlockedCompareExchange##suffix(ptr, *expected, desired); \
        if (current != *expected) {                                             \
            *expected = current;                                                \
            return 0;                                                           \
        }                                                                       \
        return 1;                                                               \
    }

#define UACPI_MSVC_CMPXCHG_INVOKE(ptr, expected, desired, width, type) \
    uacpi_do_atomic_cmpxchg##width(                                    \
        (type volatile*)ptr, (type volatile*)expected, desired         \
    )

#define UACPI_MSVC_ATOMIC_STORE(ptr, value, type, width) \
    _InterlockedExchange##width((type volatile*)(ptr), (type)(value))

#define UACPI_MSVC_ATOMIC_LOAD(ptr, type, width) \
    _InterlockedOr##width((type volatile*)(ptr), 0)

#define UACPI_MSVC_ATOMIC_INC(ptr, type, width) \
    _InterlockedIncrement##width((type volatile*)(ptr))

#define UACPI_MSVC_ATOMIC_DEC(ptr, type, width) \
    _InterlockedDecrement##width((type volatile*)(ptr))

UACPI_MAKE_MSVC_CMPXCHG(64, __int64, 64)
UACPI_MAKE_MSVC_CMPXCHG(32, long,)
UACPI_MAKE_MSVC_CMPXCHG(16, short, 16)

#define uacpi_atomic_cmpxchg16(ptr, expected, desired) \
    UACPI_MSVC_CMPXCHG_INVOKE(ptr, expected, desired, 16, short)

#define uacpi_atomic_cmpxchg32(ptr, expected, desired) \
    UACPI_MSVC_CMPXCHG_INVOKE(ptr, expected, desired, 32, long)

#define uacpi_atomic_cmpxchg64(ptr, expected, desired) \
    UACPI_MSVC_CMPXCHG_INVOKE(ptr, expected, desired, 64, __int64)

#define uacpi_atomic_load8(ptr) UACPI_MSVC_ATOMIC_LOAD(ptr, char, 8)
#define uacpi_atomic_load16(ptr) UACPI_MSVC_ATOMIC_LOAD(ptr, short, 16)
#define uacpi_atomic_load32(ptr) UACPI_MSVC_ATOMIC_LOAD(ptr, long,)
#define uacpi_atomic_load64(ptr) UACPI_MSVC_ATOMIC_LOAD(ptr, __int64, 64)

#define uacpi_atomic_store8(ptr, value) UACPI_MSVC_ATOMIC_STORE(ptr, value, char, 8)
#define uacpi_atomic_store16(ptr, value) UACPI_MSVC_ATOMIC_STORE(ptr, value, short, 16)
#define uacpi_atomic_store32(ptr, value) UACPI_MSVC_ATOMIC_STORE(ptr, value, long,)
#define uacpi_atomic_store64(ptr, value) UACPI_MSVC_ATOMIC_STORE(ptr, value, __int64, 64)

#define uacpi_atomic_inc16(ptr) UACPI_MSVC_ATOMIC_INC(ptr, short, 16)
#define uacpi_atomic_inc32(ptr) UACPI_MSVC_ATOMIC_INC(ptr, long,)
#define uacpi_atomic_inc64(ptr) UACPI_MSVC_ATOMIC_INC(ptr, __int64, 64)

#define uacpi_atomic_dec16(ptr) UACPI_MSVC_ATOMIC_DEC(ptr, short, 16)
#define uacpi_atomic_dec32(ptr) UACPI_MSVC_ATOMIC_DEC(ptr, long,)
#define uacpi_atomic_dec64(ptr) UACPI_MSVC_ATOMIC_DEC(ptr, __int64, 64)
#else

#define UACPI_DO_CMPXCHG(ptr, expected, desired)           \
    __atomic_compare_exchange_n(ptr, expected, desired, 0, \
                                __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)

#define uacpi_atomic_cmpxchg16(ptr, expected, desired) \
    UACPI_DO_CMPXCHG(ptr, expected, desired)
#define uacpi_atomic_cmpxchg32(ptr, expected, desired) \
    UACPI_DO_CMPXCHG(ptr, expected, desired)
#define uacpi_atomic_cmpxchg64(ptr, expected, desired) \
    UACPI_DO_CMPXCHG(ptr, expected, desired)

#define uacpi_atomic_load8(ptr) __atomic_load_n(ptr, __ATOMIC_ACQUIRE)
#define uacpi_atomic_load16(ptr) __atomic_load_n(ptr, __ATOMIC_ACQUIRE)
#define uacpi_atomic_load32(ptr) __atomic_load_n(ptr, __ATOMIC_ACQUIRE)
#define uacpi_atomic_load64(ptr) __atomic_load_n(ptr, __ATOMIC_ACQUIRE)

#define uacpi_atomic_store8(ptr, value) __atomic_store_n(ptr, value, __ATOMIC_RELEASE)
#define uacpi_atomic_store16(ptr, value) __atomic_store_n(ptr, value, __ATOMIC_RELEASE)
#define uacpi_atomic_store32(ptr, value) __atomic_store_n(ptr, value, __ATOMIC_RELEASE)
#define uacpi_atomic_store64(ptr, value) __atomic_store_n(ptr, value, __ATOMIC_RELEASE)

#define uacpi_atomic_inc16(ptr) __atomic_add_fetch(ptr, 1, __ATOMIC_ACQ_REL)
#define uacpi_atomic_inc32(ptr) __atomic_add_fetch(ptr, 1, __ATOMIC_ACQ_REL)
#define uacpi_atomic_inc64(ptr) __atomic_add_fetch(ptr, 1, __ATOMIC_ACQ_REL)

#define uacpi_atomic_dec16(ptr) __atomic_sub_fetch(ptr, 1, __ATOMIC_ACQ_REL)
#define uacpi_atomic_dec32(ptr) __atomic_sub_fetch(ptr, 1, __ATOMIC_ACQ_REL)
#define uacpi_atomic_dec64(ptr) __atomic_sub_fetch(ptr, 1, __ATOMIC_ACQ_REL)
#endif

#if UACPI_POINTER_SIZE == 4
#define uacpi_atomic_load_ptr(ptr_to_ptr) uacpi_atomic_load32(ptr_to_ptr)
#define uacpi_atomic_store_ptr(ptr_to_ptr, value) uacpi_atomic_store32(ptr_to_ptr, value)
#else
#define uacpi_atomic_load_ptr(ptr_to_ptr) uacpi_atomic_load64(ptr_to_ptr)
#define uacpi_atomic_store_ptr(ptr_to_ptr, value) uacpi_atomic_store64(ptr_to_ptr, value)
#endif

#endif
