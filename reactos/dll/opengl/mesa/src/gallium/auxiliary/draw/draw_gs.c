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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "draw_gs.h"

#include "draw_private.h"
#include "draw_context.h"

#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_exec.h"

#include "pipe/p_shader_tokens.h"

#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_prim.h"

/* fixme: move it from here */
#define MAX_PRIMITIVES 64

boolean
draw_gs_init( struct draw_context *draw )
{
   draw->gs.machine = tgsi_exec_machine_create();
   if (!draw->gs.machine)
      return FALSE;

   draw->gs.machine->Primitives = align_malloc(
      MAX_PRIMITIVES * sizeof(struct tgsi_exec_vector), 16);
   if (!draw->gs.machine->Primitives)
      return FALSE;
   memset(draw->gs.machine->Primitives, 0,
          MAX_PRIMITIVES * sizeof(struct tgsi_exec_vector));

   return TRUE;
}

void draw_gs_destroy( struct draw_context *draw )
{
   if (!draw->gs.machine)
      return;

   align_free(draw->gs.machine->Primitives);

   tgsi_exec_machine_destroy(draw->gs.machine);
}

void
draw_gs_set_constants(struct draw_context *draw,
                      unsigned slot,
                      const void *constants,
                      unsigned size)
{
   /* noop. added here for symmetry with the VS
    * code and in case we'll ever want to allign
    * the constants, e.g. when we'll change to a
    * different interpreter */
}


struct draw_geometry_shader *
draw_create_geometry_shader(struct draw_context *draw,
                            const struct pipe_shader_state *state)
{
   struct draw_geometry_shader *gs;
   int i;

   gs = CALLOC_STRUCT(draw_geometry_shader);

   if (!gs)
      return NULL;

   gs->draw = draw;
   gs->state = *state;
   gs->state.tokens = tgsi_dup_tokens(state->tokens);
   if (!gs->state.tokens) {
      FREE(gs);
      return NULL;
   }

   tgsi_scan_shader(state->tokens, &gs->info);

   /* setup the defaults */
   gs->input_primitive = PIPE_PRIM_TRIANGLES;
   gs->output_primitive = PIPE_PRIM_TRIANGLE_STRIP;
   gs->max_output_vertices = 32;

   for (i = 0; i < gs->info.num_properties; ++i) {
      if (gs->info.properties[i].name ==
          TGSI_PROPERTY_GS_INPUT_PRIM)
         gs->input_primitive = gs->info.properties[i].data[0];
      else if (gs->info.properties[i].name ==
               TGSI_PROPERTY_GS_OUTPUT_PRIM)
         gs->output_primitive = gs->info.properties[i].data[0];
      else if (gs->info.properties[i].name ==
               TGSI_PROPERTY_GS_MAX_OUTPUT_VERTICES)
         gs->max_output_vertices = gs->info.properties[i].data[0];
   }

   gs->machine = draw->gs.machine;

   if (gs)
   {
      uint i;
      for (i = 0; i < gs->info.num_outputs; i++) {
         if (gs->info.output_semantic_name[i] == TGSI_SEMANTIC_POSITION &&
             gs->info.output_semantic_index[i] == 0)
            gs->position_output = i;
      }
   }

   return gs;
}

void draw_bind_geometry_shader(struct draw_context *draw,
                               struct draw_geometry_shader *dgs)
{
   draw_do_flush(draw, DRAW_FLUSH_STATE_CHANGE);

   if (dgs) {
      draw->gs.geometry_shader = dgs;
      draw->gs.num_gs_outputs = dgs->info.num_outputs;
      draw->gs.position_output = dgs->position_output;
      draw_geometry_shader_prepare(dgs, draw);
   }
   else {
      draw->gs.geometry_shader = NULL;
      draw->gs.num_gs_outputs = 0;
   }
}

void draw_delete_geometry_shader(struct draw_context *draw,
                                 struct draw_geometry_shader *dgs)
{
   FREE(dgs);
}

/*#define DEBUG_OUTPUTS 1*/
static INLINE void
draw_geometry_fetch_outputs(struct draw_geometry_shader *shader,
                            int num_primitives,
                            float (**p_output)[4])
{
   struct tgsi_exec_machine *machine = shader->machine;
   unsigned prim_idx, j, slot;
   float (*output)[4];

   output = *p_output;

   /* Unswizzle all output results.
    */

   for (prim_idx = 0; prim_idx < num_primitives; ++prim_idx) {
      unsigned num_verts_per_prim = machine->Primitives[prim_idx];
      shader->primitive_lengths[prim_idx +   shader->emitted_primitives] =
         machine->Primitives[prim_idx];
      shader->emitted_vertices += num_verts_per_prim;
      for (j = 0; j < num_verts_per_prim; j++) {
         int idx = (prim_idx * num_verts_per_prim + j) *
                   shader->info.num_outputs;
#ifdef DEBUG_OUTPUTS
         debug_printf("%d) Output vert:\n", idx / shader->info.num_outputs);
#endif
         for (slot = 0; slot < shader->info.num_outputs; slot++) {
            output[slot][0] = machine->Outputs[idx + slot].xyzw[0].f[0];
            output[slot][1] = machine->Outputs[idx + slot].xyzw[1].f[0];
            output[slot][2] = machine->Outputs[idx + slot].xyzw[2].f[0];
            output[slot][3] = machine->Outputs[idx + slot].xyzw[3].f[0];
#ifdef DEBUG_OUTPUTS
            debug_printf("\t%d: %f %f %f %f\n", slot,
                         output[slot][0],
                         output[slot][1],
                         output[slot][2],
                         output[slot][3]);
#endif
            debug_assert(!util_is_inf_or_nan(output[slot][0]));
         }
         output = (float (*)[4])((char *)output + shader->vertex_size);
      }
   }
   *p_output = output;
         shader->emitted_primitives += num_primitives;
}

/*#define DEBUG_INPUTS 1*/
static void draw_fetch_gs_input(struct draw_geometry_shader *shader,
                                unsigned *indices,
                                unsigned num_vertices,
                                unsigned prim_idx)
{
   struct tgsi_exec_machine *machine = shader->machine;
   unsigned slot, vs_slot, i;
   unsigned input_vertex_stride = shader->input_vertex_stride;
   const float (*input_ptr)[4];

   input_ptr = shader->input;

   for (i = 0; i < num_vertices; ++i) {
      const float (*input)[4];
#if DEBUG_INPUTS
      debug_printf("%d) vertex index = %d (prim idx = %d)\n",
                   i, indices[i], prim_idx);
#endif
      input = (const float (*)[4])(
         (const char *)input_ptr + (indices[i] * input_vertex_stride));
      for (slot = 0, vs_slot = 0; slot < shader->info.num_inputs; ++slot) {
         unsigned idx = i * TGSI_EXEC_MAX_INPUT_ATTRIBS + slot;
         if (shader->info.input_semantic_name[slot] == TGSI_SEMANTIC_PRIMID) {
            machine->Inputs[idx].xyzw[0].f[prim_idx] =
               (float)shader->in_prim_idx;
            machine->Inputs[idx].xyzw[1].f[prim_idx] =
               (float)shader->in_prim_idx;
            machine->Inputs[idx].xyzw[2].f[prim_idx] =
               (float)shader->in_prim_idx;
            machine->Inputs[idx].xyzw[3].f[prim_idx] =
               (float)shader->in_prim_idx;
         } else {
#if DEBUG_INPUTS
            debug_printf("\tSlot = %d, vs_slot = %d, idx = %d:\n",
                         slot, vs_slot, idx);
#endif
#if 1
            assert(!util_is_inf_or_nan(input[vs_slot][0]));
            assert(!util_is_inf_or_nan(input[vs_slot][1]));
            assert(!util_is_inf_or_nan(input[vs_slot][2]));
            assert(!util_is_inf_or_nan(input[vs_slot][3]));
#endif
            machine->Inputs[idx].xyzw[0].f[prim_idx] = input[vs_slot][0];
            machine->Inputs[idx].xyzw[1].f[prim_idx] = input[vs_slot][1];
            machine->Inputs[idx].xyzw[2].f[prim_idx] = input[vs_slot][2];
            machine->Inputs[idx].xyzw[3].f[prim_idx] = input[vs_slot][3];
#if DEBUG_INPUTS
            debug_printf("\t\t%f %f %f %f\n",
                         machine->Inputs[idx].xyzw[0].f[prim_idx],
                         machine->Inputs[idx].xyzw[1].f[prim_idx],
                         machine->Inputs[idx].xyzw[2].f[prim_idx],
                         machine->Inputs[idx].xyzw[3].f[prim_idx]);
#endif
            ++vs_slot;
         }
      }
   }
}

static void gs_flush(struct draw_geometry_shader *shader,
                     unsigned input_primitives)
{
   unsigned out_prim_count;
   struct tgsi_exec_machine *machine = shader->machine;

   debug_assert(input_primitives > 0 &&
                input_primitives < 4);

   tgsi_set_exec_mask(machine,
                      1,
                      input_primitives > 1,
                      input_primitives > 2,
                      input_primitives > 3);

   /* run interpreter */
   tgsi_exec_machine_run(machine);

   out_prim_count =
      machine->Temps[TGSI_EXEC_TEMP_PRIMITIVE_I].xyzw[TGSI_EXEC_TEMP_PRIMITIVE_C].u[0];

#if 0
   debug_printf("PRIM emitted prims = %d (verts=%d), cur prim count = %d\n",
                shader->emitted_primitives, shader->emitted_vertices,
                out_prim_count);
#endif
   draw_geometry_fetch_outputs(shader, out_prim_count,
                               &shader->tmp_output);
}

static void gs_point(struct draw_geometry_shader *shader,
                     int idx)
{
   unsigned indices[1];

   indices[0] = idx;

   draw_fetch_gs_input(shader, indices, 1, 0);
   ++shader->in_prim_idx;

   gs_flush(shader, 1);
}

static void gs_line(struct draw_geometry_shader *shader,
                    int i0, int i1)
{
   unsigned indices[2];

   indices[0] = i0;
   indices[1] = i1;

   draw_fetch_gs_input(shader, indices, 2, 0);
   ++shader->in_prim_idx;

   gs_flush(shader, 1);
}

static void gs_line_adj(struct draw_geometry_shader *shader,
                        int i0, int i1, int i2, int i3)
{
   unsigned indices[4];

   indices[0] = i0;
   indices[1] = i1;
   indices[2] = i2;
   indices[3] = i3;

   draw_fetch_gs_input(shader, indices, 4, 0);
   ++shader->in_prim_idx;

   gs_flush(shader, 1);
}

static void gs_tri(struct draw_geometry_shader *shader,
                   int i0, int i1, int i2)
{
   unsigned indices[3];

   indices[0] = i0;
   indices[1] = i1;
   indices[2] = i2;

   draw_fetch_gs_input(shader, indices, 3, 0);
   ++shader->in_prim_idx;

   gs_flush(shader, 1);
}

static void gs_tri_adj(struct draw_geometry_shader *shader,
                       int i0, int i1, int i2,
                       int i3, int i4, int i5)
{
   unsigned indices[6];

   indices[0] = i0;
   indices[1] = i1;
   indices[2] = i2;
   indices[3] = i3;
   indices[4] = i4;
   indices[5] = i5;

   draw_fetch_gs_input(shader, indices, 6, 0);
   ++shader->in_prim_idx;

   gs_flush(shader, 1);
}

#define FUNC         gs_run
#define GET_ELT(idx) (idx)
#include "draw_gs_tmp.h"


#define FUNC         gs_run_elts
#define LOCAL_VARS   const ushort *elts = input_prims->elts;
#define GET_ELT(idx) (elts[idx])
#include "draw_gs_tmp.h"


/**
 * Execute geometry shader using TGSI interpreter.
 */
int draw_geometry_shader_run(struct draw_geometry_shader *shader,
                             const void *constants[PIPE_MAX_CONSTANT_BUFFERS], 
                             const unsigned constants_size[PIPE_MAX_CONSTANT_BUFFERS], 
                             const struct draw_vertex_info *input_verts,
                             const struct draw_prim_info *input_prim,
                             struct draw_vertex_info *output_verts,
                             struct draw_prim_info *output_prims )
{
   const float (*input)[4] = (const float (*)[4])input_verts->verts->data;
   unsigned input_stride = input_verts->vertex_size;
   unsigned vertex_size = input_verts->vertex_size;
   struct tgsi_exec_machine *machine = shader->machine;
   unsigned num_input_verts = input_prim->linear ?
                              input_verts->count :
                              input_prim->count;
   unsigned num_in_primitives =
      MAX2(u_gs_prims_for_vertices(input_prim->prim, num_input_verts),
           u_gs_prims_for_vertices(shader->input_primitive, num_input_verts));
   unsigned max_out_prims = u_gs_prims_for_vertices(shader->output_primitive,
                                                    shader->max_output_vertices)
                            * num_in_primitives;

   output_verts->vertex_size = input_verts->vertex_size;
   output_verts->stride = input_verts->vertex_size;
   output_verts->verts =
      (struct vertex_header *)MALLOC(input_verts->vertex_size *
                                     num_in_primitives *
                                     shader->max_output_vertices);


#if 0
   debug_printf("%s count = %d (in prims # = %d)\n",
                __FUNCTION__, num_input_verts, num_in_primitives);
   debug_printf("\tlinear = %d, prim_info->count = %d\n",
                input_prim->linear, input_prim->count);
   debug_printf("\tprimt pipe = %d, shader in = %d, shader out = %d, max out = %d\n",
                input_prim->prim, shader->input_primitive,
                shader->output_primitive,
                shader->max_output_vertices);
#endif

   shader->emitted_vertices = 0;
   shader->emitted_primitives = 0;
   shader->vertex_size = vertex_size;
   shader->tmp_output = (float (*)[4])output_verts->verts->data;
   shader->in_prim_idx = 0;
   shader->input_vertex_stride = input_stride;
   shader->input = input;
   if (shader->primitive_lengths) {
      FREE(shader->primitive_lengths);
   }
   shader->primitive_lengths = MALLOC(max_out_prims * sizeof(unsigned));

   tgsi_exec_set_constant_buffers(machine, PIPE_MAX_CONSTANT_BUFFERS,
                                  constants, constants_size);

   if (input_prim->linear)
      gs_run(shader, input_prim, input_verts,
             output_prims, output_verts);
   else
      gs_run_elts(shader, input_prim, input_verts,
                  output_prims, output_verts);

   /* Update prim_info:
    */
   output_prims->linear = TRUE;
   output_prims->elts = NULL;
   output_prims->start = 0;
   output_prims->count = shader->emitted_vertices;
   output_prims->prim = shader->output_primitive;
   output_prims->flags = 0x0;
   output_prims->primitive_lengths = shader->primitive_lengths;
   output_prims->primitive_count = shader->emitted_primitives;
   output_verts->count = shader->emitted_vertices;

#if 0
   debug_printf("GS finished, prims = %d, verts = %d\n",
                output_prims->primitive_count,
                output_verts->count);
#endif

   return shader->emitted_vertices;
}

void draw_geometry_shader_delete(struct draw_geometry_shader *shader)
{
   FREE((void*) shader->state.tokens);
   FREE(shader);
}

void draw_geometry_shader_prepare(struct draw_geometry_shader *shader,
                                  struct draw_context *draw)
{
   if (shader && shader->machine->Tokens != shader->state.tokens) {
      tgsi_exec_machine_bind_shader(shader->machine,
                                    shader->state.tokens,
                                    draw->gs.num_samplers,
                                    draw->gs.samplers);
   }
}
