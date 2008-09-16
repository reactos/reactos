/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/glint/glint_dri.h,v 1.7 2002/10/30 12:52:16 alanh Exp $ */
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
 * Author:
 *   Jens Owen <jens@tungstengraphics.com>
 *
 */

#ifndef _GLINT_DRI_H_
#define _GLINT_DRI_H_

#include "xf86drm.h"
#include "glint_common.h"

typedef struct {
	unsigned int	GDeltaMode;
	unsigned int	GDepthMode;
	unsigned int	GGeometryMode;
	unsigned int	GTransformMode;
} GAMMAContextRegionRec, *GAMMAContextRegionPtr;

typedef struct {
	unsigned char next, prev; /* indices to form a circular LRU  */
	unsigned char in_use;	/* owned by a client, or free? */
	int age;		/* tracked by clients to update local LRU's */
} GAMMATextureRegionRec, *GAMMATextureRegionPtr;

typedef struct {
	GAMMAContextRegionRec context_state;

	unsigned int dirty;

	/* Maintain an LRU of contiguous regions of texture space.  If
	 * you think you own a region of texture memory, and it has an
	 * age different to the one you set, then you are mistaken and
	 * it has been stolen by another client.  If global texAge
	 * hasn't changed, there is no need to walk the list.
	 *
	 * These regions can be used as a proxy for the fine-grained
	 * texture information of other clients - by maintaining them
	 * in the same lru which is used to age their own textures,
	 * clients have an approximate lru for the whole of global
	 * texture space, and can make informed decisions as to which
	 * areas to kick out.  There is no need to choose whether to
	 * kick out your own texture or someone else's - simply eject
	 * them all in LRU order.  
	 */
   
#define GAMMA_NR_TEX_REGIONS 64
	GAMMATextureRegionRec texList[GAMMA_NR_TEX_REGIONS+1];
				/* Last elt is sentinal */
        int texAge;		/* last time texture was uploaded */
        int last_enqueue;	/* last time a buffer was enqueued */
	int last_dispatch;	/* age of the most recently dispatched buffer */
	int last_quiescent;     /*  */
	int ctxOwner;		/* last context to upload state */

	int vertex_prim;
} GLINTSAREADRIRec, *GLINTSAREADRIPtr;

/* 
 * Glint specific record passed back to client driver 
 * via DRIGetDeviceInfo request
 */
typedef struct {
    drmRegion	registers0;
    drmRegion	registers1;
    drmRegion	registers2;
    drmRegion	registers3;
    int			numMultiDevices;
    int			pprod;
    int			cpp;
    int			frontOffset;
    int			frontPitch;
    int			backOffset;
    int			backPitch;
    int			backX;
    int			backY;
    int			depthOffset;
    int			depthPitch;
    int			textureSize;
    int 		logTextureGranularity;
} GLINTDRIRec, *GLINTDRIPtr;

#define GLINT_DRI_BUF_COUNT 256
#define GLINT_DRI_BUF_SIZE  4096

#define GAMMA_NR_TEX_REGIONS 64

#define DMA_WRITE(val,reg)                                            \
do {                                                                  \
    pGlint->buf2D++ = Glint##reg##Tag;                                 \
    pGlint->buf2D++ = val;                                           \
} while (0)

#endif /* _GLINT_DRI_H_ */
