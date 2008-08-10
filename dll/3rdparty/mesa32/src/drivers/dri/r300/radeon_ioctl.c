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

#include <sched.h>
#include <errno.h>

#include "glheader.h"
#include "imports.h"
#include "macros.h"
#include "context.h"
#include "swrast/swrast.h"
#include "r300_context.h"
#include "radeon_ioctl.h"
#include "r300_ioctl.h"
#include "r300_state.h"
#include "radeon_reg.h"

#include "drirenderbuffer.h"
#include "vblank.h"

static void radeonWaitForIdle(radeonContextPtr radeon);

/* ================================================================
 * SwapBuffers with client-side throttling
 */

static uint32_t radeonGetLastFrame(radeonContextPtr radeon)
{
	drm_radeon_getparam_t gp;
	int ret;
	uint32_t frame;

	gp.param = RADEON_PARAM_LAST_FRAME;
	gp.value = (int *)&frame;
	ret = drmCommandWriteRead(radeon->dri.fd, DRM_RADEON_GETPARAM,
				  &gp, sizeof(gp));
	if (ret) {
		fprintf(stderr, "%s: drmRadeonGetParam: %d\n", __FUNCTION__,
			ret);
		exit(1);
	}

	return frame;
}

uint32_t radeonGetAge(radeonContextPtr radeon)
{
	drm_radeon_getparam_t gp;
	int ret;
	uint32_t age;

	gp.param = RADEON_PARAM_LAST_CLEAR;
	gp.value = (int *)&age;
	ret = drmCommandWriteRead(radeon->dri.fd, DRM_RADEON_GETPARAM,
				  &gp, sizeof(gp));
	if (ret) {
		fprintf(stderr, "%s: drmRadeonGetParam: %d\n", __FUNCTION__,
			ret);
		exit(1);
	}

	return age;
}

static void radeonEmitIrqLocked(radeonContextPtr radeon)
{
	drm_radeon_irq_emit_t ie;
	int ret;

	ie.irq_seq = &radeon->iw.irq_seq;
	ret = drmCommandWriteRead(radeon->dri.fd, DRM_RADEON_IRQ_EMIT,
				  &ie, sizeof(ie));
	if (ret) {
		fprintf(stderr, "%s: drmRadeonIrqEmit: %d\n", __FUNCTION__,
			ret);
		exit(1);
	}
}

static void radeonWaitIrq(radeonContextPtr radeon)
{
	int ret;

	do {
		ret = drmCommandWrite(radeon->dri.fd, DRM_RADEON_IRQ_WAIT,
				      &radeon->iw, sizeof(radeon->iw));
	} while (ret && (errno == EINTR || errno == EBUSY));

	if (ret) {
		fprintf(stderr, "%s: drmRadeonIrqWait: %d\n", __FUNCTION__,
			ret);
		exit(1);
	}
}

static void radeonWaitForFrameCompletion(radeonContextPtr radeon)
{
	drm_radeon_sarea_t *sarea = radeon->sarea;

	if (radeon->do_irqs) {
		if (radeonGetLastFrame(radeon) < sarea->last_frame) {
			if (!radeon->irqsEmitted) {
				while (radeonGetLastFrame(radeon) <
				       sarea->last_frame) ;
			} else {
				UNLOCK_HARDWARE(radeon);
				radeonWaitIrq(radeon);
				LOCK_HARDWARE(radeon);
			}
			radeon->irqsEmitted = 10;
		}

		if (radeon->irqsEmitted) {
			radeonEmitIrqLocked(radeon);
			radeon->irqsEmitted--;
		}
	} else {
		while (radeonGetLastFrame(radeon) < sarea->last_frame) {
			UNLOCK_HARDWARE(radeon);
			if (radeon->do_usleeps)
				DO_USLEEP(1);
			LOCK_HARDWARE(radeon);
		}
	}
}

/* Copy the back color buffer to the front color buffer.
 */
void radeonCopyBuffer(const __DRIdrawablePrivate * dPriv,
		      const drm_clip_rect_t	 * rect)
{
	radeonContextPtr radeon;
	GLint nbox, i, ret;
	GLboolean missed_target;
	int64_t ust;

	assert(dPriv);
	assert(dPriv->driContextPriv);
	assert(dPriv->driContextPriv->driverPrivate);

	radeon = (radeonContextPtr) dPriv->driContextPriv->driverPrivate;

	if (RADEON_DEBUG & DEBUG_IOCTL) {
		fprintf(stderr, "\n%s( %p )\n\n", __FUNCTION__,
			(void *)radeon->glCtx);
	}

	r300Flush(radeon->glCtx);

	LOCK_HARDWARE(radeon);

	/* Throttle the frame rate -- only allow one pending swap buffers
	 * request at a time.
	 */
	radeonWaitForFrameCompletion(radeon);
	if (!rect)
	{
	    UNLOCK_HARDWARE(radeon);
	    driWaitForVBlank(dPriv, &radeon->vbl_seq, radeon->vblank_flags,
			     &missed_target);
	    LOCK_HARDWARE(radeon);
	}

	nbox = dPriv->numClipRects;	/* must be in locked region */

	for (i = 0; i < nbox;) {
		GLint nr = MIN2(i + RADEON_NR_SAREA_CLIPRECTS, nbox);
		drm_clip_rect_t *box = dPriv->pClipRects;
		drm_clip_rect_t *b = radeon->sarea->boxes;
		GLint n = 0;

		for ( ; i < nr ; i++ ) {

		    *b = box[i];

		    if (rect)
		    {
			if (rect->x1 > b->x1)
			    b->x1 = rect->x1;
			if (rect->y1 > b->y1)
			    b->y1 = rect->y1;
			if (rect->x2 < b->x2)
			    b->x2 = rect->x2;
			if (rect->y2 < b->y2)
			    b->y2 = rect->y2;

			if (b->x1 < b->x2 && b->y1 < b->y2)
			    b++;
		    }
		    else
			b++;

		    n++;
		}
		radeon->sarea->nbox = n;

		ret = drmCommandNone(radeon->dri.fd, DRM_RADEON_SWAP);

		if (ret) {
			fprintf(stderr, "DRM_RADEON_SWAP: return = %d\n",
				ret);
			UNLOCK_HARDWARE(radeon);
			exit(1);
		}
	}

	UNLOCK_HARDWARE(radeon);
	if (!rect)
	{
	    ((r300ContextPtr)radeon)->hw.all_dirty = GL_TRUE;

	    radeon->swap_count++;
	    (*dri_interface->getUST) (&ust);
	    if (missed_target) {
		radeon->swap_missed_count++;
		radeon->swap_missed_ust = ust - radeon->swap_ust;
	    }

	    radeon->swap_ust = ust;

	    sched_yield();
	}
}

void radeonPageFlip(const __DRIdrawablePrivate * dPriv)
{
	radeonContextPtr radeon;
	GLint ret;
	GLboolean missed_target;

	assert(dPriv);
	assert(dPriv->driContextPriv);
	assert(dPriv->driContextPriv->driverPrivate);

	radeon = (radeonContextPtr) dPriv->driContextPriv->driverPrivate;

	if (RADEON_DEBUG & DEBUG_IOCTL) {
		fprintf(stderr, "%s: pfCurrentPage: %d\n", __FUNCTION__,
			radeon->sarea->pfCurrentPage);
	}

	r300Flush(radeon->glCtx);
	LOCK_HARDWARE(radeon);

	if (!dPriv->numClipRects) {
		UNLOCK_HARDWARE(radeon);
		usleep(10000);	/* throttle invisible client 10ms */
		return;
	}

	/* Need to do this for the perf box placement:
	 */
	{
		drm_clip_rect_t *box = dPriv->pClipRects;
		drm_clip_rect_t *b = radeon->sarea->boxes;
		b[0] = box[0];
		radeon->sarea->nbox = 1;
	}

	/* Throttle the frame rate -- only allow a few pending swap buffers
	 * request at a time.
	 */
	radeonWaitForFrameCompletion(radeon);
	UNLOCK_HARDWARE(radeon);
	driWaitForVBlank(dPriv, &radeon->vbl_seq, radeon->vblank_flags,
			 &missed_target);
	if (missed_target) {
		radeon->swap_missed_count++;
		(void)(*dri_interface->getUST) (&radeon->swap_missed_ust);
	}
	LOCK_HARDWARE(radeon);

	ret = drmCommandNone(radeon->dri.fd, DRM_RADEON_FLIP);

	UNLOCK_HARDWARE(radeon);

	if (ret) {
		fprintf(stderr, "DRM_RADEON_FLIP: return = %d\n", ret);
		exit(1);
	}

	radeon->swap_count++;
	(void)(*dri_interface->getUST) (&radeon->swap_ust);

        driFlipRenderbuffers(radeon->glCtx->WinSysDrawBuffer, 
                             radeon->sarea->pfCurrentPage);

	if (radeon->sarea->pfCurrentPage == 1) {
		radeon->state.color.drawOffset = radeon->radeonScreen->frontOffset;
		radeon->state.color.drawPitch = radeon->radeonScreen->frontPitch;
	} else {
		radeon->state.color.drawOffset = radeon->radeonScreen->backOffset;
		radeon->state.color.drawPitch = radeon->radeonScreen->backPitch;
	}

	if (IS_R300_CLASS(radeon->radeonScreen)) {
		r300ContextPtr r300 = (r300ContextPtr)radeon;
		R300_STATECHANGE(r300, cb);
		r300->hw.cb.cmd[R300_CB_OFFSET] = r300->radeon.state.color.drawOffset + 
						r300->radeon.radeonScreen->fbLocation;
		r300->hw.cb.cmd[R300_CB_PITCH] = r300->radeon.state.color.drawPitch;
		
		if (r300->radeon.radeonScreen->cpp == 4)
			r300->hw.cb.cmd[R300_CB_PITCH] |= R300_COLOR_FORMAT_ARGB8888;
		else
			r300->hw.cb.cmd[R300_CB_PITCH] |= R300_COLOR_FORMAT_RGB565;
	
		if (r300->radeon.sarea->tiling_enabled)
			r300->hw.cb.cmd[R300_CB_PITCH] |= R300_COLOR_TILE_ENABLE;
	}
}

void radeonWaitForIdleLocked(radeonContextPtr radeon)
{
	int ret;
	int i = 0;

	do {
		ret = drmCommandNone(radeon->dri.fd, DRM_RADEON_CP_IDLE);
		if (ret)
			DO_USLEEP(1);
	} while (ret && ++i < 100);

	if (ret < 0) {
		UNLOCK_HARDWARE(radeon);
		fprintf(stderr, "Error: R300 timed out... exiting\n");
		exit(-1);
	}
}

static void radeonWaitForIdle(radeonContextPtr radeon)
{
	LOCK_HARDWARE(radeon);
	radeonWaitForIdleLocked(radeon);
	UNLOCK_HARDWARE(radeon);
}

void radeonFlush(GLcontext * ctx)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);

	if (IS_R300_CLASS(radeon->radeonScreen))
		r300Flush(ctx);
}


/* Make sure all commands have been sent to the hardware and have
 * completed processing.
 */
void radeonFinish(GLcontext * ctx)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);

	radeonFlush(ctx);

	if (radeon->do_irqs) {
		LOCK_HARDWARE(radeon);
		radeonEmitIrqLocked(radeon);
		UNLOCK_HARDWARE(radeon);
		radeonWaitIrq(radeon);
	} else
		radeonWaitForIdle(radeon);
}
