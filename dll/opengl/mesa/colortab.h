/* $Id: colortab.h,v 1.1 1997/09/27 00:12:46 brianp Exp $ */

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
 * $Log: colortab.h,v $
 * Revision 1.1  1997/09/27 00:12:46  brianp
 * Initial revision
 *
 */


#ifndef COLORTAB_H
#define COLORTAB_H


#include "types.h"

#define GL_COLOR_TABLE_FORMAT_EXT          0x80D8
#define GL_COLOR_TABLE_WIDTH_EXT           0x80D9
#define GL_COLOR_TABLE_RED_SIZE_EXT        0x80DA
#define GL_COLOR_TABLE_GREEN_SIZE_EXT      0x80DB
#define GL_COLOR_TABLE_BLUE_SIZE_EXT       0x80DC
#define GL_COLOR_TABLE_ALPHA_SIZE_EXT      0x80DD
#define GL_COLOR_TABLE_LUMINANCE_SIZE_EXT  0x80DE
#define GL_COLOR_TABLE_INTENSITY_SIZE_EXT  0x80DF


extern void gl_ColorTable( GLcontext *ctx, GLenum target,
                           GLenum internalformat,
                           struct gl_image *table );

extern void gl_ColorSubTable( GLcontext *ctx, GLenum target,
                              GLsizei start, struct gl_image *data );

extern void gl_GetColorTable( GLcontext *ctx, GLenum target, GLenum format,
                              GLenum type, GLvoid *table );

extern void gl_GetColorTableParameterfv( GLcontext *ctx, GLenum target,
                                         GLenum pname, GLfloat *params );

extern void gl_GetColorTableParameteriv( GLcontext *ctx, GLenum target,
                                         GLenum pname, GLint *params );


#endif
