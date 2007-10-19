/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/npool.c
 * PURPOSE:         Implements the kernel memory pool
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 *                  Iwan Fatahi (i_fatahi@hotmail.com)
 *                  Robert Bergkvist (fragdance@hotmail.com)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MiInitializeNonPagedPool)
#endif

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
#endif /* __GNUC__ */
#endif

VOID MmPrintMemoryStatistic(VOID);

#define NPOOL_REDZONE_CHECK             /* check the block at deallocation */
// #define NPOOL_REDZONE_CHECK_FULL     /* check all blocks at each allocation/deallocation */
#define NPOOL_REDZONE_SIZE  8           /* number of red zone bytes */
#define NPOOL_REDZONE_LOVALUE 0x87
#define NPOOL_REDZONE_HIVALUE 0xA5


/* avl types ****************************************************************/

/* FIXME:
 *   This declarations should be moved into a separate header file.
 */

typedef struct _NODE
{
   struct _NODE* link[2];
   struct _NODE* parent;
   signed char balance;
}
NODE, *PNODE;

/* TYPES *******************************************************************/

#define BLOCK_HDR_USED_MAGIC (0xdeadbeef)
#define BLOCK_HDR_FREE_MAGIC (0xceadbeef)

/*
 * fields present at the start of a block (this is for internal use only)
 */
typedef struct _HDR
{
    ULONG Magic;
    ULONG Size;
    struct _HDR* previous;
} HDR, *PHDR;

typedef struct _HDR_USED
{
    HDR hdr;
    LIST_ENTRY ListEntry;
    ULONG Tag;
    PVOID Caller;
    LIST_ENTRY TagListEntry;
#if defined(NPOOL_REDZONE_CHECK) || defined(NPOOL_REDZONE_CHECK_FULL)
    ULONG UserSize;
#endif
    BOOLEAN Dumped;
} HDR_USED, *PHDR_USED;

typedef struct _HDR_FREE
{
    HDR hdr;
    NODE Node;
} HDR_FREE, *PHDR_FREE;

#define HDR_FREE_SIZE ROUND_UP(sizeof(HDR_FREE), MM_POOL_ALIGNMENT)

#if defined(NPOOL_REDZONE_CHECK) || defined(NPOOL_REDZONE_CHECK_FULL)
#define HDR_USED_SIZE ROUND_UP(sizeof(HDR_USED) + NPOOL_REDZONE_SIZE, MM_POOL_ALIGNMENT)
#else
#define HDR_USED_SIZE ROUND_UP(sizeof(HDR_USED), MM_POOL_ALIGNMENT)
#endif

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

/*
 * Count of free blocks
 */
static ULONG EiNrFreeBlocks = 0;

/*
 * Count of used blocks
 */
static ULONG EiNrUsedBlocks = 0;

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

/* Total quota for Non Paged Pool */
ULONG MmTotalNonPagedPoolQuota = 0;

#ifdef TAG_STATISTICS_TRACKING
#define TAG_HASH_TABLE_SIZE       (1024)
static LIST_ENTRY tag_hash_table[TAG_HASH_TABLE_SIZE];
#endif /* TAG_STATISTICS_TRACKING */

static PULONG MiNonPagedPoolAllocMap;
static ULONG MiNonPagedPoolNrOfPages;

/* avl helper functions ****************************************************/

void DumpFreeBlockNode(PNODE p)
{
   static int count = 0;
   HDR_FREE* blk;

   count++;

   if (p)
   {
      DumpFreeBlockNode(p->link[0]);
      blk = CONTAINING_RECORD(p, HDR_FREE, Node);
      DbgPrint("%08x %8d (%d)\n", blk, blk->hdr.Size, count);
      DumpFreeBlockNode(p->link[1]);
   }

   count--;
}
void DumpFreeBlockTree(void)
{
   DbgPrint("--- Begin tree ------------------\n");
   DbgPrint("%08x\n", CONTAINING_RECORD(FreeBlockListRoot, HDR_FREE, Node));
   DumpFreeBlockNode(FreeBlockListRoot);
   DbgPrint("--- End tree --------------------\n");
}

int compare_node(PNODE p1, PNODE p2)
{
   HDR_FREE* blk1 = CONTAINING_RECORD(p1, HDR_FREE, Node);
   HDR_FREE* blk2 = CONTAINING_RECORD(p2, HDR_FREE, Node);

   if (blk1->hdr.Size == blk2->hdr.Size)
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
      if (blk1->hdr.Size < blk2->hdr.Size)
      {
         return -1;
      }
      if (blk1->hdr.Size > blk2->hdr.Size)
      {
         return 1;
      }
   }
   return 0;

}

int compare_value(PVOID value, PNODE p)
{
   HDR_FREE* blk = CONTAINING_RECORD(p, HDR_FREE, Node);
   ULONG v = *(PULONG)value;

   if (v < blk->hdr.Size)
   {
      return -1;
   }
   if (v > blk->hdr.Size)
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
         ASSERT(x->balance == +1);
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
         ASSERT(x->balance == -1);
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

               ASSERT(x->balance == -1);
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
               ASSERT(x->balance == +1);
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
MiRemoveFromTagHashTable(HDR_USED* block)
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
MiAddToTagHashTable(HDR_USED* block)
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

#if defined(TAG_STATISTICS_TRACKING)
VOID static
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
      DbgPrint("Tag %x Blocks %d Total Size %d Average Size %d ",
               CurrentTag, CurrentNrBlocks, CurrentSize,
               CurrentSize / CurrentNrBlocks);
      KeRosPrintAddress((PVOID)CurrentTag);
      DbgPrint("\n");
   }
}
#endif /* defined(TAG_STATISTICS_TRACKING) */

VOID
NTAPI
MiDebugDumpNonPagedPoolStats(BOOLEAN NewOnly)
{
#if defined(TAG_STATISTICS_TRACKING)
   ULONG i;
   HDR_USED* current;
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
            current = CONTAINING_RECORD(current_entry, HDR_USED, TagListEntry);
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
                  CurrentSize += current->hdr.Size;
                  TotalSize += current->hdr.Size;
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
   Size = EiFreeNonPagedPool - (MiNonPagedPoolLength - MiNonPagedPoolNrOfPages * PAGE_SIZE);
   DbgPrint("Freeblocks %d TotalFreeSize %d AverageFreeSize %d\n",
            EiNrFreeBlocks, Size, EiNrFreeBlocks ? Size / EiNrFreeBlocks : 0);
   DbgPrint("***************** Dump Complete ***************\n");
#endif /* defined(TAG_STATISTICS_TRACKING) */
}

VOID
NTAPI
MiDebugDumpNonPagedPool(BOOLEAN NewOnly)
{
#if defined(POOL_DEBUG_APIS)
   HDR_USED* current;
   PLIST_ENTRY current_entry;
   KIRQL oldIrql;

   KeAcquireSpinLock(&MmNpoolLock, &oldIrql);

   DbgPrint("******* Dumping non paging pool contents ******\n");
   current_entry = UsedBlockListHead.Flink;
   while (current_entry != &UsedBlockListHead)
   {
      current = CONTAINING_RECORD(current_entry, HDR_USED, ListEntry);
      if (!NewOnly || !current->Dumped)
      {
         CHAR c1, c2, c3, c4;

         c1 = (CHAR)((current->Tag >> 24) & 0xFF);
         c2 = (CHAR)((current->Tag >> 16) & 0xFF);
         c3 = (CHAR)((current->Tag >> 8) & 0xFF);
         c4 = (CHAR)(current->Tag & 0xFF);

         if (isprint(c1) && isprint(c2) && isprint(c3) && isprint(c4))
         {
            DbgPrint("Size 0x%x Tag 0x%x (%c%c%c%c) Allocator 0x%x\n",
                     current->hdr.Size, current->Tag, c4, c3, c2, c1,
                     current->Caller);
         }
         else
         {
            DbgPrint("Size 0x%x Tag 0x%x Allocator 0x%x\n",
                     current->hdr.Size, current->Tag, current->Caller);
         }
         current->Dumped = TRUE;
      }
      current_entry = current_entry->Flink;
   }
   DbgPrint("***************** Dump Complete ***************\n");
   KeReleaseSpinLock(&MmNpoolLock, oldIrql);
#endif
}

#ifdef ENABLE_VALIDATE_POOL
static void validate_free_list(void)
/*
 * FUNCTION: Validate the integrity of the list of free blocks
 */
{
   HDR_FREE* current;
   unsigned int blocks_seen=0;
   PNODE p;

   p = avl_get_first(FreeBlockListRoot);

   while(p)
   {
      PVOID base_addr;

      current = CONTAINING_RECORD(p, HDR_FREE, Node);
      base_addr = (PVOID)current;

      if (current->hdr.Magic != BLOCK_HDR_FREE_MAGIC)
      {
         DbgPrint("Bad block magic (probable pool corruption) at %x\n",
                  current);
         KEBUGCHECK(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
      }

      if (base_addr < MiNonPagedPoolStart ||
            (ULONG_PTR)base_addr + current->hdr.Size > (ULONG_PTR)MiNonPagedPoolStart + MiNonPagedPoolLength)
      {
         DbgPrint("Block %x found outside pool area\n",current);
         DbgPrint("Size %d\n",current->hdr.Size);
         DbgPrint("Limits are %x %x\n",MiNonPagedPoolStart,
                  (ULONG_PTR)MiNonPagedPoolStart+MiNonPagedPoolLength);
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
   HDR_USED* current;
   PLIST_ENTRY current_entry;
   unsigned int blocks_seen=0;

   current_entry = UsedBlockListHead.Flink;
   while (current_entry != &UsedBlockListHead)
   {
      PVOID base_addr;

      current = CONTAINING_RECORD(current_entry, HDR_USED, ListEntry);
      base_addr = (PVOID)current;

      if (current->hdr.Magic != BLOCK_HDR_USED_MAGIC)
      {
         DbgPrint("Bad block magic (probable pool corruption) at %x\n",
                  current);
         KEBUGCHECK(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
      }
      if (base_addr < MiNonPagedPoolStart ||
            ((ULONG_PTR)base_addr+current->hdr.Size) >
            (ULONG_PTR)MiNonPagedPoolStart+MiNonPagedPoolLength)
      {
         DbgPrint("Block %x found outside pool area\n",current);
         DbgPrint("Size %d\n",current->hdr.Size);
         DbgPrint("Limits are %x %x\n",MiNonPagedPoolStart,
                  (ULONG_PTR)MiNonPagedPoolStart+MiNonPagedPoolLength);
         KEBUGCHECK(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
      }
      blocks_seen++;
      if (blocks_seen > EiNrUsedBlocks)
      {
         DbgPrint("Too many blocks on used list\n");
         KEBUGCHECK(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
      }
      if (current->ListEntry.Flink != &UsedBlockListHead &&
            current->ListEntry.Flink->Blink != &current->ListEntry)
      {
         DbgPrint("%s:%d:Break in list (current %x next %x "
                  "current->next->previous %x)\n",
                  __FILE__,__LINE__,current, current->ListEntry.Flink,
                  current->ListEntry.Flink->Blink);
         KEBUGCHECK(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
      }

      current_entry = current_entry->Flink;
   }
}

static void check_duplicates(HDR* blk)
/*
 * FUNCTION: Check a block has no duplicates
 * ARGUMENTS:
 *           blk = block to check
 * NOTE: Bug checks if duplicates are found
 */
{
   ULONG_PTR base = (ULONG_PTR)blk;
   ULONG_PTR last = (ULONG_PTR)blk + blk->Size;
   PLIST_ENTRY current_entry;
   PNODE p;
   HDR_FREE* free;
   HDR_USED* used;

   p = avl_get_first(FreeBlockListRoot);

   while (p)
   {
      free = CONTAINING_RECORD(p, HDR_FREE, Node);
      if (free->hdr.Magic != BLOCK_HDR_FREE_MAGIC)
      {
         DbgPrint("Bad block magic (probable pool corruption) at %x\n",
                  free);
         KEBUGCHECK(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
      }

      if ( (ULONG_PTR)free > base && (ULONG_PTR)free < last )
      {
         DbgPrint("intersecting blocks on list\n");
         KEBUGCHECK(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
      }
      if  ( (ULONG_PTR)free < base &&
            ((ULONG_PTR)free + free->hdr.Size) > base )
      {
         DbgPrint("intersecting blocks on list\n");
         KEBUGCHECK(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
      }
      p = avl_get_next(FreeBlockListRoot, p);
   }

   current_entry = UsedBlockListHead.Flink;
   while (current_entry != &UsedBlockListHead)
   {
      used = CONTAINING_RECORD(current_entry, HDR_USED, ListEntry);

      if ( (ULONG_PTR)used > base && (ULONG_PTR)used < last )
      {
         DbgPrint("intersecting blocks on list\n");
         KEBUGCHECK(/*KBUG_POOL_FREE_LIST_CORRUPT*/0);
      }
      if  ( (ULONG_PTR)used < base &&
            ((ULONG_PTR)used + used->hdr.Size) > base )
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
   HDR_FREE* free;
   HDR_USED* used;
   PLIST_ENTRY current_entry;
   PNODE p;

   validate_free_list();
   validate_used_list();

   p = avl_get_first(FreeBlockListRoot);
   while (p)
   {
      free = CONTAINING_RECORD(p, HDR_FREE, Node);
      check_duplicates(&free->hdr);
      p = avl_get_next(FreeBlockListRoot, p);
   }
   current_entry = UsedBlockListHead.Flink;
   while (current_entry != &UsedBlockListHead)
   {
      used = CONTAINING_RECORD(current_entry, HDR_USED, ListEntry);
      check_duplicates(&used->hdr);
      current_entry = current_entry->Flink;
   }
}
#endif

#if 0
static VOID
free_pages(HDR_FREE* blk)
{
   ULONG start;
   ULONG end;

   start = (ULONG_PTR)blk;
   end = (ULONG_PTR)blk + blk->hdr.Size;

   /*
    * If the block doesn't contain a whole page then there is nothing to do
    */
   if (PAGE_ROUND_UP(start) >= PAGE_ROUND_DOWN(end))
   {
      return;
   }
}
#endif

static void remove_from_used_list(HDR_USED* current)
{
   RemoveEntryList(&current->ListEntry);
   EiUsedNonPagedPool -= current->hdr.Size;
   EiNrUsedBlocks--;
}

static void remove_from_free_list(HDR_FREE* current)
{
   DPRINT("remove_from_free_list %d\n", current->hdr.Size);

   avl_remove(&FreeBlockListRoot, &current->Node, compare_node);

   EiFreeNonPagedPool -= current->hdr.Size;
   EiNrFreeBlocks--;
   DPRINT("remove_from_free_list done\n");
#ifdef DUMP_AVL

   DumpFreeBlockTree();
#endif
}

static void
add_to_free_list(HDR_FREE* blk)
/*
 * FUNCTION: add the block to the free list (internal)
 */
{
   HDR_FREE* current;
   BOOLEAN UpdatePrevPtr = FALSE;

   DPRINT("add_to_free_list (%d)\n", blk->hdr.Size);

   EiNrFreeBlocks++;

   current = (HDR_FREE*)blk->hdr.previous;
   if (current && current->hdr.Magic == BLOCK_HDR_FREE_MAGIC)
   {
      remove_from_free_list(current);
      current->hdr.Size = current->hdr.Size + blk->hdr.Size;
      current->hdr.Magic = BLOCK_HDR_USED_MAGIC;
      memset(blk, 0xcc, HDR_USED_SIZE);
      blk = current;
      UpdatePrevPtr = TRUE;
   }

   current = (HDR_FREE*)((ULONG_PTR)blk + blk->hdr.Size);
   if ((char*)current < (char*)MiNonPagedPoolStart + MiNonPagedPoolLength &&
         current->hdr.Magic == BLOCK_HDR_FREE_MAGIC)
   {
      remove_from_free_list(current);
      blk->hdr.Size += current->hdr.Size;
      memset(current, 0xcc, HDR_FREE_SIZE);
      UpdatePrevPtr = TRUE;
      current = (HDR_FREE*)((ULONG_PTR)blk + blk->hdr.Size);
   }
   if (UpdatePrevPtr &&
         (char*)current < (char*)MiNonPagedPoolStart + MiNonPagedPoolLength)
   {
      current->hdr.previous = &blk->hdr;
   }
   DPRINT("%d\n", blk->hdr.Size);
   blk->hdr.Magic = BLOCK_HDR_FREE_MAGIC;
   EiFreeNonPagedPool += blk->hdr.Size;
   avl_insert(&FreeBlockListRoot, &blk->Node, compare_node);
   DPRINT("add_to_free_list done\n");
#ifdef DUMP_AVL

   DumpFreeBlockTree();
#endif
}

static void add_to_used_list(HDR_USED* blk)
/*
 * FUNCTION: add the block to the used list (internal)
 */
{
   InsertHeadList(&UsedBlockListHead, &blk->ListEntry);
   EiUsedNonPagedPool += blk->hdr.Size;
   EiNrUsedBlocks++;
}


static BOOLEAN
grow_block(HDR_FREE* blk, PVOID end)
{
   NTSTATUS Status;
   PFN_TYPE Page[32];
   ULONG_PTR StartIndex, EndIndex;
   ULONG i, j, k;

   StartIndex = (ULONG_PTR)(PAGE_ROUND_UP((ULONG_PTR)blk + HDR_FREE_SIZE - (ULONG_PTR)MiNonPagedPoolStart)) / PAGE_SIZE;
   EndIndex = ((ULONG_PTR)PAGE_ROUND_UP(end) - (ULONG_PTR)MiNonPagedPoolStart) / PAGE_SIZE;


   for (i = StartIndex; i < EndIndex; i++)
   {
      if (!(MiNonPagedPoolAllocMap[i / 32] & (1 << (i % 32))))
      {
         for (j = i + 1; j < EndIndex && j - i < 32; j++)
	 {
	    if (MiNonPagedPoolAllocMap[j / 32] & (1 << (j % 32)))
	    {
	       break;
	    }
	 }
	 for (k = 0; k < j - i; k++)
	 {
            Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Page[k]);
            if (!NT_SUCCESS(Status))
            {
               for (i = 0; i < k; i++)
	       {
	         MmReleasePageMemoryConsumer(MC_NPPOOL, Page[i]);
	       }
	       return FALSE;
	    }
	 }
         Status = MmCreateVirtualMapping(NULL,
	                                 (PVOID)((ULONG_PTR)MiNonPagedPoolStart + i * PAGE_SIZE),
                                         PAGE_READWRITE|PAGE_SYSTEM,
                                         Page,
                                         k);
	 if (!NT_SUCCESS(Status))
	 {
            for (i = 0; i < k; i++)
	    {
	      MmReleasePageMemoryConsumer(MC_NPPOOL, Page[i]);
	    }
	    return FALSE;
	 }
	 for (j = i; j < k + i; j++)
	 {
	    MiNonPagedPoolAllocMap[j / 32] |= (1 << (j % 32));
	 }
         MiNonPagedPoolNrOfPages += k;
         i += k - 1;
      }
   }
   return TRUE;
}

static HDR_USED* get_block(unsigned int size, unsigned long alignment)
{
   HDR_FREE *blk, *current, *previous = NULL, *next = NULL, *best = NULL;
   ULONG previous_size = 0, current_size, next_size = 0, new_size;
   PVOID end;
   PVOID addr, aligned_addr, best_aligned_addr=NULL;
   PNODE p;

   DPRINT("get_block %d\n", size);

   p = avl_find_equal_or_greater(FreeBlockListRoot, size + HDR_USED_SIZE, compare_value);
   while (p)
   {
      current = CONTAINING_RECORD(p, HDR_FREE, Node);
      addr = (PVOID)((ULONG_PTR)current + HDR_USED_SIZE);
      /* calculate first aligned address available within this block */
      aligned_addr = alignment > 0 ? MM_ROUND_UP(addr, alignment) : addr;
      if (size < PAGE_SIZE)
      {
         /* check that the block is in one page */
         if (PAGE_ROUND_DOWN(aligned_addr) != PAGE_ROUND_DOWN((ULONG_PTR)aligned_addr + size - 1))
         {
            aligned_addr = (PVOID)PAGE_ROUND_UP(aligned_addr);
         }
      }
      DPRINT("%x %x\n", addr, aligned_addr);
      if (aligned_addr != addr)
      {
         while((ULONG_PTR)aligned_addr - (ULONG_PTR)addr < HDR_FREE_SIZE)
         {
            if (alignment == 0)
            {
               aligned_addr = (PVOID)((ULONG_PTR)current + HDR_USED_SIZE + HDR_FREE_SIZE);
            }
            else
            {
               aligned_addr = MM_ROUND_UP((PVOID)((ULONG_PTR)current + HDR_USED_SIZE + HDR_FREE_SIZE), alignment);
            }
            if (size < PAGE_SIZE)
            {
               /* check that the block is in one page */
               if (PAGE_ROUND_DOWN(aligned_addr) != PAGE_ROUND_DOWN((ULONG_PTR)aligned_addr + size - 1))
               {
                  aligned_addr = (PVOID)PAGE_ROUND_UP(aligned_addr);
               }
            }
         }
      }
      DPRINT("%x %x\n", addr, aligned_addr);
      new_size = (ULONG_PTR)aligned_addr - (ULONG_PTR)addr + size;
      if (current->hdr.Size >= new_size + HDR_USED_SIZE &&
          (best == NULL || current->hdr.Size < best->hdr.Size))
      {
         best = current;
         best_aligned_addr = aligned_addr;
         if (new_size <= size + 2 * HDR_FREE_SIZE)
         {
             break;
         }
      }

      if (best)
      {
         if (size < PAGE_SIZE)
         {
            if (current->hdr.Size >= 2 * PAGE_SIZE + HDR_FREE_SIZE)
            {
                break;
            }
         }
         else
         {
            if (current->hdr.Size >= size + alignment + HDR_FREE_SIZE)
            {
                break;
            }
         }
      }
      p = avl_get_next(FreeBlockListRoot, p);
   }
   /*
    * We didn't find anything suitable at all.
    */
   if (best == NULL)
   {
      return NULL;
   }

   DPRINT(":: blk %x blk->hdr.Size %d (%d) Size %d\n", best, best->hdr.Size, best->hdr.Size - HDR_USED_SIZE, size);

   current = best;
   current_size = current->hdr.Size - HDR_USED_SIZE;
   addr = (PVOID)((ULONG_PTR)current + HDR_USED_SIZE);
   if (addr != best_aligned_addr)
   {
      blk = (HDR_FREE*)((ULONG_PTR)best_aligned_addr - HDR_USED_SIZE);
      /*
       * if size-aligned, break off the preceding bytes into their own block...
       */
      previous = current;
      previous_size = (ULONG_PTR)blk - (ULONG_PTR)previous - HDR_FREE_SIZE;
      current = blk;
      current_size -= ((ULONG_PTR)current - (ULONG_PTR)previous);
   }
   end = (PVOID)((ULONG_PTR)current + HDR_USED_SIZE + size);

   if (current_size >= size + HDR_FREE_SIZE + MM_POOL_ALIGNMENT)
   {
      /* create a new free block after our block, if the memory size is >= 4 byte for this block */
      next = (HDR_FREE*)((ULONG_PTR)current + size + HDR_USED_SIZE);
      next_size = current_size - size - HDR_FREE_SIZE;
      current_size = size;
      end = (PVOID)((ULONG_PTR)next + HDR_FREE_SIZE);
   }

   if (previous)
   {
      remove_from_free_list(previous);
      if (!grow_block(previous, end))
      {
         add_to_free_list(previous);
         return NULL;
      }
      memset(current, 0, HDR_USED_SIZE);
      current->hdr.Size = current_size + HDR_USED_SIZE;
      current->hdr.Magic = BLOCK_HDR_USED_MAGIC;
      current->hdr.previous = &previous->hdr;
      previous->hdr.Size = previous_size + HDR_FREE_SIZE;
      if (next == NULL)
      {
         blk = (HDR_FREE*)((ULONG_PTR)current + current->hdr.Size);
         if ((ULONG_PTR)blk < (ULONG_PTR)MiNonPagedPoolStart + MiNonPagedPoolLength)
         {
            blk->hdr.previous = &current->hdr;
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
      current->hdr.Magic = BLOCK_HDR_USED_MAGIC;
      if (next)
      {
         current->hdr.Size = current_size + HDR_USED_SIZE;
      }
   }

   if (next)
   {
      memset(next, 0, HDR_FREE_SIZE);
      next->hdr.Size = next_size + HDR_FREE_SIZE;
      next->hdr.Magic = BLOCK_HDR_FREE_MAGIC;
      next->hdr.previous = &current->hdr;
      blk = (HDR_FREE*)((ULONG_PTR)next + next->hdr.Size);
      if ((ULONG_PTR)blk < (ULONG_PTR)MiNonPagedPoolStart + MiNonPagedPoolLength)
      {
         blk->hdr.previous = &next->hdr;
      }
      add_to_free_list(next);
   }

   add_to_used_list((HDR_USED*)current);
   VALIDATE_POOL;

   if (size < PAGE_SIZE)
   {
       addr = (PVOID)((ULONG_PTR)current + HDR_USED_SIZE);
       if (PAGE_ROUND_DOWN(addr) != PAGE_ROUND_DOWN((PVOID)((ULONG_PTR)addr + size - 1)))
       {
           DPRINT1("%x %x\n", addr, (ULONG_PTR)addr + size);
       }
       ASSERT (PAGE_ROUND_DOWN(addr) == PAGE_ROUND_DOWN((PVOID)((ULONG_PTR)addr + size - 1)));
   }
   if (alignment)
   {
       addr = (PVOID)((ULONG_PTR)current + HDR_USED_SIZE);
       ASSERT(MM_ROUND_UP(addr, alignment) == addr);
   }
   return (HDR_USED*)current;
}

ULONG STDCALL
ExRosQueryNonPagedPoolTag ( PVOID Addr )
{
   HDR_USED* blk=(HDR_USED*)((ULONG_PTR)Addr - HDR_USED_SIZE);
   if (blk->hdr.Magic != BLOCK_HDR_USED_MAGIC)
      KEBUGCHECK(0);

   return blk->Tag;
}

#if defined(NPOOL_REDZONE_CHECK) || defined(NPOOL_REDZONE_CHECK_FULL)
void check_redzone_header(HDR_USED* hdr)
{
   PUCHAR LoZone = (PUCHAR)((ULONG_PTR)hdr + HDR_USED_SIZE - NPOOL_REDZONE_SIZE);
   PUCHAR HiZone = (PUCHAR)((ULONG_PTR)hdr + HDR_USED_SIZE + hdr->UserSize);
   BOOLEAN LoOK = TRUE;
   BOOLEAN HiOK = TRUE;
   ULONG i;
   CHAR c[5];

   for (i = 0; i < NPOOL_REDZONE_SIZE; i++)
   {
      if (LoZone[i] != NPOOL_REDZONE_LOVALUE)
      {
         LoOK = FALSE;
      }
      if (HiZone[i] != NPOOL_REDZONE_HIVALUE)
      {
         HiOK = FALSE;
      }
   }

   if (!HiOK || !LoOK)
   {
      c[0] = (CHAR)((hdr->Tag >> 24) & 0xFF);
      c[1] = (CHAR)((hdr->Tag >> 16) & 0xFF);
      c[2] = (CHAR)((hdr->Tag >> 8) & 0xFF);
      c[3] = (CHAR)(hdr->Tag & 0xFF);
      c[4] = 0;

      if (!isprint(c[0]) || !isprint(c[1]) || !isprint(c[2]) || !isprint(c[3]))
      {
         c[0] = 0;
      }

      if (!LoOK)
      {
         DbgPrint("NPOOL: Low-side redzone overwritten, Block %x, Size %d, Tag %x(%s), Caller %x\n",
                  (ULONG_PTR)hdr + HDR_USED_SIZE, hdr->UserSize, hdr->Tag, c, hdr->Caller);
      }
      if (!HiOK)
      {
         DbgPrint("NPPOL: High-side redzone overwritten, Block %x, Size %d, Tag %x(%s), Caller %x\n",
                  (ULONG_PTR)hdr + HDR_USED_SIZE, hdr->UserSize, hdr->Tag, c, hdr->Caller);
      }
      KEBUGCHECK(0);
   }
}
#endif

#ifdef NPOOL_REDZONE_CHECK_FULL
void check_redzone_list(void)
{
   PLIST_ENTRY current_entry;

   current_entry = UsedBlockListHead.Flink;
   while (current_entry != &UsedBlockListHead)
   {
      check_redzone_header(CONTAINING_RECORD(current_entry, HDR_USED, ListEntry));
      current_entry = current_entry->Flink;
   }
}
#endif


VOID STDCALL ExFreeNonPagedPool (PVOID block)
/*
 * FUNCTION: Releases previously allocated memory
 * ARGUMENTS:
 *        block = block to free
 */
{
   HDR_USED* blk=(HDR_USED*)((ULONG_PTR)block - HDR_USED_SIZE);
   KIRQL oldIrql;

   if (block == NULL)
   {
      return;
   }

   DPRINT("freeing block %x\n",blk);

   POOL_TRACE("ExFreePool(block %x), size %d, caller %x\n",block,blk->hdr.Size,
              blk->Caller);
   KeAcquireSpinLock(&MmNpoolLock, &oldIrql);

   VALIDATE_POOL;
   if (blk->hdr.Magic != BLOCK_HDR_USED_MAGIC)
   {
      if (blk->hdr.Magic == BLOCK_HDR_FREE_MAGIC)
      {
         DbgPrint("ExFreePool of already freed address %x\n", block);
      }
      else
      {
         DbgPrint("ExFreePool of non-allocated address %x (magic %x)\n",
                  block, blk->hdr.Magic);
      }
      KEBUGCHECK(0);
      return;
   }

#if defined(NPOOL_REDZONE_CHECK) || defined(NPOOL_REDZONE_CHECK_FULL)
   check_redzone_header(blk);
#endif

#ifdef NPOOL_REDZONE_CHECK_FULL
   check_redzone_list();
#endif

   memset(block, 0xcc, blk->hdr.Size - HDR_USED_SIZE);

#ifdef TAG_STATISTICS_TRACKING
   MiRemoveFromTagHashTable(blk);
#endif

   remove_from_used_list(blk);
   blk->hdr.Magic = BLOCK_HDR_FREE_MAGIC;
   add_to_free_list((HDR_FREE*)blk);
   VALIDATE_POOL;
   KeReleaseSpinLock(&MmNpoolLock, oldIrql);
}

PVOID STDCALL
ExAllocateNonPagedPoolWithTag(POOL_TYPE Type, ULONG Size, ULONG Tag, PVOID Caller)
{
#if defined(NPOOL_REDZONE_CHECK) || defined(NPOOL_REDZONE_CHECK_FULL)
   ULONG UserSize;
#endif
   PVOID block;
   HDR_USED* best = NULL;
   KIRQL oldIrql;
   ULONG alignment;

   POOL_TRACE("ExAllocatePool(NumberOfBytes %d) caller %x ",
              Size,Caller);

   KeAcquireSpinLock(&MmNpoolLock, &oldIrql);

#ifdef NPOOL_REDZONE_CHECK_FULL
   check_redzone_list();
#endif

   VALIDATE_POOL;

#if 1
   /* after some allocations print the npaged pool stats */
#ifdef TAG_STATISTICS_TRACKING

   {
      static ULONG counter = 0;
      if (counter++ % 100000 == 0)
      {
         MiDebugDumpNonPagedPoolStats(FALSE);
         MmPrintMemoryStatistic();
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
#if defined(NPOOL_REDZONE_CHECK) || defined(NPOOL_REDZONE_CHECK_FULL)
   UserSize = Size;
   Size = ROUND_UP(Size + NPOOL_REDZONE_SIZE, MM_POOL_ALIGNMENT);
#else
   Size = ROUND_UP(Size, MM_POOL_ALIGNMENT);
#endif

   if (Size >= PAGE_SIZE)
   {
      alignment = PAGE_SIZE;
   }
   else if (Type & CACHE_ALIGNED_POOL_MASK)
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
      KeRosDumpStackFrames(NULL, 10);
      return NULL;
   }
   best->Tag = Tag;
   best->Caller = Caller;
   best->Dumped = FALSE;
   best->TagListEntry.Flink = best->TagListEntry.Blink = NULL;
#if defined(NPOOL_REDZONE_CHECK) || defined(NPOOL_REDZONE_CHECK_FULL)
   best->UserSize = UserSize;
   memset((PVOID)((ULONG_PTR)best + HDR_USED_SIZE - NPOOL_REDZONE_SIZE), NPOOL_REDZONE_LOVALUE, NPOOL_REDZONE_SIZE);
   memset((PVOID)((ULONG_PTR)best + HDR_USED_SIZE + UserSize), NPOOL_REDZONE_HIVALUE, NPOOL_REDZONE_SIZE);
#endif

#ifdef TAG_STATISTICS_TRACKING

   MiAddToTagHashTable(best);
#endif

   KeReleaseSpinLock(&MmNpoolLock, oldIrql);
   block = (PVOID)((ULONG_PTR)best + HDR_USED_SIZE);
   /*   RtlZeroMemory(block, Size);*/
   return(block);
}

VOID
INIT_FUNCTION
NTAPI
MiInitializeNonPagedPool(VOID)
{
   NTSTATUS Status;
   PFN_TYPE Page;
   ULONG i;
   PVOID Address;
   HDR_USED* used;
   HDR_FREE* free;
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

   MiNonPagedPoolAllocMap = (PVOID)((ULONG_PTR)MiNonPagedPoolStart + PAGE_SIZE);
#if defined(NPOOL_REDZONE_CHECK) || defined(NPOOL_REDZONE_CHECK_FULL)
   MiNonPagedPoolNrOfPages = ROUND_UP(MiNonPagedPoolLength / PAGE_SIZE, 32) / 8;
   MiNonPagedPoolNrOfPages = ROUND_UP(MiNonPagedPoolNrOfPages + NPOOL_REDZONE_SIZE, MM_POOL_ALIGNMENT);
   MiNonPagedPoolNrOfPages = PAGE_ROUND_UP(MiNonPagedPoolNrOfPages + HDR_FREE_SIZE) + PAGE_SIZE;
#else
   MiNonPagedPoolNrOfPages = PAGE_ROUND_UP(ROUND_UP(MiNonPagedPoolLength / PAGE_SIZE, 32) / 8 + HDR_FREE_SIZE) + PAGE_SIZE;
#endif
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
                                      &Page,
                                      1);
      if (!NT_SUCCESS(Status))
      {
         DbgPrint("Unable to create virtual mapping\n");
         KEBUGCHECK(0);
      }
      Address = (PVOID)((ULONG_PTR)Address + PAGE_SIZE);
   }

   for (i = 0; i < MiNonPagedPoolNrOfPages; i++)
   {
      MiNonPagedPoolAllocMap[i / 32] |= (1 << (i % 32));
   }

   /* the first block is free */
   free = (HDR_FREE*)MiNonPagedPoolStart;
   free->hdr.Magic = BLOCK_HDR_FREE_MAGIC;
   free->hdr.Size = PAGE_SIZE - HDR_USED_SIZE;
   free->hdr.previous = NULL;
   memset((PVOID)((ULONG_PTR)free + HDR_FREE_SIZE), 0xcc, free->hdr.Size - HDR_FREE_SIZE);
   add_to_free_list(free);

   /* the second block contains the non paged pool bitmap */
   used = (HDR_USED*)((ULONG_PTR)free + free->hdr.Size);
   used->hdr.Magic = BLOCK_HDR_USED_MAGIC;
#if defined(NPOOL_REDZONE_CHECK) || defined(NPOOL_REDZONE_CHECK_FULL)
   used->UserSize = ROUND_UP(MiNonPagedPoolLength / PAGE_SIZE, 32) / 8;
   used->hdr.Size = ROUND_UP(used->UserSize + NPOOL_REDZONE_SIZE, MM_POOL_ALIGNMENT) + HDR_USED_SIZE;
   memset((PVOID)((ULONG_PTR)used + HDR_USED_SIZE - NPOOL_REDZONE_SIZE), NPOOL_REDZONE_LOVALUE, NPOOL_REDZONE_SIZE);
   memset((PVOID)((ULONG_PTR)used + HDR_USED_SIZE + used->UserSize), NPOOL_REDZONE_HIVALUE, NPOOL_REDZONE_SIZE);
#else
   used->hdr.Size = ROUND_UP(MiNonPagedPoolLength / PAGE_SIZE, 32) / 8 + HDR_USED_SIZE;
#endif
   used->hdr.previous = &free->hdr;
   used->Tag = 0xffffffff;
   used->Caller = (PVOID)MiInitializeNonPagedPool;
   used->Dumped = FALSE;
   add_to_used_list(used);
#ifdef TAG_STATISTICS_TRACKING
   MiAddToTagHashTable(used);
#endif

   /* the third block is the free block after the bitmap */
   free = (HDR_FREE*)((ULONG_PTR)used + used->hdr.Size);
   free->hdr.Magic = BLOCK_HDR_FREE_MAGIC;
   free->hdr.Size = MiNonPagedPoolLength - ((ULONG_PTR)free - (ULONG_PTR)MiNonPagedPoolStart);
   free->hdr.previous = &used->hdr;
   memset((PVOID)((ULONG_PTR)free + HDR_FREE_SIZE), 0xcc, (ULONG_PTR)Address - (ULONG_PTR)free - HDR_FREE_SIZE);
   add_to_free_list(free);
}

PVOID
STDCALL
MiAllocateSpecialPool  (IN POOL_TYPE PoolType,
                        IN SIZE_T NumberOfBytes,
                        IN ULONG Tag,
                        IN ULONG Underrun
                        )
{
    /* FIXME: Special Pools not Supported */
    DbgPrint("Special Pools not supported\n");
    return NULL;
}

/* EOF */
