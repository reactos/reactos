/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    trapthrd.c

Abstract:

    Contains routines for trap processing thread.

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
#include "trapthrd.h"
#include "subagnts.h"
#include "snmppdus.h"
#include "trapmgrs.h"
#include "snmpmgrs.h"
#include "network.h"
#include "snmpmgmt.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static SnmpVarBindList g_NullVbl = { NULL, 0 };


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
LoadWaitObjects(
    DWORD * pnWaitObjects,
    PHANDLE * ppWaitObjects,
    PSUBAGENT_LIST_ENTRY ** pppNLEs
    )

/*++

Routine Description:

    Loads arrays with necessary wait object information.

Arguments:

    pnWaitObjects - pointer to receive count of wait objects.

    ppWaitObjects - pointer to receive wait object handles.

    pppNLEs - pointer to receive array of associated subagents pointers.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE;
    PSUBAGENT_LIST_ENTRY pNLE;
    PSUBAGENT_LIST_ENTRY * ppNLEs = NULL;
    PHANDLE pWaitObjects = NULL;
    DWORD nWaitObjects = 2;
    BOOL fOk = FALSE;

    EnterCriticalSection(&g_RegCriticalSectionB);
    
    // point to first subagent
    pLE = g_Subagents.Flink;

    // process each subagent
    while (pLE != &g_Subagents) {

        // retreive pointer to subagent list entry from link
        pNLE = CONTAINING_RECORD(pLE, SUBAGENT_LIST_ENTRY, Link);

        // check for subagent trap event
        if (pNLE->hSubagentTrapEvent != NULL) {
        
            // increment        
            nWaitObjects++;
        }            

        // next entry
        pLE = pLE->Flink;
    }
    
    // attempt to allocate array of subagent pointers
    ppNLEs = AgentMemAlloc(nWaitObjects * sizeof(PSUBAGENT_LIST_ENTRY));
        
    // validate pointers
    if (ppNLEs != NULL) {

        // attempt to allocate array of event handles
        pWaitObjects = AgentMemAlloc(nWaitObjects * sizeof(HANDLE));

        // validate pointer
        if (pWaitObjects != NULL) {

            // success
            fOk = TRUE;

        } else {
                
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: SVC: could not allocate handle array.\n"
                ));

            // release array
            AgentMemFree(ppNLEs);
    
            // re-init
            ppNLEs = NULL;
        }

    } else {
        
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SVC: could not allocate subagent pointers.\n"
            ));
    }
    
    if (fOk) {
    
        // initialize
        DWORD dwIndex = 0;

        // point to first subagent
        pLE = g_Subagents.Flink;

        // process each subagent and check for overflow
        while ((pLE != &g_Subagents) && (dwIndex < nWaitObjects - 1)) {

            // retreive pointer to subagent list entry from link
            pNLE = CONTAINING_RECORD(pLE, SUBAGENT_LIST_ENTRY, Link);

            // check for subagent trap event
            if (pNLE->hSubagentTrapEvent != NULL) {
            
                // copy subagent trap event handle
                pWaitObjects[dwIndex] = pNLE->hSubagentTrapEvent;

                // copy subagent pointer
                ppNLEs[dwIndex] = pNLE;

                // next
                dwIndex++;
            }            

            // next entry
            pLE = pLE->Flink;
        }

        // copy registry update event into second last entry
        pWaitObjects[dwIndex++] = g_hRegistryEvent;

        // copy termination event into last entry
        pWaitObjects[dwIndex++] = g_hTerminationEvent;

        // validate number of items
        if (dwIndex != nWaitObjects) {
            
            SNMPDBG((
                SNMP_LOG_WARNING,
                "SNMP: SVC: updating number of events from %d to %d.\n",
                nWaitObjects,
                dwIndex
                ));

            // use latest number
            nWaitObjects = dwIndex;
        }
    }

    // transfer wait object information 
    *pnWaitObjects = fOk ? nWaitObjects : 0;
    *ppWaitObjects = pWaitObjects;
    *pppNLEs = ppNLEs;

    LeaveCriticalSection(&g_RegCriticalSectionB);

    return fOk;
}


BOOL
UnloadWaitObjects(
    PHANDLE pWaitObjects,
    PSUBAGENT_LIST_ENTRY * ppNLEs
    )

/*++

Routine Description:

    Loads arrays with necessary wait object information.

Arguments:

    pWaitObjects - pointer to wait object handles.

    ppNLEs - pointer to array of associated subagents pointers.

Return Values:

    Returns true if successful.

--*/

{
    // release array
    AgentMemFree(pWaitObjects);

    // release array
    AgentMemFree(ppNLEs);
    
    return TRUE;
}


BOOL
GenerateExtensionTrap(
    AsnObjectIdentifier * pEnterpriseOid,
    AsnInteger32          nGenericTrap,
    AsnInteger32          nSpecificTrap,
    AsnTimeticks          nTimeStamp,
    SnmpVarBindList *     pVbl
    )

/*

Routine Description:

    Generates trap for subagent.

Arguments:

    pEnterpriseOid - pointer to EnterpriseOid OID.

    nGenericTrap - generic trap identifier.

    nSpecificTrap - EnterpriseOid specific trap identifier.

    nTimeStamp - timestamp to include in trap.

    pVbl - pointer to optional variables.

Return Values:

    Returns true if successful.

*/

{
    SNMP_PDU Pdu;
    BOOL fOk = FALSE;

    // note this is in older format
    Pdu.nType = SNMP_PDU_V1TRAP;

    // validate pointer 
    if (pVbl != NULL) {

        // copy varbinds 
        Pdu.Vbl = *pVbl;

    } else {

        // initialize
        Pdu.Vbl.len = 0;
        Pdu.Vbl.list = NULL;
    }

    // validate enterprise oid
    if ((pEnterpriseOid != NULL) &&
        (pEnterpriseOid->ids != NULL) &&
        (pEnterpriseOid->idLength != 0)) {

        // transfer specified enterprise oid
        Pdu.Pdu.TrapPdu.EnterpriseOid = *pEnterpriseOid;

    } else {

        // transfer microsoft enterprise oid 
        // BUGBUG: transfer the AsnObjectIdentifier structure as a whole, but no new memory is allocated
        // for the 'ids' buffer. Hence, Pdu....EnterpriseOid should not be 'SnmpUtilFreeOid'!!
        Pdu.Pdu.TrapPdu.EnterpriseOid = snmpMgmtBase.AsnObjectIDs[OsnmpSysObjectID].asnValue.object;
    }    

    // make sure that the system uptime is consistent by overriding
    Pdu.Pdu.TrapPdu.nTimeticks = nTimeStamp ? SnmpSvcGetUptime() : 0;

    // transfer the remaining parameters 
    Pdu.Pdu.TrapPdu.nGenericTrap  = nGenericTrap;
    Pdu.Pdu.TrapPdu.nSpecificTrap = nSpecificTrap;

    // initialize agent address structure
    Pdu.Pdu.TrapPdu.AgentAddr.dynamic = FALSE;
    Pdu.Pdu.TrapPdu.AgentAddr.stream  = NULL;
    Pdu.Pdu.TrapPdu.AgentAddr.length  = 0;

    // send trap to managers
    return GenerateTrap(&Pdu);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
ProcessSubagentEvents(
    )

/*++

Routine Description:

    Processes subagent trap events.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;
    PSUBAGENT_LIST_ENTRY * ppNLEs = NULL;
    PHANDLE pWaitObjects = NULL;
    DWORD nWaitObjects = 0;
    DWORD dwIndex;

    // attempt to load waitable objects into array
    if (LoadWaitObjects(&nWaitObjects, &pWaitObjects, &ppNLEs)) {

        // loop
        for (;;) {

            // subagent event or termination 
            dwIndex = WaitForMultipleObjects(
                            nWaitObjects,
                            pWaitObjects,
                            FALSE,
                            INFINITE
                            );

            // check for process termination event first 
            if (dwIndex == (WAIT_OBJECT_0 + nWaitObjects - 1)) {
                
                SNMPDBG((
                    SNMP_LOG_TRACE,
                    "SNMP: SVC: shutting down trap thread.\n"
                    ));
                
                break; // bail...

            // check for registry update event next
            } else if (dwIndex == (WAIT_OBJECT_0 + nWaitObjects - 2)) {
                
                SNMPDBG((
                    SNMP_LOG_TRACE,
                    "SNMP: SVC: recalling LoadWaitObjects.\n"
                    ));
                
                if (!LoadWaitObjects(&nWaitObjects, &pWaitObjects, &ppNLEs))
                    break;

            // check for subagent trap notification event
            } else if (dwIndex < (WAIT_OBJECT_0 + nWaitObjects - 1)) {

                AsnObjectIdentifier EnterpriseOid;
                AsnInteger          nGenericTrap;
                AsnInteger          nSpecificTrap;
                AsnInteger          nTimeStamp;
                SnmpVarBindList     Vbl;

                PFNSNMPEXTENSIONTRAP pfnSnmpExtensionTrap;

                // retrieve pointer to subagent trap entry point
                pfnSnmpExtensionTrap = ppNLEs[dwIndex]->pfnSnmpExtensionTrap;
                                
                // validate function pointer                    
                if (pfnSnmpExtensionTrap != NULL) {            

                    __try {

                        // loop until false is returned
                        while ((*pfnSnmpExtensionTrap)(
                                    &EnterpriseOid,
                                    &nGenericTrap,
                                    &nSpecificTrap,
                                    &nTimeStamp,
                                    &Vbl)) {
                                    
                            // send extension trap                                
                            GenerateExtensionTrap(
                                &EnterpriseOid,
                                nGenericTrap,
                                nSpecificTrap,
                                nTimeStamp,
                                &Vbl
                                );

                            SnmpUtilVarBindListFree(&Vbl);
                        }
                    
                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        
                        SNMPDBG((
                            SNMP_LOG_ERROR,
                            "SNMP: SVC: exception 0x%08lx polling %s.\n",
                            GetExceptionCode(),
                            ppNLEs[dwIndex]->pPathname
                            ));
                    }
                }
            }            
        }
    
        // release memory for wait objects
        UnloadWaitObjects(pWaitObjects, ppNLEs);
    }

    return fOk;
}


BOOL
GenerateTrap(
    PSNMP_PDU pPdu
    )

/*

Routine Description:

    Generates trap for agent.

Arguments:

    pPdu - pointer to initialized TRAP or TRAPv1 PDU.

Return Values:

    Returns true if successful.

*/

{
    BOOL fOk = TRUE;
    PLIST_ENTRY pLE1;
    PLIST_ENTRY pLE2;
    PLIST_ENTRY pLE3;
    PNETWORK_LIST_ENTRY pNLE;
    PMANAGER_LIST_ENTRY pMLE;
    PTRAP_DESTINATION_LIST_ENTRY pTLE;
    AsnOctetString CommunityOctets;
    DWORD dwStatus;
    DWORD dwIPAddr;

    // obtain first trap destination
    pLE1 = g_TrapDestinations.Flink;

    // process each trap destination
    while (pLE1 != &g_TrapDestinations) {

        // retrieve pointer to outgoing transport structure
        pTLE = CONTAINING_RECORD(pLE1, TRAP_DESTINATION_LIST_ENTRY, Link);

        // copy community string into octet structure
        CommunityOctets.length  = strlen(pTLE->pCommunity);
        CommunityOctets.stream  = pTLE->pCommunity;
        CommunityOctets.dynamic = FALSE;

        // obtain first manager
        pLE2 = pTLE->Managers.Flink;

        // process each receiving manager
        while (pLE2 != &pTLE->Managers) {

            // retrieve pointer to next manager
            pMLE = CONTAINING_RECORD(pLE2, MANAGER_LIST_ENTRY, Link);
            
			// refresh addr 
            UpdateMLE(pMLE);

            // don't send traps to addresses that are DEAD or NULL
			if (pMLE->dwAge == MGRADDR_DEAD || 
                !IsValidSockAddr(&pMLE->SockAddr))
			{
                pLE2 = pLE2->Flink;
				continue;
			}
        
            // obtain first outgoing transport
            pLE3 = g_OutgoingTransports.Flink;

            // process each outgoing transport
            while (pLE3 != &g_OutgoingTransports) {

                // retrieve pointer to outgoing transport structure
                pNLE = CONTAINING_RECORD(pLE3, NETWORK_LIST_ENTRY, Link);

                // initialize buffer length
                pNLE->Buffer.len = NLEBUFLEN;

                // can only send on same protocol
                if (pNLE->SockAddr.sa_family != pMLE->SockAddr.sa_family)
                {
                    pLE3 = pLE3->Flink;
                    continue;
                }

                // modify agent address
                if (pNLE->SockAddr.sa_family == AF_INET) 
				{

                    struct sockaddr_in * pSockAddrIn;
                    DWORD                szSockToBind;

					// see if the trap destination address is valid and if the
                    // card to use for sending the trap could be determined
                    if (WSAIoctl(pNLE->Socket,
                             SIO_ROUTING_INTERFACE_QUERY,
                             &pMLE->SockAddr,
                             sizeof(pMLE->SockAddr),
                             &pNLE->SockAddr,
                             sizeof(pNLE->SockAddr),
                             &szSockToBind,
                             NULL,
                             NULL) == SOCKET_ERROR)
                    {
                        SNMPDBG((
                            SNMP_LOG_ERROR,
                            "SNMP: SVC: cannot determine interface to use for trap destination %s [err=%d].\n",
                            inet_ntoa(((struct sockaddr_in *)&pMLE->SockAddr)->sin_addr),
                            WSAGetLastError()
                            ));
                        // if we can't determine on what interface the trap will be sent from, just bail.
                        pLE3 = pLE3->Flink;
                        continue;
                    }
                    
                    // obtain pointer to protocol specific structure
                    pSockAddrIn = (struct sockaddr_in * )&pNLE->SockAddr;

                    // copy agent address into temp buffer
                    dwIPAddr = pSockAddrIn->sin_addr.s_addr;

                    // initialize agent address structure
                    pPdu->Pdu.TrapPdu.AgentAddr.dynamic = FALSE;
                    pPdu->Pdu.TrapPdu.AgentAddr.stream  = (LPBYTE)&dwIPAddr;
                    pPdu->Pdu.TrapPdu.AgentAddr.length  = sizeof(dwIPAddr);

                } else {

                    // re-initialize agent address structure
                    pPdu->Pdu.TrapPdu.AgentAddr.dynamic = FALSE;
                    pPdu->Pdu.TrapPdu.AgentAddr.stream  = NULL;
                    pPdu->Pdu.TrapPdu.AgentAddr.length  = 0;
                }

                // build message
                if (BuildMessage(
                        SNMP_VERSION_1,
                        &CommunityOctets,
                        pPdu,
                        pNLE->Buffer.buf,
                        &pNLE->Buffer.len
                        )) {
                                
                    // synchronous send
                    dwStatus = WSASendTo(
                                  pNLE->Socket,
                                  &pNLE->Buffer,
                                  1,
                                  &pNLE->dwBytesTransferred,
                                  pNLE->dwFlags,
                                  &pMLE->SockAddr,
                                  pMLE->SockAddrLen,
                                  NULL,
                                  NULL
                                  );
                            
                    // register outgoing packet into the management structure
                    mgmtCTick(CsnmpOutPkts);
                    // retister outgoing trap into the management structure
                    mgmtCTick(CsnmpOutTraps);

                    // validate return code
                    if (dwStatus == SOCKET_ERROR) {
                        
                        SNMPDBG((
                            SNMP_LOG_ERROR,
                            "SNMP: SVC: error code %d on sending trap to %s.\n",
                            WSAGetLastError(),
                            pTLE->pCommunity
                            ));
                    }
                }    

                // next entry
                pLE3 = pLE3->Flink;
            }

            // next entry
            pLE2 = pLE2->Flink;
        }

        // next entry
        pLE1 = pLE1->Flink;                
    }

    return fOk;
}


BOOL
GenerateColdStartTrap(
    )

/*

Routine Description:

    Generates cold start trap.

Arguments:

    None.

Return Values:

    Returns true if successful.

*/

{
    // generate cold start
    return GenerateExtensionTrap(
                NULL,   // pEnterpriseOid
                SNMP_GENERICTRAP_COLDSTART,
                0,      // nSpecificTrapId
                0,      // nTimeStamp
                &g_NullVbl
                );    
}


BOOL
GenerateAuthenticationTrap(
    )

/*

Routine Description:

    Generates authentication trap.

Arguments:

    None.

Return Values:

    Returns true if successful.

*/

{
    // generate cold start
    return GenerateExtensionTrap(
                NULL,   // pEnterpriseOid
                SNMP_GENERICTRAP_AUTHFAILURE,
                0,      // nSpecificTrapId
                SnmpSvcGetUptime(),
                &g_NullVbl
                );    
}
