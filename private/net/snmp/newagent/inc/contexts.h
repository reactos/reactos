/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    contexts.h

Abstract:

    Contains definitions for manipulating SNMP community structures.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/
 
#ifndef _CONTEXTS_H_
#define _CONTEXTS_H_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public definitions                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef struct _COMMUNITY_LIST_ENTRY {

    LIST_ENTRY     Link;
    DWORD          dwAccess;     
    AsnOctetString Community;

} COMMUNITY_LIST_ENTRY, *PCOMMUNITY_LIST_ENTRY;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
AllocCLE(
    PCOMMUNITY_LIST_ENTRY * ppCLE,
    LPWSTR                  pCommunity
    );

BOOL 
FreeCLE(
    PCOMMUNITY_LIST_ENTRY pCLE
    );

BOOL
FindValidCommunity(
    PCOMMUNITY_LIST_ENTRY * ppCLE,
    AsnOctetString *        pCommunity
    );

BOOL
LoadValidCommunities(
    BOOL    bFirstCall
    );

BOOL
UnloadValidCommunities(
    );

#endif // _CONTEXTS_H_