/**
 * \file stencil.c
 * Stencil operations.
 */

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
 */


#include "glheader.h"
#include "imports.h"
#include "context.h"
#include "depth.h"
#include "macros.h"
#include "stencil.h"
#include "mtypes.h"
#include "enable.h"


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

   if (ctx->Stencil.Clear == (GLstencil) s)
      return;

   FLUSH_VERTICES(ctx, _NEW_STENCIL);
   ctx->Stencil.Clear = (GLstencil) s;

   if (ctx->Driver.ClearStencil) {
      (*ctx->Driver.ClearStencil)( ctx, s );
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
 * __GLcontextRec::Stencil. On change flushes the vertices and notifies the
 * driver via the dd_function_table::StencilFunc callback.
 */
void GLAPIENTRY
_mesa_StencilFunc( GLenum func, GLint ref, GLuint mask )
{
   GET_CURRENT_CONTEXT(ctx);
   const GLint face = ctx->Stencil.ActiveFace;
   GLint maxref;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (func) {
      case GL_NEVER:
      case GL_LESS:
      case GL_LEQUAL:
      case GL_GREATER:
      case GL_GEQUAL:
      case GL_EQUAL:
      case GL_NOTEQUAL:
      case GL_ALWAYS:
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glStencilFunc" );
         return;
   }

   maxref = (1 << STENCIL_BITS) - 1;
   ref = (GLstencil) CLAMP( ref, 0, maxref );

   if (ctx->Stencil.Function[face] == func &&
       ctx->Stencil.ValueMask[face] == (GLstencil) mask &&
       ctx->Stencil.Ref[face] == ref)
      return;

   FLUSH_VERTICES(ctx, _NEW_STENCIL);
   ctx->Stencil.Function[face] = func;
   ctx->Stencil.Ref[face] = ref;
   ctx->Stencil.ValueMask[face] = (GLstencil) mask;

   if (ctx->Driver.StencilFunc) {
      (*ctx->Driver.StencilFunc)( ctx, func, ref, mask );
   }
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
   const GLint face = ctx->Stencil.ActiveFace;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->Stencil.WriteMask[face] == (GLstencil) mask)
      return;

   FLUSH_VERTICES(ctx, _NEW_STENCIL);
   ctx->Stencil.WriteMask[face] = (GLstencil) mask;

   if (ctx->Driver.StencilMask) {
      (*ctx->Driver.StencilMask)( ctx, mask );
   }
}


/**
 * Set the stencil test actions.
 *
 * \param fail action to take when stencil test fails.
 * \param zfail action to take when stencil test passes, but the depth test fails.
 * \param zpass action to take when stencil test passes and the depth test
 * passes (or depth testing is not enabled).
 * 
 * \sa glStencilOp().
 * 
 * Verifies the parameters and updates the respective fields in
 * __GLcontextRec::Stencil. On change flushes the vertices and notifies the
 * driver via the dd_function_table::StencilOp callback.
 */
void GLAPIENTRY
_mesa_StencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
   GET_CURRENT_CONTEXT(ctx);
   const GLint face = ctx->Stencil.ActiveFace;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (fail) {
      case GL_KEEP:
      case GL_ZERO:
      case GL_REPLACE:
      case GL_INCR:
      case GL_DECR:
      case GL_INVERT:
         break;
      case GL_INCR_WRAP_EXT:
      case GL_DECR_WRAP_EXT:
         if (ctx->Extensions.EXT_stencil_wrap) {
            break;
         }
         /* FALL-THROUGH */
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glStencilOp");
         return;
   }
   switch (zfail) {
      case GL_KEEP:
      case GL_ZERO:
      case GL_REPLACE:
      case GL_INCR:
      case GL_DECR:
      case GL_INVERT:
         break;
      case GL_INCR_WRAP_EXT:
      case GL_DECR_WRAP_EXT:
         if (ctx->Extensions.EXT_stencil_wrap) {
            break;
         }
         /* FALL-THROUGH */
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glStencilOp");
         return;
   }
   switch (zpass) {
      case GL_KEEP:
      case GL_ZERO:
      case GL_REPLACE:
      case GL_INCR:
      case GL_DECR:
      case GL_INVERT:
         break;
      case GL_INCR_WRAP_EXT:
      case GL_DECR_WRAP_EXT:
         if (ctx->Extensions.EXT_stencil_wrap) {
            break;
         }
         /* FALL-THROUGH */
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glStencilOp");
         return;
   }

   if (ctx->Stencil.ZFailFunc[face] == zfail &&
       ctx->Stencil.ZPassFunc[face] == zpass &&
       ctx->Stencil.FailFunc[face] == fail)
      return;

   FLUSH_VERTICES(ctx, _NEW_STENCIL);
   ctx->Stencil.ZFailFunc[face] = zfail;
   ctx->Stencil.ZPassFunc[face] = zpass;
   ctx->Stencil.FailFunc[face] = fail;

   if (ctx->Driver.StencilOp) {
      (*ctx->Driver.StencilOp)(ctx, fail, zfail, zpass);
   }
}



#if _HAVE_FULL_GL
/* GL_EXT_stencil_two_side */
void GLAPIENTRY
_mesa_ActiveStencilFaceEXT(GLenum face)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (face == GL_FRONT || face == GL_BACK) {
      FLUSH_VERTICES(ctx, _NEW_STENCIL);
      ctx->Stencil.ActiveFace = (face == GL_FRONT) ? 0 : 1;
   }

   if (ctx->Driver.ActiveStencilFace) {
      (*ctx->Driver.ActiveStencilFace)( ctx, (GLuint) ctx->Stencil.ActiveFace );
   }
}
#endif



/**
 * OpenGL 2.0 function.
 * \todo Make StencilOp() call this function.  And eventually remove the
 * ctx->Driver.StencilOp function and use ctx->Driver.StencilOpSeparate
 * instead.
 */
void GLAPIENTRY
_mesa_StencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glStencilOpSeparate(face)");
      return;
   }

   switch (fail) {
      case GL_KEEP:
      case GL_ZERO:
      case GL_REPLACE:
      case GL_INCR:
      case GL_DECR:
      case GL_INVERT:
         break;
      case GL_INCR_WRAP_EXT:
      case GL_DECR_WRAP_EXT:
         if (ctx->Extensions.EXT_stencil_wrap) {
            break;
         }
         /* FALL-THROUGH */
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glStencilOpSeparate(fail)");
         return;
   }
   switch (zfail) {
      case GL_KEEP:
      case GL_ZERO:
      case GL_REPLACE:
      case GL_INCR:
      case GL_DECR:
      case GL_INVERT:
         break;
      case GL_INCR_WRAP_EXT:
      case GL_DECR_WRAP_EXT:
         if (ctx->Extensions.EXT_stencil_wrap) {
            break;
         }
         /* FALL-THROUGH */
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glStencilOpSeparate(zfail)");
         return;
   }
   switch (zpass) {
      case GL_KEEP:
      case GL_ZERO:
      case GL_REPLACE:
      case GL_INCR:
      case GL_DECR:
      case GL_INVERT:
         break;
      case GL_INCR_WRAP_EXT:
      case GL_DECR_WRAP_EXT:
         if (ctx->Extensions.EXT_stencil_wrap) {
            break;
         }
         /* FALL-THROUGH */
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glStencilOpSeparate(zpass)");
         return;
   }

   FLUSH_VERTICES(ctx, _NEW_STENCIL);

   if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
      ctx->Stencil.FailFunc[0] = fail;
      ctx->Stencil.ZFailFunc[0] = zfail;
      ctx->Stencil.ZPassFunc[0] = zpass;
   }
   if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
      ctx->Stencil.FailFunc[1] = fail;
      ctx->Stencil.ZFailFunc[1] = zfail;
      ctx->Stencil.ZPassFunc[1] = zpass;
   }

   if (ctx->Driver.StencilOpSeparate) {
      ctx->Driver.StencilOpSeparate(ctx, face, fail, zfail, zpass);
   }
}


/* OpenGL 2.0 */
void GLAPIENTRY
_mesa_StencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint maxref;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glStencilFuncSeparate(face)");
      return;
   }

   switch (func) {
      case GL_NEVER:
      case GL_LESS:
      case GL_LEQUAL:
      case GL_GREATER:
      case GL_GEQUAL:
      case GL_EQUAL:
      case GL_NOTEQUAL:
      case GL_ALWAYS:
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glStencilFuncSeparate(func)");
         return;
   }

   maxref = (1 << STENCIL_BITS) - 1;
   ref = (GLstencil) CLAMP(ref, 0, maxref);

   FLUSH_VERTICES(ctx, _NEW_STENCIL);

   if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
      ctx->Stencil.Function[0] = func;
      ctx->Stencil.Ref[0] = ref;
      ctx->Stencil.ValueMask[0] = (GLstencil) mask;
   }
   if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
      ctx->Stencil.Function[1] = func;
      ctx->Stencil.Ref[1] = ref;
      ctx->Stencil.ValueMask[1] = (GLstencil) mask;
   }

   if (ctx->Driver.StencilFuncSeparate) {
      ctx->Driver.StencilFuncSeparate(ctx, face, func, ref, mask);
   }
}


/* OpenGL 2.0 */
void GLAPIENTRY
_mesa_StencilMaskSeparate(GLenum face, GLuint mask)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glStencilaMaskSeparate(face)");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_STENCIL);

   if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
      ctx->Stencil.WriteMask[0] = (GLstencil) mask;
   }
   if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
      ctx->Stencil.WriteMask[1] = (GLstencil) mask;
   }

   if (ctx->Driver.StencilMaskSeparate) {
      ctx->Driver.StencilMaskSeparate(ctx, face, mask);
   }
}


/**
 * Initialize the context stipple state.
 *
 * \param ctx GL context.
 *
 * Initializes __GLcontextRec::Stencil attribute group.
 */
void _mesa_init_stencil( GLcontext * ctx )
{

   /* Stencil group */
   ctx->Stencil.Enabled = GL_FALSE;
   ctx->Stencil.TestTwoSide = GL_FALSE;
   ctx->Stencil.ActiveFace = 0;  /* 0 = GL_FRONT, 1 = GL_BACK */
   ctx->Stencil.Function[0] = GL_ALWAYS;
   ctx->Stencil.Function[1] = GL_ALWAYS;
   ctx->Stencil.FailFunc[0] = GL_KEEP;
   ctx->Stencil.FailFunc[1] = GL_KEEP;
   ctx->Stencil.ZPassFunc[0] = GL_KEEP;
   ctx->Stencil.ZPassFunc[1] = GL_KEEP;
   ctx->Stencil.ZFailFunc[0] = GL_KEEP;
   ctx->Stencil.ZFailFunc[1] = GL_KEEP;
   ctx->Stencil.Ref[0] = 0;
   ctx->Stencil.Ref[1] = 0;
   ctx->Stencil.ValueMask[0] = STENCIL_MAX;
   ctx->Stencil.ValueMask[1] = STENCIL_MAX;
   ctx->Stencil.WriteMask[0] = STENCIL_MAX;
   ctx->Stencil.WriteMask[1] = STENCIL_MAX;
   ctx->Stencil.Clear = 0;
}
