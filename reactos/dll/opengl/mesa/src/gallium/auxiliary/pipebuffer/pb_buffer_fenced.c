/**************************************************************************
 *
 * Copyright 2007-2010 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * \file
 * Implementation of fenced buffers.
 *
 * \author Jose Fonseca <jfonseca-at-vmware-dot-com>
 * \author Thomas Hellström <thellstrom-at-vmware-dot-com>
 */


#include "pipe/p_config.h"

#if defined(PIPE_OS_LINUX) || defined(PIPE_OS_BSD) || defined(PIPE_OS_SOLARIS)
#include <unistd.h>
#include <sched.h>
#endif

#include "pipe/p_compiler.h"
#include "pipe/p_defines.h"
#include "util/u_debug.h"
#include "os/os_thread.h"
#include "util/u_memory.h"
#include "util/u_double_list.h"

#include "pb_buffer.h"
#include "pb_buffer_fenced.h"
#include "pb_bufmgr.h"



/**
 * Convenience macro (type safe).
 */
#define SUPER(__derived) (&(__derived)->base)


struct fenced_manager
{
   struct pb_manager base;
   struct pb_manager *provider;
   struct pb_fence_ops *ops;

   /**
    * Maximum buffer size that can be safely allocated.
    */
   pb_size max_buffer_size;

   /**
    * Maximum cpu memory we can allocate before we start waiting for the
    * GPU to idle.
    */
   pb_size max_cpu_total_size;

   /**
    * Following members are mutable and protected by this mutex.
    */
   pipe_mutex mutex;

   /**
    * Fenced buffer list.
    *
    * All fenced buffers are placed in this listed, ordered from the oldest
    * fence to the newest fence.
    */
   struct list_head fenced;
   pb_size num_fenced;

   struct list_head unfenced;
   pb_size num_unfenced;

   /**
    * How much temporary CPU memory is being used to hold unvalidated buffers.
    */
   pb_size cpu_total_size;
};


/**
 * Fenced buffer.
 *
 * Wrapper around a pipe buffer which adds fencing and reference counting.
 */
struct fenced_buffer
{
   /*
    * Immutable members.
    */

   struct pb_buffer base;
   struct fenced_manager *mgr;

   /*
    * Following members are mutable and protected by fenced_manager::mutex.
    */

   struct list_head head;

   /**
    * Buffer with storage.
    */
   struct pb_buffer *buffer;
   pb_size size;
   struct pb_desc desc;

   /**
    * Temporary CPU storage data. Used when there isn't enough GPU memory to
    * store the buffer.
    */
   void *data;

   /**
    * A bitmask of PB_USAGE_CPU/GPU_READ/WRITE describing the current
    * buffer usage.
    */
   unsigned flags;

   unsigned mapcount;

   struct pb_validate *vl;
   unsigned validation_flags;

   struct pipe_fence_handle *fence;
};


static INLINE struct fenced_manager *
fenced_manager(struct pb_manager *mgr)
{
   assert(mgr);
   return (struct fenced_manager *)mgr;
}


static INLINE struct fenced_buffer *
fenced_buffer(struct pb_buffer *buf)
{
   assert(buf);
   return (struct fenced_buffer *)buf;
}


static void
fenced_buffer_destroy_cpu_storage_locked(struct fenced_buffer *fenced_buf);

static enum pipe_error
fenced_buffer_create_cpu_storage_locked(struct fenced_manager *fenced_mgr,
                                        struct fenced_buffer *fenced_buf);

static void
fenced_buffer_destroy_gpu_storage_locked(struct fenced_buffer *fenced_buf);

static enum pipe_error
fenced_buffer_create_gpu_storage_locked(struct fenced_manager *fenced_mgr,
                                        struct fenced_buffer *fenced_buf,
                                        boolean wait);

static enum pipe_error
fenced_buffer_copy_storage_to_gpu_locked(struct fenced_buffer *fenced_buf);

static enum pipe_error
fenced_buffer_copy_storage_to_cpu_locked(struct fenced_buffer *fenced_buf);


/**
 * Dump the fenced buffer list.
 *
 * Useful to understand failures to allocate buffers.
 */
static void
fenced_manager_dump_locked(struct fenced_manager *fenced_mgr)
{
#ifdef DEBUG
   struct pb_fence_ops *ops = fenced_mgr->ops;
   struct list_head *curr, *next;
   struct fenced_buffer *fenced_buf;

   debug_printf("%10s %7s %8s %7s %10s %s\n",
                "buffer", "size", "refcount", "storage", "fence", "signalled");

   curr = fenced_mgr->unfenced.next;
   next = curr->next;
   while(curr != &fenced_mgr->unfenced) {
      fenced_buf = LIST_ENTRY(struct fenced_buffer, curr, head);
      assert(!fenced_buf->fence);
      debug_printf("%10p %7u %8u %7s\n",
                   (void *) fenced_buf,
                   fenced_buf->base.size,
                   p_atomic_read(&fenced_buf->base.reference.count),
                   fenced_buf->buffer ? "gpu" : (fenced_buf->data ? "cpu" : "none"));
      curr = next;
      next = curr->next;
   }

   curr = fenced_mgr->fenced.next;
   next = curr->next;
   while(curr != &fenced_mgr->fenced) {
      int signaled;
      fenced_buf = LIST_ENTRY(struct fenced_buffer, curr, head);
      assert(fenced_buf->buffer);
      signaled = ops->fence_signalled(ops, fenced_buf->fence, 0);
      debug_printf("%10p %7u %8u %7s %10p %s\n",
                   (void *) fenced_buf,
                   fenced_buf->base.size,
                   p_atomic_read(&fenced_buf->base.reference.count),
                   "gpu",
                   (void *) fenced_buf->fence,
                   signaled == 0 ? "y" : "n");
      curr = next;
      next = curr->next;
   }
#else
   (void)fenced_mgr;
#endif
}


static INLINE void
fenced_buffer_destroy_locked(struct fenced_manager *fenced_mgr,
                             struct fenced_buffer *fenced_buf)
{
   assert(!pipe_is_referenced(&fenced_buf->base.reference));

   assert(!fenced_buf->fence);
   assert(fenced_buf->head.prev);
   assert(fenced_buf->head.next);
   LIST_DEL(&fenced_buf->head);
   assert(fenced_mgr->num_unfenced);
   --fenced_mgr->num_unfenced;

   fenced_buffer_destroy_gpu_storage_locked(fenced_buf);
   fenced_buffer_destroy_cpu_storage_locked(fenced_buf);

   FREE(fenced_buf);
}


/**
 * Add the buffer to the fenced list.
 *
 * Reference count should be incremented before calling this function.
 */
static INLINE void
fenced_buffer_add_locked(struct fenced_manager *fenced_mgr,
                         struct fenced_buffer *fenced_buf)
{
   assert(pipe_is_referenced(&fenced_buf->base.reference));
   assert(fenced_buf->flags & PB_USAGE_GPU_READ_WRITE);
   assert(fenced_buf->fence);

   p_atomic_inc(&fenced_buf->base.reference.count);

   LIST_DEL(&fenced_buf->head);
   assert(fenced_mgr->num_unfenced);
   --fenced_mgr->num_unfenced;
   LIST_ADDTAIL(&fenced_buf->head, &fenced_mgr->fenced);
   ++fenced_mgr->num_fenced;
}


/**
 * Remove the buffer from the fenced list, and potentially destroy the buffer
 * if the reference count reaches zero.
 *
 * Returns TRUE if the buffer was detroyed.
 */
static INLINE boolean
fenced_buffer_remove_locked(struct fenced_manager *fenced_mgr,
                            struct fenced_buffer *fenced_buf)
{
   struct pb_fence_ops *ops = fenced_mgr->ops;

   assert(fenced_buf->fence);
   assert(fenced_buf->mgr == fenced_mgr);

   ops->fence_reference(ops, &fenced_buf->fence, NULL);
   fenced_buf->flags &= ~PB_USAGE_GPU_READ_WRITE;

   assert(fenced_buf->head.prev);
   assert(fenced_buf->head.next);

   LIST_DEL(&fenced_buf->head);
   assert(fenced_mgr->num_fenced);
   --fenced_mgr->num_fenced;

   LIST_ADDTAIL(&fenced_buf->head, &fenced_mgr->unfenced);
   ++fenced_mgr->num_unfenced;

   if (p_atomic_dec_zero(&fenced_buf->base.reference.count)) {
      fenced_buffer_destroy_locked(fenced_mgr, fenced_buf);
      return TRUE;
   }

   return FALSE;
}


/**
 * Wait for the fence to expire, and remove it from the fenced list.
 *
 * This function will release and re-aquire the mutex, so any copy of mutable
 * state must be discarded after calling it.
 */
static INLINE enum pipe_error
fenced_buffer_finish_locked(struct fenced_manager *fenced_mgr,
                            struct fenced_buffer *fenced_buf)
{
   struct pb_fence_ops *ops = fenced_mgr->ops;
   enum pipe_error ret = PIPE_ERROR;

#if 0
   debug_warning("waiting for GPU");
#endif

   assert(pipe_is_referenced(&fenced_buf->base.reference));
   assert(fenced_buf->fence);

   if(fenced_buf->fence) {
      struct pipe_fence_handle *fence = NULL;
      int finished;
      boolean proceed;

      ops->fence_reference(ops, &fence, fenced_buf->fence);

      pipe_mutex_unlock(fenced_mgr->mutex);

      finished = ops->fence_finish(ops, fenced_buf->fence, 0);

      pipe_mutex_lock(fenced_mgr->mutex);

      assert(pipe_is_referenced(&fenced_buf->base.reference));

      /*
       * Only proceed if the fence object didn't change in the meanwhile.
       * Otherwise assume the work has been already carried out by another
       * thread that re-aquired the lock before us.
       */
      proceed = fence == fenced_buf->fence ? TRUE : FALSE;

      ops->fence_reference(ops, &fence, NULL);

      if(proceed && finished == 0) {
         /*
          * Remove from the fenced list
          */

         boolean destroyed;

         destroyed = fenced_buffer_remove_locked(fenced_mgr, fenced_buf);

         /* TODO: remove consequents buffers with the same fence? */

         assert(!destroyed);

         fenced_buf->flags &= ~PB_USAGE_GPU_READ_WRITE;

         ret = PIPE_OK;
      }
   }

   return ret;
}


/**
 * Remove as many fenced buffers from the fenced list as possible.
 *
 * Returns TRUE if at least one buffer was removed.
 */
static boolean
fenced_manager_check_signalled_locked(struct fenced_manager *fenced_mgr,
                                      boolean wait)
{
   struct pb_fence_ops *ops = fenced_mgr->ops;
   struct list_head *curr, *next;
   struct fenced_buffer *fenced_buf;
   struct pipe_fence_handle *prev_fence = NULL;
   boolean ret = FALSE;

   curr = fenced_mgr->fenced.next;
   next = curr->next;
   while(curr != &fenced_mgr->fenced) {
      fenced_buf = LIST_ENTRY(struct fenced_buffer, curr, head);

      if(fenced_buf->fence != prev_fence) {
	 int signaled;

	 if (wait) {
	    signaled = ops->fence_finish(ops, fenced_buf->fence, 0);

	    /*
	     * Don't return just now. Instead preemptively check if the
	     * following buffers' fences already expired, without further waits.
	     */
	    wait = FALSE;
	 }
	 else {
	    signaled = ops->fence_signalled(ops, fenced_buf->fence, 0);
	 }

	 if (signaled != 0) {
	    return ret;
         }

	 prev_fence = fenced_buf->fence;
      }
      else {
         /* This buffer's fence object is identical to the previous buffer's
          * fence object, so no need to check the fence again.
          */
	 assert(ops->fence_signalled(ops, fenced_buf->fence, 0) == 0);
      }

      fenced_buffer_remove_locked(fenced_mgr, fenced_buf);

      ret = TRUE;

      curr = next;
      next = curr->next;
   }

   return ret;
}


/**
 * Try to free some GPU memory by backing it up into CPU memory.
 *
 * Returns TRUE if at least one buffer was freed.
 */
static boolean
fenced_manager_free_gpu_storage_locked(struct fenced_manager *fenced_mgr)
{
   struct list_head *curr, *next;
   struct fenced_buffer *fenced_buf;

   curr = fenced_mgr->unfenced.next;
   next = curr->next;
   while(curr != &fenced_mgr->unfenced) {
      fenced_buf = LIST_ENTRY(struct fenced_buffer, curr, head);

      /*
       * We can only move storage if the buffer is not mapped and not
       * validated.
       */
      if(fenced_buf->buffer &&
         !fenced_buf->mapcount &&
         !fenced_buf->vl) {
         enum pipe_error ret;

         ret = fenced_buffer_create_cpu_storage_locked(fenced_mgr, fenced_buf);
         if(ret == PIPE_OK) {
            ret = fenced_buffer_copy_storage_to_cpu_locked(fenced_buf);
            if(ret == PIPE_OK) {
               fenced_buffer_destroy_gpu_storage_locked(fenced_buf);
               return TRUE;
            }
            fenced_buffer_destroy_cpu_storage_locked(fenced_buf);
         }
      }

      curr = next;
      next = curr->next;
   }

   return FALSE;
}


/**
 * Destroy CPU storage for this buffer.
 */
static void
fenced_buffer_destroy_cpu_storage_locked(struct fenced_buffer *fenced_buf)
{
   if(fenced_buf->data) {
      align_free(fenced_buf->data);
      fenced_buf->data = NULL;
      assert(fenced_buf->mgr->cpu_total_size >= fenced_buf->size);
      fenced_buf->mgr->cpu_total_size -= fenced_buf->size;
   }
}


/**
 * Create CPU storage for this buffer.
 */
static enum pipe_error
fenced_buffer_create_cpu_storage_locked(struct fenced_manager *fenced_mgr,
                                        struct fenced_buffer *fenced_buf)
{
   assert(!fenced_buf->data);
   if(fenced_buf->data)
      return PIPE_OK;

   if (fenced_mgr->cpu_total_size + fenced_buf->size > fenced_mgr->max_cpu_total_size)
      return PIPE_ERROR_OUT_OF_MEMORY;

   fenced_buf->data = align_malloc(fenced_buf->size, fenced_buf->desc.alignment);
   if(!fenced_buf->data)
      return PIPE_ERROR_OUT_OF_MEMORY;

   fenced_mgr->cpu_total_size += fenced_buf->size;

   return PIPE_OK;
}


/**
 * Destroy the GPU storage.
 */
static void
fenced_buffer_destroy_gpu_storage_locked(struct fenced_buffer *fenced_buf)
{
   if(fenced_buf->buffer) {
      pb_reference(&fenced_buf->buffer, NULL);
   }
}


/**
 * Try to create GPU storage for this buffer.
 *
 * This function is a shorthand around pb_manager::create_buffer for
 * fenced_buffer_create_gpu_storage_locked()'s benefit.
 */
static INLINE boolean
fenced_buffer_try_create_gpu_storage_locked(struct fenced_manager *fenced_mgr,
                                            struct fenced_buffer *fenced_buf)
{
   struct pb_manager *provider = fenced_mgr->provider;

   assert(!fenced_buf->buffer);

   fenced_buf->buffer = provider->create_buffer(fenced_mgr->provider,
                                                fenced_buf->size,
                                                &fenced_buf->desc);
   return fenced_buf->buffer ? TRUE : FALSE;
}


/**
 * Create GPU storage for this buffer.
 */
static enum pipe_error
fenced_buffer_create_gpu_storage_locked(struct fenced_manager *fenced_mgr,
                                        struct fenced_buffer *fenced_buf,
                                        boolean wait)
{
   assert(!fenced_buf->buffer);

   /*
    * Check for signaled buffers before trying to allocate.
    */
   fenced_manager_check_signalled_locked(fenced_mgr, FALSE);

   fenced_buffer_try_create_gpu_storage_locked(fenced_mgr, fenced_buf);

   /*
    * Keep trying while there is some sort of progress:
    * - fences are expiring,
    * - or buffers are being being swapped out from GPU memory into CPU memory.
    */
   while(!fenced_buf->buffer &&
         (fenced_manager_check_signalled_locked(fenced_mgr, FALSE) ||
          fenced_manager_free_gpu_storage_locked(fenced_mgr))) {
      fenced_buffer_try_create_gpu_storage_locked(fenced_mgr, fenced_buf);
   }

   if(!fenced_buf->buffer && wait) {
      /*
       * Same as before, but this time around, wait to free buffers if
       * necessary.
       */
      while(!fenced_buf->buffer &&
            (fenced_manager_check_signalled_locked(fenced_mgr, TRUE) ||
             fenced_manager_free_gpu_storage_locked(fenced_mgr))) {
         fenced_buffer_try_create_gpu_storage_locked(fenced_mgr, fenced_buf);
      }
   }

   if(!fenced_buf->buffer) {
      if(0)
         fenced_manager_dump_locked(fenced_mgr);

      /* give up */
      return PIPE_ERROR_OUT_OF_MEMORY;
   }

   return PIPE_OK;
}


static enum pipe_error
fenced_buffer_copy_storage_to_gpu_locked(struct fenced_buffer *fenced_buf)
{
   uint8_t *map;

   assert(fenced_buf->data);
   assert(fenced_buf->buffer);

   map = pb_map(fenced_buf->buffer, PB_USAGE_CPU_WRITE, NULL);
   if(!map)
      return PIPE_ERROR;

   memcpy(map, fenced_buf->data, fenced_buf->size);

   pb_unmap(fenced_buf->buffer);

   return PIPE_OK;
}


static enum pipe_error
fenced_buffer_copy_storage_to_cpu_locked(struct fenced_buffer *fenced_buf)
{
   const uint8_t *map;

   assert(fenced_buf->data);
   assert(fenced_buf->buffer);

   map = pb_map(fenced_buf->buffer, PB_USAGE_CPU_READ, NULL);
   if(!map)
      return PIPE_ERROR;

   memcpy(fenced_buf->data, map, fenced_buf->size);

   pb_unmap(fenced_buf->buffer);

   return PIPE_OK;
}


static void
fenced_buffer_destroy(struct pb_buffer *buf)
{
   struct fenced_buffer *fenced_buf = fenced_buffer(buf);
   struct fenced_manager *fenced_mgr = fenced_buf->mgr;

   assert(!pipe_is_referenced(&fenced_buf->base.reference));

   pipe_mutex_lock(fenced_mgr->mutex);

   fenced_buffer_destroy_locked(fenced_mgr, fenced_buf);

   pipe_mutex_unlock(fenced_mgr->mutex);
}


static void *
fenced_buffer_map(struct pb_buffer *buf,
                  unsigned flags, void *flush_ctx)
{
   struct fenced_buffer *fenced_buf = fenced_buffer(buf);
   struct fenced_manager *fenced_mgr = fenced_buf->mgr;
   struct pb_fence_ops *ops = fenced_mgr->ops;
   void *map = NULL;

   pipe_mutex_lock(fenced_mgr->mutex);

   assert(!(flags & PB_USAGE_GPU_READ_WRITE));

   /*
    * Serialize writes.
    */
   while((fenced_buf->flags & PB_USAGE_GPU_WRITE) ||
         ((fenced_buf->flags & PB_USAGE_GPU_READ) &&
          (flags & PB_USAGE_CPU_WRITE))) {

      /* 
       * Don't wait for the GPU to finish accessing it, if blocking is forbidden.
       */
      if((flags & PB_USAGE_DONTBLOCK) &&
          ops->fence_signalled(ops, fenced_buf->fence, 0) != 0) {
         goto done;
      }

      if (flags & PB_USAGE_UNSYNCHRONIZED) {
         break;
      }

      /*
       * Wait for the GPU to finish accessing. This will release and re-acquire
       * the mutex, so all copies of mutable state must be discarded.
       */
      fenced_buffer_finish_locked(fenced_mgr, fenced_buf);
   }

   if(fenced_buf->buffer) {
      map = pb_map(fenced_buf->buffer, flags, flush_ctx);
   }
   else {
      assert(fenced_buf->data);
      map = fenced_buf->data;
   }

   if(map) {
      ++fenced_buf->mapcount;
      fenced_buf->flags |= flags & PB_USAGE_CPU_READ_WRITE;
   }

done:
   pipe_mutex_unlock(fenced_mgr->mutex);

   return map;
}


static void
fenced_buffer_unmap(struct pb_buffer *buf)
{
   struct fenced_buffer *fenced_buf = fenced_buffer(buf);
   struct fenced_manager *fenced_mgr = fenced_buf->mgr;

   pipe_mutex_lock(fenced_mgr->mutex);

   assert(fenced_buf->mapcount);
   if(fenced_buf->mapcount) {
      if (fenced_buf->buffer)
         pb_unmap(fenced_buf->buffer);
      --fenced_buf->mapcount;
      if(!fenced_buf->mapcount)
	 fenced_buf->flags &= ~PB_USAGE_CPU_READ_WRITE;
   }

   pipe_mutex_unlock(fenced_mgr->mutex);
}


static enum pipe_error
fenced_buffer_validate(struct pb_buffer *buf,
                       struct pb_validate *vl,
                       unsigned flags)
{
   struct fenced_buffer *fenced_buf = fenced_buffer(buf);
   struct fenced_manager *fenced_mgr = fenced_buf->mgr;
   enum pipe_error ret;

   pipe_mutex_lock(fenced_mgr->mutex);

   if(!vl) {
      /* invalidate */
      fenced_buf->vl = NULL;
      fenced_buf->validation_flags = 0;
      ret = PIPE_OK;
      goto done;
   }

   assert(flags & PB_USAGE_GPU_READ_WRITE);
   assert(!(flags & ~PB_USAGE_GPU_READ_WRITE));
   flags &= PB_USAGE_GPU_READ_WRITE;

   /* Buffer cannot be validated in two different lists */
   if(fenced_buf->vl && fenced_buf->vl != vl) {
      ret = PIPE_ERROR_RETRY;
      goto done;
   }

   if(fenced_buf->vl == vl &&
      (fenced_buf->validation_flags & flags) == flags) {
      /* Nothing to do -- buffer already validated */
      ret = PIPE_OK;
      goto done;
   }

   /*
    * Create and update GPU storage.
    */
   if(!fenced_buf->buffer) {
      assert(!fenced_buf->mapcount);

      ret = fenced_buffer_create_gpu_storage_locked(fenced_mgr, fenced_buf, TRUE);
      if(ret != PIPE_OK) {
         goto done;
      }

      ret = fenced_buffer_copy_storage_to_gpu_locked(fenced_buf);
      if(ret != PIPE_OK) {
         fenced_buffer_destroy_gpu_storage_locked(fenced_buf);
         goto done;
      }

      if(fenced_buf->mapcount) {
         debug_printf("warning: validating a buffer while it is still mapped\n");
      }
      else {
         fenced_buffer_destroy_cpu_storage_locked(fenced_buf);
      }
   }

   ret = pb_validate(fenced_buf->buffer, vl, flags);
   if (ret != PIPE_OK)
      goto done;

   fenced_buf->vl = vl;
   fenced_buf->validation_flags |= flags;

done:
   pipe_mutex_unlock(fenced_mgr->mutex);

   return ret;
}


static void
fenced_buffer_fence(struct pb_buffer *buf,
                    struct pipe_fence_handle *fence)
{
   struct fenced_buffer *fenced_buf = fenced_buffer(buf);
   struct fenced_manager *fenced_mgr = fenced_buf->mgr;
   struct pb_fence_ops *ops = fenced_mgr->ops;

   pipe_mutex_lock(fenced_mgr->mutex);

   assert(pipe_is_referenced(&fenced_buf->base.reference));
   assert(fenced_buf->buffer);

   if(fence != fenced_buf->fence) {
      assert(fenced_buf->vl);
      assert(fenced_buf->validation_flags);

      if (fenced_buf->fence) {
         boolean destroyed;
         destroyed = fenced_buffer_remove_locked(fenced_mgr, fenced_buf);
         assert(!destroyed);
      }
      if (fence) {
         ops->fence_reference(ops, &fenced_buf->fence, fence);
         fenced_buf->flags |= fenced_buf->validation_flags;
         fenced_buffer_add_locked(fenced_mgr, fenced_buf);
      }

      pb_fence(fenced_buf->buffer, fence);

      fenced_buf->vl = NULL;
      fenced_buf->validation_flags = 0;
   }

   pipe_mutex_unlock(fenced_mgr->mutex);
}


static void
fenced_buffer_get_base_buffer(struct pb_buffer *buf,
                              struct pb_buffer **base_buf,
                              pb_size *offset)
{
   struct fenced_buffer *fenced_buf = fenced_buffer(buf);
   struct fenced_manager *fenced_mgr = fenced_buf->mgr;

   pipe_mutex_lock(fenced_mgr->mutex);

   /*
    * This should only be called when the buffer is validated. Typically
    * when processing relocations.
    */
   assert(fenced_buf->vl);
   assert(fenced_buf->buffer);

   if(fenced_buf->buffer)
      pb_get_base_buffer(fenced_buf->buffer, base_buf, offset);
   else {
      *base_buf = buf;
      *offset = 0;
   }

   pipe_mutex_unlock(fenced_mgr->mutex);
}


static const struct pb_vtbl
fenced_buffer_vtbl = {
      fenced_buffer_destroy,
      fenced_buffer_map,
      fenced_buffer_unmap,
      fenced_buffer_validate,
      fenced_buffer_fence,
      fenced_buffer_get_base_buffer
};


/**
 * Wrap a buffer in a fenced buffer.
 */
static struct pb_buffer *
fenced_bufmgr_create_buffer(struct pb_manager *mgr,
                            pb_size size,
                            const struct pb_desc *desc)
{
   struct fenced_manager *fenced_mgr = fenced_manager(mgr);
   struct fenced_buffer *fenced_buf;
   enum pipe_error ret;

   /*
    * Don't stall the GPU, waste time evicting buffers, or waste memory
    * trying to create a buffer that will most likely never fit into the
    * graphics aperture.
    */
   if(size > fenced_mgr->max_buffer_size) {
      goto no_buffer;
   }

   fenced_buf = CALLOC_STRUCT(fenced_buffer);
   if(!fenced_buf)
      goto no_buffer;

   pipe_reference_init(&fenced_buf->base.reference, 1);
   fenced_buf->base.alignment = desc->alignment;
   fenced_buf->base.usage = desc->usage;
   fenced_buf->base.size = size;
   fenced_buf->size = size;
   fenced_buf->desc = *desc;

   fenced_buf->base.vtbl = &fenced_buffer_vtbl;
   fenced_buf->mgr = fenced_mgr;

   pipe_mutex_lock(fenced_mgr->mutex);

   /*
    * Try to create GPU storage without stalling,
    */
   ret = fenced_buffer_create_gpu_storage_locked(fenced_mgr, fenced_buf, FALSE);

   /*
    * Attempt to use CPU memory to avoid stalling the GPU.
    */
   if(ret != PIPE_OK) {
      ret = fenced_buffer_create_cpu_storage_locked(fenced_mgr, fenced_buf);
   }

   /*
    * Create GPU storage, waiting for some to be available.
    */
   if(ret != PIPE_OK) {
      ret = fenced_buffer_create_gpu_storage_locked(fenced_mgr, fenced_buf, TRUE);
   }

   /*
    * Give up.
    */
   if(ret != PIPE_OK) {
      goto no_storage;
   }

   assert(fenced_buf->buffer || fenced_buf->data);

   LIST_ADDTAIL(&fenced_buf->head, &fenced_mgr->unfenced);
   ++fenced_mgr->num_unfenced;
   pipe_mutex_unlock(fenced_mgr->mutex);

   return &fenced_buf->base;

no_storage:
   pipe_mutex_unlock(fenced_mgr->mutex);
   FREE(fenced_buf);
no_buffer:
   return NULL;
}


static void
fenced_bufmgr_flush(struct pb_manager *mgr)
{
   struct fenced_manager *fenced_mgr = fenced_manager(mgr);

   pipe_mutex_lock(fenced_mgr->mutex);
   while(fenced_manager_check_signalled_locked(fenced_mgr, TRUE))
      ;
   pipe_mutex_unlock(fenced_mgr->mutex);

   assert(fenced_mgr->provider->flush);
   if(fenced_mgr->provider->flush)
      fenced_mgr->provider->flush(fenced_mgr->provider);
}


static void
fenced_bufmgr_destroy(struct pb_manager *mgr)
{
   struct fenced_manager *fenced_mgr = fenced_manager(mgr);

   pipe_mutex_lock(fenced_mgr->mutex);

   /* Wait on outstanding fences */
   while (fenced_mgr->num_fenced) {
      pipe_mutex_unlock(fenced_mgr->mutex);
#if defined(PIPE_OS_LINUX) || defined(PIPE_OS_BSD) || defined(PIPE_OS_SOLARIS)
      sched_yield();
#endif
      pipe_mutex_lock(fenced_mgr->mutex);
      while(fenced_manager_check_signalled_locked(fenced_mgr, TRUE))
         ;
   }

#ifdef DEBUG
   /*assert(!fenced_mgr->num_unfenced);*/
#endif

   pipe_mutex_unlock(fenced_mgr->mutex);
   pipe_mutex_destroy(fenced_mgr->mutex);

   if(fenced_mgr->provider)
      fenced_mgr->provider->destroy(fenced_mgr->provider);

   fenced_mgr->ops->destroy(fenced_mgr->ops);

   FREE(fenced_mgr);
}


struct pb_manager *
fenced_bufmgr_create(struct pb_manager *provider,
                     struct pb_fence_ops *ops,
                     pb_size max_buffer_size,
                     pb_size max_cpu_total_size)
{
   struct fenced_manager *fenced_mgr;

   if(!provider)
      return NULL;

   fenced_mgr = CALLOC_STRUCT(fenced_manager);
   if (!fenced_mgr)
      return NULL;

   fenced_mgr->base.destroy = fenced_bufmgr_destroy;
   fenced_mgr->base.create_buffer = fenced_bufmgr_create_buffer;
   fenced_mgr->base.flush = fenced_bufmgr_flush;

   fenced_mgr->provider = provider;
   fenced_mgr->ops = ops;
   fenced_mgr->max_buffer_size = max_buffer_size;
   fenced_mgr->max_cpu_total_size = max_cpu_total_size;

   LIST_INITHEAD(&fenced_mgr->fenced);
   fenced_mgr->num_fenced = 0;

   LIST_INITHEAD(&fenced_mgr->unfenced);
   fenced_mgr->num_unfenced = 0;

   pipe_mutex_init(fenced_mgr->mutex);

   return &fenced_mgr->base;
}
