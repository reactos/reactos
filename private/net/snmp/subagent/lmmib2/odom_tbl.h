/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    odom_tbl.h

Abstract:

    Define the structures and routines used in the other domain table.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
#ifndef odom_tbl_h
#define odom_tbl_h

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#define ODOM_NAME_FIELD        1

//--------------------------- PUBLIC STRUCTS --------------------------------

   // Entries in the other domain table
typedef struct dom_other_entry
           {
	   AsnObjectIdentifier Oid;
	   AsnDisplayString domOtherName;
	   } DOM_OTHER_ENTRY;

   // Other domain table definition
typedef struct
           {
	   UINT            Len;
	   DOM_OTHER_ENTRY *Table;
           } DOM_OTHER_TABLE;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern DOM_OTHER_TABLE  MIB_DomOtherDomainTable;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI MIB_odoms_lmget(
           void
	   );

int MIB_odoms_match(
       IN AsnObjectIdentifier *Oid,
       OUT UINT *Pos
       );

UINT MIB_odoms_lmset(
        IN AsnObjectIdentifier *Index,
	IN UINT Field,
	IN AsnAny *Value
	);

//------------------------------- END ---------------------------------------

#endif /* odom_tbl_h */

