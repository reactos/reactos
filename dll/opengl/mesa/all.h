/* $Id: all.h,v 1.6 1997/12/15 03:40:05 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.6
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
 * $Log: all.h,v $
 * Revision 1.6  1997/12/15 03:40:05  brianp
 * added asm-386.h file
 *
 * Revision 1.5  1997/11/20 00:23:21  brianp
 * added rect.h
 *
 * Revision 1.4  1997/09/27 00:16:04  brianp
 * added colortab.h
 *
 * Revision 1.3  1997/08/22 01:42:48  brianp
 * added api.h and hash.h
 *
 * Revision 1.2  1997/05/28 03:22:27  brianp
 * added a few more headers
 *
 * Revision 1.1  1997/04/02 03:49:26  brianp
 * Initial revision
 *
 */


/* The purpose of this file is to collect all the header files that Mesa
 * uses into a single header so that we can get new compilers that support
 * pre-compiled headers to compile much faster.
 * All we do is list all the internal headers used by Mesa in this one
 * main header file, and most compilers will pre-compile all these headers
 * and use them over and over again for each source module. This makes a
 * big difference for Win32 support, because the <windows.h> headers take
 * a *long* time to compile.
 */


#ifndef SRC_ALL_H
#define SRC_ALL_H


#ifndef PC_HEADER
  This is an error.  all.h should be included only if PC_HEADER is defined.
#endif


#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include "GL/gl.h"
#include "GL/osmesa.h"
#include "accum.h"
#include "alpha.h"
#include "alphabuf.h"
#include "api.h"
#include "asm-386.h"
#include "attrib.h"
#include "bitmap.h"
#include "blend.h"
#include "clip.h"
#include "colortab.h"
#include "context.h"
#include "config.h"
#include "copypix.h"
#include "dd.h"
#include "depth.h"
#include "dlist.h"
#include "drawpix.h"
#include "enable.h"
#include "eval.h"
#include "feedback.h"
#include "fixed.h"
#include "fog.h"
#include "get.h"
#include "hash.h"
#include "image.h"
#include "light.h"
#include "lines.h"
#include "logic.h"
#include "macros.h"
#include "masking.h"
#include "matrix.h"
#include "misc.h"
#include "mmath.h"
#include "pb.h"
#include "pixel.h"
#include "pointers.h"
#include "points.h"
#include "polygon.h"
#include "rastpos.h"
#include "readpix.h"
#include "rect.h"
#include "scissor.h"
#include "shade.h"
#include "span.h"
#include "stencil.h"
#include "teximage.h"
#include "texobj.h"
#include "texstate.h"
#include "texture.h"
#include "triangle.h"
#include "types.h"
#include "varray.h"
#include "vb.h"
#include "vbfill.h"
#include "vbrender.h"
#include "vbxform.h"
#include "winpos.h"
#include "xform.h"


#endif /*SRC_ALL_H*/
