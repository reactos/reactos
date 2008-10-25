/*
 * Author: Max Lingua <sunmax@libero.it>
 */

#ifndef _S3V_DRI
#define _S3V_DRI

#include "s3v_common.h"

#define S3V_MAX_DRAWABLES (S3V_DMA_BUF_NR/2) /* 32 */ /* 256 */ /* FIXME */

typedef struct
{
   int deviceID;
   int width;
   int height;
   int mem;
   int cpp;
   int bitsPerPixel;

   int fbOffset;
   int fbStride;

   int logTextureGranularity;
   int textureOffset; 

   drm_handle_t regs;
   drmSize regsSize;

   unsigned int sarea_priv_offset;
/*
   drmAddress regsMap;

   drmSize textureSize;
   drm_handle_t textures;
*/

#if 0
   drm_handle_t agp_buffers;
   drmSize agp_buf_size;
#endif

/*
   drmBufMapPtr drmBufs;
   int irq;
   unsigned int sarea_priv_offset;
*/

/* FIXME: cleanup ! */

   drmSize            registerSize; /* == S3V_MMIO_REGSIZE */
   drm_handle_t       registerHandle;

   drmSize            pciSize;
   drm_handle_t       pciMemHandle;

   drmSize            frontSize;    /* == videoRambytes */
/* drm_handle_t       frontHandle; */
   unsigned long      frontOffset;  /* == fbOffset */
   int                frontPitch;
/* unsigned char      *front; */

   unsigned int       bufferSize; /* size of depth/back buffer */

   drmSize            backSize;
/* drm_handle_t       backHandle; */
   unsigned long      backOffset;
   int                backPitch;
/* unsigned char      *back; */

   drmSize            depthSize;
/* drm_handle_t       depthHandle; */
   unsigned long      depthOffset;
   int                depthPitch;
/* unsigned char      *depth; */

   drmSize            texSize;
/* drm_handle_t       texHandle; */
   unsigned long      texOffset;
   int                texPitch;
/* unsigned char      *tex; */

   drmSize            dmaBufSize;       /* Size of buffers (in bytes) */
   drm_handle_t       dmaBufHandle;     /* Handle from drmAddMap */
   unsigned long      dmaBufOffset;     /* Offset/Start */
   int                dmaBufPitch;      /* Pitch */
   unsigned char      *dmaBuf;          /* Map */
   int                bufNumBufs;       /* Number of buffers */
   drmBufMapPtr       buffers;          /* Buffer map */

} S3VDRIRec, *S3VDRIPtr;

/* WARNING: Do not change the SAREA structure without changing the kernel
 * as well */

typedef struct {
   unsigned char next, prev; /* indices to form a circular LRU  */
   unsigned char in_use;   /* owned by a client, or free? */
   int age;                /* tracked by clients to update local LRU's */
} S3VTexRegionRec, *S3VTexRegionPtr;

typedef struct {
   unsigned int nbox;
   drm_clip_rect_t boxes[S3V_NR_SAREA_CLIPRECTS];
   
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
   S3VTexRegionRec texList[S3V_NR_TEX_REGIONS+1]; /* Last elt is sentinal */
   
   int texAge;             /* last time texture was uploaded */
   
   int last_enqueue;       /* last time a buffer was enqueued */
   int last_dispatch;      /* age of the most recently dispatched buffer */
   int last_quiescent;     /*  */
   
   int ctxOwner;           /* last context to upload state */
} S3VSAREARec, *S3VSAREAPtr;

typedef struct {
  /* Nothing here yet */
  int dummy;
} S3VConfigPrivRec, *S3VConfigPrivPtr;

typedef struct {
  /* Nothing here yet */
  int dummy;
} S3VDRIContextRec, *S3VDRIContextPtr;


#endif
