/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    trapmgrs.h

Abstract:

    Contains definitions for manipulating trap destination structures.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/
 
#ifndef _TRAPMGRS_H_
#define _TRAPMGRS_H_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public definitions                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef struct _TRAP_DESTINATION_LIST_ENTRY {

    LIST_ENTRY Link;
    LIST_ENTRY Managers;
    LPSTR      pCommunity;

} TRAP_DESTINATION_LIST_ENTRY, *PTRAP_DESTINATION_LIST_ENTRY;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
AllocTLE(
    PTRAP_DESTINATION_LIST_ENTRY * ppTLE,
    LPSTR                          pCommunity    
    );

BOOL
FreeTLE(
    PTRAP_DESTINATION_LIST_ENTRY pTLE
    );

BOOL
LoadTrapDestinations(
    BOOL bFirstCall
    );

BOOL
UnloadTrapDestinations(
    );

#endif // _TRAPMGRS_H_

