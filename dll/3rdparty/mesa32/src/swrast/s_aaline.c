/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "main/glheader.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "swrast/s_aaline.h"
#include "swrast/s_context.h"
#include "swrast/s_span.h"
#include "swrast/swrast.h"


#define SUB_PIXEL 4


/*
 * Info about the AA line we're rendering
 */
struct LineInfo
{
   GLfloat x0, y0;        /* start */
   GLfloat x1, y1;        /* end */
   GLfloat dx, dy;        /* direction vector */
   GLfloat len;           /* length */
   GLfloat halfWidth;     /* half of line width */
   GLfloat xAdj, yAdj;    /* X and Y adjustment for quad corners around line */
   /* for coverage computation */
   GLfloat qx0, qy0;      /* quad vertices */
   GLfloat qx1, qy1;
   GLfloat qx2, qy2;
   GLfloat qx3, qy3;
   GLfloat ex0, ey0;      /* quad edge vectors */
   GLfloat ex1, ey1;
   GLfloat ex2, ey2;
   GLfloat ex3, ey3;

   /* DO_Z */
   GLfloat zPlane[4];
   /* DO_RGBA */
   GLfloat rPlane[4], gPlane[4], bPlane[4], aPlane[4];
   /* DO_INDEX */
   GLfloat iPlane[4];
   /* DO_ATTRIBS */
   GLfloat wPlane[4];
   GLfloat attrPlane[FRAG_ATTRIB_MAX][4][4];
   GLfloat lambda[FRAG_ATTRIB_MAX];
   GLfloat texWidth[FRAG_ATTRIB_MAX];
   GLfloat texHeight[FRAG_ATTRIB_MAX];

   SWspan span;
};



/*
 * Compute the equation of a plane used to interpolate line fragment data
 * such as color, Z, texture coords, etc.
 * Input: (x0, y0) and (x1,y1) are the endpoints of the line.
 *        z0, and z1 are the end point values to interpolate.
 * Output:  plane - the plane equation.
 *
 * Note: we don't really have enough parameters to specify a plane.
 * We take the endpoints of the line and compute a plane such that
 * the cross product of the line vector and the plane normal is
 * parallel to the projection plane.
 */
static void
compute_plane(GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1,
              GLfloat z0, GLfloat z1, GLfloat plane[4])
{
#if 0
   /* original */
   const GLfloat px = x1 - x0;
   const GLfloat py = y1 - y0;
   const GLfloat pz = z1 - z0;
   const GLfloat qx = -py;
   const GLfloat qy = px;
   const GLfloat qz = 0;
   const GLfloat a = py * qz - pz * qy;
   const GLfloat b = pz * qx - px * qz;
   const GLfloat c = px * qy - py * qx;
   const GLfloat d = -(a * x0 + b * y0 + c * z0);
   plane[0] = a;
   plane[1] = b;
   plane[2] = c;
   plane[3] = d;
#else
   /* simplified */
   const GLfloat px = x1 - x0;
   const GLfloat py = y1 - y0;
   const GLfloat pz = z0 - z1;
   const GLfloat a = pz * px;
   const GLfloat b = pz * py;
   const GLfloat c = px * px + py * py;
   const GLfloat d = -(a * x0 + b * y0 + c * z0);
   if (a == 0.0 && b == 0.0 && c == 0.0 && d == 0.0) {
      plane[0] = 0.0;
      plane[1] = 0.0;
      plane[2] = 1.0;
      plane[3] = 0.0;
   }
   else {
      plane[0] = a;
      plane[1] = b;
      plane[2] = c;
      plane[3] = d;
   }
#endif
}


static INLINE void
constant_plane(GLfloat value, GLfloat plane[4])
{
   plane[0] = 0.0;
   plane[1] = 0.0;
   plane[2] = -1.0;
   plane[3] = value;
}


static INLINE GLfloat
solve_plane(GLfloat x, GLfloat y, const GLfloat plane[4])
{
   const GLfloat z = (plane[3] + plane[0] * x + plane[1] * y) / -plane[2];
   return z;
}

#define SOLVE_PLANE(X, Y, PLANE) \
   ((PLANE[3] + PLANE[0] * (X) + PLANE[1] * (Y)) / -PLANE[2])


/*
 * Return 1 / solve_plane().
 */
static INLINE GLfloat
solve_plane_recip(GLfloat x, GLfloat y, const GLfloat plane[4])
{
   const GLfloat denom = plane[3] + plane[0] * x + plane[1] * y;
   if (denom == 0.0)
      return 0.0;
   else
      return -plane[2] / denom;
}


/*
 * Solve plane and return clamped GLchan value.
 */
static INLINE GLchan
solve_plane_chan(GLfloat x, GLfloat y, const GLfloat plane[4])
{
   const GLfloat z = (plane[3] + plane[0] * x + plane[1] * y) / -plane[2];
#if CHAN_TYPE == GL_FLOAT
   return CLAMP(z, 0.0F, CHAN_MAXF);
#else
   if (z < 0)
      return 0;
   else if (z > CHAN_MAX)
      return CHAN_MAX;
   return (GLchan) IROUND_POS(z);
#endif
}


/*
 * Compute mipmap level of detail.
 */
static INLINE GLfloat
compute_lambda(const GLfloat sPlane[4], const GLfloat tPlane[4],
               GLfloat invQ, GLfloat width, GLfloat height)
{
   GLfloat dudx = sPlane[0] / sPlane[2] * invQ * width;
   GLfloat dudy = sPlane[1] / sPlane[2] * invQ * width;
   GLfloat dvdx = tPlane[0] / tPlane[2] * invQ * height;
   GLfloat dvdy = tPlane[1] / tPlane[2] * invQ * height;
   GLfloat r1 = dudx * dudx + dudy * dudy;
   GLfloat r2 = dvdx * dvdx + dvdy * dvdy;
   GLfloat rho2 = r1 + r2;
   /* return log base 2 of rho */
   if (rho2 == 0.0F)
      return 0.0;
   else
      return (GLfloat) (LOGF(rho2) * 1.442695 * 0.5);/* 1.442695 = 1/log(2) */
}




/*
 * Fill in the samples[] array with the (x,y) subpixel positions of
 * xSamples * ySamples sample positions.
 * Note that the four corner samples are put into the first four
 * positions of the array.  This allows us to optimize for the common
 * case of all samples being inside the polygon.
 */
static void
make_sample_table(GLint xSamples, GLint ySamples, GLfloat samples[][2])
{
   const GLfloat dx = 1.0F / (GLfloat) xSamples;
   const GLfloat dy = 1.0F / (GLfloat) ySamples;
   GLint x, y;
   GLint i;

   i = 4;
   for (x = 0; x < xSamples; x++) {
      for (y = 0; y < ySamples; y++) {
         GLint j;
         if (x == 0 && y == 0) {
            /* lower left */
            j = 0;
         }
         else if (x == xSamples - 1 && y == 0) {
            /* lower right */
            j = 1;
         }
         else if (x == 0 && y == ySamples - 1) {
            /* upper left */
            j = 2;
         }
         else if (x == xSamples - 1 && y == ySamples - 1) {
            /* upper right */
            j = 3;
         }
         else {
            j = i++;
         }
         samples[j][0] = x * dx + 0.5F * dx;
         samples[j][1] = y * dy + 0.5F * dy;
      }
   }
}



/*
 * Compute how much of the given pixel's area is inside the rectangle
 * defined by vertices v0, v1, v2, v3.
 * Vertices MUST be specified in counter-clockwise order.
 * Return:  coverage in [0, 1].
 */
static GLfloat
compute_coveragef(const struct LineInfo *info,
                  GLint winx, GLint winy)
{
   static GLfloat samples[SUB_PIXEL * SUB_PIXEL][2];
   static GLboolean haveSamples = GL_FALSE;
   const GLfloat x = (GLfloat) winx;
   const GLfloat y = (GLfloat) winy;
   GLint stop = 4, i;
   GLfloat insideCount = SUB_PIXEL * SUB_PIXEL;

   if (!haveSamples) {
      make_sample_table(SUB_PIXEL, SUB_PIXEL, samples);
      haveSamples = GL_TRUE;
   }

#if 0 /*DEBUG*/
   {
      const GLfloat area = dx0 * dy1 - dx1 * dy0;
      assert(area >= 0.0);
   }
#endif

   for (i = 0; i < stop; i++) {
      const GLfloat sx = x + samples[i][0];
      const GLfloat sy = y + samples[i][1];
      const GLfloat fx0 = sx - info->qx0;
      const GLfloat fy0 = sy - info->qy0;
      const GLfloat fx1 = sx - info->qx1;
      const GLfloat fy1 = sy - info->qy1;
      const GLfloat fx2 = sx - info->qx2;
      const GLfloat fy2 = sy - info->qy2;
      const GLfloat fx3 = sx - info->qx3;
      const GLfloat fy3 = sy - info->qy3;
      /* cross product determines if sample is inside or outside each edge */
      GLfloat cross0 = (info->ex0 * fy0 - info->ey0 * fx0);
      GLfloat cross1 = (info->ex1 * fy1 - info->ey1 * fx1);
      GLfloat cross2 = (info->ex2 * fy2 - info->ey2 * fx2);
      GLfloat cross3 = (info->ex3 * fy3 - info->ey3 * fx3);
      /* Check if the sample is exactly on an edge.  If so, let cross be a
       * positive or negative value depending on the direction of the edge.
       */
      if (cross0 == 0.0F)
         cross0 = info->ex0 + info->ey0;
      if (cross1 == 0.0F)
         cross1 = info->ex1 + info->ey1;
      if (cross2 == 0.0F)
         cross2 = info->ex2 + info->ey2;
      if (cross3 == 0.0F)
         cross3 = info->ex3 + info->ey3;
      if (cross0 < 0.0F || cross1 < 0.0F || cross2 < 0.0F || cross3 < 0.0F) {
         /* point is outside quadrilateral */
         insideCount -= 1.0F;
         stop = SUB_PIXEL * SUB_PIXEL;
      }
   }
   if (stop == 4)
      return 1.0F;
   else
      return insideCount * (1.0F / (SUB_PIXEL * SUB_PIXEL));
}


/**
 * Compute coverage value for color index mode.
 * XXX this may not be quite correct.
 * \return coverage in [0,15].
 */
static GLfloat
compute_coveragei(const struct LineInfo *info,
                  GLint winx, GLint winy)
{
   return compute_coveragef(info, winx, winy) * 15.0F;
}



typedef void (*plot_func)(GLcontext *ctx, struct LineInfo *line,
                          int ix, int iy);
                         


/*
 * Draw an AA line segment (called many times per line when stippling)
 */
static void
segment(GLcontext *ctx,
        struct LineInfo *line,
        plot_func plot,
        GLfloat t0, GLfloat t1)
{
   const GLfloat absDx = (line->dx < 0.0F) ? -line->dx : line->dx;
   const GLfloat absDy = (line->dy < 0.0F) ? -line->dy : line->dy;
   /* compute the actual segment's endpoints */
   const GLfloat x0 = line->x0 + t0 * line->dx;
   const GLfloat y0 = line->y0 + t0 * line->dy;
   const GLfloat x1 = line->x0 + t1 * line->dx;
   const GLfloat y1 = line->y0 + t1 * line->dy;

   /* compute vertices of the line-aligned quadrilateral */
   line->qx0 = x0 - line->yAdj;
   line->qy0 = y0 + line->xAdj;
   line->qx1 = x0 + line->yAdj;
   line->qy1 = y0 - line->xAdj;
   line->qx2 = x1 + line->yAdj;
   line->qy2 = y1 - line->xAdj;
   line->qx3 = x1 - line->yAdj;
   line->qy3 = y1 + line->xAdj;
   /* compute the quad's edge vectors (for coverage calc) */
   line->ex0 = line->qx1 - line->qx0;
   line->ey0 = line->qy1 - line->qy0;
   line->ex1 = line->qx2 - line->qx1;
   line->ey1 = line->qy2 - line->qy1;
   line->ex2 = line->qx3 - line->qx2;
   line->ey2 = line->qy3 - line->qy2;
   line->ex3 = line->qx0 - line->qx3;
   line->ey3 = line->qy0 - line->qy3;

   if (absDx > absDy) {
      /* X-major line */
      GLfloat dydx = line->dy / line->dx;
      GLfloat xLeft, xRight, yBot, yTop;
      GLint ix, ixRight;
      if (x0 < x1) {
         xLeft = x0 - line->halfWidth;
         xRight = x1 + line->halfWidth;
         if (line->dy >= 0.0) {
            yBot = y0 - 3.0F * line->halfWidth;
            yTop = y0 + line->halfWidth;
         }
         else {
            yBot = y0 - line->halfWidth;
            yTop = y0 + 3.0F * line->halfWidth;
         }
      }
      else {
         xLeft = x1 - line->halfWidth;
         xRight = x0 + line->halfWidth;
         if (line->dy <= 0.0) {
            yBot = y1 - 3.0F * line->halfWidth;
            yTop = y1 + line->halfWidth;
         }
         else {
            yBot = y1 - line->halfWidth;
            yTop = y1 + 3.0F * line->halfWidth;
         }
      }

      /* scan along the line, left-to-right */
      ixRight = (GLint) (xRight + 1.0F);

      /*printf("avg span height: %g\n", yTop - yBot);*/
      for (ix = (GLint) xLeft; ix < ixRight; ix++) {
         const GLint iyBot = (GLint) yBot;
         const GLint iyTop = (GLint) (yTop + 1.0F);
         GLint iy;
         /* scan across the line, bottom-to-top */
         for (iy = iyBot; iy < iyTop; iy++) {
            (*plot)(ctx, line, ix, iy);
         }
         yBot += dydx;
         yTop += dydx;
      }
   }
   else {
      /* Y-major line */
      GLfloat dxdy = line->dx / line->dy;
      GLfloat yBot, yTop, xLeft, xRight;
      GLint iy, iyTop;
      if (y0 < y1) {
         yBot = y0 - line->halfWidth;
         yTop = y1 + line->halfWidth;
         if (line->dx >= 0.0) {
            xLeft = x0 - 3.0F * line->halfWidth;
            xRight = x0 + line->halfWidth;
         }
         else {
            xLeft = x0 - line->halfWidth;
            xRight = x0 + 3.0F * line->halfWidth;
         }
      }
      else {
         yBot = y1 - line->halfWidth;
         yTop = y0 + line->halfWidth;
         if (line->dx <= 0.0) {
            xLeft = x1 - 3.0F * line->halfWidth;
            xRight = x1 + line->halfWidth;
         }
         else {
            xLeft = x1 - line->halfWidth;
            xRight = x1 + 3.0F * line->halfWidth;
         }
      }

      /* scan along the line, bottom-to-top */
      iyTop = (GLint) (yTop + 1.0F);

      /*printf("avg span width: %g\n", xRight - xLeft);*/
      for (iy = (GLint) yBot; iy < iyTop; iy++) {
         const GLint ixLeft = (GLint) xLeft;
         const GLint ixRight = (GLint) (xRight + 1.0F);
         GLint ix;
         /* scan across the line, left-to-right */
         for (ix = ixLeft; ix < ixRight; ix++) {
            (*plot)(ctx, line, ix, iy);
         }
         xLeft += dxdy;
         xRight += dxdy;
      }
   }
}


#define NAME(x) aa_ci_##x
#define DO_Z
#define DO_ATTRIBS /* for fog */
#define DO_INDEX
#include "s_aalinetemp.h"


#define NAME(x) aa_rgba_##x
#define DO_Z
#define DO_RGBA
#include "s_aalinetemp.h"


#define NAME(x)  aa_general_rgba_##x
#define DO_Z
#define DO_RGBA
#define DO_ATTRIBS
#include "s_aalinetemp.h"



void
_swrast_choose_aa_line_function(GLcontext *ctx)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   ASSERT(ctx->Line.SmoothFlag);

   if (ctx->Visual.rgbMode) {
      /* RGBA */
      if (ctx->Texture._EnabledCoordUnits != 0
          || ctx->FragmentProgram._Current
          || (ctx->Light.Enabled &&
              ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)
          || ctx->Fog.ColorSumEnabled
          || swrast->_FogEnabled) {
         swrast->Line = aa_general_rgba_line;
      }
      else {
         swrast->Line = aa_rgba_line;
      }
   }
   else {
      /* Color Index */
      swrast->Line = aa_ci_line;
   }
}
