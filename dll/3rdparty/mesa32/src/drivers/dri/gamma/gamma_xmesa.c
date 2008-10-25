/* $XFree86: xc/lib/GL/mesa/src/drv/gamma/gamma_xmesa.c,v 1.14 2002/10/30 12:51:30 alanh Exp $ */
/*
 * Copyright 2001 by Alan Hourihane.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, <alanh@tungstengraphics.com>
 *
 * 3DLabs Gamma driver
 */

#include "gamma_context.h"
#include "gamma_vb.h"
#include "context.h"
#include "matrix.h"
#include "glint_dri.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "vbo/vbo.h"

static GLboolean 
gammaInitDriver(__DRIscreenPrivate *sPriv)
{
    sPriv->private = (void *) gammaCreateScreen( sPriv );

    if (!sPriv->private) {
	gammaDestroyScreen( sPriv );
	return GL_FALSE;
    }

    return GL_TRUE;
}

static void 
gammaDestroyContext(__DRIcontextPrivate *driContextPriv)
{
    gammaContextPtr gmesa = (gammaContextPtr)driContextPriv->driverPrivate;

    if (gmesa) {
      _swsetup_DestroyContext( gmesa->glCtx );
      _tnl_DestroyContext( gmesa->glCtx );
      _vbo_DestroyContext( gmesa->glCtx );
      _swrast_DestroyContext( gmesa->glCtx );

      gammaFreeVB( gmesa->glCtx );

      /* free the Mesa context */
      gmesa->glCtx->DriverCtx = NULL;
      _mesa_destroy_context(gmesa->glCtx);

      FREE(gmesa);
      driContextPriv->driverPrivate = NULL;
    }
}


static GLboolean
gammaCreateBuffer( __DRIscreenPrivate *driScrnPriv,
                   __DRIdrawablePrivate *driDrawPriv,
                   const __GLcontextModes *mesaVis,
                   GLboolean isPixmap )
{
   if (isPixmap) {
      return GL_FALSE; /* not implemented */
   }
   else {
      driDrawPriv->driverPrivate = (void *) 
         _mesa_create_framebuffer(mesaVis,
                                  GL_FALSE,  /* software depth buffer? */
                                  mesaVis->stencilBits > 0,
                                  mesaVis->accumRedBits > 0,
                                  mesaVis->alphaBits > 0
                                  );
      return (driDrawPriv->driverPrivate != NULL);
   }
}


static void
gammaDestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_unreference_framebuffer((GLframebuffer **)(&(driDrawPriv->driverPrivate)));
}

static void
gammaSwapBuffers( __DRIdrawablePrivate *dPriv )
{
   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
    gammaContextPtr gmesa;
    __DRIscreenPrivate *driScrnPriv;
    GLcontext *ctx;

    gmesa = (gammaContextPtr) dPriv->driContextPriv->driverPrivate;
    ctx = gmesa->glCtx;
    driScrnPriv = gmesa->driScreen;

    _mesa_notifySwapBuffers(ctx);

    VALIDATE_DRAWABLE_INFO(gmesa);

    /* Flush any partially filled buffers */
    FLUSH_DMA_BUFFER(gmesa);

    DRM_SPINLOCK(&driScrnPriv->pSAREA->drawable_lock,
		     driScrnPriv->drawLockID);
    VALIDATE_DRAWABLE_INFO_NO_LOCK(gmesa);

    if (gmesa->EnabledFlags & GAMMA_BACK_BUFFER) {
	int src, dst, x0, y0, x1, h;
	int i;
	int nRect = dPriv->numClipRects;
	drm_clip_rect_t *pRect = dPriv->pClipRects;
	__DRIscreenPrivate *driScrnPriv = gmesa->driScreen;
   	GLINTDRIPtr gDRIPriv = (GLINTDRIPtr)driScrnPriv->pDevPriv;

	CHECK_DMA_BUFFER(gmesa, 2);
	WRITE(gmesa->buf, FBReadMode, (gmesa->FBReadMode |
					 FBReadSrcEnable));
	WRITE(gmesa->buf, LBWriteMode, LBWriteModeDisable);

	for (i = 0; i < nRect; i++, pRect++) {
	    x0 = pRect->x1;
	    x1 = pRect->x2;
	    h  = pRect->y2 - pRect->y1;

	    y0 = driScrnPriv->fbHeight - (pRect->y1+h);
	    if (gDRIPriv->numMultiDevices == 2) 
	    	src = (y0/2)*driScrnPriv->fbWidth+x0;
	    else
	    	src = y0*driScrnPriv->fbWidth+x0;

	    y0 += driScrnPriv->fbHeight;
	    if (gDRIPriv->numMultiDevices == 2) 
	    	dst = (y0/2)*driScrnPriv->fbWidth+x0;
	    else
	    	dst = y0*driScrnPriv->fbWidth+x0;

	    CHECK_DMA_BUFFER(gmesa, 9);
	    WRITE(gmesa->buf, StartXDom,       x0<<16);   /* X0dest */
	    WRITE(gmesa->buf, StartY,          y0<<16);   /* Y0dest */
	    WRITE(gmesa->buf, StartXSub,       x1<<16);   /* X1dest */
	    WRITE(gmesa->buf, GLINTCount,      h);        /* H */
	    WRITE(gmesa->buf, dY,              1<<16);    /* ydir */
	    WRITE(gmesa->buf, dXDom,           0<<16);
	    WRITE(gmesa->buf, dXSub,           0<<16);
	    WRITE(gmesa->buf, FBSourceOffset, (dst-src));
	    WRITE(gmesa->buf, Render,          0x00040048); /* NOT_DONE */
	}

	/*
	** NOTE: FBSourceOffset (above) is backwards from what is
	** described in the manual (i.e., dst-src instead of src-dst)
	** due to our using the bottom-left window origin instead of the
	** top-left window origin.
	*/

	/* Restore FBReadMode */
	CHECK_DMA_BUFFER(gmesa, 2);
	WRITE(gmesa->buf, FBReadMode, (gmesa->FBReadMode |
				       gmesa->AB_FBReadMode));
	WRITE(gmesa->buf, LBWriteMode, LBWriteModeEnable);
    }

    if (gmesa->EnabledFlags & GAMMA_BACK_BUFFER)
        PROCESS_DMA_BUFFER_TOP_HALF(gmesa);

    DRM_SPINUNLOCK(&driScrnPriv->pSAREA->drawable_lock,
		       driScrnPriv->drawLockID);
    VALIDATE_DRAWABLE_INFO_NO_LOCK_POST(gmesa);

    if (gmesa->EnabledFlags & GAMMA_BACK_BUFFER)
        PROCESS_DMA_BUFFER_BOTTOM_HALF(gmesa);
  } else {
    _mesa_problem(NULL, "gammaSwapBuffers: drawable has no context!\n");
  }
}

static GLboolean 
gammaMakeCurrent(__DRIcontextPrivate *driContextPriv,
		 __DRIdrawablePrivate *driDrawPriv,
		 __DRIdrawablePrivate *driReadPriv)
{
    if (driContextPriv) {
	GET_CURRENT_CONTEXT(ctx);
	gammaContextPtr oldGammaCtx = ctx ? GAMMA_CONTEXT(ctx) : NULL;
	gammaContextPtr newGammaCtx = (gammaContextPtr) driContextPriv->driverPrivate;

	if ( newGammaCtx != oldGammaCtx ) {
	    newGammaCtx->dirty = ~0;
	}

	if (newGammaCtx->driDrawable != driDrawPriv) {
	    newGammaCtx->driDrawable = driDrawPriv;
	    gammaUpdateWindow ( newGammaCtx->glCtx );
	    gammaUpdateViewportOffset( newGammaCtx->glCtx );
	}

#if 0
	newGammaCtx->Window &= ~W_GIDMask;
	newGammaCtx->Window |= (driDrawPriv->index << 5);
	CHECK_DMA_BUFFER(newGammaCtx,1);
	WRITE(newGammaCtx->buf, GLINTWindow, newGammaCtx->Window);
#endif

newGammaCtx->new_state |= GAMMA_NEW_WINDOW; /* FIXME */

	_mesa_make_current2( newGammaCtx->glCtx, 
			  (GLframebuffer *) driDrawPriv->driverPrivate,
			  (GLframebuffer *) driReadPriv->driverPrivate );
    } else {
	_mesa_make_current( 0, 0 );
    }
    return GL_TRUE;
}


static GLboolean 
gammaUnbindContext( __DRIcontextPrivate *driContextPriv )
{
   return GL_TRUE;
}

static struct __DriverAPIRec gammaAPI = {
   gammaInitDriver,
   gammaDestroyScreen,
   gammaCreateContext,
   gammaDestroyContext,
   gammaCreateBuffer,
   gammaDestroyBuffer,
   gammaSwapBuffers,
   gammaMakeCurrent,
   gammaUnbindContext
};



/*
 * This is the bootstrap function for the driver.
 * The __driCreateScreen name is the symbol that libGL.so fetches.
 * Return:  pointer to a __DRIscreenPrivate.
 */
void *__driCreateScreen(Display *dpy, int scrn, __DRIscreen *psc,
                        int numConfigs, __GLXvisualConfig *config)
{
   __DRIscreenPrivate *psp;
   psp = __driUtilCreateScreen(dpy, scrn, psc, numConfigs, config, &gammaAPI);
   return (void *) psp;
}
