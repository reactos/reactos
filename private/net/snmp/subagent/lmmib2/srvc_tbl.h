/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    srvc_tbl.h

Abstract:

    Define all of the structures and routines used in the service table.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/

#ifndef srvc_tbl_h
#define srvc_tbl_h

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#define SRVC_NAME_FIELD        1
#define SRVC_INSTALLED_FIELD   2
#define SRVC_OPERATING_FIELD   3
#define SRVC_UNINSTALLED_FIELD 4
#define SRVC_PAUSED_FIELD      5

//--------------------------- PUBLIC STRUCTS --------------------------------

   // Entries in the service table
typedef struct srvc_entry
           {
	   AsnObjectIdentifier Oid;
	   AsnDisplayString svSvcName;            // Index
	   AsnInteger       svSvcInstalledState;
	   AsnInteger       svSvcOperatingState;
	   AsnInteger       svSvcCanBeUninstalled;
	   AsnInteger       svSvcCanBePaused;
	   } SRVC_ENTRY;

   // Service table definition
typedef struct
           {
	   UINT       Len;
	   SRVC_ENTRY *Table;
           } SRVC_TABLE;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern SRVC_TABLE       MIB_SrvcTable;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI MIB_srvcs_lmget(
           void
	   );

//------------------------------- END ---------------------------------------

#endif /* srvc_tbl_h */

