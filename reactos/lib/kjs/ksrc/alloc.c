/*
 * Memory allocation routines.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/ksrc/alloc.c,v $
 * $Id: alloc.c,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

#include "ddk/ntddk.h"
#include "jsint.h"

/*
 * Global functions.
 */

#if JS_DEBUG_MEMORY_LEAKS

struct mem_debug_st
{
  struct mem_debug_st *next;
  struct mem_debug_st *prev;
  char *file;
  int line;
  size_t size;
};

typedef struct mem_debug_st MemDebug;

static MemDebug *mem_debug_blocks = NULL;

int mem_debug_balance = 0;

unsigned int alloc_fail = 0;
unsigned int alloc_count = 0;

static void
register_block (MemDebug *b, size_t size, char *file, int line)
{
  b->next = NULL;
  b->prev = NULL;
  b->file = file;
  b->line = line;
  b->size = size;

  if (mem_debug_blocks)
    {
      b->next = mem_debug_blocks;
      mem_debug_blocks->prev = b;
    }

  mem_debug_blocks = b;
  mem_debug_balance++;
}


static void
unregister_block (MemDebug *b)
{
  if (b->file == NULL)
    {
      fprintf (stderr, "freeing the same block twise\n");
      abort ();
    }

  if (b->next)
    b->next->prev = b->prev;

  if (b->prev)
    b->prev->next = b->next;
  else
    mem_debug_blocks = b->next;

  b->file = NULL;

  mem_debug_balance--;
}


static inline int
check_fail ()
{
  return alloc_fail != 0 && ++alloc_count >= alloc_fail;
}


void
js_alloc_dump_blocks ()
{
  MemDebug *b;
  unsigned int bytes = 0;

  fprintf (stderr, "js_alloc_dump_blocks(): #blocks=%d\n", mem_debug_balance);

  for (b = mem_debug_blocks; b; b = b->next)
    {
      fprintf (stderr, "%s:%d: %lu\n", b->file, b->line, b->size);
      bytes += b->size;
    }

  fprintf (stderr, "leaks=%u\n", bytes);
}



void *
js_malloc_i (JSVirtualMachine *vm, size_t size, char *file, int line)
{
  MemDebug *ptr;

  ptr = ExAllocatePool (NonPagedPool,
			sizeof (MemDebug) + sizeof( size_t ) + size);
  if (check_fail () || ptr == NULL)
    {
      if (vm != NULL)
	{
	  sprintf (vm->error, "VM: memory exhausted");
	  js_vm_error (vm);
	}

      return NULL;
    }

  *((size_t *)ptr) = size;
  ptr = &((size_t *)ptr)[1];

  register_block (ptr, size, file, line);

  return (unsigned char *) ptr + sizeof (MemDebug);
}


void *
js_calloc_i (JSVirtualMachine *vm, size_t num, size_t size, char *file,
	     int line)
{
  MemDebug *ptr;

  ptr = ExAllocatePool (NonPagedPool, sizeof (MemDebug) + sizeof( int ) +  num * size);
  if (check_fail () || ptr == NULL)
    {
      if (vm != NULL)
	{
	  sprintf (vm->error, "VM: memory exhausted");
	  js_vm_error (vm);
	}

      return NULL;
    }

  *((size_t *)ptr) = size;
  ptr = &((size_t *)ptr)[1];

  memset (ptr, 0, sizeof (MemDebug) + num * size);
  register_block (ptr, num * size, file, line);

  return (unsigned char *) ptr + sizeof (MemDebug);
}


void *
js_realloc_i (JSVirtualMachine *vm, void *ptr, size_t size, char *file,
	      int line)
{
  void *nptr;
  MemDebug *b;

  if (ptr == NULL)
    return js_malloc_i (vm, size, file, line);

  nptr = js_malloc_i (vm, size, file, line);
  if (nptr == NULL)
    {
      if (vm != NULL)
	{
	  sprintf (vm->error, "VM: memory exhausted");
	  js_vm_error (vm);
	}

      return NULL;
    }

  b = (MemDebug *) ((unsigned char *) ptr - sizeof (MemDebug));

  memcpy (nptr, ptr, size < b->size ? size : b->size);

  js_free (ptr);

  return nptr;
}


void
js_free (void *ptr)
{
  MemDebug *b;

  if (ptr == NULL)
    return;

  b = (MemDebug *) ((unsigned char *) ptr - sizeof (MemDebug));
  unregister_block (b);

  ptr = &((size_t *)b)[-1];

  ExFreePool(ptr);
}


char *
js_strdup_i (JSVirtualMachine *vm, const char *str, char *file, int line)
{
  char *tmp;

  tmp = js_malloc_i (vm, strlen (str) + 1, file, line);
  if (tmp == NULL)
    return NULL;

  strcpy (tmp, str);

  return tmp;
}

#else /* not JS_DEBUG_MEMORY_LEAKS */

void *
js_malloc (JSVirtualMachine *vm, size_t size)
{
  void *ptr;

  ptr = ExAllocatePool (NonPagedPool, sizeof( size_t ) + size);
  if (ptr == NULL && vm != NULL)
    {
      sprintf (vm->error, "VM: memory exhausted");
      js_vm_error (vm);
    }

  *((size_t *)ptr) = size;
  ptr = &((size_t *)ptr)[1];

  return ptr;
}


void *
js_calloc (JSVirtualMachine *vm, size_t num, size_t size)
{
  void *ptr;

  ptr = ExAllocatePool (NonPagedPool, sizeof( size_t ) + num * size);
  if (ptr == NULL && vm != NULL)
    {
      sprintf (vm->error, "VM: memory exhausted");
      js_vm_error (vm);
    }

  *((size_t *)ptr) = size;
  ptr = &((size_t *)ptr)[1];

  RtlZeroMemory( ptr, num * size );

  return ptr;
}


void *
js_realloc (JSVirtualMachine *vm, void *ptr, size_t size)
{
  void *nptr;
  size_t old_size;

  if (ptr == NULL)
    return js_malloc (vm, size);

  nptr = js_malloc(vm, size);
  if (nptr == NULL && vm != NULL)
    {
      sprintf (vm->error, "VM: memory exhausted");
      js_vm_error (vm);
    }

  old_size = ((size_t *)ptr)[-1];

  if( size && ptr ) {
    if (size > old_size)
      size = old_size;
    RtlCopyMemory( nptr, ptr, size );
  }

  js_free(ptr);

  return nptr;
}


void
js_free (void *ptr)
{
  if (ptr == NULL)
    return;

  ptr = &((size_t *)ptr)[-1];

  ExFreePool (ptr);
}


char *
js_strdup (JSVirtualMachine *vm, const char *str)
{
  char *tmp;

  tmp = js_malloc (vm, strlen (str) + 1);
  if (tmp == NULL)
    return NULL;

  strcpy (tmp, str);

  return tmp;
}

#endif /* not JS_DEBUG_MEMORY_LEAKS */
