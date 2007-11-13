/* $XFree86$ */ /* -*- mode: c; c-basic-offset: 3 -*- */
/*
 * Copyright 2000 Gareth Hughes
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * GARETH HUGHES BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *	Leif Delgass <ldelgass@retinalburn.net>
 *	Jos�Fonseca <j_r_fonseca@yahoo.co.uk>
 */

#include "mach64_context.h"
#include "mach64_state.h"
#include "mach64_ioctl.h"
#include "mach64_tris.h"
#include "mach64_vb.h"
#include "mach64_tex.h"

#include "context.h"
#include "enums.h"
#include "colormac.h"
#include "swrast/swrast.h"
#include "vbo/vbo.h"
#include "tnl/tnl.h"
#include "swrast_setup/swrast_setup.h"

#include "tnl/t_pipeline.h"


/* =============================================================
 * Alpha blending
 */

static void mach64UpdateAlphaMode( GLcontext *ctx )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   GLuint a = mmesa->setup.alpha_tst_cntl;
   GLuint s = mmesa->setup.scale_3d_cntl;
   GLuint m = mmesa->setup.dp_write_mask;

   if ( ctx->Color.AlphaEnabled ) {
      GLubyte ref;

      CLAMPED_FLOAT_TO_UBYTE(ref, ctx->Color.AlphaRef);

      a &= ~(MACH64_ALPHA_TEST_MASK | MACH64_REF_ALPHA_MASK);

      switch ( ctx->Color.AlphaFunc ) {
      case GL_NEVER:
	 a |= MACH64_ALPHA_TEST_NEVER;
	 break;
      case GL_LESS:
	 a |= MACH64_ALPHA_TEST_LESS;
         break;
      case GL_LEQUAL:
	 a |= MACH64_ALPHA_TEST_LEQUAL;
	 break;
      case GL_EQUAL:
	 a |= MACH64_ALPHA_TEST_EQUAL;
	 break;
      case GL_GEQUAL:
	 a |= MACH64_ALPHA_TEST_GEQUAL;
	 break;
      case GL_GREATER:
	 a |= MACH64_ALPHA_TEST_GREATER;
	 break;
      case GL_NOTEQUAL:
	 a |= MACH64_ALPHA_TEST_NOTEQUAL;
	 break;
      case GL_ALWAYS:
	 a |= MACH64_ALPHA_TEST_ALWAYS;
	 break;
      }

      a |= (ref << MACH64_REF_ALPHA_SHIFT);
      a |=  MACH64_ALPHA_TEST_EN;
   } else {
      a &= ~MACH64_ALPHA_TEST_EN;
   }

   FALLBACK( mmesa, MACH64_FALLBACK_BLEND_FUNC, GL_FALSE );

   if ( ctx->Color.BlendEnabled ) {
      s &= ~(MACH64_ALPHA_BLEND_SRC_MASK |
	     MACH64_ALPHA_BLEND_DST_MASK |
	     MACH64_ALPHA_BLEND_SAT);

      switch ( ctx->Color.BlendSrcRGB ) {
      case GL_ZERO:
	 s |= MACH64_ALPHA_BLEND_SRC_ZERO;
	 break;
      case GL_ONE:
	 s |= MACH64_ALPHA_BLEND_SRC_ONE;
	 break;
      case GL_DST_COLOR:
	 s |= MACH64_ALPHA_BLEND_SRC_DSTCOLOR;
	 break;
      case GL_ONE_MINUS_DST_COLOR:
	 s |= MACH64_ALPHA_BLEND_SRC_INVDSTCOLOR;
	 break;
      case GL_SRC_ALPHA:
	 s |= MACH64_ALPHA_BLEND_SRC_SRCALPHA;
	 break;
      case GL_ONE_MINUS_SRC_ALPHA:
	 s |= MACH64_ALPHA_BLEND_SRC_INVSRCALPHA;
	 break;
      case GL_DST_ALPHA:
	 s |= MACH64_ALPHA_BLEND_SRC_DSTALPHA;
	 break;
      case GL_ONE_MINUS_DST_ALPHA:
	 s |= MACH64_ALPHA_BLEND_SRC_INVDSTALPHA;
	 break;
      case GL_SRC_ALPHA_SATURATE:
	 s |= (MACH64_ALPHA_BLEND_SRC_SRCALPHA |
	       MACH64_ALPHA_BLEND_SAT);
	 break;
      default:
         FALLBACK( mmesa, MACH64_FALLBACK_BLEND_FUNC, GL_TRUE );
      }

      switch ( ctx->Color.BlendDstRGB ) {
      case GL_ZERO:
	 s |= MACH64_ALPHA_BLEND_DST_ZERO;
	 break;
      case GL_ONE:
	 s |= MACH64_ALPHA_BLEND_DST_ONE;
	 break;
      case GL_SRC_COLOR:
	 s |= MACH64_ALPHA_BLEND_DST_SRCCOLOR;
	 break;
      case GL_ONE_MINUS_SRC_COLOR:
	 s |= MACH64_ALPHA_BLEND_DST_INVSRCCOLOR;
	 break;
      case GL_SRC_ALPHA:
	 s |= MACH64_ALPHA_BLEND_DST_SRCALPHA;
	 break;
      case GL_ONE_MINUS_SRC_ALPHA:
	 s |= MACH64_ALPHA_BLEND_DST_INVSRCALPHA;
	 break;
      case GL_DST_ALPHA:
	 s |= MACH64_ALPHA_BLEND_DST_DSTALPHA;
	 break;
      case GL_ONE_MINUS_DST_ALPHA:
	 s |= MACH64_ALPHA_BLEND_DST_INVDSTALPHA;
	 break;
      default:
         FALLBACK( mmesa, MACH64_FALLBACK_BLEND_FUNC, GL_TRUE );
      }

      m = 0xffffffff; /* Can't color mask and blend at the same time */
      s &= ~MACH64_ALPHA_FOG_EN_FOG; /* Can't fog and blend at the same time */
      s |=  MACH64_ALPHA_FOG_EN_ALPHA;
   } else {
      s &= ~MACH64_ALPHA_FOG_EN_ALPHA;
   }

   if ( mmesa->setup.alpha_tst_cntl != a ) {
      mmesa->setup.alpha_tst_cntl = a;
      mmesa->dirty |= MACH64_UPLOAD_Z_ALPHA_CNTL;
   }
   if ( mmesa->setup.scale_3d_cntl != s ) {
      mmesa->setup.scale_3d_cntl = s;
      mmesa->dirty |= MACH64_UPLOAD_SCALE_3D_CNTL;
   }
   if ( mmesa->setup.dp_write_mask != m ) {
      mmesa->setup.dp_write_mask = m;
      mmesa->dirty |= MACH64_UPLOAD_DP_WRITE_MASK;
   }
}

static void mach64DDAlphaFunc( GLcontext *ctx, GLenum func, GLfloat ref )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);

   FLUSH_BATCH( mmesa );
   mmesa->new_state |= MACH64_NEW_ALPHA;
}

static void mach64DDBlendEquationSeparate( GLcontext *ctx, 
					   GLenum modeRGB, GLenum modeA )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);

   assert( modeRGB == modeA );
   FLUSH_BATCH( mmesa );

   /* BlendEquation affects ColorLogicOpEnabled
    */
   FALLBACK( MACH64_CONTEXT(ctx), MACH64_FALLBACK_LOGICOP,
	     (ctx->Color.ColorLogicOpEnabled &&
	      ctx->Color.LogicOp != GL_COPY));

   /* Can only do blend addition, not min, max, subtract, etc. */
   FALLBACK( MACH64_CONTEXT(ctx), MACH64_FALLBACK_BLEND_EQ,
	     modeRGB != GL_FUNC_ADD);

   mmesa->new_state |= MACH64_NEW_ALPHA;
}

static void mach64DDBlendFuncSeparate( GLcontext *ctx,
				       GLenum sfactorRGB, GLenum dfactorRGB,
				       GLenum sfactorA, GLenum dfactorA )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);

   FLUSH_BATCH( mmesa );
   mmesa->new_state |= MACH64_NEW_ALPHA;
}


/* =============================================================
 * Depth testing
 */

static void mach64UpdateZMode( GLcontext *ctx )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   GLuint z = mmesa->setup.z_cntl;

   if ( MACH64_DEBUG & DEBUG_VERBOSE_MSG ) {
      fprintf( stderr, "%s:\n", __FUNCTION__ );
   }

   if ( ctx->Depth.Test ) {
      z &= ~MACH64_Z_TEST_MASK;

      switch ( ctx->Depth.Func ) {
      case GL_NEVER:
	 z |= MACH64_Z_TEST_NEVER;
	 break;
      case GL_ALWAYS:
	 z |= MACH64_Z_TEST_ALWAYS;
	 break;
      case GL_LESS:
	 z |= MACH64_Z_TEST_LESS;
	 break;
      case GL_LEQUAL:
	 z |= MACH64_Z_TEST_LEQUAL;
	 break;
      case GL_EQUAL:
	 z |= MACH64_Z_TEST_EQUAL;
	 break;
      case GL_GEQUAL:
	 z |= MACH64_Z_TEST_GEQUAL;
	 break;
      case GL_GREATER:
	 z |= MACH64_Z_TEST_GREATER;
	 break;
      case GL_NOTEQUAL:
	 z |= MACH64_Z_TEST_NOTEQUAL;
	 break;
      }

      z |=  MACH64_Z_EN;
   } else {
      z &= ~MACH64_Z_EN;
   }

   if ( ctx->Depth.Mask ) {
      z |=  MACH64_Z_MASK_EN;
   } else {
      z &= ~MACH64_Z_MASK_EN;
   }

   if ( mmesa->setup.z_cntl != z ) {
      mmesa->setup.z_cntl = z;
      mmesa->dirty |= MACH64_UPLOAD_Z_ALPHA_CNTL;
   }
}

static void mach64DDDepthFunc( GLcontext *ctx, GLenum func )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);

   FLUSH_BATCH( mmesa );
   mmesa->new_state |= MACH64_NEW_DEPTH;
}

static void mach64DDDepthMask( GLcontext *ctx, GLboolean flag )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);

   FLUSH_BATCH( mmesa );
   mmesa->new_state |= MACH64_NEW_DEPTH;
}

static void mach64DDClearDepth( GLcontext *ctx, GLclampd d )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);

   /* Always have a 16-bit depth buffer.
    */
   mmesa->ClearDepth = d * 0xffff;
}


/* =============================================================
 * Fog
 */

static void mach64UpdateFogAttrib( GLcontext *ctx )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);

   CARD32 s = mmesa->setup.scale_3d_cntl;
   GLubyte c[4];
   CARD32 col;

   /* Can't fog if blending is on */
   if ( ctx->Color.BlendEnabled )
      return;

   if ( ctx->Fog.Enabled ) {
      s |= MACH64_ALPHA_FOG_EN_FOG;
      s &= ~(MACH64_ALPHA_BLEND_SRC_MASK |
	     MACH64_ALPHA_BLEND_DST_MASK |
	     MACH64_ALPHA_BLEND_SAT);
      /* From Utah-glx: "fog color is now dest and fog factor is alpha, so
       * use GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA"
       */
      s |= (MACH64_ALPHA_BLEND_SRC_SRCALPHA | 
	    MACH64_ALPHA_BLEND_DST_INVSRCALPHA);
      /* From Utah-glx: "can't use texture alpha when fogging" */
      s &= ~MACH64_TEX_MAP_AEN;
   } else {
      s &= ~(MACH64_ALPHA_BLEND_SRC_MASK |
	     MACH64_ALPHA_BLEND_DST_MASK |
	     MACH64_ALPHA_BLEND_SAT);
      s |= (MACH64_ALPHA_BLEND_SRC_ONE | 
	    MACH64_ALPHA_BLEND_DST_ZERO);
      s &= ~MACH64_ALPHA_FOG_EN_FOG;
   }

   c[0] = FLOAT_TO_UBYTE( ctx->Fog.Color[0] );
   c[1] = FLOAT_TO_UBYTE( ctx->Fog.Color[1] );
   c[2] = FLOAT_TO_UBYTE( ctx->Fog.Color[2] );
   c[3] = FLOAT_TO_UBYTE( ctx->Fog.Color[3] );

   col = mach64PackColor( 4, c[0], c[1], c[2], c[3] );

   if ( mmesa->setup.dp_fog_clr != col ) {
      mmesa->setup.dp_fog_clr = col;
      mmesa->dirty |= MACH64_UPLOAD_DP_FOG_CLR;
   }
   if ( mmesa->setup.scale_3d_cntl != s ) {
      mmesa->setup.scale_3d_cntl = s;
      mmesa->dirty |= MACH64_UPLOAD_SCALE_3D_CNTL;
   }

}

static void mach64DDFogfv( GLcontext *ctx, GLenum pname, const GLfloat *param )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);

   FLUSH_BATCH( mmesa );
   mmesa->new_state |= MACH64_NEW_FOG;
}


/* =============================================================
 * Clipping
 */

static void mach64UpdateClipping( GLcontext *ctx )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   mach64ScreenPtr mach64Screen = mmesa->mach64Screen;

   if ( mmesa->driDrawable ) {
      __DRIdrawablePrivate *drawable = mmesa->driDrawable;
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

      /* clamp to screen borders */
      if (x1 < 0) x1 = 0;
      if (y1 < 0) y1 = 0;
      if (x2 < 0) x2 = 0;
      if (y2 < 0) y2 = 0;
      if (x2 > mach64Screen->width-1) x2 = mach64Screen->width-1;
      if (y2 > mach64Screen->height-1) y2 = mach64Screen->height-1;

      if ( MACH64_DEBUG & DEBUG_VERBOSE_MSG ) {
	 fprintf( stderr, "%s: drawable %3d %3d %3d %3d\n",
		  __FUNCTION__,
		  drawable->x,
		  drawable->y,
		  drawable->w,
		  drawable->h );
	 fprintf( stderr, "%s:  scissor %3d %3d %3d %3d\n",
		  __FUNCTION__,
		  ctx->Scissor.X,
		  ctx->Scissor.Y,
		  ctx->Scissor.Width,
		  ctx->Scissor.Height );
	 fprintf( stderr, "%s:    final %3d %3d %3d %3d\n",
		  __FUNCTION__, x1, y1, x2, y2 );
	 fprintf( stderr, "\n" );
      }

      mmesa->setup.sc_top_bottom = ((y1 << 0) |
				    (y2 << 16));

      mmesa->setup.sc_left_right = ((x1 << 0) |
				    (x2 << 16));

       /* UPLOAD_MISC reduces the dirty state, we just need to
       * emit the scissor to the SAREA.  We need to dirty cliprects
       * since the scissor and cliprects are intersected to update the
       * single hardware scissor
       */
      mmesa->dirty |= MACH64_UPLOAD_MISC | MACH64_UPLOAD_CLIPRECTS;
   }
}

static void mach64DDScissor( GLcontext *ctx,
			     GLint x, GLint y, GLsizei w, GLsizei h )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);

   FLUSH_BATCH( mmesa );
   mmesa->new_state |= MACH64_NEW_CLIP;
}


/* =============================================================
 * Culling
 */

static void mach64UpdateCull( GLcontext *ctx )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   GLfloat backface_sign = 1;

   if ( ctx->Polygon.CullFlag /*&& ctx->PB->primitive == GL_POLYGON*/ ) {
      backface_sign = 1;
      switch ( ctx->Polygon.CullFaceMode ) {
      case GL_BACK:
	 if ( ctx->Polygon.FrontFace == GL_CCW )
	    backface_sign = -1;
	 break;
      case GL_FRONT:
	 if ( ctx->Polygon.FrontFace != GL_CCW )
	    backface_sign = -1;
	 break;
      default:
      case GL_FRONT_AND_BACK:
	 backface_sign = 0;
	 break;
      }
   } else {
      backface_sign = 0;
   }

   mmesa->backface_sign = backface_sign;

}

static void mach64DDCullFace( GLcontext *ctx, GLenum mode )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);

   FLUSH_BATCH( mmesa );
   mmesa->new_state |= MACH64_NEW_CULL;
}

static void mach64DDFrontFace( GLcontext *ctx, GLenum mode )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);

   FLUSH_BATCH( mmesa );
   mmesa->new_state |= MACH64_NEW_CULL;
}


/* =============================================================
 * Masks
 */

static void mach64UpdateMasks( GLcontext *ctx )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   GLuint mask = 0xffffffff;

   /* mach64 can't color mask with alpha blending enabled */
   if ( !ctx->Color.BlendEnabled ) {
      mask = mach64PackColor( mmesa->mach64Screen->cpp,
			      ctx->Color.ColorMask[RCOMP],
			      ctx->Color.ColorMask[GCOMP],
			      ctx->Color.ColorMask[BCOMP],
			      ctx->Color.ColorMask[ACOMP] );
   }

   if ( mmesa->setup.dp_write_mask != mask ) {
      mmesa->setup.dp_write_mask = mask;
      mmesa->dirty |= MACH64_UPLOAD_DP_WRITE_MASK;
   }
}

static void mach64DDColorMask( GLcontext *ctx,
			       GLboolean r, GLboolean g,
			       GLboolean b, GLboolean a )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);

   FLUSH_BATCH( mmesa );
   mmesa->new_state |= MACH64_NEW_MASKS;
}


/* =============================================================
 * Rendering attributes
 *
 * We really don't want to recalculate all this every time we bind a
 * texture.  These things shouldn't change all that often, so it makes
 * sense to break them out of the core texture state update routines.
 */

static void mach64UpdateSpecularLighting( GLcontext *ctx )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   GLuint a = mmesa->setup.alpha_tst_cntl;

   if ( MACH64_DEBUG & DEBUG_VERBOSE_MSG ) {
      fprintf( stderr, "%s:\n", __FUNCTION__ );
   }

   if ( ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR  &&
        ctx->Light.Enabled ) {
      a |=  MACH64_SPECULAR_LIGHT_EN;
   } else {
      a &= ~MACH64_SPECULAR_LIGHT_EN;
   }

   if ( mmesa->setup.alpha_tst_cntl != a ) {
      mmesa->setup.alpha_tst_cntl = a;
      mmesa->dirty |= MACH64_UPLOAD_Z_ALPHA_CNTL;
      mmesa->new_state |= MACH64_NEW_CONTEXT;
   }
}

static void mach64DDLightModelfv( GLcontext *ctx, GLenum pname,
				  const GLfloat *param )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);

   if ( pname == GL_LIGHT_MODEL_COLOR_CONTROL ) {
      FLUSH_BATCH( mmesa );
      mach64UpdateSpecularLighting(ctx);
   }
}

static void mach64DDShadeModel( GLcontext *ctx, GLenum mode )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   GLuint s = mmesa->setup.setup_cntl;

   s &= ~MACH64_FLAT_SHADE_MASK;

   switch ( mode ) {
   case GL_FLAT:
      s |= MACH64_FLAT_SHADE_VERTEX_3;
      break;
   case GL_SMOOTH:
      s |= MACH64_FLAT_SHADE_OFF;
      break;
   default:
      return;
   }

   if ( mmesa->setup.setup_cntl != s ) {
      FLUSH_BATCH( mmesa );
      mmesa->setup.setup_cntl = s;

      mmesa->dirty |= MACH64_UPLOAD_SETUP_CNTL;
   }
}


/* =============================================================
 * Viewport
 */


void mach64CalcViewport( GLcontext *ctx )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   const GLfloat *v = ctx->Viewport._WindowMap.m;
   GLfloat *m = mmesa->hw_viewport;

   /* See also mach64_translate_vertex.
    */
   m[MAT_SX] =   v[MAT_SX];
   m[MAT_TX] =   v[MAT_TX] + (GLfloat)mmesa->drawX + SUBPIXEL_X;
   m[MAT_SY] = - v[MAT_SY];
   m[MAT_TY] = - v[MAT_TY] + mmesa->driDrawable->h + (GLfloat)mmesa->drawY + SUBPIXEL_Y;
   m[MAT_SZ] =   v[MAT_SZ] * mmesa->depth_scale;
   m[MAT_TZ] =   v[MAT_TZ] * mmesa->depth_scale;

   mmesa->SetupNewInputs = ~0;
}

static void mach64Viewport( GLcontext *ctx,
			  GLint x, GLint y,
			  GLsizei width, GLsizei height )
{
   mach64CalcViewport( ctx );
}

static void mach64DepthRange( GLcontext *ctx,
			    GLclampd nearval, GLclampd farval )
{
   mach64CalcViewport( ctx );
}


/* =============================================================
 * Miscellaneous
 */

static void mach64DDClearColor( GLcontext *ctx,
				const GLfloat color[4] )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   GLubyte c[4];
   
   CLAMPED_FLOAT_TO_UBYTE(c[0], color[0]);
   CLAMPED_FLOAT_TO_UBYTE(c[1], color[1]);
   CLAMPED_FLOAT_TO_UBYTE(c[2], color[2]);
   CLAMPED_FLOAT_TO_UBYTE(c[3], color[3]);

   mmesa->ClearColor = mach64PackColor( mmesa->mach64Screen->cpp,
					c[0], c[1], c[2], c[3] );
}

static void mach64DDLogicOpCode( GLcontext *ctx, GLenum opcode )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   
   if ( ctx->Color.ColorLogicOpEnabled ) {
      FLUSH_BATCH( mmesa );

      FALLBACK( mmesa, MACH64_FALLBACK_LOGICOP, opcode != GL_COPY);
   }
}

void mach64SetCliprects( GLcontext *ctx, GLenum mode )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   __DRIdrawablePrivate *dPriv = mmesa->driDrawable;

   switch ( mode ) {
   case GL_FRONT_LEFT:
      mmesa->numClipRects = dPriv->numClipRects;
      mmesa->pClipRects = dPriv->pClipRects;
      mmesa->drawX = dPriv->x;
      mmesa->drawY = dPriv->y;
      break;
   case GL_BACK_LEFT:
      if ( dPriv->numBackClipRects == 0 ) {
	 mmesa->numClipRects = dPriv->numClipRects;
	 mmesa->pClipRects = dPriv->pClipRects;
	 mmesa->drawX = dPriv->x;
	 mmesa->drawY = dPriv->y;
      } else {
	 mmesa->numClipRects = dPriv->numBackClipRects;
	 mmesa->pClipRects = dPriv->pBackClipRects;
	 mmesa->drawX = dPriv->backX;
	 mmesa->drawY = dPriv->backY;
      }
      break;
   default:
      return;
   }

   mach64UpdateClipping( ctx );

   mmesa->dirty |= MACH64_UPLOAD_CLIPRECTS;
}

static void mach64DDDrawBuffer( GLcontext *ctx, GLenum mode )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);

   FLUSH_BATCH( mmesa );

   /*
    * _DrawDestMask is easier to cope with than <mode>.
    */
   switch ( ctx->DrawBuffer->_ColorDrawBufferMask[0] ) {
   case BUFFER_BIT_FRONT_LEFT:
      FALLBACK( mmesa, MACH64_FALLBACK_DRAW_BUFFER, GL_FALSE );
      mach64SetCliprects( ctx, GL_FRONT_LEFT );
      if (MACH64_DEBUG & DEBUG_VERBOSE_MSG)
	 fprintf(stderr,"%s: BUFFER_BIT_FRONT_LEFT\n", __FUNCTION__);
      break;
   case BUFFER_BIT_BACK_LEFT:
      FALLBACK( mmesa, MACH64_FALLBACK_DRAW_BUFFER, GL_FALSE );
      mach64SetCliprects( ctx, GL_BACK_LEFT );
      if (MACH64_DEBUG & DEBUG_VERBOSE_MSG)
	 fprintf(stderr,"%s: BUFFER_BIT_BACK_LEFT\n", __FUNCTION__);
      break;
   default:
      /* GL_NONE or GL_FRONT_AND_BACK or stereo left&right, etc */
      FALLBACK( mmesa, MACH64_FALLBACK_DRAW_BUFFER, GL_TRUE );
      if (MACH64_DEBUG & DEBUG_VERBOSE_MSG)
	 fprintf(stderr,"%s: fallback (mode=%d)\n", __FUNCTION__, mode);
      break;
   }

   mmesa->setup.dst_off_pitch = (((mmesa->drawPitch/8) << 22) |
				 (mmesa->drawOffset >> 3));

   mmesa->dirty |= MACH64_UPLOAD_DST_OFF_PITCH;
}

static void mach64DDReadBuffer( GLcontext *ctx, GLenum mode )
{
   /* nothing, until we implement h/w glRead/CopyPixels or CopyTexImage */
}

/* =============================================================
 * State enable/disable
 */

static void mach64DDEnable( GLcontext *ctx, GLenum cap, GLboolean state )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);

   if ( MACH64_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s( %s = %s )\n",
	       __FUNCTION__, _mesa_lookup_enum_by_nr( cap ),
	       state ? "GL_TRUE" : "GL_FALSE" );
   }

   switch ( cap ) {
   case GL_ALPHA_TEST:
      FLUSH_BATCH( mmesa );
      mmesa->new_state |= MACH64_NEW_ALPHA;
      break;

   case GL_BLEND:
      FLUSH_BATCH( mmesa );
      mmesa->new_state |= MACH64_NEW_ALPHA;

      /* enable(GL_BLEND) affects ColorLogicOpEnabled.
       */
      FALLBACK( mmesa, MACH64_FALLBACK_LOGICOP,
		(ctx->Color.ColorLogicOpEnabled &&
		 ctx->Color.LogicOp != GL_COPY));
      break;

   case GL_CULL_FACE:
      FLUSH_BATCH( mmesa );
      mmesa->new_state |= MACH64_NEW_CULL;
      break;

   case GL_DEPTH_TEST:
      FLUSH_BATCH( mmesa );
      mmesa->new_state |= MACH64_NEW_DEPTH;
      break;

   case GL_DITHER:
      do {
	 GLuint s = mmesa->setup.scale_3d_cntl;
	 FLUSH_BATCH( mmesa );

	 if ( ctx->Color.DitherFlag ) {
	    /* Dithering causes problems w/ 24bpp depth */
	    if ( mmesa->mach64Screen->cpp == 4 )
	       s |=  MACH64_ROUND_EN;
	    else
	       s |=  MACH64_DITHER_EN;
	 } else {
	    s &= ~MACH64_DITHER_EN;
	    s &= ~MACH64_ROUND_EN;
	 }

	 if ( mmesa->setup.scale_3d_cntl != s ) {
	    mmesa->setup.scale_3d_cntl = s;
	    mmesa->dirty |= ( MACH64_UPLOAD_SCALE_3D_CNTL );
	 }
      } while (0);
      break;

   case GL_FOG:
      FLUSH_BATCH( mmesa );
      mmesa->new_state |= MACH64_NEW_FOG;
      break;

   case GL_INDEX_LOGIC_OP:
   case GL_COLOR_LOGIC_OP:
      FLUSH_BATCH( mmesa );
      FALLBACK( mmesa, MACH64_FALLBACK_LOGICOP,
		state && ctx->Color.LogicOp != GL_COPY );
      break;

   case GL_LIGHTING:
      mach64UpdateSpecularLighting(ctx);
      break;

   case GL_SCISSOR_TEST:
      FLUSH_BATCH( mmesa );
      mmesa->scissor = state;
      mmesa->new_state |= MACH64_NEW_CLIP;
      break;

   case GL_STENCIL_TEST:
      FLUSH_BATCH( mmesa );
      FALLBACK( mmesa, MACH64_FALLBACK_STENCIL, state );
      break;

   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
      FLUSH_BATCH( mmesa );
      mmesa->new_state |= MACH64_NEW_TEXTURE;
      break;

   default:
      return;
   }
}

/* =============================================================
 * Render mode
 */

static void mach64DDRenderMode( GLcontext *ctx, GLenum mode )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   FALLBACK( mmesa, MACH64_FALLBACK_RENDER_MODE, (mode != GL_RENDER) );
}

/* =============================================================
 * State initialization, management
 */

static void mach64DDPrintDirty( const char *msg, GLuint state )
{
   fprintf( stderr,
	    "%s: (0x%x) %s%s%s%s%s%s%s%s%s%s%s%s\n",
	    msg,
	    state,
	    (state & MACH64_UPLOAD_DST_OFF_PITCH) ? "dst_off_pitch, " : "",
	    (state & MACH64_UPLOAD_Z_ALPHA_CNTL)  ? "z_alpha_cntl, " : "",
	    (state & MACH64_UPLOAD_SCALE_3D_CNTL) ? "scale_3d_cntl, " : "",
	    (state & MACH64_UPLOAD_DP_FOG_CLR)    ? "dp_fog_clr, " : "",
	    (state & MACH64_UPLOAD_DP_WRITE_MASK) ? "dp_write_mask, " : "",
	    (state & MACH64_UPLOAD_DP_PIX_WIDTH)  ? "dp_pix_width, " : "",
	    (state & MACH64_UPLOAD_SETUP_CNTL)    ? "setup_cntl, " : "",
	    (state & MACH64_UPLOAD_MISC)          ? "misc, " : "",
	    (state & MACH64_UPLOAD_TEXTURE)       ? "texture, " : "",
	    (state & MACH64_UPLOAD_TEX0IMAGE)     ? "tex0 image, " : "",
	    (state & MACH64_UPLOAD_TEX1IMAGE)     ? "tex1 image, " : "",
	    (state & MACH64_UPLOAD_CLIPRECTS)     ? "cliprects, " : "" );
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
void mach64EmitHwStateLocked( mach64ContextPtr mmesa )
{
   drm_mach64_sarea_t *sarea = mmesa->sarea;
   drm_mach64_context_regs_t *regs = &(mmesa->setup);
   mach64TexObjPtr t0 = mmesa->CurrentTexObj[0];
   mach64TexObjPtr t1 = mmesa->CurrentTexObj[1];

   if ( MACH64_DEBUG & DEBUG_VERBOSE_MSG ) {
      mach64DDPrintDirty( __FUNCTION__, mmesa->dirty );
   }

   if ( t0 && t1 && mmesa->mach64Screen->numTexHeaps > 1 ) {
      if (t0->heap != t1->heap || 
	     (mmesa->dirty & MACH64_UPLOAD_TEX0IMAGE) ||
	     (mmesa->dirty & MACH64_UPLOAD_TEX1IMAGE))
	 mach64UploadMultiTexImages( mmesa, t0, t1 );
   } else {
      if ( mmesa->dirty & MACH64_UPLOAD_TEX0IMAGE ) {
	 if ( t0 ) mach64UploadTexImages( mmesa, t0 );
      }
      if ( mmesa->dirty & MACH64_UPLOAD_TEX1IMAGE ) {
	 if ( t1 ) mach64UploadTexImages( mmesa, t1 );
      }
   }

   if ( mmesa->dirty & (MACH64_UPLOAD_CONTEXT | MACH64_UPLOAD_MISC) ) {
      memcpy( &sarea->context_state, regs,
	      MACH64_NR_CONTEXT_REGS * sizeof(GLuint) );
   }

   if ( mmesa->dirty & MACH64_UPLOAD_TEXTURE ) {
      mach64EmitTexStateLocked( mmesa, t0, t1 );
   }

   sarea->vertsize = mmesa->vertex_size;

   /* Turn off the texture cache flushing.
    */
   mmesa->setup.tex_cntl &= ~MACH64_TEX_CACHE_FLUSH;

   sarea->dirty |= mmesa->dirty;

   mmesa->dirty &= MACH64_UPLOAD_CLIPRECTS;
}

static void mach64DDPrintState( const char *msg, GLuint flags )
{
   fprintf( stderr,
	    "%s: (0x%x) %s%s%s%s%s%s%s%s%s\n",
	    msg,
	    flags,
	    (flags & MACH64_NEW_CONTEXT)	? "context, " : "",
	    (flags & MACH64_NEW_ALPHA)		? "alpha, " : "",
	    (flags & MACH64_NEW_DEPTH)		? "depth, " : "",
	    (flags & MACH64_NEW_FOG)		? "fog, " : "",
	    (flags & MACH64_NEW_CLIP)		? "clip, " : "",
	    (flags & MACH64_NEW_TEXTURE)	? "texture, " : "",
	    (flags & MACH64_NEW_CULL)		? "cull, " : "",
	    (flags & MACH64_NEW_MASKS)		? "masks, " : "",
	    (flags & MACH64_NEW_WINDOW)		? "window, " : "" );
}

/* Update the hardware state */
void mach64DDUpdateHWState( GLcontext *ctx )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   int new_state = mmesa->new_state;

   if ( MACH64_DEBUG & DEBUG_VERBOSE_MSG ) {
      fprintf( stderr, "%s:\n", __FUNCTION__ );
   }

   if ( new_state )
   {
      FLUSH_BATCH( mmesa );

      mmesa->new_state = 0;

      if ( MACH64_DEBUG & DEBUG_VERBOSE_MSG )
	 mach64DDPrintState( __FUNCTION__, new_state );

      /* Update the various parts of the context's state.
       */
      if ( new_state & MACH64_NEW_ALPHA )
	 mach64UpdateAlphaMode( ctx );

      if ( new_state & MACH64_NEW_DEPTH )
	 mach64UpdateZMode( ctx );

      if ( new_state & MACH64_NEW_FOG )
	 mach64UpdateFogAttrib( ctx );

      if ( new_state & MACH64_NEW_CLIP )
	 mach64UpdateClipping( ctx );

      if ( new_state & MACH64_NEW_WINDOW )
	 mach64CalcViewport( ctx );

      if ( new_state & MACH64_NEW_CULL )
	 mach64UpdateCull( ctx );

      if ( new_state & MACH64_NEW_MASKS )
	 mach64UpdateMasks( ctx );

      if ( new_state & MACH64_NEW_TEXTURE )
	 mach64UpdateTextureState( ctx );
   }
}


static void mach64DDInvalidateState( GLcontext *ctx, GLuint new_state )
{
   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _vbo_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   MACH64_CONTEXT(ctx)->NewGLState |= new_state;
}


/* Initialize the context's hardware state */
void mach64DDInitState( mach64ContextPtr mmesa )
{
   GLuint format;

   switch ( mmesa->mach64Screen->cpp ) {
   case 2:
      format = MACH64_DATATYPE_RGB565;
      break;
   case 4:
      format = MACH64_DATATYPE_ARGB8888;
      break;
   default:
      fprintf( stderr, "Error: Unsupported pixel depth... exiting\n" );
      exit( -1 );
   }

   /* Always have a 16-bit depth buffer
    * but Z coordinates are specified in 16.1 format to the setup engine.
    */
   mmesa->depth_scale = 2.0;

   mmesa->ClearColor = 0x00000000;
   mmesa->ClearDepth = 0x0000ffff;

   mmesa->Fallback = 0;

   if ( mmesa->glCtx->Visual.doubleBufferMode ) {
      mmesa->drawOffset = mmesa->readOffset = mmesa->mach64Screen->backOffset;
      mmesa->drawPitch  = mmesa->readPitch  = mmesa->mach64Screen->backPitch;
   } else {
      mmesa->drawOffset = mmesa->readOffset = mmesa->mach64Screen->frontOffset;
      mmesa->drawPitch  = mmesa->readPitch  = mmesa->mach64Screen->frontPitch;
   }

   /* Harware state:
    */
   mmesa->setup.dst_off_pitch = (((mmesa->drawPitch/8) << 22) |
				 (mmesa->drawOffset >> 3));

   mmesa->setup.z_off_pitch = (((mmesa->mach64Screen->depthPitch/8) << 22) |
			       (mmesa->mach64Screen->depthOffset >> 3));

   mmesa->setup.z_cntl = (MACH64_Z_TEST_LESS |
			  MACH64_Z_MASK_EN);

   mmesa->setup.alpha_tst_cntl = (MACH64_ALPHA_TEST_ALWAYS |
				  MACH64_ALPHA_DST_SRCALPHA |
				  MACH64_ALPHA_TST_SRC_TEXEL |
				  (0 << MACH64_REF_ALPHA_SHIFT));

   mmesa->setup.scale_3d_cntl = (MACH64_SCALE_PIX_EXPAND_DYNAMIC_RANGE |
				 /*  MACH64_SCALE_DITHER_ERROR_DIFFUSE | */
				 MACH64_SCALE_DITHER_2D_TABLE |
				 /*  MACH64_DITHER_INIT_CURRENT | */
				 MACH64_DITHER_INIT_RESET |
				 MACH64_SCALE_3D_FCN_SHADE |
				 MACH64_ALPHA_FOG_DIS |
				 MACH64_ALPHA_BLEND_SRC_ONE |
				 MACH64_ALPHA_BLEND_DST_ZERO |
				 MACH64_TEX_LIGHT_FCN_MODULATE |
				 MACH64_MIP_MAP_DISABLE |
				 MACH64_BILINEAR_TEX_EN |
				 MACH64_TEX_BLEND_FCN_LINEAR);

   /* GL spec says dithering initially enabled, but dithering causes
    * problems w/ 24bpp depth
    */
   if ( mmesa->mach64Screen->cpp == 4 )
      mmesa->setup.scale_3d_cntl |= MACH64_ROUND_EN;
   else
      mmesa->setup.scale_3d_cntl |= MACH64_DITHER_EN;

   mmesa->setup.sc_left_right = 0x1fff0000;
   mmesa->setup.sc_top_bottom = 0x3fff0000;

   mmesa->setup.dp_fog_clr    = 0x00ffffff;
   mmesa->setup.dp_write_mask = 0xffffffff;

   mmesa->setup.dp_pix_width = ((format << 0) |
				(format << 4) |
				(format << 8) |
				(format << 16) |
				(format << 28));

   mmesa->setup.dp_mix = (MACH64_BKGD_MIX_S |
			  MACH64_FRGD_MIX_S);
   mmesa->setup.dp_src = (MACH64_BKGD_SRC_3D |
			  MACH64_FRGD_SRC_3D |
			  MACH64_MONO_SRC_ONE);

   mmesa->setup.clr_cmp_cntl  = 0x00000000;
   mmesa->setup.gui_traj_cntl = (MACH64_DST_X_LEFT_TO_RIGHT |
				 MACH64_DST_Y_TOP_TO_BOTTOM);

   mmesa->setup.setup_cntl = (MACH64_FLAT_SHADE_OFF |
			      MACH64_SOLID_MODE_OFF |
			      MACH64_LOG_MAX_INC_ADJ);
   mmesa->setup.setup_cntl = 0;

   mmesa->setup.tex_size_pitch = 0x00000000;

   mmesa->setup.tex_cntl = ((0 << MACH64_LOD_BIAS_SHIFT) |
			    (0 << MACH64_COMP_FACTOR_SHIFT) |
			    MACH64_COMP_COMBINE_MODULATE |
			    MACH64_COMP_BLEND_NEAREST |
			    MACH64_COMP_FILTER_NEAREST |
			    /* MACH64_TEXTURE_TILING | */
#ifdef MACH64_PREMULT_TEXCOORDS
			    MACH64_TEX_ST_DIRECT | 
#endif
			    MACH64_TEX_SRC_LOCAL |
			    MACH64_TEX_UNCOMPRESSED |
			    MACH64_TEX_CACHE_FLUSH |
			    MACH64_TEX_CACHE_SIZE_4K);

   mmesa->setup.secondary_tex_off = 0x00000000;
   mmesa->setup.tex_offset = 0x00000000;

   mmesa->new_state = MACH64_NEW_ALL;
}

/* Initialize the driver's state functions.
  */
void mach64DDInitStateFuncs( GLcontext *ctx )
{
   ctx->Driver.UpdateState		= mach64DDInvalidateState;

   ctx->Driver.ClearIndex		= NULL;
   ctx->Driver.ClearColor		= mach64DDClearColor;
   ctx->Driver.DrawBuffer		= mach64DDDrawBuffer;
   ctx->Driver.ReadBuffer		= mach64DDReadBuffer;

   ctx->Driver.IndexMask		= NULL;
   ctx->Driver.ColorMask		= mach64DDColorMask;
   ctx->Driver.AlphaFunc		= mach64DDAlphaFunc;
   ctx->Driver.BlendEquationSeparate	= mach64DDBlendEquationSeparate;
   ctx->Driver.BlendFuncSeparate	= mach64DDBlendFuncSeparate;
   ctx->Driver.ClearDepth		= mach64DDClearDepth;
   ctx->Driver.CullFace			= mach64DDCullFace;
   ctx->Driver.FrontFace		= mach64DDFrontFace;
   ctx->Driver.DepthFunc		= mach64DDDepthFunc;
   ctx->Driver.DepthMask		= mach64DDDepthMask;
   ctx->Driver.Enable			= mach64DDEnable;
   ctx->Driver.Fogfv			= mach64DDFogfv;
   ctx->Driver.Hint			= NULL;
   ctx->Driver.Lightfv			= NULL;
   ctx->Driver.LightModelfv		= mach64DDLightModelfv;
   ctx->Driver.LogicOpcode		= mach64DDLogicOpCode;
   ctx->Driver.PolygonMode		= NULL;
   ctx->Driver.PolygonStipple		= NULL;
   ctx->Driver.RenderMode		= mach64DDRenderMode;
   ctx->Driver.Scissor			= mach64DDScissor;
   ctx->Driver.ShadeModel		= mach64DDShadeModel;
   
   ctx->Driver.DepthRange		= mach64DepthRange;
   ctx->Driver.Viewport			= mach64Viewport;
}
