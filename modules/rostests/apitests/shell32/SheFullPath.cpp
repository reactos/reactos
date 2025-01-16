/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for SheFullPath
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"

typedef DWORD (WINAPI *FN_SheFullPathA)(PCSTR, DWORD, PSTR);
typedef DWORD (WINAPI *FN_SheFullPathW)(PCWSTR, DWORD, PWSTR);

#define ok_stri(x, y) \
    ok(_stricmp(x, y) == 0, "Wrong string. Expected '%s', got '%s'\n", y, x)
#define ok_wstri(x, y) \
    ok(_wcsicmp(x, y) == 0, "Wrong string. Expected '%S', got '%S'\n", y, x)

START_TEST(SheFullPath)
{
    CHAR szPathA[MAX_PATH], szWinDirA[MAX_PATH], szSysDirA[MAX_PATH];
    WCHAR szPathW[MAX_PATH], szWinDirW[MAX_PATH], szSysDirW[MAX_PATH];
    WCHAR szCurDirW[MAX_PATH];

    GetCurrentDirectoryW(_countof(szCurDirW), szCurDirW);
    GetWindowsDirectoryA(szWinDirA, _countof(szWinDirA));
    GetWindowsDirectoryW(szWinDirW, _countof(szWinDirW));
    GetSystemDirectoryA(szSysDirA, _countof(szSysDirA));
    GetSystemDirectoryW(szSysDirW, _countof(szSysDirW));

    FN_SheFullPathA SheFullPathA = (FN_SheFullPathA)GetProcAddress(GetModuleHandleW(L"shell32"), MAKEINTRESOURCEA(343));
    FN_SheFullPathW SheFullPathW = (FN_SheFullPathW)GetProcAddress(GetModuleHandleW(L"shell32"), MAKEINTRESOURCEA(344));
    if (!SheFullPathA)
    {
        skip("SheFullPathA not found\n");
        return;
    }
    if (!SheFullPathW)
    {
        skip("SheFullPathW not found\n");
        return;
    }

    SetCurrentDirectoryW(szWinDirW);

    SheFullPathA("", _countof(szPathA), szPathA);
    PathAddBackslashA(szWinDirA);
    ok_stri(szPathA, szWinDirA);

    SheFullPathW(L"", _countof(szPathW), szPathW);
    PathAddBackslashW(szWinDirW);
    ok_wstri(szPathW, szWinDirW);

    SheFullPathA(szSysDirA, _countof(szPathA), szPathA);
    ok_stri(szPathA, szSysDirA);

    SheFullPathW(szSysDirW, _countof(szPathW), szPathW);
    ok_wstri(szPathW, szSysDirW);

    SetCurrentDirectoryW(szCurDirW);
}
