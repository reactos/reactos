/*
 * Mesa 3-D graphics library
 * Version:  7.1
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
#include "main/context.h"
#include "main/colormac.h"
#include "main/macros.h"
#include "s_aaline.h"
#include "s_context.h"
#include "s_depth.h"
#include "s_feedback.h"
#include "s_lines.h"
#include "s_span.h"


/*
 * Init the mask[] array to implement a line stipple.
 */
static void
compute_stipple_mask( GLcontext *ctx, GLuint len, GLubyte mask[] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLuint i;

   for (i = 0; i < len; i++) {
      GLuint bit = (swrast->StippleCounter / ctx->Line.StippleFactor) & 0xf;
      if ((1 << bit) & ctx->Line.StipplePattern) {
         mask[i] = GL_TRUE;
      }
      else {
         mask[i] = GL_FALSE;
      }
      swrast->StippleCounter++;
   }
}


/*
 * To draw a wide line we can simply redraw the span N times, side by side.
 */
static void
draw_wide_line( GLcontext *ctx, SWspan *span, GLboolean xMajor )
{
   const GLint width = (GLint) CLAMP(ctx->Line.Width,
                                     ctx->Const.MinLineWidth,
                                     ctx->Const.MaxLineWidth);
   GLint start;

   ASSERT(span->end < MAX_WIDTH);

   if (width & 1)
      start = width / 2;
   else
      start = width / 2 - 1;

   if (xMajor) {
      GLint *y = span->array->y;
      GLuint i;
      GLint w;
      for (w = 0; w < width; w++) {
         if (w == 0) {
            for (i = 0; i < span->end; i++)
               y[i] -= start;
         }
         else {
            for (i = 0; i < span->end; i++)
               y[i]++;
         }
         if (ctx->Visual.rgbMode)
            _swrast_write_rgba_span(ctx, span);
         else
            _swrast_write_index_span(ctx, span);
      }
   }
   else {
      GLint *x = span->array->x;
      GLuint i;
      GLint w;
      for (w = 0; w < width; w++) {
         if (w == 0) {
            for (i = 0; i < span->end; i++)
               x[i] -= start;
         }
         else {
            for (i = 0; i < span->end; i++)
               x[i]++;
         }
         if (ctx->Visual.rgbMode)
            _swrast_write_rgba_span(ctx, span);
         else
            _swrast_write_index_span(ctx, span);
      }
   }
}



/**********************************************************************/
/*****                    Rasterization                           *****/
/**********************************************************************/

/* Simple color index line (no stipple, width=1, no Z, no fog, no tex)*/
#define NAME simple_no_z_ci_line
#define INTERP_INDEX
#define RENDER_SPAN(span) _swrast_write_index_span(ctx, &span)
#include "s_linetemp.h"

/* Simple RGBA index line (no stipple, width=1, no Z, no fog, no tex)*/
#define NAME simple_no_z_rgba_line
#define INTERP_RGBA
#define RENDER_SPAN(span) _swrast_write_rgba_span(ctx, &span);
#include "s_linetemp.h"


/* Z, fog, wide, stipple color index line */
#define NAME ci_line
#define INTERP_INDEX
#define INTERP_Z
#define INTERP_ATTRIBS /* for fog */
#define RENDER_SPAN(span)					\
   if (ctx->Line.StippleFlag) {					\
      span.arrayMask |= SPAN_MASK;				\
      compute_stipple_mask(ctx, span.end, span.array->mask);    \
   }								\
   if (ctx->Line.Width > 1.0) {					\
      draw_wide_line(ctx, &span, (GLboolean)(dx > dy));		\
   }								\
   else {							\
      _swrast_write_index_span(ctx, &span);			\
   }
#include "s_linetemp.h"


/* Z, fog, wide, stipple RGBA line */
#define NAME rgba_line
#define INTERP_RGBA
#define INTERP_Z
#define RENDER_SPAN(span)					\
   if (ctx->Line.StippleFlag) {					\
      span.arrayMask |= SPAN_MASK;				\
      compute_stipple_mask(ctx, span.end, span.array->mask);	\
   }								\
   if (ctx->Line.Width > 1.0) {					\
      draw_wide_line(ctx, &span, (GLboolean)(dx > dy));		\
   }								\
   else {							\
      _swrast_write_rgba_span(ctx, &span);			\
   }
#include "s_linetemp.h"


/* General-purpose line (any/all features). */
#define NAME general_line
#define INTERP_RGBA
#define INTERP_Z
#define INTERP_ATTRIBS
#define RENDER_SPAN(span)					\
   if (ctx->Line.StippleFlag) {					\
      span.arrayMask |= SPAN_MASK;				\
      compute_stipple_mask(ctx, span.end, span.array->mask);	\
   }								\
   if (ctx->Line.Width > 1.0) {					\
      draw_wide_line(ctx, &span, (GLboolean)(dx > dy));		\
   }								\
   else {							\
      _swrast_write_rgba_span(ctx, &span);			\
   }
#include "s_linetemp.h"



void
_swrast_add_spec_terms_line(GLcontext *ctx,
                            const SWvertex *v0, const SWvertex *v1)
{
   SWvertex *ncv0 = (SWvertex *)v0;
   SWvertex *ncv1 = (SWvertex *)v1;
   GLfloat rSum, gSum, bSum;
   GLchan cSave[2][4];

   /* save original colors */
   COPY_CHAN4(cSave[0], ncv0->color);
   COPY_CHAN4(cSave[1], ncv1->color);
   /* sum v0 */
   rSum = CHAN_TO_FLOAT(ncv0->color[0]) + ncv0->attrib[FRAG_ATTRIB_COL1][0];
   gSum = CHAN_TO_FLOAT(ncv0->color[1]) + ncv0->attrib[FRAG_ATTRIB_COL1][1];
   bSum = CHAN_TO_FLOAT(ncv0->color[2]) + ncv0->attrib[FRAG_ATTRIB_COL1][2];
   UNCLAMPED_FLOAT_TO_CHAN(ncv0->color[0], rSum);
   UNCLAMPED_FLOAT_TO_CHAN(ncv0->color[1], gSum);
   UNCLAMPED_FLOAT_TO_CHAN(ncv0->color[2], bSum);
   /* sum v1 */
   rSum = CHAN_TO_FLOAT(ncv1->color[0]) + ncv1->attrib[FRAG_ATTRIB_COL1][0];
   gSum = CHAN_TO_FLOAT(ncv1->color[1]) + ncv1->attrib[FRAG_ATTRIB_COL1][1];
   bSum = CHAN_TO_FLOAT(ncv1->color[2]) + ncv1->attrib[FRAG_ATTRIB_COL1][2];
   UNCLAMPED_FLOAT_TO_CHAN(ncv1->color[0], rSum);
   UNCLAMPED_FLOAT_TO_CHAN(ncv1->color[1], gSum);
   UNCLAMPED_FLOAT_TO_CHAN(ncv1->color[2], bSum);
   /* draw */
   SWRAST_CONTEXT(ctx)->SpecLine( ctx, ncv0, ncv1 );
   /* restore original colors */
   COPY_CHAN4( ncv0->attrib[FRAG_ATTRIB_COL0], cSave[0] );
   COPY_CHAN4( ncv1->attrib[FRAG_ATTRIB_COL0], cSave[1] );
}



#ifdef DEBUG

/* record the current line function name */
static const char *lineFuncName = NULL;

#define USE(lineFunc)                   \
do {                                    \
    lineFuncName = #lineFunc;           \
    /*_mesa_printf("%s\n", lineFuncName);*/   \
    swrast->Line = lineFunc;            \
} while (0)

#else

#define USE(lineFunc)  swrast->Line = lineFunc

#endif



/**
 * Determine which line drawing function to use given the current
 * rendering context.
 *
 * Please update the summary flag _SWRAST_NEW_LINE if you add or remove
 * tests to this code.
 */
void
_swrast_choose_line( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLboolean rgbmode = ctx->Visual.rgbMode;
   GLboolean specular = (ctx->Fog.ColorSumEnabled ||
                         (ctx->Light.Enabled &&
                          ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR));

   if (ctx->RenderMode == GL_RENDER) {
      if (ctx->Line.SmoothFlag) {
         /* antialiased lines */
         _swrast_choose_aa_line_function(ctx);
         ASSERT(swrast->Line);
      }
      else if (ctx->Texture._EnabledCoordUnits
               || ctx->FragmentProgram._Current
               || swrast->_FogEnabled
               || specular) {
         USE(general_line);
      }
      else if (ctx->Depth.Test
               || ctx->Line.Width != 1.0
               || ctx->Line.StippleFlag) {
         /* no texture, but Z, fog, width>1, stipple, etc. */
         if (rgbmode)
#if CHAN_BITS == 32
            USE(general_line);
#else
            USE(rgba_line);
#endif
         else
            USE(ci_line);
      }
      else {
         ASSERT(!ctx->Depth.Test);
         ASSERT(ctx->Line.Width == 1.0);
         /* simple lines */
         if (rgbmode)
            USE(simple_no_z_rgba_line);
         else
            USE(simple_no_z_ci_line);
      }
   }
   else if (ctx->RenderMode == GL_FEEDBACK) {
      USE(_swrast_feedback_line);
   }
   else {
      ASSERT(ctx->RenderMode == GL_SELECT);
      USE(_swrast_select_line);
   }
}
