/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_screen.c,v 1.9 2003/03/26 20:43:49 tsi Exp $ */
/**************************************************************************

Copyright 1999, 2000 ATI Technologies Inc. and Precision Insight, Inc.,
                                               Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Gareth Hughes <gareth@valinux.com>
 *   Kevin E. Martin <martin@valinux.com>
 *
 */

#include "r128_dri.h"

#include "r128_context.h"
#include "r128_ioctl.h"
#include "r128_span.h"
#include "r128_tris.h"

#include "context.h"
#include "imports.h"
#include "framebuffer.h"
#include "renderbuffer.h"

#include "utils.h"
#include "vblank.h"

#include "GL/internal/dri_interface.h"

/* R128 configuration
 */
#include "xmlpool.h"

PUBLIC const char __driConfigOptions[] =
DRI_CONF_BEGIN
    DRI_CONF_SECTION_PERFORMANCE
        DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_DEF_INTERVAL_0)
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_QUALITY
        DRI_CONF_TEXTURE_DEPTH(DRI_CONF_TEXTURE_DEPTH_FB)
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_DEBUG
        DRI_CONF_NO_RAST(false)
#if ENABLE_PERF_BOXES
        DRI_CONF_PERFORMANCE_BOXES(false)
#endif
    DRI_CONF_SECTION_END
DRI_CONF_END;
#if ENABLE_PERF_BOXES
static const GLuint __driNConfigOptions = 4;
#else
static const GLuint __driNConfigOptions = 3;
#endif

extern const struct dri_extension card_extensions[];

#if 1
/* Including xf86PciInfo.h introduces a bunch of errors...
 */
#define PCI_CHIP_RAGE128LE	0x4C45
#define PCI_CHIP_RAGE128LF	0x4C46
#define PCI_CHIP_RAGE128PF	0x5046
#define PCI_CHIP_RAGE128PR	0x5052
#define PCI_CHIP_RAGE128RE	0x5245
#define PCI_CHIP_RAGE128RF	0x5246
#define PCI_CHIP_RAGE128RK	0x524B
#define PCI_CHIP_RAGE128RL	0x524C
#endif


/* Create the device specific screen private data struct.
 */
static r128ScreenPtr
r128CreateScreen( __DRIscreenPrivate *sPriv )
{
   r128ScreenPtr r128Screen;
   R128DRIPtr r128DRIPriv = (R128DRIPtr)sPriv->pDevPriv;
   PFNGLXSCRENABLEEXTENSIONPROC glx_enable_extension =
     (PFNGLXSCRENABLEEXTENSIONPROC) (*dri_interface->getProcAddress("glxEnableExtension"));
   void * const psc = sPriv->psc->screenConfigs;

   if (sPriv->devPrivSize != sizeof(R128DRIRec)) {
      fprintf(stderr,"\nERROR!  sizeof(R128DRIRec) does not match passed size from device driver\n");
      return GL_FALSE;
   }

   /* Allocate the private area */
   r128Screen = (r128ScreenPtr) CALLOC( sizeof(*r128Screen) );
   if ( !r128Screen ) return NULL;

   /* parse information in __driConfigOptions */
   driParseOptionInfo (&r128Screen->optionCache,
		       __driConfigOptions, __driNConfigOptions);

   /* This is first since which regions we map depends on whether or
    * not we are using a PCI card.
    */
   r128Screen->IsPCI = r128DRIPriv->IsPCI;
   r128Screen->sarea_priv_offset = r128DRIPriv->sarea_priv_offset;
   
   if (sPriv->drmMinor >= 3) {
      drm_r128_getparam_t gp;
      int ret;

      gp.param = R128_PARAM_IRQ_NR;
      gp.value = &r128Screen->irq;

      ret = drmCommandWriteRead( sPriv->fd, DRM_R128_GETPARAM,
				    &gp, sizeof(gp));
      if (ret) {
         fprintf(stderr, "drmR128GetParam (R128_PARAM_IRQ_NR): %d\n", ret);
         FREE( r128Screen );
         return NULL;
      }
   }

   r128Screen->mmio.handle = r128DRIPriv->registerHandle;
   r128Screen->mmio.size   = r128DRIPriv->registerSize;
   if ( drmMap( sPriv->fd,
		r128Screen->mmio.handle,
		r128Screen->mmio.size,
		(drmAddressPtr)&r128Screen->mmio.map ) ) {
      FREE( r128Screen );
      return NULL;
   }

   r128Screen->buffers = drmMapBufs( sPriv->fd );
   if ( !r128Screen->buffers ) {
      drmUnmap( (drmAddress)r128Screen->mmio.map, r128Screen->mmio.size );
      FREE( r128Screen );
      return NULL;
   }

   if ( !r128Screen->IsPCI ) {
      r128Screen->agpTextures.handle = r128DRIPriv->agpTexHandle;
      r128Screen->agpTextures.size   = r128DRIPriv->agpTexMapSize;
      if ( drmMap( sPriv->fd,
		   r128Screen->agpTextures.handle,
		   r128Screen->agpTextures.size,
		   (drmAddressPtr)&r128Screen->agpTextures.map ) ) {
	 drmUnmapBufs( r128Screen->buffers );
	 drmUnmap( (drmAddress)r128Screen->mmio.map, r128Screen->mmio.size );
	 FREE( r128Screen );
	 return NULL;
      }
   }

   switch ( r128DRIPriv->deviceID ) {
   case PCI_CHIP_RAGE128RE:
   case PCI_CHIP_RAGE128RF:
   case PCI_CHIP_RAGE128RK:
   case PCI_CHIP_RAGE128RL:
      r128Screen->chipset = R128_CARD_TYPE_R128;
      break;
   case PCI_CHIP_RAGE128PF:
      r128Screen->chipset = R128_CARD_TYPE_R128_PRO;
      break;
   case PCI_CHIP_RAGE128LE:
   case PCI_CHIP_RAGE128LF:
      r128Screen->chipset = R128_CARD_TYPE_R128_MOBILITY;
      break;
   default:
      r128Screen->chipset = R128_CARD_TYPE_R128;
      break;
   }

   r128Screen->cpp = r128DRIPriv->bpp / 8;
   r128Screen->AGPMode = r128DRIPriv->AGPMode;

   r128Screen->frontOffset	= r128DRIPriv->frontOffset;
   r128Screen->frontPitch	= r128DRIPriv->frontPitch;
   r128Screen->backOffset	= r128DRIPriv->backOffset;
   r128Screen->backPitch	= r128DRIPriv->backPitch;
   r128Screen->depthOffset	= r128DRIPriv->depthOffset;
   r128Screen->depthPitch	= r128DRIPriv->depthPitch;
   r128Screen->spanOffset	= r128DRIPriv->spanOffset;

   if ( r128DRIPriv->textureSize == 0 ) {
      r128Screen->texOffset[R128_LOCAL_TEX_HEAP] =
	 r128DRIPriv->agpTexOffset + R128_AGP_TEX_OFFSET;
      r128Screen->texSize[R128_LOCAL_TEX_HEAP] = r128DRIPriv->agpTexMapSize;
      r128Screen->logTexGranularity[R128_LOCAL_TEX_HEAP] =
	 r128DRIPriv->log2AGPTexGran;
   } else {
      r128Screen->texOffset[R128_LOCAL_TEX_HEAP] = r128DRIPriv->textureOffset;
      r128Screen->texSize[R128_LOCAL_TEX_HEAP] = r128DRIPriv->textureSize;
      r128Screen->logTexGranularity[R128_LOCAL_TEX_HEAP] = r128DRIPriv->log2TexGran;
   }

   if ( !r128Screen->agpTextures.map || r128DRIPriv->textureSize == 0 ) {
      r128Screen->numTexHeaps = R128_NR_TEX_HEAPS - 1;
      r128Screen->texOffset[R128_AGP_TEX_HEAP] = 0;
      r128Screen->texSize[R128_AGP_TEX_HEAP] = 0;
      r128Screen->logTexGranularity[R128_AGP_TEX_HEAP] = 0;
   } else {
      r128Screen->numTexHeaps = R128_NR_TEX_HEAPS;
      r128Screen->texOffset[R128_AGP_TEX_HEAP] =
	 r128DRIPriv->agpTexOffset + R128_AGP_TEX_OFFSET;
      r128Screen->texSize[R128_AGP_TEX_HEAP] = r128DRIPriv->agpTexMapSize;
      r128Screen->logTexGranularity[R128_AGP_TEX_HEAP] =
	 r128DRIPriv->log2AGPTexGran;
   }

   r128Screen->driScreen = sPriv;

   if ( glx_enable_extension != NULL ) {
      if ( r128Screen->irq != 0 ) {
	 (*glx_enable_extension)( psc, "GLX_SGI_swap_control" );
	 (*glx_enable_extension)( psc, "GLX_SGI_video_sync" );
	 (*glx_enable_extension)( psc, "GLX_MESA_swap_control" );
      }

      (*glx_enable_extension)( psc, "GLX_MESA_swap_frame_usage" );
   }

   return r128Screen;
}

/* Destroy the device specific screen private data struct.
 */
static void
r128DestroyScreen( __DRIscreenPrivate *sPriv )
{
   r128ScreenPtr r128Screen = (r128ScreenPtr)sPriv->private;

   if ( !r128Screen )
      return;

   if ( !r128Screen->IsPCI ) {
      drmUnmap( (drmAddress)r128Screen->agpTextures.map,
		r128Screen->agpTextures.size );
   }
   drmUnmapBufs( r128Screen->buffers );
   drmUnmap( (drmAddress)r128Screen->mmio.map, r128Screen->mmio.size );

   /* free all option information */
   driDestroyOptionInfo (&r128Screen->optionCache);

   FREE( r128Screen );
   sPriv->private = NULL;
}


/* Create and initialize the Mesa and driver specific pixmap buffer
 * data.
 */
static GLboolean
r128CreateBuffer( __DRIscreenPrivate *driScrnPriv,
                  __DRIdrawablePrivate *driDrawPriv,
                  const __GLcontextModes *mesaVis,
                  GLboolean isPixmap )
{
   r128ScreenPtr screen = (r128ScreenPtr) driScrnPriv->private;

   if (isPixmap) {
      return GL_FALSE; /* not implemented */
   }
   else {
      const GLboolean swDepth = GL_FALSE;
      const GLboolean swAlpha = GL_FALSE;
      const GLboolean swAccum = mesaVis->accumRedBits > 0;
      const GLboolean swStencil = mesaVis->stencilBits > 0 &&
         mesaVis->depthBits != 24;
      struct gl_framebuffer *fb = _mesa_create_framebuffer(mesaVis);

      {
         driRenderbuffer *frontRb
            = driNewRenderbuffer(GL_RGBA,
                                 NULL,
                                 screen->cpp,
                                 screen->frontOffset, screen->frontPitch,
                                 driDrawPriv);
         r128SetSpanFunctions(frontRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_FRONT_LEFT, &frontRb->Base);
      }

      if (mesaVis->doubleBufferMode) {
         driRenderbuffer *backRb
            = driNewRenderbuffer(GL_RGBA,
                                 NULL,
                                 screen->cpp,
                                 screen->backOffset, screen->backPitch,
                                 driDrawPriv);
         r128SetSpanFunctions(backRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_BACK_LEFT, &backRb->Base);
      }

      if (mesaVis->depthBits == 16) {
         driRenderbuffer *depthRb
            = driNewRenderbuffer(GL_DEPTH_COMPONENT16,
                                 NULL,
                                 screen->cpp,
                                 screen->depthOffset, screen->depthPitch,
                                 driDrawPriv);
         r128SetSpanFunctions(depthRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
      }
      else if (mesaVis->depthBits == 24) {
         driRenderbuffer *depthRb
            = driNewRenderbuffer(GL_DEPTH_COMPONENT24,
                                 NULL,
                                 screen->cpp,
                                 screen->depthOffset, screen->depthPitch,
                                 driDrawPriv);
         r128SetSpanFunctions(depthRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
      }

      if (mesaVis->stencilBits > 0 && !swStencil) {
         driRenderbuffer *stencilRb
            = driNewRenderbuffer(GL_STENCIL_INDEX8_EXT,
                                 NULL,
                                 screen->cpp,
                                 screen->depthOffset, screen->depthPitch,
                                 driDrawPriv);
         r128SetSpanFunctions(stencilRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_STENCIL, &stencilRb->Base);
      }

      _mesa_add_soft_renderbuffers(fb,
                                   GL_FALSE, /* color */
                                   swDepth,
                                   swStencil,
                                   swAccum,
                                   swAlpha,
                                   GL_FALSE /* aux */);
      driDrawPriv->driverPrivate = (void *) fb;

      return (driDrawPriv->driverPrivate != NULL);
   }
}


static void
r128DestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_unreference_framebuffer((GLframebuffer **)(&(driDrawPriv->driverPrivate)));
}


/* Copy the back color buffer to the front color buffer */
static void
r128SwapBuffers(__DRIdrawablePrivate *dPriv)
{
   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
      r128ContextPtr rmesa;
      GLcontext *ctx;
      rmesa = (r128ContextPtr) dPriv->driContextPriv->driverPrivate;
      ctx = rmesa->glCtx;
      if (ctx->Visual.doubleBufferMode) {
         _mesa_notifySwapBuffers( ctx );  /* flush pending rendering comands */
         if ( rmesa->doPageFlip ) {
            r128PageFlip( dPriv );
         }
         else {
            r128CopyBuffer( dPriv );
         }
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
r128InitDriver( __DRIscreenPrivate *sPriv )
{
   sPriv->private = (void *) r128CreateScreen( sPriv );

   if ( !sPriv->private ) {
      r128DestroyScreen( sPriv );
      return GL_FALSE;
   }

   return GL_TRUE;
}


static struct __DriverAPIRec r128API = {
   .InitDriver      = r128InitDriver,
   .DestroyScreen   = r128DestroyScreen,
   .CreateContext   = r128CreateContext,
   .DestroyContext  = r128DestroyContext,
   .CreateBuffer    = r128CreateBuffer,
   .DestroyBuffer   = r128DestroyBuffer,
   .SwapBuffers     = r128SwapBuffers,
   .MakeCurrent     = r128MakeCurrent,
   .UnbindContext   = r128UnbindContext,
   .GetSwapInfo     = NULL,
   .GetMSC          = driGetMSC32,
   .WaitForMSC      = driWaitForMSC32,
   .WaitForSBC      = NULL,
   .SwapBuffersMSC  = NULL

};


static __GLcontextModes *
r128FillInModes( unsigned pixel_bits, unsigned depth_bits,
		 unsigned stencil_bits, GLboolean have_back_buffer )
{
    __GLcontextModes * modes;
    __GLcontextModes * m;
    unsigned num_modes;
    unsigned depth_buffer_factor;
    unsigned back_buffer_factor;
    GLenum fb_format;
    GLenum fb_type;

    /* Right now GLX_SWAP_COPY_OML isn't supported, but it would be easy
     * enough to add support.  Basically, if a context is created with an
     * fbconfig where the swap method is GLX_SWAP_COPY_OML, pageflipping
     * will never be used.
     */
    static const GLenum back_buffer_modes[] = {
	GLX_NONE, GLX_SWAP_UNDEFINED_OML /*, GLX_SWAP_COPY_OML */
    };

    u_int8_t depth_bits_array[2];
    u_int8_t stencil_bits_array[2];


    depth_bits_array[0] = depth_bits;
    depth_bits_array[1] = depth_bits;
    
    /* Just like with the accumulation buffer, always provide some modes
     * with a stencil buffer.  It will be a sw fallback, but some apps won't
     * care about that.
     */
    stencil_bits_array[0] = 0;
    stencil_bits_array[1] = (stencil_bits == 0) ? 8 : stencil_bits;

    depth_buffer_factor = ((depth_bits != 0) || (stencil_bits != 0)) ? 2 : 1;
    back_buffer_factor  = (have_back_buffer) ? 2 : 1;

    num_modes = depth_buffer_factor * back_buffer_factor * 4;

    if ( pixel_bits == 16 ) {
        fb_format = GL_RGB;
        fb_type = GL_UNSIGNED_SHORT_5_6_5;
    }
    else {
        fb_format = GL_BGR;
        fb_type = GL_UNSIGNED_INT_8_8_8_8_REV;
    }

    modes = (*dri_interface->createContextModes)( num_modes, sizeof( __GLcontextModes ) );
    m = modes;
    if ( ! driFillInModes( & m, fb_format, fb_type,
			   depth_bits_array, stencil_bits_array, depth_buffer_factor,
			   back_buffer_modes, back_buffer_factor,
			   GLX_TRUE_COLOR ) ) {
	fprintf( stderr, "[%s:%u] Error creating FBConfig!\n",
		 __func__, __LINE__ );
	return NULL;
    }

    if ( ! driFillInModes( & m, fb_format, fb_type,
			   depth_bits_array, stencil_bits_array, depth_buffer_factor,
			   back_buffer_modes, back_buffer_factor,
			   GLX_DIRECT_COLOR ) ) {
	fprintf( stderr, "[%s:%u] Error creating FBConfig!\n",
		 __func__, __LINE__ );
	return NULL;
    }

    /* Mark the visual as slow if there are "fake" stencil bits.
     */
    for ( m = modes ; m != NULL ; m = m->next ) {
	if ( (m->stencilBits != 0) && (m->stencilBits != stencil_bits) ) {
	    m->visualRating = GLX_SLOW_CONFIG;
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
   static const __DRIversion ddx_expected = { 4, 0, 0 };
   static const __DRIversion dri_expected = { 4, 0, 0 };
   static const __DRIversion drm_expected = { 2, 2, 0 };


   dri_interface = interface;

   if ( ! driCheckDriDdxDrmVersions2( "Rage128",
				      dri_version, & dri_expected,
				      ddx_version, & ddx_expected,
				      drm_version, & drm_expected ) ) {
      return NULL;
   }
      
   psp = __driUtilCreateNewScreen(dpy, scrn, psc, NULL,
				  ddx_version, dri_version, drm_version,
				  frame_buffer, pSAREA, fd,
				  internal_api_version, &r128API);
   if ( psp != NULL ) {
      R128DRIPtr dri_priv = (R128DRIPtr) psp->pDevPriv;
      *driver_modes = r128FillInModes( dri_priv->bpp,
				       (dri_priv->bpp == 16) ? 16 : 24,
				       (dri_priv->bpp == 16) ? 0  : 8,
				       (dri_priv->backOffset != dri_priv->depthOffset) );

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
