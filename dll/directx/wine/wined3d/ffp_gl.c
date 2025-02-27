/*
 * GL fixed-function pipeline
 *
 * Copyright 2002 Lionel Ulmer
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Henri Verbeet
 * Copyright 2006-2008 Stefan Dösinger for CodeWeavers
 * Copyright 2009-2011 Henri Verbeet for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdio.h>

#include "wined3d_private.h"
#include "wined3d_gl.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/* Context activation for state handler is done by the caller. */

static void state_undefined(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    ERR("Undefined state %s (%#lx).\n", debug_d3dstate(state_id), state_id);
}

void state_nop(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    TRACE("%s: nop in current pipe config.\n", debug_d3dstate(state_id));
}

static void fillmode(const struct wined3d_rasterizer_state *r, const struct wined3d_gl_info *gl_info)
{
    enum wined3d_fill_mode mode = r ? r->desc.fill_mode : WINED3D_FILL_SOLID;

    switch (mode)
    {
        case WINED3D_FILL_POINT:
            gl_info->gl_ops.gl.p_glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            checkGLcall("glPolygonMode(GL_FRONT_AND_BACK, GL_POINT)");
            break;
        case WINED3D_FILL_WIREFRAME:
            gl_info->gl_ops.gl.p_glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            checkGLcall("glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)");
            break;
        case WINED3D_FILL_SOLID:
            gl_info->gl_ops.gl.p_glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            checkGLcall("glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)");
            break;
        default:
            FIXME("Unrecognized fill mode %#x.\n", mode);
    }
}

static void cullmode(const struct wined3d_rasterizer_state *r, const struct wined3d_gl_info *gl_info)
{
    enum wined3d_cull mode = r ? r->desc.cull_mode : WINED3D_CULL_BACK;

    /* glFrontFace() is set in context.c at context init and on an
     * offscreen / onscreen rendering switch. */
    switch (mode)
    {
        case WINED3D_CULL_NONE:
            gl_info->gl_ops.gl.p_glDisable(GL_CULL_FACE);
            checkGLcall("glDisable GL_CULL_FACE");
            break;
        case WINED3D_CULL_FRONT:
            gl_info->gl_ops.gl.p_glEnable(GL_CULL_FACE);
            checkGLcall("glEnable GL_CULL_FACE");
            gl_info->gl_ops.gl.p_glCullFace(GL_FRONT);
            checkGLcall("glCullFace(GL_FRONT)");
            break;
        case WINED3D_CULL_BACK:
            gl_info->gl_ops.gl.p_glEnable(GL_CULL_FACE);
            checkGLcall("glEnable GL_CULL_FACE");
            gl_info->gl_ops.gl.p_glCullFace(GL_BACK);
            checkGLcall("glCullFace(GL_BACK)");
            break;
        default:
            FIXME("Unrecognized cull mode %#x.\n", mode);
    }
}

void state_shademode(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;

    switch (state->render_states[WINED3D_RS_SHADEMODE])
    {
        case WINED3D_SHADE_FLAT:
            gl_info->gl_ops.gl.p_glShadeModel(GL_FLAT);
            checkGLcall("glShadeModel(GL_FLAT)");
            break;
        case WINED3D_SHADE_GOURAUD:
        /* WINED3D_SHADE_PHONG in practice is the same as WINED3D_SHADE_GOURAUD
         * in D3D. */
        case WINED3D_SHADE_PHONG:
            gl_info->gl_ops.gl.p_glShadeModel(GL_SMOOTH);
            checkGLcall("glShadeModel(GL_SMOOTH)");
            break;
        default:
            FIXME("Unrecognized shade mode %#x.\n",
                    state->render_states[WINED3D_RS_SHADEMODE]);
    }
}

static void state_ditherenable(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;

    if (state->render_states[WINED3D_RS_DITHERENABLE])
    {
        gl_info->gl_ops.gl.p_glEnable(GL_DITHER);
        checkGLcall("glEnable GL_DITHER");
    }
    else
    {
        gl_info->gl_ops.gl.p_glDisable(GL_DITHER);
        checkGLcall("glDisable GL_DITHER");
    }
}

GLenum wined3d_gl_compare_func(enum wined3d_cmp_func f)
{
    switch (f)
    {
        case WINED3D_CMP_NEVER:
            return GL_NEVER;
        case WINED3D_CMP_LESS:
            return GL_LESS;
        case WINED3D_CMP_EQUAL:
            return GL_EQUAL;
        case WINED3D_CMP_LESSEQUAL:
            return GL_LEQUAL;
        case WINED3D_CMP_GREATER:
            return GL_GREATER;
        case WINED3D_CMP_NOTEQUAL:
            return GL_NOTEQUAL;
        case WINED3D_CMP_GREATEREQUAL:
            return GL_GEQUAL;
        case WINED3D_CMP_ALWAYS:
            return GL_ALWAYS;
        default:
            if (!f)
                WARN("Unrecognized compare function %#x.\n", f);
            else
                FIXME("Unrecognized compare function %#x.\n", f);
            return GL_NONE;
    }
}

static GLenum gl_blend_op(const struct wined3d_gl_info *gl_info, enum wined3d_blend_op op)
{
    switch (op)
    {
        case WINED3D_BLEND_OP_ADD:
            return GL_FUNC_ADD;
        case WINED3D_BLEND_OP_SUBTRACT:
            return gl_info->supported[EXT_BLEND_SUBTRACT] ? GL_FUNC_SUBTRACT : GL_FUNC_ADD;
        case WINED3D_BLEND_OP_REVSUBTRACT:
            return gl_info->supported[EXT_BLEND_SUBTRACT] ? GL_FUNC_REVERSE_SUBTRACT : GL_FUNC_ADD;
        case WINED3D_BLEND_OP_MIN:
            return gl_info->supported[EXT_BLEND_MINMAX] ? GL_MIN : GL_FUNC_ADD;
        case WINED3D_BLEND_OP_MAX:
            return gl_info->supported[EXT_BLEND_MINMAX] ? GL_MAX : GL_FUNC_ADD;
        default:
            if (!op)
                WARN("Unhandled blend op %#x.\n", op);
            else
                FIXME("Unhandled blend op %#x.\n", op);
            return GL_FUNC_ADD;
    }
}

static void blendop(const struct wined3d_state *state, const struct wined3d_gl_info *gl_info)
{
    const struct wined3d_blend_state *b = state->blend_state;
    GLenum blend_equation_alpha = GL_FUNC_ADD_EXT;
    GLenum blend_equation = GL_FUNC_ADD_EXT;

    if (!gl_info->supported[WINED3D_GL_BLEND_EQUATION])
    {
        WARN("Unsupported in local OpenGL implementation: glBlendEquation.\n");
        return;
    }

    /* BLENDOPALPHA requires GL_EXT_blend_equation_separate, so make sure it is around */
    if (b->desc.rt[0].op_alpha && !gl_info->supported[EXT_BLEND_EQUATION_SEPARATE])
    {
        WARN("Unsupported in local OpenGL implementation: glBlendEquationSeparate.\n");
        return;
    }

    blend_equation = gl_blend_op(gl_info, b->desc.rt[0].op);
    blend_equation_alpha = gl_blend_op(gl_info, b->desc.rt[0].op_alpha);
    TRACE("blend_equation %#x, blend_equation_alpha %#x.\n", blend_equation, blend_equation_alpha);

    if (b->desc.rt[0].op != b->desc.rt[0].op_alpha)
    {
        GL_EXTCALL(glBlendEquationSeparate(blend_equation, blend_equation_alpha));
        checkGLcall("glBlendEquationSeparate");
    }
    else
    {
        GL_EXTCALL(glBlendEquation(blend_equation));
        checkGLcall("glBlendEquation");
    }
}

static GLenum gl_blend_factor(enum wined3d_blend factor, const struct wined3d_format *dst_format)
{
    switch (factor)
    {
        case WINED3D_BLEND_ZERO:
            return GL_ZERO;
        case WINED3D_BLEND_ONE:
            return GL_ONE;
        case WINED3D_BLEND_SRCCOLOR:
            return GL_SRC_COLOR;
        case WINED3D_BLEND_INVSRCCOLOR:
            return GL_ONE_MINUS_SRC_COLOR;
        case WINED3D_BLEND_SRCALPHA:
            return GL_SRC_ALPHA;
        case WINED3D_BLEND_INVSRCALPHA:
            return GL_ONE_MINUS_SRC_ALPHA;
        case WINED3D_BLEND_DESTCOLOR:
            return GL_DST_COLOR;
        case WINED3D_BLEND_INVDESTCOLOR:
            return GL_ONE_MINUS_DST_COLOR;
        /* To compensate for the lack of format switching with backbuffer
         * offscreen rendering, and with onscreen rendering, we modify the
         * alpha test parameters for (INV)DESTALPHA if the render target
         * doesn't support alpha blending. A nonexistent alpha channel
         * returns 1.0, so WINED3D_BLEND_DESTALPHA becomes GL_ONE, and
         * WINED3D_BLEND_INVDESTALPHA becomes GL_ZERO. */
        case WINED3D_BLEND_DESTALPHA:
            return dst_format->alpha_size ? GL_DST_ALPHA : GL_ONE;
        case WINED3D_BLEND_INVDESTALPHA:
            return dst_format->alpha_size ? GL_ONE_MINUS_DST_ALPHA : GL_ZERO;
        case WINED3D_BLEND_SRCALPHASAT:
            return GL_SRC_ALPHA_SATURATE;
        case WINED3D_BLEND_BLENDFACTOR:
            return GL_CONSTANT_COLOR_EXT;
        case WINED3D_BLEND_INVBLENDFACTOR:
            return GL_ONE_MINUS_CONSTANT_COLOR_EXT;
        case WINED3D_BLEND_SRC1COLOR:
            return GL_SRC1_COLOR;
        case WINED3D_BLEND_INVSRC1COLOR:
            return GL_ONE_MINUS_SRC1_COLOR;
        case WINED3D_BLEND_SRC1ALPHA:
            return GL_SRC1_ALPHA;
        case WINED3D_BLEND_INVSRC1ALPHA:
            return GL_ONE_MINUS_SRC1_ALPHA;
        default:
            if (!factor)
                WARN("Unhandled blend factor %#x.\n", factor);
            else
                FIXME("Unhandled blend factor %#x.\n", factor);
            return GL_NONE;
    }
}

static void gl_blend_from_d3d(GLenum *src_blend, GLenum *dst_blend,
        enum wined3d_blend d3d_src_blend, enum wined3d_blend d3d_dst_blend,
        const struct wined3d_format *rt_format)
{
    /* WINED3D_BLEND_BOTHSRCALPHA and WINED3D_BLEND_BOTHINVSRCALPHA are legacy
     * source blending values which are still valid up to d3d9. They should
     * not occur as dest blend values. */
    if (d3d_src_blend == WINED3D_BLEND_BOTHSRCALPHA)
    {
        *src_blend = GL_SRC_ALPHA;
        *dst_blend = GL_ONE_MINUS_SRC_ALPHA;
    }
    else if (d3d_src_blend == WINED3D_BLEND_BOTHINVSRCALPHA)
    {
        *src_blend = GL_ONE_MINUS_SRC_ALPHA;
        *dst_blend = GL_SRC_ALPHA;
    }
    else
    {
        *src_blend = gl_blend_factor(d3d_src_blend, rt_format);
        *dst_blend = gl_blend_factor(d3d_dst_blend, rt_format);
    }
}

static void state_blend_factor_w(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    WARN("Unsupported in local OpenGL implementation: glBlendColor.\n");
}

static void state_blend_factor(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    const struct wined3d_color *factor = &state->blend_factor;

    TRACE("Setting blend factor to %s.\n", debug_color(factor));

    GL_EXTCALL(glBlendColor(factor->r, factor->g, factor->b, factor->a));
    checkGLcall("glBlendColor");
}

static BOOL is_blend_enabled(struct wined3d_context *context, const struct wined3d_state *state, UINT index)
{
    const struct wined3d_blend_state *b = state->blend_state;

    if (!state->fb.render_targets[index])
        return FALSE;

    if (!b->desc.rt[index].enable)
        return FALSE;

    /* Disable blending in all cases even without pixel shaders.
     * With blending on we could face a big performance penalty.
     * The d3d9 visual test confirms the behavior. */
    if (!(state->fb.render_targets[index]->format_caps & WINED3D_FORMAT_CAP_POSTPIXELSHADER_BLENDING))
        return FALSE;

    return TRUE;
}

static void blend(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    const struct wined3d_blend_state *b = state->blend_state;
    const struct wined3d_format *rt_format;
    GLenum src_blend, dst_blend;
    unsigned int mask;

    if (gl_info->supported[ARB_MULTISAMPLE])
    {
        if (b && b->desc.alpha_to_coverage)
            gl_info->gl_ops.gl.p_glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        else
            gl_info->gl_ops.gl.p_glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        checkGLcall("glEnable GL_SAMPLE_ALPHA_TO_COVERAGE");
    }

    if (b && b->desc.independent)
        WARN("Independent blend is not supported by this GL implementation.\n");

    mask = b ? b->desc.rt[0].writemask : 0xf;

    gl_info->gl_ops.gl.p_glColorMask(mask & WINED3DCOLORWRITEENABLE_RED ? GL_TRUE : GL_FALSE,
            mask & WINED3DCOLORWRITEENABLE_GREEN ? GL_TRUE : GL_FALSE,
            mask & WINED3DCOLORWRITEENABLE_BLUE ? GL_TRUE : GL_FALSE,
            mask & WINED3DCOLORWRITEENABLE_ALPHA ? GL_TRUE : GL_FALSE);
    checkGLcall("glColorMask");

    if (!b || !is_blend_enabled(context, state, 0))
    {
        gl_info->gl_ops.gl.p_glDisable(GL_BLEND);
        checkGLcall("glDisable GL_BLEND");
        return;
    }

    gl_info->gl_ops.gl.p_glEnable(GL_BLEND);
    checkGLcall("glEnable GL_BLEND");

    rt_format = state->fb.render_targets[0]->format;

    gl_blend_from_d3d(&src_blend, &dst_blend, b->desc.rt[0].src, b->desc.rt[0].dst, rt_format);

    blendop(state, gl_info);

    if (b->desc.rt[0].src != b->desc.rt[0].src_alpha || b->desc.rt[0].dst != b->desc.rt[0].dst_alpha)
    {
        GLenum src_blend_alpha, dst_blend_alpha;

        /* Separate alpha blending requires GL_EXT_blend_function_separate, so make sure it is around */
        if (!gl_info->supported[EXT_BLEND_FUNC_SEPARATE])
        {
            WARN("Unsupported in local OpenGL implementation: glBlendFuncSeparate.\n");
            return;
        }

        gl_blend_from_d3d(&src_blend_alpha, &dst_blend_alpha, b->desc.rt[0].src_alpha, b->desc.rt[0].dst_alpha, rt_format);

        GL_EXTCALL(glBlendFuncSeparate(src_blend, dst_blend, src_blend_alpha, dst_blend_alpha));
        checkGLcall("glBlendFuncSeparate");
    }
    else
    {
        TRACE("glBlendFunc src=%x, dst=%x.\n", src_blend, dst_blend);
        gl_info->gl_ops.gl.p_glBlendFunc(src_blend, dst_blend);
        checkGLcall("glBlendFunc");
    }
}

static void set_color_mask(const struct wined3d_gl_info *gl_info, UINT index, DWORD mask)
{
    GL_EXTCALL(glColorMaski(index,
            mask & WINED3DCOLORWRITEENABLE_RED ? GL_TRUE : GL_FALSE,
            mask & WINED3DCOLORWRITEENABLE_GREEN ? GL_TRUE : GL_FALSE,
            mask & WINED3DCOLORWRITEENABLE_BLUE ? GL_TRUE : GL_FALSE,
            mask & WINED3DCOLORWRITEENABLE_ALPHA ? GL_TRUE : GL_FALSE));
    checkGLcall("glColorMaski");
}

static void blend_db2(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    GLenum src_blend, dst_blend, src_blend_alpha, dst_blend_alpha;
    const struct wined3d_blend_state *b = state->blend_state;
    const struct wined3d_format *rt_format;
    BOOL dual_source = b && b->dual_source;
    unsigned int i;

    if (b && b->desc.alpha_to_coverage)
        gl_info->gl_ops.gl.p_glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    else
        gl_info->gl_ops.gl.p_glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    checkGLcall("glEnable GL_SAMPLE_ALPHA_TO_COVERAGE");

    if (context->last_was_dual_source_blend != dual_source)
    {
        /* Dual source blending changes the location of the output varyings. */
        context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_PIXEL;
        context->last_was_dual_source_blend = dual_source;
    }

    if (!b || !b->desc.independent)
    {
        blend(context, state, state_id);
        return;
    }

    rt_format = state->fb.render_targets[0]->format;
    gl_blend_from_d3d(&src_blend, &dst_blend, b->desc.rt[0].src, b->desc.rt[0].dst, rt_format);
    gl_blend_from_d3d(&src_blend_alpha, &dst_blend_alpha, b->desc.rt[0].src_alpha, b->desc.rt[0].dst_alpha, rt_format);

    GL_EXTCALL(glBlendFuncSeparate(src_blend, dst_blend, src_blend_alpha, dst_blend_alpha));
    checkGLcall("glBlendFuncSeparate");

    GL_EXTCALL(glBlendEquationSeparate(gl_blend_op(gl_info, b->desc.rt[0].op),
            gl_blend_op(gl_info, b->desc.rt[0].op_alpha)));
    checkGLcall("glBlendEquationSeparate");

    for (i = 0; i < WINED3D_MAX_RENDER_TARGETS; ++i)
    {
        set_color_mask(gl_info, i, b->desc.rt[i].writemask);

        if (!is_blend_enabled(context, state, i))
        {
            GL_EXTCALL(glDisablei(GL_BLEND, i));
            checkGLcall("glDisablei GL_BLEND");
            continue;
        }

        GL_EXTCALL(glEnablei(GL_BLEND, i));
        checkGLcall("glEnablei GL_BLEND");

        if (b->desc.rt[i].src != b->desc.rt[0].src
                || b->desc.rt[i].dst != b->desc.rt[0].dst
                || b->desc.rt[i].op != b->desc.rt[0].op
                || b->desc.rt[i].src_alpha != b->desc.rt[0].src_alpha
                || b->desc.rt[i].dst_alpha != b->desc.rt[0].dst_alpha
                || b->desc.rt[i].op_alpha != b->desc.rt[0].op_alpha)
            WARN("Independent blend equations and blend functions are not supported by this GL implementation.\n");
    }
}

static void blend_dbb(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    const struct wined3d_blend_state *b = state->blend_state;
    BOOL dual_source = b && b->dual_source;
    unsigned int i;

    if (b && b->desc.alpha_to_coverage)
        gl_info->gl_ops.gl.p_glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    else
        gl_info->gl_ops.gl.p_glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    checkGLcall("glEnable GL_SAMPLE_ALPHA_TO_COVERAGE");

    if (context->last_was_dual_source_blend != dual_source)
    {
        /* Dual source blending changes the location of the output varyings. */
        context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_PIXEL;
        context->last_was_dual_source_blend = dual_source;
    }

    if (!b || !b->desc.independent)
    {
        blend(context, state, state_id);
        return;
    }

    for (i = 0; i < WINED3D_MAX_RENDER_TARGETS; ++i)
    {
        GLenum src_blend, dst_blend, src_blend_alpha, dst_blend_alpha;
        const struct wined3d_format *rt_format;

        set_color_mask(gl_info, i, b->desc.rt[i].writemask);

        if (!is_blend_enabled(context, state, i))
        {
            GL_EXTCALL(glDisablei(GL_BLEND, i));
            checkGLcall("glDisablei GL_BLEND");
            continue;
        }

        GL_EXTCALL(glEnablei(GL_BLEND, i));
        checkGLcall("glEnablei GL_BLEND");

        rt_format = state->fb.render_targets[i]->format;
        gl_blend_from_d3d(&src_blend, &dst_blend, b->desc.rt[i].src, b->desc.rt[i].dst, rt_format);
        gl_blend_from_d3d(&src_blend_alpha, &dst_blend_alpha,
                b->desc.rt[i].src_alpha, b->desc.rt[i].dst_alpha, rt_format);

        GL_EXTCALL(glBlendFuncSeparatei(i, src_blend, dst_blend, src_blend_alpha, dst_blend_alpha));
        checkGLcall("glBlendFuncSeparatei");

        GL_EXTCALL(glBlendEquationSeparatei(i, gl_blend_op(gl_info, b->desc.rt[i].op),
                gl_blend_op(gl_info, b->desc.rt[i].op_alpha)));
        checkGLcall("glBlendEquationSeparatei");
    }
}

void state_clipping(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    uint32_t enable_mask;

    /* glEnable(GL_CLIP_PLANEx) doesn't apply to (ARB backend) vertex shaders.
     * The enabled / disabled planes are hardcoded into the shader. Update the
     * shader to update the enabled clipplanes. In case of fixed function, we
     * need to update the clipping field from ffp_vertex_settings. */
    context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_VERTEX;

    /* If enabling / disabling all
     * TODO: Is this correct? Doesn't D3DRS_CLIPPING disable clipping on the viewport frustrum?
     */
    enable_mask = state->render_states[WINED3D_RS_CLIPPING] ?
            state->render_states[WINED3D_RS_CLIPPLANEENABLE] : 0;
    wined3d_context_gl_enable_clip_distances(context_gl, enable_mask);
}

static void renderstate_stencil_twosided(struct wined3d_context *context, GLint face,
        GLint func, GLint ref, GLuint mask, GLint stencilFail, GLint depthFail, GLint stencilPass)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;

    gl_info->gl_ops.gl.p_glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
    checkGLcall("glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT)");
    GL_EXTCALL(glActiveStencilFaceEXT(face));
    checkGLcall("glActiveStencilFaceEXT(...)");
    gl_info->gl_ops.gl.p_glStencilFunc(func, ref, mask);
    checkGLcall("glStencilFunc(...)");
    gl_info->gl_ops.gl.p_glStencilOp(stencilFail, depthFail, stencilPass);
    checkGLcall("glStencilOp(...)");
}

static GLenum gl_stencil_op(enum wined3d_stencil_op op)
{
    switch (op)
    {
        case WINED3D_STENCIL_OP_KEEP:
            return GL_KEEP;
        case WINED3D_STENCIL_OP_ZERO:
            return GL_ZERO;
        case WINED3D_STENCIL_OP_REPLACE:
            return GL_REPLACE;
        case WINED3D_STENCIL_OP_INCR_SAT:
            return GL_INCR;
        case WINED3D_STENCIL_OP_DECR_SAT:
            return GL_DECR;
        case WINED3D_STENCIL_OP_INVERT:
            return GL_INVERT;
        case WINED3D_STENCIL_OP_INCR:
            return GL_INCR_WRAP;
        case WINED3D_STENCIL_OP_DECR:
            return GL_DECR_WRAP;
        default:
            if (!op)
                WARN("Unrecognized stencil op %#x.\n", op);
            else
                FIXME("Unrecognized stencil op %#x.\n", op);
            return GL_KEEP;
    }
}

static void state_stencil(struct wined3d_context *context, const struct wined3d_state *state)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    const struct wined3d_depth_stencil_state *d = state->depth_stencil_state;
    GLint func;
    GLint func_back;
    GLint ref;
    GLuint mask;
    GLint stencilFail;
    GLint stencilFail_back;
    GLint stencilPass;
    GLint stencilPass_back;
    GLint depthFail;
    GLint depthFail_back;

    /* No stencil test without a stencil buffer. */
    if (!state->fb.depth_stencil || !d || !d->desc.stencil)
    {
        gl_info->gl_ops.gl.p_glDisable(GL_STENCIL_TEST);
        checkGLcall("glDisable GL_STENCIL_TEST");
        return;
    }

    if (!(func = wined3d_gl_compare_func(d->desc.front.func)))
        func = GL_ALWAYS;
    if (!(func_back = wined3d_gl_compare_func(d->desc.back.func)))
        func_back = GL_ALWAYS;
    mask = d->desc.stencil_read_mask;
    ref = state->stencil_ref & wined3d_mask_from_size(state->fb.depth_stencil->format->stencil_size);
    stencilFail = gl_stencil_op(d->desc.front.fail_op);
    depthFail = gl_stencil_op(d->desc.front.depth_fail_op);
    stencilPass = gl_stencil_op(d->desc.front.pass_op);
    stencilFail_back = gl_stencil_op(d->desc.back.fail_op);
    depthFail_back = gl_stencil_op(d->desc.back.depth_fail_op);
    stencilPass_back = gl_stencil_op(d->desc.back.pass_op);

    TRACE("(ref %x, mask %x, "
            "GL_FRONT: func: %x, fail %x, zfail %x, zpass %x "
            "GL_BACK: func: %x, fail %x, zfail %x, zpass %x)\n",
            ref, mask,
            func, stencilFail, depthFail, stencilPass,
            func_back, stencilFail_back, depthFail_back, stencilPass_back);

    if (memcmp(&d->desc.front, &d->desc.back, sizeof(d->desc.front)))
    {
        gl_info->gl_ops.gl.p_glEnable(GL_STENCIL_TEST);
        checkGLcall("glEnable GL_STENCIL_TEST");

        if (gl_info->supported[WINED3D_GL_VERSION_2_0])
        {
            GL_EXTCALL(glStencilFuncSeparate(GL_FRONT, func, ref, mask));
            GL_EXTCALL(glStencilOpSeparate(GL_FRONT, stencilFail, depthFail, stencilPass));
            GL_EXTCALL(glStencilFuncSeparate(GL_BACK, func_back, ref, mask));
            GL_EXTCALL(glStencilOpSeparate(GL_BACK, stencilFail_back, depthFail_back, stencilPass_back));
            checkGLcall("setting two sided stencil state");
        }
        else if (gl_info->supported[EXT_STENCIL_TWO_SIDE])
        {
            /* Apply back first, then front. This function calls glActiveStencilFaceEXT,
             * which has an effect on the code below too. If we apply the front face
             * afterwards, we are sure that the active stencil face is set to front,
             * and other stencil functions which do not use two sided stencil do not have
             * to set it back
             */
            renderstate_stencil_twosided(context, GL_BACK,
                    func_back, ref, mask, stencilFail_back, depthFail_back, stencilPass_back);
            renderstate_stencil_twosided(context, GL_FRONT,
                    func, ref, mask, stencilFail, depthFail, stencilPass);
        }
        else if (gl_info->supported[ATI_SEPARATE_STENCIL])
        {
            GL_EXTCALL(glStencilFuncSeparateATI(func, func_back, ref, mask));
            checkGLcall("glStencilFuncSeparateATI(...)");
            GL_EXTCALL(glStencilOpSeparateATI(GL_FRONT, stencilFail, depthFail, stencilPass));
            checkGLcall("glStencilOpSeparateATI(GL_FRONT, ...)");
            GL_EXTCALL(glStencilOpSeparateATI(GL_BACK, stencilFail_back, depthFail_back, stencilPass_back));
            checkGLcall("glStencilOpSeparateATI(GL_BACK, ...)");
        }
        else
        {
            FIXME("Separate (two sided) stencil not supported on this version of OpenGL. Caps weren't honored?\n");
        }
    }
    else
    {
        if (gl_info->supported[EXT_STENCIL_TWO_SIDE])
        {
            gl_info->gl_ops.gl.p_glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
            checkGLcall("glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT)");
        }

        /* This code disables the ATI extension as well, since the standard stencil functions are equal
         * to calling the ATI functions with GL_FRONT_AND_BACK as face parameter
         */
        gl_info->gl_ops.gl.p_glEnable(GL_STENCIL_TEST);
        checkGLcall("glEnable GL_STENCIL_TEST");
        gl_info->gl_ops.gl.p_glStencilFunc(func, ref, mask);
        checkGLcall("glStencilFunc(...)");
        gl_info->gl_ops.gl.p_glStencilOp(stencilFail, depthFail, stencilPass);
        checkGLcall("glStencilOp(...)");
    }
}

static void depth(struct wined3d_context *context, const struct wined3d_state *state)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    const struct wined3d_depth_stencil_state *d = state->depth_stencil_state;
    BOOL enable_depth = d ? d->desc.depth : TRUE;
    GLenum depth_func = GL_LESS;

    if (!state->fb.depth_stencil)
    {
        TRACE("No depth/stencil buffer is attached; disabling depth test.\n");
        enable_depth = FALSE;
    }

    if (enable_depth)
    {
        gl_info->gl_ops.gl.p_glEnable(GL_DEPTH_TEST);
        checkGLcall("glEnable GL_DEPTH_TEST");
    }
    else
    {
        gl_info->gl_ops.gl.p_glDisable(GL_DEPTH_TEST);
        checkGLcall("glDisable GL_DEPTH_TEST");
    }

    if (!d || d->desc.depth_write)
    {
        gl_info->gl_ops.gl.p_glDepthMask(GL_TRUE);
        checkGLcall("glDepthMask(GL_TRUE)");
    }
    else
    {
        gl_info->gl_ops.gl.p_glDepthMask(GL_FALSE);
        checkGLcall("glDepthMask(GL_FALSE)");
    }

    if (d)
        depth_func = wined3d_gl_compare_func(d->desc.depth_func);
    if (depth_func)
    {
        gl_info->gl_ops.gl.p_glDepthFunc(depth_func);
        checkGLcall("glDepthFunc");
    }

    if (gl_info->supported[EXT_DEPTH_BOUNDS_TEST])
    {
        /* If min is larger than max, an INVALID_VALUE error is generated.
         * In d3d9, the test is not performed in this case. */
        if (state->depth_bounds_enable && state->depth_bounds_min <= state->depth_bounds_max)
        {
            gl_info->gl_ops.gl.p_glEnable(GL_DEPTH_BOUNDS_TEST_EXT);
            checkGLcall("glEnable(GL_DEPTH_BOUNDS_TEST_EXT)");
            GL_EXTCALL(glDepthBoundsEXT(state->depth_bounds_min, state->depth_bounds_max));
            checkGLcall("glDepthBoundsEXT");
        }
        else
        {
            gl_info->gl_ops.gl.p_glDisable(GL_DEPTH_BOUNDS_TEST_EXT);
            checkGLcall("glDisable(GL_DEPTH_BOUNDS_TEST_EXT)");
        }
    }

    if (context->stream_info.position_transformed)
        context->constant_update_mask |= WINED3D_SHADER_CONST_FFP_PROJ;
}

static void depth_stencil(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    const struct wined3d_depth_stencil_state *d = state->depth_stencil_state;
    GLuint stencil_write_mask = 0;

    depth(context, state);
    state_stencil(context, state);

    if (state->fb.depth_stencil)
        stencil_write_mask = d ? d->desc.stencil_write_mask : ~0u;

    gl_info->gl_ops.gl.p_glStencilMask(stencil_write_mask);
    checkGLcall("glStencilMask");
}

static void depth_stencil_2s(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    const struct wined3d_depth_stencil_state *d = state->depth_stencil_state;
    GLuint stencil_write_mask = 0;

    depth(context, state);
    state_stencil(context, state);

    if (state->fb.depth_stencil)
        stencil_write_mask = d ? d->desc.stencil_write_mask : ~0u;

    GL_EXTCALL(glActiveStencilFaceEXT(GL_BACK));
    checkGLcall("glActiveStencilFaceEXT(GL_BACK)");
    gl_info->gl_ops.gl.p_glStencilMask(stencil_write_mask);
    checkGLcall("glStencilMask");
    GL_EXTCALL(glActiveStencilFaceEXT(GL_FRONT));
    checkGLcall("glActiveStencilFaceEXT(GL_FRONT)");
    gl_info->gl_ops.gl.p_glStencilMask(stencil_write_mask);
}

static void state_linepattern(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    union
    {
        DWORD d;
        struct wined3d_line_pattern lp;
    } tmppattern;
    tmppattern.d = state->render_states[WINED3D_RS_LINEPATTERN];

    TRACE("Line pattern: repeat %d bits %x.\n", tmppattern.lp.repeat_factor, tmppattern.lp.line_pattern);

    if (tmppattern.lp.repeat_factor)
    {
        gl_info->gl_ops.gl.p_glLineStipple(tmppattern.lp.repeat_factor, tmppattern.lp.line_pattern);
        checkGLcall("glLineStipple(repeat, linepattern)");
        gl_info->gl_ops.gl.p_glEnable(GL_LINE_STIPPLE);
        checkGLcall("glEnable(GL_LINE_STIPPLE);");
    }
    else
    {
        gl_info->gl_ops.gl.p_glDisable(GL_LINE_STIPPLE);
        checkGLcall("glDisable(GL_LINE_STIPPLE);");
    }
}

static void state_linepattern_w(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    static unsigned int once;

    if (!once++)
        FIXME("Setting line patterns is not supported in OpenGL core contexts.\n");
}

static void state_msaa_w(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (state->render_states[WINED3D_RS_MULTISAMPLEANTIALIAS])
        WARN("Multisample antialiasing not supported by GL.\n");
}

static void state_msaa(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;

    if (state->render_states[WINED3D_RS_MULTISAMPLEANTIALIAS])
    {
        gl_info->gl_ops.gl.p_glEnable(GL_MULTISAMPLE_ARB);
        checkGLcall("glEnable(GL_MULTISAMPLE_ARB)");
    }
    else
    {
        gl_info->gl_ops.gl.p_glDisable(GL_MULTISAMPLE_ARB);
        checkGLcall("glDisable(GL_MULTISAMPLE_ARB)");
    }
}

static void line_antialias(const struct wined3d_rasterizer_state *r, const struct wined3d_gl_info *gl_info)
{
    if (r && r->desc.line_antialias)
    {
        gl_info->gl_ops.gl.p_glEnable(GL_LINE_SMOOTH);
        checkGLcall("glEnable(GL_LINE_SMOOTH)");
    }
    else
    {
        gl_info->gl_ops.gl.p_glDisable(GL_LINE_SMOOTH);
        checkGLcall("glDisable(GL_LINE_SMOOTH)");
    }
}

static void scissor(const struct wined3d_rasterizer_state *r, const struct wined3d_gl_info *gl_info)
{
    if (r && r->desc.scissor)
    {
        gl_info->gl_ops.gl.p_glEnable(GL_SCISSOR_TEST);
        checkGLcall("glEnable(GL_SCISSOR_TEST)");
    }
    else
    {
        gl_info->gl_ops.gl.p_glDisable(GL_SCISSOR_TEST);
        checkGLcall("glDisable(GL_SCISSOR_TEST)");
    }
}

/* The Direct3D depth bias is specified in normalized depth coordinates. In
 * OpenGL the bias is specified in units of "the smallest value that is
 * guaranteed to produce a resolvable offset for a given implementation". To
 * convert from D3D to GL we need to divide the D3D depth bias by that value.
 * We try to detect the value from GL with test draws. On most drivers (r300g,
 * 600g, Nvidia, i965 on Mesa) the value is 2^23 for fixed point depth buffers,
 * for r200 and i965 on OSX it is 2^24, for r500 on OSX it is 2^22. For floating
 * point buffers it is 2^22, 2^23 or 2^24 depending on the GPU. The value does
 * not depend on the depth buffer precision on any driver.
 *
 * Two games that are picky regarding depth bias are Mass Effect 2 (flickering
 * decals) and F.E.A.R and F.E.A.R. 2 (semi-transparent guns).
 *
 * Note that SLOPESCALEDEPTHBIAS is a scaling factor for the depth slope, and
 * doesn't need to be scaled to account for GL vs D3D differences. */
static void depthbias(struct wined3d_context *context, const struct wined3d_state *state)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    const struct wined3d_rasterizer_state *r = state->rasterizer_state;
    float scale_bias = r ? r->desc.scale_bias : 0.0f;
    union
    {
        DWORD d;
        float f;
    } const_bias;

    const_bias.f = r ? r->desc.depth_bias : 0.0f;

    if (scale_bias || const_bias.f)
    {
        const struct wined3d_rendertarget_view *depth = state->fb.depth_stencil;
        float factor, units, scale, clamp;

        clamp = r ? r->desc.depth_bias_clamp : 0.0f;

        if (context->d3d_info->wined3d_creation_flags & WINED3D_LEGACY_DEPTH_BIAS)
        {
            factor = units = -(float)const_bias.d;
        }
        else
        {
            if (depth)
            {
                scale = depth->format->depth_bias_scale;

                TRACE("Depth format %s, using depthbias scale of %.8e.\n",
                        debug_d3dformat(depth->format->id), scale);
            }
            else
            {
                /* The context manager will reapply this state on a depth stencil change */
                TRACE("No depth stencil, using depth bias scale of 0.0.\n");
                scale = 0.0f;
            }

            factor = scale_bias;
            units = const_bias.f * scale;
        }

        gl_info->gl_ops.gl.p_glEnable(GL_POLYGON_OFFSET_FILL);
        if (gl_info->supported[ARB_POLYGON_OFFSET_CLAMP])
        {
            gl_info->gl_ops.ext.p_glPolygonOffsetClamp(factor, units, clamp);
        }
        else
        {
            if (clamp != 0.0f)
                WARN("Ignoring depth bias clamp %.8e.\n", clamp);
            gl_info->gl_ops.gl.p_glPolygonOffset(factor, units);
        }
    }
    else
    {
        gl_info->gl_ops.gl.p_glDisable(GL_POLYGON_OFFSET_FILL);
    }

    checkGLcall("depth bias");
}

static void state_sample_mask(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    unsigned int sample_mask = state->sample_mask;

    TRACE("Setting sample mask to %#x.\n", sample_mask);
    if (sample_mask != 0xffffffff)
    {
        gl_info->gl_ops.gl.p_glEnable(GL_SAMPLE_MASK);
        checkGLcall("glEnable GL_SAMPLE_MASK");
        GL_EXTCALL(glSampleMaski(0, sample_mask));
        checkGLcall("glSampleMaski");
    }
    else
    {
        gl_info->gl_ops.gl.p_glDisable(GL_SAMPLE_MASK);
        checkGLcall("glDisable GL_SAMPLE_MASK");
    }
}

static void state_sample_mask_w(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    WARN("Unsupported in local OpenGL implementation: glSampleMaski.\n");
}

static void state_compute_shader(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_COMPUTE;
}

static void state_shader(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    enum wined3d_shader_type shader_type = state_id - STATE_SHADER(0);
    context->shader_update_mask |= 1u << shader_type;
}

void clipplane(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    context->constant_update_mask |= WINED3D_SHADER_CONST_VS_CLIP_PLANES;
}

void ffp_vertex_update_clip_plane_constants(const struct wined3d_gl_info *gl_info, const struct wined3d_state *state)
{
    for (unsigned int i = 0; i < gl_info->limits.user_clip_distances; ++i)
    {
        GLdouble plane[4];

        gl_info->gl_ops.gl.p_glMatrixMode(GL_MODELVIEW);
        gl_info->gl_ops.gl.p_glPushMatrix();
        gl_info->gl_ops.gl.p_glLoadIdentity();

        plane[0] = state->clip_planes[i].x;
        plane[1] = state->clip_planes[i].y;
        plane[2] = state->clip_planes[i].z;
        plane[3] = state->clip_planes[i].w;

        gl_info->gl_ops.gl.p_glClipPlane(GL_CLIP_PLANE0 + i, plane);
        checkGLcall("glClipPlane");

        gl_info->gl_ops.gl.p_glPopMatrix();
    }
}

static void streamsrc(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    wined3d_context_gl_update_stream_sources(wined3d_context_gl(context), state);
}

static void vdecl_miscpart(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (isStateDirty(context, STATE_STREAMSRC))
        return;
    wined3d_context_gl_update_stream_sources(wined3d_context_gl(context), state);
}

static void viewport_miscpart(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    float min_z, max_z;

    if (gl_info->supported[ARB_VIEWPORT_ARRAY])
    {
        GLdouble depth_ranges[2 * WINED3D_MAX_VIEWPORTS];
        GLfloat viewports[4 * WINED3D_MAX_VIEWPORTS];

        unsigned int i, reset_count = 0;

        for (i = 0; i < state->viewport_count; ++i)
        {
            wined3d_viewport_get_z_range(&state->viewports[i], &min_z, &max_z);
            depth_ranges[i * 2] = min_z;
            depth_ranges[i * 2 + 1] = max_z;

            viewports[i * 4]     = state->viewports[i].x;
            viewports[i * 4 + 1] = state->viewports[i].y;
            viewports[i * 4 + 2] = state->viewports[i].width;
            viewports[i * 4 + 3] = state->viewports[i].height;

            /* Don't pass fractionals to GL if we earlier decided not to use
             * this functionality for two reasons: First, GL might offer us
             * fewer than 8 bits, and still make use of the fractional, in
             * addition to the emulation we apply in shader_get_position_fixup.
             * Second, even if GL tells us it has no subpixel precision (Mac OS!)
             * it might still do something with the fractional amount, e.g.
             * round it upwards. I can't find any info on rounding in
             * GL_ARB_viewport_array. */
            if (!context->d3d_info->subpixel_viewport)
            {
                viewports[i * 4]     = floor(viewports[i * 4]);
                viewports[i * 4 + 1] = floor(viewports[i * 4 + 1]);
                viewports[i * 4 + 2] = floor(viewports[i * 4 + 2]);
                viewports[i * 4 + 3] = floor(viewports[i * 4 + 3]);
            }
        }

        if (context->viewport_count > state->viewport_count)
            reset_count = context->viewport_count - state->viewport_count;

        if (reset_count)
        {
            memset(&depth_ranges[state->viewport_count * 2], 0, reset_count * 2 * sizeof(*depth_ranges));
            memset(&viewports[state->viewport_count * 4], 0, reset_count * 4 * sizeof(*viewports));
        }

        GL_EXTCALL(glDepthRangeArrayv(0, state->viewport_count + reset_count, depth_ranges));
        GL_EXTCALL(glViewportArrayv(0, state->viewport_count + reset_count, viewports));
        context->viewport_count = state->viewport_count;
    }
    else
    {
        wined3d_viewport_get_z_range(&state->viewports[0], &min_z, &max_z);
        gl_info->gl_ops.gl.p_glDepthRange(min_z, max_z);
        gl_info->gl_ops.gl.p_glViewport(state->viewports[0].x, state->viewports[0].y,
                state->viewports[0].width, state->viewports[0].height);
    }
    checkGLcall("setting clip space and viewport");
}

static void viewport_miscpart_cc(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    /* See get_projection_matrix() in glsl_shader.c for a discussion about those values. */
    float pixel_center_offset = context->d3d_info->wined3d_creation_flags
            & WINED3D_PIXEL_CENTER_INTEGER ? 0.5f : 0.0f;
    GLdouble depth_ranges[2 * WINED3D_MAX_VIEWPORTS];
    GLfloat viewports[4 * WINED3D_MAX_VIEWPORTS];
    unsigned int i, reset_count = 0;
    float min_z, max_z;

    pixel_center_offset += context->d3d_info->filling_convention_offset / 2.0f;

    GL_EXTCALL(glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE));

    for (i = 0; i < state->viewport_count; ++i)
    {
        wined3d_viewport_get_z_range(&state->viewports[i], &min_z, &max_z);
        depth_ranges[i * 2] = min_z;
        depth_ranges[i * 2 + 1] = max_z;

        viewports[i * 4] = state->viewports[i].x + pixel_center_offset;
        viewports[i * 4 + 1] = state->viewports[i].y + pixel_center_offset;
        viewports[i * 4 + 2] = state->viewports[i].width;
        viewports[i * 4 + 3] = state->viewports[i].height;
    }

    if (context->viewport_count > state->viewport_count)
        reset_count = context->viewport_count - state->viewport_count;

    if (reset_count)
    {
        memset(&depth_ranges[state->viewport_count * 2], 0, reset_count * 2 * sizeof(*depth_ranges));
        memset(&viewports[state->viewport_count * 4], 0, reset_count * 4 * sizeof(*viewports));
    }

    GL_EXTCALL(glDepthRangeArrayv(0, state->viewport_count + reset_count, depth_ranges));
    GL_EXTCALL(glViewportArrayv(0, state->viewport_count + reset_count, viewports));
    context->viewport_count = state->viewport_count;

    checkGLcall("setting clip space and viewport");
}

static void scissorrect(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    const RECT *r;

    /* Warning: glScissor uses window coordinates, not viewport coordinates,
     * so our viewport correction does not apply. Warning2: Even in windowed
     * mode the coords are relative to the window, not the screen. */

    if (gl_info->supported[ARB_VIEWPORT_ARRAY])
    {
        GLint sr[4 * WINED3D_MAX_VIEWPORTS];
        unsigned int i, reset_count = 0;

        for (i = 0; i < state->scissor_rect_count; ++i)
        {
            r = &state->scissor_rects[i];

            sr[i * 4] = r->left;
            sr[i * 4 + 1] = r->top;
            sr[i * 4 + 2] = r->right - r->left;
            sr[i * 4 + 3] = r->bottom - r->top;
        }

        if (context->scissor_rect_count > state->scissor_rect_count)
            reset_count = context->scissor_rect_count - state->scissor_rect_count;

        if (reset_count)
            memset(&sr[state->scissor_rect_count * 4], 0, reset_count * 4 * sizeof(GLint));

        GL_EXTCALL(glScissorArrayv(0, state->scissor_rect_count + reset_count, sr));
        checkGLcall("glScissorArrayv");
        context->scissor_rect_count = state->scissor_rect_count;
    }
    else
    {
        r = &state->scissor_rects[0];
        gl_info->gl_ops.gl.p_glScissor(r->left, r->top, r->right - r->left, r->bottom - r->top);
        checkGLcall("glScissor");
    }
}

static void indexbuffer(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    const struct wined3d_stream_info *stream_info = &context->stream_info;
    struct wined3d_buffer *buffer;

    if (!state->index_buffer || !stream_info->all_vbo)
    {
        GL_EXTCALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
        return;
    }

    buffer = state->index_buffer;
    if (buffer->buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wined3d_bo_gl(buffer->buffer_object)->id));
        wined3d_buffer_validate_user(buffer);
    }
    else
    {
        GL_EXTCALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    }
}

static void depth_clip(const struct wined3d_rasterizer_state *r, const struct wined3d_gl_info *gl_info)
{
    if (!gl_info->supported[ARB_DEPTH_CLAMP])
    {
        if (r && !r->desc.depth_clip)
            FIXME("Depth clamp not supported by this GL implementation.\n");
        return;
    }

    if (r && !r->desc.depth_clip)
        gl_info->gl_ops.gl.p_glEnable(GL_DEPTH_CLAMP);
    else
        gl_info->gl_ops.gl.p_glDisable(GL_DEPTH_CLAMP);
    checkGLcall("depth clip");
}

static void rasterizer(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    const struct wined3d_rasterizer_state *r = state->rasterizer_state;

    /* Rendering without ARB_clip_control requires flipping position manually.
     * This also means that all primitives will be backwards, so we need to
     * also swap which side is the front face. */
    gl_info->gl_ops.gl.p_glFrontFace(r && r->desc.front_ccw ? GL_CW : GL_CCW);
    checkGLcall("glFrontFace");
    depthbias(context, state);
    fillmode(r, gl_info);
    cullmode(r, gl_info);
    depth_clip(r, gl_info);
    scissor(r, gl_info);
    line_antialias(r, gl_info);
}

static void rasterizer_cc(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    const struct wined3d_rasterizer_state *r = state->rasterizer_state;
    GLenum mode;

    mode = r && r->desc.front_ccw ? GL_CCW : GL_CW;

    gl_info->gl_ops.gl.p_glFrontFace(mode);
    checkGLcall("glFrontFace");
    depthbias(context, state);
    fillmode(r, gl_info);
    cullmode(r, gl_info);
    depth_clip(r, gl_info);
    scissor(r, gl_info);
    line_antialias(r, gl_info);
}

void state_srgbwrite(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;

    TRACE("context %p, state %p, state_id %#lx.\n", context, state, state_id);

    if (needs_srgb_write(context->d3d_info, state, &state->fb))
        gl_info->gl_ops.gl.p_glEnable(GL_FRAMEBUFFER_SRGB);
    else
        gl_info->gl_ops.gl.p_glDisable(GL_FRAMEBUFFER_SRGB);
}

static void state_cb(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    enum wined3d_shader_type shader_type;
    struct wined3d_buffer *buffer;
    unsigned int i, base, count;
    struct wined3d_bo_gl *bo_gl;

    TRACE("context %p, state %p, state_id %#lx.\n", context, state, state_id);

    if (STATE_IS_GRAPHICS_CONSTANT_BUFFER(state_id))
        shader_type = state_id - STATE_GRAPHICS_CONSTANT_BUFFER(0);
    else
        shader_type = WINED3D_SHADER_TYPE_COMPUTE;

    /* If a shader has not been set, buffer objects are not yet initialised. */
    if (!state->shader[shader_type])
        return;

    wined3d_gl_limits_get_uniform_block_range(&gl_info->limits, shader_type, &base, &count);
    for (i = 0; i < count; ++i)
    {
        const struct wined3d_constant_buffer_state *buffer_state = &state->cb[shader_type][i];

        if (!buffer_state->buffer)
        {
            GL_EXTCALL(glBindBufferBase(GL_UNIFORM_BUFFER, base + i, 0));
            continue;
        }

        buffer = buffer_state->buffer;
        bo_gl = wined3d_bo_gl(buffer->buffer_object);
        GL_EXTCALL(glBindBufferRange(GL_UNIFORM_BUFFER, base + i,
                bo_gl->id, bo_gl->b.buffer_offset + buffer_state->offset,
                min(buffer_state->size, buffer->resource.size - buffer_state->offset)));

        wined3d_buffer_validate_user(buffer);
    }
    checkGLcall("bind constant buffers");
}

static void state_cb_warn(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    TRACE("context %p, state %p, state_id %#lx.\n", context, state, state_id);

    WARN("Constant buffers (%s) no supported.\n", debug_d3dstate(state_id));
}

static void state_shader_resource_binding(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    TRACE("context %p, state %p, state_id %#lx.\n", context, state, state_id);

    context->update_shader_resource_bindings = 1;
}

static void state_cs_resource_binding(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    TRACE("context %p, state %p, state_id %#lx.\n", context, state, state_id);
    context->update_compute_shader_resource_bindings = 1;
}

static void state_uav_binding(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    TRACE("context %p, state %p, state_id %#lx.\n", context, state, state_id);
    context->update_unordered_access_view_bindings = 1;
}

static void state_cs_uav_binding(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    TRACE("context %p, state %p, state_id %#lx.\n", context, state, state_id);
    context->update_compute_unordered_access_view_bindings = 1;
}

static void state_uav_warn(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    WARN("ARB_shader_image_load_store is not supported by this OpenGL implementation.\n");
}

static void state_so(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_buffer *buffer;
    unsigned int offset, size, i;
    struct wined3d_bo_gl *bo_gl;

    TRACE("context %p, state %p, state_id %#lx.\n", context, state, state_id);

    wined3d_context_gl_end_transform_feedback(context_gl);

    for (i = 0; i < ARRAY_SIZE(state->stream_output); ++i)
    {
        if (!state->stream_output[i].buffer)
        {
            GL_EXTCALL(glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, i, 0));
            continue;
        }

        buffer = state->stream_output[i].buffer;
        offset = state->stream_output[i].offset;
        bo_gl = wined3d_bo_gl(buffer->buffer_object);
        if (offset == ~0u)
        {
            FIXME("Appending to stream output buffers not implemented.\n");
            offset = 0;
        }
        size = buffer->resource.size - offset;
        GL_EXTCALL(glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, i,
                bo_gl->id, bo_gl->b.buffer_offset + offset, size));
        wined3d_buffer_validate_user(buffer);
    }
    checkGLcall("bind transform feedback buffers");
}

static void state_so_warn(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    WARN("Transform feedback not supported.\n");
}

const struct wined3d_state_entry_template misc_state_template_gl[] =
{
    { STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_VERTEX),  { STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_VERTEX),  state_cb,           }, ARB_UNIFORM_BUFFER_OBJECT       },
    { STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_VERTEX),  { STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_VERTEX),  state_cb_warn,      }, WINED3D_GL_EXT_NONE             },
    { STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_HULL),    { STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_HULL),    state_cb,           }, ARB_UNIFORM_BUFFER_OBJECT       },
    { STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_HULL),    { STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_HULL),    state_cb_warn,      }, WINED3D_GL_EXT_NONE             },
    { STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_DOMAIN),  { STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_DOMAIN),  state_cb,           }, ARB_UNIFORM_BUFFER_OBJECT       },
    { STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_DOMAIN),  { STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_DOMAIN),  state_cb_warn,      }, WINED3D_GL_EXT_NONE             },
    { STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_GEOMETRY),{ STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_GEOMETRY),state_cb,           }, ARB_UNIFORM_BUFFER_OBJECT       },
    { STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_GEOMETRY),{ STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_GEOMETRY),state_cb_warn,      }, WINED3D_GL_EXT_NONE             },
    { STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_PIXEL),   { STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_PIXEL),   state_cb,           }, ARB_UNIFORM_BUFFER_OBJECT       },
    { STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_PIXEL),   { STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_PIXEL),   state_cb_warn,      }, WINED3D_GL_EXT_NONE             },
    { STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_COMPUTE), { STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_COMPUTE), state_cb,           }, ARB_UNIFORM_BUFFER_OBJECT       },
    { STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_COMPUTE), { STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_COMPUTE), state_cb_warn,      }, WINED3D_GL_EXT_NONE             },
    { STATE_GRAPHICS_SHADER_RESOURCE_BINDING,             { STATE_GRAPHICS_SHADER_RESOURCE_BINDING,             state_shader_resource_binding}, WINED3D_GL_EXT_NONE    },
    { STATE_GRAPHICS_UNORDERED_ACCESS_VIEW_BINDING,       { STATE_GRAPHICS_UNORDERED_ACCESS_VIEW_BINDING,       state_uav_binding   }, ARB_SHADER_IMAGE_LOAD_STORE     },
    { STATE_GRAPHICS_UNORDERED_ACCESS_VIEW_BINDING,       { STATE_GRAPHICS_UNORDERED_ACCESS_VIEW_BINDING,       state_uav_warn      }, WINED3D_GL_EXT_NONE             },
    { STATE_COMPUTE_SHADER_RESOURCE_BINDING,              { STATE_COMPUTE_SHADER_RESOURCE_BINDING,              state_cs_resource_binding}, WINED3D_GL_EXT_NONE        },
    { STATE_COMPUTE_UNORDERED_ACCESS_VIEW_BINDING,        { STATE_COMPUTE_UNORDERED_ACCESS_VIEW_BINDING,        state_cs_uav_binding}, ARB_SHADER_IMAGE_LOAD_STORE     },
    { STATE_COMPUTE_UNORDERED_ACCESS_VIEW_BINDING,        { STATE_COMPUTE_UNORDERED_ACCESS_VIEW_BINDING,        state_uav_warn      }, WINED3D_GL_EXT_NONE             },
    { STATE_STREAM_OUTPUT,                                { STATE_STREAM_OUTPUT,                                state_so,           }, WINED3D_GL_VERSION_3_2          },
    { STATE_STREAM_OUTPUT,                                { STATE_STREAM_OUTPUT,                                state_so_warn,      }, WINED3D_GL_EXT_NONE             },
    { STATE_BLEND,                                        { STATE_BLEND,                                        blend_dbb           }, ARB_DRAW_BUFFERS_BLEND          },
    { STATE_BLEND,                                        { STATE_BLEND,                                        blend_db2           }, EXT_DRAW_BUFFERS2               },
    { STATE_BLEND,                                        { STATE_BLEND,                                        blend               }, WINED3D_GL_EXT_NONE             },
    { STATE_BLEND_FACTOR,                                 { STATE_BLEND_FACTOR,                                 state_blend_factor  }, EXT_BLEND_COLOR                 },
    { STATE_BLEND_FACTOR,                                 { STATE_BLEND_FACTOR,                                 state_blend_factor_w}, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLE_MASK,                                  { STATE_SAMPLE_MASK,                                  state_sample_mask   }, ARB_TEXTURE_MULTISAMPLE         },
    { STATE_SAMPLE_MASK,                                  { STATE_SAMPLE_MASK,                                  state_sample_mask_w }, WINED3D_GL_EXT_NONE             },
    { STATE_DEPTH_STENCIL,                                { STATE_DEPTH_STENCIL,                                depth_stencil_2s    }, EXT_STENCIL_TWO_SIDE            },
    { STATE_DEPTH_STENCIL,                                { STATE_DEPTH_STENCIL,                                depth_stencil       }, WINED3D_GL_EXT_NONE             },
    { STATE_STENCIL_REF,                                  { STATE_DEPTH_STENCIL,                                NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_DEPTH_BOUNDS,                                 { STATE_DEPTH_STENCIL,                                NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_STREAMSRC,                                    { STATE_STREAMSRC,                                    streamsrc           }, WINED3D_GL_EXT_NONE             },
    { STATE_VDECL,                                        { STATE_VDECL,                                        vdecl_miscpart      }, WINED3D_GL_EXT_NONE             },
    { STATE_RASTERIZER,                                   { STATE_RASTERIZER,                                   rasterizer_cc       }, ARB_CLIP_CONTROL                },
    { STATE_RASTERIZER,                                   { STATE_RASTERIZER,                                   rasterizer          }, WINED3D_GL_EXT_NONE             },
    { STATE_SCISSORRECT,                                  { STATE_SCISSORRECT,                                  scissorrect         }, WINED3D_GL_EXT_NONE             },
    { STATE_VIEWPORT,                                     { STATE_VIEWPORT,                                     viewport_miscpart_cc}, ARB_CLIP_CONTROL                },
    { STATE_VIEWPORT,                                     { STATE_VIEWPORT,                                     viewport_miscpart   }, WINED3D_GL_EXT_NONE             },
    { STATE_INDEXBUFFER,                                  { STATE_INDEXBUFFER,                                  indexbuffer         }, ARB_VERTEX_BUFFER_OBJECT        },
    { STATE_INDEXBUFFER,                                  { STATE_INDEXBUFFER,                                  state_nop           }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_LINEPATTERN),               { STATE_RENDER(WINED3D_RS_LINEPATTERN),               state_linepattern   }, WINED3D_GL_LEGACY_CONTEXT       },
    { STATE_RENDER(WINED3D_RS_LINEPATTERN),               { STATE_RENDER(WINED3D_RS_LINEPATTERN),               state_linepattern_w }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_DITHERENABLE),              { STATE_RENDER(WINED3D_RS_DITHERENABLE),              state_ditherenable  }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_MULTISAMPLEANTIALIAS),      { STATE_RENDER(WINED3D_RS_MULTISAMPLEANTIALIAS),      state_msaa          }, ARB_MULTISAMPLE                 },
    { STATE_RENDER(WINED3D_RS_MULTISAMPLEANTIALIAS),      { STATE_RENDER(WINED3D_RS_MULTISAMPLEANTIALIAS),      state_msaa_w        }, WINED3D_GL_EXT_NONE             },
    { STATE_BASEVERTEXINDEX,                              { STATE_BASEVERTEXINDEX,                              state_nop,          }, ARB_DRAW_ELEMENTS_BASE_VERTEX   },
    { STATE_BASEVERTEXINDEX,                              { STATE_STREAMSRC,                                    NULL,               }, WINED3D_GL_EXT_NONE             },
    { STATE_FRAMEBUFFER,                                  { STATE_FRAMEBUFFER,                                  context_state_fb    }, WINED3D_GL_EXT_NONE             },
    { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            context_state_drawbuf},WINED3D_GL_EXT_NONE             },
    { STATE_SHADER(WINED3D_SHADER_TYPE_HULL),             { STATE_SHADER(WINED3D_SHADER_TYPE_HULL),             state_shader        }, WINED3D_GL_EXT_NONE             },
    { STATE_SHADER(WINED3D_SHADER_TYPE_DOMAIN),           { STATE_SHADER(WINED3D_SHADER_TYPE_DOMAIN),           state_shader        }, WINED3D_GL_EXT_NONE             },
    { STATE_SHADER(WINED3D_SHADER_TYPE_GEOMETRY),         { STATE_SHADER(WINED3D_SHADER_TYPE_GEOMETRY),         state_shader        }, WINED3D_GL_EXT_NONE             },
    { STATE_SHADER(WINED3D_SHADER_TYPE_COMPUTE),          { STATE_SHADER(WINED3D_SHADER_TYPE_COMPUTE),          state_compute_shader}, WINED3D_GL_EXT_NONE             },
    {0 /* Terminate */,                                   { 0,                                                  0                   }, WINED3D_GL_EXT_NONE             },
};

static BOOL ffp_none_context_alloc(struct wined3d_context *context)
{
    return TRUE;
}

static void ffp_none_context_free(struct wined3d_context *context)
{
}

static void none_pipe_apply_draw_state(struct wined3d_context *context, const struct wined3d_state *state) {}

static void none_pipe_disable(const struct wined3d_context *context) {}

static void *none_alloc(const struct wined3d_shader_backend_ops *shader_backend, void *shader_priv)
{
    return shader_priv;
}

static void none_free(struct wined3d_device *device, struct wined3d_context *context) {}

static void vp_none_get_caps(const struct wined3d_adapter *adapter, struct wined3d_vertex_caps *caps)
{
    memset(caps, 0, sizeof(*caps));
}

static unsigned int vp_none_get_emul_mask(const struct wined3d_adapter *adapter)
{
    return 0;
}

const struct wined3d_vertex_pipe_ops none_vertex_pipe =
{
    .vp_apply_draw_state = none_pipe_apply_draw_state,
    .vp_disable = none_pipe_disable,
    .vp_get_caps = vp_none_get_caps,
    .vp_get_emul_mask = vp_none_get_emul_mask,
    .vp_alloc = none_alloc,
    .vp_free = none_free,
};

static void fp_none_get_caps(const struct wined3d_adapter *adapter, struct fragment_caps *caps)
{
    memset(caps, 0, sizeof(*caps));
}

static unsigned int fp_none_get_emul_mask(const struct wined3d_adapter *adapter)
{
    return 0;
}

static BOOL fp_none_color_fixup_supported(struct color_fixup_desc fixup)
{
    return is_identity_fixup(fixup);
}

const struct wined3d_fragment_pipe_ops none_fragment_pipe =
{
    .fp_apply_draw_state = none_pipe_apply_draw_state,
    .fp_disable = none_pipe_disable,
    .get_caps = fp_none_get_caps,
    .get_emul_mask = fp_none_get_emul_mask,
    .alloc_private = none_alloc,
    .free_private = none_free,
    .allocate_context_data = ffp_none_context_alloc,
    .free_context_data = ffp_none_context_free,
    .color_fixup_supported = fp_none_color_fixup_supported,
};

static unsigned int num_handlers(const APPLYSTATEFUNC *funcs)
{
    unsigned int i;
    for(i = 0; funcs[i]; i++);
    return i;
}

static void multistate_apply_2(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    context->device->multistate_funcs[state_id][0](context, state, state_id);
    context->device->multistate_funcs[state_id][1](context, state, state_id);
}

static void multistate_apply_3(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    context->device->multistate_funcs[state_id][0](context, state, state_id);
    context->device->multistate_funcs[state_id][1](context, state, state_id);
    context->device->multistate_funcs[state_id][2](context, state, state_id);
}

static void prune_invalid_states(struct wined3d_state_entry *state_table, const struct wined3d_d3d_info *d3d_info)
{
    unsigned int start, last, i;

    start = STATE_TEXTURESTAGE(d3d_info->ffp_fragment_caps.max_blend_stages, 0);
    last = STATE_TEXTURESTAGE(WINED3D_MAX_FFP_TEXTURES - 1, WINED3D_HIGHEST_TEXTURE_STATE);
    for (i = start; i <= last; ++i)
    {
        state_table[i].representative = 0;
        state_table[i].apply = state_undefined;
    }
}

static void validate_state_table(struct wined3d_state_entry *state_table)
{
    static const struct
    {
        DWORD first;
        DWORD last;
    }
    rs_holes[] =
    {
        {  1,   8},
        { 11,  14},
        { 16,  24},
        { 27,  27},
        { 30,  33},
        { 39,  40},
        { 42,  47},
        { 49, 135},
        {138, 139},
        {144, 144},
        {149, 150},
        {153, 153},
        {157, 160},
        {162, 165},
        {167, 193},
        {195, 209},
        {  0,   0},
    };
    static const unsigned int simple_states[] =
    {
        STATE_VDECL,
        STATE_STREAMSRC,
        STATE_INDEXBUFFER,
        STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),
        STATE_SHADER(WINED3D_SHADER_TYPE_HULL),
        STATE_SHADER(WINED3D_SHADER_TYPE_DOMAIN),
        STATE_SHADER(WINED3D_SHADER_TYPE_GEOMETRY),
        STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),
        STATE_SHADER(WINED3D_SHADER_TYPE_COMPUTE),
        STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_VERTEX),
        STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_HULL),
        STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_DOMAIN),
        STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_GEOMETRY),
        STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_PIXEL),
        STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_COMPUTE),
        STATE_COMPUTE_SHADER_RESOURCE_BINDING,
        STATE_GRAPHICS_SHADER_RESOURCE_BINDING,
        STATE_COMPUTE_UNORDERED_ACCESS_VIEW_BINDING,
        STATE_GRAPHICS_UNORDERED_ACCESS_VIEW_BINDING,
        STATE_VIEWPORT,
        STATE_SCISSORRECT,
        STATE_RASTERIZER,
        STATE_BASEVERTEXINDEX,
        STATE_FRAMEBUFFER,
        STATE_BLEND,
        STATE_BLEND_FACTOR,
        STATE_DEPTH_STENCIL,
        STATE_STENCIL_REF,
        STATE_DEPTH_BOUNDS,
    };
    unsigned int i, current;

    for (i = STATE_RENDER(1), current = 0; i <= STATE_RENDER(WINEHIGHEST_RENDER_STATE); ++i)
    {
        if (!rs_holes[current].first || i < STATE_RENDER(rs_holes[current].first))
        {
            if (!state_table[i].representative)
                ERR("State %s (%#x) should have a representative.\n", debug_d3dstate(i), i);
        }
        else if (state_table[i].representative)
            ERR("State %s (%#x) shouldn't have a representative.\n", debug_d3dstate(i), i);

        if (i == STATE_RENDER(rs_holes[current].last)) ++current;
    }

    for (i = 0; i < ARRAY_SIZE(simple_states); ++i)
    {
        if (!state_table[simple_states[i]].representative)
            ERR("State %s (%#x) should have a representative.\n",
                    debug_d3dstate(simple_states[i]), simple_states[i]);
    }

    for (i = 0; i < STATE_HIGHEST + 1; ++i)
    {
        unsigned int rep = state_table[i].representative;
        if (rep)
        {
            if (state_table[rep].representative != rep)
            {
                ERR("State %s (%#x) has invalid representative %s (%#x).\n",
                        debug_d3dstate(i), i, debug_d3dstate(rep), rep);
                state_table[i].representative = 0;
            }

            if (rep != i)
            {
                if (state_table[i].apply)
                    ERR("State %s (%#x) has both a handler and representative.\n", debug_d3dstate(i), i);
            }
            else if (!state_table[i].apply)
            {
                ERR("Self representing state %s (%#x) has no handler.\n", debug_d3dstate(i), i);
            }
        }
    }
}

HRESULT compile_state_table(struct wined3d_state_entry *state_table, APPLYSTATEFUNC **dev_multistate_funcs,
        const struct wined3d_d3d_info *d3d_info, const BOOL *supported_extensions,
        const struct wined3d_vertex_pipe_ops *vertex, const struct wined3d_fragment_pipe_ops *fragment,
        const struct wined3d_state_entry_template *misc)
{
    APPLYSTATEFUNC multistate_funcs[STATE_HIGHEST + 1][3];
    const struct wined3d_state_entry_template *cur;
    unsigned int i, type, handlers;
    BOOL set[STATE_HIGHEST + 1];

    memset(multistate_funcs, 0, sizeof(multistate_funcs));

    for (i = 0; i < STATE_HIGHEST + 1; ++i)
    {
        state_table[i].representative = 0;
        state_table[i].apply = state_undefined;
    }

    for (type = 0; type < 3; ++type)
    {
        /* This switch decides the order in which the states are applied */
        switch (type)
        {
            case 0: cur = misc; break;
            case 1: cur = fragment->states; break;
            case 2: cur = vertex->vp_states; break;
            default: cur = NULL; /* Stupid compiler */
        }
        if (!cur) continue;

        /* GL extension filtering should not prevent multiple handlers being applied from different
         * pipeline parts
         */
        memset(set, 0, sizeof(set));

        for (i = 0; cur[i].state; ++i)
        {
            APPLYSTATEFUNC *funcs_array;

            /* Only use the first matching state with the available extension from one template.
             * e.g.
             * {D3DRS_FOOBAR, {D3DRS_FOOBAR, func1}, XYZ_FANCY},
             * {D3DRS_FOOBAR, {D3DRS_FOOBAR, func2}, 0        }
             *
             * if GL_XYZ_fancy is supported, ignore the 2nd line
             */
            if (set[cur[i].state]) continue;
            /* Skip state lines depending on unsupported extensions */
            if (!supported_extensions[cur[i].extension]) continue;
            set[cur[i].state] = TRUE;
            /* In some cases having an extension means that nothing has to be
             * done for a state, e.g. if GL_ARB_texture_non_power_of_two is
             * supported, the texture coordinate fixup can be ignored. If the
             * apply function is used, mark the state set(done above) to prevent
             * applying later lines, but do not record anything in the state
             * table
             */
            if (!cur[i].content.representative) continue;

            handlers = num_handlers(multistate_funcs[cur[i].state]);
            multistate_funcs[cur[i].state][handlers] = cur[i].content.apply;
            switch (handlers)
            {
                case 0:
                    state_table[cur[i].state].apply = cur[i].content.apply;
                    break;
                case 1:
                    state_table[cur[i].state].apply = multistate_apply_2;
                    if (!(dev_multistate_funcs[cur[i].state] = calloc(2, sizeof(**dev_multistate_funcs))))
                        goto out_of_mem;

                    dev_multistate_funcs[cur[i].state][0] = multistate_funcs[cur[i].state][0];
                    dev_multistate_funcs[cur[i].state][1] = multistate_funcs[cur[i].state][1];
                    break;
                case 2:
                    state_table[cur[i].state].apply = multistate_apply_3;
                    if (!(funcs_array = realloc(dev_multistate_funcs[cur[i].state],
                            sizeof(**dev_multistate_funcs) * 3)))
                        goto out_of_mem;

                    dev_multistate_funcs[cur[i].state] = funcs_array;
                    dev_multistate_funcs[cur[i].state][2] = multistate_funcs[cur[i].state][2];
                    break;
                default:
                    ERR("Unexpected amount of state handlers for state %u: %u.\n",
                            cur[i].state, handlers + 1);
            }

            if (state_table[cur[i].state].representative
                    && state_table[cur[i].state].representative != cur[i].content.representative)
            {
                FIXME("State %s (%#x) has different representatives in different pipeline parts.\n",
                        debug_d3dstate(cur[i].state), cur[i].state);
            }
            state_table[cur[i].state].representative = cur[i].content.representative;
        }
    }

    prune_invalid_states(state_table, d3d_info);
    validate_state_table(state_table);

    return WINED3D_OK;

out_of_mem:
    for (i = 0; i <= STATE_HIGHEST; ++i)
    {
        free(dev_multistate_funcs[i]);
    }

    memset(dev_multistate_funcs, 0, (STATE_HIGHEST + 1) * sizeof(*dev_multistate_funcs));

    return E_OUTOFMEMORY;
}
