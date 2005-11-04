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
 *
 * \note
 * When \c DRI_NEW_INTERFACE_ONLY is defined, code is built / not built so
 * that only the "new" libGL-to-driver interfaces are supported.  This breaks
 * backwards compatability.  However, this may be necessary when DRI drivers
 * are built to be used in non-XFree86 environments.
 *
 * \todo There are still some places in the code that need to be wrapped with
 *       \c DRI_NEW_INTERFACE_ONLY.
 */


#ifdef GLX_DIRECT_RENDERING

#include <inttypes.h>
#include <assert.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>

#ifndef DRI_NEW_INTERFACE_ONLY
# include <X11/Xlibint.h>
# include <Xext.h>
# include <extutil.h>
# include "xf86dri.h"
# define _mesa_malloc(b) Xmalloc(b)
# define _mesa_free(m) Xfree(m)
#else
# include "imports.h"
# define None 0
#endif /* DRI_NEW_INTERFACE_ONLY */

#include "dri_util.h"
#include "drm_sarea.h"
#include "glcontextmodes.h"

#ifndef PFNGLXGETMSCRATEOMLPROC
typedef GLboolean ( * PFNGLXGETMSCRATEOMLPROC) (__DRInativeDisplay *dpy, __DRIid drawable, int32_t *numerator, int32_t *denominator);
#endif

/**
 * Weak thread-safety dispatch pointer.  Older versions of libGL will not have
 * this symbol, so a "weak" version is included here so that the driver will
 * dynamically link properly.  The value is set to \c NULL.  This forces the
 * driver to fall back to the old dispatch interface.
 */
struct _glapi_table *_glapi_DispatchTSD __attribute__((weak)) = NULL;

/**
 * This is used in a couple of places that call \c driCreateNewDrawable.
 */
static const int empty_attribute_list[1] = { None };

/**
 * Function used to determine if a drawable (window) still exists.  Ideally
 * this function comes from libGL.  With older versions of libGL from XFree86
 * we can fall-back to an internal version.
 * 
 * \sa __driWindowExists __glXWindowExists
 */
static PFNGLXWINDOWEXISTSPROC window_exists;

typedef GLboolean (*PFNGLXCREATECONTEXTWITHCONFIGPROC)( __DRInativeDisplay*, int, int, void *,
    drm_context_t * );

static PFNGLXCREATECONTEXTWITHCONFIGPROC create_context_with_config;

/**
 * Cached copy of the internal API version used by libGL and the client-side
 * DRI driver.
 */
static int api_ver = 0;

/* forward declarations */
static int driQueryFrameTracking( __DRInativeDisplay * dpy, void * priv,
    int64_t * sbc, int64_t * missedFrames, float * lastMissedUsage,
    float * usage );

static void *driCreateNewDrawable(__DRInativeDisplay *dpy, const __GLcontextModes *modes,
    __DRIid draw, __DRIdrawable *pdraw, int renderType, const int *attrs);

static void driDestroyDrawable(__DRInativeDisplay *dpy, void *drawablePrivate);




#ifdef not_defined
static GLboolean driFeatureOn(const char *name)
{
    char *env = getenv(name);

    if (!env) return GL_FALSE;
    if (!strcasecmp(env, "enable")) return GL_TRUE;
    if (!strcasecmp(env, "1"))      return GL_TRUE;
    if (!strcasecmp(env, "on"))     return GL_TRUE;
    if (!strcasecmp(env, "true"))   return GL_TRUE;
    if (!strcasecmp(env, "t"))      return GL_TRUE;
    if (!strcasecmp(env, "yes"))    return GL_TRUE;
    if (!strcasecmp(env, "y"))      return GL_TRUE;

    return GL_FALSE;
}
#endif /* not_defined */


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
/** \name Visual utility functions                               */
/*****************************************************************/
/*@{*/

#ifndef DRI_NEW_INTERFACE_ONLY
/**
 * Find a \c __GLcontextModes structure matching the given visual ID.
 * 
 * \param dpy   Display to search for a matching configuration.
 * \param scrn  Screen number on \c dpy to be searched.
 * \param vid   Desired \c VisualID to find.
 *
 * \returns A pointer to a \c __GLcontextModes structure that matches \c vid,
 *          if found, or \c NULL if no match is found.
 */
static const __GLcontextModes *
findConfigMode(__DRInativeDisplay *dpy, int scrn, VisualID vid, 
	       const __DRIscreen * pDRIScreen)
{
    if ( (pDRIScreen != NULL) && (pDRIScreen->private != NULL) ) {
	const __DRIscreenPrivate * const psp =
	    (const __DRIscreenPrivate *) pDRIScreen->private;

	return _gl_context_modes_find_visual( psp->modes, vid );
    }

    return NULL;
}


/**
 * This function is a hack to work-around old versions of libGL.so that
 * do not export \c XF86DRICreateContextWithConfig.  I would modify the
 * code to just use this function, but the stand-alone driver (i.e., DRI
 * drivers that are built to work without XFree86) shouldn't have to know
 * about X structures like a \c Visual.
 */
static GLboolean
fake_XF86DRICreateContextWithConfig( __DRInativeDisplay* dpy, int screen, int configID,
				     XID* context, drm_context_t * hHWContext )
{
    Visual  vis;
    
    vis.visualid = configID;
    return XF86DRICreateContext( dpy, screen, & vis, context, hHWContext );
}
#endif /* DRI_NEW_INTERFACE_ONLY */

/*@}*/


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

    retcode = drmHashLookup(drawHash, draw, (void **)&pdraw);
    if (retcode)
	return NULL;

    return pdraw;
}

static void __driRemoveDrawable(void *drawHash, __DRIdrawable *pdraw)
{
    int retcode;
    __DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *)pdraw->private;

    retcode = drmHashLookup(drawHash, pdp->draw, (void **)&pdraw);
    if (!retcode) { /* Found */
	drmHashDelete(drawHash, pdp->draw);
    }
}

#ifndef DRI_NEW_INTERFACE_ONLY
static GLboolean __driWindowExistsFlag;

static int __driWindowExistsErrorHandler(Display *dpy, XErrorEvent *xerr)
{
    if (xerr->error_code == BadWindow) {
	__driWindowExistsFlag = GL_FALSE;
    }
    return 0;
}

/**
 * Determine if a window associated with a \c GLXDrawable exists on the
 * X-server.
 *
 * \param dpy  Display associated with the drawable to be queried.
 * \param draw \c GLXDrawable to test.
 * 
 * \returns \c GL_TRUE if a window exists that is associated with \c draw,
 *          otherwise \c GL_FALSE is returned.
 * 
 * \warning This function is not currently thread-safe.
 *
 * \deprecated
 * \c __glXWindowExists (from libGL) is prefered over this function.  Starting
 * with the next major release of XFree86, this function will be removed.
 * Even now this function is no longer directly called.  Instead it is called
 * via a function pointer if and only if \c __glXWindowExists does not exist.
 * 
 * \sa __glXWindowExists glXGetProcAddress window_exists
 */
static GLboolean __driWindowExists(Display *dpy, GLXDrawable draw)
{
    XWindowAttributes xwa;
    int (*oldXErrorHandler)(Display *, XErrorEvent *);

    XSync(dpy, GL_FALSE);
    __driWindowExistsFlag = GL_TRUE;
    oldXErrorHandler = XSetErrorHandler(__driWindowExistsErrorHandler);
    XGetWindowAttributes(dpy, draw, &xwa); /* dummy request */
    XSetErrorHandler(oldXErrorHandler);
    return __driWindowExistsFlag;
}
#endif /* DRI_NEW_INTERFACE_ONLY */

/**
 * Find drawables in the local hash that have been destroyed on the
 * server.
 * 
 * \param drawHash  Hash-table containing all know drawables.
 */
static void __driGarbageCollectDrawables(void *drawHash)
{
    __DRIid draw;
    __DRIdrawable *pdraw;
    __DRInativeDisplay *dpy;

    if (drmHashFirst(drawHash, &draw, (void **)&pdraw)) {
	do {
	    __DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *)pdraw->private;
	    dpy = pdp->driScreenPriv->display;
	    if (! (*window_exists)(dpy, draw)) {
		/* Destroy the local drawable data in the hash table, if the
		   drawable no longer exists in the Xserver */
		__driRemoveDrawable(drawHash, pdraw);
		(*pdraw->destroyDrawable)(dpy, pdraw->private);
		_mesa_free(pdraw);
	    }
	} while (drmHashNext(drawHash, &draw, (void **)&pdraw));
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
static GLboolean driUnbindContext3(__DRInativeDisplay *dpy, int scrn,
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
    ** calling driUnbindContext3.
    */

    if (ctx == NULL || draw == None || read == None) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    pDRIScreen = __glXFindDRIScreen(dpy, scrn);
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

    /* Call device-specific MakeCurrent */
    (*psp->DriverAPI.MakeCurrent)(pcp, pdp, prp);

    return GL_TRUE;
}


/**
 * This function takes both a read buffer and a draw buffer.  This is needed
 * for \c glXMakeCurrentReadSGI or GLX 1.3's \c glXMakeContextCurrent
 * function.
 */
static GLboolean driBindContext3(__DRInativeDisplay *dpy, int scrn,
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

    pDRIScreen = __glXFindDRIScreen(dpy, scrn);
    if ( (pDRIScreen == NULL) || (pDRIScreen->private == NULL) ) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    return DoBindContext( dpy, draw, read, ctx, ctx->mode,
			  (__DRIscreenPrivate *)pDRIScreen->private );
}


#ifndef DRI_NEW_INTERFACE_ONLY
/**
 * This function takes both a read buffer and a draw buffer.  This is needed
 * for \c glXMakeCurrentReadSGI or GLX 1.3's \c glXMakeContextCurrent
 * function.
 */
static GLboolean driBindContext2(Display *dpy, int scrn,
                            GLXDrawable draw, GLXDrawable read,
                            GLXContext gc)
{
    __DRIscreen *pDRIScreen;
    const __GLcontextModes *modes;

    /*
    ** Assume error checking is done properly in glXMakeCurrent before
    ** calling driBindContext.
    */

    if (gc == NULL || draw == None || read == None) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    pDRIScreen = __glXFindDRIScreen(dpy, scrn);
    modes = (driCompareGLXAPIVersion( 20040317 ) >= 0)
	? gc->driContext.mode
	: findConfigMode( dpy, scrn, gc->vid, pDRIScreen );

    if ( modes == NULL ) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    /* findConfigMode will return NULL if the DRI screen or screen private
     * are NULL.
     */
    assert( (pDRIScreen != NULL) && (pDRIScreen->private != NULL) );

    return DoBindContext( dpy, draw, read, & gc->driContext, modes,
			  (__DRIscreenPrivate *)pDRIScreen->private );
}

static GLboolean driUnbindContext2(Display *dpy, int scrn,
			      GLXDrawable draw, GLXDrawable read,
			      GLXContext gc)
{
    return driUnbindContext3(dpy, scrn, draw, read, & gc->driContext);
}

/*
 * Simply call bind with the same GLXDrawable for the read and draw buffers.
 */
static GLboolean driBindContext(Display *dpy, int scrn,
                           GLXDrawable draw, GLXContext gc)
{
    return driBindContext2(dpy, scrn, draw, draw, gc);
}


/*
 * Simply call bind with the same GLXDrawable for the read and draw buffers.
 */
static GLboolean driUnbindContext(Display *dpy, int scrn,
                             GLXDrawable draw, GLXContext gc,
                             int will_rebind)
{
   (void) will_rebind;
   return driUnbindContext2( dpy, scrn, draw, draw, gc );
}
#endif /* DRI_NEW_INTERFACE_ONLY */

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
 * cliprect information by calling \c __DRIDrawablePrivate::getInfo.  This is
 * usually called by the DRI_VALIDATE_DRAWABLE_INFO macro which
 * compares the __DRIdrwablePrivate pStamp and lastStamp values.  If
 * the values are different that means we have to update the clipping
 * info.
 */
void
__driUtilUpdateDrawableInfo(__DRIdrawablePrivate *pdp)
{
    __DRIscreenPrivate *psp;
    __DRIcontextPrivate *pcp = pdp->driContextPriv;
    
    if (!pcp || (pdp != pcp->driDrawablePriv)) {
	/* ERROR!!! */
	return;
    }

    psp = pdp->driScreenPriv;
    if (!psp) {
	/* ERROR!!! */
	return;
    }

    if (pdp->pClipRects) {
	_mesa_free(pdp->pClipRects); 
    }

    if (pdp->pBackClipRects) {
	_mesa_free(pdp->pBackClipRects); 
    }

    DRM_SPINUNLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);

    if (!__driFindDrawable(psp->drawHash, pdp->draw) ||
	! (*pdp->getInfo)(pdp->display, pdp->screen, pdp->draw,
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
    dPriv->swapBuffers(dPriv);
    (void) dpy;
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
    __DRIscreen * const pDRIScreen = __glXFindDRIScreen(dpy, modes->screen);
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

    if (!XF86DRICreateDrawable(dpy, modes->screen, draw, &pdp->hHWDrawable)) {
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

    pdp->getInfo = (PFNGLXGETDRAWABLEINFOPROC)
	glXGetProcAddress( (const GLubyte *) "__glXGetDrawableInfo" );
    if ( pdp->getInfo == NULL ) {
#ifdef DRI_NEW_INTERFACE_ONLY
        (void)XF86DRIDestroyDrawable(dpy, modes->screen, pdp->draw);
	_mesa_free(pdp);
	return NULL;
#else
	pdp->getInfo = XF86DRIGetDrawableInfo;
#endif /* DRI_NEW_INTERFACE_ONLY */
    }

    if (!(*psp->DriverAPI.CreateBuffer)(psp, pdp, modes,
					renderType == GLX_PIXMAP_BIT)) {
       (void)XF86DRIDestroyDrawable(dpy, modes->screen, pdp->draw);
       _mesa_free(pdp);
       return NULL;
    }

    pdraw->private = pdp;
    pdraw->destroyDrawable = driDestroyDrawable;
    pdraw->swapBuffers = driSwapBuffers;  /* called by glXSwapBuffers() */

    if ( driCompareGLXAPIVersion( 20030317 ) >= 0 ) {
        pdraw->getSBC = driGetSBC;
        pdraw->waitForSBC = driWaitForSBC;
        pdraw->waitForMSC = driWaitForMSC;
        pdraw->swapBuffersMSC = driSwapBuffersMSC;
        pdraw->frameTracking = NULL;
        pdraw->queryFrameTracking = driQueryFrameTracking;

        /* This special default value is replaced with the configured
	 * default value when the drawable is first bound to a direct
	 * rendering context. */
        pdraw->swap_interval = (unsigned)-1;
    }

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

static __DRIdrawable *driGetDrawable(__DRInativeDisplay *dpy, __DRIid draw,
					 void *screenPrivate)
{
    __DRIscreenPrivate *psp = (__DRIscreenPrivate *) screenPrivate;

    /*
    ** Make sure this routine returns NULL if the drawable is not bound
    ** to a direct rendering context!
    */
    return __driFindDrawable(psp->drawHash, draw);
}

static void driDestroyDrawable(__DRInativeDisplay *dpy, void *drawablePrivate)
{
    __DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *) drawablePrivate;
    __DRIscreenPrivate *psp = pdp->driScreenPriv;
    int scrn = psp->myNum;

    if (pdp) {
        (*psp->DriverAPI.DestroyBuffer)(pdp);
	if ((*window_exists)(dpy, pdp->draw))
	    (void)XF86DRIDestroyDrawable(dpy, scrn, pdp->draw);
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
static void driDestroyContext(__DRInativeDisplay *dpy, int scrn, void *contextPrivate)
{
    __DRIcontextPrivate  *pcp   = (__DRIcontextPrivate *) contextPrivate;

    if (pcp) {
	(*pcp->driScreenPriv->DriverAPI.DestroyContext)(pcp);
	__driGarbageCollectDrawables(pcp->driScreenPriv->drawHash);
	(void)XF86DRIDestroyContext(dpy, scrn, pcp->contextID);
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

    pDRIScreen = __glXFindDRIScreen(dpy, modes->screen);
    if ( (pDRIScreen == NULL) || (pDRIScreen->private == NULL) ) {
	/* ERROR!!! */
	return NULL;
    } 

    psp = (__DRIscreenPrivate *)pDRIScreen->private;

    pcp = (__DRIcontextPrivate *)_mesa_malloc(sizeof(__DRIcontextPrivate));
    if (!pcp) {
	return NULL;
    }

    if (! (*create_context_with_config)(dpy, modes->screen, modes->fbconfigID,
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
#ifdef DRI_NEW_INTERFACE_ONLY
    pctx->bindContext    = NULL;
    pctx->unbindContext  = NULL;
    pctx->bindContext2   = NULL;
    pctx->unbindContext2 = NULL;
    pctx->bindContext3   = driBindContext3;
    pctx->unbindContext3 = driUnbindContext3;
#else
    pctx->bindContext    = driBindContext;
    pctx->unbindContext  = driUnbindContext;
    if ( driCompareGLXAPIVersion( 20030606 ) >= 0 ) {
        pctx->bindContext2   = driBindContext2;
        pctx->unbindContext2 = driUnbindContext2;
    }

    if ( driCompareGLXAPIVersion( 20040415 ) >= 0 ) {
        pctx->bindContext3   = driBindContext3;
        pctx->unbindContext3 = driUnbindContext3;
    }
#endif

    if ( !(*psp->DriverAPI.CreateContext)(modes, pcp, shareCtx) ) {
        (void)XF86DRIDestroyContext(dpy, modes->screen, pcp->contextID);
        _mesa_free(pcp);
        return NULL;
    }

    __driGarbageCollectDrawables(pcp->driScreenPriv->drawHash);

    return pcp;
}


#ifndef DRI_NEW_INTERFACE_ONLY
/**
 * Create the per-drawable private driver information.
 * 
 * \param dpy the display handle.
 * \param vis the visual information.
 * \param sharedPrivate the shared context dependent methods or \c NULL if
 *                      non-existent.
 * \param pctx will receive the context dependent methods.
 *
 * \returns a opaque pointer to the per-context private information on success, or \c NULL
 * on failure.
 * 
 * \deprecated
 * This function has been replaced by \c driCreateNewContext.  In drivers
 * built to work with XFree86, this function will continue to exist to support
 * older versions of libGL.  Starting with the next major relelase of XFree86,
 * this function will be removed.
 * 
 * \internal
 * This function allocates and fills a __DRIcontextPrivateRec structure.  It
 * gets the visual, converts it into a __GLcontextModesRec and passes it
 * to __DriverAPIRec::CreateContext to create the context.
 */
static void *driCreateContext(Display *dpy, XVisualInfo *vis,
                              void *sharedPrivate, __DRIcontext *pctx)
{
    __DRIscreen *pDRIScreen;
    const __GLcontextModes *modes;

    pDRIScreen = __glXFindDRIScreen(dpy, vis->screen);
    if ( (pDRIScreen == NULL) || (pDRIScreen->private == NULL) ) {
	/* ERROR!!! */
	return NULL;
    } 


    /* Setup a __GLcontextModes struct corresponding to vis->visualid
     * and create the rendering context.
     */

    modes = findConfigMode(dpy, vis->screen, vis->visualid, pDRIScreen);
    return (modes == NULL) 
	? NULL
	: driCreateNewContext( dpy, modes, GLX_RGBA_TYPE,
			       sharedPrivate, pctx );
}
#endif /* DRI_NEW_INTERFACE_ONLY */

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
	(void)drmClose(psp->fd);
	if ( psp->modes != NULL ) {
	    _gl_context_modes_destroy( psp->modes );
	}
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


#ifdef DRI_NEW_INTERFACE_ONLY
    if ( internal_api_version < 20040602 ) {
	fprintf( stderr, "libGL error: libGL.so version (%08u) is too old.  "
		 "20040602 or later is required.\n", internal_api_version );
	return NULL;
    }
#else
    if ( internal_api_version == 20031201 ) {
	fprintf( stderr, "libGL error: libGL version 20031201 has critical "
		 "binary compatilibity bugs.\nlibGL error: You must upgrade "
		 "to use direct-rendering!\n" );
	return NULL;
    }
#endif /* DRI_NEW_INTERFACE_ONLY */


    window_exists = (PFNGLXWINDOWEXISTSPROC)
	glXGetProcAddress( (const GLubyte *) "__glXWindowExists" );

    if ( window_exists == NULL ) {
#ifdef DRI_NEW_INTERFACE_ONLY
	fprintf( stderr, "libGL error: libGL.so version (%08u) is too old.  "
		 "20021128 or later is required.\n", internal_api_version );
	return NULL;
#else
	window_exists = (PFNGLXWINDOWEXISTSPROC) __driWindowExists;
#endif /* DRI_NEW_INTERFACE_ONLY */
    }

    create_context_with_config = (PFNGLXCREATECONTEXTWITHCONFIGPROC)
	glXGetProcAddress( (const GLubyte *) "__glXCreateContextWithConfig" );
    if ( create_context_with_config == NULL ) {
#ifdef DRI_NEW_INTERFACE_ONLY
	fprintf( stderr, "libGL error: libGL.so version (%08u) is too old.  "
		 "20031201 or later is required.\n", internal_api_version );
	return NULL;
#else
	create_context_with_config = (PFNGLXCREATECONTEXTWITHCONFIGPROC)
	    fake_XF86DRICreateContextWithConfig;
#endif /* DRI_NEW_INTERFACE_ONLY */
    }
	
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

    psp->fd = fd;

    /*
    ** Do not init dummy context here; actual initialization will be
    ** done when the first DRI context is created.  Init screen priv ptr
    ** to NULL to let CreateContext routine that it needs to be inited.
    */
    psp->dummyContextPriv.driScreenPriv = NULL;

    psc->destroyScreen     = driDestroyScreen;
#ifndef DRI_NEW_INTERFACE_ONLY
    psc->createContext     = driCreateContext;
#else
    psc->createContext     = NULL;
#endif
    psc->createNewDrawable = driCreateNewDrawable;
    psc->getDrawable       = driGetDrawable;
#ifdef DRI_NEW_INTERFACE_ONLY
    psc->getMSC            = driGetMSC;
    psc->createNewContext  = driCreateNewContext;
#else
    if ( driCompareGLXAPIVersion( 20030317 ) >= 0 ) {
        psc->getMSC        = driGetMSC;

	if ( driCompareGLXAPIVersion( 20030824 ) >= 0 ) {
	    psc->createNewContext = driCreateNewContext;
	}
    }
#endif

    if ( (psp->DriverAPI.InitDriver != NULL)
	 && !(*psp->DriverAPI.InitDriver)(psp) ) {
	_mesa_free( psp );
	return NULL;
    }


    return psp;
}


#ifndef DRI_NEW_INTERFACE_ONLY
/**
 * Utility function used to create a new driver-private screen structure.
 * 
 * \param dpy        Display pointer.
 * \param scrn       Index of the screen.
 * \param psc        DRI screen data (not driver private)
 * \param numConfigs Number of visual configs pointed to by \c configs.
 * \param configs    Array of GLXvisualConfigs exported by the 2D driver.
 * \param driverAPI Driver API functions used by other routines in dri_util.c.
 * 
 * \deprecated
 * This function has been replaced by \c __driUtilCreateNewScreen.  In drivers
 * built to work with XFree86, this function will continue to exist to support
 * older versions of libGL.  Starting with the next major relelase of XFree86,
 * this function will be removed.
 */
__DRIscreenPrivate *
__driUtilCreateScreen(Display *dpy, int scrn, __DRIscreen *psc,
                      int numConfigs, __GLXvisualConfig *configs,
                      const struct __DriverAPIRec *driverAPI)
{
    int directCapable;
    __DRIscreenPrivate *psp = NULL;
    drm_handle_t hSAREA;
    drmAddress pSAREA;
    char *BusID;
    __GLcontextModes *modes;
    __GLcontextModes *temp;
    int   i;
    __DRIversion   ddx_version;
    __DRIversion   dri_version;
    __DRIversion   drm_version;
    __DRIframebuffer  framebuffer;
    int   fd = -1;
    int   status;
    const char * err_msg;
    const char * err_extra;


    if (!XF86DRIQueryDirectRenderingCapable(dpy, scrn, &directCapable)
	|| !directCapable) {
	return NULL;
    }


    /* Create the linked list of context modes, and populate it with the
     * GLX visual information passed in by libGL.
     */

    modes = _gl_context_modes_create( numConfigs, sizeof(__GLcontextModes) );
    if ( modes == NULL ) {
	return NULL;
    }

    temp = modes;
    for ( i = 0 ; i < numConfigs ; i++ ) {
	assert( temp != NULL );
	_gl_copy_visual_to_context_mode( temp, & configs[i] );
	temp->screen = scrn;

	temp = temp->next;
    }

    err_msg = "XF86DRIOpenConnection";
    err_extra = NULL;

    if (XF86DRIOpenConnection(dpy, scrn, &hSAREA, &BusID)) {
	fd = drmOpen(NULL,BusID);
	_mesa_free(BusID); /* No longer needed */

	err_msg = "open DRM";
	err_extra = strerror( -fd );

	if (fd >= 0) {
	    drm_magic_t magic;

	    err_msg = "drmGetMagic";
	    err_extra = NULL;

	    if (!drmGetMagic(fd, &magic)) {
		drmVersionPtr version = drmGetVersion(fd);
		if (version) {
		    drm_version.major = version->version_major;
		    drm_version.minor = version->version_minor;
		    drm_version.patch = version->version_patchlevel;
		    drmFreeVersion(version);
		}
		else {
		    drm_version.major = -1;
		    drm_version.minor = -1;
		    drm_version.patch = -1;
		}

		err_msg = "XF86DRIAuthConnection";
		if (XF86DRIAuthConnection(dpy, scrn, magic)) {
		    char *driverName;

		    /*
		     * Get device name (like "tdfx") and the ddx version numbers.
		     * We'll check the version in each DRI driver's "createScreen"
		     * function.
		     */
		    err_msg = "XF86DRIGetClientDriverName";
		    if (XF86DRIGetClientDriverName(dpy, scrn,
						   &ddx_version.major,
						   &ddx_version.minor,
						   &ddx_version.patch,
						   &driverName)) {

			/* No longer needed. */
			_mesa_free( driverName );

			/*
			 * Get the DRI X extension version.
			 */
			err_msg = "XF86DRIQueryVersion";
			if (XF86DRIQueryVersion(dpy,
						&dri_version.major,
						&dri_version.minor,
						&dri_version.patch)) {
			    drm_handle_t  hFB;
			    int        junk;

			    /*
			     * Get device-specific info.  pDevPriv will point to a struct
			     * (such as DRIRADEONRec in xfree86/driver/ati/radeon_dri.h)
			     * that has information about the screen size, depth, pitch,
			     * ancilliary buffers, DRM mmap handles, etc.
			     */
			    err_msg = "XF86DRIGetDeviceInfo";
			    if (XF86DRIGetDeviceInfo(dpy, scrn,
						     &hFB,
						     &junk,
						     &framebuffer.size,
						     &framebuffer.stride,
						     &framebuffer.dev_priv_size,
						     &framebuffer.dev_priv)) {
				framebuffer.width = DisplayWidth(dpy, scrn);
				framebuffer.height = DisplayHeight(dpy, scrn);

				/*
				 * Map the framebuffer region.
				 */
				status = drmMap(fd, hFB, framebuffer.size, 
						(drmAddressPtr)&framebuffer.base);
				
				err_msg = "drmMap of framebuffer";
				err_extra = strerror( -status );

				if ( status == 0 ) {
				    /*
				     * Map the SAREA region.  Further mmap regions may be setup in
				     * each DRI driver's "createScreen" function.
				     */
				    status = drmMap(fd, hSAREA, SAREA_MAX, 
						    &pSAREA);

				    err_msg = "drmMap of sarea";
				    err_extra = strerror( -status );

				    if ( status == 0 ) {
					PFNGLXGETINTERNALVERSIONPROC get_ver;

					get_ver = (PFNGLXGETINTERNALVERSIONPROC)
					    glXGetProcAddress( (const GLubyte *) "__glXGetInternalVersion" );

					err_msg = "InitDriver";
					err_extra = NULL;
					psp = __driUtilCreateNewScreen( dpy, scrn, psc, modes,
									& ddx_version,
									& dri_version,
									& drm_version,
									& framebuffer,
									pSAREA,
									fd,
									(get_ver != NULL) ? (*get_ver)() : 1,
									driverAPI );
				    }
				}
			    }
			}
		    }
		}
	    }
	}
    }

    if ( psp == NULL ) {
	if ( pSAREA != MAP_FAILED ) {
	    (void)drmUnmap(pSAREA, SAREA_MAX);
	}

	if ( framebuffer.base != MAP_FAILED ) {
	    (void)drmUnmap((drmAddress)framebuffer.base, framebuffer.size);
	}

	if ( framebuffer.dev_priv != NULL ) {
	    _mesa_free(framebuffer.dev_priv);
	}

	if ( fd >= 0 ) {
	    (void)drmClose(fd);
	}

	if ( modes != NULL ) {
	    _gl_context_modes_destroy( modes );
	}

	(void)XF86DRICloseConnection(dpy, scrn);

	if ( err_extra != NULL ) {
	    fprintf(stderr, "libGL error: %s failed (%s)\n", err_msg,
		    err_extra);
	}
	else {
	    fprintf(stderr, "libGL error: %s failed\n", err_msg );
	}

        fprintf(stderr, "libGL error: reverting to (slow) indirect rendering\n");
    }

    return psp;
}
#endif /* DRI_NEW_INTERFACE_ONLY */


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
int driCompareGLXAPIVersion( GLuint required_version )
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
   static PFNGLXGETUSTPROC   get_ust;
   __DRIswapInfo   sInfo;
   int             status;
   int64_t         ust;
   __DRIdrawablePrivate * dpriv = (__DRIdrawablePrivate *) priv;

   if ( get_ust == NULL ) {
      get_ust = (PFNGLXGETUSTPROC) glXGetProcAddress( (const GLubyte *) "__glXGetUST" );
   }

   status = dpriv->driScreenPriv->DriverAPI.GetSwapInfo( dpriv, & sInfo );
   if ( status == 0 ) {
      *sbc = sInfo.swap_count;
      *missedFrames = sInfo.swap_missed_count;
      *lastMissedUsage = sInfo.swap_missed_usage;

      (*get_ust)( & ust );
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
   static PFNGLXGETMSCRATEOMLPROC get_msc_rate = NULL;
   int32_t   n;
   int32_t   d;
   int       interval;
   float     usage = 1.0;


   if ( get_msc_rate == NULL ) {
      get_msc_rate = (PFNGLXGETMSCRATEOMLPROC)
	  glXGetProcAddress( (const GLubyte *) "glXGetMscRateOML" );
   }
   
   if ( (get_msc_rate != NULL)
	&& get_msc_rate( dPriv->display, dPriv->draw, &n, &d ) ) {
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

#endif /* GLX_DIRECT_RENDERING */
