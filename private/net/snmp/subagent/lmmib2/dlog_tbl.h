/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    dlog_tbl.h

Abstract:

    Define all of the structures and routines used in the domain logon list
    table.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
#ifndef dlog_tbl_h
#define dlog_tbl_h

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#define DLOG_USER_FIELD        1
#define DLOG_MACHINE_FIELD     2

//--------------------------- PUBLIC STRUCTS --------------------------------

   // Entries in the domain logon table
typedef struct dom_logon_entry
           {
	   AsnObjectIdentifier Oid;
	   AsnDisplayString domLogonUser;    // Index
	   AsnDisplayString domLogonMachine; // Index
	   } DOM_LOGON_ENTRY;

   // Domain logon table definition
typedef struct
           {
	   UINT            Len;
	   DOM_LOGON_ENTRY *Table;
           } DOM_LOGON_TABLE;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern DOM_LOGON_TABLE  MIB_DomLogonTable;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI MIB_dlogons_lmget(
           void
	   );

//------------------------------- END ---------------------------------------

#endif /* dlog_tbl_h */

