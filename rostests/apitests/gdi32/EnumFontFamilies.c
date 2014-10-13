/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for EnumFontFamilies[Ex]
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>

#include <wingdi.h>
#include <winddi.h>
#include <strsafe.h>

static BYTE ContextContinue;
static BYTE ContextStop;

static int EnumProcCalls;
static ENUMLOGFONTA LastFontA;
static ENUMLOGFONTW LastFontW;

static
int
CALLBACK
EnumProcA(
    _In_ const LOGFONTA *elf,
    _In_ const TEXTMETRICA *ntm,
    _In_ DWORD FontType,
    _In_ LPARAM lParam)
{
    EnumProcCalls++;

    ok(lParam == (LPARAM)&ContextContinue ||
       lParam == (LPARAM)&ContextStop,
       "Context is %p, expected %p or %p\n",
       (PVOID)lParam, &ContextContinue, &ContextStop);

    LastFontA = *(ENUMLOGFONTA *)elf;
    return lParam == (LPARAM)&ContextContinue ? 7 : 0;
}

static
int
CALLBACK
EnumProcW(
    _In_ const LOGFONTW *elf,
    _In_ const TEXTMETRICW *ntm,
    _In_ DWORD FontType,
    _In_ LPARAM lParam)
{
    EnumProcCalls++;

    ok(lParam == (LPARAM)&ContextContinue ||
       lParam == (LPARAM)&ContextStop,
       "Context is %p, expected %p or %p\n",
       (PVOID)lParam, &ContextContinue, &ContextStop);

    LastFontW = *(ENUMLOGFONTW *)elf;
    return lParam == (LPARAM)&ContextContinue ? 7 : 0;
}

static
void
TestEnumFontFamiliesA(
    _In_ HDC hdc,
    _In_ PCSTR FontName)
{
    int ret;
    DWORD error;

    EnumProcCalls = 0;
    SetLastError(0xdeadbeef);
    ret = EnumFontFamiliesA(hdc,
                            FontName,
                            EnumProcA,
                            (LPARAM)&ContextContinue);
    error = GetLastError();
    ok(ret == 1, "ret is %d, expected 0\n", ret);
    ok(error == 0xdeadbeef, "error is %lu\n", error);
    ok(EnumProcCalls == 0, "EnumProcCalls is %d\n", EnumProcCalls);
}

static
void
TestEnumFontFamiliesW(
    _In_ HDC hdc,
    _In_ PCWSTR FontName)
{
    int ret;
    DWORD error;

    EnumProcCalls = 0;
    SetLastError(0xdeadbeef);
    ret = EnumFontFamiliesW(hdc,
                            FontName,
                            EnumProcW,
                            (LPARAM)&ContextContinue);
    error = GetLastError();
    ok(ret == 1, "ret is %d, expected 0\n", ret);
    ok(error == 0xdeadbeef, "error is %lu\n", error);
    ok(EnumProcCalls == 0, "EnumProcCalls is %d\n", EnumProcCalls);
}

static
void
TestEnumFontFamiliesExA(
    _In_ HDC hdc,
    _In_ PCSTR FontName)
{
    int ret;
    DWORD error;
    LOGFONTA lf;

    EnumProcCalls = 0;
    ZeroMemory(&lf, sizeof(lf));
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfPitchAndFamily = 0;
    StringCbCopyA(lf.lfFaceName, sizeof(lf.lfFaceName), FontName);
    SetLastError(0xdeadbeef);
    ret = EnumFontFamiliesExA(hdc,
                              &lf,
                              EnumProcA,
                              (LPARAM)&ContextContinue,
                              0);
    error = GetLastError();
    ok(ret == 1, "ret is %d, expected 0\n", ret);
    ok(error == 0xdeadbeef, "error is %lu\n", error);
    ok(EnumProcCalls == 0, "EnumProcCalls is %d\n", EnumProcCalls);
}

static
void
TestEnumFontFamiliesExW(
    _In_ HDC hdc,
    _In_ PCWSTR FontName)
{
    int ret;
    DWORD error;
    LOGFONTW lf;

    EnumProcCalls = 0;
    ZeroMemory(&lf, sizeof(lf));
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfPitchAndFamily = 0;
    StringCbCopyW(lf.lfFaceName, sizeof(lf.lfFaceName), FontName);
    SetLastError(0xdeadbeef);
    ret = EnumFontFamiliesExW(hdc,
                              &lf,
                              EnumProcW,
                              (LPARAM)&ContextContinue,
                              0);
    error = GetLastError();
    ok(ret == 1, "ret is %d, expected 0\n", ret);
    ok(error == 0xdeadbeef, "error is %lu\n", error);
    ok(EnumProcCalls == 0, "EnumProcCalls is %d\n", EnumProcCalls);
}

START_TEST(EnumFontFamilies)
{
    HDC hdc;

    hdc = CreateCompatibleDC(NULL);
    if (!hdc)
    {
        skip("No DC\n");
        return;
    }

    TestEnumFontFamiliesA(hdc, "ThisFontDoesNotExist");
    TestEnumFontFamiliesW(hdc, L"ThisFontDoesNotExist");
    TestEnumFontFamiliesExA(hdc, "ThisFontDoesNotExist");
    TestEnumFontFamiliesExW(hdc, L"ThisFontDoesNotExist");

    DeleteDC(hdc);
}

