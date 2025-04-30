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

VOID
NTAPI
CmpFlushPostBlock(_In_ PCM_POST_BLOCK PostBlock,
                  _In_ ULONG Filter)
{
    /* Don't flush if it doesn't match our filter
     * Using REG_LEGAL_CHANGE_FILTER to flush all PostBlocks regardless to their filter
     */
    if ((Filter != REG_LEGAL_CHANGE_FILTER) && (PostBlock->Filter & Filter) != Filter)
        return;
        
    /* Signal the event */
    if (PostBlock->Event)
        KeSetEvent(PostBlock->Event, 1, FALSE);

    /* Queue the Work Item
     *
     * REG_LEGAL_CHANGE_FILTER is used to indicate this Post Block is being flushed,
     * because the notification session is closing therefore there won't be a notification.
     */
    if (Filter != REG_LEGAL_CHANGE_FILTER && PostBlock->WorkQueueItem)
        ExQueueWorkItem(PostBlock->WorkQueueItem, PostBlock->WorkQueueType);

    /* Queue the APC routine */
    if (Filter != REG_LEGAL_CHANGE_FILTER && PostBlock->UserApc)
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

        return;
    }

    /* Cleanup resources */
    if (PostBlock->Event)
    {
        ObDereferenceObject(PostBlock->Event);
        ZwClose(PostBlock->EventHandle);
    }

    /* Remove post block entry */
    RemoveEntryList(&(PostBlock->NotifyList));

    /* Free the memory */
    ExFreePoolWithTag(PostBlock, TAG_CM);
}

VOID
NTAPI
CmpFlushAllPostBlocks(_In_ PCM_NOTIFY_BLOCK NotifyBlock,
                      _In_ ULONG Filter,
                      _In_ BOOLEAN IsSubKey)
{
    PLIST_ENTRY ListHead, NextEntry;
    PCM_POST_BLOCK PostBlock;

    /* Get the PostList */
    ListHead = &(NotifyBlock->PostList);
    if (IsListEmpty(ListHead)) return;

    /* Enumerate through NotifyBlock->PostList */
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get PostBlock for current entry */
        PostBlock = CONTAINING_RECORD(NextEntry, CM_POST_BLOCK, NotifyList);
        /* Navigate to the next, since the previous entry might get freed */
        NextEntry = NextEntry->Flink;

        /* Check if the post block likes children */
        if (IsSubKey && !PostBlock->WatchTree)
            continue;

        /* Flush it */
        CmpFlushPostBlock(PostBlock, Filter);
    }
}

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
CmpInsertNewPostBlock(_In_      PCM_NOTIFY_BLOCK NotifyBlock,
                      _In_opt_  HANDLE EventHandle,
                      _In_opt_  PKEVENT EventObject,
                      _In_opt_  PIO_APC_ROUTINE ApcRoutine,
                      _In_opt_  PVOID ApcContext,
                      _Out_     PCM_POST_BLOCK *Result)
{
    NTSTATUS Status;
    HANDLE LocalEventHandle;
    PKEVENT LocalEventObject;
    PKAPC LocalApc;
    PCM_POST_BLOCK PostBlock;
    PETHREAD CurrentThread;

    /* EventHandle must be provided when an EventObject is provided */
    ASSERT(!(EventHandle == NULL && EventObject != NULL));

    if (EventHandle == NULL)
    {
        /* No event notification is requested */
        LocalEventHandle = NULL;
        LocalEventObject = NULL;
    }
    else if (EventObject == NULL)
    {
        /* Event is provided by its handle, open it */
        LocalEventHandle = EventHandle;
        
        Status = ObReferenceObjectByHandle(EventHandle,
                                           EVENT_MODIFY_STATE,
                                           ExEventObjectType,
                                           KernelMode,
                                           (PVOID*)&LocalEventObject,
                                           NULL);
        if (!NT_SUCCESS(Status))
            goto Failure;
    }
    else
    {
        /* Both event handle and event objects are provided */
        LocalEventHandle = EventHandle;
        LocalEventObject = EventObject;
    }

    /* Initialize KAPC if an APC routine provided */
    if (ApcRoutine)
    {
        CurrentThread = PsGetCurrentThread();
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
    PostBlock = ExAllocatePoolWithTag(NonPagedPool, sizeof(CM_POST_BLOCK), TAG_CM);
    if (!PostBlock)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Failure;
    }
    RtlZeroMemory(PostBlock, sizeof(CM_POST_BLOCK));

    /* Initialize list heads */
    InitializeListHead(&(PostBlock->NotifyList));

    /* Fill fields */
    PostBlock->EventHandle = LocalEventHandle;
    PostBlock->Event = LocalEventObject;
    PostBlock->UserApc = LocalApc;

    /* Insert to NotifyBlock */
    InsertHeadList(&(NotifyBlock->PostList), &(PostBlock->NotifyList));

    *Result = PostBlock;
    return STATUS_SUCCESS;

Failure:
    /* Free the event resources if we own them */
    if (LocalEventObject != EventObject)
        ObDereferenceObject(LocalEventObject);
    if (LocalEventHandle != EventHandle)
        ZwClose(LocalEventHandle);
    if (LocalApc)
        ExFreePoolWithTag(LocalApc, TAG_CM);
    /*if (PostBlock)
        ExFreePoolWithTag(PostBlock, TAG_CM);*/
    return Status;
}

VOID
NTAPI
CmpReportNotify(IN PCM_KEY_CONTROL_BLOCK Kcb,
                IN PHHIVE Hive,
                IN HCELL_INDEX Cell,
                IN ULONG Filter)
{
    PCMHIVE KeyHive;
    PLIST_ENTRY ListHead, NextEntry;
    PCM_NOTIFY_BLOCK NotifyBlock;
    BOOLEAN IsSubKey = FALSE;

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

        /* Check if the NotifyBlock is paired with this Kcb */
        if (NotifyBlock->KeyControlBlock != Kcb)
        {
            if (NotifyBlock->WatchTree)
            {
                /* Check if the Kcb is a subkey of this NotifyBlock's Kcb */

                /* This helps to optimize our search for our parents */
                int Depth = Kcb->TotalLevels - NotifyBlock->KeyControlBlock->TotalLevels;

                /* A subkey must be deeper than its parent */
                if (Depth < 0)
                    goto SkipEntry;

                /* Walk the parents tree, either it reaches to the root or it reaches to the NotifyBlock's KCB */
                PCM_KEY_CONTROL_BLOCK Parent = Kcb->ParentKcb;
                while (Parent != NULL && Parent != NotifyBlock->KeyControlBlock && Depth > 0)
                {
                    Parent = Parent->ParentKcb;
                    Depth--;
                }

                /* I guess we're orphan now */
                if (Parent != NotifyBlock->KeyControlBlock)
                    goto SkipEntry;

                /* We should remember this so we wouldn't notify a post block who doesn't want children */
                IsSubKey = TRUE;
            }
            else
            {
                goto SkipEntry;
            }
        }

        /* Check the notification filter */
        if ((NotifyBlock->Filter & Filter) != Filter) goto SkipEntry;

        /* Report the notification to PostBlocks linked to this NotifyBlock */
        CmpFlushAllPostBlocks(NotifyBlock, Filter, IsSubKey);

SkipEntry:
        /* Navigate to next entry */
        NextEntry = NextEntry->Flink;
    }
}

VOID
NTAPI
CmpFlushNotify(IN PCM_KEY_BODY KeyBody,
               IN BOOLEAN LockHeld)
{
    /* Lock the KCB so no one would try to make changes
     * to NotifyBlock while we are releasing its resources
     */
    if (!LockHeld)
        CmpAcquireKcbLockExclusive(KeyBody->KeyControlBlock);

    /* Flush PostBlocks */
    CmpFlushAllPostBlocks(KeyBody->NotifyBlock, REG_LEGAL_CHANGE_FILTER, FALSE);

    /* Free the NotifyBlock */
    RemoveEntryList(&(KeyBody->NotifyBlock->HiveList));
    ExFreePoolWithTag(KeyBody->NotifyBlock, TAG_CM);
    KeyBody->NotifyBlock = NULL;

    /* Unlock the Kcb if we locked it previously */
    if (!LockHeld)
        CmpReleaseKcbLock(KeyBody->KeyControlBlock);
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
        ZwClose(PostBlock->EventHandle);
        PostBlock->EventHandle = NULL;
    }

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

    DPRINT1("CmpApcRoutineRundown( %p, %p )\n", PostBlock, Apc->SystemArgument2);
    DPRINT1("PostBlock { Event = %p , EventHandle = %p , UserApc = %p }\n", PostBlock->Event, PostBlock->EventHandle, PostBlock->UserApc);
    
    /* TODO: Set IO_STATUS_BLOCK */
    /* NTSTATUS Status = (NTSTATUS)SystemArgument2; */

    /* Cleanup resources */
    if (PostBlock->Event)
    {
        ObDereferenceObject(PostBlock->Event);
        PostBlock->Event = NULL;
        ZwClose(PostBlock->EventHandle);
        PostBlock->EventHandle = NULL;
    }
    
    ExFreePoolWithTag(PostBlock->UserApc, TAG_CM);
    PostBlock->UserApc = NULL;

    ExFreePoolWithTag(PostBlock, TAG_CM);
}
