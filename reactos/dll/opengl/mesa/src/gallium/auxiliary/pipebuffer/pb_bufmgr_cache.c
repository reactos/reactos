/**************************************************************************
 *
 * Copyright 2007-2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * \file
 * Buffer cache.
 * 
 * \author Jose Fonseca <jrfonseca-at-tungstengraphics-dot-com>
 * \author Thomas Hellström <thomas-at-tungstengraphics-dot-com>
 */


#include "pipe/p_compiler.h"
#include "util/u_debug.h"
#include "os/os_thread.h"
#include "util/u_memory.h"
#include "util/u_double_list.h"
#include "util/u_time.h"

#include "pb_buffer.h"
#include "pb_bufmgr.h"


/**
 * Convenience macro (type safe).
 */
#define SUPER(__derived) (&(__derived)->base)


struct pb_cache_manager;


/**
 * Wrapper around a pipe buffer which adds delayed destruction.
 */
struct pb_cache_buffer
{
   struct pb_buffer base;
   
   struct pb_buffer *buffer;
   struct pb_cache_manager *mgr;

   /** Caching time interval */
   int64_t start, end;

   struct list_head head;
};


struct pb_cache_manager
{
   struct pb_manager base;

   struct pb_manager *provider;
   unsigned usecs;
   
   pipe_mutex mutex;
   
   struct list_head delayed;
   pb_size numDelayed;
};


static INLINE struct pb_cache_buffer *
pb_cache_buffer(struct pb_buffer *buf)
{
   assert(buf);
   return (struct pb_cache_buffer *)buf;
}


static INLINE struct pb_cache_manager *
pb_cache_manager(struct pb_manager *mgr)
{
   assert(mgr);
   return (struct pb_cache_manager *)mgr;
}


/**
 * Actually destroy the buffer.
 */
static INLINE void
_pb_cache_buffer_destroy(struct pb_cache_buffer *buf)
{
   struct pb_cache_manager *mgr = buf->mgr;

   LIST_DEL(&buf->head);
   assert(mgr->numDelayed);
   --mgr->numDelayed;
   assert(!pipe_is_referenced(&buf->base.reference));
   pb_reference(&buf->buffer, NULL);
   FREE(buf);
}


/**
 * Free as many cache buffers from the list head as possible. 
 */
static void
_pb_cache_buffer_list_check_free(struct pb_cache_manager *mgr)
{
   struct list_head *curr, *next;
   struct pb_cache_buffer *buf;
   int64_t now;
   
   now = os_time_get();
   
   curr = mgr->delayed.next;
   next = curr->next;
   while(curr != &mgr->delayed) {
      buf = LIST_ENTRY(struct pb_cache_buffer, curr, head);

      if(!os_time_timeout(buf->start, buf->end, now))
	 break;
	 
      _pb_cache_buffer_destroy(buf);

      curr = next; 
      next = curr->next;
   }
}


static void
pb_cache_buffer_destroy(struct pb_buffer *_buf)
{
   struct pb_cache_buffer *buf = pb_cache_buffer(_buf);   
   struct pb_cache_manager *mgr = buf->mgr;

   pipe_mutex_lock(mgr->mutex);
   assert(!pipe_is_referenced(&buf->base.reference));
   
   _pb_cache_buffer_list_check_free(mgr);
   
   buf->start = os_time_get();
   buf->end = buf->start + mgr->usecs;
   LIST_ADDTAIL(&buf->head, &mgr->delayed);
   ++mgr->numDelayed;
   pipe_mutex_unlock(mgr->mutex);
}


static void *
pb_cache_buffer_map(struct pb_buffer *_buf, 
		    unsigned flags, void *flush_ctx)
{
   struct pb_cache_buffer *buf = pb_cache_buffer(_buf);   
   return pb_map(buf->buffer, flags, flush_ctx);
}


static void
pb_cache_buffer_unmap(struct pb_buffer *_buf)
{
   struct pb_cache_buffer *buf = pb_cache_buffer(_buf);   
   pb_unmap(buf->buffer);
}


static enum pipe_error 
pb_cache_buffer_validate(struct pb_buffer *_buf, 
                         struct pb_validate *vl,
                         unsigned flags)
{
   struct pb_cache_buffer *buf = pb_cache_buffer(_buf);
   return pb_validate(buf->buffer, vl, flags);
}


static void
pb_cache_buffer_fence(struct pb_buffer *_buf, 
                      struct pipe_fence_handle *fence)
{
   struct pb_cache_buffer *buf = pb_cache_buffer(_buf);
   pb_fence(buf->buffer, fence);
}


static void
pb_cache_buffer_get_base_buffer(struct pb_buffer *_buf,
                              struct pb_buffer **base_buf,
                              pb_size *offset)
{
   struct pb_cache_buffer *buf = pb_cache_buffer(_buf);
   pb_get_base_buffer(buf->buffer, base_buf, offset);
}


const struct pb_vtbl 
pb_cache_buffer_vtbl = {
      pb_cache_buffer_destroy,
      pb_cache_buffer_map,
      pb_cache_buffer_unmap,
      pb_cache_buffer_validate,
      pb_cache_buffer_fence,
      pb_cache_buffer_get_base_buffer
};


static INLINE int
pb_cache_is_buffer_compat(struct pb_cache_buffer *buf,  
                          pb_size size,
                          const struct pb_desc *desc)
{
   if(buf->base.size < size)
      return 0;

   /* be lenient with size */
   if(buf->base.size >= 2*size)
      return 0;
   
   if(!pb_check_alignment(desc->alignment, buf->base.alignment))
      return 0;
   
   if(!pb_check_usage(desc->usage, buf->base.usage))
      return 0;

   if (buf->mgr->provider->is_buffer_busy) {
      if (buf->mgr->provider->is_buffer_busy(buf->mgr->provider, buf->buffer))
         return -1;
   } else {
      void *ptr = pb_map(buf->buffer, PB_USAGE_DONTBLOCK, NULL);

      if (!ptr)
         return -1;

      pb_unmap(buf->buffer);
   }

   return 1;
}


static struct pb_buffer *
pb_cache_manager_create_buffer(struct pb_manager *_mgr, 
                               pb_size size,
                               const struct pb_desc *desc)
{
   struct pb_cache_manager *mgr = pb_cache_manager(_mgr);
   struct pb_cache_buffer *buf;
   struct pb_cache_buffer *curr_buf;
   struct list_head *curr, *next;
   int64_t now;
   int ret = 0;

   pipe_mutex_lock(mgr->mutex);

   buf = NULL;
   curr = mgr->delayed.next;
   next = curr->next;
   
   /* search in the expired buffers, freeing them in the process */
   now = os_time_get();
   while(curr != &mgr->delayed) {
      curr_buf = LIST_ENTRY(struct pb_cache_buffer, curr, head);
      if(!buf && (ret = pb_cache_is_buffer_compat(curr_buf, size, desc) > 0))
         buf = curr_buf;
      else if(os_time_timeout(curr_buf->start, curr_buf->end, now))
         _pb_cache_buffer_destroy(curr_buf);
      else
         /* This buffer (and all hereafter) are still hot in cache */
         break;
      if (ret == -1)
         break;
      curr = next; 
      next = curr->next;
   }

   /* keep searching in the hot buffers */
   if(!buf && ret != -1) {
      while(curr != &mgr->delayed) {
         curr_buf = LIST_ENTRY(struct pb_cache_buffer, curr, head);
         ret = pb_cache_is_buffer_compat(curr_buf, size, desc);
         if (ret > 0) {
            buf = curr_buf;
            break;
         }
         if (ret == -1)
            break;
         /* no need to check the timeout here */
         curr = next;
         next = curr->next;
      }
   }
   
   if(buf) {
      LIST_DEL(&buf->head);
      --mgr->numDelayed;
      pipe_mutex_unlock(mgr->mutex);
      /* Increase refcount */
      pipe_reference_init(&buf->base.reference, 1);
      return &buf->base;
   }
   
   pipe_mutex_unlock(mgr->mutex);

   buf = CALLOC_STRUCT(pb_cache_buffer);
   if(!buf)
      return NULL;
   
   buf->buffer = mgr->provider->create_buffer(mgr->provider, size, desc);

   /* Empty the cache and try again. */
   if (!buf->buffer) {
      mgr->base.flush(&mgr->base);
      buf->buffer = mgr->provider->create_buffer(mgr->provider, size, desc);
   }

   if(!buf->buffer) {
      FREE(buf);
      return NULL;
   }
   
   assert(pipe_is_referenced(&buf->buffer->reference));
   assert(pb_check_alignment(desc->alignment, buf->buffer->alignment));
   assert(pb_check_usage(desc->usage, buf->buffer->usage));
   assert(buf->buffer->size >= size);
   
   pipe_reference_init(&buf->base.reference, 1);
   buf->base.alignment = buf->buffer->alignment;
   buf->base.usage = buf->buffer->usage;
   buf->base.size = buf->buffer->size;
   
   buf->base.vtbl = &pb_cache_buffer_vtbl;
   buf->mgr = mgr;
   
   return &buf->base;
}


static void
pb_cache_manager_flush(struct pb_manager *_mgr)
{
   struct pb_cache_manager *mgr = pb_cache_manager(_mgr);
   struct list_head *curr, *next;
   struct pb_cache_buffer *buf;

   pipe_mutex_lock(mgr->mutex);
   curr = mgr->delayed.next;
   next = curr->next;
   while(curr != &mgr->delayed) {
      buf = LIST_ENTRY(struct pb_cache_buffer, curr, head);
      _pb_cache_buffer_destroy(buf);
      curr = next; 
      next = curr->next;
   }
   pipe_mutex_unlock(mgr->mutex);
   
   assert(mgr->provider->flush);
   if(mgr->provider->flush)
      mgr->provider->flush(mgr->provider);
}


static void
pb_cache_manager_destroy(struct pb_manager *mgr)
{
   pb_cache_manager_flush(mgr);
   FREE(mgr);
}


struct pb_manager *
pb_cache_manager_create(struct pb_manager *provider, 
                     	unsigned usecs) 
{
   struct pb_cache_manager *mgr;

   if(!provider)
      return NULL;
   
   mgr = CALLOC_STRUCT(pb_cache_manager);
   if (!mgr)
      return NULL;

   mgr->base.destroy = pb_cache_manager_destroy;
   mgr->base.create_buffer = pb_cache_manager_create_buffer;
   mgr->base.flush = pb_cache_manager_flush;
   mgr->provider = provider;
   mgr->usecs = usecs;
   LIST_INITHEAD(&mgr->delayed);
   mgr->numDelayed = 0;
   pipe_mutex_init(mgr->mutex);
      
   return &mgr->base;
}
