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

#include "config.h"
#include "wine/port.h"

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_draw);

#include <stdio.h>
#include <math.h>

/* GL locking is done by the caller */
static void drawStridedFast(const struct wined3d_gl_info *gl_info, GLenum primitive_type, UINT count, UINT idx_size,
        const void *idx_data, UINT start_idx, INT base_vertex_index)
{
    if (idx_size)
    {
        GLenum idxtype = idx_size == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
        if (gl_info->supported[ARB_DRAW_ELEMENTS_BASE_VERTEX])
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

/* GL locking is done by the caller */
static void drawStridedSlow(const struct wined3d_device *device, const struct wined3d_context *context,
        const struct wined3d_stream_info *si, UINT NumVertexes, GLenum glPrimType,
        const void *idxData, UINT idxSize, UINT startIdx)
{
    unsigned int               textureNo    = 0;
    const WORD                *pIdxBufS     = NULL;
    const DWORD               *pIdxBufL     = NULL;
    UINT vx_index;
    const struct wined3d_state *state = &device->stateBlock->state;
    LONG SkipnStrides = startIdx;
    BOOL pixelShader = use_ps(state);
    BOOL specular_fog = FALSE;
    const BYTE *texCoords[WINED3DDP_MAXTEXCOORD];
    const BYTE *diffuse = NULL, *specular = NULL, *normal = NULL, *position = NULL;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    UINT texture_stages = gl_info->limits.texture_stages;
    const struct wined3d_stream_info_element *element;
    UINT num_untracked_materials;
    DWORD tex_mask = 0;

    TRACE("Using slow vertex array code\n");

    /* Variable Initialization */
    if (idxSize)
    {
        /* Immediate mode drawing can't make use of indices in a vbo - get the
         * data from the index buffer. If the index buffer has no vbo (not
         * supported or other reason), or with user pointer drawing idxData
         * will be non-NULL. */
        if (!idxData)
            idxData = buffer_get_sysmem(state->index_buffer, gl_info);

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
        DWORD texture_idx = device->texUnitMap[textureNo];

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

            texture_idx = device->texUnitMap[texture];
            multi_texcoord_funcs[si->elements[WINED3D_FFP_TEXCOORD0 + coord_idx].format->emit_idx](
                    GL_TEXTURE0_ARB + texture_idx, ptr);
        }

        /* Diffuse -------------------------------- */
        if (diffuse) {
            const void *ptrToCoords = diffuse + SkipnStrides * si->elements[WINED3D_FFP_DIFFUSE].stride;

            diffuse_funcs[si->elements[WINED3D_FFP_DIFFUSE].format->emit_idx](ptrToCoords);
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
        if (specular) {
            const void *ptrToCoords = specular + SkipnStrides * si->elements[WINED3D_FFP_SPECULAR].stride;

            specular_funcs[si->elements[WINED3D_FFP_SPECULAR].format->emit_idx](ptrToCoords);

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
            normal_funcs[si->elements[WINED3D_FFP_NORMAL].format->emit_idx](ptrToCoords);
        }

        /* Position -------------------------------- */
        if (position) {
            const void *ptrToCoords = position + SkipnStrides * si->elements[WINED3D_FFP_POSITION].stride;
            position_funcs[si->elements[WINED3D_FFP_POSITION].format->emit_idx](ptrToCoords);
        }

        /* For non indexed mode, step onto next parts */
        if (!idxData) ++SkipnStrides;
    }

    gl_info->gl_ops.gl.p_glEnd();
    checkGLcall("glEnd and previous calls");
}

/* GL locking is done by the caller */
static inline void send_attribute(const struct wined3d_gl_info *gl_info,
        enum wined3d_format_id format, const UINT index, const void *ptr)
{
    switch(format)
    {
        case WINED3DFMT_R32_FLOAT:
            GL_EXTCALL(glVertexAttrib1fvARB(index, ptr));
            break;
        case WINED3DFMT_R32G32_FLOAT:
            GL_EXTCALL(glVertexAttrib2fvARB(index, ptr));
            break;
        case WINED3DFMT_R32G32B32_FLOAT:
            GL_EXTCALL(glVertexAttrib3fvARB(index, ptr));
            break;
        case WINED3DFMT_R32G32B32A32_FLOAT:
            GL_EXTCALL(glVertexAttrib4fvARB(index, ptr));
            break;

        case WINED3DFMT_R8G8B8A8_UINT:
            GL_EXTCALL(glVertexAttrib4ubvARB(index, ptr));
            break;
        case WINED3DFMT_B8G8R8A8_UNORM:
            if (gl_info->supported[ARB_VERTEX_ARRAY_BGRA])
            {
                const DWORD *src = ptr;
                DWORD c = *src & 0xff00ff00;
                c |= (*src & 0xff0000) >> 16;
                c |= (*src & 0xff) << 16;
                GL_EXTCALL(glVertexAttrib4NubvARB(index, (GLubyte *)&c));
                break;
            }
            /* else fallthrough */
        case WINED3DFMT_R8G8B8A8_UNORM:
            GL_EXTCALL(glVertexAttrib4NubvARB(index, ptr));
            break;

        case WINED3DFMT_R16G16_SINT:
            GL_EXTCALL(glVertexAttrib4svARB(index, ptr));
            break;
        case WINED3DFMT_R16G16B16A16_SINT:
            GL_EXTCALL(glVertexAttrib4svARB(index, ptr));
            break;

        case WINED3DFMT_R16G16_SNORM:
        {
            GLshort s[4] = {((const GLshort *)ptr)[0], ((const GLshort *)ptr)[1], 0, 1};
            GL_EXTCALL(glVertexAttrib4NsvARB(index, s));
            break;
        }
        case WINED3DFMT_R16G16_UNORM:
        {
            GLushort s[4] = {((const GLushort *)ptr)[0], ((const GLushort *)ptr)[1], 0, 1};
            GL_EXTCALL(glVertexAttrib4NusvARB(index, s));
            break;
        }
        case WINED3DFMT_R16G16B16A16_SNORM:
            GL_EXTCALL(glVertexAttrib4NsvARB(index, ptr));
            break;
        case WINED3DFMT_R16G16B16A16_UNORM:
            GL_EXTCALL(glVertexAttrib4NusvARB(index, ptr));
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
                GL_EXTCALL(glVertexAttrib2fARB(index, x, y));
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
                GL_EXTCALL(glVertexAttrib4fARB(index, x, y, z, w));
            }
            break;

        default:
            ERR("Unexpected attribute format: %s\n", debug_d3dformat(format));
            break;
    }
}

/* GL locking is done by the caller */
static void drawStridedSlowVs(const struct wined3d_gl_info *gl_info, const struct wined3d_state *state,
        const struct wined3d_stream_info *si, UINT numberOfVertices, GLenum glPrimitiveType,
        const void *idxData, UINT idxSize, UINT startIdx)
{
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
            idxData = buffer_get_sysmem(state->index_buffer, gl_info);

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

/* GL locking is done by the caller */
static void drawStridedInstanced(const struct wined3d_gl_info *gl_info, const struct wined3d_state *state,
        const struct wined3d_stream_info *si, UINT numberOfVertices, GLenum glPrimitiveType,
        const void *idxData, UINT idxSize, UINT startIdx, UINT base_vertex_index)
{
    UINT numInstances = 0, i;
    int numInstancedAttribs = 0, j;
    UINT instancedData[sizeof(si->elements) / sizeof(*si->elements) /* 16 */];
    GLenum idxtype = idxSize == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

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

    /* First, figure out how many instances we have to draw */
    for (i = 0; i < MAX_STREAMS; ++i)
    {
        /* Look at the streams and take the first one which matches */
        if (state->streams[i].buffer
                && ((state->streams[i].flags & WINED3DSTREAMSOURCE_INSTANCEDATA)
                || (state->streams[i].flags & WINED3DSTREAMSOURCE_INDEXEDDATA)))
        {
            /* Use the specified number of instances from the first matched
             * stream. A streamFreq of 0 (with INSTANCEDATA or INDEXEDDATA)
             * is handled as 1. See d3d9/tests/visual.c-> stream_test(). */
            numInstances = state->streams[i].frequency ? state->streams[i].frequency : 1;
            break;
        }
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

    /* now draw numInstances instances :-) */
    for(i = 0; i < numInstances; i++) {
        /* Specify the instanced attributes using immediate mode calls */
        for(j = 0; j < numInstancedAttribs; j++) {
            const BYTE *ptr = si->elements[instancedData[j]].data.addr
                    + si->elements[instancedData[j]].stride * i;
            if (si->elements[instancedData[j]].data.buffer_object)
            {
                struct wined3d_buffer *vb = state->streams[si->elements[instancedData[j]].stream_idx].buffer;
                ptr += (ULONG_PTR)buffer_get_sysmem(vb, gl_info);
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

static void remove_vbos(const struct wined3d_gl_info *gl_info,
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
            e->data.addr = (BYTE *)((ULONG_PTR)e->data.addr + (ULONG_PTR)buffer_get_sysmem(vb, gl_info));
        }
    }
}

/* Routine common to the draw primitive and draw indexed primitive routines */
void drawPrimitive(struct wined3d_device *device, UINT index_count, UINT StartIdx, BOOL indexed, const void *idxData)
{
    const struct wined3d_state *state = &device->stateBlock->state;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    unsigned int i;

    if (!index_count) return;

    if (state->render_states[WINED3D_RS_COLORWRITEENABLE])
    {
        /* Invalidate the back buffer memory so LockRect will read it the next time */
        for (i = 0; i < device->adapter->gl_info.limits.buffers; ++i)
        {
            struct wined3d_surface *target = device->fb.render_targets[i];
            if (target)
            {
                surface_load_location(target, target->draw_binding, NULL);
                surface_modify_location(target, target->draw_binding, TRUE);
            }
        }
    }

    /* Signals other modules that a drawing is in progress and the stateblock finalized */
    device->isInDraw = TRUE;

    context = context_acquire(device, device->fb.render_targets[0]);
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
        DWORD location = context->render_offscreen ? device->fb.depth_stencil->draw_binding : SFLAG_INDRAWABLE;
        if (state->render_states[WINED3D_RS_ZWRITEENABLE] || state->render_states[WINED3D_RS_ZENABLE])
        {
            struct wined3d_surface *ds = device->fb.depth_stencil;
            RECT current_rect, draw_rect, r;

            if (!context->render_offscreen && ds != device->onscreen_depth_stencil)
                device_switch_onscreen_ds(device, context, ds);

            if (ds->flags & location)
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
        struct wined3d_surface *ds = device->fb.depth_stencil;
        DWORD location = context->render_offscreen ? ds->draw_binding : SFLAG_INDRAWABLE;

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

    /* Ok, we will be updating the screen from here onwards so grab the lock */
    ENTER_GL();
    {
        GLenum glPrimType = state->gl_primitive_type;
        INT base_vertex_index = state->base_vertex_index;
        BOOL emulation = FALSE;
        const struct wined3d_stream_info *stream_info = &device->strided_streams;
        struct wined3d_stream_info stridedlcl;
        UINT idx_size = 0;

        if (indexed)
        {
            if (!state->user_stream)
            {
                struct wined3d_buffer *index_buffer = state->index_buffer;
                if (!index_buffer->buffer_object || !stream_info->all_vbo)
                    idxData = index_buffer->resource.allocatedMemory;
                else
                    idxData = NULL;
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
                if (!warned) {
                    FIXME("Using software emulation because not all material properties could be tracked\n");
                    warned = TRUE;
                } else {
                    TRACE("Using software emulation because not all material properties could be tracked\n");
                }
                emulation = TRUE;
            }
            else if (context->fog_coord && state->render_states[WINED3D_RS_FOGENABLE])
            {
                /* Either write a pipeline replacement shader or convert the specular alpha from unsigned byte
                 * to a float in the vertex buffer
                 */
                static BOOL warned;
                if (!warned) {
                    FIXME("Using software emulation because manual fog coordinates are provided\n");
                    warned = TRUE;
                } else {
                    TRACE("Using software emulation because manual fog coordinates are provided\n");
                }
                emulation = TRUE;
            }

            if(emulation) {
                stream_info = &stridedlcl;
                memcpy(&stridedlcl, &device->strided_streams, sizeof(stridedlcl));
                remove_vbos(gl_info, state, &stridedlcl);
            }
        }

        if (device->useDrawStridedSlow || emulation)
        {
            /* Immediate mode drawing */
            if (use_vs(state))
            {
                static BOOL warned;
                if (!warned) {
                    FIXME("Using immediate mode with vertex shaders for half float emulation\n");
                    warned = TRUE;
                } else {
                    TRACE("Using immediate mode with vertex shaders for half float emulation\n");
                }
                drawStridedSlowVs(gl_info, state, stream_info,
                        index_count, glPrimType, idxData, idx_size, StartIdx);
            }
            else
            {
                drawStridedSlow(device, context, stream_info, index_count,
                        glPrimType, idxData, idx_size, StartIdx);
            }
        }
        else if (device->instancedDraw)
        {
            /* Instancing emulation with mixing immediate mode and arrays */
            drawStridedInstanced(gl_info, state, stream_info,
                    index_count, glPrimType, idxData, idx_size, StartIdx, base_vertex_index);
        }
        else
        {
            drawStridedFast(gl_info, glPrimType, index_count, idx_size, idxData, StartIdx, base_vertex_index);
        }
    }

    /* Finished updating the screen, restore lock */
    LEAVE_GL();

    for(i = 0; i < device->num_buffer_queries; ++i)
    {
        wined3d_event_query_issue(device->buffer_queries[i], device);
    }

    if (wined3d_settings.strict_draw_ordering)
        gl_info->gl_ops.gl.p_glFlush(); /* Flush to ensure ordering across contexts. */

    context_release(context);

    TRACE("Done all gl drawing\n");

    /* Control goes back to the device, stateblock values may change again */
    device->isInDraw = FALSE;
}

static void normalize_normal(float *n) {
    float length = n[0] * n[0] + n[1] * n[1] + n[2] * n[2];
    if (length == 0.0f) return;
    length = sqrtf(length);
    n[0] = n[0] / length;
    n[1] = n[1] / length;
    n[2] = n[2] / length;
}

/* Tesselates a high order rectangular patch into single triangles using gl evaluators
 *
 * The problem is that OpenGL does not offer a direct way to return the tesselated primitives,
 * and they can't be sent off for rendering directly either. Tesselating is slow, so we want
 * to cache the patches in a vertex buffer. But more importantly, gl can't bind generated
 * attributes to numbered shader attributes, so we have to store them and rebind them as needed
 * in drawprim.
 *
 * To read back, the opengl feedback mode is used. This creates a problem because we want
 * untransformed, unlit vertices, but feedback runs everything through transform and lighting.
 * Thus disable lighting and set identity matrices to get unmodified colors and positions.
 * To overcome clipping find the biggest x, y and z values of the vertices in the patch and scale
 * them to [-1.0;+1.0] and set the viewport up to scale them back.
 *
 * Normals are more tricky: Draw white vertices with 3 directional lights, and calculate the
 * resulting colors back to the normals.
 *
 * NOTE: This function activates a context for blitting, modifies matrices & viewport, but
 * does not restore it because normally a draw follows immediately afterwards. The caller is
 * responsible of taking care that either the gl states are restored, or the context activated
 * for drawing to reset the lastWasBlit flag.
 */
HRESULT tesselate_rectpatch(struct wined3d_device *This, struct wined3d_rect_patch *patch)
{
    unsigned int i, j, num_quads, out_vertex_size, buffer_size, d3d_out_vertex_size;
    const struct wined3d_rect_patch_info *info = &patch->rect_patch_info;
    float max_x = 0.0f, max_y = 0.0f, max_z = 0.0f, neg_z = 0.0f;
    struct wined3d_state *state = &This->stateBlock->state;
    struct wined3d_stream_info stream_info;
    struct wined3d_stream_info_element *e;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    struct wined3d_shader *vs;
    const BYTE *data;
    DWORD vtxStride;
    GLenum feedback_type;
    GLfloat *feedbuffer;

    /* Simply activate the context for blitting. This disables all the things we don't want and
     * takes care of dirtifying. Dirtifying is preferred over pushing / popping, since drawing the
     * patch (as opposed to normal draws) will most likely need different changes anyway. */
    context = context_acquire(This, NULL);
    gl_info = context->gl_info;
    context_apply_blit_state(context, This);

    /* First, locate the position data. This is provided in a vertex buffer in
     * the stateblock. Beware of VBOs. */
    vs = state->vertex_shader;
    state->vertex_shader = NULL;
    device_stream_info_from_declaration(This, &stream_info);
    state->vertex_shader = vs;

    e = &stream_info.elements[WINED3D_FFP_POSITION];
    if (e->data.buffer_object)
    {
        struct wined3d_buffer *vb = state->streams[e->stream_idx].buffer;
        e->data.addr = (BYTE *)((ULONG_PTR)e->data.addr + (ULONG_PTR)buffer_get_sysmem(vb, context->gl_info));
    }
    vtxStride = e->stride;
    data = e->data.addr
            + vtxStride * info->stride * info->start_vertex_offset_height
            + vtxStride * info->start_vertex_offset_width;

    /* Not entirely sure about what happens with transformed vertices */
    if (stream_info.position_transformed) FIXME("Transformed position in rectpatch generation\n");

    if(vtxStride % sizeof(GLfloat)) {
        /* glMap2f reads vertex sizes in GLfloats, the d3d stride is in bytes.
         * I don't see how the stride could not be a multiple of 4, but make sure
         * to check it
         */
        ERR("Vertex stride is not a multiple of sizeof(GLfloat)\n");
    }
    if (info->basis != WINED3D_BASIS_BEZIER)
        FIXME("Basis is %s, how to handle this?\n", debug_d3dbasis(info->basis));
    if (info->degree != WINED3D_DEGREE_CUBIC)
        FIXME("Degree is %s, how to handle this?\n", debug_d3ddegree(info->degree));

    /* First, get the boundary cube of the input data */
    for (j = 0; j < info->height; ++j)
    {
        for (i = 0; i < info->width; ++i)
        {
            const float *v = (const float *)(data + vtxStride * i + vtxStride * info->stride * j);
            if(fabs(v[0]) > max_x) max_x = fabsf(v[0]);
            if(fabs(v[1]) > max_y) max_y = fabsf(v[1]);
            if(fabs(v[2]) > max_z) max_z = fabsf(v[2]);
            if(v[2] < neg_z) neg_z = v[2];
        }
    }

    /* This needs some improvements in the vertex decl code */
    FIXME("Cannot find data to generate. Only generating position and normals\n");
    patch->has_normals = TRUE;
    patch->has_texcoords = FALSE;

    ENTER_GL();

    gl_info->gl_ops.gl.p_glMatrixMode(GL_PROJECTION);
    checkGLcall("glMatrixMode(GL_PROJECTION)");
    gl_info->gl_ops.gl.p_glLoadIdentity();
    checkGLcall("glLoadIdentity()");
    gl_info->gl_ops.gl.p_glScalef(1.0f / (max_x), 1.0f / (max_y), max_z == 0.0f ? 1.0f : 1.0f / (2.0f * max_z));
    gl_info->gl_ops.gl.p_glTranslatef(0.0f, 0.0f, 0.5f);
    checkGLcall("glScalef");
    gl_info->gl_ops.gl.p_glViewport(-max_x, -max_y, 2 * (max_x), 2 * (max_y));
    checkGLcall("glViewport");

    /* Some states to take care of. If we're in wireframe opengl will produce lines, and confuse
     * our feedback buffer parser
     */
    gl_info->gl_ops.gl.p_glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    checkGLcall("glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)");
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_FILLMODE));
    if (patch->has_normals)
    {
        static const GLfloat black[] = {0.0f, 0.0f, 0.0f, 0.0f};
        static const GLfloat red[]   = {1.0f, 0.0f, 0.0f, 0.0f};
        static const GLfloat green[] = {0.0f, 1.0f, 0.0f, 0.0f};
        static const GLfloat blue[]  = {0.0f, 0.0f, 1.0f, 0.0f};
        static const GLfloat white[] = {1.0f, 1.0f, 1.0f, 1.0f};
        gl_info->gl_ops.gl.p_glEnable(GL_LIGHTING);
        checkGLcall("glEnable(GL_LIGHTING)");
        gl_info->gl_ops.gl.p_glLightModelfv(GL_LIGHT_MODEL_AMBIENT, black);
        checkGLcall("glLightModel for MODEL_AMBIENT");
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_AMBIENT));

        for (i = 3; i < context->gl_info->limits.lights; ++i)
        {
            gl_info->gl_ops.gl.p_glDisable(GL_LIGHT0 + i);
            checkGLcall("glDisable(GL_LIGHT0 + i)");
            context_invalidate_state(context, STATE_ACTIVELIGHT(i));
        }

        context_invalidate_state(context, STATE_ACTIVELIGHT(0));
        gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT0, GL_DIFFUSE, red);
        gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT0, GL_SPECULAR, black);
        gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT0, GL_AMBIENT, black);
        gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT0, GL_POSITION, red);
        gl_info->gl_ops.gl.p_glEnable(GL_LIGHT0);
        checkGLcall("Setting up light 1");
        context_invalidate_state(context, STATE_ACTIVELIGHT(1));
        gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT1, GL_DIFFUSE, green);
        gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT1, GL_SPECULAR, black);
        gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT1, GL_AMBIENT, black);
        gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT1, GL_POSITION, green);
        gl_info->gl_ops.gl.p_glEnable(GL_LIGHT1);
        checkGLcall("Setting up light 2");
        context_invalidate_state(context, STATE_ACTIVELIGHT(2));
        gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT2, GL_DIFFUSE, blue);
        gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT2, GL_SPECULAR, black);
        gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT2, GL_AMBIENT, black);
        gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT2, GL_POSITION, blue);
        gl_info->gl_ops.gl.p_glEnable(GL_LIGHT2);
        checkGLcall("Setting up light 3");

        context_invalidate_state(context, STATE_MATERIAL);
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORVERTEX));
        gl_info->gl_ops.gl.p_glDisable(GL_COLOR_MATERIAL);
        gl_info->gl_ops.gl.p_glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, black);
        gl_info->gl_ops.gl.p_glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, black);
        gl_info->gl_ops.gl.p_glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);
        checkGLcall("Setting up materials");
    }

    /* Enable the needed maps.
     * GL_MAP2_VERTEX_3 is needed for positional data.
     * GL_AUTO_NORMAL to generate normals from the position. Do not use GL_MAP2_NORMAL.
     * GL_MAP2_TEXTURE_COORD_4 for texture coords
     */
    num_quads = ceilf(patch->numSegs[0]) * ceilf(patch->numSegs[1]);
    out_vertex_size = 3 /* position */;
    d3d_out_vertex_size = 3;
    gl_info->gl_ops.gl.p_glEnable(GL_MAP2_VERTEX_3);
    if (patch->has_normals && patch->has_texcoords)
    {
        FIXME("Texcoords not handled yet\n");
        feedback_type = GL_3D_COLOR_TEXTURE;
        out_vertex_size += 8;
        d3d_out_vertex_size += 7;
        gl_info->gl_ops.gl.p_glEnable(GL_AUTO_NORMAL);
        gl_info->gl_ops.gl.p_glEnable(GL_MAP2_TEXTURE_COORD_4);
    }
    else if (patch->has_texcoords)
    {
        FIXME("Texcoords not handled yet\n");
        feedback_type = GL_3D_COLOR_TEXTURE;
        out_vertex_size += 7;
        d3d_out_vertex_size += 4;
        gl_info->gl_ops.gl.p_glEnable(GL_MAP2_TEXTURE_COORD_4);
    }
    else if (patch->has_normals)
    {
        feedback_type = GL_3D_COLOR;
        out_vertex_size += 4;
        d3d_out_vertex_size += 3;
        gl_info->gl_ops.gl.p_glEnable(GL_AUTO_NORMAL);
    }
    else
    {
        feedback_type = GL_3D;
    }
    checkGLcall("glEnable vertex attrib generation");

    buffer_size = num_quads * out_vertex_size * 2 /* triangle list */ * 3 /* verts per tri */
                   + 4 * num_quads /* 2 triangle markers per quad + num verts in tri */;
    feedbuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, buffer_size * sizeof(float) * 8);

    gl_info->gl_ops.gl.p_glMap2f(GL_MAP2_VERTEX_3,
            0.0f, 1.0f, vtxStride / sizeof(float), info->width,
            0.0f, 1.0f, info->stride * vtxStride / sizeof(float), info->height,
            (const GLfloat *)data);
    checkGLcall("glMap2f");
    if (patch->has_texcoords)
    {
        gl_info->gl_ops.gl.p_glMap2f(GL_MAP2_TEXTURE_COORD_4,
                0.0f, 1.0f, vtxStride / sizeof(float), info->width,
                0.0f, 1.0f, info->stride * vtxStride / sizeof(float), info->height,
                (const GLfloat *)data);
        checkGLcall("glMap2f");
    }
    gl_info->gl_ops.gl.p_glMapGrid2f(ceilf(patch->numSegs[0]), 0.0f, 1.0f, ceilf(patch->numSegs[1]), 0.0f, 1.0f);
    checkGLcall("glMapGrid2f");

    gl_info->gl_ops.gl.p_glFeedbackBuffer(buffer_size * 2, feedback_type, feedbuffer);
    checkGLcall("glFeedbackBuffer");
    gl_info->gl_ops.gl.p_glRenderMode(GL_FEEDBACK);

    gl_info->gl_ops.gl.p_glEvalMesh2(GL_FILL, 0, ceilf(patch->numSegs[0]), 0, ceilf(patch->numSegs[1]));
    checkGLcall("glEvalMesh2");

    i = gl_info->gl_ops.gl.p_glRenderMode(GL_RENDER);
    if (i == -1)
    {
        LEAVE_GL();
        ERR("Feedback failed. Expected %d elements back\n", buffer_size);
        HeapFree(GetProcessHeap(), 0, feedbuffer);
        context_release(context);
        return WINED3DERR_DRIVERINTERNALERROR;
    } else if(i != buffer_size) {
        LEAVE_GL();
        ERR("Unexpected amount of elements returned. Expected %d, got %d\n", buffer_size, i);
        HeapFree(GetProcessHeap(), 0, feedbuffer);
        context_release(context);
        return WINED3DERR_DRIVERINTERNALERROR;
    } else {
        TRACE("Got %d elements as expected\n", i);
    }

    HeapFree(GetProcessHeap(), 0, patch->mem);
    patch->mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, num_quads * 6 * d3d_out_vertex_size * sizeof(float) * 8);
    i = 0;
    for(j = 0; j < buffer_size; j += (3 /* num verts */ * out_vertex_size + 2 /* tri marker */)) {
        if(feedbuffer[j] != GL_POLYGON_TOKEN) {
            ERR("Unexpected token: %f\n", feedbuffer[j]);
            continue;
        }
        if(feedbuffer[j + 1] != 3) {
            ERR("Unexpected polygon: %f corners\n", feedbuffer[j + 1]);
            continue;
        }
        /* Somehow there are different ideas about back / front facing, so fix up the
         * vertex order
         */
        patch->mem[i + 0] =  feedbuffer[j + out_vertex_size * 2 + 2]; /* x, triangle 2 */
        patch->mem[i + 1] =  feedbuffer[j + out_vertex_size * 2 + 3]; /* y, triangle 2 */
        patch->mem[i + 2] = (feedbuffer[j + out_vertex_size * 2 + 4] - 0.5f) * 4.0f * max_z; /* z, triangle 3 */
        if(patch->has_normals) {
            patch->mem[i + 3] = feedbuffer[j + out_vertex_size * 2 + 5];
            patch->mem[i + 4] = feedbuffer[j + out_vertex_size * 2 + 6];
            patch->mem[i + 5] = feedbuffer[j + out_vertex_size * 2 + 7];
        }
        i += d3d_out_vertex_size;

        patch->mem[i + 0] =  feedbuffer[j + out_vertex_size * 1 + 2]; /* x, triangle 2 */
        patch->mem[i + 1] =  feedbuffer[j + out_vertex_size * 1 + 3]; /* y, triangle 2 */
        patch->mem[i + 2] = (feedbuffer[j + out_vertex_size * 1 + 4] - 0.5f) * 4.0f * max_z; /* z, triangle 2 */
        if(patch->has_normals) {
            patch->mem[i + 3] = feedbuffer[j + out_vertex_size * 1 + 5];
            patch->mem[i + 4] = feedbuffer[j + out_vertex_size * 1 + 6];
            patch->mem[i + 5] = feedbuffer[j + out_vertex_size * 1 + 7];
        }
        i += d3d_out_vertex_size;

        patch->mem[i + 0] =  feedbuffer[j + out_vertex_size * 0 + 2]; /* x, triangle 1 */
        patch->mem[i + 1] =  feedbuffer[j + out_vertex_size * 0 + 3]; /* y, triangle 1 */
        patch->mem[i + 2] = (feedbuffer[j + out_vertex_size * 0 + 4] - 0.5f) * 4.0f * max_z; /* z, triangle 1 */
        if(patch->has_normals) {
            patch->mem[i + 3] = feedbuffer[j + out_vertex_size * 0 + 5];
            patch->mem[i + 4] = feedbuffer[j + out_vertex_size * 0 + 6];
            patch->mem[i + 5] = feedbuffer[j + out_vertex_size * 0 + 7];
        }
        i += d3d_out_vertex_size;
    }

    if(patch->has_normals) {
        /* Now do the same with reverse light directions */
        static const GLfloat x[] = {-1.0f,  0.0f,  0.0f, 0.0f};
        static const GLfloat y[] = { 0.0f, -1.0f,  0.0f, 0.0f};
        static const GLfloat z[] = { 0.0f,  0.0f, -1.0f, 0.0f};
        gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT0, GL_POSITION, x);
        gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT1, GL_POSITION, y);
        gl_info->gl_ops.gl.p_glLightfv(GL_LIGHT2, GL_POSITION, z);
        checkGLcall("Setting up reverse light directions");

        gl_info->gl_ops.gl.p_glRenderMode(GL_FEEDBACK);
        checkGLcall("glRenderMode(GL_FEEDBACK)");
        gl_info->gl_ops.gl.p_glEvalMesh2(GL_FILL, 0, ceilf(patch->numSegs[0]), 0, ceilf(patch->numSegs[1]));
        checkGLcall("glEvalMesh2");
        i = gl_info->gl_ops.gl.p_glRenderMode(GL_RENDER);
        checkGLcall("glRenderMode(GL_RENDER)");

        i = 0;
        for(j = 0; j < buffer_size; j += (3 /* num verts */ * out_vertex_size + 2 /* tri marker */)) {
            if(feedbuffer[j] != GL_POLYGON_TOKEN) {
                ERR("Unexpected token: %f\n", feedbuffer[j]);
                continue;
            }
            if(feedbuffer[j + 1] != 3) {
                ERR("Unexpected polygon: %f corners\n", feedbuffer[j + 1]);
                continue;
            }
            if(patch->mem[i + 3] == 0.0f)
                patch->mem[i + 3] = -feedbuffer[j + out_vertex_size * 2 + 5];
            if(patch->mem[i + 4] == 0.0f)
                patch->mem[i + 4] = -feedbuffer[j + out_vertex_size * 2 + 6];
            if(patch->mem[i + 5] == 0.0f)
                patch->mem[i + 5] = -feedbuffer[j + out_vertex_size * 2 + 7];
            normalize_normal(patch->mem + i + 3);
            i += d3d_out_vertex_size;

            if(patch->mem[i + 3] == 0.0f)
                patch->mem[i + 3] = -feedbuffer[j + out_vertex_size * 1 + 5];
            if(patch->mem[i + 4] == 0.0f)
                patch->mem[i + 4] = -feedbuffer[j + out_vertex_size * 1 + 6];
            if(patch->mem[i + 5] == 0.0f)
                patch->mem[i + 5] = -feedbuffer[j + out_vertex_size * 1 + 7];
            normalize_normal(patch->mem + i + 3);
            i += d3d_out_vertex_size;

            if(patch->mem[i + 3] == 0.0f)
                patch->mem[i + 3] = -feedbuffer[j + out_vertex_size * 0 + 5];
            if(patch->mem[i + 4] == 0.0f)
                patch->mem[i + 4] = -feedbuffer[j + out_vertex_size * 0 + 6];
            if(patch->mem[i + 5] == 0.0f)
                patch->mem[i + 5] = -feedbuffer[j + out_vertex_size * 0 + 7];
            normalize_normal(patch->mem + i + 3);
            i += d3d_out_vertex_size;
        }
    }

    gl_info->gl_ops.gl.p_glDisable(GL_MAP2_VERTEX_3);
    gl_info->gl_ops.gl.p_glDisable(GL_AUTO_NORMAL);
    gl_info->gl_ops.gl.p_glDisable(GL_MAP2_NORMAL);
    gl_info->gl_ops.gl.p_glDisable(GL_MAP2_TEXTURE_COORD_4);
    checkGLcall("glDisable vertex attrib generation");
    LEAVE_GL();

    context_release(context);

    HeapFree(GetProcessHeap(), 0, feedbuffer);

    vtxStride = 3 * sizeof(float);
    if(patch->has_normals) {
        vtxStride += 3 * sizeof(float);
    }
    if(patch->has_texcoords) {
        vtxStride += 4 * sizeof(float);
    }
    memset(&patch->strided, 0, sizeof(patch->strided));
    patch->strided.position.format = WINED3DFMT_R32G32B32_FLOAT;
    patch->strided.position.data = (BYTE *)patch->mem;
    patch->strided.position.stride = vtxStride;

    if (patch->has_normals)
    {
        patch->strided.normal.format = WINED3DFMT_R32G32B32_FLOAT;
        patch->strided.normal.data = (BYTE *)patch->mem + 3 * sizeof(float) /* pos */;
        patch->strided.normal.stride = vtxStride;
    }
    if (patch->has_texcoords)
    {
        patch->strided.tex_coords[0].format = WINED3DFMT_R32G32B32A32_FLOAT;
        patch->strided.tex_coords[0].data = (BYTE *)patch->mem + 3 * sizeof(float) /* pos */;
        if (patch->has_normals)
            patch->strided.tex_coords[0].data += 3 * sizeof(float);
        patch->strided.tex_coords[0].stride = vtxStride;
    }

    return WINED3D_OK;
}
