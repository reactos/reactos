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
/* $XFree86: xc/lib/GL/mesa/src/drv/tdfx/tdfx_lock.c,v 1.5 2002/12/16 16:19:00 dawes Exp $ */

/*
 * Original rewrite:
 *	Gareth Hughes <gareth@valinux.com>, 29 Sep - 1 Oct 2000
 *
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *
 */

#include "tdfx_context.h"
#include "tdfx_lock.h"
#include "tdfx_state.h"
#include "tdfx_render.h"
#include "tdfx_texman.h"
#include "tdfx_tris.h"

#include "drirenderbuffer.h"


void tdfxGetLock( tdfxContextPtr fxMesa )
{
    __DRIcontextPrivate *cPriv = fxMesa->driContext;
    __DRIdrawablePrivate *const drawable = cPriv->driDrawablePriv;
    __DRIdrawablePrivate *const readable = cPriv->driReadablePriv;
    __DRIscreenPrivate *sPriv = drawable->driScreenPriv;
    TDFXSAREAPriv *saPriv = (TDFXSAREAPriv *) (((char *) sPriv->pSAREA) +
					fxMesa->fxScreen->sarea_priv_offset);
    unsigned int stamp = drawable->lastStamp;

    drmGetLock( fxMesa->driFd, fxMesa->hHWContext, 0 );

    /* This macro will update drawable's cliprects if needed */
    DRI_VALIDATE_DRAWABLE_INFO(sPriv, drawable);
    if (drawable != readable) {
	DRI_VALIDATE_DRAWABLE_INFO(sPriv, readable);
    }

    if ( saPriv->fifoOwner != fxMesa->hHWContext ) {
        fxMesa->Glide.grDRIImportFifo( saPriv->fifoPtr, saPriv->fifoRead );
    }

    if ( saPriv->ctxOwner != fxMesa->hHWContext ) {
        /* This sequence looks a little odd. Glide mirrors the state, and
	 * when you get the state you are forcing the mirror to be up to
	 * date, and then getting a copy from the mirror. You can then force
	 * that state onto the hardware when you set the state.
	 */
        void *state;
        FxI32 stateSize;
        fxMesa->Glide.grGet(GR_GLIDE_STATE_SIZE, 4, &stateSize);
        state = malloc(stateSize);
        fxMesa->Glide.grGlideGetState( state );
        fxMesa->Glide.grGlideSetState( state );
        free( state );
    }

#if 0
    if ( saPriv->texOwner != fxMesa->hHWContext ) {
        tdfxTMRestoreTextures_NoLock( fxMesa );
    }
#endif

    if ((*drawable->pStamp != stamp)
	|| (saPriv->ctxOwner != fxMesa->hHWContext)) {
       driUpdateFramebufferSize(fxMesa->glCtx, drawable);
       if (drawable != readable) {
	   driUpdateFramebufferSize(fxMesa->glCtx, readable);
       }

       tdfxUpdateClipping(fxMesa->glCtx);
       tdfxUploadClipping(fxMesa);
    }

    DEBUG_LOCK();
}
