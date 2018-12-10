/* $Id: points.h,v 1.2 1997/10/29 01:29:09 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.5
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
 * $Log: points.h,v $
 * Revision 1.2  1997/10/29 01:29:09  brianp
 * added GL_EXT_point_parameters extension from Daniel Barrero
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifndef POINTS_H
#define POINTS_H


#include "types.h"


extern void gl_PointSize( GLcontext *ctx, GLfloat size );

extern void gl_set_point_function( GLcontext *ctx );

#endif
