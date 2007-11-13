/* * $XFree86: xc/programs/Xserver/hw/xfree86/drivers/sis/sis_common.h,v 1.1 2003/08/29 08:52:12 twini Exp $ */
/*
 * Common header definitions for SiS 2D/3D/DRM suite
 *
 * Copyright (C) 2003 Eric Anholt
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holder not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The copyright holder makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:
 *   	Eric Anholt <anholt@FreeBSD.org>
 *
 */

#ifndef _SIS_COMMON_H_
#define _SIS_COMMON_H_

#define DRM_SIS_FB_ALLOC	0x04
#define DRM_SIS_FB_FREE		0x05
#define DRM_SIS_FLIP		0x08
#define DRM_SIS_FLIP_INIT	0x09
#define DRM_SIS_FLIP_FINAL	0x10
#define DRM_SIS_AGP_INIT	0x13
#define DRM_SIS_AGP_ALLOC	0x14
#define DRM_SIS_AGP_FREE	0x15
#define DRM_SIS_FB_INIT		0x16

typedef struct {
  	int context;
  	unsigned long offset;
  	unsigned long size;
  	void *free;
} drm_sis_mem_t;

typedef struct {
  	unsigned long offset, size;
} drm_sis_agp_t;

typedef struct {
  	unsigned long offset, size;
} drm_sis_fb_t;

typedef struct {
  	unsigned int left, right;
} drm_sis_flip_t;

#endif /* _SIS_COMMON_H_ */

