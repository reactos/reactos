/* $XFree86: xc/lib/GL/mesa/src/drv/i810/i810tris.c,v 1.7 2002/10/30 12:51:33 alanh Exp $ */
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
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "mtypes.h"
#include "macros.h"
#include "enums.h"
#include "colormac.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#include "i810screen.h"
#include "i810_dri.h"

#include "i810tris.h"
#include "i810state.h"
#include "i810vb.h"
#include "i810ioctl.h"

static void i810RenderPrimitive( GLcontext *ctx, GLenum prim );

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

static __inline__ void i810_draw_triangle( i810ContextPtr imesa,
					   i810VertexPtr v0,
					   i810VertexPtr v1,
					   i810VertexPtr v2 )
{
   GLuint vertsize = imesa->vertex_size;
   GLuint *vb = i810AllocDmaLow( imesa, 3 * 4 * vertsize );
   int j;

   COPY_DWORDS( j, vb, vertsize, v0 );
   COPY_DWORDS( j, vb, vertsize, v1 );
   COPY_DWORDS( j, vb, vertsize, v2 );
}


static __inline__ void i810_draw_quad( i810ContextPtr imesa,
				       i810VertexPtr v0,
				       i810VertexPtr v1,
				       i810VertexPtr v2,
				       i810VertexPtr v3 )
{
   GLuint vertsize = imesa->vertex_size;
   GLuint *vb = i810AllocDmaLow( imesa, 6 * 4 * vertsize );
   int j;

   COPY_DWORDS( j, vb, vertsize, v0 );
   COPY_DWORDS( j, vb, vertsize, v1 );
   COPY_DWORDS( j, vb, vertsize, v3 );
   COPY_DWORDS( j, vb, vertsize, v1 );
   COPY_DWORDS( j, vb, vertsize, v2 );
   COPY_DWORDS( j, vb, vertsize, v3 );
}


static __inline__ void i810_draw_point( i810ContextPtr imesa,
					i810VertexPtr tmp )
{
   GLfloat sz = imesa->glCtx->Point._Size * .5;
   int vertsize = imesa->vertex_size;
   GLuint *vb = i810AllocDmaLow( imesa, 2 * 4 * vertsize );
   int j;

   /* Draw a point as a horizontal line.
    */
   *(float *)&vb[0] = tmp->v.x - sz + 0.125;
   for (j = 1 ; j < vertsize ; j++)
      vb[j] = tmp->ui[j];
   vb += vertsize;

   *(float *)&vb[0] = tmp->v.x + sz + 0.125;
   for (j = 1 ; j < vertsize ; j++)
      vb[j] = tmp->ui[j];
   vb += vertsize;
}


static __inline__ void i810_draw_line( i810ContextPtr imesa,
				       i810VertexPtr v0,
				       i810VertexPtr v1 )
{
   GLuint vertsize = imesa->vertex_size;
   GLuint *vb = i810AllocDmaLow( imesa, 2 * 4 * vertsize );
   int j;

   COPY_DWORDS( j, vb, vertsize, v0 );
   COPY_DWORDS( j, vb, vertsize, v1 );
}



/***********************************************************************
 *          Macros for t_dd_tritmp.h to draw basic primitives          *
 ***********************************************************************/

#define TRI( a, b, c )				\
do { 						\
   if (0) fprintf(stderr, "hw TRI\n");		\
   if (DO_FALLBACK)				\
      imesa->draw_tri( imesa, a, b, c );	\
   else						\
      i810_draw_triangle( imesa, a, b, c );	\
} while (0)

#define QUAD( a, b, c, d )			\
do { 						\
   if (0) fprintf(stderr, "hw QUAD\n");		\
   if (DO_FALLBACK) {				\
      imesa->draw_tri( imesa, a, b, d );	\
      imesa->draw_tri( imesa, b, c, d );	\
   } else					\
      i810_draw_quad( imesa, a, b, c, d );	\
} while (0)

#define LINE( v0, v1 )				\
do { 						\
   if (0) fprintf(stderr, "hw LINE\n");		\
   if (DO_FALLBACK)				\
      imesa->draw_line( imesa, v0, v1 );	\
   else						\
      i810_draw_line( imesa, v0, v1 );		\
} while (0)

#define POINT( v0 )				\
do { 						\
   if (0) fprintf(stderr, "hw POINT\n");	\
   if (DO_FALLBACK)				\
      imesa->draw_point( imesa, v0 );		\
   else						\
      i810_draw_point( imesa, v0 );		\
} while (0)


/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/

#define I810_OFFSET_BIT 	0x01
#define I810_TWOSIDE_BIT	0x02
#define I810_UNFILLED_BIT	0x04
#define I810_FALLBACK_BIT	0x08
#define I810_MAX_TRIFUNC	0x10


static struct {
   tnl_points_func	        points;
   tnl_line_func		line;
   tnl_triangle_func	triangle;
   tnl_quad_func		quad;
} rast_tab[I810_MAX_TRIFUNC];


#define DO_FALLBACK (IND & I810_FALLBACK_BIT)
#define DO_OFFSET   (IND & I810_OFFSET_BIT)
#define DO_UNFILLED (IND & I810_UNFILLED_BIT)
#define DO_TWOSIDE  (IND & I810_TWOSIDE_BIT)
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
#define VERTEX            i810Vertex
#define TAB               rast_tab


#define DEPTH_SCALE (1.0/0xffff)
#define UNFILLED_TRI unfilled_tri
#define UNFILLED_QUAD unfilled_quad
#define VERT_X(_v) _v->v.x
#define VERT_Y(_v) _v->v.y
#define VERT_Z(_v) _v->v.z
#define AREA_IS_CCW( a ) (a > 0)
#define GET_VERTEX(e) (imesa->verts + (e * imesa->vertex_size * sizeof(int)))

#define VERT_SET_RGBA( v, c )  					\
do {								\
   i810_color_t *color = (i810_color_t *)&((v)->ui[coloroffset]);	\
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
   i810ContextPtr imesa = I810_CONTEXT(ctx);				\
   GLuint color[n], spec[n];						\
   GLuint coloroffset = (imesa->vertex_size == 4 ? 3 : 4);		\
   GLboolean havespec = (imesa->vertex_size > 4);			\
   (void) color; (void) spec; (void) coloroffset; (void) havespec;


/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/

static const GLuint hw_prim[GL_POLYGON+1] = {
   PR_LINES,
   PR_LINES,
   PR_LINES,
   PR_LINES,
   PR_TRIANGLES,
   PR_TRIANGLES,
   PR_TRIANGLES,
   PR_TRIANGLES,
   PR_TRIANGLES,
   PR_TRIANGLES
};

#define RASTERIZE(x) if (imesa->hw_primitive != hw_prim[x]) \
                        i810RasterPrimitive( ctx, x, hw_prim[x] )
#define RENDER_PRIMITIVE imesa->render_primitive
#define TAG(x) x
#define IND I810_FALLBACK_BIT
#include "tnl_dd/t_dd_unfilled.h"
#undef IND

/***********************************************************************
 *                      Generate GL render functions                   *
 ***********************************************************************/

#define IND (0)
#define TAG(x) x
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I810_OFFSET_BIT)
#define TAG(x) x##_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I810_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I810_TWOSIDE_BIT|I810_OFFSET_BIT)
#define TAG(x) x##_twoside_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I810_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I810_OFFSET_BIT|I810_UNFILLED_BIT)
#define TAG(x) x##_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I810_TWOSIDE_BIT|I810_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I810_TWOSIDE_BIT|I810_OFFSET_BIT|I810_UNFILLED_BIT)
#define TAG(x) x##_twoside_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I810_FALLBACK_BIT)
#define TAG(x) x##_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I810_OFFSET_BIT|I810_FALLBACK_BIT)
#define TAG(x) x##_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I810_TWOSIDE_BIT|I810_FALLBACK_BIT)
#define TAG(x) x##_twoside_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I810_TWOSIDE_BIT|I810_OFFSET_BIT|I810_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I810_UNFILLED_BIT|I810_FALLBACK_BIT)
#define TAG(x) x##_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I810_OFFSET_BIT|I810_UNFILLED_BIT|I810_FALLBACK_BIT)
#define TAG(x) x##_offset_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I810_TWOSIDE_BIT|I810_UNFILLED_BIT|I810_FALLBACK_BIT)
#define TAG(x) x##_twoside_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (I810_TWOSIDE_BIT|I810_OFFSET_BIT|I810_UNFILLED_BIT| \
	     I810_FALLBACK_BIT)
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
i810_fallback_tri( i810ContextPtr imesa,
		   i810Vertex *v0,
		   i810Vertex *v1,
		   i810Vertex *v2 )
{
   GLcontext *ctx = imesa->glCtx;
   SWvertex v[3];
   i810_translate_vertex( ctx, v0, &v[0] );
   i810_translate_vertex( ctx, v1, &v[1] );
   i810_translate_vertex( ctx, v2, &v[2] );
   _swrast_Triangle( ctx, &v[0], &v[1], &v[2] );
}


static void
i810_fallback_line( i810ContextPtr imesa,
		    i810Vertex *v0,
		    i810Vertex *v1 )
{
   GLcontext *ctx = imesa->glCtx;
   SWvertex v[2];
   i810_translate_vertex( ctx, v0, &v[0] );
   i810_translate_vertex( ctx, v1, &v[1] );
   _swrast_Line( ctx, &v[0], &v[1] );
}


static void
i810_fallback_point( i810ContextPtr imesa,
		     i810Vertex *v0 )
{
   GLcontext *ctx = imesa->glCtx;
   SWvertex v[1];
   i810_translate_vertex( ctx, v0, &v[0] );
   _swrast_Point( ctx, &v[0] );
}



/**********************************************************************/
/*               Render unclipped begin/end objects                   */
/**********************************************************************/

#define IND 0
#define V(x) (i810Vertex *)(vertptr + ((x)*vertsize*sizeof(int)))
#define RENDER_POINTS( start, count )	\
   for ( ; start < count ; start++) POINT( V(ELT(start)) );
#define RENDER_LINE( v0, v1 )         LINE( V(v0), V(v1) )
#define RENDER_TRI(  v0, v1, v2 )     TRI(  V(v0), V(v1), V(v2) )
#define RENDER_QUAD( v0, v1, v2, v3 ) QUAD( V(v0), V(v1), V(v2), V(v3) )
#define INIT(x) i810RenderPrimitive( ctx, x )
#undef LOCAL_VARS
#define LOCAL_VARS						\
    i810ContextPtr imesa = I810_CONTEXT(ctx);			\
    GLubyte *vertptr = (GLubyte *)imesa->verts;			\
    const GLuint vertsize = imesa->vertex_size;       	\
    const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;	\
    (void) elt;
#define RESET_STIPPLE
#define RESET_OCCLUSION
#define PRESERVE_VB_DEFS
#define ELT(x) x
#define TAG(x) i810_##x##_verts
#include "tnl/t_vb_rendertmp.h"
#undef ELT
#undef TAG
#define TAG(x) i810_##x##_elts
#define ELT(x) elt[x]
#include "tnl/t_vb_rendertmp.h"

/**********************************************************************/
/*                   Render clipped primitives                        */
/**********************************************************************/



static void i810RenderClippedPoly( GLcontext *ctx, const GLuint *elts,
				   GLuint n )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
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

static void i810RenderClippedLine( GLcontext *ctx, GLuint ii, GLuint jj )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   tnl->Driver.Render.Line( ctx, ii, jj );
}

static void i810FastRenderClippedPoly( GLcontext *ctx, const GLuint *elts,
				       GLuint n )
{
   i810ContextPtr imesa = I810_CONTEXT( ctx );
   GLuint vertsize = imesa->vertex_size;
   GLuint *vb = i810AllocDmaLow( imesa, (n-2) * 3 * 4 * vertsize );
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

/***********************************************************************
 *                    Rasterization fallback helpers                   *
 ***********************************************************************/



#define _I810_NEW_RENDERSTATE (_DD_NEW_LINE_STIPPLE |		\
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

static void i810ChooseRenderState(GLcontext *ctx)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   GLuint flags = ctx->_TriangleCaps;
   GLuint index = 0;

   if (I810_DEBUG & DEBUG_STATE)
     fprintf(stderr,"\n%s\n",__FUNCTION__);

   if (flags & (ANY_FALLBACK_FLAGS|ANY_RASTER_FLAGS)) {
      if (flags & ANY_RASTER_FLAGS) {
	 if (flags & DD_TRI_LIGHT_TWOSIDE)    index |= I810_TWOSIDE_BIT;
	 if (flags & DD_TRI_OFFSET)	      index |= I810_OFFSET_BIT;
	 if (flags & DD_TRI_UNFILLED)	      index |= I810_UNFILLED_BIT;
      }

      imesa->draw_point = i810_draw_point;
      imesa->draw_line = i810_draw_line;
      imesa->draw_tri = i810_draw_triangle;

      /* Hook in fallbacks for specific primitives.
       */
      if (flags & ANY_FALLBACK_FLAGS)
      {
	 if (flags & POINT_FALLBACK)
	    imesa->draw_point = i810_fallback_point;

	 if (flags & LINE_FALLBACK)
	    imesa->draw_line = i810_fallback_line;

	 if (flags & TRI_FALLBACK)
	    imesa->draw_tri = i810_fallback_tri;

	 if ((flags & DD_TRI_STIPPLE) && !imesa->stipple_in_hw)
	    imesa->draw_tri = i810_fallback_tri;

	 index |= I810_FALLBACK_BIT;
      }
   }

   if (imesa->RenderIndex != index) {
      imesa->RenderIndex = index;

      tnl->Driver.Render.Points = rast_tab[index].points;
      tnl->Driver.Render.Line = rast_tab[index].line;
      tnl->Driver.Render.Triangle = rast_tab[index].triangle;
      tnl->Driver.Render.Quad = rast_tab[index].quad;

      if (index == 0) {
	 tnl->Driver.Render.PrimTabVerts = i810_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = i810_render_tab_elts;
	 tnl->Driver.Render.ClippedLine = line; /* from tritmp.h */
	 tnl->Driver.Render.ClippedPolygon = i810FastRenderClippedPoly;
      } else {
	 tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
	 tnl->Driver.Render.ClippedLine = i810RenderClippedLine;
	 tnl->Driver.Render.ClippedPolygon = i810RenderClippedPoly;
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
static void i810RenderPrimitive( GLcontext *ctx, GLenum prim )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   GLuint rprim = reduced_prim[prim];

   imesa->render_primitive = prim;

   if (rprim == GL_TRIANGLES && (ctx->_TriangleCaps & DD_TRI_UNFILLED))
      return;

   if (imesa->reduced_primitive != rprim ||
       hw_prim[prim] != imesa->hw_primitive) {
      i810RasterPrimitive( ctx, rprim, hw_prim[prim] );
   }
}

static void i810RunPipeline( GLcontext *ctx )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);

   if (imesa->new_state) {
      if (imesa->new_state & _NEW_TEXTURE)
	 i810UpdateTextureState( ctx );	/* may modify imesa->new_state */

      if (!imesa->Fallback) {
	 if (imesa->new_state & _I810_NEW_VERTEX)
	    i810ChooseVertexState( ctx );

	 if (imesa->new_state & _I810_NEW_RENDERSTATE)
	    i810ChooseRenderState( ctx );
      }

      imesa->new_state = 0;
   }

   _tnl_run_pipeline( ctx );
}

static void i810RenderStart( GLcontext *ctx )
{
   /* Check for projective textureing.  Make sure all texcoord
    * pointers point to something.  (fix in mesa?)
    */
   i810CheckTexSizes( ctx );
}

static void i810RenderFinish( GLcontext *ctx )
{
   if (I810_CONTEXT(ctx)->RenderIndex & I810_FALLBACK_BIT)
      _swrast_flush( ctx );
}




/* System to flush dma and emit state changes based on the rasterized
 * primitive.
 */
void i810RasterPrimitive( GLcontext *ctx,
			  GLenum rprim,
			  GLuint hwprim )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   GLuint st1 = imesa->Setup[I810_CTXREG_ST1];
   GLuint aa = imesa->Setup[I810_CTXREG_AA];
   GLuint lcs = imesa->Setup[I810_CTXREG_LCS];

   st1 &= ~ST1_ENABLE;
   aa &= ~AA_ENABLE;

   if (I810_DEBUG & DEBUG_PRIMS) {
      /* Prints reduced prim, and hw prim */
      char *prim_name = "Unknown";
      
      switch(hwprim) {
      case PR_LINES:
	 prim_name = "Lines";
	 break;
      case PR_LINESTRIP:
	 prim_name = "LineStrip";
	 break;	 
      case PR_TRIANGLES:
	 prim_name = "Triangles";
	 break;	 
      case PR_TRISTRIP_0:
	 prim_name = "TriStrip_0";
	 break;	 
      case PR_TRIFAN:
	 prim_name = "TriFan";
	 break;	 
      case PR_POLYGON:
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
      if (ctx->Polygon.StippleFlag)
	 st1 |= ST1_ENABLE;
      if (ctx->Polygon.SmoothFlag)
	 aa |= AA_ENABLE;
      break;
   case GL_LINES:
      lcs &= ~(LCS_LINEWIDTH_3_0|LCS_LINEWIDTH_0_5);
      lcs |= imesa->LcsLineWidth;
      if (ctx->Line.SmoothFlag) {
	 aa |= AA_ENABLE;
	 lcs |= LCS_LINEWIDTH_0_5;
      }
      break;
   case GL_POINTS:
      lcs &= ~(LCS_LINEWIDTH_3_0|LCS_LINEWIDTH_0_5);
      lcs |= imesa->LcsPointSize;
      if (ctx->Point.SmoothFlag) {
	 aa |= AA_ENABLE;
	 lcs |= LCS_LINEWIDTH_0_5;
      }
      break;
   default:
      return;
   }

   imesa->reduced_primitive = rprim;

   if (st1 != imesa->Setup[I810_CTXREG_ST1] ||
       aa != imesa->Setup[I810_CTXREG_AA] ||
       lcs != imesa->Setup[I810_CTXREG_LCS])
   {
      I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
      imesa->hw_primitive = hwprim;
      imesa->Setup[I810_CTXREG_LCS] = lcs;
      imesa->Setup[I810_CTXREG_ST1] = st1;
      imesa->Setup[I810_CTXREG_AA] = aa;
   }
   else if (hwprim != imesa->hw_primitive) {
      I810_STATECHANGE(imesa, 0);
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

void i810Fallback( i810ContextPtr imesa, GLuint bit, GLboolean mode )
{
   GLcontext *ctx = imesa->glCtx;
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint oldfallback = imesa->Fallback;

   if (0) fprintf(stderr, "%s old %x bit %x mode %d\n", __FUNCTION__,
		  imesa->Fallback, bit, mode );

   if (mode) {
      imesa->Fallback |= bit;
      if (oldfallback == 0) {
	 I810_FIREVERTICES(imesa);
	 if (I810_DEBUG & DEBUG_FALLBACKS) 
	    fprintf(stderr, "ENTER FALLBACK %s\n", getFallbackString( bit ));
	 _swsetup_Wakeup( ctx );
	 imesa->RenderIndex = ~0;
      }
   }
   else {
      imesa->Fallback &= ~bit;
      if (oldfallback == bit) {
	 _swrast_flush( ctx );
	 if (I810_DEBUG & DEBUG_FALLBACKS) 
	    fprintf(stderr, "LEAVE FALLBACK %s\n", getFallbackString( bit ));
	 tnl->Driver.Render.Start = i810RenderStart;
	 tnl->Driver.Render.PrimitiveNotify = i810RenderPrimitive;
	 tnl->Driver.Render.Finish = i810RenderFinish;
	 tnl->Driver.Render.BuildVertices = i810BuildVertices;
	 imesa->new_state |= (_I810_NEW_RENDERSTATE|_I810_NEW_VERTEX);
      }
   }
}


/**********************************************************************/
/*                            Initialization.                         */
/**********************************************************************/


void i810InitTriFuncs( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   static int firsttime = 1;

   if (firsttime) {
      init_rast_tab();
      firsttime = 0;
   }

   tnl->Driver.RunPipeline = i810RunPipeline;
   tnl->Driver.Render.Start = i810RenderStart;
   tnl->Driver.Render.Finish = i810RenderFinish;
   tnl->Driver.Render.PrimitiveNotify = i810RenderPrimitive;
   tnl->Driver.Render.ResetLineStipple = _swrast_ResetLineStipple;
   tnl->Driver.Render.BuildVertices = i810BuildVertices;
}
