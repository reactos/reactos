/*
 * Mesa 3-D graphics library
 * Version:  6.1
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

/**
 * \file debug.h
 * Debugging functions.
 * 
 * \if subset
 * (No-op)
 *
 * \endif
 */


#ifndef _DEBUG_H
#define _DEBUG_H

#include "glheader.h"
#include "mfeatures.h"

struct gl_context;
struct gl_texture_image;

#if _HAVE_FULL_GL

extern void _mesa_print_tri_caps( const char *name, GLuint flags );
extern void _mesa_print_enable_flags( const char *msg, GLuint flags );
extern void _mesa_print_state( const char *msg, GLuint state );
extern void _mesa_print_info( void );
extern void _mesa_init_debug( struct gl_context *ctx );

#else

/** No-op */
#define _mesa_print_state( m, s ) ((void)0)

/** No-op */
#define _mesa_print_info() ((void)0)

/** No-op */
#define _mesa_init_debug( c ) ((void)0)

#endif

extern void
_mesa_write_renderbuffer_image(const struct gl_renderbuffer *rb);

extern void
_mesa_dump_texture(GLuint texture, GLuint writeImages);

extern void
_mesa_dump_textures(GLuint writeImages);

extern void
_mesa_dump_renderbuffers(GLboolean writeImages);

extern void
_mesa_dump_color_buffer(const char *filename);

extern void
_mesa_dump_depth_buffer(const char *filename);

extern void
_mesa_dump_stencil_buffer(const char *filename);

extern void
_mesa_dump_image(const char *filename, const void *image, GLuint w, GLuint h,
                 GLenum format, GLenum type);

extern void
_mesa_print_texture(struct gl_context *ctx, struct gl_texture_image *img);

#endif
