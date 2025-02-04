/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for extended state
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"
#include <windows.h>

#include <intrin.h>
#include <immintrin.h>

typedef struct _DECLSPEC_INTRIN_TYPE _CRT_ALIGN(16) _M128U64
{
    unsigned __int64 u64[2];
} M128U64;

typedef struct _DECLSPEC_INTRIN_TYPE  _CRT_ALIGN(32) _M256U64
{
    unsigned __int64 u64[4];
} M256U64;

void set_SSE2_state(const M128U64 data[16]);
void get_SSE2_state(M128U64 data[16]);

void set_AVX_state(const M256U64 data[16]);
void get_AVX_state(M256U64 data[16]);

BOOL ok_eq_m128i_(M128U64 a, M128U64 b, const char* variable, unsigned int line)
{
    BOOL equal = !memcmp(&a, &b, sizeof(M128U64));
    ok_(__FILE__, line)(equal, "Variable %s Expected %I64x %I64x, got %I64x %I64x\n",
        variable, b.u64[1], b.u64[0], a.u64[1], a.u64[0]);
    return TRUE;
}
#define ok_eq_m128i(a, b) ok_eq_m128i_(a, b, #a, __LINE__)

BOOL ok_eq_m256i_(M256U64 a, M256U64 b, const char* variable, unsigned int line)
{
    BOOL equal = !memcmp(&a, &b, sizeof(M256U64));
    ok_(__FILE__, line)(equal, "Variable %s Expected %I64x %I64x %I64x %I64x, got %I64x %I64x %I64x %I64x\n",
        variable, b.u64[3], b.u64[2], b.u64[1], b.u64[0],
        a.u64[3], a.u64[2], a.u64[1], a.u64[0]);
    return TRUE;
}
#define ok_eq_m256i(a, b) ok_eq_m256i_(a, b, #a, __LINE__)

static DWORD WINAPI thread_SSE(LPVOID Parameter)
{
    // Clear SSE state
    M128U64 data[16] = { 0 };
    set_SSE2_state(data);
    return 0;
}

void test_SSE(void)
{
    if (!IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE))
    {
        skip("SSE not supported\n");
        return;
    }

    // Supported, now check if the OS supports it by setting up SSE state, then running
    // a different thread that also uses SSE.

    // An array of "random" numbers to set the SSE registers to
    const M128U64 in_data[16] =
    {
        { 0x1234567823456789ull, 0x3456789a456789abull },
        { 0x56789abc6789abcdull, 0x789abcde89abcdefull },
        { 0x9abcdef0abcdef01ull, 0xbcdef012cdef0123ull },
        { 0xdef01234ef012345ull, 0xf012345601234567ull },
        { 0x1234567823456789ull, 0x3456789a456789abull },
        { 0x56789abc6789abcdull, 0x789abcde89abcdefull },
        { 0x9abcdefabcdef001ull, 0xbcdef012cdef0123ull },
        { 0xdef01234ef012345ull, 0xf012345601234567ull },
        { 0x1234567823456789ull, 0x3456789a456789abull },
        { 0x56789abc6789abcdull, 0x789abcde89abcdefull },
        { 0x9abcdef0abcdef01ull, 0xbcdef012cdef0123ull },
        { 0xdef01234ef012345ull, 0xf012345601234567ull },
        { 0x1234567823456789ull, 0x3456789a456789abull },
        { 0x56789abc6789abcdull, 0x789abcde89abcdefull },
        { 0x9abcdef0abcdef01ull, 0xbcdef012cdef0123ull },
        { 0xdef01234ef012345ull, 0xf012345601234567ull },
    };

    _SEH2_TRY
    {
        // Set the state
        set_SSE2_state(in_data);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        // Not supported
        ok(FALSE, "SSE not supported\n");
        return;
    }
    _SEH2_END;

    // Run a different thread that uses SSE
    HANDLE hThread = CreateThread(NULL, 0, thread_SSE, NULL, 0, NULL);
    ok(hThread != NULL, "CreateThread failed\n");
    if (hThread)
    {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }

    // Get the state
    M128U64 out_data[16] = { 0 };
    get_SSE2_state(out_data);

    // Validte the state of the non-volatile registers
    for (int i = 6; i <= 15; i++)
    {
        ok_eq_m128i(in_data[i], out_data[i]);
    }
}

static DWORD WINAPI thread_AVX(LPVOID Parameter)
{
    // Clear AVX state
    M256U64 data[16] = { 0 };
    set_AVX_state(data);
    return 0;
}

void test_AVX(void)
{
    if (!IsProcessorFeaturePresent(PF_AVX_INSTRUCTIONS_AVAILABLE))
    {
        skip("AVX not supported\n");
        return;
    }

    // Supported, now check if the OS supports it by setting up AVX state, then running
    // a different thread that also uses AVX.
    // An array of "random" numbers to set the AVX registers to
    const M256U64 in_data[16] =
    {
        { 0x1234567823456789, 0x3456789a456789ab, 0x56789abc6789abcd, 0x789abcde89abcdef },
        { 0x9abcdef0abcdef01, 0xbcdef012cdef0123, 0xdef01234ef012345, 0xf012345601234567 },
        { 0x1234567823456789, 0x3456789a456789ab, 0x56789abc6789abcd, 0x789abcde89abcdef },
        { 0x9abcdef0abcdef01, 0xbcdef012cdef0123, 0xdef01234ef012345, 0xf012345601234567 },
        { 0x1234567823456789, 0x3456789a456789ab, 0x56789abc6789abcd, 0x789abcde89abcdef },
        { 0x9abcdef0abcdef01, 0xbcdef012cdef0123, 0xdef01234ef012345, 0xf012345601234567 },
        { 0x1234567823456789, 0x3456789a456789ab, 0x56789abc6789abcd, 0x789abcde89abcdef },
        { 0x9abcdef0abcdef01, 0xbcdef012cdef0123, 0xdef01234ef012345, 0xf012345601234567 },
        { 0x1234567823456789, 0x3456789a456789ab, 0x56789abc6789abcd, 0x789abcde89abcdef },
        { 0x9abcdef0abcdef01, 0xbcdef012cdef0123, 0xdef01234ef012345, 0xf012345601234567 },
        { 0x1234567823456789, 0x3456789a456789ab, 0x56789abc6789abcd, 0x789abcde89abcdef },
        { 0x9abcdef0abcdef01, 0xbcdef012cdef0123, 0xdef01234ef012345, 0xf012345601234567 },
        { 0x1234567823456789, 0x3456789a456789ab, 0x56789abc6789abcd, 0x789abcde89abcdef },
        { 0x9abcdef0abcdef01, 0xbcdef012cdef0123, 0xdef01234ef012345, 0xf012345601234567 },
        { 0x1234567823456789, 0x3456789a456789ab, 0x56789abc6789abcd, 0x789abcde89abcdef },
        { 0x9abcdef0abcdef01, 0xbcdef012cdef0123, 0xdef01234ef012345, 0xf012345601234567 },
    };

    _SEH2_TRY
    {
        // Set the state
        set_AVX_state(in_data);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        // Not supported
        ok(FALSE, "AVX not supported\n");
        return;
    }
    _SEH2_END;

    // Run a different thread that uses AVX
    HANDLE hThread = CreateThread(NULL, 0, thread_AVX, NULL, 0, NULL);
    ok(hThread != NULL, "CreateThread failed\n");
    if (hThread)
    {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }

    // Get the state
    M256U64 out_data[16] = { 0 };
    get_AVX_state(out_data);

    // Validte the state of the non-volatile registers
    for (int i = 6; i <= 15; i++)
    {
        ok_eq_m256i(in_data[i], out_data[i]);
    }
}

void test_AVX2(void)
{
    if (!IsProcessorFeaturePresent(PF_AVX2_INSTRUCTIONS_AVAILABLE))
    {
        skip("AVX2 not supported\n");
        return;
    }
}

START_TEST(XState)
{
#ifdef _M_IX86
    test_SSE();
#endif

    test_AVX();
    test_AVX2();
}
