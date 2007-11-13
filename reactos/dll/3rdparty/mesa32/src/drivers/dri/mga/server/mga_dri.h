/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga_dri.h,v 1.8 2002/11/29 11:06:42 eich Exp $ */

/*
 * Copyright 2000 VA Linux Systems Inc., Fremont, California.
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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *   Gareth Hughes <gareth@valinux.com>
 */

#ifndef __MGA_DRI_H__
#define __MGA_DRI_H__

#include "xf86drm.h"
#include "drm.h"
#include "mga_drm.h"

#define MGA_DEFAULT_AGP_SIZE     64
#define MGA_DEFAULT_AGP_MODE     4
#define MGA_MAX_AGP_MODE         4

/* Buffer are aligned on 4096 byte boundaries.
 */
#define MGA_BUFFER_ALIGN	0x00000fff

#ifdef __GNUC__
# define DEPRECATED  __attribute__ ((deprecated))
#else
# define DEPRECATED
#endif

#if 1
typedef struct _mgaDrmRegion {
    drm_handle_t     handle;
    unsigned int  offset;
    drmSize       size;
} mgaDrmRegion, *mgaDrmRegionPtr;
#else
#define mgaDrmRegion drmRegion
#endif

typedef struct {
   int chipset;
   int width DEPRECATED;
   int height DEPRECATED;
   int mem DEPRECATED;
   int cpp;

   int agpMode;

   unsigned int frontOffset;
   unsigned int frontPitch;

   unsigned int backOffset;
   unsigned int backPitch;

   unsigned int depthOffset;
   unsigned int depthPitch;

   unsigned int textureOffset;
   unsigned int textureSize;
   int logTextureGranularity DEPRECATED;

   /* Allow calculation of setup dma addresses.
    */
   unsigned int agpBufferOffset DEPRECATED;

   unsigned int agpTextureOffset;
   unsigned int agpTextureSize;
   int logAgpTextureGranularity DEPRECATED;

   unsigned int mAccess DEPRECATED;

   /**
    * \name DRM memory regions.
    *
    * \todo
    * Several of these fields are no longer used (and will never be used
    * again) on the client-side.  None of them, except \c registers, are used
    * on the server-side.  At some point when it is safe to do so (probably
    * for the X.org 6.9 / 7.0 release), these fields should be removed.
    */
   /*@{*/
   mgaDrmRegion registers;            /**< MMIO registers. */
   mgaDrmRegion status DEPRECATED;    /**< No longer used on the client-side. */
   mgaDrmRegion primary;              /**< Primary DMA region. */
   mgaDrmRegion buffers DEPRECATED;   /**< No longer used on the client-side. */
   /*@}*/

   unsigned int sarea_priv_offset;
} MGADRIRec, *MGADRIPtr;

#endif
