/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    network.h

Abstract:

    Contains definitions for manipulating transport structures.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/
 
#ifndef _NETWORK_H_
#define _NETWORK_H_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "snmppdus.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public definitions                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef struct _NETWORK_LIST_ENTRY {

    LIST_ENTRY      Link;
    SOCKET          Socket;
    struct sockaddr SockAddr;
    INT             SockAddrLen;
    INT             SockAddrLenUsed;
    WSAOVERLAPPED   Overlapped;
    DWORD           dwStatus;
    DWORD           dwBytesTransferred;
    DWORD           dwFlags;
    WSABUF          Buffer;
    LIST_ENTRY      Bindings;
    LIST_ENTRY      Queries;
    SNMP_PDU        Pdu;
    UINT            nVersion;
    UINT            nTransactionId;
    AsnOctetString  Community;

} NETWORK_LIST_ENTRY, *PNETWORK_LIST_ENTRY;

#define NLEBUFLEN   8192


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
BOOL
IsValidSockAddr(
    struct sockaddr *pAddress
    );

BOOL
AllocNLE(
    PNETWORK_LIST_ENTRY * ppNLE
    );

BOOL 
FreeNLE(
    PNETWORK_LIST_ENTRY pNLE
    );

BOOL
LoadIncomingTransports(
    );

BOOL
UnloadTransport(
    PNETWORK_LIST_ENTRY pNLE
    );

BOOL
UnloadIncomingTransports(
    );

BOOL
LoadOutgoingTransports(
    );

BOOL
UnloadOutgoingTransports(
    );

BOOL
UnloadPdu(
    PNETWORK_LIST_ENTRY pNLE
    );

#endif // _NETWORK_H_
