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


#include "main/api_arrayelt.h"
#include "main/glheader.h"
#include "main/imports.h"
#include "main/context.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/vtxfmt.h"

#include "vbo_context.h"

void vbo_exec_init( GLcontext *ctx )
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
   ctx->Driver.FlushVertices = vbo_exec_FlushVertices;

   vbo_exec_invalidate_state( ctx, ~0 );
}


void vbo_exec_destroy( GLcontext *ctx )
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

   if (ctx->aelt_context) {
      _ae_destroy_context( ctx );
      ctx->aelt_context = NULL;
   }

   vbo_exec_vtx_destroy( exec );
   vbo_exec_array_destroy( exec );
}

/* Really want to install these callbacks to a central facility to be
 * invoked according to the state flags.  That will have to wait for a
 * mesa rework:
 */ 
void vbo_exec_invalidate_state( GLcontext *ctx, GLuint new_state )
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

   if (new_state & (_NEW_PROGRAM|_NEW_EVAL))
      exec->eval.recalculate_maps = 1;

   _ae_invalidate_state(ctx, new_state);
}





