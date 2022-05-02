/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for SHELLSTATE
 * PROGRAMMERS:     Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */
#include "shelltest.h"

#define NDEBUG
#include <debug.h>
#include <stdio.h>
#include <shellutils.h>
#include <strsafe.h>
#include <shlwapi.h>

/* [HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer] */
/* The contents of RegValue ShellState. */
typedef struct REGSHELLSTATE
{
    DWORD dwSize;
    SHELLSTATE ss;
} REGSHELLSTATE, *PREGSHELLSTATE;

static void dump(const char *name, const void *ptr, size_t siz)
{
    char buf[256], sz[16];

    StringCbCopyA(buf, sizeof(buf), name);
    StringCbCatA(buf, sizeof(buf), ": ");

    const BYTE *pb = reinterpret_cast<const BYTE *>(ptr);
    while (siz--)
    {
        StringCbPrintfA(sz, sizeof(sz), "%02X ", *pb++);
        StringCbCatA(buf, sizeof(buf), sz);
    }

    trace("%s\n", buf);
}

static int read_key(REGSHELLSTATE *prss)
{
    HKEY hKey;
    LONG result;
    DWORD cb;
    static const LPCWSTR s_pszExplorer =
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer";

    memset(prss, 0, sizeof(*prss));

    result = RegOpenKeyExW(HKEY_CURRENT_USER, s_pszExplorer, 0, KEY_READ, &hKey);
    ok(result == ERROR_SUCCESS, "result was %ld\n", result);
    ok(hKey != NULL, "hKey was NULL\n");

    if (result != ERROR_SUCCESS || !hKey)
    {
        skip("RegOpenKeyEx failed: %ld\n", result);
        return 1;
    }

    cb = sizeof(*prss);
    result = RegQueryValueExW(hKey, L"ShellState", NULL, NULL, reinterpret_cast<LPBYTE>(prss), &cb);
    RegCloseKey(hKey);

    ok(result == ERROR_SUCCESS, "result was %ld\n", result);
    if (result != ERROR_SUCCESS)
    {
        skip("RegQueryValueEx failed: %ld\n", result);
        return 2;
    }

    return 0;
}

extern "C" HKEY WINAPI SHGetShellKey(DWORD flags, LPCWSTR sub_key, BOOL create);

static int read_advanced_key(SHELLSTATE* pss)
{
    HKEY hKey;
    DWORD dwValue, dwSize;

    hKey = SHGetShellKey(1, L"Advanced", FALSE);
    if (hKey == NULL)
    {
        return 0;
    }

    dwSize = sizeof(dwValue);
    if (SHQueryValueExW(hKey, L"Hidden", NULL, NULL, &dwValue, &dwSize) == ERROR_SUCCESS)
    {
        pss->fShowAllObjects = (dwValue == 1);
        pss->fShowSysFiles = (dwValue == 2);
    }

    dwSize = sizeof(dwValue);
    if (SHQueryValueExW(hKey, L"HideFileExt", NULL, NULL, &dwValue, &dwSize) == ERROR_SUCCESS)
    {
        pss->fShowExtensions = (dwValue == 0);
    }

    dwSize = sizeof(dwValue);
    if (SHQueryValueExW(hKey, L"DontPrettyPath", NULL, NULL, &dwValue, &dwSize) == ERROR_SUCCESS)
    {
        pss->fDontPrettyPath = (dwValue != 0);
    }

    dwSize = sizeof(dwValue);
    if (SHQueryValueExW(hKey, L"MapNetDrvBtn", NULL, NULL, &dwValue, &dwSize) == ERROR_SUCCESS)
    {
        pss->fMapNetDrvBtn = (dwValue != 0);
    }

    dwSize = sizeof(dwValue);
    if (SHQueryValueExW(hKey, L"ShowInfoTip", NULL, NULL, &dwValue, &dwSize) == ERROR_SUCCESS)
    {
        pss->fShowInfoTip = (dwValue != 0);
    }

    dwSize = sizeof(dwValue);
    if (SHQueryValueExW(hKey, L"HideIcons", NULL, NULL, &dwValue, &dwSize) == ERROR_SUCCESS)
    {
        pss->fHideIcons = (dwValue != 0);
    }

    dwSize = sizeof(dwValue);
    if (SHQueryValueExW(hKey, L"WebView", NULL, NULL, &dwValue, &dwSize) == ERROR_SUCCESS)
    {
        pss->fWebView = (dwValue != 0);
    }

    dwSize = sizeof(dwValue);
    if (SHQueryValueExW(hKey, L"Filter", NULL, NULL, &dwValue, &dwSize) == ERROR_SUCCESS)
    {
        pss->fFilter = (dwValue != 0);
    }

    dwSize = sizeof(dwValue);
    if (SHQueryValueExW(hKey, L"ShowSuperHidden", NULL, NULL, &dwValue, &dwSize) == ERROR_SUCCESS)
    {
        pss->fShowSuperHidden = (dwValue != 0);
    }

    dwSize = sizeof(dwValue);
    if (SHQueryValueExW(hKey, L"NoNetCrawling", NULL, NULL, &dwValue, &dwSize) == ERROR_SUCCESS)
    {
        pss->fNoNetCrawling = (dwValue != 0);
    }

    RegCloseKey(hKey);
    return 0;
}

static int dump_pss(SHELLSTATE *pss)
{
    dump("SHELLSTATE", pss, sizeof(*pss));
    return 0;
}

START_TEST(ShellState)
{
    OSVERSIONINFO osinfo;
    REGSHELLSTATE rss;
    SHELLSTATE ss, *pss;
    SHELLFLAGSTATE FlagState;
    LPBYTE pb;
    int ret;

    trace("GetVersion(): 0x%08lX\n", GetVersion());

    osinfo.dwOSVersionInfoSize = sizeof(osinfo);
    GetVersionEx(&osinfo);
    trace("osinfo.dwMajorVersion: 0x%08lX\n", osinfo.dwMajorVersion);
    trace("osinfo.dwMinorVersion: 0x%08lX\n", osinfo.dwMinorVersion);
    trace("osinfo.dwBuildNumber: 0x%08lX\n", osinfo.dwBuildNumber);
    trace("osinfo.dwPlatformId: 0x%08lX\n", osinfo.dwPlatformId);

    trace("WINVER: 0x%04X\n", WINVER);
    trace("_WIN32_WINNT: 0x%04X\n", _WIN32_WINNT);
    trace("_WIN32_IE: 0x%04X\n", _WIN32_IE);
    trace("NTDDI_VERSION: 0x%08X\n", NTDDI_VERSION);

#ifdef _MSC_VER
    trace("_MSC_VER: 0x%08X\n", int(_MSC_VER));
#elif defined(__MINGW32__)
    trace("__MINGW32__: 0x%08X\n", int(__MINGW32__));
#elif defined(__clang__)
    trace("__clang__: 0x%08X\n", int(__clang__));
#else
    #error Unknown compiler.
#endif

    ok(sizeof(REGSHELLSTATE) >= 0x24, "sizeof(REGSHELLSTATE) was %d\n", (int)sizeof(REGSHELLSTATE));
    trace("sizeof(SHELLSTATE): %d\n", (int)sizeof(SHELLSTATE));
    trace("__alignof(SHELLSTATE): %d\n", (int)__alignof(SHELLSTATE));
    trace("sizeof(SHELLFLAGSTATE): %d\n", (int)sizeof(SHELLFLAGSTATE));
    trace("sizeof(CABINETSTATE): %d\n", (int)sizeof(CABINETSTATE));

    pss = &rss.ss;
    pb = reinterpret_cast<LPBYTE>(pss);

    ret = read_key(&rss);
    if (ret)
    {
        return;
    }

    dump_pss(pss);
    ok(rss.dwSize >= 0x24, "rss.dwSize was %ld (0x%lX).\n", rss.dwSize, rss.dwSize);

    read_advanced_key(&rss.ss);

#define DUMP_LONG(x) trace(#x ": 0x%08X\n", int(x));
#define DUMP_BOOL(x) trace(#x ": %d\n", !!int(x));
    DUMP_BOOL(pss->fShowAllObjects);
    DUMP_BOOL(pss->fShowExtensions);
    DUMP_BOOL(pss->fNoConfirmRecycle);
    DUMP_BOOL(pss->fShowSysFiles);
    DUMP_BOOL(pss->fShowCompColor);
    DUMP_BOOL(pss->fDoubleClickInWebView);
    DUMP_BOOL(pss->fDesktopHTML);
    DUMP_BOOL(pss->fWin95Classic);
    DUMP_BOOL(pss->fDontPrettyPath);
    DUMP_BOOL(pss->fShowAttribCol);
    DUMP_BOOL(pss->fMapNetDrvBtn);
    DUMP_BOOL(pss->fShowInfoTip);
    DUMP_BOOL(pss->fHideIcons);
    DUMP_BOOL(pss->fWebView);
    DUMP_BOOL(pss->fFilter);
    DUMP_BOOL(pss->fShowSuperHidden);
    DUMP_BOOL(pss->fNoNetCrawling);
    DUMP_LONG(pss->lParamSort);
    DUMP_LONG(pss->iSortDirection);
    DUMP_LONG(pss->version);
    DUMP_LONG(pss->lParamSort);
    DUMP_LONG(pss->iSortDirection);
    DUMP_LONG(pss->version);
    DUMP_BOOL(pss->fSepProcess);
    DUMP_BOOL(pss->fStartPanelOn);
    DUMP_BOOL(pss->fShowStartPage);
#if NTDDI_VERSION >= 0x06000000     // for future use
    DUMP_BOOL(pss->fIconsOnly);
    DUMP_BOOL(pss->fShowTypeOverlay);
    DUMP_BOOL(pss->fShowStatusBar);
#endif

#define SSF_MASK \
    (SSF_SHOWALLOBJECTS | SSF_SHOWEXTENSIONS | SSF_NOCONFIRMRECYCLE | \
     SSF_SHOWCOMPCOLOR | SSF_DOUBLECLICKINWEBVIEW | SSF_DESKTOPHTML | \
     SSF_WIN95CLASSIC | SSF_DONTPRETTYPATH | SSF_SHOWATTRIBCOL | \
     SSF_MAPNETDRVBUTTON | SSF_SHOWINFOTIP | SSF_HIDEICONS)
    // For future:
    // SSF_AUTOCHECKSELECT, SSF_ICONSONLY, SSF_SHOWTYPEOVERLAY, SSF_SHOWSTATUSBAR

    /* Get the settings */
    memset(&ss, 0, sizeof(ss));
    SHGetSetSettings(&ss, SSF_MASK, FALSE);
#define CHECK_REG_FLAG(x) ok(pss->x == ss.x, "ss.%s expected %d, was %d\n", #x, (int)pss->x, (int)ss.x)
    CHECK_REG_FLAG(fShowAllObjects);
    CHECK_REG_FLAG(fShowExtensions);
    CHECK_REG_FLAG(fNoConfirmRecycle);
    CHECK_REG_FLAG(fShowSysFiles);    // No use
    CHECK_REG_FLAG(fShowCompColor);
    CHECK_REG_FLAG(fDoubleClickInWebView);
    CHECK_REG_FLAG(fDesktopHTML);
    CHECK_REG_FLAG(fWin95Classic);
    CHECK_REG_FLAG(fDontPrettyPath);
    CHECK_REG_FLAG(fShowAttribCol);
    CHECK_REG_FLAG(fMapNetDrvBtn);
    CHECK_REG_FLAG(fShowInfoTip);
    CHECK_REG_FLAG(fHideIcons);
#if NTDDI_VERSION >= 0x06000000     // for future use
    CHECK_REG_FLAG(fAutoCheckSelect);
    CHECK_REG_FLAG(fIconsOnly);
#endif

    /* Get the flag settings */
    memset(&FlagState, 0, sizeof(FlagState));
    SHGetSettings(&FlagState, SSF_MASK);
#define CHECK_FLAG(x) ok(ss.x == FlagState.x, "FlagState.%s expected %d, was %d\n", #x, (int)ss.x, (int)FlagState.x)
    CHECK_FLAG(fShowAllObjects);
    CHECK_FLAG(fShowExtensions);
    CHECK_FLAG(fNoConfirmRecycle);
    CHECK_FLAG(fShowSysFiles);    // No use
    CHECK_FLAG(fShowCompColor);
    CHECK_FLAG(fDoubleClickInWebView);
    CHECK_FLAG(fDesktopHTML);
    CHECK_FLAG(fWin95Classic);
    CHECK_FLAG(fDontPrettyPath);
    CHECK_FLAG(fShowAttribCol);
    CHECK_FLAG(fMapNetDrvBtn);
    CHECK_FLAG(fShowInfoTip);
    CHECK_FLAG(fHideIcons);
#if NTDDI_VERSION >= 0x06000000     // for future use
    CHECK_FLAG(fAutoCheckSelect);
    CHECK_FLAG(fIconsOnly);
#endif

#if 1
    #define DO_IT(x) x
#else
    #define DO_IT(x) do { trace(#x ";\n"); x; } while (0)
#endif

    DO_IT(memset(pss, 0, sizeof(*pss)));
    DO_IT(pss->dwWin95Unused = 1);
    ok(pb[4] == 0x01 || dump_pss(pss), "Unexpected pss ^\n");

    DO_IT(memset(pss, 0, sizeof(*pss)));
    DO_IT(pss->lParamSort = 1);
    ok(pb[12] == 0x01 || dump_pss(pss), "Unexpected pss ^\n");

    DO_IT(memset(pss, 0, sizeof(*pss)));
    DO_IT(pss->iSortDirection = 0xDEADBEEF);
    ok(*(UNALIGNED DWORD *)(pb + 16) == 0xDEADBEEF || dump_pss(pss), "Unexpected pss ^\n");

    DO_IT(memset(pss, 0, sizeof(*pss)));
    DO_IT(pss->version = 0xDEADBEEF);
    ok(*(UNALIGNED DWORD *)(pb + 20) == 0xDEADBEEF || dump_pss(pss), "Unexpected pss ^\n");

    DO_IT(memset(pss, 0, sizeof(*pss)));
    DO_IT(pss->fSepProcess = TRUE);
    ok(pb[28] == 0x01 || dump_pss(pss), "Unexpected pss ^\n");
}
