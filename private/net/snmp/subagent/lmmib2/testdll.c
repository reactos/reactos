/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    testdll.c

Abstract:

    LAN Manager MIB 2 Extension Agent DLL.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
//--------------------------- WINDOWS DEPENDENCIES --------------------------

#include <windows.h>


//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

//#include <stdio.h>


//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <snmputil.h>

#include "hash.h"
#include "mib.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

DWORD timeZero = 0;


//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

BOOL DllEntryPoint(
    HANDLE hDll,
    DWORD  dwReason,
    LPVOID lpReserved)
    {
    switch(dwReason)
        {
        case DLL_PROCESS_ATTACH:
        case DLL_PROCESS_DETACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        default:
            break;

        } // end switch()

    return TRUE;

    } // end DllEntryPoint()


BOOL SnmpExtensionInit(
    IN  DWORD  timeZeroReference,
    OUT HANDLE *hPollForTrapEvent,
    OUT AsnObjectIdentifier *supportedView)
    {
    // record time reference from extendible agent
    timeZero = timeZeroReference;

    // setup trap notification
    *hPollForTrapEvent = NULL;

    // tell extendible agent what view this extension agent supports
    *supportedView = MIB_OidPrefix; // NOTE!  structure copy

    // Initialize MIB access hash table
    MIB_HashInit();

    return TRUE;

    } // end SnmpExtensionInit()


BOOL SnmpExtensionTrap(
    OUT AsnObjectIdentifier *enterprise,
    OUT AsnInteger          *genericTrap,
    OUT AsnInteger          *specificTrap,
    OUT AsnTimeticks        *timeStamp,
    OUT RFC1157VarBindList  *variableBindings)
    {

    return FALSE;

    } // end SnmpExtensionTrap()


// This function is implemented in file RESOLVE.C

#if 0
BOOL SnmpExtensionQuery(
    IN BYTE requestType,
    IN OUT RFC1157VarBindList *variableBindings,
    OUT AsnInteger *errorStatus,
    OUT AsnInteger *errorIndex)
    {

    } // end SnmpExtensionQuery()
#endif


//-------------------------------- END --------------------------------------

