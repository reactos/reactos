/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for SHELLSTATE
 * PROGRAMMERS:     Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "shelltest.h"
#include "shell32_apitest_sub.h"

#define NDEBUG
#include <debug.h>
#include <stdio.h>
#include <shellutils.h>
#include <strsafe.h>
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <versionhelpers.h>

/* [HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer] */
/* The contents of RegValue ShellState. */
typedef struct REGSHELLSTATE
{
    DWORD dwSize;
    SHELLSTATE ss;
} REGSHELLSTATE, *PREGSHELLSTATE;

static const LPCWSTR s_pszExplorer =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer";
static const LPCWSTR s_pszShellState =
    L"ShellState";

static ULONG dump(const char *name, const void *ptr, size_t siz)
{
    char buf[256], sz[16];
    ULONG ret = 0;

    StringCbCopyA(buf, sizeof(buf), name);
    StringCbCatA(buf, sizeof(buf), ": ");

    const BYTE *pb = reinterpret_cast<const BYTE *>(ptr);
    while (siz--)
    {
        if (*pb)
            ret++;
        StringCbPrintfA(sz, sizeof(sz), "%02X ", *pb++);
        StringCbCatA(buf, sizeof(buf), sz);
    }

    trace("%s\n", buf);
    return ret;
}

static int read_key(REGSHELLSTATE *prss)
{
    HKEY hKey;
    LONG result;
    DWORD cb;

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
    result = RegQueryValueExW(hKey, s_pszShellState, NULL, NULL, reinterpret_cast<LPBYTE>(prss), &cb);
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS)
    {
        skip("RegQueryValueEx failed: %ld\n", result);
        return 2;
    }

    return 0;
}

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
    dump("SHELLSTATE KEY", pss, sizeof(*pss));
    return 0;
}

static HWND start_sub(void)
{
    int retries = 50;
    WCHAR s_szSubProgram[MAX_PATH]; // shell32_apitest_sub.exe
    if (!FindSubProgram(s_szSubProgram, _countof(s_szSubProgram)))
    {
        trace("shell32_apitest_sub.exe not found\n");
        return 0;
    }

    // Close the SUB_CLASSNAME windows
    DoWaitForWindow(SUB_CLASSNAME, SUB_CLASSNAME, TRUE, TRUE);

    // Execute sub program
    HINSTANCE hinst = ShellExecuteW(NULL, NULL, s_szSubProgram, L"----", NULL, SW_HIDE);
    if ((INT_PTR)hinst <= 32)
    {
        trace("Unable to run shell32_apitest_sub.exe.\n");
        return 0;
    }

Retry:
    HWND s_hSubWnd = DoWaitForWindow(SUB_CLASSNAME, SUB_CLASSNAME, FALSE, FALSE);
    if (!s_hSubWnd)
    {
        if (--retries > 0)
        {
            Sleep(100);
            goto Retry;
        }
        trace("Unable to find sub-program window.\n");
        return 0;
    }

    return s_hSubWnd;
}

static void stop_sub(HWND s_hSubWnd)
{
    PostMessageW(s_hSubWnd, WM_COMMAND, IDNO, 0); // Finish
    DoWaitForWindow(SUB_CLASSNAME, SUB_CLASSNAME, TRUE, TRUE); // Close sub-windows
}

static LONG del_state_key()
{
    HKEY hKey;
    LONG res = RegOpenKeyExW(HKEY_CURRENT_USER, s_pszExplorer, 0, KEY_SET_VALUE, &hKey);
    if (res != ERROR_SUCCESS || !hKey)
    {
        trace("RegOpenKeyEx failed: %ld, skipping default values test\n", res);
        return res;
    }

    res = RegDeleteValueW(hKey, s_pszShellState);
    RegCloseKey(hKey);
    return res;
}

static LONG state_key_exists()
{
    HKEY hKey;
    LONG res = RegOpenKeyExW(HKEY_CURRENT_USER, s_pszExplorer, 0, KEY_QUERY_VALUE, &hKey);
    if (res != ERROR_SUCCESS || !hKey)
    {
        trace("RegOpenKeyEx failed: %ld, skipping default values test\n", res);
        return res;
    }

    res = RegQueryValueExW(hKey, s_pszShellState, NULL, NULL, NULL, NULL);
    RegCloseKey(hKey);
    return res;
}

static void process_test(SHELLSTATE *pss)
{
    SHELLSTATE bak, ss1, ss2;
    SHELLSTATE_SUB sub;
    LONG result;
    COPYDATASTRUCT copyData = { ID_SHSTATE, sizeof(sub), &sub };

    // Backup current state
    memset(&bak, 0, sizeof(bak));
    SHGetSetSettings(&bak, MAXDWORD, FALSE);

    memset(&ss1, 0, sizeof(ss1));
    SHGetSetSettings(&ss1, MAXDWORD, FALSE);

    // Test 1: Read one field, then write another
    HWND s_hSubWnd = start_sub();
    if (!s_hSubWnd)
    {
        skip("Skipping read/write value test 1\n");
        goto Cleanup;
    }

    memset(&sub, 0, sizeof(sub));

    sub.dwMask = SSF_DOUBLECLICKINWEBVIEW;
    sub.bSet = FALSE;
    SendMessageW(s_hSubWnd, WM_COPYDATA, 0, (LPARAM)&copyData);

    sub.ss.fNoConfirmRecycle = !ss1.fNoConfirmRecycle;
    ss1.fNoConfirmRecycle = sub.ss.fNoConfirmRecycle;
    sub.dwMask = SSF_NOCONFIRMRECYCLE;
    sub.bSet = TRUE;
    SendMessageW(s_hSubWnd, WM_COPYDATA, 0, (LPARAM)&copyData);

    memset(&ss2, 0, sizeof(ss2));
    SHGetSetSettings(&ss2, MAXDWORD, FALSE);

#define CHECK_BITS(x) ok(ss1.x == ss2.x, "ss2.%s expected %d, was %d\n", #x, (int)ss1.x, (int)ss2.x)
    CHECK_BITS(fNoConfirmRecycle);
    CHECK_BITS(fDoubleClickInWebView);
    CHECK_BITS(fDesktopHTML);
    CHECK_BITS(fWin95Classic);
    CHECK_BITS(lParamSort);
    CHECK_BITS(iSortDirection);
    CHECK_BITS(fStartPanelOn);
    CHECK_BITS(fShowCompColor);
    CHECK_BITS(fShowAttribCol);
    CHECK_BITS(fShowInfoTip);

    stop_sub(s_hSubWnd);

    // Test 2: Only write a field
    s_hSubWnd = start_sub();
    if (!s_hSubWnd)
    {
        skip("Skipping write value test 2\n");
        goto Cleanup;
    }

    memset(&sub, 0, sizeof(sub));

    sub.ss.fNoConfirmRecycle = !ss1.fNoConfirmRecycle;
    ss1.fNoConfirmRecycle = sub.ss.fNoConfirmRecycle;
    sub.dwMask = SSF_NOCONFIRMRECYCLE;
    sub.bSet = TRUE;
    SendMessageW(s_hSubWnd, WM_COPYDATA, 0, (LPARAM)&copyData);

    memset(&ss2, 0, sizeof(ss2));
    SHGetSetSettings(&ss2, MAXDWORD, FALSE);

    CHECK_BITS(fNoConfirmRecycle);
    CHECK_BITS(fDoubleClickInWebView);
    CHECK_BITS(fDesktopHTML);
    CHECK_BITS(fWin95Classic);
    CHECK_BITS(lParamSort);
    CHECK_BITS(iSortDirection);
    CHECK_BITS(fStartPanelOn);
    CHECK_BITS(fShowCompColor);
    CHECK_BITS(fShowAttribCol);
    CHECK_BITS(fShowInfoTip);

    stop_sub(s_hSubWnd);

    // Test 3: Write another one
    ss1.fNoConfirmRecycle = 0;
    SHGetSetSettings(&ss1, SSF_NOCONFIRMRECYCLE, TRUE);
    ss1.fNoConfirmRecycle = 1;
    ss1.fDoubleClickInWebView = 1;
    SHGetSetSettings(&ss1, SSF_NOCONFIRMRECYCLE | SSF_DOUBLECLICKINWEBVIEW, TRUE);

    s_hSubWnd = start_sub();
    if (!s_hSubWnd)
    {
        skip("Skipping write value test 3\n");
        goto Cleanup;
    }

    memset(&sub, 0, sizeof(sub));

    sub.ss.fDoubleClickInWebView = !ss1.fDoubleClickInWebView;
    ss1.fDoubleClickInWebView = sub.ss.fDoubleClickInWebView;
    sub.dwMask = SSF_DOUBLECLICKINWEBVIEW;
    sub.bSet = TRUE;
    SendMessageW(s_hSubWnd, WM_COPYDATA, 0, (LPARAM)&copyData);

    memset(&ss2, 0, sizeof(ss2));
    SHGetSetSettings(&ss2, MAXDWORD, FALSE);

    CHECK_BITS(fNoConfirmRecycle);
    CHECK_BITS(fDoubleClickInWebView);
    CHECK_BITS(fDesktopHTML);
    CHECK_BITS(fWin95Classic);
    CHECK_BITS(lParamSort);
    CHECK_BITS(iSortDirection);
    CHECK_BITS(fStartPanelOn);
    CHECK_BITS(fShowCompColor);
    CHECK_BITS(fShowAttribCol);
    CHECK_BITS(fShowInfoTip);

    stop_sub(s_hSubWnd);

    // Test 4: Check default state values
    ss1.fNoConfirmRecycle = 1;
    ss1.fDoubleClickInWebView = 0;
    ss1.fDesktopHTML = 1;
    ss1.fWin95Classic = 1;
    ss1.fStartPanelOn = 0;
    ss1.fShowCompColor = 0;
    ss1.fShowAttribCol = 1;
    ss1.fShowInfoTip = 0;
    SHGetSetSettings(&ss1, MAXDWORD, TRUE);

    // ShellState key is expected to always exist at this point (we have set the state above)
    result = del_state_key();
    ok(result == ERROR_SUCCESS, "del_state_key failed: %ld\n", result);
    if (result != ERROR_SUCCESS)
    {
        skip("Skipping default values test\n");
        goto Cleanup;
    }

    s_hSubWnd = start_sub();
    if (!s_hSubWnd)
    {
        skip("Skipping default values test\n");
        goto Cleanup;
    }

    memset(&sub, 0, sizeof(sub));

    sub.dwMask = SSF_NOCONFIRMRECYCLE;
    sub.bGetSet = TRUE;
    SendMessageW(s_hSubWnd, WM_COPYDATA, 0, (LPARAM)&copyData);

    memset(&ss2, 0, sizeof(ss2));
    SHGetSetSettings(&ss2, MAXDWORD, FALSE);

#define CHECK_TRUE(x) ok(ss2.x, "ss2.%s expected to be TRUE\n", #x)
#define CHECK_FALSE(x) ok(!ss2.x, "ss2.%s expected to be FALSE\n", #x)
    if (GetNTVersion() < _WIN32_WINNT_WIN8)
        CHECK_FALSE(fNoConfirmRecycle); // Different between Windows versions
    else
        CHECK_TRUE(fNoConfirmRecycle);
    CHECK_TRUE(fDoubleClickInWebView);
    CHECK_FALSE(fDesktopHTML);
    CHECK_FALSE(fWin95Classic);
    if (IsReactOS())
        CHECK_FALSE(fStartPanelOn); // CORE-12158: Disabled intentionally, as no Modern Start menu yet
    else
        CHECK_TRUE(fStartPanelOn);
    CHECK_BITS(fShowCompColor); // Keeps the state
    CHECK_FALSE(fShowAttribCol);
    CHECK_BITS(fShowInfoTip); // Keeps the state

    stop_sub(s_hSubWnd);

    // Test 5: Check when the registry key is created
    result = del_state_key();
    ok(result == ERROR_SUCCESS, "del_state_key failed: %ld\n", result);
    if (result != ERROR_SUCCESS)
    {
        skip("Skipping registry key test\n");
        goto Cleanup;
    }

    s_hSubWnd = start_sub();
    if (!s_hSubWnd)
    {
        skip("Skipping default values test\n");
        goto Cleanup;
    }

    memset(&sub, 0, sizeof(sub));

    // Just get the field
    sub.dwMask = SSF_DOUBLECLICKINWEBVIEW;
    sub.bSet = FALSE;
    SendMessageW(s_hSubWnd, WM_COPYDATA, 0, (LPARAM)&copyData);

    stop_sub(s_hSubWnd);

    result = state_key_exists();
    if (GetNTVersion() < _WIN32_WINNT_WIN7)
    {
        ok(result == ERROR_FILE_NOT_FOUND, "There should be no registry key: %ld\n", result);
    }
    else
    {
        trace("state_key_exists(): %ld\n", result); // This may be random
    }
    if (result == ERROR_SUCCESS)
        del_state_key();

    s_hSubWnd = start_sub();
    if (!s_hSubWnd)
    {
        skip("Skipping default values test\n");
        goto Cleanup;
    }

    memset(&sub, 0, sizeof(sub));

    // Set the field to expected default value
    sub.ss.fDoubleClickInWebView = 1;
    sub.dwMask = SSF_DOUBLECLICKINWEBVIEW;
    sub.bSet = TRUE;
    SendMessageW(s_hSubWnd, WM_COPYDATA, 0, (LPARAM)&copyData);

    stop_sub(s_hSubWnd);

    result = state_key_exists();
    if (GetNTVersion() < _WIN32_WINNT_WIN7)
    {
        ok(result == ERROR_FILE_NOT_FOUND, "There should be no registry key: %ld\n", result);
    }
    else
    {
        trace("state_key_exists(): %ld\n", result); // This may be random
    }
    if (result == ERROR_SUCCESS)
        del_state_key();

    s_hSubWnd = start_sub();
    if (!s_hSubWnd)
    {
        skip("Skipping default values test\n");
        goto Cleanup;
    }

    memset(&sub, 0, sizeof(sub));

    // Set the field to different value
    sub.ss.fDoubleClickInWebView = 0;
    sub.dwMask = SSF_DOUBLECLICKINWEBVIEW;
    sub.bSet = TRUE;
    SendMessageW(s_hSubWnd, WM_COPYDATA, 0, (LPARAM)&copyData);

    stop_sub(s_hSubWnd);

    result = state_key_exists();
    ok(result == ERROR_SUCCESS, "Registry state value expected: %ld\n", result);

Cleanup:
    // Restore current state
    SHGetSetSettings(&bak, MAXDWORD, TRUE);

    return;
}

START_TEST(ShellState)
{
    OSVERSIONINFO osinfo;
    REGSHELLSTATE rss;
    SHELLSTATE ss, *pss;
    SHELLFLAGSTATE FlagState;
    BOOL GotReg = TRUE;
    LPBYTE pb;
    int ret;
    ULONG c1, c2;

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
        GotReg = FALSE;
        goto SkipReg;
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
    DUMP_BOOL(pss->fSepProcess);
    DUMP_BOOL(pss->fStartPanelOn);
    DUMP_BOOL(pss->fShowStartPage);
#if NTDDI_VERSION >= NTDDI_VISTA    // for future use
    DUMP_BOOL(pss->fAutoCheckSelect);
    DUMP_BOOL(pss->fIconsOnly);
    DUMP_BOOL(pss->fShowTypeOverlay);
#endif
#if NTDDI_VERSION >= NTDDI_WIN8     // for future use
    DUMP_BOOL(pss->fShowStatusBar);
#endif

SkipReg:
#define SSF_MASK \
    (SSF_SHOWALLOBJECTS | SSF_SHOWEXTENSIONS | SSF_NOCONFIRMRECYCLE | SSF_SHOWSYSFILES | \
     SSF_SHOWCOMPCOLOR | SSF_DOUBLECLICKINWEBVIEW | SSF_DESKTOPHTML | \
     SSF_WIN95CLASSIC | SSF_DONTPRETTYPATH | SSF_SHOWATTRIBCOL | \
     SSF_MAPNETDRVBUTTON | SSF_SHOWINFOTIP | SSF_HIDEICONS)
    // For future:
    // SSF_AUTOCHECKSELECT, SSF_ICONSONLY, SSF_SHOWTYPEOVERLAY, SSF_SHOWSTATUSBAR

    /* Get the settings */
    memset(&ss, 0, sizeof(ss));
    SHGetSetSettings(&ss, SSF_MASK, FALSE);
    if (GotReg)
    {
#define CHECK_REG_FLAG(x) ok(pss->x == ss.x, "ss.%s expected %d, was %d\n", #x, (int)pss->x, (int)ss.x)
    CHECK_REG_FLAG(fShowAllObjects);
    CHECK_REG_FLAG(fShowExtensions);
    CHECK_REG_FLAG(fNoConfirmRecycle);
    CHECK_REG_FLAG(fShowSysFiles);
    CHECK_REG_FLAG(fShowCompColor);
    CHECK_REG_FLAG(fDoubleClickInWebView);
    CHECK_REG_FLAG(fDesktopHTML);
    CHECK_REG_FLAG(fWin95Classic);
    CHECK_REG_FLAG(fDontPrettyPath);
    CHECK_REG_FLAG(fShowAttribCol);
    CHECK_REG_FLAG(fMapNetDrvBtn);
    CHECK_REG_FLAG(fShowInfoTip);
    CHECK_REG_FLAG(fHideIcons);
#if NTDDI_VERSION >= NTDDI_VISTA    // for future use
    CHECK_REG_FLAG(fAutoCheckSelect);
    CHECK_REG_FLAG(fIconsOnly);
    CHECK_REG_FLAG(fShowTypeOverlay);
#endif
#if NTDDI_VERSION >= NTDDI_WIN8     // for future use
    CHECK_REG_FLAG(fShowStatusBar);
#endif
    }
    c1 = dump("SHELLSTATE SSF_MASK", &ss, sizeof(ss));

    memset(&ss, 0, sizeof(ss));
    SHGetSetSettings(&ss, MAXDWORD, FALSE);
    c2 = dump("SHELLSTATE MAXDWORD", &ss, sizeof(ss));

    ok(c2 >= c1, "Set bytes in MAXDWORD state expected to be greater than %u, was %u\n", c1, c2);

    /* Get the flag settings */
    memset(&FlagState, 0, sizeof(FlagState));
    SHGetSettings(&FlagState, SSF_MASK);
#define CHECK_FLAG(x) ok(ss.x == FlagState.x, "FlagState.%s expected %d, was %d\n", #x, (int)ss.x, (int)FlagState.x)
    CHECK_FLAG(fShowAllObjects);
    CHECK_FLAG(fShowExtensions);
    CHECK_FLAG(fNoConfirmRecycle);
    CHECK_FLAG(fShowSysFiles);
    CHECK_FLAG(fShowCompColor);
    CHECK_FLAG(fDoubleClickInWebView);
    CHECK_FLAG(fDesktopHTML);
    CHECK_FLAG(fWin95Classic);
    CHECK_FLAG(fDontPrettyPath);
    CHECK_FLAG(fShowAttribCol);
    CHECK_FLAG(fMapNetDrvBtn);
    CHECK_FLAG(fShowInfoTip);
    CHECK_FLAG(fHideIcons);
#if NTDDI_VERSION >= NTDDI_VISTA    // for future use
    CHECK_FLAG(fAutoCheckSelect);
    CHECK_FLAG(fIconsOnly);
#endif
    c1 = dump("SHELLFLAGSTATE SSF_MASK", &FlagState, sizeof(FlagState));

    memset(&FlagState, 0, sizeof(FlagState));
    SHGetSettings(&FlagState, MAXDWORD);
    c2 = dump("SHELLFLAGSTATE MAXDWORD", &FlagState, sizeof(FlagState));

    ok(c2 >= c1, "Set bytes in MAXDWORD flags expected to be greater than %u, was %u\n", c1, c2);

    /* Inter-process tests */
    process_test(pss);

    /* Structure alignment tests */
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
