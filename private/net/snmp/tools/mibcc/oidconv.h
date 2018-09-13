/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    oidconv.h

Abstract:

    Routines to manage conversions between OID descriptions and numerical OIDs.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
#ifndef oidconv_h
#define oidconv_h

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#include "mibtree.h"

//--------------------------- PUBLIC STRUCTS --------------------------------

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern LPSTR lpInputFileName; /* name to used when converting OID <--> TEXT */

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI SnmpMgrMIB2Disk(
           IN lpTreeNode lpTree,  // Pointer to MIB root
           IN LPSTR lpOutputFileName // Filename of mib
	   );

SNMPAPI SnmpMgrOid2Text(
           IN AsnObjectIdentifier *Oid, // Pointer to OID to convert
	   OUT LPSTR *String            // Resulting text OID
	   );

SNMPAPI SnmpMgrText2Oid(
         IN LPSTR lpszTextOid,           // Pointer to text OID to convert
	 IN OUT AsnObjectIdentifier *Oid // Resulting numeric OID
	 );

//------------------------------- END ---------------------------------------

#endif /* oidconv_h */
