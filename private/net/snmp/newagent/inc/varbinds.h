/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    varbinds.h

Abstract:

    Contains definitions for manipulating varbinds.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/
 
#ifndef _VARBINDS_H_
#define _VARBINDS_H_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "regions.h"
#include "network.h"
#include "snmppdus.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public definitions                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef struct _VARBIND_LIST_ENTRY {

    LIST_ENTRY             Link;
    LIST_ENTRY             QueryLink;
    SnmpVarBind            ResolvedVb;        
    SnmpVarBindList        ResolvedVbl;
    UINT                   nState;
    UINT                   nErrorIndex;  
    UINT                   nMaxRepetitions;
    PMIB_REGION_LIST_ENTRY pCurrentRLE;

} VARBIND_LIST_ENTRY, *PVARBIND_LIST_ENTRY;
                                            
#define VARBIND_UNINITIALIZED          0 // varbind info invalid
#define VARBIND_INITIALIZED            1 // varbind info valid
#define VARBIND_RESOLVING              2 // involved in query now
#define VARBIND_PARTIALLY_RESOLVED     3 // subsequent query needed
#define VARBIND_RESOLVED               4 // completed successfully
#define VARBIND_ABORTED                5 // completed unsuccessfully

#define VARBINDSTATESTRING(nState) \
            ((nState == VARBIND_INITIALIZED) \
                ? "initialized" \
                : (nState == VARBIND_PARTIALLY_RESOLVED) \
                    ? "partially resolved" \
                    : (nState == VARBIND_RESOLVED) \
                        ? "resolved" \
                        : (nState == VARBIND_RESOLVING) \
                            ? "resolving" \
                            : (nState == VARBIND_UNINITIALIZED) \
                                ? "uninitialized" \
                                : "aborted")


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
ProcessVarBinds(
    PNETWORK_LIST_ENTRY pNLE
    );

BOOL
UnloadVarBinds(
    PNETWORK_LIST_ENTRY pNLE
    );

BOOL
AllocVLE(
    PVARBIND_LIST_ENTRY * ppVLE
    );

BOOL 
FreeVLE(
    PVARBIND_LIST_ENTRY pVLE
    );

#endif // _VARBINDS_H_
