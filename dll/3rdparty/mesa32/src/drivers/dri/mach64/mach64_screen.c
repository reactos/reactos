/* $XFree86$ */ /* -*- mode: c; c-basic-offset: 3 -*- */
/*
 * Copyright 2000 Gareth Hughes
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
 * GARETH HUGHES BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *	Leif Delgass <ldelgass@retinalburn.net>
 *	Jos�Fonseca <j_r_fonseca@yahoo.co.uk>
 */

#include "mach64_context.h"
#include "mach64_ioctl.h"
#include "mach64_tris.h"
#include "mach64_vb.h"
#include "mach64_span.h"

#include "context.h"
#include "imports.h"
#include "framebuffer.h"
#include "renderbuffer.h"

#include "utils.h"
#include "vblank.h"

#include "GL/internal/dri_interface.h"

/* Mach64 configuration
 */
#include "xmlpool.h"

PUBLIC const char __driConfigOptions[] =
DRI_CONF_BEGIN
    DRI_CONF_SECTION_PERFORMANCE
        DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_DEF_INTERVAL_0)
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_DEBUG
        DRI_CONF_NO_RAST(false)
#if ENABLE_PERF_BOXES
        DRI_CONF_PERFORMANCE_BOXES(false)
#endif
    DRI_CONF_SECTION_END
DRI_CONF_END;
#if ENABLE_PERF_BOXES
static const GLuint __driNConfigOptions = 3;
#else
static const GLuint __driNConfigOptions = 2;
#endif

extern const struct dri_extension card_extensions[];

static __GLcontextModes * fill_in_modes( __GLcontextModes * modes,
					 unsigned pixel_bits, 
					 unsigned depth_bits,
					 unsigned stencil_bits,
					 const GLenum * db_modes,
					 unsigned num_db_modes,
					 int visType )
{
       static const u_int8_t bits[2][4] = {
	{          5,          6,          5,          0 },
	{          8,          8,          8,          0 }
    };

    static const u_int32_t masks[2][4] = {
	{ 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000 },
	{ 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000 }
    };

    unsigned   i;
    unsigned   j;
    const unsigned index = ((pixel_bits + 15) / 16) - 1;

    for ( i = 0 ; i < num_db_modes ; i++ ) {
	for ( j = 0 ; j < 2 ; j++ ) {

	    modes->redBits   = bits[index][0];
	    modes->greenBits = bits[index][1];
	    modes->blueBits  = bits[index][2];
	    modes->alphaBits = bits[index][3];
	    modes->redMask   = masks[index][0];
	    modes->greenMask = masks[index][1];
	    modes->blueMask  = masks[index][2];
	    modes->alphaMask = masks[index][3];
	    modes->rgbBits   = modes->redBits + modes->greenBits
		+ modes->blueBits + modes->alphaBits;

	    modes->accumRedBits   = 16 * j;
	    modes->accumGreenBits = 16 * j;
	    modes->accumBlueBits  = 16 * j;
	    modes->accumAlphaBits = 0;
	    modes->visualRating = (j == 0) ? GLX_NONE : GLX_SLOW_CONFIG;
	    modes->drawableType = GLX_WINDOW_BIT | GLX_PIXMAP_BIT;
	    modes->stencilBits = stencil_bits;
	    modes->depthBits = depth_bits;

	    modes->visualType = visType;
	    modes->renderType = GLX_RGBA_BIT;
	    modes->rgbMode = GL_TRUE;

	    if ( db_modes[i] == GLX_NONE ) {

		modes->doubleBufferMode = GL_FALSE;
	    }
	    else {
		modes->doubleBufferMode = GL_TRUE;
		modes->swapMethod = db_modes[i];
	    }

	    modes = modes->next;
	}
    }
    
    return modes;
}


static __GLcontextModes *
mach64FillInModes( unsigned pixel_bits, unsigned depth_bits,
		 unsigned stencil_bits, GLboolean have_back_buffer )
{
   __GLcontextModes * modes;
    __GLcontextModes * m;
    unsigned num_modes;
    unsigned depth_buffer_factor;
    unsigned back_buffer_factor;
    unsigned i;

    /* Right now GLX_SWAP_COPY_OML isn't supported, but it would be easy
     * enough to add support.  Basically, if a context is created with an
     * fbconfig where the swap method is GLX_SWAP_COPY_OML, pageflipping
     * will never be used.
     */
    static const GLenum back_buffer_modes[] = {
	GLX_NONE, GLX_SWAP_UNDEFINED_OML /*, GLX_SWAP_COPY_OML */
    };

    int depth_buffer_modes[2][2];


    depth_buffer_modes[0][0] = depth_bits;
    depth_buffer_modes[1][0] = depth_bits;
    
    /* Just like with the accumulation buffer, always provide some modes
     * with a stencil buffer.  It will be a sw fallback, but some apps won't
     * care about that.
     */
    depth_buffer_modes[0][1] = 0;
    depth_buffer_modes[1][1] = (stencil_bits == 0) ? 8 : stencil_bits;

    depth_buffer_factor = ((depth_bits != 0) || (stencil_bits != 0)) ? 2 : 1;
    back_buffer_factor  = (have_back_buffer) ? 2 : 1;

    num_modes = depth_buffer_factor * back_buffer_factor * 4;

    modes = (*dri_interface->createContextModes)( num_modes, sizeof( __GLcontextModes ) );
    m = modes;
    for ( i = 0 ; i < depth_buffer_factor ; i++ ) {
	m = fill_in_modes( m, pixel_bits, 
			   depth_buffer_modes[i][0], depth_buffer_modes[i][1],
			   back_buffer_modes, back_buffer_factor,
			   GLX_TRUE_COLOR );
    }

    for ( i = 0 ; i < depth_buffer_factor ; i++ ) {
	m = fill_in_modes( m, pixel_bits, 
			   depth_buffer_modes[i][0], depth_buffer_modes[i][1],
			   back_buffer_modes, back_buffer_factor,
			   GLX_DIRECT_COLOR );
    }

    /* Mark the visual as slow if there are "fake" stencil bits.
     */
    for ( m = modes ; m != NULL ; m = m->next ) {
       if ( (m->stencilBits != 0) && (m->stencilBits != stencil_bits) ){
	    m->visualRating = GLX_SLOW_CONFIG;
	}
    }

    return modes;
}


/* Create the device specific screen private data struct.
 */
static mach64ScreenRec *
mach64CreateScreen( __DRIscreenPrivate *sPriv )
{
   mach64ScreenPtr mach64Screen;
   ATIDRIPtr serverInfo = (ATIDRIPtr)sPriv->pDevPriv;
   PFNGLXSCRENABLEEXTENSIONPROC glx_enable_extension =
     (PFNGLXSCRENABLEEXTENSIONPROC) (*dri_interface->getProcAddress("glxEnableExtension"));
   void * const psc = sPriv->psc->screenConfigs;

   if (sPriv->devPrivSize != sizeof(ATIDRIRec)) {
      fprintf(stderr,"\nERROR!  sizeof(ATIDRIRec) does not match passed size from device driver\n");
      return GL_FALSE;
   }

   if ( MACH64_DEBUG & DEBUG_VERBOSE_DRI ) 
      fprintf( stderr, "%s\n", __FUNCTION__ );

   /* Allocate the private area */
   mach64Screen = (mach64ScreenPtr) CALLOC( sizeof(*mach64Screen) );
   if ( !mach64Screen ) return NULL;

   /* parse information in __driConfigOptions */
   driParseOptionInfo (&mach64Screen->optionCache,
		       __driConfigOptions, __driNConfigOptions);

   mach64Screen->IsPCI = serverInfo->IsPCI;

   {
      drm_mach64_getparam_t gp;
      int ret;

      gp.param = MACH64_PARAM_IRQ_NR;
      gp.value = (void *) &mach64Screen->irq;

      ret = drmCommandWriteRead( sPriv->fd, DRM_MACH64_GETPARAM,
				    &gp, sizeof(gp));
      if (ret) {
         fprintf(stderr, "DRM_MACH64_GETPARAM (MACH64_PARAM_IRQ_NR): %d\n", ret);
         FREE( mach64Screen );
         return NULL;
      }
   }

   mach64Screen->mmio.handle = serverInfo->regs;
   mach64Screen->mmio.size   = serverInfo->regsSize;
   if ( drmMap( sPriv->fd,
		mach64Screen->mmio.handle,
		mach64Screen->mmio.size,
		(drmAddressPtr)&mach64Screen->mmio.map ) != 0 ) {
      FREE( mach64Screen );
      return NULL;
   }

   mach64Screen->buffers = drmMapBufs( sPriv->fd );
   if ( !mach64Screen->buffers ) {
      drmUnmap( (drmAddress)mach64Screen->mmio.map,
		mach64Screen->mmio.size );
      FREE( mach64Screen );
      return NULL;
   }

   if ( !mach64Screen->IsPCI ) {
      mach64Screen->agpTextures.handle = serverInfo->agp;
      mach64Screen->agpTextures.size   = serverInfo->agpSize;
      if ( drmMap( sPriv->fd,
		   mach64Screen->agpTextures.handle,
		   mach64Screen->agpTextures.size,
		   (drmAddressPtr)&mach64Screen->agpTextures.map ) ) {
	 drmUnmapBufs( mach64Screen->buffers );
	 drmUnmap( (drmAddress)mach64Screen->mmio.map, mach64Screen->mmio.size );
	 FREE( mach64Screen );
	 return NULL;
      }
   }

   mach64Screen->AGPMode	= serverInfo->AGPMode;

   mach64Screen->chipset	= serverInfo->chipset;
   mach64Screen->width		= serverInfo->width;
   mach64Screen->height		= serverInfo->height;
   mach64Screen->mem		= serverInfo->mem;
   mach64Screen->cpp		= serverInfo->cpp;

   mach64Screen->frontOffset	= serverInfo->frontOffset;
   mach64Screen->frontPitch	= serverInfo->frontPitch;
   mach64Screen->backOffset	= serverInfo->backOffset;
   mach64Screen->backPitch	= serverInfo->backPitch;
   mach64Screen->depthOffset	= serverInfo->depthOffset;
   mach64Screen->depthPitch	= serverInfo->depthPitch;

   mach64Screen->texOffset[MACH64_CARD_HEAP] = serverInfo->textureOffset;
   mach64Screen->texSize[MACH64_CARD_HEAP] = serverInfo->textureSize;
   mach64Screen->logTexGranularity[MACH64_CARD_HEAP] =
      serverInfo->logTextureGranularity;

   if ( mach64Screen->IsPCI ) {
      mach64Screen->numTexHeaps = MACH64_NR_TEX_HEAPS - 1;
      mach64Screen->firstTexHeap = MACH64_CARD_HEAP;
      mach64Screen->texOffset[MACH64_AGP_HEAP] = 0;
      mach64Screen->texSize[MACH64_AGP_HEAP] = 0;
      mach64Screen->logTexGranularity[MACH64_AGP_HEAP] = 0;
   } else {
      if (serverInfo->textureSize > 0) {
	 mach64Screen->numTexHeaps = MACH64_NR_TEX_HEAPS;
	 mach64Screen->firstTexHeap = MACH64_CARD_HEAP;
      } else {
	 mach64Screen->numTexHeaps = MACH64_NR_TEX_HEAPS - 1;
	 mach64Screen->firstTexHeap = MACH64_AGP_HEAP;
      }
      mach64Screen->texOffset[MACH64_AGP_HEAP] = serverInfo->agpTextureOffset;
      mach64Screen->texSize[MACH64_AGP_HEAP] = serverInfo->agpSize;
      mach64Screen->logTexGranularity[MACH64_AGP_HEAP] = serverInfo->logAgpTextureGranularity;
   }

   mach64Screen->driScreen = sPriv;

   if ( glx_enable_extension != NULL ) {
      if ( mach64Screen->irq != 0 ) {
	 (*glx_enable_extension)( psc, "GLX_SGI_swap_control" );
	 (*glx_enable_extension)( psc, "GLX_SGI_video_sync" );
	 (*glx_enable_extension)( psc, "GLX_MESA_swap_control" );
      }

      (*glx_enable_extension)( psc, "GLX_MESA_swap_frame_usage" );
   }

   return mach64Screen;
}

/* Destroy the device specific screen private data struct.
 */
static void
mach64DestroyScreen( __DRIscreenPrivate *driScreen )
{
   mach64ScreenRec *mach64Screen = (mach64ScreenRec *) driScreen->private;

   if ( !mach64Screen )
      return;

   if ( MACH64_DEBUG & DEBUG_VERBOSE_DRI ) 
      fprintf( stderr, "%s\n", __FUNCTION__ );

   if ( !mach64Screen->IsPCI ) {
      drmUnmap( (drmAddress)mach64Screen->agpTextures.map,
		mach64Screen->agpTextures.size );
   }

   drmUnmapBufs( mach64Screen->buffers );
   drmUnmap( (drmAddress)mach64Screen->mmio.map, mach64Screen->mmio.size );

   FREE( mach64Screen );
   driScreen->private = NULL;
}


/* Create and initialize the Mesa and driver specific pixmap buffer
 * data.
 */
static GLboolean
mach64CreateBuffer( __DRIscreenPrivate *driScrnPriv,
		    __DRIdrawablePrivate *driDrawPriv,
		    const __GLcontextModes *mesaVis,
		    GLboolean isPixmap )
{
   mach64ScreenPtr screen = (mach64ScreenPtr) driScrnPriv->private;

   if (isPixmap) {
      return GL_FALSE; /* not implemented */
   }
   else {
      struct gl_framebuffer *fb = _mesa_create_framebuffer(mesaVis);

      {
         driRenderbuffer *frontRb
            = driNewRenderbuffer(GL_RGBA,
                                 NULL,
                                 screen->cpp,
                                 screen->frontOffset, screen->frontPitch,
                                 driDrawPriv);
         mach64SetSpanFunctions(frontRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_FRONT_LEFT, &frontRb->Base);
      }

      if (mesaVis->doubleBufferMode) {
         driRenderbuffer *backRb
            = driNewRenderbuffer(GL_RGBA,
                                 NULL,
                                 screen->cpp,
                                 screen->backOffset, screen->backPitch,
                                 driDrawPriv);
         mach64SetSpanFunctions(backRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_BACK_LEFT, &backRb->Base);
      }

      if (mesaVis->depthBits == 16) {
         driRenderbuffer *depthRb
            = driNewRenderbuffer(GL_DEPTH_COMPONENT16,
                                 NULL, screen->cpp,
                                 screen->depthOffset, screen->depthPitch,
                                 driDrawPriv);
         mach64SetSpanFunctions(depthRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
      }
      else if (mesaVis->depthBits == 24) {
         /* XXX I don't think 24-bit Z is supported - so this isn't used */
         driRenderbuffer *depthRb
            = driNewRenderbuffer(GL_DEPTH_COMPONENT24,
                                 NULL,
                                 screen->cpp,
                                 screen->depthOffset, screen->depthPitch,
                                 driDrawPriv);
         mach64SetSpanFunctions(depthRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
      }

      _mesa_add_soft_renderbuffers(fb,
                                   GL_FALSE, /* color */
                                   GL_FALSE, /* depth */
                                   mesaVis->stencilBits > 0,
                                   mesaVis->accumRedBits > 0,
                                   GL_FALSE, /* alpha */
                                   GL_FALSE /* aux */);
      driDrawPriv->driverPrivate = (void *) fb;

      return (driDrawPriv->driverPrivate != NULL);
   }
}


static void
mach64DestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_unreference_framebuffer((GLframebuffer **)(&(driDrawPriv->driverPrivate)));
}


/* Copy the back color buffer to the front color buffer */
static void
mach64SwapBuffers(__DRIdrawablePrivate *dPriv)
{
   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
      mach64ContextPtr mmesa;
      GLcontext *ctx;
      mmesa = (mach64ContextPtr) dPriv->driContextPriv->driverPrivate;
      ctx = mmesa->glCtx;
      if (ctx->Visual.doubleBufferMode) {
         _mesa_notifySwapBuffers( ctx );  /* flush pending rendering comands */
         mach64CopyBuffer( dPriv );
      }
   }
   else {
      /* XXX this shouldn't be an error but we can't handle it for now */
      _mesa_problem(NULL, "%s: drawable has no context!", __FUNCTION__);
   }
}


/* Initialize the driver specific screen private data.
 */
static GLboolean
mach64InitDriver( __DRIscreenPrivate *driScreen )
{
   driScreen->private = (void *) mach64CreateScreen( driScreen );

   if ( !driScreen->private ) {
      mach64DestroyScreen( driScreen );
      return GL_FALSE;
   }

   return GL_TRUE;
}


static struct __DriverAPIRec mach64API = {
   .InitDriver      = mach64InitDriver,
   .DestroyScreen   = mach64DestroyScreen,
   .CreateContext   = mach64CreateContext,
   .DestroyContext  = mach64DestroyContext,
   .CreateBuffer    = mach64CreateBuffer,
   .DestroyBuffer   = mach64DestroyBuffer,
   .SwapBuffers     = mach64SwapBuffers,
   .MakeCurrent     = mach64MakeCurrent,
   .UnbindContext   = mach64UnbindContext,
   .GetSwapInfo     = NULL,
   .GetMSC          = driGetMSC32,
   .WaitForMSC      = driWaitForMSC32,
   .WaitForSBC      = NULL,
   .SwapBuffersMSC  = NULL
};


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
   static const __DRIversion ddx_expected = { 6, 4, 0 };
   static const __DRIversion dri_expected = { 4, 0, 0 };
   static const __DRIversion drm_expected = { 2, 0, 0 };

   dri_interface = interface;

   if ( ! driCheckDriDdxDrmVersions2( "Mach64",
				      dri_version, & dri_expected,
				      ddx_version, & ddx_expected,
				      drm_version, & drm_expected ) ) {
      return NULL;
   }

   psp = __driUtilCreateNewScreen(dpy, scrn, psc, NULL,
				  ddx_version, dri_version, drm_version,
				  frame_buffer, pSAREA, fd,
				  internal_api_version, &mach64API);
   if ( psp != NULL ) {
      ATIDRIPtr dri_priv = (ATIDRIPtr) psp->pDevPriv;
      *driver_modes = mach64FillInModes( dri_priv->cpp * 8,
					 16,
					 0,
					 1);

      /* Calling driInitExtensions here, with a NULL context pointer, does not actually
       * enable the extensions.  It just makes sure that all the dispatch offsets for all
       * the extensions that *might* be enables are known.  This is needed because the
       * dispatch offsets need to be known when _mesa_context_create is called, but we can't
       * enable the extensions until we have a context pointer.
       *
       * Hello chicken.  Hello egg.  How are you two today?
       */
      driInitExtensions( NULL, card_extensions, GL_FALSE );
   }

   return (void *) psp;
}
