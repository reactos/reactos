/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
 *    Keith Whitwell <keith@tungstengraphics.com>
 */


#include "main/glheader.h"
#include "main/colormac.h"
#include "main/macros.h"
#include "main/imports.h"
#include "main/mtypes.h"

#include "math/m_xform.h"

#include "t_context.h"
#include "t_pipeline.h"


struct normal_stage_data {
   normal_func NormalTransform;
   GLvector4f normal;
};

#define NORMAL_STAGE_DATA(stage) ((struct normal_stage_data *)stage->privatePtr)


static GLboolean
run_normal_stage(struct gl_context *ctx, struct tnl_pipeline_stage *stage)
{
   struct normal_stage_data *store = NORMAL_STAGE_DATA(stage);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   const GLfloat *lengths;

   if (!store->NormalTransform)
      return GL_TRUE;

   /* We can only use the display list's saved normal lengths if we've
    * got a transformation matrix with uniform scaling.
    */
   if (_math_matrix_is_general_scale(ctx->ModelviewMatrixStack.Top))
      lengths = NULL;
   else
      lengths = VB->NormalLengthPtr;

   store->NormalTransform( ctx->ModelviewMatrixStack.Top,
			   ctx->_ModelViewInvScale,
			   VB->AttribPtr[_TNL_ATTRIB_NORMAL],  /* input normals */
			   lengths,
			   &store->normal ); /* resulting normals */

   if (VB->AttribPtr[_TNL_ATTRIB_NORMAL]->count > 1) {
      store->normal.stride = 4 * sizeof(GLfloat);
   }
   else {
      store->normal.stride = 0;
   }

   VB->AttribPtr[_TNL_ATTRIB_NORMAL] = &store->normal;

   VB->NormalLengthPtr = NULL;	/* no longer valid */
   return GL_TRUE;
}


/**
 * Examine current GL state and set the store->NormalTransform pointer
 * to point to the appropriate normal transformation routine.
 */
static void
validate_normal_stage(struct gl_context *ctx, struct tnl_pipeline_stage *stage)
{
   struct normal_stage_data *store = NORMAL_STAGE_DATA(stage);

   if (ctx->VertexProgram._Current ||
       (!ctx->Light.Enabled &&
	!(ctx->Texture._GenFlags & TEXGEN_NEED_NORMALS))) {
      store->NormalTransform = NULL;
      return;
   }

   if (ctx->_NeedEyeCoords) {
      /* Eye coordinates are needed, for whatever reasons.
       * Do lighting in eye coordinates, as the GL spec says.
       */
      GLuint transform = NORM_TRANSFORM_NO_ROT;

      if (_math_matrix_has_rotation(ctx->ModelviewMatrixStack.Top)) {
         /* need to do full (3x3) matrix transform */
	 transform = NORM_TRANSFORM;
      }

      if (ctx->Transform.Normalize) {
	 store->NormalTransform = _mesa_normal_tab[transform | NORM_NORMALIZE];
      }
      else if (ctx->Transform.RescaleNormals &&
               ctx->_ModelViewInvScale != 1.0) {
	 store->NormalTransform = _mesa_normal_tab[transform | NORM_RESCALE];
      }
      else {
	 store->NormalTransform = _mesa_normal_tab[transform];
      }
   }
   else {
      /* We don't need eye coordinates.
       * Do lighting in object coordinates.  Thus, we don't need to fully
       * transform normal vectors (just leave them in object coordinates)
       * but we still need to do normalization/rescaling if enabled.
       */
      if (ctx->Transform.Normalize) {
	 store->NormalTransform = _mesa_normal_tab[NORM_NORMALIZE];
      }
      else if (!ctx->Transform.RescaleNormals &&
	       ctx->_ModelViewInvScale != 1.0) {
	 store->NormalTransform = _mesa_normal_tab[NORM_RESCALE];
      }
      else {
	 store->NormalTransform = NULL;
      }
   }
}


/**
 * Allocate stage's private data (storage for transformed normals).
 */
static GLboolean
alloc_normal_data(struct gl_context *ctx, struct tnl_pipeline_stage *stage)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct normal_stage_data *store;

   stage->privatePtr = malloc(sizeof(*store));
   store = NORMAL_STAGE_DATA(stage);
   if (!store)
      return GL_FALSE;

   _mesa_vector4f_alloc( &store->normal, 0, tnl->vb.Size, 32 );
   return GL_TRUE;
}


/**
 * Free stage's private data.
 */
static void
free_normal_data(struct tnl_pipeline_stage *stage)
{
   struct normal_stage_data *store = NORMAL_STAGE_DATA(stage);
   if (store) {
      _mesa_vector4f_free( &store->normal );
      free( store );
      stage->privatePtr = NULL;
   }
}


const struct tnl_pipeline_stage _tnl_normal_transform_stage =
{
   "normal transform",		/* name */
   NULL,			/* privatePtr */
   alloc_normal_data,		/* create */
   free_normal_data,		/* destroy */
   validate_normal_stage,	/* validate */
   run_normal_stage             /* run */
};
