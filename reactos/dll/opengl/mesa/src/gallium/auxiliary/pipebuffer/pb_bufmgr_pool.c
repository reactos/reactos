/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Bismarck, ND., USA
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * 
 **************************************************************************/

/**
 * \file
 * Batch buffer pool management.
 * 
 * \author Jose Fonseca <jrfonseca-at-tungstengraphics-dot-com>
 * \author Thomas Hellström <thomas-at-tungstengraphics-dot-com>
 */


#include "pipe/p_compiler.h"
#include "util/u_debug.h"
#include "os/os_thread.h"
#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "util/u_double_list.h"

#include "pb_buffer.h"
#include "pb_bufmgr.h"


/**
 * Convenience macro (type safe).
 */
#define SUPER(__derived) (&(__derived)->base)


struct pool_pb_manager
{
   struct pb_manager base;
   
   pipe_mutex mutex;
   
   pb_size bufSize;
   pb_size bufAlign;
   
   pb_size numFree;
   pb_size numTot;
   
   struct list_head free;
   
   struct pb_buffer *buffer;
   void *map;
   
   struct pool_buffer *bufs;
};


static INLINE struct pool_pb_manager *
pool_pb_manager(struct pb_manager *mgr)
{
   assert(mgr);
   return (struct pool_pb_manager *)mgr;
}


struct pool_buffer
{
   struct pb_buffer base;
   
   struct pool_pb_manager *mgr;
   
   struct list_head head;
   
   pb_size start;
};


static INLINE struct pool_buffer *
pool_buffer(struct pb_buffer *buf)
{
   assert(buf);
   return (struct pool_buffer *)buf;
}



static void
pool_buffer_destroy(struct pb_buffer *buf)
{
   struct pool_buffer *pool_buf = pool_buffer(buf);
   struct pool_pb_manager *pool = pool_buf->mgr;
   
   assert(!pipe_is_referenced(&pool_buf->base.reference));

   pipe_mutex_lock(pool->mutex);
   LIST_ADD(&pool_buf->head, &pool->free);
   pool->numFree++;
   pipe_mutex_unlock(pool->mutex);
}


static void *
pool_buffer_map(struct pb_buffer *buf, unsigned flags, void *flush_ctx)
{
   struct pool_buffer *pool_buf = pool_buffer(buf);
   struct pool_pb_manager *pool = pool_buf->mgr;
   void *map;

   /* XXX: it will be necessary to remap here to propagate flush_ctx */

   pipe_mutex_lock(pool->mutex);
   map = (unsigned char *) pool->map + pool_buf->start;
   pipe_mutex_unlock(pool->mutex);
   return map;
}


static void
pool_buffer_unmap(struct pb_buffer *buf)
{
   /* No-op */
}


static enum pipe_error 
pool_buffer_validate(struct pb_buffer *buf, 
                     struct pb_validate *vl,
                     unsigned flags)
{
   struct pool_buffer *pool_buf = pool_buffer(buf);
   struct pool_pb_manager *pool = pool_buf->mgr;
   return pb_validate(pool->buffer, vl, flags);
}


static void
pool_buffer_fence(struct pb_buffer *buf, 
                  struct pipe_fence_handle *fence)
{
   struct pool_buffer *pool_buf = pool_buffer(buf);
   struct pool_pb_manager *pool = pool_buf->mgr;
   pb_fence(pool->buffer, fence);
}


static void
pool_buffer_get_base_buffer(struct pb_buffer *buf,
                            struct pb_buffer **base_buf,
                            pb_size *offset)
{
   struct pool_buffer *pool_buf = pool_buffer(buf);
   struct pool_pb_manager *pool = pool_buf->mgr;
   pb_get_base_buffer(pool->buffer, base_buf, offset);
   *offset += pool_buf->start;
}


static const struct pb_vtbl 
pool_buffer_vtbl = {
      pool_buffer_destroy,
      pool_buffer_map,
      pool_buffer_unmap,
      pool_buffer_validate,
      pool_buffer_fence,
      pool_buffer_get_base_buffer
};


static struct pb_buffer *
pool_bufmgr_create_buffer(struct pb_manager *mgr,
                          pb_size size,
                          const struct pb_desc *desc)
{
   struct pool_pb_manager *pool = pool_pb_manager(mgr);
   struct pool_buffer *pool_buf;
   struct list_head *item;

   assert(size == pool->bufSize);
   assert(pool->bufAlign % desc->alignment == 0);
   
   pipe_mutex_lock(pool->mutex);

   if (pool->numFree == 0) {
      pipe_mutex_unlock(pool->mutex);
      debug_printf("warning: out of fixed size buffer objects\n");
      return NULL;
   }

   item = pool->free.next;

   if (item == &pool->free) {
      pipe_mutex_unlock(pool->mutex);
      debug_printf("error: fixed size buffer pool corruption\n");
      return NULL;
   }

   LIST_DEL(item);
   --pool->numFree;

   pipe_mutex_unlock(pool->mutex);
   
   pool_buf = LIST_ENTRY(struct pool_buffer, item, head);
   assert(!pipe_is_referenced(&pool_buf->base.reference));
   pipe_reference_init(&pool_buf->base.reference, 1);
   pool_buf->base.alignment = desc->alignment;
   pool_buf->base.usage = desc->usage;
   
   return SUPER(pool_buf);
}


static void
pool_bufmgr_flush(struct pb_manager *mgr)
{
   /* No-op */
}


static void
pool_bufmgr_destroy(struct pb_manager *mgr)
{
   struct pool_pb_manager *pool = pool_pb_manager(mgr);
   pipe_mutex_lock(pool->mutex);

   FREE(pool->bufs);
   
   pb_unmap(pool->buffer);
   pb_reference(&pool->buffer, NULL);
   
   pipe_mutex_unlock(pool->mutex);
   
   FREE(mgr);
}


struct pb_manager *
pool_bufmgr_create(struct pb_manager *provider, 
                   pb_size numBufs, 
                   pb_size bufSize,
                   const struct pb_desc *desc) 
{
   struct pool_pb_manager *pool;
   struct pool_buffer *pool_buf;
   pb_size i;

   if(!provider)
      return NULL;
   
   pool = CALLOC_STRUCT(pool_pb_manager);
   if (!pool)
      return NULL;

   pool->base.destroy = pool_bufmgr_destroy;
   pool->base.create_buffer = pool_bufmgr_create_buffer;
   pool->base.flush = pool_bufmgr_flush;

   LIST_INITHEAD(&pool->free);

   pool->numTot = numBufs;
   pool->numFree = numBufs;
   pool->bufSize = bufSize;
   pool->bufAlign = desc->alignment; 
   
   pipe_mutex_init(pool->mutex);

   pool->buffer = provider->create_buffer(provider, numBufs*bufSize, desc); 
   if (!pool->buffer)
      goto failure;

   pool->map = pb_map(pool->buffer,
                          PB_USAGE_CPU_READ |
                          PB_USAGE_CPU_WRITE, NULL);
   if(!pool->map)
      goto failure;

   pool->bufs = (struct pool_buffer *)CALLOC(numBufs, sizeof(*pool->bufs));
   if (!pool->bufs)
      goto failure;

   pool_buf = pool->bufs;
   for (i = 0; i < numBufs; ++i) {
      pipe_reference_init(&pool_buf->base.reference, 0);
      pool_buf->base.alignment = 0;
      pool_buf->base.usage = 0;
      pool_buf->base.size = bufSize;
      pool_buf->base.vtbl = &pool_buffer_vtbl;
      pool_buf->mgr = pool;
      pool_buf->start = i * bufSize;
      LIST_ADDTAIL(&pool_buf->head, &pool->free);
      pool_buf++;
   }

   return SUPER(pool);
   
failure:
   if(pool->bufs)
      FREE(pool->bufs);
   if(pool->map)
      pb_unmap(pool->buffer);
   if(pool->buffer)
      pb_reference(&pool->buffer, NULL);
   if(pool)
      FREE(pool);
   return NULL;
}
