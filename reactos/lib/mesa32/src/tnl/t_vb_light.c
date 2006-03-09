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
 */



#include "glheader.h"
#include "colormac.h"
#include "light.h"
#include "macros.h"
#include "imports.h"
#include "simple_list.h"
#include "mtypes.h"

#include "math/m_translate.h"

#include "t_context.h"
#include "t_pipeline.h"

#define LIGHT_TWOSIDE       0x1
#define LIGHT_MATERIAL      0x2
#define MAX_LIGHT_FUNC      0x4

typedef void (*light_func)( GLcontext *ctx,
			    struct vertex_buffer *VB,
			    struct tnl_pipeline_stage *stage,
			    GLvector4f *input );

struct material_cursor {
   const GLfloat *ptr;
   GLuint stride;
   GLfloat *current;
};

struct light_stage_data {
   GLvector4f Input;
   GLvector4f LitColor[2];
   GLvector4f LitSecondary[2];
   GLvector4f LitIndex[2];
   light_func *light_func_tab;

   struct material_cursor mat[MAT_ATTRIB_MAX];
   GLuint mat_count;
   GLuint mat_bitmask;
};


#define LIGHT_STAGE_DATA(stage) ((struct light_stage_data *)(stage->privatePtr))



/* In the case of colormaterial, the effected material attributes
 * should already have been bound to point to the incoming color data,
 * prior to running the pipeline.
 */
static void update_materials( GLcontext *ctx,
			      struct light_stage_data *store )
{
   GLuint i;

   for (i = 0 ; i < store->mat_count ; i++) {
      COPY_4V(store->mat[i].current, store->mat[i].ptr);
      STRIDE_F(store->mat[i].ptr, store->mat[i].stride);
   }
      
   _mesa_update_material( ctx, store->mat_bitmask );
   _mesa_validate_all_lighting_tables( ctx );
}

static GLuint prepare_materials( GLcontext *ctx,
				 struct vertex_buffer *VB,
				 struct light_stage_data *store )
{
   GLuint i;
   
   store->mat_count = 0;
   store->mat_bitmask = 0;

   /* If ColorMaterial enabled, overwrite affected AttrPtr's with
    * the color pointer.  This could be done earlier.
    */
   if (ctx->Light.ColorMaterialEnabled) {
      GLuint bitmask = ctx->Light.ColorMaterialBitmask;
      for (i = 0 ; i < MAT_ATTRIB_MAX ; i++)
	 if (bitmask & (1<<i))
	    VB->AttribPtr[_TNL_ATTRIB_MAT_FRONT_AMBIENT + i] = VB->ColorPtr[0];
   }

   for (i = _TNL_ATTRIB_MAT_FRONT_AMBIENT ; i < _TNL_ATTRIB_INDEX ; i++) {
      if (VB->AttribPtr[i]->stride) {
	 GLuint j = store->mat_count++;
	 GLuint attr = i - _TNL_ATTRIB_MAT_FRONT_AMBIENT;
	 store->mat[j].ptr = VB->AttribPtr[i]->start;
	 store->mat[j].stride = VB->AttribPtr[i]->stride;
	 store->mat[j].current = ctx->Light.Material.Attrib[attr];
	 store->mat_bitmask |= (1<<attr);
      }
   }
   

   /* FIXME: Is this already done?
    */
   _mesa_update_material( ctx, ~0 );
   _mesa_validate_all_lighting_tables( ctx );

   return store->mat_count;
}

/* Tables for all the shading functions.
 */
static light_func _tnl_light_tab[MAX_LIGHT_FUNC];
static light_func _tnl_light_fast_tab[MAX_LIGHT_FUNC];
static light_func _tnl_light_fast_single_tab[MAX_LIGHT_FUNC];
static light_func _tnl_light_spec_tab[MAX_LIGHT_FUNC];
static light_func _tnl_light_ci_tab[MAX_LIGHT_FUNC];

#define TAG(x)           x
#define IDX              (0)
#include "t_vb_lighttmp.h"

#define TAG(x)           x##_twoside
#define IDX              (LIGHT_TWOSIDE)
#include "t_vb_lighttmp.h"

#define TAG(x)           x##_material
#define IDX              (LIGHT_MATERIAL)
#include "t_vb_lighttmp.h"

#define TAG(x)           x##_twoside_material
#define IDX              (LIGHT_TWOSIDE|LIGHT_MATERIAL)
#include "t_vb_lighttmp.h"


static void init_lighting( void )
{
   static int done;

   if (!done) {
      init_light_tab();
      init_light_tab_twoside();
      init_light_tab_material();
      init_light_tab_twoside_material();
      done = 1;
   }
}


static GLboolean run_lighting( GLcontext *ctx, 
			       struct tnl_pipeline_stage *stage )
{
   struct light_stage_data *store = LIGHT_STAGE_DATA(stage);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLvector4f *input = ctx->_NeedEyeCoords ? VB->EyePtr : VB->ObjPtr;
   GLuint idx;

   /* Make sure we can talk about position x,y and z:
    */
   if (stage->changed_inputs & _TNL_BIT_POS) {
      if (input->size <= 2 && input == VB->ObjPtr) {

	 _math_trans_4f( store->Input.data,
			 VB->ObjPtr->data,
			 VB->ObjPtr->stride,
			 GL_FLOAT,
			 VB->ObjPtr->size,
			 0,
			 VB->Count );

	 if (input->size <= 2) {
	    /* Clean z.
	     */
	    _mesa_vector4f_clean_elem(&store->Input, VB->Count, 2);
	 }
	 
	 if (input->size <= 1) {
	    /* Clean y.
	     */
	    _mesa_vector4f_clean_elem(&store->Input, VB->Count, 1);
	 }

	 input = &store->Input;
      }
   }
   
   idx = 0;

   if (prepare_materials( ctx, VB, store ))
      idx |= LIGHT_MATERIAL;

   if (ctx->Light.Model.TwoSide)
      idx |= LIGHT_TWOSIDE;

   /* The individual functions know about replaying side-effects
    * vs. full re-execution. 
    */
   store->light_func_tab[idx]( ctx, VB, stage, input );

   VB->AttribPtr[_TNL_ATTRIB_COLOR0] = VB->ColorPtr[0];
   VB->AttribPtr[_TNL_ATTRIB_COLOR1] = VB->SecondaryColorPtr[0];
   VB->AttribPtr[_TNL_ATTRIB_INDEX] = VB->IndexPtr[0];

   return GL_TRUE;
}


/* Called in place of do_lighting when the light table may have changed.
 */
static GLboolean run_validate_lighting( GLcontext *ctx,
					struct tnl_pipeline_stage *stage )
{
   light_func *tab;

   if (ctx->Visual.rgbMode) {
      if (ctx->Light._NeedVertices) {
	 if (ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)
	    tab = _tnl_light_spec_tab;
	 else
	    tab = _tnl_light_tab;
      }
      else {
	 if (ctx->Light.EnabledList.next == ctx->Light.EnabledList.prev)
	    tab = _tnl_light_fast_single_tab;
	 else
	    tab = _tnl_light_fast_tab;
      }
   }
   else
      tab = _tnl_light_ci_tab;


   LIGHT_STAGE_DATA(stage)->light_func_tab = tab;

   /* This and the above should only be done on _NEW_LIGHT:
    */
   TNL_CONTEXT(ctx)->Driver.NotifyMaterialChange( ctx );

   /* Now run the stage...
    */
   stage->run = run_lighting;
   return stage->run( ctx, stage );
}



/* Called the first time stage->run is called.  In effect, don't
 * allocate data until the first time the stage is run.
 */
static GLboolean run_init_lighting( GLcontext *ctx,
				    struct tnl_pipeline_stage *stage )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct light_stage_data *store;
   GLuint size = tnl->vb.Size;

   stage->privatePtr = MALLOC(sizeof(*store));
   store = LIGHT_STAGE_DATA(stage);
   if (!store)
      return GL_FALSE;

   /* Do onetime init.
    */
   init_lighting();

   _mesa_vector4f_alloc( &store->Input, 0, size, 32 );
   _mesa_vector4f_alloc( &store->LitColor[0], 0, size, 32 );
   _mesa_vector4f_alloc( &store->LitColor[1], 0, size, 32 );
   _mesa_vector4f_alloc( &store->LitSecondary[0], 0, size, 32 );
   _mesa_vector4f_alloc( &store->LitSecondary[1], 0, size, 32 );
   _mesa_vector4f_alloc( &store->LitIndex[0], 0, size, 32 );
   _mesa_vector4f_alloc( &store->LitIndex[1], 0, size, 32 );

   store->LitColor[0].size = 4;
   store->LitColor[1].size = 4;
   store->LitSecondary[0].size = 3;
   store->LitSecondary[1].size = 3;

   store->LitIndex[0].size = 1;
   store->LitIndex[0].stride = sizeof(GLfloat);
   store->LitIndex[1].size = 1;
   store->LitIndex[1].stride = sizeof(GLfloat);

   /* Now validate the stage derived data...
    */
   stage->run = run_validate_lighting;
   return stage->run( ctx, stage );
}



/*
 * Check if lighting is enabled.  If so, configure the pipeline stage's
 * type, inputs, and outputs.
 */
static void check_lighting( GLcontext *ctx, struct tnl_pipeline_stage *stage )
{
   stage->active = ctx->Light.Enabled && !ctx->VertexProgram._Enabled;
   if (stage->active) {
      if (stage->privatePtr)
	 stage->run = run_validate_lighting;
      stage->inputs = _TNL_BIT_NORMAL|_TNL_BITS_MAT_ANY;
      if (ctx->Light._NeedVertices)
	 stage->inputs |= _TNL_BIT_POS; 
      if (ctx->Light.ColorMaterialEnabled)
	 stage->inputs |= _TNL_BIT_COLOR0;

      stage->outputs = _TNL_BIT_COLOR0;
      if (ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)
	 stage->outputs |= _TNL_BIT_COLOR1;
   }
}


static void dtr( struct tnl_pipeline_stage *stage )
{
   struct light_stage_data *store = LIGHT_STAGE_DATA(stage);

   if (store) {
      _mesa_vector4f_free( &store->Input );
      _mesa_vector4f_free( &store->LitColor[0] );
      _mesa_vector4f_free( &store->LitColor[1] );
      _mesa_vector4f_free( &store->LitSecondary[0] );
      _mesa_vector4f_free( &store->LitSecondary[1] );
      _mesa_vector4f_free( &store->LitIndex[0] );
      _mesa_vector4f_free( &store->LitIndex[1] );
      FREE( store );
      stage->privatePtr = 0;
   }
}

const struct tnl_pipeline_stage _tnl_lighting_stage =
{
   "lighting",			/* name */
   _NEW_LIGHT|_NEW_PROGRAM,			/* recheck */
   _NEW_LIGHT|_NEW_MODELVIEW,	/* recalc -- modelview dependency
				 * otherwise not captured by inputs
				 * (which may be _TNL_BIT_POS) */
   GL_FALSE,			/* active? */
   0,				/* inputs */
   0,				/* outputs */
   0,				/* changed_inputs */
   NULL,			/* private_data */
   dtr,				/* destroy */
   check_lighting,		/* check */
   run_init_lighting		/* run -- initially set to ctr */
};
