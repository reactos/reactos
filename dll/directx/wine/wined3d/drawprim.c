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

/* Context activation is done by the caller. */
static void drawStridedFast(const struct wined3d_gl_info *gl_info, GLenum primitive_type, UINT count, UINT idx_size,
        const void *idx_data, UINT start_idx, INT base_vertex_index, UINT start_instance, UINT instance_count)
{
    if (idx_size)
    {
        GLenum idxtype = idx_size == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
        if (instance_count)
        {
            if (!gl_info->supported[ARB_DRAW_INSTANCED] && !gl_info->supported[ARB_INSTANCED_ARRAYS])
            {
                FIXME("Instanced drawing not supported.\n");
            }
            else
            {
                if (start_instance)
                    FIXME("Start instance (%u) not supported.\n", start_instance);
                if (gl_info->supported[ARB_DRAW_ELEMENTS_BASE_VERTEX])
                {
                    GL_EXTCALL(glDrawElementsInstancedBaseVertex(primitive_type, count, idxtype,
                            (const char *)idx_data + (idx_size * start_idx), instance_count, base_vertex_index));
                    checkGLcall("glDrawElementsInstancedBaseVertex");
                }
                else
                {
                    GL_EXTCALL(glDrawElementsInstanced(primitive_type, count, idxtype,
                            (const char *)idx_data + (idx_size * start_idx), instance_count));
                    checkGLcall("glDrawElementsInstanced");
                }
            }
        }
        else if (gl_info->supported[ARB_DRAW_ELEMENTS_BASE_VERTEX])
        {
            GL_EXTCALL(glDrawElementsBaseVertex(primitive_type, count, idxtype,
                    (const char *)idx_data + (idx_size * start_idx), base_vertex_index));
            checkGLcall("glDrawElementsBaseVertex");
        }
        else
        {
            gl_info->gl_ops.gl.p_glDrawElements(primitive_type, count,
                    idxtype, (const char *)idx_data + (idx_size * start_idx));
            checkGLcall("glDrawElements");
        }
    }
    else
    {
        gl_info->gl_ops.gl.p_glDrawArrays(primitive_type, start_idx, count);
        checkGLcall("glDrawArrays");
    }
}

/*
 * Actually draw using the supplied information.
 * Slower GL version which extracts info about each vertex in turn
 */

/* Context activation is done by the caller. */
static void drawStridedSlow(const struct wined3d_device *device, struct wined3d_context *context,
        const struct wined3d_stream_info *si, UINT NumVertexes, GLenum glPrimType,
        const void *idxData, UINT idxSize, UINT startIdx)
{
    unsigned int               textureNo    = 0;
    const WORD                *pIdxBufS     = NULL;
    const DWORD               *pIdxBufL     = NULL;
    UINT vx_index;
    const struct wined3d_state *state = &device->state;
    LONG SkipnStrides = startIdx;
    BOOL pixelShader = use_ps(state);
    BOOL specular_fog = FALSE;
    const BYTE *texCoords[WINED3DDP_MAXTEXCOORD];
    const BYTE *diffuse = NULL, *specular = NULL, *normal = NULL, *position = NULL;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    const struct wined3d_d3d_info *d3d_info = context->d3d_info;
    const struct wined3d_ffp_attrib_ops *ops = &d3d_info->ffp_attrib_ops;
    UINT texture_stages = d3d_info->limits.ffp_blend_stages;
    const struct wined3d_stream_info_element *element;
    UINT num_untracked_materials;
    DWORD tex_mask = 0;

    TRACE_(d3d_perf)("Using slow vertex array code\n");

    /* Variable Initialization */
    if (idxSize)
    {
        /* Immediate mode drawing can't make use of indices in a vbo - get the
         * data from the index buffer. If the index buffer has no vbo (not
         * supported or other reason), or with user pointer drawing idxData
         * will be non-NULL. */
        if (!idxData)
            idxData = buffer_get_sysmem(state->index_buffer, context);

        if (idxSize == 2) pIdxBufS = idxData;
        else pIdxBufL = idxData;
    } else if (idxData) {
        ERR("non-NULL idxData with 0 idxSize, this should never happen\n");
        return;
    }

    /* Start drawing in GL */
    gl_info->gl_ops.gl.p_glBegin(glPrimType);

    if (si->use_map & (1 << WINED3D_FFP_POSITION))
    {
        element = &si->elements[WINED3D_FFP_POSITION];
        position = element->data.addr;
    }

    if (si->use_map & (1 << WINED3D_FFP_NORMAL))
    {
        element = &si->elements[WINED3D_FFP_NORMAL];
        normal = element->data.addr;
    }
    else
    {
        gl_info->gl_ops.gl.p_glNormal3f(0, 0, 0);
    }

    num_untracked_materials = context->num_untracked_materials;
    if (si->use_map & (1 << WINED3D_FFP_DIFFUSE))
    {
        element = &si->elements[WINED3D_FFP_DIFFUSE];
        diffuse = element->data.addr;

        if (num_untracked_materials && element->format->id != WINED3DFMT_B8G8R8A8_UNORM)
            FIXME("Implement diffuse color tracking from %s\n", debug_d3dformat(element->format->id));
    }
    else
    {
        gl_info->gl_ops.gl.p_glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }

    if (si->use_map & (1 << WINED3D_FFP_SPECULAR))
    {
        element = &si->elements[WINED3D_FFP_SPECULAR];
        specular = element->data.addr;

        /* special case where the fog density is stored in the specular alpha channel */
        if (state->render_states[WINED3D_RS_FOGENABLE]
                && (state->render_states[WINED3D_RS_FOGVERTEXMODE] == WINED3D_FOG_NONE
                    || si->elements[WINED3D_FFP_POSITION].format->id == WINED3DFMT_R32G32B32A32_FLOAT)
                && state->render_states[WINED3D_RS_FOGTABLEMODE] == WINED3D_FOG_NONE)
        {
            if (gl_info->supported[EXT_FOG_COORD])
            {
                if (element->format->id == WINED3DFMT_B8G8R8A8_UNORM) specular_fog = TRUE;
                else FIXME("Implement fog coordinates from %s\n", debug_d3dformat(element->format->id));
            }
            else
            {
                static BOOL warned;

                if (!warned)
                {
                    /* TODO: Use the fog table code from old ddraw */
                    FIXME("Implement fog for transformed vertices in software\n");
                    warned = TRUE;
                }
            }
        }
    }
    else if (gl_info->supported[EXT_SECONDARY_COLOR])
    {
        GL_EXTCALL(glSecondaryColor3fEXT)(0, 0, 0);
    }

    for (textureNo = 0; textureNo < texture_stages; ++textureNo)
    {
        int coordIdx = state->texture_states[textureNo][WINED3D_TSS_TEXCOORD_INDEX];
        DWORD texture_idx = context->tex_unit_map[textureNo];

        if (!gl_info->supported[ARB_MULTITEXTURE] && textureNo > 0)
        {
            FIXME("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
            continue;
        }

        if (!pixelShader && !state->textures[textureNo]) continue;

        if (texture_idx == WINED3D_UNMAPPED_STAGE) continue;

        if (coordIdx > 7)
        {
            TRACE("tex: %d - Skip tex coords, as being system generated\n", textureNo);
            continue;
        }
        else if (coordIdx < 0)
        {
            FIXME("tex: %d - Coord index %d is less than zero, expect a crash.\n", textureNo, coordIdx);
            continue;
        }

        if (si->use_map & (1 << (WINED3D_FFP_TEXCOORD0 + coordIdx)))
        {
            element = &si->elements[WINED3D_FFP_TEXCOORD0 + coordIdx];
            texCoords[coordIdx] = element->data.addr;
            tex_mask |= (1 << textureNo);
        }
        else
        {
            TRACE("tex: %d - Skipping tex coords, as no data supplied\n", textureNo);
            if (gl_info->supported[ARB_MULTITEXTURE])
                GL_EXTCALL(glMultiTexCoord4fARB(GL_TEXTURE0_ARB + texture_idx, 0, 0, 0, 1));
            else
                gl_info->gl_ops.gl.p_glTexCoord4f(0, 0, 0, 1);
        }
    }

    /* We shouldn't start this function if any VBO is involved. Should I put a safety check here?
     * Guess it's not necessary(we crash then anyway) and would only eat CPU time
     */

    /* For each primitive */
    for (vx_index = 0; vx_index < NumVertexes; ++vx_index) {
        UINT texture, tmp_tex_mask;
        /* Blending data and Point sizes are not supported by this function. They are not supported by the fixed
         * function pipeline at all. A Fixme for them is printed after decoding the vertex declaration
         */

        /* For indexed data, we need to go a few more strides in */
        if (idxData)
        {
            /* Indexed so work out the number of strides to skip */
            if (idxSize == 2)
                SkipnStrides = pIdxBufS[startIdx + vx_index] + state->base_vertex_index;
            else
                SkipnStrides = pIdxBufL[startIdx + vx_index] + state->base_vertex_index;
        }

        tmp_tex_mask = tex_mask;
        for (texture = 0; tmp_tex_mask; tmp_tex_mask >>= 1, ++texture)
        {
            int coord_idx;
            const void *ptr;
            DWORD texture_idx;

            if (!(tmp_tex_mask & 1)) continue;

            coord_idx = state->texture_states[texture][WINED3D_TSS_TEXCOORD_INDEX];
            ptr = texCoords[coord_idx] + (SkipnStrides * si->elements[WINED3D_FFP_TEXCOORD0 + coord_idx].stride);

            texture_idx = context->tex_unit_map[texture];
            ops->texcoord[si->elements[WINED3D_FFP_TEXCOORD0 + coord_idx].format->emit_idx](
                    GL_TEXTURE0_ARB + texture_idx, ptr);
        }

        /* Diffuse -------------------------------- */
        if (diffuse)
        {
            const void *ptrToCoords = diffuse + SkipnStrides * si->elements[WINED3D_FFP_DIFFUSE].stride;
            ops->diffuse[si->elements[WINED3D_FFP_DIFFUSE].format->emit_idx](ptrToCoords);

            if (num_untracked_materials)
            {
                DWORD diffuseColor = ((const DWORD *)ptrToCoords)[0];
                unsigned char i;
                float color[4];

                color[0] = D3DCOLOR_B_R(diffuseColor) / 255.0f;
                color[1] = D3DCOLOR_B_G(diffuseColor) / 255.0f;
                color[2] = D3DCOLOR_B_B(diffuseColor) / 255.0f;
                color[3] = D3DCOLOR_B_A(diffuseColor) / 255.0f;

                for (i = 0; i < num_untracked_materials; ++i)
                {
                    gl_info->gl_ops.gl.p_glMaterialfv(GL_FRONT_AND_BACK, context->untracked_materials[i], color);
                }
            }
        }

        /* Specular ------------------------------- */
        if (specular)
        {
            const void *ptrToCoords = specular + SkipnStrides * si->elements[WINED3D_FFP_SPECULAR].stride;
            ops->specular[si->elements[WINED3D_FFP_SPECULAR].format->emit_idx](ptrToCoords);

            if (specular_fog)
            {
                DWORD specularColor = *(const DWORD *)ptrToCoords;
                GL_EXTCALL(glFogCoordfEXT((float) (specularColor >> 24)));
            }
        }

        /* Normal -------------------------------- */
        if (normal)
        {
            const void *ptrToCoords = normal + SkipnStrides * si->elements[WINED3D_FFP_NORMAL].stride;
            ops->normal[si->elements[WINED3D_FFP_NORMAL].format->emit_idx](ptrToCoords);
        }

        /* Position -------------------------------- */
        if (position)
        {
            const void *ptrToCoords = position + SkipnStrides * si->elements[WINED3D_FFP_POSITION].stride;
            ops->position[si->elements[WINED3D_FFP_POSITION].format->emit_idx](ptrToCoords);
        }

        /* For non indexed mode, step onto next parts */
        if (!idxData) ++SkipnStrides;
    }

    gl_info->gl_ops.gl.p_glEnd();
    checkGLcall("glEnd and previous calls");
}

/* Context activation is done by the caller. */
static inline void send_attribute(const struct wined3d_gl_info *gl_info,
        enum wined3d_format_id format, const UINT index, const void *ptr)
{
    switch(format)
    {
        case WINED3DFMT_R32_FLOAT:
            GL_EXTCALL(glVertexAttrib1fv(index, ptr));
            break;
        case WINED3DFMT_R32G32_FLOAT:
            GL_EXTCALL(glVertexAttrib2fv(index, ptr));
            break;
        case WINED3DFMT_R32G32B32_FLOAT:
            GL_EXTCALL(glVertexAttrib3fv(index, ptr));
            break;
        case WINED3DFMT_R32G32B32A32_FLOAT:
            GL_EXTCALL(glVertexAttrib4fv(index, ptr));
            break;

        case WINED3DFMT_R8G8B8A8_UINT:
            GL_EXTCALL(glVertexAttrib4ubv(index, ptr));
            break;
        case WINED3DFMT_B8G8R8A8_UNORM:
            if (gl_info->supported[ARB_VERTEX_ARRAY_BGRA])
            {
                const DWORD *src = ptr;
                DWORD c = *src & 0xff00ff00;
                c |= (*src & 0xff0000) >> 16;
                c |= (*src & 0xff) << 16;
                GL_EXTCALL(glVertexAttrib4Nubv(index, (GLubyte *)&c));
                break;
            }
            /* else fallthrough */
        case WINED3DFMT_R8G8B8A8_UNORM:
            GL_EXTCALL(glVertexAttrib4Nubv(index, ptr));
            break;

        case WINED3DFMT_R16G16_SINT:
            GL_EXTCALL(glVertexAttrib2sv(index, ptr));
            break;
        case WINED3DFMT_R16G16B16A16_SINT:
            GL_EXTCALL(glVertexAttrib4sv(index, ptr));
            break;

        case WINED3DFMT_R16G16_SNORM:
        {
            GLshort s[4] = {((const GLshort *)ptr)[0], ((const GLshort *)ptr)[1], 0, 1};
            GL_EXTCALL(glVertexAttrib4Nsv(index, s));
            break;
        }
        case WINED3DFMT_R16G16_UNORM:
        {
            GLushort s[4] = {((const GLushort *)ptr)[0], ((const GLushort *)ptr)[1], 0, 1};
            GL_EXTCALL(glVertexAttrib4Nusv(index, s));
            break;
        }
        case WINED3DFMT_R16G16B16A16_SNORM:
            GL_EXTCALL(glVertexAttrib4Nsv(index, ptr));
            break;
        case WINED3DFMT_R16G16B16A16_UNORM:
            GL_EXTCALL(glVertexAttrib4Nusv(index, ptr));
            break;

        case WINED3DFMT_R10G10B10A2_UINT:
            FIXME("Unsure about WINED3DDECLTYPE_UDEC3\n");
            /*glVertexAttrib3usvARB(instancedData[j], (GLushort *) ptr); Does not exist */
            break;
        case WINED3DFMT_R10G10B10A2_SNORM:
            FIXME("Unsure about WINED3DDECLTYPE_DEC3N\n");
            /*glVertexAttrib3NusvARB(instancedData[j], (GLushort *) ptr); Does not exist */
            break;

        case WINED3DFMT_R16G16_FLOAT:
            /* Are those 16 bit floats. C doesn't have a 16 bit float type. I could read the single bits and calculate a 4
             * byte float according to the IEEE standard
             */
            if (gl_info->supported[NV_HALF_FLOAT] && gl_info->supported[NV_VERTEX_PROGRAM])
            {
                /* Not supported by GL_ARB_half_float_vertex */
                GL_EXTCALL(glVertexAttrib2hvNV(index, ptr));
            }
            else
            {
                float x = float_16_to_32(((const unsigned short *)ptr) + 0);
                float y = float_16_to_32(((const unsigned short *)ptr) + 1);
                GL_EXTCALL(glVertexAttrib2f(index, x, y));
            }
            break;
        case WINED3DFMT_R16G16B16A16_FLOAT:
            if (gl_info->supported[NV_HALF_FLOAT] && gl_info->supported[NV_VERTEX_PROGRAM])
            {
                /* Not supported by GL_ARB_half_float_vertex */
                GL_EXTCALL(glVertexAttrib4hvNV(index, ptr));
            }
            else
            {
                float x = float_16_to_32(((const unsigned short *)ptr) + 0);
                float y = float_16_to_32(((const unsigned short *)ptr) + 1);
                float z = float_16_to_32(((const unsigned short *)ptr) + 2);
                float w = float_16_to_32(((const unsigned short *)ptr) + 3);
                GL_EXTCALL(glVertexAttrib4f(index, x, y, z, w));
            }
            break;

        default:
            ERR("Unexpected attribute format: %s\n", debug_d3dformat(format));
            break;
    }
}

/* Context activation is done by the caller. */
static void drawStridedSlowVs(struct wined3d_context *context, const struct wined3d_state *state,
        const struct wined3d_stream_info *si, UINT numberOfVertices, GLenum glPrimitiveType,
        const void *idxData, UINT idxSize, UINT startIdx)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    LONG SkipnStrides = startIdx + state->load_base_vertex_index;
    const DWORD *pIdxBufL = NULL;
    const WORD *pIdxBufS = NULL;
    UINT vx_index;
    int i;
    const BYTE *ptr;

    if (idxSize)
    {
        /* Immediate mode drawing can't make use of indices in a vbo - get the
         * data from the index buffer. If the index buffer has no vbo (not
         * supported or other reason), or with user pointer drawing idxData
         * will be non-NULL. */
        if (!idxData)
            idxData = buffer_get_sysmem(state->index_buffer, context);

        if (idxSize == 2) pIdxBufS = idxData;
        else pIdxBufL = idxData;
    } else if (idxData) {
        ERR("non-NULL idxData with 0 idxSize, this should never happen\n");
        return;
    }

    /* Start drawing in GL */
    gl_info->gl_ops.gl.p_glBegin(glPrimitiveType);

    for (vx_index = 0; vx_index < numberOfVertices; ++vx_index)
    {
        if (idxData)
        {
            /* Indexed so work out the number of strides to skip */
            if (idxSize == 2)
                SkipnStrides = pIdxBufS[startIdx + vx_index] + state->load_base_vertex_index;
            else
                SkipnStrides = pIdxBufL[startIdx + vx_index] + state->load_base_vertex_index;
        }

        for (i = MAX_ATTRIBS - 1; i >= 0; i--)
        {
            if (!(si->use_map & (1 << i))) continue;

            ptr = si->elements[i].data.addr + si->elements[i].stride * SkipnStrides;

            send_attribute(gl_info, si->elements[i].format->id, i, ptr);
        }
        SkipnStrides++;
    }

    gl_info->gl_ops.gl.p_glEnd();
}

/* Context activation is done by the caller. */
static void drawStridedInstanced(struct wined3d_context *context, const struct wined3d_state *state,
        const struct wined3d_stream_info *si, UINT numberOfVertices, GLenum glPrimitiveType,
        const void *idxData, UINT idxSize, UINT startIdx, UINT base_vertex_index, UINT instance_count)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    int numInstancedAttribs = 0, j;
    UINT instancedData[sizeof(si->elements) / sizeof(*si->elements) /* 16 */];
    GLenum idxtype = idxSize == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
    UINT i;

    if (!idxSize)
    {
        /* This is a nasty thing. MSDN says no hardware supports that and apps have to use software vertex processing.
         * We don't support this for now
         *
         * Shouldn't be too hard to support with opengl, in theory just call glDrawArrays instead of drawElements.
         * But the StreamSourceFreq value has a different meaning in that situation.
         */
        FIXME("Non-indexed instanced drawing is not supported\n");
        return;
    }

    for (i = 0; i < sizeof(si->elements) / sizeof(*si->elements); ++i)
    {
        if (!(si->use_map & (1 << i))) continue;

        if (state->streams[si->elements[i].stream_idx].flags & WINED3DSTREAMSOURCE_INSTANCEDATA)
        {
            instancedData[numInstancedAttribs] = i;
            numInstancedAttribs++;
        }
    }

    for (i = 0; i < instance_count; ++i)
    {
        /* Specify the instanced attributes using immediate mode calls */
        for(j = 0; j < numInstancedAttribs; j++) {
            const BYTE *ptr = si->elements[instancedData[j]].data.addr
                    + si->elements[instancedData[j]].stride * i;
            if (si->elements[instancedData[j]].data.buffer_object)
            {
                struct wined3d_buffer *vb = state->streams[si->elements[instancedData[j]].stream_idx].buffer;
                ptr += (ULONG_PTR)buffer_get_sysmem(vb, context);
            }

            send_attribute(gl_info, si->elements[instancedData[j]].format->id, instancedData[j], ptr);
        }

        if (gl_info->supported[ARB_DRAW_ELEMENTS_BASE_VERTEX])
        {
            GL_EXTCALL(glDrawElementsBaseVertex(glPrimitiveType, numberOfVertices, idxtype,
                        (const char *)idxData+(idxSize * startIdx), base_vertex_index));
            checkGLcall("glDrawElementsBaseVertex");
        }
        else
        {
            gl_info->gl_ops.gl.p_glDrawElements(glPrimitiveType, numberOfVertices, idxtype,
                        (const char *)idxData + (idxSize * startIdx));
            checkGLcall("glDrawElements");
        }
    }
}

static void remove_vbos(struct wined3d_context *context,
        const struct wined3d_state *state, struct wined3d_stream_info *s)
{
    unsigned int i;

    for (i = 0; i < (sizeof(s->elements) / sizeof(*s->elements)); ++i)
    {
        struct wined3d_stream_info_element *e;

        if (!(s->use_map & (1 << i))) continue;

        e = &s->elements[i];
        if (e->data.buffer_object)
        {
            struct wined3d_buffer *vb = state->streams[e->stream_idx].buffer;
            e->data.buffer_object = 0;
            e->data.addr = (BYTE *)((ULONG_PTR)e->data.addr + (ULONG_PTR)buffer_get_sysmem(vb, context));
        }
    }
}

/* Routine common to the draw primitive and draw indexed primitive routines */
void draw_primitive(struct wined3d_device *device, UINT start_idx, UINT index_count,
        UINT start_instance, UINT instance_count, BOOL indexed)
{
    const struct wined3d_state *state = &device->state;
    const struct wined3d_stream_info *stream_info;
    struct wined3d_event_query *ib_query = NULL;
    struct wined3d_stream_info si_emulated;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    BOOL emulation = FALSE;
    const void *idx_data = NULL;
    UINT idx_size = 0;
    unsigned int i;

    if (!index_count) return;

    if (state->render_states[WINED3D_RS_COLORWRITEENABLE])
    {
        /* Invalidate the back buffer memory so LockRect will read it the next time */
        for (i = 0; i < device->adapter->gl_info.limits.buffers; ++i)
        {
            struct wined3d_surface *target = wined3d_rendertarget_view_get_surface(device->fb.render_targets[i]);
            if (target)
            {
                surface_load_location(target, target->container->resource.draw_binding);
                surface_invalidate_location(target, ~target->container->resource.draw_binding);
            }
        }
    }

    context = context_acquire(device, wined3d_rendertarget_view_get_surface(device->fb.render_targets[0]));
    if (!context->valid)
    {
        context_release(context);
        WARN("Invalid context, skipping draw.\n");
        return;
    }
    gl_info = context->gl_info;

    if (device->fb.depth_stencil)
    {
        /* Note that this depends on the context_acquire() call above to set
         * context->render_offscreen properly. We don't currently take the
         * Z-compare function into account, but we could skip loading the
         * depthstencil for D3DCMP_NEVER and D3DCMP_ALWAYS as well. Also note
         * that we never copy the stencil data.*/
        DWORD location = context->render_offscreen ? device->fb.depth_stencil->resource->draw_binding
                : WINED3D_LOCATION_DRAWABLE;
        if (state->render_states[WINED3D_RS_ZWRITEENABLE] || state->render_states[WINED3D_RS_ZENABLE])
        {
            struct wined3d_surface *ds = wined3d_rendertarget_view_get_surface(device->fb.depth_stencil);
            RECT current_rect, draw_rect, r;

            if (!context->render_offscreen && ds != device->onscreen_depth_stencil)
                device_switch_onscreen_ds(device, context, ds);

            if (ds->locations & location)
                SetRect(&current_rect, 0, 0, ds->ds_current_size.cx, ds->ds_current_size.cy);
            else
                SetRectEmpty(&current_rect);

            wined3d_get_draw_rect(state, &draw_rect);

            IntersectRect(&r, &draw_rect, &current_rect);
            if (!EqualRect(&r, &draw_rect))
                surface_load_ds_location(ds, context, location);
        }
    }

    if (!context_apply_draw_state(context, device))
    {
        context_release(context);
        WARN("Unable to apply draw state, skipping draw.\n");
        return;
    }

    if (device->fb.depth_stencil && state->render_states[WINED3D_RS_ZWRITEENABLE])
    {
        struct wined3d_surface *ds = wined3d_rendertarget_view_get_surface(device->fb.depth_stencil);
        DWORD location = context->render_offscreen ? ds->container->resource.draw_binding : WINED3D_LOCATION_DRAWABLE;

        surface_modify_ds_location(ds, location, ds->ds_current_size.cx, ds->ds_current_size.cy);
    }

    if ((!gl_info->supported[WINED3D_GL_VERSION_2_0]
            || !gl_info->supported[NV_POINT_SPRITE])
            && context->render_offscreen
            && state->render_states[WINED3D_RS_POINTSPRITEENABLE]
            && state->gl_primitive_type == GL_POINTS)
    {
        FIXME("Point sprite coordinate origin switching not supported.\n");
    }

    stream_info = &context->stream_info;
    if (context->instance_count)
        instance_count = context->instance_count;

    if (indexed)
    {
        struct wined3d_buffer *index_buffer = state->index_buffer;
        if (!index_buffer->buffer_object || !stream_info->all_vbo)
            idx_data = index_buffer->resource.heap_memory;
        else
        {
            ib_query = index_buffer->query;
            idx_data = NULL;
        }

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

        if (emulation)
        {
            si_emulated = context->stream_info;
            remove_vbos(context, state, &si_emulated);
            stream_info = &si_emulated;
        }
    }

    if (context->use_immediate_mode_draw || emulation)
    {
        /* Immediate mode drawing. */
        if (use_vs(state))
        {
            static BOOL warned;

            if (!warned++)
                FIXME("Using immediate mode with vertex shaders for half float emulation.\n");
            else
                WARN_(d3d_perf)("Using immediate mode with vertex shaders for half float emulation.\n");

            drawStridedSlowVs(context, state, stream_info, index_count,
                    state->gl_primitive_type, idx_data, idx_size, start_idx);
        }
        else
        {
            drawStridedSlow(device, context, stream_info, index_count,
                    state->gl_primitive_type, idx_data, idx_size, start_idx);
        }
    }
    else if (!gl_info->supported[ARB_INSTANCED_ARRAYS] && instance_count)
    {
        /* Instancing emulation by mixing immediate mode and arrays. */
        drawStridedInstanced(context, state, stream_info, index_count, state->gl_primitive_type,
                idx_data, idx_size, start_idx, state->base_vertex_index, instance_count);
    }
    else
    {
        drawStridedFast(gl_info, state->gl_primitive_type, index_count, idx_size, idx_data,
                start_idx, state->base_vertex_index, start_instance, instance_count);
    }

    if (ib_query)
        wined3d_event_query_issue(ib_query, device);
    for (i = 0; i < context->num_buffer_queries; ++i)
    {
        wined3d_event_query_issue(context->buffer_queries[i], device);
    }

    if (wined3d_settings.strict_draw_ordering)
        gl_info->gl_ops.gl.p_glFlush(); /* Flush to ensure ordering across contexts. */

    context_release(context);

    TRACE("Done all gl drawing\n");
}
