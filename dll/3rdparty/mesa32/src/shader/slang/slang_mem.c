/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 2005-2007  Brian Paul   All Rights Reserved.
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
 * \file slang_mem.c
 *
 * Memory manager for GLSL compiler.  The general idea is to do all
 * allocations out of a large pool then just free the pool when done
 * compiling to avoid intricate malloc/free tracking and memory leaks.
 *
 * \author Brian Paul
 */

#include "main/context.h"
#include "main/macros.h"
#include "slang_mem.h"


#define GRANULARITY 8
#define ROUND_UP(B)  ( ((B) + (GRANULARITY - 1)) & ~(GRANULARITY - 1) )


/** If 1, use conventional malloc/free.  Helpful for debugging */
#define USE_MALLOC_FREE 0


struct slang_mempool_
{
   GLuint Size, Used, Count, Largest;
   char *Data;
   struct slang_mempool_ *Next;
};


slang_mempool *
_slang_new_mempool(GLuint initialSize)
{
   slang_mempool *pool = (slang_mempool *) _mesa_calloc(sizeof(slang_mempool));
   if (pool) {
      pool->Data = (char *) _mesa_calloc(initialSize);
      /*printf("ALLOC MEMPOOL %d at %p\n", initialSize, pool->Data);*/
      if (!pool->Data) {
         _mesa_free(pool);
         return NULL;
      }
      pool->Size = initialSize;
      pool->Used = 0;
   }
   return pool;
}


void
_slang_delete_mempool(slang_mempool *pool)
{
   GLuint total = 0;
   while (pool) {
      slang_mempool *next = pool->Next;
      /*
      printf("DELETE MEMPOOL %u / %u  count=%u largest=%u\n",
             pool->Used, pool->Size, pool->Count, pool->Largest);
      */
      total += pool->Used;
      _mesa_free(pool->Data);
      _mesa_free(pool);
      pool = next;
   }
   /*printf("TOTAL ALLOCATED: %u\n", total);*/
}


#ifdef DEBUG
static void
check_zero(const char *addr, GLuint n)
{
   GLuint i;
   for (i = 0; i < n; i++) {
      assert(addr[i]==0);
   }
}
#endif


#ifdef DEBUG
static GLboolean
is_valid_address(const slang_mempool *pool, void *addr)
{
   while (pool) {
      if ((char *) addr >= pool->Data &&
          (char *) addr < pool->Data + pool->Used)
         return GL_TRUE;

      pool = pool->Next;
   }
   return GL_FALSE;
}
#endif


/**
 * Alloc 'bytes' from shader mempool.
 */
void *
_slang_alloc(GLuint bytes)
{
#if USE_MALLOC_FREE
   return _mesa_calloc(bytes);
#else
   slang_mempool *pool;
   GET_CURRENT_CONTEXT(ctx);
   pool = (slang_mempool *) ctx->Shader.MemPool;

   if (bytes == 0)
      bytes = 1;

   while (pool) {
      if (pool->Used + bytes <= pool->Size) {
         /* found room */
         void *addr = (void *) (pool->Data + pool->Used);
#ifdef DEBUG
         check_zero((char*) addr, bytes);
#endif
         pool->Used += ROUND_UP(bytes);
         pool->Largest = MAX2(pool->Largest, bytes);
         pool->Count++;
         /*printf("alloc %u  Used %u\n", bytes, pool->Used);*/
         return addr;
      }
      else if (pool->Next) {
         /* try next block */
         pool = pool->Next;
      }
      else {
         /* alloc new pool */
         const GLuint sz = MAX2(bytes, pool->Size);
         pool->Next = _slang_new_mempool(sz);
         if (!pool->Next) {
            /* we're _really_ out of memory */
            return NULL;
         }
         else {
            pool = pool->Next;
            pool->Largest = bytes;
            pool->Count++;
            pool->Used = ROUND_UP(bytes);
#ifdef DEBUG
            check_zero((char*) pool->Data, bytes);
#endif
            return (void *) pool->Data;
         }
      }
   }
   return NULL;
#endif
}


void *
_slang_realloc(void *oldBuffer, GLuint oldSize, GLuint newSize)
{
#if USE_MALLOC_FREE
   return _mesa_realloc(oldBuffer, oldSize, newSize);
#else
   GET_CURRENT_CONTEXT(ctx);
   slang_mempool *pool = (slang_mempool *) ctx->Shader.MemPool;
   (void) pool;

   if (newSize < oldSize) {
      return oldBuffer;
   }
   else {
      const GLuint copySize = (oldSize < newSize) ? oldSize : newSize;
      void *newBuffer = _slang_alloc(newSize);

      if (oldBuffer)
         ASSERT(is_valid_address(pool, oldBuffer));

      if (newBuffer && oldBuffer && copySize > 0)
         _mesa_memcpy(newBuffer, oldBuffer, copySize);

      return newBuffer;
   }
#endif
}


/**
 * Clone string, storing in current mempool.
 */
char *
_slang_strdup(const char *s)
{
   if (s) {
      size_t l = _mesa_strlen(s);
      char *s2 = (char *) _slang_alloc(l + 1);
      if (s2)
         _mesa_strcpy(s2, s);
      return s2;
   }
   else {
      return NULL;
   }
}


/**
 * Don't actually free memory, but mark it (for debugging).
 */
void
_slang_free(void *addr)
{
#if USE_MALLOC_FREE
   _mesa_free(addr);
#else
   if (addr) {
      GET_CURRENT_CONTEXT(ctx);
      slang_mempool *pool = (slang_mempool *) ctx->Shader.MemPool;
      (void) pool;
      ASSERT(is_valid_address(pool, addr));
   }
#endif
}
