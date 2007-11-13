/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

All Rights Reserved.

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
 *   Gareth Hughes <gareth@valinux.com>
 *   Keith Whitwell <keith@tungstengraphics.com>
 *   Kevin E. Martin <martin@valinux.com>
 *   Nicolai Haehnle <prefect_@gmx.net>
 */

#ifndef __RADEON_CONTEXT_H__
#define __RADEON_CONTEXT_H__

#include "mtypes.h"
#include "radeon_screen.h"
#include "drm.h"
#include "dri_util.h"
#include "colormac.h"

struct radeon_context;
typedef struct radeon_context radeonContextRec;
typedef struct radeon_context *radeonContextPtr;

#define TEX_0   0x1
#define TEX_1   0x2
#define TEX_2	0x4
#define TEX_3	0x8
#define TEX_4	0x10
#define TEX_5	0x20
#define TEX_6	0x40
#define TEX_7	0x80
#define TEX_ALL 0xff

/* Rasterizing fallbacks */
/* See correponding strings in r200_swtcl.c */
#define RADEON_FALLBACK_TEXTURE		0x0001
#define RADEON_FALLBACK_DRAW_BUFFER	0x0002
#define RADEON_FALLBACK_STENCIL		0x0004
#define RADEON_FALLBACK_RENDER_MODE	0x0008
#define RADEON_FALLBACK_BLEND_EQ	0x0010
#define RADEON_FALLBACK_BLEND_FUNC	0x0020
#define RADEON_FALLBACK_DISABLE		0x0040
#define RADEON_FALLBACK_BORDER_MODE	0x0080

#if R200_MERGED
extern void radeonFallback(GLcontext * ctx, GLuint bit, GLboolean mode);

#define FALLBACK( radeon, bit, mode ) do {			\
   if ( 0 ) fprintf( stderr, "FALLBACK in %s: #%d=%d\n",	\
		     __FUNCTION__, bit, mode );			\
   radeonFallback( (radeon)->glCtx, bit, mode );		\
} while (0)
#else
#define FALLBACK( radeon, bit, mode ) fprintf(stderr, "%s:%s\n", __LINE__, __FILE__);
#endif

/* TCL fallbacks */
extern void radeonTclFallback(GLcontext * ctx, GLuint bit, GLboolean mode);

#define RADEON_TCL_FALLBACK_RASTER		0x0001	/* rasterization */
#define RADEON_TCL_FALLBACK_UNFILLED		0x0002	/* unfilled tris */
#define RADEON_TCL_FALLBACK_LIGHT_TWOSIDE	0x0004	/* twoside tris */
#define RADEON_TCL_FALLBACK_MATERIAL		0x0008	/* material in vb */
#define RADEON_TCL_FALLBACK_TEXGEN_0		0x0010	/* texgen, unit 0 */
#define RADEON_TCL_FALLBACK_TEXGEN_1		0x0020	/* texgen, unit 1 */
#define RADEON_TCL_FALLBACK_TEXGEN_2		0x0040	/* texgen, unit 2 */
#define RADEON_TCL_FALLBACK_TEXGEN_3		0x0080	/* texgen, unit 3 */
#define RADEON_TCL_FALLBACK_TEXGEN_4		0x0100	/* texgen, unit 4 */
#define RADEON_TCL_FALLBACK_TEXGEN_5		0x0200	/* texgen, unit 5 */
#define RADEON_TCL_FALLBACK_TCL_DISABLE		0x0400	/* user disable */
#define RADEON_TCL_FALLBACK_BITMAP		0x0800	/* draw bitmap with points */
#define RADEON_TCL_FALLBACK_VERTEX_PROGRAM	0x1000	/* vertex program active */

#if R200_MERGED
#define TCL_FALLBACK( ctx, bit, mode )	radeonTclFallback( ctx, bit, mode )
#else
#define TCL_FALLBACK( ctx, bit, mode )	;
#endif

struct radeon_dri_mirror {
	__DRIcontextPrivate *context;	/* DRI context */
	__DRIscreenPrivate *screen;	/* DRI screen */
	/**
	 * DRI drawable bound to this context for drawing.
	 */
	__DRIdrawablePrivate *drawable;

	/**
	 * DRI drawable bound to this context for reading.
	 */
	__DRIdrawablePrivate *readable;

	drm_context_t hwContext;
	drm_hw_lock_t *hwLock;
	int fd;
	int drmMinor;
};

/**
 * Derived state for internal purposes.
 */
struct radeon_scissor_state {
	drm_clip_rect_t rect;
	GLboolean enabled;

	GLuint numClipRects;	/* Cliprects active */
	GLuint numAllocedClipRects;	/* Cliprects available */
	drm_clip_rect_t *pClipRects;
};

struct radeon_colorbuffer_state {
	GLuint clear;
	GLint drawOffset, drawPitch;
};

struct radeon_state {
	struct radeon_colorbuffer_state color;
	struct radeon_scissor_state scissor;
};

/**
 * Common per-context variables shared by R200 and R300.
 * R200- and R300-specific code "derive" their own context from this
 * structure.
 */
struct radeon_context {
	GLcontext *glCtx;	/* Mesa context */
	radeonScreenPtr radeonScreen;	/* Screen private DRI data */

	/* Fallback state */
	GLuint Fallback;
	GLuint TclFallback;

	/* Page flipping */
	GLuint doPageFlip;

	/* Drawable, cliprect and scissor information */
	GLuint numClipRects;	/* Cliprects for the draw buffer */
	drm_clip_rect_t *pClipRects;
	unsigned int lastStamp;
	GLboolean lost_context;
	drm_radeon_sarea_t *sarea;	/* Private SAREA data */

	/* Mirrors of some DRI state */
	struct radeon_dri_mirror dri;

	/* Busy waiting */
	GLuint do_usleeps;
	GLuint do_irqs;
	GLuint irqsEmitted;
	drm_radeon_irq_wait_t iw;

	/* VBI / buffer swap */
	GLuint vbl_seq;
	GLuint vblank_flags;

	int64_t swap_ust;
	int64_t swap_missed_ust;

	GLuint swap_count;
	GLuint swap_missed_count;

	/* Derived state */
	struct radeon_state state;

	/* Configuration cache
	 */
	driOptionCache optionCache;
};

#define RADEON_CONTEXT(glctx) ((radeonContextPtr)(ctx->DriverCtx))

extern void radeonSwapBuffers(__DRIdrawablePrivate * dPriv);
extern void radeonCopySubBuffer(__DRIdrawablePrivate * dPriv,
				int x, int y, int w, int h);
extern GLboolean radeonInitContext(radeonContextPtr radeon,
				   struct dd_function_table *functions,
				   const __GLcontextModes * glVisual,
				   __DRIcontextPrivate * driContextPriv,
				   void *sharedContextPrivate);
extern void radeonCleanupContext(radeonContextPtr radeon);
extern GLboolean radeonMakeCurrent(__DRIcontextPrivate * driContextPriv,
				   __DRIdrawablePrivate * driDrawPriv,
				   __DRIdrawablePrivate * driReadPriv);
extern GLboolean radeonUnbindContext(__DRIcontextPrivate * driContextPriv);

/* ================================================================
 * Debugging:
 */
#define DO_DEBUG		1

#if DO_DEBUG
extern int RADEON_DEBUG;
#else
#define RADEON_DEBUG		0
#endif

#define DEBUG_TEXTURE	0x0001
#define DEBUG_STATE	0x0002
#define DEBUG_IOCTL	0x0004
#define DEBUG_PRIMS	0x0008
#define DEBUG_VERTS	0x0010
#define DEBUG_FALLBACKS	0x0020
#define DEBUG_VFMT	0x0040
#define DEBUG_CODEGEN	0x0080
#define DEBUG_VERBOSE	0x0100
#define DEBUG_DRI       0x0200
#define DEBUG_DMA       0x0400
#define DEBUG_SANITY    0x0800
#define DEBUG_SYNC      0x1000
#define DEBUG_PIXEL     0x2000
#define DEBUG_MEMORY    0x4000

#endif				/* __RADEON_CONTEXT_H__ */
