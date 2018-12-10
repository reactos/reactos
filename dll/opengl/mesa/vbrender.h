/* $Id: vbrender.h,v 1.1 1997/04/02 03:14:14 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.3
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
 * $Log: vbrender.h,v $
 * Revision 1.1  1997/04/02 03:14:14  brianp
 * Initial revision
 *
 */


#ifndef VBRENDER_H
#define VBRENDER_H


#include "types.h"


extern void gl_render_vb( GLcontext *ctx, GLboolean alldone );

extern void gl_reset_vb( GLcontext *ctx, GLboolean allDone );


#endif
