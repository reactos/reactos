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
#include "intel_buffers.h"
#include "intel_depthstencil.h"
#include "intel_fbo.h"
#include "intel_tris.h"
#include "intel_regions.h"
#include "intel_batchbuffer.h"
#include "intel_reg.h"
#include "context.h"
#include "utils.h"
#include "drirenderbuffer.h"
#include "framebuffer.h"
#include "swrast/swrast.h"
#include "vblank.h"


/* This block can be removed when libdrm >= 2.3.1 is required */

#ifndef DRM_VBLANK_FLIP

#define DRM_VBLANK_FLIP 0x8000000

typedef struct drm_i915_flip {
   int pipes;
} drm_i915_flip_t;

#undef DRM_IOCTL_I915_FLIP
#define DRM_IOCTL_I915_FLIP DRM_IOW(DRM_COMMAND_BASE + DRM_I915_FLIP, \
				    drm_i915_flip_t)

#endif


/**
 * XXX move this into a new dri/common/cliprects.c file.
 */
GLboolean
intel_intersect_cliprects(drm_clip_rect_t * dst,
                          const drm_clip_rect_t * a,
                          const drm_clip_rect_t * b)
{
   GLint bx = b->x1;
   GLint by = b->y1;
   GLint bw = b->x2 - bx;
   GLint bh = b->y2 - by;

   if (bx < a->x1)
      bw -= a->x1 - bx, bx = a->x1;
   if (by < a->y1)
      bh -= a->y1 - by, by = a->y1;
   if (bx + bw > a->x2)
      bw = a->x2 - bx;
   if (by + bh > a->y2)
      bh = a->y2 - by;
   if (bw <= 0)
      return GL_FALSE;
   if (bh <= 0)
      return GL_FALSE;

   dst->x1 = bx;
   dst->y1 = by;
   dst->x2 = bx + bw;
   dst->y2 = by + bh;

   return GL_TRUE;
}

/**
 * Return pointer to current color drawing region, or NULL.
 */
struct intel_region *
intel_drawbuf_region(struct intel_context *intel)
{
   struct intel_renderbuffer *irbColor =
      intel_renderbuffer(intel->ctx.DrawBuffer->_ColorDrawBuffers[0][0]);
   if (irbColor)
      return irbColor->region;
   else
      return NULL;
}

/**
 * Return pointer to current color reading region, or NULL.
 */
struct intel_region *
intel_readbuf_region(struct intel_context *intel)
{
   struct intel_renderbuffer *irb
      = intel_renderbuffer(intel->ctx.ReadBuffer->_ColorReadBuffer);
   if (irb)
      return irb->region;
   else
      return NULL;
}



/**
 * Update the following fields for rendering to a user-created FBO:
 *   intel->numClipRects
 *   intel->pClipRects
 *   intel->drawX
 *   intel->drawY
 */
static void
intelSetRenderbufferClipRects(struct intel_context *intel)
{
   assert(intel->ctx.DrawBuffer->Width > 0);
   assert(intel->ctx.DrawBuffer->Height > 0);
   intel->fboRect.x1 = 0;
   intel->fboRect.y1 = 0;
   intel->fboRect.x2 = intel->ctx.DrawBuffer->Width;
   intel->fboRect.y2 = intel->ctx.DrawBuffer->Height;
   intel->numClipRects = 1;
   intel->pClipRects = &intel->fboRect;
   intel->drawX = 0;
   intel->drawY = 0;
}


/**
 * As above, but for rendering to front buffer of a window.
 * \sa intelSetRenderbufferClipRects
 */
static void
intelSetFrontClipRects(struct intel_context *intel)
{
   __DRIdrawablePrivate *dPriv = intel->driDrawable;

   if (!dPriv)
      return;

   intel->numClipRects = dPriv->numClipRects;
   intel->pClipRects = dPriv->pClipRects;
   intel->drawX = dPriv->x;
   intel->drawY = dPriv->y;
}


/**
 * As above, but for rendering to back buffer of a window.
 */
static void
intelSetBackClipRects(struct intel_context *intel)
{
   __DRIdrawablePrivate *dPriv = intel->driDrawable;
   struct intel_framebuffer *intel_fb;

   if (!dPriv)
      return;

   intel_fb = dPriv->driverPrivate;

   if (intel_fb->pf_active || dPriv->numBackClipRects == 0) {
      /* use the front clip rects */
      intel->numClipRects = dPriv->numClipRects;
      intel->pClipRects = dPriv->pClipRects;
      intel->drawX = dPriv->x;
      intel->drawY = dPriv->y;
   }
   else {
      /* use the back clip rects */
      intel->numClipRects = dPriv->numBackClipRects;
      intel->pClipRects = dPriv->pBackClipRects;
      intel->drawX = dPriv->backX;
      intel->drawY = dPriv->backY;
   }
}


/**
 * This will be called whenever the currently bound window is moved/resized.
 * XXX: actually, it seems to NOT be called when the window is only moved (BP).
 */
void
intelWindowMoved(struct intel_context *intel)
{
   GLcontext *ctx = &intel->ctx;
   __DRIdrawablePrivate *dPriv = intel->driDrawable;
   struct intel_framebuffer *intel_fb = dPriv->driverPrivate;

   if (!intel->ctx.DrawBuffer) {
      /* when would this happen? -BP */
      intelSetFrontClipRects(intel);
   }
   else if (intel->ctx.DrawBuffer->Name != 0) {
      /* drawing to user-created FBO - do nothing */
      /* Cliprects would be set from intelDrawBuffer() */
   }
   else {
      /* drawing to a window */
      switch (intel_fb->Base._ColorDrawBufferMask[0]) {
      case BUFFER_BIT_FRONT_LEFT:
         intelSetFrontClipRects(intel);
         break;
      case BUFFER_BIT_BACK_LEFT:
         intelSetBackClipRects(intel);
         break;
      default:
         /* glDrawBuffer(GL_NONE or GL_FRONT_AND_BACK): software fallback */
         intelSetFrontClipRects(intel);
      }
   }

   if (intel->intelScreen->driScrnPriv->ddxMinor >= 7) {
      drmI830Sarea *sarea = intel->sarea;
      drm_clip_rect_t drw_rect = { .x1 = dPriv->x, .x2 = dPriv->x + dPriv->w,
				   .y1 = dPriv->y, .y2 = dPriv->y + dPriv->h };
      drm_clip_rect_t pipeA_rect = { .x1 = sarea->pipeA_x, .y1 = sarea->pipeA_y,
				     .x2 = sarea->pipeA_x + sarea->pipeA_w,
				     .y2 = sarea->pipeA_y + sarea->pipeA_h };
      drm_clip_rect_t pipeB_rect = { .x1 = sarea->pipeB_x, .y1 = sarea->pipeB_y,
				     .x2 = sarea->pipeB_x + sarea->pipeB_w,
				     .y2 = sarea->pipeB_y + sarea->pipeB_h };
      GLint areaA = driIntersectArea( drw_rect, pipeA_rect );
      GLint areaB = driIntersectArea( drw_rect, pipeB_rect );
      GLuint flags = intel_fb->vblank_flags;
      GLboolean pf_active;
      GLint pf_pipes;

      /* Update page flipping info
       */
      pf_pipes = 0;

      if (areaA > 0)
	 pf_pipes |= 1;

      if (areaB > 0)
	 pf_pipes |= 2;

      intel_fb->pf_current_page = (intel->sarea->pf_current_page >>
				   (intel_fb->pf_pipes & 0x2)) & 0x3;

      intel_fb->pf_num_pages = intel->intelScreen->third.handle ? 3 : 2;

      pf_active = pf_pipes && (pf_pipes & intel->sarea->pf_active) == pf_pipes;

      if (INTEL_DEBUG & DEBUG_LOCK)
	 if (pf_active != intel_fb->pf_active)
	    _mesa_printf("%s - Page flipping %sactive\n", __progname,
			 pf_active ? "" : "in");

      if (pf_active) {
	 /* Sync pages between pipes if we're flipping on both at the same time */
	 if (pf_pipes == 0x3 &&	pf_pipes != intel_fb->pf_pipes &&
	     (intel->sarea->pf_current_page & 0x3) !=
	     (((intel->sarea->pf_current_page) >> 2) & 0x3)) {
	    drm_i915_flip_t flip;

	    if (intel_fb->pf_current_page ==
		(intel->sarea->pf_current_page & 0x3)) {
	       /* XXX: This is ugly, but emitting two flips 'in a row' can cause
		* lockups for unknown reasons.
		*/
               intel->sarea->pf_current_page =
		  intel->sarea->pf_current_page & 0x3;
	       intel->sarea->pf_current_page |=
		  ((intel_fb->pf_current_page + intel_fb->pf_num_pages - 1) %
		   intel_fb->pf_num_pages) << 2;

	       flip.pipes = 0x2;
	    } else {
               intel->sarea->pf_current_page =
		  intel->sarea->pf_current_page & (0x3 << 2);
	       intel->sarea->pf_current_page |=
		  (intel_fb->pf_current_page + intel_fb->pf_num_pages - 1) %
		  intel_fb->pf_num_pages;

	       flip.pipes = 0x1;
	    }

	    drmCommandWrite(intel->driFd, DRM_I915_FLIP, &flip, sizeof(flip));
	 }

	 intel_fb->pf_pipes = pf_pipes;
      }

      intel_fb->pf_active = pf_active;
      intel_flip_renderbuffers(intel_fb);
      intel_draw_buffer(&intel->ctx, intel->ctx.DrawBuffer);

      /* Update vblank info
       */
      if (areaB > areaA || (areaA == areaB && areaB > 0)) {
	 flags = intel_fb->vblank_flags | VBLANK_FLAG_SECONDARY;
      } else {
	 flags = intel_fb->vblank_flags & ~VBLANK_FLAG_SECONDARY;
      }

      if (flags != intel_fb->vblank_flags && intel_fb->vblank_flags &&
	  !(intel_fb->vblank_flags & VBLANK_FLAG_NO_IRQ)) {
	 drmVBlank vbl;
	 int i;

	 vbl.request.type = DRM_VBLANK_ABSOLUTE;

	 if ( intel_fb->vblank_flags & VBLANK_FLAG_SECONDARY ) {
	    vbl.request.type |= DRM_VBLANK_SECONDARY;
	 }

	 for (i = 0; i < intel_fb->pf_num_pages; i++) {
	    if (!intel_fb->color_rb[i] ||
		(intel_fb->vbl_waited - intel_fb->color_rb[i]->vbl_pending) <=
		(1<<23))
	       continue;

	    vbl.request.sequence = intel_fb->color_rb[i]->vbl_pending;
	    drmWaitVBlank(intel->driFd, &vbl);
	 }

	 intel_fb->vblank_flags = flags;
	 driGetCurrentVBlank(dPriv, intel_fb->vblank_flags, &intel_fb->vbl_seq);
	 intel_fb->vbl_waited = intel_fb->vbl_seq;

	 for (i = 0; i < intel_fb->pf_num_pages; i++) {
	    if (intel_fb->color_rb[i])
	       intel_fb->color_rb[i]->vbl_pending = intel_fb->vbl_waited;
	 }
      }
   } else {
      intel_fb->vblank_flags &= ~VBLANK_FLAG_SECONDARY;
   }

   /* Update Mesa's notion of window size */
   driUpdateFramebufferSize(ctx, dPriv);
   intel_fb->Base.Initialized = GL_TRUE; /* XXX remove someday */

   /* Update hardware scissor */
   ctx->Driver.Scissor(ctx, ctx->Scissor.X, ctx->Scissor.Y,
                       ctx->Scissor.Width, ctx->Scissor.Height);

   /* Re-calculate viewport related state */
   ctx->Driver.DepthRange( ctx, ctx->Viewport.Near, ctx->Viewport.Far );
}



/* A true meta version of this would be very simple and additionally
 * machine independent.  Maybe we'll get there one day.
 */
static void
intelClearWithTris(struct intel_context *intel, GLbitfield mask)
{
   GLcontext *ctx = &intel->ctx;
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   drm_clip_rect_t clear;

   if (INTEL_DEBUG & DEBUG_BLIT)
      _mesa_printf("%s 0x%x\n", __FUNCTION__, mask);

   LOCK_HARDWARE(intel);

   /* XXX FBO: was: intel->driDrawable->numClipRects */
   if (intel->numClipRects) {
      GLint cx, cy, cw, ch;
      GLuint buf;

      intel->vtbl.install_meta_state(intel);

      /* Get clear bounds after locking */
      cx = fb->_Xmin;
      cy = fb->_Ymin;
      ch = fb->_Ymax - cx;
      cw = fb->_Xmax - cy;

      /* note: regardless of 'all', cx, cy, cw, ch are now correct */
      clear.x1 = cx;
      clear.y1 = cy;
      clear.x2 = cx + cw;
      clear.y2 = cy + ch;

      /* Back and stencil cliprects are the same.  Try and do both
       * buffers at once:
       */
      if (mask &
          (BUFFER_BIT_BACK_LEFT | BUFFER_BIT_STENCIL | BUFFER_BIT_DEPTH)) {
         struct intel_region *backRegion =
            intel_get_rb_region(fb, BUFFER_BACK_LEFT);
         struct intel_region *depthRegion =
            intel_get_rb_region(fb, BUFFER_DEPTH);
         const GLuint clearColor = (backRegion && backRegion->cpp == 4)
            ? intel->ClearColor8888 : intel->ClearColor565;

         intel->vtbl.meta_draw_region(intel, backRegion, depthRegion);

         if (mask & BUFFER_BIT_BACK_LEFT)
            intel->vtbl.meta_color_mask(intel, GL_TRUE);
         else
            intel->vtbl.meta_color_mask(intel, GL_FALSE);

         if (mask & BUFFER_BIT_STENCIL)
            intel->vtbl.meta_stencil_replace(intel,
                                             intel->ctx.Stencil.WriteMask[0],
                                             intel->ctx.Stencil.Clear);
         else
            intel->vtbl.meta_no_stencil_write(intel);

         if (mask & BUFFER_BIT_DEPTH)
            intel->vtbl.meta_depth_replace(intel);
         else
            intel->vtbl.meta_no_depth_write(intel);

         /* XXX: Using INTEL_BATCH_NO_CLIPRECTS here is dangerous as the
          * drawing origin may not be correctly emitted.
          */
         intel_meta_draw_quad(intel, clear.x1, clear.x2, clear.y1, clear.y2, intel->ctx.Depth.Clear, clearColor, 0, 0, 0, 0);   /* texcoords */

         mask &=
            ~(BUFFER_BIT_BACK_LEFT | BUFFER_BIT_STENCIL | BUFFER_BIT_DEPTH);
      }

      /* clear the remaining (color) renderbuffers */
      for (buf = 0; buf < BUFFER_COUNT && mask; buf++) {
         const GLuint bufBit = 1 << buf;
         if (mask & bufBit) {
            struct intel_renderbuffer *irbColor =
               intel_renderbuffer(fb->Attachment[buf].Renderbuffer);
            GLuint color = (irbColor->region->cpp == 4)
               ? intel->ClearColor8888 : intel->ClearColor565;

            ASSERT(irbColor);

            intel->vtbl.meta_no_depth_write(intel);
            intel->vtbl.meta_no_stencil_write(intel);
            intel->vtbl.meta_color_mask(intel, GL_TRUE);
            intel->vtbl.meta_draw_region(intel, irbColor->region, NULL);

            /* XXX: Using INTEL_BATCH_NO_CLIPRECTS here is dangerous as the
             * drawing origin may not be correctly emitted.
             */
            intel_meta_draw_quad(intel, clear.x1, clear.x2, clear.y1, clear.y2, 0,      /* depth clear val */
                                 color, 0, 0, 0, 0);    /* texcoords */

            mask &= ~bufBit;
         }
      }

      intel->vtbl.leave_meta_state(intel);
      intel_batchbuffer_flush(intel->batch);
   }
   UNLOCK_HARDWARE(intel);
}




/**
 * Copy the window contents named by dPriv to the rotated (or reflected)
 * color buffer.
 * srcBuf is BUFFER_BIT_FRONT_LEFT or BUFFER_BIT_BACK_LEFT to indicate the source.
 */
void
intelRotateWindow(struct intel_context *intel,
                  __DRIdrawablePrivate * dPriv, GLuint srcBuf)
{
   intelScreenPrivate *screen = intel->intelScreen;
   drm_clip_rect_t fullRect;
   struct intel_framebuffer *intel_fb;
   struct intel_region *src;
   const drm_clip_rect_t *clipRects;
   int numClipRects;
   int i;
   GLenum format, type;

   int xOrig, yOrig;
   int origNumClipRects;
   drm_clip_rect_t *origRects;

   /*
    * set up hardware state
    */
   intelFlush(&intel->ctx);

   LOCK_HARDWARE(intel);

   if (!intel->numClipRects) {
      UNLOCK_HARDWARE(intel);
      return;
   }

   intel->vtbl.install_meta_state(intel);

   intel->vtbl.meta_no_depth_write(intel);
   intel->vtbl.meta_no_stencil_write(intel);
   intel->vtbl.meta_color_mask(intel, GL_FALSE);


   /* save current drawing origin and cliprects (restored at end) */
   xOrig = intel->drawX;
   yOrig = intel->drawY;
   origNumClipRects = intel->numClipRects;
   origRects = intel->pClipRects;

   /*
    * set drawing origin, cliprects for full-screen access to rotated screen
    */
   fullRect.x1 = 0;
   fullRect.y1 = 0;
   fullRect.x2 = screen->rotatedWidth;
   fullRect.y2 = screen->rotatedHeight;
   intel->drawX = 0;
   intel->drawY = 0;
   intel->numClipRects = 1;
   intel->pClipRects = &fullRect;

   intel->vtbl.meta_draw_region(intel, screen->rotated_region, NULL);    /* ? */

   intel_fb = dPriv->driverPrivate;

   if ((srcBuf == BUFFER_BIT_BACK_LEFT && !intel_fb->pf_active)) {
      src = intel_get_rb_region(&intel_fb->Base, BUFFER_BACK_LEFT);
      clipRects = dPriv->pBackClipRects;
      numClipRects = dPriv->numBackClipRects;
   }
   else {
      src = intel_get_rb_region(&intel_fb->Base, BUFFER_FRONT_LEFT);
      clipRects = dPriv->pClipRects;
      numClipRects = dPriv->numClipRects;
   }

   if (src->cpp == 4) {
      format = GL_BGRA;
      type = GL_UNSIGNED_BYTE;
   }
   else {
      format = GL_BGR;
      type = GL_UNSIGNED_SHORT_5_6_5_REV;
   }

   /* set the whole screen up as a texture to avoid alignment issues */
   intel->vtbl.meta_tex_rect_source(intel,
                                    src->buffer,
                                    screen->width,
                                    screen->height, src->pitch, format, type);

   intel->vtbl.meta_texture_blend_replace(intel);

   /*
    * loop over the source window's cliprects
    */
   for (i = 0; i < numClipRects; i++) {
      int srcX0 = clipRects[i].x1;
      int srcY0 = clipRects[i].y1;
      int srcX1 = clipRects[i].x2;
      int srcY1 = clipRects[i].y2;
      GLfloat verts[4][2], tex[4][2];
      int j;

      /* build vertices for four corners of clip rect */
      verts[0][0] = srcX0;
      verts[0][1] = srcY0;
      verts[1][0] = srcX1;
      verts[1][1] = srcY0;
      verts[2][0] = srcX1;
      verts[2][1] = srcY1;
      verts[3][0] = srcX0;
      verts[3][1] = srcY1;

      /* .. and texcoords */
      tex[0][0] = srcX0;
      tex[0][1] = srcY0;
      tex[1][0] = srcX1;
      tex[1][1] = srcY0;
      tex[2][0] = srcX1;
      tex[2][1] = srcY1;
      tex[3][0] = srcX0;
      tex[3][1] = srcY1;

      /* transform coords to rotated screen coords */

      for (j = 0; j < 4; j++) {
         matrix23TransformCoordf(&screen->rotMatrix,
                                 &verts[j][0], &verts[j][1]);
      }

      /* draw polygon to map source image to dest region */
      intel_meta_draw_poly(intel, 4, verts, 0, 0, tex);

   }                            /* cliprect loop */

   intel->vtbl.leave_meta_state(intel);
   intel_batchbuffer_flush(intel->batch);

   /* restore original drawing origin and cliprects */
   intel->drawX = xOrig;
   intel->drawY = yOrig;
   intel->numClipRects = origNumClipRects;
   intel->pClipRects = origRects;

   UNLOCK_HARDWARE(intel);
}


/**
 * Called by ctx->Driver.Clear.
 */
static void
intelClear(GLcontext *ctx, GLbitfield mask)
{
   struct intel_context *intel = intel_context(ctx);
   const GLuint colorMask = *((GLuint *) & ctx->Color.ColorMask);
   GLbitfield tri_mask = 0;
   GLbitfield blit_mask = 0;
   GLbitfield swrast_mask = 0;
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   GLuint i;

   if (0)
      fprintf(stderr, "%s\n", __FUNCTION__);

   /* HW color buffers (front, back, aux, generic FBO, etc) */
   if (colorMask == ~0) {
      /* clear all R,G,B,A */
      /* XXX FBO: need to check if colorbuffers are software RBOs! */
      blit_mask |= (mask & BUFFER_BITS_COLOR);
   }
   else {
      /* glColorMask in effect */
      tri_mask |= (mask & BUFFER_BITS_COLOR);
   }

   /* HW stencil */
   if (mask & BUFFER_BIT_STENCIL) {
      const struct intel_region *stencilRegion
         = intel_get_rb_region(fb, BUFFER_STENCIL);
      if (stencilRegion) {
         /* have hw stencil */
         if ((ctx->Stencil.WriteMask[0] & 0xff) != 0xff) {
            /* not clearing all stencil bits, so use triangle clearing */
            tri_mask |= BUFFER_BIT_STENCIL;
         }
         else {
            /* clearing all stencil bits, use blitting */
            blit_mask |= BUFFER_BIT_STENCIL;
         }
      }
   }

   /* HW depth */
   if (mask & BUFFER_BIT_DEPTH) {
      /* clear depth with whatever method is used for stencil (see above) */
      if (tri_mask & BUFFER_BIT_STENCIL)
         tri_mask |= BUFFER_BIT_DEPTH;
      else
         blit_mask |= BUFFER_BIT_DEPTH;
   }

   /* SW fallback clearing */
   swrast_mask = mask & ~tri_mask & ~blit_mask;

   for (i = 0; i < BUFFER_COUNT; i++) {
      GLuint bufBit = 1 << i;
      if ((blit_mask | tri_mask) & bufBit) {
         if (!fb->Attachment[i].Renderbuffer->ClassID) {
            blit_mask &= ~bufBit;
            tri_mask &= ~bufBit;
            swrast_mask |= bufBit;
         }
      }
   }


   intelFlush(ctx);             /* XXX intelClearWithBlit also does this */

   if (blit_mask)
      intelClearWithBlit(ctx, blit_mask);

   if (tri_mask)
      intelClearWithTris(intel, tri_mask);

   if (swrast_mask)
      _swrast_Clear(ctx, swrast_mask);
}


/* Emit wait for pending flips */
void
intel_wait_flips(struct intel_context *intel, GLuint batch_flags)
{
   struct intel_framebuffer *intel_fb =
      (struct intel_framebuffer *) intel->ctx.DrawBuffer;
   struct intel_renderbuffer *intel_rb =
      intel_get_renderbuffer(&intel_fb->Base,
			     intel_fb->Base._ColorDrawBufferMask[0] ==
			     BUFFER_BIT_FRONT_LEFT ? BUFFER_FRONT_LEFT :
			     BUFFER_BACK_LEFT);

   if (intel_fb->Base.Name == 0 && intel_rb->pf_pending == intel_fb->pf_seq) {
      GLint pf_pipes = intel_fb->pf_pipes;
      BATCH_LOCALS;

      /* Wait for pending flips to take effect */
      BEGIN_BATCH(2, batch_flags);
      OUT_BATCH(pf_pipes & 0x1 ? (MI_WAIT_FOR_EVENT | MI_WAIT_FOR_PLANE_A_FLIP)
		: 0);
      OUT_BATCH(pf_pipes & 0x2 ? (MI_WAIT_FOR_EVENT | MI_WAIT_FOR_PLANE_B_FLIP)
		: 0);
      ADVANCE_BATCH();

      intel_rb->pf_pending--;
   }
}


/* Flip the front & back buffers
 */
static GLboolean
intelPageFlip(const __DRIdrawablePrivate * dPriv)
{
   struct intel_context *intel;
   int ret;
   struct intel_framebuffer *intel_fb = dPriv->driverPrivate;

   if (INTEL_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   assert(dPriv);
   assert(dPriv->driContextPriv);
   assert(dPriv->driContextPriv->driverPrivate);

   intel = (struct intel_context *) dPriv->driContextPriv->driverPrivate;

   if (intel->intelScreen->drmMinor < 9)
      return GL_FALSE;

   intelFlush(&intel->ctx);

   ret = 0;

   LOCK_HARDWARE(intel);

   if (dPriv->numClipRects && intel_fb->pf_active) {
      drm_i915_flip_t flip;

      flip.pipes = intel_fb->pf_pipes;

      ret = drmCommandWrite(intel->driFd, DRM_I915_FLIP, &flip, sizeof(flip));
   }

   UNLOCK_HARDWARE(intel);

   if (ret || !intel_fb->pf_active)
      return GL_FALSE;

   if (!dPriv->numClipRects) {
      usleep(10000);	/* throttle invisible client 10ms */
   }

   intel_fb->pf_current_page = (intel->sarea->pf_current_page >>
				(intel_fb->pf_pipes & 0x2)) & 0x3;

   if (dPriv->numClipRects != 0) {
      intel_get_renderbuffer(&intel_fb->Base, BUFFER_FRONT_LEFT)->pf_pending =
      intel_get_renderbuffer(&intel_fb->Base, BUFFER_BACK_LEFT)->pf_pending =
	 ++intel_fb->pf_seq;
   }

   intel_flip_renderbuffers(intel_fb);
   intel_draw_buffer(&intel->ctx, &intel_fb->Base);

   return GL_TRUE;
}

#if 0
void
intelSwapBuffers(__DRIdrawablePrivate * dPriv)
{
   if (dPriv->driverPrivate) {
      const struct gl_framebuffer *fb
         = (struct gl_framebuffer *) dPriv->driverPrivate;
      if (fb->Visual.doubleBufferMode) {
         GET_CURRENT_CONTEXT(ctx);
         if (ctx && ctx->DrawBuffer == fb) {
            _mesa_notifySwapBuffers(ctx);       /* flush pending rendering */
         }
         if (intel->doPageFlip) {
            intelPageFlip(dPriv);
         }
         else {
            intelCopyBuffer(dPriv);
         }
      }
   }
   else {
      _mesa_problem(NULL,
                    "dPriv has no gl_framebuffer pointer in intelSwapBuffers");
   }
}
#else
/* Trunk version:
 */

static GLboolean
intelScheduleSwap(const __DRIdrawablePrivate * dPriv, GLboolean *missed_target)
{
   struct intel_framebuffer *intel_fb = dPriv->driverPrivate;
   unsigned int interval = driGetVBlankInterval(dPriv, intel_fb->vblank_flags);
   struct intel_context *intel =
      intelScreenContext(dPriv->driScreenPriv->private);
   const intelScreenPrivate *intelScreen = intel->intelScreen;
   unsigned int target;
   drm_i915_vblank_swap_t swap;
   GLboolean ret;

   if (!intel_fb->vblank_flags ||
       (intel_fb->vblank_flags & VBLANK_FLAG_NO_IRQ) ||
       intelScreen->current_rotation != 0 ||
       intelScreen->drmMinor < (intel_fb->pf_active ? 9 : 6))
      return GL_FALSE;

   swap.seqtype = DRM_VBLANK_ABSOLUTE;

   if (intel_fb->vblank_flags & VBLANK_FLAG_SYNC) {
      swap.seqtype |= DRM_VBLANK_NEXTONMISS;
   } else if (interval == 0) {
      return GL_FALSE;
   }

   swap.drawable = dPriv->hHWDrawable;
   target = swap.sequence = intel_fb->vbl_seq + interval;

   if ( intel_fb->vblank_flags & VBLANK_FLAG_SECONDARY ) {
      swap.seqtype |= DRM_VBLANK_SECONDARY;
   }

   LOCK_HARDWARE(intel);

   intel_batchbuffer_flush(intel->batch);

   if ( intel_fb->pf_active ) {
      swap.seqtype |= DRM_VBLANK_FLIP;

      intel_fb->pf_current_page = (((intel->sarea->pf_current_page >>
				     (intel_fb->pf_pipes & 0x2)) & 0x3) + 1) %
				  intel_fb->pf_num_pages;
   }

   if (!drmCommandWriteRead(intel->driFd, DRM_I915_VBLANK_SWAP, &swap,
			    sizeof(swap))) {
      intel_fb->vbl_seq = swap.sequence;
      swap.sequence -= target;
      *missed_target = swap.sequence > 0 && swap.sequence <= (1 << 23);

      intel_get_renderbuffer(&intel_fb->Base, BUFFER_BACK_LEFT)->vbl_pending =
	 intel_get_renderbuffer(&intel_fb->Base,
				BUFFER_FRONT_LEFT)->vbl_pending =
	 intel_fb->vbl_seq;

      if (swap.seqtype & DRM_VBLANK_FLIP) {
	 intel_flip_renderbuffers(intel_fb);
	 intel_draw_buffer(&intel->ctx, intel->ctx.DrawBuffer);
      }

      ret = GL_TRUE;
   } else {
      if (swap.seqtype & DRM_VBLANK_FLIP) {
	 intel_fb->pf_current_page = ((intel->sarea->pf_current_page >>
					(intel_fb->pf_pipes & 0x2)) & 0x3) %
				     intel_fb->pf_num_pages;
      }

      ret = GL_FALSE;
   }

   UNLOCK_HARDWARE(intel);

   return ret;
}
  
void
intelSwapBuffers(__DRIdrawablePrivate * dPriv)
{
   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
      GET_CURRENT_CONTEXT(ctx);
      struct intel_context *intel;

      if (ctx == NULL)
	 return;

      intel = intel_context(ctx);

      if (ctx->Visual.doubleBufferMode) {
         intelScreenPrivate *screen = intel->intelScreen;
	 GLboolean missed_target;
	 struct intel_framebuffer *intel_fb = dPriv->driverPrivate;
	 int64_t ust;
         
	 _mesa_notifySwapBuffers(ctx);  /* flush pending rendering comands */

         if (screen->current_rotation != 0 ||
	     !intelScheduleSwap(dPriv, &missed_target)) {
	    driWaitForVBlank(dPriv, &intel_fb->vbl_seq, intel_fb->vblank_flags,
			     &missed_target);

	    if (screen->current_rotation != 0 || !intelPageFlip(dPriv)) {
	       intelCopyBuffer(dPriv, NULL);
	    }

	    if (screen->current_rotation != 0) {
	       intelRotateWindow(intel, dPriv, BUFFER_BIT_FRONT_LEFT);
	    }
	 }

	 intel_fb->swap_count++;
	 (*dri_interface->getUST) (&ust);
	 if (missed_target) {
	    intel_fb->swap_missed_count++;
	    intel_fb->swap_missed_ust = ust - intel_fb->swap_ust;
	 }

	 intel_fb->swap_ust = ust;
      }
   }
   else {
      /* XXX this shouldn't be an error but we can't handle it for now */
      fprintf(stderr, "%s: drawable has no context!\n", __FUNCTION__);
   }
}
#endif

void
intelCopySubBuffer(__DRIdrawablePrivate * dPriv, int x, int y, int w, int h)
{
   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
      struct intel_context *intel =
         (struct intel_context *) dPriv->driContextPriv->driverPrivate;
      GLcontext *ctx = &intel->ctx;

      if (ctx->Visual.doubleBufferMode) {
         drm_clip_rect_t rect;
         rect.x1 = x + dPriv->x;
         rect.y1 = (dPriv->h - y - h) + dPriv->y;
         rect.x2 = rect.x1 + w;
         rect.y2 = rect.y1 + h;
         _mesa_notifySwapBuffers(ctx);  /* flush pending rendering comands */
         intelCopyBuffer(dPriv, &rect);
      }
   }
   else {
      /* XXX this shouldn't be an error but we can't handle it for now */
      fprintf(stderr, "%s: drawable has no context!\n", __FUNCTION__);
   }
}


/**
 * Update the hardware state for drawing into a window or framebuffer object.
 *
 * Called by glDrawBuffer, glBindFramebufferEXT, MakeCurrent, and other
 * places within the driver.
 *
 * Basically, this needs to be called any time the current framebuffer
 * changes, the renderbuffers change, or we need to draw into different
 * color buffers.
 */
void
intel_draw_buffer(GLcontext * ctx, struct gl_framebuffer *fb)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_region *colorRegion, *depthRegion = NULL;
   struct intel_renderbuffer *irbDepth = NULL, *irbStencil = NULL;
   int front = 0;               /* drawing to front color buffer? */

   if (!fb) {
      /* this can happen during the initial context initialization */
      return;
   }

   /* Do this here, note core Mesa, since this function is called from
    * many places within the driver.
    */
   if (ctx->NewState & (_NEW_BUFFERS | _NEW_COLOR | _NEW_PIXEL)) {
      /* this updates the DrawBuffer->_NumColorDrawBuffers fields, etc */
      _mesa_update_framebuffer(ctx);
      /* this updates the DrawBuffer's Width/Height if it's a FBO */
      _mesa_update_draw_buffer_bounds(ctx);
   }

   if (fb->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      /* this may occur when we're called by glBindFrameBuffer() during
       * the process of someone setting up renderbuffers, etc.
       */
      /*_mesa_debug(ctx, "DrawBuffer: incomplete user FBO\n");*/
      return;
   }

   if (fb->Name)
      intel_validate_paired_depth_stencil(ctx, fb);

   /*
    * How many color buffers are we drawing into?
    */
   if (fb->_NumColorDrawBuffers[0] != 1
#if 0
       /* XXX FBO temporary - always use software rendering */
       || 1
#endif
      ) {
      /* writing to 0 or 2 or 4 color buffers */
      /*_mesa_debug(ctx, "Software rendering\n");*/
      FALLBACK(intel, INTEL_FALLBACK_DRAW_BUFFER, GL_TRUE);
      front = 1;                /* might not have back color buffer */
   }
   else {
      /* draw to exactly one color buffer */
      /*_mesa_debug(ctx, "Hardware rendering\n");*/
      FALLBACK(intel, INTEL_FALLBACK_DRAW_BUFFER, GL_FALSE);
      if (fb->_ColorDrawBufferMask[0] == BUFFER_BIT_FRONT_LEFT) {
         front = 1;
      }
   }

   /*
    * Get the intel_renderbuffer for the colorbuffer we're drawing into.
    * And set up cliprects.
    */
   if (fb->Name == 0) {
      /* drawing to window system buffer */
      if (front) {
         intelSetFrontClipRects(intel);
         colorRegion = intel_get_rb_region(fb, BUFFER_FRONT_LEFT);
      }
      else {
         intelSetBackClipRects(intel);
         colorRegion = intel_get_rb_region(fb, BUFFER_BACK_LEFT);
      }
   }
   else {
      /* drawing to user-created FBO */
      struct intel_renderbuffer *irb;
      intelSetRenderbufferClipRects(intel);
      irb = intel_renderbuffer(fb->_ColorDrawBuffers[0][0]);
      colorRegion = (irb && irb->region) ? irb->region : NULL;
   }

   /* Update culling direction which changes depending on the
    * orientation of the buffer:
    */
   if (ctx->Driver.FrontFace)
      ctx->Driver.FrontFace(ctx, ctx->Polygon.FrontFace);
   else
      ctx->NewState |= _NEW_POLYGON;

   if (!colorRegion) {
      FALLBACK(intel, INTEL_FALLBACK_DRAW_BUFFER, GL_TRUE);
   }
   else {
      FALLBACK(intel, INTEL_FALLBACK_DRAW_BUFFER, GL_FALSE);
   }

   /***
    *** Get depth buffer region and check if we need a software fallback.
    *** Note that the depth buffer is usually a DEPTH_STENCIL buffer.
    ***/
   if (fb->_DepthBuffer && fb->_DepthBuffer->Wrapped) {
      irbDepth = intel_renderbuffer(fb->_DepthBuffer->Wrapped);
      if (irbDepth && irbDepth->region) {
         FALLBACK(intel, INTEL_FALLBACK_DEPTH_BUFFER, GL_FALSE);
         depthRegion = irbDepth->region;
      }
      else {
         FALLBACK(intel, INTEL_FALLBACK_DEPTH_BUFFER, GL_TRUE);
         depthRegion = NULL;
      }
   }
   else {
      /* not using depth buffer */
      FALLBACK(intel, INTEL_FALLBACK_DEPTH_BUFFER, GL_FALSE);
      depthRegion = NULL;
   }

   /***
    *** Stencil buffer
    *** This can only be hardware accelerated if we're using a
    *** combined DEPTH_STENCIL buffer (for now anyway).
    ***/
   if (fb->_StencilBuffer && fb->_StencilBuffer->Wrapped) {
      irbStencil = intel_renderbuffer(fb->_StencilBuffer->Wrapped);
      if (irbStencil && irbStencil->region) {
         ASSERT(irbStencil->Base._ActualFormat == GL_DEPTH24_STENCIL8_EXT);
         FALLBACK(intel, INTEL_FALLBACK_STENCIL_BUFFER, GL_FALSE);
         /* need to re-compute stencil hw state */
         ctx->Driver.Enable(ctx, GL_STENCIL_TEST, ctx->Stencil.Enabled);
         if (!depthRegion)
            depthRegion = irbStencil->region;
      }
      else {
         FALLBACK(intel, INTEL_FALLBACK_STENCIL_BUFFER, GL_TRUE);
      }
   }
   else {
      /* XXX FBO: instead of FALSE, pass ctx->Stencil.Enabled ??? */
      FALLBACK(intel, INTEL_FALLBACK_STENCIL_BUFFER, GL_FALSE);
      /* need to re-compute stencil hw state */
      ctx->Driver.Enable(ctx, GL_STENCIL_TEST, ctx->Stencil.Enabled);
   }

   /*
    * Update depth test state
    */
   if (ctx->Depth.Test && fb->Visual.depthBits > 0) {
      ctx->Driver.Enable(ctx, GL_DEPTH_TEST, GL_TRUE);
   }
   else {
      ctx->Driver.Enable(ctx, GL_DEPTH_TEST, GL_FALSE);
   }

   /**
    ** Release old regions, reference new regions
    **/
#if 0                           /* XXX FBO: this seems to be redundant with i915_state_draw_region() */
   if (intel->draw_region != colorRegion) {
      intel_region_release(&intel->draw_region);
      intel_region_reference(&intel->draw_region, colorRegion);
   }
   if (intel->intelScreen->depth_region != depthRegion) {
      intel_region_release(&intel->intelScreen->depth_region);
      intel_region_reference(&intel->intelScreen->depth_region, depthRegion);
   }
#endif

   intel->vtbl.set_draw_region(intel, colorRegion, depthRegion);

   /* update viewport since it depends on window size */
   ctx->Driver.Viewport(ctx, ctx->Viewport.X, ctx->Viewport.Y,
                        ctx->Viewport.Width, ctx->Viewport.Height);

   /* Update hardware scissor */
   ctx->Driver.Scissor(ctx, ctx->Scissor.X, ctx->Scissor.Y,
                       ctx->Scissor.Width, ctx->Scissor.Height);
}


static void
intelDrawBuffer(GLcontext * ctx, GLenum mode)
{
   intel_draw_buffer(ctx, ctx->DrawBuffer);
}


static void
intelReadBuffer(GLcontext * ctx, GLenum mode)
{
   if (ctx->ReadBuffer == ctx->DrawBuffer) {
      /* This will update FBO completeness status.
       * A framebuffer will be incomplete if the GL_READ_BUFFER setting
       * refers to a missing renderbuffer.  Calling glReadBuffer can set
       * that straight and can make the drawing buffer complete.
       */
      intel_draw_buffer(ctx, ctx->DrawBuffer);
   }
   /* Generally, functions which read pixels (glReadPixels, glCopyPixels, etc)
    * reference ctx->ReadBuffer and do appropriate state checks.
    */
}


void
intelInitBufferFuncs(struct dd_function_table *functions)
{
   functions->Clear = intelClear;
   functions->DrawBuffer = intelDrawBuffer;
   functions->ReadBuffer = intelReadBuffer;
}
