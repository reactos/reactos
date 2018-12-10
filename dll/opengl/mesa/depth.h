/* $Id: depth.h,v 1.2 1996/09/15 14:19:16 brianp Exp $ */

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
 * $Log: depth.h,v $
 * Revision 1.2  1996/09/15 14:19:16  brianp
 * now use GLframebuffer and GLvisual
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifndef DEPTH_H
#define DEPTH_H


#include "types.h"


/*
 * Return the address of the Z-buffer value for window coordinate (x,y):
 */
#define Z_ADDRESS( CTX, X, Y )  \
            ((CTX)->Buffer->Depth + (CTX)->Buffer->Width * (Y) + (X))




extern GLuint
gl_depth_test_span_generic( GLcontext* ctx, GLuint n, GLint x, GLint y,
                            const GLdepth z[], GLubyte mask[] );

extern GLuint
gl_depth_test_span_less( GLcontext* ctx, GLuint n, GLint x, GLint y,
                         const GLdepth z[], GLubyte mask[] );

extern GLuint
gl_depth_test_span_greater( GLcontext* ctx, GLuint n, GLint x, GLint y,
                            const GLdepth z[], GLubyte mask[] );



extern void
gl_depth_test_pixels_generic( GLcontext* ctx,
                              GLuint n, const GLint x[], const GLint y[],
                              const GLdepth z[], GLubyte mask[] );

extern void
gl_depth_test_pixels_less( GLcontext* ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           const GLdepth z[], GLubyte mask[] );

extern void
gl_depth_test_pixels_greater( GLcontext* ctx,
                              GLuint n, const GLint x[], const GLint y[],
                              const GLdepth z[], GLubyte mask[] );


extern void gl_read_depth_span_float( GLcontext* ctx,
                                      GLuint n, GLint x, GLint y,
                                      GLfloat depth[] );


extern void gl_read_depth_span_int( GLcontext* ctx, GLuint n, GLint x, GLint y,
                                    GLdepth depth[] );


extern void gl_alloc_depth_buffer( GLcontext* ctx );


extern void gl_clear_depth_buffer( GLcontext* ctx );


extern void gl_ClearDepth( GLcontext* ctx, GLclampd depth );
extern void gl_DepthFunc( GLcontext* ctx, GLenum func );
extern void gl_DepthMask( GLcontext* ctx, GLboolean flag );
extern void gl_DepthRange( GLcontext* ctx, GLclampd nearval, GLclampd farval );

#endif
