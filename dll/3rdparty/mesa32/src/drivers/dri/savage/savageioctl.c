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
#include <sys/mman.h>

#include "mtypes.h"
#include "macros.h"
#include "dd.h"
#include "context.h"
#include "swrast/swrast.h"
#include "colormac.h"

#include "mm.h"
#include "savagecontext.h"
#include "savageioctl.h"
#include "savage_bci.h"
#include "savagestate.h"
#include "savagespan.h"

#include "drm.h"
#include <sys/ioctl.h>
#include <sys/timeb.h>

#define DEPTH_SCALE_16 ((1<<16)-1)
#define DEPTH_SCALE_24 ((1<<24)-1)


void savageGetDMABuffer( savageContextPtr imesa )
{
   int idx = 0;
   int size = 0;
   drmDMAReq dma;
   int retcode;
   drmBufPtr buf;

   assert (imesa->savageScreen->bufs);

   if (SAVAGE_DEBUG & DEBUG_DMA)
      fprintf(stderr,  "Getting dma buffer\n");

   dma.context = imesa->hHWContext;
   dma.send_count = 0;
   dma.send_list = NULL;
   dma.send_sizes = NULL;
   dma.flags = 0;
   dma.request_count = 1;
   dma.request_size = imesa->bufferSize;
   dma.request_list = &idx;
   dma.request_sizes = &size;
   dma.granted_count = 0;


   if (SAVAGE_DEBUG & DEBUG_DMA)
      fprintf(stderr, "drmDMA (get) ctx %d count %d size 0x%x\n",
	   dma.context, dma.request_count,
	   dma.request_size);

   while (1) {
      retcode = drmDMA(imesa->driFd, &dma);

      if (SAVAGE_DEBUG & DEBUG_DMA)
	 fprintf(stderr, "retcode %d sz %d idx %d count %d\n",
		 retcode,
		 dma.request_sizes[0],
		 dma.request_list[0],
		 dma.granted_count);

      if (retcode == 0 &&
	  dma.request_sizes[0] &&
	  dma.granted_count)
	 break;

      if (SAVAGE_DEBUG & DEBUG_DMA)
	 fprintf(stderr, "\n\nflush");
   }

   buf = &(imesa->savageScreen->bufs->list[idx]);

   if (SAVAGE_DEBUG & DEBUG_DMA)
      fprintf(stderr,
	   "drmDMA (get) returns size[0] 0x%x idx[0] %d\n"
	   "dma_buffer now: buf idx: %d size: %d used: %d addr %p\n",
	   dma.request_sizes[0], dma.request_list[0],
	   buf->idx, buf->total,
	   buf->used, buf->address);

   imesa->dmaVtxBuf.total = buf->total / 4;
   imesa->dmaVtxBuf.used = 0;
   imesa->dmaVtxBuf.flushed = 0;
   imesa->dmaVtxBuf.idx = buf->idx;
   imesa->dmaVtxBuf.buf = (u_int32_t *)buf->address;

   if (SAVAGE_DEBUG & DEBUG_DMA)
      fprintf(stderr, "finished getbuffer\n");
}

#if 0
/* Still keeping this around because it demonstrates page flipping and
 * automatic z-clear. */
static void savage_BCI_clear(GLcontext *ctx, drm_savage_clear_t *pclear)
{
	savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
	int nbox = imesa->sarea->nbox;
	drm_clip_rect_t *pbox = imesa->sarea->boxes;
        int i;

	
      	if (nbox > SAVAGE_NR_SAREA_CLIPRECTS)
     		nbox = SAVAGE_NR_SAREA_CLIPRECTS;

	for (i = 0 ; i < nbox ; i++, pbox++) {
		unsigned int x = pbox->x1;
		unsigned int y = pbox->y1;
		unsigned int width = pbox->x2 - x;
		unsigned int height = pbox->y2 - y;
 		u_int32_t *bciptr;

		if (pbox->x1 > pbox->x2 ||
		    pbox->y1 > pbox->y2 ||
		    pbox->x2 > imesa->savageScreen->width ||
		    pbox->y2 > imesa->savageScreen->height)
			continue;

	   	if ( pclear->flags & SAVAGE_FRONT ) {
		        bciptr = savageDMAAlloc (imesa, 8);
			WRITE_CMD((bciptr) , 0x4BCC8C00,u_int32_t);
			WRITE_CMD((bciptr) , imesa->savageScreen->frontOffset,u_int32_t);
			WRITE_CMD((bciptr) , imesa->savageScreen->frontBitmapDesc,u_int32_t);
			WRITE_CMD((bciptr) , pclear->clear_color,u_int32_t);
			WRITE_CMD((bciptr) , (y <<16) | x,u_int32_t);
			WRITE_CMD((bciptr) , (height << 16) | width,u_int32_t);
			savageDMACommit (imesa, bciptr);
		}
		if ( pclear->flags & SAVAGE_BACK ) {
		        bciptr = savageDMAAlloc (imesa, 8);
			WRITE_CMD((bciptr) , 0x4BCC8C00,u_int32_t);
			WRITE_CMD((bciptr) , imesa->savageScreen->backOffset,u_int32_t);
			WRITE_CMD((bciptr) , imesa->savageScreen->backBitmapDesc,u_int32_t);
			WRITE_CMD((bciptr) , pclear->clear_color,u_int32_t);
			WRITE_CMD((bciptr) , (y <<16) | x,u_int32_t);
			WRITE_CMD((bciptr) , (height << 16) | width,u_int32_t);
			savageDMACommit (imesa, bciptr);
		}
		
		if ( pclear->flags & (SAVAGE_DEPTH |SAVAGE_STENCIL) ) {
		        u_int32_t writeMask = 0x0;
		        if(imesa->hw_stencil)
		        {        
		            if(pclear->flags & SAVAGE_STENCIL)
		            {
		          
		                 writeMask |= 0xFF000000;
		            }
		            if(pclear->flags & SAVAGE_DEPTH)
		            {
		                 writeMask |= 0x00FFFFFF;
		            }
                        }
		        if(imesa->IsFullScreen && imesa->NotFirstFrame &&
			   imesa->savageScreen->chipset >= S3_SAVAGE4)
		        {
		            imesa->regs.s4.zBufCtrl.ni.autoZEnable = GL_TRUE;
                            imesa->regs.s4.zBufCtrl.ni.frameID =
				~imesa->regs.s4.zBufCtrl.ni.frameID;
                            
                            imesa->dirty |= SAVAGE_UPLOAD_GLOBAL;
		        }
		        else
		        {
		            if(imesa->IsFullScreen)
		                imesa->NotFirstFrame = GL_TRUE;
		                
			    if(imesa->hw_stencil)
			    {
				bciptr = savageDMAAlloc (imesa, 10);
			        if(writeMask != 0xFFFFFFFF)
			        {
                                    WRITE_CMD((bciptr) , 0x960100D7,u_int32_t);
                                    WRITE_CMD((bciptr) , writeMask,u_int32_t);
                                }
                            }
			    else
			    {
				bciptr = savageDMAAlloc (imesa, 6);
			    }

			    WRITE_CMD((bciptr) , 0x4BCC8C00,u_int32_t);
			    WRITE_CMD((bciptr) , imesa->savageScreen->depthOffset,u_int32_t);
			    WRITE_CMD((bciptr) , imesa->savageScreen->depthBitmapDesc,u_int32_t);
			    WRITE_CMD((bciptr) , pclear->clear_depth,u_int32_t);
			    WRITE_CMD((bciptr) , (y <<16) | x,u_int32_t);
			    WRITE_CMD((bciptr) , (height << 16) | width,u_int32_t);
			    if(imesa->hw_stencil)
			    {
			        if(writeMask != 0xFFFFFFFF)
			        {
			           WRITE_CMD((bciptr) , 0x960100D7,u_int32_t);
                                   WRITE_CMD((bciptr) , 0xFFFFFFFF,u_int32_t);  
			        }
			    }
			    savageDMACommit (imesa, bciptr);
			}
		}
	}
	/* FK: Make sure that the clear stuff is emitted. Otherwise a
	   software fallback may get overwritten by a delayed clear. */
	savageDMAFlush (imesa);
}

static void savage_BCI_swap(savageContextPtr imesa)
{
    int nbox = imesa->sarea->nbox;
    drm_clip_rect_t *pbox = imesa->sarea->boxes;
    int i;
    volatile u_int32_t *bciptr;
    
    if (nbox > SAVAGE_NR_SAREA_CLIPRECTS)
        nbox = SAVAGE_NR_SAREA_CLIPRECTS;
    savageDMAFlush (imesa);
    
    if(imesa->IsFullScreen)
    { /* full screen*/
        unsigned int tmp0;
        tmp0 = imesa->savageScreen->frontOffset; 
        imesa->savageScreen->frontOffset = imesa->savageScreen->backOffset;
        imesa->savageScreen->backOffset = tmp0;
        
        if(imesa->toggle == TARGET_BACK)
            imesa->toggle = TARGET_FRONT;
        else
            imesa->toggle = TARGET_BACK; 
        
        driFlipRenderbuffers(imesa->glCtx->DrawBuffer,
                             imesa->toggle != TARGET_FRONT);

        imesa->regs.s4.destCtrl.ni.offset = imesa->savageScreen->backOffset>>11;
        imesa->dirty |= SAVAGE_UPLOAD_GLOBAL;
        bciptr = SAVAGE_GET_BCI_POINTER(imesa,3);
        *(bciptr) = 0x960100B0;
        *(bciptr) = (imesa->savageScreen->frontOffset); 
        *(bciptr) = 0xA0000000;
    } 
    
    else
    {  /* Use bitblt copy from back to front buffer*/
        
        for (i = 0 ; i < nbox; i++, pbox++)
        {
            unsigned int w = pbox->x2 - pbox->x1;
            unsigned int h = pbox->y2 - pbox->y1;
            
            if (pbox->x1 > pbox->x2 ||
                pbox->y1 > pbox->y2 ||
                pbox->x2 > imesa->savageScreen->width ||
                pbox->y2 > imesa->savageScreen->height)
                continue;

            bciptr = SAVAGE_GET_BCI_POINTER(imesa,6);
            
            *(bciptr) = 0x4BCC00C0;
            
            *(bciptr) = imesa->savageScreen->backOffset;
            *(bciptr) = imesa->savageScreen->backBitmapDesc;
            *(bciptr) = (pbox->y1 <<16) | pbox->x1;   /*x0, y0*/
            *(bciptr) = (pbox->y1 <<16) | pbox->x1;
            *(bciptr) = (h << 16) | w;
        }
        
    }
}
#endif


static GLboolean intersect_rect( drm_clip_rect_t *out,
				 const drm_clip_rect_t *a,
				 const drm_clip_rect_t *b )
{
   *out = *a;
   if (b->x1 > out->x1) out->x1 = b->x1;
   if (b->y1 > out->y1) out->y1 = b->y1;
   if (b->x2 < out->x2) out->x2 = b->x2;
   if (b->y2 < out->y2) out->y2 = b->y2;

   return ((out->x1 < out->x2) && (out->y1 < out->y2));
}


static GLuint savageIntersectClipRects(drm_clip_rect_t *dest,
				       const drm_clip_rect_t *src,
				       GLuint nsrc,
				       const drm_clip_rect_t *clip)
{
    GLuint i, ndest;

    for (i = 0, ndest = 0; i < nsrc; ++i, ++src) {
	if (intersect_rect(dest, src, clip)) {
	    dest++;
	    ndest++;
	}
    }

    return ndest;
}


static void savageDDClear( GLcontext *ctx, GLbitfield mask )
{
   savageContextPtr imesa = SAVAGE_CONTEXT( ctx );
   GLuint colorMask, depthMask, clearColor, clearDepth, flags;
   GLint cx = ctx->DrawBuffer->_Xmin;
   GLint cy = ctx->DrawBuffer->_Ymin;
   GLint cw = ctx->DrawBuffer->_Xmax - cx;
   GLint ch = ctx->DrawBuffer->_Ymax - cy;

   /* XXX FIX ME: the cx,cy,cw,ch vars are currently ignored! */

   if (SAVAGE_DEBUG & DEBUG_VERBOSE_MSG)
       fprintf (stderr, "%s\n", __FUNCTION__);

   clearColor = imesa->ClearColor;
   if (imesa->float_depth) {
       if (imesa->savageScreen->zpp == 2)
	   clearDepth = savageEncodeFloat16(1.0 - ctx->Depth.Clear);
       else
	   clearDepth = savageEncodeFloat24(1.0 - ctx->Depth.Clear);
   } else {
       if (imesa->savageScreen->zpp == 2)
	   clearDepth = (GLuint) ((1.0 - ctx->Depth.Clear) * DEPTH_SCALE_16);
       else
	   clearDepth = (GLuint) ((1.0 - ctx->Depth.Clear) * DEPTH_SCALE_24);
   }

   colorMask = 0;
   depthMask = 0;
   switch (imesa->savageScreen->cpp) {
   case 2:
       colorMask = PACK_COLOR_565(ctx->Color.ColorMask[0],
				  ctx->Color.ColorMask[1],
				  ctx->Color.ColorMask[2]);
       break;
   case 4:
       colorMask = PACK_COLOR_8888(ctx->Color.ColorMask[3],
				   ctx->Color.ColorMask[2],
				   ctx->Color.ColorMask[1],
				   ctx->Color.ColorMask[0]);
       break;
   }

   flags = 0;

   if (mask & BUFFER_BIT_FRONT_LEFT) {
      flags |= SAVAGE_FRONT;
      mask &= ~BUFFER_BIT_FRONT_LEFT;
   }

   if (mask & BUFFER_BIT_BACK_LEFT) {
      flags |= SAVAGE_BACK;
      mask &= ~BUFFER_BIT_BACK_LEFT;
   }

   if ((mask & BUFFER_BIT_DEPTH) && ctx->Depth.Mask) {
      flags |= SAVAGE_DEPTH;
      depthMask |=
	  (imesa->savageScreen->zpp == 2) ? 0xffffffff : 0x00ffffff;
      mask &= ~BUFFER_BIT_DEPTH;
   }
   
   if((mask & BUFFER_BIT_STENCIL) && imesa->hw_stencil)
   {
      flags |= SAVAGE_DEPTH;
      depthMask |= 0xff000000;
      mask &= ~BUFFER_BIT_STENCIL;
   }

   savageFlushVertices(imesa);

   if (flags) {
       GLboolean depthCleared = GL_FALSE;
       if (flags & (SAVAGE_FRONT|SAVAGE_BACK)) {
	   drm_savage_cmd_header_t *cmd;
	   cmd = savageAllocCmdBuf(imesa, sizeof(drm_savage_cmd_header_t));
	   cmd[0].clear0.cmd = SAVAGE_CMD_CLEAR;
	   if ((flags & SAVAGE_DEPTH) &&
	       clearDepth == clearColor && depthMask == colorMask) {
	       cmd[0].clear0.flags = flags;
	       depthCleared = GL_TRUE;
	   } else
	       cmd[0].clear0.flags = flags & (SAVAGE_FRONT|SAVAGE_BACK);
	   cmd[1].clear1.mask = colorMask;
	   cmd[1].clear1.value = clearColor;
       }

       if ((flags & SAVAGE_DEPTH) && !depthCleared) {
	   drm_savage_cmd_header_t *cmd;
	   cmd = savageAllocCmdBuf(imesa, sizeof(drm_savage_cmd_header_t));
	   cmd[0].clear0.cmd = SAVAGE_CMD_CLEAR;
	   cmd[0].clear0.flags = SAVAGE_DEPTH;
	   cmd[1].clear1.mask = depthMask;
	   cmd[1].clear1.value = clearDepth;
       }
   }

   if (mask) 
      _swrast_Clear( ctx, mask );
}

/*
 * Copy the back buffer to the front buffer. 
 */
void savageSwapBuffers( __DRIdrawablePrivate *dPriv )
{
   savageContextPtr imesa;

   if (SAVAGE_DEBUG & DEBUG_VERBOSE_MSG)
       fprintf (stderr, "%s\n================================\n", __FUNCTION__);

   assert(dPriv);
   assert(dPriv->driContextPriv);
   assert(dPriv->driContextPriv->driverPrivate);

   imesa = (savageContextPtr) dPriv->driContextPriv->driverPrivate;
   if (imesa->IsDouble)
       _mesa_notifySwapBuffers( imesa->glCtx );

   FLUSH_BATCH(imesa);

   if (imesa->sync_frames)
       imesa->lastSwap = savageEmitEvent( imesa, 0 );

   if (imesa->lastSwap != 0)
       savageWaitEvent( imesa, imesa->lastSwap );

   {
       drm_savage_cmd_header_t *cmd = savageAllocCmdBuf(imesa, 0);
       cmd->cmd.cmd = SAVAGE_CMD_SWAP;
       imesa->inSwap = GL_TRUE; /* ignore scissors in savageFlushCmdBuf */
       savageFlushCmdBuf(imesa, GL_FALSE);
       imesa->inSwap = GL_FALSE;
   }

   if (!imesa->sync_frames)
       /* don't sync, but limit the lag to one frame. */
       imesa->lastSwap = savageEmitEvent( imesa, 0 );
}

unsigned int savageEmitEventLocked( savageContextPtr imesa, unsigned int flags )
{
    drm_savage_event_emit_t event;
    int ret;
    event.count = 0;
    event.flags = flags;
    ret = drmCommandWriteRead( imesa->driFd, DRM_SAVAGE_BCI_EVENT_EMIT,
			       &event, sizeof(event) );
    if (ret) {
	fprintf (stderr, "emit event returned %d\n", ret);
	exit (1);
    }
    return event.count;
}
unsigned int savageEmitEvent( savageContextPtr imesa, unsigned int flags )
{
    unsigned int ret;
    LOCK_HARDWARE( imesa );
    ret = savageEmitEventLocked( imesa, flags );
    UNLOCK_HARDWARE( imesa );
    return ret;
}


void savageWaitEvent( savageContextPtr imesa, unsigned int count )
{
    drm_savage_event_wait_t event;
    int ret;
    event.count = count;
    event.flags = 0;
    ret = drmCommandWriteRead( imesa->driFd, DRM_SAVAGE_BCI_EVENT_WAIT,
			       &event, sizeof(event) );
    if (ret) {
	fprintf (stderr, "wait event returned %d\n", ret);
	exit (1);
    }
}


void savageFlushVertices( savageContextPtr imesa )
{
    struct savage_vtxbuf_t *buffer = imesa->vtxBuf;

    if (SAVAGE_DEBUG & DEBUG_VERBOSE_MSG)
	fprintf (stderr, "%s\n", __FUNCTION__);

    if (!buffer->total)
	return;

    if (buffer->used > buffer->flushed) {
	drm_savage_cmd_header_t *cmd;
	/* State must be updated "per primitive" because hardware
	 * culling must be disabled for unfilled primitives, points
	 * and lines. */
	savageEmitChangedState (imesa);
	cmd = savageAllocCmdBuf(imesa, 0);
	cmd->prim.cmd = buffer == &imesa->dmaVtxBuf ?
	    SAVAGE_CMD_DMA_PRIM : SAVAGE_CMD_VB_PRIM;
	cmd->prim.prim = imesa->HwPrim;
	cmd->prim.skip = imesa->skip;
	cmd->prim.start = buffer->flushed / imesa->HwVertexSize;
	cmd->prim.count = buffer->used / imesa->HwVertexSize - cmd->prim.start;
	buffer->flushed = buffer->used;
    }
}

void savageFlushCmdBufLocked( savageContextPtr imesa, GLboolean discard )
{
    __DRIdrawablePrivate *dPriv = imesa->driDrawable;

    if (!imesa->dmaVtxBuf.total)
	discard = GL_FALSE;

    /* complete indexed drawing commands */
    savageFlushElts(imesa);

    if (imesa->cmdBuf.write != imesa->cmdBuf.start || discard) {
	drm_savage_cmdbuf_t cmdbuf;
	drm_savage_cmd_header_t *start;
	int ret;

	/* If we lost the context we must restore the initial state (at
	 * the start of the command buffer). */
	if (imesa->lostContext) {
	    start = imesa->cmdBuf.base;
	    imesa->lostContext = GL_FALSE;
	} else
	    start = imesa->cmdBuf.start;

	if ((SAVAGE_DEBUG & DEBUG_DMA) && discard)
	    fprintf (stderr, "Discarding DMA buffer, used=%u\n",
		     imesa->dmaVtxBuf.used);

	cmdbuf.dma_idx = imesa->dmaVtxBuf.idx;
	cmdbuf.discard = discard;
	cmdbuf.vb_addr = imesa->clientVtxBuf.buf;
	cmdbuf.vb_size = imesa->clientVtxBuf.total*4;
	cmdbuf.vb_stride = imesa->HwVertexSize;
	cmdbuf.cmd_addr = start;
	cmdbuf.size = (imesa->cmdBuf.write - start);
	if (!imesa->inSwap && imesa->scissor.enabled) {
	    drm_clip_rect_t *box = dPriv->pClipRects, *ibox;
	    drm_clip_rect_t scissor;
	    GLuint nbox = dPriv->numClipRects, nibox;
	    /* transform and clip scissor to viewport */
	    scissor.x1 = MAX2(imesa->scissor.x, 0) + dPriv->x;
	    scissor.y1 = MAX2(dPriv->h - imesa->scissor.y - imesa->scissor.h,
			      0) + dPriv->y;
	    scissor.x2 = MIN2(imesa->scissor.x + imesa->scissor.w,
			      dPriv->w) + dPriv->x;
	    scissor.y2 = MIN2(dPriv->h - imesa->scissor.y,
			      dPriv->h) + dPriv->y;
	    /* intersect cliprects with scissor */
	    ibox = malloc(dPriv->numClipRects*sizeof(drm_clip_rect_t));
	    if (!ibox) {
		fprintf(stderr, "Out of memory.\n");
		exit(1);
	    }
	    nibox = savageIntersectClipRects(ibox, box, nbox, &scissor);
	    cmdbuf.nbox = nibox;
	    cmdbuf.box_addr = ibox;
	} else {
	    cmdbuf.nbox = dPriv->numClipRects;
	    cmdbuf.box_addr = dPriv->pClipRects;
	}

	ret = drmCommandWrite( imesa->driFd, DRM_SAVAGE_BCI_CMDBUF,
			       &cmdbuf, sizeof(cmdbuf) );
	if (ret) {
	    fprintf (stderr, "cmdbuf ioctl returned %d\n", ret);
	    exit(1);
	}

	if (cmdbuf.box_addr != dPriv->pClipRects) {
	    free(cmdbuf.box_addr);
	}

	/* Save the current state at the start of the command buffer. That
	 * state will only be emitted, if the context was lost since the
	 * last command buffer. */
	imesa->cmdBuf.write = imesa->cmdBuf.base;
	savageEmitOldState(imesa);
	imesa->cmdBuf.start = imesa->cmdBuf.write;
    }

    if (discard) {
	assert (!savageHaveIndexedVerts(imesa));
	imesa->dmaVtxBuf.total = 0;
	imesa->dmaVtxBuf.used = 0;
	imesa->dmaVtxBuf.flushed = 0;
    }
    if (!savageHaveIndexedVerts(imesa)) {
	imesa->clientVtxBuf.used = 0;
	imesa->clientVtxBuf.flushed = 0;
    }
}


void savageFlushCmdBuf( savageContextPtr imesa, GLboolean discard ) 
{
    if (SAVAGE_DEBUG & DEBUG_VERBOSE_MSG)
	fprintf (stderr, "%s\n", __FUNCTION__);
    LOCK_HARDWARE(imesa);
    savageFlushCmdBufLocked (imesa, discard);
    UNLOCK_HARDWARE(imesa);
}


static void savageDDFlush( GLcontext *ctx )
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    if (SAVAGE_DEBUG & DEBUG_VERBOSE_MSG)
	fprintf (stderr, "%s\n", __FUNCTION__);
    savageFlushVertices (imesa);
    savageFlushCmdBuf(imesa, GL_FALSE);
}

static void savageDDFinish( GLcontext *ctx  ) 
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    if (SAVAGE_DEBUG & DEBUG_VERBOSE_MSG)
	fprintf (stderr, "%s\n", __FUNCTION__);
    savageFlushVertices (imesa);
    savageFlushCmdBuf(imesa, GL_FALSE);
    WAIT_IDLE_EMPTY(imesa);
}

void savageDDInitIoctlFuncs( GLcontext *ctx )
{
   ctx->Driver.Clear = savageDDClear;
   ctx->Driver.Flush = savageDDFlush;
   ctx->Driver.Finish = savageDDFinish;
}
