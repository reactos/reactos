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


#include "util/u_math.h"
#include "util/u_memory.h"
#include "draw/draw_context.h"
#include "draw/draw_private.h"
#include "draw/draw_vbuf.h"
#include "draw/draw_vertex.h"
#include "draw/draw_pt.h"
#include "draw/draw_vs.h"


struct fetch_shade_emit;


/* Prototype fetch, shade, emit-hw-verts all in one go.
 */
struct fetch_shade_emit {
   struct draw_pt_middle_end base;
   struct draw_context *draw;

   /* Temporaries:
    */
   const float *constants;
   unsigned pitch[PIPE_MAX_ATTRIBS];
   const ubyte *src[PIPE_MAX_ATTRIBS];
   unsigned prim;

   struct draw_vs_variant_key key;
   struct draw_vs_variant *active;

   const struct vertex_info *vinfo;
};



static void
fse_prepare(struct draw_pt_middle_end *middle,
            unsigned prim,
            unsigned opt,
            unsigned *max_vertices)
{
   struct fetch_shade_emit *fse = (struct fetch_shade_emit *)middle;
   struct draw_context *draw = fse->draw;
   unsigned num_vs_inputs = draw->vs.vertex_shader->info.num_inputs;
   const struct vertex_info *vinfo;
   unsigned i;
   unsigned nr_vbs = 0;

   /* Can't support geometry shader on this path.
    */
   assert(!draw->gs.geometry_shader);

   draw->render->set_primitive(draw->render, prim);

   /* Must do this after set_primitive() above:
    */
   fse->vinfo = vinfo = draw->render->get_vertex_info(draw->render);

   fse->key.output_stride = vinfo->size * 4;
   fse->key.nr_outputs = vinfo->num_attribs;
   fse->key.nr_inputs = num_vs_inputs;

   fse->key.nr_elements = MAX2(fse->key.nr_outputs,     /* outputs - translate to hw format */
                               fse->key.nr_inputs);     /* inputs - fetch from api format */

   fse->key.viewport = !draw->identity_viewport;
   fse->key.clip = draw->clip_xy || draw->clip_z || draw->clip_user;
   fse->key.const_vbuffers = 0;

   memset(fse->key.element, 0,
          fse->key.nr_elements * sizeof(fse->key.element[0]));

   for (i = 0; i < num_vs_inputs; i++) {
      const struct pipe_vertex_element *src = &draw->pt.vertex_element[i];
      fse->key.element[i].in.format = src->src_format;

      /* Consider ignoring these, ie make generated programs
       * independent of this state:
       */
      fse->key.element[i].in.buffer = src->vertex_buffer_index;
      fse->key.element[i].in.offset = src->src_offset;
      nr_vbs = MAX2(nr_vbs, src->vertex_buffer_index + 1);
   }

   for (i = 0; i < 5 && i < nr_vbs; i++) {
      if (draw->pt.vertex_buffer[i].stride == 0)
         fse->key.const_vbuffers |= (1<<i);
   }

   if (0) debug_printf("%s: lookup const_vbuffers: %x\n", __FUNCTION__, fse->key.const_vbuffers);

   {
      unsigned dst_offset = 0;

      for (i = 0; i < vinfo->num_attribs; i++) {
         unsigned emit_sz = draw_translate_vinfo_size(vinfo->attrib[i].emit);

         /* doesn't handle EMIT_OMIT */
         assert(emit_sz != 0);

         /* The elements in the key correspond to vertex shader output
          * numbers, not to positions in the hw vertex description --
          * that's handled by the output_offset field.
          */
         fse->key.element[i].out.format = vinfo->attrib[i].emit;
         fse->key.element[i].out.vs_output = vinfo->attrib[i].src_index;
         fse->key.element[i].out.offset = dst_offset;

         dst_offset += emit_sz;
         assert(fse->key.output_stride >= dst_offset);
      }
   }

   fse->active = draw_vs_lookup_variant( draw->vs.vertex_shader,
                                         &fse->key );

   if (!fse->active) {
      assert(0);
      return ;
   }

   if (0) debug_printf("%s: found const_vbuffers: %x\n", __FUNCTION__,
                       fse->active->key.const_vbuffers);

   /* Now set buffer pointers:
    */
   for (i = 0; i < draw->pt.nr_vertex_buffers; i++) {
      fse->active->set_buffer( fse->active,
                               i,
                               ((const ubyte *) draw->pt.user.vbuffer[i] +
                                draw->pt.vertex_buffer[i].buffer_offset),
                              draw->pt.vertex_buffer[i].stride,
                              draw->pt.max_index );
   }

   *max_vertices = (draw->render->max_vertex_buffer_bytes /
                    (vinfo->size * 4));

   /* Probably need to do this somewhere (or fix exec shader not to
    * need it):
    */
   if (1) {
      struct draw_vertex_shader *vs = draw->vs.vertex_shader;
      vs->prepare(vs, draw);
   }
}



static void
fse_run_linear(struct draw_pt_middle_end *middle,
               unsigned start,
               unsigned count,
               unsigned prim_flags)
{
   struct fetch_shade_emit *fse = (struct fetch_shade_emit *)middle;
   struct draw_context *draw = fse->draw;
   char *hw_verts;

   /* XXX: need to flush to get prim_vbuf.c to release its allocation??
    */
   draw_do_flush( draw, DRAW_FLUSH_BACKEND );

   if (!draw->render->allocate_vertices( draw->render,
                                         (ushort)fse->key.output_stride,
                                         (ushort)count ))
      goto fail;

   hw_verts = draw->render->map_vertices( draw->render );
   if (!hw_verts)
      goto fail;

   /* Single routine to fetch vertices, run shader and emit HW verts.
    * Clipping is done elsewhere -- either by the API or on hardware,
    * or for some other reason not required...
    */
   fse->active->run_linear( fse->active,
                            start, count,
                            hw_verts );

   if (0) {
      unsigned i;
      for (i = 0; i < count; i++) {
         debug_printf("\n\n%s vertex %d: (stride %d, offset %d)\n", __FUNCTION__, i,
                      fse->key.output_stride,
                      fse->key.output_stride * i);

         draw_dump_emitted_vertex( fse->vinfo,
                                   (const uint8_t *)hw_verts + fse->key.output_stride * i );
      }
   }

   draw->render->unmap_vertices( draw->render, 0, (ushort)(count - 1) );

   /* Draw arrays path to avoid re-emitting index list again and
    * again.
    */
   draw->render->draw_arrays( draw->render,
                              0,
                              count );

   draw->render->release_vertices( draw->render );

   return;

fail:
   debug_warn_once("allocate or map of vertex buffer failed (out of memory?)");
   return;
}


static void
fse_run(struct draw_pt_middle_end *middle,
        const unsigned *fetch_elts,
        unsigned fetch_count,
        const ushort *draw_elts,
        unsigned draw_count,
        unsigned prim_flags )
{
   struct fetch_shade_emit *fse = (struct fetch_shade_emit *)middle;
   struct draw_context *draw = fse->draw;
   void *hw_verts;

   /* XXX: need to flush to get prim_vbuf.c to release its allocation??
    */
   draw_do_flush( draw, DRAW_FLUSH_BACKEND );

   if (!draw->render->allocate_vertices( draw->render,
                                         (ushort)fse->key.output_stride,
                                         (ushort)fetch_count ))
      goto fail;

   hw_verts = draw->render->map_vertices( draw->render );
   if (!hw_verts)
      goto fail;

   /* Single routine to fetch vertices, run shader and emit HW verts.
    */
   fse->active->run_elts( fse->active,
                          fetch_elts,
                          fetch_count,
                          hw_verts );

   if (0) {
      unsigned i;
      for (i = 0; i < fetch_count; i++) {
         debug_printf("\n\n%s vertex %d:\n", __FUNCTION__, i);
         draw_dump_emitted_vertex( fse->vinfo,
                                   (const uint8_t *)hw_verts +
                                   fse->key.output_stride * i );
      }
   }

   draw->render->unmap_vertices( draw->render, 0, (ushort)(fetch_count - 1) );

   draw->render->draw_elements( draw->render,
                                draw_elts,
                                draw_count );

   draw->render->release_vertices( draw->render );
   return;

fail:
   debug_warn_once("allocate or map of vertex buffer failed (out of memory?)");
   return;
}



static boolean
fse_run_linear_elts(struct draw_pt_middle_end *middle,
                    unsigned start,
                    unsigned count,
                    const ushort *draw_elts,
                    unsigned draw_count,
                    unsigned prim_flags)
{
   struct fetch_shade_emit *fse = (struct fetch_shade_emit *)middle;
   struct draw_context *draw = fse->draw;
   char *hw_verts;

   /* XXX: need to flush to get prim_vbuf.c to release its allocation??
    */
   draw_do_flush( draw, DRAW_FLUSH_BACKEND );

   if (!draw->render->allocate_vertices( draw->render,
                                         (ushort)fse->key.output_stride,
                                         (ushort)count ))
      return FALSE;

   hw_verts = draw->render->map_vertices( draw->render );
   if (!hw_verts)
      return FALSE;

   /* Single routine to fetch vertices, run shader and emit HW verts.
    * Clipping is done elsewhere -- either by the API or on hardware,
    * or for some other reason not required...
    */
   fse->active->run_linear( fse->active,
                            start, count,
                            hw_verts );

   draw->render->draw_elements( draw->render,
                                draw_elts,
                                draw_count );

   draw->render->unmap_vertices( draw->render, 0, (ushort)(count - 1) );

   draw->render->release_vertices( draw->render );

   return TRUE;
}



static void
fse_finish(struct draw_pt_middle_end *middle)
{
}


static void
fse_destroy(struct draw_pt_middle_end *middle)
{
   FREE(middle);
}


struct draw_pt_middle_end *
draw_pt_middle_fse(struct draw_context *draw)
{
   struct fetch_shade_emit *fse = CALLOC_STRUCT(fetch_shade_emit);
   if (!fse)
      return NULL;

   fse->base.prepare = fse_prepare;
   fse->base.run = fse_run;
   fse->base.run_linear = fse_run_linear;
   fse->base.run_linear_elts = fse_run_linear_elts;
   fse->base.finish = fse_finish;
   fse->base.destroy = fse_destroy;
   fse->draw = draw;

   return &fse->base;
}
