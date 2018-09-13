/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

Abstract:

Revision history:

--*/

#include <snmp.h>
#include <snmpexts.h>
#include "mibentry.h"

SnmpTfxHandle   g_tfxHandle;
PSNMP_MGMTVARS  ge_pMgmtVars;

BOOL
SnmpExtensionInit(
    IN     DWORD                 uptimeReference,
       OUT HANDLE *              lpPollForTrapEvent,
       OUT AsnObjectIdentifier * lpFirstSupportedView)
{
    g_tfxHandle = SnmpTfxOpen(1,&view_snmp);

    if (g_tfxHandle == NULL)
        return FALSE;

    *lpFirstSupportedView = view_snmp.viewOid;
    *lpPollForTrapEvent = NULL;

    return TRUE;    
}

// the SNMP master agent calls this function immediately after SnmpExtensionInit
// in order to provide a pointer to the internal service management information.
// No mutex protection is needed so far, as there is no concurrency in writing the
// buffer pointed by this parameter.
BOOL
SnmpExtensionMonitor(
    IN  LPVOID                  pAgentMgmtVars)
{
    ge_pMgmtVars = (PSNMP_MGMTVARS)pAgentMgmtVars;
    return TRUE;
}

BOOL 
SnmpExtensionQuery(
    IN     BYTE                 requestType,
    IN OUT RFC1157VarBindList * variableBindings,
       OUT AsnInteger *         errorStatus,
       OUT AsnInteger *         errorIndex)
{
    // forward to framework
    return SnmpTfxQuery(
                g_tfxHandle,
                requestType,
                variableBindings,
                errorStatus,
                errorIndex);
}

BOOL 
SnmpExtensionTrap(
    OUT AsnObjectIdentifier *enterprise,
    OUT AsnInteger *genericTrap,
    OUT AsnInteger *specificTrap,
    OUT AsnTimeticks *timeStamp,
    OUT RFC1157VarBindList *variableBindings)
{
    // no traps
    return FALSE;
}


BOOL WINAPI
DllMain(
    HINSTANCE       hInstDLL,
    DWORD           fdwReason,
    LPVOID          pReserved)
{
    
    switch ( fdwReason )
    {
    case DLL_PROCESS_ATTACH :
        DisableThreadLibraryCalls( hInstDLL );
        break;

    case DLL_PROCESS_DETACH :
        break;

    default :
        break;
    }

    return TRUE;
}
