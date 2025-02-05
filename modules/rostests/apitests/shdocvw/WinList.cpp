/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tests for shdocvw!WinList_*
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>
#include <shlobj.h>

typedef BOOL (WINAPI *FN_WinList_Init)(VOID);
typedef VOID (WINAPI *FN_WinList_Terminate)(VOID);
typedef IShellWindows* (WINAPI *FN_WinList_GetShellWindows)(BOOL);

static FN_WinList_Init g_pWinList_Init = NULL;
static FN_WinList_Terminate g_pWinList_Terminate = NULL;
static FN_WinList_GetShellWindows g_pWinList_GetShellWindows = NULL;

static VOID
TEST_WinList_GetShellWindows(VOID)
{
    BOOL bInited = g_pWinList_Init && g_pWinList_Init();
    ok_int(bInited, FALSE); // WinList_Init should fail because this process is not explorer.exe

    IShellWindows *pShellWindows1 = g_pWinList_GetShellWindows(FALSE);
    trace("%p\n", pShellWindows1);
    ok(pShellWindows1 != NULL, "pShellWindows1 was null\n");

    IShellWindows *pShellWindows2 = g_pWinList_GetShellWindows(FALSE);
    trace("%p\n", pShellWindows2);
    ok(pShellWindows2 != NULL, "pShellWindows2 was null\n");

    IShellWindows *pShellWindows3 = g_pWinList_GetShellWindows(TRUE);
    trace("%p\n", pShellWindows3);
    ok(pShellWindows3 != NULL, "pShellWindows3 was null\n");

    ok_ptr(pShellWindows1, pShellWindows2);
    ok_ptr(pShellWindows2, pShellWindows3);

    if (pShellWindows1)
    {
        LONG nCount = -1;
        HRESULT hr = pShellWindows1->get_Count(&nCount);
        ok_long(hr, S_OK);
        ok(nCount >= 0, "nCount was %ld\n", nCount);

        pShellWindows1->Release();
    }
    else
    {
        ok_int(TRUE, FALSE);
        ok_int(TRUE, FALSE);
    }

    if (pShellWindows1 != pShellWindows2 && pShellWindows2)
        pShellWindows2->Release();

    if (pShellWindows1 != pShellWindows2 && pShellWindows2 != pShellWindows3 && pShellWindows3)
        pShellWindows3->Release();

    if (bInited && g_pWinList_Terminate)
        g_pWinList_Terminate();
}

START_TEST(WinList)
{
    HRESULT hrCoInit = CoInitialize(NULL);

    HINSTANCE hSHDOCVW = LoadLibraryW(L"shdocvw.dll");
    if (!hSHDOCVW)
    {
        skip("shdocvw.dll not loaded\n");
    }
    else
    {
        g_pWinList_Init = (FN_WinList_Init)GetProcAddress(hSHDOCVW, MAKEINTRESOURCEA(110));
        g_pWinList_Terminate = (FN_WinList_Terminate)GetProcAddress(hSHDOCVW, MAKEINTRESOURCEA(111));
        g_pWinList_GetShellWindows = (FN_WinList_GetShellWindows)GetProcAddress(hSHDOCVW, MAKEINTRESOURCEA(179));
        if (!g_pWinList_Init || !g_pWinList_Terminate || !g_pWinList_GetShellWindows)
        {
            skip("Some WinList_* functions not found: %p %p %p\n",
                 g_pWinList_Init, g_pWinList_Terminate, g_pWinList_GetShellWindows);
        }
        else
        {
            TEST_WinList_GetShellWindows();
        }
    }

    if (SUCCEEDED(hrCoInit))
        CoUninitialize();
}
