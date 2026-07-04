/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmnotify.c
 * PURPOSE:         Configuration Manager - Wrappers for Hive Operations
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#define NDEBUG
#include "debug.h"

/* PRIVATE FUNCTIONS *********************************************************/

/**
 * @brief Report a change in registry to a listener represented by the post block
 * 
 * This function signals the listener that a change has occurred in the registry,
 * then releases the resources used by the @p PostBlock as the notification session
 * has finished.
 * This is an utility function used by CmpReportNotify to report changes to each
 * CMP_POST_BLOCK attached to a CM_NOTIFY_BLOCK.
 * 
 * @param[in] PostBlock The change notification receiver
 */
VOID
NTAPI
CmpReportNotifyToPostBlock(_In_ PCMP_POST_BLOCK PostBlock)
{
    PCMP_POST_BLOCK SecondaryPostBlock;

    /* Report the operation result using caller-allocated IO_STATUS_BLOCK buffer  */
    if (PostBlock->IoStatusBlock)
    {
        /* Write to an IO_STATUS_BLOCK allocated by an user-mode caller */
        if (PostBlock->OwnerProcess != NULL)
        {
            /* Attach to caller's context if necessary */
            KAPC_STATE ApcState;
            BOOLEAN IsSameProcess = PostBlock->OwnerProcess == &PsGetCurrentProcess()->Pcb;
            if (!IsSameProcess)
                KeStackAttachProcess(PostBlock->OwnerProcess, &ApcState);

            /* Try to write status to buffer */
            _SEH2_TRY
            {
                PostBlock->IoStatusBlock->Status = STATUS_NOTIFY_ENUM_DIR;
                PostBlock->IoStatusBlock->Information = 0;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                NTSTATUS Status = _SEH2_GetExceptionCode();
                DPRINT1("CmpReportNotifyToPostBlock: Writing to IO_STATUS_BLOCK failed with %lx. Process=0x%lx, IoStatusBlock=0x%lx\n", Status, PostBlock->OwnerProcess, PostBlock->IoStatusBlock);
            }
            _SEH2_END;

            if (!IsSameProcess)
                KeUnstackDetachProcess(&ApcState);
        }
        else
        {
            /* PostBlock->OwnerProcess should be set to NULL for kernel-mode callers */
            PostBlock->IoStatusBlock->Status = STATUS_NOTIFY_ENUM_DIR;
            PostBlock->IoStatusBlock->Information = 0;
        }
    }

    /* Signal the event object provided by the user-mode or kernel-mode callers */
    if (PostBlock->Event)
    {
        KeSetEvent(PostBlock->Event, 1, FALSE);
        ObDereferenceObject(PostBlock->Event);
        PostBlock->Event = NULL;
    }

    /* Queue the WorkQueueItem for asynchronous kernel-mode callers */
    if (PostBlock->WorkQueueItem)
    {
        ExQueueWorkItem(PostBlock->WorkQueueItem, PostBlock->WorkQueueType);
    }
    else if (PostBlock->UserApc)
    {
        /* FIXME: TODO */
    }

    /* Detach the PostBlock from its NotifyBlock as the wait is complete */
    RemoveEntryList(&(PostBlock->NotifyList));
    if (PostBlock->SecondaryBlock)
    {
        /* Also detach the PostBlock for the Subordinate key if one was provided */
        SecondaryPostBlock = CONTAINING_RECORD(PostBlock->SecondaryBlock->PostList.Flink, CMP_POST_BLOCK, NotifyList);
        RemoveEntryList(&SecondaryPostBlock->NotifyList);
        /* Free the memory allocated for the SecondaryPostBlock as we don't need it anymore */
        ExFreePoolWithTag(SecondaryPostBlock, TAG_CM_NOTIFY);
        ExFreePoolWithTag(PostBlock->SecondaryBlock, TAG_CM_NOTIFY);
    }
    ExFreePoolWithTag(PostBlock, TAG_CM_NOTIFY);
}

/**
 * @brief Check if @p SubKey is part of @p ParentKey's subtree
 * 
 * This is an utility function used by CmpReportNotify to check
 * if a notification should be send or not when WatchTree flag is set.
 * 
 * @param[in] ParentKey The parent key to check against
 * 
 * @param[in] SubKey The target key
 * 
 * @return TRUE if @p SubKey is a subkey of @p ParentKey, FALSE otherwise
 */
BOOLEAN
NTAPI
CmpIsKcbSubKey(_In_ PCM_KEY_CONTROL_BLOCK ParentKey,
               _In_ PCM_KEY_CONTROL_BLOCK SubKey)
{
    PCM_KEY_CONTROL_BLOCK SubKeyParent;
    ULONG Depth = SubKey->TotalLevels - ParentKey->TotalLevels;

    if (Depth <= 0)
        return FALSE;

    SubKeyParent = SubKey->ParentKcb;
    while (SubKeyParent != NULL && SubKeyParent != ParentKey && Depth >= 0)
    {
        SubKeyParent = SubKeyParent->ParentKcb;
        Depth--;
    }

    return SubKeyParent == ParentKey;
}

/* FUNCTIONS *****************************************************************/

/**
 * @brief Report a change in a registry key
 * 
 * This function is called by CM functions whenever a change occurs in a registry key.
 * It iterates through all attached CM_NOTIFY_BLOCKs to find one that matches the change
 * then reports the change to all attached CMP_POST_BLOCKs.
 * 
 * @param[in] Kcb The changed registry key
 * 
 * @param[in] Hive The registry hive containing the changed key
 * 
 * @param[in] Cell Unused
 * 
 * @param[in] Filter The type of change, can be REG_NOTIFY_CHANGE_NAME, REG_NOTIFY_CHANGE_ATTRIBUTES, REG_NOTIFY_CHANGE_LAST_SET, or REG_NOTIFY_CHANGE_SECURITY
 * 
 */
VOID
NTAPI
CmpReportNotify(IN PCM_KEY_CONTROL_BLOCK Kcb,
                IN PHHIVE Hive,
                IN HCELL_INDEX Cell,
                IN ULONG Filter)
{
    PCMHIVE KeyHive;
    PLIST_ENTRY ListHead, NextEntry;
    PLIST_ENTRY PostListHead, PostNextEntry;
    PCM_NOTIFY_BLOCK NotifyBlock;
    PCMP_POST_BLOCK PostBlock;

    KeyHive = CONTAINING_RECORD(Hive, CMHIVE, Hive);
    ListHead = &(KeyHive->NotifyList);
    if (IsListEmpty(ListHead)) return;

    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        NotifyBlock = CONTAINING_RECORD(NextEntry, CM_NOTIFY_BLOCK, HiveList);
        NextEntry = NextEntry->Flink;

        /* Check if this KCB matches to this NotifyBlock */
        if (NotifyBlock->KeyControlBlock != Kcb && 
            !(NotifyBlock->WatchTree && CmpIsKcbSubKey(NotifyBlock->KeyControlBlock, Kcb)))
            continue;

        /* Check the notification filter */
        if ((NotifyBlock->Filter & Filter) != Filter) 
            continue;

        PostListHead = &(NotifyBlock->PostList);
        if (IsListEmpty(PostListHead))
        {
            /* A notification block is allocated but no one is watching, so this is a pending notification */
            NotifyBlock->NotifyPending = TRUE;
            continue;
        }
        PostNextEntry = PostListHead->Flink;
        while (PostNextEntry != PostListHead)
        {
            PostBlock = CONTAINING_RECORD(PostNextEntry, CMP_POST_BLOCK, NotifyList);
            PostNextEntry = PostNextEntry->Flink;

            if (PostBlock->IsPrimary)
            {
                CmpReportNotifyToPostBlock(PostBlock);
            }
            else
            {
                /* Prevent system from freeing Primary PostBlock's NotifyBlock by locking its KCB */
                PCM_KEY_CONTROL_BLOCK PrimaryKcb = PostBlock->PrimaryNotifyBlock->KeyControlBlock;
                CmpAcquireKcbLockShared(PrimaryKcb);

                CmpReportNotifyToPostBlock(PostBlock->PrimaryPostBlock);

                CmpReleaseKcbLock(PrimaryKcb);
            }
        }
    }
}

/**
 * @brief Terminates a notification session and releases all the resources used by the attached CM_NOTIFY_BLOCK
 * 
 * This function is called when the system is releasing a registry key object,
 * to signal the listeners that the notification session has ended, then releases
 * all the resources used for the notification session.
 * 
 * @param[in] KeyBody The registry key object being released
 * 
 * @param[in] LockHeld TRUE if the KCB lock is already held by the caller, FALSE otherwise
 * 
 */
VOID
NTAPI
CmpFlushNotify(IN PCM_KEY_BODY KeyBody,
               IN BOOLEAN LockHeld)
{
    PLIST_ENTRY ListHead, NextEntry;
    PCMP_POST_BLOCK PostBlock, SecondaryPostBlock;

    /* Lock the KCB so no one would try to make changes
     * to NotifyBlock while we are releasing its resources */
    if (!LockHeld)
        CmpAcquireKcbLockExclusive(KeyBody->KeyControlBlock);

    ListHead = &(KeyBody->NotifyBlock->PostList);
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        PostBlock = CONTAINING_RECORD(NextEntry, CMP_POST_BLOCK, NotifyList);
        NextEntry = NextEntry->Flink;

        /* This shouldn't happen, We don't assign subordinate NotifyBlocks to a KeyBody */
        ASSERT(PostBlock->IsPrimary);

        if (PostBlock->IoStatusBlock)
        {
            _SEH2_TRY
            {
                /* We are ending the notification session without signalling the caller for any change */
                PostBlock->IoStatusBlock->Status = STATUS_NOTIFY_CLEANUP;
                PostBlock->IoStatusBlock->Information = 0;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                NTSTATUS Status = _SEH2_GetExceptionCode();
                DPRINT1("CmpFlushNotify: Writing to IO_STATUS_BLOCK failed with %lx. Process=0x%lx, IoStatusBlock=0x%lx\n", Status, PostBlock->OwnerProcess, PostBlock->IoStatusBlock);
            }
            _SEH2_END;
        }

        if (PostBlock->Event)
        {
            KeSetEvent(PostBlock->Event, 1, FALSE);
            ObDereferenceObject(PostBlock->Event);
        }

        if (PostBlock->UserApc)
            ExFreePoolWithTag(PostBlock->UserApc, TAG_CM_NOTIFY);

        if (PostBlock->SecondaryBlock)
        {
            SecondaryPostBlock = CONTAINING_RECORD(PostBlock->SecondaryBlock->PostList.Flink, CMP_POST_BLOCK, NotifyList);
            RemoveEntryList(&SecondaryPostBlock->NotifyList);
            ExFreePoolWithTag(SecondaryPostBlock, TAG_CM_NOTIFY);
            ExFreePoolWithTag(PostBlock->SecondaryBlock, TAG_CM_NOTIFY);
        }

        RemoveEntryList(&(PostBlock->NotifyList));
        ExFreePoolWithTag(PostBlock, TAG_CM_NOTIFY);
    }

    /* Free the NotifyBlock */
    RemoveEntryList(&(KeyBody->NotifyBlock->HiveList));
    ExFreePoolWithTag(KeyBody->NotifyBlock, TAG_CM_NOTIFY);
    KeyBody->NotifyBlock = NULL;

    /* Unlock the Kcb if we locked it previously */
    if (!LockHeld)
        CmpReleaseKcbLock(KeyBody->KeyControlBlock);
}

