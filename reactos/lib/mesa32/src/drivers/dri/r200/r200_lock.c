/* $XFree86: xc/lib/GL/mesa/src/drv/r200/r200_lock.c,v 1.1 2002/10/30 12:51:52 alanh Exp $ */
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
#include "r200_lock.h"
#include "r200_tex.h"
#include "r200_state.h"
#include "r200_ioctl.h"

#if DEBUG_LOCKING
char *prevLockFile = NULL;
int prevLockLine = 0;
#endif

/* Turn on/off page flipping according to the flags in the sarea:
 */
static void
r200UpdatePageFlipping( r200ContextPtr rmesa )
{
   int use_back;
   rmesa->doPageFlip = rmesa->sarea->pfState;

   use_back = (rmesa->glCtx->DrawBuffer->_ColorDrawBufferMask[0] == BUFFER_BIT_BACK_LEFT);
   use_back ^= (rmesa->sarea->pfCurrentPage == 1);
   
   if (use_back) {
	 rmesa->state.color.drawOffset = rmesa->r200Screen->backOffset;
	 rmesa->state.color.drawPitch  = rmesa->r200Screen->backPitch;
   } else {
	 rmesa->state.color.drawOffset = rmesa->r200Screen->frontOffset;
	 rmesa->state.color.drawPitch  = rmesa->r200Screen->frontPitch;
   }

   R200_STATECHANGE( rmesa, ctx );
   rmesa->hw.ctx.cmd[CTX_RB3D_COLOROFFSET] = rmesa->state.color.drawOffset
					   + rmesa->r200Screen->fbLocation;
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
void r200GetLock( r200ContextPtr rmesa, GLuint flags )
{
   __DRIdrawablePrivate *dPriv = rmesa->dri.drawable;
   __DRIscreenPrivate *sPriv = rmesa->dri.screen;
   drm_radeon_sarea_t *sarea = rmesa->sarea;
   int i;

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
      r200UpdatePageFlipping( rmesa );
      if (rmesa->glCtx->DrawBuffer->_ColorDrawBufferMask[0] == BUFFER_BIT_BACK_LEFT)
         r200SetCliprects( rmesa, GL_BACK_LEFT );
      else
         r200SetCliprects( rmesa, GL_FRONT_LEFT );
      r200UpdateViewportOffset( rmesa->glCtx );
      rmesa->lastStamp = dPriv->lastStamp;
   }

   R200_STATECHANGE( rmesa, ctx );
   if (rmesa->sarea->tiling_enabled) {
      rmesa->hw.ctx.cmd[CTX_RB3D_COLORPITCH] |= R200_COLOR_TILE_ENABLE;
   }
   else rmesa->hw.ctx.cmd[CTX_RB3D_COLORPITCH] &= ~R200_COLOR_TILE_ENABLE;

   if ( sarea->ctx_owner != rmesa->dri.hwContext ) {
      sarea->ctx_owner = rmesa->dri.hwContext;
   }

   for ( i = 0 ; i < rmesa->nr_heaps ; i++ ) {
      DRI_AGE_TEXTURES( rmesa->texture_heaps[ i ] );
   }

   rmesa->lost_context = GL_TRUE;
}
