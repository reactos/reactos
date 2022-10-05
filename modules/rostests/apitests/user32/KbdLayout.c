/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for Keyboard Layouts DLL files
 * COPYRIGHT:   Copyright 2022 Stanislav Motylkov <x86corez@gmail.com>
 */

#include "precomp.h"
#include <ndk/kbd.h>
#include <strsafe.h>

typedef PVOID (*PFN_KBDLAYERDESCRIPTOR)(VOID);

static void testLayout(
    _In_ LPWSTR szFileName,
    _In_ LPWSTR szFilePath)
{
    HMODULE hModule;
    PFN_KBDLAYERDESCRIPTOR pfnKbdLayerDescriptor;
    PKBDTABLES pKbdTbl;
    USHORT i, uTableSize;

    trace("Testing '%ls'...\n", szFileName);

    hModule = LoadLibraryW(szFilePath);
    if (!hModule)
    {
        ok(FALSE, "LoadLibraryW failed with code %ld\n", GetLastError());
        return;
    }

    pfnKbdLayerDescriptor = (PFN_KBDLAYERDESCRIPTOR)GetProcAddress(hModule, "KbdLayerDescriptor");
    if (!pfnKbdLayerDescriptor)
    {
        ok(FALSE, "KbdLayerDescriptor not found!\n");
        goto Cleanup;
    }

    pKbdTbl = pfnKbdLayerDescriptor();
    if (!pKbdTbl)
    {
        ok(FALSE, "PKBDTABLES is NULL!\n");
        goto Cleanup;
    }

    if (!pKbdTbl->pusVSCtoVK)
    {
        ok(FALSE, "pusVSCtoVK table is NULL!\n");
        goto Cleanup;
    }

    if (wcscmp(szFileName, L"kbdnec.dll") == 0)
        uTableSize = 128; /* Only NEC PC-9800 Japanese keyboard layout has 128 entries. */
    else
        uTableSize = 127;

    /* Validate number of entries in pusVSCtoVK array. */
    ok(pKbdTbl->bMaxVSCtoVK == uTableSize, "pKbdTbl->bMaxVSCtoVK = %u\n", pKbdTbl->bMaxVSCtoVK);

    for (i = 0; i < pKbdTbl->bMaxVSCtoVK; ++i)
    {
        /* Make sure there are no Virtual Keys with zero value. */
        if (pKbdTbl->pusVSCtoVK[i] == 0)
            ok(FALSE, "Scan Code %u => Virtual Key %u\n", i, pKbdTbl->pusVSCtoVK[i]);
    }

Cleanup:
    if (hModule)
        FreeLibrary(hModule);
}

static void testKeyboardLayouts(void)
{
    DWORD dwRet;
    WCHAR szSysPath[MAX_PATH],
          szPattern[MAX_PATH],
          szFilePath[MAX_PATH];
    HANDLE hFindFile = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW wfd;
    BOOL bFound = TRUE;

    dwRet = GetSystemDirectoryW(szSysPath, ARRAYSIZE(szSysPath));
    if (!dwRet)
    {
        skip("GetSystemDirectoryW failed with code %ld\n", GetLastError());
        return;
    }

    StringCchCopyW(szPattern, ARRAYSIZE(szPattern), szSysPath);
    StringCchCatW(szPattern, ARRAYSIZE(szPattern), L"\\kbd*.dll");

    for (hFindFile = FindFirstFileW(szPattern, &wfd);
         bFound && (hFindFile != INVALID_HANDLE_VALUE);
         bFound = FindNextFileW(hFindFile, &wfd))
    {
        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        StringCchCopyW(szFilePath, ARRAYSIZE(szFilePath), szSysPath);
        StringCchCatW(szFilePath, ARRAYSIZE(szFilePath), L"\\");
        StringCchCatW(szFilePath, ARRAYSIZE(szFilePath), wfd.cFileName);

        testLayout(wfd.cFileName, szFilePath);
    }

    if (hFindFile != INVALID_HANDLE_VALUE)
        FindClose(hFindFile);
}

START_TEST(KbdLayout)
{
    testKeyboardLayouts();
}
