/*
 * Mesa 3-D graphics library
 * Version:  6.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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


#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "macros.h"
#include "imports.h"
#include "mtypes.h"

#include "math/m_xform.h"

#include "t_context.h"
#include "t_pipeline.h"



struct normal_stage_data {
   normal_func NormalTransform;
   GLvector4f normal;
};

#define NORMAL_STAGE_DATA(stage) ((struct normal_stage_data *)stage->privatePtr)




static GLboolean run_normal_stage( GLcontext *ctx,
				   struct tnl_pipeline_stage *stage )
{
   struct normal_stage_data *store = NORMAL_STAGE_DATA(stage);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   ASSERT(store->NormalTransform);

   if (stage->changed_inputs) {
      /* We can only use the display list's saved normal lengths if we've
       * got a transformation matrix with uniform scaling.
       */
      const GLfloat *lengths;
      if (ctx->ModelviewMatrixStack.Top->flags & MAT_FLAG_GENERAL_SCALE)
         lengths = NULL;
      else
         lengths = VB->NormalLengthPtr;

      store->NormalTransform( ctx->ModelviewMatrixStack.Top,
			      ctx->_ModelViewInvScale,
			      VB->NormalPtr,  /* input normals */
			      lengths,
			      &store->normal ); /* resulting normals */
   }

   VB->NormalPtr = &store->normal;
   VB->AttribPtr[_TNL_ATTRIB_NORMAL] = VB->NormalPtr;

   VB->NormalLengthPtr = 0;	/* no longer valid */
   return GL_TRUE;
}


static GLboolean run_validate_normal_stage( GLcontext *ctx,
					    struct tnl_pipeline_stage *stage )
{
   struct normal_stage_data *store = NORMAL_STAGE_DATA(stage);


   if (ctx->_NeedEyeCoords) {
      GLuint transform = NORM_TRANSFORM_NO_ROT;

      if (ctx->ModelviewMatrixStack.Top->flags & (MAT_FLAG_GENERAL |
				  MAT_FLAG_ROTATION |
				  MAT_FLAG_GENERAL_3D |
				  MAT_FLAG_PERSPECTIVE))
	 transform = NORM_TRANSFORM;


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
      if (ctx->Transform.Normalize) {
	 store->NormalTransform = _mesa_normal_tab[NORM_NORMALIZE];
      }
      else if (!ctx->Transform.RescaleNormals &&
	       ctx->_ModelViewInvScale != 1.0) {
	 store->NormalTransform = _mesa_normal_tab[NORM_RESCALE];
      }
      else {
	 store->NormalTransform = 0;
      }
   }

   if (store->NormalTransform) {
      stage->run = run_normal_stage;
      return stage->run( ctx, stage );
   } else {
      stage->active = GL_FALSE;	/* !!! */
      return GL_TRUE;
   }
}


static void check_normal_transform( GLcontext *ctx,
				    struct tnl_pipeline_stage *stage )
{
   stage->active = !ctx->VertexProgram._Enabled &&
      (ctx->Light.Enabled || (ctx->Texture._GenFlags & TEXGEN_NEED_NORMALS));

   /* Don't clobber the initialize function:
    */
   if (stage->privatePtr)
      stage->run = run_validate_normal_stage;
}


static GLboolean alloc_normal_data( GLcontext *ctx,
				 struct tnl_pipeline_stage *stage )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct normal_stage_data *store;
   stage->privatePtr = MALLOC(sizeof(*store));
   store = NORMAL_STAGE_DATA(stage);
   if (!store)
      return GL_FALSE;

   _mesa_vector4f_alloc( &store->normal, 0, tnl->vb.Size, 32 );

   /* Now run the stage.
    */
   stage->run = run_validate_normal_stage;
   return stage->run( ctx, stage );
}



static void free_normal_data( struct tnl_pipeline_stage *stage )
{
   struct normal_stage_data *store = NORMAL_STAGE_DATA(stage);
   if (store) {
      _mesa_vector4f_free( &store->normal );
      FREE( store );
      stage->privatePtr = NULL;
   }
}

#define _TNL_NEW_NORMAL_TRANSFORM        (_NEW_MODELVIEW| \
					  _NEW_TRANSFORM| \
					  _NEW_PROGRAM| \
                                          _MESA_NEW_NEED_NORMALS| \
                                          _MESA_NEW_NEED_EYE_COORDS)



const struct tnl_pipeline_stage _tnl_normal_transform_stage =
{
   "normal transform",		/* name */
   _TNL_NEW_NORMAL_TRANSFORM,	/* re-check */
   _TNL_NEW_NORMAL_TRANSFORM,	/* re-run */
   GL_FALSE,			/* active? */
   _TNL_BIT_NORMAL,		/* inputs */
   _TNL_BIT_NORMAL,		/* outputs */
   0,				/* changed_inputs */
   NULL,			/* private data */
   free_normal_data,		/* destructor */
   check_normal_transform,	/* check */
   alloc_normal_data		/* run -- initially set to alloc */
};
