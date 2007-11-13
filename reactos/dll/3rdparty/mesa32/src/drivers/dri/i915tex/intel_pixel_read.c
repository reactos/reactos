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

#include "glheader.h"
#include "enums.h"
#include "mtypes.h"
#include "macros.h"
#include "image.h"
#include "bufferobj.h"
#include "swrast/swrast.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_ioctl.h"
#include "intel_batchbuffer.h"
#include "intel_blit.h"
#include "intel_buffers.h"
#include "intel_regions.h"
#include "intel_pixel.h"
#include "intel_buffer_objects.h"

/* For many applications, the new ability to pull the source buffers
 * back out of the GTT and then do the packing/conversion operations
 * in software will be as much of an improvement as trying to get the
 * blitter and/or texture engine to do the work. 
 *
 * This step is gated on private backbuffers.
 * 
 * Obviously the frontbuffer can't be pulled back, so that is either
 * an argument for blit/texture readpixels, or for blitting to a
 * temporary and then pulling that back.
 *
 * When the destination is a pbo, however, it's not clear if it is
 * ever going to be pulled to main memory (though the access param
 * will be a good hint).  So it sounds like we do want to be able to
 * choose between blit/texture implementation on the gpu and pullback
 * and cpu-based copying.
 *
 * Unless you can magically turn client memory into a PBO for the
 * duration of this call, there will be a cpu-based copying step in
 * any case.
 */


static GLboolean
do_texture_readpixels(GLcontext * ctx,
                      GLint x, GLint y, GLsizei width, GLsizei height,
                      GLenum format, GLenum type,
                      const struct gl_pixelstore_attrib *pack,
                      struct intel_region *dest_region)
{
#if 0
   struct intel_context *intel = intel_context(ctx);
   intelScreenPrivate *screen = intel->intelScreen;
   GLint pitch = pack->RowLength ? pack->RowLength : width;
   __DRIdrawablePrivate *dPriv = intel->driDrawable;
   int textureFormat;
   GLenum glTextureFormat;
   int destFormat, depthFormat, destPitch;
   drm_clip_rect_t tmp;

   if (INTEL_DEBUG & DEBUG_PIXEL)
      fprintf(stderr, "%s\n", __FUNCTION__);


   if (ctx->_ImageTransferState ||
       pack->SwapBytes || pack->LsbFirst || !pack->Invert) {
      if (INTEL_DEBUG & DEBUG_PIXEL)
         fprintf(stderr, "%s: check_color failed\n", __FUNCTION__);
      return GL_FALSE;
   }

   intel->vtbl.meta_texrect_source(intel, intel_readbuf_region(intel));

   if (!intel->vtbl.meta_render_dest(intel, dest_region, type, format)) {
      if (INTEL_DEBUG & DEBUG_PIXEL)
         fprintf(stderr, "%s: couldn't set dest %s/%s\n",
                 __FUNCTION__,
                 _mesa_lookup_enum_by_nr(type),
                 _mesa_lookup_enum_by_nr(format));
      return GL_FALSE;
   }

   LOCK_HARDWARE(intel);

   if (intel->driDrawable->numClipRects) {
      intel->vtbl.install_meta_state(intel);
      intel->vtbl.meta_no_depth_write(intel);
      intel->vtbl.meta_no_stencil_write(intel);

      if (!driClipRectToFramebuffer(ctx->ReadBuffer, &x, &y, &width, &height)) {
         UNLOCK_HARDWARE(intel);
         SET_STATE(i830, state);
         if (INTEL_DEBUG & DEBUG_PIXEL)
            fprintf(stderr, "%s: cliprect failed\n", __FUNCTION__);
         return GL_TRUE;
      }

      y = dPriv->h - y - height;
      x += dPriv->x;
      y += dPriv->y;


      /* Set the frontbuffer up as a large rectangular texture.
       */
      intel->vtbl.meta_tex_rect_source(intel, src_region, textureFormat);


      intel->vtbl.meta_texture_blend_replace(i830, glTextureFormat);


      /* Set the 3d engine to draw into the destination region:
       */

      intel->vtbl.meta_draw_region(intel, dest_region);
      intel->vtbl.meta_draw_format(intel, destFormat, depthFormat);     /* ?? */


      /* Draw a single quad, no cliprects:
       */
      intel->vtbl.meta_disable_cliprects(intel);

      intel->vtbl.draw_quad(intel,
                            0, width, 0, height,
                            0x00ff00ff, x, x + width, y, y + height);

      intel->vtbl.leave_meta_state(intel);
   }
   UNLOCK_HARDWARE(intel);

   intel_region_wait_fence(ctx, dest_region);   /* required by GL */
   return GL_TRUE;
#endif

   return GL_FALSE;
}




static GLboolean
do_blit_readpixels(GLcontext * ctx,
                   GLint x, GLint y, GLsizei width, GLsizei height,
                   GLenum format, GLenum type,
                   const struct gl_pixelstore_attrib *pack, GLvoid * pixels)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_region *src = intel_readbuf_region(intel);
   struct intel_buffer_object *dst = intel_buffer_object(pack->BufferObj);
   GLuint dst_offset;
   GLuint rowLength;
   struct _DriFenceObject *fence = NULL;

   if (INTEL_DEBUG & DEBUG_PIXEL)
      _mesa_printf("%s\n", __FUNCTION__);

   if (!src)
      return GL_FALSE;

   if (dst) {
      /* XXX This validation should be done by core mesa:
       */
      if (!_mesa_validate_pbo_access(2, pack, width, height, 1,
                                     format, type, pixels)) {
         _mesa_error(ctx, GL_INVALID_OPERATION, "glDrawPixels");
         return GL_TRUE;
      }
   }
   else {
      /* PBO only for now:
       */
      if (INTEL_DEBUG & DEBUG_PIXEL)
         _mesa_printf("%s - not PBO\n", __FUNCTION__);
      return GL_FALSE;
   }


   if (ctx->_ImageTransferState ||
       !intel_check_blit_format(src, format, type)) {
      if (INTEL_DEBUG & DEBUG_PIXEL)
         _mesa_printf("%s - bad format for blit\n", __FUNCTION__);
      return GL_FALSE;
   }

   if (pack->Alignment != 1 || pack->SwapBytes || pack->LsbFirst) {
      if (INTEL_DEBUG & DEBUG_PIXEL)
         _mesa_printf("%s: bad packing params\n", __FUNCTION__);
      return GL_FALSE;
   }

   if (pack->RowLength > 0)
      rowLength = pack->RowLength;
   else
      rowLength = width;

   if (pack->Invert) {
      if (INTEL_DEBUG & DEBUG_PIXEL)
         _mesa_printf("%s: MESA_PACK_INVERT not done yet\n", __FUNCTION__);
      return GL_FALSE;
   }
   else {
      rowLength = -rowLength;
   }

   /* XXX 64-bit cast? */
   dst_offset = (GLuint) _mesa_image_address(2, pack, pixels, width, height,
                                             format, type, 0, 0, 0);


   /* Although the blits go on the command buffer, need to do this and
    * fire with lock held to guarentee cliprects are correct.
    */
   intelFlush(&intel->ctx);
   LOCK_HARDWARE(intel);

   if (intel->driDrawable->numClipRects) {
      GLboolean all = (width * height * src->cpp == dst->Base.Size &&
                       x == 0 && dst_offset == 0);

      struct _DriBufferObject *dst_buffer =
         intel_bufferobj_buffer(intel, dst, all ? INTEL_WRITE_FULL :
                                INTEL_WRITE_PART);
      __DRIdrawablePrivate *dPriv = intel->driDrawable;
      int nbox = dPriv->numClipRects;
      drm_clip_rect_t *box = dPriv->pClipRects;
      drm_clip_rect_t rect;
      drm_clip_rect_t src_rect;
      int i;

      src_rect.x1 = dPriv->x + x;
      src_rect.y1 = dPriv->y + dPriv->h - (y + height);
      src_rect.x2 = src_rect.x1 + width;
      src_rect.y2 = src_rect.y1 + height;



      for (i = 0; i < nbox; i++) {
         if (!intel_intersect_cliprects(&rect, &src_rect, &box[i]))
            continue;

         intelEmitCopyBlit(intel,
                           src->cpp,
                           src->pitch, src->buffer, 0,
                           rowLength,
                           dst_buffer, dst_offset,
                           rect.x1,
                           rect.y1,
                           rect.x1 - src_rect.x1,
                           rect.y2 - src_rect.y2,
                           rect.x2 - rect.x1, rect.y2 - rect.y1,
			   GL_COPY);
      }

      fence = intel_batchbuffer_flush(intel->batch);
      driFenceReference(fence);

   }
   UNLOCK_HARDWARE(intel);

   if (fence) {
      driFenceFinish(fence, DRM_FENCE_TYPE_EXE | DRM_I915_FENCE_TYPE_RW, 
		     GL_FALSE);
      driFenceUnReference(fence);
   }

   if (INTEL_DEBUG & DEBUG_PIXEL)
      _mesa_printf("%s - DONE\n", __FUNCTION__);

   return GL_TRUE;
}

void
intelReadPixels(GLcontext * ctx,
                GLint x, GLint y, GLsizei width, GLsizei height,
                GLenum format, GLenum type,
                const struct gl_pixelstore_attrib *pack, GLvoid * pixels)
{
   if (INTEL_DEBUG & DEBUG_PIXEL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   intelFlush(ctx);

   if (do_blit_readpixels
       (ctx, x, y, width, height, format, type, pack, pixels))
      return;

   if (do_texture_readpixels
       (ctx, x, y, width, height, format, type, pack, pixels))
      return;

   if (INTEL_DEBUG & DEBUG_PIXEL)
      _mesa_printf("%s: fallback to swrast\n", __FUNCTION__);

   _swrast_ReadPixels(ctx, x, y, width, height, format, type, pack, pixels);
}
