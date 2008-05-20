/* -*- mode: c; c-basic-offset: 3 -*-
 *
 * Copyright 2000 VA Linux Systems Inc., Fremont, California.
 *
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
 * VA LINUX SYSTEMS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/tdfx/dri_glide.h,v 1.1 2001/03/21 16:14:26 dawes Exp $ */

/*
 * Original rewrite:
 *	Gareth Hughes <gareth@valinux.com>, 29 Sep - 1 Oct 2000
 *
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *
 */

#ifndef __DRI_GLIDE_H__
#define __DRI_GLIDE_H__

#include <glide.h>
#include "dri_mesaint.h"

/*
 * This is the private interface between Glide and the DRI.
 */
extern void grDRIOpen( char *pFB, char *pRegs, int deviceID,
		       int width, int height,
		       int mem, int cpp, int stride,
		       int fifoOffset, int fifoSize,
		       int fbOffset, int backOffset, int depthOffset,
		       int textureOffset, int textureSize,
		       volatile int *fifoPtr, volatile int *fifoRead );
extern void grDRIPosition( int x, int y, int w, int h,
			   int numClip, drm_clip_rect_t *pClip );
extern void grDRILostContext( void );
extern void grDRIImportFifo( int fifoPtr, int fifoRead );
extern void grDRIInvalidateAll( void );
extern void grDRIResetSAREA( void );
extern void grDRIBufferSwap( FxU32 swapInterval );
#endif
