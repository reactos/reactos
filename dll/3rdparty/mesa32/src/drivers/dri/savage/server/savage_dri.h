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


#ifndef __SAVAGE_DRI_H__
#define __SAVAGE_DRI_H__

#include "drm.h"

typedef struct {
   int chipset;
   int width;
   int height;
   int mem;
   int cpp;
   int zpp;

   int agpMode; /* 0 for PCI cards */

   unsigned int sarea_priv_offset;

   unsigned int bufferSize; /* size of DMA buffers */
   
   unsigned int frontbufferSize;
   unsigned int frontOffset;

   unsigned int backbufferSize;
   unsigned int backOffset;

   unsigned int depthbufferSize;
   unsigned int depthOffset;

   unsigned int textureOffset;
   unsigned int textureSize;
   int logTextureGranularity;

   /* Linear aperture */
   drm_handle_t apertureHandle;
   unsigned int apertureSize;
   unsigned int aperturePitch;    /* in byte */

   /* Status page (probably not needed, but no harm, read-only) */
   drm_handle_t statusHandle;
   unsigned int statusSize;

   /* AGP textures */
   drm_handle_t agpTextureHandle;
   unsigned int agpTextureSize;
   int logAgpTextureGranularity;

   /* Not sure about this one */
   drm_handle_t xvmcSurfHandle; /* ? */
} SAVAGEDRIRec, *SAVAGEDRIPtr;

#endif
