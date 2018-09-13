/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    query.c

Abstract:

    Contains routines for querying subagents.

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
#include "varbinds.h"
#include "network.h"
#include "query.h"
#include "snmpmgmt.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
FindQueryBySLE(
    PQUERY_LIST_ENTRY * ppQLE,
    PNETWORK_LIST_ENTRY pNLE,
    PSUBAGENT_LIST_ENTRY pSLE
    )

/*++

Routine Description:

    Allocates query list entry.

Arguments:

    ppQLE - pointer to receive query entry pointer.

    pNLE - pointer to network list entry.

    pSLE - pointer to subagent list entry.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE;
    PQUERY_LIST_ENTRY pQLE = NULL;

    // point to first query
    pLE = pNLE->Queries.Flink;

    // process each query
    while (pLE != &pNLE->Queries) {

        // retrieve pointer to query entry from link
        pQLE = CONTAINING_RECORD(pLE, QUERY_LIST_ENTRY, Link);

        // compare subagents
        if (pQLE->pSLE == pSLE) {

            // transfer
            *ppQLE = pQLE;

            // success
            return TRUE;
        }
            
        // next entry
        pLE = pLE->Flink;        
    }

    // initialize
    *ppQLE = NULL;

    // failure
    return FALSE;
}


BOOL
LoadSubagentData(
    PNETWORK_LIST_ENTRY pNLE,
    PQUERY_LIST_ENTRY   pQLE
    )

/*++

Routine Description:

    Loads data to be passed to the subagent DLL.

Arguments:

    pNLE - pointer to network list entry.

    pQLE - pointer to current query.

Return Values:

    Returns true if successful.

--*/

{   
    PLIST_ENTRY pLE;
    PVARBIND_LIST_ENTRY pVLE;
        
    SNMPDBG((
        SNMP_LOG_VERBOSE,
        "SNMP: SVC: query 0x%08lx loading.\n", pQLE
        ));    

    // attempt to allocate varbind list    
    pQLE->SubagentVbl.list = SnmpUtilMemAlloc(
                                pQLE->nSubagentVbs * sizeof(SnmpVarBind)
                                );

    // validate varbind list pointer
    if (pQLE->SubagentVbl.list != NULL) {

        // point to first varbind
        pLE = pQLE->SubagentVbs.Flink;

        // process each outgoing varbind
        while (pLE != &pQLE->SubagentVbs) {

            // retrieve pointer to varbind entry from query link
            pVLE = CONTAINING_RECORD(pLE, VARBIND_LIST_ENTRY, QueryLink);

            // transfer varbind
            SnmpUtilVarBindCpy(
                &pQLE->SubagentVbl.list[pQLE->SubagentVbl.len++],
                &pVLE->ResolvedVb
                );                    

            SNMPDBG((
                SNMP_LOG_VERBOSE,
                "SNMP: SVC: variable %d copied to query 0x%08lx.\n",
                pVLE->nErrorIndex,
                pQLE
                ));    

            // next entry
            pLE = pLE->Flink;        
        }

        // create copy of context in case subagent mucks with it
        SnmpUtilOctetsCpy(&pQLE->ContextInfo, &pNLE->Community);
    
    } else {
     
        SNMPDBG((   
            SNMP_LOG_ERROR,
            "SNMP: SVC: could not allocate varbind list.\n"
            ));
        
        // failure    
        return FALSE;
    }

    // success
    return TRUE;    
}


BOOL
UnloadSubagentData(
    PQUERY_LIST_ENTRY pQLE 
    )

/*++

Routine Description:

    Unloads data passed to the subagent DLL.

Arguments:

    pQLE - pointer to current query.

Return Values:

    Returns true if successful.

--*/

{   
    __try {
    
        // release subagent varbind list
        SnmpUtilVarBindListFree(&pQLE->SubagentVbl);

        // release subagent context information        
        SnmpUtilOctetsFree(&pQLE->ContextInfo);
    
    } __except (EXCEPTION_EXECUTE_HANDLER) {
                
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SVC: exception 0x%08lx processing structure from %s.\n",
            GetExceptionCode(),
            (pQLE->pSLE != NULL)
                ? pQLE->pSLE->pPathname 
                : "<unknown>"
            ));

        // failure
        return FALSE;    
    }

    // success
    return TRUE;
}


BOOL
UpdateResolvedVarBind(
    PQUERY_LIST_ENTRY   pQLE,
    PVARBIND_LIST_ENTRY pVLE,
    UINT                iVb
    )

/*++

Routine Description:

    Updates resolved varbind with data from subagent DLL.

Arguments:

    pQLE - pointer to current query.

    pVLE - pointer to varbind.

    iVb - index of varbind.

Return Values:

    Returns true if successful.

--*/

{
    // see if this is non-repeater
    if (pVLE->nMaxRepetitions == 1) {

        // flag varbind as resolved
        pVLE->nState = VARBIND_RESOLVED;

        // release memory for current varbind
        SnmpUtilVarBindFree(&pVLE->ResolvedVb);

        // transfer varbind from subagent
        SnmpUtilVarBindCpy(&pVLE->ResolvedVb, 
                           &pQLE->SubagentVbl.list[iVb]
                           );

        SNMPDBG((
            SNMP_LOG_VERBOSE,
            "SNMP: SVC: variable %d name %s.\n",
            pVLE->nErrorIndex,
            SnmpUtilOidToA(&pVLE->ResolvedVb.name)
            ));    

        SNMPDBG((
            SNMP_LOG_VERBOSE,
            "SNMP: SVC: variable %d state '%s'.\n",
            pVLE->nErrorIndex,
            VARBINDSTATESTRING(pVLE->nState)
            ));    

        // success
        return TRUE;
    } 
    
    // see if varbind list allocated
    if ((pVLE->ResolvedVbl.len == 0) &&
        (pVLE->ResolvedVbl.list == NULL)) {

        // allocate varbind list to fill in
        pVLE->ResolvedVbl.list = SnmpUtilMemAlloc(
                                    pVLE->nMaxRepetitions *
                                    sizeof(SnmpVarBind)
                                    );    
        // validate pointer before continuing
        if (pVLE->ResolvedVbl.list == NULL) {

            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: SVC: could not allocate new varbinds.\n"
                ));
    
            // failure
            return FALSE;
        }
    }

    // release working varbind name
    SnmpUtilOidFree(&pVLE->ResolvedVb.name);

    // transfer name for next iteration
    SnmpUtilOidCpy(&pVLE->ResolvedVb.name,
                   &pQLE->SubagentVbl.list[iVb].name 
                   ); 

    // transfer varbind
    SnmpUtilVarBindCpy( 
        &pVLE->ResolvedVbl.list[pVLE->ResolvedVbl.len],
        &pQLE->SubagentVbl.list[iVb]
        );

    // increment count
    pVLE->ResolvedVbl.len++;
                
    // see if this is the last varbind to retrieve
    pVLE->nState = (pVLE->nMaxRepetitions > pVLE->ResolvedVbl.len)
                        ? VARBIND_PARTIALLY_RESOLVED
                        : VARBIND_RESOLVED
                        ;            

    SNMPDBG((
        SNMP_LOG_VERBOSE,
        "SNMP: SVC: variable %d name %s.\n",
        pVLE->nErrorIndex,
        SnmpUtilOidToA(&pVLE->ResolvedVb.name)
        ));    

    SNMPDBG((
        SNMP_LOG_VERBOSE,
        "SNMP: SVC: variable %d state '%s'.\n",
        pVLE->nErrorIndex,
        VARBINDSTATESTRING(pVLE->nState)
        ));    

    // success
    return TRUE;
}


BOOL
UpdateVarBind(
    PNETWORK_LIST_ENTRY pNLE,
    PQUERY_LIST_ENTRY   pQLE,
    PVARBIND_LIST_ENTRY pVLE,
    UINT                iVb
    )

/*++

Routine Description:

    Updates varbind with data from subagent DLL.

Arguments:

    pNLE - pointer to network list entry.

    pQLE - pointer to current query.

    pVLE - pointer to varbind.

    iVb - index of varbind.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE;
    INT nDiff1;
    INT nDiff2;

    __try {
                        
        // determine order of returned varbind
        nDiff1 = SnmpUtilOidCmp(&pQLE->SubagentVbl.list[iVb].name,
                                &pVLE->ResolvedVb.name
                                );

        // see if this is getnext or getbulk 
        if ((pNLE->Pdu.nType == SNMP_PDU_GETNEXT) ||
            (pNLE->Pdu.nType == SNMP_PDU_GETBULK)) {
            
            // determine whether returned varbind in range
            nDiff2 = SnmpUtilOidCmp(&pQLE->SubagentVbl.list[iVb].name,
                                    &pVLE->pCurrentRLE->LimitOid
                                    );

            // make sure returned oid in range
            if ((nDiff1 > 0) && (nDiff2 < 0)) {

                // update resolved variable binding
                return UpdateResolvedVarBind(pQLE, pVLE, iVb);

            } else if (nDiff2 >= 0) {

                SNMPDBG((
                    SNMP_LOG_VERBOSE,
                    "SNMP: SVC: %s received getnext request for %s.\n",
                    pQLE->pSLE->pPathname,
                    SnmpUtilOidToA(&pVLE->ResolvedVb.name)
                    ));

                SNMPDBG((
                    SNMP_LOG_VERBOSE,
                    "SNMP: SVC: %s returned out-of-range oid %s.\n",
                    pQLE->pSLE->pPathname,
                    SnmpUtilOidToA(&pQLE->SubagentVbl.list[iVb].name)
                    ));
                
                // retrieve pointer to next region
                pLE = pVLE->pCurrentRLE->Link.Flink;

                // see if we exhausted regions
                if (pLE != &g_SupportedRegions) {

                    PMIB_REGION_LIST_ENTRY pNextRLE;

                    // retrieve pointer to mib region 
                    pNextRLE = CONTAINING_RECORD(pLE, 
                                                 MIB_REGION_LIST_ENTRY, 
                                                 Link
                                                 );
                                            
                    // see if next region supported by same subagent
                    if (pVLE->pCurrentRLE->pSLE == pNextRLE->pSLE) {

                        BOOL retCode;

                        SNMPDBG((
                            SNMP_LOG_TRACE,
                            "SNMP: SVC: next region also supported by %s.\n",
                            pVLE->pCurrentRLE->pSLE->pPathname
                            ));    

                        // update resolved variable binding
                        retCode = UpdateResolvedVarBind(pQLE, pVLE, iVb);
                        if (pQLE->SubagentVbl.list[iVb].value.asnType != ASN_NULL)
                        {
                            return retCode;
                        }
                    }

                    // point to next region 
                    pVLE->pCurrentRLE = pNextRLE;
                    
                    // change state to partially resolved
                    pVLE->nState = VARBIND_PARTIALLY_RESOLVED;

                    SNMPDBG((
                        SNMP_LOG_VERBOSE,
                        "SNMP: SVC: variable %d state '%s'.\n",
                        pVLE->nErrorIndex,
                        VARBINDSTATESTRING(pVLE->nState)
                        ));    

                    SNMPDBG((
                        SNMP_LOG_TRACE,
                        "SNMP: SVC: variable %d re-assigned to %s.\n",
                        pVLE->nErrorIndex,
                        pVLE->pCurrentRLE->pSLE->pPathname
                        ));    
                } 
                
                else if ((pVLE->ResolvedVbl.len == 0) &&
                    (pVLE->ResolvedVbl.list == NULL)) {

                    // flag varbind as resolved
                    pVLE->nState = VARBIND_RESOLVED;

                    // set default varbind to eomv
                    pVLE->ResolvedVb.value.asnType = 
                        SNMP_EXCEPTION_ENDOFMIBVIEW;
                        
                    // update error status counter for the operation
                    mgmtCTick(CsnmpOutNoSuchNames);

                    SNMPDBG((
                        SNMP_LOG_VERBOSE,
                        "SNMP: SVC: variable %d state '%s'.\n",
                        pVLE->nErrorIndex,
                        VARBINDSTATESTRING(pVLE->nState)
                        ));    

                } else {                
                
                    // flag varbind as resolved
                    pVLE->nState = VARBIND_RESOLVED;

                    // transfer name 
                    SnmpUtilOidCpy(
                        &pVLE->ResolvedVbl.list[pVLE->ResolvedVbl.len].name,
                        &pVLE->ResolvedVb.name
                        );

                    // set current varbind to eomv
                    pVLE->ResolvedVbl.list[pVLE->ResolvedVbl.len].value.asnType =
                        SNMP_EXCEPTION_ENDOFMIBVIEW;

                    // increment count
                    pVLE->ResolvedVbl.len++;

                    // update error status counter for the operation
                    mgmtCTick(CsnmpOutNoSuchNames);

                    SNMPDBG((
                        SNMP_LOG_VERBOSE,
                        "SNMP: SVC: variable %d state '%s'.\n",
                        pVLE->nErrorIndex,
                        VARBINDSTATESTRING(pVLE->nState)
                        ));    
                }                                                

            } else {

                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "SNMP: SVC: %s received getnext request for %s.\n",
                    pQLE->pSLE->pPathname,
                    SnmpUtilOidToA(&pVLE->ResolvedVb.name)
                    ));

                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "SNMP: SVC: %s returned invalid oid %s.\n",
                    pQLE->pSLE->pPathname,
                    SnmpUtilOidToA(&pQLE->SubagentVbl.list[iVb].name)
                    ));

                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "SNMP: SVC: Ban %s subagent, forward the request to a different one.\n",
                    pQLE->pSLE->pPathname
                    ));

                // trying to forward this getnext request to the next region but only if not handled
                // by the same subagent!
                pLE = pVLE->pCurrentRLE->Link.Flink;
                while( pLE != &g_SupportedRegions)
                {
                    PMIB_REGION_LIST_ENTRY pNextRLE;

                   // retrieve pointer to mib region 
                    pNextRLE = CONTAINING_RECORD(pLE, 
                                                 MIB_REGION_LIST_ENTRY, 
                                                 Link
                                                 );

                    // if this 'next' region is handled by the same subagent, skip to the next!
                    if (pVLE->pCurrentRLE->pSLE == pNextRLE->pSLE)
                    {
                        pLE = pNextRLE->Link.Flink;
                        continue;
                    }

                    // ok, we have one, forward the original GetNext request to it
                    pVLE->pCurrentRLE = pNextRLE;
                    pVLE->nState = VARBIND_PARTIALLY_RESOLVED;

                    SNMPDBG((
                        SNMP_LOG_TRACE,
                        "SNMP: SVC: variable %d re-assigned to %s.\n",
                        pVLE->nErrorIndex,
                        pVLE->pCurrentRLE->pSLE->pPathname
                        ));
                    
                    return TRUE;
                }

                // failure
                // here I should emulate a (NO_SUCH_NAME) EndOfMibView, resolve the variable and return TRUE.
                pVLE->nState = VARBIND_RESOLVED;
                pVLE->ResolvedVb.value.asnType = SNMP_EXCEPTION_ENDOFMIBVIEW;
                pVLE->pCurrentRLE = NULL;

                // update error status counter
                mgmtCTick(CsnmpOutNoSuchNames);

                return TRUE;
            }

        } else if (pNLE->Pdu.nType == SNMP_PDU_GET) {

            // must match
            if (nDiff1 == 0) {

                // flag varbind as resolved
                pVLE->nState = VARBIND_RESOLVED;

                // release memory for current varbind
                SnmpUtilVarBindFree(&pVLE->ResolvedVb);

                // transfer varbind from subagent
                SnmpUtilVarBindCpy(&pVLE->ResolvedVb, 
                                   &pQLE->SubagentVbl.list[iVb]
                                   );

                SNMPDBG((
                    SNMP_LOG_VERBOSE,
                    "SNMP: SVC: variable %d name %s.\n",
                    pVLE->nErrorIndex,
                    SnmpUtilOidToA(&pVLE->ResolvedVb.name)
                    ));    

                SNMPDBG((
                    SNMP_LOG_VERBOSE,
                    "SNMP: SVC: variable %d state '%s'.\n",
                    pVLE->nErrorIndex,
                    VARBINDSTATESTRING(pVLE->nState)
                    ));    

            } else {

                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "SNMP: SVC: %s received get request for %s.\n",
                    pQLE->pSLE->pPathname,
                    SnmpUtilOidToA(&pVLE->ResolvedVb.name)
                    ));

                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "SNMP: SVC: %s returned invalid oid %s.\n",
                    pQLE->pSLE->pPathname,
                    SnmpUtilOidToA(&pQLE->SubagentVbl.list[iVb].name)
                    ));
                
                // failure
                return FALSE;
            }

        } else if (nDiff1 != 0) { 
			// set request failed -> invalid oid            
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: SVC: %s received set request for %s.\n",
                pQLE->pSLE->pPathname,
                SnmpUtilOidToA(&pVLE->ResolvedVb.name)
                ));

            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: SVC: %s returned invalid oid %s.\n",
                pQLE->pSLE->pPathname,
                SnmpUtilOidToA(&pQLE->SubagentVbl.list[iVb].name)
                ));
            
            // failure
            return FALSE;
        } else {

            // set request, oids match
			// WARNING!! - state might be set prematurely on SET_TEST / SET_CLEANUP
            pVLE->nState = VARBIND_RESOLVED;
			return TRUE;
		}

    } __except (EXCEPTION_EXECUTE_HANDLER) {
                
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SVC: exception 0x%08lx processing structure from %s.\n",
            GetExceptionCode(),
            pQLE->pSLE->pPathname
            ));

        // failure
        return FALSE;        
    }

    // success
    return TRUE;   
}


BOOL
UpdateVarBinds(
    PNETWORK_LIST_ENTRY pNLE,
    PQUERY_LIST_ENTRY   pQLE 
    )

/*++

Routine Description:

    Updates varbind list entries with data from subagent DLL.

Arguments:

    pNLE - pointer to network list entry.

    pQLE - pointer to current query.

Return Values:

    Returns true if successful.

--*/

{   
    PLIST_ENTRY pLE;
    PVARBIND_LIST_ENTRY pVLE;
    BOOL fOk = TRUE;
    UINT iVb = 0;
            
    // point to first varbind
    pLE = pQLE->SubagentVbs.Flink;

    // see if error encountered during callback
    if (pQLE->nErrorStatus == SNMP_ERRORSTATUS_NOERROR) {
    
        // process each outgoing varbind
        while (pLE != &pQLE->SubagentVbs) {

            // retrieve pointer to varbind entry from query link
            pVLE = CONTAINING_RECORD(pLE, VARBIND_LIST_ENTRY, QueryLink);
            
            // update individual varbind
            if (!UpdateVarBind(pNLE, pQLE, pVLE, iVb++)) {
                
                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "SNMP: SVC: variable %d could not be updated.\n", 
                    pQLE->nErrorIndex
                    ));    

                // update pdu with the proper varbind error index    
                pNLE->Pdu.Pdu.NormPdu.nErrorStatus = SNMP_ERRORSTATUS_GENERR;
                pNLE->Pdu.Pdu.NormPdu.nErrorIndex  = pVLE->nErrorIndex;

                // failure
                return FALSE;
            }

            // next entry
            pLE = pLE->Flink; 
        }
    
    } else {

        SNMPDBG((
            SNMP_LOG_VERBOSE,
            "SNMP: SVC: searching for errant variable.\n" 
            ));    

        // update pdu with status returned from subagent
        pNLE->Pdu.Pdu.NormPdu.nErrorStatus = pQLE->nErrorStatus;

        // process each outgoing varbind
        while (pLE != &pQLE->SubagentVbs) {

            // retrieve pointer to varbind entry from query link
            pVLE = CONTAINING_RECORD(pLE, VARBIND_LIST_ENTRY, QueryLink);
            
            // see if errant varbind nErrorIndex is starts from 1 !!
            if (pQLE->nErrorIndex == ++iVb) {
                
                SNMPDBG((
                    SNMP_LOG_TRACE,
                    "SNMP: SVC: variable %d involved in failure.\n",
                    pVLE->nErrorIndex
                    ));    

                // update pdu with the proper varbind error index    
                pNLE->Pdu.Pdu.NormPdu.nErrorIndex = pVLE->nErrorIndex;

                // the error code was successfully identified
                return	TRUE;
            }

            // next entry
            pLE = pLE->Flink; 
        }
        
        SNMPDBG((
            SNMP_LOG_TRACE,
            "SNMP: SVC: no variable involved in failure.\n"
            ));    

        // failure
        return FALSE;
    }

    // success
    return TRUE;
}


BOOL
CallSubagent(
    PQUERY_LIST_ENTRY pQLE,
    UINT              nRequestType,
    UINT              nTransactionId
    )

/*++

Routine Description:
    Invokes method from subagent DLL.

Arguments:

    pNLE - pointer to network list entry.

    nRequestType - type of request to post to subagent.

    nTransactionId - identifies snmp pdu sent from manager.

Return Values:

    Returns true if successful.

--*/

{   
    BOOL fOk = FALSE;

    SNMPDBG((
        SNMP_LOG_VERBOSE,
        "SNMP: SVC: --- query %s begin ---\n", 
        pQLE->pSLE->pPathname
        ));    

    __try {
        
        // determine which version of query supported
        if (pQLE->pSLE->pfnSnmpExtensionQueryEx != NULL) {

            // process query using new interface
            fOk = (*pQLE->pSLE->pfnSnmpExtensionQueryEx)(
                        nRequestType,
                        nTransactionId,
                        &pQLE->SubagentVbl,
                        &pQLE->ContextInfo,
                        &pQLE->nErrorStatus,
                        &pQLE->nErrorIndex
                        );
                                                
        // see if query is actually valid for downlevel call
        } else if ((pQLE->pSLE->pfnSnmpExtensionQuery != NULL) &&
                  ((nRequestType == SNMP_EXTENSION_GET) ||
                   (nRequestType == SNMP_EXTENSION_GET_NEXT) ||
                   (nRequestType == SNMP_EXTENSION_SET_COMMIT))) {
            
            // process query using old interface
            fOk = (*pQLE->pSLE->pfnSnmpExtensionQuery)(
                        (BYTE)(UINT)nRequestType,
                        &pQLE->SubagentVbl,
                        &pQLE->nErrorStatus,
                        &pQLE->nErrorIndex
                        );

        // see if query can be completed successfully anyway
        } else if ((nRequestType == SNMP_EXTENSION_SET_TEST) ||
                   (nRequestType == SNMP_EXTENSION_SET_CLEANUP)) { 

            // fake it
            fOk = TRUE;    
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SVC: exception 0x%08lx calling %s.\n",
            GetExceptionCode(),
            pQLE->pSLE->pPathname
            ));
    }

    SNMPDBG((
        SNMP_LOG_VERBOSE,
        "SNMP: SVC: --- query %s end ---\n", 
        pQLE->pSLE->pPathname
        ));    

    // validate
    if (!fOk) {
    
        // identify failing subagent
        pQLE->nErrorStatus = SNMP_ERRORSTATUS_GENERR;
        pQLE->nErrorIndex  = 1; 

    } else if (pQLE->nErrorStatus != SNMP_ERRORSTATUS_NOERROR) {

        // see if error index needs to be adjusted
        if ((pQLE->nErrorIndex > pQLE->nSubagentVbs) ||
            (pQLE->nErrorIndex == 0)) {

            // set to first varbind
            pQLE->nErrorIndex = 1; 
        }
    
    } else {

        // re-initialize
        pQLE->nErrorIndex = 0; 
    }

    SNMPDBG((
        SNMP_LOG_VERBOSE,
        "SNMP: SVC: query 0x%08lx %s, errorStatus=%s, errorIndex=%d.\n", 
        pQLE,
        (pQLE->nErrorStatus == SNMP_ERRORSTATUS_NOERROR)
            ? "succeeded"
            : "failed"
            ,
        SNMPERRORSTRING(pQLE->nErrorStatus),
        pQLE->nErrorIndex
        ));    

    return TRUE;
}


BOOL
ProcessSet(
    PNETWORK_LIST_ENTRY pNLE
    )

/*++

Routine Description:

    Processes SNMP_PDU_SET requests.

Arguments:

    pNLE - pointer to network list entry.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE = NULL;
    PQUERY_LIST_ENTRY pQLE;
    BOOL fOk = TRUE;

    // load subagent queries
    if (!LoadQueries(pNLE)) {

        // unload immediately
        UnloadQueries(pNLE);

        // failure
        return FALSE;
    }
    
    // point to first query
    pLE = pNLE->Queries.Flink;

    // process each subagent query 
    while (fOk && (pLE != &pNLE->Queries)) {

        // retrieve pointer to query entry from link
        pQLE = CONTAINING_RECORD(pLE, QUERY_LIST_ENTRY, Link);

        // load outgoing varbinds
        fOk = LoadSubagentData(pNLE, pQLE);

        // validate
        if (fOk) {

            // dispatch 
            CallSubagent(
                pQLE, 
                SNMP_EXTENSION_SET_TEST,
                pNLE->nTransactionId
                );

            // process results returned
            fOk = UpdateVarBinds(pNLE, pQLE);
        }

        // next entry (or reverse direction)
        pLE = fOk ? pLE->Flink : pLE->Blink;
    }
    
    // validate
    if (fOk) {

		// if this line is missing => GenErr on UpdatePdu()
		pLE = pNLE->Queries.Flink;

        // process each subagent query 
        while (fOk && (pLE != &pNLE->Queries)) {

            // retrieve pointer to query entry from link
            pQLE = CONTAINING_RECORD(pLE, QUERY_LIST_ENTRY, Link);

            // dispatch 
            CallSubagent(
                pQLE, 
                SNMP_EXTENSION_SET_COMMIT,
                pNLE->nTransactionId
                );

            // process results returned
            fOk = UpdateVarBinds(pNLE, pQLE);

            // next entry (or reverse direction)
            pLE = fOk ? pLE->Flink : pLE->Blink;
        }

        // validate
        if (!fOk) {

            // process each subagent query 
            while (pLE != &pNLE->Queries) {

                // retrieve pointer to query entry from link
                pQLE = CONTAINING_RECORD(pLE, QUERY_LIST_ENTRY, Link);

                // dispatch 
                CallSubagent(
                    pQLE, 
                    SNMP_EXTENSION_SET_UNDO,
                    pNLE->nTransactionId
                    );

                // process results returned
                UpdateVarBinds(pNLE, pQLE);

                // previous entry 
                pLE = pLE->Blink;
            }
        }

        // point to last query
        pLE = pNLE->Queries.Blink;
    }
        
    // process each subagent query 
    while (pLE != &pNLE->Queries) {

        // retrieve pointer to query entry from link
        pQLE = CONTAINING_RECORD(pLE, QUERY_LIST_ENTRY, Link);

        // dispatch 
        CallSubagent(
            pQLE, 
            SNMP_EXTENSION_SET_CLEANUP,
            pNLE->nTransactionId
            );

        // process results returned
        UpdateVarBinds(pNLE, pQLE);

        // previous entry 
        pLE = pLE->Blink;
    }

    // cleanup queries
    UnloadQueries(pNLE);

    return TRUE;
}


BOOL
ProcessGet(
    PNETWORK_LIST_ENTRY pNLE
    )

/*++

Routine Description:

    Queries subagents to resolve varbinds.

Arguments:

    pNLE - pointer to network list entry.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE;
    PQUERY_LIST_ENTRY pQLE = NULL;
    BOOL fOk = TRUE;

    // load subagent queries
    if (!LoadQueries(pNLE)) {

        // unload immediately
        UnloadQueries(pNLE);

        // failure
        return FALSE;
    }
        
    // point to first query
    pLE = pNLE->Queries.Flink;

    // process each subagent query 
    while (fOk && (pLE != &pNLE->Queries)) {

        // retrieve pointer to query entry from link
        pQLE = CONTAINING_RECORD(pLE, QUERY_LIST_ENTRY, Link);

        // load outgoing varbinds
        fOk = LoadSubagentData(pNLE, pQLE);

        // validate
        if (fOk) {

            // dispatch 
            CallSubagent(
                pQLE, 
                SNMP_EXTENSION_GET,
                pNLE->nTransactionId
                );

            // process results returned
            fOk = UpdateVarBinds(pNLE, pQLE);
        }

        // next entry
        pLE = pLE->Flink;        
    }
    
    // cleanup queries
    UnloadQueries(pNLE);

    return fOk;
}


BOOL
ProcessGetBulk(
    PNETWORK_LIST_ENTRY pNLE
    )

/*++

Routine Description:

    Queries subagents to resolve varbinds.

Arguments:

    pNLE - pointer to network list entry.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE;
    PQUERY_LIST_ENTRY pQLE = NULL;
    BOOL fOk = TRUE;

    SNMPDBG((
        SNMP_LOG_VERBOSE,
        "SNMP: SVC: getbulk request, non-repeaters %d, max-repetitions %d.\n",
        pNLE->Pdu.Pdu.BulkPdu.nNonRepeaters,
        pNLE->Pdu.Pdu.BulkPdu.nMaxRepetitions
        ));

    // loop
    while (fOk) {

        // load subagent queries
        fOk = LoadQueries(pNLE);

        // validate
        if (fOk && !IsListEmpty(&pNLE->Queries)) {

            // point to first query
            pLE = pNLE->Queries.Flink;

            // process each subagent query 
            while (fOk && (pLE != &pNLE->Queries)) {

                // retrieve pointer to query entry from link
                pQLE = CONTAINING_RECORD(pLE, QUERY_LIST_ENTRY, Link);

                // load outgoing varbinds
                fOk = LoadSubagentData(pNLE, pQLE);

                // validate
                if (fOk) {

                    // dispatch 
                    CallSubagent(
                        pQLE, 
                        SNMP_EXTENSION_GET_NEXT,
                        pNLE->nTransactionId
                        );

                    // process results returned
                    fOk = UpdateVarBinds(pNLE, pQLE);
                }

                // next entry
                pLE = pLE->Flink;        
            }

        } else if (IsListEmpty(&pNLE->Queries)) {

            SNMPDBG((
                SNMP_LOG_VERBOSE,
                "SNMP: SVC: no more queries to process.\n"
                ));

            break; // finished...
        }

        // cleanup queries
        UnloadQueries(pNLE);
    }

    return fOk;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
AllocQLE(
    PQUERY_LIST_ENTRY * ppQLE
    )

/*++

Routine Description:

    Allocates query list entry.

Arguments:

    ppQLE - pointer to receive list entry pointer.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;
    PQUERY_LIST_ENTRY pQLE = NULL;

    // attempt to allocate structure
    pQLE = AgentMemAlloc(sizeof(QUERY_LIST_ENTRY));

    // validate
    if (pQLE != NULL) {

        // initialize outgoing varbind list
        InitializeListHead(&pQLE->SubagentVbs);        

        // success
        fOk = TRUE;
    
    } else {
        
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SVC: could not allocate query.\n"
            ));
    }    

    // transfer
    *ppQLE = pQLE;

    return fOk;
}


BOOL
FreeQLE(
    PQUERY_LIST_ENTRY pQLE
    )

/*++

Routine Description:

    Creates queries from varbind list entries.

Arguments:

    pNLE - pointer to network list entry with SNMP message.

Return Values:

    Returns true if successful.

--*/

{
    // validate pointer
    if (pQLE != NULL) {

        // release subagent info
        UnloadSubagentData(pQLE);

        // release structure
        AgentMemFree(pQLE);
    }

    return TRUE;
}


BOOL
LoadQueries(
    PNETWORK_LIST_ENTRY pNLE
    )

/*++

Routine Description:

    Creates queries from varbind list entries.

Arguments:

    pNLE - pointer to network list entry.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE;
    PVARBIND_LIST_ENTRY pVLE;
    PQUERY_LIST_ENTRY pQLE = NULL;

    // point to first varbind
    pLE = pNLE->Bindings.Flink;

    // process each binding
    while (pLE != &pNLE->Bindings) {

        // retrieve pointer to varbind entry from link
        pVLE = CONTAINING_RECORD(pLE, VARBIND_LIST_ENTRY, Link);

        // analyze current state of varbind        
        if ((pVLE->nState == VARBIND_INITIALIZED) ||
            (pVLE->nState == VARBIND_PARTIALLY_RESOLVED)) {

            // attempt to locate existing query
            if (FindQueryBySLE(&pQLE, pNLE, pVLE->pCurrentRLE->pSLE)) {

                // attach varbind entry to query via query link
                InsertTailList(&pQLE->SubagentVbs, &pVLE->QueryLink);

                // change varbind state
                pVLE->nState = VARBIND_RESOLVING;

                // increment total
                pQLE->nSubagentVbs++;

                SNMPDBG((
                    SNMP_LOG_VERBOSE,
                    "SNMP: SVC: variable %d added to existing query 0x%08lx.\n",
                    pVLE->nErrorIndex,
                    pQLE
                    ));    

                SNMPDBG((
                    SNMP_LOG_VERBOSE,
                    "SNMP: SVC: variable %d state '%s'.\n",
                    pVLE->nErrorIndex,
                    VARBINDSTATESTRING(pVLE->nState)
                    ));    

            // attempt to allocate entry
            } else if (AllocQLE(&pQLE)) {
                
                // obtain subagent pointer
                pQLE->pSLE = pVLE->pCurrentRLE->pSLE;

                // insert into query list 
                InsertTailList(&pNLE->Queries, &pQLE->Link);

                // attach varbind entry to query via query link
                InsertTailList(&pQLE->SubagentVbs, &pVLE->QueryLink);

                // change varbind state
                pVLE->nState = VARBIND_RESOLVING;

                // increment total
                pQLE->nSubagentVbs++;

                SNMPDBG((
                    SNMP_LOG_VERBOSE,
                    "SNMP: SVC: variable %d added to new query 0x%08lx.\n",
                    pVLE->nErrorIndex,
                    pQLE
                    ));    

                SNMPDBG((
                    SNMP_LOG_VERBOSE,
                    "SNMP: SVC: variable %d state '%s'.\n",
                    pVLE->nErrorIndex,
                    VARBINDSTATESTRING(pVLE->nState)
                    ));    

            } else {
                
                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "SNMP: SVC: could not contruct query.\n"
                    ));

                // failure
                return FALSE;
            }
        }

        // next entry
        pLE = pLE->Flink;
    }
    
    // success    
    return TRUE;
}


BOOL
UnloadQueries(
    PNETWORK_LIST_ENTRY pNLE
    )

/*++

Routine Description:

    Destroys queries from varbind list entries.

Arguments:

    pNLE - pointer to network list entry with SNMP message.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE;
    PQUERY_LIST_ENTRY pQLE;
    
    // process each query entry
    while (!IsListEmpty(&pNLE->Queries)) {

        // point to first query
        pLE = RemoveHeadList(&pNLE->Queries);

        // retrieve pointer to query from link
        pQLE = CONTAINING_RECORD(pLE, QUERY_LIST_ENTRY, Link);

        // release
        FreeQLE(pQLE);
    }

    return TRUE;
}


BOOL
ProcessQueries(
    PNETWORK_LIST_ENTRY pNLE
    )

/*++

Routine Description:

    Queries subagents to resolve varbinds.

Arguments:

    pNLE - pointer to network list entry.

Return Values:

    Returns true if successful.

--*/

{
    // determine pdu
    switch (pNLE->Pdu.nType) {

    case SNMP_PDU_GETNEXT:
    case SNMP_PDU_GETBULK:
        
        // multiple non-exact reads
        return ProcessGetBulk(pNLE);

    case SNMP_PDU_GET:
    
        // single exact read
        return ProcessGet(pNLE);

    case SNMP_PDU_SET:

        // single exact write
        return ProcessSet(pNLE);
    }                

    // failure
    return FALSE;
}
