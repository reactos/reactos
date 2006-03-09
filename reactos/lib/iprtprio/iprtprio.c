/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS IP Route Priority API DLL
 * FILE:        iprtprio.c
 * PURPOSE:     DLL entry
 * PROGRAMMERS: Robert Dickenson (robd@reactos.org)
 * REVISIONS:
 *   RDD August 27, 2002 Created
 */

#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <time.h>

#include <iptypes.h>
#include <ipexport.h>
//#include <mprapi.h>
//#include <iprtprio.h>
//#include "iprtprio.h"
#include "debug.h"

#ifdef __GNUC__
#define EXPORT STDCALL
#else
#define EXPORT CALLBACK
#endif

#ifdef DBG
/* See debug.h for debug/trace constants */
DWORD DebugTraceLevel = MAX_TRACE;
#endif /* DBG */

typedef struct tag_somestruct {
    int size;
    TCHAR szData[2345];
} somestruct;

BOOL Initialised = FALSE;
CRITICAL_SECTION CriticalSection;

/* To make the linker happy */
//VOID STDCALL KeBugCheck (ULONG	BugCheckCode) {}

BOOL
EXPORT
DllMain(HANDLE hInstDll,
        ULONG dwReason,
        PVOID Reserved)
{
    //WSH_DbgPrint(MIN_TRACE, ("DllMain of iprtprio.dll\n"));
    if (!Initialised) {
        InitializeCriticalSection(&CriticalSection);
    }

    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        /* Don't need thread attach notifications so disable them to improve performance */
        DisableThreadLibraryCalls(hInstDll);
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

DWORD
WINAPI
ComputeRouteMetric(IPAddr unknown1, IPMask unknown2, DWORD unknown3, DWORD unknown4)
{
    BYTE* buf = NULL;

    buf = HeapAlloc(GetProcessHeap(), 0, sizeof(somestruct));
    if (buf != NULL) {
        HeapFree(GetProcessHeap(), 0, buf);
    }

    EnterCriticalSection(&CriticalSection);
    LeaveCriticalSection(&CriticalSection);

    UNIMPLEMENTED
    return 0L;
}


DWORD
WINAPI
GetPriorityInfo(DWORD unknown)
{
    DWORD result = NO_ERROR;

    EnterCriticalSection(&CriticalSection);
    LeaveCriticalSection(&CriticalSection);

    UNIMPLEMENTED
    return result;
}

DWORD
WINAPI
SetPriorityInfo(DWORD unknown)
{
    DWORD result = NO_ERROR;

    EnterCriticalSection(&CriticalSection);
    LeaveCriticalSection(&CriticalSection);

    UNIMPLEMENTED
    return result;
}

/* EOF */
