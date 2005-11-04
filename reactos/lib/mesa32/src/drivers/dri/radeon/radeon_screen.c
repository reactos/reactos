/* $XFree86: xc/lib/GL/mesa/src/drv/radeon/radeon_screen.c,v 1.7 2003/03/26 20:43:51 tsi Exp $ */
/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/**
 * \file radeon_screen.c
 * Screen initialization functions for the Radeon driver.
 *
 * \author Kevin E. Martin <martin@valinux.com>
 * \author  Gareth Hughes <gareth@valinux.com>
 */

#include "glheader.h"
#include "imports.h"
#include "mtypes.h"
#include "framebuffer.h"
#include "renderbuffer.h"

#define STANDALONE_MMIO
#include "radeon_context.h"
#include "radeon_screen.h"
#include "radeon_macros.h"
#include "radeon_span.h"

#include "utils.h"
#include "context.h"
#include "vblank.h"
#include "drirenderbuffer.h"

#include "GL/internal/dri_interface.h"

/* Radeon configuration
 */
#include "xmlpool.h"

PUBLIC const char __driConfigOptions[] =
DRI_CONF_BEGIN
    DRI_CONF_SECTION_PERFORMANCE
        DRI_CONF_TCL_MODE(DRI_CONF_TCL_CODEGEN)
        DRI_CONF_FTHROTTLE_MODE(DRI_CONF_FTHROTTLE_IRQS)
        DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_DEF_INTERVAL_0)
        DRI_CONF_HYPERZ(false)
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_QUALITY
        DRI_CONF_TEXTURE_DEPTH(DRI_CONF_TEXTURE_DEPTH_FB)
        DRI_CONF_DEF_MAX_ANISOTROPY(1.0,"1.0,2.0,4.0,8.0,16.0")
        DRI_CONF_NO_NEG_LOD_BIAS(false)
        DRI_CONF_FORCE_S3TC_ENABLE(false)
        DRI_CONF_COLOR_REDUCTION(DRI_CONF_COLOR_REDUCTION_DITHER)
        DRI_CONF_ROUND_MODE(DRI_CONF_ROUND_TRUNC)
        DRI_CONF_DITHER_MODE(DRI_CONF_DITHER_XERRORDIFF)
        DRI_CONF_TEXTURE_LEVEL_HACK(false)
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_DEBUG
        DRI_CONF_NO_RAST(false)
    DRI_CONF_SECTION_END
DRI_CONF_END;
static const GLuint __driNConfigOptions = 13;

extern const struct dri_extension card_extensions[];

#if 1
/* Including xf86PciInfo.h introduces a bunch of errors...
 */
#define PCI_CHIP_RADEON_QD	0x5144
#define PCI_CHIP_RADEON_QE	0x5145
#define PCI_CHIP_RADEON_QF	0x5146
#define PCI_CHIP_RADEON_QG	0x5147

#define PCI_CHIP_RADEON_QY	0x5159
#define PCI_CHIP_RADEON_QZ	0x515A

#define PCI_CHIP_RN50_515E	0x515E
#define PCI_CHIP_RN50_5969	0x5969

#define PCI_CHIP_RADEON_LW	0x4C57 /* mobility 7 - has tcl */
#define PCI_CHIP_RADEON_LX	0x4C58 /* mobility FireGL 7800 m7 */

#define PCI_CHIP_RADEON_LY	0x4C59
#define PCI_CHIP_RADEON_LZ	0x4C5A

#define PCI_CHIP_RV200_QW	0x5157 /* Radeon 7500 - not an R200 at all */
#define PCI_CHIP_RV200_QX	0x5158

/* IGP Chipsets */
#define PCI_CHIP_RS100_4136     0x4136
#define PCI_CHIP_RS200_4137     0x4137
#define PCI_CHIP_RS250_4237     0x4237
#define PCI_CHIP_RS100_4336     0x4336
#define PCI_CHIP_RS200_4337     0x4337
#define PCI_CHIP_RS250_4437     0x4437
#endif


static int getSwapInfo( __DRIdrawablePrivate *dPriv, __DRIswapInfo * sInfo );

static __GLcontextModes *
radeonFillInModes( unsigned pixel_bits, unsigned depth_bits,
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
        fb_format = GL_BGRA;
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


/* Create the device specific screen private data struct.
 */
radeonScreenPtr radeonCreateScreen( __DRIscreenPrivate *sPriv )
{
   radeonScreenPtr screen;
   RADEONDRIPtr dri_priv = (RADEONDRIPtr)sPriv->pDevPriv;
   unsigned char *RADEONMMIO;
   PFNGLXSCRENABLEEXTENSIONPROC glx_enable_extension =
     (PFNGLXSCRENABLEEXTENSIONPROC) (*dri_interface->getProcAddress("glxEnableExtension"));
   void * const psc = sPriv->psc->screenConfigs;

   if (sPriv->devPrivSize != sizeof(RADEONDRIRec)) {
      fprintf(stderr,"\nERROR!  sizeof(RADEONDRIRec) does not match passed size from device driver\n");
      return GL_FALSE;
   }

   /* Allocate the private area */
   screen = (radeonScreenPtr) CALLOC( sizeof(*screen) );
   if ( !screen ) {
      __driUtilMessage("%s: Could not allocate memory for screen structure",
		       __FUNCTION__);
      return NULL;
   }

   /* parse information in __driConfigOptions */
   driParseOptionInfo (&screen->optionCache,
		       __driConfigOptions, __driNConfigOptions);

   /* This is first since which regions we map depends on whether or
    * not we are using a PCI card.
    */
   screen->IsPCI = dri_priv->IsPCI;

   {
      int ret;
      drm_radeon_getparam_t gp;

      gp.param = RADEON_PARAM_GART_BUFFER_OFFSET;
      gp.value = &screen->gart_buffer_offset;

      ret = drmCommandWriteRead( sPriv->fd, DRM_RADEON_GETPARAM,
				 &gp, sizeof(gp));
      if (ret) {
	 FREE( screen );
	 fprintf(stderr, "drm_radeon_getparam_t (RADEON_PARAM_GART_BUFFER_OFFSET): %d\n", ret);
	 return NULL;
      }

      if (sPriv->drmMinor >= 6) {
	 gp.param = RADEON_PARAM_IRQ_NR;
	 gp.value = &screen->irq;

	 ret = drmCommandWriteRead( sPriv->fd, DRM_RADEON_GETPARAM,
				    &gp, sizeof(gp));
	 if (ret) {
	    FREE( screen );
	    fprintf(stderr, "drm_radeon_getparam_t (RADEON_PARAM_IRQ_NR): %d\n", ret);
	    return NULL;
	 }
      }
   }

   screen->mmio.handle = dri_priv->registerHandle;
   screen->mmio.size   = dri_priv->registerSize;
   if ( drmMap( sPriv->fd,
		screen->mmio.handle,
		screen->mmio.size,
		&screen->mmio.map ) ) {
      FREE( screen );
      __driUtilMessage("%s: drmMap failed\n", __FUNCTION__ );
      return NULL;
   }

   RADEONMMIO = screen->mmio.map;

   screen->status.handle = dri_priv->statusHandle;
   screen->status.size   = dri_priv->statusSize;
   if ( drmMap( sPriv->fd,
		screen->status.handle,
		screen->status.size,
		&screen->status.map ) ) {
      drmUnmap( screen->mmio.map, screen->mmio.size );
      FREE( screen );
      __driUtilMessage("%s: drmMap (2) failed\n", __FUNCTION__ );
      return NULL;
   }
   screen->scratch = (__volatile__ u_int32_t *)
      ((GLubyte *)screen->status.map + RADEON_SCRATCH_REG_OFFSET);

   screen->buffers = drmMapBufs( sPriv->fd );
   if ( !screen->buffers ) {
      drmUnmap( screen->status.map, screen->status.size );
      drmUnmap( screen->mmio.map, screen->mmio.size );
      FREE( screen );
      __driUtilMessage("%s: drmMapBufs failed\n", __FUNCTION__ );
      return NULL;
   }

   if ( dri_priv->gartTexHandle && dri_priv->gartTexMapSize ) {
      screen->gartTextures.handle = dri_priv->gartTexHandle;
      screen->gartTextures.size   = dri_priv->gartTexMapSize;
      if ( drmMap( sPriv->fd,
		   screen->gartTextures.handle,
		   screen->gartTextures.size,
		   (drmAddressPtr)&screen->gartTextures.map ) ) {
	 drmUnmapBufs( screen->buffers );
	 drmUnmap( screen->status.map, screen->status.size );
	 drmUnmap( screen->mmio.map, screen->mmio.size );
	 FREE( screen );
	 __driUtilMessage("%s: drmMap failed for GART texture area\n", __FUNCTION__);
	 return NULL;
      }

      screen->gart_texture_offset = dri_priv->gartTexOffset + ( screen->IsPCI
		? INREG( RADEON_AIC_LO_ADDR )
		: ( ( INREG( RADEON_MC_AGP_LOCATION ) & 0x0ffffU ) << 16 ) );
   }

   screen->chipset = 0;
   switch ( dri_priv->deviceID ) {
   default:
      fprintf(stderr, "unknown chip id, assuming full radeon support\n");
   case PCI_CHIP_RADEON_QD:
   case PCI_CHIP_RADEON_QE:
   case PCI_CHIP_RADEON_QF:
   case PCI_CHIP_RADEON_QG:
      /* all original radeons (7200) presumably have a stencil op bug */
      screen->chipset |= RADEON_CHIPSET_BROKEN_STENCIL;
   case PCI_CHIP_RV200_QW:
   case PCI_CHIP_RV200_QX:
   case PCI_CHIP_RADEON_LW:
   case PCI_CHIP_RADEON_LX:
      screen->chipset |= RADEON_CHIPSET_TCL;
   case PCI_CHIP_RADEON_QY:
   case PCI_CHIP_RADEON_QZ:
   case PCI_CHIP_RN50_515E:
   case PCI_CHIP_RN50_5969:
   case PCI_CHIP_RADEON_LY:
   case PCI_CHIP_RADEON_LZ:
   case PCI_CHIP_RS100_4136: /* IGPs don't have TCL */
   case PCI_CHIP_RS200_4137:
   case PCI_CHIP_RS250_4237:
   case PCI_CHIP_RS100_4336:
   case PCI_CHIP_RS200_4337:
   case PCI_CHIP_RS250_4437:
      break;
   }

   screen->cpp = dri_priv->bpp / 8;
   screen->AGPMode = dri_priv->AGPMode;

   screen->fbLocation	= ( INREG( RADEON_MC_FB_LOCATION ) & 0xffff ) << 16;

   if ( sPriv->drmMinor >= 10 ) {
      drm_radeon_setparam_t sp;

      sp.param = RADEON_SETPARAM_FB_LOCATION;
      sp.value = screen->fbLocation;

      drmCommandWrite( sPriv->fd, DRM_RADEON_SETPARAM,
		       &sp, sizeof( sp ) );
   }

   screen->frontOffset	= dri_priv->frontOffset;
   screen->frontPitch	= dri_priv->frontPitch;
   screen->backOffset	= dri_priv->backOffset;
   screen->backPitch	= dri_priv->backPitch;
   screen->depthOffset	= dri_priv->depthOffset;
   screen->depthPitch	= dri_priv->depthPitch;

   /* Check if ddx has set up a surface reg to cover depth buffer */
   screen->depthHasSurface = ((sPriv->ddxMajor > 4) &&
      (screen->chipset & RADEON_CHIPSET_TCL));

   screen->texOffset[RADEON_LOCAL_TEX_HEAP] = dri_priv->textureOffset
				       + screen->fbLocation;
   screen->texSize[RADEON_LOCAL_TEX_HEAP] = dri_priv->textureSize;
   screen->logTexGranularity[RADEON_LOCAL_TEX_HEAP] =
      dri_priv->log2TexGran;

   if ( !screen->gartTextures.map
	|| getenv( "RADEON_GARTTEXTURING_FORCE_DISABLE" ) ) {
      screen->numTexHeaps = RADEON_NR_TEX_HEAPS - 1;
      screen->texOffset[RADEON_GART_TEX_HEAP] = 0;
      screen->texSize[RADEON_GART_TEX_HEAP] = 0;
      screen->logTexGranularity[RADEON_GART_TEX_HEAP] = 0;
   } else {
      screen->numTexHeaps = RADEON_NR_TEX_HEAPS;
      screen->texOffset[RADEON_GART_TEX_HEAP] = screen->gart_texture_offset;
      screen->texSize[RADEON_GART_TEX_HEAP] = dri_priv->gartTexMapSize;
      screen->logTexGranularity[RADEON_GART_TEX_HEAP] =
	 dri_priv->log2GARTTexGran;
   }

   if ( glx_enable_extension != NULL ) {
      if ( screen->irq != 0 ) {
	 (*glx_enable_extension)( psc, "GLX_SGI_swap_control" );
	 (*glx_enable_extension)( psc, "GLX_SGI_video_sync" );
	 (*glx_enable_extension)( psc, "GLX_MESA_swap_control" );
      }

      (*glx_enable_extension)( psc, "GLX_MESA_swap_frame_usage" );
   }

   screen->driScreen = sPriv;
   screen->sarea_priv_offset = dri_priv->sarea_priv_offset;
   return screen;
}

/* Destroy the device specific screen private data struct.
 */
void radeonDestroyScreen( __DRIscreenPrivate *sPriv )
{
   radeonScreenPtr screen = (radeonScreenPtr)sPriv->private;

   if (!screen)
      return;

   if ( screen->gartTextures.map ) {
      drmUnmap( screen->gartTextures.map, screen->gartTextures.size );
   }
   drmUnmapBufs( screen->buffers );
   drmUnmap( screen->status.map, screen->status.size );
   drmUnmap( screen->mmio.map, screen->mmio.size );

   /* free all option information */
   driDestroyOptionInfo (&screen->optionCache);

   FREE( screen );
   sPriv->private = NULL;
}


/* Initialize the driver specific screen private data.
 */
static GLboolean
radeonInitDriver( __DRIscreenPrivate *sPriv )
{
   sPriv->private = (void *) radeonCreateScreen( sPriv );
   if ( !sPriv->private ) {
      radeonDestroyScreen( sPriv );
      return GL_FALSE;
   }

   return GL_TRUE;
}


/**
 * Create the Mesa framebuffer and renderbuffers for a given window/drawable.
 *
 * \todo This function (and its interface) will need to be updated to support
 * pbuffers.
 */
static GLboolean
radeonCreateBuffer( __DRIscreenPrivate *driScrnPriv,
                    __DRIdrawablePrivate *driDrawPriv,
                    const __GLcontextModes *mesaVis,
                    GLboolean isPixmap )
{
   radeonScreenPtr screen = (radeonScreenPtr) driScrnPriv->private;

   if (isPixmap) {
      return GL_FALSE; /* not implemented */
   }
   else {
      const GLboolean swDepth = GL_FALSE;
      const GLboolean swAlpha = GL_FALSE;
      const GLboolean swAccum = mesaVis->accumRedBits > 0;
      const GLboolean swStencil = mesaVis->stencilBits > 0 &&
         mesaVis->depthBits != 24;
#if 0
      driDrawPriv->driverPrivate = (void *)
         _mesa_create_framebuffer( mesaVis,
                                   swDepth,
                                   swStencil,
                                   swAccum,
                                   swAlpha );
#else
      struct gl_framebuffer *fb = _mesa_create_framebuffer(mesaVis);

      {
         driRenderbuffer *frontRb
            = driNewRenderbuffer(GL_RGBA, screen->cpp,
                                 screen->frontOffset, screen->frontPitch);
         radeonSetSpanFunctions(frontRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_FRONT_LEFT, &frontRb->Base);
      }

      if (mesaVis->doubleBufferMode) {
         driRenderbuffer *backRb
            = driNewRenderbuffer(GL_RGBA, screen->cpp,
                                 screen->backOffset, screen->backPitch);
         radeonSetSpanFunctions(backRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_BACK_LEFT, &backRb->Base);
      }

      if (mesaVis->depthBits == 16) {
         driRenderbuffer *depthRb
            = driNewRenderbuffer(GL_DEPTH_COMPONENT16, screen->cpp,
                                 screen->depthOffset, screen->depthPitch);
         radeonSetSpanFunctions(depthRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
      }
      else if (mesaVis->depthBits == 24) {
         driRenderbuffer *depthRb
            = driNewRenderbuffer(GL_DEPTH_COMPONENT24, screen->cpp,
                                 screen->depthOffset, screen->depthPitch);
         radeonSetSpanFunctions(depthRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
      }

      if (mesaVis->stencilBits > 0 && !swStencil) {
         driRenderbuffer *stencilRb
            = driNewRenderbuffer(GL_STENCIL_INDEX8_EXT, screen->cpp,
                                 screen->depthOffset, screen->depthPitch);
         radeonSetSpanFunctions(stencilRb, mesaVis);
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
#endif
      return (driDrawPriv->driverPrivate != NULL);
   }
}


static void
radeonDestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_destroy_framebuffer((GLframebuffer *) (driDrawPriv->driverPrivate));
}

static struct __DriverAPIRec radeonAPI = {
   .InitDriver      = radeonInitDriver,
   .DestroyScreen   = radeonDestroyScreen,
   .CreateContext   = radeonCreateContext,
   .DestroyContext  = radeonDestroyContext,
   .CreateBuffer    = radeonCreateBuffer,
   .DestroyBuffer   = radeonDestroyBuffer,
   .SwapBuffers     = radeonSwapBuffers,
   .MakeCurrent     = radeonMakeCurrent,
   .UnbindContext   = radeonUnbindContext,
   .GetSwapInfo     = getSwapInfo,
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
   static const __DRIutilversion2 ddx_expected = { 4, 5, 0, 0 };
   static const __DRIversion dri_expected = { 4, 0, 0 };
   static const __DRIversion drm_expected = { 1, 3, 0 };

   dri_interface = interface;

   if ( ! driCheckDriDdxDrmVersions3( "Radeon",
				      dri_version, & dri_expected,
				      ddx_version, & ddx_expected,
				      drm_version, & drm_expected ) ) {
      return NULL;
   }

   psp = __driUtilCreateNewScreen(dpy, scrn, psc, NULL,
				  ddx_version, dri_version, drm_version,
				  frame_buffer, pSAREA, fd,
				  internal_api_version, &radeonAPI);
   if ( psp != NULL ) {
      RADEONDRIPtr dri_priv = (RADEONDRIPtr) psp->pDevPriv;
      *driver_modes = radeonFillInModes( dri_priv->bpp,
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


/**
 * Get information about previous buffer swaps.
 */
static int
getSwapInfo( __DRIdrawablePrivate *dPriv, __DRIswapInfo * sInfo )
{
   radeonContextPtr  rmesa;

   if ( (dPriv == NULL) || (dPriv->driContextPriv == NULL)
	|| (dPriv->driContextPriv->driverPrivate == NULL)
	|| (sInfo == NULL) ) {
      return -1;
   }

   rmesa = (radeonContextPtr) dPriv->driContextPriv->driverPrivate;
   sInfo->swap_count = rmesa->swap_count;
   sInfo->swap_ust = rmesa->swap_ust;
   sInfo->swap_missed_count = rmesa->swap_missed_count;

   sInfo->swap_missed_usage = (sInfo->swap_missed_count != 0)
       ? driCalculateSwapUsage( dPriv, 0, rmesa->swap_missed_ust )
       : 0.0;

   return 0;
}
