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


#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "macros.h"
#include "imports.h"
#include "mtypes.h"

#include "math/m_xform.h"

#include "t_context.h"
#include "t_pipeline.h"


struct fog_stage_data {
   GLvector4f fogcoord;		/* has actual storage allocated */
};

#define FOG_STAGE_DATA(stage) ((struct fog_stage_data *)stage->privatePtr)

#define FOG_EXP_TABLE_SIZE 256
#define FOG_MAX (10.0)
#define EXP_FOG_MAX .0006595
#define FOG_INCR (FOG_MAX/FOG_EXP_TABLE_SIZE)
static GLfloat exp_table[FOG_EXP_TABLE_SIZE];
static GLfloat inited = 0;

#if 1
#define NEG_EXP( result, narg )						\
do {									\
   GLfloat f = (GLfloat) (narg * (1.0/FOG_INCR));			\
   GLint k = (GLint) f;							\
   if (k > FOG_EXP_TABLE_SIZE-2) 					\
      result = (GLfloat) EXP_FOG_MAX;					\
   else									\
      result = exp_table[k] + (f-k)*(exp_table[k+1]-exp_table[k]);	\
} while (0)
#else
#define NEG_EXP( result, narg )					\
do {								\
   result = exp(-narg);						\
} while (0)
#endif


/**
 * Initialize the exp_table[] lookup table for approximating exp().
 */
static void
init_static_data( void )
{
   GLfloat f = 0.0F;
   GLint i = 0;
   for ( ; i < FOG_EXP_TABLE_SIZE ; i++, f += FOG_INCR) {
      exp_table[i] = EXPF(-f);
   }
   inited = 1;
}


/**
 * Compute per-vertex fog blend factors from fog coordinates by
 * evaluating the GL_LINEAR, GL_EXP or GL_EXP2 fog function.
 * Fog coordinates are distances from the eye (typically between the
 * near and far clip plane distances).
 * Note that fogcoords may be negative, if eye z is source absolute
 * value must be taken earlier.
 * Fog blend factors are in the range [0,1].
 */
static void
compute_fog_blend_factors(GLcontext *ctx, GLvector4f *out, const GLvector4f *in)
{
   GLfloat end  = ctx->Fog.End;
   GLfloat *v = in->start;
   GLuint stride = in->stride;
   GLuint n = in->count;
   GLfloat (*data)[4] = out->data;
   GLfloat d;
   GLuint i;

   out->count = in->count;

   switch (ctx->Fog.Mode) {
   case GL_LINEAR:
      if (ctx->Fog.Start == ctx->Fog.End)
         d = 1.0F;
      else
         d = 1.0F / (ctx->Fog.End - ctx->Fog.Start);
      for ( i = 0 ; i < n ; i++, STRIDE_F(v, stride)) {
         const GLfloat z = *v;
         GLfloat f = (end - z) * d;
	 data[i][0] = CLAMP(f, 0.0F, 1.0F);
      }
      break;
   case GL_EXP:
      d = ctx->Fog.Density;
      for ( i = 0 ; i < n ; i++, STRIDE_F(v,stride)) {
         const GLfloat z = *v;
         NEG_EXP( data[i][0], d * z );
      }
      break;
   case GL_EXP2:
      d = ctx->Fog.Density*ctx->Fog.Density;
      for ( i = 0 ; i < n ; i++, STRIDE_F(v, stride)) {
         const GLfloat z = *v;
         NEG_EXP( data[i][0], d * z * z );
      }
      break;
   default:
      _mesa_problem(ctx, "Bad fog mode in make_fog_coord");
      return;
   }
}


static GLboolean
run_fog_stage(GLcontext *ctx, struct tnl_pipeline_stage *stage)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   struct fog_stage_data *store = FOG_STAGE_DATA(stage);
   GLvector4f *input;

   if (!ctx->Fog.Enabled || ctx->VertexProgram._Current)
      return GL_TRUE;


   if (ctx->Fog.FogCoordinateSource == GL_FRAGMENT_DEPTH_EXT) {
      GLuint i;
      GLfloat *coord;
      /* Fog is computed from vertex or fragment Z values */
      /* source = VB->ObjPtr or VB->EyePtr coords */
      /* dest = VB->AttribPtr[_TNL_ATTRIB_FOG] = fog stage private storage */
      VB->AttribPtr[_TNL_ATTRIB_FOG] = &store->fogcoord;

      if (!ctx->_NeedEyeCoords) {
         /* compute fog coords from object coords */
	 const GLfloat *m = ctx->ModelviewMatrixStack.Top->m;
	 GLfloat plane[4];

	 /* Use this to store calculated eye z values:
	  */
	 input = &store->fogcoord;

	 plane[0] = m[2];
	 plane[1] = m[6];
	 plane[2] = m[10];
	 plane[3] = m[14];
	 /* Full eye coords weren't required, just calculate the
	  * eye Z values.
	  */
	 _mesa_dotprod_tab[VB->ObjPtr->size]( (GLfloat *) input->data,
					      4 * sizeof(GLfloat),
					      VB->ObjPtr, plane );

	 input->count = VB->ObjPtr->count;

	 /* make sure coords are really positive
	    NOTE should avoid going through array twice */
	 coord = input->start;
	 for (i = 0; i < input->count; i++) {
	    *coord = FABSF(*coord);
	    STRIDE_F(coord, input->stride);
	 }
      }
      else {
         /* fog coordinates = eye Z coordinates - need to copy for ABS */
	 input = &store->fogcoord;

	 if (VB->EyePtr->size < 2)
	    _mesa_vector4f_clean_elem( VB->EyePtr, VB->Count, 2 );

	 input->stride = 4 * sizeof(GLfloat);
	 input->count = VB->EyePtr->count;
	 coord = VB->EyePtr->start;
	 for (i = 0 ; i < VB->EyePtr->count; i++) {
	    input->data[i][0] = FABSF(coord[2]);
	    STRIDE_F(coord, VB->EyePtr->stride);
	 }
      }
   }
   else {
      /* use glFogCoord() coordinates */
      input = VB->AttribPtr[_TNL_ATTRIB_FOG];  /* source data */

      /* input->count may be one if glFogCoord was only called once
       * before glBegin.  But we need to compute fog for all vertices.
       */
      input->count = VB->ObjPtr->count;

      VB->AttribPtr[_TNL_ATTRIB_FOG] = &store->fogcoord;  /* dest data */
   }

   if (tnl->_DoVertexFog) {
      /* compute blend factors from fog coordinates */
      compute_fog_blend_factors( ctx, VB->AttribPtr[_TNL_ATTRIB_FOG], input );
   }
   else {
      /* results = incoming fog coords (compute fog per-fragment later) */
      VB->AttribPtr[_TNL_ATTRIB_FOG] = input;
   }

   VB->FogCoordPtr = VB->AttribPtr[_TNL_ATTRIB_FOG];
   return GL_TRUE;
}



/* Called the first time stage->run() is invoked.
 */
static GLboolean
alloc_fog_data(GLcontext *ctx, struct tnl_pipeline_stage *stage)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct fog_stage_data *store;
   stage->privatePtr = MALLOC(sizeof(*store));
   store = FOG_STAGE_DATA(stage);
   if (!store)
      return GL_FALSE;

   _mesa_vector4f_alloc( &store->fogcoord, 0, tnl->vb.Size, 32 );

   if (!inited)
      init_static_data();

   return GL_TRUE;
}


static void
free_fog_data(struct tnl_pipeline_stage *stage)
{
   struct fog_stage_data *store = FOG_STAGE_DATA(stage);
   if (store) {
      _mesa_vector4f_free( &store->fogcoord );
      FREE( store );
      stage->privatePtr = NULL;
   }
}


const struct tnl_pipeline_stage _tnl_fog_coordinate_stage =
{
   "build fog coordinates",	/* name */
   NULL,			/* private_data */
   alloc_fog_data,		/* dtr */
   free_fog_data,		/* dtr */
   NULL,		/* check */
   run_fog_stage		/* run -- initially set to init. */
};
