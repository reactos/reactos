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
    KeSetEvent(PostBlock->Event, 1, FALSE);

    /* FIXME: Handle WorkQueueItem */

    /* FIXME: Handle ApcRoutine */

    /* Cleanup resources */
    ObDereferenceObject(PostBlock->Event);
    ZwClose(PostBlock->EventHandle);

    /* Remove post block entry */
    RemoveEntryList(&(PostBlock->NotifyList));

    /* Free the memory */
    ExFreePoolWithTag(PostBlock, TAG_CM);
}

VOID
NTAPI
CmpFlushAllPostBlocks(_In_ PCM_NOTIFY_BLOCK NotifyBlock,
                      _In_ ULONG Filter)
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

        /* Flush it */
        CmpFlushPostBlock(PostBlock, Filter);
    }
}

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
CmpInsertNewPostBlock(_In_      PCM_NOTIFY_BLOCK NotifyBlock,
                      _In_      ULONG Filter,
                      _In_opt_  HANDLE EventHandle,
                      _In_opt_  PKEVENT EventObject,
                      _Out_     PCM_POST_BLOCK *PostBlockOut)
{
    NTSTATUS Status;
    HANDLE LocalEventHandle;
    PKEVENT LocalEventObject;
    PCM_POST_BLOCK PostBlock;

    /* EventHandle should be provided when an EventObject is provided */
    if (EventHandle == NULL && EventObject != NULL)
        return STATUS_INVALID_PARAMETER;

    if (EventHandle == NULL && EventObject == NULL)
    {
        /* Allocate an event object if it's not provided by the caller */
        Status = CmpCreateEvent(NotificationEvent, &LocalEventHandle, &LocalEventObject);
        if (!NT_SUCCESS(Status))
            return Status;
    }
    else if (EventHandle != NULL && EventObject == NULL)
    {
        /* Open the event handle if the event object is not provided by the caller */
        LocalEventHandle = EventHandle;
        
        Status = ObReferenceObjectByHandle(EventHandle,
                                           EVENT_MODIFY_STATE,
                                           ExEventObjectType,
                                           KernelMode,
                                           (PVOID*)&LocalEventObject,
                                           NULL);
        if (!NT_SUCCESS(Status))
            return Status;
    }
    else
    {
        /* The only reminaing state is, both EventHandle and EventObjects are non-null */
        LocalEventHandle = EventHandle;
        LocalEventObject = EventObject;
    }

    /* Allocate memory */
    PostBlock = ExAllocatePoolWithTag(NonPagedPool, sizeof(CM_POST_BLOCK), TAG_CM);
    if (!PostBlock)
    {
        /* Free the event resources if we own them */
        if (LocalEventObject != EventObject)
            ObDereferenceObject(LocalEventObject);
        if (LocalEventHandle != EventHandle)
            ZwClose(LocalEventHandle);

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(PostBlock, sizeof(CM_POST_BLOCK));

    /* Initialize list heads */
    InitializeListHead(&(PostBlock->NotifyList));

    /* Fill fields */
    PostBlock->Filter = Filter;
    PostBlock->EventHandle = LocalEventHandle;
    PostBlock->Event = LocalEventObject;

    /* Insert to NotifyBlock */
    InsertHeadList(&(PostBlock->NotifyList), &(NotifyBlock->PostList));

    *PostBlockOut = PostBlock;
    return STATUS_SUCCESS;
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
        if (NotifyBlock->KeyControlBlock != Kcb) goto SkipEntry;

        /* Check the notification filter */
        if ((NotifyBlock->Filter & Filter) != Filter) goto SkipEntry;

        /* Report the notification to PostBlocks linked to this NotifyBlock */
        CmpFlushAllPostBlocks(NotifyBlock, Filter);

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
    CmpFlushAllPostBlocks(KeyBody->NotifyBlock, REG_LEGAL_CHANGE_FILTER);

    /* Free the NotifyBlock */
    RemoveEntryList(&(KeyBody->NotifyBlock->HiveList));
    ExFreePoolWithTag(KeyBody->NotifyBlock, TAG_CM);
    KeyBody->NotifyBlock = NULL;

    /* Unlock the Kcb if we locked it previously */
    if (!LockHeld)
        CmpReleaseKcbLock(KeyBody->KeyControlBlock);
}

