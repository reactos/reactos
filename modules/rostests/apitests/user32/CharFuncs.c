/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for Char* functions
 * COPYRIGHT:   Copyright 2022 Stanislav Motylkov <x86corez@gmail.com>
 */

#include "precomp.h"

#include <winnls.h>
#include <ndk/rtlfuncs.h>
#include <pseh/pseh2.h>
#include <strsafe.h>
#include <versionhelpers.h>

#define INVALID_PTR_OFF(x)  ((PVOID)(ULONG_PTR)(0xdeadbeefdeadbeefULL + x))
#define INVALID_PTR         INVALID_PTR_OFF(0)

/* Default code page to be tested */
#define TEST_ACP            1252

typedef enum
{
    testLen,
    testOffs,
    testBoth,
} TEST_TYPE;

/* Dynamic allocation tests */
typedef struct
{
    TEST_TYPE testType;
    LPWSTR lpszStart;   /* Specified string for szStart */
    LPWSTR lpszCurrent; /* Specified string for szCurrent (only when testType == testBoth) */
    INT iOffset;        /* Specified offset to test (only when testType == testOffs) */
    INT iResOffset;     /* Expected offset when szCurrent >= szStart */
    INT iResOffsetNeg;  /* Expected offset when szCurrent < szStart */
    BOOL bWithinStart;  /* TRUE for pointer expected to be within szStart, FALSE for within szCurrent */
    BOOL bWideOnly;     /* Perform test only for Unicode case */
} TESTS_CHARPREV;

TESTS_CHARPREV TestCharPrev[] =
{
    {testLen, L"C:\\ReactOS", NULL, 0, 9, 9, TRUE, FALSE},
    {testOffs, L"abcdefghijk", NULL, 11, 10, 10, TRUE, FALSE},
    {testOffs, L"test a´^~¯", NULL, 10, 9, 9, TRUE, FALSE},
    {testOffs, L"test å", NULL, 6, 5, 5, TRUE, FALSE},
    {testBoth, L"C:\\ReactOS", L"", 0, -1, 0, FALSE, FALSE},
    {testBoth, L"C:\\ReactOS\\", L"C:\\ReactOS", 0, -1, 0, FALSE, FALSE},
    {testBoth, L"C:\\ReactOS\\", L"ReactOS", 0, -1, 0, FALSE, FALSE},
};

TESTS_CHARPREV TestCharPrev_XP[] =
{
    /* XP/2003 treat diacritics as normal characters */
    {testOffs, L"test a\x030a", NULL, 7, 6, 6, TRUE, TRUE},
    {testOffs, L"test a\x0301\x0302\x0303\x0304", NULL, 10, 9, 9, TRUE, TRUE},
    {testOffs, L"test a\x0301\x0302\x0303\x0304", NULL, 9, 8, 8, TRUE, TRUE},
    {testOffs, L"test a\x0301\x0302\x0303\x0304", NULL, 8, 7, 7, TRUE, TRUE},
    {testOffs, L"test a\x0301\x0302\x0303\x0304", NULL, 7, 6, 6, TRUE, TRUE},
};

TESTS_CHARPREV TestCharPrev_Vista[] =
{
    /* Vista+ does respect diacritics and skip them */
    {testOffs, L"test a\x030a", NULL, 7, 5, 5, TRUE, TRUE},
    {testOffs, L"test a\x0301\x0302\x0303\x0304", NULL, 10, 5, 5, TRUE, TRUE},
    {testOffs, L"test a\x0301\x0302\x0303\x0304", NULL, 9, 5, 5, TRUE, TRUE},
    {testOffs, L"test a\x0301\x0302\x0303\x0304", NULL, 8, 5, 5, TRUE, TRUE},
    {testOffs, L"test a\x0301\x0302\x0303\x0304", NULL, 7, 5, 5, TRUE, TRUE},
};

/* Static tests */
static const WCHAR wszReactOS[] = L"C:\\ReactOS";
static const CHAR szReactOS[] = "C:\\ReactOS";
static const WCHAR wszSpecial[] = L"test\0\0\0\0\0\0aa\t\t\t\r\n\r\n";
static const CHAR szSpecial[] = "test\0\0\0\0\0\0aa\t\t\t\r\n\r\n";
static const WCHAR wszMagic1[] = L"test a\x030a";
static const WCHAR wszMagic2[] = L"test a\x0301\x0302\x0303\x0304";

static const CHAR szUTF8Cyril[] =  "test \xD1\x82\xD0\xB5\xD1\x81\xD1\x82";     /* UTF8(L"test тест") */
static const CHAR szUTF8Greek[] =  "test \xCF\x84\xCE\xB5\xCF\x83\xCF\x84";     /* UTF8(L"test τεστ") */
static const CHAR szUTF8Japan[] =  "test \xE3\x83\x86\xE3\x82\xB9\xE3\x83\x88"; /* UTF8(L"test テスト") */
static const CHAR szCP932Japan[] = "test \x83\x65\x83\x58\x83\x67";             /* CP932(L"test テスト") */

typedef struct
{
    LPCWSTR wszStart;
    LPCWSTR wszCurrent;
    LPCWSTR wszResult;
    LPCSTR szStart;
    LPCSTR szCurrent;
    LPCSTR szResult;
} ST_TESTS_CHARPREV;

ST_TESTS_CHARPREV TestStaticCharPrev[] =
{
    {wszReactOS, wszReactOS, wszReactOS,
      szReactOS,  szReactOS,  szReactOS},
    {wszReactOS, wszReactOS + 1, wszReactOS,
      szReactOS,  szReactOS + 1,  szReactOS},
    {wszReactOS, wszReactOS + 2, wszReactOS + 1,
      szReactOS,  szReactOS + 2,  szReactOS + 1},
    {wszReactOS, wszReactOS + 3, wszReactOS + 2,
      szReactOS,  szReactOS + 3,  szReactOS + 2},
    {wszReactOS, wszReactOS + 10, wszReactOS + 9,
      szReactOS,  szReactOS + 10,  szReactOS + 9},

    {wszReactOS + 2, wszReactOS, wszReactOS,
      szReactOS + 2,  szReactOS,  szReactOS},
    {wszReactOS + 2, wszReactOS + 1, wszReactOS + 1,
      szReactOS + 2,  szReactOS + 1,  szReactOS + 1},
    {wszReactOS + 2, wszReactOS + 2, wszReactOS + 2,
      szReactOS + 2,  szReactOS + 2,  szReactOS + 2},
    {wszReactOS + 2, wszReactOS + 3, wszReactOS + 2,
      szReactOS + 2,  szReactOS + 3,  szReactOS + 2},
    {wszReactOS + 2, wszReactOS + 4, wszReactOS + 3,
      szReactOS + 2,  szReactOS + 4,  szReactOS + 3},

    /* Test null-terminators */
    {wszSpecial, wszSpecial + 8, wszSpecial + 7,
      szSpecial,  szSpecial + 8,  szSpecial + 7},

    /* Test tabulation */
    {wszSpecial, wszSpecial + 13, wszSpecial + 12,
      szSpecial,  szSpecial + 13,  szSpecial + 12},

    /* Test linebreak */
    {wszSpecial, wszSpecial + 17, wszSpecial + 16,
      szSpecial,  szSpecial + 17,  szSpecial + 16},
    {wszSpecial, wszSpecial + 18, wszSpecial + 17,
      szSpecial,  szSpecial + 18,  szSpecial + 17},
};

ST_TESTS_CHARPREV TestStaticCharPrev_XP[] =
{
    /* XP/2003 treat diacritics as normal characters */
    {wszMagic1, wszMagic1 + 7,  wszMagic1 + 6,
     NULL, NULL, NULL},
    {wszMagic2, wszMagic2 + 10, wszMagic2 + 9,
     NULL, NULL, NULL},
    {wszMagic2, wszMagic2 + 9,  wszMagic2 + 8,
     NULL, NULL, NULL},
    {wszMagic2, wszMagic2 + 8,  wszMagic2 + 7,
     NULL, NULL, NULL},
    {wszMagic2, wszMagic2 + 7,  wszMagic2 + 6,
     NULL, NULL, NULL},
};

ST_TESTS_CHARPREV TestStaticCharPrev_Vista[] =
{
    /* Vista+ does respect diacritics and skip them */
    {wszMagic1, wszMagic1 + 7,  wszMagic1 + 5,
     NULL, NULL, NULL},
    {wszMagic2, wszMagic2 + 10, wszMagic2 + 5,
     NULL, NULL, NULL},
    {wszMagic2, wszMagic2 + 9,  wszMagic2 + 5,
     NULL, NULL, NULL},
    {wszMagic2, wszMagic2 + 8,  wszMagic2 + 5,
     NULL, NULL, NULL},
    {wszMagic2, wszMagic2 + 7,  wszMagic2 + 5,
     NULL, NULL, NULL},
};

typedef struct
{
    UINT uCodePage;
    LPCSTR szStart;
    LPCSTR szCurrent;
    LPCSTR szResult;
} ST_CODEPAGE_TESTS_CHARPREV;

ST_CODEPAGE_TESTS_CHARPREV TestStaticCodePageCharPrev[] =
{
    /* UTF-8 characters are not properly counted */
    {CP_UTF8, szUTF8Cyril, szUTF8Cyril + 2, szUTF8Cyril + 1},
    {CP_UTF8, szUTF8Cyril, szUTF8Cyril + 7, szUTF8Cyril + 6},
    {CP_UTF8, szUTF8Cyril, szUTF8Cyril + 9, szUTF8Cyril + 8},

    {CP_UTF8, szUTF8Greek, szUTF8Greek + 7, szUTF8Greek + 6},
    {CP_UTF8, szUTF8Greek, szUTF8Greek + 9, szUTF8Greek + 8},

    {CP_UTF8, szUTF8Japan, szUTF8Japan + 8, szUTF8Japan + 7},
    {CP_UTF8, szUTF8Japan, szUTF8Japan + 11, szUTF8Japan + 10},

    /* Code Page 932 / Shift-JIS characters are properly counted */
    {932,     szCP932Japan, szCP932Japan + 2, szCP932Japan + 1},
    {932,     szCP932Japan, szCP932Japan + 7, szCP932Japan + 5},
    {932,     szCP932Japan, szCP932Japan + 9, szCP932Japan + 7},
};

typedef struct
{
    LPCWSTR wszString;
    LPCWSTR wszResult;
    LPCSTR szString;
    LPCSTR szResult;
} ST_TESTS_CHARNEXT;

ST_TESTS_CHARNEXT TestStaticCharNext[] =
{
    {wszReactOS, wszReactOS + 1,
      szReactOS,  szReactOS + 1},
    {wszReactOS + 1, wszReactOS + 2,
      szReactOS + 1,  szReactOS + 2},
    {wszReactOS + 2, wszReactOS + 3,
      szReactOS + 2,  szReactOS + 3},
    {wszReactOS + 9, wszReactOS + 10,
      szReactOS + 9,  szReactOS + 10},
    {wszReactOS + 10, wszReactOS + 10,
      szReactOS + 10,  szReactOS + 10},

    /* Test null-terminators */
    {wszSpecial + 3, wszSpecial + 4,
      szSpecial + 3,  szSpecial + 4},
    {wszSpecial + 4, wszSpecial + 4,
      szSpecial + 4,  szSpecial + 4},
    {wszSpecial + 5, wszSpecial + 5,
      szSpecial + 5,  szSpecial + 5},

    /* Test tabulation */
    {wszSpecial + 12, wszSpecial + 13,
      szSpecial + 12,  szSpecial + 13},

    /* Test linebreak */
    {wszSpecial + 15, wszSpecial + 16,
      szSpecial + 15,  szSpecial + 16},
    {wszSpecial + 16, wszSpecial + 17,
      szSpecial + 16,  szSpecial + 17},
};

ST_TESTS_CHARNEXT TestStaticCharNext_XP[] =
{
    /* XP/2003 treat diacritics as normal characters */
    {wszMagic1 + 5, wszMagic1 + 6,
     NULL, NULL},
    {wszMagic2 + 5, wszMagic2 + 6,
     NULL, NULL},
    {wszMagic2 + 6, wszMagic2 + 7,
     NULL, NULL},
    {wszMagic2 + 7, wszMagic2 + 8,
     NULL, NULL},
    {wszMagic2 + 8, wszMagic2 + 9,
     NULL, NULL},
};

ST_TESTS_CHARNEXT TestStaticCharNext_Vista[] =
{
    /* Vista+ does respect diacritics and skip them */
    {wszMagic1 + 5, wszMagic1 + 7,
     NULL, NULL},
    {wszMagic2 + 5, wszMagic2 + 10,
     NULL, NULL},
    {wszMagic2 + 6, wszMagic2 + 10,
     NULL, NULL},
    {wszMagic2 + 7, wszMagic2 + 10,
     NULL, NULL},
    {wszMagic2 + 8, wszMagic2 + 10,
     NULL, NULL},
};

typedef struct
{
    UINT uCodePage;
    LPCSTR szString;
    LPCSTR szResult;
} ST_CODEPAGE_TESTS_CHARNEXT;

ST_CODEPAGE_TESTS_CHARNEXT TestStaticCodePageCharNext[] =
{
    /* UTF-8 characters are not properly counted */
    {CP_UTF8, szUTF8Cyril, szUTF8Cyril + 1},
    {CP_UTF8, szUTF8Cyril + 4, szUTF8Cyril + 5},
    {CP_UTF8, szUTF8Cyril + 5, szUTF8Cyril + 6},
    {CP_UTF8, szUTF8Cyril + 7, szUTF8Cyril + 8},

    {CP_UTF8, szUTF8Greek + 5, szUTF8Greek + 6},
    {CP_UTF8, szUTF8Greek + 7, szUTF8Greek + 8},

    {CP_UTF8, szUTF8Japan + 5, szUTF8Japan + 6},
    {CP_UTF8, szUTF8Japan + 8, szUTF8Japan + 9},

    /* Code Page 932 / Shift-JIS characters are properly counted */
    {932,     szCP932Japan, szCP932Japan + 1},
    {932,     szCP932Japan + 5, szCP932Japan + 7},
    {932,     szCP932Japan + 7, szCP932Japan + 9},
};

/* Exception tests (corner cases) */
typedef struct
{
    LPCWSTR wszStart;
    LPCWSTR wszCurrent;
    LPCWSTR wszResult;
    LPCSTR szStart;
    LPCSTR szCurrent;
    LPCSTR szResult;
    LPCSTR szExResult;
    NTSTATUS resStatus;
} EX_TESTS_CHARPREV;

EX_TESTS_CHARPREV TestExceptionCharPrev[] =
{
    {wszReactOS, NULL, NULL,
      szReactOS, NULL, NULL, NULL,
     STATUS_SUCCESS},
    {NULL, NULL, NULL,
     NULL, NULL, NULL, NULL,
     STATUS_SUCCESS},
    {NULL, wszReactOS, wszReactOS - 1,
     NULL,  szReactOS,  szReactOS - 1, szReactOS - 1,
     STATUS_SUCCESS},

    {INVALID_PTR, NULL, NULL,
     INVALID_PTR, NULL, NULL, NULL,
     STATUS_SUCCESS},
    {NULL, NULL, NULL,
     NULL, NULL, NULL, NULL,
     STATUS_SUCCESS},
    {NULL, INVALID_PTR, NULL,
     NULL, INVALID_PTR, INVALID_PTR_OFF(-1) /* NULL on Win7 with updates */, NULL,
     STATUS_ACCESS_VIOLATION},

    {wszReactOS, INVALID_PTR, NULL,
      szReactOS, INVALID_PTR, INVALID_PTR_OFF(-1) /* NULL on Win7 with updates */, NULL,
     STATUS_ACCESS_VIOLATION},
    {INVALID_PTR, INVALID_PTR, INVALID_PTR,
     INVALID_PTR, INVALID_PTR, INVALID_PTR, INVALID_PTR,
     STATUS_SUCCESS},
    {INVALID_PTR, wszReactOS, wszReactOS,
     INVALID_PTR,  szReactOS,  szReactOS, szReactOS,
     STATUS_SUCCESS},

    {INVALID_PTR_OFF(-2), INVALID_PTR, NULL,
     INVALID_PTR_OFF(-2), INVALID_PTR, INVALID_PTR_OFF(-1) /* NULL on Win7 with updates */, NULL,
     STATUS_ACCESS_VIOLATION},
    {INVALID_PTR, INVALID_PTR_OFF(2), NULL,
     INVALID_PTR, INVALID_PTR_OFF(2), INVALID_PTR_OFF(1) /* NULL on Win7 with updates */, NULL,
     STATUS_ACCESS_VIOLATION},
    {INVALID_PTR, INVALID_PTR_OFF(-2), INVALID_PTR_OFF(-2),
     INVALID_PTR, INVALID_PTR_OFF(-2), INVALID_PTR_OFF(-2), INVALID_PTR_OFF(-2),
     STATUS_SUCCESS},
    {INVALID_PTR_OFF(2), INVALID_PTR, INVALID_PTR,
     INVALID_PTR_OFF(2), INVALID_PTR, INVALID_PTR, INVALID_PTR,
     STATUS_SUCCESS},
};

typedef struct
{
    LPCWSTR wszString;
    LPCWSTR wszResult;
    LPCSTR szString;
    LPCSTR szResult;
    NTSTATUS resStatus;
} EX_TESTS_CHARNEXT;

EX_TESTS_CHARNEXT TestExceptionCharNext[] =
{
    {wszReactOS, wszReactOS + 1,
      szReactOS,  szReactOS + 1,
     STATUS_SUCCESS},
    {NULL, NULL,
     NULL, NULL,
     STATUS_ACCESS_VIOLATION},
    {INVALID_PTR, NULL,
     INVALID_PTR, NULL,
     STATUS_ACCESS_VIOLATION},

    {INVALID_PTR_OFF(-2), NULL,
     INVALID_PTR_OFF(-2), NULL,
     STATUS_ACCESS_VIOLATION},
    {INVALID_PTR_OFF(2), NULL,
     INVALID_PTR_OFF(2), NULL,
     STATUS_ACCESS_VIOLATION},
};

static LPWSTR AllocStringW(LPWSTR lpszStr, SIZE_T len)
{
    LPWSTR str;
    SIZE_T sz;

    if (!lpszStr)
        return NULL;

    sz = (len + 1) * sizeof(lpszStr[0]);
    str = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sz);
    if (!str)
    {
        trace("HeapAlloc failed (error %ld)\n", GetLastError());
        goto Skip;
    }
    StringCbCopyW(str, sz, lpszStr);
Skip:
    return str;
}

static LPSTR AllocStringA(LPWSTR lpszStr, SIZE_T len)
{
    LPSTR str;
    SIZE_T sz, mbs;

    if (!lpszStr)
        return NULL;

    sz = len + 1;
    str = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sz);
    if (!str)
    {
        trace("HeapAlloc failed (error %ld)\n", GetLastError());
        goto Skip;
    }

    mbs = WideCharToMultiByte(TEST_ACP, 0, lpszStr, -1, NULL, 0, NULL, NULL);
    if (!mbs || mbs > sz)
    {
        HeapFree(GetProcessHeap(), 0, str);
        str = NULL;
        trace("WideCharToMultiByte returned %lu (error %ld)\n", mbs, GetLastError());
        goto Skip;
    }

    WideCharToMultiByte(TEST_ACP, 0, lpszStr, -1, str, mbs, NULL, NULL);
Skip:
    return str;
}

static void testCharPrevW(const TESTS_CHARPREV *pEntry, SIZE_T len, UINT i)
{
    LPWSTR wszStart, wszCurrent;
    LPWSTR pchW;
    INT iRealOffset;
    BOOL b;

    wszStart = AllocStringW(pEntry->lpszStart, len);
    if (!wszStart && pEntry->lpszStart)
    {
        skip("[%u] AllocStringW for wszStart failed\n", i);
        goto Cleanup;
    }
    if (pEntry->testType == testLen)
        wszCurrent = wszStart + len;
    else if (pEntry->testType == testOffs)
        wszCurrent = wszStart + pEntry->iOffset;
    else
    {
        wszCurrent = AllocStringW(pEntry->lpszCurrent, wcslen(pEntry->lpszCurrent));
        if (!wszCurrent && pEntry->lpszCurrent)
        {
            skip("[%u] AllocStringW for wszCurrent failed\n", i);
            goto Cleanup;
        }
    }
    pchW = CharPrevW(wszStart, wszCurrent);
    if (wszCurrent - wszStart >= 0)
        iRealOffset = pEntry->iResOffset;
    else
        iRealOffset = pEntry->iResOffsetNeg;
    if (pEntry->bWithinStart)
    {
        b = pchW >= wszStart && pchW <= wszStart + len;
        if (iRealOffset >= 0)
            ok(b, "[%u] CharPrevW: pchW (0x%p) is expected to be within wszStart (0x%p)\n", i, pchW, wszStart);
        else
            ok(!b, "[%u] CharPrevW: pchW (0x%p) is expected to be outside wszStart (0x%p)\n", i, pchW, wszStart);
        ok(pchW == wszStart + iRealOffset, "[%u] CharPrevW: pchW is 0x%p (offset %d)\n", i, pchW, pchW - wszStart);
    }
    else
    {
        b = pchW >= wszCurrent && pchW <= wszCurrent + wcslen(pEntry->lpszCurrent);
        if (iRealOffset >= 0)
            ok(b, "[%u] CharPrevW: pchW (0x%p) is expected to be within wszCurrent (0x%p)\n", i, pchW, wszCurrent);
        else
            ok(!b, "[%u] CharPrevW: pchW (0x%p) is expected to be outside wszCurrent (0x%p)\n", i, pchW, wszCurrent);
        ok(pchW == wszCurrent + iRealOffset, "[%u] CharPrevW: pchW is 0x%p (offset %d)\n", i, pchW, pchW - wszCurrent);
    }

Cleanup:
    if (pEntry->testType != testBoth)
        wszCurrent = NULL;
    HeapFree(GetProcessHeap(), 0, wszStart);
    HeapFree(GetProcessHeap(), 0, wszCurrent);
}

static void testCharPrevA(const TESTS_CHARPREV *pEntry, SIZE_T len, UINT i)
{
    LPSTR szStart, szCurrent;
    LPSTR pchA, pchEx;
    INT iRealOffset;
    BOOL b;

    szStart = AllocStringA(pEntry->lpszStart, len);
    if (!szStart && pEntry->lpszStart)
    {
        skip("[%u] AllocStringA for szStart failed\n", i);
        goto Cleanup;
    }
    if (pEntry->testType == testLen)
        szCurrent = szStart + len;
    else if (pEntry->testType == testOffs)
        szCurrent = szStart + pEntry->iOffset;
    else
    {
        szCurrent = AllocStringA(pEntry->lpszCurrent, wcslen(pEntry->lpszCurrent));
        if (!szCurrent && pEntry->lpszCurrent)
        {
            skip("[%u] AllocStringA for szCurrent failed\n", i);
            goto Cleanup;
        }
    }
    pchA = CharPrevA(szStart, szCurrent);
    pchEx = CharPrevExA(TEST_ACP, szStart, szCurrent, 0);
    if (szCurrent - szStart >= 0)
        iRealOffset = pEntry->iResOffset;
    else
        iRealOffset = pEntry->iResOffsetNeg;
    if (pEntry->bWithinStart)
    {
        b = pchA >= szStart && pchA <= szStart + len;
        if (iRealOffset >= 0)
            ok(b, "[%u] CharPrevA: pchA (0x%p) is expected to be within szStart (0x%p)\n", i, pchA, szStart);
        else
            ok(!b, "[%u] CharPrevA: pchA (0x%p) is expected to be outside szStart (0x%p)\n", i, pchA, szStart);
        ok(pchA == szStart + iRealOffset, "[%u] CharPrevA: pchA is 0x%p (offset %d)\n", i, pchA, pchA - szStart);
    }
    else
    {
        b = pchA >= szCurrent && pchA <= szCurrent + wcslen(pEntry->lpszCurrent);
        if (iRealOffset >= 0)
            ok(b, "[%u] CharPrevA: pchA (0x%p) is expected to be within szCurrent (0x%p)\n", i, pchA, szCurrent);
        else
            ok(!b, "[%u] CharPrevA: pchA (0x%p) is expected to be outside szCurrent (0x%p)\n", i, pchA, szCurrent);
        ok(pchA == szCurrent + iRealOffset, "[%u] CharPrevA: pchA is 0x%p (offset %d)\n", i, pchA, pchA - szCurrent);
    }
    ok(pchA == pchEx, "[%u] CharPrevExA: pchA (0x%p) is not equal to pchEx (0x%p)\n", i, pchA, pchEx);

Cleanup:
    if (pEntry->testType != testBoth)
        szCurrent = NULL;
    HeapFree(GetProcessHeap(), 0, szStart);
    HeapFree(GetProcessHeap(), 0, szCurrent);
}

static void testDynCharPrev(const TESTS_CHARPREV *pEntry, UINT i)
{
    SIZE_T len;

    len = wcslen(pEntry->lpszStart);
    testCharPrevW(pEntry, len, i);

    if (pEntry->bWideOnly)
        return;

    testCharPrevA(pEntry, len, i);
}

static void testStatCharPrev(const ST_TESTS_CHARPREV *pEntry, UINT i)
{
    LPWSTR pchW;
    LPSTR pchA;

    pchW = CharPrevW(pEntry->wszStart, pEntry->wszCurrent);
    ok(pchW == pEntry->wszResult, "[%u] CharPrevW: pchW is 0x%p (expected 0x%p)\n", i, pchW, pEntry->wszResult);

    if (!pEntry->szStart)
        return;

    pchA = CharPrevA(pEntry->szStart, pEntry->szCurrent);
    ok(pchA == pEntry->szResult, "[%u] CharPrevA: pchA is 0x%p (expected 0x%p)\n", i, pchA, pEntry->szResult);

    pchA = CharPrevExA(TEST_ACP, pEntry->szStart, pEntry->szCurrent, 0);
    ok(pchA == pEntry->szResult, "[%u] CharPrevExA: pchA is 0x%p (expected 0x%p)\n", i, pchA, pEntry->szResult);
}

static void testStatCodePageCharPrev(const ST_CODEPAGE_TESTS_CHARPREV *pEntry, UINT i)
{
    LPSTR pchA;

    pchA = CharPrevExA(pEntry->uCodePage, pEntry->szStart, pEntry->szCurrent, 0);
    ok(pchA == pEntry->szResult, "[%u] CharPrevExA(%u): pchA is 0x%p (expected 0x%p)\n", i, pEntry->uCodePage, pchA, pEntry->szResult);
}

static void testStatCharNext(const ST_TESTS_CHARNEXT *pEntry, UINT i)
{
    LPWSTR pchW;
    LPSTR pchA;

    pchW = CharNextW(pEntry->wszString);
    ok(pchW == pEntry->wszResult, "[%u] CharNextW: pchW is 0x%p (expected 0x%p)\n", i, pchW, pEntry->wszResult);

    if (!pEntry->szString)
        return;

    pchA = CharNextA(pEntry->szString);
    ok(pchA == pEntry->szResult, "[%u] CharNextA: pchA is 0x%p (expected 0x%p)\n", i, pchA, pEntry->szResult);

    pchA = CharNextExA(TEST_ACP, pEntry->szString, 0);
    ok(pchA == pEntry->szResult, "[%u] CharNextExA: pchA is 0x%p (expected 0x%p)\n", i, pchA, pEntry->szResult);
}

static void testStatCodePageCharNext(const ST_CODEPAGE_TESTS_CHARNEXT *pEntry, UINT i)
{
    LPSTR pchA;

    pchA = CharNextExA(pEntry->uCodePage, pEntry->szString, 0);
    ok(pchA == pEntry->szResult, "[%u] CharNextExA(%u): pchA is 0x%p (expected 0x%p)\n", i, pEntry->uCodePage, pchA, pEntry->szResult);
}

static void testCharPrev(void)
{
    UINT i;

    /* Perform dynamic allocation tests */
    for (i = 0; i < _countof(TestCharPrev); i++)
    {
        testDynCharPrev(&TestCharPrev[i], i);
    }

    if (!IsWindowsVistaOrGreater())
    {
        for (i = 0; i < _countof(TestCharPrev_XP); i++)
        {
            testDynCharPrev(&TestCharPrev_XP[i], i);
        }
    }
    else
    {
        for (i = 0; i < _countof(TestCharPrev_Vista); i++)
        {
            testDynCharPrev(&TestCharPrev_Vista[i], i);
        }
    }

    /* Perform static tests */
    for (i = 0; i < _countof(TestStaticCharPrev); i++)
    {
        testStatCharPrev(&TestStaticCharPrev[i], i);
    }

    if (!IsWindowsVistaOrGreater())
    {
        for (i = 0; i < _countof(TestStaticCharPrev_XP); i++)
        {
            testStatCharPrev(&TestStaticCharPrev_XP[i], i);
        }
    }
    else
    {
        for (i = 0; i < _countof(TestStaticCharPrev_Vista); i++)
        {
            testStatCharPrev(&TestStaticCharPrev_Vista[i], i);
        }
    }

    for (i = 0; i < _countof(TestStaticCodePageCharPrev); i++)
    {
        testStatCodePageCharPrev(&TestStaticCodePageCharPrev[i], i);
    }

    /* Perform exception tests (check corner cases) */
    if (INVALID_PTR < (PVOID)wszReactOS)
    {
        ok(FALSE, "testCharPrev: unexpected INVALID PTR < wszReactOS\n");
        return;
    }
    if (INVALID_PTR < (PVOID)szReactOS)
    {
        ok(FALSE, "testCharPrev: unexpected INVALID PTR < szReactOS\n");
        return;
    }

    for (i = 0; i < _countof(TestExceptionCharPrev); i++)
    {
        LPWSTR pchW;
        LPSTR pchA;
        const EX_TESTS_CHARPREV *pEntry = &TestExceptionCharPrev[i];
        NTSTATUS Status = STATUS_SUCCESS;

        //trace("0x%p 0x%p\n", pEntry->wszStart, pEntry->wszCurrent);
        pchW = NULL;
        _SEH2_TRY
        {
            pchW = CharPrevW(pEntry->wszStart, pEntry->wszCurrent);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
        ok(Status == pEntry->resStatus, "[%u] CharPrevW: Status is 0x%lX, expected 0x%lX\n", i, Status, pEntry->resStatus);
        ok(pchW == pEntry->wszResult, "[%u] CharPrevW: pchW is 0x%p, expected 0x%p\n", i, pchW, pEntry->wszResult);

        //trace("0x%p 0x%p\n", pEntry->szStart, pEntry->szCurrent);
        pchA = NULL;
        _SEH2_TRY
        {
            pchA = CharPrevA(pEntry->szStart, pEntry->szCurrent);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
        ok(Status == pEntry->resStatus, "[%u] CharPrevA: Status is 0x%lX, expected 0x%lX\n", i, Status, pEntry->resStatus);
        ok(pchA == pEntry->szResult, "[%u] CharPrevA: pchA is 0x%p, expected 0x%p\n", i, pchA, pEntry->szResult);

        pchA = NULL;
        _SEH2_TRY
        {
            pchA = CharPrevExA(TEST_ACP, pEntry->szStart, pEntry->szCurrent, 0);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
        ok(Status == pEntry->resStatus, "[%u] CharPrevExA: Status is 0x%lX, expected 0x%lX\n", i, Status, pEntry->resStatus);
        ok(pchA == pEntry->szExResult, "[%u] CharPrevExA: pchA is 0x%p, expected 0x%p\n", i, pchA, pEntry->szExResult);
    }
}

static void testCharNext(void)
{
    UINT i;

    /* Perform static tests */
    for (i = 0; i < _countof(TestStaticCharNext); i++)
    {
        testStatCharNext(&TestStaticCharNext[i], i);
    }

    if (!IsWindowsVistaOrGreater())
    {
        for (i = 0; i < _countof(TestStaticCharNext_XP); i++)
        {
            testStatCharNext(&TestStaticCharNext_XP[i], i);
        }
    }
    else
    {
        for (i = 0; i < _countof(TestStaticCharNext_Vista); i++)
        {
            testStatCharNext(&TestStaticCharNext_Vista[i], i);
        }
    }

    for (i = 0; i < _countof(TestStaticCodePageCharNext); i++)
    {
        testStatCodePageCharNext(&TestStaticCodePageCharNext[i], i);
    }

    /* Perform exception tests (check corner cases) */
    if (INVALID_PTR < (PVOID)wszReactOS)
    {
        ok(FALSE, "testCharNext: unexpected INVALID PTR < wszReactOS\n");
        return;
    }
    if (INVALID_PTR < (PVOID)szReactOS)
    {
        ok(FALSE, "testCharNext: unexpected INVALID PTR < szReactOS\n");
        return;
    }

    for (i = 0; i < _countof(TestExceptionCharNext); i++)
    {
        LPWSTR pchW;
        LPSTR pchA;
        const EX_TESTS_CHARNEXT *pEntry = &TestExceptionCharNext[i];
        NTSTATUS Status = STATUS_SUCCESS;

        //trace("0x%p\n", pEntry->wszString);
        pchW = NULL;
        _SEH2_TRY
        {
            pchW = CharNextW(pEntry->wszString);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
        ok(Status == pEntry->resStatus, "[%u] CharNextW: Status is 0x%lX, expected 0x%lX\n", i, Status, pEntry->resStatus);
        ok(pchW == pEntry->wszResult, "[%u] CharNextW: pchW is 0x%p, expected 0x%p\n", i, pchW, pEntry->wszResult);

        //trace("0x%p 0x%p\n", pEntry->szString);
        pchA = NULL;
        _SEH2_TRY
        {
            pchA = CharNextA(pEntry->szString);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
        ok(Status == pEntry->resStatus, "[%u] CharNextA: Status is 0x%lX, expected 0x%lX\n", i, Status, pEntry->resStatus);
        ok(pchA == pEntry->szResult, "[%u] CharNextA: pchA is 0x%p, expected 0x%p\n", i, pchA, pEntry->szResult);

        pchA = NULL;
        _SEH2_TRY
        {
            pchA = CharNextExA(TEST_ACP, pEntry->szString, 0);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
        ok(Status == pEntry->resStatus, "[%u] CharNextExA: Status is 0x%lX, expected 0x%lX\n", i, Status, pEntry->resStatus);
        ok(pchA == pEntry->szResult, "[%u] CharNextExA: pchA is 0x%p, expected 0x%p\n", i, pchA, pEntry->szResult);
    }
}

START_TEST(CharFuncs)
{
    testCharPrev();
    testCharNext();
}
