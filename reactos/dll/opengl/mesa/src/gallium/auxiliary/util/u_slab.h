/*
 * Copyright 2010 Marek Olšák <maraeo@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

/**
 * @file
 * Simple slab allocator for equally sized memory allocations.
 * util_slab_alloc and util_slab_free have time complexity in O(1).
 *
 * Good for allocations which have very low lifetime and are allocated
 * and freed very often. Use a profiler first to know if it's worth using it!
 *
 * Candidates: get_transfer, user_buffer_create
 *
 * @author Marek Olšák
 */

#ifndef U_SLAB_H
#define U_SLAB_H

#include "os/os_thread.h"

enum util_slab_threading {
   UTIL_SLAB_SINGLETHREADED = FALSE,
   UTIL_SLAB_MULTITHREADED = TRUE
};

/* The page is an array of blocks (allocations). */
struct util_slab_page {
   /* The header (linked-list pointers). */
   struct util_slab_page *prev, *next;

   /* Memory after the last member is dedicated to the page itself.
    * The allocated size is always larger than this structure. */
};

struct util_slab_mempool {
   /* Public members. */
   void *(*alloc)(struct util_slab_mempool *pool);
   void (*free)(struct util_slab_mempool *pool, void *ptr);

   /* Private members. */
   struct util_slab_block *first_free;

   struct util_slab_page list;

   unsigned block_size;
   unsigned page_size;
   unsigned num_blocks;
   unsigned num_pages;
   enum util_slab_threading threading;

   pipe_mutex mutex;
};

void util_slab_create(struct util_slab_mempool *pool,
                      unsigned item_size,
                      unsigned num_blocks,
                      enum util_slab_threading threading);

void util_slab_destroy(struct util_slab_mempool *pool);

void util_slab_set_thread_safety(struct util_slab_mempool *pool,
                                 enum util_slab_threading threading);

#define util_slab_alloc(pool)     (pool)->alloc(pool)
#define util_slab_free(pool, ptr) (pool)->free(pool, ptr)

#endif
