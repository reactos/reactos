/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    regions.h

Abstract:

    Contains definitions for manipulating MIB region structures.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/
 
#ifndef _REGIONS_H_
#define _REGIONS_H_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "subagnts.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public definitions                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef struct _MIB_REGION_LIST_ENTRY {

    AsnObjectIdentifier             PrefixOid;
    AsnObjectIdentifier             LimitOid;
    LIST_ENTRY                      Link;      
    PSUBAGENT_LIST_ENTRY            pSLE;
    struct _MIB_REGION_LIST_ENTRY * pSubagentRLE;

} MIB_REGION_LIST_ENTRY, *PMIB_REGION_LIST_ENTRY;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
AllocRLE(
    PMIB_REGION_LIST_ENTRY * ppRLE    
    );

BOOL 
FreeRLE(
    PMIB_REGION_LIST_ENTRY pRLE    
    );

BOOL
FindFirstOverlappingRegion(
    PMIB_REGION_LIST_ENTRY * ppRLE,
    PMIB_REGION_LIST_ENTRY pNewRLE
    );

BOOL
FindSupportedRegion(
    PMIB_REGION_LIST_ENTRY * ppRLE,
    AsnObjectIdentifier *    pPrefixOid,
    BOOL                     fAnyOk
    );

BOOL    
UnloadRegions(
    PLIST_ENTRY pListHead
    );

BOOL
LoadSupportedRegions(
    );

BOOL
UnloadSupportedRegions(
    );

#endif // _REGIONS_H_