/*
 * WINED3D draw functions
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2002-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Henri Verbeet
 * Copyright 2007 Stefan Dösinger for CodeWeavers
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
#define GLINFO_LOCATION ((IWineD3DImpl *)(This->wineD3D))->gl_info

#include <stdio.h>

#if 0 /* TODO */
extern IWineD3DVertexShaderImpl*            VertexShaders[64];
extern IWineD3DVertexDeclarationImpl*       VertexShaderDeclarations[64];
extern IWineD3DPixelShaderImpl*             PixelShaders[64];

#undef GL_VERSION_1_4 /* To be fixed, caused by mesa headers */
#endif

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

    if (*regnum < 0) {
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

    BYTE  *data    = NULL;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DVertexDeclarationImpl* vertexDeclaration = (IWineD3DVertexDeclarationImpl *)This->stateBlock->vertexDecl;
    int i;
    WINED3DVERTEXELEMENT *element;
    DWORD stride;
    int reg;
    char isPreLoaded[MAX_STREAMS];
    DWORD preLoadStreams[MAX_STREAMS], numPreloadStreams = 0;

    memset(isPreLoaded, 0, sizeof(isPreLoaded));

    /* Check for transformed vertices, disable vertex shader if present */
    strided->u.s.position_transformed = FALSE;
    for (i = 0; i < vertexDeclaration->declarationWNumElements - 1; ++i) {
        element = vertexDeclaration->pDeclarationWine + i;

        if (element->Usage == WINED3DDECLUSAGE_POSITIONT) {
            strided->u.s.position_transformed = TRUE;
            useVertexShaderFunction = FALSE;
        }
    }

    /* Translate the declaration into strided data */
    for (i = 0 ; i < vertexDeclaration->declarationWNumElements - 1; ++i) {
        GLint streamVBO = 0;
        BOOL stride_used;
        unsigned int idx;

        element = vertexDeclaration->pDeclarationWine + i;
        TRACE("%p Element %p (%d of %d)\n", vertexDeclaration->pDeclarationWine,
            element,  i + 1, vertexDeclaration->declarationWNumElements - 1);

        if (This->stateBlock->streamSource[element->Stream] == NULL)
            continue;

        if (This->stateBlock->streamIsUP) {
            TRACE("Stream is up %d, %p\n", element->Stream, This->stateBlock->streamSource[element->Stream]);
            streamVBO = 0;
            data    = (BYTE *)This->stateBlock->streamSource[element->Stream];
        } else {
            TRACE("Stream isn't up %d, %p\n", element->Stream, This->stateBlock->streamSource[element->Stream]);
            if(!isPreLoaded[element->Stream]) {
                preLoadStreams[numPreloadStreams] = element->Stream;
                numPreloadStreams++;
                isPreLoaded[element->Stream] = 1;
            }
            data    = IWineD3DVertexBufferImpl_GetMemory(This->stateBlock->streamSource[element->Stream], 0, &streamVBO);
            if(fixup) {
                if( streamVBO != 0) *fixup = TRUE;
                else if(*fixup && !useVertexShaderFunction) {
                    /* This may be bad with the fixed function pipeline */
                    FIXME("Missing fixed and unfixed vertices, expect graphics glitches\n");
                }
            }
        }
        stride  = This->stateBlock->streamStride[element->Stream];
        data += element->Offset;
        reg = element->Reg;

        TRACE("Offset %d Stream %d UsageIndex %d\n", element->Offset, element->Stream, element->UsageIndex);

        if (useVertexShaderFunction)
            stride_used = vshader_get_input(This->stateBlock->vertexShader,
                element->Usage, element->UsageIndex, &idx);
        else
            stride_used = fixed_get_input(element->Usage, element->UsageIndex, &idx);

        if (stride_used) {
            TRACE("Loaded %s array %u [usage=%s, usage_idx=%u, "
                    "stream=%u, offset=%u, stride=%u, VBO=%u]\n",
                    useVertexShaderFunction? "shader": "fixed function", idx,
                    debug_d3ddeclusage(element->Usage), element->UsageIndex,
                    element->Stream, element->Offset, stride, streamVBO);

            strided->u.input[idx].lpData = data;
            strided->u.input[idx].dwType = element->Type;
            strided->u.input[idx].dwStride = stride;
            strided->u.input[idx].VBO = streamVBO;
            strided->u.input[idx].streamNo = element->Stream;
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
        IWineD3DVertexBuffer_PreLoad(This->stateBlock->streamSource[preLoadStreams[i]]);
    }
}

void primitiveConvertFVFtoOffset(DWORD thisFVF, DWORD stride, BYTE *data, WineDirect3DVertexStridedData *strided, GLint streamVBO, UINT streamNo) {
    int           numBlends;
    int           numTextures;
    int           textureNo;
    int           coordIdxInfo = 0x00;    /* Information on number of coords supplied */
    int           numCoords[8];           /* Holding place for WINED3DFVF_TEXTUREFORMATx  */

    /* Either 3 or 4 floats depending on the FVF */
    /* FIXME: Can blending data be in a different stream to the position data?
          and if so using the fixed pipeline how do we handle it               */
    if (thisFVF & WINED3DFVF_POSITION_MASK) {
        strided->u.s.position.lpData    = data;
        strided->u.s.position.dwType    = WINED3DDECLTYPE_FLOAT3;
        strided->u.s.position.dwStride  = stride;
        strided->u.s.position.VBO       = streamVBO;
        strided->u.s.position.streamNo  = streamNo;
        data += 3 * sizeof(float);
        if (thisFVF & WINED3DFVF_XYZRHW) {
            strided->u.s.position.dwType = WINED3DDECLTYPE_FLOAT4;
            strided->u.s.position_transformed = TRUE;
            data += sizeof(float);
        } else
            strided->u.s.position_transformed = FALSE;
    }

    /* Blending is numBlends * FLOATs followed by a DWORD for UBYTE4 */
    /** do we have to Check This->stateBlock->renderState[D3DRS_INDEXEDVERTEXBLENDENABLE] ? */
    numBlends = 1 + (((thisFVF & WINED3DFVF_XYZB5) - WINED3DFVF_XYZB1) >> 1);
    if(thisFVF & WINED3DFVF_LASTBETA_UBYTE4) numBlends--;

    if ((thisFVF & WINED3DFVF_XYZB5 ) > WINED3DFVF_XYZRHW) {
        TRACE("Setting blend Weights to %p\n", data);
        strided->u.s.blendWeights.lpData    = data;
        strided->u.s.blendWeights.dwType    = WINED3DDECLTYPE_FLOAT1 + numBlends - 1;
        strided->u.s.blendWeights.dwStride  = stride;
        strided->u.s.blendWeights.VBO       = streamVBO;
        strided->u.s.blendWeights.streamNo  = streamNo;
        data += numBlends * sizeof(FLOAT);

        if (thisFVF & WINED3DFVF_LASTBETA_UBYTE4) {
            strided->u.s.blendMatrixIndices.lpData = data;
            strided->u.s.blendMatrixIndices.dwType  = WINED3DDECLTYPE_UBYTE4;
            strided->u.s.blendMatrixIndices.dwStride= stride;
            strided->u.s.blendMatrixIndices.VBO     = streamVBO;
            strided->u.s.blendMatrixIndices.streamNo= streamNo;
            data += sizeof(DWORD);
        }
    }

    /* Normal is always 3 floats */
    if (thisFVF & WINED3DFVF_NORMAL) {
        strided->u.s.normal.lpData    = data;
        strided->u.s.normal.dwType    = WINED3DDECLTYPE_FLOAT3;
        strided->u.s.normal.dwStride  = stride;
        strided->u.s.normal.VBO       = streamVBO;
        strided->u.s.normal.streamNo  = streamNo;
        data += 3 * sizeof(FLOAT);
    }

    /* Pointsize is a single float */
    if (thisFVF & WINED3DFVF_PSIZE) {
        strided->u.s.pSize.lpData    = data;
        strided->u.s.pSize.dwType    = WINED3DDECLTYPE_FLOAT1;
        strided->u.s.pSize.dwStride  = stride;
        strided->u.s.pSize.VBO       = streamVBO;
        strided->u.s.pSize.streamNo  = streamNo;
        data += sizeof(FLOAT);
    }

    /* Diffuse is 4 unsigned bytes */
    if (thisFVF & WINED3DFVF_DIFFUSE) {
        strided->u.s.diffuse.lpData    = data;
        strided->u.s.diffuse.dwType    = WINED3DDECLTYPE_SHORT4;
        strided->u.s.diffuse.dwStride  = stride;
        strided->u.s.diffuse.VBO       = streamVBO;
        strided->u.s.diffuse.streamNo  = streamNo;
        data += sizeof(DWORD);
    }

    /* Specular is 4 unsigned bytes */
    if (thisFVF & WINED3DFVF_SPECULAR) {
        strided->u.s.specular.lpData    = data;
        strided->u.s.specular.dwType    = WINED3DDECLTYPE_SHORT4;
        strided->u.s.specular.dwStride  = stride;
        strided->u.s.specular.VBO       = streamVBO;
        strided->u.s.specular.streamNo  = streamNo;
        data += sizeof(DWORD);
    }

    /* Texture coords */
    numTextures   = (thisFVF & WINED3DFVF_TEXCOUNT_MASK) >> WINED3DFVF_TEXCOUNT_SHIFT;
    coordIdxInfo  = (thisFVF & 0x00FF0000) >> 16; /* 16 is from definition of WINED3DFVF_TEXCOORDSIZE1, and is 8 (0-7 stages) * 2bits long */

    /* numTextures indicates the number of texture coordinates supplied */
    /* However, the first set may not be for stage 0 texture - it all   */
    /*   depends on WINED3DTSS_TEXCOORDINDEX.                           */
    /* The number of bytes for each coordinate set is based off         */
    /*   WINED3DFVF_TEXCOORDSIZEn, which are the bottom 2 bits              */

    /* So, for each supplied texture extract the coords */
    for (textureNo = 0; textureNo < numTextures; ++textureNo) {

        strided->u.s.texCoords[textureNo].lpData    = data;
        strided->u.s.texCoords[textureNo].dwType    = WINED3DDECLTYPE_FLOAT1;
        strided->u.s.texCoords[textureNo].dwStride  = stride;
        strided->u.s.texCoords[textureNo].VBO       = streamVBO;
        strided->u.s.texCoords[textureNo].streamNo  = streamNo;
        numCoords[textureNo] = coordIdxInfo & 0x03;

        /* Always one set */
        data += sizeof(float);
        if (numCoords[textureNo] != WINED3DFVF_TEXTUREFORMAT1) {
            strided->u.s.texCoords[textureNo].dwType = WINED3DDECLTYPE_FLOAT2;
            data += sizeof(float);
            if (numCoords[textureNo] != WINED3DFVF_TEXTUREFORMAT2) {
                strided->u.s.texCoords[textureNo].dwType = WINED3DDECLTYPE_FLOAT3;
                data += sizeof(float);
                if (numCoords[textureNo] != WINED3DFVF_TEXTUREFORMAT3) {
                    strided->u.s.texCoords[textureNo].dwType = WINED3DDECLTYPE_FLOAT4;
                    data += sizeof(float);
                }
            }
        }
        coordIdxInfo = coordIdxInfo >> 2; /* Drop bottom two bits */
    }
}

void primitiveConvertToStridedData(IWineD3DDevice *iface, WineDirect3DVertexStridedData *strided, BOOL *fixup) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    GLint         streamVBO = 0;
    DWORD  stride  = This->stateBlock->streamStride[0];
    BYTE  *data    = NULL;
    DWORD  thisFVF = 0;

    /* Retrieve appropriate FVF */
    thisFVF = This->stateBlock->fvf;
    /* Handle memory passed directly as well as vertex buffers */
    if (This->stateBlock->streamIsUP) {
        streamVBO = 0;
        data    = (BYTE *)This->stateBlock->streamSource[0];
    } else {
        /* The for loop should iterate through here only once per stream, so we don't need magic to prevent double loading
         * buffers
         */
        data = IWineD3DVertexBufferImpl_GetMemory(This->stateBlock->streamSource[0], 0, &streamVBO);
        if(fixup) {
            if(streamVBO != 0 ) *fixup = TRUE;
        }
    }
    VTRACE(("FVF for stream 0 is %lx\n", thisFVF));

    /* Now convert the stream into pointers */
    primitiveConvertFVFtoOffset(thisFVF, stride, data, strided, streamVBO, 0);

    /* Now call PreLoad on the vertex buffer. In the very rare case
     * that the buffers stopps converting PreLoad will dirtify the VDECL again.
     * The vertex buffer can now use the strided structure in the device instead of finding its
     * own again.
     */
    if(!This->stateBlock->streamIsUP) {
        IWineD3DVertexBuffer_PreLoad(This->stateBlock->streamSource[0]);
    }
}

static void drawStridedFast(IWineD3DDevice *iface,UINT numberOfVertices, GLenum glPrimitiveType,
                     const void *idxData, short idxSize, ULONG minIndex, ULONG startIdx, ULONG startVertex) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    if (idxSize != 0 /* This crashes sometimes!*/) {
        TRACE("(%p) : glElements(%x, %d, %d, ...)\n", This, glPrimitiveType, numberOfVertices, minIndex);
        idxData = idxData == (void *)-1 ? NULL : idxData;
#if 1
        glDrawElements(glPrimitiveType, numberOfVertices, idxSize == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                     (const char *)idxData+(idxSize * startIdx));
#else /* using drawRangeElements may be faster */

        glDrawRangeElements(glPrimitiveType, minIndex, minIndex + numberOfVertices - 1, numberOfVertices,
                      idxSize == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                      (const char *)idxData+(idxSize * startIdx));
#endif
        checkGLcall("glDrawRangeElements");

    } else {

        /* Note first is now zero as we shuffled along earlier */
        TRACE("(%p) : glDrawArrays(%x, 0, %d)\n", This, glPrimitiveType, numberOfVertices);
        glDrawArrays(glPrimitiveType, startVertex, numberOfVertices);
        checkGLcall("glDrawArrays");

    }

    return;
}

/*
 * Actually draw using the supplied information.
 * Slower GL version which extracts info about each vertex in turn
 */

static void drawStridedSlow(IWineD3DDevice *iface, WineDirect3DVertexStridedData *sd,
                     UINT NumVertexes, GLenum glPrimType,
                     const void *idxData, short idxSize, ULONG minIndex, ULONG startIdx, ULONG startVertex) {

    unsigned int               textureNo    = 0;
    unsigned int               texture_idx  = 0;
    const WORD                *pIdxBufS     = NULL;
    const DWORD               *pIdxBufL     = NULL;
    LONG                       vx_index;
    float x  = 0.0f, y  = 0.0f, z = 0.0f;  /* x,y,z coordinates          */
    float rhw = 0.0f;                      /* rhw                        */
    DWORD diffuseColor = 0xFFFFFFFF;       /* Diffuse Color              */
    DWORD specularColor = 0;               /* Specular Color             */
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    UINT *streamOffset = This->stateBlock->streamOffset;
    DWORD                      SkipnStrides = startVertex + This->stateBlock->loadBaseVertexIndex;

    BYTE *texCoords[WINED3DDP_MAXTEXCOORD];
    BYTE *diffuse = NULL, *specular = NULL, *normal = NULL, *position = NULL;

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

        if (idxSize == 2) pIdxBufS = (const WORD *) idxData;
        else pIdxBufL = (const DWORD *) idxData;
    }

    /* Adding the stream offset once is cheaper than doing it every iteration. Do not modify the strided data, it is a pointer
     * to the strided Data in the device and might be needed intact on the next draw
     */
    for (textureNo = 0; textureNo < GL_LIMITS(texture_stages); ++textureNo) {
        if(sd->u.s.texCoords[textureNo].lpData) {
            texCoords[textureNo] = sd->u.s.texCoords[textureNo].lpData + streamOffset[sd->u.s.texCoords[textureNo].streamNo];
        } else {
            texCoords[textureNo] = NULL;
        }
    }
    if(sd->u.s.diffuse.lpData) {
        diffuse = sd->u.s.diffuse.lpData + streamOffset[sd->u.s.diffuse.streamNo];
    }
    if(sd->u.s.specular.lpData) {
        specular = sd->u.s.specular.lpData + streamOffset[sd->u.s.specular.streamNo];
    }
    if(sd->u.s.normal.lpData) {
        normal = sd->u.s.normal.lpData + streamOffset[sd->u.s.normal.streamNo];
    }
    if(sd->u.s.position.lpData) {
        position = sd->u.s.position.lpData + streamOffset[sd->u.s.position.streamNo];
    }

    /* Start drawing in GL */
    VTRACE(("glBegin(%x)\n", glPrimType));
    glBegin(glPrimType);

    /* Default settings for data that is not passed */
    if (sd->u.s.normal.lpData == NULL) {
        glNormal3f(0, 0, 1);
    }
    if(sd->u.s.diffuse.lpData == NULL) {
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }
    if(sd->u.s.specular.lpData == NULL) {
        if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
            GL_EXTCALL(glSecondaryColor3fEXT)(0, 0, 0);
        }
    }

    /* We shouldn't start this function if any VBO is involved. Should I put a safety check here?
     * Guess it's not necessary(we crash then anyway) and would only eat CPU time
     */

    /* For each primitive */
    for (vx_index = 0; vx_index < NumVertexes; ++vx_index) {

        /* Initialize diffuse color */
        diffuseColor = 0xFFFFFFFF;

        /* Blending data and Point sizes are not supported by this function. They are not supported by the fixed
         * function pipeline at all. A Fixme for them is printed after decoding the vertex declaration
         */

        /* For indexed data, we need to go a few more strides in */
        if (idxData != NULL) {

            /* Indexed so work out the number of strides to skip */
            if (idxSize == 2) {
                VTRACE(("Idx for vertex %d = %d\n", vx_index, pIdxBufS[startIdx+vx_index]));
                SkipnStrides = pIdxBufS[startIdx + vx_index] + This->stateBlock->loadBaseVertexIndex;
            } else {
                VTRACE(("Idx for vertex %d = %d\n", vx_index, pIdxBufL[startIdx+vx_index]));
                SkipnStrides = pIdxBufL[startIdx + vx_index] + This->stateBlock->loadBaseVertexIndex;
            }
        }

        /* Texture coords --------------------------- */
        for (textureNo = 0, texture_idx = GL_TEXTURE0_ARB; textureNo < GL_LIMITS(texture_stages); ++textureNo) {

            if (!GL_SUPPORT(ARB_MULTITEXTURE) && textureNo > 0) {
                FIXME("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
                continue ;
            }

            /* Query tex coords */
            if (This->stateBlock->textures[textureNo] != NULL) {

                int    coordIdx = This->stateBlock->textureState[textureNo][WINED3DTSS_TEXCOORDINDEX];
                float *ptrToCoords = NULL;
                float  s = 0.0, t = 0.0, r = 0.0, q = 0.0;

                if (coordIdx > 7) {
                    VTRACE(("tex: %d - Skip tex coords, as being system generated\n", textureNo));
                    ++texture_idx;
                    continue;
                } else if (coordIdx < 0) {
                    FIXME("tex: %d - Coord index %d is less than zero, expect a crash.\n", textureNo, coordIdx);
                    ++texture_idx;
                    continue;
                }

                ptrToCoords = (float *)(texCoords[coordIdx] + (SkipnStrides * sd->u.s.texCoords[coordIdx].dwStride));
                if (texCoords[coordIdx] == NULL) {
                    TRACE("tex: %d - Skipping tex coords, as no data supplied\n", textureNo);
                    ++texture_idx;
                    continue;
                } else {

                    int coordsToUse = sd->u.s.texCoords[coordIdx].dwType + 1; /* 0 == WINED3DDECLTYPE_FLOAT1 etc */

                    /* The coords to supply depend completely on the fvf / vertex shader */
                    switch (coordsToUse) {
                    case 4: q = ptrToCoords[3]; /* drop through */
                    case 3: r = ptrToCoords[2]; /* drop through */
                    case 2: t = ptrToCoords[1]; /* drop through */
                    case 1: s = ptrToCoords[0];
                    }

                    /* Projected is more 'fun' - Move the last coord to the 'q'
                          parameter (see comments under WINED3DTSS_TEXTURETRANSFORMFLAGS */
                    if ((This->stateBlock->textureState[textureNo][WINED3DTSS_TEXTURETRANSFORMFLAGS] != WINED3DTTFF_DISABLE) &&
                        (This->stateBlock->textureState[textureNo][WINED3DTSS_TEXTURETRANSFORMFLAGS] & WINED3DTTFF_PROJECTED)) {

                        if (This->stateBlock->textureState[textureNo][WINED3DTSS_TEXTURETRANSFORMFLAGS] & WINED3DTTFF_PROJECTED) {
                            switch (coordsToUse) {
                            case 0:  /* Drop Through */
                            case 1:
                                FIXME("WINED3DTTFF_PROJECTED but only zero or one coordinate?\n");
                                break;
                            case 2:
                                q = t;
                                t = 0.0;
                                coordsToUse = 4;
                                break;
                            case 3:
                                q = r;
                                r = 0.0;
                                coordsToUse = 4;
                                break;
                            case 4:  /* Nop here */
                                break;
                            default:
                                FIXME("Unexpected WINED3DTSS_TEXTURETRANSFORMFLAGS value of %d\n",
                                      This->stateBlock->textureState[textureNo][WINED3DTSS_TEXTURETRANSFORMFLAGS] & WINED3DTTFF_PROJECTED);
                            }
                        }
                    }

                    switch (coordsToUse) {   /* Supply the provided texture coords */
                    case WINED3DTTFF_COUNT1:
                        VTRACE(("tex:%d, s=%f\n", textureNo, s));
                        if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                            GL_EXTCALL(glMultiTexCoord1fARB(texture_idx, s));
                        } else {
                            glTexCoord1f(s);
                        }
                        break;
                    case WINED3DTTFF_COUNT2:
                        VTRACE(("tex:%d, s=%f, t=%f\n", textureNo, s, t));
                        if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                            GL_EXTCALL(glMultiTexCoord2fARB(texture_idx, s, t));
                        } else {
                            glTexCoord2f(s, t);
                        }
                        break;
                    case WINED3DTTFF_COUNT3:
                        VTRACE(("tex:%d, s=%f, t=%f, r=%f\n", textureNo, s, t, r));
                        if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                            GL_EXTCALL(glMultiTexCoord3fARB(texture_idx, s, t, r));
                        } else {
                            glTexCoord3f(s, t, r);
                        }
                        break;
                    case WINED3DTTFF_COUNT4:
                        VTRACE(("tex:%d, s=%f, t=%f, r=%f, q=%f\n", textureNo, s, t, r, q));
                        if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                            GL_EXTCALL(glMultiTexCoord4fARB(texture_idx, s, t, r, q));
                        } else {
                            glTexCoord4f(s, t, r, q);
                        }
                        break;
                    default:
                        FIXME("Should not get here as coordsToUse is two bits only (%x)!\n", coordsToUse);
                    }
                }
            }
            if (/*!GL_SUPPORT(NV_REGISTER_COMBINERS) || This->stateBlock->textures[textureNo]*/TRUE) ++texture_idx;
        } /* End of textures */

        /* Diffuse -------------------------------- */
        if (diffuse) {
            DWORD *ptrToCoords = (DWORD *)(diffuse + (SkipnStrides * sd->u.s.diffuse.dwStride));
            diffuseColor = ptrToCoords[0];
            VTRACE(("diffuseColor=%lx\n", diffuseColor));

            glColor4ub(D3DCOLOR_B_R(diffuseColor),
		     D3DCOLOR_B_G(diffuseColor),
		     D3DCOLOR_B_B(diffuseColor),
		     D3DCOLOR_B_A(diffuseColor));
            VTRACE(("glColor4ub: r,g,b,a=%lu,%lu,%lu,%lu\n", 
                    D3DCOLOR_B_R(diffuseColor),
		    D3DCOLOR_B_G(diffuseColor),
		    D3DCOLOR_B_B(diffuseColor),
		    D3DCOLOR_B_A(diffuseColor)));
        }

        /* Specular ------------------------------- */
        if (specular) {
            DWORD *ptrToCoords = (DWORD *)(specular + (SkipnStrides * sd->u.s.specular.dwStride));
            specularColor = ptrToCoords[0];
            VTRACE(("specularColor=%lx\n", specularColor));

            /* special case where the fog density is stored in the specular alpha channel */
            if(This->stateBlock->renderState[WINED3DRS_FOGENABLE] &&
              (This->stateBlock->renderState[WINED3DRS_FOGVERTEXMODE] == WINED3DFOG_NONE || sd->u.s.position.dwType == WINED3DDECLTYPE_FLOAT4 )&&
              This->stateBlock->renderState[WINED3DRS_FOGTABLEMODE] == WINED3DFOG_NONE) {
                if(GL_SUPPORT(EXT_FOG_COORD)) {
                    GL_EXTCALL(glFogCoordfEXT(specularColor >> 24));
                } else {
                    static BOOL warned = FALSE;
                    if(!warned) {
                        /* TODO: Use the fog table code from old ddraw */
                        FIXME("Implement fog for transformed vertices in software\n");
                        warned = TRUE;
                    }
                }
            }

            VTRACE(("glSecondaryColor4ub: r,g,b=%lu,%lu,%lu\n", 
                    D3DCOLOR_B_R(specularColor), 
                    D3DCOLOR_B_G(specularColor), 
                    D3DCOLOR_B_B(specularColor)));
            if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
                GL_EXTCALL(glSecondaryColor3ubEXT)(
                           D3DCOLOR_B_R(specularColor),
                           D3DCOLOR_B_G(specularColor),
                           D3DCOLOR_B_B(specularColor));
            } else {
                /* Do not worry if specular colour missing and disable request */
                VTRACE(("Specular color extensions not supplied\n"));
            }
        }

        /* Normal -------------------------------- */
        if (normal != NULL) {
            float *ptrToCoords = (float *)(normal + (SkipnStrides * sd->u.s.normal.dwStride));

            VTRACE(("glNormal:nx,ny,nz=%f,%f,%f\n", ptrToCoords[0], ptrToCoords[1], ptrToCoords[2]));
            glNormal3f(ptrToCoords[0], ptrToCoords[1], ptrToCoords[2]);
        }

        /* Position -------------------------------- */
        if (position) {
            float *ptrToCoords = (float *)(position + (SkipnStrides * sd->u.s.position.dwStride));
            x = ptrToCoords[0];
            y = ptrToCoords[1];
            z = ptrToCoords[2];
            rhw = 1.0;
            VTRACE(("x,y,z=%f,%f,%f\n", x,y,z));

            /* RHW follows, only if transformed, ie 4 floats were provided */
            if (sd->u.s.position_transformed) {
                rhw = ptrToCoords[3];
                VTRACE(("rhw=%f\n", rhw));
            }

            if (1.0f == rhw || ((rhw < eps) && (rhw > -eps))) {
                VTRACE(("Vertex: glVertex:x,y,z=%f,%f,%f\n", x,y,z));
                glVertex3f(x, y, z);
            } else {
                GLfloat w = 1.0f / rhw;
                VTRACE(("Vertex: glVertex:x,y,z=%f,%f,%f / rhw=%f\n", x,y,z,rhw));
                glVertex4f(x*w, y*w, z*w, w);
            }
        }

        /* For non indexed mode, step onto next parts */
        if (idxData == NULL) {
            ++SkipnStrides;
        }
    }

    glEnd();
    checkGLcall("glEnd and previous calls");
}

static void depth_blt(IWineD3DDevice *iface, GLuint texture) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    GLint old_binding = 0;

    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

    GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB));
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &old_binding);
    glBindTexture(GL_TEXTURE_2D, texture);
    glEnable(GL_TEXTURE_2D);

    This->shader_backend->shader_select_depth_blt(iface);

    glBegin(GL_TRIANGLE_STRIP);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(1.0f, -1.0f);
    glVertex2f(-1.0f, 1.0f);
    glVertex2f(1.0f, 1.0f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, old_binding);

    glPopAttrib();

    /* Reselect the old shaders. There doesn't seem to be any glPushAttrib bit for arb shaders,
     * and this seems easier and more efficient than providing the shader backend with a private
     * storage to read and restore the old shader settings
     */
    This->shader_backend->shader_select(iface, use_ps(This), use_vs(This));
}

static void depth_copy(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSurfaceImpl *depth_stencil = (IWineD3DSurfaceImpl *)This->depthStencilBuffer;

    /* Only copy the depth buffer if there is one. */
    if (!depth_stencil) return;

    /* TODO: Make this work for modes other than FBO */
    if (wined3d_settings.offscreen_rendering_mode != ORM_FBO) return;

    if (depth_stencil->current_renderbuffer) {
        FIXME("Not supported with fixed up depth stencil\n");
        return;
    }

    if (This->render_offscreen) {
        static GLuint tmp_texture = 0;
        GLint old_binding = 0;

        TRACE("Copying onscreen depth buffer to offscreen surface\n");

        if (!tmp_texture) {
            glGenTextures(1, &tmp_texture);
        }

        /* Note that we use depth_blt here as well, rather than glCopyTexImage2D
         * directly on the FBO texture. That's because we need to flip. */
        GL_EXTCALL(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0));
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &old_binding);
        glBindTexture(GL_TEXTURE_2D, tmp_texture);
        glCopyTexImage2D(depth_stencil->glDescription.target,
                depth_stencil->glDescription.level,
                depth_stencil->glDescription.glFormatInternal,
                0,
                0,
                depth_stencil->currentDesc.Width,
                depth_stencil->currentDesc.Height,
                0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
        glBindTexture(GL_TEXTURE_2D, old_binding);

        GL_EXTCALL(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, This->fbo));
        checkGLcall("glBindFramebuffer()");
        depth_blt(iface, tmp_texture);
        checkGLcall("depth_blt");
    } else {
        TRACE("Copying offscreen surface to onscreen depth buffer\n");

        GL_EXTCALL(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0));
        checkGLcall("glBindFramebuffer()");
        depth_blt(iface, depth_stencil->glDescription.textureName);
        checkGLcall("depth_blt");
    }
}

static inline void drawStridedInstanced(IWineD3DDevice *iface, WineDirect3DVertexStridedData *sd, UINT numberOfVertices,
                                 GLenum glPrimitiveType, const void *idxData, short idxSize, ULONG minIndex,
                                 ULONG startIdx, ULONG startVertex) {
    UINT numInstances = 0;
    int numInstancedAttribs = 0, i, j;
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
    idxData = idxData == (void *)-1 ? NULL : idxData;

    /* First, figure out how many instances we have to draw */
    for(i = 0; i < MAX_STREAMS; i++) {
        /* Look at all non-instanced streams */
        if(!(stateblock->streamFlags[i] & WINED3DSTREAMSOURCE_INSTANCEDATA) &&
           stateblock->streamSource[i]) {
            int inst = stateblock->streamFreq[i];

            if(numInstances && inst != numInstances) {
                ERR("Two streams specify a different number of instances. Got %d, new is %d\n", numInstances, inst);
            }
            numInstances = inst;
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
            BYTE *ptr = sd->u.input[instancedData[j]].lpData +
                        sd->u.input[instancedData[j]].dwStride * i +
                        stateblock->streamOffset[sd->u.input[instancedData[j]].streamNo];
            if(sd->u.input[instancedData[j]].VBO) {
                IWineD3DVertexBufferImpl *vb = (IWineD3DVertexBufferImpl *) stateblock->streamSource[sd->u.input[instancedData[j]].streamNo];
                ptr += (long) vb->resource.allocatedMemory;
            }

            switch(sd->u.input[instancedData[j]].dwType) {
                case WINED3DDECLTYPE_FLOAT1:
                    GL_EXTCALL(glVertexAttrib1fvARB(instancedData[j], (float *) ptr));
                    break;
                case WINED3DDECLTYPE_FLOAT2:
                    GL_EXTCALL(glVertexAttrib2fvARB(instancedData[j], (float *) ptr));
                    break;
                case WINED3DDECLTYPE_FLOAT3:
                    GL_EXTCALL(glVertexAttrib3fvARB(instancedData[j], (float *) ptr));
                    break;
                case WINED3DDECLTYPE_FLOAT4:
                    GL_EXTCALL(glVertexAttrib4fvARB(instancedData[j], (float *) ptr));
                    break;

                case WINED3DDECLTYPE_UBYTE4:
                    GL_EXTCALL(glVertexAttrib4NubvARB(instancedData[j], ptr));
                    break;
                case WINED3DDECLTYPE_UBYTE4N:
                case WINED3DDECLTYPE_D3DCOLOR:
                    GL_EXTCALL(glVertexAttrib4NubvARB(instancedData[j], ptr));
                    break;

                case WINED3DDECLTYPE_SHORT2:
                    GL_EXTCALL(glVertexAttrib4svARB(instancedData[j], (GLshort *) ptr));
                    break;
                case WINED3DDECLTYPE_SHORT4:
                    GL_EXTCALL(glVertexAttrib4svARB(instancedData[j], (GLshort *) ptr));
                    break;

                case WINED3DDECLTYPE_SHORT2N:
                {
                    GLshort s[4] = {((short *) ptr)[0], ((short *) ptr)[1], 0, 1};
                    GL_EXTCALL(glVertexAttrib4NsvARB(instancedData[j], s));
                    break;
                }
                case WINED3DDECLTYPE_USHORT2N:
                {
                    GLushort s[4] = {((unsigned short *) ptr)[0], ((unsigned short *) ptr)[1], 0, 1};
                    GL_EXTCALL(glVertexAttrib4NusvARB(instancedData[j], s));
                    break;
                }
                case WINED3DDECLTYPE_SHORT4N:
                    GL_EXTCALL(glVertexAttrib4NsvARB(instancedData[j], (GLshort *) ptr));
                    break;
                case WINED3DDECLTYPE_USHORT4N:
                    GL_EXTCALL(glVertexAttrib4NusvARB(instancedData[j], (GLushort *) ptr));
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
                    FIXME("Unsupported WINED3DDECLTYPE_FLOAT16_2\n");
                    break;
                case WINED3DDECLTYPE_FLOAT16_4:
                    FIXME("Unsupported WINED3DDECLTYPE_FLOAT16_4\n");
                    break;

                case WINED3DDECLTYPE_UNUSED:
                default:
                    ERR("Unexpected declaration in instanced attributes\n");
                    break;
            }
        }

        glDrawElements(glPrimitiveType, numberOfVertices, idxSize == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                    (const char *)idxData+(idxSize * startIdx));
        checkGLcall("glDrawElements");
    }
}

struct coords {
    int x, y, z;
};

void blt_to_drawable(IWineD3DDeviceImpl *This, IWineD3DSurfaceImpl *surface) {
    struct coords coords[4];
    int low_coord;

    /* TODO: This could be supported for lazy unlocking */
    if(!(surface->Flags & SFLAG_INTEXTURE)) {
        /* It is ok at init to be nowhere */
        if(!(surface->Flags & SFLAG_INSYSMEM)) {
            ERR("Blitting surfaces from sysmem not supported yet\n");
        }
        return;
    }

    ENTER_GL();
    ActivateContext(This, This->render_targets[0], CTXUSAGE_BLIT);

    if(surface->glDescription.target == GL_TEXTURE_2D) {
        glBindTexture(GL_TEXTURE_2D, surface->glDescription.textureName);
        checkGLcall("GL_TEXTURE_2D, This->glDescription.textureName)");

        coords[0].x = 0;    coords[0].y = 0;    coords[0].z = 0;
        coords[1].x = 0;    coords[1].y = 1;    coords[1].z = 0;
        coords[2].x = 1;    coords[2].y = 1;    coords[2].z = 0;
        coords[3].x = 1;    coords[3].y = 0;    coords[3].z = 0;

        low_coord = 0;
    } else {
        /* Must be a cube map */
        glDisable(GL_TEXTURE_2D);
        checkGLcall("glDisable(GL_TEXTURE_2D)");
        glEnable(GL_TEXTURE_CUBE_MAP_ARB);
        checkGLcall("glEnable(surface->glDescription.target)");
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, surface->glDescription.textureName);
        checkGLcall("GL_TEXTURE_CUBE_MAP_ARB, This->glDescription.textureName)");

        switch(surface->glDescription.target) {
            case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
                coords[0].x =  1;   coords[0].y = -1;   coords[0].z =  1;
                coords[1].x =  1;   coords[1].y =  1;   coords[1].z =  1;
                coords[2].x =  1;   coords[2].y =  1;   coords[2].z = -1;
                coords[3].x =  1;   coords[3].y = -1;   coords[3].z = -1;
                break;

            case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
                coords[0].x = -1;   coords[0].y = -1;   coords[0].z =  1;
                coords[1].x = -1;   coords[1].y =  1;   coords[1].z =  1;
                coords[2].x = -1;   coords[2].y =  1;   coords[2].z = -1;
                coords[3].x = -1;   coords[3].y = -1;   coords[3].z = -1;
                break;

            case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
                coords[0].x = -1;   coords[0].y =  1;   coords[0].z =  1;
                coords[1].x =  1;   coords[1].y =  1;   coords[1].z =  1;
                coords[2].x =  1;   coords[2].y =  1;   coords[2].z = -1;
                coords[3].x = -1;   coords[3].y =  1;   coords[3].z = -1;
                break;

            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
                coords[0].x = -1;   coords[0].y = -1;   coords[0].z =  1;
                coords[1].x =  1;   coords[1].y = -1;   coords[1].z =  1;
                coords[2].x =  1;   coords[2].y = -1;   coords[2].z = -1;
                coords[3].x = -1;   coords[3].y = -1;   coords[3].z = -1;
                break;

            case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
                coords[0].x = -1;   coords[0].y = -1;   coords[0].z =  1;
                coords[1].x =  1;   coords[1].y = -1;   coords[1].z =  1;
                coords[2].x =  1;   coords[2].y = -1;   coords[2].z =  1;
                coords[3].x = -1;   coords[3].y = -1;   coords[3].z =  1;
                break;

            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
                coords[0].x = -1;   coords[0].y = -1;   coords[0].z = -1;
                coords[1].x =  1;   coords[1].y = -1;   coords[1].z = -1;
                coords[2].x =  1;   coords[2].y = -1;   coords[2].z = -1;
                coords[3].x = -1;   coords[3].y = -1;   coords[3].z = -1;

            default:
                ERR("Unexpected texture target\n");
                LEAVE_GL();
                return;
        }

        low_coord = -1;
    }

    if(This->render_offscreen) {
        coords[0].y = coords[0].y == 1 ? low_coord : 1;
        coords[1].y = coords[1].y == 1 ? low_coord : 1;
        coords[2].y = coords[2].y == 1 ? low_coord : 1;
        coords[3].y = coords[3].y == 1 ? low_coord : 1;
    }

    glBegin(GL_QUADS);
        glTexCoord3iv((GLint *) &coords[0]);
        glVertex2i(0, 0);

        glTexCoord3iv((GLint *) &coords[1]);
        glVertex2i(0, surface->pow2Height);

        glTexCoord3iv((GLint *) &coords[2]);
        glVertex2i(surface->pow2Width, surface->pow2Height);

        glTexCoord3iv((GLint *) &coords[3]);
        glVertex2i(surface->pow2Width, 0);
    glEnd();
    checkGLcall("glEnd");

    if(surface->glDescription.target != GL_TEXTURE_2D) {
        glEnable(GL_TEXTURE_2D);
        checkGLcall("glEnable(GL_TEXTURE_2D)");
        glDisable(GL_TEXTURE_CUBE_MAP_ARB);
        checkGLcall("glDisable(GL_TEXTURE_CUBE_MAP_ARB)");
    }
    LEAVE_GL();
}

/* Routine common to the draw primitive and draw indexed primitive routines */
void drawPrimitive(IWineD3DDevice *iface,
                   int PrimitiveType,
                   long NumPrimitives,
                   /* for Indexed: */
                   long  StartVertexIndex,
                   UINT  numberOfVertices,
                   long  StartIdx,
                   short idxSize,
                   const void *idxData,
                   int   minIndex) {

    IWineD3DDeviceImpl           *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSwapChain            *swapchain;
    IWineD3DBaseTexture          *texture = NULL;
    IWineD3DSurfaceImpl          *target;
    int i;

    /* Signals other modules that a drawing is in progress and the stateblock finalized */
    This->isInDraw = TRUE;

    /* Invalidate the back buffer memory so LockRect will read it the next time */
    for(i = 0; i < GL_LIMITS(buffers); i++) {
        target = (IWineD3DSurfaceImpl *) This->render_targets[i];

        /* TODO: Only do all that if we're going to change anything
         * Texture container dirtification does not work quite right yet
         */
        if(target /*&& target->Flags & (SFLAG_INTEXTURE | SFLAG_INSYSMEM)*/) {
            swapchain = NULL;
            texture = NULL;

            if(i == 0) {
                IWineD3DSurface_GetContainer((IWineD3DSurface *) target, &IID_IWineD3DSwapChain, (void **)&swapchain);

                /* Need the surface in the drawable! */
                if(!(target->Flags & SFLAG_INDRAWABLE) && (swapchain || wined3d_settings.offscreen_rendering_mode != ORM_FBO)) {
                    blt_to_drawable(This, target);
                }

                if(swapchain) {
                    /* Onscreen target. Invalidate system memory copy and texture copy */
                    target->Flags &= ~(SFLAG_INSYSMEM | SFLAG_INTEXTURE);
                    target->Flags |= SFLAG_INDRAWABLE;
                    IWineD3DSwapChain_Release(swapchain);
                } else if(wined3d_settings.offscreen_rendering_mode != ORM_FBO) {
                    /* Non-FBO target: Invalidate system copy, texture copy and dirtify the container */
                    IWineD3DSurface_GetContainer((IWineD3DSurface *) target, &IID_IWineD3DBaseTexture, (void **)&texture);

                    if(texture) {
                        IWineD3DBaseTexture_SetDirty(texture, TRUE);
                        IWineD3DTexture_Release(texture);
                    }

                    target->Flags &= ~(SFLAG_INSYSMEM | SFLAG_INTEXTURE);
                    target->Flags |= SFLAG_INDRAWABLE;
                } else {
                    /* FBO offscreen target. Invalidate system memory copy */
                    target->Flags &= ~SFLAG_INSYSMEM;
                }
            } else {
                /* Must be an fbo render target */
                target->Flags &= ~SFLAG_INSYSMEM;
                target->Flags |=  SFLAG_INTEXTURE;
            }
        }
    }

    /* Ok, we will be updating the screen from here onwards so grab the lock */
    ENTER_GL();

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO) {
        apply_fbo_state(iface);
    }

    ActivateContext(This, This->render_targets[0], CTXUSAGE_DRAWPRIM);

    if (This->depth_copy_state == WINED3D_DCS_COPY) {
        depth_copy(iface);
    }
    This->depth_copy_state = WINED3D_DCS_INITIAL;

    {
        GLenum glPrimType;
        /* Ok, Work out which primitive is requested and how many vertexes that
           will be                                                              */
        UINT calculatedNumberOfindices = primitiveToGl(PrimitiveType, NumPrimitives, &glPrimType);
        if (numberOfVertices == 0 )
            numberOfVertices = calculatedNumberOfindices;

        if (This->useDrawStridedSlow) {
            /* Immediate mode drawing */
            drawStridedSlow(iface, &This->strided_streams, calculatedNumberOfindices,
                            glPrimType, idxData, idxSize, minIndex, StartIdx, StartVertexIndex);
        } else if(This->instancedDraw) {
            /* Instancing emulation with mixing immediate mode and arrays */
            drawStridedInstanced(iface, &This->strided_streams, calculatedNumberOfindices, glPrimType,
                            idxData, idxSize, minIndex, StartIdx, StartVertexIndex);
        } else {
            /* Simple array draw call */
            drawStridedFast(iface, calculatedNumberOfindices, glPrimType,
                            idxData, idxSize, minIndex, StartIdx, StartVertexIndex);
        }
    }

    /* Finshed updating the screen, restore lock */
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
            IWineD3DSurface_LockRect(This->renderTarget, &r, NULL, WINED3DLOCK_READONLY);
            sprintf(buffer, "/tmp/backbuffer_%d.tga", primCounter);
            TRACE("Saving screenshot %s\n", buffer);
            IWineD3DSurface_SaveSnapshot(This->renderTarget, buffer);
            IWineD3DSurface_UnlockRect(This->renderTarget);

#ifdef SHOW_TEXTURE_MAKEUP
           {
            IWineD3DSurface *pSur;
            int textureNo;
            for (textureNo = 0; textureNo < GL_LIMITS(textures); ++textureNo) {
                if (This->stateBlock->textures[textureNo] != NULL) {
                    sprintf(buffer, "/tmp/texture_%p_%d_%d.tga", This->stateBlock->textures[textureNo], primCounter, textureNo);
                    TRACE("Saving texture %s\n", buffer);
                    if (IWineD3DBaseTexture_GetType(This->stateBlock->textures[textureNo]) == WINED3DRTYPE_TEXTURE) {
                            IWineD3DTexture_GetSurfaceLevel((IWineD3DTexture *)This->stateBlock->textures[textureNo], 0, &pSur);
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
        TRACE("drawprim #%d\n", primCounter);
        ++primCounter;
    }
#endif

    /* Control goes back to the device, stateblock values may change again */
    This->isInDraw = FALSE;
}
