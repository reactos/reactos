/* $Id: npool.c,v 1.64 2002/11/10 18:17:42 chorns Exp $
 *
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         ntoskrnl/mm/npool.c
 * PURPOSE:      Implements the kernel memory pool
 * PROGRAMMER:   David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *               27/05/98: Created
 *               10/06/98: Bug fixes by Iwan Fatahi (i_fatahi@hotmail.com)
 *                         in take_block (if current bigger than required)
 *                         in remove_from_used_list 
 *                         in ExFreePool
 *               23/08/98: Fixes from Robert Bergkvist (fragdance@hotmail.com)
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/ntoskrnl.h>
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>

/* Enable strict checking of the nonpaged pool on every allocation */
//#define ENABLE_VALIDATE_POOL

/* Enable tracking of statistics about the tagged blocks in the pool */
#define TAG_STATISTICS_TRACKING

/* 
 * Put each block in its own range of pages and position the block at the
 * end of the range so any accesses beyond the end of block are to invalid
 * memory locations. 
 */
//#define WHOLE_PAGE_ALLOCATIONS

#ifdef ENABLE_VALIDATE_POOL
#define VALIDATE_POOL validate_kernel_pool()
#else
#define VALIDATE_POOL
#endif

#if 0
#define POOL_TRACE(args...) do { DbgPrint(args); } while(0);
#else
#define POOL_TRACE(args...)
#endif

/* TYPES *******************************************************************/

#define BLOCK_HDR_USED_MAGIC (0xdeadbeef)
#define BLOCK_HDR_FREE_MAGIC (0xceadbeef)

/*
 * fields present at the start of a block (this is for internal use only)
 */
typedef struct _BLOCK_HDR
{
  ULONG Magic;
  ULONG Size;
  LIST_ENTRY ListEntry;
  ULONG Tag;
  PVOID Caller;
  struct _BLOCK_HDR* tag_next;
  BOOLEAN Dumped;
} BLOCK_HDR;

PVOID STDCALL 
ExAllocateWholePageBlock(ULONG Size);
VOID STDCALL
ExFreeWholePageBlock(PVOID Addr);

/* GLOBALS *****************************************************************/

/*
 * Memory managment initalized symbol for the base of the pool
 */
static unsigned int kernel_pool_base = 0;

/*
 * Head of the list of free blocks
 */
static LIST_ENTRY FreeBlockListHead;

/*
 * Head of the list of in use block
 */
static LIST_ENTRY UsedBlockListHead;

#ifndef WHOLE_PAGE_ALLOCATIONS
/*
 * Count of free blocks
 */
static ULONG EiNrFreeBlocks = 0;

/*
 * Count of used blocks
 */
static ULONG EiNrUsedBlocks = 0;
#endif

/*
 * Lock that protects the non-paged pool data structures
 */
static KSPIN_LOCK MmNpoolLock;

/*
 * Total memory used for free nonpaged pool blocks
 */
ULONG EiFreeNonPagedPool = 0;

/*
 * Total memory used for nonpaged pool blocks
 */
ULONG EiUsedNonPagedPool = 0;

/*
 * Allocate a range of memory in the nonpaged pool
 */
PVOID
MiAllocNonPagedPoolRegion(unsigned int nr_pages);

VOID
MiFreeNonPagedPoolRegion(PVOID Addr, ULONG Count, BOOLEAN Free);

#ifdef TAG_STATISTICS_TRACKING
#define TAG_HASH_TABLE_SIZE       (1024)
static BLOCK_HDR* tag_hash_table[TAG_HASH_TABLE_SIZE];
#endif /* TAG_STATISTICS_TRACKING */

/* FUNCTIONS ***************************************************************/

#ifdef TAG_STATISTICS_TRACKING
VOID
MiRemoveFromTagHashTable(BLOCK_HDR* block)
     /*
      * Remove a block from the tag hash table
      */
{
  BLOCK_HDR* previous;
  BLOCK_HDR* current;
  ULONG hash;

  if (block->Tag == 0)
    {
      return;
    }

  hash = block->Tag % TAG_HASH_TABLE_SIZE;
  
  previous = NULL;
  current = tag_hash_table[hash];
  while (current != NULL)
    {
      if (current == block)
	{
	  if (previous == NULL)
	    {
	      tag_hash_table[hash] = block->tag_next;
	    }
	  else
	    {
	      previous->tag_next = block->tag_next;
	    }
	  return;
	}
      previous = current;
      current = current->tag_next;
    }
  DPRINT1("Tagged block wasn't on hash table list (Tag %x Caller %x)\n",
	  block->Tag, block->Caller);
  KeBugCheck(0);
}

VOID
MiAddToTagHashTable(BLOCK_HDR* block)
     /*
      * Add a block to the tag hash table
      */
{
  ULONG hash;
  BLOCK_HDR* current;
  BLOCK_HDR* previous;

  if (block->Tag == 0)
    {
      return;
    }

  hash = block->Tag % TAG_HASH_TABLE_SIZE;

  previous = NULL;
  current = tag_hash_table[hash];
  while (current != NULL)
    {
      if (current->Tag == block->Tag)
	{
	  block->tag_next = current->tag_next;
	  current->tag_next = block;
	  return;
	}
      previous = current;
      if (current->tag_next &&((PVOID)current->tag_next >= (PVOID)kernel_pool_base + NONPAGED_POOL_SIZE || (PVOID)current->tag_next < (PVOID)kernel_pool_base))
	{
	  DbgPrint("previous %x\n", previous);
	}
      current = current->tag_next;
    }
  block->tag_next = NULL;
  if (previous == NULL)
    {
      tag_hash_table[hash] = block;
    }
  else
    {
      previous->tag_next = block;
    }
}
#endif /* TAG_STATISTICS_TRACKING */

VOID 
ExInitNonPagedPool(ULONG BaseAddress)
{
   kernel_pool_base = BaseAddress;
   KeInitializeSpinLock(&MmNpoolLock);
   MmInitKernelMap((PVOID)BaseAddress);
   memset(tag_hash_table, 0, sizeof(tag_hash_table));
   InitializeListHead(&FreeBlockListHead);
   InitializeListHead(&UsedBlockListHead);
}

#ifdef TAG_STATISTICS_TRACKING
VOID STATIC
MiDumpTagStats(ULONG CurrentTag, ULONG CurrentNrBlocks, ULONG CurrentSize)
{
  CHAR c1, c2, c3, c4;
  
  c1 = (CurrentTag >> 24) & 0xFF;
  c2 = (CurrentTag >> 16) & 0xFF;
  c3 = (CurrentTag >> 8) & 0xFF;
  c4 = CurrentTag & 0xFF;
  
  if (isprint(c1) && isprint(c2) && isprint(c3) && isprint(c4))
    {
      DbgPrint("Tag %x (%c%c%c%c) Blocks %d Total Size %d Average Size %d\n",
	       CurrentTag, c4, c3, c2, c1, CurrentNrBlocks,
	       CurrentSize, CurrentSize / CurrentNrBlocks);
    }
  else
    {
      DbgPrint("Tag %x Blocks %d Total Size %d Average Size %d\n",
	       CurrentTag, CurrentNrBlocks, CurrentSize,
	       CurrentSize / CurrentNrBlocks);
    }
}
#endif /* TAG_STATISTICS_TRACKING */

VOID
MiDebugDumpNonPagedPoolStats(BOOLEAN NewOnly)
{
#ifdef TAG_STATISTICS_TRACKING
  ULONG i;
  BLOCK_HDR* current;
  ULONG CurrentTag;
  ULONG CurrentNrBlocks;
  ULONG CurrentSize;
  ULONG TotalBlocks;
  ULONG TotalSize;

  DbgPrint("******* Dumping non paging pool stats ******\n");
  TotalBlocks = 0;
  TotalSize = 0;
  for (i = 0; i < TAG_HASH_TABLE_SIZE; i++)
    {
      CurrentTag = 0;
      CurrentNrBlocks = 0;
      CurrentSize = 0;
      current = tag_hash_table[i];
      while (current != NULL)
	{
	  if (current->Tag != CurrentTag)
	    {
	      if (CurrentTag != 0 && CurrentNrBlocks != 0)
		{
		  MiDumpTagStats(CurrentTag, CurrentNrBlocks, CurrentSize);
		}
	      CurrentTag = current->Tag;
	      CurrentNrBlocks = 0;
	      CurrentSize = 0;
	    }

	  if (!NewOnly || !current->Dumped)
	    {
	      CurrentNrBlocks++;
	      TotalBlocks++;
	      CurrentSize = CurrentSize + current->Size;
	      TotalSize = TotalSize + current->Size;
	      current->Dumped = TRUE;
	    }
	  current = current->tag_next;
	}
      if (CurrentTag != 0 && CurrentNrBlocks != 0)
	{
	  MiDumpTagStats(CurrentTag, CurrentNrBlocks, CurrentSize);
	}
    }
  if (TotalBlocks != 0)
    {
      DbgPrint("TotalBlocks %d TotalSize %d AverageSize %d\n",
	       TotalBlocks, TotalSize, TotalSize / TotalBlocks);
    }
  else
    {
      DbgPrint("TotalBlocks %d TotalSize %d\n",
	       TotalBlocks, TotalSize);
    }
  DbgPrint("***************** Dump Complete ***************\n");
#endif /* TAG_STATISTICS_TRACKING */
}

VOID
MiDebugDumpNonPagedPool(BOOLEAN NewOnly)
{
   BLOCK_HDR* current;
   PLIST_ENTRY current_entry;
   KIRQL oldIrql;
   
   KeAcquireSpinLock(&MmNpoolLock, &oldIrql);

   DbgPrint("******* Dumping non paging pool contents ******\n");
   current_entry = UsedBlockListHead.Flink;
   while (current_entry != &UsedBlockListHead)
     {
       current = CONTAINING_RECORD(current_entry, BLOCK_HDR, ListEntry);
       if (!NewOnly || !current->Dumped)
	 {
	   CHAR c1, c2, c3, c4;
	   
	   c1 = (current->Tag >> 24) & 0xFF;
	   c2 = (current->Tag >> 16) & 0xFF;
	   c3 = (current->Tag >> 8) & 0xFF;
	   c4 = current->Tag & 0xFF;
	   
	   if (isprint(c1) && isprint(c2) && isprint(c3) && isprint(c4))
	     {
	       DbgPrint("Size 0x%x Tag 0x%x (%c%c%c%c) Allocator 0x%x\n",
			current->Size, current->Tag, c4, c3, c2, c1, 
			current->Caller);
	     }
	   else
	     {
	       DbgPrint("Size 0x%x Tag 0x%x Allocator 0x%x\n",
			current->Size, current->Tag, current->Caller);
	     }
	   current->Dumped = TRUE;
	 }
       current_entry = current_entry->Flink;
     }
   DbgPrint("***************** Dump Complete ***************\n");
   KeReleaseSpinLock(&MmNpoolLock, oldIrql);
}

#ifndef WHOLE_PAGE_ALLOCATIONS

#ifdef ENABLE_VALIDATE_POOL
static void validate_free_list(void)
/*
 * FUNCTION: Validate the integrity of the list of free blocks
 */
{
   BLOCK_HDR* current;
   PLIST_ENTRY current_entry;
   unsigned int blocks_seen=0;     
   
   current_entry = FreeBlockListHead.Flink;
   while (current_entry != &FreeBlockListHead)
     {
	unsigned int base_addr;

	current = CONTAINING_RECORD(current_entry, BLOCK_HDR, ListEntry);
	base_addr = (int)current;

	if (current->Magic != BLOCK_HDR_FREE_MAGIC)
	  {
	     DbgPrint("Bad block magic (probable pool corruption) at %x\n",
		      current);
	     KeBugCheck(KBUG_POOL_FREE_LIST_CORRUPT);
	  }
	
	if (base_addr < (kernel_pool_base) ||
	    (base_addr+current->Size) > (kernel_pool_base)+NONPAGED_POOL_SIZE)
	  {		       
	     DbgPrint("Block %x found outside pool area\n",current);
	     DbgPrint("Size %d\n",current->Size);
	     DbgPrint("Limits are %x %x\n",kernel_pool_base,
		    kernel_pool_base+NONPAGED_POOL_SIZE);
	     KeBugCheck(KBUG_POOL_FREE_LIST_CORRUPT);
	  }
	blocks_seen++;
	if (blocks_seen > EiNrFreeBlocks)
	  {
	     DbgPrint("Too many blocks on free list\n");
	     KeBugCheck(KBUG_POOL_FREE_LIST_CORRUPT);
	  }
	if (current->ListEntry.Flink != &FreeBlockListHead &&
	    current->ListEntry.Flink->Blink != &current->ListEntry)
	  {
	     DbgPrint("%s:%d:Break in list (current %x next %x "
		    "current->next->previous %x)\n",
		    __FILE__,__LINE__,current, current->ListEntry.Flink,
		    current->ListEntry.Flink->Blink);
	     KeBugCheck(KBUG_POOL_FREE_LIST_CORRUPT);
	  }

	current_entry = current_entry->Flink;
     }
}

static void validate_used_list(void)
/*
 * FUNCTION: Validate the integrity of the list of used blocks
 */
{
   BLOCK_HDR* current;
   PLIST_ENTRY current_entry;
   unsigned int blocks_seen=0;
   
   current_entry = UsedBlockListHead.Flink;
   while (current_entry != &UsedBlockListHead)
     {
	unsigned int base_addr;

	current = CONTAINING_RECORD(current_entry, BLOCK_HDR, ListEntry);
	base_addr = (int)current;
	
	if (current->Magic != BLOCK_HDR_USED_MAGIC)
	  {
	     DbgPrint("Bad block magic (probable pool corruption) at %x\n",
		      current);
	     KeBugCheck(KBUG_POOL_FREE_LIST_CORRUPT);
	  }
	if (base_addr < (kernel_pool_base) ||
	    (base_addr+current->Size) >
	    (kernel_pool_base)+NONPAGED_POOL_SIZE)
	  {
	     DbgPrint("Block %x found outside pool area\n",current);
	     for(;;);
	  }
	blocks_seen++;
	if (blocks_seen > EiNrUsedBlocks)
	  {
	     DbgPrint("Too many blocks on used list\n");
	     for(;;);
	  }
	if (current->ListEntry.Flink != &UsedBlockListHead &&
	    current->ListEntry.Flink->Blink != &current->ListEntry)
	  {
	     DbgPrint("Break in list (current %x next %x)\n",
		    current, current->ListEntry.Flink);
	     for(;;);
	  }

	current_entry = current_entry->Flink;
     }
}

static void check_duplicates(BLOCK_HDR* blk)
/*
 * FUNCTION: Check a block has no duplicates
 * ARGUMENTS:
 *           blk = block to check
 * NOTE: Bug checks if duplicates are found
 */
{
   unsigned int base = (int)blk;
   unsigned int last = ((int)blk) + +sizeof(BLOCK_HDR) + blk->Size;
   BLOCK_HDR* current;
   PLIST_ENTRY current_entry;
   
   current_entry = FreeBlockListHead.Flink;
   while (current_entry != &FreeBlockListHead)
     {
       current = CONTAINING_RECORD(current_entry, BLOCK_HDR, ListEntry);

       if (current->Magic != BLOCK_HDR_FREE_MAGIC)
	 {
	   DbgPrint("Bad block magic (probable pool corruption) at %x\n",
		    current);
	     KeBugCheck(KBUG_POOL_FREE_LIST_CORRUPT);
	 }
       
       if ( (int)current > base && (int)current < last ) 
	 {
	   DbgPrint("intersecting blocks on list\n");
	   for(;;);
	 }
       if  ( (int)current < base &&
	     ((int)current + current->Size + sizeof(BLOCK_HDR))
	     > base )
	 {
	   DbgPrint("intersecting blocks on list\n");
	   for(;;);
	  }

       current_entry = current_entry->Flink;
     }

   current_entry = UsedBlockListHead.Flink;
   while (current_entry != &UsedBlockListHead)
     {
       current = CONTAINING_RECORD(current_entry, BLOCK_HDR, ListEntry);

       if ( (int)current > base && (int)current < last ) 
	 {
	   DbgPrint("intersecting blocks on list\n");
	   for(;;);
	 }
       if  ( (int)current < base &&
	     ((int)current + current->Size + sizeof(BLOCK_HDR))
	     > base )
	 {
	   DbgPrint("intersecting blocks on list\n");
	   for(;;);
	 }
       
       current_entry = current_entry->Flink;
     }
   
}

static void validate_kernel_pool(void)
/*
 * FUNCTION: Checks the integrity of the kernel memory heap
 */
{
   BLOCK_HDR* current;
   PLIST_ENTRY current_entry;
   
   validate_free_list();
   validate_used_list();

   current_entry = FreeBlockListHead.Flink;
   while (current_entry != &FreeBlockListHead)
     {
       current = CONTAINING_RECORD(current_entry, BLOCK_HDR, ListEntry);
       check_duplicates(current);
       current_entry = current_entry->Flink;
     }
   current_entry = UsedBlockListHead.Flink;
   while (current_entry != &UsedBlockListHead)
     {
       current = CONTAINING_RECORD(current_entry, BLOCK_HDR, ListEntry);
       check_duplicates(current);
       current_entry = current_entry->Flink;
     }
}
#endif

#if 0
STATIC VOID
free_pages(BLOCK_HDR* blk)
{
  ULONG start;
  ULONG end;
  ULONG i;

  start = (ULONG)blk;
  end = (ULONG)blk + sizeof(BLOCK_HDR) + blk->Size;

  /*
   * If the block doesn't contain a whole page then there is nothing to do
   */
  if (PAGE_ROUND_UP(start) >= PAGE_ROUND_DOWN(end))
    {
      return;
    }
}
#endif

STATIC VOID
merge_free_block(BLOCK_HDR* blk)
{
  PLIST_ENTRY next_entry;
  BLOCK_HDR* next;
  PLIST_ENTRY previous_entry;
  BLOCK_HDR* previous;

  next_entry = blk->ListEntry.Flink;
  if (next_entry != &FreeBlockListHead)
    {
      next = CONTAINING_RECORD(next_entry, BLOCK_HDR, ListEntry);
      if (((unsigned int)blk + sizeof(BLOCK_HDR) + blk->Size) == 
	  (unsigned int)next)
	{
	  RemoveEntryList(&next->ListEntry);
	  blk->Size = blk->Size + next->Size;
	  memset(next, 0xcc, sizeof(BLOCK_HDR));
          EiFreeNonPagedPool += sizeof(BLOCK_HDR);
	  EiNrFreeBlocks--;
	}
    }

  previous_entry = blk->ListEntry.Blink;
  if (previous_entry != &FreeBlockListHead)
    {
      previous = CONTAINING_RECORD(previous_entry, BLOCK_HDR, ListEntry);
      if (((unsigned int)previous + sizeof(BLOCK_HDR) + previous->Size) == 
	  (unsigned int)blk)
	{
	  RemoveEntryList(&blk->ListEntry);
	  previous->Size = previous->Size + sizeof(BLOCK_HDR) + blk->Size;
	  memset(blk, 0xcc, sizeof(BLOCK_HDR));
          EiFreeNonPagedPool += sizeof(BLOCK_HDR);
	  EiNrFreeBlocks--;
	}
    }
}

STATIC VOID 
add_to_free_list(BLOCK_HDR* blk)
/*
 * FUNCTION: add the block to the free list (internal)
 */
{
  PLIST_ENTRY current_entry;
  BLOCK_HDR* current;

  current_entry = FreeBlockListHead.Flink;
  while (current_entry != &FreeBlockListHead)
    {
      current = CONTAINING_RECORD(current_entry, BLOCK_HDR, ListEntry);

      if ((unsigned int)current > (unsigned int)blk)
	{
	  blk->ListEntry.Flink = current_entry;
	  blk->ListEntry.Blink = current_entry->Blink;
	  current_entry->Blink->Flink = &blk->ListEntry;
	  current_entry->Blink = &blk->ListEntry;
          EiFreeNonPagedPool += blk->Size;
	  EiNrFreeBlocks++;
	  return;
	}

      current_entry = current_entry->Flink;
    }
  InsertTailList(&FreeBlockListHead, &blk->ListEntry);
  EiFreeNonPagedPool += blk->Size;
  EiNrFreeBlocks++;
}

static void add_to_used_list(BLOCK_HDR* blk)
/*
 * FUNCTION: add the block to the used list (internal)
 */
{
  InsertHeadList(&UsedBlockListHead, &blk->ListEntry);
  EiUsedNonPagedPool += blk->Size;
  EiNrUsedBlocks++;
}


static void remove_from_free_list(BLOCK_HDR* current)
{
  RemoveEntryList(&current->ListEntry);
  EiFreeNonPagedPool -= current->Size;
  EiNrFreeBlocks--;
}


static void remove_from_used_list(BLOCK_HDR* current)
{
  RemoveEntryList(&current->ListEntry);
  EiUsedNonPagedPool -= current->Size;
  EiNrUsedBlocks--;
}


inline static void* block_to_address(BLOCK_HDR* blk)
/*
 * FUNCTION: Translate a block header address to the corresponding block
 * address (internal)
 */
{
        return ( (void *) ((int)blk + sizeof(BLOCK_HDR)) );
}

inline static BLOCK_HDR* address_to_block(void* addr)
{
        return (BLOCK_HDR *)
               ( ((int)addr) - sizeof(BLOCK_HDR) );
}

static BLOCK_HDR* lookup_block(unsigned int size)
{
   PLIST_ENTRY current_entry;
   BLOCK_HDR* current;
   BLOCK_HDR* best = NULL;
   ULONG new_size;
   PVOID block, block_boundary;
    
   current_entry = FreeBlockListHead.Flink;
   if (size < PAGE_SIZE)
   {
      while (current_entry != &FreeBlockListHead)
      {
         DPRINT("current %x size %x tag_next %x\n",
	        current, current->Size, current->tag_next);
	 current = CONTAINING_RECORD(current_entry, BLOCK_HDR, ListEntry);
         if (current->Size >= size && 
	     (best == NULL || current->Size < best->Size)) 
	 {
	    best = current;
	 }
         current_entry = current_entry->Flink;
      }
   }
   else
   {
      while (current_entry != &FreeBlockListHead)
      {
         DPRINT("current %x size %x tag_next %x\n",
	        current, current->Size, current->tag_next);
	 current = CONTAINING_RECORD(current_entry, BLOCK_HDR, ListEntry);

	 block = block_to_address(current);
	 block_boundary = (PVOID)PAGE_ROUND_UP((ULONG)block);
	 new_size = (ULONG)block_boundary - (ULONG)block + size;
	 if (new_size != size && (ULONG)block_boundary - (ULONG)block < sizeof(BLOCK_HDR))
	 {
	     new_size += PAGE_SIZE;
	 }
	 if (current->Size >= new_size && 
	     (best == NULL || current->Size < best->Size))
	 {
	    best = current;
	 }
         current_entry = current_entry->Flink;
      }
   }
   return best;
}

static void* take_block(BLOCK_HDR* current, unsigned int size,
			ULONG Tag, PVOID Caller)
/*
 * FUNCTION: Allocate a used block of least 'size' from the specified
 * free block
 * RETURNS: The address of the created memory block
 */
{

    BLOCK_HDR* blk;
    if (size >= PAGE_SIZE)
    {
       blk = address_to_block((PVOID)PAGE_ROUND_UP(block_to_address (current)));
       if (blk != current)
       {
          if ((ULONG)blk - (ULONG)current < sizeof(BLOCK_HDR))
	  {
             (ULONG)blk += PAGE_SIZE;
	  }
	  assert((ULONG)blk - (ULONG)current + size <= current->Size && (ULONG)blk - (ULONG)current >= sizeof(BLOCK_HDR));

	  memset(blk, 0, sizeof(BLOCK_HDR));
	  blk->Magic = BLOCK_HDR_FREE_MAGIC;
	  blk->Size = current->Size - ((ULONG)blk - (ULONG)current);
	  current->Size -= (blk->Size + sizeof(BLOCK_HDR));
	  InsertHeadList(&current->ListEntry, &blk->ListEntry);
          EiFreeNonPagedPool -= sizeof(BLOCK_HDR);
          EiNrFreeBlocks++;
	  current = blk;
       }
   }
   /*
    * If the block is much bigger than required then split it and
    * return a pointer to the allocated section. If the difference
    * between the sizes is marginal it makes no sense to have the
    * extra overhead 
    */
   if (current->Size > size + sizeof(BLOCK_HDR))
     {
	BLOCK_HDR* free_blk;
	
	EiFreeNonPagedPool -= current->Size;
	
	/*
	 * Replace the bigger block with a smaller block in the
	 * same position in the list
	 */
        free_blk = (BLOCK_HDR *)(((int)current)
				 + sizeof(BLOCK_HDR) + size);		
	free_blk->Magic = BLOCK_HDR_FREE_MAGIC;
	InsertHeadList(&current->ListEntry, &free_blk->ListEntry);
	free_blk->Size = current->Size - (sizeof(BLOCK_HDR) + size);
	
	current->Size=size;
	RemoveEntryList(&current->ListEntry);
	add_to_used_list(current);
	current->Magic = BLOCK_HDR_USED_MAGIC;
	current->Tag = Tag;
	current->Caller = Caller;
	current->Dumped = FALSE;
#ifdef TAG_STATISTICS_TRACKING
	MiAddToTagHashTable(current);
#endif /* TAG_STATISTICS_TRACKING */
	
	EiFreeNonPagedPool += free_blk->Size;
	
	VALIDATE_POOL;
	return(block_to_address(current));
     }
   
   /*
    * Otherwise allocate the whole block
    */
   remove_from_free_list(current);
   add_to_used_list(current);
   
   current->Magic = BLOCK_HDR_USED_MAGIC;   
   current->Tag = Tag;
   current->Caller = Caller;
   current->Dumped = FALSE;
#ifdef TAG_STATISTICS_TRACKING
   MiAddToTagHashTable(current);
#endif /* TAG_STATISTICS_TRACKING */

   VALIDATE_POOL;
   return(block_to_address(current));
}

static void* grow_kernel_pool(unsigned int size, ULONG Tag, PVOID Caller)
/*
 * FUNCTION: Grow the executive heap to accomodate a block of at least 'size'
 * bytes
 */
{
   ULONG nr_pages = PAGE_ROUND_UP(size + sizeof(BLOCK_HDR)) / PAGE_SIZE;
   ULONG start;
   BLOCK_HDR* blk=NULL;
   ULONG i;
   KIRQL oldIrql;
   NTSTATUS Status;
   PVOID block = NULL;

   if (size >= PAGE_SIZE)
   {
      nr_pages++;
   }

   start = (ULONG)MiAllocNonPagedPoolRegion(nr_pages);
 
   DPRINT("growing heap for block size %d, ",size);
   DPRINT("start %x\n",start);
  
   for (i=0;i<nr_pages;i++)
     {
       PHYSICAL_ADDRESS Page;
       /* FIXME: Check whether we can really wait here. */
       Status = MmRequestPageMemoryConsumer(MC_NPPOOL, TRUE, &Page);
       if (!NT_SUCCESS(Status))
	 {
	   KeBugCheck(0);
	   return(NULL);
	 }
       Status = MmCreateVirtualMapping(NULL,
				       (PVOID)(start + (i*PAGE_SIZE)),
				       PAGE_READWRITE|PAGE_SYSTEM,
				       Page,
				       FALSE);
	if (!NT_SUCCESS(Status))
	  {
	     DbgPrint("Unable to create virtual mapping\n");
	     KeBugCheck(0);
	  }
     }

   blk = (struct _BLOCK_HDR *)start;
   memset(blk, 0, sizeof(BLOCK_HDR));
   blk->Size = (nr_pages * PAGE_SIZE) - sizeof(BLOCK_HDR);
   blk->Magic = BLOCK_HDR_FREE_MAGIC;
   memset(block_to_address(blk), 0xcc, blk->Size);

   KeAcquireSpinLock(&MmNpoolLock, &oldIrql);
   add_to_free_list(blk);
   merge_free_block(blk);

   blk = lookup_block(size);
   if (blk)
   {
      block = take_block(blk, size, Tag, Caller);
      VALIDATE_POOL;
   }
   KeReleaseSpinLock(&MmNpoolLock, oldIrql);
   if (block == NULL)
   {
       CHECKPOINT1;
   }
   return block;
}

#endif /* not WHOLE_PAGE_ALLOCATIONS */

VOID STDCALL ExFreeNonPagedPool (PVOID block)
/*
 * FUNCTION: Releases previously allocated memory
 * ARGUMENTS:
 *        block = block to free
 */
{
#ifdef WHOLE_PAGE_ALLOCATIONS /* WHOLE_PAGE_ALLOCATIONS */
   KIRQL oldIrql;

   if (block == NULL)
     {
       return;
     }

   DPRINT("freeing block %x\n",blk);
   
   POOL_TRACE("ExFreePool(block %x), size %d, caller %x\n",block,blk->size,
            ((PULONG)&block)[-1]);
   
   KeAcquireSpinLock(&MmNpoolLock, &oldIrql);

   ExFreeWholePageBlock(block);
   KeReleaseSpinLock(&MmNpoolLock, oldIrql);      

#else /* not WHOLE_PAGE_ALLOCATIONS */

   BLOCK_HDR* blk=address_to_block(block);
   KIRQL oldIrql;

   if (block == NULL)
     {
       return;
     }

   DPRINT("freeing block %x\n",blk);
   
   POOL_TRACE("ExFreePool(block %x), size %d, caller %x\n",block,blk->size,
            ((PULONG)&block)[-1]);
   
   KeAcquireSpinLock(&MmNpoolLock, &oldIrql);

   VALIDATE_POOL;
   
   if (blk->Magic != BLOCK_HDR_USED_MAGIC)
     {
       if (blk->Magic == BLOCK_HDR_FREE_MAGIC)
	 {
	   DbgPrint("ExFreePool of already freed address %x\n", block);
	 }
       else
	 {
	   DbgPrint("ExFreePool of non-allocated address %x (magic %x)\n", 
		    block, blk->Magic);
	 }
	KeBugCheck(0);
	return;
     }
   
   memset(block, 0xcc, blk->Size);
   
#ifdef TAG_STATISTICS_TRACKING
   MiRemoveFromTagHashTable(blk);
#endif /* TAG_STATISTICS_TRACKING */
   remove_from_used_list(blk);
   blk->Magic = BLOCK_HDR_FREE_MAGIC;
   blk->Tag = 0;
   blk->Caller = NULL;
   blk->tag_next = NULL;
   add_to_free_list(blk);
   merge_free_block(blk);

   VALIDATE_POOL;
   KeReleaseSpinLock(&MmNpoolLock, oldIrql);

#endif /* WHOLE_PAGE_ALLOCATIONS */
}

PVOID STDCALL 
ExAllocateNonPagedPoolWithTag(ULONG Type, ULONG Size, ULONG Tag, PVOID Caller)
{
#ifdef WHOLE_PAGE_ALLOCATIONS
   PVOID block;
   KIRQL oldIrql;
   
   POOL_TRACE("ExAllocatePool(NumberOfBytes %d) caller %x ",
	      Size,Caller);
   
   KeAcquireSpinLock(&MmNpoolLock, &oldIrql);
   
   /*
    * accomodate this useful idiom
    */
   if (Size == 0)
     {
	POOL_TRACE("= NULL\n");
	KeReleaseSpinLock(&MmNpoolLock, oldIrql);
	return(NULL);
     }

   block = ExAllocateWholePageBlock(Size);
   KeReleaseSpinLock(&MmNpoolLock, oldIrql);
   return(block);

#else /* not WHOLE_PAGE_ALLOCATIONS */
   PVOID block;
   BLOCK_HDR* best = NULL;
   KIRQL oldIrql;
   
   POOL_TRACE("ExAllocatePool(NumberOfBytes %d) caller %x ",
	      Size,Caller);
   
   KeAcquireSpinLock(&MmNpoolLock, &oldIrql);

   VALIDATE_POOL;

#if 0
   /* after some allocations print the npaged pool stats */
#ifdef TAG_STATISTICS_TRACKING
   {
       static ULONG counter = 0;
       if (counter++ % 100000 == 0)
       {
          MiDebugDumpNonPagedPoolStats(FALSE);   
       }
   }
#endif
#endif
   /*
    * accomodate this useful idiom
    */
   if (Size == 0)
     {
	POOL_TRACE("= NULL\n");
	KeReleaseSpinLock(&MmNpoolLock, oldIrql);
	return(NULL);
     }
   /* Make the size dword alligned, this makes the block dword alligned */  
   Size = ROUND_UP(Size, 4);
   /*
    * Look for an already created block of sufficent size
    */
   best = lookup_block(Size);
   if (best == NULL)
   {
       KeReleaseSpinLock(&MmNpoolLock, oldIrql);
       block = grow_kernel_pool(Size, Tag, Caller);
       assert(block != NULL);
       memset(block,0,Size);
   }
   else
   {
       block=take_block(best, Size, Tag, Caller);
       VALIDATE_POOL;
       KeReleaseSpinLock(&MmNpoolLock, oldIrql);
       memset(block,0,Size);
   }
   return(block);
#endif /* WHOLE_PAGE_ALLOCATIONS */
}

#ifdef WHOLE_PAGE_ALLOCATIONS

PVOID STDCALL 
ExAllocateWholePageBlock(ULONG UserSize)
{
  PVOID Address;
  PHYSICAL_ADDRESS Page;
  ULONG i;
  ULONG Size;
  ULONG NrPages;

  Size = sizeof(ULONG) + UserSize;
  NrPages = ROUND_UP(Size, PAGE_SIZE) / PAGE_SIZE;

  Address = MiAllocNonPagedPoolRegion(NrPages + 1);

  for (i = 0; i < NrPages; i++)
    {
      Page = MmAllocPage(MC_NPPOOL, 0);
      if (Page.QuadPart == 0LL)
	{
	  KeBugCheck(0);
	}
      MmCreateVirtualMapping(NULL, 
			     Address + (i * PAGE_SIZE),
			     PAGE_READWRITE | PAGE_SYSTEM,
			     Page,
			     TRUE);
    }

  *((PULONG)((ULONG)Address + (NrPages * PAGE_SIZE) - Size)) = NrPages;
  return((PVOID)((ULONG)Address + (NrPages * PAGE_SIZE) - UserSize));
}

VOID STDCALL
ExFreeWholePageBlock(PVOID Addr)
{
  ULONG NrPages;
  
  if ((ULONG)Addr < kernel_pool_base ||
      (ULONG)Addr >= (kernel_pool_base + NONPAGED_POOL_SIZE))
    {
      DbgPrint("Block %x found outside pool area\n", Addr);
      KeBugCheck(0);
    }
  NrPages = *(PULONG)((ULONG)Addr - sizeof(ULONG));
  MiFreeNonPagedPoolRegion((PVOID)PAGE_ROUND_DOWN((ULONG)Addr), NrPages, TRUE);
}

#endif /* WHOLE_PAGE_ALLOCATIONS */

/* EOF */
