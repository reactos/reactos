/*
 * Win32 heap functions
 *
 * Copyright 1996 Alexandre Julliard
 * Copyright 1998 Ulrich Weigand
 */


/* Note: the heap data structures are based on what Pietrek describes in his
 * book 'Windows 95 System Programming Secrets'. The layout is not exactly
 * the same, but could be easily adapted if it turns out some programs
 * require it.
 */

#include <string.h>
#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <ntos/heap.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <ntdll/ntdll.h>

#define DPRINTF DPRINT
#define ERR DPRINT
#define SetLastError(x)
#define WARN DPRINT
#define TRACE DPRINT
#define WARN_ON(x) (1)

#undef assert
#ifdef NDEBUG
#define TRACE_ON(x) (0)
#define assert(x)
#else
#define TRACE_ON(x) (1)
#define assert(x)
#endif


static CRITICAL_SECTION RtlpProcessHeapsListLock;


typedef struct tagARENA_INUSE
{
    DWORD  size;                    /* Block size; must be the first field */
    WORD   threadId;                /* Allocating thread id */
    WORD   magic;                   /* Magic number */
    void  *callerEIP;               /* EIP of caller upon allocation */
} ARENA_INUSE;

typedef struct tagARENA_FREE
{
    DWORD                 size;     /* Block size; must be the first field */
    WORD                  threadId; /* Freeing thread id */
    WORD                  magic;    /* Magic number */
    struct tagARENA_FREE *next;     /* Next free arena */
    struct tagARENA_FREE *prev;     /* Prev free arena */
} ARENA_FREE;

#define ARENA_FLAG_FREE        0x00000001  /* flags OR'ed with arena size */
#define ARENA_FLAG_PREV_FREE   0x00000002
#define ARENA_SIZE_MASK        0xfffffffc
#define ARENA_INUSE_MAGIC      0x4842      /* Value for arena 'magic' field */
#define ARENA_FREE_MAGIC       0x4846      /* Value for arena 'magic' field */

#define ARENA_INUSE_FILLER     0x55
#define ARENA_FREE_FILLER      0xaa

#define QUIET                  1           /* Suppress messages  */
#define NOISY                  0           /* Report all errors  */

#define HEAP_NB_FREE_LISTS   4   /* Number of free lists */

/* Max size of the blocks on the free lists */
static const DWORD HEAP_freeListSizes[HEAP_NB_FREE_LISTS] =
{
    0x20, 0x80, 0x200, 0xffffffff
};

typedef struct
{
    DWORD       size;
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
    WORD                selector;   /* Selector for HEAP_WINE_SEGPTR heaps */
} SUBHEAP;

#define SUBHEAP_MAGIC    ((DWORD)('S' | ('U'<<8) | ('B'<<16) | ('H'<<24)))

typedef struct tagHEAP
{
    SUBHEAP          subheap;       /* First sub-heap */
    struct tagHEAP  *next;          /* Next heap for this process */
    FREE_LIST_ENTRY  freeList[HEAP_NB_FREE_LISTS];  /* Free lists */
    CRITICAL_SECTION critSection;   /* Critical section for serialization */
    DWORD            flags;         /* Heap flags */
    DWORD            magic;         /* Magic number */
    void            *private;       /* Private pointer for the user of the heap */
} HEAP;

#define HEAP_MAGIC       ((DWORD)('H' | ('E'<<8) | ('A'<<16) | ('P'<<24)))

#define HEAP_DEF_SIZE        0x110000   /* Default heap size = 1Mb + 64Kb */
#define HEAP_MIN_BLOCK_SIZE  (8+sizeof(ARENA_FREE))  /* Min. heap block size */
#define COMMIT_MASK          0xffff  /* bitmask for commit/decommit granularity */


static BOOL HEAP_IsRealArena( HANDLE heap, DWORD flags, LPCVOID block, BOOL quiet );

#ifdef __GNUC__
#define GET_EIP()    (__builtin_return_address(0))
#define SET_EIP(ptr) ((ARENA_INUSE*)(ptr) - 1)->callerEIP = GET_EIP()
#else
#define GET_EIP()    0
#define SET_EIP(ptr) /* nothing */
#endif  /* __GNUC__ */


/***********************************************************************
 *           HEAP_Dump
 */
void HEAP_Dump( HEAP *heap )
{
    int i;
    SUBHEAP *subheap;
    char *ptr;

    DPRINTF( "Heap: %08lx\n", (DWORD)heap );
    DPRINTF( "Next: %08lx  Sub-heaps: %08lx",
	  (DWORD)heap->next, (DWORD)&heap->subheap );
    subheap = &heap->subheap;
    while (subheap->next)
    {
        DPRINTF( " -> %08lx", (DWORD)subheap->next );
        subheap = subheap->next;
    }

    DPRINTF( "\nFree lists:\n Block   Stat   Size    Id\n" );
    for (i = 0; i < HEAP_NB_FREE_LISTS; i++)
        DPRINTF( "%08lx free %08lx %04x prev=%08lx next=%08lx\n",
	      (DWORD)&heap->freeList[i].arena, heap->freeList[i].arena.size,
	      heap->freeList[i].arena.threadId,
	      (DWORD)heap->freeList[i].arena.prev,
	      (DWORD)heap->freeList[i].arena.next );

    subheap = &heap->subheap;
    while (subheap)
    {
        DWORD freeSize = 0, usedSize = 0, arenaSize = subheap->headerSize;
        DPRINTF( "\n\nSub-heap %08lx: size=%08lx committed=%08lx\n",
	      (DWORD)subheap, subheap->size, subheap->commitSize );
	
        DPRINTF( "\n Block   Stat   Size    Id\n" );
        ptr = (char*)subheap + subheap->headerSize;
        while (ptr < (char *)subheap + subheap->size)
        {
            if (*(DWORD *)ptr & ARENA_FLAG_FREE)
            {
                ARENA_FREE *pArena = (ARENA_FREE *)ptr;
                DPRINTF( "%08lx free %08lx %04x prev=%08lx next=%08lx\n",
		      (DWORD)pArena, pArena->size & ARENA_SIZE_MASK,
		      pArena->threadId, (DWORD)pArena->prev,
		      (DWORD)pArena->next);
                ptr += sizeof(*pArena) + (pArena->size & ARENA_SIZE_MASK);
                arenaSize += sizeof(ARENA_FREE);
                freeSize += pArena->size & ARENA_SIZE_MASK;
            }
            else if (*(DWORD *)ptr & ARENA_FLAG_PREV_FREE)
            {
                ARENA_INUSE *pArena = (ARENA_INUSE *)ptr;
                DPRINTF( "%08lx Used %08lx %04x back=%08lx EIP=%p\n",
		      (DWORD)pArena, pArena->size & ARENA_SIZE_MASK,
		      pArena->threadId, *((DWORD *)pArena - 1),
		      pArena->callerEIP );
                ptr += sizeof(*pArena) + (pArena->size & ARENA_SIZE_MASK);
                arenaSize += sizeof(ARENA_INUSE);
                usedSize += pArena->size & ARENA_SIZE_MASK;
            }
            else
            {
                ARENA_INUSE *pArena = (ARENA_INUSE *)ptr;
                DPRINTF( "%08lx used %08lx %04x EIP=%p\n",
		      (DWORD)pArena, pArena->size & ARENA_SIZE_MASK,
		      pArena->threadId, pArena->callerEIP );
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
        ERR("Invalid heap %08x!\n", heap );
        return NULL;
    }
    if (TRACE_ON(heap) && !HEAP_IsRealArena( heap, 0, NULL, NOISY ))
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
static void HEAP_InsertFreeBlock( HEAP *heap, ARENA_FREE *pArena )
{
    FREE_LIST_ENTRY *pEntry = heap->freeList;
    while (pEntry->size < pArena->size) pEntry++;
    pArena->size      |= ARENA_FLAG_FREE;
    pArena->next       = pEntry->arena.next;
    pArena->next->prev = pArena;
    pArena->prev       = &pEntry->arena;
    pEntry->arena.next = pArena;
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
                HEAP *heap, /* [in] Heap pointer */
                LPCVOID ptr /* [in] Address */
) {
    SUBHEAP *sub = &heap->subheap;
    while (sub)
    {
        if (((char *)ptr >= (char *)sub) &&
            ((char *)ptr < (char *)sub + sub->size)) return sub;
        sub = sub->next;
    }
    return NULL;
}


/***********************************************************************
 *           HEAP_Commit
 *
 * Make sure the heap storage is committed up to (not including) ptr.
 */
static inline BOOL HEAP_Commit( SUBHEAP *subheap, void *ptr )
{
    DWORD size = (DWORD)((char *)ptr - (char *)subheap);
    NTSTATUS Status;
    PVOID address;
    ULONG commitsize;
   
    size = (size + COMMIT_MASK) & ~COMMIT_MASK;
    if (size > subheap->size) size = subheap->size;
    if (size <= subheap->commitSize) return TRUE;
   
    address = (PVOID)((char *)subheap + subheap->commitSize);
    commitsize = size - subheap->commitSize;

    Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
				     &address,
				     0,
				     &commitsize,
				     MEM_COMMIT,
				     PAGE_EXECUTE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        WARN("Could not commit %08lx bytes at %08lx for heap %08lx\n",
                 size - subheap->commitSize,
                 (DWORD)((char *)subheap + subheap->commitSize),
                 (DWORD)subheap->heap );
        return FALSE;
    }
    subheap->commitSize = size;
    return TRUE;
}


/***********************************************************************
 *           HEAP_Decommit
 *
 * If possible, decommit the heap storage from (including) 'ptr'.
 */
static inline BOOL HEAP_Decommit( SUBHEAP *subheap, void *ptr )
{
    DWORD size = (DWORD)((char *)ptr - (char *)subheap);
    PVOID address;
    ULONG decommitsize;
    NTSTATUS Status;
    /* round to next block and add one full block */
    size = ((size + COMMIT_MASK) & ~COMMIT_MASK) + COMMIT_MASK + 1;
    if (size >= subheap->commitSize) return TRUE;
   
    address = (PVOID)((char *)subheap + size);
    decommitsize = subheap->commitSize - size;

    Status = ZwFreeVirtualMemory(NtCurrentProcess(),
				 &address,
				 &decommitsize,
				 MEM_DECOMMIT);
    if (!NT_SUCCESS(Status));
    {
        WARN("Could not decommit %08lx bytes at %08lx for heap %08lx\n",
                 subheap->commitSize - size,
                 (DWORD)((char *)subheap + size),
                 (DWORD)subheap->heap );
        return FALSE;
    }
    subheap->commitSize = size;
    return TRUE;
}


/***********************************************************************
 *           HEAP_CreateFreeBlock
 *
 * Create a free block at a specified address. 'size' is the size of the
 * whole block, including the new arena.
 */
static void HEAP_CreateFreeBlock( SUBHEAP *subheap, void *ptr, DWORD size )
{
    ARENA_FREE *pFree;

    /* Create a free arena */

    pFree = (ARENA_FREE *)ptr;
    pFree->threadId = (DWORD)NtCurrentTeb()->Cid.UniqueThread;
    pFree->magic = ARENA_FREE_MAGIC;

    /* If debugging, erase the freed block content */

    if (TRACE_ON(heap))
    {
        char *pEnd = (char *)ptr + size;
        if (pEnd > (char *)subheap + subheap->commitSize)
            pEnd = (char *)subheap + subheap->commitSize;
        if (pEnd > (char *)(pFree + 1))
            memset( pFree + 1, ARENA_FREE_FILLER, pEnd - (char *)(pFree + 1) );
    }

    /* Check if next block is free also */

    if (((char *)ptr + size < (char *)subheap + subheap->size) &&
        (*(DWORD *)((char *)ptr + size) & ARENA_FLAG_FREE))
    {
        /* Remove the next arena from the free list */
        ARENA_FREE *pNext = (ARENA_FREE *)((char *)ptr + size);
        pNext->next->prev = pNext->prev;
        pNext->prev->next = pNext->next;
        size += (pNext->size & ARENA_SIZE_MASK) + sizeof(*pNext);
        if (TRACE_ON(heap))
            memset( pNext, ARENA_FREE_FILLER, sizeof(ARENA_FREE) );
    }

    /* Set the next block PREV_FREE flag and pointer */

    if ((char *)ptr + size < (char *)subheap + subheap->size)
    {
        DWORD *pNext = (DWORD *)((char *)ptr + size);
        *pNext |= ARENA_FLAG_PREV_FREE;
        *(ARENA_FREE **)(pNext - 1) = pFree;
    }

    /* Last, insert the new block into the free list */

    pFree->size = size - sizeof(*pFree);
    HEAP_InsertFreeBlock( subheap->heap, pFree );
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
    DWORD size = (pArena->size & ARENA_SIZE_MASK) + sizeof(*pArena);

    /* Check if we can merge with previous block */

    if (pArena->size & ARENA_FLAG_PREV_FREE)
    {
        pFree = *((ARENA_FREE **)pArena - 1);
        size += (pFree->size & ARENA_SIZE_MASK) + sizeof(ARENA_FREE);
        /* Remove it from the free list */
        pFree->next->prev = pFree->prev;
        pFree->prev->next = pFree->next;
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
        SUBHEAP *pPrev = &subheap->heap->subheap;
        /* Remove the free block from the list */
        pFree->next->prev = pFree->prev;
        pFree->prev->next = pFree->next;
        /* Remove the subheap from the list */
        while (pPrev && (pPrev->next != subheap)) pPrev = pPrev->next;
        if (pPrev) pPrev->next = subheap->next;
        /* Free the memory */
        subheap->magic = 0;
        ZwFreeVirtualMemory(NtCurrentProcess(),
			    (PVOID*)&subheap,
			    0,
			    MEM_RELEASE);
        return;
    }
    
    /* Decommit the end of the heap */
}


/***********************************************************************
 *           HEAP_ShrinkBlock
 *
 * Shrink an in-use block.
 */
static void HEAP_ShrinkBlock(SUBHEAP *subheap, ARENA_INUSE *pArena, DWORD size)
{
    if ((pArena->size & ARENA_SIZE_MASK) >= size + HEAP_MIN_BLOCK_SIZE)
    {
        HEAP_CreateFreeBlock( subheap, (char *)(pArena + 1) + size,
                              (pArena->size & ARENA_SIZE_MASK) - size );
        pArena->size = (pArena->size & ~ARENA_SIZE_MASK) | size;
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
                                DWORD commitSize, DWORD totalSize )
{
    SUBHEAP *subheap = (SUBHEAP *)address;
    WORD selector = 0;
    FREE_LIST_ENTRY *pEntry;
    int i;
    NTSTATUS Status;
   
    /* Commit memory */

    Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
				     &address,
				     0,
				     (PULONG)&commitSize,
				     MEM_COMMIT,
				     PAGE_EXECUTE_READWRITE);
   if (!NT_SUCCESS(Status))
    {
        WARN("Could not commit %08lx bytes for sub-heap %08lx\n",
                   commitSize, (DWORD)address );
        return FALSE;
    }

    /* Fill the sub-heap structure */

    subheap->heap       = heap;
    subheap->selector   = selector;
    subheap->size       = totalSize;
    subheap->commitSize = commitSize;
    subheap->magic      = SUBHEAP_MAGIC;

    if ( subheap != (SUBHEAP *)heap )
    {
        /* If this is a secondary subheap, insert it into list */

        subheap->headerSize = sizeof(SUBHEAP);
        subheap->next       = heap->subheap.next;
        heap->subheap.next  = subheap;
    }
    else
    {
        /* If this is a primary subheap, initialize main heap */

        subheap->headerSize = sizeof(HEAP);
        subheap->next       = NULL;
        heap->next          = NULL;
        heap->flags         = flags;
        heap->magic         = HEAP_MAGIC;

        /* Build the free lists */

        for (i = 0, pEntry = heap->freeList; i < HEAP_NB_FREE_LISTS; i++, pEntry++)
        {
            pEntry->size           = HEAP_freeListSizes[i];
            pEntry->arena.size     = 0 | ARENA_FLAG_FREE;
            pEntry->arena.next     = i < HEAP_NB_FREE_LISTS-1 ?
                         &heap->freeList[i+1].arena : &heap->freeList[0].arena;
            pEntry->arena.prev     = i ? &heap->freeList[i-1].arena : 
                                   &heap->freeList[HEAP_NB_FREE_LISTS-1].arena;
            pEntry->arena.threadId = 0;
            pEntry->arena.magic    = ARENA_FREE_MAGIC;
        }

        /* Initialize critical section */

        RtlInitializeCriticalSection( &heap->critSection );
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
static SUBHEAP *HEAP_CreateSubHeap(PVOID BaseAddress,  
				   HEAP *heap, DWORD flags, 
                                   DWORD commitSize, DWORD totalSize )
{
    LPVOID address;
    NTSTATUS Status;
   
    /* Round-up sizes on a 64K boundary */

    totalSize  = (totalSize + 0xffff) & 0xffff0000;
    commitSize = (commitSize + 0xffff) & 0xffff0000;
    if (!commitSize) commitSize = 0x10000;
    if (totalSize < commitSize) totalSize = commitSize;

    /* Allocate the memory block */

    address = BaseAddress;
    Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
				     &address,
				     0,
				     (PULONG)&totalSize,
				     MEM_RESERVE,
				     PAGE_EXECUTE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        WARN("Could not VirtualAlloc %08lx bytes\n",
                 totalSize );
        return NULL;
    }

    /* Initialize subheap */

    if (!HEAP_InitSubHeap( heap? heap : (HEAP *)address, 
                           address, flags, commitSize, totalSize ))
    {
        ZwFreeVirtualMemory(NtCurrentProcess(),
			    address,
			    0,
			    MEM_RELEASE);
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
static ARENA_FREE *HEAP_FindFreeBlock( HEAP *heap, DWORD size,
                                       SUBHEAP **ppSubHeap )
{
    SUBHEAP *subheap;
    ARENA_FREE *pArena;
    FREE_LIST_ENTRY *pEntry = heap->freeList;

    /* Find a suitable free list, and in it find a block large enough */

    while (pEntry->size < size) pEntry++;
    pArena = pEntry->arena.next;
    while (pArena != &heap->freeList[0].arena)
    {
        if (pArena->size > size)
        {
            subheap = HEAP_FindSubHeap( heap, pArena );
            if (!HEAP_Commit( subheap, (char *)pArena + sizeof(ARENA_INUSE)
                                               + size + HEAP_MIN_BLOCK_SIZE))
                return NULL;
            *ppSubHeap = subheap;
            return pArena;
        }

        pArena = pArena->next;
    }

    /* If no block was found, attempt to grow the heap */

    if (!(heap->flags & HEAP_GROWABLE))
    {
        WARN("Not enough space in heap %08lx for %08lx bytes\n",
                 (DWORD)heap, size );
        return NULL;
    }
    size += sizeof(SUBHEAP) + sizeof(ARENA_FREE);
    if (!(subheap = HEAP_CreateSubHeap( NULL, heap, heap->flags, size,
                                        max( HEAP_DEF_SIZE, size ) )))
        return NULL;

    TRACE("created new sub-heap %08lx of %08lx bytes for heap %08lx\n",
                (DWORD)subheap, size, (DWORD)heap );

    *ppSubHeap = subheap;
    return (ARENA_FREE *)(subheap + 1);
}


/***********************************************************************
 *           HEAP_IsValidArenaPtr
 *
 * Check that the pointer is inside the range possible for arenas.
 */
static BOOL HEAP_IsValidArenaPtr( HEAP *heap, void *ptr )
{
    int i;
    SUBHEAP *subheap = HEAP_FindSubHeap( heap, ptr );
    if (!subheap) return FALSE;
    if ((char *)ptr >= (char *)subheap + subheap->headerSize) return TRUE;
    if (subheap != &heap->subheap) return FALSE;
    for (i = 0; i < HEAP_NB_FREE_LISTS; i++)
        if (ptr == (void *)&heap->freeList[i].arena) return TRUE;
    return FALSE;
}


/***********************************************************************
 *           HEAP_ValidateFreeArena
 */
static BOOL HEAP_ValidateFreeArena( SUBHEAP *subheap, ARENA_FREE *pArena )
{
    char *heapEnd = (char *)subheap + subheap->size;

    /* Check magic number */
    if (pArena->magic != ARENA_FREE_MAGIC)
    {
        ERR("Heap %08lx: invalid free arena magic for %08lx\n",
                 (DWORD)subheap->heap, (DWORD)pArena );
        return FALSE;
    }
    /* Check size flags */
    if (!(pArena->size & ARENA_FLAG_FREE) ||
        (pArena->size & ARENA_FLAG_PREV_FREE))
    {
        ERR("Heap %08lx: bad flags %lx for free arena %08lx\n",
                 (DWORD)subheap->heap, pArena->size & ~ARENA_SIZE_MASK, (DWORD)pArena );
    }
    /* Check arena size */
    if ((char *)(pArena + 1) + (pArena->size & ARENA_SIZE_MASK) > heapEnd)
    {
        ERR("Heap %08lx: bad size %08lx for free arena %08lx\n",
                 (DWORD)subheap->heap, (DWORD)pArena->size & ARENA_SIZE_MASK, (DWORD)pArena );
        return FALSE;
    }
    /* Check that next pointer is valid */
    if (!HEAP_IsValidArenaPtr( subheap->heap, pArena->next ))
    {
        ERR("Heap %08lx: bad next ptr %08lx for arena %08lx\n",
                 (DWORD)subheap->heap, (DWORD)pArena->next, (DWORD)pArena );
        return FALSE;
    }
    /* Check that next arena is free */
    if (!(pArena->next->size & ARENA_FLAG_FREE) ||
        (pArena->next->magic != ARENA_FREE_MAGIC))
    { 
        ERR("Heap %08lx: next arena %08lx invalid for %08lx\n", 
                 (DWORD)subheap->heap, (DWORD)pArena->next, (DWORD)pArena );
        return FALSE;
    }
    /* Check that prev pointer is valid */
    if (!HEAP_IsValidArenaPtr( subheap->heap, pArena->prev ))
    {
        ERR("Heap %08lx: bad prev ptr %08lx for arena %08lx\n",
                 (DWORD)subheap->heap, (DWORD)pArena->prev, (DWORD)pArena );
        return FALSE;
    }
    /* Check that prev arena is free */
    if (!(pArena->prev->size & ARENA_FLAG_FREE) ||
        (pArena->prev->magic != ARENA_FREE_MAGIC))
    { 
        ERR("Heap %08lx: prev arena %08lx invalid for %08lx\n", 
                 (DWORD)subheap->heap, (DWORD)pArena->prev, (DWORD)pArena );
        return FALSE;
    }
    /* Check that next block has PREV_FREE flag */
    if ((char *)(pArena + 1) + (pArena->size & ARENA_SIZE_MASK) < heapEnd)
    {
        if (!(*(DWORD *)((char *)(pArena + 1) +
            (pArena->size & ARENA_SIZE_MASK)) & ARENA_FLAG_PREV_FREE))
        {
            ERR("Heap %08lx: free arena %08lx next block has no PREV_FREE flag\n",
                     (DWORD)subheap->heap, (DWORD)pArena );
            return FALSE;
        }
        /* Check next block back pointer */
        if (*((ARENA_FREE **)((char *)(pArena + 1) +
            (pArena->size & ARENA_SIZE_MASK)) - 1) != pArena)
        {
            ERR("Heap %08lx: arena %08lx has wrong back ptr %08lx\n",
                     (DWORD)subheap->heap, (DWORD)pArena,
                     *((DWORD *)((char *)(pArena+1)+ (pArena->size & ARENA_SIZE_MASK)) - 1));
            return FALSE;
        }
    }
    return TRUE;
}


/***********************************************************************
 *           HEAP_ValidateInUseArena
 */
static BOOL HEAP_ValidateInUseArena( SUBHEAP *subheap, ARENA_INUSE *pArena, BOOL quiet )
{
    char *heapEnd = (char *)subheap + subheap->size;

    /* Check magic number */
    if (pArena->magic != ARENA_INUSE_MAGIC)
    {
        if (quiet == NOISY) {
        ERR("Heap %08lx: invalid in-use arena magic for %08lx\n",
                 (DWORD)subheap->heap, (DWORD)pArena );
            if (TRACE_ON(heap))
               HEAP_Dump( subheap->heap );
        }  else if (WARN_ON(heap)) {
            WARN("Heap %08lx: invalid in-use arena magic for %08lx\n",
                 (DWORD)subheap->heap, (DWORD)pArena );
            if (TRACE_ON(heap))
               HEAP_Dump( subheap->heap );
        }
        return FALSE;
    }
    /* Check size flags */
    if (pArena->size & ARENA_FLAG_FREE) 
    {
        ERR("Heap %08lx: bad flags %lx for in-use arena %08lx\n",
                 (DWORD)subheap->heap, pArena->size & ~ARENA_SIZE_MASK, (DWORD)pArena );
    }
    /* Check arena size */
    if ((char *)(pArena + 1) + (pArena->size & ARENA_SIZE_MASK) > heapEnd)
    {
        ERR("Heap %08lx: bad size %08lx for in-use arena %08lx\n",
                 (DWORD)subheap->heap, (DWORD)pArena->size & ARENA_SIZE_MASK, (DWORD)pArena );
        return FALSE;
    }
    /* Check next arena PREV_FREE flag */
    if (((char *)(pArena + 1) + (pArena->size & ARENA_SIZE_MASK) < heapEnd) &&
        (*(DWORD *)((char *)(pArena + 1) + (pArena->size & ARENA_SIZE_MASK)) & ARENA_FLAG_PREV_FREE))
    {
        ERR("Heap %08lx: in-use arena %08lx next block has PREV_FREE flag\n",
                 (DWORD)subheap->heap, (DWORD)pArena );
        return FALSE;
    }
    /* Check prev free arena */
    if (pArena->size & ARENA_FLAG_PREV_FREE)
    {
        ARENA_FREE *pPrev = *((ARENA_FREE **)pArena - 1);
        /* Check prev pointer */
        if (!HEAP_IsValidArenaPtr( subheap->heap, pPrev ))
        {
            ERR("Heap %08lx: bad back ptr %08lx for arena %08lx\n",
                    (DWORD)subheap->heap, (DWORD)pPrev, (DWORD)pArena );
            return FALSE;
        }
        /* Check that prev arena is free */
        if (!(pPrev->size & ARENA_FLAG_FREE) ||
            (pPrev->magic != ARENA_FREE_MAGIC))
        { 
            ERR("Heap %08lx: prev arena %08lx invalid for in-use %08lx\n", 
                     (DWORD)subheap->heap, (DWORD)pPrev, (DWORD)pArena );
            return FALSE;
        }
        /* Check that prev arena is really the previous block */
        if ((char *)(pPrev + 1) + (pPrev->size & ARENA_SIZE_MASK) != (char *)pArena)
        {
            ERR("Heap %08lx: prev arena %08lx is not prev for in-use %08lx\n",
                     (DWORD)subheap->heap, (DWORD)pPrev, (DWORD)pArena );
            return FALSE;
        }
    }
    return TRUE;
}


/***********************************************************************
 *           HEAP_IsInsideHeap
 * Checks whether the pointer points to a block inside a given heap.
 *
 * NOTES
 *	Should this return BOOL32?
 *
 * RETURNS
 *	!0: Success
 *	0: Failure
 */
int HEAP_IsInsideHeap(
    HANDLE heap, /* [in] Heap */
    DWORD flags,   /* [in] Flags */
    LPCVOID ptr    /* [in] Pointer */
) {
    HEAP *heapPtr = HEAP_GetPtr( heap );
    SUBHEAP *subheap;
    int ret;

    /* Validate the parameters */

    if (!heapPtr) return 0;
    flags |= heapPtr->flags;
    if (!(flags & HEAP_NO_SERIALIZE)) RtlEnterCriticalSection( &heapPtr->critSection );
    ret = (((subheap = HEAP_FindSubHeap( heapPtr, ptr )) != NULL) &&
           (((char *)ptr >= (char *)subheap + subheap->headerSize
                              + sizeof(ARENA_INUSE))));
    if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveCriticalSection( &heapPtr->critSection );
    return ret;
}




/***********************************************************************
 *           HEAP_IsRealArena  [Internal]
 * Validates a block is a valid arena.
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
static BOOL HEAP_IsRealArena(
              HANDLE heap,   /* [in] Handle to the heap */
              DWORD flags,   /* [in] Bit flags that control access during operation */
              LPCVOID block, /* [in] Optional pointer to memory block to validate */
              BOOL quiet     /* [in] Flag - if true, HEAP_ValidateInUseArena
                              *             does not complain    */
) {
    SUBHEAP *subheap;
    HEAP *heapPtr = (HEAP *)(heap);
    BOOL ret = TRUE;

    if (!heapPtr || (heapPtr->magic != HEAP_MAGIC))
    {
        ERR("Invalid heap %08x!\n", heap );
        return FALSE;
    }

    flags &= HEAP_NO_SERIALIZE;
    flags |= heapPtr->flags;
    /* calling HeapLock may result in infinite recursion, so do the critsect directly */
    if (!(flags & HEAP_NO_SERIALIZE))
        RtlEnterCriticalSection( &heapPtr->critSection );

    if (block)
    {
        /* Only check this single memory block */

        /* The following code is really HEAP_IsInsideHeap   *
         * with serialization already done.                 */
        if (!(subheap = HEAP_FindSubHeap( heapPtr, block )) ||
            ((char *)block < (char *)subheap + subheap->headerSize
                              + sizeof(ARENA_INUSE)))
        {
            if (quiet == NOISY) 
                ERR("Heap %08lx: block %08lx is not inside heap\n",
                     (DWORD)heap, (DWORD)block );
            else if (WARN_ON(heap)) 
                WARN("Heap %08lx: block %08lx is not inside heap\n",
                     (DWORD)heap, (DWORD)block );
            ret = FALSE;
        } else
            ret = HEAP_ValidateInUseArena( subheap, (ARENA_INUSE *)block - 1, quiet );

        if (!(flags & HEAP_NO_SERIALIZE))
            RtlLeaveCriticalSection( &heapPtr->critSection );
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

    if (!(flags & HEAP_NO_SERIALIZE))
	RtlLeaveCriticalSection( &heapPtr->critSection );
    return ret;
}


/***********************************************************************
 *           HeapCreate   (KERNEL32.336)
 * RETURNS
 *	Handle of heap: Success
 *	NULL: Failure
 */
HANDLE STDCALL
RtlCreateHeap(ULONG flags,
	      PVOID BaseAddress,
	      ULONG initialSize,
	      ULONG maxSize,
	      PVOID Unknown,
	      PRTL_HEAP_DEFINITION Definition)
{
    SUBHEAP *subheap;
    ULONG i;
   
    /* Allocate the heap block */

    if (!maxSize)
    {
        maxSize = HEAP_DEF_SIZE;
        flags |= HEAP_GROWABLE;
    }
    if (!(subheap = HEAP_CreateSubHeap( BaseAddress, NULL, flags, initialSize, maxSize )))
    {
        return 0;
    }

   RtlEnterCriticalSection (&RtlpProcessHeapsListLock);
   for (i = 0; i < NtCurrentPeb ()->NumberOfHeaps; i++)
     {
	if (NtCurrentPeb ()->ProcessHeaps[i] == NULL)
	  {
	     NtCurrentPeb()->ProcessHeaps[i] = (PVOID)subheap;
	     break;
	  }
     }
   RtlLeaveCriticalSection (&RtlpProcessHeapsListLock);

    return (HANDLE)subheap;
}

/***********************************************************************
 *           HeapDestroy   (KERNEL32.337)
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL STDCALL RtlDestroyHeap( HANDLE heap /* [in] Handle of heap */ )
{
    HEAP *heapPtr = HEAP_GetPtr( heap );
    SUBHEAP *subheap;
    ULONG i;
   
    TRACE("%08x\n", heap );
    if (!heapPtr) return FALSE;

   RtlEnterCriticalSection (&RtlpProcessHeapsListLock);
   for (i = 0; i < NtCurrentPeb ()->NumberOfHeaps; i++)
     {
	if (NtCurrentPeb ()->ProcessHeaps[i] == heap)
	  {
	     NtCurrentPeb()->ProcessHeaps[i] = NULL;
	     break;
	  }
     }
   RtlLeaveCriticalSection (&RtlpProcessHeapsListLock);
   
    RtlDeleteCriticalSection( &heapPtr->critSection );
    subheap = &heapPtr->subheap;
    while (subheap)
    {
        SUBHEAP *next = subheap->next;

        ZwFreeVirtualMemory(NtCurrentProcess(),
			    (PVOID*)&subheap,
			    0,
			    MEM_RELEASE);

        subheap = next;
    }
    return TRUE;
}


/***********************************************************************
 *           HeapAlloc   (KERNEL32.334)
 * RETURNS
 *	Pointer to allocated memory block
 *	NULL: Failure
 */
PVOID STDCALL RtlAllocateHeap(
              HANDLE heap, /* [in] Handle of private heap block */
              ULONG flags,   /* [in] Heap allocation control flags */
              ULONG size     /* [in] Number of bytes to allocate */
) {
    ARENA_FREE *pArena;
    ARENA_INUSE *pInUse;
    SUBHEAP *subheap;
    HEAP *heapPtr = HEAP_GetPtr( heap );

    /* Validate the parameters */

    if (!heapPtr) return NULL;
    flags &= HEAP_GENERATE_EXCEPTIONS | HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY;
    flags |= heapPtr->flags;
    if (!(flags & HEAP_NO_SERIALIZE)) RtlEnterCriticalSection( &heapPtr->critSection );
    size = (size + 3) & ~3;
    if (size < HEAP_MIN_BLOCK_SIZE) size = HEAP_MIN_BLOCK_SIZE;

    /* Locate a suitable free block */

    if (!(pArena = HEAP_FindFreeBlock( heapPtr, size, &subheap )))
    {
        TRACE("(%08x,%08lx,%08lx): returning NULL\n",
                  heap, flags, size  );
        if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveCriticalSection( &heapPtr->critSection );
        return NULL;
    }

    /* Remove the arena from the free list */

    pArena->next->prev = pArena->prev;
    pArena->prev->next = pArena->next;

    /* Build the in-use arena */

    pInUse = (ARENA_INUSE *)pArena;
    pInUse->size      = (pInUse->size & ~ARENA_FLAG_FREE)
                        + sizeof(ARENA_FREE) - sizeof(ARENA_INUSE);
    pInUse->callerEIP = GET_EIP();
    pInUse->threadId  = (DWORD)NtCurrentTeb()->Cid.UniqueThread;
    pInUse->magic     = ARENA_INUSE_MAGIC;

    /* Shrink the block */

    HEAP_ShrinkBlock( subheap, pInUse, size );

    if (flags & HEAP_ZERO_MEMORY)
        memset( pInUse + 1, 0, pInUse->size & ARENA_SIZE_MASK );
    else if (TRACE_ON(heap))
        memset( pInUse + 1, ARENA_INUSE_FILLER, pInUse->size & ARENA_SIZE_MASK );

    if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveCriticalSection( &heapPtr->critSection );

    TRACE("(%08x,%08lx,%08lx): returning %08lx\n",
                  heap, flags, size, (DWORD)(pInUse + 1) );
    return (LPVOID)(pInUse + 1);
}


/***********************************************************************
 *           HeapFree   (KERNEL32.338)
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOLEAN STDCALL RtlFreeHeap(
              HANDLE heap, /* [in] Handle of heap */
              ULONG flags,   /* [in] Heap freeing flags */
              PVOID ptr     /* [in] Address of memory to free */
) {
    ARENA_INUSE *pInUse;
    SUBHEAP *subheap;
    HEAP *heapPtr = HEAP_GetPtr( heap );

    /* Validate the parameters */

    if (!heapPtr) return FALSE;
    if (!ptr)  /* Freeing a NULL ptr is doesn't indicate an error in Win2k */
    {
	WARN("(%08x,%08lx,%08lx): asked to free NULL\n",
                   heap, flags, (DWORD)ptr );
	return TRUE;
    }

    flags &= HEAP_NO_SERIALIZE;
    flags |= heapPtr->flags;
    if (!(flags & HEAP_NO_SERIALIZE)) RtlEnterCriticalSection( &heapPtr->critSection );
    if (!HEAP_IsRealArena( heap, HEAP_NO_SERIALIZE, ptr, QUIET ))
    {
        if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveCriticalSection( &heapPtr->critSection );
        TRACE("(%08x,%08lx,%08lx): returning FALSE\n",
                      heap, flags, (DWORD)ptr );
        return FALSE;
    }

    /* Turn the block into a free block */

    pInUse  = (ARENA_INUSE *)ptr - 1;
    subheap = HEAP_FindSubHeap( heapPtr, pInUse );
    HEAP_MakeInUseBlockFree( subheap, pInUse );

    if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveCriticalSection( &heapPtr->critSection );

    TRACE("(%08x,%08lx,%08lx): returning TRUE\n",
                  heap, flags, (DWORD)ptr );
    return TRUE;
}


/***********************************************************************
 *           HeapReAlloc   (KERNEL32.340)
 * RETURNS
 *	Pointer to reallocated memory block
 *	NULL: Failure
 */
LPVOID STDCALL RtlReAllocateHeap(
              HANDLE heap, /* [in] Handle of heap block */
              DWORD flags,   /* [in] Heap reallocation flags */
              LPVOID ptr,    /* [in] Address of memory to reallocate */
              DWORD size     /* [in] Number of bytes to reallocate */
) {
    ARENA_INUSE *pArena;
    DWORD oldSize;
    HEAP *heapPtr;
    SUBHEAP *subheap;

    if (!ptr) return RtlAllocateHeap( heap, flags, size );  /* FIXME: correct? */
    if (!(heapPtr = HEAP_GetPtr( heap ))) return FALSE;

    /* Validate the parameters */

    flags &= HEAP_GENERATE_EXCEPTIONS | HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY |
             HEAP_REALLOC_IN_PLACE_ONLY;
    flags |= heapPtr->flags;
    size = (size + 3) & ~3;
    if (size < HEAP_MIN_BLOCK_SIZE) size = HEAP_MIN_BLOCK_SIZE;

    if (!(flags & HEAP_NO_SERIALIZE)) RtlEnterCriticalSection( &heapPtr->critSection );
    if (!HEAP_IsRealArena( heap, HEAP_NO_SERIALIZE, ptr, QUIET ))
    {
        if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveCriticalSection( &heapPtr->critSection );
        TRACE("(%08x,%08lx,%08lx,%08lx): returning NULL\n",
                      heap, flags, (DWORD)ptr, size );
        return NULL;
    }

    /* Check if we need to grow the block */

    pArena = (ARENA_INUSE *)ptr - 1;
    pArena->threadId = (DWORD)NtCurrentTeb()->Cid.UniqueThread;

    subheap = HEAP_FindSubHeap( heapPtr, pArena );
    oldSize = (pArena->size & ARENA_SIZE_MASK);
    if (size > oldSize)
    {
        char *pNext = (char *)(pArena + 1) + oldSize;
        if ((pNext < (char *)subheap + subheap->size) &&
            (*(DWORD *)pNext & ARENA_FLAG_FREE) &&
            (oldSize + (*(DWORD *)pNext & ARENA_SIZE_MASK) + sizeof(ARENA_FREE) >= size))
        {
            /* The next block is free and large enough */
            ARENA_FREE *pFree = (ARENA_FREE *)pNext;
            pFree->next->prev = pFree->prev;
            pFree->prev->next = pFree->next;
            pArena->size += (pFree->size & ARENA_SIZE_MASK) + sizeof(*pFree);
            if (!HEAP_Commit( subheap, (char *)pArena + sizeof(ARENA_INUSE)
                                               + size + HEAP_MIN_BLOCK_SIZE))
            {
                if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveCriticalSection( &heapPtr->critSection );
                return NULL;
            }
            HEAP_ShrinkBlock( subheap, pArena, size );
        }
        else  /* Do it the hard way */
        {
            ARENA_FREE *pNew;
            ARENA_INUSE *pInUse;
            SUBHEAP *newsubheap;

            if ((flags & HEAP_REALLOC_IN_PLACE_ONLY) ||
                !(pNew = HEAP_FindFreeBlock( heapPtr, size, &newsubheap )))
            {
                if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveCriticalSection( &heapPtr->critSection );
                return NULL;
            }

            /* Build the in-use arena */

            pNew->next->prev = pNew->prev;
            pNew->prev->next = pNew->next;
            pInUse = (ARENA_INUSE *)pNew;
            pInUse->size     = (pInUse->size & ~ARENA_FLAG_FREE)
                               + sizeof(ARENA_FREE) - sizeof(ARENA_INUSE);
	    pInUse->threadId = (DWORD)NtCurrentTeb()->Cid.UniqueThread;
            pInUse->magic    = ARENA_INUSE_MAGIC;
            HEAP_ShrinkBlock( newsubheap, pInUse, size );
            memcpy( pInUse + 1, pArena + 1, oldSize );

            /* Free the previous block */

            HEAP_MakeInUseBlockFree( subheap, pArena );
            subheap = newsubheap;
            pArena  = pInUse;
        }
    }
    else HEAP_ShrinkBlock( subheap, pArena, size );  /* Shrink the block */

    /* Clear the extra bytes if needed */

    if (size > oldSize)
    {
        if (flags & HEAP_ZERO_MEMORY)
            memset( (char *)(pArena + 1) + oldSize, 0,
                    (pArena->size & ARENA_SIZE_MASK) - oldSize );
        else if (TRACE_ON(heap))
            memset( (char *)(pArena + 1) + oldSize, ARENA_INUSE_FILLER,
                    (pArena->size & ARENA_SIZE_MASK) - oldSize );
    }

    /* Return the new arena */

    pArena->callerEIP = GET_EIP();
    if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveCriticalSection( &heapPtr->critSection );

    TRACE("(%08x,%08lx,%08lx,%08lx): returning %08lx\n",
                  heap, flags, (DWORD)ptr, size, (DWORD)(pArena + 1) );
    return (LPVOID)(pArena + 1);
}


/***********************************************************************
 *           HeapCompact   (KERNEL32.335)
 */
DWORD STDCALL RtlCompactHeap( HANDLE heap, DWORD flags )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/***********************************************************************
 *           HeapLock   (KERNEL32.339)
 * Attempts to acquire the critical section object for a specified heap.
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL STDCALL RtlLockHeap(
              HANDLE heap /* [in] Handle of heap to lock for exclusive access */
) {
    HEAP *heapPtr = HEAP_GetPtr( heap );
    if (!heapPtr) return FALSE;
    RtlEnterCriticalSection( &heapPtr->critSection );
    return TRUE;
}


/***********************************************************************
 *           HeapUnlock   (KERNEL32.342)
 * Releases ownership of the critical section object.
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL STDCALL RtlUnlockHeap(
              HANDLE heap /* [in] Handle to the heap to unlock */
) {
    HEAP *heapPtr = HEAP_GetPtr( heap );
    if (!heapPtr) return FALSE;
    RtlLeaveCriticalSection( &heapPtr->critSection );
    return TRUE;
}


/***********************************************************************
 *           HeapSize   (KERNEL32.341)
 * RETURNS
 *	Size in bytes of allocated memory
 *	0xffffffff: Failure
 */
DWORD STDCALL RtlSizeHeap(
             HANDLE heap, /* [in] Handle of heap */
             DWORD flags,   /* [in] Heap size control flags */
             LPVOID ptr     /* [in] Address of memory to return size for */
) {
    DWORD ret;
    HEAP *heapPtr = HEAP_GetPtr( heap );

    if (!heapPtr) return FALSE;
    flags &= HEAP_NO_SERIALIZE;
    flags |= heapPtr->flags;
    if (!(flags & HEAP_NO_SERIALIZE)) RtlEnterCriticalSection( &heapPtr->critSection );
    if (!HEAP_IsRealArena( heap, HEAP_NO_SERIALIZE, ptr, QUIET ))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        ret = 0xffffffff;
    }
    else
    {
        ARENA_INUSE *pArena = (ARENA_INUSE *)ptr - 1;
        ret = pArena->size & ARENA_SIZE_MASK;
    }
    if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveCriticalSection( &heapPtr->critSection );

    TRACE("(%08x,%08lx,%08lx): returning %08lx\n",
                  heap, flags, (DWORD)ptr, ret );
    return ret;
}


/***********************************************************************
 *           HeapValidate   (KERNEL32.343)
 * Validates a specified heap.
 *
 * NOTES
 *	Flags is ignored.
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL STDCALL RtlValidateHeap(
              HANDLE heap, /* [in] Handle to the heap */
              DWORD flags,   /* [in] Bit flags that control access during operation */
              PVOID block  /* [in] Optional pointer to memory block to validate */
) {

    return HEAP_IsRealArena( heap, flags, block, QUIET );
}


/***********************************************************************
 *           HeapWalk   (KERNEL32.344)
 * Enumerates the memory blocks in a specified heap.
 * See HEAP_Dump() for info on heap structure.
 *
 * TODO
 *   - handling of PROCESS_HEAP_ENTRY_MOVEABLE and
 *     PROCESS_HEAP_ENTRY_DDESHARE (needs heap.c support)
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
#if 0
BOOL STDCALL HeapWalk(
              HANDLE heap,               /* [in]  Handle to heap to enumerate */
              LPPROCESS_HEAP_ENTRY entry /* [out] Pointer to structure of enumeration info */
) {
    HEAP *heapPtr = HEAP_GetPtr(heap);
    SUBHEAP *sub, *currentheap = NULL;
    BOOL ret = FALSE;
    char *ptr;
    int region_index = 0;

    if (!heapPtr || !entry)
    {
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }

    if (!(heapPtr->flags & HEAP_NO_SERIALIZE)) RtlEnterCriticalSection( &heapPtr->critSection );

    /* set ptr to the next arena to be examined */

    if (!entry->lpData) /* first call (init) ? */
    {
	TRACE("begin walking of heap 0x%08x.\n", heap);
	/*HEAP_Dump(heapPtr);*/
	currentheap = &heapPtr->subheap;
	ptr = (char*)currentheap + currentheap->headerSize;
    }
    else
    {
	ptr = entry->lpData;
	sub = &heapPtr->subheap;
	while (sub)
	{
	    if (((char *)ptr >= (char *)sub) &&
		((char *)ptr < (char *)sub + sub->size))
	    {
		currentheap = sub;
		break;
	    }
	    sub = sub->next;
	    region_index++;
	}
	if (currentheap == NULL)
	{
	    ERR("no matching subheap found, shouldn't happen !\n");
	    SetLastError(ERROR_NO_MORE_ITEMS);
	    goto HW_end;
	}

	ptr += entry->cbData; /* point to next arena */
	if (ptr > (char *)currentheap + currentheap->size - 1)
	{   /* proceed with next subheap */
	    if (!(currentheap = currentheap->next))
	    {  /* successfully finished */
		TRACE("end reached.\n");
		SetLastError(ERROR_NO_MORE_ITEMS);
		goto HW_end;
	    }
	    ptr = (char*)currentheap + currentheap->headerSize;
	}
    }

    entry->wFlags = 0;
    if (*(DWORD *)ptr & ARENA_FLAG_FREE)
    {
	ARENA_FREE *pArena = (ARENA_FREE *)ptr;

	/*TRACE("free, magic: %04x\n", pArena->magic);*/

	entry->lpData = pArena + 1;
	entry->cbData = pArena->size & ARENA_SIZE_MASK;
	entry->cbOverhead = sizeof(ARENA_FREE);
	entry->wFlags = PROCESS_HEAP_UNCOMMITTED_RANGE;
    }
    else
    {
	ARENA_INUSE *pArena = (ARENA_INUSE *)ptr;

	/*TRACE("busy, magic: %04x\n", pArena->magic);*/
	
	entry->lpData = pArena + 1;
	entry->cbData = pArena->size & ARENA_SIZE_MASK;
	entry->cbOverhead = sizeof(ARENA_INUSE);
	entry->wFlags = PROCESS_HEAP_ENTRY_BUSY;
	/* FIXME: can't handle PROCESS_HEAP_ENTRY_MOVEABLE
	and PROCESS_HEAP_ENTRY_DDESHARE yet */
    }

    entry->iRegionIndex = region_index;

    /* first element of heap ? */
    if (ptr == (char *)(currentheap + currentheap->headerSize))
    {
	entry->wFlags |= PROCESS_HEAP_REGION;
	entry->Foo.Region.dwCommittedSize = currentheap->commitSize;
	entry->Foo.Region.dwUnCommittedSize =
		currentheap->size - currentheap->commitSize;
	entry->Foo.Region.lpFirstBlock = /* first valid block */
		currentheap + currentheap->headerSize;
	entry->Foo.Region.lpLastBlock  = /* first invalid block */
		currentheap + currentheap->size;
    }
    ret = TRUE;

HW_end:
    if (!(heapPtr->flags & HEAP_NO_SERIALIZE)) RtlLeaveCriticalSection( &heapPtr->critSection );

    return ret;
}


/***********************************************************************
 *           HEAP_CreateSystemHeap
 *
 * Create the system heap.
 */
BOOL HEAP_CreateSystemHeap(void)
{
    SYSTEM_HEAP_DESCR *descr;
    HANDLE heap;
    HEAP *heapPtr;
    int created;

    HANDLE map = CreateFileMappingA( INVALID_HANDLE_VALUE, NULL, SEC_COMMIT | PAGE_READWRITE,
                                     0, HEAP_DEF_SIZE, "__SystemHeap" );
    if (!map) return FALSE;
    created = (GetLastError() != ERROR_ALREADY_EXISTS);

    if (!(heapPtr = MapViewOfFileEx( map, FILE_MAP_ALL_ACCESS, 0, 0, 0, SYSTEM_HEAP_BASE )))
    {
        /* pre-defined address not available, use any one */
        fprintf( stderr, "Warning: system heap base address %p not available\n",
                 SYSTEM_HEAP_BASE );
        if (!(heapPtr = MapViewOfFile( map, FILE_MAP_ALL_ACCESS, 0, 0, 0 )))
        {
            CloseHandle( map );
            return FALSE;
        }
    }
    heap = (HANDLE)heapPtr;

    if (created)  /* newly created heap */
    {
        HEAP_InitSubHeap( heapPtr, heapPtr, HEAP_WINE_SHARED, 0, HEAP_DEF_SIZE );
        HeapLock( heap );
        descr = heapPtr->private = HeapAlloc( heap, HEAP_ZERO_MEMORY, sizeof(*descr) );
        assert( descr );
    }
    else
    {
        /* wait for the heap to be initialized */
        while (!heapPtr->private) Sleep(1);
        HeapLock( heap );
        /* remap it to the right address if necessary */
        if (heapPtr->subheap.heap != heapPtr)
        {
            void *base = heapPtr->subheap.heap;
            HeapUnlock( heap );
            UnmapViewOfFile( heapPtr );
            if (!(heapPtr = MapViewOfFileEx( map, FILE_MAP_ALL_ACCESS, 0, 0, 0, base )))
            {
                fprintf( stderr, "Couldn't map system heap at the correct address (%p)\n", base );
                CloseHandle( map );
                return FALSE;
            }
            heap = (HANDLE)heapPtr;
            HeapLock( heap );
        }
        descr = heapPtr->private;
        assert( descr );
    }
    SystemHeap = heap;
    SystemHeapDescr = descr;
    HeapUnlock( heap );
    CloseHandle( map );
    return TRUE;
}
#endif

HANDLE STDCALL
RtlGetProcessHeap(VOID)
{
   DPRINT("RtlGetProcessHeap()\n");
   return (HANDLE)NtCurrentPeb()->ProcessHeap;
}

VOID
RtlInitializeHeapManager(VOID)
{
   PPEB Peb;
   
   Peb = NtCurrentPeb();
   
   Peb->NumberOfHeaps = 0;
   Peb->MaximumNumberOfHeaps = (PAGESIZE - sizeof(PPEB)) / sizeof(HANDLE);
   Peb->ProcessHeaps = (PVOID)Peb + sizeof(PEB);
   
   RtlInitializeCriticalSection(&RtlpProcessHeapsListLock);
}


NTSTATUS STDCALL
RtlEnumProcessHeaps(DWORD STDCALL(*func)(void*,LONG),
		    LONG lParam)
{
   NTSTATUS Status = STATUS_SUCCESS;
   ULONG i;

   RtlEnterCriticalSection(&RtlpProcessHeapsListLock);

   for (i = 0; i < NtCurrentPeb()->NumberOfHeaps; i++)
     {
	Status = func(NtCurrentPeb()->ProcessHeaps[i],lParam);
	if (!NT_SUCCESS(Status))
	  break;
     }

   RtlLeaveCriticalSection(&RtlpProcessHeapsListLock);

   return Status;
}


ULONG STDCALL
RtlGetProcessHeaps(ULONG HeapCount,
		   HANDLE *HeapArray)
{
   ULONG Result = 0;

   RtlEnterCriticalSection(&RtlpProcessHeapsListLock);

   if (NtCurrentPeb()->NumberOfHeaps <= HeapCount)
     {
	Result = NtCurrentPeb()->NumberOfHeaps;
	memmove(HeapArray,
		NtCurrentPeb()->ProcessHeaps,
		Result * sizeof(HANDLE));
     }

   RtlLeaveCriticalSection (&RtlpProcessHeapsListLock);

   return Result;
}


BOOLEAN STDCALL
RtlValidateProcessHeaps(VOID)
{
   HANDLE Heaps[128];
   BOOLEAN Result = TRUE;
   ULONG HeapCount;
   ULONG i;

   HeapCount = RtlGetProcessHeaps(128, Heaps);
   for (i = 0; i < HeapCount; i++)
     {
	if (!RtlValidateHeap(Heaps[i], 0, NULL))
	  Result = FALSE;
     }

   return Result;
}
