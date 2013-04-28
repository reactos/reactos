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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "util/u_math.h"
#include "util/u_memory.h"
#include "draw/draw_context.h"
#include "draw/draw_gs.h"
#include "draw/draw_vbuf.h"
#include "draw/draw_vertex.h"
#include "draw/draw_pt.h"
#include "draw/draw_vs.h"
#include "draw/draw_llvm.h"
#include "gallivm/lp_bld_init.h"


struct llvm_middle_end {
   struct draw_pt_middle_end base;
   struct draw_context *draw;

   struct pt_emit *emit;
   struct pt_so_emit *so_emit;
   struct pt_fetch *fetch;
   struct pt_post_vs *post_vs;


   unsigned vertex_data_offset;
   unsigned vertex_size;
   unsigned input_prim;
   unsigned opt;

   struct draw_llvm *llvm;
   struct draw_llvm_variant *current_variant;
};


static void
llvm_middle_end_prepare( struct draw_pt_middle_end *middle,
                         unsigned in_prim,
                         unsigned opt,
                         unsigned *max_vertices )
{
   struct llvm_middle_end *fpme = (struct llvm_middle_end *)middle;
   struct draw_context *draw = fpme->draw;
   struct llvm_vertex_shader *shader =
      llvm_vertex_shader(draw->vs.vertex_shader);
   char store[DRAW_LLVM_MAX_VARIANT_KEY_SIZE];
   struct draw_llvm_variant_key *key;
   struct draw_llvm_variant *variant = NULL;
   struct draw_llvm_variant_list_item *li;
   const unsigned out_prim = (draw->gs.geometry_shader ? 
                              draw->gs.geometry_shader->output_primitive :
                              in_prim);

   /* Add one to num_outputs because the pipeline occasionally tags on
    * an additional texcoord, eg for AA lines.
    */
   const unsigned nr = MAX2( shader->base.info.num_inputs,
                             shader->base.info.num_outputs + 1 );

   fpme->input_prim = in_prim;
   fpme->opt = opt;

   /* Always leave room for the vertex header whether we need it or
    * not.  It's hard to get rid of it in particular because of the
    * viewport code in draw_pt_post_vs.c.
    */
   fpme->vertex_size = sizeof(struct vertex_header) + nr * 4 * sizeof(float);


   /* XXX: it's not really gl rasterization rules we care about here,
    * but gl vs dx9 clip spaces.
    */
   draw_pt_post_vs_prepare( fpme->post_vs,
			    draw->clip_xy,
			    draw->clip_z,
			    draw->clip_user,
                            draw->guard_band_xy,
			    draw->identity_viewport,
			    (boolean)draw->rasterizer->gl_rasterization_rules,
			    (draw->vs.edgeflag_output ? TRUE : FALSE) );

   draw_pt_so_emit_prepare( fpme->so_emit );

   if (!(opt & PT_PIPELINE)) {
      draw_pt_emit_prepare( fpme->emit,
			    out_prim,
                            max_vertices );

      *max_vertices = MAX2( *max_vertices, 4096 );
   }
   else {
      /* limit max fetches by limiting max_vertices */
      *max_vertices = 4096;
   }

   /* return even number */
   *max_vertices = *max_vertices & ~1;
   
   key = draw_llvm_make_variant_key(fpme->llvm, store);

   /* Search shader's list of variants for the key */
   li = first_elem(&shader->variants);
   while (!at_end(&shader->variants, li)) {
      if (memcmp(&li->base->key, key, shader->variant_key_size) == 0) {
         variant = li->base;
         break;
      }
      li = next_elem(li);
   }

   if (variant) {
      /* found the variant, move to head of global list (for LRU) */
      move_to_head(&fpme->llvm->vs_variants_list, &variant->list_item_global);
   }
   else {
      /* Need to create new variant */
      unsigned i;

      /* First check if we've created too many variants.  If so, free
       * 25% of the LRU to avoid using too much memory.
       */
      if (fpme->llvm->nr_variants >= DRAW_MAX_SHADER_VARIANTS) {
         /*
          * XXX: should we flush here ?
          */
         for (i = 0; i < DRAW_MAX_SHADER_VARIANTS / 4; i++) {
            struct draw_llvm_variant_list_item *item;
            if (is_empty_list(&fpme->llvm->vs_variants_list)) {
               break;
            }
            item = last_elem(&fpme->llvm->vs_variants_list);
            assert(item);
            assert(item->base);
            draw_llvm_destroy_variant(item->base);
         }
      }

      variant = draw_llvm_create_variant(fpme->llvm, nr, key);

      if (variant) {
         insert_at_head(&shader->variants, &variant->list_item_local);
         insert_at_head(&fpme->llvm->vs_variants_list, &variant->list_item_global);
         fpme->llvm->nr_variants++;
         shader->variants_cached++;
      }
   }

   fpme->current_variant = variant;

   /*XXX we only support one constant buffer */
   fpme->llvm->jit_context.vs_constants =
      draw->pt.user.vs_constants[0];
   fpme->llvm->jit_context.gs_constants =
      draw->pt.user.gs_constants[0];
   fpme->llvm->jit_context.planes =
      (float (*) [DRAW_TOTAL_CLIP_PLANES][4]) draw->pt.user.planes[0];
   fpme->llvm->jit_context.viewport =
      (float *)draw->viewport.scale;
    
}


static void pipeline(struct llvm_middle_end *llvm,
                     const struct draw_vertex_info *vert_info,
                     const struct draw_prim_info *prim_info)
{
   if (prim_info->linear)
      draw_pipeline_run_linear( llvm->draw,
                                vert_info,
                                prim_info);
   else
      draw_pipeline_run( llvm->draw,
                         vert_info,
                         prim_info );
}

static void emit(struct pt_emit *emit,
                 const struct draw_vertex_info *vert_info,
                 const struct draw_prim_info *prim_info)
{
   if (prim_info->linear) {
      draw_pt_emit_linear(emit, vert_info, prim_info);
   }
   else {
      draw_pt_emit(emit, vert_info, prim_info);
   }
}

static void
llvm_pipeline_generic( struct draw_pt_middle_end *middle,
                       const struct draw_fetch_info *fetch_info,
                       const struct draw_prim_info *prim_info )
{
   struct llvm_middle_end *fpme = (struct llvm_middle_end *)middle;
   struct draw_context *draw = fpme->draw;
   struct draw_geometry_shader *gshader = draw->gs.geometry_shader;
   struct draw_prim_info gs_prim_info;
   struct draw_vertex_info llvm_vert_info;
   struct draw_vertex_info gs_vert_info;
   struct draw_vertex_info *vert_info;
   unsigned opt = fpme->opt;
   unsigned clipped = 0;

   llvm_vert_info.count = fetch_info->count;
   llvm_vert_info.vertex_size = fpme->vertex_size;
   llvm_vert_info.stride = fpme->vertex_size;
   llvm_vert_info.verts =
      (struct vertex_header *)MALLOC(fpme->vertex_size *
                                     align(fetch_info->count,  4));
   if (!llvm_vert_info.verts) {
      assert(0);
      return;
   }

   if (fetch_info->linear)
      clipped = fpme->current_variant->jit_func( &fpme->llvm->jit_context,
                                       llvm_vert_info.verts,
                                       (const char **)draw->pt.user.vbuffer,
                                       fetch_info->start,
                                       fetch_info->count,
                                       fpme->vertex_size,
                                       draw->pt.vertex_buffer,
                                       draw->instance_id);
   else
      clipped = fpme->current_variant->jit_func_elts( &fpme->llvm->jit_context,
                                            llvm_vert_info.verts,
                                            (const char **)draw->pt.user.vbuffer,
                                            fetch_info->elts,
                                            fetch_info->count,
                                            fpme->vertex_size,
                                            draw->pt.vertex_buffer,
                                            draw->instance_id);

   /* Finished with fetch and vs:
    */
   fetch_info = NULL;
   vert_info = &llvm_vert_info;


   if ((opt & PT_SHADE) && gshader) {
      draw_geometry_shader_run(gshader,
                               draw->pt.user.gs_constants,
                               draw->pt.user.gs_constants_size,
                               vert_info,
                               prim_info,
                               &gs_vert_info,
                               &gs_prim_info);

      FREE(vert_info->verts);
      vert_info = &gs_vert_info;
      prim_info = &gs_prim_info;

      clipped = draw_pt_post_vs_run( fpme->post_vs, vert_info );

   }

   /* stream output needs to be done before clipping */
   draw_pt_so_emit( fpme->so_emit,
		    vert_info,
                    prim_info );

   if (clipped) {
      opt |= PT_PIPELINE;
   }

   /* Do we need to run the pipeline? Now will come here if clipped
    */
   if (opt & PT_PIPELINE) {
      pipeline( fpme,
                vert_info,
                prim_info );
   }
   else {
      emit( fpme->emit,
            vert_info,
            prim_info );
   }
   FREE(vert_info->verts);
}


static void llvm_middle_end_run( struct draw_pt_middle_end *middle,
                                 const unsigned *fetch_elts,
                                 unsigned fetch_count,
                                 const ushort *draw_elts,
                                 unsigned draw_count,
                                 unsigned prim_flags )
{
   struct llvm_middle_end *fpme = (struct llvm_middle_end *)middle;
   struct draw_fetch_info fetch_info;
   struct draw_prim_info prim_info;

   fetch_info.linear = FALSE;
   fetch_info.start = 0;
   fetch_info.elts = fetch_elts;
   fetch_info.count = fetch_count;

   prim_info.linear = FALSE;
   prim_info.start = 0;
   prim_info.count = draw_count;
   prim_info.elts = draw_elts;
   prim_info.prim = fpme->input_prim;
   prim_info.flags = prim_flags;
   prim_info.primitive_count = 1;
   prim_info.primitive_lengths = &draw_count;

   llvm_pipeline_generic( middle, &fetch_info, &prim_info );
}


static void llvm_middle_end_linear_run( struct draw_pt_middle_end *middle,
                                       unsigned start,
                                       unsigned count,
                                       unsigned prim_flags)
{
   struct llvm_middle_end *fpme = (struct llvm_middle_end *)middle;
   struct draw_fetch_info fetch_info;
   struct draw_prim_info prim_info;

   fetch_info.linear = TRUE;
   fetch_info.start = start;
   fetch_info.count = count;
   fetch_info.elts = NULL;

   prim_info.linear = TRUE;
   prim_info.start = 0;
   prim_info.count = count;
   prim_info.elts = NULL;
   prim_info.prim = fpme->input_prim;
   prim_info.flags = prim_flags;
   prim_info.primitive_count = 1;
   prim_info.primitive_lengths = &count;

   llvm_pipeline_generic( middle, &fetch_info, &prim_info );
}



static boolean
llvm_middle_end_linear_run_elts( struct draw_pt_middle_end *middle,
                                 unsigned start,
                                 unsigned count,
                                 const ushort *draw_elts,
                                 unsigned draw_count,
                                 unsigned prim_flags )
{
   struct llvm_middle_end *fpme = (struct llvm_middle_end *)middle;
   struct draw_fetch_info fetch_info;
   struct draw_prim_info prim_info;

   fetch_info.linear = TRUE;
   fetch_info.start = start;
   fetch_info.count = count;
   fetch_info.elts = NULL;

   prim_info.linear = FALSE;
   prim_info.start = 0;
   prim_info.count = draw_count;
   prim_info.elts = draw_elts;
   prim_info.prim = fpme->input_prim;
   prim_info.flags = prim_flags;
   prim_info.primitive_count = 1;
   prim_info.primitive_lengths = &draw_count;

   llvm_pipeline_generic( middle, &fetch_info, &prim_info );

   return TRUE;
}



static void llvm_middle_end_finish( struct draw_pt_middle_end *middle )
{
   /* nothing to do */
}

static void llvm_middle_end_destroy( struct draw_pt_middle_end *middle )
{
   struct llvm_middle_end *fpme = (struct llvm_middle_end *)middle;

   if (fpme->fetch)
      draw_pt_fetch_destroy( fpme->fetch );

   if (fpme->emit)
      draw_pt_emit_destroy( fpme->emit );

   if (fpme->so_emit)
      draw_pt_so_emit_destroy( fpme->so_emit );

   if (fpme->post_vs)
      draw_pt_post_vs_destroy( fpme->post_vs );

   FREE(middle);
}


struct draw_pt_middle_end *
draw_pt_fetch_pipeline_or_emit_llvm(struct draw_context *draw)
{
   struct llvm_middle_end *fpme = 0;

   if (!draw->llvm || !draw->llvm->gallivm->engine)
      return NULL;

   fpme = CALLOC_STRUCT( llvm_middle_end );
   if (!fpme)
      goto fail;

   fpme->base.prepare         = llvm_middle_end_prepare;
   fpme->base.run             = llvm_middle_end_run;
   fpme->base.run_linear      = llvm_middle_end_linear_run;
   fpme->base.run_linear_elts = llvm_middle_end_linear_run_elts;
   fpme->base.finish          = llvm_middle_end_finish;
   fpme->base.destroy         = llvm_middle_end_destroy;

   fpme->draw = draw;

   fpme->fetch = draw_pt_fetch_create( draw );
   if (!fpme->fetch)
      goto fail;

   fpme->post_vs = draw_pt_post_vs_create( draw );
   if (!fpme->post_vs)
      goto fail;

   fpme->emit = draw_pt_emit_create( draw );
   if (!fpme->emit)
      goto fail;

   fpme->so_emit = draw_pt_so_emit_create( draw );
   if (!fpme->so_emit)
      goto fail;

   fpme->llvm = draw->llvm;
   if (!fpme->llvm)
      goto fail;

   fpme->current_variant = NULL;

   return &fpme->base;

 fail:
   if (fpme)
      llvm_middle_end_destroy( &fpme->base );

   return NULL;
}
