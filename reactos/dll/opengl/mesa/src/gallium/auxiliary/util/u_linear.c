/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
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
 * Functions for converting tiled data to linear and vice versa.
 */


#include "util/u_debug.h"
#include "u_linear.h"

void
pipe_linear_to_tile(size_t src_stride, const void *src_ptr,
		    struct pipe_tile_info *t, void *dst_ptr)
{
   int x, y, z;
   char *ptr;
   size_t bytes = t->cols * t->block.size;
   char *dst_ptr2 = (char *) dst_ptr;

   assert(pipe_linear_check_tile(t));

   /* lets write lineary to the tiled buffer */
   for (y = 0; y < t->tiles_y; y++) {
      for (x = 0; x < t->tiles_x; x++) {
	 /* this inner loop could be replace with SSE magic */
	 ptr = (char*)src_ptr + src_stride * t->rows * y + bytes * x;
	 for (z = 0; z < t->rows; z++) {
	    memcpy(dst_ptr2, ptr, bytes);
	    dst_ptr2 += bytes;
	    ptr += src_stride;
	 }
      }
   }
}

void pipe_linear_from_tile(struct pipe_tile_info *t, const void *src_ptr,
			   size_t dst_stride, void *dst_ptr)
{
   int x, y, z;
   char *ptr;
   size_t bytes = t->cols * t->block.size;
   const char *src_ptr2 = (const char *) src_ptr;

   /* lets read lineary from the tiled buffer */
   for (y = 0; y < t->tiles_y; y++) {
      for (x = 0; x < t->tiles_x; x++) {
	 /* this inner loop could be replace with SSE magic */
	 ptr = (char*)dst_ptr + dst_stride * t->rows * y + bytes * x;
	 for (z = 0; z < t->rows; z++) {
	    memcpy(ptr, src_ptr2, bytes);
	    src_ptr2 += bytes;
	    ptr += dst_stride;
	 }
      }
   }
}

void
pipe_linear_fill_info(struct pipe_tile_info *t,
		      const struct u_linear_format_block *block,
		      unsigned tile_width, unsigned tile_height,
		      unsigned tiles_x, unsigned tiles_y)
{
   t->block = *block;

   t->tile.width = tile_width;
   t->tile.height = tile_height;
   t->cols = t->tile.width / t->block.width;
   t->rows = t->tile.height / t->block.height;
   t->tile.size = t->cols * t->rows * t->block.size;

   t->tiles_x = tiles_x;
   t->tiles_y = tiles_y;
   t->stride = t->cols * t->tiles_x * t->block.size;
   t->size = t->tiles_x * t->tiles_y * t->tile.size;
}
