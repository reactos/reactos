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
#elif defined(__WATCOMC__)

#include <stdint.h>

static int uacpi_do_atomic_cmpxchg16(volatile uint16_t *ptr, volatile uint16_t *expected, uint16_t desired);
#pragma aux uacpi_do_atomic_cmpxchg16 = \
    ".486"                              \
    "mov ax, [esi]"                     \
    "lock cmpxchg [edi], bx"            \
    "mov [esi], ax"                     \
    "setz al"                           \
    "movzx eax, al"                     \
    parm [ edi ] [ esi ] [ ebx ]        \
    value [ eax ]

static int uacpi_do_atomic_cmpxchg32(volatile uint32_t *ptr, volatile uint32_t *expected, uint32_t desired);
#pragma aux uacpi_do_atomic_cmpxchg32 = \
    ".486"                              \
    "mov eax, [esi]"                    \
    "lock cmpxchg [edi], ebx"           \
    "mov [esi], eax"                    \
    "setz al"                           \
    "movzx eax, al"                     \
    parm [ edi ] [ esi ] [ ebx ]        \
    value [ eax ]

static int uacpi_do_atomic_cmpxchg64_asm(volatile uint64_t *ptr, volatile uint64_t *expected, uint32_t low, uint32_t high);
#pragma aux uacpi_do_atomic_cmpxchg64_asm = \
    ".586"                                  \
    "mov eax, [esi]"                        \
    "mov edx, [esi + 4]"                    \
    "lock cmpxchg8b [edi]"                  \
    "mov [esi], eax"                        \
    "mov [esi + 4], edx"                    \
    "setz al"                               \
    "movzx eax, al"                         \
    modify [ edx ]                          \
    parm [ edi ] [ esi ] [ ebx ] [ ecx ]    \
    value [ eax ]

static inline int uacpi_do_atomic_cmpxchg64(volatile uint64_t *ptr, volatile uint64_t *expected, uint64_t desired) {
    return uacpi_do_atomic_cmpxchg64_asm(ptr, expected, desired, desired >> 32);
}

#define uacpi_atomic_cmpxchg16(ptr, expected, desired) \
    uacpi_do_atomic_cmpxchg16((volatile uint16_t*)ptr, (volatile uint16_t*)expected, (uint16_t)desired)
#define uacpi_atomic_cmpxchg32(ptr, expected, desired) \
    uacpi_do_atomic_cmpxchg32((volatile uint32_t*)ptr, (volatile uint32_t*)expected, (uint32_t)desired)
#define uacpi_atomic_cmpxchg64(ptr, expected, desired) \
    uacpi_do_atomic_cmpxchg64((volatile uint64_t*)ptr, (volatile uint64_t*)expected, (uint64_t)desired)

static uint8_t uacpi_do_atomic_load8(volatile uint8_t *ptr);
#pragma aux uacpi_do_atomic_load8 = \
    "mov al, [esi]"                 \
    parm [ esi ]                    \
    value [ al ]

static uint16_t uacpi_do_atomic_load16(volatile uint16_t *ptr);
#pragma aux uacpi_do_atomic_load16 = \
    "mov ax, [esi]"                  \
    parm [ esi ]                     \
    value [ ax ]

static uint32_t uacpi_do_atomic_load32(volatile uint32_t *ptr);
#pragma aux uacpi_do_atomic_load32 = \
    "mov eax, [esi]"                 \
    parm [ esi ]                     \
    value [ eax ]

static void uacpi_do_atomic_load64_asm(volatile uint64_t *ptr, uint64_t *out);
#pragma aux uacpi_do_atomic_load64_asm =   \
    ".586"                                 \
    "xor eax, eax"                         \
    "xor ebx, ebx"                         \
    "xor ecx, ecx"                         \
    "xor edx, edx"                         \
    "lock cmpxchg8b [esi]"                 \
    "mov [edi], eax"                       \
    "mov [edi + 4], edx"                   \
    modify [ eax ebx ecx edx ]             \
    parm [ esi ] [ edi ]

static inline uint64_t uacpi_do_atomic_load64(volatile uint64_t *ptr) {
    uint64_t value;
    uacpi_do_atomic_load64_asm(ptr, &value);
    return value;
}

#define uacpi_atomic_load8(ptr) uacpi_do_atomic_load8((volatile uint8_t*)ptr)
#define uacpi_atomic_load16(ptr) uacpi_do_atomic_load16((volatile uint16_t*)ptr)
#define uacpi_atomic_load32(ptr) uacpi_do_atomic_load32((volatile uint32_t*)ptr)
#define uacpi_atomic_load64(ptr) uacpi_do_atomic_load64((volatile uint64_t*)ptr)

static void uacpi_do_atomic_store8(volatile uint8_t *ptr, uint8_t value);
#pragma aux uacpi_do_atomic_store8 = \
    "mov [edi], al"                  \
    parm [ edi ] [ eax ]

static void uacpi_do_atomic_store16(volatile uint16_t *ptr, uint16_t value);
#pragma aux uacpi_do_atomic_store16 = \
    "mov [edi], ax"                   \
    parm [ edi ] [ eax ]

static void uacpi_do_atomic_store32(volatile uint32_t *ptr, uint32_t value);
#pragma aux uacpi_do_atomic_store32 = \
    "mov [edi], eax"                  \
    parm [ edi ] [ eax ]

static void uacpi_do_atomic_store64_asm(volatile uint64_t *ptr, uint32_t low, uint32_t high);
#pragma aux uacpi_do_atomic_store64_asm =  \
    ".586"                                 \
    "xor eax, eax"                         \
    "xor edx, edx"                         \
    "retry: lock cmpxchg8b [edi]"          \
    "jnz retry"                            \
    modify [ eax edx ]                     \
    parm [ edi ] [ ebx ] [ ecx ]

static inline void uacpi_do_atomic_store64(volatile uint64_t *ptr, uint64_t value) {
    uacpi_do_atomic_store64_asm(ptr, value, value >> 32);
}

#define uacpi_atomic_store8(ptr, value) uacpi_do_atomic_store8((volatile uint8_t*)ptr, (uint8_t)value)
#define uacpi_atomic_store16(ptr, value) uacpi_do_atomic_store16((volatile uint16_t*)ptr, (uint16_t)value)
#define uacpi_atomic_store32(ptr, value) uacpi_do_atomic_store32((volatile uint32_t*)ptr, (uint32_t)value)
#define uacpi_atomic_store64(ptr, value) uacpi_do_atomic_store64((volatile uint64_t*)ptr, (uint64_t)value)

static uint16_t uacpi_do_atomic_inc16(volatile uint16_t *ptr);
#pragma aux uacpi_do_atomic_inc16 = \
    ".486"                          \
    "mov ax, 1"                     \
    "lock xadd [edi], ax"           \
    "add ax, 1"                     \
    parm [ edi ]                    \
    value [ ax ]

static uint32_t uacpi_do_atomic_inc32(volatile uint32_t *ptr);
#pragma aux uacpi_do_atomic_inc32 = \
    ".486"                          \
    "mov eax, 1"                    \
    "lock xadd [edi], eax"          \
    "add eax, 1"                    \
    parm [ edi ]                    \
    value [ eax ]

static void uacpi_do_atomic_inc64_asm(volatile uint64_t *ptr, uint64_t *out);
#pragma aux uacpi_do_atomic_inc64_asm = \
    ".586"                              \
    "xor eax, eax"                      \
    "xor edx, edx"                      \
    "mov ebx, 1"                        \
    "mov ecx, 1"                        \
    "retry: lock cmpxchg8b [esi]"       \
    "mov ebx, eax"                      \
    "mov ecx, edx"                      \
    "add ebx, 1"                        \
    "adc ecx, 0"                        \
    "jnz retry"                         \
    "mov [edi], ebx"                    \
    "mov [edi + 4], ecx"                \
    modify [ eax ebx ecx edx ]          \
    parm [ esi ] [ edi ]

static inline uint64_t uacpi_do_atomic_inc64(volatile uint64_t *ptr) {
    uint64_t value;
    uacpi_do_atomic_inc64_asm(ptr, &value);
    return value;
}

#define uacpi_atomic_inc16(ptr) uacpi_do_atomic_inc16((volatile uint16_t*)ptr)
#define uacpi_atomic_inc32(ptr) uacpi_do_atomic_inc32((volatile uint32_t*)ptr)
#define uacpi_atomic_inc64(ptr) uacpi_do_atomic_inc64((volatile uint64_t*)ptr)

static uint16_t uacpi_do_atomic_dec16(volatile uint16_t *ptr);
#pragma aux uacpi_do_atomic_dec16 = \
    ".486"                          \
    "mov ax, -1"                    \
    "lock xadd [edi], ax"           \
    "add ax, -1"                    \
    parm [ edi ]                    \
    value [ ax ]

static uint32_t uacpi_do_atomic_dec32(volatile uint32_t *ptr);
#pragma aux uacpi_do_atomic_dec32 = \
    ".486"                          \
    "mov eax, -1"                   \
    "lock xadd [edi], eax"          \
    "add eax, -1"                   \
    parm [ edi ]                    \
    value [ eax ]

static void uacpi_do_atomic_dec64_asm(volatile uint64_t *ptr, uint64_t *out);
#pragma aux uacpi_do_atomic_dec64_asm = \
    ".586"                              \
    "xor eax, eax"                      \
    "xor edx, edx"                      \
    "mov ebx, -1"                       \
    "mov ecx, -1"                       \
    "retry: lock cmpxchg8b [esi]"       \
    "mov ebx, eax"                      \
    "mov ecx, edx"                      \
    "sub ebx, 1"                        \
    "sbb ecx, 0"                        \
    "jnz retry"                         \
    "mov [edi], ebx"                    \
    "mov [edi + 4], ecx"                \
    modify [ eax ebx ecx edx ]          \
    parm [ esi ] [ edi ]

static inline uint64_t uacpi_do_atomic_dec64(volatile uint64_t *ptr) {
    uint64_t value;
    uacpi_do_atomic_dec64_asm(ptr, &value);
    return value;
}

#define uacpi_atomic_dec16(ptr) uacpi_do_atomic_dec16((volatile uint16_t*)ptr)
#define uacpi_atomic_dec32(ptr) uacpi_do_atomic_dec32((volatile uint32_t*)ptr)
#define uacpi_atomic_dec64(ptr) uacpi_do_atomic_dec64((volatile uint64_t*)ptr)
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
