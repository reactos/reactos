/* $XFree86: xc/lib/GL/mesa/src/drv/radeon/radeon_lock.c,v 1.5 2002/10/30 12:51:55 alanh Exp $ */
/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

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
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 */

#include "glheader.h"
#include "radeon_context.h"
#include "radeon_lock.h"
#include "radeon_tex.h"
#include "radeon_state.h"
#include "radeon_ioctl.h"

#if DEBUG_LOCKING
char *prevLockFile = NULL;
int prevLockLine = 0;
#endif

/* Turn on/off page flipping according to the flags in the sarea:
 */
static void
radeonUpdatePageFlipping( radeonContextPtr rmesa )
{
   int use_back;


   rmesa->doPageFlip = rmesa->sarea->pfState;

   use_back = (rmesa->glCtx->DrawBuffer->_ColorDrawBufferMask[0] == BUFFER_BIT_BACK_LEFT);
   use_back ^= (rmesa->sarea->pfCurrentPage == 1);

   if ( RADEON_DEBUG & DEBUG_VERBOSE )
      fprintf(stderr, "%s allow %d current %d\n", __FUNCTION__, 
	      rmesa->doPageFlip,
	      rmesa->sarea->pfCurrentPage );

   if ( use_back ) {
	 rmesa->state.color.drawOffset = rmesa->radeonScreen->backOffset;
	 rmesa->state.color.drawPitch  = rmesa->radeonScreen->backPitch;
   } else {
	 rmesa->state.color.drawOffset = rmesa->radeonScreen->frontOffset;
	 rmesa->state.color.drawPitch  = rmesa->radeonScreen->frontPitch;
   }

   RADEON_STATECHANGE( rmesa, ctx );
   rmesa->hw.ctx.cmd[CTX_RB3D_COLOROFFSET] = rmesa->state.color.drawOffset
					   + rmesa->radeonScreen->fbLocation;
   rmesa->hw.ctx.cmd[CTX_RB3D_COLORPITCH]  = rmesa->state.color.drawPitch;
}



/* Update the hardware state.  This is called if another context has
 * grabbed the hardware lock, which includes the X server.  This
 * function also updates the driver's window state after the X server
 * moves, resizes or restacks a window -- the change will be reflected
 * in the drawable position and clip rects.  Since the X server grabs
 * the hardware lock when it changes the window state, this routine will
 * automatically be called after such a change.
 */
void radeonGetLock( radeonContextPtr rmesa, GLuint flags )
{
   __DRIdrawablePrivate *dPriv = rmesa->dri.drawable;
   __DRIscreenPrivate *sPriv = rmesa->dri.screen;
   drm_radeon_sarea_t *sarea = rmesa->sarea;

   drmGetLock( rmesa->dri.fd, rmesa->dri.hwContext, flags );

   /* The window might have moved, so we might need to get new clip
    * rects.
    *
    * NOTE: This releases and regrabs the hw lock to allow the X server
    * to respond to the DRI protocol request for new drawable info.
    * Since the hardware state depends on having the latest drawable
    * clip rects, all state checking must be done _after_ this call.
    */
   DRI_VALIDATE_DRAWABLE_INFO( sPriv, dPriv );

   if ( rmesa->lastStamp != dPriv->lastStamp ) {
      radeonUpdatePageFlipping( rmesa );
      if (rmesa->glCtx->DrawBuffer->_ColorDrawBufferMask[0] == BUFFER_BIT_BACK_LEFT)
         radeonSetCliprects( rmesa, GL_BACK_LEFT );
      else
         radeonSetCliprects( rmesa, GL_FRONT_LEFT );
      radeonUpdateViewportOffset( rmesa->glCtx );
      rmesa->lastStamp = dPriv->lastStamp;
   }

   RADEON_STATECHANGE( rmesa, ctx );
   if (rmesa->sarea->tiling_enabled) {
      rmesa->hw.ctx.cmd[CTX_RB3D_COLORPITCH] |= RADEON_COLOR_TILE_ENABLE;
   }
   else rmesa->hw.ctx.cmd[CTX_RB3D_COLORPITCH] &= ~RADEON_COLOR_TILE_ENABLE;

   if ( sarea->ctx_owner != rmesa->dri.hwContext ) {
      int i;
      sarea->ctx_owner = rmesa->dri.hwContext;

      for ( i = 0 ; i < rmesa->nr_heaps ; i++ ) {
	 DRI_AGE_TEXTURES( rmesa->texture_heaps[ i ] );
      }
   }

   rmesa->lost_context = GL_TRUE;
}
