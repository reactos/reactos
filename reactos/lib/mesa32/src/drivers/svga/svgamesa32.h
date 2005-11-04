/* $Id: svgamesa32.h,v 1.6 2002/11/11 18:42:42 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  5.0
 * Copyright (C) 1995-2002  Brian Paul
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
 * SVGA driver for Mesa.
 * Original author:  Brian Paul
 * Additional authors:  Slawomir Szczyrba <steev@hot.pl>  (Mesa 3.2)
 */


#ifndef SVGA_MESA_32_H
#define SVGA_MESA_32_H

extern void __clear_color32( GLcontext *ctx, const GLfloat color[4] );
extern void __clear32( GLcontext *ctx, GLbitfield mask, GLboolean all, GLint x, GLint y, GLint width, GLint height );
extern void __write_rgba_span32( const GLcontext *ctx, GLuint n, GLint x, GLint y, const GLubyte rgba[][4], const GLubyte mask[] );
extern void __write_mono_rgba_span32( const GLcontext *ctx, GLuint n, GLint x, GLint y, const GLchan color[4], const GLubyte mask[]);
extern void __read_rgba_span32( const GLcontext *ctx, GLuint n, GLint x, GLint y, GLubyte rgba[][4] );
extern void __write_rgba_pixels32( const GLcontext *ctx, GLuint n, const GLint x[], const GLint y[], const GLubyte rgba[][4], const GLubyte mask[] );
extern void __write_mono_rgba_pixels32( const GLcontext *ctx, GLuint n, const GLint x[], const GLint y[], const GLchan color[4], const GLubyte mask[] );
extern void __read_rgba_pixels32( const GLcontext *ctx, GLuint n, const GLint x[], const GLint y[], GLubyte rgba[][4], const GLubyte mask[] );

#endif /* SVGA_MESA_32_H */

