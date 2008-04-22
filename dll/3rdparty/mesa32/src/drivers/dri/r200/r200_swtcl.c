/* $XFree86: xc/lib/GL/mesa/src/drv/r200/r200_swtcl.c,v 1.5 2003/05/06 23:52:08 daenzer Exp $ */
/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "mtypes.h"
#include "colormac.h"
#include "enums.h"
#include "image.h"
#include "imports.h"
#include "macros.h"

#include "swrast/s_context.h"
#include "swrast/s_fog.h"
#include "swrast_setup/swrast_setup.h"
#include "math/m_translate.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#include "r200_context.h"
#include "r200_ioctl.h"
#include "r200_state.h"
#include "r200_swtcl.h"
#include "r200_tcl.h"


static void flush_last_swtcl_prim( r200ContextPtr rmesa  );


/***********************************************************************
 *                         Initialization 
 ***********************************************************************/

#define EMIT_ATTR( ATTR, STYLE, F0 )					\
do {									\
   rmesa->swtcl.vertex_attrs[rmesa->swtcl.vertex_attr_count].attrib = (ATTR);	\
   rmesa->swtcl.vertex_attrs[rmesa->swtcl.vertex_attr_count].format = (STYLE);	\
   rmesa->swtcl.vertex_attr_count++;					\
   fmt_0 |= F0;								\
} while (0)

#define EMIT_PAD( N )							\
do {									\
   rmesa->swtcl.vertex_attrs[rmesa->swtcl.vertex_attr_count].attrib = 0;		\
   rmesa->swtcl.vertex_attrs[rmesa->swtcl.vertex_attr_count].format = EMIT_PAD;	\
   rmesa->swtcl.vertex_attrs[rmesa->swtcl.vertex_attr_count].offset = (N);		\
   rmesa->swtcl.vertex_attr_count++;					\
} while (0)

static void r200SetVertexFormat( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   DECLARE_RENDERINPUTS(index_bitset);
   int fmt_0 = 0;
   int fmt_1 = 0;
   int offset = 0;

   RENDERINPUTS_COPY( index_bitset, tnl->render_inputs_bitset );

   /* Important:
    */
   if ( VB->NdcPtr != NULL ) {
      VB->AttribPtr[VERT_ATTRIB_POS] = VB->NdcPtr;
   }
   else {
      VB->AttribPtr[VERT_ATTRIB_POS] = VB->ClipPtr;
   }

   assert( VB->AttribPtr[VERT_ATTRIB_POS] != NULL );
   rmesa->swtcl.vertex_attr_count = 0;

   /* EMIT_ATTR's must be in order as they tell t_vertex.c how to
    * build up a hardware vertex.
    */
   if ( !rmesa->swtcl.needproj ||
       RENDERINPUTS_TEST_RANGE( index_bitset, _TNL_FIRST_TEX, _TNL_LAST_TEX )) { /* need w coord for projected textures */
      EMIT_ATTR( _TNL_ATTRIB_POS, EMIT_4F, R200_VTX_XY | R200_VTX_Z0 | R200_VTX_W0 );
      offset = 4;
   }
   else {
      EMIT_ATTR( _TNL_ATTRIB_POS, EMIT_3F, R200_VTX_XY | R200_VTX_Z0 );
      offset = 3;
   }

   if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_POINTSIZE )) {
      EMIT_ATTR( _TNL_ATTRIB_POINTSIZE, EMIT_1F, R200_VTX_POINT_SIZE );
      offset += 1;
   }

   rmesa->swtcl.coloroffset = offset;
#if MESA_LITTLE_ENDIAN 
   EMIT_ATTR( _TNL_ATTRIB_COLOR0, EMIT_4UB_4F_RGBA, (R200_VTX_PK_RGBA << R200_VTX_COLOR_0_SHIFT) );
#else
   EMIT_ATTR( _TNL_ATTRIB_COLOR0, EMIT_4UB_4F_ABGR, (R200_VTX_PK_RGBA << R200_VTX_COLOR_0_SHIFT) );
#endif
   offset += 1;

   rmesa->swtcl.specoffset = 0;
   if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_COLOR1 ) ||
       RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_FOG )) {

#if MESA_LITTLE_ENDIAN 
      if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_COLOR1 )) {
	 rmesa->swtcl.specoffset = offset;
	 EMIT_ATTR( _TNL_ATTRIB_COLOR1, EMIT_3UB_3F_RGB, (R200_VTX_PK_RGBA << R200_VTX_COLOR_1_SHIFT) );
      }
      else {
	 EMIT_PAD( 3 );
      }

      if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_FOG )) {
	 EMIT_ATTR( _TNL_ATTRIB_FOG, EMIT_1UB_1F, (R200_VTX_PK_RGBA << R200_VTX_COLOR_1_SHIFT) );
      }
      else {
	 EMIT_PAD( 1 );
      }
#else
      if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_FOG )) {
	 EMIT_ATTR( _TNL_ATTRIB_FOG, EMIT_1UB_1F, (R200_VTX_PK_RGBA << R200_VTX_COLOR_1_SHIFT) );
      }
      else {
	 EMIT_PAD( 1 );
      }

      if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_COLOR1 )) {
	 rmesa->swtcl.specoffset = offset;
	 EMIT_ATTR( _TNL_ATTRIB_COLOR1, EMIT_3UB_3F_BGR, (R200_VTX_PK_RGBA << R200_VTX_COLOR_1_SHIFT) );
      }
      else {
	 EMIT_PAD( 3 );
      }
#endif
   }

   if (RENDERINPUTS_TEST_RANGE( index_bitset, _TNL_FIRST_TEX, _TNL_LAST_TEX )) {
      int i;

      for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
	 if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_TEX(i) )) {
	    GLuint sz = VB->TexCoordPtr[i]->size;

	    fmt_1 |= sz << (3 * i);
	    EMIT_ATTR( _TNL_ATTRIB_TEX0+i, EMIT_1F + sz - 1, 0 );
	 }
      }
   }

   if ( (rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] & R200_FOG_USE_MASK)
      != R200_FOG_USE_SPEC_ALPHA ) {
      R200_STATECHANGE( rmesa, ctx );
      rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] &= ~R200_FOG_USE_MASK;
      rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] |= R200_FOG_USE_SPEC_ALPHA;
   }

   if (!RENDERINPUTS_EQUAL( rmesa->tnl_index_bitset, index_bitset ) ||
	(rmesa->hw.vtx.cmd[VTX_VTXFMT_0] != fmt_0) ||
	(rmesa->hw.vtx.cmd[VTX_VTXFMT_1] != fmt_1) ) {
      R200_NEWPRIM(rmesa);
      R200_STATECHANGE( rmesa, vtx );
      rmesa->hw.vtx.cmd[VTX_VTXFMT_0] = fmt_0;
      rmesa->hw.vtx.cmd[VTX_VTXFMT_1] = fmt_1;

      rmesa->swtcl.vertex_size =
	  _tnl_install_attrs( ctx,
			      rmesa->swtcl.vertex_attrs, 
			      rmesa->swtcl.vertex_attr_count,
			      NULL, 0 );
      rmesa->swtcl.vertex_size /= 4;
      RENDERINPUTS_COPY( rmesa->tnl_index_bitset, index_bitset );
   }
}


static void r200RenderStart( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT( ctx );

   r200SetVertexFormat( ctx );

   if (rmesa->dma.flush != 0 && 
       rmesa->dma.flush != flush_last_swtcl_prim)
      rmesa->dma.flush( rmesa );
}


/**
 * Set vertex state for SW TCL.  The primary purpose of this function is to
 * determine in advance whether or not the hardware can / should do the
 * projection divide or Mesa should do it.
 */
void r200ChooseVertexState( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint vte;
   GLuint vap;

   /* We must ensure that we don't do _tnl_need_projected_coords while in a
    * rasterization fallback.  As this function will be called again when we
    * leave a rasterization fallback, we can just skip it for now.
    */
   if (rmesa->Fallback != 0)
      return;

   vte = rmesa->hw.vte.cmd[VTE_SE_VTE_CNTL];
   vap = rmesa->hw.vap.cmd[VAP_SE_VAP_CNTL];

   /* HW perspective divide is a win, but tiny vertex formats are a
    * bigger one.
    */
   if (!RENDERINPUTS_TEST_RANGE( tnl->render_inputs_bitset, _TNL_FIRST_TEX, _TNL_LAST_TEX )
	|| (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
      rmesa->swtcl.needproj = GL_TRUE;
      vte |= R200_VTX_XY_FMT | R200_VTX_Z_FMT;
      vte &= ~R200_VTX_W0_FMT;
      if (RENDERINPUTS_TEST_RANGE( tnl->render_inputs_bitset, _TNL_FIRST_TEX, _TNL_LAST_TEX )) {
	 vap &= ~R200_VAP_FORCE_W_TO_ONE;
      }
      else {
	 vap |= R200_VAP_FORCE_W_TO_ONE;
      }
   }
   else {
      rmesa->swtcl.needproj = GL_FALSE;
      vte &= ~(R200_VTX_XY_FMT | R200_VTX_Z_FMT);
      vte |= R200_VTX_W0_FMT;
      vap &= ~R200_VAP_FORCE_W_TO_ONE;
   }

   _tnl_need_projected_coords( ctx, rmesa->swtcl.needproj );

   if (vte != rmesa->hw.vte.cmd[VTE_SE_VTE_CNTL]) {
      R200_STATECHANGE( rmesa, vte );
      rmesa->hw.vte.cmd[VTE_SE_VTE_CNTL] = vte;
   }

   if (vap != rmesa->hw.vap.cmd[VAP_SE_VAP_CNTL]) {
      R200_STATECHANGE( rmesa, vap );
      rmesa->hw.vap.cmd[VAP_SE_VAP_CNTL] = vap;
   }
}


/* Flush vertices in the current dma region.
 */
static void flush_last_swtcl_prim( r200ContextPtr rmesa  )
{
   if (R200_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   rmesa->dma.flush = NULL;

   if (rmesa->dma.current.buf) {
      struct r200_dma_region *current = &rmesa->dma.current;
      GLuint current_offset = (rmesa->r200Screen->gart_buffer_offset +
			       current->buf->buf->idx * RADEON_BUFFER_SIZE + 
			       current->start);

      assert (!(rmesa->swtcl.hw_primitive & R200_VF_PRIM_WALK_IND));

      assert (current->start + 
	      rmesa->swtcl.numverts * rmesa->swtcl.vertex_size * 4 ==
	      current->ptr);

      if (rmesa->dma.current.start != rmesa->dma.current.ptr) {
	 r200EnsureCmdBufSpace( rmesa, VERT_AOS_BUFSZ +
			        rmesa->hw.max_state_size + VBUF_BUFSZ );
	 r200EmitVertexAOS( rmesa,
			      rmesa->swtcl.vertex_size,
			      current_offset);

	 r200EmitVbufPrim( rmesa,
			   rmesa->swtcl.hw_primitive,
			   rmesa->swtcl.numverts);
      }

      rmesa->swtcl.numverts = 0;
      current->start = current->ptr;
   }
}


/* Alloc space in the current dma region.
 */
static INLINE void *
r200AllocDmaLowVerts( r200ContextPtr rmesa, int nverts, int vsize )
{
   GLuint bytes = vsize * nverts;

   if ( rmesa->dma.current.ptr + bytes > rmesa->dma.current.end ) 
      r200RefillCurrentDmaRegion( rmesa );

   if (!rmesa->dma.flush) {
      rmesa->glCtx->Driver.NeedFlush |= FLUSH_STORED_VERTICES;
      rmesa->dma.flush = flush_last_swtcl_prim;
   }

   ASSERT( vsize == rmesa->swtcl.vertex_size * 4 );
   ASSERT( rmesa->dma.flush == flush_last_swtcl_prim );
   ASSERT( rmesa->dma.current.start + 
	   rmesa->swtcl.numverts * rmesa->swtcl.vertex_size * 4 ==
	   rmesa->dma.current.ptr );


   {
      GLubyte *head = (GLubyte *) (rmesa->dma.current.address + rmesa->dma.current.ptr);
      rmesa->dma.current.ptr += bytes;
      rmesa->swtcl.numverts += nverts;
      return head;
   }

}


/**************************************************************************/


static INLINE GLuint reduced_hw_prim( GLcontext *ctx, GLuint prim)
{
   switch (prim) {
   case GL_POINTS:
      return (ctx->Point.PointSprite ||
	 ((ctx->_TriangleCaps & (DD_POINT_SIZE | DD_POINT_ATTEN)) &&
	 !(ctx->_TriangleCaps & (DD_POINT_SMOOTH)))) ?
	 R200_VF_PRIM_POINT_SPRITES : R200_VF_PRIM_POINTS;
   case GL_LINES:
   /* fallthrough */
   case GL_LINE_LOOP:
   /* fallthrough */
   case GL_LINE_STRIP:
      return R200_VF_PRIM_LINES;
   default:
   /* all others reduced to triangles */
      return R200_VF_PRIM_TRIANGLES;
   }
}


static void r200RasterPrimitive( GLcontext *ctx, GLuint hwprim );
static void r200RenderPrimitive( GLcontext *ctx, GLenum prim );
static void r200ResetLineStipple( GLcontext *ctx );

/***********************************************************************
 *                    Emit primitives as inline vertices               *
 ***********************************************************************/

#define HAVE_POINTS      1
#define HAVE_LINES       1
#define HAVE_LINE_STRIPS 1
#define HAVE_TRIANGLES   1
#define HAVE_TRI_STRIPS  1
#define HAVE_TRI_STRIP_1 0
#define HAVE_TRI_FANS    1
#define HAVE_QUADS       0
#define HAVE_QUAD_STRIPS 0
#define HAVE_POLYGONS    1
#define HAVE_ELTS        0

#undef LOCAL_VARS
#undef ALLOC_VERTS
#define CTX_ARG r200ContextPtr rmesa
#define GET_VERTEX_DWORDS() rmesa->swtcl.vertex_size
#define ALLOC_VERTS( n, size ) r200AllocDmaLowVerts( rmesa, n, size * 4 )
#define LOCAL_VARS						\
   r200ContextPtr rmesa = R200_CONTEXT(ctx);		\
   const char *r200verts = (char *)rmesa->swtcl.verts;
#define VERT(x) (r200Vertex *)(r200verts + ((x) * vertsize * sizeof(int)))
#define VERTEX r200Vertex 
#define DO_DEBUG_VERTS (1 && (R200_DEBUG & DEBUG_VERTS))

#undef TAG
#define TAG(x) r200_##x
#include "tnl_dd/t_dd_triemit.h"


/***********************************************************************
 *          Macros for t_dd_tritmp.h to draw basic primitives          *
 ***********************************************************************/

#define QUAD( a, b, c, d ) r200_quad( rmesa, a, b, c, d )
#define TRI( a, b, c )     r200_triangle( rmesa, a, b, c )
#define LINE( a, b )       r200_line( rmesa, a, b )
#define POINT( a )         r200_point( rmesa, a )

/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/

#define R200_TWOSIDE_BIT	0x01
#define R200_UNFILLED_BIT	0x02
#define R200_MAX_TRIFUNC	0x04


static struct {
   tnl_points_func	        points;
   tnl_line_func		line;
   tnl_triangle_func	triangle;
   tnl_quad_func		quad;
} rast_tab[R200_MAX_TRIFUNC];


#define DO_FALLBACK  0
#define DO_UNFILLED (IND & R200_UNFILLED_BIT)
#define DO_TWOSIDE  (IND & R200_TWOSIDE_BIT)
#define DO_FLAT      0
#define DO_OFFSET     0
#define DO_TRI       1
#define DO_QUAD      1
#define DO_LINE      1
#define DO_POINTS    1
#define DO_FULL_QUAD 1

#define HAVE_RGBA   1
#define HAVE_SPEC   1
#define HAVE_BACK_COLORS  0
#define HAVE_HW_FLATSHADE 1
#define TAB rast_tab

#define DEPTH_SCALE 1.0
#define UNFILLED_TRI unfilled_tri
#define UNFILLED_QUAD unfilled_quad
#define VERT_X(_v) _v->v.x
#define VERT_Y(_v) _v->v.y
#define VERT_Z(_v) _v->v.z
#define AREA_IS_CCW( a ) (a < 0)
#define GET_VERTEX(e) (rmesa->swtcl.verts + (e*rmesa->swtcl.vertex_size*sizeof(int)))

#define VERT_SET_RGBA( v, c )  					\
do {								\
   r200_color_t *color = (r200_color_t *)&((v)->ui[coloroffset]);	\
   UNCLAMPED_FLOAT_TO_UBYTE(color->red, (c)[0]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->green, (c)[1]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->blue, (c)[2]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->alpha, (c)[3]);		\
} while (0)

#define VERT_COPY_RGBA( v0, v1 ) v0->ui[coloroffset] = v1->ui[coloroffset]

#define VERT_SET_SPEC( v, c )					\
do {								\
   if (specoffset) {						\
      r200_color_t *spec = (r200_color_t *)&((v)->ui[specoffset]);	\
      UNCLAMPED_FLOAT_TO_UBYTE(spec->red, (c)[0]);	\
      UNCLAMPED_FLOAT_TO_UBYTE(spec->green, (c)[1]);	\
      UNCLAMPED_FLOAT_TO_UBYTE(spec->blue, (c)[2]);	\
   }								\
} while (0)
#define VERT_COPY_SPEC( v0, v1 )			\
do {							\
   if (specoffset) {					\
      r200_color_t *spec0 = (r200_color_t *)&((v0)->ui[specoffset]);	\
      r200_color_t *spec1 = (r200_color_t *)&((v1)->ui[specoffset]);	\
      spec0->red   = spec1->red;	\
      spec0->green = spec1->green;	\
      spec0->blue  = spec1->blue; 	\
   }							\
} while (0)

/* These don't need LE32_TO_CPU() as they used to save and restore
 * colors which are already in the correct format.
 */
#define VERT_SAVE_RGBA( idx )    color[idx] = v[idx]->ui[coloroffset]
#define VERT_RESTORE_RGBA( idx ) v[idx]->ui[coloroffset] = color[idx]
#define VERT_SAVE_SPEC( idx )    if (specoffset) spec[idx] = v[idx]->ui[specoffset]
#define VERT_RESTORE_SPEC( idx ) if (specoffset) v[idx]->ui[specoffset] = spec[idx]

#undef LOCAL_VARS
#undef TAG
#undef INIT

#define LOCAL_VARS(n)							\
   r200ContextPtr rmesa = R200_CONTEXT(ctx);			\
   GLuint color[n], spec[n];						\
   GLuint coloroffset = rmesa->swtcl.coloroffset;	\
   GLuint specoffset = rmesa->swtcl.specoffset;			\
   (void) color; (void) spec; (void) coloroffset; (void) specoffset;

/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/

#define RASTERIZE(x) r200RasterPrimitive( ctx, reduced_hw_prim(ctx, x) )
#define RENDER_PRIMITIVE rmesa->swtcl.render_primitive
#undef TAG
#define TAG(x) x
#include "tnl_dd/t_dd_unfilled.h"
#undef IND


/***********************************************************************
 *                      Generate GL render functions                   *
 ***********************************************************************/


#define IND (0)
#define TAG(x) x
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R200_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R200_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R200_TWOSIDE_BIT|R200_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled
#include "tnl_dd/t_dd_tritmp.h"


static void init_rast_tab( void )
{
   init();
   init_twoside();
   init_unfilled();
   init_twoside_unfilled();
}

/**********************************************************************/
/*               Render unclipped begin/end objects                   */
/**********************************************************************/

#define RENDER_POINTS( start, count )		\
   for ( ; start < count ; start++)		\
      r200_point( rmesa, VERT(start) )
#define RENDER_LINE( v0, v1 ) \
   r200_line( rmesa, VERT(v0), VERT(v1) )
#define RENDER_TRI( v0, v1, v2 )  \
   r200_triangle( rmesa, VERT(v0), VERT(v1), VERT(v2) )
#define RENDER_QUAD( v0, v1, v2, v3 ) \
   r200_quad( rmesa, VERT(v0), VERT(v1), VERT(v2), VERT(v3) )
#define INIT(x) do {					\
   r200RenderPrimitive( ctx, x );			\
} while (0)
#undef LOCAL_VARS
#define LOCAL_VARS						\
   r200ContextPtr rmesa = R200_CONTEXT(ctx);		\
   const GLuint vertsize = rmesa->swtcl.vertex_size;		\
   const char *r200verts = (char *)rmesa->swtcl.verts;		\
   const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;	\
   const GLboolean stipple = ctx->Line.StippleFlag;		\
   (void) elt; (void) stipple;
#define RESET_STIPPLE	if ( stipple ) r200ResetLineStipple( ctx );
#define RESET_OCCLUSION
#define PRESERVE_VB_DEFS
#define ELT(x) (x)
#define TAG(x) r200_##x##_verts
#include "tnl/t_vb_rendertmp.h"
#undef ELT
#undef TAG
#define TAG(x) r200_##x##_elts
#define ELT(x) elt[x]
#include "tnl/t_vb_rendertmp.h"



/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/

void r200ChooseRenderState( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint index = 0;
   GLuint flags = ctx->_TriangleCaps;

   if (!rmesa->TclFallback || rmesa->Fallback) 
      return;

   if (flags & DD_TRI_LIGHT_TWOSIDE) index |= R200_TWOSIDE_BIT;
   if (flags & DD_TRI_UNFILLED)      index |= R200_UNFILLED_BIT;

   if (index != rmesa->swtcl.RenderIndex) {
      tnl->Driver.Render.Points = rast_tab[index].points;
      tnl->Driver.Render.Line = rast_tab[index].line;
      tnl->Driver.Render.ClippedLine = rast_tab[index].line;
      tnl->Driver.Render.Triangle = rast_tab[index].triangle;
      tnl->Driver.Render.Quad = rast_tab[index].quad;

      if (index == 0) {
	 tnl->Driver.Render.PrimTabVerts = r200_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = r200_render_tab_elts;
	 tnl->Driver.Render.ClippedPolygon = r200_fast_clipped_poly;
      } else {
	 tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
	 tnl->Driver.Render.ClippedPolygon = _tnl_RenderClippedPolygon;
      }

      rmesa->swtcl.RenderIndex = index;
   }
}


/**********************************************************************/
/*                 High level hooks for t_vb_render.c                 */
/**********************************************************************/


static void r200RasterPrimitive( GLcontext *ctx, GLuint hwprim )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   if (rmesa->swtcl.hw_primitive != hwprim) {
      /* need to disable perspective-correct texturing for point sprites */
      if ((hwprim & 0xf) == R200_VF_PRIM_POINT_SPRITES && ctx->Point.PointSprite) {
	 if (rmesa->hw.set.cmd[SET_RE_CNTL] & R200_PERSPECTIVE_ENABLE) {
	    R200_STATECHANGE( rmesa, set );
	    rmesa->hw.set.cmd[SET_RE_CNTL] &= ~R200_PERSPECTIVE_ENABLE;
	 }
      }
      else if (!(rmesa->hw.set.cmd[SET_RE_CNTL] & R200_PERSPECTIVE_ENABLE)) {
	 R200_STATECHANGE( rmesa, set );
	 rmesa->hw.set.cmd[SET_RE_CNTL] |= R200_PERSPECTIVE_ENABLE;
      }
      R200_NEWPRIM( rmesa );
      rmesa->swtcl.hw_primitive = hwprim;
   }
}

static void r200RenderPrimitive( GLcontext *ctx, GLenum prim )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   rmesa->swtcl.render_primitive = prim;
   if (prim < GL_TRIANGLES || !(ctx->_TriangleCaps & DD_TRI_UNFILLED)) 
      r200RasterPrimitive( ctx, reduced_hw_prim(ctx, prim) );
}

static void r200RenderFinish( GLcontext *ctx )
{
}

static void r200ResetLineStipple( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   R200_STATECHANGE( rmesa, lin );
}


/**********************************************************************/
/*           Transition to/from hardware rasterization.               */
/**********************************************************************/

static const char * const fallbackStrings[] = {
   "Texture mode",
   "glDrawBuffer(GL_FRONT_AND_BACK)",
   "glEnable(GL_STENCIL) without hw stencil buffer",
   "glRenderMode(selection or feedback)",
   "R200_NO_RAST",
   "Mixing GL_CLAMP_TO_BORDER and GL_CLAMP (or GL_MIRROR_CLAMP_ATI)"
};


static const char *getFallbackString(GLuint bit)
{
   int i = 0;
   while (bit > 1) {
      i++;
      bit >>= 1;
   }
   return fallbackStrings[i];
}


void r200Fallback( GLcontext *ctx, GLuint bit, GLboolean mode )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint oldfallback = rmesa->Fallback;

   if (mode) {
      rmesa->Fallback |= bit;
      if (oldfallback == 0) {
	 R200_FIREVERTICES( rmesa );
	 TCL_FALLBACK( ctx, R200_TCL_FALLBACK_RASTER, GL_TRUE );
	 _swsetup_Wakeup( ctx );
	 rmesa->swtcl.RenderIndex = ~0;
         if (R200_DEBUG & DEBUG_FALLBACKS) {
            fprintf(stderr, "R200 begin rasterization fallback: 0x%x %s\n",
                    bit, getFallbackString(bit));
         }
      }
   }
   else {
      rmesa->Fallback &= ~bit;
      if (oldfallback == bit) {

	 _swrast_flush( ctx );
	 tnl->Driver.Render.Start = r200RenderStart;
	 tnl->Driver.Render.PrimitiveNotify = r200RenderPrimitive;
	 tnl->Driver.Render.Finish = r200RenderFinish;

	 tnl->Driver.Render.BuildVertices = _tnl_build_vertices;
	 tnl->Driver.Render.CopyPV = _tnl_copy_pv;
	 tnl->Driver.Render.Interp = _tnl_interp;

	 tnl->Driver.Render.ResetLineStipple = r200ResetLineStipple;
	 TCL_FALLBACK( ctx, R200_TCL_FALLBACK_RASTER, GL_FALSE );
	 if (rmesa->TclFallback) {
	    /* These are already done if rmesa->TclFallback goes to
	     * zero above. But not if it doesn't (R200_NO_TCL for
	     * example?)
	     */
	    _tnl_invalidate_vertex_state( ctx, ~0 );
	    _tnl_invalidate_vertices( ctx, ~0 );
	    RENDERINPUTS_ZERO( rmesa->tnl_index_bitset );
	    r200ChooseVertexState( ctx );
	    r200ChooseRenderState( ctx );
	 }
         if (R200_DEBUG & DEBUG_FALLBACKS) {
            fprintf(stderr, "R200 end rasterization fallback: 0x%x %s\n",
                    bit, getFallbackString(bit));
         }
      }
   }
}




/**
 * Cope with depth operations by drawing individual pixels as points.
 * 
 * \todo
 * The way the vertex state is set in this routine is hokey.  It seems to
 * work, but it's very hackish.  This whole routine is pretty hackish.  If
 * the bitmap is small enough, it seems like it would be faster to copy it
 * to AGP memory and use it as a non-power-of-two texture (i.e.,
 * NV_texture_rectangle).
 */
void
r200PointsBitmap( GLcontext *ctx, GLint px, GLint py,
		  GLsizei width, GLsizei height,
		  const struct gl_pixelstore_attrib *unpack,
		  const GLubyte *bitmap )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   const GLfloat *rc = ctx->Current.RasterColor; 
   GLint row, col;
   r200Vertex vert;
   GLuint orig_vte;
   GLuint h;


   /* Turn off tcl.  
    */
   TCL_FALLBACK( ctx, R200_TCL_FALLBACK_BITMAP, 1 );

   /* Choose tiny vertex format
    */
   {
      const GLuint fmt_0 = R200_VTX_XY | R200_VTX_Z0 | R200_VTX_W0
	  | (R200_VTX_PK_RGBA << R200_VTX_COLOR_0_SHIFT);
      const GLuint fmt_1 = 0;
      GLuint vte = rmesa->hw.vte.cmd[VTE_SE_VTE_CNTL];
      GLuint vap = rmesa->hw.vap.cmd[VAP_SE_VAP_CNTL];

      vte &= ~(R200_VTX_XY_FMT | R200_VTX_Z_FMT);
      vte |= R200_VTX_W0_FMT;
      vap &= ~R200_VAP_FORCE_W_TO_ONE;

      rmesa->swtcl.vertex_size = 5;

      if ( (rmesa->hw.vtx.cmd[VTX_VTXFMT_0] != fmt_0)
	   || (rmesa->hw.vtx.cmd[VTX_VTXFMT_1] != fmt_1) ) {
	 R200_NEWPRIM(rmesa);
	 R200_STATECHANGE( rmesa, vtx );
	 rmesa->hw.vtx.cmd[VTX_VTXFMT_0] = fmt_0;
	 rmesa->hw.vtx.cmd[VTX_VTXFMT_1] = fmt_1;
      }

      if (vte != rmesa->hw.vte.cmd[VTE_SE_VTE_CNTL]) {
	 R200_STATECHANGE( rmesa, vte );
	 rmesa->hw.vte.cmd[VTE_SE_VTE_CNTL] = vte;
      }

      if (vap != rmesa->hw.vap.cmd[VAP_SE_VAP_CNTL]) {
	 R200_STATECHANGE( rmesa, vap );
	 rmesa->hw.vap.cmd[VAP_SE_VAP_CNTL] = vap;
      }
   }

   /* Ready for point primitives:
    */
   r200RenderPrimitive( ctx, GL_POINTS );

   /* Turn off the hw viewport transformation:
    */
   R200_STATECHANGE( rmesa, vte );
   orig_vte = rmesa->hw.vte.cmd[VTE_SE_VTE_CNTL];
   rmesa->hw.vte.cmd[VTE_SE_VTE_CNTL] &= ~(R200_VPORT_X_SCALE_ENA |
					   R200_VPORT_Y_SCALE_ENA |
					   R200_VPORT_Z_SCALE_ENA |
					   R200_VPORT_X_OFFSET_ENA |
					   R200_VPORT_Y_OFFSET_ENA |
					   R200_VPORT_Z_OFFSET_ENA); 

   /* Turn off other stuff:  Stipple?, texture?, blending?, etc.
    */


   /* Populate the vertex
    *
    * Incorporate FOG into RGBA
    */
   if (ctx->Fog.Enabled) {
      const GLfloat *fc = ctx->Fog.Color;
      GLfloat color[4];
      GLfloat f;

      if (ctx->Fog.FogCoordinateSource == GL_FOG_COORDINATE_EXT)
         f = _swrast_z_to_fogfactor(ctx, ctx->Current.Attrib[VERT_ATTRIB_FOG][0]);
      else
         f = _swrast_z_to_fogfactor(ctx, ctx->Current.RasterDistance);

      color[0] = f * rc[0] + (1.F - f) * fc[0];
      color[1] = f * rc[1] + (1.F - f) * fc[1];
      color[2] = f * rc[2] + (1.F - f) * fc[2];
      color[3] = rc[3];

      UNCLAMPED_FLOAT_TO_CHAN(vert.tv.color.red,   color[0]);
      UNCLAMPED_FLOAT_TO_CHAN(vert.tv.color.green, color[1]);
      UNCLAMPED_FLOAT_TO_CHAN(vert.tv.color.blue,  color[2]);
      UNCLAMPED_FLOAT_TO_CHAN(vert.tv.color.alpha, color[3]);
   }
   else {
      UNCLAMPED_FLOAT_TO_CHAN(vert.tv.color.red,   rc[0]);
      UNCLAMPED_FLOAT_TO_CHAN(vert.tv.color.green, rc[1]);
      UNCLAMPED_FLOAT_TO_CHAN(vert.tv.color.blue,  rc[2]);
      UNCLAMPED_FLOAT_TO_CHAN(vert.tv.color.alpha, rc[3]);
   }


   vert.tv.z = ctx->Current.RasterPos[2];


   /* Update window height
    */
   LOCK_HARDWARE( rmesa );
   UNLOCK_HARDWARE( rmesa );
   h = rmesa->dri.drawable->h + rmesa->dri.drawable->y;
   px += rmesa->dri.drawable->x;

   /* Clipping handled by existing mechansims in r200_ioctl.c?
    */
   for (row=0; row<height; row++) {
      const GLubyte *src = (const GLubyte *) 
	 _mesa_image_address2d(unpack, bitmap, width, height, 
                               GL_COLOR_INDEX, GL_BITMAP, row, 0 );

      if (unpack->LsbFirst) {
         /* Lsb first */
         GLubyte mask = 1U << (unpack->SkipPixels & 0x7);
         for (col=0; col<width; col++) {
            if (*src & mask) {
	       vert.tv.x = px+col;
	       vert.tv.y = h - (py+row) - 1;
	       r200_point( rmesa, &vert );
            }
	    src += (mask >> 7);
	    mask = ((mask << 1) & 0xff) | (mask >> 7);
         }

         /* get ready for next row */
         if (mask != 1)
            src++;
      }
      else {
         /* Msb first */
         GLubyte mask = 128U >> (unpack->SkipPixels & 0x7);
         for (col=0; col<width; col++) {
            if (*src & mask) {
	       vert.tv.x = px+col;
	       vert.tv.y = h - (py+row) - 1;
	       r200_point( rmesa, &vert );
            }
	    src += mask & 1;
	    mask = ((mask << 7) & 0xff) | (mask >> 1);
         }
         /* get ready for next row */
         if (mask != 128)
            src++;
      }
   }

   /* Fire outstanding vertices, restore state
    */
   R200_STATECHANGE( rmesa, vte );
   rmesa->hw.vte.cmd[VTE_SE_VTE_CNTL] = orig_vte;

   /* Unfallback
    */
   TCL_FALLBACK( ctx, R200_TCL_FALLBACK_BITMAP, 0 );

   /* Need to restore vertexformat?
    */
   if (rmesa->TclFallback)
      r200ChooseVertexState( ctx );
}



/**********************************************************************/
/*                            Initialization.                         */
/**********************************************************************/

void r200InitSwtcl( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   static int firsttime = 1;

   if (firsttime) {
      init_rast_tab();
      firsttime = 0;
   }

   tnl->Driver.Render.Start = r200RenderStart;
   tnl->Driver.Render.Finish = r200RenderFinish;
   tnl->Driver.Render.PrimitiveNotify = r200RenderPrimitive;
   tnl->Driver.Render.ResetLineStipple = r200ResetLineStipple;
   tnl->Driver.Render.BuildVertices = _tnl_build_vertices;
   tnl->Driver.Render.CopyPV = _tnl_copy_pv;
   tnl->Driver.Render.Interp = _tnl_interp;

   /* FIXME: what are these numbers? */
   _tnl_init_vertices( ctx, ctx->Const.MaxArrayLockSize + 12, 
		       36 * sizeof(GLfloat) );
   
   rmesa->swtcl.verts = (GLubyte *)tnl->clipspace.vertex_buf;
   rmesa->swtcl.RenderIndex = ~0;
   rmesa->swtcl.render_primitive = GL_TRIANGLES;
   rmesa->swtcl.hw_primitive = 0;
}


void r200DestroySwtcl( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   if (rmesa->swtcl.indexed_verts.buf) 
      r200ReleaseDmaRegion( rmesa, &rmesa->swtcl.indexed_verts, __FUNCTION__ );
}
