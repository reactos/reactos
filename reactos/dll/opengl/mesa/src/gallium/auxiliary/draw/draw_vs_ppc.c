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

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Brian Paul
  */

#include "util/u_math.h"
#include "util/u_memory.h"
#include "pipe/p_config.h"

#include "draw_vs.h"

#if defined(PIPE_ARCH_PPC)

#include "pipe/p_shader_tokens.h"

#include "draw_private.h"
#include "draw_context.h"

#include "rtasm/rtasm_cpu.h"
#include "rtasm/rtasm_ppc.h"
#include "tgsi/tgsi_ppc.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_exec.h"



typedef void (PIPE_CDECL *codegen_function) (float (*inputs)[4][4],
                                             float (*outputs)[4][4],
                                             float (*temps)[4][4],
                                             float (*immeds)[4],
                                             float (*consts)[4],
                                             const float *builtins);


struct draw_ppc_vertex_shader {
   struct draw_vertex_shader base;
   struct ppc_function ppc_program;

   codegen_function func;
};


static void
vs_ppc_prepare( struct draw_vertex_shader *base,
		struct draw_context *draw )
{
   /* nothing */
}


/**
 * Simplified vertex shader interface for the pt paths.  Given the
 * complexity of code-generating all the above operations together,
 * it's time to try doing all the other stuff separately.
 */
static void
vs_ppc_run_linear( struct draw_vertex_shader *base,
		   const float (*input)[4],
		   float (*output)[4],
                  const void *constants[PIPE_MAX_CONSTANT_BUFFERS],
		   unsigned count,
		   unsigned input_stride,
		   unsigned output_stride )
{
   struct draw_ppc_vertex_shader *shader = (struct draw_ppc_vertex_shader *)base;
   unsigned int i;

#define MAX_VERTICES 4

   /* loop over verts */
   for (i = 0; i < count; i += MAX_VERTICES) {
      const uint max_vertices = MIN2(MAX_VERTICES, count - i);
      PIPE_ALIGN_VAR(16) float inputs_soa[PIPE_MAX_SHADER_INPUTS][4][4];
      PIPE_ALIGN_VAR(16) float outputs_soa[PIPE_MAX_SHADER_OUTPUTS][4][4];
      PIPE_ALIGN_VAR(16) float temps_soa[TGSI_EXEC_NUM_TEMPS][4][4];
      uint attr;

      /* convert (up to) four input verts to SoA format */
      for (attr = 0; attr < base->info.num_inputs; attr++) {
         const float *vIn = (const float *) input;
         uint vert;
         for (vert = 0; vert < max_vertices; vert++) {
#if 0
            if (attr==0)
               printf("Input v%d a%d: %f %f %f %f\n",
                      vert, attr, vIn[0], vIn[1], vIn[2], vIn[3]);
#endif
            inputs_soa[attr][0][vert] = vIn[attr * 4 + 0];
            inputs_soa[attr][1][vert] = vIn[attr * 4 + 1];
            inputs_soa[attr][2][vert] = vIn[attr * 4 + 2];
            inputs_soa[attr][3][vert] = vIn[attr * 4 + 3];
            vIn += input_stride / 4;
         }
      }

      /* run compiled shader
       */
      shader->func(inputs_soa, outputs_soa, temps_soa,
		   (float (*)[4]) shader->base.immediates,
                   (float (*)[4])constants[0],
                   ppc_builtin_constants);

      /* convert (up to) four output verts from SoA back to AoS format */
      for (attr = 0; attr < base->info.num_outputs; attr++) {
         float *vOut = (float *) output;
         uint vert;
         for (vert = 0; vert < max_vertices; vert++) {
            vOut[attr * 4 + 0] = outputs_soa[attr][0][vert];
            vOut[attr * 4 + 1] = outputs_soa[attr][1][vert];
            vOut[attr * 4 + 2] = outputs_soa[attr][2][vert];
            vOut[attr * 4 + 3] = outputs_soa[attr][3][vert];
#if 0
            if (attr==0)
               printf("Output v%d a%d: %f %f %f %f\n",
                      vert, attr, vOut[0], vOut[1], vOut[2], vOut[3]);
#endif
            vOut += output_stride / 4;
         }
      }

      /* advance to next group of four input/output verts */
      input = (const float (*)[4])((const char *)input + input_stride * max_vertices);
      output = (float (*)[4])((char *)output + output_stride * max_vertices);
   }
}


static void
vs_ppc_delete( struct draw_vertex_shader *base )
{
   struct draw_ppc_vertex_shader *shader = (struct draw_ppc_vertex_shader *)base;
   
   ppc_release_func( &shader->ppc_program );

   align_free( (void *) shader->base.immediates );

   FREE( (void*) shader->base.state.tokens );
   FREE( shader );
}


struct draw_vertex_shader *
draw_create_vs_ppc(struct draw_context *draw,
                   const struct pipe_shader_state *templ)
{
   struct draw_ppc_vertex_shader *vs;

   vs = CALLOC_STRUCT( draw_ppc_vertex_shader );
   if (vs == NULL) 
      return NULL;

   /* we make a private copy of the tokens */
   vs->base.state.tokens = tgsi_dup_tokens(templ->tokens);
   if (!vs->base.state.tokens)
      goto fail;

   tgsi_scan_shader(templ->tokens, &vs->base.info);

   vs->base.draw = draw;
   vs->base.create_variant = draw_vs_create_variant_generic;
   vs->base.prepare = vs_ppc_prepare;
   vs->base.run_linear = vs_ppc_run_linear;
   vs->base.delete = vs_ppc_delete;
   
   vs->base.immediates = align_malloc(TGSI_EXEC_NUM_IMMEDIATES * 4 *
                                      sizeof(float), 16);

   ppc_init_func( &vs->ppc_program );

#if 0
   ppc_print_code(&vs->ppc_program, TRUE);
   ppc_indent(&vs->ppc_program, 8);
#endif

   if (!tgsi_emit_ppc( (struct tgsi_token *) vs->base.state.tokens,
			&vs->ppc_program, 
                       (float (*)[4]) vs->base.immediates, 
                        TRUE )) 
      goto fail;
      
   vs->func = (codegen_function) ppc_get_func( &vs->ppc_program );
   if (!vs->func) {
      goto fail;
   }
   
   return &vs->base;

fail:
   /*
   debug_error("tgsi_emit_ppc() failed, falling back to interpreter\n");
   */

   ppc_release_func( &vs->ppc_program );
   
   FREE(vs);
   return NULL;
}



#else /* PIPE_ARCH_PPC */


struct draw_vertex_shader *
draw_create_vs_ppc( struct draw_context *draw,
		    const struct pipe_shader_state *templ )
{
   return (void *) 0;
}


#endif /* PIPE_ARCH_PPC */
