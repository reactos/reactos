/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for RealShellExecuteExA/W
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"
#include "closewnd.h"
#include <versionhelpers.h>

BOOL IsWindowsServer2003SP2OrGreater(void)
{
    return IsWindowsVersionOrGreater(5, 2, 2);
}

typedef HINSTANCE (WINAPI *FN_RealShellExecuteExA)(
    _In_opt_ HWND hwnd,
    _In_opt_ LPCSTR lpOperation,
    _In_opt_ LPCSTR lpFile,
    _In_opt_ LPCSTR lpParameters,
    _In_opt_ LPCSTR lpDirectory,
    _In_opt_ LPSTR lpReturn,
    _In_opt_ LPCSTR lpTitle,
    _In_opt_ LPVOID lpReserved,
    _In_ INT nCmdShow,
    _Out_opt_ PHANDLE lphProcess,
    _In_ DWORD dwFlags);

typedef HINSTANCE (WINAPI *FN_RealShellExecuteExW)(
    _In_opt_ HWND hwnd,
    _In_opt_ LPCWSTR lpOperation,
    _In_opt_ LPCWSTR lpFile,
    _In_opt_ LPCWSTR lpParameters,
    _In_opt_ LPCWSTR lpDirectory,
    _In_opt_ LPWSTR lpReturn,
    _In_opt_ LPCWSTR lpTitle,
    _In_opt_ LPVOID lpReserved,
    _In_ INT nCmdShow,
    _Out_opt_ PHANDLE lphProcess,
    _In_ DWORD dwFlags);

static HINSTANCE s_hSHELL32 = NULL;
static FN_RealShellExecuteExA s_fnRealShellExecuteExA = NULL;
static FN_RealShellExecuteExW s_fnRealShellExecuteExW = NULL;

static WINDOW_LIST s_List1, s_List2;

static void TEST_Start(void)
{
    GetWindowList(&s_List1);
}

static void TEST_End(void)
{
    Sleep(500);
    GetWindowList(&s_List2);
    CloseNewWindows(&s_List1, &s_List2);
    FreeWindowList(&s_List1);
    FreeWindowList(&s_List2);
}

static void TEST_RealShellExecuteExA(void)
{
    TEST_Start();

    INT_PTR ret;

    ret = (INT_PTR)s_fnRealShellExecuteExA(
        NULL,
        NULL,
        "notepad.exe",
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        SW_SHOWDEFAULT,
        NULL,
        0);
    if (IsWindowsServer2003SP2OrGreater())
        ok_long((LONG)ret, 42);
    else
        ok_long((LONG)ret, 2);

    TEST_End();
}

static void TEST_RealShellExecuteExW(void)
{
    TEST_Start();

    INT_PTR ret;

    ret = (INT_PTR)s_fnRealShellExecuteExW(
        NULL,
        NULL,
        L"notepad.exe",
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        SW_SHOWDEFAULT,
        NULL,
        0);
    ok_long((LONG)ret, 42);

    TEST_End();
}

START_TEST(RealShellExecuteEx)
{
    if (IsWindowsVistaOrGreater())
    {
        skip("Vista+\n");
        return;
    }

    s_hSHELL32 = LoadLibraryW(L"shell32.dll");

    s_fnRealShellExecuteExA = (FN_RealShellExecuteExA)GetProcAddress(s_hSHELL32, MAKEINTRESOURCEA(266));
    s_fnRealShellExecuteExW = (FN_RealShellExecuteExW)GetProcAddress(s_hSHELL32, MAKEINTRESOURCEA(267));

    if (!s_fnRealShellExecuteExA || !s_fnRealShellExecuteExW)
    {
        skip("RealShellExecuteExA/W not found: %p, %p\n",
             s_fnRealShellExecuteExA, s_fnRealShellExecuteExW);
        return;
    }

    TEST_RealShellExecuteExA();
    TEST_RealShellExecuteExW();

    FreeLibrary(s_hSHELL32);
}
