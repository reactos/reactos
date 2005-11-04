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
#include "xmlconfig.h"

typedef struct {
   drm_handle_t handle;
   drmSize size;
   char *map;
} intelRegion;

typedef struct 
{
   intelRegion front;
   intelRegion back;
   intelRegion depth;
   intelRegion tex;
   
   int deviceID;
   int width;
   int height;
   int mem;
   
   int cpp;         /* for front and back buffers */
   int bitsPerPixel;
   
   int fbFormat;

   int frontOffset;
   int frontPitch;

   int backOffset;
   int backPitch;

   int depthOffset;
   int depthPitch;
   
   
   int textureOffset;
   int textureSize;
   int logTextureGranularity;
   
   __DRIscreenPrivate *driScrnPriv;
   unsigned int sarea_priv_offset;

   int drmMinor;

   int irq_active;
   int allow_batchbuffer;

   /**
    * Configuration cache with default values for all contexts 
    */
   driOptionCache optionCache;
} intelScreenPrivate;


extern void
intelDestroyContext(__DRIcontextPrivate *driContextPriv);

extern GLboolean
intelUnbindContext(__DRIcontextPrivate *driContextPriv);

extern GLboolean
intelMakeCurrent(__DRIcontextPrivate *driContextPriv,
                __DRIdrawablePrivate *driDrawPriv,
                __DRIdrawablePrivate *driReadPriv);

extern void
intelSwapBuffers( __DRIdrawablePrivate *dPriv);

#endif
