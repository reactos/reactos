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
#include "colormac.h"
#include "mtypes.h"
#include "macros.h"
#include "bufferobj.h"
#include "swrast/swrast.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_ioctl.h"
#include "intel_batchbuffer.h"
#include "intel_blit.h"
#include "intel_regions.h"
#include "intel_buffer_objects.h"



#define FILE_DEBUG_FLAG DEBUG_PIXEL


/* Unlike the other intel_pixel_* functions, the expectation here is
 * that the incoming data is not in a PBO.  With the XY_TEXT blit
 * method, there's no benefit haveing it in a PBO, but we could
 * implement a path based on XY_MONO_SRC_COPY_BLIT which might benefit
 * PBO bitmaps.  I think they are probably pretty rare though - I
 * wonder if Xgl uses them?
 */
static const GLubyte *map_pbo( GLcontext *ctx,
			       GLsizei width, GLsizei height,
			       const struct gl_pixelstore_attrib *unpack,
			       const GLubyte *bitmap )
{
   GLubyte *buf;

   if (!_mesa_validate_pbo_access(2, unpack, width, height, 1,
				  GL_COLOR_INDEX, GL_BITMAP,
				  (GLvoid *) bitmap)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,"glBitmap(invalid PBO access)");
      return NULL;
   }

   buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
					   GL_READ_ONLY_ARB,
					   unpack->BufferObj);
   if (!buf) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBitmap(PBO is mapped)");
      return NULL;
   }

   return ADD_POINTERS(buf, bitmap);
}

static GLboolean test_bit( const GLubyte *src,
			    GLuint bit )
{
   return (src[bit/8] & (1<<(bit % 8))) ? 1 : 0;
}

static void set_bit( GLubyte *dest,
			  GLuint bit )
{
   dest[bit/8] |= 1 << (bit % 8);
}

static int align(int x, int align)
{
   return (x + align - 1) & ~(align - 1);
}

/* Extract a rectangle's worth of data from the bitmap.  Called
 * per-cliprect.
 */
static GLuint get_bitmap_rect(GLsizei width, GLsizei height,
			      const struct gl_pixelstore_attrib *unpack,
			      const GLubyte *bitmap,
			      GLuint x, GLuint y, 
			      GLuint w, GLuint h,
			      GLubyte *dest,
			      GLuint row_align,
			      GLboolean invert)
{
   GLuint src_offset = (x + unpack->SkipPixels) & 0x7;
   GLuint mask = unpack->LsbFirst ? 0 : 7;
   GLuint bit = 0;
   GLint row, col;
   GLint first, last;
   GLint incr;
   GLuint count = 0;

   if (INTEL_DEBUG & DEBUG_PIXEL)
      _mesa_printf("%s %d,%d %dx%d bitmap %dx%d skip %d src_offset %d mask %d\n",
		   __FUNCTION__, x,y,w,h,width,height,unpack->SkipPixels, src_offset, mask);

   if (invert) {
      first = h-1;
      last = 0;
      incr = -1;
   }
   else {
      first = 0;
      last = h-1;
      incr = 1;
   }

   /* Require that dest be pre-zero'd.
    */
   for (row = first; row != (last+incr); row += incr) {
      const GLubyte *rowsrc = _mesa_image_address2d(unpack, bitmap, 
						    width, height, 
						    GL_COLOR_INDEX, GL_BITMAP, 
						    y + row, x);

      for (col = 0; col < w; col++, bit++) {
	 if (test_bit(rowsrc, (col + src_offset) ^ mask)) {
	    set_bit(dest, bit ^ 7);
	    count++;
	 }
      }

      if (row_align)
	 bit = (bit + row_align - 1) & ~(row_align - 1);
   }

   return count;
}




/*
 * Render a bitmap.
 */
static GLboolean
do_blit_bitmap( GLcontext *ctx, 
		GLint dstx, GLint dsty,
		GLsizei width, GLsizei height,
		const struct gl_pixelstore_attrib *unpack,
		const GLubyte *bitmap )
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_region *dst = intel_drawbuf_region(intel);
   GLfloat tmpColor[4];

   union {
      GLuint ui;
      GLubyte ub[4];
   } color;

   if (!dst)
       return GL_FALSE;

   if (unpack->BufferObj->Name) {
      bitmap = map_pbo(ctx, width, height, unpack, bitmap);
      if (bitmap == NULL)
	 return GL_TRUE;	/* even though this is an error, we're done */
   }

   COPY_4V(tmpColor, ctx->Current.RasterColor);

   if (NEED_SECONDARY_COLOR(ctx)) {
       ADD_3V(tmpColor, tmpColor, ctx->Current.RasterSecondaryColor);
   }

   UNCLAMPED_FLOAT_TO_CHAN(color.ub[0], tmpColor[2]);
   UNCLAMPED_FLOAT_TO_CHAN(color.ub[1], tmpColor[1]);
   UNCLAMPED_FLOAT_TO_CHAN(color.ub[2], tmpColor[0]);
   UNCLAMPED_FLOAT_TO_CHAN(color.ub[3], tmpColor[3]);

   /* Does zoom apply to bitmaps?
    */
   if (!intel_check_blit_fragment_ops(ctx) ||
       ctx->Pixel.ZoomX != 1.0F || 
       ctx->Pixel.ZoomY != 1.0F)
      return GL_FALSE;

   LOCK_HARDWARE(intel);

   if (intel->driDrawable->numClipRects) {
      __DRIdrawablePrivate *dPriv = intel->driDrawable;
      drm_clip_rect_t *box = dPriv->pClipRects;
      drm_clip_rect_t dest_rect;
      GLint nbox = dPriv->numClipRects;
      GLint srcx = 0, srcy = 0;
      GLint orig_screen_x1, orig_screen_y2;
      GLuint i;


      orig_screen_x1 = dPriv->x + dstx;
      orig_screen_y2 = dPriv->y + (dPriv->h - dsty);

      /* Do scissoring in GL coordinates:
       */
      if (ctx->Scissor.Enabled)
      {
	 GLint x = ctx->Scissor.X;
	 GLint y = ctx->Scissor.Y;
	 GLuint w = ctx->Scissor.Width;
	 GLuint h = ctx->Scissor.Height;

         if (!_mesa_clip_to_region(x, y, x+w-1, y+h-1, &dstx, &dsty, &width, &height))
            goto out;
      }

      /* Convert from GL to hardware coordinates:
       */
      dsty = dPriv->y + (dPriv->h - dsty - height);  
      dstx = dPriv->x + dstx;

      dest_rect.x1 = dstx;
      dest_rect.y1 = dsty;
      dest_rect.x2 = dstx + width;
      dest_rect.y2 = dsty + height;

      for (i = 0; i < nbox; i++) {
         drm_clip_rect_t rect;
	 int box_w, box_h;
	 GLint px, py;
	 GLuint stipple[32];  

         if (!intel_intersect_cliprects(&rect, &dest_rect, &box[i]))
            continue;

	 /* Now go back to GL coordinates to figure out what subset of
	  * the bitmap we are uploading for this cliprect:
	  */
	 box_w = rect.x2 - rect.x1;
	 box_h = rect.y2 - rect.y1;
	 srcx = rect.x1 - orig_screen_x1;
	 srcy = orig_screen_y2 - rect.y2;


#define DY 32
#define DX 32

	 /* Then, finally, chop it all into chunks that can be
	  * digested by hardware:
	  */
	 for (py = 0; py < box_h; py += DY) { 
	    for (px = 0; px < box_w; px += DX) { 
	       int h = MIN2(DY, box_h - py);
	       int w = MIN2(DX, box_w - px); 
	       GLuint sz = align(align(w,8) * h, 64)/8;
	       GLenum logic_op = ctx->Color.ColorLogicOpEnabled ?
		  ctx->Color.LogicOp : GL_COPY;

	       assert(sz <= sizeof(stipple));
	       memset(stipple, 0, sz);

	       /* May need to adjust this when padding has been introduced in
		* sz above:
		*/
	       if (get_bitmap_rect(width, height, unpack, 
				   bitmap,
				   srcx + px, srcy + py, w, h,
				   (GLubyte *)stipple,
				   8,
				   GL_TRUE) == 0)
		  continue;

	       /* 
		*/
	       intelEmitImmediateColorExpandBlit( intel,
						  dst->cpp,
						  (GLubyte *)stipple, 
						  sz,
						  color.ui,
						  dst->pitch,
						  dst->buffer,
						  0,
						  dst->tiled,
						  rect.x1 + px,
						  rect.y2 - (py + h),
						  w, h,
						  logic_op);
	    } 
	 } 
      }
      intel->need_flush = GL_TRUE;
   out:
      intel_batchbuffer_flush(intel->batch);
   }
   UNLOCK_HARDWARE(intel);


   if (unpack->BufferObj->Name) {
      /* done with PBO so unmap it now */
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                              unpack->BufferObj);
   }

   return GL_TRUE;
}





/* There are a large number of possible ways to implement bitmap on
 * this hardware, most of them have some sort of drawback.  Here are a
 * few that spring to mind:
 * 
 * Blit:
 *    - XY_MONO_SRC_BLT_CMD
 *         - use XY_SETUP_CLIP_BLT for cliprect clipping.
 *    - XY_TEXT_BLT
 *    - XY_TEXT_IMMEDIATE_BLT
 *         - blit per cliprect, subject to maximum immediate data size.
 *    - XY_COLOR_BLT 
 *         - per pixel or run of pixels
 *    - XY_PIXEL_BLT
 *         - good for sparse bitmaps
 *
 * 3D engine:
 *    - Point per pixel
 *    - Translate bitmap to an alpha texture and render as a quad
 *    - Chop bitmap up into 32x32 squares and render w/polygon stipple.
 */
void
intelBitmap(GLcontext * ctx,
	    GLint x, GLint y,
	    GLsizei width, GLsizei height,
	    const struct gl_pixelstore_attrib *unpack,
	    const GLubyte * pixels)
{
   if (do_blit_bitmap(ctx, x, y, width, height,
                          unpack, pixels))
      return;

   if (INTEL_DEBUG & DEBUG_PIXEL)
      _mesa_printf("%s: fallback to swrast\n", __FUNCTION__);

   _swrast_Bitmap(ctx, x, y, width, height, unpack, pixels);
}
