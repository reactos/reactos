/*
 * Copyright 2016 Józef Kucia for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __VKD3D_COMMON_H
#define __VKD3D_COMMON_H

#include "config.h"
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "vkd3d_types.h"

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <pthread.h>
#endif

#ifdef _MSC_VER
#include <intrin.h>
#endif

#ifndef ARRAY_SIZE
# define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))
#endif

#define DIV_ROUND_UP(a, b) ((a) % (b) == 0 ? (a) / (b) : (a) / (b) + 1)

#define STATIC_ASSERT(e) extern void __VKD3D_STATIC_ASSERT__(int [(e) ? 1 : -1])

#define VKD3D_ASSERT(cond) \
        do { \
            if (!(cond)) \
                ERR("Failed assertion: %s\n", #cond); \
        } while (0)

#define MEMBER_SIZE(t, m) sizeof(((t *)0)->m)

#define VKD3D_MAKE_TAG(ch0, ch1, ch2, ch3) \
        ((uint32_t)(ch0) | ((uint32_t)(ch1) << 8) \
        | ((uint32_t)(ch2) << 16) | ((uint32_t)(ch3) << 24))

#define VKD3D_EXPAND(x) x
#define VKD3D_STRINGIFY(x) #x
#define VKD3D_EXPAND_AND_STRINGIFY(x) VKD3D_EXPAND(VKD3D_STRINGIFY(x))

#define vkd3d_clamp(value, lower, upper) max(min(value, upper), lower)

#define TAG_AON9 VKD3D_MAKE_TAG('A', 'o', 'n', '9')
#define TAG_DXBC VKD3D_MAKE_TAG('D', 'X', 'B', 'C')
#define TAG_DXIL VKD3D_MAKE_TAG('D', 'X', 'I', 'L')
#define TAG_FX10 VKD3D_MAKE_TAG('F', 'X', '1', '0')
#define TAG_ISG1 VKD3D_MAKE_TAG('I', 'S', 'G', '1')
#define TAG_ISGN VKD3D_MAKE_TAG('I', 'S', 'G', 'N')
#define TAG_OSG1 VKD3D_MAKE_TAG('O', 'S', 'G', '1')
#define TAG_OSG5 VKD3D_MAKE_TAG('O', 'S', 'G', '5')
#define TAG_OSGN VKD3D_MAKE_TAG('O', 'S', 'G', 'N')
#define TAG_PCSG VKD3D_MAKE_TAG('P', 'C', 'S', 'G')
#define TAG_PSG1 VKD3D_MAKE_TAG('P', 'S', 'G', '1')
#define TAG_RD11 VKD3D_MAKE_TAG('R', 'D', '1', '1')
#define TAG_RDEF VKD3D_MAKE_TAG('R', 'D', 'E', 'F')
#define TAG_RTS0 VKD3D_MAKE_TAG('R', 'T', 'S', '0')
#define TAG_SDBG VKD3D_MAKE_TAG('S', 'D', 'B', 'G')
#define TAG_SFI0 VKD3D_MAKE_TAG('S', 'F', 'I', '0')
#define TAG_SHDR VKD3D_MAKE_TAG('S', 'H', 'D', 'R')
#define TAG_SHEX VKD3D_MAKE_TAG('S', 'H', 'E', 'X')
#define TAG_STAT VKD3D_MAKE_TAG('S', 'T', 'A', 'T')
#define TAG_TEXT VKD3D_MAKE_TAG('T', 'E', 'X', 'T')
#define TAG_XNAP VKD3D_MAKE_TAG('X', 'N', 'A', 'P')
#define TAG_XNAS VKD3D_MAKE_TAG('X', 'N', 'A', 'S')

#define TAG_RD11_REVERSE 0x25441313

static inline uint64_t align(uint64_t addr, size_t alignment)
{
    return (addr + (alignment - 1)) & ~(alignment - 1);
}

#if defined(__GNUC__) || defined(__clang__)
# define VKD3D_NORETURN __attribute__((noreturn))
# ifdef __MINGW_PRINTF_FORMAT
#  define VKD3D_PRINTF_FUNC(fmt, args) __attribute__((format(__MINGW_PRINTF_FORMAT, fmt, args)))
# else
#  define VKD3D_PRINTF_FUNC(fmt, args) __attribute__((format(printf, fmt, args)))
# endif
# define VKD3D_UNUSED __attribute__((unused))
# define VKD3D_UNREACHABLE __builtin_unreachable()
#else
# define VKD3D_NORETURN
# define VKD3D_PRINTF_FUNC(fmt, args)
# define VKD3D_UNUSED
# define VKD3D_UNREACHABLE (void)0
#endif  /* __GNUC__ */

#define vkd3d_unreachable() \
        do { \
            ERR("%s:%u: Unreachable code reached.\n", __FILE__, __LINE__); \
            VKD3D_UNREACHABLE; \
        } while (0)

#ifdef VKD3D_NO_TRACE_MESSAGES
#define TRACE(args...) do { } while (0)
#define TRACE_ON() (false)
#endif

#ifdef VKD3D_NO_DEBUG_MESSAGES
#define WARN(args...) do { } while (0)
#define FIXME(args...) do { } while (0)
#define WARN_ON() (false)
#define FIXME_ONCE(args...) do { } while (0)
#endif

#ifdef VKD3D_NO_ERROR_MESSAGES
#define ERR(args...) do { } while (0)
#define MESSAGE(args...) do { } while (0)
#endif

enum vkd3d_dbg_level
{
    VKD3D_DBG_LEVEL_NONE,
    VKD3D_DBG_LEVEL_MESSAGE,
    VKD3D_DBG_LEVEL_ERR,
    VKD3D_DBG_LEVEL_FIXME,
    VKD3D_DBG_LEVEL_WARN,
    VKD3D_DBG_LEVEL_TRACE,
};

enum vkd3d_dbg_level vkd3d_dbg_get_level(void);

void vkd3d_dbg_printf(enum vkd3d_dbg_level level, const char *function, const char *fmt, ...) VKD3D_PRINTF_FUNC(3, 4);
void vkd3d_dbg_set_log_callback(PFN_vkd3d_log callback);

const char *vkd3d_dbg_sprintf(const char *fmt, ...) VKD3D_PRINTF_FUNC(1, 2);
const char *vkd3d_dbg_vsprintf(const char *fmt, va_list args);
const char *debugstr_a(const char *str);
const char *debugstr_an(const char *str, size_t n);
const char *debugstr_w(const WCHAR *wstr, size_t wchar_size);

#define VKD3D_DBG_LOG(level) \
        do { \
        const enum vkd3d_dbg_level vkd3d_dbg_level = VKD3D_DBG_LEVEL_##level; \
        VKD3D_DBG_PRINTF_##level

#define VKD3D_DBG_LOG_ONCE(first_time_level, level) \
        do { \
        static bool vkd3d_dbg_next_time; \
        const enum vkd3d_dbg_level vkd3d_dbg_level = vkd3d_dbg_next_time \
        ? VKD3D_DBG_LEVEL_##level : VKD3D_DBG_LEVEL_##first_time_level; \
        vkd3d_dbg_next_time = true; \
        VKD3D_DBG_PRINTF_##level

#define VKD3D_DBG_PRINTF(...) \
        vkd3d_dbg_printf(vkd3d_dbg_level, __FUNCTION__, __VA_ARGS__); } while (0)

#define VKD3D_DBG_PRINTF_TRACE(...) VKD3D_DBG_PRINTF(__VA_ARGS__)
#define VKD3D_DBG_PRINTF_WARN(...) VKD3D_DBG_PRINTF(__VA_ARGS__)
#define VKD3D_DBG_PRINTF_FIXME(...) VKD3D_DBG_PRINTF(__VA_ARGS__)
#define VKD3D_DBG_PRINTF_MESSAGE(...) VKD3D_DBG_PRINTF(__VA_ARGS__)

#ifdef VKD3D_ABORT_ON_ERR
#define VKD3D_DBG_PRINTF_ERR(...) \
        vkd3d_dbg_printf(vkd3d_dbg_level, __FUNCTION__, __VA_ARGS__); \
        abort(); \
        } while (0)
#else
#define VKD3D_DBG_PRINTF_ERR(...) VKD3D_DBG_PRINTF(__VA_ARGS__)
#endif

/* Used by vkd3d_unreachable(). */
#ifdef VKD3D_CROSSTEST
#undef ERR
#define ERR(...) do { fprintf(stderr, __VA_ARGS__); abort(); } while (0)
#endif

#ifndef TRACE
#define TRACE   VKD3D_DBG_LOG(TRACE)
#endif

#ifndef WARN
#define WARN    VKD3D_DBG_LOG(WARN)
#endif

#ifndef FIXME
#define FIXME   VKD3D_DBG_LOG(FIXME)
#endif

#ifndef ERR
#define ERR     VKD3D_DBG_LOG(ERR)
#endif

#ifndef MESSAGE
#define MESSAGE VKD3D_DBG_LOG(MESSAGE)
#endif

#ifndef TRACE_ON
#define TRACE_ON() (vkd3d_dbg_get_level() == VKD3D_DBG_LEVEL_TRACE)
#endif

#ifndef WARN_ON
#define WARN_ON() (vkd3d_dbg_get_level() >= VKD3D_DBG_LEVEL_WARN)
#endif

#ifndef FIXME_ONCE
#define FIXME_ONCE VKD3D_DBG_LOG_ONCE(FIXME, WARN)
#endif

#define VKD3D_DEBUG_ENV_NAME(name) const char *const vkd3d_dbg_env_name = name

static inline const char *debugstr_guid(const GUID *guid)
{
    if (!guid)
        return "(null)";

    return vkd3d_dbg_sprintf("{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
            (unsigned long)guid->Data1, guid->Data2, guid->Data3, guid->Data4[0],
            guid->Data4[1], guid->Data4[2], guid->Data4[3], guid->Data4[4],
            guid->Data4[5], guid->Data4[6], guid->Data4[7]);
}

static inline const char *debugstr_hresult(HRESULT hr)
{
    switch (hr)
    {
#define TO_STR(u) case u: return #u;
        TO_STR(S_OK)
        TO_STR(S_FALSE)
        TO_STR(E_NOTIMPL)
        TO_STR(E_NOINTERFACE)
        TO_STR(E_POINTER)
        TO_STR(E_ABORT)
        TO_STR(E_FAIL)
        TO_STR(E_OUTOFMEMORY)
        TO_STR(E_INVALIDARG)
        TO_STR(DXGI_ERROR_NOT_FOUND)
        TO_STR(DXGI_ERROR_MORE_DATA)
        TO_STR(DXGI_ERROR_UNSUPPORTED)
#undef TO_STR
        default:
            return vkd3d_dbg_sprintf("%#x", (int)hr);
    }
}

unsigned int vkd3d_env_var_as_uint(const char *name, unsigned int default_value);

struct vkd3d_debug_option
{
    const char *name;
    uint64_t flag;
};

bool vkd3d_debug_list_has_member(const char *string, const char *member);
uint64_t vkd3d_parse_debug_options(const char *string,
        const struct vkd3d_debug_option *options, unsigned int option_count);
void vkd3d_set_thread_name(const char *name);

static inline unsigned int vkd3d_popcount(unsigned int v)
{
#ifdef _MSC_VER
    return __popcnt(v);
#elif defined(__MINGW32__)
    return __builtin_popcount(v);
#else
    v -= (v >> 1) & 0x55555555;
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
    return (((v + (v >> 4)) & 0x0f0f0f0f) * 0x01010101) >> 24;
#endif
}

static inline bool vkd3d_bitmask_is_contiguous(unsigned int mask)
{
    unsigned int i, j;

    for (i = 0, j = 0; i < sizeof(mask) * CHAR_BIT; ++i)
    {
        if (mask & (1u << i))
            ++j;
        else if (j)
            break;
    }

    return vkd3d_popcount(mask) == j;
}

/* Undefined for x == 0. */
static inline unsigned int vkd3d_log2i(unsigned int x)
{
#ifdef _WIN32
    /* _BitScanReverse returns the index of the highest set bit,
     * unlike clz which is 31 - index. */
    ULONG result;
    _BitScanReverse(&result, x);
    return (unsigned int)result;
#elif defined(HAVE_BUILTIN_CLZ)
    return __builtin_clz(x) ^ 0x1f;
#else
    static const unsigned int l[] =
    {
        ~0u, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
          4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
          5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
          5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
          6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
          6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
          6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
          6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    };
    unsigned int i;

    return (i = x >> 16) ? (x = i >> 8) ? l[x] + 24
            : l[i] + 16 : (i = x >> 8) ? l[i] + 8 : l[x];
#endif
}

static inline void *vkd3d_memmem( const void *haystack, size_t haystack_len, const void *needle, size_t needle_len)
{
    const char *str = haystack;

    while (haystack_len >= needle_len)
    {
        if (!memcmp(str, needle, needle_len))
            return (char *)str;
        ++str;
        --haystack_len;
    }
    return NULL;
}

static inline bool vkd3d_bound_range(size_t start, size_t count, size_t limit)
{
#ifdef HAVE_BUILTIN_ADD_OVERFLOW
    size_t sum;

    return !__builtin_add_overflow(start, count, &sum) && sum <= limit;
#else
    return start <= limit && count <= limit - start;
#endif
}

static inline bool vkd3d_object_range_overflow(size_t start, size_t count, size_t size)
{
    return (~(size_t)0 - start) / size < count;
}

static inline uint16_t vkd3d_make_u16(uint8_t low, uint8_t high)
{
    return low | ((uint16_t)high << 8);
}

static inline uint32_t vkd3d_make_u32(uint16_t low, uint16_t high)
{
    return low | ((uint32_t)high << 16);
}

static inline int vkd3d_u32_compare(uint32_t x, uint32_t y)
{
    return (x > y) - (x < y);
}

static inline int vkd3d_u64_compare(uint64_t x, uint64_t y)
{
    return (x > y) - (x < y);
}

#define VKD3D_BITMAP_SIZE(x) (((x) + 0x1f) >> 5)

static inline bool bitmap_clear(uint32_t *map, unsigned int idx)
{
    return map[idx >> 5] &= ~(1u << (idx & 0x1f));
}

static inline bool bitmap_set(uint32_t *map, unsigned int idx)
{
    return map[idx >> 5] |= (1u << (idx & 0x1f));
}

static inline bool bitmap_is_set(const uint32_t *map, unsigned int idx)
{
    return map[idx >> 5] & (1u << (idx & 0x1f));
}

static inline int ascii_isupper(int c)
{
    return 'A' <= c && c <= 'Z';
}

static inline int ascii_tolower(int c)
{
    return ascii_isupper(c) ? c - 'A' + 'a' : c;
}

static inline int ascii_strncasecmp(const char *a, const char *b, size_t n)
{
    int c_a, c_b;

    while (n--)
    {
        c_a = ascii_tolower(*a++);
        c_b = ascii_tolower(*b++);
        if (c_a != c_b || !c_a)
            return c_a - c_b;
    }
    return 0;
}

static inline int ascii_strcasecmp(const char *a, const char *b)
{
    int c_a, c_b;

    do
    {
        c_a = ascii_tolower(*a++);
        c_b = ascii_tolower(*b++);
    } while (c_a == c_b && c_a != '\0');

    return c_a - c_b;
}

static inline uint64_t vkd3d_atomic_add_fetch_u64(uint64_t volatile *x, uint64_t val)
{
#if HAVE_SYNC_ADD_AND_FETCH
    return __sync_add_and_fetch(x, val);
#elif defined(_WIN32)
    return InterlockedAdd64((LONG64 *)x, val);
#else
# error "vkd3d_atomic_add_fetch_u64() not implemented for this platform"
#endif
}

static inline uint32_t vkd3d_atomic_add_fetch_u32(uint32_t volatile *x, uint32_t val)
{
#if HAVE_SYNC_ADD_AND_FETCH
    return __sync_add_and_fetch(x, val);
#elif defined(_WIN32)
    return InterlockedAdd((LONG *)x, val);
#else
# error "vkd3d_atomic_add_fetch_u32() not implemented for this platform"
#endif
}

static inline uint64_t vkd3d_atomic_increment_u64(uint64_t volatile *x)
{
    return vkd3d_atomic_add_fetch_u64(x, 1);
}

static inline uint32_t vkd3d_atomic_decrement_u32(uint32_t volatile *x)
{
    return vkd3d_atomic_add_fetch_u32(x, ~0u);
}

static inline uint32_t vkd3d_atomic_increment_u32(uint32_t volatile *x)
{
    return vkd3d_atomic_add_fetch_u32(x, 1);
}

static inline bool vkd3d_atomic_compare_exchange_u32(uint32_t volatile *x, uint32_t expected, uint32_t val)
{
#if HAVE_SYNC_BOOL_COMPARE_AND_SWAP
    return __sync_bool_compare_and_swap(x, expected, val);
#elif defined(_WIN32)
    return InterlockedCompareExchange((LONG *)x, val, expected) == expected;
#else
# error "vkd3d_atomic_compare_exchange_u32() not implemented for this platform"
#endif
}

static inline bool vkd3d_atomic_compare_exchange_ptr(void * volatile *x, void *expected, void *val)
{
#if HAVE_SYNC_BOOL_COMPARE_AND_SWAP
    return __sync_bool_compare_and_swap(x, expected, val);
#elif defined(_WIN32)
    return InterlockedCompareExchangePointer(x, val, expected) == expected;
#else
# error "vkd3d_atomic_compare_exchange_ptr() not implemented for this platform"
#endif
}

static inline uint32_t vkd3d_atomic_exchange_u32(uint32_t volatile *x, uint32_t val)
{
#if HAVE_ATOMIC_EXCHANGE_N
    return __atomic_exchange_n(x, val, __ATOMIC_SEQ_CST);
#elif defined(_WIN32)
    return InterlockedExchange((LONG *)x, val);
#else
    uint32_t expected;

    do
    {
        expected = *x;
    } while (!vkd3d_atomic_compare_exchange_u32(x, expected, val));

    return expected;
#endif
}

static inline void *vkd3d_atomic_exchange_ptr(void * volatile *x, void *val)
{
#if HAVE_ATOMIC_EXCHANGE_N
    return __atomic_exchange_n(x, val, __ATOMIC_SEQ_CST);
#elif defined(_WIN32)
    return InterlockedExchangePointer(x, val);
#else
    void *expected;

    do
    {
        expected = *x;
    } while (!vkd3d_atomic_compare_exchange_ptr(x, expected, val));

    return expected;
#endif
}

struct vkd3d_mutex
{
#ifdef _WIN32
    CRITICAL_SECTION lock;
#else
    pthread_mutex_t lock;
#endif
};

#ifdef _WIN32
#define VKD3D_MUTEX_INITIALIZER {{NULL, -1, 0, 0, 0, 0}}
#else
#define VKD3D_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#endif

static inline void vkd3d_mutex_init(struct vkd3d_mutex *lock)
{
#ifdef _WIN32
    InitializeCriticalSection(&lock->lock);
#else
    int ret;

    if ((ret = pthread_mutex_init(&lock->lock, NULL)))
        ERR("Failed to initialise the mutex, ret %d.\n", ret);
#endif
}

static inline void vkd3d_mutex_lock(struct vkd3d_mutex *lock)
{
#ifdef _WIN32
    EnterCriticalSection(&lock->lock);
#else
    int ret;

    if ((ret = pthread_mutex_lock(&lock->lock)))
        ERR("Failed to lock the mutex, ret %d.\n", ret);
#endif
}

static inline void vkd3d_mutex_unlock(struct vkd3d_mutex *lock)
{
#ifdef _WIN32
    LeaveCriticalSection(&lock->lock);
#else
    int ret;

    if ((ret = pthread_mutex_unlock(&lock->lock)))
        ERR("Failed to unlock the mutex, ret %d.\n", ret);
#endif
}

static inline void vkd3d_mutex_destroy(struct vkd3d_mutex *lock)
{
#ifdef _WIN32
    DeleteCriticalSection(&lock->lock);
#else
    int ret;

    if ((ret = pthread_mutex_destroy(&lock->lock)))
        ERR("Failed to destroy the mutex, ret %d.\n", ret);
#endif
}

struct vkd3d_cond
{
#ifdef _WIN32
    CONDITION_VARIABLE cond;
#else
    pthread_cond_t cond;
#endif
};

static inline void vkd3d_cond_init(struct vkd3d_cond *cond)
{
#ifdef _WIN32
    InitializeConditionVariable(&cond->cond);
#else
    int ret;

    if ((ret = pthread_cond_init(&cond->cond, NULL)))
        ERR("Failed to initialise the condition variable, ret %d.\n", ret);
#endif
}

static inline void vkd3d_cond_signal(struct vkd3d_cond *cond)
{
#ifdef _WIN32
    WakeConditionVariable(&cond->cond);
#else
    int ret;

    if ((ret = pthread_cond_signal(&cond->cond)))
        ERR("Failed to signal the condition variable, ret %d.\n", ret);
#endif
}

static inline void vkd3d_cond_broadcast(struct vkd3d_cond *cond)
{
#ifdef _WIN32
    WakeAllConditionVariable(&cond->cond);
#else
    int ret;

    if ((ret = pthread_cond_broadcast(&cond->cond)))
        ERR("Failed to broadcast the condition variable, ret %d.\n", ret);
#endif
}

static inline void vkd3d_cond_wait(struct vkd3d_cond *cond, struct vkd3d_mutex *lock)
{
#ifdef _WIN32
    if (!SleepConditionVariableCS(&cond->cond, &lock->lock, INFINITE))
        ERR("Failed to wait on the condition variable, error %lu.\n", GetLastError());
#else
    int ret;

    if ((ret = pthread_cond_wait(&cond->cond, &lock->lock)))
        ERR("Failed to wait on the condition variable, ret %d.\n", ret);
#endif
}

static inline void vkd3d_cond_destroy(struct vkd3d_cond *cond)
{
#ifdef _WIN32
    /* Nothing to do. */
#else
    int ret;

    if ((ret = pthread_cond_destroy(&cond->cond)))
        ERR("Failed to destroy the condition variable, ret %d.\n", ret);
#endif
}

static inline void vkd3d_parse_version(const char *version, int *major, int *minor)
{
    *major = atoi(version);

    while (isdigit(*version))
        ++version;
    if (*version == '.')
        ++version;

    *minor = atoi(version);
}

HRESULT hresult_from_vkd3d_result(int vkd3d_result);

#ifdef _WIN32
static inline void *vkd3d_dlopen(const char *name)
{
    return LoadLibraryA(name);
}

static inline void *vkd3d_dlsym(void *handle, const char *symbol)
{
    return GetProcAddress(handle, symbol);
}

static inline int vkd3d_dlclose(void *handle)
{
    return FreeLibrary(handle);
}

static inline const char *vkd3d_dlerror(void)
{
    unsigned int error = GetLastError();
    static char message[256];

    if (FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, message, sizeof(message), NULL))
        return message;
    sprintf(message, "Unknown error %u.\n", error);
    return message;
}
#elif defined(HAVE_DLFCN_H)
#include <dlfcn.h>

static inline void *vkd3d_dlopen(const char *name)
{
    return dlopen(name, RTLD_NOW);
}

static inline void *vkd3d_dlsym(void *handle, const char *symbol)
{
    return dlsym(handle, symbol);
}

static inline int vkd3d_dlclose(void *handle)
{
    return dlclose(handle);
}

static inline const char *vkd3d_dlerror(void)
{
    return dlerror();
}
#else
static inline void *vkd3d_dlopen(const char *name)
{
    return NULL;
}

static inline void *vkd3d_dlsym(void *handle, const char *symbol)
{
    return NULL;
}

static inline int vkd3d_dlclose(void *handle)
{
    return 0;
}

static inline const char *vkd3d_dlerror(void)
{
    return "Not implemented for this platform.\n";
}
#endif

#endif  /* __VKD3D_COMMON_H */
