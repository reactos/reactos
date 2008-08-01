/* $XFree86: xc/lib/GL/mesa/src/drv/ffb/ffb_lock.h,v 1.2 2002/02/22 21:32:59 dawes Exp $ */

#ifndef _FFB_LOCK_H
#define _FFB_LOCK_H

#include "ffb_context.h"

extern void ffbXMesaUpdateState(ffbContextPtr fmesa);
#define FFB_UPDATE_STATE(fmesa)	ffbXMesaUpdateState(fmesa)

/* Lock the hardware and validate our state.  This hardware can only ever
 * exist on SPARC platforms.  Don't bother building the real LOCK_HARDWARE and
 * UNLOCK_HARDWARE code on non-SPARC platforms.  The only reason the driver
 * gets built on non-SPARC is to catch build breakages earlier.
 */
#if !defined(__sparc__)
#define LOCK_HARDWARE(fmesa)			
#define UNLOCK_HARDWARE(fmesa)			
#else
#define LOCK_HARDWARE(fmesa)				\
  do {							\
    DRM_CAS_RESULT(__ret);				\
    DRM_CAS(fmesa->driHwLock, fmesa->hHWContext,	\
	    (DRM_LOCK_HELD | fmesa->hHWContext), __ret);\
    if (__ret) {					\
        drmGetLock(fmesa->driFd, fmesa->hHWContext, 0);	\
	FFB_UPDATE_STATE(fmesa);			\
    }							\
  } while (0)


/* Unlock the hardware. */
#define UNLOCK_HARDWARE(fmesa)					\
    DRM_UNLOCK(fmesa->driFd, fmesa->driHwLock, fmesa->hHWContext);	
#endif

#endif /* !(_FFB_LOCK_H) */
