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

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
CmpReportNotifyToPostBlocks(_In_ PCM_NOTIFY_BLOCK NotifyBlock,
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
        /* Get the post block */
        PostBlock = CONTAINING_RECORD(NextEntry, CM_POST_BLOCK, NotifyList);

        /* Check the filter */
        if ((PostBlock->Filter & Filter) != Filter)
        {
            /* The listener doesn't want this type of notification, skip it */
            NextEntry = NextEntry->Flink;
            continue;
        }

        /* Signal the event */
        KeSetEvent(PostBlock->Event, 1, FALSE);

        /* FIXME: Support for ApcRoutine */

        /* Release the Post Block */
        NextEntry = NextEntry->Flink;
        /* Free the event object */
        ObDereferenceObject(PostBlock->Event);
        ZwClose(PostBlock->EventHandle);
        /* Remove post block entry */
        RemoveEntryList(&(PostBlock->NotifyList));
        /* Free the memory */
        ExFreePoolWithTag(PostBlock, TAG_CM);
    }
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
        CmpReportNotifyToPostBlocks(NotifyBlock, Filter);

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
        /* Get the post block */
        PostBlock = CONTAINING_RECORD(NextEntry, CM_POST_BLOCK, NotifyList);

        /* Signal the event */
        KeSetEvent(PostBlock->Event, 1, FALSE);

        /* Free the event object */
        ObDereferenceObject(PostBlock->Event);
        ZwClose(PostBlock->EventHandle);
        /* Remove post block entry */
        RemoveEntryList(&(PostBlock->NotifyList));
        /* Free the memory */
        ExFreePoolWithTag(PostBlock, TAG_CM);

        NextEntry = ListHead->Flink;
    }

    /* Finally, free the NotifyBlock */
    RemoveEntryList(&(KeyBody->NotifyBlock->HiveList));
    ExFreePoolWithTag(KeyBody->NotifyBlock, TAG_CM);
    KeyBody->NotifyBlock = NULL;

    if (!LockHeld)
        CmpReleaseKcbLock(KeyBody->KeyControlBlock);
}

