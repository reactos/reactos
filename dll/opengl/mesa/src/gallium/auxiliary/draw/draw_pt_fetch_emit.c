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

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */

#include "util/u_memory.h"
#include "draw/draw_context.h"
#include "draw/draw_private.h"
#include "draw/draw_vbuf.h"
#include "draw/draw_vertex.h"
#include "draw/draw_pt.h"
#include "draw/draw_gs.h"
#include "translate/translate.h"
#include "translate/translate_cache.h"

/* The simplest 'middle end' in the new vertex code.
 *
 * The responsibilities of a middle end are to:
 *  - perform vertex fetch using
 *       - draw vertex element/buffer state
 *       - a list of fetch indices we received as an input
 *  - run the vertex shader
 *  - cliptest,
 *  - clip coord calculation
 *  - viewport transformation
 *  - if necessary, run the primitive pipeline, passing it:
 *       - a linear array of vertex_header vertices constructed here
 *       - a set of draw indices we received as an input
 *  - otherwise, drive the hw backend,
 *       - allocate space for hardware format vertices
 *       - translate the vertex-shader output vertices to hw format
 *       - calling the backend draw functions.
 *
 * For convenience, we provide a helper function to drive the hardware
 * backend given similar inputs to those required to run the pipeline.
 *
 * In the case of passthrough mode, many of these actions are disabled
 * or noops, so we end up doing:
 *
 *  - perform vertex fetch
 *  - drive the hw backend
 *
 * IE, basically just vertex fetch to post-vs-format vertices,
 * followed by a call to the backend helper function.
 */


struct fetch_emit_middle_end {
   struct draw_pt_middle_end base;
   struct draw_context *draw;

   struct translate *translate;
   const struct vertex_info *vinfo;

   /* Cache point size somewhere it's address won't change:
    */
   float point_size;

   struct translate_cache *cache;
};


static void fetch_emit_prepare( struct draw_pt_middle_end *middle,
                                unsigned prim,
				unsigned opt,
                                unsigned *max_vertices )
{
   struct fetch_emit_middle_end *feme = (struct fetch_emit_middle_end *)middle;
   struct draw_context *draw = feme->draw;
   const struct vertex_info *vinfo;
   unsigned i, dst_offset;
   struct translate_key key;
   unsigned gs_out_prim = (draw->gs.geometry_shader ?
                           draw->gs.geometry_shader->output_primitive :
                           prim);

   draw->render->set_primitive(draw->render, gs_out_prim);

   /* Must do this after set_primitive() above:
    */
   vinfo = feme->vinfo = draw->render->get_vertex_info(draw->render);

   /* Transform from API vertices to HW vertices, skipping the
    * pipeline_vertex intermediate step.
    */
   dst_offset = 0;
   memset(&key, 0, sizeof(key));

   for (i = 0; i < vinfo->num_attribs; i++) {
      const struct pipe_vertex_element *src = &draw->pt.vertex_element[vinfo->attrib[i].src_index];

      unsigned emit_sz = 0;
      unsigned input_format = src->src_format;
      unsigned input_buffer = src->vertex_buffer_index;
      unsigned input_offset = src->src_offset;
      unsigned output_format;

      output_format = draw_translate_vinfo_format(vinfo->attrib[i].emit);
      emit_sz = draw_translate_vinfo_size(vinfo->attrib[i].emit);

      if (vinfo->attrib[i].emit == EMIT_OMIT)
	 continue;

      if (vinfo->attrib[i].emit == EMIT_1F_PSIZE) {
	 input_format = PIPE_FORMAT_R32_FLOAT;
	 input_buffer = draw->pt.nr_vertex_buffers;
	 input_offset = 0;
      }

      key.element[i].type = TRANSLATE_ELEMENT_NORMAL;
      key.element[i].input_format = input_format;
      key.element[i].input_buffer = input_buffer;
      key.element[i].input_offset = input_offset;
      key.element[i].instance_divisor = src->instance_divisor;
      key.element[i].output_format = output_format;
      key.element[i].output_offset = dst_offset;

      dst_offset += emit_sz;
   }

   key.nr_elements = vinfo->num_attribs;
   key.output_stride = vinfo->size * 4;

   /* Don't bother with caching at this stage:
    */
   if (!feme->translate ||
       translate_key_compare(&feme->translate->key, &key) != 0)
   {
      translate_key_sanitize(&key);
      feme->translate = translate_cache_find(feme->cache,
                                             &key);

      feme->translate->set_buffer(feme->translate,
				  draw->pt.nr_vertex_buffers,
				  &feme->point_size,
				  0,
				  ~0);
   }

   feme->point_size = draw->rasterizer->point_size;

   for (i = 0; i < draw->pt.nr_vertex_buffers; i++) {
      feme->translate->set_buffer(feme->translate,
                                  i,
                                  ((char *)draw->pt.user.vbuffer[i] +
                                   draw->pt.vertex_buffer[i].buffer_offset),
                                  draw->pt.vertex_buffer[i].stride,
                                  draw->pt.max_index);
   }

   *max_vertices = (draw->render->max_vertex_buffer_bytes /
                    (vinfo->size * 4));
}


static void fetch_emit_run( struct draw_pt_middle_end *middle,
                            const unsigned *fetch_elts,
                            unsigned fetch_count,
                            const ushort *draw_elts,
                            unsigned draw_count,
                            unsigned prim_flags )
{
   struct fetch_emit_middle_end *feme = (struct fetch_emit_middle_end *)middle;
   struct draw_context *draw = feme->draw;
   void *hw_verts;

   /* XXX: need to flush to get prim_vbuf.c to release its allocation??
    */
   draw_do_flush( draw, DRAW_FLUSH_BACKEND );

   draw->render->allocate_vertices( draw->render,
                                    (ushort)feme->translate->key.output_stride,
                                    (ushort)fetch_count );

   hw_verts = draw->render->map_vertices( draw->render );
   if (!hw_verts) {
      debug_warn_once("vertex buffer allocation failed (out of memory?)");
      return;
   }

   /* Single routine to fetch vertices and emit HW verts.
    */
   feme->translate->run_elts( feme->translate,
			      fetch_elts,
			      fetch_count,
                              draw->instance_id,
			      hw_verts );

   if (0) {
      unsigned i;
      for (i = 0; i < fetch_count; i++) {
         debug_printf("\n\nvertex %d:\n", i);
         draw_dump_emitted_vertex( feme->vinfo,
                                   (const uint8_t *)hw_verts + feme->vinfo->size * 4 * i );
      }
   }

   draw->render->unmap_vertices( draw->render,
                                 0,
                                 (ushort)(fetch_count - 1) );

   /* XXX: Draw arrays path to avoid re-emitting index list again and
    * again.
    */
   draw->render->draw_elements( draw->render,
                                draw_elts,
                                draw_count );

   /* Done -- that was easy, wasn't it:
    */
   draw->render->release_vertices( draw->render );

}


static void fetch_emit_run_linear( struct draw_pt_middle_end *middle,
                                   unsigned start,
                                   unsigned count,
                                   unsigned prim_flags )
{
   struct fetch_emit_middle_end *feme = (struct fetch_emit_middle_end *)middle;
   struct draw_context *draw = feme->draw;
   void *hw_verts;

   /* XXX: need to flush to get prim_vbuf.c to release its allocation??
    */
   draw_do_flush( draw, DRAW_FLUSH_BACKEND );

   if (!draw->render->allocate_vertices( draw->render,
                                         (ushort)feme->translate->key.output_stride,
                                         (ushort)count ))
      goto fail;

   hw_verts = draw->render->map_vertices( draw->render );
   if (!hw_verts)
      goto fail;

   /* Single routine to fetch vertices and emit HW verts.
    */
   feme->translate->run( feme->translate,
                         start,
                         count,
                         draw->instance_id,
                         hw_verts );

   if (0) {
      unsigned i;
      for (i = 0; i < count; i++) {
         debug_printf("\n\nvertex %d:\n", i);
         draw_dump_emitted_vertex( feme->vinfo,
                                   (const uint8_t *)hw_verts + feme->vinfo->size * 4 * i );
      }
   }

   draw->render->unmap_vertices( draw->render, 0, count - 1 );

   /* XXX: Draw arrays path to avoid re-emitting index list again and
    * again.
    */
   draw->render->draw_arrays( draw->render, 0, count );

   /* Done -- that was easy, wasn't it:
    */
   draw->render->release_vertices( draw->render );
   return;

fail:
   debug_warn_once("allocate or map of vertex buffer failed (out of memory?)");
   return;
}


static boolean fetch_emit_run_linear_elts( struct draw_pt_middle_end *middle,
                                        unsigned start,
                                        unsigned count,
                                        const ushort *draw_elts,
                                        unsigned draw_count,
                                        unsigned prim_flags )
{
   struct fetch_emit_middle_end *feme = (struct fetch_emit_middle_end *)middle;
   struct draw_context *draw = feme->draw;
   void *hw_verts;

   /* XXX: need to flush to get prim_vbuf.c to release its allocation??
    */
   draw_do_flush( draw, DRAW_FLUSH_BACKEND );

   if (!draw->render->allocate_vertices( draw->render,
                                         (ushort)feme->translate->key.output_stride,
                                         (ushort)count ))
      return FALSE;

   hw_verts = draw->render->map_vertices( draw->render );
   if (!hw_verts)
      return FALSE;

   /* Single routine to fetch vertices and emit HW verts.
    */
   feme->translate->run( feme->translate,
                         start,
                         count,
                         draw->instance_id,
                         hw_verts );

   draw->render->unmap_vertices( draw->render, 0, (ushort)(count - 1) );

   /* XXX: Draw arrays path to avoid re-emitting index list again and
    * again.
    */
   draw->render->draw_elements( draw->render,
                                draw_elts,
                                draw_count );

   /* Done -- that was easy, wasn't it:
    */
   draw->render->release_vertices( draw->render );

   return TRUE;
}


static void fetch_emit_finish( struct draw_pt_middle_end *middle )
{
   /* nothing to do */
}


static void fetch_emit_destroy( struct draw_pt_middle_end *middle )
{
   struct fetch_emit_middle_end *feme = (struct fetch_emit_middle_end *)middle;

   if (feme->cache)
      translate_cache_destroy(feme->cache);

   FREE(middle);
}


struct draw_pt_middle_end *draw_pt_fetch_emit( struct draw_context *draw )
{
   struct fetch_emit_middle_end *fetch_emit = CALLOC_STRUCT( fetch_emit_middle_end );
   if (fetch_emit == NULL)
      return NULL;

   fetch_emit->cache = translate_cache_create();
   if (!fetch_emit->cache) {
      FREE(fetch_emit);
      return NULL;
   }

   fetch_emit->base.prepare    = fetch_emit_prepare;
   fetch_emit->base.run        = fetch_emit_run;
   fetch_emit->base.run_linear = fetch_emit_run_linear;
   fetch_emit->base.run_linear_elts = fetch_emit_run_linear_elts;
   fetch_emit->base.finish     = fetch_emit_finish;
   fetch_emit->base.destroy    = fetch_emit_destroy;

   fetch_emit->draw = draw;

   return &fetch_emit->base;
}

