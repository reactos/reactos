/* $Id: pixel.h,v 1.1 1996/09/13 01:38:16 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.0
 * Copyright (C) 1995-1996  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * $Log: pixel.h,v $
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifndef PIXEL_H
#define PIXEL_H


#include "types.h"


extern void gl_GetPixelMapfv( GLcontext *ctx, GLenum map, GLfloat *values );

extern void gl_GetPixelMapuiv( GLcontext *ctx, GLenum map, GLuint *values );

extern void gl_GetPixelMapusv( GLcontext *ctx, GLenum map, GLushort *values );


extern void gl_PixelMapfv( GLcontext *ctx,
                           GLenum map, GLint mapsize, const GLfloat *values );

extern void gl_PixelStorei( GLcontext *ctx, GLenum pname, GLint param );

extern void gl_PixelTransferf( GLcontext *ctx, GLenum pname, GLfloat param );

extern void gl_PixelZoom( GLcontext *ctx, GLfloat xfactor, GLfloat yfactor );


extern GLvoid *gl_unpack_pixels( GLcontext *ctx,
                                 GLsizei width, GLsizei height,
                                 GLenum format, GLenum type,
                                 const GLvoid *pixels );


extern void
gl_write_zoomed_color_span( GLcontext *ctx,
                            GLuint n, GLint x, GLint y, const GLdepth z[],
                            const GLubyte red[], const GLubyte green[],
                            const GLubyte blue[], const GLubyte alpha[],
                            GLint y0 );


extern void
gl_write_zoomed_index_span( GLcontext *ctx,
                            GLuint n, GLint x, GLint y, const GLdepth z[],
                            const GLuint indexes[], GLint y0 );


extern void
gl_write_zoomed_stencil_span( GLcontext *ctx,
                              GLuint n, GLint x, GLint y,
                              const GLubyte stencil[], GLint y0 );


#endif

