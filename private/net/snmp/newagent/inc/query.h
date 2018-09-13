/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    query.h

Abstract:

    Contains definitions for querying subagents.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/
 
#ifndef _QUERY_H_
#define _QUERY_H_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Header files                                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "subagnts.h"
#include "network.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public definitions                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef struct _QUERY_LIST_ENTRY {

    LIST_ENTRY           Link;
    LIST_ENTRY           SubagentVbs;
    SnmpVarBindList      SubagentVbl;
    AsnOctetString       ContextInfo;
    UINT                 nSubagentVbs;
    UINT                 nErrorStatus;
    UINT                 nErrorIndex;
    PSUBAGENT_LIST_ENTRY pSLE;

} QUERY_LIST_ENTRY, *PQUERY_LIST_ENTRY;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
ProcessQueries(
    PNETWORK_LIST_ENTRY pNLE
    );

BOOL
LoadQueries(
    PNETWORK_LIST_ENTRY pNLE
    );

BOOL
UnloadQueries(
    PNETWORK_LIST_ENTRY pNLE
    );

#endif // _QUERY_H_
