/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    shar_tbl.h

Abstract:

    Define all structures and routines used by the share table.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
#ifndef shar_tbl_h
#define shar_tbl_h

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#define SHARE_NAME_FIELD        1
#define SHARE_PATH_FIELD        2
#define SHARE_COMMENT_FIELD     3

//--------------------------- PUBLIC STRUCTS --------------------------------

   // Entries in the share table
typedef struct share_entry
           {
	   AsnObjectIdentifier Oid;
	   AsnDisplayString svShareName;    // Index
	   AsnDisplayString svSharePath;
	   AsnDisplayString svShareComment;
	   } SHARE_ENTRY;

   // Share table definition
typedef struct
           {
	   UINT        Len;
	   SHARE_ENTRY *Table;
           } SHARE_TABLE;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern SHARE_TABLE      MIB_ShareTable;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI MIB_shares_lmget(
           void
	   );

//------------------------------- END ---------------------------------------

#endif /* shar_tbl_h */

