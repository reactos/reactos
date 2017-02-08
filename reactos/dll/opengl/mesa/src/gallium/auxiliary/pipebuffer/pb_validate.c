/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * @file
 * Buffer validation.
 * 
 * @author Jose Fonseca <jrfonseca@tungstengraphics.com>
 */


#include "pipe/p_compiler.h"
#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "util/u_debug.h"

#include "pb_buffer.h"
#include "pb_validate.h"


#define PB_VALIDATE_INITIAL_SIZE 1 /* 512 */ 


struct pb_validate_entry
{
   struct pb_buffer *buf;
   unsigned flags;
};


struct pb_validate
{
   struct pb_validate_entry *entries;
   unsigned used;
   unsigned size;
};


enum pipe_error
pb_validate_add_buffer(struct pb_validate *vl,
                       struct pb_buffer *buf,
                       unsigned flags)
{
   assert(buf);
   if(!buf)
      return PIPE_ERROR;

   assert(flags & PB_USAGE_GPU_READ_WRITE);
   assert(!(flags & ~PB_USAGE_GPU_READ_WRITE));
   flags &= PB_USAGE_GPU_READ_WRITE;

   /* We only need to store one reference for each buffer, so avoid storing
    * consecutive references for the same buffer. It might not be the most 
    * common pattern, but it is easy to implement.
    */
   if(vl->used && vl->entries[vl->used - 1].buf == buf) {
      vl->entries[vl->used - 1].flags |= flags;
      return PIPE_OK;
   }
   
   /* Grow the table */
   if(vl->used == vl->size) {
      unsigned new_size;
      struct pb_validate_entry *new_entries;
      
      new_size = vl->size * 2;
      if(!new_size)
	 return PIPE_ERROR_OUT_OF_MEMORY;

      new_entries = (struct pb_validate_entry *)REALLOC(vl->entries,
                                                        vl->size*sizeof(struct pb_validate_entry),
                                                        new_size*sizeof(struct pb_validate_entry));
      if(!new_entries)
         return PIPE_ERROR_OUT_OF_MEMORY;
      
      memset(new_entries + vl->size, 0, (new_size - vl->size)*sizeof(struct pb_validate_entry));
      
      vl->size = new_size;
      vl->entries = new_entries;
   }
   
   assert(!vl->entries[vl->used].buf);
   pb_reference(&vl->entries[vl->used].buf, buf);
   vl->entries[vl->used].flags = flags;
   ++vl->used;
   
   return PIPE_OK;
}


enum pipe_error
pb_validate_foreach(struct pb_validate *vl,
                    enum pipe_error (*callback)(struct pb_buffer *buf, void *data),
                    void *data)
{
   unsigned i;
   for(i = 0; i < vl->used; ++i) {
      enum pipe_error ret;
      ret = callback(vl->entries[i].buf, data);
      if(ret != PIPE_OK)
         return ret;
   }
   return PIPE_OK;
}


enum pipe_error
pb_validate_validate(struct pb_validate *vl) 
{
   unsigned i;
   
   for(i = 0; i < vl->used; ++i) {
      enum pipe_error ret;
      ret = pb_validate(vl->entries[i].buf, vl, vl->entries[i].flags);
      if(ret != PIPE_OK) {
         while(i--)
            pb_validate(vl->entries[i].buf, NULL, 0);
         return ret;
      }
   }

   return PIPE_OK;
}


void
pb_validate_fence(struct pb_validate *vl,
                  struct pipe_fence_handle *fence)
{
   unsigned i;
   for(i = 0; i < vl->used; ++i) {
      pb_fence(vl->entries[i].buf, fence);
      pb_reference(&vl->entries[i].buf, NULL);
   }
   vl->used = 0;
}


void
pb_validate_destroy(struct pb_validate *vl)
{
   unsigned i;
   for(i = 0; i < vl->used; ++i)
      pb_reference(&vl->entries[i].buf, NULL);
   FREE(vl->entries);
   FREE(vl);
}


struct pb_validate *
pb_validate_create()
{
   struct pb_validate *vl;
   
   vl = CALLOC_STRUCT(pb_validate);
   if(!vl)
      return NULL;
   
   vl->size = PB_VALIDATE_INITIAL_SIZE;
   vl->entries = (struct pb_validate_entry *)CALLOC(vl->size, sizeof(struct pb_validate_entry));
   if(!vl->entries) {
      FREE(vl);
      return NULL;
   }

   return vl;
}

