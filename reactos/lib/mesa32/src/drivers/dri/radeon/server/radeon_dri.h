/**
 * \file server/radeon_dri.h
 * \brief Radeon server-side structures.
 * 
 * \author Kevin E. Martin <martin@xfree86.org>
 * \author Rickard E. Faith <faith@valinux.com>
 */

/*
 * Copyright 2000 ATI Technologies Inc., Markham, Ontario,
 *                VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation on the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT.  IN NO EVENT SHALL ATI, VA LINUX SYSTEMS AND/OR
 * THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/radeon_dri.h,v 1.3 2002/04/24 16:20:40 martin Exp $ */

#ifndef _RADEON_DRI_
#define _RADEON_DRI_

#include "xf86drm.h"
#include "drm.h"
#include "radeon_drm.h"

/* DRI Driver defaults */
#define RADEON_DEFAULT_CP_PIO_MODE    RADEON_CSQ_PRIPIO_INDPIO
#define RADEON_DEFAULT_CP_BM_MODE     RADEON_CSQ_PRIBM_INDBM
#define RADEON_DEFAULT_AGP_MODE       1
#define RADEON_DEFAULT_AGP_FAST_WRITE 0
#define RADEON_DEFAULT_AGP_SIZE       8 /* MB (must be 2^n and > 4MB) */
#define RADEON_DEFAULT_RING_SIZE      1 /* MB (must be page aligned) */
#define RADEON_DEFAULT_BUFFER_SIZE    2 /* MB (must be page aligned) */
#define RADEON_DEFAULT_AGP_TEX_SIZE   1 /* MB (must be page aligned) */
#define RADEON_DEFAULT_CP_TIMEOUT     10000  /* usecs */
#define RADEON_DEFAULT_PAGE_FLIP      0 /* page flipping diabled */
#define RADEON_BUFFER_ALIGN           0x00000fff

/**
 * \brief Radeon DRI driver private data.
 */
typedef struct {
    /**
     * \name DRI screen private data
     */
    /*@{*/
    int           deviceID;	 /**< \brief PCI device ID */
    int           width;	 /**< \brief width in pixels of display */
    int           height;	 /**< \brief height in scanlines of display */
    int           depth;	 /**< \brief depth of display (8, 15, 16, 24) */
    int           bpp;		 /**< \brief bit depth of display (8, 16, 24, 32) */

    int           IsPCI;	 /**< \brief is current card a PCI card? */
    int           AGPMode;	 /**< \brief AGP mode */

    int           frontOffset;   /**< \brief front buffer offset */
    int           frontPitch;	 /**< \brief front buffer pitch */
    int           backOffset;    /**< \brief shared back buffer offset */
    int           backPitch;     /**< \brief shared back buffer pitch */
    int           depthOffset;   /**< \brief shared depth buffer offset */
    int           depthPitch;    /**< \brief shared depth buffer pitch */
    int           textureOffset; /**< \brief start of texture data in frame buffer */
    int           textureSize;   /**< \brief size of texture date */
    int           log2TexGran;   /**< \brief log2 texture granularity */
    /*@}*/

    /**
     * \name MMIO register data
     */
    /*@{*/
    drm_handle_t     registerHandle; /**< \brief MMIO register map size */
    drmSize       registerSize;   /**< \brief MMIO register map handle */
    /*@}*/

    /**
     * \name CP in-memory status information
     */
    /*@{*/
    drm_handle_t     statusHandle;   /**< \brief status map handle */
    drmSize       statusSize;     /**< \brief status map size */
    /*@}*/

    /**
     * \name CP AGP Texture data
     */
    /*@{*/
    drm_handle_t     gartTexHandle;   /**< \brief AGP texture area map handle */
    drmSize       gartTexMapSize;  /**< \brief AGP texture area map size */
    int           log2GARTTexGran; /**< \brief AGP texture granularity in log base 2 */
    int           gartTexOffset;   /**< \brief AGP texture area offset in AGP space */
    /*@}*/

    unsigned int  sarea_priv_offset; /**< \brief offset of the private SAREA data*/
} RADEONDRIRec, *RADEONDRIPtr;

#endif
