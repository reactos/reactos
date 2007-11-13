/*
 * Copyright 2005 Eric Anholt
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <anholt@FreeBSD.org>
 *
 */

#include "sis_context.h"
#include "sis_state.h"
#include "sis_lock.h"
#include "sis_reg.h"

#include "swrast/swrast.h"
#include "macros.h"

static void sis_clear_front_buffer(GLcontext *ctx, GLenum mask, GLint x,
				   GLint y, GLint width, GLint height);
static void sis_clear_back_buffer(GLcontext *ctx, GLenum mask, GLint x,
				  GLint y, GLint width, GLint height);
static void sis_clear_z_buffer(GLcontext * ctx, GLbitfield mask, GLint x,
			       GLint y, GLint width, GLint height );

static void
set_color_pattern( sisContextPtr smesa, GLubyte red, GLubyte green,
		   GLubyte blue, GLubyte alpha )
{
   /* XXX only RGB565 and ARGB8888 */
   switch (smesa->colorFormat)
   {
   case DST_FORMAT_ARGB_8888:
      smesa->clearColorPattern = (alpha << 24) +
	 (red << 16) + (green << 8) + (blue);
      break;
   case DST_FORMAT_RGB_565:
      smesa->clearColorPattern = ((red >> 3) << 11) +
	 ((green >> 2) << 5) + (blue >> 3);
      smesa->clearColorPattern |= smesa->clearColorPattern << 16;
      break;
   default:
      sis_fatal_error("Bad dst color format\n");
   }
}

void
sis6326UpdateZPattern(sisContextPtr smesa, GLclampd z)
{
   CLAMPED_FLOAT_TO_USHORT(smesa->clearZStencilPattern, z * 65535.0);
}

void
sis6326DDClear(GLcontext *ctx, GLbitfield mask)
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   GLint x1, y1, width1, height1;

   /* get region after locking: */
   x1 = ctx->DrawBuffer->_Xmin;
   y1 = ctx->DrawBuffer->_Ymin;
   width1 = ctx->DrawBuffer->_Xmax - x1;
   height1 = ctx->DrawBuffer->_Ymax - y1;
   y1 = Y_FLIP(y1 + height1 - 1);

   /* XXX: Scissoring */
   
   fprintf(stderr, "Clear\n");

   /* Mask out any non-existent buffers */
   if (smesa->depth.offset == 0 || !ctx->Depth.Mask)
      mask &= ~BUFFER_BIT_DEPTH;

   LOCK_HARDWARE();

   if (mask & BUFFER_BIT_FRONT_LEFT) {
      sis_clear_front_buffer(ctx, mask, x1, y1, width1, height1);
      mask &= ~BUFFER_BIT_FRONT_LEFT;
   }

   if (mask & BUFFER_BIT_BACK_LEFT) {
      sis_clear_back_buffer(ctx, mask, x1, y1, width1, height1);
      mask &= ~BUFFER_BIT_BACK_LEFT;
   }

   if (mask & BUFFER_BIT_DEPTH) {
      sis_clear_z_buffer(ctx, mask, x1, y1, width1, height1);
      mask &= ~BUFFER_BIT_DEPTH;
   }

   UNLOCK_HARDWARE();

   if (mask != 0)
      _swrast_Clear(ctx, mask);
}


void
sis6326DDClearColor(GLcontext *ctx, const GLfloat color[4])
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   GLubyte c[4];

   CLAMPED_FLOAT_TO_UBYTE(c[0], color[0]);
   CLAMPED_FLOAT_TO_UBYTE(c[1], color[1]);
   CLAMPED_FLOAT_TO_UBYTE(c[2], color[2]);
   CLAMPED_FLOAT_TO_UBYTE(c[3], color[3]);

   set_color_pattern( smesa, c[0], c[1], c[2], c[3] );
}

void
sis6326DDClearDepth(GLcontext *ctx, GLclampd d)
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   sis6326UpdateZPattern(smesa, d);
}

static void
sis_clear_back_buffer(GLcontext *ctx, GLenum mask, GLint x, GLint y,
		      GLint width, GLint height)
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   
   /* XXX: The order of writing these registers seems to matter, while
    * it actually shouldn't.
    */
   mWait3DCmdQueue(6);
   MMIO(REG_6326_BitBlt_DstSrcPitch, smesa->back.pitch << 16);
   MMIO(REG_6326_BitBlt_fgColor, SiS_ROP_PATCOPY |
	smesa->clearColorPattern);
   MMIO(REG_6326_BitBlt_bgColor, SiS_ROP_PATCOPY |
	smesa->clearColorPattern);
   MMIO(REG_6326_BitBlt_DstAddr, smesa->back.offset +
	(y+height) * smesa->back.pitch +
	(x+width) * smesa->bytesPerPixel);
   MMIO(REG_6326_BitBlt_HeightWidth, ((height-1) << 16) |
	(width * smesa->bytesPerPixel));
   MMIO_WMB();
   MMIO(REG_6326_BitBlt_Cmd, BLT_PAT_BG);
}

static void
sis_clear_front_buffer(GLcontext *ctx, GLenum mask, GLint x, GLint y,
		       GLint width, GLint height)
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   int count;
   drm_clip_rect_t *pExtents = NULL;
   
   pExtents = smesa->driDrawable->pClipRects;
   count = smesa->driDrawable->numClipRects;

   mWait3DCmdQueue(3);
   MMIO(REG_6326_BitBlt_DstSrcPitch, smesa->front.pitch << 16);
   MMIO(REG_6326_BitBlt_fgColor, SiS_ROP_PATCOPY |
       smesa->clearColorPattern);
   MMIO(REG_6326_BitBlt_bgColor, SiS_ROP_PATCOPY |
       smesa->clearColorPattern);

   while (count--) {
      GLint x1 = pExtents->x1 - smesa->driDrawable->x;
      GLint y1 = pExtents->y1 - smesa->driDrawable->y;
      GLint x2 = pExtents->x2 - smesa->driDrawable->x;
      GLint y2 = pExtents->y2 - smesa->driDrawable->y;

      if (x > x1)
	 x1 = x;
      if (y > y1)
	 y1 = y;

      if (x + width < x2)
	 x2 = x + width;
      if (y + height < y2)
	 y2 = y + height;
      width = x2 - x1;
      height = y2 - y1;

      pExtents++;

      if (width <= 0 || height <= 0)
	 continue;

      mWait3DCmdQueue(3);
      MMIO(REG_6326_BitBlt_DstAddr, smesa->front.offset +
	   (y2-1) * smesa->front.pitch + x2 * smesa->bytesPerPixel);
      MMIO(REG_6326_BitBlt_HeightWidth, ((height-1) << 16) |
	   (width * smesa->bytesPerPixel));
      MMIO_WMB();
      MMIO(REG_6326_BitBlt_Cmd, BLT_PAT_BG);
   }
}

static void
sis_clear_z_buffer(GLcontext * ctx, GLbitfield mask, GLint x, GLint y,
		   GLint width, GLint height)
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   mWait3DCmdQueue(6);
   MMIO(REG_6326_BitBlt_DstAddr,
	smesa->depth.offset + y * smesa->depth.pitch + x * 2);
   MMIO(REG_6326_BitBlt_DstSrcPitch, smesa->depth.pitch << 16);
   MMIO(REG_6326_BitBlt_HeightWidth, ((height-1) << 16) | (width * 2));
   MMIO(REG_6326_BitBlt_fgColor, SiS_ROP_PATCOPY | smesa->clearZStencilPattern);
   MMIO(REG_6326_BitBlt_bgColor, SiS_ROP_PATCOPY | smesa->clearZStencilPattern);
   MMIO_WMB();
   MMIO(REG_6326_BitBlt_Cmd, BLT_PAT_BG | BLT_XINC | BLT_YINC);
}

