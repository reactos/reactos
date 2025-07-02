/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for extended state
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"
#include <windows.h>

static ULONG s_ProcessorNumber;

static
VOID
AdjustAffinity(VOID)
{
    KAFFINITY OldAffinity, NewAffinity;

    /* Set affinity to the current test processor */
    NewAffinity = (KAFFINITY)1 << s_ProcessorNumber;
    OldAffinity = SetThreadAffinityMask(GetCurrentThread(), NewAffinity);
    ok(OldAffinity != 0, "SetThreadAffinityMask(0x%Ix) failed\n", NewAffinity);
}

typedef struct _DECLSPEC_INTRIN_TYPE _CRT_ALIGN(16) _M128U64
{
    unsigned __int64 u64[2];
} M128U64;

typedef struct _DECLSPEC_INTRIN_TYPE _CRT_ALIGN(32) _M256U64
{
    unsigned __int64 u64[4];
} M256U64;

typedef struct _DECLSPEC_INTRIN_TYPE _CRT_ALIGN(64) _M512U64
{
    unsigned __int64 u64[8];
} M512U64;

#ifdef _M_IX86
#define REG_COUNT_SSE 8
#define REG_COUNT_AVX 8
#define REG_COUNT_AVX512 8
#else
#define REG_COUNT_SSE 16
#define REG_COUNT_AVX 16
#define REG_COUNT_AVX512 32
#endif

void __fastcall set_SSE_state(const M128U64 data[REG_COUNT_SSE]);
void __fastcall get_SSE_state(M128U64 data[REG_COUNT_SSE]);

void __fastcall set_AVX_state(const M256U64 data[REG_COUNT_AVX]);
void __fastcall get_AVX_state(M256U64 data[REG_COUNT_AVX]);

void __fastcall set_AVX512_state(const M512U64 data[REG_COUNT_AVX512]);
void __fastcall get_AVX512_state(M512U64 data[REG_COUNT_AVX512]);

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

BOOL ok_eq_m512i_(M512U64 a, M512U64 b, const char* variable, unsigned int line)
{
    BOOL equal = !memcmp(&a, &b, sizeof(M512U64));
    ok_(__FILE__, line)(equal, "Variable %s Expected %I64x %I64x %I64x %I64x %I64x %I64x %I64x %I64x, got %I64x %I64x %I64x %I64x %I64x %I64x %I64x %I64x\n",
        variable,
        b.u64[7], b.u64[6], b.u64[5], b.u64[4],
        b.u64[3], b.u64[2], b.u64[1], b.u64[0],
        a.u64[7], a.u64[6], a.u64[5], a.u64[4],
        a.u64[3], a.u64[2], a.u64[1], a.u64[0]);
    return TRUE;
}
#define ok_eq_m512i(a, b) ok_eq_m512i_(a, b, #a, __LINE__)

ULONG g_randomSeed = 0x12345678;

ULONG64 GenRandom64(VOID)
{
    ULONG Low32 = RtlRandom(&g_randomSeed);
    ULONG High32 = RtlRandom(&g_randomSeed);
    return ((ULONG64)High32 << 32) | Low32;
}

VOID RandomFill128(M128U64* Data128, ULONG Count)
{
    for (ULONG i = 0; i < Count; i++)
    {
        Data128[i].u64[0] = GenRandom64();
        Data128[i].u64[1] = GenRandom64();
    }
}

VOID RandomFill256(M256U64* Data256, ULONG Count)
{
    for (ULONG i = 0; i < Count; i++)
    {
        Data256[i].u64[0] = GenRandom64();
        Data256[i].u64[1] = GenRandom64();
        Data256[i].u64[2] = GenRandom64();
        Data256[i].u64[3] = GenRandom64();
    }
}

VOID RandomFill512(M512U64* Data512, ULONG Count)
{
    for (ULONG i = 0; i < Count; i++)
    {
        Data512[i].u64[0] = GenRandom64();
        Data512[i].u64[1] = GenRandom64();
        Data512[i].u64[2] = GenRandom64();
        Data512[i].u64[3] = GenRandom64();
        Data512[i].u64[4] = GenRandom64();
        Data512[i].u64[5] = GenRandom64();
        Data512[i].u64[6] = GenRandom64();
        Data512[i].u64[7] = GenRandom64();
    }
}

static DWORD WINAPI Thread_SSE(LPVOID Parameter)
{
    AdjustAffinity();

    // Get the current (fresh) state
    M128U64 SseState[REG_COUNT_SSE] = { 0 };
    get_SSE_state(SseState);

    // Make sure it's all zero
    static const M128U64 Zero128 = { 0 };
    for (ULONG i = 0; i < ARRAYSIZE(SseState); i++)
    {
        ok_eq_m128i(SseState[i], Zero128);
    }

    // Set a new "random" state
    RandomFill128(SseState, ARRAYSIZE(SseState));
    set_SSE_state(SseState);
    return 0;
}

void Test_SSE(void)
{
    if (!IsProcessorFeaturePresent(PF_XMMI_INSTRUCTIONS_AVAILABLE))
    {
        skip("SSE not supported\n");
        return;
    }

    // Fill the array with random numbers
    M128U64 InSseState[REG_COUNT_SSE];
    RandomFill128(InSseState, ARRAYSIZE(InSseState));

    _SEH2_TRY
    {
        // Set the state
        set_SSE_state(InSseState);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ok(FALSE, "Failed to set SSE state\n");
        return;
    }
    _SEH2_END;

    // Run a different thread that uses SSE
    HANDLE hThread = CreateThread(NULL, 0, Thread_SSE, NULL, 0, NULL);
    ok(hThread != NULL, "CreateThread failed\n");
    if (hThread == NULL)
    {
        skip("CreateThread failed\n");
        return;
    }

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);

    // Get the state
    M128U64 OutSseState[REG_COUNT_SSE] = { 0 };
    get_SSE_state(OutSseState);

    // Validte the state of the non-volatile registers
    for (ULONG i = 6; i < REG_COUNT_SSE; i++)
    {
        ok_eq_m128i(OutSseState[i], InSseState[i]);
    }
}

static DWORD WINAPI Thread_AVX(LPVOID Parameter)
{
    AdjustAffinity();

    // Get the current (fresh) state
    M256U64 AvxState[REG_COUNT_AVX];
    get_AVX_state(AvxState);

    // Make sure it's all zero
    static const M256U64 Zero256 = { 0 };
    for (ULONG i = 0; i < ARRAYSIZE(AvxState); i++)
    {
        ok_eq_m256i(AvxState[i], Zero256);
    }

    // Set a new "random" state
    RandomFill256(AvxState, ARRAYSIZE(AvxState));
    set_AVX_state(AvxState);
    return 0;
}

void Test_AVX(void)
{
    if (!IsProcessorFeaturePresent(PF_AVX_INSTRUCTIONS_AVAILABLE))
    {
        skip("AVX not supported\n");
        return;
    }

    // Fill the array with random numbers
    M256U64 InAvxState[REG_COUNT_AVX];
    RandomFill256(InAvxState, ARRAYSIZE(InAvxState));

    _SEH2_TRY
    {
        // Set the state
        set_AVX_state(InAvxState);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ok(FALSE, "Failed to set AVX state\n");
        return;
    }
    _SEH2_END;

    // Run a different thread that uses AVX
    HANDLE hThread = CreateThread(NULL, 0, Thread_AVX, NULL, 0, NULL);
    ok(hThread != NULL, "CreateThread failed\n");
    if (hThread == NULL)
    {
        skip("CreateThread failed\n");
        return;
    }

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);

    // Get the state
    M256U64 OutAvxState[REG_COUNT_AVX] = { 0 };
    get_AVX_state(OutAvxState);

    // Validte the state of the non-volatile registers
    for (ULONG i = 6; i < ARRAYSIZE(OutAvxState); i++)
    {
        ok_eq_m256i(OutAvxState[i], InAvxState[i]);
    }
}

static DWORD WINAPI Thread_AVX512(LPVOID Parameter)
{
    AdjustAffinity();

    // Get the current (fresh) state
    M512U64 Avx512State[REG_COUNT_AVX512];
    get_AVX512_state(Avx512State);

    // Make sure it's all zero
    static const M512U64 Zero512 = { 0 };
    for (ULONG i = 0; i < ARRAYSIZE(Avx512State); i++)
    {
        ok_eq_m512i(Avx512State[i], Zero512);
    }

    // Set a new "random" state
    RandomFill512(Avx512State, ARRAYSIZE(Avx512State));
    set_AVX512_state(Avx512State);
    return 0;
}

void Test_AVX512(void)
{
    if (!IsProcessorFeaturePresent(PF_AVX512F_INSTRUCTIONS_AVAILABLE))
    {
        skip("AVX512 not supported\n");
        return;
    }

    // Fill the array with random numbers
    M512U64 InAvx512State[REG_COUNT_AVX512];
    RandomFill512(InAvx512State, ARRAYSIZE(InAvx512State));

    _SEH2_TRY
    {
        // Set the state
        set_AVX512_state(InAvx512State);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ok(FALSE, "Failed to set AVX512 state\n");
        return;
    }
    _SEH2_END;

    // Run a different thread that uses AVX512
    HANDLE hThread = CreateThread(NULL, 0, Thread_AVX512, NULL, 0, NULL);
    ok(hThread != NULL, "CreateThread failed\n");
    if (hThread == NULL)
    {
        skip("CreateThread failed\n");
        return;
    }

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);

    // Get the state
    M512U64 OutAvx512State[REG_COUNT_AVX512] = { 0 };
    get_AVX512_state(OutAvx512State);

    // Validte the state of the non-volatile registers
    for (ULONG i = 6; i < ARRAYSIZE(OutAvx512State); i++)
    {
        ok_eq_m512i(OutAvx512State[i], InAvx512State[i]);
    }
}

START_TEST(XStateSave)
{
    SYSTEM_INFO sysinfo;

    GetSystemInfo(&sysinfo);

    for (s_ProcessorNumber = 0;
         s_ProcessorNumber < sysinfo.dwNumberOfProcessors;
         s_ProcessorNumber++)
    {
        AdjustAffinity();

        Test_SSE();
        Test_AVX();
        Test_AVX512();
    }
}
