/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/tunnel.c
 * PURPOSE:         Provides the Tunnel Cache implementation for file system drivers.
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 *                  Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

typedef struct {
    RTL_SPLAY_LINKS SplayInfo;
    LIST_ENTRY TimerQueueEntry;
    LARGE_INTEGER Time;
    ULONGLONG DirectoryKey;
    ULONG Flags;
    UNICODE_STRING LongName;
    UNICODE_STRING ShortName;
    PVOID Data;
    ULONG DataLength;
} TUNNEL_NODE_ENTRY, *PTUNNEL_NODE_ENTRY;

ULONG TunnelMaxEntries = 256;
ULONG TunnelMaxAge = 15;
PAGED_LOOKASIDE_LIST TunnelLookasideList;

#define DEFAULT_EXTRA_SIZE (72)
#define DEFAULT_ENTRY_SIZE (sizeof(TUNNEL_NODE_ENTRY) + DEFAULT_EXTRA_SIZE)

#define TUNNEL_FLAG_POOL 0x2
#define TUNNEL_FLAG_KEY_SHORT_NAME 0x1

VOID
FsRtlFreeTunnelNode(
    IN PTUNNEL_NODE_ENTRY CurEntry,
    IN PLIST_ENTRY PoolList OPTIONAL)
{
    if (PoolList)
    {
        /* divert the linked list entry, it's not required anymore, but we need it */ 
        InsertHeadList(PoolList, &CurEntry->TimerQueueEntry);
        return;
    }

    if (CurEntry->Flags & TUNNEL_FLAG_POOL)
        ExFreePool(CurEntry);
    else
        ExFreeToPagedLookasideList(&TunnelLookasideList, CurEntry);
}

VOID
FsRtlRemoveNodeFromTunnel(
    IN PTUNNEL Cache,
    IN PTUNNEL_NODE_ENTRY CurEntry,
    IN PLIST_ENTRY PoolList,
    OUT PBOOLEAN Rebalance)
{
    /* delete entry and rebalance if required */
    if (Rebalance && *Rebalance)
    {
        Cache->Cache = RtlDelete(&CurEntry->SplayInfo);
        /* reset */
        *Rebalance = FALSE;
    }
    else
    {
        RtlDeleteNoSplay(&CurEntry->SplayInfo, &Cache->Cache);
    }

    /* remove entry */
    RemoveEntryList(&CurEntry->TimerQueueEntry);

    /* free node entry */
    FsRtlFreeTunnelNode(CurEntry, PoolList);

    /* decrement node count */
    Cache->NumEntries--;
}

VOID
FsRtlPruneTunnelCache(
    IN PTUNNEL Cache,
    IN PLIST_ENTRY PoolList)
{
    PLIST_ENTRY Entry, NextEntry;
    PTUNNEL_NODE_ENTRY CurEntry;
    LARGE_INTEGER CurTime, OldTime;
    BOOLEAN Rebalance = TRUE;
    PAGED_CODE();

    /* query time */
    KeQuerySystemTime(&CurTime);

    /* subtract maximum node age */
    OldTime.QuadPart = CurTime.QuadPart - TunnelMaxAge;

    /* free all entries */
    Entry = Cache->TimerQueue.Flink;

    while(Entry != &Cache->TimerQueue)
    {
        /* get node entry */
        CurEntry = (PTUNNEL_NODE_ENTRY)CONTAINING_RECORD(Entry, TUNNEL_NODE_ENTRY, TimerQueueEntry);

        /* get next entry */
         NextEntry = Entry->Flink;

        /* prune if expired OR if in advance in time */
        if (CurEntry->Time.QuadPart < OldTime.QuadPart ||
            CurEntry->Time.QuadPart > CurTime.QuadPart)
        {
            FsRtlRemoveNodeFromTunnel(Cache, CurEntry, PoolList, &Rebalance);
        }

        /* move to next entry */
        Entry = NextEntry;
    }

    /* If we have too many entries */
    while (Cache->NumEntries > TunnelMaxEntries)
    {
        CurEntry = (PTUNNEL_NODE_ENTRY)CONTAINING_RECORD(Entry, TUNNEL_NODE_ENTRY, TimerQueueEntry);
        FsRtlRemoveNodeFromTunnel(Cache, CurEntry, PoolList, &Rebalance);
    }
}

VOID
FsRtlGetTunnelParameterValue(
    IN PUNICODE_STRING ParameterName,
    OUT PULONG Value)
{
    UNICODE_STRING Root = RTL_CONSTANT_STRING(L"Registry\\Machine\\System\\CurrentControlSet\\Control\\FileSystem");
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hKey;
    NTSTATUS Status;
    ULONG Length;
    PKEY_VALUE_FULL_INFORMATION Info;

    /* initialize object attributes */
    InitializeObjectAttributes(&ObjectAttributes, &Root, OBJ_CASE_INSENSITIVE, NULL, NULL);

    /* open registry key */
    Status = ZwOpenKey(&hKey, KEY_READ, &ObjectAttributes);

    if (!NT_SUCCESS(Status))
    {
        /* failed to open key */
        return;
    }

    /* query value size */
    Status = ZwQueryValueKey(hKey, ParameterName, KeyValueFullInformation, NULL, 0, &Length);

    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        /* failed to query size */
        ZwClose(hKey);
        return;
    }

    /* allocate buffer */
    Info = ExAllocatePool(PagedPool, Length);

    if (!Info)
    {
         /* out of memory */
        ZwClose(hKey);
        return;
    }

    /* query value */
    Status = ZwQueryValueKey(hKey, ParameterName, KeyValueFullInformation, NULL, 0, &Length);

    if (NT_SUCCESS(Status))
    {
        if (Info->DataLength)
        {
            /* store result */
            *Value = (ULONG)((ULONG_PTR)Info + Info->DataOffset);
        }
    }

    /* free buffer */
    ExFreePool(Info);

    /* close key */
    ZwClose(hKey);
}

VOID
NTAPI
FsRtlInitializeTunnels()
{
    ULONG TunnelEntries;
    UNICODE_STRING MaximumTunnelEntryAgeInSeconds = RTL_CONSTANT_STRING(L"MaximumTunnelEntryAgeInSeconds");
    UNICODE_STRING MaximumTunnelEntries = RTL_CONSTANT_STRING( L"MaximumTunnelEntries");

    /* check for nt */
    if (MmIsThisAnNtAsSystem())
    {
        /* default */
        TunnelMaxEntries = 1024;
    }

    /* check for custom override of max entries*/
    FsRtlGetTunnelParameterValue(&MaximumTunnelEntries, &TunnelMaxEntries);

    /* check for custom override of age*/
    FsRtlGetTunnelParameterValue(&MaximumTunnelEntryAgeInSeconds, &TunnelMaxAge);

    if (!TunnelMaxAge)
    {
        /* no age means no entries */
        TunnelMaxEntries = 0;
    }

    /* get max entries */
    TunnelEntries = TunnelMaxEntries;

    /* convert to ticks */
    TunnelMaxAge *= 10000000;

    if(TunnelMaxEntries <= 65535)
    {
        /* use max 256 entries */
        TunnelEntries = TunnelMaxEntries / 16;
    }

    if(!TunnelEntries && TunnelMaxEntries )
    {
        /* max tunnel entries was too small */
        TunnelEntries = TunnelMaxEntries + 1;
    }

    if (TunnelEntries > 0xFFFF)
    {
        /* max entries is 256 */
        TunnelEntries = 256;
    }

    /* initialize look aside list */
    ExInitializePagedLookasideList(&TunnelLookasideList, NULL, NULL, 0, DEFAULT_ENTRY_SIZE, 0 /*FIXME*/, TunnelEntries);
}

LONG
FsRtlCompareNodeAndKey(
    IN PTUNNEL_NODE_ENTRY CurEntry,
    IN ULONGLONG DirectoryKey,
    IN PUNICODE_STRING KeyString)
{
    PUNICODE_STRING String;
    LONG Ret;

    if (DirectoryKey > CurEntry->DirectoryKey)
    {
        Ret = 1;
    }
    else if (DirectoryKey < CurEntry->DirectoryKey)
    {
        Ret = -1;
    }
    else
    {
        if (CurEntry->Flags & TUNNEL_FLAG_KEY_SHORT_NAME)
        {
            /* use short name as key */
            String = &CurEntry->ShortName;
        }
        else
        {
            /* use long name as key */
            String = &CurEntry->LongName;
        }

        Ret = RtlCompareUnicodeString(KeyString, String, TRUE);
    }

    return Ret;
}

VOID
FsRtlEmptyFreePoolList(
    IN PLIST_ENTRY PoolList)
{
    PLIST_ENTRY CurEntry;
    PTUNNEL_NODE_ENTRY CurNode;

    /* loop over all the entry */
    while (!IsListEmpty(PoolList))
    {
        /* and free them, one by one */
        CurEntry = RemoveHeadList(PoolList);
        CurNode = (PTUNNEL_NODE_ENTRY)CONTAINING_RECORD(CurEntry, TUNNEL_NODE_ENTRY, TimerQueueEntry);
        FsRtlFreeTunnelNode(CurNode, 0);
    }
}

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name FsRtlAddToTunnelCache
 * @implemented
 *
 * FILLME
 *
 * @param Cache
 *        FILLME
 *
 * @param DirectoryKey
 *        FILLME
 *
 * @param ShortName
 *        FILLME
 *
 * @param LongName
 *        FILLME
 *
 * @param KeyByShortName
 *        FILLME
 *
 * @param DataLength
 *        FILLME
 *
 * @param Data
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlAddToTunnelCache(IN PTUNNEL Cache,
                      IN ULONGLONG DirectoryKey,
                      IN PUNICODE_STRING ShortName,
                      IN PUNICODE_STRING LongName,
                      IN BOOLEAN KeyByShortName,
                      IN ULONG DataLength,
                      IN PVOID Data)
{
    PTUNNEL_NODE_ENTRY NodeEntry;
    PRTL_SPLAY_LINKS CurEntry, LastEntry;
    ULONG Length;
    LONG Result = 0;
    BOOLEAN AllocatedFromPool = FALSE;
    PUNICODE_STRING KeyString;
    LIST_ENTRY PoolList;

    PAGED_CODE();

    /* check if tunnel cache is enabled */
    if (!TunnelMaxEntries)
    {
        /* entries are disabled */
        return;
    }

    /* initialize free pool list */
    InitializeListHead(&PoolList);

    /* calculate node length */
    Length = sizeof(TUNNEL_NODE_ENTRY);

    /* add data size */
    Length += DataLength;

    if (ShortName)
    {
        /* add short name length */
        Length += ShortName->Length;
    }

    if (LongName)
    {
        /* add short name length */
        Length += LongName->Length;
    }

    if (Length > DEFAULT_ENTRY_SIZE)
    {
        /* bigger than default entry */
        NodeEntry = ExAllocatePool(NonPagedPool, Length);
        AllocatedFromPool = TRUE;
    }
    else
    {
        /* get standard entry */
        NodeEntry = ExAllocateFromPagedLookasideList(&TunnelLookasideList);
    }

    /* check for success */
    if (!NodeEntry)
    {
         /* out of memory */
         return;
    }

    /* acquire lock */
    ExAcquireFastMutex(&Cache->Mutex);

    /* now search cache for existing entries */
    CurEntry = Cache->Cache;

    /* check which key should be used for search */
    KeyString = (KeyByShortName ? ShortName : LongName);

    /* initialize last entry */
    LastEntry = NULL;

    while(CurEntry)
    {
        /* compare current node */
        Result = FsRtlCompareNodeAndKey((PTUNNEL_NODE_ENTRY)CurEntry, DirectoryKey, KeyString);

        /* backup last entry */
        LastEntry = CurEntry;

        if (Result > 0)
        {
            /* current directory key is bigger */
            CurEntry = CurEntry->LeftChild;
        }
        else
        {
            if (Result == 0)
            {
                /* found equal entry */
                break;
            }

            /* current directory key is smaller */
            CurEntry = CurEntry->RightChild;
        }
    }

    /* initialize node entry */
    RtlInitializeSplayLinks(&NodeEntry->SplayInfo);

    if (CurEntry != NULL)
    {
         /* found existing item */
         if (CurEntry->LeftChild)
         {
             /* update parent */
             RtlInsertAsLeftChild(NodeEntry, CurEntry->LeftChild);
         }

         if (CurEntry->RightChild)
         {
             /* update parent */
             RtlInsertAsRightChild(NodeEntry, CurEntry->RightChild);
         }

         if (CurEntry->Parent == CurEntry)
         {
              /* cur entry was root */
              Cache->Cache = (struct _RTL_SPLAY_LINKS*)NodeEntry;
         }
         else
         {
              /* update parent node */
              if (LastEntry->LeftChild == CurEntry)
              {
                  RtlInsertAsLeftChild(LastEntry, NodeEntry);
              }
              else
              {
                  RtlInsertAsRightChild(LastEntry, NodeEntry);
              }
         }
         
         /* remove entry */
         RemoveEntryList(&((PTUNNEL_NODE_ENTRY)LastEntry)->TimerQueueEntry);

         /* free node entry */
         FsRtlFreeTunnelNode((PTUNNEL_NODE_ENTRY)LastEntry, &PoolList);

         /* decrement node count */
         Cache->NumEntries--;
    }
    else
    {
        if (LastEntry == NULL)
        {
            /* first entry in tunnel cache */
            Cache->Cache = (struct _RTL_SPLAY_LINKS*)NodeEntry;
        }
        else
        {
            if (Result > 0)
            {
                /* new left node */
                RtlInsertAsLeftChild(LastEntry, NodeEntry);
            }
            else
            {
                /* new right node */
                RtlInsertAsRightChild(LastEntry, NodeEntry);
            }
        }
    }

    /* initialize entry */
    KeQuerySystemTime(&NodeEntry->Time);

    NodeEntry->DirectoryKey = DirectoryKey;
    NodeEntry->Flags = (AllocatedFromPool ? TUNNEL_FLAG_POOL : 0x0);
    NodeEntry->Flags |= (KeyByShortName ? TUNNEL_FLAG_KEY_SHORT_NAME : 0x0);

    if (ShortName)
    {
        /* copy short name */
        NodeEntry->ShortName.Length = ShortName->Length;
        NodeEntry->ShortName.MaximumLength = ShortName->Length;
        NodeEntry->ShortName.Buffer = (LPWSTR)((ULONG_PTR)NodeEntry + sizeof(TUNNEL_NODE_ENTRY));

        RtlMoveMemory(NodeEntry->ShortName.Buffer, ShortName->Buffer, ShortName->Length);
    }
    else
    {
        NodeEntry->ShortName.Length = NodeEntry->ShortName.MaximumLength = 0;
        NodeEntry->ShortName.Buffer = NULL;
    }

    if (LongName)
    {
        /* copy long name */
        NodeEntry->LongName.Length = LongName->Length;
        NodeEntry->LongName.MaximumLength = LongName->Length;
        NodeEntry->LongName.Buffer = (LPWSTR)((ULONG_PTR)NodeEntry + sizeof(TUNNEL_NODE_ENTRY) + NodeEntry->ShortName.Length);

        RtlMoveMemory(NodeEntry->LongName.Buffer, LongName->Buffer, LongName->Length);
    }
    else
    {
        NodeEntry->LongName.Length = NodeEntry->LongName.MaximumLength = 0;
        NodeEntry->LongName.Buffer = NULL;
    }

     NodeEntry->DataLength = DataLength;
     NodeEntry->Data = (PVOID)((ULONG_PTR)NodeEntry + sizeof(TUNNEL_NODE_ENTRY) + NodeEntry->ShortName.Length + NodeEntry->LongName.Length);
     RtlMoveMemory(NodeEntry->Data, Data, DataLength);

     /* increment node count */
     Cache->NumEntries++;

     /* insert into list */
     InsertTailList(&Cache->TimerQueue, &NodeEntry->TimerQueueEntry);

     /* prune cache */
     FsRtlPruneTunnelCache(Cache, &PoolList);

     /* release lock */
     ExReleaseFastMutex(&Cache->Mutex);

     /* free pool list */
     FsRtlEmptyFreePoolList(&PoolList);
}

/*++
 * @name FsRtlDeleteKeyFromTunnelCache
 * @implemented
 *
 * FILLME
 *
 * @param Cache
 *        FILLME
 *
 * @param DirectoryKey
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlDeleteKeyFromTunnelCache(IN PTUNNEL Cache,
                              IN ULONGLONG DirectoryKey)
{
    BOOLEAN Rebalance = TRUE;
    LIST_ENTRY PoolList;
    PTUNNEL_NODE_ENTRY CurNode;
    PRTL_SPLAY_LINKS CurEntry, LastEntry = NULL, Successors;

    PAGED_CODE();

    /* check if tunnel cache is enabled */
    if (!TunnelMaxEntries)
    {
        /* entries are disabled */
        return;
    }

    /* initialize free pool list */
    InitializeListHead(&PoolList);

    /* acquire lock */
    ExAcquireFastMutex(&Cache->Mutex);

    /* Look for the entry */
    CurEntry = Cache->Cache;
    while (CurEntry)
    {
        CurNode = (PTUNNEL_NODE_ENTRY)CONTAINING_RECORD(CurEntry, TUNNEL_NODE_ENTRY, SplayInfo);

        if (CurNode->DirectoryKey > DirectoryKey)
        {
            /* current directory key is bigger */
            CurEntry = CurEntry->LeftChild;
        }
        else if (CurNode->DirectoryKey < DirectoryKey)
        {
            /* if we have already found one suitable, break */
            if (LastEntry != NULL)
            {
                break;
            }

            /* current directory key is smaller */
            CurEntry = CurEntry->RightChild;
        }
        else
        {
            /* save and look for another */
            LastEntry = CurEntry;
            CurEntry = CurEntry->LeftChild;
        }
    }

    /* was it found? */
    if (LastEntry == NULL)
    {
        /* release tunnel lock */
        ExReleaseFastMutex(&Cache->Mutex);

        return;
    }

    /* delete any matching key */
    do
    {
        CurNode = (PTUNNEL_NODE_ENTRY)CONTAINING_RECORD(LastEntry, TUNNEL_NODE_ENTRY, SplayInfo);

        Successors = RtlRealSuccessor(LastEntry);
        if (CurNode->DirectoryKey != DirectoryKey)
        {
            break;
        }

        /* remove from tunnel */
        FsRtlRemoveNodeFromTunnel(Cache, CurNode, &PoolList, &Rebalance);
        LastEntry = Successors;
    }
    while (LastEntry != NULL);

    /* release tunnel lock */
    ExReleaseFastMutex(&Cache->Mutex);

    /* free pool */
    FsRtlEmptyFreePoolList(&PoolList);
}

/*++
 * @name FsRtlDeleteTunnelCache
 * @implemented
 *
 * FILLME
 *
 * @param Cache
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlDeleteTunnelCache(IN PTUNNEL Cache)
{
    PLIST_ENTRY Entry, NextEntry;
    PTUNNEL_NODE_ENTRY CurEntry;

    PAGED_CODE();

    /* check if tunnel cache is enabled */
    if (!TunnelMaxEntries)
    {
        /* entries are disabled */
        return;
    }

    /* free all entries */
    Entry = Cache->TimerQueue.Flink;

    while(Entry != &Cache->TimerQueue)
    {
        /* get node entry */
        CurEntry = (PTUNNEL_NODE_ENTRY)CONTAINING_RECORD(Entry, TUNNEL_NODE_ENTRY, TimerQueueEntry);

        /* get next entry */
         NextEntry = Entry->Flink;

        /* remove entry from list */
        RemoveEntryList(&CurEntry->TimerQueueEntry);

        /* free entry */
        FsRtlFreeTunnelNode(CurEntry, NULL);

        /* move to next entry */
        Entry = NextEntry;
    }

    /* reset object */
    Cache->Cache = NULL;
    Cache->NumEntries = 0;
    InitializeListHead(&Cache->TimerQueue);
}

/*++
 * @name FsRtlFindInTunnelCache
 * @implemented
 *
 * FILLME
 *
 * @param Cache
 *        FILLME
 *
 * @param DirectoryKey
 *        FILLME
 *
 * @param ShortName
 *        FILLME
 *
 * @param LongName
 *        FILLME
 *
 * @param KeyByShortName
 *        FILLME
 *
 * @param DataLength
 *        FILLME
 *
 * @param Data
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
BOOLEAN
NTAPI
FsRtlFindInTunnelCache(IN PTUNNEL Cache,
                       IN ULONGLONG DirectoryKey,
                       IN PUNICODE_STRING Name,
                       OUT PUNICODE_STRING ShortName,
                       OUT PUNICODE_STRING LongName,
                       IN OUT PULONG DataLength,
                       OUT PVOID Data)
{
    BOOLEAN Ret = FALSE;
    PTUNNEL_NODE_ENTRY CurEntry;
    LIST_ENTRY PoolList;
    //NTSTATUS Status;
    LONG Result;

    PAGED_CODE();

    /* check if tunnel cache is enabled */
    if (!TunnelMaxEntries)
    {
        /* entries are disabled */
        return FALSE;
    }

    /* initialize free pool list */
    InitializeListHead(&PoolList);

    /* acquire tunnel lock */
    ExAcquireFastMutex(&Cache->Mutex);

    /* prune old entries */
    FsRtlPruneTunnelCache(Cache, &PoolList);

    /* now search cache for existing entries */
    CurEntry = (PTUNNEL_NODE_ENTRY)Cache->Cache;

    while(CurEntry)
    {
        /* compare current node */
        Result = FsRtlCompareNodeAndKey(CurEntry, DirectoryKey, Name);

        if (Result > 0)
        {
            /* current directory key is bigger */
            CurEntry = (PTUNNEL_NODE_ENTRY)CurEntry->SplayInfo.LeftChild;
        }
        else
        {
            if (Result == 0)
            {
                /* found equal entry */
                break;
            }

            /* current directory key is smaller */
            CurEntry = (PTUNNEL_NODE_ENTRY)CurEntry->SplayInfo.RightChild;
        }
    }

    if (CurEntry != NULL)
    {
        _SEH2_TRY
        {
            /* copy short name */
            RtlCopyUnicodeString(ShortName, &CurEntry->ShortName);

            /* check size */
            if (LongName->MaximumLength < CurEntry->LongName.Length)
            {
                /* buffer is too small */
                LongName->Buffer = ExAllocatePool(PagedPool, CurEntry->LongName.Length);
                if (LongName->Buffer)
                {
                    LongName->Length = CurEntry->LongName.Length;
                    LongName->MaximumLength = CurEntry->LongName.MaximumLength;
                    RtlMoveMemory(LongName->Buffer, CurEntry->LongName.Buffer, CurEntry->LongName.Length);
                }
            }
            else
            {
                /* buffer is big enough */
                RtlCopyUnicodeString(LongName, &CurEntry->LongName);
            }

            /* copy data */
            RtlMoveMemory(Data, CurEntry->Data, CurEntry->DataLength);

            /* store size */
            *DataLength = CurEntry->DataLength;

            /* done */
            Ret = TRUE;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Get the status */
            //Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

    }

    /* release tunnel lock */
    ExReleaseFastMutex(&Cache->Mutex);

    /* free pool */
    FsRtlEmptyFreePoolList(&PoolList);

    return Ret;
}

/*++
 * @name FsRtlInitializeTunnelCache
 * @implemented
 *
 * FILLME
 *
 * @param Cache
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlInitializeTunnelCache(IN PTUNNEL Cache)
{
    PAGED_CODE();

    /* initialize mutex */
    ExInitializeFastMutex(&Cache->Mutex);

    /* initialize node tree */
    Cache->Cache = NULL;

    /* initialize timer list */
    InitializeListHead(&Cache->TimerQueue);

    /* initialize node count */
    Cache->NumEntries = 0;
}

/* EOF */
