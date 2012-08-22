/**************************************************************************
 *
 * Copyright 2010 Luca Barbieri
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef U_DYNARRAY_H
#define U_DYNARRAY_H

#include "pipe/p_compiler.h"
#include "util/u_memory.h"

/* A zero-initialized version of this is guaranteed to represent an
 * empty array.
 *
 * Also, size <= capacity and data != 0 if and only if capacity != 0
 * capacity will always be the allocation size of data
 */
struct util_dynarray
{
   void *data;
   unsigned size;
   unsigned capacity;
};

static INLINE void
util_dynarray_init(struct util_dynarray *buf)
{
   memset(buf, 0, sizeof(*buf));
}

static INLINE void
util_dynarray_fini(struct util_dynarray *buf)
{
   if(buf->data)
   {
      FREE(buf->data);
      util_dynarray_init(buf);
   }
}

/* use util_dynarray_trim to reduce the allocated storage */
static INLINE void *
util_dynarray_resize(struct util_dynarray *buf, unsigned newsize)
{
   char *p;
   if(newsize > buf->capacity)
   {
      unsigned newcap = buf->capacity << 1;
      if(newsize > newcap)
	      newcap = newsize;
      buf->data = REALLOC(buf->data, buf->capacity, newcap);
      buf->capacity = newcap;
   }

   p = (char *)buf->data + buf->size;
   buf->size = newsize;
   return p;
}

static INLINE void *
util_dynarray_grow(struct util_dynarray *buf, int diff)
{
   return util_dynarray_resize(buf, buf->size + diff);
}

static INLINE void
util_dynarray_trim(struct util_dynarray *buf)
{
   if (buf->size != buf->capacity) {
      if (buf->size) {
         buf->data = REALLOC(buf->data, buf->capacity, buf->size);
         buf->capacity = buf->size;
      }
      else {
         FREE(buf->data);
         buf->data = 0;
         buf->capacity = 0;
      }
   }
}

#define util_dynarray_append(buf, type, v) do {type __v = (v); memcpy(util_dynarray_grow((buf), sizeof(type)), &__v, sizeof(type));} while(0)
#define util_dynarray_top_ptr(buf, type) (type*)((char*)(buf)->data + (buf)->size - sizeof(type))
#define util_dynarray_top(buf, type) *util_dynarray_top_ptr(buf, type)
#define util_dynarray_pop_ptr(buf, type) (type*)((char*)(buf)->data + ((buf)->size -= sizeof(type)))
#define util_dynarray_pop(buf, type) *util_dynarray_pop_ptr(buf, type)
#define util_dynarray_contains(buf, type) ((buf)->size >= sizeof(type))
#define util_dynarray_element(buf, type, idx) ((type*)(buf)->data + (idx))
#define util_dynarray_begin(buf) ((buf)->data)
#define util_dynarray_end(buf) ((void*)util_dynarray_element((buf), char, (buf)->size))

#endif /* U_DYNARRAY_H */

