/* $Id: npool.c,v 1.81 2004/02/07 16:37:23 hbirr Exp $
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
/*#define ENABLE_VALIDATE_POOL*/

/* Enable tracking of statistics about the tagged blocks in the pool */
#define TAG_STATISTICS_TRACKING

/* 
 * Put each block in its own range of pages and position the block at the
 * end of the range so any accesses beyond the end of block are to invalid
 * memory locations. 
 */
/*#define WHOLE_PAGE_ALLOCATIONS*/

#ifdef ENABLE_VALIDATE_POOL
#define VALIDATE_POOL validate_kernel_pool()
#else
#define VALIDATE_POOL
#endif

#if 0
#define POOL_TRACE(args...) do { DbgPrint(args); } while(0);
#else
#if defined(__GNUC__)
#define POOL_TRACE(args...)
#else
#define POOL_TRACE
#endif	/* __GNUC__ */
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
  struct _BLOCK_HDR* previous;
  union
  {
    struct
    {
      LIST_ENTRY ListEntry;
      ULONG Tag;
      PVOID Caller;
      LIST_ENTRY TagListEntry;
      BOOLEAN Dumped;
    } Used;
    struct
    {
      NODE Node;
    } Free;
  };
} BLOCK_HDR;

#define BLOCK_HDR_SIZE	ROUND_UP(sizeof(BLOCK_HDR), MM_POOL_ALIGNMENT)

PVOID STDCALL 
ExAllocateWholePageBlock(ULONG Size);
VOID STDCALL
ExFreeWholePageBlock(PVOID Addr);

/* GLOBALS *****************************************************************/

extern PVOID MiNonPagedPoolStart;
extern ULONG MiNonPagedPoolLength;

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

#ifdef WHOLE_PAGE_ALLOCATIONS
static RTL_BITMAP NonPagedPoolAllocMap;
static ULONG NonPagedPoolAllocMapHint;
static ULONG MiCurrentNonPagedPoolLength = 0;
#else
static PULONG MiNonPagedPoolAllocMap;
static ULONG MiNonPagedPoolNrOfPages;
#endif /* WHOLE_PAGE_ALLOCATIONS */

/* avl helper functions ****************************************************/

void DumpFreeBlockNode(PNODE p)
{
  static int count = 0;
  BLOCK_HDR* blk;

  count++;

  if (p)
    {
      DumpFreeBlockNode(p->link[0]);
      blk = CONTAINING_RECORD(p, BLOCK_HDR, Free.Node);
      DbgPrint("%08x %8d (%d)\n", blk, blk->Size, count);
      DumpFreeBlockNode(p->link[1]);
    }

  count--;
}
void DumpFreeBlockTree(void)
{
  DbgPrint("--- Begin tree ------------------\n");
  DbgPrint("%08x\n", CONTAINING_RECORD(FreeBlockListRoot, BLOCK_HDR, Free.Node));
  DumpFreeBlockNode(FreeBlockListRoot);
  DbgPrint("--- End tree --------------------\n");
}

int compare_node(PNODE p1, PNODE p2)
{
  BLOCK_HDR* blk1 = CONTAINING_RECORD(p1, BLOCK_HDR, Free.Node);
  BLOCK_HDR* blk2 = CONTAINING_RECORD(p2, BLOCK_HDR, Free.Node);

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
  BLOCK_HDR* blk = CONTAINING_RECORD(p, BLOCK_HDR, Free.Node);
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
  int dir = 0; /* Direction to descend. */

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

PNODE _cdecl avl_get_first(PNODE root)
{
  PNODE p;
  if (root == NULL)
    {
      return NULL;
    }
  p = root;
  while (p->link[0])
    {
      p = p->link[0];
    }
  return p;
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
  if (block->Used.Tag == 0)
    {
      return;
    }

  RemoveEntryList(&block->Used.TagListEntry);
}

VOID
MiAddToTagHashTable(BLOCK_HDR* block)
     /*
      * Add a block to the tag hash table
      */
{
  ULONG hash;

  if (block->Used.Tag == 0)
    {
      return;
    }

  hash = block->Used.Tag % TAG_HASH_TABLE_SIZE;

  InsertHeadList(&tag_hash_table[hash], &block->Used.TagListEntry);
}
#endif /* TAG_STATISTICS_TRACKING */

#if defined(TAG_STATISTICS_TRACKING) && !defined(WHOLE_PAGE_ALLOCATIONS)
VOID STATIC
MiDumpTagStats(ULONG CurrentTag, ULONG CurrentNrBlocks, ULONG CurrentSize)
{
  CHAR c1, c2, c3, c4;
  
  c1 = (CHAR)((CurrentTag >> 24) & 0xFF);
  c2 = (CHAR)((CurrentTag >> 16) & 0xFF);
  c3 = (CHAR)((CurrentTag >> 8) & 0xFF);
  c4 = (CHAR)(CurrentTag & 0xFF);
  
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
#endif /* defined(TAG_STATISTICS_TRACKING) && !defined(WHOLE_PAGE_ALLOCATIONS); */

VOID
MiDebugDumpNonPagedPoolStats(BOOLEAN NewOnly)
{
#if defined(TAG_STATISTICS_TRACKING) && !defined(WHOLE_PAGE_ALLOCATIONS)
  ULONG i;
  BLOCK_HDR* current;
  ULONG CurrentTag;
  ULONG CurrentNrBlocks = 0;
  ULONG CurrentSize = 0;
  ULONG TotalBlocks;
  ULONG TotalSize;
  ULONG Size;
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
              current = CONTAINING_RECORD(current_entry, BLOCK_HDR, Used.TagListEntry);
	      current_entry = current_entry->Flink;
	      if (CurrentTag == 0)
	        {
		  CurrentTag = current->Used.Tag;
		  CurrentNrBlocks = 0;
		  CurrentSize = 0;
		}
	      if (current->Used.Tag == CurrentTag)
	        {
	          RemoveEntryList(&current->Used.TagListEntry);
		  InsertHeadList(&tmpListHead, &current->Used.TagListEntry);
		  if (!NewOnly || !current->Used.Dumped)
		    {
		      CurrentNrBlocks++;
		      TotalBlocks++;
		      CurrentSize += current->Size;
		      TotalSize += current->Size;
		      current->Used.Dumped = TRUE;
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
  Size = EiFreeNonPagedPool - (MiNonPagedPoolLength - MiNonPagedPoolNrOfPages * PAGE_SIZE);
  DbgPrint("Freeblocks %d TotalFreeSize %d AverageFreeSize %d\n", 
	  EiNrFreeBlocks, Size, EiNrFreeBlocks ? Size / EiNrFreeBlocks : 0);
  DbgPrint("***************** Dump Complete ***************\n");
#endif /* defined(TAG_STATISTICS_TRACKING) && !defined(WHOLE_PAGE_ALLOCATIONS) */
}

VOID
MiDebugDumpNonPagedPool(BOOLEAN NewOnly)
{
#ifndef WHOLE_PAGE_ALLOCATIONS
   BLOCK_HDR* current;
   PLIST_ENTRY current_entry;
   KIRQL oldIrql;
   
   KeAcquireSpinLock(&MmNpoolLock, &oldIrql);

   DbgPrint("******* Dumping non paging pool contents ******\n");
   current_entry = UsedBlockListHead.Flink;
   while (current_entry != &UsedBlockListHead)
     {
       current = CONTAINING_RECORD(current_entry, BLOCK_HDR, Used.ListEntry);
       if (!NewOnly || !current->Used.Dumped)
	 {
	   CHAR c1, c2, c3, c4;

	   c1 = (CHAR)((current->Used.Tag >> 24) & 0xFF);
	   c2 = (CHAR)((current->Used.Tag >> 16) & 0xFF);
	   c3 = (CHAR)((current->Used.Tag >> 8) & 0xFF);
	   c4 = (CHAR)(current->Used.Tag & 0xFF);

	   if (isprint(c1) && isprint(c2) && isprint(c3) && isprint(c4))
	     {
	       DbgPrint("Size 0x%x Tag 0x%x (%c%c%c%c) Allocator 0x%x\n",
			current->Size, current->Used.Tag, c4, c3, c2, c1, 
			current->Used.Caller);
	     }
	   else
	     {
	       DbgPrint("Size 0x%x Tag 0x%x Allocator 0x%x\n",
			current->Size, current->Used.Tag, current->Used.Caller);
	     }
	   current->Used.Dumped = TRUE;
	 }
       current_entry = current_entry->Flink;
     }
   DbgPrint("***************** Dump Complete ***************\n");
   KeReleaseSpinLock(&MmNpoolLock, oldIrql);
#endif /* not WHOLE_PAGE_ALLOCATIONS */
}

#ifndef WHOLE_PAGE_ALLOCATIONS

#ifdef ENABLE_VALIDATE_POOL
static void validate_free_list(void)
/*
 * FUNCTION: Validate the integrity of the list of free blocks
 */
{
   BLOCK_HDR* current;
   unsigned int blocks_seen=0;     
   PNODE p;

   p = avl_get_first(FreeBlockListRoot);

   while(p)
     {
	PVOID base_addr;

	current = CONTAINING_RECORD(p, BLOCK_HDR, Free.Node);
	base_addr = (PVOID)current;

	if (current->Magic != BLOCK_HDR_FREE_MAGIC)
	  {
	     DbgPrint("Bad block magic (probable pool corruption) at %x\n",
		      current);
	     KEBUGCHECK(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
	  }
	
	if (base_addr < MiNonPagedPoolStart ||
	    base_addr + BLOCK_HDR_SIZE + current->Size > MiNonPagedPoolStart + MiNonPagedPoolLength)
	  {		       
	     DbgPrint("Block %x found outside pool area\n",current);
	     DbgPrint("Size %d\n",current->Size);
	     DbgPrint("Limits are %x %x\n",MiNonPagedPoolStart,
		      MiNonPagedPoolStart+MiNonPagedPoolLength);
	     KEBUGCHECK(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
	  }
	blocks_seen++;
	if (blocks_seen > EiNrFreeBlocks)
	  {
	     DbgPrint("Too many blocks on free list\n");
	     KEBUGCHECK(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
	  }
	p = avl_get_next(FreeBlockListRoot, p);
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

	current = CONTAINING_RECORD(current_entry, BLOCK_HDR, Used.ListEntry);
	base_addr = (PVOID)current;
	
	if (current->Magic != BLOCK_HDR_USED_MAGIC)
	  {
	     DbgPrint("Bad block magic (probable pool corruption) at %x\n",
		      current);
	     KEBUGCHECK(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
	  }
	if (base_addr < MiNonPagedPoolStart ||
	    (base_addr+BLOCK_HDR_SIZE+current->Size) >
	    MiNonPagedPoolStart+MiNonPagedPoolLength)
	  {
	     DbgPrint("Block %x found outside pool area\n",current);
	     DbgPrint("Size %d\n",current->Size);
	     DbgPrint("Limits are %x %x\n",MiNonPagedPoolStart,
		      MiNonPagedPoolStart+MiNonPagedPoolLength);
	     KEBUGCHECK(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
	  }
	blocks_seen++;
	if (blocks_seen > EiNrUsedBlocks)
	  {
	     DbgPrint("Too many blocks on used list\n");
	     KEBUGCHECK(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
	  }
	if (current->Used.ListEntry.Flink != &UsedBlockListHead &&
	    current->Used.ListEntry.Flink->Blink != &current->Used.ListEntry)
	  {
	     DbgPrint("%s:%d:Break in list (current %x next %x "
		      "current->next->previous %x)\n",
		      __FILE__,__LINE__,current, current->Used.ListEntry.Flink,
		      current->Used.ListEntry.Flink->Blink);
	     KEBUGCHECK(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
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
   char* base = (char*)blk;
   char* last = ((char*)blk) + BLOCK_HDR_SIZE + blk->Size;
   BLOCK_HDR* current;
   PLIST_ENTRY current_entry;
   PNODE p;

   p = avl_get_first(FreeBlockListRoot);

   while (p)
     {
       current = CONTAINING_RECORD(p, BLOCK_HDR, Free.Node);

       if (current->Magic != BLOCK_HDR_FREE_MAGIC)
	 {
	   DbgPrint("Bad block magic (probable pool corruption) at %x\n",
		    current);
	   KEBUGCHECK(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
	 }
       
       if ( (char*)current > base && (char*)current < last ) 
	 {
	   DbgPrint("intersecting blocks on list\n");
	   KEBUGCHECK(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
	 }
       if  ( (char*)current < base &&
	     ((char*)current + current->Size + BLOCK_HDR_SIZE)
	     > base )
	 {
	   DbgPrint("intersecting blocks on list\n");
	   KEBUGCHECK(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
	  }
       p = avl_get_next(FreeBlockListRoot, p);
     }

   current_entry = UsedBlockListHead.Flink;
   while (current_entry != &UsedBlockListHead)
     {
       current = CONTAINING_RECORD(current_entry, BLOCK_HDR, Used.ListEntry);

       if ( (char*)current > base && (char*)current < last ) 
	 {
	   DbgPrint("intersecting blocks on list\n");
	   KEBUGCHECK(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
	 }
       if  ( (char*)current < base &&
	     ((char*)current + current->Size + BLOCK_HDR_SIZE)
	     > base )
	 {
	   DbgPrint("intersecting blocks on list\n");
	   KEBUGCHECK(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
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
   PNODE p;
   
   validate_free_list();
   validate_used_list();

   p = avl_get_first(FreeBlockListRoot);
   while (p)
     {
       current = CONTAINING_RECORD(p, BLOCK_HDR, Free.Node);
       check_duplicates(current);
       p = avl_get_next(FreeBlockListRoot, p);
     }
   current_entry = UsedBlockListHead.Flink;
   while (current_entry != &UsedBlockListHead)
     {
       current = CONTAINING_RECORD(current_entry, BLOCK_HDR, Used.ListEntry);
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
  end = (ULONG)blk + BLOCK_HDR_SIZE + blk->Size;

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
  RemoveEntryList(&current->Used.ListEntry);
  EiUsedNonPagedPool -= current->Size;
  EiNrUsedBlocks--;
}

static void remove_from_free_list(BLOCK_HDR* current)
{
  DPRINT("remove_from_free_list %d\n", current->Size);

  avl_remove(&FreeBlockListRoot, &current->Free.Node, compare_node);

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
  BOOL UpdatePrevPtr = FALSE;

  DPRINT("add_to_free_list (%d)\n", blk->Size);

  EiNrFreeBlocks++;
  
  current = blk->previous;
  if (current && current->Magic == BLOCK_HDR_FREE_MAGIC)
    {
      remove_from_free_list(current);
      current->Size = current->Size + BLOCK_HDR_SIZE + blk->Size;
      current->Magic = BLOCK_HDR_USED_MAGIC;
      memset(blk, 0xcc, BLOCK_HDR_SIZE);
      blk = current;
      UpdatePrevPtr = TRUE;
    }

  current = (BLOCK_HDR*)((char*)blk + BLOCK_HDR_SIZE + blk->Size);
  if ((char*)current < (char*)MiNonPagedPoolStart + MiNonPagedPoolLength &&
      current->Magic == BLOCK_HDR_FREE_MAGIC)
    {
      remove_from_free_list(current);
      blk->Size += BLOCK_HDR_SIZE + current->Size;
      memset(current, 0xcc, BLOCK_HDR_SIZE);
      UpdatePrevPtr = TRUE;
      current = (BLOCK_HDR*)((char*)blk + BLOCK_HDR_SIZE + blk->Size);
    }
  if (UpdatePrevPtr &&
      (char*)current < (char*)MiNonPagedPoolStart + MiNonPagedPoolLength)
    {
      current->previous = blk;
    }
  DPRINT("%d\n", blk->Size);
  blk->Magic = BLOCK_HDR_FREE_MAGIC;
  EiFreeNonPagedPool += blk->Size;
  avl_insert(&FreeBlockListRoot, &blk->Free.Node, compare_node);

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
  InsertHeadList(&UsedBlockListHead, &blk->Used.ListEntry);
  EiUsedNonPagedPool += blk->Size;
  EiNrUsedBlocks++;
}

inline static void* block_to_address(BLOCK_HDR* blk)
/*
 * FUNCTION: Translate a block header address to the corresponding block
 * address (internal)
 */
{
        return ( (void *) ((char*)blk + BLOCK_HDR_SIZE));
}

inline static BLOCK_HDR* address_to_block(void* addr)
{
        return (BLOCK_HDR *)
               ( ((char*)addr) - BLOCK_HDR_SIZE );
}

static BOOLEAN
grow_block(BLOCK_HDR* blk, PVOID end)
{
   NTSTATUS Status;
   PHYSICAL_ADDRESS Page;
   BOOLEAN result = TRUE;
   ULONG index;

   PVOID start = (PVOID)PAGE_ROUND_UP((ULONG)((char*)blk + BLOCK_HDR_SIZE));
   end = (PVOID)PAGE_ROUND_UP(end);
   index = (ULONG)((char*)start - (char*)MiNonPagedPoolStart) / PAGE_SIZE;
   while (start < end)
     {
       if (!(MiNonPagedPoolAllocMap[index / 32] & (1 << (index % 32))))
         {
           Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Page);
           if (!NT_SUCCESS(Status))
             {
	       result = FALSE;
	       break;
	     }
           Status = MmCreateVirtualMapping(NULL,
				           start,
				           PAGE_READWRITE|PAGE_SYSTEM,
				           Page,
				           FALSE);
           if (!NT_SUCCESS(Status))
             {
               DbgPrint("Unable to create virtual mapping\n");
	       MmReleasePageMemoryConsumer(MC_NPPOOL, Page);
	       result = FALSE;
	       break;
	     }
	   MiNonPagedPoolAllocMap[index / 32] |= (1 << (index % 32));
	   memset(start, 0xcc, PAGE_SIZE);
	   MiNonPagedPoolNrOfPages++;
	 }
       index++;
#if defined(__GNUC__)
       start += PAGE_SIZE;
#else
       {
	 char* pTemp = start;
	 pTemp += PAGE_SIZE;
	 start = pTemp;
       }
#endif
     }
   return result;
}

static BLOCK_HDR* get_block(unsigned int size, unsigned long alignment)
{
   BLOCK_HDR *blk, *current, *previous = NULL, *next = NULL, *best = NULL;
   ULONG previous_size = 0, current_size, next_size = 0, new_size;
   PVOID end;
   PVOID addr, aligned_addr;
   PNODE p;
 
   DPRINT("get_block %d\n", size);

   if (alignment == 0)
     {
       p = avl_find_equal_or_greater(FreeBlockListRoot, size, compare_value);
       if (p)
	 { 
	   best = CONTAINING_RECORD(p, BLOCK_HDR, Free.Node);
	   addr = block_to_address(best);
	 }
     }
   else
     {
       p = avl_find_equal_or_greater(FreeBlockListRoot, size, compare_value);

       while(p)
         {
           current = CONTAINING_RECORD(p, BLOCK_HDR, Free.Node);
           addr = block_to_address(current);
	   /* calculate first aligned address available within this block */
	   aligned_addr = MM_ROUND_UP(addr, alignment);
   	   /* check to see if this address is already aligned */
	   if (addr == aligned_addr)
	     {
               if (current->Size >= size && 
	         (best == NULL || current->Size < best->Size))
               {
                 best = current;
               }
	     }
	   else
	     {
	       /* make sure there's enough room to make a free block by the space skipped
	        * from alignment. If not, calculate forward to the next alignment
	        * and see if we allocate there...
	        */
               new_size = (ULONG)aligned_addr - (ULONG)addr + size;
               if ((ULONG)aligned_addr - (ULONG)addr < BLOCK_HDR_SIZE)
 	         {
		   /* not enough room for a free block header, add some more bytes */
	           aligned_addr = MM_ROUND_UP(block_to_address((BLOCK_HDR*)((char*)current + BLOCK_HDR_SIZE)), alignment);
                   new_size = (ULONG)aligned_addr - (ULONG)addr + size;
                 }
               if (current->Size >= new_size && 
	         (best == NULL || current->Size < best->Size))
                 {
                   best = current;
                 }
	     }
	   if (best && current->Size >= size + alignment + 2 * BLOCK_HDR_SIZE)
	     {
	       break;
	     }
	   p = avl_get_next(FreeBlockListRoot, p);

	 }
     }

   /*
    * We didn't find anything suitable at all.
    */
   if (best == NULL)
     {
       return NULL;
     }

   current = best;
   current_size = current->Size;

   if (alignment > 0)
     {
       addr = block_to_address(current);
       aligned_addr = MM_ROUND_UP(addr, alignment);
       if (addr != aligned_addr)
         {
           blk = address_to_block(aligned_addr);
           if ((char*)blk < (char*)current + BLOCK_HDR_SIZE)
             {
	       aligned_addr = MM_ROUND_UP(block_to_address((BLOCK_HDR*)((char*)current + BLOCK_HDR_SIZE)), alignment);
               blk = address_to_block(aligned_addr);
	     }
           /*
            * if size-aligned, break off the preceding bytes into their own block...
            */
           previous = current;
	   previous_size = (ULONG)blk - (ULONG)previous - BLOCK_HDR_SIZE;
	   current = blk;
	   current_size -= ((ULONG)current - (ULONG)previous);
	 }
     }

   end = (char*)current + BLOCK_HDR_SIZE + size;

   if (current_size >= size + BLOCK_HDR_SIZE + MM_POOL_ALIGNMENT)
     {
       /* create a new free block after our block, if the memory size is >= 4 byte for this block */
       next = (BLOCK_HDR*)((ULONG)current + size + BLOCK_HDR_SIZE);
       next_size = current_size - size - BLOCK_HDR_SIZE;
       current_size = size;
       end = (char*)next + BLOCK_HDR_SIZE;
     }

   if (previous)
     {
       remove_from_free_list(previous);
       if (!grow_block(previous, end))
         {
	   add_to_free_list(previous);
	   return NULL;
	 }
       memset(current, 0, BLOCK_HDR_SIZE);
       current->Size = current_size;
       current->Magic = BLOCK_HDR_USED_MAGIC;
       current->previous = previous;
       previous->Size = previous_size;
       if (next == NULL)
         {
	   blk = (BLOCK_HDR*)((char*)current + BLOCK_HDR_SIZE + current->Size);
	   if ((char*)blk < (char*)MiNonPagedPoolStart + MiNonPagedPoolLength)
	     {
	       blk->previous = current;
	     }
	 }

       add_to_free_list(previous);
     }
   else
     {
       remove_from_free_list(current);

       if (!grow_block(current, end))
         {
	   add_to_free_list(current);
	   return NULL;
	 }

       current->Magic = BLOCK_HDR_USED_MAGIC;
       if (next)
         {
	   current->Size = current_size;
	 }
     }
        
   if (next)
     {
       memset(next, 0, BLOCK_HDR_SIZE);

       next->Size = next_size;
       next->Magic = BLOCK_HDR_FREE_MAGIC;
       next->previous = current;
       blk = (BLOCK_HDR*)((char*)next + BLOCK_HDR_SIZE + next->Size);
       if ((char*)blk < (char*)MiNonPagedPoolStart + MiNonPagedPoolLength)
         {
	   blk->previous = next;
	 }
       add_to_free_list(next);
     }

   add_to_used_list(current);
   VALIDATE_POOL;
   return current;
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
   
   POOL_TRACE("ExFreePool(block %x), size %d, caller %x\n",block,blk->Size,
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
	KEBUGCHECK(0);
	return;
     }
   
   memset(block, 0xcc, blk->Size);
#ifdef TAG_STATISTICS_TRACKING
   MiRemoveFromTagHashTable(blk);
#endif
   remove_from_used_list(blk);
   blk->Magic = BLOCK_HDR_FREE_MAGIC;
   add_to_free_list(blk);
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
   if (NULL == block)
     {
        DPRINT1("Trying to allocate %lu bytes from nonpaged pool - nothing suitable found, returning NULL\n",
                Size );
     }
   return(block);

#else /* not WHOLE_PAGE_ALLOCATIONS */
   PVOID block;
   BLOCK_HDR* best = NULL;
   KIRQL oldIrql;
   ULONG alignment;
   
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
   Size = ROUND_UP(Size, MM_POOL_ALIGNMENT);

   if (Size >= PAGE_SIZE)
     {
       alignment = PAGE_SIZE;
     }
   else if (Type == NonPagedPoolCacheAligned ||
            Type == NonPagedPoolCacheAlignedMustS)
     {
       alignment = MM_CACHE_LINE_SIZE;
     }
   else
     {
       alignment = 0;
     }

   best = get_block(Size, alignment);
   if (best == NULL)
     {
      KeReleaseSpinLock(&MmNpoolLock, oldIrql);
      DPRINT1("Trying to allocate %lu bytes from nonpaged pool - nothing suitable found, returning NULL\n",
              Size );
      return NULL;
     }
   best->Used.Tag = Tag;
   best->Used.Caller = Caller;
   best->Used.Dumped = FALSE;
   best->Used.TagListEntry.Flink = best->Used.TagListEntry.Blink = NULL;
#ifdef TAG_STATISTICS_TRACKING
   MiAddToTagHashTable(best);
#endif
   KeReleaseSpinLock(&MmNpoolLock, oldIrql);
   block = block_to_address(best);
   memset(block,0,Size);
   return(block);
#endif /* WHOLE_PAGE_ALLOCATIONS */
}

#ifdef WHOLE_PAGE_ALLOCATIONS

PVOID STDCALL 
ExAllocateWholePageBlock(ULONG Size)
{
  PVOID Address;
  PHYSICAL_ADDRESS Page;
  ULONG i;
  ULONG NrPages;
  ULONG Base;

  NrPages = ROUND_UP(Size, PAGE_SIZE) / PAGE_SIZE;

  Base = RtlFindClearBitsAndSet(&NonPagedPoolAllocMap, NrPages + 1, NonPagedPoolAllocMapHint);
  if (Base == 0xffffffff)
    {
      DbgPrint("Out of non paged pool space.\n");
      KEBUGCHECK(0);
    }
  if (NonPagedPoolAllocMapHint == Base)
    {
      NonPagedPoolAllocMapHint += (NrPages + 1);
    }
  Address = MiNonPagedPoolStart + Base * PAGE_SIZE;

  for (i = 0; i < NrPages; i++)
    {
      Page = MmAllocPage(MC_NPPOOL, 0);
      if (Page.QuadPart == 0LL)
	{
	  KEBUGCHECK(0);
	}
      MmCreateVirtualMapping(NULL, 
			     Address + (i * PAGE_SIZE),
			     PAGE_READWRITE | PAGE_SYSTEM,
			     Page,
			     TRUE);
    }

  MiCurrentNonPagedPoolLength = max(MiCurrentNonPagedPoolLength, (Base + NrPages) * PAGE_SIZE);
  return((PVOID)((PUCHAR)Address + (NrPages * PAGE_SIZE) - Size));
}

VOID STDCALL
ExFreeWholePageBlock(PVOID Addr)
{
  ULONG Base;
  
  if (Addr < MiNonPagedPoolStart ||
      Addr >= (MiNonPagedPoolStart + MiCurrentNonPagedPoolLength))
    {
      DbgPrint("Block %x found outside pool area\n", Addr);
      KEBUGCHECK(0);
    }
  Base = (Addr - MiNonPagedPoolStart) / PAGE_SIZE;
  NonPagedPoolAllocMapHint = min(NonPagedPoolAllocMapHint, Base);
  while (MmIsPagePresent(NULL, Addr))
    {
      MmDeleteVirtualMapping(NULL,
			     Addr,
			     TRUE,
			     NULL,
			     NULL);
      RtlClearBits(&NonPagedPoolAllocMap, Base, 1);
      Base++;
      Addr += PAGE_SIZE;
    }  
}

#endif /* WHOLE_PAGE_ALLOCATIONS */

VOID INIT_FUNCTION
MiInitializeNonPagedPool(VOID)
{
  NTSTATUS Status;
  PHYSICAL_ADDRESS Page;
  ULONG i;
  PVOID Address;
#ifdef WHOLE_PAGE_ALLOCATIONS
#else
  BLOCK_HDR* blk;
#endif
#ifdef TAG_STATISTICS_TRACKING
  for (i = 0; i < TAG_HASH_TABLE_SIZE; i++)
    {
      InitializeListHead(&tag_hash_table[i]);
    }
#endif
   KeInitializeSpinLock(&MmNpoolLock);
   InitializeListHead(&UsedBlockListHead);
   InitializeListHead(&AddressListHead);
   FreeBlockListRoot = NULL;
#ifdef WHOLE_PAGE_ALLOCATIONS
   NonPagedPoolAllocMapHint = PAGE_ROUND_UP(MiNonPagedPoolLength / PAGE_SIZE / 8) / PAGE_SIZE;
   MiCurrentNonPagedPoolLength = NonPagedPoolAllocMapHint * PAGE_SIZE;
   Address = MiNonPagedPoolStart;
   for (i = 0; i < NonPagedPoolAllocMapHint; i++)
     {
       Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Page);
       if (!NT_SUCCESS(Status))
         {
           DbgPrint("Unable to allocate a page\n");
           KEBUGCHECK(0);
         }
       Status = MmCreateVirtualMapping(NULL,
			               Address,
				       PAGE_READWRITE|PAGE_SYSTEM,
				       Page,
				       FALSE);
       if (!NT_SUCCESS(Status))
         {
           DbgPrint("Unable to create virtual mapping\n");
           KEBUGCHECK(0);
         }
       Address += PAGE_SIZE;
     }
   RtlInitializeBitMap(&NonPagedPoolAllocMap, MiNonPagedPoolStart, MM_NONPAGED_POOL_SIZE / PAGE_SIZE);
   RtlClearAllBits(&NonPagedPoolAllocMap);  
   RtlSetBits(&NonPagedPoolAllocMap, 0, NonPagedPoolAllocMapHint);
#else
   MiNonPagedPoolAllocMap = block_to_address((BLOCK_HDR*)MiNonPagedPoolStart);
   MiNonPagedPoolNrOfPages = PAGE_ROUND_UP(ROUND_UP(MiNonPagedPoolLength / PAGE_SIZE, 32) / 8 + 2 * BLOCK_HDR_SIZE);
   MiNonPagedPoolNrOfPages /= PAGE_SIZE;
   Address = MiNonPagedPoolStart;
   for (i = 0; i < MiNonPagedPoolNrOfPages; i++)
     {
       Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Page);
       if (!NT_SUCCESS(Status))
         {
           DbgPrint("Unable to allocate a page\n");
           KEBUGCHECK(0);
         }
       Status = MmCreateVirtualMapping(NULL,
			               Address,
				       PAGE_READWRITE|PAGE_SYSTEM,
				       Page,
				       FALSE);
       if (!NT_SUCCESS(Status))
         {
           DbgPrint("Unable to create virtual mapping\n");
           KEBUGCHECK(0);
         }
       MiNonPagedPoolAllocMap[i / 32] |= (1 << (i % 32));
#if defined(__GNUC__)
       Address += PAGE_SIZE;
#else
       {
	 char* pTemp = Address;
	 pTemp += PAGE_SIZE;
	 Address = pTemp;
       }
#endif
     }
   /* the first block contains the non paged pool bitmap */
   blk = (BLOCK_HDR*)MiNonPagedPoolStart;
   memset(blk, 0, BLOCK_HDR_SIZE);
   blk->Magic = BLOCK_HDR_USED_MAGIC;
   blk->Size = ROUND_UP(MiNonPagedPoolLength / PAGE_SIZE, 32) / 8;
   blk->previous = NULL;
   blk->Used.Tag = 0xffffffff;
   blk->Used.Caller = 0;
   blk->Used.Dumped = FALSE;
   add_to_used_list(blk);
#ifdef TAG_STATISTICS_TRACKING
   MiAddToTagHashTable(blk);
#endif
   /* the second block is the first free block */
   blk = (BLOCK_HDR*)((char*)blk + BLOCK_HDR_SIZE + blk->Size);
   memset(blk, 0, BLOCK_HDR_SIZE);
   memset((char*)blk + BLOCK_HDR_SIZE, 0x0cc, MiNonPagedPoolNrOfPages * PAGE_SIZE - ((ULONG)blk + BLOCK_HDR_SIZE - (ULONG)MiNonPagedPoolStart));
   blk->Magic = BLOCK_HDR_FREE_MAGIC;
   blk->Size = MiNonPagedPoolLength - ((ULONG)blk + BLOCK_HDR_SIZE - (ULONG)MiNonPagedPoolStart);
   blk->previous = (BLOCK_HDR*)MiNonPagedPoolStart;
   add_to_free_list(blk);
#endif
}

/* EOF */
