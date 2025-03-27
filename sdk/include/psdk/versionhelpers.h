/*
 * versionhelpers.h
 *
 * Inline helper functions for Windows version detection
 *
 * This file is part of the ReactOS PSDK package.
 *
 * Contributors:
 *   Created by Timo Kreuzer <timo.kreuzer@reactos.org>
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
#include <mmtypes.h>
VERSIONHELPERAPI
IsReactOS()
{
    return *(UINT*)(MM_SHARED_USER_DATA_VA + PAGE_SIZE - sizeof(ULONG)) == 0x8EAC705;
}
#endif // __REACTOS__
