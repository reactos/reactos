/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for ExtractIconEx routine
 * COPYRIGHT:   Copyright 2019 George Bișoc (george.bisoc@reactos.org)
 *              Copyright 2023 Doug Lyons (douglyons@douglyons.com)
 */

#include "shelltest.h"
#include <stdio.h>

EXTERN_C BOOL WINAPI SHAreIconsEqual(HICON hIcon1, HICON hIcon2);

static void SafeDestroyIcon(HICON hIco)
{
    if (hIco)
        DestroyIcon(hIco);
}

static UINT GetIcoSize(HICON hIco)
{
    ICONINFO info;
    if (!GetIconInfo(hIco, &info))
        return 0;

    BITMAP bm;
    if (!GetObject(info.hbmColor ? info.hbmColor : info.hbmMask, sizeof(bm), &bm))
        bm.bmWidth = 0;
    DeleteObject(info.hbmMask);
    DeleteObject(info.hbmColor);
    return bm.bmWidth;
}

typedef struct
{
    PCWSTR pszFilePath;
    UINT nIcons;
} EXTRACTICONTEST;

BOOL FileExists(LPCSTR FileName)
{
    FILE *fp = NULL;
    bool exists = FALSE;

    fp = fopen(FileName, "r");
    if (fp != NULL)
    {
        exists = TRUE;
        fclose(fp);
    }
    return exists;
}

BOOL ResourceToFile(INT i, LPCSTR FileName)
{
    FILE *fout;
    HGLOBAL hData;
    HRSRC hRes;
    LPVOID lpResLock;
    UINT iSize;

    if (FileExists(FileName))
    {
        skip("'%s' already exists. Exiting now\n", FileName);
        return FALSE;
    }

    hRes = FindResourceW(NULL, MAKEINTRESOURCEW(i), MAKEINTRESOURCEW(RT_RCDATA));
    if (hRes == NULL)
    {
        skip("Could not locate resource (%d). Exiting now\n", i);
        return FALSE;
    }

    iSize = SizeofResource(NULL, hRes);

    hData = LoadResource(NULL, hRes);
    if (hData == NULL)
    {
        skip("Could not load resource (%d). Exiting now\n", i);
        return FALSE;
    }

    // Lock the resource into global memory.
    lpResLock = LockResource(hData);
    if (lpResLock == NULL)
    {
        skip("Could not lock resource (%d). Exiting now\n", i);
        return FALSE;
    }

    fout = fopen(FileName, "wb");
    fwrite(lpResLock, iSize, 1, fout);
    fclose(fout);
    return TRUE;
}

EXTRACTICONTEST IconTests[] =
{
    /* Executable file with icon */
    {L"%SystemRoot%\\System32\\cmd.exe", 1},

    /* Executable file without icon */
    {L"%SystemRoot%\\System32\\autochk.exe", 0},

    /* Non-existing files */
    {L"%SystemRoot%\\non-existent-file.sdf", 0},

    /* Multiple icons in the same ICO file (6 icons)
     * Per MS: If the file is an .ico file, the return value is 1. */
    {L"sysicon.ico", 1},

    /* ICO file with both normal and PNG icons */
    {L"ROS.ico", (UINT)(GetNTVersion() >= _WIN32_WINNT_VISTA ? 1 : 0)}
};

VOID RunExtractIconTest(EXTRACTICONTEST *Test)
{
    UINT nReturnedIcons, nExtractedIcons;

    /* Check count of icons returned */
    nReturnedIcons = ExtractIconExW(Test->pszFilePath, -1, NULL, NULL, 0);
    ok(nReturnedIcons == Test->nIcons, "ExtractIconExW(L\"%S\"): Expects %u icons, got %u\n", Test->pszFilePath, Test->nIcons, nReturnedIcons);

    /* Check if the 0th icon can be extracted successfully */
    nExtractedIcons = ExtractIconExW(Test->pszFilePath, 0, NULL, NULL, 1);
    ok(nExtractedIcons == Test->nIcons, "ExtractIconExW(L\"%S\"): Expects %u icons, got %u\n", Test->pszFilePath, Test->nIcons, nExtractedIcons);
}

START_TEST(ExtractIconEx)
{
    UINT i;
    CHAR FileName[2][13] = { "ROS.ico", "sysicon.ico" };
    EXTRACTICONTEST explorer_exe = {L"%SystemRoot%\\explorer.exe", 0};

    if (!ResourceToFile(2, FileName[0]))
        return;
    if (!ResourceToFile(3, FileName[1]))
        return;

    /* Run normal tests */
    for (i = 0; i < _countof(IconTests); ++i)
        RunExtractIconTest(&IconTests[i]);

    /* Run special case checks */
    switch (GetNTVersion())
    {
        case _WIN32_WINNT_WS03:
            explorer_exe.nIcons = 18;
            break;
        case _WIN32_WINNT_VISTA:
            explorer_exe.nIcons = 23;
            break;
        case _WIN32_WINNT_WIN7:
            explorer_exe.nIcons = 25;
            break;
        case _WIN32_WINNT_WIN8:
        case _WIN32_WINNT_WINBLUE:
            explorer_exe.nIcons = 24;
            break;
        case _WIN32_WINNT_WIN10:
            explorer_exe.nIcons = 28;
            break;
    }

    if (explorer_exe.nIcons)
        RunExtractIconTest(&explorer_exe);
    else
        skip("Unknown NT Version: 0x%lX\n", GetNTVersion());

    DeleteFileA(FileName[0]);
    DeleteFileA(FileName[1]);
}

static HRESULT SHDEI(LPCWSTR pszIconFile, int Index = 0, UINT GIL = 0, UINT Size = 0)
{
    HICON hIco = NULL;
    HRESULT hr = SHDefExtractIcon(pszIconFile, Index, GIL, &hIco, NULL, Size);
    if (hr == S_OK)
    {
        hr = GetIcoSize(hIco);
        SafeDestroyIcon(hIco);
    }
    return hr;
}

START_TEST(SHDefExtractIcon)
{
    HRESULT hr;
    int SysBigIconSize = GetSystemMetrics(SM_CXICON);

    // Modern Windows requires the system image list to be initialized for GIL_SIMULATEDOC to work!
    SHFILEINFOW shfi;
    SHGetFileInfoW(L"x", 0, &shfi, sizeof(shfi), SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES);

    WCHAR path[MAX_PATH];
    GetSystemDirectoryW(path, _countof(path));
    PathAppendW(path, L"user32.dll");
    int index = 1;

    ok(SHDEI(path, index, 0, 0) == SysBigIconSize, "0 size must match GetSystemMetrics\n");
    ok(SHDEI(path, index, 0, SysBigIconSize * 2) == SysBigIconSize * 2, "Resize failed\n");

    HICON hIcoLarge, hIcoSmall;
    if (SHDefExtractIcon(path, index, 0, &hIcoLarge, &hIcoSmall, 0) != S_OK)
        hIcoLarge = hIcoSmall = NULL;
    ok(hIcoLarge && hIcoSmall && !SHAreIconsEqual(hIcoLarge, hIcoSmall), "Large+Small failed\n");
    SafeDestroyIcon(hIcoLarge);
    SafeDestroyIcon(hIcoSmall);

    static const int sizes[] = { 0, SysBigIconSize * 2 };
    for (UINT i = 0; i < _countof(sizes); ++i)
    {
        HICON hIcoNormal, hIcoSimDoc;
        if (FAILED(hr = SHDefExtractIcon(path, index, 0, &hIcoNormal, NULL, sizes[i])))
            hIcoNormal = NULL;
        if (FAILED(hr = SHDefExtractIcon(path, index, GIL_SIMULATEDOC, &hIcoSimDoc, NULL, sizes[i])))
            hIcoSimDoc = NULL;
        ok(hIcoNormal && hIcoSimDoc && !SHAreIconsEqual(hIcoNormal, hIcoSimDoc), "GIL_SIMULATEDOC failed\n");
        SafeDestroyIcon(hIcoNormal);
        SafeDestroyIcon(hIcoSimDoc);
    }

    GetTempPathW(_countof(path), path);
    GetTempFileNameW(path, L"TEST", 0, path);
    ok(SHDEI(path) == S_FALSE, "Empty file should return S_FALSE\n");
    HANDLE hFile = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD io;
        WriteFile(hFile, "!", 1, &io, NULL);
        CloseHandle(hFile);
        ok(SHDEI(path) == S_FALSE, "File without icons should return S_FALSE\n");
    }
    DeleteFile(path);
}
