/*++

Copyright (c) 1994-2000  Microsoft Corporation

Module Name:

    heappagi.h

Abstract:

    The following definitions are internal to the debug heap manager,
    but are placed in this include file so that debugger extensions
    can reference the same structure definitions.  The following
    definitions are not intended to be referenced externally except
    by debugger extensions.

Author:

    Tom McGuire (TomMcg) 06-Jan-1995
    Silviu Calinoiu (SilviuC) 22-Feb-2000

Revision History:

--*/

#ifndef _HEAP_PAGE_I_
#define _HEAP_PAGE_I_

#ifdef DEBUG_PAGE_HEAP

#include "heap.h"

#define DPH_INTERNAL_DEBUG      0   // change to 0 or #undef for production code

//
// Stack trace size. 
//
                                
#define DPH_MAX_STACK_LENGTH   16

//
// Capture stacktraces in any context (x86/alpha, fre/chk). On alpha
// the stack acquisition function will fail and no stack trace will be
// acquired but in case we will find a better algorithm the page heap
// code will automatically take advantage of that.
//

#define DPH_CAPTURE_STACK_TRACE 1

//
// DPH_HEAP_BLOCK
//

typedef struct _DPH_HEAP_BLOCK DPH_HEAP_BLOCK, *PDPH_HEAP_BLOCK;

struct _DPH_HEAP_BLOCK {

    //
    //  Singly linked list of allocations (pNextAlloc must be
    //  first member in structure).
    //

    PDPH_HEAP_BLOCK pNextAlloc;

    //
    //   | PAGE_READWRITE          | PAGE_NOACCESS           |
    //   |____________________|___||_________________________|
    //
    //   ^pVirtualBlock       ^pUserAllocation
    //
    //   |---------------- nVirtualBlockSize ----------------|
    //
    //   |---nVirtualAccessSize----|
    //
    //                        |---|  nUserRequestedSize
    //
    //                        |----|  nUserActualSize
    //

    PUCHAR pVirtualBlock;
    SIZE_T  nVirtualBlockSize;

    SIZE_T  nVirtualAccessSize;
    PUCHAR pUserAllocation;
    SIZE_T  nUserRequestedSize;
    SIZE_T  nUserActualSize;
    PVOID  UserValue;
    ULONG  UserFlags;

    PRTL_TRACE_BLOCK StackTrace;
};


typedef struct _DPH_HEAP_ROOT DPH_HEAP_ROOT, *PDPH_HEAP_ROOT;

struct _DPH_HEAP_ROOT {

    //
    //  Maintain a signature (DPH_HEAP_SIGNATURE) as the
    //  first value in the heap root structure.
    //

    ULONG                 Signature;
    ULONG                 HeapFlags;

    //
    //  Access to this heap is synchronized with a critical section.
    //

    PRTL_CRITICAL_SECTION HeapCritSect;
    ULONG                 nRemoteLockAcquired;

    //
    //  The "VirtualStorage" list only uses the pVirtualBlock,
    //  nVirtualBlockSize, and nVirtualAccessSize fields of the
    //  HEAP_ALLOCATION structure.  This is the list of virtual
    //  allocation entries that all the heap allocations are
    //  taken from.
    //

    PDPH_HEAP_BLOCK  pVirtualStorageListHead;
    PDPH_HEAP_BLOCK  pVirtualStorageListTail;
    ULONG                 nVirtualStorageRanges;
    SIZE_T                 nVirtualStorageBytes;

    //
    //  The "Busy" list is the list of active heap allocations.
    //  It is stored in LIFO order to improve temporal locality
    //  for linear searches since most initial heap allocations
    //  tend to remain permanent throughout a process's lifetime.
    //

    PDPH_HEAP_BLOCK  pBusyAllocationListHead;
    PDPH_HEAP_BLOCK  pBusyAllocationListTail;
    ULONG                 nBusyAllocations;
    SIZE_T                 nBusyAllocationBytesCommitted;

    //
    //  The "Free" list is the list of freed heap allocations, stored
    //  in FIFO order to increase the length of time a freed block
    //  remains on the freed list without being used to satisfy an
    //  allocation request.  This increases the odds of catching
    //  a reference-after-freed bug in an app.
    //

    PDPH_HEAP_BLOCK  pFreeAllocationListHead;
    PDPH_HEAP_BLOCK  pFreeAllocationListTail;
    ULONG                 nFreeAllocations;
    SIZE_T                 nFreeAllocationBytesCommitted;

    //
    //  The "Available" list is stored in address-sorted order to facilitate
    //  coalescing.  When an allocation request cannot be satisfied from the
    //  "Available" list, it is attempted from the free list.  If it cannot
    //  be satisfied from the free list, the free list is coalesced into the
    //  available list.  If the request still cannot be satisfied from the
    //  coalesced available list, new VM is added to the available list.
    //

    PDPH_HEAP_BLOCK  pAvailableAllocationListHead;
    PDPH_HEAP_BLOCK  pAvailableAllocationListTail;
    ULONG                 nAvailableAllocations;
    SIZE_T                 nAvailableAllocationBytesCommitted;

    //
    //  The "UnusedNode" list is simply a list of available node
    //  entries to place "Busy", "Free", or "Virtual" entries.
    //  When freed nodes get coalesced into a single free node,
    //  the other "unused" node goes on this list.  When a new
    //  node is needed (like an allocation not satisfied from the
    //  free list), the node comes from this list if it's not empty.
    //

    PDPH_HEAP_BLOCK  pUnusedNodeListHead;
    PDPH_HEAP_BLOCK  pUnusedNodeListTail;
    ULONG                 nUnusedNodes;

    SIZE_T                 nBusyAllocationBytesAccessible;

    //
    //  Node pools need to be tracked so they can be protected
    //  from app scribbling on them.
    //

    PDPH_HEAP_BLOCK  pNodePoolListHead;
    PDPH_HEAP_BLOCK  pNodePoolListTail;
    ULONG                 nNodePools;
    SIZE_T                 nNodePoolBytes;

    //
    //  Doubly linked list of DPH heaps in process is tracked through this.
    //

    PDPH_HEAP_ROOT        pNextHeapRoot;
    PDPH_HEAP_ROOT        pPrevHeapRoot;

    ULONG                 nUnProtectionReferenceCount;
    ULONG                 InsideAllocateNode;           // only for debugging

    //
    // These are extra flags used to control page heap behavior.
    // During heap creation the current value of the global page heap
    // flags (process wise) is written into this field.
    //

    ULONG                 ExtraFlags;

    //
    // Seed for the random generator used to decide from where
    // should we make an allocation (normal or verified heap).
    // The field is protected by the critical section associated
    // with each page heap.
    //

    ULONG                  Seed;
    ULONG                  Counter[16];

    //
    // `NormalHeap' is used in case we want to combine verified allocations
    // with normal ones. This is useful to minimize memory impact. Without
    // this feature certain processes that are very heap intensive cannot
    // be verified at all.
    //

    PVOID                 NormalHeap;

    //
    // Heap creation stack trace.
    //

    PRTL_TRACE_BLOCK      CreateStackTrace;
};


//
// Page heap and global counters
//

#define DPH_COUNTER_SIZE_BELOW_1K            0
#define DPH_COUNTER_SIZE_BELOW_4K            1
#define DPH_COUNTER_SIZE_ABOVE_4K            2
#define DPH_COUNTER_NO_BLOCK_INFORMATION     3
#define DPH_COUNTER_NO_OF_ALLOCS             4
#define DPH_COUNTER_NO_OF_REALLOCS           5
#define DPH_COUNTER_NO_OF_FREES              6
#define DPH_COUNTER_NO_OF_NORMAL_ALLOCS      7
#define DPH_COUNTER_NO_OF_NORMAL_REALLOCS    8
#define DPH_COUNTER_NO_OF_NORMAL_FREES       9

//
// DPH_BLOCK_INFORMATION
//
// This structure is stored in every page heap allocated block
// if allocation size permits to stuff the info. It is completely
// redundant information. Its purpose is to ease debugging.
//
// If there is not enough empty space in the allocation (e.g. size
// of the allocation is close to PAGE_SIZE) then we will not force
// this information because this will modify the required size of
// the block and the memory usage pattern.
//
// This information is not saved if the catch backward overruns
// flag is set.
//

#define DPH_NORMAL_BLOCK_START_STAMP_ALLOCATED   0xABCDAAAA
#define DPH_NORMAL_BLOCK_END_STAMP_ALLOCATED     0xDCBAAAAA
#define DPH_NORMAL_BLOCK_START_STAMP_FREE        (0xABCDAAAA - 1)
#define DPH_NORMAL_BLOCK_END_STAMP_FREE          (0xDCBAAAAA - 1)

#define DPH_PAGE_BLOCK_START_STAMP_ALLOCATED     0xABCDBBBB
#define DPH_PAGE_BLOCK_END_STAMP_ALLOCATED       0xDCBABBBB
#define DPH_PAGE_BLOCK_START_STAMP_FREE          (0xABCDBBBB - 1)
#define DPH_PAGE_BLOCK_END_STAMP_FREE            (0xDCBABBBB - 1)

//silviuc:obsolete
#define DPH_BLOCK_INFORMATION_TRACE_SIZE 9

#define DPH_NORMAL_BLOCK_SUFFIX 	0xA0
#define DPH_PAGE_BLOCK_PREFIX 	    0xB0
#define DPH_PAGE_BLOCK_INFIX 	    0xC0
#define DPH_PAGE_BLOCK_SUFFIX 	    0xD0
#define DPH_NORMAL_BLOCK_INFIX 	    0xE0
#define DPH_FREE_BLOCK_INFIX 	    0xF0

typedef struct _DPH_BLOCK_INFORMATION {

    ULONG StartStamp;

    PVOID Heap;
    SIZE_T RequestedSize;
    SIZE_T ActualSize;
    LIST_ENTRY FreeQueue;
    PVOID StackTrace;
    
    ULONG EndStamp;

    //
    // (SilviuC): This structure needs to be 8-byte aligned.
    // If it is not, applications expecting aligned blocks will get
    // unaligned ones because this structure will prefix their
    // allocations. Internet Explorer is one such application
    // that stops working in these conditions.
    //

} DPH_BLOCK_INFORMATION, * PDPH_BLOCK_INFORMATION;

//
// Error reasons used in debug messages
//

#define DPH_SUCCESS                           0x0000
#define DPH_ERROR_CORRUPTED_START_STAMP       0x0001
#define DPH_ERROR_CORRUPTED_END_STAMP         0x0002
#define DPH_ERROR_CORRUPTED_HEAP_POINTER      0x0004
#define DPH_ERROR_CORRUPTED_PREFIX_PATTERN    0x0008
#define DPH_ERROR_CORRUPTED_SUFFIX_PATTERN    0x0010
#define DPH_ERROR_RAISED_EXCEPTION            0x0020
#define DPH_ERROR_NO_NORMAL_HEAP              0x0040
#define DPH_ERROR_CORRUPTED_INFIX_PATTERN     0x0080


#endif // DEBUG_PAGE_HEAP

#endif // _HEAP_PAGE_I_
