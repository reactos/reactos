/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/image.c
 * PURPOSE:         Image handling functions
 * PROGRAMMERS:     Copyright 1996 Alexandre Julliard
 *                  Copyright 1998 Ulrich Weigand
 */

//
// Note: This is a slightly modified implementation of WINE's.
//
// WINE's implementation is a hack based on Windows 95's heap implementation,
// itself a hack of DOS memory management.It supports 3 out of the 18 possible
// NT Heap Flags, does not support custom allocation/deallocation routines,
// and is about 50-80x slower with fragmentation rates up to 500x higher when
// compared to NT's LFH. WINE is lucky because the advanced NT Heap features are
// used in kernel-mode usually, not in user-mode, and they are crossing their
// fingers for this being the same. Note that several high-end SQL/Database
// applications would significantly benefit from custom heap features provided
// by NT.
//
// ROS's changes include:
//  - Using Zw instead of Nt calls, because this matters when in Kernel Mode
//  - Not using per-process heap lists while in Kernel Mode
//  - Using a macro to handle the Critical Section, because it's meaningless
//    in Kernel Mode.
//  - Crappy support for a custom Commit routine.
//  - Crappy support for User-defined flags and the User-defined value.
//  - Ripping out all the code for shared heaps, because those don't exist on NT.
//
// Be aware of these changes when you try to sync something back.
//

/* INCLUDES *****************************************************************/

#include <rtl.h>
#undef LIST_FOR_EACH
#undef LIST_FOR_EACH_SAFE
#include <wine/list.h>

#define NDEBUG
#include <debug.h>

#define TRACE DPRINT
#define WARN DPRINT1
#define ERR DPRINT1
#define DPRINTF DPRINT

/* FUNCTIONS *****************************************************************/

#define WARN_ON(x) (1)

#ifdef NDEBUG
#define TRACE_ON(x) (0)
#else
#define TRACE_ON(x) (1)
#endif

/* Note: the heap data structures are based on what Pietrek describes in his
 * book 'Windows 95 System Programming Secrets'. The layout is not exactly
 * the same, but could be easily adapted if it turns out some programs
 * require it.
 */

/* FIXME: use SIZE_T for 'size' structure members, but we need to make sure
 * that there is no unaligned accesses to structure fields.
 */

typedef struct tagARENA_INUSE
{
    DWORD  size;                    /* Block size; must be the first field */
    DWORD  magic : 24;              /* Magic number */
    DWORD  unused_bytes : 8;        /* Number of bytes in the block not used by user data (max value is HEAP_MIN_DATA_SIZE+HEAP_MIN_SHRINK_SIZE) */
} ARENA_INUSE;

typedef struct tagARENA_FREE
{
    DWORD                 size;     /* Block size; must be the first field */
    DWORD                 magic;    /* Magic number */
    struct list           entry;    /* Entry in free list */
} ARENA_FREE;

#define ARENA_FLAG_FREE        0x00000001  /* flags OR'ed with arena size */
#define ARENA_FLAG_PREV_FREE   0x00000002
#define ARENA_SIZE_MASK        (~3)
#define ARENA_INUSE_MAGIC      0x455355        /* Value for arena 'magic' field */
#define ARENA_FREE_MAGIC       0x45455246      /* Value for arena 'magic' field */

#define ARENA_INUSE_FILLER     0x55
#define ARENA_FREE_FILLER      0xaa

#define ALIGNMENT              8   /* everything is aligned on 8 byte boundaries */
#define ROUND_SIZE(size)       (((size) + ALIGNMENT - 1) & ~(ALIGNMENT-1))

#define QUIET                  1           /* Suppress messages  */
#define NOISY                  0           /* Report all errors  */

/* minimum data size (without arenas) of an allocated block */
#define HEAP_MIN_DATA_SIZE    16
/* minimum size that must remain to shrink an allocated block */
#define HEAP_MIN_SHRINK_SIZE  (HEAP_MIN_DATA_SIZE+sizeof(ARENA_FREE))

#define HEAP_NB_FREE_LISTS   4   /* Number of free lists */

/* Max size of the blocks on the free lists */
static const DWORD HEAP_freeListSizes[HEAP_NB_FREE_LISTS] =
{
    0x20, 0x80, 0x200, ~0UL
};

typedef struct
{
    ARENA_FREE  arena;
} FREE_LIST_ENTRY;

struct tagHEAP;

typedef struct tagSUBHEAP
{
    DWORD               size;       /* Size of the whole sub-heap */
    DWORD               commitSize; /* Committed size of the sub-heap */
    DWORD               headerSize; /* Size of the heap header */
    struct tagSUBHEAP  *next;       /* Next sub-heap */
    struct tagHEAP     *heap;       /* Main heap structure */
    DWORD               magic;      /* Magic number */
    ULONG UserFlags;
    PVOID UserValue;
} SUBHEAP;

#define SUBHEAP_MAGIC    ((DWORD)('S' | ('U'<<8) | ('B'<<16) | ('H'<<24)))

typedef struct tagHEAP
{
    SUBHEAP          subheap;       /* First sub-heap */
    struct list      entry;         /* Entry in process heap list */
    RTL_CRITICAL_SECTION critSection; /* Critical section for serialization */
    FREE_LIST_ENTRY  freeList[HEAP_NB_FREE_LISTS];  /* Free lists */
    DWORD            flags;         /* Heap flags */
    DWORD            magic;         /* Magic number */
    PRTL_HEAP_COMMIT_ROUTINE commitRoutine;
} HEAP;

#define HEAP_MAGIC       ((DWORD)('H' | ('E'<<8) | ('A'<<16) | ('P'<<24)))

#define HEAP_DEF_SIZE        0x110000   /* Default heap size = 1Mb + 64Kb */
#define COMMIT_MASK          0xffff  /* bitmask for commit/decommit granularity */

static HEAP *processHeap;  /* main process heap */

static BOOL HEAP_IsRealArena( HEAP *heapPtr, DWORD flags, LPCVOID block, BOOL quiet );

/* mark a block of memory as free for debugging purposes */
static __inline void mark_block_free( void *ptr, SIZE_T size )
{
    if (TRACE_ON(heap)) memset( ptr, ARENA_FREE_FILLER, size );
#ifdef VALGRIND_MAKE_NOACCESS
    VALGRIND_DISCARD( VALGRIND_MAKE_NOACCESS( ptr, size ));
#endif
}

/* mark a block of memory as initialized for debugging purposes */
static __inline void mark_block_initialized( void *ptr, SIZE_T size )
{
#ifdef VALGRIND_MAKE_READABLE
    VALGRIND_DISCARD( VALGRIND_MAKE_READABLE( ptr, size ));
#endif
}

/* mark a block of memory as uninitialized for debugging purposes */
static __inline void mark_block_uninitialized( void *ptr, SIZE_T size )
{
#ifdef VALGRIND_MAKE_WRITABLE
    VALGRIND_DISCARD( VALGRIND_MAKE_WRITABLE( ptr, size ));
#endif
    if (TRACE_ON(heap))
    {
        memset( ptr, ARENA_INUSE_FILLER, size );
#ifdef VALGRIND_MAKE_WRITABLE
        /* make it uninitialized to valgrind again */
        VALGRIND_DISCARD( VALGRIND_MAKE_WRITABLE( ptr, size ));
#endif
    }
}

/* clear contents of a block of memory */
static __inline void clear_block( void *ptr, SIZE_T size )
{
    mark_block_initialized( ptr, size );
    memset( ptr, 0, size );
}

/* locate a free list entry of the appropriate size */
/* size is the size of the whole block including the arena header */
static __inline unsigned int get_freelist_index( SIZE_T size )
{
    unsigned int i;

    size -= sizeof(ARENA_FREE);
    for (i = 0; i < HEAP_NB_FREE_LISTS - 1; i++) if (size <= HEAP_freeListSizes[i]) break;
    return i;
}

static RTL_CRITICAL_SECTION_DEBUG process_heap_critsect_debug =
{
    0, 0, NULL,  /* will be set later */
    { &process_heap_critsect_debug.ProcessLocksList, &process_heap_critsect_debug.ProcessLocksList },
      0, 0, 0, 0, 0
};

/***********************************************************************
 *           HEAP_Dump
 */
static void HEAP_Dump( HEAP *heap )
{
    int i;
    SUBHEAP *subheap;
    char *ptr;

    DPRINTF( "Heap: %p\n", heap );
    DPRINTF( "Next: %p  Sub-heaps: %p",
             LIST_ENTRY( heap->entry.next, HEAP, entry ), &heap->subheap );
    subheap = &heap->subheap;
    while (subheap->next)
    {
        DPRINTF( " -> %p", subheap->next );
        subheap = subheap->next;
    }

    DPRINTF( "\nFree lists:\n Block   Stat   Size    Id\n" );
    for (i = 0; i < HEAP_NB_FREE_LISTS; i++)
        DPRINTF( "%p free %08lx prev=%p next=%p\n",
                 &heap->freeList[i].arena, HEAP_freeListSizes[i],
                 LIST_ENTRY( heap->freeList[i].arena.entry.prev, ARENA_FREE, entry ),
                 LIST_ENTRY( heap->freeList[i].arena.entry.next, ARENA_FREE, entry ));

    subheap = &heap->subheap;
    while (subheap)
    {
        SIZE_T freeSize = 0, usedSize = 0, arenaSize = subheap->headerSize;
        DPRINTF( "\n\nSub-heap %p: size=%08lx committed=%08lx\n",
                subheap, subheap->size, subheap->commitSize );

        DPRINTF( "\n Block   Stat   Size    Id\n" );
        ptr = (char*)subheap + subheap->headerSize;
        while (ptr < (char *)subheap + subheap->size)
        {
            if (*(DWORD *)ptr & ARENA_FLAG_FREE)
            {
                ARENA_FREE *pArena = (ARENA_FREE *)ptr;
                DPRINTF( "%p free %08lx prev=%p next=%p\n",
                         pArena, pArena->size & ARENA_SIZE_MASK,
                         LIST_ENTRY( pArena->entry.prev, ARENA_FREE, entry ),
                         LIST_ENTRY( pArena->entry.next, ARENA_FREE, entry ) );
                ptr += sizeof(*pArena) + (pArena->size & ARENA_SIZE_MASK);
                arenaSize += sizeof(ARENA_FREE);
                freeSize += pArena->size & ARENA_SIZE_MASK;
            }
            else if (*(DWORD *)ptr & ARENA_FLAG_PREV_FREE)
            {
                ARENA_INUSE *pArena = (ARENA_INUSE *)ptr;
                DPRINTF( "%p Used %08lx back=%p\n",
                        pArena, pArena->size & ARENA_SIZE_MASK, *((ARENA_FREE **)pArena - 1) );
                ptr += sizeof(*pArena) + (pArena->size & ARENA_SIZE_MASK);
                arenaSize += sizeof(ARENA_INUSE);
                usedSize += pArena->size & ARENA_SIZE_MASK;
            }
            else
            {
                ARENA_INUSE *pArena = (ARENA_INUSE *)ptr;
                DPRINTF( "%p used %08lx\n", pArena, pArena->size & ARENA_SIZE_MASK );
                ptr += sizeof(*pArena) + (pArena->size & ARENA_SIZE_MASK);
                arenaSize += sizeof(ARENA_INUSE);
                usedSize += pArena->size & ARENA_SIZE_MASK;
            }
        }
        DPRINTF( "\nTotal: Size=%08lx Committed=%08lx Free=%08lx Used=%08lx Arenas=%08lx (%ld%%)\n\n",
	      subheap->size, subheap->commitSize, freeSize, usedSize,
	      arenaSize, (arenaSize * 100) / subheap->size );
        subheap = subheap->next;
    }
}

#if 0
static void HEAP_DumpEntry( LPPROCESS_HEAP_ENTRY entry )
{
    WORD rem_flags;
    TRACE( "Dumping entry %p\n", entry );
    TRACE( "lpData\t\t: %p\n", entry->lpData );
    TRACE( "cbData\t\t: %08lx\n", entry->cbData);
    TRACE( "cbOverhead\t: %08x\n", entry->cbOverhead);
    TRACE( "iRegionIndex\t: %08x\n", entry->iRegionIndex);
    TRACE( "WFlags\t\t: ");
    if (entry->wFlags & PROCESS_HEAP_REGION)
        TRACE( "PROCESS_HEAP_REGION ");
    if (entry->wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE)
        TRACE( "PROCESS_HEAP_UNCOMMITTED_RANGE ");
    if (entry->wFlags & PROCESS_HEAP_ENTRY_BUSY)
        TRACE( "PROCESS_HEAP_ENTRY_BUSY ");
    if (entry->wFlags & PROCESS_HEAP_ENTRY_MOVEABLE)
        TRACE( "PROCESS_HEAP_ENTRY_MOVEABLE ");
    if (entry->wFlags & PROCESS_HEAP_ENTRY_DDESHARE)
        TRACE( "PROCESS_HEAP_ENTRY_DDESHARE ");
    rem_flags = entry->wFlags &
        ~(PROCESS_HEAP_REGION | PROCESS_HEAP_UNCOMMITTED_RANGE |
          PROCESS_HEAP_ENTRY_BUSY | PROCESS_HEAP_ENTRY_MOVEABLE|
          PROCESS_HEAP_ENTRY_DDESHARE);
    if (rem_flags)
        TRACE( "Unknown %08x", rem_flags);
    TRACE( "\n");
    if ((entry->wFlags & PROCESS_HEAP_ENTRY_BUSY )
        && (entry->wFlags & PROCESS_HEAP_ENTRY_MOVEABLE))
    {
        /* Treat as block */
        TRACE( "BLOCK->hMem\t\t:%p\n", entry->Block.hMem);
    }
    if (entry->wFlags & PROCESS_HEAP_REGION)
    {
        TRACE( "Region.dwCommittedSize\t:%08lx\n",entry->Region.dwCommittedSize);
        TRACE( "Region.dwUnCommittedSize\t:%08lx\n",entry->Region.dwUnCommittedSize);
        TRACE( "Region.lpFirstBlock\t:%p\n",entry->Region.lpFirstBlock);
        TRACE( "Region.lpLastBlock\t:%p\n",entry->Region.lpLastBlock);
    }
}
#endif

/***********************************************************************
 *           HEAP_GetPtr
 * RETURNS
 *	Pointer to the heap
 *	NULL: Failure
 */
static HEAP *HEAP_GetPtr(
             HANDLE heap /* [in] Handle to the heap */
) {
    HEAP *heapPtr = (HEAP *)heap;
    if (!heapPtr || (heapPtr->magic != HEAP_MAGIC))
    {
        ERR("Invalid heap %p!\n", heap );
        return NULL;
    }
    if (TRACE_ON(heap) && !HEAP_IsRealArena( heapPtr, 0, NULL, NOISY ))
    {
        HEAP_Dump( heapPtr );
        assert( FALSE );
        return NULL;
    }
    return heapPtr;
}


/***********************************************************************
 *           HEAP_InsertFreeBlock
 *
 * Insert a free block into the free list.
 */
static __inline void HEAP_InsertFreeBlock( HEAP *heap, ARENA_FREE *pArena, BOOL last )
{
    FREE_LIST_ENTRY *pEntry = heap->freeList + get_freelist_index( pArena->size + sizeof(*pArena) );
    if (last)
    {
        /* insert at end of free list, i.e. before the next free list entry */
        pEntry++;
        if (pEntry == &heap->freeList[HEAP_NB_FREE_LISTS]) pEntry = heap->freeList;
        list_add_before( &pEntry->arena.entry, &pArena->entry );
    }
    else
    {
        /* insert at head of free list */
        list_add_after( &pEntry->arena.entry, &pArena->entry );
    }
    pArena->size |= ARENA_FLAG_FREE;
}


/***********************************************************************
 *           HEAP_FindSubHeap
 * Find the sub-heap containing a given address.
 *
 * RETURNS
 *	Pointer: Success
 *	NULL: Failure
 */
static SUBHEAP *HEAP_FindSubHeap(
                const HEAP *heap, /* [in] Heap pointer */
                LPCVOID ptr /* [in] Address */
) {
    const SUBHEAP *sub = &heap->subheap;
    while (sub)
    {
        if (((const char *)ptr >= (const char *)sub) &&
            ((const char *)ptr < (const char *)sub + sub->size)) return (SUBHEAP*)sub;
        sub = sub->next;
    }
    return NULL;
}

/***********************************************************************
 *           HEAP_Commit
 *
 * Make sure the heap storage is committed for a given size in the specified arena.
 */
static __inline BOOL HEAP_Commit( SUBHEAP *subheap, ARENA_INUSE *pArena, SIZE_T data_size )
{
    NTSTATUS Status;
    void *ptr = (char *)(pArena + 1) + data_size + sizeof(ARENA_FREE);
    SIZE_T size = (char *)ptr - (char *)subheap;
    size = (size + COMMIT_MASK) & ~COMMIT_MASK;
    if (size > subheap->size) size = subheap->size;
    if (size <= subheap->commitSize) return TRUE;
    size -= subheap->commitSize;
    ptr = (char *)subheap + subheap->commitSize;
   if (subheap->heap->commitRoutine != NULL)
   {
      Status = subheap->heap->commitRoutine(subheap->heap, &ptr, &size);
   }
   else
   {
        Status = ZwAllocateVirtualMemory( NtCurrentProcess(), &ptr, 0,
                                 &size, MEM_COMMIT, PAGE_READWRITE );
   }
    if (!NT_SUCCESS(Status))
    {
        WARN("Could not commit %08lx bytes at %p for heap %p\n",
                 size, ptr, subheap->heap );
        return FALSE;
    }
    subheap->commitSize += size;
    return TRUE;
}

#if 0
/***********************************************************************
 *           HEAP_Decommit
 *
 * If possible, decommit the heap storage from (including) 'ptr'.
 */
static inline BOOL HEAP_Decommit( SUBHEAP *subheap, void *ptr )
{
    void *addr;
    SIZE_T decommit_size;
    SIZE_T size = (char *)ptr - (char *)subheap;

    /* round to next block and add one full block */
    size = ((size + COMMIT_MASK) & ~COMMIT_MASK) + COMMIT_MASK + 1;
    if (size >= subheap->commitSize) return TRUE;
    decommit_size = subheap->commitSize - size;
    addr = (char *)subheap + size;

    if (ZwFreeVirtualMemory( NtCurrentProcess(), &addr, &decommit_size, MEM_DECOMMIT ))
    {
        WARN("Could not decommit %08lx bytes at %p for heap %p\n",
                decommit_size, (char *)subheap + size, subheap->heap );
        return FALSE;
    }
    subheap->commitSize -= decommit_size;
    return TRUE;
}
#endif

/***********************************************************************
 *           HEAP_CreateFreeBlock
 *
 * Create a free block at a specified address. 'size' is the size of the
 * whole block, including the new arena.
 */
static void HEAP_CreateFreeBlock( SUBHEAP *subheap, void *ptr, SIZE_T size )
{
    ARENA_FREE *pFree;
    char *pEnd;
    BOOL last;

    /* Create a free arena */
    mark_block_uninitialized( ptr, sizeof( ARENA_FREE ) );
    pFree = (ARENA_FREE *)ptr;
    pFree->magic = ARENA_FREE_MAGIC;

    /* If debugging, erase the freed block content */

    pEnd = (char *)ptr + size;
    if (pEnd > (char *)subheap + subheap->commitSize) pEnd = (char *)subheap + subheap->commitSize;
    if (pEnd > (char *)(pFree + 1)) mark_block_free( pFree + 1, pEnd - (char *)(pFree + 1) );

    /* Check if next block is free also */

    if (((char *)ptr + size < (char *)subheap + subheap->size) &&
        (*(DWORD *)((char *)ptr + size) & ARENA_FLAG_FREE))
    {
        /* Remove the next arena from the free list */
        ARENA_FREE *pNext = (ARENA_FREE *)((char *)ptr + size);
        list_remove( &pNext->entry );
        size += (pNext->size & ARENA_SIZE_MASK) + sizeof(*pNext);
        mark_block_free( pNext, sizeof(ARENA_FREE) );
    }

    /* Set the next block PREV_FREE flag and pointer */

    last = ((char *)ptr + size >= (char *)subheap + subheap->size);
    if (!last)
    {
        DWORD *pNext = (DWORD *)((char *)ptr + size);
        *pNext |= ARENA_FLAG_PREV_FREE;
        mark_block_initialized( pNext - 1, sizeof( ARENA_FREE * ) );
        *((ARENA_FREE **)pNext - 1) = pFree;
    }

    /* Last, insert the new block into the free list */

    pFree->size = size - sizeof(*pFree);
    HEAP_InsertFreeBlock( subheap->heap, pFree, last );
}


/***********************************************************************
 *           HEAP_MakeInUseBlockFree
 *
 * Turn an in-use block into a free block. Can also decommit the end of
 * the heap, and possibly even free the sub-heap altogether.
 */
static void HEAP_MakeInUseBlockFree( SUBHEAP *subheap, ARENA_INUSE *pArena )
{
    ARENA_FREE *pFree;
    SIZE_T size = (pArena->size & ARENA_SIZE_MASK) + sizeof(*pArena);

    /* Check if we can merge with previous block */

    if (pArena->size & ARENA_FLAG_PREV_FREE)
    {
        pFree = *((ARENA_FREE **)pArena - 1);
        size += (pFree->size & ARENA_SIZE_MASK) + sizeof(ARENA_FREE);
        /* Remove it from the free list */
        list_remove( &pFree->entry );
    }
    else pFree = (ARENA_FREE *)pArena;

    /* Create a free block */

    HEAP_CreateFreeBlock( subheap, pFree, size );
    size = (pFree->size & ARENA_SIZE_MASK) + sizeof(ARENA_FREE);
    if ((char *)pFree + size < (char *)subheap + subheap->size)
        return;  /* Not the last block, so nothing more to do */

    /* Free the whole sub-heap if it's empty and not the original one */

    if (((char *)pFree == (char *)subheap + subheap->headerSize) &&
        (subheap != &subheap->heap->subheap))
    {
        SIZE_T size = 0;
        SUBHEAP *pPrev = &subheap->heap->subheap;
        /* Remove the free block from the list */
        list_remove( &pFree->entry );
        /* Remove the subheap from the list */
        while (pPrev && (pPrev->next != subheap)) pPrev = pPrev->next;
        if (pPrev) pPrev->next = subheap->next;
        /* Free the memory */
        subheap->magic = 0;
        ZwFreeVirtualMemory( NtCurrentProcess(), (void **)&subheap, &size, MEM_RELEASE );
        return;
    }

    /* Decommit the end of the heap */
}

/***********************************************************************
 *           HEAP_ShrinkBlock
 *
 * Shrink an in-use block.
 */
static void HEAP_ShrinkBlock(SUBHEAP *subheap, ARENA_INUSE *pArena, SIZE_T size)
{
    if ((pArena->size & ARENA_SIZE_MASK) >= size + HEAP_MIN_SHRINK_SIZE)
    {
        HEAP_CreateFreeBlock( subheap, (char *)(pArena + 1) + size,
                              (pArena->size & ARENA_SIZE_MASK) - size );
	/* assign size plus previous arena flags */
        pArena->size = size | (pArena->size & ~ARENA_SIZE_MASK);
    }
    else
    {
        /* Turn off PREV_FREE flag in next block */
        char *pNext = (char *)(pArena + 1) + (pArena->size & ARENA_SIZE_MASK);
        if (pNext < (char *)subheap + subheap->size)
            *(DWORD *)pNext &= ~ARENA_FLAG_PREV_FREE;
    }
}

/***********************************************************************
 *           HEAP_InitSubHeap
 */
static BOOL HEAP_InitSubHeap( HEAP *heap, LPVOID address, DWORD flags,
                              SIZE_T commitSize, SIZE_T totalSize,
                              PRTL_HEAP_PARAMETERS Parameters)
{
    SUBHEAP *subheap;
    FREE_LIST_ENTRY *pEntry;
    int i;
    NTSTATUS Status;

#if 0
    if (ZwAllocateVirtualMemory( NtCurrentProcess(), &address, 0,
                                 &commitSize, MEM_COMMIT, PAGE_READWRITE ))
    {
        WARN("Could not commit %08lx bytes for sub-heap %p\n", commitSize, address );
        return FALSE;
    }
#endif

    /* Fill the sub-heap structure */

    subheap = (SUBHEAP *)address;
    subheap->heap       = heap;
    subheap->size       = totalSize;
    subheap->commitSize = commitSize;
    subheap->magic      = SUBHEAP_MAGIC;

    if ( subheap != (SUBHEAP *)heap )
    {
        /* If this is a secondary subheap, insert it into list */

        subheap->headerSize = ROUND_SIZE( sizeof(SUBHEAP) );
        subheap->next       = heap->subheap.next;
        heap->subheap.next  = subheap;
    }
    else
    {
        /* If this is a primary subheap, initialize main heap */

        subheap->headerSize = ROUND_SIZE( sizeof(HEAP) );
        subheap->next       = NULL;
        heap->flags         = flags;
        heap->magic         = HEAP_MAGIC;
      if (Parameters)
         heap->commitRoutine = Parameters->CommitRoutine;
      else
         heap->commitRoutine = NULL;

        /* Build the free lists */

        list_init( &heap->freeList[0].arena.entry );
        for (i = 0, pEntry = heap->freeList; i < HEAP_NB_FREE_LISTS; i++, pEntry++)
        {
            pEntry->arena.size = 0 | ARENA_FLAG_FREE;
            pEntry->arena.magic = ARENA_FREE_MAGIC;
            if (i) list_add_after( &pEntry[-1].arena.entry, &pEntry->arena.entry );
        }

          /* Initialize critical section */

          if (RtlpGetMode() == UserMode)
          {
            if (!processHeap)  /* do it by hand to avoid memory allocations */
            {
                heap->critSection.DebugInfo      = &process_heap_critsect_debug;
                heap->critSection.LockCount      = -1;
                heap->critSection.RecursionCount = 0;
                heap->critSection.OwningThread   = 0;
                heap->critSection.LockSemaphore  = 0;
                heap->critSection.SpinCount      = 0;
                process_heap_critsect_debug.CriticalSection = &heap->critSection;
            }
            else RtlInitializeHeapLock( &heap->critSection );
          }
    }

    /* Commit memory */
    if (heap->commitRoutine)
    {
      if (subheap != (SUBHEAP *)heap)
      {
         Status = heap->commitRoutine(heap, &address, &commitSize);
      }
      else
      {
         /* the caller is responsible for committing the first page! */
         Status = STATUS_SUCCESS;
      }
    }
    else
    {
      Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                       &address,
                                       0,
                                       &commitSize,
                                       MEM_COMMIT,
                                       PAGE_EXECUTE_READWRITE);
    }
    if (!NT_SUCCESS(Status))
    {
      DPRINT("Could not commit %08lx bytes for sub-heap %p\n",
             commitSize, address);
      return FALSE;
    }

    /* Create the first free block */

    HEAP_CreateFreeBlock( subheap, (LPBYTE)subheap + subheap->headerSize,
                          subheap->size - subheap->headerSize );

    return TRUE;
}

/***********************************************************************
 *           HEAP_CreateSubHeap
 *
 * Create a sub-heap of the given size.
 * If heap == NULL, creates a main heap.
 */
static SUBHEAP *HEAP_CreateSubHeap( HEAP *heap, void *base, DWORD flags,
                                    SIZE_T commitSize, SIZE_T totalSize,
                                    IN PRTL_HEAP_PARAMETERS Parameters)
{
    LPVOID address = base;

    /* round-up sizes on a 64K boundary */
    totalSize  = (totalSize + 0xffff) & 0xffff0000;
    commitSize = (commitSize + 0xffff) & 0xffff0000;
    if (!commitSize) commitSize = 0x10000;
    if (totalSize < commitSize) totalSize = commitSize;

    if (!address)
    {
        /* allocate the memory block */
        if (ZwAllocateVirtualMemory( NtCurrentProcess(), &address, 0, &totalSize,
                                     MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE ))
        {
            WARN("Could not allocate %08lx bytes\n", totalSize );
            return NULL;
        }
    }

    /* Initialize subheap */

    if (!HEAP_InitSubHeap( heap ? heap : (HEAP *)address,
                           address, flags, commitSize, totalSize, Parameters ))
    {
        SIZE_T size = 0;
        if (!base) ZwFreeVirtualMemory( NtCurrentProcess(), &address, &size, MEM_RELEASE );
        return NULL;
    }

    return (SUBHEAP *)address;
}


/***********************************************************************
 *           HEAP_FindFreeBlock
 *
 * Find a free block at least as large as the requested size, and make sure
 * the requested size is committed.
 */
static ARENA_FREE *HEAP_FindFreeBlock( HEAP *heap, SIZE_T size,
                                       SUBHEAP **ppSubHeap )
{
    SUBHEAP *subheap;
    struct list *ptr;
    FREE_LIST_ENTRY *pEntry = heap->freeList + get_freelist_index( size + sizeof(ARENA_INUSE) );

    /* Find a suitable free list, and in it find a block large enough */

    ptr = &pEntry->arena.entry;
    while ((ptr = list_next( &heap->freeList[0].arena.entry, ptr )))
    {
        ARENA_FREE *pArena = LIST_ENTRY( ptr, ARENA_FREE, entry );
        SIZE_T arena_size = (pArena->size & ARENA_SIZE_MASK) +
                            sizeof(ARENA_FREE) - sizeof(ARENA_INUSE);
        if (arena_size >= size)
        {
            subheap = HEAP_FindSubHeap( heap, pArena );
            if (!HEAP_Commit( subheap, (ARENA_INUSE *)pArena, size )) return NULL;
            *ppSubHeap = subheap;
            return pArena;
        }
    }

    /* If no block was found, attempt to grow the heap */

    if (!(heap->flags & HEAP_GROWABLE))
    {
        WARN("Not enough space in heap %p for %08lx bytes\n", heap, size );
        return NULL;
    }
    /* make sure that we have a big enough size *committed* to fit another
     * last free arena in !
     * So just one heap struct, one first free arena which will eventually
     * get used, and a second free arena that might get assigned all remaining
     * free space in HEAP_ShrinkBlock() */
    size += ROUND_SIZE(sizeof(SUBHEAP)) + sizeof(ARENA_INUSE) + sizeof(ARENA_FREE);
    if (!(subheap = HEAP_CreateSubHeap( heap, NULL, heap->flags, size,
                                        max( HEAP_DEF_SIZE, size ), NULL )))
        return NULL;

    TRACE("created new sub-heap %p of %08lx bytes for heap %p\n",
            subheap, size, heap );

    *ppSubHeap = subheap;
    return (ARENA_FREE *)(subheap + 1);
}

/***********************************************************************
 *           HEAP_IsValidArenaPtr
 *
 * Check that the pointer is inside the range possible for arenas.
 */
static BOOL HEAP_IsValidArenaPtr( const HEAP *heap, const void *ptr )
{
    int i;
    const SUBHEAP *subheap = HEAP_FindSubHeap( heap, ptr );
    if (!subheap) return FALSE;
    if ((const char *)ptr >= (const char *)subheap + subheap->headerSize) return TRUE;
    if (subheap != &heap->subheap) return FALSE;
    for (i = 0; i < HEAP_NB_FREE_LISTS; i++)
        if (ptr == (const void *)&heap->freeList[i].arena) return TRUE;
    return FALSE;
}


/***********************************************************************
 *           HEAP_ValidateFreeArena
 */
static BOOL HEAP_ValidateFreeArena( SUBHEAP *subheap, ARENA_FREE *pArena )
{
    ARENA_FREE *prev, *next;
    char *heapEnd = (char *)subheap + subheap->size;

    /* Check for unaligned pointers */
    if ( (ULONG_PTR)pArena % ALIGNMENT != 0 )
    {
        ERR("Heap %p: unaligned arena pointer %p\n", subheap->heap, pArena );
        return FALSE;
    }

    /* Check magic number */
    if (pArena->magic != ARENA_FREE_MAGIC)
    {
        ERR("Heap %p: invalid free arena magic for %p\n", subheap->heap, pArena );
        return FALSE;
    }
    /* Check size flags */
    if (!(pArena->size & ARENA_FLAG_FREE) ||
        (pArena->size & ARENA_FLAG_PREV_FREE))
    {
        ERR("Heap %p: bad flags %08lx for free arena %p\n",
            subheap->heap, pArena->size & ~ARENA_SIZE_MASK, pArena );
        return FALSE;
    }
    /* Check arena size */
    if ((char *)(pArena + 1) + (pArena->size & ARENA_SIZE_MASK) > heapEnd)
    {
        ERR("Heap %p: bad size %08lx for free arena %p\n",
            subheap->heap, pArena->size & ARENA_SIZE_MASK, pArena );
        return FALSE;
    }
    /* Check that next pointer is valid */
    next = LIST_ENTRY( pArena->entry.next, ARENA_FREE, entry );
    if (!HEAP_IsValidArenaPtr( subheap->heap, next ))
    {
        ERR("Heap %p: bad next ptr %p for arena %p\n",
            subheap->heap, next, pArena );
        return FALSE;
    }
    /* Check that next arena is free */
    if (!(next->size & ARENA_FLAG_FREE) || (next->magic != ARENA_FREE_MAGIC))
    {
        ERR("Heap %p: next arena %p invalid for %p\n",
            subheap->heap, next, pArena );
        return FALSE;
    }
    /* Check that prev pointer is valid */
    prev = LIST_ENTRY( pArena->entry.prev, ARENA_FREE, entry );
    if (!HEAP_IsValidArenaPtr( subheap->heap, prev ))
    {
        ERR("Heap %p: bad prev ptr %p for arena %p\n",
            subheap->heap, prev, pArena );
        return FALSE;
    }
    /* Check that prev arena is free */
    if (!(prev->size & ARENA_FLAG_FREE) || (prev->magic != ARENA_FREE_MAGIC))
    {
	/* this often means that the prev arena got overwritten
	 * by a memory write before that prev arena */
        ERR("Heap %p: prev arena %p invalid for %p\n",
            subheap->heap, prev, pArena );
        return FALSE;
    }
    /* Check that next block has PREV_FREE flag */
    if ((char *)(pArena + 1) + (pArena->size & ARENA_SIZE_MASK) < heapEnd)
    {
        if (!(*(DWORD *)((char *)(pArena + 1) +
            (pArena->size & ARENA_SIZE_MASK)) & ARENA_FLAG_PREV_FREE))
        {
            ERR("Heap %p: free arena %p next block has no PREV_FREE flag\n",
                subheap->heap, pArena );
            return FALSE;
        }
        /* Check next block back pointer */
        if (*((ARENA_FREE **)((char *)(pArena + 1) +
            (pArena->size & ARENA_SIZE_MASK)) - 1) != pArena)
        {
            ERR("Heap %p: arena %p has wrong back ptr %p\n",
                subheap->heap, pArena,
                *((ARENA_FREE **)((char *)(pArena+1) + (pArena->size & ARENA_SIZE_MASK)) - 1));
            return FALSE;
        }
    }
    return TRUE;
}

/***********************************************************************
 *           HEAP_ValidateInUseArena
 */
static BOOL HEAP_ValidateInUseArena( const SUBHEAP *subheap, const ARENA_INUSE *pArena, BOOL quiet )
{
    const char *heapEnd = (const char *)subheap + subheap->size;

    /* Check for unaligned pointers */
    if ( (ULONG_PTR)pArena % ALIGNMENT != 0 )
    {
        if ( quiet == NOISY )
        {
            ERR( "Heap %p: unaligned arena pointer %p\n", subheap->heap, pArena );
            if ( TRACE_ON(heap) )
                HEAP_Dump( subheap->heap );
        }
        else if ( WARN_ON(heap) )
        {
            WARN( "Heap %p: unaligned arena pointer %p\n", subheap->heap, pArena );
            if ( TRACE_ON(heap) )
                HEAP_Dump( subheap->heap );
        }
        return FALSE;
    }

    /* Check magic number */
    if (pArena->magic != ARENA_INUSE_MAGIC)
    {
        if (quiet == NOISY) {
            ERR("Heap %p: invalid in-use arena magic for %p\n", subheap->heap, pArena );
            if (TRACE_ON(heap))
               HEAP_Dump( subheap->heap );
        }  else if (WARN_ON(heap)) {
            WARN("Heap %p: invalid in-use arena magic for %p\n", subheap->heap, pArena );
            if (TRACE_ON(heap))
               HEAP_Dump( subheap->heap );
        }
        return FALSE;
    }
    /* Check size flags */
    if (pArena->size & ARENA_FLAG_FREE)
    {
        ERR("Heap %p: bad flags %08lx for in-use arena %p\n",
            subheap->heap, pArena->size & ~ARENA_SIZE_MASK, pArena );
        return FALSE;
    }
    /* Check arena size */
    if ((const char *)(pArena + 1) + (pArena->size & ARENA_SIZE_MASK) > heapEnd)
    {
        ERR("Heap %p: bad size %08lx for in-use arena %p\n",
            subheap->heap, pArena->size & ARENA_SIZE_MASK, pArena );
        return FALSE;
    }
    /* Check next arena PREV_FREE flag */
    if (((const char *)(pArena + 1) + (pArena->size & ARENA_SIZE_MASK) < heapEnd) &&
        (*(const DWORD *)((const char *)(pArena + 1) + (pArena->size & ARENA_SIZE_MASK)) & ARENA_FLAG_PREV_FREE))
    {
        ERR("Heap %p: in-use arena %p next block has PREV_FREE flag\n",
            subheap->heap, pArena );
        return FALSE;
    }
    /* Check prev free arena */
    if (pArena->size & ARENA_FLAG_PREV_FREE)
    {
        const ARENA_FREE *pPrev = *((const ARENA_FREE * const*)pArena - 1);
        /* Check prev pointer */
        if (!HEAP_IsValidArenaPtr( subheap->heap, pPrev ))
        {
            ERR("Heap %p: bad back ptr %p for arena %p\n",
                subheap->heap, pPrev, pArena );
            return FALSE;
        }
        /* Check that prev arena is free */
        if (!(pPrev->size & ARENA_FLAG_FREE) ||
            (pPrev->magic != ARENA_FREE_MAGIC))
        {
            ERR("Heap %p: prev arena %p invalid for in-use %p\n",
                subheap->heap, pPrev, pArena );
            return FALSE;
        }
        /* Check that prev arena is really the previous block */
        if ((const char *)(pPrev + 1) + (pPrev->size & ARENA_SIZE_MASK) != (const char *)pArena)
        {
            ERR("Heap %p: prev arena %p is not prev for in-use %p\n",
                subheap->heap, pPrev, pArena );
            return FALSE;
        }
    }
    return TRUE;
}

/***********************************************************************
 *           HEAP_IsRealArena  [Internal]
 * Validates a block is a valid arena.
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
static BOOL HEAP_IsRealArena( HEAP *heapPtr,   /* [in] ptr to the heap */
              DWORD flags,   /* [in] Bit flags that control access during operation */
              LPCVOID block, /* [in] Optional pointer to memory block to validate */
              BOOL quiet )   /* [in] Flag - if true, HEAP_ValidateInUseArena
                              *             does not complain    */
{
    SUBHEAP *subheap;
    BOOL ret = TRUE;

    if (!heapPtr || (heapPtr->magic != HEAP_MAGIC))
    {
        ERR("Invalid heap %p!\n", heapPtr );
        return FALSE;
    }

    flags &= HEAP_NO_SERIALIZE;
    flags |= heapPtr->flags;
    /* calling HeapLock may result in infinite recursion, so do the critsect directly */
    if (!(flags & HEAP_NO_SERIALIZE))
        RtlEnterHeapLock( &heapPtr->critSection );

    if (block)
    {
        /* Only check this single memory block */

        if (!(subheap = HEAP_FindSubHeap( heapPtr, block )) ||
            ((const char *)block < (char *)subheap + subheap->headerSize
                                   + sizeof(ARENA_INUSE)))
        {
            if (quiet == NOISY)
                ERR("Heap %p: block %p is not inside heap\n", heapPtr, block );
            else if (WARN_ON(heap))
                WARN("Heap %p: block %p is not inside heap\n", heapPtr, block );
            ret = FALSE;
        } else
            ret = HEAP_ValidateInUseArena( subheap, (const ARENA_INUSE *)block - 1, quiet );

        if (!(flags & HEAP_NO_SERIALIZE))
            RtlLeaveHeapLock( &heapPtr->critSection );
        return ret;
    }

    subheap = &heapPtr->subheap;
    while (subheap && ret)
    {
        char *ptr = (char *)subheap + subheap->headerSize;
        while (ptr < (char *)subheap + subheap->size)
        {
            if (*(DWORD *)ptr & ARENA_FLAG_FREE)
            {
                if (!HEAP_ValidateFreeArena( subheap, (ARENA_FREE *)ptr )) {
                    ret = FALSE;
                    break;
                }
                ptr += sizeof(ARENA_FREE) + (*(DWORD *)ptr & ARENA_SIZE_MASK);
            }
            else
            {
                if (!HEAP_ValidateInUseArena( subheap, (ARENA_INUSE *)ptr, NOISY )) {
                    ret = FALSE;
                    break;
                }
                ptr += sizeof(ARENA_INUSE) + (*(DWORD *)ptr & ARENA_SIZE_MASK);
            }
        }
        subheap = subheap->next;
    }

    if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveHeapLock( &heapPtr->critSection );
    return ret;
}


/***********************************************************************
 *           HeapCreate   (KERNEL32.336)
 * RETURNS
 * Handle of heap: Success
 * NULL: Failure
 *
 * @implemented
 */
HANDLE NTAPI
RtlCreateHeap(ULONG flags,
              PVOID addr,
              SIZE_T totalSize,
              SIZE_T commitSize,
              PVOID Lock,
              PRTL_HEAP_PARAMETERS Parameters)
{
   SUBHEAP *subheap;

   /* Allocate the heap block */

    if (!totalSize)
    {
        totalSize = HEAP_DEF_SIZE;
        flags |= HEAP_GROWABLE;
    }
    if (!(subheap = HEAP_CreateSubHeap( NULL, addr, flags, commitSize, totalSize, Parameters ))) return 0;

   if (RtlpGetMode() == UserMode)
   {
        /* link it into the per-process heap list */
        if (processHeap)
        {
            HEAP *heapPtr = subheap->heap;
            RtlEnterHeapLock( &processHeap->critSection );
            list_add_head( &processHeap->entry, &heapPtr->entry );
            RtlLeaveHeapLock( &processHeap->critSection );
        }
        else
        {
            processHeap = subheap->heap;  /* assume the first heap we create is the process main heap */
            list_init( &processHeap->entry );
        }
   }

   return (HANDLE)subheap;
}

/***********************************************************************
 *           HeapDestroy   (KERNEL32.337)
 * RETURNS
 * TRUE: Success
 * FALSE: Failure
 *
 * @implemented
 *
 * RETURNS
 *  Success: A NULL HANDLE, if heap is NULL or it was destroyed
 *  Failure: The Heap handle, if heap is the process heap.
 */
HANDLE NTAPI
RtlDestroyHeap(HANDLE heap) /* [in] Handle of heap */
{
   HEAP *heapPtr = HEAP_GetPtr( heap );
   SUBHEAP *subheap;

   DPRINT("%p\n", heap );
   if (!heapPtr)
      return heap;

   if (RtlpGetMode() == UserMode)
   {
      if (heap == NtCurrentPeb()->ProcessHeap)
         return heap; /* cannot delete the main process heap */

      /* remove it from the per-process list */
      RtlEnterHeapLock( &processHeap->critSection );
      list_remove( &heapPtr->entry );
      RtlLeaveHeapLock( &processHeap->critSection );
   }

    RtlDeleteHeapLock( &heapPtr->critSection );
    subheap = &heapPtr->subheap;
    while (subheap)
    {
        SUBHEAP *next = subheap->next;
        SIZE_T size = 0;
        void *addr = subheap;
        ZwFreeVirtualMemory( NtCurrentProcess(), &addr, &size, MEM_RELEASE );
        subheap = next;
    }
   return (HANDLE)NULL;
}


/***********************************************************************
 *           HeapAlloc   (KERNEL32.334)
 * RETURNS
 * Pointer to allocated memory block
 * NULL: Failure
 * 0x7d030f60--invalid flags in RtlHeapAllocate
 * @implemented
 */
PVOID NTAPI
RtlAllocateHeap(HANDLE heap,   /* [in] Handle of private heap block */
                ULONG flags,   /* [in] Heap allocation control flags */
                ULONG size)    /* [in] Number of bytes to allocate */
{
   ARENA_FREE *pArena;
   ARENA_INUSE *pInUse;
   SUBHEAP *subheap;
   HEAP *heapPtr = HEAP_GetPtr( heap );
   SIZE_T rounded_size;

   /* Validate the parameters */

   if (!heapPtr)
   {
      if (flags & HEAP_GENERATE_EXCEPTIONS)
         RtlRaiseStatus( STATUS_NO_MEMORY );
      return NULL;
   }
   //flags &= HEAP_GENERATE_EXCEPTIONS | HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY;
    flags |= heapPtr->flags;
    rounded_size = ROUND_SIZE(size);
    if (rounded_size < HEAP_MIN_DATA_SIZE) rounded_size = HEAP_MIN_DATA_SIZE;

    if (!(flags & HEAP_NO_SERIALIZE)) RtlEnterHeapLock( &heapPtr->critSection );
    /* Locate a suitable free block */

   /* Locate a suitable free block */

    if (!(pArena = HEAP_FindFreeBlock( heapPtr, rounded_size, &subheap )))
    {
        TRACE("(%p,%08lx,%08lx): returning NULL\n",
                  heap, flags, size  );
        if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveHeapLock( &heapPtr->critSection );
        if (flags & HEAP_GENERATE_EXCEPTIONS) RtlRaiseStatus( STATUS_NO_MEMORY );
        return NULL;
    }

   /* Remove the arena from the free list */

    list_remove( &pArena->entry );

    /* Build the in-use arena */

    pInUse = (ARENA_INUSE *)pArena;

    /* in-use arena is smaller than free arena,
     * so we have to add the difference to the size */
    pInUse->size  = (pInUse->size & ~ARENA_FLAG_FREE) + sizeof(ARENA_FREE) - sizeof(ARENA_INUSE);
    pInUse->magic = ARENA_INUSE_MAGIC;

    /* Save user flags */
    subheap->UserFlags = flags & HEAP_SETTABLE_USER_FLAGS;

    /* Shrink the block */

    HEAP_ShrinkBlock( subheap, pInUse, rounded_size );
    pInUse->unused_bytes = (pInUse->size & ARENA_SIZE_MASK) - size;

    if (flags & HEAP_ZERO_MEMORY)
        clear_block( pInUse + 1, pInUse->size & ARENA_SIZE_MASK );
    else
        mark_block_uninitialized( pInUse + 1, pInUse->size & ARENA_SIZE_MASK );

    if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveHeapLock( &heapPtr->critSection );

    TRACE("(%p,%08lx,%08lx): returning %p\n", heap, flags, size, pInUse + 1 );
    return (LPVOID)(pInUse + 1);
}


/***********************************************************************
 *           HeapFree   (KERNEL32.338)
 * RETURNS
 * TRUE: Success
 * FALSE: Failure
 *
 * @implemented
 */
BOOLEAN NTAPI RtlFreeHeap(
   HANDLE heap, /* [in] Handle of heap */
   ULONG flags,   /* [in] Heap freeing flags */
   PVOID ptr     /* [in] Address of memory to free */
)
{
    ARENA_INUSE *pInUse;
    SUBHEAP *subheap;
    HEAP *heapPtr;

    /* Validate the parameters */

    if (!ptr) return TRUE;  /* freeing a NULL ptr isn't an error in Win2k */

    heapPtr = HEAP_GetPtr( heap );
    if (!heapPtr)
    {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus( STATUS_INVALID_HANDLE );
        return FALSE;
    }

    flags &= HEAP_NO_SERIALIZE;
    flags |= heapPtr->flags;
    if (!(flags & HEAP_NO_SERIALIZE)) RtlEnterHeapLock( &heapPtr->critSection );
    if (!HEAP_IsRealArena( heapPtr, HEAP_NO_SERIALIZE, ptr, QUIET ))
    {
        if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveHeapLock( &heapPtr->critSection );
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus( STATUS_INVALID_PARAMETER );
        TRACE("(%p,%08lx,%p): returning FALSE\n", heap, flags, ptr );
        return FALSE;
    }

    /* Turn the block into a free block */

    pInUse  = (ARENA_INUSE *)ptr - 1;
    subheap = HEAP_FindSubHeap( heapPtr, pInUse );
    HEAP_MakeInUseBlockFree( subheap, pInUse );

    if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveHeapLock( &heapPtr->critSection );

    TRACE("(%p,%08lx,%p): returning TRUE\n", heap, flags, ptr );
    return TRUE;
}


/***********************************************************************
 *           RtlReAllocateHeap
 * PARAMS
 *   Heap   [in] Handle of heap block
 *   Flags    [in] Heap reallocation flags
 *   Ptr,    [in] Address of memory to reallocate
 *   Size     [in] Number of bytes to reallocate
 *
 * RETURNS
 * Pointer to reallocated memory block
 * NULL: Failure
 * 0x7d030f60--invalid flags in RtlHeapAllocate
 * @implemented
 */
PVOID NTAPI RtlReAllocateHeap(
   HANDLE heap,
   ULONG flags,
   PVOID ptr,
   SIZE_T size
)
{
    ARENA_INUSE *pArena;
    HEAP *heapPtr;
    SUBHEAP *subheap;
    SIZE_T oldSize, rounded_size;

    if (!ptr) return NULL;
    if (!(heapPtr = HEAP_GetPtr( heap )))
    {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus( STATUS_INVALID_HANDLE );
        return NULL;
    }

   /* Validate the parameters */

   //Flags &= HEAP_GENERATE_EXCEPTIONS | HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY |
   //         HEAP_REALLOC_IN_PLACE_ONLY;
    flags |= heapPtr->flags;
    rounded_size = ROUND_SIZE(size);
    if (rounded_size < HEAP_MIN_DATA_SIZE) rounded_size = HEAP_MIN_DATA_SIZE;

    if (!(flags & HEAP_NO_SERIALIZE)) RtlEnterHeapLock( &heapPtr->critSection );
    if (!HEAP_IsRealArena( heapPtr, HEAP_NO_SERIALIZE, ptr, QUIET ))
    {
        if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveHeapLock( &heapPtr->critSection );
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus( STATUS_INVALID_PARAMETER );
        TRACE("(%p,%08lx,%p,%08lx): returning NULL\n", heap, flags, ptr, size );
        return NULL;
    }

    pArena = (ARENA_INUSE *)ptr - 1;
    subheap = HEAP_FindSubHeap( heapPtr, pArena );
    oldSize = (pArena->size & ARENA_SIZE_MASK);
    if (rounded_size > oldSize)
    {
        char *pNext = (char *)(pArena + 1) + oldSize;
        if ((pNext < (char *)subheap + subheap->size) &&
            (*(DWORD *)pNext & ARENA_FLAG_FREE) &&
            (oldSize + (*(DWORD *)pNext & ARENA_SIZE_MASK) + sizeof(ARENA_FREE) >= rounded_size))
        {
            /* The next block is free and large enough */
            ARENA_FREE *pFree = (ARENA_FREE *)pNext;
            list_remove( &pFree->entry );
            pArena->size += (pFree->size & ARENA_SIZE_MASK) + sizeof(*pFree);
            if (!HEAP_Commit( subheap, pArena, rounded_size ))
            {
                if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveHeapLock( &heapPtr->critSection );
                if (flags & HEAP_GENERATE_EXCEPTIONS) RtlRaiseStatus( STATUS_NO_MEMORY );
                RtlSetLastWin32ErrorAndNtStatusFromNtStatus( STATUS_NO_MEMORY );
                return NULL;
            }
            HEAP_ShrinkBlock( subheap, pArena, rounded_size );
        }
        else  /* Do it the hard way */
        {
            ARENA_FREE *pNew;
            ARENA_INUSE *pInUse;
            SUBHEAP *newsubheap;

            if ((flags & HEAP_REALLOC_IN_PLACE_ONLY) ||
                !(pNew = HEAP_FindFreeBlock( heapPtr, rounded_size, &newsubheap )))
            {
                if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveHeapLock( &heapPtr->critSection );
                if (flags & HEAP_GENERATE_EXCEPTIONS) RtlRaiseStatus( STATUS_NO_MEMORY );
                RtlSetLastWin32ErrorAndNtStatusFromNtStatus( STATUS_NO_MEMORY );
                return NULL;
            }

            /* Build the in-use arena */

            list_remove( &pNew->entry );
            pInUse = (ARENA_INUSE *)pNew;
            pInUse->size = (pInUse->size & ~ARENA_FLAG_FREE)
                           + sizeof(ARENA_FREE) - sizeof(ARENA_INUSE);
            pInUse->magic = ARENA_INUSE_MAGIC;
            HEAP_ShrinkBlock( newsubheap, pInUse, rounded_size );
            mark_block_initialized( pInUse + 1, oldSize );
            memcpy( pInUse + 1, pArena + 1, oldSize );

            /* Free the previous block */

            HEAP_MakeInUseBlockFree( subheap, pArena );
            subheap = newsubheap;
            pArena  = pInUse;
        }
    }
    else HEAP_ShrinkBlock( subheap, pArena, rounded_size );  /* Shrink the block */

    pArena->unused_bytes = (pArena->size & ARENA_SIZE_MASK) - size;

    /* Clear the extra bytes if needed */

    if (rounded_size > oldSize)
    {
        if (flags & HEAP_ZERO_MEMORY)
            clear_block( (char *)(pArena + 1) + oldSize,
                         (pArena->size & ARENA_SIZE_MASK) - oldSize );
        else
            mark_block_uninitialized( (char *)(pArena + 1) + oldSize,
                                      (pArena->size & ARENA_SIZE_MASK) - oldSize );
    }

    /* Return the new arena */

    if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveHeapLock( &heapPtr->critSection );

    TRACE("(%p,%08lx,%p,%08lx): returning %p\n", heap, flags, ptr, size, pArena + 1 );
    return (LPVOID)(pArena + 1);
}


/***********************************************************************
 *           RtlCompactHeap
 *
 * @unimplemented
 */
ULONG NTAPI
RtlCompactHeap(HANDLE Heap,
		ULONG Flags)
{
   UNIMPLEMENTED;
   return 0;
}


/***********************************************************************
 *           RtlLockHeap
 * Attempts to acquire the critical section object for a specified heap.
 *
 * PARAMS
 *   Heap  [in] Handle of heap to lock for exclusive access
 *
 * RETURNS
 * TRUE: Success
 * FALSE: Failure
 *
 * @implemented
 */
BOOLEAN NTAPI
RtlLockHeap(IN HANDLE Heap)
{
   HEAP *heapPtr = HEAP_GetPtr( Heap );
   if (!heapPtr)
      return FALSE;
   RtlEnterHeapLock( &heapPtr->critSection );
   return TRUE;
}


/***********************************************************************
 *           RtlUnlockHeap
 * Releases ownership of the critical section object.
 *
 * PARAMS
 *   Heap  [in] Handle to the heap to unlock
 *
 * RETURNS
 * TRUE: Success
 * FALSE: Failure
 *
 * @implemented
 */
BOOLEAN NTAPI
RtlUnlockHeap(HANDLE Heap)
{
   HEAP *heapPtr = HEAP_GetPtr( Heap );
   if (!heapPtr)
      return FALSE;
   RtlLeaveHeapLock( &heapPtr->critSection );
   return TRUE;
}


/***********************************************************************
 *           RtlSizeHeap
 * PARAMS
 *   Heap  [in] Handle of heap
 *   Flags   [in] Heap size control flags
 *   Ptr     [in] Address of memory to return size for
 *
 * RETURNS
 * Size in bytes of allocated memory
 * 0xffffffff: Failure
 *
 * @implemented
 */
ULONG NTAPI
RtlSizeHeap(
   HANDLE heap,
   ULONG flags,
   PVOID ptr
)
{
   SIZE_T ret;
    HEAP *heapPtr = HEAP_GetPtr( heap );

    if (!heapPtr)
    {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus( STATUS_INVALID_HANDLE );
        return ~0UL;
    }
    flags &= HEAP_NO_SERIALIZE;
    flags |= heapPtr->flags;
    if (!(flags & HEAP_NO_SERIALIZE)) RtlEnterHeapLock( &heapPtr->critSection );
    if (!HEAP_IsRealArena( heapPtr, HEAP_NO_SERIALIZE, ptr, QUIET ))
    {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus( STATUS_INVALID_PARAMETER );
        ret = ~0UL;
    }
    else
    {
        ARENA_INUSE *pArena = (ARENA_INUSE *)ptr - 1;
        ret = (pArena->size & ARENA_SIZE_MASK) - pArena->unused_bytes;
    }
    if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveHeapLock( &heapPtr->critSection );

    TRACE("(%p,%08lx,%p): returning %08lx\n", heap, flags, ptr, ret );
    return ret;
}


/***********************************************************************
 *           RtlValidateHeap
 * Validates a specified heap.
 *
 * PARAMS
 *   Heap  [in] Handle to the heap
 *   Flags   [in] Bit flags that control access during operation
 *   Block  [in] Optional pointer to memory block to validate
 *
 * NOTES
 * Flags is ignored.
 *
 * RETURNS
 * TRUE: Success
 * FALSE: Failure
 *
 * @implemented
 */
BOOLEAN NTAPI RtlValidateHeap(
   HANDLE Heap,
   ULONG Flags,
   PVOID Block
)
{
   HEAP *heapPtr = HEAP_GetPtr( Heap );
   if (!heapPtr)
      return FALSE;
   return HEAP_IsRealArena( heapPtr, Flags, Block, QUIET );
}

VOID
RtlInitializeHeapManager(VOID)
{
   PPEB Peb;

   Peb = NtCurrentPeb();

   Peb->NumberOfHeaps = 0;
   Peb->MaximumNumberOfHeaps = -1; /* no limit */
   Peb->ProcessHeaps = NULL;

   //RtlInitializeHeapLock(&RtlpProcessHeapsListLock);
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlEnumProcessHeaps(PHEAP_ENUMERATION_ROUTINE HeapEnumerationRoutine,
                    PVOID lParam)
{
    DPRINT1("UNIMPLEMENTED\n");
    DPRINT1("UNIMPLEMENTED\n");
    DPRINT1("UNIMPLEMENTED\n");
    DPRINT1("UNIMPLEMENTED\n");
    DbgBreakPoint();
    return STATUS_SUCCESS;
#if 0
   NTSTATUS Status = STATUS_SUCCESS;
   HEAP** pptr;

   RtlEnterHeapLock(&RtlpProcessHeapsListLock);

   for (pptr = (HEAP**)&NtCurrentPeb()->ProcessHeaps; *pptr; pptr = &(*pptr)->next)
   {
      Status = HeapEnumerationRoutine(*pptr,lParam);
      if (!NT_SUCCESS(Status))
         break;
   }

   RtlLeaveHeapLock(&RtlpProcessHeapsListLock);

   return Status;
#endif
}


/*
 * @implemented
 */
ULONG NTAPI
RtlGetProcessHeaps(ULONG count,
                   HANDLE *heaps )
{
    ULONG total = 1;  /* main heap */
    struct list *ptr;

    RtlEnterHeapLock( &processHeap->critSection );
    LIST_FOR_EACH( ptr, &processHeap->entry ) total++;
    if (total <= count)
    {
        *heaps++ = processHeap;
        LIST_FOR_EACH( ptr, &processHeap->entry )
            *heaps++ = LIST_ENTRY( ptr, HEAP, entry );
    }
    RtlLeaveHeapLock( &processHeap->critSection );
    return total;
}


/*
 * @implemented
 */
BOOLEAN NTAPI
RtlValidateProcessHeaps(VOID)
{
    DPRINT1("UNIMPLEMENTED\n");
    DPRINT1("UNIMPLEMENTED\n");
    DPRINT1("UNIMPLEMENTED\n");
    DPRINT1("UNIMPLEMENTED\n");
    DbgBreakPoint();
    return STATUS_SUCCESS;
#if 0
   BOOLEAN Result = TRUE;
   HEAP ** pptr;

   RtlEnterHeapLock(&RtlpProcessHeapsListLock);

   for (pptr = (HEAP**)&NtCurrentPeb()->ProcessHeaps; *pptr; pptr = &(*pptr)->next)
   {
      if (!RtlValidateHeap(*pptr, 0, NULL))
      {
         Result = FALSE;
         break;
      }
   }

   RtlLeaveHeapLock (&RtlpProcessHeapsListLock);

   return Result;
#endif
}


/*
 * @unimplemented
 */
BOOLEAN NTAPI
RtlZeroHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags
    )
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
RtlSetUserValueHeap(IN PVOID HeapHandle,
                    IN ULONG Flags,
                    IN PVOID BaseAddress,
                    IN PVOID UserValue)
{
    HEAP *heapPtr = HEAP_GetPtr(HeapHandle);
    ARENA_INUSE *pInUse;
    SUBHEAP *subheap;

    /* Get the subheap */
    pInUse  = (ARENA_INUSE *)BaseAddress - 1;
    subheap = HEAP_FindSubHeap( heapPtr, pInUse );

    /* Hack */
    subheap->UserValue = UserValue;
    return TRUE;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
RtlGetUserInfoHeap(IN PVOID HeapHandle,
                   IN ULONG Flags,
                   IN PVOID BaseAddress,
                   OUT PVOID *UserValue,
                   OUT PULONG UserFlags)
{
    HEAP *heapPtr = HEAP_GetPtr(HeapHandle);
    ARENA_INUSE *pInUse;
    SUBHEAP *subheap;

    /* Get the subheap */
    pInUse  = (ARENA_INUSE *)BaseAddress - 1;
    subheap = HEAP_FindSubHeap( heapPtr, pInUse );

    /* Hack */
    DPRINT1("V/F: %lx %p\n", subheap->UserValue, subheap->UserFlags);
    if (UserValue) *UserValue = subheap->UserValue;
    if (UserFlags) *UserFlags = subheap->UserFlags;
    return TRUE;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlUsageHeap(IN HANDLE Heap,
             IN ULONG Flags,
             OUT PRTL_HEAP_USAGE Usage)
{
    /* TODO */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

PWSTR
NTAPI
RtlQueryTagHeap(IN PVOID HeapHandle,
                IN ULONG Flags,
                IN USHORT TagIndex,
                IN BOOLEAN ResetCounters,
                OUT PRTL_HEAP_TAG_INFO HeapTagInfo)
{
    /* TODO */
    UNIMPLEMENTED;
    return NULL;
}

ULONG
NTAPI
RtlExtendHeap(IN HANDLE Heap,
              IN ULONG Flags,
              IN PVOID P,
              IN ULONG Size)
{
    /* TODO */
    UNIMPLEMENTED;
    return 0;
}

ULONG
NTAPI
RtlCreateTagHeap(IN HANDLE HeapHandle,
                 IN ULONG Flags,
                 IN PWSTR TagName,
                 IN PWSTR TagSubName)
{
    /* TODO */
    UNIMPLEMENTED;
    return 0;
}

/* EOF */
