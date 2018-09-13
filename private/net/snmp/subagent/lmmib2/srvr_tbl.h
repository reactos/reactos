/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    srvr_tbl.h

Abstract:

    Define all of the structures and routines used in the server table.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
#ifndef srvr_tbl_h
#define srvr_tbl_h

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#define SRVR_NAME_FIELD        1

//--------------------------- PUBLIC STRUCTS --------------------------------

   // Entries in the domain server table
typedef struct dom_server_entry
           {
	   AsnObjectIdentifier Oid;
	   AsnDisplayString domServerName; // Index
	   } DOM_SERVER_ENTRY;

   // Domain server table definition
typedef struct
           {
	   UINT             Len;
	   DOM_SERVER_ENTRY *Table;
           } DOM_SERVER_TABLE;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern DOM_SERVER_TABLE MIB_DomServerTable;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI MIB_svsond_lmget(
           void
	   );

//------------------------------- END ---------------------------------------

#endif /* srvr_tbl_h */

