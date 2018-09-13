/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    sysoid.c

Abstract:

    Contains enterprise oid routine.

        SnmpSvcGetEnterpriseOid

Environment:

    User Mode - Win32

Revision History:

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <snmp.h>
#include <snmputil.h>


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global Variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

extern AsnObjectIdentifier * g_pEnterpriseOid;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public Procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

AsnObjectIdentifier *
SNMP_FUNC_TYPE
SnmpSvcGetEnterpriseOID(
    )

/*++

Routine Description:

    Retrieves enterprise oid for SNMP process.

Arguments:

    None.

Return Values:

    Returns pointer to enterprise oid.

--*/

{
    // return system oid
    return g_pEnterpriseOid;
}
