/* $XFree86: xc/lib/GL/mesa/src/drv/gamma/gamma_macros.h,v 1.5 2002/02/22 21:33:02 dawes Exp $ */
/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <kevin@precisioninsight.com>
 *
 */

#ifndef _GAMMA_MACROS_H_
#define _GAMMA_MACROS_H_

#define DEBUG_DRMDMA
#define DEBUG_ERRORS
#define DEBUG_COMMANDS_NOT
#define DEBUG_VERBOSE_NOT
#define DEBUG_VERBOSE_EXTRA_NOT

#define RANDOMIZE_COLORS_NOT
#define TURN_OFF_CLEARS_NOT
#define CULL_ALL_PRIMS_NOT
#define TURN_OFF_DEPTH_NOT
#define TURN_OFF_BLEND_NOT
#define FAST_CLEAR_4_NOT
#define FORCE_DEPTH32_NOT
#define DONT_SEND_DMA_NOT
#define TURN_OFF_FCP_NOT
#define TURN_OFF_TEXTURES_NOT
#define DO_VALIDATE

#define GAMMA_DMA_BUFFER_SIZE 4096

#if 0
#define GAMMA_DMA_SEND_FLAGS    DRM_DMA_PRIORITY
#define GAMMA_DMA_SEND_FLAGS    DRM_DMA_BLOCK
#else
/* MUST use non-blocking dma flags for drawable lock routines */
#define GAMMA_DMA_SEND_FLAGS    0
#endif

#if 0
#define GAMMA_DMA_GET_FLAGS     \
    (DRM_DMA_SMALLER_OK | DRM_DMA_LARGER_OK | DRM_DMA_WAIT)
#else
#define GAMMA_DMA_GET_FLAGS     DRM_DMA_WAIT
#endif

#if defined(DEBUG_DRMDMA) || defined(DEBUG_COMMANDS) || defined(DEBUG_VERBOSE)
#include <stdio.h>
#endif

/* Note: The argument to DEBUG_GLCMDS() _must_ be enclosed in parenthesis */
#ifdef DEBUG_VERBOSE
#define DEBUG_GLCMDS(s) printf s
#else
#define DEBUG_GLCMDS(s)
#endif

/* Note: The argument to DEBUG_DMACMDS() _must_ be enclosed in parenthesis */
#ifdef DEBUG_DRMDMA
#define DEBUG_DMACMDS(s) printf s
#else
#define DEBUG_DMACMDS(s)
#endif

/* Note: The argument to DEBUG_WRITE() _must_ be enclosed in parenthesis */
#ifdef DEBUG_COMMANDS
#define DEBUG_WRITE(s) printf s
#else
#define DEBUG_WRITE(s)
#endif

/* Note: The argument to DEBUG_ERROR() _must_ be enclosed in parenthesis */
#ifdef DEBUG_ERRORS
#define DEBUG_ERROR(s) printf s
#else
#define DEBUG_ERROR(s)
#endif

#define WRITEV(buf,val1,val2,val3,val4)                               \
do {                                                                  \
    buf++->i = 0x9C008300;                        \
    buf++->f = val1;                                                   \
    buf++->f = val2;                                                   \
    buf++->f = val3;                                                   \
    buf++->f = val4;                                                   \
} while (0)

#define WRITE(buf,reg,val)                                            \
do {                                                                  \
    buf++->i = Glint##reg##Tag;                                       \
    buf++->i = val;                                                   \
    DEBUG_WRITE(("WRITE(buf, %s, 0x%08x);\n", #reg, (int)val));       \
} while (0)

#define WRITEF(buf,reg,val)                                           \
do {                                                                  \
    buf++->i = Glint##reg##Tag;                                       \
    buf++->f = val;                                                   \
    DEBUG_WRITE(("WRITEF(buf, %s, %f);\n", #reg, (float)val));        \
} while (0)

#define CHECK_WC_DMA_BUFFER(gcp,n)                                    \
do {                                                                  \
    (gcp)->WCbufCount += (n<<1);                                      \
} while (0)

#define CHECK_DMA_BUFFER(gcp,n)                                   \
do {                                                                  \
    if ((gcp)->bufCount+(n<<1) >= (gcp)->bufSize)                     \
	PROCESS_DMA_BUFFER(gcp);                                  \
    (gcp)->bufCount += (n<<1);                                        \
} while (0)

#define CHECK_DMA_BUFFER2(gcp,n)                                   \
do {                                                                  \
    if ((gcp)->bufCount+n >= (gcp)->bufSize)                     \
	PROCESS_DMA_BUFFER(gcp);                                  \
    (gcp)->bufCount += n;                                        \
} while (0)

#define FLUSH_DMA_BUFFER(gcp)                                     \
do {                                                                  \
   if (gcp->bufCount)                                                \
	PROCESS_DMA_BUFFER(gcp);                                  \
} while (0)

#ifdef DONT_SEND_DMA
#define GET_DMA(fd, hHWCtx, n, idx, size)
#define SEND_DMA(fd, hHWCtx,n, idx, cnt)
#else
#define GET_DMA(fd, hHWCtx, n, idx, size)                                  \
do {                                                                       \
    drmDMAReq dma;                                                         \
    int retcode, i;                                                        \
                                                                           \
    dma.context       = (hHWCtx);                                          \
    dma.send_count    = 0;                                                 \
    dma.send_list     = NULL;                                              \
    dma.send_sizes    = NULL;                                              \
    dma.flags         = GAMMA_DMA_GET_FLAGS;                               \
    dma.request_count = (n);                                               \
    dma.request_size  = GAMMA_DMA_BUFFER_SIZE;                             \
    dma.request_list  = (idx);                                             \
    dma.request_sizes = (size);                                            \
                                                                           \
    do {                                                                   \
	if ((retcode = drmDMA((fd), &dma))) {                              \
	    DEBUG_DMACMDS(("drmDMA returned %d\n", retcode));              \
	}                                                                  \
    } while (!(dma).granted_count);                                        \
                                                                           \
    for (i = 0; i < (n); i++) {                                            \
	(size)[i] >>= 2; /* Convert from bytes to words */                 \
    }                                                                      \
} while (0)

#define SEND_DMA(fd, hHWCtx, n, idx, cnt)                                  \
do {                                                                       \
    drmDMAReq dma;                                                         \
    int retcode, i;                                                        \
                                                                           \
    for (i = 0; i < (n); i++) {                                            \
	(cnt)[i] <<= 2; /* Convert from words to bytes */                  \
    }                                                                      \
                                                                           \
    dma.context       = (hHWCtx);                                          \
    dma.send_count    = 1;                                                 \
    dma.send_list     = (idx);                                             \
    dma.send_sizes    = (cnt);                                             \
    dma.flags         = GAMMA_DMA_SEND_FLAGS;                              \
    dma.request_count = 0;                                                 \
    dma.request_size  = 0;                                                 \
    dma.request_list  = NULL;                                              \
    dma.request_sizes = NULL;                                              \
                                                                           \
    if ((retcode = drmDMA((fd), &dma))) {                                  \
	DEBUG_DMACMDS(("drmDMA returned %d\n", retcode));                  \
    }                                                                      \
                                                                           \
    for (i = 0; i < (n); i++) {                                            \
	(cnt)[i] = 0;                                                      \
    }                                                                      \
} while (0)
#endif

#define GET_FIRST_DMA(fd, hHWCtx, n, idx, size, buf, cnt, gPriv)           \
do {                                                                       \
    int i;                                                                 \
                                                                           \
    GET_DMA(fd, hHWCtx, n, idx, size);                                     \
                                                                           \
    for (i = 0; i < (n); i++) {                                            \
	(buf)[i] = (dmaBuf)(gPriv)->bufs->list[(idx)[i]].address;          \
	(cnt)[i] = 0;                                                      \
    }                                                                      \
} while (0)

#define PROCESS_DMA_BUFFER_TOP_HALF(gcp)                                   \
do {                                                                       \
    SEND_DMA((gcp)->driFd,                        \
	     (gcp)->hHWContext, 1, &(gcp)->bufIndex, &(gcp)->bufCount);    \
} while (0)

#define PROCESS_DMA_BUFFER_BOTTOM_HALF(gcp)                                \
do {                                                                       \
    GET_DMA((gcp)->driFd,                         \
	    (gcp)->hHWContext, 1, &(gcp)->bufIndex, &(gcp)->bufSize);      \
                                                                           \
    (gcp)->buf =                                                           \
	(dmaBuf)(gcp)->gammaScreen->bufs->list[(gcp)->bufIndex].address; \
} while (0)

#define PROCESS_DMA_BUFFER(gcp)                                        \
do {                                                                       \
    VALIDATE_DRAWABLE_INFO(gcp);                             \
    PROCESS_DMA_BUFFER_TOP_HALF(gcp);                                      \
    PROCESS_DMA_BUFFER_BOTTOM_HALF(gcp);                                   \
} while (0)

#ifdef DO_VALIDATE
#define VALIDATE_DRAWABLE_INFO_NO_LOCK(gcp)                                \
do {                                                                       \
    /*__DRIscreenPrivate *psp = gcp->driScreen;*/                          \
    __DRIdrawablePrivate *pdp = gcp->driDrawable;                          \
                                                                           \
    if (*(pdp->pStamp) != pdp->lastStamp) {                                \
	int old_index = pdp->index;                                        \
	while (*(pdp->pStamp) != pdp->lastStamp) {                         \
	    DRI_VALIDATE_DRAWABLE_INFO_ONCE(pdp);                          \
        }                                                                  \
	if (pdp->index != old_index) {                                     \
	    gcp->Window &= ~W_GIDMask;                                     \
	    gcp->Window |= (pdp->index << 5);                              \
	    CHECK_WC_DMA_BUFFER(gcp, 1);                                   \
	    WRITE(gcp->WCbuf, GLINTWindow, gcp->Window|(gcp->FrameCount<<9));\
	}                                                                  \
									   \
	gammaUpdateViewportOffset( gcp->glCtx);				   \
                                                                           \
	if (pdp->numClipRects == 1 &&                                      \
	    pdp->pClipRects->x1 ==  pdp->x &&                              \
	    pdp->pClipRects->x2 == (pdp->x+pdp->w) &&                      \
	    pdp->pClipRects->y1 ==  pdp->y &&                              \
	    pdp->pClipRects->y2 == (pdp->y+pdp->h)) {                      \
	    CHECK_WC_DMA_BUFFER(gcp, 1);                                   \
	    WRITE(gcp->WCbuf, Rectangle2DControl, 0);                     \
	    gcp->NotClipped = GL_TRUE;                                     \
	} else {                                                           \
	    CHECK_WC_DMA_BUFFER(gcp, 1);                                   \
	    WRITE(gcp->WCbuf, Rectangle2DControl, 1);                     \
	    gcp->NotClipped = GL_FALSE;                                    \
	}                                                                  \
	gcp->WindowChanged = GL_TRUE;                                      \
                                                                           \
	if (gcp->WCbufCount) {                                             \
	    SEND_DMA((gcp)->gammaScreen->driScreen->fd,                \
		     (gcp)->hHWContext, 1, &(gcp)->WCbufIndex,             \
		     &(gcp)->WCbufCount);                                  \
	    (gcp)->WCbufIndex = -1;                                        \
	}                                                                  \
    }                                                                      \
} while (0)

#define VALIDATE_DRAWABLE_INFO_NO_LOCK_POST(gcp)                       \
do {                                                                       \
    if ((gcp)->WCbufIndex < 0) {                                           \
	GET_DMA((gcp)->gammaScreen->driScreen->fd,                     \
		(gcp)->hHWContext, 1, &(gcp)->WCbufIndex,                  \
		&(gcp)->WCbufSize);                                        \
                                                                           \
	(gcp)->WCbuf =                                                     \
	    (dmaBuf)(gcp)->gammaScreen->bufs->                           \
		list[(gcp)->WCbufIndex].address;                           \
    }                                                                      \
} while (0)

#define VALIDATE_DRAWABLE_INFO(gcp)                                    \
do {                                                                       \
    __DRIscreenPrivate *psp = gcp->driScreen;                          \
if (gcp->driDrawable) { \
    DRM_SPINLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);            \
    VALIDATE_DRAWABLE_INFO_NO_LOCK(gcp);                               \
    DRM_SPINUNLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);          \
    VALIDATE_DRAWABLE_INFO_NO_LOCK_POST(gcp);                          \
} \
} while (0)
#else
#define VALIDATE_DRAWABLE_INFO(gcp)
#endif

#define CALC_LOG2(l2,s)                       \
do {                                          \
    int __s = s;                              \
    l2 = 0;                                   \
    while (__s > 1) { ++l2; __s >>= 1; }      \
} while (0)

#endif /* _GAMMA_MACROS_H_ */
