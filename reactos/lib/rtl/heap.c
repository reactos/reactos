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

#include <rtl.h>

#define NDEBUG
#include <debug.h>

#define WARN_ON(x) (1)

#ifdef NDEBUG
#define TRACE_ON(x) (0)
#else
#define TRACE_ON(x) (1)
#endif

static RTL_CRITICAL_SECTION RtlpProcessHeapsListLock;


typedef struct tagARENA_INUSE
{
   ULONG  size;                    /* Block size; must be the first field */
   USHORT threadId;                /* Allocating thread id */
   USHORT magic;                   /* Magic number */
}
ARENA_INUSE;

typedef struct tagARENA_FREE
{
   ULONG                 size;     /* Block size; must be the first field */
   USHORT                threadId; /* Freeing thread id */
   USHORT                magic;    /* Magic number */
   struct tagARENA_FREE *next;     /* Next free arena */
   struct tagARENA_FREE *prev;     /* Prev free arena */
}
ARENA_FREE;

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
static const ULONG HEAP_freeListSizes[HEAP_NB_FREE_LISTS] =
   {
      0x20, 0x80, 0x200, 0xffffffff
   };

typedef struct
{
   ULONG       size;
   ARENA_FREE  arena;
}
FREE_LIST_ENTRY;

struct tagHEAP;

typedef struct tagSUBHEAP
{
   ULONG               size;       /* Size of the whole sub-heap */
   ULONG               commitSize; /* Committed size of the sub-heap */
   ULONG               headerSize; /* Size of the heap header */
   struct tagSUBHEAP  *next;       /* Next sub-heap */
   struct tagHEAP     *heap;       /* Main heap structure */
   ULONG               magic;      /* Magic number */
}
SUBHEAP, *PSUBHEAP;

#define SUBHEAP_MAGIC    ((ULONG)('S' | ('U'<<8) | ('B'<<16) | ('H'<<24)))

typedef struct tagHEAP
{
   SUBHEAP          subheap;       /* First sub-heap */
   struct tagHEAP  *next;          /* Next heap for this process */
   FREE_LIST_ENTRY  freeList[HEAP_NB_FREE_LISTS];  /* Free lists */
   RTL_CRITICAL_SECTION critSection;   /* Critical section for serialization */
   ULONG            flags;         /* Heap flags */
   ULONG            magic;         /* Magic number */
   PRTL_HEAP_COMMIT_ROUTINE commitRoutine;
}
HEAP, *PHEAP;

#define HEAP_MAGIC       ((ULONG)('H' | ('E'<<8) | ('A'<<16) | ('P'<<24)))

#define HEAP_DEF_SIZE        0x110000   /* Default heap size = 1Mb + 64Kb */
#define HEAP_MIN_BLOCK_SIZE  (sizeof(ARENA_FREE) + 8)  /* Min. heap block size */
#define COMMIT_MASK          0xffff  /* bitmask for commit/decommit granularity */


static BOOLEAN HEAP_IsRealArena( HANDLE heap, ULONG flags, PVOID block, BOOLEAN quiet );


/***********************************************************************
 *           HEAP_Dump
 */
VOID
HEAP_Dump(PHEAP heap)
{
   int i;
   SUBHEAP *subheap;
   char *ptr;

   DPRINT( "Heap: %p\n", heap );
   DPRINT( "Next: %p  Sub-heaps: %p",
           heap->next, &heap->subheap );
   subheap = &heap->subheap;
   while (subheap->next)
   {
      DPRINT( " -> %p", subheap->next );
      subheap = subheap->next;
   }

   DPRINT( "\nFree lists:\n Block   Stat   Size    Id\n" );
   for (i = 0; i < HEAP_NB_FREE_LISTS; i++)
      DPRINT( "%p free %08lx %04x prev=%p next=%p\n",
              &heap->freeList[i].arena,
              heap->freeList[i].arena.size,
              heap->freeList[i].arena.threadId,
              heap->freeList[i].arena.prev,
              heap->freeList[i].arena.next );

   subheap = &heap->subheap;
   while (subheap)
   {
      ULONG freeSize = 0, usedSize = 0, arenaSize = subheap->headerSize;
      DPRINT( "\n\nSub-heap %p: size=%08lx committed=%08lx\n",
              subheap, subheap->size, subheap->commitSize );

      DPRINT( "\n Block   Stat   Size    Id\n" );
      ptr = (char*)subheap + subheap->headerSize;
      while (ptr < (char *)subheap + subheap->size)
      {
         if (*(PULONG)ptr & ARENA_FLAG_FREE)
         {
            ARENA_FREE *pArena = (ARENA_FREE *)ptr;
            DPRINT( "%p free %08lx %04x prev=%p next=%p\n",
                    pArena, pArena->size & ARENA_SIZE_MASK,
                    pArena->threadId, pArena->prev,
                    pArena->next);
            ptr += sizeof(*pArena) + (pArena->size & ARENA_SIZE_MASK);
            arenaSize += sizeof(ARENA_FREE);
            freeSize += pArena->size & ARENA_SIZE_MASK;
         }
         else if (*(PULONG)ptr & ARENA_FLAG_PREV_FREE)
         {
            ARENA_INUSE *pArena = (ARENA_INUSE *)ptr;
            DPRINT( "%p Used %08lx %04x back=%08lx\n",
                    pArena, pArena->size & ARENA_SIZE_MASK,
                    pArena->threadId, *((PULONG)pArena - 1));
            ptr += sizeof(*pArena) + (pArena->size & ARENA_SIZE_MASK);
            arenaSize += sizeof(ARENA_INUSE);
            usedSize += pArena->size & ARENA_SIZE_MASK;
         }
         else
         {
            ARENA_INUSE *pArena = (ARENA_INUSE *)ptr;
            DPRINT( "%p used %08lx %04x\n",
                    pArena, pArena->size & ARENA_SIZE_MASK,
                    pArena->threadId);
            ptr += sizeof(*pArena) + (pArena->size & ARENA_SIZE_MASK);
            arenaSize += sizeof(ARENA_INUSE);
            usedSize += pArena->size & ARENA_SIZE_MASK;
         }
      }
      DPRINT( "\nTotal: Size=%08lx Committed=%08lx Free=%08lx Used=%08lx Arenas=%08lx (%ld%%)\n\n",
               subheap->size, subheap->commitSize, freeSize, usedSize,
               arenaSize, (arenaSize * 100) / subheap->size );
      subheap = subheap->next;
   }
}


/***********************************************************************
 *           HEAP_GetPtr
 * RETURNS
 * Pointer to the heap
 * NULL: Failure
 */
static PHEAP
HEAP_GetPtr(HANDLE heap) /* [in] Handle to the heap */
{
   HEAP *heapPtr = (HEAP *)heap;
   if (!heapPtr || (heapPtr->magic != HEAP_MAGIC))
   {
      DPRINT("Invalid heap %08x!\n", heap );
      return NULL;
   }
   if (TRACE_ON(heap) && !HEAP_IsRealArena( heap, 0, NULL, NOISY ))
   {
      HEAP_Dump( heapPtr );
      ASSERT( FALSE );
      return NULL;
   }
   return heapPtr;
}


/***********************************************************************
 *           HEAP_InsertFreeBlock
 *
 * Insert a free block into the free list.
 */
static VOID
HEAP_InsertFreeBlock(PHEAP heap,
                     ARENA_FREE *pArena,
                     BOOLEAN last)
{
   FREE_LIST_ENTRY *pEntry = heap->freeList;
   while (pEntry->size < pArena->size)
      pEntry++;
   if (last)
   {
      /* insert at end of free list, i.e. before next free list entry */
      pEntry++;
      if (pEntry == &heap->freeList[HEAP_NB_FREE_LISTS])
      {
         pEntry = heap->freeList;
      }
      pArena->prev       = pEntry->arena.prev;
      pArena->prev->next = pArena;
      pArena->next       = &pEntry->arena;
      pEntry->arena.prev = pArena;
   }
   else
   {
      /* insert at head of free list */
      pArena->next       = pEntry->arena.next;
      pArena->next->prev = pArena;
      pArena->prev       = &pEntry->arena;
      pEntry->arena.next = pArena;
   }
   pArena->size |= ARENA_FLAG_FREE;
}


/***********************************************************************
 *           HEAP_FindSubHeap
 * Find the sub-heap containing a given address.
 *
 * RETURNS
 * Pointer: Success
 * NULL: Failure
 */
static PSUBHEAP
HEAP_FindSubHeap(HEAP *heap,  /* [in] Heap pointer */
                 PVOID ptr) /* [in] Address */
{
   PSUBHEAP sub = &heap->subheap;
   while (sub)
   {
      if (((char *)ptr >= (char *)sub) &&
            ((char *)ptr < (char *)sub + sub->size))
         return sub;
      sub = sub->next;
   }
   return NULL;
}


/***********************************************************************
 *           HEAP_Commit
 *
 * Make sure the heap storage is committed up to (not including) ptr.
 */
static inline BOOLEAN
HEAP_Commit(SUBHEAP *subheap,
            PVOID ptr,
            ULONG flags)
{
   ULONG size = (ULONG)((char *)ptr - (char *)subheap);
   NTSTATUS Status;
   PVOID address;
   ULONG commitsize;

   size = (size + COMMIT_MASK) & ~COMMIT_MASK;
   if (size > subheap->size)
      size = subheap->size;
   if (size <= subheap->commitSize)
      return TRUE;

   address = (PVOID)((char *)subheap + subheap->commitSize);
   commitsize = size - subheap->commitSize;

   if (subheap->heap->commitRoutine != NULL)
   {
      Status = subheap->heap->commitRoutine(subheap->heap, &address, &commitsize);
   }
   else
   {
      Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                       &address,
                                       0,
                                       &commitsize,
                                       MEM_COMMIT,
                                       PAGE_EXECUTE_READWRITE);
   }
   if (!NT_SUCCESS(Status))
   {
      DPRINT( "Could not commit %08lx bytes at %p for heap %p\n",
              size - subheap->commitSize,
              ((char *)subheap + subheap->commitSize),
              subheap->heap );
      return FALSE;
   }

   subheap->commitSize += commitsize;
   return TRUE;
}


#if 0
/***********************************************************************
 *           HEAP_Decommit
 *
 * If possible, decommit the heap storage from (including) 'ptr'.
 */
static inline BOOLEAN HEAP_Decommit( SUBHEAP *subheap, PVOID ptr, ULONG flags )
{
   ULONG size = (ULONG)((char *)ptr - (char *)subheap);
   PVOID address;
   ULONG decommitsize;
   NTSTATUS Status;
   /* round to next block and add one full block */
   size = ((size + COMMIT_MASK) & ~COMMIT_MASK) + COMMIT_MASK + 1;
   if (size >= subheap->commitSize)
      return TRUE;

   address = (PVOID)((char *)subheap + size);
   decommitsize = subheap->commitSize - size;

   Status = ZwFreeVirtualMemory(NtCurrentProcess(),
                                &address,
                                &decommitsize,
                                MEM_DECOMMIT);
   if (!NT_SUCCESS(Status))
   {
      DPRINT( "Could not decommit %08lx bytes at %p for heap %p\n",
              subheap->commitSize - size,
              ((char *)subheap + size),
              subheap->heap );
      return FALSE;
   }

   subheap->commitSize -= decommitsize;
   return TRUE;
}
#endif

/***********************************************************************
 *           HEAP_CreateFreeBlock
 *
 * Create a free block at a specified address. 'size' is the size of the
 * whole block, including the new arena.
 */
static VOID HEAP_CreateFreeBlock( SUBHEAP *subheap, PVOID ptr, ULONG size )
{
   ARENA_FREE *pFree;
   BOOLEAN last;

   /* Create a free arena */

   pFree = (ARENA_FREE *)ptr;
   pFree->threadId = (ULONG)NtCurrentTeb()->Cid.UniqueThread;
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
         (*(PULONG)((char *)ptr + size) & ARENA_FLAG_FREE))
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

   last = ((char *)ptr + size >= (char *)subheap + subheap->size);
   if (!last)
   {
      PULONG pNext = (PULONG)((char *)ptr + size);
      *pNext |= ARENA_FLAG_PREV_FREE;
      *(ARENA_FREE **)(pNext - 1) = pFree;
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
static VOID HEAP_MakeInUseBlockFree( SUBHEAP *subheap, ARENA_INUSE *pArena,
                                     ULONG flags)
{
   ARENA_FREE *pFree;
   ULONG size = (pArena->size & ARENA_SIZE_MASK) + sizeof(*pArena);
   ULONG dummySize = 0;

   /* Check if we can merge with previous block */

   if (pArena->size & ARENA_FLAG_PREV_FREE)
   {
      pFree = *((ARENA_FREE **)pArena - 1);
      size += (pFree->size & ARENA_SIZE_MASK) + sizeof(ARENA_FREE);
      /* Remove it from the free list */
      pFree->next->prev = pFree->prev;
      pFree->prev->next = pFree->next;
   }
   else
      pFree = (ARENA_FREE *)pArena;

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
      while (pPrev && (pPrev->next != subheap))
         pPrev = pPrev->next;
      if (pPrev)
         pPrev->next = subheap->next;
      /* Free the memory */
      subheap->magic = 0;
      ZwFreeVirtualMemory(NtCurrentProcess(),
                          (PVOID*)&subheap,
                          &dummySize,
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
static void HEAP_ShrinkBlock(SUBHEAP *subheap, ARENA_INUSE *pArena, ULONG size)
{
   if ((pArena->size & ARENA_SIZE_MASK) >= size + HEAP_MIN_BLOCK_SIZE)
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
         *(PULONG)pNext &= ~ARENA_FLAG_PREV_FREE;
   }
}

/***********************************************************************
 *           HEAP_InitSubHeap
 */
static BOOLEAN HEAP_InitSubHeap( HEAP *heap, PVOID address, ULONG flags,
                                 ULONG commitSize, ULONG totalSize,
                                 PRTL_HEAP_PARAMETERS Parameters )
{
   SUBHEAP *subheap = (SUBHEAP *)address;
   FREE_LIST_ENTRY *pEntry;
   int i;
   NTSTATUS Status;

   /* Fill the sub-heap structure */

   subheap = (PSUBHEAP)address;
   subheap->heap       = heap;
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
      if (Parameters)
         heap->commitRoutine = Parameters->CommitRoutine;
      else
         heap->commitRoutine = NULL;

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

   /* Commit memory */
   if (heap->commitRoutine)
   {
      Status = heap->commitRoutine(heap, &address, &commitSize);
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

   HEAP_CreateFreeBlock( subheap, (PUCHAR)subheap + subheap->headerSize,
                         subheap->size - subheap->headerSize );

   return TRUE;
}

/***********************************************************************
 *           HEAP_CreateSubHeap
 *
 * Create a sub-heap of the given size.
 * If heap == NULL, creates a main heap.
 */
static PSUBHEAP
HEAP_CreateSubHeap(PVOID BaseAddress,
                   HEAP *heap, ULONG flags,
                   ULONG commitSize, ULONG totalSize,
                   PRTL_HEAP_PARAMETERS Parameters )
{
   PVOID address;
   NTSTATUS Status;

   /* Round-up sizes on a 64K boundary */

   totalSize  = (totalSize + 0xffff) & 0xffff0000;
   commitSize = (commitSize + 0xffff) & 0xffff0000;
   if (!commitSize)
      commitSize = 0x10000;
   if (totalSize < commitSize)
      totalSize = commitSize;

   /* Allocate the memory block */
   address = BaseAddress;
   if (!address)
   {
      Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                       &address,
                                       0,
                                       (PULONG)&totalSize,
                                       MEM_RESERVE | MEM_COMMIT,
                                       PAGE_EXECUTE_READWRITE);
      if (!NT_SUCCESS(Status))
      {
         DPRINT("Could not VirtualAlloc %08lx bytes\n",
                totalSize );
         return NULL;
      }
   }

   /* Initialize subheap */

   if (!HEAP_InitSubHeap( heap ? heap : (HEAP *)address,
                          address, flags, commitSize, totalSize,
                          Parameters ))
   {
      if (!BaseAddress)
      {
         ULONG dummySize = 0;
         ZwFreeVirtualMemory(NtCurrentProcess(),
                             &address,
                             &dummySize,
                             MEM_RELEASE);
      }
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
static ARENA_FREE *HEAP_FindFreeBlock( HEAP *heap, ULONG size,
                                       SUBHEAP **ppSubHeap )
{
   SUBHEAP *subheap;
   ARENA_FREE *pArena;
   FREE_LIST_ENTRY *pEntry = heap->freeList;

   /* Find a suitable free list, and in it find a block large enough */

   while (pEntry->size < size)
      pEntry++;
   pArena = pEntry->arena.next;
   while (pArena != &heap->freeList[0].arena)
   {
      ULONG arena_size = (pArena->size & ARENA_SIZE_MASK) +
                         sizeof(ARENA_FREE) - sizeof(ARENA_INUSE);
      if (arena_size >= size)
      {
         subheap = HEAP_FindSubHeap( heap, pArena );
         if (!HEAP_Commit( subheap, (char *)pArena + sizeof(ARENA_INUSE)
                           + size + HEAP_MIN_BLOCK_SIZE,
                           heap->flags))
            return NULL;
         *ppSubHeap = subheap;
         return pArena;
      }

      pArena = pArena->next;
   }

   /* If no block was found, attempt to grow the heap */

   if (!(heap->flags & HEAP_GROWABLE))
   {
      DPRINT("Not enough space in heap %p for %08lx bytes\n",
             heap, size );
      return NULL;
   }
   /* make sure that we have a big enough size *committed* to fit another
    * last free arena in !
    * So just one heap struct, one first free arena which will eventually
    * get inuse, and HEAP_MIN_BLOCK_SIZE for the second free arena that
    * might get assigned all remaining free space in HEAP_ShrinkBlock() */
   size += sizeof(SUBHEAP) + sizeof(ARENA_FREE) + HEAP_MIN_BLOCK_SIZE;
   if (!(subheap = HEAP_CreateSubHeap( NULL, heap, heap->flags, size,
                                       max( HEAP_DEF_SIZE, size ), NULL )))
      return NULL;

   DPRINT("created new sub-heap %p of %08lx bytes for heap %p\n",
          subheap, size, heap );

   *ppSubHeap = subheap;
   return (ARENA_FREE *)(subheap + 1);
}


/***********************************************************************
 *           HEAP_IsValidArenaPtr
 *
 * Check that the pointer is inside the range possible for arenas.
 */
static BOOLEAN HEAP_IsValidArenaPtr( HEAP *heap, PVOID ptr )
{
   int i;
   SUBHEAP *subheap = HEAP_FindSubHeap( heap, ptr );
   if (!subheap)
      return FALSE;
   if ((char *)ptr >= (char *)subheap + subheap->headerSize)
      return TRUE;
   if (subheap != &heap->subheap)
      return FALSE;
   for (i = 0; i < HEAP_NB_FREE_LISTS; i++)
      if (ptr == (PVOID)&heap->freeList[i].arena)
         return TRUE;
   return FALSE;
}


/***********************************************************************
 *           HEAP_ValidateFreeArena
 */
static BOOLEAN HEAP_ValidateFreeArena( SUBHEAP *subheap, ARENA_FREE *pArena )
{
   char *heapEnd = (char *)subheap + subheap->size;

   /* Check magic number */
   if (pArena->magic != ARENA_FREE_MAGIC)
   {
      DPRINT("Heap %p: invalid free arena magic for %p\n",
             subheap->heap, pArena);
      return FALSE;
   }
   /* Check size flags */
   if (!(pArena->size & ARENA_FLAG_FREE) ||
         (pArena->size & ARENA_FLAG_PREV_FREE))
   {
      DPRINT("Heap %p: bad flags %lx for free arena %p\n",
             subheap->heap, pArena->size & ~ARENA_SIZE_MASK, pArena);
   }
   /* Check arena size */
   if ((char *)(pArena + 1) + (pArena->size & ARENA_SIZE_MASK) > heapEnd)
   {
      DPRINT("Heap %p: bad size %08lx for free arena %p\n",
             subheap->heap, (ULONG)pArena->size & ARENA_SIZE_MASK, pArena );
      return FALSE;
   }
   /* Check that next pointer is valid */
   if (!HEAP_IsValidArenaPtr( subheap->heap, pArena->next ))
   {
      DPRINT("Heap %p: bad next ptr %p for arena %p\n",
             subheap->heap, pArena->next, pArena);
      return FALSE;
   }
   /* Check that next arena is free */
   if (!(pArena->next->size & ARENA_FLAG_FREE) ||
         (pArena->next->magic != ARENA_FREE_MAGIC))
   {
      DPRINT("Heap %p: next arena %p invalid for %p\n",
             subheap->heap, pArena->next, pArena);
      return FALSE;
   }
   /* Check that prev pointer is valid */
   if (!HEAP_IsValidArenaPtr( subheap->heap, pArena->prev ))
   {
      DPRINT("Heap %p: bad prev ptr %p for arena %p\n",
             subheap->heap, pArena->prev, pArena);
      return FALSE;
   }
   /* Check that prev arena is free */
   if (!(pArena->prev->size & ARENA_FLAG_FREE) ||
         (pArena->prev->magic != ARENA_FREE_MAGIC))
   {
      DPRINT("Heap %p: prev arena %p invalid for %p\n",
             subheap->heap, pArena->prev, pArena);
      return FALSE;
   }
   /* Check that next block has PREV_FREE flag */
   if ((char *)(pArena + 1) + (pArena->size & ARENA_SIZE_MASK) < heapEnd)
   {
      if (!(*(PULONG)((char *)(pArena + 1) +
                       (pArena->size & ARENA_SIZE_MASK)) & ARENA_FLAG_PREV_FREE))
      {
         DPRINT("Heap %p: free arena %p next block has no PREV_FREE flag\n",
                subheap->heap, pArena);
         return FALSE;
      }
      /* Check next block back pointer */
      if (*((ARENA_FREE **)((char *)(pArena + 1) +
                            (pArena->size & ARENA_SIZE_MASK)) - 1) != pArena)
      {
         DPRINT("Heap %p: arena %p has wrong back ptr %p\n",
                subheap->heap, pArena,
                *((PULONG)((char *)(pArena+1)+ (pArena->size & ARENA_SIZE_MASK)) - 1));
         return FALSE;
      }
   }
   return TRUE;
}


/***********************************************************************
 *           HEAP_ValidateInUseArena
 */
static BOOLEAN HEAP_ValidateInUseArena( SUBHEAP *subheap, ARENA_INUSE *pArena, BOOLEAN quiet )
{
   char *heapEnd = (char *)subheap + subheap->size;

   /* Check magic number */
   if (pArena->magic != ARENA_INUSE_MAGIC)
   {
      if (quiet == NOISY)
      {
         DPRINT("Heap %p: invalid in-use arena magic for %p\n",
                subheap->heap, pArena);
         if (TRACE_ON(heap))
            HEAP_Dump( subheap->heap );
      }
      else if (WARN_ON(heap))
      {
         DPRINT("Heap %p: invalid in-use arena magic for %p\n",
                subheap->heap, pArena);
         if (TRACE_ON(heap))
            HEAP_Dump( subheap->heap );
      }
      return FALSE;
   }
   /* Check size flags */
   if (pArena->size & ARENA_FLAG_FREE)
   {
      DPRINT("Heap %p: bad flags %lx for in-use arena %p\n",
             subheap->heap, pArena->size & ~ARENA_SIZE_MASK, pArena);
      return FALSE;
   }
   /* Check arena size */
   if ((char *)(pArena + 1) + (pArena->size & ARENA_SIZE_MASK) > heapEnd)
   {
      DPRINT("Heap %p: bad size %08lx for in-use arena %p\n",
             subheap->heap, (ULONG)pArena->size & ARENA_SIZE_MASK, pArena);
      return FALSE;
   }
   /* Check next arena PREV_FREE flag */
   if (((char *)(pArena + 1) + (pArena->size & ARENA_SIZE_MASK) < heapEnd) &&
         (*(PULONG)((char *)(pArena + 1) + (pArena->size & ARENA_SIZE_MASK)) & ARENA_FLAG_PREV_FREE))
   {
      DPRINT("Heap %p: in-use arena %p next block has PREV_FREE flag\n",
             subheap->heap, pArena);
      return FALSE;
   }
   /* Check prev free arena */
   if (pArena->size & ARENA_FLAG_PREV_FREE)
   {
      ARENA_FREE *pPrev = *((ARENA_FREE **)pArena - 1);
      /* Check prev pointer */
      if (!HEAP_IsValidArenaPtr( subheap->heap, pPrev ))
      {
         DPRINT("Heap %p: bad back ptr %p for arena %p\n",
                subheap->heap, pPrev, pArena );
         return FALSE;
      }
      /* Check that prev arena is free */
      if (!(pPrev->size & ARENA_FLAG_FREE) ||
            (pPrev->magic != ARENA_FREE_MAGIC))
      {
         DPRINT("Heap %p: prev arena %p invalid for in-use %p\n",
                subheap->heap, pPrev, pArena);
         return FALSE;
      }
      /* Check that prev arena is really the previous block */
      if ((char *)(pPrev + 1) + (pPrev->size & ARENA_SIZE_MASK) != (char *)pArena)
      {
         DPRINT("Heap %p: prev arena %p is not prev for in-use %p\n",
                subheap->heap, pPrev, pArena );
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
 * Should this return BOOL32?
 *
 * RETURNS
 * !0: Success
 * 0: Failure
 */
int HEAP_IsInsideHeap(
   HANDLE heap, /* [in] Heap */
   ULONG flags,   /* [in] Flags */
   PVOID ptr    /* [in] Pointer */
)
{
   HEAP *heapPtr = HEAP_GetPtr( heap );
   SUBHEAP *subheap;
   int ret;

   /* Validate the parameters */

   if (!heapPtr)
      return 0;
   flags |= heapPtr->flags;
   if (!(flags & HEAP_NO_SERIALIZE))
      RtlEnterCriticalSection( &heapPtr->critSection );
   ret = (((subheap = HEAP_FindSubHeap( heapPtr, ptr )) != NULL) &&
          (((char *)ptr >= (char *)subheap + subheap->headerSize
            + sizeof(ARENA_INUSE))));
   if (!(flags & HEAP_NO_SERIALIZE))
      RtlLeaveCriticalSection( &heapPtr->critSection );
   return ret;
}


void DumpStackFrames ( PULONG Frame, ULONG FrameCount )
{
	ULONG i=0;

	DbgPrint("Frames: ");
	if ( !Frame )
	{
#if defined __GNUC__
		__asm__("mov %%ebp, %%ebx" : "=b" (Frame) : );
#elif defined(_MSC_VER)
		__asm mov [Frame], ebp
#endif
		Frame = (PULONG)Frame[0]; // step out of DumpStackFrames
	}
	while ( Frame != 0 && (ULONG)Frame != 0xDEADBEEF && (ULONG)Frame != 0xcdcdcdcd && (ULONG)Frame != 0xcccccccc && i++ < FrameCount )
	{
		DbgPrint("<%p>", (PVOID)Frame[1]);
		if (Frame[1] == 0xdeadbeef)
		    break;
		Frame = (PULONG)Frame[0];
		DbgPrint(" ");
	}
	DbgPrint("\n");
}

/***********************************************************************
 *           HEAP_IsRealArena  [Internal]
 * Validates a block is a valid arena.
 *
 * RETURNS
 * TRUE: Success
 * FALSE: Failure
 */
static BOOLEAN HEAP_IsRealArena(
   HANDLE heap,   /* [in] Handle to the heap */
   ULONG flags,   /* [in] Bit flags that control access during operation */
   PVOID block,   /* [in] Optional pointer to memory block to validate */
   BOOLEAN quiet  /* [in] Flag - if true, HEAP_ValidateInUseArena
                   *             does not complain    */
)
{
   SUBHEAP *subheap;
   HEAP *heapPtr = (HEAP *)(heap);
   BOOLEAN ret = TRUE;

   if (!heapPtr || (heapPtr->magic != HEAP_MAGIC))
   {
      DPRINT("Invalid heap %08x!\n", heap );
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
         {
            DPRINT("Heap %p: block %p is not inside heap\n",
                   heap, block );
			DumpStackFrames(NULL,10);
         }
         else if (WARN_ON(heap))
         {
            DPRINT1("Heap %p: block %p is not inside heap\n",
                    heap, block );
			DumpStackFrames(NULL,10);
         }
         ret = FALSE;
      }
      else
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
         if (*(PULONG)ptr & ARENA_FLAG_FREE)
         {
            if (!HEAP_ValidateFreeArena( subheap, (ARENA_FREE *)ptr ))
            {
               ret = FALSE;
               break;
            }
            ptr += sizeof(ARENA_FREE) + (*(PULONG)ptr & ARENA_SIZE_MASK);
         }
         else
         {
            if (!HEAP_ValidateInUseArena( subheap, (ARENA_INUSE *)ptr, NOISY ))
            {
               ret = FALSE;
               break;
            }
            ptr += sizeof(ARENA_INUSE) + (*(PULONG)ptr & ARENA_SIZE_MASK);
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
 * Handle of heap: Success
 * NULL: Failure
 *
 * @implemented
 */
HANDLE STDCALL
RtlCreateHeap(ULONG flags,
              PVOID BaseAddress,
              SIZE_T maxSize,
              SIZE_T initialSize,
              PVOID Lock,
              PRTL_HEAP_PARAMETERS Parameters)
{
   SUBHEAP *subheap;
   HEAP *heapPtr;

   /* Allocate the heap block */

   if (!maxSize)
   {
      maxSize = HEAP_DEF_SIZE;
      flags |= HEAP_GROWABLE;
   }
   if (!(subheap = HEAP_CreateSubHeap( BaseAddress, NULL, flags, initialSize,
                                       maxSize, Parameters )))
   {
      return 0;
   }

   if (RtlpGetMode() == UserMode)
   {
      /* link it into the per-process heap list */
      RtlEnterCriticalSection (&RtlpProcessHeapsListLock);

      heapPtr = subheap->heap;
      heapPtr->next = (HEAP*)NtCurrentPeb()->ProcessHeaps;
      NtCurrentPeb()->ProcessHeaps = (HANDLE)heapPtr;
      NtCurrentPeb()->NumberOfHeaps++;

      RtlLeaveCriticalSection (&RtlpProcessHeapsListLock);
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
HANDLE STDCALL
RtlDestroyHeap(HANDLE heap) /* [in] Handle of heap */
{
   HEAP *heapPtr = HEAP_GetPtr( heap );
   SUBHEAP *subheap;
   ULONG flags;
   HEAP **pptr;

   DPRINT("%08x\n", heap );
   if (!heapPtr)
      return heap;

   if (RtlpGetMode() == UserMode)
   {
      if (heap == NtCurrentPeb()->ProcessHeap)
         return heap; /* cannot delete the main process heap */

      /* remove it from the per-process list */
      RtlEnterCriticalSection (&RtlpProcessHeapsListLock);

      pptr = (HEAP**)&NtCurrentPeb()->ProcessHeaps;
      while (*pptr && *pptr != heapPtr) pptr = &(*pptr)->next;
      if (*pptr) *pptr = (*pptr)->next;
      NtCurrentPeb()->NumberOfHeaps--;

      RtlLeaveCriticalSection (&RtlpProcessHeapsListLock);
   }

   RtlDeleteCriticalSection( &heapPtr->critSection );
   subheap = &heapPtr->subheap;
   // We must save the flags. The first subheap is located after
   // the heap structure. If we release the first subheap,
   // we release also the heap structure.
   flags = heapPtr->flags;
   while (subheap)
   {
      SUBHEAP *next = subheap->next;
      ULONG dummySize = 0;
      ZwFreeVirtualMemory(NtCurrentProcess(),
                          (PVOID*)&subheap,
                          &dummySize,
                          MEM_RELEASE);
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
PVOID STDCALL
RtlAllocateHeap(HANDLE heap,   /* [in] Handle of private heap block */
                ULONG flags,   /* [in] Heap allocation control flags */
                ULONG size)    /* [in] Number of bytes to allocate */
{
   ARENA_FREE *pArena;
   ARENA_INUSE *pInUse;
   SUBHEAP *subheap;
   HEAP *heapPtr = HEAP_GetPtr( heap );

   /* Validate the parameters */

   if (!heapPtr)
   {
      if (flags & HEAP_GENERATE_EXCEPTIONS)
         RtlRaiseStatus( STATUS_NO_MEMORY );
      return NULL;
   }
   flags &= HEAP_GENERATE_EXCEPTIONS | HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY;
   flags |= heapPtr->flags;
   if (!(flags & HEAP_NO_SERIALIZE))
      RtlEnterCriticalSection( &heapPtr->critSection );
   size = (size + 7) & ~7;
   if (size < HEAP_MIN_BLOCK_SIZE)
      size = HEAP_MIN_BLOCK_SIZE;

   /* Locate a suitable free block */

   if (!(pArena = HEAP_FindFreeBlock( heapPtr, size, &subheap )))
   {
      DPRINT("(%08x,%08lx,%08lx): returning NULL\n",
            heap, flags, size  );
      if (!(flags & HEAP_NO_SERIALIZE))
         RtlLeaveCriticalSection( &heapPtr->critSection );
      if (flags & HEAP_GENERATE_EXCEPTIONS)
         RtlRaiseStatus( STATUS_NO_MEMORY );
      return NULL;
   }

   /* Remove the arena from the free list */

   pArena->next->prev = pArena->prev;
   pArena->prev->next = pArena->next;

   /* Build the in-use arena */

   pInUse = (ARENA_INUSE *)pArena;
   pInUse->size      = (pInUse->size & ~ARENA_FLAG_FREE)
                       + sizeof(ARENA_FREE) - sizeof(ARENA_INUSE);
   pInUse->threadId  = (ULONG)NtCurrentTeb()->Cid.UniqueThread;
   pInUse->magic     = ARENA_INUSE_MAGIC;

   /* Shrink the block */

   HEAP_ShrinkBlock( subheap, pInUse, size );

   if (flags & HEAP_ZERO_MEMORY)
      memset( pInUse + 1, 0, pInUse->size & ARENA_SIZE_MASK );
   else if (TRACE_ON(heap))
      memset( pInUse + 1, ARENA_INUSE_FILLER, pInUse->size & ARENA_SIZE_MASK );

   if (!(flags & HEAP_NO_SERIALIZE))
      RtlLeaveCriticalSection( &heapPtr->critSection );

   DPRINT("(%08x,%08lx,%08lx): returning %p\n",
         heap, flags, size, (PVOID)(pInUse + 1) );
   return (PVOID)(pInUse + 1);
}


/***********************************************************************
 *           HeapFree   (KERNEL32.338)
 * RETURNS
 * TRUE: Success
 * FALSE: Failure
 *
 * @implemented
 */
BOOLEAN STDCALL RtlFreeHeap(
   HANDLE heap, /* [in] Handle of heap */
   ULONG flags,   /* [in] Heap freeing flags */
   PVOID ptr     /* [in] Address of memory to free */
)
{
   ARENA_INUSE *pInUse;
   SUBHEAP *subheap;
   HEAP *heapPtr = HEAP_GetPtr( heap );

   /* Validate the parameters */

   if (!heapPtr)
      return FALSE;
   if (!ptr)  /* Freeing a NULL ptr is doesn't indicate an error in Win2k */
   {
      DPRINT("(%08x,%08lx,%p): asked to free NULL\n",
           heap, flags, ptr );
      return TRUE;
   }

   flags &= HEAP_NO_SERIALIZE;
   flags |= heapPtr->flags;
   if (!(flags & HEAP_NO_SERIALIZE))
      RtlEnterCriticalSection( &heapPtr->critSection );
   if (!HEAP_IsRealArena( heap, HEAP_NO_SERIALIZE, ptr, QUIET ))
   {
      if (!(flags & HEAP_NO_SERIALIZE))
         RtlLeaveCriticalSection( &heapPtr->critSection );
      DPRINT("(%08x,%08lx,%p): returning FALSE\n",
            heap, flags, ptr );
      return FALSE;
   }

   /* Turn the block into a free block */

   pInUse  = (ARENA_INUSE *)ptr - 1;
   subheap = HEAP_FindSubHeap( heapPtr, pInUse );
   HEAP_MakeInUseBlockFree( subheap, pInUse, heapPtr->flags );

   if (!(flags & HEAP_NO_SERIALIZE))
      RtlLeaveCriticalSection( &heapPtr->critSection );

   DPRINT("(%08x,%08lx,%p): returning TRUE\n",
         heap, flags, ptr );
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
PVOID STDCALL RtlReAllocateHeap(
   HANDLE Heap,
   ULONG Flags,
   PVOID Ptr,
   ULONG Size
)
{
   ARENA_INUSE *pArena;
   ULONG oldSize;
   HEAP *heapPtr;
   SUBHEAP *subheap;

   if (!Ptr)
      return FALSE;
   if (!(heapPtr = HEAP_GetPtr( Heap )))
      return FALSE;

   /* Validate the parameters */

   Flags &= HEAP_GENERATE_EXCEPTIONS | HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY |
            HEAP_REALLOC_IN_PLACE_ONLY;
   Flags |= heapPtr->flags;
   Size = (Size + 7) & ~7;
   if (Size < HEAP_MIN_BLOCK_SIZE)
      Size = HEAP_MIN_BLOCK_SIZE;

   if (!(Flags & HEAP_NO_SERIALIZE))
      RtlEnterCriticalSection( &heapPtr->critSection );
   if (!HEAP_IsRealArena( Heap, HEAP_NO_SERIALIZE, Ptr, QUIET ))
   {
      if (!(Flags & HEAP_NO_SERIALIZE))
         RtlLeaveCriticalSection( &heapPtr->critSection );
      DPRINT("(%08x,%08lx,%p,%08lx): returning NULL\n",
            Heap, Flags, Ptr, Size );
      if (Flags & HEAP_GENERATE_EXCEPTIONS)
         RtlRaiseStatus( STATUS_NO_MEMORY );
      return NULL;
   }

   /* Check if we need to grow the block */

   pArena = (ARENA_INUSE *)Ptr - 1;
   pArena->threadId = (ULONG)NtCurrentTeb()->Cid.UniqueThread;

   subheap = HEAP_FindSubHeap( heapPtr, pArena );
   oldSize = (pArena->size & ARENA_SIZE_MASK);
   if (Size > oldSize)
   {
      char *pNext = (char *)(pArena + 1) + oldSize;
      if ((pNext < (char *)subheap + subheap->size) &&
            (*(PULONG)pNext & ARENA_FLAG_FREE) &&
            (oldSize + (*(PULONG)pNext & ARENA_SIZE_MASK) + sizeof(ARENA_FREE) >= Size))
      {
         /* The next block is free and large enough */
         ARENA_FREE *pFree = (ARENA_FREE *)pNext;
         pFree->next->prev = pFree->prev;
         pFree->prev->next = pFree->next;
         pArena->size += (pFree->size & ARENA_SIZE_MASK) + sizeof(*pFree);
         if (!HEAP_Commit( subheap, (char *)pArena + sizeof(ARENA_INUSE)
                           + Size + HEAP_MIN_BLOCK_SIZE,
                           heapPtr->flags))
         {
            if (!(Flags & HEAP_NO_SERIALIZE))
               RtlLeaveCriticalSection( &heapPtr->critSection );
            if (Flags & HEAP_GENERATE_EXCEPTIONS)
               RtlRaiseStatus( STATUS_NO_MEMORY );
            return NULL;
         }
         HEAP_ShrinkBlock( subheap, pArena, Size );
      }
      else  /* Do it the hard way */
      {
         ARENA_FREE *pNew;
         ARENA_INUSE *pInUse;
         SUBHEAP *newsubheap;

         if ((Flags & HEAP_REALLOC_IN_PLACE_ONLY) ||
               !(pNew = HEAP_FindFreeBlock( heapPtr, Size, &newsubheap )))
         {
            if (!(Flags & HEAP_NO_SERIALIZE))
               RtlLeaveCriticalSection( &heapPtr->critSection );
            if (Flags & HEAP_GENERATE_EXCEPTIONS)
               RtlRaiseStatus( STATUS_NO_MEMORY );
            return NULL;
         }

         /* Build the in-use arena */

         pNew->next->prev = pNew->prev;
         pNew->prev->next = pNew->next;
         pInUse = (ARENA_INUSE *)pNew;
         pInUse->size     = (pInUse->size & ~ARENA_FLAG_FREE)
                            + sizeof(ARENA_FREE) - sizeof(ARENA_INUSE);
         pInUse->threadId = (ULONG)NtCurrentTeb()->Cid.UniqueThread;
         pInUse->magic    = ARENA_INUSE_MAGIC;
         HEAP_ShrinkBlock( newsubheap, pInUse, Size );
         memcpy( pInUse + 1, pArena + 1, oldSize );

         /* Free the previous block */

         HEAP_MakeInUseBlockFree( subheap, pArena, Flags );
         subheap = newsubheap;
         pArena  = pInUse;
      }
   }
   else
      HEAP_ShrinkBlock( subheap, pArena, Size );  /* Shrink the block */

   /* Clear the extra bytes if needed */

   if (Size > oldSize)
   {
      if (Flags & HEAP_ZERO_MEMORY)
         memset( (char *)(pArena + 1) + oldSize, 0,
                 (pArena->size & ARENA_SIZE_MASK) - oldSize );
      else if (TRACE_ON(heap))
         memset( (char *)(pArena + 1) + oldSize, ARENA_INUSE_FILLER,
                 (pArena->size & ARENA_SIZE_MASK) - oldSize );
   }

   /* Return the new arena */

   if (!(Flags & HEAP_NO_SERIALIZE))
      RtlLeaveCriticalSection( &heapPtr->critSection );

   DPRINT("(%08x,%08lx,%p,%08lx): returning %p\n",
         Heap, Flags, Ptr, Size, (PVOID)(pArena + 1) );
   return (PVOID)(pArena + 1);
}


/***********************************************************************
 *           RtlCompactHeap
 *
 * @unimplemented
 */
ULONG STDCALL
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
BOOLEAN STDCALL
RtlLockHeap(IN HANDLE Heap)
{
   HEAP *heapPtr = HEAP_GetPtr( Heap );
   if (!heapPtr)
      return FALSE;
   RtlEnterCriticalSection( &heapPtr->critSection );
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
BOOLEAN STDCALL
RtlUnlockHeap(HANDLE Heap)
{
   HEAP *heapPtr = HEAP_GetPtr( Heap );
   if (!heapPtr)
      return FALSE;
   RtlLeaveCriticalSection( &heapPtr->critSection );
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
ULONG STDCALL
RtlSizeHeap(
   HANDLE Heap,
   ULONG Flags,
   PVOID Ptr
)
{
   ULONG ret;
   HEAP *heapPtr = HEAP_GetPtr( Heap );

   if (!heapPtr)
      return 0;
   Flags &= HEAP_NO_SERIALIZE;
   Flags |= heapPtr->flags;
   if (!(Flags & HEAP_NO_SERIALIZE))
      RtlEnterCriticalSection( &heapPtr->critSection );
   if (!HEAP_IsRealArena( Heap, HEAP_NO_SERIALIZE, Ptr, QUIET ))
   {
      ret = 0xffffffff;
   }
   else
   {
      ARENA_INUSE *pArena = (ARENA_INUSE *)Ptr - 1;
      ret = pArena->size & ARENA_SIZE_MASK;
   }
   if (!(Flags & HEAP_NO_SERIALIZE))
      RtlLeaveCriticalSection( &heapPtr->critSection );

   DPRINT("(%08x,%08lx,%p): returning %08lx\n",
         Heap, Flags, Ptr, ret );
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
BOOLEAN STDCALL RtlValidateHeap(
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
 * TRUE: Success
 * FALSE: Failure
 */
#if 0
BOOLEAN STDCALL HeapWalk(
   HANDLE heap,               /* [in]  Handle to heap to enumerate */
   LPPROCESS_HEAP_ENTRY entry /* [out] Pointer to structure of enumeration info */
)
{
   HEAP *heapPtr = HEAP_GetPtr(heap);
   SUBHEAP *sub, *currentheap = NULL;
   BOOLEAN ret = FALSE;
   char *ptr;
   int region_index = 0;

   if (!heapPtr || !entry)
   {
      return FALSE;
   }

   if (!(heapPtr->flags & HEAP_NO_SERIALIZE))
      RtlEnterCriticalSection( &heapPtr->critSection );

   /* set ptr to the next arena to be examined */

   if (!entry->lpData) /* first call (init) ? */
   {
      DPRINT("begin walking of heap 0x%08x.\n", heap);
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
         DPRINT("no matching subheap found, shouldn't happen !\n");
         goto HW_end;
      }

      ptr += entry->cbData; /* point to next arena */
      if (ptr > (char *)currentheap + currentheap->size - 1)
      {   /* proceed with next subheap */
         if (!(currentheap = currentheap->next))
         {  /* successfully finished */
            DPRINT("end reached.\n");
            goto HW_end;
         }
         ptr = (char*)currentheap + currentheap->headerSize;
      }
   }

   entry->wFlags = 0;
   if (*(PULONG)ptr & ARENA_FLAG_FREE)
   {
      ARENA_FREE *pArena = (ARENA_FREE *)ptr;

      /*DPRINT("free, magic: %04x\n", pArena->magic);*/

      entry->lpData = pArena + 1;
      entry->cbData = pArena->size & ARENA_SIZE_MASK;
      entry->cbOverhead = sizeof(ARENA_FREE);
      entry->wFlags = PROCESS_HEAP_UNCOMMITTED_RANGE;
   }
   else
   {
      ARENA_INUSE *pArena = (ARENA_INUSE *)ptr;

      /*DPRINT("busy, magic: %04x\n", pArena->magic);*/

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
   if (!(heapPtr->flags & HEAP_NO_SERIALIZE))
      RtlLeaveCriticalSection( &heapPtr->critSection );

   return ret;
}
#endif


VOID
RtlInitializeHeapManager(VOID)
{
   PPEB Peb;

   Peb = NtCurrentPeb();

   Peb->NumberOfHeaps = 0;
   Peb->MaximumNumberOfHeaps = -1; /* no limit */
   Peb->ProcessHeaps = NULL;

   RtlInitializeCriticalSection(&RtlpProcessHeapsListLock);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlEnumProcessHeaps(PHEAP_ENUMERATION_ROUTINE HeapEnumerationRoutine,
                    PVOID lParam)
{
   NTSTATUS Status = STATUS_SUCCESS;
   HEAP** pptr;

   RtlEnterCriticalSection(&RtlpProcessHeapsListLock);

   for (pptr = (HEAP**)&NtCurrentPeb()->ProcessHeaps; *pptr; pptr = &(*pptr)->next)
   {
      Status = HeapEnumerationRoutine(*pptr,lParam);
      if (!NT_SUCCESS(Status))
         break;
   }

   RtlLeaveCriticalSection(&RtlpProcessHeapsListLock);

   return Status;
}


/*
 * @implemented
 */
ULONG STDCALL
RtlGetProcessHeaps(ULONG HeapCount,
                   HANDLE *HeapArray)
{
   ULONG Result = 0;
   HEAP ** pptr;

   RtlEnterCriticalSection(&RtlpProcessHeapsListLock);

   Result = NtCurrentPeb()->NumberOfHeaps;

   if (NtCurrentPeb()->NumberOfHeaps <= HeapCount)
   {
      int i = 0;
      for (pptr = (HEAP**)&NtCurrentPeb()->ProcessHeaps; *pptr; pptr = &(*pptr)->next)
      {
         HeapArray[i++] = *pptr;
      }
   }

   RtlLeaveCriticalSection (&RtlpProcessHeapsListLock);

   return Result;
}


/*
 * @implemented
 */
BOOLEAN STDCALL
RtlValidateProcessHeaps(VOID)
{
   BOOLEAN Result = TRUE;
   HEAP ** pptr;

   RtlEnterCriticalSection(&RtlpProcessHeapsListLock);

   for (pptr = (HEAP**)&NtCurrentPeb()->ProcessHeaps; *pptr; pptr = &(*pptr)->next)
   {
      if (!RtlValidateHeap(*pptr, 0, NULL))
      {
         Result = FALSE;
         break;
      }
   }

   RtlLeaveCriticalSection (&RtlpProcessHeapsListLock);

   return Result;
}


/*
 * @unimplemented
 */
BOOLEAN STDCALL
RtlZeroHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags
    )
{
    UNIMPLEMENTED;
    return FALSE;
}

/* EOF */
