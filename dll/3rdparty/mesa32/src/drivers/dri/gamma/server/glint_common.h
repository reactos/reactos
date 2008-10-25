/* glint_common.h -- common header definitions for Gamma 2D/3D/DRM suite
 *
 * Copyright 2002 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Converted to common header format:
 *   Jens Owen <jens@tungstengraphics.com>
 *
 * $XFree86: xc/programs/Xserver/hw/xfree86/drivers/glint/glint_common.h,v 1.2 2003/04/03 16:52:18 dawes Exp $
 *
 */

#ifndef _GLINT_COMMON_H_
#define _GLINT_COMMON_H_

/*
 * WARNING: If you change any of these defines, make sure to change
 * the kernel include file as well (gamma_drm.h)
 */

/* Driver specific DRM command indices
 * NOTE: these are not OS specific, but they are driver specific
 */
#define DRM_GAMMA_INIT                0x00
#define DRM_GAMMA_COPY                0x01

typedef struct {
   enum {
      GAMMA_INIT_DMA    = 0x01,
      GAMMA_CLEANUP_DMA = 0x02
   } func;
   int sarea_priv_offset;
   int pcimode;
   unsigned int mmio0;
   unsigned int mmio1;
   unsigned int mmio2;
   unsigned int mmio3;
   unsigned int buffers_offset;
   int num_rast;
} drmGAMMAInit;

extern int drmGAMMAInitDMA( int fd, drmGAMMAInit *info );
extern int drmGAMMACleanupDMA( int fd );

#endif
