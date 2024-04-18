/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for basic hash APIs
 * COPYRIGHT:   Copyright 2023 Ratin Gao <ratin@knsoft.org>
 */

#include "precomp.h"

typedef struct _A_SHA_CTX
{
    UCHAR Buffer[64];
    ULONG State[5];
    ULONG Count[2];
} A_SHA_CTX, *PA_SHA_CTX;

#define A_SHA_DIGEST_LEN 20

typedef struct _MD5_CTX
{
    ULONG Count[2];
    ULONG State[4];
    UCHAR Buffer[64];
    UCHAR Hash[16];
} MD5_CTX, *PMD5_CTX;

#define MD5_DIGEST_LEN 16

typedef struct _MD4_CTX
{
    ULONG State[4];
    ULONG Count[2];
    UCHAR Buffer[64];
    UCHAR Hash[16];
} MD4_CTX, *PMD4_CTX;

#define MD4_DIGEST_LEN 16

#ifndef RSA32API
#define RSA32API __stdcall
#endif

typedef
VOID
RSA32API
FN_A_SHAInit(
    _Out_ PA_SHA_CTX Context);

typedef
VOID
RSA32API
FN_A_SHAUpdate(
    _Inout_ PA_SHA_CTX Context,
    _In_reads_(BufferSize) PUCHAR Buffer,
    _In_ ULONG BufferSize);

typedef
VOID
RSA32API
FN_A_SHAFinal(
    _Inout_ PA_SHA_CTX Context,
    _Out_ PUCHAR Result);

typedef
VOID
RSA32API
FN_MD5Init(
    _Out_ PMD5_CTX Context);

typedef
VOID
RSA32API
FN_MD5Update(
    _Inout_ PMD5_CTX Context,
    _In_reads_(BufferSize) PUCHAR Buffer,
    _In_ ULONG BufferSize);

typedef
VOID
RSA32API
FN_MD5Final(
    _Inout_ PMD5_CTX Context);

typedef
VOID
RSA32API
FN_MD4Init(
    _Out_ PMD4_CTX Context);

typedef
VOID
RSA32API
FN_MD4Update(
    _Inout_ PMD4_CTX Context,
    _In_reads_(BufferSize) PUCHAR Buffer,
    _In_ ULONG BufferSize);

typedef
VOID
RSA32API
FN_MD4Final(
    _Inout_ PMD4_CTX Context);

static HMODULE g_hAdvapi32 = NULL;
static ANSI_STRING g_TestString = RTL_CONSTANT_STRING("ReactOS Hash API Test String");

static ULONG g_ctxSHA1StateInit[] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };
static UCHAR g_aucSHA1Result[A_SHA_DIGEST_LEN] = {
    0xEC, 0x05, 0x43, 0xE7, 0xDE, 0x8A, 0xEE, 0xFF,
    0xAD, 0x72, 0x2B, 0x9D, 0x55, 0x4F, 0xCA, 0x6A,
    0x8D, 0x81, 0xF1, 0xC7
};

static ULONG g_aulMD5Or4StateInit[] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476 };
static UCHAR g_aucMD5Result[MD5_DIGEST_LEN] = {
    0x3D, 0xE8, 0x23, 0x8B, 0x9D, 0xE0, 0xCE, 0x48,
    0xB1, 0x1B, 0xDD, 0xD9, 0xC6, 0x86, 0xB2, 0xDE
};
static UCHAR g_aucMD4Result[MD4_DIGEST_LEN] = {
    0xE0, 0xE8, 0x50, 0x8A, 0x4D, 0x11, 0x02, 0xA6,
    0x6A, 0xF0, 0xA7, 0xAB, 0xD8, 0xC4, 0x40, 0xED
};

static void Test_SHA1(void)
{
    FN_A_SHAInit* pfnA_SHAInit;
    FN_A_SHAUpdate* pfnA_SHAUpdate;
    FN_A_SHAFinal* pfnA_SHAFinal;
    DECLSPEC_ALIGN(4) A_SHA_CTX ctx;
    UCHAR Result[A_SHA_DIGEST_LEN];
    SIZE_T ComparedSize;

    /* Load functions */
    pfnA_SHAInit = (FN_A_SHAInit*)GetProcAddress(g_hAdvapi32, "A_SHAInit");
    pfnA_SHAUpdate = (FN_A_SHAUpdate*)GetProcAddress(g_hAdvapi32, "A_SHAUpdate");
    pfnA_SHAFinal = (FN_A_SHAFinal*)GetProcAddress(g_hAdvapi32, "A_SHAFinal");

    if (!pfnA_SHAInit || !pfnA_SHAUpdate || !pfnA_SHAFinal)
    {
        skip("advapi32.dll!A_SHA*** not found\n");
        return;
    }

    /* Test A_SHAInit */
    pfnA_SHAInit(&ctx);
    ComparedSize = RtlCompareMemory(ctx.State,
                                    g_ctxSHA1StateInit,
                                    RTL_FIELD_SIZE(A_SHA_CTX, State));
    ok_eq_size(ComparedSize, RTL_FIELD_SIZE(A_SHA_CTX, State));
    ok_eq_ulong(ctx.Count[0], 0UL);
    ok_eq_ulong(ctx.Count[1], 0UL);

    /* Test A_SHAUpdate */
    pfnA_SHAUpdate(&ctx, (PUCHAR)g_TestString.Buffer, g_TestString.Length);
    ComparedSize = RtlCompareMemory(ctx.Buffer,
                                    g_TestString.Buffer,
                                    g_TestString.Length);
    ok_eq_size(ComparedSize, g_TestString.Length);
    ComparedSize = RtlCompareMemory(ctx.State,
                                    g_ctxSHA1StateInit,
                                    RTL_FIELD_SIZE(A_SHA_CTX, State));
    ok_eq_size(ComparedSize, RTL_FIELD_SIZE(A_SHA_CTX, State));
    ok_eq_ulong(ctx.Count[0], 0UL);
    ok_eq_ulong(ctx.Count[1], (ULONG)g_TestString.Length);

    /* Test A_SHAFinal */
    pfnA_SHAFinal(&ctx, Result);
    ComparedSize = RtlCompareMemoryUlong(ctx.Buffer, sizeof(ctx.Buffer), 0);
    ok_eq_size(ComparedSize, sizeof(ctx.Buffer));
    ComparedSize = RtlCompareMemory(ctx.State,
                                    g_ctxSHA1StateInit,
                                    RTL_FIELD_SIZE(A_SHA_CTX, State));
    ok_eq_size(ComparedSize, RTL_FIELD_SIZE(A_SHA_CTX, State));
    ok_eq_ulong(ctx.Count[0], 0UL);
    ok_eq_ulong(ctx.Count[1], 0UL);
    ComparedSize = RtlCompareMemory(Result, g_aucSHA1Result, A_SHA_DIGEST_LEN);
    ok_eq_size(ComparedSize, A_SHA_DIGEST_LEN);
}

static void Test_MD5(void)
{
    FN_MD5Init* pfnMD5Init;
    FN_MD5Update* pfnMD5Update;
    FN_MD5Final* pfnMD5Final;
    DECLSPEC_ALIGN(4) MD5_CTX ctx;
    SIZE_T ComparedSize;

    /* Load functions */
    pfnMD5Init = (FN_MD5Init*)GetProcAddress(g_hAdvapi32, "MD5Init");
    pfnMD5Update = (FN_MD5Update*)GetProcAddress(g_hAdvapi32, "MD5Update");
    pfnMD5Final = (FN_MD5Final*)GetProcAddress(g_hAdvapi32, "MD5Final");

    if (!pfnMD5Init || !pfnMD5Update || !pfnMD5Final)
    {
        skip("advapi32.dll!MD5*** not found\n");
        return;
    }
    
    /* Test MD5Init */
    pfnMD5Init(&ctx);
    ok_eq_ulong(ctx.Count[0], 0UL);
    ok_eq_ulong(ctx.Count[1], 0UL);
    ComparedSize = RtlCompareMemory(ctx.State,
                                    g_aulMD5Or4StateInit,
                                    RTL_FIELD_SIZE(MD5_CTX, State));
    ok_eq_size(ComparedSize, RTL_FIELD_SIZE(MD5_CTX, State));

    /* Test MD5Update */
    pfnMD5Update(&ctx, (PUCHAR)g_TestString.Buffer, g_TestString.Length);
    ComparedSize = RtlCompareMemory(ctx.Buffer,
                                    g_TestString.Buffer,
                                    g_TestString.Length);
    ok_eq_size(ComparedSize, g_TestString.Length);
    ComparedSize = RtlCompareMemory(ctx.State,
                                    g_aulMD5Or4StateInit,
                                    RTL_FIELD_SIZE(MD5_CTX, State));
    ok_eq_size(ComparedSize, RTL_FIELD_SIZE(MD5_CTX, State));
    ok_eq_ulong(ctx.Count[0], (ULONG)g_TestString.Length * CHAR_BIT);
    ok_eq_ulong(ctx.Count[1], 0UL);

    /* Test MD5Final */
    pfnMD5Final(&ctx);
    ok_eq_ulong(ctx.Count[0], 0x200UL);
    ok_eq_ulong(ctx.Count[1], 0UL);
    ok_eq_ulong(ctx.State[0], 0x8B23E83DUL);
    ok_eq_ulong(ctx.State[1], 0x48CEE09DUL);
    ok_eq_ulong(ctx.State[2], 0xD9DD1BB1UL);
    ok_eq_ulong(ctx.State[3], 0xDEB286C6UL);
    ComparedSize = RtlCompareMemoryUlong(ctx.Buffer, sizeof(ctx.Buffer), 0);
    ok_eq_size(ComparedSize, sizeof(ctx.Buffer));
    ComparedSize = RtlCompareMemory(ctx.Hash, &g_aucMD5Result, MD5_DIGEST_LEN);
    ok_eq_size(ComparedSize, MD5_DIGEST_LEN);
}

static void Test_MD4(void)
{
    FN_MD4Init* pfnMD4Init;
    FN_MD4Update* pfnMD4Update;
    FN_MD4Final* pfnMD4Final;
    DECLSPEC_ALIGN(4) MD4_CTX ctx;
    SIZE_T ComparedSize;

    /* Load functions */
    pfnMD4Init = (FN_MD4Init*)GetProcAddress(g_hAdvapi32, "MD4Init");
    pfnMD4Update = (FN_MD4Update*)GetProcAddress(g_hAdvapi32, "MD4Update");
    pfnMD4Final = (FN_MD4Final*)GetProcAddress(g_hAdvapi32, "MD4Final");

    if (!pfnMD4Init || !pfnMD4Update || !pfnMD4Final)
    {
        skip("advapi32.dll!MD4*** not found\n");
        return;
    }
    
    /* Test MD4Init */
    pfnMD4Init(&ctx);
    ok_eq_ulong(ctx.Count[0], 0UL);
    ok_eq_ulong(ctx.Count[1], 0UL);
    ComparedSize = RtlCompareMemory(ctx.State,
                                    g_aulMD5Or4StateInit,
                                    RTL_FIELD_SIZE(MD4_CTX, State));
    ok_eq_size(ComparedSize, RTL_FIELD_SIZE(MD4_CTX, State));

    /* Test MD4Update */
    pfnMD4Update(&ctx, (PUCHAR)g_TestString.Buffer, g_TestString.Length);
    ComparedSize = RtlCompareMemory(ctx.Buffer,
                                    g_TestString.Buffer,
                                    g_TestString.Length);
    ok_eq_size(ComparedSize, g_TestString.Length);
    ComparedSize = RtlCompareMemory(ctx.State,
                                    g_aulMD5Or4StateInit,
                                    RTL_FIELD_SIZE(MD4_CTX, State));
    ok_eq_size(ComparedSize, RTL_FIELD_SIZE(MD4_CTX, State));
    ok_eq_ulong(ctx.Count[0], (ULONG)g_TestString.Length * CHAR_BIT);
    ok_eq_ulong(ctx.Count[1], 0UL);

    /* Test MD4Final */
    pfnMD4Final(&ctx);
    ok_eq_ulong(ctx.Count[0], 0x200UL);
    ok_eq_ulong(ctx.Count[1], 0UL);
    ok_eq_ulong(ctx.State[0], 0x8A50E8E0UL);
    ok_eq_ulong(ctx.State[1], 0xA602114DUL);
    ok_eq_ulong(ctx.State[2], 0xABA7F06AUL);
    ok_eq_ulong(ctx.State[3], 0xED40C4D8UL);
    ComparedSize = RtlCompareMemoryUlong(ctx.Buffer, sizeof(ctx.Buffer), 0);
    ok_eq_size(ComparedSize, sizeof(ctx.Buffer));
    ComparedSize = RtlCompareMemory(ctx.Hash, &g_aucMD4Result, MD4_DIGEST_LEN);
    ok_eq_size(ComparedSize, MD4_DIGEST_LEN);
}

START_TEST(Hash)
{
    /* Load advapi32.dll */
    g_hAdvapi32 = GetModuleHandleW(L"advapi32.dll");
    if (!g_hAdvapi32)
    {
        skip("Module advapi32 not found\n");
        return;
    }

    Test_SHA1();
    Test_MD5();
    Test_MD4();
}
