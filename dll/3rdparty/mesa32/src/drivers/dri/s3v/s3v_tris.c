/*
 * Author: Max Lingua <sunmax@libero.it>
 */

#include <stdio.h>
#include <stdlib.h>

#include <sys/ioctl.h>

#include "s3v_context.h"
#include "s3v_vb.h"
#include "s3v_tris.h"

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

#define DO_TRI			1
#define HAVE_RGBA		1
#define HAVE_SPEC		0
#define HAVE_BACK_COLORS	0
#define HAVE_HW_FLATSHADE	1
#define VERTEX			s3vVertex
#define TAB			rast_tab

#define VERT_SET_RGBA( v, c ) \
do { \
	UNCLAMPED_FLOAT_TO_RGBA_CHAN( v->ub4[4], c); \
/*	*(v->ub4[4]) = c; \ */ \
} while (0)
#define VERT_COPY_RGBA( v0, v1 ) v0->ui[4] = v1->ui[4]
/*
#define VERT_COPY_RGBA1( v0, v1 ) v0->ui[4] = v1->ui[4]
*/
#define VERT_SAVE_RGBA( idx )    color[idx] = v[idx]->ui[4]
#define VERT_RESTORE_RGBA( idx ) v[idx]->ui[4] = color[idx]

#define S3V_OFFSET_BIT  	0x01
#define S3V_TWOSIDE_BIT 	0x02
#define S3V_UNFILLED_BIT        0x04
#define S3V_FALLBACK_BIT        0x08
#define S3V_MAX_TRIFUNC 	0x10


static struct {
	tnl_points_func		points;
	tnl_line_func		line;
	tnl_triangle_func	triangle;
	tnl_quad_func		quad;
} rast_tab[S3V_MAX_TRIFUNC];

#define S3V_RAST_CULL_BIT	0x01
#define S3V_RAST_FLAT_BIT	0x02
#define S3V_RAST_TEX_BIT	0x04

static s3v_point_func s3v_point_tab[0x8];
static s3v_line_func s3v_line_tab[0x8];
static s3v_tri_func s3v_tri_tab[0x8];
static s3v_quad_func s3v_quad_tab[0x8];

#define IND (0)
#define TAG(x) x
#include "s3v_tritmp.h"

#define IND (S3V_RAST_CULL_BIT)
#define TAG(x) x##_cull
#include "s3v_tritmp.h"

#define IND (S3V_RAST_FLAT_BIT)
#define TAG(x) x##_flat
#include "s3v_tritmp.h"

#define IND (S3V_RAST_CULL_BIT|S3V_RAST_FLAT_BIT)
#define TAG(x) x##_cull_flat
#include "s3v_tritmp.h"

#define IND (S3V_RAST_TEX_BIT)
#define TAG(x) x##_tex
#include "s3v_tritmp.h"

#define IND (S3V_RAST_CULL_BIT|S3V_RAST_TEX_BIT)
#define TAG(x) x##_cull_tex
#include "s3v_tritmp.h"

#define IND (S3V_RAST_FLAT_BIT|S3V_RAST_TEX_BIT)
#define TAG(x) x##_flat_tex
#include "s3v_tritmp.h"

#define IND (S3V_RAST_CULL_BIT|S3V_RAST_FLAT_BIT|S3V_RAST_TEX_BIT)
#define TAG(x) x##_cull_flat_tex
#include "s3v_tritmp.h"

static void init_rast_tab( void )
{
	DEBUG(("*** init_rast_tab ***\n"));

	s3v_init();
	s3v_init_cull();
	s3v_init_flat();
	s3v_init_cull_flat();
	s3v_init_tex();
	s3v_init_cull_tex();
	s3v_init_flat_tex();
	s3v_init_cull_flat_tex();
}

/***********************************************************************
 *                    Rasterization fallback helpers                   *
 ***********************************************************************/


/* This code is hit only when a mix of accelerated and unaccelerated
 * primitives are being drawn, and only for the unaccelerated
 * primitives.  
 */

#if 0
static void 
s3v_fallback_quad( s3vContextPtr vmesa, 
		    const s3vVertex *v0, 
		    const s3vVertex *v1, 
		    const s3vVertex *v2, 
		    const s3vVertex *v3 )
{
   GLcontext *ctx = vmesa->glCtx;
   SWvertex v[4];
   s3v_translate_vertex( ctx, v0, &v[0] );
   s3v_translate_vertex( ctx, v1, &v[1] );
   s3v_translate_vertex( ctx, v2, &v[2] );
   s3v_translate_vertex( ctx, v3, &v[3] );
   DEBUG(("s3v_fallback_quad\n"));
/*   _swrast_Quad( ctx, &v[0], &v[1], &v[2], &v[3] ); */
}

static void 
s3v_fallback_tri( s3vContextPtr vmesa, 
		    const s3vVertex *v0, 
		    const s3vVertex *v1, 
		    const s3vVertex *v2 )
{
   GLcontext *ctx = vmesa->glCtx;
   SWvertex v[3];
   s3v_translate_vertex( ctx, v0, &v[0] );
   s3v_translate_vertex( ctx, v1, &v[1] );
   s3v_translate_vertex( ctx, v2, &v[2] );
   DEBUG(("s3v_fallback_tri\n"));
/*   _swrast_Triangle( ctx, &v[0], &v[1], &v[2] ); */
}

static void
s3v_fallback_line( s3vContextPtr vmesa,
		     const s3vVertex *v0,
		     const s3vVertex *v1 )
{
   GLcontext *ctx = vmesa->glCtx;
   SWvertex v[2];
   s3v_translate_vertex( ctx, v0, &v[0] );
   s3v_translate_vertex( ctx, v1, &v[1] );
   DEBUG(("s3v_fallback_line\n"));
   _swrast_Line( ctx, &v[0], &v[1] );
}

/*
static void 
s3v_fallback_point( s3vContextPtr vmesa, 
		      const s3vVertex *v0 )
{
   GLcontext *ctx = vmesa->glCtx;
   SWvertex v[1];
   s3v_translate_vertex( ctx, v0, &v[0] );
   _swrast_Point( ctx, &v[0] );
}
*/
#endif

/***********************************************************************
 *                    Choose rasterization functions                   *
 ***********************************************************************/

#define _S3V_NEW_RASTER_STATE 	(_NEW_FOG | \
				 _NEW_TEXTURE | \
				 _DD_NEW_TRI_SMOOTH | \
				 _DD_NEW_LINE_SMOOTH | \
				 _DD_NEW_POINT_SMOOTH | \
				 _DD_NEW_TRI_STIPPLE | \
				 _DD_NEW_LINE_STIPPLE)

#define LINE_FALLBACK (0)
#define TRI_FALLBACK (0)

static void s3v_nodraw_triangle(GLcontext *ctx, s3vVertex *v0,
                                s3vVertex *v1, s3vVertex *v2)
{
	(void) (ctx && v0 && v1 && v2);
}

static void s3v_nodraw_quad(GLcontext *ctx,
                            s3vVertex *v0, s3vVertex *v1,
                            s3vVertex *v2, s3vVertex *v3)
{
	(void) (ctx && v0 && v1 && v2 && v3);
}

void s3vChooseRasterState(GLcontext *ctx);

void s3vChooseRasterState(GLcontext *ctx)
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);
	GLuint flags = ctx->_TriangleCaps;
	GLuint ind = 0;

	DEBUG(("*** s3vChooseRasterState ***\n"));

	if (ctx->Polygon.CullFlag) {
		if (ctx->Polygon.CullFaceMode == GL_FRONT_AND_BACK) {
			vmesa->draw_tri = (s3v_tri_func)s3v_nodraw_triangle;
			vmesa->draw_quad = (s3v_quad_func)s3v_nodraw_quad;
			return;
		}
		ind |= S3V_RAST_CULL_BIT;
		/* s3v_update_cullsign(ctx); */
	} /* else vmesa->backface_sign = 0; */

	if ( flags & DD_FLATSHADE )
		ind |= S3V_RAST_FLAT_BIT;

	if ( ctx->Texture.Unit[0]._ReallyEnabled ) {
		ind |= S3V_RAST_TEX_BIT;
	}

	DEBUG(("ind = %i\n", ind));

	vmesa->draw_line = s3v_line_tab[ind];
	vmesa->draw_tri = s3v_tri_tab[ind];
	vmesa->draw_quad = s3v_quad_tab[ind];
	vmesa->draw_point = s3v_point_tab[ind];

#if 0
	/* Hook in fallbacks for specific primitives.  CURRENTLY DISABLED
	 */
	
	if (flags & LINE_FALLBACK) 
		vmesa->draw_line = s3v_fallback_line;
	 
	if (flags & TRI_FALLBACK) {
		DEBUG(("TRI_FALLBACK\n"));
		vmesa->draw_tri = s3v_fallback_tri;
		vmesa->draw_quad = s3v_fallback_quad;
	}
#endif
}




/***********************************************************************
 *          Macros for t_dd_tritmp.h to draw basic primitives          *
 ***********************************************************************/

#define TRI( v0, v1, v2 ) \
do { \
	/*
	if (DO_FALLBACK) \
		vmesa->draw_tri( vmesa, v0, v1, v2 ); \
	else */ \
   	DEBUG(("TRI: max was here\n")); /* \
	s3v_draw_tex_triangle( vmesa, v0, v1, v2 ); */ \
	vmesa->draw_tri( vmesa, v0, v1, v2 ); \
} while (0)

#define QUAD( v0, v1, v2, v3 ) \
do { \
	DEBUG(("QUAD: max was here\n")); \
	vmesa->draw_quad( vmesa, v0, v1, v2, v3 ); \
} while (0)

#define LINE( v0, v1 ) \
do { \
	DEBUG(("LINE: max was here\n")); \
	vmesa->draw_line( vmesa, v0, v1 ); \
} while (0)

#define POINT( v0 ) \
do { \
	vmesa->draw_point( vmesa, v0 ); \
} while (0)


/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/

/*
#define S3V_OFFSET_BIT 	0x01
#define S3V_TWOSIDE_BIT	0x02
#define S3V_UNFILLED_BIT	0x04
#define S3V_FALLBACK_BIT	0x08
#define S3V_MAX_TRIFUNC	0x10


static struct {
   points_func		points;
   line_func		line;
   triangle_func	triangle;
   quad_func		quad;
} rast_tab[S3V_MAX_TRIFUNC];
*/

#define DO_FALLBACK  (IND & S3V_FALLBACK_BIT)
#define DO_OFFSET    (IND & S3V_OFFSET_BIT)
#define DO_UNFILLED  (IND & S3V_UNFILLED_BIT)
#define DO_TWOSIDE   (IND & S3V_TWOSIDE_BIT)
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
#define VERTEX            s3vVertex
#define TAB               rast_tab

#define DEPTH_SCALE 1.0
#define UNFILLED_TRI unfilled_tri
#define UNFILLED_QUAD unfilled_quad
#define VERT_X(_v) _v->v.x
#define VERT_Y(_v) _v->v.y
#define VERT_Z(_v) _v->v.z
#define AREA_IS_CCW( a ) (a > 0)
#define GET_VERTEX(e) (vmesa->verts + (e<<vmesa->vertex_stride_shift))

#if 0
#define VERT_SET_RGBA( v, c ) \
do { \
/*    UNCLAMPED_FLOAT_TO_RGBA_CHAN( v->ub4[4], c) */ \
} while (0)

#define VERT_COPY_RGBA( v0, v1 ) v0->ui[4] = v1->ui[4]
/*
#define VERT_COPY_RGBA1( v0, v1 ) v0->ui[4] = v1->ui[4]
*/
#define VERT_SAVE_RGBA( idx )    color[idx] = v[idx]->ui[4]
#define VERT_RESTORE_RGBA( idx ) v[idx]->ui[4] = color[idx]   
#endif

#define LOCAL_VARS(n) \
	s3vContextPtr vmesa = S3V_CONTEXT(ctx); \
	GLuint color[n]; \
	(void) color;


/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/

static const GLuint hw_prim[GL_POLYGON+1] = {
	PrimType_Points,
	PrimType_Lines,
	PrimType_Lines,
	PrimType_Lines,
	PrimType_Triangles,
	PrimType_Triangles,
	PrimType_Triangles,
	PrimType_Triangles,
	PrimType_Triangles,
	PrimType_Triangles
};

static void s3vResetLineStipple( GLcontext *ctx );
static void s3vRasterPrimitive( GLcontext *ctx, GLuint hwprim );
static void s3vRenderPrimitive( GLcontext *ctx, GLenum prim );
/*
extern static void s3v_lines_emit(GLcontext *ctx, GLuint start, GLuint end);
extern static void s3v_tris_emit(GLcontext *ctx, GLuint start, GLuint end);
*/
#define RASTERIZE(x) if (vmesa->hw_primitive != hw_prim[x]) \
                        s3vRasterPrimitive( ctx, hw_prim[x] )
#define RENDER_PRIMITIVE vmesa->render_primitive
#define TAG(x) x
#define IND S3V_FALLBACK_BIT
#include "tnl_dd/t_dd_unfilled.h"
#undef IND

/***********************************************************************
 *                      Generate GL render functions                   *
 ***********************************************************************/

#define IND (0)
#define TAG(x) x
#include "tnl_dd/t_dd_tritmp.h"

#define IND (S3V_OFFSET_BIT)
#define TAG(x) x##_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (S3V_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "tnl_dd/t_dd_tritmp.h"

#define IND (S3V_TWOSIDE_BIT|S3V_OFFSET_BIT)
#define TAG(x) x##_twoside_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (S3V_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (S3V_OFFSET_BIT|S3V_UNFILLED_BIT)
#define TAG(x) x##_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (S3V_TWOSIDE_BIT|S3V_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (S3V_TWOSIDE_BIT|S3V_OFFSET_BIT|S3V_UNFILLED_BIT)
#define TAG(x) x##_twoside_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"


static void init_render_tab( void )
{
	DEBUG(("*** init_render_tab ***\n"));

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

#define VERT(x) (s3vVertex *)(s3vverts + (x << shift))

#define RENDER_POINTS( start, count ) \
	DEBUG(("RENDER_POINTS...(ok)\n")); \
	for ( ; start < count ; start++) \
		vmesa->draw_line( vmesa, VERT(start), VERT(start) )
	/*      vmesa->draw_point( vmesa, VERT(start) ) */

#define RENDER_LINE( v0, v1 ) \
	/* DEBUG(("RENDER_LINE...(ok)\n")); \ */ \
	vmesa->draw_line( vmesa, VERT(v0), VERT(v1) ); \
	DEBUG(("RENDER_LINE...(ok)\n"))

#define RENDER_TRI( v0, v1, v2 )  \
	DEBUG(("RENDER_TRI...(ok)\n")); \
	vmesa->draw_tri( vmesa, VERT(v0), VERT(v1), VERT(v2) )

#define RENDER_QUAD( v0, v1, v2, v3 ) \
	DEBUG(("RENDER_QUAD...(ok)\n")); \
	/* s3v_draw_quad( vmesa, VERT(v0), VERT(v1), VERT(v2),VERT(v3) ) */\
	/* s3v_draw_triangle( vmesa, VERT(v0), VERT(v1), VERT(v2) ); \
	s3v_draw_triangle( vmesa, VERT(v0), VERT(v2), VERT(v3) ) */ \
	vmesa->draw_quad( vmesa, VERT(v0), VERT(v1), VERT(v2), VERT(v3) ) 
	
#define INIT(x) s3vRenderPrimitive( ctx, x );
#undef LOCAL_VARS
#define LOCAL_VARS						\
   s3vContextPtr vmesa = S3V_CONTEXT(ctx);		\
   const GLuint shift = vmesa->vertex_stride_shift;		\
   const char *s3vverts = (char *)vmesa->verts;		\
   const GLboolean stipple = ctx->Line.StippleFlag;		\
   (void) stipple;
#define RESET_STIPPLE	if ( stipple ) s3vResetLineStipple( ctx );
#define RESET_OCCLUSION
#define PRESERVE_VB_DEFS
#define ELT(x) (x)
#define TAG(x) s3v_##x##_verts
#include "tnl_dd/t_dd_rendertmp.h"


/**********************************************************************/
/*                   Render clipped primitives                        */
/**********************************************************************/

static void s3vRenderClippedPoly( GLcontext *ctx, const GLuint *elts, 
				   GLuint n )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);
	struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	GLuint prim = vmesa->render_primitive;

	DEBUG(("I AM in: s3vRenderClippedPoly\n"));

	/* Render the new vertices as an unclipped polygon. 
	 */
	if (1)
	{
	GLuint *tmp = VB->Elts;
	VB->Elts = (GLuint *)elts;
	tnl->Driver.Render.PrimTabElts[GL_POLYGON]
		( ctx, 0, n, PRIM_BEGIN|PRIM_END );

	VB->Elts = tmp;
	}

	/* Restore the render primitive
	 */
#if 1
	if (prim != GL_POLYGON) {
		DEBUG(("and prim != GL_POLYGON\n"));
		tnl->Driver.Render.PrimitiveNotify( ctx, prim );
	}
	
#endif
}

static void s3vRenderClippedLine( GLcontext *ctx, GLuint ii, GLuint jj )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   /*tnl->Driver.LineFunc = s3v_line_tab[2];*/ /* _swsetup_Line; */

   DEBUG(("I AM in: s3vRenderClippedLine\n"));
   tnl->Driver.Render.Line( ctx, ii, jj );
}


/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/



#define _S3V_NEW_RENDERSTATE (_DD_NEW_TRI_UNFILLED |		\
			       _DD_NEW_TRI_LIGHT_TWOSIDE |	\
			       _DD_NEW_TRI_OFFSET)

#define ANY_RASTER_FLAGS (DD_TRI_LIGHT_TWOSIDE|DD_TRI_OFFSET|DD_TRI_UNFILLED)

static void s3vChooseRenderState(GLcontext *ctx)
{
   s3vContextPtr vmesa = S3V_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint flags = ctx->_TriangleCaps;
   GLuint index = 0;

   DEBUG(("s3vChooseRenderState\n"));

   if (flags & ANY_RASTER_FLAGS) {
      if (flags & DD_TRI_LIGHT_TWOSIDE)       index |= S3V_TWOSIDE_BIT;
      if (flags & DD_TRI_OFFSET)	      index |= S3V_OFFSET_BIT;
      if (flags & DD_TRI_UNFILLED)	      index |= S3V_UNFILLED_BIT;
   }

   DEBUG(("vmesa->RenderIndex = %i\n", vmesa->RenderIndex));
   DEBUG(("index = %i\n", index));

   if (vmesa->RenderIndex != index) {
      vmesa->RenderIndex = index;

      tnl->Driver.Render.Points = rast_tab[index].points;
      tnl->Driver.Render.Line = rast_tab[index].line;
      tnl->Driver.Render.Triangle = rast_tab[index].triangle;
      tnl->Driver.Render.Quad = rast_tab[index].quad;

      if (vmesa->RenderIndex == 0)
         tnl->Driver.Render.PrimTabVerts = s3v_render_tab_verts;
      else
         tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
      tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
      tnl->Driver.Render.ClippedLine = s3vRenderClippedLine;
      tnl->Driver.Render.ClippedPolygon = s3vRenderClippedPoly;
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
 * performed in s3v_render.c.
 */

static void s3vRasterPrimitive( GLcontext *ctx, GLuint hwprim )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);
/*	__DRIdrawablePrivate *dPriv = vmesa->driDrawable; */
	GLuint cmd = vmesa->CMD;
	
	unsigned int _hw_prim = hwprim;

	DEBUG(("s3vRasterPrimitive: hwprim = 0x%x ", _hw_prim));

/*	printf("* vmesa->CMD = 0x%x\n", vmesa->CMD); */

	if (vmesa->hw_primitive != _hw_prim)
	{
		DEBUG(("(new one) ***\n"));
		cmd &= ~DO_MASK;
		cmd &= ~ALPHA_BLEND_MASK;
		vmesa->hw_primitive = _hw_prim;

		if (_hw_prim == PrimType_Triangles) {
		/* TRI */
	                DEBUG(("->switching to tri\n"));
			cmd |= (vmesa->_tri[vmesa->_3d_mode] | vmesa->_alpha[vmesa->_3d_mode]);
	        } else if (_hw_prim == PrimType_Lines
			|| _hw_prim == PrimType_Points) {
		/* LINE */
        	        DEBUG(("->switching to line\n"));
			cmd |= (DO_3D_LINE | vmesa->_alpha[0]);
		} else  {
		/* ugh? */
			DEBUG(("->switching to your sis'ass\n"));
		}
        
		DEBUG(("\n"));

		vmesa->restore_primitive = _hw_prim;
		/* 0xacc16827: good value -> lightened newave!!! */
		vmesa->CMD = cmd;
		CMDCHANGE();
	}
}

static void s3vRenderPrimitive( GLcontext *ctx, GLenum prim )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);
	__DRIdrawablePrivate *dPriv = vmesa->driDrawable;
	GLuint cmd = vmesa->CMD;

	unsigned int _hw_prim = hw_prim[prim];

	vmesa->render_primitive = prim;
	vmesa->hw_primitive = _hw_prim;
   
	DEBUG(("s3vRenderPrimitive #%i ", prim));
	DEBUG(("_hw_prim = 0x%x\n", _hw_prim));

/*	printf(" vmesa->CMD = 0x%x\n", vmesa->CMD); */

	if (_hw_prim != vmesa->restore_primitive) {
		DEBUG(("_hw_prim != vmesa->restore_primitive (was 0x%x)\n",
			vmesa->restore_primitive));
#if 1
		cmd &= ~DO_MASK;
		cmd &= ~ALPHA_BLEND_MASK;
/*		
		printf(" cmd = 0x%x\n", cmd);
		printf(" vmesa->_3d_mode=%i; vmesa->_tri[vmesa->_3d_mode]=0x%x\n",
			vmesa->_3d_mode, vmesa->_tri[vmesa->_3d_mode]);
		printf("vmesa->alpha[0] = 0x%x; vmesa->alpha[1] = 0x%x\n",
			vmesa->_alpha[0], vmesa->_alpha[1]);
*/		
	   	if (_hw_prim == PrimType_Triangles) { /* TRI */
			DEBUG(("->switching to tri\n"));
   			cmd |= (vmesa->_tri[vmesa->_3d_mode] | vmesa->_alpha[vmesa->_3d_mode]);
			DEBUG(("vmesa->TexStride = %i\n", vmesa->TexStride));
			DEBUG(("vmesa->TexOffset = %i\n", vmesa->TexOffset));
			DMAOUT_CHECK(3DTRI_Z_BASE, 12);
		} else { /* LINE */
			DEBUG(("->switching to line\n"));
			cmd |= (DO_3D_LINE | vmesa->_alpha[0]);
			DMAOUT_CHECK(3DLINE_Z_BASE, 12);
		}

		DMAOUT(vmesa->s3vScreen->depthOffset & 0x003FFFF8);
		DMAOUT(vmesa->DestBase);
		/* DMAOUT(vmesa->ScissorLR); */
		/* DMAOUT(vmesa->ScissorTB); */

		/* NOTE: we need to restore all these values since we
		 * are coming back from a vmesa->restore_primitive */
		DMAOUT( (0 << 16) | (dPriv->w-1) );
		DMAOUT( (0 << 16) | (dPriv->h-1) );
		DMAOUT( (vmesa->SrcStride << 16) | vmesa->TexStride );
		DMAOUT(vmesa->SrcStride);
		DMAOUT(vmesa->TexOffset);
		DMAOUT(vmesa->TextureBorderColor);
		DMAOUT(0); /* FOG */
		DMAOUT(0);
		DMAOUT(0);
	        DMAOUT(cmd);
		/* 0xacc16827: good value -> lightened newave!!! */
        DMAFINISH();

	vmesa->CMD = cmd;
#endif
	}

	DEBUG(("\n"));

	vmesa->restore_primitive = _hw_prim;
}

static void s3vRunPipeline( GLcontext *ctx )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);

	DEBUG(("*** s3vRunPipeline ***\n"));

	if ( vmesa->new_state )
		s3vDDUpdateHWState( ctx );

	if (vmesa->new_gl_state) {

		if (vmesa->new_gl_state & _NEW_TEXTURE) {
			s3vUpdateTextureState( ctx );
		}

		if (!vmesa->Fallback) {
			if (vmesa->new_gl_state & _S3V_NEW_VERTEX)
				s3vChooseVertexState( ctx );
      
			if (vmesa->new_gl_state & _S3V_NEW_RASTER_STATE)
				s3vChooseRasterState( ctx );
      
			if (vmesa->new_gl_state & _S3V_NEW_RENDERSTATE)
				s3vChooseRenderState( ctx );
		}

	vmesa->new_gl_state = 0;
	
	}

	_tnl_run_pipeline( ctx );
}

static void s3vRenderStart( GLcontext *ctx )
{
	/* Check for projective texturing.  Make sure all texcoord
	 * pointers point to something.  (fix in mesa?)  
	 */

	DEBUG(("s3vRenderStart\n"));
	/* s3vCheckTexSizes( ctx ); */
}

static void s3vRenderFinish( GLcontext *ctx )
{
   if (0)
      _swrast_flush( ctx );	/* never needed */
}

static void s3vResetLineStipple( GLcontext *ctx )
{
/*   s3vContextPtr vmesa = S3V_CONTEXT(ctx); */

   /* Reset the hardware stipple counter.
    */
/*
   CHECK_DMA_BUFFER(vmesa, 1);
   WRITE(vmesa->buf, UpdateLineStippleCounters, 0);
*/
}


/**********************************************************************/
/*           Transition to/from hardware rasterization.               */
/**********************************************************************/


void s3vFallback( s3vContextPtr vmesa, GLuint bit, GLboolean mode )
{
   GLcontext *ctx = vmesa->glCtx;
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint oldfallback = vmesa->Fallback;

   DEBUG(("*** s3vFallback: "));

   if (mode) {
      vmesa->Fallback |= bit;
      if (oldfallback == 0) {
		 DEBUG(("oldfallback == 0 ***\n"));
		 _swsetup_Wakeup( ctx );
		 _tnl_need_projected_coords( ctx, GL_TRUE );
		 vmesa->RenderIndex = ~0;
      }
   }
   else {
      DEBUG(("***\n"));
      vmesa->Fallback &= ~bit;
      if (oldfallback == bit) {
	 	_swrast_flush( ctx );
	 	tnl->Driver.Render.Start = s3vRenderStart;
	 	tnl->Driver.Render.PrimitiveNotify = s3vRenderPrimitive;
	 	tnl->Driver.Render.Finish = s3vRenderFinish;
	 	tnl->Driver.Render.BuildVertices = s3vBuildVertices;
   		tnl->Driver.Render.ResetLineStipple = s3vResetLineStipple;
	 	vmesa->new_gl_state |= (_S3V_NEW_RENDERSTATE|
					_S3V_NEW_RASTER_STATE|
					_S3V_NEW_VERTEX);
      }
   }
}


/**********************************************************************/
/*                            Initialization.                         */
/**********************************************************************/


void s3vInitTriFuncs( GLcontext *ctx )
{
   s3vContextPtr vmesa = S3V_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   static int firsttime = 1;

   if (firsttime) {
      init_rast_tab();
      init_render_tab();
      firsttime = 0;
   }
   
   vmesa->RenderIndex = ~0;

   tnl->Driver.RunPipeline = s3vRunPipeline;
   tnl->Driver.Render.Start = s3vRenderStart;
   tnl->Driver.Render.Finish = s3vRenderFinish; 
   tnl->Driver.Render.PrimitiveNotify = s3vRenderPrimitive;
   tnl->Driver.Render.ResetLineStipple = s3vResetLineStipple;
/*
   tnl->Driver.RenderInterp = _swsetup_RenderInterp;
   tnl->Driver.RenderCopyPV = _swsetup_RenderCopyPV;
*/
   tnl->Driver.Render.BuildVertices = s3vBuildVertices;
}
