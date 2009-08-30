/*
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2007 Stefan Dösinger for CodeWeavers
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
 *
 */

#include "config.h"
#include "wine/port.h"

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

#define GLINFO_LOCATION This->resource.wineD3DDevice->adapter->gl_info

#define VB_MAXDECLCHANGES     100     /* After that number we stop converting */
#define VB_RESETDECLCHANGE    1000    /* Reset the changecount after that number of draws */

static void buffer_create_buffer_object(struct wined3d_buffer *This)
{
    GLenum error, gl_usage;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;

    TRACE("Creating an OpenGL vertex buffer object for IWineD3DVertexBuffer %p Usage(%s)\n",
            This, debug_d3dusage(This->resource.usage));

    /* Make sure that a context is there. Needed in a multithreaded environment. Otherwise this call is a nop */
    ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
    ENTER_GL();

    /* Make sure that the gl error is cleared. Do not use checkGLcall
    * here because checkGLcall just prints a fixme and continues. However,
    * if an error during VBO creation occurs we can fall back to non-vbo operation
    * with full functionality(but performance loss)
    */
    while (glGetError() != GL_NO_ERROR);

    /* Basically the FVF parameter passed to CreateVertexBuffer is no good
     * It is the FVF set with IWineD3DDevice::SetFVF or the Vertex Declaration set with
     * IWineD3DDevice::SetVertexDeclaration that decides how the vertices in the buffer
     * look like. This means that on each DrawPrimitive call the vertex buffer has to be verified
     * to check if the rhw and color values are in the correct format.
     */

    GL_EXTCALL(glGenBuffersARB(1, &This->buffer_object));
    error = glGetError();
    if (!This->buffer_object || error != GL_NO_ERROR)
    {
        ERR("Failed to create a VBO with error %s (%#x)\n", debug_glerror(error), error);
        goto fail;
    }

    if(This->buffer_type_hint == GL_ELEMENT_ARRAY_BUFFER_ARB)
    {
        IWineD3DDeviceImpl_MarkStateDirty(This->resource.wineD3DDevice, STATE_INDEXBUFFER);
    }
    GL_EXTCALL(glBindBufferARB(This->buffer_type_hint, This->buffer_object));
    error = glGetError();
    if (error != GL_NO_ERROR)
    {
        ERR("Failed to bind the VBO with error %s (%#x)\n", debug_glerror(error), error);
        goto fail;
    }

    /* Don't use static, because dx apps tend to update the buffer
    * quite often even if they specify 0 usage. Because we always keep the local copy
    * we never read from the vbo and can create a write only opengl buffer.
    */
    switch(This->resource.usage & (WINED3DUSAGE_WRITEONLY | WINED3DUSAGE_DYNAMIC))
    {
        case WINED3DUSAGE_WRITEONLY | WINED3DUSAGE_DYNAMIC:
        case WINED3DUSAGE_DYNAMIC:
            TRACE("Gl usage = GL_STREAM_DRAW\n");
            gl_usage = GL_STREAM_DRAW_ARB;
            break;

        case WINED3DUSAGE_WRITEONLY:
        default:
            TRACE("Gl usage = GL_DYNAMIC_DRAW\n");
            gl_usage = GL_DYNAMIC_DRAW_ARB;
            break;
    }

    /* Reserve memory for the buffer. The amount of data won't change
     * so we are safe with calling glBufferData once and
     * calling glBufferSubData on updates. Upload the actual data in case
     * we're not double buffering, so we can release the heap mem afterwards
     */
    GL_EXTCALL(glBufferDataARB(This->buffer_type_hint, This->resource.size, This->resource.allocatedMemory, gl_usage));
    error = glGetError();
    if (error != GL_NO_ERROR)
    {
        ERR("glBufferDataARB failed with error %s (%#x)\n", debug_glerror(error), error);
        goto fail;
    }

    LEAVE_GL();

    This->buffer_object_size = This->resource.size;
    This->buffer_object_usage = gl_usage;
    This->dirty_start = 0;
    This->dirty_end = This->resource.size;

    if(This->flags & WINED3D_BUFFER_DOUBLEBUFFER)
    {
        This->flags |= WINED3D_BUFFER_DIRTY;
    }
    else
    {
        HeapFree(GetProcessHeap(), 0, This->resource.heapMemory);
        This->resource.allocatedMemory = NULL;
        This->resource.heapMemory = NULL;
        This->flags &= ~WINED3D_BUFFER_DIRTY;
    }

    return;

fail:
    /* Clean up all vbo init, but continue because we can work without a vbo :-) */
    ERR("Failed to create a vertex buffer object. Continuing, but performance issues may occur\n");
    if (This->buffer_object) GL_EXTCALL(glDeleteBuffersARB(1, &This->buffer_object));
    This->buffer_object = 0;
    LEAVE_GL();

    return;
}

static BOOL buffer_process_converted_attribute(struct wined3d_buffer *This,
        const enum wined3d_buffer_conversion_type conversion_type,
        const struct wined3d_stream_info_element *attrib, DWORD *stride_this_run)
{
    DWORD attrib_size;
    BOOL ret = FALSE;
    unsigned int i;
    DWORD offset = This->resource.wineD3DDevice->stateBlock->streamOffset[attrib->stream_idx];
    DWORD_PTR data;

    /* Check for some valid situations which cause us pain. One is if the buffer is used for
     * constant attributes(stride = 0), the other one is if the buffer is used on two streams
     * with different strides. In the 2nd case we might have to drop conversion entirely,
     * it is possible that the same bytes are once read as FLOAT2 and once as UBYTE4N.
     */
    if (!attrib->stride)
    {
        FIXME("%s used with stride 0, let's hope we get the vertex stride from somewhere else\n",
                debug_d3dformat(attrib->format_desc->format));
    }
    else if(attrib->stride != *stride_this_run && *stride_this_run)
    {
        FIXME("Got two concurrent strides, %d and %d\n", attrib->stride, *stride_this_run);
    }
    else
    {
        *stride_this_run = attrib->stride;
        if (This->stride != *stride_this_run)
        {
            /* We rely that this happens only on the first converted attribute that is found,
             * if at all. See above check
             */
            TRACE("Reconverting because converted attributes occur, and the stride changed\n");
            This->stride = *stride_this_run;
            HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, This->conversion_map);
            This->conversion_map = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                    sizeof(*This->conversion_map) * This->stride);
            ret = TRUE;
        }
    }

    data = (((DWORD_PTR)attrib->data) + offset) % This->stride;
    attrib_size = attrib->format_desc->component_count * attrib->format_desc->component_size;
    for (i = 0; i < attrib_size; ++i)
    {
        if (This->conversion_map[data + i] != conversion_type)
        {
            TRACE("Byte %ld in vertex changed\n", i + data);
            TRACE("It was type %d, is %d now\n", This->conversion_map[data + i], conversion_type);
            ret = TRUE;
            This->conversion_map[data + i] = conversion_type;
        }
    }

    return ret;
}

static BOOL buffer_check_attribute(struct wined3d_buffer *This,
        const struct wined3d_stream_info_element *attrib, const BOOL check_d3dcolor, const BOOL is_ffp_position,
        const BOOL is_ffp_color, DWORD *stride_this_run, BOOL *float16_used)
{
    BOOL ret = FALSE;
    WINED3DFORMAT format;

    /* Ignore attributes that do not have our vbo. After that check we can be sure that the attribute is
     * there, on nonexistent attribs the vbo is 0.
     */
    if (attrib->buffer_object != This->buffer_object) return FALSE;

    format = attrib->format_desc->format;
    /* Look for newly appeared conversion */
    if (!GL_SUPPORT(ARB_HALF_FLOAT_VERTEX) && (format == WINED3DFMT_R16G16_FLOAT || format == WINED3DFMT_R16G16B16A16_FLOAT))
    {
        ret = buffer_process_converted_attribute(This, CONV_FLOAT16_2, attrib, stride_this_run);

        if (is_ffp_position) FIXME("Test FLOAT16 fixed function processing positions\n");
        else if (is_ffp_color) FIXME("test FLOAT16 fixed function processing colors\n");
        *float16_used = TRUE;
    }
    else if (check_d3dcolor && format == WINED3DFMT_A8R8G8B8)
    {
        ret = buffer_process_converted_attribute(This, CONV_D3DCOLOR, attrib, stride_this_run);

        if (!is_ffp_color) FIXME("Test for non-color fixed function WINED3DFMT_A8R8G8B8 format\n");
    }
    else if (is_ffp_position && format == WINED3DFMT_R32G32B32A32_FLOAT)
    {
        ret = buffer_process_converted_attribute(This, CONV_POSITIONT, attrib, stride_this_run);
    }
    else if (This->conversion_map)
    {
        ret = buffer_process_converted_attribute(This, CONV_NONE, attrib, stride_this_run);
    }

    return ret;
}

static UINT *find_conversion_shift(struct wined3d_buffer *This,
        const struct wined3d_stream_info *strided, UINT stride)
{
    UINT *ret, i, j, shift, orig_type_size;

    if (!stride)
    {
        TRACE("No shift\n");
        return NULL;
    }

    This->conversion_stride = stride;
    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DWORD) * stride);
    for (i = 0; i < MAX_ATTRIBS; ++i)
    {
        WINED3DFORMAT format;

        if (strided->elements[i].buffer_object != This->buffer_object) continue;

        format = strided->elements[i].format_desc->format;
        if (format == WINED3DFMT_R16G16_FLOAT)
        {
            shift = 4;
        }
        else if (format == WINED3DFMT_R16G16B16A16_FLOAT)
        {
            shift = 8;
            /* Pre-shift the last 4 bytes in the FLOAT16_4 by 4 bytes - this makes FLOAT16_2 and FLOAT16_4 conversions
             * compatible
             */
            for (j = 4; j < 8; ++j)
            {
                ret[(DWORD_PTR)strided->elements[i].data + j] += 4;
            }
        }
        else
        {
            shift = 0;
        }
        This->conversion_stride += shift;

        if (shift)
        {
            orig_type_size = strided->elements[i].format_desc->component_count
                    * strided->elements[i].format_desc->component_size;
            for (j = (DWORD_PTR)strided->elements[i].data + orig_type_size; j < stride; ++j)
            {
                ret[j] += shift;
            }
        }
    }

    if (TRACE_ON(d3d))
    {
        TRACE("Dumping conversion shift:\n");
        for (i = 0; i < stride; ++i)
        {
            TRACE("[%d]", ret[i]);
        }
        TRACE("\n");
    }

    return ret;
}

static BOOL buffer_find_decl(struct wined3d_buffer *This)
{
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
    UINT stride_this_run = 0;
    BOOL float16_used = FALSE;
    BOOL ret = FALSE;
    unsigned int i;

    /* In d3d7 the vertex buffer declaration NEVER changes because it is stored in the d3d7 vertex buffer.
     * Once we have our declaration there is no need to look it up again. Index buffers also never need
     * conversion, so once the (empty) conversion structure is created don't bother checking again
     */
    if (This->flags & WINED3D_BUFFER_HASDESC)
    {
        if(((IWineD3DImpl *)device->wineD3D)->dxVersion == 7 ||
             This->resource.format_desc->format != WINED3DFMT_VERTEXDATA) return FALSE;
    }

    TRACE("Finding vertex buffer conversion information\n");
    /* Certain declaration types need some fixups before we can pass them to
     * opengl. This means D3DCOLOR attributes with fixed function vertex
     * processing, FLOAT4 POSITIONT with fixed function, and FLOAT16 if
     * GL_ARB_half_float_vertex is not supported.
     *
     * Note for d3d8 and d3d9:
     * The vertex buffer FVF doesn't help with finding them, we have to use
     * the decoded vertex declaration and pick the things that concern the
     * current buffer. A problem with this is that this can change between
     * draws, so we have to validate the information and reprocess the buffer
     * if it changes, and avoid false positives for performance reasons.
     * WineD3D doesn't even know the vertex buffer any more, it is managed
     * by the client libraries and passed to SetStreamSource and ProcessVertices
     * as needed.
     *
     * We have to distinguish between vertex shaders and fixed function to
     * pick the way we access the strided vertex information.
     *
     * This code sets up a per-byte array with the size of the detected
     * stride of the arrays in the buffer. For each byte we have a field
     * that marks the conversion needed on this byte. For example, the
     * following declaration with fixed function vertex processing:
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
     * Where in this example map P means 4 component position conversion, 0
     * means no conversion, F means FLOAT16_2 conversion and C means D3DCOLOR
     * conversion (red / blue swizzle).
     *
     * If we're doing conversion and the stride changes we have to reconvert
     * the whole buffer. Note that we do not mind if the semantic changes,
     * we only care for the conversion type. So if the NORMAL is replaced
     * with a TEXCOORD, nothing has to be done, or if the DIFFUSE is replaced
     * with a D3DCOLOR BLENDWEIGHT we can happily dismiss the change. Some
     * conversion types depend on the semantic as well, for example a FLOAT4
     * texcoord needs no conversion while a FLOAT4 positiont needs one
     */
    if (use_vs(device->stateBlock))
    {
        TRACE("vshader\n");
        /* If the current vertex declaration is marked for no half float conversion don't bother to
         * analyse the strided streams in depth, just set them up for no conversion. Return decl changed
         * if we used conversion before
         */
        if (!((IWineD3DVertexDeclarationImpl *) device->stateBlock->vertexDecl)->half_float_conv_needed)
        {
            if (This->conversion_map)
            {
                TRACE("Now using shaders without conversion, but conversion used before\n");
                HeapFree(GetProcessHeap(), 0, This->conversion_map);
                HeapFree(GetProcessHeap(), 0, This->conversion_shift);
                This->conversion_map = NULL;
                This->stride = 0;
                This->conversion_shift = NULL;
                This->conversion_stride = 0;
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
        for (i = 0; i < MAX_ATTRIBS; ++i)
        {
            ret = buffer_check_attribute(This, &device->strided_streams.elements[i],
                    FALSE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        }

        /* Recalculate the conversion shift map if the declaration has changed,
         * and we're using float16 conversion or used it on the last run
         */
        if (ret && (float16_used || This->conversion_map))
        {
            HeapFree(GetProcessHeap(), 0, This->conversion_shift);
            This->conversion_shift = find_conversion_shift(This, &device->strided_streams, This->stride);
        }
    }
    else
    {
        /* Fixed function is a bit trickier. We have to take care for D3DCOLOR types, FLOAT4 positions and of course
         * FLOAT16s if not supported. Also, we can't iterate over the array, so use macros to generate code for all
         * the attributes that our current fixed function pipeline implementation cares for.
         */
        BOOL support_d3dcolor = GL_SUPPORT(EXT_VERTEX_ARRAY_BGRA);
        ret = buffer_check_attribute(This, &device->strided_streams.elements[WINED3D_FFP_POSITION],
                TRUE, TRUE,  FALSE, &stride_this_run, &float16_used) || ret;
        ret = buffer_check_attribute(This, &device->strided_streams.elements[WINED3D_FFP_NORMAL],
                TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        ret = buffer_check_attribute(This, &device->strided_streams.elements[WINED3D_FFP_DIFFUSE],
                !support_d3dcolor, FALSE, TRUE,  &stride_this_run, &float16_used) || ret;
        ret = buffer_check_attribute(This, &device->strided_streams.elements[WINED3D_FFP_SPECULAR],
                !support_d3dcolor, FALSE, TRUE,  &stride_this_run, &float16_used) || ret;
        ret = buffer_check_attribute(This, &device->strided_streams.elements[WINED3D_FFP_TEXCOORD0],
                TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        ret = buffer_check_attribute(This, &device->strided_streams.elements[WINED3D_FFP_TEXCOORD1],
                TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        ret = buffer_check_attribute(This, &device->strided_streams.elements[WINED3D_FFP_TEXCOORD2],
                TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        ret = buffer_check_attribute(This, &device->strided_streams.elements[WINED3D_FFP_TEXCOORD3],
                TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        ret = buffer_check_attribute(This, &device->strided_streams.elements[WINED3D_FFP_TEXCOORD4],
                TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        ret = buffer_check_attribute(This, &device->strided_streams.elements[WINED3D_FFP_TEXCOORD5],
                TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        ret = buffer_check_attribute(This, &device->strided_streams.elements[WINED3D_FFP_TEXCOORD6],
                TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        ret = buffer_check_attribute(This, &device->strided_streams.elements[WINED3D_FFP_TEXCOORD7],
                TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;

        if (float16_used) FIXME("Float16 conversion used with fixed function vertex processing\n");
    }

    if (stride_this_run == 0 && This->conversion_map)
    {
        /* Sanity test */
        if (!ret) ERR("no converted attributes found, old conversion map exists, and no declaration change?\n");
        HeapFree(GetProcessHeap(), 0, This->conversion_map);
        This->conversion_map = NULL;
        This->stride = 0;
    }

    if (ret) TRACE("Conversion information changed\n");

    return ret;
}

static void buffer_check_buffer_object_size(struct wined3d_buffer *This)
{
    UINT size = This->conversion_stride ?
            This->conversion_stride * (This->resource.size / This->stride) : This->resource.size;
    if (This->buffer_object_size != size)
    {
        TRACE("Old size %u, creating new size %u\n", This->buffer_object_size, size);

        if(This->buffer_type_hint == GL_ELEMENT_ARRAY_BUFFER_ARB)
        {
            IWineD3DDeviceImpl_MarkStateDirty(This->resource.wineD3DDevice, STATE_INDEXBUFFER);
        }

        /* Rescue the data before resizing the buffer object if we do not have our backup copy */
        if(!(This->flags & WINED3D_BUFFER_DOUBLEBUFFER))
        {
            buffer_get_sysmem(This);
        }

        ENTER_GL();
        GL_EXTCALL(glBindBufferARB(This->buffer_type_hint, This->buffer_object));
        checkGLcall("glBindBufferARB");
        GL_EXTCALL(glBufferDataARB(This->buffer_type_hint, size, NULL, This->buffer_object_usage));
        This->buffer_object_size = size;
        checkGLcall("glBufferDataARB");
        LEAVE_GL();
    }
}

static inline void fixup_d3dcolor(DWORD *dst_color)
{
    DWORD src_color = *dst_color;

    /* Color conversion like in drawStridedSlow. watch out for little endianity
     * If we want that stuff to work on big endian machines too we have to consider more things
     *
     * 0xff000000: Alpha mask
     * 0x00ff0000: Blue mask
     * 0x0000ff00: Green mask
     * 0x000000ff: Red mask
     */
    *dst_color = 0;
    *dst_color |= (src_color & 0xff00ff00);         /* Alpha Green */
    *dst_color |= (src_color & 0x00ff0000) >> 16;   /* Red */
    *dst_color |= (src_color & 0x000000ff) << 16;   /* Blue */
}

static inline void fixup_transformed_pos(float *p)
{
    float x, y, z, w;

    /* rhw conversion like in drawStridedSlow */
    if (p[3] == 1.0 || ((p[3] < eps) && (p[3] > -eps)))
    {
        x = p[0];
        y = p[1];
        z = p[2];
        w = 1.0;
    }
    else
    {
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

const BYTE *buffer_get_memory(IWineD3DBuffer *iface, UINT offset, GLuint *buffer_object)
{
    struct wined3d_buffer *This = (struct wined3d_buffer *)iface;

    *buffer_object = This->buffer_object;
    if (!This->buffer_object)
    {
        if (This->flags & WINED3D_BUFFER_CREATEBO)
        {
            buffer_create_buffer_object(This);
            This->flags &= ~WINED3D_BUFFER_CREATEBO;
            if (This->buffer_object)
            {
                *buffer_object = This->buffer_object;
                return (const BYTE *)offset;
            }
        }
        return This->resource.allocatedMemory + offset;
    }
    else
    {
        return (const BYTE *)offset;
    }
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE buffer_QueryInterface(IWineD3DBuffer *iface,
        REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IWineD3DBuffer)
            || IsEqualGUID(riid, &IID_IWineD3DResource)
            || IsEqualGUID(riid, &IID_IWineD3DBase)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;

    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE buffer_AddRef(IWineD3DBuffer *iface)
{
    struct wined3d_buffer *This = (struct wined3d_buffer *)iface;
    ULONG refcount = InterlockedIncrement(&This->resource.ref);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    return refcount;
}

const BYTE *buffer_get_sysmem(struct wined3d_buffer *This)
{
    /* AllocatedMemory exists if the buffer is double buffered or has no buffer object at all */
    if(This->resource.allocatedMemory) return This->resource.allocatedMemory;

    This->resource.heapMemory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, This->resource.size + RESOURCE_ALIGNMENT);
    This->resource.allocatedMemory = (BYTE *)(((ULONG_PTR)This->resource.heapMemory + (RESOURCE_ALIGNMENT - 1)) & ~(RESOURCE_ALIGNMENT - 1));
    ENTER_GL();
    GL_EXTCALL(glBindBufferARB(This->buffer_type_hint, This->buffer_object));
    GL_EXTCALL(glGetBufferSubDataARB(This->buffer_type_hint, 0, This->resource.size, This->resource.allocatedMemory));
    LEAVE_GL();
    This->flags |= WINED3D_BUFFER_DOUBLEBUFFER;

    return This->resource.allocatedMemory;
}

static void STDMETHODCALLTYPE buffer_UnLoad(IWineD3DBuffer *iface)
{
    struct wined3d_buffer *This = (struct wined3d_buffer *)iface;

    TRACE("iface %p\n", iface);

    if (This->buffer_object)
    {
        IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;

        ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);

        /* Download the buffer, but don't permanently enable double buffering */
        if(!(This->flags & WINED3D_BUFFER_DOUBLEBUFFER))
        {
            buffer_get_sysmem(This);
            This->flags &= ~WINED3D_BUFFER_DOUBLEBUFFER;
        }

        ENTER_GL();
        GL_EXTCALL(glDeleteBuffersARB(1, &This->buffer_object));
        checkGLcall("glDeleteBuffersARB");
        LEAVE_GL();
        This->buffer_object = 0;
        This->flags |= WINED3D_BUFFER_CREATEBO; /* Recreate the buffer object next load */
    }
}

static ULONG STDMETHODCALLTYPE buffer_Release(IWineD3DBuffer *iface)
{
    struct wined3d_buffer *This = (struct wined3d_buffer *)iface;
    ULONG refcount = InterlockedDecrement(&This->resource.ref);

    TRACE("%p decreasing refcount to %u\n", This, refcount);

    if (!refcount)
    {
        buffer_UnLoad(iface);
        resource_cleanup((IWineD3DResource *)iface);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refcount;
}

/* IWineD3DBase methods */

static HRESULT STDMETHODCALLTYPE buffer_GetParent(IWineD3DBuffer *iface, IUnknown **parent)
{
    return resource_get_parent((IWineD3DResource *)iface, parent);
}

/* IWineD3DResource methods */

static HRESULT STDMETHODCALLTYPE buffer_GetDevice(IWineD3DBuffer *iface, IWineD3DDevice **device)
{
    return resource_get_device((IWineD3DResource *)iface, device);
}

static HRESULT STDMETHODCALLTYPE buffer_SetPrivateData(IWineD3DBuffer *iface,
        REFGUID guid, const void *data, DWORD data_size, DWORD flags)
{
    return resource_set_private_data((IWineD3DResource *)iface, guid, data, data_size, flags);
}

static HRESULT STDMETHODCALLTYPE buffer_GetPrivateData(IWineD3DBuffer *iface,
        REFGUID guid, void *data, DWORD *data_size)
{
    return resource_get_private_data((IWineD3DResource *)iface, guid, data, data_size);
}

static HRESULT STDMETHODCALLTYPE buffer_FreePrivateData(IWineD3DBuffer *iface, REFGUID guid)
{
    return resource_free_private_data((IWineD3DResource *)iface, guid);
}

static DWORD STDMETHODCALLTYPE buffer_SetPriority(IWineD3DBuffer *iface, DWORD priority)
{
    return resource_set_priority((IWineD3DResource *)iface, priority);
}

static DWORD STDMETHODCALLTYPE buffer_GetPriority(IWineD3DBuffer *iface)
{
    return resource_get_priority((IWineD3DResource *)iface);
}

static void STDMETHODCALLTYPE buffer_PreLoad(IWineD3DBuffer *iface)
{
    struct wined3d_buffer *This = (struct wined3d_buffer *)iface;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
    UINT start = 0, end = 0, vertices;
    BOOL decl_changed = FALSE;
    unsigned int i, j;
    BYTE *data;

    TRACE("iface %p\n", iface);

    if (!This->buffer_object)
    {
        /* TODO: Make converting independent from VBOs */
        if (This->flags & WINED3D_BUFFER_CREATEBO)
        {
            buffer_create_buffer_object(This);
            This->flags &= ~WINED3D_BUFFER_CREATEBO;
        }
        else
        {
            return; /* Not doing any conversion */
        }
    }

    /* Reading the declaration makes only sense if the stateblock is finalized and the buffer bound to a stream */
    if (device->isInDraw && This->bind_count > 0)
    {
        decl_changed = buffer_find_decl(This);
        This->flags |= WINED3D_BUFFER_HASDESC;
    }

    if (!decl_changed && !(This->flags & WINED3D_BUFFER_HASDESC && This->flags & WINED3D_BUFFER_DIRTY)) return;

    /* If applications change the declaration over and over, reconverting all the time is a huge
     * performance hit. So count the declaration changes and release the VBO if there are too many
     * of them (and thus stop converting)
     */
    if (decl_changed)
    {
        ++This->conversion_count;
        This->draw_count = 0;

        if (This->conversion_count > VB_MAXDECLCHANGES)
        {
            FIXME("Too many declaration changes, stopping converting\n");
            ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
            ENTER_GL();
            GL_EXTCALL(glDeleteBuffersARB(1, &This->buffer_object));
            checkGLcall("glDeleteBuffersARB");
            LEAVE_GL();
            This->buffer_object = 0;
            HeapFree(GetProcessHeap(), 0, This->conversion_shift);

            /* The stream source state handler might have read the memory of the vertex buffer already
             * and got the memory in the vbo which is not valid any longer. Dirtify the stream source
             * to force a reload. This happens only once per changed vertexbuffer and should occur rather
             * rarely
             */
            IWineD3DDeviceImpl_MarkStateDirty(device, STATE_STREAMSRC);

            return;
        }
        buffer_check_buffer_object_size(This);
    }
    else
    {
        /* However, it is perfectly fine to change the declaration every now and then. We don't want a game that
         * changes it every minute drop the VBO after VB_MAX_DECL_CHANGES minutes. So count draws without
         * decl changes and reset the decl change count after a specific number of them
         */
        ++This->draw_count;
        if (This->draw_count > VB_RESETDECLCHANGE) This->conversion_count = 0;
    }

    if (decl_changed)
    {
        /* The declaration changed, reload the whole buffer */
        WARN("Reloading buffer because of decl change\n");
        start = 0;
        end = This->resource.size;
    }
    else
    {
        /* No decl change, but dirty data, reload the changed stuff */
        if (This->conversion_shift)
        {
            if (This->dirty_start != 0 || This->dirty_end != 0)
            {
                FIXME("Implement partial buffer loading with shifted conversion\n");
            }
        }
        start = This->dirty_start;
        end = This->dirty_end;
    }

    /* Mark the buffer clean */
    This->flags &= ~WINED3D_BUFFER_DIRTY;
    This->dirty_start = 0;
    This->dirty_end = 0;

    if(This->buffer_type_hint == GL_ELEMENT_ARRAY_BUFFER_ARB)
    {
        IWineD3DDeviceImpl_MarkStateDirty(This->resource.wineD3DDevice, STATE_INDEXBUFFER);
    }

    if (!This->conversion_map)
    {
        /* That means that there is nothing to fixup. Just upload from This->resource.allocatedMemory
         * directly into the vbo. Do not free the system memory copy because drawPrimitive may need it if
         * the stride is 0, for instancing emulation, vertex blending emulation or shader emulation.
         */
        TRACE("No conversion needed\n");

        /* Nothing to do because we locked directly into the vbo */
        if(!(This->flags & WINED3D_BUFFER_DOUBLEBUFFER)) return;

        if (!device->isInDraw)
        {
            ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
        }
        ENTER_GL();
        GL_EXTCALL(glBindBufferARB(This->buffer_type_hint, This->buffer_object));
        checkGLcall("glBindBufferARB");
        GL_EXTCALL(glBufferSubDataARB(This->buffer_type_hint, start, end-start, This->resource.allocatedMemory + start));
        checkGLcall("glBufferSubDataARB");
        LEAVE_GL();
        return;
    }

    if(!(This->flags & WINED3D_BUFFER_DOUBLEBUFFER))
    {
        buffer_get_sysmem(This);
    }

    /* Now for each vertex in the buffer that needs conversion */
    vertices = This->resource.size / This->stride;

    if (This->conversion_shift)
    {
        TRACE("Shifted conversion\n");
        data = HeapAlloc(GetProcessHeap(), 0, vertices * This->conversion_stride);

        for (i = start / This->stride; i < min((end / This->stride) + 1, vertices); ++i)
        {
            for (j = 0; j < This->stride; ++j)
            {
                switch(This->conversion_map[j])
                {
                    case CONV_NONE:
                        data[This->conversion_stride * i + j + This->conversion_shift[j]]
                                = This->resource.allocatedMemory[This->stride * i + j];
                        break;

                    case CONV_FLOAT16_2:
                    {
                        float *out = (float *)(&data[This->conversion_stride * i + j + This->conversion_shift[j]]);
                        const WORD *in = (WORD *)(&This->resource.allocatedMemory[i * This->stride + j]);

                        out[1] = float_16_to_32(in + 1);
                        out[0] = float_16_to_32(in + 0);
                        j += 3;    /* Skip 3 additional bytes,as a FLOAT16_2 has 4 bytes */
                        break;
                    }

                    default:
                        FIXME("Unimplemented conversion %d in shifted conversion\n", This->conversion_map[j]);
                        break;
                }
            }
        }

        ENTER_GL();
        GL_EXTCALL(glBindBufferARB(This->buffer_type_hint, This->buffer_object));
        checkGLcall("glBindBufferARB");
        GL_EXTCALL(glBufferSubDataARB(This->buffer_type_hint, 0, vertices * This->conversion_stride, data));
        checkGLcall("glBufferSubDataARB");
        LEAVE_GL();
    }
    else
    {
        data = HeapAlloc(GetProcessHeap(), 0, This->resource.size);
        memcpy(data + start, This->resource.allocatedMemory + start, end - start);
        for (i = start / This->stride; i < min((end / This->stride) + 1, vertices); ++i)
        {
            for (j = 0; j < This->stride; ++j)
            {
                switch(This->conversion_map[j])
                {
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
                        FIXME("Unimplemented conversion %d in shifted conversion\n", This->conversion_map[j]);
                }
            }
        }

        ENTER_GL();
        GL_EXTCALL(glBindBufferARB(This->buffer_type_hint, This->buffer_object));
        checkGLcall("glBindBufferARB");
        GL_EXTCALL(glBufferSubDataARB(This->buffer_type_hint, start, end - start, data + start));
        checkGLcall("glBufferSubDataARB");
        LEAVE_GL();
    }

    HeapFree(GetProcessHeap(), 0, data);
}

static WINED3DRESOURCETYPE STDMETHODCALLTYPE buffer_GetType(IWineD3DBuffer *iface)
{
    return resource_get_type((IWineD3DResource *)iface);
}

/* IWineD3DBuffer methods */

static HRESULT STDMETHODCALLTYPE buffer_Map(IWineD3DBuffer *iface, UINT offset, UINT size, BYTE **data, DWORD flags)
{
    struct wined3d_buffer *This = (struct wined3d_buffer *)iface;
    LONG count;

    TRACE("iface %p, offset %u, size %u, data %p, flags %#x\n", iface, offset, size, data, flags);

    count = InterlockedIncrement(&This->lock_count);

    if (This->flags & WINED3D_BUFFER_DIRTY)
    {
        if (This->dirty_start > offset) This->dirty_start = offset;

        if (size)
        {
            if (This->dirty_end < offset + size) This->dirty_end = offset + size;
        }
        else
        {
            This->dirty_end = This->resource.size;
        }
    }
    else
    {
        This->dirty_start = offset;
        if (size) This->dirty_end = offset + size;
        else This->dirty_end = This->resource.size;
    }

    if(!(This->flags & WINED3D_BUFFER_DOUBLEBUFFER) && This->buffer_object)
    {
        if(count == 1)
        {
            IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;

            if(This->buffer_type_hint == GL_ELEMENT_ARRAY_BUFFER_ARB)
            {
                IWineD3DDeviceImpl_MarkStateDirty(This->resource.wineD3DDevice, STATE_INDEXBUFFER);
            }

            ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
            ENTER_GL();
            GL_EXTCALL(glBindBufferARB(This->buffer_type_hint, This->buffer_object));
            This->resource.allocatedMemory = GL_EXTCALL(glMapBufferARB(This->buffer_type_hint, GL_READ_WRITE_ARB));
            LEAVE_GL();
        }
    }
    else
    {
        This->flags |= WINED3D_BUFFER_DIRTY;
    }

    *data = This->resource.allocatedMemory + offset;

    TRACE("Returning memory at %p (base %p, offset %u)\n", *data, This->resource.allocatedMemory, offset);
    /* TODO: check Flags compatibility with This->currentDesc.Usage (see MSDN) */

    return WINED3D_OK;
}

static HRESULT STDMETHODCALLTYPE buffer_Unmap(IWineD3DBuffer *iface)
{
    struct wined3d_buffer *This = (struct wined3d_buffer *)iface;

    TRACE("(%p)\n", This);

    if (InterlockedDecrement(&This->lock_count))
    {
        /* Delay loading the buffer until everything is unlocked */
        TRACE("Ignoring unlock\n");
        return WINED3D_OK;
    }

    if(!(This->flags & WINED3D_BUFFER_DOUBLEBUFFER) && This->buffer_object)
    {
        IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;

        if(This->buffer_type_hint == GL_ELEMENT_ARRAY_BUFFER_ARB)
        {
            IWineD3DDeviceImpl_MarkStateDirty(This->resource.wineD3DDevice, STATE_INDEXBUFFER);
        }

        ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
        ENTER_GL();
        GL_EXTCALL(glBindBufferARB(This->buffer_type_hint, This->buffer_object));
        GL_EXTCALL(glUnmapBufferARB(This->buffer_type_hint));
        LEAVE_GL();

        This->resource.allocatedMemory = NULL;
    }
    else if (This->flags & WINED3D_BUFFER_HASDESC)
    {
        buffer_PreLoad(iface);
    }

    return WINED3D_OK;
}

static HRESULT STDMETHODCALLTYPE buffer_GetDesc(IWineD3DBuffer *iface, WINED3DBUFFER_DESC *desc)
{
    struct wined3d_buffer *This = (struct wined3d_buffer *)iface;

    TRACE("(%p)\n", This);

    desc->Type = This->resource.resourceType;
    desc->Usage = This->resource.usage;
    desc->Pool = This->resource.pool;
    desc->Size = This->resource.size;

    return WINED3D_OK;
}

const struct IWineD3DBufferVtbl wined3d_buffer_vtbl =
{
    /* IUnknown methods */
    buffer_QueryInterface,
    buffer_AddRef,
    buffer_Release,
    /* IWineD3DBase methods */
    buffer_GetParent,
    /* IWineD3DResource methods */
    buffer_GetDevice,
    buffer_SetPrivateData,
    buffer_GetPrivateData,
    buffer_FreePrivateData,
    buffer_SetPriority,
    buffer_GetPriority,
    buffer_PreLoad,
    buffer_UnLoad,
    buffer_GetType,
    /* IWineD3DBuffer methods */
    buffer_Map,
    buffer_Unmap,
    buffer_GetDesc,
};
