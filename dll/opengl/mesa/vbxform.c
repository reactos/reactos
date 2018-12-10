/* $Id: vbxform.c,v 1.22 1997/12/17 00:53:18 brianp Exp $ */

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
 * $Log: vbxform.c,v $
 * Revision 1.22  1997/12/17 00:53:18  brianp
 * added #include "asm-386.h"
 *
 * Revision 1.21  1997/12/09 02:58:27  brianp
 * added volatile keyword to prevent excess precision in clip mask computation
 * fixed bug involving clip flags and user clipping planes
 *
 * Revision 1.20  1997/11/20 00:09:38  brianp
 * transform_points4() wasn't calling asm routines
 *
 * Revision 1.19  1997/10/30 06:00:06  brianp
 * added Intel X86 assembly optimzations (Josh Vanderhoof)
 *
 * Revision 1.18  1997/10/15 00:36:36  brianp
 * renamed the FAST/REGULAR_MATH macros
 *
 * Revision 1.17  1997/10/04 00:30:52  brianp
 * vertices specified with glVertex4 weren't always projected correctly
 *
 * Revision 1.16  1997/09/29 22:24:22  brianp
 * added REGULAR/FAST_MATH macros
 *
 * Revision 1.15  1997/09/18 01:32:47  brianp
 * fixed divide by zero problem for "weird" projection matrices
 *
 * Revision 1.14  1997/09/10 00:28:11  brianp
 * fixed an optimization bug in viewport_map_vertices()
 *
 * Revision 1.13  1997/07/24 01:25:27  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.12  1997/06/20 02:57:59  brianp
 * changed color components from GLfixed to GLubyte
 *
 * Revision 1.11  1997/05/28 03:26:49  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.10  1997/05/23 03:01:45  brianp
 * commented out a few const keywords because IRIX cc chokes on them
 *
 * Revision 1.9  1997/04/29 01:31:07  brianp
 * added RasterSetup() function to device driver
 *
 * Revision 1.8  1997/04/21 01:21:52  brianp
 * added MATRIX_2D_NO_ROT
 *
 * Revision 1.7  1997/04/20 19:47:27  brianp
 * added RenderVB to device driver
 *
 * Revision 1.6  1997/04/20 15:59:30  brianp
 * removed VERTEX2_BIT stuff
 *
 * Revision 1.5  1997/04/14 02:12:53  brianp
 * small optimization in transform_texcoords()
 *
 * Revision 1.4  1997/04/12 16:22:22  brianp
 * removed gl_init_vb()
 *
 * Revision 1.3  1997/04/12 12:28:39  brianp
 * fixed <= material_update bug, removed some unused vars
 *
 * Revision 1.2  1997/04/07 03:01:11  brianp
 * optimized vertex[234] code
 *
 * Revision 1.1  1997/04/02 03:14:29  brianp
 * Initial revision
 *
 */


/*
 * This file implements transformation, clip testing and projection of
 * vertices in the vertex buffer.
 *
 * The entry points to this file are the functions:
 *    gl_transform_vb_part1() - first stage of vertex transformation
 *    gl_transform_vb_part2() - second stage of vertex transformation
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <stdlib.h>
#include "asm-386.h"
#include "context.h"
#include "fog.h"
#include "light.h"
#include "macros.h"
#include "matrix.h"
#include "mmath.h"
#include "shade.h"
#include "texture.h"
#include "types.h"
#include "vb.h"
#include "vbrender.h"
#include "vbxform.h"
#include "xform.h"
#include <wine/debug.h>
#endif

WINE_DEFAULT_DEBUG_CHANNEL(opengl32);


#if 0  /* NOT USED AT THIS TIME */
/*
 * Use the current modelview matrix to transform XY vertices from object
 * to eye coordinates.
 * Input:  ctx - the context
 *         n - number of vertices to transform
 *         vObj - array [n][4] of object coordinates
 * In/Out;  vEye - array [n][4] of eye coordinates
 */
static void transform_points2( GLcontext *ctx, GLuint n,
                               const GLfloat vObj[][4], GLfloat vEye[][4] )
{
   switch (ctx->ModelViewMatrixType) {
      case MATRIX_GENERAL:
         {
            const GLfloat *m = ctx->ModelViewMatrix;
            GLfloat m0 = m[0],  m4 = m[4],  m12 = m[12];
            GLfloat m1 = m[1],  m5 = m[5],  m13 = m[13];
            GLfloat m2 = m[2],  m6 = m[6],  m14 = m[14];
            GLfloat m3 = m[3],  m7 = m[7],  m15 = m[15];
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat ox = vObj[i][0], oy = vObj[i][1];
               vEye[i][0] = m0 * ox + m4 * oy + m12;
               vEye[i][1] = m1 * ox + m5 * oy + m13;
               vEye[i][2] = m2 * ox + m6 * oy + m14;
               vEye[i][3] = m3 * ox + m7 * oy + m15;
            }
         }
         break;
      case MATRIX_IDENTITY:
         {
            GLuint i;
            for (i=0;i<n;i++) {
               vEye[i][0] = vObj[i][0];
               vEye[i][1] = vObj[i][1];
               vEye[i][2] = 0.0F;
               vEye[i][3] = 1.0F;
            }
         }
         break;
      case MATRIX_2D:
         {
            const GLfloat *m = ctx->ModelViewMatrix;
            GLfloat m0 = m[0], m1 = m[1], m4 = m[4], m5 = m[5];
            GLfloat m12 = m[12], m13 = m[13];
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat ox = vObj[i][0], oy = vObj[i][1];
               vEye[i][0] = m0 * ox + m4 * oy + m12;
               vEye[i][1] = m1 * ox + m5 * oy + m13;
               vEye[i][2] = 0.0F;
               vEye[i][3] = 1.0F;
            }
         }
         break;
      case MATRIX_2D_NO_ROT:
         {
            const GLfloat *m = ctx->ModelViewMatrix;
            GLfloat m0 = m[0], m5 = m[5], m12 = m[12], m13 = m[13];
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat ox = vObj[i][0], oy = vObj[i][1];
               vEye[i][0] = m0 * ox           + m12;
               vEye[i][1] =           m5 * oy + m13;
               vEye[i][2] = 0.0F;
               vEye[i][3] = 1.0F;
            }
         }
         break;
      case MATRIX_3D:
         {
            const GLfloat *m = ctx->ModelViewMatrix;
            GLfloat m0 = m[0], m1 = m[1], m2 = m[2], m4 = m[4], m5 = m[5];
            GLfloat m6 = m[6], m12 = m[12], m13 = m[13], m14 = m[14];
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat ox = vObj[i][0], oy = vObj[i][1];
               vEye[i][0] = m0 * ox + m4 * oy + m12;
               vEye[i][1] = m1 * ox + m5 * oy + m13;
               vEye[i][2] = m2 * ox + m6 * oy + m14;
               vEye[i][3] = 1.0F;
            }
         }
         break;
      default:
         /* should never get here */
         gl_problem( NULL, "invalid matrix type in transform_points3()" );
         return;
   }
}
#endif


/*
 * Use the current modelview matrix to transform XYZ vertices from object
 * to eye coordinates.
 * Input:  ctx - the context
 *         n - number of vertices to transform
 *         vObj - array [n][4] of object coordinates
 * In/Out;  vEye - array [n][4] of eye coordinates
 */
static void transform_points3( GLcontext *ctx, GLuint n,
                               /*const*/ GLfloat vObj[][4], GLfloat vEye[][4] )
{
#ifndef USE_ASM
   switch (ctx->ModelViewMatrixType) {
      case MATRIX_GENERAL:
         {
            const GLfloat *m = ctx->ModelViewMatrix;
            GLfloat m0 = m[0],  m4 = m[4],  m8 = m[8],  m12 = m[12];
            GLfloat m1 = m[1],  m5 = m[5],  m9 = m[9],  m13 = m[13];
            GLfloat m2 = m[2],  m6 = m[6],  m10 = m[10],  m14 = m[14];
            GLfloat m3 = m[3],  m7 = m[7],  m11 = m[11],  m15 = m[15];
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat ox = vObj[i][0], oy = vObj[i][1], oz = vObj[i][2];
               vEye[i][0] = m0 * ox + m4 * oy + m8  * oz + m12;
               vEye[i][1] = m1 * ox + m5 * oy + m9  * oz + m13;
               vEye[i][2] = m2 * ox + m6 * oy + m10 * oz + m14;
               vEye[i][3] = m3 * ox + m7 * oy + m11 * oz + m15;
            }
         }
         break;
      case MATRIX_IDENTITY:
         {
            GLuint i;
            for (i=0;i<n;i++) {
               vEye[i][0] = vObj[i][0];
               vEye[i][1] = vObj[i][1];
               vEye[i][2] = vObj[i][2];
               vEye[i][3] = 1.0F;
            }
         }
         break;
      case MATRIX_2D:
         {
            const GLfloat *m = ctx->ModelViewMatrix;
            GLfloat m0 = m[0], m1 = m[1], m4 = m[4], m5 = m[5];
            GLfloat m12 = m[12], m13 = m[13];
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat ox = vObj[i][0], oy = vObj[i][1], oz = vObj[i][2];
               vEye[i][0] = m0 * ox + m4 * oy            + m12       ;
               vEye[i][1] = m1 * ox + m5 * oy            + m13       ;
               vEye[i][2] =                   +       oz             ;
               vEye[i][3] =                                      1.0F;
            }
         }
         break;
      case MATRIX_2D_NO_ROT:
         {
            const GLfloat *m = ctx->ModelViewMatrix;
            GLfloat m0 = m[0], m5 = m[5], m12 = m[12], m13 = m[13];
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat ox = vObj[i][0], oy = vObj[i][1], oz = vObj[i][2];
               vEye[i][0] = m0 * ox                      + m12       ;
               vEye[i][1] =           m5 * oy            + m13       ;
               vEye[i][2] =                   +       oz             ;
               vEye[i][3] =                                      1.0F;
            }
         }
         break;
      case MATRIX_3D:
         {
            const GLfloat *m = ctx->ModelViewMatrix;
            GLfloat m0 = m[0], m1 = m[1], m2 = m[2], m4 = m[4], m5 = m[5];
            GLfloat m6 = m[6], m8 = m[8], m9 = m[9], m10 = m[10];
            GLfloat m12 = m[12], m13 = m[13], m14 = m[14];
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat ox = vObj[i][0], oy = vObj[i][1], oz = vObj[i][2];
               vEye[i][0] = m0 * ox + m4 * oy +  m8 * oz + m12       ;
               vEye[i][1] = m1 * ox + m5 * oy +  m9 * oz + m13       ;
               vEye[i][2] = m2 * ox + m6 * oy + m10 * oz + m14       ;
               vEye[i][3] =                                      1.0F;
            }
         }
         break;
      default:
         /* should never get here */
         gl_problem( NULL, "invalid matrix type in transform_points3()" );
   }
#else
   switch (ctx->ModelViewMatrixType) {
      case MATRIX_GENERAL:
         asm_transform_points3_general( n, vEye, ctx->ModelViewMatrix, vObj );
         break;
      case MATRIX_IDENTITY:
         asm_transform_points3_identity( n, vEye, vObj );
         break;
      case MATRIX_2D:
         asm_transform_points3_2d( n, vEye, ctx->ModelViewMatrix, vObj );
         break;
      case MATRIX_2D_NO_ROT:
         asm_transform_points3_2d_no_rot( n, vEye, ctx->ModelViewMatrix,
                                          vObj );
         break;
      case MATRIX_3D:
         asm_transform_points3_3d( n, vEye, ctx->ModelViewMatrix, vObj );
         break;
      default:
         /* should never get here */
         gl_problem( NULL, "invalid matrix type in transform_points3()" );
         return;
   }
#endif
   if (1)
   {
       GLuint i;
       for (i = 0; i < n; i++)
       {
           TRACE("(%3.1f, %3.1f, %3.1f, %3.1f) --> (%3.1f, %3.1f, %3.1f, %3.1f)\n",
                   vObj[i][0], vObj[i][1], vObj[i][2], vObj[i][3],
                   vEye[i][0], vEye[i][1], vEye[i][2], vEye[i][3]);
       }
   }

}



/*
 * Use the current modelview matrix to transform XYZW vertices from object
 * to eye coordinates.
 * Input:  ctx - the context
 *         n - number of vertices to transform
 *         vObj - array [n][4] of object coordinates
 * In/Out;  vEye - array [n][4] of eye coordinates
 */
static void transform_points4( GLcontext *ctx, GLuint n,
                               /*const*/ GLfloat vObj[][4], GLfloat vEye[][4] )
{
#ifndef USE_ASM
   switch (ctx->ModelViewMatrixType) {
      case MATRIX_GENERAL:
         {
            const GLfloat *m = ctx->ModelViewMatrix;
            GLfloat m0 = m[0],  m4 = m[4],  m8 = m[8],  m12 = m[12];
            GLfloat m1 = m[1],  m5 = m[5],  m9 = m[9],  m13 = m[13];
            GLfloat m2 = m[2],  m6 = m[6],  m10 = m[10],  m14 = m[14];
            GLfloat m3 = m[3],  m7 = m[7],  m11 = m[11],  m15 = m[15];
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat ox = vObj[i][0], oy = vObj[i][1];
               GLfloat oz = vObj[i][2], ow = vObj[i][3];
               vEye[i][0] = m0 * ox + m4 * oy + m8  * oz + m12 * ow;
               vEye[i][1] = m1 * ox + m5 * oy + m9  * oz + m13 * ow;
               vEye[i][2] = m2 * ox + m6 * oy + m10 * oz + m14 * ow;
               vEye[i][3] = m3 * ox + m7 * oy + m11 * oz + m15 * ow;
            }
         }
         break;
      case MATRIX_IDENTITY:
         {
            GLuint i;
            for (i=0;i<n;i++) {
               vEye[i][0] = vObj[i][0];
               vEye[i][1] = vObj[i][1];
               vEye[i][2] = vObj[i][2];
               vEye[i][3] = vObj[i][3];
            }
         }
         break;
      case MATRIX_2D:
         {
            const GLfloat *m = ctx->ModelViewMatrix;
            GLfloat m0 = m[0], m1 = m[1], m4 = m[4], m5 = m[5];
            GLfloat m12 = m[12], m13 = m[13];
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat ox = vObj[i][0], oy = vObj[i][1];
               GLfloat oz = vObj[i][2], ow = vObj[i][3];
               vEye[i][0] = m0 * ox + m4 * oy            + m12 * ow;
               vEye[i][1] = m1 * ox + m5 * oy            + m13 * ow;
               vEye[i][2] =                   +       oz           ;
               vEye[i][3] =                                      ow;
            }
         }
         break;
      case MATRIX_2D_NO_ROT:
         {
            const GLfloat *m = ctx->ModelViewMatrix;
            GLfloat m0 = m[0], m5 = m[5], m12 = m[12], m13 = m[13];
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat ox = vObj[i][0], oy = vObj[i][1];
               GLfloat oz = vObj[i][2], ow = vObj[i][3];
               vEye[i][0] = m0 * ox                      + m12 * ow;
               vEye[i][1] =           m5 * oy            + m13 * ow;
               vEye[i][2] =                   +       oz           ;
               vEye[i][3] =                                      ow;
            }
         }
         break;
      case MATRIX_3D:
         {
            const GLfloat *m = ctx->ModelViewMatrix;
            GLfloat m0 = m[0], m1 = m[1], m2 = m[2], m4 = m[4], m5 = m[5];
            GLfloat m6 = m[6], m8 = m[8], m9 = m[9], m10 = m[10];
            GLfloat m12 = m[12], m13 = m[13], m14 = m[14];
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat ox = vObj[i][0], oy = vObj[i][1];
               GLfloat oz = vObj[i][2], ow = vObj[i][3];
               vEye[i][0] = m0 * ox + m4 * oy +  m8 * oz + m12 * ow;
               vEye[i][1] = m1 * ox + m5 * oy +  m9 * oz + m13 * ow;
               vEye[i][2] = m2 * ox + m6 * oy + m10 * oz + m14 * ow;
               vEye[i][3] =                                      ow;
            }
         }
         break;
      default:
         /* should never get here */
         gl_problem( NULL, "invalid matrix type in transform_points4()" );
   }
#else
   switch (ctx->ModelViewMatrixType) {
      case MATRIX_GENERAL:
         asm_transform_points4_general( n, vEye, ctx->ModelViewMatrix, vObj );
         break;
      case MATRIX_IDENTITY:
         asm_transform_points4_identity( n, vEye, vObj );
         break;
      case MATRIX_2D:
         asm_transform_points4_2d( n, vEye, ctx->ModelViewMatrix, vObj );
         break;
      case MATRIX_2D_NO_ROT:
         asm_transform_points4_2d_no_rot( n, vEye, ctx->ModelViewMatrix,
                                          vObj );
         break;
      case MATRIX_3D:
         asm_transform_points4_3d( n, vEye, ctx->ModelViewMatrix, vObj );
         break;
      default:
         /* should never get here */
         gl_problem( NULL, "invalid matrix type in transform_points4()" );
         return;
   }
#endif
}



/*
 * Transform an array of texture coordinates by the current texture matrix.
 * Input:  ctx - the context
 *         n - number of texture coordinates in array
 * In/Out:  t - array [n][4] of texture coordinates to transform
 */
static void transform_texcoords( GLcontext *ctx, GLuint n, GLfloat t[][4] )
{
#ifndef USE_ASM
   switch (ctx->TextureMatrixType) {
      case MATRIX_GENERAL:
         {
            const GLfloat *m = ctx->TextureMatrix;
            GLfloat m0 = m[0],  m4 = m[4],  m8 = m[8],  m12 = m[12];
            GLfloat m1 = m[1],  m5 = m[5],  m9 = m[9],  m13 = m[13];
            GLfloat m2 = m[2],  m6 = m[6],  m10 = m[10],  m14 = m[14];
            GLfloat m3 = m[3],  m7 = m[7],  m11 = m[11],  m15 = m[15];
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat t0 = t[i][0], t1 = t[i][1], t2 = t[i][2], t3 = t[i][3];
               t[i][0] = m0 * t0 + m4 * t1 + m8  * t2 + m12 * t3;
               t[i][1] = m1 * t0 + m5 * t1 + m9  * t2 + m13 * t3;
               t[i][2] = m2 * t0 + m6 * t1 + m10 * t2 + m14 * t3;
               t[i][3] = m3 * t0 + m7 * t1 + m11 * t2 + m15 * t3;
            }
         }
         break;
      case MATRIX_IDENTITY:
         /* Do nothing */
         break;
      case MATRIX_2D:
         {
            const GLfloat *m = ctx->TextureMatrix;
            GLfloat m0 = m[0], m1 = m[1], m4 = m[4], m5 = m[5];
            GLfloat m12 = m[12], m13 = m[13];
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat t0 = t[i][0], t1 = t[i][1], t2 = t[i][2], t3 = t[i][3];
               t[i][0] = m0 * t0 + m4 * t1            + m12 * t3;
               t[i][1] = m1 * t0 + m5 * t1            + m13 * t3;
               t[i][2] =                   +       t2           ;
               /*t[i][3] unchanged*/
            }
         }
         break;
      case MATRIX_3D:
         {
            const GLfloat *m = ctx->TextureMatrix;
            GLfloat m0 = m[0], m1 = m[1], m2 = m[2], m4 = m[4], m5 = m[5];
            GLfloat m6 = m[6], m8 = m[8], m9 = m[9], m10 = m[10];
            GLfloat m12 = m[12], m13 = m[13], m14 = m[14];
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat t0 = t[i][0], t1 = t[i][1], t2 = t[i][2], t3 = t[i][3];
               t[i][0] = m0 * t0 + m4 * t1 +  m8 * t2 + m12 * t3;
               t[i][1] = m1 * t0 + m5 * t1 +  m9 * t2 + m13 * t3;
               t[i][2] = m2 * t0 + m6 * t1 + m10 * t2 + m14 * t3;
               /*t[i][3] unchanged*/
            }
         }
         break;
      default:
         /* should never get here */
         gl_problem( NULL, "invalid matrix type in transform_texcoords()" );
   }
#else
   switch (ctx->TextureMatrixType) {
      case MATRIX_GENERAL:
         asm_transform_points4_general( n, t, ctx->TextureMatrix, t );
         break;
      case MATRIX_IDENTITY:
         /* Do nothing */
         break;
      case MATRIX_2D:
         asm_transform_points4_2d( n, t, ctx->TextureMatrix, t );
         break;
      case MATRIX_3D:
         asm_transform_points4_3d( n, t, ctx->TextureMatrix, t );
         break;
      default:
         /* should never get here */
         gl_problem( NULL, "invalid matrix type in transform_texcoords()" );
         return;
   }
#endif
}



/*
 * Apply the projection matrix to an array of vertices in Eye coordinates
 * resulting in Clip coordinates.  Also, compute the ClipMask bitfield for
 * each vertex.
 *
 * NOTE: the volatile keyword is used in this function to ensure that the
 * FP computations are computed to low-precision.  If high precision is
 * used (ala 80-bit X86 arithmetic) then the clipMask results may be
 * inconsistant with the computations in clip.c.  Later, clipped polygons
 * may be rendered incorrectly.
 *
 * Input:  ctx - the context
 *         n - number of vertices
 *         vEye - array [n][4] of Eye coordinates
 * Output:  vClip - array [n][4] of Clip coordinates
 *          clipMask - array [n] of clip masks
 */
static void project_and_cliptest( GLcontext *ctx,
                                  GLuint n, /*const*/ GLfloat vEye[][4],
                                  GLfloat vClip[][4], GLubyte clipMask[],
                                  GLubyte *orMask, GLubyte *andMask )

{
#ifndef USE_ASM
   GLubyte tmpOrMask = *orMask;
   GLubyte tmpAndMask = *andMask;

   switch (ctx->ProjectionMatrixType) {
      case MATRIX_GENERAL:
         {
            const GLfloat *m = ctx->ProjectionMatrix;
            GLfloat m0 = m[0],  m4 = m[4],  m8 = m[8],  m12 = m[12];
            GLfloat m1 = m[1],  m5 = m[5],  m9 = m[9],  m13 = m[13];
            GLfloat m2 = m[2],  m6 = m[6],  m10 = m[10],  m14 = m[14];
            GLfloat m3 = m[3],  m7 = m[7],  m11 = m[11],  m15 = m[15];
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat ex = vEye[i][0], ey = vEye[i][1];
               GLfloat ez = vEye[i][2], ew = vEye[i][3];
               GLfloat cx = m0 * ex + m4 * ey + m8  * ez + m12 * ew;
               GLfloat cy = m1 * ex + m5 * ey + m9  * ez + m13 * ew;
               GLfloat cz = m2 * ex + m6 * ey + m10 * ez + m14 * ew;
               GLfloat cw = m3 * ex + m7 * ey + m11 * ez + m15 * ew;
               GLubyte mask = 0;
               vClip[i][0] = cx;
               vClip[i][1] = cy;
               vClip[i][2] = cz;
               vClip[i][3] = cw;
               if (cx >  cw)       mask |= CLIP_RIGHT_BIT;
               else if (cx < -cw)  mask |= CLIP_LEFT_BIT;
               if (cy >  cw)       mask |= CLIP_TOP_BIT;
               else if (cy < -cw)  mask |= CLIP_BOTTOM_BIT;
               if (cz >  cw)       mask |= CLIP_FAR_BIT;
               else if (cz < -cw)  mask |= CLIP_NEAR_BIT;
               if (mask) {
                  clipMask[i] |= mask;
                  tmpOrMask |= mask;
               }
               tmpAndMask &= mask;
            }
         }
         break;
      case MATRIX_IDENTITY:
         {
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat cx = vClip[i][0] = vEye[i][0];
               GLfloat cy = vClip[i][1] = vEye[i][1];
               GLfloat cz = vClip[i][2] = vEye[i][2];
               GLfloat cw = vClip[i][3] = vEye[i][3];
               GLubyte mask = 0;
               if (cx >  cw)       mask |= CLIP_RIGHT_BIT;
               else if (cx < -cw)  mask |= CLIP_LEFT_BIT;
               if (cy >  cw)       mask |= CLIP_TOP_BIT;
               else if (cy < -cw)  mask |= CLIP_BOTTOM_BIT;
               if (cz >  cw)       mask |= CLIP_FAR_BIT;
               else if (cz < -cw)  mask |= CLIP_NEAR_BIT;
               if (mask) {
                  clipMask[i] |= mask;
                  tmpOrMask |= mask;
               }
               tmpAndMask &= mask;
            }
         }
         break;
      case MATRIX_ORTHO:
         {
            const GLfloat *m = ctx->ProjectionMatrix;
            GLfloat m0 = m[0], m5 = m[5], m10 = m[10], m12 = m[12];
            GLfloat m13 = m[13], m14 = m[14];
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat ex = vEye[i][0], ey = vEye[i][1];
               GLfloat ez = vEye[i][2], ew = vEye[i][3];
               volatile GLfloat cx = m0 * ex                      + m12 * ew;
               volatile GLfloat cy =           m5 * ey            + m13 * ew;
               volatile GLfloat cz =                     m10 * ez + m14 * ew;
               volatile GLfloat cw =                                      ew;
               GLubyte mask = 0;
               vClip[i][0] = cx;
               vClip[i][1] = cy;
               vClip[i][2] = cz;
               vClip[i][3] = cw;
               if (cx >  cw)       mask |= CLIP_RIGHT_BIT;
               else if (cx < -cw)  mask |= CLIP_LEFT_BIT;
               if (cy >  cw)       mask |= CLIP_TOP_BIT;
               else if (cy < -cw)  mask |= CLIP_BOTTOM_BIT;
               if (cz >  cw)       mask |= CLIP_FAR_BIT;
               else if (cz < -cw)  mask |= CLIP_NEAR_BIT;
               if (mask) {
                  clipMask[i] |= mask;
                  tmpOrMask |= mask;
               }
               tmpAndMask &= mask;
            }
         }
         break;
      case MATRIX_PERSPECTIVE:
         {
            const GLfloat *m = ctx->ProjectionMatrix;
            GLfloat m0 = m[0], m5 = m[5], m8 = m[8], m9 = m[9];
            GLfloat m10 = m[10], m14 = m[14];
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat ex = vEye[i][0], ey = vEye[i][1];
               GLfloat ez = vEye[i][2], ew = vEye[i][3];
               volatile GLfloat cx = m0 * ex           + m8  * ez           ;
               volatile GLfloat cy =           m5 * ey + m9  * ez           ;
               volatile GLfloat cz =                     m10 * ez + m14 * ew;
               volatile GLfloat cw =                          -ez           ;
               GLubyte mask = 0;
               vClip[i][0] = cx;
               vClip[i][1] = cy;
               vClip[i][2] = cz;
               vClip[i][3] = cw;
               if (cx >  cw)       mask |= CLIP_RIGHT_BIT;
               else if (cx < -cw)  mask |= CLIP_LEFT_BIT;
               if (cy >  cw)       mask |= CLIP_TOP_BIT;
               else if (cy < -cw)  mask |= CLIP_BOTTOM_BIT;
               if (cz >  cw)       mask |= CLIP_FAR_BIT;
               else if (cz < -cw)  mask |= CLIP_NEAR_BIT;
               if (mask) {
                  clipMask[i] |= mask;
                  tmpOrMask |= mask;
               }
               tmpAndMask &= mask;
            }
         }
         break;
      default:
         /* should never get here */
         gl_problem( NULL, "invalid matrix type in project_and_cliptest()" );
   }

   *orMask = tmpOrMask;
   *andMask = tmpAndMask;
#else
   switch (ctx->ProjectionMatrixType) {
      case MATRIX_GENERAL:
         asm_project_and_cliptest_general( n, vClip, ctx->ProjectionMatrix, vEye, 
                                           clipMask, orMask, andMask );
         break;
      case MATRIX_IDENTITY:
         asm_project_and_cliptest_identity( n, vClip, vEye, clipMask, orMask, andMask );
         break;
      case MATRIX_ORTHO:
         asm_project_and_cliptest_ortho( n, vClip, ctx->ProjectionMatrix, vEye, 
                                         clipMask, orMask, andMask );
         break;
      case MATRIX_PERSPECTIVE:
         asm_project_and_cliptest_perspective( n, vClip, ctx->ProjectionMatrix,
                                               vEye, clipMask, orMask, andMask );
         break;
      default:
         /* should never get here */
         gl_problem( NULL, "invalid matrix type in project_and_cliptest()" );
         return;
   }
#endif
}


/* This value matches the one in clip.c, used to cope with numeric error. */
#define MAGIC_NUMBER -0.8e-03F

/*
 * Test an array of vertices against the user-defined clipping planes.
 * Input:  ctx - the context
 *         n - number of vertices
 *         vEye - array [n] of vertices, in eye coordinate system
 * Output:  clipMask - array [n] of clip values: 0=not clipped, !0=clipped
 * Return:  CLIP_ALL - if all vertices are clipped by one of the planes
 *          CLIP_NONE - if no vertices were clipped
 *          CLIP_SOME - if some vertices were clipped
 */
static GLuint userclip_vertices( GLcontext *ctx, GLuint n,
                                 /*const*/ GLfloat vEye[][4],
                                 GLubyte clipMask[] )
{
   GLboolean anyClipped = GL_FALSE;
   GLuint p;

   ASSERT(ctx->Transform.AnyClip);

   for (p=0;p<MAX_CLIP_PLANES;p++) {
      if (ctx->Transform.ClipEnabled[p]) {
         GLfloat a = ctx->Transform.ClipEquation[p][0];
         GLfloat b = ctx->Transform.ClipEquation[p][1];
         GLfloat c = ctx->Transform.ClipEquation[p][2];
         GLfloat d = ctx->Transform.ClipEquation[p][3];
         GLboolean allClipped = GL_TRUE;
         GLuint i;
         for (i=0;i<n;i++) {
            GLfloat dot = vEye[i][0] * a + vEye[i][1] * b
                        + vEye[i][2] * c + vEye[i][3] * d;
            if (dot < MAGIC_NUMBER) {
               /* this vertex is clipped */
               clipMask[i] = CLIP_USER_BIT;
               anyClipped = GL_TRUE;
            }
            else {
               /* vertex not clipped */
               allClipped = GL_FALSE;
            }
         }
         if (allClipped) {
            return CLIP_ALL;
         }
      }
   }

   return anyClipped ? CLIP_SOME : CLIP_NONE;
}



/*
 * Transform an array of vertices from clip coordinate space to window
 * coordinates.
 * Input:  ctx - the context
 *         n - number of vertices to transform
 *         vClip - array [n] of input vertices
 *         clipMask - array [n] of vertex clip masks.  NULL = no clipped verts
 * Output:  vWin - array [n] of vertices in window coordinate system
 */
static void viewport_map_vertices( GLcontext *ctx,
                                   GLuint n, /*const*/ GLfloat vClip[][4],
                                   const GLubyte clipMask[], GLfloat vWin[][3])
{
   GLfloat sx = ctx->Viewport.Sx;
   GLfloat tx = ctx->Viewport.Tx;
   GLfloat sy = ctx->Viewport.Sy;
   GLfloat ty = ctx->Viewport.Ty;
   GLfloat sz = ctx->Viewport.Sz;
   GLfloat tz = ctx->Viewport.Tz;

   if ((ctx->ProjectionMatrixType==MATRIX_ORTHO || 
        ctx->ProjectionMatrixType==MATRIX_IDENTITY)
       && ctx->ModelViewMatrixType!=MATRIX_GENERAL
       && (ctx->VB->VertexSizeMask & VERTEX4_BIT)==0) {
      /* don't need to divide by W */
      if (clipMask) {
         /* one or more vertices are clipped */
         GLuint i;
         for (i=0;i<n;i++) {
            if (clipMask[i]==0) {
               vWin[i][0] = vClip[i][0] * sx + tx;
               vWin[i][1] = vClip[i][1] * sy + ty;
               vWin[i][2] = vClip[i][2] * sz + tz;
            }
         }
      }
      else {
         /* no vertices are clipped */
         GLuint i;
         for (i=0;i<n;i++) {
            vWin[i][0] = vClip[i][0] * sx + tx;
            vWin[i][1] = vClip[i][1] * sy + ty;
            vWin[i][2] = vClip[i][2] * sz + tz;
         }
      }
   }
   else {
      /* need to divide by W */
      if (clipMask) {
         /* one or more vertices are clipped */
         GLuint i;
         for (i=0;i<n;i++) {
            if (clipMask[i] == 0) {
               if (vClip[i][3] != 0.0F) {
                  GLfloat wInv = 1.0F / vClip[i][3];
                  vWin[i][0] = vClip[i][0] * wInv * sx + tx;
                  vWin[i][1] = vClip[i][1] * wInv * sy + ty;
                  vWin[i][2] = vClip[i][2] * wInv * sz + tz;
               }
               else {
                  /* Div by zero!  Can't set window coords to infinity, so...*/
                  vWin[i][0] = 0.0F;
                  vWin[i][1] = 0.0F;
                  vWin[i][2] = 0.0F;
               }
            }
         }
      }
      else {
         /* no vertices are clipped */
         GLuint i;
         for (i=0;i<n;i++) {
            if (vClip[i][3] != 0.0F) {
               GLfloat wInv = 1.0F / vClip[i][3];
               vWin[i][0] = vClip[i][0] * wInv * sx + tx;
               vWin[i][1] = vClip[i][1] * wInv * sy + ty;
               vWin[i][2] = vClip[i][2] * wInv * sz + tz;
            }
            else {
               /* Divide by zero!  Can't set window coords to infinity, so...*/
               vWin[i][0] = 0.0F;
               vWin[i][1] = 0.0F;
               vWin[i][2] = 0.0F;
            }
         }
      }
   }
   if (1)
   {
       GLuint i;
       for (i = 0; i < n; i++)
       {
           TRACE("(%3.1f, %3.1f, %3.1f, %3.1f) --> (%3.1f, %3.1f, %3.1f)\n",
                   vClip[i][0], vClip[i][1], vClip[i][2], vClip[i][3],
                   vWin[i][0], vWin[i][1], vWin[i][2]);
       }
   }
}



/*
 * Check if the global material has to be updated with info that was
 * associated with a vertex via glMaterial.
 * This function is used when any material values get changed between
 * glBegin/glEnd either by calling glMaterial() or by calling glColor()
 * when GL_COLOR_MATERIAL is enabled.
 */
static void update_material( GLcontext *ctx, GLuint i )
{
   struct vertex_buffer *VB = ctx->VB;

   if (VB->MaterialMask[i]) {
      if (VB->MaterialMask[i] & FRONT_AMBIENT_BIT) {
         COPY_4V( ctx->Light.Material[0].Ambient, VB->Material[i][0].Ambient );
      }
      if (VB->MaterialMask[i] & BACK_AMBIENT_BIT) {
         COPY_4V( ctx->Light.Material[1].Ambient, VB->Material[i][1].Ambient );
      }
      if (VB->MaterialMask[i] & FRONT_DIFFUSE_BIT) {
         COPY_4V( ctx->Light.Material[0].Diffuse, VB->Material[i][0].Diffuse );
      }
      if (VB->MaterialMask[i] & BACK_DIFFUSE_BIT) {
         COPY_4V( ctx->Light.Material[1].Diffuse, VB->Material[i][1].Diffuse );
      }
      if (VB->MaterialMask[i] & FRONT_SPECULAR_BIT) {
         COPY_4V( ctx->Light.Material[0].Specular, VB->Material[i][0].Specular );
      }
      if (VB->MaterialMask[i] & BACK_SPECULAR_BIT) {
         COPY_4V( ctx->Light.Material[1].Specular, VB->Material[i][1].Specular );
      }
      if (VB->MaterialMask[i] & FRONT_EMISSION_BIT) {
         COPY_4V( ctx->Light.Material[0].Emission, VB->Material[i][0].Emission );
      }
      if (VB->MaterialMask[i] & BACK_EMISSION_BIT) {
         COPY_4V( ctx->Light.Material[1].Emission, VB->Material[i][1].Emission );
      }
      if (VB->MaterialMask[i] & FRONT_SHININESS_BIT) {
         ctx->Light.Material[0].Shininess = VB->Material[i][0].Shininess;
         gl_compute_material_shine_table( &ctx->Light.Material[0] );
      }
      if (VB->MaterialMask[i] & BACK_SHININESS_BIT) {
         ctx->Light.Material[1].Shininess = VB->Material[i][1].Shininess;
         gl_compute_material_shine_table( &ctx->Light.Material[1] );
      }
      if (VB->MaterialMask[i] & FRONT_INDEXES_BIT) {
         ctx->Light.Material[0].AmbientIndex = VB->Material[i][0].AmbientIndex;
         ctx->Light.Material[0].DiffuseIndex = VB->Material[i][0].DiffuseIndex;
         ctx->Light.Material[0].SpecularIndex = VB->Material[i][0].SpecularIndex;
      }
      if (VB->MaterialMask[i] & BACK_INDEXES_BIT) {
         ctx->Light.Material[1].AmbientIndex = VB->Material[i][1].AmbientIndex;
         ctx->Light.Material[1].DiffuseIndex = VB->Material[i][1].DiffuseIndex;
         ctx->Light.Material[1].SpecularIndex = VB->Material[i][1].SpecularIndex;
      }
      VB->MaterialMask[i] = 0;  /* reset now */
   }
}


/*
 * Compute the shading (lighting) for the vertices in the vertex buffer.
 */
static void shade_vertices( GLcontext *ctx )
{
   struct vertex_buffer *VB = ctx->VB;

   if (ctx->Visual->RGBAflag) {
      if (!VB->MonoMaterial) {
         /* Material may change with each vertex */
         GLuint i;
         for (i=VB->Start; i<VB->Count; i++) {
            update_material( ctx, i );
            gl_color_shade_vertices( ctx, 0, 1, &VB->Eye[i],
                                     &VB->Normal[i], &VB->Fcolor[i]);
            if (ctx->Light.Model.TwoSide) {
               gl_color_shade_vertices( ctx, 1, 1, &VB->Eye[i],
                                        &VB->Normal[i], &VB->Bcolor[i]);
            }
         }
         /* Need this in case a glColor/glMaterial is called after the
          * last vertex between glBegin/glEnd.
          */
         update_material( ctx, VB->Count );
      }
      else {
         if (ctx->Light.Fast) {
            if (VB->MonoNormal) {
               /* call optimized shader */
               GLubyte color[1][4];
               GLuint i;
               gl_color_shade_vertices_fast( ctx, 0,  /* front side */
                                             1,
                                             VB->Normal + VB->Start,
                                             color );
               for (i=VB->Start; i<VB->Count; i++) {
                  COPY_4V( VB->Fcolor[i], color[0] );
               }
               if (ctx->Light.Model.TwoSide) {
                  gl_color_shade_vertices_fast( ctx, 1,  /* back side */
                                                1,
                                                VB->Normal + VB->Start,
                                                color );
                  for (i=VB->Start; i<VB->Count; i++) {
                     COPY_4V( VB->Bcolor[i], color[0] );
                  }
               }

            }
            else {
               /* call optimized shader */
               gl_color_shade_vertices_fast( ctx, 0,  /* front side */
                                             VB->Count - VB->Start,
                                             VB->Normal + VB->Start,
                                             VB->Fcolor + VB->Start );
               if (ctx->Light.Model.TwoSide) {
                  gl_color_shade_vertices_fast( ctx, 1,  /* back side */
                                                VB->Count - VB->Start,
                                                VB->Normal + VB->Start,
                                                VB->Bcolor + VB->Start );
               }
            }
         }
         else {
            /* call slower, full-featured shader */
            gl_color_shade_vertices( ctx, 0,
                                     VB->Count - VB->Start,
                                     VB->Eye + VB->Start,
                                     VB->Normal + VB->Start,
                                     VB->Fcolor + VB->Start );
            if (ctx->Light.Model.TwoSide) {
               gl_color_shade_vertices( ctx, 1,
                                        VB->Count - VB->Start,
                                        VB->Eye + VB->Start,
                                        VB->Normal + VB->Start,
                                        VB->Bcolor + VB->Start );
            }
         }
      }
   }
   else {
      /* Color index mode */
      if (!VB->MonoMaterial) {
         /* Material may change with each vertex */
         GLuint i;
         /* NOTE the <= here.  This is needed in case glColor/glMaterial
          * is called after the last glVertex inside a glBegin/glEnd pair.
          */
         for (i=VB->Start; i<VB->Count; i++) {
            update_material( ctx, i );
            gl_index_shade_vertices( ctx, 0, 1, &VB->Eye[i],
                                     &VB->Normal[i], &VB->Findex[i] );
            if (ctx->Light.Model.TwoSide) {
               gl_index_shade_vertices( ctx, 1, 1, &VB->Eye[i],
                                        &VB->Normal[i], &VB->Bindex[i] );
            }
         }
         /* Need this in case a glColor/glMaterial is called after the
          * last vertex between glBegin/glEnd.
          */
         update_material( ctx, VB->Count );
      }
      else {
         gl_index_shade_vertices( ctx, 0,
                                  VB->Count - VB->Start,
                                  VB->Eye + VB->Start,
                                  VB->Normal + VB->Start,
                                  VB->Findex + VB->Start );
         if (ctx->Light.Model.TwoSide) {
            gl_index_shade_vertices( ctx, 1,
                                     VB->Count - VB->Start,
                                     VB->Eye + VB->Start,
                                     VB->Normal + VB->Start,
                                     VB->Bindex + VB->Start );
         }
      }
   }
}



/*
 * Compute fog for the vertices in the vertex buffer.
 */
static void fog_vertices( GLcontext *ctx )
{
   struct vertex_buffer *VB = ctx->VB;

   if (ctx->Visual->RGBAflag) {
      /* Fog RGB colors */
      gl_fog_color_vertices( ctx, VB->Count - VB->Start,
                             VB->Eye + VB->Start,
                             VB->Fcolor + VB->Start );
      if (ctx->LightTwoSide) {
         gl_fog_color_vertices( ctx, VB->Count - VB->Start,
                                VB->Eye + VB->Start,
                                VB->Bcolor + VB->Start );
      }
   }
   else {
      /* Fog color indexes */
      gl_fog_index_vertices( ctx, VB->Count - VB->Start,
                             VB->Eye + VB->Start,
                             VB->Findex + VB->Start );
      if (ctx->LightTwoSide) {
         gl_fog_index_vertices( ctx, VB->Count - VB->Start,
                                VB->Eye + VB->Start,
                                VB->Bindex + VB->Start );
      }
   }
}



/*
 * When the Vertex Buffer is full, this function applies the modelview
 * matrix to transform vertices and normals from object coordinates to
 * eye coordinates.  Next, we'll call gl_transform_vb_part2()...
 * This function might not be called when using vertex arrays.
 */
void gl_transform_vb_part1( GLcontext *ctx, GLboolean allDone )
{
   struct vertex_buffer *VB = ctx->VB;
#ifdef PROFILE
   GLdouble t0 = gl_time();
#endif

   ASSERT( VB->Count>0 );

   /* Apply the modelview matrix to transform vertexes from Object
    * to Eye coords.
    */
   if (VB->VertexSizeMask==VERTEX4_BIT) {
      transform_points4( ctx, VB->Count - VB->Start,
                         VB->Obj + VB->Start, VB->Eye + VB->Start );
   }
   else {
      transform_points3( ctx, VB->Count - VB->Start,
                         VB->Obj + VB->Start, VB->Eye + VB->Start );
   }

   /* Now transform the normal vectors */
   if (ctx->NeedNormals) {
      gl_xform_normals_3fv( VB->Count - VB->Start,
                            VB->Normal + VB->Start, ctx->ModelViewInv,
                            VB->Normal + VB->Start, ctx->Transform.Normalize );
   }

#ifdef PROFILE
   ctx->VertexTime += gl_time() - t0;
#endif

   /* lighting, project, etc */
   gl_transform_vb_part2( ctx, allDone );
}



/*
 * Part 2 of Vertex Buffer transformation:  compute lighting, clipflags,
 * fog, texture coords, etc.
 * Before this function is called the VB->Eye coordinates must have
 * already been computed.
 * Callers:  gl_transform_vb_part1(), glDrawArraysEXT()
 */
void gl_transform_vb_part2( GLcontext *ctx, GLboolean allDone )
{
   struct vertex_buffer *VB = ctx->VB;
#ifdef PROFILE
   GLdouble t0 = gl_time();
#endif

   ASSERT( VB->Count>0 );

   /* Test vertices in eye coordinate space against user clipping planes */
   if (ctx->Transform.AnyClip) {
      GLuint result = userclip_vertices( ctx, VB->Count - VB->Start,
                                         VB->Eye + VB->Start,
                                         VB->ClipMask + VB->Start );
      if (result==CLIP_ALL) {
         /* All vertices were outside one of the clip planes! */
         VB->ClipOrMask = CLIP_ALL_BITS; /* force reset of clipping flags */
         gl_reset_vb( ctx, allDone );
         return;
      }
      else if (result==CLIP_SOME) {
         VB->ClipOrMask = CLIP_USER_BIT;
      }
      else {
         VB->ClipAndMask = 0;
      }
   }

   /* Apply the projection matrix to the Eye coordinates, resulting in
    * Clip coordinates.  Also, compute the ClipMask for each vertex.
    */
   project_and_cliptest( ctx, VB->Count - VB->Start, VB->Eye + VB->Start,
                         VB->Clip + VB->Start, VB->ClipMask + VB->Start,
                         &VB->ClipOrMask, &VB->ClipAndMask );

   if (VB->ClipAndMask) {
      /* All vertices clipped by one plane, all done! */
      /*assert(VB->ClipOrMask);*/
      VB->ClipOrMask = CLIP_ALL_BITS; /* force reset of clipping flags */
      gl_reset_vb( ctx, allDone );
      return;
   }

   /* Lighting */
   if (ctx->Light.Enabled) {
      shade_vertices(ctx);
   }

   /* Per-vertex fog */
   if (ctx->Fog.Enabled && ctx->Hint.Fog!=GL_NICEST) {
      fog_vertices(ctx);
   }

   /* Generate/transform texture coords */
   if (ctx->Texture.Enabled || ctx->RenderMode==GL_FEEDBACK) {
      if (ctx->Texture.TexGenEnabled) {
         gl_texgen( ctx, VB->Count - VB->Start,
                    VB->Obj + VB->Start,
                    VB->Eye + VB->Start,
                    VB->Normal + VB->Start,
                    VB->TexCoord + VB->Start );
      }
      if (ctx->NewTextureMatrix) {
         gl_analyze_texture_matrix(ctx);
      }
      if (ctx->TextureMatrixType!=MATRIX_IDENTITY) {
         transform_texcoords( ctx, VB->Count - VB->Start,
                              VB->TexCoord + VB->Start );
      }
   }

   /* Use the viewport parameters to transform vertices from Clip
    * coordinates to Window coordinates.
    */
   viewport_map_vertices( ctx, VB->Count - VB->Start, VB->Clip + VB->Start,
                          VB->ClipOrMask ? VB->ClipMask + VB->Start : NULL,
                          VB->Win + VB->Start );

   /* Device driver rasterization setup.  3Dfx driver, for example. */
   if (ctx->Driver.RasterSetup) {
      (*ctx->Driver.RasterSetup)( ctx, 0, VB->Count );
   }


#ifdef PROFILE
   ctx->VertexTime += gl_time() - t0;
   ctx->VertexCount += VB->Count - VB->Start;
#endif

   /*
    * Now we're ready to rasterize the Vertex Buffer!!!
    *
    * If the device driver can't rasterize the vertex buffer then we'll
    * do it ourselves.
    */
   if (!ctx->Driver.RenderVB || !(*ctx->Driver.RenderVB)(ctx,allDone)) {
      gl_render_vb( ctx, allDone );
   }
}
