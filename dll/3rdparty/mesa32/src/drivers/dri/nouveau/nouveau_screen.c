/**************************************************************************

Copyright 2006 Stephane Marchesin
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
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

#include "glheader.h"
#include "imports.h"
#include "mtypes.h"
#include "framebuffer.h"
#include "renderbuffer.h"

#include "nouveau_context.h"
#include "nouveau_screen.h"
#include "nouveau_object.h"
#include "nouveau_span.h"

#include "utils.h"
#include "context.h"
#include "vblank.h"
#include "drirenderbuffer.h"

#include "GL/internal/dri_interface.h"

#include "xmlpool.h"

PUBLIC const char __driConfigOptions[] =
DRI_CONF_BEGIN
    DRI_CONF_SECTION_DEBUG
        DRI_CONF_NO_RAST(false)
    DRI_CONF_SECTION_END
DRI_CONF_END;
static const GLuint __driNConfigOptions = 1;

extern const struct dri_extension common_extensions[];
extern const struct dri_extension nv10_extensions[];
extern const struct dri_extension nv20_extensions[];
extern const struct dri_extension nv30_extensions[];
extern const struct dri_extension nv40_extensions[];
extern const struct dri_extension nv50_extensions[];

static nouveauScreenPtr nouveauCreateScreen(__DRIscreenPrivate *sPriv)
{
	nouveauScreenPtr screen;
	NOUVEAUDRIPtr dri_priv=(NOUVEAUDRIPtr)sPriv->pDevPriv;

	/* allocate screen */
	screen = (nouveauScreenPtr) CALLOC( sizeof(*screen) );
	if ( !screen ) {         
		__driUtilMessage("%s: Could not allocate memory for screen structure",__FUNCTION__);
		return NULL;
	}
	
	screen->card=nouveau_card_lookup(dri_priv->device_id);
	if (!screen->card) {
		__driUtilMessage("%s: Unknown card type 0x%04x:0x%04x\n",
			__func__, dri_priv->device_id >> 16, dri_priv->device_id & 0xFFFF);
		FREE(screen);
		return NULL;
	}

	/* parse information in __driConfigOptions */
	driParseOptionInfo (&screen->optionCache,__driConfigOptions, __driNConfigOptions);

	screen->fbFormat    = dri_priv->bpp / 8;
	screen->frontOffset = dri_priv->front_offset;
	screen->frontPitch  = dri_priv->front_pitch;
	screen->backOffset  = dri_priv->back_offset;
	screen->backPitch   = dri_priv->back_pitch;
	screen->depthOffset = dri_priv->depth_offset;
	screen->depthPitch  = dri_priv->depth_pitch;

	screen->driScreen = sPriv;
	return screen;
}

static void
nouveauDestroyScreen(__DRIscreenPrivate *sPriv)
{
	nouveauScreenPtr screen = (nouveauScreenPtr)sPriv->private;

	if (!screen) return;

	/* free all option information */
	driDestroyOptionInfo (&screen->optionCache);

	FREE(screen);
	sPriv->private = NULL;
}

static GLboolean nouveauInitDriver(__DRIscreenPrivate *sPriv)
{
	sPriv->private = (void *) nouveauCreateScreen( sPriv );
	if ( !sPriv->private ) {
		nouveauDestroyScreen( sPriv );
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
nouveauCreateBuffer(__DRIscreenPrivate *driScrnPriv,
                    __DRIdrawablePrivate *driDrawPriv,
                    const __GLcontextModes *mesaVis,
                    GLboolean isPixmap)
{
	nouveauScreenPtr screen = (nouveauScreenPtr) driScrnPriv->private;
	nouveau_renderbuffer  *nrb;
	struct gl_framebuffer *fb;
	const GLboolean swAccum = mesaVis->accumRedBits > 0;
	const GLboolean swStencil = mesaVis->stencilBits > 0 && mesaVis->depthBits != 24;
	GLenum color_format = screen->fbFormat == 4 ? GL_RGBA8 : GL_RGB5;

	if (isPixmap)
		return GL_FALSE; /* not implemented */

	fb = _mesa_create_framebuffer(mesaVis);
	if (!fb)
		return GL_FALSE;

	/* Front buffer */
	nrb = nouveau_renderbuffer_new(color_format,
				       driScrnPriv->pFB + screen->frontOffset,
				       screen->frontOffset,
				       screen->frontPitch * screen->fbFormat,
				       driDrawPriv);
	nouveauSpanSetFunctions(nrb, mesaVis);
	_mesa_add_renderbuffer(fb, BUFFER_FRONT_LEFT, &nrb->mesa);

	if (0 /* unified buffers if we choose to support them.. */) {
	} else {
		if (mesaVis->doubleBufferMode) {
			nrb = nouveau_renderbuffer_new(color_format, NULL,
						       0, 0,
						       NULL);
			nouveauSpanSetFunctions(nrb, mesaVis);
			_mesa_add_renderbuffer(fb, BUFFER_BACK_LEFT, &nrb->mesa);
		}

		if (mesaVis->depthBits == 24 && mesaVis->stencilBits == 8) {
			nrb = nouveau_renderbuffer_new(GL_DEPTH24_STENCIL8_EXT, NULL,
						       0, 0,
						       NULL);
			nouveauSpanSetFunctions(nrb, mesaVis);
			_mesa_add_renderbuffer(fb, BUFFER_DEPTH, &nrb->mesa);
			_mesa_add_renderbuffer(fb, BUFFER_STENCIL, &nrb->mesa);
		} else if (mesaVis->depthBits == 24) {
			nrb = nouveau_renderbuffer_new(GL_DEPTH_COMPONENT24, NULL,
						       0, 0,
						       NULL);
			nouveauSpanSetFunctions(nrb, mesaVis);
			_mesa_add_renderbuffer(fb, BUFFER_DEPTH, &nrb->mesa);
		} else if (mesaVis->depthBits == 16) {
			nrb = nouveau_renderbuffer_new(GL_DEPTH_COMPONENT16, NULL,
						       0, 0,
						       NULL);
			nouveauSpanSetFunctions(nrb, mesaVis);
			_mesa_add_renderbuffer(fb, BUFFER_DEPTH, &nrb->mesa);
		}
	}

	_mesa_add_soft_renderbuffers(fb,
				     GL_FALSE, /* color */
				     GL_FALSE, /* depth */
				     swStencil,
				     swAccum,
				     GL_FALSE, /* alpha */
				     GL_FALSE  /* aux */);

	driDrawPriv->driverPrivate = (void *) fb;
	return (driDrawPriv->driverPrivate != NULL);
}


static void
nouveauDestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
	_mesa_unreference_framebuffer((GLframebuffer **)(&(driDrawPriv->driverPrivate)));
}

static int
nouveauGetSwapInfo(__DRIdrawablePrivate *dpriv, __DRIswapInfo *sInfo)
{
	return -1;
}

static const struct __DriverAPIRec nouveauAPI = {
	.InitDriver      = nouveauInitDriver,
	.DestroyScreen   = nouveauDestroyScreen,
	.CreateContext   = nouveauCreateContext,
	.DestroyContext  = nouveauDestroyContext,
	.CreateBuffer    = nouveauCreateBuffer,
	.DestroyBuffer   = nouveauDestroyBuffer,
	.SwapBuffers     = nouveauSwapBuffers,
	.MakeCurrent     = nouveauMakeCurrent,
	.UnbindContext   = nouveauUnbindContext,
	.GetSwapInfo     = nouveauGetSwapInfo,
	.GetMSC          = driGetMSC32,
	.WaitForMSC      = driWaitForMSC32,
	.WaitForSBC      = NULL,
	.SwapBuffersMSC  = NULL,
	.CopySubBuffer   = nouveauCopySubBuffer
};


static __GLcontextModes *
nouveauFillInModes( unsigned pixel_bits, unsigned depth_bits,
		 unsigned stencil_bits, GLboolean have_back_buffer )
{
	__GLcontextModes * modes;
	__GLcontextModes * m;
	unsigned num_modes;
	unsigned depth_buffer_factor;
	unsigned back_buffer_factor;
	int i;

	static const struct {
		GLenum format;
		GLenum type;
	} fb_format_array[] = {
		{ GL_RGB , GL_UNSIGNED_SHORT_5_6_5     },
		{ GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV },
		{ GL_BGR , GL_UNSIGNED_INT_8_8_8_8_REV },
	};

	/* GLX_SWAP_COPY_OML is only supported because the Intel driver doesn't
	 * support pageflipping at all.
	 */
	static const GLenum back_buffer_modes[] = {
		GLX_NONE, GLX_SWAP_UNDEFINED_OML, GLX_SWAP_COPY_OML
	};

	u_int8_t depth_bits_array[4]   = { 0, 16, 24, 24 };
	u_int8_t stencil_bits_array[4] = { 0,  0,  0,  8 };

	depth_buffer_factor = 4;
	back_buffer_factor  = (have_back_buffer) ? 3 : 1;

	num_modes = ((pixel_bits==16) ? 1 : 2) *
		depth_buffer_factor * back_buffer_factor * 4;
	modes = (*dri_interface->createContextModes)(num_modes,
						     sizeof(__GLcontextModes));
	m = modes;

	for (i=((pixel_bits==16)?0:1);i<((pixel_bits==16)?1:3);i++) {
		if (!driFillInModes(&m, fb_format_array[i].format,
					fb_format_array[i].type,
					depth_bits_array,
					stencil_bits_array,
					depth_buffer_factor,
					back_buffer_modes,
					back_buffer_factor,
					GLX_TRUE_COLOR)) {
		fprintf( stderr, "[%s:%u] Error creating FBConfig!\n",
				__func__, __LINE__ );
		return NULL;
		}

		if (!driFillInModes(&m, fb_format_array[i].format,
					fb_format_array[i].type,
					depth_bits_array,
					stencil_bits_array,
					depth_buffer_factor,
					back_buffer_modes,
					back_buffer_factor,
					GLX_DIRECT_COLOR)) {
		fprintf( stderr, "[%s:%u] Error creating FBConfig!\n",
				__func__, __LINE__ );
		return NULL;
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
				     __GLcontextModes ** driver_modes)
			     
{
	__DRIscreenPrivate *psp;
	static const __DRIversion ddx_expected = { 1, 2, 0 };
	static const __DRIversion dri_expected = { 4, 0, 0 };
	static const __DRIversion drm_expected = { 0, 0, NOUVEAU_DRM_HEADER_PATCHLEVEL };
#if NOUVEAU_DRM_HEADER_PATCHLEVEL != 6
#error nouveau_drm.h version doesn't match expected version
#endif
	dri_interface = interface;

	if (!driCheckDriDdxDrmVersions2("nouveau",
					dri_version, & dri_expected,
					ddx_version, & ddx_expected,
					drm_version, & drm_expected)) {
		return NULL;
	}

	// temporary lock step versioning
	if (drm_expected.patch!=drm_version->patch) {
		__driUtilMessage("%s: wrong DRM version, expected %d, got %d\n",
				__func__,
				drm_expected.patch, drm_version->patch);
		return NULL;
	}

	psp = __driUtilCreateNewScreen(dpy, scrn, psc, NULL,
				       ddx_version, dri_version, drm_version,
				       frame_buffer, pSAREA, fd,
				       internal_api_version, &nouveauAPI);
	if ( psp != NULL ) {
		NOUVEAUDRIPtr dri_priv = (NOUVEAUDRIPtr)psp->pDevPriv;

		*driver_modes = nouveauFillInModes(dri_priv->bpp,
						   (dri_priv->bpp == 16) ? 16 : 24,
						   (dri_priv->bpp == 16) ? 0  : 8,
						   1
						   );

		/* Calling driInitExtensions here, with a NULL context pointer, does not actually
		 * enable the extensions.  It just makes sure that all the dispatch offsets for all
		 * the extensions that *might* be enables are known.  This is needed because the
		 * dispatch offsets need to be known when _mesa_context_create is called, but we can't
		 * enable the extensions until we have a context pointer.
		 * 
		 * Hello chicken.  Hello egg.  How are you two today?
		 */
		driInitExtensions( NULL, common_extensions, GL_FALSE );
		driInitExtensions( NULL,   nv10_extensions, GL_FALSE );
		driInitExtensions( NULL,   nv10_extensions, GL_FALSE );
		driInitExtensions( NULL,   nv30_extensions, GL_FALSE );
		driInitExtensions( NULL,   nv40_extensions, GL_FALSE );
		driInitExtensions( NULL,   nv50_extensions, GL_FALSE );
	}

	return (void *) psp;
}

