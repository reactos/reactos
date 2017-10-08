/*
 * PROJECT:     ReactOS Service Control Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/services/controlset.c
 * PURPOSE:     Control Set Management
 * COPYRIGHT:   Copyright 2012 Eric Kohl
 *
 */

/* INCLUDES *****************************************************************/

#include "services.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS *******************************************************************/

static DWORD dwCurrentControlSet;
static DWORD dwDefaultControlSet;
static DWORD dwFailedControlSet;
static DWORD dwLastKnownGoodControlSet;


/* FUNCTIONS *****************************************************************/

BOOL
ScmGetControlSetValues(VOID)
{
    HKEY hSelectKey;
    DWORD dwType;
    DWORD dwSize;
    LONG lError;

    DPRINT("ScmGetControlSetValues() called\n");

    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"System\\Select",
                           0,
                           KEY_READ,
                           &hSelectKey);
    if (lError != ERROR_SUCCESS)
        return FALSE;

    dwSize = sizeof(DWORD);
    lError = RegQueryValueExW(hSelectKey,
                              L"Current",
                              0,
                              &dwType,
                              (LPBYTE)&dwCurrentControlSet,
                              &dwSize);
    if (lError != ERROR_SUCCESS)
    {
        dwCurrentControlSet = 0;
    }

    dwSize = sizeof(DWORD);
    lError = RegQueryValueExW(hSelectKey,
                              L"Default",
                              0,
                              &dwType,
                              (LPBYTE)&dwDefaultControlSet,
                              &dwSize);
    if (lError != ERROR_SUCCESS)
    {
        dwDefaultControlSet = 0;
    }

    dwSize = sizeof(DWORD);
    lError = RegQueryValueExW(hSelectKey,
                              L"Failed",
                              0,
                              &dwType,
                              (LPBYTE)&dwFailedControlSet,
                              &dwSize);
    if (lError != ERROR_SUCCESS)
    {
        dwFailedControlSet = 0;
    }

    dwSize = sizeof(DWORD);
    lError = RegQueryValueExW(hSelectKey,
                              L"LastKnownGood",
                              0,
                              &dwType,
                              (LPBYTE)&dwLastKnownGoodControlSet,
                              &dwSize);
    if (lError != ERROR_SUCCESS)
    {
        dwLastKnownGoodControlSet = 0;
    }

    RegCloseKey(hSelectKey);

    DPRINT("ControlSets:\n");
    DPRINT("Current: %lu\n", dwCurrentControlSet);
    DPRINT("Default: %lu\n", dwDefaultControlSet);
    DPRINT("Failed: %lu\n", dwFailedControlSet);
    DPRINT("LastKnownGood: %lu\n", dwLastKnownGoodControlSet);

    return TRUE;
}

/* EOF */
