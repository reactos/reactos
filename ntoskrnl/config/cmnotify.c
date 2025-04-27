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

VOID
NTAPI
CmpReportNotifyToPostBlock(_In_ PCMP_POST_BLOCK PostBlock)
{
    PCMP_POST_BLOCK SecondaryPostBlock;

    if (PostBlock->IoStatusBlock)
    {
        KAPC_STATE ApcState;
        BOOLEAN IsSameProcess = PostBlock->OwnerProcess == &PsGetCurrentProcess()->Pcb;

        if (!IsSameProcess)
            KeStackAttachProcess(PostBlock->OwnerProcess, &ApcState);

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

    if (PostBlock->Event)
    {
        KeSetEvent(PostBlock->Event, 1, FALSE);
        ObDereferenceObject(PostBlock->Event);
        PostBlock->Event = NULL;
    }

    if (PostBlock->WorkQueueItem)
        ExQueueWorkItem(PostBlock->WorkQueueItem, PostBlock->WorkQueueType);

    if (PostBlock->UserApc)
    {
        /* FIXME: TODO */
        return;
    }

    RemoveEntryList(&(PostBlock->NotifyList));
    if (PostBlock->SecondaryBlock)
    {
        SecondaryPostBlock = CONTAINING_RECORD(PostBlock->SecondaryBlock->PostList.Flink, CMP_POST_BLOCK, NotifyList);
        RemoveEntryList(&SecondaryPostBlock->NotifyList);
        ExFreePoolWithTag(SecondaryPostBlock, TAG_CM);
        ExFreePoolWithTag(PostBlock->SecondaryBlock, TAG_CM);
    }
    ExFreePoolWithTag(PostBlock, TAG_CM);
}

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

VOID
NTAPI
CmpFlushNotify(IN PCM_KEY_BODY KeyBody,
               IN BOOLEAN LockHeld)
{
    /* FIXME: TODO */
    return;
}

