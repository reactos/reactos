/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * next paragraph) shall be included in all copies or substantial portionsalloc
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
#include "image.h"
#include "mtypes.h"
#include "macros.h"
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
#include "intel_tris.h"



static GLboolean
do_texture_drawpixels(GLcontext * ctx,
                      GLint x, GLint y,
                      GLsizei width, GLsizei height,
                      GLenum format, GLenum type,
                      const struct gl_pixelstore_attrib *unpack,
                      const GLvoid * pixels)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_region *dst = intel_drawbuf_region(intel);
   struct intel_buffer_object *src = intel_buffer_object(unpack->BufferObj);
   GLuint rowLength = unpack->RowLength ? unpack->RowLength : width;
   GLuint src_offset;

   if (INTEL_DEBUG & DEBUG_PIXEL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   intelFlush(&intel->ctx);
   intel->vtbl.render_start(intel);
   intel->vtbl.emit_state(intel);

   if (!dst)
      return GL_FALSE;

   if (src) {
      if (!_mesa_validate_pbo_access(2, unpack, width, height, 1,
                                     format, type, pixels)) {
         _mesa_error(ctx, GL_INVALID_OPERATION, "glDrawPixels");
         return GL_TRUE;
      }
   }
   else {
      /* PBO only for now:
       */
/*       _mesa_printf("%s - not PBO\n", __FUNCTION__); */
      return GL_FALSE;
   }

   /* There are a couple of things we can't do yet, one of which is
    * set the correct state for pixel operations when GL texturing is
    * enabled.  That's a pretty rare state and probably not worth the
    * effort.  A completely device-independent version of this may do
    * more.
    *
    * Similarly, we make no attempt to merge metaops processing with
    * an enabled fragment program, though it would certainly be
    * possible.
    */
   if (!intel_check_meta_tex_fragment_ops(ctx)) {
      if (INTEL_DEBUG & DEBUG_PIXEL)
         _mesa_printf("%s - bad GL fragment state for metaops texture\n",
                      __FUNCTION__);
      return GL_FALSE;
   }

   intel->vtbl.install_meta_state(intel);


   /* Is this true?  Also will need to turn depth testing on according
    * to state:
    */
   intel->vtbl.meta_no_stencil_write(intel);
   intel->vtbl.meta_no_depth_write(intel);

   /* Set the 3d engine to draw into the destination region:
    */
   intel->vtbl.meta_draw_region(intel, dst, intel->intelScreen->depth_region);

   intel->vtbl.meta_import_pixel_state(intel);

   src_offset = (GLuint) _mesa_image_address(2, unpack, pixels, width, height,
                                             format, type, 0, 0, 0);


   /* Setup the pbo up as a rectangular texture, if possible.
    *
    * TODO: This is almost always possible if the i915 fragment
    * program is adjusted to correctly swizzle the sampled colors.
    * The major exception is any 24bit texture, like RGB888, for which
    * there is no hardware support.  
    */
   if (!intel->vtbl.meta_tex_rect_source(intel, src->buffer, src_offset,
                                         rowLength, height, format, type)) {
      intel->vtbl.leave_meta_state(intel);
      return GL_FALSE;
   }

   intel->vtbl.meta_texture_blend_replace(intel);


   LOCK_HARDWARE(intel);

   if (intel->driDrawable->numClipRects) {
      __DRIdrawablePrivate *dPriv = intel->driDrawable;
      GLint srcx, srcy;
      GLint dstx, dsty;

      dstx = x;
      dsty = dPriv->h - (y + height);

      srcx = 0;                 /* skiprows/pixels already done */
      srcy = 0;

      if (0) {
         const GLint orig_x = dstx;
         const GLint orig_y = dsty;

         if (!_mesa_clip_to_region(0, 0, dst->pitch, dst->height,
                                   &dstx, &dsty, &width, &height))
            goto out;

         srcx += dstx - orig_x;
         srcy += dsty - orig_y;
      }


      if (INTEL_DEBUG & DEBUG_PIXEL)
         _mesa_printf("draw %d,%d %dx%d\n", dstx, dsty, width, height);

      /* Must use the regular cliprect mechanism in order to get the
       * drawing origin set correctly.  Otherwise scissor state is in
       * incorrect coordinate space.  Does this even need to hold the
       * lock???
       */
      intel_meta_draw_quad(intel,
                           dstx, dstx + width * ctx->Pixel.ZoomX,
                           dPriv->h - (y + height * ctx->Pixel.ZoomY),
                           dPriv->h - (y),
                           -ctx->Current.RasterPos[2] * .5,
                           0x00ff00ff,
                           srcx, srcx + width, srcy + height, srcy);
    out:
      intel->vtbl.leave_meta_state(intel);
      intel_batchbuffer_flush(intel->batch);
   }
   UNLOCK_HARDWARE(intel);
   return GL_TRUE;
}





/* Pros:  
 *   - no waiting for idle before updating framebuffer.
 *   
 * Cons:
 *   - if upload is by memcpy, this may actually be slower than fallback path.
 *   - uploads the whole image even if destination is clipped
 *   
 * Need to benchmark.
 *
 * Given the questions about performance, implement for pbo's only.
 * This path is definitely a win if the pbo is already in agp.  If it
 * turns out otherwise, we can add the code necessary to upload client
 * data to agp space before performing the blit.  (Though it may turn
 * out to be better/simpler just to use the texture engine).
 */
static GLboolean
do_blit_drawpixels(GLcontext * ctx,
                   GLint x, GLint y,
                   GLsizei width, GLsizei height,
                   GLenum format, GLenum type,
                   const struct gl_pixelstore_attrib *unpack,
                   const GLvoid * pixels)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_region *dest = intel_drawbuf_region(intel);
   struct intel_buffer_object *src = intel_buffer_object(unpack->BufferObj);
   GLuint src_offset;
   GLuint rowLength;
   struct _DriFenceObject *fence = NULL;

   if (INTEL_DEBUG & DEBUG_PIXEL)
      _mesa_printf("%s\n", __FUNCTION__);


   if (!dest) {
      if (INTEL_DEBUG & DEBUG_PIXEL)
         _mesa_printf("%s - no dest\n", __FUNCTION__);
      return GL_FALSE;
   }

   if (src) {
      /* This validation should be done by core mesa:
       */
      if (!_mesa_validate_pbo_access(2, unpack, width, height, 1,
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

   if (!intel_check_blit_format(dest, format, type)) {
      if (INTEL_DEBUG & DEBUG_PIXEL)
         _mesa_printf("%s - bad format for blit\n", __FUNCTION__);
      return GL_FALSE;
   }

   if (!intel_check_blit_fragment_ops(ctx)) {
      if (INTEL_DEBUG & DEBUG_PIXEL)
         _mesa_printf("%s - bad GL fragment state for blitter\n",
                      __FUNCTION__);
      return GL_FALSE;
   }

   if (ctx->Pixel.ZoomX != 1.0F) {
      if (INTEL_DEBUG & DEBUG_PIXEL)
         _mesa_printf("%s - bad PixelZoomX for blit\n", __FUNCTION__);
      return GL_FALSE;
   }


   if (unpack->RowLength > 0)
      rowLength = unpack->RowLength;
   else
      rowLength = width;

   if (ctx->Pixel.ZoomY == -1.0F) {
      if (INTEL_DEBUG & DEBUG_PIXEL)
         _mesa_printf("%s - bad PixelZoomY for blit\n", __FUNCTION__);
      return GL_FALSE;          /* later */
      y -= height;
   }
   else if (ctx->Pixel.ZoomY == 1.0F) {
      rowLength = -rowLength;
   }
   else {
      if (INTEL_DEBUG & DEBUG_PIXEL)
         _mesa_printf("%s - bad PixelZoomY for blit\n", __FUNCTION__);
      return GL_FALSE;
   }

   src_offset = (GLuint) _mesa_image_address(2, unpack, pixels, width, height,
                                             format, type, 0, 0, 0);

   intelFlush(&intel->ctx);
   LOCK_HARDWARE(intel);

   if (intel->driDrawable->numClipRects) {
      __DRIdrawablePrivate *dPriv = intel->driDrawable;
      int nbox = dPriv->numClipRects;
      drm_clip_rect_t *box = dPriv->pClipRects;
      drm_clip_rect_t rect;
      drm_clip_rect_t dest_rect;
      struct _DriBufferObject *src_buffer =
         intel_bufferobj_buffer(intel, src, INTEL_READ);
      int i;

      dest_rect.x1 = dPriv->x + x;
      dest_rect.y1 = dPriv->y + dPriv->h - (y + height);
      dest_rect.x2 = dest_rect.x1 + width;
      dest_rect.y2 = dest_rect.y1 + height;

      for (i = 0; i < nbox; i++) {
         if (!intel_intersect_cliprects(&rect, &dest_rect, &box[i]))
            continue;

         intelEmitCopyBlit(intel,
                           dest->cpp,
                           rowLength,
                           src_buffer, src_offset,
                           dest->pitch,
                           dest->buffer, 0,
                           rect.x1 - dest_rect.x1,
                           rect.y2 - dest_rect.y2,
                           rect.x1,
                           rect.y1, rect.x2 - rect.x1, rect.y2 - rect.y1,
			   ctx->Color.ColorLogicOpEnabled ?
			   ctx->Color.LogicOp : GL_COPY);
      }
      fence = intel_batchbuffer_flush(intel->batch);
      driFenceReference(fence);
   }
   UNLOCK_HARDWARE(intel);

   if (fence) {
      driFenceFinish(fence, DRM_FENCE_TYPE_EXE | DRM_I915_FENCE_TYPE_RW, GL_FALSE);
      driFenceUnReference(fence);
   }

   if (INTEL_DEBUG & DEBUG_PIXEL)
      _mesa_printf("%s - DONE\n", __FUNCTION__);

   return GL_TRUE;
}



void
intelDrawPixels(GLcontext * ctx,
                GLint x, GLint y,
                GLsizei width, GLsizei height,
                GLenum format,
                GLenum type,
                const struct gl_pixelstore_attrib *unpack,
                const GLvoid * pixels)
{
   if (do_blit_drawpixels(ctx, x, y, width, height, format, type,
                          unpack, pixels))
      return;

   if (do_texture_drawpixels(ctx, x, y, width, height, format, type,
                             unpack, pixels))
      return;


   if (INTEL_DEBUG & DEBUG_PIXEL)
      _mesa_printf("%s: fallback to swrast\n", __FUNCTION__);

   _swrast_DrawPixels(ctx, x, y, width, height, format, type, unpack, pixels);
}
