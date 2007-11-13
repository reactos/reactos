/**************************************************************************

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

#include "glheader.h"
#include "imports.h"
#include "api_arrayelt.h"
#include "enums.h"
#include "colormac.h"
#include "light.h"

#include "swrast/swrast.h"
#include "vbo/vbo.h"
#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "swrast_setup/swrast_setup.h"

#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "r300_ioctl.h"
#include "framebuffer.h"

/* =============================================================
 * Scissoring
 */

static GLboolean intersect_rect(drm_clip_rect_t * out,
				drm_clip_rect_t * a, drm_clip_rect_t * b)
{
	*out = *a;
	if (b->x1 > out->x1)
		out->x1 = b->x1;
	if (b->y1 > out->y1)
		out->y1 = b->y1;
	if (b->x2 < out->x2)
		out->x2 = b->x2;
	if (b->y2 < out->y2)
		out->y2 = b->y2;
	if (out->x1 >= out->x2)
		return GL_FALSE;
	if (out->y1 >= out->y2)
		return GL_FALSE;
	return GL_TRUE;
}

void radeonRecalcScissorRects(radeonContextPtr radeon)
{
	drm_clip_rect_t *out;
	int i;

	/* Grow cliprect store?
	 */
	if (radeon->state.scissor.numAllocedClipRects < radeon->numClipRects) {
		while (radeon->state.scissor.numAllocedClipRects <
		       radeon->numClipRects) {
			radeon->state.scissor.numAllocedClipRects += 1;	/* zero case */
			radeon->state.scissor.numAllocedClipRects *= 2;
		}

		if (radeon->state.scissor.pClipRects)
			FREE(radeon->state.scissor.pClipRects);

		radeon->state.scissor.pClipRects =
		    MALLOC(radeon->state.scissor.numAllocedClipRects *
			   sizeof(drm_clip_rect_t));

		if (radeon->state.scissor.pClipRects == NULL) {
			radeon->state.scissor.numAllocedClipRects = 0;
			return;
		}
	}

	out = radeon->state.scissor.pClipRects;
	radeon->state.scissor.numClipRects = 0;

	for (i = 0; i < radeon->numClipRects; i++) {
		if (intersect_rect(out,
				   &radeon->pClipRects[i],
				   &radeon->state.scissor.rect)) {
			radeon->state.scissor.numClipRects++;
			out++;
		}
	}
}

void radeonUpdateScissor(GLcontext* ctx)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);

	if (radeon->dri.drawable) {
		__DRIdrawablePrivate *dPriv = radeon->dri.drawable;
		int x1 = dPriv->x + ctx->Scissor.X;
		int y1 = dPriv->y + dPriv->h - (ctx->Scissor.Y + ctx->Scissor.Height);

		radeon->state.scissor.rect.x1 = x1;
		radeon->state.scissor.rect.y1 = y1;
		radeon->state.scissor.rect.x2 = x1 + ctx->Scissor.Width - 1;
		radeon->state.scissor.rect.y2 = y1 + ctx->Scissor.Height - 1;

		radeonRecalcScissorRects(radeon);
	}
}

static void radeonScissor(GLcontext* ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
	if (ctx->Scissor.Enabled) {
		/* We don't pipeline cliprect changes */
		r300Flush(ctx);
		radeonUpdateScissor(ctx);
	}
}


/**
 * Update cliprects and scissors.
 */
void radeonSetCliprects(radeonContextPtr radeon)
{
	__DRIdrawablePrivate *const drawable = radeon->dri.drawable;
	__DRIdrawablePrivate *const readable = radeon->dri.readable;
	GLframebuffer *const draw_fb = (GLframebuffer*)drawable->driverPrivate;
	GLframebuffer *const read_fb = (GLframebuffer*)readable->driverPrivate;

	if (draw_fb->_ColorDrawBufferMask[0] == BUFFER_BIT_BACK_LEFT) {
		/* Can't ignore 2d windows if we are page flipping. */
		if (drawable->numBackClipRects == 0 || radeon->doPageFlip ||
		    radeon->sarea->pfCurrentPage == 1) {
			radeon->numClipRects = drawable->numClipRects;
			radeon->pClipRects = drawable->pClipRects;
		} else {
			radeon->numClipRects = drawable->numBackClipRects;
			radeon->pClipRects = drawable->pBackClipRects;
		}
	} else {
		/* front buffer (or none, or multiple buffers */
		radeon->numClipRects = drawable->numClipRects;
		radeon->pClipRects = drawable->pClipRects;
	}

	if ((draw_fb->Width != drawable->w) ||
	    (draw_fb->Height != drawable->h)) {
		_mesa_resize_framebuffer(radeon->glCtx, draw_fb,
					 drawable->w, drawable->h);
		draw_fb->Initialized = GL_TRUE;
	}

	if (drawable != readable) {
		if ((read_fb->Width != readable->w) ||
		    (read_fb->Height != readable->h)) {
			_mesa_resize_framebuffer(radeon->glCtx, read_fb,
						 readable->w, readable->h);
			read_fb->Initialized = GL_TRUE;
		}
	}

	if (radeon->state.scissor.enabled)
		radeonRecalcScissorRects(radeon);

	radeon->lastStamp = drawable->lastStamp;
}


/**
 * Handle common enable bits.
 * Called as a fallback by r200Enable/r300Enable.
 */
void radeonEnable(GLcontext* ctx, GLenum cap, GLboolean state)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);

	switch(cap) {
	case GL_SCISSOR_TEST:
		/* We don't pipeline cliprect & scissor changes */
		r300Flush(ctx);

		radeon->state.scissor.enabled = state;
		radeonUpdateScissor(ctx);
		break;

	default:
		return;
	}
}


/**
 * Initialize default state.
 * This function is called once at context init time from
 * r200InitState/r300InitState
 */
void radeonInitState(radeonContextPtr radeon)
{
	radeon->Fallback = 0;

	if (radeon->glCtx->Visual.doubleBufferMode && radeon->sarea->pfCurrentPage == 0) {
		radeon->state.color.drawOffset = radeon->radeonScreen->backOffset;
		radeon->state.color.drawPitch = radeon->radeonScreen->backPitch;
	} else {
		radeon->state.color.drawOffset = radeon->radeonScreen->frontOffset;
		radeon->state.color.drawPitch = radeon->radeonScreen->frontPitch;
	}
}


/**
 * Initialize common state functions.
 * Called by r200InitStateFuncs/r300InitStateFuncs
 */
void radeonInitStateFuncs(struct dd_function_table *functions)
{
	functions->Scissor = radeonScissor;
}
