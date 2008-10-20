/*
 * Mesa 3-D graphics library
 * Version:  7.0
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

#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "enums.h"
#include "feedback.h"
#include "macros.h"

#include "s_context.h"
#include "s_feedback.h"
#include "s_triangle.h"



static void
feedback_vertex(GLcontext * ctx, const SWvertex * v, const SWvertex * pv)
{
   GLfloat win[4];
   const GLfloat *vtc = v->attrib[FRAG_ATTRIB_TEX0];
   const GLfloat *color = v->attrib[FRAG_ATTRIB_COL0];

   win[0] = v->attrib[FRAG_ATTRIB_WPOS][0];
   win[1] = v->attrib[FRAG_ATTRIB_WPOS][1];
   win[2] = v->attrib[FRAG_ATTRIB_WPOS][2] / ctx->DrawBuffer->_DepthMaxF;
   win[3] = 1.0F / v->attrib[FRAG_ATTRIB_WPOS][3];

   _mesa_feedback_vertex(ctx, win, color, v->attrib[FRAG_ATTRIB_CI][0], vtc);
}


/*
 * Put triangle in feedback buffer.
 */
void
_swrast_feedback_triangle(GLcontext *ctx, const SWvertex *v0,
                          const SWvertex *v1, const SWvertex *v2)
{
   if (_swrast_culltriangle(ctx, v0, v1, v2)) {
      FEEDBACK_TOKEN(ctx, (GLfloat) (GLint) GL_POLYGON_TOKEN);
      FEEDBACK_TOKEN(ctx, (GLfloat) 3); /* three vertices */

      if (ctx->Light.ShadeModel == GL_SMOOTH) {
         feedback_vertex(ctx, v0, v0);
         feedback_vertex(ctx, v1, v1);
         feedback_vertex(ctx, v2, v2);
      }
      else {
         feedback_vertex(ctx, v0, v2);
         feedback_vertex(ctx, v1, v2);
         feedback_vertex(ctx, v2, v2);
      }
   }
}


void
_swrast_feedback_line(GLcontext *ctx, const SWvertex *v0,
                      const SWvertex *v1)
{
   GLenum token = GL_LINE_TOKEN;
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   if (swrast->StippleCounter == 0)
      token = GL_LINE_RESET_TOKEN;

   FEEDBACK_TOKEN(ctx, (GLfloat) (GLint) token);

   if (ctx->Light.ShadeModel == GL_SMOOTH) {
      feedback_vertex(ctx, v0, v0);
      feedback_vertex(ctx, v1, v1);
   }
   else {
      feedback_vertex(ctx, v0, v1);
      feedback_vertex(ctx, v1, v1);
   }

   swrast->StippleCounter++;
}


void
_swrast_feedback_point(GLcontext *ctx, const SWvertex *v)
{
   FEEDBACK_TOKEN(ctx, (GLfloat) (GLint) GL_POINT_TOKEN);
   feedback_vertex(ctx, v, v);
}


void
_swrast_select_triangle(GLcontext *ctx, const SWvertex *v0,
                        const SWvertex *v1, const SWvertex *v2)
{
   if (_swrast_culltriangle(ctx, v0, v1, v2)) {
      const GLfloat zs = 1.0F / ctx->DrawBuffer->_DepthMaxF;

      _mesa_update_hitflag( ctx, v0->attrib[FRAG_ATTRIB_WPOS][2] * zs );
      _mesa_update_hitflag( ctx, v1->attrib[FRAG_ATTRIB_WPOS][2] * zs );
      _mesa_update_hitflag( ctx, v2->attrib[FRAG_ATTRIB_WPOS][2] * zs );
   }
}


void
_swrast_select_line(GLcontext *ctx, const SWvertex *v0, const SWvertex *v1)
{
   const GLfloat zs = 1.0F / ctx->DrawBuffer->_DepthMaxF;
   _mesa_update_hitflag( ctx, v0->attrib[FRAG_ATTRIB_WPOS][2] * zs );
   _mesa_update_hitflag( ctx, v1->attrib[FRAG_ATTRIB_WPOS][2] * zs );
}


void
_swrast_select_point(GLcontext *ctx, const SWvertex *v)
{
   const GLfloat zs = 1.0F / ctx->DrawBuffer->_DepthMaxF;
   _mesa_update_hitflag( ctx, v->attrib[FRAG_ATTRIB_WPOS][2] * zs );
}
