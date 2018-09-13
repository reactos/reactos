/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    snmpmgrs.h

Abstract:

    Contains definitions for manipulating managers structures.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/
 
#ifndef _SNMPMGRS_H_
#define _SNMPMGRS_H_


#include "snmpmgmt.h"

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public definitions                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define MGRADDR_ALIVE	-1
#define MGRADDR_DEAD	0
#define MGRADDR_DYING	16

typedef struct _MANAGER_LIST_ENTRY {

    LIST_ENTRY      Link;
    struct sockaddr SockAddr;
    INT             SockAddrLen;
    LPSTR           pManager;
    BOOL            fDynamicName;
    DWORD           dwLastUpdate;
    AsnInteger      dwAge;

} MANAGER_LIST_ENTRY, *PMANAGER_LIST_ENTRY;

#define DEFAULT_NAME_TIMEOUT    0x0036EE80  // one hour timeout


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
AllocMLE(
    PMANAGER_LIST_ENTRY * ppMLE,
    LPSTR                pManager
    );

BOOL
FreeMLE(
    PMANAGER_LIST_ENTRY pMLE
    );

BOOL
UpdateMLE(
    PMANAGER_LIST_ENTRY pMLE
    );

BOOL
AddManager(
    PLIST_ENTRY pListHead,
    LPSTR       pManager
    );

BOOL
FindManagerByName(
    PMANAGER_LIST_ENTRY * ppMLE,
    PLIST_ENTRY           pListHead,
    LPSTR                 pManager
    );    

BOOL
IsManagerAddrLegal(
    struct sockaddr_in *  pAddr
    );

BOOL
FindManagerByAddr(
    PMANAGER_LIST_ENTRY * ppMLE,
    struct sockaddr *     pAddr
    );    

BOOL
LoadManagers(
    HKEY        hKey,
    PLIST_ENTRY pListHead
    );

BOOL
UnloadManagers(
    PLIST_ENTRY pListHead
    );

BOOL
LoadPermittedManagers(
    BOOL bFirstCall
    );

BOOL
UnloadPermittedManagers(
    );

#endif // _SNMPMGRS_H_


