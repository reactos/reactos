/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    varbinds.c

Abstract:

    Contains routines for manipulating varbinds.

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
#include "query.h"
#include "snmpmgmt.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
LoadVarBind(
    PNETWORK_LIST_ENTRY pNLE,
    UINT                iVb
    )

/*++

Routine Description:

    Creates varbind list entry from varbind structure.

Arguments:

    pNLE - pointer to network list entry.

    iVb - index of variable binding.

Return Values:

    Returns true if successful.

--*/

{
    SnmpVarBind * pVb;
    PVARBIND_LIST_ENTRY pVLE = NULL;
    PMIB_REGION_LIST_ENTRY pRLE = NULL;
    BOOL fAnyOk;
    BOOL fOk;

    // allocate list entry
    if (fOk = AllocVLE(&pVLE)) {

        // save varbind list index
        pVLE->nErrorIndex = iVb + 1;    

        // retrieve varbind pointer
        pVb = &pNLE->Pdu.Vbl.list[iVb];

        SNMPDBG((
            SNMP_LOG_TRACE,
            "SNMP: SVC: variable %d name %s.\n",
            pVLE->nErrorIndex,
            SnmpUtilOidToA(&pVb->name)
            ));    

        // initialize type of resolved variable
        pVLE->ResolvedVb.value.asnType = ASN_NULL;

        // copy varbind name to working structure
        SnmpUtilOidCpy(&pVLE->ResolvedVb.name, &pVb->name);

        // see if specific object asked for
        fAnyOk = ((pNLE->Pdu.nType == SNMP_PDU_GETNEXT) ||
                  (pNLE->Pdu.nType == SNMP_PDU_GETBULK));

        // attempt to lookup variable name in supported regions
        if (FindSupportedRegion(&pRLE, &pVb->name, fAnyOk)) {

            // save pointer to region
            pVLE->pCurrentRLE = pRLE;

            // structure has been initialized
            pVLE->nState = VARBIND_INITIALIZED;

            SNMPDBG((
                SNMP_LOG_TRACE,
                "SNMP: SVC: variable %d assigned to %s.\n",
                pVLE->nErrorIndex,
                pVLE->pCurrentRLE->pSLE->pPathname
                ));    

            SNMPDBG((
                SNMP_LOG_VERBOSE,
                "SNMP: SVC: variable %d state '%s'.\n",
                pVLE->nErrorIndex,
                VARBINDSTATESTRING(pVLE->nState)
                ));    

            // see if this is a getnext request
            if (pNLE->Pdu.nType == SNMP_PDU_GETNEXT) {    

                // only need single rep
                pVLE->nMaxRepetitions = 1;

            } else if (pNLE->Pdu.nType == SNMP_PDU_GETBULK) {

                // see if this non-repeater which is in bounds
                if (pNLE->Pdu.Pdu.BulkPdu.nNonRepeaters > (int)iVb) {

                    // only need single rep
                    pVLE->nMaxRepetitions = 1;

                    SNMPDBG((
                        SNMP_LOG_VERBOSE,
                        "SNMP: SVC: variable %d max repetitions %d.\n",
                        pVLE->nErrorIndex,
                        pVLE->nMaxRepetitions
                        ));    

                // see if max-repetitions is non-zero
                } else if (pNLE->Pdu.Pdu.BulkPdu.nMaxRepetitions > 0) {

                    // set max-repetitions to value in getbulk pdu
                    pVLE->nMaxRepetitions = pNLE->Pdu.Pdu.BulkPdu.nMaxRepetitions;

                    SNMPDBG((
                        SNMP_LOG_VERBOSE,
                        "SNMP: SVC: variable %d max repetitions %d.\n",
                        pVLE->nErrorIndex,
                        pVLE->nMaxRepetitions
                        ));    

                } else {

                    // modify state to resolved 
                    pVLE->nState = VARBIND_RESOLVED;

                    SNMPDBG((
                        SNMP_LOG_VERBOSE,
                        "SNMP: SVC: variable %d state '%s'.\n",
                        pVLE->nErrorIndex,
                        VARBINDSTATESTRING(pVLE->nState)
                        ));    

                    SNMPDBG((
                        SNMP_LOG_VERBOSE,
                        "SNMP: SVC: variable %d value NULL.\n",
                        pVLE->nErrorIndex
                        ));    
                }

            } else if (pNLE->Pdu.nType == SNMP_PDU_SET) {

                // copy varbind value to working structure
                SnmpUtilAsnAnyCpy(&pVLE->ResolvedVb.value, &pVb->value);

                SNMPDBG((
                    SNMP_LOG_VERBOSE,
                    "SNMP: SVC: variable %d value %s.\n",
                    pVLE->nErrorIndex,
                    "<TBD>"
                    ));    
            }
    
        } else {

            // null pointer to region
            pVLE->pCurrentRLE = NULL;
            
            SNMPDBG((
                SNMP_LOG_TRACE,
                "SNMP: SVC: variable %d unable to be assigned.\n",
                pVLE->nErrorIndex
                ));    

            // getbulk            
            if (fAnyOk) {

                // modify state to resolved 
                pVLE->nState = VARBIND_RESOLVED;

                // set the exception in the variable's type field
                pVLE->ResolvedVb.value.asnType = SNMP_EXCEPTION_ENDOFMIBVIEW;

                SNMPDBG((
                    SNMP_LOG_VERBOSE,
                    "SNMP: SVC: variable %d state '%s'.\n",
                    pVLE->nErrorIndex,
                    VARBINDSTATESTRING(pVLE->nState)
                    ));    

                SNMPDBG((
                    SNMP_LOG_VERBOSE,
                    "SNMP: SVC: variable %d value ENDOFMIBVIEW.\n",
                    pVLE->nErrorIndex
                    ));    

            } else if (pNLE->Pdu.nType == SNMP_PDU_GET) {

                // modify state to resolved 
                pVLE->nState = VARBIND_RESOLVED;

                // set the exception in the variable's type field
                pVLE->ResolvedVb.value.asnType = SNMP_EXCEPTION_NOSUCHOBJECT;

                SNMPDBG((
                    SNMP_LOG_VERBOSE,
                    "SNMP: SVC: variable %d state '%s'.\n",
                    pVLE->nErrorIndex,
                    VARBINDSTATESTRING(pVLE->nState)
                    ));    

                SNMPDBG((
                    SNMP_LOG_VERBOSE,
                    "SNMP: SVC: variable %d value NOSUCHOBJECT.\n",
                    pVLE->nErrorIndex
                    ));    

            } else {

                // modify state to resolved
                //pVLE->nState = VARBIND_ABORTED;
                pVLE->nState = VARBIND_RESOLVED;

                // save error status in network list entry
                pNLE->Pdu.Pdu.NormPdu.nErrorStatus = SNMP_ERRORSTATUS_NOTWRITABLE;
                pNLE->Pdu.Pdu.NormPdu.nErrorIndex  = pVLE->nErrorIndex;

                SNMPDBG((
                    SNMP_LOG_VERBOSE,
                    "SNMP: SVC: variable %d state '%s'.\n",
                    pVLE->nErrorIndex,
                    VARBINDSTATESTRING(pVLE->nState)
                    ));    

                SNMPDBG((
                    SNMP_LOG_VERBOSE,
                    "SNMP: SVC: variable %d error %s.\n",
                    pVLE->nErrorIndex,
                    SNMPERRORSTRING(pNLE->Pdu.Pdu.NormPdu.nErrorStatus)
                    ));    
                
                // failure
                //fOk = FALSE;
            }
        }

        // add to existing varbind list
        InsertTailList(&pNLE->Bindings, &pVLE->Link);
    }
    
    return fOk;
}


BOOL
LoadVarBinds(
    PNETWORK_LIST_ENTRY pNLE
    )

/*++

Routine Description:

    Creates list of varbind entries from varbind structure.

Arguments:

    pNLE - pointer to network list entry.

Return Values:

    Returns true if successful.

--*/

{
    UINT iVb;
    BOOL fOk = TRUE;
    
    // process each varbind in list
    for (iVb = 0; (fOk && (iVb < pNLE->Pdu.Vbl.len)); iVb++) {

        // load individual varbind
        fOk = LoadVarBind(pNLE, iVb);
    }

    return fOk;
}


BOOL
UnloadVarBinds(
    PNETWORK_LIST_ENTRY pNLE
    )

/*++

Routine Description:

    Destroys list of varbind entries.

Arguments:

    pNLE - pointer to network list entry.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE;
    PVARBIND_LIST_ENTRY pVLE;

    // process each varbind entry
    while (!IsListEmpty(&pNLE->Bindings)) {

        // point to first varbind
        pLE = RemoveHeadList(&pNLE->Bindings);

        // retrieve pointer to varbind entry from link
        pVLE = CONTAINING_RECORD(pLE, VARBIND_LIST_ENTRY, Link);

        // release
        FreeVLE(pVLE);
    }    

    return TRUE;
}


BOOL
ValidateVarBinds(
    PNETWORK_LIST_ENTRY pNLE
    )

/*++

Routine Description:

    Updates error status based on query results and version.

Arguments:

    pNLE - pointer to network list entry.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE;
    PVARBIND_LIST_ENTRY pVLE;

    // see if error has already report during processing
    if (pNLE->Pdu.Pdu.NormPdu.nErrorStatus == SNMP_ERRORSTATUS_NOERROR) {
        
        // point to first varbind
        pLE = pNLE->Bindings.Flink;

        // process each varbind entry
        while (pLE != &pNLE->Bindings) {

            // retrieve pointer to varbind entry from link
            pVLE = CONTAINING_RECORD(pLE, VARBIND_LIST_ENTRY, Link);

            // see if varbind has been resolved
            if (pVLE->nState != VARBIND_RESOLVED) {       

                SNMPDBG((
                    SNMP_LOG_WARNING,
                    "SNMP: SVC: variable %d unresolved.\n",
                    pVLE->nErrorIndex
                    ));

                // report internal error has occurred
                pNLE->Pdu.Pdu.NormPdu.nErrorStatus = SNMP_ERRORSTATUS_GENERR;
                pNLE->Pdu.Pdu.NormPdu.nErrorIndex  = pVLE->nErrorIndex;

                break; // bail...
            
            } else if (pNLE->nVersion == SNMP_VERSION_1) {
        
                // report error if exceptions are present instead of values
                if ((pVLE->ResolvedVb.value.asnType == SNMP_EXCEPTION_NOSUCHOBJECT) ||
                    (pVLE->ResolvedVb.value.asnType == SNMP_EXCEPTION_NOSUCHINSTANCE) ||
                    (pVLE->ResolvedVb.value.asnType == SNMP_EXCEPTION_ENDOFMIBVIEW)) {
                        
                    SNMPDBG((
                        SNMP_LOG_WARNING,
                        "SNMP: SVC: variable %d unresolved in SNMPv1.\n",
                        pVLE->nErrorIndex
                        ));

                    // report that variable could not be found
                    pNLE->Pdu.Pdu.NormPdu.nErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
                    pNLE->Pdu.Pdu.NormPdu.nErrorIndex  = pVLE->nErrorIndex;

                    break; // bail...
                }
            }

            // next entry
            pLE = pLE->Flink;
        }    
    }

    // see if this is first version
    if (pNLE->nVersion == SNMP_VERSION_1) {

        // adjust status code
        switch (pNLE->Pdu.Pdu.NormPdu.nErrorStatus) {

        case SNMP_ERRORSTATUS_NOERROR:
        case SNMP_ERRORSTATUS_TOOBIG:
        case SNMP_ERRORSTATUS_NOSUCHNAME:
        case SNMP_ERRORSTATUS_BADVALUE:
        case SNMP_ERRORSTATUS_READONLY:
        case SNMP_ERRORSTATUS_GENERR:
            break;

        case SNMP_ERRORSTATUS_NOACCESS:
        case SNMP_ERRORSTATUS_NOCREATION:
        case SNMP_ERRORSTATUS_NOTWRITABLE:
        case SNMP_ERRORSTATUS_AUTHORIZATIONERROR:
        case SNMP_ERRORSTATUS_INCONSISTENTNAME:
            pNLE->Pdu.Pdu.NormPdu.nErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
            break;

        case SNMP_ERRORSTATUS_WRONGTYPE:
        case SNMP_ERRORSTATUS_WRONGLENGTH:
        case SNMP_ERRORSTATUS_WRONGENCODING:
        case SNMP_ERRORSTATUS_WRONGVALUE:
        case SNMP_ERRORSTATUS_INCONSISTENTVALUE:
            pNLE->Pdu.Pdu.NormPdu.nErrorStatus = SNMP_ERRORSTATUS_BADVALUE;
            break;

        case SNMP_ERRORSTATUS_RESOURCEUNAVAILABLE:
        case SNMP_ERRORSTATUS_COMMITFAILED:
        case SNMP_ERRORSTATUS_UNDOFAILED:
        default:
            pNLE->Pdu.Pdu.NormPdu.nErrorStatus = SNMP_ERRORSTATUS_GENERR;
            break;
        }
    }

    return (pNLE->Pdu.Pdu.NormPdu.nErrorStatus == SNMP_ERRORSTATUS_NOERROR);
}


BOOL
UpdateVarBindsFromResolvedVb(
    PNETWORK_LIST_ENTRY pNLE
    )

/*++

Routine Description:

    Updates varbinds with results containing single varbinds.

Arguments:

    pNLE - pointer to network list entry.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE;
    PVARBIND_LIST_ENTRY pVLE;
    
    // point to first varbind
    pLE = pNLE->Bindings.Flink;

    // process each varbind entry
    while (pLE != &pNLE->Bindings) {

        // retrieve pointer to varbind entry from link
        pVLE = CONTAINING_RECORD(pLE, VARBIND_LIST_ENTRY, Link);

        SNMPDBG((
            SNMP_LOG_TRACE,
            "SNMP: SVC: variable %d resolved name %s.\n",
            pVLE->nErrorIndex,
            SnmpUtilOidToA(&pVLE->ResolvedVb.name)
            ));    

        // release memory for original varbind
        SnmpUtilVarBindFree(&pNLE->Pdu.Vbl.list[pVLE->nErrorIndex - 1]);

        // copy resolved varbind structure into pdu varbindlist
        SnmpUtilVarBindCpy(&pNLE->Pdu.Vbl.list[pVLE->nErrorIndex - 1], 
                           &pVLE->ResolvedVb
                           );

        // next entry
        pLE = pLE->Flink;
    }

    // success
    return TRUE;
}


BOOL
UpdateVarBindsFromResolvedVbl(
    PNETWORK_LIST_ENTRY pNLE
    )

/*++

Routine Description:

    Updates varbinds with results containing multiple varbinds.

Arguments:

    pNLE - pointer to network list entry.

Return Values:

    Returns true if successful.

--*/

{
    UINT nRepeaters;
    UINT nNonRepeaters;
    UINT nMaxRepetitions;
    UINT nIterations;
    UINT nVarBindsLast;
    UINT nVarBinds = 0;
    SnmpVarBind * pVarBind;
    PVARBIND_LIST_ENTRY pVLE;
    PLIST_ENTRY pLE1;
    PLIST_ENTRY pLE2;

    // retrieve getbulk parameters from pdu
    nNonRepeaters   = pNLE->Pdu.Pdu.BulkPdu.nNonRepeaters;
    nMaxRepetitions = pNLE->Pdu.Pdu.BulkPdu.nMaxRepetitions;
    nRepeaters      = (pNLE->Pdu.Vbl.len >= nNonRepeaters)
                         ? (pNLE->Pdu.Vbl.len - nNonRepeaters)
                         : 0
                         ;

    // see if we need to expand size of varbind list
    if ((nRepeaters > 0) && (nMaxRepetitions > 1)) {
    
        UINT nMaxVarBinds;
        SnmpVarBind * pVarBinds;

        // calculate maximum number of varbinds possible
        nMaxVarBinds = nNonRepeaters + (nMaxRepetitions * nRepeaters);
    
        // reallocate varbind list to fit maximum
        pVarBinds = SnmpUtilMemReAlloc(pNLE->Pdu.Vbl.list, 
                                       nMaxVarBinds * sizeof(SnmpVarBind)
                                       );

        // validate pointer
        if (pVarBinds == NULL) {    
            
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: SVC: Could not re-allocate varbind list.\n"
                ));

            return FALSE; // bail...
        }

        // restore varbind pointer
        pNLE->Pdu.Vbl.list = pVarBinds;
    }

    // point to first varbind
    pLE1 = pNLE->Bindings.Flink;

    // process each varbind entry
    while (pLE1 != &pNLE->Bindings) {

        // retrieve pointer to varbind entry from link
        pVLE = CONTAINING_RECORD(pLE1, VARBIND_LIST_ENTRY, Link);

        // see if this is non-repeater
        if (pVLE->nMaxRepetitions == 1) {

            // release memory for original varbind
            SnmpUtilVarBindFree(&pNLE->Pdu.Vbl.list[nVarBinds]);

            // copy resolved varbind into pdu structure
            SnmpUtilVarBindCpy(&pNLE->Pdu.Vbl.list[nVarBinds++],
                               &pVLE->ResolvedVb
                               );

        } else {

            //
            // finished processing non-repeaters 
            //

            break;
        }

        // next entry
        pLE1 = pLE1->Flink;
    }

    // initialize
    nIterations = 0;

    // store
    pLE2 = pLE1; 

    // process any repeaters until max
    while (nIterations < nMaxRepetitions) {

        // restore
        pLE1 = pLE2;        

        // process each varbind entry
        while (pLE1 != &pNLE->Bindings) {

            // retrieve pointer to varbind entry from link
            pVLE = CONTAINING_RECORD(pLE1, VARBIND_LIST_ENTRY, Link);

            // see if value stored in default
            if (pVLE->ResolvedVbl.len == 0) {

                // release memory for original varbind 
                SnmpUtilVarBindFree(&pNLE->Pdu.Vbl.list[nVarBinds]);

                // copy resolved varbind into pdu varbind list
                SnmpUtilVarBindCpy(&pNLE->Pdu.Vbl.list[nVarBinds],
                                   &pVLE->ResolvedVb
                                   );

                // increment
                nVarBinds++;

            // see if value available in this iteration
            } else if (pVLE->ResolvedVbl.len > nIterations) {

                // release memory for original varbind 
                SnmpUtilVarBindFree(&pNLE->Pdu.Vbl.list[nVarBinds]);

                // copy resolved varbind into pdu varbind list
                SnmpUtilVarBindCpy(&pNLE->Pdu.Vbl.list[nVarBinds],
                                   &pVLE->ResolvedVbl.list[nIterations]
                                   );

                // increment
                nVarBinds++;
            }

            // next entry
            pLE1 = pLE1->Flink;
        }
    
        // increment
        nIterations++;
    }

    // save new varbind count
    pNLE->Pdu.Vbl.len = nVarBinds;

    // success
    return TRUE;
}


BOOL
UpdatePdu(
    PNETWORK_LIST_ENTRY pNLE,
    BOOL                fOk
    )

/*++

Routine Description:

    Updates pdu with query results.

Arguments:

    pNLE - pointer to network list entry.

    fOk - true if process succeeded to this point.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE;
    PVARBIND_LIST_ENTRY pVLE;

    // validate
    if (fOk) {
        
        // make sure varbinds valid
        fOk = ValidateVarBinds(pNLE);

        // validate
        if (fOk) {

            // see if pdu type is getnext or getbulk
            if (pNLE->Pdu.nType != SNMP_PDU_GETBULK) {

                // update varbinds with single result
                fOk = UpdateVarBindsFromResolvedVb(pNLE);

            } else {

                // update varbinds with multiple results
                fOk = UpdateVarBindsFromResolvedVbl(pNLE);
            }
        }
    }

    // trap internal errors that have not been accounted for as of yet
    if (!fOk && (pNLE->Pdu.Pdu.NormPdu.nErrorStatus == SNMP_ERRORSTATUS_NOERROR)) {

        // report status that was determined above
        pNLE->Pdu.Pdu.NormPdu.nErrorStatus = SNMP_ERRORSTATUS_GENERR;
        pNLE->Pdu.Pdu.NormPdu.nErrorIndex  = 0;
    }

    if (pNLE->Pdu.Pdu.NormPdu.nErrorStatus == SNMP_ERRORSTATUS_NOERROR)
    {
        switch(pNLE->Pdu.nType)
        {
        case SNMP_PDU_GETNEXT:
        case SNMP_PDU_GETBULK:
        case SNMP_PDU_GET:
            // update counter for successful GET-NEXT GET-BULK
            mgmtCAdd(CsnmpInTotalReqVars, pNLE->Pdu.Vbl.len);
            break;
        case SNMP_PDU_SET:
            // update counter for successful SET
            mgmtCAdd(CsnmpInTotalSetVars, pNLE->Pdu.Vbl.len);
            break;
        }
    }
    else
    {
        // update here counters for all OUT errors
        mgmtUtilUpdateErrStatus(OUT_errStatus, pNLE->Pdu.Pdu.NormPdu.nErrorStatus);
    }
    
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
AllocVLE(
    PVARBIND_LIST_ENTRY * ppVLE
    )

/*++

Routine Description:

    Allocates varbind structure and initializes.

Arguments:

    ppVLE - pointer to receive pointer to entry.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;
    PVARBIND_LIST_ENTRY pVLE = NULL;

    // attempt to allocate structure
    pVLE = AgentMemAlloc(sizeof(VARBIND_LIST_ENTRY));

    // validate
    if (pVLE != NULL) {

        // success
        fOk = TRUE;
    
    } else {
        
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SVC: could not allocate varbind entry.\n"
            ));
    }    

    // transfer
    *ppVLE = pVLE;

    return fOk;
}


BOOL 
FreeVLE(
    PVARBIND_LIST_ENTRY pVLE
    )

/*++

Routine Description:

    Releases varbind structure.

Arguments:

    pVLE - pointer to list entry to be freed.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = TRUE;

    // validate pointer
    if (pVLE != NULL) {

        // release current varbind
        SnmpUtilVarBindFree(&pVLE->ResolvedVb);

        // release current varbind list
        SnmpUtilVarBindListFree(&pVLE->ResolvedVbl);

        // release structure
        AgentMemFree(pVLE);
    }

    return TRUE;
}


BOOL
ProcessVarBinds(
    PNETWORK_LIST_ENTRY pNLE
    )

/*++

Routine Description:

    Creates list of varbind entries from varbind structure.

Arguments:

    pNLE - pointer to network list entry.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;

    // validate type before processing
    if ((pNLE->Pdu.nType == SNMP_PDU_SET) ||
        (pNLE->Pdu.nType == SNMP_PDU_GET) ||
        (pNLE->Pdu.nType == SNMP_PDU_GETNEXT) ||
        (pNLE->Pdu.nType == SNMP_PDU_GETBULK)) {

        // initialize varbinds
        if (LoadVarBinds(pNLE)) {

            // process queries 
            fOk = ProcessQueries(pNLE);
        }

        // transfer results 
        UpdatePdu(pNLE, fOk);
    
        // unload varbinds
        UnloadVarBinds(pNLE);

        // update management counters for accepted and processed requests
        switch(pNLE->Pdu.nType)
        {
        case SNMP_PDU_GET:
            mgmtCTick(CsnmpInGetRequests);
            break;
        case SNMP_PDU_GETNEXT:
        case SNMP_PDU_GETBULK:
            mgmtCTick(CsnmpInGetNexts);
            break;
        case SNMP_PDU_SET:
            mgmtCTick(CsnmpInSetRequests);
            break;
        }

    } else {

        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SVC: ignoring unknown pdu type %d.\n",
            pNLE->Pdu.nType
            ));
    }

    return fOk;
}
