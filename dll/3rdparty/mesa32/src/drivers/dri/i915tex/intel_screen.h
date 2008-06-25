/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef _INTEL_INIT_H_
#define _INTEL_INIT_H_

#include <sys/time.h>
#include "dri_util.h"
#include "intel_rotate.h"
#include "i830_common.h"
#include "xmlconfig.h"
#include "dri_bufpool.h"

/* XXX: change name or eliminate to avoid conflict with "struct
 * intel_region"!!!
 */
typedef struct
{
   drm_handle_t handle;
   drmSize size;                /* region size in bytes */
   char *map;                   /* memory map */
   int offset;                  /* from start of video mem, in bytes */
   int pitch;                   /* row stride, in bytes */
} intelRegion;

typedef struct
{
   intelRegion front;
   intelRegion back;
   intelRegion third;
   intelRegion rotated;
   intelRegion depth;
   intelRegion tex;

   struct intel_region *front_region;
   struct intel_region *back_region;
   struct intel_region *third_region;
   struct intel_region *depth_region;
   struct intel_region *rotated_region;

   int deviceID;
   int width;
   int height;
   int mem;                     /* unused */

   int cpp;                     /* for front and back buffers */
/*    int bitsPerPixel;   */
   int fbFormat;                /* XXX FBO: this is obsolete - remove after i830 updates */

   int logTextureGranularity;

   __DRIscreenPrivate *driScrnPriv;
   unsigned int sarea_priv_offset;

   int drmMinor;

   int irq_active;
   int allow_batchbuffer;

   struct matrix23 rotMatrix;

   int current_rotation;        /* 0, 90, 180 or 270 */
   int rotatedWidth, rotatedHeight;

   /**
   * Configuration cache with default values for all contexts
   */
   driOptionCache optionCache;
   struct _DriBufferPool *batchPool;
   struct _DriBufferPool *texPool;
   struct _DriBufferPool *regionPool;
   struct _DriBufferPool *staticPool;
   unsigned int maxBatchSize;
   GLboolean havePools;
} intelScreenPrivate;



extern GLboolean intelMapScreenRegions(__DRIscreenPrivate * sPriv);

extern void intelUnmapScreenRegions(intelScreenPrivate * intelScreen);

extern void
intelUpdateScreenFromSAREA(intelScreenPrivate * intelScreen,
                           drmI830Sarea * sarea);

extern void intelDestroyContext(__DRIcontextPrivate * driContextPriv);

extern GLboolean intelUnbindContext(__DRIcontextPrivate * driContextPriv);

extern GLboolean
intelMakeCurrent(__DRIcontextPrivate * driContextPriv,
                 __DRIdrawablePrivate * driDrawPriv,
                 __DRIdrawablePrivate * driReadPriv);

extern void intelSwapBuffers(__DRIdrawablePrivate * dPriv);

extern void
intelCopySubBuffer(__DRIdrawablePrivate * dPriv, int x, int y, int w, int h);

extern struct _DriBufferPool *driBatchPoolInit(int fd, unsigned flags,
                                               unsigned long bufSize,
                                               unsigned numBufs,
                                               unsigned checkDelayed);

extern struct intel_context *intelScreenContext(intelScreenPrivate *intelScreen);

extern void
intelUpdateScreenRotation(__DRIscreenPrivate * sPriv, drmI830Sarea * sarea);
extern GLboolean
intelCreatePools(intelScreenPrivate *intelScreen);

#endif
