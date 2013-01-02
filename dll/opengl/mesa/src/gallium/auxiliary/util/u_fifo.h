/**************************************************************************
 *
 * Copyright © 2009 Jakob Bornecrantz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef U_FIFO_H
#define U_FIFO_H

#include "util/u_memory.h"

struct util_fifo
{
   size_t head;
   size_t tail;
   size_t num;
   size_t size;
};

static INLINE struct util_fifo *
u_fifo_create(size_t size)
{
   struct util_fifo *fifo;
   fifo = MALLOC(sizeof(*fifo) + size * sizeof(void*));

   fifo->head = 0;
   fifo->tail = 0;
   fifo->num = 0;
   fifo->size = size;

   return fifo;
}

static INLINE boolean
u_fifo_add(struct util_fifo *fifo, void *ptr)
{
   void **array = (void**)&fifo[1];
   if (fifo->num >= fifo->size)
      return FALSE;

   if (++fifo->head >= fifo->size)
      fifo->head = 0;

   array[fifo->head] = ptr;

   ++fifo->num;

   return TRUE;
}

static INLINE boolean
u_fifo_pop(struct util_fifo *fifo, void **ptr)
{
   void **array = (void**)&fifo[1];

   if (!fifo->num)
      return FALSE;

   if (++fifo->tail >= fifo->size)
      fifo->tail = 0;

   *ptr = array[fifo->tail];

   ++fifo->num;

   return TRUE;
}

static INLINE void
u_fifo_destroy(struct util_fifo *fifo)
{
   FREE(fifo);
}

#endif
