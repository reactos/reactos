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
#include "clip.h"
#include "context.h"
#include "macros.h"
#include "mtypes.h"

#include "math/m_matrix.h"


/**
 * Update derived clip plane state.
 */
void
_mesa_update_clip_plane(struct gl_context *ctx, GLuint plane)
{
   if (_math_matrix_is_dirty(ctx->ProjectionMatrixStack.Top))
      _math_matrix_analyse( ctx->ProjectionMatrixStack.Top );

   /* Clip-Space Plane = Eye-Space Plane * Projection Matrix */
   _mesa_transform_vector(ctx->Transform._ClipUserPlane[plane],
                          ctx->Transform.EyeUserPlane[plane],
                          ctx->ProjectionMatrixStack.Top->inv);
}


void GLAPIENTRY
_mesa_ClipPlane( GLenum plane, const GLdouble *eq )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint p;
   GLfloat equation[4];
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   p = (GLint) plane - (GLint) GL_CLIP_PLANE0;
   if (p < 0 || p >= (GLint) ctx->Const.MaxClipPlanes) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glClipPlane" );
      return;
   }

   equation[0] = (GLfloat) eq[0];
   equation[1] = (GLfloat) eq[1];
   equation[2] = (GLfloat) eq[2];
   equation[3] = (GLfloat) eq[3];

   /*
    * The equation is transformed by the transpose of the inverse of the
    * current modelview matrix and stored in the resulting eye coordinates.
    *
    * KW: Eqn is then transformed to the current clip space, where user
    * clipping now takes place.  The clip-space equations are recalculated
    * whenever the projection matrix changes.
    */
   if (_math_matrix_is_dirty(ctx->ModelviewMatrixStack.Top))
      _math_matrix_analyse( ctx->ModelviewMatrixStack.Top );

   _mesa_transform_vector( equation, equation,
                           ctx->ModelviewMatrixStack.Top->inv );

   if (TEST_EQ_4V(ctx->Transform.EyeUserPlane[p], equation))
      return;

   FLUSH_VERTICES(ctx, _NEW_TRANSFORM);
   COPY_4FV(ctx->Transform.EyeUserPlane[p], equation);

   if (ctx->Transform.ClipPlanesEnabled & (1 << p)) {
      _mesa_update_clip_plane(ctx, p);
   }

   if (ctx->Driver.ClipPlane)
      ctx->Driver.ClipPlane( ctx, plane, equation );
}


void GLAPIENTRY
_mesa_GetClipPlane( GLenum plane, GLdouble *equation )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint p;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   p = (GLint) (plane - GL_CLIP_PLANE0);
   if (p < 0 || p >= (GLint) ctx->Const.MaxClipPlanes) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetClipPlane" );
      return;
   }

   equation[0] = (GLdouble) ctx->Transform.EyeUserPlane[p][0];
   equation[1] = (GLdouble) ctx->Transform.EyeUserPlane[p][1];
   equation[2] = (GLdouble) ctx->Transform.EyeUserPlane[p][2];
   equation[3] = (GLdouble) ctx->Transform.EyeUserPlane[p][3];
}
