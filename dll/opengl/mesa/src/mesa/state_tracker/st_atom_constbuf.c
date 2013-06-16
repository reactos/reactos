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
 *   Brian Paul
 */

#include "main/imports.h"
#include "program/prog_parameter.h"
#include "program/prog_print.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"

#include "st_debug.h"
#include "st_context.h"
#include "st_atom.h"
#include "st_atom_constbuf.h"
#include "st_program.h"


/**
 * Pass the given program parameters to the graphics pipe as a
 * constant buffer.
 * \param shader_type  either PIPE_SHADER_VERTEX or PIPE_SHADER_FRAGMENT
 */
void st_upload_constants( struct st_context *st,
                          struct gl_program_parameter_list *params,
                          unsigned shader_type)
{
   struct pipe_context *pipe = st->pipe;

   assert(shader_type == PIPE_SHADER_VERTEX ||
          shader_type == PIPE_SHADER_FRAGMENT ||
          shader_type == PIPE_SHADER_GEOMETRY);

   /* update constants */
   if (params && params->NumParameters) {
      struct pipe_resource *cbuf;
      const uint paramBytes = params->NumParameters * sizeof(GLfloat) * 4;

      /* Update the constants which come from fixed-function state, such as
       * transformation matrices, fog factors, etc.  The rest of the values in
       * the parameters list are explicitly set by the user with glUniform,
       * glProgramParameter(), etc.
       */
      _mesa_load_state_parameters(st->ctx, params);

      /* We always need to get a new buffer, to keep the drivers simple and
       * avoid gratuitous rendering synchronization.
       * Let's use a user buffer to avoid an unnecessary copy.
       */
      cbuf = pipe_user_buffer_create(pipe->screen,
                                     params->ParameterValues,
                                     paramBytes,
                                     PIPE_BIND_CONSTANT_BUFFER);

      if (ST_DEBUG & DEBUG_CONSTANTS) {
	 debug_printf("%s(shader=%d, numParams=%d, stateFlags=0x%x)\n", 
                      __FUNCTION__, shader_type, params->NumParameters,
                      params->StateFlags);
         _mesa_print_parameter_list(params);
      }

      st->pipe->set_constant_buffer(st->pipe, shader_type, 0, cbuf);
      pipe_resource_reference(&cbuf, NULL);

      st->state.constants[shader_type].ptr = params->ParameterValues;
      st->state.constants[shader_type].size = paramBytes;
   }
   else if (st->state.constants[shader_type].ptr) {
      st->state.constants[shader_type].ptr = NULL;
      st->state.constants[shader_type].size = 0;
      st->pipe->set_constant_buffer(st->pipe, shader_type, 0, NULL);
   }
}


/**
 * Vertex shader:
 */
static void update_vs_constants(struct st_context *st )
{
   struct st_vertex_program *vp = st->vp;
   struct gl_program_parameter_list *params = vp->Base.Base.Parameters;

   st_upload_constants( st, params, PIPE_SHADER_VERTEX );
}


const struct st_tracked_state st_update_vs_constants = {
   "st_update_vs_constants",				/* name */
   {							/* dirty */
      (_NEW_PROGRAM | _NEW_PROGRAM_CONSTANTS),          /* mesa */
      ST_NEW_VERTEX_PROGRAM,				/* st */
   },
   update_vs_constants					/* update */
};



/**
 * Fragment shader:
 */
static void update_fs_constants(struct st_context *st )
{
   struct st_fragment_program *fp = st->fp;
   struct gl_program_parameter_list *params = fp->Base.Base.Parameters;

   st_upload_constants( st, params, PIPE_SHADER_FRAGMENT );
}


const struct st_tracked_state st_update_fs_constants = {
   "st_update_fs_constants",				/* name */
   {							/* dirty */
      (_NEW_PROGRAM | _NEW_PROGRAM_CONSTANTS),          /* mesa */
      ST_NEW_FRAGMENT_PROGRAM,				/* st */
   },
   update_fs_constants					/* update */
};

/* Geometry shader:
 */
static void update_gs_constants(struct st_context *st )
{
   struct st_geometry_program *gp = st->gp;
   struct gl_program_parameter_list *params;

   if (gp) {
      params = gp->Base.Base.Parameters;
      st_upload_constants( st, params, PIPE_SHADER_GEOMETRY );
   }
}

const struct st_tracked_state st_update_gs_constants = {
   "st_update_gs_constants",				/* name */
   {							/* dirty */
      (_NEW_PROGRAM | _NEW_PROGRAM_CONSTANTS),          /* mesa */
      ST_NEW_GEOMETRY_PROGRAM,				/* st */
   },
   update_gs_constants					/* update */
};
