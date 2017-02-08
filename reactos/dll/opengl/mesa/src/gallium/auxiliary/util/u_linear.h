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


#ifndef U_LINEAR_H
#define U_LINEAR_H

#include "pipe/p_compiler.h"
#include "pipe/p_format.h"

struct u_linear_format_block
{
   /** Block size in bytes */
   unsigned size;
   
   /** Block width in pixels */
   unsigned width;
   
   /** Block height in pixels */
   unsigned height;
};


struct pipe_tile_info
{
   unsigned size;
   unsigned stride;

   /* The number of tiles */
   unsigned tiles_x;
   unsigned tiles_y;

   /* size of each tile expressed in blocks */
   unsigned cols;
   unsigned rows;

   /* Describe the tile in pixels */
   struct u_linear_format_block tile;

   /* Describe each block within the tile */
   struct u_linear_format_block block;
};

void pipe_linear_to_tile(size_t src_stride, const void *src_ptr,
			 struct pipe_tile_info *t, void  *dst_ptr);

void pipe_linear_from_tile(struct pipe_tile_info *t, const void *src_ptr,
			   size_t dst_stride, void *dst_ptr);

/**
 * Convenience function to fillout a pipe_tile_info struct.
 * @t info to fill out.
 * @block block info about pixel layout
 * @tile_width the width of the tile in pixels
 * @tile_height the height of the tile in pixels
 * @tiles_x number of tiles in x axis
 * @tiles_y number of tiles in y axis
 */
void pipe_linear_fill_info(struct pipe_tile_info *t,
			   const struct u_linear_format_block *block,
			   unsigned tile_width, unsigned tile_height,
			   unsigned tiles_x, unsigned tiles_y);

static INLINE boolean pipe_linear_check_tile(const struct pipe_tile_info *t)
{
   if (t->tile.size != t->block.size * t->cols * t->rows)
      return FALSE;

   if (t->stride != t->block.size * t->cols * t->tiles_x)
      return FALSE;

   if (t->size < t->stride * t->rows * t->tiles_y)
      return FALSE;

   return TRUE;
}

#endif /* U_LINEAR_H */
