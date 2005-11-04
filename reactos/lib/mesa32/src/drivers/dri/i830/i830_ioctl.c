
/**************************************************************************

Copyright 2001 VA Linux Systems Inc., Fremont, California.

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
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/* $XFree86: xc/lib/GL/mesa/src/drv/i830/i830_ioctl.c,v 1.5 2002/12/10 01:26:53 dawes Exp $ */

/*
 * Author:
 *   Jeff Hartmann <jhartmann@2d3d.com>
 *   Graeme Fisher <graeme@2d3d.co.za>
 *   Abraham vd Merwe <abraham@2d3d.co.za>
 *
 * Heavily based on the I810 driver, which was written by:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "glheader.h"
#include "mtypes.h"
#include "macros.h"
#include "dd.h"
#include "swrast/swrast.h"

#include "mm.h"

#include "i830_screen.h"
#include "i830_dri.h"

#include "i830_context.h"
#include "i830_ioctl.h"
#include "i830_state.h"
#include "i830_debug.h"

#include "drm.h"

static drmBufPtr i830_get_buffer_ioctl( i830ContextPtr imesa )
{
   drmI830DMA dma;
   drmBufPtr buf;
   int retcode,i = 0;
   while (1) {
      retcode = drmCommandWriteRead(imesa->driFd, 
				    DRM_I830_GETBUF, 
				    &dma, 
				    sizeof(drmI830DMA));
      if (dma.granted == 1 && retcode == 0)
	break;

      if (++i > 1000) {
	 imesa->sarea->perf_boxes |= I830_BOX_WAIT;
	 retcode = drmCommandNone(imesa->driFd, DRM_I830_FLUSH);
	 i = 0;
      }
   }

   buf = &(imesa->i830Screen->bufs->list[dma.request_idx]);
   buf->idx = dma.request_idx;
   buf->used = 0;
   buf->total = dma.request_size;
   buf->address = (drmAddress)dma.virtual;

   return buf;
}

static void i830ClearDrawQuad(i830ContextPtr imesa, float left, 
				 float right,
				 float bottom, float top, GLubyte red,
				 GLubyte green, GLubyte blue, GLubyte alpha)
{
    GLuint *vb = i830AllocDmaLowLocked( imesa, 128 );
    i830Vertex tmp;
    int i;

    /* PRIM3D_TRIFAN */

    /* initial vertex, left bottom */
    tmp.v.x = left;
    tmp.v.y = bottom;
    tmp.v.z = 1.0;
    tmp.v.w = 1.0;
    tmp.v.color.red = red;
    tmp.v.color.green = green;
    tmp.v.color.blue = blue;
    tmp.v.color.alpha = alpha;
    tmp.v.specular.red = 0;
    tmp.v.specular.green = 0;
    tmp.v.specular.blue = 0;
    tmp.v.specular.alpha = 0;
    tmp.v.u0 = 0.0f;
    tmp.v.v0 = 0.0f;
    for (i = 0 ; i < 8 ; i++)
        vb[i] = tmp.ui[i];

    /* right bottom */
    vb += 8;
    tmp.v.x = right;
    for (i = 0 ; i < 8 ; i++)
        vb[i] = tmp.ui[i];

    /* right top */
    vb += 8;
    tmp.v.y = top;
    for (i = 0 ; i < 8 ; i++)
        vb[i] = tmp.ui[i];

    /* left top */
    vb += 8;
    tmp.v.x = left;
    for (i = 0 ; i < 8 ; i++)
        vb[i] = tmp.ui[i];
}

static void i830ClearWithTris(GLcontext *ctx, GLbitfield mask,
				 GLboolean all,
				 GLint cx, GLint cy, GLint cw, GLint ch)
{
   i830ContextPtr imesa = I830_CONTEXT( ctx );
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;
   i830ScreenPrivate *i830Screen = imesa->i830Screen;
   I830SAREAPtr sarea = imesa->sarea;
   GLuint old_vertex_prim;
   GLuint old_dirty;
   int x0, y0, x1, y1;

   if (I830_DEBUG & DEBUG_IOCTL) 
     fprintf(stderr, "Clearing with triangles\n");

   old_dirty = imesa->dirty & ~I830_UPLOAD_CLIPRECTS;
   /* Discard all the dirty flags except the cliprect one, reset later */
   imesa->dirty &= I830_UPLOAD_CLIPRECTS;

   if(!all) {
      x0 = cx;
      y0 = cy;
      x1 = x0 + cw;
      y1 = y0 + ch;
   } else {
      x0 = 0;
      y0 = 0;
      x1 = x0 + dPriv->w;
      y1 = y0 + dPriv->h;
   }

   /* Clip to Screen */
   if (x0 < 0) x0 = 0;
   if (y0 < 0) y0 = 0;
   if (x1 > i830Screen->width-1) x1 = i830Screen->width-1;
   if (y1 > i830Screen->height-1) y1 = i830Screen->height-1;

   LOCK_HARDWARE(imesa);
   memcpy(sarea->ContextState,
	  imesa->Init_Setup,
	  sizeof(imesa->Setup) );
   memcpy(sarea->BufferState,
	  imesa->BufferSetup,
	  sizeof(imesa->BufferSetup) );
   sarea->StippleState[I830_STPREG_ST1] = 0;

   old_vertex_prim = imesa->hw_primitive;
   imesa->hw_primitive = PRIM3D_TRIFAN;

   if(mask & BUFFER_BIT_FRONT_LEFT) {
      GLuint tmp = sarea->ContextState[I830_CTXREG_ENABLES_2];

      sarea->dirty |= (I830_UPLOAD_CTX | I830_UPLOAD_BUFFERS |
		       I830_UPLOAD_TEXBLEND0);

      sarea->TexBlendState[0][0] = (STATE3D_MAP_BLEND_OP_CMD(0) |
				    TEXPIPE_COLOR |
				    ENABLE_TEXOUTPUT_WRT_SEL |
				    TEXOP_OUTPUT_CURRENT |
				    DISABLE_TEX_CNTRL_STAGE |
				    TEXOP_SCALE_1X |
				    TEXOP_MODIFY_PARMS |
				    TEXOP_LAST_STAGE |
				    TEXBLENDOP_ARG1);
      sarea->TexBlendState[0][1] = (STATE3D_MAP_BLEND_OP_CMD(0) |
				    TEXPIPE_ALPHA |
				    ENABLE_TEXOUTPUT_WRT_SEL |
				    TEXOP_OUTPUT_CURRENT |
				    TEXOP_SCALE_1X |
				    TEXOP_MODIFY_PARMS |
				    TEXBLENDOP_ARG1);
      sarea->TexBlendState[0][2] = (STATE3D_MAP_BLEND_ARG_CMD(0) |
				    TEXPIPE_COLOR |
				    TEXBLEND_ARG1 |
				    TEXBLENDARG_MODIFY_PARMS |
				    TEXBLENDARG_CURRENT);
      sarea->TexBlendState[0][3] = (STATE3D_MAP_BLEND_ARG_CMD(0) |
				    TEXPIPE_ALPHA |
				    TEXBLEND_ARG1 |
				    TEXBLENDARG_MODIFY_PARMS |
				    TEXBLENDARG_CURRENT);
      sarea->TexBlendStateWordsUsed[0] = 4;

      tmp &= ~(ENABLE_STENCIL_WRITE | ENABLE_DEPTH_WRITE);
      tmp |= (DISABLE_STENCIL_WRITE | 
	      DISABLE_DEPTH_WRITE |
	      (imesa->mask_red << WRITEMASK_RED_SHIFT) |
	      (imesa->mask_green << WRITEMASK_GREEN_SHIFT) |
	      (imesa->mask_blue << WRITEMASK_BLUE_SHIFT) |
	      (imesa->mask_alpha << WRITEMASK_ALPHA_SHIFT));
      sarea->ContextState[I830_CTXREG_ENABLES_2] = tmp;

      if(0)
	fprintf(stderr, "fcdq : r_mask(%d) g_mask(%d) b_mask(%d) a_mask(%d)\n",
		imesa->mask_red, imesa->mask_green, imesa->mask_blue,
		imesa->mask_alpha);

      sarea->BufferState[I830_DESTREG_CBUFADDR] = i830Screen->fbOffset;

      if(0)
	fprintf(stderr, "fcdq : x0(%d) x1(%d) y0(%d) y1(%d)\n"
		"r(0x%x) g(0x%x) b(0x%x) a(0x%x)\n",
		x0, x1, y0, y1, imesa->clear_red, imesa->clear_green,
		imesa->clear_blue, imesa->clear_alpha);

      i830ClearDrawQuad(imesa, (float)x0, (float)x1, (float)y0, (float)y1,
                        imesa->clear_red, imesa->clear_green,
                        imesa->clear_blue, imesa->clear_alpha);
      i830FlushPrimsLocked( imesa );
   }

   if(mask & BUFFER_BIT_BACK_LEFT) {
      GLuint tmp = sarea->ContextState[I830_CTXREG_ENABLES_2];

      sarea->dirty |= (I830_UPLOAD_CTX | I830_UPLOAD_BUFFERS |
		       I830_UPLOAD_TEXBLEND0);

      sarea->TexBlendState[0][0] = (STATE3D_MAP_BLEND_OP_CMD(0) |
				    TEXPIPE_COLOR |
				    ENABLE_TEXOUTPUT_WRT_SEL |
				    TEXOP_OUTPUT_CURRENT |
				    DISABLE_TEX_CNTRL_STAGE |
				    TEXOP_SCALE_1X |
				    TEXOP_MODIFY_PARMS |
				    TEXOP_LAST_STAGE |
				    TEXBLENDOP_ARG1);
      sarea->TexBlendState[0][1] = (STATE3D_MAP_BLEND_OP_CMD(0) |
				    TEXPIPE_ALPHA |
				    ENABLE_TEXOUTPUT_WRT_SEL |
				    TEXOP_OUTPUT_CURRENT |
				    TEXOP_SCALE_1X |
				    TEXOP_MODIFY_PARMS |
				    TEXBLENDOP_ARG1);
      sarea->TexBlendState[0][2] = (STATE3D_MAP_BLEND_ARG_CMD(0) |
				    TEXPIPE_COLOR |
				    TEXBLEND_ARG1 |
				    TEXBLENDARG_MODIFY_PARMS |
				    TEXBLENDARG_CURRENT);
      sarea->TexBlendState[0][3] = (STATE3D_MAP_BLEND_ARG_CMD(0) |
				    TEXPIPE_ALPHA |
				    TEXBLEND_ARG2 |
				    TEXBLENDARG_MODIFY_PARMS |
				    TEXBLENDARG_CURRENT);
      sarea->TexBlendStateWordsUsed[0] = 4;

      tmp &= ~(ENABLE_STENCIL_WRITE | ENABLE_DEPTH_WRITE);
      tmp |= (DISABLE_STENCIL_WRITE | 
	      DISABLE_DEPTH_WRITE |
	      (imesa->mask_red << WRITEMASK_RED_SHIFT) |
	      (imesa->mask_green << WRITEMASK_GREEN_SHIFT) |
	      (imesa->mask_blue << WRITEMASK_BLUE_SHIFT) |
	      (imesa->mask_alpha << WRITEMASK_ALPHA_SHIFT));

      if(0)
	fprintf(stderr, "bcdq : r_mask(%d) g_mask(%d) b_mask(%d) a_mask(%d)\n",
		imesa->mask_red, imesa->mask_green, imesa->mask_blue,
		imesa->mask_alpha);

      sarea->ContextState[I830_CTXREG_ENABLES_2] = tmp;

      sarea->BufferState[I830_DESTREG_CBUFADDR] = i830Screen->backOffset;

      if(0)
	fprintf(stderr, "bcdq : x0(%d) x1(%d) y0(%d) y1(%d)\n"
		"r(0x%x) g(0x%x) b(0x%x) a(0x%x)\n",
		x0, x1, y0, y1, imesa->clear_red, imesa->clear_green,
		imesa->clear_blue, imesa->clear_alpha);

      i830ClearDrawQuad(imesa, (float)x0, (float)x1, (float)y0, (float)y1,
		      imesa->clear_red, imesa->clear_green,
		      imesa->clear_blue, imesa->clear_alpha);
      i830FlushPrimsLocked( imesa );
   }

   if(mask & BUFFER_BIT_STENCIL) {
      GLuint s_mask = ctx->Stencil.WriteMask[0];

      sarea->dirty |= (I830_UPLOAD_CTX | I830_UPLOAD_BUFFERS |
		       I830_UPLOAD_TEXBLEND0);

      sarea->TexBlendState[0][0] = (STATE3D_MAP_BLEND_OP_CMD(0) |
				    TEXPIPE_COLOR |
				    ENABLE_TEXOUTPUT_WRT_SEL |
				    TEXOP_OUTPUT_CURRENT |
				    DISABLE_TEX_CNTRL_STAGE |
				    TEXOP_SCALE_1X |
				    TEXOP_MODIFY_PARMS |
				    TEXOP_LAST_STAGE |
				    TEXBLENDOP_ARG1);
      sarea->TexBlendState[0][1] = (STATE3D_MAP_BLEND_OP_CMD(0) |
				    TEXPIPE_ALPHA |
				    ENABLE_TEXOUTPUT_WRT_SEL |
				    TEXOP_OUTPUT_CURRENT |
				    TEXOP_SCALE_1X |
				    TEXOP_MODIFY_PARMS |
				    TEXBLENDOP_ARG1);
      sarea->TexBlendState[0][2] = (STATE3D_MAP_BLEND_ARG_CMD(0) |
				    TEXPIPE_COLOR |
				    TEXBLEND_ARG1 |
				    TEXBLENDARG_MODIFY_PARMS |
				    TEXBLENDARG_CURRENT);
      sarea->TexBlendState[0][3] = (STATE3D_MAP_BLEND_ARG_CMD(0) |
				    TEXPIPE_ALPHA |
				    TEXBLEND_ARG2 |
				    TEXBLENDARG_MODIFY_PARMS |
				    TEXBLENDARG_CURRENT);
      sarea->TexBlendStateWordsUsed[0] = 4;

      sarea->ContextState[I830_CTXREG_ENABLES_1] |= (ENABLE_STENCIL_TEST |
						     ENABLE_DEPTH_TEST);

      sarea->ContextState[I830_CTXREG_ENABLES_2] &= ~(ENABLE_STENCIL_WRITE |
						     ENABLE_DEPTH_WRITE |
						     ENABLE_COLOR_WRITE);

      sarea->ContextState[I830_CTXREG_ENABLES_2] |= 
	 (ENABLE_STENCIL_WRITE |
	  DISABLE_DEPTH_WRITE |
	  (1 << WRITEMASK_RED_SHIFT) |
	  (1 << WRITEMASK_GREEN_SHIFT) |
	  (1 << WRITEMASK_BLUE_SHIFT) |
	  (1 << WRITEMASK_ALPHA_SHIFT) |
	  ENABLE_COLOR_WRITE);

      sarea->ContextState[I830_CTXREG_STATE4] &= 
	 ~MODE4_ENABLE_STENCIL_WRITE_MASK;

      sarea->ContextState[I830_CTXREG_STATE4] |= 
	 (ENABLE_STENCIL_WRITE_MASK |
	  STENCIL_WRITE_MASK(s_mask));

      sarea->ContextState[I830_CTXREG_STENCILTST] &= 
	 ~(STENCIL_OPS_MASK |
	   STENCIL_REF_VALUE_MASK |
	   ENABLE_STENCIL_TEST_FUNC_MASK);

      sarea->ContextState[I830_CTXREG_STENCILTST] |= 
	 (ENABLE_STENCIL_PARMS |
	  ENABLE_STENCIL_REF_VALUE |
	  ENABLE_STENCIL_TEST_FUNC |
	  STENCIL_FAIL_OP(STENCILOP_REPLACE) |
	  STENCIL_PASS_DEPTH_FAIL_OP(STENCILOP_REPLACE) |
	  STENCIL_PASS_DEPTH_PASS_OP(STENCILOP_REPLACE) |
	  STENCIL_REF_VALUE((ctx->Stencil.Clear & 0xff)) |
	  STENCIL_TEST_FUNC(COMPAREFUNC_ALWAYS));

      if(0) 
	fprintf(stderr, "Enables_1 (0x%x) Enables_2 (0x%x) StenTst (0x%x)\n"
		"Modes_4 (0x%x)\n",
		sarea->ContextState[I830_CTXREG_ENABLES_1],
		sarea->ContextState[I830_CTXREG_ENABLES_2],
		sarea->ContextState[I830_CTXREG_STENCILTST],
		sarea->ContextState[I830_CTXREG_STATE4]);

      sarea->BufferState[I830_DESTREG_CBUFADDR] = i830Screen->fbOffset;
      
      i830ClearDrawQuad(imesa, (float)x0, (float)x1, (float)y0, (float)y1,
			   255, 255, 255, 255);
      i830FlushPrimsLocked( imesa );
   }

   UNLOCK_HARDWARE(imesa);
   imesa->dirty = old_dirty;
   imesa->dirty |= (I830_UPLOAD_CTX |
		    I830_UPLOAD_BUFFERS |
		    I830_UPLOAD_TEXBLEND0);

   imesa->hw_primitive = old_vertex_prim;
}

static void i830Clear(GLcontext *ctx, GLbitfield mask, GLboolean all,
		      GLint cx1, GLint cy1, GLint cw, GLint ch)
{
   i830ContextPtr imesa = I830_CONTEXT( ctx );
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;
   const GLuint colorMask = *((GLuint *) &ctx->Color.ColorMask);
   drmI830Clear clear;
   GLbitfield tri_mask = 0;
   int i;
   GLint cx, cy;

   /* flip top to bottom */
   cy = dPriv->h-cy1-ch;
   cx = cx1 + imesa->drawX;
   cy += imesa->drawY;

   if(0) fprintf(stderr, "\nClearColor : 0x%08x\n", imesa->ClearColor);
   
   clear.flags = 0;
   clear.clear_color = imesa->ClearColor;
   clear.clear_depth = 0;
   clear.clear_colormask = 0;
   clear.clear_depthmask = 0;

   I830_FIREVERTICES( imesa );

   if (mask & BUFFER_BIT_FRONT_LEFT) {
      if(colorMask == ~0) {
	 clear.flags |= I830_FRONT;
      } else {
	 tri_mask |= BUFFER_BIT_FRONT_LEFT;
      }
      mask &= ~BUFFER_BIT_FRONT_LEFT;
   }

   if (mask & BUFFER_BIT_BACK_LEFT) {
      if(colorMask == ~0) {
	 clear.flags |= I830_BACK;
      } else {
	 tri_mask |= BUFFER_BIT_BACK_LEFT;
      }
      mask &= ~BUFFER_BIT_BACK_LEFT;
   }

   if (mask & BUFFER_BIT_DEPTH) {
      clear.flags |= I830_DEPTH;
      clear.clear_depthmask = imesa->depth_clear_mask;
      clear.clear_depth = (GLuint)(ctx->Depth.Clear * imesa->ClearDepth);
      mask &= ~BUFFER_BIT_DEPTH;
   }

   if((mask & BUFFER_BIT_STENCIL) && imesa->hw_stencil) {
      if (ctx->Stencil.WriteMask[0] != 0xff) {
	 tri_mask |= BUFFER_BIT_STENCIL;
      } else {
	 clear.flags |= I830_DEPTH;
	 clear.clear_depthmask |= imesa->stencil_clear_mask;
	 clear.clear_depth |= (ctx->Stencil.Clear & 0xff) << 24;
      }
      mask &= ~BUFFER_BIT_STENCIL;
   }

   /* First check for clears that need to happen with triangles */
   if(tri_mask) {
      i830ClearWithTris(ctx, tri_mask, all, cx, cy, cw, ch);
   }

   if (clear.flags) {
      LOCK_HARDWARE( imesa );

      for (i = 0 ; i < imesa->numClipRects ; ) 
      { 	 
	 int nr = MIN2(i + I830_NR_SAREA_CLIPRECTS, imesa->numClipRects);
	 drm_clip_rect_t *box = imesa->pClipRects;	 
	 drm_clip_rect_t *b = (drm_clip_rect_t *)imesa->sarea->boxes;
	 int n = 0;

	 if (!all) {
	    for ( ; i < nr ; i++) {
	       GLint x = box[i].x1;
	       GLint y = box[i].y1;
	       GLint w = box[i].x2 - x;
	       GLint h = box[i].y2 - y;

	       if (x < cx) w -= cx - x, x = cx; 
	       if (y < cy) h -= cy - y, y = cy;
	       if (x + w > cx + cw) w = cx + cw - x;
	       if (y + h > cy + ch) h = cy + ch - y;
	       if (w <= 0) continue;
	       if (h <= 0) continue;

	       b->x1 = x;
	       b->y1 = y;
	       b->x2 = x + w;
	       b->y2 = y + h;
	       b++;
	       n++;
	    }
	 } else {
	    for ( ; i < nr ; i++) {
	       *b++ = *(drm_clip_rect_t *)&box[i];
	       n++;
	    }
	 }

	 imesa->sarea->nbox = n;
	 drmCommandWrite(imesa->driFd, DRM_I830_CLEAR,
			 &clear, sizeof(drmI830Clear));
      }

      UNLOCK_HARDWARE( imesa );
      imesa->upload_cliprects = GL_TRUE;
   }

   if (mask)
      _swrast_Clear( ctx, mask, all, cx1, cy1, cw, ch );
}



/*
 * Copy the back buffer to the front buffer. 
 */
void i830CopyBuffer( const __DRIdrawablePrivate *dPriv ) 
{
   i830ContextPtr imesa;
   drm_clip_rect_t *pbox;
   int nbox, i, tmp;

   assert(dPriv);
   assert(dPriv->driContextPriv);
   assert(dPriv->driContextPriv->driverPrivate);

   imesa = (i830ContextPtr) dPriv->driContextPriv->driverPrivate;

   I830_FIREVERTICES( imesa );
   LOCK_HARDWARE( imesa );

   imesa->sarea->perf_boxes |= imesa->perf_boxes;
   imesa->perf_boxes = 0;

   pbox = dPriv->pClipRects;
   nbox = dPriv->numClipRects;

   for (i = 0 ; i < nbox ; )
   {
      int nr = MIN2(i + I830_NR_SAREA_CLIPRECTS, dPriv->numClipRects);
      drm_clip_rect_t *b = (drm_clip_rect_t *)imesa->sarea->boxes;

      imesa->sarea->nbox = nr - i;

      for ( ; i < nr ; i++) 
	 *b++ = pbox[i];
      drmCommandNone(imesa->driFd, DRM_I830_SWAP);
   }

   tmp = GET_ENQUEUE_AGE(imesa);
   UNLOCK_HARDWARE( imesa );

   /* multiarb will suck the life out of the server without this throttle:
    */
   if (GET_DISPATCH_AGE(imesa) < imesa->lastSwap) {
      i830WaitAge(imesa, imesa->lastSwap);
   }

   imesa->lastSwap = tmp;
   imesa->upload_cliprects = GL_TRUE;
}

/* Flip the front & back buffes
 */
void i830PageFlip( const __DRIdrawablePrivate *dPriv )
{
#if 0
   i830ContextPtr imesa;
   int tmp, ret;

   if (I830_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   assert(dPriv);
   assert(dPriv->driContextPriv);
   assert(dPriv->driContextPriv->driverPrivate);

   imesa = (i830ContextPtr) dPriv->driContextPriv->driverPrivate;

   I830_FIREVERTICES( imesa );
   LOCK_HARDWARE( imesa );

   imesa->sarea->perf_boxes |= imesa->perf_boxes;
   imesa->perf_boxes = 0;

   if (dPriv->pClipRects) {
      *(drm_clip_rect_t *)imesa->sarea->boxes = dPriv->pClipRects[0];
      imesa->sarea->nbox = 1;
   }

   ret = drmCommandNone(imesa->driFd, DRM_I830_FLIP); 
   if (ret) {
      fprintf(stderr, "%s: %d\n", __FUNCTION__, ret);
      UNLOCK_HARDWARE( imesa );
      exit(1);
   }

   tmp = GET_ENQUEUE_AGE(imesa);
   UNLOCK_HARDWARE( imesa );

   /* multiarb will suck the life out of the server without this throttle:
    */
   if (GET_DISPATCH_AGE(imesa) < imesa->lastSwap) {
      i830WaitAge(imesa, imesa->lastSwap);
   }

   i830SetDrawBuffer( imesa->glCtx, imesa->glCtx->Color.DriverDrawBuffer );
   imesa->upload_cliprects = GL_TRUE;
   imesa->lastSwap = tmp;
#endif
}

/* This waits for *everybody* to finish rendering -- overkill.
 */
void i830DmaFinish( i830ContextPtr imesa  )
{
   I830_FIREVERTICES( imesa );
   LOCK_HARDWARE_QUIESCENT( imesa );
   UNLOCK_HARDWARE( imesa );
}

void i830RegetLockQuiescent( i830ContextPtr imesa )
{
   drmUnlock(imesa->driFd, imesa->hHWContext);
   i830GetLock( imesa, DRM_LOCK_QUIESCENT );
}

void i830WaitAgeLocked( i830ContextPtr imesa, int age )
{
   int i = 0;
   while (++i < 5000) {
      drmCommandNone(imesa->driFd, DRM_I830_GETAGE);
      if (GET_DISPATCH_AGE(imesa) >= age) return;
      imesa->sarea->perf_boxes |= I830_BOX_WAIT;
      UNLOCK_HARDWARE( imesa );
      if (I830_DEBUG & DEBUG_SLEEP) fprintf(stderr, ".");
      usleep(1);
      LOCK_HARDWARE( imesa );
   }
   /* If that didn't work, just do a flush:
    */
   drmCommandNone(imesa->driFd, DRM_I830_FLUSH); 
}

void i830WaitAge( i830ContextPtr imesa, int age  ) 
{
   int i = 0;
   if (GET_DISPATCH_AGE(imesa) >= age) return;

   while (1) {
      drmCommandNone(imesa->driFd, DRM_I830_GETAGE);
      if (GET_DISPATCH_AGE(imesa) >= age) return;
      imesa->perf_boxes |= I830_BOX_WAIT;

      if (imesa->do_irqs) {
	 drmI830IrqEmit ie;
	 drmI830IrqWait iw;
	 int ret;
      
	 ie.irq_seq = &iw.irq_seq;
	 
	 LOCK_HARDWARE( imesa ); 
	 ret = drmCommandWriteRead( imesa->driFd, DRM_I830_IRQ_EMIT, &ie, sizeof(ie) );
	 if ( ret ) {
	    fprintf( stderr, "%s: drmI830IrqEmit: %d\n", __FUNCTION__, ret );
	    exit(1);
	 }
	 UNLOCK_HARDWARE(imesa);
	 
	 ret = drmCommandWrite( imesa->driFd, DRM_I830_IRQ_WAIT, &iw, sizeof(iw) );
	 if ( ret ) {
	    fprintf( stderr, "%s: drmI830IrqWait: %d\n", __FUNCTION__, ret );
	    exit(1);
	 }
      } else {
	 if (++i > 5000) usleep(1); 
      }
   }
}

static void age_imesa( i830ContextPtr imesa, int age )
{
   if (imesa->CurrentTexObj[0]) imesa->CurrentTexObj[0]->base.timestamp = age;
   if (imesa->CurrentTexObj[1]) imesa->CurrentTexObj[1]->base.timestamp = age;
}

void i830FlushPrimsLocked( i830ContextPtr imesa )
{
   drm_clip_rect_t *pbox = imesa->pClipRects;
   int nbox = imesa->numClipRects;
   drmBufPtr buffer = imesa->vertex_buffer;
   I830SAREAPtr sarea = imesa->sarea;
   drmI830Vertex vertex;
   int i, nr;

   if (I830_DEBUG & DEBUG_IOCTL) 
      fprintf(stderr, "%s dirty: %08x\n", __FUNCTION__, imesa->dirty);


   vertex.idx = buffer->idx;
   vertex.used = imesa->vertex_low;
   vertex.discard = 0;
   sarea->vertex_prim = imesa->hw_primitive;

   /* Reset imesa vars:
    */
   imesa->vertex_buffer = 0;
   imesa->vertex_addr = 0;
   imesa->vertex_low = 0;
   imesa->vertex_high = 0;
   imesa->vertex_last_prim = 0;

   if (imesa->dirty) {
      if (I830_DEBUG & DEBUG_SANITY)
	 i830EmitHwStateLockedDebug(imesa);
      else
	 i830EmitHwStateLocked(imesa);
   }

   if (I830_DEBUG & DEBUG_IOCTL)
      fprintf(stderr,"%s: Vertex idx %d used %d discard %d\n", 
	      __FUNCTION__, vertex.idx, vertex.used, vertex.discard);

   if (!nbox) {
      vertex.used = 0;
      vertex.discard = 1;
      if (drmCommandWrite (imesa->driFd, DRM_I830_VERTEX, 
			   &vertex, sizeof(drmI830Vertex))) {
	 fprintf(stderr, "DRM_I830_VERTEX: %d\n",  -errno);
	 UNLOCK_HARDWARE(imesa);
	 exit(1);
      }
      return;
   }

   for (i = 0 ; i < nbox ; i = nr ) {
      drm_clip_rect_t *b = sarea->boxes;
      int j;

      nr = MIN2(i + I830_NR_SAREA_CLIPRECTS, nbox);
      sarea->nbox = nr - i;

      for ( j = i ; j < nr ; j++) {
	 b[j-i] = pbox[j];
      }

      /* Finished with the buffer?
       */
      if (nr == nbox)
	 vertex.discard = 1;

      /* Do a bunch of sanity checks on the vertices sent to the hardware */
      if (I830_DEBUG & DEBUG_SANITY) {
	 i830VertexSanity(imesa, vertex);
      
	 for ( j = 0 ; j < sarea->nbox ; j++) {
	    fprintf(stderr, "box %d/%d %d,%d %d,%d\n",
		    j, sarea->nbox, b[j].x1, b[j].y1, b[j].x2, b[j].y2);
	 }
      }

      drmCommandWrite (imesa->driFd, DRM_I830_VERTEX, 
		       &vertex, sizeof(drmI830Vertex));
      age_imesa(imesa, imesa->sarea->last_enqueue);
   }

   imesa->dirty = 0;
   imesa->upload_cliprects = GL_FALSE;
}

void i830FlushPrimsGetBufferLocked( i830ContextPtr imesa )
{
   if (imesa->vertex_buffer)
     i830FlushPrimsLocked( imesa );
   imesa->vertex_buffer = i830_get_buffer_ioctl( imesa );
   imesa->vertex_addr = (char *)imesa->vertex_buffer->address;

   /* leave room for instruction header & footer:
    */
   imesa->vertex_high = imesa->vertex_buffer->total - 4; 
   imesa->vertex_low = 4;	
   imesa->vertex_last_prim = imesa->vertex_low;
}

void i830FlushPrimsGetBuffer( i830ContextPtr imesa )
{
   LOCK_HARDWARE(imesa);
   i830FlushPrimsGetBufferLocked( imesa );
   UNLOCK_HARDWARE(imesa);
}


void i830FlushPrims( i830ContextPtr imesa )
{
   if (imesa->vertex_buffer) {
      LOCK_HARDWARE( imesa );
      i830FlushPrimsLocked( imesa );
      UNLOCK_HARDWARE( imesa );
   }
}

int i830_check_copy(int fd)
{
   return drmCommandNone(fd, DRM_I830_DOCOPY);
}

static void i830Flush( GLcontext *ctx )
{
   i830ContextPtr imesa = I830_CONTEXT( ctx );
   I830_FIREVERTICES( imesa );
}

static void i830Finish( GLcontext *ctx  ) 
{
   i830ContextPtr imesa = I830_CONTEXT( ctx );
   i830DmaFinish( imesa );
}

void i830InitIoctlFuncs( struct dd_function_table *functions )
{
   functions->Flush = i830Flush;
   functions->Clear = i830Clear;
   functions->Finish = i830Finish;
}

