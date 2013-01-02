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
 * Rectangle-related helper functions.
 */


#include "util/u_format.h"
#include "util/u_rect.h"
#include "util/u_pack_color.h"


/**
 * Copy 2D rect from one place to another.
 * Position and sizes are in pixels.
 * src_stride may be negative to do vertical flip of pixels from source.
 */
void
util_copy_rect(ubyte * dst,
               enum pipe_format format,
               unsigned dst_stride,
               unsigned dst_x,
               unsigned dst_y,
               unsigned width,
               unsigned height,
               const ubyte * src,
               int src_stride,
               unsigned src_x, 
               unsigned src_y)
{
   unsigned i;
   int src_stride_pos = src_stride < 0 ? -src_stride : src_stride;
   int blocksize = util_format_get_blocksize(format);
   int blockwidth = util_format_get_blockwidth(format);
   int blockheight = util_format_get_blockheight(format);

   assert(blocksize > 0);
   assert(blockwidth > 0);
   assert(blockheight > 0);

   dst_x /= blockwidth;
   dst_y /= blockheight;
   width = (width + blockwidth - 1)/blockwidth;
   height = (height + blockheight - 1)/blockheight;
   src_x /= blockwidth;
   src_y /= blockheight;
   
   dst += dst_x * blocksize;
   src += src_x * blocksize;
   dst += dst_y * dst_stride;
   src += src_y * src_stride_pos;
   width *= blocksize;

   if (width == dst_stride && width == src_stride)
      memcpy(dst, src, height * width);
   else {
      for (i = 0; i < height; i++) {
         memcpy(dst, src, width);
         dst += dst_stride;
         src += src_stride;
      }
   }
}

void
util_fill_rect(ubyte * dst,
               enum pipe_format format,
               unsigned dst_stride,
               unsigned dst_x,
               unsigned dst_y,
               unsigned width,
               unsigned height,
               union util_color *uc)
{
   const struct util_format_description *desc = util_format_description(format);
   unsigned i, j;
   unsigned width_size;
   int blocksize = desc->block.bits / 8;
   int blockwidth = desc->block.width;
   int blockheight = desc->block.height;

   assert(blocksize > 0);
   assert(blockwidth > 0);
   assert(blockheight > 0);

   dst_x /= blockwidth;
   dst_y /= blockheight;
   width = (width + blockwidth - 1)/blockwidth;
   height = (height + blockheight - 1)/blockheight;

   dst += dst_x * blocksize;
   dst += dst_y * dst_stride;
   width_size = width * blocksize;

   switch (blocksize) {
   case 1:
      if(dst_stride == width_size)
         memset(dst, uc->ub, height * width_size);
      else {
         for (i = 0; i < height; i++) {
            memset(dst, uc->ub, width_size);
            dst += dst_stride;
         }
      }
      break;
   case 2:
      for (i = 0; i < height; i++) {
         uint16_t *row = (uint16_t *)dst;
         for (j = 0; j < width; j++)
            *row++ = uc->us;
         dst += dst_stride;
      }
      break;
   case 4:
      for (i = 0; i < height; i++) {
         uint32_t *row = (uint32_t *)dst;
         for (j = 0; j < width; j++)
            *row++ = uc->ui;
         dst += dst_stride;
      }
      break;
   case 8:
   case 12:
   case 16:
   case 24:
   case 32:
      for (i = 0; i < height; i++) {
         ubyte *row = dst;
         for (j = 0; j < width; j++) {
            memcpy(row, uc, blocksize);
            row += blocksize;
         }
         dst += dst_stride;
      }
      break;
   default:
      assert(0);
      break;
   }
}
