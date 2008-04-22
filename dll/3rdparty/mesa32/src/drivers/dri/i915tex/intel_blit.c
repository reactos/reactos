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


#include <stdio.h>
#include <errno.h>

#include "mtypes.h"
#include "context.h"
#include "enums.h"

#include "intel_batchbuffer.h"
#include "intel_blit.h"
#include "intel_buffers.h"
#include "intel_context.h"
#include "intel_fbo.h"
#include "intel_reg.h"
#include "intel_regions.h"
#include "vblank.h"

#define FILE_DEBUG_FLAG DEBUG_BLIT

/**
 * Copy the back color buffer to the front color buffer. 
 * Used for SwapBuffers().
 */
void
intelCopyBuffer(const __DRIdrawablePrivate * dPriv,
                const drm_clip_rect_t * rect)
{

   struct intel_context *intel;
   const intelScreenPrivate *intelScreen;

   DBG("%s\n", __FUNCTION__);

   assert(dPriv);

   intel = intelScreenContext(dPriv->driScreenPriv->private);
   if (!intel)
      return;

   intelScreen = intel->intelScreen;

   if (intel->last_swap_fence) {
      driFenceFinish(intel->last_swap_fence, DRM_FENCE_TYPE_EXE, GL_TRUE);
      driFenceUnReference(intel->last_swap_fence);
      intel->last_swap_fence = NULL;
   }
   intel->last_swap_fence = intel->first_swap_fence;
   intel->first_swap_fence = NULL;

   /* The LOCK_HARDWARE is required for the cliprects.  Buffer offsets
    * should work regardless.
    */
   LOCK_HARDWARE(intel);

   if (dPriv && dPriv->numClipRects) {
      struct intel_framebuffer *intel_fb = dPriv->driverPrivate;
      const struct intel_region *frontRegion
	 = intel_get_rb_region(&intel_fb->Base, BUFFER_FRONT_LEFT);
      const struct intel_region *backRegion
	 = intel_get_rb_region(&intel_fb->Base, BUFFER_BACK_LEFT);
      const int nbox = dPriv->numClipRects;
      const drm_clip_rect_t *pbox = dPriv->pClipRects;
      const int pitch = frontRegion->pitch;
      const int cpp = frontRegion->cpp;
      int BR13, CMD;
      int i;

      ASSERT(intel_fb);
      ASSERT(intel_fb->Base.Name == 0);    /* Not a user-created FBO */
      ASSERT(frontRegion);
      ASSERT(backRegion);
      ASSERT(frontRegion->pitch == backRegion->pitch);
      ASSERT(frontRegion->cpp == backRegion->cpp);

      if (cpp == 2) {
	 BR13 = (pitch * cpp) | (0xCC << 16) | (1 << 24);
	 CMD = XY_SRC_COPY_BLT_CMD;
      }
      else {
	 BR13 = (pitch * cpp) | (0xCC << 16) | (1 << 24) | (1 << 25);
	 CMD = (XY_SRC_COPY_BLT_CMD | XY_SRC_COPY_BLT_WRITE_ALPHA |
		XY_SRC_COPY_BLT_WRITE_RGB);
      }

      for (i = 0; i < nbox; i++, pbox++) {
	 drm_clip_rect_t box;

	 if (pbox->x1 > pbox->x2 ||
	     pbox->y1 > pbox->y2 ||
	     pbox->x2 > intelScreen->width || pbox->y2 > intelScreen->height)
	    continue;

	 box = *pbox;

	 if (rect) {
	    if (rect->x1 > box.x1)
	       box.x1 = rect->x1;
	    if (rect->y1 > box.y1)
	       box.y1 = rect->y1;
	    if (rect->x2 < box.x2)
	       box.x2 = rect->x2;
	    if (rect->y2 < box.y2)
	       box.y2 = rect->y2;

	    if (box.x1 > box.x2 || box.y1 > box.y2)
	       continue;
	 }

	 BEGIN_BATCH(8, INTEL_BATCH_NO_CLIPRECTS);
	 OUT_BATCH(CMD);
	 OUT_BATCH(BR13);
	 OUT_BATCH((pbox->y1 << 16) | pbox->x1);
	 OUT_BATCH((pbox->y2 << 16) | pbox->x2);

	 OUT_RELOC(frontRegion->buffer, DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_WRITE,
		   DRM_BO_MASK_MEM | DRM_BO_FLAG_WRITE, 0);
	 OUT_BATCH((pbox->y1 << 16) | pbox->x1);
	 OUT_BATCH(BR13 & 0xffff);
	 OUT_RELOC(backRegion->buffer, DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_READ,
		   DRM_BO_MASK_MEM | DRM_BO_FLAG_READ, 0);

	 ADVANCE_BATCH();
      }

      if (intel->first_swap_fence)
	 driFenceUnReference(intel->first_swap_fence);
      intel->first_swap_fence = intel_batchbuffer_flush(intel->batch);
      driFenceReference(intel->first_swap_fence);
   }

   UNLOCK_HARDWARE(intel);
}




void
intelEmitFillBlit(struct intel_context *intel,
                  GLuint cpp,
                  GLshort dst_pitch,
                  struct _DriBufferObject *dst_buffer,
                  GLuint dst_offset,
                  GLshort x, GLshort y, GLshort w, GLshort h, GLuint color)
{
   GLuint BR13, CMD;
   BATCH_LOCALS;

   dst_pitch *= cpp;

   switch (cpp) {
   case 1:
   case 2:
   case 3:
      BR13 = dst_pitch | (0xF0 << 16) | (1 << 24);
      CMD = XY_COLOR_BLT_CMD;
      break;
   case 4:
      BR13 = dst_pitch | (0xF0 << 16) | (1 << 24) | (1 << 25);
      CMD = (XY_COLOR_BLT_CMD | XY_COLOR_BLT_WRITE_ALPHA |
             XY_COLOR_BLT_WRITE_RGB);
      break;
   default:
      return;
   }

   DBG("%s dst:buf(%p)/%d+%d %d,%d sz:%dx%d\n",
       __FUNCTION__, dst_buffer, dst_pitch, dst_offset, x, y, w, h);


   BEGIN_BATCH(6, INTEL_BATCH_NO_CLIPRECTS);
   OUT_BATCH(CMD);
   OUT_BATCH(BR13);
   OUT_BATCH((y << 16) | x);
   OUT_BATCH(((y + h) << 16) | (x + w));
   OUT_RELOC(dst_buffer, DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_WRITE,
             DRM_BO_MASK_MEM | DRM_BO_FLAG_WRITE, dst_offset);
   OUT_BATCH(color);
   ADVANCE_BATCH();
}


static GLuint translate_raster_op(GLenum logicop)
{
   switch(logicop) {
   case GL_CLEAR: return 0x00;
   case GL_AND: return 0x88;
   case GL_AND_REVERSE: return 0x44;
   case GL_COPY: return 0xCC;
   case GL_AND_INVERTED: return 0x22;
   case GL_NOOP: return 0xAA;
   case GL_XOR: return 0x66;
   case GL_OR: return 0xEE;
   case GL_NOR: return 0x11;
   case GL_EQUIV: return 0x99;
   case GL_INVERT: return 0x55;
   case GL_OR_REVERSE: return 0xDD;
   case GL_COPY_INVERTED: return 0x33;
   case GL_OR_INVERTED: return 0xBB;
   case GL_NAND: return 0x77;
   case GL_SET: return 0xFF;
   default: return 0;
   }
}


/* Copy BitBlt
 */
void
intelEmitCopyBlit(struct intel_context *intel,
                  GLuint cpp,
                  GLshort src_pitch,
                  struct _DriBufferObject *src_buffer,
                  GLuint src_offset,
                  GLshort dst_pitch,
                  struct _DriBufferObject *dst_buffer,
                  GLuint dst_offset,
                  GLshort src_x, GLshort src_y,
                  GLshort dst_x, GLshort dst_y, 
		  GLshort w, GLshort h,
		  GLenum logic_op)
{
   GLuint CMD, BR13;
   int dst_y2 = dst_y + h;
   int dst_x2 = dst_x + w;
   BATCH_LOCALS;


   DBG("%s src:buf(%p)/%d+%d %d,%d dst:buf(%p)/%d+%d %d,%d sz:%dx%d\n",
       __FUNCTION__,
       src_buffer, src_pitch, src_offset, src_x, src_y,
       dst_buffer, dst_pitch, dst_offset, dst_x, dst_y, w, h);

   src_pitch *= cpp;
   dst_pitch *= cpp;

   switch (cpp) {
   case 1:
   case 2:
   case 3:
      BR13 = (((GLint) dst_pitch) & 0xffff) | 
	 (translate_raster_op(logic_op) << 16) | (1 << 24);
      CMD = XY_SRC_COPY_BLT_CMD;
      break;
   case 4:
      BR13 =
         (((GLint) dst_pitch) & 0xffff) | 
	 (translate_raster_op(logic_op) << 16) | (1 << 24) | (1 << 25);
      CMD =
         (XY_SRC_COPY_BLT_CMD | XY_SRC_COPY_BLT_WRITE_ALPHA |
          XY_SRC_COPY_BLT_WRITE_RGB);
      break;
   default:
      return;
   }

   if (dst_y2 < dst_y || dst_x2 < dst_x) {
      return;
   }

   /* Initial y values don't seem to work with negative pitches.  If
    * we adjust the offsets manually (below), it seems to work fine.
    *
    * On the other hand, if we always adjust, the hardware doesn't
    * know which blit directions to use, so overlapping copypixels get
    * the wrong result.
    */
   if (dst_pitch > 0 && src_pitch > 0) {
      BEGIN_BATCH(8, INTEL_BATCH_NO_CLIPRECTS);
      OUT_BATCH(CMD);
      OUT_BATCH(BR13);
      OUT_BATCH((dst_y << 16) | dst_x);
      OUT_BATCH((dst_y2 << 16) | dst_x2);
      OUT_RELOC(dst_buffer, DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_WRITE,
                DRM_BO_MASK_MEM | DRM_BO_FLAG_WRITE, dst_offset);
      OUT_BATCH((src_y << 16) | src_x);
      OUT_BATCH(((GLint) src_pitch & 0xffff));
      OUT_RELOC(src_buffer, DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_READ,
                DRM_BO_MASK_MEM | DRM_BO_FLAG_READ, src_offset);
      ADVANCE_BATCH();
   }
   else {
      BEGIN_BATCH(8, INTEL_BATCH_NO_CLIPRECTS);
      OUT_BATCH(CMD);
      OUT_BATCH(BR13);
      OUT_BATCH((0 << 16) | dst_x);
      OUT_BATCH((h << 16) | dst_x2);
      OUT_RELOC(dst_buffer, DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_WRITE,
                DRM_BO_MASK_MEM | DRM_BO_FLAG_WRITE,
                dst_offset + dst_y * dst_pitch);
      OUT_BATCH((0 << 16) | src_x);
      OUT_BATCH(((GLint) src_pitch & 0xffff));
      OUT_RELOC(src_buffer, DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_READ,
                DRM_BO_MASK_MEM | DRM_BO_FLAG_READ,
                src_offset + src_y * src_pitch);
      ADVANCE_BATCH();
   }
}


/**
 * Use blitting to clear the renderbuffers named by 'flags'.
 * Note: we can't use the ctx->DrawBuffer->_ColorDrawBufferMask field
 * since that might include software renderbuffers or renderbuffers
 * which we're clearing with triangles.
 * \param mask  bitmask of BUFFER_BIT_* values indicating buffers to clear
 */
void
intelClearWithBlit(GLcontext * ctx, GLbitfield mask)
{
   struct intel_context *intel = intel_context(ctx);
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   GLuint clear_depth;
   GLbitfield skipBuffers = 0;
   BATCH_LOCALS;

   DBG("%s %x\n", __FUNCTION__, mask);

   /*
    * Compute values for clearing the buffers.
    */
   clear_depth = 0;
   if (mask & BUFFER_BIT_DEPTH) {
      clear_depth = (GLuint) (fb->_DepthMax * ctx->Depth.Clear);
   }
   if (mask & BUFFER_BIT_STENCIL) {
      clear_depth |= (ctx->Stencil.Clear & 0xff) << 24;
   }

   /* If clearing both depth and stencil, skip BUFFER_BIT_STENCIL in
    * the loop below.
    */
   if ((mask & BUFFER_BIT_DEPTH) && (mask & BUFFER_BIT_STENCIL)) {
      skipBuffers = BUFFER_BIT_STENCIL;
   }

   /* XXX Move this flush/lock into the following conditional? */
   intelFlush(&intel->ctx);
   LOCK_HARDWARE(intel);

   if (intel->numClipRects) {
      GLint cx, cy, cw, ch;
      drm_clip_rect_t clear;
      int i;

      /* Get clear bounds after locking */
      cx = fb->_Xmin;
      cy = fb->_Ymin;
      cw = fb->_Xmax - cx;
      ch = fb->_Ymax - cy;

      if (fb->Name == 0) {
         /* clearing a window */

         /* flip top to bottom */
         clear.x1 = cx + intel->drawX;
         clear.y1 = intel->driDrawable->y + intel->driDrawable->h - cy - ch;
         clear.x2 = clear.x1 + cw;
         clear.y2 = clear.y1 + ch;
      }
      else {
         /* clearing FBO */
         assert(intel->numClipRects == 1);
         assert(intel->pClipRects == &intel->fboRect);
         clear.x1 = cx;
         clear.y1 = cy;
         clear.x2 = clear.x1 + cw;
         clear.y2 = clear.y1 + ch;
         /* no change to mask */
      }

      for (i = 0; i < intel->numClipRects; i++) {
         const drm_clip_rect_t *box = &intel->pClipRects[i];
         drm_clip_rect_t b;
         GLuint buf;
         GLuint clearMask = mask;      /* use copy, since we modify it below */
         GLboolean all = (cw == fb->Width && ch == fb->Height);

         if (!all) {
            intel_intersect_cliprects(&b, &clear, box);
         }
         else {
            b = *box;
         }

         if (b.x1 >= b.x2 || b.y1 >= b.y2)
            continue;

         if (0)
            _mesa_printf("clear %d,%d..%d,%d, mask %x\n",
                         b.x1, b.y1, b.x2, b.y2, mask);

         /* Loop over all renderbuffers */
         for (buf = 0; buf < BUFFER_COUNT && clearMask; buf++) {
            const GLbitfield bufBit = 1 << buf;
            if ((clearMask & bufBit) && !(bufBit & skipBuffers)) {
               /* OK, clear this renderbuffer */
               struct intel_region *irb_region =
		  intel_get_rb_region(fb, buf);
               struct _DriBufferObject *write_buffer =
                  intel_region_buffer(intel->intelScreen, irb_region,
                                      all ? INTEL_WRITE_FULL :
                                      INTEL_WRITE_PART);

               GLuint clearVal;
               GLint pitch, cpp;
               GLuint BR13, CMD;

               ASSERT(irb_region);

               pitch = irb_region->pitch;
               cpp = irb_region->cpp;

               DBG("%s dst:buf(%p)/%d+%d %d,%d sz:%dx%d\n",
                   __FUNCTION__,
                   irb_region->buffer, (pitch * cpp),
                   irb_region->draw_offset,
                   b.x1, b.y1, b.x2 - b.x1, b.y2 - b.y1);


               /* Setup the blit command */
               if (cpp == 4) {
                  BR13 = (0xF0 << 16) | (pitch * cpp) | (1 << 24) | (1 << 25);
                  if (buf == BUFFER_DEPTH || buf == BUFFER_STENCIL) {
                     CMD = XY_COLOR_BLT_CMD;
                     if (clearMask & BUFFER_BIT_DEPTH)
                        CMD |= XY_COLOR_BLT_WRITE_RGB;
                     if (clearMask & BUFFER_BIT_STENCIL)
                        CMD |= XY_COLOR_BLT_WRITE_ALPHA;
                  }
                  else {
                     /* clearing RGBA */
                     CMD = (XY_COLOR_BLT_CMD |
                            XY_COLOR_BLT_WRITE_ALPHA |
                            XY_COLOR_BLT_WRITE_RGB);
                  }
               }
               else {
                  ASSERT(cpp == 2 || cpp == 0);
                  BR13 = (0xF0 << 16) | (pitch * cpp) | (1 << 24);
                  CMD = XY_COLOR_BLT_CMD;
               }

               if (buf == BUFFER_DEPTH || buf == BUFFER_STENCIL) {
                  clearVal = clear_depth;
               }
               else {
                  clearVal = (cpp == 4)
                     ? intel->ClearColor8888 : intel->ClearColor565;
               }
               /*
                  _mesa_debug(ctx, "hardware blit clear buf %d rb id %d\n",
                  buf, irb->Base.Name);
                */
	       intel_wait_flips(intel, INTEL_BATCH_NO_CLIPRECTS);

               BEGIN_BATCH(6, INTEL_BATCH_NO_CLIPRECTS);
               OUT_BATCH(CMD);
               OUT_BATCH(BR13);
               OUT_BATCH((b.y1 << 16) | b.x1);
               OUT_BATCH((b.y2 << 16) | b.x2);
               OUT_RELOC(write_buffer, DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_WRITE,
                         DRM_BO_MASK_MEM | DRM_BO_FLAG_WRITE,
                         irb_region->draw_offset);
               OUT_BATCH(clearVal);
               ADVANCE_BATCH();
               clearMask &= ~bufBit;    /* turn off bit, for faster loop exit */
            }
         }
      }
      intel_batchbuffer_flush(intel->batch);
   }

   UNLOCK_HARDWARE(intel);
}
