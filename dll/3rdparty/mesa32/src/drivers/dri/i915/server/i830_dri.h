/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/i810/i830_dri.h,v 1.4 2002/10/30 12:52:18 alanh Exp $ */

#ifndef _I830_DRI_H
#define _I830_DRI_H

#include "xf86drm.h"
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

   drmSize rotatedSize;
   drm_handle_t rotatedbuffer;

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
   int backPitch;

   int depthOffset;
   int depthPitch;

   int rotatedOffset;
   int rotatedPitch;

   int logTextureGranularity;
   int textureOffset;

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


#endif
