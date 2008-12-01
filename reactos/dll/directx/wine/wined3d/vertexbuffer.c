/*
 * IWineD3DVertexBuffer Implementation
 *
 * Copyright 2002-2005 Jason Edmeades
 *                     Raphael Junqueira
 * Copyright 2004 Christian Costa
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

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
#define GLINFO_LOCATION This->resource.wineD3DDevice->adapter->gl_info

#define VB_MAXDECLCHANGES     100     /* After that number we stop converting */
#define VB_RESETDECLCHANGE    1000    /* Reset the changecount after that number of draws */

/* *******************************************
   IWineD3DVertexBuffer IUnknown parts follow
   ******************************************* */
static HRESULT WINAPI IWineD3DVertexBufferImpl_QueryInterface(IWineD3DVertexBuffer *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DResource)
        || IsEqualGUID(riid, &IID_IWineD3DVertexBuffer)){
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IWineD3DVertexBufferImpl_AddRef(IWineD3DVertexBuffer *iface) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->resource.ref);
    TRACE("(%p) : AddRef increasing from %d\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IWineD3DVertexBufferImpl_Release(IWineD3DVertexBuffer *iface) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->resource.ref);
    TRACE("(%p) : Releasing from %d\n", This, ref + 1);
    if (ref == 0) {

        if(This->vbo) {
            IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;

            ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
            ENTER_GL();
            GL_EXTCALL(glDeleteBuffersARB(1, &This->vbo));
            checkGLcall("glDeleteBuffersARB");
            LEAVE_GL();
        }

        IWineD3DResourceImpl_CleanUp((IWineD3DResource *)iface);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* ****************************************************
   IWineD3DVertexBuffer IWineD3DResource parts follow
   **************************************************** */
static HRESULT WINAPI IWineD3DVertexBufferImpl_GetDevice(IWineD3DVertexBuffer *iface, IWineD3DDevice** ppDevice) {
    return IWineD3DResourceImpl_GetDevice((IWineD3DResource *)iface, ppDevice);
}

static HRESULT WINAPI IWineD3DVertexBufferImpl_SetPrivateData(IWineD3DVertexBuffer *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    return IWineD3DResourceImpl_SetPrivateData((IWineD3DResource *)iface, refguid, pData, SizeOfData, Flags);
}

static HRESULT WINAPI IWineD3DVertexBufferImpl_GetPrivateData(IWineD3DVertexBuffer *iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    return IWineD3DResourceImpl_GetPrivateData((IWineD3DResource *)iface, refguid, pData, pSizeOfData);
}

static HRESULT WINAPI IWineD3DVertexBufferImpl_FreePrivateData(IWineD3DVertexBuffer *iface, REFGUID refguid) {
    return IWineD3DResourceImpl_FreePrivateData((IWineD3DResource *)iface, refguid);
}

static DWORD    WINAPI IWineD3DVertexBufferImpl_SetPriority(IWineD3DVertexBuffer *iface, DWORD PriorityNew) {
    return IWineD3DResourceImpl_SetPriority((IWineD3DResource *)iface, PriorityNew);
}

static DWORD    WINAPI IWineD3DVertexBufferImpl_GetPriority(IWineD3DVertexBuffer *iface) {
    return IWineD3DResourceImpl_GetPriority((IWineD3DResource *)iface);
}

static inline void fixup_d3dcolor(DWORD *pos) {
    DWORD srcColor = *pos;

    /* Color conversion like in drawStridedSlow. watch out for little endianity
     * If we want that stuff to work on big endian machines too we have to consider more things
     *
     * 0xff000000: Alpha mask
     * 0x00ff0000: Blue mask
     * 0x0000ff00: Green mask
     * 0x000000ff: Red mask
     */
    *pos = 0;
    *pos |= (srcColor & 0xff00ff00)      ;   /* Alpha Green */
    *pos |= (srcColor & 0x00ff0000) >> 16;   /* Red */
    *pos |= (srcColor & 0x000000ff) << 16;   /* Blue */
}

static inline void fixup_transformed_pos(float *p) {
    float x, y, z, w;

    /* rhw conversion like in drawStridedSlow */
    if(p[3] == 1.0 || ((p[3] < eps) && (p[3] > -eps))) {
        x = p[0];
        y = p[1];
        z = p[2];
        w = 1.0;
    } else {
        w = 1.0 / p[3];
        x = p[0] * w;
        y = p[1] * w;
        z = p[2] * w;
    }
    p[0] = x;
    p[1] = y;
    p[2] = z;
    p[3] = w;
}

DWORD *find_conversion_shift(IWineD3DVertexBufferImpl *This, WineDirect3DVertexStridedData *strided, DWORD stride) {
    DWORD *ret, i, shift, j, type;
    DWORD orig_type_size;

    if(!stride) {
        TRACE("No shift\n");
        return NULL;
    }

    This->conv_stride = stride;
    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DWORD) * stride);
    for(i = 0; i < MAX_ATTRIBS; i++) {
        if(strided->u.input[i].VBO != This->vbo) continue;

        type = strided->u.input[i].dwType;
        if(type == WINED3DDECLTYPE_FLOAT16_2) {
            shift = 4;
        } else if(type == WINED3DDECLTYPE_FLOAT16_4) {
            shift = 8;
            /* Pre-shift the last 4 bytes in the FLOAT16_4 by 4 bytes - this makes FLOAT16_2 and FLOAT16_4 conversions
             * compatible
             */
            for(j = 4; j < 8; j++) {
                ret[(DWORD_PTR) strided->u.input[i].lpData + j ] += 4;
            }
        } else {
            shift = 0;
        }
        This->conv_stride += shift;

        if(shift) {
            orig_type_size = WINED3D_ATR_TYPESIZE(type) * WINED3D_ATR_SIZE(type);
            for(j = (DWORD_PTR) strided->u.input[i].lpData + orig_type_size; j < stride; j++) {
                ret[j] += shift;
            }
        }
    }

    if(TRACE_ON(d3d)) {
        TRACE("Dumping conversion shift:\n");
        for(i = 0; i < stride; i++) {
            TRACE("[%d]", ret[i]);
        }
        TRACE("\n");
    }
    return ret;
}

static inline BOOL process_converted_attribute(IWineD3DVertexBufferImpl *This,
                                               const enum vbo_conversion_type conv_type,
                                               const WineDirect3DStridedData *attrib,
                                               DWORD *stride_this_run, const DWORD type) {
    DWORD attrib_size;
    BOOL ret = FALSE;
    int i;
    DWORD offset = This->resource.wineD3DDevice->stateBlock->streamOffset[attrib->streamNo];
    DWORD_PTR data;

    /* Check for some valid situations which cause us pain. One is if the buffer is used for
     * constant attributes(stride = 0), the other one is if the buffer is used on two streams
     * with different strides. In the 2nd case we might have to drop conversion entirely,
     * it is possible that the same bytes are once read as FLOAT2 and once as UBYTE4N.
     */
    if(attrib->dwStride == 0) {
        FIXME("%s used with stride 0, let's hope we get the vertex stride from somewhere else\n",
                debug_d3ddecltype(type));
    } else if(attrib->dwStride != *stride_this_run &&
                *stride_this_run) {
        FIXME("Got two concurrent strides, %d and %d\n", attrib->dwStride, *stride_this_run);
    } else {
        *stride_this_run = attrib->dwStride;
        if(This->stride != *stride_this_run) {
            /* We rely that this happens only on the first converted attribute that is found,
             * if at all. See above check
             */
            TRACE("Reconverting because converted attributes occur, and the stride changed\n");
            This->stride = *stride_this_run;
            HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, This->conv_map);
            This->conv_map = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*This->conv_map) * This->stride);
            ret = TRUE;
        }
    }

    data = (((DWORD_PTR) attrib->lpData) + offset) % This->stride;
    attrib_size = WINED3D_ATR_SIZE(type) * WINED3D_ATR_TYPESIZE(type);
    for(i = 0; i < attrib_size; i++) {
        if(This->conv_map[data + i] != conv_type) {
            TRACE("Byte %ld in vertex changed\n", i + data);
            TRACE("It was type %d, is %d now\n", This->conv_map[data + i], conv_type);
            ret = TRUE;
            This->conv_map[data + i] = conv_type;
        }
    }
    return ret;
}

static inline BOOL check_attribute(IWineD3DVertexBufferImpl *This, const WineDirect3DStridedData *attrib,
                                   const BOOL check_d3dcolor, const BOOL is_ffp_position, const BOOL is_ffp_color,
                                   DWORD *stride_this_run, BOOL *float16_used) {
    BOOL ret = FALSE;
    DWORD type;

    /* Ignore attributes that do not have our vbo. After that check we can be sure that the attribute is
     * there, on nonexistent attribs the vbo is 0.
     */
    if(attrib->VBO != This->vbo) return FALSE;

    type = attrib->dwType;
    /* Look for newly appeared conversion */
    if(!GL_SUPPORT(NV_HALF_FLOAT) && (
       type == WINED3DDECLTYPE_FLOAT16_2 ||
       type == WINED3DDECLTYPE_FLOAT16_4)) {

        ret = process_converted_attribute(This, CONV_FLOAT16_2, attrib, stride_this_run, type);

        if(is_ffp_position) {
            FIXME("Test FLOAT16 fixed function processing positions\n");
        } else if(is_ffp_color) {
            FIXME("test FLOAT16 fixed function processing colors\n");
        }
        *float16_used = TRUE;
    } else if(check_d3dcolor && type == WINED3DDECLTYPE_D3DCOLOR) {

        ret = process_converted_attribute(This, CONV_D3DCOLOR, attrib, stride_this_run, WINED3DDECLTYPE_D3DCOLOR);

        if(!is_ffp_color) {
            FIXME("Test for non-color fixed function D3DCOLOR type\n");
        }
    } else if(is_ffp_position && type == WINED3DDECLTYPE_FLOAT4) {
        ret = process_converted_attribute(This, CONV_POSITIONT, attrib, stride_this_run, WINED3DDECLTYPE_FLOAT4);
    } else if(This->conv_map) {
        ret = process_converted_attribute(This, CONV_NONE, attrib, stride_this_run, type);
    }
    return ret;
}

inline BOOL WINAPI IWineD3DVertexBufferImpl_FindDecl(IWineD3DVertexBufferImpl *This)
{
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
    BOOL ret = FALSE;
    int i;
    DWORD stride_this_run = 0;
    BOOL float16_used = FALSE;

    /* In d3d7 the vertex buffer declaration NEVER changes because it is stored in the d3d7 vertex buffer.
     * Once we have our declaration there is no need to look it up again.
     */
    if(((IWineD3DImpl *)device->wineD3D)->dxVersion == 7 && This->Flags & VBFLAG_HASDESC) {
        return FALSE;
    }

    TRACE("Finding vertex buffer conversion information\n");
    /* Certain declaration types need some fixups before we can pass them to opengl. This means D3DCOLOR attributes with fixed
     * function vertex processing, FLOAT4 POSITIONT with fixed function, and FLOAT16 if GL_NV_half_float is not supported.
     *
     * The vertex buffer FVF doesn't help with finding them, we have to use the decoded vertex declaration and pick the things
     * that concern the current buffer. A problem with this is that this can change between draws, so we have to validate
     * the information and reprocess the buffer if it changes, and avoid false positives for performance reasons.
     *
     * We have to distinguish between vertex shaders and fixed function to pick the way we access the
     * strided vertex information.
     *
     * This code sets up a per-byte array with the size of the detected stride of the arrays in the
     * buffer. For each byte we have a field that marks the conversion needed on this byte. For example,
     * the following declaration with fixed function vertex processing:
     *
     *      POSITIONT, FLOAT4
     *      NORMAL, FLOAT3
     *      DIFFUSE, FLOAT16_4
     *      SPECULAR, D3DCOLOR
     *
     * Will result in
     * {                 POSITIONT                    }{             NORMAL                }{    DIFFUSE          }{SPECULAR }
     * [P][P][P][P][P][P][P][P][P][P][P][P][P][P][P][P][0][0][0][0][0][0][0][0][0][0][0][0][F][F][F][F][F][F][F][F][C][C][C][C]
     *
     * Where in this example map P means 4 component position conversion, 0 means no conversion, F means FLOAT16_2 conversion
     * and C means D3DCOLOR conversion(red / blue swizzle).
     *
     * If we're doing conversion and the stride changes we have to reconvert the whole buffer. Note that we do not mind if the
     * semantic changes, we only care for the conversion type. So if the NORMAL is replaced with a TEXCOORD, nothing has to be
     * done, or if the DIFFUSE is replaced with a D3DCOLOR BLENDWEIGHT we can happily dismiss the change. Some conversion types
     * depend on the semantic as well, for example a FLOAT4 texcoord needs no conversion while a FLOAT4 positiont needs one
     */
    if(use_vs(device)) {
        TRACE("vhsader\n");
        /* If the current vertex declaration is marked for no half float conversion don't bother to
         * analyse the strided streams in depth, just set them up for no conversion. Return decl changed
         * if we used conversion before
         */
        if(!((IWineD3DVertexDeclarationImpl *) device->stateBlock->vertexDecl)->half_float_conv_needed) {
            if(This->conv_map) {
                TRACE("Now using shaders without conversion, but conversion used before\n");
                HeapFree(GetProcessHeap(), 0, This->conv_map);
                HeapFree(GetProcessHeap(), 0, This->conv_shift);
                This->conv_map = NULL;
                This->stride = 0;
                This->conv_shift = NULL;
                This->conv_stride = 0;
                return TRUE;
            } else {
                return FALSE;
            }
        }
        for(i = 0; i < MAX_ATTRIBS; i++) {
            ret = check_attribute(This, &device->strided_streams.u.input[i], FALSE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        }

        /* Recalculate the conversion shift map if the declaration has changed,
         * and we're using float16 conversion or used it on the last run
         */
        if(ret && (float16_used || This->conv_map)) {
            HeapFree(GetProcessHeap(), 0, This->conv_shift);
            This->conv_shift = find_conversion_shift(This, &device->strided_streams, This->stride);
        }
    } else {
        /* Fixed function is a bit trickier. We have to take care for D3DCOLOR types, FLOAT4 positions and of course
         * FLOAT16s if not supported. Also, we can't iterate over the array, so use macros to generate code for all
         * the attributes that our current fixed function pipeline implementation cares for.
         */
        ret = check_attribute(This, &device->strided_streams.u.s.position,     TRUE, TRUE,  FALSE, &stride_this_run, &float16_used) || ret;
        ret = check_attribute(This, &device->strided_streams.u.s.normal,       TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        ret = check_attribute(This, &device->strided_streams.u.s.diffuse,      TRUE, FALSE, TRUE,  &stride_this_run, &float16_used) || ret;
        ret = check_attribute(This, &device->strided_streams.u.s.specular,     TRUE, FALSE, TRUE,  &stride_this_run, &float16_used) || ret;
        ret = check_attribute(This, &device->strided_streams.u.s.texCoords[0], TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        ret = check_attribute(This, &device->strided_streams.u.s.texCoords[1], TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        ret = check_attribute(This, &device->strided_streams.u.s.texCoords[2], TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        ret = check_attribute(This, &device->strided_streams.u.s.texCoords[3], TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        ret = check_attribute(This, &device->strided_streams.u.s.texCoords[4], TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        ret = check_attribute(This, &device->strided_streams.u.s.texCoords[5], TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        ret = check_attribute(This, &device->strided_streams.u.s.texCoords[6], TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        ret = check_attribute(This, &device->strided_streams.u.s.texCoords[7], TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;

        if(float16_used) FIXME("Float16 conversion used with fixed function vertex processing\n");
    }

    if(stride_this_run == 0 && This->conv_map) {
        /* Sanity test */
        if(ret == FALSE) {
            ERR("no converted attributes found, old conversion map exists, and no declaration change???\n");
        }
        HeapFree(GetProcessHeap(), 0, This->conv_map);
        This->conv_map = NULL;
        This->stride = 0;
    }
    This->Flags |= VBFLAG_HASDESC;

    if(ret) TRACE("Conversion information changed\n");
    return ret;
}

static void check_vbo_size(IWineD3DVertexBufferImpl *This) {
    DWORD size = This->conv_stride ? This->conv_stride * (This->resource.size / This->stride) : This->resource.size;
    if(This->vbo_size != size) {
        TRACE("Old size %d, creating new size %d\n", This->vbo_size, size);
        ENTER_GL();
        GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, This->vbo));
        checkGLcall("glBindBufferARB");
        GL_EXTCALL(glBufferDataARB(GL_ARRAY_BUFFER_ARB, size, NULL, This->vbo_usage));
        This->vbo_size = size;
        checkGLcall("glBufferDataARB");
        LEAVE_GL();
    }
}

static void CreateVBO(IWineD3DVertexBufferImpl *This) {
    GLenum error, glUsage;
    DWORD vboUsage = This->resource.usage;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;

    TRACE("Creating an OpenGL vertex buffer object for IWineD3DVertexBuffer %p  Usage(%s)\n", This, debug_d3dusage(vboUsage));

    /* Make sure that a context is there. Needed in a multithreaded environment. Otherwise this call is a nop */
    ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
    ENTER_GL();

    /* Make sure that the gl error is cleared. Do not use checkGLcall
    * here because checkGLcall just prints a fixme and continues. However,
    * if an error during VBO creation occurs we can fall back to non-vbo operation
    * with full functionality(but performance loss)
    */
    while(glGetError() != GL_NO_ERROR);

    /* Basically the FVF parameter passed to CreateVertexBuffer is no good
    * It is the FVF set with IWineD3DDevice::SetFVF or the Vertex Declaration set with
    * IWineD3DDevice::SetVertexDeclaration that decides how the vertices in the buffer
    * look like. This means that on each DrawPrimitive call the vertex buffer has to be verified
    * to check if the rhw and color values are in the correct format.
    */

    GL_EXTCALL(glGenBuffersARB(1, &This->vbo));
    error = glGetError();
    if(This->vbo == 0 || error != GL_NO_ERROR) {
        WARN("Failed to create a VBO with error %s (%#x)\n", debug_glerror(error), error);
        goto error;
    }

    GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, This->vbo));
    error = glGetError();
    if(error != GL_NO_ERROR) {
        WARN("Failed to bind the VBO with error %s (%#x)\n", debug_glerror(error), error);
        goto error;
    }

    /* Don't use static, because dx apps tend to update the buffer
    * quite often even if they specify 0 usage. Because we always keep the local copy
    * we never read from the vbo and can create a write only opengl buffer.
    */
    switch(vboUsage & (WINED3DUSAGE_WRITEONLY | WINED3DUSAGE_DYNAMIC) ) {
        case WINED3DUSAGE_WRITEONLY | WINED3DUSAGE_DYNAMIC:
        case WINED3DUSAGE_DYNAMIC:
            TRACE("Gl usage = GL_STREAM_DRAW\n");
            glUsage = GL_STREAM_DRAW_ARB;
            break;
        case WINED3DUSAGE_WRITEONLY:
        default:
            TRACE("Gl usage = GL_DYNAMIC_DRAW\n");
            glUsage = GL_DYNAMIC_DRAW_ARB;
            break;
    }

    /* Reserve memory for the buffer. The amount of data won't change
    * so we are safe with calling glBufferData once with a NULL ptr and
    * calling glBufferSubData on updates
    */
    GL_EXTCALL(glBufferDataARB(GL_ARRAY_BUFFER_ARB, This->resource.size, NULL, glUsage));
    error = glGetError();
    if(error != GL_NO_ERROR) {
        WARN("glBufferDataARB failed with error %s (%#x)\n", debug_glerror(error), error);
        goto error;
    }
    This->vbo_size = This->resource.size;
    This->vbo_usage = glUsage;
    This->dirtystart = 0;
    This->dirtyend = This->resource.size;
    This->Flags |= VBFLAG_DIRTY;

    LEAVE_GL();

    return;
    error:
            /* Clean up all vbo init, but continue because we can work without a vbo :-) */
            FIXME("Failed to create a vertex buffer object. Continuing, but performance issues can occur\n");
    if(This->vbo) GL_EXTCALL(glDeleteBuffersARB(1, &This->vbo));
    This->vbo = 0;
    LEAVE_GL();
    return;
}

static void     WINAPI IWineD3DVertexBufferImpl_PreLoad(IWineD3DVertexBuffer *iface) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *) iface;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
    BYTE *data;
    UINT start = 0, end = 0, vertices;
    BOOL declChanged = FALSE;
    int i, j;
    TRACE("(%p)->()\n", This);

    if(!This->vbo) {
        /* TODO: Make converting independent from VBOs */
        if(This->Flags & VBFLAG_CREATEVBO) {
            CreateVBO(This);
            This->Flags &= ~VBFLAG_CREATEVBO;
        } else {
            return; /* Not doing any conversion */
        }
    }

    /* Reading the declaration makes only sense if the stateblock is finalized and the buffer bound to a stream */
    if(device->isInDraw && This->bindCount > 0) {
        declChanged = IWineD3DVertexBufferImpl_FindDecl(This);
    } else if(This->Flags & VBFLAG_HASDESC) {
        /* Reuse the declaration stored in the buffer. It will most likely not change, and if it does
         * the stream source state handler will call PreLoad again and the change will be caught
         */
    } else {
        /* Cannot get a declaration, and no declaration is stored in the buffer. It is pointless to preload
         * now. When the buffer is used, PreLoad will be called by the stream source state handler and a valid
         * declaration for the buffer can be found
         */
        return;
    }

    /* If applications change the declaration over and over, reconverting all the time is a huge
     * performance hit. So count the declaration changes and release the VBO if there are too many
     * of them (and thus stop converting)
     */
    if(declChanged) {
        This->declChanges++;
        This->draws = 0;

        if(This->declChanges > VB_MAXDECLCHANGES) {
            FIXME("Too many declaration changes, stopping converting\n");
            ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
            ENTER_GL();
            GL_EXTCALL(glDeleteBuffersARB(1, &This->vbo));
            checkGLcall("glDeleteBuffersARB");
            LEAVE_GL();
            This->vbo = 0;
            HeapFree(GetProcessHeap(), 0, This->conv_shift);

            /* The stream source state handler might have read the memory of the vertex buffer already
             * and got the memory in the vbo which is not valid any longer. Dirtify the stream source
             * to force a reload. This happens only once per changed vertexbuffer and should occur rather
             * rarely
             */
            IWineD3DDeviceImpl_MarkStateDirty(device, STATE_STREAMSRC);

            return;
        }
        check_vbo_size(This);
    } else {
        /* However, it is perfectly fine to change the declaration every now and then. We don't want a game that
         * changes it every minute drop the VBO after VB_MAX_DECL_CHANGES minutes. So count draws without
         * decl changes and reset the decl change count after a specific number of them
         */
        This->draws++;
        if(This->draws > VB_RESETDECLCHANGE) This->declChanges = 0;
    }

    if(declChanged) {
        /* The declaration changed, reload the whole buffer */
        WARN("Reloading buffer because of decl change\n");
        start = 0;
        end = This->resource.size;
    } else if(This->Flags & VBFLAG_DIRTY) {
        /* No decl change, but dirty data, reload the changed stuff */
        if(This->conv_shift) {
            if(This->dirtystart != 0 || This->dirtyend != 0) {
                FIXME("Implement partial buffer loading with shifted conversion\n");
            }
        }
        start = This->dirtystart;
        end = This->dirtyend;
    } else {
        /* Desc not changed, buffer not dirty, nothing to do :-) */
        return;
    }

    /* Mark the buffer clean */
    This->Flags &= ~VBFLAG_DIRTY;
    This->dirtystart = 0;
    This->dirtyend = 0;

    if(!This->conv_map) {
        /* That means that there is nothing to fixup. Just upload from This->resource.allocatedMemory
         * directly into the vbo. Do not free the system memory copy because drawPrimitive may need it if
         * the stride is 0, for instancing emulation, vertex blending emulation or shader emulation.
         */
        TRACE("No conversion needed\n");

        if(!device->isInDraw) {
            ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
        }
        ENTER_GL();
        GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, This->vbo));
        checkGLcall("glBindBufferARB");
        GL_EXTCALL(glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, start, end-start, This->resource.allocatedMemory + start));
        checkGLcall("glBufferSubDataARB");
        LEAVE_GL();
        return;
    }

    /* Now for each vertex in the buffer that needs conversion */
    vertices = This->resource.size / This->stride;

    if(This->conv_shift) {
        TRACE("Shifted conversion\n");
        data = HeapAlloc(GetProcessHeap(), 0, vertices * This->conv_stride);

        for(i = start / This->stride; i < min((end / This->stride) + 1, vertices); i++) {
            for(j = 0; j < This->stride; j++) {
                switch(This->conv_map[j]) {
                    case CONV_NONE:
                        data[This->conv_stride * i + j + This->conv_shift[j]] = This->resource.allocatedMemory[This->stride * i + j];
                        break;

                    case CONV_FLOAT16_2:
                    {
                        float *out = (float *) (&data[This->conv_stride * i + j + This->conv_shift[j]]);
                        WORD *in = (WORD *) (&This->resource.allocatedMemory[i * This->stride + j]);

                        out[1] = float_16_to_32(in + 1);
                        out[0] = float_16_to_32(in + 0);
                        j += 3;    /* Skip 3 additional bytes,as a FLOAT16_2 has 4 bytes */
                        break;
                    }

                    default:
                        FIXME("Unimplemented conversion %d in shifted conversion\n", This->conv_map[j]);
                }
            }
        }

        ENTER_GL();
        GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, This->vbo));
        checkGLcall("glBindBufferARB");
        GL_EXTCALL(glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, vertices * This->conv_stride, data));
        checkGLcall("glBufferSubDataARB");
        LEAVE_GL();

    } else {
        data = HeapAlloc(GetProcessHeap(), 0, This->resource.size);
        memcpy(data + start, This->resource.allocatedMemory + start, end - start);
        for(i = start / This->stride; i < min((end / This->stride) + 1, vertices); i++) {
            for(j = 0; j < This->stride; j++) {
                switch(This->conv_map[j]) {
                    case CONV_NONE:
                        /* Done already */
                        j += 3;
                        break;
                    case CONV_D3DCOLOR:
                        fixup_d3dcolor((DWORD *) (data + i * This->stride + j));
                        j += 3;
                        break;

                    case CONV_POSITIONT:
                        fixup_transformed_pos((float *) (data + i * This->stride + j));
                        j += 15;
                        break;

                    case CONV_FLOAT16_2:
                        ERR("Did not expect FLOAT16 conversion in unshifted conversion\n");
                    default:
                        FIXME("Unimplemented conversion %d in shifted conversion\n", This->conv_map[j]);
                }
            }
        }

        ENTER_GL();
        GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, This->vbo));
        checkGLcall("glBindBufferARB");
        GL_EXTCALL(glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, start, end - start, data + start));
        checkGLcall("glBufferSubDataARB");
        LEAVE_GL();
    }

    HeapFree(GetProcessHeap(), 0, data);
}

static void WINAPI IWineD3DVertexBufferImpl_UnLoad(IWineD3DVertexBuffer *iface) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *) iface;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
    TRACE("(%p)\n", This);

    /* This is easy: The whole content is shadowed in This->resource.allocatedMemory,
     * so we only have to destroy the vbo. Only do it if we have a vbo, which implies
     * that vbos are supported
     */
    if(This->vbo) {
        ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
        ENTER_GL();
        GL_EXTCALL(glDeleteBuffersARB(1, &This->vbo));
        checkGLcall("glDeleteBuffersARB");
        LEAVE_GL();
        This->vbo = 0;
        This->Flags |= VBFLAG_CREATEVBO; /* Recreate the VBO next load */
    }
}

static WINED3DRESOURCETYPE WINAPI IWineD3DVertexBufferImpl_GetType(IWineD3DVertexBuffer *iface) {
    return IWineD3DResourceImpl_GetType((IWineD3DResource *)iface);
}

static HRESULT WINAPI IWineD3DVertexBufferImpl_GetParent(IWineD3DVertexBuffer *iface, IUnknown **pParent) {
    return IWineD3DResourceImpl_GetParent((IWineD3DResource *)iface, pParent);
}

/* ******************************************************
   IWineD3DVertexBuffer IWineD3DVertexBuffer parts follow
   ****************************************************** */
static HRESULT  WINAPI IWineD3DVertexBufferImpl_Lock(IWineD3DVertexBuffer *iface, UINT OffsetToLock, UINT SizeToLock, BYTE** ppbData, DWORD Flags) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;
    BYTE *data;
    TRACE("(%p)->%d, %d, %p, %08x\n", This, OffsetToLock, SizeToLock, ppbData, Flags);

    InterlockedIncrement(&This->lockcount);

    if(This->Flags & VBFLAG_DIRTY) {
        if(This->dirtystart > OffsetToLock) This->dirtystart = OffsetToLock;
        if(SizeToLock) {
            if(This->dirtyend < OffsetToLock + SizeToLock) This->dirtyend = OffsetToLock + SizeToLock;
        } else {
            This->dirtyend = This->resource.size;
        }
    } else {
        This->dirtystart = OffsetToLock;
        if(SizeToLock)
            This->dirtyend = OffsetToLock + SizeToLock;
        else
            This->dirtyend = This->resource.size;
    }

    data = This->resource.allocatedMemory;
    This->Flags |= VBFLAG_DIRTY;
    *ppbData = data + OffsetToLock;

    TRACE("(%p) : returning memory of %p (base:%p,offset:%u)\n", This, data + OffsetToLock, data, OffsetToLock);
    /* TODO: check Flags compatibility with This->currentDesc.Usage (see MSDN) */
    return WINED3D_OK;
}
HRESULT  WINAPI IWineD3DVertexBufferImpl_Unlock(IWineD3DVertexBuffer *iface) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *) iface;
    LONG lockcount;
    TRACE("(%p)\n", This);

    lockcount = InterlockedDecrement(&This->lockcount);
    if(lockcount > 0) {
        /* Delay loading the buffer until everything is unlocked */
        TRACE("Ignoring the unlock\n");
        return WINED3D_OK;
    }

    if(This->Flags & VBFLAG_HASDESC) {
        IWineD3DVertexBufferImpl_PreLoad(iface);
    }
    return WINED3D_OK;
}
static HRESULT  WINAPI IWineD3DVertexBufferImpl_GetDesc(IWineD3DVertexBuffer *iface, WINED3DVERTEXBUFFER_DESC *pDesc) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;

    TRACE("(%p)\n", This);
    pDesc->Format = This->resource.format;
    pDesc->Type   = This->resource.resourceType;
    pDesc->Usage  = This->resource.usage;
    pDesc->Pool   = This->resource.pool;
    pDesc->Size   = This->resource.size;
    pDesc->FVF    = This->fvf;
    return WINED3D_OK;
}

const IWineD3DVertexBufferVtbl IWineD3DVertexBuffer_Vtbl =
{
    /* IUnknown */
    IWineD3DVertexBufferImpl_QueryInterface,
    IWineD3DVertexBufferImpl_AddRef,
    IWineD3DVertexBufferImpl_Release,
    /* IWineD3DResource */
    IWineD3DVertexBufferImpl_GetParent,
    IWineD3DVertexBufferImpl_GetDevice,
    IWineD3DVertexBufferImpl_SetPrivateData,
    IWineD3DVertexBufferImpl_GetPrivateData,
    IWineD3DVertexBufferImpl_FreePrivateData,
    IWineD3DVertexBufferImpl_SetPriority,
    IWineD3DVertexBufferImpl_GetPriority,
    IWineD3DVertexBufferImpl_PreLoad,
    IWineD3DVertexBufferImpl_UnLoad,
    IWineD3DVertexBufferImpl_GetType,
    /* IWineD3DVertexBuffer */
    IWineD3DVertexBufferImpl_Lock,
    IWineD3DVertexBufferImpl_Unlock,
    IWineD3DVertexBufferImpl_GetDesc
};

BYTE* WINAPI IWineD3DVertexBufferImpl_GetMemory(IWineD3DVertexBuffer* iface, DWORD iOffset, GLint *vbo) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;

    *vbo = This->vbo;
    if(This->vbo == 0) {
        if(This->Flags & VBFLAG_CREATEVBO) {
            CreateVBO(This);
            This->Flags &= ~VBFLAG_CREATEVBO;
            if(This->vbo) {
                *vbo = This->vbo;
                return (BYTE *) iOffset;
            }
        }
        return This->resource.allocatedMemory + iOffset;
    } else {
        return (BYTE *) iOffset;
    }
}

HRESULT WINAPI IWineD3DVertexBufferImpl_ReleaseMemory(IWineD3DVertexBuffer* iface) {
  return WINED3D_OK;
}
