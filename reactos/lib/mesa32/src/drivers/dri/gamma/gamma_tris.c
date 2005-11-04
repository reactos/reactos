/*
 * Copyright 2001 by Alan Hourihane.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, <alanh@tungstengraphics.com>
 *           Keith Whitwell, <keith@tungstengraphics.com>
 *
 * 3DLabs Gamma driver.
 */

#include "gamma_context.h"
#include "gamma_vb.h"
#include "gamma_tris.h"

#include "glheader.h"
#include "mtypes.h"
#include "macros.h"
#include "colormac.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"


/***********************************************************************
 *                   Build hardware rasterization functions            *
 ***********************************************************************/

#define GAMMA_RAST_ALPHA_BIT	0x01
#define GAMMA_RAST_TEX_BIT	0x02
#define GAMMA_RAST_FLAT_BIT	0x04

static gamma_point_func gamma_point_tab[0x8];
static gamma_line_func gamma_line_tab[0x8];
static gamma_tri_func gamma_tri_tab[0x8];
static gamma_quad_func gamma_quad_tab[0x8];

#define IND (0)
#define TAG(x) x
#include "gamma_tritmp.h"

#define IND (GAMMA_RAST_ALPHA_BIT)
#define TAG(x) x##_alpha
#include "gamma_tritmp.h"

#define IND (GAMMA_RAST_TEX_BIT)
#define TAG(x) x##_tex
#include "gamma_tritmp.h"

#define IND (GAMMA_RAST_ALPHA_BIT|GAMMA_RAST_TEX_BIT)
#define TAG(x) x##_alpha_tex
#include "gamma_tritmp.h"

#define IND (GAMMA_RAST_FLAT_BIT)
#define TAG(x) x##_flat
#include "gamma_tritmp.h"

#define IND (GAMMA_RAST_ALPHA_BIT|GAMMA_RAST_FLAT_BIT)
#define TAG(x) x##_alpha_flat
#include "gamma_tritmp.h"

#define IND (GAMMA_RAST_TEX_BIT|GAMMA_RAST_FLAT_BIT)
#define TAG(x) x##_tex_flat
#include "gamma_tritmp.h"

#define IND (GAMMA_RAST_ALPHA_BIT|GAMMA_RAST_TEX_BIT|GAMMA_RAST_FLAT_BIT)
#define TAG(x) x##_alpha_tex_flat
#include "gamma_tritmp.h"


static void init_rast_tab( void )
{
   gamma_init();
   gamma_init_alpha();
   gamma_init_tex();
   gamma_init_alpha_tex();
   gamma_init_flat();
   gamma_init_alpha_flat();
   gamma_init_tex_flat();
   gamma_init_alpha_tex_flat();
}

/***********************************************************************
 *                    Rasterization fallback helpers                   *
 ***********************************************************************/


/* This code is hit only when a mix of accelerated and unaccelerated
 * primitives are being drawn, and only for the unaccelerated
 * primitives.  
 */
static void 
gamma_fallback_quad( gammaContextPtr gmesa, 
		    const gammaVertex *v0, 
		    const gammaVertex *v1, 
		    const gammaVertex *v2, 
		    const gammaVertex *v3 )
{
   GLcontext *ctx = gmesa->glCtx;
   SWvertex v[4];
   gamma_translate_vertex( ctx, v0, &v[0] );
   gamma_translate_vertex( ctx, v1, &v[1] );
   gamma_translate_vertex( ctx, v2, &v[2] );
   gamma_translate_vertex( ctx, v3, &v[3] );
   _swrast_Quad( ctx, &v[0], &v[1], &v[2], &v[3] );
}

static void 
gamma_fallback_tri( gammaContextPtr gmesa, 
		    const gammaVertex *v0, 
		    const gammaVertex *v1, 
		    const gammaVertex *v2 )
{
   GLcontext *ctx = gmesa->glCtx;
   SWvertex v[3];
   gamma_translate_vertex( ctx, v0, &v[0] );
   gamma_translate_vertex( ctx, v1, &v[1] );
   gamma_translate_vertex( ctx, v2, &v[2] );
   _swrast_Triangle( ctx, &v[0], &v[1], &v[2] );
}

static void 
gamma_fallback_line( gammaContextPtr gmesa,
		     const gammaVertex *v0,
		     const gammaVertex *v1 )
{
   GLcontext *ctx = gmesa->glCtx;
   SWvertex v[2];
   gamma_translate_vertex( ctx, v0, &v[0] );
   gamma_translate_vertex( ctx, v1, &v[1] );
   _swrast_Line( ctx, &v[0], &v[1] );
}


#if 0
static void 
gamma_fallback_point( gammaContextPtr gmesa, 
		      const gammaVertex *v0 )
{
   GLcontext *ctx = gmesa->glCtx;
   SWvertex v[1];
   gamma_translate_vertex( ctx, v0, &v[0] );
   _swrast_Point( ctx, &v[0] );
}
#endif


/***********************************************************************
 *                    Choose rasterization functions                   *
 ***********************************************************************/

#define _GAMMA_NEW_RASTER_STATE (_NEW_FOG | \
				 _NEW_TEXTURE | \
				 _DD_NEW_TRI_SMOOTH | \
				 _DD_NEW_LINE_SMOOTH | \
				 _DD_NEW_POINT_SMOOTH | \
				 _DD_NEW_TRI_STIPPLE | \
				 _DD_NEW_LINE_STIPPLE)

#define LINE_FALLBACK (0)
#define TRI_FALLBACK (0)

static void gammaChooseRasterState(GLcontext *ctx)
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   GLuint flags = ctx->_TriangleCaps;
   GLuint ind = 0;

   if ( ctx->Line.SmoothFlag || 
        ctx->Polygon.SmoothFlag || 
        ctx->Point.SmoothFlag )
      gmesa->Begin |= B_AntiAliasEnable;
   else
      gmesa->Begin &= ~B_AntiAliasEnable;

   if ( ctx->Texture.Unit[0]._ReallyEnabled ) {
      ind |= GAMMA_RAST_TEX_BIT;
      gmesa->Begin |= B_TextureEnable;
   } else
      gmesa->Begin &= ~B_TextureEnable;

   if (flags & DD_LINE_STIPPLE)
      gmesa->Begin |= B_LineStippleEnable;
   else
      gmesa->Begin &= ~B_LineStippleEnable;
   
   if (flags & DD_TRI_STIPPLE)
      gmesa->Begin |= B_AreaStippleEnable;
   else
      gmesa->Begin &= ~B_AreaStippleEnable;

   if (ctx->Fog.Enabled) 
      gmesa->Begin |= B_FogEnable;
   else
      gmesa->Begin &= ~B_FogEnable;

   if (ctx->Color.BlendEnabled || ctx->Color.AlphaEnabled)
      ind |= GAMMA_RAST_ALPHA_BIT;

   if ( flags & DD_FLATSHADE )
      ind |= GAMMA_RAST_FLAT_BIT;

   gmesa->draw_line = gamma_line_tab[ind];
   gmesa->draw_tri = gamma_tri_tab[ind];
   gmesa->draw_quad = gamma_quad_tab[ind];
   gmesa->draw_point = gamma_point_tab[ind];

   /* Hook in fallbacks for specific primitives.  CURRENTLY DISABLED
    */
   if (flags & LINE_FALLBACK) 
      gmesa->draw_line = gamma_fallback_line;
	 
   if (flags & TRI_FALLBACK) {
      gmesa->draw_tri = gamma_fallback_tri;
      gmesa->draw_quad = gamma_fallback_quad;
   }
}




/***********************************************************************
 *          Macros for t_dd_tritmp.h to draw basic primitives          *
 ***********************************************************************/

#define TRI( a, b, c )				\
do {						\
   gmesa->draw_tri( gmesa, a, b, c );	\
} while (0)

#define QUAD( a, b, c, d )			\
do {						\
   gmesa->draw_quad( gmesa, a, b, c, d );	\
} while (0)

#define LINE( v0, v1 )				\
do {						\
   gmesa->draw_line( gmesa, v0, v1 );	\
} while (0)

#define POINT( v0 )				\
do {						\
   gmesa->draw_point( gmesa, v0 );		\
} while (0)


/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/

#define GAMMA_OFFSET_BIT 	0x01
#define GAMMA_TWOSIDE_BIT	0x02
#define GAMMA_UNFILLED_BIT	0x04
#define GAMMA_FALLBACK_BIT	0x08
#define GAMMA_MAX_TRIFUNC	0x10


static struct {
   tnl_points_func		points;
   tnl_line_func		line;
   tnl_triangle_func	triangle;
   tnl_quad_func		quad;
} rast_tab[GAMMA_MAX_TRIFUNC];


#define DO_FALLBACK (IND & GAMMA_FALLBACK_BIT)
#define DO_OFFSET    0 /* (IND & GAMMA_OFFSET_BIT) */
#define DO_UNFILLED  0 /* (IND & GAMMA_UNFILLED_BIT) */
#define DO_TWOSIDE  (IND & GAMMA_TWOSIDE_BIT)
#define DO_FLAT      0
#define DO_TRI       1
#define DO_QUAD      1
#define DO_LINE      1
#define DO_POINTS    1
#define DO_FULL_QUAD 1

#define HAVE_RGBA         1
#define HAVE_SPEC         0
#define HAVE_BACK_COLORS  0
#define HAVE_HW_FLATSHADE 1
#define VERTEX            gammaVertex
#define TAB               rast_tab

#define DEPTH_SCALE 1.0
#define UNFILLED_TRI unfilled_tri
#define UNFILLED_QUAD unfilled_quad
#define VERT_X(_v) _v->v.x
#define VERT_Y(_v) _v->v.y
#define VERT_Z(_v) _v->v.z
#define AREA_IS_CCW( a ) (a > 0)
#define GET_VERTEX(e) (gmesa->verts + (e * gmesa->vertex_size * sizeof(int)))

#define VERT_SET_RGBA( v, c )  					\
do {								\
   UNCLAMPED_FLOAT_TO_UBYTE(v->ub4[4][0], (c)[0]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(v->ub4[4][1], (c)[1]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(v->ub4[4][2], (c)[2]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(v->ub4[4][3], (c)[3]);		\
} while (0)
#define VERT_COPY_RGBA( v0, v1 ) v0->ui[4] = v1->ui[4]
#define VERT_SAVE_RGBA( idx )    color[idx] = v[idx]->ui[4]
#define VERT_RESTORE_RGBA( idx ) v[idx]->ui[4] = color[idx]   

#define LOCAL_VARS(n)					\
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);	\
   GLuint color[n];				\
   (void) color;


/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/

static const GLuint hw_prim[GL_POLYGON+1] = {
   B_PrimType_Points,
   B_PrimType_Lines,
   B_PrimType_Lines,
   B_PrimType_Lines,
   B_PrimType_Triangles,
   B_PrimType_Triangles,
   B_PrimType_Triangles,
   B_PrimType_Triangles,
   B_PrimType_Triangles,
   B_PrimType_Triangles
};

static void gammaResetLineStipple( GLcontext *ctx );
static void gammaRasterPrimitive( GLcontext *ctx, GLuint hwprim );
static void gammaRenderPrimitive( GLcontext *ctx, GLenum prim );

#define RASTERIZE(x) if (gmesa->hw_primitive != hw_prim[x]) \
                        gammaRasterPrimitive( ctx, hw_prim[x] )
#define RENDER_PRIMITIVE gmesa->render_primitive
#define TAG(x) x
#define IND GAMMA_FALLBACK_BIT
#include "tnl_dd/t_dd_unfilled.h"
#undef IND

/***********************************************************************
 *                      Generate GL render functions                   *
 ***********************************************************************/

#define IND (0)
#define TAG(x) x
#include "tnl_dd/t_dd_tritmp.h"

#define IND (GAMMA_OFFSET_BIT)
#define TAG(x) x##_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (GAMMA_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "tnl_dd/t_dd_tritmp.h"

#define IND (GAMMA_TWOSIDE_BIT|GAMMA_OFFSET_BIT)
#define TAG(x) x##_twoside_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (GAMMA_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (GAMMA_OFFSET_BIT|GAMMA_UNFILLED_BIT)
#define TAG(x) x##_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (GAMMA_TWOSIDE_BIT|GAMMA_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (GAMMA_TWOSIDE_BIT|GAMMA_OFFSET_BIT|GAMMA_UNFILLED_BIT)
#define TAG(x) x##_twoside_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"



static void init_render_tab( void )
{
   init();
   init_offset();
   init_twoside();
   init_twoside_offset();
   init_unfilled();
   init_offset_unfilled();
   init_twoside_unfilled();
   init_twoside_offset_unfilled();
}


/**********************************************************************/
/*               Render unclipped begin/end objects                   */
/**********************************************************************/

#define VERT(x) (gammaVertex *)(gammaverts + (x * size * sizeof(int)))
#define RENDER_POINTS( start, count )		\
   for ( ; start < count ; start++) 		\
      gmesa->draw_point( gmesa, VERT(start) )
#define RENDER_LINE( v0, v1 ) \
   gmesa->draw_line( gmesa, VERT(v0), VERT(v1) )
#define RENDER_TRI( v0, v1, v2 )  \
   gmesa->draw_tri( gmesa, VERT(v0), VERT(v1), VERT(v2) )
#define RENDER_QUAD( v0, v1, v2, v3 ) \
   gmesa->draw_quad( gmesa, VERT(v0), VERT(v1), VERT(v2), VERT(v3) )
#define INIT(x) gammaRenderPrimitive( ctx, x );
#undef LOCAL_VARS
#define LOCAL_VARS						\
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);		\
   const GLuint size = gmesa->vertex_size;		\
   const char *gammaverts = (char *)gmesa->verts;		\
   const GLboolean stipple = ctx->Line.StippleFlag;		\
   (void) stipple;
#define RESET_STIPPLE	if ( stipple ) gammaResetLineStipple( ctx );
#define RESET_OCCLUSION
#define PRESERVE_VB_DEFS
#define ELT(x) (x)
#define TAG(x) gamma_##x##_verts
#include "tnl/t_vb_rendertmp.h"


/**********************************************************************/
/*                   Render clipped primitives                        */
/**********************************************************************/

static void gammaRenderClippedPoly( GLcontext *ctx, const GLuint *elts, 
				   GLuint n )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint prim = gmesa->render_primitive;

   /* Render the new vertices as an unclipped polygon. 
    */
   {
      GLuint *tmp = VB->Elts;
      VB->Elts = (GLuint *)elts;
      tnl->Driver.Render.PrimTabElts[GL_POLYGON]( ctx, 0, n, PRIM_BEGIN|PRIM_END );
      VB->Elts = tmp;
   }

   /* Restore the render primitive
    */
   if (prim != GL_POLYGON)
      tnl->Driver.Render.PrimitiveNotify( ctx, prim );
}

static void gammaRenderClippedLine( GLcontext *ctx, GLuint ii, GLuint jj )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   tnl->Driver.Render.Line( ctx, ii, jj );
}


/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/



#define _GAMMA_NEW_RENDERSTATE (_DD_NEW_TRI_UNFILLED |		\
			       _DD_NEW_TRI_LIGHT_TWOSIDE |	\
			       _DD_NEW_TRI_OFFSET)

#define ANY_RASTER_FLAGS (DD_TRI_LIGHT_TWOSIDE|DD_TRI_OFFSET|DD_TRI_UNFILLED)

static void gammaChooseRenderState(GLcontext *ctx)
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint flags = ctx->_TriangleCaps;
   GLuint index = 0;

   if (flags & ANY_RASTER_FLAGS) {
      if (flags & DD_TRI_LIGHT_TWOSIDE)       index |= GAMMA_TWOSIDE_BIT;
      if (flags & DD_TRI_OFFSET)	      index |= GAMMA_OFFSET_BIT;
      if (flags & DD_TRI_UNFILLED)	      index |= GAMMA_UNFILLED_BIT;
   }

   if (gmesa->RenderIndex != index) {
      gmesa->RenderIndex = index;

      tnl->Driver.Render.Points = rast_tab[index].points;
      tnl->Driver.Render.Line = rast_tab[index].line;
      tnl->Driver.Render.Triangle = rast_tab[index].triangle;
      tnl->Driver.Render.Quad = rast_tab[index].quad;
         
      if (gmesa->RenderIndex == 0)
         tnl->Driver.Render.PrimTabVerts = gamma_render_tab_verts;
      else
         tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
      tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
      tnl->Driver.Render.ClippedLine = gammaRenderClippedLine;
      tnl->Driver.Render.ClippedPolygon = gammaRenderClippedPoly;
   }
}


/**********************************************************************/
/*                 High level hooks for t_vb_render.c                 */
/**********************************************************************/



/* Determine the rasterized primitive when not drawing unfilled 
 * polygons.
 *
 * Used only for the default render stage which always decomposes
 * primitives to trianges/lines/points.  For the accelerated stage,
 * which renders strips as strips, the equivalent calculations are
 * performed in gammarender.c.
 */

static void gammaRasterPrimitive( GLcontext *ctx, GLuint hwprim )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   if (gmesa->hw_primitive != hwprim)
      gmesa->hw_primitive = hwprim;
}

static void gammaRenderPrimitive( GLcontext *ctx, GLenum prim )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   gmesa->render_primitive = prim;
}

static void gammaRunPipeline( GLcontext *ctx )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   if ( gmesa->new_state )
      gammaDDUpdateHWState( ctx );

   if (gmesa->new_gl_state) {
      if (gmesa->new_gl_state & _NEW_TEXTURE)
	 gammaUpdateTextureState( ctx );

   if (!gmesa->Fallback) {
      if (gmesa->new_gl_state & _GAMMA_NEW_VERTEX)
	 gammaChooseVertexState( ctx );
      
      if (gmesa->new_gl_state & _GAMMA_NEW_RASTER_STATE)
	 gammaChooseRasterState( ctx );
      
      if (gmesa->new_gl_state & _GAMMA_NEW_RENDERSTATE)
	 gammaChooseRenderState( ctx );
   }
      
      gmesa->new_gl_state = 0;
   }

   _tnl_run_pipeline( ctx );
}

static void gammaRenderStart( GLcontext *ctx )
{
   /* Check for projective texturing.  Make sure all texcoord
    * pointers point to something.  (fix in mesa?)  
    */
   gammaCheckTexSizes( ctx );
}

static void gammaRenderFinish( GLcontext *ctx )
{
   if (0)
      _swrast_flush( ctx );	/* never needed */
}

static void gammaResetLineStipple( GLcontext *ctx )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   /* Reset the hardware stipple counter.
    */
   CHECK_DMA_BUFFER(gmesa, 1);
   WRITE(gmesa->buf, UpdateLineStippleCounters, 0);
}


/**********************************************************************/
/*           Transition to/from hardware rasterization.               */
/**********************************************************************/


void gammaFallback( gammaContextPtr gmesa, GLuint bit, GLboolean mode )
{
   GLcontext *ctx = gmesa->glCtx;
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint oldfallback = gmesa->Fallback;

   if (mode) {
      gmesa->Fallback |= bit;
      if (oldfallback == 0) {
	 _swsetup_Wakeup( ctx );
	 _tnl_need_projected_coords( ctx, GL_TRUE );
	 gmesa->RenderIndex = ~0;
      }
   }
   else {
      gmesa->Fallback &= ~bit;
      if (oldfallback == bit) {
	 _swrast_flush( ctx );
	 tnl->Driver.Render.Start = gammaRenderStart;
	 tnl->Driver.Render.PrimitiveNotify = gammaRenderPrimitive;
	 tnl->Driver.Render.Finish = gammaRenderFinish;
	 tnl->Driver.Render.BuildVertices = gammaBuildVertices;
         tnl->Driver.Render.ResetLineStipple = gammaResetLineStipple;
	 gmesa->new_gl_state |= (_GAMMA_NEW_RENDERSTATE|
				 _GAMMA_NEW_RASTER_STATE|
				 _GAMMA_NEW_VERTEX);
      }
   }
}


/**********************************************************************/
/*                            Initialization.                         */
/**********************************************************************/


void gammaDDInitTriFuncs( GLcontext *ctx )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   static int firsttime = 1;

   if (firsttime) {
      init_rast_tab();
      init_render_tab();
      firsttime = 0;
   }
   
   gmesa->RenderIndex = ~0;

   tnl->Driver.RunPipeline = gammaRunPipeline;
   tnl->Driver.Render.Start = gammaRenderStart;
   tnl->Driver.Render.Finish = gammaRenderFinish; 
   tnl->Driver.Render.PrimitiveNotify = gammaRenderPrimitive;
   tnl->Driver.Render.ResetLineStipple = gammaResetLineStipple;
   tnl->Driver.Render.BuildVertices = gammaBuildVertices;
}
