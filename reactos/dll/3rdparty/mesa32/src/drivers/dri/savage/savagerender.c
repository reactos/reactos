/*
 * Copyright 2005  Felix Kuehling
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT. IN NO EVENT SHALL FELIX KUEHLING BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Render unclipped vertex buffers by emitting vertices directly to
 * dma buffers.  Use strip/fan hardware primitives where possible.
 * Simulate missing primitives with indexed vertices.
 */
#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "imports.h"
#include "mtypes.h"

#include "tnl/t_context.h"

#include "savagecontext.h"
#include "savagetris.h"
#include "savagestate.h"
#include "savageioctl.h"

/*
 * Standard render tab for Savage4 and smooth shading on Savage3D
 */
#define HAVE_POINTS      0
#define HAVE_LINES       0
#define HAVE_LINE_STRIPS 0
#define HAVE_TRIANGLES   1
#define HAVE_TRI_STRIPS  1
#define HAVE_TRI_STRIP_1 0
#define HAVE_TRI_FANS    1
#define HAVE_POLYGONS    0
#define HAVE_QUADS       0
#define HAVE_QUAD_STRIPS 0

#define HAVE_ELTS        1

#define LOCAL_VARS savageContextPtr imesa = SAVAGE_CONTEXT(ctx) 
#define INIT( prim ) do {						\
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);			\
   savageFlushVertices(imesa);						\
   switch (prim) {							\
   case GL_TRIANGLES:	   imesa->HwPrim = SAVAGE_PRIM_TRILIST; break;	\
   case GL_TRIANGLE_STRIP: imesa->HwPrim = SAVAGE_PRIM_TRISTRIP; break;	\
   case GL_TRIANGLE_FAN:   imesa->HwPrim = SAVAGE_PRIM_TRIFAN; break;	\
   }									\
} while (0)
#define FLUSH()		savageFlushElts(imesa), savageFlushVertices(imesa)

#define GET_CURRENT_VB_MAX_VERTS() \
   ((imesa->bufferSize/4 - imesa->vtxBuf->used) / imesa->HwVertexSize)
#define GET_SUBSEQUENT_VB_MAX_VERTS() \
   (imesa->bufferSize/4 / imesa->HwVertexSize)

#define ALLOC_VERTS( nr ) \
	savageAllocVtxBuf( imesa, (nr) * imesa->HwVertexSize )
#define EMIT_VERTS( ctx, j, nr, buf ) \
	_tnl_emit_vertices_to_buffer(ctx, j, (j)+(nr), buf )

#define ELTS_VARS( buf ) GLushort *dest = buf, firstElt = imesa->firstElt
#define ELT_INIT( prim ) INIT(prim)

/* (size - used - 1 qword for drawing command) * 4 elts per qword */
#define GET_CURRENT_VB_MAX_ELTS() \
   ((imesa->cmdBuf.size - (imesa->cmdBuf.write - imesa->cmdBuf.base) - 1)*4)
/* (size - space for initial state - 1 qword for drawing command) * 4 elts
 * imesa is not defined in validate_render :( */
#define GET_SUBSEQUENT_VB_MAX_ELTS()					\
   ((SAVAGE_CONTEXT(ctx)->cmdBuf.size - 				\
     (SAVAGE_CONTEXT(ctx)->cmdBuf.start - 				\
      SAVAGE_CONTEXT(ctx)->cmdBuf.base) - 1)*4)

#define ALLOC_ELTS(nr) savageAllocElts(imesa, nr)
#define EMIT_ELT(offset, x) do {					\
   (dest)[offset] = (GLushort) ((x)+firstElt);				\
} while (0)
#define EMIT_TWO_ELTS(offset, x, y) do {				\
   *(GLuint *)(dest + offset) = (((y)+firstElt) << 16) |		\
				((x)+firstElt);				\
} while (0)

#define INCR_ELTS( nr ) dest += nr
#define ELTPTR dest
#define RELEASE_ELT_VERTS() \
   savageReleaseIndexedVerts(imesa)

#define EMIT_INDEXED_VERTS( ctx, start, count ) do {			\
   GLuint *buf = savageAllocIndexedVerts(imesa, count-start);		\
   EMIT_VERTS(ctx, start, count-start, buf);				\
} while (0)

#define TAG(x) savage_##x
#include "tnl_dd/t_dd_dmatmp.h"

/*
 * On Savage3D triangle fans and strips are broken with flat
 * shading. With triangles it wants the color for flat shading in the
 * first vertex! So we make another template instance which uses
 * triangles only (with reordered vertices: SAVAGE_PRIM_TRILIST_201).
 * The reordering is done by the DRM.
 */
#undef  HAVE_TRI_STRIPS
#undef  HAVE_TRI_FANS
#define HAVE_TRI_STRIPS	0
#define HAVE_TRI_FANS	0

#undef  INIT
#define INIT( prim ) do {						\
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);			\
   savageFlushVertices(imesa);						\
   imesa->HwPrim = SAVAGE_PRIM_TRILIST_201;				\
} while(0)

#undef  TAG
#define TAG(x) savage_flat_##x##_s3d
#include "tnl_dd/t_dd_dmatmp.h"


/**********************************************************************/
/*                          Render pipeline stage                     */
/**********************************************************************/

static GLboolean savage_run_render( GLcontext *ctx,
				    struct tnl_pipeline_stage *stage )
{
   savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb; 
   tnl_render_func *tab, *tab_elts;
   GLboolean valid;
   GLuint i;

   if (savageHaveIndexedVerts(imesa))
      savageReleaseIndexedVerts(imesa);

   if (imesa->savageScreen->chipset < S3_SAVAGE4 &&
       (ctx->_TriangleCaps & DD_FLATSHADE)) {
      tab = savage_flat_render_tab_verts_s3d;
      tab_elts = savage_flat_render_tab_elts_s3d;
      valid = savage_flat_validate_render_s3d( ctx, VB );
   } else {
      tab = savage_render_tab_verts;
      tab_elts = savage_render_tab_elts;
      valid = savage_validate_render( ctx, VB );
   }

   /* Don't handle clipping or vertex manipulations.
    */
   if (imesa->RenderIndex != 0 || !valid) {
      return GL_TRUE;
   }
   
   tnl->Driver.Render.Start( ctx );
   /* Check RenderIndex again. The ptexHack is detected late in RenderStart.
    * Also check for ptex fallbacks detected late.
    */
   if (imesa->RenderIndex != 0 || imesa->Fallback != 0) {
      return GL_TRUE;
   }

   /* setup for hardware culling */
   imesa->raster_primitive = GL_TRIANGLES;
   imesa->new_state |= SAVAGE_NEW_CULL;

   /* update and emit state */
   savageDDUpdateHwState(ctx);
   savageEmitChangedState(imesa);

   if (VB->Elts) {
      tab = tab_elts;
      if (!savageHaveIndexedVerts(imesa)) {
	 if (VB->Count > GET_SUBSEQUENT_VB_MAX_VERTS())
	    return GL_TRUE;
	 EMIT_INDEXED_VERTS(ctx, 0, VB->Count);
      }
   }

   for (i = 0 ; i < VB->PrimitiveCount ; i++)
   {
      GLuint prim = VB->Primitive[i].mode;
      GLuint start = VB->Primitive[i].start;
      GLuint length = VB->Primitive[i].count;

      if (length)
	 tab[prim & PRIM_MODE_MASK]( ctx, start, start+length, prim);
   }

   tnl->Driver.Render.Finish( ctx );

   return GL_FALSE;		/* finished the pipe */
}

struct tnl_pipeline_stage _savage_render_stage = 
{ 
   "savage render",
   NULL,
   NULL,
   NULL,
   NULL,
   savage_run_render		/* run */
};


/**********************************************************************/
/*         Pipeline stage for texture coordinate normalization        */
/**********************************************************************/
struct texnorm_stage_data {
   GLboolean active;
   GLvector4f texcoord[MAX_TEXTURE_UNITS];
};

#define TEXNORM_STAGE_DATA(stage) ((struct texnorm_stage_data *)stage->privatePtr)


static GLboolean run_texnorm_stage( GLcontext *ctx,
				    struct tnl_pipeline_stage *stage )
{
   struct texnorm_stage_data *store = TEXNORM_STAGE_DATA(stage);
   savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;

   if (imesa->Fallback || !store->active)
      return GL_TRUE;

   for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++) {
      const GLbitfield reallyEnabled = ctx->Texture.Unit[i]._ReallyEnabled;
      if (reallyEnabled) {
         const struct gl_texture_object *texObj = ctx->Texture.Unit[i]._Current;
         const GLboolean normalizeS = (texObj->WrapS == GL_REPEAT);
         const GLboolean normalizeT = (reallyEnabled & TEXTURE_2D_BIT) &&
            (texObj->WrapT == GL_REPEAT);
         const GLfloat *in = (GLfloat *)VB->TexCoordPtr[i]->data;
         const GLint instride = VB->TexCoordPtr[i]->stride;
         GLfloat (*out)[4] = store->texcoord[i].data;
         GLint j;

         if (!ctx->Texture.Unit[i]._ReallyEnabled ||
             VB->TexCoordPtr[i]->size == 4)
            /* Never try to normalize homogenous tex coords! */
            continue;

         if (normalizeS && normalizeT) {
            /* take first texcoords as rough estimate of mean value */
            GLfloat correctionS = -floor(in[0]+0.5);
            GLfloat correctionT = -floor(in[1]+0.5);
            for (j = 0; j < VB->Count; ++j) {
               out[j][0] = in[0] + correctionS;
               out[j][1] = in[1] + correctionT;
               in = (GLfloat *)((GLubyte *)in + instride);
            }
         } else if (normalizeS) {
            /* take first texcoords as rough estimate of mean value */
            GLfloat correctionS = -floor(in[0]+0.5);
            if (reallyEnabled & TEXTURE_2D_BIT) {
               for (j = 0; j < VB->Count; ++j) {
                  out[j][0] = in[0] + correctionS;
                  out[j][1] = in[1];
                  in = (GLfloat *)((GLubyte *)in + instride);
               }
            } else {
               for (j = 0; j < VB->Count; ++j) {
                  out[j][0] = in[0] + correctionS;
                  in = (GLfloat *)((GLubyte *)in + instride);
               }
            }
         } else if (normalizeT) {
            /* take first texcoords as rough estimate of mean value */
            GLfloat correctionT = -floor(in[1]+0.5);
            for (j = 0; j < VB->Count; ++j) {
               out[j][0] = in[0];
               out[j][1] = in[1] + correctionT;
               in = (GLfloat *)((GLubyte *)in + instride);
            }
         }

         if (normalizeS || normalizeT)
            VB->AttribPtr[VERT_ATTRIB_TEX0+i] = VB->TexCoordPtr[i] = &store->texcoord[i];
      }
   }

   return GL_TRUE;
}

/* Called the first time stage->run() is invoked.
 */
static GLboolean alloc_texnorm_data( GLcontext *ctx,
				     struct tnl_pipeline_stage *stage )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   struct texnorm_stage_data *store;
   GLuint i;

   stage->privatePtr = CALLOC(sizeof(*store));
   store = TEXNORM_STAGE_DATA(stage);
   if (!store)
      return GL_FALSE;

   for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++)
      _mesa_vector4f_alloc( &store->texcoord[i], 0, VB->Size, 32 );
   
   return GL_TRUE;
}

static void validate_texnorm( GLcontext *ctx,
			      struct tnl_pipeline_stage *stage )
{
   struct texnorm_stage_data *store = TEXNORM_STAGE_DATA(stage);
   GLuint flags = 0;

   if (((ctx->Texture.Unit[0]._ReallyEnabled & (TEXTURE_1D_BIT|TEXTURE_2D_BIT)) &&
	(ctx->Texture.Unit[0]._Current->WrapS == GL_REPEAT)) ||
       ((ctx->Texture.Unit[0]._ReallyEnabled & TEXTURE_2D_BIT) &&
	(ctx->Texture.Unit[0]._Current->WrapT == GL_REPEAT)))
      flags |= VERT_BIT_TEX0;

   if (((ctx->Texture.Unit[1]._ReallyEnabled & (TEXTURE_1D_BIT|TEXTURE_2D_BIT)) &&
	(ctx->Texture.Unit[1]._Current->WrapS == GL_REPEAT)) ||
       ((ctx->Texture.Unit[1]._ReallyEnabled & TEXTURE_2D_BIT) &&
	(ctx->Texture.Unit[1]._Current->WrapT == GL_REPEAT)))
      flags |= VERT_BIT_TEX1;

   store->active = (flags != 0);
}

static void free_texnorm_data( struct tnl_pipeline_stage *stage )
{
   struct texnorm_stage_data *store = TEXNORM_STAGE_DATA(stage);
   GLuint i;

   if (store) {
      for (i = 0 ; i < MAX_TEXTURE_UNITS ; i++)
	 if (store->texcoord[i].data)
	    _mesa_vector4f_free( &store->texcoord[i] );
      FREE( store );
      stage->privatePtr = 0;
   }
}

struct tnl_pipeline_stage _savage_texnorm_stage =
{
   "savage texture coordinate normalization stage", /* name */
   NULL,				/* private data */
   alloc_texnorm_data,			/* run -- initially set to init */
   free_texnorm_data,			/* destructor */
   validate_texnorm,
   run_texnorm_stage
};
