/* $Id: clip.c,v 1.16 1998/02/03 23:45:36 brianp Exp $ */

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
 * $Log: clip.c,v $
 * Revision 1.16  1998/02/03 23:45:36  brianp
 * added space parameter to clip interpolation functions
 *
 * Revision 1.15  1998/01/06 02:40:52  brianp
 * added DavidB's clipping interpolation optimization
 *
 * Revision 1.14  1997/07/24 01:24:45  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.13  1997/05/28 03:23:48  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.12  1997/04/02 03:10:06  brianp
 * call gl_analyze_modelview_matrix instead of gl_compute_modelview_inverse
 *
 * Revision 1.11  1997/02/13 21:16:09  brianp
 * if too many vertices in polygon return VB_SIZE-1, not VB_SIZE
 *
 * Revision 1.10  1997/02/10 21:16:12  brianp
 * added checks in polygon clippers to prevent array overflows
 *
 * Revision 1.9  1997/02/04 19:39:39  brianp
 * changed size of vlist2[] arrays to VB_SIZE per Randy Frank
 *
 * Revision 1.8  1996/12/02 20:10:07  brianp
 * changed the macros in gl_viewclip_polygon() to be like gl_viewclip_line()
 *
 * Revision 1.7  1996/10/29 02:55:02  brianp
 * fixed duplicate vertex bug in gl_viewclip_polygon()
 *
 * Revision 1.6  1996/10/07 23:48:33  brianp
 * changed temporaries to GLdouble in gl_viewclip_polygon()
 *
 * Revision 1.5  1996/10/03 01:43:45  brianp
 * changed INSIDE() macro in gl_viewclip_polygon() to work like other macros
 *
 * Revision 1.4  1996/10/03 01:36:33  brianp
 * changed COMPUTE_INTERSECTION macros in gl_viewclip_polygon to avoid
 * potential roundoff errors
 *
 * Revision 1.3  1996/09/27 01:24:23  brianp
 * removed unused variables
 *
 * Revision 1.2  1996/09/15 01:48:58  brianp
 * removed #define NULL 0
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <string.h>
#include "clip.h"
#include "context.h"
#include "dlist.h"
#include "macros.h"
#include "matrix.h"
#include "types.h"
#include "vb.h"
#include "xform.h"
#endif




/* Linear interpolation between A and B: */
#define LINTERP( T, A, B )   ( (A) + (T) * ( (B) - (A) ) )


/* Clipping coordinate spaces */
#define EYE_SPACE 1
#define CLIP_SPACE 2



/*
 * This function is used to interpolate colors, indexes, and texture
 * coordinates when clipping has to be done.  In general, we compute
 *     aux[dst] = aux[in] + t * (aux[out] - aux[in])
 * where aux is the quantity to be interpolated.
 * Input:  space - either EYE_SPACE or CLIP_SPACE
 *         dst - index of array position to store interpolated value
 *         t - a value in [0,1]
 *         in - index of array position corresponding to 'inside' vertex
 *         out - index of array position corresponding to 'outside' vertex
 */
void interpolate_aux( GLcontext* ctx, GLuint space,
                      GLuint dst, GLfloat t, GLuint in, GLuint out )
{
   struct vertex_buffer* VB = ctx->VB;

   if (ctx->ClipMask & CLIP_FCOLOR_BIT) {
      VB->Fcolor[dst][0] = LINTERP( t, VB->Fcolor[in][0], VB->Fcolor[out][0] );
      VB->Fcolor[dst][1] = LINTERP( t, VB->Fcolor[in][1], VB->Fcolor[out][1] );
      VB->Fcolor[dst][2] = LINTERP( t, VB->Fcolor[in][2], VB->Fcolor[out][2] );
      VB->Fcolor[dst][3] = LINTERP( t, VB->Fcolor[in][3], VB->Fcolor[out][3] );
   }
   else if (ctx->ClipMask & CLIP_FINDEX_BIT) {
      VB->Findex[dst] = (GLuint) (GLint) LINTERP( t, (GLfloat) VB->Findex[in],
                                                 (GLfloat) VB->Findex[out] );
   }

   if (ctx->ClipMask & CLIP_BCOLOR_BIT) {
      VB->Bcolor[dst][0] = LINTERP( t, VB->Bcolor[in][0], VB->Bcolor[out][0] );
      VB->Bcolor[dst][1] = LINTERP( t, VB->Bcolor[in][1], VB->Bcolor[out][1] );
      VB->Bcolor[dst][2] = LINTERP( t, VB->Bcolor[in][2], VB->Bcolor[out][2] );
      VB->Bcolor[dst][3] = LINTERP( t, VB->Bcolor[in][3], VB->Bcolor[out][3] );
   }
   else if (ctx->ClipMask & CLIP_BINDEX_BIT) {
      VB->Bindex[dst] = (GLuint) (GLint) LINTERP( t, (GLfloat) VB->Bindex[in],
                                                 (GLfloat) VB->Bindex[out] );
   }

   if (ctx->ClipMask & CLIP_TEXTURE_BIT) {
      /* TODO: is more sophisticated texture coord interpolation needed?? */
      if (space==CLIP_SPACE) {
	 /* also interpolate eye Z component */
	 VB->Eye[dst][2] = LINTERP( t, VB->Eye[in][2], VB->Eye[out][2] );
      }
      VB->TexCoord[dst][0] = LINTERP(t,VB->TexCoord[in][0],VB->TexCoord[out][0]);
      VB->TexCoord[dst][1] = LINTERP(t,VB->TexCoord[in][1],VB->TexCoord[out][1]);
      VB->TexCoord[dst][2] = LINTERP(t,VB->TexCoord[in][2],VB->TexCoord[out][2]);
      VB->TexCoord[dst][3] = LINTERP(t,VB->TexCoord[in][3],VB->TexCoord[out][3]);
   }

}


/*
 * Some specialized version of the interpolate_aux
 *
 */

void interpolate_aux_color_tex2( GLcontext* ctx, GLuint space,
				 GLuint dst, GLfloat t, GLuint in, GLuint out )
{
   struct vertex_buffer* VB = ctx->VB;

   VB->Fcolor[dst][0] = LINTERP( t, VB->Fcolor[in][0], VB->Fcolor[out][0] );
   VB->Fcolor[dst][1] = LINTERP( t, VB->Fcolor[in][1], VB->Fcolor[out][1] );
   VB->Fcolor[dst][2] = LINTERP( t, VB->Fcolor[in][2], VB->Fcolor[out][2] );
   VB->Fcolor[dst][3] = LINTERP( t, VB->Fcolor[in][3], VB->Fcolor[out][3] );

   VB->Eye[dst][2] = LINTERP( t, VB->Eye[in][2], VB->Eye[out][2] );
   VB->TexCoord[dst][0] = LINTERP(t,VB->TexCoord[in][0],VB->TexCoord[out][0]);
   VB->TexCoord[dst][1] = LINTERP(t,VB->TexCoord[in][1],VB->TexCoord[out][1]);
}


void interpolate_aux_tex2( GLcontext* ctx, GLuint space,
			   GLuint dst, GLfloat t, GLuint in, GLuint out )
{
   struct vertex_buffer* VB = ctx->VB;

   VB->Eye[dst][2] = LINTERP( t, VB->Eye[in][2], VB->Eye[out][2] );
   VB->TexCoord[dst][0] = LINTERP(t,VB->TexCoord[in][0],VB->TexCoord[out][0]);
   VB->TexCoord[dst][1] = LINTERP(t,VB->TexCoord[in][1],VB->TexCoord[out][1]);
}


void interpolate_aux_color( GLcontext* ctx, GLuint space,
			    GLuint dst, GLfloat t, GLuint in, GLuint out )
{
   struct vertex_buffer* VB = ctx->VB;

   VB->Fcolor[dst][0] = LINTERP( t, VB->Fcolor[in][0], VB->Fcolor[out][0] );
   VB->Fcolor[dst][1] = LINTERP( t, VB->Fcolor[in][1], VB->Fcolor[out][1] );
   VB->Fcolor[dst][2] = LINTERP( t, VB->Fcolor[in][2], VB->Fcolor[out][2] );
   VB->Fcolor[dst][3] = LINTERP( t, VB->Fcolor[in][3], VB->Fcolor[out][3] );
}




void gl_ClipPlane( GLcontext* ctx, GLenum plane, const GLfloat *equation )
{
   GLint p;

   p = (GLint) plane - (GLint) GL_CLIP_PLANE0;
   if (p<0 || p>=MAX_CLIP_PLANES) {
      gl_error( ctx, GL_INVALID_ENUM, "glClipPlane" );
      return;
   }

   /*
    * The equation is transformed by the transpose of the inverse of the
    * current modelview matrix and stored in the resulting eye coordinates.
    */
   if (ctx->NewModelViewMatrix) {
      gl_analyze_modelview_matrix(ctx);
   }
   gl_transform_vector( ctx->Transform.ClipEquation[p], equation,
		        ctx->ModelViewInv );
}



void gl_GetClipPlane( GLcontext* ctx, GLenum plane, GLdouble *equation )
{
   GLint p;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glGetClipPlane" );
      return;
   }

   p = (GLint) (plane - GL_CLIP_PLANE0);
   if (p<0 || p>=MAX_CLIP_PLANES) {
      gl_error( ctx, GL_INVALID_ENUM, "glGetClipPlane" );
      return;
   }

   equation[0] = (GLdouble) ctx->Transform.ClipEquation[p][0];
   equation[1] = (GLdouble) ctx->Transform.ClipEquation[p][1];
   equation[2] = (GLdouble) ctx->Transform.ClipEquation[p][2];
   equation[3] = (GLdouble) ctx->Transform.ClipEquation[p][3];
}




/**********************************************************************/
/*                         View volume clipping.                      */
/**********************************************************************/


/*
 * Clip a point against the view volume.
 * Input:  v - vertex-vector describing the point to clip
 * Return:  0 = outside view volume
 *          1 = inside view volume
 */
GLuint gl_viewclip_point( const GLfloat v[] )
{
   if (   v[0] > v[3] || v[0] < -v[3]
       || v[1] > v[3] || v[1] < -v[3]
       || v[2] > v[3] || v[2] < -v[3] ) {
      return 0;
   }
   else {
      return 1;
   }
}




/*
 * Clip a line segment against the view volume defined by -w<=x,y,z<=w.
 * Input:  i, j - indexes into VB->V* of endpoints of the line
 * Return:  0 = line completely outside of view
 *          1 = line is inside view.
 */
GLuint gl_viewclip_line( GLcontext* ctx, GLuint *i, GLuint *j )
{
   struct vertex_buffer* VB = ctx->VB;
   GLfloat (*coord)[4] = VB->Clip;

   GLfloat t, dx, dy, dz, dw;
   register GLuint ii, jj;

   ii = *i;
   jj = *j;

/*
 * We use 6 instances of this code to clip agains the 6 planes.
 * For each plane, we define the OUTSIDE and COMPUTE_INTERSECTION
 * macros apprpriately.
 */
#define GENERAL_CLIP							\
   if (OUTSIDE(ii)) {           	                        	\
      if (OUTSIDE(jj)) {                	                	\
         /* both verts are outside ==> return 0 */			\
         return 0;                                      		\
      }                                                 		\
      else {                                            		\
         /* ii is outside, jj is inside ==> clip */     		\
	 /* new vertex put in position VB->Free */			\
         COMPUTE_INTERSECTION( VB->Free, jj, ii )             		\
	 if (ctx->ClipMask)						\
            ctx->ClipInterpAuxFunc( ctx, CLIP_SPACE, VB->Free, t, jj, ii );\
	 ii = VB->Free;							\
	 VB->Free++;							\
	 if (VB->Free==VB_SIZE)  VB->Free = 1;				\
      }                                                 		\
   }                                                    		\
   else {                                               		\
      if (OUTSIDE(jj)) {                                		\
         /* ii is inside, jj is outside ==> clip */     		\
	 /* new vertex put in position VB->Free */			\
         COMPUTE_INTERSECTION( VB->Free, ii, jj );            		\
	 if (ctx->ClipMask)						\
	    ctx->ClipInterpAuxFunc( ctx, CLIP_SPACE, VB->Free, t, ii, jj );\
	 jj = VB->Free;							\
	 VB->Free++;							\
	 if (VB->Free==VB_SIZE)  VB->Free = 1;				\
      }                                                 		\
      /* else both verts are inside ==> do nothing */   		\
   }


#define X(I)	coord[I][0]
#define Y(I)	coord[I][1]
#define Z(I)	coord[I][2]
#define W(I)	coord[I][3]

/*
 * Begin clipping
 */

   /*** Clip against +X side ***/
#define OUTSIDE(K)      (X(K) > W(K))
#define COMPUTE_INTERSECTION( new, in, out )		\
	dx = X(out) - X(in);				\
	dw = W(out) - W(in);				\
	t = (X(in) - W(in)) / (dw-dx);			\
	X(new) = X(in) + t * dx;			\
	Y(new) = Y(in) + t * (Y(out) - Y(in));		\
	Z(new) = Z(in) + t * (Z(out) - Z(in));		\
	W(new) = W(in) + t * dw;

   GENERAL_CLIP

#undef OUTSIDE
#undef COMPUTE_INTERSECTION


   /*** Clip against -X side ***/
#define OUTSIDE(K)      (X(K) < -W(K))
#define COMPUTE_INTERSECTION( new, in, out )		\
	dx = X(out) - X(in);				\
	dw = W(out) - W(in);				\
        t = -(X(in) + W(in)) / (dw+dx);			\
	X(new) = X(in) + t * dx;			\
	Y(new) = Y(in) + t * (Y(out) - Y(in));		\
	Z(new) = Z(in) + t * (Z(out) - Z(in));		\
	W(new) = W(in) + t * dw;

   GENERAL_CLIP

#undef OUTSIDE
#undef COMPUTE_INTERSECTION


   /*** Clip against +Y side ***/
#define OUTSIDE(K)      (Y(K) > W(K))
#define COMPUTE_INTERSECTION( new, in, out )		\
	dy = Y(out) - Y(in);				\
	dw = W(out) - W(in);				\
        t = (Y(in) - W(in)) / (dw-dy);			\
	X(new) = X(in) + t * (X(out) - X(in));		\
	Y(new) = Y(in) + t * dy;			\
	Z(new) = Z(in) + t * (Z(out) - Z(in));		\
	W(new) = W(in) + t * dw;

   GENERAL_CLIP

#undef OUTSIDE
#undef COMPUTE_INTERSECTION


   /*** Clip against -Y side ***/
#define OUTSIDE(K)      (Y(K) < -W(K))
#define COMPUTE_INTERSECTION( new, in, out )		\
        dy = Y(out) - Y(in);				\
        dw = W(out) - W(in);				\
        t = -(Y(in) + W(in)) / (dw+dy);			\
        X(new) = X(in) + t * (X(out) - X(in));		\
	Y(new) = Y(in) + t * dy;			\
	Z(new) = Z(in) + t * (Z(out) - Z(in));		\
	W(new) = W(in) + t * dw;

   GENERAL_CLIP

#undef OUTSIDE
#undef COMPUTE_INTERSECTION


   /*** Clip against +Z side ***/
#define OUTSIDE(K)      (Z(K) > W(K))
#define COMPUTE_INTERSECTION( new, in, out )		\
        dz = Z(out) - Z(in);				\
        dw = W(out) - W(in);				\
        t = (Z(in) - W(in)) / (dw-dz);			\
        X(new) = X(in) + t * (X(out) - X(in));		\
        Y(new) = Y(in) + t * (Y(out) - Y(in));		\
	Z(new) = Z(in) + t * dz;			\
	W(new) = W(in) + t * dw;

   GENERAL_CLIP

#undef OUTSIDE
#undef COMPUTE_INTERSECTION


   /*** Clip against -Z side ***/
#define OUTSIDE(K)      (Z(K) < -W(K))
#define COMPUTE_INTERSECTION( new, in, out )		\
        dz = Z(out) - Z(in);				\
        dw = W(out) - W(in);				\
        t = -(Z(in) + W(in)) / (dw+dz);			\
        X(new) = X(in) + t * (X(out) - X(in));		\
        Y(new) = Y(in) + t * (Y(out) - Y(in));		\
	Z(new) = Z(in) + t * dz;			\
	W(new) = W(in) + t * dw;

   GENERAL_CLIP

#undef OUTSIDE
#undef COMPUTE_INTERSECTION

#undef GENERAL_CLIP

   *i = ii;
   *j = jj;
   return 1;
}




/*
 * Clip a polygon against the view volume defined by -w<=x,y,z<=w.
 * Input:  n - number of vertices in input polygon.
 *         vlist - list of indexes into VB->V* of polygon to clip.
 * Output:  vlist - modified list of vertex indexes
 * Return:  number of vertices in resulting polygon
 */
GLuint gl_viewclip_polygon( GLcontext* ctx, GLuint n, GLuint vlist[] )

{
   struct vertex_buffer* VB = ctx->VB;
   GLfloat (*coord)[4] = VB->Clip;

   GLuint previ, prevj;
   GLuint curri, currj;
   GLuint vlist2[VB_SIZE];
   GLuint n2;
   GLdouble dx, dy, dz, dw, t, neww;

/*
 * We use 6 instances of this code to implement clipping against the
 * 6 sides of the view volume.  Prior to each we define the macros:
 *    INLIST = array which lists input vertices
 *    OUTLIST = array which lists output vertices
 *    INCOUNT = variable which is the number of vertices in INLIST[]
 *    OUTCOUNT = variable which is the number of vertices in OUTLIST[]
 *    INSIDE(i) = test if vertex v[i] is inside the view volume
 *    COMPUTE_INTERSECTION(in,out,new) = compute intersection of line
 *              from v[in] to v[out] with the clipping plane and store
 *              the result in v[new]
 */

#define GENERAL_CLIP                                                    \
   if (INCOUNT<3)  return 0;						\
   previ = INCOUNT-1;		/* let previous = last vertex */	\
   prevj = INLIST[previ];						\
   OUTCOUNT = 0;                                                        \
   for (curri=0;curri<INCOUNT;curri++) {				\
      currj = INLIST[curri];						\
      if (INSIDE(currj)) {						\
         if (INSIDE(prevj)) {						\
            /* both verts are inside ==> copy current to outlist */     \
	    OUTLIST[OUTCOUNT] = currj;					\
	    OUTCOUNT++;							\
         }                                                              \
         else {                                                         \
            /* current is inside and previous is outside ==> clip */	\
	    COMPUTE_INTERSECTION( currj, prevj,	VB->Free )		\
	    /* if new point not coincident with previous point... */	\
	    if (t>0.0) {						\
	       /* interpolate aux info using the value of t */		\
	       if (ctx->ClipMask)					\
		  ctx->ClipInterpAuxFunc( ctx, CLIP_SPACE, VB->Free, t, currj, prevj ); \
	       VB->Edgeflag[VB->Free] = VB->Edgeflag[prevj];		\
	       /* output new point */					\
	       OUTLIST[OUTCOUNT] = VB->Free;				\
	       VB->Free++;						\
	       if (VB->Free==VB_SIZE)   VB->Free = 1;			\
	       OUTCOUNT++;						\
	    }								\
	    /* Output current */					\
	    OUTLIST[OUTCOUNT] = currj;					\
	    OUTCOUNT++;							\
         }                                                              \
      }                                                                 \
      else {                                                            \
         if (INSIDE(prevj)) {						\
            /* current is outside and previous is inside ==> clip */	\
	    COMPUTE_INTERSECTION( prevj, currj, VB->Free )		\
	    /* if new point not coincident with previous point... */	\
	    if (t>0.0) {						\
	       /* interpolate aux info using the value of t */		\
	       if (ctx->ClipMask)					\
		  ctx->ClipInterpAuxFunc( ctx, CLIP_SPACE, VB->Free, t, prevj, currj ); \
	       VB->Edgeflag[VB->Free] = VB->Edgeflag[prevj];		\
	       /* output new point */					\
	       OUTLIST[OUTCOUNT] = VB->Free;				\
	       VB->Free++;						\
	       if (VB->Free==VB_SIZE)   VB->Free = 1;			\
	       OUTCOUNT++;						\
	    }								\
         }								\
         /* else both verts are outside ==> do nothing */		\
      }									\
      /* let previous = current */					\
      previ = curri;							\
      prevj = currj;							\
      /* check for overflowing vertex buffer */				\
      if (OUTCOUNT>=VB_SIZE-1) {					\
	 /* Too many vertices */					\
         if (OUTLIST==vlist2) {						\
	    /* copy OUTLIST[] to vlist[] */				\
	    int i;							\
	    for (i=0;i<VB_SIZE;i++) {					\
	       vlist[i] = OUTLIST[i];					\
	    }								\
	 }								\
	 return VB_SIZE-1;						\
      }									\
   }


#define X(I)	coord[I][0]
#define Y(I)	coord[I][1]
#define Z(I)	coord[I][2]
#define W(I)	coord[I][3]

/*
 * Clip against +X
 */
#define INCOUNT n
#define OUTCOUNT n2
#define INLIST vlist
#define OUTLIST vlist2
#define INSIDE(K)      (X(K) <= W(K))

#define COMPUTE_INTERSECTION( in, out, new )		\
        dx = X(out) - X(in);				\
        dw = W(out) - W(in);				\
        t = (X(in)-W(in)) / (dw-dx);			\
	neww = W(in) + t * dw;				\
	X(new) = neww;					\
	Y(new) = Y(in) + t * (Y(out) - Y(in));		\
	Z(new) = Z(in) + t * (Z(out) - Z(in)); 		\
	W(new) = neww;

   GENERAL_CLIP

#undef INCOUNT
#undef OUTCOUNT
#undef INLIST
#undef OUTLIST
#undef INSIDE
#undef COMPUTE_INTERSECTION


/*
 * Clip against -X
 */
#define INCOUNT n2
#define OUTCOUNT n
#define INLIST vlist2
#define OUTLIST vlist
#define INSIDE(K)       (X(K) >= -W(K))
#define COMPUTE_INTERSECTION( in, out, new )		\
        dx = X(out)-X(in);                      	\
        dw = W(out)-W(in);                      	\
        t = -(X(in)+W(in)) / (dw+dx);           	\
	neww = W(in) + t * dw;				\
        X(new) = -neww;					\
        Y(new) = Y(in) + t * (Y(out) - Y(in));		\
        Z(new) = Z(in) + t * (Z(out) - Z(in));		\
        W(new) = neww;

   GENERAL_CLIP

#undef INCOUNT
#undef OUTCOUNT
#undef INLIST
#undef OUTLIST
#undef INSIDE
#undef COMPUTE_INTERSECTION


/*
 * Clip against +Y
 */
#define INCOUNT n
#define OUTCOUNT n2
#define INLIST vlist
#define OUTLIST vlist2
#define INSIDE(K)       (Y(K) <= W(K))
#define COMPUTE_INTERSECTION( in, out, new )		\
        dy = Y(out)-Y(in);                      	\
        dw = W(out)-W(in);                      	\
        t = (Y(in)-W(in)) / (dw-dy);            	\
	neww = W(in) + t * dw; 				\
        X(new) = X(in) + t * (X(out) - X(in));		\
        Y(new) = neww;					\
        Z(new) = Z(in) + t * (Z(out) - Z(in));		\
        W(new) = neww;

   GENERAL_CLIP

#undef INCOUNT
#undef OUTCOUNT
#undef INLIST
#undef OUTLIST
#undef INSIDE
#undef COMPUTE_INTERSECTION


/*
 * Clip against -Y
 */
#define INCOUNT n2
#define OUTCOUNT n
#define INLIST vlist2
#define OUTLIST vlist
#define INSIDE(K)       (Y(K) >= -W(K))
#define COMPUTE_INTERSECTION( in, out, new )		\
        dy = Y(out)-Y(in);                      	\
        dw = W(out)-W(in);                      	\
        t = -(Y(in)+W(in)) / (dw+dy);           	\
	neww = W(in) + t * dw;				\
        X(new) = X(in) + t * (X(out) - X(in));		\
        Y(new) = -neww;					\
        Z(new) = Z(in) + t * (Z(out) - Z(in));		\
        W(new) = neww;

   GENERAL_CLIP

#undef INCOUNT
#undef OUTCOUNT
#undef INLIST
#undef OUTLIST
#undef INSIDE
#undef COMPUTE_INTERSECTION



/*
 * Clip against +Z
 */
#define INCOUNT n
#define OUTCOUNT n2
#define INLIST vlist
#define OUTLIST vlist2
#define INSIDE(K)       (Z(K) <= W(K))
#define COMPUTE_INTERSECTION( in, out, new )		\
        dz = Z(out)-Z(in);                      	\
        dw = W(out)-W(in);                      	\
        t = (Z(in)-W(in)) / (dw-dz);            	\
	neww = W(in) + t * dw;				\
        X(new) = X(in) + t * (X(out) - X(in));		\
        Y(new) = Y(in) + t * (Y(out) - Y(in));		\
        Z(new) = neww;					\
        W(new) = neww;

   GENERAL_CLIP

#undef INCOUNT
#undef OUTCOUNT
#undef INLIST
#undef OUTLIST
#undef INSIDE
#undef COMPUTE_INTERSECTION


/*
 * Clip against -Z
 */
#define INCOUNT n2
#define OUTCOUNT n
#define INLIST vlist2
#define OUTLIST vlist
#define INSIDE(K)       (Z(K) >= -W(K))
#define COMPUTE_INTERSECTION( in, out, new )		\
        dz = Z(out)-Z(in);                      	\
        dw = W(out)-W(in);                      	\
        t = -(Z(in)+W(in)) / (dw+dz);           	\
	neww = W(in) + t * dw;				\
        X(new) = X(in) + t * (X(out) - X(in));		\
        Y(new) = Y(in) + t * (Y(out) - Y(in));		\
        Z(new) = -neww;					\
        W(new) = neww;

   GENERAL_CLIP

#undef INCOUNT
#undef INLIST
#undef OUTLIST
#undef INSIDE
#undef COMPUTE_INTERSECTION

   /* 'OUTCOUNT' clipped vertices are now back in v[] */
   return OUTCOUNT;

#undef GENERAL_CLIP
#undef OUTCOUNT
}




/**********************************************************************/
/*         Clipping against user-defined clipping planes.             */
/**********************************************************************/



/*
 * If the dot product of the eye coordinates of a vertex with the
 * stored plane equation components is positive or zero, the vertex
 * is in with respect to that clipping plane, otherwise it is out.
 */



/*
 * Clip a point against the user clipping planes.
 * Input:  v - vertex-vector describing the point to clip.
 * Return:  0 = point was clipped
 *          1 = point not clipped
 */
GLuint gl_userclip_point( GLcontext* ctx, const GLfloat v[] )
{
   GLuint p;

   for (p=0;p<MAX_CLIP_PLANES;p++) {
      if (ctx->Transform.ClipEnabled[p]) {
	 GLfloat dot = v[0] * ctx->Transform.ClipEquation[p][0]
		     + v[1] * ctx->Transform.ClipEquation[p][1]
		     + v[2] * ctx->Transform.ClipEquation[p][2]
		     + v[3] * ctx->Transform.ClipEquation[p][3];
         if (dot < 0.0F) {
            return 0;
         }
      }
   }

   return 1;
}


#define MAGIC_NUMBER -0.8e-03F


/* Test if VB->Eye[J] is inside the clipping plane defined by A,B,C,D */
#define INSIDE( J, A, B, C, D )   				\
   ( (VB->Eye[J][0] * A + VB->Eye[J][1] * B			\
    + VB->Eye[J][2] * C + VB->Eye[J][3] * D) >= MAGIC_NUMBER )


/* Test if VB->Eye[J] is outside the clipping plane defined by A,B,C,D */
#define OUTSIDE( J, A, B, C, D )   				\
   ( (VB->Eye[J][0] * A + VB->Eye[J][1] * B			\
    + VB->Eye[J][2] * C + VB->Eye[J][3] * D) < MAGIC_NUMBER )


/*
 * Clip a line against the user clipping planes.
 * Input:  i, j - indexes into VB->V*[] of endpoints
 * Output:  i, j - indexes into VB->V*[] of (possibly clipped) endpoints
 * Return:  0 = line completely clipped
 *          1 = line is visible
 */
GLuint gl_userclip_line( GLcontext* ctx, GLuint *i, GLuint *j )
{
   struct vertex_buffer* VB = ctx->VB;

   GLuint p, ii, jj;

   ii = *i;
   jj = *j;

   for (p=0;p<MAX_CLIP_PLANES;p++) {
      if (ctx->Transform.ClipEnabled[p]) {
	 register GLfloat a, b, c, d;
	 a = ctx->Transform.ClipEquation[p][0];
	 b = ctx->Transform.ClipEquation[p][1];
	 c = ctx->Transform.ClipEquation[p][2];
	 d = ctx->Transform.ClipEquation[p][3];

         if (OUTSIDE( ii, a,b,c,d  )) {
            if (OUTSIDE( jj, a,b,c,d )) {
               /* ii and jj outside ==> quit */
               return 0;
            }
            else {
               /* ii is outside, jj is inside ==> clip */
               GLfloat dx, dy, dz, dw, t, denom;
               dx = VB->Eye[ii][0] - VB->Eye[jj][0];
               dy = VB->Eye[ii][1] - VB->Eye[jj][1];
               dz = VB->Eye[ii][2] - VB->Eye[jj][2];
               dw = VB->Eye[ii][3] - VB->Eye[jj][3];
	       denom = dx*a + dy*b + dz*c + dw*d;
	       if (denom==0.0) {
		  t = 0.0;
	       }
	       else {
		  t = -(VB->Eye[jj][0]*a+VB->Eye[jj][1]*b
		       +VB->Eye[jj][2]*c+VB->Eye[jj][3]*d) / denom;
                  if (t>1.0F)  t = 1.0F;
	       }
	       VB->Eye[VB->Free][0] = VB->Eye[jj][0] + t * dx;
	       VB->Eye[VB->Free][1] = VB->Eye[jj][1] + t * dy;
	       VB->Eye[VB->Free][2] = VB->Eye[jj][2] + t * dz;
	       VB->Eye[VB->Free][3] = VB->Eye[jj][3] + t * dw;

	       /* Interpolate colors, indexes, and/or texture coords */
	       if (ctx->ClipMask)
		  interpolate_aux( ctx, EYE_SPACE, VB->Free, t, jj, ii );

	       ii = VB->Free;
	       VB->Free++;
	       if (VB->Free==VB_SIZE)   VB->Free = 1;
            }
         }
         else {
            if (OUTSIDE( jj, a,b,c,d )) {
               /* ii is inside, jj is outside ==> clip */
               GLfloat dx, dy, dz, dw, t, denom;
               dx = VB->Eye[jj][0] - VB->Eye[ii][0];
               dy = VB->Eye[jj][1] - VB->Eye[ii][1];
               dz = VB->Eye[jj][2] - VB->Eye[ii][2];
               dw = VB->Eye[jj][3] - VB->Eye[ii][3];
	       denom = dx*a + dy*b + dz*c + dw*d;
	       if (denom==0.0) {
		  t = 0.0;
	       }
	       else {
		  t = -(VB->Eye[ii][0]*a+VB->Eye[ii][1]*b
		       +VB->Eye[ii][2]*c+VB->Eye[ii][3]*d) / denom;
                  if (t>1.0F)  t = 1.0F;
	       }
	       VB->Eye[VB->Free][0] = VB->Eye[ii][0] + t * dx;
	       VB->Eye[VB->Free][1] = VB->Eye[ii][1] + t * dy;
	       VB->Eye[VB->Free][2] = VB->Eye[ii][2] + t * dz;
	       VB->Eye[VB->Free][3] = VB->Eye[ii][3] + t * dw;

	       /* Interpolate colors, indexes, and/or texture coords */
	       if (ctx->ClipMask)
		  interpolate_aux( ctx, EYE_SPACE, VB->Free, t, ii, jj );

	       jj = VB->Free;
	       VB->Free++;
	       if (VB->Free==VB_SIZE)   VB->Free = 1;
            }
            else {
               /* ii and jj inside ==> do nothing */
            }
         }
      }
   }

   *i = ii;
   *j = jj;
   return 1;
}




/*
 * Clip a polygon against the user clipping planes defined in eye coordinates.
 * Input:  n - number of vertices.
 *         vlist - list of vertices in input polygon.
 * Output:  vlist - list of vertices in output polygon.
 * Return:  number of vertices after clipping.
 */
GLuint gl_userclip_polygon( GLcontext* ctx, GLuint n, GLuint vlist[] )
{
   struct vertex_buffer* VB = ctx->VB;

   GLuint vlist2[VB_SIZE];
   GLuint *inlist, *outlist;
   GLuint incount, outcount;
   GLuint curri, currj;
   GLuint previ, prevj;
   GLuint p;

   /* initialize input vertex list */
   incount = n;
   inlist = vlist;
   outlist = vlist2;

   for (p=0;p<MAX_CLIP_PLANES;p++) {
      if (ctx->Transform.ClipEnabled[p]) {
	 register float a = ctx->Transform.ClipEquation[p][0];
	 register float b = ctx->Transform.ClipEquation[p][1];
	 register float c = ctx->Transform.ClipEquation[p][2];
	 register float d = ctx->Transform.ClipEquation[p][3];

	 if (incount<3)  return 0;

	 /* initialize prev to be last in the input list */
	 previ = incount - 1;
	 prevj = inlist[previ];

         outcount = 0;

         for (curri=0;curri<incount;curri++) {
	    currj = inlist[curri];

            if (INSIDE(currj, a,b,c,d)) {
               if (INSIDE(prevj, a,b,c,d)) {
                  /* both verts are inside ==> copy current to outlist */
		  outlist[outcount++] = currj;
               }
               else {
                  /* current is inside and previous is outside ==> clip */
                  GLfloat dx, dy, dz, dw, t, denom;
		  /* compute t */
		  dx = VB->Eye[prevj][0] - VB->Eye[currj][0];
		  dy = VB->Eye[prevj][1] - VB->Eye[currj][1];
		  dz = VB->Eye[prevj][2] - VB->Eye[currj][2];
		  dw = VB->Eye[prevj][3] - VB->Eye[currj][3];
		  denom = dx*a + dy*b + dz*c + dw*d;
		  if (denom==0.0) {
		     t = 0.0;
		  }
		  else {
		     t = -(VB->Eye[currj][0]*a+VB->Eye[currj][1]*b
		       +VB->Eye[currj][2]*c+VB->Eye[currj][3]*d) / denom;
                     if (t>1.0F) {
                        t = 1.0F;
                     }
		  }
		  /* interpolate new vertex position */
		  VB->Eye[VB->Free][0] = VB->Eye[currj][0] + t*dx;
		  VB->Eye[VB->Free][1] = VB->Eye[currj][1] + t*dy;
		  VB->Eye[VB->Free][2] = VB->Eye[currj][2] + t*dz;
		  VB->Eye[VB->Free][3] = VB->Eye[currj][3] + t*dw;

		  /* interpolate color, index, and/or texture coord */
		  if (ctx->ClipMask) {
		     interpolate_aux( ctx, EYE_SPACE, VB->Free, t, currj, prevj);
		  }
		  VB->Edgeflag[VB->Free] = VB->Edgeflag[prevj];

		  /* output new vertex */
		  outlist[outcount++] = VB->Free;
		  VB->Free++;
		  if (VB->Free==VB_SIZE)   VB->Free = 1;
		  /* output current vertex */
		  outlist[outcount++] = currj;
               }
            }
            else {
               if (INSIDE(prevj, a,b,c,d)) {
                  /* current is outside and previous is inside ==> clip */
                  GLfloat dx, dy, dz, dw, t, denom;
		  /* compute t */
                  dx = VB->Eye[currj][0]-VB->Eye[prevj][0];
                  dy = VB->Eye[currj][1]-VB->Eye[prevj][1];
                  dz = VB->Eye[currj][2]-VB->Eye[prevj][2];
                  dw = VB->Eye[currj][3]-VB->Eye[prevj][3];
		  denom = dx*a + dy*b + dz*c + dw*d;
		  if (denom==0.0) {
		     t = 0.0;
		  }
		  else {
		     t = -(VB->Eye[prevj][0]*a+VB->Eye[prevj][1]*b
		       +VB->Eye[prevj][2]*c+VB->Eye[prevj][3]*d) / denom;
                     if (t>1.0F) {
                        t = 1.0F;
                     }
		  }
		  /* interpolate new vertex position */
		  VB->Eye[VB->Free][0] = VB->Eye[prevj][0] + t*dx;
		  VB->Eye[VB->Free][1] = VB->Eye[prevj][1] + t*dy;
		  VB->Eye[VB->Free][2] = VB->Eye[prevj][2] + t*dz;
		  VB->Eye[VB->Free][3] = VB->Eye[prevj][3] + t*dw;

		  /* interpolate color, index, and/or texture coord */
		  if (ctx->ClipMask) {
		     interpolate_aux( ctx, EYE_SPACE, VB->Free, t, prevj, currj);
		  }
		  VB->Edgeflag[VB->Free] = VB->Edgeflag[prevj];

		  /* output new vertex */
		  outlist[outcount++] = VB->Free;
		  VB->Free++;
		  if (VB->Free==VB_SIZE)   VB->Free = 1;
	       }
               /* else  both verts are outside ==> do nothing */
            }

	    previ = curri;
	    prevj = currj;

	    /* check for overflowing vertex buffer */
            if (outcount>=VB_SIZE-1) {
               /* Too many vertices */
               if (outlist!=vlist2) {
                  MEMCPY( vlist, vlist2, outcount * sizeof(GLuint) );
               }
               return VB_SIZE-1;
            }

         }  /* for i */

         /* swap inlist and outlist pointers */
         {
            GLuint *tmp;
            tmp = inlist;
            inlist = outlist;
            outlist = tmp;
            incount = outcount;
         }

      } /* if */
   } /* for p */

   /* outlist points to the list of vertices resulting from the last */
   /* clipping.  If outlist == vlist2 then we have to copy the vertices */
   /* back to vlist */
   if (outlist!=vlist2) {
      MEMCPY( vlist, vlist2, outcount * sizeof(GLuint) );
   }

   return outcount;
}

