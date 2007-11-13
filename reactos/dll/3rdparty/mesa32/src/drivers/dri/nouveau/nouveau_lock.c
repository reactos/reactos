/**************************************************************************

Copyright 2006 Stephane Marchesin
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
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/


#include "nouveau_context.h"
#include "nouveau_lock.h"

#include "drirenderbuffer.h"
#include "framebuffer.h"


/* Update the hardware state.  This is called if another context has
 * grabbed the hardware lock, which includes the X server.  This
 * function also updates the driver's window state after the X server
 * moves, resizes or restacks a window -- the change will be reflected
 * in the drawable position and clip rects.  Since the X server grabs
 * the hardware lock when it changes the window state, this routine will
 * automatically be called after such a change.
 */
void nouveauGetLock( nouveauContextPtr nmesa, GLuint flags )
{
   __DRIdrawablePrivate *dPriv = nmesa->driDrawable;
   __DRIscreenPrivate *sPriv = nmesa->driScreen;
   drm_nouveau_sarea_t *sarea = nmesa->sarea;

   drmGetLock( nmesa->driFd, nmesa->hHWContext, flags );

   /* The window might have moved, so we might need to get new clip
    * rects.
    *
    * NOTE: This releases and regrabs the hw lock to allow the X server
    * to respond to the DRI protocol request for new drawable info.
    * Since the hardware state depends on having the latest drawable
    * clip rects, all state checking must be done _after_ this call.
    */
   DRI_VALIDATE_DRAWABLE_INFO( sPriv, dPriv );

   /* If timestamps don't match, the window has been changed */
   if (nmesa->lastStamp != dPriv->lastStamp) {
      struct gl_framebuffer *fb = (struct gl_framebuffer *)dPriv->driverPrivate;

      /* _mesa_resize_framebuffer will take care of calling the renderbuffer's
       * AllocStorage function if we need more memory to hold it */
      if (fb->Width != dPriv->w || fb->Height != dPriv->h) {
	 _mesa_resize_framebuffer(nmesa->glCtx, fb, dPriv->w, dPriv->h);
	 /* resize buffers, will call nouveau_window_moved */
	 nouveau_build_framebuffer(nmesa->glCtx, fb);
      } else {
	 nouveau_window_moved(nmesa->glCtx);
      }

      nmesa->lastStamp = dPriv->lastStamp;
   }

   nmesa->numClipRects = dPriv->numClipRects;
   nmesa->pClipRects = dPriv->pClipRects;

}
