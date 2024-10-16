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

#include <precomp.h>

/* Is there any real benefit seperating texmat from texgen?  It means
 * we need two lots of intermediate storage.  Any changes to
 * _NEW_TEXTURE will invalidate both sets -- it's only on changes to
 * *only* _NEW_TEXTURE_MATRIX that texgen survives but texmat doesn't.
 *
 * However, the seperation of this code from the complex texgen stuff
 * is very appealing.
 */
struct texmat_stage_data {
   GLvector4f texcoord;
};

#define TEXMAT_STAGE_DATA(stage) ((struct texmat_stage_data *)stage->privatePtr)



static GLboolean run_texmat_stage( struct gl_context *ctx,
				   struct tnl_pipeline_stage *stage )
{
   struct texmat_stage_data *store = TEXMAT_STAGE_DATA(stage);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   if (!ctx->Texture._TexMatEnabled) 
      return GL_TRUE;

   /* ENABLE_TEXMAT implies that the texture matrix is not the
    * identity, so we don't have to check that here.
    */
   if (ctx->Texture._TexMatEnabled) {
      (void) TransformRaw( &store->texcoord,
                  ctx->TextureMatrixStack.Top,
                  VB->AttribPtr[_TNL_ATTRIB_TEX]);

      VB->AttribPtr[VERT_ATTRIB_TEX] = &store->texcoord;
   }

   return GL_TRUE;
}


/* Called the first time stage->run() is invoked.
 */
static GLboolean alloc_texmat_data( struct gl_context *ctx,
				    struct tnl_pipeline_stage *stage )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   struct texmat_stage_data *store;

   stage->privatePtr = CALLOC(sizeof(*store));
   store = TEXMAT_STAGE_DATA(stage);
   if (!store)
      return GL_FALSE;

   _mesa_vector4f_alloc( &store->texcoord, 0, VB->Size, 32 );

   return GL_TRUE;
}


static void free_texmat_data( struct tnl_pipeline_stage *stage )
{
   struct texmat_stage_data *store = TEXMAT_STAGE_DATA(stage);

   if (store) {
	 if (store->texcoord.data)
	    _mesa_vector4f_free( &store->texcoord );
      FREE( store );
      stage->privatePtr = NULL;
   }
}



const struct tnl_pipeline_stage _tnl_texture_transform_stage =
{
   "texture transform",			/* name */
   NULL,				/* private data */
   alloc_texmat_data,
   free_texmat_data,			/* destructor */
   NULL,
   run_texmat_stage,
};
