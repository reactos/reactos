/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_blit.h"
#include "intel_regions.h"
#include "intel_batchbuffer.h"
#include "context.h"
#include "framebuffer.h"
#include "macros.h"
#include "swrast/swrast.h"

GLboolean intel_intersect_cliprects( drm_clip_rect_t *dst,
				     const drm_clip_rect_t *a,
				     const drm_clip_rect_t *b )
{
   dst->x1 = MAX2(a->x1, b->x1);
   dst->x2 = MIN2(a->x2, b->x2);
   dst->y1 = MAX2(a->y1, b->y1);
   dst->y2 = MIN2(a->y2, b->y2);

   return (dst->x1 <= dst->x2 &&
	   dst->y1 <= dst->y2);
}

struct intel_region *intel_drawbuf_region( struct intel_context *intel )
{
   switch (intel->ctx.DrawBuffer->_ColorDrawBufferMask[0]) {
   case BUFFER_BIT_FRONT_LEFT:
      return intel->front_region;
   case BUFFER_BIT_BACK_LEFT:
      return intel->back_region;
   default:
      /* Not necessary to fallback - could handle either NONE or
       * FRONT_AND_BACK cases below.
       */
      return NULL;		
   }
}

struct intel_region *intel_readbuf_region( struct intel_context *intel )
{
   GLcontext *ctx = &intel->ctx;

   /* This will have to change to support EXT_fbo's, but is correct
    * for now:
    */
   switch (ctx->ReadBuffer->_ColorReadBufferIndex) {
   case BUFFER_FRONT_LEFT:
      return intel->front_region;
   case BUFFER_BACK_LEFT:
      return intel->back_region;
   default:
      assert(0);
      return NULL;
   }
}



static void intelBufferSize(GLframebuffer *buffer,
			    GLuint *width, 
			    GLuint *height)
{
   GET_CURRENT_CONTEXT(ctx);
   struct intel_context *intel = intel_context(ctx);
   /* Need to lock to make sure the driDrawable is uptodate.  This
    * information is used to resize Mesa's software buffers, so it has
    * to be correct.
    */
   LOCK_HARDWARE(intel);
   if (intel->driDrawable) {
      *width = intel->driDrawable->w;
      *height = intel->driDrawable->h;
   }
   else {
      *width = 0;
      *height = 0;
   }
   UNLOCK_HARDWARE(intel);
}


static void intelSetFrontClipRects( struct intel_context *intel )
{
   __DRIdrawablePrivate *dPriv = intel->driDrawable;

   if (!dPriv) return;

   intel->numClipRects = dPriv->numClipRects;
   intel->pClipRects = dPriv->pClipRects;
   intel->drawX = dPriv->x;
   intel->drawY = dPriv->y;
}


static void intelSetBackClipRects( struct intel_context *intel )
{
   __DRIdrawablePrivate *dPriv = intel->driDrawable;

   if (!dPriv) return;

   if (intel->sarea->pf_enabled == 0 && dPriv->numBackClipRects == 0) {
      intel->numClipRects = dPriv->numClipRects;
      intel->pClipRects = dPriv->pClipRects;
      intel->drawX = dPriv->x;
      intel->drawY = dPriv->y;
   } else {
      intel->numClipRects = dPriv->numBackClipRects;
      intel->pClipRects = dPriv->pBackClipRects;
      intel->drawX = dPriv->backX;
      intel->drawY = dPriv->backY;
      
      if (dPriv->numBackClipRects == 1 &&
	  dPriv->x == dPriv->backX &&
	  dPriv->y == dPriv->backY) {
      
	 /* Repeat the calculation of the back cliprect dimensions here
	  * as early versions of dri.a in the Xserver are incorrect.  Try
	  * very hard not to restrict future versions of dri.a which
	  * might eg. allocate truly private back buffers.
	  */
	 int x1, y1;
	 int x2, y2;
	 
	 x1 = dPriv->x;
	 y1 = dPriv->y;      
	 x2 = dPriv->x + dPriv->w;
	 y2 = dPriv->y + dPriv->h;
	 
	 if (x1 < 0) x1 = 0;
	 if (y1 < 0) y1 = 0;
	 if (x2 > intel->intelScreen->width) x2 = intel->intelScreen->width;
	 if (y2 > intel->intelScreen->height) y2 = intel->intelScreen->height;

	 if (x1 == dPriv->pBackClipRects[0].x1 &&
	     y1 == dPriv->pBackClipRects[0].y1) {

	    dPriv->pBackClipRects[0].x2 = x2;
	    dPriv->pBackClipRects[0].y2 = y2;
	 }
      }
   }
}


void intelWindowMoved( struct intel_context *intel )
{
   __DRIdrawablePrivate *dPriv = intel->driDrawable;

   if (!intel->ctx.DrawBuffer) {
      intelSetFrontClipRects( intel );
   }
   else {
      switch (intel->ctx.DrawBuffer->_ColorDrawBufferMask[0]) {
      case BUFFER_BIT_FRONT_LEFT:
	 intelSetFrontClipRects( intel );
	 break;
      case BUFFER_BIT_BACK_LEFT:
	 intelSetBackClipRects( intel );
	 break;
      default:
	 /* glDrawBuffer(GL_NONE or GL_FRONT_AND_BACK): software fallback */
	 intelSetFrontClipRects( intel );
      }
   }

   _mesa_resize_framebuffer(&intel->ctx,
			    (GLframebuffer*)dPriv->driverPrivate,
			    dPriv->w, dPriv->h);

   /* Set state we know depends on drawable parameters:
    */
   {
      GLcontext *ctx = &intel->ctx;

      if (ctx->Driver.Scissor)
	 ctx->Driver.Scissor( ctx, ctx->Scissor.X, ctx->Scissor.Y,
			      ctx->Scissor.Width, ctx->Scissor.Height );
      
      if (ctx->Driver.DepthRange)
	 ctx->Driver.DepthRange( ctx, 
				 ctx->Viewport.Near,
				 ctx->Viewport.Far );

      intel->NewGLState |= _NEW_SCISSOR;
   }

   /* This works because the lock is always grabbed before emitting
    * commands and commands are always flushed prior to releasing
    * the lock.
    */
   intel->NewGLState |= _NEW_WINDOW_POS; 
}



/* A true meta version of this would be very simple and additionally
 * machine independent.  Maybe we'll get there one day.
 */
static void intelClearWithTris(struct intel_context *intel, 
			       GLbitfield mask)
{
   GLcontext *ctx = &intel->ctx;
   drm_clip_rect_t clear;
   GLint cx, cy, cw, ch;

   if (INTEL_DEBUG & DEBUG_DRI)
      _mesa_printf("%s %x\n", __FUNCTION__, mask);

   {

      intel->vtbl.install_meta_state(intel);

      /* Get clear bounds after locking */
      cx = ctx->DrawBuffer->_Xmin;
      cy = ctx->DrawBuffer->_Ymin;
      cw = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
      ch = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;

      clear.x1 = cx;
      clear.y1 = cy;
      clear.x2 = cx + cw;
      clear.y2 = cy + ch;

      /* Back and stencil cliprects are the same.  Try and do both
       * buffers at once:
       */
      if (mask & (BUFFER_BIT_BACK_LEFT|BUFFER_BIT_STENCIL|BUFFER_BIT_DEPTH)) { 
	 intel->vtbl.meta_draw_region(intel, 
				      intel->back_region,
				      intel->depth_region );

	 if (mask & BUFFER_BIT_BACK_LEFT)
	    intel->vtbl.meta_color_mask(intel, GL_TRUE );
	 else
	    intel->vtbl.meta_color_mask(intel, GL_FALSE );

	 if (mask & BUFFER_BIT_STENCIL) 
	    intel->vtbl.meta_stencil_replace( intel, 
					      intel->ctx.Stencil.WriteMask[0], 
					      intel->ctx.Stencil.Clear);
	 else
	    intel->vtbl.meta_no_stencil_write(intel);

	 if (mask & BUFFER_BIT_DEPTH) 
	    intel->vtbl.meta_depth_replace( intel );
	 else
	    intel->vtbl.meta_no_depth_write(intel);
      
	 /* XXX: Using INTEL_BATCH_NO_CLIPRECTS here is dangerous as the
	  * drawing origin may not be correctly emitted.
	  */
	 intel->vtbl.meta_draw_quad(intel, 
				    clear.x1, clear.x2, 
				    clear.y1, clear.y2, 
				    intel->ctx.Depth.Clear,
				    intel->clear_chan[0], 
				    intel->clear_chan[1], 
				    intel->clear_chan[2], 
				    intel->clear_chan[3], 
				    0, 0, 0, 0);
      }

      /* Front may have different cliprects: 
       */
      if (mask & BUFFER_BIT_FRONT_LEFT) {
	 intel->vtbl.meta_no_depth_write(intel);
	 intel->vtbl.meta_no_stencil_write(intel);
	 intel->vtbl.meta_color_mask(intel, GL_TRUE );
	 intel->vtbl.meta_draw_region(intel, 
				      intel->front_region,
				      intel->depth_region);

	 /* XXX: Using INTEL_BATCH_NO_CLIPRECTS here is dangerous as the
	  * drawing origin may not be correctly emitted.
	  */
	 intel->vtbl.meta_draw_quad(intel, 
				    clear.x1, clear.x2, 
				    clear.y1, clear.y2, 
				    0,
				    intel->clear_chan[0], 
				    intel->clear_chan[1], 
				    intel->clear_chan[2], 
				    intel->clear_chan[3], 
				    0, 0, 0, 0);
      }

      intel->vtbl.leave_meta_state( intel );
   }
}





static void intelClear(GLcontext *ctx, GLbitfield mask)
{
   struct intel_context *intel = intel_context( ctx );
   const GLuint colorMask = *((GLuint *) &ctx->Color.ColorMask);
   GLbitfield tri_mask = 0;
   GLbitfield blit_mask = 0;
   GLbitfield swrast_mask = 0;

   if (INTEL_DEBUG & DEBUG_DRI)
      fprintf(stderr, "%s %x\n", __FUNCTION__, mask);


   if (mask & BUFFER_BIT_FRONT_LEFT) {
      if (colorMask == ~0) {
	 blit_mask |= BUFFER_BIT_FRONT_LEFT;
      } 
      else {
	 tri_mask |= BUFFER_BIT_FRONT_LEFT;
      }
   }

   if (mask & BUFFER_BIT_BACK_LEFT) {
      if (colorMask == ~0) {
	 blit_mask |= BUFFER_BIT_BACK_LEFT;
      } 
      else {
	 tri_mask |= BUFFER_BIT_BACK_LEFT;
      }
   }


   if (mask & BUFFER_BIT_STENCIL) {
      if (!intel->hw_stencil) {
	 swrast_mask |= BUFFER_BIT_STENCIL;
      }
      else if ((ctx->Stencil.WriteMask[0] & 0xff) != 0xff ||
	       intel->depth_region->tiled) {
	 tri_mask |= BUFFER_BIT_STENCIL;
      } 
      else {
	 blit_mask |= BUFFER_BIT_STENCIL;
      }
   }

   /* Do depth with stencil if possible to avoid 2nd pass over the
    * same buffer.
    */
   if (mask & BUFFER_BIT_DEPTH) {
      if ((tri_mask & BUFFER_BIT_STENCIL) ||
	  intel->depth_region->tiled)
	 tri_mask |= BUFFER_BIT_DEPTH;
      else 
	 blit_mask |= BUFFER_BIT_DEPTH;
   }

   swrast_mask |= (mask & BUFFER_BIT_ACCUM);

   intelFlush( ctx );

   if (blit_mask)
      intelClearWithBlit( ctx, blit_mask );

   if (tri_mask) 
      intelClearWithTris( intel, tri_mask );

   if (swrast_mask)
      _swrast_Clear( ctx, swrast_mask );
}







/* Flip the front & back buffers
 */
static void intelPageFlip( const __DRIdrawablePrivate *dPriv )
{
#if 0
   struct intel_context *intel;
   int tmp, ret;

   if (INTEL_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   assert(dPriv);
   assert(dPriv->driContextPriv);
   assert(dPriv->driContextPriv->driverPrivate);

   intel = (struct intel_context *) dPriv->driContextPriv->driverPrivate;

   intelFlush( &intel->ctx );
   LOCK_HARDWARE( intel );

   if (dPriv->pClipRects) {
      *(drm_clip_rect_t *)intel->sarea->boxes = dPriv->pClipRects[0];
      intel->sarea->nbox = 1;
   }

   ret = drmCommandNone(intel->driFd, DRM_I830_FLIP); 
   if (ret) {
      fprintf(stderr, "%s: %d\n", __FUNCTION__, ret);
      UNLOCK_HARDWARE( intel );
      exit(1);
   }

   tmp = intel->sarea->last_enqueue;
   intelRefillBatchLocked( intel );
   UNLOCK_HARDWARE( intel );


   intelSetDrawBuffer( &intel->ctx, intel->ctx.Color.DriverDrawBuffer );
#endif
}


void intelSwapBuffers( __DRIdrawablePrivate *dPriv )
{
   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
      struct intel_context *intel;
      GLcontext *ctx;
      intel = (struct intel_context *) dPriv->driContextPriv->driverPrivate;
      ctx = &intel->ctx;
      if (ctx->Visual.doubleBufferMode) {
	 _mesa_notifySwapBuffers( ctx );  /* flush pending rendering comands */
	 if ( 0 /*intel->doPageFlip*/ ) { /* doPageFlip is never set !!! */
	    intelPageFlip( dPriv );
	 } else {
	    intelCopyBuffer( dPriv, NULL );
	 }
	 if (intel->aub_file) {
	    intelFlush(ctx);
	    intel->vtbl.aub_dump_bmp( intel, 1 );

	    intel->aub_wrap = 1;
	 }
      }
   } else {
      /* XXX this shouldn't be an error but we can't handle it for now */
      fprintf(stderr, "%s: drawable has no context!\n", __FUNCTION__);
   }
}

void intelCopySubBuffer( __DRIdrawablePrivate *dPriv,
			 int x, int y, int w, int h )
{
   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
      struct intel_context *intel = dPriv->driContextPriv->driverPrivate;
      GLcontext *ctx = &intel->ctx;

      if (ctx->Visual.doubleBufferMode) {
	 drm_clip_rect_t rect;
	 rect.x1 = x + dPriv->x;
	 rect.y1 = (dPriv->h - y - h) + dPriv->y;
	 rect.x2 = rect.x1 + w;
	 rect.y2 = rect.y1 + h;
	 _mesa_notifySwapBuffers( ctx );  /* flush pending rendering comands */
	 intelCopyBuffer( dPriv, &rect );
      }
   } else {
      /* XXX this shouldn't be an error but we can't handle it for now */
      fprintf(stderr, "%s: drawable has no context!\n", __FUNCTION__);
   }
}


static void intelDrawBuffer(GLcontext *ctx, GLenum mode )
{
   struct intel_context *intel = intel_context(ctx);
   int front = 0;
 
   if (!ctx->DrawBuffer)
      return;

   switch ( ctx->DrawBuffer->_ColorDrawBufferMask[0] ) {
   case BUFFER_BIT_FRONT_LEFT:
      front = 1;
      FALLBACK( intel, INTEL_FALLBACK_DRAW_BUFFER, GL_FALSE );
      break;
   case BUFFER_BIT_BACK_LEFT:
      front = 0;
      FALLBACK( intel, INTEL_FALLBACK_DRAW_BUFFER, GL_FALSE );
      break;
   default:
      FALLBACK( intel, INTEL_FALLBACK_DRAW_BUFFER, GL_TRUE );
      return;
   }

   if ( intel->sarea->pf_current_page == 1 ) 
      front ^= 1;
   
   intelSetFrontClipRects( intel );


   if (front) {
      if (intel->draw_region != intel->front_region) {
	 intel_region_release(intel, &intel->draw_region);
	 intel_region_reference(&intel->draw_region, intel->front_region);
      }
   } else {
      if (intel->draw_region != intel->back_region) {
	 intel_region_release(intel, &intel->draw_region);
	 intel_region_reference(&intel->draw_region, intel->back_region);
      }
   }

   intel->vtbl.set_draw_region( intel, 
				intel->draw_region,
				intel->depth_region);
}

static void intelReadBuffer( GLcontext *ctx, GLenum mode )
{
   /* nothing, until we implement h/w glRead/CopyPixels or CopyTexImage */
}



void intelInitBufferFuncs( struct dd_function_table *functions )
{
   functions->Clear = intelClear;
   functions->GetBufferSize = intelBufferSize;
   functions->DrawBuffer = intelDrawBuffer;
   functions->ReadBuffer = intelReadBuffer;
}
