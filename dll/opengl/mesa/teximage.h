/* $Id: teximage.h,v 1.4 1997/11/02 20:20:47 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.5
 * Copyright (C) 1995-1997  Brian Paul
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
 * $Log: teximage.h,v $
 * Revision 1.4  1997/11/02 20:20:47  brianp
 * removed gl_unpack_texsubimage3D()
 *
 * Revision 1.3  1997/02/09 18:53:05  brianp
 * added GL_EXT_texture3D support
 *
 * Revision 1.2  1996/11/07 04:13:24  brianp
 * all new texture image handling, now pixel scale, bias, mapping work
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifndef TEXIMAGE_H
#define TEXIMAGE_H


#include "types.h"


/*** Internal functions ***/


extern struct gl_texture_image *gl_alloc_texture_image( void );


extern void gl_free_texture_image( struct gl_texture_image *teximage );


extern struct gl_image *
gl_unpack_texsubimage( GLcontext *ctx, GLint width, GLint height,
                       GLenum format, GLenum type, const GLvoid *pixels );


extern struct gl_texture_image *
gl_unpack_texture( GLcontext *ctx,
                   GLint dimensions,
                   GLenum target,
                   GLint level,
                   GLint internalformat,
                   GLsizei width, GLsizei height,
                   GLint border,
                   GLenum format, GLenum type,
                   const GLvoid *pixels );

extern struct gl_texture_image *
gl_unpack_texture3D( GLcontext *ctx,
                     GLint dimensions,
                     GLenum target,
                     GLint level,
                     GLint internalformat,
                     GLsizei width, GLsizei height, GLsizei depth,
                     GLint border,
                     GLenum format, GLenum type,
                     const GLvoid *pixels );


extern void gl_tex_image_1D( GLcontext *ctx,
                             GLenum target, GLint level, GLint internalformat,
                             GLsizei width, GLint border, GLenum format,
                             GLenum type, const GLvoid *pixels );


extern void gl_tex_image_2D( GLcontext *ctx,
                             GLenum target, GLint level, GLint internalformat,
                             GLsizei width, GLint height, GLint border,
                             GLenum format, GLenum type,
                             const GLvoid *pixels );

extern void gl_tex_image_3D( GLcontext *ctx,
                             GLenum target, GLint level, GLint internalformat,
                             GLsizei width, GLint height, GLint depth,
                             GLint border,
                             GLenum format, GLenum type,
                             const GLvoid *pixels );


/*** API entry points ***/


extern void gl_TexImage1D( GLcontext *ctx,
                           GLenum target, GLint level, GLint internalformat,
                           GLsizei width, GLint border, GLenum format,
                           GLenum type, struct gl_image *teximage );


extern void gl_TexImage2D( GLcontext *ctx,
                           GLenum target, GLint level, GLint internalformat,
                           GLsizei width, GLsizei height, GLint border,
                           GLenum format, GLenum type,
                           struct gl_image *teximage );

extern void gl_GetTexImage( GLcontext *ctx, GLenum target, GLint level,
                            GLenum format, GLenum type, GLvoid *pixels );



extern void gl_TexSubImage1D( GLcontext *ctx,
                              GLenum target, GLint level, GLint xoffset,
                              GLsizei width, GLenum format, GLenum type,
                              struct gl_image *image );


extern void gl_TexSubImage2D( GLcontext *ctx,
                              GLenum target, GLint level,
                              GLint xoffset, GLint yoffset,
                              GLsizei width, GLsizei height,
                              GLenum format, GLenum type,
                              struct gl_image *image );


extern void gl_CopyTexImage1D( GLcontext *ctx,
                               GLenum target, GLint level,
                               GLenum internalformat,
                               GLint x, GLint y,
                               GLsizei width, GLint border );


extern void gl_CopyTexImage2D( GLcontext *ctx,
                               GLenum target, GLint level,
                               GLenum internalformat, GLint x, GLint y,
                               GLsizei width, GLsizei height,
                               GLint border );


extern void gl_CopyTexSubImage1D( GLcontext *ctx,
                                  GLenum target, GLint level,
                                  GLint xoffset, GLint x, GLint y,
                                  GLsizei width );


extern void gl_CopyTexSubImage2D( GLcontext *ctx,
                                  GLenum target, GLint level,
                                  GLint xoffset, GLint yoffset,
                                  GLint x, GLint y,
                                  GLsizei width, GLsizei height );

#endif

