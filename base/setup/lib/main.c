/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     DLL Main Routine
 * COPYRIGHT:   Copyright 2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES ******************************************************************/

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>

#define NTOS_MODE_USER
// #include <ndk/umfuncs.h>
#include <ndk/rtlfuncs.h>

/* GLOBALS *******************************************************************/

extern HANDLE ProcessHeap;

/* ENTRY-POINT ***************************************************************/

/* Declared in ndk/umfuncs.h */
NTSTATUS
NTAPI
LdrDisableThreadCalloutsForDll(
    _In_ PVOID BaseAddress);

BOOL
NTAPI
DllMain(
    _In_ HINSTANCE hDll,
    _In_ ULONG dwReason,
    _In_opt_ PVOID pReserved)
{
    UNREFERENCED_PARAMETER(pReserved);

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        LdrDisableThreadCalloutsForDll(hDll);
        ProcessHeap = RtlGetProcessHeap();
    }

    return TRUE;
}

/* EOF */
