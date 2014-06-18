/**
 * \file blend.c
 * Blending operations.
 */

/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
 * Check if given blend source factor is legal.
 * \return GL_TRUE if legal, GL_FALSE otherwise.
 */
static GLboolean
legal_src_factor(const struct gl_context *ctx, GLenum factor)
{
   switch (factor) {
   case GL_SRC_COLOR:
   case GL_ONE_MINUS_SRC_COLOR:
      return ctx->Extensions.NV_blend_square;
   case GL_ZERO:
   case GL_ONE:
   case GL_DST_COLOR:
   case GL_ONE_MINUS_DST_COLOR:
   case GL_SRC_ALPHA:
   case GL_ONE_MINUS_SRC_ALPHA:
   case GL_DST_ALPHA:
   case GL_ONE_MINUS_DST_ALPHA:
   case GL_SRC_ALPHA_SATURATE:
   case GL_CONSTANT_COLOR:
   case GL_ONE_MINUS_CONSTANT_COLOR:
   case GL_CONSTANT_ALPHA:
   case GL_ONE_MINUS_CONSTANT_ALPHA:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


/**
 * Check if given blend destination factor is legal.
 * \return GL_TRUE if legal, GL_FALSE otherwise.
 */
static GLboolean
legal_dst_factor(const struct gl_context *ctx, GLenum factor)
{
   switch (factor) {
   case GL_DST_COLOR:
   case GL_ONE_MINUS_DST_COLOR:
      return ctx->Extensions.NV_blend_square;
   case GL_ZERO:
   case GL_ONE:
   case GL_SRC_COLOR:
   case GL_ONE_MINUS_SRC_COLOR:
   case GL_SRC_ALPHA:
   case GL_ONE_MINUS_SRC_ALPHA:
   case GL_DST_ALPHA:
   case GL_ONE_MINUS_DST_ALPHA:
   case GL_CONSTANT_COLOR:
   case GL_ONE_MINUS_CONSTANT_COLOR:
   case GL_CONSTANT_ALPHA:
   case GL_ONE_MINUS_CONSTANT_ALPHA:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


/**
 * Check if src/dest RGB/A blend factors are legal.  If not generate
 * a GL error.
 * \return GL_TRUE if factors are legal, GL_FALSE otherwise.
 */
static GLboolean
validate_blend_factors(struct gl_context *ctx, const char *func,
                       GLenum srcfactor, GLenum dstfactor)
{
   if (!legal_src_factor(ctx, srcfactor)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "%s(sfactorRGB = %s)", func,
                  _mesa_lookup_enum_by_nr(srcfactor));
      return GL_FALSE;
   }

   if (!legal_dst_factor(ctx, dstfactor)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "%s(dfactorRGB = %s)", func,
                  _mesa_lookup_enum_by_nr(dstfactor));
      return GL_FALSE;
   }

   return GL_TRUE;
}


/**
 * Specify the blending operation.
 *
 * \param sfactor source factor operator.
 * \param dfactor destination factor operator.
 *
 * \sa glBlendFunc
 */
void GLAPIENTRY
_mesa_BlendFunc( GLenum sfactor, GLenum dfactor )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glBlendFunc %s %s\n",
                  _mesa_lookup_enum_by_nr(sfactor),
                  _mesa_lookup_enum_by_nr(dfactor));

   if (!validate_blend_factors(ctx, "glBlendFunc", sfactor, dfactor)) {
      return;
   }

   if (ctx->Color.SrcFactor == sfactor &&
       ctx->Color.DstFactor == dfactor) {
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_COLOR);

   ctx->Color.SrcFactor = sfactor;
   ctx->Color.DstFactor = dfactor;
}


/**
 * Specify the alpha test function.
 *
 * \param func alpha comparison function.
 * \param ref reference value.
 *
 * Verifies the parameters and updates gl_colorbuffer_attrib. 
 * On a change, flushes the vertices and notifies the driver via
 * dd_function_table::AlphaFunc callback.
 */
void GLAPIENTRY
_mesa_AlphaFunc( GLenum func, GLclampf ref )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glAlphaFunc(%s, %f)\n",
                  _mesa_lookup_enum_by_nr(func), ref);

   switch (func) {
   case GL_NEVER:
   case GL_LESS:
   case GL_EQUAL:
   case GL_LEQUAL:
   case GL_GREATER:
   case GL_NOTEQUAL:
   case GL_GEQUAL:
   case GL_ALWAYS:
      if (ctx->Color.AlphaFunc == func && ctx->Color.AlphaRef == ref)
         return; /* no change */

      FLUSH_VERTICES(ctx, _NEW_COLOR);
      ctx->Color.AlphaFunc = func;
      ctx->Color.AlphaRef = ref;

      if (ctx->Driver.AlphaFunc)
         ctx->Driver.AlphaFunc(ctx, func, ctx->Color.AlphaRef);
      return;

   default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glAlphaFunc(func)" );
      return;
   }
}


/**
 * Specify a logic pixel operation for color index rendering.
 *
 * \param opcode operation.
 *
 * Verifies that \p opcode is a valid enum and updates
gl_colorbuffer_attrib::LogicOp.
 * On a change, flushes the vertices and notifies the driver via the
 * dd_function_table::LogicOpcode callback.
 */
void GLAPIENTRY
_mesa_LogicOp( GLenum opcode )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glLogicOp(%s)\n", _mesa_lookup_enum_by_nr(opcode));

   switch (opcode) {
      case GL_CLEAR:
      case GL_SET:
      case GL_COPY:
      case GL_COPY_INVERTED:
      case GL_NOOP:
      case GL_INVERT:
      case GL_AND:
      case GL_NAND:
      case GL_OR:
      case GL_NOR:
      case GL_XOR:
      case GL_EQUIV:
      case GL_AND_REVERSE:
      case GL_AND_INVERTED:
      case GL_OR_REVERSE:
      case GL_OR_INVERTED:
	 break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glLogicOp" );
	 return;
   }

   if (ctx->Color.LogicOp == opcode)
      return;

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   ctx->Color.LogicOp = opcode;

   if (ctx->Driver.LogicOpcode)
      ctx->Driver.LogicOpcode( ctx, opcode );
}

#if _HAVE_FULL_GL
void GLAPIENTRY
_mesa_IndexMask( GLuint mask )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->Color.IndexMask == mask)
      return;

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   ctx->Color.IndexMask = mask;
}
#endif


/**
 * Enable or disable writing of frame buffer color components.
 *
 * \param red whether to mask writing of the red color component.
 * \param green whether to mask writing of the green color component.
 * \param blue whether to mask writing of the blue color component.
 * \param alpha whether to mask writing of the alpha color component.
 *
 * \sa glColorMask().
 *
 * Sets the appropriate value of gl_colorbuffer_attrib::ColorMask.  On a
 * change, flushes the vertices and notifies the driver via the
 * dd_function_table::ColorMask callback.
 */
void GLAPIENTRY
_mesa_ColorMask( GLboolean red, GLboolean green,
                 GLboolean blue, GLboolean alpha )
{
   GET_CURRENT_CONTEXT(ctx);
   GLubyte tmp[4];
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glColorMask(%d, %d, %d, %d)\n",
                  red, green, blue, alpha);

   /* Shouldn't have any information about channel depth in core mesa
    * -- should probably store these as the native booleans:
    */
   tmp[RCOMP] = red    ? 0xff : 0x0;
   tmp[GCOMP] = green  ? 0xff : 0x0;
   tmp[BCOMP] = blue   ? 0xff : 0x0;
   tmp[ACOMP] = alpha  ? 0xff : 0x0;

   if (!TEST_EQ_4V(tmp, ctx->Color.ColorMask)) {
      FLUSH_VERTICES(ctx, _NEW_COLOR);
      COPY_4UBV(ctx->Color.ColorMask, tmp);
   }

   if (ctx->Driver.ColorMask)
      ctx->Driver.ColorMask( ctx, red, green, blue, alpha );
}


/**********************************************************************/
/** \name Initialization */
/*@{*/

/**
 * Initialization of the context's Color attribute group.
 *
 * \param ctx GL context.
 *
 * Initializes the related fields in the context color attribute group,
 * __struct gl_contextRec::Color.
 */
void _mesa_init_color( struct gl_context * ctx )
{
   /* Color buffer group */
   ctx->Color.IndexMask = ~0u;
   memset(ctx->Color.ColorMask, 0xff, sizeof(ctx->Color.ColorMask));
   ctx->Color.ClearIndex = 0;
   ASSIGN_4V( ctx->Color.ClearColor.f, 0, 0, 0, 0 );
   ctx->Color.AlphaEnabled = GL_FALSE;
   ctx->Color.AlphaFunc = GL_ALWAYS;
   ctx->Color.AlphaRef = 0;
   ctx->Color.BlendEnabled = 0x0;
   ctx->Color.SrcFactor = GL_ONE;
   ctx->Color.DstFactor = GL_ZERO;
   ctx->Color.IndexLogicOpEnabled = GL_FALSE;
   ctx->Color.ColorLogicOpEnabled = GL_FALSE;
   ctx->Color.LogicOp = GL_COPY;
   ctx->Color.DitherFlag = GL_TRUE;

   if (ctx->Visual.doubleBufferMode) {
      ctx->Color.DrawBuffer = GL_BACK;
   }
   else {
      ctx->Color.DrawBuffer = GL_FRONT;
   }
}

/*@}*/
