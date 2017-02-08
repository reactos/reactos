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
 *    Brian Paul
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

/*
 * Regarding GL_NV_texgen_reflection:
 *
 * Portions of this software may use or implement intellectual
 * property owned and licensed by NVIDIA Corporation. NVIDIA disclaims
 * any and all warranties with respect to such intellectual property,
 * including any use thereof or modifications thereto.
 */

#include <precomp.h>

/***********************************************************************
 * Automatic texture coordinate generation (texgen) code.
 */


struct texgen_stage_data;

typedef void (*texgen_func)( struct gl_context *ctx,
			     struct texgen_stage_data *store);


struct texgen_stage_data {

   /* Per-texunit derived state.
    */
   GLuint TexgenSize;
   texgen_func TexgenFunc;

   /* Temporary values used in texgen.
    */
   GLfloat (*tmp_f)[3];
   GLfloat *tmp_m;

   /* Buffered outputs of the stage.
    */
   GLvector4f texcoord;
};


#define TEXGEN_STAGE_DATA(stage) ((struct texgen_stage_data *)stage->privatePtr)



static GLuint all_bits[5] = {
   0,
   VEC_SIZE_1,
   VEC_SIZE_2,
   VEC_SIZE_3,
   VEC_SIZE_4,
};

#define VEC_SIZE_FLAGS (VEC_SIZE_1|VEC_SIZE_2|VEC_SIZE_3|VEC_SIZE_4)

#define TEXGEN_NEED_M            (TEXGEN_SPHERE_MAP)
#define TEXGEN_NEED_F            (TEXGEN_SPHERE_MAP        | \
				  TEXGEN_REFLECTION_MAP_NV)



static void build_m3( GLfloat f[][3], GLfloat m[],
		      const GLvector4f *normal,
		      const GLvector4f *eye )
{
   GLuint stride = eye->stride;
   GLfloat *coord = (GLfloat *)eye->start;
   GLuint count = eye->count;
   const GLfloat *norm = normal->start;
   GLuint i;

   for (i=0;i<count;i++,STRIDE_F(coord,stride),STRIDE_F(norm,normal->stride)) {
      GLfloat u[3], two_nu, fx, fy, fz;
      COPY_3V( u, coord );
      NORMALIZE_3FV( u );
      two_nu = 2.0F * DOT3(norm,u);
      fx = f[i][0] = u[0] - norm[0] * two_nu;
      fy = f[i][1] = u[1] - norm[1] * two_nu;
      fz = f[i][2] = u[2] - norm[2] * two_nu;
      m[i] = fx * fx + fy * fy + (fz + 1.0F) * (fz + 1.0F);
      if (m[i] != 0.0F) {
	 m[i] = 0.5F * _mesa_inv_sqrtf(m[i]);
      }
   }
}



static void build_m2( GLfloat f[][3], GLfloat m[],
		      const GLvector4f *normal,
		      const GLvector4f *eye )
{
   GLuint stride = eye->stride;
   GLfloat *coord = eye->start;
   GLuint count = eye->count;

   GLfloat *norm = normal->start;
   GLuint i;

   for (i=0;i<count;i++,STRIDE_F(coord,stride),STRIDE_F(norm,normal->stride)) {
      GLfloat u[3], two_nu, fx, fy, fz;
      COPY_2V( u, coord );
      u[2] = 0;
      NORMALIZE_3FV( u );
      two_nu = 2.0F * DOT3(norm,u);
      fx = f[i][0] = u[0] - norm[0] * two_nu;
      fy = f[i][1] = u[1] - norm[1] * two_nu;
      fz = f[i][2] = u[2] - norm[2] * two_nu;
      m[i] = fx * fx + fy * fy + (fz + 1.0F) * (fz + 1.0F);
      if (m[i] != 0.0F) {
	 m[i] = 0.5F * _mesa_inv_sqrtf(m[i]);
      }
   }
}



typedef void (*build_m_func)( GLfloat f[][3],
			      GLfloat m[],
			      const GLvector4f *normal,
			      const GLvector4f *eye );


static build_m_func build_m_tab[5] = {
   NULL,
   NULL,
   build_m2,
   build_m3,
   build_m3
};


/* This is unusual in that we respect the stride of the output vector
 * (f).  This allows us to pass in either a texcoord vector4f, or a
 * temporary vector3f.
 */
static void build_f3( GLfloat *f,
		      GLuint fstride,
		      const GLvector4f *normal,
		      const GLvector4f *eye )
{
   GLuint stride = eye->stride;
   GLfloat *coord = eye->start;
   GLuint count = eye->count;

   GLfloat *norm = normal->start;
   GLuint i;

   for (i=0;i<count;i++) {
      GLfloat u[3], two_nu;
      COPY_3V( u, coord );
      NORMALIZE_3FV( u );
      two_nu = 2.0F * DOT3(norm,u);
      f[0] = u[0] - norm[0] * two_nu;
      f[1] = u[1] - norm[1] * two_nu;
      f[2] = u[2] - norm[2] * two_nu;
      STRIDE_F(coord,stride);
      STRIDE_F(f,fstride);
      STRIDE_F(norm, normal->stride);
   }
}


static void build_f2( GLfloat *f,
		      GLuint fstride,
		      const GLvector4f *normal,
		      const GLvector4f *eye )
{
   GLuint stride = eye->stride;
   GLfloat *coord = eye->start;
   GLuint count = eye->count;
   GLfloat *norm = normal->start;
   GLuint i;

   for (i=0;i<count;i++) {

      GLfloat u[3], two_nu;
      COPY_2V( u, coord );
      u[2] = 0;
      NORMALIZE_3FV( u );
      two_nu = 2.0F * DOT3(norm,u);
      f[0] = u[0] - norm[0] * two_nu;
      f[1] = u[1] - norm[1] * two_nu;
      f[2] = u[2] - norm[2] * two_nu;

      STRIDE_F(coord,stride);
      STRIDE_F(f,fstride);
      STRIDE_F(norm, normal->stride);
   }
}

typedef void (*build_f_func)( GLfloat *f,
			      GLuint fstride,
			      const GLvector4f *normal_vec,
			      const GLvector4f *eye );



/* Just treat 4-vectors as 3-vectors.
 */
static build_f_func build_f_tab[5] = {
   NULL,
   NULL,
   build_f2,
   build_f3,
   build_f3
};



/* Special case texgen functions.
 */
static void texgen_reflection_map_nv( struct gl_context *ctx,
				      struct texgen_stage_data *store)
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLvector4f *in = VB->AttribPtr[VERT_ATTRIB_TEX];
   GLvector4f *out = &store->texcoord;

   build_f_tab[VB->EyePtr->size]( out->start,
				  out->stride,
				  VB->AttribPtr[_TNL_ATTRIB_NORMAL],
				  VB->EyePtr );

   out->flags |= (in->flags & VEC_SIZE_FLAGS) | VEC_SIZE_3;
   out->count = VB->Count;
   out->size = MAX2(in->size, 3);
   if (in->size == 4) 
      _mesa_copy_tab[0x8]( out, in );
}



static void texgen_normal_map_nv( struct gl_context *ctx,
				  struct texgen_stage_data *store)
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLvector4f *in = VB->AttribPtr[VERT_ATTRIB_TEX];
   GLvector4f *out = &store->texcoord;
   GLvector4f *normal = VB->AttribPtr[_TNL_ATTRIB_NORMAL];
   GLfloat (*texcoord)[4] = (GLfloat (*)[4])out->start;
   GLuint count = VB->Count;
   GLuint i;
   const GLfloat *norm = normal->start;

   for (i=0;i<count;i++, STRIDE_F(norm, normal->stride)) {
      texcoord[i][0] = norm[0];
      texcoord[i][1] = norm[1];
      texcoord[i][2] = norm[2];
   }


   out->flags |= (in->flags & VEC_SIZE_FLAGS) | VEC_SIZE_3;
   out->count = count;
   out->size = MAX2(in->size, 3);
   if (in->size == 4) 
      _mesa_copy_tab[0x8]( out, in );
}


static void texgen_sphere_map( struct gl_context *ctx,
			       struct texgen_stage_data *store)
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLvector4f *in = VB->AttribPtr[VERT_ATTRIB_TEX];
   GLvector4f *out = &store->texcoord;
   GLfloat (*texcoord)[4] = (GLfloat (*)[4]) out->start;
   GLuint count = VB->Count;
   GLuint i;
   GLfloat (*f)[3] = store->tmp_f;
   GLfloat *m = store->tmp_m;

   (build_m_tab[VB->EyePtr->size])( store->tmp_f,
				    store->tmp_m,
				    VB->AttribPtr[_TNL_ATTRIB_NORMAL],
				    VB->EyePtr );

   out->size = MAX2(in->size,2);

   for (i=0;i<count;i++) {
      texcoord[i][0] = f[i][0] * m[i] + 0.5F;
      texcoord[i][1] = f[i][1] * m[i] + 0.5F;
   }

   out->count = count;
   out->flags |= (in->flags & VEC_SIZE_FLAGS) | VEC_SIZE_2;
   if (in->size > 2)
      _mesa_copy_tab[all_bits[in->size] & ~0x3]( out, in );
}



static void texgen( struct gl_context *ctx,
		    struct texgen_stage_data *store)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLvector4f *in = VB->AttribPtr[VERT_ATTRIB_TEX];
   GLvector4f *out = &store->texcoord;
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit;
   const GLvector4f *obj = VB->AttribPtr[_TNL_ATTRIB_POS];
   const GLvector4f *eye = VB->EyePtr;
   const GLvector4f *normal = VB->AttribPtr[_TNL_ATTRIB_NORMAL];
   const GLfloat *m = store->tmp_m;
   const GLuint count = VB->Count;
   GLfloat (*texcoord)[4] = (GLfloat (*)[4])out->data;
   GLfloat (*f)[3] = store->tmp_f;
   GLuint copy;

   if (texUnit->_GenFlags & TEXGEN_NEED_M) {
      build_m_tab[eye->size]( store->tmp_f, store->tmp_m, normal, eye );
   } else if (texUnit->_GenFlags & TEXGEN_NEED_F) {
      build_f_tab[eye->size]( (GLfloat *)store->tmp_f, 3, normal, eye );
   }


   out->size = MAX2(in->size, store->TexgenSize);
   out->flags |= (in->flags & VEC_SIZE_FLAGS) | texUnit->TexGenEnabled;
   out->count = count;

   copy = (all_bits[in->size] & ~texUnit->TexGenEnabled);
   if (copy)
      _mesa_copy_tab[copy]( out, in );

   if (texUnit->TexGenEnabled & S_BIT) {
      GLuint i;
      switch (texUnit->GenS.Mode) {
      case GL_OBJECT_LINEAR:
	 _mesa_dotprod_tab[obj->size]( (GLfloat *)out->data,
				       sizeof(out->data[0]), obj,
				       texUnit->GenS.ObjectPlane );
	 break;
      case GL_EYE_LINEAR:
	 _mesa_dotprod_tab[eye->size]( (GLfloat *)out->data,
				       sizeof(out->data[0]), eye,
				       texUnit->GenS.EyePlane );
	 break;
      case GL_SPHERE_MAP:
         for (i = 0; i < count; i++)
            texcoord[i][0] = f[i][0] * m[i] + 0.5F;
	 break;
      case GL_REFLECTION_MAP_NV:
	 for (i=0;i<count;i++)
	     texcoord[i][0] = f[i][0];
	 break;
      case GL_NORMAL_MAP_NV: {
	 const GLfloat *norm = normal->start;
	 for (i=0;i<count;i++, STRIDE_F(norm, normal->stride)) {
	     texcoord[i][0] = norm[0];
	 }
	 break;
      }
      default:
	 _mesa_problem(ctx, "Bad S texgen");
      }
   }

   if (texUnit->TexGenEnabled & T_BIT) {
      GLuint i;
      switch (texUnit->GenT.Mode) {
      case GL_OBJECT_LINEAR:
	 _mesa_dotprod_tab[obj->size]( &(out->data[0][1]),
				       sizeof(out->data[0]), obj,
				       texUnit->GenT.ObjectPlane );
	 break;
      case GL_EYE_LINEAR:
	 _mesa_dotprod_tab[eye->size]( &(out->data[0][1]),
				       sizeof(out->data[0]), eye,
				       texUnit->GenT.EyePlane );
	 break;
      case GL_SPHERE_MAP:
         for (i = 0; i < count; i++)
            texcoord[i][1] = f[i][1] * m[i] + 0.5F;
	 break;
      case GL_REFLECTION_MAP_NV:
	 for (i=0;i<count;i++)
	     texcoord[i][1] = f[i][1];
	 break;
      case GL_NORMAL_MAP_NV: {
	 const GLfloat *norm = normal->start;
	 for (i=0;i<count;i++, STRIDE_F(norm, normal->stride)) {
	     texcoord[i][1] = norm[1];
	 }
	 break;
      }
      default:
	 _mesa_problem(ctx, "Bad T texgen");
      }
   }

   if (texUnit->TexGenEnabled & R_BIT) {
      GLuint i;
      switch (texUnit->GenR.Mode) {
      case GL_OBJECT_LINEAR:
	 _mesa_dotprod_tab[obj->size]( &(out->data[0][2]),
				       sizeof(out->data[0]), obj,
				       texUnit->GenR.ObjectPlane );
	 break;
      case GL_EYE_LINEAR:
	 _mesa_dotprod_tab[eye->size]( &(out->data[0][2]),
				       sizeof(out->data[0]), eye,
				       texUnit->GenR.EyePlane );
	 break;
      case GL_REFLECTION_MAP_NV:
	 for (i=0;i<count;i++)
	     texcoord[i][2] = f[i][2];
	 break;
      case GL_NORMAL_MAP_NV: {
	 const GLfloat *norm = normal->start;
	 for (i=0;i<count;i++,STRIDE_F(norm, normal->stride)) {
	     texcoord[i][2] = norm[2];
	 }
	 break;
      }
      default:
	 _mesa_problem(ctx, "Bad R texgen");
      }
   }

   if (texUnit->TexGenEnabled & Q_BIT) {
      switch (texUnit->GenQ.Mode) {
      case GL_OBJECT_LINEAR:
	 _mesa_dotprod_tab[obj->size]( &(out->data[0][3]),
				       sizeof(out->data[0]), obj,
				       texUnit->GenQ.ObjectPlane );
	 break;
      case GL_EYE_LINEAR:
	 _mesa_dotprod_tab[eye->size]( &(out->data[0][3]),
				       sizeof(out->data[0]), eye,
				       texUnit->GenQ.EyePlane );
	 break;
      default:
	 _mesa_problem(ctx, "Bad Q texgen");
      }
   }
}




static GLboolean run_texgen_stage( struct gl_context *ctx,
				   struct tnl_pipeline_stage *stage )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   struct texgen_stage_data *store = TEXGEN_STAGE_DATA(stage);

   if (!ctx->Texture._TexGenEnabled) 
      return GL_TRUE;

   {
      struct gl_texture_unit *texUnit = &ctx->Texture.Unit;

      if (texUnit->TexGenEnabled) {

         store->TexgenFunc( ctx, store);

         VB->AttribPtr[VERT_ATTRIB_TEX] = &store->texcoord;
      }
   }

   return GL_TRUE;
}


static void validate_texgen_stage( struct gl_context *ctx,
				   struct tnl_pipeline_stage *stage )
{
   struct texgen_stage_data *store = TEXGEN_STAGE_DATA(stage);

   if (!ctx->Texture._TexGenEnabled) 
      return;

   {
      struct gl_texture_unit *texUnit = &ctx->Texture.Unit;

      if (texUnit->TexGenEnabled) {
         GLuint sz;

      if (texUnit->TexGenEnabled & Q_BIT)
	     sz = 4;
	  else if (texUnit->TexGenEnabled & R_BIT)
	     sz = 3;
      else if (texUnit->TexGenEnabled & T_BIT)
         sz = 2;
      else
         sz = 1;

      store->TexgenSize = sz;
      store->TexgenFunc = texgen; /* general solution */

      /* look for special texgen cases */
      if (texUnit->TexGenEnabled == (S_BIT|T_BIT|R_BIT)) {
         if (texUnit->_GenFlags == TEXGEN_REFLECTION_MAP_NV) {
            store->TexgenFunc = texgen_reflection_map_nv;
         }
         else if (texUnit->_GenFlags == TEXGEN_NORMAL_MAP_NV) {
            store->TexgenFunc = texgen_normal_map_nv;
         }
      }
	 else if (texUnit->TexGenEnabled == (S_BIT|T_BIT) &&
		  texUnit->_GenFlags == TEXGEN_SPHERE_MAP) {
	    store->TexgenFunc = texgen_sphere_map;
	 }
      }
   }
}





/* Called the first time stage->run() is invoked.
 */
static GLboolean alloc_texgen_data( struct gl_context *ctx,
				    struct tnl_pipeline_stage *stage )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   struct texgen_stage_data *store;

   stage->privatePtr = CALLOC(sizeof(*store));
   store = TEXGEN_STAGE_DATA(stage);
   if (!store)
      return GL_FALSE;

   _mesa_vector4f_alloc( &store->texcoord, 0, VB->Size, 32 );

   store->tmp_f = (GLfloat (*)[3]) MALLOC(VB->Size * sizeof(GLfloat) * 3);
   store->tmp_m = (GLfloat *) MALLOC(VB->Size * sizeof(GLfloat));

   return GL_TRUE;
}


static void free_texgen_data( struct tnl_pipeline_stage *stage )

{
   struct texgen_stage_data *store = TEXGEN_STAGE_DATA(stage);

   if (store) {
	 if (store->texcoord.data)
	    _mesa_vector4f_free( &store->texcoord );


      if (store->tmp_f) FREE( store->tmp_f );
      if (store->tmp_m) FREE( store->tmp_m );
      FREE( store );
      stage->privatePtr = NULL;
   }
}



const struct tnl_pipeline_stage _tnl_texgen_stage =
{
   "texgen",			/* name */
   NULL,			/* private data */
   alloc_texgen_data,		/* destructor */
   free_texgen_data,		/* destructor */
   validate_texgen_stage,		/* check */
   run_texgen_stage		/* run -- initially set to alloc data */
};
