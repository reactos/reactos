/* $XFree86$ */ /* -*- mode: c; c-basic-offset: 3 -*- */
/*
 * Copyright 2000 Gareth Hughes
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * GARETH HUGHES BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Gareth Hughes <gareth@valinux.com>
 *   Leif Delgass <ldelgass@retinalburn.net>
 */

#ifndef __MACH64_DRI_H__
#define __MACH64_DRI_H__ 1

#include "xf86drm.h"

typedef struct {
   drm_handle_t fbHandle;

   drm_handle_t regsHandle;
   drmSize regsSize;

   int IsPCI;

   drm_handle_t agpHandle;            /* Handle from drmAgpAlloc */
   unsigned long agpOffset;
   drmSize agpSize;
   int agpMode;

   /* DMA descriptor ring */
   unsigned long     ringStart;        /* Offset into AGP space */
   drm_handle_t         ringHandle;       /* Handle from drmAddMap */
   drmSize           ringMapSize;      /* Size of map */
   int               ringSize;         /* Size of ring (in kB) */
   drmAddress        ringMap;          /* Map */

   /* vertex buffer data */
   unsigned long     bufferStart;      /* Offset into AGP space */
   drm_handle_t         bufferHandle;     /* Handle from drmAddMap */
   drmSize           bufferMapSize;    /* Size of map */
   int               bufferSize;       /* Size of buffers (in MB) */
   drmAddress        bufferMap;        /* Map */

   drmBufMapPtr      drmBuffers;       /* Buffer map */
   int               numBuffers;       /* Number of buffers */

   /* AGP Texture data */
   unsigned long     agpTexStart;      /* Offset into AGP space */
   drm_handle_t         agpTexHandle;     /* Handle from drmAddMap */
   drmSize           agpTexMapSize;    /* Size of map */
   int               agpTexSize;       /* Size of AGP tex space (in MB) */
   drmAddress        agpTexMap;        /* Map */
   int               log2AGPTexGran;

   int fbX;
   int fbY;
   int backX;
   int backY;
   int depthX;
   int depthY;

   int frontOffset;
   int frontPitch;
   int backOffset;
   int backPitch;
   int depthOffset;
   int depthPitch;

   int textureOffset;
   int textureSize;
   int logTextureGranularity;
} ATIDRIServerInfoRec, *ATIDRIServerInfoPtr;

typedef struct {
   int chipset;
   int width;
   int height;
   int mem;
   int cpp;

   int IsPCI;
   int AGPMode;

   unsigned int frontOffset;
   unsigned int frontPitch;

   unsigned int backOffset;
   unsigned int backPitch;

   unsigned int depthOffset;
   unsigned int depthPitch;

   unsigned int textureOffset;
   unsigned int textureSize;
   int logTextureGranularity;

   drm_handle_t regs;
   drmSize regsSize;

   drm_handle_t agp;
   drmSize agpSize;
   unsigned int agpTextureOffset;
   unsigned int agpTextureSize;
   int logAgpTextureGranularity;
} ATIDRIRec, *ATIDRIPtr;

#endif /* __MACH64_DRI_H__ */
