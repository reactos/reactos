/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
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

#include "draw/draw_private.h"
#include "draw/draw_vs.h"
#include "draw/draw_context.h"
#include "draw/draw_vbuf.h"
#include "draw/draw_vertex.h"
#include "draw/draw_pt.h"

#include "pipe/p_state.h"

#include "util/u_math.h"
#include "util/u_memory.h"

struct pt_so_emit {
   struct draw_context *draw;

   unsigned input_vertex_stride;
   const float (*inputs)[4];

   boolean has_so;

   unsigned emitted_primitives;
   unsigned emitted_vertices;
   unsigned generated_primitives;
};


void draw_pt_so_emit_prepare(struct pt_so_emit *emit)
{
   struct draw_context *draw = emit->draw;

   emit->has_so = (draw->vs.vertex_shader->state.stream_output.num_outputs > 0);

   /* if we have a state with outputs make sure we have
    * buffers to output to */
   if (emit->has_so) {
      boolean has_valid_buffer = FALSE;
      unsigned i;
      for (i = 0; i < draw->so.num_targets; ++i) {
         if (draw->so.targets[i]) {
            has_valid_buffer = TRUE;
            break;
         }
      }
      emit->has_so = has_valid_buffer;
   }

   if (!emit->has_so)
      return;

   /* XXX: need to flush to get prim_vbuf.c to release its allocation??
    */
   draw_do_flush( draw, DRAW_FLUSH_BACKEND );
}

static boolean
is_component_writable(unsigned mask,
                      unsigned compo)
{
   switch (mask) {
   case TGSI_WRITEMASK_NONE:
      return FALSE;
   case TGSI_WRITEMASK_X:
      return compo == 0;
   case TGSI_WRITEMASK_Y:
      return compo == 1;
   case TGSI_WRITEMASK_XY:
      return compo == 0 || compo == 1;
   case TGSI_WRITEMASK_Z:
      return compo == 2;
   case TGSI_WRITEMASK_XZ:
      return compo == 0 || compo == 2;
   case TGSI_WRITEMASK_YZ:
      return compo == 1 || compo == 2;
   case TGSI_WRITEMASK_XYZ:
      return compo == 0 || compo == 1 || compo == 2;
   case TGSI_WRITEMASK_W:
      return compo == 3;
   case TGSI_WRITEMASK_XW:
      return compo == 0 || compo == 3;
   case TGSI_WRITEMASK_YW:
      return compo == 1 || compo == 3;
   case TGSI_WRITEMASK_XYW:
      return compo == 0 || compo == 1 || compo == 3;
   case TGSI_WRITEMASK_ZW:
      return compo == 2 || compo == 3;
   case TGSI_WRITEMASK_XZW:
      return compo == 0 || compo == 1 || compo == 3;
   case TGSI_WRITEMASK_YZW:
      return compo == 1 || compo == 2 || compo == 4;
   case TGSI_WRITEMASK_XYZW:
      return compo < 4;
   default:
      debug_assert(!"Unknown writemask in stream out");
      return compo < 4;
   }
}

static INLINE int mask_num_comps(int register_mask)
{
   int comps = 0;
   switch (register_mask) {
   case TGSI_WRITEMASK_XYZW:
      comps = 4;
      break;
   case TGSI_WRITEMASK_XYZ:
      comps = 3;
      break;
   case TGSI_WRITEMASK_XY:
      comps = 2;
      break;
   case TGSI_WRITEMASK_X:
      comps = 1;
      break;
   default:
      assert(0);
      break;
   }
   return comps;
}

static void so_emit_prim(struct pt_so_emit *so,
                         unsigned *indices,
                         unsigned num_vertices)
{
   unsigned slot, i;
   unsigned input_vertex_stride = so->input_vertex_stride;
   struct draw_context *draw = so->draw;
   const float (*input_ptr)[4];
   const struct pipe_stream_output_info *state =
      &draw->vs.vertex_shader->state.stream_output;
   float *buffer;
   int buffer_total_bytes[PIPE_MAX_SO_BUFFERS];

   input_ptr = so->inputs;

   ++so->generated_primitives;

   for (i = 0; i < draw->so.num_targets; i++) {
      struct draw_so_target *target = draw->so.targets[i];
      buffer_total_bytes[i] = target->internal_offset;
   }

   /* check have we space to emit prim first - if not don't do anything */
   for (i = 0; i < num_vertices; ++i) {
      for (slot = 0; slot < state->num_outputs; ++slot) {
         unsigned writemask = state->output[slot].register_mask;
         int ob = state->output[slot].output_buffer;

         if ((buffer_total_bytes[ob] + mask_num_comps(writemask) * sizeof(float)) >
             draw->so.targets[ob]->target.buffer_size) {
            return;
         }
         buffer_total_bytes[ob] += mask_num_comps(writemask) * sizeof(float);
      }
   }

   for (i = 0; i < num_vertices; ++i) {
      const float (*input)[4];
      unsigned total_written_compos = 0;
      /*debug_printf("%d) vertex index = %d (prim idx = %d)\n", i, indices[i], prim_idx);*/
      input = (const float (*)[4])(
         (const char *)input_ptr + (indices[i] * input_vertex_stride));

      for (slot = 0; slot < state->num_outputs; ++slot) {
         unsigned idx = state->output[slot].register_index;
         unsigned writemask = state->output[slot].register_mask;
         unsigned written_compos = 0;
         unsigned compo;
         int ob = state->output[slot].output_buffer;

         buffer = (float *)((char *)draw->so.targets[ob]->mapping +
                            draw->so.targets[ob]->target.buffer_offset +
                            draw->so.targets[ob]->internal_offset);
         for (compo = 0; compo < 4; ++compo) {
            if (is_component_writable(writemask, compo)) {
               buffer[written_compos++] = input[idx][compo];
            }
         }
         draw->so.targets[ob]->internal_offset += written_compos * sizeof(float);
         total_written_compos += written_compos;
      }
   }
   so->emitted_vertices += num_vertices;
   ++so->emitted_primitives;
}

static void so_point(struct pt_so_emit *so, int idx)
{
   unsigned indices[1];

   indices[0] = idx;

   so_emit_prim(so, indices, 1);
}

static void so_line(struct pt_so_emit *so, int i0, int i1)
{
   unsigned indices[2];

   indices[0] = i0;
   indices[1] = i1;

   so_emit_prim(so, indices, 2);
}

static void so_tri(struct pt_so_emit *so, int i0, int i1, int i2)
{
   unsigned indices[3];

   indices[0] = i0;
   indices[1] = i1;
   indices[2] = i2;

   so_emit_prim(so, indices, 3);
}


#define FUNC         so_run_linear
#define GET_ELT(idx) (start + (idx))
#include "draw_so_emit_tmp.h"


#define FUNC         so_run_elts
#define LOCAL_VARS   const ushort *elts = input_prims->elts;
#define GET_ELT(idx) (elts[start + (idx)])
#include "draw_so_emit_tmp.h"


void draw_pt_so_emit( struct pt_so_emit *emit,
                      const struct draw_vertex_info *input_verts,
                      const struct draw_prim_info *input_prims )
{
   struct draw_context *draw = emit->draw;
   struct vbuf_render *render = draw->render;
   unsigned start, i;

   if (!emit->has_so)
      return;

   emit->emitted_vertices = 0;
   emit->emitted_primitives = 0;
   emit->generated_primitives = 0;
   emit->input_vertex_stride = input_verts->stride;
   emit->inputs = (const float (*)[4])input_verts->verts->data;

   /* XXX: need to flush to get prim_vbuf.c to release its allocation??*/
   draw_do_flush( draw, DRAW_FLUSH_BACKEND );

   for (start = i = 0; i < input_prims->primitive_count;
        start += input_prims->primitive_lengths[i], i++)
   {
      unsigned count = input_prims->primitive_lengths[i];

      if (input_prims->linear) {
         so_run_linear(emit, input_prims, input_verts,
                       start, count);
      } else {
         so_run_elts(emit, input_prims, input_verts,
                     start, count);
      }
   }

   render->set_stream_output_info(render,
                                  emit->emitted_primitives,
                                  emit->emitted_vertices,
                                  emit->generated_primitives);
}


struct pt_so_emit *draw_pt_so_emit_create( struct draw_context *draw )
{
   struct pt_so_emit *emit = CALLOC_STRUCT(pt_so_emit);
   if (!emit)
      return NULL;

   emit->draw = draw;

   return emit;
}

void draw_pt_so_emit_destroy( struct pt_so_emit *emit )
{
   FREE(emit);
}
