/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_tris.c,v 1.8 2002/10/30 12:51:43 alanh Exp $ */ /* -*- c-basic-offset: 3 -*- */
/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *
 */

#include "glheader.h"
#include "mtypes.h"
#include "colormac.h"
#include "macros.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#include "r128_tris.h"
#include "r128_state.h"
#include "r128_tex.h"
#include "r128_ioctl.h"

static const GLuint hw_prim[GL_POLYGON+1] = {
   R128_CCE_VC_CNTL_PRIM_TYPE_POINT,
   R128_CCE_VC_CNTL_PRIM_TYPE_LINE,
   R128_CCE_VC_CNTL_PRIM_TYPE_LINE,
   R128_CCE_VC_CNTL_PRIM_TYPE_LINE,
   R128_CCE_VC_CNTL_PRIM_TYPE_TRI_LIST,
   R128_CCE_VC_CNTL_PRIM_TYPE_TRI_LIST,
   R128_CCE_VC_CNTL_PRIM_TYPE_TRI_LIST,
   R128_CCE_VC_CNTL_PRIM_TYPE_TRI_LIST,
   R128_CCE_VC_CNTL_PRIM_TYPE_TRI_LIST,
   R128_CCE_VC_CNTL_PRIM_TYPE_TRI_LIST,
};

static void r128RasterPrimitive( GLcontext *ctx, GLuint hwprim );
static void r128RenderPrimitive( GLcontext *ctx, GLenum prim );


/***********************************************************************
 *                    Emit primitives as inline vertices               *
 ***********************************************************************/
	
#define HAVE_QUADS 0
#define HAVE_LINES 1
#define HAVE_POINTS 1
#define HAVE_LE32_VERTS 1
#define CTX_ARG r128ContextPtr rmesa
#define GET_VERTEX_DWORDS() rmesa->vertex_size
#define ALLOC_VERTS( n, size ) r128AllocDmaLow( rmesa, (n), (size) * 4 )
#undef LOCAL_VARS
#define LOCAL_VARS						\
   r128ContextPtr rmesa = R128_CONTEXT(ctx);			\
   const char *vertptr = rmesa->verts;
#define VERT(x) (r128Vertex *)(vertptr + ((x) * vertsize * 4))
#define VERTEX r128Vertex
#undef TAG
#define TAG(x) r128_##x
#include "tnl_dd/t_dd_triemit.h"
#undef TAG
#undef LOCAL_VARS


/***********************************************************************
 *          Macros for t_dd_tritmp.h to draw basic primitives          *
 ***********************************************************************/

#define TRI( a, b, c )				\
do {						\
   if (DO_FALLBACK)				\
      rmesa->draw_tri( rmesa, a, b, c );	\
   else						\
      r128_triangle( rmesa, a, b, c );		\
} while (0)

#define QUAD( a, b, c, d )			\
do {						\
   if (DO_FALLBACK) {				\
      rmesa->draw_tri( rmesa, a, b, d );	\
      rmesa->draw_tri( rmesa, b, c, d );	\
   } else 					\
      r128_quad( rmesa, a, b, c, d );		\
} while (0)

#define LINE( v0, v1 )				\
do {						\
   if (DO_FALLBACK)				\
      rmesa->draw_line( rmesa, v0, v1 );	\
   else 					\
      r128_line( rmesa, v0, v1 );		\
} while (0)

#define POINT( v0 )				\
do {						\
   if (DO_FALLBACK)				\
      rmesa->draw_point( rmesa, v0 );		\
   else 					\
      r128_point( rmesa, v0 );			\
} while (0)


/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/

#define R128_OFFSET_BIT	0x01
#define R128_TWOSIDE_BIT	0x02
#define R128_UNFILLED_BIT	0x04
#define R128_FALLBACK_BIT	0x08
#define R128_MAX_TRIFUNC	0x10


static struct {
   tnl_points_func	        points;
   tnl_line_func		line;
   tnl_triangle_func	triangle;
   tnl_quad_func		quad;
} rast_tab[R128_MAX_TRIFUNC];


#define DO_FALLBACK (IND & R128_FALLBACK_BIT)
#define DO_OFFSET   (IND & R128_OFFSET_BIT)
#define DO_UNFILLED (IND & R128_UNFILLED_BIT)
#define DO_TWOSIDE  (IND & R128_TWOSIDE_BIT)
#define DO_FLAT      0
#define DO_TRI       1
#define DO_QUAD      1
#define DO_LINE      1
#define DO_POINTS    1
#define DO_FULL_QUAD 1

#define HAVE_RGBA   1
#define HAVE_SPEC   1
#define HAVE_BACK_COLORS  0
#define HAVE_HW_FLATSHADE 1
#define VERTEX r128Vertex
#define TAB rast_tab

#define DEPTH_SCALE rmesa->depth_scale
#define UNFILLED_TRI unfilled_tri
#define UNFILLED_QUAD unfilled_quad
#define VERT_X(_v) _v->v.x
#define VERT_Y(_v) _v->v.y
#define VERT_Z(_v) _v->v.z
#define AREA_IS_CCW( a ) (a > 0)
#define GET_VERTEX(e) (rmesa->verts + (e * rmesa->vertex_size * sizeof(int)))

#define VERT_SET_RGBA( v, c )  					\
do {								\
   r128_color_t *color = (r128_color_t *)&((v)->ui[coloroffset]);	\
   UNCLAMPED_FLOAT_TO_UBYTE(color->red, (c)[0]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->green, (c)[1]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->blue, (c)[2]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->alpha, (c)[3]);		\
} while (0)

#define VERT_COPY_RGBA( v0, v1 ) v0->ui[coloroffset] = v1->ui[coloroffset]

#define VERT_SET_SPEC( v0, c )					\
do {								\
   if (havespec) {						\
      r128_color_t *spec = (r128_color_t *)&((v0)->ui[specoffset]); \
      UNCLAMPED_FLOAT_TO_UBYTE(spec->red, (c)[0]);		\
      UNCLAMPED_FLOAT_TO_UBYTE(spec->green, (c)[1]);		\
      UNCLAMPED_FLOAT_TO_UBYTE(spec->blue, (c)[2]);		\
   }								\
} while (0)
#define VERT_COPY_SPEC( v0, v1 )			\
do {							\
   if (havespec) {					\
      r128_color_t *spec0 = (r128_color_t *)&((v0)->ui[specoffset]); \
      r128_color_t *spec1 = (r128_color_t *)&((v1)->ui[specoffset]); \
      spec0->red   = spec1->red;			\
      spec0->green = spec1->green;			\
      spec0->blue  = spec1->blue; 			\
   }							\
} while (0)

/* These don't need LE32_TO_CPU() as they are used to save and restore
 * colors which are already in the correct format.
 */
#define VERT_SAVE_RGBA( idx )    color[idx] = v[idx]->ui[coloroffset]
#define VERT_RESTORE_RGBA( idx ) v[idx]->ui[coloroffset] = color[idx]
#define VERT_SAVE_SPEC( idx )    if (havespec) spec[idx] = v[idx]->ui[specoffset]
#define VERT_RESTORE_SPEC( idx ) if (havespec) v[idx]->ui[specoffset] = spec[idx]


#define LOCAL_VARS(n)						\
   r128ContextPtr rmesa = R128_CONTEXT(ctx);			\
   GLuint color[n], spec[n];					\
   GLuint coloroffset = rmesa->coloroffset;			\
   GLuint specoffset = rmesa->specoffset;			\
   GLboolean havespec = (rmesa->specoffset != 0);		\
   (void) color; (void) spec; (void) specoffset;		\
   (void) coloroffset; (void) havespec;

/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/

#define RASTERIZE(x) if (rmesa->hw_primitive != hw_prim[x]) \
                        r128RasterPrimitive( ctx, hw_prim[x] )
#define RENDER_PRIMITIVE rmesa->render_primitive
#define IND R128_FALLBACK_BIT
#define TAG(x) x
#include "tnl_dd/t_dd_unfilled.h"
#undef IND


/***********************************************************************
 *                      Generate GL render functions                   *
 ***********************************************************************/


#define IND (0)
#define TAG(x) x
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_OFFSET_BIT)
#define TAG(x) x##_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_TWOSIDE_BIT|R128_OFFSET_BIT)
#define TAG(x) x##_twoside_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_OFFSET_BIT|R128_UNFILLED_BIT)
#define TAG(x) x##_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_TWOSIDE_BIT|R128_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_TWOSIDE_BIT|R128_OFFSET_BIT|R128_UNFILLED_BIT)
#define TAG(x) x##_twoside_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_FALLBACK_BIT)
#define TAG(x) x##_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_OFFSET_BIT|R128_FALLBACK_BIT)
#define TAG(x) x##_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_TWOSIDE_BIT|R128_FALLBACK_BIT)
#define TAG(x) x##_twoside_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_TWOSIDE_BIT|R128_OFFSET_BIT|R128_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_UNFILLED_BIT|R128_FALLBACK_BIT)
#define TAG(x) x##_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_OFFSET_BIT|R128_UNFILLED_BIT|R128_FALLBACK_BIT)
#define TAG(x) x##_offset_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_TWOSIDE_BIT|R128_UNFILLED_BIT|R128_FALLBACK_BIT)
#define TAG(x) x##_twoside_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (R128_TWOSIDE_BIT|R128_OFFSET_BIT|R128_UNFILLED_BIT| \
	     R128_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"


static void init_rast_tab( void )
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
r128_fallback_tri( r128ContextPtr rmesa,
		     r128Vertex *v0,
		     r128Vertex *v1,
		     r128Vertex *v2 )
{
   GLcontext *ctx = rmesa->glCtx;
   SWvertex v[3];
   _swsetup_Translate( ctx, v0, &v[0] );
   _swsetup_Translate( ctx, v1, &v[1] );
   _swsetup_Translate( ctx, v2, &v[2] );
   _swrast_Triangle( ctx, &v[0], &v[1], &v[2] );
}


static void
r128_fallback_line( r128ContextPtr rmesa,
		    r128Vertex *v0,
		    r128Vertex *v1 )
{
   GLcontext *ctx = rmesa->glCtx;
   SWvertex v[2];
   _swsetup_Translate( ctx, v0, &v[0] );
   _swsetup_Translate( ctx, v1, &v[1] );
   _swrast_Line( ctx, &v[0], &v[1] );
}


static void
r128_fallback_point( r128ContextPtr rmesa,
		     r128Vertex *v0 )
{
   GLcontext *ctx = rmesa->glCtx;
   SWvertex v[1];
   _swsetup_Translate( ctx, v0, &v[0] );
   _swrast_Point( ctx, &v[0] );
}



/**********************************************************************/
/*               Render unclipped begin/end objects                   */
/**********************************************************************/

#define RENDER_POINTS( start, count )		\
   for ( ; start < count ; start++)		\
      r128_point( rmesa, VERT(start) )
#define RENDER_LINE( v0, v1 ) \
   r128_line( rmesa, VERT(v0), VERT(v1) )
#define RENDER_TRI( v0, v1, v2 )  \
   r128_triangle( rmesa, VERT(v0), VERT(v1), VERT(v2) )
#define RENDER_QUAD( v0, v1, v2, v3 ) \
   r128_quad( rmesa, VERT(v0), VERT(v1), VERT(v2), VERT(v3) )
#define INIT(x) do {					\
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);	\
   r128RenderPrimitive( ctx, x );			\
} while (0)
#undef LOCAL_VARS
#define LOCAL_VARS						\
    r128ContextPtr rmesa = R128_CONTEXT(ctx);		\
    const GLuint vertsize = rmesa->vertex_size;		\
    const char *vertptr = (char *)rmesa->verts;		\
    const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;	\
    (void) elt;
#define RESET_STIPPLE
#define RESET_OCCLUSION
#define PRESERVE_VB_DEFS
#define ELT(x) (x)
#define TAG(x) r128_##x##_verts
#include "tnl/t_vb_rendertmp.h"
#undef ELT
#undef TAG
#define TAG(x) r128_##x##_elts
#define ELT(x) elt[x]
#include "tnl/t_vb_rendertmp.h"


/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/

#define POINT_FALLBACK (DD_POINT_SMOOTH)
#define LINE_FALLBACK (DD_LINE_STIPPLE)
#define TRI_FALLBACK (DD_TRI_SMOOTH)
#define ANY_FALLBACK_FLAGS (POINT_FALLBACK|LINE_FALLBACK|TRI_FALLBACK)
#define ANY_RASTER_FLAGS (DD_TRI_LIGHT_TWOSIDE|DD_TRI_OFFSET|DD_TRI_UNFILLED)
#define _R128_NEW_RENDER_STATE (ANY_FALLBACK_FLAGS | ANY_RASTER_FLAGS)

static void r128ChooseRenderState(GLcontext *ctx)
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   GLuint flags = ctx->_TriangleCaps;
   GLuint index = 0;

   if (flags & (ANY_RASTER_FLAGS|ANY_FALLBACK_FLAGS)) {
      rmesa->draw_point = r128_point;
      rmesa->draw_line = r128_line;
      rmesa->draw_tri = r128_triangle;

      if (flags & ANY_RASTER_FLAGS) {
	 if (flags & DD_TRI_LIGHT_TWOSIDE) index |= R128_TWOSIDE_BIT;
	 if (flags & DD_TRI_OFFSET)        index |= R128_OFFSET_BIT;
	 if (flags & DD_TRI_UNFILLED)      index |= R128_UNFILLED_BIT;
      }

      /* Hook in fallbacks for specific primitives.
       */
      if (flags & (POINT_FALLBACK|LINE_FALLBACK|TRI_FALLBACK)) {
	 if (flags & POINT_FALLBACK) rmesa->draw_point = r128_fallback_point;
	 if (flags & LINE_FALLBACK)  rmesa->draw_line = r128_fallback_line;
	 if (flags & TRI_FALLBACK)   rmesa->draw_tri = r128_fallback_tri;
	 index |= R128_FALLBACK_BIT;
      }
   }

   if (index != rmesa->RenderIndex) {
      TNLcontext *tnl = TNL_CONTEXT(ctx);
      tnl->Driver.Render.Points = rast_tab[index].points;
      tnl->Driver.Render.Line = rast_tab[index].line;
      tnl->Driver.Render.ClippedLine = rast_tab[index].line;
      tnl->Driver.Render.Triangle = rast_tab[index].triangle;
      tnl->Driver.Render.Quad = rast_tab[index].quad;

      if (index == 0) {
	 tnl->Driver.Render.PrimTabVerts = r128_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = r128_render_tab_elts;
	 tnl->Driver.Render.ClippedPolygon = r128_fast_clipped_poly;
      } else {
	 tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
	 tnl->Driver.Render.ClippedPolygon = _tnl_RenderClippedPolygon;
      }

      rmesa->RenderIndex = index;
   }
}

/**********************************************************************/
/*                 Validate state at pipeline start                   */
/**********************************************************************/

static void r128RunPipeline( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   if (rmesa->new_state || rmesa->NewGLState & _NEW_TEXTURE)
      r128DDUpdateHWState( ctx );

   if (!rmesa->Fallback && rmesa->NewGLState) {
      if (rmesa->NewGLState & _R128_NEW_RENDER_STATE)
	 r128ChooseRenderState( ctx );

      rmesa->NewGLState = 0;
   }

   _tnl_run_pipeline( ctx );
}

/**********************************************************************/
/*                 High level hooks for t_vb_render.c                 */
/**********************************************************************/

/* This is called when Mesa switches between rendering triangle
 * primitives (such as GL_POLYGON, GL_QUADS, GL_TRIANGLE_STRIP, etc),
 * and lines, points and bitmaps.
 *
 * As the r128 uses triangles to render lines and points, it is
 * necessary to turn off hardware culling when rendering these
 * primitives.
 */

static void r128RasterPrimitive( GLcontext *ctx, GLuint hwprim )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   rmesa->setup.dp_gui_master_cntl_c &= ~R128_GMC_BRUSH_NONE;

   if ( ctx->Polygon.StippleFlag && hwprim == GL_TRIANGLES ) {
      rmesa->setup.dp_gui_master_cntl_c |= R128_GMC_BRUSH_32x32_MONO_FG_LA;
   }
   else {
      rmesa->setup.dp_gui_master_cntl_c |= R128_GMC_BRUSH_SOLID_COLOR;
   }

   rmesa->new_state |= R128_NEW_CONTEXT;
   rmesa->dirty |= R128_UPLOAD_CONTEXT;

   if (rmesa->hw_primitive != hwprim) {
      FLUSH_BATCH( rmesa );
      rmesa->hw_primitive = hwprim;
   }
}

static void r128SetupAntialias( GLcontext *ctx, GLenum prim )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   GLuint currAA, wantAA;
   
   currAA = (rmesa->setup.pm4_vc_fpu_setup & R128_EDGE_ANTIALIAS) != 0;
   if( prim >= GL_TRIANGLES )
      wantAA = ctx->Polygon.SmoothFlag;
   else if( prim >= GL_LINES )
      wantAA = ctx->Line.SmoothFlag;
   else
      wantAA = 0;
      
   if( wantAA != currAA )
   {
     FLUSH_BATCH( rmesa );
     rmesa->setup.pm4_vc_fpu_setup ^= R128_EDGE_ANTIALIAS;
     rmesa->dirty |= R128_UPLOAD_SETUP;
   }
}

static void r128RenderPrimitive( GLcontext *ctx, GLenum prim )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   GLuint hw = hw_prim[prim];
   rmesa->render_primitive = prim;

   r128SetupAntialias( ctx, prim );
   
   if (prim >= GL_TRIANGLES && (ctx->_TriangleCaps & DD_TRI_UNFILLED))
      return;
   r128RasterPrimitive( ctx, hw );
}

#define EMIT_ATTR( ATTR, STYLE, VF, SIZE )				\
do {									\
   rmesa->vertex_attrs[rmesa->vertex_attr_count].attrib = (ATTR);	\
   rmesa->vertex_attrs[rmesa->vertex_attr_count].format = (STYLE);	\
   rmesa->vertex_attr_count++;						\
   vc_frmt |= (VF);							\
   offset += (SIZE);							\
} while (0)

#define EMIT_PAD( SIZE )						\
do {									\
   rmesa->vertex_attrs[rmesa->vertex_attr_count].attrib = 0;		\
   rmesa->vertex_attrs[rmesa->vertex_attr_count].format = EMIT_PAD;	\
   rmesa->vertex_attrs[rmesa->vertex_attr_count].offset = (SIZE);	\
   rmesa->vertex_attr_count++;						\
   offset += (SIZE);							\
} while (0)

static void r128RenderStart( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   DECLARE_RENDERINPUTS(index_bitset);
   GLuint vc_frmt = 0;
   GLboolean fallback_projtex = GL_FALSE;
   GLuint offset = 0;

   RENDERINPUTS_COPY( index_bitset, tnl->render_inputs_bitset );

   /* Important: */
   VB->AttribPtr[VERT_ATTRIB_POS] = VB->NdcPtr;
   rmesa->vertex_attr_count = 0;
   rmesa->specoffset = 0;

   /* EMIT_ATTR's must be in order as they tell t_vertex.c how to
    * build up a hardware vertex.
    */
   if (RENDERINPUTS_TEST_RANGE( index_bitset, _TNL_FIRST_TEX, _TNL_LAST_TEX ))
      EMIT_ATTR( _TNL_ATTRIB_POS, EMIT_4F_VIEWPORT, R128_CCE_VC_FRMT_RHW, 4 );
   else
      EMIT_ATTR( _TNL_ATTRIB_POS, EMIT_3F_VIEWPORT, 0, 3 );

   rmesa->coloroffset = offset;
#if MESA_LITTLE_ENDIAN 
   EMIT_ATTR( _TNL_ATTRIB_COLOR0, EMIT_4UB_4F_BGRA,
      R128_CCE_VC_FRMT_DIFFUSE_ARGB, 4 );
#else
   EMIT_ATTR( _TNL_ATTRIB_COLOR0, EMIT_4UB_4F_ARGB,
      R128_CCE_VC_FRMT_DIFFUSE_ARGB, 4 );
#endif

   if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_COLOR1 ) ||
       RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_FOG )) {
#if MESA_LITTLE_ENDIAN
      if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_COLOR1 )) {
	 rmesa->specoffset = offset;
	 EMIT_ATTR( _TNL_ATTRIB_COLOR1, EMIT_3UB_3F_BGR,
	    R128_CCE_VC_FRMT_SPEC_FRGB, 3 );
      } else 
	 EMIT_PAD( 3 );

      if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_FOG ))
	 EMIT_ATTR( _TNL_ATTRIB_FOG, EMIT_1UB_1F, R128_CCE_VC_FRMT_SPEC_FRGB,
		    1 );
      else
	 EMIT_PAD( 1 );
#else
      if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_FOG ))
	 EMIT_ATTR( _TNL_ATTRIB_FOG, EMIT_1UB_1F, R128_CCE_VC_FRMT_SPEC_FRGB,
		    1 );
      else
	 EMIT_PAD( 1 );

      if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_COLOR1 )) {
	 rmesa->specoffset = offset;
	 EMIT_ATTR( _TNL_ATTRIB_COLOR1, EMIT_3UB_3F_RGB,
	    R128_CCE_VC_FRMT_SPEC_FRGB, 3 );
      } else 
	 EMIT_PAD( 3 );
#endif
   }

   if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_TEX(rmesa->tmu_source[0]) )) {
      if ( VB->TexCoordPtr[rmesa->tmu_source[0]]->size > 2 )
	 fallback_projtex = GL_TRUE;
      EMIT_ATTR( _TNL_ATTRIB_TEX0, EMIT_2F, R128_CCE_VC_FRMT_S_T, 8 );
   }
   if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_TEX(rmesa->tmu_source[1]) )) {
      if ( VB->TexCoordPtr[rmesa->tmu_source[1]]->size > 2 )
	 fallback_projtex = GL_TRUE;
      EMIT_ATTR( _TNL_ATTRIB_TEX1, EMIT_2F, R128_CCE_VC_FRMT_S2_T2, 8 );
   }

   /* projective textures are not supported by the hardware */
   FALLBACK( rmesa, R128_FALLBACK_PROJTEX, fallback_projtex );

   /* Only need to change the vertex emit code if there has been a
    * statechange to a TNL index.
    */
   if (!RENDERINPUTS_EQUAL( index_bitset, rmesa->tnl_state_bitset )) {
      FLUSH_BATCH( rmesa );
      rmesa->dirty |= R128_UPLOAD_CONTEXT;

      rmesa->vertex_size = 
	 _tnl_install_attrs( ctx, 
			     rmesa->vertex_attrs, 
			     rmesa->vertex_attr_count,
			     rmesa->hw_viewport, 0 );
      rmesa->vertex_size >>= 2;

      rmesa->vertex_format = vc_frmt;
   }
}

static void r128RenderFinish( GLcontext *ctx )
{
   if (R128_CONTEXT(ctx)->RenderIndex & R128_FALLBACK_BIT)
      _swrast_flush( ctx );
}


/**********************************************************************/
/*           Transition to/from hardware rasterization.               */
/**********************************************************************/

static const char * const fallbackStrings[] = {
   "Texture mode",
   "glDrawBuffer(GL_FRONT_AND_BACK)",
   "glReadBuffer",
   "glEnable(GL_STENCIL) without hw stencil buffer",
   "glRenderMode(selection or feedback)",
   "glLogicOp (mode != GL_COPY)",
   "GL_SEPARATE_SPECULAR_COLOR",
   "glBlendEquation(mode != ADD)",
   "glBlendFunc",
   "Projective texture",
   "Rasterization disable",
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

void r128Fallback( GLcontext *ctx, GLuint bit, GLboolean mode )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   GLuint oldfallback = rmesa->Fallback;

   if (mode) {
      rmesa->Fallback |= bit;
      if (oldfallback == 0) {
	 FLUSH_BATCH( rmesa );
	 _swsetup_Wakeup( ctx );
	 rmesa->RenderIndex = ~0;
	 if ( R128_DEBUG & DEBUG_VERBOSE_FALL ) {
	     fprintf(stderr, "R128 begin rasterization fallback: 0x%x %s\n",
		     bit, getFallbackString(bit));
	 }
      }
   }
   else {
      rmesa->Fallback &= ~bit;
      if (oldfallback == bit) {
	 _swrast_flush( ctx );
	 tnl->Driver.Render.Start = r128RenderStart;
	 tnl->Driver.Render.PrimitiveNotify = r128RenderPrimitive;
	 tnl->Driver.Render.Finish = r128RenderFinish;

	 tnl->Driver.Render.BuildVertices = _tnl_build_vertices;
	 tnl->Driver.Render.CopyPV = _tnl_copy_pv;
	 tnl->Driver.Render.Interp = _tnl_interp;

	 _tnl_invalidate_vertex_state( ctx, ~0 );
	 _tnl_invalidate_vertices( ctx, ~0 );
	 _tnl_install_attrs( ctx, 
			     rmesa->vertex_attrs, 
			     rmesa->vertex_attr_count,
			     rmesa->hw_viewport, 0 ); 

	 rmesa->NewGLState |= _R128_NEW_RENDER_STATE;
	 if ( R128_DEBUG & DEBUG_VERBOSE_FALL ) {
	     fprintf(stderr, "R128 end rasterization fallback: 0x%x %s\n",
		     bit, getFallbackString(bit));
	 }
      }
   }
}


/**********************************************************************/
/*                            Initialization.                         */
/**********************************************************************/

void r128InitTriFuncs( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   static int firsttime = 1;

   if (firsttime) {
      init_rast_tab();
      firsttime = 0;
   }

   tnl->Driver.RunPipeline = r128RunPipeline;
   tnl->Driver.Render.Start = r128RenderStart;
   tnl->Driver.Render.Finish = r128RenderFinish;
   tnl->Driver.Render.PrimitiveNotify = r128RenderPrimitive;
   tnl->Driver.Render.ResetLineStipple = _swrast_ResetLineStipple;
   tnl->Driver.Render.BuildVertices = _tnl_build_vertices;
   tnl->Driver.Render.CopyPV = _tnl_copy_pv;
   tnl->Driver.Render.Interp = _tnl_interp;

   _tnl_init_vertices( ctx, ctx->Const.MaxArrayLockSize + 12, 
		       (6 + 2 * ctx->Const.MaxTextureUnits) * sizeof(GLfloat) );
   rmesa->verts = (char *)tnl->clipspace.vertex_buf;
   RENDERINPUTS_ONES( rmesa->tnl_state_bitset );

   rmesa->NewGLState |= _R128_NEW_RENDER_STATE;
}
