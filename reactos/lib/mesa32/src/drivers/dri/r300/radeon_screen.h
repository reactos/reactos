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

#ifndef __RADEON_SCREEN_H__
#define __RADEON_SCREEN_H__

#ifdef GLX_DIRECT_RENDERING

#include "xf86drm.h"
#include "drm.h"
#include "radeon_drm.h"
#include "dri_util.h"
#include "xmlconfig.h"

typedef struct {
	drm_handle_t handle;	/* Handle to the DRM region */
	drmSize size;		/* Size of the DRM region */
	drmAddress map;		/* Mapping of the DRM region */
} radeonRegionRec, *radeonRegionPtr;

/* chipset features */
#define RADEON_CHIP_UNREAL_R200		0
#define RADEON_CHIP_REAL_R200		1
#define RADEON_CHIP_R300		2
#define RADEON_CHIP_RV350		3
#define RADEON_CHIP_R420		4
#define RADEON_CHIP_MASK		0x0f

#define RADEON_CHIPSET_TCL		(1 << 8)

#define GET_CHIP(screen) ((screen)->chipset & RADEON_CHIP_MASK)
#define IS_FAMILY_R200(radeon) (GET_CHIP((radeon)->radeonScreen) < RADEON_CHIP_R300)
#define IS_FAMILY_R300(radeon) (GET_CHIP((radeon)->radeonScreen) >= RADEON_CHIP_R300)

#define R200_NR_TEX_HEAPS 2

typedef struct {
	int chipset;
	int cpp;
	int IsPCI;		/* Current card is a PCI card */
	int AGPMode;
	unsigned int irq;	/* IRQ number (0 means none) */

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

	radeonRegionRec mmio;
	radeonRegionRec status;
	radeonRegionRec gartTextures;

	drmBufMapPtr buffers;

	__volatile__ int32_t *scratch;

	__DRIscreenPrivate *driScreen;
	unsigned int sarea_priv_offset;
	unsigned int gart_buffer_offset;	/* offset in card memory space */
	unsigned int gart_texture_offset;	/* offset in card memory space */
	unsigned int gart_base;

	GLboolean drmSupportsCubeMaps;	/* need radeon kernel module >=1.7 */
	GLboolean drmSupportsBlendColor;	/* need radeon kernel module >= 1.11 */

	/* Configuration cache with default values for all contexts */
	driOptionCache optionCache;
} radeonScreenRec, *radeonScreenPtr;

#endif
#endif				/* __RADEON_SCREEN_H__ */
