/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    init.c

Abstract:

    Contains routines and definitions that are independant to any SNMP API.

Environment:

    User Mode - Win32

Revision History:

--*/

#include <windows.h>

BOOLEAN
InitializeDLL(
    IN PVOID  DllHandle,
    IN ULONG  Reason,
    IN LPVOID lpReserved
    )
{
    if (Reason == DLL_PROCESS_ATTACH) {

        DisableThreadLibraryCalls(DllHandle);
    }

    return TRUE;

} // InitializeDLL
