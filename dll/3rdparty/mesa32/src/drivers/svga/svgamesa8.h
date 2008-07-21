/* $Id: svgamesa8.h,v 1.4 2001/02/06 00:03:48 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.2
 * Copyright (C) 1995-2000  Brian Paul
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


#ifndef SVGA_MESA_8_H
#define SVGA_MESA_8_H

extern void __clear_index8( GLcontext *ctx, GLuint index );
extern void __clear8( GLcontext *ctx, GLbitfield mask, GLboolean all, GLint x, GLint y, GLint width, GLint height );
extern void __write_ci32_span8( const GLcontext *ctx, GLuint n, GLint x, GLint y, const GLuint index[], const GLubyte mask[] );
extern void __write_ci8_span8( const GLcontext *ctx, GLuint n, GLint x, GLint y, const GLubyte index[], const GLubyte mask[] );
extern void __write_mono_ci_span8( const GLcontext *ctx, GLuint n, GLint x, GLint y, GLuint colorIndex, const GLubyte mask[] );
extern void __read_ci32_span8( const GLcontext *ctx, GLuint n, GLint x, GLint y, GLuint index[]);
extern void __write_ci32_pixels8( const GLcontext *ctx, GLuint n, const GLint x[], const GLint y[], const GLuint index[], const GLubyte mask[] );
extern void __write_mono_ci_pixels8( const GLcontext *ctx, GLuint n, const GLint x[], const GLint y[], GLuint colorIndex, const GLubyte mask[] );
extern void __read_ci32_pixels8( const GLcontext *ctx, GLuint n, const GLint x[], const GLint y[], GLuint index[], const GLubyte mask[] );

#endif /* SVGA_MESA_15_H */
