/* $XFree86$ */ /* -*- mode: c; c-basic-offset: 3 -*- */
/*
 * Copyright 2000 Gareth Hughes
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
 * GARETH HUGHES BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *	Leif Delgass <ldelgass@retinalburn.net>
 *	José Fonseca <j_r_fonseca@yahoo.co.uk>
 */

#include "mach64_context.h"
#include "mach64_state.h"
#include "mach64_lock.h"
#include "mach64_tex.h"
#include "drirenderbuffer.h"

#if DEBUG_LOCKING
char *prevLockFile = NULL;
int   prevLockLine = 0;
#endif


/* Update the hardware state.  This is called if another context has
 * grabbed the hardware lock, which includes the X server.  This
 * function also updates the driver's window state after the X server
 * moves, resizes or restacks a window -- the change will be reflected
 * in the drawable position and clip rects.  Since the X server grabs
 * the hardware lock when it changes the window state, this routine will
 * automatically be called after such a change.
 */
void mach64GetLock( mach64ContextPtr mmesa, GLuint flags )
{
   __DRIdrawablePrivate *dPriv = mmesa->driDrawable;
   __DRIscreenPrivate *sPriv = mmesa->driScreen;
   drm_mach64_sarea_t *sarea = mmesa->sarea;
   int i;

   drmGetLock( mmesa->driFd, mmesa->hHWContext, flags );

   /* The window might have moved, so we might need to get new clip
    * rects.
    *
    * NOTE: This releases and regrabs the hw lock to allow the X server
    * to respond to the DRI protocol request for new drawable info.
    * Since the hardware state depends on having the latest drawable
    * clip rects, all state checking must be done _after_ this call.
    */
   DRI_VALIDATE_DRAWABLE_INFO( sPriv, dPriv ); 

   if ( mmesa->lastStamp != dPriv->lastStamp ) {
      mmesa->lastStamp = dPriv->lastStamp;
      if (mmesa->glCtx->DrawBuffer->_ColorDrawBufferMask[0] == BUFFER_BIT_BACK_LEFT)
         mach64SetCliprects( mmesa->glCtx, GL_BACK_LEFT );
      else
         mach64SetCliprects( mmesa->glCtx, GL_FRONT_LEFT );
      driUpdateFramebufferSize( mmesa->glCtx, dPriv );
      mach64CalcViewport( mmesa->glCtx );
   }

   mmesa->dirty |= (MACH64_UPLOAD_CONTEXT
		    | MACH64_UPLOAD_MISC
		    | MACH64_UPLOAD_CLIPRECTS);

   /* EXA render acceleration uses the texture engine, so restore it */
   mmesa->dirty |= (MACH64_UPLOAD_TEXTURE);

   if ( sarea->ctx_owner != mmesa->hHWContext ) {
      sarea->ctx_owner = mmesa->hHWContext;
      mmesa->dirty = MACH64_UPLOAD_ALL;
   }

   for ( i = mmesa->firstTexHeap ; i < mmesa->lastTexHeap ; i++ ) {
      DRI_AGE_TEXTURES( mmesa->texture_heaps[i] );
   }
}
