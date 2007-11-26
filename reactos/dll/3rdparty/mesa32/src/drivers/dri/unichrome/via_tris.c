/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <math.h>

#include "glheader.h"
#include "context.h"
#include "mtypes.h"
#include "macros.h"
#include "colormac.h"
#include "enums.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#include "via_context.h"
#include "via_tris.h"
#include "via_state.h"
#include "via_span.h"
#include "via_ioctl.h"
#include "via_3d_reg.h"
#include "via_tex.h"

/***********************************************************************
 *                    Emit primitives as inline vertices               *
 ***********************************************************************/
#define LINE_FALLBACK (0)
#define POINT_FALLBACK (0)
#define TRI_FALLBACK (0)
#define ANY_FALLBACK_FLAGS (POINT_FALLBACK|LINE_FALLBACK|TRI_FALLBACK)
#define ANY_RASTER_FLAGS (DD_TRI_LIGHT_TWOSIDE|DD_TRI_OFFSET|DD_TRI_UNFILLED)


#if 0
#define COPY_DWORDS(vb, vertsize, v) 		\
do {						\
   via_sse_memcpy(vb, v, vertsize * 4);		\
   vb += vertsize;				\
} while (0)
#else
#if defined( USE_X86_ASM )
#define COPY_DWORDS(vb, vertsize, v)					\
    do {								\
        int j;								\
        int __tmp;							\
        __asm__ __volatile__("rep ; movsl"				\
                              : "=%c" (j), "=D" (vb), "=S" (__tmp)	\
                              : "0" (vertsize),				\
                                "D" ((long)vb),				\
                                "S" ((long)v));				\
    } while (0)
#else
#define COPY_DWORDS(vb, vertsize, v)		\
    do {					\
        int j;					\
        for (j = 0; j < vertsize; j++)		\
            vb[j] = ((GLuint *)v)[j];		\
        vb += vertsize;				\
    } while (0)
#endif
#endif

static void via_draw_triangle(struct via_context *vmesa,
			      viaVertexPtr v0,
			      viaVertexPtr v1,
			      viaVertexPtr v2)
{
   GLuint vertsize = vmesa->vertexSize;
   GLuint *vb = viaExtendPrimitive(vmesa, 3 * 4 * vertsize);

   COPY_DWORDS(vb, vertsize, v0);
   COPY_DWORDS(vb, vertsize, v1);
   COPY_DWORDS(vb, vertsize, v2);
}


static void via_draw_quad(struct via_context *vmesa,
			  viaVertexPtr v0,
			  viaVertexPtr v1,
			  viaVertexPtr v2,
			  viaVertexPtr v3)
{
   GLuint vertsize = vmesa->vertexSize;
   GLuint *vb = viaExtendPrimitive(vmesa, 6 * 4 * vertsize);

   COPY_DWORDS(vb, vertsize, v0);
   COPY_DWORDS(vb, vertsize, v1);
   COPY_DWORDS(vb, vertsize, v3);
   COPY_DWORDS(vb, vertsize, v1);
   COPY_DWORDS(vb, vertsize, v2);
   COPY_DWORDS(vb, vertsize, v3);
}

static void via_draw_line(struct via_context *vmesa,
			  viaVertexPtr v0,
			  viaVertexPtr v1)
{
   GLuint vertsize = vmesa->vertexSize;
   GLuint *vb = viaExtendPrimitive(vmesa, 2 * 4 * vertsize);
   COPY_DWORDS(vb, vertsize, v0);
   COPY_DWORDS(vb, vertsize, v1);
}


static void via_draw_point(struct via_context *vmesa,
			   viaVertexPtr v0)
{
   GLuint vertsize = vmesa->vertexSize;
   GLuint *vb = viaExtendPrimitive(vmesa, 4 * vertsize);
   COPY_DWORDS(vb, vertsize, v0);
}


/* Fallback drawing functions for the ptex hack.
 */
#define PTEX_VERTEX( tmp, vertex_size, v)	\
do {							\
   GLuint j;						\
   GLfloat rhw = 1.0 / v->f[vertex_size];		\
   for ( j = 0 ; j < vertex_size ; j++ )		\
      tmp.f[j] = v->f[j];				\
   tmp.f[3] *= v->f[vertex_size];			\
   tmp.f[vertex_size-2] *= rhw;				\
   tmp.f[vertex_size-1] *= rhw;				\
} while (0)

static void via_ptex_tri (struct via_context *vmesa,
			  viaVertexPtr v0,
			  viaVertexPtr v1,
			  viaVertexPtr v2)
{
   GLuint vertsize = vmesa->hwVertexSize;
   GLuint *vb = viaExtendPrimitive(vmesa, 3*4*vertsize);
   viaVertex tmp;

   PTEX_VERTEX(tmp, vertsize, v0); COPY_DWORDS(vb, vertsize, &tmp);
   PTEX_VERTEX(tmp, vertsize, v1); COPY_DWORDS(vb, vertsize, &tmp);
   PTEX_VERTEX(tmp, vertsize, v2); COPY_DWORDS(vb, vertsize, &tmp);
}

static void via_ptex_line (struct via_context *vmesa,
			   viaVertexPtr v0,
			   viaVertexPtr v1)
{
   GLuint vertsize = vmesa->hwVertexSize;
   GLuint *vb = viaExtendPrimitive(vmesa, 2*4*vertsize);
   viaVertex tmp;

   PTEX_VERTEX(tmp, vertsize, v0); COPY_DWORDS(vb, vertsize, &tmp);
   PTEX_VERTEX(tmp, vertsize, v1); COPY_DWORDS(vb, vertsize, &tmp);
}

static void via_ptex_point (struct via_context *vmesa,
			    viaVertexPtr v0)
{
   GLuint vertsize = vmesa->hwVertexSize;
   GLuint *vb = viaExtendPrimitive(vmesa, 1*4*vertsize);
   viaVertex tmp;

   PTEX_VERTEX(tmp, vertsize, v0); COPY_DWORDS(vb, vertsize, &tmp);
}





/***********************************************************************
 *          Macros for via_dd_tritmp.h to draw basic primitives        *
 ***********************************************************************/

#define TRI(a, b, c)                                \
    do {                                            \
        if (DO_FALLBACK)                            \
            vmesa->drawTri(vmesa, a, b, c);         \
        else                                        \
            via_draw_triangle(vmesa, a, b, c);      \
    } while (0)

#define QUAD(a, b, c, d)                            \
    do {                                            \
        if (DO_FALLBACK) {                          \
            vmesa->drawTri(vmesa, a, b, d);         \
            vmesa->drawTri(vmesa, b, c, d);         \
        }                                           \
        else                                        \
            via_draw_quad(vmesa, a, b, c, d);       \
    } while (0)

#define LINE(v0, v1)                                \
    do {                                            \
        if (DO_FALLBACK)                            \
            vmesa->drawLine(vmesa, v0, v1);         \
        else                                        \
            via_draw_line(vmesa, v0, v1);           \
    } while (0)

#define POINT(v0)                                    \
    do {                                             \
        if (DO_FALLBACK)                             \
            vmesa->drawPoint(vmesa, v0);             \
        else                                         \
            via_draw_point(vmesa, v0);               \
    } while (0)


/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/

#define VIA_OFFSET_BIT         0x01
#define VIA_TWOSIDE_BIT        0x02
#define VIA_UNFILLED_BIT       0x04
#define VIA_FALLBACK_BIT       0x08
#define VIA_MAX_TRIFUNC        0x10


static struct {
    tnl_points_func          points;
    tnl_line_func            line;
    tnl_triangle_func        triangle;
    tnl_quad_func            quad;
} rast_tab[VIA_MAX_TRIFUNC + 1];


#define DO_FALLBACK (IND & VIA_FALLBACK_BIT)
#define DO_OFFSET   (IND & VIA_OFFSET_BIT)
#define DO_UNFILLED (IND & VIA_UNFILLED_BIT)
#define DO_TWOSIDE  (IND & VIA_TWOSIDE_BIT)
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
#define VERTEX            viaVertex
#define TAB               rast_tab

/* Only used to pull back colors into vertices (ie, we know color is
 * floating point).
 */
#define VIA_COLOR(dst, src)                     \
    do {                                        \
        dst[0] = src[2];                        \
        dst[1] = src[1];                        \
        dst[2] = src[0];                        \
        dst[3] = src[3];                        \
    } while (0)

#define VIA_SPEC(dst, src)                      \
    do {                                        \
        dst[0] = src[2];                        \
        dst[1] = src[1];                        \
        dst[2] = src[0];                        \
    } while (0)


#define DEPTH_SCALE vmesa->polygon_offset_scale
#define UNFILLED_TRI unfilled_tri
#define UNFILLED_QUAD unfilled_quad
#define VERT_X(_v) _v->v.x
#define VERT_Y(_v) _v->v.y
#define VERT_Z(_v) _v->v.z
#define AREA_IS_CCW(a) (a > 0)
#define GET_VERTEX(e) (vmesa->verts + (e * vmesa->vertexSize * sizeof(int)))

#define VERT_SET_RGBA( v, c )  					\
do {								\
   via_color_t *color = (via_color_t *)&((v)->ui[coloroffset]);	\
   UNCLAMPED_FLOAT_TO_UBYTE(color->red, (c)[0]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->green, (c)[1]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->blue, (c)[2]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->alpha, (c)[3]);		\
} while (0)

#define VERT_COPY_RGBA( v0, v1 ) v0->ui[coloroffset] = v1->ui[coloroffset]

#define VERT_SET_SPEC( v, c )					\
do {								\
   if (specoffset) {						\
     via_color_t *color = (via_color_t *)&((v)->ui[specoffset]);	\
     UNCLAMPED_FLOAT_TO_UBYTE(color->red, (c)[0]);		\
     UNCLAMPED_FLOAT_TO_UBYTE(color->green, (c)[1]);		\
     UNCLAMPED_FLOAT_TO_UBYTE(color->blue, (c)[2]);		\
   }								\
} while (0)
#define VERT_COPY_SPEC( v0, v1 )			\
do {							\
   if (specoffset) {					\
      v0->ub4[specoffset][0] = v1->ub4[specoffset][0];	\
      v0->ub4[specoffset][1] = v1->ub4[specoffset][1];	\
      v0->ub4[specoffset][2] = v1->ub4[specoffset][2];	\
   }							\
} while (0)


#define VERT_SAVE_RGBA( idx )    color[idx] = v[idx]->ui[coloroffset]
#define VERT_RESTORE_RGBA( idx ) v[idx]->ui[coloroffset] = color[idx]
#define VERT_SAVE_SPEC( idx )    if (specoffset) spec[idx] = v[idx]->ui[specoffset]
#define VERT_RESTORE_SPEC( idx ) if (specoffset) v[idx]->ui[specoffset] = spec[idx]


#define LOCAL_VARS(n)                                                   \
    struct via_context *vmesa = VIA_CONTEXT(ctx);                             \
    GLuint color[n], spec[n];                                           \
    GLuint coloroffset = vmesa->coloroffset;              \
    GLuint specoffset = vmesa->specoffset;                       \
    (void)color; (void)spec; (void)coloroffset; (void)specoffset;


/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/

static const GLenum hwPrim[GL_POLYGON + 2] = {
    GL_POINTS,
    GL_LINES,
    GL_LINES,
    GL_LINES,
    GL_TRIANGLES,
    GL_TRIANGLES,
    GL_TRIANGLES,
    GL_TRIANGLES,
    GL_TRIANGLES,
    GL_TRIANGLES,
    GL_POLYGON+1
};


#define RASTERIZE(x) viaRasterPrimitive( ctx, x, hwPrim[x] )
#define RENDER_PRIMITIVE vmesa->renderPrimitive
#define TAG(x) x
#define IND VIA_FALLBACK_BIT
#include "tnl_dd/t_dd_unfilled.h"
#undef IND
#undef RASTERIZE

/***********************************************************************
 *                      Generate GL render functions                   *
 ***********************************************************************/
#define RASTERIZE(x)

#define IND (0)
#define TAG(x) x
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_OFFSET_BIT)
#define TAG(x) x##_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_TWOSIDE_BIT|VIA_OFFSET_BIT)
#define TAG(x) x##_twoside_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_OFFSET_BIT|VIA_UNFILLED_BIT)
#define TAG(x) x##_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_TWOSIDE_BIT|VIA_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_TWOSIDE_BIT|VIA_OFFSET_BIT|VIA_UNFILLED_BIT)
#define TAG(x) x##_twoside_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_FALLBACK_BIT)
#define TAG(x) x##_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_OFFSET_BIT|VIA_FALLBACK_BIT)
#define TAG(x) x##_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_TWOSIDE_BIT|VIA_FALLBACK_BIT)
#define TAG(x) x##_twoside_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_TWOSIDE_BIT|VIA_OFFSET_BIT|VIA_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_UNFILLED_BIT|VIA_FALLBACK_BIT)
#define TAG(x) x##_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_OFFSET_BIT|VIA_UNFILLED_BIT|VIA_FALLBACK_BIT)
#define TAG(x) x##_offset_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_TWOSIDE_BIT|VIA_UNFILLED_BIT|VIA_FALLBACK_BIT)
#define TAG(x) x##_twoside_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_TWOSIDE_BIT|VIA_OFFSET_BIT|VIA_UNFILLED_BIT| \
             VIA_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"


/* Catchall case for flat, separate specular triangles (via has flat
 * diffuse shading, but always does specular color with gouraud).
 */
#undef  DO_FALLBACK
#undef  DO_OFFSET
#undef  DO_UNFILLED
#undef  DO_TWOSIDE
#undef  DO_FLAT
#define DO_FALLBACK (0)
#define DO_OFFSET   (ctx->_TriangleCaps & DD_TRI_OFFSET)
#define DO_UNFILLED (ctx->_TriangleCaps & DD_TRI_UNFILLED)
#define DO_TWOSIDE  (ctx->_TriangleCaps & DD_TRI_LIGHT_TWOSIDE)
#define DO_FLAT     1
#define TAG(x) x##_flat_specular
#define IND VIA_MAX_TRIFUNC
#include "tnl_dd/t_dd_tritmp.h"


static void init_rast_tab(void)
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

    init_flat_specular();	/* special! */
}


/***********************************************************************
 *                    Rasterization fallback helpers                   *
 ***********************************************************************/


/* This code is hit only when a mix of accelerated and unaccelerated
 * primitives are being drawn, and only for the unaccelerated
 * primitives.
 */
static void
via_fallback_tri(struct via_context *vmesa,
                 viaVertex *v0,
                 viaVertex *v1,
                 viaVertex *v2)
{    
    GLcontext *ctx = vmesa->glCtx;
    SWvertex v[3];
    _swsetup_Translate(ctx, v0, &v[0]);
    _swsetup_Translate(ctx, v1, &v[1]);
    _swsetup_Translate(ctx, v2, &v[2]);
    viaSpanRenderStart( ctx );
    _swrast_Triangle(ctx, &v[0], &v[1], &v[2]);
    viaSpanRenderFinish( ctx );
}


static void
via_fallback_line(struct via_context *vmesa,
                  viaVertex *v0,
                  viaVertex *v1)
{
    GLcontext *ctx = vmesa->glCtx;
    SWvertex v[2];
    _swsetup_Translate(ctx, v0, &v[0]);
    _swsetup_Translate(ctx, v1, &v[1]);
    viaSpanRenderStart( ctx );
    _swrast_Line(ctx, &v[0], &v[1]);
    viaSpanRenderFinish( ctx );
}


static void
via_fallback_point(struct via_context *vmesa,
                   viaVertex *v0)
{
    GLcontext *ctx = vmesa->glCtx;
    SWvertex v[1];
    _swsetup_Translate(ctx, v0, &v[0]);
    viaSpanRenderStart( ctx );
    _swrast_Point(ctx, &v[0]);
    viaSpanRenderFinish( ctx );
}

static void viaResetLineStipple( GLcontext *ctx )
{
   struct via_context *vmesa = VIA_CONTEXT(ctx);
   vmesa->regCmdB |= HC_HLPrst_MASK;
}

/**********************************************************************/
/*               Render unclipped begin/end objects                   */
/**********************************************************************/
#define IND 0
#define V(x) (viaVertex *)(vertptr + ((x) * vertsize * sizeof(int)))
#define RENDER_POINTS(start, count)   \
    for (; start < count; start++) POINT(V(ELT(start)));
#define RENDER_LINE(v0, v1)         LINE(V(v0), V(v1))
#define RENDER_TRI( v0, v1, v2)     TRI( V(v0), V(v1), V(v2))
#define RENDER_QUAD(v0, v1, v2, v3) QUAD(V(v0), V(v1), V(v2), V(v3))
#define INIT(x) viaRasterPrimitive(ctx, x, hwPrim[x])
#undef LOCAL_VARS
#define LOCAL_VARS                                              \
    struct via_context *vmesa = VIA_CONTEXT(ctx);                     \
    GLubyte *vertptr = (GLubyte *)vmesa->verts;                 \
    const GLuint vertsize = vmesa->vertexSize;          \
    const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;       \
   const GLboolean stipple = ctx->Line.StippleFlag;		\
   (void) elt; (void) stipple;
#define RESET_STIPPLE	if ( stipple ) viaResetLineStipple( ctx );
#define RESET_OCCLUSION
#define PRESERVE_VB_DEFS
#define ELT(x) x
#define TAG(x) via_##x##_verts
#include "tnl/t_vb_rendertmp.h"
#undef ELT
#undef TAG
#define TAG(x) via_##x##_elts
#define ELT(x) elt[x]
#include "tnl/t_vb_rendertmp.h"
#undef ELT
#undef TAG
#undef NEED_EDGEFLAG_SETUP
#undef EDGEFLAG_GET
#undef EDGEFLAG_SET
#undef RESET_OCCLUSION


/**********************************************************************/
/*                   Render clipped primitives                        */
/**********************************************************************/



static void viaRenderClippedPoly(GLcontext *ctx, const GLuint *elts,
                                 GLuint n)
{
    TNLcontext *tnl = TNL_CONTEXT(ctx);
    struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
    GLuint prim = VIA_CONTEXT(ctx)->renderPrimitive;

    /* Render the new vertices as an unclipped polygon.
     */
    {
        GLuint *tmp = VB->Elts;
        VB->Elts = (GLuint *)elts;
        tnl->Driver.Render.PrimTabElts[GL_POLYGON](ctx, 0, n,
                                                   PRIM_BEGIN|PRIM_END);
        VB->Elts = tmp;
    }

    /* Restore the render primitive
     */
    if (prim != GL_POLYGON &&
	prim != GL_POLYGON + 1)
       tnl->Driver.Render.PrimitiveNotify( ctx, prim );
}

static void viaRenderClippedLine(GLcontext *ctx, GLuint ii, GLuint jj)
{
    TNLcontext *tnl = TNL_CONTEXT(ctx);
    tnl->Driver.Render.Line(ctx, ii, jj);
}

static void viaFastRenderClippedPoly(GLcontext *ctx, const GLuint *elts,
                                     GLuint n)
{
    struct via_context *vmesa = VIA_CONTEXT(ctx);
    GLuint vertsize = vmesa->vertexSize;
    GLuint *vb = viaExtendPrimitive(vmesa, (n - 2) * 3 * 4 * vertsize);
    GLubyte *vertptr = (GLubyte *)vmesa->verts;
    const GLuint *start = (const GLuint *)V(elts[0]);
    int i;

    for (i = 2; i < n; i++) {
	COPY_DWORDS(vb, vertsize, V(elts[i - 1]));
        COPY_DWORDS(vb, vertsize, V(elts[i]));
	COPY_DWORDS(vb, vertsize, start);	
    }
}


/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/


#define _VIA_NEW_VERTEX (_NEW_TEXTURE |                         \
                         _DD_NEW_SEPARATE_SPECULAR |            \
                         _DD_NEW_TRI_UNFILLED |                 \
                         _DD_NEW_TRI_LIGHT_TWOSIDE |            \
                         _NEW_FOG)

#define _VIA_NEW_RENDERSTATE (_DD_NEW_LINE_STIPPLE |            \
                              _DD_NEW_TRI_UNFILLED |            \
                              _DD_NEW_TRI_LIGHT_TWOSIDE |       \
                              _DD_NEW_TRI_OFFSET |              \
                              _DD_NEW_TRI_STIPPLE |             \
                              _NEW_POLYGONSTIPPLE)


static void viaChooseRenderState(GLcontext *ctx)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct via_context *vmesa = VIA_CONTEXT(ctx);
   GLuint flags = ctx->_TriangleCaps;
   GLuint index = 0;

   if (vmesa->ptexHack) {
      vmesa->drawPoint = via_ptex_point;
      vmesa->drawLine = via_ptex_line;
      vmesa->drawTri = via_ptex_tri;
      index |= VIA_FALLBACK_BIT;
   }
   else {
      vmesa->drawPoint = via_draw_point;
      vmesa->drawLine = via_draw_line;
      vmesa->drawTri = via_draw_triangle;
   }

   if (flags & (ANY_FALLBACK_FLAGS | ANY_RASTER_FLAGS)) {
      if (ctx->Light.Enabled && ctx->Light.Model.TwoSide)
         index |= VIA_TWOSIDE_BIT;
      if (ctx->Polygon.FrontMode != GL_FILL || ctx->Polygon.BackMode != GL_FILL)
         index |= VIA_UNFILLED_BIT;
      if (flags & DD_TRI_OFFSET)
         index |= VIA_OFFSET_BIT;
      if (flags & ANY_FALLBACK_FLAGS)
         index |= VIA_FALLBACK_BIT;

      /* Hook in fallbacks for specific primitives. */
      if (flags & POINT_FALLBACK)
	 vmesa->drawPoint = via_fallback_point;
      
      if (flags & LINE_FALLBACK)
	 vmesa->drawLine = via_fallback_line;

      if (flags & TRI_FALLBACK)
	 vmesa->drawTri = via_fallback_tri;
   }

   if ((flags & DD_SEPARATE_SPECULAR) && ctx->Light.ShadeModel == GL_FLAT)
      index = VIA_MAX_TRIFUNC;	/* flat specular */

   if (vmesa->renderIndex != index) {
      vmesa->renderIndex = index;

      tnl->Driver.Render.Points = rast_tab[index].points;
      tnl->Driver.Render.Line = rast_tab[index].line;
      tnl->Driver.Render.Triangle = rast_tab[index].triangle;
      tnl->Driver.Render.Quad = rast_tab[index].quad;

      if (index == 0) {
	 tnl->Driver.Render.PrimTabVerts = via_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = via_render_tab_elts;
	 tnl->Driver.Render.ClippedLine = line; /* from tritmp.h */
	 tnl->Driver.Render.ClippedPolygon = viaFastRenderClippedPoly;
      }
      else {
	 tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
	 tnl->Driver.Render.ClippedLine = viaRenderClippedLine;
	 tnl->Driver.Render.ClippedPolygon = viaRenderClippedPoly;
      }
   }
}


#define VIA_EMIT_TEX1	0x01
#define VIA_EMIT_TEX0	0x02
#define VIA_EMIT_PTEX0	0x04
#define VIA_EMIT_RGBA	0x08
#define VIA_EMIT_SPEC	0x10
#define VIA_EMIT_FOG	0x20
#define VIA_EMIT_W	0x40

#define EMIT_ATTR( ATTR, STYLE, INDEX, REGB )				\
do {									\
   vmesa->vertex_attrs[vmesa->vertex_attr_count].attrib = (ATTR);	\
   vmesa->vertex_attrs[vmesa->vertex_attr_count].format = (STYLE);	\
   vmesa->vertex_attr_count++;						\
   setupIndex |= (INDEX);						\
   regCmdB |= (REGB);							\
} while (0)

#define EMIT_PAD( N )							\
do {									\
   vmesa->vertex_attrs[vmesa->vertex_attr_count].attrib = 0;		\
   vmesa->vertex_attrs[vmesa->vertex_attr_count].format = EMIT_PAD;	\
   vmesa->vertex_attrs[vmesa->vertex_attr_count].offset = (N);		\
   vmesa->vertex_attr_count++;						\
} while (0)



static void viaChooseVertexState( GLcontext *ctx )
{
   struct via_context *vmesa = VIA_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   DECLARE_RENDERINPUTS(index_bitset);
   GLuint regCmdB = HC_HVPMSK_X | HC_HVPMSK_Y | HC_HVPMSK_Z;
   GLuint setupIndex = 0;

   RENDERINPUTS_COPY( index_bitset, tnl->render_inputs_bitset );
   vmesa->vertex_attr_count = 0;
 
   /* EMIT_ATTR's must be in order as they tell t_vertex.c how to
    * build up a hardware vertex.
    */
   if (RENDERINPUTS_TEST_RANGE( index_bitset, _TNL_FIRST_TEX, _TNL_LAST_TEX ) ||
       RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_FOG )) {
      EMIT_ATTR( _TNL_ATTRIB_POS, EMIT_4F_VIEWPORT, VIA_EMIT_W, HC_HVPMSK_W );
      vmesa->coloroffset = 4;
   }
   else {
      EMIT_ATTR( _TNL_ATTRIB_POS, EMIT_3F_VIEWPORT, 0, 0 );
      vmesa->coloroffset = 3;
   }

   /* t_context.c always includes a diffuse color */
   EMIT_ATTR( _TNL_ATTRIB_COLOR0, EMIT_4UB_4F_BGRA, VIA_EMIT_RGBA, 
	      HC_HVPMSK_Cd );
      
   vmesa->specoffset = 0;
   if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_COLOR1 ) ||
       RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_FOG )) {
      if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_COLOR1 )) {
	 vmesa->specoffset = vmesa->coloroffset + 1;
	 EMIT_ATTR( _TNL_ATTRIB_COLOR1, EMIT_3UB_3F_BGR, VIA_EMIT_SPEC, 
		    HC_HVPMSK_Cs );
      }
      else
	 EMIT_PAD( 3 );

      if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_FOG ))
	 EMIT_ATTR( _TNL_ATTRIB_FOG, EMIT_1UB_1F, VIA_EMIT_FOG, HC_HVPMSK_Cs );
      else
	 EMIT_PAD( 1 );
   }

   if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_TEX0 )) {
      if (vmesa->ptexHack)
	 EMIT_ATTR( _TNL_ATTRIB_TEX0, EMIT_3F_XYW, VIA_EMIT_PTEX0, 
		    (HC_HVPMSK_S | HC_HVPMSK_T) );
      else 
	 EMIT_ATTR( _TNL_ATTRIB_TEX0, EMIT_2F, VIA_EMIT_TEX0, 
		    (HC_HVPMSK_S | HC_HVPMSK_T) );
   }

   if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_TEX1 )) {
      EMIT_ATTR( _TNL_ATTRIB_TEX1, EMIT_2F, VIA_EMIT_TEX1, 
		 (HC_HVPMSK_S | HC_HVPMSK_T) );
   }

   if (setupIndex != vmesa->setupIndex) {
      vmesa->vertexSize = _tnl_install_attrs( ctx, 
					       vmesa->vertex_attrs, 
					       vmesa->vertex_attr_count,
					       vmesa->ViewportMatrix.m, 0 );
      vmesa->vertexSize >>= 2;
      vmesa->setupIndex = setupIndex;
      vmesa->regCmdB &= ~HC_HVPMSK_MASK;
      vmesa->regCmdB |= regCmdB;

      if (vmesa->ptexHack) 
	 vmesa->hwVertexSize = vmesa->vertexSize - 1;
      else
	 vmesa->hwVertexSize = vmesa->vertexSize;
   }
}




/* Check if projective texture coordinates are used and if we can fake
 * them. Fallback to swrast if we can't. Returns GL_TRUE if projective
 * texture coordinates must be faked, GL_FALSE otherwise.
 */
static GLboolean viaCheckPTexHack( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   DECLARE_RENDERINPUTS(index_bitset);
   GLboolean fallback = GL_FALSE;
   GLboolean ptexHack = GL_FALSE;

   RENDERINPUTS_COPY( index_bitset, tnl->render_inputs_bitset );

   if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_TEX0 ) && VB->TexCoordPtr[0]->size == 4) {
      if (!RENDERINPUTS_TEST_RANGE( index_bitset, _TNL_ATTRIB_TEX1, _TNL_LAST_TEX ))
	 ptexHack = GL_TRUE; 
      else
	 fallback = GL_TRUE;
   }
   if (RENDERINPUTS_TEST( index_bitset, _TNL_ATTRIB_TEX1 ) && VB->TexCoordPtr[1]->size == 4)
      fallback = GL_TRUE;

   FALLBACK(VIA_CONTEXT(ctx), VIA_FALLBACK_PROJ_TEXTURE, fallback);
   return ptexHack;
}




/**********************************************************************/
/*                 High level hooks for t_vb_render.c                 */
/**********************************************************************/


static void viaRenderStart(GLcontext *ctx)
{
   struct via_context *vmesa = VIA_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   {
      GLboolean ptexHack = viaCheckPTexHack( ctx );
      if (ptexHack != vmesa->ptexHack) {
	 vmesa->ptexHack = ptexHack;
	 vmesa->newRenderState |= _VIA_NEW_RENDERSTATE;
      }
   }

   if (vmesa->newState) {
      vmesa->newRenderState |= vmesa->newState;
      viaValidateState( ctx );
   }

   if (vmesa->Fallback) {
      tnl->Driver.Render.Start(ctx);
      return;
   }

   if (vmesa->newRenderState) {
      viaChooseVertexState(ctx);
      viaChooseRenderState(ctx);
      vmesa->newRenderState = 0;
   }

   /* Important:
    */
   VB->AttribPtr[VERT_ATTRIB_POS] = VB->NdcPtr;
}

static void viaRenderFinish(GLcontext *ctx)
{
   VIA_FINISH_PRIM(VIA_CONTEXT(ctx));
}


/* System to flush dma and emit state changes based on the rasterized
 * primitive.
 */
void viaRasterPrimitive(GLcontext *ctx,
			GLenum glprim,
			GLenum hwprim)
{
   struct via_context *vmesa = VIA_CONTEXT(ctx);
   GLuint regCmdB;
   RING_VARS;

   if (VIA_DEBUG & DEBUG_PRIMS) 
      fprintf(stderr, "%s: %s/%s/%s\n", 
	      __FUNCTION__, _mesa_lookup_enum_by_nr(glprim),
	      _mesa_lookup_enum_by_nr(hwprim),
	      _mesa_lookup_enum_by_nr(ctx->Light.ShadeModel));

   assert (!vmesa->newState);

   vmesa->renderPrimitive = glprim;

   if (hwprim != vmesa->hwPrimitive ||
       ctx->Light.ShadeModel != vmesa->hwShadeModel) {

      VIA_FINISH_PRIM(vmesa);

      /* Ensure no wrapping inside this function  */    
      viaCheckDma( vmesa, 1024 );	

      if (vmesa->newEmitState) {
	 viaEmitState(vmesa);
      }
       
      vmesa->regCmdA_End = HC_ACMD_HCmdA;

      if (ctx->Light.ShadeModel == GL_SMOOTH) {
	 vmesa->regCmdA_End |= HC_HShading_Gouraud;
      }
      
      vmesa->hwShadeModel = ctx->Light.ShadeModel;
      regCmdB = vmesa->regCmdB;

      switch (hwprim) {
      case GL_POINTS:
	 vmesa->regCmdA_End |= HC_HPMType_Point | HC_HVCycle_Full;
	 vmesa->regCmdA_End |= HC_HShading_Gouraud; /* always Gouraud 
						       shade points?!? */
	 break;
      case GL_LINES:
	 vmesa->regCmdA_End |= HC_HPMType_Line | HC_HVCycle_Full;
         regCmdB |= HC_HLPrst_MASK;
	 if (ctx->Light.ShadeModel == GL_FLAT)
            vmesa->regCmdA_End |= HC_HShading_FlatB; 
	 break;
      case GL_LINE_LOOP:
      case GL_LINE_STRIP:
	 vmesa->regCmdA_End |= HC_HPMType_Line | HC_HVCycle_AFP |
	    HC_HVCycle_AB | HC_HVCycle_NewB;
	 regCmdB |= HC_HVCycle_AB | HC_HVCycle_NewB | HC_HLPrst_MASK;
	 if (ctx->Light.ShadeModel == GL_FLAT)
            vmesa->regCmdA_End |= HC_HShading_FlatB; 
	 break;
      case GL_TRIANGLES:
	 vmesa->regCmdA_End |= HC_HPMType_Tri | HC_HVCycle_Full;
	 if (ctx->Light.ShadeModel == GL_FLAT)
            vmesa->regCmdA_End |= HC_HShading_FlatC; 
	 break;
      case GL_TRIANGLE_STRIP:
	 vmesa->regCmdA_End |= HC_HPMType_Tri | HC_HVCycle_AFP |
	    HC_HVCycle_AC | HC_HVCycle_BB | HC_HVCycle_NewC;
	 regCmdB |= HC_HVCycle_AA | HC_HVCycle_BC | HC_HVCycle_NewC;
	 if (ctx->Light.ShadeModel == GL_FLAT)
            vmesa->regCmdA_End |= HC_HShading_FlatC; 
	 break;
      case GL_TRIANGLE_FAN:
	 vmesa->regCmdA_End |= HC_HPMType_Tri | HC_HVCycle_AFP |
	    HC_HVCycle_AA | HC_HVCycle_BC | HC_HVCycle_NewC;
	 regCmdB |= HC_HVCycle_AA | HC_HVCycle_BC | HC_HVCycle_NewC;
	 if (ctx->Light.ShadeModel == GL_FLAT)
            vmesa->regCmdA_End |= HC_HShading_FlatC; 
	 break;
      case GL_QUADS:
	 abort();
	 return;
      case GL_QUAD_STRIP:
	 abort();
	 return;
      case GL_POLYGON:
	 vmesa->regCmdA_End |= HC_HPMType_Tri | HC_HVCycle_AFP |
	    HC_HVCycle_AA | HC_HVCycle_BC | HC_HVCycle_NewC;
	 regCmdB |= HC_HVCycle_AA | HC_HVCycle_BC | HC_HVCycle_NewC;
	 if (ctx->Light.ShadeModel == GL_FLAT)
            vmesa->regCmdA_End |= HC_HShading_FlatC; 
	 break;                          
      default:
	 abort();
	 return;
      }
    
/*     assert((vmesa->dmaLow & 0x4) == 0); */

      if (vmesa->dmaCliprectAddr == ~0) {
	 if (VIA_DEBUG & DEBUG_DMA) 
	    fprintf(stderr, "reserve cliprect space at %x\n", vmesa->dmaLow);
	 vmesa->dmaCliprectAddr = vmesa->dmaLow;
	 BEGIN_RING(8);
	 OUT_RING( HC_HEADER2 );    
	 OUT_RING( (HC_ParaType_NotTex << 16) );
	 OUT_RING( 0xCCCCCCCC );
	 OUT_RING( 0xCCCCCCCC );
	 OUT_RING( 0xCCCCCCCC );
	 OUT_RING( 0xCCCCCCCC );
	 OUT_RING( 0xCCCCCCCC );
	 OUT_RING( 0xCCCCCCCC );
	 ADVANCE_RING();
      }

      assert(vmesa->dmaLastPrim == 0);

      BEGIN_RING(8);
      OUT_RING( HC_HEADER2 );    
      OUT_RING( (HC_ParaType_NotTex << 16) );
      OUT_RING( 0xCCCCCCCC );
      OUT_RING( 0xDDDDDDDD );

      OUT_RING( HC_HEADER2 );    
      OUT_RING( (HC_ParaType_CmdVdata << 16) );
      OUT_RING( regCmdB );
      OUT_RING( vmesa->regCmdA_End );
      ADVANCE_RING();

      vmesa->hwPrimitive = hwprim;        
      vmesa->dmaLastPrim = vmesa->dmaLow;
   }
   else {
      assert(!vmesa->newEmitState);
   }
}

/* Callback for mesa:
 */
static void viaRenderPrimitive( GLcontext *ctx, GLuint prim )
{
   viaRasterPrimitive( ctx, prim, hwPrim[prim] );
}


void viaFinishPrimitive(struct via_context *vmesa)
{
   if (VIA_DEBUG & (DEBUG_DMA|DEBUG_PRIMS)) 
      fprintf(stderr, "%s\n", __FUNCTION__);

   if (!vmesa->dmaLastPrim || vmesa->dmaCliprectAddr == ~0) {
      assert(0);
   }
   else if (vmesa->dmaLow != vmesa->dmaLastPrim) {
      GLuint cmdA = (vmesa->regCmdA_End | HC_HPLEND_MASK | 
		     HC_HPMValidN_MASK | HC_HE3Fire_MASK); 
      RING_VARS;

      vmesa->dmaLastPrim = 0;

      /* KW: modified 0x1 to 0x4 below:
       */
      if ((vmesa->dmaLow & 0x4) || !vmesa->useAgp) {
	 BEGIN_RING_NOCHECK( 1 );
	 OUT_RING( cmdA );
	 ADVANCE_RING();
      }   
      else {      
	 BEGIN_RING_NOCHECK( 2 );
	 OUT_RING( cmdA );
	 OUT_RING( cmdA );
	 ADVANCE_RING();
      }   

      if (vmesa->dmaLow > VIA_DMA_HIGHWATER)
	 viaFlushDma( vmesa );
   }
   else {
      if (VIA_DEBUG & (DEBUG_DMA|DEBUG_PRIMS)) 
	 fprintf(stderr, "remove empty primitive\n");

      /* Remove the primitive header:
       */
      vmesa->dmaLastPrim = 0;
      vmesa->dmaLow -= 8 * sizeof(GLuint);

      /* Maybe remove the cliprect as well:
       */
      if (vmesa->dmaCliprectAddr == vmesa->dmaLow - 8 * sizeof(GLuint)) {
	 vmesa->dmaLow -= 8 * sizeof(GLuint);
	 vmesa->dmaCliprectAddr = ~0;
      }
   }

   vmesa->renderPrimitive = GL_POLYGON + 1;
   vmesa->hwPrimitive = GL_POLYGON + 1;
   vmesa->dmaLastPrim = 0;
}


/**********************************************************************/
/*           Transition to/from hardware rasterization.               */
/**********************************************************************/


void viaFallback(struct via_context *vmesa, GLuint bit, GLboolean mode)
{
    GLcontext *ctx = vmesa->glCtx;
    TNLcontext *tnl = TNL_CONTEXT(ctx);
    GLuint oldfallback = vmesa->Fallback;
    
    if (mode) {
        vmesa->Fallback |= bit;
        if (oldfallback == 0) {
	    VIA_FLUSH_DMA(vmesa);

 	    if (VIA_DEBUG & DEBUG_FALLBACKS) 
	       fprintf(stderr, "ENTER FALLBACK %x\n", bit);

            _swsetup_Wakeup(ctx);
            vmesa->renderIndex = ~0;
        }
    }
    else {
        vmesa->Fallback &= ~bit;
        if (oldfallback == bit) {
	    _swrast_flush( ctx );

 	    if (VIA_DEBUG & DEBUG_FALLBACKS) 
	       fprintf(stderr, "LEAVE FALLBACK %x\n", bit);

	    tnl->Driver.Render.Start = viaRenderStart;
            tnl->Driver.Render.PrimitiveNotify = viaRenderPrimitive;
            tnl->Driver.Render.Finish = viaRenderFinish;

	    tnl->Driver.Render.BuildVertices = _tnl_build_vertices;
	    tnl->Driver.Render.CopyPV = _tnl_copy_pv;
	    tnl->Driver.Render.Interp = _tnl_interp;
    	    tnl->Driver.Render.ResetLineStipple = viaResetLineStipple;

	    _tnl_invalidate_vertex_state( ctx, ~0 );
	    _tnl_invalidate_vertices( ctx, ~0 );
	    _tnl_install_attrs( ctx, 
				vmesa->vertex_attrs, 
				vmesa->vertex_attr_count,
				vmesa->ViewportMatrix.m, 0 ); 

            vmesa->newState |= (_VIA_NEW_RENDERSTATE|_VIA_NEW_VERTEX);
        }
    }    
}

static void viaRunPipeline( GLcontext *ctx )
{
   struct via_context *vmesa = VIA_CONTEXT(ctx);

   if (vmesa->newState) {
      vmesa->newRenderState |= vmesa->newState;
      viaValidateState( ctx );
   }

   _tnl_run_pipeline( ctx );
}


/**********************************************************************/
/*                            Initialization.                         */
/**********************************************************************/


void viaInitTriFuncs(GLcontext *ctx)
{
    struct via_context *vmesa = VIA_CONTEXT(ctx);
    TNLcontext *tnl = TNL_CONTEXT(ctx);
    static int firsttime = 1;

    if (firsttime) {
        init_rast_tab();
        firsttime = 0;
    }

    tnl->Driver.RunPipeline = viaRunPipeline;
    tnl->Driver.Render.Start = viaRenderStart;
    tnl->Driver.Render.Finish = viaRenderFinish;
    tnl->Driver.Render.PrimitiveNotify = viaRenderPrimitive;
    tnl->Driver.Render.ResetLineStipple = viaResetLineStipple;
    tnl->Driver.Render.BuildVertices = _tnl_build_vertices;
    tnl->Driver.Render.CopyPV = _tnl_copy_pv;
    tnl->Driver.Render.Interp = _tnl_interp;

    _tnl_init_vertices( ctx, ctx->Const.MaxArrayLockSize + 12, 
			(6 + 2*ctx->Const.MaxTextureUnits) * sizeof(GLfloat) );
   
    vmesa->verts = (GLubyte *)tnl->clipspace.vertex_buf;

}
