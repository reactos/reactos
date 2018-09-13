/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    sess_tbl.h

Abstract:

    Definition of all structures used by the Session table.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/

#ifndef sess_tbl_h
#define sess_tbl_h

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#define SESS_CLIENT_FIELD      1
#define SESS_USER_FIELD        2
#define SESS_NUMCONS_FIELD     3
#define SESS_NUMOPENS_FIELD    4
#define SESS_TIME_FIELD        5
#define SESS_IDLETIME_FIELD    6
#define SESS_CLIENTTYPE_FIELD  7
#define SESS_STATE_FIELD       8

   // State definitions
#define SESS_STATE_ACTIVE      1
#define SESS_STATE_DELETED     2

//--------------------------- PUBLIC STRUCTS --------------------------------

   // Entries in the session table
typedef struct sess_entry
           {
           AsnObjectIdentifier Oid;
	   AsnDisplayString svSesClientName; // Index
	   AsnDisplayString svSesUserName;   // Index
	   AsnInteger       svSesNumConns;
	   AsnInteger       svSesNumOpens;
	   AsnCounter       svSesTime;
	   AsnCounter       svSesIdleTime;
	   AsnInteger       svSesClientType;
	   AsnInteger       svSesState;
	   } SESS_ENTRY;

   // Session table definition
typedef struct
           {
	   UINT       Len;
	   SESS_ENTRY *Table;
           } SESSION_TABLE;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern SESSION_TABLE    MIB_SessionTable ;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI MIB_sess_lmget(
           void
	   );

UINT MIB_sess_lmset(
        IN AsnObjectIdentifier *Index,
	IN UINT Field,
	IN AsnAny *Value
	);

int MIB_sess_match(
       IN AsnObjectIdentifier *Oid,
       OUT UINT *Pos,
       IN BOOL Next
       );

//------------------------------- END ---------------------------------------

#endif /* sess_tbl_h */
