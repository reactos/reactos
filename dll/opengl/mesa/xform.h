/* $Id: xform.h,v 1.4 1997/04/02 03:15:02 brianp Exp $ */

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
 * $Log: xform.h,v $
 * Revision 1.4  1997/04/02 03:15:02  brianp
 * removed gl_xform_texcoords_4fv()
 *
 * Revision 1.3  1996/11/08 02:20:39  brianp
 * added gl_xform_texcoords_4fv()
 *
 * Revision 1.2  1996/11/04 01:39:46  brianp
 * removed MAP_X(), MAP_Y() and MAP_Z() macros
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifndef XFORM_H
#define XFORM_H


#include "types.h"


/*
 * Transform a point (column vector) by a matrix:   Q = M * P
 */
#define TRANSFORM_POINT( Q, M, P )					\
   Q[0] = M[0] * P[0] + M[4] * P[1] + M[8] *  P[2] + M[12] * P[3];	\
   Q[1] = M[1] * P[0] + M[5] * P[1] + M[9] *  P[2] + M[13] * P[3];	\
   Q[2] = M[2] * P[0] + M[6] * P[1] + M[10] * P[2] + M[14] * P[3];	\
   Q[3] = M[3] * P[0] + M[7] * P[1] + M[11] * P[2] + M[15] * P[3];


/*
 * Transform a normal (row vector) by a matrix:  [NX NY NZ] = N * MAT
 */
#define TRANSFORM_NORMAL( NX, NY, NZ, N, MAT )		\
   NX = N[0] * MAT[0] + N[1] * MAT[1] + N[2] * MAT[2];	\
   NY = N[0] * MAT[4] + N[1] * MAT[5] + N[2] * MAT[6];	\
   NZ = N[0] * MAT[8] + N[1] * MAT[9] + N[2] * MAT[10];	\



extern void gl_xform_points_4fv( GLuint n, GLfloat q[][4], const GLfloat m[16],
                                 GLfloat p[][4] );


extern void gl_xform_points_3fv( GLuint n, GLfloat q[][4], const GLfloat m[16],
                                 GLfloat p[][3] );


extern void gl_xform_normals_3fv( GLuint n, GLfloat v[][3],
                                  const GLfloat m[16],
                                  GLfloat u[][3], GLboolean normalize );


extern void gl_transform_vector( GLfloat u[4],
				 const GLfloat v[4], const GLfloat m[16] );


#endif
