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

#include "nouveau_context.h"
//#include "nouveau_state.h"
#include "nouveau_lock.h"
#include "nouveau_fifo.h"
#include "nouveau_driver.h"
#include "swrast/swrast.h"

#include "context.h"
#include "framebuffer.h"

#include "utils.h"

/* Wrapper for DRM_NOUVEAU_GETPARAM ioctl */
GLboolean nouveauDRMGetParam(nouveauContextPtr nmesa,
			     unsigned int      param,
			     uint64_t*         value)
{
	drm_nouveau_getparam_t getp;

	getp.param = param;
	if (!value || drmCommandWriteRead(nmesa->driFd, DRM_NOUVEAU_GETPARAM,
					  &getp, sizeof(getp)))
		return GL_FALSE;
	*value = getp.value;
	return GL_TRUE;
}

/* Wrapper for DRM_NOUVEAU_GETPARAM ioctl */
GLboolean nouveauDRMSetParam(nouveauContextPtr nmesa,
			     unsigned int      param,
			     uint64_t          value)
{
	drm_nouveau_setparam_t setp;

	setp.param = param;
	setp.value = value;
	if (drmCommandWrite(nmesa->driFd, DRM_NOUVEAU_SETPARAM, &setp,
				sizeof(setp)))
		return GL_FALSE;
	return GL_TRUE;
}

/* Return the width and height of the current color buffer */
static void nouveauGetBufferSize( GLframebuffer *buffer,
		GLuint *width, GLuint *height )
{
	GET_CURRENT_CONTEXT(ctx);
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	LOCK_HARDWARE( nmesa );
	*width  = nmesa->driDrawable->w;
	*height = nmesa->driDrawable->h;
	UNLOCK_HARDWARE( nmesa );
}

/* glGetString */
static const GLubyte *nouveauGetString( GLcontext *ctx, GLenum name )
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	static char buffer[128];
	const char * card_name = "Unknown";
	GLuint agp_mode = 0;

	switch ( name ) {
		case GL_VENDOR:
			return (GLubyte *)DRIVER_AUTHOR;

		case GL_RENDERER:
			card_name=nmesa->screen->card->name;

			switch(nmesa->screen->bus_type)
			{
				case NV_PCI:
				case NV_PCIE:
				default:
					agp_mode=0;
					break;
				case NV_AGP:
					agp_mode=nmesa->screen->agp_mode;
					break;
			}
			driGetRendererString( buffer, card_name, DRIVER_DATE,
					agp_mode );
			return (GLubyte *)buffer;
		default:
			return NULL;
	}
}

/* glFlush */
static void nouveauFlush( GLcontext *ctx )
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	FIRE_RING();
}

/* glFinish */
static void nouveauFinish( GLcontext *ctx )
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	nouveauFlush( ctx );
	nouveauWaitForIdle( nmesa );
}

/* glClear */
static void nouveauClear( GLcontext *ctx, GLbitfield mask )
{
	// XXX we really should do something here...
}

void nouveauDriverInitFunctions( struct dd_function_table *functions )
{
	functions->GetBufferSize	= nouveauGetBufferSize;
	functions->ResizeBuffers	= _mesa_resize_framebuffer;
	functions->GetString		= nouveauGetString;
	functions->Flush		= nouveauFlush;
	functions->Finish		= nouveauFinish;
	functions->Clear		= nouveauClear;
}

