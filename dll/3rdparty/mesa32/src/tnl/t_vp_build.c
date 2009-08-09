/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 2007  Tungsten Graphics   All Rights Reserved.
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
 * TUNGSTEN GRAPHICS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file t_vp_build.c
 * Create a vertex program to execute the current fixed function T&L pipeline.
 * \author Keith Whitwell
 */


#include "main/glheader.h"
#include "main/ffvertex_prog.h"
#include "t_vp_build.h"


/**
 * XXX This should go away someday, but still referenced by some drivers...
 */
void _tnl_UpdateFixedFunctionProgram( GLcontext *ctx )
{
   const struct gl_vertex_program *prev = ctx->VertexProgram._Current;

   if (!ctx->VertexProgram._Current ||
       ctx->VertexProgram._Current == ctx->VertexProgram._TnlProgram) {
      ctx->VertexProgram._Current
         = ctx->VertexProgram._TnlProgram
         = _mesa_get_fixed_func_vertex_program(ctx);
   }

   /* Tell the driver about the change.  Could define a new target for
    * this?
    */
   if (ctx->VertexProgram._Current != prev && ctx->Driver.BindProgram) {
      ctx->Driver.BindProgram(ctx, GL_VERTEX_PROGRAM_ARB,
                            (struct gl_program *) ctx->VertexProgram._Current);
   }
}
