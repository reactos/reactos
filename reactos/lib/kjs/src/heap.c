/*
 * Dynamic memory allocation.
 * Copyright (c) 1998 New Generation Software (NGS) Oy
 *
 * Author: Markku Rossi <mtr@ngs.fi>
 */

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA
 */

/*
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/heap.c,v $
 * $Id: heap.c,v 1.1 2004/01/10 20:38:18 arty Exp $
 */

#include "jsint.h"

/*
 * Types and definitions.
 */

#if SIZEOF_INT == 2
#define BLOCK_SIZE 	(63 * 1024)
#else
#define BLOCK_SIZE 	(100 * 1024)
#endif

/*
 * The size of the minimum block that can be allocated from the heap.
 * The block must be big enought to hold one pointer that is used when
 * then block is entered to the freelist.
 */
#define MIN_ALLOC 	(sizeof (void *))

#if JS_MEM_DEBUG
#define MAGIC 0xfe109abe
#endif

/*
 * Prototypes for static functions.
 */

static inline unsigned int
list (unsigned int size)
{
  unsigned int list = 0;
  size >>= 3;

  while (size > 0)
    {
      size >>= 1;
      list++;
    }
  if (list >= JS_NUM_HEAP_FREELISTS)
    list = JS_NUM_HEAP_FREELISTS - 1;

  return list;
}


static inline void
delete_destroyable (JSHeapMemoryBlock *b)
{
  JSHeapDestroyable *destroyable
    = (JSHeapDestroyable *) ((unsigned char *) b
			     + sizeof (JSHeapMemoryBlock));

  if (destroyable->destroy)
    (*destroyable->destroy) (destroyable);
}


static unsigned long
sweep (JSVirtualMachine *vm)
{
  JSHeapBlock *hb;
  unsigned long bytes_in_use = 0;
  int i;
  unsigned int freelist;

  for (i = 0; i < JS_NUM_HEAP_FREELISTS; i++)
    vm->heap_freelists[i] = NULL;

  vm->gc.bytes_free = 0;
  vm->gc.bytes_allocated = 0;

  for (hb = vm->heap; hb; hb = hb->next)
    {
      JSHeapMemoryBlock *b, *e, *bnext;

      b = (JSHeapMemoryBlock *) ((unsigned char *) hb + sizeof (JSHeapBlock));
      e = (JSHeapMemoryBlock *) ((unsigned char *) hb + sizeof (JSHeapBlock)
				 + hb->size);
      for (; b < e; b = bnext)
	{
#if JS_MEM_DEBUG
	  assert (b->magic == MAGIC);
#endif
	  bnext = (JSHeapMemoryBlock *) ((unsigned char *) b
					 + sizeof (JSHeapMemoryBlock)
					 + b->size);

	  if (b->flag_mark)
	    {
	      bytes_in_use += b->size;
	      b->flag_mark = 0;
	      vm->gc.bytes_allocated = b->size;
	    }
	  else
	    {
	      if (b->flag_destroyable)
		delete_destroyable (b);

	      /* Pack consecutive free blocks to one big block. */
	      while (bnext < e && bnext->flag_mark == 0)
		{
#if JS_MEM_DEBUG
		  assert (bnext->magic == MAGIC);
#endif
		  if (bnext->flag_destroyable)
		    delete_destroyable (bnext);

		  b->size += bnext->size + sizeof (JSHeapMemoryBlock);
		  bnext = (JSHeapMemoryBlock *) ((unsigned char *) bnext
						 + sizeof (JSHeapMemoryBlock)
						 + bnext->size);
		}

	      JS_HEAP_MEMORY_BLOCK_CLEAR_FLAGS (b);

	      /* Link it to the freelist. */
	      freelist = list (b->size);

	      ((JSHeapFreelistBlock *) b)->next = vm->heap_freelists[freelist];
	      vm->heap_freelists[freelist] = b;
	      vm->gc.bytes_free += b->size;
	    }
	}
    }

  return bytes_in_use;
}


/*
 * Global functions.
 */

void *
js_vm_alloc (JSVirtualMachine *vm, unsigned int size)
{
  JSHeapMemoryBlock *b, *prev;
  unsigned int alloc_size;
  JSHeapBlock *hb;
  unsigned int to_alloc;
  unsigned int freelist;
  char buf[512];

  /* Round it up to the next pow of two. */
  for (alloc_size = MIN_ALLOC; alloc_size < size; alloc_size *= 2)
    ;

 retry:

  /* Take first block from the freelist that is big enough for us. */
  for (freelist = list (alloc_size); freelist < JS_NUM_HEAP_FREELISTS;
       freelist++)
    for (prev = NULL, b = vm->heap_freelists[freelist]; b;
	 prev = b, b = ((JSHeapFreelistBlock *) b)->next)
      if (b->size >= alloc_size)
	{
	  /* Ok, take this one. */
	  if (prev)
	    ((JSHeapFreelistBlock *) prev)->next
	      = ((JSHeapFreelistBlock *) b)->next;
	  else
	    vm->heap_freelists[freelist] = ((JSHeapFreelistBlock *) b)->next;

	  if (b->size > alloc_size + sizeof (JSHeapMemoryBlock) + MIN_ALLOC)
	    {
	      JSHeapMemoryBlock *nb;

	      /* We can split it. */
	      nb = ((JSHeapMemoryBlock *)
		    ((unsigned char *) b
		     + sizeof (JSHeapMemoryBlock) + alloc_size));

#if JS_MEM_DEBUG
	      nb->magic = MAGIC;
#endif
	      JS_HEAP_MEMORY_BLOCK_CLEAR_FLAGS (nb);
	      nb->size = b->size - sizeof (JSHeapMemoryBlock) - alloc_size;

	      vm->gc.bytes_free -= sizeof (JSHeapMemoryBlock);

	      freelist = list (nb->size);
	      ((JSHeapFreelistBlock *) nb)->next
		= vm->heap_freelists[freelist];
	      vm->heap_freelists[freelist] = nb;

	      b->size = alloc_size;
	    }

	  JS_HEAP_MEMORY_BLOCK_CLEAR_FLAGS (b);
	  vm->gc.bytes_free -= b->size;
	  vm->gc.bytes_allocated += b->size;

	  return (unsigned char *) b + sizeof (JSHeapMemoryBlock);
	}

  /* Must allocate new blocks to the freelist. */

  if (alloc_size > (BLOCK_SIZE
		    - sizeof (JSHeapBlock)
		    - sizeof (JSHeapMemoryBlock)))
    to_alloc = alloc_size + sizeof (JSHeapBlock) + sizeof (JSHeapMemoryBlock);
  else
    to_alloc = BLOCK_SIZE;

  if (vm->verbose > 2)
    {
      sprintf (buf,
	       "VM: heap: malloc(%u): needed=%u, size=%lu, free=%lu, allocated=%lu%s",
	       to_alloc, alloc_size, vm->heap_size, vm->gc.bytes_free,
	       vm->gc.bytes_allocated,
	       JS_HOST_LINE_BREAK);
      js_iostream_write (vm->s_stderr, buf, strlen (buf));
    }

  hb = js_malloc (vm, to_alloc);

  vm->heap_size += to_alloc;
  hb->next = vm->heap;
  vm->heap = hb;
  hb->size = to_alloc - sizeof (JSHeapBlock);

  /* Link it to the freelist. */
  b = (JSHeapMemoryBlock *) ((unsigned char *) hb + sizeof (JSHeapBlock));

#if JS_MEM_DEBUG
  b->magic = MAGIC;
#endif
  JS_HEAP_MEMORY_BLOCK_CLEAR_FLAGS (b);
  b->size = hb->size - sizeof (JSHeapMemoryBlock);

  freelist = list (b->size);

  ((JSHeapFreelistBlock *) b)->next = vm->heap_freelists[freelist];
  vm->heap_freelists[freelist] = b;

  vm->gc.bytes_free += b->size;

  goto retry;

  /* NOTRECHED */
  return NULL;
}


void *
js_vm_alloc_destroyable (JSVirtualMachine *vm, unsigned int size)
{
  unsigned char *bi;
  JSHeapMemoryBlock *b;

  bi = js_vm_alloc (vm, size);
  memset (bi, 0, size);

  b = (JSHeapMemoryBlock *) (bi - sizeof (JSHeapMemoryBlock));
  b->flag_destroyable = 1;

  return bi;
}


void *
js_vm_realloc (JSVirtualMachine *vm, void *ptr, unsigned int new_size)
{
  JSHeapMemoryBlock *b;
  void *nptr;

  if (ptr == NULL)
    return js_vm_alloc (vm, new_size);

  /* Can be use the old block? */

  b = (JSHeapMemoryBlock *) ((unsigned char *) ptr
			     - sizeof (JSHeapMemoryBlock));

#if JS_MEM_DEBUG
  assert (b->magic == MAGIC);
#endif

  if (b->size >= new_size)
    /* Yes we can. */
    return ptr;

  /* No we can't.  Must allocate a new one. */
  nptr = js_vm_alloc (vm, new_size);
  memcpy (nptr, ptr, b->size < new_size ? b->size : new_size);

  js_vm_free (vm, ptr);

  return nptr;
}


void
js_vm_free (JSVirtualMachine *vm, void *ptr)
{
  JSHeapMemoryBlock *b;
  unsigned int freelist;

  b = (JSHeapMemoryBlock *) ((unsigned char *) ptr
			     - sizeof (JSHeapMemoryBlock));

#if JS_MEM_DEBUG
  assert (b->magic == MAGIC);
#endif

  freelist = list (b->size);

  ((JSHeapFreelistBlock *) b)->next = vm->heap_freelists[freelist];
  vm->heap_freelists[freelist] = b;
  vm->gc.bytes_free += b->size;

  /*
   * We could try to compact the heap, but we left it to the garbage
   * collection.
   */
}


int
js_vm_mark_ptr (void *ptr)
{
  JSHeapMemoryBlock *b;

  if (ptr == NULL)
    return 0;

  b = (JSHeapMemoryBlock *) ((unsigned char *) ptr
			     - sizeof (JSHeapMemoryBlock));
#if JS_MEM_DEBUG
  assert (b->magic == MAGIC);
#endif

  if (b->flag_mark)
    return 0;

  b->flag_mark = 1;

  return 1;
}


int
js_vm_is_marked_ptr (void *ptr)
{
  JSHeapMemoryBlock *b;

  if (ptr == NULL)
    return 1;

  b = (JSHeapMemoryBlock *) ((unsigned char *) ptr
			     - sizeof (JSHeapMemoryBlock));
#if JS_MEM_DEBUG
  assert (b->magic == MAGIC);
#endif

  if (b->flag_mark)
    return 1;

  return 0;
}


void
js_vm_mark (JSNode *n)
{
  unsigned int i;

  switch (n->type)
    {
    case JS_UNDEFINED:
    case JS_NULL:
    case JS_BOOLEAN:
    case JS_INTEGER:
    case JS_FLOAT:
    case JS_SYMBOL:
    case JS_NAN:
    case JS_IPTR:
    case JS_ARGS_FIX:
      /* Nothing here. */
      break;

    case JS_STRING:
      js_vm_mark_ptr (n->u.vstring);
      if (!n->u.vstring->staticp)
	js_vm_mark_ptr (n->u.vstring->data);

      js_vm_object_mark (n->u.vstring->prototype);
      break;

    case JS_OBJECT:
      js_vm_object_mark (n->u.vobject);
      break;

    case JS_ARRAY:
      if (js_vm_mark_ptr (n->u.varray))
	{
	  js_vm_mark_ptr (n->u.varray->data);

	  for (i = 0; i < n->u.varray->length; i++)
	    js_vm_mark (&n->u.varray->data[i]);

	  js_vm_object_mark (n->u.varray->prototype);
	}
      break;

    case JS_BUILTIN:
      if (js_vm_mark_ptr (n->u.vbuiltin))
	{
	  js_vm_mark_ptr (n->u.vbuiltin->info);

	  js_vm_object_mark (n->u.vbuiltin->info->prototype);
	  js_vm_object_mark (n->u.vbuiltin->prototype);

	  if (n->u.vbuiltin->info->mark_proc)
	    (*n->u.vbuiltin->info->mark_proc) (
					n->u.vbuiltin->info,
					n->u.vbuiltin->instance_context);
	}
      break;

    case JS_FUNC:
      js_vm_mark_ptr (n->u.vfunction);
      js_vm_mark_ptr (n->u.vfunction->implementation);
      js_vm_object_mark (n->u.vfunction->prototype);
      break;
    }
}

#define GC_TIMES 0

void
js_vm_garbage_collect (JSVirtualMachine *vm, JSNode *fp, JSNode *sp)
{
  unsigned int i;
  unsigned long bytes_in_use;
  char buf[512];
#if GC_TIMES
  clock_t start_clock;
  clock_t after_mark_clock;
  clock_t after_sweep_clock;
#endif

  if (vm->verbose > 1)
    {
      sprintf (buf,
	       "VM: heap: garbage collect: num_consts=%u, num_globals=%u%s",
	       vm->num_consts, vm->num_globals,
	       JS_HOST_LINE_BREAK);
      js_iostream_write (vm->s_stderr, buf, strlen (buf));
    }

  vm->gc.count++;

  /* Mark */

#if GC_TIMES
  start_clock = clock ();
#endif

  /* Mark all constants. */
  for (i = 0; i < vm->num_consts; i++)
    js_vm_mark (&vm->consts[i]);

  /* Mark all globals. */
  for (i = 0; i < vm->num_globals; i++)
    js_vm_mark (&vm->globals[i]);

  /* Mark the buitin-infos of the core objects. */
  for (i = 0; i <= JS_IPTR; i++)
    js_vm_mark_ptr (vm->prim[i]);

  /* Mark stack. */

  /* STACKFRAME */

  /* Use brute force and mark the whole stack. */
  for (sp++; sp < vm->stack + vm->stack_size; sp++)
    {
      if (sp->type == JS_IPTR)
	{
	  /* Handle the stack frames here. */

	  /* Skip the return address. */
	  sp++;

	  /* Possible with-chain. */
	  if (sp->u.iptr)
	    {
	      JSUIntAlign *uip = sp->u.iptr;
	      JSUIntAlign ui = *uip;
	      JSNode *wp;

	      /* Mark the with-chain block. */
	      js_vm_mark_ptr (uip);

	      /* Mark the objects in the with-chain. */
	      wp = (JSNode *) ((unsigned char *) uip + sizeof (JSUIntAlign));

	      for (i = 0; i < ui; i++)
		js_vm_mark (&wp[i]);

	    }
	  sp++;

	  /* Skip the args_fix. */
	  sp++;

	  /*
	   * And now we point to the old_fp.  We skip it too at the
	   * for-loop.
	   */
	}
      else
	/* Mark this stack item. */
	js_vm_mark (sp);
    }

  /* Sweep all blocks and collect free nodes to the freelist. */

#if GC_TIMES
  after_mark_clock = clock ();
#endif

  bytes_in_use = sweep (vm);

#if GC_TIMES
  after_sweep_clock = clock ();
#endif

  if (vm->verbose > 1)
    {
      sprintf (buf, "VM: heap: bytes_in_use=%lu, bytes_free=%lu%s",
	       bytes_in_use, vm->gc.bytes_free,
	       JS_HOST_LINE_BREAK);
      js_iostream_write (vm->s_stderr, buf, strlen (buf));
    }

#if GC_TIMES
  if (vm->verbose > 1)
    {
      sprintf (buf, "VM: heap: mark_time=%.4f, sweep_time=%.4f%s",
	       (double) (after_mark_clock - start_clock) / CLOCKS_PER_SEC,
	       (double) (after_sweep_clock - after_mark_clock)
	       / CLOCKS_PER_SEC,
	       JS_HOST_LINE_BREAK);
      js_iostream_write (vm->s_stderr, buf, strlen (buf));
    }
#endif
}


void
js_vm_clear_heap (JSVirtualMachine *vm)
{
  /* Just sweep without marking. */
  sweep (vm);
}
