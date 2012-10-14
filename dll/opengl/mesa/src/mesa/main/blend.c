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



#include "glheader.h"
#include "blend.h"
#include "context.h"
#include "enums.h"
#include "macros.h"
#include "mtypes.h"



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
                       GLenum sfactorRGB, GLenum dfactorRGB,
                       GLenum sfactorA, GLenum dfactorA)
{
   if (!legal_src_factor(ctx, sfactorRGB)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "%s(sfactorRGB = %s)", func,
                  _mesa_lookup_enum_by_nr(sfactorRGB));
      return GL_FALSE;
   }

   if (!legal_dst_factor(ctx, dfactorRGB)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "%s(dfactorRGB = %s)", func,
                  _mesa_lookup_enum_by_nr(dfactorRGB));
      return GL_FALSE;
   }

   if (sfactorA != sfactorRGB && !legal_src_factor(ctx, sfactorA)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "%s(sfactorA = %s)", func,
                  _mesa_lookup_enum_by_nr(sfactorA));
      return GL_FALSE;
   }

   if (dfactorA != dfactorRGB && !legal_dst_factor(ctx, dfactorA)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "%s(dfactorA = %s)", func,
                  _mesa_lookup_enum_by_nr(dfactorA));
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
 * \sa glBlendFunc, glBlendFuncSeparateEXT
 */
void GLAPIENTRY
_mesa_BlendFunc( GLenum sfactor, GLenum dfactor )
{
   _mesa_BlendFuncSeparateEXT(sfactor, dfactor, sfactor, dfactor);
}


/**
 * Set the separate blend source/dest factors for all draw buffers.
 *
 * \param sfactorRGB RGB source factor operator.
 * \param dfactorRGB RGB destination factor operator.
 * \param sfactorA alpha source factor operator.
 * \param dfactorA alpha destination factor operator.
 */
void GLAPIENTRY
_mesa_BlendFuncSeparateEXT( GLenum sfactorRGB, GLenum dfactorRGB,
                            GLenum sfactorA, GLenum dfactorA )
{
   GLuint buf, numBuffers;
   GLboolean changed;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glBlendFuncSeparate %s %s %s %s\n",
                  _mesa_lookup_enum_by_nr(sfactorRGB),
                  _mesa_lookup_enum_by_nr(dfactorRGB),
                  _mesa_lookup_enum_by_nr(sfactorA),
                  _mesa_lookup_enum_by_nr(dfactorA));

   if (!validate_blend_factors(ctx, "glBlendFuncSeparate",
                               sfactorRGB, dfactorRGB,
                               sfactorA, dfactorA)) {
      return;
   }

   numBuffers = ctx->Extensions.ARB_draw_buffers_blend
      ? ctx->Const.MaxDrawBuffers : 1;

   changed = GL_FALSE;
   for (buf = 0; buf < numBuffers; buf++) {
      if (ctx->Color.Blend[buf].SrcRGB != sfactorRGB ||
          ctx->Color.Blend[buf].DstRGB != dfactorRGB ||
          ctx->Color.Blend[buf].SrcA != sfactorA ||
          ctx->Color.Blend[buf].DstA != dfactorA) {
         changed = GL_TRUE;
         break;
      }
   }
   if (!changed)
      return;

   FLUSH_VERTICES(ctx, _NEW_COLOR);

   for (buf = 0; buf < numBuffers; buf++) {
      ctx->Color.Blend[buf].SrcRGB = sfactorRGB;
      ctx->Color.Blend[buf].DstRGB = dfactorRGB;
      ctx->Color.Blend[buf].SrcA = sfactorA;
      ctx->Color.Blend[buf].DstA = dfactorA;
   }
   ctx->Color._BlendFuncPerBuffer = GL_FALSE;

   if (ctx->Driver.BlendFuncSeparate) {
      ctx->Driver.BlendFuncSeparate(ctx, sfactorRGB, dfactorRGB,
                                    sfactorA, dfactorA);
   }
}


#if _HAVE_FULL_GL


/**
 * Set blend source/dest factors for one color buffer/target.
 */
void GLAPIENTRY
_mesa_BlendFunci(GLuint buf, GLenum sfactor, GLenum dfactor)
{
   _mesa_BlendFuncSeparatei(buf, sfactor, dfactor, sfactor, dfactor);
}


/**
 * Set separate blend source/dest factors for one color buffer/target.
 */
void GLAPIENTRY
_mesa_BlendFuncSeparatei(GLuint buf, GLenum sfactorRGB, GLenum dfactorRGB,
                         GLenum sfactorA, GLenum dfactorA)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!ctx->Extensions.ARB_draw_buffers_blend) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBlendFunc[Separate]i()");
      return;
   }

   if (buf >= ctx->Const.MaxDrawBuffers) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBlendFuncSeparatei(buffer=%u)",
                  buf);
      return;
   }

   if (!validate_blend_factors(ctx, "glBlendFuncSeparatei",
                               sfactorRGB, dfactorRGB,
                               sfactorA, dfactorA)) {
      return;
   }

   if (ctx->Color.Blend[buf].SrcRGB == sfactorRGB &&
       ctx->Color.Blend[buf].DstRGB == dfactorRGB &&
       ctx->Color.Blend[buf].SrcA == sfactorA &&
       ctx->Color.Blend[buf].DstA == dfactorA)
      return; /* no change */

   FLUSH_VERTICES(ctx, _NEW_COLOR);

   ctx->Color.Blend[buf].SrcRGB = sfactorRGB;
   ctx->Color.Blend[buf].DstRGB = dfactorRGB;
   ctx->Color.Blend[buf].SrcA = sfactorA;
   ctx->Color.Blend[buf].DstA = dfactorA;
   ctx->Color._BlendFuncPerBuffer = GL_TRUE;

   if (ctx->Driver.BlendFuncSeparatei) {
      ctx->Driver.BlendFuncSeparatei(ctx, buf, sfactorRGB, dfactorRGB,
                                     sfactorA, dfactorA);
   }
}


/**
 * Check if given blend equation is legal.
 * \return GL_TRUE if legal, GL_FALSE otherwise.
 */
static GLboolean
legal_blend_equation(const struct gl_context *ctx, GLenum mode)
{
   switch (mode) {
   case GL_FUNC_ADD:
   case GL_FUNC_SUBTRACT:
   case GL_FUNC_REVERSE_SUBTRACT:
      return GL_TRUE;
   case GL_MIN:
   case GL_MAX:
      return ctx->Extensions.EXT_blend_minmax;
   default:
      return GL_FALSE;
   }
}


/* This is really an extension function! */
void GLAPIENTRY
_mesa_BlendEquation( GLenum mode )
{
   GLuint buf, numBuffers;
   GLboolean changed;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glBlendEquation(%s)\n",
                  _mesa_lookup_enum_by_nr(mode));

   if (!legal_blend_equation(ctx, mode)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBlendEquation");
      return;
   }

   numBuffers = ctx->Extensions.ARB_draw_buffers_blend
      ? ctx->Const.MaxDrawBuffers : 1;

   changed = GL_FALSE;
   for (buf = 0; buf < numBuffers; buf++) {
      if (ctx->Color.Blend[buf].EquationRGB != mode ||
          ctx->Color.Blend[buf].EquationA != mode) {
         changed = GL_TRUE;
         break;
      }
   }
   if (!changed)
      return;

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   for (buf = 0; buf < numBuffers; buf++) {
      ctx->Color.Blend[buf].EquationRGB = mode;
      ctx->Color.Blend[buf].EquationA = mode;
   }
   ctx->Color._BlendEquationPerBuffer = GL_FALSE;

   if (ctx->Driver.BlendEquationSeparate)
      (*ctx->Driver.BlendEquationSeparate)( ctx, mode, mode );
}


/**
 * Set blend equation for one color buffer/target.
 */
void GLAPIENTRY
_mesa_BlendEquationi(GLuint buf, GLenum mode)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glBlendEquationi(%u, %s)\n",
                  buf, _mesa_lookup_enum_by_nr(mode));

   if (buf >= ctx->Const.MaxDrawBuffers) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBlendFuncSeparatei(buffer=%u)",
                  buf);
      return;
   }

   if (!legal_blend_equation(ctx, mode)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBlendEquationi");
      return;
   }

   if (ctx->Color.Blend[buf].EquationRGB == mode &&
       ctx->Color.Blend[buf].EquationA == mode)
      return;  /* no change */

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   ctx->Color.Blend[buf].EquationRGB = mode;
   ctx->Color.Blend[buf].EquationA = mode;
   ctx->Color._BlendEquationPerBuffer = GL_TRUE;

   if (ctx->Driver.BlendEquationSeparatei)
      ctx->Driver.BlendEquationSeparatei(ctx, buf, mode, mode);
}


void GLAPIENTRY
_mesa_BlendEquationSeparateEXT( GLenum modeRGB, GLenum modeA )
{
   GLuint buf, numBuffers;
   GLboolean changed;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glBlendEquationSeparateEXT(%s %s)\n",
                  _mesa_lookup_enum_by_nr(modeRGB),
                  _mesa_lookup_enum_by_nr(modeA));

   if ( (modeRGB != modeA) && !ctx->Extensions.EXT_blend_equation_separate ) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
		  "glBlendEquationSeparateEXT not supported by driver");
      return;
   }

   if (!legal_blend_equation(ctx, modeRGB)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBlendEquationSeparateEXT(modeRGB)");
      return;
   }

   if (!legal_blend_equation(ctx, modeA)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBlendEquationSeparateEXT(modeA)");
      return;
   }

   numBuffers = ctx->Extensions.ARB_draw_buffers_blend
      ? ctx->Const.MaxDrawBuffers : 1;

   changed = GL_FALSE;
   for (buf = 0; buf < numBuffers; buf++) {
      if (ctx->Color.Blend[buf].EquationRGB != modeRGB ||
          ctx->Color.Blend[buf].EquationA != modeA) {
         changed = GL_TRUE;
         break;
      }
   }
   if (!changed)
      return;

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   for (buf = 0; buf < numBuffers; buf++) {
      ctx->Color.Blend[buf].EquationRGB = modeRGB;
      ctx->Color.Blend[buf].EquationA = modeA;
   }
   ctx->Color._BlendEquationPerBuffer = GL_FALSE;

   if (ctx->Driver.BlendEquationSeparate)
      ctx->Driver.BlendEquationSeparate(ctx, modeRGB, modeA);
}


/**
 * Set separate blend equations for one color buffer/target.
 */
void GLAPIENTRY
_mesa_BlendEquationSeparatei(GLuint buf, GLenum modeRGB, GLenum modeA)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glBlendEquationSeparatei(%u, %s %s)\n", buf,
                  _mesa_lookup_enum_by_nr(modeRGB),
                  _mesa_lookup_enum_by_nr(modeA));

   if (buf >= ctx->Const.MaxDrawBuffers) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBlendEquationSeparatei(buffer=%u)",
                  buf);
      return;
   }

   if (!legal_blend_equation(ctx, modeRGB)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBlendEquationSeparatei(modeRGB)");
      return;
   }

   if (!legal_blend_equation(ctx, modeA)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBlendEquationSeparatei(modeA)");
      return;
   }

   if (ctx->Color.Blend[buf].EquationRGB == modeRGB &&
       ctx->Color.Blend[buf].EquationA == modeA)
      return;  /* no change */

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   ctx->Color.Blend[buf].EquationRGB = modeRGB;
   ctx->Color.Blend[buf].EquationA = modeA;
   ctx->Color._BlendEquationPerBuffer = GL_TRUE;

   if (ctx->Driver.BlendEquationSeparatei)
      ctx->Driver.BlendEquationSeparatei(ctx, buf, modeRGB, modeA);
}



#endif /* _HAVE_FULL_GL */


/**
 * Set the blending color.
 *
 * \param red red color component.
 * \param green green color component.
 * \param blue blue color component.
 * \param alpha alpha color component.
 *
 * \sa glBlendColor().
 *
 * Clamps the parameters and updates gl_colorbuffer_attrib::BlendColor.  On a
 * change, flushes the vertices and notifies the driver via
 * dd_function_table::BlendColor callback.
 */
void GLAPIENTRY
_mesa_BlendColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha )
{
   GLfloat tmp[4];
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   tmp[0] = red;
   tmp[1] = green;
   tmp[2] = blue;
   tmp[3] = alpha;

   if (TEST_EQ_4V(tmp, ctx->Color.BlendColorUnclamped))
      return;

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   COPY_4FV( ctx->Color.BlendColorUnclamped, tmp );

   ctx->Color.BlendColor[0] = CLAMP(tmp[0], 0.0F, 1.0F);
   ctx->Color.BlendColor[1] = CLAMP(tmp[1], 0.0F, 1.0F);
   ctx->Color.BlendColor[2] = CLAMP(tmp[2], 0.0F, 1.0F);
   ctx->Color.BlendColor[3] = CLAMP(tmp[3], 0.0F, 1.0F);

   if (ctx->Driver.BlendColor)
      (*ctx->Driver.BlendColor)(ctx, ctx->Color.BlendColor);
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
      if (ctx->Color.AlphaFunc == func && ctx->Color.AlphaRefUnclamped == ref)
         return; /* no change */

      FLUSH_VERTICES(ctx, _NEW_COLOR);
      ctx->Color.AlphaFunc = func;
      ctx->Color.AlphaRefUnclamped = ref;
      ctx->Color.AlphaRef = CLAMP(ref, 0.0F, 1.0F);

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
   GLuint i;
   GLboolean flushed;
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

   flushed = GL_FALSE;
   for (i = 0; i < ctx->Const.MaxDrawBuffers; i++) {
      if (!TEST_EQ_4V(tmp, ctx->Color.ColorMask[i])) {
         if (!flushed) {
            FLUSH_VERTICES(ctx, _NEW_COLOR);
         }
         flushed = GL_TRUE;
         COPY_4UBV(ctx->Color.ColorMask[i], tmp);
      }
   }

   if (ctx->Driver.ColorMask)
      ctx->Driver.ColorMask( ctx, red, green, blue, alpha );
}


/**
 * For GL_EXT_draw_buffers2 and GL3
 */
void GLAPIENTRY
_mesa_ColorMaskIndexed( GLuint buf, GLboolean red, GLboolean green,
                        GLboolean blue, GLboolean alpha )
{
   GLubyte tmp[4];
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glColorMaskIndexed %u %d %d %d %d\n",
                  buf, red, green, blue, alpha);

   if (buf >= ctx->Const.MaxDrawBuffers) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glColorMaskIndexed(buf=%u)", buf);
      return;
   }

   /* Shouldn't have any information about channel depth in core mesa
    * -- should probably store these as the native booleans:
    */
   tmp[RCOMP] = red    ? 0xff : 0x0;
   tmp[GCOMP] = green  ? 0xff : 0x0;
   tmp[BCOMP] = blue   ? 0xff : 0x0;
   tmp[ACOMP] = alpha  ? 0xff : 0x0;

   if (TEST_EQ_4V(tmp, ctx->Color.ColorMask[buf]))
      return;

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   COPY_4UBV(ctx->Color.ColorMask[buf], tmp);

   if (ctx->Driver.ColorMaskIndexed)
      ctx->Driver.ColorMaskIndexed(ctx, buf, red, green, blue, alpha);
}


void GLAPIENTRY
_mesa_ClampColorARB(GLenum target, GLenum clamp)
{
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (clamp != GL_TRUE && clamp != GL_FALSE && clamp != GL_FIXED_ONLY_ARB) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glClampColorARB(clamp)");
      return;
   }

   switch (target) {
   case GL_CLAMP_VERTEX_COLOR_ARB:
      FLUSH_VERTICES(ctx, _NEW_LIGHT);
      ctx->Light.ClampVertexColor = clamp;
      break;
   case GL_CLAMP_FRAGMENT_COLOR_ARB:
      FLUSH_VERTICES(ctx, _NEW_FRAG_CLAMP);
      ctx->Color.ClampFragmentColor = clamp;
      break;
   case GL_CLAMP_READ_COLOR_ARB:
      FLUSH_VERTICES(ctx, _NEW_COLOR);
      ctx->Color.ClampReadColor = clamp;
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glClampColorARB(target)");
      return;
   }
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
   GLuint i;

   /* Color buffer group */
   ctx->Color.IndexMask = ~0u;
   memset(ctx->Color.ColorMask, 0xff, sizeof(ctx->Color.ColorMask));
   ctx->Color.ClearIndex = 0;
   ASSIGN_4V( ctx->Color.ClearColor.f, 0, 0, 0, 0 );
   ctx->Color.AlphaEnabled = GL_FALSE;
   ctx->Color.AlphaFunc = GL_ALWAYS;
   ctx->Color.AlphaRef = 0;
   ctx->Color.BlendEnabled = 0x0;
   for (i = 0; i < Elements(ctx->Color.Blend); i++) {
      ctx->Color.Blend[i].SrcRGB = GL_ONE;
      ctx->Color.Blend[i].DstRGB = GL_ZERO;
      ctx->Color.Blend[i].SrcA = GL_ONE;
      ctx->Color.Blend[i].DstA = GL_ZERO;
      ctx->Color.Blend[i].EquationRGB = GL_FUNC_ADD;
      ctx->Color.Blend[i].EquationA = GL_FUNC_ADD;
   }
   ASSIGN_4V( ctx->Color.BlendColor, 0.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( ctx->Color.BlendColorUnclamped, 0.0, 0.0, 0.0, 0.0 );
   ctx->Color.IndexLogicOpEnabled = GL_FALSE;
   ctx->Color.ColorLogicOpEnabled = GL_FALSE;
   ctx->Color.LogicOp = GL_COPY;
   ctx->Color.DitherFlag = GL_TRUE;

   if (ctx->Visual.doubleBufferMode) {
      ctx->Color.DrawBuffer[0] = GL_BACK;
   }
   else {
      ctx->Color.DrawBuffer[0] = GL_FRONT;
   }

   ctx->Color.ClampFragmentColor = GL_FIXED_ONLY_ARB;
   ctx->Color._ClampFragmentColor = GL_TRUE;
   ctx->Color.ClampReadColor = GL_FIXED_ONLY_ARB;
   ctx->Color._ClampReadColor = GL_TRUE;
}

/*@}*/
