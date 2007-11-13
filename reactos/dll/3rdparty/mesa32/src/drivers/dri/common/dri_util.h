/* $XFree86: xc/lib/GL/dri/dri_util.h,v 1.1 2002/02/22 21:32:52 dawes Exp $ */
/**
 * \file dri_util.h
 * DRI utility functions definitions.
 *
 * This module acts as glue between GLX and the actual hardware driver.  A DRI
 * driver doesn't really \e have to use any of this - it's optional.  But, some
 * useful stuff is done here that otherwise would have to be duplicated in most
 * drivers.
 * 
 * Basically, these utility functions take care of some of the dirty details of
 * screen initialization, context creation, context binding, DRM setup, etc.
 *
 * These functions are compiled into each DRI driver so libGL.so knows nothing
 * about them.
 *
 * \sa dri_util.c.
 * 
 * \author Kevin E. Martin <kevin@precisioninsight.com>
 * \author Brian Paul <brian@precisioninsight.com>
 */

/*
 * Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef _DRI_UTIL_H_
#define _DRI_UTIL_H_

#include <GL/gl.h>
#include "drm.h"
#include "drm_sarea.h"
#include "xf86drm.h"
#include "GL/internal/glcore.h"
#include "GL/internal/dri_interface.h"

#define GLX_BAD_CONTEXT                    5

typedef struct __DRIdisplayPrivateRec  __DRIdisplayPrivate;
typedef struct __DRIscreenPrivateRec   __DRIscreenPrivate;
typedef struct __DRIcontextPrivateRec  __DRIcontextPrivate;
typedef struct __DRIdrawablePrivateRec __DRIdrawablePrivate;
typedef struct __DRIswapInfoRec        __DRIswapInfo;
typedef struct __DRIutilversionRec2    __DRIutilversion2;


/**
 * Used by DRI_VALIDATE_DRAWABLE_INFO
 */
#define DRI_VALIDATE_DRAWABLE_INFO_ONCE(pDrawPriv)              \
    do {                                                        \
	if (*(pDrawPriv->pStamp) != pDrawPriv->lastStamp) {     \
	    __driUtilUpdateDrawableInfo(pDrawPriv);             \
	}                                                       \
    } while (0)


/**
 * Utility macro to validate the drawable information.
 *
 * See __DRIdrawablePrivate::pStamp and __DRIdrawablePrivate::lastStamp.
 */
#define DRI_VALIDATE_DRAWABLE_INFO(psp, pdp)                            \
do {                                                                    \
    while (*(pdp->pStamp) != pdp->lastStamp) {                          \
        register unsigned int hwContext = psp->pSAREA->lock.lock &      \
		     ~(DRM_LOCK_HELD | DRM_LOCK_CONT);                  \
	DRM_UNLOCK(psp->fd, &psp->pSAREA->lock, hwContext);             \
                                                                        \
	DRM_SPINLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);     \
	DRI_VALIDATE_DRAWABLE_INFO_ONCE(pdp);                           \
	DRM_SPINUNLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);   \
                                                                        \
	DRM_LIGHT_LOCK(psp->fd, &psp->pSAREA->lock, hwContext);         \
    }                                                                   \
} while (0)


/**
 * Driver callback functions.
 *
 * Each DRI driver must have one of these structures with all the pointers set
 * to appropriate functions within the driver.
 * 
 * When glXCreateContext() is called, for example, it'll call a helper function
 * dri_util.c which in turn will jump through the \a CreateContext pointer in
 * this structure.
 */
struct __DriverAPIRec {
    /** 
     * Driver initialization callback
     */
    GLboolean (*InitDriver)(__DRIscreenPrivate *driScrnPriv);
    
    /**
     * Screen destruction callback
     */
    void (*DestroyScreen)(__DRIscreenPrivate *driScrnPriv);

    /**
     * Context creation callback
     */	    	    
    GLboolean (*CreateContext)(const __GLcontextModes *glVis,
                               __DRIcontextPrivate *driContextPriv,
                               void *sharedContextPrivate);

    /**
     * Context destruction callback
     */
    void (*DestroyContext)(__DRIcontextPrivate *driContextPriv);

    /**
     * Buffer (drawable) creation callback
     */
    GLboolean (*CreateBuffer)(__DRIscreenPrivate *driScrnPriv,
                              __DRIdrawablePrivate *driDrawPriv,
                              const __GLcontextModes *glVis,
                              GLboolean pixmapBuffer);
    
    /**
     * Buffer (drawable) destruction callback
     */
    void (*DestroyBuffer)(__DRIdrawablePrivate *driDrawPriv);

    /**
     * Buffer swapping callback 
     */
    void (*SwapBuffers)(__DRIdrawablePrivate *driDrawPriv);

    /**
     * Context activation callback
     */
    GLboolean (*MakeCurrent)(__DRIcontextPrivate *driContextPriv,
                             __DRIdrawablePrivate *driDrawPriv,
                             __DRIdrawablePrivate *driReadPriv);

    /**
     * Context unbinding callback
     */
    GLboolean (*UnbindContext)(__DRIcontextPrivate *driContextPriv);
  
    /**
     * Retrieves statistics about buffer swap operations.  Required if
     * GLX_OML_sync_control or GLX_MESA_swap_frame_usage is supported.
     */
    int (*GetSwapInfo)( __DRIdrawablePrivate *dPriv, __DRIswapInfo * sInfo );


    /**
     * Required if GLX_SGI_video_sync or GLX_OML_sync_control is
     * supported.
     */
    int (*GetMSC)( __DRIscreenPrivate * priv, int64_t * count );

    /**
     * These are required if GLX_OML_sync_control is supported.
     */
    /*@{*/
    int (*WaitForMSC)( __DRIdrawablePrivate *priv, int64_t target_msc, 
		       int64_t divisor, int64_t remainder,
		       int64_t * msc );
    int (*WaitForSBC)( __DRIdrawablePrivate *priv, int64_t target_sbc,
		       int64_t * msc, int64_t * sbc );

    int64_t (*SwapBuffersMSC)( __DRIdrawablePrivate *priv, int64_t target_msc,
			       int64_t divisor, int64_t remainder );
    /*@}*/
    void (*CopySubBuffer)(__DRIdrawablePrivate *driDrawPriv,
			  int x, int y, int w, int h);

    /**
     * See corresponding field in \c __DRIscreenRec.
     */
    void (*setTexOffset)(__DRIcontext *pDRICtx, GLint texname,
			 unsigned long long offset, GLint depth, GLuint pitch);
};


struct __DRIswapInfoRec {
    /** 
     * Number of swapBuffers operations that have been *completed*. 
     */
    u_int64_t swap_count;

    /**
     * Unadjusted system time of the last buffer swap.  This is the time
     * when the swap completed, not the time when swapBuffers was called.
     */
    int64_t   swap_ust;

    /**
     * Number of swap operations that occurred after the swap deadline.  That
     * is if a swap happens more than swap_interval frames after the previous
     * swap, it has missed its deadline.  If swap_interval is 0, then the
     * swap deadline is 1 frame after the previous swap.
     */
    u_int64_t swap_missed_count;

    /**
     * Amount of time used by the last swap that missed its deadline.  This
     * is calculated as (__glXGetUST() - swap_ust) / (swap_interval * 
     * time_for_single_vrefresh)).  If the actual value of swap_interval is
     * 0, then 1 is used instead.  If swap_missed_count is non-zero, this
     * should be greater-than 1.0.
     */
    float     swap_missed_usage;
};


/**
 * Per-drawable private DRI driver information.
 */
struct __DRIdrawablePrivateRec {
    /**
     * Kernel drawable handle
     */
    drm_drawable_t hHWDrawable;

    /**
     * Driver's private drawable information.  
     *
     * This structure is opaque.
     */
    void *driverPrivate;

    /**
     * X's drawable ID associated with this private drawable.
     */
    __DRIid draw;
    __DRIdrawable *pdraw;

    /**
     * Reference count for number of context's currently bound to this
     * drawable.  
     *
     * Once it reaches zero, the drawable can be destroyed.
     *
     * \note This behavior will change with GLX 1.3.
     */
    int refcount;

    /**
     * Index of this drawable information in the SAREA.
     */
    unsigned int index;

    /**
     * Pointer to the "drawable has changed ID" stamp in the SAREA.
     */
    unsigned int *pStamp;

    /**
     * Last value of the stamp.
     *
     * If this differs from the value stored at __DRIdrawablePrivate::pStamp,
     * then the drawable information has been modified by the X server, and the
     * drawable information (below) should be retrieved from the X server.
     */
    unsigned int lastStamp;

    /**
     * \name Drawable 
     *
     * Drawable information used in software fallbacks.
     */
    /*@{*/
    int x;
    int y;
    int w;
    int h;
    int numClipRects;
    drm_clip_rect_t *pClipRects;
    /*@}*/

    /**
     * \name Back and depthbuffer
     *
     * Information about the back and depthbuffer where different from above.
     */
    /*@{*/
    int backX;
    int backY;
    int backClipRectType;
    int numBackClipRects;
    drm_clip_rect_t *pBackClipRects;
    /*@}*/

    /**
     * Pointer to context to which this drawable is currently bound.
     */
    __DRIcontextPrivate *driContextPriv;

    /**
     * Pointer to screen on which this drawable was created.
     */
    __DRIscreenPrivate *driScreenPriv;

    /**
     * \name Display and screen information.
     * 
     * Basically just need these for when the locking code needs to call
     * \c __driUtilUpdateDrawableInfo.
     */
    /*@{*/
    __DRInativeDisplay *display;
    int screen;
    /*@}*/

    /**
     * Called via glXSwapBuffers().
     */
    void (*swapBuffers)( __DRIdrawablePrivate *dPriv );
};

/**
 * Per-context private driver information.
 */
struct __DRIcontextPrivateRec {
    /**
     * Kernel context handle used to access the device lock.
     */
    __DRIid contextID;

    /**
     * Kernel context handle used to access the device lock.
     */
    drm_context_t hHWContext;

    /**
     * Device driver's private context data.  This structure is opaque.
     */
    void *driverPrivate;

    /**
     * This context's display pointer.
     */
    __DRInativeDisplay *display;

    /**
     * Pointer to drawable currently bound to this context for drawing.
     */
    __DRIdrawablePrivate *driDrawablePriv;

    /**
     * Pointer to drawable currently bound to this context for reading.
     */
    __DRIdrawablePrivate *driReadablePriv;

    /**
     * Pointer to screen on which this context was created.
     */
    __DRIscreenPrivate *driScreenPriv;
};

/**
 * Per-screen private driver information.
 */
struct __DRIscreenPrivateRec {
    /**
     * Display for this screen
     */
    __DRInativeDisplay *display;

    /**
     * Current screen's number
     */
    int myNum;

    /**
     * Callback functions into the hardware-specific DRI driver code.
     */
    struct __DriverAPIRec DriverAPI;

    /**
     * \name DDX version
     * DDX / 2D driver version information.
     * \todo Replace these fields with a \c __DRIversionRec.
     */
    /*@{*/
    int ddxMajor;
    int ddxMinor;
    int ddxPatch;
    /*@}*/

    /**
     * \name DRI version
     * DRI X extension version information.
     * \todo Replace these fields with a \c __DRIversionRec.
     */
    /*@{*/
    int driMajor;
    int driMinor;
    int driPatch;
    /*@}*/

    /**
     * \name DRM version
     * DRM (kernel module) version information.
     * \todo Replace these fields with a \c __DRIversionRec.
     */
    /*@{*/
    int drmMajor;
    int drmMinor;
    int drmPatch;
    /*@}*/

    /**
     * ID used when the client sets the drawable lock.
     *
     * The X server uses this value to detect if the client has died while
     * holding the drawable lock.
     */
    int drawLockID;

    /**
     * File descriptor returned when the kernel device driver is opened.
     * 
     * Used to:
     *   - authenticate client to kernel
     *   - map the frame buffer, SAREA, etc.
     *   - close the kernel device driver
     */
    int fd;

    /**
     * SAREA pointer 
     *
     * Used to access:
     *   - the device lock
     *   - the device-independent per-drawable and per-context(?) information
     */
    drm_sarea_t *pSAREA;

    /**
     * \name Direct frame buffer access information 
     * Used for software fallbacks.
     */
    /*@{*/
    unsigned char *pFB;
    int fbSize;
    int fbOrigin;
    int fbStride;
    int fbWidth;
    int fbHeight;
    int fbBPP;
    /*@}*/

    /**
     * \name Device-dependent private information (stored in the SAREA).
     *
     * This data is accessed by the client driver only.
     */
    /*@{*/
    void *pDevPriv;
    int devPrivSize;
    /*@}*/

    /**
     * Dummy context to which drawables are bound when not bound to any
     * other context. 
     *
     * A dummy hHWContext is created for this context, and is used by the GL
     * core when a hardware lock is required but the drawable is not currently
     * bound (e.g., potentially during a SwapBuffers request).  The dummy
     * context is created when the first "real" context is created on this
     * screen.
     */
    __DRIcontextPrivate dummyContextPriv;

    /**
     * Hash table to hold the drawable information for this screen.
     */
    void *drawHash;

    /**
     * Device-dependent private information (not stored in the SAREA).
     * 
     * This pointer is never touched by the DRI layer.
     */
    void *private;

    /**
     * GLX visuals / FBConfigs for this screen.  These are stored as a
     * linked list.
     * 
     * \note
     * This field is \b only used in conjunction with the old interfaces.  If
     * the new interfaces are used, this field will be set to \c NULL and will
     * not be dereferenced.
     */
    __GLcontextModes *modes;

    /**
     * Pointer back to the \c __DRIscreen that contains this structure.
     */

    __DRIscreen *psc;
};


/**
 * Used to store a version which includes a major range instead of a single
 * major version number.
 */
struct __DRIutilversionRec2 {
    int    major_min;    /** min allowed Major version number. */
    int    major_max;    /** max allowed Major version number. */
    int    minor;        /**< Minor version number. */
    int    patch;        /**< Patch-level. */
};


extern void
__driUtilMessage(const char *f, ...);


extern void
__driUtilUpdateDrawableInfo(__DRIdrawablePrivate *pdp);


extern __DRIscreenPrivate * __driUtilCreateNewScreen( __DRInativeDisplay *dpy,
    int scrn, __DRIscreen *psc, __GLcontextModes * modes,
    const __DRIversion * ddx_version, const __DRIversion * dri_version,
    const __DRIversion * drm_version, const __DRIframebuffer * frame_buffer,
    drm_sarea_t *pSAREA, int fd, int internal_api_version,
    const struct __DriverAPIRec *driverAPI );

/* Test the version of the internal GLX API.  Returns a value like strcmp. */
extern int
driCompareGLXAPIVersion( GLint required_version );

extern float
driCalculateSwapUsage( __DRIdrawablePrivate *dPriv,
		       int64_t last_swap_ust, int64_t current_ust );

/**
 * Pointer to the \c __DRIinterfaceMethods passed to the driver by the loader.
 * 
 * This pointer is set in the driver's \c __driCreateNewScreen function and
 * is defined in dri_util.c.
 */
extern const __DRIinterfaceMethods * dri_interface;

#endif /* _DRI_UTIL_H_ */
