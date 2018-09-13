/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    regions.c

Abstract:

    Contains routines for manipulating MIB region structures.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Header files                                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"
#include "subagnts.h"
#include "regions.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
UpdateSupportedRegion(
    PMIB_REGION_LIST_ENTRY pExistingRLE,
    PMIB_REGION_LIST_ENTRY pRLE
    )

/*++

Routine Description:

    Updates MIB region properties based on supporting subagent.

Arguments:

    pExisingRLE - pointer to existing MIB region to be updated.

    pRLE - pointer to subagent MIB region to be analyzed and saved.

Return Values:

    Returns true if successful.

--*/

{
    INT nDiff;
    PMIB_REGION_LIST_ENTRY pSubagentRLE;

    // see if source is subagent
    if (pRLE->pSubagentRLE == NULL) {
    
        // save pointer
        pSubagentRLE = pRLE;

    } else {

        // save pointer
        pSubagentRLE = pRLE->pSubagentRLE;
    }    

    // see if target uninitialized    
    if (pExistingRLE->pSubagentRLE == NULL) {

        // save pointer to subagent region
        pExistingRLE->pSubagentRLE = pSubagentRLE;

        // save pointer to supporting subagent
        pExistingRLE->pSLE = pSubagentRLE->pSLE;        

    } else {

        UINT nSubIds1;
        UINT nSubIds2;

        // determine length of existing subagent's original prefix
        nSubIds1 = pExistingRLE->pSubagentRLE->PrefixOid.idLength;

        // determine length of new subagent's prefix
        nSubIds2 = pSubagentRLE->PrefixOid.idLength;

        // update if more specific
        if (nSubIds1 <= nSubIds2) {
        
            // save pointer to subagent region
            pExistingRLE->pSubagentRLE = pSubagentRLE;

            // save pointer to supporting subagent
            pExistingRLE->pSLE = pSubagentRLE->pSLE;        
        }             
    }

    return TRUE;
}


BOOL
SplitSupportedRegion(
    PMIB_REGION_LIST_ENTRY   pRLE1,
    PMIB_REGION_LIST_ENTRY   pRLE2,
    PMIB_REGION_LIST_ENTRY * ppLastSplitRLE
    )

/*++

Routine Description:

    Splits existing MIB region in order to insert new region.

Arguments:

    pRLE1 - pointer to first MIB region to be split.

    pRLE2 - pointer to second MIB region to be split (not released).

    ppLastSplitRLE - pointer to receiver pointer to last split MIB region.

Return Values:

    Returns true if successful.

--*/

{
    INT nLimitDiff;
    INT nPrefixDiff;
    PMIB_REGION_LIST_ENTRY pRLE3 = NULL;
    PMIB_REGION_LIST_ENTRY pRLE4 = NULL;
    PMIB_REGION_LIST_ENTRY pRLE5 = NULL;
    BOOL fOk = TRUE;

    // allocate regions
    if (!AllocRLE(&pRLE3) ||
        !AllocRLE(&pRLE4) ||
        !AllocRLE(&pRLE5)) {

        // release
        FreeRLE(pRLE3);
        FreeRLE(pRLE4);
        FreeRLE(pRLE5);
    
        // failure
        return FALSE;
    }

    // initialize pointer
    *ppLastSplitRLE = pRLE5;

    // calculate difference betweeen mib region limits
    nLimitDiff = SnmpUtilOidCmp(&pRLE1->LimitOid, &pRLE2->LimitOid);
        
    // calculate difference betweeen mib region prefixes
    nPrefixDiff = SnmpUtilOidCmp(&pRLE1->PrefixOid, &pRLE2->PrefixOid);
        
    // check for same prefix        
    if (nPrefixDiff != 0) {

        // first prefix less 
        if (nPrefixDiff < 0) {

            // r3.prefix equals min(rl.prefix,r2.prefix)
            SnmpUtilOidCpy(&pRLE3->PrefixOid, &pRLE1->PrefixOid);

            // r3.limit equals max(rl.prefix,r2.prefix)
            SnmpUtilOidCpy(&pRLE3->LimitOid, &pRLE2->PrefixOid);

            // r3 is supported by r1 subagent 
            UpdateSupportedRegion(pRLE3, pRLE1);

        } else {

            // r3.prefix equals min(rl.prefix,r2.prefix)
            SnmpUtilOidCpy(&pRLE3->PrefixOid, &pRLE2->PrefixOid);

            // r3.limit equals max(rl.prefix,r2.prefix)
            SnmpUtilOidCpy(&pRLE3->LimitOid, &pRLE1->PrefixOid);

            // r3 is supported by r2 subagent 
            UpdateSupportedRegion(pRLE3, pRLE2);
        }

        // r4.prefix equals r3.limit
        SnmpUtilOidCpy(&pRLE4->PrefixOid, &pRLE3->LimitOid);

        // r4 is supported by both subagents
        UpdateSupportedRegion(pRLE4, pRLE1);
        UpdateSupportedRegion(pRLE4, pRLE2);

        // first limit less 
        if (nLimitDiff < 0) {

            // r4.limit equals min(rl.limit,r2.limit)
            SnmpUtilOidCpy(&pRLE4->LimitOid, &pRLE1->LimitOid);

            // r5.prefix equals r4.limit
            SnmpUtilOidCpy(&pRLE5->PrefixOid, &pRLE4->LimitOid);

            // r5.limit equals max(rl.limit,r2.limit)
            SnmpUtilOidCpy(&pRLE5->LimitOid, &pRLE2->LimitOid);

            // r5 is supported by r2 subagent 
            UpdateSupportedRegion(pRLE5, pRLE2);

            // insert third mib region into list first
            InsertHeadList(&pRLE1->Link, &pRLE5->Link);

        } else if (nLimitDiff > 0) {

            // r4.limit equals min(rl.limit,r2.limit)
            SnmpUtilOidCpy(&pRLE4->LimitOid, &pRLE2->LimitOid);

            // r5.prefix equals r4.limit
            SnmpUtilOidCpy(&pRLE5->PrefixOid, &pRLE4->LimitOid);

            // r5.limit equals max(rl.limit,r2.limit)
            SnmpUtilOidCpy(&pRLE5->LimitOid, &pRLE1->LimitOid);

            // r5 is supported by r1 subagent 
            UpdateSupportedRegion(pRLE5, pRLE1);

            // insert third mib region into list first
            InsertHeadList(&pRLE1->Link, &pRLE5->Link);

        } else {

            // r4.limit equals min(rl.limit,r2.limit)
            SnmpUtilOidCpy(&pRLE4->LimitOid, &pRLE2->LimitOid);

            // return r4 as last
            *ppLastSplitRLE = pRLE4;

            // release
            FreeRLE(pRLE5);
        }

        // insert remaining mib regions into list
        InsertHeadList(&pRLE1->Link, &pRLE4->Link);
        InsertHeadList(&pRLE1->Link, &pRLE3->Link);

        // remove existing
        RemoveEntryList(&pRLE1->Link);

        // release
        FreeRLE(pRLE1);

    } else if (nLimitDiff != 0) {

        // r3.prefix equals same prefix for r1 and r2
        SnmpUtilOidCpy(&pRLE3->PrefixOid, &pRLE1->PrefixOid);

        // r3 is supported by both subagents
        UpdateSupportedRegion(pRLE3, pRLE1);
        UpdateSupportedRegion(pRLE3, pRLE2);

        // first limit less 
        if (nLimitDiff < 0) {

            // r3.limit equals min(rl.limit,r2.limit)
            SnmpUtilOidCpy(&pRLE3->LimitOid, &pRLE1->LimitOid);

            // r4.prefix equals r3.limit
            SnmpUtilOidCpy(&pRLE4->PrefixOid, &pRLE3->LimitOid);

            // r4.limit equals max(rl.limit,r2.limit)
            SnmpUtilOidCpy(&pRLE4->LimitOid, &pRLE2->LimitOid);

            // r4 is supported by r2 subagent
            UpdateSupportedRegion(pRLE4, pRLE2);

        } else {

            // r3.limit equals min(rl.limit,r2.limit)
            SnmpUtilOidCpy(&pRLE3->LimitOid, &pRLE2->LimitOid);

            // r4.prefix equals r3.limit
            SnmpUtilOidCpy(&pRLE4->PrefixOid, &pRLE3->LimitOid);

            // r4.limit equals max(rl.limit,r2.limit)
            SnmpUtilOidCpy(&pRLE4->LimitOid, &pRLE1->LimitOid);

            // r4 is supported by r1 subagent
            UpdateSupportedRegion(pRLE4, pRLE1);
        } 

        // return r4 as last
        *ppLastSplitRLE = pRLE4;

        // insert mib regions into list
        InsertHeadList(&pRLE1->Link, &pRLE4->Link);
        InsertHeadList(&pRLE1->Link, &pRLE3->Link);

        // remove existing
        RemoveEntryList(&pRLE1->Link);

        // release
        FreeRLE(pRLE1);
        FreeRLE(pRLE5);

    } else {

        // region supported existing subagent
        UpdateSupportedRegion(pRLE1, pRLE2);

        // return r1 as last
        *ppLastSplitRLE = pRLE1;

        // release
        FreeRLE(pRLE3);
        FreeRLE(pRLE4);
        FreeRLE(pRLE5);
    }

    // success
    return TRUE;
}


BOOL
InsertSupportedRegion(
    PMIB_REGION_LIST_ENTRY pExistingRLE,
    PMIB_REGION_LIST_ENTRY pRLE
    )

/*++

Routine Description:

    Splits existing MIB region in order to insert new region.

Arguments:

    pExisingRLE - pointer to existing MIB region to be split.

    pRLE - pointer to MIB region to be inserted.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk;
    PLIST_ENTRY pLE;
    PMIB_REGION_LIST_ENTRY pLastSplitRLE = NULL;
    INT nDiff;
    
    // attempt to split mib regions into pieces parts
    fOk = SplitSupportedRegion(pExistingRLE, pRLE, &pLastSplitRLE);

    // process remaining entries
    while (pLastSplitRLE != NULL) {

        // re-use stack pointer
        pExistingRLE = pLastSplitRLE;    

        // re-initialize 
        pLastSplitRLE = NULL;

        // obtain pointer to next entry        
        pLE = pExistingRLE->Link.Flink;

        // make sure entries remaining
        if (pLE != &g_SupportedRegions) {

            // retrieve pointer to mib region that follows last split one
            pRLE = CONTAINING_RECORD(pLE, MIB_REGION_LIST_ENTRY, Link);

            // compare mib regions
            nDiff = SnmpUtilOidCmp(
                        &pExistingRLE->LimitOid,
                        &pRLE->PrefixOid
                        );

            // overlapped?
            if (nDiff > 0) {

                // remove from list
                RemoveEntryList(&pRLE->Link);

                // split the two new overlapped mib regions
                SplitSupportedRegion(pExistingRLE, pRLE, &pLastSplitRLE);

                // release
                FreeRLE(pRLE);
            }
        }
    }                

    return fOk;
}
/*---debug purpose only----
void PrintSupportedRegion()
{
    PLIST_ENTRY pLE;
    PMIB_REGION_LIST_ENTRY pRLE;

    // obtain pointer to list head
    pLE = g_SupportedRegions.Flink;

    // process all entries in list
    while (pLE != &g_SupportedRegions) {

        // retrieve pointer to mib region structure 
        pRLE = CONTAINING_RECORD(pLE, MIB_REGION_LIST_ENTRY, Link);
        SNMPDBG((SNMP_LOG_VERBOSE,"\t[%s\n", SnmpUtilOidToA(&(pRLE->PrefixOid))));
        SNMPDBG((SNMP_LOG_VERBOSE,"\t\t%s]\n", SnmpUtilOidToA(&(pRLE->LimitOid))));

        // next entry
        pLE = pLE->Flink;
    }
    SNMPDBG((SNMP_LOG_VERBOSE,"----\n"));
}
*/

BOOL
AddSupportedRegion(
    PMIB_REGION_LIST_ENTRY pRLE
    )

/*++

Routine Description:

    Add subagent's MIB region into master agent's list.

Arguments:

    pRLE - pointer to MIB region to add to supported list.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE;
    PMIB_REGION_LIST_ENTRY pRLE2;
    PMIB_REGION_LIST_ENTRY pExistingRLE;
    BOOL fFoundOk = FALSE;
    BOOL fOk = FALSE;
    INT nDiff;

//    PrintSupportedRegion();

    // attempt to locate prefix in existing mib regions
    if (FindFirstOverlappingRegion(&pExistingRLE, pRLE)) {
            
        // split existing region into bits
        fOk = InsertSupportedRegion(pExistingRLE, pRLE);

    } else {

        // obtain pointer to list head
        pLE = g_SupportedRegions.Flink;

        // process all entries in list
        while (pLE != &g_SupportedRegions) {

            // retrieve pointer to mib region 
            pExistingRLE = CONTAINING_RECORD(pLE, MIB_REGION_LIST_ENTRY, Link);

            // compare region prefix
            nDiff = SnmpUtilOidCmp(&pRLE->PrefixOid, &pExistingRLE->PrefixOid);

            // found match?
            if (nDiff < 0) {

                // success
                fFoundOk = TRUE;

                break; // bail...
            } 

            // next entry
            pLE = pLE->Flink;
        }

        // validate pointer
        if (AllocRLE(&pRLE2)) {

            // transfer prefix oid from subagent region
            SnmpUtilOidCpy(&pRLE2->PrefixOid, &pRLE->PrefixOid);
        
            // transfer limit oid from subagent region
            SnmpUtilOidCpy(&pRLE2->LimitOid, &pRLE->LimitOid);

            // save region pointer
            pRLE2->pSubagentRLE = pRLE;

            // save subagent pointer
            pRLE2->pSLE = pRLE->pSLE;

            // validate
            if (fFoundOk) {

                // add new mib range into supported list 
                InsertTailList(&pExistingRLE->Link, &pRLE2->Link);

            } else {

                // add new mib range into global supported list
                InsertTailList(&g_SupportedRegions, &pRLE2->Link);
            }

            // success
            fOk = TRUE;
        }
    }

    return fOk;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
AllocRLE(
    PMIB_REGION_LIST_ENTRY * ppRLE    
    )

/*++

Routine Description:

    Allocates MIB region structure and initializes.

Arguments:

    ppRLE - pointer to receive pointer to list entry.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;
    PMIB_REGION_LIST_ENTRY pRLE;
    
    // attempt to allocate structure
    pRLE = AgentMemAlloc(sizeof(MIB_REGION_LIST_ENTRY));

    // validate pointer
    if (pRLE != NULL) {

        // initialize links
        InitializeListHead(&pRLE->Link);

        // success
        fOk = TRUE;
    
    } else {
        
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SVC: could not allocate region entry.\n"
            ));
    }    

    // transfer
    *ppRLE = pRLE;

    return fOk;
}


BOOL 
FreeRLE(
    PMIB_REGION_LIST_ENTRY pRLE    
    )

/*++

Routine Description:

    Releases MIB region structure.

Arguments:

    ppRLE - pointer to MIB region to be freed.

Return Values:

    Returns true if successful.

--*/

{
    // validate pointer
    if (pRLE != NULL) {

        // release memory for prefix oid
        SnmpUtilOidFree(&pRLE->PrefixOid);

        // release memory for limit oid
        SnmpUtilOidFree(&pRLE->LimitOid);

        // release memory
        AgentMemFree(pRLE);
    }

    return TRUE;
}

BOOL    
UnloadRegions(
    PLIST_ENTRY pListHead
    )

/*++

Routine Description:

    Destroys list of MIB regions.

Arguments:

    pListHead - pointer to list of regions.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE;
    PMIB_REGION_LIST_ENTRY pRLE;

    // process entries until empty
    while (!IsListEmpty(pListHead)) {

        // extract next entry from head 
        pLE = RemoveHeadList(pListHead);

        // retrieve pointer to mib region structure 
        pRLE = CONTAINING_RECORD(pLE, MIB_REGION_LIST_ENTRY, Link);

        // release
        FreeRLE(pRLE);
    }

    return TRUE;
}

BOOL
FindFirstOverlappingRegion(
    PMIB_REGION_LIST_ENTRY * ppRLE,
    PMIB_REGION_LIST_ENTRY pNewRLE
    )
/*++

Routine Description:

    Detects if any existent region overlapps with the new one to be added.

Arguments:

    ppRLE - pointer to receive pointer to list entry.

    pNewRLE - pointer to new region to be tested
    
Return Values:

    Returns true if match found.

--*/

{
    PLIST_ENTRY pLE;
    PMIB_REGION_LIST_ENTRY pRLE;

    // initialize
    *ppRLE = NULL;

    // obtain pointer to list head
    pLE = g_SupportedRegions.Flink;

    // process all entries in list
    while (pLE != &g_SupportedRegions) {

        // retrieve pointer to mib region structure 
        pRLE = CONTAINING_RECORD(pLE, MIB_REGION_LIST_ENTRY, Link);

        if (SnmpUtilOidCmp(&pNewRLE->PrefixOid, &pRLE->LimitOid) < 0 &&
            SnmpUtilOidCmp(&pNewRLE->LimitOid, &pRLE->PrefixOid) > 0)
        {
            *ppRLE = pRLE;
            return TRUE;
        } 

        // next entry
        pLE = pLE->Flink;
    }

    // failure
    return FALSE;
}


BOOL
FindSupportedRegion(
    PMIB_REGION_LIST_ENTRY * ppRLE,
    AsnObjectIdentifier *    pPrefixOid,
    BOOL                     fAnyOk
    )

/*++

Routine Description:

    Locates MIB region in list.

Arguments:

    ppRLE - pointer to receive pointer to list entry.

    pPrefixOid - pointer to OID to locate within MIB region.
    
    fAnyOk - true if exact match not necessary.

Return Values:

    Returns true if match found.

--*/

{
    PLIST_ENTRY pLE;
    PMIB_REGION_LIST_ENTRY pRLE;
    INT nDiff;

    // initialize
    *ppRLE = NULL;

    // obtain pointer to list head
    pLE = g_SupportedRegions.Flink;

    // process all entries in list
    while (pLE != &g_SupportedRegions) {

        // retrieve pointer to mib region structure 
        pRLE = CONTAINING_RECORD(pLE, MIB_REGION_LIST_ENTRY, Link);

        // region prefix should be also the prefix for the given OID
        nDiff = SnmpUtilOidNCmp(pPrefixOid, &pRLE->PrefixOid, pRLE->PrefixOid.idLength);

        // found match?
        if ((nDiff < 0 && fAnyOk) ||
            (nDiff == 0 && SnmpUtilOidCmp(pPrefixOid, &pRLE->LimitOid) < 0))
        {
            *ppRLE = pRLE;
            return TRUE;
        } 

        // next entry
        pLE = pLE->Flink;
    }

    // failure
    return FALSE;
}


BOOL
LoadSupportedRegions(
    )

/*++

Routine Description:

    Creates global list of supported MIB regions from subagent MIB regions.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE1;
    PLIST_ENTRY pLE2;
    PSUBAGENT_LIST_ENTRY pSLE;
    PMIB_REGION_LIST_ENTRY pRLE;

    // get subagent list head
    pLE1 = g_Subagents.Flink;

    // process all entries in list
    while (pLE1 != &g_Subagents) {

        // retrieve pointer to subagent structure 
        pSLE = CONTAINING_RECORD(pLE1, SUBAGENT_LIST_ENTRY, Link);

        SNMPDBG((
            SNMP_LOG_VERBOSE,
            "SNMP: SVC: Scan views supported by %s.\n",
            pSLE->pPathname
            ));

        // get supported regions list head
        pLE2 = pSLE->SupportedRegions.Flink;
        
        // process all entries in list
        while (pLE2 != &pSLE->SupportedRegions) {

            // retrieve pointer to mib region structure
            pRLE = CONTAINING_RECORD(pLE2, MIB_REGION_LIST_ENTRY, Link);

            SNMPDBG((
                SNMP_LOG_VERBOSE,
                "SNMP: SVC: view %s\n",
                SnmpUtilOidToA(&pRLE->PrefixOid)
                ));

            // attempt to add mib region    
            if (!AddSupportedRegion(pRLE)) {

                // failure
                return FALSE;
            }

            // next mib region
            pLE2 = pLE2->Flink;
        }

        // next subagent
        pLE1 = pLE1->Flink;
    }

    // success
    return TRUE;
}


BOOL
UnloadSupportedRegions(
    )

/*++

Routine Description:

    Destroys list of MIB regions.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    // unload global supported regions
    return UnloadRegions(&g_SupportedRegions);
}


