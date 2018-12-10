/* $Id: fog.h,v 1.2 1997/06/20 04:14:47 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.3
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
 * $Log: fog.h,v $
 * Revision 1.2  1997/06/20 04:14:47  brianp
 * changed color components from GLfixed to GLubyte
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */



#ifndef FOG_H
#define FOG_H


#include "types.h"


extern void gl_Fogfv( GLcontext *ctx, GLenum pname, const GLfloat *params );


extern void gl_fog_color_vertices( GLcontext *ctx, GLuint n,
                                   GLfloat v[][4], GLubyte color[][4] );

extern void gl_fog_index_vertices( GLcontext *ctx, GLuint n,
                                   GLfloat v[][4], GLuint indx[] );


extern void gl_fog_color_pixels( GLcontext *ctx,
                                 GLuint n, const GLdepth z[],
				 GLubyte red[], GLubyte green[],
				 GLubyte blue[], GLubyte alpha[] );

extern void gl_fog_index_pixels( GLcontext *ctx,
                                 GLuint n, const GLdepth z[], GLuint indx[] );


#endif
