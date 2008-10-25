/**************************************************************************

Copyright 2000 Silicon Integrated Systems Corp, Inc., HsinChu, Taiwan.
Copyright 2003 Eric Anholt
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/
/* $XFree86: xc/lib/GL/mesa/src/drv/sis/sis_clear.c,v 1.5 2000/09/26 15:56:48 tsi Exp $ */

/*
 * Authors:
 *   Sung-Ching Lin <sclin@sis.com.tw>
 *   Eric Anholt <anholt@FreeBSD.org>
 */

#include "sis_context.h"
#include "sis_state.h"
#include "sis_lock.h"

#include "swrast/swrast.h"
#include "macros.h"

static GLbitfield sis_3D_Clear( GLcontext * ctx, GLbitfield mask,
				GLint x, GLint y, GLint width,
				GLint height );
static void sis_clear_color_buffer( GLcontext *ctx, GLenum mask, GLint x,
				    GLint y, GLint width, GLint height );
static void sis_clear_z_stencil_buffer( GLcontext * ctx,
					GLbitfield mask, GLint x,
					GLint y, GLint width,
					GLint height );

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
sisUpdateZStencilPattern( sisContextPtr smesa, GLclampd z, GLint stencil )
{
   GLuint zPattern;

   switch (smesa->zFormat)
   {
   case SiS_ZFORMAT_Z16:
      CLAMPED_FLOAT_TO_USHORT(zPattern, z);
      zPattern |= zPattern << 16;
      break;
   case SiS_ZFORMAT_S8Z24:
      zPattern = FLOAT_TO_UINT(z) >> 8;
      zPattern |= stencil << 24;
      break;
   case SiS_ZFORMAT_Z32:
      zPattern = FLOAT_TO_UINT(z);
      break;
   default:
      sis_fatal_error("Bad Z format\n");
   }
   smesa->clearZStencilPattern = zPattern;
}

void
sisDDClear( GLcontext * ctx, GLbitfield mask )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   GLint x1, y1, width1, height1;

   /* get region after locking: */
   x1 = ctx->DrawBuffer->_Xmin;
   y1 = ctx->DrawBuffer->_Ymin;
   width1 = ctx->DrawBuffer->_Xmax - x1;
   height1 = ctx->DrawBuffer->_Ymax - y1;
   y1 = Y_FLIP(y1 + height1 - 1);

   /* Mask out any non-existent buffers */
   if (ctx->Visual.depthBits == 0 || !ctx->Depth.Mask)
      mask &= ~BUFFER_BIT_DEPTH;
   if (ctx->Visual.stencilBits == 0)
      mask &= ~BUFFER_BIT_STENCIL;

   LOCK_HARDWARE();

   /* The 3d clear code is use for masked clears because apparently the SiS
    * 300-series can't do write masks for 2d blits.  3d isn't used in general
    * because it's slower, even in the case of clearing multiple buffers.
    */
   /* XXX: Appears to be broken with stencil. */
   if ((smesa->current.hwCapEnable2 & (MASK_AlphaMaskWriteEnable |
      MASK_ColorMaskWriteEnable) &&
      (mask & (BUFFER_BIT_BACK_LEFT | BUFFER_BIT_FRONT_LEFT)) != 0) ||
      ((ctx->Stencil.WriteMask[0] & 0xff) != 0xff && 
       (mask & BUFFER_BIT_STENCIL) != 0) )
   {
      mask = sis_3D_Clear( ctx, mask, x1, y1, width1, height1 );
   }

   if ( mask & BUFFER_BIT_FRONT_LEFT || mask & BUFFER_BIT_BACK_LEFT) {
      sis_clear_color_buffer( ctx, mask, x1, y1, width1, height1 );
      mask &= ~(BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_BACK_LEFT);
   }

   if (mask & (BUFFER_BIT_DEPTH | BUFFER_BIT_STENCIL)) {
      if (smesa->depth.offset != 0)
         sis_clear_z_stencil_buffer( ctx, mask, x1, y1, width1, height1 );
      mask &= ~(BUFFER_BIT_DEPTH | BUFFER_BIT_STENCIL);
   }

   UNLOCK_HARDWARE();

   if (mask != 0)
      _swrast_Clear( ctx, mask);
}


void
sisDDClearColor( GLcontext * ctx, const GLfloat color[4] )
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
sisDDClearDepth( GLcontext * ctx, GLclampd d )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   sisUpdateZStencilPattern( smesa, d, ctx->Stencil.Clear );
}

void
sisDDClearStencil( GLcontext * ctx, GLint s )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   sisUpdateZStencilPattern( smesa, ctx->Depth.Clear, s );
}

static GLbitfield
sis_3D_Clear( GLcontext * ctx, GLbitfield mask,
	      GLint x, GLint y, GLint width, GLint height )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   __GLSiSHardware *current = &smesa->current;

   float left, top, right, bottom, zClearVal;
   GLboolean bClrColor, bClrDepth, bClrStencil;
   GLint dwPrimitiveSet;
   GLint dwEnable1 = 0, dwEnable2 = MASK_ColorMaskWriteEnable;
   GLint dwDepthMask = 0, dwSten1 = 0, dwSten2 = 0;
   GLint dirtyflags = GFLAG_ENABLESETTING | GFLAG_ENABLESETTING2 |
      GFLAG_CLIPPING | GFLAG_DESTSETTING;
   int count;
   drm_clip_rect_t *pExtents;

   bClrColor = (mask & (BUFFER_BIT_BACK_LEFT | BUFFER_BIT_FRONT_LEFT)) != 0;
   bClrDepth = (mask & BUFFER_BIT_DEPTH) != 0;
   bClrStencil = (mask & BUFFER_BIT_STENCIL) != 0;

   if (smesa->GlobalFlag & GFLAG_RENDER_STATES)
      sis_update_render_state( smesa );

   if (bClrStencil) {
      dwSten1 = STENCIL_FORMAT_8 | SiS_STENCIL_ALWAYS |
         ((ctx->Stencil.Clear & 0xff) << 8) | 0xff;
      dwSten2 = SiS_SFAIL_REPLACE | SiS_SPASS_ZFAIL_REPLACE |
         SiS_SPASS_ZPASS_REPLACE;
      dwEnable1 = MASK_ZWriteEnable | MASK_StencilWriteEnable |
	MASK_StencilTestEnable;
      dwEnable2 |= MASK_ZMaskWriteEnable;
      dwDepthMask |= (ctx->Stencil.WriteMask[0] & 0xff) << 24;
   } else if (bClrDepth) {
      dwEnable1 = MASK_ZWriteEnable;
      dwEnable2 |= MASK_ZMaskWriteEnable;
   }

   if (bClrDepth) {
      zClearVal = ctx->Depth.Clear;
      if (ctx->Visual.depthBits != 32)
         dwDepthMask |= 0x00ffffff;
      else
         dwDepthMask = 0xffffffff;
   } else
      zClearVal = 0.0;

   mWait3DCmdQueue(9);
   MMIO(REG_3D_TEnable, dwEnable1);
   MMIO(REG_3D_TEnable2, dwEnable2);
   if (bClrDepth || bClrStencil) {
      MMIO(REG_3D_ZSet, (current->hwZ & ~MASK_ZTestMode) | SiS_Z_COMP_ALWAYS);
      dirtyflags |= GFLAG_ZSETTING;
   }
   if (bClrColor) {
      MMIO(REG_3D_DstSet, (current->hwDstSet & ~MASK_ROP2) | LOP_COPY);
   } else {
      MMIO(REG_3D_DstAlphaWriteMask, 0L);
   }
   if (bClrStencil) {
      MMIO(REG_3D_StencilSet, dwSten1);
      MMIO(REG_3D_StencilSet2, dwSten2);
      dirtyflags |= GFLAG_STENCILSETTING;
   }

   if (mask & BUFFER_BIT_FRONT_LEFT) {
      pExtents = smesa->driDrawable->pClipRects;
      count = smesa->driDrawable->numClipRects;
   } else {
      pExtents = NULL;
      count = 1;
   }

   while(count--) {
      left = x;
      right = x + width;
      top = y;
      bottom = y + height;

      if (pExtents != NULL) {
         GLuint x1, y1, x2, y2;

         x1 = pExtents->x1 - smesa->driDrawable->x;
         y1 = pExtents->y1 - smesa->driDrawable->y;
         x2 = pExtents->x2 - smesa->driDrawable->x - 1;
         y2 = pExtents->y2 - smesa->driDrawable->y - 1;

         left = (left > x1) ? left : x1;
         right = (right > x2) ? x2 : right;
         top = (top > y1) ? top : y1;
         bottom = (bottom > y2) ? y2 : bottom;
         pExtents++;
         if (left > right || top > bottom)
            continue;
      }

      mWait3DCmdQueue(20);

      MMIO(REG_3D_ClipTopBottom, ((GLint)top << 13) | (GLint)bottom);
      MMIO(REG_3D_ClipLeftRight, ((GLint)left << 13) | (GLint)right);

      /* the first triangle */
      dwPrimitiveSet = OP_3D_TRIANGLE_DRAW | OP_3D_FIRE_TSARGBc | 
                        SHADE_FLAT_VertexC;
      MMIO(REG_3D_PrimitiveSet, dwPrimitiveSet);

      MMIO(REG_3D_TSZa, *(GLint *) &zClearVal);
      MMIO(REG_3D_TSXa, *(GLint *) &right);
      MMIO(REG_3D_TSYa, *(GLint *) &top);
      MMIO(REG_3D_TSARGBa, smesa->clearColorPattern);

      MMIO(REG_3D_TSZb, *(GLint *) &zClearVal);
      MMIO(REG_3D_TSXb, *(GLint *) &left);
      MMIO(REG_3D_TSYb, *(GLint *) &top);
      MMIO(REG_3D_TSARGBb, smesa->clearColorPattern);

      MMIO(REG_3D_TSZc, *(GLint *) &zClearVal);
      MMIO(REG_3D_TSXc, *(GLint *) &left);
      MMIO(REG_3D_TSYc, *(GLint *) &bottom);
      MMIO(REG_3D_TSARGBc, smesa->clearColorPattern);

      /* second triangle */
      dwPrimitiveSet = OP_3D_TRIANGLE_DRAW | OP_3D_FIRE_TSARGBb |
                        SHADE_FLAT_VertexB;
      MMIO(REG_3D_PrimitiveSet, dwPrimitiveSet);

      MMIO(REG_3D_TSZb, *(GLint *) &zClearVal);
      MMIO(REG_3D_TSXb, *(GLint *) &right);
      MMIO(REG_3D_TSYb, *(GLint *) &bottom);
      MMIO(REG_3D_TSARGBb, smesa->clearColorPattern);
   }

   mEndPrimitive();

   /* If BUFFER_BIT_FRONT_LEFT is set, we've only cleared the front buffer so far */
   if ((mask & BUFFER_BIT_FRONT_LEFT) != 0 && (mask & BUFFER_BIT_BACK_LEFT) != 0)
      sis_3D_Clear( ctx, BUFFER_BIT_BACK_LEFT, x, y, width, height );

   smesa->GlobalFlag |= dirtyflags;

   return mask & ~(BUFFER_BIT_DEPTH | BUFFER_BIT_STENCIL | BUFFER_BIT_BACK_LEFT |
      BUFFER_BIT_FRONT_LEFT);
}

static void
sis_clear_color_buffer( GLcontext *ctx, GLenum mask, GLint x, GLint y,
			GLint width, GLint height )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   int count;
   drm_clip_rect_t *pExtents = NULL;
   GLint xx, yy;
   GLint x0, y0, width0, height0;

   /* Clear back buffer */
   if (mask & BUFFER_BIT_BACK_LEFT) {
      mWait3DCmdQueue (8);
      MMIO(REG_SRC_PITCH, (smesa->bytesPerPixel == 4) ? 
			   BLIT_DEPTH_32 : BLIT_DEPTH_16);
      MMIO(REG_DST_X_Y, (x << 16) | y);
      MMIO(REG_DST_ADDR, smesa->back.offset);
      MMIO(REG_DST_PITCH_HEIGHT, (smesa->virtualY << 16) | smesa->back.pitch);
      MMIO(REG_WIDTH_HEIGHT, (height << 16) | width);
      MMIO(REG_PATFG, smesa->clearColorPattern);
      MMIO(REG_BLIT_CMD, CMD_DIR_X_INC | CMD_DIR_Y_INC | CMD_ROP_PAT);
      MMIO(REG_CommandQueue, -1);
   }
  
   if ((mask & BUFFER_BIT_FRONT_LEFT) == 0)
      return;

   /* Clear front buffer */
   x0 = x;
   y0 = y;
   width0 = width;
   height0 = height;

   pExtents = smesa->driDrawable->pClipRects;
   count = smesa->driDrawable->numClipRects;

   while (count--) {
      GLint x2 = pExtents->x1 - smesa->driDrawable->x;
      GLint y2 = pExtents->y1 - smesa->driDrawable->y;
      GLint xx2 = pExtents->x2 - smesa->driDrawable->x;
      GLint yy2 = pExtents->y2 - smesa->driDrawable->y;

      x = (x0 > x2) ? x0 : x2;
      y = (y0 > y2) ? y0 : y2;
      xx = ((x0 + width0) > (xx2)) ? xx2 : x0 + width0;
      yy = ((y0 + height0) > (yy2)) ? yy2 : y0 + height0;
      width = xx - x;
      height = yy - y;
      pExtents++;

      if (width <= 0 || height <= 0)
	continue;

      mWait3DCmdQueue (8);
      MMIO(REG_SRC_PITCH, (smesa->bytesPerPixel == 4) ? 
			   BLIT_DEPTH_32 : BLIT_DEPTH_16);
      MMIO(REG_DST_X_Y, (x << 16) | y);
      MMIO(REG_DST_ADDR, smesa->front.offset);
      MMIO(REG_DST_PITCH_HEIGHT, (smesa->virtualY << 16) | smesa->front.pitch);
      MMIO(REG_WIDTH_HEIGHT, (height << 16) | width);
      MMIO(REG_PATFG, smesa->clearColorPattern);
      MMIO(REG_BLIT_CMD, CMD_DIR_X_INC | CMD_DIR_Y_INC | CMD_ROP_PAT);
      MMIO(REG_CommandQueue, -1);
   }
}

static void
sis_clear_z_stencil_buffer( GLcontext * ctx, GLbitfield mask,
			    GLint x, GLint y, GLint width, GLint height )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   int cmd;

   mWait3DCmdQueue (8);
   MMIO(REG_SRC_PITCH, (smesa->zFormat == SiS_ZFORMAT_Z16) ?
			BLIT_DEPTH_16 : BLIT_DEPTH_32);
   MMIO(REG_DST_X_Y, (x << 16) | y);
   MMIO(REG_DST_ADDR, smesa->depth.offset);
   MMIO(REG_DST_PITCH_HEIGHT, (smesa->virtualY << 16) | smesa->depth.pitch);
   MMIO(REG_WIDTH_HEIGHT, (height << 16) | width);
   MMIO(REG_PATFG, smesa->clearZStencilPattern);
   MMIO(REG_BLIT_CMD, CMD_DIR_X_INC | CMD_DIR_Y_INC | CMD_ROP_PAT);
   MMIO(REG_CommandQueue, -1);
}

