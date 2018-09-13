/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    user_tbl.h

Abstract:

    Define all structures and routines for user table.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
#ifndef user_tbl_h
#define user_tbl_h

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#define USER_NAME_FIELD        1

//--------------------------- PUBLIC STRUCTS --------------------------------


   // Entries in the user table
typedef struct user_entry
           {
	   AsnObjectIdentifier Oid;
	   AsnDisplayString svUserName; // Index
	   } USER_ENTRY;

   // User table definition
typedef struct
           {
	   UINT       Len;
	   USER_ENTRY *Table;
           } USER_TABLE;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern USER_TABLE       MIB_UserTable;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI MIB_users_lmget(
           void
	   );

//------------------------------- END ---------------------------------------

#endif /* user_tbl_h */

