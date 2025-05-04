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

/* INTERNAL FUNCTIONS ********************************************************/

/**
 * @brief
 * Checks if SubKey is part of ParentKey's tree.
 * 
 * @param[in] ParentKey
 * KeyControlBlock for the parent key.
 * 
 * @param[in] SubKey
 * KeyControlBlock for the potential subkey of ParentKey.
 * 
 * @return
 * TRUE is ParentKey is found while walking SubKey's parents, FALSE if not.
 */
BOOLEAN
NTAPI
CmpIsKcbSubKey(_In_ PCM_KEY_CONTROL_BLOCK ParentKey,
               _In_ PCM_KEY_CONTROL_BLOCK SubKey)
{
    ULONG Depth;
    PCM_KEY_CONTROL_BLOCK SubKeyParent;

    /* This helps to optimize our search for our parents */
    Depth = SubKey->TotalLevels - ParentKey->TotalLevels;
    /* A subkey must be deeper than its parent */
    if (Depth <= 0)
        return FALSE;

    /* Walk the parents tree, either it reaches to the root or it reaches to the NotifyBlock's KCB */
    SubKeyParent = SubKey->ParentKcb;
    while (SubKeyParent != NULL && SubKeyParent != ParentKey && Depth >= 0)
    {
        SubKeyParent = SubKeyParent->ParentKcb;
        Depth--;
    }
    return SubKeyParent == ParentKey;
}

/**
 * @brief
 * Helper function for releasing resources allocated for Subordinate objects in a NotifyBlock
 */
VOID
NTAPI
CmpPostBlockFreeSubordinates(_In_ ULONG Count,
                             _In_ PCM_NOTIFY_BLOCK* Subordinates)
{
    PCM_POST_BLOCK PostBlock;

    if (Count <= 0) return;

    for (int i = 0; i < Count; i++)
    {
        /* Every subordinate has one PostBlock, because the KeyBody is allocated internally just for us and we didn't allocate more */
        PostBlock = CONTAINING_RECORD(Subordinates[i]->PostList.Flink, CM_POST_BLOCK, NotifyList);
        RemoveEntryList(&PostBlock->NotifyList);
        ExFreePoolWithTag(PostBlock, TAG_CM);
    }

    /* At last free the array itself */
    ExFreePoolWithTag(Subordinates);
}

/**
 * @brief
 * Helper function for sending change notification to a PostBlock
 */
VOID
NTAPI
CmpNotifyPostBlock(_In_ PCM_POST_BLOCK PostBlock)
{
    /* Signal the event */
    if (PostBlock->Event)
        KeSetEvent(PostBlock->Event, 1, FALSE);

    /* Queue the Work Item */
    if (PostBlock->WorkQueueItem)
        ExQueueWorkItem(PostBlock->WorkQueueItem, PostBlock->WorkQueueType);

    /* Queue the APC routine */
    if (PostBlock->UserApc)
    {
        KeInsertQueueApc(PostBlock->UserApc, PostBlock, (PVOID)STATUS_NOTIFY_ENUM_DIR, 0);

        /* We can't free the resource, yet
         * There's an APC routine to be called so we still need them
         * Let the CmpApcKernelRoutine handle it for us
         */

        /* Even though we are not freeing the resources, we don't want the PostBlock be triggered twice 
         * Remove it from the post list
         */
        RemoveEntryList(&(PostBlock->NotifyList));
        CmpPostBlockFreeSubordinates(PostBlock->SubCount, PostBlock->SubNotifyBlocks);
        PostBlock->SubCount = 0;
        PostBlock->SubNotifyBlocks = NULL;

        return;
    }

    /* Cleanup resources */
    if (PostBlock->Event)
        ObDereferenceObject(PostBlock->Event);

    /* WorkQueueItem should be handled by the caller, not us */

    /* Remove post block entry */
    RemoveEntryList(&(PostBlock->NotifyList));

    /* Release subordinate NotifyBlocks */
    CmpPostBlockFreeSubordinates(PostBlock->SubCount, PostBlock->SubNotifyBlocks);

    /* Free the memory */
    ExFreePoolWithTag(PostBlock, TAG_CM);
}

/* APC ROUTINES **************************************************************/

VOID
NTAPI 
CmpApcKernelRoutine(_In_ PKAPC Apc,
                    _Inout_ PKNORMAL_ROUTINE *NormalRoutine OPTIONAL,
                    _Inout_ PVOID *NormalContext OPTIONAL,
                    _Inout_ PVOID *SystemArgument1 OPTIONAL,
                    _Inout_ PVOID *SystemArgument2 OPTIONAL)
{
    PCM_POST_BLOCK PostBlock = (PCM_POST_BLOCK)*SystemArgument1;

    /* TODO: Set IO_STATUS_BLOCK */
    /* NTSTATUS Status = (NTSTATUS)SystemArgument2; */

    /* Cleanup resources */
    if (PostBlock->Event)
    {
        ObDereferenceObject(PostBlock->Event);
        PostBlock->Event = NULL;
    }

    /* We released subordinates resources before queueing the APC, so there's no need to release them here again */

    /* it's safe to release the KAPC here */
    ExFreePoolWithTag(PostBlock->UserApc, TAG_CM);
    PostBlock->UserApc = NULL;

    ExFreePoolWithTag(PostBlock, TAG_CM);
    SystemArgument1 = NULL;
}

VOID
NTAPI 
CmpApcRoutineRundown(_In_ PKAPC Apc)
{
    if (!Apc->SystemArgument1) return;

    PCM_POST_BLOCK PostBlock = (PCM_POST_BLOCK)Apc->SystemArgument1;

    /* TODO: Set IO_STATUS_BLOCK */
    /* NTSTATUS Status = (NTSTATUS)SystemArgument2; */

    /* Cleanup resources */
    if (PostBlock->Event)
    {
        ObDereferenceObject(PostBlock->Event);
        PostBlock->Event = NULL;
    }
    
    ExFreePoolWithTag(PostBlock->UserApc, TAG_CM);
    PostBlock->UserApc = NULL;

    ExFreePoolWithTag(PostBlock, TAG_CM);
}


/* FUNCTIONS *****************************************************************/

/**
 * @brief
 * Report a change notifications to its listeners
 * 
 * @param[in] Kcb
 * The KeyControlBlock of the changed registry key
 * 
 * @param[in] Hive
 * The registry hive containing the KCB, this is used for enumerating NotifyBlocks
 * 
 * @param[in] Cell
 * This parameter is unused.
 * 
 * @param[in] Filter
 * The notification type, one of REG_CHANGE_FILTER_* enum values.
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
    PCM_POST_BLOCK PostBlock;

    /* Find the CMHIVE linked to this Hive */
    KeyHive = CONTAINING_RECORD(Hive, CMHIVE, Hive);
    
    /* Get NotifyBlock list on the Hive */
    ListHead = &(KeyHive->NotifyList);
    if (IsListEmpty(ListHead)) return;

    /* Enumerate through CMHIVE->NotifyList */
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

        /* Enumerate PostBlocks */
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
            PostBlock = CONTAINING_RECORD(PostNextEntry, CM_POST_BLOCK, NotifyList);
            PostNextEntry = PostNextEntry->Flink;

            if (PostBlock->IsMasterPostBlock)
            {
                CmpNotifyPostBlock(PostBlock);
            }
            else
            {
                /* Let the master handle the notification */

                /* Hold a copy of the master KCB because we might get freed later */
                PCM_KEY_CONTROL_BLOCK MasterKcb = PostBlock->MasterNotifyBlock->KeyControlBlock;
                /* Lock the master KCB before we do anything with its NotifyBlock */
                CmpAcquireKcbLockShared(MasterKcb);
                
                CmpNotifyPostBlock(PostBlock->MasterPostBlock);
                
                CmpReleaseKcbLock(MasterKcb);
            }
        }
    }
}

/**
 * @brief
 * Free the resources allocated for a NotifyBlock on a KeyBody. 
 * This is called when system is deleting a key body object, or it's closing a handle to a registry key.
 * 
 * @param[in] LockHeld
 * TRUE if calling while the KCB is already locked, FALSE if it not
 */
VOID
NTAPI
CmpFlushNotify(IN PCM_KEY_BODY KeyBody,
               IN BOOLEAN LockHeld)
{
    PLIST_ENTRY ListHead, NextEntry;
    PCM_POST_BLOCK PostBlock;

    /* Lock the KCB so no one would try to make changes
     * to NotifyBlock while we are releasing its resources
     */
    if (!LockHeld)
        CmpAcquireKcbLockExclusive(KeyBody->KeyControlBlock);

    /* Enumerate PostBlocks */
    ListHead = &(KeyBody->NotifyBlock->PostList);
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        PostBlock = CONTAINING_RECORD(NextEntry, CM_POST_BLOCK, NotifyList);
        NextEntry = NextEntry->Flink;

        /* This shouldn't happen, We don't assign subordinate NotifyBlocks to a KeyBody */
        ASSERT(PostBlock->IsMasterPostBlock);

        if (PostBlock->Event)
            ObDereferenceObject(PostBlock->Event);

        if (PostBlock->UserApc)
            ExFreePoolWithTag(PostBlock->UserApc, TAG_CM);

        RemoveEntryList(&(PostBlock->NotifyList));
        CmpPostBlockFreeSubordinates(PostBlock->SubCount, PostBlock->SubNotifyBlocks);
        ExFreePoolWithTag(PostBlock, TAG_CM);
    }

    /* Free the NotifyBlock */
    RemoveEntryList(&(KeyBody->NotifyBlock->HiveList));
    ExFreePoolWithTag(KeyBody->NotifyBlock, TAG_CM);
    KeyBody->NotifyBlock = NULL;

    /* Unlock the Kcb if we locked it previously */
    if (!LockHeld)
        CmpReleaseKcbLock(KeyBody->KeyControlBlock);
}

/**
 * @brief
 * Allocate a NotifyBlock for a registry key, and attach it to the hive.
 * KeyBody->NotifyBlock should be set manually.
 * 
 * @param[in] KeyBody
 * The registry key to be watched for notifications
 * 
 * @param[in] Filter
 * The desired notification type, a bit flag of REG_CHANGE_FILTER_* enum values
 */
NTSTATUS
NTAPI
CmpInsertNotifyBlock(_In_ PCM_KEY_BODY KeyBody,
                     _In_ ULONG Filter,
                     _In_ BOOLEAN WatchTree,
                     _Out_ PCM_NOTIFY_BLOCK *Result)
{
    PCM_NOTIFY_BLOCK NotifyBlock;
    PCMHIVE Hive;

    /* Allocate memory */
    NotifyBlock = ExAllocatePoolWithTag(NonPagedPool, sizeof(CM_NOTIFY_BLOCK), TAG_CM);
    if (!NotifyBlock)
    {
        *Result = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(NotifyBlock, sizeof(CM_NOTIFY_BLOCK));

    /* Initialize list heads */
    InitializeListHead(&NotifyBlock->HiveList);
    InitializeListHead(&NotifyBlock->PostList);

    /* Initialize fields */
    NotifyBlock->Filter = Filter;
    NotifyBlock->WatchTree = WatchTree;

    /* Attach to the key object */
    NotifyBlock->KeyControlBlock = KeyBody->KeyControlBlock;
    NotifyBlock->KeyBody = KeyBody;
    /* Let the caller attach us to the KeyBody, because if this is a subordinate our owner would be the master not the KeyBody */
    /*KeyBody->NotifyBlock = NotifyBlock;*/

    /* Attach to the hive */
    Hive = CONTAINING_RECORD(KeyBody->KeyControlBlock->KeyHive, CMHIVE, Hive);
    InsertHeadList(&(Hive->NotifyList), &(NotifyBlock->HiveList));

    *Result = NotifyBlock;
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Allocate a PostBlock and insert it to a NotifyBlock.
 * A PostBlock holds information for a notification callback.
 * 
 * @param[in] NotifyBlock
 * The NotifyBlock for attaching the PostBlock to.
 * 
 * @param[in] EventObject
 * The event object to signal when a notification occured, will be dereferenced after signalling the event.
 * 
 * @param[in] ApcRoutine
 * The user-mode APC routine to be queued when a notification occured
 * 
 * @param[in] ApcContext
 * The parameter to be passed to the ApcRoutine
 */
NTSTATUS
NTAPI
CmpInsertPostBlock(_In_      PCM_NOTIFY_BLOCK NotifyBlock,
                   _In_opt_  PKEVENT EventObject,
                   _In_opt_  PIO_APC_ROUTINE ApcRoutine,
                   _In_opt_  PVOID ApcContext,
                   _Out_     PCM_POST_BLOCK *Result)
{
    PKAPC LocalApc;
    PCM_POST_BLOCK PostBlock;

    /* Initialize KAPC if an APC routine provided */
    if (ApcRoutine)
    {
        PETHREAD CurrentThread = PsGetCurrentThread();
        LocalApc = ExAllocatePoolWithTag(NonPagedPool, sizeof(KAPC), TAG_CM);
        if (!LocalApc)
            return STATUS_INSUFFICIENT_RESOURCES;

        KeInitializeApc(LocalApc,
                        &CurrentThread->Tcb,
                        CurrentApcEnvironment,
                        CmpApcKernelRoutine,
                        CmpApcRoutineRundown,
                        (PKNORMAL_ROUTINE)ApcRoutine,
                        UserMode,
                        ApcContext);
    }
    else
    {
        LocalApc = NULL;
    }

    /* Allocate memory */
    PostBlock = ExAllocatePoolZero(NonPagedPool, sizeof(CM_POST_BLOCK), TAG_CM);
    if (!PostBlock)
    {
        ExFreePoolWithTag(LocalApc, TAG_CM);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialize list heads */
    InitializeListHead(&(PostBlock->NotifyList));

    /* Fill fields */
    PostBlock->IsMasterPostBlock = TRUE;
    PostBlock->Event = EventObject;
    PostBlock->UserApc = LocalApc;

    /* Insert to NotifyBlock */
    InsertHeadList(&(NotifyBlock->PostList), &(PostBlock->NotifyList));

    *Result = PostBlock;
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Allocate a PostBlock and insert it to a NotifyBlock for a subordinate object.
 * The PostBlock will be released by the master NotifyBlock.
 * 
 * @param[in] NotifyBlock
 * The subordinate NotifyBlock to insert the PostBlock to.
 * 
 * @param[in] MasterNotifyBlock
 * The NotifyBlock for the master registry key
 * 
 * @param[in] MasterPostBlock
 * The PostBlock allocated for the master registry key
 */
NTSTATUS
NTAPI
CmpInsertSubPostBlock(_In_  PCM_NOTIFY_BLOCK NotifyBlock,
                      _In_  PCM_NOTIFY_BLOCK MasterNotifyBlock,
                      _In_  PCM_POST_BLOCK MasterPostBlock,
                      _Out_ PCM_POST_BLOCK *Result)
{
    PCM_POST_BLOCK PostBlock;

    /* Allocate memory */
    PostBlock = ExAllocatePoolZero(NonPagedPool, sizeof(CM_POST_BLOCK), TAG_CM);
    if (!PostBlock)
    {
        *Result = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    PostBlock->IsMasterPostBlock = FALSE;

    /* Attach to master NotifyBlock */
    PostBlock->MasterNotifyBlock = MasterNotifyBlock;
    PostBlock->MasterPostBlock = MasterPostBlock;

    /* Attach to our NotifyBlock */
    InsertHeadList(&(NotifyBlock->PostList), &(PostBlock->NotifyList));

    *Result = PostBlock;
    return STATUS_SUCCESS;
}
