/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    prnt_tbl.h

Abstract:

    Print Queue Table processing routine and structure definitions.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
#ifndef prnt_tbl_h
#define prnt_tbl_h

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#define PRNTQ_NAME_FIELD       1
#define PRNTQ_JOBS_FIELD       2

//--------------------------- PUBLIC STRUCTS --------------------------------

   // Entries in the print queue table
typedef struct printq_entry
           {
           AsnObjectIdentifier Oid;
	   AsnDisplayString svPrintQName;    // Index
	   AsnInteger       svPrintQNumJobs;
	   }  PRINTQ_ENTRY;

   // Print Queue table definition
typedef struct
           {
	   UINT         Len;
	   PRINTQ_ENTRY *Table;
           } PRINTQ_TABLE;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern PRINTQ_TABLE     MIB_PrintQTable;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI MIB_prntq_lmget(
           void
           );

//------------------------------- END ---------------------------------------

#endif /* prnt_tbl_h */

