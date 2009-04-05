/*
 * WINED3D draw functions
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2002-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006, 2008 Henri Verbeet
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
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
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_draw);
#define GLINFO_LOCATION This->adapter->gl_info

#include <stdio.h>
#include <math.h>

static void drawStridedFast(IWineD3DDevice *iface, GLenum primitive_type,
        UINT min_vertex_idx, UINT max_vertex_idx, UINT count, UINT idx_size,
        const void *idx_data, UINT start_idx)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    if (idx_size)
    {
        TRACE("(%p) : glElements(%x, %d, %d, ...)\n", This, primitive_type, count, min_vertex_idx);

#if 1
        glDrawElements(primitive_type, count,
                idx_size == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                (const char *)idx_data + (idx_size * start_idx));
        checkGLcall("glDrawElements");
#else
        glDrawRangeElements(primitive_type, min_vertex_idx, max_vertex_idx, count,
                idx_size == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                (const char *)idx_data + (idx_size * start_idx));
        checkGLcall("glDrawRangeElements");
#endif
    }
    else
    {
        TRACE("(%p) : glDrawArrays(%#x, %d, %d)\n", This, primitive_type, start_idx, count);

        glDrawArrays(primitive_type, start_idx, count);
        checkGLcall("glDrawArrays");
    }
}

/*
 * Actually draw using the supplied information.
 * Slower GL version which extracts info about each vertex in turn
 */

static void drawStridedSlow(IWineD3DDevice *iface, const struct wined3d_stream_info *si, UINT NumVertexes,
        GLenum glPrimType, const void *idxData, UINT idxSize, UINT minIndex, UINT startIdx)
{
    unsigned int               textureNo    = 0;
    const WORD                *pIdxBufS     = NULL;
    const DWORD               *pIdxBufL     = NULL;
    UINT vx_index;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    const UINT *streamOffset = This->stateBlock->streamOffset;
    long                      SkipnStrides = startIdx + This->stateBlock->loadBaseVertexIndex;
    BOOL                      pixelShader = use_ps(This->stateBlock);
    BOOL specular_fog = FALSE;
    UINT texture_stages = GL_LIMITS(texture_stages);
    const BYTE *texCoords[WINED3DDP_MAXTEXCOORD];
    const BYTE *diffuse = NULL, *specular = NULL, *normal = NULL, *position = NULL;
    const struct wined3d_stream_info_element *element;
    DWORD tex_mask = 0;

    TRACE("Using slow vertex array code\n");

    /* Variable Initialization */
    if (idxSize != 0) {
        /* Immediate mode drawing can't make use of indices in a vbo - get the data from the index buffer.
         * If the index buffer has no vbo(not supported or other reason), or with user pointer drawing
         * idxData will be != NULL
         */
        if(idxData == NULL) {
            idxData = ((IWineD3DIndexBufferImpl *) This->stateBlock->pIndexData)->resource.allocatedMemory;
        }

        if (idxSize == 2) pIdxBufS = idxData;
        else pIdxBufL = idxData;
    } else if (idxData) {
        ERR("non-NULL idxData with 0 idxSize, this should never happen\n");
        return;
    }

    /* Start drawing in GL */
    VTRACE(("glBegin(%x)\n", glPrimType));
    glBegin(glPrimType);

    element = &si->elements[WINED3D_FFP_POSITION];
    if (element->data) position = element->data + streamOffset[element->stream_idx];

    element = &si->elements[WINED3D_FFP_NORMAL];
    if (element->data) normal = element->data + streamOffset[element->stream_idx];
    else glNormal3f(0, 0, 0);

    element = &si->elements[WINED3D_FFP_DIFFUSE];
    if (element->data) diffuse = element->data + streamOffset[element->stream_idx];
    else glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    if (This->activeContext->num_untracked_materials && element->format_desc->format != WINED3DFMT_A8R8G8B8)
        FIXME("Implement diffuse color tracking from %s\n", debug_d3dformat(element->format_desc->format));

    element = &si->elements[WINED3D_FFP_SPECULAR];
    if (element->data)
    {
        specular = element->data + streamOffset[element->stream_idx];

        /* special case where the fog density is stored in the specular alpha channel */
        if (This->stateBlock->renderState[WINED3DRS_FOGENABLE]
                && (This->stateBlock->renderState[WINED3DRS_FOGVERTEXMODE] == WINED3DFOG_NONE
                    || si->elements[WINED3D_FFP_POSITION].format_desc->format == WINED3DFMT_R32G32B32A32_FLOAT)
                && This->stateBlock->renderState[WINED3DRS_FOGTABLEMODE] == WINED3DFOG_NONE)
        {
            if (GL_SUPPORT(EXT_FOG_COORD))
            {
                if (element->format_desc->format == WINED3DFMT_A8R8G8B8) specular_fog = TRUE;
                else FIXME("Implement fog coordinates from %s\n", debug_d3dformat(element->format_desc->format));
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
    else if (GL_SUPPORT(EXT_SECONDARY_COLOR))
    {
        GL_EXTCALL(glSecondaryColor3fEXT)(0, 0, 0);
    }

    for (textureNo = 0; textureNo < texture_stages; ++textureNo)
    {
        int coordIdx = This->stateBlock->textureState[textureNo][WINED3DTSS_TEXCOORDINDEX];
        int texture_idx = This->texUnitMap[textureNo];

        if (!GL_SUPPORT(ARB_MULTITEXTURE) && textureNo > 0)
        {
            FIXME("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
            continue;
        }

        if (!pixelShader && !This->stateBlock->textures[textureNo]) continue;

        if (texture_idx == -1) continue;

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

        element = &si->elements[WINED3D_FFP_TEXCOORD0 + coordIdx];
        if (element->data)
        {
            texCoords[coordIdx] = element->data + streamOffset[element->stream_idx];
            tex_mask |= (1 << textureNo);
        }
        else
        {
            TRACE("tex: %d - Skipping tex coords, as no data supplied\n", textureNo);
            if (GL_SUPPORT(ARB_MULTITEXTURE))
                GL_EXTCALL(glMultiTexCoord4fARB(GL_TEXTURE0_ARB + texture_idx, 0, 0, 0, 1));
            else
                glTexCoord4f(0, 0, 0, 1);
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
        if (idxData != NULL) {

            /* Indexed so work out the number of strides to skip */
            if (idxSize == 2) {
                VTRACE(("Idx for vertex %u = %u\n", vx_index, pIdxBufS[startIdx+vx_index]));
                SkipnStrides = pIdxBufS[startIdx + vx_index] + This->stateBlock->loadBaseVertexIndex;
            } else {
                VTRACE(("Idx for vertex %u = %u\n", vx_index, pIdxBufL[startIdx+vx_index]));
                SkipnStrides = pIdxBufL[startIdx + vx_index] + This->stateBlock->loadBaseVertexIndex;
            }
        }

        tmp_tex_mask = tex_mask;
        for (texture = 0; tmp_tex_mask; tmp_tex_mask >>= 1, ++texture)
        {
            int coord_idx;
            const void *ptr;
            int texture_idx;

            if (!(tmp_tex_mask & 1)) continue;

            coord_idx = This->stateBlock->textureState[texture][WINED3DTSS_TEXCOORDINDEX];
            ptr = texCoords[coord_idx] + (SkipnStrides * si->elements[WINED3D_FFP_TEXCOORD0 + coord_idx].stride);

            texture_idx = This->texUnitMap[texture];
            multi_texcoord_funcs[si->elements[WINED3D_FFP_TEXCOORD0 + coord_idx].format_desc->emit_idx](
                    GL_TEXTURE0_ARB + texture_idx, ptr);
        }

        /* Diffuse -------------------------------- */
        if (diffuse) {
            const void *ptrToCoords = diffuse + SkipnStrides * si->elements[WINED3D_FFP_DIFFUSE].stride;

            diffuse_funcs[si->elements[WINED3D_FFP_DIFFUSE].format_desc->emit_idx](ptrToCoords);
            if(This->activeContext->num_untracked_materials) {
                DWORD diffuseColor = ((const DWORD *)ptrToCoords)[0];
                unsigned char i;
                float color[4];

                color[0] = D3DCOLOR_B_R(diffuseColor) / 255.0;
                color[1] = D3DCOLOR_B_G(diffuseColor) / 255.0;
                color[2] = D3DCOLOR_B_B(diffuseColor) / 255.0;
                color[3] = D3DCOLOR_B_A(diffuseColor) / 255.0;

                for(i = 0; i < This->activeContext->num_untracked_materials; i++) {
                    glMaterialfv(GL_FRONT_AND_BACK, This->activeContext->untracked_materials[i], color);
                }
            }
        }

        /* Specular ------------------------------- */
        if (specular) {
            const void *ptrToCoords = specular + SkipnStrides * si->elements[WINED3D_FFP_SPECULAR].stride;

            specular_funcs[si->elements[WINED3D_FFP_SPECULAR].format_desc->emit_idx](ptrToCoords);

            if (specular_fog)
            {
                DWORD specularColor = *(const DWORD *)ptrToCoords;
                GL_EXTCALL(glFogCoordfEXT(specularColor >> 24));
            }
        }

        /* Normal -------------------------------- */
        if (normal != NULL) {
            const void *ptrToCoords = normal + SkipnStrides * si->elements[WINED3D_FFP_NORMAL].stride;
            normal_funcs[si->elements[WINED3D_FFP_NORMAL].format_desc->emit_idx](ptrToCoords);
        }

        /* Position -------------------------------- */
        if (position) {
            const void *ptrToCoords = position + SkipnStrides * si->elements[WINED3D_FFP_POSITION].stride;
            position_funcs[si->elements[WINED3D_FFP_POSITION].format_desc->emit_idx](ptrToCoords);
        }

        /* For non indexed mode, step onto next parts */
        if (idxData == NULL) {
            ++SkipnStrides;
        }
    }

    glEnd();
    checkGLcall("glEnd and previous calls");
}

static inline void send_attribute(IWineD3DDeviceImpl *This, WINED3DFORMAT format, const UINT index, const void *ptr)
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
        case WINED3DFMT_A8R8G8B8:
            if (GL_SUPPORT(EXT_VERTEX_ARRAY_BGRA))
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
            if (GL_SUPPORT(NV_HALF_FLOAT)) {
                GL_EXTCALL(glVertexAttrib2hvNV(index, ptr));
            } else {
                float x = float_16_to_32(((const unsigned short *)ptr) + 0);
                float y = float_16_to_32(((const unsigned short *)ptr) + 1);
                GL_EXTCALL(glVertexAttrib2fARB(index, x, y));
            }
            break;
        case WINED3DFMT_R16G16B16A16_FLOAT:
            if (GL_SUPPORT(NV_HALF_FLOAT)) {
                GL_EXTCALL(glVertexAttrib4hvNV(index, ptr));
            } else {
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

static void drawStridedSlowVs(IWineD3DDevice *iface, const struct wined3d_stream_info *si, UINT numberOfVertices,
        GLenum glPrimitiveType, const void *idxData, UINT idxSize, UINT minIndex, UINT startIdx)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    long                      SkipnStrides = startIdx + This->stateBlock->loadBaseVertexIndex;
    const WORD                *pIdxBufS     = NULL;
    const DWORD               *pIdxBufL     = NULL;
    UINT vx_index;
    int i;
    IWineD3DStateBlockImpl *stateblock = This->stateBlock;
    const BYTE *ptr;

    if (idxSize != 0) {
        /* Immediate mode drawing can't make use of indices in a vbo - get the data from the index buffer.
         * If the index buffer has no vbo(not supported or other reason), or with user pointer drawing
         * idxData will be != NULL
         */
        if(idxData == NULL) {
            idxData = ((IWineD3DIndexBufferImpl *) stateblock->pIndexData)->resource.allocatedMemory;
        }

        if (idxSize == 2) pIdxBufS = idxData;
        else pIdxBufL = idxData;
    } else if (idxData) {
        ERR("non-NULL idxData with 0 idxSize, this should never happen\n");
        return;
    }

    /* Start drawing in GL */
    VTRACE(("glBegin(%x)\n", glPrimitiveType));
    glBegin(glPrimitiveType);

    for (vx_index = 0; vx_index < numberOfVertices; ++vx_index) {
        if (idxData != NULL) {

            /* Indexed so work out the number of strides to skip */
            if (idxSize == 2) {
                VTRACE(("Idx for vertex %d = %d\n", vx_index, pIdxBufS[startIdx+vx_index]));
                SkipnStrides = pIdxBufS[startIdx + vx_index] + stateblock->loadBaseVertexIndex;
            } else {
                VTRACE(("Idx for vertex %d = %d\n", vx_index, pIdxBufL[startIdx+vx_index]));
                SkipnStrides = pIdxBufL[startIdx + vx_index] + stateblock->loadBaseVertexIndex;
            }
        }

        for(i = MAX_ATTRIBS - 1; i >= 0; i--) {
            if(!si->elements[i].data) continue;

            ptr = si->elements[i].data +
                  si->elements[i].stride * SkipnStrides +
                  stateblock->streamOffset[si->elements[i].stream_idx];

            send_attribute(This, si->elements[i].format_desc->format, i, ptr);
        }
        SkipnStrides++;
    }

    glEnd();
}

static inline void drawStridedInstanced(IWineD3DDevice *iface, const struct wined3d_stream_info *si,
        UINT numberOfVertices, GLenum glPrimitiveType, const void *idxData, UINT idxSize, UINT minIndex,
        UINT startIdx)
{
    UINT numInstances = 0, i;
    int numInstancedAttribs = 0, j;
    UINT instancedData[sizeof(si->elements) / sizeof(*si->elements) /* 16 */];
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    IWineD3DStateBlockImpl *stateblock = This->stateBlock;

    if (idxSize == 0) {
        /* This is a nasty thing. MSDN says no hardware supports that and apps have to use software vertex processing.
         * We don't support this for now
         *
         * Shouldn't be too hard to support with opengl, in theory just call glDrawArrays instead of drawElements.
         * But the StreamSourceFreq value has a different meaning in that situation.
         */
        FIXME("Non-indexed instanced drawing is not supported\n");
        return;
    }

    TRACE("(%p) : glElements(%x, %d, %d, ...)\n", This, glPrimitiveType, numberOfVertices, minIndex);

    /* First, figure out how many instances we have to draw */
    for(i = 0; i < MAX_STREAMS; i++) {
        /* Look at the streams and take the first one which matches */
        if(((stateblock->streamFlags[i] & WINED3DSTREAMSOURCE_INSTANCEDATA) || (stateblock->streamFlags[i] & WINED3DSTREAMSOURCE_INDEXEDDATA)) && stateblock->streamSource[i]) {
            /* D3D9 could set streamFreq 0 with (INSTANCEDATA or INDEXEDDATA) and then it is handled as 1. See d3d9/tests/visual.c-> stream_test() */
            if(stateblock->streamFreq[i] == 0){
                numInstances = 1;
            } else {
                numInstances = stateblock->streamFreq[i]; /* use the specified number of instances from the first matched stream. See d3d9/tests/visual.c-> stream_test() */
            }
            break; /* break, because only the first suitable value is interesting */
        }
    }

    for (i = 0; i < sizeof(si->elements) / sizeof(*si->elements); ++i)
    {
        if (stateblock->streamFlags[si->elements[i].stream_idx] & WINED3DSTREAMSOURCE_INSTANCEDATA)
        {
            instancedData[numInstancedAttribs] = i;
            numInstancedAttribs++;
        }
    }

    /* now draw numInstances instances :-) */
    for(i = 0; i < numInstances; i++) {
        /* Specify the instanced attributes using immediate mode calls */
        for(j = 0; j < numInstancedAttribs; j++) {
            const BYTE *ptr = si->elements[instancedData[j]].data +
                        si->elements[instancedData[j]].stride * i +
                        stateblock->streamOffset[si->elements[instancedData[j]].stream_idx];
            if (si->elements[instancedData[j]].buffer_object)
            {
                struct wined3d_buffer *vb =
                        (struct wined3d_buffer *)stateblock->streamSource[si->elements[instancedData[j]].stream_idx];
                ptr += (long) vb->resource.allocatedMemory;
            }

            send_attribute(This, si->elements[instancedData[j]].format_desc->format, instancedData[j], ptr);
        }

        glDrawElements(glPrimitiveType, numberOfVertices, idxSize == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                    (const char *)idxData+(idxSize * startIdx));
        checkGLcall("glDrawElements");
    }
}

static inline void remove_vbos(IWineD3DDeviceImpl *This, struct wined3d_stream_info *s)
{
    unsigned int i;

    for (i = 0; i < (sizeof(s->elements) / sizeof(*s->elements)); ++i)
    {
        struct wined3d_stream_info_element *e = &s->elements[i];
        if (e->buffer_object)
        {
            struct wined3d_buffer *vb = (struct wined3d_buffer *)This->stateBlock->streamSource[e->stream_idx];
            e->buffer_object = 0;
            e->data = (BYTE *)((unsigned long)e->data + (unsigned long)vb->resource.allocatedMemory);
        }
    }
}

/* Routine common to the draw primitive and draw indexed primitive routines */
void drawPrimitive(IWineD3DDevice *iface, UINT index_count, UINT numberOfVertices,
        UINT StartIdx, UINT idxSize, const void *idxData, UINT minIndex)
{

    IWineD3DDeviceImpl           *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSurfaceImpl          *target;
    unsigned int i;

    if (!index_count) return;

    /* Invalidate the back buffer memory so LockRect will read it the next time */
    for(i = 0; i < GL_LIMITS(buffers); i++) {
        target = (IWineD3DSurfaceImpl *) This->render_targets[i];
        if (target) {
            IWineD3DSurface_LoadLocation((IWineD3DSurface *) target, SFLAG_INDRAWABLE, NULL);
            IWineD3DSurface_ModifyLocation((IWineD3DSurface *) target, SFLAG_INDRAWABLE, TRUE);
        }
    }

    /* Signals other modules that a drawing is in progress and the stateblock finalized */
    This->isInDraw = TRUE;

    ActivateContext(This, This->render_targets[0], CTXUSAGE_DRAWPRIM);

    if (This->stencilBufferTarget) {
        /* Note that this depends on the ActivateContext call above to set
         * This->render_offscreen properly */
        DWORD location = This->render_offscreen ? SFLAG_DS_OFFSCREEN : SFLAG_DS_ONSCREEN;
        surface_load_ds_location(This->stencilBufferTarget, location);
        surface_modify_ds_location(This->stencilBufferTarget, location);
    }

    /* Ok, we will be updating the screen from here onwards so grab the lock */
    ENTER_GL();
    {
        GLenum glPrimType = This->stateBlock->gl_primitive_type;
        BOOL emulation = FALSE;
        const struct wined3d_stream_info *stream_info = &This->strided_streams;
        struct wined3d_stream_info stridedlcl;

        if (!numberOfVertices) numberOfVertices = index_count;

        if (!use_vs(This->stateBlock))
        {
            if (!This->strided_streams.position_transformed && This->activeContext->num_untracked_materials
                    && This->stateBlock->renderState[WINED3DRS_LIGHTING])
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
            else if(This->activeContext->fog_coord && This->stateBlock->renderState[WINED3DRS_FOGENABLE]) {
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
                memcpy(&stridedlcl, &This->strided_streams, sizeof(stridedlcl));
                remove_vbos(This, &stridedlcl);
            }
        }

        if (This->useDrawStridedSlow || emulation) {
            /* Immediate mode drawing */
            if (use_vs(This->stateBlock))
            {
                static BOOL warned;
                if (!warned) {
                    FIXME("Using immediate mode with vertex shaders for half float emulation\n");
                    warned = TRUE;
                } else {
                    TRACE("Using immediate mode with vertex shaders for half float emulation\n");
                }
                drawStridedSlowVs(iface, stream_info, index_count, glPrimType, idxData, idxSize, minIndex, StartIdx);
            } else {
                drawStridedSlow(iface, stream_info, index_count, glPrimType, idxData, idxSize, minIndex, StartIdx);
            }
        } else if(This->instancedDraw) {
            /* Instancing emulation with mixing immediate mode and arrays */
            drawStridedInstanced(iface, &This->strided_streams, index_count,
                    glPrimType, idxData, idxSize, minIndex, StartIdx);
        } else {
            drawStridedFast(iface, glPrimType, minIndex, minIndex + numberOfVertices - 1,
                    index_count, idxSize, idxData, StartIdx);
        }
    }

    /* Finished updating the screen, restore lock */
    LEAVE_GL();
    TRACE("Done all gl drawing\n");

    /* Diagnostics */
#ifdef SHOW_FRAME_MAKEUP
    {
        static long int primCounter = 0;
        /* NOTE: set primCounter to the value reported by drawprim 
           before you want to to write frame makeup to /tmp */
        if (primCounter >= 0) {
            WINED3DLOCKED_RECT r;
            char buffer[80];
            IWineD3DSurface_LockRect(This->render_targets[0], &r, NULL, WINED3DLOCK_READONLY);
            sprintf(buffer, "/tmp/backbuffer_%ld.tga", primCounter);
            TRACE("Saving screenshot %s\n", buffer);
            IWineD3DSurface_SaveSnapshot(This->render_targets[0], buffer);
            IWineD3DSurface_UnlockRect(This->render_targets[0]);

#ifdef SHOW_TEXTURE_MAKEUP
           {
            IWineD3DSurface *pSur;
            int textureNo;
            for (textureNo = 0; textureNo < MAX_COMBINED_SAMPLERS; ++textureNo) {
                if (This->stateBlock->textures[textureNo] != NULL) {
                    sprintf(buffer, "/tmp/texture_%p_%ld_%d.tga", This->stateBlock->textures[textureNo], primCounter, textureNo);
                    TRACE("Saving texture %s\n", buffer);
                    if (IWineD3DBaseTexture_GetType(This->stateBlock->textures[textureNo]) == WINED3DRTYPE_TEXTURE) {
                            IWineD3DTexture_GetSurfaceLevel(This->stateBlock->textures[textureNo], 0, &pSur);
                            IWineD3DSurface_SaveSnapshot(pSur, buffer);
                            IWineD3DSurface_Release(pSur);
                    } else  {
                        FIXME("base Texture isn't of type texture %d\n", IWineD3DBaseTexture_GetType(This->stateBlock->textures[textureNo]));
                    }
                }
            }
           }
#endif
        }
        TRACE("drawprim #%ld\n", primCounter);
        ++primCounter;
    }
#endif

    /* Control goes back to the device, stateblock values may change again */
    This->isInDraw = FALSE;
}

static void normalize_normal(float *n) {
    float length = n[0] * n[0] + n[1] * n[1] + n[2] * n[2];
    if(length == 0.0) return;
    length = sqrt(length);
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
HRESULT tesselate_rectpatch(IWineD3DDeviceImpl *This,
                            struct WineD3DRectPatch *patch) {
    unsigned int i, j, num_quads, out_vertex_size, buffer_size, d3d_out_vertex_size;
    float max_x = 0.0, max_y = 0.0, max_z = 0.0, neg_z = 0.0;
    struct wined3d_stream_info stream_info;
    struct wined3d_stream_info_element *e;
    const BYTE *data;
    const WINED3DRECTPATCH_INFO *info = &patch->RectPatchInfo;
    DWORD vtxStride;
    GLenum feedback_type;
    GLfloat *feedbuffer;

    /* First, locate the position data. This is provided in a vertex buffer in the stateblock.
     * Beware of vbos
     */
    device_stream_info_from_declaration(This, FALSE, &stream_info, NULL);

    e = &stream_info.elements[WINED3D_FFP_POSITION];
    if (e->buffer_object)
    {
        struct wined3d_buffer *vb;
        vb = (struct wined3d_buffer *)This->stateBlock->streamSource[e->stream_idx];
        e->data = (BYTE *)((unsigned long)e->data + (unsigned long)vb->resource.allocatedMemory);
    }
    vtxStride = e->stride;
    data = e->data +
           vtxStride * info->Stride * info->StartVertexOffsetHeight +
           vtxStride * info->StartVertexOffsetWidth;

    /* Not entirely sure about what happens with transformed vertices */
    if (stream_info.position_transformed) FIXME("Transformed position in rectpatch generation\n");

    if(vtxStride % sizeof(GLfloat)) {
        /* glMap2f reads vertex sizes in GLfloats, the d3d stride is in bytes.
         * I don't see how the stride could not be a multiple of 4, but make sure
         * to check it
         */
        ERR("Vertex stride is not a multiple of sizeof(GLfloat)\n");
    }
    if(info->Basis != WINED3DBASIS_BEZIER) {
        FIXME("Basis is %s, how to handle this?\n", debug_d3dbasis(info->Basis));
    }
    if(info->Degree != WINED3DDEGREE_CUBIC) {
        FIXME("Degree is %s, how to handle this?\n", debug_d3ddegree(info->Degree));
    }

    /* First, get the boundary cube of the input data */
    for(j = 0; j < info->Height; j++) {
        for(i = 0; i < info->Width; i++) {
            const float *v = (const float *)(data + vtxStride * i + vtxStride * info->Stride * j);
            if(fabs(v[0]) > max_x) max_x = fabs(v[0]);
            if(fabs(v[1]) > max_y) max_y = fabs(v[1]);
            if(fabs(v[2]) > max_z) max_z = fabs(v[2]);
            if(v[2] < neg_z) neg_z = v[2];
        }
    }

    /* This needs some improvements in the vertex decl code */
    FIXME("Cannot find data to generate. Only generating position and normals\n");
    patch->has_normals = TRUE;
    patch->has_texcoords = FALSE;

    /* Simply activate the context for blitting. This disables all the things we don't want and
     * takes care of dirtifying. Dirtifying is preferred over pushing / popping, since drawing the
     * patch (as opposed to normal draws) will most likely need different changes anyway
     */
    ActivateContext(This, This->lastActiveRenderTarget, CTXUSAGE_BLIT);
    ENTER_GL();

    glMatrixMode(GL_PROJECTION);
    checkGLcall("glMatrixMode(GL_PROJECTION)");
    glLoadIdentity();
    checkGLcall("glLoadIndentity()");
    glScalef(1 / (max_x) , 1 / (max_y), max_z == 0 ? 1 : 1 / ( 2 * max_z));
    glTranslatef(0, 0, 0.5);
    checkGLcall("glScalef");
    glViewport(-max_x, -max_y, 2 * (max_x), 2 * (max_y));
    checkGLcall("glViewport");

    /* Some states to take care of. If we're in wireframe opengl will produce lines, and confuse
     * our feedback buffer parser
     */
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    checkGLcall("glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)");
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_FILLMODE));
    if(patch->has_normals) {
        static const GLfloat black[] = {0, 0, 0, 0};
        static const GLfloat red[]   = {1, 0, 0, 0};
        static const GLfloat green[] = {0, 1, 0, 0};
        static const GLfloat blue[]  = {0, 0, 1, 0};
        static const GLfloat white[] = {1, 1, 1, 1};
        glEnable(GL_LIGHTING);
        checkGLcall("glEnable(GL_LIGHTING)");
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, black);
        checkGLcall("glLightModel for MODEL_AMBIENT");
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_AMBIENT));

        for(i = 3; i < GL_LIMITS(lights); i++) {
            glDisable(GL_LIGHT0 + i);
            checkGLcall("glDisable(GL_LIGHT0 + i)");
            IWineD3DDeviceImpl_MarkStateDirty(This, STATE_ACTIVELIGHT(i));
        }

        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_ACTIVELIGHT(0));
        glLightfv(GL_LIGHT0, GL_DIFFUSE, red);
        glLightfv(GL_LIGHT0, GL_SPECULAR, black);
        glLightfv(GL_LIGHT0, GL_AMBIENT, black);
        glLightfv(GL_LIGHT0, GL_POSITION, red);
        glEnable(GL_LIGHT0);
        checkGLcall("Setting up light 1\n");
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_ACTIVELIGHT(1));
        glLightfv(GL_LIGHT1, GL_DIFFUSE, green);
        glLightfv(GL_LIGHT1, GL_SPECULAR, black);
        glLightfv(GL_LIGHT1, GL_AMBIENT, black);
        glLightfv(GL_LIGHT1, GL_POSITION, green);
        glEnable(GL_LIGHT1);
        checkGLcall("Setting up light 2\n");
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_ACTIVELIGHT(2));
        glLightfv(GL_LIGHT2, GL_DIFFUSE, blue);
        glLightfv(GL_LIGHT2, GL_SPECULAR, black);
        glLightfv(GL_LIGHT2, GL_AMBIENT, black);
        glLightfv(GL_LIGHT2, GL_POSITION, blue);
        glEnable(GL_LIGHT2);
        checkGLcall("Setting up light 3\n");

        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_MATERIAL);
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_COLORVERTEX));
        glDisable(GL_COLOR_MATERIAL);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, black);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, black);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);
        checkGLcall("Setting up materials\n");
    }

    /* Enable the needed maps.
     * GL_MAP2_VERTEX_3 is needed for positional data.
     * GL_AUTO_NORMAL to generate normals from the position. Do not use GL_MAP2_NORMAL.
     * GL_MAP2_TEXTURE_COORD_4 for texture coords
     */
    num_quads = ceilf(patch->numSegs[0]) * ceilf(patch->numSegs[1]);
    out_vertex_size = 3 /* position */;
    d3d_out_vertex_size = 3;
    glEnable(GL_MAP2_VERTEX_3);
    if(patch->has_normals && patch->has_texcoords) {
        FIXME("Texcoords not handled yet\n");
        feedback_type = GL_3D_COLOR_TEXTURE;
        out_vertex_size += 8;
        d3d_out_vertex_size += 7;
        glEnable(GL_AUTO_NORMAL);
        glEnable(GL_MAP2_TEXTURE_COORD_4);
    } else if(patch->has_texcoords) {
        FIXME("Texcoords not handled yet\n");
        feedback_type = GL_3D_COLOR_TEXTURE;
        out_vertex_size += 7;
        d3d_out_vertex_size += 4;
        glEnable(GL_MAP2_TEXTURE_COORD_4);
    } else if(patch->has_normals) {
        feedback_type = GL_3D_COLOR;
        out_vertex_size += 4;
        d3d_out_vertex_size += 3;
        glEnable(GL_AUTO_NORMAL);
    } else {
        feedback_type = GL_3D;
    }
    checkGLcall("glEnable vertex attrib generation");

    buffer_size = num_quads * out_vertex_size * 2 /* triangle list */ * 3 /* verts per tri */
                   + 4 * num_quads /* 2 triangle markers per quad + num verts in tri */;
    feedbuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, buffer_size * sizeof(float) * 8);

    glMap2f(GL_MAP2_VERTEX_3,
            0, 1, vtxStride / sizeof(float), info->Width,
            0, 1, info->Stride * vtxStride / sizeof(float), info->Height,
            (const GLfloat *)data);
    checkGLcall("glMap2f");
    if(patch->has_texcoords) {
        glMap2f(GL_MAP2_TEXTURE_COORD_4,
                0, 1, vtxStride / sizeof(float), info->Width,
                0, 1, info->Stride * vtxStride / sizeof(float), info->Height,
                (const GLfloat *)data);
        checkGLcall("glMap2f");
    }
    glMapGrid2f(ceilf(patch->numSegs[0]), 0.0, 1.0, ceilf(patch->numSegs[1]), 0.0, 1.0);
    checkGLcall("glMapGrid2f");

    glFeedbackBuffer(buffer_size * 2, feedback_type, feedbuffer);
    checkGLcall("glFeedbackBuffer");
    glRenderMode(GL_FEEDBACK);

    glEvalMesh2(GL_FILL, 0, ceilf(patch->numSegs[0]), 0, ceilf(patch->numSegs[1]));
    checkGLcall("glEvalMesh2\n");

    i = glRenderMode(GL_RENDER);
    if(i == -1) {
        LEAVE_GL();
        ERR("Feedback failed. Expected %d elements back\n", buffer_size);
        Sleep(10000);
        HeapFree(GetProcessHeap(), 0, feedbuffer);
        return WINED3DERR_DRIVERINTERNALERROR;
    } else if(i != buffer_size) {
        LEAVE_GL();
        ERR("Unexpected amount of elements returned. Expected %d, got %d\n", buffer_size, i);
        Sleep(10000);
        HeapFree(GetProcessHeap(), 0, feedbuffer);
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
        patch->mem[i + 2] = (feedbuffer[j + out_vertex_size * 2 + 4] - 0.5) * 4 * max_z; /* z, triangle 3 */
        if(patch->has_normals) {
            patch->mem[i + 3] = feedbuffer[j + out_vertex_size * 2 + 5];
            patch->mem[i + 4] = feedbuffer[j + out_vertex_size * 2 + 6];
            patch->mem[i + 5] = feedbuffer[j + out_vertex_size * 2 + 7];
        }
        i += d3d_out_vertex_size;

        patch->mem[i + 0] =  feedbuffer[j + out_vertex_size * 1 + 2]; /* x, triangle 2 */
        patch->mem[i + 1] =  feedbuffer[j + out_vertex_size * 1 + 3]; /* y, triangle 2 */
        patch->mem[i + 2] = (feedbuffer[j + out_vertex_size * 1 + 4] - 0.5) * 4 * max_z; /* z, triangle 2 */
        if(patch->has_normals) {
            patch->mem[i + 3] = feedbuffer[j + out_vertex_size * 1 + 5];
            patch->mem[i + 4] = feedbuffer[j + out_vertex_size * 1 + 6];
            patch->mem[i + 5] = feedbuffer[j + out_vertex_size * 1 + 7];
        }
        i += d3d_out_vertex_size;

        patch->mem[i + 0] =  feedbuffer[j + out_vertex_size * 0 + 2]; /* x, triangle 1 */
        patch->mem[i + 1] =  feedbuffer[j + out_vertex_size * 0 + 3]; /* y, triangle 1 */
        patch->mem[i + 2] = (feedbuffer[j + out_vertex_size * 0 + 4] - 0.5) * 4 * max_z; /* z, triangle 1 */
        if(patch->has_normals) {
            patch->mem[i + 3] = feedbuffer[j + out_vertex_size * 0 + 5];
            patch->mem[i + 4] = feedbuffer[j + out_vertex_size * 0 + 6];
            patch->mem[i + 5] = feedbuffer[j + out_vertex_size * 0 + 7];
        }
        i += d3d_out_vertex_size;
    }

    if(patch->has_normals) {
        /* Now do the same with reverse light directions */
        static const GLfloat x[] = {-1,  0,  0, 0};
        static const GLfloat y[] = { 0, -1,  0, 0};
        static const GLfloat z[] = { 0,  0, -1, 0};
        glLightfv(GL_LIGHT0, GL_POSITION, x);
        glLightfv(GL_LIGHT1, GL_POSITION, y);
        glLightfv(GL_LIGHT2, GL_POSITION, z);
        checkGLcall("Setting up reverse light directions\n");

        glRenderMode(GL_FEEDBACK);
        checkGLcall("glRenderMode(GL_FEEDBACK)");
        glEvalMesh2(GL_FILL, 0, ceilf(patch->numSegs[0]), 0, ceilf(patch->numSegs[1]));
        checkGLcall("glEvalMesh2\n");
        i = glRenderMode(GL_RENDER);
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
            if(patch->mem[i + 3] == 0.0)
                patch->mem[i + 3] = -feedbuffer[j + out_vertex_size * 2 + 5];
            if(patch->mem[i + 4] == 0.0)
                patch->mem[i + 4] = -feedbuffer[j + out_vertex_size * 2 + 6];
            if(patch->mem[i + 5] == 0.0)
                patch->mem[i + 5] = -feedbuffer[j + out_vertex_size * 2 + 7];
            normalize_normal(patch->mem + i + 3);
            i += d3d_out_vertex_size;

            if(patch->mem[i + 3] == 0.0)
                patch->mem[i + 3] = -feedbuffer[j + out_vertex_size * 1 + 5];
            if(patch->mem[i + 4] == 0.0)
                patch->mem[i + 4] = -feedbuffer[j + out_vertex_size * 1 + 6];
            if(patch->mem[i + 5] == 0.0)
                patch->mem[i + 5] = -feedbuffer[j + out_vertex_size * 1 + 7];
            normalize_normal(patch->mem + i + 3);
            i += d3d_out_vertex_size;

            if(patch->mem[i + 3] == 0.0)
                patch->mem[i + 3] = -feedbuffer[j + out_vertex_size * 0 + 5];
            if(patch->mem[i + 4] == 0.0)
                patch->mem[i + 4] = -feedbuffer[j + out_vertex_size * 0 + 6];
            if(patch->mem[i + 5] == 0.0)
                patch->mem[i + 5] = -feedbuffer[j + out_vertex_size * 0 + 7];
            normalize_normal(patch->mem + i + 3);
            i += d3d_out_vertex_size;
        }
    }

    glDisable(GL_MAP2_VERTEX_3);
    glDisable(GL_AUTO_NORMAL);
    glDisable(GL_MAP2_NORMAL);
    glDisable(GL_MAP2_TEXTURE_COORD_4);
    checkGLcall("glDisable vertex attrib generation");
    LEAVE_GL();

    HeapFree(GetProcessHeap(), 0, feedbuffer);

    vtxStride = 3 * sizeof(float);
    if(patch->has_normals) {
        vtxStride += 3 * sizeof(float);
    }
    if(patch->has_texcoords) {
        vtxStride += 4 * sizeof(float);
    }
    memset(&patch->strided, 0, sizeof(&patch->strided));
    patch->strided.position.format = WINED3DFMT_R32G32B32_FLOAT;
    patch->strided.position.lpData = (BYTE *) patch->mem;
    patch->strided.position.dwStride = vtxStride;

    if(patch->has_normals) {
        patch->strided.normal.format = WINED3DFMT_R32G32B32_FLOAT;
        patch->strided.normal.lpData = (BYTE *) patch->mem + 3 * sizeof(float) /* pos */;
        patch->strided.normal.dwStride = vtxStride;
    }
    if(patch->has_texcoords) {
        patch->strided.texCoords[0].format = WINED3DFMT_R32G32B32A32_FLOAT;
        patch->strided.texCoords[0].lpData = (BYTE *) patch->mem + 3 * sizeof(float) /* pos */;
        if(patch->has_normals) {
            patch->strided.texCoords[0].lpData += 3 * sizeof(float);
        }
        patch->strided.texCoords[0].dwStride = vtxStride;
    }

    return WINED3D_OK;
}
