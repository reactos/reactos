/* $Id: npool.c,v 1.69 2003/07/10 21:05:03 royce Exp $
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

/* avl types ****************************************************************/

/* FIXME:
 *   This declarations should be moved into a separate header file.
 */

typedef struct _NODE
{
  struct _NODE* link[2];
  struct _NODE* parent;
  signed char balance;
} NODE, *PNODE;

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
  LIST_ENTRY AddressList;
  LIST_ENTRY TagListEntry;
  NODE Node;
  ULONG Tag;
  PVOID Caller;
  BOOLEAN Dumped;
} BLOCK_HDR;

PVOID STDCALL 
ExAllocateWholePageBlock(ULONG Size);
VOID STDCALL
ExFreeWholePageBlock(PVOID Addr);

/* GLOBALS *****************************************************************/

extern PVOID MiNonPagedPoolStart;
extern ULONG MiNonPagedPoolLength;
static ULONG MiCurrentNonPagedPoolLength = 0;

/*
 * Head of the list of free blocks
 */
static PNODE FreeBlockListRoot = NULL;

/*
 * Head of the list of in use block
 */
static LIST_ENTRY UsedBlockListHead;

static LIST_ENTRY AddressListHead;

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
static LIST_ENTRY tag_hash_table[TAG_HASH_TABLE_SIZE];
#endif /* TAG_STATISTICS_TRACKING */

/* avl helper functions ****************************************************/

void DumpFreeBlockNode(PNODE p)
{
  static int count = 0;
  BLOCK_HDR* blk;

  count++;

  if (p)
    {
      DumpFreeBlockNode(p->link[0]);
      blk = CONTAINING_RECORD(p, BLOCK_HDR, Node);
      DbgPrint("%08x %8d (%d)\n", blk, blk->Size, count);
      DumpFreeBlockNode(p->link[1]);
    }

  count--;
}
void DumpFreeBlockTree(void)
{
  DbgPrint("--- Begin tree ------------------\n");
  DbgPrint("%08x\n", CONTAINING_RECORD(FreeBlockListRoot, BLOCK_HDR, Node));
  DumpFreeBlockNode(FreeBlockListRoot);
  DbgPrint("--- End tree --------------------\n");
}

int compare_node(PNODE p1, PNODE p2)
{
  BLOCK_HDR* blk1 = CONTAINING_RECORD(p1, BLOCK_HDR, Node);
  BLOCK_HDR* blk2 = CONTAINING_RECORD(p2, BLOCK_HDR, Node);

  if (blk1->Size == blk2->Size)
    {
      if (blk1 < blk2)
        {
	  return -1;
        }
      if (blk1 > blk2)
        {
	  return 1;
	}
    }
  else
    {
      if (blk1->Size < blk2->Size)
        {
          return -1;
        }
      if (blk1->Size > blk2->Size)
        {
          return 1;
        }
    }
  return 0;

}

int compare_value(PVOID value, PNODE p)
{
  BLOCK_HDR* blk = CONTAINING_RECORD(p, BLOCK_HDR, Node);
  ULONG v = *(PULONG)value;

  if (v < blk->Size)
    {
      return -1;
    }
  if (v > blk->Size)
    {
      return 1;
    }
  return 0;
}

/* avl functions **********************************************************/

/* FIXME:
 *   The avl functions should be moved into a separate file.
 */

/* The avl_insert and avl_remove are based on libavl (library for manipulation of binary trees). */

void avl_insert (PNODE * root, PNODE n, int (*compare)(PNODE, PNODE))
{
  PNODE y;     /* Top node to update balance factor, and parent. */
  PNODE p, q;  /* Iterator, and parent. */
  PNODE w;     /* New root of rebalanced subtree. */
  int dir;     /* Direction to descend. */

  n->link[0] = n->link[1] = n->parent = NULL;
  n->balance = 0;

  y = *root;
  for (q = NULL, p = *root; p != NULL; q = p, p = p->link[dir])
    {
      dir = compare(n, p) > 0;
      if (p->balance != 0)
        {
          y = p;
	}
    }

  n->parent = q;
  if (q != NULL)
    {
      q->link[dir] = n;
    }
  else
    {
      *root = n;
    }

  if (*root == n)
    {
      return;
    }

  for (p = n; p != y; p = q)
    {
      q = p->parent;
      dir = q->link[0] != p;
      if (dir == 0)
        {
          q->balance--;
	}
      else
        {
          q->balance++;
	}
    }

  if (y->balance == -2)
    {
      PNODE x = y->link[0];
      if (x->balance == -1)
        {
          w = x;
          y->link[0] = x->link[1];
          x->link[1] = y;
          x->balance = y->balance = 0;
          x->parent = y->parent;
          y->parent = x;
          if (y->link[0] != NULL)
	    {
              y->link[0]->parent = y;
	    }
        }
      else
        {
          assert (x->balance == +1);
          w = x->link[1];
          x->link[1] = w->link[0];
          w->link[0] = x;
          y->link[0] = w->link[1];
          w->link[1] = y;
          if (w->balance == -1)
	    {
              x->balance = 0;
	      y->balance = +1;
	    }
          else if (w->balance == 0)
	    {
              x->balance = y->balance = 0;
	    }
          else /* |w->pavl_balance == +1| */
	    {
              x->balance = -1;
	      y->balance = 0;
	    }
          w->balance = 0;
          w->parent = y->parent;
          x->parent = y->parent = w;
          if (x->link[1] != NULL)
	    {
              x->link[1]->parent = x;
	    }
          if (y->link[0] != NULL)
	    {
              y->link[0]->parent = y;
	    }
        }
    }
  else if (y->balance == +2)
    {
      PNODE x = y->link[1];
      if (x->balance == +1)
        {
          w = x;
          y->link[1] = x->link[0];
          x->link[0] = y;
          x->balance = y->balance = 0;
          x->parent = y->parent;
          y->parent = x;
          if (y->link[1] != NULL)
	    {
              y->link[1]->parent = y;
	    }
        }
      else
        {
          assert (x->balance == -1);
          w = x->link[0];
          x->link[0] = w->link[1];
          w->link[1] = x;
          y->link[1] = w->link[0];
          w->link[0] = y;
          if (w->balance == 1)
	    {
              x->balance = 0;
	      y->balance = -1;
	    }
          else if (w->balance == 0)
	    {
              x->balance = y->balance = 0;
	    }
          else /* |w->pavl_balance == -1| */
	    {
              x->balance = +1;
	      y->balance = 0;
	    }
          w->balance = 0;
          w->parent = y->parent;
          x->parent = y->parent = w;
          if (x->link[0] != NULL)
	    {
              x->link[0]->parent = x;
	    }
          if (y->link[1] != NULL)
	    {
              y->link[1]->parent = y;
	    }
        }
    }
  else
    {
      return;
    }
  if (w->parent != NULL)
    {
      w->parent->link[y != w->parent->link[0]] = w;
    }
  else
    {
      *root = w;
    }

  return;
}

void avl_remove (PNODE *root, PNODE item, int (*compare)(PNODE, PNODE))
{
  PNODE p;  /* Traverses tree to find node to delete. */
  PNODE q;  /* Parent of |p|. */
  int dir;  /* Side of |q| on which |p| is linked. */

  if (root == NULL || *root == NULL)
    {
      return ;
    }

  p = item;
  q = p->parent;
  if (q == NULL)
    {
      q = (PNODE) root;
      dir = 0;
    }
  else
    {
      dir = compare(p, q) > 0;
    }

  if (p->link[1] == NULL)
    {
      q->link[dir] = p->link[0];
      if (q->link[dir] != NULL)
        {
          q->link[dir]->parent = p->parent;
	}
    }
  else
    {
      PNODE r = p->link[1];
      if (r->link[0] == NULL)
        {
          r->link[0] = p->link[0];
          q->link[dir] = r;
          r->parent = p->parent;
          if (r->link[0] != NULL)
	    {
              r->link[0]->parent = r;
	    }
          r->balance = p->balance;
          q = r;
          dir = 1;
        }
      else
        {
          PNODE s = r->link[0];
          while (s->link[0] != NULL)
	    {
              s = s->link[0];
	    }
          r = s->parent;
          r->link[0] = s->link[1];
          s->link[0] = p->link[0];
          s->link[1] = p->link[1];
          q->link[dir] = s;
          if (s->link[0] != NULL)
	    {
              s->link[0]->parent = s;
	    }
          s->link[1]->parent = s;
          s->parent = p->parent;
          if (r->link[0] != NULL)
	    {
              r->link[0]->parent = r;
	    }
          s->balance = p->balance;
          q = r;
          dir = 0;
        }
    }

  item->link[0] = item->link[1] = item->parent = NULL;
  item->balance = 0;

  while (q != (PNODE) root)
    {
      PNODE y = q;

      if (y->parent != NULL)
        {
          q = y->parent;
	}
      else
        {
          q = (PNODE) root;
	}

      if (dir == 0)
        {
          dir = q->link[0] != y;
          y->balance++;
          if (y->balance == +1)
	    {
              break;
	    }
          else if (y->balance == +2)
            {
              PNODE x = y->link[1];
              if (x->balance == -1)
                {
                  PNODE w;

                  assert (x->balance == -1);
                  w = x->link[0];
                  x->link[0] = w->link[1];
                  w->link[1] = x;
                  y->link[1] = w->link[0];
                  w->link[0] = y;
                  if (w->balance == +1)
		    {
                      x->balance = 0;
		      y->balance = -1;
		    }
                  else if (w->balance == 0)
		    {
                      x->balance = y->balance = 0;
		    }
                  else /* |w->pavl_balance == -1| */
		    {
                      x->balance = +1;
		      y->balance = 0;
		    }
                  w->balance = 0;
                  w->parent = y->parent;
                  x->parent = y->parent = w;
                  if (x->link[0] != NULL)
		    {
                      x->link[0]->parent = x;
		    }
                  if (y->link[1] != NULL)
		    {
                      y->link[1]->parent = y;
		    }
                  q->link[dir] = w;
                }
              else
                {
                  y->link[1] = x->link[0];
                  x->link[0] = y;
                  x->parent = y->parent;
                  y->parent = x;
                  if (y->link[1] != NULL)
		    {
                      y->link[1]->parent = y;
		    }
                  q->link[dir] = x;
                  if (x->balance == 0)
                    {
                      x->balance = -1;
                      y->balance = +1;
                      break;
                    }
                  else
                    {
                      x->balance = y->balance = 0;
                      y = x;
                    }
                }
            }
        }
      else
        {
          dir = q->link[0] != y;
          y->balance--;
          if (y->balance == -1)
	    {
              break;
	    }
          else if (y->balance == -2)
            {
              PNODE x = y->link[0];
              if (x->balance == +1)
                {
                  PNODE w;
                  assert (x->balance == +1);
                  w = x->link[1];
                  x->link[1] = w->link[0];
                  w->link[0] = x;
                  y->link[0] = w->link[1];
                  w->link[1] = y;
                  if (w->balance == -1)
		    {
                      x->balance = 0;
		      y->balance = +1;
		    }
                  else if (w->balance == 0)
		    {
                      x->balance = y->balance = 0;
		    }
                  else /* |w->pavl_balance == +1| */
		    {
                      x->balance = -1;
		      y->balance = 0;
		    }
                  w->balance = 0;
                  w->parent = y->parent;
                  x->parent = y->parent = w;
                  if (x->link[1] != NULL)
		    {
                      x->link[1]->parent = x;
		    }
                  if (y->link[0] != NULL)
		    {
                      y->link[0]->parent = y;
		    }
                  q->link[dir] = w;
                }
              else
                {
                  y->link[0] = x->link[1];
                  x->link[1] = y;
                  x->parent = y->parent;
                  y->parent = x;
                  if (y->link[0] != NULL)
		    {
                      y->link[0]->parent = y;
		    }
                  q->link[dir] = x;
                  if (x->balance == 0)
                    {
                      x->balance = +1;
                      y->balance = -1;
                      break;
                    }
                  else
                    {
                      x->balance = y->balance = 0;
                      y = x;
                    }
                }
            }
        }
    }

}

PNODE avl_get_next(PNODE root, PNODE p)
{
  PNODE q;
  if (p->link[1])
    {
      p = p->link[1];
      while(p->link[0])
        {
	  p = p->link[0];
	}
      return p;
    }
  else
    {
      q = p->parent;
      while (q && q->link[1] == p)
        {
	  p = q;
	  q = q->parent;
	}
      if (q == NULL)
        {
	  return NULL;
	}
      else
        {
	  return q;
	}
    }
}

PNODE avl_find_equal_or_greater(PNODE root, ULONG size, int (compare)(PVOID, PNODE))
{
  PNODE p;
  PNODE prev = NULL;
  int cmp;

  for (p = root; p != NULL;)
    {
      cmp = compare((PVOID)&size, p);
      if (cmp < 0)
        {
	  prev = p;
	  p = p->link[0];
	}
      else if (cmp > 0)
        {
          p = p->link[1];
	}
      else
        {
	  while (p->link[0])
	    {
              cmp = compare((PVOID)&size, p->link[0]);
	      if (cmp != 0)
	        {
		  break;
		}
	      p = p->link[0];
	    }
	  return p;
	}
    }
  return prev;
}

/* non paged pool functions ************************************************/

#ifdef TAG_STATISTICS_TRACKING
VOID
MiRemoveFromTagHashTable(BLOCK_HDR* block)
     /*
      * Remove a block from the tag hash table
      */
{
  if (block->Tag == 0)
    {
      return;
    }

  RemoveEntryList(&block->TagListEntry);
}

VOID
MiAddToTagHashTable(BLOCK_HDR* block)
     /*
      * Add a block to the tag hash table
      */
{
  ULONG hash;

  if (block->Tag == 0)
    {
      return;
    }

  hash = block->Tag % TAG_HASH_TABLE_SIZE;

  InsertHeadList(&tag_hash_table[hash], &block->TagListEntry);
}
#endif /* TAG_STATISTICS_TRACKING */

VOID 
MiInitializeNonPagedPool(VOID)
{
#ifdef TAG_STATISTICS_TRACKING
  ULONG i;
  for (i = 0; i < TAG_HASH_TABLE_SIZE; i++)
    {
      InitializeListHead(&tag_hash_table[i]);
    }
#endif
   MiCurrentNonPagedPoolLength = 0;
   KeInitializeSpinLock(&MmNpoolLock);
   InitializeListHead(&UsedBlockListHead);
   InitializeListHead(&AddressListHead);
   FreeBlockListRoot = NULL;
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
  LIST_ENTRY tmpListHead;
  PLIST_ENTRY current_entry;

  DbgPrint("******* Dumping non paging pool stats ******\n");
  TotalBlocks = 0;
  TotalSize = 0;
  for (i = 0; i < TAG_HASH_TABLE_SIZE; i++)
    {
      InitializeListHead(&tmpListHead);

      while (!IsListEmpty(&tag_hash_table[i]))
        {
	  CurrentTag = 0;

          current_entry = tag_hash_table[i].Flink;
          while (current_entry != &tag_hash_table[i])
	    {
              current = CONTAINING_RECORD(current_entry, BLOCK_HDR, TagListEntry);
	      current_entry = current_entry->Flink;
	      if (CurrentTag == 0)
	        {
		  CurrentTag = current->Tag;
		  CurrentNrBlocks = 0;
		  CurrentSize = 0;
		}
	      if (current->Tag == CurrentTag)
	        {
	          RemoveEntryList(&current->TagListEntry);
		  InsertHeadList(&tmpListHead, &current->TagListEntry);
		  if (!NewOnly || !current->Dumped)
		    {
		      CurrentNrBlocks++;
		      TotalBlocks++;
		      CurrentSize += current->Size;
		      TotalSize += current->Size;
		      current->Dumped = TRUE;
		    }
		}
	    }
	  if (CurrentTag != 0 && CurrentNrBlocks != 0)
	    {
	      MiDumpTagStats(CurrentTag, CurrentNrBlocks, CurrentSize);
	    }
	}
      if (!IsListEmpty(&tmpListHead))
        {
          tag_hash_table[i].Flink = tmpListHead.Flink;
          tag_hash_table[i].Flink->Blink = &tag_hash_table[i];
	  tag_hash_table[i].Blink = tmpListHead.Blink;
          tag_hash_table[i].Blink->Flink = &tag_hash_table[i];
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
  DbgPrint("Freeblocks %d TotalFreeSize %d AverageFreeSize %d\n", 
	  EiNrFreeBlocks, EiFreeNonPagedPool, EiNrFreeBlocks ? EiFreeNonPagedPool / EiNrFreeBlocks : 0);
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
   
   current_entry = MiFreeBlockListHead.Flink;
   while (current_entry != &MiFreeBlockListHead)
     {
	PVOID base_addr;

	current = CONTAINING_RECORD(current_entry, BLOCK_HDR, ListEntry);
	base_addr = (PVOID)current;

	if (current->Magic != BLOCK_HDR_FREE_MAGIC)
	  {
	     DbgPrint("Bad block magic (probable pool corruption) at %x\n",
		      current);
	     KeBugCheck(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
	  }
	
	if (base_addr < MiNonPagedPoolStart ||
	    MiNonPagedPoolStart + current->Size > MiNonPagedPoolStart + MiCurrentNonPagedPoolLength)
	  {		       
	     DbgPrint("Block %x found outside pool area\n",current);
	     DbgPrint("Size %d\n",current->Size);
	     DbgPrint("Limits are %x %x\n",MiNonPagedPoolStart,
		      MiNonPagedPoolStart+MiCurrentNonPagedPoolLength);
	     KeBugCheck(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
	  }
	blocks_seen++;
	if (blocks_seen > MiNrFreeBlocks)
	  {
	     DbgPrint("Too many blocks on free list\n");
	     KeBugCheck(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
	  }
	if (current->ListEntry.Flink != &MiFreeBlockListHead &&
	    current->ListEntry.Flink->Blink != &current->ListEntry)
	  {
	     DbgPrint("%s:%d:Break in list (current %x next %x "
		      "current->next->previous %x)\n",
		      __FILE__,__LINE__,current, current->ListEntry.Flink,
		      current->ListEntry.Flink->Blink);
	     KeBugCheck(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
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
	PVOID base_addr;

	current = CONTAINING_RECORD(current_entry, BLOCK_HDR, ListEntry);
	base_addr = (PVOID)current;
	
	if (current->Magic != BLOCK_HDR_USED_MAGIC)
	  {
	     DbgPrint("Bad block magic (probable pool corruption) at %x\n",
		      current);
	     KeBugCheck(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
	  }
	if (base_addr < MiNonPagedPoolStart ||
	    (base_addr+current->Size) >
	    MiNonPagedPoolStart+MiCurrentNonPagedPoolLength)
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
   
   current_entry = MiFreeBlockListHead.Flink;
   while (current_entry != &MiFreeBlockListHead)
     {
       current = CONTAINING_RECORD(current_entry, BLOCK_HDR, ListEntry);

       if (current->Magic != BLOCK_HDR_FREE_MAGIC)
	 {
	   DbgPrint("Bad block magic (probable pool corruption) at %x\n",
		    current);
	   KeBugCheck(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
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

   current_entry = MiFreeBlockListHead.Flink;
   while (current_entry != &MiFreeBlockListHead)
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

static void remove_from_used_list(BLOCK_HDR* current)
{
  RemoveEntryList(&current->ListEntry);
  EiUsedNonPagedPool -= current->Size;
  EiNrUsedBlocks--;
}

static void remove_from_free_list(BLOCK_HDR* current)
{
  DPRINT("remove_from_free_list %d\n", current->Size);

  avl_remove(&FreeBlockListRoot, &current->Node, compare_node);

  EiFreeNonPagedPool -= current->Size;
  EiNrFreeBlocks--;
  DPRINT("remove_from_free_list done\n");
#ifdef DUMP_AVL
  DumpFreeBlockTree();
#endif
}

static void 
add_to_free_list(BLOCK_HDR* blk)
/*
 * FUNCTION: add the block to the free list (internal)
 */
{
  BLOCK_HDR* current;

  DPRINT("add_to_free_list (%d)\n", blk->Size);

  EiNrFreeBlocks++;

  if (blk->AddressList.Blink != &AddressListHead)
    {
      current = CONTAINING_RECORD(blk->AddressList.Blink, BLOCK_HDR, AddressList);
      if (current->Magic == BLOCK_HDR_FREE_MAGIC &&
	  (PVOID)current + current->Size + sizeof(BLOCK_HDR) == (PVOID)blk)
        {
	  CHECKPOINT;
          remove_from_free_list(current);
	  RemoveEntryList(&blk->AddressList);
	  current->Size = current->Size + sizeof(BLOCK_HDR) + blk->Size;
	  current->Magic = BLOCK_HDR_USED_MAGIC;
	  memset(blk, 0xcc, sizeof(BLOCK_HDR));
	  blk = current;
	}
    }

  if (blk->AddressList.Flink != &AddressListHead)
    {
      current = CONTAINING_RECORD(blk->AddressList.Flink, BLOCK_HDR, AddressList);
      if (current->Magic == BLOCK_HDR_FREE_MAGIC &&
	  (PVOID)blk + blk->Size + sizeof(BLOCK_HDR) == (PVOID)current)
        {
	  CHECKPOINT;
          remove_from_free_list(current);
	  RemoveEntryList(&current->AddressList);
	  blk->Size = blk->Size + sizeof(BLOCK_HDR) + current->Size;
	  memset(current, 0xcc, sizeof(BLOCK_HDR));
	}
    }

  DPRINT("%d\n", blk->Size);
  blk->Magic = BLOCK_HDR_FREE_MAGIC;
  EiFreeNonPagedPool += blk->Size;
  avl_insert(&FreeBlockListRoot, &blk->Node, compare_node);

  DPRINT("add_to_free_list done\n");
#ifdef DUMP_AVL
  DumpFreeBlockTree();
#endif
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
   BLOCK_HDR* current;
   BLOCK_HDR* best = NULL;
   ULONG new_size;
   PVOID block, block_boundary;
   PNODE p;

   DPRINT("lookup_block %d\n", size);

   if (size < PAGE_SIZE)
     {
       p = avl_find_equal_or_greater(FreeBlockListRoot, size, compare_value);
       if (p)
	 { 
	   best = CONTAINING_RECORD(p, BLOCK_HDR, Node);
	 }
     }
   else
     {
       p = avl_find_equal_or_greater(FreeBlockListRoot, size, compare_value);

       while(p)
         {
           current = CONTAINING_RECORD(p, BLOCK_HDR, Node);
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
	   if (best && current->Size >= size + PAGE_SIZE + 2 * sizeof(BLOCK_HDR))
	     {
	       break;
	     }
	   p = avl_get_next(FreeBlockListRoot, p);

	 }
     }
   DPRINT("lookup_block done %d\n", best ? best->Size : 0);
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
    BOOL Removed = FALSE;
   
    DPRINT("take_block\n");

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
	    blk->Magic = BLOCK_HDR_USED_MAGIC;
	    blk->Size = current->Size - ((ULONG)blk - (ULONG)current);
	    remove_from_free_list(current);
	    current->Size -= (blk->Size + sizeof(BLOCK_HDR));
	    blk->AddressList.Flink = current->AddressList.Flink;
	    blk->AddressList.Flink->Blink = &blk->AddressList;
	    blk->AddressList.Blink = &current->AddressList;
	    current->AddressList.Flink = &blk->AddressList;
	    add_to_free_list(current);
	    Removed = TRUE;
	    current = blk;
	  }
      }
   if (Removed == FALSE)
     {
       remove_from_free_list(current);
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

	/*
	 * Replace the bigger block with a smaller block in the
	 * same position in the list
	 */
        free_blk = (BLOCK_HDR *)(((int)current)
				 + sizeof(BLOCK_HDR) + size);		

	free_blk->Size = current->Size - (sizeof(BLOCK_HDR) + size);
	current->Size=size;
        free_blk->AddressList.Flink = current->AddressList.Flink;
	free_blk->AddressList.Flink->Blink = &free_blk->AddressList;
	free_blk->AddressList.Blink = &current->AddressList;
	current->AddressList.Flink = &free_blk->AddressList;
	current->Magic = BLOCK_HDR_USED_MAGIC;
	free_blk->Magic = BLOCK_HDR_FREE_MAGIC;
	add_to_free_list(free_blk);
	add_to_used_list(current);
	current->Tag = Tag;
	current->Caller = Caller;
	current->Dumped = FALSE;
#ifdef TAG_STATISTICS_TRACKING
	MiAddToTagHashTable(current);
#endif /* TAG_STATISTICS_TRACKING */
	VALIDATE_POOL;
	return(block_to_address(current));
     }
   
   /*
    * Otherwise allocate the whole block
    */

   current->Magic = BLOCK_HDR_USED_MAGIC;   
   current->Tag = Tag;
   current->Caller = Caller;
   current->Dumped = FALSE;
   add_to_used_list(current);
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
   BLOCK_HDR* current;
   ULONG i;
   KIRQL oldIrql;
   NTSTATUS Status;
   PVOID block = NULL;
   PLIST_ENTRY current_entry;

   if (size >= PAGE_SIZE)
     {
       nr_pages ++;
     }

   KeAcquireSpinLock(&MmNpoolLock, &oldIrql);
   start = (ULONG)MiNonPagedPoolStart + MiCurrentNonPagedPoolLength;
   if (MiCurrentNonPagedPoolLength + nr_pages * PAGE_SIZE > MiNonPagedPoolLength)
     {
       DbgPrint("CRITICAL: Out of non-paged pool space\n");
       KeBugCheck(0);
     }
   MiCurrentNonPagedPoolLength += nr_pages * PAGE_SIZE;
   KeReleaseSpinLock(&MmNpoolLock, oldIrql);

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
				       TRUE);
	if (!NT_SUCCESS(Status))
	  {
	     DbgPrint("Unable to create virtual mapping\n");
	     KeBugCheck(0);
	  }
     }

   blk = (struct _BLOCK_HDR *)start;
   memset(blk, 0, sizeof(BLOCK_HDR));
   blk->Size = (nr_pages * PAGE_SIZE) - sizeof(BLOCK_HDR);
   memset(block_to_address(blk), 0xcc, blk->Size);

   KeAcquireSpinLock(&MmNpoolLock, &oldIrql);
   current_entry = AddressListHead.Blink;
   while (current_entry != &AddressListHead)
     {
       current = CONTAINING_RECORD(current_entry, BLOCK_HDR, AddressList);
       if ((PVOID)current + current->Size < (PVOID)blk)
         {
	   InsertHeadList(current_entry, &blk->AddressList);
	   break;
	 }
       current_entry = current_entry->Blink;
     }
   if (current_entry == &AddressListHead)
     {
       InsertHeadList(&AddressListHead, &blk->AddressList);
     }
   blk->Magic = BLOCK_HDR_FREE_MAGIC;
   add_to_free_list(blk);
   blk = lookup_block(size);
   if (blk)
     {
       block = take_block(blk, size, Tag, Caller);
       VALIDATE_POOL;
     }
   KeReleaseSpinLock(&MmNpoolLock, oldIrql);
   return block;
}

#endif /* not WHOLE_PAGE_ALLOCATIONS */

/*
 * @implemented
 */
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
   blk->Tag = 0;
   blk->Caller = NULL;
   blk->TagListEntry.Flink = blk->TagListEntry.Blink = NULL;
   blk->Magic = BLOCK_HDR_FREE_MAGIC;
   add_to_free_list(blk);
   VALIDATE_POOL;
   KeReleaseSpinLock(&MmNpoolLock, oldIrql);

#endif /* WHOLE_PAGE_ALLOCATIONS */
}

/*
 * @implemented
 */
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
       if (block == NULL)
       {
         DPRINT1("%d\n", Size);
	 DumpFreeBlockTree();
       }
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

/*
 * @implemented
 */
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

/*
 * @implemented
 */
VOID STDCALL
ExFreeWholePageBlock(PVOID Addr)
{
  ULONG NrPages;
  
  if (Addr < MiNonPagedPoolStart ||
      Addr >= (MiNonPagedPoolStart + MiCurrentNonPagedPoolLength))
    {
      DbgPrint("Block %x found outside pool area\n", Addr);
      KeBugCheck(0);
    }
  NrPages = *(PULONG)((ULONG)Addr - sizeof(ULONG));
  MiFreeNonPagedPoolRegion((PVOID)PAGE_ROUND_DOWN((ULONG)Addr), NrPages, TRUE);
}

#endif /* WHOLE_PAGE_ALLOCATIONS */

/* EOF */
