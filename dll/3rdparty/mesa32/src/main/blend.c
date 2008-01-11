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
#include "colormac.h"
#include "context.h"
#include "enums.h"
#include "macros.h"
#include "mtypes.h"


/**
 * Specify the blending operation.
 *
 * \param sfactor source factor operator.
 * \param dfactor destination factor operator.
 *
 * \sa glBlendFunc, glBlendFuncSeparateEXT
 *
 * Swizzles the inputs and calls \c glBlendFuncSeparateEXT.  This is done
 * using the \c CurrentDispatch table in the context, so this same function
 * can be used while compiling display lists.  Therefore, there is no need
 * for the display list code to save and restore this function.
 */
void GLAPIENTRY
_mesa_BlendFunc( GLenum sfactor, GLenum dfactor )
{
   GET_CURRENT_CONTEXT(ctx);

   (*ctx->CurrentDispatch->BlendFuncSeparateEXT)( sfactor, dfactor,
						  sfactor, dfactor );
}


/**
 * Process GL_EXT_blend_func_separate().
 *
 * \param sfactorRGB RGB source factor operator.
 * \param dfactorRGB RGB destination factor operator.
 * \param sfactorA alpha source factor operator.
 * \param dfactorA alpha destination factor operator.
 *
 * Verifies the parameters and updates gl_colorbuffer_attrib.
 * On a change, flush the vertices and notify the driver via
 * dd_function_table::BlendFuncSeparate.
 */
void GLAPIENTRY
_mesa_BlendFuncSeparateEXT( GLenum sfactorRGB, GLenum dfactorRGB,
                            GLenum sfactorA, GLenum dfactorA )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glBlendFuncSeparate %s %s %s %s\n",
                  _mesa_lookup_enum_by_nr(sfactorRGB),
                  _mesa_lookup_enum_by_nr(dfactorRGB),
                  _mesa_lookup_enum_by_nr(sfactorA),
                  _mesa_lookup_enum_by_nr(dfactorA));

   switch (sfactorRGB) {
      case GL_SRC_COLOR:
      case GL_ONE_MINUS_SRC_COLOR:
         if (!ctx->Extensions.NV_blend_square) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glBlendFunc or glBlendFuncSeparate (sfactorRGB)");
            return;
         }
         /* fall-through */
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
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glBlendFunc or glBlendFuncSeparate (sfactorRGB)");
         return;
   }

   switch (dfactorRGB) {
      case GL_DST_COLOR:
      case GL_ONE_MINUS_DST_COLOR:
         if (!ctx->Extensions.NV_blend_square) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glBlendFunc or glBlendFuncSeparate (dfactorRGB)");
            return;
         }
         /* fall-through */
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
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glBlendFunc or glBlendFuncSeparate (dfactorRGB)");
         return;
   }

   switch (sfactorA) {
      case GL_SRC_COLOR:
      case GL_ONE_MINUS_SRC_COLOR:
         if (!ctx->Extensions.NV_blend_square) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glBlendFunc or glBlendFuncSeparate (sfactorA)");
            return;
         }
         /* fall-through */
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
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glBlendFunc or glBlendFuncSeparate (sfactorA)");
         return;
   }

   switch (dfactorA) {
      case GL_DST_COLOR:
      case GL_ONE_MINUS_DST_COLOR:
         if (!ctx->Extensions.NV_blend_square) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glBlendFunc or glBlendFuncSeparate (dfactorA)");
            return;
         }
         /* fall-through */
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
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glBlendFunc or glBlendFuncSeparate (dfactorA)" );
         return;
   }

   if (ctx->Color.BlendSrcRGB == sfactorRGB &&
       ctx->Color.BlendDstRGB == dfactorRGB &&
       ctx->Color.BlendSrcA == sfactorA &&
       ctx->Color.BlendDstA == dfactorA)
      return;

   FLUSH_VERTICES(ctx, _NEW_COLOR);

   ctx->Color.BlendSrcRGB = sfactorRGB;
   ctx->Color.BlendDstRGB = dfactorRGB;
   ctx->Color.BlendSrcA = sfactorA;
   ctx->Color.BlendDstA = dfactorA;

   if (ctx->Driver.BlendFuncSeparate) {
      (*ctx->Driver.BlendFuncSeparate)( ctx, sfactorRGB, dfactorRGB,
					sfactorA, dfactorA );
   }
}


#if _HAVE_FULL_GL

static GLboolean
_mesa_validate_blend_equation( GLcontext *ctx,
			       GLenum mode, GLboolean is_separate )
{
   switch (mode) {
      case GL_FUNC_ADD:
         break;
      case GL_MIN:
      case GL_MAX:
         if (!ctx->Extensions.EXT_blend_minmax &&
             !ctx->Extensions.ARB_imaging) {
            return GL_FALSE;
         }
         break;
      /* glBlendEquationSeparate cannot take GL_LOGIC_OP as a parameter.
       */
      case GL_LOGIC_OP:
         if (!ctx->Extensions.EXT_blend_logic_op || is_separate) {
            return GL_FALSE;
         }
         break;
      case GL_FUNC_SUBTRACT:
      case GL_FUNC_REVERSE_SUBTRACT:
         if (!ctx->Extensions.EXT_blend_subtract &&
             !ctx->Extensions.ARB_imaging) {
            return GL_FALSE;
         }
         break;
      default:
         return GL_FALSE;
   }

   return GL_TRUE;
}


/* This is really an extension function! */
void GLAPIENTRY
_mesa_BlendEquation( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glBlendEquation %s\n",
                  _mesa_lookup_enum_by_nr(mode));

   if ( ! _mesa_validate_blend_equation( ctx, mode, GL_FALSE ) ) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBlendEquation");
      return;
   }

   if ( (ctx->Color.BlendEquationRGB == mode) &&
	(ctx->Color.BlendEquationA == mode) )
      return;

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   ctx->Color.BlendEquationRGB = mode;
   ctx->Color.BlendEquationA = mode;

   if (ctx->Driver.BlendEquationSeparate)
      (*ctx->Driver.BlendEquationSeparate)( ctx, mode, mode );
}


void GLAPIENTRY
_mesa_BlendEquationSeparateEXT( GLenum modeRGB, GLenum modeA )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glBlendEquationSeparateEXT %s %s\n",
                  _mesa_lookup_enum_by_nr(modeRGB),
                  _mesa_lookup_enum_by_nr(modeA));

   if ( (modeRGB != modeA) && !ctx->Extensions.EXT_blend_equation_separate ) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
		  "glBlendEquationSeparateEXT not supported by driver");
      return;
   }

   if ( ! _mesa_validate_blend_equation( ctx, modeRGB, GL_TRUE ) ) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBlendEquationSeparateEXT(modeRGB)");
      return;
   }

   if ( ! _mesa_validate_blend_equation( ctx, modeA, GL_TRUE ) ) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBlendEquationSeparateEXT(modeA)");
      return;
   }


   if ( (ctx->Color.BlendEquationRGB == modeRGB) &&
	(ctx->Color.BlendEquationA == modeA) )
      return;

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   ctx->Color.BlendEquationRGB = modeRGB;
   ctx->Color.BlendEquationA = modeA;

   if (ctx->Driver.BlendEquationSeparate)
      (*ctx->Driver.BlendEquationSeparate)( ctx, modeRGB, modeA );
}
#endif


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

   tmp[0] = CLAMP( red,   0.0F, 1.0F );
   tmp[1] = CLAMP( green, 0.0F, 1.0F );
   tmp[2] = CLAMP( blue,  0.0F, 1.0F );
   tmp[3] = CLAMP( alpha, 0.0F, 1.0F );

   if (TEST_EQ_4V(tmp, ctx->Color.BlendColor))
      return;

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   COPY_4FV( ctx->Color.BlendColor, tmp );

   if (ctx->Driver.BlendColor)
      (*ctx->Driver.BlendColor)(ctx, tmp);
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

   switch (func) {
   case GL_NEVER:
   case GL_LESS:
   case GL_EQUAL:
   case GL_LEQUAL:
   case GL_GREATER:
   case GL_NOTEQUAL:
   case GL_GEQUAL:
   case GL_ALWAYS:
      ref = CLAMP(ref, 0.0F, 1.0F);

      if (ctx->Color.AlphaFunc == func && ctx->Color.AlphaRef == ref)
         return; /* no change */

      FLUSH_VERTICES(ctx, _NEW_COLOR);
      ctx->Color.AlphaFunc = func;
      ctx->Color.AlphaRef = ref;

      if (ctx->Driver.AlphaFunc)
         ctx->Driver.AlphaFunc(ctx, func, ref);
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

   if (ctx->Driver.IndexMask)
      ctx->Driver.IndexMask( ctx, mask );
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
      _mesa_debug(ctx, "glColorMask %d %d %d %d\n", red, green, blue, alpha);

   /* Shouldn't have any information about channel depth in core mesa
    * -- should probably store these as the native booleans:
    */
   tmp[RCOMP] = red    ? 0xff : 0x0;
   tmp[GCOMP] = green  ? 0xff : 0x0;
   tmp[BCOMP] = blue   ? 0xff : 0x0;
   tmp[ACOMP] = alpha  ? 0xff : 0x0;

   if (TEST_EQ_4UBV(tmp, ctx->Color.ColorMask))
      return;

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   COPY_4UBV(ctx->Color.ColorMask, tmp);

   if (ctx->Driver.ColorMask)
      ctx->Driver.ColorMask( ctx, red, green, blue, alpha );
}


extern void GLAPIENTRY
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
      ctx->Light.ClampVertexColor = clamp;
      break;
   case GL_CLAMP_FRAGMENT_COLOR_ARB:
      ctx->Color.ClampFragmentColor = clamp;
      break;
   case GL_CLAMP_READ_COLOR_ARB:
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
 * __GLcontextRec::Color.
 */
void _mesa_init_color( GLcontext * ctx )
{
   /* Color buffer group */
   ctx->Color.IndexMask = ~0u;
   ctx->Color.ColorMask[0] = 0xff;
   ctx->Color.ColorMask[1] = 0xff;
   ctx->Color.ColorMask[2] = 0xff;
   ctx->Color.ColorMask[3] = 0xff;
   ctx->Color.ClearIndex = 0;
   ASSIGN_4V( ctx->Color.ClearColor, 0, 0, 0, 0 );
   ctx->Color.AlphaEnabled = GL_FALSE;
   ctx->Color.AlphaFunc = GL_ALWAYS;
   ctx->Color.AlphaRef = 0;
   ctx->Color.BlendEnabled = GL_FALSE;
   ctx->Color.BlendSrcRGB = GL_ONE;
   ctx->Color.BlendDstRGB = GL_ZERO;
   ctx->Color.BlendSrcA = GL_ONE;
   ctx->Color.BlendDstA = GL_ZERO;
   ctx->Color.BlendEquationRGB = GL_FUNC_ADD;
   ctx->Color.BlendEquationA = GL_FUNC_ADD;
   ASSIGN_4V( ctx->Color.BlendColor, 0.0, 0.0, 0.0, 0.0 );
   ctx->Color.IndexLogicOpEnabled = GL_FALSE;
   ctx->Color.ColorLogicOpEnabled = GL_FALSE;
   ctx->Color._LogicOpEnabled = GL_FALSE;
   ctx->Color.LogicOp = GL_COPY;
   ctx->Color.DitherFlag = GL_TRUE;

   if (ctx->Visual.doubleBufferMode) {
      ctx->Color.DrawBuffer[0] = GL_BACK;
   }
   else {
      ctx->Color.DrawBuffer[0] = GL_FRONT;
   }

   ctx->Color.ClampFragmentColor = GL_FIXED_ONLY_ARB;
   ctx->Color.ClampReadColor = GL_FIXED_ONLY_ARB;
}

/*@}*/
