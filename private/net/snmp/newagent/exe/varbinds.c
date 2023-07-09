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


// Include files                                                             //
#include "globals.h"
#include "varbinds.h"
#include "query.h"
#include "snmpmgmt.h"


// Private procedures                                                        //


BOOL LoadVarBind(PNETWORK_LIST_ENTRY pNLE, UINT                iVb)
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
        pVLE->nErrorIndex = iVb + 1;// save varbind list index        
        pVb = &pNLE->Pdu.Vbl.list[iVb];// retrieve varbind pointer

        SNMPDBG((SNMP_LOG_TRACE, "SNMP: SVC: variable %d name %s.\n", pVLE->nErrorIndex, SnmpUtilOidToA(&pVb->name)));
        
        pVLE->ResolvedVb.value.asnType = ASN_NULL;// initialize type of resolved variable        
        SnmpUtilOidCpy(&pVLE->ResolvedVb.name, &pVb->name);// copy varbind name to working structure

        // see if specific object asked for
        fAnyOk = ((pNLE->Pdu.nType == SNMP_PDU_GETNEXT) || (pNLE->Pdu.nType == SNMP_PDU_GETBULK));

        // attempt to lookup variable name in supported regions
        if (FindSupportedRegion(&pRLE, &pVb->name, fAnyOk)) {            
            pVLE->pCurrentRLE = pRLE;// save pointer to region            
            pVLE->nState = VARBIND_INITIALIZED;// structure has been initialized

            SNMPDBG((SNMP_LOG_TRACE, "SNMP: SVC: variable %d assigned to %s.\n", pVLE->nErrorIndex, pVLE->pCurrentRLE->pSLE->pPathname));
            SNMPDBG((SNMP_LOG_VERBOSE, "SNMP: SVC: variable %d state '%s'.\n", pVLE->nErrorIndex, VARBINDSTATESTRING(pVLE->nState)));

            // see if this is a getnext request
            if (pNLE->Pdu.nType == SNMP_PDU_GETNEXT) {                
                pVLE->nMaxRepetitions = 1;// only need single rep
            }
            else if (pNLE->Pdu.nType == SNMP_PDU_GETBULK) {
                // see if this non-repeater which is in bounds
                if (pNLE->Pdu.Pdu.BulkPdu.nNonRepeaters > (int)iVb) {                    
                    pVLE->nMaxRepetitions = 1;// only need single rep

                    SNMPDBG((SNMP_LOG_VERBOSE, "SNMP: SVC: variable %d max repetitions %d.\n", pVLE->nErrorIndex, pVLE->nMaxRepetitions));

                    // see if max-repetitions is non-zero
                }
                else if (pNLE->Pdu.Pdu.BulkPdu.nMaxRepetitions > 0) {
                    // set max-repetitions to value in getbulk pdu
                    pVLE->nMaxRepetitions = pNLE->Pdu.Pdu.BulkPdu.nMaxRepetitions;

                    SNMPDBG((SNMP_LOG_VERBOSE, "SNMP: SVC: variable %d max repetitions %d.\n", pVLE->nErrorIndex, pVLE->nMaxRepetitions));
                }
                else {                    
                    pVLE->nState = VARBIND_RESOLVED;// modify state to resolved

                    SNMPDBG((SNMP_LOG_VERBOSE, "SNMP: SVC: variable %d state '%s'.\n", pVLE->nErrorIndex, VARBINDSTATESTRING(pVLE->nState)));
                    SNMPDBG((SNMP_LOG_VERBOSE, "SNMP: SVC: variable %d value NULL.\n", pVLE->nErrorIndex));
                }
            }
            else if (pNLE->Pdu.nType == SNMP_PDU_SET) {
                // copy varbind value to working structure
                SnmpUtilAsnAnyCpy(&pVLE->ResolvedVb.value, &pVb->value);

                SNMPDBG((SNMP_LOG_VERBOSE, "SNMP: SVC: variable %d value %s.\n", pVLE->nErrorIndex, "<TBD>"));
            }
        }
        else {            
            pVLE->pCurrentRLE = NULL;// null pointer to region

            SNMPDBG((SNMP_LOG_TRACE, "SNMP: SVC: variable %d unable to be assigned.\n", pVLE->nErrorIndex));

            // getbulk
            if (fAnyOk) {                
                pVLE->nState = VARBIND_RESOLVED;// modify state to resolved                
                pVLE->ResolvedVb.value.asnType = SNMP_EXCEPTION_ENDOFMIBVIEW;// set the exception in the variable's type field

                SNMPDBG((SNMP_LOG_VERBOSE, "SNMP: SVC: variable %d state '%s'.\n", pVLE->nErrorIndex, VARBINDSTATESTRING(pVLE->nState)));
                SNMPDBG((SNMP_LOG_VERBOSE, "SNMP: SVC: variable %d value ENDOFMIBVIEW.\n", pVLE->nErrorIndex));
            }
            else if (pNLE->Pdu.nType == SNMP_PDU_GET) {                
                pVLE->nState = VARBIND_RESOLVED;// modify state to resolved                
                pVLE->ResolvedVb.value.asnType = SNMP_EXCEPTION_NOSUCHOBJECT;// set the exception in the variable's type field

                SNMPDBG((SNMP_LOG_VERBOSE, "SNMP: SVC: variable %d state '%s'.\n", pVLE->nErrorIndex, VARBINDSTATESTRING(pVLE->nState)));
                SNMPDBG((SNMP_LOG_VERBOSE, "SNMP: SVC: variable %d value NOSUCHOBJECT.\n", pVLE->nErrorIndex));
            }
            else {
                // modify state to resolved
                //pVLE->nState = VARBIND_ABORTED;
                pVLE->nState = VARBIND_RESOLVED;

                // save error status in network list entry
                pNLE->Pdu.Pdu.NormPdu.nErrorStatus = SNMP_ERRORSTATUS_NOTWRITABLE;
                pNLE->Pdu.Pdu.NormPdu.nErrorIndex = pVLE->nErrorIndex;

                SNMPDBG((SNMP_LOG_VERBOSE, "SNMP: SVC: variable %d state '%s'.\n", pVLE->nErrorIndex, VARBINDSTATESTRING(pVLE->nState)));
                SNMPDBG((SNMP_LOG_VERBOSE,
                         "SNMP: SVC: variable %d error %s.\n",
                         pVLE->nErrorIndex,
                         SNMPERRORSTRING(pNLE->Pdu.Pdu.NormPdu.nErrorStatus)));

                // failure
                //fOk = FALSE;
            }
        }
        
        InsertTailList(&pNLE->Bindings, &pVLE->Link);// add to existing varbind list
    }

    return fOk;
}


BOOL LoadVarBinds(PNETWORK_LIST_ENTRY pNLE)
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
        fOk = LoadVarBind(pNLE, iVb);// load individual varbind
    }

    return fOk;
}


BOOL UnloadVarBinds(PNETWORK_LIST_ENTRY pNLE)
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
        pLE = RemoveHeadList(&pNLE->Bindings);// point to first varbind        
        pVLE = CONTAINING_RECORD(pLE, VARBIND_LIST_ENTRY, Link);// retrieve pointer to varbind entry from link        
        FreeVLE(pVLE);// release
    }

    return TRUE;
}


BOOL ValidateVarBinds(PNETWORK_LIST_ENTRY pNLE)
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
        pLE = pNLE->Bindings.Flink;// point to first varbind

        // process each varbind entry
        while (pLE != &pNLE->Bindings) {            
            pVLE = CONTAINING_RECORD(pLE, VARBIND_LIST_ENTRY, Link);// retrieve pointer to varbind entry from link

            // see if varbind has been resolved
            if (pVLE->nState != VARBIND_RESOLVED) {
                SNMPDBG((SNMP_LOG_WARNING, "SNMP: SVC: variable %d unresolved.\n", pVLE->nErrorIndex));

                // report internal error has occurred
                pNLE->Pdu.Pdu.NormPdu.nErrorStatus = SNMP_ERRORSTATUS_GENERR;
                pNLE->Pdu.Pdu.NormPdu.nErrorIndex = pVLE->nErrorIndex;

                break; // bail...
            }
            else if (pNLE->nVersion == SNMP_VERSION_1) {
                // report error if exceptions are present instead of values
                if ((pVLE->ResolvedVb.value.asnType == SNMP_EXCEPTION_NOSUCHOBJECT) ||
                    (pVLE->ResolvedVb.value.asnType == SNMP_EXCEPTION_NOSUCHINSTANCE) ||
                    (pVLE->ResolvedVb.value.asnType == SNMP_EXCEPTION_ENDOFMIBVIEW)) {

                    SNMPDBG((SNMP_LOG_WARNING, "SNMP: SVC: variable %d unresolved in SNMPv1.\n", pVLE->nErrorIndex));

                    // report that variable could not be found
                    pNLE->Pdu.Pdu.NormPdu.nErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
                    pNLE->Pdu.Pdu.NormPdu.nErrorIndex = pVLE->nErrorIndex;

                    break; // bail...
                }
            }
            
            pLE = pLE->Flink;// next entry
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


BOOL UpdateVarBindsFromResolvedVb(PNETWORK_LIST_ENTRY pNLE)
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
    
    pLE = pNLE->Bindings.Flink;// point to first varbind

    // process each varbind entry
    while (pLE != &pNLE->Bindings) {        
        pVLE = CONTAINING_RECORD(pLE, VARBIND_LIST_ENTRY, Link);// retrieve pointer to varbind entry from link

        SNMPDBG((
            SNMP_LOG_TRACE,
            "SNMP: SVC: variable %d resolved name %s.\n",
            pVLE->nErrorIndex,
            SnmpUtilOidToA(&pVLE->ResolvedVb.name)
            ));
        
        SnmpUtilVarBindFree(&pNLE->Pdu.Vbl.list[pVLE->nErrorIndex - 1]);// release memory for original varbind        
        SnmpUtilVarBindCpy(&pNLE->Pdu.Vbl.list[pVLE->nErrorIndex - 1], &pVLE->ResolvedVb);// copy resolved varbind structure into pdu varbindlist        
        pLE = pLE->Flink;// next entry
    }
    
    return TRUE;// success
}


BOOL UpdateVarBindsFromResolvedVbl(PNETWORK_LIST_ENTRY pNLE)
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
    nNonRepeaters = pNLE->Pdu.Pdu.BulkPdu.nNonRepeaters;
    nMaxRepetitions = pNLE->Pdu.Pdu.BulkPdu.nMaxRepetitions;
    nRepeaters = (pNLE->Pdu.Vbl.len >= nNonRepeaters) ? (pNLE->Pdu.Vbl.len - nNonRepeaters) : 0;

    // see if we need to expand size of varbind list
    if ((nRepeaters > 0) && (nMaxRepetitions > 1)) {
        UINT nMaxVarBinds;
        SnmpVarBind * pVarBinds;
        
        nMaxVarBinds = nNonRepeaters + (nMaxRepetitions * nRepeaters);// calculate maximum number of varbinds possible        
        pVarBinds = SnmpUtilMemReAlloc(pNLE->Pdu.Vbl.list, nMaxVarBinds * sizeof(SnmpVarBind));// reallocate varbind list to fit maximum

        // validate pointer
        if (pVarBinds == NULL) {
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: SVC: Could not re-allocate varbind list.\n"));
            return FALSE; // bail...
        }
        
        pNLE->Pdu.Vbl.list = pVarBinds;// restore varbind pointer
    }
    
    pLE1 = pNLE->Bindings.Flink;// point to first varbind

    // process each varbind entry
    while (pLE1 != &pNLE->Bindings) {        
        pVLE = CONTAINING_RECORD(pLE1, VARBIND_LIST_ENTRY, Link);// retrieve pointer to varbind entry from link

        // see if this is non-repeater
        if (pVLE->nMaxRepetitions == 1) {            
            SnmpUtilVarBindFree(&pNLE->Pdu.Vbl.list[nVarBinds]);// release memory for original varbind            
            SnmpUtilVarBindCpy(&pNLE->Pdu.Vbl.list[nVarBinds++], &pVLE->ResolvedVb);// copy resolved varbind into pdu structure
        }
        else {
            // finished processing non-repeaters
            break;
        }
        
        pLE1 = pLE1->Flink;// next entry
    }

    
    nIterations = 0;// initialize    
    pLE2 = pLE1;// store

    // process any repeaters until max
    while (nIterations < nMaxRepetitions) {        
        pLE1 = pLE2;// restore

        // process each varbind entry
        while (pLE1 != &pNLE->Bindings) {            
            pVLE = CONTAINING_RECORD(pLE1, VARBIND_LIST_ENTRY, Link);// retrieve pointer to varbind entry from link

            // see if value stored in default
            if (pVLE->ResolvedVbl.len == 0) {                
                SnmpUtilVarBindFree(&pNLE->Pdu.Vbl.list[nVarBinds]);// release memory for original varbind                
                SnmpUtilVarBindCpy(&pNLE->Pdu.Vbl.list[nVarBinds], &pVLE->ResolvedVb);// copy resolved varbind into pdu varbind list               
                nVarBinds++; // increment

                // see if value available in this iteration
            }
            else if (pVLE->ResolvedVbl.len > nIterations) {                
                SnmpUtilVarBindFree(&pNLE->Pdu.Vbl.list[nVarBinds]);// release memory for original varbind

                // copy resolved varbind into pdu varbind list
                SnmpUtilVarBindCpy(&pNLE->Pdu.Vbl.list[nVarBinds], &pVLE->ResolvedVbl.list[nIterations]);
                
                nVarBinds++;// increment
            }
           
            pLE1 = pLE1->Flink; // next entry
        }
        
        nIterations++;// increment
    }
    
    pNLE->Pdu.Vbl.len = nVarBinds;// save new varbind count
    
    return TRUE;// success
}


BOOL UpdatePdu(PNETWORK_LIST_ENTRY pNLE, BOOL                fOk)
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
        fOk = ValidateVarBinds(pNLE);// make sure varbinds valid

        // validate
        if (fOk) {
            // see if pdu type is getnext or getbulk
            if (pNLE->Pdu.nType != SNMP_PDU_GETBULK) {                
                fOk = UpdateVarBindsFromResolvedVb(pNLE);// update varbinds with single result
            }
            else {                
                fOk = UpdateVarBindsFromResolvedVbl(pNLE);// update varbinds with multiple results
            }
        }
    }

    // trap internal errors that have not been accounted for as of yet
    if (!fOk && (pNLE->Pdu.Pdu.NormPdu.nErrorStatus == SNMP_ERRORSTATUS_NOERROR)) {

        // report status that was determined above
        pNLE->Pdu.Pdu.NormPdu.nErrorStatus = SNMP_ERRORSTATUS_GENERR;
        pNLE->Pdu.Pdu.NormPdu.nErrorIndex = 0;
    }

    if (pNLE->Pdu.Pdu.NormPdu.nErrorStatus == SNMP_ERRORSTATUS_NOERROR)
    {
        switch (pNLE->Pdu.nType)
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


// Public procedures                                                         //


BOOL AllocVLE(PVARBIND_LIST_ENTRY * ppVLE)
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
   
    pVLE = AgentMemAlloc(sizeof(VARBIND_LIST_ENTRY)); // attempt to allocate structure    
    if (pVLE != NULL) {// validate        
        fOk = TRUE;// success
    }
    else {
        SNMPDBG((SNMP_LOG_ERROR, "SNMP: SVC: could not allocate varbind entry.\n"));
    }
    
    *ppVLE = pVLE;// transfer

    return fOk;
}


BOOL FreeVLE(PVARBIND_LIST_ENTRY pVLE)
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
        SnmpUtilVarBindFree(&pVLE->ResolvedVb);// release current varbind        
        SnmpUtilVarBindListFree(&pVLE->ResolvedVbl);// release current varbind list        
        AgentMemFree(pVLE);// release structure
    }

    return TRUE;
}


BOOL ProcessVarBinds(PNETWORK_LIST_ENTRY pNLE)
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
        (pNLE->Pdu.nType == SNMP_PDU_GETBULK)) 
    {
        // initialize varbinds
        if (LoadVarBinds(pNLE)) {            
            fOk = ProcessQueries(pNLE);// process queries
        }
        
        UpdatePdu(pNLE, fOk);// transfer results        
        UnloadVarBinds(pNLE);// unload varbinds

        // update management counters for accepted and processed requests
        switch (pNLE->Pdu.nType)
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
    }
    else {
        SNMPDBG((SNMP_LOG_ERROR, "SNMP: SVC: ignoring unknown pdu type %d.\n", pNLE->Pdu.nType));
    }

    return fOk;
}
