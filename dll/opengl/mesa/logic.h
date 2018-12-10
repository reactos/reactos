/* $Id: logic.h,v 1.2 1997/01/28 22:16:31 brianp Exp $ */

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
 * $Log: logic.h,v $
 * Revision 1.2  1997/01/28 22:16:31  brianp
 * added gl_logicop_rgba_span() and gl_logicop_rgba_pixels()
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifndef LOGIC_H
#define LOGIC_H


#include "types.h"


extern void gl_LogicOp( GLcontext *ctx, GLenum opcode );


extern void gl_logicop_ci_span( GLcontext *ctx,
                                GLuint n, GLint x, GLint y, GLuint index[],
                                GLubyte mask[] );


extern void gl_logicop_ci_pixels( GLcontext *ctx,
                                  GLuint n, const GLint x[], const GLint y[],
                                  GLuint index[], GLubyte mask[] );


extern void gl_logicop_rgba_span( GLcontext *ctx,
                                  GLuint n, GLint x, GLint y,
                                  GLubyte red[], GLubyte green[],
                                  GLubyte blue[], GLubyte alpha[],
                                  GLubyte mask[] );


extern void gl_logicop_rgba_pixels( GLcontext *ctx,
                                    GLuint n, const GLint x[], const GLint y[],
                                    GLubyte red[], GLubyte green[],
                                    GLubyte blue[], GLubyte alpha[],
                                    GLubyte mask[] );


#endif
