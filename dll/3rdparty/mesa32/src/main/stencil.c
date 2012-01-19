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
 * Note: There's some conflict between GL_EXT_stencil_two_side and
 * OpenGL 2.0's two-sided stencil feature.
 *
 * With GL_EXT_stencil_two_side, calling glStencilOp/Func/Mask() only the
 * front OR back face state (as set by glActiveStencilFaceEXT) is set.
 *
 * But with OpenGL 2.0, calling glStencilOp/Func/Mask() sets BOTH the
 * front AND back state.
 *
 * Also, note that GL_ATI_separate_stencil is different as well:
 * glStencilFuncSeparateATI(GLenum frontfunc, GLenum backfunc, ...)  vs.
 * glStencilFuncSeparate(GLenum face, GLenum func, ...).
 *
 * This problem is solved by keeping three sets of stencil state:
 *  state[0] = GL_FRONT state.
 *  state[1] = OpenGL 2.0 / GL_ATI_separate_stencil GL_BACK state.
 *  state[2] = GL_EXT_stencil_two_side GL_BACK state.
 */


#include "glheader.h"
#include "imports.h"
#include "context.h"
#include "macros.h"
#include "stencil.h"
#include "mtypes.h"


static GLboolean
validate_stencil_op(GLcontext *ctx, GLenum op)
{
   switch (op) {
   case GL_KEEP:
   case GL_ZERO:
   case GL_REPLACE:
   case GL_INCR:
   case GL_DECR:
   case GL_INVERT:
      return GL_TRUE;
   case GL_INCR_WRAP_EXT:
   case GL_DECR_WRAP_EXT:
      if (ctx->Extensions.EXT_stencil_wrap) {
         return GL_TRUE;
      }
      /* FALL-THROUGH */
   default:
      return GL_FALSE;
   }
}


static GLboolean
validate_stencil_func(GLcontext *ctx, GLenum func)
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
 * \param frontfunc front test function.
 * \param backfunc back test function.
 * \param ref front and back reference value.
 * \param mask front and back bitmask.
 *
 * \sa glStencilFunc().
 *
 * Verifies the parameters and updates the respective values in
 * __GLcontextRec::Stencil. On change flushes the vertices and notifies the
 * driver via the dd_function_table::StencilFunc callback.
 */
void GLAPIENTRY
_mesa_StencilFuncSeparateATI( GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask )
{
   GET_CURRENT_CONTEXT(ctx);
   const GLint stencilMax = (1 << ctx->DrawBuffer->Visual.stencilBits) - 1;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!validate_stencil_func(ctx, frontfunc)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glStencilFuncSeparateATI(frontfunc)");
      return;
   }
   if (!validate_stencil_func(ctx, backfunc)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glStencilFuncSeparateATI(backfunc)");
      return;
   }

   ref = CLAMP( ref, 0, stencilMax );

   /* set both front and back state */
   if (ctx->Stencil.Function[0] == frontfunc &&
       ctx->Stencil.Function[1] == backfunc &&
       ctx->Stencil.ValueMask[0] == mask &&
       ctx->Stencil.ValueMask[1] == mask &&
       ctx->Stencil.Ref[0] == ref &&
       ctx->Stencil.Ref[1] == ref)
      return;
   FLUSH_VERTICES(ctx, _NEW_STENCIL);
   ctx->Stencil.Function[0]  = frontfunc;
   ctx->Stencil.Function[1]  = backfunc;
   ctx->Stencil.Ref[0]       = ctx->Stencil.Ref[1]       = ref;
   ctx->Stencil.ValueMask[0] = ctx->Stencil.ValueMask[1] = mask;
   if (ctx->Driver.StencilFuncSeparate) {
      ctx->Driver.StencilFuncSeparate(ctx, GL_FRONT,
                                      frontfunc, ref, mask);
      ctx->Driver.StencilFuncSeparate(ctx, GL_BACK,
                                      backfunc, ref, mask);
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
   const GLint stencilMax = (1 << ctx->DrawBuffer->Visual.stencilBits) - 1;
   const GLint face = ctx->Stencil.ActiveFace;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!validate_stencil_func(ctx, func)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glStencilFunc(func)");
      return;
   }

   ref = CLAMP( ref, 0, stencilMax );

   if (face != 0) {
      if (ctx->Stencil.Function[face] == func &&
          ctx->Stencil.ValueMask[face] == mask &&
          ctx->Stencil.Ref[face] == ref)
         return;
      FLUSH_VERTICES(ctx, _NEW_STENCIL);
      ctx->Stencil.Function[face] = func;
      ctx->Stencil.Ref[face] = ref;
      ctx->Stencil.ValueMask[face] = mask;

      /* Only propagate the change to the driver if EXT_stencil_two_side
       * is enabled.
       */
      if (ctx->Driver.StencilFuncSeparate && ctx->Stencil.TestTwoSide) {
         ctx->Driver.StencilFuncSeparate(ctx, GL_BACK, func, ref, mask);
      }
   }
   else {
      /* set both front and back state */
      if (ctx->Stencil.Function[0] == func &&
          ctx->Stencil.Function[1] == func &&
          ctx->Stencil.ValueMask[0] == mask &&
          ctx->Stencil.ValueMask[1] == mask &&
          ctx->Stencil.Ref[0] == ref &&
          ctx->Stencil.Ref[1] == ref)
         return;
      FLUSH_VERTICES(ctx, _NEW_STENCIL);
      ctx->Stencil.Function[0]  = ctx->Stencil.Function[1]  = func;
      ctx->Stencil.Ref[0]       = ctx->Stencil.Ref[1]       = ref;
      ctx->Stencil.ValueMask[0] = ctx->Stencil.ValueMask[1] = mask;
      if (ctx->Driver.StencilFuncSeparate) {
         ctx->Driver.StencilFuncSeparate(ctx,
					 ((ctx->Stencil.TestTwoSide)
					  ? GL_FRONT : GL_FRONT_AND_BACK),
                                         func, ref, mask);
      }
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

   if (face != 0) {
      /* Only modify the EXT_stencil_two_side back-face state.
       */
      if (ctx->Stencil.WriteMask[face] == mask)
         return;
      FLUSH_VERTICES(ctx, _NEW_STENCIL);
      ctx->Stencil.WriteMask[face] = mask;

      /* Only propagate the change to the driver if EXT_stencil_two_side
       * is enabled.
       */
      if (ctx->Driver.StencilMaskSeparate && ctx->Stencil.TestTwoSide) {
         ctx->Driver.StencilMaskSeparate(ctx, GL_BACK, mask);
      }
   }
   else {
      /* set both front and back state */
      if (ctx->Stencil.WriteMask[0] == mask &&
          ctx->Stencil.WriteMask[1] == mask)
         return;
      FLUSH_VERTICES(ctx, _NEW_STENCIL);
      ctx->Stencil.WriteMask[0] = ctx->Stencil.WriteMask[1] = mask;
      if (ctx->Driver.StencilMaskSeparate) {
         ctx->Driver.StencilMaskSeparate(ctx,
					 ((ctx->Stencil.TestTwoSide)
					  ? GL_FRONT : GL_FRONT_AND_BACK),
					  mask);
      }
   }
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
 * __GLcontextRec::Stencil. On change flushes the vertices and notifies the
 * driver via the dd_function_table::StencilOp callback.
 */
void GLAPIENTRY
_mesa_StencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
   GET_CURRENT_CONTEXT(ctx);
   const GLint face = ctx->Stencil.ActiveFace;

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

   if (face != 0) {
      /* only set active face state */
      if (ctx->Stencil.ZFailFunc[face] == zfail &&
          ctx->Stencil.ZPassFunc[face] == zpass &&
          ctx->Stencil.FailFunc[face] == fail)
         return;
      FLUSH_VERTICES(ctx, _NEW_STENCIL);
      ctx->Stencil.ZFailFunc[face] = zfail;
      ctx->Stencil.ZPassFunc[face] = zpass;
      ctx->Stencil.FailFunc[face] = fail;

      /* Only propagate the change to the driver if EXT_stencil_two_side
       * is enabled.
       */
      if (ctx->Driver.StencilOpSeparate && ctx->Stencil.TestTwoSide) {
         ctx->Driver.StencilOpSeparate(ctx, GL_BACK, fail, zfail, zpass);
      }
   }
   else {
      /* set both front and back state */
      if (ctx->Stencil.ZFailFunc[0] == zfail &&
          ctx->Stencil.ZFailFunc[1] == zfail &&
          ctx->Stencil.ZPassFunc[0] == zpass &&
          ctx->Stencil.ZPassFunc[1] == zpass &&
          ctx->Stencil.FailFunc[0] == fail &&
          ctx->Stencil.FailFunc[1] == fail)
         return;
      FLUSH_VERTICES(ctx, _NEW_STENCIL);
      ctx->Stencil.ZFailFunc[0] = ctx->Stencil.ZFailFunc[1] = zfail;
      ctx->Stencil.ZPassFunc[0] = ctx->Stencil.ZPassFunc[1] = zpass;
      ctx->Stencil.FailFunc[0]  = ctx->Stencil.FailFunc[1]  = fail;
      if (ctx->Driver.StencilOpSeparate) {
         ctx->Driver.StencilOpSeparate(ctx,
				       ((ctx->Stencil.TestTwoSide)
					? GL_FRONT : GL_FRONT_AND_BACK),
                                       fail, zfail, zpass);
      }
   }
}



#if _HAVE_FULL_GL
/* GL_EXT_stencil_two_side */
void GLAPIENTRY
_mesa_ActiveStencilFaceEXT(GLenum face)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!ctx->Extensions.EXT_stencil_two_side) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glActiveStencilFaceEXT");
      return;
   }

   if (face == GL_FRONT || face == GL_BACK) {
      FLUSH_VERTICES(ctx, _NEW_STENCIL);
      ctx->Stencil.ActiveFace = (face == GL_FRONT) ? 0 : 2;
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glActiveStencilFaceEXT(face)");
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
_mesa_StencilOpSeparate(GLenum face, GLenum sfail, GLenum zfail, GLenum zpass)
{
   GLboolean set = GL_FALSE;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!validate_stencil_op(ctx, sfail)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glStencilOpSeparate(sfail)");
      return;
   }
   if (!validate_stencil_op(ctx, zfail)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glStencilOpSeparate(zfail)");
      return;
   }
   if (!validate_stencil_op(ctx, zpass)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glStencilOpSeparate(zpass)");
      return;
   }
   if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glStencilOpSeparate(face)");
      return;
   }

   if (face != GL_BACK) {
      /* set front */
      if (ctx->Stencil.ZFailFunc[0] != zfail ||
          ctx->Stencil.ZPassFunc[0] != zpass ||
          ctx->Stencil.FailFunc[0] != sfail){
         FLUSH_VERTICES(ctx, _NEW_STENCIL);
         ctx->Stencil.ZFailFunc[0] = zfail;
         ctx->Stencil.ZPassFunc[0] = zpass;
         ctx->Stencil.FailFunc[0] = sfail;
         set = GL_TRUE;
      }
   }
   if (face != GL_FRONT) {
      /* set back */
      if (ctx->Stencil.ZFailFunc[1] != zfail ||
          ctx->Stencil.ZPassFunc[1] != zpass ||
          ctx->Stencil.FailFunc[1] != sfail) {
         FLUSH_VERTICES(ctx, _NEW_STENCIL);
         ctx->Stencil.ZFailFunc[1] = zfail;
         ctx->Stencil.ZPassFunc[1] = zpass;
         ctx->Stencil.FailFunc[1] = sfail;
         set = GL_TRUE;
      }
   }
   if (set && ctx->Driver.StencilOpSeparate) {
      ctx->Driver.StencilOpSeparate(ctx, face, sfail, zfail, zpass);
   }
}


/* OpenGL 2.0 */
void GLAPIENTRY
_mesa_StencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
{
   GET_CURRENT_CONTEXT(ctx);
   const GLint stencilMax = (1 << ctx->DrawBuffer->Visual.stencilBits) - 1;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glStencilFuncSeparate(face)");
      return;
   }
   if (!validate_stencil_func(ctx, func)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glStencilFuncSeparate(func)");
      return;
   }

   ref = CLAMP(ref, 0, stencilMax);

   FLUSH_VERTICES(ctx, _NEW_STENCIL);

   if (face != GL_BACK) {
      /* set front */
      ctx->Stencil.Function[0] = func;
      ctx->Stencil.Ref[0] = ref;
      ctx->Stencil.ValueMask[0] = mask;
   }
   if (face != GL_FRONT) {
      /* set back */
      ctx->Stencil.Function[1] = func;
      ctx->Stencil.Ref[1] = ref;
      ctx->Stencil.ValueMask[1] = mask;
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

   if (face != GL_BACK) {
      ctx->Stencil.WriteMask[0] = mask;
   }
   if (face != GL_FRONT) {
      ctx->Stencil.WriteMask[1] = mask;
   }
   if (ctx->Driver.StencilMaskSeparate) {
      ctx->Driver.StencilMaskSeparate(ctx, face, mask);
   }
}


/**
 * Update derived stencil state.
 */
void
_mesa_update_stencil(GLcontext *ctx)
{
   const GLint face = ctx->Stencil._BackFace;

   ctx->Stencil._TestTwoSide =
       (ctx->Stencil.Function[0] != ctx->Stencil.Function[face] ||
	ctx->Stencil.FailFunc[0] != ctx->Stencil.FailFunc[face] ||
	ctx->Stencil.ZPassFunc[0] != ctx->Stencil.ZPassFunc[face] ||
	ctx->Stencil.ZFailFunc[0] != ctx->Stencil.ZFailFunc[face] ||
	ctx->Stencil.Ref[0] != ctx->Stencil.Ref[face] ||
	ctx->Stencil.ValueMask[0] != ctx->Stencil.ValueMask[face] ||
	ctx->Stencil.WriteMask[0] != ctx->Stencil.WriteMask[face]);
}


/**
 * Initialize the context stipple state.
 *
 * \param ctx GL context.
 *
 * Initializes __GLcontextRec::Stencil attribute group.
 */
void
_mesa_init_stencil(GLcontext *ctx)
{
   ctx->Stencil.Enabled = GL_FALSE;
   ctx->Stencil.TestTwoSide = GL_FALSE;
   ctx->Stencil.ActiveFace = 0;  /* 0 = GL_FRONT, 2 = GL_BACK */
   ctx->Stencil.Function[0] = GL_ALWAYS;
   ctx->Stencil.Function[1] = GL_ALWAYS;
   ctx->Stencil.Function[2] = GL_ALWAYS;
   ctx->Stencil.FailFunc[0] = GL_KEEP;
   ctx->Stencil.FailFunc[1] = GL_KEEP;
   ctx->Stencil.FailFunc[2] = GL_KEEP;
   ctx->Stencil.ZPassFunc[0] = GL_KEEP;
   ctx->Stencil.ZPassFunc[1] = GL_KEEP;
   ctx->Stencil.ZPassFunc[2] = GL_KEEP;
   ctx->Stencil.ZFailFunc[0] = GL_KEEP;
   ctx->Stencil.ZFailFunc[1] = GL_KEEP;
   ctx->Stencil.ZFailFunc[2] = GL_KEEP;
   ctx->Stencil.Ref[0] = 0;
   ctx->Stencil.Ref[1] = 0;
   ctx->Stencil.Ref[2] = 0;
   ctx->Stencil.ValueMask[0] = ~0U;
   ctx->Stencil.ValueMask[1] = ~0U;
   ctx->Stencil.ValueMask[2] = ~0U;
   ctx->Stencil.WriteMask[0] = ~0U;
   ctx->Stencil.WriteMask[1] = ~0U;
   ctx->Stencil.WriteMask[2] = ~0U;
   ctx->Stencil.Clear = 0;
   ctx->Stencil._BackFace = 1;
}
