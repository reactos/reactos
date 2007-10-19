/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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
 *
 */

/**
 * \file swrast/swrast.h
 * \brief Public interface to the software rasterization functions.
 * \author Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef SWRAST_H
#define SWRAST_H

#include "mtypes.h"

/**
 * \struct SWvertex
 * \brief Data-structure to handle vertices in the software rasterizer.
 *
 * The software rasterizer now uses this format for vertices.  Thus a
 * 'RasterSetup' stage or other translation is required between the
 * tnl module and the swrast rasterization functions.  This serves to
 * isolate the swrast module from the internals of the tnl module, and
 * improve its usefulness as a fallback mechanism for hardware
 * drivers.
 *
 * Full software drivers:
 *   - Register the rastersetup and triangle functions from
 *     utils/software_helper.
 *   - On statechange, update the rasterization pointers in that module.
 *
 * Rasterization hardware drivers:
 *   - Keep native rastersetup.
 *   - Implement native twoside,offset and unfilled triangle setup.
 *   - Implement a translator from native vertices to swrast vertices.
 *   - On partial fallback (mix of accelerated and unaccelerated
 *   prims), call a pass-through function which translates native
 *   vertices to SWvertices and calls the appropriate swrast function.
 *   - On total fallback (vertex format insufficient for state or all
 *     primitives unaccelerated), hook in swrast_setup instead.
 */
typedef struct {
   /** win[0], win[1] are the screen-coords of SWvertex.
    * win[2] is the z-buffer coord (if 16-bit Z buffer, in range [0,65535]).
    * win[3] is 1/w where w is the clip-space W coord.  This is the value
    * that clip{XYZ} were multiplied by to get ndc{XYZ}.
    */
   GLfloat win[4];
   GLfloat texcoord[MAX_TEXTURE_COORD_UNITS][4];
   GLchan color[4];
   GLchan specular[4];
   GLfloat fog;
   GLfloat index;
   GLfloat pointSize;
} SWvertex;


struct swrast_device_driver;


/* These are the public-access functions exported from swrast.
 */
extern void
_swrast_use_read_buffer( GLcontext *ctx );

extern void
_swrast_use_draw_buffer( GLcontext *ctx );

extern GLboolean
_swrast_CreateContext( GLcontext *ctx );

extern void
_swrast_DestroyContext( GLcontext *ctx );

/* Get a (non-const) reference to the device driver struct for swrast.
 */
extern struct swrast_device_driver *
_swrast_GetDeviceDriverReference( GLcontext *ctx );

extern void
_swrast_Bitmap( GLcontext *ctx,
		GLint px, GLint py,
		GLsizei width, GLsizei height,
		const struct gl_pixelstore_attrib *unpack,
		const GLubyte *bitmap );

extern void
_swrast_CopyPixels( GLcontext *ctx,
		    GLint srcx, GLint srcy,
		    GLint destx, GLint desty,
		    GLsizei width, GLsizei height,
		    GLenum type );

extern void
_swrast_DrawPixels( GLcontext *ctx,
		    GLint x, GLint y,
		    GLsizei width, GLsizei height,
		    GLenum format, GLenum type,
		    const struct gl_pixelstore_attrib *unpack,
		    const GLvoid *pixels );

extern void
_swrast_ReadPixels( GLcontext *ctx,
		    GLint x, GLint y, GLsizei width, GLsizei height,
		    GLenum format, GLenum type,
		    const struct gl_pixelstore_attrib *unpack,
		    GLvoid *pixels );

extern void
_swrast_Clear( GLcontext *ctx, GLbitfield mask, GLboolean all,
	       GLint x, GLint y, GLint width, GLint height );

extern void
_swrast_Accum( GLcontext *ctx, GLenum op,
	       GLfloat value, GLint xpos, GLint ypos,
	       GLint width, GLint height );


extern void
_swrast_DrawBuffer( GLcontext *ctx, GLenum mode );


extern void
_swrast_DrawBuffers( GLcontext *ctx, GLsizei n, const GLenum *buffers );


/* Reset the stipple counter
 */
extern void
_swrast_ResetLineStipple( GLcontext *ctx );

/* These will always render the correct point/line/triangle for the
 * current state.
 *
 * For flatshaded primitives, the provoking vertex is the final one.
 */
extern void
_swrast_Point( GLcontext *ctx, const SWvertex *v );

extern void
_swrast_Line( GLcontext *ctx, const SWvertex *v0, const SWvertex *v1 );

extern void
_swrast_Triangle( GLcontext *ctx, const SWvertex *v0,
                  const SWvertex *v1, const SWvertex *v2 );

extern void
_swrast_Quad( GLcontext *ctx,
              const SWvertex *v0, const SWvertex *v1,
	      const SWvertex *v2,  const SWvertex *v3);

extern void
_swrast_flush( GLcontext *ctx );

extern void
_swrast_render_primitive( GLcontext *ctx, GLenum mode );

extern void
_swrast_render_start( GLcontext *ctx );

extern void
_swrast_render_finish( GLcontext *ctx );

/* Tell the software rasterizer about core state changes.
 */
extern void
_swrast_InvalidateState( GLcontext *ctx, GLuint new_state );

/* Configure software rasterizer to match hardware rasterizer characteristics:
 */
extern void
_swrast_allow_vertex_fog( GLcontext *ctx, GLboolean value );

extern void
_swrast_allow_pixel_fog( GLcontext *ctx, GLboolean value );

/* Debug:
 */
extern void
_swrast_print_vertex( GLcontext *ctx, const SWvertex *v );


/*
 * Imaging fallbacks (a better solution should be found, perhaps
 * moving all the imaging fallback code to a new module)
 */
extern void
_swrast_CopyConvolutionFilter2D(GLcontext *ctx, GLenum target,
				GLenum internalFormat,
				GLint x, GLint y, GLsizei width,
				GLsizei height);
extern void
_swrast_CopyConvolutionFilter1D(GLcontext *ctx, GLenum target,
				GLenum internalFormat,
				GLint x, GLint y, GLsizei width);
extern void
_swrast_CopyColorSubTable( GLcontext *ctx,GLenum target, GLsizei start,
			   GLint x, GLint y, GLsizei width);
extern void
_swrast_CopyColorTable( GLcontext *ctx,
			GLenum target, GLenum internalformat,
			GLint x, GLint y, GLsizei width);


/*
 * Texture fallbacks.  Could also live in a new module
 * with the rest of the texture store fallbacks?
 */
extern void
_swrast_copy_teximage1d(GLcontext *ctx, GLenum target, GLint level,
                        GLenum internalFormat,
                        GLint x, GLint y, GLsizei width, GLint border);

extern void
_swrast_copy_teximage2d(GLcontext *ctx, GLenum target, GLint level,
                        GLenum internalFormat,
                        GLint x, GLint y, GLsizei width, GLsizei height,
                        GLint border);


extern void
_swrast_copy_texsubimage1d(GLcontext *ctx, GLenum target, GLint level,
                           GLint xoffset, GLint x, GLint y, GLsizei width);

extern void
_swrast_copy_texsubimage2d(GLcontext *ctx,
                           GLenum target, GLint level,
                           GLint xoffset, GLint yoffset,
                           GLint x, GLint y, GLsizei width, GLsizei height);

extern void
_swrast_copy_texsubimage3d(GLcontext *ctx,
                           GLenum target, GLint level,
                           GLint xoffset, GLint yoffset, GLint zoffset,
                           GLint x, GLint y, GLsizei width, GLsizei height);


/* The driver interface for the software rasterizer.
 * Unless otherwise noted, all functions are mandatory.
 */
struct swrast_device_driver {
#if OLD_RENDERBUFFER
   void (*SetBuffer)(GLcontext *ctx, GLframebuffer *buffer, GLuint bufferBit);
   /*
    * Specifies the current color buffer for span/pixel writing/reading.
    * buffer indicates which window to write to / read from.  Normally,
    * this'll be the buffer currently bound to the context, but it doesn't
    * have to be!
    * bufferBit indicates which color buffer, exactly one of:
    *    DD_FRONT_LEFT_BIT - this buffer always exists
    *    DD_BACK_LEFT_BIT - when double buffering
    *    DD_FRONT_RIGHT_BIT - when using stereo
    *    DD_BACK_RIGHT_BIT - when using stereo and double buffering
    *    DD_AUXn_BIT - if aux buffers are implemented
    */
#endif

   /***
    *** Functions for synchronizing access to the framebuffer:
    ***/

   void (*SpanRenderStart)(GLcontext *ctx);
   void (*SpanRenderFinish)(GLcontext *ctx);
   /* OPTIONAL.
    *
    * Called before and after all rendering operations, including DrawPixels,
    * ReadPixels, Bitmap, span functions, and CopyTexImage, etc commands.
    * These are a suitable place for grabbing/releasing hardware locks.
    *
    * NOTE: The swrast triangle/line/point routines *DO NOT* call
    * these functions.  Locking in that case must be organized by the
    * driver by other mechanisms.
    */
};



#endif
