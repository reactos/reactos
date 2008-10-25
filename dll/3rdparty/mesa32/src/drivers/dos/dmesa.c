/*
 * Mesa 3-D graphics library
 * Version:  6.3
 * 
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * DOS/DJGPP device driver for Mesa
 *
 *  Author: Daniel Borca
 *  Email : dborca@users.sourceforge.net
 *  Web   : http://www.geocities.com/dborca
 */


#include "context.h"
#include "imports.h"
#include "mtypes.h"

#include "video.h"

#include "GL/osmesa.h"
#include "GL/dmesa.h"


/*
 * This has nothing to do with Mesa Visual structure.
 * We keep this one around for backwards compatibility,
 * and to store video mode data for DMesaCreateContext.
 */
struct dmesa_visual {
    GLenum format;		/* OSMesa framebuffer format */
    GLint depthBits;
    GLint stencilBits;
    GLint accumBits;
};

/*
 * This has nothing to do with Mesa Buffer structure.
 * We keep this one around for backwards compatibility,
 * and to store various data.
 */
struct dmesa_buffer {
    int xpos, ypos;              /* position */
    int width, height;           /* size in pixels */
    GLenum type;
    void *the_window;            /* your window handle, etc */
};

/*
 * This has nothing to do with Mesa Context structure.
 * We keep this one around for backwards compatibility,
 * and to store real off-screen context.
 */
struct dmesa_context {
    OSMesaContext osmesa;
    DMesaBuffer buffer;
};


static DMesaContext ctx;


/****************************************************************************
 * DMesa Public API Functions
 ***************************************************************************/

/*
 * The exact arguments to this function will depend on your window system
 */
DMesaVisual
DMesaCreateVisual (GLint width,
                   GLint height,
                   GLint colDepth,
                   GLint refresh,
                   GLboolean dbFlag,
                   GLboolean rgbFlag,
                   GLint alphaSize,
                   GLint depthSize,
                   GLint stencilSize,
                   GLint accumSize)
{
    DMesaVisual visual;
    GLenum format;
    int fbbits;

    if (dbFlag) {
	return NULL;
    }

    if (!rgbFlag) {
	format = OSMESA_COLOR_INDEX;
	fbbits = 8;
    } else if (alphaSize) {
	format = OSMESA_BGRA;
	fbbits = 32;
    } else if (colDepth == 15 || colDepth == 16) {
	format = OSMESA_RGB_565;
	fbbits = 16;
    } else {
	format = OSMESA_BGR;
	fbbits = 24;
    }

    if ((visual = (DMesaVisual)CALLOC_STRUCT(dmesa_visual)) == NULL) {
	return NULL;
    }

    if (vl_video_init(width, height, colDepth, rgbFlag, refresh, fbbits) <= 0) {
	FREE(visual);
	return NULL;
    }

    visual->format = format;
    visual->depthBits = depthSize;
    visual->stencilBits = stencilSize;
    visual->accumBits = accumSize;
    return visual;
}


void
DMesaDestroyVisual (DMesaVisual visual)
{
   vl_video_exit();
   FREE(visual);
}


DMesaBuffer
DMesaCreateBuffer (DMesaVisual visual,
                   GLint xpos, GLint ypos,
                   GLint width, GLint height)
{
    DMesaBuffer buffer;
    GLenum type;
    int bytesPerPixel;

    switch (visual->format) {
	case OSMESA_COLOR_INDEX:
	    bytesPerPixel = 1;
	    type = CHAN_TYPE;
	    break;
	case OSMESA_RGB_565:
	    bytesPerPixel = 2;
	    type = GL_UNSIGNED_SHORT_5_6_5;
	    break;
	case OSMESA_BGR:
	    bytesPerPixel = 3;
	    type = CHAN_TYPE;
	    break;
	default:
	    bytesPerPixel = 4;
	    type = CHAN_TYPE;
    }

    if ((buffer = (DMesaBuffer)CALLOC_STRUCT(dmesa_buffer)) != NULL) {
	buffer->xpos = xpos;
	buffer->ypos = ypos;
	buffer->width = width;
	buffer->height = height;
	buffer->type = type;
	buffer->the_window = MALLOC(width * height * bytesPerPixel + 1);
	if (buffer->the_window == NULL) {
	    FREE(buffer);
	    buffer = NULL;
	}
    }

    return buffer;
}


void
DMesaDestroyBuffer (DMesaBuffer buffer)
{
    FREE(buffer->the_window);
    FREE(buffer);
}


DMesaContext
DMesaCreateContext (DMesaVisual visual, DMesaContext share)
{
    DMesaContext dmesa;
    if ((dmesa = (DMesaContext)CALLOC_STRUCT(dmesa_context)) != NULL) {
	dmesa->osmesa = OSMesaCreateContextExt(
				visual->format,
				visual->depthBits,
				visual->stencilBits,
				visual->accumBits,
				(share != NULL) ? share->osmesa : NULL);
	if (dmesa->osmesa == NULL) {
	    FREE(dmesa);
	    dmesa = NULL;
	}
    }
    return dmesa;
}


void
DMesaDestroyContext (DMesaContext dmesa)
{
    OSMesaDestroyContext(dmesa->osmesa);
    FREE(dmesa);
}


GLboolean
DMesaMoveBuffer (GLint xpos, GLint ypos)
{
    const DMesaContext dmesa = DMesaGetCurrentContext();
    DMesaBuffer b = dmesa->buffer;

    if (vl_sync_buffer(&b->the_window, xpos, ypos, b->width, b->height) == 0) {
	b->xpos = xpos;
	b->ypos = ypos;
	return GL_TRUE;
    }

    return GL_FALSE;
}


GLboolean
DMesaResizeBuffer (GLint width, GLint height)
{
    const DMesaContext dmesa = DMesaGetCurrentContext();
    DMesaBuffer b = dmesa->buffer;

    if (vl_sync_buffer(&b->the_window, b->xpos, b->ypos, width, height) == 0) {
	b->width = width;
	b->height = height;
	return GL_TRUE;
    }

    return GL_FALSE;
}


GLboolean
DMesaMakeCurrent (DMesaContext dmesa, DMesaBuffer buffer)
{
    if (dmesa == NULL || buffer == NULL) {
	ctx = NULL;
	return GL_TRUE;
    }
    if (OSMesaMakeCurrent(dmesa->osmesa, buffer->the_window,
			  buffer->type,
			  buffer->width, buffer->height) &&
	vl_sync_buffer(&buffer->the_window, buffer->xpos, buffer->ypos, buffer->width, buffer->height) == 0) {
	OSMesaPixelStore(OSMESA_Y_UP, GL_FALSE);
	dmesa->buffer = buffer;
	ctx = dmesa;
	return GL_TRUE;
    }
    return GL_FALSE;
}


void
DMesaSwapBuffers (DMesaBuffer buffer)
{
    /* copy/swap back buffer to front if applicable */
    GET_CURRENT_CONTEXT(ctx);
    _mesa_notifySwapBuffers(ctx);
    vl_flip();
    (void)buffer;
}


void
DMesaSetCI (int ndx, GLfloat red, GLfloat green, GLfloat blue)
{
    vl_setCI(ndx, red, green, blue);
}


DMesaContext
DMesaGetCurrentContext (void)
{
   return ctx;
}


DMesaBuffer
DMesaGetCurrentBuffer (void)
{
    const DMesaContext dmesa = DMesaGetCurrentContext();

    if (dmesa != NULL) {
	return dmesa->buffer;
    }

    return NULL;
}


DMesaProc
DMesaGetProcAddress (const char *name)
{
   DMesaProc p = (DMesaProc)_glapi_get_proc_address(name);

   /* TODO: handle DMesa* namespace
   if (p == NULL) {
   }
   */

   return p;
}


int
DMesaGetIntegerv (GLenum pname, GLint *params)
{
    switch (pname) {
	case DMESA_GET_SCREEN_SIZE:
	    vl_get(VL_GET_SCREEN_SIZE, params);
	    break;
	case DMESA_GET_DRIVER_CAPS:
	    params[0] = 0;
	    break;
	case DMESA_GET_VIDEO_MODES:
	    return vl_get(VL_GET_VIDEO_MODES, params);
	case DMESA_GET_BUFFER_ADDR: {
	    const DMesaContext dmesa = DMesaGetCurrentContext();
	    if (dmesa != NULL) {
		DMesaBuffer b = dmesa->buffer;
		if (b != NULL) {
		    params[0] = (GLint)b->the_window;
		}
	    }
	    break;
	}
	default:
	    return -1;
    }

    return 0;
}
