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
 *
 * Authors:
 *    Brian Paul
 */

#include "main/mtypes.h"
#include "main/imports.h"
#include "t_context.h"
#include "t_pipeline.h"


struct point_stage_data {
   GLvector4f PointSize;
};

#define POINT_STAGE_DATA(stage) ((struct point_stage_data *)stage->privatePtr)


/**
 * Compute point size for each vertex from the vertex eye-space Z
 * coordinate and the point size attenuation factors.
 * Only done when point size attenuation is enabled and vertex program is
 * disabled.
 */
static GLboolean
run_point_stage(GLcontext *ctx, struct tnl_pipeline_stage *stage)
{
   if (ctx->Point._Attenuated && !ctx->VertexProgram._Current) {
      struct point_stage_data *store = POINT_STAGE_DATA(stage);
      struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
      const GLfloat *eyeCoord = (GLfloat *) VB->EyePtr->data + 2;
      const GLint eyeCoordStride = VB->EyePtr->stride / sizeof(GLfloat);
      const GLfloat p0 = ctx->Point.Params[0];
      const GLfloat p1 = ctx->Point.Params[1];
      const GLfloat p2 = ctx->Point.Params[2];
      const GLfloat pointSize = ctx->Point.Size;
      GLfloat (*size)[4] = store->PointSize.data;
      GLuint i;

      for (i = 0; i < VB->Count; i++) {
         const GLfloat dist = FABSF(*eyeCoord);
         const GLfloat q = p0 + dist * (p1 + dist * p2);
         const GLfloat atten = (q != 0.0) ? SQRTF(1.0 / q) : 1.0;
         size[i][0] = pointSize * atten; /* clamping done in rasterization */
         eyeCoord += eyeCoordStride;
      }

      VB->AttribPtr[_TNL_ATTRIB_POINTSIZE] = &store->PointSize;
   }

   return GL_TRUE;
}


static GLboolean
alloc_point_data(GLcontext *ctx, struct tnl_pipeline_stage *stage)
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   struct point_stage_data *store;
   stage->privatePtr = _mesa_malloc(sizeof(*store));
   store = POINT_STAGE_DATA(stage);
   if (!store)
      return GL_FALSE;

   _mesa_vector4f_alloc( &store->PointSize, 0, VB->Size, 32 );
   return GL_TRUE;
}


static void
free_point_data(struct tnl_pipeline_stage *stage)
{
   struct point_stage_data *store = POINT_STAGE_DATA(stage);
   if (store) {
      _mesa_vector4f_free( &store->PointSize );
      _mesa_free( store );
      stage->privatePtr = NULL;
   }
}


const struct tnl_pipeline_stage _tnl_point_attenuation_stage =
{
   "point size attenuation",	/* name */
   NULL,			/* stage private data */
   alloc_point_data,		/* alloc data */
   free_point_data,		/* destructor */
   NULL,
   run_point_stage		/* run */
};
