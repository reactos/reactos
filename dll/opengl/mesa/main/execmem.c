/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * \file execmem.c
 * Functions for allocating executable memory.
 *
 * \author Keith Whitwell
 */

#include <precomp.h>

#if defined(__linux__) || defined(__OpenBSD__) || defined(_NetBSD__) || defined(__sun)

/*
 * Allocate a large block of memory which can hold code then dole it out
 * in pieces by means of the generic memory manager code.
*/

#include <unistd.h>
#include <sys/mman.h>

#ifdef MESA_SELINUX
#include <selinux/selinux.h>
#endif


#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif


#define EXEC_HEAP_SIZE (10*1024*1024)

_glthread_DECLARE_STATIC_MUTEX(exec_mutex);

static struct mem_block *exec_heap = NULL;
static unsigned char *exec_mem = NULL;


static int
init_heap(void)
{
#ifdef MESA_SELINUX
   if (is_selinux_enabled()) {
      if (!security_get_boolean_active("allow_execmem") ||
	  !security_get_boolean_pending("allow_execmem"))
         return 0;
   }
#endif

   if (!exec_heap)
      exec_heap = mmInit( 0, EXEC_HEAP_SIZE );
   
   if (!exec_mem)
      exec_mem = mmap(NULL, EXEC_HEAP_SIZE, PROT_EXEC | PROT_READ | PROT_WRITE,
		      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

   return (exec_mem != MAP_FAILED);
}


void *
_mesa_exec_malloc(GLuint size)
{
   struct mem_block *block = NULL;
   void *addr = NULL;

   _glthread_LOCK_MUTEX(exec_mutex);

   if (!init_heap())
      goto bail;

   if (exec_heap) {
      size = (size + 31) & ~31;
      block = mmAllocMem( exec_heap, size, 32, 0 );
   }

   if (block)
      addr = exec_mem + block->ofs;
   else 
      printf("_mesa_exec_malloc failed\n");

bail:
   _glthread_UNLOCK_MUTEX(exec_mutex);
   
   return addr;
}

 
void 
_mesa_exec_free(void *addr)
{
   _glthread_LOCK_MUTEX(exec_mutex);

   if (exec_heap) {
      struct mem_block *block = mmFindBlock(exec_heap, (unsigned char *)addr - exec_mem);
   
      if (block)
	 mmFreeMem(block);
   }

   _glthread_UNLOCK_MUTEX(exec_mutex);
}


#else

/*
 * Just use regular memory.
 */

void *
_mesa_exec_malloc(GLuint size)
{
   return malloc( size );
}

 
void 
_mesa_exec_free(void *addr)
{
   free(addr);
}


#endif
