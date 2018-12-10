/* $Id: varray.h,v 1.1 1996/09/13 01:38:16 brianp Exp $ */

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
 * $Log: varray.h,v $
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifndef VARRAY_H
#define VARRAY_H


#include "types.h"


extern void gl_VertexPointer( GLcontext *ctx,
                              GLint size, GLenum type, GLsizei stride,
                              const GLvoid *ptr );


extern void gl_NormalPointer( GLcontext *ctx,
                              GLenum type, GLsizei stride, const GLvoid *ptr );


extern void gl_ColorPointer( GLcontext *ctx,
                             GLint size, GLenum type, GLsizei stride,
                             const GLvoid *ptr );


extern void gl_IndexPointer( GLcontext *ctx,
                                GLenum type, GLsizei stride,
                                const GLvoid *ptr );


extern void gl_TexCoordPointer( GLcontext *ctx,
                                GLint size, GLenum type, GLsizei stride,
                                const GLvoid *ptr );


extern void gl_EdgeFlagPointer( GLcontext *ctx,
                                GLsizei stride, const GLboolean *ptr );


extern void gl_GetPointerv( GLcontext *ctx, GLenum pname, GLvoid **params );


extern void gl_ArrayElement( GLcontext *ctx, GLint i );

extern void gl_save_ArrayElement( GLcontext *ctx, GLint i );


extern void gl_DrawArrays( GLcontext *ctx,
                           GLenum mode, GLint first, GLsizei count );

extern void gl_save_DrawArrays( GLcontext *ctx,
                                GLenum mode, GLint first, GLsizei count );


extern void gl_DrawElements( GLcontext *ctx,
                             GLenum mode, GLsizei count,
                             GLenum type, const GLvoid *indices );

extern void gl_save_DrawElements( GLcontext *ctx,
                                  GLenum mode, GLsizei count,
                                  GLenum type, const GLvoid *indices );


extern void gl_InterleavedArrays( GLcontext *ctx,
                                  GLenum format, GLsizei stride,
                                  const GLvoid *pointer );

extern void gl_save_InterleavedArrays( GLcontext *ctx,
                                       GLenum format, GLsizei stride,
                                       const GLvoid *pointer );

#endif



