/* $Id: fixed.h,v 1.2 1996/10/01 03:31:04 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.0
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
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifndef FIXED_H
#define FIXED_H


typedef int GLfixed;

#define FIXED_ONE       0x00000800
#define FIXED_HALF      0x00000400
#define FIXED_FRAC_MASK 0x000007FF
#define FIXED_INT_MASK  (~FIXED_FRAC_MASK)
#define FIXED_EPSILON   1
#define FIXED_SCALE     2048.0f
#define FIXED_SHIFT     11
#define FloatToFixed(X) ((GLfixed) ((X) * FIXED_SCALE))
#define IntToFixed(I)   ((I) << FIXED_SHIFT)
#define FixedToInt(X)   ((X) >> FIXED_SHIFT)
#define FixedToUns(X)   (((unsigned int)(X)) >> 11)
#define FixedCeil(X)    (((X) + FIXED_ONE - FIXED_EPSILON) & FIXED_INT_MASK)
#define FixedFloor(X)   ((X) & FIXED_INT_MASK)
/* 0.00048828125 = 1/FIXED_SCALE */
#define FixedToFloat(X) ((X) * 0.00048828125f)
#define PosFloatToFixed(X)      FloatToFixed(X)
#define SignedFloatToFixed(X)   FloatToFixed(X)


#endif
