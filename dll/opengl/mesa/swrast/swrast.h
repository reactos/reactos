/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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

#include "main/mtypes.h"
#include "swrast/s_chan.h"

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
 * wpos = attr[FRAG_ATTRIB_WPOS] and MUST BE THE FIRST values in the
 * vertex because of the tnl clipping code.

 * wpos[0] and [1] are the screen-coords of SWvertex.
 * wpos[2] is the z-buffer coord (if 16-bit Z buffer, in range [0,65535]).
 * wpos[3] is 1/w where w is the clip-space W coord.  This is the value
 * that clip{XYZ} were multiplied by to get ndc{XYZ}.
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
   GLfloat attrib[FRAG_ATTRIB_MAX][4];
   GLchan color[4];   /** integer color */
   GLfloat pointSize;
} SWvertex;


#define FRAG_ATTRIB_CI FRAG_ATTRIB_COL


struct swrast_device_driver;


/* These are the public-access functions exported from swrast.
 */

extern GLboolean
_swrast_CreateContext( struct gl_context *ctx );

extern void
_swrast_DestroyContext( struct gl_context *ctx );

/* Get a (non-const) reference to the device driver struct for swrast.
 */
extern struct swrast_device_driver *
_swrast_GetDeviceDriverReference( struct gl_context *ctx );

extern void
_swrast_Bitmap( struct gl_context *ctx,
		GLint px, GLint py,
		GLsizei width, GLsizei height,
		const struct gl_pixelstore_attrib *unpack,
		const GLubyte *bitmap );

extern void
_swrast_CopyPixels( struct gl_context *ctx,
		    GLint srcx, GLint srcy,
		    GLint destx, GLint desty,
		    GLsizei width, GLsizei height,
		    GLenum type );

extern GLboolean
swrast_fast_copy_pixels(struct gl_context *ctx,
			GLint srcX, GLint srcY, GLsizei width, GLsizei height,
			GLint dstX, GLint dstY, GLenum type);

extern void
_swrast_DrawPixels( struct gl_context *ctx,
		    GLint x, GLint y,
		    GLsizei width, GLsizei height,
		    GLenum format, GLenum type,
		    const struct gl_pixelstore_attrib *unpack,
		    const GLvoid *pixels );

extern void
_swrast_Clear(struct gl_context *ctx, GLbitfield buffers);



/* Reset the stipple counter
 */
extern void
_swrast_ResetLineStipple( struct gl_context *ctx );

/* These will always render the correct point/line/triangle for the
 * current state.
 *
 * For flatshaded primitives, the provoking vertex is the final one.
 */
extern void
_swrast_Point( struct gl_context *ctx, const SWvertex *v );

extern void
_swrast_Line( struct gl_context *ctx, const SWvertex *v0, const SWvertex *v1 );

extern void
_swrast_Triangle( struct gl_context *ctx, const SWvertex *v0,
                  const SWvertex *v1, const SWvertex *v2 );

extern void
_swrast_Quad( struct gl_context *ctx,
              const SWvertex *v0, const SWvertex *v1,
	      const SWvertex *v2,  const SWvertex *v3);

extern void
_swrast_flush( struct gl_context *ctx );

extern void
_swrast_render_primitive( struct gl_context *ctx, GLenum mode );

extern void
_swrast_render_start( struct gl_context *ctx );

extern void
_swrast_render_finish( struct gl_context *ctx );

extern struct gl_texture_image *
_swrast_new_texture_image( struct gl_context *ctx );

extern void
_swrast_delete_texture_image(struct gl_context *ctx,
                             struct gl_texture_image *texImage);

extern GLboolean
_swrast_alloc_texture_image_buffer(struct gl_context *ctx,
                                   struct gl_texture_image *texImage,
                                   gl_format format, GLsizei width,
                                   GLsizei height, GLsizei depth);

extern void
_swrast_init_texture_image(struct gl_texture_image *texImage, GLsizei width,
                           GLsizei height, GLsizei depth);

extern void
_swrast_free_texture_image_buffer(struct gl_context *ctx,
                                  struct gl_texture_image *texImage);

extern void
_swrast_map_teximage(struct gl_context *ctx,
		     struct gl_texture_image *texImage,
		     GLuint slice,
		     GLuint x, GLuint y, GLuint w, GLuint h,
		     GLbitfield mode,
		     GLubyte **mapOut,
		     GLint *rowStrideOut);

extern void
_swrast_unmap_teximage(struct gl_context *ctx,
		       struct gl_texture_image *texImage,
		       GLuint slice);

/* Tell the software rasterizer about core state changes.
 */
extern void
_swrast_InvalidateState( struct gl_context *ctx, GLbitfield new_state );

/* Configure software rasterizer to match hardware rasterizer characteristics:
 */
extern void
_swrast_allow_vertex_fog( struct gl_context *ctx, GLboolean value );

extern void
_swrast_allow_pixel_fog( struct gl_context *ctx, GLboolean value );

/* Debug:
 */
extern void
_swrast_print_vertex( struct gl_context *ctx, const SWvertex *v );



extern void
_swrast_eject_texture_images(struct gl_context *ctx);


extern void
_swrast_render_texture(struct gl_context *ctx,
                       struct gl_framebuffer *fb,
                       struct gl_renderbuffer_attachment *att);



extern GLboolean
_swrast_AllocTextureStorage(struct gl_context *ctx,
                            struct gl_texture_object *texObj,
                            GLsizei levels, GLsizei width,
                            GLsizei height, GLsizei depth);


/**
 * The driver interface for the software rasterizer.
 * XXX this may go away.
 * We may move these functions to ctx->Driver.RenderStart, RenderEnd.
 */
struct swrast_device_driver {
   /*
    * These are called before and after accessing renderbuffers during
    * software rasterization.
    *
    * These are a suitable place for grabbing/releasing hardware locks.
    *
    * NOTE: The swrast triangle/line/point routines *DO NOT* call
    * these functions.  Locking in that case must be organized by the
    * driver by other mechanisms.
    */
   void (*SpanRenderStart)(struct gl_context *ctx);
   void (*SpanRenderFinish)(struct gl_context *ctx);
};



#endif
