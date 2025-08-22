/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Unit tests for the comctl32 tooltips
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "wine/test.h"

#include <windows.h>

/* FUNCTIONS ******************************************************************/

static
VOID
TestDllProductVersion(VOID)
{
    HANDLE hAppHeap = GetProcessHeap();
    DWORD dwInfoSize;
    LPVOID lpData;
    VS_FIXEDFILEINFO* pInfo;
    UINT uInfoLen;

    dwInfoSize = GetFileVersionInfoSizeW(L"comctl32.dll", NULL);
    if (dwInfoSize == 0)
    {
        skip("GetModuleFileNameW failed\n");
        return;
    }

    lpData = HeapAlloc(hAppHeap, 0, dwInfoSize);
    if (!lpData)
    {
        skip("No memory\n");
        return;
    }

    if (!GetFileVersionInfoW(L"comctl32.dll", 0, dwInfoSize, lpData))
    {
        skip("Unable to retrieve the file version information\n");
        goto Cleanup;
    }

    if (!VerQueryValueW(lpData, L"\\", (LPVOID *)&pInfo, &uInfoLen) || uInfoLen == 0)
    {
        skip("Unable to retrieve the root block\n");
        goto Cleanup;
    }

    /*
     * SIV 5.80 expects that the "product version" string of the comctl32.dll file
     * will have the "file version" format. This value is used to determine
     * whether tooltip support is available.
     */
    ok(pInfo->dwProductVersionMS >= MAKELONG(5, 0),
       "Unknown comctl32.dll version %lx\n", pInfo->dwProductVersionMS);

Cleanup:
    HeapFree(hAppHeap, 0, lpData);
}

START_TEST(tooltip)
{
    TestDllProductVersion();
}
