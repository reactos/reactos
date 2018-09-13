/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    kdexts.c

Abstract:

    This file contains the generic routines and initialization code
    for the kernel debugger extensions dll.

Author:

    Wesley Witt (wesw) 26-Aug-1993

Environment:

    User Mode

--*/

#include "precomp.h"
#pragma hdrstop

#include <imagehlp.h>
#include <wdbgexts.h>
#include <ntsdexts.h>
#include <ntverp.h>
//#include <stdexts.h>

//
// globals
//
EXT_API_VERSION        ApiVersion = { VER_PRODUCTVERSION_W >> 8,
				      VER_PRODUCTVERSION_W & 0xff,
                                      EXT_API_VERSION_NUMBER, 0 };
WINDBG_EXTENSION_APIS  ExtensionApis;
ULONG                  STeip;
ULONG                  STebp;
ULONG                  STesp;
USHORT                 SavedMajorVersion;
USHORT                 SavedMinorVersion;
USHORT                 usProcessorArchitecture;
BOOL                   bDebuggingChecked;
ULONG_PTR              UserProbeAddress;

PSZ szProcessorArchitecture[] = {
    "Intel",
    "MIPS",
    "Alpha",
    "PPC",
    "IA64"
};
#define cArchitecture (sizeof(szProcessorArchitecture) / sizeof(PSZ))

PGETEPROCESSDATAFUNC aGetEProcessDataFunc[] = {
    GetEProcessData_X86,
    NULL,
    GetEProcessData_ALPHA,
    NULL,
    GetEProcessData_IA64
};

extern PGETEPROCESSDATAFUNC GetEProcessData;

DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    switch (dwReason) {
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_PROCESS_ATTACH:
            break;
    }

    return TRUE;
}


VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ULONG_PTR offKeProcessorArchitecture;
    ULONG Result;

    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    bDebuggingChecked = (SavedMajorVersion == 0x0c);
    usProcessorArchitecture = (USHORT)-1;
    offKeProcessorArchitecture = GetExpression("KeProcessorArchitecture");
    if (offKeProcessorArchitecture != 0)
        ReadMemory(offKeProcessorArchitecture, &usProcessorArchitecture,
                sizeof(USHORT), &Result);
    if (usProcessorArchitecture >= cArchitecture) {
#ifdef IA64
        GetEProcessData = GetEProcessData_IA64;
#else
        GetEProcessData = GetEProcessData_X86;
#endif
    } else {
        GetEProcessData = aGetEProcessDataFunc[usProcessorArchitecture];
    }

    //
    // Read the user probe address from the target system.
    //
    // N.B. The user probe address is constant on MIPS, Alpha, and the PPC.
    //      On the x86, it may not be defined for the target system if it
    //      does not contain the code to support 3gb of user address space.
    //

    UserProbeAddress = GetExpression("MmUserProbeAddress");
    if ((UserProbeAddress == 0) ||
        (ReadMemory(UserProbeAddress,
                    &UserProbeAddress,
                    sizeof(UserProbeAddress),
                    &Result) == FALSE)) {
        UserProbeAddress = 0x7fff0000;
    }

    return;
}

DECLARE_API( version )
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    dprintf( "%s Extension dll for Build %d debugging %s kernel for Build %d\n",
             DebuggerType,
             VER_PRODUCTBUILD,
             SavedMajorVersion == 0x0c ? "Checked" : "Free",
             SavedMinorVersion
           );
}

VOID
CheckVersion(
    VOID
    )
{
#if DBG
    if ((SavedMajorVersion != 0x0c) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Checked) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#else
    if ((SavedMajorVersion != 0x0f) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Free) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#endif
}

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}

