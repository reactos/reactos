/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <stdio.h>
#include <unistd.h>

#include "glheader.h"
#include "mtypes.h"
#include "macros.h"
#include "dd.h"
#include "swrast/swrast.h"

#include "mm.h"
#include "via_context.h"
#include "via_tris.h"
#include "via_ioctl.h"
#include "via_state.h"
#include "via_fb.h"
#include "via_3d_reg.h"

#include "vblank.h"
#include "drm.h"
#include "xf86drm.h"
#include <sys/ioctl.h>
#include <errno.h>


#define VIA_REG_STATUS          0x400
#define VIA_REG_GEMODE          0x004
#define VIA_REG_SRCBASE         0x030
#define VIA_REG_DSTBASE         0x034
#define VIA_REG_PITCH           0x038      
#define VIA_REG_SRCCOLORKEY     0x01C      
#define VIA_REG_KEYCONTROL      0x02C       
#define VIA_REG_SRCPOS          0x008
#define VIA_REG_DSTPOS          0x00C
#define VIA_REG_GECMD           0x000
#define VIA_REG_DIMENSION       0x010       /* width and height */
#define VIA_REG_FGCOLOR         0x018

#define VIA_GEM_8bpp            0x00000000
#define VIA_GEM_16bpp           0x00000100
#define VIA_GEM_32bpp           0x00000300
#define VIA_GEC_BLT             0x00000001
#define VIA_PITCH_ENABLE        0x80000000
#define VIA_GEC_INCX            0x00000000
#define VIA_GEC_DECY            0x00004000
#define VIA_GEC_INCY            0x00000000
#define VIA_GEC_DECX            0x00008000
#define VIA_GEC_FIXCOLOR_PAT    0x00002000


#define VIA_BLIT_CLEAR 0x00
#define VIA_BLIT_COPY 0xCC
#define VIA_BLIT_FILL 0xF0
#define VIA_BLIT_SET 0xFF

static void dump_dma( struct via_context *vmesa )
{
   GLuint i;
   GLuint *data = (GLuint *)vmesa->dma;
   for (i = 0; i < vmesa->dmaLow; i += 16) {
      fprintf(stderr, "%04x:   ", i);
      fprintf(stderr, "%08x  ", *data++);
      fprintf(stderr, "%08x  ", *data++);
      fprintf(stderr, "%08x  ", *data++);
      fprintf(stderr, "%08x\n", *data++);
   }
   fprintf(stderr, "******************************************\n");
}



void viaCheckDma(struct via_context *vmesa, GLuint bytes)
{
    VIA_FINISH_PRIM( vmesa );
    if (vmesa->dmaLow + bytes > VIA_DMA_HIGHWATER) {
	viaFlushDma(vmesa);
    }
}



#define SetReg2DAGP(nReg, nData) do {		\
    OUT_RING( ((nReg) >> 2) | 0xF0000000 );	\
    OUT_RING( nData );				\
} while (0)


static void viaBlit(struct via_context *vmesa, GLuint bpp,
		    GLuint srcBase, GLuint srcPitch, 
		    GLuint dstBase, GLuint dstPitch,
		    GLuint w, GLuint h, 
		    GLuint blitMode, 
		    GLuint color, GLuint nMask ) 
{

    GLuint dwGEMode, srcX, dstX, cmd;
    RING_VARS;

    if (VIA_DEBUG & DEBUG_2D)
       fprintf(stderr, 
	       "%s bpp %d src %x/%x dst %x/%x w %d h %d "
	       " mode: %x color: 0x%08x mask 0x%08x\n",
	       __FUNCTION__, bpp, srcBase, srcPitch, dstBase,
	       dstPitch, w,h, blitMode, color, nMask);


    if (!w || !h)
        return;

    switch (bpp) {
    case 16:
        dwGEMode = VIA_GEM_16bpp;
	srcX = (srcBase & 0x1f) >> 1;
	dstX = (dstBase & 0x1f) >> 1;
        break;
    case 32:
        dwGEMode = VIA_GEM_32bpp;
	srcX = (srcBase & 0x1f) >> 2;
	dstX = (dstBase & 0x1f) >> 2;
	break;
    default:
        return;
    }

    switch(blitMode) {
    case VIA_BLIT_FILL:
	cmd = VIA_GEC_BLT | VIA_GEC_FIXCOLOR_PAT | (VIA_BLIT_FILL << 24);
	break;
    case VIA_BLIT_COPY:
	cmd = VIA_GEC_BLT | (VIA_BLIT_COPY << 24);
	break;
    default:
        return;
    }	

    BEGIN_RING(22);
    SetReg2DAGP( VIA_REG_GEMODE, dwGEMode);
    SetReg2DAGP( VIA_REG_FGCOLOR, color);
    SetReg2DAGP( 0x2C, nMask);
    SetReg2DAGP( VIA_REG_SRCBASE, (srcBase & ~0x1f) >> 3);
    SetReg2DAGP( VIA_REG_DSTBASE, (dstBase & ~0x1f) >> 3);
    SetReg2DAGP( VIA_REG_PITCH, VIA_PITCH_ENABLE |
	       (srcPitch >> 3) | ((dstPitch >> 3) << 16));
    SetReg2DAGP( VIA_REG_SRCPOS, srcX);
    SetReg2DAGP( VIA_REG_DSTPOS, dstX);
    SetReg2DAGP( VIA_REG_DIMENSION, (((h - 1) << 16) | (w - 1)));
    SetReg2DAGP( VIA_REG_GECMD, cmd);
    SetReg2DAGP( 0x2C, 0x00000000);
    ADVANCE_RING();
}

static void viaFillBuffer(struct via_context *vmesa,
			  struct via_renderbuffer *buffer,
			  drm_clip_rect_t *pbox,
			  int nboxes,
			  GLuint pixel,
			  GLuint mask)
{
   GLuint bytePerPixel = buffer->bpp >> 3;
   GLuint i;

   for (i = 0; i < nboxes ; i++) {        
      int x = pbox[i].x1 - buffer->drawX;
      int y = pbox[i].y1 - buffer->drawY;
      int w = pbox[i].x2 - pbox[i].x1;
      int h = pbox[i].y2 - pbox[i].y1;

      int offset = (buffer->offset + 
		    y * buffer->pitch + 
		    x * bytePerPixel);

      viaBlit(vmesa,
	      buffer->bpp, 
	      offset, buffer->pitch,
	      offset, buffer->pitch, 
	      w, h,
	      VIA_BLIT_FILL, pixel, mask); 
   }
}



static void viaClear(GLcontext *ctx, GLbitfield mask)
{
   struct via_context *vmesa = VIA_CONTEXT(ctx);
   __DRIdrawablePrivate *dPriv = vmesa->driDrawable;
   struct via_renderbuffer *const vrb = 
     (struct via_renderbuffer *) dPriv->driverPrivate;
   int flag = 0;
   GLuint i = 0;
   GLuint clear_depth_mask = 0xf << 28;
   GLuint clear_depth = 0;

   VIA_FLUSH_DMA(vmesa);

   if (mask & BUFFER_BIT_FRONT_LEFT) {
      flag |= VIA_FRONT;
      mask &= ~BUFFER_BIT_FRONT_LEFT;
   }

   if (mask & BUFFER_BIT_BACK_LEFT) {
      flag |= VIA_BACK;	
      mask &= ~BUFFER_BIT_BACK_LEFT;
   }

   if (mask & BUFFER_BIT_DEPTH) {
      flag |= VIA_DEPTH;
      clear_depth = (GLuint)(ctx->Depth.Clear * vmesa->ClearDepth);
      clear_depth_mask &= ~vmesa->depth_clear_mask;
      mask &= ~BUFFER_BIT_DEPTH;
   }
    
   if (mask & BUFFER_BIT_STENCIL) {
      if (vmesa->have_hw_stencil) {
	 if ((ctx->Stencil.WriteMask[0] & 0xff) == 0xff) {
	    flag |= VIA_DEPTH;
	    clear_depth &= ~0xff;
	    clear_depth |= (ctx->Stencil.Clear & 0xff);
	    clear_depth_mask &= ~vmesa->stencil_clear_mask;
	    mask &= ~BUFFER_BIT_STENCIL;
	 }
	 else {
	    if (VIA_DEBUG & DEBUG_2D)
	       fprintf(stderr, "Clear stencil writemask %x\n", 
		       ctx->Stencil.WriteMask[0]);
	 }
      }
   }

   /* 16bpp doesn't support masked clears */
   if (vmesa->viaScreen->bytesPerPixel == 2 &&
       vmesa->ClearMask & 0xf0000000) {
      if (flag & VIA_FRONT)
         mask |= BUFFER_BIT_FRONT_LEFT;
      if (flag & VIA_BACK)
         mask |= BUFFER_BIT_BACK_LEFT;
      flag &= ~(VIA_FRONT | VIA_BACK);
   }
    
   if (flag) {
      drm_clip_rect_t *boxes, *tmp_boxes = 0;
      int nr = 0;
      GLint cx, cy, cw, ch;
      GLboolean all;

      LOCK_HARDWARE(vmesa);
	    
      /* get region after locking: */
      cx = ctx->DrawBuffer->_Xmin;
      cy = ctx->DrawBuffer->_Ymin;
      cw = ctx->DrawBuffer->_Xmax - cx;
      ch = ctx->DrawBuffer->_Ymax - cy;
      all = (cw == ctx->DrawBuffer->Width && ch == ctx->DrawBuffer->Height);

      /* flip top to bottom */
      cy = dPriv->h - cy - ch;
      cx += vrb->drawX;
      cy += vrb->drawY;
        
      if (!all) {
	 drm_clip_rect_t *b = vmesa->pClipRects;	 
	 
	 boxes = tmp_boxes = 
	    (drm_clip_rect_t *)malloc(vmesa->numClipRects * 
				      sizeof(drm_clip_rect_t)); 
	 if (!boxes) {
	    UNLOCK_HARDWARE(vmesa);
	    return;
	 }

	 for (; i < vmesa->numClipRects; i++) {
	    GLint x = b[i].x1;
	    GLint y = b[i].y1;
	    GLint w = b[i].x2 - x;
	    GLint h = b[i].y2 - y;

	    if (x < cx) w -= cx - x, x = cx;
	    if (y < cy) h -= cy - y, y = cy;
	    if (x + w > cx + cw) w = cx + cw - x;
	    if (y + h > cy + ch) h = cy + ch - y;
	    if (w <= 0) continue;
	    if (h <= 0) continue;

	    boxes[nr].x1 = x;
	    boxes[nr].y1 = y;
	    boxes[nr].x2 = x + w;
	    boxes[nr].y2 = y + h;
	    nr++;
	 }
      }
      else {
	 boxes = vmesa->pClipRects;
	 nr = vmesa->numClipRects;
      }
	    
      if (flag & VIA_FRONT) {
	 viaFillBuffer(vmesa, &vmesa->front, boxes, nr, vmesa->ClearColor,
		       vmesa->ClearMask);
      } 
		
      if (flag & VIA_BACK) {
	 viaFillBuffer(vmesa, &vmesa->back, boxes, nr, vmesa->ClearColor, 
		       vmesa->ClearMask);
      }

      if (flag & VIA_DEPTH) {
	 viaFillBuffer(vmesa, &vmesa->depth, boxes, nr, clear_depth,
		       clear_depth_mask);
      }		

      viaFlushDmaLocked(vmesa, VIA_NO_CLIPRECTS);
      UNLOCK_HARDWARE(vmesa);

      if (tmp_boxes)
	 free(tmp_boxes);
   }
   
   if (mask)
      _swrast_Clear(ctx, mask);
}




static void viaDoSwapBuffers(struct via_context *vmesa,
			     drm_clip_rect_t *b,
			     GLuint nbox)
{    
   GLuint bytePerPixel = vmesa->viaScreen->bitsPerPixel >> 3;
   struct via_renderbuffer *front = &vmesa->front;
   struct via_renderbuffer *back = &vmesa->back;
   GLuint i;
        
   for (i = 0; i < nbox; i++, b++) {        
      GLint x = b->x1 - back->drawX;
      GLint y = b->y1 - back->drawY;
      GLint w = b->x2 - b->x1;
      GLint h = b->y2 - b->y1;
	
      GLuint src = back->offset + y * back->pitch + x * bytePerPixel;
      GLuint dest = front->offset + y * front->pitch + x * bytePerPixel;

      viaBlit(vmesa, 
	      bytePerPixel << 3, 
	      src, back->pitch,
	      dest, front->pitch,
	      w, h,
	      VIA_BLIT_COPY, 0, 0); 
   }

   viaFlushDmaLocked(vmesa, VIA_NO_CLIPRECTS); /* redundant */
}


static void viaEmitBreadcrumbLocked( struct via_context *vmesa )
{
   struct via_renderbuffer *buffer = &vmesa->breadcrumb;
   GLuint value = vmesa->lastBreadcrumbWrite + 1;

   if (VIA_DEBUG & DEBUG_IOCTL) 
      fprintf(stderr, "%s %d\n", __FUNCTION__, value);

   assert(!vmesa->dmaLow);

   viaBlit(vmesa,
	   buffer->bpp, 
	   buffer->offset, buffer->pitch,
	   buffer->offset, buffer->pitch, 
	   1, 1,
	   VIA_BLIT_FILL, value, 0); 

   viaFlushDmaLocked(vmesa, VIA_NO_CLIPRECTS); /* often redundant */
   vmesa->lastBreadcrumbWrite = value;
}

void viaEmitBreadcrumb( struct via_context *vmesa )
{
   LOCK_HARDWARE(vmesa);
   if (vmesa->dmaLow) 
      viaFlushDmaLocked(vmesa, 0);

   viaEmitBreadcrumbLocked( vmesa );
   UNLOCK_HARDWARE(vmesa);
}

static GLboolean viaCheckIdle( struct via_context *vmesa )
{
   if ((vmesa->regEngineStatus[0] & 0xFFFEFFFF) == 0x00020000) {
      return GL_TRUE;
   }
   return GL_FALSE;
}


GLboolean viaCheckBreadcrumb( struct via_context *vmesa, GLuint value )
{
   GLuint *buf = (GLuint *)vmesa->breadcrumb.map; 
   vmesa->lastBreadcrumbRead = *buf;

   if (VIA_DEBUG & DEBUG_IOCTL) 
      fprintf(stderr, "%s %d < %d: %d\n", __FUNCTION__, value, 
	      vmesa->lastBreadcrumbRead,
	      !VIA_GEQ_WRAP(value, vmesa->lastBreadcrumbRead));

   return !VIA_GEQ_WRAP(value, vmesa->lastBreadcrumbRead);
}

static void viaWaitBreadcrumb( struct via_context *vmesa, GLuint value )
{
   if (VIA_DEBUG & DEBUG_IOCTL) 
      fprintf(stderr, "%s %d\n", __FUNCTION__, value);

   assert(!VIA_GEQ_WRAP(value, vmesa->lastBreadcrumbWrite));

   while (!viaCheckBreadcrumb( vmesa, value )) {
      viaSwapOutWork( vmesa );
      via_release_pending_textures( vmesa );
   }
}


void viaWaitIdle( struct via_context *vmesa, GLboolean light )
{
   VIA_FLUSH_DMA(vmesa);

   if (VIA_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s lastDma %d lastBreadcrumbWrite %d\n",
	      __FUNCTION__, vmesa->lastDma, vmesa->lastBreadcrumbWrite);

   /* Need to emit a new breadcrumb?
    */
   if (vmesa->lastDma == vmesa->lastBreadcrumbWrite) {
      LOCK_HARDWARE(vmesa);
      viaEmitBreadcrumbLocked( vmesa );
      UNLOCK_HARDWARE(vmesa);
   }

   /* Need to wait?
    */
   if (VIA_GEQ_WRAP(vmesa->lastDma, vmesa->lastBreadcrumbRead)) 
      viaWaitBreadcrumb( vmesa, vmesa->lastDma );

   if (light) return;

   LOCK_HARDWARE(vmesa);
   while(!viaCheckIdle(vmesa))
      ;
   UNLOCK_HARDWARE(vmesa);
   via_release_pending_textures(vmesa);
}


void viaWaitIdleLocked( struct via_context *vmesa, GLboolean light )
{
   if (vmesa->dmaLow) 
      viaFlushDmaLocked(vmesa, 0);

   if (VIA_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s lastDma %d lastBreadcrumbWrite %d\n",
	      __FUNCTION__, vmesa->lastDma, vmesa->lastBreadcrumbWrite);

   /* Need to emit a new breadcrumb?
    */
   if (vmesa->lastDma == vmesa->lastBreadcrumbWrite) {
      viaEmitBreadcrumbLocked( vmesa );
   }

   /* Need to wait?
    */
   if (vmesa->lastDma >= vmesa->lastBreadcrumbRead) 
      viaWaitBreadcrumb( vmesa, vmesa->lastDma );

   if (light) return;

   while(!viaCheckIdle(vmesa))
      ;

   via_release_pending_textures(vmesa);
}



/* Wait for command stream to be processed *and* the next vblank to
 * occur.  Equivalent to calling WAIT_IDLE() and then WaitVBlank,
 * except that WAIT_IDLE() will spin the CPU polling, while this is
 * IRQ driven.
 */
static void viaWaitIdleVBlank( const __DRIdrawablePrivate *dPriv, 
			       struct via_context *vmesa,
			       GLuint value )
{
   GLboolean missed_target;

   VIA_FLUSH_DMA(vmesa); 

   if (!value)
      return;

   do {
      if (value < vmesa->lastBreadcrumbRead ||
	  vmesa->thrashing)
	 viaSwapOutWork(vmesa);

      driWaitForVBlank( dPriv, & vmesa->vbl_seq, 
			vmesa->vblank_flags, & missed_target );
      if ( missed_target ) {
	 vmesa->swap_missed_count++;
	 (*dri_interface->getUST)( &vmesa->swap_missed_ust );
      }
   } 
   while (!viaCheckBreadcrumb(vmesa, value));	 

   vmesa->thrashing = 0;	/* reset flag on swap */
   vmesa->swap_count++;   
   via_release_pending_textures( vmesa );
}



static void viaDoPageFlipLocked(struct via_context *vmesa, GLuint offset)
{
   RING_VARS;

   if (VIA_DEBUG & DEBUG_2D)
      fprintf(stderr, "%s %x\n", __FUNCTION__, offset);

   if (!vmesa->nDoneFirstFlip) {
      vmesa->nDoneFirstFlip = GL_TRUE;
      BEGIN_RING(4);
      OUT_RING(HALCYON_HEADER2);
      OUT_RING(0x00fe0000);
      OUT_RING(0x0000000e);
      OUT_RING(0x0000000e);
      ADVANCE_RING();
   }

   BEGIN_RING(4);
   OUT_RING( HALCYON_HEADER2 );
   OUT_RING( 0x00fe0000 );
   OUT_RING((HC_SubA_HFBBasL << 24) | (offset & 0xFFFFF8) | 0x2);
   OUT_RING((HC_SubA_HFBDrawFirst << 24) |
	    ((offset & 0xFF000000) >> 24) | 0x0100);
   ADVANCE_RING();

   vmesa->pfCurrentOffset = vmesa->sarea->pfCurrentOffset = offset;

   viaFlushDmaLocked(vmesa, VIA_NO_CLIPRECTS); /* often redundant */
}

void viaResetPageFlippingLocked(struct via_context *vmesa)
{
   if (VIA_DEBUG & DEBUG_2D)
      fprintf(stderr, "%s\n", __FUNCTION__);

   viaDoPageFlipLocked( vmesa, 0 );

   if (vmesa->front.offset != 0) {
      struct via_renderbuffer buffer_tmp;
      memcpy(&buffer_tmp, &vmesa->back, sizeof(struct via_renderbuffer));
      memcpy(&vmesa->back, &vmesa->front, sizeof(struct via_renderbuffer));
      memcpy(&vmesa->front, &buffer_tmp, sizeof(struct via_renderbuffer));
   }

   assert(vmesa->front.offset == 0);
   vmesa->doPageFlip = vmesa->allowPageFlip = 0;
}


/*
 * Copy the back buffer to the front buffer. 
 */
void viaCopyBuffer(const __DRIdrawablePrivate *dPriv)
{
   struct via_context *vmesa = 
      (struct via_context *)dPriv->driContextPriv->driverPrivate;

   if (VIA_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, 
	      "%s: lastSwap[1] %d lastSwap[0] %d lastWrite %d lastRead %d\n",
	      __FUNCTION__,
	      vmesa->lastSwap[1], 
	      vmesa->lastSwap[0], 
	      vmesa->lastBreadcrumbWrite,
	      vmesa->lastBreadcrumbRead);

   VIA_FLUSH_DMA(vmesa);

   if (vmesa->vblank_flags == VBLANK_FLAG_SYNC &&
       vmesa->lastBreadcrumbWrite > 1)
      viaWaitIdleVBlank(dPriv, vmesa, vmesa->lastBreadcrumbWrite-1);
   else
      viaWaitIdleVBlank(dPriv, vmesa, vmesa->lastSwap[1]);

   LOCK_HARDWARE(vmesa);

   /* Catch and cleanup situation where we were pageflipping but have
    * stopped.
    */
   if (dPriv->numClipRects && vmesa->sarea->pfCurrentOffset != 0) {
      viaResetPageFlippingLocked(vmesa);
      UNLOCK_HARDWARE(vmesa);
      return;
   }

   viaDoSwapBuffers(vmesa, dPriv->pClipRects, dPriv->numClipRects);
   vmesa->lastSwap[1] = vmesa->lastSwap[0];
   vmesa->lastSwap[0] = vmesa->lastBreadcrumbWrite;
   viaEmitBreadcrumbLocked(vmesa);
   UNLOCK_HARDWARE(vmesa);

   (*dri_interface->getUST)( &vmesa->swap_ust );
}


void viaPageFlip(const __DRIdrawablePrivate *dPriv)
{
    struct via_context *vmesa = 
       (struct via_context *)dPriv->driContextPriv->driverPrivate;
    struct via_renderbuffer buffer_tmp;

    VIA_FLUSH_DMA(vmesa);
   if (vmesa->vblank_flags == VBLANK_FLAG_SYNC &&
       vmesa->lastBreadcrumbWrite > 1)
      viaWaitIdleVBlank(dPriv, vmesa, vmesa->lastBreadcrumbWrite - 1);
   else
      viaWaitIdleVBlank(dPriv, vmesa, vmesa->lastSwap[0]);

    LOCK_HARDWARE(vmesa);
    viaDoPageFlipLocked(vmesa, vmesa->back.offset);
    vmesa->lastSwap[1] = vmesa->lastSwap[0];
    vmesa->lastSwap[0] = vmesa->lastBreadcrumbWrite;
    viaEmitBreadcrumbLocked(vmesa);
    UNLOCK_HARDWARE(vmesa);

    (*dri_interface->getUST)( &vmesa->swap_ust );


    /* KW: FIXME: When buffers are freed, could free frontbuffer by
     * accident:
     */
    memcpy(&buffer_tmp, &vmesa->back, sizeof(struct via_renderbuffer));
    memcpy(&vmesa->back, &vmesa->front, sizeof(struct via_renderbuffer));
    memcpy(&vmesa->front, &buffer_tmp, sizeof(struct via_renderbuffer));
}




#define VIA_CMDBUF_MAX_LAG 50000

static int fire_buffer(struct via_context *vmesa)
{
   drm_via_cmdbuffer_t bufI;
   int ret;

   bufI.buf = (char *)vmesa->dma;
   bufI.size = vmesa->dmaLow;

   if (vmesa->useAgp) {
      drm_via_cmdbuf_size_t bSiz;

      /* Do the CMDBUF_SIZE ioctl:
       */
      bSiz.func = VIA_CMDBUF_LAG;
      bSiz.wait = 1;
      bSiz.size = VIA_CMDBUF_MAX_LAG;
      do {
	 ret = drmCommandWriteRead(vmesa->driFd, DRM_VIA_CMDBUF_SIZE, 
				   &bSiz, sizeof(bSiz));
      } while (ret == -EAGAIN);
      if (ret) {
	 UNLOCK_HARDWARE(vmesa);
	 fprintf(stderr, "%s: DRM_VIA_CMDBUF_SIZE returned %d\n",
		 __FUNCTION__, ret);
	 abort();
	 return ret;
      }

      /* Actually fire the buffer:
       */
      do {
	 ret = drmCommandWrite(vmesa->driFd, DRM_VIA_CMDBUFFER, 
			       &bufI, sizeof(bufI));
      } while (ret == -EAGAIN);
      if (ret) {
	 UNLOCK_HARDWARE(vmesa);
	 fprintf(stderr, "%s: DRM_VIA_CMDBUFFER returned %d\n",
		 __FUNCTION__, ret);
	 abort();
	 /* If this fails, the original code fell back to the PCI path. 
	  */
      }
      else 
	 return 0;

      /* Fall through to PCI handling?!?
       */
      viaWaitIdleLocked(vmesa, GL_FALSE);
   }
	    
   ret = drmCommandWrite(vmesa->driFd, DRM_VIA_PCICMD, &bufI, sizeof(bufI));
   if (ret) {
      UNLOCK_HARDWARE(vmesa);
      dump_dma(vmesa);
      fprintf(stderr, "%s: DRM_VIA_PCICMD returned %d\n", __FUNCTION__, ret); 
      abort();
   }

   return ret;
}


/* Inserts the surface addresss and active cliprects one at a time
 * into the head of the DMA buffer being flushed.  Fires the buffer
 * for each cliprect.
 */
static void via_emit_cliprect(struct via_context *vmesa,
			      drm_clip_rect_t *b) 
{
   struct via_renderbuffer *buffer = vmesa->drawBuffer;
   GLuint *vb = (GLuint *)(vmesa->dma + vmesa->dmaCliprectAddr);

   GLuint format = (vmesa->viaScreen->bitsPerPixel == 0x20 
		    ? HC_HDBFM_ARGB8888 
		    : HC_HDBFM_RGB565);

   GLuint pitch = buffer->pitch;
   GLuint offset = buffer->offset;

   if (0)
      fprintf(stderr, "emit cliprect for box %d,%d %d,%d\n", 
	      b->x1, b->y1, b->x2, b->y2);

   vb[0] = HC_HEADER2;
   vb[1] = (HC_ParaType_NotTex << 16);

   if (vmesa->driDrawable->w == 0 || vmesa->driDrawable->h == 0) {
      vb[2] = (HC_SubA_HClipTB << 24) | 0x0;
      vb[3] = (HC_SubA_HClipLR << 24) | 0x0;
   }
   else {
      vb[2] = (HC_SubA_HClipTB << 24) | (b->y1 << 12) | b->y2;
      vb[3] = (HC_SubA_HClipLR << 24) | (b->x1 << 12) | b->x2;
   }
	    
   vb[4] = (HC_SubA_HDBBasL << 24) | (offset & 0xFFFFFF);
   vb[5] = (HC_SubA_HDBBasH << 24) | ((offset & 0xFF000000) >> 24); 

   vb[6] = (HC_SubA_HSPXYOS << 24);
   vb[7] = (HC_SubA_HDBFM << 24) | HC_HDBLoc_Local | format | pitch;
}



static int intersect_rect(drm_clip_rect_t *out,
                          drm_clip_rect_t *a,
                          drm_clip_rect_t *b)
{
    *out = *a;
    
    if (0)
       fprintf(stderr, "intersect %d,%d %d,%d and %d,%d %d,%d\n", 
	       a->x1, a->y1, a->x2, a->y2,
	       b->x1, b->y1, b->x2, b->y2);

    if (b->x1 > out->x1) out->x1 = b->x1;
    if (b->x2 < out->x2) out->x2 = b->x2;
    if (out->x1 >= out->x2) return 0;

    if (b->y1 > out->y1) out->y1 = b->y1;
    if (b->y2 < out->y2) out->y2 = b->y2;
    if (out->y1 >= out->y2) return 0;

    return 1;
}

void viaFlushDmaLocked(struct via_context *vmesa, GLuint flags)
{
   int i;
   RING_VARS;

   if (VIA_DEBUG & (DEBUG_IOCTL|DEBUG_DMA))
      fprintf(stderr, "%s\n", __FUNCTION__);

   if (*(GLuint *)vmesa->driHwLock != (DRM_LOCK_HELD|vmesa->hHWContext) &&
       *(GLuint *)vmesa->driHwLock != 
       (DRM_LOCK_HELD|DRM_LOCK_CONT|vmesa->hHWContext)) {
      fprintf(stderr, "%s called without lock held\n", __FUNCTION__);
      abort();
   }

   if (vmesa->dmaLow == 0) {
      return;
   }

   assert(vmesa->dmaLastPrim == 0);

   /* viaFinishPrimitive can add up to 8 bytes beyond VIA_DMA_HIGHWATER:
    */
   if (vmesa->dmaLow > VIA_DMA_HIGHWATER + 8) {
      fprintf(stderr, "buffer overflow in Flush Prims = %d\n",vmesa->dmaLow);
      abort();
   }

   switch (vmesa->dmaLow & 0x1F) {	
   case 8:
      BEGIN_RING_NOCHECK( 6 );
      OUT_RING( HC_HEADER2 );
      OUT_RING( (HC_ParaType_NotTex << 16) );
      OUT_RING( HC_DUMMY );
      OUT_RING( HC_DUMMY );
      OUT_RING( HC_DUMMY );
      OUT_RING( HC_DUMMY );
      ADVANCE_RING();
      break;
   case 16:
      BEGIN_RING_NOCHECK( 4 );
      OUT_RING( HC_HEADER2 );
      OUT_RING( (HC_ParaType_NotTex << 16) );
      OUT_RING( HC_DUMMY );
      OUT_RING( HC_DUMMY );
      ADVANCE_RING();
      break;    
   case 24:
      BEGIN_RING_NOCHECK( 10 );
      OUT_RING( HC_HEADER2 );
      OUT_RING( (HC_ParaType_NotTex << 16) );
      OUT_RING( HC_DUMMY );
      OUT_RING( HC_DUMMY );
      OUT_RING( HC_DUMMY );
      OUT_RING( HC_DUMMY );
      OUT_RING( HC_DUMMY );
      OUT_RING( HC_DUMMY );
      OUT_RING( HC_DUMMY );
      OUT_RING( HC_DUMMY );	
      ADVANCE_RING();
      break;    
   case 0:
      break;
   default:
      if (VIA_DEBUG & DEBUG_IOCTL)
	 fprintf(stderr, "%s: unaligned value for vmesa->dmaLow: %x\n",
		 __FUNCTION__, vmesa->dmaLow);
   }

   vmesa->lastDma = vmesa->lastBreadcrumbWrite;

   if (VIA_DEBUG & DEBUG_DMA)
      dump_dma( vmesa );

   if (flags & VIA_NO_CLIPRECTS) {
      if (0) fprintf(stderr, "%s VIA_NO_CLIPRECTS\n", __FUNCTION__);
      assert(vmesa->dmaCliprectAddr == ~0);
      fire_buffer( vmesa );
   }
   else if (vmesa->dmaCliprectAddr == ~0) {
      /* Contains only state.  Could just dump the packet?
       */
      if (0) fprintf(stderr, "%s: no dmaCliprectAddr\n", __FUNCTION__);
      if (0) fire_buffer( vmesa );
   }
   else if (vmesa->numClipRects) {
      drm_clip_rect_t *pbox = vmesa->pClipRects;
      __DRIdrawablePrivate *dPriv = vmesa->driDrawable;
      struct via_renderbuffer *const vrb = 
	(struct via_renderbuffer *) dPriv->driverPrivate;

      for (i = 0; i < vmesa->numClipRects; i++) {
	 drm_clip_rect_t b;

	 b.x1 = pbox[i].x1;
	 b.x2 = pbox[i].x2;
	 b.y1 = pbox[i].y1;
	 b.y2 = pbox[i].y2;

	 if (vmesa->scissor &&
	     !intersect_rect(&b, &b, &vmesa->scissorRect)) 
	    continue;

	 via_emit_cliprect(vmesa, &b);

	 if (fire_buffer(vmesa) != 0) {
	    dump_dma( vmesa );
	    goto done;
	 }
      }
   } else {
      if (0) fprintf(stderr, "%s: no cliprects\n", __FUNCTION__);
      UNLOCK_HARDWARE(vmesa);
      sched_yield();
      LOCK_HARDWARE(vmesa);
   }

 done:
   /* Reset vmesa vars:
    */
   vmesa->dmaLow = 0;
   vmesa->dmaCliprectAddr = ~0;
   vmesa->newEmitState = ~0;
}

void viaWrapPrimitive( struct via_context *vmesa )
{
   GLenum renderPrimitive = vmesa->renderPrimitive;
   GLenum hwPrimitive = vmesa->hwPrimitive;

   if (VIA_DEBUG & DEBUG_PRIMS) fprintf(stderr, "%s\n", __FUNCTION__);
   
   if (vmesa->dmaLastPrim)
      viaFinishPrimitive( vmesa );
   
   viaFlushDma(vmesa);

   if (renderPrimitive != GL_POLYGON + 1)
      viaRasterPrimitive( vmesa->glCtx,
			  renderPrimitive,
			  hwPrimitive );

}

void viaFlushDma(struct via_context *vmesa)
{
   if (vmesa->dmaLow) {
      assert(!vmesa->dmaLastPrim);

      LOCK_HARDWARE(vmesa); 
      viaFlushDmaLocked(vmesa, 0);
      UNLOCK_HARDWARE(vmesa);
   }
}

static void viaFlush(GLcontext *ctx)
{
    struct via_context *vmesa = VIA_CONTEXT(ctx);
    VIA_FLUSH_DMA(vmesa);
}

static void viaFinish(GLcontext *ctx)
{
    struct via_context *vmesa = VIA_CONTEXT(ctx);
    VIA_FLUSH_DMA(vmesa);
    viaWaitIdle(vmesa, GL_FALSE);
}

static void viaClearStencil(GLcontext *ctx,  int s)
{
    return;
}

void viaInitIoctlFuncs(GLcontext *ctx)
{
    ctx->Driver.Flush = viaFlush;
    ctx->Driver.Clear = viaClear;
    ctx->Driver.Finish = viaFinish;
    ctx->Driver.ClearStencil = viaClearStencil;
}



