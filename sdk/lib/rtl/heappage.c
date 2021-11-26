/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/heappage.c
 * PURPOSE:         RTL Page Heap implementation
 * PROGRAMMERS:     Copyright 2011 Aleksey Bragin
 */

/* Useful references:
    http://msdn.microsoft.com/en-us/library/ms220938(VS.80).aspx
    http://blogs.msdn.com/b/jiangyue/archive/2010/03/16/windows-heap-overrun-monitoring.aspx
*/

/* INCLUDES *****************************************************************/

#include <rtl.h>
#include <heap.h>

#define NDEBUG
#include <debug.h>

/* TYPES **********************************************************************/

typedef struct _DPH_BLOCK_INFORMATION
{
     ULONG StartStamp;
     PVOID Heap;
     SIZE_T RequestedSize;
     SIZE_T ActualSize;
     union
     {
          LIST_ENTRY FreeQueue;
          SINGLE_LIST_ENTRY FreePushList;
          WORD TraceIndex;
     };
     PVOID StackTrace;
     ULONG EndStamp;
} DPH_BLOCK_INFORMATION, *PDPH_BLOCK_INFORMATION;

typedef struct _DPH_HEAP_BLOCK
{
     union
     {
          struct _DPH_HEAP_BLOCK *pNextAlloc;
          LIST_ENTRY AvailableEntry;
          RTL_BALANCED_LINKS TableLinks;
     };
     PUCHAR pUserAllocation;
     PUCHAR pVirtualBlock;
     SIZE_T nVirtualBlockSize;
     SIZE_T nVirtualAccessSize;
     SIZE_T nUserRequestedSize;
     SIZE_T nUserActualSize;
     PVOID UserValue;
     ULONG UserFlags;
     PRTL_TRACE_BLOCK StackTrace;
     LIST_ENTRY AdjacencyEntry;
     PUCHAR pVirtualRegion;
} DPH_HEAP_BLOCK, *PDPH_HEAP_BLOCK;

typedef struct _DPH_HEAP_ROOT
{
     ULONG Signature;
     ULONG HeapFlags;
     PHEAP_LOCK HeapCritSect;
     ULONG nRemoteLockAcquired;

     PDPH_HEAP_BLOCK pVirtualStorageListHead;
     PDPH_HEAP_BLOCK pVirtualStorageListTail;
     ULONG nVirtualStorageRanges;
     SIZE_T nVirtualStorageBytes;

     RTL_AVL_TABLE BusyNodesTable;
     PDPH_HEAP_BLOCK NodeToAllocate;
     ULONG nBusyAllocations;
     SIZE_T nBusyAllocationBytesCommitted;

     PDPH_HEAP_BLOCK pFreeAllocationListHead;
     PDPH_HEAP_BLOCK pFreeAllocationListTail;
     ULONG nFreeAllocations;
     SIZE_T nFreeAllocationBytesCommitted;

     LIST_ENTRY AvailableAllocationHead;
     ULONG nAvailableAllocations;
     SIZE_T nAvailableAllocationBytesCommitted;

     PDPH_HEAP_BLOCK pUnusedNodeListHead;
     PDPH_HEAP_BLOCK pUnusedNodeListTail;
     ULONG nUnusedNodes;
     SIZE_T nBusyAllocationBytesAccessible;
     PDPH_HEAP_BLOCK pNodePoolListHead;
     PDPH_HEAP_BLOCK pNodePoolListTail;
     ULONG nNodePools;
     SIZE_T nNodePoolBytes;

     LIST_ENTRY NextHeap;
     ULONG ExtraFlags;
     ULONG Seed;
     PVOID NormalHeap;
     PRTL_TRACE_BLOCK CreateStackTrace;
     PVOID FirstThread;
} DPH_HEAP_ROOT, *PDPH_HEAP_ROOT;

/* GLOBALS ********************************************************************/

BOOLEAN RtlpPageHeapEnabled = FALSE;
ULONG RtlpDphGlobalFlags;
ULONG RtlpPageHeapSizeRangeStart, RtlpPageHeapSizeRangeEnd;
ULONG RtlpPageHeapDllRangeStart, RtlpPageHeapDllRangeEnd;
WCHAR RtlpDphTargetDlls[512];

LIST_ENTRY RtlpDphPageHeapList;
BOOLEAN RtlpDphPageHeapListInitialized;
HEAP_LOCK _RtlpDphPageHeapListLock;
PHEAP_LOCK RtlpDphPageHeapListLock = &_RtlpDphPageHeapListLock;
ULONG RtlpDphPageHeapListLength;
UNICODE_STRING RtlpDphTargetDllsUnicode;

HEAP_LOCK _RtlpDphDelayedFreeQueueLock;
PHEAP_LOCK RtlpDphDelayedFreeQueueLock = &_RtlpDphDelayedFreeQueueLock;
LIST_ENTRY RtlpDphDelayedFreeQueue;
SLIST_HEADER RtlpDphDelayedTemporaryPushList;
SIZE_T RtlpDphMemoryUsedByDelayedFreeBlocks;
ULONG RtlpDphNumberOfDelayedFreeBlocks;

/* Counters */
LONG RtlpDphCounter;
LONG RtlpDphAllocFails;
LONG RtlpDphReleaseFails;
LONG RtlpDphFreeFails;
LONG RtlpDphProtectFails;

#define DPH_RESERVE_SIZE 0x100000
#define DPH_POOL_SIZE 0x4000
#define DPH_FREE_LIST_MINIMUM 8

/* RtlpDphBreakOptions */
#define DPH_BREAK_ON_RESERVE_FAIL 0x01
#define DPH_BREAK_ON_COMMIT_FAIL  0x02
#define DPH_BREAK_ON_RELEASE_FAIL 0x04
#define DPH_BREAK_ON_FREE_FAIL    0x08
#define DPH_BREAK_ON_PROTECT_FAIL 0x10
#define DPH_BREAK_ON_NULL_FREE    0x80

/* RtlpDphDebugOptions */
#define DPH_DEBUG_INTERNAL_VALIDATE 0x01
#define DPH_DEBUG_VERBOSE           0x04

/* DPH ExtraFlags */
#define DPH_EXTRA_LOG_STACK_TRACES 0x02
#define DPH_EXTRA_CHECK_UNDERRUN   0x10

/* Fillers */
#define DPH_FILL 0xEEEEEEEE
#define DPH_FILL_START_STAMP_1 0xABCDBBBB
#define DPH_FILL_START_STAMP_2 0xABCDBBBA
#define DPH_FILL_END_STAMP_1   0xDCBABBBB
#define DPH_FILL_END_STAMP_2   0xDCBABBBA
#define DPH_FILL_SUFFIX        0xD0
#define DPH_FILL_INFIX         0xC0

/* Validation info flags */
#define DPH_VALINFO_BAD_START_STAMP      0x01
#define DPH_VALINFO_BAD_END_STAMP        0x02
#define DPH_VALINFO_BAD_POINTER          0x04
#define DPH_VALINFO_BAD_PREFIX_PATTERN   0x08
#define DPH_VALINFO_BAD_SUFFIX_PATTERN   0x10
#define DPH_VALINFO_EXCEPTION            0x20
#define DPH_VALINFO_1                    0x40
#define DPH_VALINFO_BAD_INFIX_PATTERN    0x80
#define DPH_VALINFO_ALREADY_FREED        0x100
#define DPH_VALINFO_CORRUPTED_AFTER_FREE 0x200

/* Signatures */
#define DPH_SIGNATURE 0xFFEEDDCC

/* Biased pointer macros */
#define IS_BIASED_POINTER(ptr) ((ULONG_PTR)(ptr) & 1)
#define POINTER_REMOVE_BIAS(ptr) ((ULONG_PTR)(ptr) & ~(ULONG_PTR)1)
#define POINTER_ADD_BIAS(ptr) ((ULONG_PTR)(ptr) | 1)


ULONG RtlpDphBreakOptions = 0;//0xFFFFFFFF;
ULONG RtlpDphDebugOptions;

/* FUNCTIONS ******************************************************************/

BOOLEAN NTAPI
RtlpDphGrowVirtual(PDPH_HEAP_ROOT DphRoot, SIZE_T Size);

BOOLEAN NTAPI
RtlpDphIsNormalFreeHeapBlock(PVOID Block, PULONG ValidationInformation, BOOLEAN CheckFillers);

VOID NTAPI
RtlpDphReportCorruptedBlock(PDPH_HEAP_ROOT DphRoot, ULONG Reserved, PVOID Block, ULONG ValidationInfo);

BOOLEAN NTAPI
RtlpDphNormalHeapValidate(PDPH_HEAP_ROOT DphRoot, ULONG Flags, PVOID BaseAddress);


VOID NTAPI
RtlpDphRaiseException(NTSTATUS Status)
{
    EXCEPTION_RECORD Exception;

    /* Initialize exception record */
    Exception.ExceptionCode = Status;
    Exception.ExceptionAddress = RtlpDphRaiseException;
    Exception.ExceptionFlags = 0;
    Exception.ExceptionRecord = NULL;
    Exception.NumberParameters = 0;

    /* Raise the exception */
    RtlRaiseException(&Exception);
}

PVOID NTAPI
RtlpDphPointerFromHandle(PVOID Handle)
{
    PHEAP NormalHeap = (PHEAP)Handle;
    PDPH_HEAP_ROOT DphHeap = (PDPH_HEAP_ROOT)((PUCHAR)Handle + PAGE_SIZE);

    if (NormalHeap->ForceFlags & HEAP_FLAG_PAGE_ALLOCS)
    {
        if (DphHeap->Signature == DPH_SIGNATURE)
            return DphHeap;
    }

    DPRINT1("heap handle with incorrect signature\n");
    DbgBreakPoint();
    return NULL;
}

VOID NTAPI
RtlpDphEnterCriticalSection(PDPH_HEAP_ROOT DphRoot, ULONG Flags)
{
    if (Flags & HEAP_NO_SERIALIZE)
    {
        /* More complex scenario */
        if (!RtlTryEnterHeapLock(DphRoot->HeapCritSect, TRUE))
        {
            if (!DphRoot->nRemoteLockAcquired)
            {
                DPRINT1("multithreaded access in HEAP_NO_SERIALIZE heap\n");
                DbgBreakPoint();

                /* Clear out the no serialize flag */
                DphRoot->HeapFlags &= ~HEAP_NO_SERIALIZE;
            }

            /* Enter the heap's critical section */
            RtlEnterHeapLock(DphRoot->HeapCritSect, TRUE);
        }
    }
    else
    {
        /* Just enter the heap's critical section */
        RtlEnterHeapLock(DphRoot->HeapCritSect, TRUE);
    }
}

VOID NTAPI
RtlpDphLeaveCriticalSection(PDPH_HEAP_ROOT DphRoot)
{
    /* Just leave the heap's critical section */
    RtlLeaveHeapLock(DphRoot->HeapCritSect);
}


VOID NTAPI
RtlpDphPreProcessing(PDPH_HEAP_ROOT DphRoot, ULONG Flags)
{
    RtlpDphEnterCriticalSection(DphRoot, Flags);

    /* FIXME: Validate integrity, internal lists if necessary */
}

VOID NTAPI
RtlpDphPostProcessing(PDPH_HEAP_ROOT DphRoot)
{
    if (!DphRoot) return;

    if (RtlpDphDebugOptions & DPH_DEBUG_INTERNAL_VALIDATE)
    {
        /* FIXME: Validate integrity, internal lists if necessary */
    }

    /* Release the lock */
    RtlpDphLeaveCriticalSection(DphRoot);
}

NTSTATUS NTAPI
RtlpSecMemFreeVirtualMemory(HANDLE Process, PVOID *Base, PSIZE_T Size, ULONG Type)
{
    NTSTATUS Status;
    //PVOID *SavedBase = Base;
    //PSIZE_T SavedSize = Size;

    /* Free the memory */
    Status = ZwFreeVirtualMemory(Process, Base, Size, Type);

    /* Flush secure memory cache if needed and retry freeing */
#if 0
    if (Status == STATUS_INVALID_PAGE_PROTECTION &&
        Process == NtCurrentProcess() &&
        RtlFlushSecureMemoryCache(*SavedBase, *SavedSize))
    {
        Status = ZwFreeVirtualMemory(NtCurrentProcess(), SavedBase, SavedSize, Type);
    }
#endif

    return Status;
}

NTSTATUS NTAPI
RtlpDphAllocateVm(PVOID *Base, SIZE_T Size, ULONG Type, ULONG Protection)
{
    NTSTATUS Status;
    Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                     Base,
                                     0,
                                     &Size,
                                     Type,
                                     Protection);
    DPRINT("Page heap: AllocVm (%p, %Ix, %lx) status %lx\n", Base, Size, Type, Status);
    /* Check for failures */
    if (!NT_SUCCESS(Status))
    {
        if (Type == MEM_RESERVE)
        {
            _InterlockedIncrement(&RtlpDphCounter);
            if (RtlpDphBreakOptions & DPH_BREAK_ON_RESERVE_FAIL)
            {
                DPRINT1("Page heap: AllocVm (%p, %Ix, %lx) failed with %lx\n", Base, Size, Type, Status);
                DbgBreakPoint();
                return Status;
            }
        }
        else
        {
            _InterlockedIncrement(&RtlpDphAllocFails);
            if (RtlpDphBreakOptions & DPH_BREAK_ON_COMMIT_FAIL)
            {
                DPRINT1("Page heap: AllocVm (%p, %Ix, %lx) failed with %lx\n", Base, Size, Type, Status);
                DbgBreakPoint();
                return Status;
            }
        }
    }

    return Status;
}

NTSTATUS NTAPI
RtlpDphFreeVm(PVOID Base, SIZE_T Size, ULONG Type)
{
    NTSTATUS Status;

    /* Free the memory */
    Status = RtlpSecMemFreeVirtualMemory(NtCurrentProcess(), &Base, &Size, Type);
    DPRINT("Page heap: FreeVm (%p, %Ix, %lx) status %lx\n", Base, Size, Type, Status);
    /* Log/report failures */
    if (!NT_SUCCESS(Status))
    {
        if (Type == MEM_RELEASE)
        {
            _InterlockedIncrement(&RtlpDphReleaseFails);
            if (RtlpDphBreakOptions & DPH_BREAK_ON_RELEASE_FAIL)
            {
                DPRINT1("Page heap: FreeVm (%p, %Ix, %lx) failed with %lx\n", Base, Size, Type, Status);
                DbgBreakPoint();
                return Status;
            }
        }
        else
        {
            _InterlockedIncrement(&RtlpDphFreeFails);
            if (RtlpDphBreakOptions & DPH_BREAK_ON_FREE_FAIL)
            {
                DPRINT1("Page heap: FreeVm (%p, %Ix, %lx) failed with %lx\n", Base, Size, Type, Status);
                DbgBreakPoint();
                return Status;
            }
        }
    }

    return Status;
}

NTSTATUS NTAPI
RtlpDphProtectVm(PVOID Base, SIZE_T Size, ULONG Protection)
{
    NTSTATUS Status;
    ULONG OldProtection;

    /* Change protection */
    Status = ZwProtectVirtualMemory(NtCurrentProcess(), &Base, &Size, Protection, &OldProtection);

    /* Log/report failures */
    if (!NT_SUCCESS(Status))
    {
        _InterlockedIncrement(&RtlpDphProtectFails);
        if (RtlpDphBreakOptions & DPH_BREAK_ON_PROTECT_FAIL)
        {
            DPRINT1("Page heap: ProtectVm (%p, %Ix, %lx) failed with %lx\n", Base, Size, Protection, Status);
            DbgBreakPoint();
            return Status;
        }
    }

    return Status;
}

BOOLEAN NTAPI
RtlpDphWritePageHeapBlockInformation(PDPH_HEAP_ROOT DphRoot, PVOID UserAllocation, SIZE_T Size, SIZE_T UserSize)
{
    PDPH_BLOCK_INFORMATION BlockInfo;
    PUCHAR FillPtr;

    /* Get pointer to the block info structure */
    BlockInfo = (PDPH_BLOCK_INFORMATION)UserAllocation - 1;

    /* Set up basic fields */
    BlockInfo->Heap = DphRoot;
    BlockInfo->ActualSize = UserSize;
    BlockInfo->RequestedSize = Size;
    BlockInfo->StartStamp = DPH_FILL_START_STAMP_1;
    BlockInfo->EndStamp = DPH_FILL_END_STAMP_1;

    /* Fill with a pattern */
    FillPtr = (PUCHAR)UserAllocation + Size;
    RtlFillMemory(FillPtr, ROUND_UP(FillPtr, PAGE_SIZE) - (ULONG_PTR)FillPtr, DPH_FILL_SUFFIX);

    /* FIXME: Check if logging stack traces is turned on */
    //if (DphRoot->ExtraFlags &

    return TRUE;
}

VOID NTAPI
RtlpDphPlaceOnBusyList(PDPH_HEAP_ROOT DphRoot, PDPH_HEAP_BLOCK DphNode)
{
    BOOLEAN NewElement;
    PVOID AddressUserData;

    DPRINT("RtlpDphPlaceOnBusyList(%p %p)\n", DphRoot, DphNode);

    /* Add it to the AVL busy nodes table */
    DphRoot->NodeToAllocate = DphNode;
    AddressUserData = RtlInsertElementGenericTableAvl(&DphRoot->BusyNodesTable,
                                                      &DphNode->pUserAllocation,
                                                      sizeof(ULONG_PTR),
                                                      &NewElement);

    ASSERT(AddressUserData == &DphNode->pUserAllocation);
    ASSERT(NewElement == TRUE);

    /* Update heap counters */
    DphRoot->nBusyAllocations++;
    DphRoot->nBusyAllocationBytesAccessible += DphNode->nVirtualAccessSize;
    DphRoot->nBusyAllocationBytesCommitted += DphNode->nVirtualBlockSize;
}

VOID NTAPI
RtlpDphPlaceOnFreeList(PDPH_HEAP_ROOT DphRoot, PDPH_HEAP_BLOCK Node)
{
    DPRINT("RtlpDphPlaceOnFreeList(%p %p)\n", DphRoot, Node);

    /* Node is being added to the tail of the list */
    Node->pNextAlloc = NULL;

    /* Add it to the tail of the linked list */
    if (DphRoot->pFreeAllocationListTail)
        DphRoot->pFreeAllocationListTail->pNextAlloc = Node;
    else
        DphRoot->pFreeAllocationListHead = Node;
    DphRoot->pFreeAllocationListTail = Node;

    /* Update byte counts taking in account this new node */
    DphRoot->nFreeAllocations++;
    DphRoot->nFreeAllocationBytesCommitted += Node->nVirtualBlockSize;
}

VOID NTAPI
RtlpDphPlaceOnPoolList(PDPH_HEAP_ROOT DphRoot, PDPH_HEAP_BLOCK Node)
{
    DPRINT("RtlpDphPlaceOnPoolList(%p %p)\n", DphRoot, Node);

    /* Node is being added to the tail of the list */
    Node->pNextAlloc = NULL;

    /* Add it to the tail of the linked list */
    if (DphRoot->pNodePoolListTail)
        DphRoot->pNodePoolListTail->pNextAlloc = Node;
    else
        DphRoot->pNodePoolListHead = Node;
    DphRoot->pNodePoolListTail = Node;

    /* Update byte counts taking in account this new node */
    DphRoot->nNodePools++;
    DphRoot->nNodePoolBytes += Node->nVirtualBlockSize;
}

VOID NTAPI
RtlpDphPlaceOnVirtualList(PDPH_HEAP_ROOT DphRoot, PDPH_HEAP_BLOCK Node)
{
    DPRINT("RtlpDphPlaceOnVirtualList(%p %p)\n", DphRoot, Node);

    /* Add it to the head of the virtual list */
    Node->pNextAlloc = DphRoot->pVirtualStorageListHead;
    if (!DphRoot->pVirtualStorageListHead)
        DphRoot->pVirtualStorageListTail = Node;
    DphRoot->pVirtualStorageListHead = Node;

    /* Update byte counts taking in account this new node */
    DphRoot->nVirtualStorageRanges++;
    DphRoot->nVirtualStorageBytes += Node->nVirtualBlockSize;
}

PDPH_HEAP_BLOCK NTAPI
RtlpDphTakeNodeFromUnusedList(PDPH_HEAP_ROOT DphRoot)
{
    PDPH_HEAP_BLOCK Node = DphRoot->pUnusedNodeListHead;
    PDPH_HEAP_BLOCK Next;

    DPRINT("RtlpDphTakeNodeFromUnusedList(%p), ret %p\n", DphRoot, Node);

    /* Take the first entry */
    if (!Node) return NULL;

    /* Remove that entry (Node) from the list */
    Next = Node->pNextAlloc;
    if (DphRoot->pUnusedNodeListHead == Node) DphRoot->pUnusedNodeListHead = Next;
    if (DphRoot->pUnusedNodeListTail == Node) DphRoot->pUnusedNodeListTail = NULL;

    /* Decrease amount of unused nodes */
    DphRoot->nUnusedNodes--;

    return Node;
}

VOID NTAPI
RtlpDphReturnNodeToUnusedList(PDPH_HEAP_ROOT DphRoot,
                              PDPH_HEAP_BLOCK Node)
{
    DPRINT("RtlpDphReturnNodeToUnusedList(%p, %p)\n", DphRoot, Node);

    /* Add it back to the head of the unused list */
    Node->pNextAlloc = DphRoot->pUnusedNodeListHead;
    if (!DphRoot->pUnusedNodeListHead)
        DphRoot->pUnusedNodeListTail = Node;
    DphRoot->pUnusedNodeListHead = Node;

    /* Increase amount of unused nodes */
    DphRoot->nUnusedNodes++;
}

VOID NTAPI
RtlpDphRemoveFromAvailableList(PDPH_HEAP_ROOT DphRoot,
                               PDPH_HEAP_BLOCK Node)
{
    /* Make sure Adjacency list pointers are biased */
    //ASSERT(IS_BIASED_POINTER(Node->AdjacencyEntry.Flink));
    //ASSERT(IS_BIASED_POINTER(Node->AdjacencyEntry.Blink));

    DPRINT("RtlpDphRemoveFromAvailableList(%p %p)\n", DphRoot, Node);

    /* Check if it is in the list */
#if 0
    {
        PLIST_ENTRY CurEntry;
        PDPH_HEAP_BLOCK NodeEntry;
        BOOLEAN Found = FALSE;

        /* Find where to put this node according to its virtual address */
        CurEntry = DphRoot->AvailableAllocationHead.Flink;

        while (CurEntry != &DphRoot->AvailableAllocationHead)
        {
            NodeEntry = CONTAINING_RECORD(CurEntry, DPH_HEAP_BLOCK, AvailableEntry);

            if (NodeEntry == Node)
            {
                Found = TRUE;
                break;
            }

            CurEntry = CurEntry->Flink;
        }

        if (!Found)
        {
            DPRINT1("Trying to remove non-existing in availlist node!\n");
            DbgBreakPoint();
        }
    }
#endif

    /* Remove it from the list */
    RemoveEntryList(&Node->AvailableEntry);

    /* Decrease heap counters */
    DphRoot->nAvailableAllocations--;
    DphRoot->nAvailableAllocationBytesCommitted -= Node->nVirtualBlockSize;

    /* Remove bias from the AdjacencyEntry pointer */
    Node->AdjacencyEntry.Flink = (PLIST_ENTRY)POINTER_REMOVE_BIAS(Node->AdjacencyEntry.Flink);
    Node->AdjacencyEntry.Blink = (PLIST_ENTRY)POINTER_REMOVE_BIAS(Node->AdjacencyEntry.Blink);
}

VOID NTAPI
RtlpDphRemoveFromBusyList(PDPH_HEAP_ROOT DphRoot,
                          PDPH_HEAP_BLOCK Node)
{
    BOOLEAN ElementPresent;

    DPRINT("RtlpDphRemoveFromBusyList(%p %p)\n", DphRoot, Node);

    /* Delete it from busy nodes table */
    ElementPresent = RtlDeleteElementGenericTableAvl(&DphRoot->BusyNodesTable, &Node->pUserAllocation);
    ASSERT(ElementPresent == TRUE);

    /* Update counters */
    DphRoot->nBusyAllocations--;
    DphRoot->nBusyAllocationBytesCommitted -= Node->nVirtualBlockSize;
    DphRoot->nBusyAllocationBytesAccessible -= Node->nVirtualAccessSize;
}

VOID NTAPI
RtlpDphRemoveFromFreeList(PDPH_HEAP_ROOT DphRoot,
                          PDPH_HEAP_BLOCK Node,
                          PDPH_HEAP_BLOCK Prev)
{
    PDPH_HEAP_BLOCK Next;

    DPRINT("RtlpDphRemoveFromFreeList(%p %p %p)\n", DphRoot, Node, Prev);

    /* Detach it from the list */
    Next = Node->pNextAlloc;
    if (DphRoot->pFreeAllocationListHead == Node)
        DphRoot->pFreeAllocationListHead = Next;
    if (DphRoot->pFreeAllocationListTail == Node)
        DphRoot->pFreeAllocationListTail = Prev;
    if (Prev) Prev->pNextAlloc = Next;

    /* Decrease heap counters */
    DphRoot->nFreeAllocations--;
    DphRoot->nFreeAllocationBytesCommitted -= Node->nVirtualBlockSize;

    Node->StackTrace = NULL;
}

VOID NTAPI
RtlpDphCoalesceNodeIntoAvailable(PDPH_HEAP_ROOT DphRoot,
                                 PDPH_HEAP_BLOCK Node)
{
    PDPH_HEAP_BLOCK NodeEntry, PrevNode = NULL, NextNode;
    PLIST_ENTRY AvailListHead;
    PLIST_ENTRY CurEntry;

    DPRINT("RtlpDphCoalesceNodeIntoAvailable(%p %p)\n", DphRoot, Node);

    /* Update heap counters */
    DphRoot->nAvailableAllocationBytesCommitted += Node->nVirtualBlockSize;
    DphRoot->nAvailableAllocations++;

    /* Find where to put this node according to its virtual address */
    AvailListHead = &DphRoot->AvailableAllocationHead;

    /* Find a point where to insert an available node */
    CurEntry = AvailListHead->Flink;

    while (CurEntry != AvailListHead)
    {
        NodeEntry = CONTAINING_RECORD(CurEntry, DPH_HEAP_BLOCK, AvailableEntry);
        if (NodeEntry->pVirtualBlock >= Node->pVirtualBlock)
        {
            PrevNode = NodeEntry;
            break;
        }
        CurEntry = CurEntry->Flink;
    }

    if (!PrevNode)
    {
        /* That means either this list is empty, or we should add to the head of it */
        InsertHeadList(AvailListHead, &Node->AvailableEntry);
    }
    else
    {
        /* Check the previous node and merge if possible */
        if (PrevNode->pVirtualBlock + PrevNode->nVirtualBlockSize == Node->pVirtualBlock)
        {
            /* Check they actually belong to the same virtual memory block */
            NTSTATUS Status;
            MEMORY_BASIC_INFORMATION MemoryBasicInfo;

            Status = ZwQueryVirtualMemory(
                ZwCurrentProcess(),
                Node->pVirtualBlock,
                MemoryBasicInformation,
                &MemoryBasicInfo,
                sizeof(MemoryBasicInfo),
                NULL);

            /* There is no way this can fail, we committed this memory! */
            ASSERT(NT_SUCCESS(Status));

            if ((PUCHAR)MemoryBasicInfo.AllocationBase <= PrevNode->pVirtualBlock)
            {
                /* They are adjacent, and from the same VM region. - merge! */
                PrevNode->nVirtualBlockSize += Node->nVirtualBlockSize;
                RtlpDphReturnNodeToUnusedList(DphRoot, Node);
                DphRoot->nAvailableAllocations--;

                Node = PrevNode;
            }
            else
            {
                /* Insert after PrevNode */
                InsertTailList(&PrevNode->AvailableEntry, &Node->AvailableEntry);
            }
        }
        else
        {
            /* Insert after PrevNode */
            InsertTailList(&PrevNode->AvailableEntry, &Node->AvailableEntry);
        }

        /* Now check the next entry after our one */
        if (Node->AvailableEntry.Flink != AvailListHead)
        {
            NextNode = CONTAINING_RECORD(Node->AvailableEntry.Flink, DPH_HEAP_BLOCK, AvailableEntry);
            /* Node is not at the tail of the list, check if it's adjacent */
            if (Node->pVirtualBlock + Node->nVirtualBlockSize == NextNode->pVirtualBlock)
            {
                /* Check they actually belong to the same virtual memory block */
                NTSTATUS Status;
                MEMORY_BASIC_INFORMATION MemoryBasicInfo;

                Status = ZwQueryVirtualMemory(
                    ZwCurrentProcess(),
                    NextNode->pVirtualBlock,
                    MemoryBasicInformation,
                    &MemoryBasicInfo,
                    sizeof(MemoryBasicInfo),
                    NULL);

                /* There is no way this can fail, we committed this memory! */
                ASSERT(NT_SUCCESS(Status));

                if ((PUCHAR)MemoryBasicInfo.AllocationBase <= Node->pVirtualBlock)
                {
                    /* They are adjacent - merge! */
                    Node->nVirtualBlockSize += NextNode->nVirtualBlockSize;

                    /* Remove next entry from the list and put it into unused entries list */
                    RemoveEntryList(&NextNode->AvailableEntry);
                    RtlpDphReturnNodeToUnusedList(DphRoot, NextNode);
                    DphRoot->nAvailableAllocations--;
                }
            }
        }
    }
}

VOID NTAPI
RtlpDphCoalesceFreeIntoAvailable(PDPH_HEAP_ROOT DphRoot,
                                 ULONG LeaveOnFreeList)
{
    PDPH_HEAP_BLOCK Node = DphRoot->pFreeAllocationListHead, Next;
    SIZE_T FreeAllocations = DphRoot->nFreeAllocations;

    /* Make sure requested size is not too big */
    ASSERT(FreeAllocations >= LeaveOnFreeList);

    DPRINT("RtlpDphCoalesceFreeIntoAvailable(%p %lu)\n", DphRoot, LeaveOnFreeList);

    while (Node)
    {
        FreeAllocations--;
        if (FreeAllocations < LeaveOnFreeList) break;

        /* Get the next pointer, because it may be changed after following two calls */
        Next = Node->pNextAlloc;

        /* Remove it from the free list */
        RtlpDphRemoveFromFreeList(DphRoot, Node, NULL);

        /* And put into the available */
        RtlpDphCoalesceNodeIntoAvailable(DphRoot, Node);

        /* Go to the next node */
        Node = Next;
    }
}

VOID NTAPI
RtlpDphAddNewPool(PDPH_HEAP_ROOT DphRoot, PDPH_HEAP_BLOCK NodeBlock, PVOID Virtual, SIZE_T Size, BOOLEAN PlaceOnPool)
{
    PDPH_HEAP_BLOCK DphNode, DphStartNode;
    ULONG NodeCount, i;

    //NodeCount = (Size >> 6) - 1;
    NodeCount = (ULONG)(Size / sizeof(DPH_HEAP_BLOCK));
    DphStartNode = Virtual;

    /* Set pNextAlloc for all blocks */
    for (DphNode = Virtual, i=NodeCount-1; i > 0; i--)
    {
        DphNode->pNextAlloc = DphNode + 1;
        DphNode = DphNode->pNextAlloc;
    }

    /* and the last one */
    DphNode->pNextAlloc = NULL;

    /* Add it to the tail of unused node list */
    if (DphRoot->pUnusedNodeListTail)
        DphRoot->pUnusedNodeListTail->pNextAlloc = DphStartNode;
    else
        DphRoot->pUnusedNodeListHead = DphStartNode;

    DphRoot->pUnusedNodeListTail = DphNode;

    /* Increase counters */
    DphRoot->nUnusedNodes += NodeCount;

    /* Check if we need to place it on the pool list */
    if (PlaceOnPool)
    {
        /* Get a node from the unused list */
        DphNode = RtlpDphTakeNodeFromUnusedList(DphRoot);
        ASSERT(DphNode);

        /* Set its virtual block values */
        DphNode->pVirtualBlock = Virtual;
        DphNode->nVirtualBlockSize = Size;

        /* Place it on the pool list */
        RtlpDphPlaceOnPoolList(DphRoot, DphNode);
    }
}

PDPH_HEAP_BLOCK NTAPI
RtlpDphSearchAvailableMemoryListForBestFit(PDPH_HEAP_ROOT DphRoot,
                                           SIZE_T Size)
{
    PLIST_ENTRY CurEntry;
    PDPH_HEAP_BLOCK Node, NodeFound = NULL;

    CurEntry = DphRoot->AvailableAllocationHead.Flink;

    while (CurEntry != &DphRoot->AvailableAllocationHead)
    {
        /* Get the current available node */
        Node = CONTAINING_RECORD(CurEntry, DPH_HEAP_BLOCK, AvailableEntry);

        /* Check its size */
        if (Node->nVirtualBlockSize >= Size)
        {
            NodeFound = Node;
            break;
        }

        /* Move to the next available entry */
        CurEntry = CurEntry->Flink;
    }

    /* Make sure Adjacency list pointers are biased */
    //ASSERT(IS_BIASED_POINTER(Node->AdjacencyEntry.Flink));
    //ASSERT(IS_BIASED_POINTER(Node->AdjacencyEntry.Blink));

    return NodeFound;
}

PDPH_HEAP_BLOCK NTAPI
RtlpDphFindAvailableMemory(PDPH_HEAP_ROOT DphRoot,
                           SIZE_T Size,
                           BOOLEAN Grow)
{
    PDPH_HEAP_BLOCK Node;
    ULONG NewSize;

    /* Find an available best fitting node */
    Node = RtlpDphSearchAvailableMemoryListForBestFit(DphRoot, Size);

    /* If that didn't work, try to search a smaller one in the loop */
    while (!Node)
    {
        /* Break if the free list becomes too small */
        if (DphRoot->nFreeAllocations <= DPH_FREE_LIST_MINIMUM) break;

        /* Calculate a new free list size */
        NewSize = DphRoot->nFreeAllocations >> 2;
        if (NewSize < DPH_FREE_LIST_MINIMUM) NewSize = DPH_FREE_LIST_MINIMUM;

        /* Coalesce free into available */
        RtlpDphCoalesceFreeIntoAvailable(DphRoot, NewSize);

        /* Try to find an available best fitting node again */
        Node = RtlpDphSearchAvailableMemoryListForBestFit(DphRoot, Size);
    }

    /* If Node is NULL, then we could fix the situation only by
       growing the available VM size */
    if (!Node && Grow)
    {
        /* Grow VM size, if it fails - return failure directly */
        if (!RtlpDphGrowVirtual(DphRoot, Size)) return NULL;

        /* Try to find an available best fitting node again */
        Node = RtlpDphSearchAvailableMemoryListForBestFit(DphRoot, Size);

        if (!Node)
        {
            /* Do the last attempt: coalesce all free into available (if Size fits there) */
            if (DphRoot->nFreeAllocationBytesCommitted + DphRoot->nAvailableAllocationBytesCommitted >= Size)
            {
                /* Coalesce free into available */
                RtlpDphCoalesceFreeIntoAvailable(DphRoot, 0);

                /* Try to find an available best fitting node again */
                Node = RtlpDphSearchAvailableMemoryListForBestFit(DphRoot, Size);
            }
        }
    }

    /* Return node we found */
    return Node;
}

PDPH_HEAP_BLOCK NTAPI
RtlpDphFindBusyMemory(PDPH_HEAP_ROOT DphRoot,
                      PVOID pUserMem)
{
    PDPH_HEAP_BLOCK Node;
    PVOID Ptr;

    /* Lookup busy block in AVL */
    Ptr = RtlLookupElementGenericTableAvl(&DphRoot->BusyNodesTable, &pUserMem);
    if (!Ptr) return NULL;

    /* Restore pointer to the heap block */
    Node = CONTAINING_RECORD(Ptr, DPH_HEAP_BLOCK, pUserAllocation);
    ASSERT(Node->pUserAllocation == pUserMem);
    return Node;
}

NTSTATUS NTAPI
RtlpDphSetProtectionBeforeUse(PDPH_HEAP_ROOT DphRoot, PUCHAR VirtualBlock, ULONG UserSize)
{
    ULONG Protection;
    PVOID Base;

    if (DphRoot->ExtraFlags & DPH_EXTRA_CHECK_UNDERRUN)
    {
        Base = VirtualBlock + PAGE_SIZE;
    }
    else
    {
        Base = VirtualBlock;
    }

    // FIXME: It should be different, but for now it's fine
    Protection = PAGE_READWRITE;

    return RtlpDphProtectVm(Base, UserSize, Protection);
}

NTSTATUS NTAPI
RtlpDphSetProtectionAfterUse(PDPH_HEAP_ROOT DphRoot, /*PUCHAR VirtualBlock*/PDPH_HEAP_BLOCK Node)
{
    ASSERT((Node->nVirtualAccessSize + PAGE_SIZE) <= Node->nVirtualBlockSize);

    // FIXME: Bring stuff here
    if (DphRoot->ExtraFlags & DPH_EXTRA_CHECK_UNDERRUN)
    {
    }
    else
    {
    }

    return STATUS_SUCCESS;
}

PDPH_HEAP_BLOCK NTAPI
RtlpDphAllocateNode(PDPH_HEAP_ROOT DphRoot)
{
    PDPH_HEAP_BLOCK Node;
    NTSTATUS Status;
    SIZE_T Size = DPH_POOL_SIZE, SizeVirtual;
    PVOID Ptr = NULL;

    /* Check for the easy case */
    if (DphRoot->pUnusedNodeListHead)
    {
        /* Just take a node from this list */
        Node = RtlpDphTakeNodeFromUnusedList(DphRoot);
        ASSERT(Node);
        return Node;
    }

    /* There is a need to make free space */
    Node = RtlpDphFindAvailableMemory(DphRoot, DPH_POOL_SIZE, FALSE);

    if (!DphRoot->pUnusedNodeListHead && !Node)
    {
        /* Retry with a smaller request */
        Size = PAGE_SIZE;
        Node = RtlpDphFindAvailableMemory(DphRoot, PAGE_SIZE, FALSE);
    }

    if (!DphRoot->pUnusedNodeListHead)
    {
        if (Node)
        {
            RtlpDphRemoveFromAvailableList(DphRoot, Node);
            Ptr = Node->pVirtualBlock;
            SizeVirtual = Node->nVirtualBlockSize;
        }
        else
        {
            /* No free space, need to alloc a new VM block */
            Size = DPH_POOL_SIZE;
            SizeVirtual = DPH_RESERVE_SIZE;
            Status = RtlpDphAllocateVm(&Ptr, SizeVirtual, MEM_COMMIT, PAGE_NOACCESS);

            if (!NT_SUCCESS(Status))
            {
                /* Retry with a smaller size */
                SizeVirtual = 0x10000;
                Status = RtlpDphAllocateVm(&Ptr, SizeVirtual, MEM_COMMIT, PAGE_NOACCESS);
                if (!NT_SUCCESS(Status)) return NULL;
            }
        }

        /* VM is allocated at this point, set protection */
        Status = RtlpDphProtectVm(Ptr, Size, PAGE_READWRITE);
        if (!NT_SUCCESS(Status))
        {
            if (Node)
            {
                RtlpDphCoalesceNodeIntoAvailable(DphRoot, Node);
            }
            else
            {
                //RtlpDphFreeVm();
                ASSERT(FALSE);
            }

            return NULL;
        }

        /* Zero the memory */
        if (Node) RtlZeroMemory(Ptr, Size);

        /* Add a new pool based on this VM */
        RtlpDphAddNewPool(DphRoot, Node, Ptr, Size, TRUE);

        if (Node)
        {
            if (Node->nVirtualBlockSize > Size)
            {
                Node->pVirtualBlock += Size;
                Node->nVirtualBlockSize -= Size;

                RtlpDphCoalesceNodeIntoAvailable(DphRoot, Node);
            }
            else
            {
                RtlpDphReturnNodeToUnusedList(DphRoot, Node);
            }
        }
        else
        {
            /* The new VM block was just allocated a few code lines ago,
               so initialize it */
            Node = RtlpDphTakeNodeFromUnusedList(DphRoot);
            Node->pVirtualBlock = Ptr;
            Node->nVirtualBlockSize = SizeVirtual;
            RtlpDphPlaceOnVirtualList(DphRoot, Node);

            Node = RtlpDphTakeNodeFromUnusedList(DphRoot);
            Node->pVirtualBlock = (PUCHAR)Ptr + Size;
            Node->nVirtualBlockSize = SizeVirtual - Size;
            RtlpDphPlaceOnVirtualList(DphRoot, Node);

            /* Coalesce them into available list */
            RtlpDphCoalesceNodeIntoAvailable(DphRoot, Node);
        }
    }

    return RtlpDphTakeNodeFromUnusedList(DphRoot);
}

BOOLEAN NTAPI
RtlpDphGrowVirtual(PDPH_HEAP_ROOT DphRoot,
                   SIZE_T Size)
{
    PDPH_HEAP_BLOCK Node, AvailableNode;
    PVOID Base = NULL;
    SIZE_T VirtualSize;
    NTSTATUS Status;

    /* Start with allocating a couple of nodes */
    Node = RtlpDphAllocateNode(DphRoot);
    if (!Node) return FALSE;

    AvailableNode = RtlpDphAllocateNode(DphRoot);
    if (!AvailableNode)
    {
        /* Free the allocated node and return failure */
        RtlpDphReturnNodeToUnusedList(DphRoot, Node);
        return FALSE;
    }

    /* Calculate size of VM to allocate by rounding it up */
    Size = ROUND_UP(Size, 0xFFFF);
    VirtualSize = Size;
    if (Size < DPH_RESERVE_SIZE)
        VirtualSize = DPH_RESERVE_SIZE;

    /* Allocate the virtual memory */
    // FIXME: Shouldn't it be MEM_RESERVE with later committing?
    Status = RtlpDphAllocateVm(&Base, VirtualSize, MEM_COMMIT, PAGE_NOACCESS);
    if (!NT_SUCCESS(Status))
    {
        /* Retry again with a smaller size */
        VirtualSize = Size;
        Status = RtlpDphAllocateVm(&Base, VirtualSize, MEM_COMMIT, PAGE_NOACCESS);
        if (!NT_SUCCESS(Status))
        {
            /* Free the allocated node and return failure */
            RtlpDphReturnNodeToUnusedList(DphRoot, Node);
            RtlpDphReturnNodeToUnusedList(DphRoot, AvailableNode);
            return FALSE;
        }
    }

    /* Set up our two nodes describing this VM */
    Node->pVirtualBlock = Base;
    Node->nVirtualBlockSize = VirtualSize;
    AvailableNode->pVirtualBlock = Base;
    AvailableNode->nVirtualBlockSize = VirtualSize;

    /* Add them to virtual and available lists respectively */
    RtlpDphPlaceOnVirtualList(DphRoot, Node);
    RtlpDphCoalesceNodeIntoAvailable(DphRoot, AvailableNode);

    /* Return success */
    return TRUE;
}

RTL_GENERIC_COMPARE_RESULTS
NTAPI
RtlpDphCompareNodeForTable(IN PRTL_AVL_TABLE Table,
                           IN PVOID FirstStruct,
                           IN PVOID SecondStruct)
{
    ULONG_PTR FirstBlock, SecondBlock;

    FirstBlock = *((ULONG_PTR *)FirstStruct);
    SecondBlock = *((ULONG_PTR *)SecondStruct);

    if (FirstBlock < SecondBlock)
        return GenericLessThan;
    else if (FirstBlock > SecondBlock)
        return GenericGreaterThan;

    return GenericEqual;
}

PVOID
NTAPI
RtlpDphAllocateNodeForTable(IN PRTL_AVL_TABLE Table,
                            IN CLONG ByteSize)
{
    PDPH_HEAP_BLOCK pBlock;
    PDPH_HEAP_ROOT DphRoot;

    /* This mega-assert comes from a text search over Windows 2003 checked binary of ntdll.dll */
    ASSERT((ULONG_PTR)(((PRTL_BALANCED_LINKS)0)+1) + sizeof(PUCHAR) == ByteSize);

    /* Get pointer to the containing heap root record */
    DphRoot = CONTAINING_RECORD(Table, DPH_HEAP_ROOT, BusyNodesTable);
    pBlock = DphRoot->NodeToAllocate;

    DphRoot->NodeToAllocate = NULL;
    ASSERT(pBlock);

    return &(pBlock->TableLinks);
}

VOID
NTAPI
RtlpDphFreeNodeForTable(IN PRTL_AVL_TABLE Table,
                        IN PVOID Buffer)
{
    /* Nothing */
}

NTSTATUS NTAPI
RtlpDphInitializeDelayedFreeQueue(VOID)
{
    NTSTATUS Status;

    Status = RtlInitializeHeapLock(&RtlpDphDelayedFreeQueueLock);
    if (!NT_SUCCESS(Status))
    {
        // TODO: Log this error!
        DPRINT1("Failure initializing delayed free queue critical section\n");
        return Status;
    }

    /* Initialize lists */
    InitializeListHead(&RtlpDphDelayedFreeQueue);
    RtlInitializeSListHead(&RtlpDphDelayedTemporaryPushList);

    /* Reset counters */
    RtlpDphMemoryUsedByDelayedFreeBlocks = 0;
    RtlpDphNumberOfDelayedFreeBlocks = 0;

    return Status;
}

VOID NTAPI
RtlpDphFreeDelayedBlocksFromHeap(PDPH_HEAP_ROOT DphRoot,
                                 PHEAP NormalHeap)
{
    PLIST_ENTRY Current, Next;
    PDPH_BLOCK_INFORMATION BlockInfo;
    ULONG ValidationInfo;

    /* The original routine seems to use a temporary SList to put blocks to be freed,
       then it releases the lock and frees the blocks. But let's make it simple for now */

    /* Acquire the delayed free queue lock */
    RtlEnterHeapLock(RtlpDphDelayedFreeQueueLock, TRUE);

    /* Traverse the list */
    Current = RtlpDphDelayedFreeQueue.Flink;
    while (Current != &RtlpDphDelayedFreeQueue)
    {
        /* Get the next entry pointer */
        Next = Current->Flink;

        BlockInfo = CONTAINING_RECORD(Current, DPH_BLOCK_INFORMATION, FreeQueue);

        /* Check if it belongs to the same heap */
        if (BlockInfo->Heap == DphRoot)
        {
            /* Remove it from the list */
            RemoveEntryList(Current);

            /* Reset its heap to NULL */
            BlockInfo->Heap = NULL;

            if (!RtlpDphIsNormalFreeHeapBlock(BlockInfo + 1, &ValidationInfo, TRUE))
            {
                RtlpDphReportCorruptedBlock(DphRoot, 10, BlockInfo + 1, ValidationInfo);
            }

            /* Decrement counters */
            RtlpDphMemoryUsedByDelayedFreeBlocks -= BlockInfo->ActualSize;
            RtlpDphNumberOfDelayedFreeBlocks--;

            /* Free the normal heap */
            RtlFreeHeap(NormalHeap, 0, BlockInfo);
        }

        /* Move to the next one */
        Current = Next;
    }

    /* Release the delayed free queue lock */
    RtlLeaveHeapLock(RtlpDphDelayedFreeQueueLock);
}

NTSTATUS NTAPI
RtlpDphTargetDllsLogicInitialize(VOID)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

VOID NTAPI
RtlpDphInternalValidatePageHeap(PDPH_HEAP_ROOT DphRoot, PVOID Address, ULONG Value)
{
    UNIMPLEMENTED;
}

VOID NTAPI
RtlpDphVerifyIntegrity(PDPH_HEAP_ROOT DphRoot)
{
    UNIMPLEMENTED;
}

VOID NTAPI
RtlpDphReportCorruptedBlock(PDPH_HEAP_ROOT DphRoot,
                            ULONG Reserved,
                            PVOID Block,
                            ULONG ValidationInfo)
{
    //RtlpDphGetBlockSizeFromCorruptedBlock();

    if (ValidationInfo & DPH_VALINFO_CORRUPTED_AFTER_FREE)
    {
        DPRINT1("block corrupted after having been freed\n");
    }

    if (ValidationInfo & DPH_VALINFO_ALREADY_FREED)
    {
        DPRINT1("block already freed\n");
    }

    if (ValidationInfo & DPH_VALINFO_BAD_INFIX_PATTERN)
    {
        DPRINT1("corrupted infix pattern for freed block\n");
    }

    if (ValidationInfo & DPH_VALINFO_BAD_POINTER)
    {
        DPRINT1("corrupted heap pointer or using wrong heap\n");
    }

    if (ValidationInfo & DPH_VALINFO_BAD_SUFFIX_PATTERN)
    {
        DPRINT1("corrupted suffix pattern\n");
    }

    if (ValidationInfo & DPH_VALINFO_BAD_PREFIX_PATTERN)
    {
        DPRINT1("corrupted prefix pattern\n");
    }

    if (ValidationInfo & DPH_VALINFO_BAD_START_STAMP)
    {
        DPRINT1("corrupted start stamp\n");
    }

    if (ValidationInfo & DPH_VALINFO_BAD_END_STAMP)
    {
        DPRINT1("corrupted end stamp\n");
    }

    if (ValidationInfo & DPH_VALINFO_EXCEPTION)
    {
        DPRINT1("exception raised while verifying block\n");
    }

    DPRINT1("Corrupted heap block %p\n", Block);
}

BOOLEAN NTAPI
RtlpDphIsPageHeapBlock(PDPH_HEAP_ROOT DphRoot,
                       PVOID Block,
                       PULONG ValidationInformation,
                       BOOLEAN CheckFillers)
{
    PDPH_BLOCK_INFORMATION BlockInfo;
    BOOLEAN SomethingWrong = FALSE;
    PUCHAR Byte, Start, End;

    ASSERT(ValidationInformation != NULL);
    *ValidationInformation = 0;

    // _SEH2_TRY {
    BlockInfo = (PDPH_BLOCK_INFORMATION)Block - 1;

    /* Check stamps */
    if (BlockInfo->StartStamp != DPH_FILL_START_STAMP_1)
    {
        *ValidationInformation |= DPH_VALINFO_BAD_START_STAMP;
        SomethingWrong = TRUE;

        /* Check if it has an alloc/free mismatch */
        if (BlockInfo->StartStamp == DPH_FILL_START_STAMP_2)
        {
            /* Notify respectively */
            *ValidationInformation = 0x101;
        }
    }

    if (BlockInfo->EndStamp != DPH_FILL_END_STAMP_1)
    {
        *ValidationInformation |= DPH_VALINFO_BAD_END_STAMP;
        SomethingWrong = TRUE;
    }

    /* Check root heap pointer */
    if (BlockInfo->Heap != DphRoot)
    {
        *ValidationInformation |= DPH_VALINFO_BAD_POINTER;
        SomethingWrong = TRUE;
    }

    /* Check other fillers if requested */
    if (CheckFillers)
    {
        /* Check space after the block */
        Start = (PUCHAR)Block + BlockInfo->RequestedSize;
        End = (PUCHAR)ROUND_UP(Start, PAGE_SIZE);
        for (Byte = Start; Byte < End; Byte++)
        {
            if (*Byte != DPH_FILL_SUFFIX)
            {
                *ValidationInformation |= DPH_VALINFO_BAD_SUFFIX_PATTERN;
                SomethingWrong = TRUE;
                break;
            }
        }
    }

    return (SomethingWrong == FALSE);
}

BOOLEAN NTAPI
RtlpDphIsNormalFreeHeapBlock(PVOID Block,
                             PULONG ValidationInformation,
                             BOOLEAN CheckFillers)
{
    ASSERT(ValidationInformation != NULL);

    UNIMPLEMENTED;
    *ValidationInformation = 0;
    return TRUE;
}

NTSTATUS NTAPI
RtlpDphProcessStartupInitialization(VOID)
{
    NTSTATUS Status;
    PTEB Teb = NtCurrentTeb();

    /* Initialize the DPH heap list and its critical section */
    InitializeListHead(&RtlpDphPageHeapList);
    Status = RtlInitializeHeapLock(&RtlpDphPageHeapListLock);
    if (!NT_SUCCESS(Status))
    {
        ASSERT(FALSE);
        return Status;
    }

    /* Initialize delayed-free queue */
    Status = RtlpDphInitializeDelayedFreeQueue();
    if (!NT_SUCCESS(Status)) return Status;

    /* Initialize the target dlls string */
    RtlInitUnicodeString(&RtlpDphTargetDllsUnicode, RtlpDphTargetDlls);
    Status = RtlpDphTargetDllsLogicInitialize();

    /* Per-process DPH init is done */
    RtlpDphPageHeapListInitialized = TRUE;

    DPRINT1("Page heap: pid 0x%p: page heap enabled with flags 0x%X.\n",
            Teb->ClientId.UniqueProcess, RtlpDphGlobalFlags);

    return Status;
}

BOOLEAN NTAPI
RtlpDphShouldAllocateInPageHeap(PDPH_HEAP_ROOT DphRoot,
                                SIZE_T Size)
{
    //UNIMPLEMENTED;
    /* Always use page heap for now */
    return TRUE;
}

HANDLE NTAPI
RtlpPageHeapCreate(ULONG Flags,
                   PVOID Addr,
                   SIZE_T TotalSize,
                   SIZE_T CommitSize,
                   PVOID Lock,
                   PRTL_HEAP_PARAMETERS Parameters)
{
    PVOID Base = NULL;
    PHEAP HeapPtr;
    PDPH_HEAP_ROOT DphRoot;
    PDPH_HEAP_BLOCK DphNode;
    ULONG MemSize;
    NTSTATUS Status;
    LARGE_INTEGER PerfCounter;

    /* Check for a DPH bypass flag */
    if ((ULONG_PTR)Parameters == -1) return NULL;

    /* Make sure no user-allocated stuff was provided */
    if (Addr || Lock) return NULL;

    /* Allocate minimum amount of virtual memory */
    MemSize = DPH_RESERVE_SIZE;
    Status = RtlpDphAllocateVm(&Base, MemSize, MEM_COMMIT, PAGE_NOACCESS);
    if (!NT_SUCCESS(Status))
    {
        ASSERT(FALSE);
        return NULL;
    }

    /* Set protection */
    Status = RtlpDphProtectVm(Base, 2*PAGE_SIZE + DPH_POOL_SIZE, PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        //RtlpDphFreeVm(Base, 0, 0, 0);
        ASSERT(FALSE);
        return NULL;
    }

    /* Start preparing the 1st page. Fill it with the default filler */
    RtlFillMemoryUlong(Base, PAGE_SIZE, DPH_FILL);

    /* Set flags in the "HEAP" structure */
    HeapPtr = (PHEAP)Base;
    HeapPtr->Flags = Flags | HEAP_FLAG_PAGE_ALLOCS;
    HeapPtr->ForceFlags = Flags | HEAP_FLAG_PAGE_ALLOCS;

    /* Set 1st page to read only now */
    Status = RtlpDphProtectVm(Base, PAGE_SIZE, PAGE_READONLY);
    if (!NT_SUCCESS(Status))
    {
        ASSERT(FALSE);
        return NULL;
    }

    /* 2nd page is the real DPH root block */
    DphRoot = (PDPH_HEAP_ROOT)((PCHAR)Base + PAGE_SIZE);

    /* Initialize the DPH root */
    DphRoot->Signature = DPH_SIGNATURE;
    DphRoot->HeapFlags = Flags;
    DphRoot->HeapCritSect = (PHEAP_LOCK)((PCHAR)DphRoot + DPH_POOL_SIZE);
    DphRoot->ExtraFlags = RtlpDphGlobalFlags;

    ZwQueryPerformanceCounter(&PerfCounter, NULL);
    DphRoot->Seed = PerfCounter.LowPart;

    RtlInitializeHeapLock(&DphRoot->HeapCritSect);
    InitializeListHead(&DphRoot->AvailableAllocationHead);

    /* Create a normal heap for this paged heap */
    DphRoot->NormalHeap = RtlCreateHeap(Flags, NULL, TotalSize, CommitSize, NULL, (PRTL_HEAP_PARAMETERS)-1);
    if (!DphRoot->NormalHeap)
    {
        ASSERT(FALSE);
        return NULL;
    }

    /* 3rd page: a pool for DPH allocations */
    RtlpDphAddNewPool(DphRoot, NULL, DphRoot + 1, DPH_POOL_SIZE - sizeof(DPH_HEAP_ROOT), FALSE);

    /* Allocate internal heap blocks. For the root */
    DphNode = RtlpDphAllocateNode(DphRoot);
    ASSERT(DphNode != NULL);
    DphNode->pVirtualBlock = (PUCHAR)DphRoot;
    DphNode->nVirtualBlockSize = DPH_POOL_SIZE;
    RtlpDphPlaceOnPoolList(DphRoot, DphNode);

    /* For the memory we allocated as a whole */
    DphNode = RtlpDphAllocateNode(DphRoot);
    ASSERT(DphNode != NULL);
    DphNode->pVirtualBlock = Base;
    DphNode->nVirtualBlockSize = MemSize;
    RtlpDphPlaceOnVirtualList(DphRoot, DphNode);

    /* For the remaining part */
    DphNode = RtlpDphAllocateNode(DphRoot);
    ASSERT(DphNode != NULL);
    DphNode->pVirtualBlock = (PUCHAR)Base + 2*PAGE_SIZE + DPH_POOL_SIZE;
    DphNode->nVirtualBlockSize = MemSize - (2*PAGE_SIZE + DPH_POOL_SIZE);
    RtlpDphCoalesceNodeIntoAvailable(DphRoot, DphNode);

    //DphRoot->CreateStackTrace = RtlpDphLogStackTrace(1);

    /* Initialize AVL-based busy nodes table */
    RtlInitializeGenericTableAvl(&DphRoot->BusyNodesTable,
                                 RtlpDphCompareNodeForTable,
                                 RtlpDphAllocateNodeForTable,
                                 RtlpDphFreeNodeForTable,
                                 NULL);

    /* Initialize per-process startup info */
    if (!RtlpDphPageHeapListInitialized) RtlpDphProcessStartupInitialization();

    /* Acquire the heap list lock */
    RtlEnterHeapLock(RtlpDphPageHeapListLock, TRUE);

    /* Insert this heap to the tail of the global list */
    InsertTailList(&RtlpDphPageHeapList, &DphRoot->NextHeap);

    /* Note we increased the size of the list */
    RtlpDphPageHeapListLength++;

    /* Release the heap list lock */
    RtlLeaveHeapLock(RtlpDphPageHeapListLock);

    if (RtlpDphDebugOptions & DPH_DEBUG_VERBOSE)
    {
        DPRINT1("Page heap: process 0x%p created heap @ %p (%p, flags 0x%X)\n",
                NtCurrentTeb()->ClientId.UniqueProcess, (PUCHAR)DphRoot - PAGE_SIZE,
                DphRoot->NormalHeap, DphRoot->ExtraFlags);
    }

    /* Perform internal validation if required */
    if (RtlpDphDebugOptions & DPH_DEBUG_INTERNAL_VALIDATE)
        RtlpDphInternalValidatePageHeap(DphRoot, NULL, 0);

    return (PUCHAR)DphRoot - PAGE_SIZE;
}

PVOID NTAPI
RtlpPageHeapDestroy(HANDLE HeapPtr)
{
    PDPH_HEAP_ROOT DphRoot;
    PVOID Ptr;
    PDPH_HEAP_BLOCK Node, Next;
    PHEAP NormalHeap;
    ULONG Value;

    /* Check if it's not a process heap */
    if (HeapPtr == RtlGetProcessHeap())
    {
        DbgBreakPoint();
        return NULL;
    }

    /* Get pointer to the heap root */
    DphRoot = RtlpDphPointerFromHandle(HeapPtr);
    if (!DphRoot) return NULL;

    RtlpDphPreProcessing(DphRoot, DphRoot->HeapFlags);

    /* Get the pointer to the normal heap */
    NormalHeap = DphRoot->NormalHeap;

    /* Free the delayed-free blocks */
    RtlpDphFreeDelayedBlocksFromHeap(DphRoot, NormalHeap);

    /* Go through the busy blocks */
    Ptr = RtlEnumerateGenericTableAvl(&DphRoot->BusyNodesTable, TRUE);

    while (Ptr)
    {
        Node = CONTAINING_RECORD(Ptr, DPH_HEAP_BLOCK, pUserAllocation);
        if (!(DphRoot->ExtraFlags & DPH_EXTRA_CHECK_UNDERRUN))
        {
            if (!RtlpDphIsPageHeapBlock(DphRoot, Node->pUserAllocation, &Value, TRUE))
            {
                RtlpDphReportCorruptedBlock(DphRoot, 3, Node->pUserAllocation, Value);
            }
        }

        /* FIXME: Call AV notification */
        //AVrfInternalHeapFreeNotification();

        /* Go to the next node */
        Ptr = RtlEnumerateGenericTableAvl(&DphRoot->BusyNodesTable, FALSE);
    }

    /* Acquire the global heap list lock */
    RtlEnterHeapLock(RtlpDphPageHeapListLock, TRUE);

    /* Remove the entry and decrement the global counter */
    RemoveEntryList(&DphRoot->NextHeap);
    RtlpDphPageHeapListLength--;

    /* Release the global heap list lock */
    RtlLeaveHeapLock(RtlpDphPageHeapListLock);

    /* Leave and delete this heap's critical section */
    RtlLeaveHeapLock(DphRoot->HeapCritSect);
    RtlDeleteHeapLock(DphRoot->HeapCritSect);

    /* Now go through all virtual list nodes and release the VM */
    Node = DphRoot->pVirtualStorageListHead;
    while (Node)
    {
        Next = Node->pNextAlloc;
        /* Release the memory without checking result */
        RtlpDphFreeVm(Node->pVirtualBlock, 0, MEM_RELEASE);
        Node = Next;
    }

    /* Destroy the normal heap */
    RtlDestroyHeap(NormalHeap);

    /* Report success */
    if (RtlpDphDebugOptions & DPH_DEBUG_VERBOSE)
        DPRINT1("Page heap: process 0x%p destroyed heap @ %p (%p)\n",
                NtCurrentTeb()->ClientId.UniqueProcess, HeapPtr, NormalHeap);

    return NULL;
}

PVOID NTAPI
RtlpPageHeapAllocate(IN PVOID HeapPtr,
                     IN ULONG Flags,
                     IN SIZE_T Size)
{
    PDPH_HEAP_ROOT DphRoot;
    PDPH_HEAP_BLOCK AvailableNode, BusyNode;
    BOOLEAN Biased = FALSE;
    ULONG AllocateSize, AccessSize;
    NTSTATUS Status;
    SIZE_T UserActualSize;
    PVOID Ptr;

    /* Check requested size */
    if (Size > 0x7FF00000)
    {
        DPRINT1("extreme size request\n");

        /* Generate an exception if needed */
        if (Flags & HEAP_GENERATE_EXCEPTIONS) RtlpDphRaiseException(STATUS_NO_MEMORY);

        return NULL;
    }

    /* Unbias the pointer if necessary */
    if (IS_BIASED_POINTER(HeapPtr))
    {
        HeapPtr = (PVOID)POINTER_REMOVE_BIAS(HeapPtr);
        Biased = TRUE;
    }

    /* Get a pointer to the heap root */
    DphRoot = RtlpDphPointerFromHandle(HeapPtr);
    if (!DphRoot) return NULL;

    /* Acquire the heap lock */
    RtlpDphPreProcessing(DphRoot, Flags);

    /* Perform internal validation if specified by flags */
    if (RtlpDphDebugOptions & DPH_DEBUG_INTERNAL_VALIDATE && !Biased)
    {
        RtlpDphInternalValidatePageHeap(DphRoot, NULL, 0);
    }

    /* Add heap flags */
    Flags |= DphRoot->HeapFlags;

    if (!Biased && !RtlpDphShouldAllocateInPageHeap(DphRoot, Size))
    {
        /* Perform allocation from a normal heap */
        ASSERT(FALSE);
    }

    /* Perform heap integrity check if specified by flags */
    if (RtlpDphDebugOptions & DPH_DEBUG_INTERNAL_VALIDATE)
    {
        RtlpDphVerifyIntegrity(DphRoot);
    }

    /* Calculate sizes */
    AccessSize = ROUND_UP(Size + sizeof(DPH_BLOCK_INFORMATION), PAGE_SIZE);
    AllocateSize = AccessSize + PAGE_SIZE;

    // FIXME: Move RtlpDphAllocateNode(DphRoot) to this place
    AvailableNode = RtlpDphFindAvailableMemory(DphRoot, AllocateSize, TRUE);
    if (!AvailableNode)
    {
        DPRINT1("Page heap: Unable to allocate virtual memory\n");
        DbgBreakPoint();

        /* Release the lock */
        RtlpDphPostProcessing(DphRoot);

        return NULL;
    }
    ASSERT(AvailableNode->nVirtualBlockSize >= AllocateSize);

    /* Set protection */
    Status = RtlpDphSetProtectionBeforeUse(DphRoot,
                                           AvailableNode->pVirtualBlock,
                                           AccessSize);
    if (!NT_SUCCESS(Status))
    {
        ASSERT(FALSE);
    }

    /* Save available node pointer */
    Ptr = AvailableNode->pVirtualBlock;

    /* Check node's size */
    if (AvailableNode->nVirtualBlockSize > AllocateSize)
    {
        /* The block contains too much free space, reduce it */
        AvailableNode->pVirtualBlock += AllocateSize;
        AvailableNode->nVirtualBlockSize -= AllocateSize;
        DphRoot->nAvailableAllocationBytesCommitted -= AllocateSize;

        /* Allocate a new node which will be our busy node */
        BusyNode = RtlpDphAllocateNode(DphRoot);
        ASSERT(BusyNode != NULL);
        BusyNode->pVirtualBlock = Ptr;
        BusyNode->nVirtualBlockSize = AllocateSize;
    }
    else
    {
        /* The block's size fits exactly */
        RtlpDphRemoveFromAvailableList(DphRoot, AvailableNode);
        BusyNode = AvailableNode;
    }

    /* Calculate actual user size  */
    if (DphRoot->HeapFlags & HEAP_NO_ALIGNMENT)
        UserActualSize = Size;
    else
        UserActualSize = ROUND_UP(Size, 8);

    /* Set up the block */
    BusyNode->nVirtualAccessSize = AccessSize;
    BusyNode->nUserActualSize = UserActualSize;
    BusyNode->nUserRequestedSize = Size;

    if (DphRoot->ExtraFlags & DPH_EXTRA_CHECK_UNDERRUN)
        BusyNode->pUserAllocation = BusyNode->pVirtualBlock + PAGE_SIZE;
    else
        BusyNode->pUserAllocation = BusyNode->pVirtualBlock + BusyNode->nVirtualAccessSize - UserActualSize;

    BusyNode->UserValue = NULL;
    BusyNode->UserFlags = Flags & HEAP_SETTABLE_USER_FLAGS;

    // FIXME: Don't forget about stack traces if such flag was set
    BusyNode->StackTrace = NULL;

    /* Place it on busy list */
    RtlpDphPlaceOnBusyList(DphRoot, BusyNode);

    /* Zero or patter-fill memory depending on flags */
    if (Flags & HEAP_ZERO_MEMORY)
        RtlZeroMemory(BusyNode->pUserAllocation, Size);
    else
        RtlFillMemory(BusyNode->pUserAllocation, Size, DPH_FILL_INFIX);

    /* Write DPH info */
    if (!(DphRoot->ExtraFlags & DPH_EXTRA_CHECK_UNDERRUN))
    {
        RtlpDphWritePageHeapBlockInformation(DphRoot,
                                             BusyNode->pUserAllocation,
                                             Size,
                                             AccessSize);
    }

    /* Finally allocation is done, perform validation again if required */
    if (RtlpDphDebugOptions & DPH_DEBUG_INTERNAL_VALIDATE && !Biased)
    {
        RtlpDphInternalValidatePageHeap(DphRoot, NULL, 0);
    }

    /* Release the lock */
    RtlpDphPostProcessing(DphRoot);

    DPRINT("Allocated user block pointer: %p\n", BusyNode->pUserAllocation);

    /* Return pointer to user allocation */
    return BusyNode->pUserAllocation;
}

BOOLEAN NTAPI
RtlpPageHeapFree(HANDLE HeapPtr,
                 ULONG Flags,
                 PVOID Ptr)
{
    PDPH_HEAP_ROOT DphRoot;
    PDPH_HEAP_BLOCK Node;
    ULONG ValidationInfo;
    PDPH_BLOCK_INFORMATION Info;

    /* Check for a NULL pointer freeing */
    if (!Ptr)
    {
        if (RtlpDphBreakOptions & DPH_BREAK_ON_NULL_FREE)
        {
            DPRINT1("Page heap: freeing a null pointer\n");
            DbgBreakPoint();
        }
        return TRUE;
    }

    /* Get a pointer to the heap root */
    DphRoot = RtlpDphPointerFromHandle(HeapPtr);
    if (!DphRoot) return FALSE;

    /* Acquire the heap lock */
    RtlpDphPreProcessing(DphRoot, Flags);

    /* Perform internal validation if specified by flags */
    if (RtlpDphDebugOptions & DPH_DEBUG_INTERNAL_VALIDATE)
        RtlpDphInternalValidatePageHeap(DphRoot, NULL, 0);

    /* Add heap flags */
    Flags |= DphRoot->HeapFlags;

    /* Find busy memory */
    Node = RtlpDphFindBusyMemory(DphRoot, Ptr);

    if (!Node)
    {
        /* This block was not found in page heap, try a normal heap instead */
        //RtlpDphNormalHeapFree();
        ASSERT(FALSE);
    }

    if (!(DphRoot->ExtraFlags & DPH_EXTRA_CHECK_UNDERRUN))
    {
        /* Check and report corrupted block */
        if (!RtlpDphIsPageHeapBlock(DphRoot, Ptr, &ValidationInfo, TRUE))
        {
            RtlpDphReportCorruptedBlock(DphRoot, 1, Ptr, ValidationInfo);
        }

        // FIXME: Should go inside RtlpDphSetProtectionAfterUse
        if (Node->nVirtualAccessSize != 0)
        {
            /* Set stamps */
            Info = (PDPH_BLOCK_INFORMATION)Node->pUserAllocation - 1;
            Info->StartStamp = DPH_FILL_START_STAMP_2;
            Info->EndStamp = DPH_FILL_END_STAMP_2;

            RtlpDphProtectVm(Node->pVirtualBlock, Node->nVirtualAccessSize, PAGE_NOACCESS);
        }
    }
    else
    {
        // FIXME: Should go inside RtlpDphSetProtectionAfterUse
        if (Node->nVirtualAccessSize != 0)
            RtlpDphProtectVm(Node->pVirtualBlock + PAGE_SIZE, Node->nVirtualAccessSize, PAGE_NOACCESS);
    }

    /* Set new protection */
    //RtlpDphSetProtectionAfterUse(DphRoot, Node);

    /* Remove it from the list of busy nodes */
    RtlpDphRemoveFromBusyList(DphRoot, Node);

    /* And put it into the list of free nodes */
    RtlpDphPlaceOnFreeList(DphRoot, Node);

    //if (DphRoot->ExtraFlags & DPH_EXTRA_LOG_STACK_TRACES)
    //    Node->StackTrace = RtlpDphLogStackTrace(3);
    //else
        Node->StackTrace = NULL;

    /* Leave the heap lock */
    RtlpDphPostProcessing(DphRoot);

    /* Return success */
    return TRUE;
}

PVOID NTAPI
RtlpPageHeapReAllocate(HANDLE HeapPtr,
                       ULONG Flags,
                       PVOID Ptr,
                       SIZE_T Size)
{
    PDPH_HEAP_ROOT DphRoot;
    PDPH_HEAP_BLOCK Node = NULL, AllocatedNode;
    BOOLEAN Biased = FALSE, UseNormalHeap = FALSE, OldBlockPageHeap = TRUE;
    ULONG ValidationInfo;
    SIZE_T DataSize;
    PVOID NewAlloc = NULL;

    /* Check requested size */
    if (Size > 0x7FF00000)
    {
        DPRINT1("extreme size request\n");

        /* Generate an exception if needed */
        if (Flags & HEAP_GENERATE_EXCEPTIONS) RtlpDphRaiseException(STATUS_NO_MEMORY);

        return NULL;
    }

    /* Unbias the pointer if necessary */
    if (IS_BIASED_POINTER(HeapPtr))
    {
        HeapPtr = (PVOID)POINTER_REMOVE_BIAS(HeapPtr);
        Biased = TRUE;
    }

    /* Get a pointer to the heap root */
    DphRoot = RtlpDphPointerFromHandle(HeapPtr);
    if (!DphRoot) return NULL;

    /* Acquire the heap lock */
    RtlpDphPreProcessing(DphRoot, Flags);

    /* Perform internal validation if specified by flags */
    if (RtlpDphDebugOptions & DPH_DEBUG_INTERNAL_VALIDATE)
    {
        RtlpDphInternalValidatePageHeap(DphRoot, NULL, 0);
    }

    /* Add heap flags */
    Flags |= DphRoot->HeapFlags;

    /* Exit with NULL right away if inplace is specified */
    if (Flags & HEAP_REALLOC_IN_PLACE_ONLY)
    {
        /* Release the lock */
        RtlpDphPostProcessing(DphRoot);

        /* Generate an exception if needed */
        if (Flags & HEAP_GENERATE_EXCEPTIONS) RtlpDphRaiseException(STATUS_NO_MEMORY);

        return NULL;
    }

    /* Try to get node of the allocated block */
    AllocatedNode = RtlpDphFindBusyMemory(DphRoot, Ptr);

    if (!AllocatedNode)
    {
        /* This block was not found in page heap, try a normal heap instead */
        //RtlpDphNormalHeapFree();
        ASSERT(FALSE);
        OldBlockPageHeap = FALSE;
    }

    /* Check the block */
    if (!(DphRoot->ExtraFlags & DPH_EXTRA_CHECK_UNDERRUN))
    {
        if (!RtlpDphIsPageHeapBlock(DphRoot, AllocatedNode->pUserAllocation, &ValidationInfo, TRUE))
        {
            RtlpDphReportCorruptedBlock(DphRoot, 3, AllocatedNode->pUserAllocation, ValidationInfo);
        }
    }

    /* Remove old one from the busy list */
    RtlpDphRemoveFromBusyList(DphRoot, AllocatedNode);

    if (!Biased && !RtlpDphShouldAllocateInPageHeap(DphRoot, Size))
    {
        // FIXME: Use normal heap
        ASSERT(FALSE);
        UseNormalHeap = TRUE;
    }
    else
    {
        /* Now do a trick: bias the pointer and call our allocate routine */
        NewAlloc = RtlpPageHeapAllocate((PVOID)POINTER_ADD_BIAS(HeapPtr), Flags, Size);
    }

    if (!NewAlloc)
    {
        /* New allocation failed, put the block back (if it was found in page heap) */
        RtlpDphPlaceOnBusyList(DphRoot, AllocatedNode);

        /* Release the lock */
        RtlpDphPostProcessing(DphRoot);

        /* Perform validation again if required */
        if (RtlpDphDebugOptions & DPH_DEBUG_INTERNAL_VALIDATE)
        {
            RtlpDphInternalValidatePageHeap(DphRoot, NULL, 0);
        }

        /* Generate an exception if needed */
        if (Flags & HEAP_GENERATE_EXCEPTIONS) RtlpDphRaiseException(STATUS_NO_MEMORY);

        return NULL;
    }

    /* Copy contents of the old block */
    if (AllocatedNode->nUserRequestedSize > Size)
        DataSize = Size;
    else
        DataSize = AllocatedNode->nUserRequestedSize;

    if (DataSize != 0) RtlCopyMemory(NewAlloc, Ptr, DataSize);

    /* Copy user flags and values */
    if (!UseNormalHeap)
    {
        /* Get the node of the new block */
        Node = RtlpDphFindBusyMemory(DphRoot, NewAlloc);
        ASSERT(Node != NULL);

        /* Set its values/flags */
        Node->UserValue = AllocatedNode->UserValue;
        if (Flags & HEAP_SETTABLE_USER_FLAGS)
            Node->UserFlags = Flags & HEAP_SETTABLE_USER_FLAGS;
        else
            Node->UserFlags = AllocatedNode->UserFlags;
    }

    if (!OldBlockPageHeap)
    {
        /* Weird scenario, investigate */
        ASSERT(FALSE);
    }

    /* Mark the old block as no access */
    if (AllocatedNode->nVirtualAccessSize != 0)
    {
        RtlpDphProtectVm(AllocatedNode->pVirtualBlock, AllocatedNode->nVirtualAccessSize, PAGE_NOACCESS);
    }

    /* And place it on the free list */
    RtlpDphPlaceOnFreeList(DphRoot, AllocatedNode);

    // FIXME: Capture stack traces if needed
    AllocatedNode->StackTrace = NULL;

    /* Finally allocation is done, perform validation again if required */
    if (RtlpDphDebugOptions & DPH_DEBUG_INTERNAL_VALIDATE && !Biased)
    {
        RtlpDphInternalValidatePageHeap(DphRoot, NULL, 0);
    }

    /* Release the lock */
    RtlpDphPostProcessing(DphRoot);

    DPRINT("Allocated new user block pointer: %p\n", NewAlloc);

    /* Return pointer to user allocation */
    return NewAlloc;
}

BOOLEAN NTAPI
RtlpPageHeapGetUserInfo(PVOID HeapHandle,
                        ULONG Flags,
                        PVOID BaseAddress,
                        PVOID *UserValue,
                        PULONG UserFlags)
{
    PDPH_HEAP_ROOT DphRoot;
    PDPH_HEAP_BLOCK Node;

    /* Get a pointer to the heap root */
    DphRoot = RtlpDphPointerFromHandle(HeapHandle);
    if (!DphRoot) return FALSE;

    /* Add heap flags */
    Flags |= DphRoot->HeapFlags;

    /* Acquire the heap lock */
    RtlpDphPreProcessing(DphRoot, Flags);

    /* Find busy memory */
    Node = RtlpDphFindBusyMemory(DphRoot, BaseAddress);

    if (!Node)
    {
        /* This block was not found in page heap, try a normal heap instead */
        //RtlpDphNormalHeapGetUserInfo();
        ASSERT(FALSE);
        return FALSE;
    }

    /* Get user values and flags and store them in user provided pointers */
    if (UserValue) *UserValue = Node->UserValue;
    if (UserFlags) *UserFlags = Node->UserFlags;

    /* Leave the heap lock */
    RtlpDphPostProcessing(DphRoot);

    /* Return success */
    return TRUE;
}

BOOLEAN NTAPI
RtlpPageHeapSetUserValue(PVOID HeapHandle,
                         ULONG Flags,
                         PVOID BaseAddress,
                         PVOID UserValue)
{
    PDPH_HEAP_ROOT DphRoot;
    PDPH_HEAP_BLOCK Node;

    /* Get a pointer to the heap root */
    DphRoot = RtlpDphPointerFromHandle(HeapHandle);
    if (!DphRoot) return FALSE;

    /* Add heap flags */
    Flags |= DphRoot->HeapFlags;

    /* Acquire the heap lock */
    RtlpDphPreProcessing(DphRoot, Flags);

    /* Find busy memory */
    Node = RtlpDphFindBusyMemory(DphRoot, BaseAddress);

    if (!Node)
    {
        /* This block was not found in page heap, try a normal heap instead */
        //RtlpDphNormalHeapSetUserValue();
        ASSERT(FALSE);
        return FALSE;
    }

    /* Get user values and flags and store them in user provided pointers */
    Node->UserValue = UserValue;

    /* Leave the heap lock */
    RtlpDphPostProcessing(DphRoot);

    /* Return success */
    return TRUE;
}

BOOLEAN
NTAPI
RtlpPageHeapSetUserFlags(PVOID HeapHandle,
                         ULONG Flags,
                         PVOID BaseAddress,
                         ULONG UserFlagsReset,
                         ULONG UserFlagsSet)
{
    PDPH_HEAP_ROOT DphRoot;
    PDPH_HEAP_BLOCK Node;

    /* Get a pointer to the heap root */
    DphRoot = RtlpDphPointerFromHandle(HeapHandle);
    if (!DphRoot) return FALSE;

    /* Add heap flags */
    Flags |= DphRoot->HeapFlags;

    /* Acquire the heap lock */
    RtlpDphPreProcessing(DphRoot, Flags);

    /* Find busy memory */
    Node = RtlpDphFindBusyMemory(DphRoot, BaseAddress);

    if (!Node)
    {
        /* This block was not found in page heap, try a normal heap instead */
        //RtlpDphNormalHeapSetUserFlags();
        ASSERT(FALSE);
        return FALSE;
    }

    /* Get user values and flags and store them in user provided pointers */
    Node->UserFlags &= ~(UserFlagsReset);
    Node->UserFlags |= UserFlagsSet;

    /* Leave the heap lock */
    RtlpDphPostProcessing(DphRoot);

    /* Return success */
    return TRUE;
}

SIZE_T NTAPI
RtlpPageHeapSize(HANDLE HeapHandle,
                 ULONG Flags,
                 PVOID BaseAddress)
{
    PDPH_HEAP_ROOT DphRoot;
    PDPH_HEAP_BLOCK Node;
    SIZE_T Size;

    /* Get a pointer to the heap root */
    DphRoot = RtlpDphPointerFromHandle(HeapHandle);
    if (!DphRoot) return -1;

    /* Add heap flags */
    Flags |= DphRoot->HeapFlags;

    /* Acquire the heap lock */
    RtlpDphPreProcessing(DphRoot, Flags);

    /* Find busy memory */
    Node = RtlpDphFindBusyMemory(DphRoot, BaseAddress);

    if (!Node)
    {
        /* This block was not found in page heap, try a normal heap instead */
        //RtlpDphNormalHeapSize();
        ASSERT(FALSE);
        return -1;
    }

    /* Get heap block size */
    Size = Node->nUserRequestedSize;

    /* Leave the heap lock */
    RtlpDphPostProcessing(DphRoot);

    /* Return user requested size */
    return Size;
}

BOOLEAN
NTAPI
RtlpDebugPageHeapValidate(PVOID HeapHandle,
                          ULONG Flags,
                          PVOID BaseAddress)
{
    PDPH_HEAP_ROOT DphRoot;
    PDPH_HEAP_BLOCK Node = NULL;
    BOOLEAN Valid = FALSE;

    /* Get a pointer to the heap root */
    DphRoot = RtlpDphPointerFromHandle(HeapHandle);
    if (!DphRoot) return -1;

    /* Add heap flags */
    Flags |= DphRoot->HeapFlags;

    /* Acquire the heap lock */
    RtlpDphPreProcessing(DphRoot, Flags);

    /* Find busy memory */
    if (BaseAddress)
        Node = RtlpDphFindBusyMemory(DphRoot, BaseAddress);

    if (!Node)
    {
        /* This block was not found in page heap, or the request is to validate all normal heap */
        Valid = RtlpDphNormalHeapValidate(DphRoot, Flags, BaseAddress);
    }

    /* Leave the heap lock */
    RtlpDphPostProcessing(DphRoot);

    /* Return result of a normal heap validation */
    if (BaseAddress && !Node)
        return Valid;

    /* Otherwise return our own result */
    if (!BaseAddress || Node) Valid = TRUE;

    return Valid;
}

BOOLEAN
NTAPI
RtlpDphNormalHeapValidate(PDPH_HEAP_ROOT DphRoot,
                          ULONG Flags,
                          PVOID BaseAddress)
{
    PDPH_BLOCK_INFORMATION BlockInfo = (PDPH_BLOCK_INFORMATION)BaseAddress - 1;
    if (!BaseAddress)
    {
        /* Validate all normal heap */
        return RtlValidateHeap(DphRoot->NormalHeap, Flags, NULL);
    }

    // FIXME: Check is this a normal heap block
    /*if (!RtlpDphIsNormalHeapBlock(DphRoot, BaseAddress, &ValidationInfo))
    {
    }*/

    return RtlValidateHeap(DphRoot->NormalHeap, Flags, BlockInfo);
}

BOOLEAN
NTAPI
RtlpPageHeapLock(HANDLE HeapPtr)
{
    PDPH_HEAP_ROOT DphRoot;

    /* Get pointer to the heap root */
    DphRoot = RtlpDphPointerFromHandle(HeapPtr);
    if (!DphRoot) return FALSE;

    RtlpDphEnterCriticalSection(DphRoot, DphRoot->HeapFlags);
    return TRUE;
}

BOOLEAN
NTAPI
RtlpPageHeapUnlock(HANDLE HeapPtr)
{
    PDPH_HEAP_ROOT DphRoot;

    /* Get pointer to the heap root */
    DphRoot = RtlpDphPointerFromHandle(HeapPtr);
    if (!DphRoot) return FALSE;

    RtlpDphLeaveCriticalSection(DphRoot);
    return TRUE;
}

/* EOF */
