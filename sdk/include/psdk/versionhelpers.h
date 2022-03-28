/*
 * versionhelpers.h
 *
 * Inline helper functions for Windows version detection
 *
 * This file is part of the ReactOS PSDK package.
 *
 * Contributors:
 *   Created by Timo Kreuzer <timo.kreuzer@reactos.org>
 *   Modified by Raymond Czerny <chip@raymisoft.de>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#pragma once
#define _versionhelpers_H_INCLUDED_

#include <specstrings.h>

#ifdef __cplusplus
#define VERSIONHELPERAPI inline bool
#else
#define VERSIONHELPERAPI FORCEINLINE BOOL
#endif

VERSIONHELPERAPI
IsWindowsVersionOrGreater(WORD wMajorVersion, WORD wMinorVersion, WORD wServicePackMajor)
{
    OSVERSIONINFOEXW osvi = { sizeof(osvi), wMajorVersion, wMinorVersion, 0, 0, {0}, wServicePackMajor, 0, 0, 0, 0 };
    DWORDLONG dwlConditionMask = VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL);
    dwlConditionMask = VerSetConditionMask(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
    dwlConditionMask = VerSetConditionMask(dwlConditionMask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);

    return VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR, dwlConditionMask) != FALSE;
}

VERSIONHELPERAPI
IsWindowsXPOrGreater()
{
    return IsWindowsVersionOrGreater(5, 1, 0);
}

VERSIONHELPERAPI
IsWindowsXPSP1OrGreater()
{
    return IsWindowsVersionOrGreater(5, 1, 1);
}

VERSIONHELPERAPI
IsWindowsXPSP2OrGreater()
{
    return IsWindowsVersionOrGreater(5, 1, 2);
}

VERSIONHELPERAPI
IsWindowsXPSP3OrGreater()
{
    return IsWindowsVersionOrGreater(5, 1, 3);
}

VERSIONHELPERAPI
IsWindowsVistaOrGreater()
{
    return IsWindowsVersionOrGreater(6, 0, 0);
}

VERSIONHELPERAPI
IsWindowsVistaSP1OrGreater()
{
    return IsWindowsVersionOrGreater(6, 0, 1);
}

VERSIONHELPERAPI
IsWindowsVistaSP2OrGreater()
{
    return IsWindowsVersionOrGreater(6, 0, 2);
}

VERSIONHELPERAPI
IsWindows7OrGreater()
{
    return IsWindowsVersionOrGreater(6, 1, 0);
}

VERSIONHELPERAPI
IsWindows7SP1OrGreater()
{
    return IsWindowsVersionOrGreater(6, 1, 1);
}

VERSIONHELPERAPI
IsWindows8OrGreater()
{
    return IsWindowsVersionOrGreater(6, 2, 0);
}

VERSIONHELPERAPI
IsWindows8Point1OrGreater()
{
    return IsWindowsVersionOrGreater(6, 3, 0);
}

VERSIONHELPERAPI
IsWindowsThresholdOrGreater()
{
    return IsWindowsVersionOrGreater(10, 0, 0);
}

VERSIONHELPERAPI
IsWindows10OrGreater()
{
    return IsWindowsVersionOrGreater(10, 0, 0);
}

VERSIONHELPERAPI
IsWindowsServer()
{
    OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0, {0}, 0, 0, 0, VER_NT_WORKSTATION };
    DWORDLONG const dwlConditionMask = VerSetConditionMask(0, VER_PRODUCT_TYPE, VER_EQUAL);
    return VerifyVersionInfoW(&osvi, VER_PRODUCT_TYPE, dwlConditionMask) == FALSE;
}

VERSIONHELPERAPI
IsActiveSessionCountLimited()
{
    OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0, {0}, 0, 0, 0, 0, 0 };
    DWORDLONG dwlConditionMask = VerSetConditionMask(0, VER_SUITENAME, VER_AND);
    BOOL fSuiteTerminal, fSuiteSingleUserTS;

    osvi.wSuiteMask = VER_SUITE_TERMINAL;
    fSuiteTerminal = VerifyVersionInfoW(&osvi, VER_SUITENAME, dwlConditionMask);

    osvi.wSuiteMask = VER_SUITE_SINGLEUSERTS;
    fSuiteSingleUserTS = VerifyVersionInfoW(&osvi, VER_SUITENAME, dwlConditionMask);

    return !(fSuiteTerminal & !fSuiteSingleUserTS);
}

#ifdef __REACTOS__
VERSIONHELPERAPI
IsReactOS()
{
    BOOL bResult = FALSE;
    HMODULE hMod;

    hMod = GetModuleHandleW(L"ntdll");

    if (hMod)   // should always be loaded
    {
        DWORD dwLen;
        WCHAR szFullPath[MAX_PATH];

        dwLen = GetModuleFileNameW(hMod, szFullPath, MAX_PATH);
        if (dwLen)
        {
            DWORD dwSize;

            szFullPath[dwLen] = 0; // required by Windows XP
            dwSize = GetFileVersionInfoSizeW(szFullPath, NULL);
            if (dwSize)
            {
                BOOL bProduct;
                UINT cchVer = 0;
                LPCWSTR lszValue = NULL;
                LPVOID VerInfo = NULL;

                VerInfo  = HeapAlloc(GetProcessHeap(), 0, dwSize);
                GetFileVersionInfoW(szFullPath, 0L, dwSize, VerInfo);
                bProduct = VerQueryValueW(VerInfo,
                                          L"\\StringFileInfo\\040904B0\\ProductName",
                                          (LPVOID)&lszValue,
                                          &cchVer);
                if (bProduct)
                {
                    bResult = (wcsstr(lszValue, L"ReactOS") != NULL);
                }
                HeapFree(GetProcessHeap(), 0, VerInfo);
            }
        }
    }

    return bResult;
}
#endif // __REACTOS__
