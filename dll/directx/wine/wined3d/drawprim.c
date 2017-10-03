/*
 * WINED3D draw functions
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2002-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006, 2008 Henri Verbeet
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
 * Copyright 2009 Henri Verbeet for CodeWeavers
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

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_draw);
WINE_DECLARE_DEBUG_CHANNEL(d3d_perf);
WINE_DECLARE_DEBUG_CHANNEL(d3d);

/* Context activation is done by the caller. */
static void draw_primitive_arrays(struct wined3d_context *context, const struct wined3d_state *state,
        const void *idx_data, unsigned int idx_size, int base_vertex_idx, unsigned int start_idx,
        unsigned int count, unsigned int start_instance, unsigned int instance_count)
{
    const struct wined3d_ffp_attrib_ops *ops = &context->d3d_info->ffp_attrib_ops;
    GLenum idx_type = idx_size == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
    const struct wined3d_stream_info *si = &context->stream_info;
    unsigned int instanced_elements[ARRAY_SIZE(si->elements)];
    const struct wined3d_gl_info *gl_info = context->gl_info;
    unsigned int instanced_element_count = 0;
    unsigned int i, j;

    if (!instance_count)
    {
        if (!idx_size)
        {
            gl_info->gl_ops.gl.p_glDrawArrays(state->gl_primitive_type, start_idx, count);
            checkGLcall("glDrawArrays");
            return;
        }

        if (gl_info->supported[ARB_DRAW_ELEMENTS_BASE_VERTEX])
        {
            GL_EXTCALL(glDrawElementsBaseVertex(state->gl_primitive_type, count, idx_type,
                    (const char *)idx_data + (idx_size * start_idx), base_vertex_idx));
            checkGLcall("glDrawElementsBaseVertex");
            return;
        }

        gl_info->gl_ops.gl.p_glDrawElements(state->gl_primitive_type, count,
                idx_type, (const char *)idx_data + (idx_size * start_idx));
        checkGLcall("glDrawElements");
        return;
    }

    if (start_instance && !(gl_info->supported[ARB_BASE_INSTANCE] && gl_info->supported[ARB_INSTANCED_ARRAYS]))
        FIXME("Start instance (%u) not supported.\n", start_instance);

    if (gl_info->supported[ARB_INSTANCED_ARRAYS])
    {
        if (!idx_size)
        {
            if (gl_info->supported[ARB_BASE_INSTANCE])
            {
                GL_EXTCALL(glDrawArraysInstancedBaseInstance(state->gl_primitive_type, start_idx, count, instance_count, start_instance));
                checkGLcall("glDrawArraysInstancedBaseInstance");
                return;
            }

            GL_EXTCALL(glDrawArraysInstanced(state->gl_primitive_type, start_idx, count, instance_count));
            checkGLcall("glDrawArraysInstanced");
            return;
        }

        if (gl_info->supported[ARB_BASE_INSTANCE])
        {
            GL_EXTCALL(glDrawElementsInstancedBaseVertexBaseInstance(state->gl_primitive_type, count, idx_type,
                        (const char *)idx_data + (idx_size * start_idx), instance_count, base_vertex_idx, start_instance));
            checkGLcall("glDrawElementsInstancedBaseVertexBaseInstance");
            return;
        }
        if (gl_info->supported[ARB_DRAW_ELEMENTS_BASE_VERTEX])
        {
            GL_EXTCALL(glDrawElementsInstancedBaseVertex(state->gl_primitive_type, count, idx_type,
                        (const char *)idx_data + (idx_size * start_idx), instance_count, base_vertex_idx));
            checkGLcall("glDrawElementsInstancedBaseVertex");
            return;
        }

        GL_EXTCALL(glDrawElementsInstanced(state->gl_primitive_type, count, idx_type,
                    (const char *)idx_data + (idx_size * start_idx), instance_count));
        checkGLcall("glDrawElementsInstanced");
        return;
    }

    /* Instancing emulation by mixing immediate mode and arrays. */

    /* This is a nasty thing. MSDN says no hardware supports this and
     * applications have to use software vertex processing. We don't support
     * this for now.
     *
     * Shouldn't be too hard to support with OpenGL, in theory just call
     * glDrawArrays() instead of drawElements(). But the stream fequency value
     * has a different meaning in that situation. */
    if (!idx_size)
    {
        FIXME("Non-indexed instanced drawing is not supported\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(si->elements); ++i)
    {
        if (!(si->use_map & (1u << i)))
            continue;

        if (state->streams[si->elements[i].stream_idx].flags & WINED3DSTREAMSOURCE_INSTANCEDATA)
            instanced_elements[instanced_element_count++] = i;
    }

    for (i = 0; i < instance_count; ++i)
    {
        /* Specify the instanced attributes using immediate mode calls. */
        for (j = 0; j < instanced_element_count; ++j)
        {
            const struct wined3d_stream_info_element *element;
            unsigned int element_idx;
            const BYTE *ptr;

            element_idx = instanced_elements[j];
            element = &si->elements[element_idx];
            ptr = element->data.addr + element->stride * i;
            if (element->data.buffer_object)
                ptr += (ULONG_PTR)wined3d_buffer_load_sysmem(state->streams[element->stream_idx].buffer, context);
            ops->generic[element->format->emit_idx](element_idx, ptr);
        }

        if (gl_info->supported[ARB_DRAW_ELEMENTS_BASE_VERTEX])
        {
            GL_EXTCALL(glDrawElementsBaseVertex(state->gl_primitive_type, count, idx_type,
                        (const char *)idx_data + (idx_size * start_idx), base_vertex_idx));
            checkGLcall("glDrawElementsBaseVertex");
        }
        else
        {
            gl_info->gl_ops.gl.p_glDrawElements(state->gl_primitive_type, count, idx_type,
                        (const char *)idx_data + (idx_size * start_idx));
            checkGLcall("glDrawElements");
        }
    }
}

/* Context activation is done by the caller. */
static void draw_primitive_arrays_indirect(struct wined3d_context *context, const struct wined3d_state *state,
        const void *idx_data, unsigned int idx_size, struct wined3d_buffer *buffer, unsigned int offset)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    if (!gl_info->supported[ARB_DRAW_INDIRECT])
    {
        FIXME("Indirect draw not supported.\n");
        return;
    }

    wined3d_buffer_load(buffer, context, state);
    GL_EXTCALL(glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffer->buffer_object));

    if (idx_size)
    {
        GLenum idx_type = (idx_size == 2) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

        GL_EXTCALL(glDrawElementsIndirect(state->gl_primitive_type, idx_type,
                (const BYTE *)NULL + offset));
    }
    else
    {
        GL_EXTCALL(glDrawArraysIndirect(state->gl_primitive_type,
                (const BYTE *)NULL + offset));
    }

    GL_EXTCALL(glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0));
    checkGLcall("draw indirect");
}

static const BYTE *software_vertex_blending(struct wined3d_context *context,
        const struct wined3d_state *state, const struct wined3d_stream_info *si,
        unsigned int element_idx, unsigned int stride_idx, float *result)
{
#define SI_FORMAT(idx) (si->elements[(idx)].format->emit_idx)
#define SI_PTR(idx1, idx2) (si->elements[(idx1)].data.addr + si->elements[(idx1)].stride * (idx2))

    const float *data = (const float *)SI_PTR(element_idx, stride_idx);
    float vector[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float cur_weight, weight_sum = 0.0f;
    struct wined3d_matrix m;
    const BYTE *blend_index;
    const float *weights;
    int i, num_weights;

    if (element_idx != WINED3D_FFP_POSITION && element_idx != WINED3D_FFP_NORMAL)
        return (BYTE *)data;

    if (!use_indexed_vertex_blending(state, si) || !use_software_vertex_processing(context->device))
        return (BYTE *)data;

    if (!si->elements[WINED3D_FFP_BLENDINDICES].data.addr ||
        !si->elements[WINED3D_FFP_BLENDWEIGHT].data.addr)
    {
        FIXME("no blend indices / weights set\n");
        return (BYTE *)data;
    }

    if (SI_FORMAT(WINED3D_FFP_BLENDINDICES) != WINED3D_FFP_EMIT_UBYTE4)
    {
        FIXME("unsupported blend index format: %u\n", SI_FORMAT(WINED3D_FFP_BLENDINDICES));
        return (BYTE *)data;
    }

    /* FIXME: validate weight format */
    switch (state->render_states[WINED3D_RS_VERTEXBLEND])
    {
        case WINED3D_VBF_0WEIGHTS: num_weights = 0; break;
        case WINED3D_VBF_1WEIGHTS: num_weights = 1; break;
        case WINED3D_VBF_2WEIGHTS: num_weights = 2; break;
        case WINED3D_VBF_3WEIGHTS: num_weights = 3; break;
        default:
            FIXME("unsupported vertex blend render state: %u\n", state->render_states[WINED3D_RS_VERTEXBLEND]);
            return (BYTE *)data;
    }

    switch (SI_FORMAT(element_idx))
    {
        case WINED3D_FFP_EMIT_FLOAT4: vector[3] = data[3];
        case WINED3D_FFP_EMIT_FLOAT3: vector[2] = data[2];
        case WINED3D_FFP_EMIT_FLOAT2: vector[1] = data[1];
        case WINED3D_FFP_EMIT_FLOAT1: vector[0] = data[0]; break;
        default:
            FIXME("unsupported value format: %u\n", SI_FORMAT(element_idx));
            return (BYTE *)data;
    }

    blend_index = SI_PTR(WINED3D_FFP_BLENDINDICES, stride_idx);
    weights = (const float *)SI_PTR(WINED3D_FFP_BLENDWEIGHT, stride_idx);
    result[0] = result[1] = result[2] = result[3] = 0.0f;

    for (i = 0; i < num_weights + 1; i++)
    {
        cur_weight = (i < num_weights) ? weights[i] : 1.0f - weight_sum;
        get_modelview_matrix(context, state, blend_index[i], &m);

        if (element_idx == WINED3D_FFP_POSITION)
        {
            result[0] += cur_weight * (vector[0] * m._11 + vector[1] * m._21 + vector[2] * m._31 + vector[3] * m._41);
            result[1] += cur_weight * (vector[0] * m._12 + vector[1] * m._22 + vector[2] * m._32 + vector[3] * m._42);
            result[2] += cur_weight * (vector[0] * m._13 + vector[1] * m._23 + vector[2] * m._33 + vector[3] * m._43);
            result[3] += cur_weight * (vector[0] * m._14 + vector[1] * m._24 + vector[2] * m._34 + vector[3] * m._44);
        }
        else
        {
            if (context->d3d_info->wined3d_creation_flags & WINED3D_LEGACY_FFP_LIGHTING)
                invert_matrix_3d(&m, &m);
            else
                invert_matrix(&m, &m);

            /* multiply with transposed M */
            result[0] += cur_weight * (vector[0] * m._11 + vector[1] * m._12 + vector[2] * m._13);
            result[1] += cur_weight * (vector[0] * m._21 + vector[1] * m._22 + vector[2] * m._23);
            result[2] += cur_weight * (vector[0] * m._31 + vector[1] * m._32 + vector[2] * m._33);
        }

        weight_sum += weights[i];
    }

#undef SI_FORMAT
#undef SI_PTR

    return (BYTE *)result;
}

static unsigned int get_stride_idx(const void *idx_data, unsigned int idx_size,
        unsigned int base_vertex_idx, unsigned int start_idx, unsigned int vertex_idx)
{
    if (!idx_data)
        return start_idx + vertex_idx;
    if (idx_size == 2)
        return ((const WORD *)idx_data)[start_idx + vertex_idx] + base_vertex_idx;
    return ((const DWORD *)idx_data)[start_idx + vertex_idx] + base_vertex_idx;
}

/* Context activation is done by the caller. */
static void draw_primitive_immediate_mode(struct wined3d_context *context, const struct wined3d_state *state,
        const struct wined3d_stream_info *si, const void *idx_data, unsigned int idx_size,
        int base_vertex_idx, unsigned int start_idx, unsigned int vertex_count, unsigned int instance_count)
{
    const BYTE *position = NULL, *normal = NULL, *diffuse = NULL, *specular = NULL;
    const struct wined3d_d3d_info *d3d_info = context->d3d_info;
    unsigned int coord_idx, stride_idx, texture_idx, vertex_idx;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    const struct wined3d_stream_info_element *element;
    const BYTE *tex_coords[WINED3DDP_MAXTEXCOORD];
    unsigned int texture_unit, texture_stages;
    const struct wined3d_ffp_attrib_ops *ops;
    unsigned int untracked_material_count;
    unsigned int tex_mask = 0;
    BOOL specular_fog = FALSE;
    BOOL ps = use_ps(state);
    const void *ptr;
    float tmp[4];

    static unsigned int once;

    if (!once++)
        FIXME_(d3d_perf)("Drawing using immediate mode.\n");
    else
        WARN_(d3d_perf)("Drawing using immediate mode.\n");

    if (!idx_size && idx_data)
        ERR("Non-NULL idx_data with 0 idx_size, this should never happen.\n");

    if (instance_count)
        FIXME("Instancing not implemented.\n");

    /* Immediate mode drawing can't make use of indices in a VBO - get the
     * data from the index buffer. */
    if (idx_size)
        idx_data = wined3d_buffer_load_sysmem(state->index_buffer, context) + state->index_offset;

    ops = &d3d_info->ffp_attrib_ops;

    gl_info->gl_ops.gl.p_glBegin(state->gl_primitive_type);

    if (use_vs(state) || d3d_info->ffp_generic_attributes)
    {
        for (vertex_idx = 0; vertex_idx < vertex_count; ++vertex_idx)
        {
            unsigned int use_map = si->use_map;
            unsigned int element_idx;

            stride_idx = get_stride_idx(idx_data, idx_size, base_vertex_idx, start_idx, vertex_idx);
            for (element_idx = MAX_ATTRIBS - 1; use_map; use_map &= ~(1u << element_idx), --element_idx)
            {
                if (!(use_map & 1u << element_idx))
                    continue;

                ptr = software_vertex_blending(context, state, si, element_idx, stride_idx, tmp);
                ops->generic[si->elements[element_idx].format->emit_idx](element_idx, ptr);
            }
        }

        gl_info->gl_ops.gl.p_glEnd();
        return;
    }

    if (si->use_map & (1u << WINED3D_FFP_POSITION))
        position = si->elements[WINED3D_FFP_POSITION].data.addr;

    if (si->use_map & (1u << WINED3D_FFP_NORMAL))
        normal = si->elements[WINED3D_FFP_NORMAL].data.addr;
    else
        gl_info->gl_ops.gl.p_glNormal3f(0.0f, 0.0f, 0.0f);

    untracked_material_count = context->num_untracked_materials;
    if (si->use_map & (1u << WINED3D_FFP_DIFFUSE))
    {
        element = &si->elements[WINED3D_FFP_DIFFUSE];
        diffuse = element->data.addr;

        if (untracked_material_count && element->format->id != WINED3DFMT_B8G8R8A8_UNORM)
            FIXME("Implement diffuse color tracking from %s.\n", debug_d3dformat(element->format->id));
    }
    else
    {
        gl_info->gl_ops.gl.p_glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }

    if (si->use_map & (1u << WINED3D_FFP_SPECULAR))
    {
        element = &si->elements[WINED3D_FFP_SPECULAR];
        specular = element->data.addr;

        /* Special case where the fog density is stored in the specular alpha channel. */
        if (state->render_states[WINED3D_RS_FOGENABLE]
                && (state->render_states[WINED3D_RS_FOGVERTEXMODE] == WINED3D_FOG_NONE
                    || si->elements[WINED3D_FFP_POSITION].format->id == WINED3DFMT_R32G32B32A32_FLOAT)
                && state->render_states[WINED3D_RS_FOGTABLEMODE] == WINED3D_FOG_NONE)
        {
            if (gl_info->supported[EXT_FOG_COORD])
            {
                if (element->format->id == WINED3DFMT_B8G8R8A8_UNORM)
                    specular_fog = TRUE;
                else
                    FIXME("Implement fog coordinates from %s.\n", debug_d3dformat(element->format->id));
            }
            else
            {
                static unsigned int once;

                if (!once++)
                    FIXME("Implement fog for transformed vertices in software.\n");
            }
        }
    }
    else if (gl_info->supported[EXT_SECONDARY_COLOR])
    {
        GL_EXTCALL(glSecondaryColor3fEXT)(0.0f, 0.0f, 0.0f);
    }

    texture_stages = d3d_info->limits.ffp_blend_stages;
    for (texture_idx = 0; texture_idx < texture_stages; ++texture_idx)
    {
        if (!gl_info->supported[ARB_MULTITEXTURE] && texture_idx > 0)
        {
            FIXME("Program using multiple concurrent textures which this OpenGL implementation doesn't support.\n");
            continue;
        }

        if (!ps && !state->textures[texture_idx])
            continue;

        texture_unit = context->tex_unit_map[texture_idx];
        if (texture_unit == WINED3D_UNMAPPED_STAGE)
            continue;

        coord_idx = state->texture_states[texture_idx][WINED3D_TSS_TEXCOORD_INDEX];
        if (coord_idx > 7)
        {
            TRACE("Skipping generated coordinates (%#x) for texture %u.\n", coord_idx, texture_idx);
            continue;
        }

        if (si->use_map & (1u << (WINED3D_FFP_TEXCOORD0 + coord_idx)))
        {
            tex_coords[coord_idx] = si->elements[WINED3D_FFP_TEXCOORD0 + coord_idx].data.addr;
            tex_mask |= (1u << texture_idx);
        }
        else
        {
            TRACE("Setting default coordinates for texture %u.\n", texture_idx);
            if (gl_info->supported[ARB_MULTITEXTURE])
                GL_EXTCALL(glMultiTexCoord4fARB(GL_TEXTURE0_ARB + texture_unit, 0.0f, 0.0f, 0.0f, 1.0f));
            else
                gl_info->gl_ops.gl.p_glTexCoord4f(0.0f, 0.0f, 0.0f, 1.0f);
        }
    }

    /* Blending data and point sizes are not supported by this function. They
     * are not supported by the fixed function pipeline at all. A FIXME for
     * them is printed after decoding the vertex declaration. */
    for (vertex_idx = 0; vertex_idx < vertex_count; ++vertex_idx)
    {
        unsigned int tmp_tex_mask;

        stride_idx = get_stride_idx(idx_data, idx_size, base_vertex_idx, start_idx, vertex_idx);

        if (normal)
        {
            ptr = software_vertex_blending(context, state, si, WINED3D_FFP_NORMAL, stride_idx, tmp);
            ops->normal[si->elements[WINED3D_FFP_NORMAL].format->emit_idx](ptr);
        }

        if (diffuse)
        {
            ptr = diffuse + stride_idx * si->elements[WINED3D_FFP_DIFFUSE].stride;
            ops->diffuse[si->elements[WINED3D_FFP_DIFFUSE].format->emit_idx](ptr);

            if (untracked_material_count)
            {
                struct wined3d_color color;
                unsigned int i;

                wined3d_color_from_d3dcolor(&color, *(const DWORD *)ptr);
                for (i = 0; i < untracked_material_count; ++i)
                {
                    gl_info->gl_ops.gl.p_glMaterialfv(GL_FRONT_AND_BACK, context->untracked_materials[i], &color.r);
                }
            }
        }

        if (specular)
        {
            ptr = specular + stride_idx * si->elements[WINED3D_FFP_SPECULAR].stride;
            ops->specular[si->elements[WINED3D_FFP_SPECULAR].format->emit_idx](ptr);

            if (specular_fog)
                GL_EXTCALL(glFogCoordfEXT((float)(*(const DWORD *)ptr >> 24)));
        }

        tmp_tex_mask = tex_mask;
        for (texture_idx = 0; tmp_tex_mask; tmp_tex_mask >>= 1, ++texture_idx)
        {
            if (!(tmp_tex_mask & 1))
                continue;

            coord_idx = state->texture_states[texture_idx][WINED3D_TSS_TEXCOORD_INDEX];
            ptr = tex_coords[coord_idx] + (stride_idx * si->elements[WINED3D_FFP_TEXCOORD0 + coord_idx].stride);
            ops->texcoord[si->elements[WINED3D_FFP_TEXCOORD0 + coord_idx].format->emit_idx](
                    GL_TEXTURE0_ARB + context->tex_unit_map[texture_idx], ptr);
        }

        if (position)
        {
            ptr = software_vertex_blending(context, state, si, WINED3D_FFP_POSITION, stride_idx, tmp);
            ops->position[si->elements[WINED3D_FFP_POSITION].format->emit_idx](ptr);
        }
    }

    gl_info->gl_ops.gl.p_glEnd();
    checkGLcall("glEnd and previous calls");
}

static void remove_vbos(struct wined3d_context *context,
        const struct wined3d_state *state, struct wined3d_stream_info *s)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(s->elements); ++i)
    {
        struct wined3d_stream_info_element *e;

        if (!(s->use_map & (1u << i)))
            continue;

        e = &s->elements[i];
        if (e->data.buffer_object)
        {
            struct wined3d_buffer *vb = state->streams[e->stream_idx].buffer;
            e->data.buffer_object = 0;
            e->data.addr += (ULONG_PTR)wined3d_buffer_load_sysmem(vb, context);
        }
    }
}

static BOOL use_transform_feedback(const struct wined3d_state *state)
{
    const struct wined3d_shader *shader;
    if (!(shader = state->shader[WINED3D_SHADER_TYPE_GEOMETRY]))
        return FALSE;
    return shader->u.gs.so_desc.element_count;
}

static void context_pause_transform_feedback(struct wined3d_context *context, BOOL force)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    if (!context->transform_feedback_active || context->transform_feedback_paused)
        return;

    if (gl_info->supported[ARB_TRANSFORM_FEEDBACK2])
    {
        GL_EXTCALL(glPauseTransformFeedback());
        checkGLcall("glPauseTransformFeedback");
        context->transform_feedback_paused = 1;
        return;
    }

    WARN("Cannot pause transform feedback operations.\n");

    if (force)
        context_end_transform_feedback(context);
}

static GLenum gl_tfb_primitive_type_from_d3d(enum wined3d_primitive_type primitive_type)
{
    GLenum gl_primitive_type = gl_primitive_type_from_d3d(primitive_type);
    switch (gl_primitive_type)
    {
        case GL_POINTS:
            return GL_POINTS;

        case GL_LINE_STRIP:
        case GL_LINE_STRIP_ADJACENCY:
        case GL_LINES_ADJACENCY:
        case GL_LINES:
            return GL_LINES;

        case GL_TRIANGLE_FAN:
        case GL_TRIANGLE_STRIP:
        case GL_TRIANGLE_STRIP_ADJACENCY:
        case GL_TRIANGLES_ADJACENCY:
        case GL_TRIANGLES:
            return GL_TRIANGLES;

        default:
            return gl_primitive_type;
    }
}

/* Routine common to the draw primitive and draw indexed primitive routines */
void draw_primitive(struct wined3d_device *device, const struct wined3d_state *state,
        const struct wined3d_draw_parameters *parameters)
{
    BOOL emulation = FALSE, rasterizer_discard = FALSE;
    const struct wined3d_fb_state *fb = state->fb;
    const struct wined3d_stream_info *stream_info;
    struct wined3d_rendertarget_view *dsv, *rtv;
    struct wined3d_stream_info si_emulated;
    struct wined3d_fence *ib_fence = NULL;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    unsigned int i, idx_size = 0;
    const void *idx_data = NULL;

    if (!parameters->indirect && !parameters->u.direct.index_count)
        return;

    if (!(rtv = fb->render_targets[0]))
        rtv = fb->depth_stencil;
    if (rtv)
        context = context_acquire(device, wined3d_texture_from_resource(rtv->resource), rtv->sub_resource_idx);
    else
        context = context_acquire(device, NULL, 0);
    if (!context->valid)
    {
        context_release(context);
        WARN("Invalid context, skipping draw.\n");
        return;
    }
    gl_info = context->gl_info;

    if (!use_transform_feedback(state))
        context_pause_transform_feedback(context, TRUE);

    for (i = 0; i < gl_info->limits.buffers; ++i)
    {
        if (!(rtv = fb->render_targets[i]) || rtv->format->id == WINED3DFMT_NULL)
            continue;

        if (state->render_states[WINED3D_RS_COLORWRITE(i)])
        {
            wined3d_rendertarget_view_load_location(rtv, context, rtv->resource->draw_binding);
            wined3d_rendertarget_view_invalidate_location(rtv, ~rtv->resource->draw_binding);
        }
        else
        {
            wined3d_rendertarget_view_prepare_location(rtv, context, rtv->resource->draw_binding);
        }
    }

    if ((dsv = fb->depth_stencil))
    {
        /* Note that this depends on the context_acquire() call above to set
         * context->render_offscreen properly. We don't currently take the
         * Z-compare function into account, but we could skip loading the
         * depthstencil for D3DCMP_NEVER and D3DCMP_ALWAYS as well. Also note
         * that we never copy the stencil data.*/
        DWORD location = context->render_offscreen ? dsv->resource->draw_binding : WINED3D_LOCATION_DRAWABLE;

        if (state->render_states[WINED3D_RS_ZWRITEENABLE] || state->render_states[WINED3D_RS_ZENABLE])
            wined3d_rendertarget_view_load_location(dsv, context, location);
        else
            wined3d_rendertarget_view_prepare_location(dsv, context, location);
    }

    if (!context_apply_draw_state(context, device, state))
    {
        context_release(context);
        WARN("Unable to apply draw state, skipping draw.\n");
        return;
    }

    if (dsv && state->render_states[WINED3D_RS_ZWRITEENABLE])
    {
        DWORD location = context->render_offscreen ? dsv->resource->draw_binding : WINED3D_LOCATION_DRAWABLE;

        wined3d_rendertarget_view_validate_location(dsv, location);
        wined3d_rendertarget_view_invalidate_location(dsv, ~location);
    }

    stream_info = &context->stream_info;

    if (parameters->indexed)
    {
        struct wined3d_buffer *index_buffer = state->index_buffer;
        if (!index_buffer->buffer_object || !stream_info->all_vbo)
        {
            idx_data = index_buffer->resource.heap_memory;
        }
        else
        {
            ib_fence = index_buffer->fence;
            idx_data = NULL;
        }
        idx_data = (const BYTE *)idx_data + state->index_offset;

        if (state->index_format == WINED3DFMT_R16_UINT)
            idx_size = 2;
        else
            idx_size = 4;
    }

    if (!use_vs(state))
    {
        if (!stream_info->position_transformed && context->num_untracked_materials
                && state->render_states[WINED3D_RS_LIGHTING])
        {
            static BOOL warned;

            if (!warned++)
                FIXME("Using software emulation because not all material properties could be tracked.\n");
            else
                WARN_(d3d_perf)("Using software emulation because not all material properties could be tracked.\n");
            emulation = TRUE;
        }
        else if (context->fog_coord && state->render_states[WINED3D_RS_FOGENABLE])
        {
            static BOOL warned;

            /* Either write a pipeline replacement shader or convert the
             * specular alpha from unsigned byte to a float in the vertex
             * buffer. */
            if (!warned++)
                FIXME("Using software emulation because manual fog coordinates are provided.\n");
            else
                WARN_(d3d_perf)("Using software emulation because manual fog coordinates are provided.\n");
            emulation = TRUE;
        }
        else if (use_indexed_vertex_blending(state, stream_info) && use_software_vertex_processing(context->device))
        {
            WARN_(d3d_perf)("Using software emulation because application requested SVP.\n");
            emulation = TRUE;
        }

        if (emulation)
        {
            si_emulated = context->stream_info;
            remove_vbos(context, state, &si_emulated);
            stream_info = &si_emulated;
        }
    }

    if (use_transform_feedback(state))
    {
        const struct wined3d_shader *shader = state->shader[WINED3D_SHADER_TYPE_GEOMETRY];

        if (shader->u.gs.so_desc.rasterizer_stream_idx == WINED3D_NO_RASTERIZER_STREAM)
        {
            glEnable(GL_RASTERIZER_DISCARD);
            checkGLcall("enable rasterizer discard");
            rasterizer_discard = TRUE;
        }

        if (context->transform_feedback_paused)
        {
            GL_EXTCALL(glResumeTransformFeedback());
            checkGLcall("glResumeTransformFeedback");
            context->transform_feedback_paused = 0;
        }
        else if (!context->transform_feedback_active)
        {
            GLenum mode = gl_tfb_primitive_type_from_d3d(shader->u.gs.output_type);
            GL_EXTCALL(glBeginTransformFeedback(mode));
            checkGLcall("glBeginTransformFeedback");
            context->transform_feedback_active = 1;
        }
    }

    if (state->gl_primitive_type == GL_PATCHES)
    {
        GL_EXTCALL(glPatchParameteri(GL_PATCH_VERTICES, state->gl_patch_vertices));
        checkGLcall("glPatchParameteri");
    }

    if (parameters->indirect)
    {
        if (context->use_immediate_mode_draw || emulation)
            FIXME("Indirect draw with immediate mode/emulation is not supported.\n");
        else
            draw_primitive_arrays_indirect(context, state, idx_data, idx_size,
                    parameters->u.indirect.buffer, parameters->u.indirect.offset);
    }
    else
    {
        unsigned int instance_count = parameters->u.direct.instance_count;
        if (context->instance_count)
            instance_count = context->instance_count;

        if (context->use_immediate_mode_draw || emulation)
            draw_primitive_immediate_mode(context, state, stream_info, idx_data,
                    idx_size, parameters->u.direct.base_vertex_idx,
                    parameters->u.direct.start_idx, parameters->u.direct.index_count, instance_count);
        else
            draw_primitive_arrays(context, state, idx_data, idx_size, parameters->u.direct.base_vertex_idx,
                    parameters->u.direct.start_idx, parameters->u.direct.index_count,
                    parameters->u.direct.start_instance, instance_count);
    }

    if (context->uses_uavs)
    {
        GL_EXTCALL(glMemoryBarrier(GL_ALL_BARRIER_BITS));
        checkGLcall("glMemoryBarrier");
    }

    context_pause_transform_feedback(context, FALSE);

    if (rasterizer_discard)
    {
        glDisable(GL_RASTERIZER_DISCARD);
        checkGLcall("disable rasterizer discard");
    }

    if (ib_fence)
        wined3d_fence_issue(ib_fence, device);
    for (i = 0; i < context->buffer_fence_count; ++i)
        wined3d_fence_issue(context->buffer_fences[i], device);

    if (wined3d_settings.strict_draw_ordering)
        gl_info->gl_ops.gl.p_glFlush(); /* Flush to ensure ordering across contexts. */

    context_release(context);

    TRACE("Done all gl drawing.\n");
}

void dispatch_compute(struct wined3d_device *device, const struct wined3d_state *state,
        const struct wined3d_dispatch_parameters *parameters)
{
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;

    context = context_acquire(device, NULL, 0);
    if (!context->valid)
    {
        context_release(context);
        WARN("Invalid context, skipping dispatch.\n");
        return;
    }
    gl_info = context->gl_info;

    if (!gl_info->supported[ARB_COMPUTE_SHADER])
    {
        context_release(context);
        FIXME("OpenGL implementation does not support compute shaders.\n");
        return;
    }

    context_apply_compute_state(context, device, state);

    if (!state->shader[WINED3D_SHADER_TYPE_COMPUTE])
    {
        context_release(context);
        WARN("No compute shader bound, skipping dispatch.\n");
        return;
    }

    if (parameters->indirect)
    {
        const struct wined3d_indirect_dispatch_parameters *indirect = &parameters->u.indirect;
        struct wined3d_buffer *buffer = indirect->buffer;

        wined3d_buffer_load(buffer, context, state);
        GL_EXTCALL(glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, buffer->buffer_object));
        GL_EXTCALL(glDispatchComputeIndirect((GLintptr)indirect->offset));
        GL_EXTCALL(glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, 0));
    }
    else
    {
        const struct wined3d_direct_dispatch_parameters *direct = &parameters->u.direct;
        GL_EXTCALL(glDispatchCompute(direct->group_count_x, direct->group_count_y, direct->group_count_z));
    }
    checkGLcall("dispatch compute");

    GL_EXTCALL(glMemoryBarrier(GL_ALL_BARRIER_BITS));
    checkGLcall("glMemoryBarrier");

    if (wined3d_settings.strict_draw_ordering)
        gl_info->gl_ops.gl.p_glFlush(); /* Flush to ensure ordering across contexts. */

    context_release(context);
}
