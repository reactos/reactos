/* $Id: vbrender.c,v 1.21 1998/01/18 15:04:48 brianp Exp $ */

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
 * $Log: vbrender.c,v $
 * Revision 1.21  1998/01/18 15:04:48  brianp
 * fixed clipmask bug in gl_reset_vb() (reported by Michael Callahan)
 *
 * Revision 1.20  1998/01/09 02:49:33  brianp
 * cleaned-up gl_reset_vb(), added small GL_POLYGON optimization
 *
 * Revision 1.19  1997/12/29 23:49:57  brianp
 * added a call to gl_update_lighting() in gl_reset_vb() to fix material bug
 *
 * Revision 1.18  1997/12/19 03:36:42  brianp
 * check bit-wise AND of vertex clip masks to cull polygons sooner
 *
 * Revision 1.17  1997/12/09 02:56:57  brianp
 * in render_clipped_polygon() only recompute window coords for new verts
 *
 * Revision 1.16  1997/11/20 00:00:47  brianp
 * only call Driver.RasterSetup() once in render_clipped_polygon()
 *
 * Revision 1.15  1997/09/18 01:32:47  brianp
 * fixed divide by zero problem for "weird" projection matrices
 *
 * Revision 1.14  1997/08/13 01:31:41  brianp
 * cleaned up code involving LightTwoSide
 *
 * Revision 1.13  1997/07/24 01:25:27  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.12  1997/07/11 02:19:52  brianp
 * flat-shaded quads in a strip were miscolored if clipped (Randy Frank)
 *
 * Revision 1.11  1997/05/28 03:26:49  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.10  1997/05/16 02:09:26  brianp
 * clipped GL_TRIANGLE_STRIP triangles sometimes got wrong provoking vertex
 *
 * Revision 1.9  1997/04/30 02:20:00  brianp
 * fixed a line clipping bug in GL_LINE_LOOPs
 *
 * Revision 1.8  1997/04/29 01:31:07  brianp
 * added RasterSetup() function to device driver
 *
 * Revision 1.7  1997/04/24 00:30:17  brianp
 * optimized glTexCoord2() code
 *
 * Revision 1.6  1997/04/20 19:47:06  brianp
 * fixed an error message, added a comment
 *
 * Revision 1.5  1997/04/20 15:59:30  brianp
 * removed VERTEX2_BIT stuff
 *
 * Revision 1.4  1997/04/20 15:27:34  brianp
 * removed odd_flag from all polygon rendering functions
 *
 * Revision 1.3  1997/04/12 12:26:06  brianp
 * now directly call ctx->Driver.Points/Line/Triangle/QuadFunc
 *
 * Revision 1.2  1997/04/07 03:01:11  brianp
 * optimized vertex[234] code
 *
 * Revision 1.1  1997/04/02 03:14:14  brianp
 * Initial revision
 *
 */


/*
 * Render points, lines, and polygons.  The only entry point to this
 * file is the gl_render_vb() function.  This function is called after
 * the vertex buffer has filled up or glEnd() has been called.
 *
 * This file basically only makes calls to the clipping functions and
 * the point, line and triangle rasterizers via the function pointers.
 *    context->Driver.PointsFunc()
 *    context->Driver.LineFunc()
 *    context->Driver.TriangleFunc()
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include "clip.h"
#include "context.h"
#include "light.h"
#include "macros.h"
#include "matrix.h"
#include "pb.h"
#include "types.h"
#include "vb.h"
#include "vbrender.h"
#include "xform.h"
#endif


/*
 * This file implements rendering of points, lines and polygons defined by
 * vertices in the vertex buffer.
 */



#ifdef PROFILE
#  define START_PROFILE				\
	{					\
	   GLdouble t0 = gl_time();

#  define END_PROFILE( TIMER, COUNTER, INCR )	\
	   TIMER += (gl_time() - t0);		\
	   COUNTER += INCR;			\
	}
#else
#  define START_PROFILE
#  define END_PROFILE( TIMER, COUNTER, INCR )
#endif




/*
 * Render a line segment from VB[v1] to VB[v2] when either one or both
 * endpoints must be clipped.
 */
static void render_clipped_line( GLcontext *ctx, GLuint v1, GLuint v2 )
{
   GLfloat ndc_x, ndc_y, ndc_z;
   GLuint provoking_vertex;
   struct vertex_buffer *VB = ctx->VB;

   /* which vertex dictates the color when flat shading: */
   provoking_vertex = v2;

   /*
    * Clipping may introduce new vertices.  New vertices will be stored
    * in the vertex buffer arrays starting with location VB->Free.  After
    * we've rendered the line, these extra vertices can be overwritten.
    */
   VB->Free = VB_MAX;

   /* Clip against user clipping planes */
   if (ctx->Transform.AnyClip) {
      GLuint orig_v1 = v1, orig_v2 = v2;
      if (gl_userclip_line( ctx, &v1, &v2 )==0)
	return;
      /* Apply projection matrix:  clip = Proj * eye */
      if (v1!=orig_v1) {
         TRANSFORM_POINT( VB->Clip[v1], ctx->ProjectionMatrix, VB->Eye[v1] );
      }
      if (v2!=orig_v2) {
         TRANSFORM_POINT( VB->Clip[v2], ctx->ProjectionMatrix, VB->Eye[v2] );
      }
   }

   /* Clip against view volume */
   if (gl_viewclip_line( ctx, &v1, &v2 )==0)
      return;

   /* Transform from clip coords to ndc:  ndc = clip / W */
   if (VB->Clip[v1][3] != 0.0F) {
      GLfloat wInv = 1.0F / VB->Clip[v1][3];
      ndc_x = VB->Clip[v1][0] * wInv;
      ndc_y = VB->Clip[v1][1] * wInv;
      ndc_z = VB->Clip[v1][2] * wInv;
   }
   else {
      /* Can't divide by zero, so... */
      ndc_x = ndc_y = ndc_z = 0.0F;
   }

   /* Map ndc coord to window coords. */
   VB->Win[v1][0] = ndc_x * ctx->Viewport.Sx + ctx->Viewport.Tx;
   VB->Win[v1][1] = ndc_y * ctx->Viewport.Sy + ctx->Viewport.Ty;
   VB->Win[v1][2] = ndc_z * ctx->Viewport.Sz + ctx->Viewport.Tz;

   /* Transform from clip coords to ndc:  ndc = clip / W */
   if (VB->Clip[v2][3] != 0.0F) {
      GLfloat wInv = 1.0F / VB->Clip[v2][3];
      ndc_x = VB->Clip[v2][0] * wInv;
      ndc_y = VB->Clip[v2][1] * wInv;
      ndc_z = VB->Clip[v2][2] * wInv;
   }
   else {
      /* Can't divide by zero, so... */
      ndc_x = ndc_y = ndc_z = 0.0F;
   }

   /* Map ndc coord to window coords. */
   VB->Win[v2][0] = ndc_x * ctx->Viewport.Sx + ctx->Viewport.Tx;
   VB->Win[v2][1] = ndc_y * ctx->Viewport.Sy + ctx->Viewport.Ty;
   VB->Win[v2][2] = ndc_z * ctx->Viewport.Sz + ctx->Viewport.Tz;

   if (ctx->Driver.RasterSetup) {
      /* Device driver rasterization setup */
      (*ctx->Driver.RasterSetup)( ctx, v1, v1+1 );
      (*ctx->Driver.RasterSetup)( ctx, v2, v2+1 );
   }

   START_PROFILE
   (*ctx->Driver.LineFunc)( ctx, v1, v2, provoking_vertex );
   END_PROFILE( ctx->LineTime, ctx->LineCount, 1 )
}



/*
 * Compute Z offsets for a polygon with plane defined by (A,B,C,D)
 * D is not needed.
 */
static void offset_polygon( GLcontext *ctx, GLfloat a, GLfloat b, GLfloat c )
{
   GLfloat ac, bc, m;
   GLfloat offset;

   if (c<0.001F && c>-0.001F) {
      /* to prevent underflow problems */
      offset = 0.0F;
   }
   else {
      ac = a / c;
      bc = b / c;
      if (ac<0.0F)  ac = -ac;
      if (bc<0.0F)  bc = -bc;
      m = MAX2( ac, bc );
      /* m = sqrt( ac*ac + bc*bc ); */

      offset = m * ctx->Polygon.OffsetFactor + ctx->Polygon.OffsetUnits;
   }

   ctx->PointZoffset   = ctx->Polygon.OffsetPoint ? offset : 0.0F;
   ctx->LineZoffset    = ctx->Polygon.OffsetLine  ? offset : 0.0F;
   ctx->PolygonZoffset = ctx->Polygon.OffsetFill  ? offset : 0.0F;
}



/*
 * When glPolygonMode() is used to specify that the front/back rendering
 * mode for polygons is not GL_FILL we end up calling this function.
 */
static void unfilled_polygon( GLcontext *ctx,
                              GLuint n, GLuint vlist[],
                              GLuint pv, GLuint facing )
{
   GLenum mode = facing ? ctx->Polygon.BackMode : ctx->Polygon.FrontMode;
   struct vertex_buffer *VB = ctx->VB;

   if (mode==GL_POINT) {
      GLint i, j;
      GLboolean edge;

      if (   ctx->Primitive==GL_TRIANGLES
          || ctx->Primitive==GL_QUADS
          || ctx->Primitive==GL_POLYGON) {
         edge = GL_FALSE;
      }
      else {
         edge = GL_TRUE;
      }

      for (i=0;i<n;i++) {
         j = vlist[i];
         if (edge || VB->Edgeflag[j]) {
            (*ctx->Driver.PointsFunc)( ctx, j, j );
         }
      }
   }
   else if (mode==GL_LINE) {
      GLuint i, j0, j1;
      GLboolean edge;

      ctx->StippleCounter = 0;

      if (   ctx->Primitive==GL_TRIANGLES
          || ctx->Primitive==GL_QUADS
          || ctx->Primitive==GL_POLYGON) {
         edge = GL_FALSE;
      }
      else {
         edge = GL_TRUE;
      }

      /* draw the edges */
      for (i=0;i<n;i++) {
         j0 = (i==0) ? vlist[n-1] : vlist[i-1];
         j1 = vlist[i];
         if (edge || VB->Edgeflag[j0]) {
            START_PROFILE
            (*ctx->Driver.LineFunc)( ctx, j0, j1, pv );
            END_PROFILE( ctx->LineTime, ctx->LineCount, 1 )
         }
      }
   }
   else {
      /* Fill the polygon */
      GLuint j0, i;
      j0 = vlist[0];
      for (i=2;i<n;i++) {
         START_PROFILE
         (*ctx->Driver.TriangleFunc)( ctx, j0, vlist[i-1], vlist[i], pv );
         END_PROFILE( ctx->PolygonTime, ctx->PolygonCount, 1 )
      }
   }
}


/*
 * Compute signed area of the n-sided polgyon specified by vertices vb->Win[]
 * and vertex list vlist[].
 * A clockwise polygon will return a negative area.
 * A counter-clockwise polygon will return a positive area.
 */
static GLfloat polygon_area( const struct vertex_buffer *vb,
                             GLuint n, const GLuint vlist[] )
{
   GLfloat area = 0.0F;
   GLint i;
   for (i=0;i<n;i++) {
      /* area = sum of trapezoids */
      GLuint j0 = vlist[i];
      GLuint j1 = vlist[(i+1)%n];
      GLfloat x0 = vb->Win[j0][0];
      GLfloat y0 = vb->Win[j0][1];
      GLfloat x1 = vb->Win[j1][0];
      GLfloat y1 = vb->Win[j1][1];
      GLfloat trapArea = (x0-x1)*(y0+y1);  /* Note: no divide by two here! */
      area += trapArea;
   }
   return area * 0.5F;     /* divide by two now! */
}


/*
 * Render a polygon in which doesn't have to be clipped.
 * Input:  n - number of vertices
 *         vlist - list of vertices in the polygon.
 */
static void render_polygon( GLcontext *ctx, GLuint n, GLuint vlist[] )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint pv;

   /* which vertex dictates the color when flat shading: */
   pv = (ctx->Primitive==GL_POLYGON) ? vlist[0] : vlist[n-1];

   /* Compute orientation of polygon, do cull test, offset, etc */
   {
      GLuint facing;   /* 0=front, 1=back */
      GLfloat area = polygon_area( VB, n, vlist );

      if (area==0.0F) {
         /* polygon has zero area, don't draw it */
         return;
      }

      facing = (area<0.0F) ^ (ctx->Polygon.FrontFace==GL_CW);

      if ((facing+1) & ctx->Polygon.CullBits) {
         return;   /* culled */
      }

      if (ctx->Polygon.OffsetAny) {
         /* compute plane equation of polygon, apply offset */
         GLuint j0 = vlist[0];
         GLuint j1 = vlist[1];
         GLuint j2 = vlist[2];
         GLuint j3 = vlist[ (n==3) ? 0 : 3 ];
         GLfloat ex = VB->Win[j1][0] - VB->Win[j3][0];
         GLfloat ey = VB->Win[j1][1] - VB->Win[j3][1];
         GLfloat ez = VB->Win[j1][2] - VB->Win[j3][2];
         GLfloat fx = VB->Win[j2][0] - VB->Win[j0][0];
         GLfloat fy = VB->Win[j2][1] - VB->Win[j0][1];
         GLfloat fz = VB->Win[j2][2] - VB->Win[j0][2];
         GLfloat a = ey*fz-ez*fy;
         GLfloat b = ez*fx-ex*fz;
         GLfloat c = ex*fy-ey*fx;
         offset_polygon( ctx, a, b, c );
      }

      if (ctx->LightTwoSide) {
         if (facing==1) {
            /* use back color or index */
            VB->Color = VB->Bcolor;
            VB->Index = VB->Bindex;
         }
         else {
            /* use front color or index */
            VB->Color = VB->Fcolor;
            VB->Index = VB->Findex;
         }
      }

      /* Render the polygon! */
      if (ctx->Polygon.Unfilled) {
         unfilled_polygon( ctx, n, vlist, pv, facing );
      }
      else {
         /* Draw filled polygon as a triangle fan */
         GLint i;
         GLuint j0 = vlist[0];
         for (i=2;i<n;i++) {
            START_PROFILE
            (*ctx->Driver.TriangleFunc)( ctx, j0, vlist[i-1], vlist[i], pv );
            END_PROFILE( ctx->PolygonTime, ctx->PolygonCount, 1 )
         }
      }
   }
}



/*
 * Render a polygon in which at least one vertex has to be clipped.
 * Input:  n - number of vertices
 *         vlist - list of vertices in the polygon.
 *                 CCW order = front facing.
 */
static void render_clipped_polygon( GLcontext *ctx, GLuint n, GLuint vlist[] )
{
   GLuint pv;
   struct vertex_buffer *VB = ctx->VB;
   GLfloat (*win)[3] = VB->Win;

   /* which vertex dictates the color when flat shading: */
   pv = (ctx->Primitive==GL_POLYGON) ? vlist[0] : vlist[n-1];

   /*
    * Clipping may introduce new vertices.  New vertices will be stored
    * in the vertex buffer arrays starting with location VB->Free.  After
    * we've rendered the polygon, these extra vertices can be overwritten.
    */
   VB->Free = VB_MAX;

   /* Clip against user clipping planes in eye coord space. */
   if (ctx->Transform.AnyClip) {
      GLfloat *proj = ctx->ProjectionMatrix;
      GLuint i;
      n = gl_userclip_polygon( ctx, n, vlist );
      if (n<3)
         return;
      /* Transform vertices from eye to clip coordinates:  clip = Proj * eye */
      for (i=0;i<n;i++) {
         GLuint j = vlist[i];
         TRANSFORM_POINT( VB->Clip[j], proj, VB->Eye[j] );
      }
   }

   /* Clip against view volume in clip coord space */
   n = gl_viewclip_polygon( ctx, n, vlist );
   if (n<3)
      return;

   /* Transform new vertices from clip to ndc to window coords.    */
   /* ndc = clip / W    window = viewport_mapping(ndc)             */
   /* Note that window Z values are scaled to the range of integer */
   /* depth buffer values.                                         */
   {
      GLfloat sx = ctx->Viewport.Sx;
      GLfloat tx = ctx->Viewport.Tx;
      GLfloat sy = ctx->Viewport.Sy;
      GLfloat ty = ctx->Viewport.Ty;
      GLfloat sz = ctx->Viewport.Sz;
      GLfloat tz = ctx->Viewport.Tz;
      GLuint i;
      /* Only need to compute window coords for new vertices */
      for (i=VB_MAX; i<VB->Free; i++) {
         if (VB->Clip[i][3] != 0.0F) {
            GLfloat wInv = 1.0F / VB->Clip[i][3];
            win[i][0] = VB->Clip[i][0] * wInv * sx + tx;
            win[i][1] = VB->Clip[i][1] * wInv * sy + ty;
            win[i][2] = VB->Clip[i][2] * wInv * sz + tz;
         }
         else {
            /* Can't divide by zero, so... */
            win[i][0] = win[i][1] = win[i][2] = 0.0F;
         }
      }
      if (ctx->Driver.RasterSetup && (VB->Free > VB_MAX)) {
         /* Device driver raster setup for newly introduced vertices */
         (*ctx->Driver.RasterSetup)(ctx, VB_MAX, VB->Free);
      }

#ifdef DEBUG
      {
         int i, j;
         for (i=0;i<n;i++) {
            j = vlist[i];
            if (VB->ClipMask[j]) {
               /* Uh oh!  There should be no clip bits set in final polygon! */
               int k, l;
               printf("CLIPMASK %d %d %02x\n", i, j, VB->ClipMask[j]);
               printf("%f %f %f %f\n", VB->Eye[j][0], VB->Eye[j][1], 
                      VB->Eye[j][2], VB->Eye[j][3]);
               printf("%f %f %f %f\n", VB->Clip[j][0], VB->Clip[j][1], 
                      VB->Clip[j][2], VB->Clip[j][3]);
               for (k=0;k<n;k++) {
                  l = vlist[k];
                  printf("%d %d %02x\n", k, l, VB->ClipMask[l]);
               }
            }
         }
      }
#endif
   }

   /* Compute orientation of polygon, do cull test, offset, etc */
   {
      GLuint facing;   /* 0=front, 1=back */
      GLfloat area = polygon_area( VB, n, vlist );

      if (area==0.0F) {
         /* polygon has zero area, don't draw it */
         return;
      }

      facing = (area<0.0F) ^ (ctx->Polygon.FrontFace==GL_CW);

      if ((facing+1) & ctx->Polygon.CullBits) {
         return;   /* culled */
      }

      if (ctx->Polygon.OffsetAny) {
         /* compute plane equation of polygon, apply offset */
         GLuint j0 = vlist[0];
         GLuint j1 = vlist[1];
         GLuint j2 = vlist[2];
         GLuint j3 = vlist[ (n==3) ? 0 : 3 ];
         GLfloat ex = win[j1][0] - win[j3][0];
         GLfloat ey = win[j1][1] - win[j3][1];
         GLfloat ez = win[j1][2] - win[j3][2];
         GLfloat fx = win[j2][0] - win[j0][0];
         GLfloat fy = win[j2][1] - win[j0][1];
         GLfloat fz = win[j2][2] - win[j0][2];
         GLfloat a = ey*fz-ez*fy;
         GLfloat b = ez*fx-ex*fz;
         GLfloat c = ex*fy-ey*fx;
         offset_polygon( ctx, a, b, c );
      }

      if (ctx->LightTwoSide) {
         if (facing==1) {
            /* use back color or index */
            VB->Color = VB->Bcolor;
            VB->Index = VB->Bindex;
         }
         else {
            /* use front color or index */
            VB->Color = VB->Fcolor;
            VB->Index = VB->Findex;
         }
      }

      /* Render the polygon! */
      if (ctx->Polygon.Unfilled) {
         unfilled_polygon( ctx, n, vlist, pv, facing );
      }
      else {
         /* Draw filled polygon as a triangle fan */
         GLint i;
         GLuint j0 = vlist[0];
         for (i=2;i<n;i++) {
            START_PROFILE
            (*ctx->Driver.TriangleFunc)( ctx, j0, vlist[i-1], vlist[i], pv );
            END_PROFILE( ctx->PolygonTime, ctx->PolygonCount, 1 )
         }
      }
   }
}



/*
 * Render an un-clipped triangle.
 * v0, v1, v2 - vertex indexes.  CCW order = front facing
 * pv - provoking vertex
 */
static void render_triangle( GLcontext *ctx,
                             GLuint v0, GLuint v1, GLuint v2, GLuint pv )
{
   struct vertex_buffer *VB = ctx->VB;
   GLfloat ex, ey, fx, fy, c;
   GLuint facing;  /* 0=front, 1=back */
   GLfloat (*win)[3] = VB->Win;

   /* Compute orientation of triangle */
   ex = win[v1][0] - win[v0][0];
   ey = win[v1][1] - win[v0][1];
   fx = win[v2][0] - win[v0][0];
   fy = win[v2][1] - win[v0][1];
   c = ex*fy-ey*fx;

   if (c==0.0F) {
      /* polygon is perpindicular to view plane, don't draw it */
      return;
   }

   facing = (c<0.0F) ^ (ctx->Polygon.FrontFace==GL_CW);

   if ((facing+1) & ctx->Polygon.CullBits) {
      return;   /* culled */
   }

   if (ctx->Polygon.OffsetAny) {
      /* finish computing plane equation of polygon, compute offset */
      GLfloat fz = win[v2][2] - win[v0][2];
      GLfloat ez = win[v1][2] - win[v0][2];
      GLfloat a = ey*fz-ez*fy;
      GLfloat b = ez*fx-ex*fz;
      offset_polygon( ctx, a, b, c );
   }

   if (ctx->LightTwoSide) {
      if (facing==1) {
         /* use back color or index */
         VB->Color = VB->Bcolor;
         VB->Index = VB->Bindex;
      }
      else {
         /* use front color or index */
         VB->Color = VB->Fcolor;
         VB->Index = VB->Findex;
      }
   }

   if (ctx->Polygon.Unfilled) {
      GLuint vlist[3];
      vlist[0] = v0;
      vlist[1] = v1;
      vlist[2] = v2;
      unfilled_polygon( ctx, 3, vlist, pv, facing );
   }
   else {
      START_PROFILE
      (*ctx->Driver.TriangleFunc)( ctx, v0, v1, v2, pv );
      END_PROFILE( ctx->PolygonTime, ctx->PolygonCount, 1 )
   }
}



/*
 * Render an un-clipped quadrilateral.
 * v0, v1, v2, v3 : CCW order = front facing
 * pv - provoking vertex
 */
static void render_quad( GLcontext *ctx, GLuint v0, GLuint v1,
                         GLuint v2, GLuint v3, GLuint pv )
{
   struct vertex_buffer *VB = ctx->VB;
   GLfloat ex, ey, fx, fy, c;
   GLuint facing;  /* 0=front, 1=back */
   GLfloat (*win)[3] = VB->Win;

   /* Compute polygon orientation */
   ex = win[v2][0] - win[v0][0];
   ey = win[v2][1] - win[v0][1];
   fx = win[v3][0] - win[v1][0];
   fy = win[v3][1] - win[v1][1];
   c = ex*fy-ey*fx;

   if (c==0.0F) {
      /* polygon is perpindicular to view plane, don't draw it */
      return;
   }

   facing = (c<0.0F) ^ (ctx->Polygon.FrontFace==GL_CW);

   if ((facing+1) & ctx->Polygon.CullBits) {
      return;   /* culled */
   }

   if (ctx->Polygon.OffsetAny) {
      /* finish computing plane equation of polygon, compute offset */
      GLfloat ez = win[v2][2] - win[v0][2];
      GLfloat fz = win[v3][2] - win[v1][2];
      GLfloat a = ey*fz-ez*fy;
      GLfloat b = ez*fx-ex*fz;
      offset_polygon( ctx, a, b, c );
   }

   if (ctx->LightTwoSide) {
      if (facing==1) {
         /* use back color or index */
         VB->Color = VB->Bcolor;
         VB->Index = VB->Bindex;
      }
      else {
         /* use front color or index */
         VB->Color = VB->Fcolor;
         VB->Index = VB->Findex;
      }
   }

   /* Render the quad! */
   if (ctx->Polygon.Unfilled) {
      GLuint vlist[4];
      vlist[0] = v0;
      vlist[1] = v1;
      vlist[2] = v2;
      vlist[3] = v3;
      unfilled_polygon( ctx, 4, vlist, pv, facing );
   }
   else {
      START_PROFILE
      (*ctx->Driver.QuadFunc)( ctx, v0, v1, v2, v3, pv );
      END_PROFILE( ctx->PolygonTime, ctx->PolygonCount, 2 )
   }
}



/*
 * When the vertex buffer is full, we transform/render it.  Sometimes we
 * have to copy the last vertex (or two) to the front of the vertex list
 * to "continue" the primitive.  For example:  line or triangle strips.
 * This function is a helper for that.
 */
static void copy_vertex( struct vertex_buffer *vb, GLuint dst, GLuint src )
{
   COPY_4V( vb->Clip[dst], vb->Clip[src] );
   COPY_4V( vb->Eye[dst], vb->Eye[src] );
   COPY_3V( vb->Win[dst], vb->Win[src] );
   COPY_4V( vb->Fcolor[dst], vb->Fcolor[src] );
   COPY_4V( vb->Bcolor[dst], vb->Bcolor[src] );
   COPY_4V( vb->TexCoord[dst], vb->TexCoord[src] );
   vb->Findex[dst] = vb->Findex[src];
   vb->Bindex[dst] = vb->Bindex[src];
   vb->Edgeflag[dst] = vb->Edgeflag[src];
   vb->ClipMask[dst] = vb->ClipMask[src];
   vb->MaterialMask[dst] = vb->MaterialMask[src];
   vb->Material[dst][0] = vb->Material[src][0];
   vb->Material[dst][1] = vb->Material[src][1];
}




/*
 * Either the vertex buffer is full (VB->Count==VB_MAX) or glEnd() has been
 * called.  Render the primitives defined by the vertices and reset the
 * buffer.
 *
 * This function won't be called if the device driver implements a
 * RenderVB() function.  If the device driver renders the vertex buffer
 * then the driver must also call gl_reset_vb()!
 *
 * Input:  allDone - GL_TRUE = caller is glEnd()
 *                   GL_FALSE = calling because buffer is full.
 */
void gl_render_vb( GLcontext *ctx, GLboolean allDone )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint vlist[VB_SIZE];

   switch (ctx->Primitive) {
      case GL_POINTS:
         START_PROFILE
         (*ctx->Driver.PointsFunc)( ctx, 0, VB->Count-1 );
         END_PROFILE( ctx->PointTime, ctx->PointCount, VB->Count )
	 break;

      case GL_LINES:
         if (VB->ClipOrMask) {
            GLuint i;
            for (i=1;i<VB->Count;i+=2) {
               if (VB->ClipMask[i-1] | VB->ClipMask[i]) {
                  render_clipped_line( ctx, i-1, i );
               }
               else {
                  START_PROFILE
                  (*ctx->Driver.LineFunc)( ctx, i-1, i, i );
                  END_PROFILE( ctx->LineTime, ctx->LineCount, 1 )
               }
               ctx->StippleCounter = 0;
            }
         }
         else {
            GLuint i;
            for (i=1;i<VB->Count;i+=2) {
               START_PROFILE
               (*ctx->Driver.LineFunc)( ctx, i-1, i, i );
               END_PROFILE( ctx->LineTime, ctx->LineCount, 1 )
               ctx->StippleCounter = 0;
            }
         }
	 break;

      case GL_LINE_STRIP:
         if (VB->ClipOrMask) {
            GLuint i;
	    for (i=1;i<VB->Count;i++) {
               if (VB->ClipMask[i-1] | VB->ClipMask[i]) {
                  render_clipped_line( ctx, i-1, i );
               }
               else {
                  START_PROFILE
                  (*ctx->Driver.LineFunc)( ctx, i-1, i, i );
                  END_PROFILE( ctx->LineTime, ctx->LineCount, 1 )
               }
	    }
         }
         else {
            /* no clipping needed */
            GLuint i;
	    for (i=1;i<VB->Count;i++) {
               START_PROFILE
               (*ctx->Driver.LineFunc)( ctx, i-1, i, i );
               END_PROFILE( ctx->LineTime, ctx->LineCount, 1 )
            }
         }
         break;

      case GL_LINE_LOOP:
         {
            GLuint i;
            if (VB->Start==0) {
               i = 1;  /* start at 0th vertex */
            }
            else {
               i = 2;  /* skip first vertex, we're saving it until glEnd */
            }
            while (i<VB->Count) {
               if (VB->ClipMask[i-1] | VB->ClipMask[i]) {
                  render_clipped_line( ctx, i-1, i );
               }
               else {
                  START_PROFILE
                  (*ctx->Driver.LineFunc)( ctx, i-1, i, i );
                  END_PROFILE( ctx->LineTime, ctx->LineCount, 1 )
               }
               i++;
            }
         }
         break;

      case GL_TRIANGLES:
         if (VB->ClipOrMask) {
            GLuint i;
            for (i=2;i<VB->Count;i+=3) {
               if (VB->ClipMask[i-2] & VB->ClipMask[i-1]
                   & VB->ClipMask[i] & CLIP_ALL_BITS) {
                  /* all points clipped by common plane */
                  continue;
               }
               else if (VB->ClipMask[i-2] | VB->ClipMask[i-1] | VB->ClipMask[i]) {
                  vlist[0] = i-2;
                  vlist[1] = i-1;
                  vlist[2] = i-0;
                  render_clipped_polygon( ctx, 3, vlist );
               }
               else {
                  if (ctx->DirectTriangles) {
                     START_PROFILE
                     (*ctx->Driver.TriangleFunc)( ctx, i-2, i-1, i, i );
                     END_PROFILE( ctx->PolygonTime, ctx->PolygonCount, 1 )
                  }
                  else {
                     render_triangle( ctx, i-2, i-1, i, i );
                  }
               }
            }
         }
         else {
            /* no clipping needed */
            GLuint i;
            if (ctx->DirectTriangles) {
               for (i=2;i<VB->Count;i+=3) {
                  START_PROFILE
                  (*ctx->Driver.TriangleFunc)( ctx, i-2, i-1, i, i );
                  END_PROFILE( ctx->PolygonTime, ctx->PolygonCount, 1 )
               }
            }
            else {
               for (i=2;i<VB->Count;i+=3) {
                  render_triangle( ctx, i-2, i-1, i, i );
               }
            }
         }
	 break;

      case GL_TRIANGLE_STRIP:
         if (VB->ClipOrMask) {
            GLuint i;
            for (i=2;i<VB->Count;i++) {
               if (VB->ClipMask[i-2] & VB->ClipMask[i-1]
                   & VB->ClipMask[i] & CLIP_ALL_BITS) {
                  /* all points clipped by common plane */
                  continue;
               }
               else if (VB->ClipMask[i-2] | VB->ClipMask[i-1] | VB->ClipMask[i]) {
                  if (i&1) {
                     /* reverse vertex order */
                     vlist[0] = i-1;
                     vlist[1] = i-2;
                     vlist[2] = i-0;
                     render_clipped_polygon( ctx, 3, vlist );
                  }
                  else {
                     vlist[0] = i-2;
                     vlist[1] = i-1;
                     vlist[2] = i-0;
                     render_clipped_polygon( ctx, 3, vlist );
                  }
               }
               else {
                  if (ctx->DirectTriangles) {
                     START_PROFILE
                     (*ctx->Driver.TriangleFunc)( ctx, i-2, i-1, i, i );
                     END_PROFILE( ctx->PolygonTime, ctx->PolygonCount, 1 )
                  }
                  else {
                     if (i&1)
                        render_triangle( ctx, i, i-1, i-2, i );
                     else
                        render_triangle( ctx, i-2, i-1, i, i );
                  }
               }
            }
         }
         else {
            /* no vertices were clipped */
            GLuint i;
            if (ctx->DirectTriangles) {
               for (i=2;i<VB->Count;i++) {
                  START_PROFILE
                  (*ctx->Driver.TriangleFunc)( ctx, i-2, i-1, i, i );
                  END_PROFILE( ctx->PolygonTime, ctx->PolygonCount, 1 )
               }
            }
            else {
               for (i=2;i<VB->Count;i++) {
                  if (i&1)
                     render_triangle( ctx, i, i-1, i-2, i );
                  else
                     render_triangle( ctx, i-2, i-1, i, i );
               }
            }
         }
	 break;

      case GL_TRIANGLE_FAN:
         if (VB->ClipOrMask) {
            GLuint i;
            for (i=2;i<VB->Count;i++) {
               if (VB->ClipMask[0] & VB->ClipMask[i-1] & VB->ClipMask[i]
                   & CLIP_ALL_BITS) {
                  /* all points clipped by common plane */
                  continue;
               }
               else if (VB->ClipMask[0] | VB->ClipMask[i-1] | VB->ClipMask[i]) {
                  vlist[0] = 0;
                  vlist[1] = i-1;
                  vlist[2] = i;
                  render_clipped_polygon( ctx, 3, vlist );
               }
               else {
                  if (ctx->DirectTriangles) {
                     START_PROFILE
                     (*ctx->Driver.TriangleFunc)( ctx, 0, i-1, i, i );
                     END_PROFILE( ctx->PolygonTime, ctx->PolygonCount, 1 )
                  }
                  else {
                     render_triangle( ctx, 0, i-1, i, i );
                  }
               }
            }
         }
         else {
            /* no clipping needed */
            GLuint i;
            if (ctx->DirectTriangles) {
               for (i=2;i<VB->Count;i++) {
                  START_PROFILE
                  (*ctx->Driver.TriangleFunc)( ctx, 0, i-1, i, i );
                  END_PROFILE( ctx->PolygonTime, ctx->PolygonCount, 1 )
               }
            }
            else {
               for (i=2;i<VB->Count;i++) {
                  render_triangle( ctx, 0, i-1, i, i );
               }
            }
         }
	 break;

      case GL_QUADS:
         if (VB->ClipOrMask) {
            GLuint i;
            for (i=3;i<VB->Count;i+=4) {
               if (VB->ClipMask[i-3] & VB->ClipMask[i-2]
                   & VB->ClipMask[i-1] & VB->ClipMask[i] & CLIP_ALL_BITS) {
                  /* all points clipped by common plane */
                  continue;
               }
               else if (VB->ClipMask[i-3] | VB->ClipMask[i-2]
                        | VB->ClipMask[i-1] | VB->ClipMask[i]) {
                  vlist[0] = i-3;
                  vlist[1] = i-2;
                  vlist[2] = i-1;
                  vlist[3] = i-0;
                  render_clipped_polygon( ctx, 4, vlist );
               }
               else {
                  if (ctx->DirectTriangles) {
                     START_PROFILE
                     (*ctx->Driver.QuadFunc)( ctx, i-3, i-2, i-1, i, i );
                     END_PROFILE( ctx->PolygonTime, ctx->PolygonCount, 2 )
                  }
                  else {
                     render_quad( ctx, i-3, i-2, i-1, i, i );
                  }
               }
            }
         }
         else {
            /* no vertices were clipped */
            GLuint i;
            if (ctx->DirectTriangles) {
               for (i=3;i<VB->Count;i+=4) {
                  START_PROFILE
                  (*ctx->Driver.QuadFunc)( ctx, i-3, i-2, i-1, i, i );
                  END_PROFILE( ctx->PolygonTime, ctx->PolygonCount, 2 )
               }
            }
            else {
               for (i=3;i<VB->Count;i+=4) {
                  render_quad( ctx, i-3, i-2, i-1, i, i );
               }
            }
         }
	 break;

      case GL_QUAD_STRIP:
         if (VB->ClipOrMask) {
            GLuint i;
            for (i=3;i<VB->Count;i+=2) {
               if (VB->ClipMask[i-2] & VB->ClipMask[i-3]
                   & VB->ClipMask[i-1] & VB->ClipMask[i] & CLIP_ALL_BITS) {
                  /* all points clipped by common plane */
                  continue;
               }
               else if (VB->ClipMask[i-2] | VB->ClipMask[i-3]
                        | VB->ClipMask[i-1] | VB->ClipMask[i]) {
                  vlist[0] = i-1;
                  vlist[1] = i-3;
                  vlist[2] = i-2;
                  vlist[3] = i-0;
                  render_clipped_polygon( ctx, 4, vlist );
               }
               else {
                  if (ctx->DirectTriangles) {
                     START_PROFILE
                     (*ctx->Driver.QuadFunc)( ctx, i-3, i-2, i, i-1, i );
                     END_PROFILE( ctx->PolygonTime, ctx->PolygonCount, 2 )
                  }
                  else {
                     render_quad( ctx, i-3, i-2, i, i-1, i );
                  }
               }
            }
         }
         else {
            /* no clipping needed */
            GLuint i;
            if (ctx->DirectTriangles) {
               for (i=3;i<VB->Count;i+=2) {
                  START_PROFILE
                  (*ctx->Driver.QuadFunc)( ctx, i-3, i-2, i, i-1, i );
                  END_PROFILE( ctx->PolygonTime, ctx->PolygonCount, 2 )
               }
            }
            else {
               for (i=3;i<VB->Count;i+=2) {
                  render_quad( ctx, i-3, i-2, i, i-1, i );
               }
            }
         }
	 break;

      case GL_POLYGON:
         if (VB->Count>2) {
            if (VB->ClipAndMask & CLIP_ALL_BITS) {
               /* all points clipped by common plane, draw nothing */
               break;
            }
            if (VB->ClipOrMask) {
               /* need clipping */
               GLuint i;
               for (i=0;i<VB->Count;i++) {
                  vlist[i] = i;
               }
               render_clipped_polygon( ctx, VB->Count, vlist );
            }
            else {
               /* no clipping needed */
               static GLuint const_vlist[VB_SIZE];
               static GLboolean initFlag = GL_TRUE;
               if (initFlag) {
                  /* vertex list always the same, never changes */
                  GLuint i;
                  for (i=0;i<VB_SIZE;i++) {
                     const_vlist[i] = i;
                  }
                  initFlag = GL_FALSE;
               }
               render_polygon( ctx, VB->Count, const_vlist );
            }
         }
	 break;

      default:
         /* should never get here */
         gl_problem( ctx, "invalid mode in gl_render_vb" );
   }

   gl_reset_vb( ctx, allDone );
}


#define CLIP_ALL_BITS    0x3f


/*
 * After we've rendered the primitives in the vertex buffer we call
 * this function to reset the vertex buffer.  That is, we prepare it
 * for the next batch of vertices.
 * Input:  ctx - the context
 *         allDone - GL_TRUE = glEnd() was called
 *                   GL_FALSE = buffer was filled, more vertices to come
 */
void gl_reset_vb( GLcontext *ctx, GLboolean allDone )
{
   struct vertex_buffer *VB = ctx->VB;

   /* save a few VB values for the end of this function */
   int oldCount = VB->Count;
   GLubyte clipOrMask = VB->ClipOrMask;
   GLboolean monoMaterial = VB->MonoMaterial;
   GLuint vertexSizeMask = VB->VertexSizeMask;

   /* Special case for GL_LINE_LOOP */
   if (ctx->Primitive==GL_LINE_LOOP && allDone) {
      if (VB->ClipMask[VB->Count-1] | VB->ClipMask[0]) {
         render_clipped_line( ctx, VB->Count-1, 0 );
      }
      else {
         START_PROFILE
         (*ctx->Driver.LineFunc)( ctx, VB->Count-1, 0, 0 );
         END_PROFILE( ctx->LineTime, ctx->LineCount, 1 )
      }
   }

   if (allDone) {
      /* glEnd() was called so reset Vertex Buffer to default, empty state */
      VB->Start = VB->Count = 0;
      VB->ClipOrMask = 0;
      VB->ClipAndMask = CLIP_ALL_BITS;
      VB->MonoMaterial = GL_TRUE;
      VB->MonoNormal = GL_TRUE;
      VB->MonoColor = GL_TRUE;
      VB->VertexSizeMask = VERTEX3_BIT;
      if (VB->TexCoordSize!=2) {
         GLint i, n = VB->Count;
         for (i=0;i<n;i++) {
            VB->TexCoord[i][2] = 0.0F;
            VB->TexCoord[i][3] = 1.0F;
         }
      }
      if (ctx->Current.TexCoord[2]==0.0F && ctx->Current.TexCoord[3]==1.0F) {
         VB->TexCoordSize = 2;
      }
      else {
         VB->TexCoordSize = 4;
      }
   }
   else {
      /* The vertex buffer was filled but we didn't get a glEnd() call yet
       * have to "re-cycle" the vertex buffer.
       */
      switch (ctx->Primitive) {
         case GL_POINTS:
            ASSERT(VB->Start==0);
            VB->Start = VB->Count = 0;
            VB->ClipOrMask = 0;
            VB->ClipAndMask = CLIP_ALL_BITS;
            VB->MonoMaterial = GL_TRUE;
            VB->MonoNormal = GL_TRUE;
            break;
         case GL_LINES:
            ASSERT(VB->Start==0);
            VB->Start = VB->Count = 0;
            VB->ClipOrMask = 0;
            VB->ClipAndMask = CLIP_ALL_BITS;
            VB->MonoMaterial = GL_TRUE;
            VB->MonoNormal = GL_TRUE;
            break;
         case GL_LINE_STRIP:
            copy_vertex( VB, 0, VB->Count-1 );  /* copy last vertex to front */
            VB->Start = VB->Count = 1;
            VB->ClipOrMask = VB->ClipMask[0];
            VB->ClipAndMask = VB->ClipMask[0];
            VB->MonoMaterial = VB->MaterialMask[0] ? GL_FALSE : GL_TRUE;
            break;
         case GL_LINE_LOOP:
            ASSERT(VB->Count==VB_MAX);
            copy_vertex( VB, 1, VB_MAX-1 );
            VB->Start = VB->Count = 2;
            VB->ClipOrMask = VB->ClipMask[0] | VB->ClipMask[1];
            VB->ClipAndMask = VB->ClipMask[0] & VB->ClipMask[1];
            VB->MonoMaterial = !(VB->MaterialMask[0] | VB->MaterialMask[1]);
            break;
         case GL_TRIANGLES:
            ASSERT(VB->Start==0);
            VB->Start = VB->Count = 0;
            VB->ClipOrMask = 0;
            VB->ClipAndMask = CLIP_ALL_BITS;
            VB->MonoMaterial = GL_TRUE;
            VB->MonoNormal = GL_TRUE;
            break;
         case GL_TRIANGLE_STRIP:
            copy_vertex( VB, 0, VB_MAX-2 );
            copy_vertex( VB, 1, VB_MAX-1 );
            VB->Start = VB->Count = 2;
            VB->ClipOrMask = VB->ClipMask[0] | VB->ClipMask[1];
            VB->ClipAndMask = VB->ClipMask[0] & VB->ClipMask[1];
            VB->MonoMaterial = !(VB->MaterialMask[0] | VB->MaterialMask[1]);
            break;
         case GL_TRIANGLE_FAN:
            copy_vertex( VB, 1, VB_MAX-1 );
            VB->Start = VB->Count = 2;
            VB->ClipOrMask = VB->ClipMask[0] | VB->ClipMask[1];
            VB->ClipAndMask = VB->ClipMask[0] & VB->ClipMask[1];
            VB->MonoMaterial = !(VB->MaterialMask[0] | VB->MaterialMask[1]);
            break;
         case GL_QUADS:
            ASSERT(VB->Start==0);
            VB->Start = VB->Count = 0;
            VB->ClipOrMask = 0;
            VB->ClipAndMask = CLIP_ALL_BITS;
            VB->MonoMaterial = GL_TRUE;
            VB->MonoNormal = GL_TRUE;
            break;
         case GL_QUAD_STRIP:
            copy_vertex( VB, 0, VB_MAX-2 );
            copy_vertex( VB, 1, VB_MAX-1 );
            VB->Start = VB->Count = 2;
            VB->ClipOrMask = VB->ClipMask[0] | VB->ClipMask[1];
            VB->ClipAndMask = VB->ClipMask[0] & VB->ClipMask[1];
            VB->MonoMaterial = !(VB->MaterialMask[0] | VB->MaterialMask[1]);
            break;
         case GL_POLYGON:
            copy_vertex( VB, 1, VB_MAX-1 );
            VB->Start = VB->Count = 2;
            VB->ClipOrMask = VB->ClipMask[0] | VB->ClipMask[1];
            VB->ClipAndMask = VB->ClipMask[0] & VB->ClipMask[1];
            VB->MonoMaterial = !(VB->MaterialMask[0] | VB->MaterialMask[1]);
            break;
         default:
            /* should never get here */
            gl_problem(ctx, "Bad primitive type in gl_reset_vb()");
      }
   }

   if (clipOrMask) {
      /* reset clip masks to zero */
      MEMSET( VB->ClipMask + VB->Start, 0,
              (oldCount - VB->Start) * sizeof(VB->ClipMask[0]) );
   }

   if (!monoMaterial) {
      /* reset material masks to zero */
      MEMSET( VB->MaterialMask + VB->Start, 0,
              (oldCount - VB->Start) * sizeof(VB->MaterialMask[0]) );
      gl_update_lighting(ctx);
   }

   if (vertexSizeMask!=VERTEX3_BIT) {
      /* reset object W coords to one */
      GLint i, n;
      GLfloat (*obj)[4] = VB->Obj + VB->Start;
      n = oldCount - VB->Start;
      for (i=0; i<n; i++) {
         obj[i][3] = 1.0F;
      }
   }
}
