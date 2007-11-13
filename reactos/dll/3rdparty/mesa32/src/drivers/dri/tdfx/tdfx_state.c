/* -*- mode: c; c-basic-offset: 3 -*-
 *
 * Copyright 2000 VA Linux Systems Inc., Fremont, California.
 *
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
 * VA LINUX SYSTEMS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/tdfx/tdfx_state.c,v 1.7 2002/10/30 12:52:00 alanh Exp $ */

/*
 * New fixes:
 *	Daniel Borca <dborca@users.sourceforge.net>, 19 Jul 2004
 *
 * Original rewrite:
 *	Gareth Hughes <gareth@valinux.com>, 29 Sep - 1 Oct 2000
 *
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *	Brian Paul <brianp@valinux.com>
 *      Keith Whitwell <keith@tungstengraphics.com> (port to 3.5)
 *
 */

#include "mtypes.h"
#include "colormac.h"
#include "texformat.h"
#include "texstore.h"
#include "teximage.h"

#include "swrast/swrast.h"
#include "vbo/vbo.h"
#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "swrast_setup/swrast_setup.h"

#include "tdfx_context.h"
#include "tdfx_state.h"
#include "tdfx_vb.h"
#include "tdfx_tex.h"
#include "tdfx_texman.h"
#include "tdfx_texstate.h"
#include "tdfx_tris.h"
#include "tdfx_render.h"



/* =============================================================
 * Alpha blending
 */

static void tdfxUpdateAlphaMode( GLcontext *ctx )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   GrCmpFnc_t func;
   GrAlphaBlendFnc_t srcRGB, dstRGB, srcA, dstA;
   GrAlphaBlendOp_t eqRGB, eqA;
   GrAlpha_t ref = (GLint) (ctx->Color.AlphaRef * 255.0);
   
   GLboolean isNapalm = TDFX_IS_NAPALM(fxMesa);
   GLboolean have32bpp = (ctx->Visual.greenBits == 8);
   GLboolean haveAlpha = fxMesa->haveHwAlpha;

   if ( TDFX_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s()\n", __FUNCTION__ );
   }

   if ( ctx->Color.AlphaEnabled ) {
      func = ctx->Color.AlphaFunc - GL_NEVER + GR_CMP_NEVER;
   } else {
      func = GR_CMP_ALWAYS;
   }

   if ( ctx->Color.BlendEnabled
        && (fxMesa->Fallback & TDFX_FALLBACK_BLEND) == 0 ) {
      switch ( ctx->Color.BlendSrcRGB ) {
      case GL_ZERO:
	 srcRGB = GR_BLEND_ZERO;
	 break;
      case GL_ONE:
	 srcRGB = GR_BLEND_ONE;
	 break;
      case GL_DST_COLOR:
	 srcRGB = GR_BLEND_DST_COLOR;
	 break;
      case GL_ONE_MINUS_DST_COLOR:
	 srcRGB = GR_BLEND_ONE_MINUS_DST_COLOR;
	 break;
      case GL_SRC_ALPHA:
	 srcRGB = GR_BLEND_SRC_ALPHA;
	 break;
      case GL_ONE_MINUS_SRC_ALPHA:
	 srcRGB = GR_BLEND_ONE_MINUS_SRC_ALPHA;
	 break;
      case GL_DST_ALPHA:
	 srcRGB = haveAlpha ? GR_BLEND_DST_ALPHA : GR_BLEND_ONE/*JJJ*/;
	 break;
      case GL_ONE_MINUS_DST_ALPHA:
	 srcRGB = haveAlpha ? GR_BLEND_ONE_MINUS_DST_ALPHA : GR_BLEND_ZERO/*JJJ*/;
	 break;
      case GL_SRC_ALPHA_SATURATE:
	 srcRGB = GR_BLEND_ALPHA_SATURATE;
	 break;
      case GL_SRC_COLOR:
         if (isNapalm) {
	    srcRGB = GR_BLEND_SAME_COLOR_EXT;
	    break;
         }
      case GL_ONE_MINUS_SRC_COLOR:
         if (isNapalm) {
	    srcRGB = GR_BLEND_ONE_MINUS_SAME_COLOR_EXT;
	    break;
         }
      default:
	 srcRGB = GR_BLEND_ONE;
      }

      switch ( ctx->Color.BlendSrcA ) {
      case GL_ZERO:
	 srcA = GR_BLEND_ZERO;
	 break;
      case GL_ONE:
	 srcA = GR_BLEND_ONE;
	 break;
      case GL_SRC_COLOR:
      case GL_SRC_ALPHA:
	 srcA = have32bpp ? GR_BLEND_SRC_ALPHA : GR_BLEND_ONE/*JJJ*/;
	 break;
      case GL_ONE_MINUS_SRC_COLOR:
      case GL_ONE_MINUS_SRC_ALPHA:
	 srcA = have32bpp ? GR_BLEND_ONE_MINUS_SRC_ALPHA : GR_BLEND_ONE/*JJJ*/;
	 break;
      case GL_DST_COLOR:
      case GL_DST_ALPHA:
	 srcA = (have32bpp && haveAlpha) ? GR_BLEND_DST_ALPHA : GR_BLEND_ONE/*JJJ*/;
	 break;
      case GL_ONE_MINUS_DST_COLOR:
      case GL_ONE_MINUS_DST_ALPHA:
	 srcA = (have32bpp && haveAlpha) ? GR_BLEND_ONE_MINUS_DST_ALPHA : GR_BLEND_ZERO/*JJJ*/;
	 break;
      case GL_SRC_ALPHA_SATURATE:
         srcA = GR_BLEND_ONE;
	 break;
      default:
	 srcA = GR_BLEND_ONE;
      }

      switch ( ctx->Color.BlendDstRGB ) {
      case GL_ZERO:
	 dstRGB = GR_BLEND_ZERO;
	 break;
      case GL_ONE:
	 dstRGB = GR_BLEND_ONE;
	 break;
      case GL_SRC_COLOR:
	 dstRGB = GR_BLEND_SRC_COLOR;
	 break;
      case GL_ONE_MINUS_SRC_COLOR:
	 dstRGB = GR_BLEND_ONE_MINUS_SRC_COLOR;
	 break;
      case GL_SRC_ALPHA:
	 dstRGB = GR_BLEND_SRC_ALPHA;
	 break;
      case GL_ONE_MINUS_SRC_ALPHA:
	 dstRGB = GR_BLEND_ONE_MINUS_SRC_ALPHA;
	 break;
      case GL_DST_ALPHA:
	 dstRGB = haveAlpha ? GR_BLEND_DST_ALPHA : GR_BLEND_ONE/*JJJ*/;
	 break;
      case GL_ONE_MINUS_DST_ALPHA:
	 dstRGB = haveAlpha ? GR_BLEND_ONE_MINUS_DST_ALPHA : GR_BLEND_ZERO/*JJJ*/;
	 break;
      case GL_DST_COLOR:
         if (isNapalm) {
	    dstRGB = GR_BLEND_SAME_COLOR_EXT;
	    break;
         }
      case GL_ONE_MINUS_DST_COLOR:
         if (isNapalm) {
	    dstRGB = GR_BLEND_ONE_MINUS_SAME_COLOR_EXT;
	    break;
         }
      default:
	 dstRGB = GR_BLEND_ZERO;
      }

      switch ( ctx->Color.BlendDstA ) {
      case GL_ZERO:
	 dstA = GR_BLEND_ZERO;
	 break;
      case GL_ONE:
	 dstA = GR_BLEND_ONE;
	 break;
      case GL_SRC_COLOR:
      case GL_SRC_ALPHA:
	 dstA = have32bpp ? GR_BLEND_SRC_ALPHA : GR_BLEND_ZERO/*JJJ*/;
	 break;
      case GL_ONE_MINUS_SRC_COLOR:
      case GL_ONE_MINUS_SRC_ALPHA:
	 dstA = have32bpp ? GR_BLEND_ONE_MINUS_SRC_ALPHA : GR_BLEND_ZERO/*JJJ*/;
	 break;
      case GL_DST_COLOR:
      case GL_DST_ALPHA:
	 dstA = have32bpp ? GR_BLEND_DST_ALPHA : GR_BLEND_ONE/*JJJ*/;
	 break;
      case GL_ONE_MINUS_DST_COLOR:
      case GL_ONE_MINUS_DST_ALPHA:
	 dstA = have32bpp ? GR_BLEND_ONE_MINUS_DST_ALPHA : GR_BLEND_ZERO/*JJJ*/;
	 break;
      default:
	 dstA = GR_BLEND_ZERO;
      }

      switch ( ctx->Color.BlendEquationRGB ) {
      case GL_FUNC_SUBTRACT:
	 eqRGB = GR_BLEND_OP_SUB;
	 break;
      case GL_FUNC_REVERSE_SUBTRACT:
	 eqRGB = GR_BLEND_OP_REVSUB;
	 break;
      case GL_FUNC_ADD:
      default:
	 eqRGB = GR_BLEND_OP_ADD;
	 break;
      }

      switch ( ctx->Color.BlendEquationA ) {
      case GL_FUNC_SUBTRACT:
	 eqA = GR_BLEND_OP_SUB;
	 break;
      case GL_FUNC_REVERSE_SUBTRACT:
	 eqA = GR_BLEND_OP_REVSUB;
	 break;
      case GL_FUNC_ADD:
      default:
	 eqA = GR_BLEND_OP_ADD;
	 break;
      }
   } else {
      /* blend disabled */
      srcRGB = GR_BLEND_ONE;
      dstRGB = GR_BLEND_ZERO;
      eqRGB = GR_BLEND_OP_ADD;
      srcA = GR_BLEND_ONE;
      dstA = GR_BLEND_ZERO;
      eqA = GR_BLEND_OP_ADD;
   }

   if ( fxMesa->Color.AlphaFunc != func ) {
      fxMesa->Color.AlphaFunc = func;
      fxMesa->dirty |= TDFX_UPLOAD_ALPHA_TEST;
   }
   if ( fxMesa->Color.AlphaRef != ref ) {
      fxMesa->Color.AlphaRef = ref;
      fxMesa->dirty |= TDFX_UPLOAD_ALPHA_REF;
   }

   if ( fxMesa->Color.BlendSrcRGB != srcRGB ||
	fxMesa->Color.BlendDstRGB != dstRGB ||
	fxMesa->Color.BlendEqRGB != eqRGB ||
	fxMesa->Color.BlendSrcA != srcA ||
	fxMesa->Color.BlendDstA != dstA ||
	fxMesa->Color.BlendEqA != eqA )
   {
      fxMesa->Color.BlendSrcRGB = srcRGB;
      fxMesa->Color.BlendDstRGB = dstRGB;
      fxMesa->Color.BlendEqRGB = eqRGB;
      fxMesa->Color.BlendSrcA = srcA;
      fxMesa->Color.BlendDstA = dstA;
      fxMesa->Color.BlendEqA = eqA;
      fxMesa->dirty |= TDFX_UPLOAD_BLEND_FUNC;
   }
}

static void tdfxDDAlphaFunc( GLcontext *ctx, GLenum func, GLfloat ref )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT( ctx );

   FLUSH_BATCH( fxMesa );
   fxMesa->new_state |= TDFX_NEW_ALPHA;
}

static void tdfxDDBlendEquationSeparate( GLcontext *ctx, 
					 GLenum modeRGB, GLenum modeA )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT( ctx );

   assert( modeRGB == modeA );
   FLUSH_BATCH( fxMesa );
   fxMesa->new_state |= TDFX_NEW_ALPHA;
}

static void tdfxDDBlendFuncSeparate( GLcontext *ctx,
				     GLenum sfactorRGB, GLenum dfactorRGB,
				     GLenum sfactorA, GLenum dfactorA )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT( ctx );

   FLUSH_BATCH( fxMesa );
   fxMesa->new_state |= TDFX_NEW_ALPHA;

   /*
    * XXX - Voodoo5 seems to suffer from precision problems in some
    * blend modes.  To pass all the conformance tests we'd have to
    * fall back to software for many modes.  Revisit someday.
    */
}

/* =============================================================
 * Stipple
 */

void tdfxUpdateStipple( GLcontext *ctx )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT( ctx );
   GrStippleMode_t mode = GR_STIPPLE_DISABLE;

   if ( TDFX_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s()\n", __FUNCTION__ );
   }

   FLUSH_BATCH( fxMesa );

   if (ctx->Polygon.StippleFlag) {
      mode = GR_STIPPLE_PATTERN;
   }

   if ( fxMesa->Stipple.Mode != mode ) {
      fxMesa->Stipple.Mode = mode;
      fxMesa->dirty |= TDFX_UPLOAD_STIPPLE;
   }
}


/* =============================================================
 * Depth testing
 */

static void tdfxUpdateZMode( GLcontext *ctx )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT( ctx );
   GrCmpFnc_t func;
   FxI32 bias;
   FxBool mask;

   if ( TDFX_DEBUG & DEBUG_VERBOSE_API ) 
      fprintf( stderr, "%s()\n", __FUNCTION__ );


   bias = (FxI32) (ctx->Polygon.OffsetUnits * TDFX_DEPTH_BIAS_SCALE);

   if ( ctx->Depth.Test ) {
      func = ctx->Depth.Func - GL_NEVER + GR_CMP_NEVER;
      mask = ctx->Depth.Mask;
   }
   else {
      /* depth testing disabled */
      func = GR_CMP_ALWAYS;  /* fragments always pass */
      mask = FXFALSE;        /* zbuffer is not touched */
   }

   fxMesa->Depth.Clear = (FxU32) (ctx->DrawBuffer->_DepthMaxF * ctx->Depth.Clear);

   if ( fxMesa->Depth.Bias != bias ) {
      fxMesa->Depth.Bias = bias;
      fxMesa->dirty |= TDFX_UPLOAD_DEPTH_BIAS;
   }
   if ( fxMesa->Depth.Func != func ) {
      fxMesa->Depth.Func = func;
      fxMesa->dirty |= TDFX_UPLOAD_DEPTH_FUNC | TDFX_UPLOAD_DEPTH_MASK;
   }
   if ( fxMesa->Depth.Mask != mask ) {
      fxMesa->Depth.Mask = mask;
      fxMesa->dirty |= TDFX_UPLOAD_DEPTH_MASK;
   }
}

static void tdfxDDDepthFunc( GLcontext *ctx, GLenum func )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT( ctx );

   FLUSH_BATCH( fxMesa );
   fxMesa->new_state |= TDFX_NEW_DEPTH;
}

static void tdfxDDDepthMask( GLcontext *ctx, GLboolean flag )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT( ctx );

   FLUSH_BATCH( fxMesa );
   fxMesa->new_state |= TDFX_NEW_DEPTH;
}

static void tdfxDDClearDepth( GLcontext *ctx, GLclampd d )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT( ctx );

   FLUSH_BATCH( fxMesa );
   fxMesa->new_state |= TDFX_NEW_DEPTH;
}



/* =============================================================
 * Stencil
 */


/* Evaluate all stencil state and make the Glide calls.
 */
static GrStencil_t convertGLStencilOp( GLenum op )
{
   switch ( op ) {
   case GL_KEEP:
      return GR_STENCILOP_KEEP;
   case GL_ZERO:
      return GR_STENCILOP_ZERO;
   case GL_REPLACE:
      return GR_STENCILOP_REPLACE;
   case GL_INCR:
      return GR_STENCILOP_INCR_CLAMP;
   case GL_DECR:
      return GR_STENCILOP_DECR_CLAMP;
   case GL_INVERT:
      return GR_STENCILOP_INVERT;
   case GL_INCR_WRAP_EXT:
      return GR_STENCILOP_INCR_WRAP;
   case GL_DECR_WRAP_EXT:
      return GR_STENCILOP_DECR_WRAP;
   default:
      _mesa_problem( NULL, "bad stencil op in convertGLStencilOp" );
   }
   return GR_STENCILOP_KEEP;   /* never get, silence compiler warning */
}


static void tdfxUpdateStencil( GLcontext *ctx )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);

   if ( TDFX_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s()\n", __FUNCTION__ );
   }

   if (fxMesa->haveHwStencil) {
      if (ctx->Stencil.Enabled) {
         fxMesa->Stencil.Function = ctx->Stencil.Function[0] - GL_NEVER + GR_CMP_NEVER;
         fxMesa->Stencil.RefValue = ctx->Stencil.Ref[0] & 0xff;
         fxMesa->Stencil.ValueMask = ctx->Stencil.ValueMask[0] & 0xff;
         fxMesa->Stencil.WriteMask = ctx->Stencil.WriteMask[0] & 0xff;
         fxMesa->Stencil.FailFunc = convertGLStencilOp(ctx->Stencil.FailFunc[0]);
         fxMesa->Stencil.ZFailFunc = convertGLStencilOp(ctx->Stencil.ZFailFunc[0]);
         fxMesa->Stencil.ZPassFunc = convertGLStencilOp(ctx->Stencil.ZPassFunc[0]);
         fxMesa->Stencil.Clear = ctx->Stencil.Clear & 0xff;
      }
      fxMesa->dirty |= TDFX_UPLOAD_STENCIL;
   }
}


static void
tdfxDDStencilFuncSeparate( GLcontext *ctx, GLenum face, GLenum func,
                           GLint ref, GLuint mask )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);

   FLUSH_BATCH( fxMesa );
   fxMesa->new_state |= TDFX_NEW_STENCIL;
}

static void
tdfxDDStencilMaskSeparate( GLcontext *ctx, GLenum face, GLuint mask )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);

   FLUSH_BATCH( fxMesa );
   fxMesa->new_state |= TDFX_NEW_STENCIL;
}

static void
tdfxDDStencilOpSeparate( GLcontext *ctx, GLenum face, GLenum sfail,
                         GLenum zfail, GLenum zpass )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);

   FLUSH_BATCH( fxMesa );
   fxMesa->new_state |= TDFX_NEW_STENCIL;
}


/* =============================================================
 * Fog - orthographic fog still not working
 */

static void tdfxUpdateFogAttrib( GLcontext *ctx )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   GrFogMode_t mode;
   GrColor_t color;

   if ( TDFX_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s()\n", __FUNCTION__ );
   }

   if ( ctx->Fog.Enabled ) {
      if (ctx->Fog.FogCoordinateSource == GL_FOG_COORDINATE_EXT) {
         mode = GR_FOG_WITH_TABLE_ON_FOGCOORD_EXT;
      } else {
         mode = GR_FOG_WITH_TABLE_ON_Q;
      }
   } else {
      mode = GR_FOG_DISABLE;
   }

   color = TDFXPACKCOLOR888((GLubyte)(ctx->Fog.Color[0]*255.0F),
			    (GLubyte)(ctx->Fog.Color[1]*255.0F),
			    (GLubyte)(ctx->Fog.Color[2]*255.0F));

   if ( fxMesa->Fog.Mode != mode ) {
      fxMesa->Fog.Mode = mode;
      fxMesa->dirty |= TDFX_UPLOAD_FOG_MODE;
      fxMesa->dirty |= TDFX_UPLOAD_VERTEX_LAYOUT;/*JJJ*/
   }
   if ( fxMesa->Fog.Color != color ) {
      fxMesa->Fog.Color = color;
      fxMesa->dirty |= TDFX_UPLOAD_FOG_COLOR;
   }
   if ( fxMesa->Fog.TableMode != ctx->Fog.Mode ||
	fxMesa->Fog.Density != ctx->Fog.Density ||
	fxMesa->Fog.Near != ctx->Fog.Start ||
	fxMesa->Fog.Far != ctx->Fog.End )
   {
      switch( ctx->Fog.Mode ) {
      case GL_EXP:
	 fxMesa->Glide.guFogGenerateExp( fxMesa->Fog.Table, ctx->Fog.Density );
	 break;
      case GL_EXP2:
	 fxMesa->Glide.guFogGenerateExp2( fxMesa->Fog.Table, ctx->Fog.Density);
	 break;
      case GL_LINEAR:
	 fxMesa->Glide.guFogGenerateLinear( fxMesa->Fog.Table,
                                            ctx->Fog.Start, ctx->Fog.End );
	 break;
      }

      fxMesa->Fog.TableMode = ctx->Fog.Mode;
      fxMesa->Fog.Density = ctx->Fog.Density;
      fxMesa->Fog.Near = ctx->Fog.Start;
      fxMesa->Fog.Far = ctx->Fog.End;
      fxMesa->dirty |= TDFX_UPLOAD_FOG_TABLE;
   }
}

static void tdfxDDFogfv( GLcontext *ctx, GLenum pname, const GLfloat *param )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);

   FLUSH_BATCH( fxMesa );
   fxMesa->new_state |= TDFX_NEW_FOG;

   switch (pname) {
      case GL_FOG_COORDINATE_SOURCE_EXT: {
         GLenum p = (GLenum)*param;
         if (p == GL_FOG_COORDINATE_EXT) {
            _swrast_allow_vertex_fog(ctx, GL_TRUE);
            _swrast_allow_pixel_fog(ctx, GL_FALSE);
            _tnl_allow_vertex_fog( ctx, GL_TRUE);
            _tnl_allow_pixel_fog( ctx, GL_FALSE);
         } else {
            _swrast_allow_vertex_fog(ctx, GL_FALSE);
            _swrast_allow_pixel_fog(ctx, GL_TRUE);
            _tnl_allow_vertex_fog( ctx, GL_FALSE);
            _tnl_allow_pixel_fog( ctx, GL_TRUE);
         }
         break;
      }
      default:
         ;
   }
}


/* =============================================================
 * Clipping
 */

static int intersect_rect( drm_clip_rect_t *out,
			   const drm_clip_rect_t *a,
			   const drm_clip_rect_t *b)
{
   *out = *a;
   if (b->x1 > out->x1) out->x1 = b->x1;
   if (b->y1 > out->y1) out->y1 = b->y1;
   if (b->x2 < out->x2) out->x2 = b->x2;
   if (b->y2 < out->y2) out->y2 = b->y2;
   if (out->x1 >= out->x2) return 0;
   if (out->y1 >= out->y2) return 0;
   return 1;
}


/*
 * Examine XF86 cliprect list and scissor state to recompute our
 * cliprect list.
 */
void tdfxUpdateClipping( GLcontext *ctx )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   __DRIdrawablePrivate *dPriv = fxMesa->driDrawable;

   if ( TDFX_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s()\n", __FUNCTION__ );
   }

   assert(ctx);
   assert(fxMesa);
   assert(dPriv);

   if ( dPriv->x != fxMesa->x_offset || dPriv->y != fxMesa->y_offset ||
	dPriv->w != fxMesa->width || dPriv->h != fxMesa->height ) {
      fxMesa->x_offset = dPriv->x;
      fxMesa->y_offset = dPriv->y;
      fxMesa->width = dPriv->w;
      fxMesa->height = dPriv->h;
      fxMesa->y_delta =
	 fxMesa->screen_height - fxMesa->y_offset - fxMesa->height;
      tdfxUpdateViewport( ctx );
   }

   if (fxMesa->scissoredClipRects && fxMesa->pClipRects) {
      free(fxMesa->pClipRects);
   }

   if (ctx->Scissor.Enabled) {
      /* intersect OpenGL scissor box with all cliprects to make a new
       * list of cliprects.
       */
      drm_clip_rect_t scissor;
      int x1 = ctx->Scissor.X + fxMesa->x_offset;
      int y1 = fxMesa->screen_height - fxMesa->y_delta
             - ctx->Scissor.Y - ctx->Scissor.Height;
      int x2 = x1 + ctx->Scissor.Width;
      int y2 = y1 + ctx->Scissor.Height;
      scissor.x1 = MAX2(x1, 0);
      scissor.y1 = MAX2(y1, 0);
      scissor.x2 = MAX2(x2, 0);
      scissor.y2 = MAX2(y2, 0);

      assert(scissor.x2 >= scissor.x1);
      assert(scissor.y2 >= scissor.y1);

      fxMesa->pClipRects = malloc(dPriv->numClipRects
                                  * sizeof(drm_clip_rect_t));
      if (fxMesa->pClipRects) {
         int i;
         fxMesa->numClipRects = 0;
         for (i = 0; i < dPriv->numClipRects; i++) {
            if (intersect_rect(&fxMesa->pClipRects[fxMesa->numClipRects],
                               &scissor, &dPriv->pClipRects[i])) {
               fxMesa->numClipRects++;
            }
         }
         fxMesa->scissoredClipRects = GL_TRUE;
      }
      else {
         /* out of memory, forgo scissor */
         fxMesa->numClipRects = dPriv->numClipRects;
         fxMesa->pClipRects = dPriv->pClipRects;
         fxMesa->scissoredClipRects = GL_FALSE;
      }
   }
   else {
      fxMesa->numClipRects = dPriv->numClipRects;
      fxMesa->pClipRects = dPriv->pClipRects;
      fxMesa->scissoredClipRects = GL_FALSE;
   }

   fxMesa->dirty |= TDFX_UPLOAD_CLIP;
}



/* =============================================================
 * Culling
 */

void tdfxUpdateCull( GLcontext *ctx )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   GrCullMode_t mode = GR_CULL_DISABLE;

   /* KW: don't need to check raster_primitive here as we don't
    * attempt to draw lines or points with triangles.
    */
   if ( ctx->Polygon.CullFlag ) {
      switch ( ctx->Polygon.CullFaceMode ) {
      case GL_FRONT:
	 if ( ctx->Polygon.FrontFace == GL_CCW ) {
	    mode = GR_CULL_POSITIVE;
	 } else {
	    mode = GR_CULL_NEGATIVE;
	 }
	 break;

      case GL_BACK:
	 if ( ctx->Polygon.FrontFace == GL_CCW ) {
	    mode = GR_CULL_NEGATIVE;
	 } else {
	    mode = GR_CULL_POSITIVE;
	 }
	 break;

      case GL_FRONT_AND_BACK:
	 /* Handled as a fallback on triangles in tdfx_tris.c */
	 return;

      default:
	 ASSERT(0);
	 break;
      }
   }

   if ( fxMesa->CullMode != mode ) {
      fxMesa->CullMode = mode;
      fxMesa->dirty |= TDFX_UPLOAD_CULL;
   }
}

static void tdfxDDCullFace( GLcontext *ctx, GLenum mode )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT( ctx );

   FLUSH_BATCH( fxMesa );
   fxMesa->new_state |= TDFX_NEW_CULL;
}

static void tdfxDDFrontFace( GLcontext *ctx, GLenum mode )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT( ctx );

   FLUSH_BATCH( fxMesa );
   fxMesa->new_state |= TDFX_NEW_CULL;
}


/* =============================================================
 * Line drawing.
 */

static void tdfxUpdateLine( GLcontext *ctx )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT( ctx );

   if ( TDFX_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s()\n", __FUNCTION__ );
   }

   FLUSH_BATCH( fxMesa );
   fxMesa->dirty |= TDFX_UPLOAD_LINE;
}


static void tdfxDDLineWidth( GLcontext *ctx, GLfloat width )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT( ctx );
   FLUSH_BATCH( fxMesa );
   fxMesa->new_state |= TDFX_NEW_LINE;
}


/* =============================================================
 * Color Attributes
 */

static void tdfxDDColorMask( GLcontext *ctx,
			     GLboolean r, GLboolean g,
			     GLboolean b, GLboolean a )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);

   FLUSH_BATCH( fxMesa );

   if ( fxMesa->Color.ColorMask[RCOMP] != r ||
	fxMesa->Color.ColorMask[GCOMP] != g ||
	fxMesa->Color.ColorMask[BCOMP] != b ||
	fxMesa->Color.ColorMask[ACOMP] != a ) {
      fxMesa->Color.ColorMask[RCOMP] = r;
      fxMesa->Color.ColorMask[GCOMP] = g;
      fxMesa->Color.ColorMask[BCOMP] = b;
      fxMesa->Color.ColorMask[ACOMP] = a;
      fxMesa->dirty |= TDFX_UPLOAD_COLOR_MASK;

      if (ctx->Visual.redBits < 8) {
         /* Can't do RGB colormasking in 16bpp mode. */
         /* We can completely ignore the alpha mask. */
	 FALLBACK( fxMesa, TDFX_FALLBACK_COLORMASK, (r != g || g != b) );
      }
   }
}


static void tdfxDDClearColor( GLcontext *ctx,
			      const GLfloat color[4] )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   GLubyte c[4];
   FLUSH_BATCH( fxMesa );
   CLAMPED_FLOAT_TO_UBYTE(c[0], color[0]);
   CLAMPED_FLOAT_TO_UBYTE(c[1], color[1]);
   CLAMPED_FLOAT_TO_UBYTE(c[2], color[2]);
   CLAMPED_FLOAT_TO_UBYTE(c[3], color[3]);
   fxMesa->Color.ClearColor = TDFXPACKCOLOR888( c[0], c[1], c[2] );
   fxMesa->Color.ClearAlpha = c[3];
}


/* =============================================================
 * Light Model
 */

static void tdfxDDLightModelfv( GLcontext *ctx, GLenum pname,
				const GLfloat *param )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);

   if ( pname == GL_LIGHT_MODEL_COLOR_CONTROL ) {
      FALLBACK( fxMesa, TDFX_FALLBACK_SPECULAR,
		(ctx->Light.Enabled &&
		 ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR ));
   }
}

static void tdfxDDShadeModel( GLcontext *ctx, GLenum mode )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);

   /* FIXME: Can we implement native flat shading? */
   FLUSH_BATCH( fxMesa );
   fxMesa->new_state |= TDFX_NEW_TEXTURE;
}


/* =============================================================
 * Scissor
 */

static void
tdfxDDScissor(GLcontext * ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   FLUSH_BATCH( fxMesa );
   fxMesa->new_state |= TDFX_NEW_CLIP;
}

/* =============================================================
 * Render
 */

static void tdfxUpdateRenderAttrib( GLcontext *ctx )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   FLUSH_BATCH( fxMesa );
   fxMesa->dirty |= TDFX_UPLOAD_RENDER_BUFFER;
}

/* =============================================================
 * Viewport
 */

void tdfxUpdateViewport( GLcontext *ctx )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   const GLfloat *v = ctx->Viewport._WindowMap.m;
   GLfloat *m = fxMesa->hw_viewport;

   m[MAT_SX] = v[MAT_SX];
   m[MAT_TX] = v[MAT_TX] + fxMesa->x_offset + TRI_X_OFFSET;
   m[MAT_SY] = v[MAT_SY];
   m[MAT_TY] = v[MAT_TY] + fxMesa->y_delta + TRI_Y_OFFSET;
   m[MAT_SZ] = v[MAT_SZ];
   m[MAT_TZ] = v[MAT_TZ];

   fxMesa->SetupNewInputs |= VERT_BIT_POS;
}


static void tdfxDDViewport( GLcontext *ctx, GLint x, GLint y,
			    GLsizei w, GLsizei h )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   FLUSH_BATCH( fxMesa );
   fxMesa->new_state |= TDFX_NEW_VIEWPORT;
}


static void tdfxDDDepthRange( GLcontext *ctx, GLclampd nearVal, GLclampd farVal )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   FLUSH_BATCH( fxMesa );
   fxMesa->new_state |= TDFX_NEW_VIEWPORT;
}


/* =============================================================
 * State enable/disable
 */

static void tdfxDDEnable( GLcontext *ctx, GLenum cap, GLboolean state )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT( ctx );

   switch ( cap ) {
   case GL_ALPHA_TEST:
      FLUSH_BATCH( fxMesa );
      fxMesa->new_state |= TDFX_NEW_ALPHA;
      break;

   case GL_BLEND:
      FLUSH_BATCH( fxMesa );
      fxMesa->new_state |= TDFX_NEW_ALPHA;
      FALLBACK( fxMesa, TDFX_FALLBACK_LOGICOP,
		(ctx->Color.ColorLogicOpEnabled &&
		 ctx->Color.LogicOp != GL_COPY)/*JJJ - more blending*/);
      break;

   case GL_CULL_FACE:
      FLUSH_BATCH( fxMesa );
      fxMesa->new_state |= TDFX_NEW_CULL;
      break;

   case GL_DEPTH_TEST:
      FLUSH_BATCH( fxMesa );
      fxMesa->new_state |= TDFX_NEW_DEPTH;
      break;

   case GL_DITHER:
      FLUSH_BATCH( fxMesa );
      if ( state ) {
	 fxMesa->Color.Dither = GR_DITHER_2x2;
      } else {
	 fxMesa->Color.Dither = GR_DITHER_DISABLE;
      }
      fxMesa->dirty |= TDFX_UPLOAD_DITHER;
      break;

   case GL_FOG:
      FLUSH_BATCH( fxMesa );
      fxMesa->new_state |= TDFX_NEW_FOG;
      break;

   case GL_COLOR_LOGIC_OP:
      FALLBACK( fxMesa, TDFX_FALLBACK_LOGICOP,
		(ctx->Color.ColorLogicOpEnabled &&
		 ctx->Color.LogicOp != GL_COPY));
      break;

   case GL_LIGHTING:
      FALLBACK( fxMesa, TDFX_FALLBACK_SPECULAR,
		(ctx->Light.Enabled &&
		 ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR ));
      break;

   case GL_LINE_SMOOTH:
      FLUSH_BATCH( fxMesa );
      fxMesa->new_state |= TDFX_NEW_LINE;
      break;

   case GL_LINE_STIPPLE:
      FALLBACK(fxMesa, TDFX_FALLBACK_LINE_STIPPLE, state);
      break;

   case GL_POLYGON_STIPPLE:
      FLUSH_BATCH(fxMesa);
      fxMesa->new_state |= TDFX_NEW_STIPPLE;
      break;

   case GL_SCISSOR_TEST:
      FLUSH_BATCH( fxMesa );
      fxMesa->new_state |= TDFX_NEW_CLIP;
      break;

   case GL_STENCIL_TEST:
      FLUSH_BATCH( fxMesa );
      FALLBACK( fxMesa, TDFX_FALLBACK_STENCIL, state && !fxMesa->haveHwStencil);
      fxMesa->new_state |= TDFX_NEW_STENCIL;
      break;

   case GL_TEXTURE_3D:
      FLUSH_BATCH( fxMesa );
      FALLBACK( fxMesa, TDFX_FALLBACK_TEXTURE_MAP, state); /* wrong */
      fxMesa->new_state |= TDFX_NEW_TEXTURE;
      break;

   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
      FLUSH_BATCH( fxMesa );
      fxMesa->new_state |= TDFX_NEW_TEXTURE;
      break;

   default:
      return;
   }
}



/* Set the buffer used for drawing */
/* XXX support for separate read/draw buffers hasn't been tested */
static void tdfxDDDrawBuffer( GLcontext *ctx, GLenum mode )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);

   if ( TDFX_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s()\n", __FUNCTION__ );
   }

   FLUSH_BATCH( fxMesa );

   /*
    * _ColorDrawBufferMask is easier to cope with than <mode>.
    */
   switch ( ctx->DrawBuffer->_ColorDrawBufferMask[0] ) {
   case BUFFER_BIT_FRONT_LEFT:
      fxMesa->DrawBuffer = fxMesa->ReadBuffer = GR_BUFFER_FRONTBUFFER;
      fxMesa->new_state |= TDFX_NEW_RENDER;
      FALLBACK( fxMesa, TDFX_FALLBACK_DRAW_BUFFER, GL_FALSE );
      break;
   case BUFFER_BIT_BACK_LEFT:
      fxMesa->DrawBuffer = fxMesa->ReadBuffer = GR_BUFFER_BACKBUFFER;
      fxMesa->new_state |= TDFX_NEW_RENDER;
      FALLBACK( fxMesa, TDFX_FALLBACK_DRAW_BUFFER, GL_FALSE );
      break;
   case 0:
      FX_grColorMaskv( ctx, false4 );
      FALLBACK( fxMesa, TDFX_FALLBACK_DRAW_BUFFER, GL_FALSE );
      break;
   default:
      FALLBACK( fxMesa, TDFX_FALLBACK_DRAW_BUFFER, GL_TRUE );
      break;
   }
}


static void tdfxDDReadBuffer( GLcontext *ctx, GLenum mode )
{
   /* XXX ??? */
}


/* =============================================================
 * Polygon stipple
 */

static void tdfxDDPolygonStipple( GLcontext *ctx, const GLubyte *mask )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   const GLubyte *m = mask;
   GLubyte q[4];
   int i,j,k;
   GLboolean allBitsSet;

/*     int active = (ctx->Polygon.StippleFlag &&  */
/*  		 fxMesa->reduced_prim == GL_TRIANGLES); */

   FLUSH_BATCH(fxMesa);
   fxMesa->Stipple.Pattern = 0xffffffff;
   fxMesa->dirty |= TDFX_UPLOAD_STIPPLE;
   fxMesa->new_state |= TDFX_NEW_STIPPLE;

   /* Check if the stipple pattern is fully opaque.  If so, use software
    * rendering.  This basically a trick to make sure the OpenGL conformance
    * test passes.
    */
   allBitsSet = GL_TRUE;
   for (i = 0; i < 32; i++) {
      if (((GLuint *) mask)[i] != 0xffffffff) {
         allBitsSet = GL_FALSE;
         break;
      }
   }
   if (allBitsSet) {
      fxMesa->haveHwStipple = GL_FALSE;
      return;
   }

   q[0] = mask[0];
   q[1] = mask[4];
   q[2] = mask[8];
   q[3] = mask[12];

   for (k = 0 ; k < 8 ; k++)
      for (j = 0 ; j < 4; j++)
	 for (i = 0 ; i < 4 ; i++,m++) {
	    if (*m != q[j]) {
	       fxMesa->haveHwStipple = GL_FALSE;
	       return;
	    }
         }

   fxMesa->haveHwStipple = GL_TRUE;
   fxMesa->Stipple.Pattern = ( (q[0] << 0) |
                               (q[1] << 8) |
                               (q[2] << 16) |
                               (q[3] << 24) );
}



static void tdfxDDRenderMode( GLcontext *ctx, GLenum mode )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   FALLBACK( fxMesa, TDFX_FALLBACK_RENDER_MODE, (mode != GL_RENDER) );
}



static void tdfxDDPrintState( const char *msg, GLuint flags )
{
   fprintf( stderr,
	    "%s: (0x%x) %s%s%s%s%s%s%s%s%s%s%s%s%s\n",
	    msg,
	    flags,
	    (flags & TDFX_NEW_COLOR) ? "color, " : "",
	    (flags & TDFX_NEW_ALPHA) ? "alpha, " : "",
	    (flags & TDFX_NEW_DEPTH) ? "depth, " : "",
	    (flags & TDFX_NEW_RENDER) ? "render, " : "",
	    (flags & TDFX_NEW_FOG) ? "fog, " : "",
	    (flags & TDFX_NEW_STENCIL) ? "stencil, " : "",
	    (flags & TDFX_NEW_STIPPLE) ? "stipple, " : "",
	    (flags & TDFX_NEW_CLIP) ? "clip, " : "",
	    (flags & TDFX_NEW_VIEWPORT) ? "viewport, " : "",
	    (flags & TDFX_NEW_CULL) ? "cull, " : "",
	    (flags & TDFX_NEW_GLIDE) ? "glide, " : "",
	    (flags & TDFX_NEW_TEXTURE) ? "texture, " : "",
	    (flags & TDFX_NEW_CONTEXT) ? "context, " : "");
}



void tdfxDDUpdateHwState( GLcontext *ctx )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   int new_state = fxMesa->new_state;

   if ( TDFX_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s()\n", __FUNCTION__ );
   }

   if ( new_state )
   {
      FLUSH_BATCH( fxMesa );

      fxMesa->new_state = 0;

      if ( 0 )
	 tdfxDDPrintState( "tdfxUpdateHwState", new_state );

      /* Update the various parts of the context's state.
       */
      if ( new_state & TDFX_NEW_ALPHA ) {
	 tdfxUpdateAlphaMode( ctx );
      }

      if ( new_state & TDFX_NEW_DEPTH )
	 tdfxUpdateZMode( ctx );

      if ( new_state & TDFX_NEW_FOG )
	 tdfxUpdateFogAttrib( ctx );

      if ( new_state & TDFX_NEW_CLIP )
	 tdfxUpdateClipping( ctx );

      if ( new_state & TDFX_NEW_STIPPLE )
	 tdfxUpdateStipple( ctx );

      if ( new_state & TDFX_NEW_CULL )
	 tdfxUpdateCull( ctx );

      if ( new_state & TDFX_NEW_LINE )
         tdfxUpdateLine( ctx );

      if ( new_state & TDFX_NEW_VIEWPORT )
	 tdfxUpdateViewport( ctx );

      if ( new_state & TDFX_NEW_RENDER )
	 tdfxUpdateRenderAttrib( ctx );

      if ( new_state & TDFX_NEW_STENCIL )
         tdfxUpdateStencil( ctx );

      if ( new_state & TDFX_NEW_TEXTURE ) {
	 tdfxUpdateTextureState( ctx );
      }
      else if ( new_state & TDFX_NEW_TEXTURE_BIND ) {
	 tdfxUpdateTextureBinding( ctx );
      }
   }

   if ( 0 ) {
      FxI32 bias = (FxI32) (ctx->Polygon.OffsetUnits * TDFX_DEPTH_BIAS_SCALE);

      if ( fxMesa->Depth.Bias != bias ) {
	 fxMesa->Depth.Bias = bias;
	 fxMesa->dirty |= TDFX_UPLOAD_DEPTH_BIAS;
      }
   }

   if ( fxMesa->dirty ) {
      LOCK_HARDWARE( fxMesa );
      tdfxEmitHwStateLocked( fxMesa );
      UNLOCK_HARDWARE( fxMesa );
   }
}


static void tdfxDDInvalidateState( GLcontext *ctx, GLuint new_state )
{
   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _vbo_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   TDFX_CONTEXT(ctx)->new_gl_state |= new_state;
}



/* Initialize the context's Glide state mirror.  These values will be
 * used as Glide function call parameters when the time comes.
 */
void tdfxInitState( tdfxContextPtr fxMesa )
{
   GLcontext *ctx = fxMesa->glCtx;
   GLint i;

   fxMesa->ColorCombine.Function	= GR_COMBINE_FUNCTION_LOCAL;
   fxMesa->ColorCombine.Factor		= GR_COMBINE_FACTOR_NONE;
   fxMesa->ColorCombine.Local		= GR_COMBINE_LOCAL_ITERATED;
   fxMesa->ColorCombine.Other		= GR_COMBINE_OTHER_NONE;
   fxMesa->ColorCombine.Invert		= FXFALSE;
   fxMesa->AlphaCombine.Function	= GR_COMBINE_FUNCTION_LOCAL;
   fxMesa->AlphaCombine.Factor		= GR_COMBINE_FACTOR_NONE;
   fxMesa->AlphaCombine.Local		= GR_COMBINE_LOCAL_ITERATED;
   fxMesa->AlphaCombine.Other		= GR_COMBINE_OTHER_NONE;
   fxMesa->AlphaCombine.Invert		= FXFALSE;

   fxMesa->ColorCombineExt.SourceA	= GR_CMBX_ITRGB;
   fxMesa->ColorCombineExt.ModeA	= GR_FUNC_MODE_X;
   fxMesa->ColorCombineExt.SourceB	= GR_CMBX_ZERO;
   fxMesa->ColorCombineExt.ModeB	= GR_FUNC_MODE_ZERO;
   fxMesa->ColorCombineExt.SourceC	= GR_CMBX_ZERO;
   fxMesa->ColorCombineExt.InvertC	= FXTRUE;
   fxMesa->ColorCombineExt.SourceD	= GR_CMBX_ZERO;
   fxMesa->ColorCombineExt.InvertD	= FXFALSE;
   fxMesa->ColorCombineExt.Shift	= 0;
   fxMesa->ColorCombineExt.Invert	= FXFALSE;
   fxMesa->AlphaCombineExt.SourceA	= GR_CMBX_ITALPHA;
   fxMesa->AlphaCombineExt.ModeA	= GR_FUNC_MODE_X;
   fxMesa->AlphaCombineExt.SourceB	= GR_CMBX_ZERO;
   fxMesa->AlphaCombineExt.ModeB	= GR_FUNC_MODE_ZERO;
   fxMesa->AlphaCombineExt.SourceC	= GR_CMBX_ZERO;
   fxMesa->AlphaCombineExt.InvertC	= FXTRUE;
   fxMesa->AlphaCombineExt.SourceD	= GR_CMBX_ZERO;
   fxMesa->AlphaCombineExt.InvertD	= FXFALSE;
   fxMesa->AlphaCombineExt.Shift	= 0;
   fxMesa->AlphaCombineExt.Invert	= FXFALSE;

   fxMesa->sScale0 = fxMesa->tScale0 = 1.0;
   fxMesa->sScale1 = fxMesa->tScale1 = 1.0;

   fxMesa->TexPalette.Type = 0;
   fxMesa->TexPalette.Data = NULL;

   for ( i = 0 ; i < TDFX_NUM_TMU ; i++ ) {
      fxMesa->TexSource[i].StartAddress	= 0;
      fxMesa->TexSource[i].EvenOdd	= GR_MIPMAPLEVELMASK_EVEN;
      fxMesa->TexSource[i].Info		= NULL;

      fxMesa->TexCombine[i].FunctionRGB		= 0;
      fxMesa->TexCombine[i].FactorRGB		= 0;
      fxMesa->TexCombine[i].FunctionAlpha	= 0;
      fxMesa->TexCombine[i].FactorAlpha		= 0;
      fxMesa->TexCombine[i].InvertRGB		= FXFALSE;
      fxMesa->TexCombine[i].InvertAlpha		= FXFALSE;

      fxMesa->TexCombineExt[i].Alpha.SourceA	= 0;
      /* XXX more state to init here */
      fxMesa->TexCombineExt[i].Color.SourceA	= 0;
      fxMesa->TexCombineExt[i].EnvColor        = 0x0;

      fxMesa->TexParams[i].sClamp 	= GR_TEXTURECLAMP_WRAP;
      fxMesa->TexParams[i].tClamp	= GR_TEXTURECLAMP_WRAP;
      fxMesa->TexParams[i].minFilt	= GR_TEXTUREFILTER_POINT_SAMPLED;
      fxMesa->TexParams[i].magFilt	= GR_TEXTUREFILTER_BILINEAR;
      fxMesa->TexParams[i].mmMode	= GR_MIPMAP_DISABLE;
      fxMesa->TexParams[i].LODblend	= FXFALSE;
      fxMesa->TexParams[i].LodBias	= 0.0;

      fxMesa->TexState.EnvMode[i]	= ~0;
      fxMesa->TexState.TexFormat[i]	= ~0;
      fxMesa->TexState.Enabled[i]	= 0;
   }

   if ( ctx->Visual.doubleBufferMode) {
      fxMesa->DrawBuffer		= GR_BUFFER_BACKBUFFER;
      fxMesa->ReadBuffer		= GR_BUFFER_BACKBUFFER;
   } else {
      fxMesa->DrawBuffer		= GR_BUFFER_FRONTBUFFER;
      fxMesa->ReadBuffer		= GR_BUFFER_FRONTBUFFER;
   }

   fxMesa->Color.ClearColor		= 0x00000000;
   fxMesa->Color.ClearAlpha		= 0x00;
   fxMesa->Color.ColorMask[RCOMP]	= FXTRUE;
   fxMesa->Color.ColorMask[BCOMP]	= FXTRUE;
   fxMesa->Color.ColorMask[GCOMP]	= FXTRUE;
   fxMesa->Color.ColorMask[ACOMP]	= FXTRUE;
   fxMesa->Color.MonoColor		= 0xffffffff;

   fxMesa->Color.AlphaFunc		= GR_CMP_ALWAYS;
   fxMesa->Color.AlphaRef		= 0x00;
   fxMesa->Color.BlendSrcRGB		= GR_BLEND_ONE;
   fxMesa->Color.BlendDstRGB		= GR_BLEND_ZERO;
   fxMesa->Color.BlendSrcA		= GR_BLEND_ONE;
   fxMesa->Color.BlendSrcA		= GR_BLEND_ZERO;

   fxMesa->Color.Dither			= GR_DITHER_2x2;

   if ( fxMesa->glCtx->Visual.depthBits > 0 ) {
      fxMesa->Depth.Mode		= GR_DEPTHBUFFER_ZBUFFER;
   } else {
      fxMesa->Depth.Mode		= GR_DEPTHBUFFER_DISABLE;
   }
   fxMesa->Depth.Bias			= 0;
   fxMesa->Depth.Func			= GR_CMP_LESS;
   fxMesa->Depth.Clear			= 0; /* computed later */
   fxMesa->Depth.Mask			= FXTRUE;


   fxMesa->Fog.Mode			= GR_FOG_DISABLE;
   fxMesa->Fog.Color			= 0x00000000;
   fxMesa->Fog.Table			= NULL;
   fxMesa->Fog.Density			= 1.0;
   fxMesa->Fog.Near			= 1.0;
   fxMesa->Fog.Far			= 1.0;

   fxMesa->Stencil.Function		= GR_CMP_ALWAYS;
   fxMesa->Stencil.RefValue		= 0;
   fxMesa->Stencil.ValueMask		= 0xff;
   fxMesa->Stencil.WriteMask		= 0xff;
   fxMesa->Stencil.FailFunc		= 0;
   fxMesa->Stencil.ZFailFunc		= 0;
   fxMesa->Stencil.ZPassFunc		= 0;
   fxMesa->Stencil.Clear		= 0;

   fxMesa->Stipple.Mode                 = GR_STIPPLE_DISABLE;
   fxMesa->Stipple.Pattern              = 0xffffffff;

   fxMesa->Scissor.minX			= 0;
   fxMesa->Scissor.minY			= 0;
   fxMesa->Scissor.maxX			= 0;
   fxMesa->Scissor.maxY			= 0;

   fxMesa->Viewport.Mode		= GR_WINDOW_COORDS;
   fxMesa->Viewport.X			= 0;
   fxMesa->Viewport.Y			= 0;
   fxMesa->Viewport.Width		= 0;
   fxMesa->Viewport.Height		= 0;
   fxMesa->Viewport.Near		= 0.0;
   fxMesa->Viewport.Far			= 0.0;

   fxMesa->CullMode			= GR_CULL_DISABLE;

   fxMesa->Glide.ColorFormat		= GR_COLORFORMAT_ABGR;
   fxMesa->Glide.Origin			= GR_ORIGIN_LOWER_LEFT;
   fxMesa->Glide.Initialized		= FXFALSE;
}



void tdfxDDInitStateFuncs( GLcontext *ctx )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);

   ctx->Driver.UpdateState		= tdfxDDInvalidateState;

   ctx->Driver.ClearColor		= tdfxDDClearColor;
   ctx->Driver.DrawBuffer		= tdfxDDDrawBuffer;
   ctx->Driver.ReadBuffer		= tdfxDDReadBuffer;

   ctx->Driver.AlphaFunc		= tdfxDDAlphaFunc;
   ctx->Driver.BlendEquationSeparate	= tdfxDDBlendEquationSeparate;
   ctx->Driver.BlendFuncSeparate	= tdfxDDBlendFuncSeparate;
   ctx->Driver.ClearDepth		= tdfxDDClearDepth;
   ctx->Driver.ColorMask		= tdfxDDColorMask;
   ctx->Driver.CullFace			= tdfxDDCullFace;
   ctx->Driver.FrontFace		= tdfxDDFrontFace;
   ctx->Driver.DepthFunc		= tdfxDDDepthFunc;
   ctx->Driver.DepthMask		= tdfxDDDepthMask;
   ctx->Driver.DepthRange		= tdfxDDDepthRange;
   ctx->Driver.Enable			= tdfxDDEnable;
   ctx->Driver.Fogfv			= tdfxDDFogfv;
   ctx->Driver.LightModelfv		= tdfxDDLightModelfv;
   ctx->Driver.LineWidth		= tdfxDDLineWidth;
   ctx->Driver.PolygonStipple		= tdfxDDPolygonStipple;
   ctx->Driver.RenderMode               = tdfxDDRenderMode;
   ctx->Driver.Scissor			= tdfxDDScissor;
   ctx->Driver.ShadeModel		= tdfxDDShadeModel;

   if ( fxMesa->haveHwStencil ) {
      ctx->Driver.StencilFuncSeparate	= tdfxDDStencilFuncSeparate;
      ctx->Driver.StencilMaskSeparate	= tdfxDDStencilMaskSeparate;
      ctx->Driver.StencilOpSeparate	= tdfxDDStencilOpSeparate;
   }

   ctx->Driver.Viewport			= tdfxDDViewport;
}
