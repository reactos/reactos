/* $Id: xform.c,v 1.10 1997/10/30 06:00:06 brianp Exp $ */

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
 * $Log: xform.c,v $
 * Revision 1.10  1997/10/30 06:00:06  brianp
 * added Intel X86 assembly optimzations (Josh Vanderhoof)
 *
 * Revision 1.9  1997/07/24 01:25:54  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.8  1997/05/28 03:27:03  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.7  1997/05/01 01:40:51  brianp
 * replaced sqrt() with GL_SQRT()
 *
 * Revision 1.6  1997/04/02 03:15:02  brianp
 * removed gl_xform_texcoords_4fv()
 *
 * Revision 1.5  1997/01/03 23:54:17  brianp
 * changed length threshold in gl_xform_normals_3fv() to 1E-30 per Jeroen
 *
 * Revision 1.4  1996/11/09 01:50:49  brianp
 * relaxed the minimum normal threshold in gl_xform_normals_3fv()
 *
 * Revision 1.3  1996/11/08 02:20:39  brianp
 * added gl_xform_texcoords_4fv()
 *
 * Revision 1.2  1996/11/05 01:38:50  brianp
 * fixed some comments
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


/*
 * Matrix/vertex/vector transformation stuff
 *
 *
 * NOTES:
 * 1. 4x4 transformation matrices are stored in memory in column major order.
 * 2. Points/vertices are to be thought of as column vectors.
 * 3. Transformation of a point p by a matrix M is: p' = M * p
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <math.h>
#include "mmath.h"
#include "types.h"
#include "xform.h"
#endif



/*
 * Apply a transformation matrix to an array of [X Y Z W] coordinates:
 *   for i in 0 to n-1 do   q[i] = m * p[i]
 * where p[i] and q[i] are 4-element column vectors and m is a 16-element
 * transformation matrix.
 */
void gl_xform_points_4fv( GLuint n, GLfloat q[][4], const GLfloat m[16],
                          GLfloat p[][4] )
{
   /* This function has been carefully crafted to maximize register usage
    * and use loop unrolling with IRIX 5.3's cc.  Hopefully other compilers
    * will like this code too.
    */
   {
      GLuint i;
      GLfloat m0 = m[0],  m4 = m[4],  m8 = m[8],  m12 = m[12];
      GLfloat m1 = m[1],  m5 = m[5],  m9 = m[9],  m13 = m[13];
      if (m12==0.0F && m13==0.0F) {
         /* common case */
         for (i=0;i<n;i++) {
            GLfloat p0 = p[i][0], p1 = p[i][1], p2 = p[i][2];
            q[i][0] = m0 * p0 + m4  * p1 + m8 * p2;
            q[i][1] = m1 * p0 + m5  * p1 + m9 * p2;
         }
      }
      else {
         /* general case */
         for (i=0;i<n;i++) {
            GLfloat p0 = p[i][0], p1 = p[i][1], p2 = p[i][2], p3 = p[i][3];
            q[i][0] = m0 * p0 + m4  * p1 + m8 * p2 + m12 * p3;
            q[i][1] = m1 * p0 + m5  * p1 + m9 * p2 + m13 * p3;
         }
      }
   }
   {
      GLuint i;
      GLfloat m2 = m[2],  m6 = m[6],  m10 = m[10],  m14 = m[14];
      GLfloat m3 = m[3],  m7 = m[7],  m11 = m[11],  m15 = m[15];
      if (m3==0.0F && m7==0.0F && m11==0.0F && m15==1.0F) {
         /* common case */
         for (i=0;i<n;i++) {
            GLfloat p0 = p[i][0], p1 = p[i][1], p2 = p[i][2], p3 = p[i][3];
            q[i][2] = m2 * p0 + m6 * p1 + m10 * p2 + m14 * p3;
            q[i][3] = p3;
         }
      }
      else {
         /* general case */
         for (i=0;i<n;i++) {
            GLfloat p0 = p[i][0], p1 = p[i][1], p2 = p[i][2], p3 = p[i][3];
            q[i][2] = m2 * p0 + m6 * p1 + m10 * p2 + m14 * p3;
            q[i][3] = m3 * p0 + m7 * p1 + m11 * p2 + m15 * p3;
         }
      }
   }
}



/*
 * Apply a transformation matrix to an array of [X Y Z] coordinates:
 *   for i in 0 to n-1 do   q[i] = m * p[i]
 */
void gl_xform_points_3fv( GLuint n, GLfloat q[][4], const GLfloat m[16],
                          GLfloat p[][3] )
{
   /* This function has been carefully crafted to maximize register usage
    * and use loop unrolling with IRIX 5.3's cc.  Hopefully other compilers
    * will like this code too.
    */
   {
      GLuint i;
      GLfloat m0 = m[0],  m4 = m[4],  m8 = m[8],  m12 = m[12];
      GLfloat m1 = m[1],  m5 = m[5],  m9 = m[9],  m13 = m[13];
      for (i=0;i<n;i++) {
         GLfloat p0 = p[i][0], p1 = p[i][1], p2 = p[i][2];
         q[i][0] = m0 * p0 + m4  * p1 + m8 * p2 + m12;
         q[i][1] = m1 * p0 + m5  * p1 + m9 * p2 + m13;
      }
   }
   {
      GLuint i;
      GLfloat m2 = m[2],  m6 = m[6],  m10 = m[10],  m14 = m[14];
      GLfloat m3 = m[3],  m7 = m[7],  m11 = m[11],  m15 = m[15];
      if (m3==0.0F && m7==0.0F && m11==0.0F && m15==1.0F) {
         /* common case */
         for (i=0;i<n;i++) {
            GLfloat p0 = p[i][0], p1 = p[i][1], p2 = p[i][2];
            q[i][2] = m2 * p0 + m6 * p1 + m10 * p2 + m14;
            q[i][3] = 1.0F;
         }
      }
      else {
         /* general case */
         for (i=0;i<n;i++) {
            GLfloat p0 = p[i][0], p1 = p[i][1], p2 = p[i][2];
            q[i][2] = m2 * p0 + m6 * p1 + m10 * p2 + m14;
            q[i][3] = m3 * p0 + m7 * p1 + m11 * p2 + m15;
         }
      }
   }
}



#ifndef USE_ASM
/*
 * Apply a transformation matrix to an array of normal vectors:
 *   for i in 0 to n-1 do  v[i] = u[i] * m
 * where u[i] and v[i] are 3-element row vectors and m is a 16-element
 * transformation matrix.
 * If the normalize flag is true the normals will be scaled to length 1.
 */
void gl_xform_normals_3fv( GLuint n, GLfloat v[][3], const GLfloat m[16],
                           GLfloat u[][3], GLboolean normalize )
{
   if (normalize) {
      /* Transform normals and scale to unit length */
      GLuint i;
      GLfloat m0 = m[0],  m4 = m[4],  m8 = m[8];
      GLfloat m1 = m[1],  m5 = m[5],  m9 = m[9];
      GLfloat m2 = m[2],  m6 = m[6],  m10 = m[10];
      for (i=0;i<n;i++) {
         GLdouble tx, ty, tz;
         {
            GLfloat ux = u[i][0],  uy = u[i][1],  uz = u[i][2];
            tx = ux * m0 + uy * m1 + uz * m2;
            ty = ux * m4 + uy * m5 + uz * m6;
            tz = ux * m8 + uy * m9 + uz * m10;
         }
         {
            GLdouble len, scale;
            len = GL_SQRT( tx*tx + ty*ty + tz*tz );
            scale = (len>1E-30) ? (1.0 / len) : 1.0;
            v[i][0] = tx * scale;
            v[i][1] = ty * scale;
            v[i][2] = tz * scale;
         }
      }
   }
   else {
      /* Just transform normals, don't scale */
      GLuint i;
      GLfloat m0 = m[0],  m4 = m[4],  m8 = m[8];
      GLfloat m1 = m[1],  m5 = m[5],  m9 = m[9];
      GLfloat m2 = m[2],  m6 = m[6],  m10 = m[10];
      for (i=0;i<n;i++) {
         GLfloat ux = u[i][0],  uy = u[i][1],  uz = u[i][2];
         v[i][0] = ux * m0 + uy * m1 + uz * m2;
         v[i][1] = ux * m4 + uy * m5 + uz * m6;
         v[i][2] = ux * m8 + uy * m9 + uz * m10;
      }
   }
}
#endif


/*
 * Transform a 4-element row vector (1x4 matrix) by a 4x4 matrix.  This
 * function is used for transforming clipping plane equations and spotlight
 * directions.
 * Mathematically,  u = v * m.
 * Input:  v - input vector
 *         m - transformation matrix
 * Output:  u - transformed vector
 */
void gl_transform_vector( GLfloat u[4], const GLfloat v[4], const GLfloat m[16] )
{
   GLfloat v0=v[0], v1=v[1], v2=v[2], v3=v[3];
#define M(row,col)  m[col*4+row]
   u[0] = v0 * M(0,0) + v1 * M(1,0) + v2 * M(2,0) + v3 * M(3,0);
   u[1] = v0 * M(0,1) + v1 * M(1,1) + v2 * M(2,1) + v3 * M(3,1);
   u[2] = v0 * M(0,2) + v1 * M(1,2) + v2 * M(2,2) + v3 * M(3,2);
   u[3] = v0 * M(0,3) + v1 * M(1,3) + v2 * M(2,3) + v3 * M(3,3);
#undef M
}

