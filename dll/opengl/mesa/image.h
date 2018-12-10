/* $Id: image.h,v 1.5 1997/09/27 00:15:39 brianp Exp $ */

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
 * $Log: image.h,v $
 * Revision 1.5  1997/09/27 00:15:39  brianp
 * changed parameters to gl_unpack_image()
 *
 * Revision 1.4  1997/02/09 20:05:03  brianp
 * new arguments for gl_pixel_addr_in_image()
 *
 * Revision 1.3  1997/02/09 18:52:53  brianp
 * added GL_EXT_texture3D support
 *
 * Revision 1.2  1996/11/06 04:22:23  brianp
 * changed gl_unpack_image() components argument to srcFormat
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifndef IMAGE_H
#define IMAGE_H


#include "types.h"


extern void gl_flip_bytes( GLubyte *p, GLuint n );


extern void gl_swap2( GLushort *p, GLuint n );

extern void gl_swap4( GLuint *p, GLuint n );


extern GLint gl_sizeof_type( GLenum type );


extern GLint gl_components_in_format( GLenum format );


extern GLvoid *gl_pixel_addr_in_image( struct gl_pixelstore_attrib *packing,
                                const GLvoid *image, GLsizei width,
                                GLsizei height, GLenum format, GLenum type,
                                GLint row);


extern struct gl_image *gl_unpack_image( GLcontext *ctx,
                                  GLint width, GLint height,
                                  GLenum srcFormat, GLenum srcType,
                                  const GLvoid *pixels );


extern void gl_free_image( struct gl_image *image );


#endif
