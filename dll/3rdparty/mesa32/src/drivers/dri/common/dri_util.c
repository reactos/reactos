/* $XFree86: xc/lib/GL/dri/dri_util.c,v 1.7 2003/04/28 17:01:25 dawes Exp $ */
/**
 * \file dri_util.c
 * DRI utility functions.
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
 */


#include <assert.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>

#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

#include "imports.h"
#define None 0

#include "dri_util.h"
#include "drm_sarea.h"

#ifndef GLX_OML_sync_control
typedef GLboolean ( * PFNGLXGETMSCRATEOMLPROC) (__DRInativeDisplay *dpy, __DRIid drawable, int32_t *numerator, int32_t *denominator);
#endif

/* This pointer *must* be set by the driver's __driCreateNewScreen funciton!
 */
const __DRIinterfaceMethods * dri_interface = NULL;

/**
 * This is used in a couple of places that call \c driCreateNewDrawable.
 */
static const int empty_attribute_list[1] = { None };


/**
 * Cached copy of the internal API version used by libGL and the client-side
 * DRI driver.
 */
static int api_ver = 0;

/* forward declarations */
static int driQueryFrameTracking( __DRInativeDisplay *dpy, void *priv,
                                  int64_t *sbc, int64_t *missedFrames,
                                  float *lastMissedUsage, float *usage );

static void *driCreateNewDrawable(__DRInativeDisplay *dpy,
                                  const __GLcontextModes *modes,
                                  __DRIid draw, __DRIdrawable *pdraw,
                                  int renderType, const int *attrs);

static void driDestroyDrawable(__DRInativeDisplay *dpy, void *drawablePrivate);


/**
 * Print message to \c stderr if the \c LIBGL_DEBUG environment variable
 * is set. 
 * 
 * Is called from the drivers.
 * 
 * \param f \c printf like format string.
 */
void
__driUtilMessage(const char *f, ...)
{
    va_list args;

    if (getenv("LIBGL_DEBUG")) {
        fprintf(stderr, "libGL error: \n");
        va_start(args, f);
        vfprintf(stderr, f, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
}


/*****************************************************************/
/** \name Drawable list management */
/*****************************************************************/
/*@{*/

static GLboolean __driAddDrawable(void *drawHash, __DRIdrawable *pdraw)
{
    __DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *)pdraw->private;

    if (drmHashInsert(drawHash, pdp->draw, pdraw))
	return GL_FALSE;

    return GL_TRUE;
}

static __DRIdrawable *__driFindDrawable(void *drawHash, __DRIid draw)
{
    int retcode;
    __DRIdrawable *pdraw;

    retcode = drmHashLookup(drawHash, draw, (void *)&pdraw);
    if (retcode)
	return NULL;

    return pdraw;
}


/**
 * Find drawables in the local hash that have been destroyed on the
 * server.
 * 
 * \param drawHash  Hash-table containing all know drawables.
 */
static void __driGarbageCollectDrawables(void *drawHash)
{
    __DRIid draw;
    __DRInativeDisplay *dpy;
    __DRIdrawable *pdraw;

    if (drmHashFirst(drawHash, &draw, (void *)&pdraw) == 1) {
	do {
	    __DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *)pdraw->private;
	    dpy = pdp->driScreenPriv->display;
	    if (! (*dri_interface->windowExists)(dpy, draw)) {
		/* Destroy the local drawable data, if the drawable no
		   longer exists in the Xserver */
		(*pdraw->destroyDrawable)(dpy, pdraw->private);
		_mesa_free(pdraw);
	    }
	} while (drmHashNext(drawHash, &draw, (void *)&pdraw) == 1);
    }
}

/*@}*/


/*****************************************************************/
/** \name Context (un)binding functions                          */
/*****************************************************************/
/*@{*/

/**
 * Unbind context.
 * 
 * \param dpy the display handle.
 * \param scrn the screen number.
 * \param draw drawable.
 * \param read Current reading drawable.
 * \param gc context.
 *
 * \return \c GL_TRUE on success, or \c GL_FALSE on failure.
 * 
 * \internal
 * This function calls __DriverAPIRec::UnbindContext, and then decrements
 * __DRIdrawablePrivateRec::refcount which must be non-zero for a successful
 * return.
 * 
 * While casting the opaque private pointers associated with the parameters
 * into their respective real types it also assures they are not \c NULL. 
 */
static GLboolean driUnbindContext(__DRInativeDisplay *dpy, int scrn,
			      __DRIid draw, __DRIid read,
			      __DRIcontext *ctx)
{
    __DRIscreen *pDRIScreen;
    __DRIdrawable *pdraw;
    __DRIdrawable *pread;
    __DRIcontextPrivate *pcp;
    __DRIscreenPrivate *psp;
    __DRIdrawablePrivate *pdp;
    __DRIdrawablePrivate *prp;

    /*
    ** Assume error checking is done properly in glXMakeCurrent before
    ** calling driUnbindContext.
    */

    if (ctx == NULL || draw == None || read == None) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    pDRIScreen = (*dri_interface->getScreen)(dpy, scrn);
    if ( (pDRIScreen == NULL) || (pDRIScreen->private == NULL) ) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    psp = (__DRIscreenPrivate *)pDRIScreen->private;
    pcp = (__DRIcontextPrivate *)ctx->private;

    pdraw = __driFindDrawable(psp->drawHash, draw);
    if (!pdraw) {
	/* ERROR!!! */
	return GL_FALSE;
    }
    pdp = (__DRIdrawablePrivate *)pdraw->private;

    pread = __driFindDrawable(psp->drawHash, read);
    if (!pread) {
	/* ERROR!!! */
	return GL_FALSE;
    }
    prp = (__DRIdrawablePrivate *)pread->private;


    /* Let driver unbind drawable from context */
    (*psp->DriverAPI.UnbindContext)(pcp);


    if (pdp->refcount == 0) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    pdp->refcount--;

    if (prp != pdp) {
        if (prp->refcount == 0) {
	    /* ERROR!!! */
	    return GL_FALSE;
	}

	prp->refcount--;
    }


    /* XXX this is disabled so that if we call SwapBuffers on an unbound
     * window we can determine the last context bound to the window and
     * use that context's lock. (BrianP, 2-Dec-2000)
     */
#if 0
    /* Unbind the drawable */
    pcp->driDrawablePriv = NULL;
    pdp->driContextPriv = &psp->dummyContextPriv;
#endif

    return GL_TRUE;
}


/**
 * This function takes both a read buffer and a draw buffer.  This is needed
 * for \c glXMakeCurrentReadSGI or GLX 1.3's \c glXMakeContextCurrent
 * function.
 * 
 * \bug This function calls \c driCreateNewDrawable in two places with the
 *      \c renderType hard-coded to \c GLX_WINDOW_BIT.  Some checking might
 *      be needed in those places when support for pbuffers and / or pixmaps
 *      is added.  Is it safe to assume that the drawable is a window?
 */
static GLboolean DoBindContext(__DRInativeDisplay *dpy,
			  __DRIid draw, __DRIid read,
			  __DRIcontext *ctx, const __GLcontextModes * modes,
			  __DRIscreenPrivate *psp)
{
    __DRIdrawable *pdraw;
    __DRIdrawablePrivate *pdp;
    __DRIdrawable *pread;
    __DRIdrawablePrivate *prp;
    __DRIcontextPrivate * const pcp = ctx->private;


    /* Find the _DRIdrawable which corresponds to the writing drawable. */
    pdraw = __driFindDrawable(psp->drawHash, draw);
    if (!pdraw) {
	/* Allocate a new drawable */
	pdraw = (__DRIdrawable *)_mesa_malloc(sizeof(__DRIdrawable));
	if (!pdraw) {
	    /* ERROR!!! */
	    return GL_FALSE;
	}

	/* Create a new drawable */
	driCreateNewDrawable(dpy, modes, draw, pdraw, GLX_WINDOW_BIT,
			     empty_attribute_list);
	if (!pdraw->private) {
	    /* ERROR!!! */
	    _mesa_free(pdraw);
	    return GL_FALSE;
	}

    }
    pdp = (__DRIdrawablePrivate *) pdraw->private;

    /* Find the _DRIdrawable which corresponds to the reading drawable. */
    if (read == draw) {
        /* read buffer == draw buffer */
        prp = pdp;
    }
    else {
        pread = __driFindDrawable(psp->drawHash, read);
        if (!pread) {
            /* Allocate a new drawable */
            pread = (__DRIdrawable *)_mesa_malloc(sizeof(__DRIdrawable));
            if (!pread) {
                /* ERROR!!! */
                return GL_FALSE;
            }

            /* Create a new drawable */
	    driCreateNewDrawable(dpy, modes, read, pread, GLX_WINDOW_BIT,
				 empty_attribute_list);
            if (!pread->private) {
                /* ERROR!!! */
                _mesa_free(pread);
                return GL_FALSE;
            }
        }
        prp = (__DRIdrawablePrivate *) pread->private;
    }

    /* Bind the drawable to the context */
    pcp->driDrawablePriv = pdp;
    pcp->driReadablePriv = prp;
    pdp->driContextPriv = pcp;
    pdp->refcount++;
    if ( pdp != prp ) {
	prp->refcount++;
    }

    /*
    ** Now that we have a context associated with this drawable, we can
    ** initialize the drawable information if has not been done before.
    */
    if (!pdp->pStamp || *pdp->pStamp != pdp->lastStamp) {
	DRM_SPINLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);
	__driUtilUpdateDrawableInfo(pdp);
	DRM_SPINUNLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);
    }

    if ((pdp != prp) && (!prp->pStamp || *prp->pStamp != prp->lastStamp)) {
	DRM_SPINLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);
	__driUtilUpdateDrawableInfo(prp);
	DRM_SPINUNLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);
    }

    /* Call device-specific MakeCurrent */
    (*psp->DriverAPI.MakeCurrent)(pcp, pdp, prp);

    return GL_TRUE;
}


/**
 * This function takes both a read buffer and a draw buffer.  This is needed
 * for \c glXMakeCurrentReadSGI or GLX 1.3's \c glXMakeContextCurrent
 * function.
 */
static GLboolean driBindContext(__DRInativeDisplay *dpy, int scrn,
                            __DRIid draw, __DRIid read,
                            __DRIcontext * ctx)
{
    __DRIscreen *pDRIScreen;

    /*
    ** Assume error checking is done properly in glXMakeCurrent before
    ** calling driBindContext.
    */

    if (ctx == NULL || draw == None || read == None) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    pDRIScreen = (*dri_interface->getScreen)(dpy, scrn);
    if ( (pDRIScreen == NULL) || (pDRIScreen->private == NULL) ) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    return DoBindContext( dpy, draw, read, ctx, ctx->mode,
			  (__DRIscreenPrivate *)pDRIScreen->private );
}
/*@}*/


/*****************************************************************/
/** \name Drawable handling functions                            */
/*****************************************************************/
/*@{*/

/**
 * Update private drawable information.
 *
 * \param pdp pointer to the private drawable information to update.
 * 
 * This function basically updates the __DRIdrawablePrivate struct's
 * cliprect information by calling \c __DRIinterfaceMethods::getDrawableInfo.
 * This is usually called by the DRI_VALIDATE_DRAWABLE_INFO macro which
 * compares the __DRIdrwablePrivate pStamp and lastStamp values.  If
 * the values are different that means we have to update the clipping
 * info.
 */
void
__driUtilUpdateDrawableInfo(__DRIdrawablePrivate *pdp)
{
    __DRIscreenPrivate *psp;
    __DRIcontextPrivate *pcp = pdp->driContextPriv;
    
    if (!pcp 
	|| ((pdp != pcp->driDrawablePriv) && (pdp != pcp->driReadablePriv))) {
	/* ERROR!!! 
	 * ...but we must ignore it. There can be many contexts bound to a
	 * drawable.
	 */
    }

    psp = pdp->driScreenPriv;
    if (!psp) {
	/* ERROR!!! */
       _mesa_problem(NULL, "Warning! Possible infinite loop due to bug "
		     "in file %s, line %d\n",
		     __FILE__, __LINE__);
	return;
    }

    if (pdp->pClipRects) {
	_mesa_free(pdp->pClipRects); 
	pdp->pClipRects = NULL;
    }

    if (pdp->pBackClipRects) {
	_mesa_free(pdp->pBackClipRects); 
	pdp->pBackClipRects = NULL;
    }

    DRM_SPINUNLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);

    if (!__driFindDrawable(psp->drawHash, pdp->draw) ||
	! (*dri_interface->getDrawableInfo)(pdp->display, pdp->screen, pdp->draw,
			  &pdp->index, &pdp->lastStamp,
			  &pdp->x, &pdp->y, &pdp->w, &pdp->h,
			  &pdp->numClipRects, &pdp->pClipRects,
			  &pdp->backX,
			  &pdp->backY,
			  &pdp->numBackClipRects,
			  &pdp->pBackClipRects )) {
	/* Error -- eg the window may have been destroyed.  Keep going
	 * with no cliprects.
	 */
        pdp->pStamp = &pdp->lastStamp; /* prevent endless loop */
	pdp->numClipRects = 0;
	pdp->pClipRects = NULL;
	pdp->numBackClipRects = 0;
	pdp->pBackClipRects = NULL;
    }
    else
       pdp->pStamp = &(psp->pSAREA->drawableTable[pdp->index].stamp);

    DRM_SPINLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);

}

/*@}*/

/*****************************************************************/
/** \name GLX callbacks                                          */
/*****************************************************************/
/*@{*/

/**
 * Swap buffers.
 *
 * \param dpy the display handle.
 * \param drawablePrivate opaque pointer to the per-drawable private info.
 * 
 * \internal
 * This function calls __DRIdrawablePrivate::swapBuffers.
 * 
 * Is called directly from glXSwapBuffers().
 */
static void driSwapBuffers( __DRInativeDisplay *dpy, void *drawablePrivate )
{
    __DRIdrawablePrivate *dPriv = (__DRIdrawablePrivate *) drawablePrivate;
    drm_clip_rect_t rect;

    dPriv->swapBuffers(dPriv);

    /* Check that we actually have the new damage report method */
    if (api_ver < 20070105 || dri_interface->reportDamage == NULL)
	return;

    /* Assume it's affecting the whole drawable for now */
    rect.x1 = 0;
    rect.y1 = 0;
    rect.x2 = rect.x1 + dPriv->w;
    rect.y2 = rect.y1 + dPriv->h;

    /* Report the damage.  Currently, all our drivers draw directly to the
     * front buffer, so we report the damage there rather than to the backing
     * store (if any).
     */
    (*dri_interface->reportDamage)(dpy, dPriv->screen, dPriv->draw,
				   dPriv->x, dPriv->y,
				   &rect, 1, GL_TRUE);
}

/**
 * Called directly from a number of higher-level GLX functions.
 */
static int driGetMSC( void *screenPrivate, int64_t *msc )
{
    __DRIscreenPrivate *sPriv = (__DRIscreenPrivate *) screenPrivate;

    return sPriv->DriverAPI.GetMSC( sPriv, msc );
}

/**
 * Called directly from a number of higher-level GLX functions.
 */
static int driGetSBC( __DRInativeDisplay *dpy, void *drawablePrivate, int64_t *sbc )
{
   __DRIdrawablePrivate *dPriv = (__DRIdrawablePrivate *) drawablePrivate;
   __DRIswapInfo  sInfo;
   int  status;


   status = dPriv->driScreenPriv->DriverAPI.GetSwapInfo( dPriv, & sInfo );
   *sbc = sInfo.swap_count;

   return status;
}

static int driWaitForSBC( __DRInativeDisplay * dpy, void *drawablePriv,
			  int64_t target_sbc,
			  int64_t * msc, int64_t * sbc )
{
    __DRIdrawablePrivate *dPriv = (__DRIdrawablePrivate *) drawablePriv;

    return dPriv->driScreenPriv->DriverAPI.WaitForSBC( dPriv, target_sbc,
                                                       msc, sbc );
}

static int driWaitForMSC( __DRInativeDisplay * dpy, void *drawablePriv,
			  int64_t target_msc,
			  int64_t divisor, int64_t remainder,
			  int64_t * msc, int64_t * sbc )
{
    __DRIdrawablePrivate *dPriv = (__DRIdrawablePrivate *) drawablePriv;
    __DRIswapInfo  sInfo;
    int  status;


    status = dPriv->driScreenPriv->DriverAPI.WaitForMSC( dPriv, target_msc,
                                                         divisor, remainder,
                                                         msc );

    /* GetSwapInfo() may not be provided by the driver if GLX_SGI_video_sync
     * is supported but GLX_OML_sync_control is not.  Therefore, don't return
     * an error value if GetSwapInfo() is not implemented.
    */
    if ( status == 0
         && dPriv->driScreenPriv->DriverAPI.GetSwapInfo ) {
        status = dPriv->driScreenPriv->DriverAPI.GetSwapInfo( dPriv, & sInfo );
        *sbc = sInfo.swap_count;
    }

    return status;
}

static int64_t driSwapBuffersMSC( __DRInativeDisplay * dpy, void *drawablePriv,
				  int64_t target_msc,
				  int64_t divisor, int64_t remainder )
{
    __DRIdrawablePrivate *dPriv = (__DRIdrawablePrivate *) drawablePriv;

    return dPriv->driScreenPriv->DriverAPI.SwapBuffersMSC( dPriv, target_msc,
                                                           divisor, 
                                                           remainder );
}

static void driCopySubBuffer( __DRInativeDisplay *dpy, void *drawablePrivate,
			      int x, int y, int w, int h)
{
    __DRIdrawablePrivate *dPriv = (__DRIdrawablePrivate *) drawablePrivate;
    dPriv->driScreenPriv->DriverAPI.CopySubBuffer(dPriv, x, y, w, h);
    (void) dpy;
}

/**
 * This is called via __DRIscreenRec's createNewDrawable pointer.
 */
static void *driCreateNewDrawable(__DRInativeDisplay *dpy,
				  const __GLcontextModes *modes,
				  __DRIid draw,
				  __DRIdrawable *pdraw,
				  int renderType,
				  const int *attrs)
{
    __DRIscreen * const pDRIScreen = (*dri_interface->getScreen)(dpy, modes->screen);
    __DRIscreenPrivate *psp;
    __DRIdrawablePrivate *pdp;


    pdraw->private = NULL;

    /* Since pbuffers are not yet supported, no drawable attributes are
     * supported either.
     */
    (void) attrs;

    if ( (pDRIScreen == NULL) || (pDRIScreen->private == NULL) ) {
	return NULL;
    }

    pdp = (__DRIdrawablePrivate *)_mesa_malloc(sizeof(__DRIdrawablePrivate));
    if (!pdp) {
	return NULL;
    }

    if (!(*dri_interface->createDrawable)(dpy, modes->screen, draw, &pdp->hHWDrawable)) {
	_mesa_free(pdp);
	return NULL;
    }

    pdp->draw = draw;
    pdp->pdraw = pdraw;
    pdp->refcount = 0;
    pdp->pStamp = NULL;
    pdp->lastStamp = 0;
    pdp->index = 0;
    pdp->x = 0;
    pdp->y = 0;
    pdp->w = 0;
    pdp->h = 0;
    pdp->numClipRects = 0;
    pdp->numBackClipRects = 0;
    pdp->pClipRects = NULL;
    pdp->pBackClipRects = NULL;
    pdp->display = dpy;
    pdp->screen = modes->screen;

    psp = (__DRIscreenPrivate *)pDRIScreen->private;
    pdp->driScreenPriv = psp;
    pdp->driContextPriv = &psp->dummyContextPriv;

    if (!(*psp->DriverAPI.CreateBuffer)(psp, pdp, modes,
					renderType == GLX_PIXMAP_BIT)) {
       (void)(*dri_interface->destroyDrawable)(dpy, modes->screen, pdp->draw);
       _mesa_free(pdp);
       return NULL;
    }

    pdraw->private = pdp;
    pdraw->destroyDrawable = driDestroyDrawable;
    pdraw->swapBuffers = driSwapBuffers;  /* called by glXSwapBuffers() */

    pdraw->getSBC = driGetSBC;
    pdraw->waitForSBC = driWaitForSBC;
    pdraw->waitForMSC = driWaitForMSC;
    pdraw->swapBuffersMSC = driSwapBuffersMSC;
    pdraw->frameTracking = NULL;
    pdraw->queryFrameTracking = driQueryFrameTracking;

    if (driCompareGLXAPIVersion (20060314) >= 0)
	pdraw->copySubBuffer = driCopySubBuffer;

    /* This special default value is replaced with the configured
     * default value when the drawable is first bound to a direct
     * rendering context. 
     */
    pdraw->swap_interval = (unsigned)-1;

    pdp->swapBuffers = psp->DriverAPI.SwapBuffers;

    /* Add pdraw to drawable list */
    if (!__driAddDrawable(psp->drawHash, pdraw)) {
	/* ERROR!!! */
	(*pdraw->destroyDrawable)(dpy, pdp);
	_mesa_free(pdp);
	pdp = NULL;
	pdraw->private = NULL;
    }

   return (void *) pdp;
}

static __DRIdrawable *
driGetDrawable(__DRInativeDisplay *dpy, __DRIid draw, void *screenPrivate)
{
    __DRIscreenPrivate *psp = (__DRIscreenPrivate *) screenPrivate;

    /*
    ** Make sure this routine returns NULL if the drawable is not bound
    ** to a direct rendering context!
    */
    return __driFindDrawable(psp->drawHash, draw);
}

static void
driDestroyDrawable(__DRInativeDisplay *dpy, void *drawablePrivate)
{
    __DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *) drawablePrivate;
    __DRIscreenPrivate *psp;
    int scrn;

    if (pdp) {
	psp = pdp->driScreenPriv;
	scrn = psp->myNum;
        (*psp->DriverAPI.DestroyBuffer)(pdp);
	if ((*dri_interface->windowExists)(dpy, pdp->draw))
	    (void)(*dri_interface->destroyDrawable)(dpy, scrn, pdp->draw);
	drmHashDelete(psp->drawHash, pdp->draw);
	if (pdp->pClipRects) {
	    _mesa_free(pdp->pClipRects);
	    pdp->pClipRects = NULL;
	}
	if (pdp->pBackClipRects) {
	    _mesa_free(pdp->pBackClipRects);
	    pdp->pBackClipRects = NULL;
	}
	_mesa_free(pdp);
    }
}

/*@}*/


/*****************************************************************/
/** \name Context handling functions                             */
/*****************************************************************/
/*@{*/

/**
 * Destroy the per-context private information.
 * 
 * \param dpy the display handle.
 * \param scrn the screen number.
 * \param contextPrivate opaque pointer to the per-drawable private info.
 *
 * \internal
 * This function calls __DriverAPIRec::DestroyContext on \p contextPrivate, calls
 * drmDestroyContext(), and finally frees \p contextPrivate.
 */
static void
driDestroyContext(__DRInativeDisplay *dpy, int scrn, void *contextPrivate)
{
    __DRIcontextPrivate  *pcp   = (__DRIcontextPrivate *) contextPrivate;

    if (pcp) {
	(*pcp->driScreenPriv->DriverAPI.DestroyContext)(pcp);
	__driGarbageCollectDrawables(pcp->driScreenPriv->drawHash);
	(void) (*dri_interface->destroyContext)(dpy, scrn, pcp->contextID);
	_mesa_free(pcp);
    }
}


/**
 * Create the per-drawable private driver information.
 * 
 * \param dpy           The display handle.
 * \param modes         Mode used to create the new context.
 * \param render_type   Type of rendering target.  \c GLX_RGBA is the only
 *                      type likely to ever be supported for direct-rendering.
 * \param sharedPrivate The shared context dependent methods or \c NULL if
 *                      non-existent.
 * \param pctx          DRI context to receive the context dependent methods.
 *
 * \returns An opaque pointer to the per-context private information on
 *          success, or \c NULL on failure.
 * 
 * \internal
 * This function allocates and fills a __DRIcontextPrivateRec structure.  It
 * performs some device independent initialization and passes all the
 * relevent information to __DriverAPIRec::CreateContext to create the
 * context.
 *
 */
static void *
driCreateNewContext(__DRInativeDisplay *dpy, const __GLcontextModes *modes,
		    int render_type, void *sharedPrivate, __DRIcontext *pctx)
{
    __DRIscreen *pDRIScreen;
    __DRIcontextPrivate *pcp;
    __DRIcontextPrivate *pshare = (__DRIcontextPrivate *) sharedPrivate;
    __DRIscreenPrivate *psp;
    void * const shareCtx = (pshare != NULL) ? pshare->driverPrivate : NULL;

    pDRIScreen = (*dri_interface->getScreen)(dpy, modes->screen);
    if ( (pDRIScreen == NULL) || (pDRIScreen->private == NULL) ) {
	/* ERROR!!! */
	return NULL;
    } 

    psp = (__DRIscreenPrivate *)pDRIScreen->private;

    pcp = (__DRIcontextPrivate *)_mesa_malloc(sizeof(__DRIcontextPrivate));
    if (!pcp) {
	return NULL;
    }

    if (! (*dri_interface->createContext)(dpy, modes->screen, modes->fbconfigID,
					&pcp->contextID, &pcp->hHWContext)) {
	_mesa_free(pcp);
	return NULL;
    }

    pcp->display = dpy;
    pcp->driScreenPriv = psp;
    pcp->driDrawablePriv = NULL;

    /* When the first context is created for a screen, initialize a "dummy"
     * context.
     */

    if (!psp->dummyContextPriv.driScreenPriv) {
        psp->dummyContextPriv.contextID = 0;
        psp->dummyContextPriv.hHWContext = psp->pSAREA->dummy_context;
        psp->dummyContextPriv.driScreenPriv = psp;
        psp->dummyContextPriv.driDrawablePriv = NULL;
        psp->dummyContextPriv.driverPrivate = NULL;
	/* No other fields should be used! */
    }

    pctx->destroyContext = driDestroyContext;
    pctx->bindContext    = driBindContext;
    pctx->unbindContext  = driUnbindContext;

    if ( !(*psp->DriverAPI.CreateContext)(modes, pcp, shareCtx) ) {
        (void) (*dri_interface->destroyContext)(dpy, modes->screen,
						pcp->contextID);
        _mesa_free(pcp);
        return NULL;
    }

    __driGarbageCollectDrawables(pcp->driScreenPriv->drawHash);

    return pcp;
}
/*@}*/


/*****************************************************************/
/** \name Screen handling functions                              */
/*****************************************************************/
/*@{*/

/**
 * Destroy the per-screen private information.
 * 
 * \param dpy the display handle.
 * \param scrn the screen number.
 * \param screenPrivate opaque pointer to the per-screen private information.
 *
 * \internal
 * This function calls __DriverAPIRec::DestroyScreen on \p screenPrivate, calls
 * drmClose(), and finally frees \p screenPrivate.
 */
static void driDestroyScreen(__DRInativeDisplay *dpy, int scrn, void *screenPrivate)
{
    __DRIscreenPrivate *psp = (__DRIscreenPrivate *) screenPrivate;

    if (psp) {
	/* No interaction with the X-server is possible at this point.  This
	 * routine is called after XCloseDisplay, so there is no protocol
	 * stream open to the X-server anymore.
	 */

	if (psp->DriverAPI.DestroyScreen)
	    (*psp->DriverAPI.DestroyScreen)(psp);

	(void)drmUnmap((drmAddress)psp->pSAREA, SAREA_MAX);
	(void)drmUnmap((drmAddress)psp->pFB, psp->fbSize);
	_mesa_free(psp->pDevPriv);
	(void)drmCloseOnce(psp->fd);
	if ( psp->modes != NULL ) {
	    (*dri_interface->destroyContextModes)( psp->modes );
	}

	assert(psp->drawHash);
	drmHashDestroy(psp->drawHash);

	_mesa_free(psp);
    }
}


/**
 * Utility function used to create a new driver-private screen structure.
 * 
 * \param dpy   Display pointer
 * \param scrn  Index of the screen
 * \param psc   DRI screen data (not driver private)
 * \param modes Linked list of known display modes.  This list is, at a
 *              minimum, a list of modes based on the current display mode.
 *              These roughly match the set of available X11 visuals, but it
 *              need not be limited to X11!  The calling libGL should create
 *              a list that will inform the driver of the current display
 *              mode (i.e., color buffer depth, depth buffer depth, etc.).
 * \param ddx_version Version of the 2D DDX.  This may not be meaningful for
 *                    all drivers.
 * \param dri_version Version of the "server-side" DRI.
 * \param drm_version Version of the kernel DRM.
 * \param frame_buffer Data describing the location and layout of the
 *                     framebuffer.
 * \param pSAREA       Pointer the the SAREA.
 * \param fd           Device handle for the DRM.
 * \param internal_api_version  Version of the internal interface between the
 *                              driver and libGL.
 * \param driverAPI Driver API functions used by other routines in dri_util.c.
 * 
 * \note
 * There is no need to check the minimum API version in this function.  Since
 * the \c __driCreateNewScreen function is versioned, it is impossible for a
 * loader that is too old to even load this driver.
 */
__DRIscreenPrivate *
__driUtilCreateNewScreen(__DRInativeDisplay *dpy, int scrn, __DRIscreen *psc,
			 __GLcontextModes * modes,
			 const __DRIversion * ddx_version,
			 const __DRIversion * dri_version,
			 const __DRIversion * drm_version,
			 const __DRIframebuffer * frame_buffer,
			 drm_sarea_t *pSAREA,
			 int fd,
			 int internal_api_version,
			 const struct __DriverAPIRec *driverAPI)
{
    __DRIscreenPrivate *psp;


    api_ver = internal_api_version;

    psp = (__DRIscreenPrivate *)_mesa_malloc(sizeof(__DRIscreenPrivate));
    if (!psp) {
	return NULL;
    }

    /* Create the hash table */
    psp->drawHash = drmHashCreate();
    if ( psp->drawHash == NULL ) {
	_mesa_free( psp );
	return NULL;
    }

    psp->display = dpy;
    psp->myNum = scrn;
    psp->psc = psc;
    psp->modes = modes;

    /*
    ** NOT_DONE: This is used by the X server to detect when the client
    ** has died while holding the drawable lock.  The client sets the
    ** drawable lock to this value.
    */
    psp->drawLockID = 1;

    psp->drmMajor = drm_version->major;
    psp->drmMinor = drm_version->minor;
    psp->drmPatch = drm_version->patch;
    psp->ddxMajor = ddx_version->major;
    psp->ddxMinor = ddx_version->minor;
    psp->ddxPatch = ddx_version->patch;
    psp->driMajor = dri_version->major;
    psp->driMinor = dri_version->minor;
    psp->driPatch = dri_version->patch;

    /* install driver's callback functions */
    memcpy( &psp->DriverAPI, driverAPI, sizeof(struct __DriverAPIRec) );

    psp->pSAREA = pSAREA;

    psp->pFB = frame_buffer->base;
    psp->fbSize = frame_buffer->size;
    psp->fbStride = frame_buffer->stride;
    psp->fbWidth = frame_buffer->width;
    psp->fbHeight = frame_buffer->height;
    psp->devPrivSize = frame_buffer->dev_priv_size;
    psp->pDevPriv = frame_buffer->dev_priv;
    psp->fbBPP = psp->fbStride * 8 / frame_buffer->width;

    psp->fd = fd;

    /*
    ** Do not init dummy context here; actual initialization will be
    ** done when the first DRI context is created.  Init screen priv ptr
    ** to NULL to let CreateContext routine that it needs to be inited.
    */
    psp->dummyContextPriv.driScreenPriv = NULL;

    psc->destroyScreen     = driDestroyScreen;
    psc->createNewDrawable = driCreateNewDrawable;
    psc->getDrawable       = driGetDrawable;
    psc->getMSC            = driGetMSC;
    psc->createNewContext  = driCreateNewContext;

    if (internal_api_version >= 20070121)
	psc->setTexOffset  = psp->DriverAPI.setTexOffset;

    if ( (psp->DriverAPI.InitDriver != NULL)
	 && !(*psp->DriverAPI.InitDriver)(psp) ) {
	_mesa_free( psp );
	return NULL;
    }


    return psp;
}


/**
 * Compare the current GLX API version with a driver supplied required version.
 * 
 * The minimum required version is compared with the API version exported by
 * the \c __glXGetInternalVersion function (in libGL.so).
 * 
 * \param   required_version Minimum required internal GLX API version.
 * \return  A tri-value return, as from strcmp is returned.  A value less
 *          than, equal to, or greater than zero will be returned if the
 *          internal GLX API version is less than, equal to, or greater
 *          than \c required_version.
 *
 * \sa __glXGetInternalVersion().
 */
int driCompareGLXAPIVersion( GLint required_version )
{
   if ( api_ver > required_version ) {
      return 1;
   }
   else if ( api_ver == required_version ) {
      return 0;
   }

   return -1;
}


static int
driQueryFrameTracking( __DRInativeDisplay * dpy, void * priv,
		       int64_t * sbc, int64_t * missedFrames,
		       float * lastMissedUsage, float * usage )
{
   __DRIswapInfo   sInfo;
   int             status;
   int64_t         ust;
   __DRIdrawablePrivate * dpriv = (__DRIdrawablePrivate *) priv;


   status = dpriv->driScreenPriv->DriverAPI.GetSwapInfo( dpriv, & sInfo );
   if ( status == 0 ) {
      *sbc = sInfo.swap_count;
      *missedFrames = sInfo.swap_missed_count;
      *lastMissedUsage = sInfo.swap_missed_usage;

      (*dri_interface->getUST)( & ust );
      *usage = driCalculateSwapUsage( dpriv, sInfo.swap_ust, ust );
   }

   return status;
}


/**
 * Calculate amount of swap interval used between GLX buffer swaps.
 * 
 * The usage value, on the range [0,max], is the fraction of total swap
 * interval time used between GLX buffer swaps is calculated.
 *
 *            \f$p = t_d / (i * t_r)\f$
 * 
 * Where \f$t_d\f$ is the time since the last GLX buffer swap, \f$i\f$ is the
 * swap interval (as set by \c glXSwapIntervalSGI), and \f$t_r\f$ time
 * required for a single vertical refresh period (as returned by \c
 * glXGetMscRateOML).
 * 
 * See the documentation for the GLX_MESA_swap_frame_usage extension for more
 * details.
 *
 * \param   dPriv  Pointer to the private drawable structure.
 * \return  If less than a single swap interval time period was required
 *          between GLX buffer swaps, a number greater than 0 and less than
 *          1.0 is returned.  If exactly one swap interval time period is
 *          required, 1.0 is returned, and if more than one is required then
 *          a number greater than 1.0 will be returned.
 *
 * \sa glXSwapIntervalSGI glXGetMscRateOML
 * 
 * \todo Instead of caching the \c glXGetMscRateOML function pointer, would it
 *       be possible to cache the sync rate?
 */
float
driCalculateSwapUsage( __DRIdrawablePrivate *dPriv, int64_t last_swap_ust,
		       int64_t current_ust )
{
   int32_t   n;
   int32_t   d;
   int       interval;
   float     usage = 1.0;


   if ( (*dri_interface->getMSCRate)( dPriv->display, dPriv->draw, &n, &d ) ) {
      interval = (dPriv->pdraw->swap_interval != 0)
	  ? dPriv->pdraw->swap_interval : 1;


      /* We want to calculate
       * (current_UST - last_swap_UST) / (interval * us_per_refresh).  We get
       * current_UST by calling __glXGetUST.  last_swap_UST is stored in
       * dPriv->swap_ust.  interval has already been calculated.
       *
       * The only tricky part is us_per_refresh.  us_per_refresh is
       * 1000000 / MSC_rate.  We know the MSC_rate is n / d.  We can flip it
       * around and say us_per_refresh = 1000000 * d / n.  Since this goes in
       * the denominator of the final calculation, we calculate
       * (interval * 1000000 * d) and move n into the numerator.
       */

      usage = (current_ust - last_swap_ust);
      usage *= n;
      usage /= (interval * d);
      usage /= 1000000.0;
   }
   
   return usage;
}

/*@}*/
