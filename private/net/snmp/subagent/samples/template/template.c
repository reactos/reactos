/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    template.c

Abstract:

    Contains routines for implementing SNMP subagent.

Environment:

    User Mode - Win32

--*/
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <snmp.h>   


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT                g_idsMibPrefix[] = {1,2,3,4,5,6,7,8,9};
AsnObjectIdentifier g_oidMibPrefix   = DEFINE_OID(g_idsMibPrefix);


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL 
SNMP_FUNC_TYPE
SnmpExtensionInit(
    DWORD                 dwUptimeReference,    
    HANDLE *              phSubagentTrapEvent,  
    AsnObjectIdentifier * pFirstSupportedRegion 
    )

/*++

Routine Description:

    Initializes subagent and retrieves supported region.

Arguments:

    dwUptimeReference - reference for calculating agent uptime.   

    phSubagentTrapEvent - pointer to receive subagent event handle. 

    pFirstSupportedRegion - pointer to receive first supported region oid.

Return Values:

    Returns true if successful.

--*/

{
    // initialize trap handle
    *phSubagentTrapEvent = NULL;

    // transfer subagent supported region 
    *pFirstSupportedRegion = g_oidMibPrefix;

    // success
    return TRUE;
}


BOOL 
SNMP_FUNC_TYPE
SnmpExtensionInitEx(
    AsnObjectIdentifier * pNextSupportedRegion
    )

/*++

Routine Description:

    Retrieves any additional regions a subagent may support.

Arguments:

    pNextSupportedRegion - pointer to receive next supported region oid.

Return Values:

    Returns true if next supported region available.

--*/

{
    // no more
    return FALSE;
}


BOOL
SNMP_FUNC_TYPE
SnmpExtensionQuery(
    BYTE              bPduType,    
    SnmpVarBindList * pVarBindList,
    AsnInteger32 *    pErrorStatus, 
    AsnInteger32 *    pErrorIndex  
    )

/*++

Routine Description:

    Processes SNMP PDUs forwarded by the master agent.

Arguments:

    bPduType - type of PDU to process.

    pVarBindList - pointer to variables included in PDU.

    pErrorStatus - pointer to error status of processing.

    pErrorIndex - pointer to index of errant variable (if any).

Return Values:

    Returns true if successful.

--*/

{
    // failure
    return FALSE;
}


BOOL 
SNMP_FUNC_TYPE
SnmpExtensionTrap(
    AsnObjectIdentifier * pEnterpriseOid, 
    AsnInteger32 *        pGenericTrapId, 
    AsnInteger32 *        pSpecificTrapId,
    AsnTimeticks *        pTimeStamp,     
    SnmpVarBindList *     pVarBindList
    )

/*++

Routine Description:

    Retrieves trap information to be included in outgoing PDU.

Arguments:

    pEnterpriseOid - pointer to receive enterprise oid.

    pGenericTrapId - pointer to receive generic trap id.

    pSpecificTrapId - pointer to receive specific trap id.

    pTimeStamp - pointer to receive trap time stamp.

    pVarBindList - pointer to receive allocated variables.

Return Values:

    Returns true if trap information available.

--*/

{
    // no traps
    return FALSE;
}
