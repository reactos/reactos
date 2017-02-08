/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include <precomp.h>

void vbo_exec_init( struct gl_context *ctx )
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

   exec->ctx = ctx;

   /* Initialize the arrayelt helper
    */
   if (!ctx->aelt_context &&
       !_ae_create_context( ctx )) 
      return;

   vbo_exec_vtx_init( exec );
   vbo_exec_array_init( exec );

   /* Hook our functions into exec and compile dispatch tables.
    */
   _mesa_install_exec_vtxfmt( ctx, &exec->vtxfmt );

   ctx->Driver.NeedFlush = 0;
   ctx->Driver.CurrentExecPrimitive = PRIM_OUTSIDE_BEGIN_END;
   ctx->Driver.BeginVertices = vbo_exec_BeginVertices;
   ctx->Driver.FlushVertices = vbo_exec_FlushVertices;

   vbo_exec_invalidate_state( ctx, ~0 );
}


void vbo_exec_destroy( struct gl_context *ctx )
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

   if (ctx->aelt_context) {
      _ae_destroy_context( ctx );
      ctx->aelt_context = NULL;
   }

   vbo_exec_vtx_destroy( exec );
   vbo_exec_array_destroy( exec );
}


/**
 * Really want to install these callbacks to a central facility to be
 * invoked according to the state flags.  That will have to wait for a
 * mesa rework:
 */ 
void vbo_exec_invalidate_state( struct gl_context *ctx, GLuint new_state )
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

   if (new_state & _NEW_EVAL)
      exec->eval.recalculate_maps = 1;

   _ae_invalidate_state(ctx, new_state);
}


/**
 * Figure out the number of transform feedback primitives that will be output
 * by the given _mesa_prim command, assuming that no geometry shading is done
 * and primitive restart is not used.
 *
 * This is intended for use by driver back-ends in implementing the
 * PRIMITIVES_GENERATED and TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN queries.
 */
size_t
count_tessellated_primitives(const struct _mesa_prim *prim)
{
   size_t num_primitives;
   switch (prim->mode) {
   case GL_POINTS:
      num_primitives = prim->count;
      break;
   case GL_LINE_STRIP:
      num_primitives = prim->count >= 2 ? prim->count - 1 : 0;
      break;
   case GL_LINE_LOOP:
      num_primitives = prim->count >= 2 ? prim->count : 0;
      break;
   case GL_LINES:
      num_primitives = prim->count / 2;
      break;
   case GL_TRIANGLE_STRIP:
   case GL_TRIANGLE_FAN:
   case GL_POLYGON:
      num_primitives = prim->count >= 3 ? prim->count - 2 : 0;
      break;
   case GL_TRIANGLES:
      num_primitives = prim->count / 3;
      break;
   case GL_QUAD_STRIP:
      num_primitives = prim->count >= 4 ? ((prim->count / 2) - 1) * 2 : 0;
      break;
   case GL_QUADS:
      num_primitives = (prim->count / 4) * 2;
      break;
   default:
      assert(!"Unexpected primitive type in count_tessellated_primitives");
      num_primitives = 0;
      break;
   }
   return num_primitives * prim->num_instances;
}
