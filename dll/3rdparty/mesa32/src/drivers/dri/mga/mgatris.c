/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/mga/mgatris.c,v 1.10 2002/10/30 12:51:36 alanh Exp $ */

#include "mtypes.h"
#include "macros.h"
#include "colormac.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#include "mm.h"
#include "mgacontext.h"
#include "mgaioctl.h"
#include "mgatris.h"
#include "mgavb.h"
#include "mgastate.h"


static void mgaRenderPrimitive( GLcontext *ctx, GLenum prim );

/***********************************************************************
 *                 Functions to draw basic primitives                  *
 ***********************************************************************/


#if defined (USE_X86_ASM)
#define EMIT_VERT( j, vb, vertex_size, v )		\
do {	int __tmp;					\
	__asm__ __volatile__( "rep ; movsl"		\
			 : "=%c" (j), "=D" (vb), "=S" (__tmp)		\
			 : "0" (vertex_size), 		\
			   "D" ((long)vb), 		\
			   "S" ((long)v));		\
} while (0)
#else
#define EMIT_VERT( j, vb, vertex_size, v )	\
do {						\
   for ( j = 0 ; j < vertex_size ; j++ )	\
      vb[j] = (v)->ui[j];			\
   vb += vertex_size;				\
} while (0)
#endif

static void __inline__ mga_draw_triangle( mgaContextPtr mmesa,
					   mgaVertexPtr v0,
					   mgaVertexPtr v1,
					   mgaVertexPtr v2 )
{
   GLuint vertex_size = mmesa->vertex_size;
   GLuint *vb = mgaAllocDmaLow( mmesa, 3 * 4 * vertex_size );
   int j;

   EMIT_VERT( j, vb, vertex_size, v0 );
   EMIT_VERT( j, vb, vertex_size, v1 );
   EMIT_VERT( j, vb, vertex_size, v2 );
}


static void __inline__ mga_draw_quad( mgaContextPtr mmesa,
				       mgaVertexPtr v0,
				       mgaVertexPtr v1,
				       mgaVertexPtr v2,
				       mgaVertexPtr v3 )
{
   GLuint vertex_size = mmesa->vertex_size;
   GLuint *vb = mgaAllocDmaLow( mmesa, 6 * 4 * vertex_size );
   int j;

   EMIT_VERT( j, vb, vertex_size, v0 );
   EMIT_VERT( j, vb, vertex_size, v1 );
   EMIT_VERT( j, vb, vertex_size, v3 );
   EMIT_VERT( j, vb, vertex_size, v1 );
   EMIT_VERT( j, vb, vertex_size, v2 );
   EMIT_VERT( j, vb, vertex_size, v3 );
}


static __inline__ void mga_draw_point( mgaContextPtr mmesa,
					mgaVertexPtr tmp )
{
   GLfloat sz = mmesa->glCtx->Point._Size * .5;
   int vertex_size = mmesa->vertex_size;
   GLuint *vb = mgaAllocDmaLow( mmesa, 6 * 4 * vertex_size );
   int j;
   
#if 0
   v0->v.x += PNT_X_OFFSET - TRI_X_OFFSET;
   v0->v.y += PNT_Y_OFFSET - TRI_Y_OFFSET;
#endif

   /* Draw a point as two triangles.
    */
   *(float *)&vb[0] = tmp->v.x - sz;
   *(float *)&vb[1] = tmp->v.y - sz;
   for (j = 2 ; j < vertex_size ; j++) 
      vb[j] = tmp->ui[j];
   vb += vertex_size;

   *(float *)&vb[0] = tmp->v.x + sz;
   *(float *)&vb[1] = tmp->v.y - sz;
   for (j = 2 ; j < vertex_size ; j++) 
      vb[j] = tmp->ui[j];
   vb += vertex_size;

   *(float *)&vb[0] = tmp->v.x + sz;
   *(float *)&vb[1] = tmp->v.y + sz;
   for (j = 2 ; j < vertex_size ; j++) 
      vb[j] = tmp->ui[j];
   vb += vertex_size;

   *(float *)&vb[0] = tmp->v.x + sz;
   *(float *)&vb[1] = tmp->v.y + sz;
   for (j = 2 ; j < vertex_size ; j++) 
      vb[j] = tmp->ui[j];
   vb += vertex_size;

   *(float *)&vb[0] = tmp->v.x - sz;
   *(float *)&vb[1] = tmp->v.y + sz;
   for (j = 2 ; j < vertex_size ; j++) 
      vb[j] = tmp->ui[j];
   vb += vertex_size;

   *(float *)&vb[0] = tmp->v.x - sz;
   *(float *)&vb[1] = tmp->v.y - sz;
   for (j = 2 ; j < vertex_size ; j++) 
      vb[j] = tmp->ui[j];

#if 0
   v0->v.x -= PNT_X_OFFSET - TRI_X_OFFSET;
   v0->v.y -= PNT_Y_OFFSET - TRI_Y_OFFSET;
#endif
}


static __inline__ void mga_draw_line( mgaContextPtr mmesa,
				      mgaVertexPtr v0,
				      mgaVertexPtr v1 )
{
   GLuint vertex_size = mmesa->vertex_size;
   GLuint *vb = mgaAllocDmaLow( mmesa, 6 * 4 * vertex_size );
   GLfloat dx, dy, ix, iy;
   GLfloat width = mmesa->glCtx->Line._Width;
   GLint j;

#if 0
   v0->v.x += LINE_X_OFFSET - TRI_X_OFFSET;
   v0->v.y += LINE_Y_OFFSET - TRI_Y_OFFSET;
   v1->v.x += LINE_X_OFFSET - TRI_X_OFFSET;
   v1->v.y += LINE_Y_OFFSET - TRI_Y_OFFSET;
#endif

   dx = v0->v.x - v1->v.x;
   dy = v0->v.y - v1->v.y;
   
   ix = width * .5; iy = 0;
   if (dx * dx > dy * dy) {
      iy = ix; ix = 0;
   }

   *(float *)&vb[0] = v0->v.x - ix;
   *(float *)&vb[1] = v0->v.y - iy;
   for (j = 2 ; j < vertex_size ; j++) 
      vb[j] = v0->ui[j];
   vb += vertex_size;

   *(float *)&vb[0] = v1->v.x + ix;
   *(float *)&vb[1] = v1->v.y + iy;
   for (j = 2 ; j < vertex_size ; j++) 
      vb[j] = v1->ui[j];
   vb += vertex_size;

   *(float *)&vb[0] = v0->v.x + ix;
   *(float *)&vb[1] = v0->v.y + iy;
   for (j = 2 ; j < vertex_size ; j++) 
      vb[j] = v0->ui[j];
   vb += vertex_size;
	 
   *(float *)&vb[0] = v0->v.x - ix;
   *(float *)&vb[1] = v0->v.y - iy;
   for (j = 2 ; j < vertex_size ; j++) 
      vb[j] = v0->ui[j];
   vb += vertex_size;

   *(float *)&vb[0] = v1->v.x - ix;
   *(float *)&vb[1] = v1->v.y - iy;
   for (j = 2 ; j < vertex_size ; j++) 
      vb[j] = v1->ui[j];
   vb += vertex_size;

   *(float *)&vb[0] = v1->v.x + ix;
   *(float *)&vb[1] = v1->v.y + iy;
   for (j = 2 ; j < vertex_size ; j++) 
      vb[j] = v1->ui[j];
   vb += vertex_size;

#if 0
   v0->v.x -= LINE_X_OFFSET - TRI_X_OFFSET;
   v0->v.y -= LINE_Y_OFFSET - TRI_Y_OFFSET;
   v1->v.x -= LINE_X_OFFSET - TRI_X_OFFSET;
   v1->v.y -= LINE_Y_OFFSET - TRI_Y_OFFSET;
#endif
}

/***********************************************************************
 *          Macros for t_dd_tritmp.h to draw basic primitives          *
 ***********************************************************************/

#define TRI( a, b, c )				\
do {						\
   if (DO_FALLBACK)				\
      mmesa->draw_tri( mmesa, a, b, c );	\
   else						\
      mga_draw_triangle( mmesa, a, b, c );	\
} while (0)

#define QUAD( a, b, c, d )			\
do {						\
   if (DO_FALLBACK) {				\
      mmesa->draw_tri( mmesa, a, b, d );	\
      mmesa->draw_tri( mmesa, b, c, d );	\
   } else {					\
      mga_draw_quad( mmesa, a, b, c, d );	\
   }						\
} while (0)

#define LINE( v0, v1 )				\
do {						\
   if (DO_FALLBACK)				\
      mmesa->draw_line( mmesa, v0, v1 );	\
   else {					\
      mga_draw_line( mmesa, v0, v1 );		\
   }						\
} while (0)

#define POINT( v0 )				\
do {						\
   if (DO_FALLBACK)				\
      mmesa->draw_point( mmesa, v0 );		\
   else {					\
      mga_draw_point( mmesa, v0 );		\
   }						\
} while (0)


/***********************************************************************
 *              Fallback to swrast for basic primitives                *
 ***********************************************************************/

/* This code is hit only when a mix of accelerated and unaccelerated
 * primitives are being drawn, and only for the unaccelerated
 * primitives.  
 */

static void 
mga_fallback_tri( mgaContextPtr mmesa, 
		   mgaVertex *v0, 
		   mgaVertex *v1, 
		   mgaVertex *v2 )
{
   GLcontext *ctx = mmesa->glCtx;
   SWvertex v[3];
   mga_translate_vertex( ctx, v0, &v[0] );
   mga_translate_vertex( ctx, v1, &v[1] );
   mga_translate_vertex( ctx, v2, &v[2] );
   _swrast_Triangle( ctx, &v[0], &v[1], &v[2] );
}


static void 
mga_fallback_line( mgaContextPtr mmesa,
		    mgaVertex *v0,
		    mgaVertex *v1 )
{
   GLcontext *ctx = mmesa->glCtx;
   SWvertex v[2];
   mga_translate_vertex( ctx, v0, &v[0] );
   mga_translate_vertex( ctx, v1, &v[1] );
   _swrast_Line( ctx, &v[0], &v[1] );
}


static void 
mga_fallback_point( mgaContextPtr mmesa, 
		     mgaVertex *v0 )
{
   GLcontext *ctx = mmesa->glCtx;
   SWvertex v[1];
   mga_translate_vertex( ctx, v0, &v[0] );
   _swrast_Point( ctx, &v[0] );
}

/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/


#define MGA_UNFILLED_BIT    0x1
#define MGA_OFFSET_BIT	    0x2
#define MGA_TWOSIDE_BIT     0x4
#define MGA_FLAT_BIT        0x8	/* mga can't flatshade? */
#define MGA_FALLBACK_BIT    0x10
#define MGA_MAX_TRIFUNC     0x20

static struct {
   tnl_points_func	        points;
   tnl_line_func		line;
   tnl_triangle_func	triangle;
   tnl_quad_func		quad;
} rast_tab[MGA_MAX_TRIFUNC];

#define DO_FALLBACK (IND & MGA_FALLBACK_BIT)
#define DO_OFFSET   (IND & MGA_OFFSET_BIT)
#define DO_UNFILLED (IND & MGA_UNFILLED_BIT)
#define DO_TWOSIDE  (IND & MGA_TWOSIDE_BIT)
#define DO_FLAT     (IND & MGA_FLAT_BIT)
#define DO_TRI       1
#define DO_QUAD      1
#define DO_LINE      1
#define DO_POINTS    1
#define DO_FULL_QUAD 1

#define HAVE_RGBA         1
#define HAVE_BACK_COLORS  0
#define HAVE_SPEC         1
#define HAVE_HW_FLATSHADE 0
#define VERTEX mgaVertex
#define TAB rast_tab


#define DEPTH_SCALE mmesa->depth_scale
#define UNFILLED_TRI unfilled_tri
#define UNFILLED_QUAD unfilled_quad
#define VERT_X(_v) _v->v.x
#define VERT_Y(_v) _v->v.y
#define VERT_Z(_v) _v->v.z
#define AREA_IS_CCW( a ) (a > 0)
#define GET_VERTEX(e) (mmesa->verts + (e * mmesa->vertex_size * sizeof(int)))

#define VERT_SET_RGBA( v, c )  					\
do {								\
   mga_color_t *color = (mga_color_t *)&((v)->ui[4]);	\
   UNCLAMPED_FLOAT_TO_UBYTE(color->red, (c)[0]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->green, (c)[1]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->blue, (c)[2]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->alpha, (c)[3]);		\
} while (0)

#define VERT_COPY_RGBA( v0, v1 ) v0->ui[4] = v1->ui[4]

#define VERT_SET_SPEC( v0, c )					\
do {								\
   UNCLAMPED_FLOAT_TO_UBYTE(v0->v.specular.red, (c)[0]);	\
   UNCLAMPED_FLOAT_TO_UBYTE(v0->v.specular.green, (c)[1]);	\
   UNCLAMPED_FLOAT_TO_UBYTE(v0->v.specular.blue, (c)[2]);	\
} while (0)

#define VERT_COPY_SPEC( v0, v1 )		\
do {						\
   v0->v.specular.red   = v1->v.specular.red;	\
   v0->v.specular.green = v1->v.specular.green;	\
   v0->v.specular.blue  = v1->v.specular.blue;	\
} while (0)

#define VERT_SAVE_RGBA( idx )    color[idx] = v[idx]->ui[4]
#define VERT_RESTORE_RGBA( idx ) v[idx]->ui[4] = color[idx]
#define VERT_SAVE_SPEC( idx )    spec[idx] = v[idx]->ui[5]
#define VERT_RESTORE_SPEC( idx ) v[idx]->ui[5] = spec[idx]

#define LOCAL_VARS(n)					\
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);		\
   GLuint color[n], spec[n];				\
   (void) color; (void) spec;



/***********************************************************************
 *            Functions to draw basic unfilled primitives              *
 ***********************************************************************/

#define RASTERIZE(x) if (mmesa->raster_primitive != x) \
                        mgaRasterPrimitive( ctx, x, MGA_WA_TRIANGLES )
#define RENDER_PRIMITIVE mmesa->render_primitive
#define IND MGA_FALLBACK_BIT
#define TAG(x) x
#include "tnl_dd/t_dd_unfilled.h"
#undef IND

/***********************************************************************
 *                 Functions to draw GL primitives                     *
 ***********************************************************************/

#define IND (0)
#define TAG(x) x
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_OFFSET_BIT)
#define TAG(x) x##_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_TWOSIDE_BIT|MGA_OFFSET_BIT)
#define TAG(x) x##_twoside_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_OFFSET_BIT|MGA_UNFILLED_BIT)
#define TAG(x) x##_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_TWOSIDE_BIT|MGA_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_TWOSIDE_BIT|MGA_OFFSET_BIT|MGA_UNFILLED_BIT)
#define TAG(x) x##_twoside_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_FALLBACK_BIT)
#define TAG(x) x##_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_OFFSET_BIT|MGA_FALLBACK_BIT)
#define TAG(x) x##_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_TWOSIDE_BIT|MGA_FALLBACK_BIT)
#define TAG(x) x##_twoside_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_TWOSIDE_BIT|MGA_OFFSET_BIT|MGA_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_UNFILLED_BIT|MGA_FALLBACK_BIT)
#define TAG(x) x##_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_OFFSET_BIT|MGA_UNFILLED_BIT|MGA_FALLBACK_BIT)
#define TAG(x) x##_offset_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_TWOSIDE_BIT|MGA_UNFILLED_BIT|MGA_FALLBACK_BIT)
#define TAG(x) x##_twoside_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_TWOSIDE_BIT|MGA_OFFSET_BIT|MGA_UNFILLED_BIT| \
	     MGA_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"


/* Mga doesn't support provoking-vertex flat-shading?
 */
#define IND (MGA_FLAT_BIT)
#define TAG(x) x##_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_OFFSET_BIT|MGA_FLAT_BIT)
#define TAG(x) x##_offset_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_TWOSIDE_BIT|MGA_FLAT_BIT)
#define TAG(x) x##_twoside_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_TWOSIDE_BIT|MGA_OFFSET_BIT|MGA_FLAT_BIT)
#define TAG(x) x##_twoside_offset_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_UNFILLED_BIT|MGA_FLAT_BIT)
#define TAG(x) x##_unfilled_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_OFFSET_BIT|MGA_UNFILLED_BIT|MGA_FLAT_BIT)
#define TAG(x) x##_offset_unfilled_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_TWOSIDE_BIT|MGA_UNFILLED_BIT|MGA_FLAT_BIT)
#define TAG(x) x##_twoside_unfilled_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_TWOSIDE_BIT|MGA_OFFSET_BIT|MGA_UNFILLED_BIT|MGA_FLAT_BIT)
#define TAG(x) x##_twoside_offset_unfilled_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_FALLBACK_BIT|MGA_FLAT_BIT)
#define TAG(x) x##_fallback_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_OFFSET_BIT|MGA_FALLBACK_BIT|MGA_FLAT_BIT)
#define TAG(x) x##_offset_fallback_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_TWOSIDE_BIT|MGA_FALLBACK_BIT|MGA_FLAT_BIT)
#define TAG(x) x##_twoside_fallback_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_TWOSIDE_BIT|MGA_OFFSET_BIT|MGA_FALLBACK_BIT|MGA_FLAT_BIT)
#define TAG(x) x##_twoside_offset_fallback_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_UNFILLED_BIT|MGA_FALLBACK_BIT|MGA_FLAT_BIT)
#define TAG(x) x##_unfilled_fallback_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_OFFSET_BIT|MGA_UNFILLED_BIT|MGA_FALLBACK_BIT|MGA_FLAT_BIT)
#define TAG(x) x##_offset_unfilled_fallback_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_TWOSIDE_BIT|MGA_UNFILLED_BIT|MGA_FALLBACK_BIT|MGA_FLAT_BIT)
#define TAG(x) x##_twoside_unfilled_fallback_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MGA_TWOSIDE_BIT|MGA_OFFSET_BIT|MGA_UNFILLED_BIT| \
	     MGA_FALLBACK_BIT|MGA_FLAT_BIT)
#define TAG(x) x##_twoside_offset_unfilled_fallback_flat
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

   init_flat();
   init_offset_flat();
   init_twoside_flat();
   init_twoside_offset_flat();
   init_unfilled_flat();
   init_offset_unfilled_flat();
   init_twoside_unfilled_flat();
   init_twoside_offset_unfilled_flat();
   init_fallback_flat();
   init_offset_fallback_flat();
   init_twoside_fallback_flat();
   init_twoside_offset_fallback_flat();
   init_unfilled_fallback_flat();
   init_offset_unfilled_fallback_flat();
   init_twoside_unfilled_fallback_flat();
   init_twoside_offset_unfilled_fallback_flat();
}

/**********************************************************************/
/*                 Render whole begin/end objects                     */
/**********************************************************************/


#define VERT(x) (mgaVertex *)(vertptr + ((x)*vertex_size*sizeof(int)))
#define RENDER_POINTS( start, count )		\
   for ( ; start < count ; start++)		\
      mga_draw_point( mmesa, VERT(ELT(start)) );
#define RENDER_LINE( v0, v1 ) \
   mga_draw_line( mmesa, VERT(v0), VERT(v1) )
#define RENDER_TRI( v0, v1, v2 )  \
   mga_draw_triangle( mmesa, VERT(v0), VERT(v1), VERT(v2) )
#define RENDER_QUAD( v0, v1, v2, v3 ) \
   mga_draw_quad( mmesa, VERT(v0), VERT(v1), VERT(v2), VERT(v3) )
#define INIT(x) mgaRenderPrimitive( ctx, x )
#undef LOCAL_VARS
#define LOCAL_VARS						\
    mgaContextPtr mmesa = MGA_CONTEXT(ctx);			\
    GLubyte *vertptr = (GLubyte *)mmesa->verts;			\
    const GLuint vertex_size = mmesa->vertex_size;       	\
    const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;	\
    (void) elt;
#define RESET_STIPPLE 
#define RESET_OCCLUSION 
#define PRESERVE_VB_DEFS
#define ELT(x) x
#define TAG(x) mga_##x##_verts
#include "tnl/t_vb_rendertmp.h"
#undef ELT
#undef TAG
#define TAG(x) mga_##x##_elts
#define ELT(x) elt[x]
#include "tnl/t_vb_rendertmp.h"


/**********************************************************************/
/*                   Render clipped primitives                        */
/**********************************************************************/



static void mgaRenderClippedPoly( GLcontext *ctx, const GLuint *elts, GLuint n )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint prim = mmesa->render_primitive;

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

static void mgaRenderClippedLine( GLcontext *ctx, GLuint ii, GLuint jj )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   tnl->Driver.Render.Line( ctx, ii, jj );
}

static void mgaFastRenderClippedPoly( GLcontext *ctx, const GLuint *elts, 
				       GLuint n )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   GLuint vertex_size = mmesa->vertex_size;
   GLuint *vb = mgaAllocDmaLow( mmesa, (n-2) * 3 * 4 * vertex_size );
   GLubyte *vertptr = (GLubyte *)mmesa->verts;			
   const GLuint *start = (const GLuint *)VERT(elts[0]);
   int i,j;

   for (i = 2 ; i < n ; i++) {
      EMIT_VERT( j, vb, vertex_size, (mgaVertexPtr) VERT(elts[i-1]) );
      EMIT_VERT( j, vb, vertex_size, (mgaVertexPtr) VERT(elts[i]) );
      EMIT_VERT( j, vb, vertex_size, (mgaVertexPtr) start );
   }
}

/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/


#define POINT_FALLBACK (DD_POINT_SMOOTH)
#define LINE_FALLBACK (DD_LINE_SMOOTH | DD_LINE_STIPPLE)
#define TRI_FALLBACK (DD_TRI_SMOOTH | DD_TRI_UNFILLED)
#define ANY_FALLBACK_FLAGS (POINT_FALLBACK|LINE_FALLBACK|TRI_FALLBACK)
#define ANY_RASTER_FLAGS (DD_FLATSHADE|DD_TRI_LIGHT_TWOSIDE|DD_TRI_OFFSET| \
                          DD_TRI_UNFILLED)

void mgaChooseRenderState(GLcontext *ctx)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   GLuint flags = ctx->_TriangleCaps;
   GLuint index = 0;

   if (flags & (ANY_FALLBACK_FLAGS|ANY_RASTER_FLAGS|DD_TRI_STIPPLE)) {
      if (flags & ANY_RASTER_FLAGS) {
	 if (flags & DD_TRI_LIGHT_TWOSIDE)    index |= MGA_TWOSIDE_BIT;
	 if (flags & DD_TRI_OFFSET)	      index |= MGA_OFFSET_BIT;
	 if (flags & DD_TRI_UNFILLED)	      index |= MGA_UNFILLED_BIT;
	 if (flags & DD_FLATSHADE)	      index |= MGA_FLAT_BIT;
      }

      mmesa->draw_point = mga_draw_point;
      mmesa->draw_line = mga_draw_line;
      mmesa->draw_tri = mga_draw_triangle;

      /* Hook in fallbacks for specific primitives.
       */
      if (flags & ANY_FALLBACK_FLAGS)
      {
	 if (flags & POINT_FALLBACK) 
	    mmesa->draw_point = mga_fallback_point;
	 
	 if (flags & LINE_FALLBACK) 
	    mmesa->draw_line = mga_fallback_line;
	 
	 if (flags & TRI_FALLBACK) 
	    mmesa->draw_tri = mga_fallback_tri;
	 
	 index |= MGA_FALLBACK_BIT;
      }

      if ((flags & DD_TRI_STIPPLE) && !mmesa->haveHwStipple) {
	 mmesa->draw_tri = mga_fallback_tri;
	 index |= MGA_FALLBACK_BIT;
      }
   }

   if (mmesa->RenderIndex != index) {
      mmesa->RenderIndex = index;

      tnl->Driver.Render.Points = rast_tab[index].points;
      tnl->Driver.Render.Line = rast_tab[index].line;
      tnl->Driver.Render.Triangle = rast_tab[index].triangle;
      tnl->Driver.Render.Quad = rast_tab[index].quad;
         
      if (index == 0) {
	 tnl->Driver.Render.PrimTabVerts = mga_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = mga_render_tab_elts;
	 tnl->Driver.Render.ClippedLine = line; /* from tritmp.h */
	 tnl->Driver.Render.ClippedPolygon = mgaFastRenderClippedPoly;
      } else {
	 tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
	 tnl->Driver.Render.ClippedLine = mgaRenderClippedLine;
	 tnl->Driver.Render.ClippedPolygon = mgaRenderClippedPoly;
      }
   }
}

/**********************************************************************/
/*                Runtime render state and callbacks                  */
/**********************************************************************/


static GLenum reduced_prim[GL_POLYGON+1] = {
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



/* Always called between RenderStart and RenderFinish --> We already
 * hold the lock.
 */
void mgaRasterPrimitive( GLcontext *ctx, GLenum prim, GLuint hwprim )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );

   FLUSH_BATCH( mmesa );

   /* Update culling */
   if (mmesa->raster_primitive != prim)
      mmesa->dirty |= MGA_UPLOAD_CONTEXT;

   mmesa->raster_primitive = prim;
/*     mmesa->hw_primitive = hwprim; */
   mmesa->hw_primitive = MGA_WA_TRIANGLES; /* disable mgarender.c for now */

   if (ctx->Polygon.StippleFlag && mmesa->haveHwStipple)
   {
      mmesa->dirty |= MGA_UPLOAD_CONTEXT;
      mmesa->setup.dwgctl &= ~(0xf<<20);
      if (mmesa->raster_primitive == GL_TRIANGLES)
	 mmesa->setup.dwgctl |= mmesa->poly_stipple;
   }
}



/* Determine the rasterized primitive when not drawing unfilled 
 * polygons.
 *
 * Used only for the default render stage which always decomposes
 * primitives to trianges/lines/points.  For the accelerated stage,
 * which renders strips as strips, the equivalent calculations are
 * performed in mgarender.c.
 */
static void mgaRenderPrimitive( GLcontext *ctx, GLenum prim )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   GLuint rprim = reduced_prim[prim];

   mmesa->render_primitive = prim;

   if (rprim == GL_TRIANGLES && (ctx->_TriangleCaps & DD_TRI_UNFILLED))
      return;
       
   if (mmesa->raster_primitive != rprim) {
      mgaRasterPrimitive( ctx, rprim, MGA_WA_TRIANGLES );
   }
}

static void mgaRenderFinish( GLcontext *ctx )
{
   if (MGA_CONTEXT(ctx)->RenderIndex & MGA_FALLBACK_BIT)
      _swrast_flush( ctx );
}



/**********************************************************************/
/*               Manage total rasterization fallbacks                 */
/**********************************************************************/

static const char * const fallbackStrings[] = {
   "Texture mode",
   "glDrawBuffer(GL_FRONT_AND_BACK)",
   "read buffer",
   "glBlendFunc(GL_SRC_ALPHA_SATURATE, GL_ZERO)",
   "glRenderMode(selection or feedback)",
   "No hardware stencil",
   "glDepthFunc( GL_NEVER )",
   "Mixing GL_CLAMP_TO_EDGE and GL_CLAMP",
   "rasterization fallback option"
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


void mgaFallback( GLcontext *ctx, GLuint bit, GLboolean mode )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   GLuint oldfallback = mmesa->Fallback;

   if (mode) {
      mmesa->Fallback |= bit;
      if (oldfallback == 0) {
	 FLUSH_BATCH(mmesa);
	 _swsetup_Wakeup( ctx );
	 mmesa->RenderIndex = ~0;
         if (MGA_DEBUG & DEBUG_VERBOSE_FALLBACK) {
            fprintf(stderr, "MGA begin rasterization fallback: 0x%x %s\n",
                    bit, getFallbackString(bit));
         }
      }
   }
   else {
      mmesa->Fallback &= ~bit;
      if (oldfallback == bit) {
	 _swrast_flush( ctx );
	 tnl->Driver.Render.Start = mgaCheckTexSizes;
	 tnl->Driver.Render.PrimitiveNotify = mgaRenderPrimitive;
	 tnl->Driver.Render.Finish = mgaRenderFinish;
	 tnl->Driver.Render.BuildVertices = mgaBuildVertices;
	 mmesa->NewGLState |= (_MGA_NEW_RENDERSTATE |
			       _MGA_NEW_RASTERSETUP);
         if (MGA_DEBUG & DEBUG_VERBOSE_FALLBACK) {
            fprintf(stderr, "MGA end rasterization fallback: 0x%x %s\n",
                    bit, getFallbackString(bit));
         }
      }
   }
}


void mgaDDInitTriFuncs( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   static int firsttime = 1;
   if (firsttime) {
      init_rast_tab();
      firsttime = 0;
   }

   mmesa->RenderIndex = ~0;
	
   tnl->Driver.Render.Start              = mgaCheckTexSizes;
   tnl->Driver.Render.Finish             = mgaRenderFinish; 
   tnl->Driver.Render.PrimitiveNotify    = mgaRenderPrimitive;
   tnl->Driver.Render.ResetLineStipple   = _swrast_ResetLineStipple;
   tnl->Driver.Render.BuildVertices      = mgaBuildVertices;
   tnl->Driver.Render.Multipass		 = NULL;
}
