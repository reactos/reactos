/* $XFree86$ */
/**************************************************************************

Copyright 2003 Eric Anholt
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
ERIC ANHOLT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Eric Anholt <anholt@FreeBSD.org>
 */

#include "dri_util.h"

#include "context.h"
#include "utils.h"
#include "imports.h"
#include "framebuffer.h"
#include "renderbuffer.h"

#include "sis_context.h"
#include "sis_dri.h"
#include "sis_lock.h"
#include "sis_span.h"

#include "xmlpool.h"

#include "GL/internal/dri_interface.h"

#define SIS_AGP_DISABLE(def) \
DRI_CONF_OPT_BEGIN(agp_disable,bool,def)				\
	DRI_CONF_DESC(en,"Disable AGP vertex dispatch")			\
DRI_CONF_OPT_END

PUBLIC const char __driConfigOptions[] =
DRI_CONF_BEGIN
	DRI_CONF_SECTION_QUALITY
		DRI_CONF_TEXTURE_DEPTH(DRI_CONF_TEXTURE_DEPTH_FB)
	DRI_CONF_SECTION_END
	DRI_CONF_SECTION_DEBUG
		SIS_AGP_DISABLE(true)
		DRI_CONF_NO_RAST(false)
	DRI_CONF_SECTION_END
DRI_CONF_END;
static const GLuint __driNConfigOptions = 3;

extern const struct dri_extension card_extensions[];

static __GLcontextModes *
sisFillInModes(int bpp)
{
   __GLcontextModes *modes;
   __GLcontextModes *m;
   unsigned num_modes;
   unsigned depth_buffer_factor;
   unsigned back_buffer_factor;
   GLenum fb_format;
   GLenum fb_type;
   static const GLenum back_buffer_modes[] = {
      GLX_NONE, GLX_SWAP_UNDEFINED_OML
   };
   u_int8_t depth_bits_array[4];
   u_int8_t stencil_bits_array[4];

   depth_bits_array[0] = 0;
   stencil_bits_array[0] = 0;
   depth_bits_array[1] = 16;
   stencil_bits_array[1] = 0;
   depth_bits_array[2] = 24;
   stencil_bits_array[2] = 8;
   depth_bits_array[3] = 32;
   stencil_bits_array[3] = 0;

   depth_buffer_factor = 4;
   back_buffer_factor = 2;

   /* Last 4 is for GLX_TRUE_COLOR & GLX_DIRECT_COLOR, with/without accum */
   num_modes = depth_buffer_factor * back_buffer_factor * 4;

   if (bpp == 16) {
      fb_format = GL_RGB;
      fb_type = GL_UNSIGNED_SHORT_5_6_5;
   } else {
      fb_format = GL_BGRA;
      fb_type = GL_UNSIGNED_INT_8_8_8_8_REV;
   }

   modes = (*dri_interface->createContextModes)(num_modes, sizeof(__GLcontextModes));
   m = modes;
   if (!driFillInModes(&m, fb_format, fb_type, depth_bits_array,
		       stencil_bits_array, depth_buffer_factor,
		       back_buffer_modes, back_buffer_factor,
		       GLX_TRUE_COLOR)) {
      fprintf(stderr, "[%s:%u] Error creating FBConfig!\n", __func__, __LINE__);
      return NULL;
   }

   if (!driFillInModes(&m, fb_format, fb_type, depth_bits_array,
		       stencil_bits_array, depth_buffer_factor,
		       back_buffer_modes, back_buffer_factor,
		       GLX_DIRECT_COLOR)) {
      fprintf(stderr, "[%s:%u] Error creating FBConfig!\n", __func__, __LINE__);
      return NULL;
   }

   return modes;
}


/* Create the device specific screen private data struct.
 */
static sisScreenPtr
sisCreateScreen( __DRIscreenPrivate *sPriv )
{
   sisScreenPtr sisScreen;
   SISDRIPtr sisDRIPriv = (SISDRIPtr)sPriv->pDevPriv;

   if (sPriv->devPrivSize != sizeof(SISDRIRec)) {
      fprintf(stderr,"\nERROR!  sizeof(SISDRIRec) does not match passed size from device driver\n");
      return GL_FALSE;
   }

   /* Allocate the private area */
   sisScreen = (sisScreenPtr)CALLOC( sizeof(*sisScreen) );
   if ( sisScreen == NULL )
      return NULL;

   sisScreen->screenX = sisDRIPriv->width;
   sisScreen->screenY = sisDRIPriv->height;
   sisScreen->cpp = sisDRIPriv->bytesPerPixel;
   sisScreen->deviceID = sisDRIPriv->deviceID;
   sisScreen->AGPCmdBufOffset = sisDRIPriv->AGPCmdBufOffset;
   sisScreen->AGPCmdBufSize = sisDRIPriv->AGPCmdBufSize;
   sisScreen->sarea_priv_offset = sizeof(drm_sarea_t);

   sisScreen->mmio.handle = sisDRIPriv->regs.handle;
   sisScreen->mmio.size   = sisDRIPriv->regs.size;
   if ( drmMap( sPriv->fd, sisScreen->mmio.handle, sisScreen->mmio.size,
	       &sisScreen->mmio.map ) )
   {
      FREE( sisScreen );
      return NULL;
   }

   if (sisDRIPriv->agp.size) {
      sisScreen->agp.handle = sisDRIPriv->agp.handle;
      sisScreen->agpBaseOffset = drmAgpBase(sPriv->fd);
      sisScreen->agp.size   = sisDRIPriv->agp.size;
      if ( drmMap( sPriv->fd, sisScreen->agp.handle, sisScreen->agp.size,
                   &sisScreen->agp.map ) )
      {
         sisScreen->agp.size = 0;
      }
   }

   sisScreen->driScreen = sPriv;

   /* parse information in __driConfigOptions */
   driParseOptionInfo(&sisScreen->optionCache,
		      __driConfigOptions, __driNConfigOptions);

   return sisScreen;
}

/* Destroy the device specific screen private data struct.
 */
static void
sisDestroyScreen( __DRIscreenPrivate *sPriv )
{
   sisScreenPtr sisScreen = (sisScreenPtr)sPriv->private;

   if ( sisScreen == NULL )
      return;

   if (sisScreen->agp.size != 0)
      drmUnmap( sisScreen->agp.map, sisScreen->agp.size );
   drmUnmap( sisScreen->mmio.map, sisScreen->mmio.size );

   FREE( sisScreen );
   sPriv->private = NULL;
}


/* Create and initialize the Mesa and driver specific pixmap buffer
 * data.
 */
static GLboolean
sisCreateBuffer( __DRIscreenPrivate *driScrnPriv,
                 __DRIdrawablePrivate *driDrawPriv,
                 const __GLcontextModes *mesaVis,
                 GLboolean isPixmap )
{
   /*sisScreenPtr screen = (sisScreenPtr) driScrnPriv->private;*/
   struct gl_framebuffer *fb;

   if (isPixmap)
      return GL_FALSE; /* not implemented */

   fb = _mesa_create_framebuffer(mesaVis);

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


static void
sisDestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_unreference_framebuffer((GLframebuffer **)(&(driDrawPriv->driverPrivate)));
}

static void sisCopyBuffer( __DRIdrawablePrivate *dPriv )
{
   sisContextPtr smesa = (sisContextPtr)dPriv->driContextPriv->driverPrivate;
   int i;

   while ((*smesa->FrameCountPtr) - MMIO_READ(0x8a2c) > SIS_MAX_FRAME_LENGTH)
      ;

   LOCK_HARDWARE();

   for (i = 0; i < dPriv->numClipRects; i++) {
      drm_clip_rect_t *box = &dPriv->pClipRects[i];

      mWait3DCmdQueue(10);
      MMIO(REG_SRC_ADDR, smesa->back.offset);
      MMIO(REG_SRC_PITCH, smesa->back.pitch | ((smesa->bytesPerPixel == 4) ? 
			   BLIT_DEPTH_32 : BLIT_DEPTH_16));
      MMIO(REG_SRC_X_Y, ((box->x1 - dPriv->x) << 16) | (box->y1 - dPriv->y));
      MMIO(REG_DST_X_Y, ((box->x1 - dPriv->x) << 16) | (box->y1 - dPriv->y));
      MMIO(REG_DST_ADDR, smesa->front.offset);
      MMIO(REG_DST_PITCH_HEIGHT, (smesa->virtualY << 16) | smesa->front.pitch);
      MMIO(REG_WIDTH_HEIGHT, ((box->y2 - box->y1) << 16) | (box->x2 - box->x1));
      MMIO(REG_BLIT_CMD, CMD_DIR_X_INC | CMD_DIR_Y_INC | CMD_ROP_SRC);
      MMIO(REG_CommandQueue, -1);
   }

   *(GLint *)(smesa->IOBase+0x8a2c) = *smesa->FrameCountPtr;
   (*smesa->FrameCountPtr)++;  

   UNLOCK_HARDWARE ();
}


/* Copy the back color buffer to the front color buffer */
static void
sisSwapBuffers(__DRIdrawablePrivate *dPriv)
{
   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
         sisContextPtr smesa = (sisContextPtr) dPriv->driContextPriv->driverPrivate;
         GLcontext *ctx = smesa->glCtx;

      if (ctx->Visual.doubleBufferMode) {
         _mesa_notifySwapBuffers( ctx );  /* flush pending rendering comands */
         sisCopyBuffer( dPriv );
      }
   } else {
      /* XXX this shouldn't be an error but we can't handle it for now */
      _mesa_problem(NULL, "%s: drawable has no context!", __FUNCTION__);
   }
}


/* Initialize the driver specific screen private data.
 */
static GLboolean
sisInitDriver( __DRIscreenPrivate *sPriv )
{
   sPriv->private = (void *) sisCreateScreen( sPriv );

   if ( !sPriv->private ) {
      sisDestroyScreen( sPriv );
      return GL_FALSE;
   }

   return GL_TRUE;
}

static struct __DriverAPIRec sisAPI = {
   .InitDriver      = sisInitDriver,
   .DestroyScreen   = sisDestroyScreen,
   .CreateContext   = sisCreateContext,
   .DestroyContext  = sisDestroyContext,
   .CreateBuffer    = sisCreateBuffer,
   .DestroyBuffer   = sisDestroyBuffer,
   .SwapBuffers     = sisSwapBuffers,
   .MakeCurrent     = sisMakeCurrent,
   .UnbindContext   = sisUnbindContext,
   .GetSwapInfo     = NULL,
   .GetMSC          = NULL,
   .WaitForMSC      = NULL,
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
void * __driCreateNewScreen_20050727( __DRInativeDisplay *dpy, int scrn,
			     __DRIscreen *psc,
			     const __GLcontextModes *modes,
			     const __DRIversion *ddx_version,
			     const __DRIversion *dri_version,
			     const __DRIversion *drm_version,
			     const __DRIframebuffer *frame_buffer,
			     drmAddress pSAREA, int fd,
			     int internal_api_version,
			     const __DRIinterfaceMethods * interface,
			     __GLcontextModes **driver_modes )

{
   __DRIscreenPrivate *psp;
   static const __DRIversion ddx_expected = {0, 8, 0};
   static const __DRIversion dri_expected = {4, 0, 0};
   static const __DRIversion drm_expected = {1, 0, 0};
   static const char *driver_name = "SiS";
   dri_interface = interface;

   if (!driCheckDriDdxDrmVersions2(driver_name, dri_version, &dri_expected,
				   ddx_version, &ddx_expected,
				   drm_version, &drm_expected)) {
      return NULL;
   }

   psp = __driUtilCreateNewScreen(dpy, scrn, psc, NULL,
				  ddx_version, dri_version, drm_version,
				  frame_buffer, pSAREA, fd,
				  internal_api_version, &sisAPI);
   if (psp != NULL) {
      SISDRIPtr dri_priv = (SISDRIPtr)psp->pDevPriv;
      *driver_modes = sisFillInModes(dri_priv->bytesPerPixel * 8);

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

   return (void *)psp;
}
