/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/heappage.c
 * PURPOSE:         RTL Page Heap implementation
 * PROGRAMMERS:     Copyright 2011 Aleksey Bragin
 */

/* Useful references:
    http://msdn.microsoft.com/en-us/library/ms220938(VS.80).aspx
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
     ULONG RequestedSize;
     ULONG ActualSize;
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
     ULONG nVirtualBlockSize;
     ULONG nVirtualAccessSize;
     ULONG nUserRequestedSize;
     ULONG nUserActualSize;
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
     PRTL_CRITICAL_SECTION HeapCritSect;
     ULONG nRemoteLockAcquired;

     PDPH_HEAP_BLOCK pVirtualStorageListHead;
     PDPH_HEAP_BLOCK pVirtualStorageListTail;
     ULONG nVirtualStorageRanges;
     ULONG nVirtualStorageBytes;

     RTL_AVL_TABLE BusyNodesTable;
     PDPH_HEAP_BLOCK NodeToAllocate;
     ULONG nBusyAllocations;
     ULONG nBusyAllocationBytesCommitted;

     PDPH_HEAP_BLOCK pFreeAllocationListHead;
     PDPH_HEAP_BLOCK pFreeAllocationListTail;
     ULONG nFreeAllocations;
     ULONG nFreeAllocationBytesCommitted;

     LIST_ENTRY AvailableAllocationHead;
     ULONG nAvailableAllocations;
     ULONG nAvailableAllocationBytesCommitted;

     PDPH_HEAP_BLOCK pUnusedNodeListHead;
     PDPH_HEAP_BLOCK pUnusedNodeListTail;
     ULONG nUnusedNodes;
     ULONG nBusyAllocationBytesAccessible;
     PDPH_HEAP_BLOCK pNodePoolListHead;
     PDPH_HEAP_BLOCK pNodePoolListTail;
     ULONG nNodePools;
     ULONG nNodePoolBytes;

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

ULONG RtlpDphBreakOptions;
ULONG RtlpDphDebugOptions;

LIST_ENTRY RtlpDphPageHeapList;
BOOLEAN RtlpDphPageHeapListInitialized;
RTL_CRITICAL_SECTION RtlpDphPageHeapListLock;
ULONG RtlpDphPageHeapListLength;
UNICODE_STRING RtlpDphTargetDllsUnicode;

/* Counters */
LONG RtlpDphCounter;
LONG RtlpDphAllocFails;
LONG RtlpDphReleaseFails;
LONG RtlpDphFreeFails;
LONG RtlpDphProtectFails;

#define DPH_RESERVE_SIZE 0x100000
#define DPH_POOL_SIZE 0x4000

/* RtlpDphBreakOptions */
#define DPH_BREAK_ON_RESERVE_FAIL 0x01
#define DPH_BREAK_ON_COMMIT_FAIL  0x02
#define DPH_BREAK_ON_RELEASE_FAIL 0x04
#define DPH_BREAK_ON_FREE_FAIL    0x08
#define DPH_BREAK_ON_PROTECT_FAIL 0x10

/* RtlpDphDebugOptions */
#define DPH_DEBUG_INTERNAL_VALIDATE 0x01
#define DPH_DEBUG_VERBOSE           0x04

/* Fillers */
#define DPH_FILL 0xEEEEEEEE

/* Signatures */
#define DPH_SIGNATURE 0xFFEEDDCC

/* Biased pointer macros */
#define IS_BIASED_POINTER(ptr) ((ULONG_PTR)(ptr) & 1)
#define POINTER_REMOVE_BIAS(ptr) ((ULONG_PTR)(ptr) & ~(ULONG_PTR)1)
#define POINTER_ADD_BIAS(ptr) ((ULONG_PTR)(ptr) & 1)

/* FUNCTIONS ******************************************************************/

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

    /* Check for failures */
    if (!NT_SUCCESS(Status))
    {
        if (Type == MEM_RESERVE)
        {
            _InterlockedIncrement(&RtlpDphCounter);
            if (RtlpDphBreakOptions & DPH_BREAK_ON_RESERVE_FAIL)
            {
                DPRINT1("Page heap: AllocVm (%p, %p, %x) failed with %x \n", Base, Size, Type, Status);
                DbgBreakPoint();
                return Status;
            }
        }
        else
        {
            _InterlockedIncrement(&RtlpDphAllocFails);
            if (RtlpDphBreakOptions & DPH_BREAK_ON_COMMIT_FAIL)
            {
                DPRINT1("Page heap: AllocVm (%p, %p, %x) failed with %x \n", Base, Size, Type, Status);
                DbgBreakPoint();
                return Status;
            }
        }
    }

    return Status;
}

NTSTATUS NTAPI
RtlpDphFreeVm(PVOID Base, SIZE_T Size, ULONG Type, ULONG Protection)
{
    NTSTATUS Status;

    /* Free the memory */
    Status = RtlpSecMemFreeVirtualMemory(NtCurrentProcess(), &Base, &Size, Type);

    /* Log/report failures */
    if (!NT_SUCCESS(Status))
    {
        if (Type == MEM_RELEASE)
        {
            _InterlockedIncrement(&RtlpDphReleaseFails);
            if (RtlpDphBreakOptions & DPH_BREAK_ON_RELEASE_FAIL)
            {
                DPRINT1("Page heap: FreeVm (%p, %p, %x) failed with %x \n", Base, Size, Type, Status);
                DbgBreakPoint();
                return Status;
            }
        }
        else
        {
            _InterlockedIncrement(&RtlpDphFreeFails);
            if (RtlpDphBreakOptions & DPH_BREAK_ON_FREE_FAIL)
            {
                DPRINT1("Page heap: FreeVm (%p, %p, %x) failed with %x \n", Base, Size, Type, Status);
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
            DPRINT1("Page heap: ProtectVm (%p, %p, %x) failed with %x \n", Base, Size, Protection, Status);
            DbgBreakPoint();
            return Status;
        }
    }

    return Status;
}

VOID NTAPI
RtlpDphPlaceOnPoolList(PDPH_HEAP_ROOT DphRoot, PDPH_HEAP_BLOCK DphNode)
{
    /* DphNode is being added to the tail of the list */
    DphNode->pNextAlloc = NULL;

    /* Add it to the tail of the linked list */
    if (DphRoot->pNodePoolListTail)
        DphRoot->pNodePoolListTail->pNextAlloc = DphNode;
    else
        DphRoot->pNodePoolListHead = DphNode;
    DphRoot->pNodePoolListTail = DphNode;

    /* Update byte counts taking in account this new node */
    DphRoot->nNodePools++;
    DphRoot->nNodePoolBytes += DphNode->nVirtualBlockSize;
}

VOID NTAPI
RtlpDphPlaceOnVirtualList(PDPH_HEAP_ROOT DphRoot, PDPH_HEAP_BLOCK DphNode)
{
    /* Add it to the head of the virtual list */
    DphNode->pNextAlloc = DphRoot->pVirtualStorageListHead;
    if (!DphRoot->pVirtualStorageListHead)
        DphRoot->pVirtualStorageListTail = DphNode;
    DphRoot->pVirtualStorageListHead = DphNode;

    /* Update byte counts taking in account this new node */
    DphRoot->nVirtualStorageRanges++;
    DphRoot->nVirtualStorageBytes += DphNode->nVirtualBlockSize;
}

PDPH_HEAP_BLOCK NTAPI
RtlpDphTakeNodeFromUnusedList(PDPH_HEAP_ROOT DphRoot)
{
    PDPH_HEAP_BLOCK Node = DphRoot->pUnusedNodeListHead;
    PDPH_HEAP_BLOCK Next;

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
    /* Add it back to the head of the unused list */
    Node->pNextAlloc = DphRoot->pUnusedNodeListHead;
    if (!DphRoot->pUnusedNodeListHead) DphRoot->pUnusedNodeListTail = Node;
    DphRoot->pUnusedNodeListHead = Node;

    /* Increase amount of unused nodes */
    DphRoot->nUnusedNodes++;
}

VOID NTAPI
RtlpDphRemoveFromAvailableList(PDPH_HEAP_ROOT DphRoot,
                               PDPH_HEAP_BLOCK Node)
{
    /* Make sure Adjacency list pointers are biased */
    ASSERT(IS_BIASED_POINTER(Node->AdjacencyEntry.Flink));
    ASSERT(IS_BIASED_POINTER(Node->AdjacencyEntry.Blink));

    /* Remove it from the list */
    RemoveEntryList(&Node->AvailableEntry);

    /* Decrease heap counters */
    DphRoot->nAvailableAllocations--;
    DphRoot->nAvailableAllocationBytesCommitted -= Node->nVirtualBlockSize;

    /* Remove bias from the AdjacencyEntry pointer */
    POINTER_REMOVE_BIAS(Node->AdjacencyEntry.Flink);
    POINTER_REMOVE_BIAS(Node->AdjacencyEntry.Blink);
}

VOID NTAPI
RtlpDphCoalesceNodeIntoAvailable(PDPH_HEAP_ROOT DphRoot,
                                 PDPH_HEAP_BLOCK Node)
{
    UNIMPLEMENTED;
}

VOID NTAPI
RtlpDphAddNewPool(PDPH_HEAP_ROOT DphRoot, PDPH_HEAP_BLOCK NodeBlock, PVOID Virtual, SIZE_T Size, BOOLEAN PlaceOnPool)
{
    PDPH_HEAP_BLOCK DphNode, DphStartNode;
    ULONG NodeCount;

    NodeCount = (Size >> 6) - 1;
    DphStartNode = Virtual;

    /* Set pNextAlloc for all blocks */
    for (DphNode = Virtual; NodeCount > 0; DphNode++, NodeCount--)
        DphNode->pNextAlloc = DphNode + 1;

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
RtlpDphFindAvailableMemory(PDPH_HEAP_ROOT DphRoot,
                           SIZE_T Size,
                           PDPH_HEAP_BLOCK *Prev)
{
    UNIMPLEMENTED;
    return NULL;
}

PDPH_HEAP_BLOCK NTAPI
RtlpDphAllocateNode(PDPH_HEAP_ROOT DphRoot)
{
    PDPH_HEAP_BLOCK Node;
    NTSTATUS Status;
    ULONG Size = DPH_POOL_SIZE;
    PVOID Ptr;

    /* Check for the easy case */
    if (DphRoot->pUnusedNodeListHead)
    {
        /* Just take a node from this list */
        Node = RtlpDphTakeNodeFromUnusedList(DphRoot);
        ASSERT(Node);
        return Node;
    }

    /* There is a need to make free space */
    Node = RtlpDphFindAvailableMemory(DphRoot, DPH_POOL_SIZE, NULL);

    if (!DphRoot->pUnusedNodeListHead && !Node)
    {
        /* Retry with a smaller request */
        Size = PAGE_SIZE;
        Node = RtlpDphFindAvailableMemory(DphRoot, PAGE_SIZE, NULL);
    }

    if (!DphRoot->pUnusedNodeListHead)
    {
        if (Node)
        {
            RtlpDphRemoveFromAvailableList(DphRoot, Node);
            Ptr = Node->pVirtualBlock;
        }
        else
        {
            /* No free space, need to alloc a new VM block */
            Size = DPH_POOL_SIZE;
            Status = RtlpDphAllocateVm(&Ptr, DPH_RESERVE_SIZE, MEM_COMMIT, PAGE_NOACCESS);

            if (!NT_SUCCESS(Status))
            {
                /* Retry with a smaller size */
                Status = RtlpDphAllocateVm(&Ptr, 0x10000, MEM_COMMIT, PAGE_NOACCESS);
                if (!NT_SUCCESS(Status)) return NULL;
            }
        }

        /* VM is allocated at this point, set protection */
        Status = RtlpDphProtectVm(Ptr, Size, PAGE_READWRITE);
        if (!NT_SUCCESS(Status))
        {
            ASSERT(FALSE);
            return NULL;
        }

        /* Zero the memory */
        if (Node) RtlZeroMemory(Ptr, Size);

        /* Add a new pool based on this VM */
        RtlpDphAddNewPool(DphRoot, Node, Ptr, Size, TRUE);

        if (!Node)
        {
            ASSERT(FALSE);
        }
        else
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
    }

    return RtlpDphTakeNodeFromUnusedList(DphRoot);
}

RTL_GENERIC_COMPARE_RESULTS
NTAPI
RtlpDphCompareNodeForTable(IN PRTL_AVL_TABLE Table,
                           IN PVOID FirstStruct,
                           IN PVOID SecondStruct)
{
    /* FIXME: TODO */
    ASSERT(FALSE);
    return 0;
}

PVOID
NTAPI
RtlpDphAllocateNodeForTable(IN PRTL_AVL_TABLE Table,
                            IN CLONG ByteSize)
{
    /* FIXME: TODO */
    ASSERT(FALSE);
    return NULL;
}

VOID
NTAPI
RtlpDphFreeNodeForTable(IN PRTL_AVL_TABLE Table,
                        IN PVOID Buffer)
{
    /* FIXME: TODO */
    ASSERT(FALSE);
}

NTSTATUS NTAPI
RtlpDphInitializeDelayedFreeQueue()
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
RtlpDphTargetDllsLogicInitialize()
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

VOID NTAPI
RtlpDphInternalValidatePageHeap(PDPH_HEAP_ROOT DphRoot, PVOID Address, ULONG Value)
{
    UNIMPLEMENTED;
}

NTSTATUS NTAPI
RtlpDphProcessStartupInitialization()
{
    NTSTATUS Status;
    PTEB Teb = NtCurrentTeb();

    /* Initialize the DPH heap list and its critical section */
    InitializeListHead(&RtlpDphPageHeapList);
    Status = RtlInitializeCriticalSection(&RtlpDphPageHeapListLock);
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

    DPRINT1("Page heap: pid 0x%X: page heap enabled with flags 0x%X.\n", Teb->ClientId.UniqueProcess, RtlpDphGlobalFlags);

    return Status;
}

HANDLE NTAPI
RtlpPageHeapCreate(ULONG Flags,
                   PVOID Addr,
                   SIZE_T TotalSize,
                   SIZE_T CommitSize,
                   PVOID Lock,
                   PRTL_HEAP_PARAMETERS Parameters)
{
    PVOID Base;
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
    RtlFillMemory(Base, PAGE_SIZE, DPH_FILL);

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
    DphRoot->HeapCritSect = (PRTL_CRITICAL_SECTION)((PCHAR)DphRoot + DPH_POOL_SIZE);
    DphRoot->ExtraFlags = RtlpDphGlobalFlags;

    ZwQueryPerformanceCounter(&PerfCounter, NULL);
    DphRoot->Seed = PerfCounter.LowPart;

    RtlInitializeCriticalSection(DphRoot->HeapCritSect);

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
    RtlEnterCriticalSection(&RtlpDphPageHeapListLock);

    /* Insert this heap to the tail of the global list */
    InsertTailList(&RtlpDphPageHeapList, &DphRoot->NextHeap);

    /* Note we increased the size of the list */
    RtlpDphPageHeapListLength++;

    /* Release the heap list lock */
    RtlLeaveCriticalSection(&RtlpDphPageHeapListLock);

    if (RtlpDphDebugOptions & DPH_DEBUG_VERBOSE)
    {
        DPRINT1("Page heap: process 0x%X created heap @ %p (%p, flags 0x%X)\n",
            NtCurrentTeb()->ClientId.UniqueProcess, (PUCHAR)DphRoot - PAGE_SIZE, DphRoot->NormalHeap, DphRoot->ExtraFlags);
    }

    /* Perform internal validation if required */
    if (RtlpDphDebugOptions & DPH_DEBUG_INTERNAL_VALIDATE)
        RtlpDphInternalValidatePageHeap(DphRoot, NULL, 0);

    return (PUCHAR)DphRoot - PAGE_SIZE;
}

PVOID NTAPI
RtlpPageHeapDestroy(HANDLE HeapPtr)
{
    return FALSE;
}

PVOID NTAPI
RtlpPageHeapAllocate(IN PVOID HeapPtr,
                     IN ULONG Flags,
                     IN SIZE_T Size)
{
    return NULL;
}

BOOLEAN NTAPI
RtlpPageHeapFree(HANDLE HeapPtr,
                 ULONG Flags,
                 PVOID Ptr)
{
    return FALSE;
}

PVOID NTAPI
RtlpPageHeapReAllocate(HANDLE HeapPtr,
                       ULONG Flags,
                       PVOID Ptr,
                       SIZE_T Size)
{
    return NULL;
}

BOOLEAN NTAPI
RtlpPageHeapGetUserInfo(PVOID HeapHandle,
                        ULONG Flags,
                        PVOID BaseAddress,
                        PVOID *UserValue,
                        PULONG UserFlags)
{
    return FALSE;
}

BOOLEAN NTAPI
RtlpPageHeapSetUserValue(PVOID HeapHandle,
                         ULONG Flags,
                         PVOID BaseAddress,
                         PVOID UserValue)
{
    return FALSE;
}

BOOLEAN
NTAPI
RtlpPageHeapSetUserFlags(PVOID HeapHandle,
                         ULONG Flags,
                         PVOID BaseAddress,
                         ULONG UserFlagsReset,
                         ULONG UserFlagsSet)
{
    return FALSE;
}

SIZE_T NTAPI
RtlpPageHeapSize(HANDLE HeapPtr,
                 ULONG Flags,
                 PVOID Ptr)
{
    return 0;
}

/* EOF */
