/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_state.c,v 1.11 2002/10/30 12:51:39 alanh Exp $ */
/**************************************************************************

Copyright 1999, 2000 ATI Technologies Inc. and Precision Insight, Inc.,
                                               Cedar Park, Texas.
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
ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Gareth Hughes <gareth@valinux.com>
 *   Kevin E. Martin <martin@valinux.com>
 *   Keith Whitwell <keith@tungstengraphics.com>
 *
 */

#include "r128_context.h"
#include "r128_state.h"
#include "r128_ioctl.h"
#include "r128_tris.h"
#include "r128_tex.h"

#include "context.h"
#include "enums.h"
#include "colormac.h"
#include "swrast/swrast.h"
#include "vbo/vbo.h"
#include "tnl/tnl.h"
#include "swrast_setup/swrast_setup.h"

#include "tnl/t_pipeline.h"

#include "drirenderbuffer.h"


/* =============================================================
 * Alpha blending
 */


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
static int blend_factor( r128ContextPtr rmesa, GLenum factor, GLboolean is_src )
{
   int   func;

   switch ( factor ) {
   case GL_ZERO:
      func = R128_ALPHA_BLEND_ZERO;
      break;
   case GL_ONE:
      func = R128_ALPHA_BLEND_ONE;
      break;

   case GL_SRC_COLOR:
      func = R128_ALPHA_BLEND_SRCCOLOR;
      break;
   case GL_ONE_MINUS_SRC_COLOR:
      func = R128_ALPHA_BLEND_INVSRCCOLOR;
      break;
   case GL_SRC_ALPHA:
      func = R128_ALPHA_BLEND_SRCALPHA;
      break;
   case GL_ONE_MINUS_SRC_ALPHA:
      func = R128_ALPHA_BLEND_INVSRCALPHA;
      break;
   case GL_SRC_ALPHA_SATURATE:
      func = (is_src) ? R128_ALPHA_BLEND_SAT : R128_ALPHA_BLEND_ZERO;
      break;

   case GL_DST_COLOR:
      func = R128_ALPHA_BLEND_DSTCOLOR;
      break;
   case GL_ONE_MINUS_DST_COLOR:
      func = R128_ALPHA_BLEND_INVDSTCOLOR;
      break;
   case GL_DST_ALPHA:
      func = R128_ALPHA_BLEND_DSTALPHA;
      break;
   case GL_ONE_MINUS_DST_ALPHA:
      func = R128_ALPHA_BLEND_INVDSTALPHA;
      break;

   case GL_CONSTANT_COLOR:
   case GL_ONE_MINUS_CONSTANT_COLOR:
   case GL_CONSTANT_ALPHA:
   case GL_ONE_MINUS_CONSTANT_ALPHA:
   default:
      FALLBACK( rmesa, R128_FALLBACK_BLEND_FUNC, GL_TRUE );
      func = (is_src) ? R128_ALPHA_BLEND_ONE : R128_ALPHA_BLEND_ZERO;
      break;
   }
   
   return func;
}


static void r128UpdateAlphaMode( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   GLuint a = rmesa->setup.misc_3d_state_cntl_reg;
   GLuint t = rmesa->setup.tex_cntl_c;

   if ( ctx->Color.AlphaEnabled ) {
      GLubyte ref;

      CLAMPED_FLOAT_TO_UBYTE(ref, ctx->Color.AlphaRef);

      a &= ~(R128_ALPHA_TEST_MASK | R128_REF_ALPHA_MASK);

      switch ( ctx->Color.AlphaFunc ) {
      case GL_NEVER:
	 a |= R128_ALPHA_TEST_NEVER;
	 break;
      case GL_LESS:
	 a |= R128_ALPHA_TEST_LESS;
         break;
      case GL_LEQUAL:
	 a |= R128_ALPHA_TEST_LESSEQUAL;
	 break;
      case GL_EQUAL:
	 a |= R128_ALPHA_TEST_EQUAL;
	 break;
      case GL_GEQUAL:
	 a |= R128_ALPHA_TEST_GREATEREQUAL;
	 break;
      case GL_GREATER:
	 a |= R128_ALPHA_TEST_GREATER;
	 break;
      case GL_NOTEQUAL:
	 a |= R128_ALPHA_TEST_NEQUAL;
	 break;
      case GL_ALWAYS:
	 a |= R128_ALPHA_TEST_ALWAYS;
	 break;
      }

      a |= ref & R128_REF_ALPHA_MASK;
      t |= R128_ALPHA_TEST_ENABLE;
   } else {
      t &= ~R128_ALPHA_TEST_ENABLE;
   }

   FALLBACK( rmesa, R128_FALLBACK_BLEND_FUNC, GL_FALSE );

   if ( ctx->Color.BlendEnabled ) {
      a &= ~((R128_ALPHA_BLEND_MASK << R128_ALPHA_BLEND_SRC_SHIFT) |
	     (R128_ALPHA_BLEND_MASK << R128_ALPHA_BLEND_DST_SHIFT)
	     | R128_ALPHA_COMB_FCN_MASK);

      a |= blend_factor( rmesa, ctx->Color.BlendSrcRGB, GL_TRUE ) 
	  << R128_ALPHA_BLEND_SRC_SHIFT;
      a |= blend_factor( rmesa, ctx->Color.BlendDstRGB, GL_FALSE ) 
	  << R128_ALPHA_BLEND_DST_SHIFT;

      switch (ctx->Color.BlendEquationRGB) {
      case GL_FUNC_ADD:
	 a |= R128_ALPHA_COMB_ADD_CLAMP;
	 break;
      case GL_FUNC_SUBTRACT:
	 a |= R128_ALPHA_COMB_SUB_SRC_DST_CLAMP;
	 break;
      default:
	 FALLBACK( rmesa, R128_FALLBACK_BLEND_EQ, GL_TRUE );
      }

      t |=  R128_ALPHA_ENABLE;
   } else {
      t &= ~R128_ALPHA_ENABLE;
   }

   if ( rmesa->setup.misc_3d_state_cntl_reg != a ) {
      rmesa->setup.misc_3d_state_cntl_reg = a;
      rmesa->dirty |= R128_UPLOAD_CONTEXT | R128_UPLOAD_MASKS;
   }
   if ( rmesa->setup.tex_cntl_c != t ) {
      rmesa->setup.tex_cntl_c = t;
      rmesa->dirty |= R128_UPLOAD_CONTEXT | R128_UPLOAD_MASKS;
   }
}

static void r128DDAlphaFunc( GLcontext *ctx, GLenum func, GLfloat ref )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   FLUSH_BATCH( rmesa );
   rmesa->new_state |= R128_NEW_ALPHA;
}

static void r128DDBlendEquationSeparate( GLcontext *ctx, 
					 GLenum modeRGB, GLenum modeA )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   assert( modeRGB == modeA );
   FLUSH_BATCH( rmesa );

   /* BlendEquation sets ColorLogicOpEnabled in an unexpected
    * manner.
    */
   FALLBACK( R128_CONTEXT(ctx), R128_FALLBACK_LOGICOP,
	     (ctx->Color.ColorLogicOpEnabled &&
	      ctx->Color.LogicOp != GL_COPY));

   /* Can only do blend addition, not min, max, subtract, etc. */
   FALLBACK( R128_CONTEXT(ctx), R128_FALLBACK_BLEND_EQ,
	     (modeRGB != GL_FUNC_ADD) && (modeRGB != GL_FUNC_SUBTRACT));

   rmesa->new_state |= R128_NEW_ALPHA;
}

static void r128DDBlendFuncSeparate( GLcontext *ctx,
				     GLenum sfactorRGB, GLenum dfactorRGB,
				     GLenum sfactorA, GLenum dfactorA )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   FLUSH_BATCH( rmesa );
   rmesa->new_state |= R128_NEW_ALPHA;
}

/* =============================================================
 * Stencil
 */

static void
r128DDStencilFuncSeparate( GLcontext *ctx, GLenum face, GLenum func,
                           GLint ref, GLuint mask )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   GLuint refmask = (((ctx->Stencil.Ref[0] & 0xff) << 0) |
		     ((ctx->Stencil.ValueMask[0] & 0xff) << 16) |
		     ((ctx->Stencil.WriteMask[0] & 0xff) << 24)); 
   GLuint z = rmesa->setup.z_sten_cntl_c;

   z &= ~R128_STENCIL_TEST_MASK;
   switch ( ctx->Stencil.Function[0] ) {
   case GL_NEVER:
      z |= R128_STENCIL_TEST_NEVER;
      break;
   case GL_LESS:
      z |= R128_STENCIL_TEST_LESS;
      break;
   case GL_EQUAL:
      z |= R128_STENCIL_TEST_EQUAL;
      break;
   case GL_LEQUAL:
      z |= R128_STENCIL_TEST_LESSEQUAL;
      break;
   case GL_GREATER:
      z |= R128_STENCIL_TEST_GREATER;
      break;
   case GL_NOTEQUAL:
      z |= R128_STENCIL_TEST_NEQUAL;
      break;
   case GL_GEQUAL:
      z |= R128_STENCIL_TEST_GREATEREQUAL;
      break;
   case GL_ALWAYS:
      z |= R128_STENCIL_TEST_ALWAYS;
      break;
   }

   if ( rmesa->setup.sten_ref_mask_c != refmask ) {
      rmesa->setup.sten_ref_mask_c = refmask;
      rmesa->dirty |= R128_UPLOAD_MASKS;
   }
   if ( rmesa->setup.z_sten_cntl_c != z ) {
      rmesa->setup.z_sten_cntl_c = z;
      rmesa->dirty |= R128_UPLOAD_CONTEXT;
   }
}

static void
r128DDStencilMaskSeparate( GLcontext *ctx, GLenum face, GLuint mask )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   GLuint refmask = (((ctx->Stencil.Ref[0] & 0xff) << 0) |
		     ((ctx->Stencil.ValueMask[0] & 0xff) << 16) |
		     ((ctx->Stencil.WriteMask[0] & 0xff) << 24)); 

   if ( rmesa->setup.sten_ref_mask_c != refmask ) {
      rmesa->setup.sten_ref_mask_c = refmask;
      rmesa->dirty |= R128_UPLOAD_MASKS;
   }
}

static void r128DDStencilOpSeparate( GLcontext *ctx, GLenum face, GLenum fail,
                                     GLenum zfail, GLenum zpass )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   GLuint z = rmesa->setup.z_sten_cntl_c;

   if (!( ctx->Visual.stencilBits > 0 && ctx->Visual.depthBits == 24 ))
      return;

   z &= ~(R128_STENCIL_S_FAIL_MASK | R128_STENCIL_ZPASS_MASK |
	  R128_STENCIL_ZFAIL_MASK);

   switch ( ctx->Stencil.FailFunc[0] ) {
   case GL_KEEP:
      z |= R128_STENCIL_S_FAIL_KEEP;
      break;
   case GL_ZERO:
      z |= R128_STENCIL_S_FAIL_ZERO;
      break;
   case GL_REPLACE:
      z |= R128_STENCIL_S_FAIL_REPLACE;
      break;
   case GL_INCR:
      z |= R128_STENCIL_S_FAIL_INC;
      break;
   case GL_DECR:
      z |= R128_STENCIL_S_FAIL_DEC;
      break;
   case GL_INVERT:
      z |= R128_STENCIL_S_FAIL_INV;
      break;
   case GL_INCR_WRAP:
      z |= R128_STENCIL_S_FAIL_INC_WRAP;
      break;
   case GL_DECR_WRAP:
      z |= R128_STENCIL_S_FAIL_DEC_WRAP;
      break;
   }

   switch ( ctx->Stencil.ZFailFunc[0] ) {
   case GL_KEEP:
      z |= R128_STENCIL_ZFAIL_KEEP;
      break;
   case GL_ZERO:
      z |= R128_STENCIL_ZFAIL_ZERO;
      break;
   case GL_REPLACE:
      z |= R128_STENCIL_ZFAIL_REPLACE;
      break;
   case GL_INCR:
      z |= R128_STENCIL_ZFAIL_INC;
      break;
   case GL_DECR:
      z |= R128_STENCIL_ZFAIL_DEC;
      break;
   case GL_INVERT:
      z |= R128_STENCIL_ZFAIL_INV;
      break;
   case GL_INCR_WRAP:
      z |= R128_STENCIL_ZFAIL_INC_WRAP;
      break;
   case GL_DECR_WRAP:
      z |= R128_STENCIL_ZFAIL_DEC_WRAP;
      break;
   }

   switch ( ctx->Stencil.ZPassFunc[0] ) {
   case GL_KEEP:
      z |= R128_STENCIL_ZPASS_KEEP;
      break;
   case GL_ZERO:
      z |= R128_STENCIL_ZPASS_ZERO;
      break;
   case GL_REPLACE:
      z |= R128_STENCIL_ZPASS_REPLACE;
      break;
   case GL_INCR:
      z |= R128_STENCIL_ZPASS_INC;
      break;
   case GL_DECR:
      z |= R128_STENCIL_ZPASS_DEC;
      break;
   case GL_INVERT:
      z |= R128_STENCIL_ZPASS_INV;
      break;
   case GL_INCR_WRAP:
      z |= R128_STENCIL_ZPASS_INC_WRAP;
      break;
   case GL_DECR_WRAP:
      z |= R128_STENCIL_ZPASS_DEC_WRAP;
      break;
   }

   if ( rmesa->setup.z_sten_cntl_c != z ) {
      rmesa->setup.z_sten_cntl_c = z;
      rmesa->dirty |= R128_UPLOAD_CONTEXT;
   }
}

static void r128DDClearStencil( GLcontext *ctx, GLint s )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   if (ctx->Visual.stencilBits > 0 && ctx->Visual.depthBits == 24) {
      rmesa->ClearDepth &= 0x00ffffff;
      rmesa->ClearDepth |= ctx->Stencil.Clear << 24;
   }
}

/* =============================================================
 * Depth testing
 */

static void r128UpdateZMode( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   GLuint z = rmesa->setup.z_sten_cntl_c;
   GLuint t = rmesa->setup.tex_cntl_c;

   if ( ctx->Depth.Test ) {
      z &= ~R128_Z_TEST_MASK;

      switch ( ctx->Depth.Func ) {
      case GL_NEVER:
	 z |= R128_Z_TEST_NEVER;
	 break;
      case GL_ALWAYS:
	 z |= R128_Z_TEST_ALWAYS;
	 break;
      case GL_LESS:
	 z |= R128_Z_TEST_LESS;
	 break;
      case GL_LEQUAL:
	 z |= R128_Z_TEST_LESSEQUAL;
	 break;
      case GL_EQUAL:
	 z |= R128_Z_TEST_EQUAL;
	 break;
      case GL_GEQUAL:
	 z |= R128_Z_TEST_GREATEREQUAL;
	 break;
      case GL_GREATER:
	 z |= R128_Z_TEST_GREATER;
	 break;
      case GL_NOTEQUAL:
	 z |= R128_Z_TEST_NEQUAL;
	 break;
      }

      t |=  R128_Z_ENABLE;
   } else {
      t &= ~R128_Z_ENABLE;
   }

   if ( ctx->Depth.Mask ) {
      t |=  R128_Z_WRITE_ENABLE;
   } else {
      t &= ~R128_Z_WRITE_ENABLE;
   }

   if ( rmesa->setup.z_sten_cntl_c != z ) {
      rmesa->setup.z_sten_cntl_c = z;
      rmesa->dirty |= R128_UPLOAD_CONTEXT;
   }
   if ( rmesa->setup.tex_cntl_c != t ) {
      rmesa->setup.tex_cntl_c = t;
      rmesa->dirty |= R128_UPLOAD_CONTEXT;
   }
}

static void r128DDDepthFunc( GLcontext *ctx, GLenum func )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   FLUSH_BATCH( rmesa );
   rmesa->new_state |= R128_NEW_DEPTH;
}

static void r128DDDepthMask( GLcontext *ctx, GLboolean flag )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   FLUSH_BATCH( rmesa );
   rmesa->new_state |= R128_NEW_DEPTH;
}

static void r128DDClearDepth( GLcontext *ctx, GLclampd d )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   switch ( rmesa->setup.z_sten_cntl_c &  R128_Z_PIX_WIDTH_MASK ) {
   case R128_Z_PIX_WIDTH_16:
      rmesa->ClearDepth = d * 0x0000ffff;
      break;
   case R128_Z_PIX_WIDTH_24:
      rmesa->ClearDepth = d * 0x00ffffff;
      rmesa->ClearDepth |= ctx->Stencil.Clear << 24;
      break;
   case R128_Z_PIX_WIDTH_32:
      rmesa->ClearDepth = d * 0xffffffff;
      break;
   }
}


/* =============================================================
 * Fog
 */

static void r128UpdateFogAttrib( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   GLuint t = rmesa->setup.tex_cntl_c;
   GLubyte c[4];
   GLuint col;

   if ( ctx->Fog.Enabled ) {
      t |=  R128_FOG_ENABLE;
   } else {
      t &= ~R128_FOG_ENABLE;
   }

   c[0] = FLOAT_TO_UBYTE( ctx->Fog.Color[0] );
   c[1] = FLOAT_TO_UBYTE( ctx->Fog.Color[1] );
   c[2] = FLOAT_TO_UBYTE( ctx->Fog.Color[2] );

   col = r128PackColor( 4, c[0], c[1], c[2], 0 );

   if ( rmesa->setup.fog_color_c != col ) {
      rmesa->setup.fog_color_c = col;
      rmesa->dirty |= R128_UPLOAD_CONTEXT;
   }
   if ( rmesa->setup.tex_cntl_c != t ) {
      rmesa->setup.tex_cntl_c = t;
      rmesa->dirty |= R128_UPLOAD_CONTEXT;
   }
}

static void r128DDFogfv( GLcontext *ctx, GLenum pname, const GLfloat *param )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   FLUSH_BATCH( rmesa );
   rmesa->new_state |= R128_NEW_FOG;
}


/* =============================================================
 * Clipping
 */

static void r128UpdateClipping( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   if ( rmesa->driDrawable ) {
      __DRIdrawablePrivate *drawable = rmesa->driDrawable;
      int x1 = 0;
      int y1 = 0;
      int x2 = drawable->w - 1;
      int y2 = drawable->h - 1;

      if ( ctx->Scissor.Enabled ) {
	 if ( ctx->Scissor.X > x1 ) {
	    x1 = ctx->Scissor.X;
	 }
	 if ( drawable->h - ctx->Scissor.Y - ctx->Scissor.Height > y1 ) {
	    y1 = drawable->h - ctx->Scissor.Y - ctx->Scissor.Height;
	 }
	 if ( ctx->Scissor.X + ctx->Scissor.Width - 1 < x2 ) {
	    x2 = ctx->Scissor.X + ctx->Scissor.Width - 1;
	 }
	 if ( drawable->h - ctx->Scissor.Y - 1 < y2 ) {
	    y2 = drawable->h - ctx->Scissor.Y - 1;
	 }
      }

      x1 += drawable->x;
      y1 += drawable->y;
      x2 += drawable->x;
      y2 += drawable->y;

      /* Clamp values to screen to avoid wrapping problems */
      if ( x1 < 0 )
         x1 = 0;
      else if ( x1 >= rmesa->driScreen->fbWidth )
         x1 = rmesa->driScreen->fbWidth - 1;
      if ( y1 < 0 )
         y1 = 0;
      else if ( y1 >= rmesa->driScreen->fbHeight )
         y1 = rmesa->driScreen->fbHeight - 1;
      if ( x2 < 0 )
         x2 = 0;
      else if ( x2 >= rmesa->driScreen->fbWidth )
         x2 = rmesa->driScreen->fbWidth - 1;
      if ( y2 < 0 )
         y2 = 0;
      else if ( y2 >= rmesa->driScreen->fbHeight )
         y2 = rmesa->driScreen->fbHeight - 1;

      rmesa->setup.sc_top_left_c     = (((y1 & 0x3FFF) << 16) | (x1 & 0x3FFF));
      rmesa->setup.sc_bottom_right_c = (((y2 & 0x3FFF) << 16) | (x2 & 0x3FFF));

      rmesa->dirty |= R128_UPLOAD_CONTEXT;
   }
}

static void r128DDScissor( GLcontext *ctx,
			   GLint x, GLint y, GLsizei w, GLsizei h )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   FLUSH_BATCH( rmesa );
   rmesa->new_state |= R128_NEW_CLIP;
}


/* =============================================================
 * Culling
 */

static void r128UpdateCull( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   GLuint f = rmesa->setup.pm4_vc_fpu_setup;

   f &= ~R128_FRONT_DIR_MASK;

   switch ( ctx->Polygon.FrontFace ) {
   case GL_CW:
      f |= R128_FRONT_DIR_CW;
      break;
   case GL_CCW:
      f |= R128_FRONT_DIR_CCW;
      break;
   }

   f |= R128_BACKFACE_SOLID | R128_FRONTFACE_SOLID;

   if ( ctx->Polygon.CullFlag ) {
      switch ( ctx->Polygon.CullFaceMode ) {
      case GL_FRONT:
	 f &= ~R128_FRONTFACE_SOLID;
	 break;
      case GL_BACK:
	 f &= ~R128_BACKFACE_SOLID;
	 break;
      case GL_FRONT_AND_BACK:
	 f &= ~(R128_BACKFACE_SOLID |
		R128_FRONTFACE_SOLID);
	 break;
      }
   }

   if ( 1 || rmesa->setup.pm4_vc_fpu_setup != f ) {
      rmesa->setup.pm4_vc_fpu_setup = f;
      rmesa->dirty |= R128_UPLOAD_CONTEXT | R128_UPLOAD_SETUP;
   }
}

static void r128DDCullFace( GLcontext *ctx, GLenum mode )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   FLUSH_BATCH( rmesa );
   rmesa->new_state |= R128_NEW_CULL;
}

static void r128DDFrontFace( GLcontext *ctx, GLenum mode )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   FLUSH_BATCH( rmesa );
   rmesa->new_state |= R128_NEW_CULL;
}


/* =============================================================
 * Masks
 */

static void r128UpdateMasks( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   GLuint mask = r128PackColor( rmesa->r128Screen->cpp,
				ctx->Color.ColorMask[RCOMP],
				ctx->Color.ColorMask[GCOMP],
				ctx->Color.ColorMask[BCOMP],
				ctx->Color.ColorMask[ACOMP] );

   if ( rmesa->setup.plane_3d_mask_c != mask ) {
      rmesa->setup.plane_3d_mask_c = mask;
      rmesa->dirty |= R128_UPLOAD_CONTEXT | R128_UPLOAD_MASKS;
   }
}

static void r128DDColorMask( GLcontext *ctx,
			     GLboolean r, GLboolean g,
			     GLboolean b, GLboolean a )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   FLUSH_BATCH( rmesa );
   rmesa->new_state |= R128_NEW_MASKS;
}


/* =============================================================
 * Rendering attributes
 *
 * We really don't want to recalculate all this every time we bind a
 * texture.  These things shouldn't change all that often, so it makes
 * sense to break them out of the core texture state update routines.
 */

static void updateSpecularLighting( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   GLuint t = rmesa->setup.tex_cntl_c;

   if ( NEED_SECONDARY_COLOR( ctx ) ) {
      if (ctx->Light.ShadeModel == GL_FLAT) {
         /* R128 can't do flat-shaded separate specular */
         t &= ~R128_SPEC_LIGHT_ENABLE;
         FALLBACK( rmesa, R128_FALLBACK_SEP_SPECULAR, GL_TRUE );
      }
      else {
         t |= R128_SPEC_LIGHT_ENABLE;
         FALLBACK( rmesa, R128_FALLBACK_SEP_SPECULAR, GL_FALSE );
      }
   }
   else {
      t &= ~R128_SPEC_LIGHT_ENABLE;
      FALLBACK( rmesa, R128_FALLBACK_SEP_SPECULAR, GL_FALSE );
   }

   if ( rmesa->setup.tex_cntl_c != t ) {
      rmesa->setup.tex_cntl_c = t;
      rmesa->dirty |= R128_UPLOAD_CONTEXT;
      rmesa->dirty |= R128_UPLOAD_SETUP;
      rmesa->new_state |= R128_NEW_CONTEXT;
   }
}


static void r128DDLightModelfv( GLcontext *ctx, GLenum pname,
				const GLfloat *param )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   if ( pname == GL_LIGHT_MODEL_COLOR_CONTROL ) {
      FLUSH_BATCH( rmesa );
      updateSpecularLighting(ctx);
   }
}

static void r128DDShadeModel( GLcontext *ctx, GLenum mode )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   GLuint s = rmesa->setup.pm4_vc_fpu_setup;

   s &= ~R128_FPU_COLOR_MASK;

   switch ( mode ) {
   case GL_FLAT:
      s |= R128_FPU_COLOR_FLAT;
      break;
   case GL_SMOOTH:
      s |= R128_FPU_COLOR_GOURAUD;
      break;
   default:
      return;
   }

   updateSpecularLighting(ctx);

   if ( rmesa->setup.pm4_vc_fpu_setup != s ) {
      FLUSH_BATCH( rmesa );
      rmesa->setup.pm4_vc_fpu_setup = s;

      rmesa->new_state |= R128_NEW_CONTEXT;
      rmesa->dirty |= R128_UPLOAD_SETUP;
   }
}


/* =============================================================
 * Window position
 */

static void r128UpdateWindow( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   int x = rmesa->driDrawable->x;
   int y = rmesa->driDrawable->y;
   struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[0][0];
   driRenderbuffer *drb = (driRenderbuffer *) rb;

   rmesa->setup.window_xy_offset = (((y & 0xFFF) << R128_WINDOW_Y_SHIFT) |
				    ((x & 0xFFF) << R128_WINDOW_X_SHIFT));

   rmesa->setup.dst_pitch_offset_c = (((drb->flippedPitch/8) << 21) |
                                      (drb->flippedOffset >> 5));


   rmesa->dirty |= R128_UPLOAD_CONTEXT | R128_UPLOAD_WINDOW;
}


/* =============================================================
 * Viewport
 */

static void r128CalcViewport( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   const GLfloat *v = ctx->Viewport._WindowMap.m;
   GLfloat *m = rmesa->hw_viewport;

   /* See also r128_translate_vertex.
    */
   m[MAT_SX] =   v[MAT_SX];
   m[MAT_TX] =   v[MAT_TX] + SUBPIXEL_X;
   m[MAT_SY] = - v[MAT_SY];
   m[MAT_TY] = - v[MAT_TY] + rmesa->driDrawable->h + SUBPIXEL_Y;
   m[MAT_SZ] =   v[MAT_SZ] * rmesa->depth_scale;
   m[MAT_TZ] =   v[MAT_TZ] * rmesa->depth_scale;
}

static void r128Viewport( GLcontext *ctx,
			  GLint x, GLint y,
			  GLsizei width, GLsizei height )
{
   r128CalcViewport( ctx );
}

static void r128DepthRange( GLcontext *ctx,
			    GLclampd nearval, GLclampd farval )
{
   r128CalcViewport( ctx );
}


/* =============================================================
 * Miscellaneous
 */

static void r128DDClearColor( GLcontext *ctx,
			      const GLfloat color[4] )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   GLubyte c[4];

   CLAMPED_FLOAT_TO_UBYTE(c[0], color[0]);
   CLAMPED_FLOAT_TO_UBYTE(c[1], color[1]);
   CLAMPED_FLOAT_TO_UBYTE(c[2], color[2]);
   CLAMPED_FLOAT_TO_UBYTE(c[3], color[3]);

   rmesa->ClearColor = r128PackColor( rmesa->r128Screen->cpp,
				      c[0], c[1], c[2], c[3] );
}

static void r128DDLogicOpCode( GLcontext *ctx, GLenum opcode )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   if ( ctx->Color.ColorLogicOpEnabled ) {
      FLUSH_BATCH( rmesa );

      FALLBACK( rmesa, R128_FALLBACK_LOGICOP, opcode != GL_COPY );
   }
}

static void r128DDDrawBuffer( GLcontext *ctx, GLenum mode )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   FLUSH_BATCH( rmesa );

   /*
    * _ColorDrawBufferMask is easier to cope with than <mode>.
    */
   switch ( ctx->DrawBuffer->_ColorDrawBufferMask[0] ) {
   case BUFFER_BIT_FRONT_LEFT:
   case BUFFER_BIT_BACK_LEFT:
      FALLBACK( rmesa, R128_FALLBACK_DRAW_BUFFER, GL_FALSE );
      break;
   default:
      /* GL_NONE or GL_FRONT_AND_BACK or stereo left&right, etc */
      FALLBACK( rmesa, R128_FALLBACK_DRAW_BUFFER, GL_TRUE );
      break;
   }

   rmesa->new_state |= R128_NEW_WINDOW;
}

static void r128DDReadBuffer( GLcontext *ctx, GLenum mode )
{
   /* nothing, until we implement h/w glRead/CopyPixels or CopyTexImage */
}


/* =============================================================
 * Polygon stipple
 */

static void r128DDPolygonStipple( GLcontext *ctx, const GLubyte *mask )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   GLuint stipple[32], i;
   drm_r128_stipple_t stippleRec;

   for (i = 0; i < 32; i++) {
      stipple[31 - i] = ((mask[i*4+0] << 24) |
                         (mask[i*4+1] << 16) |
                         (mask[i*4+2] << 8)  |
                         (mask[i*4+3]));
   }

   FLUSH_BATCH( rmesa );
   LOCK_HARDWARE( rmesa );

   stippleRec.mask = stipple;
   drmCommandWrite( rmesa->driFd, DRM_R128_STIPPLE, 
                    &stippleRec, sizeof(stippleRec) );

   UNLOCK_HARDWARE( rmesa );

   rmesa->new_state |= R128_NEW_CONTEXT;
   rmesa->dirty |= R128_UPLOAD_CONTEXT;
}


/* =============================================================
 * Render mode
 */

static void r128DDRenderMode( GLcontext *ctx, GLenum mode )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   FALLBACK( rmesa, R128_FALLBACK_RENDER_MODE, (mode != GL_RENDER) );
}



/* =============================================================
 * State enable/disable
 */

static void r128DDEnable( GLcontext *ctx, GLenum cap, GLboolean state )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   if ( R128_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s( %s = %s )\n",
	       __FUNCTION__, _mesa_lookup_enum_by_nr( cap ),
	       state ? "GL_TRUE" : "GL_FALSE" );
   }

   switch ( cap ) {
   case GL_ALPHA_TEST:
      FLUSH_BATCH( rmesa );
      rmesa->new_state |= R128_NEW_ALPHA;
      break;

   case GL_BLEND:
      FLUSH_BATCH( rmesa );
      rmesa->new_state |= R128_NEW_ALPHA;

      /* For some reason enable(GL_BLEND) affects ColorLogicOpEnabled.
       */
      FALLBACK( rmesa, R128_FALLBACK_LOGICOP,
		(ctx->Color.ColorLogicOpEnabled &&
		 ctx->Color.LogicOp != GL_COPY));
      break;

   case GL_CULL_FACE:
      FLUSH_BATCH( rmesa );
      rmesa->new_state |= R128_NEW_CULL;
      break;

   case GL_DEPTH_TEST:
      FLUSH_BATCH( rmesa );
      rmesa->new_state |= R128_NEW_DEPTH;
      break;

   case GL_DITHER:
      do {
	 GLuint t = rmesa->setup.tex_cntl_c;
	 FLUSH_BATCH( rmesa );

	 if ( ctx->Color.DitherFlag ) {
	    t |=  R128_DITHER_ENABLE;
	 } else {
	    t &= ~R128_DITHER_ENABLE;
	 }

	 if ( rmesa->setup.tex_cntl_c != t ) {
	    rmesa->setup.tex_cntl_c = t;
	    rmesa->dirty |= R128_UPLOAD_CONTEXT;
	 }
      } while (0);
      break;

   case GL_FOG:
      FLUSH_BATCH( rmesa );
      rmesa->new_state |= R128_NEW_FOG;
      break;

   case GL_COLOR_LOGIC_OP:
      FLUSH_BATCH( rmesa );
      FALLBACK( rmesa, R128_FALLBACK_LOGICOP,
		state && ctx->Color.LogicOp != GL_COPY );
      break;

   case GL_LIGHTING:
   case GL_COLOR_SUM_EXT:
      updateSpecularLighting(ctx);
      break;

   case GL_SCISSOR_TEST:
      FLUSH_BATCH( rmesa );
      rmesa->scissor = state;
      rmesa->new_state |= R128_NEW_CLIP;
      break;

   case GL_STENCIL_TEST:
      FLUSH_BATCH( rmesa );
      if ( ctx->Visual.stencilBits > 0 && ctx->Visual.depthBits == 24 ) {
	 if ( state ) {
	    rmesa->setup.tex_cntl_c |=  R128_STENCIL_ENABLE;
	    /* Reset the fallback (if any) for bad stencil funcs */
	    r128DDStencilOpSeparate( ctx, 0, ctx->Stencil.FailFunc[0],
				     ctx->Stencil.ZFailFunc[0],
				     ctx->Stencil.ZPassFunc[0] );
	 } else {
	    rmesa->setup.tex_cntl_c &= ~R128_STENCIL_ENABLE;
	    FALLBACK( rmesa, R128_FALLBACK_STENCIL, GL_FALSE );
	 }
	 rmesa->dirty |= R128_UPLOAD_CONTEXT;
      } else {
	 FALLBACK( rmesa, R128_FALLBACK_STENCIL, state );
      }
      break;

   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
      FLUSH_BATCH( rmesa );
      break;

   case GL_POLYGON_STIPPLE:
      if ( rmesa->render_primitive == GL_TRIANGLES ) {
	 FLUSH_BATCH( rmesa );
	 rmesa->setup.dp_gui_master_cntl_c &= ~R128_GMC_BRUSH_NONE;
	 if ( state ) {
	    rmesa->setup.dp_gui_master_cntl_c |=
	       R128_GMC_BRUSH_32x32_MONO_FG_LA;
	 } else {
	    rmesa->setup.dp_gui_master_cntl_c |=
	       R128_GMC_BRUSH_SOLID_COLOR;
	 }
	 rmesa->new_state |= R128_NEW_CONTEXT;
	 rmesa->dirty |= R128_UPLOAD_CONTEXT;
      }
      break;

   default:
      return;
   }
}


/* =============================================================
 * State initialization, management
 */

static void r128DDPrintDirty( const char *msg, GLuint state )
{
   fprintf( stderr,
	    "%s: (0x%x) %s%s%s%s%s%s%s%s%s\n",
	    msg,
	    state,
	    (state & R128_UPLOAD_CORE)		? "core, " : "",
	    (state & R128_UPLOAD_CONTEXT)	? "context, " : "",
	    (state & R128_UPLOAD_SETUP)		? "setup, " : "",
	    (state & R128_UPLOAD_TEX0)		? "tex0, " : "",
	    (state & R128_UPLOAD_TEX1)		? "tex1, " : "",
	    (state & R128_UPLOAD_MASKS)		? "masks, " : "",
	    (state & R128_UPLOAD_WINDOW)	? "window, " : "",
	    (state & R128_UPLOAD_CLIPRECTS)	? "cliprects, " : "",
	    (state & R128_REQUIRE_QUIESCENCE)	? "quiescence, " : "" );
}

/*
 * Load the current context's state into the hardware.
 *
 * NOTE: Be VERY careful about ensuring the context state is marked for
 * upload, the only place it shouldn't be uploaded is when the setup
 * state has changed in ReducedPrimitiveChange as this comes right after
 * a state update.
 *
 * Blits of any type should always upload the context and masks after
 * they are done.
 */
void r128EmitHwStateLocked( r128ContextPtr rmesa )
{
   drm_r128_sarea_t *sarea = rmesa->sarea;
   drm_r128_context_regs_t *regs = &(rmesa->setup);
   const r128TexObjPtr t0 = rmesa->CurrentTexObj[0];
   const r128TexObjPtr t1 = rmesa->CurrentTexObj[1];

   if ( R128_DEBUG & DEBUG_VERBOSE_MSG ) {
      r128DDPrintDirty( "r128EmitHwStateLocked", rmesa->dirty );
   }

   if ( rmesa->dirty & (R128_UPLOAD_CONTEXT |
			R128_UPLOAD_SETUP |
			R128_UPLOAD_MASKS |
			R128_UPLOAD_WINDOW |
			R128_UPLOAD_CORE) ) {
      memcpy( &sarea->context_state, regs, sizeof(sarea->context_state) );
      
      if( rmesa->dirty & R128_UPLOAD_CONTEXT )
      {
         /* One possible side-effect of uploading a new context is the
          * setting of the R128_GMC_AUX_CLIP_DIS bit, which causes all
          * auxilliary cliprects to be disabled. So the next command must
          * upload them again. */
         rmesa->dirty |= R128_UPLOAD_CLIPRECTS;
      }
   }

   if ( (rmesa->dirty & R128_UPLOAD_TEX0) && t0 ) {
      drm_r128_texture_regs_t *tex = &sarea->tex_state[0];

      tex->tex_cntl		= t0->setup.tex_cntl;
      tex->tex_combine_cntl	= rmesa->tex_combine[0];
      tex->tex_size_pitch	= t0->setup.tex_size_pitch;
      memcpy( &tex->tex_offset[0], &t0->setup.tex_offset[0],
	      sizeof(tex->tex_offset ) );
      tex->tex_border_color	= t0->setup.tex_border_color;
   }

   if ( (rmesa->dirty & R128_UPLOAD_TEX1) && t1 ) {
      drm_r128_texture_regs_t *tex = &sarea->tex_state[1];

      tex->tex_cntl		= t1->setup.tex_cntl;
      tex->tex_combine_cntl	= rmesa->tex_combine[1];
      tex->tex_size_pitch	= t1->setup.tex_size_pitch;
      memcpy( &tex->tex_offset[0], &t1->setup.tex_offset[0],
	      sizeof(tex->tex_offset ) );
      tex->tex_border_color	= t1->setup.tex_border_color;
   }

   sarea->vertsize = rmesa->vertex_size;
   sarea->vc_format = rmesa->vertex_format;

   /* Turn off the texture cache flushing */
   rmesa->setup.tex_cntl_c &= ~R128_TEX_CACHE_FLUSH;

   sarea->dirty |= rmesa->dirty;
   rmesa->dirty &= R128_UPLOAD_CLIPRECTS;
}

static void r128DDPrintState( const char *msg, GLuint flags )
{
   fprintf( stderr,
	    "%s: (0x%x) %s%s%s%s%s%s%s%s\n",
	    msg,
	    flags,
	    (flags & R128_NEW_CONTEXT)	? "context, " : "",
	    (flags & R128_NEW_ALPHA)	? "alpha, " : "",
	    (flags & R128_NEW_DEPTH)	? "depth, " : "",
	    (flags & R128_NEW_FOG)	? "fog, " : "",
	    (flags & R128_NEW_CLIP)	? "clip, " : "",
	    (flags & R128_NEW_CULL)	? "cull, " : "",
	    (flags & R128_NEW_MASKS)	? "masks, " : "",
	    (flags & R128_NEW_WINDOW)	? "window, " : "" );
}

void r128DDUpdateHWState( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   int new_state = rmesa->new_state;

   if ( new_state || rmesa->NewGLState & _NEW_TEXTURE )
   {
      FLUSH_BATCH( rmesa );

      rmesa->new_state = 0;

      if ( R128_DEBUG & DEBUG_VERBOSE_MSG )
	 r128DDPrintState( "r128UpdateHwState", new_state );

      /* Update the various parts of the context's state.
       */
      if ( new_state & R128_NEW_ALPHA )
	 r128UpdateAlphaMode( ctx );

      if ( new_state & R128_NEW_DEPTH )
	 r128UpdateZMode( ctx );

      if ( new_state & R128_NEW_FOG )
	 r128UpdateFogAttrib( ctx );

      if ( new_state & R128_NEW_CLIP )
	 r128UpdateClipping( ctx );

      if ( new_state & R128_NEW_CULL )
	 r128UpdateCull( ctx );

      if ( new_state & R128_NEW_MASKS )
	 r128UpdateMasks( ctx );

      if ( new_state & R128_NEW_WINDOW )
      {
	 r128UpdateWindow( ctx );
	 r128CalcViewport( ctx );
      }

      if ( rmesa->NewGLState & _NEW_TEXTURE ) {
	 r128UpdateTextureState( ctx );
      }
   }
}


static void r128DDInvalidateState( GLcontext *ctx, GLuint new_state )
{
   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _vbo_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   R128_CONTEXT(ctx)->NewGLState |= new_state;
}



/* Initialize the context's hardware state.
 */
void r128DDInitState( r128ContextPtr rmesa )
{
   int dst_bpp, depth_bpp;

   switch ( rmesa->r128Screen->cpp ) {
   case 2:
      dst_bpp = R128_GMC_DST_16BPP;
      break;
   case 4:
      dst_bpp = R128_GMC_DST_32BPP;
      break;
   default:
      fprintf( stderr, "Error: Unsupported pixel depth... exiting\n" );
      exit( -1 );
   }

   rmesa->ClearColor = 0x00000000;

   switch ( rmesa->glCtx->Visual.depthBits ) {
   case 16:
      rmesa->ClearDepth = 0x0000ffff;
      depth_bpp = R128_Z_PIX_WIDTH_16;
      rmesa->depth_scale = 1.0 / (GLfloat)0xffff;
      break;
   case 24:
      rmesa->ClearDepth = 0x00ffffff;
      depth_bpp = R128_Z_PIX_WIDTH_24;
      rmesa->depth_scale = 1.0 / (GLfloat)0xffffff;
      break;
   default:
      fprintf( stderr, "Error: Unsupported depth %d... exiting\n",
	       rmesa->glCtx->Visual.depthBits );
      exit( -1 );
   }

   rmesa->Fallback = 0;

   /* Hardware state:
    */
   rmesa->setup.dp_gui_master_cntl_c = (R128_GMC_DST_PITCH_OFFSET_CNTL |
					R128_GMC_DST_CLIPPING |
					R128_GMC_BRUSH_SOLID_COLOR |
					dst_bpp |
					R128_GMC_SRC_DATATYPE_COLOR |
					R128_GMC_BYTE_MSB_TO_LSB |
					R128_GMC_CONVERSION_TEMP_6500 |
					R128_ROP3_S |
					R128_DP_SRC_SOURCE_MEMORY |
					R128_GMC_3D_FCN_EN |
					R128_GMC_CLR_CMP_CNTL_DIS |
					R128_GMC_AUX_CLIP_DIS |
					R128_GMC_WR_MSK_DIS);

   rmesa->setup.sc_top_left_c     = 0x00000000;
   rmesa->setup.sc_bottom_right_c = 0x1fff1fff;

   rmesa->setup.z_offset_c = rmesa->r128Screen->depthOffset;
   rmesa->setup.z_pitch_c = ((rmesa->r128Screen->depthPitch >> 3) |
			     R128_Z_TILE);

   rmesa->setup.z_sten_cntl_c = (depth_bpp |
				 R128_Z_TEST_LESS |
				 R128_STENCIL_TEST_ALWAYS |
				 R128_STENCIL_S_FAIL_KEEP |
				 R128_STENCIL_ZPASS_KEEP |
				 R128_STENCIL_ZFAIL_KEEP);

   rmesa->setup.tex_cntl_c = (R128_Z_WRITE_ENABLE |
			      R128_SHADE_ENABLE |
			      R128_DITHER_ENABLE |
			      R128_ALPHA_IN_TEX_COMPLETE_A |
			      R128_LIGHT_DIS |
			      R128_ALPHA_LIGHT_DIS |
			      R128_TEX_CACHE_FLUSH |
			      (0x3f << R128_LOD_BIAS_SHIFT));

   rmesa->setup.misc_3d_state_cntl_reg = (R128_MISC_SCALE_3D_TEXMAP_SHADE |
					  R128_MISC_SCALE_PIX_REPLICATE |
					  R128_ALPHA_COMB_ADD_CLAMP |
					  R128_FOG_VERTEX |
					  (R128_ALPHA_BLEND_ONE << R128_ALPHA_BLEND_SRC_SHIFT) |
					  (R128_ALPHA_BLEND_ZERO << R128_ALPHA_BLEND_DST_SHIFT) |
					  R128_ALPHA_TEST_ALWAYS);

   rmesa->setup.texture_clr_cmp_clr_c = 0x00000000;
   rmesa->setup.texture_clr_cmp_msk_c = 0xffffffff;

   rmesa->setup.fog_color_c = 0x00000000;

   rmesa->setup.pm4_vc_fpu_setup = (R128_FRONT_DIR_CCW |
				    R128_BACKFACE_SOLID |
				    R128_FRONTFACE_SOLID |
				    R128_FPU_COLOR_GOURAUD |
				    R128_FPU_SUB_PIX_4BITS |
				    R128_FPU_MODE_3D |
				    R128_TRAP_BITS_DISABLE |
				    R128_XFACTOR_2 |
				    R128_YFACTOR_2 |
				    R128_FLAT_SHADE_VERTEX_OGL |
				    R128_FPU_ROUND_TRUNCATE |
				    R128_WM_SEL_8DW);

   rmesa->setup.setup_cntl = (R128_COLOR_GOURAUD |
			      R128_PRIM_TYPE_TRI |
			      R128_TEXTURE_ST_MULT_W |
			      R128_STARTING_VERTEX_1 |
			      R128_ENDING_VERTEX_3 |
			      R128_SU_POLY_LINE_NOT_LAST |
			      R128_SUB_PIX_4BITS);

   rmesa->setup.tex_size_pitch_c = 0x00000000;
   rmesa->setup.constant_color_c = 0x00ffffff;

   rmesa->setup.dp_write_mask   = 0xffffffff;
   rmesa->setup.sten_ref_mask_c = 0xffff0000;
   rmesa->setup.plane_3d_mask_c = 0xffffffff;

   rmesa->setup.window_xy_offset = 0x00000000;

   rmesa->setup.scale_3d_cntl = (R128_SCALE_DITHER_TABLE |
				 R128_TEX_CACHE_SIZE_FULL |
				 R128_DITHER_INIT_RESET |
				 R128_SCALE_3D_TEXMAP_SHADE |
				 R128_SCALE_PIX_REPLICATE |
				 R128_ALPHA_COMB_ADD_CLAMP |
				 R128_FOG_VERTEX |
				 (R128_ALPHA_BLEND_ONE << R128_ALPHA_BLEND_SRC_SHIFT) |
				 (R128_ALPHA_BLEND_ZERO << R128_ALPHA_BLEND_DST_SHIFT) |
				 R128_ALPHA_TEST_ALWAYS |
				 R128_COMPOSITE_SHADOW_CMP_EQUAL |
				 R128_TEX_MAP_ALPHA_IN_TEXTURE |
				 R128_TEX_CACHE_LINE_SIZE_4QW);

   rmesa->new_state = R128_NEW_ALL;
}

/* Initialize the driver's state functions.
 */
void r128DDInitStateFuncs( GLcontext *ctx )
{
   ctx->Driver.UpdateState		= r128DDInvalidateState;

   ctx->Driver.ClearIndex		= NULL;
   ctx->Driver.ClearColor		= r128DDClearColor;
   ctx->Driver.ClearStencil		= r128DDClearStencil;
   ctx->Driver.DrawBuffer		= r128DDDrawBuffer;
   ctx->Driver.ReadBuffer		= r128DDReadBuffer;

   ctx->Driver.IndexMask		= NULL;
   ctx->Driver.ColorMask		= r128DDColorMask;
   ctx->Driver.AlphaFunc		= r128DDAlphaFunc;
   ctx->Driver.BlendEquationSeparate	= r128DDBlendEquationSeparate;
   ctx->Driver.BlendFuncSeparate	= r128DDBlendFuncSeparate;
   ctx->Driver.ClearDepth		= r128DDClearDepth;
   ctx->Driver.CullFace			= r128DDCullFace;
   ctx->Driver.FrontFace		= r128DDFrontFace;
   ctx->Driver.DepthFunc		= r128DDDepthFunc;
   ctx->Driver.DepthMask		= r128DDDepthMask;
   ctx->Driver.Enable			= r128DDEnable;
   ctx->Driver.Fogfv			= r128DDFogfv;
   ctx->Driver.Hint			= NULL;
   ctx->Driver.Lightfv			= NULL;
   ctx->Driver.LightModelfv		= r128DDLightModelfv;
   ctx->Driver.LogicOpcode		= r128DDLogicOpCode;
   ctx->Driver.PolygonMode		= NULL;
   ctx->Driver.PolygonStipple		= r128DDPolygonStipple;
   ctx->Driver.RenderMode		= r128DDRenderMode;
   ctx->Driver.Scissor			= r128DDScissor;
   ctx->Driver.ShadeModel		= r128DDShadeModel;
   ctx->Driver.StencilFuncSeparate	= r128DDStencilFuncSeparate;
   ctx->Driver.StencilMaskSeparate	= r128DDStencilMaskSeparate;
   ctx->Driver.StencilOpSeparate	= r128DDStencilOpSeparate;

   ctx->Driver.DepthRange               = r128DepthRange;
   ctx->Driver.Viewport                 = r128Viewport;
}
