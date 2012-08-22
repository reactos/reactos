/**************************************************************************
 *
 * Copyright 2011 VMware, Inc.
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


#include "util/u_debug.h"
#include "util/u_math.h"
#include "util/u_format.h"
#include "util/u_draw.h"


/**
 * Returns the largest legal index value plus one for the current set
 * of bound vertex buffers.  Regardless of any other consideration,
 * all vertex lookups need to be clamped to 0..max_index-1 to prevent
 * an out-of-bound access.
 *
 * Note that if zero is returned it means that one or more buffers is
 * too small to contain any valid vertex data.
 */
unsigned
util_draw_max_index(
      const struct pipe_vertex_buffer *vertex_buffers,
      unsigned nr_vertex_buffers,
      const struct pipe_vertex_element *vertex_elements,
      unsigned nr_vertex_elements,
      const struct pipe_draw_info *info)
{
   unsigned max_index;
   unsigned i;

   max_index = ~0U - 1;
   for (i = 0; i < nr_vertex_elements; i++) {
      const struct pipe_vertex_element *element =
         &vertex_elements[i];
      const struct pipe_vertex_buffer *buffer =
         &vertex_buffers[element->vertex_buffer_index];
      unsigned buffer_size;
      const struct util_format_description *format_desc;
      unsigned format_size;

      assert(buffer->buffer->height0 == 1);
      assert(buffer->buffer->depth0 == 1);
      buffer_size = buffer->buffer->width0;

      format_desc = util_format_description(element->src_format);
      assert(format_desc->block.width == 1);
      assert(format_desc->block.height == 1);
      assert(format_desc->block.bits % 8 == 0);
      format_size = format_desc->block.bits/8;

      if (buffer->buffer_offset >= buffer_size) {
         /* buffer is too small */
         return 0;
      }

      buffer_size -= buffer->buffer_offset;

      if (element->src_offset >= buffer_size) {
         /* buffer is too small */
         return 0;
      }

      buffer_size -= element->src_offset;

      if (format_size > buffer_size) {
         /* buffer is too small */
         return 0;
      }

      buffer_size -= format_size;

      if (buffer->stride != 0) {
         unsigned buffer_max_index;

         buffer_max_index = buffer_size / buffer->stride;

         if (element->instance_divisor == 0) {
            /* Per-vertex data */
            max_index = MIN2(max_index, buffer_max_index);
         }
         else {
            /* Per-instance data. Simply make sure the state tracker didn't
             * request more instances than those that fit in the buffer */
            assert((info->start_instance + info->instance_count)/element->instance_divisor
                   <= (buffer_max_index + 1));
         }
      }
   }

   return max_index + 1;
}
