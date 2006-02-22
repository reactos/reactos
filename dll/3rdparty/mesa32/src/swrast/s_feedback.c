/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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


#define FB_3D		0x01
#define FB_4D		0x02
#define FB_INDEX	0x04
#define FB_COLOR	0x08
#define FB_TEXTURE	0X10




static void feedback_vertex( GLcontext *ctx,
                             const SWvertex *v, const SWvertex *pv )
{
   const GLuint texUnit = 0;  /* See section 5.3 of 1.2.1 spec */
   GLfloat win[4];
   GLfloat color[4];
   GLfloat tc[4];

   win[0] = v->win[0];
   win[1] = v->win[1];
   win[2] = v->win[2] / ctx->DrawBuffer->_DepthMaxF;
   win[3] = 1.0F / v->win[3];

   color[0] = CHAN_TO_FLOAT(pv->color[0]);
   color[1] = CHAN_TO_FLOAT(pv->color[1]);
   color[2] = CHAN_TO_FLOAT(pv->color[2]);
   color[3] = CHAN_TO_FLOAT(pv->color[3]);

   if (v->texcoord[texUnit][3] != 1.0 &&
       v->texcoord[texUnit][3] != 0.0) {
      GLfloat invq = 1.0F / v->texcoord[texUnit][3];
      tc[0] = v->texcoord[texUnit][0] * invq;
      tc[1] = v->texcoord[texUnit][1] * invq;
      tc[2] = v->texcoord[texUnit][2] * invq;
      tc[3] = v->texcoord[texUnit][3];
   }
   else {
      COPY_4V(tc, v->texcoord[texUnit]);
   }

   _mesa_feedback_vertex( ctx, win, color, (GLfloat) v->index, tc );
}


/*
 * Put triangle in feedback buffer.
 */
void _swrast_feedback_triangle( GLcontext *ctx,
                           const SWvertex *v0,
                           const SWvertex *v1,
			   const SWvertex *v2)
{
   if (_swrast_culltriangle( ctx, v0, v1, v2 )) {
      FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_POLYGON_TOKEN );
      FEEDBACK_TOKEN( ctx, (GLfloat) 3 );        /* three vertices */

      if (ctx->Light.ShadeModel == GL_SMOOTH) {
	 feedback_vertex( ctx, v0, v0 );
	 feedback_vertex( ctx, v1, v1 );
	 feedback_vertex( ctx, v2, v2 );
      } else {
	 feedback_vertex( ctx, v0, v2 );
	 feedback_vertex( ctx, v1, v2 );
	 feedback_vertex( ctx, v2, v2 );
      }
   }
}


void _swrast_feedback_line( GLcontext *ctx, const SWvertex *v0, const SWvertex *v1 )
{
   GLenum token = GL_LINE_TOKEN;
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   if (swrast->StippleCounter==0)
      token = GL_LINE_RESET_TOKEN;

   FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) token );

   if (ctx->Light.ShadeModel == GL_SMOOTH) {
      feedback_vertex( ctx, v0, v0 );
      feedback_vertex( ctx, v1, v1 );
   } else {
      feedback_vertex( ctx, v0, v1 );
      feedback_vertex( ctx, v1, v1 );
   }

   swrast->StippleCounter++;
}


void _swrast_feedback_point( GLcontext *ctx, const SWvertex *v )
{
   FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_POINT_TOKEN );
   feedback_vertex( ctx, v, v );
}


void _swrast_select_triangle( GLcontext *ctx,
                         const SWvertex *v0,
                         const SWvertex *v1,
			 const SWvertex *v2)
{
   if (_swrast_culltriangle( ctx, v0, v1, v2 )) {
      const GLfloat zs = 1.0F / ctx->DrawBuffer->_DepthMaxF;

      _mesa_update_hitflag( ctx, v0->win[2] * zs );
      _mesa_update_hitflag( ctx, v1->win[2] * zs );
      _mesa_update_hitflag( ctx, v2->win[2] * zs );
   }
}


void _swrast_select_line( GLcontext *ctx, const SWvertex *v0, const SWvertex *v1 )
{
   const GLfloat zs = 1.0F / ctx->DrawBuffer->_DepthMaxF;
   _mesa_update_hitflag( ctx, v0->win[2] * zs );
   _mesa_update_hitflag( ctx, v1->win[2] * zs );
}


void _swrast_select_point( GLcontext *ctx, const SWvertex *v )
{
   const GLfloat zs = 1.0F / ctx->DrawBuffer->_DepthMaxF;
   _mesa_update_hitflag( ctx, v->win[2] * zs );
}
