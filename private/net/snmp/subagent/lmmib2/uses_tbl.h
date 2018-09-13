/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    uses_tbl.h

Abstract:

    Define the structures and routines used in the workstation uses table.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
#ifndef uses_tbl_h
#define uses_tbl_h

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#define USES_LOCAL_FIELD       1
#define USES_REMOTE_FIELD      2
#define USES_STATUS_FIELD      3

//--------------------------- PUBLIC STRUCTS --------------------------------

   // Entries in the workstation uses table
typedef struct wksta_uses_entry
           {
	   AsnObjectIdentifier Oid;
	   AsnDisplayString    useLocalName; // Index
	   AsnDisplayString    useRemote;    // Index
	   AsnInteger          useStatus;
	   } WKSTA_USES_ENTRY;

   // Workstation uses table definition
typedef struct
           {
	   UINT             Len;
	   WKSTA_USES_ENTRY *Table;
           } WKSTA_USES_TABLE;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern WKSTA_USES_TABLE MIB_WkstaUsesTable;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI MIB_wsuses_lmget(
           void
	   );

//------------------------------- END ---------------------------------------

#endif /* uses_tbl_h */

