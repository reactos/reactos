/*
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2007-2010 Stefan Dösinger for CodeWeavers
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

#define VB_MAXDECLCHANGES     100     /* After that number of decl changes we stop converting */
#define VB_RESETDECLCHANGE    1000    /* Reset the decl changecount after that number of draws */
#define VB_MAXFULLCONVERSIONS 5       /* Number of full conversions before we stop converting */
#define VB_RESETFULLCONVS     20      /* Reset full conversion counts after that number of draws */

static inline BOOL buffer_add_dirty_area(struct wined3d_buffer *This, UINT offset, UINT size)
{
    if (!This->buffer_object) return TRUE;

    if (This->maps_size <= This->modified_areas)
    {
        void *new = HeapReAlloc(GetProcessHeap(), 0, This->maps,
                                This->maps_size * 2 * sizeof(*This->maps));
        if (!new)
        {
            ERR("Out of memory\n");
            return FALSE;
        }
        else
        {
            This->maps = new;
            This->maps_size *= 2;
        }
    }

    if(offset > This->resource.size || offset + size > This->resource.size)
    {
        WARN("Invalid range dirtified, marking entire buffer dirty\n");
        offset = 0;
        size = This->resource.size;
    }
    else if(!offset && !size)
    {
        size = This->resource.size;
    }

    This->maps[This->modified_areas].offset = offset;
    This->maps[This->modified_areas].size = size;
    This->modified_areas++;
    return TRUE;
}

static inline void buffer_clear_dirty_areas(struct wined3d_buffer *This)
{
    This->modified_areas = 0;
}

static inline BOOL buffer_is_dirty(struct wined3d_buffer *This)
{
    return This->modified_areas != 0;
}

static inline BOOL buffer_is_fully_dirty(struct wined3d_buffer *This)
{
    unsigned int i;

    for(i = 0; i < This->modified_areas; i++)
    {
        if(This->maps[i].offset == 0 && This->maps[i].size == This->resource.size)
        {
            return TRUE;
        }
    }
    return FALSE;
}

/* Context activation is done by the caller */
static void delete_gl_buffer(struct wined3d_buffer *This, const struct wined3d_gl_info *gl_info)
{
    if(!This->buffer_object) return;

    ENTER_GL();
    GL_EXTCALL(glDeleteBuffersARB(1, &This->buffer_object));
    checkGLcall("glDeleteBuffersARB");
    LEAVE_GL();
    This->buffer_object = 0;

    if(This->query)
    {
        wined3d_event_query_destroy(This->query);
        This->query = NULL;
    }
    This->flags &= ~WINED3D_BUFFER_APPLESYNC;
}

/* Context activation is done by the caller. */
static void buffer_create_buffer_object(struct wined3d_buffer *This, const struct wined3d_gl_info *gl_info)
{
    GLenum error, gl_usage;

    TRACE("Creating an OpenGL vertex buffer object for IWineD3DVertexBuffer %p Usage(%s)\n",
            This, debug_d3dusage(This->resource.usage));

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
        LEAVE_GL();
        goto fail;
    }

    if(This->buffer_type_hint == GL_ELEMENT_ARRAY_BUFFER_ARB)
    {
        IWineD3DDeviceImpl_MarkStateDirty(This->resource.device, STATE_INDEXBUFFER);
    }
    GL_EXTCALL(glBindBufferARB(This->buffer_type_hint, This->buffer_object));
    error = glGetError();
    if (error != GL_NO_ERROR)
    {
        ERR("Failed to bind the VBO with error %s (%#x)\n", debug_glerror(error), error);
        LEAVE_GL();
        goto fail;
    }

    /* Don't use static, because dx apps tend to update the buffer
     * quite often even if they specify 0 usage.
     */
    if(This->resource.usage & WINED3DUSAGE_DYNAMIC)
    {
        TRACE("Gl usage = GL_STREAM_DRAW_ARB\n");
        gl_usage = GL_STREAM_DRAW_ARB;

        if(gl_info->supported[APPLE_FLUSH_BUFFER_RANGE])
        {
            GL_EXTCALL(glBufferParameteriAPPLE(This->buffer_type_hint, GL_BUFFER_FLUSHING_UNMAP_APPLE, GL_FALSE));
            checkGLcall("glBufferParameteriAPPLE(This->buffer_type_hint, GL_BUFFER_FLUSHING_UNMAP_APPLE, GL_FALSE)");
            This->flags |= WINED3D_BUFFER_FLUSH;

            GL_EXTCALL(glBufferParameteriAPPLE(This->buffer_type_hint, GL_BUFFER_SERIALIZED_MODIFY_APPLE, GL_FALSE));
            checkGLcall("glBufferParameteriAPPLE(This->buffer_type_hint, GL_BUFFER_SERIALIZED_MODIFY_APPLE, GL_FALSE)");
            This->flags |= WINED3D_BUFFER_APPLESYNC;
        }
        /* No setup is needed here for GL_ARB_map_buffer_range */
    }
    else
    {
        TRACE("Gl usage = GL_DYNAMIC_DRAW_ARB\n");
        gl_usage = GL_DYNAMIC_DRAW_ARB;
    }

    /* Reserve memory for the buffer. The amount of data won't change
     * so we are safe with calling glBufferData once and
     * calling glBufferSubData on updates. Upload the actual data in case
     * we're not double buffering, so we can release the heap mem afterwards
     */
    GL_EXTCALL(glBufferDataARB(This->buffer_type_hint, This->resource.size, This->resource.allocatedMemory, gl_usage));
    error = glGetError();
    LEAVE_GL();
    if (error != GL_NO_ERROR)
    {
        ERR("glBufferDataARB failed with error %s (%#x)\n", debug_glerror(error), error);
        goto fail;
    }

    This->buffer_object_size = This->resource.size;
    This->buffer_object_usage = gl_usage;

    if(This->flags & WINED3D_BUFFER_DOUBLEBUFFER)
    {
        if(!buffer_add_dirty_area(This, 0, 0))
        {
            ERR("buffer_add_dirty_area failed, this is not expected\n");
            goto fail;
        }
    }
    else
    {
        HeapFree(GetProcessHeap(), 0, This->resource.heapMemory);
        This->resource.allocatedMemory = NULL;
        This->resource.heapMemory = NULL;
    }

    return;

fail:
    /* Clean up all vbo init, but continue because we can work without a vbo :-) */
    ERR("Failed to create a vertex buffer object. Continuing, but performance issues may occur\n");
    delete_gl_buffer(This, gl_info);
    buffer_clear_dirty_areas(This);
}

static BOOL buffer_process_converted_attribute(struct wined3d_buffer *This,
        const enum wined3d_buffer_conversion_type conversion_type,
        const struct wined3d_stream_info_element *attrib, DWORD *stride_this_run)
{
    DWORD attrib_size;
    BOOL ret = FALSE;
    unsigned int i;
    DWORD offset = This->resource.device->stateBlock->streamOffset[attrib->stream_idx];
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
        DWORD_PTR idx = (data + i) % This->stride;
        if (This->conversion_map[idx] != conversion_type)
        {
            TRACE("Byte %ld in vertex changed\n", idx);
            TRACE("It was type %d, is %d now\n", This->conversion_map[idx], conversion_type);
            ret = TRUE;
            This->conversion_map[idx] = conversion_type;
        }
    }

    return ret;
}

static BOOL buffer_check_attribute(struct wined3d_buffer *This, const struct wined3d_stream_info *si,
        UINT attrib_idx, const BOOL check_d3dcolor, const BOOL is_ffp_position, const BOOL is_ffp_color,
        DWORD *stride_this_run, BOOL *float16_used)
{
    const struct wined3d_stream_info_element *attrib = &si->elements[attrib_idx];
    IWineD3DDeviceImpl *device = This->resource.device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    BOOL ret = FALSE;
    WINED3DFORMAT format;

    /* Ignore attributes that do not have our vbo. After that check we can be sure that the attribute is
     * there, on nonexistent attribs the vbo is 0.
     */
    if (!(si->use_map & (1 << attrib_idx))
            || attrib->buffer_object != This->buffer_object)
        return FALSE;

    format = attrib->format_desc->format;
    /* Look for newly appeared conversion */
    if (!gl_info->supported[ARB_HALF_FLOAT_VERTEX]
            && (format == WINED3DFMT_R16G16_FLOAT || format == WINED3DFMT_R16G16B16A16_FLOAT))
    {
        ret = buffer_process_converted_attribute(This, CONV_FLOAT16_2, attrib, stride_this_run);

        if (is_ffp_position) FIXME("Test FLOAT16 fixed function processing positions\n");
        else if (is_ffp_color) FIXME("test FLOAT16 fixed function processing colors\n");
        *float16_used = TRUE;
    }
    else if (check_d3dcolor && format == WINED3DFMT_B8G8R8A8_UNORM)
    {
        ret = buffer_process_converted_attribute(This, CONV_D3DCOLOR, attrib, stride_this_run);

        if (!is_ffp_color) FIXME("Test for non-color fixed function WINED3DFMT_B8G8R8A8_UNORM format\n");
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

        if (!(strided->use_map & (1 << i)) || strided->elements[i].buffer_object != This->buffer_object) continue;

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
    IWineD3DDeviceImpl *device = This->resource.device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    const struct wined3d_stream_info *si = &device->strided_streams;
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
        if(This->resource.usage & WINED3DUSAGE_STATICDECL) return FALSE;
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
            ret = buffer_check_attribute(This, si, i, FALSE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        }

        /* Recalculate the conversion shift map if the declaration has changed,
         * and we're using float16 conversion or used it on the last run
         */
        if (ret && (float16_used || This->conversion_map))
        {
            HeapFree(GetProcessHeap(), 0, This->conversion_shift);
            This->conversion_shift = find_conversion_shift(This, si, This->stride);
        }
    }
    else
    {
        /* Fixed function is a bit trickier. We have to take care for D3DCOLOR types, FLOAT4 positions and of course
         * FLOAT16s if not supported. Also, we can't iterate over the array, so use macros to generate code for all
         * the attributes that our current fixed function pipeline implementation cares for.
         */
        BOOL support_d3dcolor = gl_info->supported[ARB_VERTEX_ARRAY_BGRA];
        ret = buffer_check_attribute(This, si, WINED3D_FFP_POSITION,
                TRUE, TRUE,  FALSE, &stride_this_run, &float16_used) || ret;
        ret = buffer_check_attribute(This, si, WINED3D_FFP_NORMAL,
                TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        ret = buffer_check_attribute(This, si, WINED3D_FFP_DIFFUSE,
                !support_d3dcolor, FALSE, TRUE,  &stride_this_run, &float16_used) || ret;
        ret = buffer_check_attribute(This, si, WINED3D_FFP_SPECULAR,
                !support_d3dcolor, FALSE, TRUE,  &stride_this_run, &float16_used) || ret;
        ret = buffer_check_attribute(This, si, WINED3D_FFP_TEXCOORD0,
                TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        ret = buffer_check_attribute(This, si, WINED3D_FFP_TEXCOORD1,
                TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        ret = buffer_check_attribute(This, si, WINED3D_FFP_TEXCOORD2,
                TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        ret = buffer_check_attribute(This, si, WINED3D_FFP_TEXCOORD3,
                TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        ret = buffer_check_attribute(This, si, WINED3D_FFP_TEXCOORD4,
                TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        ret = buffer_check_attribute(This, si, WINED3D_FFP_TEXCOORD5,
                TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        ret = buffer_check_attribute(This, si, WINED3D_FFP_TEXCOORD6,
                TRUE, FALSE, FALSE, &stride_this_run, &float16_used) || ret;
        ret = buffer_check_attribute(This, si, WINED3D_FFP_TEXCOORD7,
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

/* Context activation is done by the caller. */
static void buffer_check_buffer_object_size(struct wined3d_buffer *This, const struct wined3d_gl_info *gl_info)
{
    UINT size = This->conversion_stride ?
            This->conversion_stride * (This->resource.size / This->stride) : This->resource.size;
    if (This->buffer_object_size != size)
    {
        TRACE("Old size %u, creating new size %u\n", This->buffer_object_size, size);

        if(This->buffer_type_hint == GL_ELEMENT_ARRAY_BUFFER_ARB)
        {
            IWineD3DDeviceImpl_MarkStateDirty(This->resource.device, STATE_INDEXBUFFER);
        }

        /* Rescue the data before resizing the buffer object if we do not have our backup copy */
        if(!(This->flags & WINED3D_BUFFER_DOUBLEBUFFER))
        {
            buffer_get_sysmem(This, gl_info);
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
    /* rhw conversion like in position_float4(). */
    if (p[3] != 1.0f && p[3] != 0.0f)
    {
        float w = 1.0f / p[3];
        p[0] *= w;
        p[1] *= w;
        p[2] *= w;
        p[3] = w;
    }
}

/* Context activation is done by the caller. */
const BYTE *buffer_get_memory(IWineD3DBuffer *iface, const struct wined3d_gl_info *gl_info, GLuint *buffer_object)
{
    struct wined3d_buffer *This = (struct wined3d_buffer *)iface;

    *buffer_object = This->buffer_object;
    if (!This->buffer_object)
    {
        if (This->flags & WINED3D_BUFFER_CREATEBO)
        {
            buffer_create_buffer_object(This, gl_info);
            This->flags &= ~WINED3D_BUFFER_CREATEBO;
            if (This->buffer_object)
            {
                *buffer_object = This->buffer_object;
                return NULL;
            }
        }
        return This->resource.allocatedMemory;
    }
    else
    {
        return NULL;
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

/* Context activation is done by the caller. */
BYTE *buffer_get_sysmem(struct wined3d_buffer *This, const struct wined3d_gl_info *gl_info)
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
        IWineD3DDeviceImpl *device = This->resource.device;
        struct wined3d_context *context;

        context = context_acquire(device, NULL);

        /* Download the buffer, but don't permanently enable double buffering */
        if(!(This->flags & WINED3D_BUFFER_DOUBLEBUFFER))
        {
            buffer_get_sysmem(This, context->gl_info);
            This->flags &= ~WINED3D_BUFFER_DOUBLEBUFFER;
        }

        delete_gl_buffer(This, context->gl_info);
        This->flags |= WINED3D_BUFFER_CREATEBO; /* Recreate the buffer object next load */
        buffer_clear_dirty_areas(This);

        context_release(context);

        HeapFree(GetProcessHeap(), 0, This->conversion_shift);
        This->conversion_shift = NULL;
        HeapFree(GetProcessHeap(), 0, This->conversion_map);
        This->conversion_map = NULL;
        This->stride = 0;
        This->conversion_stride = 0;
        This->flags &= ~WINED3D_BUFFER_HASDESC;
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
        This->resource.parent_ops->wined3d_object_destroyed(This->resource.parent);
        HeapFree(GetProcessHeap(), 0, This->maps);
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

/* The caller provides a context and binds the buffer */
static void buffer_sync_apple(struct wined3d_buffer *This, DWORD flags, const struct wined3d_gl_info *gl_info)
{
    enum wined3d_event_query_result ret;

    /* No fencing needs to be done if the app promises not to overwrite
     * existing data */
    if(flags & WINED3DLOCK_NOOVERWRITE) return;
    if(flags & WINED3DLOCK_DISCARD)
    {
        ENTER_GL();
        GL_EXTCALL(glBufferDataARB(This->buffer_type_hint, This->resource.size, NULL, This->buffer_object_usage));
        checkGLcall("glBufferDataARB\n");
        LEAVE_GL();
        return;
    }

    if(!This->query)
    {
        TRACE("Creating event query for buffer %p\n", This);

        if (!wined3d_event_query_supported(gl_info))
        {
            FIXME("Event queries not supported, dropping async buffer locks.\n");
            goto drop_query;
        }

        This->query = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*This->query));
        if (!This->query)
        {
            ERR("Failed to allocate event query memory, dropping async buffer locks.\n");
            goto drop_query;
        }

        /* Since we don't know about old draws a glFinish is needed once */
        wglFinish();
        return;
    }
    TRACE("Synchronizing buffer %p\n", This);
    ret = wined3d_event_query_finish(This->query, This->resource.device);
    switch(ret)
    {
        case WINED3D_EVENT_QUERY_NOT_STARTED:
        case WINED3D_EVENT_QUERY_OK:
            /* All done */
            return;

        case WINED3D_EVENT_QUERY_WRONG_THREAD:
            WARN("Cannot synchronize buffer lock due to a thread conflict\n");
            goto drop_query;

        default:
            ERR("wined3d_event_query_finish returned %u, dropping async buffer locks\n", ret);
            goto drop_query;
    }

drop_query:
    if(This->query)
    {
        wined3d_event_query_destroy(This->query);
        This->query = NULL;
    }

    wglFinish();
    ENTER_GL();
    GL_EXTCALL(glBufferParameteriAPPLE(This->buffer_type_hint, GL_BUFFER_SERIALIZED_MODIFY_APPLE, GL_TRUE));
    checkGLcall("glBufferParameteriAPPLE(This->buffer_type_hint, GL_BUFFER_SERIALIZED_MODIFY_APPLE, GL_TRUE)");
    LEAVE_GL();
    This->flags &= ~WINED3D_BUFFER_APPLESYNC;
}

/* The caller provides a GL context */
static void buffer_direct_upload(struct wined3d_buffer *This, const struct wined3d_gl_info *gl_info, DWORD flags)
{
        BYTE *map;
        UINT start = 0, len = 0;

        ENTER_GL();
        GL_EXTCALL(glBindBufferARB(This->buffer_type_hint, This->buffer_object));
        checkGLcall("glBindBufferARB");
        if (gl_info->supported[ARB_MAP_BUFFER_RANGE])
        {
            GLbitfield mapflags;
            mapflags = GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT;
            if (flags & WINED3D_BUFFER_DISCARD)
            {
                mapflags |= GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
            }
            else if (flags & WINED3D_BUFFER_NOSYNC)
            {
                mapflags |= GL_MAP_UNSYNCHRONIZED_BIT;
            }
            map = GL_EXTCALL(glMapBufferRange(This->buffer_type_hint, 0,
                                              This->resource.size, mapflags));
            checkGLcall("glMapBufferRange");
        }
        else
        {
            if (This->flags & WINED3D_BUFFER_APPLESYNC)
            {
                DWORD syncflags = 0;
                if (flags & WINED3D_BUFFER_DISCARD) syncflags |= WINED3DLOCK_DISCARD;
                if (flags & WINED3D_BUFFER_NOSYNC) syncflags |= WINED3DLOCK_NOOVERWRITE;
                LEAVE_GL();
                buffer_sync_apple(This, syncflags, gl_info);
                ENTER_GL();
            }
            map = GL_EXTCALL(glMapBufferARB(This->buffer_type_hint, GL_WRITE_ONLY_ARB));
            checkGLcall("glMapBufferARB");
        }
        if (!map)
        {
            LEAVE_GL();
            ERR("Failed to map opengl buffer\n");
            return;
        }

        while(This->modified_areas)
        {
            This->modified_areas--;
            start = This->maps[This->modified_areas].offset;
            len = This->maps[This->modified_areas].size;

            memcpy(map + start, This->resource.allocatedMemory + start, len);

            if (gl_info->supported[ARB_MAP_BUFFER_RANGE])
            {
                GL_EXTCALL(glFlushMappedBufferRange(This->buffer_type_hint, start, len));
                checkGLcall("glFlushMappedBufferRange");
            }
            else if (This->flags & WINED3D_BUFFER_FLUSH)
            {
                GL_EXTCALL(glFlushMappedBufferRangeAPPLE(This->buffer_type_hint, start, len));
                checkGLcall("glFlushMappedBufferRangeAPPLE");
            }
        }
        GL_EXTCALL(glUnmapBufferARB(This->buffer_type_hint));
        checkGLcall("glUnmapBufferARB");
        LEAVE_GL();
}

static void STDMETHODCALLTYPE buffer_PreLoad(IWineD3DBuffer *iface)
{
    struct wined3d_buffer *This = (struct wined3d_buffer *)iface;
    IWineD3DDeviceImpl *device = This->resource.device;
    UINT start = 0, end = 0, len = 0, vertices;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    BOOL decl_changed = FALSE;
    unsigned int i, j;
    BYTE *data;
    DWORD flags = This->flags & (WINED3D_BUFFER_NOSYNC | WINED3D_BUFFER_DISCARD);

    TRACE("iface %p\n", iface);
    This->flags &= ~(WINED3D_BUFFER_NOSYNC | WINED3D_BUFFER_DISCARD);

    context = context_acquire(device, NULL);
    gl_info = context->gl_info;

    if (!This->buffer_object)
    {
        /* TODO: Make converting independent from VBOs */
        if (This->flags & WINED3D_BUFFER_CREATEBO)
        {
            buffer_create_buffer_object(This, gl_info);
            This->flags &= ~WINED3D_BUFFER_CREATEBO;
        }
        else
        {
            /* Not doing any conversion */
            goto end;
        }
    }

    /* Reading the declaration makes only sense if the stateblock is finalized and the buffer bound to a stream */
    if (device->isInDraw && This->bind_count > 0)
    {
        decl_changed = buffer_find_decl(This);
        This->flags |= WINED3D_BUFFER_HASDESC;
    }

    if (!decl_changed && !(This->flags & WINED3D_BUFFER_HASDESC && buffer_is_dirty(This)))
    {
        context_release(context);
        ++This->draw_count;
        if (This->draw_count > VB_RESETDECLCHANGE) This->decl_change_count = 0;
        if (This->draw_count > VB_RESETFULLCONVS) This->full_conversion_count = 0;
        return;
    }

    /* If applications change the declaration over and over, reconverting all the time is a huge
     * performance hit. So count the declaration changes and release the VBO if there are too many
     * of them (and thus stop converting)
     */
    if (decl_changed)
    {
        ++This->decl_change_count;
        This->draw_count = 0;

        if (This->decl_change_count > VB_MAXDECLCHANGES ||
            (This->conversion_map && (This->resource.usage & WINED3DUSAGE_DYNAMIC)))
        {
            FIXME("Too many declaration changes or converting dynamic buffer, stopping converting\n");

            IWineD3DBuffer_UnLoad(iface);
            This->flags &= ~WINED3D_BUFFER_CREATEBO;

            /* The stream source state handler might have read the memory of the vertex buffer already
             * and got the memory in the vbo which is not valid any longer. Dirtify the stream source
             * to force a reload. This happens only once per changed vertexbuffer and should occur rather
             * rarely
             */
            IWineD3DDeviceImpl_MarkStateDirty(device, STATE_STREAMSRC);
            goto end;
        }
        buffer_check_buffer_object_size(This, gl_info);

        /* The declaration changed, reload the whole buffer */
        WARN("Reloading buffer because of decl change\n");
        buffer_clear_dirty_areas(This);
        if(!buffer_add_dirty_area(This, 0, 0))
        {
            ERR("buffer_add_dirty_area failed, this is not expected\n");
            return;
        }
        /* Avoid unfenced updates, we might overwrite more areas of the buffer than the application
         * cleared for unsynchronized updates
         */
        flags = 0;
    }
    else
    {
        /* However, it is perfectly fine to change the declaration every now and then. We don't want a game that
         * changes it every minute drop the VBO after VB_MAX_DECL_CHANGES minutes. So count draws without
         * decl changes and reset the decl change count after a specific number of them
         */
        if(buffer_is_fully_dirty(This))
        {
            ++This->full_conversion_count;
            if(This->full_conversion_count > VB_MAXFULLCONVERSIONS)
            {
                FIXME("Too many full buffer conversions, stopping converting\n");
                IWineD3DBuffer_UnLoad(iface);
                This->flags &= ~WINED3D_BUFFER_CREATEBO;
                IWineD3DDeviceImpl_MarkStateDirty(device, STATE_STREAMSRC);
                goto end;
            }
        }
        else
        {
            ++This->draw_count;
            if (This->draw_count > VB_RESETDECLCHANGE) This->decl_change_count = 0;
            if (This->draw_count > VB_RESETFULLCONVS) This->full_conversion_count = 0;
        }
    }

    if(This->buffer_type_hint == GL_ELEMENT_ARRAY_BUFFER_ARB)
    {
        IWineD3DDeviceImpl_MarkStateDirty(This->resource.device, STATE_INDEXBUFFER);
    }

    if (!This->conversion_map)
    {
        /* That means that there is nothing to fixup. Just upload from This->resource.allocatedMemory
         * directly into the vbo. Do not free the system memory copy because drawPrimitive may need it if
         * the stride is 0, for instancing emulation, vertex blending emulation or shader emulation.
         */
        TRACE("No conversion needed\n");

        /* Nothing to do because we locked directly into the vbo */
        if (!(This->flags & WINED3D_BUFFER_DOUBLEBUFFER))
        {
            context_release(context);
            return;
        }

        buffer_direct_upload(This, context->gl_info, flags);

        context_release(context);
        return;
    }

    if(!(This->flags & WINED3D_BUFFER_DOUBLEBUFFER))
    {
        buffer_get_sysmem(This, gl_info);
    }

    /* Now for each vertex in the buffer that needs conversion */
    vertices = This->resource.size / This->stride;

    if (This->conversion_shift)
    {
        TRACE("Shifted conversion\n");
        data = HeapAlloc(GetProcessHeap(), 0, vertices * This->conversion_stride);

        start = 0;
        len = This->resource.size;
        end = start + len;

        if (This->maps[0].offset || This->maps[0].size != This->resource.size)
        {
            FIXME("Implement partial buffer load with shifted conversion\n");
        }

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

        while(This->modified_areas)
        {
            This->modified_areas--;
            start = This->maps[This->modified_areas].offset;
            len = This->maps[This->modified_areas].size;
            end = start + len;

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
            GL_EXTCALL(glBufferSubDataARB(This->buffer_type_hint, start, len, data + start));
            checkGLcall("glBufferSubDataARB");
            LEAVE_GL();
        }
    }

    HeapFree(GetProcessHeap(), 0, data);

end:
    context_release(context);
}

static WINED3DRESOURCETYPE STDMETHODCALLTYPE buffer_GetType(IWineD3DBuffer *iface)
{
    return resource_get_type((IWineD3DResource *)iface);
}

/* IWineD3DBuffer methods */

static DWORD buffer_sanitize_flags(struct wined3d_buffer *buffer, DWORD flags)
{
    /* Not all flags make sense together, but Windows never returns an error. Catch the
     * cases that could cause issues */
    if(flags & WINED3DLOCK_READONLY)
    {
        if(flags & WINED3DLOCK_DISCARD)
        {
            WARN("WINED3DLOCK_READONLY combined with WINED3DLOCK_DISCARD, ignoring flags\n");
            return 0;
        }
        if(flags & WINED3DLOCK_NOOVERWRITE)
        {
            WARN("WINED3DLOCK_READONLY combined with WINED3DLOCK_NOOVERWRITE, ignoring flags\n");
            return 0;
        }
    }
    else if((flags & (WINED3DLOCK_DISCARD | WINED3DLOCK_NOOVERWRITE)) == (WINED3DLOCK_DISCARD | WINED3DLOCK_NOOVERWRITE))
    {
        WARN("WINED3DLOCK_DISCARD and WINED3DLOCK_NOOVERWRITE used together, ignoring\n");
        return 0;
    }
    else if (flags & (WINED3DLOCK_DISCARD | WINED3DLOCK_NOOVERWRITE) && !(buffer->resource.usage & WINED3DUSAGE_DYNAMIC))
    {
        WARN("DISCARD or NOOVERWRITE lock on non-dynamic buffer, ignoring\n");
        return 0;
    }

    return flags;
}

static GLbitfield buffer_gl_map_flags(DWORD d3d_flags)
{
    GLbitfield ret = 0;

    if (!(d3d_flags & WINED3DLOCK_READONLY)) ret = GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT;

    if (d3d_flags & (WINED3DLOCK_DISCARD | WINED3DLOCK_NOOVERWRITE))
    {
        if(d3d_flags & WINED3DLOCK_DISCARD) ret |= GL_MAP_INVALIDATE_BUFFER_BIT;
        ret |= GL_MAP_UNSYNCHRONIZED_BIT;
    }
    else
    {
        ret |= GL_MAP_READ_BIT;
    }

    return ret;
}

static HRESULT STDMETHODCALLTYPE buffer_Map(IWineD3DBuffer *iface, UINT offset, UINT size, BYTE **data, DWORD flags)
{
    struct wined3d_buffer *This = (struct wined3d_buffer *)iface;
    LONG count;
    BOOL dirty = buffer_is_dirty(This);

    TRACE("iface %p, offset %u, size %u, data %p, flags %#x\n", iface, offset, size, data, flags);

    flags = buffer_sanitize_flags(This, flags);
    if (!(flags & WINED3DLOCK_READONLY))
    {
        if (!buffer_add_dirty_area(This, offset, size)) return E_OUTOFMEMORY;
    }

    count = InterlockedIncrement(&This->lock_count);

    if (This->buffer_object)
    {
        if(!(This->flags & WINED3D_BUFFER_DOUBLEBUFFER))
        {
            if(count == 1)
            {
                IWineD3DDeviceImpl *device = This->resource.device;
                struct wined3d_context *context;
                const struct wined3d_gl_info *gl_info;

                if(This->buffer_type_hint == GL_ELEMENT_ARRAY_BUFFER_ARB)
                {
                    IWineD3DDeviceImpl_MarkStateDirty(This->resource.device, STATE_INDEXBUFFER);
                }

                context = context_acquire(device, NULL);
                gl_info = context->gl_info;
                ENTER_GL();
                GL_EXTCALL(glBindBufferARB(This->buffer_type_hint, This->buffer_object));

                if (gl_info->supported[ARB_MAP_BUFFER_RANGE])
                {
                    GLbitfield mapflags = buffer_gl_map_flags(flags);
                    This->resource.allocatedMemory = GL_EXTCALL(glMapBufferRange(This->buffer_type_hint, 0,
                                                                                This->resource.size, mapflags));
                    checkGLcall("glMapBufferRange");
                }
                else
                {
                    if(This->flags & WINED3D_BUFFER_APPLESYNC)
                    {
                        LEAVE_GL();
                        buffer_sync_apple(This, flags, gl_info);
                        ENTER_GL();
                    }
                    This->resource.allocatedMemory = GL_EXTCALL(glMapBufferARB(This->buffer_type_hint, GL_READ_WRITE_ARB));
                    checkGLcall("glMapBufferARB");
                }
                LEAVE_GL();

                if (((DWORD_PTR) This->resource.allocatedMemory) & (RESOURCE_ALIGNMENT - 1))
                {
                    WARN("Pointer %p is not %u byte aligned, falling back to double buffered operation\n",
                        This->resource.allocatedMemory, RESOURCE_ALIGNMENT);

                    ENTER_GL();
                    GL_EXTCALL(glUnmapBufferARB(This->buffer_type_hint));
                    checkGLcall("glUnmapBufferARB");
                    LEAVE_GL();
                    This->resource.allocatedMemory = NULL;

                    buffer_get_sysmem(This, gl_info);
                    TRACE("New pointer is %p\n", This->resource.allocatedMemory);
                }
                context_release(context);
            }
        }
        else
        {
            if (dirty)
            {
                if (This->flags & WINED3D_BUFFER_NOSYNC && !(flags & WINED3DLOCK_NOOVERWRITE))
                {
                    This->flags &= ~WINED3D_BUFFER_NOSYNC;
                }
            }
            else if(flags & WINED3DLOCK_NOOVERWRITE)
            {
                This->flags |= WINED3D_BUFFER_NOSYNC;
            }

            if (flags & WINED3DLOCK_DISCARD)
            {
                This->flags |= WINED3D_BUFFER_DISCARD;
            }
        }
    }

    *data = This->resource.allocatedMemory + offset;

    TRACE("Returning memory at %p (base %p, offset %u)\n", *data, This->resource.allocatedMemory, offset);
    /* TODO: check Flags compatibility with This->currentDesc.Usage (see MSDN) */

    return WINED3D_OK;
}

static HRESULT STDMETHODCALLTYPE buffer_Unmap(IWineD3DBuffer *iface)
{
    struct wined3d_buffer *This = (struct wined3d_buffer *)iface;
    ULONG i;

    TRACE("(%p)\n", This);

    /* In the case that the number of Unmap calls > the
     * number of Map calls, d3d returns always D3D_OK.
     * This is also needed to prevent Map from returning garbage on
     * the next call (this will happen if the lock_count is < 0). */
    if(This->lock_count == 0)
    {
        TRACE("Unmap called without a previous Map call!\n");
        return WINED3D_OK;
    }

    if (InterlockedDecrement(&This->lock_count))
    {
        /* Delay loading the buffer until everything is unlocked */
        TRACE("Ignoring unlock\n");
        return WINED3D_OK;
    }

    if(!(This->flags & WINED3D_BUFFER_DOUBLEBUFFER) && This->buffer_object)
    {
        IWineD3DDeviceImpl *device = This->resource.device;
        const struct wined3d_gl_info *gl_info;
        struct wined3d_context *context;

        if(This->buffer_type_hint == GL_ELEMENT_ARRAY_BUFFER_ARB)
        {
            IWineD3DDeviceImpl_MarkStateDirty(This->resource.device, STATE_INDEXBUFFER);
        }

        context = context_acquire(device, NULL);
        gl_info = context->gl_info;
        ENTER_GL();
        GL_EXTCALL(glBindBufferARB(This->buffer_type_hint, This->buffer_object));

        if (gl_info->supported[ARB_MAP_BUFFER_RANGE])
        {
            for(i = 0; i < This->modified_areas; i++)
            {
                GL_EXTCALL(glFlushMappedBufferRange(This->buffer_type_hint,
                                                    This->maps[i].offset,
                                                    This->maps[i].size));
                checkGLcall("glFlushMappedBufferRange");
            }
        }
        else if (This->flags & WINED3D_BUFFER_FLUSH)
        {
            for(i = 0; i < This->modified_areas; i++)
            {
                GL_EXTCALL(glFlushMappedBufferRangeAPPLE(This->buffer_type_hint,
                                                         This->maps[i].offset,
                                                         This->maps[i].size));
                checkGLcall("glFlushMappedBufferRangeAPPLE");
            }
        }

        GL_EXTCALL(glUnmapBufferARB(This->buffer_type_hint));
        LEAVE_GL();
        context_release(context);

        This->resource.allocatedMemory = NULL;
        buffer_clear_dirty_areas(This);
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

static const struct IWineD3DBufferVtbl wined3d_buffer_vtbl =
{
    /* IUnknown methods */
    buffer_QueryInterface,
    buffer_AddRef,
    buffer_Release,
    /* IWineD3DBase methods */
    buffer_GetParent,
    /* IWineD3DResource methods */
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

HRESULT buffer_init(struct wined3d_buffer *buffer, IWineD3DDeviceImpl *device,
        UINT size, DWORD usage, WINED3DFORMAT format, WINED3DPOOL pool, GLenum bind_hint,
        const char *data, IUnknown *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_format_desc *format_desc = getFormatDescEntry(format, &device->adapter->gl_info);
    HRESULT hr;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    BOOL dynamic_buffer_ok;

    if (!size)
    {
        WARN("Size 0 requested, returning WINED3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }

    buffer->vtbl = &wined3d_buffer_vtbl;

    hr = resource_init((IWineD3DResource *)buffer, WINED3DRTYPE_BUFFER,
            device, size, usage, format_desc, pool, parent, parent_ops);
    if (FAILED(hr))
    {
        WARN("Failed to initialize resource, hr %#x\n", hr);
        return hr;
    }
    buffer->buffer_type_hint = bind_hint;

    TRACE("size %#x, usage %#x, format %s, memory @ %p, iface @ %p.\n", buffer->resource.size, buffer->resource.usage,
            debug_d3dformat(buffer->resource.format_desc->format), buffer->resource.allocatedMemory, buffer);

    /* GL_ARB_map_buffer_range is disabled for now due to numerous bugs and no gains */
    dynamic_buffer_ok = gl_info->supported[APPLE_FLUSH_BUFFER_RANGE];

    /* Observations show that drawStridedSlow is faster on dynamic VBs than converting +
     * drawStridedFast (half-life 2 and others).
     *
     * Basically converting the vertices in the buffer is quite expensive, and observations
     * show that drawStridedSlow is faster than converting + uploading + drawStridedFast.
     * Therefore do not create a VBO for WINED3DUSAGE_DYNAMIC buffers.
     */
    if (!gl_info->supported[ARB_VERTEX_BUFFER_OBJECT])
    {
        TRACE("Not creating a vbo because GL_ARB_vertex_buffer is not supported\n");
    }
    else if(buffer->resource.pool == WINED3DPOOL_SYSTEMMEM)
    {
        TRACE("Not creating a vbo because the vertex buffer is in system memory\n");
    }
    else if(!dynamic_buffer_ok && (buffer->resource.usage & WINED3DUSAGE_DYNAMIC))
    {
        TRACE("Not creating a vbo because the buffer has dynamic usage and no GL support\n");
    }
    else
    {
        buffer->flags |= WINED3D_BUFFER_CREATEBO;
    }

    if (data)
    {
        BYTE *ptr;

        hr = IWineD3DBuffer_Map((IWineD3DBuffer *)buffer, 0, size, &ptr, 0);
        if (FAILED(hr))
        {
            ERR("Failed to map buffer, hr %#x\n", hr);
            buffer_UnLoad((IWineD3DBuffer *)buffer);
            resource_cleanup((IWineD3DResource *)buffer);
            return hr;
        }

        memcpy(ptr, data, size);

        hr = IWineD3DBuffer_Unmap((IWineD3DBuffer *)buffer);
        if (FAILED(hr))
        {
            ERR("Failed to unmap buffer, hr %#x\n", hr);
            buffer_UnLoad((IWineD3DBuffer *)buffer);
            resource_cleanup((IWineD3DResource *)buffer);
            return hr;
        }
    }

    buffer->maps = HeapAlloc(GetProcessHeap(), 0, sizeof(*buffer->maps));
    if (!buffer->maps)
    {
        ERR("Out of memory\n");
        buffer_UnLoad((IWineD3DBuffer *)buffer);
        resource_cleanup((IWineD3DResource *)buffer);
        return E_OUTOFMEMORY;
    }
    buffer->maps_size = 1;

    return WINED3D_OK;
}
