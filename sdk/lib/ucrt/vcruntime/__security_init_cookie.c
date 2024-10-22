//
// __security_init_cookie.c
//
//      Copyright (c) 2024 Timo Kreuzer
//
// Implementation of __security_init_cookie.
//
// SPDX-License-Identifier: MIT
//

#include <stdlib.h>
#include <intrin.h>
#include <corecrt_internal.h>

#ifdef _WIN64
#define DEFAULT_SECURITY_COOKIE 0x00002B992DDFA232ull
#define _rotlptr _rotl64
#else
#define DEFAULT_SECURITY_COOKIE 0xBB40E64E
#define _rotlptr _rotl
#endif

uintptr_t __security_cookie = DEFAULT_SECURITY_COOKIE;
uintptr_t __security_cookie_complement = ~DEFAULT_SECURITY_COOKIE;

void __security_init_cookie(void)
{
    LARGE_INTEGER performanceCounter;
    FILETIME fileTime;
    uintptr_t randomValue = (uintptr_t)0x27E30B2C16B07297ull;

#if defined(_M_IX86) || defined(_M_X64)
    if (IsProcessorFeaturePresent(PF_RDRAND_INSTRUCTION_AVAILABLE))
    {
#ifdef _M_X64
        while (!_rdrand64_step(&randomValue))
            _mm_pause();
#else
        while (!_rdrand32_step(&randomValue))
            _mm_pause();
#endif
    }

    if (IsProcessorFeaturePresent(PF_RDTSC_INSTRUCTION_AVAILABLE))
    {
        randomValue += __rdtsc();
    }
#endif

    randomValue += (uintptr_t)&randomValue;
    randomValue ^= GetTickCount();

    QueryPerformanceCounter(&performanceCounter);
#ifdef _WIN64
    randomValue ^= performanceCounter.QuadPart;
#else
    randomValue ^= performanceCounter.LowPart;
    randomValue ^= performanceCounter.HighPart;
#endif

    randomValue += GetCurrentThreadId();
    randomValue = _rotlptr(randomValue, GetCurrentThreadId() >> 2);

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
    GetSystemTimePreciseAsFileTime(&fileTime);
#else
    GetSystemTimeAsFileTime(&fileTime);
#endif
    randomValue += fileTime.dwLowDateTime;
    randomValue += fileTime.dwHighDateTime;

    randomValue += GetCurrentProcessId();
    randomValue = _rotlptr(randomValue, GetCurrentProcessId() >> 2);

    if (randomValue == DEFAULT_SECURITY_COOKIE)
    {
        randomValue++;
    }

#ifdef _WIN64
    /* Zero out highest 16 bits */
    randomValue &= 0x0000FFFFFFFFFFFFull;
#endif

    __security_cookie = randomValue;
    __security_cookie_complement = ~randomValue;
}
