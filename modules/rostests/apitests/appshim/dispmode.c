/*
 * PROJECT:     appshim_apitest
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for display mode shims
 * COPYRIGHT:   Copyright 2016-2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#ifdef __REACTOS__
#include <ntndk.h>
#else
#include <winternl.h>
#endif
#include <stdio.h>
#include <strsafe.h>
#include "wine/test.h"
#include "apitest_iathook.h"
#include "appshim_apitest.h"

static DWORD g_Version;
#define WINVER_ANY     0

/* aclayers.dll / acgenral.dll */
static tGETHOOKAPIS pGetHookAPIs;
static BOOL(WINAPI* pNotifyShims)(DWORD fdwReason, PVOID ptr);


DWORD get_module_version(HMODULE mod)
{
    DWORD dwVersion = 0;
    HRSRC hResInfo = FindResource(mod, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
    DWORD dwSize = SizeofResource(mod, hResInfo);
    if (hResInfo && dwSize)
    {
        VS_FIXEDFILEINFO *lpFfi;
        UINT uLen;

        HGLOBAL hResData = LoadResource(mod, hResInfo);
        LPVOID pRes = LockResource(hResData);
        HLOCAL pResCopy = LocalAlloc(LMEM_FIXED, dwSize);

        CopyMemory(pResCopy, pRes, dwSize);
        FreeResource(hResData);

        if (VerQueryValueW(pResCopy, L"\\", (LPVOID*)&lpFfi, &uLen))
        {
            dwVersion = (HIWORD(lpFfi->dwProductVersionMS) << 8) | LOWORD(lpFfi->dwProductVersionMS);
            if (!dwVersion)
                dwVersion = (HIWORD(lpFfi->dwFileVersionMS) << 8) | LOWORD(lpFfi->dwFileVersionMS);
        }

        LocalFree(pResCopy);
    }

    return dwVersion;
}

static LONG g_ChangeCount;
static DEVMODEA g_LastDevmode;
static DWORD g_LastFlags;

static LONG (WINAPI *pChangeDisplaySettingsA)(_In_opt_ PDEVMODEA lpDevMode, _In_ DWORD dwflags);
LONG WINAPI mChangeDisplaySettingsA(_In_opt_ PDEVMODEA lpDevMode, _In_ DWORD dwflags)
{
    g_ChangeCount++;
    g_LastDevmode = *lpDevMode;
    g_LastFlags = dwflags;

    return DISP_CHANGE_FAILED;
}

static LONG g_EnumCount;
static BOOL bFix = TRUE;

static BOOL (WINAPI *pEnumDisplaySettingsA)(_In_opt_ LPCSTR lpszDeviceName, _In_ DWORD iModeNum, _Inout_ PDEVMODEA lpDevMode);
BOOL WINAPI mEnumDisplaySettingsA(_In_opt_ LPCSTR lpszDeviceName, _In_ DWORD iModeNum, _Inout_ PDEVMODEA lpDevMode)
{
    g_EnumCount++;
    if (pEnumDisplaySettingsA(lpszDeviceName, iModeNum, lpDevMode))
    {
        if (bFix)
        {
            if (lpDevMode && lpDevMode->dmBitsPerPel == 8)
            {
                trace("Running at 8bpp, faking 16\n");
                lpDevMode->dmBitsPerPel = 16;
            }
            if (lpDevMode && lpDevMode->dmPelsWidth == 640 && lpDevMode->dmPelsHeight == 480)
            {
                trace("Running at 640x480, faking 800x600\n");
                lpDevMode->dmPelsWidth = 800;
                lpDevMode->dmPelsHeight = 600;
            }
        }
        else
        {
            if (lpDevMode)
            {
                lpDevMode->dmBitsPerPel = 8;
                lpDevMode->dmPelsWidth = 640;
                lpDevMode->dmPelsHeight = 480;
            }
        }
        return TRUE;
    }
    return FALSE;
}



static LONG g_ThemeCount;
static DWORD g_LastThemeFlags;

static void (WINAPI *pSetThemeAppProperties)(DWORD dwFlags);
void WINAPI mSetThemeAppProperties(DWORD dwFlags)
{
    g_ThemeCount++;
    g_LastThemeFlags = dwFlags;
}


static void pre_8bit(void)
{
    g_ChangeCount = 0;
    memset(&g_LastDevmode, 0, sizeof(g_LastDevmode));
    g_LastFlags = 0xffffffff;
    g_EnumCount = 0;
}

static void pre_8bit_2(void)
{
    bFix = FALSE;

    pre_8bit();
}

static void post_8bit(void)
{
    ok_int(g_ChangeCount, 1);
    ok_hex(g_LastDevmode.dmFields & DM_BITSPERPEL, DM_BITSPERPEL);
    ok_int(g_LastDevmode.dmBitsPerPel, 8);
    ok_hex(g_LastFlags, CDS_FULLSCREEN);
    ok_int(g_EnumCount, 1);
}

static void post_8bit_2(void)
{
    ok_int(g_ChangeCount, 0);
    ok_hex(g_LastFlags, 0xffffffff);
    ok_int(g_EnumCount, 1);

    bFix = TRUE;
}

static void post_8bit_no(void)
{
    if (g_Version == _WIN32_WINNT_WS03)
    {
        ok_int(g_ChangeCount, 1);
        ok_hex(g_LastDevmode.dmFields & DM_BITSPERPEL, DM_BITSPERPEL);
        ok_int(g_LastDevmode.dmBitsPerPel, 8);
        ok_hex(g_LastFlags, CDS_FULLSCREEN);
        ok_int(g_EnumCount, 1);
    }
    else
    {
        ok_int(g_ChangeCount, 0);
        ok_hex(g_LastFlags, 0xffffffff);
        ok_int(g_EnumCount, 0);
    }

    bFix = TRUE;
}

static void post_8bit_2_no(void)
{
    if (g_Version == _WIN32_WINNT_WS03)
    {
        ok_int(g_ChangeCount, 0);
        ok_hex(g_LastFlags, 0xffffffff);
        ok_int(g_EnumCount, 1);
    }
    else
    {
        ok_int(g_ChangeCount, 0);
        ok_hex(g_LastFlags, 0xffffffff);
        ok_int(g_EnumCount, 0);
    }

    bFix = TRUE;
}

static void pre_640(void)
{
    g_ChangeCount = 0;
    memset(&g_LastDevmode, 0, sizeof(g_LastDevmode));
    g_LastFlags = 0xffffffff;
    g_EnumCount = 0;
}

static void pre_640_2(void)
{
    bFix = FALSE;

    pre_640();
}

static void post_640(void)
{
    ok_int(g_ChangeCount, 1);
    ok_hex(g_LastDevmode.dmFields & (DM_PELSWIDTH | DM_PELSHEIGHT), (DM_PELSWIDTH | DM_PELSHEIGHT));
    ok_int(g_LastDevmode.dmPelsWidth, 640);
    ok_int(g_LastDevmode.dmPelsHeight, 480);
    ok_hex(g_LastFlags, CDS_FULLSCREEN);
    ok_int(g_EnumCount, 1);
}

static void post_640_2(void)
{
    ok_int(g_ChangeCount, 0);
    ok_hex(g_LastFlags, 0xffffffff);
    ok_int(g_EnumCount, 1);

    bFix = TRUE;
}

static void post_640_no(void)
{
    if (g_Version == _WIN32_WINNT_WS03)
    {
        ok_int(g_ChangeCount, 1);
        ok_hex(g_LastDevmode.dmFields & (DM_PELSWIDTH | DM_PELSHEIGHT), (DM_PELSWIDTH | DM_PELSHEIGHT));
        ok_int(g_LastDevmode.dmPelsWidth, 640);
        ok_int(g_LastDevmode.dmPelsHeight, 480);
        ok_hex(g_LastFlags, CDS_FULLSCREEN);
        ok_int(g_EnumCount, 1);
    }
    else
    {
        ok_int(g_ChangeCount, 0);
        ok_hex(g_LastFlags, 0xffffffff);
        ok_int(g_EnumCount, 0);
    }

    bFix = TRUE;
}

static void post_640_2_no(void)
{
    if (g_Version == _WIN32_WINNT_WS03)
    {
        ok_int(g_ChangeCount, 0);
        ok_hex(g_LastFlags, 0xffffffff);
        ok_int(g_EnumCount, 1);
    }
    else
    {
        ok_int(g_ChangeCount, 0);
        ok_hex(g_LastFlags, 0xffffffff);
        ok_int(g_EnumCount, 0);
    }

    bFix = TRUE;
}

static void pre_theme(void)
{
    g_ThemeCount = 0;
    g_LastThemeFlags = 0xffffffff;
}

static void post_theme(void)
{
    ok_int(g_ThemeCount, 1);
    ok_hex(g_LastThemeFlags, 0);
}

static void post_theme_no(void)
{
    if (g_Version == _WIN32_WINNT_WS03)
    {
        ok_int(g_ThemeCount, 1);
        ok_hex(g_LastThemeFlags, 0);
    }
    else
    {
        ok_int(g_ThemeCount, 0);
        ok_hex(g_LastThemeFlags, 0xffffffff);
    }
}


static BOOL hook_disp(HMODULE dll)
{
    return RedirectIat(dll, "user32.dll", "ChangeDisplaySettingsA", (ULONG_PTR)mChangeDisplaySettingsA, (ULONG_PTR*)&pChangeDisplaySettingsA) &&
        RedirectIat(dll, "user32.dll", "EnumDisplaySettingsA", (ULONG_PTR)mEnumDisplaySettingsA, (ULONG_PTR*)&pEnumDisplaySettingsA);
}

static VOID unhook_disp(HMODULE dll)
{
    RestoreIat(dll, "user32.dll", "ChangeDisplaySettingsA", (ULONG_PTR)pChangeDisplaySettingsA);
    RestoreIat(dll, "user32.dll", "EnumDisplaySettingsA", (ULONG_PTR)pEnumDisplaySettingsA);
}

static BOOL hook_theme(HMODULE dll)
{
    return RedirectIat(dll, "uxtheme.dll", "SetThemeAppProperties", (ULONG_PTR)mSetThemeAppProperties, (ULONG_PTR*)&pSetThemeAppProperties);
}

static VOID unhook_theme(HMODULE dll)
{
    RestoreIat(dll, "uxtheme.dll", "SetThemeAppProperties", (ULONG_PTR)pSetThemeAppProperties);
}

static void test_one(LPCSTR shim, DWORD dwReason, void(*pre)(), void(*post)(), void(*second)(void))
{
    DWORD num_shims = 0;
    WCHAR wide_shim[50] = { 0 };
    PVOID hook;
    BOOL ret;
    MultiByteToWideChar(CP_ACP, 0, shim, -1, wide_shim, 50);

    if (pre)
        pre();

    hook = pGetHookAPIs("", wide_shim, &num_shims);
    if (hook == NULL)
    {
        skip("Skipping tests for layers (%s) not present in this os (0x%x)\n", shim, g_Version);
        return;
    }
    ok(hook != NULL, "Expected hook to be a valid pointer for %s\n", shim);
    ok(num_shims == 0, "Expected not to find any apihooks, got: %u for %s\n", num_shims, shim);

    ret = pNotifyShims(dwReason, NULL);

    /* Win7 and Win10 return 1, w2k3 returns a pointer */
    ok(ret != 0, "Expected pNotifyShims to succeed (%i)\n", ret);

    if (post)
        post();

    /* Invoking it a second time does not call the init functions again! */
    if (pre && second)
    {
        pre();

        ret = pNotifyShims(dwReason, NULL);
        ok(ret != 0, "Expected pNotifyShims to succeed (%i)\n", ret);

        second();
    }
}

/* In 2k3 0, 2, 4, 6, 8 are not guarded against re-initializations! */
static struct test_info
{
    const char* name;
    const WCHAR* dll;
    DWORD winver;
    DWORD reason;
    BOOL(*hook)(HMODULE);
    void(*unhook)(HMODULE);
    void(*pre)(void);
    void(*post)(void);
    void(*second)(void);
} tests[] =
{
    /* Success */
    { "Force8BitColor", L"aclayers.dll", WINVER_ANY, 1, hook_disp, unhook_disp, pre_8bit, post_8bit, post_8bit_no },
    { "Force8BitColor", L"aclayers.dll", _WIN32_WINNT_VISTA, 100, hook_disp, unhook_disp,pre_8bit, post_8bit, post_8bit_no },
    { "Force640x480", L"aclayers.dll", WINVER_ANY, 1, hook_disp, unhook_disp, pre_640, post_640, post_640_no },
    { "Force640x480", L"aclayers.dll", _WIN32_WINNT_VISTA, 100, hook_disp, unhook_disp, pre_640, post_640, post_640_no },
    { "DisableThemes", L"acgenral.dll", WINVER_ANY, 1, hook_theme, unhook_theme, pre_theme, post_theme, post_theme_no },
    { "DisableThemes", L"acgenral.dll", _WIN32_WINNT_VISTA, 100, hook_theme, unhook_theme, pre_theme, post_theme, post_theme_no },

    /* No need to change anything */
    { "Force8BitColor", L"aclayers.dll", WINVER_ANY, 1, hook_disp, unhook_disp, pre_8bit_2, post_8bit_2, post_8bit_2_no },
    { "Force8BitColor", L"aclayers.dll", _WIN32_WINNT_VISTA, 100, hook_disp, unhook_disp, pre_8bit_2, post_8bit_2, post_8bit_2_no },
    { "Force640x480", L"aclayers.dll", WINVER_ANY, 1, hook_disp, unhook_disp, pre_640_2, post_640_2, post_640_2_no },
    { "Force640x480", L"aclayers.dll", _WIN32_WINNT_VISTA, 100, hook_disp, unhook_disp, pre_640_2, post_640_2, post_640_2_no },
};


static void run_test(size_t n, BOOL unload)
{
    BOOL ret;
    HMODULE dll;

    if (!LoadShimDLL(tests[n].dll, &dll, &pGetHookAPIs))
        pGetHookAPIs = NULL;
    pNotifyShims = (void*)GetProcAddress(dll, "NotifyShims");

    if (!pGetHookAPIs || !pNotifyShims)
    {
        skip("%s not loaded, or does not export GetHookAPIs or pNotifyShims (%s, %p, %p)\n",
             wine_dbgstr_w(tests[n].dll), tests[n].name, pGetHookAPIs, pNotifyShims);
        return;
    }

    g_Version = get_module_version(dll);

    if (!g_Version)
    {
        g_Version = _WIN32_WINNT_WS03;
        trace("Module %s has no version, faking 2k3\n", wine_dbgstr_w(tests[n].dll));
    }

    if (g_Version >= tests[n].winver)
    {
        ret = tests[n].hook(dll);
        if (ret)
        {
            test_one(tests[n].name, tests[n].reason, tests[n].pre, tests[n].post, tests[n].second);
            tests[n].unhook(dll);
        }
        else
        {
            ok(0, "Unable to redirect functions!\n");
        }
    }
    FreeLibrary(dll);
    if (unload)
    {
        dll = GetModuleHandleW(tests[n].dll);
        ok(dll == NULL, "Unable to unload %s\n", wine_dbgstr_w(tests[n].dll));
    }
}


START_TEST(dispmode)
{
    HMODULE dll = LoadLibraryA("apphelp.dll");
    size_t n;
    int argc;
    char **argv;

    argc = winetest_get_mainargs(&argv);
    if (argc < 3)
    {
        WCHAR path[MAX_PATH];
        GetModuleFileNameW(NULL, path, _countof(path));
        dll = GetModuleHandleW(L"aclayers.dll");
        if (!dll)
            dll = GetModuleHandleW(L"acgenral.dll");
        if (dll != NULL)
            trace("Loaded under a shim, running each test in it's own process\n");

        for (n = 0; n < _countof(tests); ++n)
        {
            LONG failures = winetest_get_failures();

            if (dll == NULL)
            {
                run_test(n, TRUE);
            }
            else
            {
                WCHAR buf[MAX_PATH+40];
                STARTUPINFOW si = { sizeof(si) };
                PROCESS_INFORMATION pi;
                BOOL created;

                StringCchPrintfW(buf, _countof(buf), L"\"%ls\" dispmode %u", path, n);
                created = CreateProcessW(NULL, buf, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
                ok(created, "Expected CreateProcess to succeed\n");
                if (created)
                {
                    winetest_wait_child_process(pi.hProcess);
                    CloseHandle(pi.hThread);
                    CloseHandle(pi.hProcess);
                }
            }

            ok(failures == winetest_get_failures(), "Last %u failures are from %d (%s)\n",
                winetest_get_failures() - failures, n, tests[n].name);
        }
    }
    else
    {
        n = (size_t)atoi(argv[2]);
        if (n < _countof(tests))
        {
            run_test(n, FALSE);
        }
        else
        {
            ok(0, "Test out of range: %u\n", n);
        }
    }
}
