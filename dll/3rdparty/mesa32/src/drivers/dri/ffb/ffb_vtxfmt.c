/* $XFree86: xc/lib/GL/mesa/src/drv/ffb/ffb_vtxfmt.c,v 1.1 2002/02/22 21:32:59 dawes Exp $
 *
 * GLX Hardware Device Driver for Sun Creator/Creator3D
 * Copyright (C) 2001 David S. Miller
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
 * DAVID MILLER, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 *    David S. Miller <davem@redhat.com>
 */

#include "glheader.h"
#include "api_noop.h"
#include "context.h"
#include "light.h"
#include "macros.h"
#include "imports.h"
#include "mtypes.h"
#include "simple_list.h"
#include "vtxfmt.h"
#include "ffb_xmesa.h"
#include "ffb_context.h"
#include "ffb_vb.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"

#include "ffb_vtxfmt.h"

#ifndef __GNUC__
#define __inline  /**/
#endif

#define TNL_VERTEX			ffbTnlVertex

#define INTERP_RGBA(t, out, a, b)		\
do {						\
   GLint i;					\
   for ( i = 0 ; i < 4 ; i++ ) {		\
      GLfloat fa = a[i];			\
      GLfloat fb = b[i];			\
      out[i] = LINTERP( t, fa, fb );		\
   }						\
} while (0)

/* Color functions: */

static __inline void ffb_recalc_base_color(GLcontext *ctx)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	struct gl_light *light;

	COPY_3V(fmesa->vtx_state.light.base_color, ctx->Light._BaseColor[0]);
	foreach (light, &ctx->Light.EnabledList) {
		ACC_3V(fmesa->vtx_state.light.base_color,
		       light->_MatAmbient[0]);
	}

	fmesa->vtx_state.light.base_alpha = ctx->Light._BaseAlpha[0];
}

#define GET_CURRENT \
	GET_CURRENT_CONTEXT(ctx);	\
	ffbContextPtr fmesa = FFB_CONTEXT(ctx)

#define CURRENT_COLOR(COMP)		fmesa->vtx_state.current.color[COMP]
#define CURRENT_SPECULAR(COMP)		fmesa->vtx_state.current.specular[COMP]
#define COLOR_IS_FLOAT
#define RECALC_BASE_COLOR(ctx)		ffb_recalc_base_color(ctx)

#define TAG(x)	ffb_##x
#include "tnl_dd/t_dd_imm_capi.h"

/* Normal functions: */

struct ffb_norm_tab {
	void (*normal3f_multi)(GLfloat x, GLfloat y, GLfloat z);
	void (*normal3fv_multi)(const GLfloat *v);
	void (*normal3f_single)(GLfloat x, GLfloat y, GLfloat z);
	void (*normal3fv_single)(const GLfloat *v);
};

static struct ffb_norm_tab norm_tab[0x4];

#define HAVE_HW_LIGHTING 0
#define GET_CURRENT_VERTEX			\
	GET_CURRENT_CONTEXT(ctx);		\
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);	\
	ffbTnlVertexPtr v = fmesa->imm.v0

#define CURRENT_NORMAL		fmesa->vtx_state.current.normal
#define BASE_COLOR		fmesa->vtx_state.light.base_color
#define BASE_ALPHA		fmesa->vtx_state.light.base_alpha
#define VERT_COLOR( COMP )	v->color[COMP]
#define VERT_COLOR_IS_FLOAT

#define IND (0)
#define TAG(x)	ffb_##x
#define PRESERVE_NORMAL_DEFS
#include "tnl_dd/t_dd_imm_napi.h"

#define IND (NORM_RESCALE)
#define TAG(x) ffb_##x##_rescale
#define PRESERVE_NORMAL_DEFS
#include "tnl_dd/t_dd_imm_napi.h"

#define IND (NORM_NORMALIZE)
#define TAG(x) ffb_##x##_normalize
#include "tnl_dd/t_dd_imm_napi.h"

static void ffb_init_norm_funcs(void)
{
	ffb_init_norm();
	ffb_init_norm_rescale();
	ffb_init_norm_normalize();
}

static void choose_normals(void)
{
	GET_CURRENT_CONTEXT(ctx);
	GLuint index;

	if (ctx->Light.Enabled) {
		if (ctx->Transform.Normalize) {
			index = NORM_NORMALIZE;
		} else if (!ctx->Transform.RescaleNormals &&
			   ctx->_ModelViewInvScale != 1.0) {
			index = NORM_RESCALE;
		} else {
			index = 0;
		}

		if (ctx->Light.EnabledList.next == ctx->Light.EnabledList.prev) {
			SET_Normal3f(ctx->Exec, norm_tab[index].normal3f_single);
			SET_Normal3fv(ctx->Exec, norm_tab[index].normal3fv_single);
		} else {
			SET_Normal3f(ctx->Exec, norm_tab[index].normal3f_multi);
			SET_Normal3fv(ctx->Exec, norm_tab[index].normal3fv_multi);
		}
	} else {
		SET_Normal3f(ctx->Exec, _mesa_noop_Normal3f);
		SET_Normal3fv(ctx->Exec, _mesa_noop_Normal3fv);
	}
}

static void ffb_choose_Normal3f(GLfloat x, GLfloat y, GLfloat z)
{
	choose_normals();
	CALL_Normal3f(GET_DISPATCH(), (x, y, z));
}

static void ffb_choose_Normal3fv(const GLfloat *v)
{
	choose_normals();
	CALL_Normal3fv(GET_DISPATCH(), (v));
}

/* Vertex functions: */

#define GET_CURRENT_VERTEX			\
	GET_CURRENT_CONTEXT(ctx);		\
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);	\
	ffbTnlVertexPtr v = fmesa->imm.v0

#define CURRENT_VERTEX		v->obj
#define SAVE_VERTEX		fmesa->imm.save_vertex(ctx, v)

#define TAG(x)	ffb_##x
#include "tnl_dd/t_dd_imm_vapi.h"

struct ffb_vert_tab {
	void (*save_vertex)(GLcontext *ctx, ffbTnlVertexPtr v);
	void (*interpolate_vertex)(GLfloat t,
				   ffbTnlVertex *O,
				   const ffbTnlVertex *I,
				   const ffbTnlVertex *J);
};

static struct ffb_vert_tab vert_tab[0xf];

#define VTX_NORMAL	0x0
#define VTX_RGBA	0x1

#define LOCAL_VARS \
	ffbContextPtr fmesa = FFB_CONTEXT(ctx)

#define CURRENT_COLOR			fmesa->vtx_state.current.color
#define COLOR_IS_FLOAT
#define FLUSH_VERTEX			fmesa->imm.flush_vertex( ctx, v );

#define IND	(VTX_NORMAL)
#define TAG(x)	ffb_##x##_NORMAL
#define PRESERVE_VERTEX_DEFS
#include "tnl_dd/t_dd_imm_vertex.h"

#define IND	(VTX_RGBA)
#define TAG(x)	ffb_##x##_RGBA
#include "tnl_dd/t_dd_imm_vertex.h"

static void ffb_init_vert_funcs( void )
{
	ffb_init_vert_NORMAL();
	ffb_init_vert_RGBA();
}

#define LOCAL_VARS							\
	ffbContextPtr fmesa = FFB_CONTEXT(ctx)

#define GET_INTERP_FUNC							\
	ffb_interp_func interp = fmesa->imm.interp

#define FLUSH_VERTEX			fmesa->imm.flush_vertex
#define IMM_VERTEX( V )			fmesa->imm.V
#define IMM_VERTICES( n )		fmesa->imm.vertices[n]

#define EMIT_VERTEX_USES_HWREGS

/* XXX Implement me XXX */
#define EMIT_VERTEX_TRI(VTX0, VTX1, VTX2)	\
	do { } while (0)
#define EMIT_VERTEX_LINE(VTX0, VTX1)		\
	do { } while (0)
#define EMIT_VERTEX_POINT(VTX0)			\
	do { } while (0)

#define TAG(x)	ffb_##x
#include "tnl_dd/t_dd_imm_primtmp.h"

/* Bzzt: Material changes are lost on fallback. */
static void ffb_Materialfv(GLenum face, GLenum pname,
			   const GLfloat *params)
{
	GET_CURRENT_CONTEXT(ctx);

	_mesa_noop_Materialfv( face, pname, params );
	ffb_recalc_base_color( ctx );
}

/* Fallback functions: */

static void ffb_do_fallback(GLcontext *ctx)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	struct ffb_current_state *current = &fmesa->vtx_state.current;

	/* Tell tnl to restore its exec vtxfmt, rehook its driver callbacks
	 * and revive internal state that depended on those callbacks:
	 */
	_tnl_wakeup_exec(ctx);

	/* Replay enough vertices that the current primitive is continued
	 * correctly:
	 */
	if (fmesa->imm.prim != PRIM_OUTSIDE_BEGIN_END )
		CALL_Begin(GET_DISPATCH(), (fmesa->imm.prim));

	if (ctx->Light.Enabled) {
		/* Catch ColorMaterial */
		CALL_Color4fv(GET_DISPATCH(), (ctx->Current.Color));
		CALL_Normal3fv(GET_DISPATCH(), (current->normal));
	} else {
		CALL_Color4fv(GET_DISPATCH(), (current->color));
	}
}

#define PRE_LOOPBACK( FUNC ) do {	\
   GET_CURRENT_CONTEXT(ctx);		\
   ffb_do_fallback( ctx );		\
} while (0)

#define TAG(x) ffb_fallback_##x
#include "vtxfmt_tmp.h"

static void ffb_Begin(GLenum prim)
{
	GET_CURRENT_CONTEXT(ctx);
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);

	if (prim > GL_POLYGON) {
		_mesa_error( ctx, GL_INVALID_ENUM, "glBegin" );
		return;
	}

	if (fmesa->imm.prim != PRIM_OUTSIDE_BEGIN_END) {
		_mesa_error( ctx, GL_INVALID_OPERATION, "glBegin" );
		return;
	}

	ctx->Driver.NeedFlush |= (FLUSH_STORED_VERTICES |
				  FLUSH_UPDATE_CURRENT);

	fmesa->imm.prim = prim;
	fmesa->imm.v0 = &fmesa->imm.vertices[0];
	fmesa->imm.save_vertex = ffb_save_vertex_RGBA;
	fmesa->imm.flush_vertex = ffb_flush_tab[prim];

	/* XXX Lock hardware, update FBC, PPC, DRAWOP, etc. XXX */
}

static void ffb_End(void)
{
	GET_CURRENT_CONTEXT(ctx);
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);

	if (fmesa->imm.prim == PRIM_OUTSIDE_BEGIN_END) {
		_mesa_error( ctx, GL_INVALID_OPERATION, "glEnd" );
		return;
	}

	fmesa->imm.prim = PRIM_OUTSIDE_BEGIN_END;

	ctx->Driver.NeedFlush &= ~(FLUSH_STORED_VERTICES |
				   FLUSH_UPDATE_CURRENT);

	/* XXX Unlock hardware, etc. */
}

void ffbInitTnlModule(GLcontext *ctx)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	GLvertexformat *vfmt = &(fmesa->imm.vtxfmt);

	/* Work in progress... */
	return;

	ffb_init_norm_funcs();
	ffb_init_vert_funcs();

	/* start by initializing to no-op functions */
	_mesa_noop_vtxfmt_init(vfmt);

	/* Handled fully in supported states: */
	vfmt->ArrayElement		= NULL;		/* FIXME: ... */
	vfmt->Color3f			= ffb_choose_Color3f;
	vfmt->Color3fv			= ffb_choose_Color3fv;
	vfmt->Color3ub			= ffb_choose_Color3ub;
	vfmt->Color3ubv			= ffb_choose_Color3ubv;
	vfmt->Color4f			= ffb_choose_Color4f;
	vfmt->Color4fv			= ffb_choose_Color4fv;
	vfmt->Color4ub			= ffb_choose_Color4ub;
	vfmt->Color4ubv			= ffb_choose_Color4ubv;
	vfmt->FogCoordfvEXT		= ffb_FogCoordfvEXT;
	vfmt->FogCoordfEXT		= ffb_FogCoordfEXT;
	vfmt->Materialfv		= ffb_Materialfv;
	vfmt->MultiTexCoord1fARB	= ffb_fallback_MultiTexCoord1fARB;
	vfmt->MultiTexCoord1fvARB	= ffb_fallback_MultiTexCoord1fvARB;
	vfmt->MultiTexCoord2fARB	= ffb_fallback_MultiTexCoord2fARB;
	vfmt->MultiTexCoord2fvARB	= ffb_fallback_MultiTexCoord2fvARB;
	vfmt->MultiTexCoord3fARB	= ffb_fallback_MultiTexCoord3fARB;
	vfmt->MultiTexCoord3fvARB	= ffb_fallback_MultiTexCoord3fvARB;
	vfmt->MultiTexCoord4fARB	= ffb_fallback_MultiTexCoord4fARB;
	vfmt->MultiTexCoord4fvARB	= ffb_fallback_MultiTexCoord4fvARB;
	vfmt->Normal3f			= ffb_choose_Normal3f;
	vfmt->Normal3fv			= ffb_choose_Normal3fv;
	vfmt->SecondaryColor3ubEXT	= ffb_SecondaryColor3ubEXT;
	vfmt->SecondaryColor3ubvEXT	= ffb_SecondaryColor3ubvEXT;
	vfmt->SecondaryColor3fEXT	= ffb_SecondaryColor3fEXT;
	vfmt->SecondaryColor3fvEXT	= ffb_SecondaryColor3fvEXT;
	vfmt->TexCoord1f		= ffb_fallback_TexCoord1f;
	vfmt->TexCoord1fv		= ffb_fallback_TexCoord1fv;
	vfmt->TexCoord2f		= ffb_fallback_TexCoord2f;
	vfmt->TexCoord2fv		= ffb_fallback_TexCoord2fv;
	vfmt->TexCoord3f		= ffb_fallback_TexCoord3f;
	vfmt->TexCoord3fv		= ffb_fallback_TexCoord3fv;
	vfmt->TexCoord4f		= ffb_fallback_TexCoord4f;
	vfmt->TexCoord4fv		= ffb_fallback_TexCoord4fv;

	vfmt->Vertex2f			= ffb_Vertex2f;
	vfmt->Vertex2fv			= ffb_Vertex2fv;
	vfmt->Vertex3f			= ffb_Vertex3f;
	vfmt->Vertex3fv			= ffb_Vertex3fv;
	vfmt->Vertex4f			= ffb_Vertex4f;
	vfmt->Vertex4fv			= ffb_Vertex4fv;

	vfmt->Begin			= ffb_Begin;
	vfmt->End			= ffb_End;

	vfmt->DrawArrays = NULL;
	vfmt->DrawElements = NULL;

	/* Active but unsupported -- fallback if we receive these:
	 *
	 * All of these fallbacks can be fixed with additional code, except
	 * CallList, unless we build a play_immediate_noop() command which
	 * turns an immediate back into glBegin/glEnd commands...
	 */
	vfmt->CallList = ffb_fallback_CallList;
	vfmt->EvalCoord1f = ffb_fallback_EvalCoord1f;
	vfmt->EvalCoord1fv = ffb_fallback_EvalCoord1fv;
	vfmt->EvalCoord2f = ffb_fallback_EvalCoord2f;
	vfmt->EvalCoord2fv = ffb_fallback_EvalCoord2fv;
	vfmt->EvalMesh1 = ffb_fallback_EvalMesh1;
	vfmt->EvalMesh2 = ffb_fallback_EvalMesh2;
	vfmt->EvalPoint1 = ffb_fallback_EvalPoint1;
	vfmt->EvalPoint2 = ffb_fallback_EvalPoint2;

	vfmt->prefer_float_colors = GL_TRUE;

	fmesa->imm.prim = PRIM_OUTSIDE_BEGIN_END;

	/* THIS IS A HACK! */
	_mesa_install_exec_vtxfmt( ctx, vfmt );
}
