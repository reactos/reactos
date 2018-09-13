/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    hash.h

Abstract:

    Constants, types, and prototypes for Hash Table and supporting functions.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
#ifndef hash_h
#define hash_h

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#include "mib.h"

//--------------------------- PUBLIC STRUCTS --------------------------------

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI MIB_HashInit(
           void
           );

UINT MIB_Hash(
        IN AsnObjectIdentifier *Oid // OID to hash
	);

MIB_ENTRY *MIB_HashLookup(
              IN AsnObjectIdentifier *Oid // OID to lookup
	      );

#ifdef MIB_DEBUG
void MIB_HashPerformance( void );
#endif

//------------------------------- END ---------------------------------------

#endif /* hash_h */

