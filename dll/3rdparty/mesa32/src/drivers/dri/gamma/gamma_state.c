/* $XFree86: xc/lib/GL/mesa/src/drv/gamma/gamma_state.c,v 1.5 2002/11/05 17:46:07 tsi Exp $ */
/*
 * Copyright 2001 by Alan Hourihane.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, <alanh@tungstengraphics.com>
 *
 * 3DLabs Gamma driver
 */

#include "gamma_context.h"
#include "gamma_macros.h"
#include "buffers.h"
#include "macros.h"
#include "glint_dri.h"
#include "colormac.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "vbo/vbo.h"
#include "tnl/tnl.h"

#define ENABLELIGHTING 0

/* =============================================================
 * Alpha blending
 */

static void gammaUpdateAlphaMode( GLcontext *ctx )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   u_int32_t a = gmesa->AlphaTestMode;
   u_int32_t b = gmesa->AlphaBlendMode;
   u_int32_t f = gmesa->AB_FBReadMode_Save = 0;
   GLubyte refByte = (GLint) (ctx->Color.AlphaRef * 255.0);

   a &= ~(AT_CompareMask | AT_RefValueMask);
   b &= ~(AB_SrcBlendMask | AB_DstBlendMask);

   a |= refByte << 4;

   switch ( ctx->Color.AlphaFunc ) {
      case GL_NEVER:
	 a |= AT_Never;
	 break;
      case GL_LESS:
	 a |= AT_Less;
         break;
      case GL_EQUAL:
	 a |= AT_Equal;
	 break;
      case GL_LEQUAL:
	 a |= AT_LessEqual;
	 break;
      case GL_GEQUAL:
	 a |= AT_GreaterEqual;
	 break;
      case GL_GREATER:
	 a |= AT_Greater;
	 break;
      case GL_NOTEQUAL:
	 a |= AT_NotEqual;
	 break;
      case GL_ALWAYS:
	 a |= AT_Always;
	 break;
   }

   if ( ctx->Color.AlphaEnabled ) {
      f |= FBReadDstEnable;
      a |= AlphaTestModeEnable;
   } else {
      a &= ~AlphaTestModeEnable;
   }

   switch ( ctx->Color.BlendSrcRGB ) {
      case GL_ZERO:
	 b |= AB_Src_Zero; 
	 break;
      case GL_ONE:
	 b |= AB_Src_One;
	 break;
      case GL_DST_COLOR:
	 b |= AB_Src_DstColor;
	 break;
      case GL_ONE_MINUS_DST_COLOR:
	 b |= AB_Src_OneMinusDstColor;
	 break;
      case GL_SRC_ALPHA:
	 b |= AB_Src_SrcAlpha;
	 break;
      case GL_ONE_MINUS_SRC_ALPHA:
	 b |= AB_Src_OneMinusSrcAlpha;
	 break;
      case GL_DST_ALPHA:
	 b |= AB_Src_DstAlpha;
         f |= FBReadSrcEnable;
	 break;
      case GL_ONE_MINUS_DST_ALPHA:
	 b |= AB_Src_OneMinusDstAlpha;
         f |= FBReadSrcEnable;
	 break;
      case GL_SRC_ALPHA_SATURATE:
	 b |= AB_Src_SrcAlphaSaturate;
	 break;
   }

   switch ( ctx->Color.BlendDstRGB ) {
      case GL_ZERO:
	 b |= AB_Dst_Zero;
	 break;
      case GL_ONE:
	 b |= AB_Dst_One;
	 break;
      case GL_SRC_COLOR:
	 b |= AB_Dst_SrcColor;
	 break;
      case GL_ONE_MINUS_SRC_COLOR:
	 b |= AB_Dst_OneMinusSrcColor;
	 break;
      case GL_SRC_ALPHA:
	 b |= AB_Dst_SrcAlpha;
	 break;
      case GL_ONE_MINUS_SRC_ALPHA:
	 b |= AB_Dst_OneMinusSrcAlpha;
	 break;
      case GL_DST_ALPHA:
	 b |= AB_Dst_DstAlpha;
         f |= FBReadSrcEnable;
	 break;
      case GL_ONE_MINUS_DST_ALPHA:
	 b |= AB_Dst_OneMinusDstAlpha;
         f |= FBReadSrcEnable;
	 break;
   }

   if ( ctx->Color.BlendEnabled ) {
      f |= FBReadDstEnable;
      b |= AlphaBlendModeEnable;
   } else {
      b &= ~AlphaBlendModeEnable;
   }

   if ( gmesa->AlphaTestMode != a ) {
      gmesa->AlphaTestMode = a;
      gmesa->dirty |= GAMMA_UPLOAD_ALPHA;
   }
   if ( gmesa->AlphaBlendMode != b) {
      gmesa->AlphaBlendMode = b;
      gmesa->dirty |= GAMMA_UPLOAD_BLEND;
   }
   gmesa->AB_FBReadMode_Save = f;
}

static void gammaDDAlphaFunc( GLcontext *ctx, GLenum func, GLfloat ref )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   (void) ref;

   FLUSH_BATCH( gmesa );

   gmesa->new_state |= GAMMA_NEW_ALPHA;
}

static void gammaDDBlendEquationSeparate( GLcontext *ctx, 
					  GLenum modeRGB, GLenum modeA )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   assert( modeRGB == modeA );
   FLUSH_BATCH( gmesa );

   gmesa->new_state |= GAMMA_NEW_ALPHA;
}

static void gammaDDBlendFuncSeparate( GLcontext *ctx,
				     GLenum sfactorRGB, GLenum dfactorRGB,
				     GLenum sfactorA, GLenum dfactorA )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   FLUSH_BATCH( gmesa );

   gmesa->new_state |= GAMMA_NEW_ALPHA;
}


/* ================================================================
 * Buffer clear
 */

static void gammaDDClear( GLcontext *ctx, GLbitfield mask )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   GLINTDRIPtr gDRIPriv = (GLINTDRIPtr)gmesa->driScreen->pDevPriv;
   GLuint temp = 0;

   FLUSH_BATCH( gmesa );

   /* Update and emit any new state.  We need to do this here to catch
    * changes to the masks.
    * FIXME: Just update the masks?
    */
   if ( gmesa->new_state )
      gammaDDUpdateHWState( ctx );

#ifdef DO_VALIDATE
    /* Flush any partially filled buffers */
    FLUSH_DMA_BUFFER(gmesa);

    DRM_SPINLOCK(&gmesa->driScreen->pSAREA->drawable_lock,
		 gmesa->driScreen->drawLockID);
    VALIDATE_DRAWABLE_INFO_NO_LOCK(gmesa);
#endif

    if (mask & BUFFER_BIT_DEPTH) {
	 /* Turn off writes the FB */
	 CHECK_DMA_BUFFER(gmesa, 1);
	 WRITE(gmesa->buf, FBWriteMode, FBWriteModeDisable);

	 mask &= ~BUFFER_BIT_DEPTH;

	 /*
	  * Turn Rectangle2DControl off when the window is not clipped
	  * (i.e., the GID tests are not necessary).  This dramatically
	  * increases the performance of the depth clears.
	  */
	 if (!gmesa->NotClipped) {
	    CHECK_DMA_BUFFER(gmesa, 1);
	    WRITE(gmesa->buf, Rectangle2DControl, 1);
	 }

	 temp = (gmesa->LBReadMode & LBPartialProdMask) | LBWindowOriginBot;
	 if (gDRIPriv->numMultiDevices == 2) temp |= LBScanLineInt2;
	 
	 CHECK_DMA_BUFFER(gmesa, 5);
	 WRITE(gmesa->buf, LBReadMode, temp);
	 WRITE(gmesa->buf, DeltaMode, DM_DepthEnable);
	 WRITE(gmesa->buf, DepthMode, (DepthModeEnable |
					DM_Always |
					DM_SourceDepthRegister |
					DM_WriteMask));
	 WRITE(gmesa->buf, GLINTDepth, gmesa->ClearDepth);

	 /* Increment the frame count */
	 gmesa->FrameCount++;
#ifdef FAST_CLEAR_4
	 gmesa->FrameCount &= 0x0f;
#else
	 gmesa->FrameCount &= 0xff;
#endif

	 /* Force FCP to be written */
	 WRITE(gmesa->buf, GLINTWindow, (WindowEnable |
					  W_PassIfEqual |
					  (gmesa->Window & W_GIDMask) |
					  W_DepthFCP |
					  W_LBUpdateFromRegisters |
					  W_OverrideWriteFiltering |
					  (gmesa->FrameCount << 9)));

	/* Clear part of the depth and FCP buffers */
	{
	    int y = gmesa->driScreen->fbHeight - gmesa->driDrawable->y - gmesa->driDrawable->h;
	    int x = gmesa->driDrawable->x;
	    int w = gmesa->driDrawable->w;
	    int h = gmesa->driDrawable->h;
#ifndef TURN_OFF_FCP
	    float hsub = h;

	    if (gmesa->WindowChanged) {
		gmesa->WindowChanged = GL_FALSE;
	    } else {
#ifdef FAST_CLEAR_4
		hsub /= 16;
#else
		hsub /= 256;
#endif

		/* Handle the case where the height < # of FCPs */
		if (hsub < 1.0) {
		    if (gmesa->FrameCount > h)
			gmesa->FrameCount = 0;
		    h = 1;
		    y += gmesa->FrameCount;
		} else {
		    h = (gmesa->FrameCount+1)*hsub;
		    h -= (int)(gmesa->FrameCount*hsub);
		    y += gmesa->FrameCount*hsub;
		}
	    }
#endif
	    if (h && w) {
#if 0
		CHECK_DMA_BUFFER(gmesa, 2);
		WRITE(gmesa->buf, Rectangle2DMode, ((h & 0xfff)<<12) |
						   (w & 0xfff) );
		WRITE(gmesa->buf, DrawRectangle2D, ((y & 0xffff)<<16) |
						   (x & 0xffff) );
#else
		CHECK_DMA_BUFFER(gmesa, 8);
		WRITE(gmesa->buf, StartXDom,   x<<16);
		WRITE(gmesa->buf, StartY,      y<<16);
		WRITE(gmesa->buf, StartXSub,   (x+w)<<16);
		WRITE(gmesa->buf, GLINTCount,  h);
		WRITE(gmesa->buf, dY,          1<<16);
		WRITE(gmesa->buf, dXDom,       0<<16);
		WRITE(gmesa->buf, dXSub,       0<<16);
		WRITE(gmesa->buf, Render,      0x00000040); /* NOT_DONE */
#endif
	    }
	}

	CHECK_DMA_BUFFER(gmesa, 6);
	WRITE(gmesa->buf, DepthMode, gmesa->DepthMode);
	WRITE(gmesa->buf, DeltaMode, gmesa->DeltaMode);
	WRITE(gmesa->buf, LBReadMode, gmesa->LBReadMode);
	WRITE(gmesa->buf, GLINTWindow, gmesa->Window);
	WRITE(gmesa->buf, FastClearDepth, gmesa->ClearDepth);
	WRITE(gmesa->buf, FBWriteMode, FBWriteModeEnable);

	/* Turn on Depth FCP */
	if (gmesa->Window & W_DepthFCP) {
	    CHECK_DMA_BUFFER(gmesa, 1);
	    WRITE(gmesa->buf, WindowOr, (gmesa->FrameCount << 9));
	}

	/* Turn off GID clipping if window is not clipped */
	if (gmesa->NotClipped) {
	    CHECK_DMA_BUFFER(gmesa, 1);
	    WRITE(gmesa->buf, Rectangle2DControl, 0);
	}
    }

    if (mask & (BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_BACK_LEFT)) {
	int y = gmesa->driScreen->fbHeight - gmesa->driDrawable->y - gmesa->driDrawable->h;
	int x = gmesa->driDrawable->x;
	int w = gmesa->driDrawable->w;
	int h = gmesa->driDrawable->h;

	mask &= ~(BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_BACK_LEFT);

	if (x < 0) { w -= -x; x = 0; }

	/* Turn on GID clipping if window is clipped */
	if (!gmesa->NotClipped) {
	    CHECK_DMA_BUFFER(gmesa, 1);
	    WRITE(gmesa->buf, Rectangle2DControl, 1);
	}

        CHECK_DMA_BUFFER(gmesa, 18);
        WRITE(gmesa->buf, FBBlockColor, gmesa->ClearColor);
        WRITE(gmesa->buf, ColorDDAMode, ColorDDADisable);
	WRITE(gmesa->buf, FBWriteMode, FBWriteModeEnable);
	WRITE(gmesa->buf, DepthMode, 0);
	WRITE(gmesa->buf, DeltaMode, 0);
	WRITE(gmesa->buf, AlphaBlendMode, 0);
#if 1
	WRITE(gmesa->buf, dY,          1<<16);
	WRITE(gmesa->buf, dXDom,       0<<16);
	WRITE(gmesa->buf, dXSub,       0<<16);
	WRITE(gmesa->buf, StartXSub,   (x+w)<<16);
	WRITE(gmesa->buf, GLINTCount,  h);
	WRITE(gmesa->buf, StartXDom,   x<<16);
	WRITE(gmesa->buf, StartY,      y<<16);
	WRITE(gmesa->buf, Render,      0x00000048); /* NOT_DONE */
#else
	WRITE(gmesa->buf, Rectangle2DMode, (((h & 0xfff)<<12) |
					      (w & 0xfff)));
	WRITE(gmesa->buf, DrawRectangle2D, (((y & 0xffff)<<16) |
					      (x & 0xffff)));
#endif
	WRITE(gmesa->buf, DepthMode, gmesa->DepthMode);
	WRITE(gmesa->buf, DeltaMode, gmesa->DeltaMode);
	WRITE(gmesa->buf, AlphaBlendMode, gmesa->AlphaBlendMode);
	WRITE(gmesa->buf, ColorDDAMode, gmesa->ColorDDAMode);

	/* Turn off GID clipping if window is clipped */
	if (gmesa->NotClipped) {
	    CHECK_DMA_BUFFER(gmesa, 1);
	    WRITE(gmesa->buf, Rectangle2DControl, 0);
	}
    }

#ifdef DO_VALIDATE
    PROCESS_DMA_BUFFER_TOP_HALF(gmesa);

    DRM_SPINUNLOCK(&gmesa->driScreen->pSAREA->drawable_lock,
		   gmesa->driScreen->drawLockID);
    VALIDATE_DRAWABLE_INFO_NO_LOCK_POST(gmesa);

    PROCESS_DMA_BUFFER_BOTTOM_HALF(gmesa);
#endif

   if ( mask )
      _swrast_Clear( ctx, mask );
}

/* =============================================================
 * Depth testing
 */

static void gammaUpdateZMode( GLcontext *ctx )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   u_int32_t z = gmesa->DepthMode;
   u_int32_t delta = gmesa->DeltaMode;
   u_int32_t window = gmesa->Window;
   u_int32_t lbread = gmesa->LBReadMode;

   z &= ~DM_CompareMask;

   switch ( ctx->Depth.Func ) {
      case GL_NEVER:
	 z |= DM_Never;
	 break;
      case GL_ALWAYS:
	 z |= DM_Always;
	 break;
      case GL_LESS:
	 z |= DM_Less;
	 break;
      case GL_LEQUAL:
	 z |= DM_LessEqual;
	 break;
      case GL_EQUAL:
	 z |= DM_Equal;
	 break;
      case GL_GEQUAL:
	 z |= DM_GreaterEqual;
	 break;
      case GL_GREATER:
	 z |= DM_Greater;
	 break;
      case GL_NOTEQUAL:
	 z |= DM_NotEqual;
	 break;
   }

   if ( ctx->Depth.Test ) {
      z      |= DepthModeEnable;
      delta  |= DM_DepthEnable;
      window |= W_DepthFCP;
      lbread |= LBReadDstEnable;
   } else {
      z      &= ~DepthModeEnable;
      delta  &= ~DM_DepthEnable;
      window &= ~W_DepthFCP;
      lbread &= ~LBReadDstEnable;
   }

   if ( ctx->Depth.Mask ) {
      z |= DM_WriteMask;
   } else {
      z &= ~DM_WriteMask;
   }

#if 0
   if ( gmesa->DepthMode != z ){
#endif
      gmesa->DepthMode = z;
      gmesa->DeltaMode = delta;
      gmesa->Window = window;
      gmesa->LBReadMode = lbread;
      gmesa->dirty |= GAMMA_UPLOAD_DEPTH;
#if 0
   }
#endif
}

static void gammaDDDepthFunc( GLcontext *ctx, GLenum func )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   FLUSH_BATCH( gmesa );
   gmesa->new_state |= GAMMA_NEW_DEPTH;
}

static void gammaDDDepthMask( GLcontext *ctx, GLboolean flag )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   FLUSH_BATCH( gmesa );
   gmesa->new_state |= GAMMA_NEW_DEPTH;
}

static void gammaDDClearDepth( GLcontext *ctx, GLclampd d )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   switch ( gmesa->DepthSize ) {
   case 16:
      gmesa->ClearDepth = d * 0x0000ffff;
      break;
   case 24:
      gmesa->ClearDepth = d * 0x00ffffff;
      break;
   case 32:
      gmesa->ClearDepth = d * 0xffffffff;
      break;
   }
}

static void gammaDDFinish( GLcontext *ctx )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   FLUSH_DMA_BUFFER(gmesa);
}

static void gammaDDFlush( GLcontext *ctx )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   FLUSH_DMA_BUFFER(gmesa);
}

/* =============================================================
 * Fog
 */

static void gammaUpdateFogAttrib( GLcontext *ctx )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   u_int32_t f = gmesa->FogMode;
   u_int32_t g = gmesa->GeometryMode;
   u_int32_t d = gmesa->DeltaMode;

   if (ctx->Fog.Enabled) {
      f |= FogModeEnable;
      g |= GM_FogEnable;
      d |= DM_FogEnable;
   } else {
      f &= ~FogModeEnable;
      g &= ~GM_FogEnable;
      d &= ~DM_FogEnable;
   }

   g &= ~GM_FogMask;

   switch (ctx->Fog.Mode) {
      case GL_LINEAR:
         g |= GM_FogLinear;
         break;
      case GL_EXP:
         g |= GM_FogExp;
         break;
      case GL_EXP2:
         g |= GM_FogExpSquared;
         break;
   }

   if ( gmesa->FogMode != f ) {
      gmesa->FogMode = f;
      gmesa->dirty |= GAMMA_UPLOAD_FOG;
   }
 
   if ( gmesa->GeometryMode != g ) {
      gmesa->GeometryMode = g;
      gmesa->dirty |= GAMMA_UPLOAD_GEOMETRY;
   }

   if ( gmesa->DeltaMode != d ) {
      gmesa->DeltaMode = d;
      gmesa->dirty |= GAMMA_UPLOAD_DEPTH;
   }
}

#if 0
static void gammaDDFogfv( GLcontext *ctx, GLenum pname, const GLfloat *param )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   FLUSH_BATCH( gmesa );
   gmesa->new_state |= GAMMA_NEW_FOG;
}
#endif

/* =============================================================
 * Lines
 */
static void gammaDDLineWidth( GLcontext *ctx, GLfloat width )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   CHECK_DMA_BUFFER(gmesa, 3);
   WRITE(gmesa->buf, LineWidth, (GLuint)width);
   WRITEF(gmesa->buf, AAlineWidth, width);
   WRITE(gmesa->buf, LineWidthOffset, (GLuint)(width-1)/2);
}

static void gammaDDLineStipple( GLcontext *ctx, GLint factor, GLushort pattern )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   gmesa->LineMode &= ~(LM_StippleMask | LM_RepeatFactorMask);
   gmesa->LineMode |= ((GLuint)(factor - 1) << 1) | ((GLuint)pattern << 10); 

   gmesa->dirty |= GAMMA_UPLOAD_LINEMODE;
}



/* =============================================================
 * Points
 */
static void gammaDDPointSize( GLcontext *ctx, GLfloat size )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   CHECK_DMA_BUFFER(gmesa, 2);
   WRITE(gmesa->buf, PointSize, (GLuint)size);
   WRITEF(gmesa->buf, AApointSize, size);
}

/* =============================================================
 * Polygon 
 */

static void gammaUpdatePolygon( GLcontext *ctx )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   u_int32_t g = gmesa->GeometryMode;

   g &= ~(GM_PolyOffsetFillEnable | GM_PolyOffsetPointEnable |
          GM_PolyOffsetLineEnable);

   if (ctx->Polygon.OffsetFill) g |= GM_PolyOffsetFillEnable;
   if (ctx->Polygon.OffsetPoint) g |= GM_PolyOffsetPointEnable;
   if (ctx->Polygon.OffsetLine) g |= GM_PolyOffsetLineEnable;

   g &= ~GM_FB_PolyMask;

   switch (ctx->Polygon.FrontMode) {
      case GL_FILL:
         g |= GM_FrontPolyFill;
         break;
      case GL_LINE:
         g |= GM_FrontPolyLine;
         break;
      case GL_POINT:
         g |= GM_FrontPolyPoint;
         break;
   }

   switch (ctx->Polygon.BackMode) {
      case GL_FILL:
         g |= GM_BackPolyFill;
         break;
      case GL_LINE:
         g |= GM_BackPolyLine;
         break;
      case GL_POINT:
         g |= GM_BackPolyPoint;
         break;
   }

   if ( gmesa->GeometryMode != g ) {
      gmesa->GeometryMode = g;
      gmesa->dirty |= GAMMA_UPLOAD_GEOMETRY;
   }

   gmesa->dirty |= GAMMA_UPLOAD_POLYGON;
}

static void gammaDDPolygonMode( GLcontext *ctx, GLenum face, GLenum mode)
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   FLUSH_BATCH( gmesa );

   gmesa->new_state |= GAMMA_NEW_POLYGON;
}

static void gammaUpdateStipple( GLcontext *ctx )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   FLUSH_BATCH( gmesa );

   if (ctx->Polygon.StippleFlag) {
      gmesa->AreaStippleMode |= AreaStippleModeEnable/* | ASM_X32 | ASM_Y32*/;
   } else {
      gmesa->AreaStippleMode &= ~AreaStippleModeEnable;
   }

   gmesa->dirty |= GAMMA_UPLOAD_STIPPLE;
}

static void gammaDDPolygonStipple( GLcontext *ctx, const GLubyte *mask)
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   FLUSH_BATCH( gmesa );
   gmesa->new_state |= GAMMA_NEW_STIPPLE;
}

/* =============================================================
 * Clipping
 */

static void gammaUpdateClipping( GLcontext *ctx )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   GLint x1, y1, x2, y2;

   if ( gmesa->driDrawable ) {
      x1 = gmesa->driDrawable->x + ctx->Scissor.X;
      y1 = gmesa->driScreen->fbHeight -
	(gmesa->driDrawable->y +
	 gmesa->driDrawable->h) + ctx->Scissor.Y;
      x2 = x1 + ctx->Scissor.Width;
      y2 = y1 + ctx->Scissor.Height;

      gmesa->ScissorMinXY = x1 | (y1 << 16);
      gmesa->ScissorMaxXY = x2 | (y2 << 16);
      if (ctx->Scissor.Enabled) 
         gmesa->ScissorMode |= UserScissorEnable;
      else
         gmesa->ScissorMode &= ~UserScissorEnable;

      gmesa->dirty |= GAMMA_UPLOAD_CLIP;
   }
}

static void gammaDDScissor( GLcontext *ctx,
			   GLint x, GLint y, GLsizei w, GLsizei h )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   FLUSH_BATCH( gmesa );
   gmesa->new_state |= GAMMA_NEW_CLIP;
}

/* =============================================================
 * Culling
 */

static void gammaUpdateCull( GLcontext *ctx )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   u_int32_t g = gmesa->GeometryMode;

   g &= ~(GM_PolyCullMask | GM_FFMask);

   if (ctx->Polygon.FrontFace == GL_CCW) {
      g |= GM_FrontFaceCCW;
   } else {
      g |= GM_FrontFaceCW;
   }

   switch ( ctx->Polygon.CullFaceMode ) {
      case GL_FRONT:
	 g |= GM_PolyCullFront;
	 break;
      case GL_BACK:
	 g |= GM_PolyCullBack;
	 break;
      case GL_FRONT_AND_BACK:
	 g |= GM_PolyCullBoth;
	 break;
   }

   if ( ctx->Polygon.CullFlag ) {
      g |= GM_PolyCullEnable;
   } else {
      g &= ~GM_PolyCullEnable;
   }

   if ( gmesa->GeometryMode != g ) {
      gmesa->GeometryMode = g;
      gmesa->dirty |= GAMMA_UPLOAD_GEOMETRY;
   }
}

static void gammaDDCullFace( GLcontext *ctx, GLenum mode )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   FLUSH_BATCH( gmesa );
   gmesa->new_state |= GAMMA_NEW_CULL;
}

static void gammaDDFrontFace( GLcontext *ctx, GLenum mode )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   FLUSH_BATCH( gmesa );
   gmesa->new_state |= GAMMA_NEW_CULL;
}

/* =============================================================
 * Masks
 */

static void gammaUpdateMasks( GLcontext *ctx )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);


   GLuint mask = gammaPackColor( gmesa->gammaScreen->cpp,
				ctx->Color.ColorMask[RCOMP],
				ctx->Color.ColorMask[GCOMP],
				ctx->Color.ColorMask[BCOMP],
				ctx->Color.ColorMask[ACOMP] );

   if (gmesa->gammaScreen->cpp == 2) mask |= mask << 16;

   if ( gmesa->FBHardwareWriteMask != mask ) {
      gmesa->FBHardwareWriteMask = mask;
      gmesa->dirty |= GAMMA_UPLOAD_MASKS;
   }
}

static void gammaDDColorMask( GLcontext *ctx, GLboolean r, GLboolean g,
			      GLboolean b, GLboolean a)
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   FLUSH_BATCH( gmesa );
   gmesa->new_state |= GAMMA_NEW_MASKS;
}

/* =============================================================
 * Rendering attributes
 *
 * We really don't want to recalculate all this every time we bind a
 * texture.  These things shouldn't change all that often, so it makes
 * sense to break them out of the core texture state update routines.
 */

#if ENABLELIGHTING
static void gammaDDLightfv(GLcontext *ctx, GLenum light, GLenum pname, 
				const GLfloat *params, GLint nParams)
{
    gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
    GLfloat l,x,y,z,w;

    switch(light) {
    case GL_LIGHT0:
	switch (pname) {
	case GL_AMBIENT:
	    CHECK_DMA_BUFFER(gmesa, 3);
	    /* We don't do alpha */
	    WRITEF(gmesa->buf, Light0AmbientIntensityBlue, params[2]);
	    WRITEF(gmesa->buf, Light0AmbientIntensityGreen, params[1]);
	    WRITEF(gmesa->buf, Light0AmbientIntensityRed, params[0]);
	    break;
	case GL_DIFFUSE:
	    CHECK_DMA_BUFFER(gmesa, 3);
	    /* We don't do alpha */
	    WRITEF(gmesa->buf, Light0DiffuseIntensityBlue, params[2]);
	    WRITEF(gmesa->buf, Light0DiffuseIntensityGreen, params[1]);
	    WRITEF(gmesa->buf, Light0DiffuseIntensityRed, params[0]);
	    break;
	case GL_SPECULAR:
	    CHECK_DMA_BUFFER(gmesa, 3);
	    /* We don't do alpha */
	    WRITEF(gmesa->buf, Light0SpecularIntensityBlue, params[2]);
	    WRITEF(gmesa->buf, Light0SpecularIntensityGreen, params[1]);
	    WRITEF(gmesa->buf, Light0SpecularIntensityRed, params[0]);
	    break;
	case GL_POSITION:
    	    /* Normalize <x,y,z> */
	    x = params[0]; y = params[1]; z = params[2]; w = params[3];
	    l = sqrt(x*x + y*y + z*z + w*w);
	    w /= l;
	    x /= l;
	    y /= l;
	    z /= l;
	    if (params[3] != 0.0) {
		gmesa->Light0Mode |= Light0ModeAttenuation;
		gmesa->Light0Mode |= Light0ModeLocal;
	    } else {
		gmesa->Light0Mode &= ~Light0ModeAttenuation;
		gmesa->Light0Mode &= ~Light0ModeLocal;
	    }
	    CHECK_DMA_BUFFER(gmesa, 5);
	    WRITE(gmesa->buf, Light0Mode, gmesa->Light0Mode);
	    WRITEF(gmesa->buf, Light0PositionW, w);
	    WRITEF(gmesa->buf, Light0PositionZ, z);
	    WRITEF(gmesa->buf, Light0PositionY, y);
	    WRITEF(gmesa->buf, Light0PositionX, x);
	    break;
	case GL_SPOT_DIRECTION:
	    CHECK_DMA_BUFFER(gmesa, 3);
	    /* WRITEF(gmesa->buf, Light0SpotlightDirectionW, params[3]); */
	    WRITEF(gmesa->buf, Light0SpotlightDirectionZ, params[2]);
	    WRITEF(gmesa->buf, Light0SpotlightDirectionY, params[1]);
	    WRITEF(gmesa->buf, Light0SpotlightDirectionX, params[0]);
	    break;
	case GL_SPOT_EXPONENT:
	    CHECK_DMA_BUFFER(gmesa, 1);
	    WRITEF(gmesa->buf, Light0SpotlightExponent, params[0]);
	    break;
	case GL_SPOT_CUTOFF:
	    if (params[0] != 180.0) 
		gmesa->Light0Mode |= Light0ModeSpotLight;
	    else
		gmesa->Light0Mode &= ~Light0ModeSpotLight;
	    CHECK_DMA_BUFFER(gmesa, 2);
	    WRITE(gmesa->buf, Light0Mode, gmesa->Light0Mode);
	    WRITEF(gmesa->buf, Light0CosSpotlightCutoffAngle, cos(params[0]*DEG2RAD));
	    break;
	case GL_CONSTANT_ATTENUATION:
	    CHECK_DMA_BUFFER(gmesa, 1);
	    WRITEF(gmesa->buf, Light0ConstantAttenuation, params[0]);
	    break;
	case GL_LINEAR_ATTENUATION:
	    CHECK_DMA_BUFFER(gmesa, 1);
	    WRITEF(gmesa->buf, Light0LinearAttenuation, params[0]);
	    break;
	case GL_QUADRATIC_ATTENUATION:
	    CHECK_DMA_BUFFER(gmesa, 1);
	    WRITEF(gmesa->buf, Light0QuadraticAttenuation, params[0]);
	    break;
	}
	break;
    }
}

static void gammaDDLightModelfv( GLcontext *ctx, GLenum pname,
				const GLfloat *params )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   switch (pname) {
   case GL_LIGHT_MODEL_AMBIENT:
	CHECK_DMA_BUFFER(gmesa, 3);
	/* We don't do alpha */
	WRITEF(gmesa->buf, SceneAmbientColorBlue, params[2]);
	WRITEF(gmesa->buf, SceneAmbientColorGreen, params[1]);
	WRITEF(gmesa->buf, SceneAmbientColorRed, params[0]);
	break;
    case GL_LIGHT_MODEL_LOCAL_VIEWER:
	if (params[0] != 0.0)
	    gmesa->LightingMode |= LightingModeLocalViewer;
	else
	    gmesa->LightingMode &= ~LightingModeLocalViewer;
	CHECK_DMA_BUFFER(gmesa, 1);
	WRITE(gmesa->buf, LightingMode, gmesa->LightingMode);
	break;
    case GL_LIGHT_MODEL_TWO_SIDE:
	if (params[0] == 1.0f) {
	    gmesa->LightingMode |= LightingModeTwoSides;
	    gmesa->MaterialMode |= MaterialModeTwoSides;
	} else {
	    gmesa->LightingMode &= ~LightingModeTwoSides;
	    gmesa->MaterialMode &= ~MaterialModeTwoSides;
	}
	CHECK_DMA_BUFFER(gmesa, 2);
	WRITE(gmesa->buf, LightingMode, gmesa->LightingMode);
	WRITE(gmesa->buf, MaterialMode, gmesa->MaterialMode);
	break;
    }
}
#endif

static void gammaDDShadeModel( GLcontext *ctx, GLenum mode )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   u_int32_t g = gmesa->GeometryMode;
   u_int32_t c = gmesa->ColorDDAMode;

   g &= ~GM_ShadingMask;
   c &= ~ColorDDAShadingMask;

   switch ( mode ) {
   case GL_FLAT:
      g |= GM_FlatShading;
      c |= ColorDDAFlat;
      break;
   case GL_SMOOTH:
      g |= GM_GouraudShading;
      c |= ColorDDAGouraud;
      break;
   default:
      return;
   }

   if ( gmesa->ColorDDAMode != c ) {
      FLUSH_BATCH( gmesa );
      gmesa->ColorDDAMode = c;

      gmesa->dirty |= GAMMA_UPLOAD_SHADE;
   }

   if ( gmesa->GeometryMode != g ) {
      FLUSH_BATCH( gmesa );
      gmesa->GeometryMode = g;

      gmesa->dirty |= GAMMA_UPLOAD_GEOMETRY;
   }
}

/* =============================================================
 * Miscellaneous
 */

static void gammaDDClearColor( GLcontext *ctx, const GLfloat color[4])
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   GLubyte c[4];
   UNCLAMPED_FLOAT_TO_UBYTE(c[0], color[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(c[1], color[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(c[2], color[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(c[3], color[3]);

   gmesa->ClearColor = gammaPackColor( gmesa->gammaScreen->cpp,
                                       c[0], c[1], c[2], c[3] );

   if (gmesa->gammaScreen->cpp == 2) gmesa->ClearColor |= gmesa->ClearColor<<16;
}


static void gammaDDLogicalOpcode( GLcontext *ctx, GLenum opcode )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   FLUSH_BATCH( gmesa );

   if ( ctx->Color.ColorLogicOpEnabled ) {
      gmesa->LogicalOpMode = opcode << 1 | LogicalOpModeEnable;
   } else {
      gmesa->LogicalOpMode = LogicalOpModeDisable;
   }

   gmesa->dirty |= GAMMA_UPLOAD_LOGICOP;
}

static void gammaDDDrawBuffer( GLcontext *ctx, GLenum mode )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   FLUSH_BATCH( gmesa );

   switch ( mode ) {
   case GL_FRONT_LEFT:
      gmesa->drawOffset = gmesa->readOffset = 0;
      break;
   case GL_BACK_LEFT:
      gmesa->drawOffset = gmesa->readOffset = gmesa->driScreen->fbHeight * gmesa->driScreen->fbWidth * gmesa->gammaScreen->cpp; 
      break;
   }
}

static void gammaDDReadBuffer( GLcontext *ctx, GLenum mode )
{
   /* XXX anything? */
}

/* =============================================================
 * Window position and viewport transformation
 */

void gammaUpdateWindow( GLcontext *ctx )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   __DRIdrawablePrivate *dPriv = gmesa->driDrawable;
   GLfloat xoffset = (GLfloat)dPriv->x;
   GLfloat yoffset = gmesa->driScreen->fbHeight - (GLfloat)dPriv->y - dPriv->h;
   const GLfloat *v = ctx->Viewport._WindowMap.m;

   GLfloat sx = v[MAT_SX];
   GLfloat tx = v[MAT_TX] + xoffset;
   GLfloat sy = v[MAT_SY];
   GLfloat ty = v[MAT_TY] + yoffset;
   GLfloat sz = v[MAT_SZ] * gmesa->depth_scale;
   GLfloat tz = v[MAT_TZ] * gmesa->depth_scale;

   gmesa->dirty |= GAMMA_UPLOAD_VIEWPORT;

   gmesa->ViewportScaleX = sx;
   gmesa->ViewportScaleY = sy;
   gmesa->ViewportScaleZ = sz;
   gmesa->ViewportOffsetX = tx;
   gmesa->ViewportOffsetY = ty;
   gmesa->ViewportOffsetZ = tz;
}



static void gammaDDViewport( GLcontext *ctx, GLint x, GLint y,
			    GLsizei width, GLsizei height )
{
   gammaUpdateWindow( ctx );
}

static void gammaDDDepthRange( GLcontext *ctx, GLclampd nearval,
			      GLclampd farval )
{
   gammaUpdateWindow( ctx );
}

void gammaUpdateViewportOffset( GLcontext *ctx )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   __DRIdrawablePrivate *dPriv = gmesa->driDrawable;
   GLfloat xoffset = (GLfloat)dPriv->x;
   GLfloat yoffset = gmesa->driScreen->fbHeight - (GLfloat)dPriv->y - dPriv->h;
   const GLfloat *v = ctx->Viewport._WindowMap.m;

   GLfloat tx = v[MAT_TX] + xoffset;
   GLfloat ty = v[MAT_TY] + yoffset;

   if ( gmesa->ViewportOffsetX != tx ||
        gmesa->ViewportOffsetY != ty )
   {
      gmesa->ViewportOffsetX = tx;
      gmesa->ViewportOffsetY = ty;

      gmesa->new_state |= GAMMA_NEW_WINDOW;
   }

   gmesa->new_state |= GAMMA_NEW_CLIP;
}

#if 0
/* 
 * Matrix 
 */

static void gammaLoadHWMatrix(GLcontext *ctx)
{
    gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
    const GLfloat *m;

    gmesa->TransformMode &= ~XM_XformTexture;

    switch (ctx->Transform.MatrixMode) {
    case GL_MODELVIEW:
	gmesa->TransformMode |= XM_UseModelViewMatrix;
        m = ctx->ModelviewMatrixStack.Top->m;
	CHECK_DMA_BUFFER(gmesa, 16);
	WRITEF(gmesa->buf, ModelViewMatrix0,  m[0]);
	WRITEF(gmesa->buf, ModelViewMatrix1,  m[1]);
	WRITEF(gmesa->buf, ModelViewMatrix2,  m[2]);
	WRITEF(gmesa->buf, ModelViewMatrix3,  m[3]);
	WRITEF(gmesa->buf, ModelViewMatrix4,  m[4]);
	WRITEF(gmesa->buf, ModelViewMatrix5,  m[5]);
	WRITEF(gmesa->buf, ModelViewMatrix6,  m[6]);
	WRITEF(gmesa->buf, ModelViewMatrix7,  m[7]);
	WRITEF(gmesa->buf, ModelViewMatrix8,  m[8]);
	WRITEF(gmesa->buf, ModelViewMatrix9,  m[9]);
	WRITEF(gmesa->buf, ModelViewMatrix10, m[10]);
	WRITEF(gmesa->buf, ModelViewMatrix11, m[11]);
	WRITEF(gmesa->buf, ModelViewMatrix12, m[12]);
	WRITEF(gmesa->buf, ModelViewMatrix13, m[13]);
	WRITEF(gmesa->buf, ModelViewMatrix14, m[14]);
	WRITEF(gmesa->buf, ModelViewMatrix15, m[15]);
	break;
    case GL_PROJECTION:
        m = ctx->ProjectionMatrixStack.Top->m;
	CHECK_DMA_BUFFER(gmesa, 16);
	WRITEF(gmesa->buf, ModelViewProjectionMatrix0, m[0]);
	WRITEF(gmesa->buf, ModelViewProjectionMatrix1, m[1]);
	WRITEF(gmesa->buf, ModelViewProjectionMatrix2, m[2]);
	WRITEF(gmesa->buf, ModelViewProjectionMatrix3, m[3]);
	WRITEF(gmesa->buf, ModelViewProjectionMatrix4, m[4]);
	WRITEF(gmesa->buf, ModelViewProjectionMatrix5, m[5]);
	WRITEF(gmesa->buf, ModelViewProjectionMatrix6, m[6]);
	WRITEF(gmesa->buf, ModelViewProjectionMatrix7, m[7]);
	WRITEF(gmesa->buf, ModelViewProjectionMatrix8, m[8]);
	WRITEF(gmesa->buf, ModelViewProjectionMatrix9, m[9]);
	WRITEF(gmesa->buf, ModelViewProjectionMatrix10, m[10]);
	WRITEF(gmesa->buf, ModelViewProjectionMatrix11, m[11]);
	WRITEF(gmesa->buf, ModelViewProjectionMatrix12, m[12]);
	WRITEF(gmesa->buf, ModelViewProjectionMatrix13, m[13]);
	WRITEF(gmesa->buf, ModelViewProjectionMatrix14, m[14]);
	WRITEF(gmesa->buf, ModelViewProjectionMatrix15, m[15]);
	break;
    case GL_TEXTURE:
        m = ctx->TextureMatrixStack[0].Top->m;
	CHECK_DMA_BUFFER(gmesa, 16);
	gmesa->TransformMode |= XM_XformTexture;
	WRITEF(gmesa->buf, TextureMatrix0,  m[0]);
	WRITEF(gmesa->buf, TextureMatrix1,  m[1]);
	WRITEF(gmesa->buf, TextureMatrix2,  m[2]);
	WRITEF(gmesa->buf, TextureMatrix3,  m[3]);
	WRITEF(gmesa->buf, TextureMatrix4,  m[4]);
	WRITEF(gmesa->buf, TextureMatrix5,  m[5]);
	WRITEF(gmesa->buf, TextureMatrix6,  m[6]);
	WRITEF(gmesa->buf, TextureMatrix7,  m[7]);
	WRITEF(gmesa->buf, TextureMatrix8,  m[8]);
	WRITEF(gmesa->buf, TextureMatrix9,  m[9]);
	WRITEF(gmesa->buf, TextureMatrix10,  m[10]);
	WRITEF(gmesa->buf, TextureMatrix11,  m[11]);
	WRITEF(gmesa->buf, TextureMatrix12,  m[12]);
	WRITEF(gmesa->buf, TextureMatrix13,  m[13]);
	WRITEF(gmesa->buf, TextureMatrix14,  m[14]);
	WRITEF(gmesa->buf, TextureMatrix15,  m[15]);
	break;

    default:
	/* ERROR!!! -- how did this happen? */
	break;
    }

    gmesa->dirty |= GAMMA_UPLOAD_TRANSFORM;
}
#endif

/* =============================================================
 * State enable/disable
 */

static void gammaDDEnable( GLcontext *ctx, GLenum cap, GLboolean state )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   switch ( cap ) {
   case GL_ALPHA_TEST:
   case GL_BLEND:
      FLUSH_BATCH( gmesa );
      gmesa->new_state |= GAMMA_NEW_ALPHA;
      break;

   case GL_CULL_FACE:
      FLUSH_BATCH( gmesa );
      gmesa->new_state |= GAMMA_NEW_CULL;
      break;

   case GL_DEPTH_TEST:
      FLUSH_BATCH( gmesa );
      gmesa->new_state |= GAMMA_NEW_DEPTH;
      break;

   case GL_DITHER:
      do {
	 u_int32_t d = gmesa->DitherMode;
	 FLUSH_BATCH( gmesa );

	 if ( state ) {
	    d |=  DM_DitherEnable;
	 } else {
	    d &= ~DM_DitherEnable;
	 }

	 if ( gmesa->DitherMode != d ) {
	    gmesa->DitherMode = d;
	    gmesa->dirty |= GAMMA_UPLOAD_DITHER;
	 }
      } while (0);
      break;

#if 0
   case GL_FOG:
      FLUSH_BATCH( gmesa );
      gmesa->new_state |= GAMMA_NEW_FOG;
      break;
#endif

   case GL_INDEX_LOGIC_OP:
   case GL_COLOR_LOGIC_OP:
      FLUSH_BATCH( gmesa );
      gmesa->new_state |= GAMMA_NEW_LOGICOP;
      break;

#if ENABLELIGHTING
   case GL_LIGHTING:
      do {
	 u_int32_t l = gmesa->LightingMode;
	 FLUSH_BATCH( gmesa );

	 if ( state ) {
	    l |=  LightingModeEnable;
	 } else {
	    l &= ~LightingModeEnable;
	 }

	 if ( gmesa->LightingMode != l ) {
	    gmesa->LightingMode = l;
	    gmesa->dirty |= GAMMA_UPLOAD_LIGHT;
	 }
      } while (0);
      break;

   case GL_COLOR_MATERIAL:
      do {
	 u_int32_t m = gmesa->MaterialMode;
	 FLUSH_BATCH( gmesa );

	 if ( state ) {
	    m |=  MaterialModeEnable;
	 } else {
	    m &= ~MaterialModeEnable;
	 }

	 if ( gmesa->MaterialMode != m ) {
	    gmesa->MaterialMode = m;
	    gmesa->dirty |= GAMMA_UPLOAD_LIGHT;
	 }
      } while (0);
      break;
#endif

   case GL_LINE_SMOOTH:
      FLUSH_BATCH( gmesa );
      if ( state ) {
         gmesa->AntialiasMode |= AntialiasModeEnable;
         gmesa->LineMode |= LM_AntialiasEnable;
      } else {
         gmesa->AntialiasMode &= ~AntialiasModeEnable;
         gmesa->LineMode &= ~LM_AntialiasEnable;
      }
      gmesa->dirty |= GAMMA_UPLOAD_LINEMODE;
      break;

   case GL_POINT_SMOOTH:
      FLUSH_BATCH( gmesa );
      if ( state ) {
         gmesa->AntialiasMode |= AntialiasModeEnable;
         gmesa->PointMode |= PM_AntialiasEnable;
      } else {
         gmesa->AntialiasMode &= ~AntialiasModeEnable;
         gmesa->PointMode &= ~PM_AntialiasEnable;
      }
      gmesa->dirty |= GAMMA_UPLOAD_POINTMODE;
      break;

   case GL_POLYGON_SMOOTH:
      FLUSH_BATCH( gmesa );
      if ( state ) {
         gmesa->AntialiasMode |= AntialiasModeEnable;
         gmesa->TriangleMode |= TM_AntialiasEnable;
      } else {
         gmesa->AntialiasMode &= ~AntialiasModeEnable;
         gmesa->TriangleMode &= ~TM_AntialiasEnable;
      }
      gmesa->dirty |= GAMMA_UPLOAD_TRIMODE;
      break;

   case GL_SCISSOR_TEST:
      FLUSH_BATCH( gmesa );
      gmesa->new_state |= GAMMA_NEW_CLIP;
      break;

   case GL_POLYGON_OFFSET_FILL:
   case GL_POLYGON_OFFSET_POINT:
   case GL_POLYGON_OFFSET_LINE:
      FLUSH_BATCH( gmesa );
      gmesa->new_state |= GAMMA_NEW_POLYGON;
      break;

   case GL_LINE_STIPPLE:
      FLUSH_BATCH( gmesa );
      if ( state )
         gmesa->LineMode |= LM_StippleEnable;
      else
         gmesa->LineMode &= ~LM_StippleEnable;
      gmesa->dirty |= GAMMA_UPLOAD_LINEMODE;
      break;

   case GL_POLYGON_STIPPLE:
      FLUSH_BATCH( gmesa );
      gmesa->new_state |= GAMMA_NEW_STIPPLE;
      break;

   default:
      return;
   }
}

/* =============================================================
 * State initialization, management
 */


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
void gammaEmitHwState( gammaContextPtr gmesa )
{
    if (!gmesa->driDrawable) return;

    if (!gmesa->dirty) return;

#ifdef DO_VALIDATE
    /* Flush any partially filled buffers */
    FLUSH_DMA_BUFFER(gmesa);

    DRM_SPINLOCK(&gmesa->driScreen->pSAREA->drawable_lock,
		 gmesa->driScreen->drawLockID);
    VALIDATE_DRAWABLE_INFO_NO_LOCK(gmesa);
#endif

    if (gmesa->dirty & GAMMA_UPLOAD_VIEWPORT) {
	gmesa->dirty &= ~GAMMA_UPLOAD_VIEWPORT;
 	CHECK_DMA_BUFFER(gmesa, 6);
 	WRITEF(gmesa->buf, ViewPortOffsetX, gmesa->ViewportOffsetX);
 	WRITEF(gmesa->buf, ViewPortOffsetY, gmesa->ViewportOffsetY);
 	WRITEF(gmesa->buf, ViewPortOffsetZ, gmesa->ViewportOffsetZ);
 	WRITEF(gmesa->buf, ViewPortScaleX, gmesa->ViewportScaleX);
 	WRITEF(gmesa->buf, ViewPortScaleY, gmesa->ViewportScaleY);
 	WRITEF(gmesa->buf, ViewPortScaleZ, gmesa->ViewportScaleZ);
    }
    if ( (gmesa->dirty & GAMMA_UPLOAD_POINTMODE) ||
	 (gmesa->dirty & GAMMA_UPLOAD_LINEMODE) ||
	 (gmesa->dirty & GAMMA_UPLOAD_TRIMODE) ) {
 	CHECK_DMA_BUFFER(gmesa, 1);
	WRITE(gmesa->buf, AntialiasMode, gmesa->AntialiasMode);
    }
    if (gmesa->dirty & GAMMA_UPLOAD_POINTMODE) {
	gmesa->dirty &= ~GAMMA_UPLOAD_POINTMODE;
 	CHECK_DMA_BUFFER(gmesa, 1);
 	WRITE(gmesa->buf, PointMode, gmesa->PointMode);
    }
    if (gmesa->dirty & GAMMA_UPLOAD_LINEMODE) {
	gmesa->dirty &= ~GAMMA_UPLOAD_LINEMODE;
 	CHECK_DMA_BUFFER(gmesa, 2);
 	WRITE(gmesa->buf, LineMode, gmesa->LineMode);
 	WRITE(gmesa->buf, LineStippleMode, gmesa->LineMode);
    }
    if (gmesa->dirty & GAMMA_UPLOAD_TRIMODE) {
	gmesa->dirty &= ~GAMMA_UPLOAD_TRIMODE;
 	CHECK_DMA_BUFFER(gmesa, 1);
 	WRITE(gmesa->buf, TriangleMode, gmesa->TriangleMode);
    }
    if (gmesa->dirty & GAMMA_UPLOAD_FOG) {
	GLchan c[3], col;
   	UNCLAMPED_FLOAT_TO_RGB_CHAN( c, gmesa->glCtx->Fog.Color );
	col = gammaPackColor(4, c[0], c[1], c[2], 0);
	gmesa->dirty &= ~GAMMA_UPLOAD_FOG;
	CHECK_DMA_BUFFER(gmesa, 5);
#if 0
	WRITE(gmesa->buf, FogMode, gmesa->FogMode);
	WRITE(gmesa->buf, FogColor, col);
	WRITEF(gmesa->buf, FStart, gmesa->glCtx->Fog.Start);
#endif
	WRITEF(gmesa->buf, FogEnd, gmesa->glCtx->Fog.End);
	WRITEF(gmesa->buf, FogDensity, gmesa->glCtx->Fog.Density);
	WRITEF(gmesa->buf, FogScale, 
		1.0f/(gmesa->glCtx->Fog.End - gmesa->glCtx->Fog.Start));
    }
    if (gmesa->dirty & GAMMA_UPLOAD_DITHER) {
	gmesa->dirty &= ~GAMMA_UPLOAD_DITHER;
	CHECK_DMA_BUFFER(gmesa, 1);
	WRITE(gmesa->buf, DitherMode, gmesa->DitherMode);
    }
    if (gmesa->dirty & GAMMA_UPLOAD_LOGICOP) {
	gmesa->dirty &= ~GAMMA_UPLOAD_LOGICOP;
	CHECK_DMA_BUFFER(gmesa, 1);
	WRITE(gmesa->buf, LogicalOpMode, gmesa->LogicalOpMode);
    }
    if (gmesa->dirty & GAMMA_UPLOAD_CLIP) {
	gmesa->dirty &= ~GAMMA_UPLOAD_CLIP;
	CHECK_DMA_BUFFER(gmesa, 3);
	WRITE(gmesa->buf, ScissorMinXY, gmesa->ScissorMinXY);
	WRITE(gmesa->buf, ScissorMaxXY, gmesa->ScissorMaxXY);
	WRITE(gmesa->buf, ScissorMode, gmesa->ScissorMode);
    }
    if (gmesa->dirty & GAMMA_UPLOAD_MASKS) {
	gmesa->dirty &= ~GAMMA_UPLOAD_MASKS;
	CHECK_DMA_BUFFER(gmesa, 1);
	WRITE(gmesa->buf, FBHardwareWriteMask, gmesa->FBHardwareWriteMask);
    }
    if (gmesa->dirty & GAMMA_UPLOAD_ALPHA) {
	gmesa->dirty &= ~GAMMA_UPLOAD_ALPHA;
	CHECK_DMA_BUFFER(gmesa, 1);
	WRITE(gmesa->buf, AlphaTestMode, gmesa->AlphaTestMode);
    }
    if (gmesa->dirty & GAMMA_UPLOAD_BLEND) {
	gmesa->dirty &= ~GAMMA_UPLOAD_BLEND;
	CHECK_DMA_BUFFER(gmesa, 1);
	WRITE(gmesa->buf, AlphaBlendMode, gmesa->AlphaBlendMode);
    } 
    CHECK_DMA_BUFFER(gmesa, 1);
    if (gmesa->glCtx->Color.BlendEnabled || gmesa->glCtx->Color.AlphaEnabled) {
    	WRITE(gmesa->buf, FBReadMode, gmesa->FBReadMode | gmesa->AB_FBReadMode_Save);
    } else {
    	WRITE(gmesa->buf, FBReadMode, gmesa->FBReadMode);
    }
    if (gmesa->dirty & GAMMA_UPLOAD_LIGHT) {
	gmesa->dirty &= ~GAMMA_UPLOAD_LIGHT;
	CHECK_DMA_BUFFER(gmesa, 2);
	WRITE(gmesa->buf, LightingMode, gmesa->LightingMode);
	WRITE(gmesa->buf, MaterialMode, gmesa->MaterialMode);
    }
    if (gmesa->dirty & GAMMA_UPLOAD_SHADE) {
	gmesa->dirty &= ~GAMMA_UPLOAD_SHADE;
	CHECK_DMA_BUFFER(gmesa, 1);
	WRITE(gmesa->buf, ColorDDAMode, gmesa->ColorDDAMode);
    }
    if (gmesa->dirty & GAMMA_UPLOAD_POLYGON) {
	gmesa->dirty &= ~GAMMA_UPLOAD_POLYGON;
	CHECK_DMA_BUFFER(gmesa, 2);
	WRITEF(gmesa->buf, PolygonOffsetBias, gmesa->glCtx->Polygon.OffsetUnits);
	WRITEF(gmesa->buf, PolygonOffsetFactor, gmesa->glCtx->Polygon.OffsetFactor);
    }
    if (gmesa->dirty & GAMMA_UPLOAD_STIPPLE) {
	gmesa->dirty &= ~GAMMA_UPLOAD_STIPPLE;
	CHECK_DMA_BUFFER(gmesa, 33);
	WRITE(gmesa->buf, AreaStippleMode, gmesa->AreaStippleMode);
	WRITE(gmesa->buf, AreaStipplePattern0, gmesa->glCtx->PolygonStipple[0]);
	WRITE(gmesa->buf, AreaStipplePattern1, gmesa->glCtx->PolygonStipple[1]);
	WRITE(gmesa->buf, AreaStipplePattern2, gmesa->glCtx->PolygonStipple[2]);
	WRITE(gmesa->buf, AreaStipplePattern3, gmesa->glCtx->PolygonStipple[3]);
	WRITE(gmesa->buf, AreaStipplePattern4, gmesa->glCtx->PolygonStipple[4]);
	WRITE(gmesa->buf, AreaStipplePattern5, gmesa->glCtx->PolygonStipple[5]);
	WRITE(gmesa->buf, AreaStipplePattern6, gmesa->glCtx->PolygonStipple[6]);
	WRITE(gmesa->buf, AreaStipplePattern7, gmesa->glCtx->PolygonStipple[7]);
	WRITE(gmesa->buf, AreaStipplePattern8, gmesa->glCtx->PolygonStipple[8]);
	WRITE(gmesa->buf, AreaStipplePattern9, gmesa->glCtx->PolygonStipple[9]);
	WRITE(gmesa->buf, AreaStipplePattern10, gmesa->glCtx->PolygonStipple[10]);
	WRITE(gmesa->buf, AreaStipplePattern11, gmesa->glCtx->PolygonStipple[11]);
	WRITE(gmesa->buf, AreaStipplePattern12, gmesa->glCtx->PolygonStipple[12]);
	WRITE(gmesa->buf, AreaStipplePattern13, gmesa->glCtx->PolygonStipple[13]);
	WRITE(gmesa->buf, AreaStipplePattern14, gmesa->glCtx->PolygonStipple[14]);
	WRITE(gmesa->buf, AreaStipplePattern15, gmesa->glCtx->PolygonStipple[15]);
	WRITE(gmesa->buf, AreaStipplePattern16, gmesa->glCtx->PolygonStipple[16]);
	WRITE(gmesa->buf, AreaStipplePattern17, gmesa->glCtx->PolygonStipple[17]);
	WRITE(gmesa->buf, AreaStipplePattern18, gmesa->glCtx->PolygonStipple[18]);
	WRITE(gmesa->buf, AreaStipplePattern19, gmesa->glCtx->PolygonStipple[19]);
	WRITE(gmesa->buf, AreaStipplePattern20, gmesa->glCtx->PolygonStipple[20]);
	WRITE(gmesa->buf, AreaStipplePattern21, gmesa->glCtx->PolygonStipple[21]);
	WRITE(gmesa->buf, AreaStipplePattern22, gmesa->glCtx->PolygonStipple[22]);
	WRITE(gmesa->buf, AreaStipplePattern23, gmesa->glCtx->PolygonStipple[23]);
	WRITE(gmesa->buf, AreaStipplePattern24, gmesa->glCtx->PolygonStipple[24]);
	WRITE(gmesa->buf, AreaStipplePattern25, gmesa->glCtx->PolygonStipple[25]);
	WRITE(gmesa->buf, AreaStipplePattern26, gmesa->glCtx->PolygonStipple[26]);
	WRITE(gmesa->buf, AreaStipplePattern27, gmesa->glCtx->PolygonStipple[27]);
	WRITE(gmesa->buf, AreaStipplePattern28, gmesa->glCtx->PolygonStipple[28]);
	WRITE(gmesa->buf, AreaStipplePattern29, gmesa->glCtx->PolygonStipple[29]);
	WRITE(gmesa->buf, AreaStipplePattern30, gmesa->glCtx->PolygonStipple[30]);
	WRITE(gmesa->buf, AreaStipplePattern31, gmesa->glCtx->PolygonStipple[31]);
    }
    if (gmesa->dirty & GAMMA_UPLOAD_DEPTH) {
	gmesa->dirty &= ~GAMMA_UPLOAD_DEPTH;
	CHECK_DMA_BUFFER(gmesa, 4);
	WRITE(gmesa->buf, DepthMode,  gmesa->DepthMode);
	WRITE(gmesa->buf, DeltaMode,  gmesa->DeltaMode);
	WRITE(gmesa->buf, GLINTWindow,gmesa->Window | (gmesa->FrameCount << 9));
	WRITE(gmesa->buf, LBReadMode, gmesa->LBReadMode);
    }
    if (gmesa->dirty & GAMMA_UPLOAD_GEOMETRY) {
	gmesa->dirty &= ~GAMMA_UPLOAD_GEOMETRY;
	CHECK_DMA_BUFFER(gmesa, 1);
	WRITE(gmesa->buf, GeometryMode, gmesa->GeometryMode);
    }
    if (gmesa->dirty & GAMMA_UPLOAD_TRANSFORM) {
	gmesa->dirty &= ~GAMMA_UPLOAD_TRANSFORM;
	CHECK_DMA_BUFFER(gmesa, 1);
	WRITE(gmesa->buf, TransformMode, gmesa->TransformMode);
    }
    if (gmesa->dirty & GAMMA_UPLOAD_TEX0) {
	gammaTextureObjectPtr curTex = gmesa->CurrentTexObj[0];
	gmesa->dirty &= ~GAMMA_UPLOAD_TEX0;
	if (curTex) {
	CHECK_DMA_BUFFER(gmesa, 21);
	WRITE(gmesa->buf, GeometryMode, gmesa->GeometryMode | GM_TextureEnable);
	WRITE(gmesa->buf, DeltaMode, gmesa->DeltaMode | DM_TextureEnable);
	WRITE(gmesa->buf, TextureAddressMode, curTex->TextureAddressMode);
	WRITE(gmesa->buf, TextureReadMode, curTex->TextureReadMode);
	WRITE(gmesa->buf, TextureColorMode, curTex->TextureColorMode);
	WRITE(gmesa->buf, TextureFilterMode, curTex->TextureFilterMode);
	WRITE(gmesa->buf, TextureFormat, curTex->TextureFormat);
	WRITE(gmesa->buf, GLINTBorderColor, curTex->TextureBorderColor);
	WRITE(gmesa->buf, TxBaseAddr0, curTex->TextureBaseAddr[0]);
	WRITE(gmesa->buf, TxBaseAddr1, curTex->TextureBaseAddr[1]);
	WRITE(gmesa->buf, TxBaseAddr2, curTex->TextureBaseAddr[2]);
	WRITE(gmesa->buf, TxBaseAddr3, curTex->TextureBaseAddr[3]);
	WRITE(gmesa->buf, TxBaseAddr4, curTex->TextureBaseAddr[4]);
	WRITE(gmesa->buf, TxBaseAddr5, curTex->TextureBaseAddr[5]);
	WRITE(gmesa->buf, TxBaseAddr6, curTex->TextureBaseAddr[6]);
	WRITE(gmesa->buf, TxBaseAddr7, curTex->TextureBaseAddr[7]);
	WRITE(gmesa->buf, TxBaseAddr8, curTex->TextureBaseAddr[8]);
	WRITE(gmesa->buf, TxBaseAddr9, curTex->TextureBaseAddr[9]);
	WRITE(gmesa->buf, TxBaseAddr10, curTex->TextureBaseAddr[10]);
	WRITE(gmesa->buf, TxBaseAddr11, curTex->TextureBaseAddr[11]);
	WRITE(gmesa->buf, TextureCacheControl, (TCC_Enable | TCC_Invalidate));
	} else {
	CHECK_DMA_BUFFER(gmesa, 6);
	WRITE(gmesa->buf, GeometryMode, gmesa->GeometryMode);
	WRITE(gmesa->buf, DeltaMode, gmesa->DeltaMode);
	WRITE(gmesa->buf, TextureAddressMode, TextureAddressModeDisable);
	WRITE(gmesa->buf, TextureReadMode, TextureReadModeDisable);
	WRITE(gmesa->buf, TextureFilterMode, TextureFilterModeDisable);
	WRITE(gmesa->buf, TextureColorMode, TextureColorModeDisable);
	}
    }
#ifdef DO_VALIDATE
    PROCESS_DMA_BUFFER_TOP_HALF(gmesa);

    DRM_SPINUNLOCK(&gmesa->driScreen->pSAREA->drawable_lock,
		   gmesa->driScreen->drawLockID);
    VALIDATE_DRAWABLE_INFO_NO_LOCK_POST(gmesa);

    PROCESS_DMA_BUFFER_BOTTOM_HALF(gmesa);
#endif
}

void gammaDDUpdateHWState( GLcontext *ctx )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   int new_state = gmesa->new_state;

   if ( new_state )
   {
      FLUSH_BATCH( gmesa );

      gmesa->new_state = 0;

      /* Update the various parts of the context's state.
       */
      if ( new_state & GAMMA_NEW_ALPHA )
	 gammaUpdateAlphaMode( ctx );

      if ( new_state & GAMMA_NEW_DEPTH )
	 gammaUpdateZMode( ctx );

      if ( new_state & GAMMA_NEW_FOG )
	 gammaUpdateFogAttrib( ctx );

      if ( new_state & GAMMA_NEW_CLIP )
	 gammaUpdateClipping( ctx );

      if ( new_state & GAMMA_NEW_POLYGON )
	 gammaUpdatePolygon( ctx );

      if ( new_state & GAMMA_NEW_CULL )
	 gammaUpdateCull( ctx );

      if ( new_state & GAMMA_NEW_MASKS )
	 gammaUpdateMasks( ctx );

      if ( new_state & GAMMA_NEW_WINDOW )
	 gammaUpdateWindow( ctx );

      if ( new_state & GAMMA_NEW_STIPPLE )
	 gammaUpdateStipple( ctx );
   }

   /* HACK ! */

   gammaEmitHwState( gmesa );
}


static void gammaDDUpdateState( GLcontext *ctx, GLuint new_state )
{
   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _vbo_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   GAMMA_CONTEXT(ctx)->new_gl_state |= new_state;
}


/* Initialize the context's hardware state.
 */
void gammaDDInitState( gammaContextPtr gmesa )
{
   gmesa->new_state = 0;
}

/* Initialize the driver's state functions.
 */
void gammaDDInitStateFuncs( GLcontext *ctx )
{
   ctx->Driver.UpdateState		= gammaDDUpdateState;

   ctx->Driver.Clear			= gammaDDClear;
   ctx->Driver.ClearIndex		= NULL;
   ctx->Driver.ClearColor		= gammaDDClearColor;
   ctx->Driver.DrawBuffer		= gammaDDDrawBuffer;
   ctx->Driver.ReadBuffer		= gammaDDReadBuffer;

   ctx->Driver.IndexMask		= NULL;
   ctx->Driver.ColorMask		= gammaDDColorMask;

   ctx->Driver.AlphaFunc		= gammaDDAlphaFunc;
   ctx->Driver.BlendEquationSeparate	= gammaDDBlendEquationSeparate;
   ctx->Driver.BlendFuncSeparate	= gammaDDBlendFuncSeparate;
   ctx->Driver.ClearDepth		= gammaDDClearDepth;
   ctx->Driver.CullFace			= gammaDDCullFace;
   ctx->Driver.FrontFace		= gammaDDFrontFace;
   ctx->Driver.DepthFunc		= gammaDDDepthFunc;
   ctx->Driver.DepthMask		= gammaDDDepthMask;
   ctx->Driver.DepthRange		= gammaDDDepthRange;
   ctx->Driver.Enable			= gammaDDEnable;
   ctx->Driver.Finish			= gammaDDFinish;
   ctx->Driver.Flush			= gammaDDFlush;
#if 0
   ctx->Driver.Fogfv			= gammaDDFogfv;
#endif
   ctx->Driver.Hint			= NULL;
   ctx->Driver.LineWidth		= gammaDDLineWidth;
   ctx->Driver.LineStipple		= gammaDDLineStipple;
#if ENABLELIGHTING
   ctx->Driver.Lightfv			= gammaDDLightfv; 
   ctx->Driver.LightModelfv		= gammaDDLightModelfv;
#endif
   ctx->Driver.LogicOpcode		= gammaDDLogicalOpcode;
   ctx->Driver.PointSize		= gammaDDPointSize;
   ctx->Driver.PolygonMode		= gammaDDPolygonMode;
   ctx->Driver.PolygonStipple		= gammaDDPolygonStipple;
   ctx->Driver.Scissor			= gammaDDScissor;
   ctx->Driver.ShadeModel		= gammaDDShadeModel;
   ctx->Driver.Viewport			= gammaDDViewport;
}
