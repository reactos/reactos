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
 *          Keith Whitwell <keithw-at-tungstengraphics-dot-com>
 */

#include <xf86drm.h>
#include <stdlib.h>
#include "glthread.h"
#include "errno.h"
#include "dri_bufmgr.h"
#include "string.h"
#include "imports.h"
#include "dri_bufpool.h"

_glthread_DECLARE_STATIC_MUTEX(bmMutex);

/*
 * TODO: Introduce fence pools in the same way as 
 * buffer object pools.
 */



typedef struct _DriFenceObject
{
   int fd;
   _glthread_Mutex mutex;
   int refCount;
   const char *name;
   drmFence fence;
} DriFenceObject;

typedef struct _DriBufferObject
{
   DriBufferPool *pool;
   _glthread_Mutex mutex;
   int refCount;
   const char *name;
   unsigned flags;
   unsigned hint;
   unsigned alignment;
   void *private;
} DriBufferObject;


void
bmError(int val, const char *file, const char *function, int line)
{
   _mesa_printf("Fatal video memory manager error \"%s\".\n"
                "Check kernel logs or set the LIBGL_DEBUG\n"
                "environment variable to \"verbose\" for more info.\n"
                "Detected in file %s, line %d, function %s.\n",
                strerror(-val), file, line, function);
#ifndef NDEBUG
   abort();
#else
   abort();
#endif
}

DriFenceObject *
driFenceBuffers(int fd, char *name, unsigned flags)
{
   DriFenceObject *fence = (DriFenceObject *) malloc(sizeof(*fence));
   int ret;

   if (!fence)
      BM_CKFATAL(-EINVAL);

   _glthread_LOCK_MUTEX(bmMutex);
   fence->refCount = 1;
   fence->name = name;
   fence->fd = fd;
   _glthread_INIT_MUTEX(fence->mutex);
   ret = drmFenceBuffers(fd, flags, &fence->fence);
   _glthread_UNLOCK_MUTEX(bmMutex);
   if (ret) {
      free(fence);
      BM_CKFATAL(ret);
   }
   return fence;
}


unsigned 
driFenceType(DriFenceObject * fence)
{
    unsigned ret;

    _glthread_LOCK_MUTEX(bmMutex);
    ret = fence->fence.type;
    _glthread_UNLOCK_MUTEX(bmMutex);
    
    return ret;
}


DriFenceObject *
driFenceReference(DriFenceObject * fence)
{
   _glthread_LOCK_MUTEX(bmMutex);
   ++fence->refCount;
   _glthread_UNLOCK_MUTEX(bmMutex);
   return fence;
}

void
driFenceUnReference(DriFenceObject * fence)
{
   if (!fence)
      return;

   _glthread_LOCK_MUTEX(bmMutex);
   if (--fence->refCount == 0) {
      drmFenceDestroy(fence->fd, &fence->fence);
      free(fence);
   }
   _glthread_UNLOCK_MUTEX(bmMutex);
}

void
driFenceFinish(DriFenceObject * fence, unsigned type, int lazy)
{
   int ret;
   unsigned flags = (lazy) ? DRM_FENCE_FLAG_WAIT_LAZY : 0;

   _glthread_LOCK_MUTEX(fence->mutex);
   ret = drmFenceWait(fence->fd, flags, &fence->fence, type);
   _glthread_UNLOCK_MUTEX(fence->mutex);
   BM_CKFATAL(ret);
}

int
driFenceSignaled(DriFenceObject * fence, unsigned type)
{
   int signaled;
   int ret;

   if (fence == NULL)
      return GL_TRUE;

   _glthread_LOCK_MUTEX(fence->mutex);
   ret = drmFenceSignaled(fence->fd, &fence->fence, type, &signaled);
   _glthread_UNLOCK_MUTEX(fence->mutex);
   BM_CKFATAL(ret);
   return signaled;
}


extern drmBO *
driBOKernel(struct _DriBufferObject *buf)
{
   drmBO *ret;

   assert(buf->private != NULL);
   ret = buf->pool->kernel(buf->pool, buf->private);
   if (!ret)
      BM_CKFATAL(-EINVAL);

   return ret;
}

void
driBOWaitIdle(struct _DriBufferObject *buf, int lazy)
{
   struct _DriBufferPool *pool;
   void *priv;

   _glthread_LOCK_MUTEX(buf->mutex);
   pool = buf->pool;
   priv = buf->private;
   _glthread_UNLOCK_MUTEX(buf->mutex);
   
   assert(priv != NULL);
   BM_CKFATAL(buf->pool->waitIdle(pool, priv, lazy));
}

void *
driBOMap(struct _DriBufferObject *buf, unsigned flags, unsigned hint)
{
   void *virtual;

   assert(buf->private != NULL);

   _glthread_LOCK_MUTEX(buf->mutex);
   BM_CKFATAL(buf->pool->map(buf->pool, buf->private, flags, hint, &virtual));
   _glthread_UNLOCK_MUTEX(buf->mutex);
   return virtual;
}

void
driBOUnmap(struct _DriBufferObject *buf)
{
   assert(buf->private != NULL);

   buf->pool->unmap(buf->pool, buf->private);
}

unsigned long
driBOOffset(struct _DriBufferObject *buf)
{
   unsigned long ret;

   assert(buf->private != NULL);

   _glthread_LOCK_MUTEX(buf->mutex);
   ret = buf->pool->offset(buf->pool, buf->private);
   _glthread_UNLOCK_MUTEX(buf->mutex);
   return ret;
}

unsigned
driBOFlags(struct _DriBufferObject *buf)
{
   unsigned ret;

   assert(buf->private != NULL);

   _glthread_LOCK_MUTEX(buf->mutex);
   ret = buf->pool->flags(buf->pool, buf->private);
   _glthread_UNLOCK_MUTEX(buf->mutex);
   return ret;
}

struct _DriBufferObject *
driBOReference(struct _DriBufferObject *buf)
{
   _glthread_LOCK_MUTEX(bmMutex);
   if (++buf->refCount == 1) {
      BM_CKFATAL(-EINVAL);
   }
   _glthread_UNLOCK_MUTEX(bmMutex);
   return buf;
}

void
driBOUnReference(struct _DriBufferObject *buf)
{
   int tmp;

   if (!buf)
      return;

   _glthread_LOCK_MUTEX(bmMutex);
   tmp = --buf->refCount;
   _glthread_UNLOCK_MUTEX(bmMutex);
   if (!tmp) {
      buf->pool->destroy(buf->pool, buf->private);
      free(buf);
   }
}

void
driBOData(struct _DriBufferObject *buf,
          unsigned size, const void *data, unsigned flags)
{
   void *virtual;
   int newBuffer;
   struct _DriBufferPool *pool;

   _glthread_LOCK_MUTEX(buf->mutex);
   pool = buf->pool;
   if (!pool->create) {
      _mesa_error(NULL, GL_INVALID_OPERATION,
                  "driBOData called on invalid buffer\n");
      BM_CKFATAL(-EINVAL);
   }
   newBuffer = !buf->private || (pool->size(pool, buf->private) < size) ||
      pool->map(pool, buf->private, DRM_BO_FLAG_WRITE,
                DRM_BO_HINT_DONT_BLOCK, &virtual);

   if (newBuffer) {
      if (buf->private)
         pool->destroy(pool, buf->private);
      if (!flags)
         flags = buf->flags;
      buf->private = pool->create(pool, size, flags, DRM_BO_HINT_DONT_FENCE, 
				  buf->alignment);
      if (!buf->private)
         BM_CKFATAL(-ENOMEM);
      BM_CKFATAL(pool->map(pool, buf->private,
                           DRM_BO_FLAG_WRITE,
                           DRM_BO_HINT_DONT_BLOCK, &virtual));
   }

   if (data != NULL)
      memcpy(virtual, data, size);

   BM_CKFATAL(pool->unmap(pool, buf->private));
   _glthread_UNLOCK_MUTEX(buf->mutex);
}

void
driBOSubData(struct _DriBufferObject *buf,
             unsigned long offset, unsigned long size, const void *data)
{
   void *virtual;

   _glthread_LOCK_MUTEX(buf->mutex);
   if (size && data) {
      BM_CKFATAL(buf->pool->map(buf->pool, buf->private,
                                DRM_BO_FLAG_WRITE, 0, &virtual));
      memcpy((unsigned char *) virtual + offset, data, size);
      BM_CKFATAL(buf->pool->unmap(buf->pool, buf->private));
   }
   _glthread_UNLOCK_MUTEX(buf->mutex);
}

void
driBOGetSubData(struct _DriBufferObject *buf,
                unsigned long offset, unsigned long size, void *data)
{
   void *virtual;

   _glthread_LOCK_MUTEX(buf->mutex);
   if (size && data) {
      BM_CKFATAL(buf->pool->map(buf->pool, buf->private,
                                DRM_BO_FLAG_READ, 0, &virtual));
      memcpy(data, (unsigned char *) virtual + offset, size);
      BM_CKFATAL(buf->pool->unmap(buf->pool, buf->private));
   }
   _glthread_UNLOCK_MUTEX(buf->mutex);
}

void
driBOSetStatic(struct _DriBufferObject *buf,
               unsigned long offset,
               unsigned long size, void *virtual, unsigned flags)
{
   _glthread_LOCK_MUTEX(buf->mutex);
   if (buf->private != NULL) {
      _mesa_error(NULL, GL_INVALID_OPERATION,
                  "Invalid buffer for setStatic\n");
      BM_CKFATAL(-EINVAL);
   }
   if (buf->pool->setstatic == NULL) {
      _mesa_error(NULL, GL_INVALID_OPERATION,
                  "Invalid buffer pool for setStatic\n");
      BM_CKFATAL(-EINVAL);
   }

   if (!flags)
      flags = buf->flags;

   buf->private = buf->pool->setstatic(buf->pool, offset, size,
                                       virtual, flags);
   if (!buf->private) {
      _mesa_error(NULL, GL_OUT_OF_MEMORY,
                  "Invalid buffer pool for setStatic\n");
      BM_CKFATAL(-ENOMEM);
   }
   _glthread_UNLOCK_MUTEX(buf->mutex);
}



void
driGenBuffers(struct _DriBufferPool *pool,
              const char *name,
              unsigned n,
              struct _DriBufferObject *buffers[],
              unsigned alignment, unsigned flags, unsigned hint)
{
   struct _DriBufferObject *buf;
   int i;

   flags = (flags) ? flags : DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_MEM_VRAM |
      DRM_BO_FLAG_MEM_LOCAL | DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE;


   for (i = 0; i < n; ++i) {
      buf = (struct _DriBufferObject *) calloc(1, sizeof(*buf));
      if (!buf)
         BM_CKFATAL(-ENOMEM);

      _glthread_INIT_MUTEX(buf->mutex);
      _glthread_LOCK_MUTEX(buf->mutex);
      _glthread_LOCK_MUTEX(bmMutex);
      buf->refCount = 1;
      _glthread_UNLOCK_MUTEX(bmMutex);
      buf->flags = flags;
      buf->hint = hint;
      buf->name = name;
      buf->alignment = alignment;
      buf->pool = pool;
      _glthread_UNLOCK_MUTEX(buf->mutex);
      buffers[i] = buf;
   }
}

void
driDeleteBuffers(unsigned n, struct _DriBufferObject *buffers[])
{
   int i;

   for (i = 0; i < n; ++i) {
      driBOUnReference(buffers[i]);
   }
}


void
driInitBufMgr(int fd)
{
   ;
}


void
driBOCreateList(int target, drmBOList * list)
{
   _glthread_LOCK_MUTEX(bmMutex);
   BM_CKFATAL(drmBOCreateList(target, list));
   _glthread_UNLOCK_MUTEX(bmMutex);
}

void
driBOResetList(drmBOList * list)
{
   _glthread_LOCK_MUTEX(bmMutex);
   BM_CKFATAL(drmBOResetList(list));
   _glthread_UNLOCK_MUTEX(bmMutex);
}

void
driBOAddListItem(drmBOList * list, struct _DriBufferObject *buf,
                 unsigned flags, unsigned mask)
{
   int newItem;

   _glthread_LOCK_MUTEX(buf->mutex);
   _glthread_LOCK_MUTEX(bmMutex);
   BM_CKFATAL(drmAddValidateItem(list, driBOKernel(buf),
                                 flags, mask, &newItem));
   _glthread_UNLOCK_MUTEX(bmMutex);

   /*
    * Tell userspace pools to validate the buffer. This should be a 
    * noop if the pool is already validated.
    * FIXME: We should have a list for this as well.
    */

   if (buf->pool->validate) {
      BM_CKFATAL(buf->pool->validate(buf->pool, buf->private));
   }

   _glthread_UNLOCK_MUTEX(buf->mutex);
}

void
driBOFence(struct _DriBufferObject *buf, struct _DriFenceObject *fence)
{
   _glthread_LOCK_MUTEX(buf->mutex);
   BM_CKFATAL(buf->pool->fence(buf->pool, buf->private, fence));
   _glthread_UNLOCK_MUTEX(buf->mutex);

}

void
driBOValidateList(int fd, drmBOList * list)
{
   _glthread_LOCK_MUTEX(bmMutex);
   BM_CKFATAL(drmBOValidateList(fd, list));
   _glthread_UNLOCK_MUTEX(bmMutex);
}

void
driPoolTakeDown(struct _DriBufferPool *pool)
{
   pool->takeDown(pool);

}
