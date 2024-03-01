/*
 * shpolicy.c - Data for shell/system policies.
 *
 * Copyright 1999 Ian Schmidt <ischmidt@cfl.rr.com>
 * Copyright 2022 Hermes Belusca-Maito
 * Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTES:
 *
 * Some of these policies can be tweaked via the System Policy
 * Editor which came with the Win95 Migration Guide, although
 * there doesn't appear to be an updated Win98 version that
 * would handle the many new policies introduced since then.
 * You could easily write one with the information in
 * this file...
 *
 * Up to date as of SHELL32 v6.00 (Win2k3)
 * References:
 * https://www.geoffchappell.com/studies/windows/shell/shell32/api/util/restrictions.htm
 * https://docs.microsoft.com/en-us/windows/win32/api/shlobj_core/ne-shlobj_core-restrictions
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS

#include <windef.h>
#include <winbase.h>
#include <shlobj.h>
#include <initguid.h>
#include <shlwapi_undoc.h>
#include <wine/debug.h>

#include "shell32_main.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

DEFINE_GUID(GUID_Restrictions, 0xA48F1A32, 0xA340, 0x11D1, 0xBC, 0x6B, 0x00, 0xA0, 0xC9, 0x03, 0x12, 0xE1);

#define DEFINE_POLICY(policy, appstr, keystr) \
    { policy, L##appstr, L##keystr }

static const POLICYDATA s_PolicyTable[] =
{
#include "PolicyData.h"
    { 0, NULL, NULL }
};

#undef DEFINE_POLICY

/*
 * The restriction-related variables
 */
HANDLE g_hRestGlobalCounter = NULL;
LONG g_nRestCountValue = -1;
DWORD g_RestValues[_countof(s_PolicyTable)] = { 0 };

static HANDLE
SHELL_GetCachedGlobalCounter(HANDLE *phGlobalCounter, REFGUID rguid)
{
    HANDLE hGlobalCounter;
    if (*phGlobalCounter)
        return *phGlobalCounter;
    hGlobalCounter = SHGlobalCounterCreate(rguid);
    if (SHInterlockedCompareExchange(phGlobalCounter, hGlobalCounter, NULL))
        CloseHandle(hGlobalCounter);
    return *phGlobalCounter;
}

static HANDLE SHELL_GetRestrictionsCounter(VOID)
{
    return SHELL_GetCachedGlobalCounter(&g_hRestGlobalCounter, &GUID_Restrictions);
}

static BOOL SHELL_QueryRestrictionsChanged(VOID)
{
    LONG Value = SHGlobalCounterGetValue(SHELL_GetRestrictionsCounter());
    if (g_nRestCountValue == Value)
        return FALSE;

    g_nRestCountValue = Value;
    return TRUE;
}

/*************************************************************************
 * SHRestricted				 [SHELL32.100]
 *
 * Get the value associated with a policy Id.
 *
 * PARAMS
 *     pol [I] Policy Id
 *
 * RETURNS
 *     The queried value for the policy.
 *
 * NOTES
 *     Exported by ordinal.
 *     This function caches the retrieved values to prevent unnecessary registry access,
 *     if SHSettingsChanged() was previously called.
 *
 * REFERENCES
 *     a: MS System Policy Editor.
 *     b: 98Lite 2.0 (which uses many of these policy keys) http://www.98lite.net/
 *     c: 'The Windows 95 Registry', by John Woram, 1996 MIS: Press
 */
DWORD WINAPI SHRestricted (RESTRICTIONS rest)
{
    TRACE("(0x%08lX)\n", rest);

    if (SHELL_QueryRestrictionsChanged())
        FillMemory(&g_RestValues, sizeof(g_RestValues), 0xFF);

    return SHRestrictionLookup(rest, NULL, s_PolicyTable, g_RestValues);
}

/*************************************************************************
 * SHSettingsChanged          [SHELL32.244]
 *
 * Initialise the policy cache to speed up calls to SHRestricted().
 *
 * PARAMS
 *  unused    [I] Reserved.
 *  pszKey    [I] Registry key to scan.
 *
 * RETURNS
 *  Success: -1. The policy cache is initialised.
 *  Failure: 0, if inpRegKey is any value other than NULL, "Policy", or
 *           "Software\Microsoft\Windows\CurrentVersion\Policies".
 *
 * NOTES
 *  Exported by ordinal. Introduced in Win98.
 */
BOOL WINAPI SHSettingsChanged(LPCVOID unused, LPCWSTR pszKey)
{
    TRACE("(%p, %s)\n", unused, debugstr_w(pszKey));

    if (pszKey &&
        lstrcmpiW(L"Policy", pszKey) != 0 &&
        lstrcmpiW(L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies", pszKey) != 0)
    {
        return FALSE;
    }

    return SHGlobalCounterIncrement(SHELL_GetRestrictionsCounter());
}
