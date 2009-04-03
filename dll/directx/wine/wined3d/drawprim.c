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

/* Issues the glBegin call for gl given the primitive type and count */
static DWORD primitiveToGl(WINED3DPRIMITIVETYPE PrimitiveType,
                    DWORD            NumPrimitives,
                    GLenum          *primType)
{
    DWORD   NumVertexes = NumPrimitives;

    switch (PrimitiveType) {
    case WINED3DPT_POINTLIST:
        TRACE("POINTS\n");
        *primType   = GL_POINTS;
        NumVertexes = NumPrimitives;
        break;

    case WINED3DPT_LINELIST:
        TRACE("LINES\n");
        *primType   = GL_LINES;
        NumVertexes = NumPrimitives * 2;
        break;

    case WINED3DPT_LINESTRIP:
        TRACE("LINE_STRIP\n");
        *primType   = GL_LINE_STRIP;
        NumVertexes = NumPrimitives + 1;
        break;

    case WINED3DPT_TRIANGLELIST:
        TRACE("TRIANGLES\n");
        *primType   = GL_TRIANGLES;
        NumVertexes = NumPrimitives * 3;
        break;

    case WINED3DPT_TRIANGLESTRIP:
        TRACE("TRIANGLE_STRIP\n");
        *primType   = GL_TRIANGLE_STRIP;
        NumVertexes = NumPrimitives + 2;
        break;

    case WINED3DPT_TRIANGLEFAN:
        TRACE("TRIANGLE_FAN\n");
        *primType   = GL_TRIANGLE_FAN;
        NumVertexes = NumPrimitives + 2;
        break;

    default:
        FIXME("Unhandled primitive\n");
        *primType    = GL_POINTS;
        break;
    }
    return NumVertexes;
}

static BOOL fixed_get_input(
    BYTE usage, BYTE usage_idx,
    unsigned int* regnum) {

    *regnum = -1;

    /* Those positions must have the order in the
     * named part of the strided data */

    if ((usage == WINED3DDECLUSAGE_POSITION || usage == WINED3DDECLUSAGE_POSITIONT) && usage_idx == 0)
        *regnum = 0;
    else if (usage == WINED3DDECLUSAGE_BLENDWEIGHT && usage_idx == 0)
        *regnum = 1;
    else if (usage == WINED3DDECLUSAGE_BLENDINDICES && usage_idx == 0)
        *regnum = 2;
    else if (usage == WINED3DDECLUSAGE_NORMAL && usage_idx == 0)
        *regnum = 3;
    else if (usage == WINED3DDECLUSAGE_PSIZE && usage_idx == 0)
        *regnum = 4;
    else if (usage == WINED3DDECLUSAGE_COLOR && usage_idx == 0)
        *regnum = 5;
    else if (usage == WINED3DDECLUSAGE_COLOR && usage_idx == 1)
        *regnum = 6;
    else if (usage == WINED3DDECLUSAGE_TEXCOORD && usage_idx < WINED3DDP_MAXTEXCOORD)
        *regnum = 7 + usage_idx;
    else if ((usage == WINED3DDECLUSAGE_POSITION || usage == WINED3DDECLUSAGE_POSITIONT) && usage_idx == 1)
        *regnum = 7 + WINED3DDP_MAXTEXCOORD;
    else if (usage == WINED3DDECLUSAGE_NORMAL && usage_idx == 1)
        *regnum = 8 + WINED3DDP_MAXTEXCOORD;
    else if (usage == WINED3DDECLUSAGE_TANGENT && usage_idx == 0)
        *regnum = 9 + WINED3DDP_MAXTEXCOORD;
    else if (usage == WINED3DDECLUSAGE_BINORMAL && usage_idx == 0)
        *regnum = 10 + WINED3DDP_MAXTEXCOORD;
    else if (usage == WINED3DDECLUSAGE_TESSFACTOR && usage_idx == 0)
        *regnum = 11 + WINED3DDP_MAXTEXCOORD;
    else if (usage == WINED3DDECLUSAGE_FOG && usage_idx == 0)
        *regnum = 12 + WINED3DDP_MAXTEXCOORD;
    else if (usage == WINED3DDECLUSAGE_DEPTH && usage_idx == 0)
        *regnum = 13 + WINED3DDP_MAXTEXCOORD;
    else if (usage == WINED3DDECLUSAGE_SAMPLE && usage_idx == 0)
        *regnum = 14 + WINED3DDP_MAXTEXCOORD;

    if (*regnum == -1) {
        FIXME("Unsupported input stream [usage=%s, usage_idx=%u]\n",
            debug_d3ddeclusage(usage), usage_idx);
        return FALSE;
    }
    return TRUE;
}

void primitiveDeclarationConvertToStridedData(
     IWineD3DDevice *iface,
     BOOL useVertexShaderFunction,
     WineDirect3DVertexStridedData *strided,
     BOOL *fixup) {

     /* We need to deal with frequency data!*/

    const BYTE *data = NULL;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DVertexDeclarationImpl* vertexDeclaration = (IWineD3DVertexDeclarationImpl *)This->stateBlock->vertexDecl;
    unsigned int i;
    const WINED3DVERTEXELEMENT *element;
    DWORD stride;
    DWORD numPreloadStreams = This->stateBlock->streamIsUP ? 0 : vertexDeclaration->num_streams;
    const DWORD *streams = vertexDeclaration->streams;

    /* Check for transformed vertices, disable vertex shader if present */
    strided->position_transformed = vertexDeclaration->position_transformed;
    if(vertexDeclaration->position_transformed) {
        useVertexShaderFunction = FALSE;
    }

    /* Translate the declaration into strided data */
    strided->swizzle_map = 0;
    for (i = 0 ; i < vertexDeclaration->declarationWNumElements - 1; ++i) {
        GLint streamVBO = 0;
        BOOL stride_used;
        unsigned int idx;

        element = vertexDeclaration->pDeclarationWine + i;
        TRACE("%p Element %p (%u of %u)\n", vertexDeclaration->pDeclarationWine,
            element,  i + 1, vertexDeclaration->declarationWNumElements - 1);

        if (This->stateBlock->streamSource[element->Stream] == NULL)
            continue;

        stride  = This->stateBlock->streamStride[element->Stream];
        if (This->stateBlock->streamIsUP) {
            TRACE("Stream is up %d, %p\n", element->Stream, This->stateBlock->streamSource[element->Stream]);
            streamVBO = 0;
            data    = (BYTE *)This->stateBlock->streamSource[element->Stream];
        } else {
            TRACE("Stream isn't up %d, %p\n", element->Stream, This->stateBlock->streamSource[element->Stream]);
            data    = IWineD3DVertexBufferImpl_GetMemory(This->stateBlock->streamSource[element->Stream], 0, &streamVBO);

            /* Can't use vbo's if the base vertex index is negative. OpenGL doesn't accept negative offsets
             * (or rather offsets bigger than the vbo, because the pointer is unsigned), so use system memory
             * sources. In most sane cases the pointer - offset will still be > 0, otherwise it will wrap
             * around to some big value. Hope that with the indices, the driver wraps it back internally. If
             * not, drawStridedSlow is needed, including a vertex buffer path.
             */
            if(This->stateBlock->loadBaseVertexIndex < 0) {
                WARN("loadBaseVertexIndex is < 0 (%d), not using vbos\n", This->stateBlock->loadBaseVertexIndex);
                streamVBO = 0;
                data = ((IWineD3DVertexBufferImpl *) This->stateBlock->streamSource[element->Stream])->resource.allocatedMemory;
                if((UINT_PTR)data < -This->stateBlock->loadBaseVertexIndex * stride) {
                    FIXME("System memory vertex data load offset is negative!\n");
                }
            }

            if(fixup) {
                if( streamVBO != 0) *fixup = TRUE;
                else if(*fixup && !useVertexShaderFunction &&
                       (element->Usage == WINED3DDECLUSAGE_COLOR ||
                        element->Usage == WINED3DDECLUSAGE_POSITIONT)) {
                    static BOOL warned = FALSE;
                    if(!warned) {
                        /* This may be bad with the fixed function pipeline */
                        FIXME("Missing vbo streams with unfixed colors or transformed position, expect problems\n");
                        warned = TRUE;
                    }
                }
            }
        }
        data += element->Offset;

        TRACE("Offset %d Stream %d UsageIndex %d\n", element->Offset, element->Stream, element->UsageIndex);

        if (useVertexShaderFunction)
            stride_used = vshader_get_input(This->stateBlock->vertexShader,
                element->Usage, element->UsageIndex, &idx);
        else
            stride_used = fixed_get_input(element->Usage, element->UsageIndex, &idx);

        if (stride_used) {
            TRACE("Load %s array %u [usage=%s, usage_idx=%u, "
                    "stream=%u, offset=%u, stride=%u, type=%s, VBO=%u]\n",
                    useVertexShaderFunction? "shader": "fixed function", idx,
                    debug_d3ddeclusage(element->Usage), element->UsageIndex,
                    element->Stream, element->Offset, stride, debug_d3ddecltype(element->Type), streamVBO);

            if (!useVertexShaderFunction && !vertexDeclaration->ffp_valid[i]) {
                WARN("Skipping unsupported fixed function element of type %s and usage %s\n",
                    debug_d3ddecltype(element->Type), debug_d3ddeclusage(element->Usage));
            } else {
                strided->u.input[idx].lpData = data;
                strided->u.input[idx].dwType = element->Type;
                strided->u.input[idx].dwStride = stride;
                strided->u.input[idx].VBO = streamVBO;
                strided->u.input[idx].streamNo = element->Stream;
                if (!GL_SUPPORT(EXT_VERTEX_ARRAY_BGRA) && element->Type == WINED3DDECLTYPE_D3DCOLOR)
                {
                    strided->swizzle_map |= 1 << idx;
                }
                strided->use_map |= 1 << idx;
            }
        }
    }
    /* Now call PreLoad on all the vertex buffers. In the very rare case
     * that the buffers stopps converting PreLoad will dirtify the VDECL again.
     * The vertex buffer can now use the strided structure in the device instead of finding its
     * own again.
     *
     * NULL streams won't be recorded in the array, UP streams won't be either. A stream is only
     * once in there.
     */
    for(i=0; i < numPreloadStreams; i++) {
        IWineD3DVertexBuffer *vb = This->stateBlock->streamSource[streams[i]];
        if(vb) {
            IWineD3DVertexBuffer_PreLoad(vb);
        }
    }
}

static void drawStridedFast(IWineD3DDevice *iface, GLenum primitive_type,
        UINT min_vertex_idx, UINT max_vertex_idx, UINT count, short idx_size,
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

static void drawStridedSlow(IWineD3DDevice *iface, const WineDirect3DVertexStridedData *sd, UINT NumVertexes,
        GLenum glPrimType, const void *idxData, short idxSize, ULONG minIndex, ULONG startIdx)
{
    unsigned int               textureNo    = 0;
    const WORD                *pIdxBufS     = NULL;
    const DWORD               *pIdxBufL     = NULL;
    ULONG                      vx_index;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    const UINT *streamOffset = This->stateBlock->streamOffset;
    long                      SkipnStrides = startIdx + This->stateBlock->loadBaseVertexIndex;
    BOOL                      pixelShader = use_ps(This->stateBlock);
    BOOL specular_fog = FALSE;
    UINT texture_stages = GL_LIMITS(texture_stages);
    const BYTE *texCoords[WINED3DDP_MAXTEXCOORD];
    const BYTE *diffuse = NULL, *specular = NULL, *normal = NULL, *position = NULL;
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

    if (sd->u.s.position.lpData) position = sd->u.s.position.lpData + streamOffset[sd->u.s.position.streamNo];

    if (sd->u.s.normal.lpData) normal = sd->u.s.normal.lpData + streamOffset[sd->u.s.normal.streamNo];
    else glNormal3f(0, 0, 0);

    if (sd->u.s.diffuse.lpData) diffuse = sd->u.s.diffuse.lpData + streamOffset[sd->u.s.diffuse.streamNo];
    else glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    if (This->activeContext->num_untracked_materials && sd->u.s.diffuse.dwType != WINED3DDECLTYPE_D3DCOLOR)
        FIXME("Implement diffuse color tracking from %s\n", debug_d3ddecltype(sd->u.s.diffuse.dwType));

    if (sd->u.s.specular.lpData)
    {
        specular = sd->u.s.specular.lpData + streamOffset[sd->u.s.specular.streamNo];

        /* special case where the fog density is stored in the specular alpha channel */
        if (This->stateBlock->renderState[WINED3DRS_FOGENABLE]
                && (This->stateBlock->renderState[WINED3DRS_FOGVERTEXMODE] == WINED3DFOG_NONE
                    || sd->u.s.position.dwType == WINED3DDECLTYPE_FLOAT4)
                && This->stateBlock->renderState[WINED3DRS_FOGTABLEMODE] == WINED3DFOG_NONE)
        {
            if (GL_SUPPORT(EXT_FOG_COORD))
            {
                if (sd->u.s.specular.dwType == WINED3DDECLTYPE_D3DCOLOR) specular_fog = TRUE;
                else FIXME("Implement fog coordinates from %s\n", debug_d3ddecltype(sd->u.s.specular.dwType));
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

        if(sd->u.s.texCoords[coordIdx].lpData)
        {
            texCoords[coordIdx] =
                    sd->u.s.texCoords[coordIdx].lpData + streamOffset[sd->u.s.texCoords[coordIdx].streamNo];
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
            ptr = texCoords[coord_idx] + (SkipnStrides * sd->u.s.texCoords[coord_idx].dwStride);

            texture_idx = This->texUnitMap[texture];
            multi_texcoord_funcs[sd->u.s.texCoords[coord_idx].dwType](GL_TEXTURE0_ARB + texture_idx, ptr);
        }

        /* Diffuse -------------------------------- */
        if (diffuse) {
            const void *ptrToCoords = diffuse + SkipnStrides * sd->u.s.diffuse.dwStride;

            diffuse_funcs[sd->u.s.diffuse.dwType](ptrToCoords);
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
            const void *ptrToCoords = specular + SkipnStrides * sd->u.s.specular.dwStride;

            specular_funcs[sd->u.s.specular.dwType](ptrToCoords);

            if (specular_fog)
            {
                DWORD specularColor = *(const DWORD *)ptrToCoords;
                GL_EXTCALL(glFogCoordfEXT(specularColor >> 24));
            }
        }

        /* Normal -------------------------------- */
        if (normal != NULL) {
            const void *ptrToCoords = normal + SkipnStrides * sd->u.s.normal.dwStride;
            normal_funcs[sd->u.s.normal.dwType](ptrToCoords);
        }

        /* Position -------------------------------- */
        if (position) {
            const void *ptrToCoords = position + SkipnStrides * sd->u.s.position.dwStride;
            position_funcs[sd->u.s.position.dwType](ptrToCoords);
        }

        /* For non indexed mode, step onto next parts */
        if (idxData == NULL) {
            ++SkipnStrides;
        }
    }

    glEnd();
    checkGLcall("glEnd and previous calls");
}

static inline void send_attribute(IWineD3DDeviceImpl *This, const DWORD type, const UINT index, const void *ptr) {
    switch(type) {
        case WINED3DDECLTYPE_FLOAT1:
            GL_EXTCALL(glVertexAttrib1fvARB(index, ptr));
            break;
        case WINED3DDECLTYPE_FLOAT2:
            GL_EXTCALL(glVertexAttrib2fvARB(index, ptr));
            break;
        case WINED3DDECLTYPE_FLOAT3:
            GL_EXTCALL(glVertexAttrib3fvARB(index, ptr));
            break;
        case WINED3DDECLTYPE_FLOAT4:
            GL_EXTCALL(glVertexAttrib4fvARB(index, ptr));
            break;

        case WINED3DDECLTYPE_UBYTE4:
            GL_EXTCALL(glVertexAttrib4ubvARB(index, ptr));
            break;
        case WINED3DDECLTYPE_D3DCOLOR:
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
        case WINED3DDECLTYPE_UBYTE4N:
            GL_EXTCALL(glVertexAttrib4NubvARB(index, ptr));
            break;

        case WINED3DDECLTYPE_SHORT2:
            GL_EXTCALL(glVertexAttrib4svARB(index, ptr));
            break;
        case WINED3DDECLTYPE_SHORT4:
            GL_EXTCALL(glVertexAttrib4svARB(index, ptr));
            break;

        case WINED3DDECLTYPE_SHORT2N:
        {
            GLshort s[4] = {((const GLshort *)ptr)[0], ((const GLshort *)ptr)[1], 0, 1};
            GL_EXTCALL(glVertexAttrib4NsvARB(index, s));
            break;
        }
        case WINED3DDECLTYPE_USHORT2N:
        {
            GLushort s[4] = {((const GLushort *)ptr)[0], ((const GLushort *)ptr)[1], 0, 1};
            GL_EXTCALL(glVertexAttrib4NusvARB(index, s));
            break;
        }
        case WINED3DDECLTYPE_SHORT4N:
            GL_EXTCALL(glVertexAttrib4NsvARB(index, ptr));
            break;
        case WINED3DDECLTYPE_USHORT4N:
            GL_EXTCALL(glVertexAttrib4NusvARB(index, ptr));
            break;

        case WINED3DDECLTYPE_UDEC3:
            FIXME("Unsure about WINED3DDECLTYPE_UDEC3\n");
            /*glVertexAttrib3usvARB(instancedData[j], (GLushort *) ptr); Does not exist */
            break;
        case WINED3DDECLTYPE_DEC3N:
            FIXME("Unsure about WINED3DDECLTYPE_DEC3N\n");
            /*glVertexAttrib3NusvARB(instancedData[j], (GLushort *) ptr); Does not exist */
            break;

        case WINED3DDECLTYPE_FLOAT16_2:
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
        case WINED3DDECLTYPE_FLOAT16_4:
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

        case WINED3DDECLTYPE_UNUSED:
        default:
            ERR("Unexpected attribute declaration: %d\n", type);
            break;
    }
}

static void drawStridedSlowVs(IWineD3DDevice *iface, const WineDirect3DVertexStridedData *sd, UINT numberOfVertices,
        GLenum glPrimitiveType, const void *idxData, short idxSize, ULONG minIndex, ULONG startIdx)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    long                      SkipnStrides = startIdx + This->stateBlock->loadBaseVertexIndex;
    const WORD                *pIdxBufS     = NULL;
    const DWORD               *pIdxBufL     = NULL;
    ULONG                      vx_index;
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
            if(!sd->u.input[i].lpData) continue;

            ptr = sd->u.input[i].lpData +
                  sd->u.input[i].dwStride * SkipnStrides +
                  stateblock->streamOffset[sd->u.input[i].streamNo];

            send_attribute(This, sd->u.input[i].dwType, i, ptr);
        }
        SkipnStrides++;
    }

    glEnd();
}

static inline void drawStridedInstanced(IWineD3DDevice *iface, const WineDirect3DVertexStridedData *sd,
        UINT numberOfVertices, GLenum glPrimitiveType, const void *idxData, short idxSize, ULONG minIndex,
        ULONG startIdx)
{
    UINT numInstances = 0, i;
    int numInstancedAttribs = 0, j;
    UINT instancedData[sizeof(sd->u.input) / sizeof(sd->u.input[0]) /* 16 */];
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

    for(i = 0; i < sizeof(sd->u.input) / sizeof(sd->u.input[0]); i++) {
        if(stateblock->streamFlags[sd->u.input[i].streamNo] & WINED3DSTREAMSOURCE_INSTANCEDATA) {
            instancedData[numInstancedAttribs] = i;
            numInstancedAttribs++;
        }
    }

    /* now draw numInstances instances :-) */
    for(i = 0; i < numInstances; i++) {
        /* Specify the instanced attributes using immediate mode calls */
        for(j = 0; j < numInstancedAttribs; j++) {
            const BYTE *ptr = sd->u.input[instancedData[j]].lpData +
                        sd->u.input[instancedData[j]].dwStride * i +
                        stateblock->streamOffset[sd->u.input[instancedData[j]].streamNo];
            if(sd->u.input[instancedData[j]].VBO) {
                IWineD3DVertexBufferImpl *vb = (IWineD3DVertexBufferImpl *) stateblock->streamSource[sd->u.input[instancedData[j]].streamNo];
                ptr += (long) vb->resource.allocatedMemory;
            }

            send_attribute(This, sd->u.input[instancedData[j]].dwType, instancedData[j], ptr);
        }

        glDrawElements(glPrimitiveType, numberOfVertices, idxSize == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                    (const char *)idxData+(idxSize * startIdx));
        checkGLcall("glDrawElements");
    }
}

static inline void remove_vbos(IWineD3DDeviceImpl *This, WineDirect3DVertexStridedData *s) {
    unsigned char i;
    IWineD3DVertexBufferImpl *vb;

    if(s->u.s.position.VBO) {
        vb = (IWineD3DVertexBufferImpl *) This->stateBlock->streamSource[s->u.s.position.streamNo];
        s->u.s.position.VBO = 0;
        s->u.s.position.lpData = (BYTE *) ((unsigned long) s->u.s.position.lpData + (unsigned long) vb->resource.allocatedMemory);
    }
    if(s->u.s.blendWeights.VBO) {
        vb = (IWineD3DVertexBufferImpl *) This->stateBlock->streamSource[s->u.s.blendWeights.streamNo];
        s->u.s.blendWeights.VBO = 0;
        s->u.s.blendWeights.lpData = (BYTE *) ((unsigned long) s->u.s.blendWeights.lpData + (unsigned long) vb->resource.allocatedMemory);
    }
    if(s->u.s.blendMatrixIndices.VBO) {
        vb = (IWineD3DVertexBufferImpl *) This->stateBlock->streamSource[s->u.s.blendMatrixIndices.streamNo];
        s->u.s.blendMatrixIndices.VBO = 0;
        s->u.s.blendMatrixIndices.lpData = (BYTE *) ((unsigned long) s->u.s.blendMatrixIndices.lpData + (unsigned long) vb->resource.allocatedMemory);
    }
    if(s->u.s.normal.VBO) {
        vb = (IWineD3DVertexBufferImpl *) This->stateBlock->streamSource[s->u.s.normal.streamNo];
        s->u.s.normal.VBO = 0;
        s->u.s.normal.lpData = (BYTE *) ((unsigned long) s->u.s.normal.lpData + (unsigned long) vb->resource.allocatedMemory);
    }
    if(s->u.s.pSize.VBO) {
        vb = (IWineD3DVertexBufferImpl *) This->stateBlock->streamSource[s->u.s.pSize.streamNo];
        s->u.s.pSize.VBO = 0;
        s->u.s.pSize.lpData = (BYTE *) ((unsigned long) s->u.s.pSize.lpData + (unsigned long) vb->resource.allocatedMemory);
    }
    if(s->u.s.diffuse.VBO) {
        vb = (IWineD3DVertexBufferImpl *) This->stateBlock->streamSource[s->u.s.diffuse.streamNo];
        s->u.s.diffuse.VBO = 0;
        s->u.s.diffuse.lpData = (BYTE *) ((unsigned long) s->u.s.diffuse.lpData + (unsigned long) vb->resource.allocatedMemory);
    }
    if(s->u.s.specular.VBO) {
        vb = (IWineD3DVertexBufferImpl *) This->stateBlock->streamSource[s->u.s.specular.streamNo];
        s->u.s.specular.VBO = 0;
        s->u.s.specular.lpData = (BYTE *) ((unsigned long) s->u.s.specular.lpData + (unsigned long) vb->resource.allocatedMemory);
    }
    for(i = 0; i < WINED3DDP_MAXTEXCOORD; i++) {
        if(s->u.s.texCoords[i].VBO) {
            vb = (IWineD3DVertexBufferImpl *) This->stateBlock->streamSource[s->u.s.texCoords[i].streamNo];
            s->u.s.texCoords[i].VBO = 0;
            s->u.s.texCoords[i].lpData = (BYTE *) ((unsigned long) s->u.s.texCoords[i].lpData + (unsigned long) vb->resource.allocatedMemory);
        }
    }
    if(s->u.s.position2.VBO) {
        vb = (IWineD3DVertexBufferImpl *) This->stateBlock->streamSource[s->u.s.position2.streamNo];
        s->u.s.position2.VBO = 0;
        s->u.s.position2.lpData = (BYTE *) ((unsigned long) s->u.s.position2.lpData + (unsigned long) vb->resource.allocatedMemory);
    }
    if(s->u.s.normal2.VBO) {
        vb = (IWineD3DVertexBufferImpl *) This->stateBlock->streamSource[s->u.s.normal2.streamNo];
        s->u.s.normal2.VBO = 0;
        s->u.s.normal2.lpData = (BYTE *) ((unsigned long) s->u.s.normal2.lpData + (unsigned long) vb->resource.allocatedMemory);
    }
    if(s->u.s.tangent.VBO) {
        vb = (IWineD3DVertexBufferImpl *) This->stateBlock->streamSource[s->u.s.tangent.streamNo];
        s->u.s.tangent.VBO = 0;
        s->u.s.tangent.lpData = (BYTE *) ((unsigned long) s->u.s.tangent.lpData + (unsigned long) vb->resource.allocatedMemory);
    }
    if(s->u.s.binormal.VBO) {
        vb = (IWineD3DVertexBufferImpl *) This->stateBlock->streamSource[s->u.s.binormal.streamNo];
        s->u.s.binormal.VBO = 0;
        s->u.s.binormal.lpData = (BYTE *) ((unsigned long) s->u.s.binormal.lpData + (unsigned long) vb->resource.allocatedMemory);
    }
    if(s->u.s.tessFactor.VBO) {
        vb = (IWineD3DVertexBufferImpl *) This->stateBlock->streamSource[s->u.s.tessFactor.streamNo];
        s->u.s.tessFactor.VBO = 0;
        s->u.s.tessFactor.lpData = (BYTE *) ((unsigned long) s->u.s.tessFactor.lpData + (unsigned long) vb->resource.allocatedMemory);
    }
    if(s->u.s.fog.VBO) {
        vb = (IWineD3DVertexBufferImpl *) This->stateBlock->streamSource[s->u.s.fog.streamNo];
        s->u.s.fog.VBO = 0;
        s->u.s.fog.lpData = (BYTE *) ((unsigned long) s->u.s.fog.lpData + (unsigned long) vb->resource.allocatedMemory);
    }
    if(s->u.s.depth.VBO) {
        vb = (IWineD3DVertexBufferImpl *) This->stateBlock->streamSource[s->u.s.depth.streamNo];
        s->u.s.depth.VBO = 0;
        s->u.s.depth.lpData = (BYTE *) ((unsigned long) s->u.s.depth.lpData + (unsigned long) vb->resource.allocatedMemory);
    }
    if(s->u.s.sample.VBO) {
        vb = (IWineD3DVertexBufferImpl *) This->stateBlock->streamSource[s->u.s.sample.streamNo];
        s->u.s.sample.VBO = 0;
        s->u.s.sample.lpData = (BYTE *) ((unsigned long) s->u.s.sample.lpData + (unsigned long) vb->resource.allocatedMemory);
    }
}

/* Routine common to the draw primitive and draw indexed primitive routines */
void drawPrimitive(IWineD3DDevice *iface, int PrimitiveType, long NumPrimitives,
        UINT numberOfVertices, long StartIdx, short idxSize, const void *idxData, int minIndex)
{

    IWineD3DDeviceImpl           *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSurfaceImpl          *target;
    unsigned int i;

    if (NumPrimitives == 0) return;

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
        GLenum glPrimType;
        BOOL emulation = FALSE;
        const WineDirect3DVertexStridedData *strided = &This->strided_streams;
        WineDirect3DVertexStridedData stridedlcl;
        /* Ok, Work out which primitive is requested and how many vertexes that
           will be                                                              */
        UINT calculatedNumberOfindices = primitiveToGl(PrimitiveType, NumPrimitives, &glPrimType);
        if (numberOfVertices == 0 )
            numberOfVertices = calculatedNumberOfindices;

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
                strided = &stridedlcl;
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
                drawStridedSlowVs(iface, strided, calculatedNumberOfindices,
                        glPrimType, idxData, idxSize, minIndex, StartIdx);
            } else {
                drawStridedSlow(iface, strided, calculatedNumberOfindices,
                        glPrimType, idxData, idxSize, minIndex, StartIdx);
            }
        } else if(This->instancedDraw) {
            /* Instancing emulation with mixing immediate mode and arrays */
            drawStridedInstanced(iface, &This->strided_streams, calculatedNumberOfindices, glPrimType,
                            idxData, idxSize, minIndex, StartIdx);
        } else {
            drawStridedFast(iface, glPrimType, minIndex, minIndex + numberOfVertices - 1,
                    calculatedNumberOfindices, idxSize, idxData, StartIdx);
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
    WineDirect3DVertexStridedData strided;
    const BYTE *data;
    const WINED3DRECTPATCH_INFO *info = &patch->RectPatchInfo;
    DWORD vtxStride;
    GLenum feedback_type;
    GLfloat *feedbuffer;

    /* First, locate the position data. This is provided in a vertex buffer in the stateblock.
     * Beware of vbos
     */
    memset(&strided, 0, sizeof(strided));
    primitiveDeclarationConvertToStridedData((IWineD3DDevice *) This, FALSE, &strided, NULL);
    if(strided.u.s.position.VBO) {
        IWineD3DVertexBufferImpl *vb;
        vb = (IWineD3DVertexBufferImpl *) This->stateBlock->streamSource[strided.u.s.position.streamNo];
        strided.u.s.position.lpData = (BYTE *) ((unsigned long) strided.u.s.position.lpData +
                                                (unsigned long) vb->resource.allocatedMemory);
    }
    vtxStride = strided.u.s.position.dwStride;
    data = strided.u.s.position.lpData +
           vtxStride * info->Stride * info->StartVertexOffsetHeight +
           vtxStride * info->StartVertexOffsetWidth;

    /* Not entirely sure about what happens with transformed vertices */
    if (strided.position_transformed) FIXME("Transformed position in rectpatch generation\n");

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
    patch->strided.u.s.position.lpData = (BYTE *) patch->mem;
    patch->strided.u.s.position.dwStride = vtxStride;
    patch->strided.u.s.position.dwType = WINED3DDECLTYPE_FLOAT3;
    patch->strided.u.s.position.streamNo = 255;

    if(patch->has_normals) {
        patch->strided.u.s.normal.lpData = (BYTE *) patch->mem + 3 * sizeof(float) /* pos */;
        patch->strided.u.s.normal.dwStride = vtxStride;
        patch->strided.u.s.normal.dwType = WINED3DDECLTYPE_FLOAT3;
        patch->strided.u.s.normal.streamNo = 255;
    }
    if(patch->has_texcoords) {
        patch->strided.u.s.texCoords[0].lpData = (BYTE *) patch->mem + 3 * sizeof(float) /* pos */;
        if(patch->has_normals) {
            patch->strided.u.s.texCoords[0].lpData += 3 * sizeof(float);
        }
        patch->strided.u.s.texCoords[0].dwStride = vtxStride;
        patch->strided.u.s.texCoords[0].dwType = WINED3DDECLTYPE_FLOAT4;
        /* MAX_STREAMS index points to an unused element in stateblock->streamOffsets which
         * always remains set to 0. Windows uses stream 255 here, but this is not visible to the
         * application.
         */
        patch->strided.u.s.texCoords[0].streamNo = MAX_STREAMS;
    }

    return WINED3D_OK;
}
