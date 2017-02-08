/**************************************************************************
 *
 * Copyright (C) 1999 Wittawat Yamwong
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * WITTAWAT YAMWONG, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#include "pipe/p_compiler.h"
#include "util/u_debug.h"

#include "util/u_memory.h"
#include "util/u_mm.h"


void
u_mmDumpMemInfo(const struct mem_block *heap)
{
   debug_printf("Memory heap %p:\n", (void *) heap);
   if (heap == 0) {
      debug_printf("  heap == 0\n");
   }
   else {
      const struct mem_block *p;
      int total_used = 0, total_free = 0;

      for (p = heap->next; p != heap; p = p->next) {
	 debug_printf("  Offset:%08x, Size:%08x, %c%c\n", p->ofs, p->size,
                      p->free ? 'F':'.',
                      p->reserved ? 'R':'.');
         if (p->free)
            total_free += p->size;
         else
            total_used += p->size;
      }

      debug_printf("'\nMemory stats: total = %d, used = %d, free = %d\n",
                   total_used + total_free, total_used, total_free);
      debug_printf("\nFree list:\n");

      for (p = heap->next_free; p != heap; p = p->next_free) {
	 debug_printf(" FREE Offset:%08x, Size:%08x, %c%c\n", p->ofs, p->size,
                      p->free ? 'F':'.',
                      p->reserved ? 'R':'.');
      }

   }
   debug_printf("End of memory blocks\n");
}


struct mem_block *
u_mmInit(int ofs, int size)
{
   struct mem_block *heap, *block;
  
   if (size <= 0) 
      return NULL;

   heap = CALLOC_STRUCT(mem_block);
   if (!heap) 
      return NULL;
   
   block = CALLOC_STRUCT(mem_block);
   if (!block) {
      FREE(heap);
      return NULL;
   }

   heap->next = block;
   heap->prev = block;
   heap->next_free = block;
   heap->prev_free = block;

   block->heap = heap;
   block->next = heap;
   block->prev = heap;
   block->next_free = heap;
   block->prev_free = heap;

   block->ofs = ofs;
   block->size = size;
   block->free = 1;

   return heap;
}


static struct mem_block *
SliceBlock(struct mem_block *p, 
           int startofs, int size, 
           int reserved, int alignment)
{
   struct mem_block *newblock;

   /* break left  [p, newblock, p->next], then p = newblock */
   if (startofs > p->ofs) {
      newblock = CALLOC_STRUCT(mem_block);
      if (!newblock)
	 return NULL;
      newblock->ofs = startofs;
      newblock->size = p->size - (startofs - p->ofs);
      newblock->free = 1;
      newblock->heap = p->heap;

      newblock->next = p->next;
      newblock->prev = p;
      p->next->prev = newblock;
      p->next = newblock;

      newblock->next_free = p->next_free;
      newblock->prev_free = p;
      p->next_free->prev_free = newblock;
      p->next_free = newblock;

      p->size -= newblock->size;
      p = newblock;
   }

   /* break right, also [p, newblock, p->next] */
   if (size < p->size) {
      newblock = CALLOC_STRUCT(mem_block);
      if (!newblock)
	 return NULL;
      newblock->ofs = startofs + size;
      newblock->size = p->size - size;
      newblock->free = 1;
      newblock->heap = p->heap;

      newblock->next = p->next;
      newblock->prev = p;
      p->next->prev = newblock;
      p->next = newblock;

      newblock->next_free = p->next_free;
      newblock->prev_free = p;
      p->next_free->prev_free = newblock;
      p->next_free = newblock;
	 
      p->size = size;
   }

   /* p = middle block */
   p->free = 0;

   /* Remove p from the free list: 
    */
   p->next_free->prev_free = p->prev_free;
   p->prev_free->next_free = p->next_free;

   p->next_free = 0;
   p->prev_free = 0;

   p->reserved = reserved;
   return p;
}


struct mem_block *
u_mmAllocMem(struct mem_block *heap, int size, int align2, int startSearch)
{
   struct mem_block *p;
   const int mask = (1 << align2)-1;
   int startofs = 0;
   int endofs;

   assert(size >= 0);
   assert(align2 >= 0);
   assert(align2 <= 12); /* sanity check, 2^12 (4KB) enough? */

   if (!heap || align2 < 0 || size <= 0)
      return NULL;

   for (p = heap->next_free; p != heap; p = p->next_free) {
      assert(p->free);

      startofs = (p->ofs + mask) & ~mask;
      if ( startofs < startSearch ) {
	 startofs = startSearch;
      }
      endofs = startofs+size;
      if (endofs <= (p->ofs+p->size))
	 break;
   }

   if (p == heap) 
      return NULL;

   assert(p->free);
   p = SliceBlock(p,startofs,size,0,mask+1);

   return p;
}


struct mem_block *
u_mmFindBlock(struct mem_block *heap, int start)
{
   struct mem_block *p;

   for (p = heap->next; p != heap; p = p->next) {
      if (p->ofs == start) 
	 return p;
   }

   return NULL;
}


static INLINE int
Join2Blocks(struct mem_block *p)
{
   /* XXX there should be some assertions here */

   /* NOTE: heap->free == 0 */

   if (p->free && p->next->free) {
      struct mem_block *q = p->next;

      assert(p->ofs + p->size == q->ofs);
      p->size += q->size;

      p->next = q->next;
      q->next->prev = p;

      q->next_free->prev_free = q->prev_free; 
      q->prev_free->next_free = q->next_free;
     
      FREE(q);
      return 1;
   }
   return 0;
}

int
u_mmFreeMem(struct mem_block *b)
{
   if (!b)
      return 0;

   if (b->free) {
      debug_printf("block already free\n");
      return -1;
   }
   if (b->reserved) {
      debug_printf("block is reserved\n");
      return -1;
   }

   b->free = 1;
   b->next_free = b->heap->next_free;
   b->prev_free = b->heap;
   b->next_free->prev_free = b;
   b->prev_free->next_free = b;

   Join2Blocks(b);
   if (b->prev != b->heap)
      Join2Blocks(b->prev);

   return 0;
}


void
u_mmDestroy(struct mem_block *heap)
{
   struct mem_block *p;

   if (!heap)
      return;

   for (p = heap->next; p != heap; ) {
      struct mem_block *next = p->next;
      FREE(p);
      p = next;
   }

   FREE(heap);
}
