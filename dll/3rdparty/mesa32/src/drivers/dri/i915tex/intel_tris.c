/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "enums.h"
#include "texobj.h"
#include "state.h"
#include "dd.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "tnl/t_vertex.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_tris.h"
#include "intel_batchbuffer.h"
#include "intel_buffers.h"
#include "intel_reg.h"
#include "intel_span.h"
#include "intel_tex.h"

static void intelRenderPrimitive(GLcontext * ctx, GLenum prim);
static void intelRasterPrimitive(GLcontext * ctx, GLenum rprim,
                                 GLuint hwprim);

/*
 */
static void
intel_flush_inline_primitive(struct intel_context *intel)
{
   GLuint used = intel->batch->ptr - intel->prim.start_ptr;

   assert(intel->prim.primitive != ~0);

/*    _mesa_printf("/\n"); */

   if (used < 8)
      goto do_discard;

   *(int *) intel->prim.start_ptr = (_3DPRIMITIVE |
                                     intel->prim.primitive | (used / 4 - 2));

   goto finished;

 do_discard:
   intel->batch->ptr -= used;

 finished:
   intel->prim.primitive = ~0;
   intel->prim.start_ptr = 0;
   intel->prim.flush = 0;
}


/* Emit a primitive referencing vertices in a vertex buffer.
 */
void
intelStartInlinePrimitive(struct intel_context *intel,
                          GLuint prim, GLuint batch_flags)
{
   BATCH_LOCALS;

   intel->vtbl.emit_state(intel);

   /* Need to make sure at the very least that we don't wrap
    * batchbuffers in BEGIN_BATCH below, otherwise the primitive will
    * be emitted to a batchbuffer missing the required full-state
    * preamble.
    */
   if (intel_batchbuffer_space(intel->batch) < 100) {
      intel_batchbuffer_flush(intel->batch);
      intel->vtbl.emit_state(intel);
   }

/*    _mesa_printf("%s *", __progname); */

   intel_wait_flips(intel, batch_flags);

   /* Emit a slot which will be filled with the inline primitive
    * command later.
    */
   BEGIN_BATCH(2, batch_flags);
   OUT_BATCH(0);

   intel->prim.start_ptr = intel->batch->ptr;
   intel->prim.primitive = prim;
   intel->prim.flush = intel_flush_inline_primitive;

   OUT_BATCH(0);
   ADVANCE_BATCH();

/*    _mesa_printf(">"); */
}


void
intelWrapInlinePrimitive(struct intel_context *intel)
{
   GLuint prim = intel->prim.primitive;
   GLuint batchflags = intel->batch->flags;

   intel_flush_inline_primitive(intel);
   intel_batchbuffer_flush(intel->batch);
   intelStartInlinePrimitive(intel, prim, batchflags);  /* ??? */
}

GLuint *
intelExtendInlinePrimitive(struct intel_context *intel, GLuint dwords)
{
   GLuint sz = dwords * sizeof(GLuint);
   GLuint *ptr;

   assert(intel->prim.flush == intel_flush_inline_primitive);

   if (intel_batchbuffer_space(intel->batch) < sz)
      intelWrapInlinePrimitive(intel);

/*    _mesa_printf("."); */

   intel->vtbl.assert_not_dirty(intel);

   ptr = (GLuint *) intel->batch->ptr;
   intel->batch->ptr += sz;

   return ptr;
}



/***********************************************************************
 *                    Emit primitives as inline vertices               *
 ***********************************************************************/

#ifdef __i386__
#define COPY_DWORDS( j, vb, vertsize, v )			\
do {								\
   int __tmp;							\
   __asm__ __volatile__( "rep ; movsl"				\
			 : "=%c" (j), "=D" (vb), "=S" (__tmp)	\
			 : "0" (vertsize),			\
			 "D" ((long)vb),			\
			 "S" ((long)v) );			\
} while (0)
#else
#define COPY_DWORDS( j, vb, vertsize, v )	\
do {						\
   for ( j = 0 ; j < vertsize ; j++ ) {		\
      vb[j] = ((GLuint *)v)[j];			\
   }						\
   vb += vertsize;				\
} while (0)
#endif

static void
intel_draw_quad(struct intel_context *intel,
                intelVertexPtr v0,
                intelVertexPtr v1, intelVertexPtr v2, intelVertexPtr v3)
{
   GLuint vertsize = intel->vertex_size;
   GLuint *vb = intelExtendInlinePrimitive(intel, 6 * vertsize);
   int j;

   COPY_DWORDS(j, vb, vertsize, v0);
   COPY_DWORDS(j, vb, vertsize, v1);

   /* If smooth shading, draw like a trifan which gives better
    * rasterization.  Otherwise draw as two triangles with provoking
    * vertex in third position as required for flat shading.
    */
   if (intel->ctx.Light.ShadeModel == GL_FLAT) {
      COPY_DWORDS(j, vb, vertsize, v3);
      COPY_DWORDS(j, vb, vertsize, v1);
   }
   else {
      COPY_DWORDS(j, vb, vertsize, v2);
      COPY_DWORDS(j, vb, vertsize, v0);
   }

   COPY_DWORDS(j, vb, vertsize, v2);
   COPY_DWORDS(j, vb, vertsize, v3);
}

static void
intel_draw_triangle(struct intel_context *intel,
                    intelVertexPtr v0, intelVertexPtr v1, intelVertexPtr v2)
{
   GLuint vertsize = intel->vertex_size;
   GLuint *vb = intelExtendInlinePrimitive(intel, 3 * vertsize);
   int j;

   COPY_DWORDS(j, vb, vertsize, v0);
   COPY_DWORDS(j, vb, vertsize, v1);
   COPY_DWORDS(j, vb, vertsize, v2);
}


static void
intel_draw_line(struct intel_context *intel,
                intelVertexPtr v0, intelVertexPtr v1)
{
   GLuint vertsize = intel->vertex_size;
   GLuint *vb = intelExtendInlinePrimitive(intel, 2 * vertsize);
   int j;

   COPY_DWORDS(j, vb, vertsize, v0);
   COPY_DWORDS(j, vb, vertsize, v1);
}


static void
intel_draw_point(struct intel_context *intel, intelVertexPtr v0)
{
   GLuint vertsize = intel->vertex_size;
   GLuint *vb = intelExtendInlinePrimitive(intel, vertsize);
   int j;

   /* Adjust for sub pixel position -- still required for conform. */
   *(float *) &vb[0] = v0->v.x - 0.125;
   *(float *) &vb[1] = v0->v.y - 0.125;
   for (j = 2; j < vertsize; j++)
      vb[j] = v0->ui[j];
}



/***********************************************************************
 *                Fixup for ARB_point_parameters                       *
 ***********************************************************************/

/* Currently not working - VERT_ATTRIB_POINTSIZE isn't correctly
 * represented in the fragment program InputsRead field.
 */
static void
intel_atten_point(struct intel_context *intel, intelVertexPtr v0)
{
   GLcontext *ctx = &intel->ctx;
   GLfloat psz[4], col[4], restore_psz, restore_alpha;

   _tnl_get_attr(ctx, v0, _TNL_ATTRIB_POINTSIZE, psz);
   _tnl_get_attr(ctx, v0, _TNL_ATTRIB_COLOR0, col);

   restore_psz = psz[0];
   restore_alpha = col[3];

   if (psz[0] >= ctx->Point.Threshold) {
      psz[0] = MIN2(psz[0], ctx->Point.MaxSize);
   }
   else {
      GLfloat dsize = psz[0] / ctx->Point.Threshold;
      psz[0] = MAX2(ctx->Point.Threshold, ctx->Point.MinSize);
      col[3] *= dsize * dsize;
   }

   if (psz[0] < 1.0)
      psz[0] = 1.0;

   if (restore_psz != psz[0] || restore_alpha != col[3]) {
      _tnl_set_attr(ctx, v0, _TNL_ATTRIB_POINTSIZE, psz);
      _tnl_set_attr(ctx, v0, _TNL_ATTRIB_COLOR0, col);

      intel_draw_point(intel, v0);

      psz[0] = restore_psz;
      col[3] = restore_alpha;

      _tnl_set_attr(ctx, v0, _TNL_ATTRIB_POINTSIZE, psz);
      _tnl_set_attr(ctx, v0, _TNL_ATTRIB_COLOR0, col);
   }
   else
      intel_draw_point(intel, v0);
}





/***********************************************************************
 *                Fixup for I915 WPOS texture coordinate                *
 ***********************************************************************/



static void
intel_wpos_triangle(struct intel_context *intel,
                    intelVertexPtr v0, intelVertexPtr v1, intelVertexPtr v2)
{
   GLuint offset = intel->wpos_offset;
   GLuint size = intel->wpos_size;

   __memcpy(((char *) v0) + offset, v0, size);
   __memcpy(((char *) v1) + offset, v1, size);
   __memcpy(((char *) v2) + offset, v2, size);

   intel_draw_triangle(intel, v0, v1, v2);
}


static void
intel_wpos_line(struct intel_context *intel,
                intelVertexPtr v0, intelVertexPtr v1)
{
   GLuint offset = intel->wpos_offset;
   GLuint size = intel->wpos_size;

   __memcpy(((char *) v0) + offset, v0, size);
   __memcpy(((char *) v1) + offset, v1, size);

   intel_draw_line(intel, v0, v1);
}


static void
intel_wpos_point(struct intel_context *intel, intelVertexPtr v0)
{
   GLuint offset = intel->wpos_offset;
   GLuint size = intel->wpos_size;

   __memcpy(((char *) v0) + offset, v0, size);

   intel_draw_point(intel, v0);
}






/***********************************************************************
 *          Macros for t_dd_tritmp.h to draw basic primitives          *
 ***********************************************************************/

#define TRI( a, b, c )				\
do { 						\
   if (DO_FALLBACK)				\
      intel->draw_tri( intel, a, b, c );	\
   else						\
      intel_draw_triangle( intel, a, b, c );	\
} while (0)

#define QUAD( a, b, c, d )			\
do { 						\
   if (DO_FALLBACK) {				\
      intel->draw_tri( intel, a, b, d );	\
      intel->draw_tri( intel, b, c, d );	\
   } else					\
      intel_draw_quad( intel, a, b, c, d );	\
} while (0)

#define LINE( v0, v1 )				\
do { 						\
   if (DO_FALLBACK)				\
      intel->draw_line( intel, v0, v1 );	\
   else						\
      intel_draw_line( intel, v0, v1 );		\
} while (0)

#define POINT( v0 )				\
do { 						\
   if (DO_FALLBACK)				\
      intel->draw_point( intel, v0 );		\
   else						\
      intel_draw_point( intel, v0 );		\
} while (0)


/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/

#define INTEL_OFFSET_BIT 	0x01
#define INTEL_TWOSIDE_BIT	0x02
#define INTEL_UNFILLED_BIT	0x04
#define INTEL_FALLBACK_BIT	0x08
#define INTEL_MAX_TRIFUNC	0x10


static struct
{
   tnl_points_func points;
   tnl_line_func line;
   tnl_triangle_func triangle;
   tnl_quad_func quad;
} rast_tab[INTEL_MAX_TRIFUNC];


#define DO_FALLBACK (IND & INTEL_FALLBACK_BIT)
#define DO_OFFSET   (IND & INTEL_OFFSET_BIT)
#define DO_UNFILLED (IND & INTEL_UNFILLED_BIT)
#define DO_TWOSIDE  (IND & INTEL_TWOSIDE_BIT)
#define DO_FLAT      0
#define DO_TRI       1
#define DO_QUAD      1
#define DO_LINE      1
#define DO_POINTS    1
#define DO_FULL_QUAD 1

#define HAVE_RGBA         1
#define HAVE_SPEC         1
#define HAVE_BACK_COLORS  0
#define HAVE_HW_FLATSHADE 1
#define VERTEX            intelVertex
#define TAB               rast_tab

/* Only used to pull back colors into vertices (ie, we know color is
 * floating point).
 */
#define INTEL_COLOR( dst, src )				\
do {							\
   UNCLAMPED_FLOAT_TO_UBYTE((dst)[0], (src)[2]);	\
   UNCLAMPED_FLOAT_TO_UBYTE((dst)[1], (src)[1]);	\
   UNCLAMPED_FLOAT_TO_UBYTE((dst)[2], (src)[0]);	\
   UNCLAMPED_FLOAT_TO_UBYTE((dst)[3], (src)[3]);	\
} while (0)

#define INTEL_SPEC( dst, src )				\
do {							\
   UNCLAMPED_FLOAT_TO_UBYTE((dst)[0], (src)[2]);	\
   UNCLAMPED_FLOAT_TO_UBYTE((dst)[1], (src)[1]);	\
   UNCLAMPED_FLOAT_TO_UBYTE((dst)[2], (src)[0]);	\
} while (0)


#define DEPTH_SCALE intel->polygon_offset_scale
#define UNFILLED_TRI unfilled_tri
#define UNFILLED_QUAD unfilled_quad
#define VERT_X(_v) _v->v.x
#define VERT_Y(_v) _v->v.y
#define VERT_Z(_v) _v->v.z
#define AREA_IS_CCW( a ) (a > 0)
#define GET_VERTEX(e) (intel->verts + (e * intel->vertex_size * sizeof(GLuint)))

#define VERT_SET_RGBA( v, c )    if (coloroffset) INTEL_COLOR( v->ub4[coloroffset], c )
#define VERT_COPY_RGBA( v0, v1 ) if (coloroffset) v0->ui[coloroffset] = v1->ui[coloroffset]
#define VERT_SAVE_RGBA( idx )    if (coloroffset) color[idx] = v[idx]->ui[coloroffset]
#define VERT_RESTORE_RGBA( idx ) if (coloroffset) v[idx]->ui[coloroffset] = color[idx]

#define VERT_SET_SPEC( v, c )    if (specoffset) INTEL_SPEC( v->ub4[specoffset], c )
#define VERT_COPY_SPEC( v0, v1 ) if (specoffset) COPY_3V(v0->ub4[specoffset], v1->ub4[specoffset])
#define VERT_SAVE_SPEC( idx )    if (specoffset) spec[idx] = v[idx]->ui[specoffset]
#define VERT_RESTORE_SPEC( idx ) if (specoffset) v[idx]->ui[specoffset] = spec[idx]

#define LOCAL_VARS(n)							\
   struct intel_context *intel = intel_context(ctx);				\
   GLuint color[n], spec[n];						\
   GLuint coloroffset = intel->coloroffset;		\
   GLboolean specoffset = intel->specoffset;			\
   (void) color; (void) spec; (void) coloroffset; (void) specoffset;


/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/

static const GLuint hw_prim[GL_POLYGON + 1] = {
   PRIM3D_POINTLIST,
   PRIM3D_LINELIST,
   PRIM3D_LINELIST,
   PRIM3D_LINELIST,
   PRIM3D_TRILIST,
   PRIM3D_TRILIST,
   PRIM3D_TRILIST,
   PRIM3D_TRILIST,
   PRIM3D_TRILIST,
   PRIM3D_TRILIST
};

#define RASTERIZE(x) intelRasterPrimitive( ctx, x, hw_prim[x] )
#define RENDER_PRIMITIVE intel->render_primitive
#define TAG(x) x
#define IND INTEL_FALLBACK_BIT
#include "tnl_dd/t_dd_unfilled.h"
#undef IND

/***********************************************************************
 *                      Generate GL render functions                   *
 ***********************************************************************/

#define IND (0)
#define TAG(x) x
#include "tnl_dd/t_dd_tritmp.h"

#define IND (INTEL_OFFSET_BIT)
#define TAG(x) x##_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (INTEL_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "tnl_dd/t_dd_tritmp.h"

#define IND (INTEL_TWOSIDE_BIT|INTEL_OFFSET_BIT)
#define TAG(x) x##_twoside_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (INTEL_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (INTEL_OFFSET_BIT|INTEL_UNFILLED_BIT)
#define TAG(x) x##_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (INTEL_TWOSIDE_BIT|INTEL_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (INTEL_TWOSIDE_BIT|INTEL_OFFSET_BIT|INTEL_UNFILLED_BIT)
#define TAG(x) x##_twoside_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (INTEL_FALLBACK_BIT)
#define TAG(x) x##_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (INTEL_OFFSET_BIT|INTEL_FALLBACK_BIT)
#define TAG(x) x##_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (INTEL_TWOSIDE_BIT|INTEL_FALLBACK_BIT)
#define TAG(x) x##_twoside_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (INTEL_TWOSIDE_BIT|INTEL_OFFSET_BIT|INTEL_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (INTEL_UNFILLED_BIT|INTEL_FALLBACK_BIT)
#define TAG(x) x##_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (INTEL_OFFSET_BIT|INTEL_UNFILLED_BIT|INTEL_FALLBACK_BIT)
#define TAG(x) x##_offset_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (INTEL_TWOSIDE_BIT|INTEL_UNFILLED_BIT|INTEL_FALLBACK_BIT)
#define TAG(x) x##_twoside_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (INTEL_TWOSIDE_BIT|INTEL_OFFSET_BIT|INTEL_UNFILLED_BIT| \
	     INTEL_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"


static void
init_rast_tab(void)
{
   init();
   init_offset();
   init_twoside();
   init_twoside_offset();
   init_unfilled();
   init_offset_unfilled();
   init_twoside_unfilled();
   init_twoside_offset_unfilled();
   init_fallback();
   init_offset_fallback();
   init_twoside_fallback();
   init_twoside_offset_fallback();
   init_unfilled_fallback();
   init_offset_unfilled_fallback();
   init_twoside_unfilled_fallback();
   init_twoside_offset_unfilled_fallback();
}


/***********************************************************************
 *                    Rasterization fallback helpers                   *
 ***********************************************************************/


/* This code is hit only when a mix of accelerated and unaccelerated
 * primitives are being drawn, and only for the unaccelerated
 * primitives.
 */
static void
intel_fallback_tri(struct intel_context *intel,
                   intelVertex * v0, intelVertex * v1, intelVertex * v2)
{
   GLcontext *ctx = &intel->ctx;
   SWvertex v[3];

   if (0)
      fprintf(stderr, "\n%s\n", __FUNCTION__);

   INTEL_FIREVERTICES(intel);

   _swsetup_Translate(ctx, v0, &v[0]);
   _swsetup_Translate(ctx, v1, &v[1]);
   _swsetup_Translate(ctx, v2, &v[2]);
   intelSpanRenderStart(ctx);
   _swrast_Triangle(ctx, &v[0], &v[1], &v[2]);
   intelSpanRenderFinish(ctx);
}


static void
intel_fallback_line(struct intel_context *intel,
                    intelVertex * v0, intelVertex * v1)
{
   GLcontext *ctx = &intel->ctx;
   SWvertex v[2];

   if (0)
      fprintf(stderr, "\n%s\n", __FUNCTION__);

   INTEL_FIREVERTICES(intel);

   _swsetup_Translate(ctx, v0, &v[0]);
   _swsetup_Translate(ctx, v1, &v[1]);
   intelSpanRenderStart(ctx);
   _swrast_Line(ctx, &v[0], &v[1]);
   intelSpanRenderFinish(ctx);
}

static void
intel_fallback_point(struct intel_context *intel,
		     intelVertex * v0)
{
   GLcontext *ctx = &intel->ctx;
   SWvertex v[1];

   if (0)
      fprintf(stderr, "\n%s\n", __FUNCTION__);

   INTEL_FIREVERTICES(intel);

   _swsetup_Translate(ctx, v0, &v[0]);
   intelSpanRenderStart(ctx);
   _swrast_Point(ctx, &v[0]);
   intelSpanRenderFinish(ctx);
}


/**********************************************************************/
/*               Render unclipped begin/end objects                   */
/**********************************************************************/

#define IND 0
#define V(x) (intelVertex *)(vertptr + ((x)*vertsize*sizeof(GLuint)))
#define RENDER_POINTS( start, count )	\
   for ( ; start < count ; start++) POINT( V(ELT(start)) );
#define RENDER_LINE( v0, v1 )         LINE( V(v0), V(v1) )
#define RENDER_TRI(  v0, v1, v2 )     TRI(  V(v0), V(v1), V(v2) )
#define RENDER_QUAD( v0, v1, v2, v3 ) QUAD( V(v0), V(v1), V(v2), V(v3) )
#define INIT(x) intelRenderPrimitive( ctx, x )
#undef LOCAL_VARS
#define LOCAL_VARS						\
    struct intel_context *intel = intel_context(ctx);			\
    GLubyte *vertptr = (GLubyte *)intel->verts;			\
    const GLuint vertsize = intel->vertex_size;       	\
    const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;	\
    (void) elt;
#define RESET_STIPPLE
#define RESET_OCCLUSION
#define PRESERVE_VB_DEFS
#define ELT(x) x
#define TAG(x) intel_##x##_verts
#include "tnl/t_vb_rendertmp.h"
#undef ELT
#undef TAG
#define TAG(x) intel_##x##_elts
#define ELT(x) elt[x]
#include "tnl/t_vb_rendertmp.h"

/**********************************************************************/
/*                   Render clipped primitives                        */
/**********************************************************************/



static void
intelRenderClippedPoly(GLcontext * ctx, const GLuint * elts, GLuint n)
{
   struct intel_context *intel = intel_context(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLuint prim = intel->render_primitive;

   /* Render the new vertices as an unclipped polygon.
    */
   {
      GLuint *tmp = VB->Elts;
      VB->Elts = (GLuint *) elts;
      tnl->Driver.Render.PrimTabElts[GL_POLYGON] (ctx, 0, n,
                                                  PRIM_BEGIN | PRIM_END);
      VB->Elts = tmp;
   }

   /* Restore the render primitive
    */
   if (prim != GL_POLYGON)
      tnl->Driver.Render.PrimitiveNotify(ctx, prim);
}

static void
intelRenderClippedLine(GLcontext * ctx, GLuint ii, GLuint jj)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   tnl->Driver.Render.Line(ctx, ii, jj);
}

static void
intelFastRenderClippedPoly(GLcontext * ctx, const GLuint * elts, GLuint n)
{
   struct intel_context *intel = intel_context(ctx);
   const GLuint vertsize = intel->vertex_size;
   GLuint *vb = intelExtendInlinePrimitive(intel, (n - 2) * 3 * vertsize);
   GLubyte *vertptr = (GLubyte *) intel->verts;
   const GLuint *start = (const GLuint *) V(elts[0]);
   int i, j;

   for (i = 2; i < n; i++) {
      COPY_DWORDS(j, vb, vertsize, V(elts[i - 1]));
      COPY_DWORDS(j, vb, vertsize, V(elts[i]));
      COPY_DWORDS(j, vb, vertsize, start);
   }
}

/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/




#define ANY_FALLBACK_FLAGS (DD_LINE_STIPPLE | DD_TRI_STIPPLE | DD_POINT_ATTEN | DD_POINT_SMOOTH | DD_TRI_SMOOTH)
#define ANY_RASTER_FLAGS (DD_TRI_LIGHT_TWOSIDE | DD_TRI_OFFSET | DD_TRI_UNFILLED)

void
intelChooseRenderState(GLcontext * ctx)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct intel_context *intel = intel_context(ctx);
   GLuint flags = ctx->_TriangleCaps;
   const struct gl_fragment_program *fprog = ctx->FragmentProgram._Current;
   GLboolean have_wpos = (fprog && (fprog->Base.InputsRead & FRAG_BIT_WPOS));
   GLuint index = 0;

   if (INTEL_DEBUG & DEBUG_STATE)
      fprintf(stderr, "\n%s\n", __FUNCTION__);

   if ((flags & (ANY_FALLBACK_FLAGS | ANY_RASTER_FLAGS)) || have_wpos) {

      if (flags & ANY_RASTER_FLAGS) {
         if (flags & DD_TRI_LIGHT_TWOSIDE)
            index |= INTEL_TWOSIDE_BIT;
         if (flags & DD_TRI_OFFSET)
            index |= INTEL_OFFSET_BIT;
         if (flags & DD_TRI_UNFILLED)
            index |= INTEL_UNFILLED_BIT;
      }

      if (have_wpos) {
         intel->draw_point = intel_wpos_point;
         intel->draw_line = intel_wpos_line;
         intel->draw_tri = intel_wpos_triangle;

         /* Make sure these get called:
          */
         index |= INTEL_FALLBACK_BIT;
      }
      else {
         intel->draw_point = intel_draw_point;
         intel->draw_line = intel_draw_line;
         intel->draw_tri = intel_draw_triangle;
      }

      /* Hook in fallbacks for specific primitives.
       */
      if (flags & ANY_FALLBACK_FLAGS) {
         if (flags & DD_LINE_STIPPLE)
            intel->draw_line = intel_fallback_line;

         if ((flags & DD_TRI_STIPPLE) && !intel->hw_stipple)
            intel->draw_tri = intel_fallback_tri;

         if (flags & DD_TRI_SMOOTH) {
	    if (intel->strict_conformance)
	       intel->draw_tri = intel_fallback_tri;
	 }

         if (flags & DD_POINT_ATTEN) {
	    if (0)
	       intel->draw_point = intel_atten_point;
	    else
	       intel->draw_point = intel_fallback_point;
	 }

	 if (flags & DD_POINT_SMOOTH) {
	    if (intel->strict_conformance)
	       intel->draw_point = intel_fallback_point;
	 }

         index |= INTEL_FALLBACK_BIT;
      }
   }

   if (intel->RenderIndex != index) {
      intel->RenderIndex = index;

      tnl->Driver.Render.Points = rast_tab[index].points;
      tnl->Driver.Render.Line = rast_tab[index].line;
      tnl->Driver.Render.Triangle = rast_tab[index].triangle;
      tnl->Driver.Render.Quad = rast_tab[index].quad;

      if (index == 0) {
         tnl->Driver.Render.PrimTabVerts = intel_render_tab_verts;
         tnl->Driver.Render.PrimTabElts = intel_render_tab_elts;
         tnl->Driver.Render.ClippedLine = line; /* from tritmp.h */
         tnl->Driver.Render.ClippedPolygon = intelFastRenderClippedPoly;
      }
      else {
         tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
         tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
         tnl->Driver.Render.ClippedLine = intelRenderClippedLine;
         tnl->Driver.Render.ClippedPolygon = intelRenderClippedPoly;
      }
   }
}

static const GLenum reduced_prim[GL_POLYGON + 1] = {
   GL_POINTS,
   GL_LINES,
   GL_LINES,
   GL_LINES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES
};


/**********************************************************************/
/*                 High level hooks for t_vb_render.c                 */
/**********************************************************************/




static void
intelRunPipeline(GLcontext * ctx)
{
   struct intel_context *intel = intel_context(ctx);

   _mesa_lock_context_textures(ctx);
   
   if (ctx->NewState)
      _mesa_update_state_locked(ctx);

   if (intel->NewGLState) {
      if (intel->NewGLState & _NEW_TEXTURE) {
         intel->vtbl.update_texture_state(intel);
      }

      if (!intel->Fallback) {
         if (intel->NewGLState & _INTEL_NEW_RENDERSTATE)
            intelChooseRenderState(ctx);
      }

      intel->NewGLState = 0;
   }

   _tnl_run_pipeline(ctx);

   _mesa_unlock_context_textures(ctx);
}

static void
intelRenderStart(GLcontext * ctx)
{
   struct intel_context *intel = intel_context(ctx);

   intel->vtbl.render_start(intel_context(ctx));
   intel->vtbl.emit_state(intel);
}

static void
intelRenderFinish(GLcontext * ctx)
{
   struct intel_context *intel = intel_context(ctx);

   if (intel->RenderIndex & INTEL_FALLBACK_BIT)
      _swrast_flush(ctx);

   INTEL_FIREVERTICES(intel);
}




 /* System to flush dma and emit state changes based on the rasterized
  * primitive.
  */
static void
intelRasterPrimitive(GLcontext * ctx, GLenum rprim, GLuint hwprim)
{
   struct intel_context *intel = intel_context(ctx);

   if (0)
      fprintf(stderr, "%s %s %x\n", __FUNCTION__,
              _mesa_lookup_enum_by_nr(rprim), hwprim);

   intel->vtbl.reduced_primitive_state(intel, rprim);

   /* Start a new primitive.  Arrange to have it flushed later on.
    */
   if (hwprim != intel->prim.primitive) {
      INTEL_FIREVERTICES(intel);

      intelStartInlinePrimitive(intel, hwprim, INTEL_BATCH_CLIPRECTS);
   }
}


 /* 
  */
static void
intelRenderPrimitive(GLcontext * ctx, GLenum prim)
{
   struct intel_context *intel = intel_context(ctx);

   if (0)
      fprintf(stderr, "%s %s\n", __FUNCTION__, _mesa_lookup_enum_by_nr(prim));

   /* Let some clipping routines know which primitive they're dealing
    * with.
    */
   intel->render_primitive = prim;

   /* Shortcircuit this when called from t_dd_rendertmp.h for unfilled
    * triangles.  The rasterized primitive will always be reset by
    * lower level functions in that case, potentially pingponging the
    * state:
    */
   if (reduced_prim[prim] == GL_TRIANGLES &&
       (ctx->_TriangleCaps & DD_TRI_UNFILLED))
      return;

   /* Set some primitive-dependent state and Start? a new primitive.
    */
   intelRasterPrimitive(ctx, reduced_prim[prim], hw_prim[prim]);
}


 /**********************************************************************/
 /*           Transition to/from hardware rasterization.               */
 /**********************************************************************/

static char *fallbackStrings[] = {
   [0] = "Draw buffer",
   [1] = "Read buffer",
   [2] = "Depth buffer",
   [3] = "Stencil buffer",
   [4] = "User disable",
   [5] = "Render mode",

   [12] = "Texture",
   [13] = "Color mask",
   [14] = "Stencil",
   [15] = "Stipple",
   [16] = "Program",
   [17] = "Logic op",
   [18] = "Smooth polygon",
   [19] = "Smooth point",
};


static char *
getFallbackString(GLuint bit)
{
   int i = 0;
   while (bit > 1) {
      i++;
      bit >>= 1;
   }
   return fallbackStrings[i];
}



void
intelFallback(struct intel_context *intel, GLuint bit, GLboolean mode)
{
   GLcontext *ctx = &intel->ctx;
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint oldfallback = intel->Fallback;

   if (mode) {
      intel->Fallback |= bit;
      if (oldfallback == 0) {
         intelFlush(ctx);
         if (INTEL_DEBUG & DEBUG_FALLBACKS)
            fprintf(stderr, "ENTER FALLBACK %x: %s\n",
                    bit, getFallbackString(bit));
         _swsetup_Wakeup(ctx);
         intel->RenderIndex = ~0;
      }
   }
   else {
      intel->Fallback &= ~bit;
      if (oldfallback == bit) {
         _swrast_flush(ctx);
         if (INTEL_DEBUG & DEBUG_FALLBACKS)
            fprintf(stderr, "LEAVE FALLBACK %s\n", getFallbackString(bit));
         tnl->Driver.Render.Start = intelRenderStart;
         tnl->Driver.Render.PrimitiveNotify = intelRenderPrimitive;
         tnl->Driver.Render.Finish = intelRenderFinish;
         tnl->Driver.Render.BuildVertices = _tnl_build_vertices;
         tnl->Driver.Render.CopyPV = _tnl_copy_pv;
         tnl->Driver.Render.Interp = _tnl_interp;

         _tnl_invalidate_vertex_state(ctx, ~0);
         _tnl_invalidate_vertices(ctx, ~0);
         _tnl_install_attrs(ctx,
                            intel->vertex_attrs,
                            intel->vertex_attr_count,
                            intel->ViewportMatrix.m, 0);

         intel->NewGLState |= _INTEL_NEW_RENDERSTATE;
      }
   }
}

union fi
{
   GLfloat f;
   GLint i;
};


/**********************************************************************/
/*             Used only with the metaops callbacks.                  */
/**********************************************************************/
void
intel_meta_draw_poly(struct intel_context *intel,
                     GLuint n,
                     GLfloat xy[][2],
                     GLfloat z, GLuint color, GLfloat tex[][2])
{
   union fi *vb;
   GLint i;

   /* All 3d primitives should be emitted with INTEL_BATCH_CLIPRECTS,
    * otherwise the drawing origin (DR4) might not be set correctly.
    */
   intelStartInlinePrimitive(intel, PRIM3D_TRIFAN, INTEL_BATCH_CLIPRECTS);
   vb = (union fi *) intelExtendInlinePrimitive(intel, n * 6);

   for (i = 0; i < n; i++) {
      vb[0].f = xy[i][0];
      vb[1].f = xy[i][1];
      vb[2].f = z;
      vb[3].i = color;
      vb[4].f = tex[i][0];
      vb[5].f = tex[i][1];
      vb += 6;
   }

   INTEL_FIREVERTICES(intel);
}

void
intel_meta_draw_quad(struct intel_context *intel,
                     GLfloat x0, GLfloat x1,
                     GLfloat y0, GLfloat y1,
                     GLfloat z,
                     GLuint color,
                     GLfloat s0, GLfloat s1, GLfloat t0, GLfloat t1)
{
   GLfloat xy[4][2];
   GLfloat tex[4][2];

   xy[0][0] = x0;
   xy[0][1] = y0;
   xy[1][0] = x1;
   xy[1][1] = y0;
   xy[2][0] = x1;
   xy[2][1] = y1;
   xy[3][0] = x0;
   xy[3][1] = y1;

   tex[0][0] = s0;
   tex[0][1] = t0;
   tex[1][0] = s1;
   tex[1][1] = t0;
   tex[2][0] = s1;
   tex[2][1] = t1;
   tex[3][0] = s0;
   tex[3][1] = t1;

   intel_meta_draw_poly(intel, 4, xy, z, color, tex);
}



/**********************************************************************/
/*                            Initialization.                         */
/**********************************************************************/


void
intelInitTriFuncs(GLcontext * ctx)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   static int firsttime = 1;

   if (firsttime) {
      init_rast_tab();
      firsttime = 0;
   }

   tnl->Driver.RunPipeline = intelRunPipeline;
   tnl->Driver.Render.Start = intelRenderStart;
   tnl->Driver.Render.Finish = intelRenderFinish;
   tnl->Driver.Render.PrimitiveNotify = intelRenderPrimitive;
   tnl->Driver.Render.ResetLineStipple = _swrast_ResetLineStipple;
   tnl->Driver.Render.BuildVertices = _tnl_build_vertices;
   tnl->Driver.Render.CopyPV = _tnl_copy_pv;
   tnl->Driver.Render.Interp = _tnl_interp;
}
