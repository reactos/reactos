/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmchek2.c

Abstract:

    This module implements consistency checking for the registry.


Author:

    Bryan M. Willman (bryanwi) 27-Jan-92

Environment:


Revision History:

--*/

#include    "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpValidateHiveSecurityDescriptors)
#endif

extern ULONG   CmpUsedStorage;

BOOLEAN
CmpValidateHiveSecurityDescriptors(
    IN PHHIVE Hive
    )
/*++

Routine Description:

    Walks the list of security descriptors present in the hive and passes
    each security descriptor to RtlValidSecurityDescriptor.

    Only applies to descriptors in Stable store.  Those in Volatile store
    cannot have come from disk and therefore do not need this treatment
    anyway.

Arguments:

    Hive - Supplies pointer to the hive control structure

Return Value:

    TRUE  - All security descriptors are valid
    FALSE - At least one security descriptor is invalid

--*/

{
    PCM_KEY_NODE RootNode;
    PCM_KEY_SECURITY SecurityCell;
    HCELL_INDEX ListAnchor;
    HCELL_INDEX NextCell;
    HCELL_INDEX LastCell;

    CMLOG(CML_FLOW, CMS_SEC) {
        KdPrint(("CmpValidateHiveSecurityDescriptor: Hive = %lx\n",(ULONG_PTR)Hive));
    }
    if (!HvIsCellAllocated(Hive,Hive->BaseBlock->RootCell)) {
        //
        // root cell HCELL_INDEX is bogus
        //
        return(FALSE);
    }
    RootNode = (PCM_KEY_NODE) HvGetCell(Hive, Hive->BaseBlock->RootCell);
    ListAnchor = NextCell = RootNode->Security;

    do {
        if (!HvIsCellAllocated(Hive, NextCell)) {
            CMLOG(CML_MAJOR, CMS_SEC) {
                KdPrint(("CM: CmpValidateHiveSecurityDescriptors\n"));
                KdPrint(("    NextCell: %08lx is invalid HCELL_INDEX\n",NextCell));
            }
            return(FALSE);
        }
        SecurityCell = (PCM_KEY_SECURITY) HvGetCell(Hive, NextCell);
        if (NextCell != ListAnchor) {
            //
            // Check to make sure that our Blink points to where we just
            // came from.
            //
            if (SecurityCell->Blink != LastCell) {
                CMLOG(CML_MAJOR, CMS_SEC) {
                    KdPrint(("  Invalid Blink (%ld) on security cell %ld\n",SecurityCell->Blink, NextCell));
                    KdPrint(("  should point to %ld\n", LastCell));
                }
                return(FALSE);
            }
        }
        CMLOG(CML_MINOR, CMS_SEC) {
            KdPrint(("CmpValidSD:  SD shared by %d nodes\n",SecurityCell->ReferenceCount));
        }
        if (!SeValidSecurityDescriptor(SecurityCell->DescriptorLength, &SecurityCell->Descriptor)) {
            CMLOG(CML_MAJOR, CMS_SEC) {
                CmpDumpSecurityDescriptor(&SecurityCell->Descriptor,"INVALID DESCRIPTOR");
            }
            return(FALSE);
        }
        SetUsed(Hive, NextCell);
        LastCell = NextCell;
        NextCell = SecurityCell->Flink;
    } while ( NextCell != ListAnchor );
    return(TRUE);
}
