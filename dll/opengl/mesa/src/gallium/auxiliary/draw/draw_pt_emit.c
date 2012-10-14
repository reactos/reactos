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
#include "draw/draw_context.h"
#include "draw/draw_private.h"
#include "draw/draw_vbuf.h"
#include "draw/draw_vertex.h"
#include "draw/draw_pt.h"
#include "translate/translate.h"
#include "translate/translate_cache.h"

struct pt_emit {
   struct draw_context *draw;

   struct translate *translate;

   struct translate_cache *cache;
   unsigned prim;

   const struct vertex_info *vinfo;
};


void
draw_pt_emit_prepare(struct pt_emit *emit,
                     unsigned prim,
                     unsigned *max_vertices)
{
   struct draw_context *draw = emit->draw;
   const struct vertex_info *vinfo;
   unsigned dst_offset;
   struct translate_key hw_key;
   unsigned i;

   /* XXX: need to flush to get prim_vbuf.c to release its allocation??
    */
   draw_do_flush( draw, DRAW_FLUSH_BACKEND );

   /* XXX: may need to defensively reset this later on as clipping can
    * clobber this state in the render backend.
    */
   emit->prim = prim;

   draw->render->set_primitive(draw->render, emit->prim);

   /* Must do this after set_primitive() above:
    */
   emit->vinfo = vinfo = draw->render->get_vertex_info(draw->render);

   /* Translate from pipeline vertices to hw vertices.
    */
   dst_offset = 0;
   for (i = 0; i < vinfo->num_attribs; i++) {
      unsigned emit_sz = 0;
      unsigned src_buffer = 0;
      unsigned output_format;
      unsigned src_offset = (vinfo->attrib[i].src_index * 4 * sizeof(float) );

      output_format = draw_translate_vinfo_format(vinfo->attrib[i].emit);
      emit_sz = draw_translate_vinfo_size(vinfo->attrib[i].emit);

      /* doesn't handle EMIT_OMIT */
      assert(emit_sz != 0);

      if (vinfo->attrib[i].emit == EMIT_1F_PSIZE) {
	 src_buffer = 1;
	 src_offset = 0;
      }

      hw_key.element[i].type = TRANSLATE_ELEMENT_NORMAL;
      hw_key.element[i].input_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
      hw_key.element[i].input_buffer = src_buffer;
      hw_key.element[i].input_offset = src_offset;
      hw_key.element[i].instance_divisor = 0;
      hw_key.element[i].output_format = output_format;
      hw_key.element[i].output_offset = dst_offset;

      dst_offset += emit_sz;
   }

   hw_key.nr_elements = vinfo->num_attribs;
   hw_key.output_stride = vinfo->size * 4;

   if (!emit->translate ||
       translate_key_compare(&emit->translate->key, &hw_key) != 0) {
      translate_key_sanitize(&hw_key);
      emit->translate = translate_cache_find(emit->cache, &hw_key);
   }

   *max_vertices = (draw->render->max_vertex_buffer_bytes /
                    (vinfo->size * 4));
}


void
draw_pt_emit(struct pt_emit *emit,
             const struct draw_vertex_info *vert_info,
             const struct draw_prim_info *prim_info)
{
   const float (*vertex_data)[4] = (const float (*)[4])vert_info->verts->data;
   unsigned vertex_count = vert_info->count;
   unsigned stride = vert_info->stride;
   const ushort *elts = prim_info->elts;
   struct draw_context *draw = emit->draw;
   struct translate *translate = emit->translate;
   struct vbuf_render *render = draw->render;
   unsigned start, i;
   void *hw_verts;

   /* XXX: need to flush to get prim_vbuf.c to release its allocation??
    */
   draw_do_flush( draw, DRAW_FLUSH_BACKEND );

   if (vertex_count == 0)
      return;

   /* XXX: and work out some way to coordinate the render primitive
    * between vbuf.c and here...
    */
   draw->render->set_primitive(draw->render, emit->prim);

   render->allocate_vertices(render,
                             (ushort)translate->key.output_stride,
                             (ushort)vertex_count);

   hw_verts = render->map_vertices( render );
   if (!hw_verts) {
      debug_warn_once("map of vertex buffer failed (out of memory?)");
      return;
   }

   translate->set_buffer(translate,
			 0,
			 vertex_data,
			 stride,
			 ~0);

   translate->set_buffer(translate,
			 1,
			 &draw->rasterizer->point_size,
			 0,
			 ~0);

   /* fetch/translate vertex attribs to fill hw_verts[] */
   translate->run(translate,
		  0,
		  vertex_count,
                  draw->instance_id,
		  hw_verts );

   render->unmap_vertices(render, 0, vertex_count - 1);

   for (start = i = 0;
        i < prim_info->primitive_count;
        start += prim_info->primitive_lengths[i], i++)
   {
      render->draw_elements(render,
                            elts + start,
                            prim_info->primitive_lengths[i]);
   }

   render->release_vertices(render);
}


void
draw_pt_emit_linear(struct pt_emit *emit,
                    const struct draw_vertex_info *vert_info,
                    const struct draw_prim_info *prim_info)
{
   const float (*vertex_data)[4] = (const float (*)[4])vert_info->verts->data;
   unsigned stride = vert_info->stride;
   unsigned count = vert_info->count;
   struct draw_context *draw = emit->draw;
   struct translate *translate = emit->translate;
   struct vbuf_render *render = draw->render;
   void *hw_verts;
   unsigned start, i;

#if 0
   debug_printf("Linear emit\n");
#endif
   /* XXX: need to flush to get prim_vbuf.c to release its allocation??
    */
   draw_do_flush( draw, DRAW_FLUSH_BACKEND );

   /* XXX: and work out some way to coordinate the render primitive
    * between vbuf.c and here...
    */
   draw->render->set_primitive(draw->render, emit->prim);

   if (!render->allocate_vertices(render,
                                  (ushort)translate->key.output_stride,
                                  (ushort)count))
      goto fail;

   hw_verts = render->map_vertices( render );
   if (!hw_verts)
      goto fail;

   translate->set_buffer(translate, 0,
			 vertex_data, stride, count - 1);

   translate->set_buffer(translate, 1,
			 &draw->rasterizer->point_size,
			 0, ~0);

   translate->run(translate,
                  0,
                  count,
                  draw->instance_id,
                  hw_verts);

   if (0) {
      unsigned i;
      for (i = 0; i < count; i++) {
         debug_printf("\n\n%s vertex %d:\n", __FUNCTION__, i);
         draw_dump_emitted_vertex( emit->vinfo,
                                   (const uint8_t *)hw_verts +
                                   translate->key.output_stride * i );
      }
   }

   render->unmap_vertices( render, 0, count - 1 );

   for (start = i = 0;
        i < prim_info->primitive_count;
        start += prim_info->primitive_lengths[i], i++)
   {
      render->draw_arrays(render,
                          start,
                          prim_info->primitive_lengths[i]);
   }

   render->release_vertices(render);

   return;

fail:
   debug_warn_once("allocate or map of vertex buffer failed (out of memory?)");
   return;
}


struct pt_emit *
draw_pt_emit_create(struct draw_context *draw)
{
   struct pt_emit *emit = CALLOC_STRUCT(pt_emit);
   if (!emit)
      return NULL;

   emit->draw = draw;
   emit->cache = translate_cache_create();
   if (!emit->cache) {
      FREE(emit);
      return NULL;
   }

   return emit;
}


void
draw_pt_emit_destroy(struct pt_emit *emit)
{
   if (emit->cache)
      translate_cache_destroy(emit->cache);

   FREE(emit);
}
