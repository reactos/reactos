/* $XFree86: xc/lib/GL/mesa/src/drv/r200/r200_screen.h,v 1.1 2002/10/30 12:51:52 alanh Exp $ */
/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef __R200_SCREEN_H__
#define __R200_SCREEN_H__

#include "xf86drm.h"
#include "drm.h"
#include "radeon_drm.h"
#include "dri_util.h"
#include "xmlconfig.h"

typedef struct {
   drm_handle_t handle;			/* Handle to the DRM region */
   drmSize size;			/* Size of the DRM region */
   drmAddress map;			/* Mapping of the DRM region */
} r200RegionRec, *r200RegionPtr;


/* chipset features */
#define R200_CHIPSET_TCL	(1 << 0)
#define R200_CHIPSET_REAL_R200  (1 << 1)
#define R200_CHIPSET_YCBCR_BROKEN (1 << 2) 


#define R200_NR_TEX_HEAPS 2

typedef struct {

   int chipset;
   int cpp;
   int IsPCI;				/* Current card is a PCI card */
   int AGPMode;
   unsigned int irq;			/* IRQ number (0 means none) */

   unsigned int fbLocation;
   unsigned int frontOffset;
   unsigned int frontPitch;
   unsigned int backOffset;
   unsigned int backPitch;

   unsigned int depthOffset;
   unsigned int depthPitch;

    /* Shared texture data */
   int numTexHeaps;
   int texOffset[R200_NR_TEX_HEAPS];
   int texSize[R200_NR_TEX_HEAPS];
   int logTexGranularity[R200_NR_TEX_HEAPS];

   r200RegionRec mmio;
   r200RegionRec status;
   r200RegionRec gartTextures;

   drmBufMapPtr buffers;

   __volatile__ int32_t *scratch;

   __DRIscreenPrivate *driScreen;
   unsigned int sarea_priv_offset;
   unsigned int gart_buffer_offset;	/* offset in card memory space */
   unsigned int gart_texture_offset;	/* offset in card memory space */
   unsigned int gart_base;

   GLboolean drmSupportsCubeMaps;       /* need radeon kernel module >=1.7 */
   GLboolean drmSupportsBlendColor;     /* need radeon kernel module >= 1.11 */
   GLboolean drmSupportsTriPerf;        /* need radeon kernel module >= 1.16 */
   GLboolean depthHasSurface;

   /* Configuration cache with default values for all contexts */
   driOptionCache optionCache;
} r200ScreenRec, *r200ScreenPtr;

#endif /* __R200_SCREEN_H__ */
