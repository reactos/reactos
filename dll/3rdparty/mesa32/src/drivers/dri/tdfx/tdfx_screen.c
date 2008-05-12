/* -*- mode: c; c-basic-offset: 3 -*-
 *
 * Copyright 2000 VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/tdfx/tdfx_screen.c,v 1.3 2002/02/22 21:45:03 dawes Exp $ */

/*
 * Original rewrite:
 *	Gareth Hughes <gareth@valinux.com>, 29 Sep - 1 Oct 2000
 *
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *
 */

#include "tdfx_dri.h"
#include "tdfx_context.h"
#include "tdfx_lock.h"
#include "tdfx_vb.h"
#include "tdfx_span.h"
#include "tdfx_tris.h"

#include "framebuffer.h"
#include "renderbuffer.h"
#include "xmlpool.h"

#include "utils.h"

#ifdef DEBUG_LOCKING
char *prevLockFile = 0;
int prevLockLine = 0;
#endif

#ifndef TDFX_DEBUG
int TDFX_DEBUG = 0;
#endif

PUBLIC const char __driConfigOptions[] =
DRI_CONF_BEGIN
    DRI_CONF_SECTION_DEBUG
        DRI_CONF_NO_RAST(false)
    DRI_CONF_SECTION_END
DRI_CONF_END;

static const GLuint __driNConfigOptions = 1;

extern const struct dri_extension card_extensions[];
extern const struct dri_extension napalm_extensions[];

static GLboolean
tdfxCreateScreen( __DRIscreenPrivate *sPriv )
{
   tdfxScreenPrivate *fxScreen;
   TDFXDRIPtr fxDRIPriv = (TDFXDRIPtr) sPriv->pDevPriv;
   PFNGLXSCRENABLEEXTENSIONPROC glx_enable_extension =
     (PFNGLXSCRENABLEEXTENSIONPROC) (*dri_interface->getProcAddress("glxEnableExtension"));
   void *const psc = sPriv->psc->screenConfigs;

   if (sPriv->devPrivSize != sizeof(TDFXDRIRec)) {
      fprintf(stderr,"\nERROR!  sizeof(TDFXDRIRec) does not match passed size from device driver\n");
      return GL_FALSE;
   }

   /* Allocate the private area */
   fxScreen = (tdfxScreenPrivate *) CALLOC( sizeof(tdfxScreenPrivate) );
   if ( !fxScreen )
      return GL_FALSE;

   /* parse information in __driConfigOptions */
   driParseOptionInfo (&fxScreen->optionCache,
		       __driConfigOptions, __driNConfigOptions);

   fxScreen->driScrnPriv = sPriv;
   sPriv->private = (void *) fxScreen;

   fxScreen->regs.handle	= fxDRIPriv->regs;
   fxScreen->regs.size		= fxDRIPriv->regsSize;
   fxScreen->deviceID		= fxDRIPriv->deviceID;
   fxScreen->width		= fxDRIPriv->width;
   fxScreen->height		= fxDRIPriv->height;
   fxScreen->mem		= fxDRIPriv->mem;
   fxScreen->cpp		= fxDRIPriv->cpp;
   fxScreen->stride		= fxDRIPriv->stride;
   fxScreen->fifoOffset		= fxDRIPriv->fifoOffset;
   fxScreen->fifoSize		= fxDRIPriv->fifoSize;
   fxScreen->fbOffset		= fxDRIPriv->fbOffset;
   fxScreen->backOffset		= fxDRIPriv->backOffset;
   fxScreen->depthOffset	= fxDRIPriv->depthOffset;
   fxScreen->textureOffset	= fxDRIPriv->textureOffset;
   fxScreen->textureSize	= fxDRIPriv->textureSize;
   fxScreen->sarea_priv_offset	= fxDRIPriv->sarea_priv_offset;

   if ( drmMap( sPriv->fd, fxScreen->regs.handle,
		fxScreen->regs.size, &fxScreen->regs.map ) ) {
      return GL_FALSE;
   }

   if (glx_enable_extension != NULL) {
      (*glx_enable_extension)(psc, "GLX_SGI_make_current_read");
   }

   return GL_TRUE;
}


static void
tdfxDestroyScreen( __DRIscreenPrivate *sPriv )
{
   tdfxScreenPrivate *fxScreen = (tdfxScreenPrivate *) sPriv->private;

   if (!fxScreen)
      return;

   drmUnmap( fxScreen->regs.map, fxScreen->regs.size );

   /* free all option information */
   driDestroyOptionInfo (&fxScreen->optionCache);

   FREE( fxScreen );
   sPriv->private = NULL;
}


static GLboolean
tdfxInitDriver( __DRIscreenPrivate *sPriv )
{
   if ( TDFX_DEBUG & DEBUG_VERBOSE_DRI ) {
      fprintf( stderr, "%s( %p )\n", __FUNCTION__, (void *)sPriv );
   }

   if ( !tdfxCreateScreen( sPriv ) ) {
      tdfxDestroyScreen( sPriv );
      return GL_FALSE;
   }

   return GL_TRUE;
}


static GLboolean
tdfxCreateBuffer( __DRIscreenPrivate *driScrnPriv,
                  __DRIdrawablePrivate *driDrawPriv,
                  const __GLcontextModes *mesaVis,
                  GLboolean isPixmap )
{
   tdfxScreenPrivate *screen = (tdfxScreenPrivate *) driScrnPriv->private;

   if (isPixmap) {
      return GL_FALSE; /* not implemented */
   }
   else {
      struct gl_framebuffer *fb = _mesa_create_framebuffer(mesaVis);

      {
         driRenderbuffer *frontRb
            = driNewRenderbuffer(GL_RGBA, NULL, screen->cpp,
                                 screen->fbOffset, screen->width, driDrawPriv);
         tdfxSetSpanFunctions(frontRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_FRONT_LEFT, &frontRb->Base);
      }

      if (mesaVis->doubleBufferMode) {
         driRenderbuffer *backRb
            = driNewRenderbuffer(GL_RGBA, NULL, screen->cpp,
                                 screen->backOffset, screen->width,
                                 driDrawPriv);
         tdfxSetSpanFunctions(backRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_BACK_LEFT, &backRb->Base);
	 backRb->backBuffer = GL_TRUE;
      }

      if (mesaVis->depthBits == 16) {
         driRenderbuffer *depthRb
            = driNewRenderbuffer(GL_DEPTH_COMPONENT16, NULL, screen->cpp,
                                 screen->depthOffset, screen->width,
                                 driDrawPriv);
         tdfxSetSpanFunctions(depthRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
      }
      else if (mesaVis->depthBits == 24) {
         driRenderbuffer *depthRb
            = driNewRenderbuffer(GL_DEPTH_COMPONENT24, NULL, screen->cpp,
                                 screen->depthOffset, screen->width,
                                 driDrawPriv);
         tdfxSetSpanFunctions(depthRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
      }

      if (mesaVis->stencilBits > 0) {
         driRenderbuffer *stencilRb
            = driNewRenderbuffer(GL_STENCIL_INDEX8_EXT, NULL, screen->cpp,
                                 screen->depthOffset, screen->width,
                                 driDrawPriv);
         tdfxSetSpanFunctions(stencilRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_STENCIL, &stencilRb->Base);
      }

      _mesa_add_soft_renderbuffers(fb,
                                   GL_FALSE, /* color */
                                   GL_FALSE, /* depth */
                                   GL_FALSE, /*swStencil,*/
                                   mesaVis->accumRedBits > 0,
                                   GL_FALSE, /* alpha */
                                   GL_FALSE /* aux */);
      driDrawPriv->driverPrivate = (void *) fb;

      return (driDrawPriv->driverPrivate != NULL);
   }
}


static void
tdfxDestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_unreference_framebuffer((GLframebuffer **)(&(driDrawPriv->driverPrivate)));
}


static void
tdfxSwapBuffers( __DRIdrawablePrivate *driDrawPriv )

{
   GET_CURRENT_CONTEXT(ctx);
   tdfxContextPtr fxMesa = 0;
   GLframebuffer *mesaBuffer;

   if ( TDFX_DEBUG & DEBUG_VERBOSE_DRI ) {
      fprintf( stderr, "%s( %p )\n", __FUNCTION__, (void *)driDrawPriv );
   }

   mesaBuffer = (GLframebuffer *) driDrawPriv->driverPrivate;
   if ( !mesaBuffer->Visual.doubleBufferMode )
      return; /* can't swap a single-buffered window */

   /* If the current context's drawable matches the given drawable
    * we have to do a glFinish (per the GLX spec).
    */
   if ( ctx ) {
      __DRIdrawablePrivate *curDrawPriv;
      fxMesa = TDFX_CONTEXT(ctx);
      curDrawPriv = fxMesa->driContext->driDrawablePriv;

      if ( curDrawPriv == driDrawPriv ) {
	 /* swapping window bound to current context, flush first */
	 _mesa_notifySwapBuffers( ctx );
	 LOCK_HARDWARE( fxMesa );
      }
      else {
         /* find the fxMesa context previously bound to the window */
	 fxMesa = (tdfxContextPtr) driDrawPriv->driContextPriv->driverPrivate;
         if (!fxMesa)
            return;
	 LOCK_HARDWARE( fxMesa );
	 fxMesa->Glide.grSstSelect( fxMesa->Glide.Board );
#ifdef DEBUG
         printf("SwapBuf SetState 1\n");
#endif
	 fxMesa->Glide.grGlideSetState(fxMesa->Glide.State );
      }
   }

#ifdef STATS
   {
      int stalls;
      static int prevStalls = 0;

      stalls = fxMesa->Glide.grFifoGetStalls();

      fprintf( stderr, "%s:\n", __FUNCTION__ );
      if ( stalls != prevStalls ) {
	 fprintf( stderr, "    %d stalls occurred\n",
		  stalls - prevStalls );
	 prevStalls = stalls;
      }
      if ( fxMesa && fxMesa->texSwaps ) {
	 fprintf( stderr, "    %d texture swaps occurred\n",
		  fxMesa->texSwaps );
	 fxMesa->texSwaps = 0;
      }
   }
#endif

   if (fxMesa->scissoredClipRects) {
      /* restore clip rects without scissor box */
      fxMesa->Glide.grDRIPosition( driDrawPriv->x, driDrawPriv->y,
                                   driDrawPriv->w, driDrawPriv->h,
                                   driDrawPriv->numClipRects,
                                   driDrawPriv->pClipRects );
   }

   fxMesa->Glide.grDRIBufferSwap( fxMesa->Glide.SwapInterval );

   if (fxMesa->scissoredClipRects) {
      /* restore clip rects WITH scissor box */
      fxMesa->Glide.grDRIPosition( driDrawPriv->x, driDrawPriv->y,
                                   driDrawPriv->w, driDrawPriv->h,
                                   fxMesa->numClipRects, fxMesa->pClipRects );
   }


#if 0
   {
      FxI32 result;
      do {
         FxI32 result;
         fxMesa->Glide.grGet(GR_PENDING_BUFFERSWAPS, 4, &result);
      } while ( result > fxMesa->maxPendingSwapBuffers );
   }
#endif

   fxMesa->stats.swapBuffer++;

   if (ctx) {
      if (ctx->DriverCtx != fxMesa) {
         fxMesa = TDFX_CONTEXT(ctx);
	 fxMesa->Glide.grSstSelect( fxMesa->Glide.Board );
#ifdef DEBUG
         printf("SwapBuf SetState 2\n");
#endif
	 fxMesa->Glide.grGlideSetState(fxMesa->Glide.State );
      }
      UNLOCK_HARDWARE( fxMesa );
   }
}


static const struct __DriverAPIRec tdfxAPI = {
   .InitDriver      = tdfxInitDriver,
   .DestroyScreen   = tdfxDestroyScreen,
   .CreateContext   = tdfxCreateContext,
   .DestroyContext  = tdfxDestroyContext,
   .CreateBuffer    = tdfxCreateBuffer,
   .DestroyBuffer   = tdfxDestroyBuffer,
   .SwapBuffers     = tdfxSwapBuffers,
   .MakeCurrent     = tdfxMakeCurrent,
   .UnbindContext   = tdfxUnbindContext,
   .GetSwapInfo     = NULL,
   .GetMSC          = NULL,
   .WaitForMSC      = NULL,
   .WaitForSBC      = NULL,
   .SwapBuffersMSC  = NULL
};


static __GLcontextModes *tdfxFillInModes(unsigned pixel_bits,
					 unsigned depth_bits,
					 unsigned stencil_bits,
					 GLboolean have_back_buffer)
{
	__GLcontextModes *modes;
	__GLcontextModes *m;
	unsigned num_modes;
	unsigned vis[2] = { GLX_TRUE_COLOR, GLX_DIRECT_COLOR };
	unsigned deep = (depth_bits > 17);
	unsigned i, db, depth, accum, stencil;

	/* Right now GLX_SWAP_COPY_OML isn't supported, but it would be easy
	 * enough to add support.  Basically, if a context is created with an
	 * fbconfig where the swap method is GLX_SWAP_COPY_OML, pageflipping
	 * will never be used.
	 */

	num_modes = (depth_bits == 16) ? 32 : 16;

	modes = (*dri_interface->createContextModes)(num_modes, sizeof(__GLcontextModes));
	m = modes;

	for (i = 0; i <= 1; i++) {
	    for (db = 0; db <= 1; db++) {
		for (depth = 0; depth <= 1; depth++) {
		    for (accum = 0; accum <= 1; accum++) {
			for (stencil = 0; stencil <= !deep; stencil++) {
			    if (deep) stencil = depth;
			    m->redBits		= deep ? 8 : 5;
			    m->greenBits	= deep ? 8 : 6;
			    m->blueBits		= deep ? 8 : 5;
			    m->alphaBits	= deep ? 8 : 0;
			    m->redMask		= deep ?0xFF000000 :0x0000F800;
			    m->greenMask	= deep ?0x00FF0000 :0x000007E0;
			    m->blueMask		= deep ?0x0000FF00 :0x0000001F;
			    m->alphaMask	= deep ? 0x000000FF : 0;
			    m->rgbBits		= m->redBits + m->greenBits +
			    			  m->blueBits + m->alphaBits;
			    m->accumRedBits	= accum ? 16 : 0;
			    m->accumGreenBits	= accum ? 16 : 0;
			    m->accumBlueBits	= accum ? 16 : 0;
			    m->accumAlphaBits	= (accum && deep) ? 16 : 0;
			    m->stencilBits	= stencil ? 8 : 0;
			    m->depthBits	= deep
			    			  ? (depth ? 24 : 0)
			    			  : (depth ? 0 : depth_bits);
			    m->visualType	= vis[i];
			    m->renderType	= GLX_RGBA_BIT;
			    m->drawableType	= GLX_WINDOW_BIT;
			    m->rgbMode		= GL_TRUE;
			    m->doubleBufferMode = db ? GL_TRUE : GL_FALSE;
			    if (db)
			    	m->swapMethod = GLX_SWAP_UNDEFINED_OML;
			    m->visualRating	= ((stencil && !deep) || accum)
			    			  ? GLX_SLOW_CONFIG
						  : GLX_NONE;
			    m = m->next;
			    if (deep) stencil = 0;
			}
		    }
		}
	    }
	}

	return modes;
}

/**
 * This is the bootstrap function for the driver.  libGL supplies all of the
 * requisite information about the system, and the driver initializes itself.
 * This routine also fills in the linked list pointed to by \c driver_modes
 * with the \c __GLcontextModes that the driver can support for windows or
 * pbuffers.
 *
 * \return A pointer to a \c __DRIscreenPrivate on success, or \c NULL on
 *         failure.
 */
PUBLIC
void * __driCreateNewScreen_20050727( __DRInativeDisplay *dpy, int scrn, __DRIscreen *psc,
			     const __GLcontextModes * modes,
			     const __DRIversion * ddx_version,
			     const __DRIversion * dri_version,
			     const __DRIversion * drm_version,
			     const __DRIframebuffer * frame_buffer,
			     drmAddress pSAREA, int fd,
			     int internal_api_version,
			     const __DRIinterfaceMethods * interface,
			     __GLcontextModes ** driver_modes )
{
   __DRIscreenPrivate *psp;
   static const __DRIversion ddx_expected = { 1, 1, 0 };
   static const __DRIversion dri_expected = { 4, 0, 0 };
   static const __DRIversion drm_expected = { 1, 0, 0 };

   dri_interface = interface;

   if ( ! driCheckDriDdxDrmVersions2( "tdfx",
				      dri_version, & dri_expected,
				      ddx_version, & ddx_expected,
				      drm_version, & drm_expected ) ) {
      return NULL;
   }

   psp = __driUtilCreateNewScreen(dpy, scrn, psc, NULL,
   				  ddx_version, dri_version, drm_version,
				  frame_buffer, pSAREA, fd,
				  internal_api_version, &tdfxAPI);

   if (psp != NULL) {
      /* divined from tdfx_dri.c, sketchy */
      TDFXDRIPtr dri_priv = (TDFXDRIPtr) psp->pDevPriv;
      int bpp = (dri_priv->cpp > 2) ? 24 : 16;

      /* XXX i wish it was like this */
      /* bpp = dri_priv->bpp */
      
      *driver_modes = tdfxFillInModes(bpp, (bpp == 16) ? 16 : 24,
				(bpp == 16) ? 0 : 8,
				(dri_priv->backOffset!=dri_priv->depthOffset));

      /* Calling driInitExtensions here, with a NULL context pointer, does not actually
       * enable the extensions.  It just makes sure that all the dispatch offsets for all
       * the extensions that *might* be enables are known.  This is needed because the
       * dispatch offsets need to be known when _mesa_context_create is called, but we can't
       * enable the extensions until we have a context pointer.
       *
       * Hello chicken.  Hello egg.  How are you two today?
       */
      driInitExtensions( NULL, card_extensions, GL_FALSE );
      driInitExtensions( NULL, napalm_extensions, GL_FALSE );
   }

   return (void *)psp;
}
