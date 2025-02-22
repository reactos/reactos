/*
 * PROJECT:     ReactOS Standard Print Processor API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Main functions
 * COPYRIGHT:   Copyright 2016 Colin Finck (colin@reactos.org)
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winspool.h>

PVOID
GetWinprintFunc(const char* FunctionName)
{
    DWORD cbNeeded;
    HMODULE hWinprint;
    PVOID pFunc;
    WCHAR wszWinprintPath[MAX_PATH];

    // Build the path to the default Print Processor winprint.dll in the Print Processor directory.
    if (!GetPrintProcessorDirectoryW(NULL, NULL, 1, (LPBYTE)wszWinprintPath, sizeof(wszWinprintPath), &cbNeeded))
    {
        skip("Could not determine the path to the Print Processor directory, last error is %lu!\n", GetLastError());
        return NULL;
    }

    wcscat(wszWinprintPath, L"\\winprint.dll");

    // Try loading it.
    hWinprint = LoadLibraryW(wszWinprintPath);
    if (!hWinprint)
    {
        if (GetLastError() != ERROR_MOD_NOT_FOUND)
        {
            skip("LoadLibraryW failed for %S with error %lu!\n", wszWinprintPath, GetLastError());
            return NULL;
        }

        // winprint.dll does not exist prior to NT6.
        // The default Print Processor is implemented in localspl.dll instead.
        hWinprint = LoadLibraryW(L"localspl.dll");
        if (!hWinprint)
        {
            skip("LoadLibraryW failed for localspl.dll with error %lu!\n", GetLastError());
            return NULL;
        }
    }

    // Get the function we are looking for.
    pFunc = GetProcAddress(hWinprint, FunctionName);
    if (!pFunc)
    {
        skip("GetProcAddress failed for %s with error %lu!\n", FunctionName, GetLastError());
        return NULL;
    }

    return pFunc;
}
