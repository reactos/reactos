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
/*
 * Authors: Thomas Hellström <thomas-at-tungstengraphics-dot-com>
 */

#include <xf86drm.h>
#include <stdlib.h>
#include <unistd.h>
#include "dri_bufpool.h"

/*
 * Buffer pool implementation using DRM buffer objects as DRI buffer objects.
 */

static void *
pool_create(struct _DriBufferPool *pool,
            unsigned long size, unsigned flags, unsigned hint,
            unsigned alignment)
{
   drmBO *buf = (drmBO *) malloc(sizeof(*buf));
   int ret;
   unsigned pageSize = getpagesize();

   if (!buf)
      return NULL;

   if ((alignment > pageSize) && (alignment % pageSize)) {
      free(buf);
      return NULL;
   }

   ret = drmBOCreate(pool->fd, 0, size, alignment / pageSize,
		     NULL, drm_bo_type_dc,
                     flags, hint, buf);
   if (ret) {
      free(buf);
      return NULL;
   }

   return (void *) buf;
}

static int
pool_destroy(struct _DriBufferPool *pool, void *private)
{
   int ret;
   drmBO *buf = (drmBO *) private;
   ret = drmBODestroy(pool->fd, buf);
   free(buf);
   return ret;
}

static int
pool_map(struct _DriBufferPool *pool, void *private, unsigned flags,
         int hint, void **virtual)
{
   drmBO *buf = (drmBO *) private;

   return drmBOMap(pool->fd, buf, flags, hint, virtual);
}

static int
pool_unmap(struct _DriBufferPool *pool, void *private)
{
   drmBO *buf = (drmBO *) private;
   return drmBOUnmap(pool->fd, buf);
}

static unsigned long
pool_offset(struct _DriBufferPool *pool, void *private)
{
   drmBO *buf = (drmBO *) private;
   return buf->offset;
}

static unsigned
pool_flags(struct _DriBufferPool *pool, void *private)
{
   drmBO *buf = (drmBO *) private;
   return buf->flags;
}


static unsigned long
pool_size(struct _DriBufferPool *pool, void *private)
{
   drmBO *buf = (drmBO *) private;
   return buf->size;
}

static int
pool_fence(struct _DriBufferPool *pool, void *private,
           struct _DriFenceObject *fence)
{
   /*
    * Noop. The kernel handles all fencing.
    */

   return 0;
}

static drmBO *
pool_kernel(struct _DriBufferPool *pool, void *private)
{
   return (drmBO *) private;
}

static int
pool_waitIdle(struct _DriBufferPool *pool, void *private, int lazy)
{
   drmBO *buf = (drmBO *) private;
   return drmBOWaitIdle(pool->fd, buf, (lazy) ? DRM_BO_HINT_WAIT_LAZY:0);
}

    
static void
pool_takedown(struct _DriBufferPool *pool)
{
   free(pool);
}


struct _DriBufferPool *
driDRMPoolInit(int fd)
{
   struct _DriBufferPool *pool;

   pool = (struct _DriBufferPool *) malloc(sizeof(*pool));

   if (!pool)
      return NULL;

   pool->fd = fd;
   pool->map = &pool_map;
   pool->unmap = &pool_unmap;
   pool->destroy = &pool_destroy;
   pool->offset = &pool_offset;
   pool->flags = &pool_flags;
   pool->size = &pool_size;
   pool->create = &pool_create;
   pool->fence = &pool_fence;
   pool->kernel = &pool_kernel;
   pool->validate = NULL;
   pool->setstatic = NULL;
   pool->waitIdle = &pool_waitIdle;
   pool->takeDown = &pool_takedown;
   pool->data = NULL;
   return pool;
}


static void *
pool_setstatic(struct _DriBufferPool *pool, unsigned long offset,
               unsigned long size, void *virtual, unsigned flags)
{
   drmBO *buf = (drmBO *) malloc(sizeof(*buf));
   int ret;

   if (!buf)
      return NULL;

   ret = drmBOCreate(pool->fd, offset, size, 0, NULL, drm_bo_type_fake,
                     flags, DRM_BO_HINT_DONT_FENCE, buf);

   if (ret) {
      free(buf);
      return NULL;
   }

   buf->virtual = virtual;

   return (void *) buf;
}


struct _DriBufferPool *
driDRMStaticPoolInit(int fd)
{
   struct _DriBufferPool *pool;

   pool = (struct _DriBufferPool *) malloc(sizeof(*pool));

   if (!pool)
      return NULL;

   pool->fd = fd;
   pool->map = &pool_map;
   pool->unmap = &pool_unmap;
   pool->destroy = &pool_destroy;
   pool->offset = &pool_offset;
   pool->flags = &pool_flags;
   pool->size = &pool_size;
   pool->create = NULL;
   pool->fence = &pool_fence;
   pool->kernel = &pool_kernel;
   pool->validate = NULL;
   pool->setstatic = &pool_setstatic;
   pool->waitIdle = &pool_waitIdle;
   pool->takeDown = &pool_takedown;
   pool->data = NULL;
   return pool;
}
