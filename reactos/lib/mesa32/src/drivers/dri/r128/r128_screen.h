/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_screen.h,v 1.7 2002/12/16 16:18:53 dawes Exp $ */
/**************************************************************************

Copyright 1999, 2000 ATI Technologies Inc. and Precision Insight, Inc.,
                                               Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Gareth Hughes <gareth@valinux.com>
 *   Kevin E. Martin <martin@valinux.com>
 *
 */

#ifndef __R128_SCREEN_H__
#define __R128_SCREEN_H__

#include "xmlconfig.h"

typedef struct {
   drm_handle_t handle;			/* Handle to the DRM region */
   drmSize size;			/* Size of the DRM region */
   unsigned char *map;			/* Mapping of the DRM region */
} r128RegionRec, *r128RegionPtr;

typedef struct {

   GLint chipset;
   GLint cpp;
   GLint IsPCI;				/* Current card is a PCI card */
   GLint AGPMode;
   unsigned int irq;			/* IRQ number (0 means none) */

   GLuint frontOffset;
   GLuint frontPitch;
   GLuint backOffset;
   GLuint backPitch;

   GLuint depthOffset;
   GLuint depthPitch;
   GLuint spanOffset;

    /* Shared texture data */
   GLint numTexHeaps;
   GLint texOffset[R128_NR_TEX_HEAPS];
   GLint texSize[R128_NR_TEX_HEAPS];
   GLint logTexGranularity[R128_NR_TEX_HEAPS];

   r128RegionRec mmio;
   r128RegionRec agpTextures;

   drmBufMapPtr buffers;

   __DRIscreenPrivate *driScreen;
   unsigned int sarea_priv_offset;

   /* Configuration cache with default values for all contexts */
   driOptionCache optionCache;

} r128ScreenRec, *r128ScreenPtr;


#endif /* __R128_SCREEN_H__ */
