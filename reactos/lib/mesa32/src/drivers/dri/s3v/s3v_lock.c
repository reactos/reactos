/*
 * Author: Max Lingua <sunmax@libero.it>
 */

#include "s3v_context.h"

#if DEBUG_LOCKING
char *prevLockFile = NULL;
int prevLockLine = 0;
#endif


/* Update the hardware state.  This is called if another context has
 * grabbed the hardware lock, which includes the X server.  This
 * function also updates the driver's window state after the X server
 * moves, resizes or restacks a window -- the change will be reflected
 * in the drawable position and clip rects.  Since the X server grabs
 * the hardware lock when it changes the window state, this routine will
 * automatically be called after such a change.
 */
void s3vGetLock( s3vContextPtr vmesa, GLuint flags )
{
   __DRIdrawablePrivate *dPriv = vmesa->driDrawable;
/*   __DRIscreenPrivate *sPriv = vmesa->driScreen; */

   printf("s3vGetLock <- ***\n");

   drmGetLock( vmesa->driFd, vmesa->hHWContext, flags );

   /* The window might have moved, so we might need to get new clip
    * rects.
    *
    * NOTE: This releases and regrabs the hw lock to allow the X server
    * to respond to the DRI protocol request for new drawable info.
    * Since the hardware state depends on having the latest drawable
    * clip rects, all state checking must be done _after_ this call.
    */
   /* DRI_VALIDATE_DRAWABLE_INFO( vmesa->display, sPriv, dPriv ); */

   if ( vmesa->lastStamp != dPriv->lastStamp ) {
      vmesa->lastStamp = dPriv->lastStamp;
      vmesa->new_state |= S3V_NEW_WINDOW | S3V_NEW_CLIP;
   }

   vmesa->numClipRects = dPriv->numClipRects;
   vmesa->pClipRects = dPriv->pClipRects;

#if 0
   vmesa->dirty = ~0;

   if ( sarea->ctxOwner != vmesa->hHWContext ) {
      sarea->ctxOwner = vmesa->hHWContext;
      vmesa->dirty = S3V_UPLOAD_ALL;
   }

   for ( i = 0 ; i < vmesa->lastTexHeap ; i++ ) {
      if ( sarea->texAge[i] != vmesa->lastTexAge[i] ) {
	 s3vAgeTextures( vmesa, i );
      }
   }
#endif
}
