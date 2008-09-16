/*
 * Copyright 2002 by Alan Hourihane, Sychdyn, North Wales, UK.
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
 * Authors:  Alan Hourihane, <alanh@fairlite.demon.co.uk>
 *
 * Trident CyberBladeXP driver.
 *
 */
#include "trident_context.h"
#include "trident_lock.h"
#include "vbo/vbo.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "framebuffer.h"

#define TRIDENTPACKCOLOR332(r, g, b)					\
   (((r) & 0xe0) | (((g) & 0xe0) >> 3) | (((b) & 0xc0) >> 6))

#define TRIDENTPACKCOLOR1555(r, g, b, a)					\
   ((((r) & 0xf8) << 7) | (((g) & 0xf8) << 2) | (((b) & 0xf8) >> 3) |	\
    ((a) ? 0x8000 : 0))

#define TRIDENTPACKCOLOR565(r, g, b)					\
   ((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | (((b) & 0xf8) >> 3))

#define TRIDENTPACKCOLOR888(r, g, b)					\
   (((r) << 16) | ((g) << 8) | (b))

#define TRIDENTPACKCOLOR8888(r, g, b, a)					\
   (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

#define TRIDENTPACKCOLOR4444(r, g, b, a)					\
   ((((a) & 0xf0) << 8) | (((r) & 0xf0) << 4) | ((g) & 0xf0) | ((b) >> 4))

static __inline__ GLuint tridentPackColor( GLuint cpp,
					  GLubyte r, GLubyte g,
					  GLubyte b, GLubyte a )
{
   switch ( cpp ) {
   case 2:
      return TRIDENTPACKCOLOR565( r, g, b );
   case 4:
      return TRIDENTPACKCOLOR8888( r, g, b, a );
   default:
      return 0;
   }
}

void tridentUploadHwStateLocked( tridentContextPtr tmesa )
{
   unsigned char *MMIO = tmesa->tridentScreen->mmio.map;
#if 0
   ATISAREAPrivPtr sarea = tmesa->sarea;
   trident_context_regs_t *regs = &(sarea->ContextState);
#endif

   if ( tmesa->dirty & TRIDENT_UPLOAD_COMMAND_D ) {
      MMIO_OUT32(MMIO, 0x00281C, tmesa->commandD );
      tmesa->dirty &= ~TRIDENT_UPLOAD_COMMAND_D;
   }

   if ( tmesa->dirty & TRIDENT_UPLOAD_CLIPRECTS ) {
      /* XXX FIX ME ! */
      MMIO_OUT32(MMIO, 0x002C80 , 0x20008000 | tmesa->tridentScreen->height );    
      MMIO_OUT32(MMIO, 0x002C84 , 0x20000000 | tmesa->tridentScreen->width );    
      tmesa->dirty &= ~TRIDENT_UPLOAD_CLIPRECTS;
   }

   tmesa->dirty = 0;
}

/* Copy the back color buffer to the front color buffer.
 */
void tridentCopyBuffer( const __DRIdrawablePrivate *dPriv )
{
   unsigned char *MMIO;
   tridentContextPtr tmesa;
   GLint nbox, i;
   int busy;
   drm_clip_rect_t *pbox;

   assert(dPriv);
   assert(dPriv->driContextPriv);
   assert(dPriv->driContextPriv->driverPrivate);

   tmesa = (tridentContextPtr) dPriv->driContextPriv->driverPrivate;
   MMIO = tmesa->tridentScreen->mmio.map;

   LOCK_HARDWARE( tmesa );

   /* use front buffer cliprects */
   nbox = dPriv->numClipRects;
   pbox = dPriv->pClipRects;

   for ( i = 0 ; i < nbox ; i++ ) {
#if 0
      GLint nr = MIN2( i + MACH64_NR_SAREA_CLIPRECTS , nbox );
      drm_clip_rect_t *b = tmesa->sarea->boxes;
      GLint n = 0;

      for ( ; i < nr ; i++ ) {
	 *b++ = pbox[i];
	 n++;
      }
      tmesa->sarea->nbox = n;
#endif

    MMIO_OUT32(MMIO, 0x2150, tmesa->tridentScreen->frontPitch << 20 | tmesa->tridentScreen->frontOffset>>4);
    MMIO_OUT32(MMIO, 0x2154, tmesa->tridentScreen->backPitch << 20 | tmesa->tridentScreen->backOffset>>4);
    MMIO_OUT8(MMIO, 0x2127, 0xCC); /* Copy Rop */
    MMIO_OUT32(MMIO, 0x2128, 0x4); /* scr2scr */
    MMIO_OUT32(MMIO, 0x2138, (pbox->x1 << 16) | pbox->y1);
    MMIO_OUT32(MMIO, 0x213C, (pbox->x1 << 16) | pbox->y1);
    MMIO_OUT32(MMIO, 0x2140, (pbox->x2 - pbox->x1) << 16 | (pbox->y2 - pbox->y1) );
    MMIO_OUT8(MMIO, 0x2124, 0x01); /* BLT */
#define GE_BUSY 0x80
    for (;;) {
	busy = MMIO_IN8(MMIO, 0x2120);
	if ( !(busy & GE_BUSY) )
		break;
    }
   }

   UNLOCK_HARDWARE( tmesa );

#if 0
   tmesa->dirty |= (MACH64_UPLOAD_CONTEXT |
		    MACH64_UPLOAD_MISC |
		    MACH64_UPLOAD_CLIPRECTS);
#endif
}


static void tridentDDClear( GLcontext *ctx, GLbitfield mask )
{
   tridentContextPtr tmesa = TRIDENT_CONTEXT(ctx);
   unsigned char *MMIO = tmesa->tridentScreen->mmio.map;
   int busy;
   GLuint flags = 0;
   GLint i;
   GLint cx, cy, cw, ch;

#define DRM_TRIDENT_FRONT	0x01
#define DRM_TRIDENT_BACK	0x02
#define DRM_TRIDENT_DEPTH	0x04

   if ( tmesa->new_state )
      tridentDDUpdateHWState( ctx );

   if ( mask & BUFFER_BIT_FRONT_LEFT ) {
      flags |= DRM_TRIDENT_FRONT;
      mask &= ~BUFFER_BIT_FRONT_LEFT;
   }

   if ( mask & BUFFER_BIT_BACK_LEFT ) {
      flags |= DRM_TRIDENT_BACK;
      mask &= ~BUFFER_BIT_BACK_LEFT;
   }

   if ( ( mask & BUFFER_BIT_DEPTH ) && ctx->Depth.Mask ) {
      flags |= DRM_TRIDENT_DEPTH;
      mask &= ~BUFFER_BIT_DEPTH;
   }

   LOCK_HARDWARE(tmesa);

   /* get region after locking: */
   cx = ctx->DrawBuffer->_Xmin;
   cy = ctx->DrawBuffer->_Ymin;
   cw = ctx->DrawBuffer->_Xmax - cx;
   ch = ctx->DrawBuffer->_Ymax - cy;

   if ( flags ) {
   
      cx += tmesa->drawX;
      cy += tmesa->drawY;
   
      /* HACK!!!
       */
      if ( tmesa->dirty & ~TRIDENT_UPLOAD_CLIPRECTS ) {
         tridentUploadHwStateLocked( tmesa );
      }
   
      for ( i = 0 ; i < tmesa->numClipRects ; i++ ) {
#if 0
         int nr = MIN2( i + TRIDENT_NR_SAREA_CLIPRECTS, tmesa->numClipRects );
         drm_clip_rect_t *box = tmesa->pClipRects;
         drm_clip_rect_t *b = tmesa->sarea->boxes;
         GLint n = 0;
   
         if ( !all ) {
	    for ( ; i < nr ; i++ ) {
	       GLint x = box[i].x1;
	       GLint y = box[i].y1;
	       GLint w = box[i].x2 - x;
	       GLint h = box[i].y2 - y;
	       
	       if ( x < cx ) w -= cx - x, x = cx;
	       if ( y < cy ) h -= cy - y, y = cy;
	       if ( x + w > cx + cw ) w = cx + cw - x;
	       if ( y + h > cy + ch ) h = cy + ch - y;
	       if ( w <= 0 ) continue;
	       if ( h <= 0 ) continue;
	       
	       b->x1 = x;
	       b->y1 = y;
	       b->x2 = x + w;
	       b->y2 = y + h;
	       b++;
	       n++;
	    }
         } else {
	    for ( ; i < nr ; i++ ) {
	       *b++ = box[i];
	       n++;
	    }
         }
   
         tmesa->sarea->nbox = n;
#endif
   
if (flags & DRM_TRIDENT_BACK) {
    MMIO_OUT32(MMIO, 0x2150, tmesa->tridentScreen->backPitch << 20 | tmesa->tridentScreen->backOffset>>4);
         MMIO_OUT8(MMIO, 0x2127, 0xF0); /* Pat Rop */
         MMIO_OUT32(MMIO, 0x2158, tmesa->ClearColor);
         MMIO_OUT32(MMIO, 0x2128, 0x4000); /* solidfill */
         MMIO_OUT32(MMIO, 0x2138, cx << 16 | cy);
         MMIO_OUT32(MMIO, 0x2140, cw << 16 | ch);
         MMIO_OUT8(MMIO, 0x2124, 0x01); /* BLT */
#define GE_BUSY 0x80
	 for (;;) {
		busy = MMIO_IN8(MMIO, 0x2120);
		if ( !(busy & GE_BUSY) )
			break;
	 }
}
if (flags & DRM_TRIDENT_DEPTH) {
    MMIO_OUT32(MMIO, 0x2150, tmesa->tridentScreen->depthPitch << 20 | tmesa->tridentScreen->depthOffset>>4);
         MMIO_OUT8(MMIO, 0x2127, 0xF0); /* Pat Rop */
         MMIO_OUT32(MMIO, 0x2158, tmesa->ClearColor);
         MMIO_OUT32(MMIO, 0x2128, 0x4000); /* solidfill */
         MMIO_OUT32(MMIO, 0x2138, cx << 16 | cy);
         MMIO_OUT32(MMIO, 0x2140, cw << 16 | ch);
         MMIO_OUT8(MMIO, 0x2124, 0x01); /* BLT */
#define GE_BUSY 0x80
	 for (;;) {
		busy = MMIO_IN8(MMIO, 0x2120);
		if ( !(busy & GE_BUSY) )
			break;
	 }
}
    MMIO_OUT32(MMIO, 0x2150, tmesa->tridentScreen->frontPitch << 20 | tmesa->tridentScreen->frontOffset>>4);
if (flags & DRM_TRIDENT_FRONT) {
         MMIO_OUT8(MMIO, 0x2127, 0xF0); /* Pat Rop */
         MMIO_OUT32(MMIO, 0x2158, tmesa->ClearColor);
         MMIO_OUT32(MMIO, 0x2128, 0x4000); /* solidfill */
         MMIO_OUT32(MMIO, 0x2138, cx << 16 | cy);
         MMIO_OUT32(MMIO, 0x2140, cw << 16 | ch);
         MMIO_OUT8(MMIO, 0x2124, 0x01); /* BLT */
#define GE_BUSY 0x80
	 for (;;) {
		busy = MMIO_IN8(MMIO, 0x2120);
		if ( !(busy & GE_BUSY) )
			break;
	 }
}
   
      }
   
#if 0
      tmesa->dirty |= (TRIDENT_UPLOAD_CONTEXT |
   		       TRIDENT_UPLOAD_MISC |
   		       TRIDENT_UPLOAD_CLIPRECTS);
#endif
   }

   UNLOCK_HARDWARE(tmesa);

   if ( mask )
      _swrast_Clear( ctx, mask );
}

static void tridentDDShadeModel( GLcontext *ctx, GLenum mode )
{
   tridentContextPtr tmesa = TRIDENT_CONTEXT(ctx);
   GLuint s = tmesa->commandD;

#define TRIDENT_FLAT_SHADE 			0x000000E0
#define TRIDENT_FLAT_SHADE_VERTEX_C		0x00000060
#define TRIDENT_FLAT_SHADE_GOURAUD		0x00000080

   s &= ~TRIDENT_FLAT_SHADE;

   switch ( mode ) {
   case GL_FLAT:
      s |= TRIDENT_FLAT_SHADE_VERTEX_C;
      break;
   case GL_SMOOTH:
      s |= TRIDENT_FLAT_SHADE_GOURAUD;
      break;
   default:
      return;
   }

   if ( tmesa->commandD != s ) {
      tmesa->commandD = s;

      tmesa->dirty |= TRIDENT_UPLOAD_COMMAND_D;
   }
}

static void
tridentCalcViewport( GLcontext *ctx )
{
   tridentContextPtr tmesa = TRIDENT_CONTEXT(ctx);
   const GLfloat *v = ctx->Viewport._WindowMap.m;
   GLfloat *m = tmesa->hw_viewport;

   /* See also trident_translate_vertex.
    */
   m[MAT_SX] =   v[MAT_SX];
   m[MAT_TX] =   v[MAT_TX] + tmesa->drawX + SUBPIXEL_X;
   m[MAT_SY] = - v[MAT_SY];
   m[MAT_TY] = - v[MAT_TY] + tmesa->driDrawable->h + tmesa->drawY + SUBPIXEL_Y;
#if 0
   m[MAT_SZ] =   v[MAT_SZ] * tmesa->depth_scale;
   m[MAT_TZ] =   v[MAT_TZ] * tmesa->depth_scale;
#else
   m[MAT_SZ] =   v[MAT_SZ];
   m[MAT_TZ] =   v[MAT_TZ];
#endif

   tmesa->SetupNewInputs = ~0;
}

static void tridentDDViewport( GLcontext *ctx,
			  GLint x, GLint y,
			  GLsizei width, GLsizei height )
{
   tridentCalcViewport( ctx );
}

static void tridentDDDepthRange( GLcontext *ctx,
			    GLclampd nearval, GLclampd farval )
{
   tridentCalcViewport( ctx );
}

static void
tridentSetCliprects( tridentContextPtr tmesa, GLenum mode )
{
   __DRIdrawablePrivate *dPriv = tmesa->driDrawable;

   switch ( mode ) {
   case GL_FRONT_LEFT:
      if (dPriv->numClipRects == 0) {
	 static drm_clip_rect_t zeroareacliprect = {0,0,0,0};
	 tmesa->numClipRects = 1;
	 tmesa->pClipRects = &zeroareacliprect;
      } else {
	 tmesa->numClipRects = dPriv->numClipRects;
	 tmesa->pClipRects = (drm_clip_rect_t *)dPriv->pClipRects;
      }
      tmesa->drawX = dPriv->x;
      tmesa->drawY = dPriv->y;
      break;
   case GL_BACK_LEFT:
      if ( dPriv->numBackClipRects == 0 ) {
	  if (dPriv->numClipRects == 0) {
	     static drm_clip_rect_t zeroareacliprect = {0,0,0,0};
	     tmesa->numClipRects = 1;
	     tmesa->pClipRects = &zeroareacliprect;
	  } else {
	     tmesa->numClipRects = dPriv->numClipRects;
	     tmesa->pClipRects = (drm_clip_rect_t *)dPriv->pClipRects;
	     tmesa->drawX = dPriv->x;
	     tmesa->drawY = dPriv->y;
	  }
      }
      else {
	 tmesa->numClipRects = dPriv->numBackClipRects;
	 tmesa->pClipRects = (drm_clip_rect_t *)dPriv->pBackClipRects;
	 tmesa->drawX = dPriv->backX;
	 tmesa->drawY = dPriv->backY;
      }
      break;
   default:
      return;
   }

#if 0
   tmesa->dirty |= TRIDENT_UPLOAD_CLIPRECTS;
#endif
}

#if 0
static GLboolean tridentDDSetDrawBuffer( GLcontext *ctx, GLenum mode )
{
   tridentContextPtr tmesa = TRIDENT_CONTEXT(ctx);
   int found = GL_TRUE;

   if ( tmesa->DrawBuffer != mode ) {
      tmesa->DrawBuffer = mode;
      
      switch ( mode ) {
      case GL_FRONT_LEFT:
	 tridentFallback( tmesa, TRIDENT_FALLBACK_DRAW_BUFFER, GL_FALSE );
	 tmesa->drawOffset = tmesa->tridentScreen->frontOffset;
	 tmesa->drawPitch  = tmesa->tridentScreen->frontPitch;
	 tridentSetCliprects( tmesa, GL_FRONT_LEFT );
	 break;
      case GL_BACK_LEFT:
	 tridentFallback( tmesa, TRIDENT_FALLBACK_DRAW_BUFFER, GL_FALSE );
	 tmesa->drawOffset = tmesa->tridentScreen->backOffset;
	 tmesa->drawPitch  = tmesa->tridentScreen->backPitch;
	 tridentSetCliprects( tmesa, GL_BACK_LEFT );
	 break;
      default:
	 tridentFallback( tmesa, TRIDENT_FALLBACK_DRAW_BUFFER, GL_TRUE );
	 found = GL_FALSE;
	 break;
      }

#if 0
      tmesa->setup.dst_off_pitch = (((tmesa->drawPitch/8) << 22) |
				    (tmesa->drawOffset >> 3));

      tmesa->dirty |= MACH64_UPLOAD_DST_OFF_PITCH | MACH64_UPLOAD_CONTEXT;
#endif
      
   }

   return found;
}

static void tridentDDClearColor( GLcontext *ctx,
				const GLchan color[4] )
{
   tridentContextPtr tmesa = TRIDENT_CONTEXT(ctx);

   tmesa->ClearColor = tridentPackColor( tmesa->tridentScreen->cpp,
					color[0], color[1], 
					color[2], color[3] );
}
#endif

static void
tridentDDUpdateState( GLcontext *ctx, GLuint new_state )
{
   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _vbo_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   TRIDENT_CONTEXT(ctx)->new_gl_state |= new_state;
}


/* Initialize the context's hardware state.
 */
void tridentDDInitState( tridentContextPtr tmesa )
{
   tmesa->new_state = 0;

   switch ( tmesa->glCtx->Visual.depthBits ) {
   case 16:
      tmesa->depth_scale = 1.0 / (GLfloat)0xffff;
      break;
   case 24:
      tmesa->depth_scale = 1.0 / (GLfloat)0xffffff;
      break;
   }
}

void tridentDDUpdateHWState( GLcontext *ctx )
{
   tridentContextPtr tmesa = TRIDENT_CONTEXT(ctx);
   int new_state = tmesa->new_state;

   if ( new_state )
   {
      tmesa->new_state = 0;

#if 0
      /* Update the various parts of the context's state.
       */
      if ( new_state & GAMMA_NEW_ALPHA )
	 tridentUpdateAlphaMode( ctx );

      if ( new_state & GAMMA_NEW_DEPTH )
	 tridentUpdateZMode( ctx );

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

      if ( new_state & GAMMA_NEW_STIPPLE )
	 gammaUpdateStipple( ctx );
#endif
   }

   /* HACK ! */

#if 0
   gammaEmitHwState( tmesa );
#endif
}

/* Initialize the driver's state functions.
 */
void tridentDDInitStateFuncs( GLcontext *ctx )
{
   ctx->Driver.UpdateState		= tridentDDUpdateState;

   ctx->Driver.Clear			= tridentDDClear;
   ctx->Driver.DepthRange		= tridentDDDepthRange;
   ctx->Driver.ShadeModel		= tridentDDShadeModel;
   ctx->Driver.Viewport			= tridentDDViewport;
}
