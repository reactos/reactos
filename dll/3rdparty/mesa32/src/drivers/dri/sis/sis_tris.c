/* $XFree86*/ /* -*- c-basic-offset: 3 -*- */
/**************************************************************************

Copyright 2000 Silicon Integrated Systems Corp, Inc., HsinChu, Taiwan.
Copyright 2003 Eric Anholt
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
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Sung-Ching Lin <sclin@sis.com.tw>
 *   Eric Anholt <anholt@FreeBSD.org>
 */

#include "glheader.h"
#include "mtypes.h"
#include "colormac.h"
#include "macros.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#include "sis_context.h"
#include "sis_tris.h"
#include "sis_state.h"
#include "sis_lock.h"
#include "sis_span.h"
#include "sis_alloc.h"
#include "sis_tex.h"

/* 6326 and 300-series shared */
static const GLuint hw_prim[GL_POLYGON+1] = {
   OP_3D_POINT_DRAW,		/* GL_POINTS */
   OP_3D_LINE_DRAW,		/* GL_LINES */
   OP_3D_LINE_DRAW,		/* GL_LINE_LOOP */
   OP_3D_LINE_DRAW,		/* GL_LINE_STRIP */
   OP_3D_TRIANGLE_DRAW,		/* GL_TRIANGLES */
   OP_3D_TRIANGLE_DRAW,		/* GL_TRIANGLE_STRIP */
   OP_3D_TRIANGLE_DRAW,		/* GL_TRIANGLE_FAN */
   OP_3D_TRIANGLE_DRAW,		/* GL_QUADS */
   OP_3D_TRIANGLE_DRAW,		/* GL_QUAD_STRIP */
   OP_3D_TRIANGLE_DRAW		/* GL_POLYGON */
};

static const GLuint hw_prim_mmio_fire[OP_3D_TRIANGLE_DRAW+1] = {
   OP_3D_FIRE_TSARGBa,
   OP_3D_FIRE_TSARGBb,
   OP_3D_FIRE_TSARGBc
};
static const GLuint hw_prim_6326_mmio_fire[OP_3D_TRIANGLE_DRAW+1] = {
   OP_6326_3D_FIRE_TSARGBa,
   OP_6326_3D_FIRE_TSARGBb,
   OP_6326_3D_FIRE_TSARGBc
};

static const GLuint hw_prim_mmio_shade[OP_3D_TRIANGLE_DRAW+1] = {
   SHADE_FLAT_VertexA,
   SHADE_FLAT_VertexB,
   SHADE_FLAT_VertexC
};

static const GLuint hw_prim_agp_type[OP_3D_TRIANGLE_DRAW+1] = {
   MASK_PsPointList,
   MASK_PsLineList,
   MASK_PsTriangleList
};

static const GLuint hw_prim_agp_shade[OP_3D_TRIANGLE_DRAW+1] = {
   MASK_PsShadingFlatA,
   MASK_PsShadingFlatB,
   MASK_PsShadingFlatC
};

static void sisRasterPrimitive( GLcontext *ctx, GLuint hwprim );
static void sisRenderPrimitive( GLcontext *ctx, GLenum prim );

/***********************************************************************
 *                    Emit primitives as inline vertices               *
 ***********************************************************************/

#define HAVE_QUADS 0
#define HAVE_LINES 1
#define HAVE_POINTS 1
#define CTX_ARG sisContextPtr smesa
#define GET_VERTEX_DWORDS() smesa->vertex_size
#define ALLOC_VERTS( n, size ) sisAllocDmaLow( smesa, n * size * sizeof(int) )
#undef LOCAL_VARS
#define LOCAL_VARS						\
   sisContextPtr smesa = SIS_CONTEXT(ctx);			\
   const char *vertptr = smesa->verts;
#define VERT(x) (sisVertex *)(vertptr + (x * vertsize * sizeof(int)))
#define VERTEX sisVertex 
#undef TAG
#define TAG(x) sis_##x
#include "tnl_dd/t_dd_triemit.h"
#undef TAG
#undef LOCAL_VARS

/***********************************************************************
 *             Dispatch vertices to hardware through MMIO              *
 ***********************************************************************/

/* The ARGB write of the last vertex of the primitive fires the 3d engine, so
 * save it until the end.
 */
#define SIS_MMIO_WRITE_VERTEX(_v, i, lastvert)			\
do {								\
   GLuint __color, __i = 0;					\
   MMIO(REG_3D_TSXa+(i)*0x30, _v->ui[__i++]);			\
   MMIO(REG_3D_TSYa+(i)*0x30, _v->ui[__i++]);			\
   MMIO(REG_3D_TSZa+(i)*0x30, _v->ui[__i++]);			\
   if (SIS_STATES & VERT_W)					\
      MMIO(REG_3D_TSWGa+(i)*0x30, _v->ui[__i++]);		\
   __color = _v->ui[__i++];					\
   if (SIS_STATES & VERT_SPEC)					\
      MMIO(REG_3D_TSFSa+(i)*0x30, _v->ui[__i++]);		\
   if (SIS_STATES & VERT_UV0) {					\
      MMIO(REG_3D_TSUAa+(i)*0x30, _v->ui[__i++]);		\
      MMIO(REG_3D_TSVAa+(i)*0x30, _v->ui[__i++]);		\
   }								\
   if (SIS_STATES & VERT_UV1) {					\
      MMIO(REG_3D_TSUBa+(i)*0x30, _v->ui[__i++]);		\
      MMIO(REG_3D_TSVBa+(i)*0x30, _v->ui[__i++]);		\
   }								\
   if (lastvert || (SIS_STATES & VERT_SMOOTH))			\
      MMIO(REG_3D_TSARGBa+(i)*0x30, __color);			\
} while (0)

#define SIS6326_MMIO_WRITE_VERTEX(_v, i, lastvert)		\
do {								\
   GLuint __color, __i = 0;					\
   MMIO(REG_6326_3D_TSXa+(i)*0x20, _v->ui[__i++]);		\
   MMIO(REG_6326_3D_TSYa+(i)*0x20, _v->ui[__i++]);		\
   MMIO(REG_6326_3D_TSZa+(i)*0x20, _v->ui[__i++]);		\
   if (SIS_STATES & VERT_W)					\
      MMIO(REG_6326_3D_TSWa+(i)*0x20, _v->ui[__i++]);		\
   __color = _v->ui[__i++];					\
   if (SIS_STATES & VERT_SPEC)					\
      MMIO(REG_6326_3D_TSFSa+(i)*0x20, _v->ui[__i++]);		\
   if (SIS_STATES & VERT_UV0) {					\
      MMIO(REG_6326_3D_TSUa+(i)*0x20, _v->ui[__i++]);		\
      MMIO(REG_6326_3D_TSVa+(i)*0x20, _v->ui[__i++]);		\
   }								\
   if (lastvert || (SIS_STATES & VERT_SMOOTH))			\
      MMIO(REG_6326_3D_TSARGBa+(i)*0x30, __color);		\
} while (0)

#define MMIO_VERT_REG_COUNT 10

#define VERT_SMOOTH	0x01
#define VERT_W		0x02
#define VERT_SPEC	0x04
#define VERT_UV0	0x08
#define VERT_UV1	0x10
#define VERT_6326	0x20	/* Right after UV1, but won't have a UV1 set */

typedef void (*mmio_draw_func)(sisContextPtr smesa, char *verts);
static mmio_draw_func sis_tri_func_mmio[48];
static mmio_draw_func sis_line_func_mmio[48];
static mmio_draw_func sis_point_func_mmio[48];

#define SIS_STATES (0)
#define TAG(x) x##_none
#include "sis_tritmp.h"

#define SIS_STATES (VERT_SMOOTH)
#define TAG(x) x##_g
#include "sis_tritmp.h"

#define SIS_STATES (VERT_W)
#define TAG(x) x##_w
#include "sis_tritmp.h"

#define SIS_STATES (VERT_SMOOTH | VERT_W)
#define TAG(x) x##_gw
#include "sis_tritmp.h"

#define SIS_STATES (VERT_SPEC)
#define TAG(x) x##_s
#include "sis_tritmp.h"

#define SIS_STATES (VERT_SMOOTH | VERT_SPEC)
#define TAG(x) x##_gs
#include "sis_tritmp.h"

#define SIS_STATES (VERT_W | VERT_SPEC)
#define TAG(x) x##_ws
#include "sis_tritmp.h"

#define SIS_STATES (VERT_SMOOTH | VERT_W | VERT_SPEC)
#define TAG(x) x##_gws
#include "sis_tritmp.h"

#define SIS_STATES (VERT_UV0)
#define TAG(x) x##_t0
#include "sis_tritmp.h"

#define SIS_STATES (VERT_SMOOTH | VERT_UV0)
#define TAG(x) x##_gt0
#include "sis_tritmp.h"

#define SIS_STATES (VERT_W | VERT_UV0)
#define TAG(x) x##_wt0
#include "sis_tritmp.h"

#define SIS_STATES (VERT_SMOOTH | VERT_W | VERT_UV0)
#define TAG(x) x##_gwt0
#include "sis_tritmp.h"

#define SIS_STATES (VERT_SPEC | VERT_UV0)
#define TAG(x) x##_st0
#include "sis_tritmp.h"

#define SIS_STATES (VERT_SMOOTH | VERT_SPEC | VERT_UV0)
#define TAG(x) x##_gst0
#include "sis_tritmp.h"

#define SIS_STATES (VERT_W | VERT_SPEC | VERT_UV0)
#define TAG(x) x##_wst0
#include "sis_tritmp.h"

#define SIS_STATES (VERT_SMOOTH | VERT_W | VERT_SPEC | VERT_UV0)
#define TAG(x) x##_gwst0
#include "sis_tritmp.h"

#define SIS_STATES (VERT_UV1)
#define TAG(x) x##_t1
#include "sis_tritmp.h"

#define SIS_STATES (VERT_SMOOTH | VERT_UV1)
#define TAG(x) x##_gt1
#include "sis_tritmp.h"

#define SIS_STATES (VERT_W | VERT_UV1)
#define TAG(x) x##_wt1
#include "sis_tritmp.h"

#define SIS_STATES (VERT_SMOOTH | VERT_W | VERT_UV1)
#define TAG(x) x##_gwt1
#include "sis_tritmp.h"

#define SIS_STATES (VERT_SPEC | VERT_UV1)
#define TAG(x) x##_st1
#include "sis_tritmp.h"

#define SIS_STATES (VERT_SMOOTH | VERT_SPEC | VERT_UV1)
#define TAG(x) x##_gst1
#include "sis_tritmp.h"

#define SIS_STATES (VERT_W | VERT_SPEC | VERT_UV1)
#define TAG(x) x##_wst1
#include "sis_tritmp.h"

#define SIS_STATES (VERT_SMOOTH | VERT_W | VERT_SPEC | VERT_UV1)
#define TAG(x) x##_gwst1
#include "sis_tritmp.h"

#define SIS_STATES (VERT_UV0 | VERT_UV1)
#define TAG(x) x##_t0t1
#include "sis_tritmp.h"

#define SIS_STATES (VERT_SMOOTH | VERT_UV0 | VERT_UV1)
#define TAG(x) x##_gt0t1
#include "sis_tritmp.h"

#define SIS_STATES (VERT_W | VERT_UV0 | VERT_UV1)
#define TAG(x) x##_wt0t1
#include "sis_tritmp.h"

#define SIS_STATES (VERT_SMOOTH | VERT_W | VERT_UV0 | VERT_UV1)
#define TAG(x) x##_gwt0t1
#include "sis_tritmp.h"

#define SIS_STATES (VERT_SPEC | VERT_UV0 | VERT_UV1)
#define TAG(x) x##_st0t1
#include "sis_tritmp.h"

#define SIS_STATES (VERT_SMOOTH | VERT_SPEC | VERT_UV0 | VERT_UV1)
#define TAG(x) x##_gst0t1
#include "sis_tritmp.h"

#define SIS_STATES (VERT_W | VERT_SPEC | VERT_UV0 | VERT_UV1)
#define TAG(x) x##_wst0t1
#include "sis_tritmp.h"

#define SIS_STATES (VERT_SMOOTH | VERT_W | VERT_SPEC | VERT_UV0 | VERT_UV1)
#define TAG(x) x##_gwst0t1
#include "sis_tritmp.h"

/***********************************************************************
 *          Macros for t_dd_tritmp.h to draw basic primitives          *
 ***********************************************************************/

#define TRI( a, b, c )				\
do { 						\
   if (DO_FALLBACK)				\
      smesa->draw_tri( smesa, a, b, c );	\
   else						\
      sis_triangle( smesa, a, b, c );		\
} while (0)

#define QUAD( a, b, c, d )			\
do { 						\
   if (DO_FALLBACK) {				\
      smesa->draw_tri( smesa, a, b, d );	\
      smesa->draw_tri( smesa, b, c, d );	\
   } else					\
      sis_quad( smesa, a, b, c, d );		\
} while (0)

#define LINE( v0, v1 )				\
do { 						\
   if (DO_FALLBACK)				\
      smesa->draw_line( smesa, v0, v1 );	\
   else						\
      sis_line( smesa, v0, v1 );		\
} while (0)

#define POINT( v0 )				\
do { 						\
   if (DO_FALLBACK)				\
      smesa->draw_point( smesa, v0 );		\
   else						\
      sis_point( smesa, v0 );			\
} while (0)

/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/

#define SIS_OFFSET_BIT 		0x01
#define SIS_TWOSIDE_BIT		0x02
#define SIS_UNFILLED_BIT	0x04
#define SIS_FALLBACK_BIT	0x08
#define SIS_MAX_TRIFUNC		0x10


static struct {
   tnl_points_func	        points;
   tnl_line_func		line;
   tnl_triangle_func	triangle;
   tnl_quad_func		quad;
} rast_tab[SIS_MAX_TRIFUNC];


#define DO_FALLBACK (IND & SIS_FALLBACK_BIT)
#define DO_OFFSET   (IND & SIS_OFFSET_BIT)
#define DO_UNFILLED (IND & SIS_UNFILLED_BIT)
#define DO_TWOSIDE  (IND & SIS_TWOSIDE_BIT)
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
#define VERTEX sisVertex
#define TAB rast_tab

#define DEPTH_SCALE smesa->depth_scale
#define UNFILLED_TRI unfilled_tri
#define UNFILLED_QUAD unfilled_quad
#define VERT_X(_v) _v->v.x
#define VERT_Y(_v) _v->v.y
#define VERT_Z(_v) _v->v.z
#define AREA_IS_CCW( a ) (a > 0)
#define GET_VERTEX(e) (smesa->verts + (e * smesa->vertex_size * sizeof(int)))

#define VERT_SET_RGBA( v, c )  					\
do {								\
   sis_color_t *color = (sis_color_t *)&((v)->ui[coloroffset]);	\
   UNCLAMPED_FLOAT_TO_UBYTE(color->red, (c)[0]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->green, (c)[1]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->blue, (c)[2]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->alpha, (c)[3]);		\
} while (0)

#define VERT_COPY_RGBA( v0, v1 ) v0->ui[coloroffset] = v1->ui[coloroffset]

#define VERT_SET_SPEC( v, c )					\
do {								\
   if (specoffset != 0) {					\
      sis_color_t *spec = (sis_color_t *)&((v)->ui[specoffset]); \
      UNCLAMPED_FLOAT_TO_UBYTE(spec->red, (c)[0]);		\
      UNCLAMPED_FLOAT_TO_UBYTE(spec->green, (c)[1]);		\
      UNCLAMPED_FLOAT_TO_UBYTE(spec->blue, (c)[2]);		\
   }								\
} while (0)
#define VERT_COPY_SPEC( v0, v1 )				\
do {								\
   if (specoffset != 0) {					\
      sis_color_t *spec0 = (sis_color_t *)&((v0)->ui[specoffset]); \
      sis_color_t *spec1 = (sis_color_t *)&((v1)->ui[specoffset]); \
      spec0->red   = spec1->red;				\
      spec0->green = spec1->green;				\
      spec0->blue  = spec1->blue; 				\
   }								\
} while (0)

#define VERT_SAVE_RGBA( idx )    color[idx] = v[idx]->ui[coloroffset]
#define VERT_RESTORE_RGBA( idx ) v[idx]->ui[coloroffset] = color[idx]
#define VERT_SAVE_SPEC( idx )    if (specoffset != 0) spec[idx] = v[idx]->ui[specoffset]
#define VERT_RESTORE_SPEC( idx ) if (specoffset != 0) v[idx]->ui[specoffset] = spec[idx]

#define LOCAL_VARS(n)						\
   sisContextPtr smesa = SIS_CONTEXT(ctx);			\
   GLuint color[n], spec[n];					\
   GLuint coloroffset = smesa->coloroffset;			\
   GLuint specoffset = smesa->specoffset;			\
   (void) color; (void) spec; (void) coloroffset; (void) specoffset;

/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/

#define RASTERIZE(x) if (smesa->hw_primitive != hw_prim[x]) \
                        sisRasterPrimitive( ctx, hw_prim[x] )
#define RENDER_PRIMITIVE smesa->render_primitive
#define IND SIS_FALLBACK_BIT
#define TAG(x) x
#include "tnl_dd/t_dd_unfilled.h"
#undef IND


/***********************************************************************
 *                      Generate GL render functions                   *
 ***********************************************************************/


#define IND (0)
#define TAG(x) x
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_OFFSET_BIT)
#define TAG(x) x##_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_TWOSIDE_BIT|SIS_OFFSET_BIT)
#define TAG(x) x##_twoside_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_OFFSET_BIT|SIS_UNFILLED_BIT)
#define TAG(x) x##_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_TWOSIDE_BIT|SIS_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_TWOSIDE_BIT|SIS_OFFSET_BIT|SIS_UNFILLED_BIT)
#define TAG(x) x##_twoside_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_FALLBACK_BIT)
#define TAG(x) x##_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_OFFSET_BIT|SIS_FALLBACK_BIT)
#define TAG(x) x##_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_TWOSIDE_BIT|SIS_FALLBACK_BIT)
#define TAG(x) x##_twoside_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_TWOSIDE_BIT|SIS_OFFSET_BIT|SIS_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_UNFILLED_BIT|SIS_FALLBACK_BIT)
#define TAG(x) x##_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_OFFSET_BIT|SIS_UNFILLED_BIT|SIS_FALLBACK_BIT)
#define TAG(x) x##_offset_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_TWOSIDE_BIT|SIS_UNFILLED_BIT|SIS_FALLBACK_BIT)
#define TAG(x) x##_twoside_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (SIS_TWOSIDE_BIT|SIS_OFFSET_BIT|SIS_UNFILLED_BIT| \
	     SIS_FALLBACK_BIT)
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
sis_fallback_tri( sisContextPtr smesa,
		  sisVertex *v0,
		  sisVertex *v1,
		  sisVertex *v2 )
{
   GLcontext *ctx = smesa->glCtx;
   SWvertex v[3];
   _swsetup_Translate( ctx, v0, &v[0] );
   _swsetup_Translate( ctx, v1, &v[1] );
   _swsetup_Translate( ctx, v2, &v[2] );
   sisSpanRenderStart( ctx );
   _swrast_Triangle( ctx, &v[0], &v[1], &v[2] );
   sisSpanRenderFinish( ctx );
   _swrast_flush( ctx );
}


static void
sis_fallback_line( sisContextPtr smesa,
		   sisVertex *v0,
		   sisVertex *v1 )
{
   GLcontext *ctx = smesa->glCtx;
   SWvertex v[2];
   _swsetup_Translate( ctx, v0, &v[0] );
   _swsetup_Translate( ctx, v1, &v[1] );
   sisSpanRenderStart( ctx );
   _swrast_Line( ctx, &v[0], &v[1] );
   sisSpanRenderFinish( ctx );
   _swrast_flush( ctx );
}


static void
sis_fallback_point( sisContextPtr smesa,
		    sisVertex *v0 )
{
   GLcontext *ctx = smesa->glCtx;
   SWvertex v[1];
   _swsetup_Translate( ctx, v0, &v[0] );
   sisSpanRenderStart( ctx );
   _swrast_Point( ctx, &v[0] );
   sisSpanRenderFinish( ctx );
   _swrast_flush( ctx );
}



/**********************************************************************/
/*               Render unclipped begin/end objects                   */
/**********************************************************************/

#define IND 0
#define V(x) (sisVertex *)(vertptr + (x * vertsize * sizeof(int)))
#define RENDER_POINTS( start, count )		\
   for ( ; start < count ; start++)		\
      POINT( V(ELT(start)) )
#define RENDER_LINE( v0, v1 )         LINE( V(v0), V(v1) )
#define RENDER_TRI(  v0, v1, v2 )     TRI(  V(v0), V(v1), V(v2) )
#define RENDER_QUAD( v0, v1, v2, v3 ) QUAD( V(v0), V(v1), V(v2), V(v3) )
#define INIT(x) sisRenderPrimitive( ctx, x )
#undef LOCAL_VARS
#define LOCAL_VARS				\
    sisContextPtr smesa = SIS_CONTEXT(ctx);	\
    const GLuint vertsize = smesa->vertex_size;		\
    const char *vertptr = (char *)smesa->verts;		\
    const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;	\
    (void) elt;
#define RESET_STIPPLE
#define RESET_OCCLUSION
#define PRESERVE_VB_DEFS
#define ELT(x) (x)
#define TAG(x) sis_##x##_verts
#include "tnl/t_vb_rendertmp.h"
#undef ELT
#undef TAG
#define TAG(x) sis_##x##_elts
#define ELT(x) elt[x]
#include "tnl/t_vb_rendertmp.h"


/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/

#define POINT_FALLBACK (DD_POINT_SMOOTH)
#define LINE_FALLBACK (DD_LINE_STIPPLE|DD_LINE_SMOOTH)
#define TRI_FALLBACK (DD_TRI_STIPPLE|DD_TRI_SMOOTH)
#define ANY_FALLBACK_FLAGS (POINT_FALLBACK|LINE_FALLBACK|TRI_FALLBACK)
#define ANY_RASTER_FLAGS (DD_TRI_LIGHT_TWOSIDE|DD_TRI_OFFSET|DD_TRI_UNFILLED)
#define _SIS_NEW_RENDER_STATE (ANY_RASTER_FLAGS | ANY_FALLBACK_FLAGS)

static void sisChooseRenderState(GLcontext *ctx)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   sisContextPtr smesa = SIS_CONTEXT( ctx );
   GLuint flags = ctx->_TriangleCaps;
   GLuint index = 0;

   if (smesa->Fallback)
      return;

   if (flags & (ANY_RASTER_FLAGS|ANY_FALLBACK_FLAGS)) {

      if (flags & ANY_RASTER_FLAGS) {
	 if (flags & DD_TRI_LIGHT_TWOSIDE) index |= SIS_TWOSIDE_BIT;
	 if (flags & DD_TRI_OFFSET)        index |= SIS_OFFSET_BIT;
	 if (flags & DD_TRI_UNFILLED)      index |= SIS_UNFILLED_BIT;
      }

      smesa->draw_point = sis_point;
      smesa->draw_line = sis_line;
      smesa->draw_tri = sis_triangle;
      /* Hook in fallbacks for specific primitives.
       */
      if (flags & ANY_FALLBACK_FLAGS) {
	 if (flags & POINT_FALLBACK)
            smesa->draw_point = sis_fallback_point;
	 if (flags & LINE_FALLBACK)
            smesa->draw_line = sis_fallback_line;
	 if (flags & TRI_FALLBACK)
            smesa->draw_tri = sis_fallback_tri;
	 index |= SIS_FALLBACK_BIT;
      }
   }

   if (index != smesa->RenderIndex) {
      smesa->RenderIndex = index;

      tnl->Driver.Render.Points = rast_tab[index].points;
      tnl->Driver.Render.Line = rast_tab[index].line;
      tnl->Driver.Render.ClippedLine = rast_tab[index].line;
      tnl->Driver.Render.Triangle = rast_tab[index].triangle;
      tnl->Driver.Render.Quad = rast_tab[index].quad;

      if (index == 0) {
	 tnl->Driver.Render.PrimTabVerts = sis_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = sis_render_tab_elts;
	 tnl->Driver.Render.ClippedPolygon = sis_fast_clipped_poly;
      } else {
	 tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
	 tnl->Driver.Render.ClippedPolygon = _tnl_RenderClippedPolygon;
      }
   }
}

/**********************************************************************/
/*                Multipass rendering for front buffering             */
/**********************************************************************/
static GLboolean multipass_cliprect( GLcontext *ctx, GLuint pass )
{
   sisContextPtr smesa = SIS_CONTEXT( ctx );

   if (pass >= smesa->driDrawable->numClipRects) {
      return GL_FALSE;
   } else {
      GLint x1, y1, x2, y2;

      x1 = smesa->driDrawable->pClipRects[pass].x1 - smesa->driDrawable->x;
      y1 = smesa->driDrawable->pClipRects[pass].y1 - smesa->driDrawable->y;
      x2 = smesa->driDrawable->pClipRects[pass].x2 - smesa->driDrawable->x;
      y2 = smesa->driDrawable->pClipRects[pass].y2 - smesa->driDrawable->y;

      if (ctx->Scissor.Enabled) {
         GLint scisy1 = Y_FLIP(ctx->Scissor.Y + ctx->Scissor.Height - 1);
         GLint scisy2 = Y_FLIP(ctx->Scissor.Y);

         if (ctx->Scissor.X > x1)
            x1 = ctx->Scissor.X;
         if (scisy1 > y1)
            y1 = scisy1;
         if (ctx->Scissor.X + ctx->Scissor.Width - 1 < x2)
            x2 = ctx->Scissor.X + ctx->Scissor.Width - 1;
         if (scisy2 < y2)
            y2 = scisy2;
      }

      MMIO(REG_3D_ClipTopBottom, y1 << 13 | y2);
      MMIO(REG_3D_ClipLeftRight, x1 << 13 | x2);
      /* Mark that we clobbered these registers */
      smesa->GlobalFlag |= GFLAG_CLIPPING;
      return GL_TRUE;
   }
}



/**********************************************************************/
/*                 Validate state at pipeline start                   */
/**********************************************************************/

static void sisRunPipeline( GLcontext *ctx )
{
   sisContextPtr smesa = SIS_CONTEXT( ctx );

   if (smesa->NewGLState) {
      SIS_FIREVERTICES(smesa);
      if (smesa->NewGLState & _NEW_TEXTURE) {
	 sisUpdateTextureState(ctx);
      }

      if (smesa->NewGLState & _SIS_NEW_RENDER_STATE)
	 sisChooseRenderState( ctx );

      smesa->NewGLState = 0;
   }

   _tnl_run_pipeline( ctx );

   /* XXX: If we put flushing in sis_state.c and friends, we can avoid this.
    * Is it worth it?
    */
   SIS_FIREVERTICES(smesa);
}

/**********************************************************************/
/*                 High level hooks for t_vb_render.c                 */
/**********************************************************************/

/* This is called when Mesa switches between rendering triangle
 * primitives (such as GL_POLYGON, GL_QUADS, GL_TRIANGLE_STRIP, etc),
 * and lines, points and bitmaps.
 */

static void sisRasterPrimitive( GLcontext *ctx, GLuint hwprim )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   if (smesa->hw_primitive != hwprim) {
      SIS_FIREVERTICES(smesa);
      smesa->hw_primitive = hwprim;

      smesa->AGPParseSet &= ~(MASK_PsDataType | MASK_PsShadingMode);
      smesa->AGPParseSet |= hw_prim_agp_type[hwprim];

      if (smesa->is6326) {
	 smesa->dwPrimitiveSet &= ~(MASK_6326_DrawPrimitiveCommand |
	    MASK_6326_SetFirePosition | MASK_6326_ShadingMode);
	 smesa->dwPrimitiveSet |= hwprim | hw_prim_6326_mmio_fire[hwprim];
      } else {
	 smesa->dwPrimitiveSet &= ~(MASK_DrawPrimitiveCommand |
	    MASK_SetFirePosition | MASK_ShadingMode);
	 smesa->dwPrimitiveSet |= hwprim | hw_prim_mmio_fire[hwprim];
      }

      if (ctx->Light.ShadeModel == GL_FLAT) {
	 smesa->AGPParseSet |= hw_prim_agp_shade[hwprim];
	 smesa->dwPrimitiveSet |= hw_prim_mmio_shade[hwprim];
      } else {
	 smesa->AGPParseSet |= MASK_PsShadingSmooth;
	 if (smesa->is6326) {
	    smesa->dwPrimitiveSet |= OP_6326_3D_SHADE_FLAT_GOURAUD;
	 } else {
	    smesa->dwPrimitiveSet |= SHADE_GOURAUD;
	 }
      }
   }
}

static void sisRenderPrimitive( GLcontext *ctx, GLenum prim )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   smesa->render_primitive = prim;

   if (prim >= GL_TRIANGLES && (ctx->_TriangleCaps & DD_TRI_UNFILLED))
      return;
   sisRasterPrimitive( ctx, hw_prim[prim] );
}

#define EMIT_ATTR( ATTR, STYLE)						\
do {									\
   smesa->vertex_attrs[smesa->vertex_attr_count].attrib = (ATTR);	\
   smesa->vertex_attrs[smesa->vertex_attr_count].format = (STYLE);	\
   smesa->vertex_attr_count++;						\
} while (0)

#define EMIT_PAD( N )							\
do {									\
   smesa->vertex_attrs[smesa->vertex_attr_count].attrib = 0;		\
   smesa->vertex_attrs[smesa->vertex_attr_count].format = EMIT_PAD;	\
   smesa->vertex_attrs[smesa->vertex_attr_count].offset = (N);		\
   smesa->vertex_attr_count++;						\
} while (0)
				
static void sisRenderStart( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   DECLARE_RENDERINPUTS(index_bitset);
   GLuint AGPParseSet = smesa->AGPParseSet;
   GLboolean tex_fallback = GL_FALSE;

   RENDERINPUTS_COPY( index_bitset, tnl->render_inputs_bitset );

   if (ctx->DrawBuffer->_ColorDrawBufferMask[0] == BUFFER_BIT_FRONT_LEFT && 
      smesa->driDrawable->numClipRects != 0)
   {
      multipass_cliprect(ctx, 0);
      if (smesa->driDrawable->numClipRects > 1)
         tnl->Driver.Render.Multipass = multipass_cliprect;
      else
         tnl->Driver.Render.Multipass = NULL;
   } else {
      tnl->Driver.Render.Multipass = NULL;
   }

   /* Important:
    */
   VB->AttribPtr[VERT_ATTRIB_POS] = VB->NdcPtr;
   smesa->vertex_attr_count = 0;

   /* EMIT_ATTR's must be in order as they tell t_vertex.c how to build up a
    * hardware vertex.
    */

   AGPParseSet &= ~(MASK_VertexDWSize | MASK_VertexDataFormat);
   AGPParseSet |= SiS_PS_HAS_XYZ | SiS_PS_HAS_DIFFUSE;
   if (RENDERINPUTS_TEST_RANGE( index_bitset, _TNL_FIRST_TEX, _TNL_LAST_TEX )) {
      EMIT_ATTR(_TNL_ATTRIB_POS, EMIT_4F_VIEWPORT);
      AGPParseSet |= SiS_PS_HAS_W;
      smesa->coloroffset = 4;
   } else {
      EMIT_ATTR(_TNL_ATTRIB_POS, EMIT_3F_VIEWPORT);
      smesa->coloroffset = 3;
   }

   EMIT_ATTR(_TNL_ATTRIB_COLOR0, EMIT_4UB_4F_BGRA);

   smesa->specoffset = 0;
   if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_COLOR1 ) ||
       RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_FOG )) {
      AGPParseSet |= SiS_PS_HAS_SPECULAR;

      if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_COLOR1 )) {
	 EMIT_ATTR(_TNL_ATTRIB_COLOR1, EMIT_3UB_3F_BGR);
	 smesa->specoffset = smesa->coloroffset + 1;
      } else {
	 EMIT_PAD(3);
      }

      if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_FOG )) {
	 EMIT_ATTR(_TNL_ATTRIB_FOG, EMIT_1UB_1F);
      } else {
	 EMIT_PAD(1);
      }
   }

   /* projective textures are not supported by the hardware */
   if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_TEX0 )) {
      if (VB->TexCoordPtr[0]->size > 2)
	 tex_fallback = GL_TRUE;
      EMIT_ATTR(_TNL_ATTRIB_TEX0, EMIT_2F);
      AGPParseSet |= SiS_PS_HAS_UV0;
   }
   /* Will only hit tex1 on SiS300 */
   if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_TEX1 )) {
      if (VB->TexCoordPtr[1]->size > 2)
	 tex_fallback = GL_TRUE;
      EMIT_ATTR(_TNL_ATTRIB_TEX1, EMIT_2F);
      AGPParseSet |= SiS_PS_HAS_UV1;
   }
   FALLBACK(smesa, SIS_FALLBACK_TEXTURE, tex_fallback);

   if (!RENDERINPUTS_EQUAL( smesa->last_tcl_state_bitset, index_bitset )) {
      smesa->AGPParseSet = AGPParseSet;

      smesa->vertex_size =  _tnl_install_attrs( ctx, smesa->vertex_attrs, 
	 smesa->vertex_attr_count, smesa->hw_viewport, 0 );

      smesa->vertex_size >>= 2;
      smesa->AGPParseSet |= smesa->vertex_size << 28;
   }
}

static void sisRenderFinish( GLcontext *ctx )
{
}

/**********************************************************************/
/*                    AGP/PCI vertex submission                       */
/**********************************************************************/

void
sisFlushPrimsLocked(sisContextPtr smesa)
{
   if (smesa->vb_cur == smesa->vb_last)
      return;

   if (smesa->is6326)
      sis6326UpdateHWState(smesa->glCtx);
   else
      sisUpdateHWState(smesa->glCtx);

   if (smesa->using_agp) {
      mWait3DCmdQueue(8);
      mEndPrimitive();
      MMIO(REG_3D_AGPCmBase, (smesa->vb_last - smesa->vb) +
         smesa->vb_agp_offset);
      MMIO(REG_3D_AGPTtDwNum, ((smesa->vb_cur - smesa->vb_last) / 4) |
	 0x50000000);
      MMIO(REG_3D_ParsingSet, smesa->AGPParseSet);
      MMIO(REG_3D_AGPCmFire, (GLint)(-1));
      mEndPrimitive();
   } else {
      int mmio_index = 0, incr = 0;
      void (*sis_emit_func)(sisContextPtr smesa, char *verts) = NULL;

      if (smesa->AGPParseSet & MASK_PsShadingSmooth)
	 mmio_index |= VERT_SMOOTH;
      if (smesa->AGPParseSet & SiS_PS_HAS_SPECULAR)
	 mmio_index |= VERT_SPEC;
      if (smesa->AGPParseSet & SiS_PS_HAS_W)
	 mmio_index |= VERT_W;
      if (smesa->AGPParseSet & SiS_PS_HAS_UV0)
	 mmio_index |= VERT_UV0;
      if (smesa->AGPParseSet & SiS_PS_HAS_UV1)
	 mmio_index |= VERT_UV1;
      if (smesa->is6326)
	 mmio_index |= VERT_6326;

      switch (smesa->AGPParseSet & MASK_PsDataType) {
      case MASK_PsPointList:
         incr = smesa->vertex_size * 4;
	 sis_emit_func = sis_point_func_mmio[mmio_index];
	 break;
      case MASK_PsLineList:
         incr = smesa->vertex_size * 4 * 2;
	 sis_emit_func = sis_line_func_mmio[mmio_index];
	 break;
      case MASK_PsTriangleList:
         incr = smesa->vertex_size * 4 * 3;
	 sis_emit_func = sis_tri_func_mmio[mmio_index];
	 break;
      }

      if (!smesa->is6326) {
	 mWait3DCmdQueue(1);
	 MMIO(REG_3D_PrimitiveSet, smesa->dwPrimitiveSet);
      }
      while (smesa->vb_last < smesa->vb_cur) {
	 sis_emit_func(smesa, smesa->vb_last);
	 smesa->vb_last += incr;
      }
      mWait3DCmdQueue(1);
      mEndPrimitive();

      /* With PCI, we can just start writing to the start of the VB again. */
      smesa->vb_cur = smesa->vb;
   }
   smesa->vb_last = smesa->vb_cur;
}

void sisFlushPrims(sisContextPtr smesa)
{
   LOCK_HARDWARE();
   sisFlushPrimsLocked(smesa);
   UNLOCK_HARDWARE();
}

/**********************************************************************/
/*           Transition to/from hardware rasterization.               */
/**********************************************************************/

static const char * const fallbackStrings[] = {
   "Texture mode",
   "Texture 0 mode",
   "Texture 1 mode",
   "Texture 0 env",	/* Note: unused */
   "Texture 1 env",	/* Note: unused */
   "glDrawBuffer(GL_FRONT_AND_BACK)",
   "glEnable(GL_STENCIL) without hw stencil buffer",
   "write mask",
   "no_rast",
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

void sisFallback( GLcontext *ctx, GLuint bit, GLboolean mode )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   GLuint oldfallback = smesa->Fallback;

   if (mode) {
      smesa->Fallback |= bit;
      if (oldfallback == 0) {
	 SIS_FIREVERTICES(smesa);
	 _swsetup_Wakeup( ctx );
	 smesa->RenderIndex = ~0;
         if (SIS_DEBUG & DEBUG_FALLBACKS) {
            fprintf(stderr, "SiS begin rasterization fallback: 0x%x %s\n",
                    bit, getFallbackString(bit));
         }
      }
   }
   else {
      smesa->Fallback &= ~bit;
      if (oldfallback == bit) {
	 _swrast_flush( ctx );
	 tnl->Driver.Render.Start = sisRenderStart;
	 tnl->Driver.Render.PrimitiveNotify = sisRenderPrimitive;
	 tnl->Driver.Render.Finish = sisRenderFinish;

	 tnl->Driver.Render.BuildVertices = _tnl_build_vertices;
	 tnl->Driver.Render.CopyPV = _tnl_copy_pv;
	 tnl->Driver.Render.Interp = _tnl_interp;

	 _tnl_invalidate_vertex_state( ctx, ~0 );
	 _tnl_invalidate_vertices( ctx, ~0 );
	 _tnl_install_attrs( ctx, 
			     smesa->vertex_attrs, 
			     smesa->vertex_attr_count,
			     smesa->hw_viewport, 0 ); 

	 smesa->NewGLState |= _SIS_NEW_RENDER_STATE;
         if (SIS_DEBUG & DEBUG_FALLBACKS) {
            fprintf(stderr, "SiS end rasterization fallback: 0x%x %s\n",
                    bit, getFallbackString(bit));
         }
      }
   }
}


/**********************************************************************/
/*                            Initialization.                         */
/**********************************************************************/

void sisInitTriFuncs( GLcontext *ctx )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   static int firsttime = 1;

   if (firsttime) {
      init_rast_tab();
      firsttime = 0;

      sis_vert_init_none();
      sis_vert_init_g();
      sis_vert_init_w();
      sis_vert_init_gw();
      sis_vert_init_s();
      sis_vert_init_gs();
      sis_vert_init_ws();
      sis_vert_init_gws();
      sis_vert_init_t0();
      sis_vert_init_gt0();
      sis_vert_init_wt0();
      sis_vert_init_gwt0();
      sis_vert_init_st0();
      sis_vert_init_gst0();
      sis_vert_init_wst0();
      sis_vert_init_gwst0();
      sis_vert_init_t1();
      sis_vert_init_gt1();
      sis_vert_init_wt1();
      sis_vert_init_gwt1();
      sis_vert_init_st1();
      sis_vert_init_gst1();
      sis_vert_init_wst1();
      sis_vert_init_gwst1();
      sis_vert_init_t0t1();
      sis_vert_init_gt0t1();
      sis_vert_init_wt0t1();
      sis_vert_init_gwt0t1();
      sis_vert_init_st0t1();
      sis_vert_init_gst0t1();
      sis_vert_init_wst0t1();
      sis_vert_init_gwst0t1();
   }

   smesa->RenderIndex = ~0;
   smesa->NewGLState |= _SIS_NEW_RENDER_STATE;

   tnl->Driver.RunPipeline = sisRunPipeline;
   tnl->Driver.Render.Start = sisRenderStart;
   tnl->Driver.Render.Finish = sisRenderFinish;
   tnl->Driver.Render.PrimitiveNotify = sisRenderPrimitive;
   tnl->Driver.Render.ResetLineStipple = _swrast_ResetLineStipple;

   tnl->Driver.Render.BuildVertices = _tnl_build_vertices;
   tnl->Driver.Render.CopyPV = _tnl_copy_pv;
   tnl->Driver.Render.Interp = _tnl_interp;

   _tnl_init_vertices( ctx, ctx->Const.MaxArrayLockSize + 12, 
		       (6 + 2*ctx->Const.MaxTextureUnits) * sizeof(GLfloat) );

   smesa->verts = (char *)tnl->clipspace.vertex_buf;
}
