/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/i810/i830_dri.h,v 1.5 2002/12/10 01:27:05 dawes Exp $ */

#ifndef _I830_DRI_H
#define _I830_DRI_H

#include "drm.h"		/* HACK!!! why doesn't xf86drm.h work??? */
/* #include "xf86drm.h" */
#include "i830_common.h"

#define I830_MAX_DRAWABLES 256

#define I830_MAJOR_VERSION 1
#define I830_MINOR_VERSION 3
#define I830_PATCHLEVEL 0

#define I830_REG_SIZE 0x80000

typedef struct _I830DRIRec {
   drm_handle_t regs;
   drmSize regsSize;

   drmSize backbufferSize;
   drm_handle_t backbuffer;

   drmSize depthbufferSize;
   drm_handle_t depthbuffer;

   drm_handle_t textures;
   int textureSize;

   drm_handle_t agp_buffers;
   drmSize agp_buf_size;

   int deviceID;
   int width;
   int height;
   int mem;
   int cpp;
   int bitsPerPixel;
   int fbOffset;
   int fbStride;

   int backOffset;
   int depthOffset;

   int auxPitch;
   int auxPitchBits;

   int logTextureGranularity;
   int textureOffset;

   /* For non-dma direct rendering.
    */
   int ringOffset;
   int ringSize;

   drmBufMapPtr drmBufs;
   int irq;
   int sarea_priv_offset;
} I830DRIRec, *I830DRIPtr;

typedef struct {
   /* Nothing here yet */
   int dummy;
} I830ConfigPrivRec, *I830ConfigPrivPtr;

typedef struct {
   /* Nothing here yet */
   int dummy;
} I830DRIContextRec, *I830DRIContextPtr;

/* Warning: If you change the SAREA structure you must change the kernel
 * structure as well */

typedef struct _I830SAREA {
   unsigned int ContextState[I830_CTX_SETUP_SIZE];
   unsigned int BufferState[I830_DEST_SETUP_SIZE];
   unsigned int TexState[I830_TEXTURE_COUNT][I830_TEX_SETUP_SIZE];
   unsigned int TexBlendState[I830_TEXBLEND_COUNT][I830_TEXBLEND_SIZE];
   unsigned int TexBlendStateWordsUsed[I830_TEXBLEND_COUNT];
   unsigned int Palette[2][256];
   unsigned int dirty;

   unsigned int nbox;
   drm_clip_rect_t boxes[I830_NR_SAREA_CLIPRECTS];

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

   drmTextureRegion texList[I830_NR_TEX_REGIONS + 1];
   /* Last elt is sentinal */
   int texAge;				/* last time texture was uploaded */
   int last_enqueue;			/* last time a buffer was enqueued */
   int last_dispatch;			/* age of the most recently dispatched buffer */
   int last_quiescent;			/*  */
   int ctxOwner;			/* last context to upload state */

   int vertex_prim;

   int pf_enabled;                  /* is pageflipping allowed? */
   int pf_active;                   /* is pageflipping active right now? */
   int pf_current_page; 	    /* which buffer is being displayed? */

   int perf_boxes;             /* performance boxes to be displayed */

   /* Here's the state for texunits 2,3:
    */
   unsigned int TexState2[I830_TEX_SETUP_SIZE];
   unsigned int TexBlendState2[I830_TEXBLEND_SIZE];
   unsigned int TexBlendStateWordsUsed2;

   unsigned int TexState3[I830_TEX_SETUP_SIZE];
   unsigned int TexBlendState3[I830_TEXBLEND_SIZE];
   unsigned int TexBlendStateWordsUsed3;
   
   unsigned int StippleState[I830_STP_SETUP_SIZE];
} I830SAREARec, *I830SAREAPtr;

/* Flags for perf_boxes
 */
#define I830_BOX_RING_EMPTY    0x1 /* populated by kernel */
#define I830_BOX_FLIP          0x2 /* populated by kernel */
#define I830_BOX_WAIT          0x4 /* populated by kernel & client */
#define I830_BOX_TEXTURE_LOAD  0x8 /* populated by kernel */
#define I830_BOX_LOST_CONTEXT  0x10 /* populated by client */

#endif
