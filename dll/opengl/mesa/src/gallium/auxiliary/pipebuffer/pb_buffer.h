/**************************************************************************
 *
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * Generic code for buffers.
 * 
 * Behind a pipe buffle handle there can be DMA buffers, client (or user) 
 * buffers, regular malloced buffers, etc. This file provides an abstract base 
 * buffer handle that allows the driver to cope with all those kinds of buffers 
 * in a more flexible way.
 * 
 * There is no obligation of a winsys driver to use this library. And a pipe
 * driver should be completly agnostic about it.
 * 
 * \author Jose Fonseca <jrfonseca@tungstengraphics.com>
 */

#ifndef PB_BUFFER_H_
#define PB_BUFFER_H_


#include "pipe/p_compiler.h"
#include "util/u_debug.h"
#include "util/u_inlines.h"
#include "pipe/p_defines.h"


#ifdef __cplusplus
extern "C" {
#endif


struct pb_vtbl;
struct pb_validate;
struct pipe_fence_handle;


#define PB_USAGE_CPU_READ  (1 << 0)
#define PB_USAGE_CPU_WRITE (1 << 1)
#define PB_USAGE_GPU_READ  (1 << 2)
#define PB_USAGE_GPU_WRITE (1 << 3)
#define PB_USAGE_UNSYNCHRONIZED (1 << 10)
#define PB_USAGE_DONTBLOCK (1 << 9)

#define PB_USAGE_CPU_READ_WRITE \
   ( PB_USAGE_CPU_READ | PB_USAGE_CPU_WRITE )
#define PB_USAGE_GPU_READ_WRITE \
   ( PB_USAGE_GPU_READ | PB_USAGE_GPU_WRITE )
#define PB_USAGE_WRITE \
   ( PB_USAGE_CPU_WRITE | PB_USAGE_GPU_WRITE )

/**
 * Buffer description.
 * 
 * Used when allocating the buffer.
 */
struct pb_desc
{
   unsigned alignment;
   unsigned usage;
};


/**
 * Size. Regular (32bit) unsigned for now.
 */
typedef unsigned pb_size;


/**
 * Base class for all pb_* buffers.
 */
struct pb_buffer 
{
   struct pipe_reference  reference;
   unsigned               size;
   unsigned               alignment;
   unsigned               usage;

   /**
    * Pointer to the virtual function table.
    *
    * Avoid accessing this table directly. Use the inline functions below 
    * instead to avoid mistakes. 
    */
   const struct pb_vtbl *vtbl;
};


/**
 * Virtual function table for the buffer storage operations.
 * 
 * Note that creation is not done through this table.
 */
struct pb_vtbl
{
   void (*destroy)( struct pb_buffer *buf );

   /** 
    * Map the entire data store of a buffer object into the client's address.
    * flags is bitmask of PB_USAGE_CPU_READ/WRITE. 
    */
   void *(*map)( struct pb_buffer *buf, 
                 unsigned flags, void *flush_ctx );
   
   void (*unmap)( struct pb_buffer *buf );

   enum pipe_error (*validate)( struct pb_buffer *buf, 
                                struct pb_validate *vl,
                                unsigned flags );

   void (*fence)( struct pb_buffer *buf, 
                  struct pipe_fence_handle *fence );

   /**
    * Get the base buffer and the offset.
    * 
    * A buffer can be subdivided in smaller buffers. This method should return
    * the underlaying buffer, and the relative offset.
    * 
    * Buffers without an underlaying base buffer should return themselves, with 
    * a zero offset.
    * 
    * Note that this will increase the reference count of the base buffer.
    */
   void (*get_base_buffer)( struct pb_buffer *buf,
                            struct pb_buffer **base_buf,
                            pb_size *offset );
   
};



/* Accessor functions for pb->vtbl:
 */
static INLINE void *
pb_map(struct pb_buffer *buf, 
       unsigned flags, void *flush_ctx)
{
   assert(buf);
   if(!buf)
      return NULL;
   assert(pipe_is_referenced(&buf->reference));
   return buf->vtbl->map(buf, flags, flush_ctx);
}


static INLINE void 
pb_unmap(struct pb_buffer *buf)
{
   assert(buf);
   if(!buf)
      return;
   assert(pipe_is_referenced(&buf->reference));
   buf->vtbl->unmap(buf);
}


static INLINE void
pb_get_base_buffer( struct pb_buffer *buf,
		    struct pb_buffer **base_buf,
		    pb_size *offset )
{
   assert(buf);
   if(!buf) {
      base_buf = NULL;
      offset = 0;
      return;
   }
   assert(pipe_is_referenced(&buf->reference));
   assert(buf->vtbl->get_base_buffer);
   buf->vtbl->get_base_buffer(buf, base_buf, offset);
   assert(*base_buf);
   assert(*offset < (*base_buf)->size);
}


static INLINE enum pipe_error 
pb_validate(struct pb_buffer *buf, struct pb_validate *vl, unsigned flags)
{
   assert(buf);
   if(!buf)
      return PIPE_ERROR;
   assert(buf->vtbl->validate);
   return buf->vtbl->validate(buf, vl, flags);
}


static INLINE void 
pb_fence(struct pb_buffer *buf, struct pipe_fence_handle *fence)
{
   assert(buf);
   if(!buf)
      return;
   assert(buf->vtbl->fence);
   buf->vtbl->fence(buf, fence);
}


static INLINE void 
pb_destroy(struct pb_buffer *buf)
{
   assert(buf);
   if(!buf)
      return;
   assert(!pipe_is_referenced(&buf->reference));
   buf->vtbl->destroy(buf);
}

static INLINE void
pb_reference(struct pb_buffer **dst,
             struct pb_buffer *src)
{
   struct pb_buffer *old = *dst;

   if (pipe_reference(&(*dst)->reference, &src->reference))
      pb_destroy( old );
   *dst = src;
}


/**
 * Utility function to check whether the provided alignment is consistent with
 * the requested or not.
 */
static INLINE boolean
pb_check_alignment(pb_size requested, pb_size provided)
{
   if(!requested)
      return TRUE;
   if(requested > provided)
      return FALSE;
   if(provided % requested != 0)
      return FALSE;
   return TRUE;
}


/**
 * Utility function to check whether the provided alignment is consistent with
 * the requested or not.
 */
static INLINE boolean
pb_check_usage(unsigned requested, unsigned provided)
{
   return (requested & provided) == requested ? TRUE : FALSE;
}


/**
 * Malloc-based buffer to store data that can't be used by the graphics 
 * hardware.
 */
struct pb_buffer *
pb_malloc_buffer_create(pb_size size, 
                        const struct pb_desc *desc);


#ifdef __cplusplus
}
#endif

#endif /*PB_BUFFER_H_*/
