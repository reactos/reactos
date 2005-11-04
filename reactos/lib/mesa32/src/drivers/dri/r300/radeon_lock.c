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
#include <string.h>

#include "r200_context.h"
#include "radeon_lock.h"
#include "r200_tex.h"
#include "r200_state.h"
#include "radeon_ioctl.h"
#include "radeon_state.h"

#include "framebuffer.h"

#if DEBUG_LOCKING
char *prevLockFile = NULL;
int prevLockLine = 0;
#endif

/* Turn on/off page flipping according to the flags in the sarea:
 */
static void radeonUpdatePageFlipping(radeonContextPtr radeon)
{
	int use_back;

	radeon->doPageFlip = radeon->sarea->pfState;

	use_back = (radeon->glCtx->DrawBuffer->_ColorDrawBufferMask[0] == BUFFER_BIT_BACK_LEFT);
	use_back ^= (radeon->sarea->pfCurrentPage == 1);

	if (use_back) {
		radeon->state.color.drawOffset = radeon->radeonScreen->backOffset;
		radeon->state.color.drawPitch = radeon->radeonScreen->backPitch;
	} else {
		radeon->state.color.drawOffset = radeon->radeonScreen->frontOffset;
		radeon->state.color.drawPitch = radeon->radeonScreen->frontPitch;
	}
}

/**
 * Called by radeonGetLock() after the lock has been obtained.
 */
#if R200_MERGED
static void r200RegainedLock(r200ContextPtr r200)
{
	__DRIdrawablePrivate *dPriv = r200->radeon.dri.drawable;
	int i;

	if (r200->radeon.lastStamp != dPriv->lastStamp) {
		radeonUpdatePageFlipping(&r200->radeon);
		R200_STATECHANGE(r200, ctx);
		r200->hw.ctx.cmd[CTX_RB3D_COLOROFFSET] =
			r200->radeon.state.color.drawOffset
			+ r200->radeon.radeonScreen->fbLocation;
		r200->hw.ctx.cmd[CTX_RB3D_COLORPITCH] =
			r200->radeon.state.color.drawPitch;

		if (r200->radeon.glCtx->DrawBuffer->_ColorDrawBufferMask[0] == BUFFER_BIT_BACK_LEFT)
			radeonSetCliprects(&r200->radeon, GL_BACK_LEFT);
		else
			radeonSetCliprects(&r200->radeon, GL_FRONT_LEFT);
		r200UpdateViewportOffset(r200->radeon.glCtx);
		r200->radeon.lastStamp = dPriv->lastStamp;
	}

	for (i = 0; i < r200->nr_heaps; i++) {
		DRI_AGE_TEXTURES(r200->texture_heaps[i]);
	}
}
#endif

static void r300RegainedLock(radeonContextPtr radeon)
{
	__DRIdrawablePrivate *dPriv = radeon->dri.drawable;

	if (radeon->lastStamp != dPriv->lastStamp) {
		_mesa_resize_framebuffer(radeon->glCtx,
			(GLframebuffer*)dPriv->driverPrivate,
			dPriv->w, dPriv->h);

		radeonUpdatePageFlipping(radeon);

		if (radeon->glCtx->DrawBuffer->_ColorDrawBufferMask[0] == BUFFER_BIT_BACK_LEFT)
			radeonSetCliprects(radeon, GL_BACK_LEFT);
		else
			radeonSetCliprects(radeon, GL_FRONT_LEFT);

		radeonUpdateScissor(radeon->glCtx);
		radeon->lastStamp = dPriv->lastStamp;
	}

#if R200_MERGED
	for (i = 0; i < r200->nr_heaps; i++) {
		DRI_AGE_TEXTURES(r200->texture_heaps[i]);
	}
#endif
}

/* Update the hardware state.  This is called if another context has
 * grabbed the hardware lock, which includes the X server.  This
 * function also updates the driver's window state after the X server
 * moves, resizes or restacks a window -- the change will be reflected
 * in the drawable position and clip rects.  Since the X server grabs
 * the hardware lock when it changes the window state, this routine will
 * automatically be called after such a change.
 */
void radeonGetLock(radeonContextPtr radeon, GLuint flags)
{
	__DRIdrawablePrivate *dPriv = radeon->dri.drawable;
	__DRIscreenPrivate *sPriv = radeon->dri.screen;
	drm_radeon_sarea_t *sarea = radeon->sarea;

	drmGetLock(radeon->dri.fd, radeon->dri.hwContext, flags);

	/* The window might have moved, so we might need to get new clip
	 * rects.
	 *
	 * NOTE: This releases and regrabs the hw lock to allow the X server
	 * to respond to the DRI protocol request for new drawable info.
	 * Since the hardware state depends on having the latest drawable
	 * clip rects, all state checking must be done _after_ this call.
	 */
	DRI_VALIDATE_DRAWABLE_INFO(sPriv, dPriv);

	if (sarea->ctx_owner != radeon->dri.hwContext)
		sarea->ctx_owner = radeon->dri.hwContext;

	if (IS_FAMILY_R300(radeon))
		r300RegainedLock(radeon);
#if R200_MERGED
	else
		r200RegainedLock((r200ContextPtr)radeon);
#endif
	
	radeon->lost_context = GL_TRUE;
}
