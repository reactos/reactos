/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmapi2.c

Abstract:

    This module contains CM level entry points for the registry,
    particularly those which we don't want to link into tools,
    setup, the boot loader, etc.

Author:

    Bryan M. Willman (bryanwi) 26-Jan-1993

Revision History:

--*/

#include "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmDeleteKey)
#endif


NTSTATUS
CmDeleteKey(
    IN PCM_KEY_BODY KeyBody
    )
/*++

Routine Description:

    Delete a registry key, clean up Notify block.

Arguments:

    KeyBody - pointer to key handle object

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    PCM_KEY_NODE  ptarget;
    PHHIVE      Hive;
    HCELL_INDEX Cell;
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock;

    CMLOG(CML_WORKER, CMS_CM) KdPrint(("CmDeleteKey\n"));

    CmpLockRegistryExclusive();

    //
    // If already marked for deletion, storage is gone, so
    // do nothing and return success.
    //
    KeyControlBlock = KeyBody->KeyControlBlock;

    if (KeyControlBlock->Delete == TRUE) {
        status = STATUS_SUCCESS;
        goto Exit;
    }

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(KeyControlBlock->KeyHive);

    ptarget = KeyControlBlock->KeyNode;

    if ( ((ptarget->SubKeyCounts[Stable] +
           ptarget->SubKeyCounts[Volatile]) == 0) &&
         ((ptarget->Flags & KEY_NO_DELETE) == 0))
    {
        //
        // Cell is NOT marked NO_DELETE and does NOT have children
        // Send Notification while key still present, if delete fails,
        //   we'll have sent a spurious notify, that doesn't matter
        // Delete the actual storage
        //
        Hive = KeyControlBlock->KeyHive;
        Cell = KeyControlBlock->KeyCell;

        CmpReportNotify(
            KeyControlBlock,
            Hive,
            Cell,
            REG_NOTIFY_CHANGE_NAME
            );

        status = CmpFreeKeyByCell(Hive, Cell, TRUE);

        if (NT_SUCCESS(status)) {
            //
            // post any waiting notifies
            //
#ifdef KCB_TO_KEYBODY_LINK
            CmpFlushNotifiesOnKeyBodyList(KeyControlBlock);
#else
            CmpFlushNotify(KeyBody);
#endif

            //
            // Remove kcb out of cache, but do NOT
            // free its storage, CmDelete will do that when
            // the RefCount becomes zero.
            //
            // There are two things that can hold the RefCount non-zero.
            //
            // 1. open handles for this key
            // 2. Fake subKeys that are still in DelayClose.
            //
            // At this point, we have no way of deleting the fake subkeys from cache
            // unless we do a search for the whole cache, which is too expensive.
            // Thus, we decide to either let the fake keys age out of cache or when 
            // someone is doing the lookup for the fake key, then we delete it at that point.
            // See routine CmpCacheLookup in cmparse.c for more details.
            //
            // If the parent has the subkey info or hint cached, free it.
            // Again, registry is locked exclusively, no need to lock KCB.
            //
            ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
            CmpCleanUpSubKeyInfo(KeyControlBlock->ParentKcb);
            KeyControlBlock->Delete = TRUE;
            CmpRemoveKeyControlBlock(KeyControlBlock);
            KeyControlBlock->KeyCell = HCELL_NIL;
            KeyControlBlock->KeyNode = NULL;
        }

    } else {

        status = STATUS_CANNOT_DELETE;

    }

Exit:
    CmpUnlockRegistry();

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(KeyControlBlock->KeyHive);

    return status;
}
