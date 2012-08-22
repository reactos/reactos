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

#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_format.h"
#include "draw/draw_context.h"
#include "draw/draw_private.h"
#include "draw/draw_pt.h"
#include "translate/translate.h"
#include "translate/translate_cache.h"


struct pt_fetch {
   struct draw_context *draw;

   struct translate *translate;

   unsigned vertex_size;

   struct translate_cache *cache;
};


/**
 * Perform the fetch from API vertex elements & vertex buffers, to a
 * contiguous set of float[4] attributes as required for the
 * vertex_shader->run_linear() method.
 *
 * This is used in all cases except pure passthrough
 * (draw_pt_fetch_emit.c) which has its own version to translate
 * directly to hw vertices.
 *
 */
void
draw_pt_fetch_prepare(struct pt_fetch *fetch,
                      unsigned vs_input_count,
                      unsigned vertex_size,
                      unsigned instance_id_index)
{
   struct draw_context *draw = fetch->draw;
   unsigned nr_inputs;
   unsigned i, nr = 0, ei = 0;
   unsigned dst_offset = 0;
   unsigned num_extra_inputs = 0;
   struct translate_key key;

   fetch->vertex_size = vertex_size;

   /* Leave the clipmask/edgeflags/pad/vertex_id untouched
    */
   dst_offset += 1 * sizeof(float);
   /* Just leave the clip[] and pre_clip_pos[] array untouched.
    */
   dst_offset += 8 * sizeof(float);

   if (instance_id_index != ~0) {
      num_extra_inputs++;
   }

   assert(draw->pt.nr_vertex_elements + num_extra_inputs >= vs_input_count);

   nr_inputs = MIN2(vs_input_count, draw->pt.nr_vertex_elements + num_extra_inputs);

   for (i = 0; i < nr_inputs; i++) {
      if (i == instance_id_index) {
         key.element[nr].type = TRANSLATE_ELEMENT_INSTANCE_ID;
         key.element[nr].input_format = PIPE_FORMAT_R32_USCALED;
         key.element[nr].output_format = PIPE_FORMAT_R32_USCALED;
         key.element[nr].output_offset = dst_offset;

         dst_offset += sizeof(uint);
      } else if (util_format_is_pure_sint(draw->pt.vertex_element[i].src_format)) {
         key.element[nr].type = TRANSLATE_ELEMENT_NORMAL;
         key.element[nr].input_format = draw->pt.vertex_element[ei].src_format;
         key.element[nr].input_buffer = draw->pt.vertex_element[ei].vertex_buffer_index;
         key.element[nr].input_offset = draw->pt.vertex_element[ei].src_offset;
         key.element[nr].instance_divisor = draw->pt.vertex_element[ei].instance_divisor;
         key.element[nr].output_format = PIPE_FORMAT_R32G32B32A32_SINT;
         key.element[nr].output_offset = dst_offset;

         ei++;
         dst_offset += 4 * sizeof(int);
      } else if (util_format_is_pure_uint(draw->pt.vertex_element[i].src_format)) {
         key.element[nr].type = TRANSLATE_ELEMENT_NORMAL;
         key.element[nr].input_format = draw->pt.vertex_element[ei].src_format;
         key.element[nr].input_buffer = draw->pt.vertex_element[ei].vertex_buffer_index;
         key.element[nr].input_offset = draw->pt.vertex_element[ei].src_offset;
         key.element[nr].instance_divisor = draw->pt.vertex_element[ei].instance_divisor;
         key.element[nr].output_format = PIPE_FORMAT_R32G32B32A32_UINT;
         key.element[nr].output_offset = dst_offset;

         ei++;
         dst_offset += 4 * sizeof(unsigned);
      } else {
         key.element[nr].type = TRANSLATE_ELEMENT_NORMAL;
         key.element[nr].input_format = draw->pt.vertex_element[ei].src_format;
         key.element[nr].input_buffer = draw->pt.vertex_element[ei].vertex_buffer_index;
         key.element[nr].input_offset = draw->pt.vertex_element[ei].src_offset;
         key.element[nr].instance_divisor = draw->pt.vertex_element[ei].instance_divisor;
         key.element[nr].output_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
         key.element[nr].output_offset = dst_offset;

         ei++;
         dst_offset += 4 * sizeof(float);
      }

      nr++;
   }

   assert(dst_offset <= vertex_size);

   key.nr_elements = nr;
   key.output_stride = vertex_size;

   if (!fetch->translate ||
       translate_key_compare(&fetch->translate->key, &key) != 0)
   {
      translate_key_sanitize(&key);
      fetch->translate = translate_cache_find(fetch->cache, &key);
   }
}


void
draw_pt_fetch_run(struct pt_fetch *fetch,
                  const unsigned *elts,
                  unsigned count,
                  char *verts)
{
   struct draw_context *draw = fetch->draw;
   struct translate *translate = fetch->translate;
   unsigned i;

   for (i = 0; i < draw->pt.nr_vertex_buffers; i++) {
      translate->set_buffer(translate,
			    i,
			    ((char *)draw->pt.user.vbuffer[i] +
			     draw->pt.vertex_buffer[i].buffer_offset),
			    draw->pt.vertex_buffer[i].stride,
			    draw->pt.max_index);
   }

   translate->run_elts( translate,
			elts,
			count,
                        draw->instance_id,
			verts );
}


void
draw_pt_fetch_run_linear(struct pt_fetch *fetch,
                         unsigned start,
                         unsigned count,
                         char *verts)
{
   struct draw_context *draw = fetch->draw;
   struct translate *translate = fetch->translate;
   unsigned i;

   for (i = 0; i < draw->pt.nr_vertex_buffers; i++) {
      translate->set_buffer(translate,
			    i,
			    ((char *)draw->pt.user.vbuffer[i] +
			     draw->pt.vertex_buffer[i].buffer_offset),
			    draw->pt.vertex_buffer[i].stride,
			    draw->pt.user.max_index + draw->pt.user.eltBias);
   }

   translate->run( translate,
                   start,
                   count,
                   draw->instance_id,
                   verts );
}


struct pt_fetch *
draw_pt_fetch_create(struct draw_context *draw)
{
   struct pt_fetch *fetch = CALLOC_STRUCT(pt_fetch);
   if (!fetch)
      return NULL;

   fetch->draw = draw;
   fetch->cache = translate_cache_create();
   if (!fetch->cache) {
      FREE(fetch);
      return NULL;
   }

   return fetch;
}


void
draw_pt_fetch_destroy(struct pt_fetch *fetch)
{
   if (fetch->cache)
      translate_cache_destroy(fetch->cache);

   FREE(fetch);
}
