/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    network.c

Abstract:

    Contains routines for manipulating transport structures.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"
#include "network.h"
#include "varbinds.h"
#include "snmppdus.h"
#include "query.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
BOOL
IsValidSockAddr(
    struct sockaddr *pAddress
    )
/*++

Routine Description:

    Verifies if an IP or IPX address is valid.
    An IP address is valid if it is AF_INET and it is not 0.0.0.0
    An IPX address is valid if is AF_IPX and the node-number is not null: xxxxxx.000000000000

Arguments:

    pAddress - pointer to a generic network address to be tested

Return Values:

    Returns true if the address is valid.

--*/
{
    if (pAddress == NULL)
        return FALSE;

    if (pAddress->sa_family == AF_INET)
    {
        return (((struct sockaddr_in *)pAddress)->sin_addr.s_addr != 0);
    }
    else if (pAddress->sa_family == AF_IPX)
    {
        char zeroBuff[6] = {0, 0, 0, 0, 0, 0};

        return memcmp(((struct sockaddr_ipx *)pAddress)->sa_nodenum,
                       zeroBuff,
                       sizeof(zeroBuff)) != 0;
    }

    // the address is neither IP nor IPX hence it is definitely an invalid address
    return FALSE;
}

BOOL
AllocNLE(
    PNETWORK_LIST_ENTRY * ppNLE
    )

/*++

Routine Description:

    Allocates transport structure and initializes.

Arguments:

    ppNLE - pointer to receive pointer to list entry.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;
    PNETWORK_LIST_ENTRY pNLE;
    
    // attempt to allocate structure
    pNLE = AgentMemAlloc(sizeof(NETWORK_LIST_ENTRY));

    // validate pointer
    if (pNLE != NULL) {

        // allocate buffer to be used for io
        pNLE->Buffer.buf = AgentMemAlloc(NLEBUFLEN);

        // validate pointer
        if (pNLE->Buffer.buf != NULL) {

            // initialize socket to socket
            pNLE->Socket = INVALID_SOCKET;

            // initialize buffer length
            pNLE->Buffer.len = NLEBUFLEN;

            // initialize subagent query list
            InitializeListHead(&pNLE->Queries);

            // initialize variable bindings list
            InitializeListHead(&pNLE->Bindings);

            // success
            fOk = TRUE;

        } else {
                
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: SVC: could not allocate network io buffer.\n"
                ));
            
            // release
            FreeNLE(pNLE);

            // re-init
            pNLE = NULL;
        }
    
    } else {
        
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SVC: could not allocate network entry.\n"
            ));
    }

    // transfer
    *ppNLE = pNLE;

    return fOk;
}


BOOL 
FreeNLE(
    PNETWORK_LIST_ENTRY pNLE
    )

/*++

Routine Description:

    Releases transport structure.

Arguments:

    pNLE - pointer to transport structure.

Return Values:

    Returns true if successful.

--*/

{
    // validate pointer
    if (pNLE != NULL) {

        // check to see if socket valid
        if (pNLE->Socket != INVALID_SOCKET) {

            // release socket
            closesocket(pNLE->Socket);
        }

        // release pdu
        UnloadPdu(pNLE);

        // release query list
        UnloadQueries(pNLE);

        // release bindings list
        UnloadVarBinds(pNLE);

        // release network buffer
        AgentMemFree(pNLE->Buffer.buf);

        // release memory
        AgentMemFree(pNLE);
    }

    return TRUE;
}


BOOL
LoadIncomingTransports(
    )

/*++

Routine Description:

    Creates entries for each incoming interface.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fUdpOk = FALSE;
    BOOL fIpxOk = FALSE;
    PNETWORK_LIST_ENTRY pNLE = NULL;
    INT nStatus;

    // allocate tcpip
    if (AllocNLE(&pNLE)) {

        struct servent * pServEnt;
        struct sockaddr_in * pSockAddr;

        // initialize sockaddr structure size
        pNLE->SockAddrLen = sizeof(struct sockaddr_in);

        // obtain pointer to sockaddr structure
        pSockAddr = (struct sockaddr_in *)&pNLE->SockAddr;

        // attempt to get server information
        pServEnt = getservbyname("snmp","udp");

        // initialize address structure
        pSockAddr->sin_family = AF_INET;
        pSockAddr->sin_addr.s_addr = INADDR_ANY;
        pSockAddr->sin_port = (pServEnt != NULL)
                                ? (SHORT)pServEnt->s_port
                                : htons(DEFAULT_SNMP_PORT_UDP)
                                ;
        
        // allocate tpcip socket 
        pNLE->Socket = WSASocket(
                            AF_INET,
                            SOCK_DGRAM,
                            0,
                            NULL,
                            0,
                            WSA_FLAG_OVERLAPPED 
                            );

        // validate socket
        if (pNLE->Socket != INVALID_SOCKET) {

            // attempt to bind 
            nStatus = bind(pNLE->Socket, 
                          &pNLE->SockAddr, 
                          pNLE->SockAddrLen
                          );

            // validate return code
            if (nStatus != SOCKET_ERROR) {
                
                SNMPDBG((
                    SNMP_LOG_TRACE,
                    "SNMP: SVC: successfully bound to udp port %d.\n",
                    ntohs(pSockAddr->sin_port)
                    ));

                // insert transport into list of incoming
                InsertTailList(&g_IncomingTransports, &pNLE->Link);

                // success
                fUdpOk = TRUE;
            
            } else {
                
                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "SNMP: SVC: error %d binding to udp port %d.\n",
                    WSAGetLastError(),
                    ntohs(pSockAddr->sin_port)
                    ));
            }

        } else { 
            
            SNMPDBG((
                SNMP_LOG_WARNING,
                "SNMP: SVC: error %d creating udp socket.\n",
                WSAGetLastError()
                ));
        }    

        if (!fUdpOk) {
        
            // release
            FreeNLE(pNLE);
        }    
    }

    // allocate ipx
    if (AllocNLE(&pNLE)) {

        struct sockaddr_ipx * pSockAddr;

        // initialize sockaddr structure size
        pNLE->SockAddrLen = sizeof(struct sockaddr_ipx);

        // obtain pointer to sockaddr structure
        pSockAddr = (struct sockaddr_ipx *)&pNLE->SockAddr;

        // initialize address structure
        pSockAddr->sa_family = AF_IPX;
        pSockAddr->sa_socket = htons(DEFAULT_SNMP_PORT_IPX);
        
        // allocate ipx socket 
        pNLE->Socket = WSASocket(
                            AF_IPX,
                            SOCK_DGRAM,
                            NSPROTO_IPX,
                            NULL,
                            0,
                            WSA_FLAG_OVERLAPPED 
                            );

        // validate socket
        if (pNLE->Socket != INVALID_SOCKET) {

            // attempt to bind 
            nStatus = bind(pNLE->Socket, 
                          &pNLE->SockAddr, 
                          pNLE->SockAddrLen
                          );

            // validate return code
            if (nStatus != SOCKET_ERROR) {
                
                SNMPDBG((
                    SNMP_LOG_TRACE,
                    "SNMP: SVC: successfully bound to ipx port %d.\n",
                    ntohs(pSockAddr->sa_socket)
                    ));

                // insert transport into list of incoming
                InsertTailList(&g_IncomingTransports, &pNLE->Link);

                // success
                fIpxOk = TRUE;

            } else {
                
                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "SNMP: SVC: error %d binding to ipx port %d.\n",
                    WSAGetLastError(),
                    ntohs(pSockAddr->sa_socket)
                    ));
            }

        } else { 
                
            SNMPDBG((
                SNMP_LOG_WARNING,
                "SNMP: SVC: error %d creating ipx socket.\n",
                WSAGetLastError()
                ));
        }    

        if (!fIpxOk) {
        
            // release
            FreeNLE(pNLE);
        }    
    }

    // need one transport min
    return (fUdpOk || fIpxOk);
}


BOOL
UnloadTransport(
    PNETWORK_LIST_ENTRY pNLE
    )
{

    // make sure the parameter is valid, otherwise the macro below AVs
    if (pNLE == NULL)
        return FALSE;

    // remove the entry from the list
    RemoveEntryList(&(pNLE->Link));

    // close the socket
    closesocket(pNLE->Socket);

    // release the memory
    FreeNLE(pNLE);

    return TRUE;
}


BOOL
UnloadIncomingTransports(
    )

/*++

Routine Description:

    Destroys entries for each outgoing interface.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE;
    PNETWORK_LIST_ENTRY pNLE;

    // process entries until empty
    while (!IsListEmpty(&g_IncomingTransports)) {

        // extract next entry from head 
        pLE = RemoveHeadList(&g_IncomingTransports);

        // retrieve pointer to mib region structure 
        pNLE = CONTAINING_RECORD(pLE, NETWORK_LIST_ENTRY, Link);

        // release
        FreeNLE(pNLE);
    }

    return TRUE; 
}


BOOL
LoadOutgoingTransports(
    )

/*++

Routine Description:

    Creates entries for each outgoing interface.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fUdpOk = FALSE;
    BOOL fIpxOk = FALSE;
    PNETWORK_LIST_ENTRY pNLE = NULL;

    // allocate tcpip
    if (AllocNLE(&pNLE)) {

        // allocate tpcip socket 
        pNLE->Socket = WSASocket(
                            AF_INET,
                            SOCK_DGRAM,
                            0,
                            NULL,
                            0,
                            WSA_FLAG_OVERLAPPED 
                            );

        // validate socket
        if (pNLE->Socket != INVALID_SOCKET) {

            pNLE->SockAddr.sa_family = AF_INET;

            // insert transport into list of incoming
            InsertTailList(&g_OutgoingTransports, &pNLE->Link);

            // success
            fUdpOk = TRUE;

        } else {
            
            SNMPDBG((
                SNMP_LOG_WARNING,
                "SNMP: SVC: error %d creating udp socket.\n",
                WSAGetLastError()
                ));
        
            // release
            FreeNLE(pNLE);
        }    
    }

    // allocate ipx
    if (AllocNLE(&pNLE)) {

        // allocate ipx socket 
        pNLE->Socket = WSASocket(
                            AF_IPX,
                            SOCK_DGRAM,
                            NSPROTO_IPX,
                            NULL,
                            0,
                            WSA_FLAG_OVERLAPPED 
                            );

        // validate socket
        if (pNLE->Socket != INVALID_SOCKET) {

            pNLE->SockAddr.sa_family = AF_IPX;

            // insert transport into list of incoming
            InsertTailList(&g_OutgoingTransports, &pNLE->Link);

            // success
            fIpxOk = TRUE;

        } else {
            
            SNMPDBG((
                SNMP_LOG_WARNING,
                "SNMP: SVC: error %d creating ipx socket.\n",
                WSAGetLastError()
                ));
        
            // release
            FreeNLE(pNLE);
        }    
    }

    // need one transport min
    return (fUdpOk || fIpxOk);
}


BOOL
UnloadOutgoingTransports(
    )

/*++

Routine Description:

    Destroys entries for each outgoing interface.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE;
    PNETWORK_LIST_ENTRY pNLE;

    // process entries until empty
    while (!IsListEmpty(&g_OutgoingTransports)) {

        // extract next entry from head 
        pLE = RemoveHeadList(&g_OutgoingTransports);

        // retrieve pointer to mib region structure 
        pNLE = CONTAINING_RECORD(pLE, NETWORK_LIST_ENTRY, Link);

        // release
        FreeNLE(pNLE);
    }

    return TRUE; 
}


BOOL
UnloadPdu(
    PNETWORK_LIST_ENTRY pNLE
    )

/*++

Routine Description:

    Releases resources allocated in pdu structure.

Arguments:

    pNLE - pointer to network list entry.

Return Values:

    Returns true if successful.

--*/

{
    // release community string
    SnmpUtilOctetsFree(&pNLE->Community);

    // release varbinds in pdu
    SnmpUtilVarBindListFree(&pNLE->Pdu.Vbl);

    return TRUE;
}
