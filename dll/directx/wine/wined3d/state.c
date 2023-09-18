/*
 * Direct3D state management
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

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#ifdef HAVE_FLOAT_H
# include <float.h>
#endif

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

ULONG CDECL wined3d_blend_state_incref(struct wined3d_blend_state *state)
{
    ULONG refcount = InterlockedIncrement(&state->refcount);

    TRACE("%p increasing refcount to %u.\n", state, refcount);

    return refcount;
}

static void wined3d_blend_state_destroy_object(void *object)
{
    heap_free(object);
}

ULONG CDECL wined3d_blend_state_decref(struct wined3d_blend_state *state)
{
    ULONG refcount = InterlockedDecrement(&state->refcount);
    struct wined3d_device *device = state->device;

    TRACE("%p decreasing refcount to %u.\n", state, refcount);

    if (!refcount)
    {
        state->parent_ops->wined3d_object_destroyed(state->parent);
        wined3d_cs_destroy_object(device->cs, wined3d_blend_state_destroy_object, state);
    }

    return refcount;
}

void * CDECL wined3d_blend_state_get_parent(const struct wined3d_blend_state *state)
{
    TRACE("state %p.\n", state);

    return state->parent;
}

HRESULT CDECL wined3d_blend_state_create(struct wined3d_device *device,
        const struct wined3d_blend_state_desc *desc, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_blend_state **state)
{
    struct wined3d_blend_state *object;

    TRACE("device %p, desc %p, parent %p, parent_ops %p, state %p.\n",
            device, desc, parent, parent_ops, state);

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->refcount = 1;
    object->desc = *desc;
    object->parent = parent;
    object->parent_ops = parent_ops;
    object->device = device;

    TRACE("Created blend state %p.\n", object);
    *state = object;

    return WINED3D_OK;
}

ULONG CDECL wined3d_rasterizer_state_incref(struct wined3d_rasterizer_state *state)
{
    ULONG refcount = InterlockedIncrement(&state->refcount);

    TRACE("%p increasing refcount to %u.\n", state, refcount);

    return refcount;
}

static void wined3d_rasterizer_state_destroy_object(void *object)
{
    heap_free(object);
}

ULONG CDECL wined3d_rasterizer_state_decref(struct wined3d_rasterizer_state *state)
{
    ULONG refcount = InterlockedDecrement(&state->refcount);
    struct wined3d_device *device = state->device;

    TRACE("%p decreasing refcount to %u.\n", state, refcount);

    if (!refcount)
    {
        state->parent_ops->wined3d_object_destroyed(state->parent);
        wined3d_cs_destroy_object(device->cs, wined3d_rasterizer_state_destroy_object, state);
    }

    return refcount;
}

void * CDECL wined3d_rasterizer_state_get_parent(const struct wined3d_rasterizer_state *state)
{
    TRACE("rasterizer_state %p.\n", state);

    return state->parent;
}

HRESULT CDECL wined3d_rasterizer_state_create(struct wined3d_device *device,
        const struct wined3d_rasterizer_state_desc *desc, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_rasterizer_state **state)
{
    struct wined3d_rasterizer_state *object;

    TRACE("device %p, desc %p, parent %p, parent_ops %p, state %p.\n",
            device, desc, parent, parent_ops, state);

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->refcount = 1;
    object->desc = *desc;
    object->parent = parent;
    object->parent_ops = parent_ops;
    object->device = device;

    TRACE("Created rasterizer state %p.\n", object);
    *state = object;

    return WINED3D_OK;
}

/* Context activation for state handler is done by the caller. */

static void state_undefined(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    ERR("Undefined state %s (%#x).\n", debug_d3dstate(state_id), state_id);
}

void state_nop(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    TRACE("%s: nop in current pipe config.\n", debug_d3dstate(state_id));
}

static void state_fillmode(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    enum wined3d_fill_mode mode = state->render_states[WINED3D_RS_FILLMODE];

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

static void state_lighting(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;

    /* Lighting is not enabled if transformed vertices are drawn, but lighting
     * does not affect the stream sources, so it is not grouped for
     * performance reasons. This state reads the decoded vertex declaration,
     * so if it is dirty don't do anything. The vertex declaration applying
     * function calls this function for updating. */
    if (isStateDirty(context, STATE_VDECL))
        return;

    if (state->render_states[WINED3D_RS_LIGHTING]
            && !context->stream_info.position_transformed)
    {
        gl_info->gl_ops.gl.p_glEnable(GL_LIGHTING);
        checkGLcall("glEnable GL_LIGHTING");
    }
    else
    {
        gl_info->gl_ops.gl.p_glDisable(GL_LIGHTING);
        checkGLcall("glDisable GL_LIGHTING");
    }
}

static void state_zenable(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    enum wined3d_depth_buffer_type zenable = state->render_states[WINED3D_RS_ZENABLE];
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;

    /* No z test without depth stencil buffers */
    if (!state->fb->depth_stencil)
    {
        TRACE("No Z buffer - disabling depth test\n");
        zenable = WINED3D_ZB_FALSE;
    }

    switch (zenable)
    {
        case WINED3D_ZB_FALSE:
            gl_info->gl_ops.gl.p_glDisable(GL_DEPTH_TEST);
            checkGLcall("glDisable GL_DEPTH_TEST");
            break;
        case WINED3D_ZB_TRUE:
            gl_info->gl_ops.gl.p_glEnable(GL_DEPTH_TEST);
            checkGLcall("glEnable GL_DEPTH_TEST");
            break;
        case WINED3D_ZB_USEW:
            gl_info->gl_ops.gl.p_glEnable(GL_DEPTH_TEST);
            checkGLcall("glEnable GL_DEPTH_TEST");
            FIXME("W buffer is not well handled\n");
            break;
        default:
            FIXME("Unrecognized depth buffer type %#x.\n", zenable);
            break;
    }

    if (context->last_was_rhw && !isStateDirty(context, STATE_TRANSFORM(WINED3D_TS_PROJECTION)))
        context_apply_state(context, state, STATE_TRANSFORM(WINED3D_TS_PROJECTION));
}

static void state_cullmode(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;

    /* glFrontFace() is set in context.c at context init and on an
     * offscreen / onscreen rendering switch. */
    switch (state->render_states[WINED3D_RS_CULLMODE])
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
            FIXME("Unrecognized cull mode %#x.\n",
                    state->render_states[WINED3D_RS_CULLMODE]);
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

static void state_zwriteenable(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;

    if (state->render_states[WINED3D_RS_ZWRITEENABLE])
    {
        gl_info->gl_ops.gl.p_glDepthMask(1);
        checkGLcall("glDepthMask(1)");
    }
    else
    {
        gl_info->gl_ops.gl.p_glDepthMask(0);
        checkGLcall("glDepthMask(0)");
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

static void state_zfunc(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    GLenum depth_func = wined3d_gl_compare_func(state->render_states[WINED3D_RS_ZFUNC]);
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;

    if (!depth_func) return;

    gl_info->gl_ops.gl.p_glDepthFunc(depth_func);
    checkGLcall("glDepthFunc");
}

static void state_ambient(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    struct wined3d_color color;

    wined3d_color_from_d3dcolor(&color, state->render_states[WINED3D_RS_AMBIENT]);
    TRACE("Setting ambient to %s.\n", debug_color(&color));
    gl_info->gl_ops.gl.p_glLightModelfv(GL_LIGHT_MODEL_AMBIENT, &color.r);
    checkGLcall("glLightModel for MODEL_AMBIENT");
}

static void state_blendop_w(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    WARN("Unsupported in local OpenGL implementation: glBlendEquation\n");
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

static void state_blendop(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    GLenum blend_equation_alpha = GL_FUNC_ADD_EXT;
    GLenum blend_equation = GL_FUNC_ADD_EXT;

    /* BLENDOPALPHA requires GL_EXT_blend_equation_separate, so make sure it is around */
    if (state->render_states[WINED3D_RS_BLENDOPALPHA]
            && !gl_info->supported[EXT_BLEND_EQUATION_SEPARATE])
    {
        WARN("Unsupported in local OpenGL implementation: glBlendEquationSeparate.\n");
        return;
    }

    blend_equation = gl_blend_op(gl_info, state->render_states[WINED3D_RS_BLENDOP]);
    blend_equation_alpha = gl_blend_op(gl_info, state->render_states[WINED3D_RS_BLENDOPALPHA]);
    TRACE("blend_equation %#x, blend_equation_alpha %#x.\n", blend_equation, blend_equation_alpha);

    if (state->render_states[WINED3D_RS_SEPARATEALPHABLENDENABLE])
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

static void state_blend(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    const struct wined3d_format *rt_format;
    GLenum src_blend, dst_blend;
    unsigned int rt_fmt_flags;
    BOOL enable_dual_blend;
    BOOL enable_blend;

    enable_blend = state->fb->render_targets[0] && state->render_states[WINED3D_RS_ALPHABLENDENABLE];
    enable_dual_blend = wined3d_dualblend_enabled(state, gl_info);

    if (enable_blend && !enable_dual_blend)
    {
        rt_fmt_flags = state->fb->render_targets[0]->format_flags;

        /* Disable blending in all cases even without pixelshaders.
         * With blending on we could face a big performance penalty.
         * The d3d9 visual test confirms the behavior. */
        if (context->render_offscreen && !(rt_fmt_flags & WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING))
            enable_blend = FALSE;
    }

    /* Dual state blending changes the assignment of the output variables */
    if (context->last_was_dual_blend != enable_dual_blend)
    {
        context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_PIXEL;
        context->last_was_dual_blend = enable_dual_blend;
    }

    if (!enable_blend)
    {
        gl_info->gl_ops.gl.p_glDisable(GL_BLEND);
        checkGLcall("glDisable(GL_BLEND)");
        return;
    }

    gl_info->gl_ops.gl.p_glEnable(GL_BLEND);
    checkGLcall("glEnable(GL_BLEND)");

    rt_format = state->fb->render_targets[0]->format;
    gl_blend_from_d3d(&src_blend, &dst_blend,
            state->render_states[WINED3D_RS_SRCBLEND],
            state->render_states[WINED3D_RS_DESTBLEND], rt_format);

    /* Re-apply BLENDOP(ALPHA) because of a possible SEPARATEALPHABLENDENABLE change */
    if (!isStateDirty(context, STATE_RENDER(WINED3D_RS_BLENDOP)))
        state_blendop(context, state, STATE_RENDER(WINED3D_RS_BLENDOPALPHA));

    if (state->render_states[WINED3D_RS_SEPARATEALPHABLENDENABLE])
    {
        GLenum src_blend_alpha, dst_blend_alpha;

        /* Separate alpha blending requires GL_EXT_blend_function_separate, so make sure it is around */
        if (!gl_info->supported[EXT_BLEND_FUNC_SEPARATE])
        {
            WARN("Unsupported in local OpenGL implementation: glBlendFuncSeparate.\n");
            return;
        }

        gl_blend_from_d3d(&src_blend_alpha, &dst_blend_alpha,
                state->render_states[WINED3D_RS_SRCBLENDALPHA],
                state->render_states[WINED3D_RS_DESTBLENDALPHA], rt_format);

        GL_EXTCALL(glBlendFuncSeparate(src_blend, dst_blend, src_blend_alpha, dst_blend_alpha));
        checkGLcall("glBlendFuncSeparate");
    }
    else
    {
        TRACE("glBlendFunc src=%x, dst=%x.\n", src_blend, dst_blend);
        gl_info->gl_ops.gl.p_glBlendFunc(src_blend, dst_blend);
        checkGLcall("glBlendFunc");
    }

    /* Colorkey fixup for stage 0 alphaop depends on
     * WINED3D_RS_ALPHABLENDENABLE state, so it may need updating. */
    if (state->render_states[WINED3D_RS_COLORKEYENABLE])
        context_apply_state(context, state, STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_OP));
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

static void state_blend_object(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    BOOL alpha_to_coverage = FALSE;

    if (!gl_info->supported[ARB_MULTISAMPLE])
        return;

    if (state->blend_state)
    {
        struct wined3d_blend_state_desc *desc = &state->blend_state->desc;
        alpha_to_coverage = desc->alpha_to_coverage;
    }

    if (alpha_to_coverage)
        gl_info->gl_ops.gl.p_glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    else
        gl_info->gl_ops.gl.p_glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

    checkGLcall("blend state");
}

void state_alpha_test(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    int glParm = 0;
    float ref;
    BOOL enable_ckey = FALSE;

    TRACE("context %p, state %p, state_id %#x.\n", context, state, state_id);

    /* Find out if the texture on the first stage has a ckey set. The alpha
     * state func reads the texture settings, even though alpha and texture
     * are not grouped together. This is to avoid making a huge alpha +
     * texture + texture stage + ckey block due to the hardly used
     * WINED3D_RS_COLORKEYENABLE state(which is d3d <= 3 only). The texture
     * function will call alpha in case it finds some texture + colorkeyenable
     * combination which needs extra care. */
    if (state->textures[0] && (state->textures[0]->async.color_key_flags & WINED3D_CKEY_SRC_BLT))
        enable_ckey = TRUE;

    if (enable_ckey || context->last_was_ckey)
        context_apply_state(context, state, STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_OP));
    context->last_was_ckey = enable_ckey;

    if (state->render_states[WINED3D_RS_ALPHATESTENABLE]
            || (state->render_states[WINED3D_RS_COLORKEYENABLE] && enable_ckey))
    {
        gl_info->gl_ops.gl.p_glEnable(GL_ALPHA_TEST);
        checkGLcall("glEnable GL_ALPHA_TEST");
    }
    else
    {
        gl_info->gl_ops.gl.p_glDisable(GL_ALPHA_TEST);
        checkGLcall("glDisable GL_ALPHA_TEST");
        /* Alpha test is disabled, don't bother setting the params - it will happen on the next
         * enable call
         */
        return;
    }

    if (state->render_states[WINED3D_RS_COLORKEYENABLE] && enable_ckey)
    {
        glParm = GL_NOTEQUAL;
        ref = 0.0f;
    }
    else
    {
        ref = wined3d_alpha_ref(state);
        glParm = wined3d_gl_compare_func(state->render_states[WINED3D_RS_ALPHAFUNC]);
    }
    if (glParm)
    {
        gl_info->gl_ops.gl.p_glAlphaFunc(glParm, ref);
        checkGLcall("glAlphaFunc");
    }
}

void state_clipping(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    uint32_t enable_mask;

    if (use_vs(state) && !context->d3d_info->vs_clipping)
    {
        static BOOL warned;

        /* The OpenGL spec says that clipping planes are disabled when using
         * shaders. Direct3D planes aren't, so that is an issue. The MacOS ATI
         * driver keeps clipping planes activated with shaders in some
         * conditions I got sick of tracking down. The shader state handler
         * disables all clip planes because of that - don't do anything here
         * and keep them disabled. */
        if (state->render_states[WINED3D_RS_CLIPPLANEENABLE] && !warned++)
            FIXME("Clipping not supported with vertex shaders.\n");
        return;
    }

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

static void state_specularenable(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;

    /* Originally this used glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR)
     * and (GL_LIGHT_MODEL_COLOR_CONTROL,GL_SINGLE_COLOR) to swap between enabled/disabled
     * specular color. This is wrong:
     * Separate specular color means the specular colour is maintained separately, whereas
     * single color means it is merged in. However in both cases they are being used to
     * some extent.
     * To disable specular color, set it explicitly to black and turn off GL_COLOR_SUM_EXT
     * NOTE: If not supported don't give FIXMEs the impact is really minimal and very few people are
     * running 1.4 yet!
     *
     *
     * If register combiners are enabled, enabling / disabling GL_COLOR_SUM has no effect.
     * Instead, we need to setup the FinalCombiner properly.
     *
     * The default setup for the FinalCombiner is:
     *
     * <variable>       <input>                             <mapping>               <usage>
     * GL_VARIABLE_A_NV GL_FOG,                             GL_UNSIGNED_IDENTITY_NV GL_ALPHA
     * GL_VARIABLE_B_NV GL_SPARE0_PLUS_SECONDARY_COLOR_NV   GL_UNSIGNED_IDENTITY_NV GL_RGB
     * GL_VARIABLE_C_NV GL_FOG                              GL_UNSIGNED_IDENTITY_NV GL_RGB
     * GL_VARIABLE_D_NV GL_ZERO                             GL_UNSIGNED_IDENTITY_NV GL_RGB
     * GL_VARIABLE_E_NV GL_ZERO                             GL_UNSIGNED_IDENTITY_NV GL_RGB
     * GL_VARIABLE_F_NV GL_ZERO                             GL_UNSIGNED_IDENTITY_NV GL_RGB
     * GL_VARIABLE_G_NV GL_SPARE0_NV                        GL_UNSIGNED_IDENTITY_NV GL_ALPHA
     *
     * That's pretty much fine as it is, except for variable B, which needs to take
     * either GL_SPARE0_PLUS_SECONDARY_COLOR_NV or GL_SPARE0_NV, depending on
     * whether WINED3D_RS_SPECULARENABLE is enabled or not.
     */

    TRACE("Setting specular enable state and materials\n");
    if (state->render_states[WINED3D_RS_SPECULARENABLE])
    {
        gl_info->gl_ops.gl.p_glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (float *)&state->material.specular);
        checkGLcall("glMaterialfv");

        if (state->material.power > gl_info->limits.shininess)
        {
            /* glMaterialf man page says that the material says that GL_SHININESS must be between 0.0
             * and 128.0, although in d3d neither -1 nor 129 produce an error. GL_NV_max_light_exponent
             * allows bigger values. If the extension is supported, gl_info->limits.shininess contains the
             * value reported by the extension, otherwise 128. For values > gl_info->limits.shininess clamp
             * them, it should be safe to do so without major visual distortions.
             */
            WARN("Material power = %.8e, limit %.8e\n", state->material.power, gl_info->limits.shininess);
            gl_info->gl_ops.gl.p_glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, gl_info->limits.shininess);
        }
        else
        {
            gl_info->gl_ops.gl.p_glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, state->material.power);
        }
        checkGLcall("glMaterialf(GL_SHININESS)");

        if (gl_info->supported[EXT_SECONDARY_COLOR])
            gl_info->gl_ops.gl.p_glEnable(GL_COLOR_SUM_EXT);
        else
            TRACE("Specular colors cannot be enabled in this version of opengl\n");
        checkGLcall("glEnable(GL_COLOR_SUM)");

        if (gl_info->supported[NV_REGISTER_COMBINERS])
        {
            GL_EXTCALL(glFinalCombinerInputNV(GL_VARIABLE_B_NV, GL_SPARE0_PLUS_SECONDARY_COLOR_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB));
            checkGLcall("glFinalCombinerInputNV()");
        }
    } else {
        static const GLfloat black[] = {0.0f, 0.0f, 0.0f, 0.0f};

        /* for the case of enabled lighting: */
        gl_info->gl_ops.gl.p_glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &black[0]);
        checkGLcall("glMaterialfv");

        /* for the case of disabled lighting: */
        if (gl_info->supported[EXT_SECONDARY_COLOR])
            gl_info->gl_ops.gl.p_glDisable(GL_COLOR_SUM_EXT);
        else
            TRACE("Specular colors cannot be disabled in this version of opengl\n");
        checkGLcall("glDisable(GL_COLOR_SUM)");

        if (gl_info->supported[NV_REGISTER_COMBINERS])
        {
            GL_EXTCALL(glFinalCombinerInputNV(GL_VARIABLE_B_NV, GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB));
            checkGLcall("glFinalCombinerInputNV()");
        }
    }

    TRACE("diffuse %s\n", debug_color(&state->material.diffuse));
    TRACE("ambient %s\n", debug_color(&state->material.ambient));
    TRACE("specular %s\n", debug_color(&state->material.specular));
    TRACE("emissive %s\n", debug_color(&state->material.emissive));

    gl_info->gl_ops.gl.p_glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, (float *)&state->material.ambient);
    checkGLcall("glMaterialfv(GL_AMBIENT)");
    gl_info->gl_ops.gl.p_glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, (float *)&state->material.diffuse);
    checkGLcall("glMaterialfv(GL_DIFFUSE)");
    gl_info->gl_ops.gl.p_glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, (float *)&state->material.emissive);
    checkGLcall("glMaterialfv(GL_EMISSION)");
}

static void state_texfactor(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_color color;
    unsigned int i;

    /* Note the texture color applies to all textures whereas
     * GL_TEXTURE_ENV_COLOR applies to active only. */
    wined3d_color_from_d3dcolor(&color, state->render_states[WINED3D_RS_TEXTUREFACTOR]);

    /* And now the default texture color as well */
    for (i = 0; i < context->d3d_info->limits.ffp_blend_stages; ++i)
    {
        /* Note the WINED3D_RS value applies to all textures, but GL has one
         * per texture, so apply it now ready to be used! */
        wined3d_context_gl_active_texture(context_gl, gl_info, i);

        gl_info->gl_ops.gl.p_glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, &color.r);
        checkGLcall("glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);");
    }
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

static void state_stencil(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    DWORD onesided_enable;
    DWORD twosided_enable;
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
    if (!state->fb->depth_stencil)
    {
        gl_info->gl_ops.gl.p_glDisable(GL_STENCIL_TEST);
        checkGLcall("glDisable GL_STENCIL_TEST");
        return;
    }

    onesided_enable = state->render_states[WINED3D_RS_STENCILENABLE];
    twosided_enable = state->render_states[WINED3D_RS_TWOSIDEDSTENCILMODE];
    if (!(func = wined3d_gl_compare_func(state->render_states[WINED3D_RS_STENCILFUNC])))
        func = GL_ALWAYS;
    if (!(func_back = wined3d_gl_compare_func(state->render_states[WINED3D_RS_BACK_STENCILFUNC])))
        func_back = GL_ALWAYS;
    mask = state->render_states[WINED3D_RS_STENCILMASK];
    ref = state->render_states[WINED3D_RS_STENCILREF] & ((1 << state->fb->depth_stencil->format->stencil_size) - 1);
    stencilFail = gl_stencil_op(state->render_states[WINED3D_RS_STENCILFAIL]);
    depthFail = gl_stencil_op(state->render_states[WINED3D_RS_STENCILZFAIL]);
    stencilPass = gl_stencil_op(state->render_states[WINED3D_RS_STENCILPASS]);
    stencilFail_back = gl_stencil_op(state->render_states[WINED3D_RS_BACK_STENCILFAIL]);
    depthFail_back = gl_stencil_op(state->render_states[WINED3D_RS_BACK_STENCILZFAIL]);
    stencilPass_back = gl_stencil_op(state->render_states[WINED3D_RS_BACK_STENCILPASS]);

    TRACE("(onesided %d, twosided %d, ref %x, mask %x, "
            "GL_FRONT: func: %x, fail %x, zfail %x, zpass %x "
            "GL_BACK: func: %x, fail %x, zfail %x, zpass %x)\n",
            onesided_enable, twosided_enable, ref, mask,
            func, stencilFail, depthFail, stencilPass,
            func_back, stencilFail_back, depthFail_back, stencilPass_back);

    if (twosided_enable && onesided_enable)
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
    else if(onesided_enable)
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
    else
    {
        gl_info->gl_ops.gl.p_glDisable(GL_STENCIL_TEST);
        checkGLcall("glDisable GL_STENCIL_TEST");
    }
}

static void state_stencilwrite2s_ext(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    DWORD mask = state->fb->depth_stencil ? state->render_states[WINED3D_RS_STENCILWRITEMASK] : 0;
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;

    GL_EXTCALL(glActiveStencilFaceEXT(GL_BACK));
    checkGLcall("glActiveStencilFaceEXT(GL_BACK)");
    gl_info->gl_ops.gl.p_glStencilMask(mask);
    checkGLcall("glStencilMask");
    GL_EXTCALL(glActiveStencilFaceEXT(GL_FRONT));
    checkGLcall("glActiveStencilFaceEXT(GL_FRONT)");
    gl_info->gl_ops.gl.p_glStencilMask(mask);
}

static void state_stencilwrite(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    DWORD mask = state->fb->depth_stencil ? state->render_states[WINED3D_RS_STENCILWRITEMASK] : 0;
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;

    gl_info->gl_ops.gl.p_glStencilMask(mask);
    checkGLcall("glStencilMask");
}

static void state_fog_vertexpart(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;

    TRACE("context %p, state %p, state_id %#x.\n", context, state, state_id);

    if (!state->render_states[WINED3D_RS_FOGENABLE])
        return;

    /* Table fog on: Never use fog coords, and use per-fragment fog */
    if (state->render_states[WINED3D_RS_FOGTABLEMODE] != WINED3D_FOG_NONE)
    {
        gl_info->gl_ops.gl.p_glHint(GL_FOG_HINT, GL_NICEST);
        if (context->fog_coord)
        {
            gl_info->gl_ops.gl.p_glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT);
            checkGLcall("glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT)");
            context->fog_coord = FALSE;
        }

        /* Range fog is only used with per-vertex fog in d3d */
        if (gl_info->supported[NV_FOG_DISTANCE])
        {
            gl_info->gl_ops.gl.p_glFogi(GL_FOG_DISTANCE_MODE_NV, GL_EYE_PLANE_ABSOLUTE_NV);
            checkGLcall("glFogi(GL_FOG_DISTANCE_MODE_NV, GL_EYE_PLANE_ABSOLUTE_NV)");
        }
        return;
    }

    /* Otherwise use per-vertex fog in any case */
    gl_info->gl_ops.gl.p_glHint(GL_FOG_HINT, GL_FASTEST);

    if (state->render_states[WINED3D_RS_FOGVERTEXMODE] == WINED3D_FOG_NONE || context->last_was_rhw)
    {
        /* No fog at all, or transformed vertices: Use fog coord */
        if (!context->fog_coord)
        {
            gl_info->gl_ops.gl.p_glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT);
            checkGLcall("glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT)");
            context->fog_coord = TRUE;
        }
    }
    else
    {
        /* Otherwise, use the fragment depth */
        if (context->fog_coord)
        {
            gl_info->gl_ops.gl.p_glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT);
            checkGLcall("glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT)");
            context->fog_coord = FALSE;
        }

        if (state->render_states[WINED3D_RS_RANGEFOGENABLE])
        {
            if (gl_info->supported[NV_FOG_DISTANCE])
            {
                gl_info->gl_ops.gl.p_glFogi(GL_FOG_DISTANCE_MODE_NV, GL_EYE_RADIAL_NV);
                checkGLcall("glFogi(GL_FOG_DISTANCE_MODE_NV, GL_EYE_RADIAL_NV)");
            }
            else
            {
                WARN("Range fog enabled, but not supported by this GL implementation.\n");
            }
        }
        else if (gl_info->supported[NV_FOG_DISTANCE])
        {
            gl_info->gl_ops.gl.p_glFogi(GL_FOG_DISTANCE_MODE_NV, GL_EYE_PLANE_ABSOLUTE_NV);
            checkGLcall("glFogi(GL_FOG_DISTANCE_MODE_NV, GL_EYE_PLANE_ABSOLUTE_NV)");
        }
    }
}

void state_fogstartend(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    float fogstart, fogend;

    get_fog_start_end(context, state, &fogstart, &fogend);

    gl_info->gl_ops.gl.p_glFogf(GL_FOG_START, fogstart);
    checkGLcall("glFogf(GL_FOG_START, fogstart)");
    TRACE("Fog Start == %f\n", fogstart);

    gl_info->gl_ops.gl.p_glFogf(GL_FOG_END, fogend);
    checkGLcall("glFogf(GL_FOG_END, fogend)");
    TRACE("Fog End == %f\n", fogend);
}

void state_fog_fragpart(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    enum fogsource new_source;
    DWORD fogstart = state->render_states[WINED3D_RS_FOGSTART];
    DWORD fogend = state->render_states[WINED3D_RS_FOGEND];

    TRACE("context %p, state %p, state_id %#x.\n", context, state, state_id);

    if (!state->render_states[WINED3D_RS_FOGENABLE])
    {
        /* No fog? Disable it, and we're done :-) */
        gl_info->p_glDisableWINE(GL_FOG);
        checkGLcall("glDisable GL_FOG");
        return;
    }

    /* Fog Rules:
     *
     * With fixed function vertex processing, Direct3D knows 2 different fog input sources.
     * It can use the Z value of the vertex, or the alpha component of the specular color.
     * This depends on the fog vertex, fog table and the vertex declaration. If the Z value
     * is used, fogstart, fogend and the equation type are used, otherwise linear fog with
     * start = 255, end = 0 is used. Obviously the msdn is not very clear on that.
     *
     * FOGTABLEMODE != NONE:
     *  The Z value is used, with the equation specified, no matter what vertex type.
     *
     * FOGTABLEMODE == NONE, FOGVERTEXMODE != NONE, untransformed:
     *  Per vertex fog is calculated using the specified fog equation and the parameters
     *
     * FOGTABLEMODE == NONE, FOGVERTEXMODE != NONE, transformed, OR
     * FOGTABLEMODE == NONE, FOGVERTEXMODE == NONE, untransformed:
     *  Linear fog with start = 255.0, end = 0.0, input comes from the specular color
     *
     *
     * Rules for vertex fog with shaders:
     *
     * When mixing fixed function functionality with the programmable pipeline, D3D expects
     * the fog computation to happen during transformation while openGL expects it to happen
     * during rasterization. Also, prior to pixel shader 3.0 D3D handles fog blending after
     * the pixel shader while openGL always expects the pixel shader to handle the blending.
     * To solve this problem, WineD3D does:
     * 1) implement a linear fog equation and fog blending at the end of every pre 3.0 pixel
     * shader,
     * and 2) disables the fog computation (in either the fixed function or programmable
     * rasterizer) if using a vertex program.
     *
     * D3D shaders can provide an explicit fog coordinate. This fog coordinate is used with
     * D3DRS_FOGTABLEMODE==D3DFOG_NONE. The FOGVERTEXMODE is ignored, d3d always uses linear
     * fog with start=1.0 and end=0.0 in this case. This is similar to fog coordinates in
     * the specular color, a vertex shader counts as pretransformed geometry in this case.
     * There are some GL differences between specular fog coords and vertex shaders though.
     *
     * With table fog the vertex shader fog coordinate is ignored.
     *
     * If a fogtablemode and a fogvertexmode are specified, table fog is applied (with or
     * without shaders).
     */

    /* DX 7 sdk: "If both render states(vertex and table fog) are set to valid modes,
     * the system will apply only pixel(=table) fog effects."
     */
    if (state->render_states[WINED3D_RS_FOGTABLEMODE] == WINED3D_FOG_NONE)
    {
        if (use_vs(state))
        {
            gl_info->gl_ops.gl.p_glFogi(GL_FOG_MODE, GL_LINEAR);
            checkGLcall("glFogi(GL_FOG_MODE, GL_LINEAR)");
            new_source = FOGSOURCE_VS;
        }
        else
        {
            switch (state->render_states[WINED3D_RS_FOGVERTEXMODE])
            {
                /* If processed vertices are used, fall through to the NONE case */
                case WINED3D_FOG_EXP:
                    if (!context->last_was_rhw)
                    {
                        gl_info->gl_ops.gl.p_glFogi(GL_FOG_MODE, GL_EXP);
                        checkGLcall("glFogi(GL_FOG_MODE, GL_EXP)");
                        new_source = FOGSOURCE_FFP;
                        break;
                    }
                    /* drop through */

                case WINED3D_FOG_EXP2:
                    if (!context->last_was_rhw)
                    {
                        gl_info->gl_ops.gl.p_glFogi(GL_FOG_MODE, GL_EXP2);
                        checkGLcall("glFogi(GL_FOG_MODE, GL_EXP2)");
                        new_source = FOGSOURCE_FFP;
                        break;
                    }
                    /* drop through */

                case WINED3D_FOG_LINEAR:
                    if (!context->last_was_rhw)
                    {
                        gl_info->gl_ops.gl.p_glFogi(GL_FOG_MODE, GL_LINEAR);
                        checkGLcall("glFogi(GL_FOG_MODE, GL_LINEAR)");
                        new_source = FOGSOURCE_FFP;
                        break;
                    }
                    /* drop through */

                case WINED3D_FOG_NONE:
                    /* Both are none? According to msdn the alpha channel of
                     * the specular colour contains a fog factor. Set it in
                     * draw_primitive_immediate_mode(). Same happens with
                     * vertex fog on transformed vertices. */
                    new_source = FOGSOURCE_COORD;
                    gl_info->gl_ops.gl.p_glFogi(GL_FOG_MODE, GL_LINEAR);
                    checkGLcall("glFogi(GL_FOG_MODE, GL_LINEAR)");
                    break;

                default:
                    FIXME("Unexpected WINED3D_RS_FOGVERTEXMODE %#x.\n",
                            state->render_states[WINED3D_RS_FOGVERTEXMODE]);
                    new_source = FOGSOURCE_FFP; /* Make the compiler happy */
            }
        }
    } else {
        new_source = FOGSOURCE_FFP;

        switch (state->render_states[WINED3D_RS_FOGTABLEMODE])
        {
            case WINED3D_FOG_EXP:
                gl_info->gl_ops.gl.p_glFogi(GL_FOG_MODE, GL_EXP);
                checkGLcall("glFogi(GL_FOG_MODE, GL_EXP)");
                break;

            case WINED3D_FOG_EXP2:
                gl_info->gl_ops.gl.p_glFogi(GL_FOG_MODE, GL_EXP2);
                checkGLcall("glFogi(GL_FOG_MODE, GL_EXP2)");
                break;

            case WINED3D_FOG_LINEAR:
                gl_info->gl_ops.gl.p_glFogi(GL_FOG_MODE, GL_LINEAR);
                checkGLcall("glFogi(GL_FOG_MODE, GL_LINEAR)");
                break;

            case WINED3D_FOG_NONE:   /* Won't happen */
            default:
                FIXME("Unexpected WINED3D_RS_FOGTABLEMODE %#x.\n",
                        state->render_states[WINED3D_RS_FOGTABLEMODE]);
        }
    }

    gl_info->p_glEnableWINE(GL_FOG);
    checkGLcall("glEnable GL_FOG");
    if (new_source != context->fog_source || fogstart == fogend)
    {
        context->fog_source = new_source;
        state_fogstartend(context, state, STATE_RENDER(WINED3D_RS_FOGSTART));
    }
}

void state_fogcolor(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    struct wined3d_color color;

    wined3d_color_from_d3dcolor(&color, state->render_states[WINED3D_RS_FOGCOLOR]);
    gl_info->gl_ops.gl.p_glFogfv(GL_FOG_COLOR, &color.r);
    checkGLcall("glFog GL_FOG_COLOR");
}

void state_fogdensity(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    union {
        DWORD d;
        float f;
    } tmpvalue;

    tmpvalue.d = state->render_states[WINED3D_RS_FOGDENSITY];
    gl_info->gl_ops.gl.p_glFogfv(GL_FOG_DENSITY, &tmpvalue.f);
    checkGLcall("glFogf(GL_FOG_DENSITY, (float) Value)");
}

static void state_colormat(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    GLenum Parm = 0;

    /* Depends on the decoded vertex declaration to read the existence of
     * diffuse data. The vertex declaration will call this function if the
     * fixed function pipeline is used. */
    if (isStateDirty(&context_gl->c, STATE_VDECL))
        return;

    context_gl->untracked_material_count = 0;
    if ((context_gl->c.stream_info.use_map & (1u << WINED3D_FFP_DIFFUSE))
            && state->render_states[WINED3D_RS_COLORVERTEX])
    {
        TRACE("diff %d, amb %d, emis %d, spec %d\n",
                state->render_states[WINED3D_RS_DIFFUSEMATERIALSOURCE],
                state->render_states[WINED3D_RS_AMBIENTMATERIALSOURCE],
                state->render_states[WINED3D_RS_EMISSIVEMATERIALSOURCE],
                state->render_states[WINED3D_RS_SPECULARMATERIALSOURCE]);

        if (state->render_states[WINED3D_RS_DIFFUSEMATERIALSOURCE] == WINED3D_MCS_COLOR1)
        {
            if (state->render_states[WINED3D_RS_AMBIENTMATERIALSOURCE] == WINED3D_MCS_COLOR1)
                Parm = GL_AMBIENT_AND_DIFFUSE;
            else
                Parm = GL_DIFFUSE;
            if (state->render_states[WINED3D_RS_EMISSIVEMATERIALSOURCE] == WINED3D_MCS_COLOR1)
                context_gl->untracked_materials[context_gl->untracked_material_count++] = GL_EMISSION;
            if (state->render_states[WINED3D_RS_SPECULARMATERIALSOURCE] == WINED3D_MCS_COLOR1)
                context_gl->untracked_materials[context_gl->untracked_material_count++] = GL_SPECULAR;
        }
        else if (state->render_states[WINED3D_RS_AMBIENTMATERIALSOURCE] == WINED3D_MCS_COLOR1)
        {
            Parm = GL_AMBIENT;
            if (state->render_states[WINED3D_RS_EMISSIVEMATERIALSOURCE] == WINED3D_MCS_COLOR1)
                context_gl->untracked_materials[context_gl->untracked_material_count++] = GL_EMISSION;
            if (state->render_states[WINED3D_RS_SPECULARMATERIALSOURCE] == WINED3D_MCS_COLOR1)
                context_gl->untracked_materials[context_gl->untracked_material_count++] = GL_SPECULAR;
        }
        else if (state->render_states[WINED3D_RS_EMISSIVEMATERIALSOURCE] == WINED3D_MCS_COLOR1)
        {
            Parm = GL_EMISSION;
            if (state->render_states[WINED3D_RS_SPECULARMATERIALSOURCE] == WINED3D_MCS_COLOR1)
                context_gl->untracked_materials[context_gl->untracked_material_count++] = GL_SPECULAR;
        }
        else if (state->render_states[WINED3D_RS_SPECULARMATERIALSOURCE] == WINED3D_MCS_COLOR1)
        {
            Parm = GL_SPECULAR;
        }
    }

    /* Nothing changed, return. */
    if (Parm == context_gl->tracking_parm)
        return;

    if (!Parm)
    {
        gl_info->gl_ops.gl.p_glDisable(GL_COLOR_MATERIAL);
        checkGLcall("glDisable GL_COLOR_MATERIAL");
    }
    else
    {
        gl_info->gl_ops.gl.p_glColorMaterial(GL_FRONT_AND_BACK, Parm);
        checkGLcall("glColorMaterial(GL_FRONT_AND_BACK, Parm)");
        gl_info->gl_ops.gl.p_glEnable(GL_COLOR_MATERIAL);
        checkGLcall("glEnable(GL_COLOR_MATERIAL)");
    }

    /* Apparently calls to glMaterialfv are ignored for properties we're
     * tracking with glColorMaterial, so apply those here. */
    switch (context_gl->tracking_parm)
    {
        case GL_AMBIENT_AND_DIFFUSE:
            gl_info->gl_ops.gl.p_glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, (float *)&state->material.ambient);
            gl_info->gl_ops.gl.p_glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, (float *)&state->material.diffuse);
            checkGLcall("glMaterialfv");
            break;

        case GL_DIFFUSE:
            gl_info->gl_ops.gl.p_glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, (float *)&state->material.diffuse);
            checkGLcall("glMaterialfv");
            break;

        case GL_AMBIENT:
            gl_info->gl_ops.gl.p_glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, (float *)&state->material.ambient);
            checkGLcall("glMaterialfv");
            break;

        case GL_EMISSION:
            gl_info->gl_ops.gl.p_glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, (float *)&state->material.emissive);
            checkGLcall("glMaterialfv");
            break;

        case GL_SPECULAR:
            /* Only change material color if specular is enabled, otherwise it is set to black */
            if (state->render_states[WINED3D_RS_SPECULARENABLE])
            {
                gl_info->gl_ops.gl.p_glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (float *)&state->material.specular);
                checkGLcall("glMaterialfv");
            }
            else
            {
                static const GLfloat black[] = {0.0f, 0.0f, 0.0f, 0.0f};
                gl_info->gl_ops.gl.p_glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &black[0]);
                checkGLcall("glMaterialfv");
            }
            break;
    }

    context_gl->tracking_parm = Parm;
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

static void state_normalize(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;

    if (isStateDirty(context, STATE_VDECL))
        return;

    /* Without vertex normals, we set the current normal to 0/0/0 to remove the diffuse factor
     * from the opengl lighting equation, as d3d does. Normalization of 0/0/0 can lead to a division
     * by zero and is not properly defined in opengl, so avoid it
     */
    if (state->render_states[WINED3D_RS_NORMALIZENORMALS]
            && (context->stream_info.use_map & (1u << WINED3D_FFP_NORMAL)))
    {
        gl_info->gl_ops.gl.p_glEnable(GL_NORMALIZE);
        checkGLcall("glEnable(GL_NORMALIZE);");
    }
    else
    {
        gl_info->gl_ops.gl.p_glDisable(GL_NORMALIZE);
        checkGLcall("glDisable(GL_NORMALIZE);");
    }
}

static void state_psizemin_w(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    float min, max;

    get_pointsize_minmax(context, state, &min, &max);

    if (min != 1.0f)
        FIXME("WINED3D_RS_POINTSIZE_MIN value %.8e not supported on this OpenGL implementation.\n", min);
    if (max != 64.0f)
        FIXME("WINED3D_RS_POINTSIZE_MAX value %.8e not supported on this OpenGL implementation.\n", max);
}

static void state_psizemin_ext(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    float min, max;

    get_pointsize_minmax(context, state, &min, &max);

    GL_EXTCALL(glPointParameterfEXT)(GL_POINT_SIZE_MIN_EXT, min);
    checkGLcall("glPointParameterfEXT(...)");
    GL_EXTCALL(glPointParameterfEXT)(GL_POINT_SIZE_MAX_EXT, max);
    checkGLcall("glPointParameterfEXT(...)");
}

static void state_psizemin_arb(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    float min, max;

    get_pointsize_minmax(context, state, &min, &max);

    GL_EXTCALL(glPointParameterfARB)(GL_POINT_SIZE_MIN_ARB, min);
    checkGLcall("glPointParameterfARB(...)");
    GL_EXTCALL(glPointParameterfARB)(GL_POINT_SIZE_MAX_ARB, max);
    checkGLcall("glPointParameterfARB(...)");
}

static void state_pscale(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    float att[3];
    float pointsize;

    get_pointsize(context, state, &pointsize, att);

    if (gl_info->supported[ARB_POINT_PARAMETERS])
    {
        GL_EXTCALL(glPointParameterfvARB)(GL_POINT_DISTANCE_ATTENUATION_ARB, att);
        checkGLcall("glPointParameterfvARB(GL_DISTANCE_ATTENUATION_ARB, ...)");
    }
    else if (gl_info->supported[EXT_POINT_PARAMETERS])
    {
        GL_EXTCALL(glPointParameterfvEXT)(GL_DISTANCE_ATTENUATION_EXT, att);
        checkGLcall("glPointParameterfvEXT(GL_DISTANCE_ATTENUATION_EXT, ...)");
    }
    else if (state->render_states[WINED3D_RS_POINTSCALEENABLE])
    {
        WARN("POINT_PARAMETERS not supported in this version of opengl\n");
    }

    gl_info->gl_ops.gl.p_glPointSize(max(pointsize, FLT_MIN));
    checkGLcall("glPointSize(...);");
}

static void state_debug_monitor(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    WARN("token: %#x.\n", state->render_states[WINED3D_RS_DEBUGMONITORTOKEN]);
}

static void state_colorwrite(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    DWORD mask0 = state->render_states[WINED3D_RS_COLORWRITEENABLE];

    TRACE("Color mask: r(%d) g(%d) b(%d) a(%d)\n",
            mask0 & WINED3DCOLORWRITEENABLE_RED ? 1 : 0,
            mask0 & WINED3DCOLORWRITEENABLE_GREEN ? 1 : 0,
            mask0 & WINED3DCOLORWRITEENABLE_BLUE ? 1 : 0,
            mask0 & WINED3DCOLORWRITEENABLE_ALPHA ? 1 : 0);
    gl_info->gl_ops.gl.p_glColorMask(mask0 & WINED3DCOLORWRITEENABLE_RED ? GL_TRUE : GL_FALSE,
            mask0 & WINED3DCOLORWRITEENABLE_GREEN ? GL_TRUE : GL_FALSE,
            mask0 & WINED3DCOLORWRITEENABLE_BLUE ? GL_TRUE : GL_FALSE,
            mask0 & WINED3DCOLORWRITEENABLE_ALPHA ? GL_TRUE : GL_FALSE);
    checkGLcall("glColorMask(...)");

    /* FIXME: WINED3D_RS_COLORWRITEENABLE1 .. WINED3D_RS_COLORWRITEENABLE7 not implemented. */
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

static void state_colorwrite_i(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    int index;

    if (state_id == WINED3D_RS_COLORWRITEENABLE) index = 0;
    else if (state_id <= WINED3D_RS_COLORWRITEENABLE3) index = state_id - WINED3D_RS_COLORWRITEENABLE1 + 1;
    else if (state_id <= WINED3D_RS_COLORWRITEENABLE7) index = state_id - WINED3D_RS_COLORWRITEENABLE4 + 4;
    else return;

    if (index >= gl_info->limits.buffers)
        WARN("Ignoring color write value for index %d, because gpu only supports %d render targets\n", index, gl_info->limits.buffers);

    set_color_mask(gl_info, index, state->render_states[state_id]);
}

#ifdef __REACTOS__
#else
static void state_colorwrite1(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    set_color_mask(wined3d_context_gl(context)->gl_info, 1, state->render_states[WINED3D_RS_COLORWRITEENABLE1]);
}

static void state_colorwrite2(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    set_color_mask(wined3d_context_gl(context)->gl_info, 2, state->render_states[WINED3D_RS_COLORWRITEENABLE2]);
}

static void state_colorwrite3(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    set_color_mask(wined3d_context_gl(context)->gl_info, 3, state->render_states[WINED3D_RS_COLORWRITEENABLE3]);
}
#endif

static void state_localviewer(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;

    if (state->render_states[WINED3D_RS_LOCALVIEWER])
    {
        gl_info->gl_ops.gl.p_glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
        checkGLcall("glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1)");
    }
    else
    {
        gl_info->gl_ops.gl.p_glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 0);
        checkGLcall("glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 0)");
    }
}

static void state_lastpixel(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (state->render_states[WINED3D_RS_LASTPIXEL])
    {
        TRACE("Last Pixel Drawing Enabled\n");
    }
    else
    {
        static BOOL warned;
        if (!warned) {
            FIXME("Last Pixel Drawing Disabled, not handled yet\n");
            warned = TRUE;
        } else {
            TRACE("Last Pixel Drawing Disabled, not handled yet\n");
        }
    }
}

void state_pointsprite_w(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    static BOOL warned;

    /* TODO: NV_POINT_SPRITE */
    if (!warned && state->render_states[WINED3D_RS_POINTSPRITEENABLE])
    {
        /* A FIXME, not a WARN because point sprites should be software emulated if not supported by HW */
        FIXME("Point sprites not supported\n");
        warned = TRUE;
    }
}

void state_pointsprite(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;

    if (state->render_states[WINED3D_RS_POINTSPRITEENABLE])
    {
        gl_info->gl_ops.gl.p_glEnable(GL_POINT_SPRITE_ARB);
        checkGLcall("glEnable(GL_POINT_SPRITE_ARB)");
    }
    else
    {
        gl_info->gl_ops.gl.p_glDisable(GL_POINT_SPRITE_ARB);
        checkGLcall("glDisable(GL_POINT_SPRITE_ARB)");
    }
}

static void state_wrap(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    static unsigned int once;

    if ((state->render_states[WINED3D_RS_WRAP0]
            || state->render_states[WINED3D_RS_WRAP1]
            || state->render_states[WINED3D_RS_WRAP2]
            || state->render_states[WINED3D_RS_WRAP3]
            || state->render_states[WINED3D_RS_WRAP4]
            || state->render_states[WINED3D_RS_WRAP5]
            || state->render_states[WINED3D_RS_WRAP6]
            || state->render_states[WINED3D_RS_WRAP7]
            || state->render_states[WINED3D_RS_WRAP8]
            || state->render_states[WINED3D_RS_WRAP9]
            || state->render_states[WINED3D_RS_WRAP10]
            || state->render_states[WINED3D_RS_WRAP11]
            || state->render_states[WINED3D_RS_WRAP12]
            || state->render_states[WINED3D_RS_WRAP13]
            || state->render_states[WINED3D_RS_WRAP14]
            || state->render_states[WINED3D_RS_WRAP15])
            && !once++)
        FIXME("(WINED3D_RS_WRAP0) Texture wrapping not yet supported.\n");
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

static void state_line_antialias(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;

    if (state->render_states[WINED3D_RS_EDGEANTIALIAS]
            || state->render_states[WINED3D_RS_ANTIALIASEDLINEENABLE])
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

static void state_scissor(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;

    if (state->render_states[WINED3D_RS_SCISSORTESTENABLE])
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
static void state_depthbias(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;

    if (state->render_states[WINED3D_RS_SLOPESCALEDEPTHBIAS]
            || state->render_states[WINED3D_RS_DEPTHBIAS])
    {
        const struct wined3d_rendertarget_view *depth = state->fb->depth_stencil;
        float factor, units, scale, clamp;

        union
        {
            DWORD d;
            float f;
        } scale_bias, const_bias;

        clamp = state->rasterizer_state ? state->rasterizer_state->desc.depth_bias_clamp : 0.0f;
        scale_bias.d = state->render_states[WINED3D_RS_SLOPESCALEDEPTHBIAS];
        const_bias.d = state->render_states[WINED3D_RS_DEPTHBIAS];

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

            factor = scale_bias.f;
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

static void state_zvisible(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (state->render_states[WINED3D_RS_ZVISIBLE])
        FIXME("WINED3D_RS_ZVISIBLE not implemented.\n");
}

static void state_stippledalpha(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (state->render_states[WINED3D_RS_STIPPLEDALPHA])
        FIXME("Stippled Alpha not supported yet.\n");
}

static void state_antialias(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (state->render_states[WINED3D_RS_ANTIALIAS])
        FIXME("Antialias not supported yet.\n");
}

static void state_multisampmask(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (state->render_states[WINED3D_RS_MULTISAMPLEMASK] != 0xffffffff)
        FIXME("WINED3D_RS_MULTISAMPLEMASK %#x not yet implemented.\n",
                state->render_states[WINED3D_RS_MULTISAMPLEMASK]);
}

static void state_patchedgestyle(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (state->render_states[WINED3D_RS_PATCHEDGESTYLE] != WINED3D_PATCH_EDGE_DISCRETE)
        FIXME("WINED3D_RS_PATCHEDGESTYLE %#x not yet implemented.\n",
                state->render_states[WINED3D_RS_PATCHEDGESTYLE]);
}

static void state_patchsegments(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    union {
        DWORD d;
        float f;
    } tmpvalue;
    tmpvalue.f = 1.0f;

    if (state->render_states[WINED3D_RS_PATCHSEGMENTS] != tmpvalue.d)
    {
        static BOOL displayed = FALSE;

        tmpvalue.d = state->render_states[WINED3D_RS_PATCHSEGMENTS];
        if(!displayed)
            FIXME("(WINED3D_RS_PATCHSEGMENTS,%f) not yet implemented\n", tmpvalue.f);

        displayed = TRUE;
    }
}

static void state_positiondegree(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (state->render_states[WINED3D_RS_POSITIONDEGREE] != WINED3D_DEGREE_CUBIC)
        FIXME("WINED3D_RS_POSITIONDEGREE %#x not yet implemented.\n",
                state->render_states[WINED3D_RS_POSITIONDEGREE]);
}

static void state_normaldegree(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (state->render_states[WINED3D_RS_NORMALDEGREE] != WINED3D_DEGREE_LINEAR)
        FIXME("WINED3D_RS_NORMALDEGREE %#x not yet implemented.\n",
                state->render_states[WINED3D_RS_NORMALDEGREE]);
}

static void state_tessellation(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (state->render_states[WINED3D_RS_ENABLEADAPTIVETESSELLATION])
        FIXME("WINED3D_RS_ENABLEADAPTIVETESSELLATION %#x not yet implemented.\n",
                state->render_states[WINED3D_RS_ENABLEADAPTIVETESSELLATION]);
}

static void state_nvdb(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    union
    {
        uint32_t d;
        float f;
    } zmin, zmax;

    if (state->render_states[WINED3D_RS_ADAPTIVETESS_X] == WINED3DFMT_NVDB)
    {
        zmin.d = state->render_states[WINED3D_RS_ADAPTIVETESS_Z];
        zmax.d = state->render_states[WINED3D_RS_ADAPTIVETESS_W];

        /* If zmin is larger than zmax INVALID_VALUE error is generated.
         * In d3d9 test is not performed in this case*/
        if (zmin.f <= zmax.f)
        {
            gl_info->gl_ops.gl.p_glEnable(GL_DEPTH_BOUNDS_TEST_EXT);
            checkGLcall("glEnable(GL_DEPTH_BOUNDS_TEST_EXT)");
            GL_EXTCALL(glDepthBoundsEXT(zmin.f, zmax.f));
            checkGLcall("glDepthBoundsEXT(...)");
        }
        else
        {
            gl_info->gl_ops.gl.p_glDisable(GL_DEPTH_BOUNDS_TEST_EXT);
            checkGLcall("glDisable(GL_DEPTH_BOUNDS_TEST_EXT)");
        }
    }
    else
    {
        gl_info->gl_ops.gl.p_glDisable(GL_DEPTH_BOUNDS_TEST_EXT);
        checkGLcall("glDisable(GL_DEPTH_BOUNDS_TEST_EXT)");
    }

    state_tessellation(context, state, STATE_RENDER(WINED3D_RS_ENABLEADAPTIVETESSELLATION));
}

static void state_wrapu(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (state->render_states[WINED3D_RS_WRAPU])
        FIXME("Render state WINED3D_RS_WRAPU not implemented yet.\n");
}

static void state_wrapv(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (state->render_states[WINED3D_RS_WRAPV])
        FIXME("Render state WINED3D_RS_WRAPV not implemented yet.\n");
}

static void state_monoenable(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (state->render_states[WINED3D_RS_MONOENABLE])
        FIXME("Render state WINED3D_RS_MONOENABLE not implemented yet.\n");
}

static void state_rop2(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (state->render_states[WINED3D_RS_ROP2])
        FIXME("Render state WINED3D_RS_ROP2 not implemented yet.\n");
}

static void state_planemask(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (state->render_states[WINED3D_RS_PLANEMASK])
        FIXME("Render state WINED3D_RS_PLANEMASK not implemented yet.\n");
}

static void state_subpixel(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (state->render_states[WINED3D_RS_SUBPIXEL])
        FIXME("Render state WINED3D_RS_SUBPIXEL not implemented yet.\n");
}

static void state_subpixelx(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (state->render_states[WINED3D_RS_SUBPIXELX])
        FIXME("Render state WINED3D_RS_SUBPIXELX not implemented yet.\n");
}

static void state_stippleenable(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (state->render_states[WINED3D_RS_STIPPLEENABLE])
        FIXME("Render state WINED3D_RS_STIPPLEENABLE not implemented yet.\n");
}

static void state_mipmaplodbias(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (state->render_states[WINED3D_RS_MIPMAPLODBIAS])
        FIXME("Render state WINED3D_RS_MIPMAPLODBIAS not implemented yet.\n");
}

static void state_anisotropy(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (state->render_states[WINED3D_RS_ANISOTROPY])
        FIXME("Render state WINED3D_RS_ANISOTROPY not implemented yet.\n");
}

static void state_flushbatch(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (state->render_states[WINED3D_RS_FLUSHBATCH])
        FIXME("Render state WINED3D_RS_FLUSHBATCH not implemented yet.\n");
}

static void state_translucentsi(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (state->render_states[WINED3D_RS_TRANSLUCENTSORTINDEPENDENT])
        FIXME("Render state WINED3D_RS_TRANSLUCENTSORTINDEPENDENT not implemented yet.\n");
}

static void state_extents(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (state->render_states[WINED3D_RS_EXTENTS])
        FIXME("Render state WINED3D_RS_EXTENTS not implemented yet.\n");
}

static void state_ckeyblend(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (state->render_states[WINED3D_RS_COLORKEYBLENDENABLE])
        FIXME("Render state WINED3D_RS_COLORKEYBLENDENABLE not implemented yet.\n");
}

static void state_swvp(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    static int once;
    if (state->render_states[WINED3D_RS_SOFTWAREVERTEXPROCESSING])
    {
        if (!once++)
            FIXME("Software vertex processing not implemented.\n");
    }
}

static void get_src_and_opr(DWORD arg, BOOL is_alpha, GLenum* source, GLenum* operand) {
    /* The WINED3DTA_ALPHAREPLICATE flag specifies the alpha component of the
    * input should be used for all input components. The WINED3DTA_COMPLEMENT
    * flag specifies the complement of the input should be used. */
    BOOL from_alpha = is_alpha || arg & WINED3DTA_ALPHAREPLICATE;
    BOOL complement = arg & WINED3DTA_COMPLEMENT;

    /* Calculate the operand */
    if (complement) {
        if (from_alpha) *operand = GL_ONE_MINUS_SRC_ALPHA;
        else *operand = GL_ONE_MINUS_SRC_COLOR;
    } else {
        if (from_alpha) *operand = GL_SRC_ALPHA;
        else *operand = GL_SRC_COLOR;
    }

    /* Calculate the source */
    switch (arg & WINED3DTA_SELECTMASK) {
        case WINED3DTA_CURRENT: *source = GL_PREVIOUS_EXT; break;
        case WINED3DTA_DIFFUSE: *source = GL_PRIMARY_COLOR_EXT; break;
        case WINED3DTA_TEXTURE: *source = GL_TEXTURE; break;
        case WINED3DTA_TFACTOR: *source = GL_CONSTANT_EXT; break;
        case WINED3DTA_SPECULAR:
            /*
            * According to the GL_ARB_texture_env_combine specs, SPECULAR is
            * 'Secondary color' and isn't supported until base GL supports it
            * There is no concept of temp registers as far as I can tell
            */
            FIXME("Unhandled texture arg WINED3DTA_SPECULAR\n");
            *source = GL_TEXTURE;
            break;
        default:
            FIXME("Unrecognized texture arg %#x\n", arg);
            *source = GL_TEXTURE;
            break;
    }
}

/* Setup the texture operations texture stage states */
static void set_tex_op(const struct wined3d_gl_info *gl_info, const struct wined3d_state *state,
        BOOL isAlpha, int Stage, enum wined3d_texture_op op, DWORD arg1, DWORD arg2, DWORD arg3)
{
    GLenum src1, src2, src3;
    GLenum opr1, opr2, opr3;
    GLenum comb_target;
    GLenum src0_target, src1_target, src2_target;
    GLenum opr0_target, opr1_target, opr2_target;
    GLenum scal_target;
    GLenum opr=0, invopr, src3_target, opr3_target;
    BOOL Handled = FALSE;

    TRACE("Alpha?(%d), Stage:%d Op(%s), a1(%d), a2(%d), a3(%d)\n", isAlpha, Stage, debug_d3dtop(op), arg1, arg2, arg3);

    /* Operations usually involve two args, src0 and src1 and are operations
     * of the form (a1 <operation> a2). However, some of the more complex
     * operations take 3 parameters. Instead of the (sensible) addition of a3,
     * Microsoft added in a third parameter called a0. Therefore these are
     * operations of the form a0 <operation> a1 <operation> a2. I.e., the new
     * parameter goes to the front.
     *
     * However, below we treat the new (a0) parameter as src2/opr2, so in the
     * actual functions below, expect their syntax to differ slightly to those
     * listed in the manuals. I.e., replace arg1 with arg3, arg2 with arg1 and
     * arg3 with arg2. This affects WINED3DTOP_MULTIPLYADD and WINED3DTOP_LERP. */

    if (isAlpha)
    {
        comb_target = GL_COMBINE_ALPHA;
        src0_target = GL_SOURCE0_ALPHA;
        src1_target = GL_SOURCE1_ALPHA;
        src2_target = GL_SOURCE2_ALPHA;
        opr0_target = GL_OPERAND0_ALPHA;
        opr1_target = GL_OPERAND1_ALPHA;
        opr2_target = GL_OPERAND2_ALPHA;
        scal_target = GL_ALPHA_SCALE;
    }
    else
    {
        comb_target = GL_COMBINE_RGB;
        src0_target = GL_SOURCE0_RGB;
        src1_target = GL_SOURCE1_RGB;
        src2_target = GL_SOURCE2_RGB;
        opr0_target = GL_OPERAND0_RGB;
        opr1_target = GL_OPERAND1_RGB;
        opr2_target = GL_OPERAND2_RGB;
        scal_target = GL_RGB_SCALE;
    }

        /* If a texture stage references an invalid texture unit the stage just
        * passes through the result from the previous stage */
    if (is_invalid_op(state, Stage, op, arg1, arg2, arg3))
    {
        arg1 = WINED3DTA_CURRENT;
        op = WINED3D_TOP_SELECT_ARG1;
    }

    if (isAlpha && !state->textures[Stage] && arg1 == WINED3DTA_TEXTURE)
    {
        get_src_and_opr(WINED3DTA_DIFFUSE, isAlpha, &src1, &opr1);
    } else {
        get_src_and_opr(arg1, isAlpha, &src1, &opr1);
    }
    get_src_and_opr(arg2, isAlpha, &src2, &opr2);
    get_src_and_opr(arg3, isAlpha, &src3, &opr3);

    TRACE("ct(%x), 1:(%x,%x), 2:(%x,%x), 3:(%x,%x)\n", comb_target, src1, opr1, src2, opr2, src3, opr3);

    Handled = TRUE; /* Assume will be handled */

    /* Other texture operations require special extensions: */
    if (gl_info->supported[NV_TEXTURE_ENV_COMBINE4])
    {
        if (isAlpha) {
            opr = GL_SRC_ALPHA;
            invopr = GL_ONE_MINUS_SRC_ALPHA;
            src3_target = GL_SOURCE3_ALPHA_NV;
            opr3_target = GL_OPERAND3_ALPHA_NV;
        } else {
            opr = GL_SRC_COLOR;
            invopr = GL_ONE_MINUS_SRC_COLOR;
            src3_target = GL_SOURCE3_RGB_NV;
            opr3_target = GL_OPERAND3_RGB_NV;
        }
        switch (op)
        {
            case WINED3D_TOP_DISABLE: /* Only for alpha */
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_REPLACE");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, GL_PREVIOUS_EXT);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, GL_SRC_ALPHA);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src2_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr);
                checkGLcall("GL_TEXTURE_ENV, opr3_target, opr");
                break;

            case WINED3D_TOP_SELECT_ARG1: /* = a1 * 1 + 0 * 0 */
            case WINED3D_TOP_SELECT_ARG2: /* = a2 * 1 + 0 * 0 */
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
                if (op == WINED3D_TOP_SELECT_ARG1)
                {
                    gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                    checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                    gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                    checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                }
                else
                {
                    gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src2);
                    checkGLcall("GL_TEXTURE_ENV, src0_target, src2");
                    gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr2);
                    checkGLcall("GL_TEXTURE_ENV, opr0_target, opr2");
                }
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src2_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr);
                checkGLcall("GL_TEXTURE_ENV, opr3_target, opr");
                break;

            case WINED3D_TOP_MODULATE:
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD"); /* Add = a0*a1 + a2*a3 */
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr);
                checkGLcall("GL_TEXTURE_ENV, opr3_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;
            case WINED3D_TOP_MODULATE_2X:
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD"); /* Add = a0*a1 + a2*a3 */
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr);
                checkGLcall("GL_TEXTURE_ENV, opr3_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 2);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 2");
                break;
            case WINED3D_TOP_MODULATE_4X:
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD"); /* Add = a0*a1 + a2*a3 */
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr);
                checkGLcall("GL_TEXTURE_ENV, opr3_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 4);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 4");
                break;

            case WINED3D_TOP_ADD:
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr3_target, invopr);
                checkGLcall("GL_TEXTURE_ENV, opr3_target, invopr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;

            case WINED3D_TOP_ADD_SIGNED:
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD_SIGNED);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD_SIGNED");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr3_target, invopr);
                checkGLcall("GL_TEXTURE_ENV, opr3_target, invopr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;

            case WINED3D_TOP_ADD_SIGNED_2X:
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD_SIGNED);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD_SIGNED");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr3_target, invopr);
                checkGLcall("GL_TEXTURE_ENV, opr3_target, invopr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 2);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 2");
                break;

            case WINED3D_TOP_ADD_SMOOTH:
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src3_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src3_target, src1");
                switch (opr1) {
                    case GL_SRC_COLOR: opr = GL_ONE_MINUS_SRC_COLOR; break;
                    case GL_ONE_MINUS_SRC_COLOR: opr = GL_SRC_COLOR; break;
                    case GL_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                    case GL_ONE_MINUS_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
                }
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr);
                checkGLcall("GL_TEXTURE_ENV, opr3_target, opr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;

            case WINED3D_TOP_BLEND_DIFFUSE_ALPHA:
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_PRIMARY_COLOR);
                checkGLcall("GL_TEXTURE_ENV, src1_target, GL_PRIMARY_COLOR");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_PRIMARY_COLOR);
                checkGLcall("GL_TEXTURE_ENV, src3_target, GL_PRIMARY_COLOR");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr3_target, GL_ONE_MINUS_SRC_ALPHA);
                checkGLcall("GL_TEXTURE_ENV, opr3_target, GL_ONE_MINUS_SRC_ALPHA");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;
            case WINED3D_TOP_BLEND_TEXTURE_ALPHA:
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_TEXTURE);
                checkGLcall("GL_TEXTURE_ENV, src1_target, GL_TEXTURE");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_TEXTURE);
                checkGLcall("GL_TEXTURE_ENV, src3_target, GL_TEXTURE");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr3_target, GL_ONE_MINUS_SRC_ALPHA);
                checkGLcall("GL_TEXTURE_ENV, opr3_target, GL_ONE_MINUS_SRC_ALPHA");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;
            case WINED3D_TOP_BLEND_FACTOR_ALPHA:
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_CONSTANT);
                checkGLcall("GL_TEXTURE_ENV, src1_target, GL_CONSTANT");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_CONSTANT);
                checkGLcall("GL_TEXTURE_ENV, src3_target, GL_CONSTANT");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr3_target, GL_ONE_MINUS_SRC_ALPHA);
                checkGLcall("GL_TEXTURE_ENV, opr3_target, GL_ONE_MINUS_SRC_ALPHA");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;
            case WINED3D_TOP_BLEND_TEXTURE_ALPHA_PM:
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_TEXTURE);
                checkGLcall("GL_TEXTURE_ENV, src3_target, GL_TEXTURE");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr3_target, GL_ONE_MINUS_SRC_ALPHA);
                checkGLcall("GL_TEXTURE_ENV, opr3_target, GL_ONE_MINUS_SRC_ALPHA");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;
            case WINED3D_TOP_MODULATE_ALPHA_ADD_COLOR:
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");                 /* Add = a0*a1 + a2*a3 */
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);  /*  a0 = src1/opr1     */
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");                   /*  a1 = 1 (see docs)  */
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);  /*  a2 = arg2          */
                checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");                   /*  a3 = src1 alpha    */
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src3_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src3_target, src1");
                switch (opr) {
                    case GL_SRC_COLOR: opr = GL_SRC_ALPHA; break;
                    case GL_ONE_MINUS_SRC_COLOR: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                }
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr);
                checkGLcall("GL_TEXTURE_ENV, opr3_target, opr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;
            case WINED3D_TOP_MODULATE_COLOR_ADD_ALPHA:
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src2_target, src1");
                switch (opr1) {
                    case GL_SRC_COLOR: opr = GL_SRC_ALPHA; break;
                    case GL_ONE_MINUS_SRC_COLOR: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                }
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr3_target, invopr);
                checkGLcall("GL_TEXTURE_ENV, opr3_target, invopr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;
            case WINED3D_TOP_MODULATE_INVALPHA_ADD_COLOR:
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src3_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src3_target, src1");
                switch (opr1) {
                    case GL_SRC_COLOR: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                    case GL_ONE_MINUS_SRC_COLOR: opr = GL_SRC_ALPHA; break;
                    case GL_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                    case GL_ONE_MINUS_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
                }
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr);
                checkGLcall("GL_TEXTURE_ENV, opr3_target, opr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;
            case WINED3D_TOP_MODULATE_INVCOLOR_ADD_ALPHA:
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                switch (opr1) {
                    case GL_SRC_COLOR: opr = GL_ONE_MINUS_SRC_COLOR; break;
                    case GL_ONE_MINUS_SRC_COLOR: opr = GL_SRC_COLOR; break;
                    case GL_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                    case GL_ONE_MINUS_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
                }
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src2_target, src1");
                switch (opr1) {
                    case GL_SRC_COLOR: opr = GL_SRC_ALPHA; break;
                    case GL_ONE_MINUS_SRC_COLOR: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                }
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr3_target, invopr);
                checkGLcall("GL_TEXTURE_ENV, opr3_target, invopr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;
            case WINED3D_TOP_MULTIPLY_ADD:
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src3);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr3);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
                checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src3_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src3_target, src3");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr3_target, opr3");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;

            case WINED3D_TOP_BUMPENVMAP:
            case WINED3D_TOP_BUMPENVMAP_LUMINANCE:
                FIXME("Implement bump environment mapping in GL_NV_texture_env_combine4 path\n");
                Handled = FALSE;
                break;

            default:
                Handled = FALSE;
        }
        if (Handled)
        {
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE4_NV);
            checkGLcall("GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE4_NV");

            return;
        }
    } /* GL_NV_texture_env_combine4 */

    Handled = TRUE; /* Again, assume handled */
    switch (op) {
        case WINED3D_TOP_DISABLE: /* Only for alpha */
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_REPLACE);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_REPLACE");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, GL_PREVIOUS_EXT);
            checkGLcall("GL_TEXTURE_ENV, src0_target, GL_PREVIOUS_EXT");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, GL_SRC_ALPHA);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, GL_SRC_ALPHA");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;
        case WINED3D_TOP_SELECT_ARG1:
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_REPLACE);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_REPLACE");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;
        case WINED3D_TOP_SELECT_ARG2:
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_REPLACE);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_REPLACE");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;
        case WINED3D_TOP_MODULATE:
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;
        case WINED3D_TOP_MODULATE_2X:
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 2);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 2");
            break;
        case WINED3D_TOP_MODULATE_4X:
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 4);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 4");
            break;
        case WINED3D_TOP_ADD:
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;
        case WINED3D_TOP_ADD_SIGNED:
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD_SIGNED);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD_SIGNED");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;
        case WINED3D_TOP_ADD_SIGNED_2X:
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD_SIGNED);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD_SIGNED");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 2);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 2");
            break;
        case WINED3D_TOP_SUBTRACT:
            if (gl_info->supported[ARB_TEXTURE_ENV_COMBINE])
            {
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_SUBTRACT);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_SUBTRACT");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            } else {
                FIXME("This version of opengl does not support GL_SUBTRACT\n");
            }
            break;

        case WINED3D_TOP_BLEND_DIFFUSE_ALPHA:
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_INTERPOLATE);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_INTERPOLATE");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, GL_PRIMARY_COLOR);
            checkGLcall("GL_TEXTURE_ENV, src2_target, GL_PRIMARY_COLOR");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;
        case WINED3D_TOP_BLEND_TEXTURE_ALPHA:
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_INTERPOLATE);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_INTERPOLATE");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, GL_TEXTURE);
            checkGLcall("GL_TEXTURE_ENV, src2_target, GL_TEXTURE");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;
        case WINED3D_TOP_BLEND_FACTOR_ALPHA:
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_INTERPOLATE);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_INTERPOLATE");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, GL_CONSTANT);
            checkGLcall("GL_TEXTURE_ENV, src2_target, GL_CONSTANT");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;
        case WINED3D_TOP_BLEND_CURRENT_ALPHA:
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_INTERPOLATE);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_INTERPOLATE");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, GL_PREVIOUS);
            checkGLcall("GL_TEXTURE_ENV, src2_target, GL_PREVIOUS");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;
        case WINED3D_TOP_DOTPRODUCT3:
            if (gl_info->supported[ARB_TEXTURE_ENV_DOT3])
            {
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_DOT3_RGBA_ARB);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_DOT3_RGBA_ARB");
            }
            else if (gl_info->supported[EXT_TEXTURE_ENV_DOT3])
            {
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_DOT3_RGBA_EXT);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_DOT3_RGBA_EXT");
            } else {
                FIXME("This version of opengl does not support GL_DOT3\n");
            }
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;
        case WINED3D_TOP_LERP:
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_INTERPOLATE);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_INTERPOLATE");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, src3);
            checkGLcall("GL_TEXTURE_ENV, src2_target, src3");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr3);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr3");
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;
        case WINED3D_TOP_ADD_SMOOTH:
            if (gl_info->supported[ATI_TEXTURE_ENV_COMBINE3])
            {
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                switch (opr1) {
                    case GL_SRC_COLOR: opr = GL_ONE_MINUS_SRC_COLOR; break;
                    case GL_ONE_MINUS_SRC_COLOR: opr = GL_SRC_COLOR; break;
                    case GL_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                    case GL_ONE_MINUS_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
                }
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src1_target, src1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            } else
                Handled = FALSE;
            break;
        case WINED3D_TOP_BLEND_TEXTURE_ALPHA_PM:
            if (gl_info->supported[ATI_TEXTURE_ENV_COMBINE3])
            {
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, GL_TEXTURE);
                checkGLcall("GL_TEXTURE_ENV, src0_target, GL_TEXTURE");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, GL_ONE_MINUS_SRC_ALPHA);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, GL_ONE_MINUS_SRC_APHA");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src1_target, src1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            } else
                Handled = FALSE;
            break;
        case WINED3D_TOP_MODULATE_ALPHA_ADD_COLOR:
            if (gl_info->supported[ATI_TEXTURE_ENV_COMBINE3])
            {
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                switch (opr1) {
                    case GL_SRC_COLOR: opr = GL_SRC_ALPHA; break;
                    case GL_ONE_MINUS_SRC_COLOR: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                    case GL_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
                    case GL_ONE_MINUS_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                }
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src1_target, src1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            } else
                Handled = FALSE;
            break;
        case WINED3D_TOP_MODULATE_COLOR_ADD_ALPHA:
            if (gl_info->supported[ATI_TEXTURE_ENV_COMBINE3])
            {
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src1_target, src1");
                switch (opr1) {
                    case GL_SRC_COLOR: opr = GL_SRC_ALPHA; break;
                    case GL_ONE_MINUS_SRC_COLOR: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                    case GL_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
                    case GL_ONE_MINUS_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                }
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, opr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            } else
                Handled = FALSE;
            break;
        case WINED3D_TOP_MODULATE_INVALPHA_ADD_COLOR:
            if (gl_info->supported[ATI_TEXTURE_ENV_COMBINE3])
            {
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                switch (opr1) {
                    case GL_SRC_COLOR: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                    case GL_ONE_MINUS_SRC_COLOR: opr = GL_SRC_ALPHA; break;
                    case GL_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                    case GL_ONE_MINUS_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
                }
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src1_target, src1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            } else
                Handled = FALSE;
            break;
        case WINED3D_TOP_MODULATE_INVCOLOR_ADD_ALPHA:
            if (gl_info->supported[ATI_TEXTURE_ENV_COMBINE3])
            {
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                switch (opr1) {
                    case GL_SRC_COLOR: opr = GL_ONE_MINUS_SRC_COLOR; break;
                    case GL_ONE_MINUS_SRC_COLOR: opr = GL_SRC_COLOR; break;
                    case GL_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                    case GL_ONE_MINUS_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
                }
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src1_target, src1");
                switch (opr1) {
                    case GL_SRC_COLOR: opr = GL_SRC_ALPHA; break;
                    case GL_ONE_MINUS_SRC_COLOR: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                    case GL_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
                    case GL_ONE_MINUS_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                }
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, opr");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            } else
                Handled = FALSE;
            break;
        case WINED3D_TOP_MULTIPLY_ADD:
            if (gl_info->supported[ATI_TEXTURE_ENV_COMBINE3])
            {
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src1_target, src3);
                checkGLcall("GL_TEXTURE_ENV, src1_target, src3");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr3);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, opr3");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            } else
                Handled = FALSE;
            break;
        case WINED3D_TOP_BUMPENVMAP_LUMINANCE:
        case WINED3D_TOP_BUMPENVMAP:
            if (gl_info->supported[NV_TEXTURE_SHADER2])
            {
                /* Technically texture shader support without register combiners is possible, but not expected to occur
                 * on real world cards, so for now a fixme should be enough
                 */
                FIXME("Implement bump mapping with GL_NV_texture_shader in non register combiner path\n");
            }
            Handled = FALSE;
            break;

        default:
            Handled = FALSE;
    }

    if (Handled) {
        BOOL  combineOK = TRUE;
        if (gl_info->supported[NV_TEXTURE_ENV_COMBINE4])
        {
            DWORD op2;

            if (isAlpha)
                op2 = state->texture_states[Stage][WINED3D_TSS_COLOR_OP];
            else
                op2 = state->texture_states[Stage][WINED3D_TSS_ALPHA_OP];

            /* Note: If COMBINE4 in effect can't go back to combine! */
            switch (op2)
            {
                case WINED3D_TOP_ADD_SMOOTH:
                case WINED3D_TOP_BLEND_TEXTURE_ALPHA_PM:
                case WINED3D_TOP_MODULATE_ALPHA_ADD_COLOR:
                case WINED3D_TOP_MODULATE_COLOR_ADD_ALPHA:
                case WINED3D_TOP_MODULATE_INVALPHA_ADD_COLOR:
                case WINED3D_TOP_MODULATE_INVCOLOR_ADD_ALPHA:
                case WINED3D_TOP_MULTIPLY_ADD:
                    /* Ignore those implemented in both cases */
                    switch (op)
                    {
                        case WINED3D_TOP_SELECT_ARG1:
                        case WINED3D_TOP_SELECT_ARG2:
                            combineOK = FALSE;
                            Handled   = FALSE;
                            break;
                        default:
                            FIXME("Can't use COMBINE4 and COMBINE together, thisop=%s, otherop=%s, isAlpha(%d)\n", debug_d3dtop(op), debug_d3dtop(op2), isAlpha);
                            return;
                    }
            }
        }

        if (combineOK)
        {
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
            checkGLcall("GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE");

            return;
        }
    }

    /* After all the extensions, if still unhandled, report fixme */
    FIXME("Unhandled texture operation %s\n", debug_d3dtop(op));
}


static void tex_colorop(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    unsigned int stage = (state_id - STATE_TEXTURESTAGE(0, 0)) / (WINED3D_HIGHEST_TEXTURE_STATE + 1);
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    BOOL tex_used = context->fixed_function_usage_map & (1u << stage);
    unsigned int mapped_stage = context_gl->tex_unit_map[stage];
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    TRACE("Setting color op for stage %d\n", stage);

    /* Using a pixel shader? Don't care for anything here, the shader applying does it */
    if (use_ps(state)) return;

    if (stage != mapped_stage) WARN("Using non 1:1 mapping: %d -> %d!\n", stage, mapped_stage);

    if (mapped_stage != WINED3D_UNMAPPED_STAGE)
    {
        if (tex_used && mapped_stage >= gl_info->limits.textures)
        {
            FIXME("Attempt to enable unsupported stage!\n");
            return;
        }
        wined3d_context_gl_active_texture(context_gl, gl_info, mapped_stage);
    }

    if (stage >= context->lowest_disabled_stage)
    {
        TRACE("Stage disabled\n");
        if (mapped_stage != WINED3D_UNMAPPED_STAGE)
        {
            /* Disable everything here */
            gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_2D);
            checkGLcall("glDisable(GL_TEXTURE_2D)");
            gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_3D);
            checkGLcall("glDisable(GL_TEXTURE_3D)");
            if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
            {
                gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_CUBE_MAP_ARB);
                checkGLcall("glDisable(GL_TEXTURE_CUBE_MAP_ARB)");
            }
            if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
            {
                gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_RECTANGLE_ARB);
                checkGLcall("glDisable(GL_TEXTURE_RECTANGLE_ARB)");
            }
        }
        /* All done */
        return;
    }

    /* The sampler will also activate the correct texture dimensions, so no
     * need to do it here if the sampler for this stage is dirty. */
    if (!isStateDirty(context, STATE_SAMPLER(stage)) && tex_used)
        texture_activate_dimensions(state->textures[stage], gl_info);

    set_tex_op(gl_info, state, FALSE, stage,
            state->texture_states[stage][WINED3D_TSS_COLOR_OP],
            state->texture_states[stage][WINED3D_TSS_COLOR_ARG1],
            state->texture_states[stage][WINED3D_TSS_COLOR_ARG2],
            state->texture_states[stage][WINED3D_TSS_COLOR_ARG0]);
}

void tex_alphaop(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    unsigned int stage = (state_id - STATE_TEXTURESTAGE(0, 0)) / (WINED3D_HIGHEST_TEXTURE_STATE + 1);
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    BOOL tex_used = context->fixed_function_usage_map & (1u << stage);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    unsigned int mapped_stage = context_gl->tex_unit_map[stage];
    DWORD op, arg1, arg2, arg0;

    TRACE("Setting alpha op for stage %d\n", stage);
    /* Do not care for enabled / disabled stages, just assign the settings. colorop disables / enables required stuff */
    if (mapped_stage != WINED3D_UNMAPPED_STAGE)
    {
        if (tex_used && mapped_stage >= gl_info->limits.textures)
        {
            FIXME("Attempt to enable unsupported stage!\n");
            return;
        }
        wined3d_context_gl_active_texture(context_gl, gl_info, mapped_stage);
    }

    op = state->texture_states[stage][WINED3D_TSS_ALPHA_OP];
    arg1 = state->texture_states[stage][WINED3D_TSS_ALPHA_ARG1];
    arg2 = state->texture_states[stage][WINED3D_TSS_ALPHA_ARG2];
    arg0 = state->texture_states[stage][WINED3D_TSS_ALPHA_ARG0];

    if (state->render_states[WINED3D_RS_COLORKEYENABLE] && !stage && state->textures[0])
    {
        struct wined3d_texture_gl *texture_gl = wined3d_texture_gl(state->textures[0]);
        GLenum texture_dimensions = texture_gl->target;

        if (texture_dimensions == GL_TEXTURE_2D || texture_dimensions == GL_TEXTURE_RECTANGLE_ARB)
        {
            if (texture_gl->t.async.color_key_flags & WINED3D_CKEY_SRC_BLT
                    && !texture_gl->t.resource.format->alpha_size)
            {
                /* Color keying needs to pass alpha values from the texture through to have the alpha test work
                 * properly. On the other hand applications can still use texture combiners apparently. This code
                 * takes care that apps cannot remove the texture's alpha channel entirely.
                 *
                 * The fixup is required for Prince of Persia 3D(prison bars), while Moto racer 2 requires
                 * D3DTOP_MODULATE to work on color keyed surfaces. Aliens vs Predator 1 uses color keyed textures
                 * and alpha component of diffuse color to draw things like translucent text and perform other
                 * blending effects.
                 *
                 * Aliens vs Predator 1 relies on diffuse alpha having an effect, so it cannot be ignored. To
                 * provide the behavior expected by the game, while emulating the colorkey, diffuse alpha must be
                 * modulated with texture alpha. OTOH, Moto racer 2 at some points sets alphaop/alphaarg to
                 * SELECTARG/CURRENT, yet puts garbage in diffuse alpha (zeroes). This works on native, because the
                 * game disables alpha test and alpha blending. Alpha test is overwritten by wine's for purposes of
                 * color-keying though, so this will lead to missing geometry if texture alpha is modulated (pixels
                 * fail alpha test). To get around this, ALPHABLENDENABLE state is checked: if the app enables alpha
                 * blending, it can be expected to provide meaningful values in diffuse alpha, so it should be
                 * modulated with texture alpha; otherwise, selecting diffuse alpha is ignored in favour of texture
                 * alpha.
                 *
                 * What to do with multitexturing? So far no app has been found that uses color keying with
                 * multitexturing */
                if (op == WINED3D_TOP_DISABLE)
                {
                    arg1 = WINED3DTA_TEXTURE;
                    op = WINED3D_TOP_SELECT_ARG1;
                }
                else if (op == WINED3D_TOP_SELECT_ARG1 && arg1 != WINED3DTA_TEXTURE)
                {
                    if (state->render_states[WINED3D_RS_ALPHABLENDENABLE])
                    {
                        arg2 = WINED3DTA_TEXTURE;
                        op = WINED3D_TOP_MODULATE;
                    }
                    else arg1 = WINED3DTA_TEXTURE;
                }
                else if (op == WINED3D_TOP_SELECT_ARG2 && arg2 != WINED3DTA_TEXTURE)
                {
                    if (state->render_states[WINED3D_RS_ALPHABLENDENABLE])
                    {
                        arg1 = WINED3DTA_TEXTURE;
                        op = WINED3D_TOP_MODULATE;
                    }
                    else arg2 = WINED3DTA_TEXTURE;
                }
            }
        }
    }

    /* tex_alphaop is shared between the ffp and nvrc because the difference only comes down to
     * this if block here, and the other code(color keying, texture unit selection) are the same
     */
    TRACE("Setting alpha op for stage %d\n", stage);
    if (gl_info->supported[NV_REGISTER_COMBINERS])
    {
        set_tex_op_nvrc(gl_info, state, TRUE, stage, op, arg1, arg2, arg0,
                mapped_stage, state->texture_states[stage][WINED3D_TSS_RESULT_ARG]);
    }
    else
    {
        set_tex_op(gl_info, state, TRUE, stage, op, arg1, arg2, arg0);
    }
}

static void transform_texture(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    unsigned int tex = (state_id - STATE_TEXTURESTAGE(0, 0)) / (WINED3D_HIGHEST_TEXTURE_STATE + 1);
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    unsigned int mapped_stage = context_gl->tex_unit_map[tex];
    struct wined3d_matrix mat;

    /* Ignore this when a vertex shader is used, or if the streams aren't sorted out yet */
    if (use_vs(state) || isStateDirty(context, STATE_VDECL))
    {
        TRACE("Using a vertex shader, or stream sources not sorted out yet, skipping\n");
        return;
    }

    if (mapped_stage == WINED3D_UNMAPPED_STAGE) return;
    if (mapped_stage >= gl_info->limits.textures) return;

    wined3d_context_gl_active_texture(context_gl, gl_info, mapped_stage);
    gl_info->gl_ops.gl.p_glMatrixMode(GL_TEXTURE);
    checkGLcall("glMatrixMode(GL_TEXTURE)");

    get_texture_matrix(context, state, mapped_stage, &mat);

    gl_info->gl_ops.gl.p_glLoadMatrixf(&mat._11);
    checkGLcall("glLoadMatrixf");
}

static void tex_coordindex(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    unsigned int stage = (state_id - STATE_TEXTURESTAGE(0, 0)) / (WINED3D_HIGHEST_TEXTURE_STATE + 1);
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    unsigned int mapped_stage = context_gl->tex_unit_map[stage];

    static const GLfloat s_plane[] = { 1.0f, 0.0f, 0.0f, 0.0f };
    static const GLfloat t_plane[] = { 0.0f, 1.0f, 0.0f, 0.0f };
    static const GLfloat r_plane[] = { 0.0f, 0.0f, 1.0f, 0.0f };
    static const GLfloat q_plane[] = { 0.0f, 0.0f, 0.0f, 1.0f };

    if (mapped_stage == WINED3D_UNMAPPED_STAGE)
    {
        TRACE("No texture unit mapped to stage %d. Skipping texture coordinates.\n", stage);
        return;
    }

    if (mapped_stage >= min(gl_info->limits.samplers[WINED3D_SHADER_TYPE_PIXEL], WINED3D_MAX_FRAGMENT_SAMPLERS))
    {
        WARN("stage %u not mapped to a valid texture unit (%u)\n", stage, mapped_stage);
        return;
    }
    wined3d_context_gl_active_texture(context_gl, gl_info, mapped_stage);

    /* Values 0-7 are indexes into the FVF tex coords - See comments in DrawPrimitive
     *
     * FIXME: When using generated texture coordinates, the index value is used to specify the wrapping mode.
     * eg. SetTextureStageState( 0, WINED3D_TSS_TEXCOORDINDEX, WINED3D_TSS_TCI_CAMERASPACEPOSITION | 1 );
     * means use the vertex position (camera-space) as the input texture coordinates
     * for this texture stage, and the wrap mode set in the WINED3D_RS_WRAP1 render
     * state. We do not (yet) support the WINED3DRENDERSTATE_WRAPx values, nor tie them up
     * to the TEXCOORDINDEX value
     */
    switch (state->texture_states[stage][WINED3D_TSS_TEXCOORD_INDEX] & 0xffff0000)
    {
        case WINED3DTSS_TCI_PASSTHRU:
            /* Use the specified texture coordinates contained within the
             * vertex format. This value resolves to zero. */
            gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_GEN_S);
            gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_GEN_T);
            gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_GEN_R);
            gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_GEN_Q);
            checkGLcall("WINED3DTSS_TCI_PASSTHRU - Disable texgen.");
            break;

        case WINED3DTSS_TCI_CAMERASPACEPOSITION:
            /* CameraSpacePosition means use the vertex position, transformed to camera space,
             * as the input texture coordinates for this stage's texture transformation. This
             * equates roughly to EYE_LINEAR */

            gl_info->gl_ops.gl.p_glMatrixMode(GL_MODELVIEW);
            gl_info->gl_ops.gl.p_glPushMatrix();
            gl_info->gl_ops.gl.p_glLoadIdentity();
            gl_info->gl_ops.gl.p_glTexGenfv(GL_S, GL_EYE_PLANE, s_plane);
            gl_info->gl_ops.gl.p_glTexGenfv(GL_T, GL_EYE_PLANE, t_plane);
            gl_info->gl_ops.gl.p_glTexGenfv(GL_R, GL_EYE_PLANE, r_plane);
            gl_info->gl_ops.gl.p_glTexGenfv(GL_Q, GL_EYE_PLANE, q_plane);
            gl_info->gl_ops.gl.p_glPopMatrix();
            checkGLcall("WINED3DTSS_TCI_CAMERASPACEPOSITION - Set eye plane.");

            gl_info->gl_ops.gl.p_glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
            gl_info->gl_ops.gl.p_glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
            gl_info->gl_ops.gl.p_glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
            checkGLcall("WINED3DTSS_TCI_CAMERASPACEPOSITION - Set texgen mode.");

            gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_GEN_S);
            gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_GEN_T);
            gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_GEN_R);
            checkGLcall("WINED3DTSS_TCI_CAMERASPACEPOSITION - Enable texgen.");

            break;

        case WINED3DTSS_TCI_CAMERASPACENORMAL:
            /* Note that NV_TEXGEN_REFLECTION support is implied when
             * ARB_TEXTURE_CUBE_MAP is supported */
            if (!gl_info->supported[NV_TEXGEN_REFLECTION])
            {
                FIXME("WINED3DTSS_TCI_CAMERASPACENORMAL not supported.\n");
                break;
            }

            gl_info->gl_ops.gl.p_glMatrixMode(GL_MODELVIEW);
            gl_info->gl_ops.gl.p_glPushMatrix();
            gl_info->gl_ops.gl.p_glLoadIdentity();
            gl_info->gl_ops.gl.p_glTexGenfv(GL_S, GL_EYE_PLANE, s_plane);
            gl_info->gl_ops.gl.p_glTexGenfv(GL_T, GL_EYE_PLANE, t_plane);
            gl_info->gl_ops.gl.p_glTexGenfv(GL_R, GL_EYE_PLANE, r_plane);
            gl_info->gl_ops.gl.p_glTexGenfv(GL_Q, GL_EYE_PLANE, q_plane);
            gl_info->gl_ops.gl.p_glPopMatrix();
            checkGLcall("WINED3DTSS_TCI_CAMERASPACENORMAL - Set eye plane.");

            gl_info->gl_ops.gl.p_glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_NV);
            gl_info->gl_ops.gl.p_glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_NV);
            gl_info->gl_ops.gl.p_glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_NV);
            checkGLcall("WINED3DTSS_TCI_CAMERASPACENORMAL - Set texgen mode.");

            gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_GEN_S);
            gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_GEN_T);
            gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_GEN_R);
            checkGLcall("WINED3DTSS_TCI_CAMERASPACENORMAL - Enable texgen.");

            break;

        case WINED3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR:
            /* Note that NV_TEXGEN_REFLECTION support is implied when
             * ARB_TEXTURE_CUBE_MAP is supported */
            if (!gl_info->supported[NV_TEXGEN_REFLECTION])
            {
                FIXME("WINED3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR not supported.\n");
                break;
            }

            gl_info->gl_ops.gl.p_glMatrixMode(GL_MODELVIEW);
            gl_info->gl_ops.gl.p_glPushMatrix();
            gl_info->gl_ops.gl.p_glLoadIdentity();
            gl_info->gl_ops.gl.p_glTexGenfv(GL_S, GL_EYE_PLANE, s_plane);
            gl_info->gl_ops.gl.p_glTexGenfv(GL_T, GL_EYE_PLANE, t_plane);
            gl_info->gl_ops.gl.p_glTexGenfv(GL_R, GL_EYE_PLANE, r_plane);
            gl_info->gl_ops.gl.p_glTexGenfv(GL_Q, GL_EYE_PLANE, q_plane);
            gl_info->gl_ops.gl.p_glPopMatrix();
            checkGLcall("WINED3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR - Set eye plane.");

            gl_info->gl_ops.gl.p_glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_NV);
            gl_info->gl_ops.gl.p_glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_NV);
            gl_info->gl_ops.gl.p_glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_NV);
            checkGLcall("WINED3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR - Set texgen mode.");

            gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_GEN_S);
            gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_GEN_T);
            gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_GEN_R);
            checkGLcall("WINED3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR - Enable texgen.");

            break;

        case WINED3DTSS_TCI_SPHEREMAP:
            gl_info->gl_ops.gl.p_glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            gl_info->gl_ops.gl.p_glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            checkGLcall("WINED3DTSS_TCI_SPHEREMAP - Set texgen mode.");

            gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_GEN_S);
            gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_GEN_T);
            gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_GEN_R);
            checkGLcall("WINED3DTSS_TCI_SPHEREMAP - Enable texgen.");

            break;

        default:
            FIXME("Unhandled WINED3D_TSS_TEXCOORD_INDEX %#x.\n",
                    state->texture_states[stage][WINED3D_TSS_TEXCOORD_INDEX]);
            gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_GEN_S);
            gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_GEN_T);
            gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_GEN_R);
            gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_GEN_Q);
            checkGLcall("Disable texgen.");

            break;
    }

    /* Update the texture matrix. */
    if (!isStateDirty(context, STATE_TRANSFORM(WINED3D_TS_TEXTURE0 + stage)))
        transform_texture(context, state, STATE_TEXTURESTAGE(stage, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS));

    if (!isStateDirty(context, STATE_VDECL) && context->namedArraysLoaded)
    {
        /* Reload the arrays if we are using fixed function arrays to reflect the selected coord input
         * source. Call loadTexCoords directly because there is no need to reparse the vertex declaration
         * and do all the things linked to it
         * TODO: Tidy that up to reload only the arrays of the changed unit
         */
        GLuint curVBO = gl_info->supported[ARB_VERTEX_BUFFER_OBJECT] ? ~0U : 0;

        wined3d_context_gl_unload_tex_coords(context_gl);
        wined3d_context_gl_load_tex_coords(context_gl, &context->stream_info, &curVBO, state);
    }
}

static void sampler_texmatrix(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const DWORD sampler = state_id - STATE_SAMPLER(0);
    const struct wined3d_texture *texture = state->textures[sampler];

    TRACE("context %p, state %p, state_id %#x.\n", context, state, state_id);

    if (!texture)
        return;

    /* The fixed function np2 texture emulation uses the texture matrix to fix up the coordinates
     * wined3d_texture_apply_state_changes() multiplies the set matrix with a fixup matrix. Before the
     * scaling is reapplied or removed, the texture matrix has to be reapplied.
     */
    if (sampler < WINED3D_MAX_TEXTURES)
    {
        const BOOL tex_is_pow2 = !(texture->flags & WINED3D_TEXTURE_POW2_MAT_IDENT);

        if (tex_is_pow2 || (context->lastWasPow2Texture & (1u << sampler)))
        {
            if (tex_is_pow2)
                context->lastWasPow2Texture |= 1u << sampler;
            else
                context->lastWasPow2Texture &= ~(1u << sampler);

            transform_texture(context, state, STATE_TEXTURESTAGE(sampler, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS));
        }
    }
}

static enum wined3d_texture_address wined3d_texture_gl_address_mode(const struct wined3d_texture_gl *texture_gl,
        enum wined3d_texture_address t)
{
    if (t < WINED3D_TADDRESS_WRAP || t > WINED3D_TADDRESS_MIRROR_ONCE)
    {
        FIXME("Unrecognized or unsupported texture address mode %#x.\n", t);
        return WINED3D_TADDRESS_WRAP;
    }

    /* Cubemaps are always set to clamp, regardless of the sampler state. */
    if (texture_gl->target == GL_TEXTURE_CUBE_MAP_ARB || ((texture_gl->t.flags & WINED3D_TEXTURE_COND_NP2)
            && t == WINED3D_TADDRESS_WRAP))
        return WINED3D_TADDRESS_CLAMP;

    return t;
}

static void wined3d_sampler_desc_from_sampler_states(struct wined3d_sampler_desc *desc,
        const struct wined3d_context_gl *context_gl, const DWORD *sampler_states,
        const struct wined3d_texture_gl *texture_gl)
{
    union
    {
        float f;
        DWORD d;
    } lod_bias;

    desc->address_u = wined3d_texture_gl_address_mode(texture_gl, sampler_states[WINED3D_SAMP_ADDRESS_U]);
    desc->address_v = wined3d_texture_gl_address_mode(texture_gl, sampler_states[WINED3D_SAMP_ADDRESS_V]);
    desc->address_w = wined3d_texture_gl_address_mode(texture_gl, sampler_states[WINED3D_SAMP_ADDRESS_W]);
    wined3d_color_from_d3dcolor((struct wined3d_color *)desc->border_color,
            sampler_states[WINED3D_SAMP_BORDER_COLOR]);
    if (sampler_states[WINED3D_SAMP_MAG_FILTER] > WINED3D_TEXF_ANISOTROPIC)
        FIXME("Unrecognized or unsupported WINED3D_SAMP_MAG_FILTER %#x.\n",
                sampler_states[WINED3D_SAMP_MAG_FILTER]);
    desc->mag_filter = min(max(sampler_states[WINED3D_SAMP_MAG_FILTER], WINED3D_TEXF_POINT), WINED3D_TEXF_LINEAR);
    if (sampler_states[WINED3D_SAMP_MIN_FILTER] > WINED3D_TEXF_ANISOTROPIC)
        FIXME("Unrecognized or unsupported WINED3D_SAMP_MIN_FILTER %#x.\n",
                sampler_states[WINED3D_SAMP_MIN_FILTER]);
    desc->min_filter = min(max(sampler_states[WINED3D_SAMP_MIN_FILTER], WINED3D_TEXF_POINT), WINED3D_TEXF_LINEAR);
    if (sampler_states[WINED3D_SAMP_MIP_FILTER] > WINED3D_TEXF_ANISOTROPIC)
        FIXME("Unrecognized or unsupported WINED3D_SAMP_MIP_FILTER %#x.\n",
                sampler_states[WINED3D_SAMP_MIP_FILTER]);
    desc->mip_filter = min(max(sampler_states[WINED3D_SAMP_MIP_FILTER], WINED3D_TEXF_NONE), WINED3D_TEXF_LINEAR);
    lod_bias.d = sampler_states[WINED3D_SAMP_MIPMAP_LOD_BIAS];
    desc->lod_bias = lod_bias.f;
    desc->min_lod = -1000.0f;
    desc->max_lod = 1000.0f;
    desc->mip_base_level = sampler_states[WINED3D_SAMP_MAX_MIP_LEVEL];
    desc->max_anisotropy = sampler_states[WINED3D_SAMP_MAX_ANISOTROPY];
    if ((sampler_states[WINED3D_SAMP_MAG_FILTER] != WINED3D_TEXF_ANISOTROPIC
                && sampler_states[WINED3D_SAMP_MIN_FILTER] != WINED3D_TEXF_ANISOTROPIC
                && sampler_states[WINED3D_SAMP_MIP_FILTER] != WINED3D_TEXF_ANISOTROPIC)
            || (texture_gl->t.flags & WINED3D_TEXTURE_COND_NP2))
        desc->max_anisotropy = 1;
    desc->compare = texture_gl->t.resource.format_flags & WINED3DFMT_FLAG_SHADOW;
    desc->comparison_func = WINED3D_CMP_LESSEQUAL;
    desc->srgb_decode = is_srgb_enabled(sampler_states);

    if (!(texture_gl->t.resource.format_flags & WINED3DFMT_FLAG_FILTERING))
    {
        desc->mag_filter = WINED3D_TEXF_POINT;
        desc->min_filter = WINED3D_TEXF_POINT;
        desc->mip_filter = WINED3D_TEXF_NONE;
    }

    if (texture_gl->t.flags & WINED3D_TEXTURE_COND_NP2)
    {
        desc->mip_filter = WINED3D_TEXF_NONE;
        if (context_gl->gl_info->supported[WINED3D_GL_NORMALIZED_TEXRECT])
            desc->min_filter = WINED3D_TEXF_POINT;
    }
}

/* Enabling and disabling texture dimensions is done by texture stage state /
 * pixel shader setup, this function only has to bind textures and set the per
 * texture states. */
static void sampler(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    unsigned int sampler_idx = state_id - STATE_SAMPLER(0);
    unsigned int mapped_stage = context_gl->tex_unit_map[sampler_idx];
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    TRACE("Sampler %u.\n", sampler_idx);

    if (mapped_stage == WINED3D_UNMAPPED_STAGE)
    {
        TRACE("No sampler mapped to stage %u. Returning.\n", sampler_idx);
        return;
    }

    if (mapped_stage >= gl_info->limits.graphics_samplers)
        return;
    wined3d_context_gl_active_texture(context_gl, gl_info, mapped_stage);

    if (state->textures[sampler_idx])
    {
        struct wined3d_texture_gl *texture_gl = wined3d_texture_gl(state->textures[sampler_idx]);
        const DWORD *sampler_states = state->sampler_states[sampler_idx];
        struct wined3d_device *device = context->device;
        BOOL srgb = is_srgb_enabled(sampler_states);
        struct wined3d_sampler_desc desc;
        struct wined3d_sampler *sampler;
        struct wine_rb_entry *entry;

        wined3d_sampler_desc_from_sampler_states(&desc, context_gl, sampler_states, texture_gl);

        wined3d_texture_gl_bind(texture_gl, context_gl, srgb);

        if ((entry = wine_rb_get(&device->samplers, &desc)))
        {
            sampler = WINE_RB_ENTRY_VALUE(entry, struct wined3d_sampler, entry);
        }
        else
        {
            if (FAILED(wined3d_sampler_create(device, &desc, NULL, &wined3d_null_parent_ops, &sampler)))
            {
                ERR("Failed to create sampler.\n");
                return;
            }
            if (wine_rb_put(&device->samplers, &desc, &sampler->entry) == -1)
            {
                ERR("Failed to insert sampler.\n");
                wined3d_sampler_decref(sampler);
                return;
            }
        }

        wined3d_sampler_gl_bind(wined3d_sampler_gl(sampler), mapped_stage, texture_gl, context_gl);

        /* Trigger shader constant reloading (for NP2 texcoord fixup) */
        if (!(texture_gl->t.flags & WINED3D_TEXTURE_POW2_MAT_IDENT))
            context->constant_update_mask |= WINED3D_SHADER_CONST_PS_NP2_FIXUP;
    }
    else
    {
        wined3d_context_gl_bind_texture(context_gl, GL_NONE, 0);
        if (gl_info->supported[ARB_SAMPLER_OBJECTS])
        {
            GL_EXTCALL(glBindSampler(mapped_stage, 0));
            checkGLcall("glBindSampler");
        }
    }
}

void apply_pixelshader(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    unsigned int i;

    if (use_ps(state))
    {
        if (!context->last_was_pshader)
        {
            /* Former draw without a pixel shader, some samplers may be
             * disabled because of WINED3D_TSS_COLOR_OP = WINED3DTOP_DISABLE
             * make sure to enable them. */
            for (i = 0; i < WINED3D_MAX_FRAGMENT_SAMPLERS; ++i)
            {
                if (!isStateDirty(context, STATE_SAMPLER(i)))
                    sampler(context, state, STATE_SAMPLER(i));
            }
            context->last_was_pshader = TRUE;
        }
        else
        {
            /* Otherwise all samplers were activated by the code above in
             * earlier draws, or by sampler() if a different texture was
             * bound. I don't have to do anything. */
        }
    }
    else
    {
        /* Disabled the pixel shader - color ops weren't applied while it was
         * enabled, so re-apply them. */
        for (i = 0; i < context->d3d_info->limits.ffp_blend_stages; ++i)
        {
            if (!isStateDirty(context, STATE_TEXTURESTAGE(i, WINED3D_TSS_COLOR_OP)))
                context_apply_state(context, state, STATE_TEXTURESTAGE(i, WINED3D_TSS_COLOR_OP));
        }
        context->last_was_pshader = FALSE;
    }

    context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_PIXEL;
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

static void shader_bumpenv(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    context->constant_update_mask |= WINED3D_SHADER_CONST_PS_BUMP_ENV;
}

static void transform_world(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    struct wined3d_matrix mat;

    /* This function is called by transform_view below if the view matrix was changed too
     *
     * Deliberately no check if the vertex declaration is dirty because the vdecl state
     * does not always update the world matrix, only on a switch between transformed
     * and untransformed draws. It *may* happen that the world matrix is set 2 times during one
     * draw, but that should be rather rare and cheaper in total.
     */
    gl_info->gl_ops.gl.p_glMatrixMode(GL_MODELVIEW);
    checkGLcall("glMatrixMode");

    get_modelview_matrix(context, state, 0, &mat);

    gl_info->gl_ops.gl.p_glLoadMatrixf((GLfloat *)&mat);
    checkGLcall("glLoadMatrixf");
}

void clipplane(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    UINT index = state_id - STATE_CLIPPLANE(0);
    GLdouble plane[4];

    if (isStateDirty(context, STATE_TRANSFORM(WINED3D_TS_VIEW)) || index >= gl_info->limits.user_clip_distances)
        return;

    gl_info->gl_ops.gl.p_glMatrixMode(GL_MODELVIEW);
    gl_info->gl_ops.gl.p_glPushMatrix();

    /* Clip Plane settings are affected by the model view in OpenGL, the View transform in direct3d */
    if (!use_vs(state))
        gl_info->gl_ops.gl.p_glLoadMatrixf(&state->transforms[WINED3D_TS_VIEW]._11);
    else
        /* With vertex shaders, clip planes are not transformed in Direct3D,
         * while in OpenGL they are still transformed by the model view matrix. */
        gl_info->gl_ops.gl.p_glLoadIdentity();

    plane[0] = state->clip_planes[index].x;
    plane[1] = state->clip_planes[index].y;
    plane[2] = state->clip_planes[index].z;
    plane[3] = state->clip_planes[index].w;

    TRACE("Clipplane [%.8e, %.8e, %.8e, %.8e]\n",
            plane[0], plane[1], plane[2], plane[3]);
    gl_info->gl_ops.gl.p_glClipPlane(GL_CLIP_PLANE0 + index, plane);
    checkGLcall("glClipPlane");

    gl_info->gl_ops.gl.p_glPopMatrix();
}

static void transform_worldex(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    unsigned int matrix = state_id - STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(0));

    WARN("Unsupported world matrix %u set.\n", matrix);
}

static void state_vertexblend_w(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    enum wined3d_vertex_blend_flags f = state->render_states[WINED3D_RS_VERTEXBLEND];
    static unsigned int once;

    if (f == WINED3D_VBF_DISABLE)
        return;

    if (!once++) FIXME("Vertex blend flags %#x not supported.\n", f);
    else WARN("Vertex blend flags %#x not supported.\n", f);
}

static void transform_view(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    const struct wined3d_light_info *light = NULL;
    unsigned int k;

    /* If we are changing the View matrix, reset the light and clipping planes to the new view
     * NOTE: We have to reset the positions even if the light/plane is not currently
     *       enabled, since the call to enable it will not reset the position.
     * NOTE2: Apparently texture transforms do NOT need reapplying
     */

    gl_info->gl_ops.gl.p_glMatrixMode(GL_MODELVIEW);
    checkGLcall("glMatrixMode(GL_MODELVIEW)");
    gl_info->gl_ops.gl.p_glLoadMatrixf(&state->transforms[WINED3D_TS_VIEW]._11);
    checkGLcall("glLoadMatrixf(...)");

    /* Reset lights. TODO: Call light apply func */
    for (k = 0; k < gl_info->limits.lights; ++k)
    {
        if (!(light = state->light_state.lights[k]))
            continue;
        if (light->OriginalParms.type == WINED3D_LIGHT_DIRECTIONAL)
            gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT0 + light->glIndex, GL_POSITION, &light->direction.x);
        else
            gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT0 + light->glIndex, GL_POSITION, &light->position.x);
        checkGLcall("glLightfv posn");
        gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT0 + light->glIndex, GL_SPOT_DIRECTION, &light->direction.x);
        checkGLcall("glLightfv dirn");
    }

    /* Reset Clipping Planes  */
    for (k = 0; k < gl_info->limits.user_clip_distances; ++k)
    {
        if (!isStateDirty(context, STATE_CLIPPLANE(k)))
            clipplane(context, state, STATE_CLIPPLANE(k));
    }

    if (context->last_was_rhw)
    {
        gl_info->gl_ops.gl.p_glLoadIdentity();
        checkGLcall("glLoadIdentity()");
        /* No need to update the world matrix, the identity is fine */
        return;
    }

    /* Call the world matrix state, this will apply the combined WORLD + VIEW matrix
     * No need to do it here if the state is scheduled for update. */
    if (!isStateDirty(context, STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(0))))
        transform_world(context, state, STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(0)));
}

static void transform_projection(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    struct wined3d_matrix projection;

    gl_info->gl_ops.gl.p_glMatrixMode(GL_PROJECTION);
    checkGLcall("glMatrixMode(GL_PROJECTION)");

    get_projection_matrix(context, state, &projection);
    gl_info->gl_ops.gl.p_glLoadMatrixf(&projection._11);
    checkGLcall("glLoadMatrixf");
}

static void streamsrc(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (isStateDirty(context, STATE_VDECL))
        return;
    wined3d_context_gl_update_stream_sources(wined3d_context_gl(context), state);
}

static void vdecl_miscpart(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (isStateDirty(context, STATE_STREAMSRC))
        return;
    wined3d_context_gl_update_stream_sources(wined3d_context_gl(context), state);
}

static void vertexdeclaration(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    BOOL useVertexShaderFunction = use_vs(state);
    BOOL updateFog = FALSE;
    BOOL transformed;
    BOOL wasrhw = context->last_was_rhw;
    unsigned int i;

    transformed = context->stream_info.position_transformed;
    if (transformed != context->last_was_rhw && !useVertexShaderFunction)
        updateFog = TRUE;

    context->last_was_rhw = transformed;

    if (context->stream_info.swizzle_map != context->last_swizzle_map)
        context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_VERTEX;

    context->last_swizzle_map = context->stream_info.swizzle_map;

    /* Don't have to apply the matrices when vertex shaders are used. When
     * vshaders are turned off this function will be called again anyway to
     * make sure they're properly set. */
    if (!useVertexShaderFunction)
    {
        /* TODO: Move this mainly to the viewport state and only apply when
         * the vp has changed or transformed / untransformed was switched. */
        if (wasrhw != context->last_was_rhw
                && !isStateDirty(context, STATE_TRANSFORM(WINED3D_TS_PROJECTION))
                && !isStateDirty(context, STATE_VIEWPORT))
            transform_projection(context, state, STATE_TRANSFORM(WINED3D_TS_PROJECTION));
        /* World matrix needs reapplication here only if we're switching between rhw and non-rhw
         * mode.
         *
         * If a vertex shader is used, the world matrix changed and then vertex shader unbound
         * this check will fail and the matrix not applied again. This is OK because a simple
         * world matrix change reapplies the matrix - These checks here are only to satisfy the
         * needs of the vertex declaration.
         *
         * World and view matrix go into the same gl matrix, so only apply them when neither is
         * dirty
         */
        if (transformed != wasrhw && !isStateDirty(context, STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(0)))
                && !isStateDirty(context, STATE_TRANSFORM(WINED3D_TS_VIEW)))
            transform_world(context, state, STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(0)));
        if (!isStateDirty(context, STATE_RENDER(WINED3D_RS_COLORVERTEX)))
            context_apply_state(context, state, STATE_RENDER(WINED3D_RS_COLORVERTEX));
        if (!isStateDirty(context, STATE_RENDER(WINED3D_RS_LIGHTING)))
            state_lighting(context, state, STATE_RENDER(WINED3D_RS_LIGHTING));

        if (context->last_was_vshader)
        {
            updateFog = TRUE;

            if (!context->d3d_info->vs_clipping
                    && !isStateDirty(context, STATE_RENDER(WINED3D_RS_CLIPPLANEENABLE)))
            {
                state_clipping(context, state, STATE_RENDER(WINED3D_RS_CLIPPLANEENABLE));
            }

            for (i = 0; i < gl_info->limits.user_clip_distances; ++i)
            {
                clipplane(context, state, STATE_CLIPPLANE(i));
            }
        }
        if (!isStateDirty(context, STATE_RENDER(WINED3D_RS_NORMALIZENORMALS)))
            state_normalize(context, state, STATE_RENDER(WINED3D_RS_NORMALIZENORMALS));
    }
    else
    {
        if (!context->last_was_vshader)
        {
            static BOOL warned = FALSE;
            if (!context->d3d_info->vs_clipping)
            {
                /* Disable all clip planes to get defined results on all drivers. See comment in the
                 * state_clipping state handler
                 */
                wined3d_context_gl_enable_clip_distances(context_gl, 0);

                if (!warned && state->render_states[WINED3D_RS_CLIPPLANEENABLE])
                {
                    FIXME("Clipping not supported with vertex shaders.\n");
                    warned = TRUE;
                }
            }
            if (wasrhw)
            {
                /* Apply the transform matrices when switching from rhw
                 * drawing to vertex shaders. Vertex shaders themselves do
                 * not need it, but the matrices are not reapplied
                 * automatically when switching back from vertex shaders to
                 * fixed function processing. So make sure we leave the fixed
                 * function vertex processing states back in a sane state
                 * before switching to shaders. */
                if (!isStateDirty(context, STATE_TRANSFORM(WINED3D_TS_PROJECTION)))
                    transform_projection(context, state, STATE_TRANSFORM(WINED3D_TS_PROJECTION));
                if (!isStateDirty(context, STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(0))))
                    transform_world(context, state, STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(0)));
            }
            updateFog = TRUE;

            /* Vertex shader clipping ignores the view matrix. Update all clipplanes
             * (Note: ARB shaders can read the clip planes for clipping emulation even if
             * device->vs_clipping is false.
             */
            for (i = 0; i < gl_info->limits.user_clip_distances; ++i)
            {
                clipplane(context, state, STATE_CLIPPLANE(i));
            }
        }
    }

    context->last_was_vshader = useVertexShaderFunction;
    context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_VERTEX;

    if (updateFog)
        context_apply_state(context, state, STATE_RENDER(WINED3D_RS_FOGVERTEXMODE));

    if (!useVertexShaderFunction)
    {
        unsigned int i;

        for (i = 0; i < WINED3D_MAX_TEXTURES; ++i)
        {
            if (!isStateDirty(context, STATE_TRANSFORM(WINED3D_TS_TEXTURE0 + i)))
                transform_texture(context, state, STATE_TEXTURESTAGE(i, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS));
        }

        if (use_ps(state) && state->shader[WINED3D_SHADER_TYPE_PIXEL]->reg_maps.shader_version.major == 1
                && state->shader[WINED3D_SHADER_TYPE_PIXEL]->reg_maps.shader_version.minor <= 3)
            context->shader_update_mask |= 1u << WINED3D_SHADER_TYPE_PIXEL;
    }
}

static void get_viewports(struct wined3d_context *context, const struct wined3d_state *state,
        unsigned int viewport_count, struct wined3d_viewport *viewports)
{
    const struct wined3d_rendertarget_view *depth_stencil = state->fb->depth_stencil;
    const struct wined3d_rendertarget_view *target = state->fb->render_targets[0];
    unsigned int width, height, i;

    for (i = 0; i < viewport_count; ++i)
        viewports[i] = state->viewports[i];

    /* Note: GL uses a lower left origin while DirectX uses upper left. This
     * is reversed when using offscreen rendering. */
    if (context->render_offscreen)
        return;

    if (target)
    {
        wined3d_rendertarget_view_get_drawable_size(target, context, &width, &height);
    }
    else if (depth_stencil)
    {
        height = depth_stencil->height;
    }
    else
    {
        FIXME("Could not get the height of render targets.\n");
        return;
    }

    for (i = 0; i < viewport_count; ++i)
        viewports[i].y = height - (viewports[i].y + viewports[i].height);
}

static void viewport_miscpart(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    struct wined3d_viewport vp[WINED3D_MAX_VIEWPORTS];

    if (gl_info->supported[ARB_VIEWPORT_ARRAY])
    {
        GLdouble depth_ranges[2 * WINED3D_MAX_VIEWPORTS];
        GLfloat viewports[4 * WINED3D_MAX_VIEWPORTS];

        unsigned int i, reset_count = 0;

        get_viewports(context, state, state->viewport_count, vp);
        for (i = 0; i < state->viewport_count; ++i)
        {
            depth_ranges[i * 2]     = vp[i].min_z;
            depth_ranges[i * 2 + 1] = vp[i].max_z;

            viewports[i * 4]     = vp[i].x;
            viewports[i * 4 + 1] = vp[i].y;
            viewports[i * 4 + 2] = vp[i].width;
            viewports[i * 4 + 3] = vp[i].height;
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
        get_viewports(context, state, 1, vp);
        gl_info->gl_ops.gl.p_glDepthRange(vp[0].min_z, vp[0].max_z);
        gl_info->gl_ops.gl.p_glViewport(vp[0].x, vp[0].y, vp[0].width, vp[0].height);
    }
    checkGLcall("setting clip space and viewport");
}

static void viewport_miscpart_cc(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    /* See get_projection_matrix() in utils.c for a discussion about those values. */
    float pixel_center_offset = context->d3d_info->wined3d_creation_flags
            & WINED3D_PIXEL_CENTER_INTEGER ? 63.0f / 128.0f : -1.0f / 128.0f;
    struct wined3d_viewport vp[WINED3D_MAX_VIEWPORTS];
    GLdouble depth_ranges[2 * WINED3D_MAX_VIEWPORTS];
    GLfloat viewports[4 * WINED3D_MAX_VIEWPORTS];
    unsigned int i, reset_count = 0;

    get_viewports(context, state, state->viewport_count, vp);

    GL_EXTCALL(glClipControl(context->render_offscreen ? GL_UPPER_LEFT : GL_LOWER_LEFT, GL_ZERO_TO_ONE));

    for (i = 0; i < state->viewport_count; ++i)
    {
        depth_ranges[i * 2]     = vp[i].min_z;
        depth_ranges[i * 2 + 1] = vp[i].max_z;

        viewports[i * 4] = vp[i].x + pixel_center_offset;
        viewports[i * 4 + 1] = vp[i].y + pixel_center_offset;
        viewports[i * 4 + 2] = vp[i].width;
        viewports[i * 4 + 3] = vp[i].height;
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

static void viewport_vertexpart(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    if (!isStateDirty(context, STATE_TRANSFORM(WINED3D_TS_PROJECTION)))
        transform_projection(context, state, STATE_TRANSFORM(WINED3D_TS_PROJECTION));
    if (!isStateDirty(context, STATE_RENDER(WINED3D_RS_POINTSCALEENABLE))
            && state->render_states[WINED3D_RS_POINTSCALEENABLE])
        state_pscale(context, state, STATE_RENDER(WINED3D_RS_POINTSCALEENABLE));
    /* Update the position fixup. */
    context->constant_update_mask |= WINED3D_SHADER_CONST_POS_FIXUP;
}

static void light(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    UINT Index = state_id - STATE_ACTIVELIGHT(0);
    const struct wined3d_light_info *lightInfo = state->light_state.lights[Index];

    if (!lightInfo)
    {
        gl_info->gl_ops.gl.p_glDisable(GL_LIGHT0 + Index);
        checkGLcall("glDisable(GL_LIGHT0 + Index)");
    }
    else
    {
        float quad_att;

        /* Light settings are affected by the model view in OpenGL, the View transform in direct3d*/
        gl_info->gl_ops.gl.p_glMatrixMode(GL_MODELVIEW);
        gl_info->gl_ops.gl.p_glPushMatrix();
        gl_info->gl_ops.gl.p_glLoadMatrixf(&state->transforms[WINED3D_TS_VIEW]._11);

        gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT0 + Index, GL_DIFFUSE, &lightInfo->OriginalParms.diffuse.r);
        gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT0 + Index, GL_SPECULAR, &lightInfo->OriginalParms.specular.r);
        gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT0 + Index, GL_AMBIENT, &lightInfo->OriginalParms.ambient.r);
        checkGLcall("glLightfv");

        if ((lightInfo->OriginalParms.range * lightInfo->OriginalParms.range) >= FLT_MIN)
            quad_att = 1.4f / (lightInfo->OriginalParms.range * lightInfo->OriginalParms.range);
        else
            quad_att = 0.0f; /*  0 or  MAX?  (0 seems to be ok) */

        /* Do not assign attenuation values for lights that do not use them. D3D apps are free to pass any junk,
         * but gl drivers use them and may crash due to bad Attenuation values. Need for Speed most wanted sets
         * Attenuation0 to NaN and crashes in the gl lib
         */

        switch (lightInfo->OriginalParms.type)
        {
            case WINED3D_LIGHT_POINT:
                /* Position */
                gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT0 + Index, GL_POSITION, &lightInfo->position.x);
                checkGLcall("glLightfv");
                gl_info->gl_ops.gl.p_glLightf(GL_LIGHT0 + Index, GL_SPOT_CUTOFF, lightInfo->cutoff);
                checkGLcall("glLightf");
                gl_info->gl_ops.gl.p_glLightf(GL_LIGHT0 + Index, GL_CONSTANT_ATTENUATION,
                        lightInfo->OriginalParms.attenuation0);
                checkGLcall("glLightf");
                gl_info->gl_ops.gl.p_glLightf(GL_LIGHT0 + Index, GL_LINEAR_ATTENUATION,
                        lightInfo->OriginalParms.attenuation1);
                checkGLcall("glLightf");
                if (quad_att < lightInfo->OriginalParms.attenuation2)
                    quad_att = lightInfo->OriginalParms.attenuation2;
                gl_info->gl_ops.gl.p_glLightf(GL_LIGHT0 + Index, GL_QUADRATIC_ATTENUATION, quad_att);
                checkGLcall("glLightf");
                /* FIXME: Range */
                break;

            case WINED3D_LIGHT_SPOT:
                /* Position */
                gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT0 + Index, GL_POSITION, &lightInfo->position.x);
                checkGLcall("glLightfv");
                /* Direction */
                gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT0 + Index, GL_SPOT_DIRECTION, &lightInfo->direction.x);
                checkGLcall("glLightfv");
                gl_info->gl_ops.gl.p_glLightf(GL_LIGHT0 + Index, GL_SPOT_EXPONENT, lightInfo->exponent);
                checkGLcall("glLightf");
                gl_info->gl_ops.gl.p_glLightf(GL_LIGHT0 + Index, GL_SPOT_CUTOFF, lightInfo->cutoff);
                checkGLcall("glLightf");
                gl_info->gl_ops.gl.p_glLightf(GL_LIGHT0 + Index, GL_CONSTANT_ATTENUATION,
                        lightInfo->OriginalParms.attenuation0);
                checkGLcall("glLightf");
                gl_info->gl_ops.gl.p_glLightf(GL_LIGHT0 + Index, GL_LINEAR_ATTENUATION,
                        lightInfo->OriginalParms.attenuation1);
                checkGLcall("glLightf");
                if (quad_att < lightInfo->OriginalParms.attenuation2)
                    quad_att = lightInfo->OriginalParms.attenuation2;
                gl_info->gl_ops.gl.p_glLightf(GL_LIGHT0 + Index, GL_QUADRATIC_ATTENUATION, quad_att);
                checkGLcall("glLightf");
                /* FIXME: Range */
                break;

            case WINED3D_LIGHT_DIRECTIONAL:
                /* Direction */
                /* Note GL uses w position of 0 for direction! */
                gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT0 + Index, GL_POSITION, &lightInfo->direction.x);
                checkGLcall("glLightfv");
                gl_info->gl_ops.gl.p_glLightf(GL_LIGHT0 + Index, GL_SPOT_CUTOFF, lightInfo->cutoff);
                checkGLcall("glLightf");
                gl_info->gl_ops.gl.p_glLightf(GL_LIGHT0 + Index, GL_SPOT_EXPONENT, 0.0f);
                checkGLcall("glLightf");
                break;

            default:
                FIXME("Unrecognized light type %#x.\n", lightInfo->OriginalParms.type);
        }

        /* Restore the modelview matrix */
        gl_info->gl_ops.gl.p_glPopMatrix();

        gl_info->gl_ops.gl.p_glEnable(GL_LIGHT0 + Index);
        checkGLcall("glEnable(GL_LIGHT0 + Index)");
    }
}

static void scissorrect(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    unsigned int height = 0;
    const RECT *r;

    /* Warning: glScissor uses window coordinates, not viewport coordinates,
     * so our viewport correction does not apply. Warning2: Even in windowed
     * mode the coords are relative to the window, not the screen. */

    if (!context->render_offscreen)
    {
        const struct wined3d_rendertarget_view *target = state->fb->render_targets[0];
        unsigned int width;

        wined3d_rendertarget_view_get_drawable_size(target, context, &width, &height);
    }

    if (gl_info->supported[ARB_VIEWPORT_ARRAY])
    {
        GLint sr[4 * WINED3D_MAX_VIEWPORTS];
        unsigned int i, reset_count = 0;

        for (i = 0; i < state->scissor_rect_count; ++i)
        {
            r = &state->scissor_rects[i];

            sr[i * 4] = r->left;
            sr[i * 4 + 1] = height ? height - r->top : r->top;
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
        gl_info->gl_ops.gl.p_glScissor(r->left, height ? height - r->top : r->top,
                r->right - r->left, r->bottom - r->top);
        checkGLcall("glScissor");
    }
}

static void indexbuffer(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    const struct wined3d_stream_info *stream_info = &context->stream_info;
    const struct wined3d_buffer *ib = state->index_buffer;

    if (!ib || !stream_info->all_vbo)
        GL_EXTCALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    else
        GL_EXTCALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib->buffer_object));
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
    GLenum mode;

    mode = state->rasterizer_state && state->rasterizer_state->desc.front_ccw ? GL_CCW : GL_CW;
    if (context->render_offscreen)
        mode = (mode == GL_CW) ? GL_CCW : GL_CW;

    gl_info->gl_ops.gl.p_glFrontFace(mode);
    checkGLcall("glFrontFace");
    if (!isStateDirty(context, STATE_RENDER(WINED3D_RS_DEPTHBIAS)))
        state_depthbias(context, state, STATE_RENDER(WINED3D_RS_DEPTHBIAS));
    depth_clip(state->rasterizer_state, gl_info);
}

static void rasterizer_cc(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    GLenum mode;

    mode = state->rasterizer_state && state->rasterizer_state->desc.front_ccw ? GL_CCW : GL_CW;

    gl_info->gl_ops.gl.p_glFrontFace(mode);
    checkGLcall("glFrontFace");
    if (!isStateDirty(context, STATE_RENDER(WINED3D_RS_DEPTHBIAS)))
        state_depthbias(context, state, STATE_RENDER(WINED3D_RS_DEPTHBIAS));
    depth_clip(state->rasterizer_state, gl_info);
}

static void psorigin_w(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    static BOOL warned;

    if (!warned)
    {
        WARN("Point sprite coordinate origin switching not supported.\n");
        warned = TRUE;
    }
}

static void psorigin(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;
    GLint origin = context->render_offscreen ? GL_LOWER_LEFT : GL_UPPER_LEFT;

    GL_EXTCALL(glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, origin));
    checkGLcall("glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, ...)");
}

void state_srgbwrite(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_gl_info *gl_info = wined3d_context_gl(context)->gl_info;

    TRACE("context %p, state %p, state_id %#x.\n", context, state, state_id);

    if (needs_srgb_write(context, state, state->fb))
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

    TRACE("context %p, state %p, state_id %#x.\n", context, state, state_id);

    if (STATE_IS_GRAPHICS_CONSTANT_BUFFER(state_id))
        shader_type = state_id - STATE_GRAPHICS_CONSTANT_BUFFER(0);
    else
        shader_type = WINED3D_SHADER_TYPE_COMPUTE;

    wined3d_gl_limits_get_uniform_block_range(&gl_info->limits, shader_type, &base, &count);
    for (i = 0; i < count; ++i)
    {
        buffer = state->cb[shader_type][i];
        GL_EXTCALL(glBindBufferBase(GL_UNIFORM_BUFFER, base + i, buffer ? buffer->buffer_object : 0));
    }
    checkGLcall("bind constant buffers");
}

static void state_cb_warn(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    TRACE("context %p, state %p, state_id %#x.\n", context, state, state_id);

    WARN("Constant buffers (%s) no supported.\n", debug_d3dstate(state_id));
}

static void state_shader_resource_binding(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    TRACE("context %p, state %p, state_id %#x.\n", context, state, state_id);

    context->update_shader_resource_bindings = 1;
}

static void state_cs_resource_binding(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    TRACE("context %p, state %p, state_id %#x.\n", context, state, state_id);
    context->update_compute_shader_resource_bindings = 1;
}

static void state_uav_binding(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    TRACE("context %p, state %p, state_id %#x.\n", context, state, state_id);
    context->update_unordered_access_view_bindings = 1;
}

static void state_cs_uav_binding(struct wined3d_context *context,
        const struct wined3d_state *state, DWORD state_id)
{
    TRACE("context %p, state %p, state_id %#x.\n", context, state, state_id);
    context->update_compute_unordered_access_view_bindings = 1;
}

static void state_uav_warn(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    WARN("ARB_image_load_store is not supported by OpenGL implementation.\n");
}

static void state_so(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_buffer *buffer;
    unsigned int offset, size, i;

    TRACE("context %p, state %p, state_id %#x.\n", context, state, state_id);

    wined3d_context_gl_end_transform_feedback(context_gl);

    for (i = 0; i < ARRAY_SIZE(state->stream_output); ++i)
    {
        if (!(buffer = state->stream_output[i].buffer))
        {
            GL_EXTCALL(glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, i, 0));
            continue;
        }

        offset = state->stream_output[i].offset;
        if (offset == ~0u)
        {
            FIXME("Appending to stream output buffers not implemented.\n");
            offset = 0;
        }
        size = buffer->resource.size - offset;
        GL_EXTCALL(glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, i, buffer->buffer_object, offset, size));
    }
    checkGLcall("bind transform feedback buffers");
}

static void state_so_warn(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    WARN("Transform feedback not supported.\n");
}

const struct wined3d_state_entry_template misc_state_template[] =
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
    { STATE_RENDER(WINED3D_RS_SRCBLEND),                  { STATE_RENDER(WINED3D_RS_ALPHABLENDENABLE),          NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_DESTBLEND),                 { STATE_RENDER(WINED3D_RS_ALPHABLENDENABLE),          NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_ALPHABLENDENABLE),          { STATE_RENDER(WINED3D_RS_ALPHABLENDENABLE),          state_blend         }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_EDGEANTIALIAS),             { STATE_RENDER(WINED3D_RS_EDGEANTIALIAS),             state_line_antialias}, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_ANTIALIASEDLINEENABLE),     { STATE_RENDER(WINED3D_RS_ANTIALIASEDLINEENABLE),     state_line_antialias}, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_SEPARATEALPHABLENDENABLE),  { STATE_RENDER(WINED3D_RS_ALPHABLENDENABLE),          NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_SRCBLENDALPHA),             { STATE_RENDER(WINED3D_RS_ALPHABLENDENABLE),          NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_DESTBLENDALPHA),            { STATE_RENDER(WINED3D_RS_ALPHABLENDENABLE),          NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_DESTBLENDALPHA),            { STATE_RENDER(WINED3D_RS_ALPHABLENDENABLE),          NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_BLENDOPALPHA),              { STATE_RENDER(WINED3D_RS_ALPHABLENDENABLE),          NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_BLEND,                                        { STATE_BLEND,                                        state_blend_object  }, WINED3D_GL_EXT_NONE             },
    { STATE_BLEND_FACTOR,                                 { STATE_BLEND_FACTOR,                                 state_blend_factor  }, EXT_BLEND_COLOR                 },
    { STATE_BLEND_FACTOR,                                 { STATE_BLEND_FACTOR,                                 state_blend_factor_w}, WINED3D_GL_EXT_NONE             },
    { STATE_STREAMSRC,                                    { STATE_STREAMSRC,                                    streamsrc           }, WINED3D_GL_EXT_NONE             },
    { STATE_VDECL,                                        { STATE_VDECL,                                        vdecl_miscpart      }, WINED3D_GL_EXT_NONE             },
    { STATE_RASTERIZER,                                   { STATE_RASTERIZER,                                   rasterizer_cc       }, ARB_CLIP_CONTROL                },
    { STATE_RASTERIZER,                                   { STATE_RASTERIZER,                                   rasterizer          }, WINED3D_GL_EXT_NONE             },
    { STATE_SCISSORRECT,                                  { STATE_SCISSORRECT,                                  scissorrect         }, WINED3D_GL_EXT_NONE             },
    { STATE_POINTSPRITECOORDORIGIN,                       { STATE_POINTSPRITECOORDORIGIN,                       state_nop           }, ARB_CLIP_CONTROL                },
    { STATE_POINTSPRITECOORDORIGIN,                       { STATE_POINTSPRITECOORDORIGIN,                       psorigin            }, WINED3D_GL_VERSION_2_0          },
    { STATE_POINTSPRITECOORDORIGIN,                       { STATE_POINTSPRITECOORDORIGIN,                       psorigin_w          }, WINED3D_GL_EXT_NONE             },

    /* TODO: Move shader constant loading to vertex and fragment pipeline respectively, as soon as the pshader and
     * vshader loadings are untied from each other
     */
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT00),   { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT00),   shader_bumpenv      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT01),   { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT10),   { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT11),   { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT00),   { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT00),   shader_bumpenv      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT01),   { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT10),   { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT11),   { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT00),   { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT00),   shader_bumpenv      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT01),   { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT10),   { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT11),   { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT00),   { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT00),   shader_bumpenv      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT01),   { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT10),   { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT11),   { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT00),   { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT00),   shader_bumpenv      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT01),   { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT10),   { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT11),   { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT00),   { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT00),   shader_bumpenv      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT01),   { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT10),   { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT11),   { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT00),   { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT00),   shader_bumpenv      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT01),   { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT10),   { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT11),   { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT00),   { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT00),   shader_bumpenv      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT01),   { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT10),   { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT11),   { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_MAT00),   NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_LSCALE),  { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_LSCALE),  shader_bumpenv      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_LOFFSET), { STATE_TEXTURESTAGE(0, WINED3D_TSS_BUMPENV_LSCALE),  NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_LSCALE),  { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_LSCALE),  shader_bumpenv      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_LOFFSET), { STATE_TEXTURESTAGE(1, WINED3D_TSS_BUMPENV_LSCALE),  NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_LSCALE),  { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_LSCALE),  shader_bumpenv      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_LOFFSET), { STATE_TEXTURESTAGE(2, WINED3D_TSS_BUMPENV_LSCALE),  NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_LSCALE),  { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_LSCALE),  shader_bumpenv      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_LOFFSET), { STATE_TEXTURESTAGE(3, WINED3D_TSS_BUMPENV_LSCALE),  NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_LSCALE),  { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_LSCALE),  shader_bumpenv      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_LOFFSET), { STATE_TEXTURESTAGE(4, WINED3D_TSS_BUMPENV_LSCALE),  NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_LSCALE),  { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_LSCALE),  shader_bumpenv      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_LOFFSET), { STATE_TEXTURESTAGE(5, WINED3D_TSS_BUMPENV_LSCALE),  NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_LSCALE),  { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_LSCALE),  shader_bumpenv      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_LOFFSET), { STATE_TEXTURESTAGE(6, WINED3D_TSS_BUMPENV_LSCALE),  NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_LSCALE),  { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_LSCALE),  shader_bumpenv      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_LOFFSET), { STATE_TEXTURESTAGE(7, WINED3D_TSS_BUMPENV_LSCALE),  NULL                }, WINED3D_GL_EXT_NONE             },

    { STATE_VIEWPORT,                                     { STATE_VIEWPORT,                                     viewport_miscpart_cc}, ARB_CLIP_CONTROL                },
    { STATE_VIEWPORT,                                     { STATE_VIEWPORT,                                     viewport_miscpart   }, WINED3D_GL_EXT_NONE             },
    { STATE_INDEXBUFFER,                                  { STATE_INDEXBUFFER,                                  indexbuffer         }, ARB_VERTEX_BUFFER_OBJECT        },
    { STATE_INDEXBUFFER,                                  { STATE_INDEXBUFFER,                                  state_nop           }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_ANTIALIAS),                 { STATE_RENDER(WINED3D_RS_ANTIALIAS),                 state_antialias     }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_TEXTUREPERSPECTIVE),        { STATE_RENDER(WINED3D_RS_TEXTUREPERSPECTIVE),        state_nop           }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_ZENABLE),                   { STATE_RENDER(WINED3D_RS_ZENABLE),                   state_zenable       }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_WRAPU),                     { STATE_RENDER(WINED3D_RS_WRAPU),                     state_wrapu         }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_WRAPV),                     { STATE_RENDER(WINED3D_RS_WRAPV),                     state_wrapv         }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_FILLMODE),                  { STATE_RENDER(WINED3D_RS_FILLMODE),                  state_fillmode      }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_LINEPATTERN),               { STATE_RENDER(WINED3D_RS_LINEPATTERN),               state_linepattern   }, WINED3D_GL_LEGACY_CONTEXT       },
    { STATE_RENDER(WINED3D_RS_LINEPATTERN),               { STATE_RENDER(WINED3D_RS_LINEPATTERN),               state_linepattern_w }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_MONOENABLE),                { STATE_RENDER(WINED3D_RS_MONOENABLE),                state_monoenable    }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_ROP2),                      { STATE_RENDER(WINED3D_RS_ROP2),                      state_rop2          }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_PLANEMASK),                 { STATE_RENDER(WINED3D_RS_PLANEMASK),                 state_planemask     }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_ZWRITEENABLE),              { STATE_RENDER(WINED3D_RS_ZWRITEENABLE),              state_zwriteenable  }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_LASTPIXEL),                 { STATE_RENDER(WINED3D_RS_LASTPIXEL),                 state_lastpixel     }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_CULLMODE),                  { STATE_RENDER(WINED3D_RS_CULLMODE),                  state_cullmode      }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_ZFUNC),                     { STATE_RENDER(WINED3D_RS_ZFUNC),                     state_zfunc         }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_DITHERENABLE),              { STATE_RENDER(WINED3D_RS_DITHERENABLE),              state_ditherenable  }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_SUBPIXEL),                  { STATE_RENDER(WINED3D_RS_SUBPIXEL),                  state_subpixel      }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_SUBPIXELX),                 { STATE_RENDER(WINED3D_RS_SUBPIXELX),                 state_subpixelx     }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_STIPPLEDALPHA),             { STATE_RENDER(WINED3D_RS_STIPPLEDALPHA),             state_stippledalpha }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_STIPPLEENABLE),             { STATE_RENDER(WINED3D_RS_STIPPLEENABLE),             state_stippleenable }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_MIPMAPLODBIAS),             { STATE_RENDER(WINED3D_RS_MIPMAPLODBIAS),             state_mipmaplodbias }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_ANISOTROPY),                { STATE_RENDER(WINED3D_RS_ANISOTROPY),                state_anisotropy    }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_FLUSHBATCH),                { STATE_RENDER(WINED3D_RS_FLUSHBATCH),                state_flushbatch    }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_TRANSLUCENTSORTINDEPENDENT),{ STATE_RENDER(WINED3D_RS_TRANSLUCENTSORTINDEPENDENT),state_translucentsi }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_STENCILENABLE),             { STATE_RENDER(WINED3D_RS_STENCILENABLE),             state_stencil       }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_STENCILFAIL),               { STATE_RENDER(WINED3D_RS_STENCILENABLE),             NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_STENCILZFAIL),              { STATE_RENDER(WINED3D_RS_STENCILENABLE),             NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_STENCILPASS),               { STATE_RENDER(WINED3D_RS_STENCILENABLE),             NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_STENCILFUNC),               { STATE_RENDER(WINED3D_RS_STENCILENABLE),             NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_STENCILREF),                { STATE_RENDER(WINED3D_RS_STENCILENABLE),             NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_STENCILMASK),               { STATE_RENDER(WINED3D_RS_STENCILENABLE),             NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_STENCILWRITEMASK),          { STATE_RENDER(WINED3D_RS_STENCILWRITEMASK),          state_stencilwrite2s_ext}, EXT_STENCIL_TWO_SIDE        },
    { STATE_RENDER(WINED3D_RS_STENCILWRITEMASK),          { STATE_RENDER(WINED3D_RS_STENCILWRITEMASK),          state_stencilwrite  }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_TWOSIDEDSTENCILMODE),       { STATE_RENDER(WINED3D_RS_STENCILENABLE),             NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_BACK_STENCILFAIL),          { STATE_RENDER(WINED3D_RS_STENCILENABLE),             NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_BACK_STENCILZFAIL),         { STATE_RENDER(WINED3D_RS_STENCILENABLE),             NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_BACK_STENCILPASS),          { STATE_RENDER(WINED3D_RS_STENCILENABLE),             NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_BACK_STENCILFUNC),          { STATE_RENDER(WINED3D_RS_STENCILENABLE),             NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_WRAP0),                     { STATE_RENDER(WINED3D_RS_WRAP0),                     state_wrap          }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_WRAP1),                     { STATE_RENDER(WINED3D_RS_WRAP0),                     NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_WRAP2),                     { STATE_RENDER(WINED3D_RS_WRAP0),                     NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_WRAP3),                     { STATE_RENDER(WINED3D_RS_WRAP0),                     NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_WRAP4),                     { STATE_RENDER(WINED3D_RS_WRAP0),                     NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_WRAP5),                     { STATE_RENDER(WINED3D_RS_WRAP0),                     NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_WRAP6),                     { STATE_RENDER(WINED3D_RS_WRAP0),                     NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_WRAP7),                     { STATE_RENDER(WINED3D_RS_WRAP0),                     NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_WRAP8),                     { STATE_RENDER(WINED3D_RS_WRAP0),                     NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_WRAP9),                     { STATE_RENDER(WINED3D_RS_WRAP0),                     NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_WRAP10),                    { STATE_RENDER(WINED3D_RS_WRAP0),                     NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_WRAP11),                    { STATE_RENDER(WINED3D_RS_WRAP0),                     NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_WRAP12),                    { STATE_RENDER(WINED3D_RS_WRAP0),                     NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_WRAP13),                    { STATE_RENDER(WINED3D_RS_WRAP0),                     NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_WRAP14),                    { STATE_RENDER(WINED3D_RS_WRAP0),                     NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_WRAP15),                    { STATE_RENDER(WINED3D_RS_WRAP0),                     NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_EXTENTS),                   { STATE_RENDER(WINED3D_RS_EXTENTS),                   state_extents       }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_COLORKEYBLENDENABLE),       { STATE_RENDER(WINED3D_RS_COLORKEYBLENDENABLE),       state_ckeyblend     }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_SOFTWAREVERTEXPROCESSING),  { STATE_RENDER(WINED3D_RS_SOFTWAREVERTEXPROCESSING),  state_swvp          }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_PATCHEDGESTYLE),            { STATE_RENDER(WINED3D_RS_PATCHEDGESTYLE),            state_patchedgestyle}, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_PATCHSEGMENTS),             { STATE_RENDER(WINED3D_RS_PATCHSEGMENTS),             state_patchsegments }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_POSITIONDEGREE),            { STATE_RENDER(WINED3D_RS_POSITIONDEGREE),            state_positiondegree}, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_NORMALDEGREE),              { STATE_RENDER(WINED3D_RS_NORMALDEGREE),              state_normaldegree  }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_MINTESSELLATIONLEVEL),      { STATE_RENDER(WINED3D_RS_ENABLEADAPTIVETESSELLATION),NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_MAXTESSELLATIONLEVEL),      { STATE_RENDER(WINED3D_RS_ENABLEADAPTIVETESSELLATION),NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_ADAPTIVETESS_X),            { STATE_RENDER(WINED3D_RS_ENABLEADAPTIVETESSELLATION),NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_ADAPTIVETESS_Y),            { STATE_RENDER(WINED3D_RS_ENABLEADAPTIVETESSELLATION),NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_ADAPTIVETESS_Z),            { STATE_RENDER(WINED3D_RS_ENABLEADAPTIVETESSELLATION),NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_ADAPTIVETESS_W),            { STATE_RENDER(WINED3D_RS_ENABLEADAPTIVETESSELLATION),NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_ENABLEADAPTIVETESSELLATION),{ STATE_RENDER(WINED3D_RS_ENABLEADAPTIVETESSELLATION),state_nvdb          }, EXT_DEPTH_BOUNDS_TEST           },
    { STATE_RENDER(WINED3D_RS_ENABLEADAPTIVETESSELLATION),{ STATE_RENDER(WINED3D_RS_ENABLEADAPTIVETESSELLATION),state_tessellation  }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_MULTISAMPLEANTIALIAS),      { STATE_RENDER(WINED3D_RS_MULTISAMPLEANTIALIAS),      state_msaa          }, ARB_MULTISAMPLE                 },
    { STATE_RENDER(WINED3D_RS_MULTISAMPLEANTIALIAS),      { STATE_RENDER(WINED3D_RS_MULTISAMPLEANTIALIAS),      state_msaa_w        }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_MULTISAMPLEMASK),           { STATE_RENDER(WINED3D_RS_MULTISAMPLEMASK),           state_multisampmask }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_DEBUGMONITORTOKEN),         { STATE_RENDER(WINED3D_RS_DEBUGMONITORTOKEN),         state_debug_monitor }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE),          { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE),          state_colorwrite_i  }, EXT_DRAW_BUFFERS2               },
    { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE),          { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE),          state_colorwrite    }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE1),         { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE1),         state_colorwrite_i  }, EXT_DRAW_BUFFERS2               },
    { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE1),         { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE),          NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE2),         { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE2),         state_colorwrite_i  }, EXT_DRAW_BUFFERS2               },
    { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE2),         { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE),          NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE3),         { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE3),         state_colorwrite_i  }, EXT_DRAW_BUFFERS2               },
    { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE3),         { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE),          NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE4),         { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE4),         state_colorwrite_i  }, EXT_DRAW_BUFFERS2               },
    { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE4),         { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE),          NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE5),         { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE5),         state_colorwrite_i  }, EXT_DRAW_BUFFERS2               },
    { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE5),         { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE),          NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE6),         { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE6),         state_colorwrite_i  }, EXT_DRAW_BUFFERS2               },
    { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE6),         { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE),          NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE7),         { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE7),         state_colorwrite_i  }, EXT_DRAW_BUFFERS2               },
    { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE7),         { STATE_RENDER(WINED3D_RS_COLORWRITEENABLE),          NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_BLENDOP),                   { STATE_RENDER(WINED3D_RS_BLENDOP),                   state_blendop       }, WINED3D_GL_BLEND_EQUATION       },
    { STATE_RENDER(WINED3D_RS_BLENDOP),                   { STATE_RENDER(WINED3D_RS_BLENDOP),                   state_blendop_w     }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_SCISSORTESTENABLE),         { STATE_RENDER(WINED3D_RS_SCISSORTESTENABLE),         state_scissor       }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_SLOPESCALEDEPTHBIAS),       { STATE_RENDER(WINED3D_RS_DEPTHBIAS),                 NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_DEPTHBIAS),                 { STATE_RENDER(WINED3D_RS_DEPTHBIAS),                 state_depthbias     }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_ZVISIBLE),                  { STATE_RENDER(WINED3D_RS_ZVISIBLE),                  state_zvisible      }, WINED3D_GL_EXT_NONE             },
    /* Samplers */
    { STATE_SAMPLER(0),                                   { STATE_SAMPLER(0),                                   sampler             }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(1),                                   { STATE_SAMPLER(1),                                   sampler             }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(2),                                   { STATE_SAMPLER(2),                                   sampler             }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(3),                                   { STATE_SAMPLER(3),                                   sampler             }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(4),                                   { STATE_SAMPLER(4),                                   sampler             }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(5),                                   { STATE_SAMPLER(5),                                   sampler             }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(6),                                   { STATE_SAMPLER(6),                                   sampler             }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(7),                                   { STATE_SAMPLER(7),                                   sampler             }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(8),                                   { STATE_SAMPLER(8),                                   sampler             }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(9),                                   { STATE_SAMPLER(9),                                   sampler             }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(10),                                  { STATE_SAMPLER(10),                                  sampler             }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(11),                                  { STATE_SAMPLER(11),                                  sampler             }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(12),                                  { STATE_SAMPLER(12),                                  sampler             }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(13),                                  { STATE_SAMPLER(13),                                  sampler             }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(14),                                  { STATE_SAMPLER(14),                                  sampler             }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(15),                                  { STATE_SAMPLER(15),                                  sampler             }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(16), /* Vertex sampler 0 */           { STATE_SAMPLER(16),                                  sampler             }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(17), /* Vertex sampler 1 */           { STATE_SAMPLER(17),                                  sampler             }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(18), /* Vertex sampler 2 */           { STATE_SAMPLER(18),                                  sampler             }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(19), /* Vertex sampler 3 */           { STATE_SAMPLER(19),                                  sampler             }, WINED3D_GL_EXT_NONE             },
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

static const struct wined3d_state_entry_template vp_ffp_states[] =
{
    { STATE_VDECL,                                        { STATE_VDECL,                                        vertexdeclaration   }, WINED3D_GL_EXT_NONE             },
    { STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),           { STATE_VDECL,                                        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_MATERIAL,                                     { STATE_RENDER(WINED3D_RS_SPECULARENABLE),            NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_SPECULARENABLE),            { STATE_RENDER(WINED3D_RS_SPECULARENABLE),            state_specularenable}, WINED3D_GL_EXT_NONE             },
      /* Clip planes */
    { STATE_CLIPPLANE(0),                                 { STATE_CLIPPLANE(0),                                 clipplane           }, WINED3D_GL_EXT_NONE             },
    { STATE_CLIPPLANE(1),                                 { STATE_CLIPPLANE(1),                                 clipplane           }, WINED3D_GL_EXT_NONE             },
    { STATE_CLIPPLANE(2),                                 { STATE_CLIPPLANE(2),                                 clipplane           }, WINED3D_GL_EXT_NONE             },
    { STATE_CLIPPLANE(3),                                 { STATE_CLIPPLANE(3),                                 clipplane           }, WINED3D_GL_EXT_NONE             },
    { STATE_CLIPPLANE(4),                                 { STATE_CLIPPLANE(4),                                 clipplane           }, WINED3D_GL_EXT_NONE             },
    { STATE_CLIPPLANE(5),                                 { STATE_CLIPPLANE(5),                                 clipplane           }, WINED3D_GL_EXT_NONE             },
    { STATE_CLIPPLANE(6),                                 { STATE_CLIPPLANE(6),                                 clipplane           }, WINED3D_GL_EXT_NONE             },
    { STATE_CLIPPLANE(7),                                 { STATE_CLIPPLANE(7),                                 clipplane           }, WINED3D_GL_EXT_NONE             },
      /* Lights */
    { STATE_LIGHT_TYPE,                                   { STATE_LIGHT_TYPE,                                   state_nop           }, WINED3D_GL_EXT_NONE             },
    { STATE_ACTIVELIGHT(0),                               { STATE_ACTIVELIGHT(0),                               light               }, WINED3D_GL_EXT_NONE             },
    { STATE_ACTIVELIGHT(1),                               { STATE_ACTIVELIGHT(1),                               light               }, WINED3D_GL_EXT_NONE             },
    { STATE_ACTIVELIGHT(2),                               { STATE_ACTIVELIGHT(2),                               light               }, WINED3D_GL_EXT_NONE             },
    { STATE_ACTIVELIGHT(3),                               { STATE_ACTIVELIGHT(3),                               light               }, WINED3D_GL_EXT_NONE             },
    { STATE_ACTIVELIGHT(4),                               { STATE_ACTIVELIGHT(4),                               light               }, WINED3D_GL_EXT_NONE             },
    { STATE_ACTIVELIGHT(5),                               { STATE_ACTIVELIGHT(5),                               light               }, WINED3D_GL_EXT_NONE             },
    { STATE_ACTIVELIGHT(6),                               { STATE_ACTIVELIGHT(6),                               light               }, WINED3D_GL_EXT_NONE             },
    { STATE_ACTIVELIGHT(7),                               { STATE_ACTIVELIGHT(7),                               light               }, WINED3D_GL_EXT_NONE             },
    /* Viewport */
    { STATE_VIEWPORT,                                     { STATE_VIEWPORT,                                     viewport_vertexpart }, WINED3D_GL_EXT_NONE             },
      /* Transform states follow                    */
    { STATE_TRANSFORM(WINED3D_TS_VIEW),                   { STATE_TRANSFORM(WINED3D_TS_VIEW),                   transform_view      }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_PROJECTION),             { STATE_TRANSFORM(WINED3D_TS_PROJECTION),             transform_projection}, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_TEXTURE0),               { STATE_TEXTURESTAGE(0, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), NULL            }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_TEXTURE1),               { STATE_TEXTURESTAGE(1, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), NULL            }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_TEXTURE2),               { STATE_TEXTURESTAGE(2, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), NULL            }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_TEXTURE3),               { STATE_TEXTURESTAGE(3, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), NULL            }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_TEXTURE4),               { STATE_TEXTURESTAGE(4, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), NULL            }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_TEXTURE5),               { STATE_TEXTURESTAGE(5, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), NULL            }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_TEXTURE6),               { STATE_TEXTURESTAGE(6, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), NULL            }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_TEXTURE7),               { STATE_TEXTURESTAGE(7, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), NULL            }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(  0)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(  0)),      transform_world     }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(  1)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(  1)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(  2)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(  2)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(  3)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(  3)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(  4)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(  4)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(  5)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(  5)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(  6)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(  6)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(  7)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(  7)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(  8)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(  8)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(  9)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(  9)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 10)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 10)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 11)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 11)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 12)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 12)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 13)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 13)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 14)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 14)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 15)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 15)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 16)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 16)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 17)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 17)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 18)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 18)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 19)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 19)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 20)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 20)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 21)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 21)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 22)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 22)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 23)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 23)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 24)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 24)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 25)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 25)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 26)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 26)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 27)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 27)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 28)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 28)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 29)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 29)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 30)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 30)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 31)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 31)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 32)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 32)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 33)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 33)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 34)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 34)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 35)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 35)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 36)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 36)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 37)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 37)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 38)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 38)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 39)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 39)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 40)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 40)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 41)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 41)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 42)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 42)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 43)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 43)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 44)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 44)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 45)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 45)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 46)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 46)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 47)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 47)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 48)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 48)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 49)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 49)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 50)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 50)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 51)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 51)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 52)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 52)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 53)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 53)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 54)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 54)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 55)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 55)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 56)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 56)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 57)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 57)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 58)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 58)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 59)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 59)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 60)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 60)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 61)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 61)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 62)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 62)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 63)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 63)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 64)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 64)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 65)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 65)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 66)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 66)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 67)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 67)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 68)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 68)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 69)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 69)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 70)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 70)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 71)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 71)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 72)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 72)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 73)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 73)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 74)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 74)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 75)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 75)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 76)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 76)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 77)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 77)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 78)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 78)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 79)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 79)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 80)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 80)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 81)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 81)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 82)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 82)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 83)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 83)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 84)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 84)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 85)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 85)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 86)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 86)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 87)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 87)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 88)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 88)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 89)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 89)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 90)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 90)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 91)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 91)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 92)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 92)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 93)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 93)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 94)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 94)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 95)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 95)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 96)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 96)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 97)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 97)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 98)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 98)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 99)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX( 99)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(100)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(100)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(101)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(101)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(102)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(102)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(103)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(103)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(104)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(104)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(105)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(105)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(106)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(106)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(107)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(107)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(108)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(108)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(109)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(109)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(110)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(110)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(111)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(111)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(112)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(112)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(113)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(113)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(114)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(114)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(115)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(115)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(116)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(116)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(117)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(117)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(118)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(118)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(119)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(119)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(120)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(120)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(121)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(121)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(122)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(122)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(123)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(123)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(124)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(124)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(125)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(125)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(126)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(126)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(127)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(127)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(128)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(128)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(129)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(129)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(130)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(130)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(131)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(131)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(132)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(132)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(133)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(133)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(134)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(134)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(135)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(135)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(136)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(136)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(137)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(137)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(138)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(138)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(139)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(139)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(140)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(140)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(141)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(141)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(142)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(142)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(143)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(143)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(144)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(144)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(145)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(145)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(146)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(146)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(147)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(147)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(148)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(148)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(149)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(149)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(150)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(150)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(151)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(151)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(152)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(152)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(153)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(153)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(154)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(154)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(155)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(155)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(156)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(156)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(157)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(157)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(158)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(158)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(159)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(159)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(160)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(160)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(161)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(161)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(162)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(162)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(163)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(163)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(164)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(164)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(165)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(165)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(166)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(166)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(167)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(167)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(168)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(168)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(169)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(169)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(170)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(170)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(171)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(171)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(172)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(172)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(173)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(173)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(174)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(174)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(175)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(175)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(176)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(176)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(177)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(177)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(178)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(178)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(179)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(179)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(180)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(180)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(181)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(181)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(182)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(182)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(183)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(183)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(184)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(184)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(185)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(185)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(186)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(186)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(187)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(187)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(188)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(188)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(189)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(189)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(190)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(190)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(191)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(191)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(192)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(192)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(193)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(193)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(194)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(194)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(195)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(195)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(196)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(196)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(197)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(197)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(198)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(198)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(199)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(199)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(200)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(200)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(201)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(201)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(202)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(202)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(203)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(203)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(204)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(204)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(205)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(205)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(206)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(206)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(207)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(207)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(208)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(208)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(209)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(209)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(210)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(210)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(211)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(211)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(212)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(212)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(213)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(213)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(214)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(214)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(215)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(215)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(216)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(216)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(217)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(217)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(218)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(218)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(219)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(219)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(220)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(220)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(221)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(221)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(222)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(222)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(223)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(223)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(224)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(224)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(225)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(225)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(226)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(226)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(227)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(227)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(228)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(228)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(229)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(229)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(230)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(230)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(231)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(231)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(232)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(232)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(233)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(233)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(234)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(234)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(235)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(235)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(236)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(236)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(237)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(237)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(238)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(238)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(239)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(239)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(240)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(240)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(241)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(241)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(242)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(242)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(243)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(243)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(244)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(244)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(245)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(245)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(246)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(246)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(247)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(247)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(248)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(248)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(249)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(249)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(250)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(250)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(251)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(251)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(252)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(252)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(253)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(253)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(254)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(254)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(255)),      { STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(255)),      transform_worldex   }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(0, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), transform_texture   }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(1, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), transform_texture   }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(2, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), transform_texture   }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(3, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), transform_texture   }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(4, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), transform_texture   }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(5, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), transform_texture   }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(6, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), transform_texture   }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), {STATE_TEXTURESTAGE(7, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS), transform_texture   }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_TEXCOORD_INDEX),  { STATE_TEXTURESTAGE(0, WINED3D_TSS_TEXCOORD_INDEX),  tex_coordindex      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_TEXCOORD_INDEX),  { STATE_TEXTURESTAGE(1, WINED3D_TSS_TEXCOORD_INDEX),  tex_coordindex      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_TEXCOORD_INDEX),  { STATE_TEXTURESTAGE(2, WINED3D_TSS_TEXCOORD_INDEX),  tex_coordindex      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_TEXCOORD_INDEX),  { STATE_TEXTURESTAGE(3, WINED3D_TSS_TEXCOORD_INDEX),  tex_coordindex      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_TEXCOORD_INDEX),  { STATE_TEXTURESTAGE(4, WINED3D_TSS_TEXCOORD_INDEX),  tex_coordindex      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_TEXCOORD_INDEX),  { STATE_TEXTURESTAGE(5, WINED3D_TSS_TEXCOORD_INDEX),  tex_coordindex      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_TEXCOORD_INDEX),  { STATE_TEXTURESTAGE(6, WINED3D_TSS_TEXCOORD_INDEX),  tex_coordindex      }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_TEXCOORD_INDEX),  { STATE_TEXTURESTAGE(7, WINED3D_TSS_TEXCOORD_INDEX),  tex_coordindex      }, WINED3D_GL_EXT_NONE             },
      /* Fog */
    { STATE_RENDER(WINED3D_RS_FOGENABLE),                 { STATE_RENDER(WINED3D_RS_FOGENABLE),                 state_fog_vertexpart}, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_FOGTABLEMODE),              { STATE_RENDER(WINED3D_RS_FOGENABLE),                 NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_FOGVERTEXMODE),             { STATE_RENDER(WINED3D_RS_FOGENABLE),                 NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_RANGEFOGENABLE),            { STATE_RENDER(WINED3D_RS_FOGENABLE),                 NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_CLIPPING),                  { STATE_RENDER(WINED3D_RS_CLIPPING),                  state_clipping      }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_CLIPPLANEENABLE),           { STATE_RENDER(WINED3D_RS_CLIPPING),                  NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_LIGHTING),                  { STATE_RENDER(WINED3D_RS_LIGHTING),                  state_lighting      }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_AMBIENT),                   { STATE_RENDER(WINED3D_RS_AMBIENT),                   state_ambient       }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_COLORVERTEX),               { STATE_RENDER(WINED3D_RS_COLORVERTEX),               state_colormat      }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_LOCALVIEWER),               { STATE_RENDER(WINED3D_RS_LOCALVIEWER),               state_localviewer   }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_NORMALIZENORMALS),          { STATE_RENDER(WINED3D_RS_NORMALIZENORMALS),          state_normalize     }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_DIFFUSEMATERIALSOURCE),     { STATE_RENDER(WINED3D_RS_COLORVERTEX),               NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_SPECULARMATERIALSOURCE),    { STATE_RENDER(WINED3D_RS_COLORVERTEX),               NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_AMBIENTMATERIALSOURCE),     { STATE_RENDER(WINED3D_RS_COLORVERTEX),               NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_EMISSIVEMATERIALSOURCE),    { STATE_RENDER(WINED3D_RS_COLORVERTEX),               NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_VERTEXBLEND),               { STATE_RENDER(WINED3D_RS_VERTEXBLEND),               state_vertexblend_w }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_POINTSIZE),                 { STATE_RENDER(WINED3D_RS_POINTSCALEENABLE),          NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_POINTSIZE_MIN),             { STATE_RENDER(WINED3D_RS_POINTSIZE_MIN),             state_psizemin_arb  }, ARB_POINT_PARAMETERS            },
    { STATE_RENDER(WINED3D_RS_POINTSIZE_MIN),             { STATE_RENDER(WINED3D_RS_POINTSIZE_MIN),             state_psizemin_ext  }, EXT_POINT_PARAMETERS            },
    { STATE_RENDER(WINED3D_RS_POINTSIZE_MIN),             { STATE_RENDER(WINED3D_RS_POINTSIZE_MIN),             state_psizemin_w    }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_POINTSPRITEENABLE),         { STATE_RENDER(WINED3D_RS_POINTSPRITEENABLE),         state_pointsprite   }, ARB_POINT_SPRITE                },
    { STATE_RENDER(WINED3D_RS_POINTSPRITEENABLE),         { STATE_RENDER(WINED3D_RS_POINTSPRITEENABLE),         state_pointsprite_w }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_POINTSCALEENABLE),          { STATE_RENDER(WINED3D_RS_POINTSCALEENABLE),          state_pscale        }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_POINTSCALE_A),              { STATE_RENDER(WINED3D_RS_POINTSCALEENABLE),          NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_POINTSCALE_B),              { STATE_RENDER(WINED3D_RS_POINTSCALEENABLE),          NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_POINTSCALE_C),              { STATE_RENDER(WINED3D_RS_POINTSCALEENABLE),          NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_POINTSIZE_MAX),             { STATE_RENDER(WINED3D_RS_POINTSIZE_MIN),             NULL                }, ARB_POINT_PARAMETERS            },
    { STATE_RENDER(WINED3D_RS_POINTSIZE_MAX),             { STATE_RENDER(WINED3D_RS_POINTSIZE_MIN),             NULL                }, EXT_POINT_PARAMETERS            },
    { STATE_RENDER(WINED3D_RS_POINTSIZE_MAX),             { STATE_RENDER(WINED3D_RS_POINTSIZE_MIN),             NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_TWEENFACTOR),               { STATE_RENDER(WINED3D_RS_VERTEXBLEND),               NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_INDEXEDVERTEXBLENDENABLE),  { STATE_RENDER(WINED3D_RS_VERTEXBLEND),               NULL                }, WINED3D_GL_EXT_NONE             },

    /* Samplers for NP2 texture matrix adjustions. They are not needed if GL_ARB_texture_non_power_of_two is supported,
     * so register a NULL state handler in that case to get the vertex part of sampler() skipped(VTF is handled in the misc states.
     * otherwise, register sampler_texmatrix, which takes care of updating the texture matrix
     */
    { STATE_SAMPLER(0),                                   { 0,                                                  NULL                }, ARB_TEXTURE_NON_POWER_OF_TWO    },
    { STATE_SAMPLER(0),                                   { 0,                                                  NULL                }, WINED3D_GL_NORMALIZED_TEXRECT   },
    { STATE_SAMPLER(0),                                   { STATE_SAMPLER(0),                                   sampler_texmatrix   }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(1),                                   { 0,                                                  NULL                }, ARB_TEXTURE_NON_POWER_OF_TWO    },
    { STATE_SAMPLER(1),                                   { 0,                                                  NULL                }, WINED3D_GL_NORMALIZED_TEXRECT   },
    { STATE_SAMPLER(1),                                   { STATE_SAMPLER(1),                                   sampler_texmatrix   }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(2),                                   { 0,                                                  NULL                }, ARB_TEXTURE_NON_POWER_OF_TWO    },
    { STATE_SAMPLER(2),                                   { 0,                                                  NULL                }, WINED3D_GL_NORMALIZED_TEXRECT   },
    { STATE_SAMPLER(2),                                   { STATE_SAMPLER(2),                                   sampler_texmatrix   }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(3),                                   { 0,                                                  NULL                }, ARB_TEXTURE_NON_POWER_OF_TWO    },
    { STATE_SAMPLER(3),                                   { 0,                                                  NULL                }, WINED3D_GL_NORMALIZED_TEXRECT   },
    { STATE_SAMPLER(3),                                   { STATE_SAMPLER(3),                                   sampler_texmatrix   }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(4),                                   { 0,                                                  NULL                }, ARB_TEXTURE_NON_POWER_OF_TWO    },
    { STATE_SAMPLER(4),                                   { 0,                                                  NULL                }, WINED3D_GL_NORMALIZED_TEXRECT   },
    { STATE_SAMPLER(4),                                   { STATE_SAMPLER(4),                                   sampler_texmatrix   }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(5),                                   { 0,                                                  NULL                }, ARB_TEXTURE_NON_POWER_OF_TWO    },
    { STATE_SAMPLER(5),                                   { 0,                                                  NULL                }, WINED3D_GL_NORMALIZED_TEXRECT   },
    { STATE_SAMPLER(5),                                   { STATE_SAMPLER(5),                                   sampler_texmatrix   }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(6),                                   { 0,                                                  NULL                }, ARB_TEXTURE_NON_POWER_OF_TWO    },
    { STATE_SAMPLER(6),                                   { 0,                                                  NULL                }, WINED3D_GL_NORMALIZED_TEXRECT   },
    { STATE_SAMPLER(6),                                   { STATE_SAMPLER(6),                                   sampler_texmatrix   }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(7),                                   { 0,                                                  NULL                }, ARB_TEXTURE_NON_POWER_OF_TWO    },
    { STATE_SAMPLER(7),                                   { 0,                                                  NULL                }, WINED3D_GL_NORMALIZED_TEXRECT   },
    { STATE_SAMPLER(7),                                   { STATE_SAMPLER(7),                                   sampler_texmatrix   }, WINED3D_GL_EXT_NONE             },
    { STATE_POINT_ENABLE,                                 { STATE_POINT_ENABLE,                                 state_nop           }, WINED3D_GL_EXT_NONE             },
    {0 /* Terminate */,                                   { 0,                                                  0                   }, WINED3D_GL_EXT_NONE             },
};

static const struct wined3d_state_entry_template ffp_fragmentstate_template[] = {
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_OP),        { STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_OP),        tex_colorop         }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_ARG1),      { STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_ARG2),      { STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_OP),        { STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_OP),        tex_alphaop         }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_ARG1),      { STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_ARG2),      { STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_ARG0),      { STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_ARG0),      { STATE_TEXTURESTAGE(0, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_RESULT_ARG),      { STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(0, WINED3D_TSS_CONSTANT),        { 0 /* As long as we don't support D3DTA_CONSTANT */, NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_OP),        { STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_OP),        tex_colorop         }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_ARG1),      { STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_ARG2),      { STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_OP),        { STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_OP),        tex_alphaop         }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_ARG1),      { STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_ARG2),      { STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_ARG0),      { STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_ARG0),      { STATE_TEXTURESTAGE(1, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_RESULT_ARG),      { STATE_TEXTURESTAGE(1, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(1, WINED3D_TSS_CONSTANT),        { 0 /* As long as we don't support D3DTA_CONSTANT */, NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_OP),        { STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_OP),        tex_colorop         }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_ARG1),      { STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_ARG2),      { STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_OP),        { STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_OP),        tex_alphaop         }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_ARG1),      { STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_ARG2),      { STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_ARG0),      { STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_ARG0),      { STATE_TEXTURESTAGE(2, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_RESULT_ARG),      { STATE_TEXTURESTAGE(2, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(2, WINED3D_TSS_CONSTANT),        { 0 /* As long as we don't support D3DTA_CONSTANT */, NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_OP),        { STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_OP),        tex_colorop         }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_ARG1),      { STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_ARG2),      { STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_OP),        { STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_OP),        tex_alphaop         }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_ARG1),      { STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_ARG2),      { STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_ARG0),      { STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_ARG0),      { STATE_TEXTURESTAGE(3, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_RESULT_ARG),      { STATE_TEXTURESTAGE(3, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(3, WINED3D_TSS_CONSTANT),        { 0 /* As long as we don't support D3DTA_CONSTANT */, NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_OP),        { STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_OP),        tex_colorop         }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_ARG1),      { STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_ARG2),      { STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_OP),        { STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_OP),        tex_alphaop         }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_ARG1),      { STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_ARG2),      { STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_ARG0),      { STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_ARG0),      { STATE_TEXTURESTAGE(4, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_RESULT_ARG),      { STATE_TEXTURESTAGE(4, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(4, WINED3D_TSS_CONSTANT),        { 0 /* As long as we don't support D3DTA_CONSTANT */, NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_OP),        { STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_OP),        tex_colorop         }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_ARG1),      { STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_ARG2),      { STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_OP),        { STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_OP),        tex_alphaop         }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_ARG1),      { STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_ARG2),      { STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_ARG0),      { STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_ARG0),      { STATE_TEXTURESTAGE(5, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_RESULT_ARG),      { STATE_TEXTURESTAGE(5, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(5, WINED3D_TSS_CONSTANT),        { 0 /* As long as we don't support D3DTA_CONSTANT */, NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_OP),        { STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_OP),        tex_colorop         }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_ARG1),      { STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_ARG2),      { STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_OP),        { STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_OP),        tex_alphaop         }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_ARG1),      { STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_ARG2),      { STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_ARG0),      { STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_ARG0),      { STATE_TEXTURESTAGE(6, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_RESULT_ARG),      { STATE_TEXTURESTAGE(6, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(6, WINED3D_TSS_CONSTANT),        { 0 /* As long as we don't support D3DTA_CONSTANT */, NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_OP),        { STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_OP),        tex_colorop         }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_ARG1),      { STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_ARG2),      { STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_OP),        { STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_OP),        tex_alphaop         }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_ARG1),      { STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_ARG2),      { STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_ARG0),      { STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_ARG0),      { STATE_TEXTURESTAGE(7, WINED3D_TSS_ALPHA_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_RESULT_ARG),      { STATE_TEXTURESTAGE(7, WINED3D_TSS_COLOR_OP),        NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_TEXTURESTAGE(7, WINED3D_TSS_CONSTANT),        { 0 /* As long as we don't support D3DTA_CONSTANT */, NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            apply_pixelshader   }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_ALPHAFUNC),                 { STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),           NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_ALPHAREF),                  { STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),           NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),           { STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),           state_alpha_test    }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_COLORKEYENABLE),            { STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),           NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_COLOR_KEY,                                    { STATE_COLOR_KEY,                                    state_nop           }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_SRGBWRITEENABLE),           { STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL),            NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_TEXTUREFACTOR),             { STATE_RENDER(WINED3D_RS_TEXTUREFACTOR),             state_texfactor     }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_FOGCOLOR),                  { STATE_RENDER(WINED3D_RS_FOGCOLOR),                  state_fogcolor      }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_FOGDENSITY),                { STATE_RENDER(WINED3D_RS_FOGDENSITY),                state_fogdensity    }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_FOGENABLE),                 { STATE_RENDER(WINED3D_RS_FOGENABLE),                 state_fog_fragpart  }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_FOGTABLEMODE),              { STATE_RENDER(WINED3D_RS_FOGENABLE),                 NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_FOGVERTEXMODE),             { STATE_RENDER(WINED3D_RS_FOGENABLE),                 NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_FOGSTART),                  { STATE_RENDER(WINED3D_RS_FOGSTART),                  state_fogstartend   }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_FOGEND),                    { STATE_RENDER(WINED3D_RS_FOGSTART),                  NULL                }, WINED3D_GL_EXT_NONE             },
    { STATE_RENDER(WINED3D_RS_SHADEMODE),                 { STATE_RENDER(WINED3D_RS_SHADEMODE),                 state_shademode     }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(0),                                   { STATE_SAMPLER(0),                                   sampler_texdim      }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(1),                                   { STATE_SAMPLER(1),                                   sampler_texdim      }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(2),                                   { STATE_SAMPLER(2),                                   sampler_texdim      }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(3),                                   { STATE_SAMPLER(3),                                   sampler_texdim      }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(4),                                   { STATE_SAMPLER(4),                                   sampler_texdim      }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(5),                                   { STATE_SAMPLER(5),                                   sampler_texdim      }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(6),                                   { STATE_SAMPLER(6),                                   sampler_texdim      }, WINED3D_GL_EXT_NONE             },
    { STATE_SAMPLER(7),                                   { STATE_SAMPLER(7),                                   sampler_texdim      }, WINED3D_GL_EXT_NONE             },
    {0 /* Terminate */,                                   { 0,                                                  0                   }, WINED3D_GL_EXT_NONE             },
};

/* Context activation is done by the caller. */
static void ffp_pipe_enable(const struct wined3d_context *context, BOOL enable) {}

static void *ffp_alloc(const struct wined3d_shader_backend_ops *shader_backend, void *shader_priv)
{
    return shader_priv;
}

static void ffp_free(struct wined3d_device *device, struct wined3d_context *context) {}

static void vp_ffp_get_caps(const struct wined3d_adapter *adapter, struct wined3d_vertex_caps *caps)
{
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;

    caps->xyzrhw = FALSE;
    caps->ffp_generic_attributes = FALSE;
    caps->max_active_lights = gl_info->limits.lights;
    caps->max_vertex_blend_matrices = 1;
    caps->max_vertex_blend_matrix_index = 0;
    caps->vertex_processing_caps = WINED3DVTXPCAPS_DIRECTIONALLIGHTS
            | WINED3DVTXPCAPS_MATERIALSOURCE7
            | WINED3DVTXPCAPS_POSITIONALLIGHTS
            | WINED3DVTXPCAPS_LOCALVIEWER
            | WINED3DVTXPCAPS_VERTEXFOG
            | WINED3DVTXPCAPS_TEXGEN
            | WINED3DVTXPCAPS_TEXGEN_SPHEREMAP;
    caps->fvf_caps = WINED3DFVFCAPS_PSIZE | 0x0008; /* 8 texture coords */
    caps->max_user_clip_planes = gl_info->limits.user_clip_distances;
    caps->raster_caps = 0;
    if (gl_info->supported[NV_FOG_DISTANCE])
        caps->raster_caps |= WINED3DPRASTERCAPS_FOGRANGE;
}

static DWORD vp_ffp_get_emul_mask(const struct wined3d_gl_info *gl_info)
{
    return GL_EXT_EMUL_ARB_MULTITEXTURE | GL_EXT_EMUL_EXT_FOG_COORD;
}

const struct wined3d_vertex_pipe_ops ffp_vertex_pipe =
{
    ffp_pipe_enable,
    vp_ffp_get_caps,
    vp_ffp_get_emul_mask,
    ffp_alloc,
    ffp_free,
    vp_ffp_states,
};

static void ffp_fragment_get_caps(const struct wined3d_adapter *adapter, struct fragment_caps *caps)
{
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;

    caps->wined3d_caps = 0;
    caps->PrimitiveMiscCaps = 0;
    caps->TextureOpCaps = WINED3DTEXOPCAPS_ADD
            | WINED3DTEXOPCAPS_ADDSIGNED
            | WINED3DTEXOPCAPS_ADDSIGNED2X
            | WINED3DTEXOPCAPS_MODULATE
            | WINED3DTEXOPCAPS_MODULATE2X
            | WINED3DTEXOPCAPS_MODULATE4X
            | WINED3DTEXOPCAPS_SELECTARG1
            | WINED3DTEXOPCAPS_SELECTARG2
            | WINED3DTEXOPCAPS_DISABLE;

    if (gl_info->supported[ARB_TEXTURE_ENV_COMBINE]
            || gl_info->supported[EXT_TEXTURE_ENV_COMBINE]
            || gl_info->supported[NV_TEXTURE_ENV_COMBINE4])
    {
        caps->TextureOpCaps |= WINED3DTEXOPCAPS_BLENDDIFFUSEALPHA
                | WINED3DTEXOPCAPS_BLENDTEXTUREALPHA
                | WINED3DTEXOPCAPS_BLENDFACTORALPHA
                | WINED3DTEXOPCAPS_BLENDCURRENTALPHA
                | WINED3DTEXOPCAPS_LERP
                | WINED3DTEXOPCAPS_SUBTRACT;
    }
    if (gl_info->supported[ATI_TEXTURE_ENV_COMBINE3]
            || gl_info->supported[NV_TEXTURE_ENV_COMBINE4])
    {
        caps->TextureOpCaps |= WINED3DTEXOPCAPS_ADDSMOOTH
                | WINED3DTEXOPCAPS_MULTIPLYADD
                | WINED3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR
                | WINED3DTEXOPCAPS_MODULATECOLOR_ADDALPHA
                | WINED3DTEXOPCAPS_BLENDTEXTUREALPHAPM;
    }
    if (gl_info->supported[ARB_TEXTURE_ENV_DOT3])
        caps->TextureOpCaps |= WINED3DTEXOPCAPS_DOTPRODUCT3;

    caps->MaxTextureBlendStages = gl_info->limits.textures;
    caps->MaxSimultaneousTextures = gl_info->limits.textures;
}

static DWORD ffp_fragment_get_emul_mask(const struct wined3d_gl_info *gl_info)
{
    return GL_EXT_EMUL_ARB_MULTITEXTURE | GL_EXT_EMUL_EXT_FOG_COORD;
}

static BOOL ffp_color_fixup_supported(struct color_fixup_desc fixup)
{
    /* We only support identity conversions. */
    return is_identity_fixup(fixup);
}

static BOOL ffp_none_context_alloc(struct wined3d_context *context)
{
    return TRUE;
}

static void ffp_none_context_free(struct wined3d_context *context)
{
}

const struct wined3d_fragment_pipe_ops ffp_fragment_pipeline =
{
    ffp_pipe_enable,
    ffp_fragment_get_caps,
    ffp_fragment_get_emul_mask,
    ffp_alloc,
    ffp_free,
    ffp_none_context_alloc,
    ffp_none_context_free,
    ffp_color_fixup_supported,
    ffp_fragmentstate_template,
};

static void none_pipe_enable(const struct wined3d_context *context, BOOL enable) {}

static void *none_alloc(const struct wined3d_shader_backend_ops *shader_backend, void *shader_priv)
{
    return shader_priv;
}

static void none_free(struct wined3d_device *device, struct wined3d_context *context) {}

static void vp_none_get_caps(const struct wined3d_adapter *adapter, struct wined3d_vertex_caps *caps)
{
    memset(caps, 0, sizeof(*caps));
}

static DWORD vp_none_get_emul_mask(const struct wined3d_gl_info *gl_info)
{
    return 0;
}

const struct wined3d_vertex_pipe_ops none_vertex_pipe =
{
    none_pipe_enable,
    vp_none_get_caps,
    vp_none_get_emul_mask,
    none_alloc,
    none_free,
    NULL,
};

static void fp_none_get_caps(const struct wined3d_adapter *adapter, struct fragment_caps *caps)
{
    memset(caps, 0, sizeof(*caps));
}

static DWORD fp_none_get_emul_mask(const struct wined3d_gl_info *gl_info)
{
    return 0;
}

static BOOL fp_none_color_fixup_supported(struct color_fixup_desc fixup)
{
    return is_identity_fixup(fixup);
}

const struct wined3d_fragment_pipe_ops none_fragment_pipe =
{
    none_pipe_enable,
    fp_none_get_caps,
    fp_none_get_emul_mask,
    none_alloc,
    none_free,
    ffp_none_context_alloc,
    ffp_none_context_free,
    fp_none_color_fixup_supported,
    NULL,
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

    start = STATE_TEXTURESTAGE(d3d_info->limits.ffp_blend_stages, 0);
    last = STATE_TEXTURESTAGE(WINED3D_MAX_TEXTURES - 1, WINED3D_HIGHEST_TEXTURE_STATE);
    for (i = start; i <= last; ++i)
    {
        state_table[i].representative = 0;
        state_table[i].apply = state_undefined;
    }

    start = STATE_TRANSFORM(WINED3D_TS_TEXTURE0 + d3d_info->limits.ffp_blend_stages);
    last = STATE_TRANSFORM(WINED3D_TS_TEXTURE0 + WINED3D_MAX_TEXTURES - 1);
    for (i = start; i <= last; ++i)
    {
        state_table[i].representative = 0;
        state_table[i].apply = state_undefined;
    }

    start = STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(max(d3d_info->limits.ffp_vertex_blend_matrices,
            d3d_info->limits.ffp_max_vertex_blend_matrix_index + 1)));
    last = STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(255));
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
        {  1,   1},
        {  3,   3},
        { 17,  18},
        { 21,  21},
        { 42,  45},
        { 47,  47},
        { 61, 127},
        {149, 150},
        {169, 169},
        {177, 177},
        {193, 193},
        {196, 197},
        {  0,   0},
    };
    static const DWORD simple_states[] =
    {
        STATE_MATERIAL,
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
        STATE_LIGHT_TYPE,
        STATE_SCISSORRECT,
        STATE_RASTERIZER,
        STATE_POINTSPRITECOORDORIGIN,
        STATE_BASEVERTEXINDEX,
        STATE_FRAMEBUFFER,
        STATE_POINT_ENABLE,
        STATE_COLOR_KEY,
        STATE_BLEND,
        STATE_BLEND_FACTOR,
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
        DWORD rep = state_table[i].representative;
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
                    if (!(dev_multistate_funcs[cur[i].state] = heap_calloc(2, sizeof(**dev_multistate_funcs))))
                        goto out_of_mem;

                    dev_multistate_funcs[cur[i].state][0] = multistate_funcs[cur[i].state][0];
                    dev_multistate_funcs[cur[i].state][1] = multistate_funcs[cur[i].state][1];
                    break;
                case 2:
                    state_table[cur[i].state].apply = multistate_apply_3;
                    if (!(funcs_array = heap_realloc(dev_multistate_funcs[cur[i].state],
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
        heap_free(dev_multistate_funcs[i]);
    }

    memset(dev_multistate_funcs, 0, (STATE_HIGHEST + 1) * sizeof(*dev_multistate_funcs));

    return E_OUTOFMEMORY;
}