/* $XFree86: xc/lib/GL/mesa/src/drv/i830/i830_tris.c,v 1.4 2002/12/10 01:26:54 dawes Exp $ */
/**************************************************************************

Copyright 2001 VA Linux Systems Inc., Fremont, California.

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
 * Original Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 * Adapted for use on the I830M:
 *   Jeff Hartmann <jhartmann@2d3d.com>
 */

#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "enums.h"
#include "dd.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#include "i830_screen.h"
#include "i830_dri.h"

#include "i830_tris.h"
#include "i830_state.h"
#include "i830_ioctl.h"
#include "i830_span.h"

static void i830RenderPrimitive( GLcontext *ctx, GLenum prim );

/***********************************************************************
 *                    Emit primitives as inline vertices               *
 ***********************************************************************/

#if defined(USE_X86_ASM)
#define COPY_DWORDS( j, vb, vertsize, v )				\
do {									\
	int __tmp;							\
	__asm__ __volatile__( "rep ; movsl"				\
			      : "=%c" (j), "=D" (vb), "=S" (__tmp)	\
			      : "0" (vertsize),				\
			        "D" ((long)vb),				\
			        "S" ((long)v) );			\
} while (0)
#else
#define COPY_DWORDS( j, vb, vertsize, v )				\
do {									\
   for ( j = 0 ; j < vertsize ; j++ )					\
      vb[j] = ((GLuint *)v)[j];						\
   vb += vertsize;							\
} while (0)
#endif

static void __inline__ i830_draw_triangle( i830ContextPtr imesa,
					   i830VertexPtr v0,
					   i830VertexPtr v1,
					   i830VertexPtr v2 )
{
   GLuint vertsize = imesa->vertex_size;
   GLuint *vb = i830AllocDmaLow( imesa, 3 * 4 * vertsize );
   int j;

   COPY_DWORDS( j, vb, vertsize, v0 );
   COPY_DWORDS( j, vb, vertsize, v1 );
   COPY_DWORDS( j, vb, vertsize, v2 );
}


static void __inline__ i830_draw_quad( i830ContextPtr imesa,
				       i830VertexPtr v0,
				       i830VertexPtr v1,
				       i830VertexPtr v2,
				       i830VertexPtr v3 )
{
   GLuint vertsize = imesa->vertex_size;
   GLuint *vb = i830AllocDmaLow( imesa, 6 * 4 * vertsize );
   int j;

   COPY_DWORDS( j, vb, vertsize, v0 );
   COPY_DWORDS( j, vb, vertsize, v1 );
   COPY_DWORDS( j, vb, vertsize, v3 );
   COPY_DWORDS( j, vb, vertsize, v1 );
   COPY_DWORDS( j, vb, vertsize, v2 );
   COPY_DWORDS( j, vb, vertsize, v3 );
}


static __inline__ void i830_draw_point( i830ContextPtr imesa,
					i830VertexPtr tmp )
{
   GLuint vertsize = imesa->vertex_size;
   GLuint *vb = i830AllocDmaLow( imesa, 4 * vertsize );
   int j;

   /* Adjust for sub pixel position */
   *(float *)&vb[0] = tmp->v.x - 0.125;
   *(float *)&vb[1] = tmp->v.y - 0.125;
   for (j = 2 ; j < vertsize ; j++)
     vb[j] = tmp->ui[j];
}


static __inline__ void i830_draw_line( i830ContextPtr imesa,
				       i830VertexPtr v0,
				       i830VertexPtr v1 )
{
   GLuint vertsize = imesa->vertex_size;
   GLuint *vb = i830AllocDmaLow( imesa, 2 * 4 * vertsize );
   int j;

   COPY_DWORDS( j, vb, vertsize, v0 );
   COPY_DWORDS( j, vb, vertsize, v1 );
}



/***********************************************************************
 *          Macros for t_dd_tritmp.h to draw basic primitives          *
 ***********************************************************************/

#define TRI( a, b, c )				\
do { 						\
   if (DO_FALLBACK)				\
      imesa->draw_tri( imesa, a, b, c );	\
   else						\
      i830_draw_triangle( imesa, a, b, c );	\
} while (0)

#define QUAD( a, b, c, d )			\
do { 						\
   if (DO_FALLBACK) {				\
      imesa->draw_tri( imesa, a, b, d );	\
      imesa->draw_tri( imesa, b, c, d );	\
   } else					\
      i830_draw_quad( imesa, a, b, c, d );	\
} while (0)

#define LINE( v0, v1 )				\
do { 						\
   if (DO_FALLBACK)				\
      imesa->draw_line( imesa, v0, v1 );	\
   else						\
      i830_draw_line( imesa, v0, v1 );		\
} while (0)

#define POINT( v0 )				\
do { 						\
   if (DO_FALLBACK)				\
      imesa->draw_point( imesa, v0 );		\
   else						\
      i830_draw_point( imesa, v0 );		\
} while (0)


/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/

#define I830_OFFSET_BIT 	0x01
#define I830_TWOSIDE_BIT	0x02
#define I830_UNFILLED_BIT	0x04
#define I830_FALLBACK_BIT	0x08
#define I830_MAX_TRIFUNC	0x10


static struct {
   tnl_points_func	        points;
   tnl_line_func		line;
   tnl_triangle_func	triangle;
   tnl_quad_func		quad;
} rast_tab[I830_MAX_TRIFUNC];


#define DO_FALLBACK (IND & I830_FALLBACK_BIT)
#define DO_OFFSET   (IND & I830_OFFSET_BIT)
#define DO_UNFILLED (IND & I830_UNFILLED_BIT)
#define DO_TWOSIDE  (IND & I830_TWOSIDE_BIT)
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
#define VERTEX            i830Vertex
#define TAB               rast_tab

#define DEPTH_SCALE (imesa->depth_scale)
#define UNFILLED_TRI unfilled_tri
#define UNFILLED_QUAD unfilled_quad
#define VERT_X(_v) _v->v.x
#define VERT_Y(_v) _v->v.y
#define VERT_Z(_v) _v->v.z
#define AREA_IS_CCW( a ) (a > 0)
#define GET_VERTEX(e) (imesa->verts + (e * imesa->vertex_size * sizeof(int)))

#define VERT_SET_RGBA( v, c )  					\
do {								\
   i830_color_t *color = (i830_color_t *)&((v)->ui[coloroffset]);	\
   UNCLAMPED_FLOAT_TO_UBYTE(color->red, (c)[0]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->green, (c)[1]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->blue, (c)[2]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->alpha, (c)[3]);		\
} while (0)

#define VERT_COPY_RGBA( v0, v1 ) v0->ui[coloroffset] = v1->ui[coloroffset]

#define VERT_SET_SPEC( v0, c )					\
do {								\
   if (havespec) {						\
      UNCLAMPED_FLOAT_TO_UBYTE(v0->v.specular.red, (c)[0]);	\
      UNCLAMPED_FLOAT_TO_UBYTE(v0->v.specular.green, (c)[1]);	\
      UNCLAMPED_FLOAT_TO_UBYTE(v0->v.specular.blue, (c)[2]);	\
   }								\
} while (0)
#define VERT_COPY_SPEC( v0, v1 )			\
do {							\
   if (havespec) {					\
      v0->v.specular.red   = v1->v.specular.red;	\
      v0->v.specular.green = v1->v.specular.green;	\
      v0->v.specular.blue  = v1->v.specular.blue; 	\
   }							\
} while (0)

#define VERT_SAVE_RGBA( idx )    color[idx] = v[idx]->ui[coloroffset]
#define VERT_RESTORE_RGBA( idx ) v[idx]->ui[coloroffset] = color[idx]
#define VERT_SAVE_SPEC( idx )    if (havespec) spec[idx] = v[idx]->ui[5]
#define VERT_RESTORE_SPEC( idx ) if (havespec) v[idx]->ui[5] = spec[idx]

#define LOCAL_VARS(n)							\
   i830ContextPtr imesa = I830_CONTEXT(ctx);				\
   GLuint color[n], spec[n];						\
   GLuint coloroffset = (imesa->vertex_size == 4 ? 3 : 4);		\
   GLboolean havespec = (imesa->vertex_size > 4);			\
   (void) color; (void) spec; (void) coloroffset; (void) havespec;


/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/

static const GLuint hw_prim[GL_POLYGON+1] = {
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

#define RASTERIZE(x) if (imesa->hw_primitive != hw_prim[x]) \
                        i830RasterPrimitive( ctx, x, hw_prim[x] )
#define RENDER_PRIMITIVE imesa->render_primitive
#define TAG(x) x
#define IND I830_FALLBACK_BIT
#include "tnl_dd/t_dd_unfilled.h"
#undef IND

/***********************************************************************
 *                      Generate GL render functions                   *
 ***********************************************************************/

#define IND (0)
#define TAG(x) x
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I830_OFFSET_BIT)
#define TAG(x) x##_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I830_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I830_TWOSIDE_BIT|I830_OFFSET_BIT)
#define TAG(x) x##_twoside_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I830_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I830_OFFSET_BIT|I830_UNFILLED_BIT)
#define TAG(x) x##_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I830_TWOSIDE_BIT|I830_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I830_TWOSIDE_BIT|I830_OFFSET_BIT|I830_UNFILLED_BIT)
#define TAG(x) x##_twoside_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I830_FALLBACK_BIT)
#define TAG(x) x##_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I830_OFFSET_BIT|I830_FALLBACK_BIT)
#define TAG(x) x##_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I830_TWOSIDE_BIT|I830_FALLBACK_BIT)
#define TAG(x) x##_twoside_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I830_TWOSIDE_BIT|I830_OFFSET_BIT|I830_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I830_UNFILLED_BIT|I830_FALLBACK_BIT)
#define TAG(x) x##_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I830_OFFSET_BIT|I830_UNFILLED_BIT|I830_FALLBACK_BIT)
#define TAG(x) x##_offset_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I830_TWOSIDE_BIT|I830_UNFILLED_BIT|I830_FALLBACK_BIT)
#define TAG(x) x##_twoside_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I830_TWOSIDE_BIT|I830_OFFSET_BIT|I830_UNFILLED_BIT| \
	     I830_FALLBACK_BIT)
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
i830_fallback_tri( i830ContextPtr imesa,
		   i830Vertex *v0,
		   i830Vertex *v1,
		   i830Vertex *v2 )
{
   GLcontext *ctx = imesa->glCtx;
   SWvertex v[3];

   if (0)
      fprintf(stderr, "\n%s\n", __FUNCTION__);

   _swsetup_Translate( ctx, v0, &v[0] );
   _swsetup_Translate( ctx, v1, &v[1] );
   _swsetup_Translate( ctx, v2, &v[2] );
   i830SpanRenderStart( ctx );
   _swrast_Triangle( ctx, &v[0], &v[1], &v[2] );
   i830SpanRenderFinish( ctx );
}


static void
i830_fallback_line( i830ContextPtr imesa,
		    i830Vertex *v0,
		    i830Vertex *v1 )
{
   GLcontext *ctx = imesa->glCtx;
   SWvertex v[2];

   if (0)
      fprintf(stderr, "\n%s\n", __FUNCTION__);

   _swsetup_Translate( ctx, v0, &v[0] );
   _swsetup_Translate( ctx, v1, &v[1] );
   i830SpanRenderStart( ctx );
   _swrast_Line( ctx, &v[0], &v[1] );
   i830SpanRenderFinish( ctx );
}


static void
i830_fallback_point( i830ContextPtr imesa,
		     i830Vertex *v0 )
{
   GLcontext *ctx = imesa->glCtx;
   SWvertex v[1];

   if (0)
      fprintf(stderr, "\n%s\n", __FUNCTION__);

   _swsetup_Translate( ctx, v0, &v[0] );
   i830SpanRenderStart( ctx );
   _swrast_Point( ctx, &v[0] );
   i830SpanRenderFinish( ctx );
}



/**********************************************************************/
/*               Render unclipped begin/end objects                   */
/**********************************************************************/

#define IND 0
#define V(x) (i830Vertex *)(vertptr + ((x) * vertsize * sizeof(int)))
#define RENDER_POINTS( start, count )	\
   for ( ; start < count ; start++) POINT( V(ELT(start)) );
#define RENDER_LINE( v0, v1 )         LINE( V(v0), V(v1) )
#define RENDER_TRI(  v0, v1, v2 )     TRI(  V(v0), V(v1), V(v2) )
#define RENDER_QUAD( v0, v1, v2, v3 ) QUAD( V(v0), V(v1), V(v2), V(v3) )
#define INIT(x) i830RenderPrimitive( ctx, x )
#undef LOCAL_VARS
#define LOCAL_VARS						\
    i830ContextPtr imesa = I830_CONTEXT(ctx);			\
    GLubyte *vertptr = (GLubyte *)imesa->verts;			\
    const GLuint vertsize = imesa->vertex_size;       	\
    const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;	\
    (void) elt;
#define RESET_STIPPLE
#define RESET_OCCLUSION
#define PRESERVE_VB_DEFS
#define ELT(x) x
#define TAG(x) i830_##x##_verts
#include "tnl/t_vb_rendertmp.h"
#undef ELT
#undef TAG
#define TAG(x) i830_##x##_elts
#define ELT(x) elt[x]
#include "tnl/t_vb_rendertmp.h"

/**********************************************************************/
/*                   Render clipped primitives                        */
/**********************************************************************/



static void i830RenderClippedPoly( GLcontext *ctx, const GLuint *elts,
				   GLuint n )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLuint prim = imesa->render_primitive;

   /* Render the new vertices as an unclipped polygon.
    */
   {
      GLuint *tmp = VB->Elts;
      VB->Elts = (GLuint *)elts;
      tnl->Driver.Render.PrimTabElts[GL_POLYGON]( ctx, 0, n, 
						  PRIM_BEGIN|PRIM_END );
      VB->Elts = tmp;
   }

   /* Restore the render primitive
    */
   if (prim != GL_POLYGON)
      tnl->Driver.Render.PrimitiveNotify( ctx, prim );
}

static void i830RenderClippedLine( GLcontext *ctx, GLuint ii, GLuint jj )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   tnl->Driver.Render.Line( ctx, ii, jj );
}

static void i830FastRenderClippedPoly( GLcontext *ctx, const GLuint *elts,
				       GLuint n )
{
   i830ContextPtr imesa = I830_CONTEXT( ctx );
   GLuint vertsize = imesa->vertex_size;
   GLuint *vb = i830AllocDmaLow( imesa, (n-2) * 3 * 4 * vertsize );
   GLubyte *vertptr = (GLubyte *)imesa->verts;
   const GLuint *start = (const GLuint *)V(elts[0]);
   int i,j;

   for (i = 2 ; i < n ; i++) {
      COPY_DWORDS( j, vb, vertsize, V(elts[i-1]) );
      COPY_DWORDS( j, vb, vertsize, V(elts[i]) );
      COPY_DWORDS( j, vb, vertsize, start );
   }
}

/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/



#define _I830_NEW_RENDERSTATE (_DD_NEW_LINE_STIPPLE |		\
			       _DD_NEW_TRI_UNFILLED |		\
			       _DD_NEW_TRI_LIGHT_TWOSIDE |	\
			       _DD_NEW_TRI_OFFSET |		\
			       _DD_NEW_TRI_STIPPLE |		\
			       _NEW_POLYGONSTIPPLE)

#define POINT_FALLBACK (0)
#define LINE_FALLBACK (DD_LINE_STIPPLE)
#define TRI_FALLBACK (0)
#define ANY_FALLBACK_FLAGS (POINT_FALLBACK|LINE_FALLBACK|TRI_FALLBACK|\
                            DD_TRI_STIPPLE)
#define ANY_RASTER_FLAGS (DD_TRI_LIGHT_TWOSIDE|DD_TRI_OFFSET|DD_TRI_UNFILLED)

static void i830ChooseRenderState(GLcontext *ctx)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   GLuint flags = ctx->_TriangleCaps;
   GLuint index = 0;

   if (I830_DEBUG & DEBUG_STATE)
     fprintf(stderr,"\n%s\n",__FUNCTION__);

   if (flags & (ANY_FALLBACK_FLAGS|ANY_RASTER_FLAGS)) {
      if (flags & ANY_RASTER_FLAGS) {
	 if (flags & DD_TRI_LIGHT_TWOSIDE)    index |= I830_TWOSIDE_BIT;
	 if (flags & DD_TRI_OFFSET)	      index |= I830_OFFSET_BIT;
	 if (flags & DD_TRI_UNFILLED)	      index |= I830_UNFILLED_BIT;
      }

      imesa->draw_point = i830_draw_point;
      imesa->draw_line = i830_draw_line;
      imesa->draw_tri = i830_draw_triangle;

      /* Hook in fallbacks for specific primitives.
       */
      if (flags & ANY_FALLBACK_FLAGS)
      {
	 if (flags & POINT_FALLBACK)
	    imesa->draw_point = i830_fallback_point;

	 if (flags & LINE_FALLBACK)
	    imesa->draw_line = i830_fallback_line;

	 if (flags & TRI_FALLBACK)
	    imesa->draw_tri = i830_fallback_tri;

	 if ((flags & DD_TRI_STIPPLE) && !imesa->hw_stipple) {
	    imesa->draw_tri = i830_fallback_tri;
	 }

	 index |= I830_FALLBACK_BIT;
      }
   }

   if (imesa->RenderIndex != index) {
      imesa->RenderIndex = index;

      tnl->Driver.Render.Points = rast_tab[index].points;
      tnl->Driver.Render.Line = rast_tab[index].line;
      tnl->Driver.Render.Triangle = rast_tab[index].triangle;
      tnl->Driver.Render.Quad = rast_tab[index].quad;

      if (index == 0) {
	 tnl->Driver.Render.PrimTabVerts = i830_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = i830_render_tab_elts;
	 tnl->Driver.Render.ClippedLine = line; /* from tritmp.h */
	 tnl->Driver.Render.ClippedPolygon = i830FastRenderClippedPoly;
      } else {
	 tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
	 tnl->Driver.Render.ClippedLine = i830RenderClippedLine;
	 tnl->Driver.Render.ClippedPolygon = i830RenderClippedPoly;
      }
   }
}

static const GLenum reduced_prim[GL_POLYGON+1] = {
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



/* Determine the rasterized primitive when not drawing unfilled
 * polygons.
 *
 * Used only for the default render stage which always decomposes
 * primitives to trianges/lines/points.  For the accelerated stage,
 * which renders strips as strips, the equivalent calculations are
 * performed in i810render.c.
 */
static void i830RenderPrimitive( GLcontext *ctx, GLenum prim )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   GLuint rprim = reduced_prim[prim];

   imesa->render_primitive = prim;

   if (rprim == GL_TRIANGLES && (ctx->_TriangleCaps & DD_TRI_UNFILLED))
      return;

   if (imesa->reduced_primitive != rprim ||
       hw_prim[prim] != imesa->hw_primitive) {
      i830RasterPrimitive( ctx, rprim, hw_prim[prim] );
   }
}

static void i830RunPipeline( GLcontext *ctx )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);

   if (imesa->NewGLState) {
      if (imesa->NewGLState & _NEW_TEXTURE) {
	 I830_FIREVERTICES( imesa );
	 i830UpdateTextureState( ctx ); /* may modify imesa->NewGLState */
      }

      if (!imesa->Fallback) {
	 if (imesa->NewGLState & _I830_NEW_RENDERSTATE)
	    i830ChooseRenderState( ctx );
      }

      imesa->NewGLState = 0;
   }

   _tnl_run_pipeline( ctx );
}


#define TEXCOORDTYPE_MASK		(3<<11)



static void set_projective_texturing( i830ContextPtr imesa, 
				      GLuint i,
				      GLuint mcs)
{
   mcs |= (imesa->CurrentTexObj[i]->Setup[I830_TEXREG_MCS] & 
	   ~TEXCOORDTYPE_MASK);

   if (mcs != imesa->CurrentTexObj[i]->Setup[I830_TEXREG_MCS]) {
      I830_STATECHANGE(imesa, I830_UPLOAD_TEX_N(i));
      imesa->CurrentTexObj[i]->Setup[I830_TEXREG_MCS] = mcs;
   }
}


#define SZ_TO_HW(sz)  ((sz-2)&0x3)
#define EMIT_SZ(sz)   (EMIT_1F + (sz) - 1)
#define EMIT_ATTR( ATTR, STYLE, V0 )					\
do {									\
   imesa->vertex_attrs[imesa->vertex_attr_count].attrib = (ATTR);	\
   imesa->vertex_attrs[imesa->vertex_attr_count].format = (STYLE);	\
   imesa->vertex_attr_count++;						\
   v0 |= V0;								\
} while (0)

#define EMIT_PAD( N )							\
do {									\
   imesa->vertex_attrs[imesa->vertex_attr_count].attrib = 0;		\
   imesa->vertex_attrs[imesa->vertex_attr_count].format = EMIT_PAD;	\
   imesa->vertex_attrs[imesa->vertex_attr_count].offset = (N);		\
   imesa->vertex_attr_count++;						\
} while (0)

#define VRTX_TEX_SET_FMT(n, x)          ((x)<<((n)*2))

/* Make sure hardware vertex format is appropriate for VB state.
 */
static void i830RenderStart( GLcontext *ctx )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint index = tnl->render_inputs;
   GLuint v0 = STATE3D_VERTEX_FORMAT_CMD;
   GLuint v2 = STATE3D_VERTEX_FORMAT_2_CMD;
   GLuint force_emit = 0;

   /* Important:
    */
   VB->AttribPtr[VERT_ATTRIB_POS] = VB->NdcPtr;
   imesa->vertex_attr_count = 0;

   /* EMIT_ATTR's must be in order as they tell t_vertex.c how to
    * build up a hardware vertex.
    */
   if (index & _TNL_BITS_TEX_ANY) {
      EMIT_ATTR( _TNL_ATTRIB_POS, EMIT_4F_VIEWPORT, VRTX_HAS_XYZW );
   }
   else {
      EMIT_ATTR( _TNL_ATTRIB_POS, EMIT_3F_VIEWPORT, VRTX_HAS_XYZ );
   }

   EMIT_ATTR( _TNL_ATTRIB_COLOR0, EMIT_4UB_4F_BGRA, VRTX_HAS_DIFFUSE );
      
   if (index & (_TNL_BIT_COLOR1|_TNL_BIT_FOG)) {

      if (index & _TNL_BIT_COLOR1) 
      {
         if (imesa->vertex_attrs[imesa->vertex_attr_count].format != EMIT_3UB_3F_BGR)
            force_emit=1;
	 EMIT_ATTR( _TNL_ATTRIB_COLOR1, EMIT_3UB_3F_BGR, VRTX_HAS_SPEC );
      }
      else 
      {
         if (imesa->vertex_attrs[imesa->vertex_attr_count].format != EMIT_PAD)
            force_emit=1;
	 EMIT_PAD( 3 );
      }
      if (index & _TNL_BIT_FOG) 
      {
         if (imesa->vertex_attrs[imesa->vertex_attr_count].format != EMIT_1UB_1F)
            force_emit=1;
	 EMIT_ATTR( _TNL_ATTRIB_FOG, EMIT_1UB_1F, VRTX_HAS_SPEC );
      }
      else 
      {
         if (imesa->vertex_attrs[imesa->vertex_attr_count].format != EMIT_PAD)
            force_emit=1;
	 EMIT_PAD( 1 );
      }
   }

   if (index & _TNL_BITS_TEX_ANY) {
      int i, last_stage = 0;

      for (i = 0; i < ctx->Const.MaxTextureUnits ; i++)
	 if (index & _TNL_BIT_TEX(i))
	    last_stage = i+1;
	 

      for (i = 0; i < last_stage; i++) {
	 GLuint sz = VB->TexCoordPtr[i]->size;
	 GLuint emit;
	 GLuint mcs;

	 /* i830 doesn't like 1D or 4D texcoords:
	  */
	 switch (sz) {
	 case 1: 
	 case 2: 
	 case 3: 		/* no attempt at cube texturing so far */
	    emit = EMIT_2F; 
	    sz = 2; 
	    mcs = TEXCOORDTYPE_CARTESIAN; 
	    break;
	 case 4: 
	    emit = EMIT_3F_XYW; 
	    sz = 3;     
	    mcs = TEXCOORDTYPE_HOMOGENEOUS;
	    break;
	 default: 
	    continue;
	 };
	
	 v2 |= VRTX_TEX_SET_FMT(i, SZ_TO_HW(sz));
	 EMIT_ATTR( _TNL_ATTRIB_TEX0+i, EMIT_SZ(sz), 0 );

	 if (imesa->CurrentTexObj[i]) 
	    set_projective_texturing( imesa, i, mcs );
      }

      v0 |= VRTX_TEX_COORD_COUNT(last_stage);
   }

   /* Only need to change the vertex emit code if there has been a
    * statechange to a new hardware vertex format:
    */
   if (v0 != imesa->Setup[I830_CTXREG_VF] ||
       v2 != imesa->Setup[I830_CTXREG_VF2] ||
	force_emit == 1) {
    
      I830_STATECHANGE( imesa, I830_UPLOAD_CTX );

      /* Must do this *after* statechange, so as not to affect
       * buffered vertices reliant on the old state:
       */
      imesa->vertex_size = 
	 _tnl_install_attrs( ctx, 
			     imesa->vertex_attrs, 
			     imesa->vertex_attr_count,
			     imesa->ViewportMatrix.m, 0 );

      imesa->vertex_size >>= 2;

      imesa->Setup[I830_CTXREG_VF] = v0;
      imesa->Setup[I830_CTXREG_VF2] = v2;
   }
}


static void i830RenderFinish( GLcontext *ctx )
{
   if (I830_CONTEXT(ctx)->RenderIndex & I830_FALLBACK_BIT)
      _swrast_flush( ctx );
}




/* System to flush dma and emit state changes based on the rasterized
 * primitive.
 */
void i830RasterPrimitive( GLcontext *ctx,
			  GLenum rprim,
			  GLuint hwprim )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   GLuint aa = imesa->Setup[I830_CTXREG_AA];
   GLuint st1 = imesa->StippleSetup[I830_STPREG_ST1];

   aa &= ~AA_LINE_ENABLE;

   if (I830_DEBUG & DEBUG_PRIMS) {
      /* Prints reduced prim, and hw prim */
      char *prim_name = "Unknown";

      switch(hwprim) {
      case PRIM3D_POINTLIST:
	 prim_name = "PointList";
	 break;
      case PRIM3D_LINELIST:
	 prim_name = "LineList";
	 break;
      case PRIM3D_LINESTRIP:
	 prim_name = "LineStrip";
	 break;	 
      case PRIM3D_TRILIST:
	 prim_name = "TriList";
	 break;	 
      case PRIM3D_TRISTRIP:
	 prim_name = "TriStrip";
	 break;	 
      case PRIM3D_TRIFAN:
	 prim_name = "TriFan";
	 break;	 
      case PRIM3D_POLY:
	 prim_name = "Polygons";
	 break;
      default:
	 break;
      }

      fprintf(stderr, "%s : rprim(%s), hwprim(%s)\n",
	      __FUNCTION__,
	      _mesa_lookup_enum_by_nr(rprim),
	      prim_name);
   }

   switch (rprim) {
   case GL_TRIANGLES:
      aa |= AA_LINE_DISABLE;
      if (ctx->Polygon.StippleFlag)
	 st1 |= ST1_ENABLE;
      else
	 st1 &= ~ST1_ENABLE;
      break;
   case GL_LINES:
      st1 &= ~ST1_ENABLE;
      if (ctx->Line.SmoothFlag) {
	 aa |= AA_LINE_ENABLE;
      } else {
	 aa |= AA_LINE_DISABLE;
      }
      break;
   case GL_POINTS:
      st1 &= ~ST1_ENABLE;
      aa |= AA_LINE_DISABLE;
      break;
   default:
      return;
   }

   imesa->reduced_primitive = rprim;

   if (aa != imesa->Setup[I830_CTXREG_AA]) {
      I830_STATECHANGE(imesa, I830_UPLOAD_CTX);
      imesa->Setup[I830_CTXREG_AA] = aa;
   }
   
#if 0
   if (st1 != imesa->StippleSetup[I830_STPREG_ST1]) {
      I830_STATECHANGE(imesa, I830_UPLOAD_STIPPLE);
      imesa->StippleSetup[I830_STPREG_ST1] = st1;
   }
#endif

   if (hwprim != imesa->hw_primitive) {
      I830_STATECHANGE(imesa, 0);
      imesa->hw_primitive = hwprim;
   }
}

/**********************************************************************/
/*           Transition to/from hardware rasterization.               */
/**********************************************************************/

static char *fallbackStrings[] = {
   "Texture",
   "Draw buffer",
   "Read buffer",
   "Color mask",
   "Render mode",
   "Stencil",
   "Stipple",
   "User disable"
};


static char *getFallbackString(GLuint bit)
{
   int i = 0;
   while (bit > 1) {
      i++;
      bit >>= 1;
   }
   return fallbackStrings[i];
}



void i830Fallback( i830ContextPtr imesa, GLuint bit, GLboolean mode )
{
   GLcontext *ctx = imesa->glCtx;
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint oldfallback = imesa->Fallback;

   if (mode) {
      imesa->Fallback |= bit;
      if (oldfallback == 0) {
	 I830_FIREVERTICES(imesa);
	 if (I830_DEBUG & DEBUG_FALLBACKS) 
	    fprintf(stderr, "ENTER FALLBACK %s\n", getFallbackString( bit ));
	 _swsetup_Wakeup( ctx );
	 imesa->RenderIndex = ~0;
      }
   }
   else {
      imesa->Fallback &= ~bit;
      if (oldfallback == bit) {
	 _swrast_flush( ctx );
	 if (I830_DEBUG & DEBUG_FALLBACKS) 
	    fprintf(stderr, "LEAVE FALLBACK %s\n", getFallbackString( bit ));
	 tnl->Driver.Render.Start = i830RenderStart;
	 tnl->Driver.Render.PrimitiveNotify = i830RenderPrimitive;
	 tnl->Driver.Render.Finish = i830RenderFinish;

	 tnl->Driver.Render.BuildVertices = _tnl_build_vertices;
	 tnl->Driver.Render.CopyPV = _tnl_copy_pv;
	 tnl->Driver.Render.Interp = _tnl_interp;

	 _tnl_invalidate_vertex_state( ctx, ~0 );
	 _tnl_invalidate_vertices( ctx, ~0 );
	 _tnl_install_attrs( ctx, 
			     imesa->vertex_attrs, 
			     imesa->vertex_attr_count,
			     imesa->ViewportMatrix.m, 0 ); 

	 imesa->NewGLState |= _I830_NEW_RENDERSTATE;
      }
   }
}




/**********************************************************************/
/*                            Initialization.                         */
/**********************************************************************/

/**
 * \bug
 * How are the magic numbers 12 and 26 in the call to \c _tnl_init_vertices
 * derived?
 */
void i830InitTriFuncs( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   static int firsttime = 1;

   if (firsttime) {
      init_rast_tab();
      firsttime = 0;
   }

   tnl->Driver.RunPipeline = i830RunPipeline;
   tnl->Driver.Render.Start = i830RenderStart;
   tnl->Driver.Render.Finish = i830RenderFinish;
   tnl->Driver.Render.PrimitiveNotify = i830RenderPrimitive;
   tnl->Driver.Render.ResetLineStipple = _swrast_ResetLineStipple;
   tnl->Driver.Render.BuildVertices = _tnl_build_vertices;
   tnl->Driver.Render.CopyPV = _tnl_copy_pv;
   tnl->Driver.Render.Interp = _tnl_interp;

   _tnl_init_vertices( ctx, ctx->Const.MaxArrayLockSize + 12, 
		       26 * sizeof(GLfloat) );
   
   I830_CONTEXT(ctx)->verts = (char *)tnl->clipspace.vertex_buf;
}
