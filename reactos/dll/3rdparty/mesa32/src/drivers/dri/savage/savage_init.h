/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#ifndef _SAVAGE_INIT_H_
#define _SAVAGE_INIT_H_

#include <sys/time.h>
#include "dri_util.h"
#include "mtypes.h"

#include "xmlconfig.h"

typedef struct {
   drm_handle_t handle;
   drmSize size;
   char *map;
} savageRegion, *savageRegionPtr;

typedef struct {
   int chipset;
   int width;
   int height;
   int mem;

   int cpp;			/* for front and back buffers */
   int zpp;

   int agpMode;

   unsigned int bufferSize;

#if 0 
   int bitsPerPixel;
#endif
   unsigned int frontFormat;
   unsigned int frontOffset;
   unsigned int backOffset;
   unsigned int depthOffset;

   unsigned int aperturePitch;

   unsigned int textureOffset[SAVAGE_NR_TEX_HEAPS];
   unsigned int textureSize[SAVAGE_NR_TEX_HEAPS];
   unsigned int logTextureGranularity[SAVAGE_NR_TEX_HEAPS];
   drmAddress texVirtual[SAVAGE_NR_TEX_HEAPS];
  
   __DRIscreenPrivate *driScrnPriv;

   savageRegion aperture;
   savageRegion agpTextures;

   drmBufMapPtr bufs;

   unsigned int sarea_priv_offset;

   /* Configuration cache with default values for all contexts */
   driOptionCache optionCache;
} savageScreenPrivate;


#include "savagecontext.h"

extern void savageGetLock( savageContextPtr imesa, GLuint flags );
extern void savageXMesaSetClipRects(savageContextPtr imesa);


#define GET_DISPATCH_AGE( imesa ) imesa->sarea->last_dispatch
#define GET_ENQUEUE_AGE( imesa ) imesa->sarea->last_enqueue


/* Lock the hardware and validate our state.  
 */
#define LOCK_HARDWARE( imesa )				\
  do {							\
    char __ret=0;					\
    DRM_CAS(imesa->driHwLock, imesa->hHWContext,	\
	    (DRM_LOCK_HELD|imesa->hHWContext), __ret);	\
    if (__ret)						\
        savageGetLock( imesa, 0 );			\
  } while (0)



/* Unlock the hardware using the global current context 
 */
#define UNLOCK_HARDWARE(imesa)					\
    DRM_UNLOCK(imesa->driFd, imesa->driHwLock, imesa->hHWContext);


/* This is the wrong way to do it, I'm sure.  Otherwise the drm
 * bitches that I've already got the heavyweight lock.  At worst,
 * this is 3 ioctls.  The best solution probably only gets me down 
 * to 2 ioctls in the worst case.
 */
#define LOCK_HARDWARE_QUIESCENT( imesa ) do {	\
   LOCK_HARDWARE( imesa );			\
   savageRegetLockQuiescent( imesa );		\
} while(0)

/* The following definitions are copied from savage_regs.h in the XFree86
 * driver. They are unlikely to change. If they do we need to keep them in
 * sync. */

#define S3_SAVAGE3D_SERIES(chip)  ((chip>=S3_SAVAGE3D) && (chip<=S3_SAVAGE_MX))

#define S3_SAVAGE4_SERIES(chip)  ((chip==S3_SAVAGE4)            \
                                  || (chip==S3_PROSAVAGE)       \
                                  || (chip==S3_TWISTER)         \
                                  || (chip==S3_PROSAVAGEDDR))

#define	S3_SAVAGE_MOBILE_SERIES(chip)	((chip==S3_SAVAGE_MX) || (chip==S3_SUPERSAVAGE))

#define S3_SAVAGE_SERIES(chip)    ((chip>=S3_SAVAGE3D) && (chip<=S3_SAVAGE2000))

#define S3_MOBILE_TWISTER_SERIES(chip)   ((chip==S3_TWISTER)    \
                                          ||(chip==S3_PROSAVAGEDDR))

/* Chip tags.  These are used to group the adapters into 
 * related families.
 */

enum S3CHIPTAGS {
    S3_UNKNOWN = 0,
    S3_SAVAGE3D,
    S3_SAVAGE_MX,
    S3_SAVAGE4,
    S3_PROSAVAGE,
    S3_TWISTER,
    S3_PROSAVAGEDDR,
    S3_SUPERSAVAGE,
    S3_SAVAGE2000,
    S3_LAST
};

#endif
