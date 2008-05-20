/* $XFree86$ */
/**************************************************************************

Copyright 2003 Eric Anholt
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ERIC ANHOLT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Eric Anholt <anholt@FreeBSD.org>
 */

#include "context.h"
#include "sis_context.h"
#include "sis_lock.h"
#include "sis_dd.h"
#include "sis_state.h"
#include "drirenderbuffer.h"

/* Update the hardware state.  This is called if another context has
 * grabbed the hardware lock, which includes the X server.  This
 * function also updates the driver's window state after the X server
 * moves, resizes or restacks a window -- the change will be reflected
 * in the drawable position and clip rects.  Since the X server grabs
 * the hardware lock when it changes the window state, this routine will
 * automatically be called after such a change.
 */
void
sisGetLock( sisContextPtr smesa, GLuint flags )
{
   __DRIdrawablePrivate *dPriv = smesa->driDrawable;
   __DRIscreenPrivate *sPriv = smesa->driScreen;
   SISSAREAPrivPtr sarea = smesa->sarea;

   drmGetLock( smesa->driFd, smesa->hHWContext, flags );

   /* The window might have moved, so we might need to get new clip
    * rects.
    *
    * NOTE: This releases and regrabs the hw lock to allow the X server
    * to respond to the DRI protocol request for new drawable info.
    * Since the hardware state depends on having the latest drawable
    * clip rects, all state checking must be done _after_ this call.
    */
   DRI_VALIDATE_DRAWABLE_INFO( sPriv, dPriv );

   if ( smesa->lastStamp != dPriv->lastStamp ) {
      sisUpdateBufferSize( smesa );
      sisUpdateClipping( smesa->glCtx );
      if (smesa->is6326)
	 sis6326DDDrawBuffer( smesa->glCtx, smesa->glCtx->Color.DrawBuffer[0] );
      else
	 sisDDDrawBuffer( smesa->glCtx, smesa->glCtx->Color.DrawBuffer[0] );
      driUpdateFramebufferSize(smesa->glCtx, dPriv);
      smesa->lastStamp = dPriv->lastStamp;
   }

   if ( sarea->CtxOwner != smesa->hHWContext ) {
      sarea->CtxOwner = smesa->hHWContext;
      smesa->GlobalFlag = GFLAG_ALL;
   }
}
