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
 *	Gareth Hughes <gareth@valinux.com>
 *	Leif Delgass <ldelgass@retinalburn.net>
 *	José Fonseca <j_r_fonseca@yahoo.co.uk>
 */

#ifndef __MACH64_SCREEN_H__
#define __MACH64_SCREEN_H__

#include "xmlconfig.h"

typedef struct {
   drm_handle_t handle;			/* Handle to the DRM region */
   drmSize size;			/* Size of the DRM region */
   drmAddress *map;			/* Mapping of the DRM region */
} mach64RegionRec, *mach64RegionPtr;

typedef struct {
   int chipset;
   int width;
   int height;
   int mem;
   int cpp;

   unsigned int	frontOffset;
   unsigned int frontPitch;
   unsigned int	backOffset;
   unsigned int backPitch;

   unsigned int	depthOffset;
   unsigned int depthPitch;

   int IsPCI;
   int AGPMode;
   unsigned int irq;			/* IRQ number (0 means none) */

   /* Shared Texture data */
   int firstTexHeap, numTexHeaps;
   int texOffset[MACH64_NR_TEX_HEAPS];
   int texSize[MACH64_NR_TEX_HEAPS];
   int logTexGranularity[MACH64_NR_TEX_HEAPS];

   mach64RegionRec mmio;
   mach64RegionRec agpTextures;

   drmBufMapPtr buffers;

   __DRIscreenPrivate *driScreen;

   driOptionCache optionCache;
} mach64ScreenRec, *mach64ScreenPtr;

#endif /* __MACH64_SCREEN_H__ */
