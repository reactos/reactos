/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/i810/i810_dri.h,v 1.10 2002/12/10 01:27:04 dawes Exp $ */

#ifndef _I810_DRI_
#define _I810_DRI_

#include "xf86drm.h"
#include "i810_common.h"

#define I810_MAX_DRAWABLES 256

typedef struct {
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
   unsigned int sarea_priv_offset;

} I810DRIRec, *I810DRIPtr;

/* WARNING: Do not change the SAREA structure without changing the kernel
 * as well */

#define I810_UPLOAD_TEX0IMAGE  0x1	/* handled clientside */
#define I810_UPLOAD_TEX1IMAGE  0x2	/* handled clientside */
#define I810_UPLOAD_CTX        0x4
#define I810_UPLOAD_BUFFERS    0x8
#define I810_UPLOAD_TEX0       0x10
#define I810_UPLOAD_TEX1       0x20
#define I810_UPLOAD_CLIPRECTS  0x40

typedef struct {
   unsigned char next, prev;		/* indices to form a circular LRU  */
   unsigned char in_use;		/* owned by a client, or free? */
   int age;				/* tracked by clients to update local LRU's */
} I810TexRegionRec, *I810TexRegionPtr;

typedef struct {
   unsigned int ContextState[I810_CTX_SETUP_SIZE];
   unsigned int BufferState[I810_DEST_SETUP_SIZE];
   unsigned int TexState[2][I810_TEX_SETUP_SIZE];
   unsigned int dirty;

   unsigned int nbox;
   drm_clip_rect_t boxes[I810_NR_SAREA_CLIPRECTS];

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
 
   drmTextureRegion texList[I810_NR_TEX_REGIONS + 1];

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


} I810SAREARec, *I810SAREAPtr;

typedef struct {
   /* Nothing here yet */
   int dummy;
} I810ConfigPrivRec, *I810ConfigPrivPtr;

typedef struct {
   /* Nothing here yet */
   int dummy;
} I810DRIContextRec, *I810DRIContextPtr;

#endif
