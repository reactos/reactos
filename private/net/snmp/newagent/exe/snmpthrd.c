/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    snmpthrd.c

Abstract:

    Contains routines for master agent network thread.

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
#include <tchar.h>
#include <stdio.h>
#include "globals.h"
#include "contexts.h"
#include "regions.h"
#include "snmpmgrs.h"
#include "trapmgrs.h"
#include "trapthrd.h"
#include "network.h"
#include "varbinds.h"
#include "snmpmgmt.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT g_nTransactionId = 0;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private definitions                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define MAX_IPX_ADDR_LEN    64
#define MAX_COMMUNITY_LEN   255

#define ERRMSG_TRANSPORT_IP     _T("IP")
#define ERRMSG_TRANSPORT_IPX    _T("IPX")


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

LPSTR
AddrToString(
    struct sockaddr * pSockAddr
    )

/*++

Routine Description:

    Converts sockaddr to display string.

Arguments:

    pSockAddr - pointer to socket address.

Return Values:

    Returns pointer to string.

--*/

{
    static CHAR ipxAddr[MAX_IPX_ADDR_LEN];

    // determine family
    if (pSockAddr->sa_family == AF_INET) {

        struct sockaddr_in * pSockAddrIn;

        // obtain pointer to protocol specific structure
        pSockAddrIn = (struct sockaddr_in * )pSockAddr;

        // forward to winsock conversion function
        return inet_ntoa(pSockAddrIn->sin_addr);

    } else if (pSockAddr->sa_family == AF_IPX) {

        struct sockaddr_ipx * pSockAddrIpx;

        // obtain pointer to protocol specific structure
        pSockAddrIpx = (struct sockaddr_ipx * )pSockAddr;

        // transfer ipx address to static buffer
        sprintf(ipxAddr, 
            "%02x%02x%02x%02x.%02x%02x%02x%02x%02x%02x",
            (BYTE)pSockAddrIpx->sa_netnum[0],
            (BYTE)pSockAddrIpx->sa_netnum[1],
            (BYTE)pSockAddrIpx->sa_netnum[2],
            (BYTE)pSockAddrIpx->sa_netnum[3],
            (BYTE)pSockAddrIpx->sa_nodenum[0],
            (BYTE)pSockAddrIpx->sa_nodenum[1],
            (BYTE)pSockAddrIpx->sa_nodenum[2],
            (BYTE)pSockAddrIpx->sa_nodenum[3],
            (BYTE)pSockAddrIpx->sa_nodenum[4],
            (BYTE)pSockAddrIpx->sa_nodenum[5]
            );

        // return addr
        return ipxAddr;
    }

    // failure
    return NULL;
}


LPSTR
CommunityOctetsToString(
    AsnOctetString  *pAsnCommunity,
    BOOL            bUnicode
    )

/*++

Routine Description:

    Converts community octet string to display string.

Arguments:

    pAsnCommunity - pointer to community octet string.

Return Values:

    Returns pointer to string.

--*/

{
    static CHAR Community[MAX_COMMUNITY_LEN+1];
    LPSTR pCommunity = Community;

    // terminate string
    *pCommunity = '\0';

    // validate pointer
    if (pAsnCommunity != NULL)
    {
        DWORD nChars = 0;
    
        // determine number of characters to transfer
        nChars = min(pAsnCommunity->length, MAX_COMMUNITY_LEN);

        if (bUnicode)
        {
            WCHAR wCommunity[MAX_COMMUNITY_LEN+1];

            // tranfer memory into buffer
            memset(wCommunity, 0, nChars+sizeof(WCHAR));
            memcpy(wCommunity, pAsnCommunity->stream, nChars);
            SnmpUtilUnicodeToAnsi(&pCommunity, wCommunity, FALSE);
        }
        else
        {
            memcpy(Community, pAsnCommunity->stream, nChars);
            Community[nChars] = '\0';
        }
    }

    // success
    return pCommunity;
}


LPSTR
StaticUnicodeToString(
    LPWSTR wszUnicode
    )

/*++

Routine Description:

    Converts null terminated UNICODE string to static LPSTR

Arguments:

    pOctets - pointer to community octet string.

Return Values:

    Returns pointer to string.

--*/

{
    static CHAR szString[MAX_COMMUNITY_LEN+1];
    LPSTR       pszString = szString;

    // terminate string
    *pszString = '\0';

    // validate pointer
    if (wszUnicode != NULL)
    {
        WCHAR wcBreak;
        BOOL  bNeedBreak;

        bNeedBreak = (wcslen(wszUnicode) > MAX_COMMUNITY_LEN);

        if (bNeedBreak)
        {
            wcBreak = wszUnicode[MAX_COMMUNITY_LEN];
            wszUnicode[MAX_COMMUNITY_LEN] = L'\0';
        }

        SnmpUtilUnicodeToAnsi(&pszString, wszUnicode, FALSE);

        if (bNeedBreak)
            wszUnicode[MAX_COMMUNITY_LEN] = wcBreak;
    }

    // success
    return pszString;
}


LPDWORD
RefErrStatus(
    PSNMP_PDU pPdu
    )

    /*++

Routine Description:

    Returns address of the Error Code in a PDU data structure

Arguments:

    pPdu - PDU to check

Return Values:

    Returns Error Code address, returns NULL if no Error Code

--*/

{
    switch (pPdu->nType) {

        case SNMP_PDU_GET:                                                          
        case SNMP_PDU_GETNEXT:                                                      
        case SNMP_PDU_SET:
            return &pPdu->Pdu.NormPdu.nErrorStatus;
            break;
        
        case SNMP_PDU_GETBULK:
            return &pPdu->Pdu.BulkPdu.nErrorStatus;
            break;

        default:
            return NULL;
            break;
    }
}


BOOL
ValidateContext(
    PNETWORK_LIST_ENTRY pNLE
    )

/*++

Routine Description:

    Checks access rights of given context.

Arguments:

    pNLE - pointer to network list entry.

Return Values:

    Returns true if manager allowed access.

--*/

{
    BOOL fAccessOk = TRUE;
    BOOL fOk = FALSE;
    PCOMMUNITY_LIST_ENTRY pCLE = NULL;
    AsnOctetString unicodeCommunity;
    LPWSTR         pUnicodeName;

    if (pNLE->Community.length != 0)
    {
        unicodeCommunity.length = pNLE->Community.length * sizeof(WCHAR);
        unicodeCommunity.stream = SnmpUtilMemAlloc(unicodeCommunity.length);
        unicodeCommunity.dynamic = TRUE;

        if (unicodeCommunity.stream == NULL)
            return FALSE;

        fAccessOk = (MultiByteToWideChar(
                        CP_ACP,
                        MB_PRECOMPOSED,
                        pNLE->Community.stream,
                        pNLE->Community.length,
                        (LPWSTR)(unicodeCommunity.stream),
                        unicodeCommunity.length) != 0);
    }
    else
    {
        unicodeCommunity.length = 0;
        unicodeCommunity.stream = NULL;
        unicodeCommunity.dynamic = FALSE;
    }
        
    // search for community string
    if (fAccessOk && FindValidCommunity(&pCLE, &unicodeCommunity)) 
    {
        // check access per pdu type
        if (pNLE->Pdu.nType == SNMP_PDU_SET) {
        
            // check flags for write privileges
            fAccessOk = (pCLE->dwAccess >= SNMP_ACCESS_READ_WRITE);

        } else {

            // check flags for read privileges
            fAccessOk = (pCLE->dwAccess >= SNMP_ACCESS_READ_ONLY);
        }

        if (!fAccessOk) {

            // Community does not have the right access
            // RefErrStatus returns a pointer to the ErrorStatus field from the SNMP_PDU structure.
            // It returns NULL if the PDU is in fact SNMP_TRAP_PDU. This doesn't happen here as far
            // as ValidateContext is called only after ParseMessage() which is filtering out
            // SNMP_TRAP_PDU.
            *RefErrStatus(&pNLE->Pdu) = SNMP_ERRORSTATUS_NOSUCHNAME;

            // register wrong operation for specified community into management structure
            mgmtCTick(CsnmpInBadCommunityUses);
            fOk = TRUE;
        }
    }
    else
    {
        fAccessOk = FALSE;

        // register community name failure into the management structure
        mgmtCTick(CsnmpInBadCommunityNames);
    }

    // see if access attempt should be logged
	if (!fAccessOk && snmpMgmtBase.AsnIntegerPool[IsnmpEnableAuthenTraps].asnValue.number) {

        // send authentication trap
        GenerateAuthenticationTrap();        
    }
        
    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: SVC: %s request from community %s.\n",
        fAccessOk 
            ? "accepting"
            : "rejecting"
            ,
        CommunityOctetsToString(&(pNLE->Community), FALSE)
        ));

    SnmpUtilOctetsFree(&unicodeCommunity);

    return (fOk || fAccessOk);
}


BOOL
ValidateManager(
    PNETWORK_LIST_ENTRY pNLE
    )

/*++

Routine Description:

    Checks access rights of given manager.

Arguments:

    pNLE - pointer to network list entry.

Return Values:

    Returns true if manager allowed access.

--*/

{
    BOOL fAccessOk = FALSE;
    PMANAGER_LIST_ENTRY pMLE = NULL;

    fAccessOk = IsManagerAddrLegal((struct sockaddr_in *)&pNLE->SockAddr) &&
                (FindManagerByAddr(&pMLE, &pNLE->SockAddr) ||
                 IsListEmpty(&g_PermittedManagers)
                );

    if (!fAccessOk &&
        snmpMgmtBase.AsnIntegerPool[IsnmpEnableAuthenTraps].asnValue.number)
        GenerateAuthenticationTrap();

    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: SVC: %s request from %s.\n",
        fAccessOk 
            ? "accepting"
            : "rejecting"
            ,
        AddrToString(&pNLE->SockAddr)
        ));

    return fAccessOk;
}


BOOL
ProcessSnmpMessage(
    PNETWORK_LIST_ENTRY pNLE
    )

/*++

Routine Description:

    Parse SNMP message and dispatch to subagents.

Arguments:

    pNLE - pointer to network list entry.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;

    // decode request
    if (ParseMessage(
            &pNLE->nVersion,
            &pNLE->Community,
            &pNLE->Pdu,
            pNLE->Buffer.buf,
            pNLE->dwBytesTransferred
            )) {

        SNMPDBG((
            SNMP_LOG_TRACE,
            "SNMP: SVC: %s request, community %s, %d variable(s).\n",
            PDUTYPESTRING(pNLE->Pdu.nType),
            CommunityOctetsToString(&(pNLE->Community), FALSE),
            pNLE->Pdu.Vbl.len
            ));        
        
        // validate context 
        if (ValidateContext(pNLE)) {

            // process varbinds 
            // RefErrStatus returns a pointer to the ErrorStatus field from SNMP_PDU structure.
            // The return value is NULL only if SNMP_PDU is in fact an SNMP_TRAP_PDU. Which is
            // not the case here, as far as it would have been filtered out by ParseMessage().
            if ((*RefErrStatus(&pNLE->Pdu) != SNMP_ERRORSTATUS_NOERROR) ||
                (ProcessVarBinds(pNLE))) {

                // initialize buffer length
                pNLE->Buffer.len = NLEBUFLEN;

                // reset pdu type to response
                pNLE->Pdu.nType = SNMP_PDU_RESPONSE;
                
                // encode response
                fOk = BuildMessage(
                        pNLE->nVersion,
                        &pNLE->Community,
                        &pNLE->Pdu,
                        pNLE->Buffer.buf,
                        &pNLE->Buffer.len
                        );
            }
        }
    }
    else {

        // register BER decoding failure into the management structures
        mgmtCTick(CsnmpInASNParseErrs);
    }

    // release pdu
    UnloadPdu(pNLE);

    return fOk; 
}         


void CALLBACK
RecvCompletionRoutine(
    IN  DWORD           dwStatus,
    IN  DWORD           dwBytesTransferred,
    IN  LPWSAOVERLAPPED pOverlapped,
    IN  DWORD           dwFlags
    )

/*++

Routine Description:

    Callback for completing asynchronous reads.

Arguments:

    Status - completion status for the overlapped operation.

    BytesTransferred - number of bytes transferred.

    pOverlapped - pointer to overlapped structure.

    Flags - receive flags.

Return Values:

    None.

--*/

{
    PNETWORK_LIST_ENTRY pNLE; 

    EnterCriticalSection(&g_RegCriticalSectionA);

    // retreive pointer to network list entry from overlapped structure
    pNLE = CONTAINING_RECORD(pOverlapped, NETWORK_LIST_ENTRY, Overlapped);

    // copy receive completion information
    pNLE->nTransactionId = ++g_nTransactionId;
    pNLE->dwBytesTransferred = dwBytesTransferred;
    pNLE->dwStatus = dwStatus;
    pNLE->dwFlags = dwFlags;
        
    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: SVC: --- transaction %d begin ---\n",
        pNLE->nTransactionId
        ));        
        
    // validate status
    if (dwStatus == NOERROR) {

        // register incoming packet into the management structure
        mgmtCTick(CsnmpInPkts);

        SNMPDBG((
            SNMP_LOG_TRACE,
            "SNMP: SVC: received %d bytes from %s.\n",
            pNLE->dwBytesTransferred,
            AddrToString(&pNLE->SockAddr)
            ));        
        
        // check manager address
        if (ValidateManager(pNLE)) {

            // process snmp message 
            if (ProcessSnmpMessage(pNLE)) {

                // synchronous send
                dwStatus = WSASendTo(
                              pNLE->Socket,
                              &pNLE->Buffer,
                              1,
                              &pNLE->dwBytesTransferred,
                              pNLE->dwFlags,
                              &pNLE->SockAddr,
                              pNLE->SockAddrLenUsed,
                              NULL,
                              NULL
                              );

                // register outgoing packet into the management structure
                mgmtCTick(CsnmpOutPkts);
                // register outgoing Response PDU
                mgmtCTick(CsnmpOutGetResponses);

                // validate return code
                if (dwStatus != SOCKET_ERROR) {

                    SNMPDBG((
                        SNMP_LOG_TRACE,
                        "SNMP: SVC: sent %d bytes to %s.\n",
                        pNLE->dwBytesTransferred,
                        AddrToString(&pNLE->SockAddr)
                        ));

                } else {
                    
                    SNMPDBG((
                        SNMP_LOG_ERROR,
                        "SNMP: SVC: error %d sending response.\n",
                        WSAGetLastError()
                        ));
                }
            }
        }

    } else {
    
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SVC: error %d receiving snmp request.\n",
            dwStatus
            ));
    }

    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: SVC: --- transaction %d end ---\n",
        pNLE->nTransactionId
        ));        

    LeaveCriticalSection(&g_RegCriticalSectionA);

}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

DWORD
ProcessSnmpMessages(
    PVOID pParam
    )

/*++

Routine Description:

    Thread procedure for processing SNMP PDUs.

Arguments:

    pParam - unused.

Return Values:

    Returns true if successful.

--*/

{
    DWORD dwStatus;
    PLIST_ENTRY pLE;
    PNETWORK_LIST_ENTRY pNLE;
    
    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: SVC: Loading Registry Parameters.\n"
        ));

    // fire cold start trap
    GenerateColdStartTrap();

    SNMPDBG((
        SNMP_LOG_TRACE,
        "SNMP: SVC: starting pdu processing thread.\n"
        ));

    ReportSnmpEvent(
        SNMP_EVENT_SERVICE_STARTED,
        0,
        NULL,
        0);

    // loop
    for (;;)
    {
        // obtain pointer to first transport
        pLE = g_IncomingTransports.Flink;

        // loop through incoming transports
        while (pLE != &g_IncomingTransports)
        {
            // retreive pointer to network list entry from link
            pNLE = CONTAINING_RECORD(pLE, NETWORK_LIST_ENTRY, Link);

            // make sure recv is not pending
            if (pNLE->dwStatus != WSA_IO_PENDING)
            {
                // reset completion status
                pNLE->dwStatus = WSA_IO_PENDING;

                // intialize address structure size 
                pNLE->SockAddrLenUsed = pNLE->SockAddrLen;

                // initialize buffer length
                pNLE->Buffer.len = NLEBUFLEN;

                // re-initialize
                pNLE->dwFlags = 0;

                // post receive buffer
                dwStatus = WSARecvFrom(
                                pNLE->Socket,
                                &pNLE->Buffer,
                                1, // dwBufferCount
                                &pNLE->dwBytesTransferred,
                                &pNLE->dwFlags,
                                &pNLE->SockAddr,
                                &pNLE->SockAddrLenUsed,
                                &pNLE->Overlapped,
                                RecvCompletionRoutine
                                );

                // handle network failures
                if (dwStatus == SOCKET_ERROR)
                {
                    // retrieve last error
                    dwStatus = WSAGetLastError();

                    // if WSA_IO_PENDING everything is ok, just waiting for incoming traffic. Otherwise...
                    if (dwStatus != WSA_IO_PENDING)
                    {
                        // WSAECONNRESET means the last 'WSASendTo' (the one from RecvCompletionRoutine) failed
                        // most probably because the manager closed the socket (so we got back 'unreacheable destination port')
                        if (dwStatus == WSAECONNRESET)
                        {
                            SNMPDBG((
                                SNMP_LOG_ERROR,
                                "SNMP: SVC: Benign error %d posting receive buffer. Retry...\n",
                                dwStatus
                                ));

                            // just go one more time and setup the port. It shouldn't ever loop continuously
                            // and hence hog the CPU..
                            pNLE->dwStatus = ERROR_SUCCESS;
                            continue;
                        }
                        else
                        {
                            // prepare the event log insertion string
                            LPTSTR pMessage = (pNLE->SockAddr.sa_family == AF_INET) ?
                                                ERRMSG_TRANSPORT_IP :
                                                ERRMSG_TRANSPORT_IPX;

                            // another error occurred. We don't know how to handle it so it is a fatal
                            // error for this transport. Will shut it down.
                            SNMPDBG((
                                SNMP_LOG_ERROR,
                                "SNMP: SVC: Fatal error %d posting receive buffer. Skip transport.\n",
                                dwStatus
                                ));

                            ReportSnmpEvent(
                                SNMP_EVNT_INCOMING_TRANSPORT_CLOSED,
                                1,
                                &pMessage,
                                dwStatus);

                            // first step next with the pointer
                            pLE = pLE->Flink;

                            // delete this transport from the incoming transports list
                            UnloadTransport(pNLE);

                            // go on further
                            continue;
                        }
                    }
                }
            }

            pLE = pLE->Flink;
        }

        // we might want to shut the service down if no incoming transport remains.
        // we might as well consider letting the service up in order to keep sending outgoing traps.
        // for now, keep the service up (code below commented)
        //if (IsListEmpty(&g_IncomingTransports))
        //{
        //    ReportSnmpEvent(...);
        //    ProcessControllerRequests(SERVICE_CONTROL_STOP);
        //}

        // wait for incoming requests or indication of process termination 
        dwStatus = WaitForSingleObjectEx(g_hTerminationEvent, INFINITE, TRUE);

        // validate return code
        if (dwStatus == WAIT_OBJECT_0) {
                
            SNMPDBG((
                SNMP_LOG_TRACE,
                "SNMP: SVC: exiting pdu processing thread.\n"
                ));

            // success
            return NOERROR;

        } else if (dwStatus != WAIT_IO_COMPLETION) {

            // retrieve error
            dwStatus = GetLastError();
            
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: SVC: error %d waiting for request.\n",
                dwStatus
                ));
            
            // failure
            return dwStatus;
        }
    }
}
