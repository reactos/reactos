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


/**
 * \file stencil.c
 * Stencil operations.
 *
 */

#include <precomp.h>

static GLboolean
validate_stencil_op(struct gl_context *ctx, GLenum op)
{
   switch (op) {
   case GL_KEEP:
   case GL_ZERO:
   case GL_REPLACE:
   case GL_INCR:
   case GL_DECR:
   case GL_INVERT:
   case GL_INCR_WRAP:
   case GL_DECR_WRAP:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


static GLboolean
validate_stencil_func(struct gl_context *ctx, GLenum func)
{
   switch (func) {
   case GL_NEVER:
   case GL_LESS:
   case GL_LEQUAL:
   case GL_GREATER:
   case GL_GEQUAL:
   case GL_EQUAL:
   case GL_NOTEQUAL:
   case GL_ALWAYS:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


/**
 * Set the clear value for the stencil buffer.
 *
 * \param s clear value.
 *
 * \sa glClearStencil().
 *
 * Updates gl_stencil_attrib::Clear. On change
 * flushes the vertices and notifies the driver via
 * the dd_function_table::ClearStencil callback.
 */
void GLAPIENTRY
_mesa_ClearStencil( GLint s )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->Stencil.Clear == (GLuint) s)
      return;

   FLUSH_VERTICES(ctx, _NEW_STENCIL);
   ctx->Stencil.Clear = (GLuint) s;

   if (ctx->Driver.ClearStencil) {
      ctx->Driver.ClearStencil( ctx, s );
   }
}

/**
 * Set the function and reference value for stencil testing.
 *
 * \param func test function.
 * \param ref reference value.
 * \param mask bitmask.
 *
 * \sa glStencilFunc().
 *
 * Verifies the parameters and updates the respective values in
 * __struct gl_contextRec::Stencil. On change flushes the vertices and notifies the
 * driver via the dd_function_table::StencilFunc callback.
 */
void GLAPIENTRY
_mesa_StencilFunc( GLenum func, GLint ref, GLuint mask )
{
   GET_CURRENT_CONTEXT(ctx);
   const GLint stencilMax = (1 << ctx->DrawBuffer->Visual.stencilBits) - 1;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glStencilFunc()\n");

   if (!validate_stencil_func(ctx, func)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glStencilFunc(func)");
      return;
   }

   ref = CLAMP( ref, 0, stencilMax );

   /* set both front and back state */
   if (ctx->Stencil.Function == func &&
       ctx->Stencil.ValueMask == mask &&
       ctx->Stencil.Ref == ref)
      return;
   FLUSH_VERTICES(ctx, _NEW_STENCIL);
   ctx->Stencil.Function = func;
   ctx->Stencil.Ref = ref;
   ctx->Stencil.ValueMask = mask;
}


/**
 * Set the stencil writing mask.
 *
 * \param mask bit-mask to enable/disable writing of individual bits in the
 * stencil planes.
 *
 * \sa glStencilMask().
 *
 * Updates gl_stencil_attrib::WriteMask. On change flushes the vertices and
 * notifies the driver via the dd_function_table::StencilMask callback.
 */
void GLAPIENTRY
_mesa_StencilMask( GLuint mask )
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glStencilMask()\n");

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   /* set both front and back state */
   if (ctx->Stencil.WriteMask == mask)
      return;
   FLUSH_VERTICES(ctx, _NEW_STENCIL);
   ctx->Stencil.WriteMask = mask;
}


/**
 * Set the stencil test actions.
 *
 * \param fail action to take when stencil test fails.
 * \param zfail action to take when stencil test passes, but depth test fails.
 * \param zpass action to take when stencil test passes and the depth test
 * passes (or depth testing is not enabled).
 * 
 * \sa glStencilOp().
 * 
 * Verifies the parameters and updates the respective fields in
 * __struct gl_contextRec::Stencil. On change flushes the vertices and notifies the
 * driver via the dd_function_table::StencilOp callback.
 */
void GLAPIENTRY
_mesa_StencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glStencilOp()\n");

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!validate_stencil_op(ctx, fail)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glStencilOp(sfail)");
      return;
   }
   if (!validate_stencil_op(ctx, zfail)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glStencilOp(zfail)");
      return;
   }
   if (!validate_stencil_op(ctx, zpass)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glStencilOp(zpass)");
      return;
   }

   /* set both front and back state */
   if (ctx->Stencil.ZFailFunc == zfail &&
       ctx->Stencil.ZPassFunc == zpass &&
       ctx->Stencil.FailFunc == fail)
      return;
   FLUSH_VERTICES(ctx, _NEW_STENCIL);
   ctx->Stencil.ZFailFunc = zfail;
   ctx->Stencil.ZPassFunc = zpass;
   ctx->Stencil.FailFunc  = fail;
}

/**
 * Update derived stencil state.
 */
void
_mesa_update_stencil(struct gl_context *ctx)
{
   ctx->Stencil._Enabled = (ctx->Stencil.Enabled &&
                            ctx->DrawBuffer->Visual.stencilBits > 0);
}


/**
 * Initialize the context stipple state.
 *
 * \param ctx GL context.
 *
 * Initializes __struct gl_contextRec::Stencil attribute group.
 */
void
_mesa_init_stencil(struct gl_context *ctx)
{
   ctx->Stencil.Enabled = GL_FALSE;
   ctx->Stencil.Function = GL_ALWAYS;
   ctx->Stencil.FailFunc = GL_KEEP;
   ctx->Stencil.ZPassFunc = GL_KEEP;
   ctx->Stencil.ZFailFunc = GL_KEEP;
   ctx->Stencil.Ref = 0;
   ctx->Stencil.ValueMask = ~0U;
   ctx->Stencil.WriteMask = ~0U;
   ctx->Stencil.Clear = 0;
}
