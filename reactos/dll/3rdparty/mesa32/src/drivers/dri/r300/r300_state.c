/*
Copyright (C) The Weather Channel, Inc.  2002.
Copyright (C) 2004 Nicolai Haehnle.
All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/**
 * \file
 *
 * \author Nicolai Haehnle <prefect_@gmx.net>
 */

#include "glheader.h"
#include "state.h"
#include "imports.h"
#include "enums.h"
#include "macros.h"
#include "context.h"
#include "dd.h"
#include "simple_list.h"

#include "api_arrayelt.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "shader/prog_parameter.h"
#include "shader/prog_statevars.h"
#include "vbo/vbo.h"
#include "tnl/tnl.h"
#include "texformat.h"

#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "r300_context.h"
#include "r300_ioctl.h"
#include "r300_state.h"
#include "r300_reg.h"
#include "r300_emit.h"
#include "r300_fragprog.h"
#include "r300_tex.h"

#include "drirenderbuffer.h"

static void r300BlendColor(GLcontext * ctx, const GLfloat cf[4])
{
	GLubyte color[4];
	r300ContextPtr rmesa = R300_CONTEXT(ctx);

	R300_STATECHANGE(rmesa, blend_color);

	CLAMPED_FLOAT_TO_UBYTE(color[0], cf[0]);
	CLAMPED_FLOAT_TO_UBYTE(color[1], cf[1]);
	CLAMPED_FLOAT_TO_UBYTE(color[2], cf[2]);
	CLAMPED_FLOAT_TO_UBYTE(color[3], cf[3]);

	rmesa->hw.blend_color.cmd[1] = PACK_COLOR_8888(color[3], color[0],
						       color[1], color[2]);
}

/**
 * Calculate the hardware blend factor setting.  This same function is used
 * for source and destination of both alpha and RGB.
 *
 * \returns
 * The hardware register value for the specified blend factor.  This value
 * will need to be shifted into the correct position for either source or
 * destination factor.
 *
 * \todo
 * Since the two cases where source and destination are handled differently
 * are essentially error cases, they should never happen.  Determine if these
 * cases can be removed.
 */
static int blend_factor(GLenum factor, GLboolean is_src)
{
	switch (factor) {
	case GL_ZERO:
		return R300_BLEND_GL_ZERO;
		break;
	case GL_ONE:
		return R300_BLEND_GL_ONE;
		break;
	case GL_DST_COLOR:
		return R300_BLEND_GL_DST_COLOR;
		break;
	case GL_ONE_MINUS_DST_COLOR:
		return R300_BLEND_GL_ONE_MINUS_DST_COLOR;
		break;
	case GL_SRC_COLOR:
		return R300_BLEND_GL_SRC_COLOR;
		break;
	case GL_ONE_MINUS_SRC_COLOR:
		return R300_BLEND_GL_ONE_MINUS_SRC_COLOR;
		break;
	case GL_SRC_ALPHA:
		return R300_BLEND_GL_SRC_ALPHA;
		break;
	case GL_ONE_MINUS_SRC_ALPHA:
		return R300_BLEND_GL_ONE_MINUS_SRC_ALPHA;
		break;
	case GL_DST_ALPHA:
		return R300_BLEND_GL_DST_ALPHA;
		break;
	case GL_ONE_MINUS_DST_ALPHA:
		return R300_BLEND_GL_ONE_MINUS_DST_ALPHA;
		break;
	case GL_SRC_ALPHA_SATURATE:
		return (is_src) ? R300_BLEND_GL_SRC_ALPHA_SATURATE :
		    R300_BLEND_GL_ZERO;
		break;
	case GL_CONSTANT_COLOR:
		return R300_BLEND_GL_CONST_COLOR;
		break;
	case GL_ONE_MINUS_CONSTANT_COLOR:
		return R300_BLEND_GL_ONE_MINUS_CONST_COLOR;
		break;
	case GL_CONSTANT_ALPHA:
		return R300_BLEND_GL_CONST_ALPHA;
		break;
	case GL_ONE_MINUS_CONSTANT_ALPHA:
		return R300_BLEND_GL_ONE_MINUS_CONST_ALPHA;
		break;
	default:
		fprintf(stderr, "unknown blend factor %x\n", factor);
		return (is_src) ? R300_BLEND_GL_ONE : R300_BLEND_GL_ZERO;
		break;
	}
}

/**
 * Sets both the blend equation and the blend function.
 * This is done in a single
 * function because some blend equations (i.e., \c GL_MIN and \c GL_MAX)
 * change the interpretation of the blend function.
 * Also, make sure that blend function and blend equation are set to their
 * default value if color blending is not enabled, since at least blend
 * equations GL_MIN and GL_FUNC_REVERSE_SUBTRACT will cause wrong results
 * otherwise for unknown reasons.
 */

/* helper function */
static void r300SetBlendCntl(r300ContextPtr r300, int func, int eqn,
			     int cbits, int funcA, int eqnA)
{
	GLuint new_ablend, new_cblend;

#if 0
	fprintf(stderr,
		"eqnA=%08x funcA=%08x eqn=%08x func=%08x cbits=%08x\n",
		eqnA, funcA, eqn, func, cbits);
#endif
	new_ablend = eqnA | funcA;
	new_cblend = eqn | func;

	/* Some blend factor combinations don't seem to work when the
	 * BLEND_NO_SEPARATE bit is set.
	 *
	 * Especially problematic candidates are the ONE_MINUS_* flags,
	 * but I can't see a real pattern.
	 */
#if 0
	if (new_ablend == new_cblend) {
		new_cblend |= R300_BLEND_NO_SEPARATE;
	}
#endif
	new_cblend |= cbits;

	if ((new_ablend != r300->hw.bld.cmd[R300_BLD_ABLEND]) ||
	    (new_cblend != r300->hw.bld.cmd[R300_BLD_CBLEND])) {
		R300_STATECHANGE(r300, bld);
		r300->hw.bld.cmd[R300_BLD_ABLEND] = new_ablend;
		r300->hw.bld.cmd[R300_BLD_CBLEND] = new_cblend;
	}
}

static void r300SetBlendState(GLcontext * ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	int func = (R300_BLEND_GL_ONE << R300_SRC_BLEND_SHIFT) |
	    (R300_BLEND_GL_ZERO << R300_DST_BLEND_SHIFT);
	int eqn = R300_COMB_FCN_ADD_CLAMP;
	int funcA = (R300_BLEND_GL_ONE << R300_SRC_BLEND_SHIFT) |
	    (R300_BLEND_GL_ZERO << R300_DST_BLEND_SHIFT);
	int eqnA = R300_COMB_FCN_ADD_CLAMP;

	if (RGBA_LOGICOP_ENABLED(ctx) || !ctx->Color.BlendEnabled) {
		r300SetBlendCntl(r300, func, eqn, 0, func, eqn);
		return;
	}

	func =
	    (blend_factor(ctx->Color.BlendSrcRGB, GL_TRUE) <<
	     R300_SRC_BLEND_SHIFT) | (blend_factor(ctx->Color.BlendDstRGB,
						   GL_FALSE) <<
				      R300_DST_BLEND_SHIFT);

	switch (ctx->Color.BlendEquationRGB) {
	case GL_FUNC_ADD:
		eqn = R300_COMB_FCN_ADD_CLAMP;
		break;

	case GL_FUNC_SUBTRACT:
		eqn = R300_COMB_FCN_SUB_CLAMP;
		break;

	case GL_FUNC_REVERSE_SUBTRACT:
		eqn = R300_COMB_FCN_RSUB_CLAMP;
		break;

	case GL_MIN:
		eqn = R300_COMB_FCN_MIN;
		func = (R300_BLEND_GL_ONE << R300_SRC_BLEND_SHIFT) |
		    (R300_BLEND_GL_ONE << R300_DST_BLEND_SHIFT);
		break;

	case GL_MAX:
		eqn = R300_COMB_FCN_MAX;
		func = (R300_BLEND_GL_ONE << R300_SRC_BLEND_SHIFT) |
		    (R300_BLEND_GL_ONE << R300_DST_BLEND_SHIFT);
		break;

	default:
		fprintf(stderr,
			"[%s:%u] Invalid RGB blend equation (0x%04x).\n",
			__FUNCTION__, __LINE__, ctx->Color.BlendEquationRGB);
		return;
	}

	funcA =
	    (blend_factor(ctx->Color.BlendSrcA, GL_TRUE) <<
	     R300_SRC_BLEND_SHIFT) | (blend_factor(ctx->Color.BlendDstA,
						   GL_FALSE) <<
				      R300_DST_BLEND_SHIFT);

	switch (ctx->Color.BlendEquationA) {
	case GL_FUNC_ADD:
		eqnA = R300_COMB_FCN_ADD_CLAMP;
		break;

	case GL_FUNC_SUBTRACT:
		eqnA = R300_COMB_FCN_SUB_CLAMP;
		break;

	case GL_FUNC_REVERSE_SUBTRACT:
		eqnA = R300_COMB_FCN_RSUB_CLAMP;
		break;

	case GL_MIN:
		eqnA = R300_COMB_FCN_MIN;
		funcA = (R300_BLEND_GL_ONE << R300_SRC_BLEND_SHIFT) |
		    (R300_BLEND_GL_ONE << R300_DST_BLEND_SHIFT);
		break;

	case GL_MAX:
		eqnA = R300_COMB_FCN_MAX;
		funcA = (R300_BLEND_GL_ONE << R300_SRC_BLEND_SHIFT) |
		    (R300_BLEND_GL_ONE << R300_DST_BLEND_SHIFT);
		break;

	default:
		fprintf(stderr,
			"[%s:%u] Invalid A blend equation (0x%04x).\n",
			__FUNCTION__, __LINE__, ctx->Color.BlendEquationA);
		return;
	}

	r300SetBlendCntl(r300,
			 func, eqn,
			 R300_BLEND_UNKNOWN | R300_BLEND_ENABLE, funcA, eqnA);
}

static void r300BlendEquationSeparate(GLcontext * ctx,
				      GLenum modeRGB, GLenum modeA)
{
	r300SetBlendState(ctx);
}

static void r300BlendFuncSeparate(GLcontext * ctx,
				  GLenum sfactorRGB, GLenum dfactorRGB,
				  GLenum sfactorA, GLenum dfactorA)
{
	r300SetBlendState(ctx);
}

/**
 * Update our tracked culling state based on Mesa's state.
 */
static void r300UpdateCulling(GLcontext * ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	uint32_t val = 0;

	R300_STATECHANGE(r300, cul);
	if (ctx->Polygon.CullFlag) {
		if (ctx->Polygon.CullFaceMode == GL_FRONT_AND_BACK)
			val = R300_CULL_FRONT | R300_CULL_BACK;
		else if (ctx->Polygon.CullFaceMode == GL_FRONT)
			val = R300_CULL_FRONT;
		else
			val = R300_CULL_BACK;

		if (ctx->Polygon.FrontFace == GL_CW)
			val |= R300_FRONT_FACE_CW;
		else
			val |= R300_FRONT_FACE_CCW;
	}
	r300->hw.cul.cmd[R300_CUL_CULL] = val;
}

static void r300SetEarlyZState(GLcontext * ctx)
{
	/* updates register R300_RB3D_EARLY_Z (0x4F14)
	   if depth test is not enabled it should be R300_EARLY_Z_DISABLE
	   if depth is enabled and alpha not it should be R300_EARLY_Z_ENABLE
	   if depth and alpha is enabled it should be R300_EARLY_Z_DISABLE
	 */
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	R300_STATECHANGE(r300, zstencil_format);
	if (ctx->Color.AlphaEnabled && ctx->Color.AlphaFunc != GL_ALWAYS)
		/* disable early Z */
		r300->hw.zstencil_format.cmd[2] = R300_EARLY_Z_DISABLE;
	else {
		if (ctx->Depth.Test && ctx->Depth.Func != GL_NEVER)
			/* enable early Z */
			r300->hw.zstencil_format.cmd[2] = R300_EARLY_Z_ENABLE;
		else
			/* disable early Z */
			r300->hw.zstencil_format.cmd[2] = R300_EARLY_Z_DISABLE;
	}
}

static void r300SetAlphaState(GLcontext * ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	GLubyte refByte;
	uint32_t pp_misc = 0x0;
	GLboolean really_enabled = ctx->Color.AlphaEnabled;

	CLAMPED_FLOAT_TO_UBYTE(refByte, ctx->Color.AlphaRef);

	switch (ctx->Color.AlphaFunc) {
	case GL_NEVER:
		pp_misc |= R300_ALPHA_TEST_FAIL;
		break;
	case GL_LESS:
		pp_misc |= R300_ALPHA_TEST_LESS;
		break;
	case GL_EQUAL:
		pp_misc |= R300_ALPHA_TEST_EQUAL;
		break;
	case GL_LEQUAL:
		pp_misc |= R300_ALPHA_TEST_LEQUAL;
		break;
	case GL_GREATER:
		pp_misc |= R300_ALPHA_TEST_GREATER;
		break;
	case GL_NOTEQUAL:
		pp_misc |= R300_ALPHA_TEST_NEQUAL;
		break;
	case GL_GEQUAL:
		pp_misc |= R300_ALPHA_TEST_GEQUAL;
		break;
	case GL_ALWAYS:
		/*pp_misc |= R300_ALPHA_TEST_PASS; */
		really_enabled = GL_FALSE;
		break;
	}

	if (really_enabled) {
		pp_misc |= R300_ALPHA_TEST_ENABLE;
		pp_misc |= (refByte & R300_REF_ALPHA_MASK);
	} else {
		pp_misc = 0x0;
	}

	R300_STATECHANGE(r300, at);
	r300->hw.at.cmd[R300_AT_ALPHA_TEST] = pp_misc;

	r300SetEarlyZState(ctx);
}

static void r300AlphaFunc(GLcontext * ctx, GLenum func, GLfloat ref)
{
	(void)func;
	(void)ref;
	r300SetAlphaState(ctx);
}

static int translate_func(int func)
{
	switch (func) {
	case GL_NEVER:
		return R300_ZS_NEVER;
	case GL_LESS:
		return R300_ZS_LESS;
	case GL_EQUAL:
		return R300_ZS_EQUAL;
	case GL_LEQUAL:
		return R300_ZS_LEQUAL;
	case GL_GREATER:
		return R300_ZS_GREATER;
	case GL_NOTEQUAL:
		return R300_ZS_NOTEQUAL;
	case GL_GEQUAL:
		return R300_ZS_GEQUAL;
	case GL_ALWAYS:
		return R300_ZS_ALWAYS;
	}
	return 0;
}

static void r300SetDepthState(GLcontext * ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	R300_STATECHANGE(r300, zs);
	r300->hw.zs.cmd[R300_ZS_CNTL_0] &= R300_RB3D_STENCIL_ENABLE;
	r300->hw.zs.cmd[R300_ZS_CNTL_1] &=
	    ~(R300_ZS_MASK << R300_RB3D_ZS1_DEPTH_FUNC_SHIFT);

	if (ctx->Depth.Test && ctx->Depth.Func != GL_NEVER) {
		if (ctx->Depth.Mask)
			r300->hw.zs.cmd[R300_ZS_CNTL_0] |=
			    R300_RB3D_Z_TEST_AND_WRITE;
		else
			r300->hw.zs.cmd[R300_ZS_CNTL_0] |= R300_RB3D_Z_TEST;

		r300->hw.zs.cmd[R300_ZS_CNTL_1] |=
		    translate_func(ctx->Depth.
				   Func) << R300_RB3D_ZS1_DEPTH_FUNC_SHIFT;
	} else {
		r300->hw.zs.cmd[R300_ZS_CNTL_0] |= R300_RB3D_Z_DISABLED_1;
		r300->hw.zs.cmd[R300_ZS_CNTL_1] |=
		    translate_func(GL_NEVER) << R300_RB3D_ZS1_DEPTH_FUNC_SHIFT;
	}

	r300SetEarlyZState(ctx);
}

/**
 * Handle glEnable()/glDisable().
 *
 * \note Mesa already filters redundant calls to glEnable/glDisable.
 */
static void r300Enable(GLcontext * ctx, GLenum cap, GLboolean state)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	if (RADEON_DEBUG & DEBUG_STATE)
		fprintf(stderr, "%s( %s = %s )\n", __FUNCTION__,
			_mesa_lookup_enum_by_nr(cap),
			state ? "GL_TRUE" : "GL_FALSE");

	switch (cap) {
		/* Fast track this one...
		 */
	case GL_TEXTURE_1D:
	case GL_TEXTURE_2D:
	case GL_TEXTURE_3D:
		break;

	case GL_FOG:
		R300_STATECHANGE(r300, fogs);
		if (state) {
			r300->hw.fogs.cmd[R300_FOGS_STATE] |= R300_FOG_ENABLE;

			ctx->Driver.Fogfv(ctx, GL_FOG_MODE, NULL);
			ctx->Driver.Fogfv(ctx, GL_FOG_DENSITY,
					  &ctx->Fog.Density);
			ctx->Driver.Fogfv(ctx, GL_FOG_START, &ctx->Fog.Start);
			ctx->Driver.Fogfv(ctx, GL_FOG_END, &ctx->Fog.End);
			ctx->Driver.Fogfv(ctx, GL_FOG_COLOR, ctx->Fog.Color);
		} else {
			r300->hw.fogs.cmd[R300_FOGS_STATE] &= ~R300_FOG_ENABLE;
		}

		break;

	case GL_ALPHA_TEST:
		r300SetAlphaState(ctx);
		break;

	case GL_BLEND:
	case GL_COLOR_LOGIC_OP:
		r300SetBlendState(ctx);
		break;

	case GL_DEPTH_TEST:
		r300SetDepthState(ctx);
		break;

	case GL_STENCIL_TEST:
		if (r300->state.stencil.hw_stencil) {
			R300_STATECHANGE(r300, zs);
			if (state) {
				r300->hw.zs.cmd[R300_ZS_CNTL_0] |=
				    R300_RB3D_STENCIL_ENABLE;
			} else {
				r300->hw.zs.cmd[R300_ZS_CNTL_0] &=
				    ~R300_RB3D_STENCIL_ENABLE;
			}
		} else {
#if R200_MERGED
			FALLBACK(&r300->radeon, RADEON_FALLBACK_STENCIL, state);
#endif
		}
		break;

	case GL_CULL_FACE:
		r300UpdateCulling(ctx);
		break;

	case GL_POLYGON_OFFSET_POINT:
	case GL_POLYGON_OFFSET_LINE:
		break;

	case GL_POLYGON_OFFSET_FILL:
		R300_STATECHANGE(r300, occlusion_cntl);
		if (state) {
			r300->hw.occlusion_cntl.cmd[1] |= (3 << 0);
		} else {
			r300->hw.occlusion_cntl.cmd[1] &= ~(3 << 0);
		}
		break;
	default:
		radeonEnable(ctx, cap, state);
		return;
	}
}

static void r300UpdatePolygonMode(GLcontext * ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	uint32_t hw_mode = 0;

	if (ctx->Polygon.FrontMode != GL_FILL ||
	    ctx->Polygon.BackMode != GL_FILL) {
		GLenum f, b;

		if (ctx->Polygon.FrontFace == GL_CCW) {
			f = ctx->Polygon.FrontMode;
			b = ctx->Polygon.BackMode;
		} else {
			f = ctx->Polygon.BackMode;
			b = ctx->Polygon.FrontMode;
		}

		hw_mode |= R300_PM_ENABLED;

		switch (f) {
		case GL_LINE:
			hw_mode |= R300_PM_FRONT_LINE;
			break;
		case GL_POINT:	/* noop */
			hw_mode |= R300_PM_FRONT_POINT;
			break;
		case GL_FILL:
			hw_mode |= R300_PM_FRONT_FILL;
			break;
		}

		switch (b) {
		case GL_LINE:
			hw_mode |= R300_PM_BACK_LINE;
			break;
		case GL_POINT:	/* noop */
			hw_mode |= R300_PM_BACK_POINT;
			break;
		case GL_FILL:
			hw_mode |= R300_PM_BACK_FILL;
			break;
		}
	}

	if (r300->hw.polygon_mode.cmd[1] != hw_mode) {
		R300_STATECHANGE(r300, polygon_mode);
		r300->hw.polygon_mode.cmd[1] = hw_mode;
	}
}

/**
 * Change the culling mode.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r300CullFace(GLcontext * ctx, GLenum mode)
{
	(void)mode;

	r300UpdateCulling(ctx);
}

/**
 * Change the polygon orientation.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r300FrontFace(GLcontext * ctx, GLenum mode)
{
	(void)mode;

	r300UpdateCulling(ctx);
	r300UpdatePolygonMode(ctx);
}

/**
 * Change the depth testing function.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r300DepthFunc(GLcontext * ctx, GLenum func)
{
	(void)func;
	r300SetDepthState(ctx);
}

/**
 * Enable/Disable depth writing.
 *
 * \note Mesa already filters redundant calls to this function.
 */
static void r300DepthMask(GLcontext * ctx, GLboolean mask)
{
	(void)mask;
	r300SetDepthState(ctx);
}

/**
 * Handle glColorMask()
 */
static void r300ColorMask(GLcontext * ctx,
			  GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	int mask = (r ? R300_COLORMASK0_R : 0) |
	    (g ? R300_COLORMASK0_G : 0) |
	    (b ? R300_COLORMASK0_B : 0) | (a ? R300_COLORMASK0_A : 0);

	if (mask != r300->hw.cmk.cmd[R300_CMK_COLORMASK]) {
		R300_STATECHANGE(r300, cmk);
		r300->hw.cmk.cmd[R300_CMK_COLORMASK] = mask;
	}
}

/* =============================================================
 * Fog
 */
static void r300Fogfv(GLcontext * ctx, GLenum pname, const GLfloat * param)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	union {
		int i;
		float f;
	} fogScale, fogStart;

	(void)param;

	fogScale.i = r300->hw.fogp.cmd[R300_FOGP_SCALE];
	fogStart.i = r300->hw.fogp.cmd[R300_FOGP_START];

	switch (pname) {
	case GL_FOG_MODE:
		if (!ctx->Fog.Enabled)
			return;
		switch (ctx->Fog.Mode) {
		case GL_LINEAR:
			R300_STATECHANGE(r300, fogs);
			r300->hw.fogs.cmd[R300_FOGS_STATE] =
			    (r300->hw.fogs.
			     cmd[R300_FOGS_STATE] & ~R300_FOG_MODE_MASK) |
			    R300_FOG_MODE_LINEAR;

			if (ctx->Fog.Start == ctx->Fog.End) {
				fogScale.f = -1.0;
				fogStart.f = 1.0;
			} else {
				fogScale.f =
				    1.0 / (ctx->Fog.End - ctx->Fog.Start);
				fogStart.f =
				    -ctx->Fog.Start / (ctx->Fog.End -
						       ctx->Fog.Start);
			}
			break;
		case GL_EXP:
			R300_STATECHANGE(r300, fogs);
			r300->hw.fogs.cmd[R300_FOGS_STATE] =
			    (r300->hw.fogs.
			     cmd[R300_FOGS_STATE] & ~R300_FOG_MODE_MASK) |
			    R300_FOG_MODE_EXP;
			fogScale.f = 0.0933 * ctx->Fog.Density;
			fogStart.f = 0.0;
			break;
		case GL_EXP2:
			R300_STATECHANGE(r300, fogs);
			r300->hw.fogs.cmd[R300_FOGS_STATE] =
			    (r300->hw.fogs.
			     cmd[R300_FOGS_STATE] & ~R300_FOG_MODE_MASK) |
			    R300_FOG_MODE_EXP2;
			fogScale.f = 0.3 * ctx->Fog.Density;
			fogStart.f = 0.0;
		default:
			return;
		}
		break;
	case GL_FOG_DENSITY:
		switch (ctx->Fog.Mode) {
		case GL_EXP:
			fogScale.f = 0.0933 * ctx->Fog.Density;
			fogStart.f = 0.0;
			break;
		case GL_EXP2:
			fogScale.f = 0.3 * ctx->Fog.Density;
			fogStart.f = 0.0;
		default:
			break;
		}
		break;
	case GL_FOG_START:
	case GL_FOG_END:
		if (ctx->Fog.Mode == GL_LINEAR) {
			if (ctx->Fog.Start == ctx->Fog.End) {
				fogScale.f = -1.0;
				fogStart.f = 1.0;
			} else {
				fogScale.f =
				    1.0 / (ctx->Fog.End - ctx->Fog.Start);
				fogStart.f =
				    -ctx->Fog.Start / (ctx->Fog.End -
						       ctx->Fog.Start);
			}
		}
		break;
	case GL_FOG_COLOR:
		R300_STATECHANGE(r300, fogc);
		r300->hw.fogc.cmd[R300_FOGC_R] =
		    (GLuint) (ctx->Fog.Color[0] * 1023.0F) & 0x3FF;
		r300->hw.fogc.cmd[R300_FOGC_G] =
		    (GLuint) (ctx->Fog.Color[1] * 1023.0F) & 0x3FF;
		r300->hw.fogc.cmd[R300_FOGC_B] =
		    (GLuint) (ctx->Fog.Color[2] * 1023.0F) & 0x3FF;
		break;
	case GL_FOG_COORD_SRC:
		break;
	default:
		return;
	}

	if (fogScale.i != r300->hw.fogp.cmd[R300_FOGP_SCALE] ||
	    fogStart.i != r300->hw.fogp.cmd[R300_FOGP_START]) {
		R300_STATECHANGE(r300, fogp);
		r300->hw.fogp.cmd[R300_FOGP_SCALE] = fogScale.i;
		r300->hw.fogp.cmd[R300_FOGP_START] = fogStart.i;
	}
}

/* =============================================================
 * Point state
 */
static void r300PointSize(GLcontext * ctx, GLfloat size)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	size = ctx->Point._Size;

	R300_STATECHANGE(r300, ps);
	r300->hw.ps.cmd[R300_PS_POINTSIZE] =
	    ((int)(size * 6) << R300_POINTSIZE_X_SHIFT) |
	    ((int)(size * 6) << R300_POINTSIZE_Y_SHIFT);
}

/* =============================================================
 * Line state
 */
static void r300LineWidth(GLcontext * ctx, GLfloat widthf)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	widthf = ctx->Line._Width;

	R300_STATECHANGE(r300, lcntl);
	r300->hw.lcntl.cmd[1] = (int)(widthf * 6.0);
	r300->hw.lcntl.cmd[1] |= R300_LINE_CNT_VE;
}

static void r300PolygonMode(GLcontext * ctx, GLenum face, GLenum mode)
{
	(void)face;
	(void)mode;

	r300UpdatePolygonMode(ctx);
}

/* =============================================================
 * Stencil
 */

static int translate_stencil_op(int op)
{
	switch (op) {
	case GL_KEEP:
		return R300_ZS_KEEP;
	case GL_ZERO:
		return R300_ZS_ZERO;
	case GL_REPLACE:
		return R300_ZS_REPLACE;
	case GL_INCR:
		return R300_ZS_INCR;
	case GL_DECR:
		return R300_ZS_DECR;
	case GL_INCR_WRAP_EXT:
		return R300_ZS_INCR_WRAP;
	case GL_DECR_WRAP_EXT:
		return R300_ZS_DECR_WRAP;
	case GL_INVERT:
		return R300_ZS_INVERT;
	default:
		WARN_ONCE("Do not know how to translate stencil op");
		return R300_ZS_KEEP;
	}
	return 0;
}

static void r300ShadeModel(GLcontext * ctx, GLenum mode)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);

	R300_STATECHANGE(rmesa, shade);
	switch (mode) {
	case GL_FLAT:
		rmesa->hw.shade.cmd[2] = R300_RE_SHADE_MODEL_FLAT;
		break;
	case GL_SMOOTH:
		rmesa->hw.shade.cmd[2] = R300_RE_SHADE_MODEL_SMOOTH;
		break;
	default:
		return;
	}
}

static void r300StencilFuncSeparate(GLcontext * ctx, GLenum face,
				    GLenum func, GLint ref, GLuint mask)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	GLuint refmask =
	    (((ctx->Stencil.
	       Ref[0] & 0xff) << R300_RB3D_ZS2_STENCIL_REF_SHIFT) | ((ctx->
								      Stencil.
								      ValueMask
								      [0] &
								      0xff)
								     <<
								     R300_RB3D_ZS2_STENCIL_MASK_SHIFT));

	GLuint flag;

	R300_STATECHANGE(rmesa, zs);

	rmesa->hw.zs.cmd[R300_ZS_CNTL_1] &= ~((R300_ZS_MASK <<
					       R300_RB3D_ZS1_FRONT_FUNC_SHIFT)
					      | (R300_ZS_MASK <<
						 R300_RB3D_ZS1_BACK_FUNC_SHIFT));

	rmesa->hw.zs.cmd[R300_ZS_CNTL_2] &=
	    ~((R300_RB3D_ZS2_STENCIL_MASK <<
	       R300_RB3D_ZS2_STENCIL_REF_SHIFT) |
	      (R300_RB3D_ZS2_STENCIL_MASK << R300_RB3D_ZS2_STENCIL_MASK_SHIFT));

	flag = translate_func(ctx->Stencil.Function[0]);
	rmesa->hw.zs.cmd[R300_ZS_CNTL_1] |=
	    (flag << R300_RB3D_ZS1_FRONT_FUNC_SHIFT);

	if (ctx->Stencil._TestTwoSide)
		flag = translate_func(ctx->Stencil.Function[1]);

	rmesa->hw.zs.cmd[R300_ZS_CNTL_1] |=
	    (flag << R300_RB3D_ZS1_BACK_FUNC_SHIFT);
	rmesa->hw.zs.cmd[R300_ZS_CNTL_2] |= refmask;
}

static void r300StencilMaskSeparate(GLcontext * ctx, GLenum face, GLuint mask)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);

	R300_STATECHANGE(rmesa, zs);
	rmesa->hw.zs.cmd[R300_ZS_CNTL_2] &=
	    ~(R300_RB3D_ZS2_STENCIL_MASK <<
	      R300_RB3D_ZS2_STENCIL_WRITE_MASK_SHIFT);
	rmesa->hw.zs.cmd[R300_ZS_CNTL_2] |=
	    (ctx->Stencil.
	     WriteMask[0] & 0xff) << R300_RB3D_ZS2_STENCIL_WRITE_MASK_SHIFT;
}

static void r300StencilOpSeparate(GLcontext * ctx, GLenum face,
				  GLenum fail, GLenum zfail, GLenum zpass)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);

	R300_STATECHANGE(rmesa, zs);
	/* It is easier to mask what's left.. */
	rmesa->hw.zs.cmd[R300_ZS_CNTL_1] &=
	    (R300_ZS_MASK << R300_RB3D_ZS1_DEPTH_FUNC_SHIFT) |
	    (R300_ZS_MASK << R300_RB3D_ZS1_FRONT_FUNC_SHIFT) |
	    (R300_ZS_MASK << R300_RB3D_ZS1_BACK_FUNC_SHIFT);

	rmesa->hw.zs.cmd[R300_ZS_CNTL_1] |=
	    (translate_stencil_op(ctx->Stencil.FailFunc[0]) <<
	     R300_RB3D_ZS1_FRONT_FAIL_OP_SHIFT)
	    | (translate_stencil_op(ctx->Stencil.ZFailFunc[0]) <<
	       R300_RB3D_ZS1_FRONT_ZFAIL_OP_SHIFT)
	    | (translate_stencil_op(ctx->Stencil.ZPassFunc[0]) <<
	       R300_RB3D_ZS1_FRONT_ZPASS_OP_SHIFT);

	if (ctx->Stencil._TestTwoSide) {
		rmesa->hw.zs.cmd[R300_ZS_CNTL_1] |=
		    (translate_stencil_op(ctx->Stencil.FailFunc[1]) <<
		     R300_RB3D_ZS1_BACK_FAIL_OP_SHIFT)
		    | (translate_stencil_op(ctx->Stencil.ZFailFunc[1]) <<
		       R300_RB3D_ZS1_BACK_ZFAIL_OP_SHIFT)
		    | (translate_stencil_op(ctx->Stencil.ZPassFunc[1]) <<
		       R300_RB3D_ZS1_BACK_ZPASS_OP_SHIFT);
	} else {
		rmesa->hw.zs.cmd[R300_ZS_CNTL_1] |=
		    (translate_stencil_op(ctx->Stencil.FailFunc[0]) <<
		     R300_RB3D_ZS1_BACK_FAIL_OP_SHIFT)
		    | (translate_stencil_op(ctx->Stencil.ZFailFunc[0]) <<
		       R300_RB3D_ZS1_BACK_ZFAIL_OP_SHIFT)
		    | (translate_stencil_op(ctx->Stencil.ZPassFunc[0]) <<
		       R300_RB3D_ZS1_BACK_ZPASS_OP_SHIFT);
	}
}

static void r300ClearStencil(GLcontext * ctx, GLint s)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);

	rmesa->state.stencil.clear =
	    ((GLuint) (ctx->Stencil.Clear & 0xff) |
	     (R300_RB3D_ZS2_STENCIL_MASK <<
	      R300_RB3D_ZS2_STENCIL_MASK_SHIFT) | ((ctx->Stencil.
						    WriteMask[0] & 0xff) <<
						   R300_RB3D_ZS2_STENCIL_WRITE_MASK_SHIFT));
}

/* =============================================================
 * Window position and viewport transformation
 */

/*
 * To correctly position primitives:
 */
#define SUBPIXEL_X 0.125
#define SUBPIXEL_Y 0.125

static void r300UpdateWindow(GLcontext * ctx)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	__DRIdrawablePrivate *dPriv = rmesa->radeon.dri.drawable;
	GLfloat xoffset = dPriv ? (GLfloat) dPriv->x : 0;
	GLfloat yoffset = dPriv ? (GLfloat) dPriv->y + dPriv->h : 0;
	const GLfloat *v = ctx->Viewport._WindowMap.m;

	GLfloat sx = v[MAT_SX];
	GLfloat tx = v[MAT_TX] + xoffset + SUBPIXEL_X;
	GLfloat sy = -v[MAT_SY];
	GLfloat ty = (-v[MAT_TY]) + yoffset + SUBPIXEL_Y;
	GLfloat sz = v[MAT_SZ] * rmesa->state.depth.scale;
	GLfloat tz = v[MAT_TZ] * rmesa->state.depth.scale;

	R300_FIREVERTICES(rmesa);
	R300_STATECHANGE(rmesa, vpt);

	rmesa->hw.vpt.cmd[R300_VPT_XSCALE] = r300PackFloat32(sx);
	rmesa->hw.vpt.cmd[R300_VPT_XOFFSET] = r300PackFloat32(tx);
	rmesa->hw.vpt.cmd[R300_VPT_YSCALE] = r300PackFloat32(sy);
	rmesa->hw.vpt.cmd[R300_VPT_YOFFSET] = r300PackFloat32(ty);
	rmesa->hw.vpt.cmd[R300_VPT_ZSCALE] = r300PackFloat32(sz);
	rmesa->hw.vpt.cmd[R300_VPT_ZOFFSET] = r300PackFloat32(tz);
}

static void r300Viewport(GLcontext * ctx, GLint x, GLint y,
			 GLsizei width, GLsizei height)
{
	/* Don't pipeline viewport changes, conflict with window offset
	 * setting below.  Could apply deltas to rescue pipelined viewport
	 * values, or keep the originals hanging around.
	 */
	r300UpdateWindow(ctx);
}

static void r300DepthRange(GLcontext * ctx, GLclampd nearval, GLclampd farval)
{
	r300UpdateWindow(ctx);
}

void r300UpdateViewportOffset(GLcontext * ctx)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	__DRIdrawablePrivate *dPriv = ((radeonContextPtr) rmesa)->dri.drawable;
	GLfloat xoffset = (GLfloat) dPriv->x;
	GLfloat yoffset = (GLfloat) dPriv->y + dPriv->h;
	const GLfloat *v = ctx->Viewport._WindowMap.m;

	GLfloat tx = v[MAT_TX] + xoffset + SUBPIXEL_X;
	GLfloat ty = (-v[MAT_TY]) + yoffset + SUBPIXEL_Y;

	if (rmesa->hw.vpt.cmd[R300_VPT_XOFFSET] != r300PackFloat32(tx) ||
	    rmesa->hw.vpt.cmd[R300_VPT_YOFFSET] != r300PackFloat32(ty)) {
		/* Note: this should also modify whatever data the context reset
		 * code uses...
		 */
		R300_STATECHANGE(rmesa, vpt);
		rmesa->hw.vpt.cmd[R300_VPT_XOFFSET] = r300PackFloat32(tx);
		rmesa->hw.vpt.cmd[R300_VPT_YOFFSET] = r300PackFloat32(ty);

	}

	radeonUpdateScissor(ctx);
}

/**
 * Tell the card where to render (offset, pitch).
 * Effected by glDrawBuffer, etc
 */
void r300UpdateDrawBuffer(GLcontext * ctx)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	r300ContextPtr r300 = rmesa;
	struct gl_framebuffer *fb = ctx->DrawBuffer;
	driRenderbuffer *drb;

	if (fb->_ColorDrawBufferMask[0] == BUFFER_BIT_FRONT_LEFT) {
		/* draw to front */
		drb =
		    (driRenderbuffer *) fb->Attachment[BUFFER_FRONT_LEFT].
		    Renderbuffer;
	} else if (fb->_ColorDrawBufferMask[0] == BUFFER_BIT_BACK_LEFT) {
		/* draw to back */
		drb =
		    (driRenderbuffer *) fb->Attachment[BUFFER_BACK_LEFT].
		    Renderbuffer;
	} else {
		/* drawing to multiple buffers, or none */
		return;
	}

	assert(drb);
	assert(drb->flippedPitch);

	R300_STATECHANGE(rmesa, cb);

	r300->hw.cb.cmd[R300_CB_OFFSET] = drb->flippedOffset +	//r300->radeon.state.color.drawOffset +
	    r300->radeon.radeonScreen->fbLocation;
	r300->hw.cb.cmd[R300_CB_PITCH] = drb->flippedPitch;	//r300->radeon.state.color.drawPitch;

	if (r300->radeon.radeonScreen->cpp == 4)
		r300->hw.cb.cmd[R300_CB_PITCH] |= R300_COLOR_FORMAT_ARGB8888;
	else
		r300->hw.cb.cmd[R300_CB_PITCH] |= R300_COLOR_FORMAT_RGB565;

	if (r300->radeon.sarea->tiling_enabled)
		r300->hw.cb.cmd[R300_CB_PITCH] |= R300_COLOR_TILE_ENABLE;
#if 0
	R200_STATECHANGE(rmesa, ctx);

	/* Note: we used the (possibly) page-flipped values */
	rmesa->hw.ctx.cmd[CTX_RB3D_COLOROFFSET]
	    = ((drb->flippedOffset + rmesa->r200Screen->fbLocation)
	       & R200_COLOROFFSET_MASK);
	rmesa->hw.ctx.cmd[CTX_RB3D_COLORPITCH] = drb->flippedPitch;

	if (rmesa->sarea->tiling_enabled) {
		rmesa->hw.ctx.cmd[CTX_RB3D_COLORPITCH] |=
		    R200_COLOR_TILE_ENABLE;
	}
#endif
}

static void
r300FetchStateParameter(GLcontext * ctx,
			const gl_state_index state[STATE_LENGTH],
			GLfloat * value)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	switch (state[0]) {
	case STATE_INTERNAL:
		switch (state[1]) {
		case STATE_R300_WINDOW_DIMENSION:
			value[0] = r300->radeon.dri.drawable->w * 0.5f;	/* width*0.5 */
			value[1] = r300->radeon.dri.drawable->h * 0.5f;	/* height*0.5 */
			value[2] = 0.5F;	/* for moving range [-1 1] -> [0 1] */
			value[3] = 1.0F;	/* not used */
			break;

		case STATE_R300_TEXRECT_FACTOR:{
				struct gl_texture_object *t =
				    ctx->Texture.Unit[state[2]].CurrentRect;

				if (t && t->Image[0][t->BaseLevel]) {
					struct gl_texture_image *image =
					    t->Image[0][t->BaseLevel];
					value[0] = 1.0 / image->Width2;
					value[1] = 1.0 / image->Height2;
				} else {
					value[0] = 1.0;
					value[1] = 1.0;
				}
				value[2] = 1.0;
				value[3] = 1.0;
				break;
			}

		default:
			break;
		}
		break;

	default:
		break;
	}
}

/**
 * Update R300's own internal state parameters.
 * For now just STATE_R300_WINDOW_DIMENSION
 */
void r300UpdateStateParameters(GLcontext * ctx, GLuint new_state)
{
	struct r300_fragment_program *fp;
	struct gl_program_parameter_list *paramList;
	GLuint i;

	if (!(new_state & (_NEW_BUFFERS | _NEW_PROGRAM)))
		return;

	fp = (struct r300_fragment_program *)ctx->FragmentProgram._Current;
	if (!fp)
		return;

	paramList = fp->mesa_program.Base.Parameters;

	if (!paramList)
		return;

	for (i = 0; i < paramList->NumParameters; i++) {
		if (paramList->Parameters[i].Type == PROGRAM_STATE_VAR) {
			r300FetchStateParameter(ctx,
						paramList->Parameters[i].
						StateIndexes,
						paramList->ParameterValues[i]);
		}
	}
}

/* =============================================================
 * Polygon state
 */
static void r300PolygonOffset(GLcontext * ctx, GLfloat factor, GLfloat units)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	GLfloat constant = units;

	switch (ctx->Visual.depthBits) {
	case 16:
		constant *= 4.0;
		break;
	case 24:
		constant *= 2.0;
		break;
	}

	factor *= 12.0;

/*    fprintf(stderr, "%s f:%f u:%f\n", __FUNCTION__, factor, constant); */

	R300_STATECHANGE(rmesa, zbs);
	rmesa->hw.zbs.cmd[R300_ZBS_T_FACTOR] = r300PackFloat32(factor);
	rmesa->hw.zbs.cmd[R300_ZBS_T_CONSTANT] = r300PackFloat32(constant);
	rmesa->hw.zbs.cmd[R300_ZBS_W_FACTOR] = r300PackFloat32(factor);
	rmesa->hw.zbs.cmd[R300_ZBS_W_CONSTANT] = r300PackFloat32(constant);
}

/* Routing and texture-related */

/* r300 doesnt handle GL_CLAMP and GL_MIRROR_CLAMP_EXT correctly when filter is NEAREST.
 * Since texwrap produces same results for GL_CLAMP and GL_CLAMP_TO_EDGE we use them instead.
 * We need to recalculate wrap modes whenever filter mode is changed because someone might do:
 * glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
 * glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
 * glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
 * Since r300 completely ignores R300_TX_CLAMP when either min or mag is nearest it cant handle
 * combinations where only one of them is nearest.
 */
static unsigned long gen_fixed_filter(unsigned long f)
{
	unsigned long mag, min, needs_fixing = 0;
	//return f;

	/* We ignore MIRROR bit so we dont have to do everything twice */
	if ((f & ((7 - 1) << R300_TX_WRAP_S_SHIFT)) ==
	    (R300_TX_CLAMP << R300_TX_WRAP_S_SHIFT)) {
		needs_fixing |= 1;
	}
	if ((f & ((7 - 1) << R300_TX_WRAP_T_SHIFT)) ==
	    (R300_TX_CLAMP << R300_TX_WRAP_T_SHIFT)) {
		needs_fixing |= 2;
	}
	if ((f & ((7 - 1) << R300_TX_WRAP_Q_SHIFT)) ==
	    (R300_TX_CLAMP << R300_TX_WRAP_Q_SHIFT)) {
		needs_fixing |= 4;
	}

	if (!needs_fixing)
		return f;

	mag = f & R300_TX_MAG_FILTER_MASK;
	min = f & R300_TX_MIN_FILTER_MASK;

	/* TODO: Check for anisto filters too */
	if ((mag != R300_TX_MAG_FILTER_NEAREST)
	    && (min != R300_TX_MIN_FILTER_NEAREST))
		return f;

	/* r300 cant handle these modes hence we force nearest to linear */
	if ((mag == R300_TX_MAG_FILTER_NEAREST)
	    && (min != R300_TX_MIN_FILTER_NEAREST)) {
		f &= ~R300_TX_MAG_FILTER_NEAREST;
		f |= R300_TX_MAG_FILTER_LINEAR;
		return f;
	}

	if ((min == R300_TX_MIN_FILTER_NEAREST)
	    && (mag != R300_TX_MAG_FILTER_NEAREST)) {
		f &= ~R300_TX_MIN_FILTER_NEAREST;
		f |= R300_TX_MIN_FILTER_LINEAR;
		return f;
	}

	/* Both are nearest */
	if (needs_fixing & 1) {
		f &= ~((7 - 1) << R300_TX_WRAP_S_SHIFT);
		f |= R300_TX_CLAMP_TO_EDGE << R300_TX_WRAP_S_SHIFT;
	}
	if (needs_fixing & 2) {
		f &= ~((7 - 1) << R300_TX_WRAP_T_SHIFT);
		f |= R300_TX_CLAMP_TO_EDGE << R300_TX_WRAP_T_SHIFT;
	}
	if (needs_fixing & 4) {
		f &= ~((7 - 1) << R300_TX_WRAP_Q_SHIFT);
		f |= R300_TX_CLAMP_TO_EDGE << R300_TX_WRAP_Q_SHIFT;
	}
	return f;
}

static void r300SetupTextures(GLcontext * ctx)
{
	int i, mtu;
	struct r300_tex_obj *t;
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	int hw_tmu = 0;
	int last_hw_tmu = -1;	/* -1 translates into no setup costs for fields */
	int tmu_mappings[R300_MAX_TEXTURE_UNITS] = { -1, };
	struct r300_fragment_program *fp = (struct r300_fragment_program *)
	    (char *)ctx->FragmentProgram._Current;

	R300_STATECHANGE(r300, txe);
	R300_STATECHANGE(r300, tex.filter);
	R300_STATECHANGE(r300, tex.filter_1);
	R300_STATECHANGE(r300, tex.size);
	R300_STATECHANGE(r300, tex.format);
	R300_STATECHANGE(r300, tex.pitch);
	R300_STATECHANGE(r300, tex.offset);
	R300_STATECHANGE(r300, tex.chroma_key);
	R300_STATECHANGE(r300, tex.border_color);

	r300->hw.txe.cmd[R300_TXE_ENABLE] = 0x0;

	mtu = r300->radeon.glCtx->Const.MaxTextureUnits;
	if (RADEON_DEBUG & DEBUG_STATE)
		fprintf(stderr, "mtu=%d\n", mtu);

	if (mtu > R300_MAX_TEXTURE_UNITS) {
		fprintf(stderr,
			"Aiiee ! mtu=%d is greater than R300_MAX_TEXTURE_UNITS=%d\n",
			mtu, R300_MAX_TEXTURE_UNITS);
		_mesa_exit(-1);
	}

	/* We cannot let disabled tmu offsets pass DRM */
	for (i = 0; i < mtu; i++) {
		if (ctx->Texture.Unit[i]._ReallyEnabled) {

#if 0				/* Enables old behaviour */
			hw_tmu = i;
#endif
			tmu_mappings[i] = hw_tmu;

			t = r300->state.texture.unit[i].texobj;
			/* XXX questionable fix for bug 9170: */
			if (!t)
				continue;

			if ((t->format & 0xffffff00) == 0xffffff00) {
				WARN_ONCE
				    ("unknown texture format (entry %x) encountered. Help me !\n",
				     t->format & 0xff);
			}

			if (RADEON_DEBUG & DEBUG_STATE)
				fprintf(stderr,
					"Activating texture unit %d\n", i);

			r300->hw.txe.cmd[R300_TXE_ENABLE] |= (1 << hw_tmu);

			r300->hw.tex.filter.cmd[R300_TEX_VALUE_0 +
						hw_tmu] =
			    gen_fixed_filter(t->filter) | (hw_tmu << 28);
			/* Currently disabled! */
			r300->hw.tex.filter_1.cmd[R300_TEX_VALUE_0 + hw_tmu] = 0x0;	//0x20501f80;
			r300->hw.tex.size.cmd[R300_TEX_VALUE_0 + hw_tmu] =
			    t->size;
			r300->hw.tex.format.cmd[R300_TEX_VALUE_0 +
						hw_tmu] = t->format;
			r300->hw.tex.pitch.cmd[R300_TEX_VALUE_0 + hw_tmu] =
			    t->pitch_reg;
			r300->hw.tex.offset.cmd[R300_TEX_VALUE_0 +
						hw_tmu] = t->offset;

			if (t->offset & R300_TXO_MACRO_TILE) {
				WARN_ONCE("macro tiling enabled!\n");
			}

			if (t->offset & R300_TXO_MICRO_TILE) {
				WARN_ONCE("micro tiling enabled!\n");
			}

			r300->hw.tex.chroma_key.cmd[R300_TEX_VALUE_0 +
						    hw_tmu] = 0x0;
			r300->hw.tex.border_color.cmd[R300_TEX_VALUE_0 +
						      hw_tmu] =
			    t->pp_border_color;

			last_hw_tmu = hw_tmu;

			hw_tmu++;
		}
	}

	r300->hw.tex.filter.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(R300_TX_FILTER_0, last_hw_tmu + 1);
	r300->hw.tex.filter_1.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(R300_TX_FILTER1_0, last_hw_tmu + 1);
	r300->hw.tex.size.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(R300_TX_SIZE_0, last_hw_tmu + 1);
	r300->hw.tex.format.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(R300_TX_FORMAT_0, last_hw_tmu + 1);
	r300->hw.tex.pitch.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(R300_TX_PITCH_0, last_hw_tmu + 1);
	r300->hw.tex.offset.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(R300_TX_OFFSET_0, last_hw_tmu + 1);
	r300->hw.tex.chroma_key.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(R300_TX_CHROMA_KEY_0, last_hw_tmu + 1);
	r300->hw.tex.border_color.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(R300_TX_BORDER_COLOR_0, last_hw_tmu + 1);

	if (!fp)		/* should only happenen once, just after context is created */
		return;

	R300_STATECHANGE(r300, fpt);

	for (i = 0; i < fp->tex.length; i++) {
		int unit;
		int opcode;
		unsigned long val;

		unit = fp->tex.inst[i] >> R300_FPITX_IMAGE_SHIFT;
		unit &= 15;

		val = fp->tex.inst[i];
		val &= ~R300_FPITX_IMAGE_MASK;

		opcode =
		    (val & R300_FPITX_OPCODE_MASK) >> R300_FPITX_OPCODE_SHIFT;
		if (opcode == R300_FPITX_OP_KIL) {
			r300->hw.fpt.cmd[R300_FPT_INSTR_0 + i] = val;
		} else {
			if (tmu_mappings[unit] >= 0) {
				val |=
				    tmu_mappings[unit] <<
				    R300_FPITX_IMAGE_SHIFT;
				r300->hw.fpt.cmd[R300_FPT_INSTR_0 + i] = val;
			} else {
				// We get here when the corresponding texture image is incomplete
				// (e.g. incomplete mipmaps etc.)
				r300->hw.fpt.cmd[R300_FPT_INSTR_0 + i] = val;
			}
		}
	}

	r300->hw.fpt.cmd[R300_FPT_CMD_0] =
	    cmdpacket0(R300_PFS_TEXI_0, fp->tex.length);

	if (RADEON_DEBUG & DEBUG_STATE)
		fprintf(stderr, "TX_ENABLE: %08x  last_hw_tmu=%d\n",
			r300->hw.txe.cmd[R300_TXE_ENABLE], last_hw_tmu);
}

union r300_outputs_written {
	GLuint vp_outputs;	/* hw_tcl_on */
	 DECLARE_RENDERINPUTS(index_bitset);	/* !hw_tcl_on */
};

#define R300_OUTPUTS_WRITTEN_TEST(ow, vp_result, tnl_attrib) \
	((hw_tcl_on) ? (ow).vp_outputs & (1 << (vp_result)) : \
	RENDERINPUTS_TEST( (ow.index_bitset), (tnl_attrib) ))

static void r300SetupRSUnit(GLcontext * ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	/* I'm still unsure if these are needed */
	GLuint interp_magic[8] = {
		0x00,
		R300_RS_INTERP_1_UNKNOWN,
		R300_RS_INTERP_2_UNKNOWN,
		R300_RS_INTERP_3_UNKNOWN,
		0x00,
		0x00,
		0x00,
		0x00
	};
	union r300_outputs_written OutputsWritten;
	GLuint InputsRead;
	int fp_reg, high_rr;
	int in_texcoords, col_interp_nr;
	int i;

	if (hw_tcl_on)
		OutputsWritten.vp_outputs =
		    CURRENT_VERTEX_SHADER(ctx)->key.OutputsWritten;
	else
		RENDERINPUTS_COPY(OutputsWritten.index_bitset,
				  r300->state.render_inputs_bitset);

	if (ctx->FragmentProgram._Current)
		InputsRead = ctx->FragmentProgram._Current->Base.InputsRead;
	else {
		fprintf(stderr, "No ctx->FragmentProgram._Current!!\n");
		return;		/* This should only ever happen once.. */
	}

	R300_STATECHANGE(r300, ri);
	R300_STATECHANGE(r300, rc);
	R300_STATECHANGE(r300, rr);

	fp_reg = in_texcoords = col_interp_nr = high_rr = 0;

	r300->hw.rr.cmd[R300_RR_ROUTE_1] = 0;

	if (InputsRead & FRAG_BIT_WPOS) {
		for (i = 0; i < ctx->Const.MaxTextureUnits; i++)
			if (!(InputsRead & (FRAG_BIT_TEX0 << i)))
				break;

		if (i == ctx->Const.MaxTextureUnits) {
			fprintf(stderr, "\tno free texcoord found...\n");
			_mesa_exit(-1);
		}

		InputsRead |= (FRAG_BIT_TEX0 << i);
		InputsRead &= ~FRAG_BIT_WPOS;
	}

	for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
		r300->hw.ri.cmd[R300_RI_INTERP_0 + i] = 0
		    | R300_RS_INTERP_USED
		    | (in_texcoords << R300_RS_INTERP_SRC_SHIFT)
		    | interp_magic[i];

		r300->hw.rr.cmd[R300_RR_ROUTE_0 + fp_reg] = 0;
		if (InputsRead & (FRAG_BIT_TEX0 << i)) {
			//assert(r300->state.texture.tc_count != 0);
			r300->hw.rr.cmd[R300_RR_ROUTE_0 + fp_reg] |= R300_RS_ROUTE_ENABLE | i	/* source INTERP */
			    | (fp_reg << R300_RS_ROUTE_DEST_SHIFT);
			high_rr = fp_reg;

			if (!R300_OUTPUTS_WRITTEN_TEST
			    (OutputsWritten, VERT_RESULT_TEX0 + i,
			     _TNL_ATTRIB_TEX(i))) {
				/* Passing invalid data here can lock the GPU. */
				WARN_ONCE
				    ("fragprog wants coords for tex%d, vp doesn't provide them!\n",
				     i);
				//_mesa_print_program(&CURRENT_VERTEX_SHADER(ctx)->Base);
				//_mesa_exit(-1);
			}
			InputsRead &= ~(FRAG_BIT_TEX0 << i);
			fp_reg++;
		}
		/* Need to count all coords enabled at vof */
		if (R300_OUTPUTS_WRITTEN_TEST
		    (OutputsWritten, VERT_RESULT_TEX0 + i, _TNL_ATTRIB_TEX(i)))
			in_texcoords++;
	}

	if (InputsRead & FRAG_BIT_COL0) {
		if (!R300_OUTPUTS_WRITTEN_TEST
		    (OutputsWritten, VERT_RESULT_COL0, _TNL_ATTRIB_COLOR0)) {
			WARN_ONCE
			    ("fragprog wants col0, vp doesn't provide it\n");
			goto out;	/* FIXME */
			//_mesa_print_program(&CURRENT_VERTEX_SHADER(ctx)->Base);
			//_mesa_exit(-1);
		}

		r300->hw.rr.cmd[R300_RR_ROUTE_0] |= 0
		    | R300_RS_ROUTE_0_COLOR
		    | (fp_reg++ << R300_RS_ROUTE_0_COLOR_DEST_SHIFT);
		InputsRead &= ~FRAG_BIT_COL0;
		col_interp_nr++;
	}
      out:

	if (InputsRead & FRAG_BIT_COL1) {
		if (!R300_OUTPUTS_WRITTEN_TEST
		    (OutputsWritten, VERT_RESULT_COL1, _TNL_ATTRIB_COLOR1)) {
			WARN_ONCE
			    ("fragprog wants col1, vp doesn't provide it\n");
			//_mesa_exit(-1);
		}

		r300->hw.rr.cmd[R300_RR_ROUTE_1] |=
		    R300_RS_ROUTE_1_UNKNOWN11 | R300_RS_ROUTE_1_COLOR1 |
		    (fp_reg++ << R300_RS_ROUTE_1_COLOR1_DEST_SHIFT);
		InputsRead &= ~FRAG_BIT_COL1;
		if (high_rr < 1)
			high_rr = 1;
		col_interp_nr++;
	}

	/* Need at least one. This might still lock as the values are undefined... */
	if (in_texcoords == 0 && col_interp_nr == 0) {
		r300->hw.rr.cmd[R300_RR_ROUTE_0] |= 0
		    | R300_RS_ROUTE_0_COLOR
		    | (fp_reg++ << R300_RS_ROUTE_0_COLOR_DEST_SHIFT);
		col_interp_nr++;
	}

	r300->hw.rc.cmd[1] = 0 | (in_texcoords << R300_RS_CNTL_TC_CNT_SHIFT)
	    | (col_interp_nr << R300_RS_CNTL_CI_CNT_SHIFT)
	    | R300_RS_CNTL_0_UNKNOWN_18;

	assert(high_rr >= 0);
	r300->hw.rr.cmd[R300_RR_CMD_0] =
	    cmdpacket0(R300_RS_ROUTE_0, high_rr + 1);
	r300->hw.rc.cmd[2] = 0xC0 | high_rr;

	if (InputsRead)
		WARN_ONCE("Don't know how to satisfy InputsRead=0x%08x\n",
			  InputsRead);
}

#define vpucount(ptr) (((drm_r300_cmd_header_t*)(ptr))->vpu.count)

#define bump_vpu_count(ptr, new_count)   do{\
	drm_r300_cmd_header_t* _p=((drm_r300_cmd_header_t*)(ptr));\
	int _nc=(new_count)/4; \
	assert(_nc < 256); \
	if(_nc>_p->vpu.count)_p->vpu.count=_nc;\
	}while(0)

void static inline setup_vertex_shader_fragment(r300ContextPtr r300, int dest, struct
						r300_vertex_shader_fragment
						*vsf)
{
	int i;

	if (vsf->length == 0)
		return;

	if (vsf->length & 0x3) {
		fprintf(stderr,
			"VERTEX_SHADER_FRAGMENT must have length divisible by 4\n");
		_mesa_exit(-1);
	}

	switch ((dest >> 8) & 0xf) {
	case 0:
		R300_STATECHANGE(r300, vpi);
		for (i = 0; i < vsf->length; i++)
			r300->hw.vpi.cmd[R300_VPI_INSTR_0 + i +
					 4 * (dest & 0xff)] = (vsf->body.d[i]);
		bump_vpu_count(r300->hw.vpi.cmd,
			       vsf->length + 4 * (dest & 0xff));
		break;

	case 2:
		R300_STATECHANGE(r300, vpp);
		for (i = 0; i < vsf->length; i++)
			r300->hw.vpp.cmd[R300_VPP_PARAM_0 + i +
					 4 * (dest & 0xff)] = (vsf->body.d[i]);
		bump_vpu_count(r300->hw.vpp.cmd,
			       vsf->length + 4 * (dest & 0xff));
		break;
	case 4:
		R300_STATECHANGE(r300, vps);
		for (i = 0; i < vsf->length; i++)
			r300->hw.vps.cmd[1 + i + 4 * (dest & 0xff)] =
			    (vsf->body.d[i]);
		bump_vpu_count(r300->hw.vps.cmd,
			       vsf->length + 4 * (dest & 0xff));
		break;
	default:
		fprintf(stderr,
			"%s:%s don't know how to handle dest %04x\n",
			__FILE__, __FUNCTION__, dest);
		_mesa_exit(-1);
	}
}

/* just a skeleton for now.. */

/* Generate a vertex shader that simply transforms vertex and texture coordinates,
   while leaving colors intact. Nothing fancy (like lights)

   If implementing lights make a copy first, so it is easy to switch between the two versions */
static void r300GenerateSimpleVertexShader(r300ContextPtr r300)
{
	int i;
	GLuint o_reg = 0;

	/* Allocate parameters */
	r300->state.vap_param.transform_offset = 0x0;	/* transform matrix */
	r300->state.vertex_shader.param_offset = 0x0;
	r300->state.vertex_shader.param_count = 0x4;	/* 4 vector values - 4x4 matrix */

	r300->state.vertex_shader.program_start = 0x0;
	r300->state.vertex_shader.unknown_ptr1 = 0x4;	/* magic value ? */
	r300->state.vertex_shader.program_end = 0x0;

	r300->state.vertex_shader.unknown_ptr2 = 0x0;	/* magic value */
	r300->state.vertex_shader.unknown_ptr3 = 0x4;	/* magic value */

	r300->state.vertex_shader.unknown1.length = 0;
	r300->state.vertex_shader.unknown2.length = 0;

#define WRITE_OP(oper,source1,source2,source3)	{\
	r300->state.vertex_shader.program.body.i[r300->state.vertex_shader.program_end].op=(oper); \
	r300->state.vertex_shader.program.body.i[r300->state.vertex_shader.program_end].src[0]=(source1); \
	r300->state.vertex_shader.program.body.i[r300->state.vertex_shader.program_end].src[1]=(source2); \
	r300->state.vertex_shader.program.body.i[r300->state.vertex_shader.program_end].src[2]=(source3); \
	r300->state.vertex_shader.program_end++; \
	}

	for (i = VERT_ATTRIB_POS; i < VERT_ATTRIB_MAX; i++)
		if (r300->state.sw_tcl_inputs[i] != -1) {
			WRITE_OP(EASY_VSF_OP(MUL, o_reg++, ALL, RESULT),
				 VSF_REG(r300->state.sw_tcl_inputs[i]),
				 VSF_ATTR_UNITY(r300->state.
						sw_tcl_inputs[i]),
				 VSF_UNITY(r300->state.sw_tcl_inputs[i])
			    )

		}

	r300->state.vertex_shader.program_end--;	/* r300 wants program length to be one more - no idea why */
	r300->state.vertex_shader.program.length =
	    (r300->state.vertex_shader.program_end + 1) * 4;

	r300->state.vertex_shader.unknown_ptr1 = r300->state.vertex_shader.program_end;	/* magic value ? */
	r300->state.vertex_shader.unknown_ptr2 = r300->state.vertex_shader.program_end;	/* magic value ? */
	r300->state.vertex_shader.unknown_ptr3 = r300->state.vertex_shader.program_end;	/* magic value ? */

}

static void r300SetupVertexProgram(r300ContextPtr rmesa)
{
	GLcontext *ctx = rmesa->radeon.glCtx;
	int inst_count;
	int param_count;
	struct r300_vertex_program *prog =
	    (struct r300_vertex_program *)CURRENT_VERTEX_SHADER(ctx);

	((drm_r300_cmd_header_t *) rmesa->hw.vpp.cmd)->vpu.count = 0;
	R300_STATECHANGE(rmesa, vpp);
	param_count =
	    r300VertexProgUpdateParams(ctx, (struct r300_vertex_program_cont *)
				       ctx->VertexProgram._Current /*prog */ ,
				       (float *)&rmesa->hw.vpp.
				       cmd[R300_VPP_PARAM_0]);
	bump_vpu_count(rmesa->hw.vpp.cmd, param_count);
	param_count /= 4;

	/* Reset state, in case we don't use something */
	((drm_r300_cmd_header_t *) rmesa->hw.vpi.cmd)->vpu.count = 0;
	((drm_r300_cmd_header_t *) rmesa->hw.vps.cmd)->vpu.count = 0;

	setup_vertex_shader_fragment(rmesa, VSF_DEST_PROGRAM, &(prog->program));

#if 0
	setup_vertex_shader_fragment(rmesa, VSF_DEST_UNKNOWN1,
				     &(rmesa->state.vertex_shader.unknown1));
	setup_vertex_shader_fragment(rmesa, VSF_DEST_UNKNOWN2,
				     &(rmesa->state.vertex_shader.unknown2));
#endif

	inst_count = prog->program.length / 4 - 1;

	R300_STATECHANGE(rmesa, pvs);
	rmesa->hw.pvs.cmd[R300_PVS_CNTL_1] =
	    (0 << R300_PVS_CNTL_1_PROGRAM_START_SHIFT)
	    | (inst_count /*pos_end */  << R300_PVS_CNTL_1_POS_END_SHIFT)
	    | (inst_count << R300_PVS_CNTL_1_PROGRAM_END_SHIFT);
	rmesa->hw.pvs.cmd[R300_PVS_CNTL_2] =
	    (0 << R300_PVS_CNTL_2_PARAM_OFFSET_SHIFT)
	    | (param_count << R300_PVS_CNTL_2_PARAM_COUNT_SHIFT);
	rmesa->hw.pvs.cmd[R300_PVS_CNTL_3] =
	    (0 /*rmesa->state.vertex_shader.unknown_ptr2 */  <<
	     R300_PVS_CNTL_3_PROGRAM_UNKNOWN_SHIFT)
	    | (inst_count /*rmesa->state.vertex_shader.unknown_ptr3 */  <<
	       0);

	/* This is done for vertex shader fragments, but also needs to be done for vap_pvs,
	   so I leave it as a reminder */
#if 0
	reg_start(R300_VAP_PVS_WAITIDLE, 0);
	e32(0x00000000);
#endif
}

static void r300SetupVertexShader(r300ContextPtr rmesa)
{
	GLcontext *ctx = rmesa->radeon.glCtx;

	/* Reset state, in case we don't use something */
	((drm_r300_cmd_header_t *) rmesa->hw.vpp.cmd)->vpu.count = 0;
	((drm_r300_cmd_header_t *) rmesa->hw.vpi.cmd)->vpu.count = 0;
	((drm_r300_cmd_header_t *) rmesa->hw.vps.cmd)->vpu.count = 0;

	/* Not sure why this doesnt work...
	   0x400 area might have something to do with pixel shaders as it appears right after pfs programming.
	   0x406 is set to { 0.0, 0.0, 1.0, 0.0 } most of the time but should change with smooth points and in other rare cases. */
	//setup_vertex_shader_fragment(rmesa, 0x406, &unk4);
	if (hw_tcl_on
	    && ((struct r300_vertex_program *)CURRENT_VERTEX_SHADER(ctx))->
	    translated) {
		r300SetupVertexProgram(rmesa);
		return;
	}

	/* This needs to be replaced by vertex shader generation code */
	r300GenerateSimpleVertexShader(rmesa);

	setup_vertex_shader_fragment(rmesa, VSF_DEST_PROGRAM,
				     &(rmesa->state.vertex_shader.program));

#if 0
	setup_vertex_shader_fragment(rmesa, VSF_DEST_UNKNOWN1,
				     &(rmesa->state.vertex_shader.unknown1));
	setup_vertex_shader_fragment(rmesa, VSF_DEST_UNKNOWN2,
				     &(rmesa->state.vertex_shader.unknown2));
#endif

	R300_STATECHANGE(rmesa, pvs);
	rmesa->hw.pvs.cmd[R300_PVS_CNTL_1] =
	    (rmesa->state.vertex_shader.
	     program_start << R300_PVS_CNTL_1_PROGRAM_START_SHIFT)
	    | (rmesa->state.vertex_shader.
	       unknown_ptr1 << R300_PVS_CNTL_1_POS_END_SHIFT)
	    | (rmesa->state.vertex_shader.
	       program_end << R300_PVS_CNTL_1_PROGRAM_END_SHIFT);
	rmesa->hw.pvs.cmd[R300_PVS_CNTL_2] =
	    (rmesa->state.vertex_shader.
	     param_offset << R300_PVS_CNTL_2_PARAM_OFFSET_SHIFT)
	    | (rmesa->state.vertex_shader.
	       param_count << R300_PVS_CNTL_2_PARAM_COUNT_SHIFT);
	rmesa->hw.pvs.cmd[R300_PVS_CNTL_3] =
	    (rmesa->state.vertex_shader.
	     unknown_ptr2 << R300_PVS_CNTL_3_PROGRAM_UNKNOWN_SHIFT)
	    | (rmesa->state.vertex_shader.unknown_ptr3 << 0);

	/* This is done for vertex shader fragments, but also needs to be done for vap_pvs,
	   so I leave it as a reminder */
#if 0
	reg_start(R300_VAP_PVS_WAITIDLE, 0);
	e32(0x00000000);
#endif
}

/**
 * Completely recalculates hardware state based on the Mesa state.
 */
static void r300ResetHwState(r300ContextPtr r300)
{
	GLcontext *ctx = r300->radeon.glCtx;
	int has_tcl = 1;

	if (!(r300->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_TCL))
		has_tcl = 0;

	if (RADEON_DEBUG & DEBUG_STATE)
		fprintf(stderr, "%s\n", __FUNCTION__);

	/* This is a place to initialize registers which
	   have bitfields accessed by different functions
	   and not all bits are used */

	/* go and compute register values from GL state */

	r300UpdateWindow(ctx);

	r300ColorMask(ctx,
		      ctx->Color.ColorMask[RCOMP],
		      ctx->Color.ColorMask[GCOMP],
		      ctx->Color.ColorMask[BCOMP], ctx->Color.ColorMask[ACOMP]);

	r300Enable(ctx, GL_DEPTH_TEST, ctx->Depth.Test);
	r300DepthMask(ctx, ctx->Depth.Mask);
	r300DepthFunc(ctx, ctx->Depth.Func);

	/* stencil */
	r300Enable(ctx, GL_STENCIL_TEST, ctx->Stencil.Enabled);
	r300StencilMaskSeparate(ctx, 0, ctx->Stencil.WriteMask[0]);
	r300StencilFuncSeparate(ctx, 0, ctx->Stencil.Function[0],
				ctx->Stencil.Ref[0], ctx->Stencil.ValueMask[0]);
	r300StencilOpSeparate(ctx, 0, ctx->Stencil.FailFunc[0],
			      ctx->Stencil.ZFailFunc[0],
			      ctx->Stencil.ZPassFunc[0]);

	r300UpdateCulling(ctx);

	r300UpdateTextureState(ctx);

	r300SetBlendState(ctx);

	r300AlphaFunc(ctx, ctx->Color.AlphaFunc, ctx->Color.AlphaRef);
	r300Enable(ctx, GL_ALPHA_TEST, ctx->Color.AlphaEnabled);

	/* Initialize magic registers
	   TODO : learn what they really do, or get rid of
	   those we don't have to touch */
	if (!has_tcl)
		r300->hw.vap_cntl.cmd[1] = 0x0014045a;
	else
		r300->hw.vap_cntl.cmd[1] = 0x0030045A;	//0x0030065a /* Dangerous */
	r300->hw.vte.cmd[1] = R300_VPORT_X_SCALE_ENA
	    | R300_VPORT_X_OFFSET_ENA
	    | R300_VPORT_Y_SCALE_ENA
	    | R300_VPORT_Y_OFFSET_ENA
	    | R300_VPORT_Z_SCALE_ENA
	    | R300_VPORT_Z_OFFSET_ENA | R300_VTX_W0_FMT;
	r300->hw.vte.cmd[2] = 0x00000008;

	r300->hw.unk2134.cmd[1] = 0x00FFFFFF;
	r300->hw.unk2134.cmd[2] = 0x00000000;
	if (_mesa_little_endian())
		r300->hw.vap_cntl_status.cmd[1] = R300_VC_NO_SWAP;
	else
		r300->hw.vap_cntl_status.cmd[1] = R300_VC_32BIT_SWAP;

	/* disable VAP/TCL on non-TCL capable chips */
	if (!has_tcl)
		r300->hw.vap_cntl_status.cmd[1] |= R300_VAP_TCL_BYPASS;

	r300->hw.unk21DC.cmd[1] = 0xAAAAAAAA;

	r300->hw.unk221C.cmd[1] = R300_221C_NORMAL;

	r300->hw.unk2220.cmd[1] = r300PackFloat32(1.0);
	r300->hw.unk2220.cmd[2] = r300PackFloat32(1.0);
	r300->hw.unk2220.cmd[3] = r300PackFloat32(1.0);
	r300->hw.unk2220.cmd[4] = r300PackFloat32(1.0);

	/* what about other chips than r300 or rv350??? */
	if (r300->radeon.radeonScreen->chip_family == CHIP_FAMILY_R300)
		r300->hw.unk2288.cmd[1] = R300_2288_R300;
	else
		r300->hw.unk2288.cmd[1] = R300_2288_RV350;

	r300->hw.gb_enable.cmd[1] = R300_GB_POINT_STUFF_ENABLE
	    | R300_GB_LINE_STUFF_ENABLE
	    | R300_GB_TRIANGLE_STUFF_ENABLE /*| R300_GB_UNK31 */ ;

	r300->hw.gb_misc.cmd[R300_GB_MISC_MSPOS_0] = 0x66666666;
	r300->hw.gb_misc.cmd[R300_GB_MISC_MSPOS_1] = 0x06666666;
	if ((r300->radeon.radeonScreen->chip_family == CHIP_FAMILY_R300) ||
	    (r300->radeon.radeonScreen->chip_family == CHIP_FAMILY_R350))
		r300->hw.gb_misc.cmd[R300_GB_MISC_TILE_CONFIG] =
		    R300_GB_TILE_ENABLE | R300_GB_TILE_PIPE_COUNT_R300 |
		    R300_GB_TILE_SIZE_16;
	else if (r300->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV410)
		r300->hw.gb_misc.cmd[R300_GB_MISC_TILE_CONFIG] =
		    R300_GB_TILE_ENABLE | R300_GB_TILE_PIPE_COUNT_RV410 |
		    R300_GB_TILE_SIZE_16;
	else if (r300->radeon.radeonScreen->chip_family == CHIP_FAMILY_R420)
		r300->hw.gb_misc.cmd[R300_GB_MISC_TILE_CONFIG] =
		    R300_GB_TILE_ENABLE | R300_GB_TILE_PIPE_COUNT_R420 |
		    R300_GB_TILE_SIZE_16;
	else
		r300->hw.gb_misc.cmd[R300_GB_MISC_TILE_CONFIG] =
		    R300_GB_TILE_ENABLE | R300_GB_TILE_PIPE_COUNT_RV300 |
		    R300_GB_TILE_SIZE_16;
	/* set to 0 when fog is disabled? */
	r300->hw.gb_misc.cmd[R300_GB_MISC_SELECT] = R300_GB_FOG_SELECT_1_1_W;
	r300->hw.gb_misc.cmd[R300_GB_MISC_AA_CONFIG] = R300_AA_DISABLE;	/* No antialiasing */

	r300->hw.unk4200.cmd[1] = r300PackFloat32(0.0);
	r300->hw.unk4200.cmd[2] = r300PackFloat32(0.0);
	r300->hw.unk4200.cmd[3] = r300PackFloat32(1.0);
	r300->hw.unk4200.cmd[4] = r300PackFloat32(1.0);

	r300->hw.unk4214.cmd[1] = 0x00050005;

	r300PointSize(ctx, 0.0);

	r300->hw.unk4230.cmd[1] = 0x18000006;
	r300->hw.unk4230.cmd[2] = 0x00020006;
	r300->hw.unk4230.cmd[3] = r300PackFloat32(1.0 / 192.0);

	r300LineWidth(ctx, 0.0);

	r300->hw.unk4260.cmd[1] = 0;
	r300->hw.unk4260.cmd[2] = r300PackFloat32(0.0);
	r300->hw.unk4260.cmd[3] = r300PackFloat32(1.0);

	r300->hw.shade.cmd[1] = 0x00000002;
	r300ShadeModel(ctx, ctx->Light.ShadeModel);
	r300->hw.shade.cmd[3] = 0x00000000;
	r300->hw.shade.cmd[4] = 0x00000000;

	r300PolygonMode(ctx, GL_FRONT, ctx->Polygon.FrontMode);
	r300PolygonMode(ctx, GL_BACK, ctx->Polygon.BackMode);
	r300->hw.polygon_mode.cmd[2] = 0x00000001;
	r300->hw.polygon_mode.cmd[3] = 0x00000000;
	r300->hw.zbias_cntl.cmd[1] = 0x00000000;

	r300PolygonOffset(ctx, ctx->Polygon.OffsetFactor,
			  ctx->Polygon.OffsetUnits);
	r300Enable(ctx, GL_POLYGON_OFFSET_FILL, ctx->Polygon.OffsetFill);

	r300->hw.unk42C0.cmd[1] = 0x4B7FFFFF;
	r300->hw.unk42C0.cmd[2] = 0x00000000;

	r300->hw.unk43A4.cmd[1] = 0x0000001C;
	r300->hw.unk43A4.cmd[2] = 0x2DA49525;

	r300->hw.unk43E8.cmd[1] = 0x00FFFFFF;

	r300->hw.unk46A4.cmd[1] = 0x00001B01;
	r300->hw.unk46A4.cmd[2] = 0x00001B0F;
	r300->hw.unk46A4.cmd[3] = 0x00001B0F;
	r300->hw.unk46A4.cmd[4] = 0x00001B0F;
	r300->hw.unk46A4.cmd[5] = 0x00000001;

	r300Enable(ctx, GL_FOG, ctx->Fog.Enabled);
	ctx->Driver.Fogfv(ctx, GL_FOG_MODE, NULL);
	ctx->Driver.Fogfv(ctx, GL_FOG_DENSITY, &ctx->Fog.Density);
	ctx->Driver.Fogfv(ctx, GL_FOG_START, &ctx->Fog.Start);
	ctx->Driver.Fogfv(ctx, GL_FOG_END, &ctx->Fog.End);
	ctx->Driver.Fogfv(ctx, GL_FOG_COLOR, ctx->Fog.Color);
	ctx->Driver.Fogfv(ctx, GL_FOG_COORDINATE_SOURCE_EXT, NULL);

	r300->hw.at.cmd[R300_AT_UNKNOWN] = 0;
	r300->hw.unk4BD8.cmd[1] = 0;

	r300->hw.unk4E00.cmd[1] = 0;

	r300BlendColor(ctx, ctx->Color.BlendColor);
	r300->hw.blend_color.cmd[2] = 0;
	r300->hw.blend_color.cmd[3] = 0;

	/* Again, r300ClearBuffer uses this */
	r300->hw.cb.cmd[R300_CB_OFFSET] =
	    r300->radeon.state.color.drawOffset +
	    r300->radeon.radeonScreen->fbLocation;
	r300->hw.cb.cmd[R300_CB_PITCH] = r300->radeon.state.color.drawPitch;

	if (r300->radeon.radeonScreen->cpp == 4)
		r300->hw.cb.cmd[R300_CB_PITCH] |= R300_COLOR_FORMAT_ARGB8888;
	else
		r300->hw.cb.cmd[R300_CB_PITCH] |= R300_COLOR_FORMAT_RGB565;

	if (r300->radeon.sarea->tiling_enabled)
		r300->hw.cb.cmd[R300_CB_PITCH] |= R300_COLOR_TILE_ENABLE;

	r300->hw.unk4E50.cmd[1] = 0;
	r300->hw.unk4E50.cmd[2] = 0;
	r300->hw.unk4E50.cmd[3] = 0;
	r300->hw.unk4E50.cmd[4] = 0;
	r300->hw.unk4E50.cmd[5] = 0;
	r300->hw.unk4E50.cmd[6] = 0;
	r300->hw.unk4E50.cmd[7] = 0;
	r300->hw.unk4E50.cmd[8] = 0;
	r300->hw.unk4E50.cmd[9] = 0;

	r300->hw.unk4E88.cmd[1] = 0;

	r300->hw.unk4EA0.cmd[1] = 0x00000000;
	r300->hw.unk4EA0.cmd[2] = 0xffffffff;

	switch (ctx->Visual.depthBits) {
	case 16:
		r300->hw.zstencil_format.cmd[1] = R300_DEPTH_FORMAT_16BIT_INT_Z;
		break;
	case 24:
		r300->hw.zstencil_format.cmd[1] = R300_DEPTH_FORMAT_24BIT_INT_Z;
		break;
	default:
		fprintf(stderr, "Error: Unsupported depth %d... exiting\n",
			ctx->Visual.depthBits);
		_mesa_exit(-1);

	}
	/* z compress? */
	//r300->hw.zstencil_format.cmd[1] |= R300_DEPTH_FORMAT_UNK32;

	r300->hw.zstencil_format.cmd[3] = 0x00000003;
	r300->hw.zstencil_format.cmd[4] = 0x00000000;

	r300->hw.zb.cmd[R300_ZB_OFFSET] =
	    r300->radeon.radeonScreen->depthOffset +
	    r300->radeon.radeonScreen->fbLocation;
	r300->hw.zb.cmd[R300_ZB_PITCH] = r300->radeon.radeonScreen->depthPitch;

	if (r300->radeon.sarea->tiling_enabled) {
		/* Turn off when clearing buffers ? */
		r300->hw.zb.cmd[R300_ZB_PITCH] |= R300_DEPTH_TILE_ENABLE;

		if (ctx->Visual.depthBits == 24)
			r300->hw.zb.cmd[R300_ZB_PITCH] |=
			    R300_DEPTH_MICROTILE_ENABLE;
	}

	r300->hw.unk4F28.cmd[1] = 0;

	r300->hw.unk4F30.cmd[1] = 0;
	r300->hw.unk4F30.cmd[2] = 0;

	r300->hw.unk4F44.cmd[1] = 0;

	r300->hw.unk4F54.cmd[1] = 0;

	if (has_tcl) {
		r300->hw.vps.cmd[R300_VPS_ZERO_0] = 0;
		r300->hw.vps.cmd[R300_VPS_ZERO_1] = 0;
		r300->hw.vps.cmd[R300_VPS_POINTSIZE] = r300PackFloat32(1.0);
		r300->hw.vps.cmd[R300_VPS_ZERO_3] = 0;
	}
//END: TODO
	r300->hw.all_dirty = GL_TRUE;
}


extern void _tnl_UpdateFixedFunctionProgram(GLcontext * ctx);

extern int future_hw_tcl_on;
void r300UpdateShaders(r300ContextPtr rmesa)
{
	GLcontext *ctx;
	struct r300_vertex_program *vp;
	int i;

	ctx = rmesa->radeon.glCtx;

	if (rmesa->NewGLState && hw_tcl_on) {
		rmesa->NewGLState = 0;

		for (i = _TNL_FIRST_MAT; i <= _TNL_LAST_MAT; i++) {
			rmesa->temp_attrib[i] =
			    TNL_CONTEXT(ctx)->vb.AttribPtr[i];
			TNL_CONTEXT(ctx)->vb.AttribPtr[i] =
			    &rmesa->dummy_attrib[i];
		}

		_tnl_UpdateFixedFunctionProgram(ctx);

		for (i = _TNL_FIRST_MAT; i <= _TNL_LAST_MAT; i++) {
			TNL_CONTEXT(ctx)->vb.AttribPtr[i] =
			    rmesa->temp_attrib[i];
		}

		r300SelectVertexShader(rmesa);
		vp = (struct r300_vertex_program *)
		    CURRENT_VERTEX_SHADER(ctx);
		/*if (vp->translated == GL_FALSE)
		   r300TranslateVertexShader(vp); */
		if (vp->translated == GL_FALSE) {
			fprintf(stderr, "Failing back to sw-tcl\n");
			hw_tcl_on = future_hw_tcl_on = 0;
			r300ResetHwState(rmesa);

			return;
		}
		r300UpdateStateParameters(ctx, _NEW_PROGRAM);
	}

}

static void r300SetupPixelShader(r300ContextPtr rmesa)
{
	GLcontext *ctx = rmesa->radeon.glCtx;
	struct r300_fragment_program *fp = (struct r300_fragment_program *)
	    (char *)ctx->FragmentProgram._Current;
	int i, k;

	if (!fp)		/* should only happenen once, just after context is created */
		return;

	r300TranslateFragmentShader(rmesa, fp);
	if (!fp->translated) {
		fprintf(stderr, "%s: No valid fragment shader, exiting\n",
			__FUNCTION__);
		return;
	}
#define OUTPUT_FIELD(st, reg, field)  \
		R300_STATECHANGE(rmesa, st); \
		for(i=0;i<=fp->alu_end;i++) \
			rmesa->hw.st.cmd[R300_FPI_INSTR_0+i]=fp->alu.inst[i].field;\
		rmesa->hw.st.cmd[R300_FPI_CMD_0]=cmdpacket0(reg, fp->alu_end+1);

	OUTPUT_FIELD(fpi[0], R300_PFS_INSTR0_0, inst0);
	OUTPUT_FIELD(fpi[1], R300_PFS_INSTR1_0, inst1);
	OUTPUT_FIELD(fpi[2], R300_PFS_INSTR2_0, inst2);
	OUTPUT_FIELD(fpi[3], R300_PFS_INSTR3_0, inst3);
#undef OUTPUT_FIELD

	R300_STATECHANGE(rmesa, fp);
	/* I just want to say, the way these nodes are stored.. weird.. */
	for (i = 0, k = (4 - (fp->cur_node + 1)); i < 4; i++, k++) {
		if (i < (fp->cur_node + 1)) {
			rmesa->hw.fp.cmd[R300_FP_NODE0 + k] =
			    (fp->node[i].
			     alu_offset << R300_PFS_NODE_ALU_OFFSET_SHIFT)
			    | (fp->node[i].
			       alu_end << R300_PFS_NODE_ALU_END_SHIFT)
			    | (fp->node[i].
			       tex_offset << R300_PFS_NODE_TEX_OFFSET_SHIFT)
			    | (fp->node[i].
			       tex_end << R300_PFS_NODE_TEX_END_SHIFT)
			    | fp->node[i].flags;	/*  ( (k==3) ? R300_PFS_NODE_LAST_NODE : 0); */
		} else {
			rmesa->hw.fp.cmd[R300_FP_NODE0 + (3 - i)] = 0;
		}
	}

	/*  PFS_CNTL_0 */
	rmesa->hw.fp.cmd[R300_FP_CNTL0] =
	    fp->cur_node | (fp->first_node_has_tex << 3);
	/* PFS_CNTL_1 */
	rmesa->hw.fp.cmd[R300_FP_CNTL1] = fp->max_temp_idx;
	/* PFS_CNTL_2 */
	rmesa->hw.fp.cmd[R300_FP_CNTL2] =
	    (fp->alu_offset << R300_PFS_CNTL_ALU_OFFSET_SHIFT)
	    | (fp->alu_end << R300_PFS_CNTL_ALU_END_SHIFT)
	    | (fp->tex_offset << R300_PFS_CNTL_TEX_OFFSET_SHIFT)
	    | (fp->tex_end << R300_PFS_CNTL_TEX_END_SHIFT);

	R300_STATECHANGE(rmesa, fpp);
	for (i = 0; i < fp->const_nr; i++) {
		rmesa->hw.fpp.cmd[R300_FPP_PARAM_0 + 4 * i + 0] =
		    r300PackFloat24(fp->constant[i][0]);
		rmesa->hw.fpp.cmd[R300_FPP_PARAM_0 + 4 * i + 1] =
		    r300PackFloat24(fp->constant[i][1]);
		rmesa->hw.fpp.cmd[R300_FPP_PARAM_0 + 4 * i + 2] =
		    r300PackFloat24(fp->constant[i][2]);
		rmesa->hw.fpp.cmd[R300_FPP_PARAM_0 + 4 * i + 3] =
		    r300PackFloat24(fp->constant[i][3]);
	}
	rmesa->hw.fpp.cmd[R300_FPP_CMD_0] =
	    cmdpacket0(R300_PFS_PARAM_0_X, fp->const_nr * 4);
}

void r300UpdateShaderStates(r300ContextPtr rmesa)
{
	GLcontext *ctx;
	ctx = rmesa->radeon.glCtx;

	r300UpdateTextureState(ctx);

	r300SetupPixelShader(rmesa);
	r300SetupTextures(ctx);

	if ((rmesa->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_TCL))
		r300SetupVertexShader(rmesa);
	r300SetupRSUnit(ctx);
}

/**
 * Called by Mesa after an internal state update.
 */
static void r300InvalidateState(GLcontext * ctx, GLuint new_state)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	_swrast_InvalidateState(ctx, new_state);
	_swsetup_InvalidateState(ctx, new_state);
	_vbo_InvalidateState(ctx, new_state);
	_tnl_InvalidateState(ctx, new_state);
	_ae_invalidate_state(ctx, new_state);

	if (new_state & (_NEW_BUFFERS | _NEW_COLOR | _NEW_PIXEL)) {
		r300UpdateDrawBuffer(ctx);
	}

	r300UpdateStateParameters(ctx, new_state);

	r300->NewGLState |= new_state;
}

/**
 * Calculate initial hardware state and register state functions.
 * Assumes that the command buffer and state atoms have been
 * initialized already.
 */
void r300InitState(r300ContextPtr r300)
{
	GLcontext *ctx = r300->radeon.glCtx;
	GLuint depth_fmt;

	radeonInitState(&r300->radeon);

	switch (ctx->Visual.depthBits) {
	case 16:
		r300->state.depth.scale = 1.0 / (GLfloat) 0xffff;
		depth_fmt = R300_DEPTH_FORMAT_16BIT_INT_Z;
		r300->state.stencil.clear = 0x00000000;
		break;
	case 24:
		r300->state.depth.scale = 1.0 / (GLfloat) 0xffffff;
		depth_fmt = R300_DEPTH_FORMAT_24BIT_INT_Z;
		r300->state.stencil.clear = 0x00ff0000;
		break;
	default:
		fprintf(stderr, "Error: Unsupported depth %d... exiting\n",
			ctx->Visual.depthBits);
		_mesa_exit(-1);
	}

	/* Only have hw stencil when depth buffer is 24 bits deep */
	r300->state.stencil.hw_stencil = (ctx->Visual.stencilBits > 0 &&
					  ctx->Visual.depthBits == 24);

	memset(&(r300->state.texture), 0, sizeof(r300->state.texture));

	r300ResetHwState(r300);
}

static void r300RenderMode(GLcontext * ctx, GLenum mode)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	(void)rmesa;
	(void)mode;
}

/**
 * Initialize driver's state callback functions
 */
void r300InitStateFuncs(struct dd_function_table *functions)
{
	radeonInitStateFuncs(functions);

	functions->UpdateState = r300InvalidateState;
	functions->AlphaFunc = r300AlphaFunc;
	functions->BlendColor = r300BlendColor;
	functions->BlendEquationSeparate = r300BlendEquationSeparate;
	functions->BlendFuncSeparate = r300BlendFuncSeparate;
	functions->Enable = r300Enable;
	functions->ColorMask = r300ColorMask;
	functions->DepthFunc = r300DepthFunc;
	functions->DepthMask = r300DepthMask;
	functions->CullFace = r300CullFace;
	functions->Fogfv = r300Fogfv;
	functions->FrontFace = r300FrontFace;
	functions->ShadeModel = r300ShadeModel;

	/* Stencil related */
	functions->ClearStencil = r300ClearStencil;
	functions->StencilFuncSeparate = r300StencilFuncSeparate;
	functions->StencilMaskSeparate = r300StencilMaskSeparate;
	functions->StencilOpSeparate = r300StencilOpSeparate;

	/* Viewport related */
	functions->Viewport = r300Viewport;
	functions->DepthRange = r300DepthRange;
	functions->PointSize = r300PointSize;
	functions->LineWidth = r300LineWidth;

	functions->PolygonOffset = r300PolygonOffset;
	functions->PolygonMode = r300PolygonMode;

	functions->RenderMode = r300RenderMode;
}
