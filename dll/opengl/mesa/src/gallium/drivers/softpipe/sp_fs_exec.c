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

/**
 * Execute fragment shader using the TGSI interpreter.
 */

#include "sp_context.h"
#include "sp_state.h"
#include "sp_fs.h"
#include "sp_quad.h"

#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "tgsi/tgsi_exec.h"
#include "tgsi/tgsi_parse.h"


/**
 * Subclass of sp_fragment_shader_variant
 */
struct sp_exec_fragment_shader
{
   struct sp_fragment_shader_variant base;
   /* No other members for now */
};


/** cast wrapper */
static INLINE struct sp_exec_fragment_shader *
sp_exec_fragment_shader(const struct sp_fragment_shader_variant *var)
{
   return (struct sp_exec_fragment_shader *) var;
}


static void
exec_prepare( const struct sp_fragment_shader_variant *var,
	      struct tgsi_exec_machine *machine,
	      struct tgsi_sampler **samplers )
{
   /*
    * Bind tokens/shader to the interpreter's machine state.
    * Avoid redundant binding.
    */
   if (machine->Tokens != var->tokens) {
      tgsi_exec_machine_bind_shader( machine,
                                     var->tokens,
                                     PIPE_MAX_SAMPLERS,
                                     samplers );
   }
}



/**
 * Compute quad X,Y,Z,W for the four fragments in a quad.
 *
 * This should really be part of the compiled shader.
 */
static void
setup_pos_vector(const struct tgsi_interp_coef *coef,
                 float x, float y,
                 struct tgsi_exec_vector *quadpos)
{
   uint chan;
   /* do X */
   quadpos->xyzw[0].f[0] = x;
   quadpos->xyzw[0].f[1] = x + 1;
   quadpos->xyzw[0].f[2] = x;
   quadpos->xyzw[0].f[3] = x + 1;

   /* do Y */
   quadpos->xyzw[1].f[0] = y;
   quadpos->xyzw[1].f[1] = y;
   quadpos->xyzw[1].f[2] = y + 1;
   quadpos->xyzw[1].f[3] = y + 1;

   /* do Z and W for all fragments in the quad */
   for (chan = 2; chan < 4; chan++) {
      const float dadx = coef->dadx[chan];
      const float dady = coef->dady[chan];
      const float a0 = coef->a0[chan] + dadx * x + dady * y;
      quadpos->xyzw[chan].f[0] = a0;
      quadpos->xyzw[chan].f[1] = a0 + dadx;
      quadpos->xyzw[chan].f[2] = a0 + dady;
      quadpos->xyzw[chan].f[3] = a0 + dadx + dady;
   }
}


/* TODO: hide the machine struct in here somewhere, remove from this
 * interface:
 */
static unsigned 
exec_run( const struct sp_fragment_shader_variant *var,
	  struct tgsi_exec_machine *machine,
	  struct quad_header *quad )
{
   /* Compute X, Y, Z, W vals for this quad */
   setup_pos_vector(quad->posCoef, 
                    (float)quad->input.x0, (float)quad->input.y0, 
                    &machine->QuadPos);

   /* convert 0 to 1.0 and 1 to -1.0 */
   machine->Face = (float) (quad->input.facing * -2 + 1);

   quad->inout.mask &= tgsi_exec_machine_run( machine );
   if (quad->inout.mask == 0)
      return FALSE;

   /* store outputs */
   {
      const ubyte *sem_name = var->info.output_semantic_name;
      const ubyte *sem_index = var->info.output_semantic_index;
      const uint n = var->info.num_outputs;
      uint i;
      for (i = 0; i < n; i++) {
         switch (sem_name[i]) {
         case TGSI_SEMANTIC_COLOR:
            {
               uint cbuf = sem_index[i];

               assert(sizeof(quad->output.color[cbuf]) ==
                      sizeof(machine->Outputs[i]));

               /* copy float[4][4] result */
               memcpy(quad->output.color[cbuf],
                      &machine->Outputs[i],
                      sizeof(quad->output.color[0]) );
            }
            break;
         case TGSI_SEMANTIC_POSITION:
            {
               uint j;

               for (j = 0; j < 4; j++)
                  quad->output.depth[j] = machine->Outputs[i].xyzw[2].f[j];
            }
            break;
         case TGSI_SEMANTIC_STENCIL:
            {
               uint j;

               for (j = 0; j < 4; j++)
                  quad->output.stencil[j] = (unsigned)machine->Outputs[i].xyzw[1].f[j];
            }
            break;
         }
      }
   }

   return TRUE;
}


static void 
exec_delete( struct sp_fragment_shader_variant *var )
{
   FREE( (void *) var->tokens );
   FREE(var);
}


struct sp_fragment_shader_variant *
softpipe_create_fs_variant_exec(struct softpipe_context *softpipe,
                                const struct pipe_shader_state *templ)
{
   struct sp_exec_fragment_shader *shader;

   shader = CALLOC_STRUCT(sp_exec_fragment_shader);
   if (!shader)
      return NULL;

   shader->base.prepare = exec_prepare;
   shader->base.run = exec_run;
   shader->base.delete = exec_delete;

   return &shader->base;
}
