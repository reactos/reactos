/* $Id: span.h,v 1.2 1997/02/09 18:43:34 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.2
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
 * $Log: span.h,v $
 * Revision 1.2  1997/02/09 18:43:34  brianp
 * added GL_EXT_texture3D support
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifndef SPAN_H
#define SPAN_H


#include "types.h"


extern void gl_write_index_span( GLcontext *ctx,
                                 GLuint n, GLint x, GLint y, GLdepth z[],
				 GLuint index[], GLenum primitive );


extern void gl_write_monoindex_span( GLcontext *ctx,
                                     GLuint n, GLint x, GLint y, GLdepth z[],
				     GLuint index, GLenum primitive );


extern void gl_write_color_span( GLcontext *ctx,
                                 GLuint n, GLint x, GLint y, GLdepth z[],
				 GLubyte red[], GLubyte green[],
				 GLubyte blue[], GLubyte alpha[],
				 GLenum primitive );


extern void gl_write_monocolor_span( GLcontext *ctx,
                                     GLuint n, GLint x, GLint y, GLdepth z[],
				     GLint r, GLint g, GLint b, GLint a,
                                     GLenum primitive );


extern void gl_write_texture_span( GLcontext *ctx,
                                   GLuint n, GLint x, GLint y, GLdepth z[],
				   GLfloat s[], GLfloat t[], GLfloat u[],
                                   GLfloat lambda[],
				   GLubyte red[], GLubyte green[],
				   GLubyte blue[], GLubyte alpha[],
				   GLenum primitive );


extern void gl_read_color_span( GLcontext *ctx,
                                GLuint n, GLint x, GLint y,
			        GLubyte red[], GLubyte green[],
			        GLubyte blue[], GLubyte alpha[] );


extern void gl_read_index_span( GLcontext *ctx,
                                GLuint n, GLint x, GLint y, GLuint indx[] );


#endif
