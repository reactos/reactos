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

/**
 * \file radeon_context.c
 * Common context initialization.
 *
 * \author Keith Whitwell <keith@tungstengraphics.com>
 */

#include <dlfcn.h>

#include "glheader.h"
#include "imports.h"
#include "context.h"
#include "state.h"
#include "matrix.h"
#include "framebuffer.h"

#include "drivers/common/driverfuncs.h"
#include "swrast/swrast.h"

#include "radeon_screen.h"
#include "radeon_ioctl.h"
#include "radeon_macros.h"
#include "radeon_reg.h"

#include "radeon_state.h"
#include "r300_state.h"

#include "utils.h"
#include "vblank.h"
#include "xmlpool.h"		/* for symbolic values of enum-type options */

#define DRIVER_DATE "20060815"


/* Return various strings for glGetString().
 */
static const GLubyte *radeonGetString(GLcontext * ctx, GLenum name)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	static char buffer[128];

	switch (name) {
	case GL_VENDOR:
		if (IS_R300_CLASS(radeon->radeonScreen))
			return (GLubyte *) "DRI R300 Project";
		else
			return (GLubyte *) "Tungsten Graphics, Inc.";

	case GL_RENDERER:
	{
		unsigned offset;
		GLuint agp_mode = (radeon->radeonScreen->card_type==RADEON_CARD_PCI) ? 0 :
			radeon->radeonScreen->AGPMode;
		const char* chipname;

		if (IS_R300_CLASS(radeon->radeonScreen))
			chipname = "R300";
		else
			chipname = "R200";

		offset = driGetRendererString(buffer, chipname, DRIVER_DATE,
					      agp_mode);

		if (IS_R300_CLASS(radeon->radeonScreen)) {
		sprintf(&buffer[offset], " %sTCL",
			(radeon->radeonScreen->chip_flags & RADEON_CHIPSET_TCL)
			? "" : "NO-");
		} else {
			sprintf(&buffer[offset], " %sTCL",
			!(radeon->TclFallback & RADEON_TCL_FALLBACK_TCL_DISABLE)
			? "" : "NO-");
		}

		return (GLubyte *) buffer;
	}

	default:
		return NULL;
	}
}

/* Initialize the driver's misc functions.
 */
static void radeonInitDriverFuncs(struct dd_function_table *functions)
{
	functions->GetString = radeonGetString;
}


/**
 * Create and initialize all common fields of the context,
 * including the Mesa context itself.
 */
GLboolean radeonInitContext(radeonContextPtr radeon,
			    struct dd_function_table* functions,
			    const __GLcontextModes * glVisual,
			    __DRIcontextPrivate * driContextPriv,
			    void *sharedContextPrivate)
{
	__DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
	radeonScreenPtr screen = (radeonScreenPtr) (sPriv->private);
	GLcontext* ctx;
	GLcontext* shareCtx;
	int fthrottle_mode;

	/* Fill in additional standard functions. */
	radeonInitDriverFuncs(functions);

	/* Allocate and initialize the Mesa context */
	if (sharedContextPrivate)
		shareCtx = ((radeonContextPtr)sharedContextPrivate)->glCtx;
	else
		shareCtx = NULL;
	radeon->glCtx = _mesa_create_context(glVisual, shareCtx,
					    functions, (void *)radeon);
	if (!radeon->glCtx)
		return GL_FALSE;

	ctx = radeon->glCtx;
	driContextPriv->driverPrivate = radeon;

	/* DRI fields */
	radeon->dri.context = driContextPriv;
	radeon->dri.screen = sPriv;
	radeon->dri.drawable = NULL;
	radeon->dri.readable = NULL;
	radeon->dri.hwContext = driContextPriv->hHWContext;
	radeon->dri.hwLock = &sPriv->pSAREA->lock;
	radeon->dri.fd = sPriv->fd;
	radeon->dri.drmMinor = sPriv->drmMinor;

	radeon->radeonScreen = screen;
	radeon->sarea = (drm_radeon_sarea_t *) ((GLubyte *) sPriv->pSAREA +
					       screen->sarea_priv_offset);

	/* Setup IRQs */
	fthrottle_mode = driQueryOptioni(&radeon->optionCache, "fthrottle_mode");
	radeon->iw.irq_seq = -1;
	radeon->irqsEmitted = 0;
	radeon->do_irqs = (fthrottle_mode == DRI_CONF_FTHROTTLE_IRQS &&
			  radeon->radeonScreen->irq);

	radeon->do_usleeps = (fthrottle_mode == DRI_CONF_FTHROTTLE_USLEEPS);

	if (!radeon->do_irqs)
		fprintf(stderr,
			"IRQ's not enabled, falling back to %s: %d %d\n",
			radeon->do_usleeps ? "usleeps" : "busy waits",
			fthrottle_mode, radeon->radeonScreen->irq);

	radeon->vblank_flags = (radeon->radeonScreen->irq != 0)
	    ? driGetDefaultVBlankFlags(&radeon->optionCache) : VBLANK_FLAG_NO_IRQ;

	(*dri_interface->getUST) (&radeon->swap_ust);

	return GL_TRUE;
}


/**
 * Cleanup common context fields.
 * Called by r200DestroyContext/r300DestroyContext
 */
void radeonCleanupContext(radeonContextPtr radeon)
{
	/* _mesa_destroy_context() might result in calls to functions that
	 * depend on the DriverCtx, so don't set it to NULL before.
	 *
	 * radeon->glCtx->DriverCtx = NULL;
	 */

	/* free the Mesa context */
	_mesa_destroy_context(radeon->glCtx);

	if (radeon->state.scissor.pClipRects) {
		FREE(radeon->state.scissor.pClipRects);
		radeon->state.scissor.pClipRects = 0;
	}
}


/**
 * Swap front and back buffer.
 */
void radeonSwapBuffers(__DRIdrawablePrivate * dPriv)
{
	if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
		radeonContextPtr radeon;
		GLcontext *ctx;

		radeon = (radeonContextPtr) dPriv->driContextPriv->driverPrivate;
		ctx = radeon->glCtx;

		if (ctx->Visual.doubleBufferMode) {
			_mesa_notifySwapBuffers(ctx);	/* flush pending rendering comands */
			if (radeon->doPageFlip) {
				radeonPageFlip(dPriv);
			} else {
			    radeonCopyBuffer(dPriv, NULL);
			}
		}
	} else {
		/* XXX this shouldn't be an error but we can't handle it for now */
		_mesa_problem(NULL, "%s: drawable has no context!",
			      __FUNCTION__);
	}
}

void radeonCopySubBuffer(__DRIdrawablePrivate * dPriv,
			 int x, int y, int w, int h )
{
    if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
	radeonContextPtr radeon;
	GLcontext *ctx;

	radeon = (radeonContextPtr) dPriv->driContextPriv->driverPrivate;
	ctx = radeon->glCtx;

	if (ctx->Visual.doubleBufferMode) {
	    drm_clip_rect_t rect;
	    rect.x1 = x + dPriv->x;
	    rect.y1 = (dPriv->h - y - h) + dPriv->y;
	    rect.x2 = rect.x1 + w;
	    rect.y2 = rect.y1 + h;
	    _mesa_notifySwapBuffers(ctx);	/* flush pending rendering comands */
	    radeonCopyBuffer(dPriv, &rect);
	}
    } else {
	/* XXX this shouldn't be an error but we can't handle it for now */
	_mesa_problem(NULL, "%s: drawable has no context!",
		      __FUNCTION__);
    }
}

/* Force the context `c' to be the current context and associate with it
 * buffer `b'.
 */
GLboolean radeonMakeCurrent(__DRIcontextPrivate * driContextPriv,
			    __DRIdrawablePrivate * driDrawPriv,
			    __DRIdrawablePrivate * driReadPriv)
{
	if (driContextPriv) {
		radeonContextPtr radeon =
			(radeonContextPtr) driContextPriv->driverPrivate;

		if (RADEON_DEBUG & DEBUG_DRI)
			fprintf(stderr, "%s ctx %p\n", __FUNCTION__,
				radeon->glCtx);

		if (radeon->dri.drawable != driDrawPriv) {
			driDrawableInitVBlank(driDrawPriv,
					      radeon->vblank_flags,
					      &radeon->vbl_seq);
		}

		radeon->dri.readable = driReadPriv;

		if (radeon->dri.drawable != driDrawPriv ||
		    radeon->lastStamp != driDrawPriv->lastStamp) {
			radeon->dri.drawable = driDrawPriv;

			radeonSetCliprects(radeon);
			r300UpdateViewportOffset(radeon->glCtx);
		}

		_mesa_make_current(radeon->glCtx,
				    (GLframebuffer *) driDrawPriv->
				    driverPrivate,
				    (GLframebuffer *) driReadPriv->
				    driverPrivate);

		_mesa_update_state(radeon->glCtx);		

		radeonUpdatePageFlipping(radeon);
	} else {
		if (RADEON_DEBUG & DEBUG_DRI)
			fprintf(stderr, "%s ctx is null\n", __FUNCTION__);
		_mesa_make_current(0, 0, 0);
	}

	if (RADEON_DEBUG & DEBUG_DRI)
		fprintf(stderr, "End %s\n", __FUNCTION__);
	return GL_TRUE;
}

/* Force the context `c' to be unbound from its buffer.
 */
GLboolean radeonUnbindContext(__DRIcontextPrivate * driContextPriv)
{
	radeonContextPtr radeon = (radeonContextPtr) driContextPriv->driverPrivate;

	if (RADEON_DEBUG & DEBUG_DRI)
		fprintf(stderr, "%s ctx %p\n", __FUNCTION__,
			radeon->glCtx);

	return GL_TRUE;
}

