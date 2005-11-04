/**************************************************************************

Copyright 2001 2d3d Inc., Delray Beach, FL

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/* $XFree86: xc/lib/GL/mesa/src/drv/i830/i830_state.c,v 1.6 2003/01/28 22:47:06 dawes Exp $ */

/*
 * Author:
 *   Jeff Hartmann <jhartmann@2d3d.com>
 *
 * Heavily based on the I810 driver, which was written by:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "buffers.h"
#include "context.h"
#include "macros.h"
#include "enums.h"
#include "dd.h"

#include "texmem.h"

#include "i830_screen.h"
#include "i830_dri.h"

#include "i830_context.h"
#include "i830_state.h"
#include "i830_tex.h"
#include "i830_tris.h"
#include "i830_ioctl.h"

#include "swrast/swrast.h"
#include "array_cache/acache.h"
#include "tnl/tnl.h"
#include "swrast_setup/swrast_setup.h"

#include "tnl/t_pipeline.h"

static __inline__ GLuint i830PackColor(GLuint format, 
				       GLubyte r, GLubyte g, 
				       GLubyte b, GLubyte a)
{

   if (I830_DEBUG&DEBUG_DRI)
      fprintf(stderr, "%s\n", __FUNCTION__);

   switch (format) {
   case DV_PF_555:
      return I830PACKCOLOR1555(r,g,b,a);
   case DV_PF_565:
      return I830PACKCOLOR565(r,g,b);
   case DV_PF_8888:
      return I830PACKCOLOR8888(r,g,b,a);
   default:
      fprintf(stderr, "unknown format %d\n", (int)format);
      return 0;
   }
}

static void i830StencilFunc(GLcontext *ctx, GLenum func, GLint ref,
			    GLuint mask)
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   int test = 0;

   mask = mask & 0xff;

   if (I830_DEBUG&DEBUG_DRI)
      fprintf(stderr, "%s : func: %s, ref : 0x%x, mask: 0x%x\n", __FUNCTION__,
	      _mesa_lookup_enum_by_nr(func), ref, mask);

   switch(func) {
   case GL_NEVER: 
      test = COMPAREFUNC_NEVER; 
      break;
   case GL_LESS: 
      test = COMPAREFUNC_LESS; 
      break;
   case GL_LEQUAL: 
      test = COMPAREFUNC_LEQUAL; 
      break;
   case GL_GREATER: 
      test = COMPAREFUNC_GREATER; 
      break;
   case GL_GEQUAL: 
      test = COMPAREFUNC_GEQUAL; 
      break;
   case GL_NOTEQUAL: 
      test = COMPAREFUNC_NOTEQUAL; 
      break;
   case GL_EQUAL: 
      test = COMPAREFUNC_EQUAL; 
      break;
   case GL_ALWAYS: 
      test = COMPAREFUNC_ALWAYS; 
      break;
   default:
      return;
   }

   I830_STATECHANGE(imesa, I830_UPLOAD_CTX);
   imesa->Setup[I830_CTXREG_STATE4] &= ~MODE4_ENABLE_STENCIL_TEST_MASK;
   imesa->Setup[I830_CTXREG_STATE4] |= (ENABLE_STENCIL_TEST_MASK |
					STENCIL_TEST_MASK(mask));
   imesa->Setup[I830_CTXREG_STENCILTST] &= ~(STENCIL_REF_VALUE_MASK |
					     ENABLE_STENCIL_TEST_FUNC_MASK);
   imesa->Setup[I830_CTXREG_STENCILTST] |= (ENABLE_STENCIL_REF_VALUE |
					    ENABLE_STENCIL_TEST_FUNC |
					    STENCIL_REF_VALUE(ref) |
					    STENCIL_TEST_FUNC(test));
}

static void i830StencilMask(GLcontext *ctx, GLuint mask)
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);

   if (I830_DEBUG&DEBUG_DRI)
      fprintf(stderr, "%s : mask 0x%x\n", __FUNCTION__, mask);

   mask = mask & 0xff;

   I830_STATECHANGE(imesa, I830_UPLOAD_CTX);
   imesa->Setup[I830_CTXREG_STATE4] &= ~MODE4_ENABLE_STENCIL_WRITE_MASK;
   imesa->Setup[I830_CTXREG_STATE4] |= (ENABLE_STENCIL_WRITE_MASK |
					STENCIL_WRITE_MASK(mask));
}

static void i830StencilOp(GLcontext *ctx, GLenum fail, GLenum zfail,
			  GLenum zpass)
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   int fop, dfop, dpop;

   if (I830_DEBUG&DEBUG_DRI)
      fprintf(stderr, "%s: fail : %s, zfail: %s, zpass : %s\n", __FUNCTION__,
	      _mesa_lookup_enum_by_nr(fail),
	      _mesa_lookup_enum_by_nr(zfail),
	      _mesa_lookup_enum_by_nr(zpass));

   fop = 0; dfop = 0; dpop = 0;

   switch(fail) {
   case GL_KEEP: 
      fop = STENCILOP_KEEP; 
      break;
   case GL_ZERO: 
      fop = STENCILOP_ZERO; 
      break;
   case GL_REPLACE: 
      fop = STENCILOP_REPLACE; 
      break;
   case GL_INCR: 
      fop = STENCILOP_INCRSAT;
      break;
   case GL_DECR: 
      fop = STENCILOP_DECRSAT;
      break;
   case GL_INCR_WRAP:
      fop = STENCILOP_INCR; 
      break;
   case GL_DECR_WRAP:
      fop = STENCILOP_DECR; 
      break;
   case GL_INVERT: 
      fop = STENCILOP_INVERT; 
      break;
   default: 
      break;
   }
   switch(zfail) {
   case GL_KEEP: 
      dfop = STENCILOP_KEEP; 
      break;
   case GL_ZERO: 
      dfop = STENCILOP_ZERO; 
      break;
   case GL_REPLACE: 
      dfop = STENCILOP_REPLACE; 
      break;
   case GL_INCR: 
      dfop = STENCILOP_INCRSAT;
      break;
   case GL_DECR: 
      dfop = STENCILOP_DECRSAT;
      break;
   case GL_INCR_WRAP:
      dfop = STENCILOP_INCR; 
      break;
   case GL_DECR_WRAP:
      dfop = STENCILOP_DECR; 
      break;
   case GL_INVERT: 
      dfop = STENCILOP_INVERT; 
      break;
   default: 
      break;
   }
   switch(zpass) {
   case GL_KEEP: 
      dpop = STENCILOP_KEEP; 
      break;
   case GL_ZERO: 
      dpop = STENCILOP_ZERO; 
      break;
   case GL_REPLACE: 
      dpop = STENCILOP_REPLACE; 
      break;
   case GL_INCR: 
      dpop = STENCILOP_INCRSAT;
      break;
   case GL_DECR: 
      dpop = STENCILOP_DECRSAT;
      break;
   case GL_INCR_WRAP:
      dpop = STENCILOP_INCR; 
      break;
   case GL_DECR_WRAP:
      dpop = STENCILOP_DECR; 
      break;
   case GL_INVERT: 
      dpop = STENCILOP_INVERT; 
      break;
   default: 
      break;
   }


   I830_STATECHANGE(imesa, I830_UPLOAD_CTX);
   imesa->Setup[I830_CTXREG_STENCILTST] &= ~(STENCIL_OPS_MASK);
   imesa->Setup[I830_CTXREG_STENCILTST] |= (ENABLE_STENCIL_PARMS |
					    STENCIL_FAIL_OP(fop) |
					    STENCIL_PASS_DEPTH_FAIL_OP(dfop) |
					    STENCIL_PASS_DEPTH_PASS_OP(dpop));
}

static void i830AlphaFunc(GLcontext *ctx, GLenum func, GLfloat ref)
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   int test = 0;
   GLuint refByte = (GLint) (ref * 255.0);

   switch(func) {
   case GL_NEVER: 
      test = COMPAREFUNC_NEVER; 
      break;
   case GL_LESS: 
      test = COMPAREFUNC_LESS; 
      break;
   case GL_LEQUAL: 
      test = COMPAREFUNC_LEQUAL; 
      break;
   case GL_GREATER: 
      test = COMPAREFUNC_GREATER; 
      break;
   case GL_GEQUAL: 
      test = COMPAREFUNC_GEQUAL; 
      break;
   case GL_NOTEQUAL: 
      test = COMPAREFUNC_NOTEQUAL; 
      break;
   case GL_EQUAL: 
      test = COMPAREFUNC_EQUAL; 
      break;
   case GL_ALWAYS: 
      test = COMPAREFUNC_ALWAYS; 
      break;
   default:
      return;
   }

   I830_STATECHANGE(imesa, I830_UPLOAD_CTX);
   imesa->Setup[I830_CTXREG_STATE2] &= ~ALPHA_TEST_REF_MASK;
   imesa->Setup[I830_CTXREG_STATE2] |= (ENABLE_ALPHA_TEST_FUNC |
					ENABLE_ALPHA_REF_VALUE |
					ALPHA_TEST_FUNC(test) |
					ALPHA_REF_VALUE(refByte));
}

/* This function makes sure that the proper enables are
 * set for LogicOp, Independant Alpha Blend, and Blending.
 * It needs to be called from numerous places where we
 * could change the LogicOp or Independant Alpha Blend without subsequent
 * calls to glEnable.
 */
static void i830EvalLogicOpBlendState(GLcontext *ctx)
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);

   I830_STATECHANGE(imesa, I830_UPLOAD_CTX);

   imesa->Setup[I830_CTXREG_ENABLES_1] &= ~(ENABLE_COLOR_BLEND |
					    ENABLE_LOGIC_OP_MASK);
   imesa->Setup[I830_CTXREG_IALPHAB] &= ~ENABLE_INDPT_ALPHA_BLEND;

   if (ctx->Color.ColorLogicOpEnabled) {
      imesa->Setup[I830_CTXREG_ENABLES_1] |= (DISABLE_COLOR_BLEND |
					      ENABLE_LOGIC_OP);
      imesa->Setup[I830_CTXREG_IALPHAB] |= DISABLE_INDPT_ALPHA_BLEND;
   } else if (ctx->Color.BlendEnabled) {
      imesa->Setup[I830_CTXREG_ENABLES_1] |= (ENABLE_COLOR_BLEND |
					      DISABLE_LOGIC_OP);

      /* If the alpha blend state does not match the color blend state,
       * enable independent alpha blending.  Otherwise, leave it disabled
       * and the hardware will use the color blend state for both.
       */

      if ( 0 && (imesa->Setup[I830_CTXREG_IALPHAB] & BLEND_STATE_MASK)
	   != (imesa->Setup[I830_CTXREG_STATE1] & BLEND_STATE_MASK) ) {
	 imesa->Setup[I830_CTXREG_IALPHAB] |= ENABLE_INDPT_ALPHA_BLEND;
      } else {
	 imesa->Setup[I830_CTXREG_IALPHAB] |= DISABLE_INDPT_ALPHA_BLEND;
      }
   } else {
      imesa->Setup[I830_CTXREG_ENABLES_1] |= (DISABLE_COLOR_BLEND |
					      DISABLE_LOGIC_OP);
      imesa->Setup[I830_CTXREG_IALPHAB] |= DISABLE_INDPT_ALPHA_BLEND;
   }
}

static void i830BlendColor(GLcontext *ctx, const GLfloat color[4])
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   GLubyte r, g, b, a;

   if (I830_DEBUG&DEBUG_DRI)
      fprintf(stderr, "%s\n", __FUNCTION__);

   FLOAT_COLOR_TO_UBYTE_COLOR(r, color[RCOMP]);
   FLOAT_COLOR_TO_UBYTE_COLOR(g, color[GCOMP]);
   FLOAT_COLOR_TO_UBYTE_COLOR(b, color[BCOMP]);
   FLOAT_COLOR_TO_UBYTE_COLOR(a, color[ACOMP]);

   I830_STATECHANGE(imesa, I830_UPLOAD_CTX);
   imesa->Setup[I830_CTXREG_BLENDCOLR] = ((a << 24) |
					  (r << 16) |
					  (g << 8) |
					  b);
}

/**
 * Calculate the hardware blend factor setting.  This same function is used
 * for source and destination of both alpha and RGB.  
 *
 * \returns
 * The hardware register value for the specified blend factor.  This value
 * will need to be shifted into the correct position for either source or
 * destination factor.
 *
 * \todo
 * Since the two cases where source and destination are handled differently
 * are essentially error cases, they should never happen.  Determine if these
 * cases can be removed.
 */
static int blend_factor( GLenum factor, GLboolean is_src )
{
   int func;

   switch( factor ) {
   case GL_ZERO:
      func = BLENDFACT_ZERO;
      break;
   case GL_ONE: 
      func = BLENDFACT_ONE;
      break;
   case GL_SRC_COLOR:
      func = BLENDFACT_SRC_COLR;
      break;
   case GL_ONE_MINUS_SRC_COLOR:
      func = BLENDFACT_INV_SRC_COLR;
      break;
   case GL_SRC_ALPHA: 
      func = BLENDFACT_SRC_ALPHA;
      break;
   case GL_ONE_MINUS_SRC_ALPHA:
      func = BLENDFACT_INV_SRC_ALPHA;
      break;
   case GL_DST_ALPHA: 
      func = BLENDFACT_DST_ALPHA;
      break;
   case GL_ONE_MINUS_DST_ALPHA:
      func = BLENDFACT_INV_DST_ALPHA;
      break;
   case GL_DST_COLOR: 
      func = BLENDFACT_DST_COLR;
      break;
   case GL_ONE_MINUS_DST_COLOR: 
      func = BLENDFACT_INV_DST_COLR;
      break;
   case GL_SRC_ALPHA_SATURATE: 
      func = (is_src) ? BLENDFACT_SRC_ALPHA_SATURATE : BLENDFACT_ZERO;
      break;
   case GL_CONSTANT_COLOR:
      func = BLENDFACT_CONST_COLOR;
      break;
   case GL_ONE_MINUS_CONSTANT_COLOR:
      func = BLENDFACT_INV_CONST_COLOR;
      break;
   case GL_CONSTANT_ALPHA:
      func = BLENDFACT_CONST_ALPHA;
      break;
   case GL_ONE_MINUS_CONSTANT_ALPHA:
      func = BLENDFACT_INV_CONST_ALPHA;
      break;
   default:
      func = (is_src) ? BLENDFACT_ONE : BLENDFACT_ZERO;
   }

   return func;
}


/**
 * Sets both the blend equation (called "function" in i830 docs) and the
 * blend function (called "factor" in i830 docs).  This is done in a single
 * function because some blend equations (i.e., \c GL_MIN and \c GL_MAX)
 * change the interpretation of the blend function.
 */

static void i830_set_blend_state( GLcontext * ctx )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   int funcA;
   int funcRGB;
   int eqnA;
   int eqnRGB;


   funcRGB = SRC_BLND_FACT( blend_factor( ctx->Color.BlendSrcRGB, GL_TRUE ) )
       | DST_BLND_FACT( blend_factor( ctx->Color.BlendDstRGB, GL_FALSE ) );

   switch(ctx->Color.BlendEquationRGB) {
   case GL_FUNC_ADD:
      eqnRGB = BLENDFUNC_ADD; 
      break;
   case GL_MIN:
      eqnRGB = BLENDFUNC_MIN;
      funcRGB = SRC_BLND_FACT(BLENDFACT_ONE) | DST_BLND_FACT(BLENDFACT_ONE);
      break;
   case GL_MAX: 
      eqnRGB = BLENDFUNC_MAX;
      funcRGB = SRC_BLND_FACT(BLENDFACT_ONE) | DST_BLND_FACT(BLENDFACT_ONE);
      break;
   case GL_FUNC_SUBTRACT: 
      eqnRGB = BLENDFUNC_SUB; 
      break;
   case GL_FUNC_REVERSE_SUBTRACT:
      eqnRGB = BLENDFUNC_RVRSE_SUB; 
      break;
   default:
      fprintf( stderr, "[%s:%u] Invalid RGB blend equation (0x%04x).\n",
	       __func__, __LINE__, ctx->Color.BlendEquationRGB );
      return;
   }


   funcA = SRC_ABLEND_FACT( blend_factor( ctx->Color.BlendSrcA, GL_TRUE ) )
       | DST_ABLEND_FACT( blend_factor( ctx->Color.BlendDstA, GL_FALSE ) );

   switch(ctx->Color.BlendEquationA) {
   case GL_FUNC_ADD:
      eqnA = BLENDFUNC_ADD; 
      break;
   case GL_MIN: 
      eqnA = BLENDFUNC_MIN;
      funcA = SRC_BLND_FACT(BLENDFACT_ONE) | DST_BLND_FACT(BLENDFACT_ONE);
      break;
   case GL_MAX: 
      eqnA = BLENDFUNC_MAX;
      funcA = SRC_BLND_FACT(BLENDFACT_ONE) | DST_BLND_FACT(BLENDFACT_ONE);
      break;
   case GL_FUNC_SUBTRACT: 
      eqnA = BLENDFUNC_SUB; 
      break;
   case GL_FUNC_REVERSE_SUBTRACT:
      eqnA = BLENDFUNC_RVRSE_SUB; 
      break;
   default:
      fprintf( stderr, "[%s:%u] Invalid alpha blend equation (0x%04x).\n",
	       __func__, __LINE__, ctx->Color.BlendEquationA );
      return;
   }

   I830_STATECHANGE(imesa, I830_UPLOAD_CTX);

   imesa->Setup[I830_CTXREG_STATE1] = eqnRGB | funcRGB
       | STATE3D_MODES_1_CMD
       | ENABLE_SRC_BLND_FACTOR | ENABLE_DST_BLND_FACTOR
       | ENABLE_COLR_BLND_FUNC;

   imesa->Setup[I830_CTXREG_IALPHAB] = eqnA | funcA
       | STATE3D_INDPT_ALPHA_BLEND_CMD
       | ENABLE_SRC_ABLEND_FACTOR | ENABLE_DST_ABLEND_FACTOR
       | ENABLE_ALPHA_BLENDFUNC;


   /* This will catch a logicop blend equation.  It will also ensure
    * independant alpha blend is really in the correct state (either enabled
    * or disabled) if blending is already enabled.
    */

   i830EvalLogicOpBlendState(ctx);

   if (0) {
      fprintf(stderr, "[%s:%u] STATE1: 0x%08x IALPHAB: 0x%08x blend is %sabled\n",
	      __func__, __LINE__,
	      imesa->Setup[I830_CTXREG_STATE1],
	      imesa->Setup[I830_CTXREG_IALPHAB],
	      (ctx->Color.BlendEnabled) ? "en" : "dis");
   }
}

static void i830BlendEquationSeparate(GLcontext *ctx,
				      GLenum modeRGB, GLenum modeA)
{
   if (I830_DEBUG&DEBUG_DRI)
     fprintf(stderr, "%s -> %s, %s\n", __FUNCTION__,
	     _mesa_lookup_enum_by_nr(modeRGB),
	     _mesa_lookup_enum_by_nr(modeA));

   (void) modeRGB;
   (void) modeA;
   i830_set_blend_state( ctx );
}



static void i830BlendFuncSeparate(GLcontext *ctx, GLenum sfactorRGB, 
				  GLenum dfactorRGB, GLenum sfactorA,
				  GLenum dfactorA )
{
   if (I830_DEBUG&DEBUG_DRI)
     fprintf(stderr, "%s -> RGB(%s, %s) A(%s, %s)\n", __FUNCTION__,
	     _mesa_lookup_enum_by_nr(sfactorRGB),
	     _mesa_lookup_enum_by_nr(dfactorRGB),
	     _mesa_lookup_enum_by_nr(sfactorA),
	     _mesa_lookup_enum_by_nr(dfactorA));

   (void) sfactorRGB;
   (void) dfactorRGB;
   (void) sfactorA;
   (void) dfactorA;
   i830_set_blend_state( ctx );
}

static void i830DepthFunc(GLcontext *ctx, GLenum func)
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   int test = 0;

   if (I830_DEBUG&DEBUG_DRI)
      fprintf(stderr, "%s\n", __FUNCTION__);

   switch(func) {
   case GL_NEVER: 
      test = COMPAREFUNC_NEVER; 
      break;
   case GL_LESS: 
      test = COMPAREFUNC_LESS; 
      break;
   case GL_LEQUAL: 
      test = COMPAREFUNC_LEQUAL; 
      break;
   case GL_GREATER: 
      test = COMPAREFUNC_GREATER; 
      break;
   case GL_GEQUAL: 
      test = COMPAREFUNC_GEQUAL; 
      break;
   case GL_NOTEQUAL: 
      test = COMPAREFUNC_NOTEQUAL; 
      break;
   case GL_EQUAL: 
      test = COMPAREFUNC_EQUAL; 
      break;
   case GL_ALWAYS: 
      test = COMPAREFUNC_ALWAYS; 
      break;
   default: return;
   }

   I830_STATECHANGE(imesa, I830_UPLOAD_CTX);
   imesa->Setup[I830_CTXREG_STATE3] &= ~DEPTH_TEST_FUNC_MASK;
   imesa->Setup[I830_CTXREG_STATE3] |= (ENABLE_DEPTH_TEST_FUNC |
				       DEPTH_TEST_FUNC(test));
}

static void i830DepthMask(GLcontext *ctx, GLboolean flag)
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);

   if (I830_DEBUG&DEBUG_DRI)
      fprintf(stderr, "%s flag (%d)\n", __FUNCTION__, flag);

   I830_STATECHANGE(imesa, I830_UPLOAD_CTX);

   imesa->Setup[I830_CTXREG_ENABLES_2] &= ~ENABLE_DIS_DEPTH_WRITE_MASK;

   if (flag && ctx->Depth.Test)
      imesa->Setup[I830_CTXREG_ENABLES_2] |= ENABLE_DEPTH_WRITE;
   else
      imesa->Setup[I830_CTXREG_ENABLES_2] |= DISABLE_DEPTH_WRITE;
}

/* =============================================================
 * Polygon stipple
 *
 * The i830 supports a 4x4 stipple natively, GL wants 32x32.
 * Fortunately stipple is usually a repeating pattern.
 */
static void i830PolygonStipple( GLcontext *ctx, const GLubyte *mask )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   const GLubyte *m = mask;
   GLubyte p[4];
   int i,j,k;
   int active = (ctx->Polygon.StippleFlag &&
		 imesa->reduced_primitive == GL_TRIANGLES);
   GLuint newMask;

   if (active) {
      I830_STATECHANGE(imesa, I830_UPLOAD_STIPPLE);
      imesa->StippleSetup[I830_STPREG_ST1] &= ~ST1_ENABLE;
   }

   p[0] = mask[12] & 0xf; p[0] |= p[0] << 4;
   p[1] = mask[8] & 0xf; p[1] |= p[1] << 4;
   p[2] = mask[4] & 0xf; p[2] |= p[2] << 4;
   p[3] = mask[0] & 0xf; p[3] |= p[3] << 4;

   for (k = 0 ; k < 8 ; k++)
      for (j = 3 ; j >= 0; j--)
	 for (i = 0 ; i < 4 ; i++, m++)
	    if (*m != p[j]) {
	       imesa->hw_stipple = 0;
	       return;
	    }

   newMask = (((p[0] & 0xf) << 0) |
	      ((p[1] & 0xf) << 4) |
	      ((p[2] & 0xf) << 8) |
	      ((p[3] & 0xf) << 12));


   if (newMask == 0xffff || newMask == 0x0) {
      /* this is needed to make conform pass */
      imesa->hw_stipple = 0;
      return;
   }

   imesa->StippleSetup[I830_STPREG_ST1] &= ~0xffff;
   imesa->StippleSetup[I830_STPREG_ST1] |= newMask;
   imesa->hw_stipple = 1;

   if (active)
      imesa->StippleSetup[I830_STPREG_ST1] |= ST1_ENABLE;
}

static void i830PolygonStippleFallback( GLcontext *ctx, const GLubyte *mask )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   imesa->hw_stipple = 0;
   (void) i830PolygonStipple;
}

/* =============================================================
 * Hardware clipping
 */
static void i830Scissor(GLcontext *ctx, GLint x, GLint y, 
			GLsizei w, GLsizei h)
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   int x1 = x;
   int y1 = imesa->driDrawable->h - (y + h);
   int x2 = x + w - 1;
   int y2 = y1 + h - 1;

   if (I830_DEBUG&DEBUG_DRI)
      fprintf(stderr, "[%s] x(%d) y(%d) w(%d) h(%d)\n", __FUNCTION__,
	      x, y, w, h);

   if (x1 < 0) x1 = 0;
   if (y1 < 0) y1 = 0;
   if (x2 < 0) x2 = 0;
   if (y2 < 0) y2 = 0;

   if (x2 >= imesa->i830Screen->width) x2 = imesa->i830Screen->width-1;
   if (y2 >= imesa->i830Screen->height) y2 = imesa->i830Screen->height-1;
   if (x1 >= imesa->i830Screen->width) x1 = imesa->i830Screen->width-1;
   if (y1 >= imesa->i830Screen->height) y1 = imesa->i830Screen->height-1;


   I830_STATECHANGE(imesa, I830_UPLOAD_BUFFERS);
   imesa->BufferSetup[I830_DESTREG_SR1] = (y1 << 16) | (x1 & 0xffff);
   imesa->BufferSetup[I830_DESTREG_SR2] = (y2 << 16) | (x2 & 0xffff);
}

static void i830LogicOp(GLcontext *ctx, GLenum opcode)
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   int tmp = 0;

   if (I830_DEBUG&DEBUG_DRI)
      fprintf(stderr, "%s\n", __FUNCTION__);

   /* FIXME: This should be a look-up table, like the r200 driver. */
   switch(opcode) {
   case GL_CLEAR: 
      tmp = LOGICOP_CLEAR; 
      break;
   case GL_AND: 
      tmp = LOGICOP_AND; 
      break;
   case GL_AND_REVERSE: 
      tmp = LOGICOP_AND_RVRSE; 
      break;
   case GL_COPY: 
      tmp = LOGICOP_COPY; 
      break;
   case GL_COPY_INVERTED: 
      tmp = LOGICOP_COPY_INV; 
      break;
   case GL_AND_INVERTED: 
      tmp = LOGICOP_AND_INV; 
      break;
   case GL_NOOP: 
      tmp = LOGICOP_NOOP; 
      break;
   case GL_XOR: 
      tmp = LOGICOP_XOR; 
      break;
   case GL_OR: 
      tmp = LOGICOP_OR; 
      break;
   case GL_OR_INVERTED: 
      tmp = LOGICOP_OR_INV; 
      break;
   case GL_NOR: 
      tmp = LOGICOP_NOR; 
      break;
   case GL_EQUIV: 
      tmp = LOGICOP_EQUIV; 
      break;
   case GL_INVERT: 
      tmp = LOGICOP_INV; 
      break;
   case GL_OR_REVERSE: 
      tmp = LOGICOP_OR_RVRSE; 
      break;
   case GL_NAND: 
      tmp = LOGICOP_NAND; 
      break;
   case GL_SET: 
      tmp = LOGICOP_SET; 
      break;
   default:
      return;
   }

   I830_STATECHANGE(imesa, I830_UPLOAD_CTX);
   imesa->Setup[I830_CTXREG_STATE4] &= ~LOGICOP_MASK;
   imesa->Setup[I830_CTXREG_STATE4] |= LOGIC_OP_FUNC(tmp);

   /* Make sure all the enables are correct */
   i830EvalLogicOpBlendState(ctx);
}

/* Fallback to swrast for select and feedback.
 */
static void i830RenderMode( GLcontext *ctx, GLenum mode )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   FALLBACK( imesa, I830_FALLBACK_RENDERMODE, (mode != GL_RENDER) );
}

static void i830DrawBuffer(GLcontext *ctx, GLenum mode )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);

   /*
    * _ColorDrawBufferMask is easier to cope with than <mode>.
    */
   switch ( ctx->DrawBuffer->_ColorDrawBufferMask[0] ) {
   case BUFFER_BIT_FRONT_LEFT:
      I830_FIREVERTICES(imesa);
      I830_STATECHANGE(imesa, I830_UPLOAD_BUFFERS);
      imesa->BufferSetup[I830_DESTREG_CBUFADDR] = imesa->i830Screen->fbOffset;
      i830XMesaSetFrontClipRects( imesa );
      FALLBACK( imesa, I830_FALLBACK_DRAW_BUFFER, GL_FALSE );
      break;
   case BUFFER_BIT_BACK_LEFT:
      I830_FIREVERTICES(imesa);
      I830_STATECHANGE(imesa, I830_UPLOAD_BUFFERS);
      imesa->BufferSetup[I830_DESTREG_CBUFADDR] = 
					imesa->i830Screen->backOffset;
      i830XMesaSetBackClipRects( imesa );
      FALLBACK( imesa, I830_FALLBACK_DRAW_BUFFER, GL_FALSE );
      break;
   default:
      /* GL_NONE or GL_FRONT_AND_BACK or stereo left&right, etc */
      FALLBACK( imesa, I830_FALLBACK_DRAW_BUFFER, GL_TRUE );
      return;
   }

   /* We want to update the s/w rast state too so that i830SetBuffer()
    * gets called.
    */
   _swrast_DrawBuffer(ctx, mode);
}

static void i830ReadBuffer(GLcontext *ctx, GLenum mode )
{
   /* nothing, until we implement h/w glRead/CopyPixels or CopyTexImage */
}

static void i830ClearColor(GLcontext *ctx, const GLfloat color[4])
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);

   CLAMPED_FLOAT_TO_UBYTE(imesa->clear_red, color[0]);
   CLAMPED_FLOAT_TO_UBYTE(imesa->clear_green, color[1]);
   CLAMPED_FLOAT_TO_UBYTE(imesa->clear_blue, color[2]);
   CLAMPED_FLOAT_TO_UBYTE(imesa->clear_alpha, color[3]);

   imesa->ClearColor = i830PackColor(imesa->i830Screen->fbFormat,
				     imesa->clear_red,
                                     imesa->clear_green,
                                     imesa->clear_blue,
                                     imesa->clear_alpha);
}

static void i830CullFaceFrontFace(GLcontext *ctx, GLenum unused)
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   GLuint mode = CULLMODE_BOTH;

   if (I830_DEBUG&DEBUG_DRI)
      fprintf(stderr, "%s\n", __FUNCTION__);

   if (ctx->Polygon.CullFaceMode != GL_FRONT_AND_BACK) {
      mode = CULLMODE_CW;

      if (ctx->Polygon.CullFaceMode == GL_FRONT)
	 mode ^= (CULLMODE_CW ^ CULLMODE_CCW);
      if (ctx->Polygon.FrontFace != GL_CCW)
	 mode ^= (CULLMODE_CW ^ CULLMODE_CCW);
   }

   imesa->LcsCullMode = mode;

   if (ctx->Polygon.CullFlag) {
      I830_STATECHANGE(imesa, I830_UPLOAD_CTX);
      imesa->Setup[I830_CTXREG_STATE3] &= ~CULLMODE_MASK;
      imesa->Setup[I830_CTXREG_STATE3] |= ENABLE_CULL_MODE | mode;
   }
}

static void i830LineWidth( GLcontext *ctx, GLfloat widthf )
{
   i830ContextPtr imesa = I830_CONTEXT( ctx );
   int width;

   if (I830_DEBUG&DEBUG_DRI)
      fprintf(stderr, "%s\n", __FUNCTION__);

   width = FloatToInt(widthf * 2);
   CLAMP_SELF(width, 1, 15);

   I830_STATECHANGE(imesa, I830_UPLOAD_CTX);
   imesa->Setup[I830_CTXREG_STATE5] &= ~FIXED_LINE_WIDTH_MASK;
   imesa->Setup[I830_CTXREG_STATE5] |= (ENABLE_FIXED_LINE_WIDTH |
				       FIXED_LINE_WIDTH(width));
}

static void i830PointSize(GLcontext *ctx, GLfloat size)
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   GLint point_size = FloatToInt(size);

   if (I830_DEBUG&DEBUG_DRI)
     fprintf(stderr, "%s\n", __FUNCTION__);

   CLAMP_SELF(point_size, 1, 256);
   I830_STATECHANGE(imesa, I830_UPLOAD_CTX);
   imesa->Setup[I830_CTXREG_STATE5] &= ~FIXED_POINT_WIDTH_MASK;
   imesa->Setup[I830_CTXREG_STATE5] |= (ENABLE_FIXED_POINT_WIDTH |
				       FIXED_POINT_WIDTH(point_size));
}


/* =============================================================
 * Color masks
 */

static void i830ColorMask(GLcontext *ctx,
			  GLboolean r, GLboolean g,
			  GLboolean b, GLboolean a)
{
   i830ContextPtr imesa = I830_CONTEXT( ctx );
   GLuint tmp = 0;

   if (I830_DEBUG&DEBUG_DRI)
      fprintf(stderr, "%s r(%d) g(%d) b(%d) a(%d)\n", __FUNCTION__, r, g, b, a);

   imesa->mask_red = !r;
   imesa->mask_green = !g;
   imesa->mask_blue = !b;
   imesa->mask_alpha = !a;

   tmp = (imesa->Setup[I830_CTXREG_ENABLES_2] & ~WRITEMASK_MASK) |
      ENABLE_COLOR_MASK |
      ENABLE_COLOR_WRITE |
      ((!r) << WRITEMASK_RED_SHIFT) |
      ((!g) << WRITEMASK_GREEN_SHIFT) |
      ((!b) << WRITEMASK_BLUE_SHIFT) |
      ((!a) << WRITEMASK_ALPHA_SHIFT);

   if (tmp != imesa->Setup[I830_CTXREG_ENABLES_2]) {
      I830_FIREVERTICES(imesa);
      imesa->dirty |= I830_UPLOAD_CTX;
      imesa->Setup[I830_CTXREG_ENABLES_2] = tmp;
   }
}

static void update_specular( GLcontext *ctx )
{
   i830ContextPtr imesa = I830_CONTEXT( ctx );

   I830_STATECHANGE(imesa, I830_UPLOAD_CTX);
   imesa->Setup[I830_CTXREG_ENABLES_1] &= ~ENABLE_SPEC_ADD_MASK;

   if (NEED_SECONDARY_COLOR(ctx))
      imesa->Setup[I830_CTXREG_ENABLES_1] |= ENABLE_SPEC_ADD;
   else
      imesa->Setup[I830_CTXREG_ENABLES_1] |= DISABLE_SPEC_ADD;
}

static void i830LightModelfv(GLcontext *ctx, GLenum pname, 
			     const GLfloat *param)
{
   if (I830_DEBUG&DEBUG_DRI)
      fprintf(stderr, "%s\n", __FUNCTION__);

   if (pname == GL_LIGHT_MODEL_COLOR_CONTROL) {
      update_specular( ctx );
   }
}

/* In Mesa 3.5 we can reliably do native flatshading.
 */
static void i830ShadeModel(GLcontext *ctx, GLenum mode)
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   I830_STATECHANGE(imesa, I830_UPLOAD_CTX);


#define SHADE_MODE_MASK ((1<<10)|(1<<8)|(1<<6)|(1<<4))

   imesa->Setup[I830_CTXREG_STATE3] &= ~SHADE_MODE_MASK;

   if (mode == GL_FLAT) {
     imesa->Setup[I830_CTXREG_STATE3] |= (ALPHA_SHADE_MODE(SHADE_MODE_FLAT) |
					  FOG_SHADE_MODE(SHADE_MODE_FLAT) |
					  SPEC_SHADE_MODE(SHADE_MODE_FLAT) |
					  COLOR_SHADE_MODE(SHADE_MODE_FLAT));
   } else {
     imesa->Setup[I830_CTXREG_STATE3] |= (ALPHA_SHADE_MODE(SHADE_MODE_LINEAR) |
					  FOG_SHADE_MODE(SHADE_MODE_LINEAR) |
					  SPEC_SHADE_MODE(SHADE_MODE_LINEAR) |
					  COLOR_SHADE_MODE(SHADE_MODE_LINEAR));
   }
}

/* =============================================================
 * Fog
 */
static void i830Fogfv(GLcontext *ctx, GLenum pname, const GLfloat *param)
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);

   if (I830_DEBUG&DEBUG_DRI)
      fprintf(stderr, "%s\n", __FUNCTION__);

   if (pname == GL_FOG_COLOR) {      
      GLuint color = (((GLubyte)(ctx->Fog.Color[0]*255.0F) << 16) |
		      ((GLubyte)(ctx->Fog.Color[1]*255.0F) << 8) |
		      ((GLubyte)(ctx->Fog.Color[2]*255.0F) << 0));

      I830_STATECHANGE(imesa, I830_UPLOAD_CTX);
      imesa->Setup[I830_CTXREG_FOGCOLOR] = (STATE3D_FOG_COLOR_CMD | color);
   }
}

/* =============================================================
 */

static void i830Enable(GLcontext *ctx, GLenum cap, GLboolean state)
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);

   switch(cap) {
   case GL_LIGHTING:
   case GL_COLOR_SUM_EXT:
      update_specular( ctx );
      break;

   case GL_ALPHA_TEST:
      I830_STATECHANGE(imesa, I830_UPLOAD_CTX);
      imesa->Setup[I830_CTXREG_ENABLES_1] &= ~ENABLE_DIS_ALPHA_TEST_MASK;
      if (state)
	 imesa->Setup[I830_CTXREG_ENABLES_1] |= ENABLE_ALPHA_TEST;
      else
	 imesa->Setup[I830_CTXREG_ENABLES_1] |= DISABLE_ALPHA_TEST;

      break;

   case GL_BLEND:
   case GL_COLOR_LOGIC_OP:
   case GL_INDEX_LOGIC_OP:
      i830EvalLogicOpBlendState(ctx);
      break;

   case GL_DITHER:
      I830_STATECHANGE(imesa, I830_UPLOAD_CTX);
      imesa->Setup[I830_CTXREG_ENABLES_2] &= ~ENABLE_DITHER;

      if (state)
	 imesa->Setup[I830_CTXREG_ENABLES_2] |= ENABLE_DITHER;
      else
	 imesa->Setup[I830_CTXREG_ENABLES_2] |= DISABLE_DITHER;
      break;

   case GL_DEPTH_TEST:
      I830_STATECHANGE(imesa, I830_UPLOAD_CTX);
      imesa->Setup[I830_CTXREG_ENABLES_1] &= ~ENABLE_DIS_DEPTH_TEST_MASK;

      if (state)
	 imesa->Setup[I830_CTXREG_ENABLES_1] |= ENABLE_DEPTH_TEST;
      else
	 imesa->Setup[I830_CTXREG_ENABLES_1] |= DISABLE_DEPTH_TEST;

      /* Also turn off depth writes when GL_DEPTH_TEST is disabled:
       */
      i830DepthMask( ctx, state );
      break;

   case GL_SCISSOR_TEST:
      I830_STATECHANGE(imesa, I830_UPLOAD_BUFFERS);
      
      if (state)
	 imesa->BufferSetup[I830_DESTREG_SENABLE] = 
	    (STATE3D_SCISSOR_ENABLE_CMD |
	     ENABLE_SCISSOR_RECT);
      else
	 imesa->BufferSetup[I830_DESTREG_SENABLE] = 
	    (STATE3D_SCISSOR_ENABLE_CMD |
	     DISABLE_SCISSOR_RECT);

      imesa->upload_cliprects = GL_TRUE;
      break;

   case GL_LINE_SMOOTH:
      if (imesa->reduced_primitive == GL_LINES) {
	 I830_STATECHANGE(imesa, I830_UPLOAD_CTX);

	 imesa->Setup[I830_CTXREG_AA] &= ~AA_LINE_ENABLE;
	 if (state)
	    imesa->Setup[I830_CTXREG_AA] |= AA_LINE_ENABLE;
	 else
	    imesa->Setup[I830_CTXREG_AA] |= AA_LINE_DISABLE;
      }
      break;

   case GL_FOG:
      I830_STATECHANGE(imesa, I830_UPLOAD_CTX);
      imesa->Setup[I830_CTXREG_ENABLES_1] &= ~ENABLE_DIS_FOG_MASK;
      if (state)
	 imesa->Setup[I830_CTXREG_ENABLES_1] |= I830_ENABLE_FOG;
      else
	 imesa->Setup[I830_CTXREG_ENABLES_1] |= I830_DISABLE_FOG;
      break;

   case GL_CULL_FACE:
      I830_STATECHANGE(imesa, I830_UPLOAD_CTX);
      imesa->Setup[I830_CTXREG_STATE3] &= ~CULLMODE_MASK;
      if (state)
	 imesa->Setup[I830_CTXREG_STATE3] |= (ENABLE_CULL_MODE |
					      imesa->LcsCullMode);
      else
	 imesa->Setup[I830_CTXREG_STATE3] |= (ENABLE_CULL_MODE |
					      CULLMODE_NONE);
      break;

   case GL_TEXTURE_2D:
/*       I830_STATECHANGE(imesa, I830_UPLOAD_CTX); */
/*       imesa->Setup[I830_CTXREG_ENABLES_1] &= ~ENABLE_SPEC_ADD_MASK; */
      break;

   case GL_STENCIL_TEST:
      if (imesa->hw_stencil) {
	 I830_STATECHANGE(imesa, I830_UPLOAD_CTX);
	 imesa->Setup[I830_CTXREG_ENABLES_1] &= ~ENABLE_STENCIL_TEST;
	 imesa->Setup[I830_CTXREG_ENABLES_2] &= ~ENABLE_STENCIL_WRITE;

	 if (state) {
	    imesa->Setup[I830_CTXREG_ENABLES_1] |= ENABLE_STENCIL_TEST;
	    imesa->Setup[I830_CTXREG_ENABLES_2] |= ENABLE_STENCIL_WRITE;
	 } else {
	    imesa->Setup[I830_CTXREG_ENABLES_1] |= DISABLE_STENCIL_TEST;
	    imesa->Setup[I830_CTXREG_ENABLES_2] |= DISABLE_STENCIL_WRITE;
	 }
      } else {
	 FALLBACK( imesa, I830_FALLBACK_STENCIL, state );
      }
      break;

   case GL_POLYGON_STIPPLE:
#if 0
      /* The stipple command worked on my 855GM box, but not my 845G.
       * I'll do more testing later to find out exactly which hardware
       * supports it.  Disabled for now.
       */
      if (imesa->hw_stipple && imesa->reduced_primitive == GL_TRIANGLES)
      {
	 I830_STATECHANGE(imesa, I830_UPLOAD_STIPPLE);
	 imesa->StippleSetup[I830_STPREG_ST1] &= ~ST1_ENABLE;
	 if (state)
	    imesa->StippleSetup[I830_STPREG_ST1] |= ST1_ENABLE;
      }
#endif
      break;

   default:
      ;
   }
}


void i830EmitDrawingRectangle( i830ContextPtr imesa )
{
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;
   i830ScreenPrivate *i830Screen = imesa->i830Screen;
   int x0 = imesa->drawX;
   int y0 = imesa->drawY;
   int x1 = x0 + dPriv->w;
   int y1 = y0 + dPriv->h;

   /* Don't set drawing rectangle */
   if (I830_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s x0(%d) x1(%d) y0(%d) y1(%d)\n", __FUNCTION__,
	      x0, x1, y0, y1);

   /* Coordinate origin of the window - may be offscreen.
    */
   imesa->BufferSetup[I830_DESTREG_DR4] = ((y0<<16) | 
					   (((unsigned)x0)&0xFFFF));
  
   /* Clip to screen.
    */
   if (x0 < 0) x0 = 0;
   if (y0 < 0) y0 = 0;
   if (x1 > i830Screen->width-1) x1 = i830Screen->width-1;
   if (y1 > i830Screen->height-1) y1 = i830Screen->height-1;


   /* Onscreen drawing rectangle.
    */
   imesa->BufferSetup[I830_DESTREG_DR2] = ((y0<<16) | x0);
   imesa->BufferSetup[I830_DESTREG_DR3] = (((y1+1)<<16) | (x1+1));

   
   /* Just add in our dirty flag, since we might be called when locked */
   /* Might want to modify how this is done. */
   imesa->dirty |= I830_UPLOAD_BUFFERS;

   if (0)
      fprintf(stderr, "[%s] DR2(0x%08x) DR3(0x%08x) DR4(0x%08x)\n",
	      __FUNCTION__,
	      imesa->BufferSetup[I830_DESTREG_DR2],
	      imesa->BufferSetup[I830_DESTREG_DR3],
	      imesa->BufferSetup[I830_DESTREG_DR4]);
}

/* This could be done in hardware, will do once I have the driver
 * up and running.
 */
static void i830CalcViewport( GLcontext *ctx )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   const GLfloat *v = ctx->Viewport._WindowMap.m;
   GLfloat *m = imesa->ViewportMatrix.m;

   /* See also i830_translate_vertex.  SUBPIXEL adjustments can be done
    * via state vars, too.
    */
   m[MAT_SX] =   v[MAT_SX];
   m[MAT_TX] =   v[MAT_TX] + SUBPIXEL_X;
   m[MAT_SY] = - v[MAT_SY];
   m[MAT_TY] = - v[MAT_TY] + imesa->driDrawable->h + SUBPIXEL_Y;
   m[MAT_SZ] =   v[MAT_SZ] * imesa->depth_scale;
   m[MAT_TZ] =   v[MAT_TZ] * imesa->depth_scale;
}

static void i830Viewport( GLcontext *ctx,
			  GLint x, GLint y,
			  GLsizei width, GLsizei height )
{
   /* update size of Mesa/software ancillary buffers */
   _mesa_ResizeBuffersMESA();
   i830CalcViewport( ctx );
}

static void i830DepthRange( GLcontext *ctx,
			    GLclampd nearval, GLclampd farval )
{
   i830CalcViewport( ctx );
}

void i830PrintDirty( const char *msg, GLuint state )
{
   fprintf(stderr, "%s (0x%x): %s%s%s%s%s%s%s%s%s%s%s\n",
	   msg,
	   (unsigned int) state,
	   (state & I830_UPLOAD_TEX0)  ? "upload-tex0, " : "",
	   (state & I830_UPLOAD_TEX1)  ? "upload-tex1, " : "",
	   (state & I830_UPLOAD_TEX2)  ? "upload-tex2, " : "",
	   (state & I830_UPLOAD_TEX3)  ? "upload-tex3, " : "",
	   (state & I830_UPLOAD_CTX)        ? "upload-ctx, " : "",
	   (state & I830_UPLOAD_BUFFERS)    ? "upload-bufs, " : "",
	   (state & I830_UPLOAD_TEXBLEND0)  ? "upload-blend0, " : "",
	   (state & I830_UPLOAD_TEXBLEND1)  ? "upload-blend1, " : "",
	   (state & I830_UPLOAD_TEXBLEND2)  ? "upload-blend2, " : "",
	   (state & I830_UPLOAD_TEXBLEND3)  ? "upload-blend3, " : "",
	   (state & I830_UPLOAD_STIPPLE)  ? "stipple, " : ""
	   );
}

/* Push the state into the sarea and/or texture memory.
 */
void i830EmitHwStateLocked( i830ContextPtr imesa )
{
   int i;

   if (I830_DEBUG & DEBUG_STATE)
      i830PrintDirty( __FUNCTION__, imesa->dirty );

   for ( i = 0 ; i < imesa->glCtx->Const.MaxTextureUnits ; i++ ) {
      if ( ((imesa->dirty & I830_UPLOAD_TEX_N_IMAGE( i )) != 0)
	  && (imesa->CurrentTexObj[i] != NULL) ) {
	 i830UploadTexImagesLocked(imesa, imesa->CurrentTexObj[i]);
      }
   }

   if (imesa->dirty & I830_UPLOAD_CTX) {
      memcpy( imesa->sarea->ContextState,
	     imesa->Setup, sizeof(imesa->Setup) );
   }

   for ( i = 0 ; i < imesa->glCtx->Const.MaxTextureUnits ; i++ ) {
      if ((imesa->dirty & I830_UPLOAD_TEX_N(i)) && imesa->CurrentTexObj[i]) {
	 unsigned * TexState;
	 
	 imesa->sarea->dirty |= I830_UPLOAD_TEX_N(i);
	 
	 switch( i ) {
	 case 0:
	 case 1:
	    TexState = imesa->sarea->TexState[i];
	    break;

	 case 2:
	    TexState = imesa->sarea->TexState2;
	    break;

	 case 3:
	    TexState = imesa->sarea->TexState3;
	    break;
	 }

	 memcpy(TexState, imesa->CurrentTexObj[i]->Setup,
		sizeof(imesa->sarea->TexState[i]));
	  
	 TexState[I830_TEXREG_TM0S3] &= ~TM0S3_LOD_BIAS_MASK;
	 TexState[I830_TEXREG_TM0S3] |= imesa->LodBias[i];

	 /* Update the LRU usage */
	 if (imesa->CurrentTexObj[i]->base.memBlock)
	    driUpdateTextureLRU( (driTextureObject *) 
				 imesa->CurrentTexObj[i] );
      }
   }
   /* Need to figure out if texturing state, or enable changed. */

   for ( i = 0 ; i < imesa->glCtx->Const.MaxTextureUnits ; i++ ) {
      if (imesa->dirty & I830_UPLOAD_TEXBLEND_N(i)) {
	 unsigned * TexBlendState;
	 unsigned * words_used;
	 
	 imesa->sarea->dirty |= I830_UPLOAD_TEXBLEND_N(i);

	 switch( i ) {
	 case 0:
	 case 1:
	    TexBlendState = imesa->sarea->TexBlendState[i];
	    words_used = & imesa->sarea->TexBlendStateWordsUsed[i];
	    break;

	 case 2:
	    TexBlendState = imesa->sarea->TexBlendState2;
	    words_used = & imesa->sarea->TexBlendStateWordsUsed2;
	    break;

	 case 3:
	    TexBlendState = imesa->sarea->TexBlendState3;
	    words_used = & imesa->sarea->TexBlendStateWordsUsed3;
	    break;
	 }

	 memcpy(TexBlendState, imesa->TexBlend[i],
		imesa->TexBlendWordsUsed[i] * 4);
	 *words_used = imesa->TexBlendWordsUsed[i];
      }
   }

   if (imesa->dirty & I830_UPLOAD_BUFFERS) {
      memcpy( imesa->sarea->BufferState,imesa->BufferSetup, 
	      sizeof(imesa->BufferSetup) );
   }

   if (imesa->dirty & I830_UPLOAD_STIPPLE) {
      memcpy( imesa->sarea->StippleState,imesa->StippleSetup, 
	      sizeof(imesa->StippleSetup) );
   }

   if (imesa->dirty & I830_UPLOAD_TEX_PALETTE_SHARED) {
      memcpy( imesa->sarea->Palette[0],imesa->palette,
	      sizeof(imesa->sarea->Palette[0]));
   } else {
      i830TextureObjectPtr p;
      if (imesa->dirty & I830_UPLOAD_TEX_PALETTE_N(0)) {
	 p = imesa->CurrentTexObj[0];
	 memcpy( imesa->sarea->Palette[0],p->palette,
		sizeof(imesa->sarea->Palette[0]));
      }
      if (imesa->dirty & I830_UPLOAD_TEX_PALETTE_N(1)) {
	 p = imesa->CurrentTexObj[1];
	 memcpy( imesa->sarea->Palette[1],
		 p->palette,
		 sizeof(imesa->sarea->Palette[1]));
      }
   }
   
   imesa->sarea->dirty |= (imesa->dirty & ~(I830_UPLOAD_TEX_MASK | 
					    I830_UPLOAD_TEXBLEND_MASK));

   imesa->upload_cliprects = GL_TRUE;
   imesa->dirty = 0;
}


void i830DDInitState( GLcontext *ctx )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);   
   i830ScreenPrivate *i830Screen = imesa->i830Screen;
   int i;

   imesa->clear_red = 0;
   imesa->clear_green = 0;
   imesa->clear_blue = 0;
   imesa->clear_alpha = 0;

   imesa->mask_red = GL_FALSE;
   imesa->mask_green = GL_FALSE;
   imesa->mask_blue = GL_FALSE;
   imesa->mask_alpha = GL_FALSE;

   /* Zero all texture state */
   for (i = 0; i < I830_MAX_TEXTURE_UNITS; i++) {
      (void) memset( imesa->TexBlend[i], 0, sizeof( imesa->TexBlend[i] ) );
      (void) memset( imesa->Init_TexBlend[i], 0, sizeof( imesa->Init_TexBlend[i] ) );

      imesa->TexBlendWordsUsed[i] = 0;
      imesa->Init_TexBlendWordsUsed[i] = 0;
   }

   /* Set default blend state */
   imesa->TexBlend[0][0] = (STATE3D_MAP_BLEND_OP_CMD(0) |
			    TEXPIPE_COLOR |
			    ENABLE_TEXOUTPUT_WRT_SEL |
			    TEXOP_OUTPUT_CURRENT |
			    DISABLE_TEX_CNTRL_STAGE |
			    TEXOP_SCALE_1X |
			    TEXOP_MODIFY_PARMS |
			    TEXOP_LAST_STAGE |
			    TEXBLENDOP_ARG1);
   imesa->TexBlend[0][1] = (STATE3D_MAP_BLEND_OP_CMD(0) |
			    TEXPIPE_ALPHA |
			    ENABLE_TEXOUTPUT_WRT_SEL |
			    TEXOP_OUTPUT_CURRENT |
			    TEXOP_SCALE_1X |
			    TEXOP_MODIFY_PARMS |
			    TEXBLENDOP_ARG1);
   imesa->TexBlend[0][2] = (STATE3D_MAP_BLEND_ARG_CMD(0) |
			    TEXPIPE_COLOR |
			    TEXBLEND_ARG1 |
			    TEXBLENDARG_MODIFY_PARMS |
			    TEXBLENDARG_DIFFUSE);
   imesa->TexBlend[0][3] = (STATE3D_MAP_BLEND_ARG_CMD(0) |
			    TEXPIPE_ALPHA |
			    TEXBLEND_ARG1 |
			    TEXBLENDARG_MODIFY_PARMS |
			    TEXBLENDARG_DIFFUSE);

   imesa->TexBlendWordsUsed[0] = 4;

   imesa->Init_TexBlend[0][0] = (STATE3D_MAP_BLEND_OP_CMD(0) |
			    TEXPIPE_COLOR |
			    ENABLE_TEXOUTPUT_WRT_SEL |
			    TEXOP_OUTPUT_CURRENT |
			    DISABLE_TEX_CNTRL_STAGE |
			    TEXOP_SCALE_1X |
			    TEXOP_MODIFY_PARMS |
			    TEXOP_LAST_STAGE |
			    TEXBLENDOP_ARG1);
   imesa->Init_TexBlend[0][1] = (STATE3D_MAP_BLEND_OP_CMD(0) |
			    TEXPIPE_ALPHA |
			    ENABLE_TEXOUTPUT_WRT_SEL |
			    TEXOP_OUTPUT_CURRENT |
			    TEXOP_SCALE_1X |
			    TEXOP_MODIFY_PARMS |
			    TEXBLENDOP_ARG1);
   imesa->Init_TexBlend[0][2] = (STATE3D_MAP_BLEND_ARG_CMD(0) |
			    TEXPIPE_COLOR |
			    TEXBLEND_ARG1 |
			    TEXBLENDARG_MODIFY_PARMS |
			    TEXBLENDARG_CURRENT);
   imesa->Init_TexBlend[0][3] = (STATE3D_MAP_BLEND_ARG_CMD(0) |
			    TEXPIPE_ALPHA |
			    TEXBLEND_ARG1 |
			    TEXBLENDARG_MODIFY_PARMS |
			    TEXBLENDARG_CURRENT);
   imesa->Init_TexBlendWordsUsed[0] = 4;

   memset(imesa->Setup, 0, sizeof(imesa->Setup));

   imesa->Setup[I830_CTXREG_VF] =  0;
   imesa->Setup[I830_CTXREG_VF2] = 0;

   imesa->Setup[I830_CTXREG_AA] = (STATE3D_AA_CMD |
				   AA_LINE_ECAAR_WIDTH_ENABLE |
				   AA_LINE_ECAAR_WIDTH_1_0 |
				   AA_LINE_REGION_WIDTH_ENABLE |
				   AA_LINE_REGION_WIDTH_1_0 | 
				   AA_LINE_DISABLE);

   imesa->Setup[I830_CTXREG_ENABLES_1] = (STATE3D_ENABLES_1_CMD |
					  DISABLE_LOGIC_OP |
					  DISABLE_STENCIL_TEST |
					  DISABLE_DEPTH_BIAS |
					  DISABLE_SPEC_ADD |
					  I830_DISABLE_FOG |
					  DISABLE_ALPHA_TEST |
					  DISABLE_COLOR_BLEND |
					  DISABLE_DEPTH_TEST);

   if (imesa->hw_stencil) {
      imesa->Setup[I830_CTXREG_ENABLES_2] = (STATE3D_ENABLES_2_CMD |
					     ENABLE_STENCIL_WRITE |
					     ENABLE_TEX_CACHE |
					     ENABLE_DITHER |
					     ENABLE_COLOR_MASK |
					     /* set no color comps disabled */
					     ENABLE_COLOR_WRITE |
					     ENABLE_DEPTH_WRITE);
   } else {
      imesa->Setup[I830_CTXREG_ENABLES_2] = (STATE3D_ENABLES_2_CMD |
					     DISABLE_STENCIL_WRITE |
					     ENABLE_TEX_CACHE |
					     ENABLE_DITHER |
					     ENABLE_COLOR_MASK |
					     /* set no color comps disabled */
					     ENABLE_COLOR_WRITE |
					     ENABLE_DEPTH_WRITE);
   }

   imesa->Setup[I830_CTXREG_STATE1] = (STATE3D_MODES_1_CMD |
   				      ENABLE_COLR_BLND_FUNC |
				      BLENDFUNC_ADD |
				      ENABLE_SRC_BLND_FACTOR |
				      SRC_BLND_FACT(BLENDFACT_ONE) | 
				      ENABLE_DST_BLND_FACTOR |
				      DST_BLND_FACT(BLENDFACT_ZERO) );

   imesa->Setup[I830_CTXREG_STATE2] = (STATE3D_MODES_2_CMD |
   				      ENABLE_GLOBAL_DEPTH_BIAS | 
				      GLOBAL_DEPTH_BIAS(0) |
				      ENABLE_ALPHA_TEST_FUNC | 
				      ALPHA_TEST_FUNC(COMPAREFUNC_ALWAYS) |
				      ALPHA_REF_VALUE(0) );

   imesa->Setup[I830_CTXREG_STATE3] = (STATE3D_MODES_3_CMD |
   				      ENABLE_DEPTH_TEST_FUNC |
				      DEPTH_TEST_FUNC(COMPAREFUNC_LESS) |
				      ENABLE_ALPHA_SHADE_MODE |
				      ALPHA_SHADE_MODE(SHADE_MODE_LINEAR) |
				      ENABLE_FOG_SHADE_MODE |
				      FOG_SHADE_MODE(SHADE_MODE_LINEAR) |
				      ENABLE_SPEC_SHADE_MODE |
				      SPEC_SHADE_MODE(SHADE_MODE_LINEAR) |
				      ENABLE_COLOR_SHADE_MODE |
				      COLOR_SHADE_MODE(SHADE_MODE_LINEAR) |
				      ENABLE_CULL_MODE |
				      CULLMODE_NONE);

   imesa->Setup[I830_CTXREG_STATE4] = (STATE3D_MODES_4_CMD |
				      ENABLE_LOGIC_OP_FUNC |
				      LOGIC_OP_FUNC(LOGICOP_COPY) |
				      ENABLE_STENCIL_TEST_MASK |
				      STENCIL_TEST_MASK(0xff) |
				      ENABLE_STENCIL_WRITE_MASK |
				      STENCIL_WRITE_MASK(0xff));

   imesa->Setup[I830_CTXREG_STENCILTST] = (STATE3D_STENCIL_TEST_CMD |
				  ENABLE_STENCIL_PARMS |
				  STENCIL_FAIL_OP(STENCILOP_KEEP) |
				  STENCIL_PASS_DEPTH_FAIL_OP(STENCILOP_KEEP) |
				  STENCIL_PASS_DEPTH_PASS_OP(STENCILOP_KEEP) |
				  ENABLE_STENCIL_TEST_FUNC |
				  STENCIL_TEST_FUNC(COMPAREFUNC_ALWAYS) |
				  ENABLE_STENCIL_REF_VALUE |
				  STENCIL_REF_VALUE(0) );

   imesa->Setup[I830_CTXREG_STATE5] = (STATE3D_MODES_5_CMD |
				       FLUSH_TEXTURE_CACHE |
				       ENABLE_SPRITE_POINT_TEX |
				       SPRITE_POINT_TEX_OFF |
				       ENABLE_FIXED_LINE_WIDTH |
				       FIXED_LINE_WIDTH(0x2) | /* 1.0 */
				       ENABLE_FIXED_POINT_WIDTH |
				       FIXED_POINT_WIDTH(1) );

   imesa->Setup[I830_CTXREG_IALPHAB] = (STATE3D_INDPT_ALPHA_BLEND_CMD |
   				       DISABLE_INDPT_ALPHA_BLEND |
				       ENABLE_ALPHA_BLENDFUNC |
				       ABLENDFUNC_ADD);

   imesa->Setup[I830_CTXREG_FOGCOLOR] = (STATE3D_FOG_COLOR_CMD |
   					FOG_COLOR_RED(0) |
   					FOG_COLOR_GREEN(0) |
   					FOG_COLOR_BLUE(0));

   imesa->Setup[I830_CTXREG_BLENDCOLR0] = (STATE3D_CONST_BLEND_COLOR_CMD);

   imesa->Setup[I830_CTXREG_BLENDCOLR] = 0;

   imesa->Setup[I830_CTXREG_MCSB0] = STATE3D_MAP_COORD_SETBIND_CMD;
   imesa->Setup[I830_CTXREG_MCSB1] = (TEXBIND_SET3(TEXCOORDSRC_VTXSET_3) |
				     TEXBIND_SET2(TEXCOORDSRC_VTXSET_2) |
				     TEXBIND_SET1(TEXCOORDSRC_VTXSET_1) |
				     TEXBIND_SET0(TEXCOORDSRC_VTXSET_0));

   imesa->LcsCullMode = CULLMODE_CW; /* GL default */

   memset(imesa->BufferSetup, 0, sizeof(imesa->BufferSetup));
   memset(imesa->StippleSetup, 0, sizeof(imesa->StippleSetup));


   if (imesa->glCtx->Visual.doubleBufferMode &&
       imesa->sarea->pf_current_page == 0) {
      imesa->drawMap = i830Screen->back.map;
      imesa->readMap = i830Screen->back.map;
      imesa->BufferSetup[I830_DESTREG_CBUFADDR] = i830Screen->backOffset;
      imesa->BufferSetup[I830_DESTREG_DBUFADDR] = 0;
   } else {
      /* use front buffer by default */
      imesa->drawMap = (char *)imesa->driScreen->pFB;
      imesa->readMap = (char *)imesa->driScreen->pFB;
      imesa->BufferSetup[I830_DESTREG_CBUFADDR] = i830Screen->fbOffset;
      imesa->BufferSetup[I830_DESTREG_DBUFADDR] = 0;      
   }

   imesa->BufferSetup[I830_DESTREG_DV0] = STATE3D_DST_BUF_VARS_CMD;

   switch (i830Screen->fbFormat) {
   case DV_PF_555:
   case DV_PF_565:
      imesa->BufferSetup[I830_DESTREG_DV1] = (DSTORG_HORT_BIAS(0x8) | /* .5 */
					     DSTORG_VERT_BIAS(0x8) | /* .5 */
					     i830Screen->fbFormat |
					     DEPTH_IS_Z |
					     DEPTH_FRMT_16_FIXED);
      break;
   case DV_PF_8888:
      imesa->BufferSetup[I830_DESTREG_DV1] = (DSTORG_HORT_BIAS(0x8) | /* .5 */
					     DSTORG_VERT_BIAS(0x8) | /* .5 */
					     i830Screen->fbFormat |
					     DEPTH_IS_Z |
					     DEPTH_FRMT_24_FIXED_8_OTHER);
      break;
   }
   imesa->BufferSetup[I830_DESTREG_SENABLE] = (STATE3D_SCISSOR_ENABLE_CMD |
					      DISABLE_SCISSOR_RECT);
   imesa->BufferSetup[I830_DESTREG_SR0] = STATE3D_SCISSOR_RECT_0_CMD;
   imesa->BufferSetup[I830_DESTREG_SR1] = 0;
   imesa->BufferSetup[I830_DESTREG_SR2] = 0;

   imesa->BufferSetup[I830_DESTREG_DR0] = STATE3D_DRAW_RECT_CMD;
   imesa->BufferSetup[I830_DESTREG_DR1] = 0;
   imesa->BufferSetup[I830_DESTREG_DR2] = 0;
   imesa->BufferSetup[I830_DESTREG_DR3] = (((i830Screen->height)<<16) | 
					  (i830Screen->width));
   imesa->BufferSetup[I830_DESTREG_DR4] = 0;

   memcpy( imesa->Init_Setup,
	   imesa->Setup, 
	   sizeof(imesa->Setup) );
   memcpy( imesa->Init_BufferSetup,
	   imesa->BufferSetup, 
	   sizeof(imesa->BufferSetup) );

}

static void i830InvalidateState( GLcontext *ctx, GLuint new_state )
{
   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _ac_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   I830_CONTEXT(ctx)->NewGLState |= new_state;
}

void i830DDInitStateFuncs(GLcontext *ctx)
{
   /* Callbacks for internal Mesa events.
    */
   ctx->Driver.UpdateState = i830InvalidateState;

   /* API callbacks
    */
   ctx->Driver.AlphaFunc = i830AlphaFunc;
   ctx->Driver.BlendEquationSeparate = i830BlendEquationSeparate;
   ctx->Driver.BlendFuncSeparate = i830BlendFuncSeparate;
   ctx->Driver.BlendColor = i830BlendColor;
   ctx->Driver.ClearColor = i830ClearColor;
   ctx->Driver.ColorMask = i830ColorMask;
   ctx->Driver.CullFace = i830CullFaceFrontFace;
   ctx->Driver.DepthFunc = i830DepthFunc;
   ctx->Driver.DepthMask = i830DepthMask;
   ctx->Driver.Enable = i830Enable;
   ctx->Driver.Fogfv = i830Fogfv;
   ctx->Driver.FrontFace = i830CullFaceFrontFace;
   ctx->Driver.LineWidth = i830LineWidth;
   ctx->Driver.PointSize = i830PointSize;
   ctx->Driver.LogicOpcode = i830LogicOp;
   ctx->Driver.PolygonStipple = i830PolygonStippleFallback;
   ctx->Driver.RenderMode = i830RenderMode;
   ctx->Driver.Scissor = i830Scissor;
   ctx->Driver.DrawBuffer = i830DrawBuffer;
   ctx->Driver.ReadBuffer = i830ReadBuffer;
   ctx->Driver.ShadeModel = i830ShadeModel;
   ctx->Driver.DepthRange = i830DepthRange;
   ctx->Driver.Viewport = i830Viewport;
   ctx->Driver.LightModelfv = i830LightModelfv;

   ctx->Driver.StencilFunc = i830StencilFunc;
   ctx->Driver.StencilMask = i830StencilMask;
   ctx->Driver.StencilOp = i830StencilOp;

   /* Pixel path fallbacks.
    */
   ctx->Driver.Accum = _swrast_Accum;
   ctx->Driver.Bitmap = _swrast_Bitmap;
   ctx->Driver.CopyPixels = _swrast_CopyPixels;
   ctx->Driver.DrawPixels = _swrast_DrawPixels;
   ctx->Driver.ReadPixels = _swrast_ReadPixels;

   /* Swrast hooks for imaging extensions:
    */
   ctx->Driver.CopyColorTable = _swrast_CopyColorTable;
   ctx->Driver.CopyColorSubTable = _swrast_CopyColorSubTable;
   ctx->Driver.CopyConvolutionFilter1D = _swrast_CopyConvolutionFilter1D;
   ctx->Driver.CopyConvolutionFilter2D = _swrast_CopyConvolutionFilter2D;
}
