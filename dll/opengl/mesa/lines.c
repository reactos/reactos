/* $Id: lines.c,v 1.19 1998/02/03 23:46:00 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.4
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
 * $Log: lines.c,v $
 * Revision 1.19  1998/02/03 23:46:00  brianp
 * fixed a few problems with condition expressions for Amiga StormC compiler
 *
 * Revision 1.18  1997/07/24 01:24:11  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.17  1997/07/05 16:03:51  brianp
 * fixed PB overflow problem
 *
 * Revision 1.16  1997/06/20 02:01:49  brianp
 * changed color components from GLfixed to GLubyte
 *
 * Revision 1.15  1997/06/03 01:38:22  brianp
 * fixed divide by zero problem in feedback function (William Mitchell)
 *
 * Revision 1.14  1997/05/28 03:25:26  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.13  1997/05/03 00:51:02  brianp
 * removed calls to gl_texturing_enabled()
 *
 * Revision 1.12  1997/04/14 02:00:39  brianp
 * #include "texstate.h" instead of "texture.h"
 *
 * Revision 1.11  1997/04/12 12:25:01  brianp
 * replaced ctx->LineFunc with ctx->Driver.LineFunc, fixed PB->count bug
 *
 * Revision 1.10  1997/03/16 02:07:31  brianp
 * now use linetemp.h in line drawing functions
 *
 * Revision 1.9  1997/03/08 02:04:27  brianp
 * better implementation of feedback function
 *
 * Revision 1.8  1997/02/09 18:44:20  brianp
 * added GL_EXT_texture3D support
 *
 * Revision 1.7  1997/01/09 19:48:00  brianp
 * now call gl_texturing_enabled()
 *
 * Revision 1.6  1996/11/08 02:21:21  brianp
 * added null drawing function for GL_NO_RASTER
 *
 * Revision 1.5  1996/09/27 01:28:56  brianp
 * removed unused variables
 *
 * Revision 1.4  1996/09/25 02:01:54  brianp
 * new texture coord interpolation
 *
 * Revision 1.3  1996/09/15 14:18:10  brianp
 * now use GLframebuffer and GLvisual
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
#include "context.h"
#include "depth.h"
#include "feedback.h"
#include "lines.h"
#include "dlist.h"
#include "macros.h"
#include "pb.h"
#include "texstate.h"
#include "types.h"
#include "vb.h"
#include <wine/debug.h>
#endif

WINE_DEFAULT_DEBUG_CHANNEL(opengl32);

void gl_LineWidth( GLcontext *ctx, GLfloat width )
{
   if (width<=0.0) {
      gl_error( ctx, GL_INVALID_VALUE, "glLineWidth" );
      return;
   }
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glLineWidth" );
      return;
   }
   ctx->Line.Width = width;
   ctx->NewState |= NEW_RASTER_OPS;
}



void gl_LineStipple( GLcontext *ctx, GLint factor, GLushort pattern )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glLineStipple" );
      return;
   }
   ctx->Line.StippleFactor = CLAMP( factor, 1, 256 );
   ctx->Line.StipplePattern = pattern;
   ctx->NewState |= NEW_RASTER_OPS;
}



/**********************************************************************/
/*****                    Rasterization                           *****/
/**********************************************************************/


/*
 * There are 4 pairs (RGBA, CI) of line drawing functions:
 *   1. simple:  width=1 and no special rasterization functions (fastest)
 *   2. flat:  width=1, non-stippled, flat-shaded, any raster operations
 *   3. smooth:  width=1, non-stippled, smooth-shaded, any raster operations
 *   4. general:  any other kind of line (slowest)
 */


/*
 * All line drawing functions have the same arguments:
 * v1, v2 - indexes of first and second endpoints into vertex buffer arrays
 * pv     - provoking vertex: which vertex color/index to use for flat shading.
 */



static void feedback_line( GLcontext *ctx, GLuint v1, GLuint v2, GLuint pv )
{
   struct vertex_buffer *VB = ctx->VB;
   GLfloat x1, y1, z1, w1;
   GLfloat x2, y2, z2, w2;
   GLfloat tex1[4], tex2[4], invq;
   GLfloat invRedScale   = ctx->Visual->InvRedScale;
   GLfloat invGreenScale = ctx->Visual->InvGreenScale;
   GLfloat invBlueScale  = ctx->Visual->InvBlueScale;
   GLfloat invAlphaScale = ctx->Visual->InvAlphaScale;

   x1 = VB->Win[v1][0];
   y1 = VB->Win[v1][1];
   z1 = VB->Win[v1][2] / DEPTH_SCALE;
   w1 = VB->Clip[v1][3];

   x2 = VB->Win[v2][0];
   y2 = VB->Win[v2][1];
   z2 = VB->Win[v2][2] / DEPTH_SCALE;
   w2 = VB->Clip[v2][3];

   invq = (VB->TexCoord[v1][3]==0.0) ? 1.0 : (1.0F / VB->TexCoord[v1][3]);
   tex1[0] = VB->TexCoord[v1][0] * invq;
   tex1[1] = VB->TexCoord[v1][1] * invq;
   tex1[2] = VB->TexCoord[v1][2] * invq;
   tex1[3] = VB->TexCoord[v1][3];
   invq = (VB->TexCoord[v2][3]==0.0) ? 1.0 : (1.0F / VB->TexCoord[v2][3]);
   tex2[0] = VB->TexCoord[v2][0] * invq;
   tex2[1] = VB->TexCoord[v2][1] * invq;
   tex2[2] = VB->TexCoord[v2][2] * invq;
   tex2[3] = VB->TexCoord[v2][3];

   if (ctx->StippleCounter==0) {
      FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_LINE_RESET_TOKEN );
   }
   else {
      FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_LINE_TOKEN );
   }

   {
      GLfloat color[4];
      /* convert color from integer to a float in [0,1] */
      color[0] = (GLfloat) VB->Color[pv][0] * invRedScale;
      color[1] = (GLfloat) VB->Color[pv][1] * invGreenScale;
      color[2] = (GLfloat) VB->Color[pv][2] * invBlueScale;
      color[3] = (GLfloat) VB->Color[pv][3] * invAlphaScale;
      gl_feedback_vertex( ctx, x1,y1,z1,w1, color,
                          (GLfloat) VB->Index[pv], tex1 );
      gl_feedback_vertex( ctx, x2,y2,z2,w2, color,
                          (GLfloat) VB->Index[pv], tex2 );
   }

   ctx->StippleCounter++;
}



static void select_line( GLcontext *ctx, GLuint v1, GLuint v2, GLuint pv )
{
   gl_update_hitflag( ctx, ctx->VB->Win[v1][2] / DEPTH_SCALE );
   gl_update_hitflag( ctx, ctx->VB->Win[v2][2] / DEPTH_SCALE );
}



#if MAX_WIDTH > MAX_HEIGHT
#  define MAXPOINTS MAX_WIDTH
#else
#  define MAXPOINTS MAX_HEIGHT
#endif


/* Flat, color index line */
static void flat_ci_line( GLcontext *ctx,
                          GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   PB_SET_INDEX( ctx, ctx->PB, ctx->VB->Index[pvert] );
   count = ctx->PB->count;

#define INTERP_XY 1

#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	count++;

#include "linetemp.h"

   ctx->PB->count = count;
   PB_CHECK_FLUSH( ctx, ctx->PB );
}



/* Flat, color index line with Z interpolation/testing */
static void flat_ci_z_line( GLcontext *ctx,
                            GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLdepth *pbz = ctx->PB->z;
   PB_SET_INDEX( ctx, ctx->PB, ctx->VB->Index[pvert] );
   count = ctx->PB->count;

#define INTERP_XY 1
#define INTERP_Z 1

#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	pbz[count] = Z;		\
	count++;

#include "linetemp.h"

   ctx->PB->count = count;
   PB_CHECK_FLUSH( ctx, ctx->PB );
}



/* Flat-shaded, RGBA line */
static void flat_rgba_line( GLcontext *ctx,
                            GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLubyte *color = ctx->VB->Color[pvert];
   PB_SET_COLOR( ctx, ctx->PB, color[0], color[1], color[2], color[3] );
   count = ctx->PB->count;

#define INTERP_XY 1

#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	count++;

#include "linetemp.h"

   ctx->PB->count = count;
   PB_CHECK_FLUSH( ctx, ctx->PB );
}



/* Flat-shaded, RGBA line with Z interpolation/testing */
static void flat_rgba_z_line( GLcontext *ctx,
                              GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLdepth *pbz = ctx->PB->z;
   GLubyte *color = ctx->VB->Color[pvert];
   PB_SET_COLOR( ctx, ctx->PB, color[0], color[1], color[2], color[3] );
   count = ctx->PB->count;

#define INTERP_XY 1
#define INTERP_Z 1

#define PLOT(X,Y)	\
	pbx[count] = X;	\
	pby[count] = Y;	\
	pbz[count] = Z;	\
	count++;

#include "linetemp.h"

   ctx->PB->count = count;
   PB_CHECK_FLUSH( ctx, ctx->PB );
}



/* Smooth shaded, color index line */
static void smooth_ci_line( GLcontext *ctx,
                            GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count = ctx->PB->count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLuint *pbi = ctx->PB->i;

#define INTERP_XY 1
#define INTERP_INDEX 1

#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	pbi[count] = I;		\
	count++;

#include "linetemp.h"

   ctx->PB->count = count;
   PB_CHECK_FLUSH( ctx, ctx->PB );
}



/* Smooth shaded, color index line with Z interpolation/testing */
static void smooth_ci_z_line( GLcontext *ctx,
                              GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count = ctx->PB->count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLdepth *pbz = ctx->PB->z;
   GLuint *pbi = ctx->PB->i;

#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_INDEX 1

#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	pbz[count] = Z;		\
	pbi[count] = I;		\
	count++;

#include "linetemp.h"

   ctx->PB->count = count;
   PB_CHECK_FLUSH( ctx, ctx->PB );
}



/* Smooth-shaded, RGBA line */
static void smooth_rgba_line( GLcontext *ctx,
                       	      GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count = ctx->PB->count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLubyte *pbr = ctx->PB->r;
   GLubyte *pbg = ctx->PB->g;
   GLubyte *pbb = ctx->PB->b;
   GLubyte *pba = ctx->PB->a;

#define INTERP_XY 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1

#define PLOT(X,Y)			\
	pbx[count] = X;			\
	pby[count] = Y;			\
	pbr[count] = FixedToInt(r0);	\
	pbg[count] = FixedToInt(g0);	\
	pbb[count] = FixedToInt(b0);	\
	pba[count] = FixedToInt(a0);	\
	count++;

#include "linetemp.h"

   ctx->PB->count = count;
   PB_CHECK_FLUSH( ctx, ctx->PB );
}



/* Smooth-shaded, RGBA line with Z interpolation/testing */
static void smooth_rgba_z_line( GLcontext *ctx,
                       	        GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count = ctx->PB->count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLdepth *pbz = ctx->PB->z;
   GLubyte *pbr = ctx->PB->r;
   GLubyte *pbg = ctx->PB->g;
   GLubyte *pbb = ctx->PB->b;
   GLubyte *pba = ctx->PB->a;

#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1

#define PLOT(X,Y)			\
	pbx[count] = X;			\
	pby[count] = Y;			\
	pbz[count] = Z;			\
	pbr[count] = FixedToInt(r0);	\
	pbg[count] = FixedToInt(g0);	\
	pbb[count] = FixedToInt(b0);	\
	pba[count] = FixedToInt(a0);	\
	count++;

#include "linetemp.h"

   ctx->PB->count = count;
   PB_CHECK_FLUSH( ctx, ctx->PB );
}


#define CHECK_FULL(count)			\
	if (count >= PB_SIZE-MAX_WIDTH) {	\
	   ctx->PB->count = count;		\
	   gl_flush_pb(ctx);			\
	   count = ctx->PB->count;		\
	}



/* Smooth shaded, color index, any width, maybe stippled */
static void general_smooth_ci_line( GLcontext *ctx,
                           	    GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count = ctx->PB->count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLdepth *pbz = ctx->PB->z;
   GLuint *pbi = ctx->PB->i;

   if (ctx->Line.StippleFlag) {
      /* stippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_INDEX 1
#define WIDE 1
#define STIPPLE 1
#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	pbz[count] = Z;		\
	pbi[count] = I;		\
	count++;		\
	CHECK_FULL(count);
#include "linetemp.h"
   }
   else {
      /* unstippled */
      if (ctx->Line.Width==2.0F) {
         /* special case: unstippled and width=2 */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_INDEX 1
#define XMAJOR_PLOT(X,Y)			\
	pbx[count] = X;  pbx[count+1] = X;	\
	pby[count] = Y;  pby[count+1] = Y+1;	\
	pbz[count] = Z;  pbz[count+1] = Z;	\
	pbi[count] = I;  pbi[count+1] = I;	\
	count += 2;
#define YMAJOR_PLOT(X,Y)			\
	pbx[count] = X;  pbx[count+1] = X+1;	\
	pby[count] = Y;  pby[count+1] = Y;	\
	pbz[count] = Z;  pbz[count+1] = Z;	\
	pbi[count] = I;  pbi[count+1] = I;	\
	count += 2;
#include "linetemp.h"
      }
      else {
         /* unstippled, any width */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_INDEX 1
#define WIDE 1
#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	pbz[count] = Z;		\
	pbi[count] = I;		\
	count++;		\
	CHECK_FULL(count);
#include "linetemp.h"
      }
   }

   ctx->PB->count = count;
   PB_CHECK_FLUSH( ctx, ctx->PB );
}


/* Flat shaded, color index, any width, maybe stippled */
static void general_flat_ci_line( GLcontext *ctx,
                                  GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLdepth *pbz = ctx->PB->z;
   PB_SET_INDEX( ctx, ctx->PB, ctx->VB->Index[pvert] );
   count = ctx->PB->count;

   if (ctx->Line.StippleFlag) {
      /* stippled, any width */
#define INTERP_XY 1
#define INTERP_Z 1
#define WIDE 1
#define STIPPLE 1
#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	pbz[count] = Z;		\
	count++;		\
	CHECK_FULL(count);
#include "linetemp.h"
   }
   else {
      /* unstippled */
      if (ctx->Line.Width==2.0F) {
         /* special case: unstippled and width=2 */
#define INTERP_XY 1
#define INTERP_Z 1
#define XMAJOR_PLOT(X,Y)			\
	pbx[count] = X;  pbx[count+1] = X;	\
	pby[count] = Y;  pby[count+1] = Y+1;	\
	pbz[count] = Z;  pbz[count+1] = Z;	\
	count += 2;
#define YMAJOR_PLOT(X,Y)			\
	pbx[count] = X;  pbx[count+1] = X+1;	\
	pby[count] = Y;  pby[count+1] = Y;	\
	pbz[count] = Z;  pbz[count+1] = Z;	\
	count += 2;
#include "linetemp.h"
      }
      else {
         /* unstippled, any width */
#define INTERP_XY 1
#define INTERP_Z 1
#define WIDE 1
#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	pbz[count] = Z;		\
	count++;		\
	CHECK_FULL(count);
#include "linetemp.h"
      }
   }

   ctx->PB->count = count;
   PB_CHECK_FLUSH( ctx, ctx->PB );
}



static void general_smooth_rgba_line( GLcontext *ctx,
                                      GLuint vert0, GLuint vert1, GLuint pvert)
{
   GLint count = ctx->PB->count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLdepth *pbz = ctx->PB->z;
   GLubyte *pbr = ctx->PB->r;
   GLubyte *pbg = ctx->PB->g;
   GLubyte *pbb = ctx->PB->b;
   GLubyte *pba = ctx->PB->a;

   TRACE("Line %3.1f, %3.1f, %3.1f (r%u, g%u, b%u) --> %3.1f, %3.1f, %3.1f (r%u, g%u, b%u)\n",
           ctx->VB->Win[vert0][0], ctx->VB->Win[vert0][1], ctx->VB->Win[vert0][2], ctx->VB->Color[vert0][0], ctx->VB->Color[vert0][1], ctx->VB->Color[vert0][2],
           ctx->VB->Win[vert1][0], ctx->VB->Win[vert1][1], ctx->VB->Win[vert1][2], ctx->VB->Color[vert1][0], ctx->VB->Color[vert1][1], ctx->VB->Color[vert1][2]);

   if (ctx->Line.StippleFlag) {
      /* stippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define WIDE 1
#define STIPPLE 1
#define PLOT(X,Y)			\
	pbx[count] = X;			\
	pby[count] = Y;			\
	pbz[count] = Z;			\
	pbr[count] = FixedToInt(r0);	\
	pbg[count] = FixedToInt(g0);	\
	pbb[count] = FixedToInt(b0);	\
	pba[count] = FixedToInt(a0);	\
	count++;			\
	CHECK_FULL(count);
#include "linetemp.h"
   }
   else {
      /* unstippled */
      if (ctx->Line.Width==2.0F) {
         /* special case: unstippled and width=2 */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define XMAJOR_PLOT(X,Y)						\
	pbx[count] = X;  pbx[count+1] = X;				\
	pby[count] = Y;  pby[count+1] = Y+1;				\
	pbz[count] = Z;  pbz[count+1] = Z;				\
	pbr[count] = FixedToInt(r0);  pbr[count+1] = FixedToInt(r0);	\
	pbg[count] = FixedToInt(g0);  pbg[count+1] = FixedToInt(g0);	\
	pbb[count] = FixedToInt(b0);  pbb[count+1] = FixedToInt(b0);	\
	pba[count] = FixedToInt(a0);  pba[count+1] = FixedToInt(a0);	\
	count += 2;
#define YMAJOR_PLOT(X,Y)						\
	pbx[count] = X;  pbx[count+1] = X+1;				\
	pby[count] = Y;  pby[count+1] = Y;				\
	pbz[count] = Z;  pbz[count+1] = Z;				\
	pbr[count] = FixedToInt(r0);  pbr[count+1] = FixedToInt(r0);	\
	pbg[count] = FixedToInt(g0);  pbg[count+1] = FixedToInt(g0);	\
	pbb[count] = FixedToInt(b0);  pbb[count+1] = FixedToInt(b0);	\
	pba[count] = FixedToInt(a0);  pba[count+1] = FixedToInt(a0);	\
	count += 2;
#include "linetemp.h"
      }
      else {
         /* unstippled, any width */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define WIDE 1
#define PLOT(X,Y)			\
	pbx[count] = X;			\
	pby[count] = Y;			\
	pbz[count] = Z;			\
	pbr[count] = FixedToInt(r0);	\
	pbg[count] = FixedToInt(g0);	\
	pbb[count] = FixedToInt(b0);	\
	pba[count] = FixedToInt(a0);	\
	count++;			\
	CHECK_FULL(count);
#include "linetemp.h"
      }
   }

   ctx->PB->count = count;
   PB_CHECK_FLUSH( ctx, ctx->PB );
}


static void general_flat_rgba_line( GLcontext *ctx,
                                    GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLdepth *pbz = ctx->PB->z;
   GLubyte *color = ctx->VB->Color[pvert];
   PB_SET_COLOR( ctx, ctx->PB, color[0], color[1], color[2], color[3] );
   count = ctx->PB->count;

   if (ctx->Line.StippleFlag) {
      /* stippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define WIDE 1
#define STIPPLE 1
#define PLOT(X,Y)			\
	pbx[count] = X;			\
	pby[count] = Y;			\
	pbz[count] = Z;			\
	count++;			\
	CHECK_FULL(count);
#include "linetemp.h"
   }
   else {
      /* unstippled */
      if (ctx->Line.Width==2.0F) {
         /* special case: unstippled and width=2 */
#define INTERP_XY 1
#define INTERP_Z 1
#define XMAJOR_PLOT(X,Y)			\
	pbx[count] = X;  pbx[count+1] = X;	\
	pby[count] = Y;  pby[count+1] = Y+1;	\
	pbz[count] = Z;  pbz[count+1] = Z;	\
	count += 2;
#define YMAJOR_PLOT(X,Y)			\
	pbx[count] = X;  pbx[count+1] = X+1;	\
	pby[count] = Y;  pby[count+1] = Y;	\
	pbz[count] = Z;  pbz[count+1] = Z;	\
	count += 2;
#include "linetemp.h"
      }
      else {
         /* unstippled, any width */
#define INTERP_XY 1
#define INTERP_Z 1
#define WIDE 1
#define PLOT(X,Y)			\
	pbx[count] = X;			\
	pby[count] = Y;			\
	pbz[count] = Z;			\
	count++;			\
	CHECK_FULL(count);
#include "linetemp.h"
      }
   }

   ctx->PB->count = count;
   PB_CHECK_FLUSH( ctx, ctx->PB );
}



/* Flat-shaded, textured, any width, maybe stippled */
static void flat_textured_line( GLcontext *ctx,
                                GLuint vert0, GLuint vert1, GLuint pv )
{
   GLint count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLdepth *pbz = ctx->PB->z;
   GLfloat *pbs = ctx->PB->s;
   GLfloat *pbt = ctx->PB->t;
   GLfloat *pbu = ctx->PB->u;
   GLubyte *color = ctx->VB->Color[pv];
   PB_SET_COLOR( ctx, ctx->PB, color[0], color[1], color[2], color[3] );
   count = ctx->PB->count;

   if (ctx->Line.StippleFlag) {
      /* stippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_STW 1
#define INTERP_UV 1
#define WIDE 1
#define STIPPLE 1
#define PLOT(X,Y)			\
	pbx[count] = X;			\
	pby[count] = Y;			\
	pbz[count] = Z;			\
	pbs[count] = s0 / w0;		\
	pbt[count] = t0 / w0;		\
	pbu[count] = u0 / w0;		\
	count++;			\
	CHECK_FULL(count);
#include "linetemp.h"
   }
   else {
      /* unstippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_STW 1
#define INTERP_UV 1
#define WIDE 1
#define PLOT(X,Y)			\
	pbx[count] = X;			\
	pby[count] = Y;			\
	pbz[count] = Z;			\
	pbs[count] = s0 / w0;		\
	pbt[count] = t0 / w0;		\
	pbu[count] = u0 / w0;		\
	count++;			\
	CHECK_FULL(count);
#include "linetemp.h"
   }

   ctx->PB->count = count;
   PB_CHECK_FLUSH( ctx, ctx->PB );
}



/* Smooth-shaded, textured, any width, maybe stippled */
static void smooth_textured_line( GLcontext *ctx,
                                  GLuint vert0, GLuint vert1, GLuint pv )
{
   GLint count = ctx->PB->count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLdepth *pbz = ctx->PB->z;
   GLfloat *pbs = ctx->PB->s;
   GLfloat *pbt = ctx->PB->t;
   GLfloat *pbu = ctx->PB->u;
   GLubyte *pbr = ctx->PB->r;
   GLubyte *pbg = ctx->PB->g;
   GLubyte *pbb = ctx->PB->b;
   GLubyte *pba = ctx->PB->a;

   if (ctx->Line.StippleFlag) {
      /* stippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_STW 1
#define INTERP_UV 1
#define WIDE 1
#define STIPPLE 1
#define PLOT(X,Y)			\
	pbx[count] = X;			\
	pby[count] = Y;			\
	pbz[count] = Z;			\
	pbs[count] = s0 / w0;		\
	pbt[count] = t0 / w0;		\
	pbu[count] = u0 / w0;		\
	pbr[count] = FixedToInt(r0);	\
	pbg[count] = FixedToInt(g0);	\
	pbb[count] = FixedToInt(b0);	\
	pba[count] = FixedToInt(a0);	\
	count++;			\
	CHECK_FULL(count);
#include "linetemp.h"
   }
   else {
      /* unstippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_STW 1
#define INTERP_UV 1
#define WIDE 1
#define PLOT(X,Y)			\
	pbx[count] = X;			\
	pby[count] = Y;			\
	pbz[count] = Z;			\
	pbs[count] = s0 / w0;		\
	pbt[count] = t0 / w0;		\
	pbu[count] = u0 / w0;		\
	pbr[count] = FixedToInt(r0);	\
	pbg[count] = FixedToInt(g0);	\
	pbb[count] = FixedToInt(b0);	\
	pba[count] = FixedToInt(a0);	\
	count++;			\
	CHECK_FULL(count);
#include "linetemp.h"
   }

   ctx->PB->count = count;
   PB_CHECK_FLUSH( ctx, ctx->PB );
}



/*
 * Null rasterizer for measuring transformation speed.
 */
static void null_line( GLcontext *ctx, GLuint v1, GLuint v2, GLuint pv )
{
}



/*
 * Determine which line drawing function to use given the current
 * rendering context.
 */
void gl_set_line_function( GLcontext *ctx )
{
   GLboolean rgbmode = ctx->Visual->RGBAflag;
   /* TODO: antialiased lines */

   if (ctx->RenderMode==GL_RENDER) {
      if (ctx->NoRaster) {
         ctx->Driver.LineFunc = null_line;
         return;
      }
      if (ctx->Driver.LineFunc) {
         /* Device driver will draw lines. */
         ctx->Driver.LineFunc = ctx->Driver.LineFunc;
      }
      else if (ctx->Texture.Enabled) {
         if (ctx->Light.ShadeModel==GL_SMOOTH) {
            ctx->Driver.LineFunc = smooth_textured_line;
         }
         else {
            ctx->Driver.LineFunc = flat_textured_line;
         }
      }
      else if (ctx->Line.Width!=1.0 || ctx->Line.StippleFlag
               || ctx->Line.SmoothFlag || ctx->Texture.Enabled) {
         if (ctx->Light.ShadeModel==GL_SMOOTH) {
            if (rgbmode)
               ctx->Driver.LineFunc = general_smooth_rgba_line;
            else
               ctx->Driver.LineFunc = general_smooth_ci_line;
         }
         else {
            if (rgbmode)
               ctx->Driver.LineFunc = general_flat_rgba_line;
            else
               ctx->Driver.LineFunc = general_flat_ci_line;
         }
      }
      else {
	 if (ctx->Light.ShadeModel==GL_SMOOTH) {
	    /* Width==1, non-stippled, smooth-shaded */
            if (ctx->Depth.Test
	        || (ctx->Fog.Enabled && ctx->Hint.Fog==GL_NICEST)) {
               if (rgbmode)
                  ctx->Driver.LineFunc = smooth_rgba_z_line;
               else
                  ctx->Driver.LineFunc = smooth_ci_z_line;
            }
            else {
               if (rgbmode)
                  ctx->Driver.LineFunc = smooth_rgba_line;
               else
                  ctx->Driver.LineFunc = smooth_ci_line;
            }
	 }
         else {
	    /* Width==1, non-stippled, flat-shaded */
            if (ctx->Depth.Test
                || (ctx->Fog.Enabled && ctx->Hint.Fog==GL_NICEST)) {
               if (rgbmode)
                  ctx->Driver.LineFunc = flat_rgba_z_line;
               else
                  ctx->Driver.LineFunc = flat_ci_z_line;
            }
            else {
               if (rgbmode)
                  ctx->Driver.LineFunc = flat_rgba_line;
               else
                  ctx->Driver.LineFunc = flat_ci_line;
            }
         }
      }
   }
   else if (ctx->RenderMode==GL_FEEDBACK) {
      ctx->Driver.LineFunc = feedback_line;
   }
   else {
      /* GL_SELECT mode */
      ctx->Driver.LineFunc = select_line;
   }
}

