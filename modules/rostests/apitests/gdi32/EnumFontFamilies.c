/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for EnumFontFamilies[Ex]
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>

#include <winnls.h>
#include <wingdi.h>
#include <winddi.h>
#include <strsafe.h>

static BYTE ContextContinue;
static BYTE ContextStop;

static int EnumProcCalls;
static ENUMLOGFONTA LastFontA;
static ENUMLOGFONTW LastFontW;

typedef int WRAP_ENUM_FONT_FAMILIES(_In_ HDC, _In_ PCWSTR, _In_ PVOID, _In_ LPARAM);
typedef WRAP_ENUM_FONT_FAMILIES *PWRAP_ENUM_FONT_FAMILIES;

static
int
WrapEnumFontFamiliesA(
    _In_ HDC hdc,
    _In_ PCWSTR Family,
    _In_ PVOID EnumProc,
    _In_ LPARAM lParam)
{
    char FamilyA[100];
    WideCharToMultiByte(CP_ACP, 0, Family, -1, FamilyA, sizeof(FamilyA), NULL, NULL);
    return EnumFontFamiliesA(hdc, FamilyA, EnumProc, lParam);
}

static
int
WrapEnumFontFamiliesW(
    _In_ HDC hdc,
    _In_ PCWSTR Family,
    _In_ PVOID EnumProc,
    _In_ LPARAM lParam)
{
    return EnumFontFamiliesW(hdc, Family, EnumProc, lParam);
}

static
int
WrapEnumFontFamiliesExA(
    _In_ HDC hdc,
    _In_ PCWSTR Family,
    _In_ PVOID EnumProc,
    _In_ LPARAM lParam)
{
    LOGFONTA lf;

    ZeroMemory(&lf, sizeof(lf));
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfPitchAndFamily = 0;
    WideCharToMultiByte(CP_ACP, 0, Family, -1, lf.lfFaceName, sizeof(lf.lfFaceName), NULL, NULL);
    return EnumFontFamiliesExA(hdc, &lf, EnumProc, lParam, 0);
}

static
int
WrapEnumFontFamiliesExW(
    _In_ HDC hdc,
    _In_ PCWSTR Family,
    _In_ PVOID EnumProc,
    _In_ LPARAM lParam)
{
    LOGFONTW lf;

    ZeroMemory(&lf, sizeof(lf));
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfPitchAndFamily = 0;
    StringCbCopyW(lf.lfFaceName, sizeof(lf.lfFaceName), Family);
    return EnumFontFamiliesExW(hdc, &lf, EnumProc, lParam, 0);
}

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
TestEnumFontFamilies(
    _In_ HDC hdc,
    _In_ PCWSTR FontName,
    _In_ BOOLEAN ExpectToFind)
{
    const struct
    {
        PWRAP_ENUM_FONT_FAMILIES Wrapper;
        PCSTR Name;
        BOOLEAN Wide;
    } *fun, functions[] =
    {
        { WrapEnumFontFamiliesA,    "EnumFontFamiliesA",    FALSE },
        { WrapEnumFontFamiliesW,    "EnumFontFamiliesW",    TRUE },
        { WrapEnumFontFamiliesExA,  "EnumFontFamiliesExA",  FALSE },
        { WrapEnumFontFamiliesExW,  "EnumFontFamiliesExW",  TRUE },
    };
    const struct
    {
        PVOID Context;
        PCSTR Description;
    } *ctx, contexts[] =
    {
        { &ContextContinue, "Continue" },
        { &ContextStop,     "Stop" },
    };
    int ret;
    DWORD error;
    unsigned iFunction;
    unsigned iContext;

    for (iContext = 0; iContext < _countof(contexts); iContext++)
    {
        ctx = &contexts[iContext];
        for (iFunction = 0; iFunction < _countof(functions); iFunction++)
        {
            fun = &functions[iFunction];
            EnumProcCalls = 0;
            SetLastError(0xdeadbeef);
            ret = fun->Wrapper(hdc,
                               FontName,
                               fun->Wide ? (PVOID)EnumProcW : (PVOID)EnumProcA,
                               (LPARAM)ctx->Context);
            error = GetLastError();
            ok(error == 0xdeadbeef, "[%s, %s, '%ls'] error is %lu\n", fun->Name, ctx->Description, FontName, error);
            if (ExpectToFind)
            {
                if (ctx->Context == &ContextContinue)
                {
                    ok(ret == 7, "[%s, %s, '%ls'] ret is %d, expected 7\n", fun->Name, ctx->Description, FontName, ret);
                    ok(EnumProcCalls >= 1, "[%s, %s, '%ls'] EnumProcCalls is %d\n", fun->Name, ctx->Description, FontName, EnumProcCalls);
                }
                else
                {
                    ok(ret == 0, "[%s, %s, '%ls'] ret is %d, expected 0\n", fun->Name, ctx->Description, FontName, ret);
                    ok(EnumProcCalls == 1, "[%s, %s, '%ls'] EnumProcCalls is %d\n", fun->Name, ctx->Description, FontName, EnumProcCalls);
                }
            }
            else
            {
                ok(ret == 1, "[%s, %s, '%ls'] ret is %d, expected 1\n", fun->Name, ctx->Description, FontName, ret);
                ok(EnumProcCalls == 0, "[%s, %s, '%ls'] EnumProcCalls is %d\n", fun->Name, ctx->Description, FontName, EnumProcCalls);
            }
        }
    }
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

    TestEnumFontFamilies(hdc, L"ThisFontDoesNotExist", FALSE);
    /* Basic fonts that should be installed */
    TestEnumFontFamilies(hdc, L"MS Sans Serif", TRUE);
    TestEnumFontFamilies(hdc, L"Tahoma", TRUE);
    TestEnumFontFamilies(hdc, L"System", TRUE);
    /* Show case insensitivity */
    TestEnumFontFamilies(hdc, L"tahOmA", TRUE);
    /* Special fonts that we have a hack for in win32k ;) */
    TestEnumFontFamilies(hdc, L"Marlett", TRUE);
    TestEnumFontFamilies(hdc, L"Symbol", TRUE);
    TestEnumFontFamilies(hdc, L"VGA", FALSE);

    DeleteDC(hdc);
}

