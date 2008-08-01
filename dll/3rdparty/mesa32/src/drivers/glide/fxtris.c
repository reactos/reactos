/*
 * Mesa 3-D graphics library
 * Version:  4.0
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 *    Daniel Borca <dborca@users.sourceforge.net>
 */

#include "glheader.h"

#ifdef FX

#include "imports.h"
#include "mtypes.h"
#include "macros.h"
#include "colormac.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#include "fxdrv.h"


static GLboolean fxMultipass_ColorSum (GLcontext *ctx, GLuint pass);


/*
 * Subpixel offsets to adjust Mesa's (true) window coordinates to
 * Glide coordinates.  We need these to ensure precise rasterization.
 * Otherwise, we'll fail a bunch of conformance tests.
 */
#define TRI_X_OFFSET    ( 0.0F)
#define TRI_Y_OFFSET    ( 0.0F)
#define LINE_X_OFFSET   ( 0.0F)
#define LINE_Y_OFFSET   ( 0.125F)
#define PNT_X_OFFSET    ( 0.375F)
#define PNT_Y_OFFSET    ( 0.375F)

static void fxRasterPrimitive( GLcontext *ctx, GLenum prim );
static void fxRenderPrimitive( GLcontext *ctx, GLenum prim );

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

/***********************************************************************
 *          Macros for t_dd_tritmp.h to draw basic primitives          *
 ***********************************************************************/

#define TRI( a, b, c )				\
do {						\
   if (DO_FALLBACK)				\
      fxMesa->draw_tri( fxMesa, a, b, c );	\
   else						\
      grDrawTriangle( a, b, c );	\
} while (0)					\

#define QUAD( a, b, c, d )			\
do {						\
   if (DO_FALLBACK) {				\
      fxMesa->draw_tri( fxMesa, a, b, d );	\
      fxMesa->draw_tri( fxMesa, b, c, d );	\
   } else {					\
      GrVertex *_v_[4];				\
      _v_[0] = d;				\
      _v_[1] = a;				\
      _v_[2] = b;				\
      _v_[3] = c;				\
      grDrawVertexArray(GR_TRIANGLE_FAN, 4, _v_);\
      /*grDrawTriangle( a, b, d );*/		\
      /*grDrawTriangle( b, c, d );*/		\
   }						\
} while (0)

#define LINE( v0, v1 )				\
do {						\
   if (DO_FALLBACK)				\
      fxMesa->draw_line( fxMesa, v0, v1 );	\
   else {					\
      v0->x += LINE_X_OFFSET - TRI_X_OFFSET;	\
      v0->y += LINE_Y_OFFSET - TRI_Y_OFFSET;	\
      v1->x += LINE_X_OFFSET - TRI_X_OFFSET;	\
      v1->y += LINE_Y_OFFSET - TRI_Y_OFFSET;	\
      grDrawLine( v0, v1 );	\
      v0->x -= LINE_X_OFFSET - TRI_X_OFFSET;	\
      v0->y -= LINE_Y_OFFSET - TRI_Y_OFFSET;	\
      v1->x -= LINE_X_OFFSET - TRI_X_OFFSET;	\
      v1->y -= LINE_Y_OFFSET - TRI_Y_OFFSET;	\
   }						\
} while (0)

#define POINT( v0 )				\
do {						\
   if (DO_FALLBACK)				\
      fxMesa->draw_point( fxMesa, v0 );		\
   else {					\
      v0->x += PNT_X_OFFSET - TRI_X_OFFSET;	\
      v0->y += PNT_Y_OFFSET - TRI_Y_OFFSET;	\
      grDrawPoint( v0 );		\
      v0->x -= PNT_X_OFFSET - TRI_X_OFFSET;	\
      v0->y -= PNT_Y_OFFSET - TRI_Y_OFFSET;	\
   }						\
} while (0)


/***********************************************************************
 *              Fallback to swrast for basic primitives                *
 ***********************************************************************/

/* Build an SWvertex from a hardware vertex.
 *
 * This code is hit only when a mix of accelerated and unaccelerated
 * primitives are being drawn, and only for the unaccelerated
 * primitives.
 */
static void
fx_translate_vertex( GLcontext *ctx, const GrVertex *src, SWvertex *dst)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLuint ts0 = fxMesa->tmu_source[0];
   GLuint ts1 = fxMesa->tmu_source[1];
   GLfloat w = 1.0F / src->oow;

   dst->win[0] = src->x;
   dst->win[1] = src->y;
   dst->win[2] = src->ooz;
   dst->win[3] = src->oow;

#if FX_PACKEDCOLOR
   dst->color[0] = src->pargb[2];
   dst->color[1] = src->pargb[1];
   dst->color[2] = src->pargb[0];
   dst->color[3] = src->pargb[3];

   dst->specular[0] = src->pspec[2];
   dst->specular[1] = src->pspec[1];
   dst->specular[2] = src->pspec[0];
#else  /* !FX_PACKEDCOLOR */
   dst->color[0] = src->r;
   dst->color[1] = src->g;
   dst->color[2] = src->b;
   dst->color[3] = src->a;

   dst->specular[0] = src->r1;
   dst->specular[1] = src->g1;
   dst->specular[2] = src->g1;
#endif /* !FX_PACKEDCOLOR */

   dst->texcoord[ts0][0] = fxMesa->inv_s0scale * src->tmuvtx[0].sow * w;
   dst->texcoord[ts0][1] = fxMesa->inv_t0scale * src->tmuvtx[0].tow * w;

   if (fxMesa->stw_hint_state & GR_STWHINT_W_DIFF_TMU0)
      dst->texcoord[ts0][3] = src->tmuvtx[0].oow * w;
   else
      dst->texcoord[ts0][3] = 1.0F;

   if (fxMesa->SetupIndex & SETUP_TMU1) {
      dst->texcoord[ts1][0] = fxMesa->inv_s1scale * src->tmuvtx[1].sow * w;
      dst->texcoord[ts1][1] = fxMesa->inv_t1scale * src->tmuvtx[1].tow * w;

      if (fxMesa->stw_hint_state & GR_STWHINT_W_DIFF_TMU1)
	 dst->texcoord[ts1][3] = src->tmuvtx[1].oow * w;
      else
	 dst->texcoord[ts1][3] = 1.0F;
   }

   dst->pointSize = src->psize;
}


static void
fx_fallback_tri( fxMesaContext fxMesa,
		   GrVertex *v0,
		   GrVertex *v1,
		   GrVertex *v2 )
{
   GLcontext *ctx = fxMesa->glCtx;
   SWvertex v[3];

   fx_translate_vertex( ctx, v0, &v[0] );
   fx_translate_vertex( ctx, v1, &v[1] );
   fx_translate_vertex( ctx, v2, &v[2] );
   _swrast_Triangle( ctx, &v[0], &v[1], &v[2] );
}


static void
fx_fallback_line( fxMesaContext fxMesa,
		    GrVertex *v0,
		    GrVertex *v1 )
{
   GLcontext *ctx = fxMesa->glCtx;
   SWvertex v[2];
   fx_translate_vertex( ctx, v0, &v[0] );
   fx_translate_vertex( ctx, v1, &v[1] );
   _swrast_Line( ctx, &v[0], &v[1] );
}


static void
fx_fallback_point( fxMesaContext fxMesa,
		     GrVertex *v0 )
{
   GLcontext *ctx = fxMesa->glCtx;
   SWvertex v[1];
   fx_translate_vertex( ctx, v0, &v[0] );
   _swrast_Point( ctx, &v[0] );
}

/***********************************************************************
 *                 Functions to draw basic primitives                  *
 ***********************************************************************/

static void fx_print_vertex( GLcontext *ctx, const GrVertex *v )
{
 fprintf(stderr, "fx_print_vertex:\n");

 fprintf(stderr, "\tvertex at %p\n", (void *) v);

 fprintf(stderr, "\tx %f y %f z %f oow %f\n", v->x, v->y, v->ooz, v->oow);
#if FX_PACKEDCOLOR
 fprintf(stderr, "\tr %d g %d b %d a %d\n", v->pargb[2], v->pargb[1], v->pargb[0], v->pargb[3]);
#else  /* !FX_PACKEDCOLOR */
 fprintf(stderr, "\tr %f g %f b %f a %f\n", v->r, v->g, v->b, v->a);
#endif /* !FX_PACKEDCOLOR */

 fprintf(stderr, "\n");
}

#define DO_FALLBACK 0

/* Need to do clip loop at each triangle when mixing swrast and hw
 * rendering.  These functions are only used when mixed-mode rendering
 * is occurring.
 */
static void fx_draw_triangle( fxMesaContext fxMesa,
				GrVertex *v0,
				GrVertex *v1,
				GrVertex *v2 )
{
   BEGIN_CLIP_LOOP();
   TRI( v0, v1, v2 );
   END_CLIP_LOOP();
}

static void fx_draw_line( fxMesaContext fxMesa,
			    GrVertex *v0,
			    GrVertex *v1 )
{
   /* No support for wide lines (avoid wide/aa line fallback).
    */
   BEGIN_CLIP_LOOP();
   LINE(v0, v1);
   END_CLIP_LOOP();
}

static void fx_draw_point( fxMesaContext fxMesa,
			     GrVertex *v0 )
{
   /* No support for wide points.
    */
   BEGIN_CLIP_LOOP();
   POINT( v0 );
   END_CLIP_LOOP();
}

#ifndef M_2PI
#define M_2PI 6.28318530717958647692528676655901
#endif
#define __GL_COSF cos
#define __GL_SINF sin
static void fx_draw_point_sprite ( fxMesaContext fxMesa,
				   GrVertex *v0, GLfloat psize )
{
 const GLcontext *ctx = fxMesa->glCtx;

 GLfloat radius;
 GrVertex _v_[4];
 GLuint ts0 = fxMesa->tmu_source[0];
 GLuint ts1 = fxMesa->tmu_source[1];
 GLfloat w = v0->oow;
 GLfloat u0scale = fxMesa->s0scale * w;
 GLfloat v0scale = fxMesa->t0scale * w;
 GLfloat u1scale = fxMesa->s1scale * w;
 GLfloat v1scale = fxMesa->t1scale * w;

 radius = psize / 2.0F;
 _v_[0] = *v0;
 _v_[1] = *v0;
 _v_[2] = *v0;
 _v_[3] = *v0;
 /* CLIP_LOOP ?!? */
 /* point coverage? */
 /* we don't care about culling here (see fxSetupCull) */

 if (ctx->Point.SpriteOrigin == GL_UPPER_LEFT) {
    _v_[0].x -= radius;
    _v_[0].y += radius;
    _v_[1].x += radius;
    _v_[1].y += radius;
    _v_[2].x += radius;
    _v_[2].y -= radius;
    _v_[3].x -= radius;
    _v_[3].y -= radius;
 } else {
    _v_[0].x -= radius;
    _v_[0].y -= radius;
    _v_[1].x += radius;
    _v_[1].y -= radius;
    _v_[2].x += radius;
    _v_[2].y += radius;
    _v_[3].x -= radius;
    _v_[3].y += radius;
 }

 if (ctx->Point.CoordReplace[ts0]) {
    _v_[0].tmuvtx[0].sow = 0;
    _v_[0].tmuvtx[0].tow = 0;
    _v_[1].tmuvtx[0].sow = u0scale;
    _v_[1].tmuvtx[0].tow = 0;
    _v_[2].tmuvtx[0].sow = u0scale;
    _v_[2].tmuvtx[0].tow = v0scale;
    _v_[3].tmuvtx[0].sow = 0;
    _v_[3].tmuvtx[0].tow = v0scale;
 }
 if (ctx->Point.CoordReplace[ts1]) {
    _v_[0].tmuvtx[1].sow = 0;
    _v_[0].tmuvtx[1].tow = 0;
    _v_[1].tmuvtx[1].sow = u1scale;
    _v_[1].tmuvtx[1].tow = 0;
    _v_[2].tmuvtx[1].sow = u1scale;
    _v_[2].tmuvtx[1].tow = v1scale;
    _v_[3].tmuvtx[1].sow = 0;
    _v_[3].tmuvtx[1].tow = v1scale;
 }

 grDrawVertexArrayContiguous(GR_TRIANGLE_FAN, 4, _v_, sizeof(GrVertex));
}

static void fx_draw_point_wide ( fxMesaContext fxMesa,
			         GrVertex *v0 )
{
 GLint i, n;
 GLfloat ang, radius, oon;
 GrVertex vtxB, vtxC;
 GrVertex *_v_[3];

 const GLcontext *ctx = fxMesa->glCtx;
 const GLfloat psize = (ctx->_TriangleCaps & DD_POINT_ATTEN)
                       ? CLAMP(v0->psize, ctx->Point.MinSize, ctx->Point.MaxSize)
                       : ctx->Point._Size; /* clamped */

 if (ctx->Point.PointSprite) {
    fx_draw_point_sprite(fxMesa, v0, psize);
    return;
 }

 _v_[0] = v0;
 _v_[1] = &vtxB;
 _v_[2] = &vtxC;

 radius = psize / 2.0F;
 n = IROUND(psize * 2); /* radius x 4 */
 if (n < 4) n = 4;
 oon = 1.0F / (GLfloat)n;

 /* CLIP_LOOP ?!? */
 /* point coverage? */
 /* we don't care about culling here (see fxSetupCull) */

 vtxB = *v0;
 vtxC = *v0;

 vtxB.x += radius;
 ang = M_2PI * oon;
 vtxC.x += radius * __GL_COSF(ang);
 vtxC.y += radius * __GL_SINF(ang);
 grDrawVertexArray(GR_TRIANGLE_FAN, 3, _v_);
 for (i = 2; i <= n; i++) {
     ang = M_2PI * i * oon;
     vtxC.x = v0->x + radius * __GL_COSF(ang);
     vtxC.y = v0->y + radius * __GL_SINF(ang);
     grDrawVertexArray(GR_TRIANGLE_FAN_CONTINUE, 1, &_v_[2]);
 }
}

static void fx_render_pw_verts( GLcontext *ctx,
				GLuint start,
				GLuint count,
				GLuint flags )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GrVertex *fxVB = fxMesa->verts;
   (void) flags;

   fxRenderPrimitive( ctx, GL_POINTS );

   for ( ; start < count ; start++)
      fx_draw_point_wide(fxMesa, fxVB + start);
}

static void fx_render_pw_elts ( GLcontext *ctx,
				GLuint start,
				GLuint count,
				GLuint flags )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GrVertex *fxVB = fxMesa->verts;
   const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;
   (void) flags;

   fxRenderPrimitive( ctx, GL_POINTS );

   for ( ; start < count ; start++)
      fx_draw_point_wide(fxMesa, fxVB + elt[start]);
}

static void fx_draw_point_wide_aa ( fxMesaContext fxMesa,
			            GrVertex *v0 )
{
 GLint i, n;
 GLfloat ang, radius, oon;
 GrVertex vtxB, vtxC;

 const GLcontext *ctx = fxMesa->glCtx;
 const GLfloat psize = (ctx->_TriangleCaps & DD_POINT_ATTEN)
                       ? CLAMP(v0->psize, ctx->Point.MinSize, ctx->Point.MaxSize)
                       : ctx->Point._Size; /* clamped */

 if (ctx->Point.PointSprite) {
    fx_draw_point_sprite(fxMesa, v0, psize);
    return;
 }

 radius = psize / 2.0F;
 n = IROUND(psize * 2); /* radius x 4 */
 if (n < 4) n = 4;
 oon = 1.0F / (GLfloat)n;

 /* CLIP_LOOP ?!? */
 /* point coverage? */
 /* we don't care about culling here (see fxSetupCull) */

 vtxB = *v0;
 vtxC = *v0;

 vtxB.x += radius;
 for (i = 1; i <= n; i++) {
     ang = M_2PI * i * oon;
     vtxC.x = v0->x + radius * __GL_COSF(ang);
     vtxC.y = v0->y + radius * __GL_SINF(ang);
     grAADrawTriangle( v0, &vtxB, &vtxC, FXFALSE, FXTRUE, FXFALSE);
     /*grDrawTriangle( v0, &vtxB, &vtxC);*/
     vtxB.x = vtxC.x;
     vtxB.y = vtxC.y;
 }
}
#undef __GLCOSF
#undef __GLSINF
#undef M_2PI

#undef DO_FALLBACK


#define FX_UNFILLED_BIT    0x1
#define FX_OFFSET_BIT	   0x2
#define FX_TWOSIDE_BIT     0x4
#define FX_FLAT_BIT        0x8
#define FX_TWOSTENCIL_BIT  0x10
#define FX_FALLBACK_BIT    0x20
#define FX_MAX_TRIFUNC     0x40

static struct {
   tnl_points_func	points;
   tnl_line_func	line;
   tnl_triangle_func	triangle;
   tnl_quad_func	quad;
} rast_tab[FX_MAX_TRIFUNC];

#define DO_FALLBACK (IND & FX_FALLBACK_BIT)
#define DO_OFFSET   (IND & FX_OFFSET_BIT)
#define DO_UNFILLED (IND & FX_UNFILLED_BIT)
#define DO_TWOSIDE  (IND & FX_TWOSIDE_BIT)
#define DO_FLAT     (IND & FX_FLAT_BIT)
#define DO_TWOSTENCIL (IND & FX_TWOSTENCIL_BIT)
#define DO_TRI       1
#define DO_QUAD      1
#define DO_LINE      1
#define DO_POINTS    1
#define DO_FULL_QUAD 1

#define HAVE_RGBA   1
#define HAVE_SPEC   1
#define HAVE_HW_FLATSHADE 0
#define HAVE_BACK_COLORS  0
#define VERTEX GrVertex
#define TAB rast_tab

#define DEPTH_SCALE 1.0
#define UNFILLED_TRI unfilled_tri
#define UNFILLED_QUAD unfilled_quad
#define VERT_X(_v) _v->x
#define VERT_Y(_v) _v->y
#define VERT_Z(_v) _v->ooz
#define AREA_IS_CCW( a ) IS_NEGATIVE( a )
#define GET_VERTEX(e) (fxMesa->verts + e)


#if FX_PACKEDCOLOR
#define VERT_SET_RGBA( dst, f )			\
do {						\
   UNCLAMPED_FLOAT_TO_UBYTE(dst->pargb[2], f[0]);\
   UNCLAMPED_FLOAT_TO_UBYTE(dst->pargb[1], f[1]);\
   UNCLAMPED_FLOAT_TO_UBYTE(dst->pargb[0], f[2]);\
   UNCLAMPED_FLOAT_TO_UBYTE(dst->pargb[3], f[3]);\
} while (0)

#define VERT_COPY_RGBA( v0, v1 ) 		\
   *(GLuint *)&v0->pargb = *(GLuint *)&v1->pargb

#define VERT_SAVE_RGBA( idx )  			\
   *(GLuint *)&color[idx] = *(GLuint *)&v[idx]->pargb

#define VERT_RESTORE_RGBA( idx )		\
   *(GLuint *)&v[idx]->pargb = *(GLuint *)&color[idx]


#define VERT_SET_SPEC( dst, f )			\
do {						\
   UNCLAMPED_FLOAT_TO_UBYTE(dst->pspec[2], f[0]);\
   UNCLAMPED_FLOAT_TO_UBYTE(dst->pspec[1], f[1]);\
   UNCLAMPED_FLOAT_TO_UBYTE(dst->pspec[0], f[2]);\
} while (0)

#define VERT_COPY_SPEC( v0, v1 ) 		\
   *(GLuint *)&v0->pspec = *(GLuint *)&v1->pspec

#define VERT_SAVE_SPEC( idx )  			\
   *(GLuint *)&spec[idx] = *(GLuint *)&v[idx]->pspec

#define VERT_RESTORE_SPEC( idx )		\
   *(GLuint *)&v[idx]->pspec = *(GLuint *)&spec[idx]


#define LOCAL_VARS(n)				\
   fxMesaContext fxMesa = FX_CONTEXT(ctx);	\
   GLubyte color[n][4], spec[n][4];		\
   (void) color; (void) spec;
#else  /* !FX_PACKEDCOLOR */
#define VERT_SET_RGBA( dst, f )	\
do {				\
   CNORM(dst->r, f[0]);		\
   CNORM(dst->g, f[1]);		\
   CNORM(dst->b, f[2]);		\
   CNORM(dst->a, f[3]);		\
} while (0)

#define VERT_COPY_RGBA( v0, v1 ) 		\
do {						\
   COPY_FLOAT(v0->r, v1->r);			\
   COPY_FLOAT(v0->g, v1->g);			\
   COPY_FLOAT(v0->b, v1->b);			\
   COPY_FLOAT(v0->a, v1->a);			\
} while (0)

#define VERT_SAVE_RGBA( idx )  			\
do {						\
   COPY_FLOAT(color[idx][0], v[idx]->r);	\
   COPY_FLOAT(color[idx][1], v[idx]->g);	\
   COPY_FLOAT(color[idx][2], v[idx]->b);	\
   COPY_FLOAT(color[idx][3], v[idx]->a);	\
} while (0)

#define VERT_RESTORE_RGBA( idx )		\
do {						\
   COPY_FLOAT(v[idx]->r, color[idx][0]);	\
   COPY_FLOAT(v[idx]->g, color[idx][1]);	\
   COPY_FLOAT(v[idx]->b, color[idx][2]);	\
   COPY_FLOAT(v[idx]->a, color[idx][3]);	\
} while (0)


#define VERT_SET_SPEC( dst, f )	\
do {				\
   CNORM(dst->r1, f[0]);	\
   CNORM(dst->g1, f[1]);	\
   CNORM(dst->b1, f[2]);	\
} while (0)

#define VERT_COPY_SPEC( v0, v1 ) 		\
do {						\
   COPY_FLOAT(v0->r1, v1->r1);			\
   COPY_FLOAT(v0->g1, v1->g1);			\
   COPY_FLOAT(v0->b1, v1->b1);			\
} while (0)

#define VERT_SAVE_SPEC( idx )  			\
do {						\
   COPY_FLOAT(spec[idx][0], v[idx]->r1);	\
   COPY_FLOAT(spec[idx][1], v[idx]->g1);	\
   COPY_FLOAT(spec[idx][2], v[idx]->b1);	\
} while (0)

#define VERT_RESTORE_SPEC( idx )		\
do {						\
   COPY_FLOAT(v[idx]->r1, spec[idx][0]);	\
   COPY_FLOAT(v[idx]->g1, spec[idx][1]);	\
   COPY_FLOAT(v[idx]->b1, spec[idx][2]);	\
} while (0)


#define LOCAL_VARS(n)				\
   fxMesaContext fxMesa = FX_CONTEXT(ctx);	\
   GLfloat color[n][4], spec[n][4];		\
   (void) color; (void) spec;
#endif /* !FX_PACKEDCOLOR */


/***********************************************************************
 *            Twoside stencil                                          *
 ***********************************************************************/
#define SETUP_STENCIL(f) if (f) fxSetupStencilFace(ctx, f)
#define UNSET_STENCIL(f) if (f) fxSetupStencil(ctx)


/***********************************************************************
 *            Functions to draw basic unfilled primitives              *
 ***********************************************************************/

#define RASTERIZE(x) if (fxMesa->raster_primitive != reduced_prim[x]) \
                        fxRasterPrimitive( ctx, reduced_prim[x] )
#define RENDER_PRIMITIVE fxMesa->render_primitive
#define IND FX_FALLBACK_BIT
#define TAG(x) x
#include "tnl_dd/t_dd_unfilled.h"
#undef IND

/***********************************************************************
 *                 Functions to draw GL primitives                     *
 ***********************************************************************/

#define IND (0)
#define TAG(x) x
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_OFFSET_BIT)
#define TAG(x) x##_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_OFFSET_BIT)
#define TAG(x) x##_twoside_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_OFFSET_BIT|FX_UNFILLED_BIT)
#define TAG(x) x##_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_OFFSET_BIT|FX_UNFILLED_BIT)
#define TAG(x) x##_twoside_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_FALLBACK_BIT)
#define TAG(x) x##_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_OFFSET_BIT|FX_FALLBACK_BIT)
#define TAG(x) x##_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_FALLBACK_BIT)
#define TAG(x) x##_twoside_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_OFFSET_BIT|FX_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_UNFILLED_BIT|FX_FALLBACK_BIT)
#define TAG(x) x##_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_OFFSET_BIT|FX_UNFILLED_BIT|FX_FALLBACK_BIT)
#define TAG(x) x##_offset_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_UNFILLED_BIT|FX_FALLBACK_BIT)
#define TAG(x) x##_twoside_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_OFFSET_BIT|FX_UNFILLED_BIT| \
	     FX_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"


/* Fx doesn't support provoking-vertex flat-shading?
 */
#define IND (FX_FLAT_BIT)
#define TAG(x) x##_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_OFFSET_BIT|FX_FLAT_BIT)
#define TAG(x) x##_offset_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_FLAT_BIT)
#define TAG(x) x##_twoside_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_OFFSET_BIT|FX_FLAT_BIT)
#define TAG(x) x##_twoside_offset_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_UNFILLED_BIT|FX_FLAT_BIT)
#define TAG(x) x##_unfilled_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_OFFSET_BIT|FX_UNFILLED_BIT|FX_FLAT_BIT)
#define TAG(x) x##_offset_unfilled_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_UNFILLED_BIT|FX_FLAT_BIT)
#define TAG(x) x##_twoside_unfilled_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_OFFSET_BIT|FX_UNFILLED_BIT|FX_FLAT_BIT)
#define TAG(x) x##_twoside_offset_unfilled_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_FALLBACK_BIT|FX_FLAT_BIT)
#define TAG(x) x##_fallback_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_OFFSET_BIT|FX_FALLBACK_BIT|FX_FLAT_BIT)
#define TAG(x) x##_offset_fallback_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_FALLBACK_BIT|FX_FLAT_BIT)
#define TAG(x) x##_twoside_fallback_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_OFFSET_BIT|FX_FALLBACK_BIT|FX_FLAT_BIT)
#define TAG(x) x##_twoside_offset_fallback_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_UNFILLED_BIT|FX_FALLBACK_BIT|FX_FLAT_BIT)
#define TAG(x) x##_unfilled_fallback_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_OFFSET_BIT|FX_UNFILLED_BIT|FX_FALLBACK_BIT|FX_FLAT_BIT)
#define TAG(x) x##_offset_unfilled_fallback_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_UNFILLED_BIT|FX_FALLBACK_BIT|FX_FLAT_BIT)
#define TAG(x) x##_twoside_unfilled_fallback_flat
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_OFFSET_BIT|FX_UNFILLED_BIT| \
	     FX_FALLBACK_BIT|FX_FLAT_BIT)
#define TAG(x) x##_twoside_offset_unfilled_fallback_flat
#include "tnl_dd/t_dd_tritmp.h"


/* 2-sided stencil begin */
#define IND (FX_TWOSTENCIL_BIT)
#define TAG(x) x##_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_OFFSET_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_offset_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_twoside_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_OFFSET_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_twoside_offset_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_UNFILLED_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_unfilled_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_OFFSET_BIT|FX_UNFILLED_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_offset_unfilled_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_UNFILLED_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_twoside_unfilled_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_OFFSET_BIT|FX_UNFILLED_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_twoside_offset_unfilled_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_FALLBACK_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_fallback_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_OFFSET_BIT|FX_FALLBACK_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_offset_fallback_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_FALLBACK_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_twoside_fallback_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_OFFSET_BIT|FX_FALLBACK_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_twoside_offset_fallback_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_UNFILLED_BIT|FX_FALLBACK_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_unfilled_fallback_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_OFFSET_BIT|FX_UNFILLED_BIT|FX_FALLBACK_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_offset_unfilled_fallback_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_UNFILLED_BIT|FX_FALLBACK_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_twoside_unfilled_fallback_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_OFFSET_BIT|FX_UNFILLED_BIT| \
	     FX_FALLBACK_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_twoside_offset_unfilled_fallback_twostencil
#include "tnl_dd/t_dd_tritmp.h"


/* Fx doesn't support provoking-vertex flat-shading?
 */
#define IND (FX_FLAT_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_flat_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_OFFSET_BIT|FX_FLAT_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_offset_flat_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_FLAT_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_twoside_flat_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_OFFSET_BIT|FX_FLAT_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_twoside_offset_flat_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_UNFILLED_BIT|FX_FLAT_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_unfilled_flat_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_OFFSET_BIT|FX_UNFILLED_BIT|FX_FLAT_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_offset_unfilled_flat_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_UNFILLED_BIT|FX_FLAT_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_twoside_unfilled_flat_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_OFFSET_BIT|FX_UNFILLED_BIT|FX_FLAT_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_twoside_offset_unfilled_flat_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_FALLBACK_BIT|FX_FLAT_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_fallback_flat_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_OFFSET_BIT|FX_FALLBACK_BIT|FX_FLAT_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_offset_fallback_flat_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_FALLBACK_BIT|FX_FLAT_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_twoside_fallback_flat_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_OFFSET_BIT|FX_FALLBACK_BIT|FX_FLAT_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_twoside_offset_fallback_flat_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_UNFILLED_BIT|FX_FALLBACK_BIT|FX_FLAT_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_unfilled_fallback_flat_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_OFFSET_BIT|FX_UNFILLED_BIT|FX_FALLBACK_BIT|FX_FLAT_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_offset_unfilled_fallback_flat_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_UNFILLED_BIT|FX_FALLBACK_BIT|FX_FLAT_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_twoside_unfilled_fallback_flat_twostencil
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FX_TWOSIDE_BIT|FX_OFFSET_BIT|FX_UNFILLED_BIT| \
	     FX_FALLBACK_BIT|FX_FLAT_BIT|FX_TWOSTENCIL_BIT)
#define TAG(x) x##_twoside_offset_unfilled_fallback_flat_twostencil
#include "tnl_dd/t_dd_tritmp.h"
/* 2-sided stencil end */


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

   /* 2-sided stencil begin */
   init_twostencil();
   init_offset_twostencil();
   init_twoside_twostencil();
   init_twoside_offset_twostencil();
   init_unfilled_twostencil();
   init_offset_unfilled_twostencil();
   init_twoside_unfilled_twostencil();
   init_twoside_offset_unfilled_twostencil();
   init_fallback_twostencil();
   init_offset_fallback_twostencil();
   init_twoside_fallback_twostencil();
   init_twoside_offset_fallback_twostencil();
   init_unfilled_fallback_twostencil();
   init_offset_unfilled_fallback_twostencil();
   init_twoside_unfilled_fallback_twostencil();
   init_twoside_offset_unfilled_fallback_twostencil();

   init_flat_twostencil();
   init_offset_flat_twostencil();
   init_twoside_flat_twostencil();
   init_twoside_offset_flat_twostencil();
   init_unfilled_flat_twostencil();
   init_offset_unfilled_flat_twostencil();
   init_twoside_unfilled_flat_twostencil();
   init_twoside_offset_unfilled_flat_twostencil();
   init_fallback_flat_twostencil();
   init_offset_fallback_flat_twostencil();
   init_twoside_fallback_flat_twostencil();
   init_twoside_offset_fallback_flat_twostencil();
   init_unfilled_fallback_flat_twostencil();
   init_offset_unfilled_fallback_flat_twostencil();
   init_twoside_unfilled_fallback_flat_twostencil();
   init_twoside_offset_unfilled_fallback_flat_twostencil();
   /* 2-sided stencil end */
}


/**********************************************************************/
/*                 Render whole begin/end objects                     */
/**********************************************************************/


/* Accelerate vertex buffer rendering when renderindex == 0 and
 * there is no clipping.
 */
#define INIT(x) fxRenderPrimitive( ctx, x )

static void fx_render_vb_points( GLcontext *ctx,
				 GLuint start,
				 GLuint count,
				 GLuint flags )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GrVertex *fxVB = fxMesa->verts;
   GLint i;
   (void) flags;

   if (TDFX_DEBUG & VERBOSE_VARRAY) {
      fprintf(stderr, "fx_render_vb_points\n");
   }

   INIT(GL_POINTS);

   /* Adjust point coords */
   for (i = start; i < count; i++) {
      fxVB[i].x += PNT_X_OFFSET - TRI_X_OFFSET;
      fxVB[i].y += PNT_Y_OFFSET - TRI_Y_OFFSET;
   }

   grDrawVertexArrayContiguous( GR_POINTS, count-start,
                                fxVB + start, sizeof(GrVertex));
   /* restore point coords */
   for (i = start; i < count; i++) {
      fxVB[i].x -= PNT_X_OFFSET - TRI_X_OFFSET;
      fxVB[i].y -= PNT_Y_OFFSET - TRI_Y_OFFSET;
   }
}

static void fx_render_vb_line_strip( GLcontext *ctx,
				     GLuint start,
				     GLuint count,
				     GLuint flags )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GrVertex *fxVB = fxMesa->verts;
   GLint i;
   (void) flags;

   if (TDFX_DEBUG & VERBOSE_VARRAY) {
      fprintf(stderr, "fx_render_vb_line_strip\n");
   }

   INIT(GL_LINE_STRIP);

   /* adjust line coords */
   for (i = start; i < count; i++) {
      fxVB[i].x += LINE_X_OFFSET - TRI_X_OFFSET;
      fxVB[i].y += LINE_Y_OFFSET - TRI_Y_OFFSET;
   }

   grDrawVertexArrayContiguous( GR_LINE_STRIP, count-start,
                                fxVB + start, sizeof(GrVertex));

   /* restore line coords */
   for (i = start; i < count; i++) {
      fxVB[i].x -= LINE_X_OFFSET - TRI_X_OFFSET;
      fxVB[i].y -= LINE_Y_OFFSET - TRI_Y_OFFSET;
   }
}

static void fx_render_vb_line_loop( GLcontext *ctx,
				    GLuint start,
				    GLuint count,
				    GLuint flags )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GrVertex *fxVB = fxMesa->verts;
   GLint i;
   GLint j = start;
   (void) flags;

   if (TDFX_DEBUG & VERBOSE_VARRAY) {
      fprintf(stderr, "fx_render_vb_line_loop\n");
   }

   INIT(GL_LINE_LOOP);

   if (!(flags & PRIM_BEGIN)) {
      j++;
   }

   /* adjust line coords */
   for (i = start; i < count; i++) {
      fxVB[i].x += LINE_X_OFFSET - TRI_X_OFFSET;
      fxVB[i].y += LINE_Y_OFFSET - TRI_Y_OFFSET;
   }

   grDrawVertexArrayContiguous( GR_LINE_STRIP, count-j,
                                fxVB + j, sizeof(GrVertex));

   if (flags & PRIM_END)
      grDrawLine( fxVB + (count - 1),
                  fxVB + start );

   /* restore line coords */
   for (i = start; i < count; i++) {
      fxVB[i].x -= LINE_X_OFFSET - TRI_X_OFFSET;
      fxVB[i].y -= LINE_Y_OFFSET - TRI_Y_OFFSET;
   }
}

static void fx_render_vb_lines( GLcontext *ctx,
				GLuint start,
				GLuint count,
				GLuint flags )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GrVertex *fxVB = fxMesa->verts;
   GLint i;
   (void) flags;

   if (TDFX_DEBUG & VERBOSE_VARRAY) {
      fprintf(stderr, "fx_render_vb_lines\n");
   }

   INIT(GL_LINES);

   /* adjust line coords */
   for (i = start; i < count; i++) {
      fxVB[i].x += LINE_X_OFFSET - TRI_X_OFFSET;
      fxVB[i].y += LINE_Y_OFFSET - TRI_Y_OFFSET;
   }

   grDrawVertexArrayContiguous( GR_LINES, count-start,
                                fxVB + start, sizeof(GrVertex));

   /* restore line coords */
   for (i = start; i < count; i++) {
      fxVB[i].x -= LINE_X_OFFSET - TRI_X_OFFSET;
      fxVB[i].y -= LINE_Y_OFFSET - TRI_Y_OFFSET;
   }
}

static void fx_render_vb_triangles( GLcontext *ctx,
				    GLuint start,
				    GLuint count,
				    GLuint flags )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GrVertex *fxVB = fxMesa->verts;
   GLuint j;
   (void) flags;

   if (TDFX_DEBUG & VERBOSE_VARRAY) {
      fprintf(stderr, "fx_render_vb_triangles\n");
   }

   INIT(GL_TRIANGLES);

   for (j=start+2; j<count; j+=3) {
      grDrawTriangle(fxVB + (j-2), fxVB + (j-1), fxVB + j);
   }
}


static void fx_render_vb_tri_strip( GLcontext *ctx,
				    GLuint start,
				    GLuint count,
				    GLuint flags )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GrVertex *fxVB = fxMesa->verts;
   (void) flags;

   if (TDFX_DEBUG & VERBOSE_VARRAY) {
      fprintf(stderr, "fx_render_vb_tri_strip\n");
   }

   INIT(GL_TRIANGLE_STRIP);

   /* no GR_TRIANGLE_STRIP_CONTINUE?!? */

   grDrawVertexArrayContiguous( GR_TRIANGLE_STRIP, count-start,
                                fxVB + start, sizeof(GrVertex));
}


static void fx_render_vb_tri_fan( GLcontext *ctx,
				  GLuint start,
				  GLuint count,
				  GLuint flags )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GrVertex *fxVB = fxMesa->verts;
   (void) flags;

   if (TDFX_DEBUG & VERBOSE_VARRAY) {
      fprintf(stderr, "fx_render_vb_tri_fan\n");
   }

   INIT(GL_TRIANGLE_FAN);

   grDrawVertexArrayContiguous( GR_TRIANGLE_FAN, count-start,
                                fxVB + start, sizeof(GrVertex) );
}

static void fx_render_vb_quads( GLcontext *ctx,
				GLuint start,
				GLuint count,
				GLuint flags )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GrVertex *fxVB = fxMesa->verts;
   GLuint i;
   (void) flags;

   if (TDFX_DEBUG & VERBOSE_VARRAY) {
      fprintf(stderr, "fx_render_vb_quads\n");
   }

   INIT(GL_QUADS);

   for (i = start + 3 ; i < count ; i += 4 ) {
#define VERT(x) (fxVB + (x))
      GrVertex *_v_[4];
      _v_[0] = VERT(i);
      _v_[1] = VERT(i-3);
      _v_[2] = VERT(i-2);
      _v_[3] = VERT(i-1);
      grDrawVertexArray(GR_TRIANGLE_FAN, 4, _v_);
      /*grDrawTriangle( VERT(i-3), VERT(i-2), VERT(i) );*/
      /*grDrawTriangle( VERT(i-2), VERT(i-1), VERT(i) );*/
#undef VERT
   }
}

static void fx_render_vb_quad_strip( GLcontext *ctx,
				     GLuint start,
				     GLuint count,
				     GLuint flags )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GrVertex *fxVB = fxMesa->verts;
   (void) flags;

   if (TDFX_DEBUG & VERBOSE_VARRAY) {
      fprintf(stderr, "fx_render_vb_quad_strip\n");
   }

   INIT(GL_QUAD_STRIP);

   count -= (count-start)&1;

   grDrawVertexArrayContiguous( GR_TRIANGLE_STRIP,
                                count-start, fxVB + start, sizeof(GrVertex));
}

static void fx_render_vb_poly( GLcontext *ctx,
				GLuint start,
				GLuint count,
				GLuint flags )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GrVertex *fxVB = fxMesa->verts;
   (void) flags;

   if (TDFX_DEBUG & VERBOSE_VARRAY) {
      fprintf(stderr, "fx_render_vb_poly\n");
   }

   INIT(GL_POLYGON);

   grDrawVertexArrayContiguous( GR_POLYGON, count-start,
                                fxVB + start, sizeof(GrVertex));
}

static void fx_render_vb_noop( GLcontext *ctx,
				 GLuint start,
				 GLuint count,
				 GLuint flags )
{
   (void) (ctx && start && count && flags);
}

static void (*fx_render_tab_verts[GL_POLYGON+2])(GLcontext *,
						   GLuint,
						   GLuint,
						   GLuint) =
{
   fx_render_vb_points,
   fx_render_vb_lines,
   fx_render_vb_line_loop,
   fx_render_vb_line_strip,
   fx_render_vb_triangles,
   fx_render_vb_tri_strip,
   fx_render_vb_tri_fan,
   fx_render_vb_quads,
   fx_render_vb_quad_strip,
   fx_render_vb_poly,
   fx_render_vb_noop,
};
#undef INIT


/**********************************************************************/
/*            Render whole (indexed) begin/end objects                */
/**********************************************************************/


#define VERT(x) (vertptr + x)

#define RENDER_POINTS( start, count )		\
   for ( ; start < count ; start++)		\
      grDrawPoint( VERT(ELT(start)) );

#define RENDER_LINE( v0, v1 ) \
   grDrawLine( VERT(v0), VERT(v1) )

#define RENDER_TRI( v0, v1, v2 )  \
   grDrawTriangle( VERT(v0), VERT(v1), VERT(v2) )

#define RENDER_QUAD( v0, v1, v2, v3 ) \
   do {	\
      GrVertex *_v_[4];	\
      _v_[0] = VERT(v3);\
      _v_[1] = VERT(v0);\
      _v_[2] = VERT(v1);\
      _v_[3] = VERT(v2);\
      grDrawVertexArray(GR_TRIANGLE_FAN, 4, _v_);\
      /*grDrawTriangle( VERT(v0), VERT(v1), VERT(v3) );*/\
      /*grDrawTriangle( VERT(v1), VERT(v2), VERT(v3) );*/\
   } while (0)

#define INIT(x) fxRenderPrimitive( ctx, x )

#undef LOCAL_VARS
#define LOCAL_VARS						\
    fxMesaContext fxMesa = FX_CONTEXT(ctx);			\
    GrVertex *vertptr = fxMesa->verts;		\
    const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;	\
    (void) elt;

#define RESET_STIPPLE
#define RESET_OCCLUSION
#define PRESERVE_VB_DEFS

/* Elts, no clipping.
 */
#undef ELT
#undef TAG
#define TAG(x) fx_##x##_elts
#define ELT(x) elt[x]
#include "tnl_dd/t_dd_rendertmp.h"

/* Verts, no clipping.
 */
#undef ELT
#undef TAG
#define TAG(x) fx_##x##_verts
#define ELT(x) x
/*#include "tnl_dd/t_dd_rendertmp.h"*/ /* we have fx_render_vb_* now */



/**********************************************************************/
/*                   Render clipped primitives                        */
/**********************************************************************/



static void fxRenderClippedPoly( GLcontext *ctx, const GLuint *elts,
				   GLuint n )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint prim = fxMesa->render_primitive;

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


static void fxFastRenderClippedPoly( GLcontext *ctx, const GLuint *elts,
				       GLuint n )
{
   int i;
   fxMesaContext fxMesa = FX_CONTEXT( ctx );
   GrVertex *vertptr = fxMesa->verts;
   if (n == 3) {
      grDrawTriangle( VERT(elts[0]), VERT(elts[1]), VERT(elts[2]) );
   } else if (n <= 32) {
      GrVertex *newvptr[32];
      for (i = 0 ; i < n ; i++) {
         newvptr[i] = VERT(elts[i]);
      }
      grDrawVertexArray(GR_TRIANGLE_FAN, n, newvptr);
   } else {
      const GrVertex *start = VERT(elts[0]);
      for (i = 2 ; i < n ; i++) {
         grDrawTriangle( start, VERT(elts[i-1]), VERT(elts[i]) );
      }
   }
}

/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/


#define POINT_FALLBACK (DD_POINT_SMOOTH)
#define LINE_FALLBACK (DD_LINE_STIPPLE)
#define TRI_FALLBACK (DD_TRI_SMOOTH | DD_TRI_STIPPLE)
#define ANY_FALLBACK_FLAGS (POINT_FALLBACK | LINE_FALLBACK | TRI_FALLBACK)
#define ANY_RASTER_FLAGS (DD_FLATSHADE | DD_TRI_LIGHT_TWOSIDE | DD_TRI_OFFSET \
			  | DD_TRI_UNFILLED | DD_TRI_TWOSTENCIL)



void fxDDChooseRenderState(GLcontext *ctx)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLuint flags = ctx->_TriangleCaps;
   GLuint index = 0;

   if (flags & (ANY_FALLBACK_FLAGS|ANY_RASTER_FLAGS)) {
      if (flags & ANY_RASTER_FLAGS) {
	 if (flags & DD_TRI_TWOSTENCIL)       index |= FX_TWOSTENCIL_BIT;
	 if (flags & DD_TRI_LIGHT_TWOSIDE)    index |= FX_TWOSIDE_BIT;
	 if (flags & DD_TRI_OFFSET)	      index |= FX_OFFSET_BIT;
	 if (flags & DD_TRI_UNFILLED)	      index |= FX_UNFILLED_BIT;
	 if (flags & DD_FLATSHADE)	      index |= FX_FLAT_BIT;
      }

      fxMesa->draw_point = fx_draw_point;
      fxMesa->draw_line = fx_draw_line;
      fxMesa->draw_tri = fx_draw_triangle;

      /* Hook in fallbacks for specific primitives. */
      if (flags & (POINT_FALLBACK|
		   LINE_FALLBACK|
		   TRI_FALLBACK))
      {
         if (fxMesa->verbose) {
            fprintf(stderr, "Voodoo ! fallback (%x), raster (%x)\n",
                            flags & ANY_FALLBACK_FLAGS, flags & ANY_RASTER_FLAGS);
         }

	 if (flags & POINT_FALLBACK)
	    fxMesa->draw_point = fx_fallback_point;

	 if (flags & LINE_FALLBACK)
	    fxMesa->draw_line = fx_fallback_line;

	 if (flags & TRI_FALLBACK)
	    fxMesa->draw_tri = fx_fallback_tri;

	 index |= FX_FALLBACK_BIT;
      }
   }

   tnl->Driver.Render.Points = rast_tab[index].points;
   tnl->Driver.Render.Line = rast_tab[index].line;
   tnl->Driver.Render.ClippedLine = rast_tab[index].line;
   tnl->Driver.Render.Triangle = rast_tab[index].triangle;
   tnl->Driver.Render.Quad = rast_tab[index].quad;

   if (index == 0) {
      tnl->Driver.Render.PrimTabVerts = fx_render_tab_verts;
      tnl->Driver.Render.PrimTabElts = fx_render_tab_elts;
      tnl->Driver.Render.ClippedPolygon = fxFastRenderClippedPoly;
   } else {
      tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
      tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
      tnl->Driver.Render.ClippedPolygon = fxRenderClippedPoly;
   }

   fxMesa->render_index = index;

   /* [dBorca] Hack alert: more a trick than a real plug-in!!! */
   if (flags & (DD_POINT_SIZE | DD_POINT_ATTEN)) {
      /* We need to set the point primitive to go through "rast_tab",
       * to make sure "POINT" calls "fxMesa->draw_point" instead of
       * "grDrawPoint". We can achieve this by using FX_FALLBACK_BIT
       * (not really a total rasterization fallback, so we don't alter
       * "fxMesa->render_index"). If we get here with DD_POINT_SMOOTH,
       * we're done, cos we've already set _tnl_render_tab_{verts|elts}
       * above. Otherwise, the T&L engine can optimize point rendering
       * by using fx_render_tab_{verts|elts} hence the extra work.
       */
      if (flags & DD_POINT_SMOOTH) {
         fxMesa->draw_point = fx_draw_point_wide_aa;
      } else {
         fxMesa->draw_point = fx_draw_point_wide;
         fx_render_tab_verts[0] = fx_render_pw_verts;
         fx_render_tab_elts[0] = fx_render_pw_elts;
      }
      tnl->Driver.Render.Points = rast_tab[index|FX_FALLBACK_BIT].points;
   } else {
      fx_render_tab_verts[0] = fx_render_vb_points;
      fx_render_tab_elts[0] = fx_render_points_elts;
   }
}


/**********************************************************************/
/*                Runtime render state and callbacks                  */
/**********************************************************************/

static void fxRunPipeline( GLcontext *ctx )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLuint new_gl_state = fxMesa->new_gl_state;

   if (TDFX_DEBUG & VERBOSE_PIPELINE) {
      fprintf(stderr, "fxRunPipeline()\n");
   }

#if 0
   /* Recalculate fog table on projection matrix changes.  This used to
    * be triggered by the NearFar callback.
    */
   if (new_gl_state & _NEW_PROJECTION)
      fxMesa->new_state |= FX_NEW_FOG;
#endif

   if (new_gl_state & _FX_NEW_IS_IN_HARDWARE)
      fxCheckIsInHardware(ctx);

   if (fxMesa->new_state)
      fxSetupFXUnits(ctx);

   if (!fxMesa->fallback) {
      if (new_gl_state & _FX_NEW_RENDERSTATE)
         fxDDChooseRenderState(ctx);

      if (new_gl_state & _FX_NEW_SETUP_FUNCTION)
         fxChooseVertexState(ctx);
   }

   if (new_gl_state & _NEW_TEXTURE) {
      struct gl_texture_unit *t0 = &ctx->Texture.Unit[fxMesa->tmu_source[0]];
      struct gl_texture_unit *t1 = &ctx->Texture.Unit[fxMesa->tmu_source[1]];

      if (t0->_Current && FX_TEXTURE_DATA(t0)) {
         fxMesa->s0scale = FX_TEXTURE_DATA(t0)->sScale;
         fxMesa->t0scale = FX_TEXTURE_DATA(t0)->tScale;
         fxMesa->inv_s0scale = 1.0F / fxMesa->s0scale;
         fxMesa->inv_t0scale = 1.0F / fxMesa->t0scale;
      }

      if (t1->_Current && FX_TEXTURE_DATA(t1)) {
         fxMesa->s1scale = FX_TEXTURE_DATA(t1)->sScale;
         fxMesa->t1scale = FX_TEXTURE_DATA(t1)->tScale;
         fxMesa->inv_s1scale = 1.0F / fxMesa->s1scale;
         fxMesa->inv_t1scale = 1.0F / fxMesa->t1scale;
      }
   }

   fxMesa->new_gl_state = 0;

   _tnl_run_pipeline( ctx );
}



/* Always called between RenderStart and RenderFinish --> We already
 * hold the lock.
 */
static void fxRasterPrimitive( GLcontext *ctx, GLenum prim )
{
   fxMesaContext fxMesa = FX_CONTEXT( ctx );

   fxMesa->raster_primitive = prim;

   fxSetupCull(ctx);
}



/* Determine the rasterized primitive when not drawing unfilled
 * polygons.
 */
static void fxRenderPrimitive( GLcontext *ctx, GLenum prim )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLuint rprim = reduced_prim[prim];

   fxMesa->render_primitive = prim;

   if (rprim == GL_TRIANGLES && (ctx->_TriangleCaps & DD_TRI_UNFILLED))
      return;

   if (fxMesa->raster_primitive != rprim) {
      fxRasterPrimitive( ctx, rprim );
   }
}

static void fxRenderFinish( GLcontext *ctx )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);

   if (fxMesa->render_index & FX_FALLBACK_BIT)
      _swrast_flush( ctx );
}



/**********************************************************************/
/*               Manage total rasterization fallbacks                 */
/**********************************************************************/

static char *fallbackStrings[] = {
   "3D/Rect/Cube Texture map",
   "glDrawBuffer(GL_FRONT_AND_BACK)",
   "Separate specular color",
   "glEnable/Disable(GL_STENCIL_TEST)",
   "glRenderMode(selection or feedback)",
   "glLogicOp()",
   "Texture env mode",
   "Texture border",
   "glColorMask",
   "blend mode",
   "multitex"
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


void fxCheckIsInHardware( GLcontext *ctx )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint oldfallback = fxMesa->fallback;
   GLuint newfallback = fxMesa->fallback = fx_check_IsInHardware( ctx );

   if (newfallback) {
      if (oldfallback == 0) {
         if (fxMesa->verbose) {
            fprintf(stderr, "Voodoo ! enter SW 0x%08x %s\n", newfallback, getFallbackString(newfallback));
         }
	 _swsetup_Wakeup( ctx );
      }
   }
   else {
      if (oldfallback) {
	 _swrast_flush( ctx );
	 tnl->Driver.Render.Start = fxCheckTexSizes;
	 tnl->Driver.Render.Finish = fxRenderFinish;
	 tnl->Driver.Render.PrimitiveNotify = fxRenderPrimitive;
	 tnl->Driver.Render.ClippedPolygon = _tnl_RenderClippedPolygon;
	 tnl->Driver.Render.ClippedLine = _tnl_RenderClippedLine;
	 tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
	 tnl->Driver.Render.ResetLineStipple = _swrast_ResetLineStipple;
	 tnl->Driver.Render.BuildVertices = fxBuildVertices;
	 fxChooseVertexState(ctx);
	 fxDDChooseRenderState(ctx);
         if (fxMesa->verbose) {
            fprintf(stderr, "Voodoo ! leave SW 0x%08x %s\n", oldfallback, getFallbackString(oldfallback));
         }
      }
      tnl->Driver.Render.Multipass = NULL;
      if (HAVE_SPEC && NEED_SECONDARY_COLOR(ctx)) {
         tnl->Driver.Render.Multipass = fxMultipass_ColorSum;
         /* obey stencil, but do not change it */
         fxMesa->multipass = GL_TRUE;
         if (fxMesa->unitsState.stencilEnabled) {
            fxMesa->new_state |= FX_NEW_STENCIL;
         }
      }
   }
}

void fxDDInitTriFuncs( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   static int firsttime = 1;

   if (firsttime) {
      init_rast_tab();
      firsttime = 0;
   }

   tnl->Driver.RunPipeline = fxRunPipeline;
   tnl->Driver.Render.Start = fxCheckTexSizes;
   tnl->Driver.Render.Finish = fxRenderFinish;
   tnl->Driver.Render.PrimitiveNotify = fxRenderPrimitive;
   tnl->Driver.Render.ClippedPolygon = _tnl_RenderClippedPolygon;
   tnl->Driver.Render.ClippedLine = _tnl_RenderClippedLine;
   tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
   tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
   tnl->Driver.Render.ResetLineStipple = _swrast_ResetLineStipple;
   tnl->Driver.Render.BuildVertices = fxBuildVertices;
   tnl->Driver.Render.Multipass = NULL;

   (void) fx_print_vertex;
}


/* [dBorca] Hack alert:
 * doesn't work with blending.
 */
static GLboolean
fxMultipass_ColorSum (GLcontext *ctx, GLuint pass)
{
 fxMesaContext fxMesa = FX_CONTEXT(ctx);
 tfxUnitsState *us = &fxMesa->unitsState;

 static int t0 = 0;
 static int t1 = 0;

 switch (pass) {
        case 1: /* first pass: the TEXTURED triangles are drawn */
             /* set stencil's real values */
             fxMesa->multipass = GL_FALSE;
             if (us->stencilEnabled) {
                fxSetupStencil(ctx);
             }
             /* save per-pass data */
             fxMesa->restoreUnitsState = *us;
             /* turn off texturing */
             t0 = ctx->Texture.Unit[0]._ReallyEnabled;
             t1 = ctx->Texture.Unit[1]._ReallyEnabled;
             ctx->Texture.Unit[0]._ReallyEnabled = 0;
             ctx->Texture.Unit[1]._ReallyEnabled = 0;
             /* SUM the colors */
             fxDDBlendEquationSeparate(ctx, GL_FUNC_ADD, GL_FUNC_ADD);
             fxDDBlendFuncSeparate(ctx, GL_ONE, GL_ONE, GL_ZERO, GL_ONE);
             fxDDEnable(ctx, GL_BLEND, GL_TRUE);
             /* make sure we draw only where we want to */
             if (us->depthTestEnabled) {
                switch (us->depthTestFunc) {
                   default:
                      fxDDDepthFunc(ctx, GL_EQUAL);
                   case GL_NEVER:
                   case GL_ALWAYS:
                      ;
                }
                fxDDDepthMask(ctx, GL_FALSE);
             }
             /* switch to secondary colors */
#if FX_PACKEDCOLOR
             grVertexLayout(GR_PARAM_PARGB, GR_VERTEX_PSPEC_OFFSET << 2, GR_PARAM_ENABLE);
#else  /* !FX_PACKEDCOLOR */
             grVertexLayout(GR_PARAM_RGB, GR_VERTEX_SPEC_OFFSET << 2, GR_PARAM_ENABLE);
#endif /* !FX_PACKEDCOLOR */
             /* don't advertise new state */
             fxMesa->new_state = 0;
             break;
        case 2: /* 2nd pass (last): the secondary color is summed over texture */
             /* restore original state */
             *us = fxMesa->restoreUnitsState;
             /* restore texturing */
             ctx->Texture.Unit[0]._ReallyEnabled = t0;
             ctx->Texture.Unit[1]._ReallyEnabled = t1;
             /* revert to primary colors */
#if FX_PACKEDCOLOR
             grVertexLayout(GR_PARAM_PARGB, GR_VERTEX_PARGB_OFFSET << 2, GR_PARAM_ENABLE);
#else  /* !FX_PACKEDCOLOR */
             grVertexLayout(GR_PARAM_RGB, GR_VERTEX_RGB_OFFSET << 2, GR_PARAM_ENABLE);
#endif /* !FX_PACKEDCOLOR */
             break;
        default:
             assert(0); /* NOTREACHED */
 }

 /* update HW state */
 fxSetupBlend(ctx);
 fxSetupDepthTest(ctx);
 fxSetupTexture(ctx);

 return (pass == 1);
}


#else


/*
 * Need this to provide at least one external definition.
 */

extern int gl_fx_dummy_function_tris(void);
int
gl_fx_dummy_function_tris(void)
{
   return 0;
}

#endif /* FX */
