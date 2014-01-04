/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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
 */

#include <precomp.h>

/**
 * Called via glScissor
 */
void GLAPIENTRY
_mesa_Scissor( GLint x, GLint y, GLsizei width, GLsizei height )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glScissor %d %d %d %d\n", x, y, width, height);

   if (width < 0 || height < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glScissor" );
      return;
   }

   _mesa_set_scissor(ctx, x, y, width, height);
}


/**
 * Define the scissor box.
 *
 * \param x, y coordinates of the scissor box lower-left corner.
 * \param width width of the scissor box.
 * \param height height of the scissor box.
 *
 * \sa glScissor().
 *
 * Verifies the parameters and updates __struct gl_contextRec::Scissor. On a
 * change flushes the vertices and notifies the driver via
 * the dd_function_table::Scissor callback.
 */
void
_mesa_set_scissor(struct gl_context *ctx, 
                  GLint x, GLint y, GLsizei width, GLsizei height)
{
   if (x == ctx->Scissor.X &&
       y == ctx->Scissor.Y &&
       width == ctx->Scissor.Width &&
       height == ctx->Scissor.Height)
      return;

   FLUSH_VERTICES(ctx, _NEW_SCISSOR);
   ctx->Scissor.X = x;
   ctx->Scissor.Y = y;
   ctx->Scissor.Width = width;
   ctx->Scissor.Height = height;

   if (ctx->Driver.Scissor)
      ctx->Driver.Scissor( ctx, x, y, width, height );
}


/**
 * Initialize the context's scissor state.
 * \param ctx  the GL context.
 */
void
_mesa_init_scissor(struct gl_context *ctx)
{
   /* Scissor group */
   ctx->Scissor.Enabled = GL_FALSE;
   ctx->Scissor.X = 0;
   ctx->Scissor.Y = 0;
   ctx->Scissor.Width = 0;
   ctx->Scissor.Height = 0;
}
