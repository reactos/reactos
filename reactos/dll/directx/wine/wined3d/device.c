/*
 * IWineD3DDevice implementation
 *
 * Copyright 2002 Lionel Ulmer
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006-2008 Stefan Dösinger for CodeWeavers
 * Copyright 2006-2008 Henri Verbeet
 * Copyright 2007 Andrew Riedi
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
#include <stdio.h>
#ifdef HAVE_FLOAT_H
# include <float.h>
#endif
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
#define GLINFO_LOCATION This->adapter->gl_info

/* Define the default light parameters as specified by MSDN */
const WINED3DLIGHT WINED3D_default_light = {

    WINED3DLIGHT_DIRECTIONAL, /* Type */
    { 1.0, 1.0, 1.0, 0.0 },   /* Diffuse r,g,b,a */
    { 0.0, 0.0, 0.0, 0.0 },   /* Specular r,g,b,a */
    { 0.0, 0.0, 0.0, 0.0 },   /* Ambient r,g,b,a, */
    { 0.0, 0.0, 0.0 },        /* Position x,y,z */
    { 0.0, 0.0, 1.0 },        /* Direction x,y,z */
    0.0,                      /* Range */
    0.0,                      /* Falloff */
    0.0, 0.0, 0.0,            /* Attenuation 0,1,2 */
    0.0,                      /* Theta */
    0.0                       /* Phi */
};

/* static function declarations */
static void IWineD3DDeviceImpl_AddResource(IWineD3DDevice *iface, IWineD3DResource *resource);

/**********************************************************
 * Global variable / Constants follow
 **********************************************************/
const float identity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};  /* When needed for comparisons */

/* Note that except for WINED3DPT_POINTLIST and WINED3DPT_LINELIST these
 * actually have the same values in GL and D3D. */
static GLenum gl_primitive_type_from_d3d(WINED3DPRIMITIVETYPE primitive_type)
{
    switch(primitive_type)
    {
        case WINED3DPT_POINTLIST:
            return GL_POINTS;

        case WINED3DPT_LINELIST:
            return GL_LINES;

        case WINED3DPT_LINESTRIP:
            return GL_LINE_STRIP;

        case WINED3DPT_TRIANGLELIST:
            return GL_TRIANGLES;

        case WINED3DPT_TRIANGLESTRIP:
            return GL_TRIANGLE_STRIP;

        case WINED3DPT_TRIANGLEFAN:
            return GL_TRIANGLE_FAN;

        case WINED3DPT_LINELIST_ADJ:
            return GL_LINES_ADJACENCY_ARB;

        case WINED3DPT_LINESTRIP_ADJ:
            return GL_LINE_STRIP_ADJACENCY_ARB;

        case WINED3DPT_TRIANGLELIST_ADJ:
            return GL_TRIANGLES_ADJACENCY_ARB;

        case WINED3DPT_TRIANGLESTRIP_ADJ:
            return GL_TRIANGLE_STRIP_ADJACENCY_ARB;

        default:
            FIXME("Unhandled primitive type %s\n", debug_d3dprimitivetype(primitive_type));
            return GL_NONE;
    }
}

static WINED3DPRIMITIVETYPE d3d_primitive_type_from_gl(GLenum primitive_type)
{
    switch(primitive_type)
    {
        case GL_POINTS:
            return WINED3DPT_POINTLIST;

        case GL_LINES:
            return WINED3DPT_LINELIST;

        case GL_LINE_STRIP:
            return WINED3DPT_LINESTRIP;

        case GL_TRIANGLES:
            return WINED3DPT_TRIANGLELIST;

        case GL_TRIANGLE_STRIP:
            return WINED3DPT_TRIANGLESTRIP;

        case GL_TRIANGLE_FAN:
            return WINED3DPT_TRIANGLEFAN;

        case GL_LINES_ADJACENCY_ARB:
            return WINED3DPT_LINELIST_ADJ;

        case GL_LINE_STRIP_ADJACENCY_ARB:
            return WINED3DPT_LINESTRIP_ADJ;

        case GL_TRIANGLES_ADJACENCY_ARB:
            return WINED3DPT_TRIANGLELIST_ADJ;

        case GL_TRIANGLE_STRIP_ADJACENCY_ARB:
            return WINED3DPT_TRIANGLESTRIP_ADJ;

        default:
            FIXME("Unhandled primitive type %s\n", debug_d3dprimitivetype(primitive_type));
            return WINED3DPT_UNDEFINED;
    }
}

static BOOL fixed_get_input(BYTE usage, BYTE usage_idx, unsigned int *regnum)
{
    if ((usage == WINED3DDECLUSAGE_POSITION || usage == WINED3DDECLUSAGE_POSITIONT) && usage_idx == 0)
        *regnum = WINED3D_FFP_POSITION;
    else if (usage == WINED3DDECLUSAGE_BLENDWEIGHT && usage_idx == 0)
        *regnum = WINED3D_FFP_BLENDWEIGHT;
    else if (usage == WINED3DDECLUSAGE_BLENDINDICES && usage_idx == 0)
        *regnum = WINED3D_FFP_BLENDINDICES;
    else if (usage == WINED3DDECLUSAGE_NORMAL && usage_idx == 0)
        *regnum = WINED3D_FFP_NORMAL;
    else if (usage == WINED3DDECLUSAGE_PSIZE && usage_idx == 0)
        *regnum = WINED3D_FFP_PSIZE;
    else if (usage == WINED3DDECLUSAGE_COLOR && usage_idx == 0)
        *regnum = WINED3D_FFP_DIFFUSE;
    else if (usage == WINED3DDECLUSAGE_COLOR && usage_idx == 1)
        *regnum = WINED3D_FFP_SPECULAR;
    else if (usage == WINED3DDECLUSAGE_TEXCOORD && usage_idx < WINED3DDP_MAXTEXCOORD)
        *regnum = WINED3D_FFP_TEXCOORD0 + usage_idx;
    else
    {
        FIXME("Unsupported input stream [usage=%s, usage_idx=%u]\n", debug_d3ddeclusage(usage), usage_idx);
        *regnum = ~0U;
        return FALSE;
    }

    return TRUE;
}

void device_stream_info_from_declaration(IWineD3DDeviceImpl *This,
        BOOL use_vshader, struct wined3d_stream_info *stream_info, BOOL *fixup)
{
    /* We need to deal with frequency data! */
    IWineD3DVertexDeclarationImpl *declaration = (IWineD3DVertexDeclarationImpl *)This->stateBlock->vertexDecl;
    UINT stream_count = This->stateBlock->streamIsUP ? 0 : declaration->num_streams;
    const DWORD *streams = declaration->streams;
    unsigned int i;

    memset(stream_info, 0, sizeof(*stream_info));

    /* Check for transformed vertices, disable vertex shader if present. */
    stream_info->position_transformed = declaration->position_transformed;
    if (declaration->position_transformed) use_vshader = FALSE;

    /* Translate the declaration into strided data. */
    for (i = 0; i < declaration->element_count; ++i)
    {
        const struct wined3d_vertex_declaration_element *element = &declaration->elements[i];
        GLuint buffer_object = 0;
        const BYTE *data = NULL;
        BOOL stride_used;
        unsigned int idx;
        DWORD stride;

        TRACE("%p Element %p (%u of %u)\n", declaration->elements,
                element, i + 1, declaration->element_count);

        if (!This->stateBlock->streamSource[element->input_slot]) continue;

        stride = This->stateBlock->streamStride[element->input_slot];
        if (This->stateBlock->streamIsUP)
        {
            TRACE("Stream %u is UP, %p\n", element->input_slot, This->stateBlock->streamSource[element->input_slot]);
            buffer_object = 0;
            data = (BYTE *)This->stateBlock->streamSource[element->input_slot];
        }
        else
        {
            TRACE("Stream %u isn't UP, %p\n", element->input_slot, This->stateBlock->streamSource[element->input_slot]);
            data = buffer_get_memory(This->stateBlock->streamSource[element->input_slot], 0, &buffer_object);

            /* Can't use vbo's if the base vertex index is negative. OpenGL doesn't accept negative offsets
             * (or rather offsets bigger than the vbo, because the pointer is unsigned), so use system memory
             * sources. In most sane cases the pointer - offset will still be > 0, otherwise it will wrap
             * around to some big value. Hope that with the indices, the driver wraps it back internally. If
             * not, drawStridedSlow is needed, including a vertex buffer path. */
            if (This->stateBlock->loadBaseVertexIndex < 0)
            {
                WARN("loadBaseVertexIndex is < 0 (%d), not using vbos\n", This->stateBlock->loadBaseVertexIndex);
                buffer_object = 0;
                data = ((struct wined3d_buffer *)This->stateBlock->streamSource[element->input_slot])->resource.allocatedMemory;
                if ((UINT_PTR)data < -This->stateBlock->loadBaseVertexIndex * stride)
                {
                    FIXME("System memory vertex data load offset is negative!\n");
                }
            }

            if (fixup)
            {
                if (buffer_object) *fixup = TRUE;
                else if (*fixup && !use_vshader
                        && (element->usage == WINED3DDECLUSAGE_COLOR
                        || element->usage == WINED3DDECLUSAGE_POSITIONT))
                {
                    static BOOL warned = FALSE;
                    if (!warned)
                    {
                        /* This may be bad with the fixed function pipeline. */
                        FIXME("Missing vbo streams with unfixed colors or transformed position, expect problems\n");
                        warned = TRUE;
                    }
                }
            }
        }
        data += element->offset;

        TRACE("offset %u input_slot %u usage_idx %d\n", element->offset, element->input_slot, element->usage_idx);

        if (use_vshader)
        {
            if (element->output_slot == ~0U)
            {
                /* TODO: Assuming vertexdeclarations are usually used with the
                 * same or a similar shader, it might be worth it to store the
                 * last used output slot and try that one first. */
                stride_used = vshader_get_input(This->stateBlock->vertexShader,
                        element->usage, element->usage_idx, &idx);
            }
            else
            {
                idx = element->output_slot;
                stride_used = TRUE;
            }
        }
        else
        {
            if (!element->ffp_valid)
            {
                WARN("Skipping unsupported fixed function element of format %s and usage %s\n",
                        debug_d3dformat(element->format_desc->format), debug_d3ddeclusage(element->usage));
                stride_used = FALSE;
            }
            else
            {
                stride_used = fixed_get_input(element->usage, element->usage_idx, &idx);
            }
        }

        if (stride_used)
        {
            TRACE("Load %s array %u [usage %s, usage_idx %u, "
                    "input_slot %u, offset %u, stride %u, format %s, buffer_object %u]\n",
                    use_vshader ? "shader": "fixed function", idx,
                    debug_d3ddeclusage(element->usage), element->usage_idx, element->input_slot,
                    element->offset, stride, debug_d3dformat(element->format_desc->format), buffer_object);

            stream_info->elements[idx].format_desc = element->format_desc;
            stream_info->elements[idx].stride = stride;
            stream_info->elements[idx].data = data;
            stream_info->elements[idx].stream_idx = element->input_slot;
            stream_info->elements[idx].buffer_object = buffer_object;

            if (!GL_SUPPORT(EXT_VERTEX_ARRAY_BGRA) && element->format_desc->format == WINED3DFMT_A8R8G8B8)
            {
                stream_info->swizzle_map |= 1 << idx;
            }
            stream_info->use_map |= 1 << idx;
        }
    }

    /* Now call PreLoad on all the vertex buffers. In the very rare case
     * that the buffers stopps converting PreLoad will dirtify the VDECL again.
     * The vertex buffer can now use the strided structure in the device instead of finding its
     * own again.
     *
     * NULL streams won't be recorded in the array, UP streams won't be either. A stream is only
     * once in there. */
    for (i = 0; i < stream_count; ++i)
    {
        IWineD3DBuffer *vb = This->stateBlock->streamSource[streams[i]];
        if (vb) IWineD3DBuffer_PreLoad(vb);
    }
}

static void stream_info_element_from_strided(IWineD3DDeviceImpl *This,
        const struct WineDirect3DStridedData *strided, struct wined3d_stream_info_element *e)
{
    const struct GlPixelFormatDesc *format_desc = getFormatDescEntry(strided->format, &This->adapter->gl_info);
    e->format_desc = format_desc;
    e->stride = strided->dwStride;
    e->data = strided->lpData;
    e->stream_idx = 0;
    e->buffer_object = 0;
}

void device_stream_info_from_strided(IWineD3DDeviceImpl *This,
        const struct WineDirect3DVertexStridedData *strided, struct wined3d_stream_info *stream_info)
{
    unsigned int i;

    memset(stream_info, 0, sizeof(*stream_info));

    if (strided->position.lpData)
        stream_info_element_from_strided(This, &strided->position, &stream_info->elements[WINED3D_FFP_POSITION]);
    if (strided->normal.lpData)
        stream_info_element_from_strided(This, &strided->normal, &stream_info->elements[WINED3D_FFP_NORMAL]);
    if (strided->diffuse.lpData)
        stream_info_element_from_strided(This, &strided->diffuse, &stream_info->elements[WINED3D_FFP_DIFFUSE]);
    if (strided->specular.lpData)
        stream_info_element_from_strided(This, &strided->specular, &stream_info->elements[WINED3D_FFP_SPECULAR]);

    for (i = 0; i < WINED3DDP_MAXTEXCOORD; ++i)
    {
        if (strided->texCoords[i].lpData)
            stream_info_element_from_strided(This, &strided->texCoords[i],
                    &stream_info->elements[WINED3D_FFP_TEXCOORD0 + i]);
    }

    stream_info->position_transformed = strided->position_transformed;

    for (i = 0; i < sizeof(stream_info->elements) / sizeof(*stream_info->elements); ++i)
    {
        if (!GL_SUPPORT(EXT_VERTEX_ARRAY_BGRA) && stream_info->elements[i].format_desc->format == WINED3DFMT_A8R8G8B8)
        {
            stream_info->swizzle_map |= 1 << i;
        }
        stream_info->use_map |= 1 << i;
    }
}

/**********************************************************
 * IUnknown parts follows
 **********************************************************/

static HRESULT WINAPI IWineD3DDeviceImpl_QueryInterface(IWineD3DDevice *iface,REFIID riid,LPVOID *ppobj)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DDevice)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IWineD3DDeviceImpl_AddRef(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef increasing from %d\n", This, refCount - 1);
    return refCount;
}

static ULONG WINAPI IWineD3DDeviceImpl_Release(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p) : Releasing from %d\n", This, refCount + 1);

    if (!refCount) {
        UINT i;

        for (i = 0; i < sizeof(This->multistate_funcs)/sizeof(This->multistate_funcs[0]); ++i) {
            HeapFree(GetProcessHeap(), 0, This->multistate_funcs[i]);
            This->multistate_funcs[i] = NULL;
        }

        /* TODO: Clean up all the surfaces and textures! */
        /* NOTE: You must release the parent if the object was created via a callback
        ** ***************************/

        if (!list_empty(&This->resources)) {
            FIXME("(%p) Device released with resources still bound, acceptable but unexpected\n", This);
            dumpResources(&This->resources);
        }

        if(This->contexts) ERR("Context array not freed!\n");
        if (This->hardwareCursor) DestroyCursor(This->hardwareCursor);
        This->haveHardwareCursor = FALSE;

        IWineD3D_Release(This->wineD3D);
        This->wineD3D = NULL;
        HeapFree(GetProcessHeap(), 0, This);
        TRACE("Freed device  %p\n", This);
        This = NULL;
    }
    return refCount;
}

/**********************************************************
 * IWineD3DDevice implementation follows
 **********************************************************/
static HRESULT WINAPI IWineD3DDeviceImpl_GetParent(IWineD3DDevice *iface, IUnknown **pParent) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    *pParent = This->parent;
    IUnknown_AddRef(This->parent);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_CreateBuffer(IWineD3DDevice *iface,
        struct wined3d_buffer_desc *desc, const void *data, IUnknown *parent, IWineD3DBuffer **buffer)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    struct wined3d_buffer *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, data %p, parent %p, buffer %p\n", iface, desc, data, parent, buffer);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate memory\n");
        return E_OUTOFMEMORY;
    }

    object->vtbl = &wined3d_buffer_vtbl;
    object->desc = *desc;

    FIXME("Ignoring access flags (pool)\n");

    hr = resource_init(&object->resource, WINED3DRTYPE_BUFFER, This, desc->byte_width,
            desc->usage, WINED3DFMT_UNKNOWN, WINED3DPOOL_MANAGED, parent);
    if (FAILED(hr))
    {
        WARN("Failed to initialize resource, returning %#x\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created resource %p\n", object);

    IWineD3DDeviceImpl_AddResource(iface, (IWineD3DResource *)object);

    TRACE("size %#x, usage=%#x, format %s, memory @ %p, iface @ %p\n", object->resource.size, object->resource.usage,
            debug_d3dformat(object->resource.format_desc->format), object->resource.allocatedMemory, object);

    if (data)
    {
        BYTE *ptr;

        hr = IWineD3DBuffer_Map((IWineD3DBuffer *)object, 0, desc->byte_width, &ptr, 0);
        if (FAILED(hr))
        {
            ERR("Failed to map buffer, hr %#x\n", hr);
            IWineD3DBuffer_Release((IWineD3DBuffer *)object);
            return hr;
        }

        memcpy(ptr, data, desc->byte_width);

        hr = IWineD3DBuffer_Unmap((IWineD3DBuffer *)object);
        if (FAILED(hr))
        {
            ERR("Failed to unmap buffer, hr %#x\n", hr);
            IWineD3DBuffer_Release((IWineD3DBuffer *)object);
            return hr;
        }
    }

    *buffer = (IWineD3DBuffer *)object;

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_CreateVertexBuffer(IWineD3DDevice *iface, UINT Size, DWORD Usage,
        DWORD FVF, WINED3DPOOL Pool, IWineD3DBuffer **ppVertexBuffer, HANDLE *sharedHandle, IUnknown *parent)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    /* Dummy format for now */
    const struct GlPixelFormatDesc *format_desc = getFormatDescEntry(WINED3DFMT_VERTEXDATA, &This->adapter->gl_info);
    struct wined3d_buffer *object;
    int dxVersion = ( (IWineD3DImpl *) This->wineD3D)->dxVersion;
    HRESULT hr;
    BOOL conv;

    if(Size == 0) {
        WARN("Size 0 requested, returning WINED3DERR_INVALIDCALL\n");
        *ppVertexBuffer = NULL;
        return WINED3DERR_INVALIDCALL;
    } else if(Pool == WINED3DPOOL_SCRATCH) {
        /* The d3d9 testsuit shows that this is not allowed. It doesn't make much sense
         * anyway, SCRATCH vertex buffers aren't usable anywhere
         */
        WARN("Vertex buffer in D3DPOOL_SCRATCH requested, returning WINED3DERR_INVALIDCALL\n");
        *ppVertexBuffer = NULL;
        return WINED3DERR_INVALIDCALL;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Out of memory\n");
        *ppVertexBuffer = NULL;
        return WINED3DERR_OUTOFVIDEOMEMORY;
    }

    object->vtbl = &wined3d_buffer_vtbl;
    hr = resource_init(&object->resource, WINED3DRTYPE_VERTEXBUFFER, This, Size, Usage, format_desc, Pool, parent);
    if (FAILED(hr))
    {
        WARN("Failed to initialize resource, returning %#x\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        *ppVertexBuffer = NULL;
        return hr;
    }

    TRACE("(%p) : Created resource %p\n", This, object);

    IWineD3DDeviceImpl_AddResource(iface, (IWineD3DResource *)object);

    TRACE("(%p) : Size=%d, Usage=0x%08x, FVF=%x, Pool=%d - Memory@%p, Iface@%p\n", This, Size, Usage, FVF, Pool, object->resource.allocatedMemory, object);
    *ppVertexBuffer = (IWineD3DBuffer *)object;

    object->fvf = FVF;

    /* Observations show that drawStridedSlow is faster on dynamic VBs than converting +
     * drawStridedFast (half-life 2).
     *
     * Basically converting the vertices in the buffer is quite expensive, and observations
     * show that drawStridedSlow is faster than converting + uploading + drawStridedFast.
     * Therefore do not create a VBO for WINED3DUSAGE_DYNAMIC buffers.
     *
     * Direct3D7 has another problem: Its vertexbuffer api doesn't offer a way to specify
     * the range of vertices being locked, so each lock will require the whole buffer to be transformed.
     * Moreover geometry data in dx7 is quite simple, so drawStridedSlow isn't a big hit. A plus
     * is that the vertex buffers fvf can be trusted in dx7. So only create non-converted vbos for
     * dx7 apps.
     * There is a IDirect3DVertexBuffer7::Optimize call after which the buffer can't be locked any
     * more. In this call we can convert dx7 buffers too.
     */
    conv = ((FVF & WINED3DFVF_POSITION_MASK) == WINED3DFVF_XYZRHW ) || (FVF & (WINED3DFVF_DIFFUSE | WINED3DFVF_SPECULAR));
    if(!GL_SUPPORT(ARB_VERTEX_BUFFER_OBJECT)) {
        TRACE("Not creating a vbo because GL_ARB_vertex_buffer is not supported\n");
    } else if(Pool == WINED3DPOOL_SYSTEMMEM) {
        TRACE("Not creating a vbo because the vertex buffer is in system memory\n");
    } else if(Usage & WINED3DUSAGE_DYNAMIC) {
        TRACE("Not creating a vbo because the buffer has dynamic usage\n");
    } else if(dxVersion <= 7 && conv) {
        TRACE("Not creating a vbo because dxVersion is 7 and the fvf needs conversion\n");
    } else {
        object->flags |= WINED3D_BUFFER_CREATEBO;
    }
    return WINED3D_OK;
}

static void CreateIndexBufferVBO(IWineD3DDeviceImpl *This, IWineD3DIndexBufferImpl *object) {
    GLenum error, glUsage;
    TRACE("Creating VBO for Index Buffer %p\n", object);

    /* The following code will modify the ELEMENT_ARRAY_BUFFER binding, make sure it is
     * restored on the next draw
     */
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_INDEXBUFFER);

    /* Make sure that a context is there. Needed in a multithreaded environment. Otherwise this call is a nop */
    ActivateContext(This, This->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
    ENTER_GL();

    while(glGetError());

    GL_EXTCALL(glGenBuffersARB(1, &object->vbo));
    error = glGetError();
    if(error != GL_NO_ERROR || object->vbo == 0) {
        ERR("Creating a vbo failed with error %s (%#x), continuing without vbo for this buffer\n", debug_glerror(error), error);
        goto out;
    }

    GL_EXTCALL(glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, object->vbo));
    error = glGetError();
    if(error != GL_NO_ERROR) {
        ERR("Failed to bind index buffer with error %s (%#x), continuing without vbo for this buffer\n", debug_glerror(error), error);
        goto out;
    }

    /* Use static write only usage for now. Dynamic index buffers stay in sysmem, and due to the sysmem
        * copy no readback will be needed
        */
    glUsage = GL_STATIC_DRAW_ARB;
    GL_EXTCALL(glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, object->resource.size, NULL, glUsage));
    error = glGetError();
    if(error != GL_NO_ERROR) {
        ERR("Failed to initialize the index buffer with error %s (%#x)\n", debug_glerror(error), error);
        goto out;
    }
    LEAVE_GL();
    TRACE("Successfully created vbo %d for index buffer %p\n", object->vbo, object);
    return;

out:
    GL_EXTCALL(glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0));
    GL_EXTCALL(glDeleteBuffersARB(1, &object->vbo));
    LEAVE_GL();
    object->vbo = 0;
}

static HRESULT WINAPI IWineD3DDeviceImpl_CreateIndexBuffer(IWineD3DDevice *iface, UINT Length, DWORD Usage, 
                                                    WINED3DFORMAT Format, WINED3DPOOL Pool, IWineD3DIndexBuffer** ppIndexBuffer,
                                                    HANDLE *sharedHandle, IUnknown *parent) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    const struct GlPixelFormatDesc *format_desc = getFormatDescEntry(Format, &This->adapter->gl_info);
    IWineD3DIndexBufferImpl *object;
    HRESULT hr;

    TRACE("(%p) Creating index buffer\n", This);

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Out of memory\n");
        *ppIndexBuffer = NULL;
        return WINED3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &IWineD3DIndexBuffer_Vtbl;
    hr = resource_init(&object->resource, WINED3DRTYPE_INDEXBUFFER, This, Length, Usage, format_desc, Pool, parent);
    if (FAILED(hr))
    {
        WARN("Failed to initialize resource, returning %#x\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        *ppIndexBuffer = NULL;
        return hr;
    }

    TRACE("(%p) : Created resource %p\n", This, object);

    IWineD3DDeviceImpl_AddResource(iface, (IWineD3DResource *)object);

    if(Pool != WINED3DPOOL_SYSTEMMEM && !(Usage & WINED3DUSAGE_DYNAMIC) && GL_SUPPORT(ARB_VERTEX_BUFFER_OBJECT)) {
        CreateIndexBufferVBO(This, object);
    }

    TRACE("(%p) : Len=%d, Use=%x, Format=(%u,%s), Pool=%d - Memory@%p, Iface@%p\n", This, Length, Usage, Format, 
                           debug_d3dformat(Format), Pool, object, object->resource.allocatedMemory);
    *ppIndexBuffer = (IWineD3DIndexBuffer *) object;

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_CreateStateBlock(IWineD3DDevice* iface, WINED3DSTATEBLOCKTYPE Type, IWineD3DStateBlock** ppStateBlock, IUnknown *parent) {

    IWineD3DDeviceImpl     *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DStateBlockImpl *object;
    unsigned int i, j;
    HRESULT temp_result;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if(!object)
    {
        ERR("Out of memory\n");
        *ppStateBlock = NULL;
        return WINED3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &IWineD3DStateBlock_Vtbl;
    object->wineD3DDevice = This;
    object->parent = parent;
    object->ref = 1;
    object->blockType = Type;

    *ppStateBlock = (IWineD3DStateBlock *)object;

    for(i = 0; i < LIGHTMAP_SIZE; i++) {
        list_init(&object->lightMap[i]);
    }

    temp_result = allocate_shader_constants(object);
    if (FAILED(temp_result))
    {
        HeapFree(GetProcessHeap(), 0, object);
        return temp_result;
    }

    /* Special case - Used during initialization to produce a placeholder stateblock
          so other functions called can update a state block                         */
    if (Type == WINED3DSBT_INIT || Type == WINED3DSBT_RECORDED)
    {
        /* Don't bother increasing the reference count otherwise a device will never
           be freed due to circular dependencies                                   */
        return WINED3D_OK;
    }

    /* Otherwise, might as well set the whole state block to the appropriate values  */
    if (This->stateBlock != NULL)
        stateblock_copy((IWineD3DStateBlock*) object, (IWineD3DStateBlock*) This->stateBlock);
    else
        memset(object->streamFreq, 1, sizeof(object->streamFreq));

    /* Reset the ref and type after kludging it */
    object->wineD3DDevice = This;
    object->ref           = 1;
    object->blockType     = Type;

    TRACE("Updating changed flags appropriate for type %d\n", Type);

    if (Type == WINED3DSBT_ALL) {

        TRACE("ALL => Pretend everything has changed\n");
        stateblock_savedstates_set((IWineD3DStateBlock*) object, &object->changed, TRUE);

        /* Lights are not part of the changed / set structure */
        for(j = 0; j < LIGHTMAP_SIZE; j++) {
            struct list *e;
            LIST_FOR_EACH(e, &object->lightMap[j]) {
                PLIGHTINFOEL *light = LIST_ENTRY(e, PLIGHTINFOEL, entry);
                light->changed = TRUE;
                light->enabledChanged = TRUE;
            }
        }
        for(j = 1; j <= WINEHIGHEST_RENDER_STATE; j++) {
            object->contained_render_states[j - 1] = j;
        }
        object->num_contained_render_states = WINEHIGHEST_RENDER_STATE;
        /* TODO: Filter unused transforms between TEXTURE8 and WORLD0? */
        for(j = 1; j <= HIGHEST_TRANSFORMSTATE; j++) {
            object->contained_transform_states[j - 1] = j;
        }
        object->num_contained_transform_states = HIGHEST_TRANSFORMSTATE;
        for(j = 0; j < GL_LIMITS(vshader_constantsF); j++) {
            object->contained_vs_consts_f[j] = j;
        }
        object->num_contained_vs_consts_f = GL_LIMITS(vshader_constantsF);
        for(j = 0; j < MAX_CONST_I; j++) {
            object->contained_vs_consts_i[j] = j;
        }
        object->num_contained_vs_consts_i = MAX_CONST_I;
        for(j = 0; j < MAX_CONST_B; j++) {
            object->contained_vs_consts_b[j] = j;
        }
        object->num_contained_vs_consts_b = MAX_CONST_B;
        for(j = 0; j < GL_LIMITS(pshader_constantsF); j++) {
            object->contained_ps_consts_f[j] = j;
        }
        object->num_contained_ps_consts_f = GL_LIMITS(pshader_constantsF);
        for(j = 0; j < MAX_CONST_I; j++) {
            object->contained_ps_consts_i[j] = j;
        }
        object->num_contained_ps_consts_i = MAX_CONST_I;
        for(j = 0; j < MAX_CONST_B; j++) {
            object->contained_ps_consts_b[j] = j;
        }
        object->num_contained_ps_consts_b = MAX_CONST_B;
        for(i = 0; i < MAX_TEXTURES; i++) {
            for (j = 0; j <= WINED3D_HIGHEST_TEXTURE_STATE; ++j)
            {
                object->contained_tss_states[object->num_contained_tss_states].stage = i;
                object->contained_tss_states[object->num_contained_tss_states].state = j;
                object->num_contained_tss_states++;
            }
        }
        for(i = 0; i < MAX_COMBINED_SAMPLERS; i++) {
            for(j = 1; j <= WINED3D_HIGHEST_SAMPLER_STATE; j++) {
                object->contained_sampler_states[object->num_contained_sampler_states].stage = i;
                object->contained_sampler_states[object->num_contained_sampler_states].state = j;
                object->num_contained_sampler_states++;
            }
        }

        for(i = 0; i < MAX_STREAMS; i++) {
            if(object->streamSource[i]) {
                IWineD3DBuffer_AddRef(object->streamSource[i]);
            }
        }
        if(object->pIndexData) {
            IWineD3DIndexBuffer_AddRef(object->pIndexData);
        }
        if(object->vertexShader) {
            IWineD3DVertexShader_AddRef(object->vertexShader);
        }
        if(object->pixelShader) {
            IWineD3DPixelShader_AddRef(object->pixelShader);
        }

    } else if (Type == WINED3DSBT_PIXELSTATE) {

        TRACE("PIXELSTATE => Pretend all pixel shates have changed\n");
        stateblock_savedstates_set((IWineD3DStateBlock*) object, &object->changed, FALSE);

        object->changed.pixelShader = TRUE;

        /* Pixel Shader Constants */
        for (i = 0; i < GL_LIMITS(pshader_constantsF); ++i) {
            object->contained_ps_consts_f[i] = i;
            object->changed.pixelShaderConstantsF[i] = TRUE;
        }
        object->num_contained_ps_consts_f = GL_LIMITS(pshader_constantsF);
        for (i = 0; i < MAX_CONST_B; ++i) {
            object->contained_ps_consts_b[i] = i;
            object->changed.pixelShaderConstantsB |= (1 << i);
        }
        object->num_contained_ps_consts_b = MAX_CONST_B;
        for (i = 0; i < MAX_CONST_I; ++i) {
            object->contained_ps_consts_i[i] = i;
            object->changed.pixelShaderConstantsI |= (1 << i);
        }
        object->num_contained_ps_consts_i = MAX_CONST_I;

        for (i = 0; i < NUM_SAVEDPIXELSTATES_R; i++) {
            DWORD rs = SavedPixelStates_R[i];
            object->changed.renderState[rs >> 5] |= 1 << (rs & 0x1f);
            object->contained_render_states[i] = rs;
        }
        object->num_contained_render_states = NUM_SAVEDPIXELSTATES_R;
        for (j = 0; j < MAX_TEXTURES; j++) {
            for (i = 0; i < NUM_SAVEDPIXELSTATES_T; i++) {
                DWORD state = SavedPixelStates_T[i];
                object->changed.textureState[j] |= 1 << state;
                object->contained_tss_states[object->num_contained_tss_states].stage = j;
                object->contained_tss_states[object->num_contained_tss_states].state = state;
                object->num_contained_tss_states++;
            }
        }
        for (j = 0 ; j < MAX_COMBINED_SAMPLERS; j++) {
            for (i =0; i < NUM_SAVEDPIXELSTATES_S;i++) {
                DWORD state = SavedPixelStates_S[i];
                object->changed.samplerState[j] |= 1 << state;
                object->contained_sampler_states[object->num_contained_sampler_states].stage = j;
                object->contained_sampler_states[object->num_contained_sampler_states].state = state;
                object->num_contained_sampler_states++;
            }
        }
        if(object->pixelShader) {
            IWineD3DPixelShader_AddRef(object->pixelShader);
        }

        /* Pixel state blocks do not contain vertex buffers. Set them to NULL to avoid wrong refcounting
         * on them. This makes releasing the buffer easier
         */
        for(i = 0; i < MAX_STREAMS; i++) {
            object->streamSource[i] = NULL;
        }
        object->pIndexData = NULL;
        object->vertexShader = NULL;

    } else if (Type == WINED3DSBT_VERTEXSTATE) {

        TRACE("VERTEXSTATE => Pretend all vertex shates have changed\n");
        stateblock_savedstates_set((IWineD3DStateBlock*) object, &object->changed, FALSE);

        object->changed.vertexShader = TRUE;

        /* Vertex Shader Constants */
        for (i = 0; i < GL_LIMITS(vshader_constantsF); ++i) {
            object->changed.vertexShaderConstantsF[i] = TRUE;
            object->contained_vs_consts_f[i] = i;
        }
        object->num_contained_vs_consts_f = GL_LIMITS(vshader_constantsF);
        for (i = 0; i < MAX_CONST_B; ++i) {
            object->contained_vs_consts_b[i] = i;
            object->changed.vertexShaderConstantsB |= (1 << i);
        }
        object->num_contained_vs_consts_b = MAX_CONST_B;
        for (i = 0; i < MAX_CONST_I; ++i) {
            object->contained_vs_consts_i[i] = i;
            object->changed.vertexShaderConstantsI |= (1 << i);
        }
        object->num_contained_vs_consts_i = MAX_CONST_I;
        for (i = 0; i < NUM_SAVEDVERTEXSTATES_R; i++) {
            DWORD rs = SavedVertexStates_R[i];
            object->changed.renderState[rs >> 5] |= 1 << (rs & 0x1f);
            object->contained_render_states[i] = rs;
        }
        object->num_contained_render_states = NUM_SAVEDVERTEXSTATES_R;
        for (j = 0; j < MAX_TEXTURES; j++) {
            for (i = 0; i < NUM_SAVEDVERTEXSTATES_T; i++) {
                DWORD state = SavedVertexStates_T[i];
                object->changed.textureState[j] |= 1 << state;
                object->contained_tss_states[object->num_contained_tss_states].stage = j;
                object->contained_tss_states[object->num_contained_tss_states].state = state;
                object->num_contained_tss_states++;
            }
        }
        for (j = 0 ; j < MAX_COMBINED_SAMPLERS; j++){
            for (i =0; i < NUM_SAVEDVERTEXSTATES_S;i++) {
                DWORD state = SavedVertexStates_S[i];
                object->changed.samplerState[j] |= 1 << state;
                object->contained_sampler_states[object->num_contained_sampler_states].stage = j;
                object->contained_sampler_states[object->num_contained_sampler_states].state = state;
                object->num_contained_sampler_states++;
            }
        }

        for(j = 0; j < LIGHTMAP_SIZE; j++) {
            struct list *e;
            LIST_FOR_EACH(e, &object->lightMap[j]) {
                PLIGHTINFOEL *light = LIST_ENTRY(e, PLIGHTINFOEL, entry);
                light->changed = TRUE;
                light->enabledChanged = TRUE;
            }
        }

        for(i = 0; i < MAX_STREAMS; i++) {
            if(object->streamSource[i]) {
                IWineD3DBuffer_AddRef(object->streamSource[i]);
            }
        }
        if(object->vertexShader) {
            IWineD3DVertexShader_AddRef(object->vertexShader);
        }
        object->pIndexData = NULL;
        object->pixelShader = NULL;
    } else {
        FIXME("Unrecognized state block type %d\n", Type);
    }

    TRACE("(%p) returning token (ptr to stateblock) of %p\n", This, object);
    return WINED3D_OK;
}

static HRESULT  WINAPI IWineD3DDeviceImpl_CreateSurface(IWineD3DDevice *iface, UINT Width, UINT Height, WINED3DFORMAT Format, BOOL Lockable, BOOL Discard, UINT Level, IWineD3DSurface **ppSurface,WINED3DRESOURCETYPE Type, DWORD Usage, WINED3DPOOL Pool, WINED3DMULTISAMPLE_TYPE MultiSample ,DWORD MultisampleQuality, HANDLE* pSharedHandle, WINED3DSURFTYPE Impl, IUnknown *parent) {
    IWineD3DDeviceImpl  *This = (IWineD3DDeviceImpl *)iface;    
    IWineD3DSurfaceImpl *object; /*NOTE: impl ref allowed since this is a create function */
    unsigned int Size       = 1;
    const struct GlPixelFormatDesc *glDesc = getFormatDescEntry(Format, &GLINFO_LOCATION);
    UINT mul_4w, mul_4h;
    HRESULT hr;

    TRACE("(%p) Create surface\n",This);

    if(MultisampleQuality > 0) {
        FIXME("MultisampleQuality set to %d, substituting 0\n", MultisampleQuality);
        MultisampleQuality=0;
    }

    /** FIXME: Check that the format is supported
    *    by the device.
      *******************************/

    /** DXTn mipmaps use the same number of 'levels' down to eg. 8x1, but since
     *  it is based around 4x4 pixel blocks it requires padding, so allocate enough
     *  space!
      *********************************/
    mul_4w = (Width + 3) & ~3;
    mul_4h = (Height + 3) & ~3;
    if (WINED3DFMT_UNKNOWN == Format) {
        Size = 0;
    } else if (Format == WINED3DFMT_DXT1) {
        /* DXT1 is half byte per pixel */
        Size = (mul_4w * glDesc->byte_count * mul_4h) >> 1;

    } else if (Format == WINED3DFMT_DXT2 || Format == WINED3DFMT_DXT3 ||
               Format == WINED3DFMT_DXT4 || Format == WINED3DFMT_DXT5 ||
               Format == WINED3DFMT_ATI2N) {
        Size = (mul_4w * glDesc->byte_count * mul_4h);
    } else {
       /* The pitch is a multiple of 4 bytes */
        Size = ((Width * glDesc->byte_count) + This->surface_alignment - 1) & ~(This->surface_alignment - 1);
        Size *= Height;
    }

    if(glDesc->heightscale != 0.0) Size *= glDesc->heightscale;

    /** Create and initialise the surface resource **/
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Out of memory\n");
        *ppSurface = NULL;
        return WINED3DERR_OUTOFVIDEOMEMORY;
    }

    /* Look at the implementation and set the correct Vtable */
    switch(Impl)
    {
        case SURFACE_OPENGL:
            /* Check if a 3D adapter is available when creating gl surfaces */
            if (!This->adapter)
            {
                ERR("OpenGL surfaces are not available without opengl\n");
                HeapFree(GetProcessHeap(), 0, object);
                return WINED3DERR_NOTAVAILABLE;
            }
            object->lpVtbl = &IWineD3DSurface_Vtbl;
            break;

        case SURFACE_GDI:
            object->lpVtbl = &IWineGDISurface_Vtbl;
            break;

        default:
            /* To be sure to catch this */
            ERR("Unknown requested surface implementation %d!\n", Impl);
            HeapFree(GetProcessHeap(), 0, object);
            return WINED3DERR_INVALIDCALL;
    }

    hr = resource_init(&object->resource, WINED3DRTYPE_SURFACE, This, Size, Usage, glDesc, Pool, parent);
    if (FAILED(hr))
    {
        WARN("Failed to initialize resource, returning %#x\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        *ppSurface = NULL;
        return hr;
    }

    TRACE("(%p) : Created resource %p\n", This, object);

    IWineD3DDeviceImpl_AddResource(iface, (IWineD3DResource *)object);

    *ppSurface = (IWineD3DSurface *)object;

    /* "Standalone" surface */
    IWineD3DSurface_SetContainer((IWineD3DSurface *)object, NULL);

    object->currentDesc.Width      = Width;
    object->currentDesc.Height     = Height;
    object->currentDesc.MultiSampleType    = MultiSample;
    object->currentDesc.MultiSampleQuality = MultisampleQuality;
    object->glDescription.level            = Level;
    list_init(&object->overlays);

    /* Flags */
    object->Flags      = SFLAG_NORMCOORD; /* Default to normalized coords */
    object->Flags     |= Discard ? SFLAG_DISCARD : 0;
    object->Flags     |= (WINED3DFMT_D16_LOCKABLE == Format) ? SFLAG_LOCKABLE : 0;
    object->Flags     |= Lockable ? SFLAG_LOCKABLE : 0;

    TRACE("Pool %d %d %d %d\n",Pool, WINED3DPOOL_DEFAULT, WINED3DPOOL_MANAGED, WINED3DPOOL_SYSTEMMEM);

    /** Quick lockable sanity check TODO: remove this after surfaces, usage and lockability have been debugged properly
    * this function is too deep to need to care about things like this.
    * Levels need to be checked too, and possibly Type since they all affect what can be done.
    * ****************************************/
    switch(Pool) {
    case WINED3DPOOL_SCRATCH:
        if(!Lockable)
            FIXME("Create surface called with a pool of SCRATCH and a Lockable of FALSE "
                "which are mutually exclusive, setting lockable to TRUE\n");
                Lockable = TRUE;
    break;
    case WINED3DPOOL_SYSTEMMEM:
        if(!Lockable) FIXME("Create surface called with a pool of SYSTEMMEM and a Lockable of FALSE, "
                                    "this is acceptable but unexpected (I can't know how the surface can be usable!)\n");
    case WINED3DPOOL_MANAGED:
        if(Usage == WINED3DUSAGE_DYNAMIC) FIXME("Create surface called with a pool of MANAGED and a "
                                                "Usage of DYNAMIC which are mutually exclusive, not doing "
                                                "anything just telling you.\n");
    break;
    case WINED3DPOOL_DEFAULT: /*TODO: Create offscreen plain can cause this check to fail..., find out if it should */
        if(!(Usage & WINED3DUSAGE_DYNAMIC) && !(Usage & WINED3DUSAGE_RENDERTARGET)
           && !(Usage && WINED3DUSAGE_DEPTHSTENCIL ) && Lockable)
            WARN("Creating a surface with a POOL of DEFAULT with Lockable true, that doesn't specify DYNAMIC usage.\n");
    break;
    default:
        FIXME("(%p) Unknown pool %d\n", This, Pool);
    break;
    };

    if (Usage & WINED3DUSAGE_RENDERTARGET && Pool != WINED3DPOOL_DEFAULT) {
        FIXME("Trying to create a render target that isn't in the default pool\n");
    }

    /* mark the texture as dirty so that it gets loaded first time around*/
    surface_add_dirty_rect(*ppSurface, NULL);
    TRACE("(%p) : w(%d) h(%d) fmt(%d,%s) lockable(%d) surf@%p, surfmem@%p, %d bytes\n",
           This, Width, Height, Format, debug_d3dformat(Format),
           (WINED3DFMT_D16_LOCKABLE == Format), *ppSurface, object->resource.allocatedMemory, object->resource.size);

    list_init(&object->renderbuffers);

    /* Call the private setup routine */
    hr = IWineD3DSurface_PrivateSetup((IWineD3DSurface *)object);
    if (FAILED(hr))
    {
        ERR("Private setup failed, returning %#x\n", hr);
        IWineD3DSurface_Release(*ppSurface);
        *ppSurface = NULL;
        return hr;
    }

    return hr;
}

static HRESULT WINAPI IWineD3DDeviceImpl_CreateRendertargetView(IWineD3DDevice *iface,
        IWineD3DResource *resource, IUnknown *parent, IWineD3DRendertargetView **rendertarget_view)
{
    struct wined3d_rendertarget_view *object;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate memory\n");
        return E_OUTOFMEMORY;
    }

    object->vtbl = &wined3d_rendertarget_view_vtbl;
    object->refcount = 1;
    IWineD3DResource_AddRef(resource);
    object->resource = resource;
    object->parent = parent;

    *rendertarget_view = (IWineD3DRendertargetView *)object;

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_CreateTexture(IWineD3DDevice *iface,
        UINT Width, UINT Height, UINT Levels, DWORD Usage, WINED3DFORMAT Format, WINED3DPOOL Pool,
        IWineD3DTexture **ppTexture, HANDLE *pSharedHandle, IUnknown *parent)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    const struct GlPixelFormatDesc *format_desc = getFormatDescEntry(Format, &This->adapter->gl_info);
    IWineD3DTextureImpl *object;
    unsigned int i;
    UINT tmpW;
    UINT tmpH;
    HRESULT hr;
    unsigned int pow2Width;
    unsigned int pow2Height;

    TRACE("(%p) : Width %d, Height %d, Levels %d, Usage %#x\n", This, Width, Height, Levels, Usage);
    TRACE("Format %#x (%s), Pool %#x, ppTexture %p, pSharedHandle %p, parent %p\n",
            Format, debug_d3dformat(Format), Pool, ppTexture, pSharedHandle, parent);

    /* TODO: It should only be possible to create textures for formats
             that are reported as supported */
    if (WINED3DFMT_UNKNOWN >= Format) {
        WARN("(%p) : Texture cannot be created with a format of WINED3DFMT_UNKNOWN\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    /* Non-power2 support */
    if (GL_SUPPORT(ARB_TEXTURE_NON_POWER_OF_TWO))
    {
        pow2Width = Width;
        pow2Height = Height;
    }
    else
    {
        /* Find the nearest pow2 match */
        pow2Width = pow2Height = 1;
        while (pow2Width < Width) pow2Width <<= 1;
        while (pow2Height < Height) pow2Height <<= 1;

        if (pow2Width != Width || pow2Height != Height)
        {
            if (Levels > 1)
            {
                WARN("Attempted to create a mipmapped np2 texture without unconditional np2 support\n");
                return WINED3DERR_INVALIDCALL;
            }
            Levels = 1;
        }
    }

    /* Calculate levels for mip mapping */
    if (Usage & WINED3DUSAGE_AUTOGENMIPMAP)
    {
        if (!GL_SUPPORT(SGIS_GENERATE_MIPMAP))
        {
            WARN("No mipmap generation support, returning D3DERR_INVALIDCALL\n");
            return WINED3DERR_INVALIDCALL;
        }

        if (Levels > 1)
        {
            WARN("D3DUSAGE_AUTOGENMIPMAP is set, and level count > 1, returning D3DERR_INVALIDCALL\n");
            return WINED3DERR_INVALIDCALL;
        }

        Levels = 1;
    }
    else if (!Levels)
    {
        Levels = wined3d_log2i(max(Width, Height)) + 1;
        TRACE("Calculated levels = %d\n", Levels);
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Out of memory\n");
        *ppTexture = NULL;
        return WINED3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &IWineD3DTexture_Vtbl;
    hr = resource_init(&object->resource, WINED3DRTYPE_TEXTURE, This, 0, Usage, format_desc, Pool, parent);
    if (FAILED(hr))
    {
        WARN("Failed to initialize resource, returning %#x\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        *ppTexture = NULL;
        return hr;
    }

    TRACE("(%p) : Created resource %p\n", This, object);

    IWineD3DDeviceImpl_AddResource(iface, (IWineD3DResource *)object);

    *ppTexture = (IWineD3DTexture *)object;

    basetexture_init(&object->baseTexture, Levels, Usage);

    if (object->resource.format_desc->Flags & WINED3DFMT_FLAG_FILTERING)
    {
        object->baseTexture.minMipLookup = minMipLookup;
        object->baseTexture.magLookup    = magLookup;
    } else {
        object->baseTexture.minMipLookup = minMipLookup_noFilter;
        object->baseTexture.magLookup    = magLookup_noFilter;
    }

    /** FIXME: add support for real non-power-two if it's provided by the video card **/
    /* Precalculated scaling for 'faked' non power of two texture coords.
       Second also don't use ARB_TEXTURE_RECTANGLE in case the surface format is P8 and EXT_PALETTED_TEXTURE
       is used in combination with texture uploads (RTL_READTEX/RTL_TEXTEX). The reason is that EXT_PALETTED_TEXTURE
       doesn't work in combination with ARB_TEXTURE_RECTANGLE.
    */
    if(GL_SUPPORT(WINE_NORMALIZED_TEXRECT) && (Width != pow2Width || Height != pow2Height)) {
        object->baseTexture.pow2Matrix[0] =  1.0;
        object->baseTexture.pow2Matrix[5] =  1.0;
        object->baseTexture.pow2Matrix[10] = 1.0;
        object->baseTexture.pow2Matrix[15] = 1.0;
        object->target = GL_TEXTURE_2D;
        object->cond_np2 = TRUE;
        object->baseTexture.minMipLookup = minMipLookup_noFilter;
    } else if(GL_SUPPORT(ARB_TEXTURE_RECTANGLE) &&
       (Width != pow2Width || Height != pow2Height) &&
       !((Format == WINED3DFMT_P8) && GL_SUPPORT(EXT_PALETTED_TEXTURE) && (wined3d_settings.rendertargetlock_mode == RTL_READTEX || wined3d_settings.rendertargetlock_mode == RTL_TEXTEX)))
    {
        if ((Width != 1) || (Height != 1)) {
            object->baseTexture.pow2Matrix_identity = FALSE;
        }

        object->baseTexture.pow2Matrix[0] =  (float)Width;
        object->baseTexture.pow2Matrix[5] =  (float)Height;
        object->baseTexture.pow2Matrix[10] = 1.0;
        object->baseTexture.pow2Matrix[15] = 1.0;
        object->target = GL_TEXTURE_RECTANGLE_ARB;
        object->cond_np2 = TRUE;
        object->baseTexture.minMipLookup = minMipLookup_noFilter;
    } else {
        if ((Width != pow2Width) || (Height != pow2Height)) {
            object->baseTexture.pow2Matrix_identity = FALSE;
            object->baseTexture.pow2Matrix[0] =  (((float)Width)  / ((float)pow2Width));
            object->baseTexture.pow2Matrix[5] =  (((float)Height) / ((float)pow2Height));
        } else {
            object->baseTexture.pow2Matrix[0] =  1.0;
            object->baseTexture.pow2Matrix[5] =  1.0;
        }

        object->baseTexture.pow2Matrix[10] = 1.0;
        object->baseTexture.pow2Matrix[15] = 1.0;
        object->target = GL_TEXTURE_2D;
        object->cond_np2 = FALSE;
    }
    TRACE(" xf(%f) yf(%f)\n", object->baseTexture.pow2Matrix[0], object->baseTexture.pow2Matrix[5]);

    /* Generate all the surfaces */
    tmpW = Width;
    tmpH = Height;
    for (i = 0; i < object->baseTexture.levels; i++)
    {
        /* use the callback to create the texture surface */
        hr = IWineD3DDeviceParent_CreateSurface(This->device_parent, parent, tmpW, tmpH, Format,
                Usage, Pool, i, WINED3DCUBEMAP_FACE_POSITIVE_X, &object->surfaces[i]);
        if (hr!= WINED3D_OK || ( (IWineD3DSurfaceImpl *) object->surfaces[i])->Flags & SFLAG_OVERSIZE) {
            FIXME("Failed to create surface  %p\n", object);
            /* clean up */
            object->surfaces[i] = NULL;
            IWineD3DTexture_Release((IWineD3DTexture *)object);

            *ppTexture = NULL;
            return hr;
        }

        IWineD3DSurface_SetContainer(object->surfaces[i], (IWineD3DBase *)object);
        TRACE("Created surface level %d @ %p\n", i, object->surfaces[i]);
        surface_set_texture_target(object->surfaces[i], object->target);
        /* calculate the next mipmap level */
        tmpW = max(1, tmpW >> 1);
        tmpH = max(1, tmpH >> 1);
    }
    object->baseTexture.internal_preload = texture_internal_preload;

    TRACE("(%p) : Created  texture %p\n", This, object);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_CreateVolumeTexture(IWineD3DDevice *iface,
        UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, WINED3DFORMAT Format, WINED3DPOOL Pool,
        IWineD3DVolumeTexture **ppVolumeTexture, HANDLE *pSharedHandle, IUnknown *parent)
{
    IWineD3DDeviceImpl        *This = (IWineD3DDeviceImpl *)iface;
    const struct GlPixelFormatDesc *format_desc = getFormatDescEntry(Format, &This->adapter->gl_info);
    IWineD3DVolumeTextureImpl *object;
    unsigned int               i;
    UINT                       tmpW;
    UINT                       tmpH;
    UINT                       tmpD;
    HRESULT hr;

    /* TODO: It should only be possible to create textures for formats 
             that are reported as supported */
    if (WINED3DFMT_UNKNOWN >= Format) {
        WARN("(%p) : Texture cannot be created with a format of WINED3DFMT_UNKNOWN\n", This);
        return WINED3DERR_INVALIDCALL;
    }
    if(!GL_SUPPORT(EXT_TEXTURE3D)) {
        WARN("(%p) : Texture cannot be created - no volume texture support\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    /* Calculate levels for mip mapping */
    if (Usage & WINED3DUSAGE_AUTOGENMIPMAP)
    {
        if (!GL_SUPPORT(SGIS_GENERATE_MIPMAP))
        {
            WARN("No mipmap generation support, returning D3DERR_INVALIDCALL\n");
            return WINED3DERR_INVALIDCALL;
        }

        if (Levels > 1)
        {
            WARN("D3DUSAGE_AUTOGENMIPMAP is set, and level count > 1, returning D3DERR_INVALIDCALL\n");
            return WINED3DERR_INVALIDCALL;
        }

        Levels = 1;
    }
    else if (!Levels)
    {
        Levels = wined3d_log2i(max(max(Width, Height), Depth)) + 1;
        TRACE("Calculated levels = %d\n", Levels);
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Out of memory\n");
        *ppVolumeTexture = NULL;
        return WINED3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &IWineD3DVolumeTexture_Vtbl;
    hr = resource_init(&object->resource, WINED3DRTYPE_VOLUMETEXTURE, This, 0, Usage, format_desc, Pool, parent);
    if (FAILED(hr))
    {
        WARN("Failed to initialize resource, returning %#x\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        *ppVolumeTexture = NULL;
        return hr;
    }

    TRACE("(%p) : Created resource %p\n", This, object);

    IWineD3DDeviceImpl_AddResource(iface, (IWineD3DResource *)object);

    basetexture_init(&object->baseTexture, Levels, Usage);

    TRACE("(%p) : W(%d) H(%d) D(%d), Lvl(%d) Usage(%d), Fmt(%u,%s), Pool(%s)\n", This, Width, Height,
          Depth, Levels, Usage, Format, debug_d3dformat(Format), debug_d3dpool(Pool));

    /* Is NP2 support for volumes needed? */
    object->baseTexture.pow2Matrix[ 0] = 1.0;
    object->baseTexture.pow2Matrix[ 5] = 1.0;
    object->baseTexture.pow2Matrix[10] = 1.0;
    object->baseTexture.pow2Matrix[15] = 1.0;

    if (object->resource.format_desc->Flags & WINED3DFMT_FLAG_FILTERING)
    {
        object->baseTexture.minMipLookup = minMipLookup;
        object->baseTexture.magLookup    = magLookup;
    } else {
        object->baseTexture.minMipLookup = minMipLookup_noFilter;
        object->baseTexture.magLookup    = magLookup_noFilter;
    }

    /* Generate all the surfaces */
    tmpW = Width;
    tmpH = Height;
    tmpD = Depth;

    for (i = 0; i < object->baseTexture.levels; i++)
    {
        HRESULT hr;
        /* Create the volume */
        hr = IWineD3DDeviceParent_CreateVolume(This->device_parent, parent,
                tmpW, tmpH, tmpD, Format, Pool, Usage, &object->volumes[i]);
        if(FAILED(hr)) {
            ERR("Creating a volume for the volume texture failed(%08x)\n", hr);
            IWineD3DVolumeTexture_Release((IWineD3DVolumeTexture *) object);
            *ppVolumeTexture = NULL;
            return hr;
        }

        /* Set its container to this object */
        IWineD3DVolume_SetContainer(object->volumes[i], (IWineD3DBase *)object);

        /* calculate the next mipmap level */
        tmpW = max(1, tmpW >> 1);
        tmpH = max(1, tmpH >> 1);
        tmpD = max(1, tmpD >> 1);
    }
    object->baseTexture.internal_preload = volumetexture_internal_preload;

    *ppVolumeTexture = (IWineD3DVolumeTexture *) object;
    TRACE("(%p) : Created volume texture %p\n", This, object);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_CreateVolume(IWineD3DDevice *iface,
                                               UINT Width, UINT Height, UINT Depth,
                                               DWORD Usage,
                                               WINED3DFORMAT Format, WINED3DPOOL Pool,
                                               IWineD3DVolume** ppVolume,
                                               HANDLE* pSharedHandle, IUnknown *parent) {

    IWineD3DDeviceImpl        *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DVolumeImpl        *object; /** NOTE: impl ref allowed since this is a create function **/
    const struct GlPixelFormatDesc *format_desc = getFormatDescEntry(Format, &GLINFO_LOCATION);
    HRESULT hr;

    if(!GL_SUPPORT(EXT_TEXTURE3D)) {
        WARN("(%p) : Volume cannot be created - no volume texture support\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Out of memory\n");
        *ppVolume = NULL;
        return WINED3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &IWineD3DVolume_Vtbl;
    hr = resource_init(&object->resource, WINED3DRTYPE_VOLUME, This,
            Width * Height * Depth * format_desc->byte_count, Usage, format_desc, Pool, parent);
    if (FAILED(hr))
    {
        WARN("Failed to initialize resource, returning %#x\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        *ppVolume = NULL;
        return hr;
    }

    TRACE("(%p) : Created resource %p\n", This, object);

    IWineD3DDeviceImpl_AddResource(iface, (IWineD3DResource *)object);

    *ppVolume = (IWineD3DVolume *)object;

    TRACE("(%p) : W(%d) H(%d) D(%d), Usage(%d), Fmt(%u,%s), Pool(%s)\n", This, Width, Height,
          Depth, Usage, Format, debug_d3dformat(Format), debug_d3dpool(Pool));

    object->currentDesc.Width   = Width;
    object->currentDesc.Height  = Height;
    object->currentDesc.Depth   = Depth;

    /** Note: Volume textures cannot be dxtn, hence no need to check here **/
    object->lockable            = TRUE;
    object->locked              = FALSE;
    memset(&object->lockedBox, 0, sizeof(WINED3DBOX));
    object->dirty               = TRUE;

    volume_add_dirty_box((IWineD3DVolume *)object, NULL);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_CreateCubeTexture(IWineD3DDevice *iface,
        UINT EdgeLength, UINT Levels, DWORD Usage, WINED3DFORMAT Format, WINED3DPOOL Pool,
        IWineD3DCubeTexture **ppCubeTexture, HANDLE *pSharedHandle, IUnknown *parent)
{
    IWineD3DDeviceImpl      *This = (IWineD3DDeviceImpl *)iface;
    const struct GlPixelFormatDesc *format_desc = getFormatDescEntry(Format, &This->adapter->gl_info);
    IWineD3DCubeTextureImpl *object; /** NOTE: impl ref allowed since this is a create function **/
    unsigned int             i, j;
    UINT                     tmpW;
    HRESULT                  hr;
    unsigned int pow2EdgeLength;

    /* TODO: It should only be possible to create textures for formats 
             that are reported as supported */
    if (WINED3DFMT_UNKNOWN >= Format) {
        WARN("(%p) : Texture cannot be created with a format of WINED3DFMT_UNKNOWN\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    if (!GL_SUPPORT(ARB_TEXTURE_CUBE_MAP) && Pool != WINED3DPOOL_SCRATCH) {
        WARN("(%p) : Tried to create not supported cube texture\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    /* Calculate levels for mip mapping */
    if (Usage & WINED3DUSAGE_AUTOGENMIPMAP)
    {
        if (!GL_SUPPORT(SGIS_GENERATE_MIPMAP))
        {
            WARN("No mipmap generation support, returning D3DERR_INVALIDCALL\n");
            return WINED3DERR_INVALIDCALL;
        }

        if (Levels > 1)
        {
            WARN("D3DUSAGE_AUTOGENMIPMAP is set, and level count > 1, returning D3DERR_INVALIDCALL\n");
            return WINED3DERR_INVALIDCALL;
        }

        Levels = 1;
    }
    else if (!Levels)
    {
        Levels = wined3d_log2i(EdgeLength) + 1;
        TRACE("Calculated levels = %d\n", Levels);
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Out of memory\n");
        *ppCubeTexture = NULL;
        return WINED3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &IWineD3DCubeTexture_Vtbl;
    hr = resource_init(&object->resource, WINED3DRTYPE_CUBETEXTURE, This, 0, Usage, format_desc, Pool, parent);
    if (FAILED(hr))
    {
        WARN("Failed to initialize resource, returning %#x\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        *ppCubeTexture = NULL;
        return hr;
    }

    TRACE("(%p) : Created resource %p\n", This, object);

    IWineD3DDeviceImpl_AddResource(iface, (IWineD3DResource *)object);

    basetexture_init(&object->baseTexture, Levels, Usage);

    TRACE("(%p) Create Cube Texture\n", This);

    /* Find the nearest pow2 match */
    pow2EdgeLength = 1;
    while (pow2EdgeLength < EdgeLength) pow2EdgeLength <<= 1;

    if (GL_SUPPORT(ARB_TEXTURE_NON_POWER_OF_TWO) || (EdgeLength == pow2EdgeLength)) {
        /* Precalculated scaling for 'faked' non power of two texture coords */
        object->baseTexture.pow2Matrix[ 0] = 1.0;
        object->baseTexture.pow2Matrix[ 5] = 1.0;
        object->baseTexture.pow2Matrix[10] = 1.0;
        object->baseTexture.pow2Matrix[15] = 1.0;
    } else {
        /* Precalculated scaling for 'faked' non power of two texture coords */
        object->baseTexture.pow2Matrix[ 0] = ((float)EdgeLength) / ((float)pow2EdgeLength);
        object->baseTexture.pow2Matrix[ 5] = ((float)EdgeLength) / ((float)pow2EdgeLength);
        object->baseTexture.pow2Matrix[10] = ((float)EdgeLength) / ((float)pow2EdgeLength);
        object->baseTexture.pow2Matrix[15] = 1.0;
        object->baseTexture.pow2Matrix_identity = FALSE;
    }

    if (object->resource.format_desc->Flags & WINED3DFMT_FLAG_FILTERING)
    {
        object->baseTexture.minMipLookup = minMipLookup;
        object->baseTexture.magLookup    = magLookup;
    } else {
        object->baseTexture.minMipLookup = minMipLookup_noFilter;
        object->baseTexture.magLookup    = magLookup_noFilter;
    }

    /* Generate all the surfaces */
    tmpW = EdgeLength;
    for (i = 0; i < object->baseTexture.levels; i++) {

        /* Create the 6 faces */
        for (j = 0; j < 6; j++) {
            static const GLenum cube_targets[6] = {
                GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,
                GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,
                GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,
                GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,
                GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,
                GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB
            };

            hr = IWineD3DDeviceParent_CreateSurface(This->device_parent, parent, tmpW, tmpW,
                    Format, Usage, Pool, i /* Level */, j, &object->surfaces[j][i]);
            if (FAILED(hr))
            {
                FIXME("(%p) Failed to create surface\n",object);
                IWineD3DCubeTexture_Release((IWineD3DCubeTexture *)object);
                *ppCubeTexture = NULL;
                return hr;
            }
            IWineD3DSurface_SetContainer(object->surfaces[j][i], (IWineD3DBase *)object);
            TRACE("Created surface level %d @ %p,\n", i, object->surfaces[j][i]);
            surface_set_texture_target(object->surfaces[j][i], cube_targets[j]);
        }
        tmpW = max(1, tmpW >> 1);
    }
    object->baseTexture.internal_preload = cubetexture_internal_preload;

    TRACE("(%p) : Created Cube Texture %p\n", This, object);
    *ppCubeTexture = (IWineD3DCubeTexture *) object;
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_CreateQuery(IWineD3DDevice *iface, WINED3DQUERYTYPE Type, IWineD3DQuery **ppQuery, IUnknown* parent) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DQueryImpl *object; /*NOTE: impl ref allowed since this is a create function */
    HRESULT hr = WINED3DERR_NOTAVAILABLE;
    const IWineD3DQueryVtbl *vtable;

    /* Just a check to see if we support this type of query */
    switch(Type) {
    case WINED3DQUERYTYPE_OCCLUSION:
        TRACE("(%p) occlusion query\n", This);
        if (GL_SUPPORT(ARB_OCCLUSION_QUERY))
            hr = WINED3D_OK;
        else
            WARN("Unsupported in local OpenGL implementation: ARB_OCCLUSION_QUERY/NV_OCCLUSION_QUERY\n");

        vtable = &IWineD3DOcclusionQuery_Vtbl;
        break;

    case WINED3DQUERYTYPE_EVENT:
        if(!(GL_SUPPORT(NV_FENCE) || GL_SUPPORT(APPLE_FENCE) )) {
            /* Half-Life 2 needs this query. It does not render the main menu correctly otherwise
             * Pretend to support it, faking this query does not do much harm except potentially lowering performance
             */
            FIXME("(%p) Event query: Unimplemented, but pretending to be supported\n", This);
        }
        vtable = &IWineD3DEventQuery_Vtbl;
        hr = WINED3D_OK;
        break;

    case WINED3DQUERYTYPE_VCACHE:
    case WINED3DQUERYTYPE_RESOURCEMANAGER:
    case WINED3DQUERYTYPE_VERTEXSTATS:
    case WINED3DQUERYTYPE_TIMESTAMP:
    case WINED3DQUERYTYPE_TIMESTAMPDISJOINT:
    case WINED3DQUERYTYPE_TIMESTAMPFREQ:
    case WINED3DQUERYTYPE_PIPELINETIMINGS:
    case WINED3DQUERYTYPE_INTERFACETIMINGS:
    case WINED3DQUERYTYPE_VERTEXTIMINGS:
    case WINED3DQUERYTYPE_PIXELTIMINGS:
    case WINED3DQUERYTYPE_BANDWIDTHTIMINGS:
    case WINED3DQUERYTYPE_CACHEUTILIZATION:
    default:
        /* Use the base Query vtable until we have a special one for each query */
        vtable = &IWineD3DQuery_Vtbl;
        FIXME("(%p) Unhandled query type %d\n", This, Type);
    }
    if(NULL == ppQuery || hr != WINED3D_OK) {
        return hr;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if(!object)
    {
        ERR("Out of memory\n");
        *ppQuery = NULL;
        return WINED3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = vtable;
    object->type = Type;
    object->state = QUERY_CREATED;
    object->wineD3DDevice = This;
    object->parent = parent;
    object->ref = 1;

    *ppQuery = (IWineD3DQuery *)object;

    /* allocated the 'extended' data based on the type of query requested */
    switch(Type){
    case WINED3DQUERYTYPE_OCCLUSION:
        object->extendedData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WineQueryOcclusionData));
        ((WineQueryOcclusionData *)(object->extendedData))->ctx = This->activeContext;

        if(GL_SUPPORT(ARB_OCCLUSION_QUERY)) {
            TRACE("(%p) Allocating data for an occlusion query\n", This);

            ActivateContext(This, This->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
            ENTER_GL();
            GL_EXTCALL(glGenQueriesARB(1, &((WineQueryOcclusionData *)(object->extendedData))->queryId));
            LEAVE_GL();
            break;
        }
    case WINED3DQUERYTYPE_EVENT:
        object->extendedData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WineQueryEventData));
        ((WineQueryEventData *)(object->extendedData))->ctx = This->activeContext;

        ActivateContext(This, This->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
        ENTER_GL();
        if(GL_SUPPORT(APPLE_FENCE)) {
            GL_EXTCALL(glGenFencesAPPLE(1, &((WineQueryEventData *)(object->extendedData))->fenceId));
            checkGLcall("glGenFencesAPPLE");
        } else if(GL_SUPPORT(NV_FENCE)) {
            GL_EXTCALL(glGenFencesNV(1, &((WineQueryEventData *)(object->extendedData))->fenceId));
            checkGLcall("glGenFencesNV");
        }
        LEAVE_GL();
        break;

    case WINED3DQUERYTYPE_VCACHE:
    case WINED3DQUERYTYPE_RESOURCEMANAGER:
    case WINED3DQUERYTYPE_VERTEXSTATS:
    case WINED3DQUERYTYPE_TIMESTAMP:
    case WINED3DQUERYTYPE_TIMESTAMPDISJOINT:
    case WINED3DQUERYTYPE_TIMESTAMPFREQ:
    case WINED3DQUERYTYPE_PIPELINETIMINGS:
    case WINED3DQUERYTYPE_INTERFACETIMINGS:
    case WINED3DQUERYTYPE_VERTEXTIMINGS:
    case WINED3DQUERYTYPE_PIXELTIMINGS:
    case WINED3DQUERYTYPE_BANDWIDTHTIMINGS:
    case WINED3DQUERYTYPE_CACHEUTILIZATION:
    default:
        object->extendedData = 0;
        FIXME("(%p) Unhandled query type %d\n",This , Type);
    }
    TRACE("(%p) : Created Query %p\n", This, object);
    return WINED3D_OK;
}

/*****************************************************************************
 * IWineD3DDeviceImpl_SetupFullscreenWindow
 *
 * Helper function that modifies a HWND's Style and ExStyle for proper
 * fullscreen use.
 *
 * Params:
 *  iface: Pointer to the IWineD3DDevice interface
 *  window: Window to setup
 *
 *****************************************************************************/
static LONG fullscreen_style(LONG orig_style) {
    LONG style = orig_style;
    style &= ~WS_CAPTION;
    style &= ~WS_THICKFRAME;

    /* Make sure the window is managed, otherwise we won't get keyboard input */
    style |= WS_POPUP | WS_SYSMENU;

    return style;
}

static LONG fullscreen_exStyle(LONG orig_exStyle) {
    LONG exStyle = orig_exStyle;

    /* Filter out window decorations */
    exStyle &= ~WS_EX_WINDOWEDGE;
    exStyle &= ~WS_EX_CLIENTEDGE;

    return exStyle;
}

static void IWineD3DDeviceImpl_SetupFullscreenWindow(IWineD3DDevice *iface, HWND window, UINT w, UINT h) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    LONG style, exStyle;
    /* Don't do anything if an original style is stored.
     * That shouldn't happen
     */
    TRACE("(%p): Setting up window %p for exclusive mode\n", This, window);
    if (This->style || This->exStyle) {
        ERR("(%p): Want to change the window parameters of HWND %p, but "
            "another style is stored for restoration afterwards\n", This, window);
    }

    /* Get the parameters and save them */
    style = GetWindowLongW(window, GWL_STYLE);
    exStyle = GetWindowLongW(window, GWL_EXSTYLE);
    This->style = style;
    This->exStyle = exStyle;

    style = fullscreen_style(style);
    exStyle = fullscreen_exStyle(exStyle);

    TRACE("Old style was %08x,%08x, setting to %08x,%08x\n",
          This->style, This->exStyle, style, exStyle);

    SetWindowLongW(window, GWL_STYLE, style);
    SetWindowLongW(window, GWL_EXSTYLE, exStyle);

    /* Inform the window about the update. */
    SetWindowPos(window, HWND_TOP, 0, 0,
                 w, h, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}

/*****************************************************************************
 * IWineD3DDeviceImpl_RestoreWindow
 *
 * Helper function that restores a windows' properties when taking it out
 * of fullscreen mode
 *
 * Params:
 *  iface: Pointer to the IWineD3DDevice interface
 *  window: Window to setup
 *
 *****************************************************************************/
static void IWineD3DDeviceImpl_RestoreWindow(IWineD3DDevice *iface, HWND window) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    LONG style, exStyle;

    /* This could be a DDSCL_NORMAL -> DDSCL_NORMAL
     * switch, do nothing
     */
    if (!This->style && !This->exStyle) return;

    TRACE("(%p): Restoring window settings of window %p to %08x, %08x\n",
          This, window, This->style, This->exStyle);

    style = GetWindowLongW(window, GWL_STYLE);
    exStyle = GetWindowLongW(window, GWL_EXSTYLE);

    /* Only restore the style if the application didn't modify it during the fullscreen phase.
     * Some applications change it before calling Reset() when switching between windowed and
     * fullscreen modes(HL2), some depend on the original style(Eve Online)
     */
    if(style == fullscreen_style(This->style) &&
       exStyle == fullscreen_style(This->exStyle)) {
        SetWindowLongW(window, GWL_STYLE, This->style);
        SetWindowLongW(window, GWL_EXSTYLE, This->exStyle);
    }

    /* Delete the old values */
    This->style = 0;
    This->exStyle = 0;

    /* Inform the window about the update */
    SetWindowPos(window, 0 /* InsertAfter, ignored */,
                 0, 0, 0, 0, /* Pos, Size, ignored */
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
}

/* example at http://www.fairyengine.com/articles/dxmultiviews.htm */
static HRESULT WINAPI IWineD3DDeviceImpl_CreateSwapChain(IWineD3DDevice *iface,
        WINED3DPRESENT_PARAMETERS *pPresentationParameters, IWineD3DSwapChain **ppSwapChain,
        IUnknown *parent, WINED3DSURFTYPE surface_type)
{
    IWineD3DDeviceImpl      *This = (IWineD3DDeviceImpl *)iface;

    HDC                     hDc;
    IWineD3DSwapChainImpl  *object; /** NOTE: impl ref allowed since this is a create function **/
    HRESULT                 hr;
    IUnknown               *bufferParent;
    BOOL                    displaymode_set = FALSE;
    WINED3DDISPLAYMODE      Mode;
    const struct GlPixelFormatDesc *format_desc;

    TRACE("(%p) : Created Additional Swap Chain\n", This);

   /** FIXME: Test under windows to find out what the life cycle of a swap chain is,
   * does a device hold a reference to a swap chain giving them a lifetime of the device
   * or does the swap chain notify the device of its destruction.
    *******************************/

    /* Check the params */
    if(pPresentationParameters->BackBufferCount > WINED3DPRESENT_BACK_BUFFER_MAX) {
        ERR("App requested %d back buffers, this is not supported for now\n", pPresentationParameters->BackBufferCount);
        return WINED3DERR_INVALIDCALL;
    } else if (pPresentationParameters->BackBufferCount > 1) {
        FIXME("The app requests more than one back buffer, this can't be supported properly. Please configure the application to use double buffering(=1 back buffer) if possible\n");
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if(!object)
    {
        ERR("Out of memory\n");
        *ppSwapChain = NULL;
        return WINED3DERR_OUTOFVIDEOMEMORY;
    }

    switch(surface_type) {
        case SURFACE_GDI:
            object->lpVtbl = &IWineGDISwapChain_Vtbl;
            break;
        case SURFACE_OPENGL:
            object->lpVtbl = &IWineD3DSwapChain_Vtbl;
            break;
        case SURFACE_UNKNOWN:
            FIXME("Caller tried to create a SURFACE_UNKNOWN swapchain\n");
            HeapFree(GetProcessHeap(), 0, object);
            return WINED3DERR_INVALIDCALL;
    }
    object->wineD3DDevice = This;
    object->parent = parent;
    object->ref = 1;

    *ppSwapChain = (IWineD3DSwapChain *)object;

    /*********************
    * Lookup the window Handle and the relating X window handle
    ********************/

    /* Setup hwnd we are using, plus which display this equates to */
    object->win_handle = pPresentationParameters->hDeviceWindow;
    if (!object->win_handle) {
        object->win_handle = This->createParms.hFocusWindow;
    }
    if(!pPresentationParameters->Windowed && object->win_handle) {
        IWineD3DDeviceImpl_SetupFullscreenWindow(iface, object->win_handle,
                                                 pPresentationParameters->BackBufferWidth,
                                                 pPresentationParameters->BackBufferHeight);
    }

    hDc                = GetDC(object->win_handle);
    TRACE("Using hDc %p\n", hDc);

    if (NULL == hDc) {
        WARN("Failed to get a HDc for Window %p\n", object->win_handle);
        return WINED3DERR_NOTAVAILABLE;
    }

    /* Get info on the current display setup */
    IWineD3D_GetAdapterDisplayMode(This->wineD3D, This->adapter->num, &Mode);
    object->orig_width = Mode.Width;
    object->orig_height = Mode.Height;
    object->orig_fmt = Mode.Format;
    format_desc = getFormatDescEntry(Mode.Format, &GLINFO_LOCATION);

    if (pPresentationParameters->Windowed &&
        ((pPresentationParameters->BackBufferWidth == 0) ||
         (pPresentationParameters->BackBufferHeight == 0) ||
         (pPresentationParameters->BackBufferFormat == WINED3DFMT_UNKNOWN))) {

        RECT Rect;
        GetClientRect(object->win_handle, &Rect);

        if (pPresentationParameters->BackBufferWidth == 0) {
           pPresentationParameters->BackBufferWidth = Rect.right;
           TRACE("Updating width to %d\n", pPresentationParameters->BackBufferWidth);
        }
        if (pPresentationParameters->BackBufferHeight == 0) {
           pPresentationParameters->BackBufferHeight = Rect.bottom;
           TRACE("Updating height to %d\n", pPresentationParameters->BackBufferHeight);
        }
        if (pPresentationParameters->BackBufferFormat == WINED3DFMT_UNKNOWN) {
           pPresentationParameters->BackBufferFormat = object->orig_fmt;
           TRACE("Updating format to %s\n", debug_d3dformat(object->orig_fmt));
        }
    }

    /* Put the correct figures in the presentation parameters */
    TRACE("Copying across presentation parameters\n");
    object->presentParms = *pPresentationParameters;

    TRACE("calling rendertarget CB\n");
    hr = IWineD3DDeviceParent_CreateRenderTarget(This->device_parent, parent,
            object->presentParms.BackBufferWidth, object->presentParms.BackBufferHeight,
            object->presentParms.BackBufferFormat, object->presentParms.MultiSampleType,
            object->presentParms.MultiSampleQuality, TRUE /* Lockable */, &object->frontBuffer);
    if (SUCCEEDED(hr)) {
        IWineD3DSurface_SetContainer(object->frontBuffer, (IWineD3DBase *)object);
        ((IWineD3DSurfaceImpl *)object->frontBuffer)->Flags |= SFLAG_SWAPCHAIN;
        if(surface_type == SURFACE_OPENGL) {
            IWineD3DSurface_ModifyLocation(object->frontBuffer, SFLAG_INDRAWABLE, TRUE);
        }
    } else {
        ERR("Failed to create the front buffer\n");
        goto error;
    }

   /*********************
   * Windowed / Fullscreen
   *******************/

   /**
   * TODO: MSDN says that we are only allowed one fullscreen swapchain per device,
   * so we should really check to see if there is a fullscreen swapchain already
   * I think Windows and X have different ideas about fullscreen, does a single head count as full screen?
    **************************************/

   if (!pPresentationParameters->Windowed) {
        WINED3DDISPLAYMODE mode;


        /* Change the display settings */
        mode.Width = pPresentationParameters->BackBufferWidth;
        mode.Height = pPresentationParameters->BackBufferHeight;
        mode.Format = pPresentationParameters->BackBufferFormat;
        mode.RefreshRate = pPresentationParameters->FullScreen_RefreshRateInHz;

        IWineD3DDevice_SetDisplayMode(iface, 0, &mode);
        displaymode_set = TRUE;
    }

        /**
     * Create an opengl context for the display visual
     *  NOTE: the visual is chosen as the window is created and the glcontext cannot
     *     use different properties after that point in time. FIXME: How to handle when requested format
     *     doesn't match actual visual? Cannot choose one here - code removed as it ONLY works if the one
     *     it chooses is identical to the one already being used!
         **********************************/
    /** FIXME: Handle stencil appropriately via EnableAutoDepthStencil / AutoDepthStencilFormat **/

    object->context = HeapAlloc(GetProcessHeap(), 0, sizeof(object->context));
    if(!object->context) {
        ERR("Failed to create the context array\n");
        hr = E_OUTOFMEMORY;
        goto error;
    }
    object->num_contexts = 1;

    if(surface_type == SURFACE_OPENGL) {
        object->context[0] = CreateContext(This, (IWineD3DSurfaceImpl *) object->frontBuffer, object->win_handle, FALSE /* pbuffer */, pPresentationParameters);
        if (!object->context[0]) {
            ERR("Failed to create a new context\n");
            hr = WINED3DERR_NOTAVAILABLE;
            goto error;
        } else {
            TRACE("Context created (HWND=%p, glContext=%p)\n",
                object->win_handle, object->context[0]->glCtx);
        }
    }

   /*********************
   * Create the back, front and stencil buffers
   *******************/
    if(object->presentParms.BackBufferCount > 0) {
        UINT i;

        object->backBuffer = HeapAlloc(GetProcessHeap(), 0, sizeof(IWineD3DSurface *) * object->presentParms.BackBufferCount);
        if(!object->backBuffer) {
            ERR("Out of memory\n");
            hr = E_OUTOFMEMORY;
            goto error;
        }

        for(i = 0; i < object->presentParms.BackBufferCount; i++) {
            TRACE("calling rendertarget CB\n");
            hr = IWineD3DDeviceParent_CreateRenderTarget(This->device_parent, parent,
                    object->presentParms.BackBufferWidth, object->presentParms.BackBufferHeight,
                    object->presentParms.BackBufferFormat, object->presentParms.MultiSampleType,
                    object->presentParms.MultiSampleQuality, TRUE /* Lockable */, &object->backBuffer[i]);
            if(SUCCEEDED(hr)) {
                IWineD3DSurface_SetContainer(object->backBuffer[i], (IWineD3DBase *)object);
                ((IWineD3DSurfaceImpl *)object->backBuffer[i])->Flags |= SFLAG_SWAPCHAIN;
            } else {
                ERR("Cannot create new back buffer\n");
                goto error;
            }
            if(surface_type == SURFACE_OPENGL) {
                ENTER_GL();
                glDrawBuffer(GL_BACK);
                checkGLcall("glDrawBuffer(GL_BACK)");
                LEAVE_GL();
            }
        }
    } else {
        object->backBuffer = NULL;

        /* Single buffering - draw to front buffer */
        if(surface_type == SURFACE_OPENGL) {
            ENTER_GL();
            glDrawBuffer(GL_FRONT);
            checkGLcall("glDrawBuffer(GL_FRONT)");
            LEAVE_GL();
        }
    }

    /* Under directX swapchains share the depth stencil, so only create one depth-stencil */
    if (pPresentationParameters->EnableAutoDepthStencil && surface_type == SURFACE_OPENGL) {
        TRACE("Creating depth stencil buffer\n");
        if (This->auto_depth_stencil_buffer == NULL ) {
            hr = IWineD3DDeviceParent_CreateDepthStencilSurface(This->device_parent, parent,
                    object->presentParms.BackBufferWidth, object->presentParms.BackBufferHeight,
                    object->presentParms.AutoDepthStencilFormat, object->presentParms.MultiSampleType,
                    object->presentParms.MultiSampleQuality, FALSE /* FIXME: Discard */,
                    &This->auto_depth_stencil_buffer);
            if (SUCCEEDED(hr)) {
                IWineD3DSurface_SetContainer(This->auto_depth_stencil_buffer, 0);
            } else {
                ERR("Failed to create the auto depth stencil\n");
                goto error;
            }
        }
    }

    IWineD3DSwapChain_GetGammaRamp((IWineD3DSwapChain *) object, &object->orig_gamma);

    TRACE("Created swapchain %p\n", object);
    TRACE("FrontBuf @ %p, BackBuf @ %p, DepthStencil %d\n",object->frontBuffer, object->backBuffer ? object->backBuffer[0] : NULL, pPresentationParameters->EnableAutoDepthStencil);
    return WINED3D_OK;

error:
    if (displaymode_set) {
        DEVMODEW devmode;
        RECT     clip_rc;

        SetRect(&clip_rc, 0, 0, object->orig_width, object->orig_height);
        ClipCursor(NULL);

        /* Change the display settings */
        memset(&devmode, 0, sizeof(devmode));
        devmode.dmSize       = sizeof(devmode);
        devmode.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
        devmode.dmBitsPerPel = format_desc->byte_count * 8;
        devmode.dmPelsWidth  = object->orig_width;
        devmode.dmPelsHeight = object->orig_height;
        ChangeDisplaySettingsExW(This->adapter->DeviceName, &devmode, NULL, CDS_FULLSCREEN, NULL);
    }

    if (object->backBuffer) {
        UINT i;
        for(i = 0; i < object->presentParms.BackBufferCount; i++) {
            if(object->backBuffer[i]) {
                IWineD3DSurface_GetParent(object->backBuffer[i], &bufferParent);
                IUnknown_Release(bufferParent); /* once for the get parent */
                if (IUnknown_Release(bufferParent) > 0) {
                    FIXME("(%p) Something's still holding the back buffer\n",This);
                }
            }
        }
        HeapFree(GetProcessHeap(), 0, object->backBuffer);
        object->backBuffer = NULL;
    }
    if(object->context && object->context[0])
        DestroyContext(This, object->context[0]);
    if(object->frontBuffer) {
        IWineD3DSurface_GetParent(object->frontBuffer, &bufferParent);
        IUnknown_Release(bufferParent); /* once for the get parent */
        if (IUnknown_Release(bufferParent) > 0) {
            FIXME("(%p) Something's still holding the front buffer\n",This);
        }
    }
    HeapFree(GetProcessHeap(), 0, object);
    return hr;
}

/** NOTE: These are ahead of the other getters and setters to save using a forward declaration **/
static UINT     WINAPI  IWineD3DDeviceImpl_GetNumberOfSwapChains(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p)\n", This);

    return This->NumberOfSwapChains;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_GetSwapChain(IWineD3DDevice *iface, UINT iSwapChain, IWineD3DSwapChain **pSwapChain) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : swapchain %d\n", This, iSwapChain);

    if(iSwapChain < This->NumberOfSwapChains) {
        *pSwapChain = This->swapchains[iSwapChain];
        IWineD3DSwapChain_AddRef(*pSwapChain);
        TRACE("(%p) returning %p\n", This, *pSwapChain);
        return WINED3D_OK;
    } else {
        TRACE("Swapchain out of range\n");
        *pSwapChain = NULL;
        return WINED3DERR_INVALIDCALL;
    }
}

/*****
 * Vertex Declaration
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_CreateVertexDeclaration(IWineD3DDevice* iface, IWineD3DVertexDeclaration** ppVertexDeclaration,
        IUnknown *parent, const WINED3DVERTEXELEMENT *elements, UINT element_count) {
    IWineD3DDeviceImpl            *This   = (IWineD3DDeviceImpl *)iface;
    IWineD3DVertexDeclarationImpl *object = NULL;
    HRESULT hr = WINED3D_OK;

    TRACE("(%p) : directXVersion %u, elements %p, element_count %d, ppDecl=%p\n",
            This, ((IWineD3DImpl *)This->wineD3D)->dxVersion, elements, element_count, ppVertexDeclaration);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if(!object)
    {
        ERR("Out of memory\n");
        *ppVertexDeclaration = NULL;
        return WINED3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &IWineD3DVertexDeclaration_Vtbl;
    object->wineD3DDevice = This;
    object->parent = parent;
    object->ref = 1;

    *ppVertexDeclaration = (IWineD3DVertexDeclaration *)object;

    hr = vertexdeclaration_init(object, elements, element_count);

    if(FAILED(hr)) {
        IWineD3DVertexDeclaration_Release((IWineD3DVertexDeclaration *)object);
        *ppVertexDeclaration = NULL;
    }

    return hr;
}

static unsigned int ConvertFvfToDeclaration(IWineD3DDeviceImpl *This, /* For the GL info, which has the type table */
                                            DWORD fvf, WINED3DVERTEXELEMENT** ppVertexElements) {

    unsigned int idx, idx2;
    unsigned int offset;
    BOOL has_pos = (fvf & WINED3DFVF_POSITION_MASK) != 0;
    BOOL has_blend = (fvf & WINED3DFVF_XYZB5) > WINED3DFVF_XYZRHW;
    BOOL has_blend_idx = has_blend &&
       (((fvf & WINED3DFVF_XYZB5) == WINED3DFVF_XYZB5) ||
        (fvf & WINED3DFVF_LASTBETA_D3DCOLOR) ||
        (fvf & WINED3DFVF_LASTBETA_UBYTE4));
    BOOL has_normal = (fvf & WINED3DFVF_NORMAL) != 0;
    BOOL has_psize = (fvf & WINED3DFVF_PSIZE) != 0;
    BOOL has_diffuse = (fvf & WINED3DFVF_DIFFUSE) != 0;
    BOOL has_specular = (fvf & WINED3DFVF_SPECULAR) !=0;

    DWORD num_textures = (fvf & WINED3DFVF_TEXCOUNT_MASK) >> WINED3DFVF_TEXCOUNT_SHIFT;
    DWORD texcoords = (fvf & 0xFFFF0000) >> 16;
    WINED3DVERTEXELEMENT *elements = NULL;

    unsigned int size;
    DWORD num_blends = 1 + (((fvf & WINED3DFVF_XYZB5) - WINED3DFVF_XYZB1) >> 1);
    if (has_blend_idx) num_blends--;

    /* Compute declaration size */
    size = has_pos + (has_blend && num_blends > 0) + has_blend_idx + has_normal +
           has_psize + has_diffuse + has_specular + num_textures;

    /* convert the declaration */
    elements = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WINED3DVERTEXELEMENT));
    if (!elements) return ~0U;

    idx = 0;
    if (has_pos) {
        if (!has_blend && (fvf & WINED3DFVF_XYZRHW)) {
            elements[idx].format = WINED3DFMT_R32G32B32A32_FLOAT;
            elements[idx].usage = WINED3DDECLUSAGE_POSITIONT;
        }
        else if ((fvf & WINED3DFVF_XYZW) == WINED3DFVF_XYZW) {
            elements[idx].format = WINED3DFMT_R32G32B32A32_FLOAT;
            elements[idx].usage = WINED3DDECLUSAGE_POSITION;
        }
        else {
            elements[idx].format = WINED3DFMT_R32G32B32_FLOAT;
            elements[idx].usage = WINED3DDECLUSAGE_POSITION;
        }
        elements[idx].usage_idx = 0;
        idx++;
    }
    if (has_blend && (num_blends > 0)) {
        if (((fvf & WINED3DFVF_XYZB5) == WINED3DFVF_XYZB2) && (fvf & WINED3DFVF_LASTBETA_D3DCOLOR))
            elements[idx].format = WINED3DFMT_A8R8G8B8;
        else {
            switch(num_blends) {
                case 1: elements[idx].format = WINED3DFMT_R32_FLOAT; break;
                case 2: elements[idx].format = WINED3DFMT_R32G32_FLOAT; break;
                case 3: elements[idx].format = WINED3DFMT_R32G32B32_FLOAT; break;
                case 4: elements[idx].format = WINED3DFMT_R32G32B32A32_FLOAT; break;
                default:
                    ERR("Unexpected amount of blend values: %u\n", num_blends);
            }
        }
        elements[idx].usage = WINED3DDECLUSAGE_BLENDWEIGHT;
        elements[idx].usage_idx = 0;
        idx++;
    }
    if (has_blend_idx) {
        if (fvf & WINED3DFVF_LASTBETA_UBYTE4 ||
            (((fvf & WINED3DFVF_XYZB5) == WINED3DFVF_XYZB2) && (fvf & WINED3DFVF_LASTBETA_D3DCOLOR)))
            elements[idx].format = WINED3DFMT_R8G8B8A8_UINT;
        else if (fvf & WINED3DFVF_LASTBETA_D3DCOLOR)
            elements[idx].format = WINED3DFMT_A8R8G8B8;
        else
            elements[idx].format = WINED3DFMT_R32_FLOAT;
        elements[idx].usage = WINED3DDECLUSAGE_BLENDINDICES;
        elements[idx].usage_idx = 0;
        idx++;
    }
    if (has_normal) {
        elements[idx].format = WINED3DFMT_R32G32B32_FLOAT;
        elements[idx].usage = WINED3DDECLUSAGE_NORMAL;
        elements[idx].usage_idx = 0;
        idx++;
    }
    if (has_psize) {
        elements[idx].format = WINED3DFMT_R32_FLOAT;
        elements[idx].usage = WINED3DDECLUSAGE_PSIZE;
        elements[idx].usage_idx = 0;
        idx++;
    }
    if (has_diffuse) {
        elements[idx].format = WINED3DFMT_A8R8G8B8;
        elements[idx].usage = WINED3DDECLUSAGE_COLOR;
        elements[idx].usage_idx = 0;
        idx++;
    }
    if (has_specular) {
        elements[idx].format = WINED3DFMT_A8R8G8B8;
        elements[idx].usage = WINED3DDECLUSAGE_COLOR;
        elements[idx].usage_idx = 1;
        idx++;
    }
    for (idx2 = 0; idx2 < num_textures; idx2++) {
        unsigned int numcoords = (texcoords >> (idx2*2)) & 0x03;
        switch (numcoords) {
            case WINED3DFVF_TEXTUREFORMAT1:
                elements[idx].format = WINED3DFMT_R32_FLOAT;
                break;
            case WINED3DFVF_TEXTUREFORMAT2:
                elements[idx].format = WINED3DFMT_R32G32_FLOAT;
                break;
            case WINED3DFVF_TEXTUREFORMAT3:
                elements[idx].format = WINED3DFMT_R32G32B32_FLOAT;
                break;
            case WINED3DFVF_TEXTUREFORMAT4:
                elements[idx].format = WINED3DFMT_R32G32B32A32_FLOAT;
                break;
        }
        elements[idx].usage = WINED3DDECLUSAGE_TEXCOORD;
        elements[idx].usage_idx = idx2;
        idx++;
    }

    /* Now compute offsets, and initialize the rest of the fields */
    for (idx = 0, offset = 0; idx < size; ++idx)
    {
        const struct GlPixelFormatDesc *format_desc = getFormatDescEntry(elements[idx].format, &This->adapter->gl_info);
        elements[idx].input_slot = 0;
        elements[idx].method = WINED3DDECLMETHOD_DEFAULT;
        elements[idx].offset = offset;
        offset += format_desc->component_count * format_desc->component_size;
    }

    *ppVertexElements = elements;
    return size;
}

static HRESULT WINAPI IWineD3DDeviceImpl_CreateVertexDeclarationFromFVF(IWineD3DDevice* iface, IWineD3DVertexDeclaration** ppVertexDeclaration, IUnknown *Parent, DWORD Fvf) {
    WINED3DVERTEXELEMENT* elements = NULL;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    unsigned int size;
    DWORD hr;

    size = ConvertFvfToDeclaration(This, Fvf, &elements);
    if (size == ~0U) return WINED3DERR_OUTOFVIDEOMEMORY;

    hr = IWineD3DDevice_CreateVertexDeclaration(iface, ppVertexDeclaration, Parent, elements, size);
    HeapFree(GetProcessHeap(), 0, elements);
    if (hr != S_OK) return hr;

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_CreateVertexShader(IWineD3DDevice *iface, IWineD3DVertexDeclaration *vertex_declaration, CONST DWORD *pFunction, IWineD3DVertexShader **ppVertexShader, IUnknown *parent) {
    IWineD3DDeviceImpl       *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DVertexShaderImpl *object;  /* NOTE: impl usage is ok, this is a create */
    HRESULT hr = WINED3D_OK;

    if (!pFunction) return WINED3DERR_INVALIDCALL;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Out of memory\n");
        *ppVertexShader = NULL;
        return WINED3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &IWineD3DVertexShader_Vtbl;
    object->parent = parent;
    shader_init(&object->baseShader, iface, IWineD3DVertexShaderImpl_shader_ins);
    list_add_head(&This->shaders, &object->baseShader.shader_list_entry);
    *ppVertexShader = (IWineD3DVertexShader *)object;

    TRACE("(%p) : Created vertex shader %p\n", This, *ppVertexShader);

    if (vertex_declaration) {
        IWineD3DVertexShader_FakeSemantics(*ppVertexShader, vertex_declaration);
    }

    hr = IWineD3DVertexShader_SetFunction(*ppVertexShader, pFunction);
    if (FAILED(hr))
    {
        WARN("(%p) : Failed to set function, returning %#x\n", iface, hr);
        IWineD3DVertexShader_Release(*ppVertexShader);
        *ppVertexShader = NULL;
        return hr;
    }

    return hr;
}

static HRESULT WINAPI IWineD3DDeviceImpl_CreatePixelShader(IWineD3DDevice *iface, CONST DWORD *pFunction, IWineD3DPixelShader **ppPixelShader, IUnknown *parent) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DPixelShaderImpl *object; /* NOTE: impl allowed, this is a create */
    HRESULT hr = WINED3D_OK;

    if (!pFunction) return WINED3DERR_INVALIDCALL;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Out of memory\n");
        *ppPixelShader = NULL;
        return WINED3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &IWineD3DPixelShader_Vtbl;
    object->parent = parent;
    shader_init(&object->baseShader, iface, IWineD3DPixelShaderImpl_shader_ins);
    list_add_head(&This->shaders, &object->baseShader.shader_list_entry);
    *ppPixelShader = (IWineD3DPixelShader *)object;

    TRACE("(%p) : Created pixel shader %p\n", This, *ppPixelShader);

    hr = IWineD3DPixelShader_SetFunction(*ppPixelShader, pFunction);
    if (FAILED(hr))
    {
        WARN("(%p) : Failed to set function, returning %#x\n", iface, hr);
        IWineD3DPixelShader_Release(*ppPixelShader);
        *ppPixelShader = NULL;
        return hr;
    }

    return hr;
}

static HRESULT WINAPI IWineD3DDeviceImpl_CreatePalette(IWineD3DDevice *iface, DWORD Flags,
        const PALETTEENTRY *PalEnt, IWineD3DPalette **Palette, IUnknown *Parent)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    IWineD3DPaletteImpl *object;
    HRESULT hr;
    TRACE("(%p)->(%x, %p, %p, %p)\n", This, Flags, PalEnt, Palette, Parent);

    /* Create the new object */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IWineD3DPaletteImpl));
    if(!object) {
        ERR("Out of memory when allocating memory for a IWineD3DPalette implementation\n");
        return E_OUTOFMEMORY;
    }

    object->lpVtbl = &IWineD3DPalette_Vtbl;
    object->ref = 1;
    object->Flags = Flags;
    object->parent = Parent;
    object->wineD3DDevice = This;
    object->palNumEntries = IWineD3DPaletteImpl_Size(Flags);
    object->hpal = CreatePalette((const LOGPALETTE*)&(object->palVersion));

    if(!object->hpal) {
        HeapFree( GetProcessHeap(), 0, object);
        return E_OUTOFMEMORY;
    }

    hr = IWineD3DPalette_SetEntries((IWineD3DPalette *) object, 0, 0, IWineD3DPaletteImpl_Size(Flags), PalEnt);
    if(FAILED(hr)) {
        IWineD3DPalette_Release((IWineD3DPalette *) object);
        return hr;
    }

    *Palette = (IWineD3DPalette *) object;

    return WINED3D_OK;
}

static void IWineD3DDeviceImpl_LoadLogo(IWineD3DDeviceImpl *This, const char *filename) {
    HBITMAP hbm;
    BITMAP bm;
    HRESULT hr;
    HDC dcb = NULL, dcs = NULL;
    WINEDDCOLORKEY colorkey;

    hbm = LoadImageA(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if(hbm)
    {
        GetObjectA(hbm, sizeof(BITMAP), &bm);
        dcb = CreateCompatibleDC(NULL);
        if(!dcb) goto out;
        SelectObject(dcb, hbm);
    }
    else
    {
        /* Create a 32x32 white surface to indicate that wined3d is used, but the specified image
         * couldn't be loaded
         */
        memset(&bm, 0, sizeof(bm));
        bm.bmWidth = 32;
        bm.bmHeight = 32;
    }

    hr = IWineD3DDevice_CreateSurface((IWineD3DDevice *) This, bm.bmWidth, bm.bmHeight, WINED3DFMT_R5G6B5,
                                      TRUE, FALSE, 0, &This->logo_surface, WINED3DRTYPE_SURFACE, 0,
                                      WINED3DPOOL_DEFAULT, WINED3DMULTISAMPLE_NONE, 0, NULL, SURFACE_OPENGL, NULL);
    if(FAILED(hr)) {
        ERR("Wine logo requested, but failed to create surface\n");
        goto out;
    }

    if(dcb) {
        hr = IWineD3DSurface_GetDC(This->logo_surface, &dcs);
        if(FAILED(hr)) goto out;
        BitBlt(dcs, 0, 0, bm.bmWidth, bm.bmHeight, dcb, 0, 0, SRCCOPY);
        IWineD3DSurface_ReleaseDC(This->logo_surface, dcs);

        colorkey.dwColorSpaceLowValue = 0;
        colorkey.dwColorSpaceHighValue = 0;
        IWineD3DSurface_SetColorKey(This->logo_surface, WINEDDCKEY_SRCBLT, &colorkey);
    } else {
        /* Fill the surface with a white color to show that wined3d is there */
        IWineD3DDevice_ColorFill((IWineD3DDevice *) This, This->logo_surface, NULL, 0xffffffff);
    }

    out:
    if(dcb) {
        DeleteDC(dcb);
    }
    if(hbm) {
        DeleteObject(hbm);
    }
    return;
}

static void create_dummy_textures(IWineD3DDeviceImpl *This) {
    unsigned int i;
    /* Under DirectX you can have texture stage operations even if no texture is
    bound, whereas opengl will only do texture operations when a valid texture is
    bound. We emulate this by creating dummy textures and binding them to each
    texture stage, but disable all stages by default. Hence if a stage is enabled
    then the default texture will kick in until replaced by a SetTexture call     */
    ENTER_GL();

    if(GL_SUPPORT(APPLE_CLIENT_STORAGE)) {
        /* The dummy texture does not have client storage backing */
        glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
        checkGLcall("glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE)");
    }
    for (i = 0; i < GL_LIMITS(textures); i++) {
        GLubyte white = 255;

        /* Make appropriate texture active */
        GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + i));
        checkGLcall("glActiveTextureARB");

        /* Generate an opengl texture name */
        glGenTextures(1, &This->dummyTextureName[i]);
        checkGLcall("glGenTextures");
        TRACE("Dummy Texture %d given name %d\n", i, This->dummyTextureName[i]);

        /* Generate a dummy 2d texture (not using 1d because they cause many
        * DRI drivers fall back to sw) */
        glBindTexture(GL_TEXTURE_2D, This->dummyTextureName[i]);
        checkGLcall("glBindTexture");

        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 1, 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, &white);
        checkGLcall("glTexImage2D");
    }
    if(GL_SUPPORT(APPLE_CLIENT_STORAGE)) {
        /* Reenable because if supported it is enabled by default */
        glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
        checkGLcall("glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE)");
    }

    LEAVE_GL();
}

static HRESULT WINAPI IWineD3DDeviceImpl_Init3D(IWineD3DDevice *iface,
        WINED3DPRESENT_PARAMETERS *pPresentationParameters)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    IWineD3DSwapChainImpl *swapchain = NULL;
    HRESULT hr;
    DWORD state;
    unsigned int i;

    TRACE("(%p)->(%p)\n", This, pPresentationParameters);

    if(This->d3d_initialized) return WINED3DERR_INVALIDCALL;
    if(!This->adapter->opengl) return WINED3DERR_INVALIDCALL;

    /* TODO: Test if OpenGL is compiled in and loaded */

    TRACE("(%p) : Creating stateblock\n", This);
    /* Creating the startup stateBlock - Note Special Case: 0 => Don't fill in yet! */
    hr = IWineD3DDevice_CreateStateBlock(iface,
                                         WINED3DSBT_INIT,
                                         (IWineD3DStateBlock **)&This->stateBlock,
                                         NULL);
    if (WINED3D_OK != hr) {   /* Note: No parent needed for initial internal stateblock */
        WARN("Failed to create stateblock\n");
        goto err_out;
    }
    TRACE("(%p) : Created stateblock (%p)\n", This, This->stateBlock);
    This->updateStateBlock = This->stateBlock;
    IWineD3DStateBlock_AddRef((IWineD3DStateBlock*)This->updateStateBlock);

    This->render_targets = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IWineD3DSurface *) * GL_LIMITS(buffers));
    This->draw_buffers = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GLenum) * GL_LIMITS(buffers));

    This->NumberOfPalettes = 1;
    This->palettes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PALETTEENTRY*));
    if(!This->palettes || !This->render_targets || !This->draw_buffers) {
        ERR("Out of memory!\n");
        goto err_out;
    }
    This->palettes[0] = HeapAlloc(GetProcessHeap(), 0, sizeof(PALETTEENTRY) * 256);
    if(!This->palettes[0]) {
        ERR("Out of memory!\n");
        goto err_out;
    }
    for (i = 0; i < 256; ++i) {
        This->palettes[0][i].peRed   = 0xFF;
        This->palettes[0][i].peGreen = 0xFF;
        This->palettes[0][i].peBlue  = 0xFF;
        This->palettes[0][i].peFlags = 0xFF;
    }
    This->currentPalette = 0;

    /* Initialize the texture unit mapping to a 1:1 mapping */
    for (state = 0; state < MAX_COMBINED_SAMPLERS; ++state) {
        if (state < GL_LIMITS(fragment_samplers)) {
            This->texUnitMap[state] = state;
            This->rev_tex_unit_map[state] = state;
        } else {
            This->texUnitMap[state] = -1;
            This->rev_tex_unit_map[state] = -1;
        }
    }

    /* Setup the implicit swapchain */
    TRACE("Creating implicit swapchain\n");
    hr = IWineD3DDeviceParent_CreateSwapChain(This->device_parent,
            pPresentationParameters, (IWineD3DSwapChain **)&swapchain);
    if (FAILED(hr))
    {
        WARN("Failed to create implicit swapchain\n");
        goto err_out;
    }

    This->NumberOfSwapChains = 1;
    This->swapchains = HeapAlloc(GetProcessHeap(), 0, This->NumberOfSwapChains * sizeof(IWineD3DSwapChain *));
    if(!This->swapchains) {
        ERR("Out of memory!\n");
        goto err_out;
    }
    This->swapchains[0] = (IWineD3DSwapChain *) swapchain;

    if(swapchain->backBuffer && swapchain->backBuffer[0]) {
        TRACE("Setting rendertarget to %p\n", swapchain->backBuffer);
        This->render_targets[0] = swapchain->backBuffer[0];
        This->lastActiveRenderTarget = swapchain->backBuffer[0];
    }
    else {
        TRACE("Setting rendertarget to %p\n", swapchain->frontBuffer);
        This->render_targets[0] = swapchain->frontBuffer;
        This->lastActiveRenderTarget = swapchain->frontBuffer;
    }
    IWineD3DSurface_AddRef(This->render_targets[0]);
    This->activeContext = swapchain->context[0];
    This->lastThread = GetCurrentThreadId();

    /* Depth Stencil support */
    This->stencilBufferTarget = This->auto_depth_stencil_buffer;
    if (NULL != This->stencilBufferTarget) {
        IWineD3DSurface_AddRef(This->stencilBufferTarget);
    }

    hr = This->shader_backend->shader_alloc_private(iface);
    if(FAILED(hr)) {
        TRACE("Shader private data couldn't be allocated\n");
        goto err_out;
    }
    hr = This->frag_pipe->alloc_private(iface);
    if(FAILED(hr)) {
        TRACE("Fragment pipeline private data couldn't be allocated\n");
        goto err_out;
    }
    hr = This->blitter->alloc_private(iface);
    if(FAILED(hr)) {
        TRACE("Blitter private data couldn't be allocated\n");
        goto err_out;
    }

    /* Set up some starting GL setup */

    /* Setup all the devices defaults */
    IWineD3DStateBlock_InitStartupStateBlock((IWineD3DStateBlock *)This->stateBlock);
    create_dummy_textures(This);

    ENTER_GL();

    /* Initialize the current view state */
    This->view_ident = 1;
    This->contexts[0]->last_was_rhw = 0;
    glGetIntegerv(GL_MAX_LIGHTS, &This->maxConcurrentLights);
    checkGLcall("glGetIntegerv(GL_MAX_LIGHTS, &This->maxConcurrentLights)");

    switch(wined3d_settings.offscreen_rendering_mode) {
        case ORM_FBO:
        case ORM_PBUFFER:
            This->offscreenBuffer = GL_BACK;
            break;

        case ORM_BACKBUFFER:
        {
            if(This->activeContext->aux_buffers > 0) {
                TRACE("Using auxilliary buffer for offscreen rendering\n");
                This->offscreenBuffer = GL_AUX0;
            } else {
                TRACE("Using back buffer for offscreen rendering\n");
                This->offscreenBuffer = GL_BACK;
            }
        }
    }

    TRACE("(%p) All defaults now set up, leaving Init3D with %p\n", This, This);
    LEAVE_GL();

    /* Clear the screen */
    IWineD3DDevice_Clear((IWineD3DDevice *) This, 0, NULL,
                          WINED3DCLEAR_TARGET | pPresentationParameters->EnableAutoDepthStencil ? WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL : 0,
                          0x00, 1.0, 0);

    This->d3d_initialized = TRUE;

    if(wined3d_settings.logo) {
        IWineD3DDeviceImpl_LoadLogo(This, wined3d_settings.logo);
    }
    This->highest_dirty_ps_const = 0;
    This->highest_dirty_vs_const = 0;
    return WINED3D_OK;

err_out:
    HeapFree(GetProcessHeap(), 0, This->render_targets);
    HeapFree(GetProcessHeap(), 0, This->draw_buffers);
    HeapFree(GetProcessHeap(), 0, This->swapchains);
    This->NumberOfSwapChains = 0;
    if(This->palettes) {
        HeapFree(GetProcessHeap(), 0, This->palettes[0]);
        HeapFree(GetProcessHeap(), 0, This->palettes);
    }
    This->NumberOfPalettes = 0;
    if(swapchain) {
        IWineD3DSwapChain_Release( (IWineD3DSwapChain *) swapchain);
    }
    if(This->stateBlock) {
        IWineD3DStateBlock_Release((IWineD3DStateBlock *) This->stateBlock);
        This->stateBlock = NULL;
    }
    if (This->blit_priv) {
        This->blitter->free_private(iface);
    }
    if (This->fragment_priv) {
        This->frag_pipe->free_private(iface);
    }
    if (This->shader_priv) {
        This->shader_backend->shader_free_private(iface);
    }
    return hr;
}

static HRESULT WINAPI IWineD3DDeviceImpl_InitGDI(IWineD3DDevice *iface,
        WINED3DPRESENT_PARAMETERS *pPresentationParameters)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    IWineD3DSwapChainImpl *swapchain = NULL;
    HRESULT hr;

    /* Setup the implicit swapchain */
    TRACE("Creating implicit swapchain\n");
    hr = IWineD3DDeviceParent_CreateSwapChain(This->device_parent,
            pPresentationParameters, (IWineD3DSwapChain **)&swapchain);
    if (FAILED(hr))
    {
        WARN("Failed to create implicit swapchain\n");
        goto err_out;
    }

    This->NumberOfSwapChains = 1;
    This->swapchains = HeapAlloc(GetProcessHeap(), 0, This->NumberOfSwapChains * sizeof(IWineD3DSwapChain *));
    if(!This->swapchains) {
        ERR("Out of memory!\n");
        goto err_out;
    }
    This->swapchains[0] = (IWineD3DSwapChain *) swapchain;
    return WINED3D_OK;

err_out:
    IWineD3DSwapChain_Release((IWineD3DSwapChain *) swapchain);
    return hr;
}

static HRESULT WINAPI device_unload_resource(IWineD3DResource *resource, void *ctx)
{
    IWineD3DResource_UnLoad(resource);
    IWineD3DResource_Release(resource);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_Uninit3D(IWineD3DDevice *iface, D3DCB_DESTROYSURFACEFN D3DCB_DestroyDepthStencilSurface, D3DCB_DESTROYSWAPCHAINFN D3DCB_DestroySwapChain) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    int sampler;
    UINT i;
    TRACE("(%p)\n", This);

    if(!This->d3d_initialized) return WINED3DERR_INVALIDCALL;

    /* I don't think that the interface guarantees that the device is destroyed from the same thread
     * it was created. Thus make sure a context is active for the glDelete* calls
     */
    ActivateContext(This, This->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);

    if(This->logo_surface) IWineD3DSurface_Release(This->logo_surface);

    /* Unload resources */
    IWineD3DDevice_EnumResources(iface, device_unload_resource, NULL);

    TRACE("Deleting high order patches\n");
    for(i = 0; i < PATCHMAP_SIZE; i++) {
        struct list *e1, *e2;
        struct WineD3DRectPatch *patch;
        LIST_FOR_EACH_SAFE(e1, e2, &This->patches[i]) {
            patch = LIST_ENTRY(e1, struct WineD3DRectPatch, entry);
            IWineD3DDevice_DeletePatch(iface, patch->Handle);
        }
    }

    /* Delete the palette conversion shader if it is around */
    if(This->paletteConversionShader) {
        ENTER_GL();
        GL_EXTCALL(glDeleteProgramsARB(1, &This->paletteConversionShader));
        LEAVE_GL();
        This->paletteConversionShader = 0;
    }

    /* Delete the pbuffer context if there is any */
    if(This->pbufferContext) DestroyContext(This, This->pbufferContext);

    /* Delete the mouse cursor texture */
    if(This->cursorTexture) {
        ENTER_GL();
        glDeleteTextures(1, &This->cursorTexture);
        LEAVE_GL();
        This->cursorTexture = 0;
    }

    for (sampler = 0; sampler < MAX_FRAGMENT_SAMPLERS; ++sampler) {
        IWineD3DDevice_SetTexture(iface, sampler, NULL);
    }
    for (sampler = 0; sampler < MAX_VERTEX_SAMPLERS; ++sampler) {
        IWineD3DDevice_SetTexture(iface, WINED3DVERTEXTEXTURESAMPLER0 + sampler, NULL);
    }

    /* Destroy the depth blt resources, they will be invalid after the reset. Also free shader
     * private data, it might contain opengl pointers
     */
    if(This->depth_blt_texture) {
        glDeleteTextures(1, &This->depth_blt_texture);
        This->depth_blt_texture = 0;
    }
    if (This->depth_blt_rb) {
        GL_EXTCALL(glDeleteRenderbuffersEXT(1, &This->depth_blt_rb));
        This->depth_blt_rb = 0;
        This->depth_blt_rb_w = 0;
        This->depth_blt_rb_h = 0;
    }

    /* Release the update stateblock */
    if(IWineD3DStateBlock_Release((IWineD3DStateBlock *)This->updateStateBlock) > 0){
        if(This->updateStateBlock != This->stateBlock)
            FIXME("(%p) Something's still holding the Update stateblock\n",This);
    }
    This->updateStateBlock = NULL;

    { /* because were not doing proper internal refcounts releasing the primary state block
        causes recursion with the extra checks in ResourceReleased, to avoid this we have
        to set this->stateBlock = NULL; first */
        IWineD3DStateBlock *stateBlock = (IWineD3DStateBlock *)This->stateBlock;
        This->stateBlock = NULL;

        /* Release the stateblock */
        if(IWineD3DStateBlock_Release(stateBlock) > 0){
            FIXME("(%p) Something's still holding the Update stateblock\n",This);
        }
    }

    /* Destroy the shader backend. Note that this has to happen after all shaders are destroyed. */
    This->blitter->free_private(iface);
    This->frag_pipe->free_private(iface);
    This->shader_backend->shader_free_private(iface);

    /* Release the buffers (with sanity checks)*/
    TRACE("Releasing the depth stencil buffer at %p\n", This->stencilBufferTarget);
    if(This->stencilBufferTarget != NULL && (IWineD3DSurface_Release(This->stencilBufferTarget) >0)){
        if(This->auto_depth_stencil_buffer != This->stencilBufferTarget)
            FIXME("(%p) Something's still holding the stencilBufferTarget\n",This);
    }
    This->stencilBufferTarget = NULL;

    TRACE("Releasing the render target at %p\n", This->render_targets[0]);
    if(IWineD3DSurface_Release(This->render_targets[0]) >0){
          /* This check is a bit silly, it should be in swapchain_release FIXME("(%p) Something's still holding the renderTarget\n",This); */
    }
    TRACE("Setting rendertarget to NULL\n");
    This->render_targets[0] = NULL;

    if (This->auto_depth_stencil_buffer) {
        if(D3DCB_DestroyDepthStencilSurface(This->auto_depth_stencil_buffer) > 0) {
            FIXME("(%p) Something's still holding the auto depth stencil buffer\n", This);
        }
        This->auto_depth_stencil_buffer = NULL;
    }

    for(i=0; i < This->NumberOfSwapChains; i++) {
        TRACE("Releasing the implicit swapchain %d\n", i);
        if (D3DCB_DestroySwapChain(This->swapchains[i])  > 0) {
            FIXME("(%p) Something's still holding the implicit swapchain\n", This);
        }
    }

    HeapFree(GetProcessHeap(), 0, This->swapchains);
    This->swapchains = NULL;
    This->NumberOfSwapChains = 0;

    for (i = 0; i < This->NumberOfPalettes; i++) HeapFree(GetProcessHeap(), 0, This->palettes[i]);
    HeapFree(GetProcessHeap(), 0, This->palettes);
    This->palettes = NULL;
    This->NumberOfPalettes = 0;

    HeapFree(GetProcessHeap(), 0, This->render_targets);
    HeapFree(GetProcessHeap(), 0, This->draw_buffers);
    This->render_targets = NULL;
    This->draw_buffers = NULL;

    This->d3d_initialized = FALSE;
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_UninitGDI(IWineD3DDevice *iface, D3DCB_DESTROYSWAPCHAINFN D3DCB_DestroySwapChain) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    unsigned int i;

    for(i=0; i < This->NumberOfSwapChains; i++) {
        TRACE("Releasing the implicit swapchain %d\n", i);
        if (D3DCB_DestroySwapChain(This->swapchains[i])  > 0) {
            FIXME("(%p) Something's still holding the implicit swapchain\n", This);
        }
    }

    HeapFree(GetProcessHeap(), 0, This->swapchains);
    This->swapchains = NULL;
    This->NumberOfSwapChains = 0;
    return WINED3D_OK;
}

/* Enables thread safety in the wined3d device and its resources. Called by DirectDraw
 * from SetCooperativeLevel if DDSCL_MULTITHREADED is specified, and by d3d8/9 from
 * CreateDevice if D3DCREATE_MULTITHREADED is passed.
 *
 * There is no way to deactivate thread safety once it is enabled.
 */
static void WINAPI IWineD3DDeviceImpl_SetMultithreaded(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;

    /*For now just store the flag(needed in case of ddraw) */
    This->createParms.BehaviorFlags |= WINED3DCREATE_MULTITHREADED;

    return;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetDisplayMode(IWineD3DDevice *iface, UINT iSwapChain,
        const WINED3DDISPLAYMODE* pMode) {
    DEVMODEW devmode;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    LONG ret;
    const struct GlPixelFormatDesc *format_desc = getFormatDescEntry(pMode->Format, &GLINFO_LOCATION);
    RECT clip_rc;

    TRACE("(%p)->(%d,%p) Mode=%dx%dx@%d, %s\n", This, iSwapChain, pMode, pMode->Width, pMode->Height, pMode->RefreshRate, debug_d3dformat(pMode->Format));

    /* Resize the screen even without a window:
     * The app could have unset it with SetCooperativeLevel, but not called
     * RestoreDisplayMode first. Then the release will call RestoreDisplayMode,
     * but we don't have any hwnd
     */

    memset(&devmode, 0, sizeof(devmode));
    devmode.dmSize = sizeof(devmode);
    devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
    devmode.dmBitsPerPel = format_desc->byte_count * 8;
    devmode.dmPelsWidth  = pMode->Width;
    devmode.dmPelsHeight = pMode->Height;

    devmode.dmDisplayFrequency = pMode->RefreshRate;
    if (pMode->RefreshRate != 0)  {
        devmode.dmFields |= DM_DISPLAYFREQUENCY;
    }

    /* Only change the mode if necessary */
    if( (This->ddraw_width == pMode->Width) &&
        (This->ddraw_height == pMode->Height) &&
        (This->ddraw_format == pMode->Format) &&
        (pMode->RefreshRate == 0) ) {
        return WINED3D_OK;
    }

    ret = ChangeDisplaySettingsExW(NULL, &devmode, NULL, CDS_FULLSCREEN, NULL);
    if (ret != DISP_CHANGE_SUCCESSFUL) {
        if(devmode.dmDisplayFrequency != 0) {
            WARN("ChangeDisplaySettingsExW failed, trying without the refresh rate\n");
            devmode.dmFields &= ~DM_DISPLAYFREQUENCY;
            devmode.dmDisplayFrequency = 0;
            ret = ChangeDisplaySettingsExW(NULL, &devmode, NULL, CDS_FULLSCREEN, NULL) != DISP_CHANGE_SUCCESSFUL;
        }
        if(ret != DISP_CHANGE_SUCCESSFUL) {
            return WINED3DERR_NOTAVAILABLE;
        }
    }

    /* Store the new values */
    This->ddraw_width = pMode->Width;
    This->ddraw_height = pMode->Height;
    This->ddraw_format = pMode->Format;

    /* And finally clip mouse to our screen */
    SetRect(&clip_rc, 0, 0, pMode->Width, pMode->Height);
    ClipCursor(&clip_rc);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetDirect3D(IWineD3DDevice *iface, IWineD3D **ppD3D) {
   IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
   *ppD3D= This->wineD3D;
   TRACE("(%p) : wineD3D returning %p\n", This,  *ppD3D);
   IWineD3D_AddRef(*ppD3D);
   return WINED3D_OK;
}

static UINT WINAPI IWineD3DDeviceImpl_GetAvailableTextureMem(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : simulating %dMB, returning %dMB left\n",  This,
         (This->adapter->TextureRam/(1024*1024)),
         ((This->adapter->TextureRam - This->adapter->UsedTextureRam) / (1024*1024)));
    /* return simulated texture memory left */
    return (This->adapter->TextureRam - This->adapter->UsedTextureRam);
}

/*****
 * Get / Set Stream Source
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_SetStreamSource(IWineD3DDevice *iface, UINT StreamNumber,
        IWineD3DBuffer *pStreamData, UINT OffsetInBytes, UINT Stride)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DBuffer *oldSrc;

    if (StreamNumber >= MAX_STREAMS) {
        WARN("Stream out of range %d\n", StreamNumber);
        return WINED3DERR_INVALIDCALL;
    } else if(OffsetInBytes & 0x3) {
        WARN("OffsetInBytes is not 4 byte aligned: %d\n", OffsetInBytes);
        return WINED3DERR_INVALIDCALL;
    }

    oldSrc = This->updateStateBlock->streamSource[StreamNumber];
    TRACE("(%p) : StreamNo: %u, OldStream (%p), NewStream (%p), OffsetInBytes %u, NewStride %u\n", This, StreamNumber, oldSrc, pStreamData, OffsetInBytes, Stride);

    This->updateStateBlock->changed.streamSource |= 1 << StreamNumber;

    if(oldSrc == pStreamData &&
       This->updateStateBlock->streamStride[StreamNumber] == Stride &&
       This->updateStateBlock->streamOffset[StreamNumber] == OffsetInBytes) {
       TRACE("Application is setting the old values over, nothing to do\n");
       return WINED3D_OK;
    }

    This->updateStateBlock->streamSource[StreamNumber]         = pStreamData;
    if (pStreamData) {
        This->updateStateBlock->streamStride[StreamNumber]     = Stride;
        This->updateStateBlock->streamOffset[StreamNumber]     = OffsetInBytes;
    }

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        if (pStreamData) IWineD3DBuffer_AddRef(pStreamData);
        if (oldSrc) IWineD3DBuffer_Release(oldSrc);
        return WINED3D_OK;
    }

    if (pStreamData != NULL) {
        InterlockedIncrement(&((struct wined3d_buffer *)pStreamData)->bind_count);
        IWineD3DBuffer_AddRef(pStreamData);
    }
    if (oldSrc != NULL) {
        InterlockedDecrement(&((struct wined3d_buffer *)oldSrc)->bind_count);
        IWineD3DBuffer_Release(oldSrc);
    }

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_STREAMSRC);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetStreamSource(IWineD3DDevice *iface,
        UINT StreamNumber, IWineD3DBuffer **pStream, UINT *pOffset, UINT *pStride)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : StreamNo: %u, Stream (%p), Offset %u, Stride %u\n", This, StreamNumber,
           This->stateBlock->streamSource[StreamNumber],
           This->stateBlock->streamOffset[StreamNumber],
           This->stateBlock->streamStride[StreamNumber]);

    if (StreamNumber >= MAX_STREAMS) {
        WARN("Stream out of range %d\n", StreamNumber);
        return WINED3DERR_INVALIDCALL;
    }
    *pStream = This->stateBlock->streamSource[StreamNumber];
    *pStride = This->stateBlock->streamStride[StreamNumber];
    if (pOffset) {
        *pOffset = This->stateBlock->streamOffset[StreamNumber];
    }

    if (*pStream != NULL) {
        IWineD3DBuffer_AddRef(*pStream); /* We have created a new reference to the VB */
    }
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetStreamSourceFreq(IWineD3DDevice *iface,  UINT StreamNumber, UINT Divider) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    UINT oldFlags = This->updateStateBlock->streamFlags[StreamNumber];
    UINT oldFreq = This->updateStateBlock->streamFreq[StreamNumber];

    /* Verify input at least in d3d9 this is invalid*/
    if( (Divider & WINED3DSTREAMSOURCE_INSTANCEDATA) && (Divider & WINED3DSTREAMSOURCE_INDEXEDDATA)){
        WARN("INSTANCEDATA and INDEXEDDATA were set, returning D3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }
    if( (Divider & WINED3DSTREAMSOURCE_INSTANCEDATA) && StreamNumber == 0 ){
        WARN("INSTANCEDATA used on stream 0, returning D3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }
    if( Divider == 0 ){
        WARN("Divider is 0, returning D3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }

    TRACE("(%p) StreamNumber(%d), Divider(%d)\n", This, StreamNumber, Divider);
    This->updateStateBlock->streamFlags[StreamNumber] = Divider & (WINED3DSTREAMSOURCE_INSTANCEDATA  | WINED3DSTREAMSOURCE_INDEXEDDATA );

    This->updateStateBlock->changed.streamFreq |= 1 << StreamNumber;
    This->updateStateBlock->streamFreq[StreamNumber]          = Divider & 0x7FFFFF;

    if(This->updateStateBlock->streamFreq[StreamNumber] != oldFreq ||
       This->updateStateBlock->streamFlags[StreamNumber] != oldFlags) {
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_STREAMSRC);
    }

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetStreamSourceFreq(IWineD3DDevice *iface,  UINT StreamNumber, UINT* Divider) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) StreamNumber(%d), Divider(%p)\n", This, StreamNumber, Divider);
    *Divider = This->updateStateBlock->streamFreq[StreamNumber] | This->updateStateBlock->streamFlags[StreamNumber];

    TRACE("(%p) : returning %d\n", This, *Divider);

    return WINED3D_OK;
}

/*****
 * Get / Set & Multiply Transform
 *****/
static HRESULT  WINAPI  IWineD3DDeviceImpl_SetTransform(IWineD3DDevice *iface, WINED3DTRANSFORMSTATETYPE d3dts, CONST WINED3DMATRIX* lpmatrix) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    /* Most of this routine, comments included copied from ddraw tree initially: */
    TRACE("(%p) : Transform State=%s\n", This, debug_d3dtstype(d3dts));

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        This->updateStateBlock->changed.transform[d3dts >> 5] |= 1 << (d3dts & 0x1f);
        This->updateStateBlock->transforms[d3dts] = *lpmatrix;
        return WINED3D_OK;
    }

    /*
     * If the new matrix is the same as the current one,
     * we cut off any further processing. this seems to be a reasonable
     * optimization because as was noticed, some apps (warcraft3 for example)
     * tend towards setting the same matrix repeatedly for some reason.
     *
     * From here on we assume that the new matrix is different, wherever it matters.
     */
    if (!memcmp(&This->stateBlock->transforms[d3dts].u.m[0][0], lpmatrix, sizeof(WINED3DMATRIX))) {
        TRACE("The app is setting the same matrix over again\n");
        return WINED3D_OK;
    } else {
        conv_mat(lpmatrix, &This->stateBlock->transforms[d3dts].u.m[0][0]);
    }

    /*
       ScreenCoord = ProjectionMat * ViewMat * WorldMat * ObjectCoord
       where ViewMat = Camera space, WorldMat = world space.

       In OpenGL, camera and world space is combined into GL_MODELVIEW
       matrix.  The Projection matrix stay projection matrix.
     */

    /* Capture the times we can just ignore the change for now */
    if (d3dts == WINED3DTS_VIEW) { /* handle the VIEW matrix */
        This->view_ident = !memcmp(lpmatrix, identity, 16 * sizeof(float));
        /* Handled by the state manager */
    }

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_TRANSFORM(d3dts));
    return WINED3D_OK;

}
static HRESULT WINAPI IWineD3DDeviceImpl_GetTransform(IWineD3DDevice *iface, WINED3DTRANSFORMSTATETYPE State, WINED3DMATRIX* pMatrix) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : for Transform State %s\n", This, debug_d3dtstype(State));
    *pMatrix = This->stateBlock->transforms[State];
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_MultiplyTransform(IWineD3DDevice *iface, WINED3DTRANSFORMSTATETYPE State, CONST WINED3DMATRIX* pMatrix) {
    const WINED3DMATRIX *mat = NULL;
    WINED3DMATRIX temp;

    /* Note: Using 'updateStateBlock' rather than 'stateblock' in the code
     * below means it will be recorded in a state block change, but it
     * works regardless where it is recorded.
     * If this is found to be wrong, change to StateBlock.
     */
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : For state %s\n", This, debug_d3dtstype(State));

    if (State <= HIGHEST_TRANSFORMSTATE)
    {
        mat = &This->updateStateBlock->transforms[State];
    } else {
        FIXME("Unhandled transform state!!\n");
    }

    multiply_matrix(&temp, mat, pMatrix);

    /* Apply change via set transform - will reapply to eg. lights this way */
    return IWineD3DDeviceImpl_SetTransform(iface, State, &temp);
}

/*****
 * Get / Set Light
 *****/
/* Note lights are real special cases. Although the device caps state only eg. 8 are supported,
   you can reference any indexes you want as long as that number max are enabled at any
   one point in time! Therefore since the indexes can be anything, we need a hashmap of them.
   However, this causes stateblock problems. When capturing the state block, I duplicate the hashmap,
   but when recording, just build a chain pretty much of commands to be replayed.                  */

static HRESULT WINAPI IWineD3DDeviceImpl_SetLight(IWineD3DDevice *iface, DWORD Index, CONST WINED3DLIGHT* pLight) {
    float rho;
    PLIGHTINFOEL *object = NULL;
    UINT Hi = LIGHTMAP_HASHFUNC(Index);
    struct list *e;

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : Idx(%d), pLight(%p). Hash index is %d\n", This, Index, pLight, Hi);

    /* Check the parameter range. Need for speed most wanted sets junk lights which confuse
     * the gl driver.
     */
    if(!pLight) {
        WARN("Light pointer = NULL, returning WINED3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }

    switch(pLight->Type) {
        case WINED3DLIGHT_POINT:
        case WINED3DLIGHT_SPOT:
        case WINED3DLIGHT_PARALLELPOINT:
        case WINED3DLIGHT_GLSPOT:
            /* Incorrect attenuation values can cause the gl driver to crash. Happens with Need for speed
             * most wanted
             */
            if(pLight->Attenuation0 < 0.0 || pLight->Attenuation1 < 0.0 || pLight->Attenuation2 < 0.0) {
                WARN("Attenuation is negative, returning WINED3DERR_INVALIDCALL\n");
                return WINED3DERR_INVALIDCALL;
            }
            break;

        case WINED3DLIGHT_DIRECTIONAL:
            /* Ignores attenuation */
            break;

        default:
        WARN("Light type out of range, returning WINED3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }

    LIST_FOR_EACH(e, &This->updateStateBlock->lightMap[Hi]) {
        object = LIST_ENTRY(e, PLIGHTINFOEL, entry);
        if(object->OriginalIndex == Index) break;
        object = NULL;
    }

    if(!object) {
        TRACE("Adding new light\n");
        object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
        if(!object) {
            ERR("Out of memory error when allocating a light\n");
            return E_OUTOFMEMORY;
        }
        list_add_head(&This->updateStateBlock->lightMap[Hi], &object->entry);
        object->glIndex = -1;
        object->OriginalIndex = Index;
        object->changed = TRUE;
    }

    /* Initialize the object */
    TRACE("Light %d setting to type %d, Diffuse(%f,%f,%f,%f), Specular(%f,%f,%f,%f), Ambient(%f,%f,%f,%f)\n", Index, pLight->Type,
          pLight->Diffuse.r, pLight->Diffuse.g, pLight->Diffuse.b, pLight->Diffuse.a,
          pLight->Specular.r, pLight->Specular.g, pLight->Specular.b, pLight->Specular.a,
          pLight->Ambient.r, pLight->Ambient.g, pLight->Ambient.b, pLight->Ambient.a);
    TRACE("... Pos(%f,%f,%f), Dirn(%f,%f,%f)\n", pLight->Position.x, pLight->Position.y, pLight->Position.z,
          pLight->Direction.x, pLight->Direction.y, pLight->Direction.z);
    TRACE("... Range(%f), Falloff(%f), Theta(%f), Phi(%f)\n", pLight->Range, pLight->Falloff, pLight->Theta, pLight->Phi);

    /* Save away the information */
    object->OriginalParms = *pLight;

    switch (pLight->Type) {
    case WINED3DLIGHT_POINT:
        /* Position */
        object->lightPosn[0] = pLight->Position.x;
        object->lightPosn[1] = pLight->Position.y;
        object->lightPosn[2] = pLight->Position.z;
        object->lightPosn[3] = 1.0f;
        object->cutoff = 180.0f;
        /* FIXME: Range */
        break;

    case WINED3DLIGHT_DIRECTIONAL:
        /* Direction */
        object->lightPosn[0] = -pLight->Direction.x;
        object->lightPosn[1] = -pLight->Direction.y;
        object->lightPosn[2] = -pLight->Direction.z;
        object->lightPosn[3] = 0.0;
        object->exponent     = 0.0f;
        object->cutoff       = 180.0f;
        break;

    case WINED3DLIGHT_SPOT:
        /* Position */
        object->lightPosn[0] = pLight->Position.x;
        object->lightPosn[1] = pLight->Position.y;
        object->lightPosn[2] = pLight->Position.z;
        object->lightPosn[3] = 1.0;

        /* Direction */
        object->lightDirn[0] = pLight->Direction.x;
        object->lightDirn[1] = pLight->Direction.y;
        object->lightDirn[2] = pLight->Direction.z;
        object->lightDirn[3] = 1.0;

        /*
         * opengl-ish and d3d-ish spot lights use too different models for the
         * light "intensity" as a function of the angle towards the main light direction,
         * so we only can approximate very roughly.
         * however spot lights are rather rarely used in games (if ever used at all).
         * furthermore if still used, probably nobody pays attention to such details.
         */
        if (pLight->Falloff == 0) {
            /* Falloff = 0 is easy, because d3d's and opengl's spot light equations have the
             * falloff resp. exponent parameter as an exponent, so the spot light lighting
             * will always be 1.0 for both of them, and we don't have to care for the
             * rest of the rather complex calculation
             */
            object->exponent = 0;
        } else {
            rho = pLight->Theta + (pLight->Phi - pLight->Theta)/(2*pLight->Falloff);
            if (rho < 0.0001) rho = 0.0001f;
            object->exponent = -0.3/log(cos(rho/2));
        }
        if (object->exponent > 128.0) {
            object->exponent = 128.0;
        }
        object->cutoff = pLight->Phi*90/M_PI;

        /* FIXME: Range */
        break;

    default:
        FIXME("Unrecognized light type %d\n", pLight->Type);
    }

    /* Update the live definitions if the light is currently assigned a glIndex */
    if (object->glIndex != -1 && !This->isRecordingState) {
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_ACTIVELIGHT(object->glIndex));
    }
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetLight(IWineD3DDevice *iface, DWORD Index, WINED3DLIGHT* pLight) {
    PLIGHTINFOEL *lightInfo = NULL;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    DWORD Hi = LIGHTMAP_HASHFUNC(Index);
    struct list *e;
    TRACE("(%p) : Idx(%d), pLight(%p)\n", This, Index, pLight);

    LIST_FOR_EACH(e, &This->stateBlock->lightMap[Hi]) {
        lightInfo = LIST_ENTRY(e, PLIGHTINFOEL, entry);
        if(lightInfo->OriginalIndex == Index) break;
        lightInfo = NULL;
    }

    if (lightInfo == NULL) {
        TRACE("Light information requested but light not defined\n");
        return WINED3DERR_INVALIDCALL;
    }

    *pLight = lightInfo->OriginalParms;
    return WINED3D_OK;
}

/*****
 * Get / Set Light Enable
 *   (Note for consistency, renamed d3dx function by adding the 'set' prefix)
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_SetLightEnable(IWineD3DDevice *iface, DWORD Index, BOOL Enable) {
    PLIGHTINFOEL *lightInfo = NULL;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    UINT Hi = LIGHTMAP_HASHFUNC(Index);
    struct list *e;
    TRACE("(%p) : Idx(%d), enable? %d\n", This, Index, Enable);

    /* Tests show true = 128...not clear why */
    Enable = Enable? 128: 0;

    LIST_FOR_EACH(e, &This->updateStateBlock->lightMap[Hi]) {
        lightInfo = LIST_ENTRY(e, PLIGHTINFOEL, entry);
        if(lightInfo->OriginalIndex == Index) break;
        lightInfo = NULL;
    }
    TRACE("Found light: %p\n", lightInfo);

    /* Special case - enabling an undefined light creates one with a strict set of parms! */
    if (lightInfo == NULL) {

        TRACE("Light enabled requested but light not defined, so defining one!\n");
        IWineD3DDeviceImpl_SetLight(iface, Index, &WINED3D_default_light);

        /* Search for it again! Should be fairly quick as near head of list */
        LIST_FOR_EACH(e, &This->updateStateBlock->lightMap[Hi]) {
            lightInfo = LIST_ENTRY(e, PLIGHTINFOEL, entry);
            if(lightInfo->OriginalIndex == Index) break;
            lightInfo = NULL;
        }
        if (lightInfo == NULL) {
            FIXME("Adding default lights has failed dismally\n");
            return WINED3DERR_INVALIDCALL;
        }
    }

    lightInfo->enabledChanged = TRUE;
    if(!Enable) {
        if(lightInfo->glIndex != -1) {
            if(!This->isRecordingState) {
                IWineD3DDeviceImpl_MarkStateDirty(This, STATE_ACTIVELIGHT(lightInfo->glIndex));
            }

            This->updateStateBlock->activeLights[lightInfo->glIndex] = NULL;
            lightInfo->glIndex = -1;
        } else {
            TRACE("Light already disabled, nothing to do\n");
        }
        lightInfo->enabled = FALSE;
    } else {
        lightInfo->enabled = TRUE;
        if (lightInfo->glIndex != -1) {
            /* nop */
            TRACE("Nothing to do as light was enabled\n");
        } else {
            int i;
            /* Find a free gl light */
            for(i = 0; i < This->maxConcurrentLights; i++) {
                if(This->updateStateBlock->activeLights[i] == NULL) {
                    This->updateStateBlock->activeLights[i] = lightInfo;
                    lightInfo->glIndex = i;
                    break;
                }
            }
            if(lightInfo->glIndex == -1) {
                /* Our tests show that Windows returns D3D_OK in this situation, even with
                 * D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE devices. This
                 * is consistent among ddraw, d3d8 and d3d9. GetLightEnable returns TRUE
                 * as well for those lights.
                 *
                 * TODO: Test how this affects rendering
                 */
                WARN("Too many concurrently active lights\n");
                return WINED3D_OK;
            }

            /* i == lightInfo->glIndex */
            if(!This->isRecordingState) {
                IWineD3DDeviceImpl_MarkStateDirty(This, STATE_ACTIVELIGHT(i));
            }
        }
    }

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetLightEnable(IWineD3DDevice *iface, DWORD Index,BOOL* pEnable) {

    PLIGHTINFOEL *lightInfo = NULL;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    struct list *e;
    UINT Hi = LIGHTMAP_HASHFUNC(Index);
    TRACE("(%p) : for idx(%d)\n", This, Index);

    LIST_FOR_EACH(e, &This->stateBlock->lightMap[Hi]) {
        lightInfo = LIST_ENTRY(e, PLIGHTINFOEL, entry);
        if(lightInfo->OriginalIndex == Index) break;
        lightInfo = NULL;
    }

    if (lightInfo == NULL) {
        TRACE("Light enabled state requested but light not defined\n");
        return WINED3DERR_INVALIDCALL;
    }
    /* true is 128 according to SetLightEnable */
    *pEnable = lightInfo->enabled ? 128 : 0;
    return WINED3D_OK;
}

/*****
 * Get / Set Clip Planes
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_SetClipPlane(IWineD3DDevice *iface, DWORD Index, CONST float *pPlane) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : for idx %d, %p\n", This, Index, pPlane);

    /* Validate Index */
    if (Index >= GL_LIMITS(clipplanes)) {
        TRACE("Application has requested clipplane this device doesn't support\n");
        return WINED3DERR_INVALIDCALL;
    }

    This->updateStateBlock->changed.clipplane |= 1 << Index;

    if(This->updateStateBlock->clipplane[Index][0] == pPlane[0] &&
       This->updateStateBlock->clipplane[Index][1] == pPlane[1] &&
       This->updateStateBlock->clipplane[Index][2] == pPlane[2] &&
       This->updateStateBlock->clipplane[Index][3] == pPlane[3]) {
        TRACE("Application is setting old values over, nothing to do\n");
        return WINED3D_OK;
    }

    This->updateStateBlock->clipplane[Index][0] = pPlane[0];
    This->updateStateBlock->clipplane[Index][1] = pPlane[1];
    This->updateStateBlock->clipplane[Index][2] = pPlane[2];
    This->updateStateBlock->clipplane[Index][3] = pPlane[3];

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return WINED3D_OK;
    }

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_CLIPPLANE(Index));

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetClipPlane(IWineD3DDevice *iface, DWORD Index, float *pPlane) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : for idx %d\n", This, Index);

    /* Validate Index */
    if (Index >= GL_LIMITS(clipplanes)) {
        TRACE("Application has requested clipplane this device doesn't support\n");
        return WINED3DERR_INVALIDCALL;
    }

    pPlane[0] = This->stateBlock->clipplane[Index][0];
    pPlane[1] = This->stateBlock->clipplane[Index][1];
    pPlane[2] = This->stateBlock->clipplane[Index][2];
    pPlane[3] = This->stateBlock->clipplane[Index][3];
    return WINED3D_OK;
}

/*****
 * Get / Set Clip Plane Status
 *   WARNING: This code relies on the fact that D3DCLIPSTATUS8 == D3DCLIPSTATUS9
 *****/
static HRESULT  WINAPI  IWineD3DDeviceImpl_SetClipStatus(IWineD3DDevice *iface, CONST WINED3DCLIPSTATUS* pClipStatus) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    FIXME("(%p) : stub\n", This);
    if (NULL == pClipStatus) {
      return WINED3DERR_INVALIDCALL;
    }
    This->updateStateBlock->clip_status.ClipUnion = pClipStatus->ClipUnion;
    This->updateStateBlock->clip_status.ClipIntersection = pClipStatus->ClipIntersection;
    return WINED3D_OK;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_GetClipStatus(IWineD3DDevice *iface, WINED3DCLIPSTATUS* pClipStatus) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    FIXME("(%p) : stub\n", This);
    if (NULL == pClipStatus) {
      return WINED3DERR_INVALIDCALL;
    }
    pClipStatus->ClipUnion = This->updateStateBlock->clip_status.ClipUnion;
    pClipStatus->ClipIntersection = This->updateStateBlock->clip_status.ClipIntersection;
    return WINED3D_OK;
}

/*****
 * Get / Set Material
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_SetMaterial(IWineD3DDevice *iface, CONST WINED3DMATERIAL* pMaterial) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    This->updateStateBlock->changed.material = TRUE;
    This->updateStateBlock->material = *pMaterial;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return WINED3D_OK;
    }

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_MATERIAL);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetMaterial(IWineD3DDevice *iface, WINED3DMATERIAL* pMaterial) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    *pMaterial = This->updateStateBlock->material;
    TRACE("(%p) : Diffuse (%f,%f,%f,%f)\n", This, pMaterial->Diffuse.r, pMaterial->Diffuse.g,
        pMaterial->Diffuse.b, pMaterial->Diffuse.a);
    TRACE("(%p) : Ambient (%f,%f,%f,%f)\n", This, pMaterial->Ambient.r, pMaterial->Ambient.g,
        pMaterial->Ambient.b, pMaterial->Ambient.a);
    TRACE("(%p) : Specular (%f,%f,%f,%f)\n", This, pMaterial->Specular.r, pMaterial->Specular.g,
        pMaterial->Specular.b, pMaterial->Specular.a);
    TRACE("(%p) : Emissive (%f,%f,%f,%f)\n", This, pMaterial->Emissive.r, pMaterial->Emissive.g,
        pMaterial->Emissive.b, pMaterial->Emissive.a);
    TRACE("(%p) : Power (%f)\n", This, pMaterial->Power);

    return WINED3D_OK;
}

/*****
 * Get / Set Indices
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_SetIndices(IWineD3DDevice *iface, IWineD3DIndexBuffer* pIndexData) {
    IWineD3DDeviceImpl  *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DIndexBuffer *oldIdxs;

    TRACE("(%p) : Setting to %p\n", This, pIndexData);
    oldIdxs = This->updateStateBlock->pIndexData;

    This->updateStateBlock->changed.indices = TRUE;
    This->updateStateBlock->pIndexData = pIndexData;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        if(pIndexData) IWineD3DIndexBuffer_AddRef(pIndexData);
        if(oldIdxs) IWineD3DIndexBuffer_Release(oldIdxs);
        return WINED3D_OK;
    }

    if(oldIdxs != pIndexData) {
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_INDEXBUFFER);
        if(pIndexData) IWineD3DIndexBuffer_AddRef(pIndexData);
        if(oldIdxs) IWineD3DIndexBuffer_Release(oldIdxs);
    }
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetIndices(IWineD3DDevice *iface, IWineD3DIndexBuffer** ppIndexData) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    *ppIndexData = This->stateBlock->pIndexData;

    /* up ref count on ppindexdata */
    if (*ppIndexData) {
        IWineD3DIndexBuffer_AddRef(*ppIndexData);
        TRACE("(%p) index data set to %p\n", This, ppIndexData);
    }else{
        TRACE("(%p) No index data set\n", This);
    }
    TRACE("Returning %p\n", *ppIndexData);

    return WINED3D_OK;
}

/* Method to offer d3d9 a simple way to set the base vertex index without messing with the index buffer */
static HRESULT WINAPI IWineD3DDeviceImpl_SetBaseVertexIndex(IWineD3DDevice *iface, INT BaseIndex) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p)->(%d)\n", This, BaseIndex);

    if(This->updateStateBlock->baseVertexIndex == BaseIndex) {
        TRACE("Application is setting the old value over, nothing to do\n");
        return WINED3D_OK;
    }

    This->updateStateBlock->baseVertexIndex = BaseIndex;

    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return WINED3D_OK;
    }
    /* The base vertex index affects the stream sources */
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_STREAMSRC);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetBaseVertexIndex(IWineD3DDevice *iface, INT* base_index) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : base_index %p\n", This, base_index);

    *base_index = This->stateBlock->baseVertexIndex;

    TRACE("Returning %u\n", *base_index);

    return WINED3D_OK;
}

/*****
 * Get / Set Viewports
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_SetViewport(IWineD3DDevice *iface, CONST WINED3DVIEWPORT* pViewport) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p)\n", This);
    This->updateStateBlock->changed.viewport = TRUE;
    This->updateStateBlock->viewport = *pViewport;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return WINED3D_OK;
    }

    TRACE("(%p) : x=%d, y=%d, wid=%d, hei=%d, minz=%f, maxz=%f\n", This,
          pViewport->X, pViewport->Y, pViewport->Width, pViewport->Height, pViewport->MinZ, pViewport->MaxZ);

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_VIEWPORT);
    return WINED3D_OK;

}

static HRESULT WINAPI IWineD3DDeviceImpl_GetViewport(IWineD3DDevice *iface, WINED3DVIEWPORT* pViewport) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p)\n", This);
    *pViewport = This->stateBlock->viewport;
    return WINED3D_OK;
}

/*****
 * Get / Set Render States
 * TODO: Verify against dx9 definitions
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_SetRenderState(IWineD3DDevice *iface, WINED3DRENDERSTATETYPE State, DWORD Value) {

    IWineD3DDeviceImpl  *This     = (IWineD3DDeviceImpl *)iface;
    DWORD oldValue = This->stateBlock->renderState[State];

    TRACE("(%p)->state = %s(%d), value = %d\n", This, debug_d3drenderstate(State), State, Value);

    This->updateStateBlock->changed.renderState[State >> 5] |= 1 << (State & 0x1f);
    This->updateStateBlock->renderState[State] = Value;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return WINED3D_OK;
    }

    /* Compared here and not before the assignment to allow proper stateblock recording */
    if(Value == oldValue) {
        TRACE("Application is setting the old value over, nothing to do\n");
    } else {
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(State));
    }

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetRenderState(IWineD3DDevice *iface, WINED3DRENDERSTATETYPE State, DWORD *pValue) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) for State %d = %d\n", This, State, This->stateBlock->renderState[State]);
    *pValue = This->stateBlock->renderState[State];
    return WINED3D_OK;
}

/*****
 * Get / Set Sampler States
 * TODO: Verify against dx9 definitions
 *****/

static HRESULT WINAPI IWineD3DDeviceImpl_SetSamplerState(IWineD3DDevice *iface, DWORD Sampler, WINED3DSAMPLERSTATETYPE Type, DWORD Value) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    DWORD oldValue;

    TRACE("(%p) : Sampler %#x, Type %s (%#x), Value %#x\n",
            This, Sampler, debug_d3dsamplerstate(Type), Type, Value);

    if (Sampler >= WINED3DVERTEXTEXTURESAMPLER0 && Sampler <= WINED3DVERTEXTEXTURESAMPLER3) {
        Sampler -= (WINED3DVERTEXTEXTURESAMPLER0 - MAX_FRAGMENT_SAMPLERS);
    }

    if (Sampler >= sizeof(This->stateBlock->samplerState)/sizeof(This->stateBlock->samplerState[0])) {
        ERR("Current Sampler overflows sampleState0 array (sampler %d)\n", Sampler);
        return WINED3D_OK; /* Windows accepts overflowing this array ... we do not. */
    }
    /**
    * SetSampler is designed to allow for more than the standard up to 8 textures
    *  and Geforce has stopped supporting more than 6 standard textures in openGL.
    * So I have to use ARB for Gforce. (maybe if the sampler > 4 then use ARB?)
    *
    * http://developer.nvidia.com/object/General_FAQ.html#t6
    *
    * There are two new settings for GForce
    * the sampler one:
    * GL_MAX_TEXTURE_IMAGE_UNITS_ARB
    * and the texture one:
    * GL_MAX_TEXTURE_COORDS_ARB.
    * Ok GForce say it's ok to use glTexParameter/glGetTexParameter(...).
     ******************/

    oldValue = This->stateBlock->samplerState[Sampler][Type];
    This->updateStateBlock->samplerState[Sampler][Type]         = Value;
    This->updateStateBlock->changed.samplerState[Sampler] |= 1 << Type;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return WINED3D_OK;
    }

    if(oldValue == Value) {
        TRACE("Application is setting the old value over, nothing to do\n");
        return WINED3D_OK;
    }

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_SAMPLER(Sampler));

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetSamplerState(IWineD3DDevice *iface, DWORD Sampler, WINED3DSAMPLERSTATETYPE Type, DWORD* Value) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : Sampler %#x, Type %s (%#x)\n",
            This, Sampler, debug_d3dsamplerstate(Type), Type);

    if (Sampler >= WINED3DVERTEXTEXTURESAMPLER0 && Sampler <= WINED3DVERTEXTEXTURESAMPLER3) {
        Sampler -= (WINED3DVERTEXTEXTURESAMPLER0 - MAX_FRAGMENT_SAMPLERS);
    }

    if (Sampler >= sizeof(This->stateBlock->samplerState)/sizeof(This->stateBlock->samplerState[0])) {
        ERR("Current Sampler overflows sampleState0 array (sampler %d)\n", Sampler);
        return WINED3D_OK; /* Windows accepts overflowing this array ... we do not. */
    }
    *Value = This->stateBlock->samplerState[Sampler][Type];
    TRACE("(%p) : Returning %#x\n", This, *Value);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetScissorRect(IWineD3DDevice *iface, CONST RECT* pRect) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    This->updateStateBlock->changed.scissorRect = TRUE;
    if(EqualRect(&This->updateStateBlock->scissorRect, pRect)) {
        TRACE("App is setting the old scissor rectangle over, nothing to do\n");
        return WINED3D_OK;
    }
    CopyRect(&This->updateStateBlock->scissorRect, pRect);

    if(This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return WINED3D_OK;
    }

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_SCISSORRECT);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetScissorRect(IWineD3DDevice *iface, RECT* pRect) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    *pRect = This->updateStateBlock->scissorRect;
    TRACE("(%p)Returning a Scissor Rect of %d:%d-%d:%d\n", This, pRect->left, pRect->top, pRect->right, pRect->bottom);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetVertexDeclaration(IWineD3DDevice* iface, IWineD3DVertexDeclaration* pDecl) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    IWineD3DVertexDeclaration *oldDecl = This->updateStateBlock->vertexDecl;

    TRACE("(%p) : pDecl=%p\n", This, pDecl);

    This->updateStateBlock->vertexDecl = pDecl;
    This->updateStateBlock->changed.vertexDecl = TRUE;

    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return WINED3D_OK;
    } else if(pDecl == oldDecl) {
        /* Checked after the assignment to allow proper stateblock recording */
        TRACE("Application is setting the old declaration over, nothing to do\n");
        return WINED3D_OK;
    }

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_VDECL);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetVertexDeclaration(IWineD3DDevice* iface, IWineD3DVertexDeclaration** ppDecl) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : ppDecl=%p\n", This, ppDecl);

    *ppDecl = This->stateBlock->vertexDecl;
    if (NULL != *ppDecl) IWineD3DVertexDeclaration_AddRef(*ppDecl);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetVertexShader(IWineD3DDevice *iface, IWineD3DVertexShader* pShader) {
    IWineD3DDeviceImpl *This        = (IWineD3DDeviceImpl *)iface;
    IWineD3DVertexShader* oldShader = This->updateStateBlock->vertexShader;

    This->updateStateBlock->vertexShader         = pShader;
    This->updateStateBlock->changed.vertexShader = TRUE;

    if (This->isRecordingState) {
        if(pShader) IWineD3DVertexShader_AddRef(pShader);
        if(oldShader) IWineD3DVertexShader_Release(oldShader);
        TRACE("Recording... not performing anything\n");
        return WINED3D_OK;
    } else if(oldShader == pShader) {
        /* Checked here to allow proper stateblock recording */
        TRACE("App is setting the old shader over, nothing to do\n");
        return WINED3D_OK;
    }

    TRACE("(%p) : setting pShader(%p)\n", This, pShader);
    if(pShader) IWineD3DVertexShader_AddRef(pShader);
    if(oldShader) IWineD3DVertexShader_Release(oldShader);

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_VSHADER);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetVertexShader(IWineD3DDevice *iface, IWineD3DVertexShader** ppShader) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    if (NULL == ppShader) {
        return WINED3DERR_INVALIDCALL;
    }
    *ppShader = This->stateBlock->vertexShader;
    if( NULL != *ppShader)
        IWineD3DVertexShader_AddRef(*ppShader);

    TRACE("(%p) : returning %p\n", This, *ppShader);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetVertexShaderConstantB(
    IWineD3DDevice *iface,
    UINT start,
    CONST BOOL *srcData,
    UINT count) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    unsigned int i, cnt = min(count, MAX_CONST_B - start);

    TRACE("(iface %p, srcData %p, start %d, count %d)\n",
            iface, srcData, start, count);

    if (!srcData || start >= MAX_CONST_B) return WINED3DERR_INVALIDCALL;

    memcpy(&This->updateStateBlock->vertexShaderConstantB[start], srcData, cnt * sizeof(BOOL));
    for (i = 0; i < cnt; i++)
        TRACE("Set BOOL constant %u to %s\n", start + i, srcData[i]? "true":"false");

    for (i = start; i < cnt + start; ++i) {
        This->updateStateBlock->changed.vertexShaderConstantsB |= (1 << i);
    }

    if (!This->isRecordingState) IWineD3DDeviceImpl_MarkStateDirty(This, STATE_VERTEXSHADERCONSTANT);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetVertexShaderConstantB(
    IWineD3DDevice *iface,
    UINT start,
    BOOL *dstData,
    UINT count) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int cnt = min(count, MAX_CONST_B - start);

    TRACE("(iface %p, dstData %p, start %d, count %d)\n",
            iface, dstData, start, count);

    if (dstData == NULL || cnt < 0)
        return WINED3DERR_INVALIDCALL;

    memcpy(dstData, &This->stateBlock->vertexShaderConstantB[start], cnt * sizeof(BOOL));
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetVertexShaderConstantI(
    IWineD3DDevice *iface,
    UINT start,
    CONST int *srcData,
    UINT count) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    unsigned int i, cnt = min(count, MAX_CONST_I - start);

    TRACE("(iface %p, srcData %p, start %d, count %d)\n",
            iface, srcData, start, count);

    if (!srcData || start >= MAX_CONST_I) return WINED3DERR_INVALIDCALL;

    memcpy(&This->updateStateBlock->vertexShaderConstantI[start * 4], srcData, cnt * sizeof(int) * 4);
    for (i = 0; i < cnt; i++)
        TRACE("Set INT constant %u to { %d, %d, %d, %d }\n", start + i,
           srcData[i*4], srcData[i*4+1], srcData[i*4+2], srcData[i*4+3]);

    for (i = start; i < cnt + start; ++i) {
        This->updateStateBlock->changed.vertexShaderConstantsI |= (1 << i);
    }

    if (!This->isRecordingState) IWineD3DDeviceImpl_MarkStateDirty(This, STATE_VERTEXSHADERCONSTANT);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetVertexShaderConstantI(
    IWineD3DDevice *iface,
    UINT start,
    int *dstData,
    UINT count) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int cnt = min(count, MAX_CONST_I - start);

    TRACE("(iface %p, dstData %p, start %d, count %d)\n",
            iface, dstData, start, count);

    if (dstData == NULL || ((signed int) MAX_CONST_I - (signed int) start) <= (signed int) 0)
        return WINED3DERR_INVALIDCALL;

    memcpy(dstData, &This->stateBlock->vertexShaderConstantI[start * 4], cnt * sizeof(int) * 4);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetVertexShaderConstantF(
    IWineD3DDevice *iface,
    UINT start,
    CONST float *srcData,
    UINT count) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    UINT i;

    TRACE("(iface %p, srcData %p, start %d, count %d)\n",
            iface, srcData, start, count);

    /* Specifically test start > limit to catch MAX_UINT overflows when adding start + count */
    if (srcData == NULL || start + count > GL_LIMITS(vshader_constantsF) || start > GL_LIMITS(vshader_constantsF))
        return WINED3DERR_INVALIDCALL;

    memcpy(&This->updateStateBlock->vertexShaderConstantF[start * 4], srcData, count * sizeof(float) * 4);
    if(TRACE_ON(d3d)) {
        for (i = 0; i < count; i++)
            TRACE("Set FLOAT constant %u to { %f, %f, %f, %f }\n", start + i,
                srcData[i*4], srcData[i*4+1], srcData[i*4+2], srcData[i*4+3]);
    }

    if (!This->isRecordingState)
    {
        This->shader_backend->shader_update_float_vertex_constants(iface, start, count);
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_VERTEXSHADERCONSTANT);
    }

    memset(This->updateStateBlock->changed.vertexShaderConstantsF + start, 1,
            sizeof(*This->updateStateBlock->changed.vertexShaderConstantsF) * count);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetVertexShaderConstantF(
    IWineD3DDevice *iface,
    UINT start,
    float *dstData,
    UINT count) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int cnt = min(count, GL_LIMITS(vshader_constantsF) - start);

    TRACE("(iface %p, dstData %p, start %d, count %d)\n",
            iface, dstData, start, count);

    if (dstData == NULL || cnt < 0)
        return WINED3DERR_INVALIDCALL;

    memcpy(dstData, &This->stateBlock->vertexShaderConstantF[start * 4], cnt * sizeof(float) * 4);
    return WINED3D_OK;
}

static inline void markTextureStagesDirty(IWineD3DDeviceImpl *This, DWORD stage) {
    DWORD i;
    for(i = 0; i <= WINED3D_HIGHEST_TEXTURE_STATE; ++i)
    {
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_TEXTURESTAGE(stage, i));
    }
}

static void device_map_stage(IWineD3DDeviceImpl *This, int stage, int unit) {
    int i = This->rev_tex_unit_map[unit];
    int j = This->texUnitMap[stage];

    This->texUnitMap[stage] = unit;
    if (i != -1 && i != stage) {
        This->texUnitMap[i] = -1;
    }

    This->rev_tex_unit_map[unit] = stage;
    if (j != -1 && j != unit) {
        This->rev_tex_unit_map[j] = -1;
    }
}

static void device_update_fixed_function_usage_map(IWineD3DDeviceImpl *This) {
    int i;

    This->fixed_function_usage_map = 0;
    for (i = 0; i < MAX_TEXTURES; ++i) {
        WINED3DTEXTUREOP color_op = This->stateBlock->textureState[i][WINED3DTSS_COLOROP];
        WINED3DTEXTUREOP alpha_op = This->stateBlock->textureState[i][WINED3DTSS_ALPHAOP];
        DWORD color_arg1 = This->stateBlock->textureState[i][WINED3DTSS_COLORARG1] & WINED3DTA_SELECTMASK;
        DWORD color_arg2 = This->stateBlock->textureState[i][WINED3DTSS_COLORARG2] & WINED3DTA_SELECTMASK;
        DWORD color_arg3 = This->stateBlock->textureState[i][WINED3DTSS_COLORARG0] & WINED3DTA_SELECTMASK;
        DWORD alpha_arg1 = This->stateBlock->textureState[i][WINED3DTSS_ALPHAARG1] & WINED3DTA_SELECTMASK;
        DWORD alpha_arg2 = This->stateBlock->textureState[i][WINED3DTSS_ALPHAARG2] & WINED3DTA_SELECTMASK;
        DWORD alpha_arg3 = This->stateBlock->textureState[i][WINED3DTSS_ALPHAARG0] & WINED3DTA_SELECTMASK;

        if (color_op == WINED3DTOP_DISABLE) {
            /* Not used, and disable higher stages */
            break;
        }

        if (((color_arg1 == WINED3DTA_TEXTURE) && color_op != WINED3DTOP_SELECTARG2)
                || ((color_arg2 == WINED3DTA_TEXTURE) && color_op != WINED3DTOP_SELECTARG1)
                || ((color_arg3 == WINED3DTA_TEXTURE) && (color_op == WINED3DTOP_MULTIPLYADD || color_op == WINED3DTOP_LERP))
                || ((alpha_arg1 == WINED3DTA_TEXTURE) && alpha_op != WINED3DTOP_SELECTARG2)
                || ((alpha_arg2 == WINED3DTA_TEXTURE) && alpha_op != WINED3DTOP_SELECTARG1)
                || ((alpha_arg3 == WINED3DTA_TEXTURE) && (alpha_op == WINED3DTOP_MULTIPLYADD || alpha_op == WINED3DTOP_LERP))) {
            This->fixed_function_usage_map |= (1 << i);
        }

        if ((color_op == WINED3DTOP_BUMPENVMAP || color_op == WINED3DTOP_BUMPENVMAPLUMINANCE) && i < MAX_TEXTURES - 1) {
            This->fixed_function_usage_map |= (1 << (i + 1));
        }
    }
}

static void device_map_fixed_function_samplers(IWineD3DDeviceImpl *This) {
    unsigned int i, tex;
    WORD ffu_map;

    device_update_fixed_function_usage_map(This);
    ffu_map = This->fixed_function_usage_map;

    if (This->max_ffp_textures == This->max_ffp_texture_stages ||
            This->stateBlock->lowest_disabled_stage <= This->max_ffp_textures) {
        for (i = 0; ffu_map; ffu_map >>= 1, ++i)
        {
            if (!(ffu_map & 1)) continue;

            if (This->texUnitMap[i] != i) {
                device_map_stage(This, i, i);
                IWineD3DDeviceImpl_MarkStateDirty(This, STATE_SAMPLER(i));
                markTextureStagesDirty(This, i);
            }
        }
        return;
    }

    /* Now work out the mapping */
    tex = 0;
    for (i = 0; ffu_map; ffu_map >>= 1, ++i)
    {
        if (!(ffu_map & 1)) continue;

        if (This->texUnitMap[i] != tex) {
            device_map_stage(This, i, tex);
            IWineD3DDeviceImpl_MarkStateDirty(This, STATE_SAMPLER(i));
            markTextureStagesDirty(This, i);
        }

        ++tex;
    }
}

static void device_map_psamplers(IWineD3DDeviceImpl *This) {
    const DWORD *sampler_tokens =
            ((IWineD3DPixelShaderImpl *)This->stateBlock->pixelShader)->baseShader.reg_maps.samplers;
    unsigned int i;

    for (i = 0; i < MAX_FRAGMENT_SAMPLERS; ++i) {
        if (sampler_tokens[i] && This->texUnitMap[i] != i) {
            device_map_stage(This, i, i);
            IWineD3DDeviceImpl_MarkStateDirty(This, STATE_SAMPLER(i));
            if (i < MAX_TEXTURES) {
                markTextureStagesDirty(This, i);
            }
        }
    }
}

static BOOL device_unit_free_for_vs(IWineD3DDeviceImpl *This, const DWORD *pshader_sampler_tokens,
        const DWORD *vshader_sampler_tokens, int unit)
{
    int current_mapping = This->rev_tex_unit_map[unit];

    if (current_mapping == -1) {
        /* Not currently used */
        return TRUE;
    }

    if (current_mapping < MAX_FRAGMENT_SAMPLERS) {
        /* Used by a fragment sampler */

        if (!pshader_sampler_tokens) {
            /* No pixel shader, check fixed function */
            return current_mapping >= MAX_TEXTURES || !(This->fixed_function_usage_map & (1 << current_mapping));
        }

        /* Pixel shader, check the shader's sampler map */
        return !pshader_sampler_tokens[current_mapping];
    }

    /* Used by a vertex sampler */
    return !vshader_sampler_tokens[current_mapping];
}

static void device_map_vsamplers(IWineD3DDeviceImpl *This, BOOL ps) {
    const DWORD *vshader_sampler_tokens =
            ((IWineD3DVertexShaderImpl *)This->stateBlock->vertexShader)->baseShader.reg_maps.samplers;
    const DWORD *pshader_sampler_tokens = NULL;
    int start = GL_LIMITS(combined_samplers) - 1;
    int i;

    if (ps) {
        IWineD3DPixelShaderImpl *pshader = (IWineD3DPixelShaderImpl *)This->stateBlock->pixelShader;

        /* Note that we only care if a sampler is sampled or not, not the sampler's specific type.
         * Otherwise we'd need to call shader_update_samplers() here for 1.x pixelshaders. */
        pshader_sampler_tokens = pshader->baseShader.reg_maps.samplers;
    }

    for (i = 0; i < MAX_VERTEX_SAMPLERS; ++i) {
        int vsampler_idx = i + MAX_FRAGMENT_SAMPLERS;
        if (vshader_sampler_tokens[i]) {
            if (This->texUnitMap[vsampler_idx] != WINED3D_UNMAPPED_STAGE)
            {
                /* Already mapped somewhere */
                continue;
            }

            while (start >= 0) {
                if (device_unit_free_for_vs(This, pshader_sampler_tokens, vshader_sampler_tokens, start)) {
                    device_map_stage(This, vsampler_idx, start);
                    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_SAMPLER(vsampler_idx));

                    --start;
                    break;
                }

                --start;
            }
        }
    }
}

void IWineD3DDeviceImpl_FindTexUnitMap(IWineD3DDeviceImpl *This) {
    BOOL vs = use_vs(This->stateBlock);
    BOOL ps = use_ps(This->stateBlock);
    /*
     * Rules are:
     * -> Pixel shaders need a 1:1 map. In theory the shader input could be mapped too, but
     * that would be really messy and require shader recompilation
     * -> When the mapping of a stage is changed, sampler and ALL texture stage states have
     * to be reset. Because of that try to work with a 1:1 mapping as much as possible
     */
    if (ps) {
        device_map_psamplers(This);
    } else {
        device_map_fixed_function_samplers(This);
    }

    if (vs) {
        device_map_vsamplers(This, ps);
    }
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetPixelShader(IWineD3DDevice *iface, IWineD3DPixelShader *pShader) {
    IWineD3DDeviceImpl *This        = (IWineD3DDeviceImpl *)iface;
    IWineD3DPixelShader *oldShader  = This->updateStateBlock->pixelShader;
    This->updateStateBlock->pixelShader         = pShader;
    This->updateStateBlock->changed.pixelShader = TRUE;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
    }

    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        if(pShader) IWineD3DPixelShader_AddRef(pShader);
        if(oldShader) IWineD3DPixelShader_Release(oldShader);
        return WINED3D_OK;
    }

    if(pShader == oldShader) {
        TRACE("App is setting the old pixel shader over, nothing to do\n");
        return WINED3D_OK;
    }

    if(pShader) IWineD3DPixelShader_AddRef(pShader);
    if(oldShader) IWineD3DPixelShader_Release(oldShader);

    TRACE("(%p) : setting pShader(%p)\n", This, pShader);
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_PIXELSHADER);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetPixelShader(IWineD3DDevice *iface, IWineD3DPixelShader **ppShader) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    if (NULL == ppShader) {
        WARN("(%p) : PShader is NULL, returning INVALIDCALL\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    *ppShader =  This->stateBlock->pixelShader;
    if (NULL != *ppShader) {
        IWineD3DPixelShader_AddRef(*ppShader);
    }
    TRACE("(%p) : returning %p\n", This, *ppShader);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetPixelShaderConstantB(
    IWineD3DDevice *iface,
    UINT start,
    CONST BOOL *srcData,
    UINT count) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    unsigned int i, cnt = min(count, MAX_CONST_B - start);

    TRACE("(iface %p, srcData %p, start %u, count %u)\n",
            iface, srcData, start, count);

    if (!srcData || start >= MAX_CONST_B) return WINED3DERR_INVALIDCALL;

    memcpy(&This->updateStateBlock->pixelShaderConstantB[start], srcData, cnt * sizeof(BOOL));
    for (i = 0; i < cnt; i++)
        TRACE("Set BOOL constant %u to %s\n", start + i, srcData[i]? "true":"false");

    for (i = start; i < cnt + start; ++i) {
        This->updateStateBlock->changed.pixelShaderConstantsB |= (1 << i);
    }

    if (!This->isRecordingState) IWineD3DDeviceImpl_MarkStateDirty(This, STATE_PIXELSHADERCONSTANT);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetPixelShaderConstantB(
    IWineD3DDevice *iface,
    UINT start,
    BOOL *dstData,
    UINT count) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int cnt = min(count, MAX_CONST_B - start);

    TRACE("(iface %p, dstData %p, start %d, count %d)\n",
            iface, dstData, start, count);

    if (dstData == NULL || cnt < 0)
        return WINED3DERR_INVALIDCALL;

    memcpy(dstData, &This->stateBlock->pixelShaderConstantB[start], cnt * sizeof(BOOL));
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetPixelShaderConstantI(
    IWineD3DDevice *iface,
    UINT start,
    CONST int *srcData,
    UINT count) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    unsigned int i, cnt = min(count, MAX_CONST_I - start);

    TRACE("(iface %p, srcData %p, start %u, count %u)\n",
            iface, srcData, start, count);

    if (!srcData || start >= MAX_CONST_I) return WINED3DERR_INVALIDCALL;

    memcpy(&This->updateStateBlock->pixelShaderConstantI[start * 4], srcData, cnt * sizeof(int) * 4);
    for (i = 0; i < cnt; i++)
        TRACE("Set INT constant %u to { %d, %d, %d, %d }\n", start + i,
           srcData[i*4], srcData[i*4+1], srcData[i*4+2], srcData[i*4+3]);

    for (i = start; i < cnt + start; ++i) {
        This->updateStateBlock->changed.pixelShaderConstantsI |= (1 << i);
    }

    if (!This->isRecordingState) IWineD3DDeviceImpl_MarkStateDirty(This, STATE_PIXELSHADERCONSTANT);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetPixelShaderConstantI(
    IWineD3DDevice *iface,
    UINT start,
    int *dstData,
    UINT count) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int cnt = min(count, MAX_CONST_I - start);

    TRACE("(iface %p, dstData %p, start %d, count %d)\n",
            iface, dstData, start, count);

    if (dstData == NULL || cnt < 0)
        return WINED3DERR_INVALIDCALL;

    memcpy(dstData, &This->stateBlock->pixelShaderConstantI[start * 4], cnt * sizeof(int) * 4);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetPixelShaderConstantF(
    IWineD3DDevice *iface,
    UINT start,
    CONST float *srcData,
    UINT count) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    UINT i;

    TRACE("(iface %p, srcData %p, start %d, count %d)\n",
            iface, srcData, start, count);

    /* Specifically test start > limit to catch MAX_UINT overflows when adding start + count */
    if (srcData == NULL || start + count > GL_LIMITS(pshader_constantsF) || start > GL_LIMITS(pshader_constantsF))
        return WINED3DERR_INVALIDCALL;

    memcpy(&This->updateStateBlock->pixelShaderConstantF[start * 4], srcData, count * sizeof(float) * 4);
    if(TRACE_ON(d3d)) {
        for (i = 0; i < count; i++)
            TRACE("Set FLOAT constant %u to { %f, %f, %f, %f }\n", start + i,
                srcData[i*4], srcData[i*4+1], srcData[i*4+2], srcData[i*4+3]);
    }

    if (!This->isRecordingState)
    {
        This->shader_backend->shader_update_float_pixel_constants(iface, start, count);
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_PIXELSHADERCONSTANT);
    }

    memset(This->updateStateBlock->changed.pixelShaderConstantsF + start, 1,
            sizeof(*This->updateStateBlock->changed.pixelShaderConstantsF) * count);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetPixelShaderConstantF(
    IWineD3DDevice *iface,
    UINT start,
    float *dstData,
    UINT count) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int cnt = min(count, GL_LIMITS(pshader_constantsF) - start);

    TRACE("(iface %p, dstData %p, start %d, count %d)\n",
            iface, dstData, start, count);

    if (dstData == NULL || cnt < 0)
        return WINED3DERR_INVALIDCALL;

    memcpy(dstData, &This->stateBlock->pixelShaderConstantF[start * 4], cnt * sizeof(float) * 4);
    return WINED3D_OK;
}

#define copy_and_next(dest, src, size) memcpy(dest, src, size); dest += (size)
static HRESULT process_vertices_strided(IWineD3DDeviceImpl *This, DWORD dwDestIndex, DWORD dwCount,
        const struct wined3d_stream_info *stream_info, struct wined3d_buffer *dest, DWORD dwFlags)
{
    char *dest_ptr, *dest_conv = NULL, *dest_conv_addr = NULL;
    unsigned int i;
    DWORD DestFVF = dest->fvf;
    WINED3DVIEWPORT vp;
    WINED3DMATRIX mat, proj_mat, view_mat, world_mat;
    BOOL doClip;
    DWORD numTextures;

    if (stream_info->elements[WINED3D_FFP_NORMAL].data)
    {
        WARN(" lighting state not saved yet... Some strange stuff may happen !\n");
    }

    if (!stream_info->elements[WINED3D_FFP_POSITION].data)
    {
        ERR("Source has no position mask\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* We might access VBOs from this code, so hold the lock */
    ENTER_GL();

    if (dest->resource.allocatedMemory == NULL) {
        /* This may happen if we do direct locking into a vbo. Unlikely,
         * but theoretically possible(ddraw processvertices test)
         */
        dest->resource.allocatedMemory = HeapAlloc(GetProcessHeap(), 0, dest->resource.size);
        if(!dest->resource.allocatedMemory) {
            LEAVE_GL();
            ERR("Out of memory\n");
            return E_OUTOFMEMORY;
        }
        if (dest->buffer_object)
        {
            const void *src;
            GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, dest->buffer_object));
            checkGLcall("glBindBufferARB");
            src = GL_EXTCALL(glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_READ_ONLY_ARB));
            if(src) {
                memcpy(dest->resource.allocatedMemory, src, dest->resource.size);
            }
            GL_EXTCALL(glUnmapBufferARB(GL_ARRAY_BUFFER_ARB));
            checkGLcall("glUnmapBufferARB");
        }
    }

    /* Get a pointer into the destination vbo(create one if none exists) and
     * write correct opengl data into it. It's cheap and allows us to run drawStridedFast
     */
    if (!dest->buffer_object && GL_SUPPORT(ARB_VERTEX_BUFFER_OBJECT))
    {
        dest->flags |= WINED3D_BUFFER_CREATEBO;
        IWineD3DBuffer_PreLoad((IWineD3DBuffer *)dest);
    }

    if (dest->buffer_object)
    {
        unsigned char extrabytes = 0;
        /* If the destination vertex buffer has D3DFVF_XYZ position(non-rhw), native d3d writes RHW position, where the RHW
         * gets written into the 4 bytes after the Z position. In the case of a dest buffer that only has D3DFVF_XYZ data,
         * this may write 4 extra bytes beyond the area that should be written
         */
        if(DestFVF == WINED3DFVF_XYZ) extrabytes = 4;
        dest_conv_addr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwCount * get_flexible_vertex_size(DestFVF) + extrabytes);
        if(!dest_conv_addr) {
            ERR("Out of memory\n");
            /* Continue without storing converted vertices */
        }
        dest_conv = dest_conv_addr;
    }

    /* Should I clip?
     * a) WINED3DRS_CLIPPING is enabled
     * b) WINED3DVOP_CLIP is passed
     */
    if(This->stateBlock->renderState[WINED3DRS_CLIPPING]) {
        static BOOL warned = FALSE;
        /*
         * The clipping code is not quite correct. Some things need
         * to be checked against IDirect3DDevice3 (!), d3d8 and d3d9,
         * so disable clipping for now.
         * (The graphics in Half-Life are broken, and my processvertices
         *  test crashes with IDirect3DDevice3)
        doClip = TRUE;
         */
        doClip = FALSE;
        if(!warned) {
           warned = TRUE;
           FIXME("Clipping is broken and disabled for now\n");
        }
    } else doClip = FALSE;
    dest_ptr = ((char *) dest->resource.allocatedMemory) + dwDestIndex * get_flexible_vertex_size(DestFVF);

    IWineD3DDevice_GetTransform( (IWineD3DDevice *) This,
                                 WINED3DTS_VIEW,
                                 &view_mat);
    IWineD3DDevice_GetTransform( (IWineD3DDevice *) This,
                                 WINED3DTS_PROJECTION,
                                 &proj_mat);
    IWineD3DDevice_GetTransform( (IWineD3DDevice *) This,
                                 WINED3DTS_WORLDMATRIX(0),
                                 &world_mat);

    TRACE("View mat:\n");
    TRACE("%f %f %f %f\n", view_mat.u.s._11, view_mat.u.s._12, view_mat.u.s._13, view_mat.u.s._14);
    TRACE("%f %f %f %f\n", view_mat.u.s._21, view_mat.u.s._22, view_mat.u.s._23, view_mat.u.s._24);
    TRACE("%f %f %f %f\n", view_mat.u.s._31, view_mat.u.s._32, view_mat.u.s._33, view_mat.u.s._34);
    TRACE("%f %f %f %f\n", view_mat.u.s._41, view_mat.u.s._42, view_mat.u.s._43, view_mat.u.s._44);

    TRACE("Proj mat:\n");
    TRACE("%f %f %f %f\n", proj_mat.u.s._11, proj_mat.u.s._12, proj_mat.u.s._13, proj_mat.u.s._14);
    TRACE("%f %f %f %f\n", proj_mat.u.s._21, proj_mat.u.s._22, proj_mat.u.s._23, proj_mat.u.s._24);
    TRACE("%f %f %f %f\n", proj_mat.u.s._31, proj_mat.u.s._32, proj_mat.u.s._33, proj_mat.u.s._34);
    TRACE("%f %f %f %f\n", proj_mat.u.s._41, proj_mat.u.s._42, proj_mat.u.s._43, proj_mat.u.s._44);

    TRACE("World mat:\n");
    TRACE("%f %f %f %f\n", world_mat.u.s._11, world_mat.u.s._12, world_mat.u.s._13, world_mat.u.s._14);
    TRACE("%f %f %f %f\n", world_mat.u.s._21, world_mat.u.s._22, world_mat.u.s._23, world_mat.u.s._24);
    TRACE("%f %f %f %f\n", world_mat.u.s._31, world_mat.u.s._32, world_mat.u.s._33, world_mat.u.s._34);
    TRACE("%f %f %f %f\n", world_mat.u.s._41, world_mat.u.s._42, world_mat.u.s._43, world_mat.u.s._44);

    /* Get the viewport */
    IWineD3DDevice_GetViewport( (IWineD3DDevice *) This, &vp);
    TRACE("Viewport: X=%d, Y=%d, Width=%d, Height=%d, MinZ=%f, MaxZ=%f\n",
          vp.X, vp.Y, vp.Width, vp.Height, vp.MinZ, vp.MaxZ);

    multiply_matrix(&mat,&view_mat,&world_mat);
    multiply_matrix(&mat,&proj_mat,&mat);

    numTextures = (DestFVF & WINED3DFVF_TEXCOUNT_MASK) >> WINED3DFVF_TEXCOUNT_SHIFT;

    for (i = 0; i < dwCount; i+= 1) {
        unsigned int tex_index;

        if ( ((DestFVF & WINED3DFVF_POSITION_MASK) == WINED3DFVF_XYZ ) ||
             ((DestFVF & WINED3DFVF_POSITION_MASK) == WINED3DFVF_XYZRHW ) ) {
            /* The position first */
            const struct wined3d_stream_info_element *element = &stream_info->elements[WINED3D_FFP_POSITION];
            const float *p = (const float *)(element->data + i * element->stride);
            float x, y, z, rhw;
            TRACE("In: ( %06.2f %06.2f %06.2f )\n", p[0], p[1], p[2]);

            /* Multiplication with world, view and projection matrix */
            x =   (p[0] * mat.u.s._11) + (p[1] * mat.u.s._21) + (p[2] * mat.u.s._31) + (1.0 * mat.u.s._41);
            y =   (p[0] * mat.u.s._12) + (p[1] * mat.u.s._22) + (p[2] * mat.u.s._32) + (1.0 * mat.u.s._42);
            z =   (p[0] * mat.u.s._13) + (p[1] * mat.u.s._23) + (p[2] * mat.u.s._33) + (1.0 * mat.u.s._43);
            rhw = (p[0] * mat.u.s._14) + (p[1] * mat.u.s._24) + (p[2] * mat.u.s._34) + (1.0 * mat.u.s._44);

            TRACE("x=%f y=%f z=%f rhw=%f\n", x, y, z, rhw);

            /* WARNING: The following things are taken from d3d7 and were not yet checked
             * against d3d8 or d3d9!
             */

            /* Clipping conditions: From msdn
             *
             * A vertex is clipped if it does not match the following requirements
             * -rhw < x <= rhw
             * -rhw < y <= rhw
             *    0 < z <= rhw
             *    0 < rhw ( Not in d3d7, but tested in d3d7)
             *
             * If clipping is on is determined by the D3DVOP_CLIP flag in D3D7, and
             * by the D3DRS_CLIPPING in D3D9(according to the msdn, not checked)
             *
             */

            if( !doClip ||
                ( (-rhw -eps < x) && (-rhw -eps < y) && ( -eps < z) &&
                  (x <= rhw + eps) && (y <= rhw + eps ) && (z <= rhw + eps) && 
                  ( rhw > eps ) ) ) {

                /* "Normal" viewport transformation (not clipped)
                 * 1) The values are divided by rhw
                 * 2) The y axis is negative, so multiply it with -1
                 * 3) Screen coordinates go from -(Width/2) to +(Width/2) and
                 *    -(Height/2) to +(Height/2). The z range is MinZ to MaxZ
                 * 4) Multiply x with Width/2 and add Width/2
                 * 5) The same for the height
                 * 6) Add the viewpoint X and Y to the 2D coordinates and
                 *    The minimum Z value to z
                 * 7) rhw = 1 / rhw Reciprocal of Homogeneous W....
                 *
                 * Well, basically it's simply a linear transformation into viewport
                 * coordinates
                 */

                x /= rhw;
                y /= rhw;
                z /= rhw;

                y *= -1;

                x *= vp.Width / 2;
                y *= vp.Height / 2;
                z *= vp.MaxZ - vp.MinZ;

                x += vp.Width / 2 + vp.X;
                y += vp.Height / 2 + vp.Y;
                z += vp.MinZ;

                rhw = 1 / rhw;
            } else {
                /* That vertex got clipped
                 * Contrary to OpenGL it is not dropped completely, it just
                 * undergoes a different calculation.
                 */
                TRACE("Vertex got clipped\n");
                x += rhw;
                y += rhw;

                x  /= 2;
                y  /= 2;

                /* Msdn mentions that Direct3D9 keeps a list of clipped vertices
                 * outside of the main vertex buffer memory. That needs some more
                 * investigation...
                 */
            }

            TRACE("Writing (%f %f %f) %f\n", x, y, z, rhw);


            ( (float *) dest_ptr)[0] = x;
            ( (float *) dest_ptr)[1] = y;
            ( (float *) dest_ptr)[2] = z;
            ( (float *) dest_ptr)[3] = rhw; /* SIC, see ddraw test! */

            dest_ptr += 3 * sizeof(float);

            if((DestFVF & WINED3DFVF_POSITION_MASK) == WINED3DFVF_XYZRHW) {
                dest_ptr += sizeof(float);
            }

            if(dest_conv) {
                float w = 1 / rhw;
                ( (float *) dest_conv)[0] = x * w;
                ( (float *) dest_conv)[1] = y * w;
                ( (float *) dest_conv)[2] = z * w;
                ( (float *) dest_conv)[3] = w;

                dest_conv += 3 * sizeof(float);

                if((DestFVF & WINED3DFVF_POSITION_MASK) == WINED3DFVF_XYZRHW) {
                    dest_conv += sizeof(float);
                }
            }
        }
        if (DestFVF & WINED3DFVF_PSIZE) {
            dest_ptr += sizeof(DWORD);
            if(dest_conv) dest_conv += sizeof(DWORD);
        }
        if (DestFVF & WINED3DFVF_NORMAL) {
            const struct wined3d_stream_info_element *element = &stream_info->elements[WINED3D_FFP_NORMAL];
            const float *normal = (const float *)(element->data + i * element->stride);
            /* AFAIK this should go into the lighting information */
            FIXME("Didn't expect the destination to have a normal\n");
            copy_and_next(dest_ptr, normal, 3 * sizeof(float));
            if(dest_conv) {
                copy_and_next(dest_conv, normal, 3 * sizeof(float));
            }
        }

        if (DestFVF & WINED3DFVF_DIFFUSE) {
            const struct wined3d_stream_info_element *element = &stream_info->elements[WINED3D_FFP_DIFFUSE];
            const DWORD *color_d = (const DWORD *)(element->data + i * element->stride);
            if(!color_d) {
                static BOOL warned = FALSE;

                if(!warned) {
                    ERR("No diffuse color in source, but destination has one\n");
                    warned = TRUE;
                }

                *( (DWORD *) dest_ptr) = 0xffffffff;
                dest_ptr += sizeof(DWORD);

                if(dest_conv) {
                    *( (DWORD *) dest_conv) = 0xffffffff;
                    dest_conv += sizeof(DWORD);
                }
            }
            else {
                copy_and_next(dest_ptr, color_d, sizeof(DWORD));
                if(dest_conv) {
                    *( (DWORD *) dest_conv)  = (*color_d & 0xff00ff00)      ; /* Alpha + green */
                    *( (DWORD *) dest_conv) |= (*color_d & 0x00ff0000) >> 16; /* Red */
                    *( (DWORD *) dest_conv) |= (*color_d & 0xff0000ff) << 16; /* Blue */
                    dest_conv += sizeof(DWORD);
                }
            }
        }

        if (DestFVF & WINED3DFVF_SPECULAR) { 
            /* What's the color value in the feedback buffer? */
            const struct wined3d_stream_info_element *element = &stream_info->elements[WINED3D_FFP_SPECULAR];
            const DWORD *color_s = (const DWORD *)(element->data + i * element->stride);
            if(!color_s) {
                static BOOL warned = FALSE;

                if(!warned) {
                    ERR("No specular color in source, but destination has one\n");
                    warned = TRUE;
                }

                *( (DWORD *) dest_ptr) = 0xFF000000;
                dest_ptr += sizeof(DWORD);

                if(dest_conv) {
                    *( (DWORD *) dest_conv) = 0xFF000000;
                    dest_conv += sizeof(DWORD);
                }
            }
            else {
                copy_and_next(dest_ptr, color_s, sizeof(DWORD));
                if(dest_conv) {
                    *( (DWORD *) dest_conv)  = (*color_s & 0xff00ff00)      ; /* Alpha + green */
                    *( (DWORD *) dest_conv) |= (*color_s & 0x00ff0000) >> 16; /* Red */
                    *( (DWORD *) dest_conv) |= (*color_s & 0xff0000ff) << 16; /* Blue */
                    dest_conv += sizeof(DWORD);
                }
            }
        }

        for (tex_index = 0; tex_index < numTextures; tex_index++) {
            const struct wined3d_stream_info_element *element = &stream_info->elements[WINED3D_FFP_TEXCOORD0 + tex_index];
            const float *tex_coord = (const float *)(element->data + i * element->stride);
            if(!tex_coord) {
                ERR("No source texture, but destination requests one\n");
                dest_ptr+=GET_TEXCOORD_SIZE_FROM_FVF(DestFVF, tex_index) * sizeof(float);
                if(dest_conv) dest_conv += GET_TEXCOORD_SIZE_FROM_FVF(DestFVF, tex_index) * sizeof(float);
            }
            else {
                copy_and_next(dest_ptr, tex_coord, GET_TEXCOORD_SIZE_FROM_FVF(DestFVF, tex_index) * sizeof(float));
                if(dest_conv) {
                    copy_and_next(dest_conv, tex_coord, GET_TEXCOORD_SIZE_FROM_FVF(DestFVF, tex_index) * sizeof(float));
                }
            }
        }
    }

    if(dest_conv) {
        GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, dest->buffer_object));
        checkGLcall("glBindBufferARB(GL_ARRAY_BUFFER_ARB)");
        GL_EXTCALL(glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, dwDestIndex * get_flexible_vertex_size(DestFVF),
                                      dwCount * get_flexible_vertex_size(DestFVF),
                                      dest_conv_addr));
        checkGLcall("glBufferSubDataARB(GL_ARRAY_BUFFER_ARB)");
        HeapFree(GetProcessHeap(), 0, dest_conv_addr);
    }

    LEAVE_GL();

    return WINED3D_OK;
}
#undef copy_and_next

static HRESULT WINAPI IWineD3DDeviceImpl_ProcessVertices(IWineD3DDevice *iface, UINT SrcStartIndex, UINT DestIndex,
        UINT VertexCount, IWineD3DBuffer *pDestBuffer, IWineD3DVertexDeclaration *pVertexDecl, DWORD Flags)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    struct wined3d_stream_info stream_info;
    BOOL vbo = FALSE, streamWasUP = This->stateBlock->streamIsUP;
    TRACE("(%p)->(%d,%d,%d,%p,%p,%d\n", This, SrcStartIndex, DestIndex, VertexCount, pDestBuffer, pVertexDecl, Flags);

    if(pVertexDecl) {
        ERR("Output vertex declaration not implemented yet\n");
    }

    /* Need any context to write to the vbo. */
    ActivateContext(This, This->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);

    /* ProcessVertices reads from vertex buffers, which have to be assigned. DrawPrimitive and DrawPrimitiveUP
     * control the streamIsUP flag, thus restore it afterwards.
     */
    This->stateBlock->streamIsUP = FALSE;
    device_stream_info_from_declaration(This, FALSE, &stream_info, &vbo);
    This->stateBlock->streamIsUP = streamWasUP;

    if(vbo || SrcStartIndex) {
        unsigned int i;
        /* ProcessVertices can't convert FROM a vbo, and vertex buffers used to source into ProcessVertices are
         * unlikely to ever be used for drawing. Release vbos in those buffers and fix up the stream_info structure
         *
         * Also get the start index in, but only loop over all elements if there's something to add at all.
         */
        for (i = 0; i < (sizeof(stream_info.elements) / sizeof(*stream_info.elements)); ++i)
        {
            struct wined3d_stream_info_element *e = &stream_info.elements[i];
            if (e->buffer_object)
            {
                struct wined3d_buffer *vb = (struct wined3d_buffer *)This->stateBlock->streamSource[e->stream_idx];
                e->buffer_object = 0;
                e->data = (BYTE *)((unsigned long)e->data + (unsigned long)vb->resource.allocatedMemory);
                ENTER_GL();
                GL_EXTCALL(glDeleteBuffersARB(1, &vb->buffer_object));
                vb->buffer_object = 0;
                LEAVE_GL();
            }
            if (e->data) e->data += e->stride * SrcStartIndex;
        }
    }

    return process_vertices_strided(This, DestIndex, VertexCount, &stream_info,
            (struct wined3d_buffer *)pDestBuffer, Flags);
}

/*****
 * Get / Set Texture Stage States
 * TODO: Verify against dx9 definitions
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_SetTextureStageState(IWineD3DDevice *iface, DWORD Stage, WINED3DTEXTURESTAGESTATETYPE Type, DWORD Value) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    DWORD oldValue = This->updateStateBlock->textureState[Stage][Type];

    TRACE("(%p) : Stage=%d, Type=%s(%d), Value=%d\n", This, Stage, debug_d3dtexturestate(Type), Type, Value);

    if (Stage >= MAX_TEXTURES) {
        WARN("Attempting to set stage %u which is higher than the max stage %u, ignoring\n", Stage, MAX_TEXTURES - 1);
        return WINED3D_OK;
    }

    This->updateStateBlock->changed.textureState[Stage] |= 1 << Type;
    This->updateStateBlock->textureState[Stage][Type]         = Value;

    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return WINED3D_OK;
    }

    /* Checked after the assignments to allow proper stateblock recording */
    if(oldValue == Value) {
        TRACE("App is setting the old value over, nothing to do\n");
        return WINED3D_OK;
    }

    if(Stage > This->stateBlock->lowest_disabled_stage &&
       This->StateTable[STATE_TEXTURESTAGE(0, Type)].representative == STATE_TEXTURESTAGE(0, WINED3DTSS_COLOROP)) {
        /* Colorop change above lowest disabled stage? That won't change anything in the gl setup
         * Changes in other states are important on disabled stages too
         */
        return WINED3D_OK;
    }

    if(Type == WINED3DTSS_COLOROP) {
        unsigned int i;

        if(Value == WINED3DTOP_DISABLE && oldValue != WINED3DTOP_DISABLE) {
            /* Previously enabled stage disabled now. Make sure to dirtify all enabled stages above Stage,
             * they have to be disabled
             *
             * The current stage is dirtified below.
             */
            for(i = Stage + 1; i < This->stateBlock->lowest_disabled_stage; i++) {
                TRACE("Additionally dirtifying stage %u\n", i);
                IWineD3DDeviceImpl_MarkStateDirty(This, STATE_TEXTURESTAGE(i, WINED3DTSS_COLOROP));
            }
            This->stateBlock->lowest_disabled_stage = Stage;
            TRACE("New lowest disabled: %u\n", Stage);
        } else if(Value != WINED3DTOP_DISABLE && oldValue == WINED3DTOP_DISABLE) {
            /* Previously disabled stage enabled. Stages above it may need enabling
             * stage must be lowest_disabled_stage here, if it's bigger success is returned above,
             * and stages below the lowest disabled stage can't be enabled(because they are enabled already).
             *
             * Again stage Stage doesn't need to be dirtified here, it is handled below.
             */

            for(i = Stage + 1; i < GL_LIMITS(texture_stages); i++) {
                if(This->updateStateBlock->textureState[i][WINED3DTSS_COLOROP] == WINED3DTOP_DISABLE) {
                    break;
                }
                TRACE("Additionally dirtifying stage %u due to enable\n", i);
                IWineD3DDeviceImpl_MarkStateDirty(This, STATE_TEXTURESTAGE(i, WINED3DTSS_COLOROP));
            }
            This->stateBlock->lowest_disabled_stage = i;
            TRACE("New lowest disabled: %u\n", i);
        }
    }

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_TEXTURESTAGE(Stage, Type));

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetTextureStageState(IWineD3DDevice *iface, DWORD Stage, WINED3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : requesting Stage %d, Type %d getting %d\n", This, Stage, Type, This->updateStateBlock->textureState[Stage][Type]);
    *pValue = This->updateStateBlock->textureState[Stage][Type];
    return WINED3D_OK;
}

/*****
 * Get / Set Texture
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_SetTexture(IWineD3DDevice *iface, DWORD Stage, IWineD3DBaseTexture* pTexture) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DBaseTexture *oldTexture;

    TRACE("(%p) : Stage %#x, Texture %p\n", This, Stage, pTexture);

    if (Stage >= WINED3DVERTEXTEXTURESAMPLER0 && Stage <= WINED3DVERTEXTEXTURESAMPLER3) {
        Stage -= (WINED3DVERTEXTEXTURESAMPLER0 - MAX_FRAGMENT_SAMPLERS);
    }

    if (Stage >= sizeof(This->stateBlock->textures)/sizeof(This->stateBlock->textures[0])) {
        ERR("Current stage overflows textures array (stage %d)\n", Stage);
        return WINED3D_OK; /* Windows accepts overflowing this array ... we do not. */
    }

    oldTexture = This->updateStateBlock->textures[Stage];

    /* SetTexture isn't allowed on textures in WINED3DPOOL_SCRATCH */
    if (pTexture && ((IWineD3DTextureImpl*)pTexture)->resource.pool == WINED3DPOOL_SCRATCH)
    {
        WARN("(%p) Attempt to set scratch texture rejected\n", pTexture);
        return WINED3DERR_INVALIDCALL;
    }

    TRACE("GL_LIMITS %d\n",GL_LIMITS(sampler_stages));
    TRACE("(%p) : oldtexture(%p)\n", This,oldTexture);

    This->updateStateBlock->changed.textures |= 1 << Stage;
    TRACE("(%p) : setting new texture to %p\n", This, pTexture);
    This->updateStateBlock->textures[Stage]         = pTexture;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return WINED3D_OK;
    }

    if(oldTexture == pTexture) {
        TRACE("App is setting the same texture again, nothing to do\n");
        return WINED3D_OK;
    }

    /** NOTE: MSDN says that setTexture increases the reference count,
    * and that the application must set the texture back to null (or have a leaky application),
    * This means we should pass the refcount up to the parent
     *******************************/
    if (NULL != This->updateStateBlock->textures[Stage]) {
        IWineD3DBaseTextureImpl *new = (IWineD3DBaseTextureImpl *) This->updateStateBlock->textures[Stage];
        ULONG bindCount = InterlockedIncrement(&new->baseTexture.bindCount);
        UINT dimensions = IWineD3DBaseTexture_GetTextureDimensions(pTexture);

        IWineD3DBaseTexture_AddRef(This->updateStateBlock->textures[Stage]);

        if (!oldTexture || dimensions != IWineD3DBaseTexture_GetTextureDimensions(oldTexture))
        {
            IWineD3DDeviceImpl_MarkStateDirty(This, STATE_PIXELSHADER);
        }

        if(oldTexture == NULL && Stage < MAX_TEXTURES) {
            /* The source arguments for color and alpha ops have different meanings when a NULL texture is bound,
             * so the COLOROP and ALPHAOP have to be dirtified.
             */
            IWineD3DDeviceImpl_MarkStateDirty(This, STATE_TEXTURESTAGE(Stage, WINED3DTSS_COLOROP));
            IWineD3DDeviceImpl_MarkStateDirty(This, STATE_TEXTURESTAGE(Stage, WINED3DTSS_ALPHAOP));
        }
        if(bindCount == 1) {
            new->baseTexture.sampler = Stage;
        }
        /* More than one assignment? Doesn't matter, we only need one gl texture unit to use for uploading */

    }

    if (NULL != oldTexture) {
        IWineD3DBaseTextureImpl *old = (IWineD3DBaseTextureImpl *) oldTexture;
        LONG bindCount = InterlockedDecrement(&old->baseTexture.bindCount);

        IWineD3DBaseTexture_Release(oldTexture);
        if(pTexture == NULL && Stage < MAX_TEXTURES) {
            IWineD3DDeviceImpl_MarkStateDirty(This, STATE_TEXTURESTAGE(Stage, WINED3DTSS_COLOROP));
            IWineD3DDeviceImpl_MarkStateDirty(This, STATE_TEXTURESTAGE(Stage, WINED3DTSS_ALPHAOP));
        }

        if(bindCount && old->baseTexture.sampler == Stage) {
            int i;
            /* Have to do a search for the other sampler(s) where the texture is bound to
             * Shouldn't happen as long as apps bind a texture only to one stage
             */
            TRACE("Searcing for other sampler / stage id where the texture is bound to\n");
            for(i = 0; i < MAX_COMBINED_SAMPLERS; i++) {
                if(This->updateStateBlock->textures[i] == oldTexture) {
                    old->baseTexture.sampler = i;
                    break;
                }
            }
        }
    }

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_SAMPLER(Stage));

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetTexture(IWineD3DDevice *iface, DWORD Stage, IWineD3DBaseTexture** ppTexture) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : Stage %#x, ppTexture %p\n", This, Stage, ppTexture);

    if (Stage >= WINED3DVERTEXTEXTURESAMPLER0 && Stage <= WINED3DVERTEXTEXTURESAMPLER3) {
        Stage -= (WINED3DVERTEXTEXTURESAMPLER0 - MAX_FRAGMENT_SAMPLERS);
    }

    if (Stage >= sizeof(This->stateBlock->textures)/sizeof(This->stateBlock->textures[0])) {
        ERR("Current stage overflows textures array (stage %d)\n", Stage);
        return WINED3D_OK; /* Windows accepts overflowing this array ... we do not. */
    }

    *ppTexture=This->stateBlock->textures[Stage];
    if (*ppTexture)
        IWineD3DBaseTexture_AddRef(*ppTexture);

    TRACE("(%p) : Returning %p\n", This, *ppTexture);

    return WINED3D_OK;
}

/*****
 * Get Back Buffer
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_GetBackBuffer(IWineD3DDevice *iface, UINT iSwapChain, UINT BackBuffer, WINED3DBACKBUFFER_TYPE Type,
                                                IWineD3DSurface **ppBackBuffer) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSwapChain *swapChain;
    HRESULT hr;

    TRACE("(%p) : BackBuf %d Type %d SwapChain %d returning %p\n", This, BackBuffer, Type, iSwapChain, *ppBackBuffer);

    hr = IWineD3DDeviceImpl_GetSwapChain(iface,  iSwapChain, &swapChain);
    if (hr == WINED3D_OK) {
        hr = IWineD3DSwapChain_GetBackBuffer(swapChain, BackBuffer, Type, ppBackBuffer);
            IWineD3DSwapChain_Release(swapChain);
    } else {
        *ppBackBuffer = NULL;
    }
    return hr;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetDeviceCaps(IWineD3DDevice *iface, WINED3DCAPS* pCaps) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    WARN("(%p) : stub, calling idirect3d for now\n", This);
    return IWineD3D_GetDeviceCaps(This->wineD3D, This->adapterNo, This->devType, pCaps);
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetDisplayMode(IWineD3DDevice *iface, UINT iSwapChain, WINED3DDISPLAYMODE* pMode) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSwapChain *swapChain;
    HRESULT hr;

    if(iSwapChain > 0) {
        hr = IWineD3DDeviceImpl_GetSwapChain(iface, iSwapChain, &swapChain);
        if (hr == WINED3D_OK) {
            hr = IWineD3DSwapChain_GetDisplayMode(swapChain, pMode);
            IWineD3DSwapChain_Release(swapChain);
        } else {
            FIXME("(%p) Error getting display mode\n", This);
        }
    } else {
        /* Don't read the real display mode,
           but return the stored mode instead. X11 can't change the color
           depth, and some apps are pretty angry if they SetDisplayMode from
           24 to 16 bpp and find out that GetDisplayMode still returns 24 bpp

           Also don't relay to the swapchain because with ddraw it's possible
           that there isn't a swapchain at all */
        pMode->Width = This->ddraw_width;
        pMode->Height = This->ddraw_height;
        pMode->Format = This->ddraw_format;
        pMode->RefreshRate = 0;
        hr = WINED3D_OK;
    }

    return hr;
}

/*****
 * Stateblock related functions
 *****/

static HRESULT WINAPI IWineD3DDeviceImpl_BeginStateBlock(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DStateBlock *stateblock;
    HRESULT hr;

    TRACE("(%p)\n", This);

    if (This->isRecordingState) return WINED3DERR_INVALIDCALL;

    hr = IWineD3DDeviceImpl_CreateStateBlock(iface, WINED3DSBT_RECORDED, &stateblock, NULL);
    if (FAILED(hr)) return hr;

    IWineD3DStateBlock_Release((IWineD3DStateBlock*)This->updateStateBlock);
    This->updateStateBlock = (IWineD3DStateBlockImpl *)stateblock;
    This->isRecordingState = TRUE;

    TRACE("(%p) recording stateblock %p\n", This, stateblock);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_EndStateBlock(IWineD3DDevice *iface, IWineD3DStateBlock** ppStateBlock) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    unsigned int i, j;
    IWineD3DStateBlockImpl *object = This->updateStateBlock;

    if (!This->isRecordingState) {
        WARN("(%p) not recording! returning error\n", This);
        *ppStateBlock = NULL;
        return WINED3DERR_INVALIDCALL;
    }

    for (i = 0; i <= WINEHIGHEST_RENDER_STATE >> 5; ++i)
    {
        DWORD map = object->changed.renderState[i];
        for (j = 0; map; map >>= 1, ++j)
        {
            if (!(map & 1)) continue;

            object->contained_render_states[object->num_contained_render_states++] = (i << 5) | j;
        }
    }

    for (i = 0; i <= HIGHEST_TRANSFORMSTATE >> 5; ++i)
    {
        DWORD map = object->changed.transform[i];
        for (j = 0; map; map >>= 1, ++j)
        {
            if (!(map & 1)) continue;

            object->contained_transform_states[object->num_contained_transform_states++] = (i << 5) | j;
        }
    }
    for(i = 0; i < GL_LIMITS(vshader_constantsF); i++) {
        if(object->changed.vertexShaderConstantsF[i]) {
            object->contained_vs_consts_f[object->num_contained_vs_consts_f] = i;
            object->num_contained_vs_consts_f++;
        }
    }
    for(i = 0; i < MAX_CONST_I; i++) {
        if (object->changed.vertexShaderConstantsI & (1 << i))
        {
            object->contained_vs_consts_i[object->num_contained_vs_consts_i] = i;
            object->num_contained_vs_consts_i++;
        }
    }
    for(i = 0; i < MAX_CONST_B; i++) {
        if (object->changed.vertexShaderConstantsB & (1 << i))
        {
            object->contained_vs_consts_b[object->num_contained_vs_consts_b] = i;
            object->num_contained_vs_consts_b++;
        }
    }
    for (i = 0; i < GL_LIMITS(pshader_constantsF); ++i)
    {
        if (object->changed.pixelShaderConstantsF[i])
        {
            object->contained_ps_consts_f[object->num_contained_ps_consts_f] = i;
            ++object->num_contained_ps_consts_f;
        }
    }
    for(i = 0; i < MAX_CONST_I; i++) {
        if (object->changed.pixelShaderConstantsI & (1 << i))
        {
            object->contained_ps_consts_i[object->num_contained_ps_consts_i] = i;
            object->num_contained_ps_consts_i++;
        }
    }
    for(i = 0; i < MAX_CONST_B; i++) {
        if (object->changed.pixelShaderConstantsB & (1 << i))
        {
            object->contained_ps_consts_b[object->num_contained_ps_consts_b] = i;
            object->num_contained_ps_consts_b++;
        }
    }
    for(i = 0; i < MAX_TEXTURES; i++) {
        DWORD map = object->changed.textureState[i];

        for(j = 0; map; map >>= 1, ++j)
        {
            if (!(map & 1)) continue;

            object->contained_tss_states[object->num_contained_tss_states].stage = i;
            object->contained_tss_states[object->num_contained_tss_states].state = j;
            ++object->num_contained_tss_states;
        }
    }
    for(i = 0; i < MAX_COMBINED_SAMPLERS; i++){
        DWORD map = object->changed.samplerState[i];

        for (j = 0; map; map >>= 1, ++j)
        {
            if (!(map & 1)) continue;

            object->contained_sampler_states[object->num_contained_sampler_states].stage = i;
            object->contained_sampler_states[object->num_contained_sampler_states].state = j;
            ++object->num_contained_sampler_states;
        }
    }

    *ppStateBlock = (IWineD3DStateBlock*) object;
    This->isRecordingState = FALSE;
    This->updateStateBlock = This->stateBlock;
    IWineD3DStateBlock_AddRef((IWineD3DStateBlock*)This->updateStateBlock);
    /* IWineD3DStateBlock_AddRef(*ppStateBlock); don't need to do this, since we should really just release UpdateStateBlock first */
    TRACE("(%p) returning token (ptr to stateblock) of %p\n", This, *ppStateBlock);
    return WINED3D_OK;
}

/*****
 * Scene related functions
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_BeginScene(IWineD3DDevice *iface) {
    /* At the moment we have no need for any functionality at the beginning
       of a scene                                                          */
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p)\n", This);

    if(This->inScene) {
        TRACE("Already in Scene, returning WINED3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }
    This->inScene = TRUE;
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_EndScene(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p)\n", This);

    if(!This->inScene) {
        TRACE("Not in scene, returning WINED3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }

    ActivateContext(This, This->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
    /* We only have to do this if we need to read the, swapbuffers performs a flush for us */
    glFlush();
    /* No checkGLcall here to avoid locking the lock just for checking a call that hardly ever
     * fails
     */

    This->inScene = FALSE;
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_Present(IWineD3DDevice *iface,
                                          CONST RECT* pSourceRect, CONST RECT* pDestRect,
                                          HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSwapChain *swapChain = NULL;
    int i;
    int swapchains = IWineD3DDeviceImpl_GetNumberOfSwapChains(iface);

    TRACE("(%p) Presenting the frame\n", This);

    for(i = 0 ; i < swapchains ; i ++) {

        IWineD3DDeviceImpl_GetSwapChain(iface, i, &swapChain);
        TRACE("presentinng chain %d, %p\n", i, swapChain);
        IWineD3DSwapChain_Present(swapChain, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, 0);
        IWineD3DSwapChain_Release(swapChain);
    }

    return WINED3D_OK;
}

/* Not called from the VTable (internal subroutine) */
HRESULT IWineD3DDeviceImpl_ClearSurface(IWineD3DDeviceImpl *This,  IWineD3DSurfaceImpl *target, DWORD Count,
                                        CONST WINED3DRECT* pRects, DWORD Flags, WINED3DCOLOR Color,
                                        float Z, DWORD Stencil) {
    GLbitfield     glMask = 0;
    unsigned int   i;
    WINED3DRECT curRect;
    RECT vp_rect;
    const WINED3DVIEWPORT *vp = &This->stateBlock->viewport;
    UINT drawable_width, drawable_height;
    IWineD3DSurfaceImpl *depth_stencil = (IWineD3DSurfaceImpl *) This->stencilBufferTarget;
    IWineD3DSwapChainImpl *swapchain = NULL;

    /* When we're clearing parts of the drawable, make sure that the target surface is well up to date in the
     * drawable. After the clear we'll mark the drawable up to date, so we have to make sure that this is true
     * for the cleared parts, and the untouched parts.
     *
     * If we're clearing the whole target there is no need to copy it into the drawable, it will be overwritten
     * anyway. If we're not clearing the color buffer we don't have to copy either since we're not going to set
     * the drawable up to date. We have to check all settings that limit the clear area though. Do not bother
     * checking all this if the dest surface is in the drawable anyway.
     */
    if((Flags & WINED3DCLEAR_TARGET) && !(target->Flags & SFLAG_INDRAWABLE)) {
        while(1) {
            if(vp->X != 0 || vp->Y != 0 ||
               vp->Width < target->currentDesc.Width || vp->Height < target->currentDesc.Height) {
                IWineD3DSurface_LoadLocation((IWineD3DSurface *) target, SFLAG_INDRAWABLE, NULL);
                break;
            }
            if(This->stateBlock->renderState[WINED3DRS_SCISSORTESTENABLE] && (
               This->stateBlock->scissorRect.left > 0 || This->stateBlock->scissorRect.top > 0 ||
               This->stateBlock->scissorRect.right < target->currentDesc.Width ||
               This->stateBlock->scissorRect.bottom < target->currentDesc.Height)) {
                IWineD3DSurface_LoadLocation((IWineD3DSurface *) target, SFLAG_INDRAWABLE, NULL);
                break;
            }
            if(Count > 0 && pRects && (
               pRects[0].x1 > 0 || pRects[0].y1 > 0 ||
               pRects[0].x2 < target->currentDesc.Width ||
               pRects[0].y2 < target->currentDesc.Height)) {
                IWineD3DSurface_LoadLocation((IWineD3DSurface *) target, SFLAG_INDRAWABLE, NULL);
                break;
            }
            break;
        }
    }

    target->get_drawable_size(target, &drawable_width, &drawable_height);

    ActivateContext(This, (IWineD3DSurface *) target, CTXUSAGE_CLEAR);
    ENTER_GL();

    /* Only set the values up once, as they are not changing */
    if (Flags & WINED3DCLEAR_STENCIL) {
        glClearStencil(Stencil);
        checkGLcall("glClearStencil");
        glMask = glMask | GL_STENCIL_BUFFER_BIT;
        glStencilMask(0xFFFFFFFF);
    }

    if (Flags & WINED3DCLEAR_ZBUFFER) {
        DWORD location = This->render_offscreen ? SFLAG_DS_OFFSCREEN : SFLAG_DS_ONSCREEN;
        glDepthMask(GL_TRUE);
        glClearDepth(Z);
        checkGLcall("glClearDepth");
        glMask = glMask | GL_DEPTH_BUFFER_BIT;
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_ZWRITEENABLE));

        if (vp->X != 0 || vp->Y != 0 ||
                vp->Width < depth_stencil->currentDesc.Width || vp->Height < depth_stencil->currentDesc.Height) {
            surface_load_ds_location(This->stencilBufferTarget, location);
        }
        else if (This->stateBlock->renderState[WINED3DRS_SCISSORTESTENABLE] && (
                This->stateBlock->scissorRect.left > 0 || This->stateBlock->scissorRect.top > 0 ||
                This->stateBlock->scissorRect.right < depth_stencil->currentDesc.Width ||
                This->stateBlock->scissorRect.bottom < depth_stencil->currentDesc.Height)) {
            surface_load_ds_location(This->stencilBufferTarget, location);
        }
        else if (Count > 0 && pRects && (
                pRects[0].x1 > 0 || pRects[0].y1 > 0 ||
                pRects[0].x2 < depth_stencil->currentDesc.Width ||
                pRects[0].y2 < depth_stencil->currentDesc.Height)) {
            surface_load_ds_location(This->stencilBufferTarget, location);
        }
    }

    if (Flags & WINED3DCLEAR_TARGET) {
        TRACE("Clearing screen with glClear to color %x\n", Color);
        glClearColor(D3DCOLOR_R(Color),
                     D3DCOLOR_G(Color),
                     D3DCOLOR_B(Color),
                     D3DCOLOR_A(Color));
        checkGLcall("glClearColor");

        /* Clear ALL colors! */
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glMask = glMask | GL_COLOR_BUFFER_BIT;
    }

    vp_rect.left = vp->X;
    vp_rect.top = vp->Y;
    vp_rect.right = vp->X + vp->Width;
    vp_rect.bottom = vp->Y + vp->Height;
    if (!(Count > 0 && pRects)) {
        if(This->stateBlock->renderState[WINED3DRS_SCISSORTESTENABLE]) {
            IntersectRect(&vp_rect, &vp_rect, &This->stateBlock->scissorRect);
        }
        if(This->render_offscreen) {
            glScissor(vp_rect.left, vp_rect.top,
                        vp_rect.right - vp_rect.left, vp_rect.bottom - vp_rect.top);
        } else {
            glScissor(vp_rect.left, drawable_height - vp_rect.bottom,
                        vp_rect.right - vp_rect.left, vp_rect.bottom - vp_rect.top);
        }
        checkGLcall("glScissor");
        glClear(glMask);
        checkGLcall("glClear");
    } else {
        /* Now process each rect in turn */
        for (i = 0; i < Count; i++) {
            /* Note gl uses lower left, width/height */
            IntersectRect((RECT *)&curRect, &vp_rect, (const RECT *)&pRects[i]);
            if(This->stateBlock->renderState[WINED3DRS_SCISSORTESTENABLE]) {
                IntersectRect((RECT *) &curRect, (RECT *) &curRect, &This->stateBlock->scissorRect);
            }
            TRACE("(%p) Rect=(%d,%d)->(%d,%d) glRect=(%d,%d), len=%d, hei=%d\n", This,
                  pRects[i].x1, pRects[i].y1, pRects[i].x2, pRects[i].y2,
                  curRect.x1, (target->currentDesc.Height - curRect.y2),
                  curRect.x2 - curRect.x1, curRect.y2 - curRect.y1);

            /* Tests show that rectangles where x1 > x2 or y1 > y2 are ignored silently.
             * The rectangle is not cleared, no error is returned, but further rectanlges are
             * still cleared if they are valid
             */
            if(curRect.x1 > curRect.x2 || curRect.y1 > curRect.y2) {
                TRACE("Rectangle with negative dimensions, ignoring\n");
                continue;
            }

            if(This->render_offscreen) {
                glScissor(curRect.x1, curRect.y1,
                          curRect.x2 - curRect.x1, curRect.y2 - curRect.y1);
            } else {
                glScissor(curRect.x1, drawable_height - curRect.y2,
                          curRect.x2 - curRect.x1, curRect.y2 - curRect.y1);
            }
            checkGLcall("glScissor");

            glClear(glMask);
            checkGLcall("glClear");
        }
    }

    /* Restore the old values (why..?) */
    if (Flags & WINED3DCLEAR_STENCIL) {
        glStencilMask(This->stateBlock->renderState[WINED3DRS_STENCILWRITEMASK]);
    }
    if (Flags & WINED3DCLEAR_TARGET) {
        DWORD mask = This->stateBlock->renderState[WINED3DRS_COLORWRITEENABLE];
        glColorMask(mask & WINED3DCOLORWRITEENABLE_RED ? GL_TRUE : GL_FALSE,
                    mask & WINED3DCOLORWRITEENABLE_GREEN ? GL_TRUE : GL_FALSE,
                    mask & WINED3DCOLORWRITEENABLE_BLUE  ? GL_TRUE : GL_FALSE,
                    mask & WINED3DCOLORWRITEENABLE_ALPHA ? GL_TRUE : GL_FALSE);

        /* Dirtify the target surface for now. If the surface is locked regularly, and an up to date sysmem copy exists,
         * it is most likely more efficient to perform a clear on the sysmem copy too instead of downloading it
         */
        IWineD3DSurface_ModifyLocation(This->lastActiveRenderTarget, SFLAG_INDRAWABLE, TRUE);
    }
    if (Flags & WINED3DCLEAR_ZBUFFER) {
        /* Note that WINED3DCLEAR_ZBUFFER implies a depth stencil exists on the device */
        DWORD location = This->render_offscreen ? SFLAG_DS_OFFSCREEN : SFLAG_DS_ONSCREEN;
        surface_modify_ds_location(This->stencilBufferTarget, location);
    }

    LEAVE_GL();

    if (SUCCEEDED(IWineD3DSurface_GetContainer((IWineD3DSurface *)target, &IID_IWineD3DSwapChain, (void **)&swapchain))) {
        if (target == (IWineD3DSurfaceImpl*) swapchain->frontBuffer) {
            glFlush();
        }
        IWineD3DSwapChain_Release((IWineD3DSwapChain *) swapchain);
    }

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_Clear(IWineD3DDevice *iface, DWORD Count, CONST WINED3DRECT* pRects,
                                        DWORD Flags, WINED3DCOLOR Color, float Z, DWORD Stencil) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSurfaceImpl *target = (IWineD3DSurfaceImpl *)This->render_targets[0];

    TRACE("(%p) Count (%d), pRects (%p), Flags (%x), Color (0x%08x), Z (%f), Stencil (%d)\n", This,
          Count, pRects, Flags, Color, Z, Stencil);

    if(Flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL) && This->stencilBufferTarget == NULL) {
        WARN("Clearing depth and/or stencil without a depth stencil buffer attached, returning WINED3DERR_INVALIDCALL\n");
        /* TODO: What about depth stencil buffers without stencil bits? */
        return WINED3DERR_INVALIDCALL;
    }

    return IWineD3DDeviceImpl_ClearSurface(This, target, Count, pRects, Flags, Color, Z, Stencil);
}

/*****
 * Drawing functions
 *****/

static void WINAPI IWineD3DDeviceImpl_SetPrimitiveType(IWineD3DDevice *iface,
        WINED3DPRIMITIVETYPE primitive_type)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("iface %p, primitive_type %s\n", iface, debug_d3dprimitivetype(primitive_type));

    This->updateStateBlock->changed.primitive_type = TRUE;
    This->updateStateBlock->gl_primitive_type = gl_primitive_type_from_d3d(primitive_type);
}

static void WINAPI IWineD3DDeviceImpl_GetPrimitiveType(IWineD3DDevice *iface,
        WINED3DPRIMITIVETYPE *primitive_type)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("iface %p, primitive_type %p\n", iface, primitive_type);

    *primitive_type = d3d_primitive_type_from_gl(This->stateBlock->gl_primitive_type);

    TRACE("Returning %s\n", debug_d3dprimitivetype(*primitive_type));
}

static HRESULT WINAPI IWineD3DDeviceImpl_DrawPrimitive(IWineD3DDevice *iface, UINT StartVertex, UINT vertex_count)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : start %u, count %u\n", This, StartVertex, vertex_count);

    if(!This->stateBlock->vertexDecl) {
        WARN("(%p) : Called without a valid vertex declaration set\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    /* The index buffer is not needed here, but restore it, otherwise it is hell to keep track of */
    if(This->stateBlock->streamIsUP) {
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_INDEXBUFFER);
        This->stateBlock->streamIsUP = FALSE;
    }

    if(This->stateBlock->loadBaseVertexIndex != 0) {
        This->stateBlock->loadBaseVertexIndex = 0;
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_STREAMSRC);
    }
    /* Account for the loading offset due to index buffers. Instead of reloading all sources correct it with the startvertex parameter */
    drawPrimitive(iface, vertex_count, 0/* NumVertices */, StartVertex /* start_idx */,
                  0 /* indxSize */, NULL /* indxData */, 0 /* minIndex */);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_DrawIndexedPrimitive(IWineD3DDevice *iface,
        UINT minIndex, UINT NumVertices, UINT startIndex, UINT index_count)
{
    IWineD3DDeviceImpl  *This = (IWineD3DDeviceImpl *)iface;
    UINT                 idxStride = 2;
    IWineD3DIndexBuffer *pIB;
    WINED3DINDEXBUFFER_DESC  IdxBufDsc;
    GLuint vbo;

    pIB = This->stateBlock->pIndexData;
    if (!pIB) {
        /* D3D9 returns D3DERR_INVALIDCALL when DrawIndexedPrimitive is called
         * without an index buffer set. (The first time at least...)
         * D3D8 simply dies, but I doubt it can do much harm to return
         * D3DERR_INVALIDCALL there as well. */
        WARN("(%p) : Called without a valid index buffer set, returning WINED3DERR_INVALIDCALL\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    if(!This->stateBlock->vertexDecl) {
        WARN("(%p) : Called without a valid vertex declaration set\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    if(This->stateBlock->streamIsUP) {
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_INDEXBUFFER);
        This->stateBlock->streamIsUP = FALSE;
    }
    vbo = ((IWineD3DIndexBufferImpl *) pIB)->vbo;

    TRACE("(%p) : min %u, vertex count %u, startIdx %u, index count %u\n",
            This, minIndex, NumVertices, startIndex, index_count);

    IWineD3DIndexBuffer_GetDesc(pIB, &IdxBufDsc);
    if (IdxBufDsc.Format == WINED3DFMT_R16_UINT) {
        idxStride = 2;
    } else {
        idxStride = 4;
    }

    if(This->stateBlock->loadBaseVertexIndex != This->stateBlock->baseVertexIndex) {
        This->stateBlock->loadBaseVertexIndex = This->stateBlock->baseVertexIndex;
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_STREAMSRC);
    }

    drawPrimitive(iface, index_count, NumVertices, startIndex, idxStride,
            vbo ? NULL : ((IWineD3DIndexBufferImpl *) pIB)->resource.allocatedMemory, minIndex);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_DrawPrimitiveUP(IWineD3DDevice *iface, UINT vertex_count,
        const void *pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DBuffer *vb;

    TRACE("(%p) : vertex count %u, pVtxData %p, stride %u\n",
            This, vertex_count, pVertexStreamZeroData, VertexStreamZeroStride);

    if(!This->stateBlock->vertexDecl) {
        WARN("(%p) : Called without a valid vertex declaration set\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    /* Note in the following, it's not this type, but that's the purpose of streamIsUP */
    vb = This->stateBlock->streamSource[0];
    This->stateBlock->streamSource[0] = (IWineD3DBuffer *)pVertexStreamZeroData;
    if (vb) IWineD3DBuffer_Release(vb);
    This->stateBlock->streamOffset[0] = 0;
    This->stateBlock->streamStride[0] = VertexStreamZeroStride;
    This->stateBlock->streamIsUP = TRUE;
    This->stateBlock->loadBaseVertexIndex = 0;

    /* TODO: Only mark dirty if drawing from a different UP address */
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_STREAMSRC);

    drawPrimitive(iface, vertex_count, 0 /* NumVertices */, 0 /* start_idx */,
            0 /* indxSize*/, NULL /* indxData */, 0 /* indxMin */);

    /* MSDN specifies stream zero settings must be set to NULL */
    This->stateBlock->streamStride[0] = 0;
    This->stateBlock->streamSource[0] = NULL;

    /* stream zero settings set to null at end, as per the msdn. No need to mark dirty here, the app has to set
     * the new stream sources or use UP drawing again
     */
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_DrawIndexedPrimitiveUP(IWineD3DDevice *iface, UINT MinVertexIndex,
        UINT NumVertices, UINT index_count, const void *pIndexData, WINED3DFORMAT IndexDataFormat,
        const void *pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    int                 idxStride;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DBuffer *vb;
    IWineD3DIndexBuffer *ib;

    TRACE("(%p) : MinVtxIdx %u, NumVIdx %u, index count %u, pidxdata %p, IdxFmt %u, pVtxdata %p, stride=%u\n",
            This, MinVertexIndex, NumVertices, index_count, pIndexData,
            IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);

    if(!This->stateBlock->vertexDecl) {
        WARN("(%p) : Called without a valid vertex declaration set\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    if (IndexDataFormat == WINED3DFMT_R16_UINT) {
        idxStride = 2;
    } else {
        idxStride = 4;
    }

    /* Note in the following, it's not this type, but that's the purpose of streamIsUP */
    vb = This->stateBlock->streamSource[0];
    This->stateBlock->streamSource[0] = (IWineD3DBuffer *)pVertexStreamZeroData;
    if (vb) IWineD3DBuffer_Release(vb);
    This->stateBlock->streamIsUP = TRUE;
    This->stateBlock->streamOffset[0] = 0;
    This->stateBlock->streamStride[0] = VertexStreamZeroStride;

    /* Set to 0 as per msdn. Do it now due to the stream source loading during drawPrimitive */
    This->stateBlock->baseVertexIndex = 0;
    This->stateBlock->loadBaseVertexIndex = 0;
    /* Mark the state dirty until we have nicer tracking of the stream source pointers */
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_VDECL);
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_INDEXBUFFER);

    drawPrimitive(iface, index_count, NumVertices, 0 /* start_idx */,
            idxStride, pIndexData, MinVertexIndex);

    /* MSDN specifies stream zero settings and index buffer must be set to NULL */
    This->stateBlock->streamSource[0] = NULL;
    This->stateBlock->streamStride[0] = 0;
    ib = This->stateBlock->pIndexData;
    if(ib) {
        IWineD3DIndexBuffer_Release(ib);
        This->stateBlock->pIndexData = NULL;
    }
    /* No need to mark the stream source state dirty here. Either the app calls UP drawing again, or it has to call
     * SetStreamSource to specify a vertex buffer
     */

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_DrawPrimitiveStrided(IWineD3DDevice *iface,
        UINT vertex_count, const WineDirect3DVertexStridedData *DrawPrimStrideData)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;

    /* Mark the state dirty until we have nicer tracking
     * its fine to change baseVertexIndex because that call is only called by ddraw which does not need
     * that value.
     */
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_VDECL);
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_INDEXBUFFER);
    This->stateBlock->baseVertexIndex = 0;
    This->up_strided = DrawPrimStrideData;
    drawPrimitive(iface, vertex_count, 0, 0, 0, NULL, 0);
    This->up_strided = NULL;
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_DrawIndexedPrimitiveStrided(IWineD3DDevice *iface,
        UINT vertex_count, const WineDirect3DVertexStridedData *DrawPrimStrideData,
        UINT NumVertices, const void *pIndexData, WINED3DFORMAT IndexDataFormat)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    DWORD idxSize = (IndexDataFormat == WINED3DFMT_R32_UINT ? 4 : 2);

    /* Mark the state dirty until we have nicer tracking
     * its fine to change baseVertexIndex because that call is only called by ddraw which does not need
     * that value.
     */
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_VDECL);
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_INDEXBUFFER);
    This->stateBlock->streamIsUP = TRUE;
    This->stateBlock->baseVertexIndex = 0;
    This->up_strided = DrawPrimStrideData;
    drawPrimitive(iface, vertex_count, 0 /* numindices */, 0 /* start_idx */, idxSize, pIndexData, 0 /* minindex */);
    This->up_strided = NULL;
    return WINED3D_OK;
}

static HRESULT IWineD3DDeviceImpl_UpdateVolume(IWineD3DDevice *iface, IWineD3DVolume *pSourceVolume, IWineD3DVolume *pDestinationVolume) {
    /* This is a helper function for UpdateTexture, there is no public UpdateVolume method in d3d. Since it's
     * not callable by the app directly no parameter validation checks are needed here.
     */
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    WINED3DLOCKED_BOX src;
    WINED3DLOCKED_BOX dst;
    HRESULT hr;
    TRACE("(%p)->(%p, %p)\n", This, pSourceVolume, pDestinationVolume);

    /* TODO: Implement direct loading into the gl volume instead of using memcpy and
     * dirtification to improve loading performance.
     */
    hr = IWineD3DVolume_LockBox(pSourceVolume, &src, NULL, WINED3DLOCK_READONLY);
    if(FAILED(hr)) return hr;
    hr = IWineD3DVolume_LockBox(pDestinationVolume, &dst, NULL, WINED3DLOCK_DISCARD);
    if(FAILED(hr)) {
    IWineD3DVolume_UnlockBox(pSourceVolume);
            return hr;
    }

    memcpy(dst.pBits, src.pBits, ((IWineD3DVolumeImpl *) pDestinationVolume)->resource.size);

    hr = IWineD3DVolume_UnlockBox(pDestinationVolume);
    if(FAILED(hr)) {
        IWineD3DVolume_UnlockBox(pSourceVolume);
    } else {
        hr = IWineD3DVolume_UnlockBox(pSourceVolume);
    }
    return hr;
}

/* Yet another way to update a texture, some apps use this to load default textures instead of using surface/texture lock/unlock */
static HRESULT WINAPI IWineD3DDeviceImpl_UpdateTexture (IWineD3DDevice *iface, IWineD3DBaseTexture *pSourceTexture,  IWineD3DBaseTexture *pDestinationTexture){
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    HRESULT hr = WINED3D_OK;
    WINED3DRESOURCETYPE sourceType;
    WINED3DRESOURCETYPE destinationType;
    int i ,levels;

    /* TODO: think about moving the code into IWineD3DBaseTexture  */

    TRACE("(%p) Source %p Destination %p\n", This, pSourceTexture, pDestinationTexture);

    /* verify that the source and destination textures aren't NULL */
    if (NULL == pSourceTexture || NULL == pDestinationTexture) {
        WARN("(%p) : source (%p) and destination (%p) textures must not be NULL, returning WINED3DERR_INVALIDCALL\n",
             This, pSourceTexture, pDestinationTexture);
        hr = WINED3DERR_INVALIDCALL;
    }

    if (pSourceTexture == pDestinationTexture) {
        WARN("(%p) : source (%p) and destination (%p) textures must be different, returning WINED3DERR_INVALIDCALL\n",
             This, pSourceTexture, pDestinationTexture);
        hr = WINED3DERR_INVALIDCALL;
    }
    /* Verify that the source and destination textures are the same type */
    sourceType      = IWineD3DBaseTexture_GetType(pSourceTexture);
    destinationType = IWineD3DBaseTexture_GetType(pDestinationTexture);

    if (sourceType != destinationType) {
        WARN("(%p) Sorce and destination types must match, returning WINED3DERR_INVALIDCALL\n",
             This);
        hr = WINED3DERR_INVALIDCALL;
    }

    /* check that both textures have the identical numbers of levels  */
    if (IWineD3DBaseTexture_GetLevelCount(pDestinationTexture)  != IWineD3DBaseTexture_GetLevelCount(pSourceTexture)) {
        WARN("(%p) : source (%p) and destination (%p) textures must have identical numbers of levels, returning WINED3DERR_INVALIDCALL\n", This, pSourceTexture, pDestinationTexture);
        hr = WINED3DERR_INVALIDCALL;
    }

    if (WINED3D_OK == hr) {
        IWineD3DBaseTextureImpl *pDestImpl = (IWineD3DBaseTextureImpl *) pDestinationTexture;

        /* Make sure that the destination texture is loaded */
        pDestImpl->baseTexture.internal_preload(pDestinationTexture, SRGB_RGB);

        /* Update every surface level of the texture */
        levels = IWineD3DBaseTexture_GetLevelCount(pDestinationTexture);

        switch (sourceType) {
        case WINED3DRTYPE_TEXTURE:
            {
                IWineD3DSurface *srcSurface;
                IWineD3DSurface *destSurface;

                for (i = 0 ; i < levels ; ++i) {
                    IWineD3DTexture_GetSurfaceLevel((IWineD3DTexture *)pSourceTexture,      i, &srcSurface);
                    IWineD3DTexture_GetSurfaceLevel((IWineD3DTexture *)pDestinationTexture, i, &destSurface);
                    hr = IWineD3DDevice_UpdateSurface(iface, srcSurface, NULL, destSurface, NULL);
                    IWineD3DSurface_Release(srcSurface);
                    IWineD3DSurface_Release(destSurface);
                    if (WINED3D_OK != hr) {
                        WARN("(%p) : Call to update surface failed\n", This);
                        return hr;
                    }
                }
            }
            break;
        case WINED3DRTYPE_CUBETEXTURE:
            {
                IWineD3DSurface *srcSurface;
                IWineD3DSurface *destSurface;
                WINED3DCUBEMAP_FACES faceType;

                for (i = 0 ; i < levels ; ++i) {
                    /* Update each cube face */
                    for (faceType = WINED3DCUBEMAP_FACE_POSITIVE_X; faceType <= WINED3DCUBEMAP_FACE_NEGATIVE_Z; ++faceType){
                        hr = IWineD3DCubeTexture_GetCubeMapSurface((IWineD3DCubeTexture *)pSourceTexture,      faceType, i, &srcSurface);
                        if (WINED3D_OK != hr) {
                            FIXME("(%p) : Failed to get src cube surface facetype %d, level %d\n", This, faceType, i);
                        } else {
                            TRACE("Got srcSurface %p\n", srcSurface);
                        }
                        hr = IWineD3DCubeTexture_GetCubeMapSurface((IWineD3DCubeTexture *)pDestinationTexture, faceType, i, &destSurface);
                        if (WINED3D_OK != hr) {
                            FIXME("(%p) : Failed to get src cube surface facetype %d, level %d\n", This, faceType, i);
                        } else {
                            TRACE("Got desrSurface %p\n", destSurface);
                        }
                        hr = IWineD3DDevice_UpdateSurface(iface, srcSurface, NULL, destSurface, NULL);
                        IWineD3DSurface_Release(srcSurface);
                        IWineD3DSurface_Release(destSurface);
                        if (WINED3D_OK != hr) {
                            WARN("(%p) : Call to update surface failed\n", This);
                            return hr;
                        }
                    }
                }
            }
            break;

        case WINED3DRTYPE_VOLUMETEXTURE:
            {
                IWineD3DVolume  *srcVolume  = NULL;
                IWineD3DVolume  *destVolume = NULL;

                for (i = 0 ; i < levels ; ++i) {
                    IWineD3DVolumeTexture_GetVolumeLevel((IWineD3DVolumeTexture *)pSourceTexture,      i, &srcVolume);
                    IWineD3DVolumeTexture_GetVolumeLevel((IWineD3DVolumeTexture *)pDestinationTexture, i, &destVolume);
                    hr =  IWineD3DDeviceImpl_UpdateVolume(iface, srcVolume, destVolume);
                    IWineD3DVolume_Release(srcVolume);
                    IWineD3DVolume_Release(destVolume);
                    if (WINED3D_OK != hr) {
                        WARN("(%p) : Call to update volume failed\n", This);
                        return hr;
                    }
                }
            }
            break;

        default:
            FIXME("(%p) : Unsupported source and destination type\n", This);
            hr = WINED3DERR_INVALIDCALL;
        }
    }

    return hr;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_GetFrontBufferData(IWineD3DDevice *iface,UINT iSwapChain, IWineD3DSurface *pDestSurface) {
    IWineD3DSwapChain *swapChain;
    HRESULT hr;
    hr = IWineD3DDeviceImpl_GetSwapChain(iface, iSwapChain, &swapChain);
    if(hr == WINED3D_OK) {
        hr = IWineD3DSwapChain_GetFrontBufferData(swapChain, pDestSurface);
                IWineD3DSwapChain_Release(swapChain);
    }
    return hr;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_ValidateDevice(IWineD3DDevice *iface, DWORD* pNumPasses) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DBaseTextureImpl *texture;
    DWORD i;

    TRACE("(%p) : %p\n", This, pNumPasses);

    for(i = 0; i < MAX_COMBINED_SAMPLERS; i++) {
        if(This->stateBlock->samplerState[i][WINED3DSAMP_MINFILTER] == WINED3DTEXF_NONE) {
            WARN("Sampler state %u has minfilter D3DTEXF_NONE, returning D3DERR_UNSUPPORTEDTEXTUREFILTER\n", i);
            return WINED3DERR_UNSUPPORTEDTEXTUREFILTER;
        }
        if(This->stateBlock->samplerState[i][WINED3DSAMP_MAGFILTER] == WINED3DTEXF_NONE) {
            WARN("Sampler state %u has magfilter D3DTEXF_NONE, returning D3DERR_UNSUPPORTEDTEXTUREFILTER\n", i);
            return WINED3DERR_UNSUPPORTEDTEXTUREFILTER;
        }

        texture = (IWineD3DBaseTextureImpl *) This->stateBlock->textures[i];
        if (!texture || texture->resource.format_desc->Flags & WINED3DFMT_FLAG_FILTERING) continue;

        if(This->stateBlock->samplerState[i][WINED3DSAMP_MAGFILTER] != WINED3DTEXF_POINT) {
            WARN("Non-filterable texture and mag filter enabled on samper %u, returning E_FAIL\n", i);
            return E_FAIL;
        }
        if(This->stateBlock->samplerState[i][WINED3DSAMP_MINFILTER] != WINED3DTEXF_POINT) {
            WARN("Non-filterable texture and min filter enabled on samper %u, returning E_FAIL\n", i);
            return E_FAIL;
        }
        if(This->stateBlock->samplerState[i][WINED3DSAMP_MIPFILTER] != WINED3DTEXF_NONE &&
           This->stateBlock->samplerState[i][WINED3DSAMP_MIPFILTER] != WINED3DTEXF_POINT /* sic! */) {
            WARN("Non-filterable texture and mip filter enabled on samper %u, returning E_FAIL\n", i);
            return E_FAIL;
        }
    }

    /* return a sensible default */
    *pNumPasses = 1;

    TRACE("returning D3D_OK\n");
    return WINED3D_OK;
}

static void dirtify_p8_texture_samplers(IWineD3DDeviceImpl *device)
{
    int i;

    for (i = 0; i < MAX_COMBINED_SAMPLERS; i++) {
            IWineD3DBaseTextureImpl *texture = (IWineD3DBaseTextureImpl*)device->stateBlock->textures[i];
            if (texture && (texture->resource.format_desc->format == WINED3DFMT_P8
                    || texture->resource.format_desc->format == WINED3DFMT_A8P8))
            {
                IWineD3DDeviceImpl_MarkStateDirty(device, STATE_SAMPLER(i));
            }
        }
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_SetPaletteEntries(IWineD3DDevice *iface, UINT PaletteNumber, CONST PALETTEENTRY* pEntries) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int j;
    UINT NewSize;
    PALETTEENTRY **palettes;

    TRACE("(%p) : PaletteNumber %u\n", This, PaletteNumber);

    if (PaletteNumber >= MAX_PALETTES) {
        ERR("(%p) : (%u) Out of range 0-%u, returning Invalid Call\n", This, PaletteNumber, MAX_PALETTES);
        return WINED3DERR_INVALIDCALL;
    }

    if (PaletteNumber >= This->NumberOfPalettes) {
        NewSize = This->NumberOfPalettes;
        do {
           NewSize *= 2;
        } while(PaletteNumber >= NewSize);
        palettes = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, This->palettes, sizeof(PALETTEENTRY*) * NewSize);
        if (!palettes) {
            ERR("Out of memory!\n");
            return E_OUTOFMEMORY;
        }
        This->palettes = palettes;
        This->NumberOfPalettes = NewSize;
    }

    if (!This->palettes[PaletteNumber]) {
        This->palettes[PaletteNumber] = HeapAlloc(GetProcessHeap(),  0, sizeof(PALETTEENTRY) * 256);
        if (!This->palettes[PaletteNumber]) {
            ERR("Out of memory!\n");
            return E_OUTOFMEMORY;
        }
    }

    for (j = 0; j < 256; ++j) {
        This->palettes[PaletteNumber][j].peRed   = pEntries[j].peRed;
        This->palettes[PaletteNumber][j].peGreen = pEntries[j].peGreen;
        This->palettes[PaletteNumber][j].peBlue  = pEntries[j].peBlue;
        This->palettes[PaletteNumber][j].peFlags = pEntries[j].peFlags;
    }
    if (PaletteNumber == This->currentPalette) dirtify_p8_texture_samplers(This);
    TRACE("(%p) : returning\n", This);
    return WINED3D_OK;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_GetPaletteEntries(IWineD3DDevice *iface, UINT PaletteNumber, PALETTEENTRY* pEntries) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int j;
    TRACE("(%p) : PaletteNumber %u\n", This, PaletteNumber);
    if (PaletteNumber >= This->NumberOfPalettes || !This->palettes[PaletteNumber]) {
        /* What happens in such situation isn't documented; Native seems to silently abort
           on such conditions. Return Invalid Call. */
        ERR("(%p) : (%u) Nonexistent palette. NumberOfPalettes %u\n", This, PaletteNumber, This->NumberOfPalettes);
        return WINED3DERR_INVALIDCALL;
    }
    for (j = 0; j < 256; ++j) {
        pEntries[j].peRed   = This->palettes[PaletteNumber][j].peRed;
        pEntries[j].peGreen = This->palettes[PaletteNumber][j].peGreen;
        pEntries[j].peBlue  = This->palettes[PaletteNumber][j].peBlue;
        pEntries[j].peFlags = This->palettes[PaletteNumber][j].peFlags;
    }
    TRACE("(%p) : returning\n", This);
    return WINED3D_OK;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_SetCurrentTexturePalette(IWineD3DDevice *iface, UINT PaletteNumber) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : PaletteNumber %u\n", This, PaletteNumber);
    /* Native appears to silently abort on attempt to make an uninitialized palette current and render.
       (tested with reference rasterizer). Return Invalid Call. */
    if (PaletteNumber >= This->NumberOfPalettes || !This->palettes[PaletteNumber]) {
        ERR("(%p) : (%u) Nonexistent palette. NumberOfPalettes %u\n", This, PaletteNumber, This->NumberOfPalettes);
        return WINED3DERR_INVALIDCALL;
    }
    /*TODO: stateblocks */
    if (This->currentPalette != PaletteNumber) {
        This->currentPalette = PaletteNumber;
        dirtify_p8_texture_samplers(This);
    }
    TRACE("(%p) : returning\n", This);
    return WINED3D_OK;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_GetCurrentTexturePalette(IWineD3DDevice *iface, UINT* PaletteNumber) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    if (PaletteNumber == NULL) {
        WARN("(%p) : returning Invalid Call\n", This);
        return WINED3DERR_INVALIDCALL;
    }
    /*TODO: stateblocks */
    *PaletteNumber = This->currentPalette;
    TRACE("(%p) : returning  %u\n", This, *PaletteNumber);
    return WINED3D_OK;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_SetSoftwareVertexProcessing(IWineD3DDevice *iface, BOOL bSoftware) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    static BOOL warned;
    if (!warned)
    {
        FIXME("(%p) : stub\n", This);
        warned = TRUE;
    }

    This->softwareVertexProcessing = bSoftware;
    return WINED3D_OK;
}


static BOOL     WINAPI  IWineD3DDeviceImpl_GetSoftwareVertexProcessing(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    static BOOL warned;
    if (!warned)
    {
        FIXME("(%p) : stub\n", This);
        warned = TRUE;
    }
    return This->softwareVertexProcessing;
}


static HRESULT  WINAPI  IWineD3DDeviceImpl_GetRasterStatus(IWineD3DDevice *iface, UINT iSwapChain, WINED3DRASTER_STATUS* pRasterStatus) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSwapChain *swapChain;
    HRESULT hr;

    TRACE("(%p) :  SwapChain %d returning %p\n", This, iSwapChain, pRasterStatus);

    hr = IWineD3DDeviceImpl_GetSwapChain(iface, iSwapChain, &swapChain);
    if(hr == WINED3D_OK){
        hr = IWineD3DSwapChain_GetRasterStatus(swapChain, pRasterStatus);
        IWineD3DSwapChain_Release(swapChain);
    }else{
        FIXME("(%p) IWineD3DSwapChain_GetRasterStatus returned in error\n", This);
    }
    return hr;
}


static HRESULT  WINAPI  IWineD3DDeviceImpl_SetNPatchMode(IWineD3DDevice *iface, float nSegments) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    static BOOL warned;
    if(nSegments != 0.0f) {
        if (!warned)
        {
            FIXME("(%p) : stub nSegments(%f)\n", This, nSegments);
            warned = TRUE;
        }
    }
    return WINED3D_OK;
}

static float    WINAPI  IWineD3DDeviceImpl_GetNPatchMode(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    static BOOL warned;
    if (!warned)
    {
        FIXME("(%p) : stub returning(%f)\n", This, 0.0f);
        warned = TRUE;
    }
    return 0.0f;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_UpdateSurface(IWineD3DDevice *iface, IWineD3DSurface *pSourceSurface, CONST RECT* pSourceRect, IWineD3DSurface *pDestinationSurface, CONST POINT* pDestPoint) {
    IWineD3DDeviceImpl  *This         = (IWineD3DDeviceImpl *) iface;
    /** TODO: remove casts to IWineD3DSurfaceImpl
     *       NOTE: move code to surface to accomplish this
      ****************************************/
    IWineD3DSurfaceImpl *pSrcSurface  = (IWineD3DSurfaceImpl *)pSourceSurface;
    int srcWidth, srcHeight;
    unsigned int  srcSurfaceWidth, srcSurfaceHeight, destSurfaceWidth, destSurfaceHeight;
    WINED3DFORMAT destFormat, srcFormat;
    UINT          destSize;
    int srcLeft, destLeft, destTop;
    WINED3DPOOL       srcPool, destPool;
    int offset    = 0;
    int rowoffset = 0; /* how many bytes to add onto the end of a row to wraparound to the beginning of the next */
    glDescriptor *glDescription = NULL;
    const struct GlPixelFormatDesc *src_format_desc, *dst_format_desc;
    GLenum dummy;
    int sampler;
    int bpp;
    CONVERT_TYPES convert = NO_CONVERSION;

    WINED3DSURFACE_DESC  winedesc;

    TRACE("(%p) : Source (%p)  Rect (%p) Destination (%p) Point(%p)\n", This, pSourceSurface, pSourceRect, pDestinationSurface, pDestPoint);
    memset(&winedesc, 0, sizeof(winedesc));
    winedesc.Width  = &srcSurfaceWidth;
    winedesc.Height = &srcSurfaceHeight;
    winedesc.Pool   = &srcPool;
    winedesc.Format = &srcFormat;

    IWineD3DSurface_GetDesc(pSourceSurface, &winedesc);

    winedesc.Width  = &destSurfaceWidth;
    winedesc.Height = &destSurfaceHeight;
    winedesc.Pool   = &destPool;
    winedesc.Format = &destFormat;
    winedesc.Size   = &destSize;

    IWineD3DSurface_GetDesc(pDestinationSurface, &winedesc);

    if(srcPool != WINED3DPOOL_SYSTEMMEM  || destPool != WINED3DPOOL_DEFAULT){
        WARN("source %p must be SYSTEMMEM and dest %p must be DEFAULT, returning WINED3DERR_INVALIDCALL\n", pSourceSurface, pDestinationSurface);
        return WINED3DERR_INVALIDCALL;
    }

    /* This call loads the opengl surface directly, instead of copying the surface to the
     * destination's sysmem copy. If surface conversion is needed, use BltFast instead to
     * copy in sysmem and use regular surface loading.
     */
    d3dfmt_get_conv((IWineD3DSurfaceImpl *) pDestinationSurface, FALSE, TRUE,
                    &dummy, &dummy, &dummy, &convert, &bpp, FALSE);
    if(convert != NO_CONVERSION) {
        return IWineD3DSurface_BltFast(pDestinationSurface,
                                        pDestPoint  ? pDestPoint->x : 0,
                                        pDestPoint  ? pDestPoint->y : 0,
                                        pSourceSurface, pSourceRect, 0);
    }

    if (destFormat == WINED3DFMT_UNKNOWN) {
        TRACE("(%p) : Converting destination surface from WINED3DFMT_UNKNOWN to the source format\n", This);
        IWineD3DSurface_SetFormat(pDestinationSurface, srcFormat);

        /* Get the update surface description */
        IWineD3DSurface_GetDesc(pDestinationSurface, &winedesc);
    }

    ActivateContext(This, This->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);

    ENTER_GL();
    GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB));
    checkGLcall("glActiveTextureARB");
    LEAVE_GL();

    /* Make sure the surface is loaded and up to date */
    surface_internal_preload(pDestinationSurface, SRGB_RGB);
    IWineD3DSurface_BindTexture(pDestinationSurface, FALSE);

    IWineD3DSurface_GetGlDesc(pDestinationSurface, &glDescription);

    src_format_desc = ((IWineD3DSurfaceImpl *)pSrcSurface)->resource.format_desc;
    dst_format_desc = ((IWineD3DSurfaceImpl *)pDestinationSurface)->resource.format_desc;

    /* this needs to be done in lines if the sourceRect != the sourceWidth */
    srcWidth   = pSourceRect ? pSourceRect->right - pSourceRect->left   : srcSurfaceWidth;
    srcHeight  = pSourceRect ? pSourceRect->bottom - pSourceRect->top   : srcSurfaceHeight;
    srcLeft    = pSourceRect ? pSourceRect->left : 0;
    destLeft   = pDestPoint  ? pDestPoint->x : 0;
    destTop    = pDestPoint  ? pDestPoint->y : 0;


    /* This function doesn't support compressed textures
    the pitch is just bytesPerPixel * width */
    if(srcWidth != srcSurfaceWidth  || srcLeft ){
        rowoffset = srcSurfaceWidth * src_format_desc->byte_count;
        offset   += srcLeft * src_format_desc->byte_count;
        /* TODO: do we ever get 3bpp?, would a shift and an add be quicker than a mul (well maybe a cycle or two) */
    }
    /* TODO DXT formats */

    if(pSourceRect != NULL && pSourceRect->top != 0){
       offset +=  pSourceRect->top * srcSurfaceWidth * src_format_desc->byte_count;
    }
    TRACE("(%p) glTexSubImage2D, level %d, left %d, top %d, width %d, height %d, fmt %#x, type %#x, memory %p+%#x\n",
            This, glDescription->level, destLeft, destTop, srcWidth, srcHeight, dst_format_desc->glFormat,
            dst_format_desc->glType, IWineD3DSurface_GetData(pSourceSurface), offset);

    /* Sanity check */
    if (IWineD3DSurface_GetData(pSourceSurface) == NULL) {

        /* need to lock the surface to get the data */
        FIXME("Surfaces has no allocated memory, but should be an in memory only surface\n");
    }

    ENTER_GL();

    /* TODO: Cube and volume support */
    if(rowoffset != 0){
        /* not a whole row so we have to do it a line at a time */
        int j;

        /* hopefully using pointer addition will be quicker than using a point + j * rowoffset */
        const unsigned char* data =((const unsigned char *)IWineD3DSurface_GetData(pSourceSurface)) + offset;

        for (j = destTop; j < (srcHeight + destTop); ++j)
        {
            glTexSubImage2D(glDescription->target, glDescription->level, destLeft, j,
                    srcWidth, 1, dst_format_desc->glFormat, dst_format_desc->glType,data);
            data += rowoffset;
        }

    } else { /* Full width, so just write out the whole texture */
        const unsigned char* data = ((const unsigned char *)IWineD3DSurface_GetData(pSourceSurface)) + offset;

        if (WINED3DFMT_DXT1 == destFormat ||
            WINED3DFMT_DXT2 == destFormat ||
            WINED3DFMT_DXT3 == destFormat ||
            WINED3DFMT_DXT4 == destFormat ||
            WINED3DFMT_DXT5 == destFormat) {
            if (GL_SUPPORT(EXT_TEXTURE_COMPRESSION_S3TC)) {
                if (destSurfaceHeight != srcHeight || destSurfaceWidth != srcWidth) {
                    /* FIXME: The easy way to do this is to lock the destination, and copy the bits across */
                    FIXME("Updating part of a compressed texture is not supported at the moment\n");
                } if (destFormat != srcFormat) {
                    FIXME("Updating mixed format compressed texture is not curretly support\n");
                } else {
                    GL_EXTCALL(glCompressedTexImage2DARB(glDescription->target, glDescription->level,
                            dst_format_desc->glInternal, srcWidth, srcHeight, 0, destSize, data));
                }
            } else {
                FIXME("Attempting to update a DXT compressed texture without hardware support\n");
            }


        } else {
            glTexSubImage2D(glDescription->target, glDescription->level, destLeft, destTop,
                    srcWidth, srcHeight, dst_format_desc->glFormat, dst_format_desc->glType, data);
        }
     }
    checkGLcall("glTexSubImage2D");

    LEAVE_GL();

    IWineD3DSurface_ModifyLocation(pDestinationSurface, SFLAG_INTEXTURE, TRUE);
    sampler = This->rev_tex_unit_map[0];
    if (sampler != -1) {
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_SAMPLER(sampler));
    }

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_DrawRectPatch(IWineD3DDevice *iface, UINT Handle, CONST float* pNumSegs, CONST WINED3DRECTPATCH_INFO* pRectPatchInfo) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    struct WineD3DRectPatch *patch;
    GLenum old_primitive_type;
    unsigned int i;
    struct list *e;
    BOOL found;
    TRACE("(%p) Handle(%d) noSegs(%p) rectpatch(%p)\n", This, Handle, pNumSegs, pRectPatchInfo);

    if(!(Handle || pRectPatchInfo)) {
        /* TODO: Write a test for the return value, thus the FIXME */
        FIXME("Both Handle and pRectPatchInfo are NULL\n");
        return WINED3DERR_INVALIDCALL;
    }

    if(Handle) {
        i = PATCHMAP_HASHFUNC(Handle);
        found = FALSE;
        LIST_FOR_EACH(e, &This->patches[i]) {
            patch = LIST_ENTRY(e, struct WineD3DRectPatch, entry);
            if(patch->Handle == Handle) {
                found = TRUE;
                break;
            }
        }

        if(!found) {
            TRACE("Patch does not exist. Creating a new one\n");
            patch = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*patch));
            patch->Handle = Handle;
            list_add_head(&This->patches[i], &patch->entry);
        } else {
            TRACE("Found existing patch %p\n", patch);
        }
    } else {
        /* Since opengl does not load tesselated vertex attributes into numbered vertex
         * attributes we have to tesselate, read back, and draw. This needs a patch
         * management structure instance. Create one.
         *
         * A possible improvement is to check if a vertex shader is used, and if not directly
         * draw the patch.
         */
        FIXME("Drawing an uncached patch. This is slow\n");
        patch = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*patch));
    }

    if(pNumSegs[0] != patch->numSegs[0] || pNumSegs[1] != patch->numSegs[1] ||
       pNumSegs[2] != patch->numSegs[2] || pNumSegs[3] != patch->numSegs[3] ||
       (pRectPatchInfo && memcmp(pRectPatchInfo, &patch->RectPatchInfo, sizeof(*pRectPatchInfo)) != 0) ) {
        HRESULT hr;
        TRACE("Tesselation density or patch info changed, retesselating\n");

        if(pRectPatchInfo) {
            patch->RectPatchInfo = *pRectPatchInfo;
        }
        patch->numSegs[0] = pNumSegs[0];
        patch->numSegs[1] = pNumSegs[1];
        patch->numSegs[2] = pNumSegs[2];
        patch->numSegs[3] = pNumSegs[3];

        hr = tesselate_rectpatch(This, patch);
        if(FAILED(hr)) {
            WARN("Patch tesselation failed\n");

            /* Do not release the handle to store the params of the patch */
            if(!Handle) {
                HeapFree(GetProcessHeap(), 0, patch);
            }
            return hr;
        }
    }

    This->currentPatch = patch;
    old_primitive_type = This->stateBlock->gl_primitive_type;
    This->stateBlock->gl_primitive_type = GL_TRIANGLES;
    IWineD3DDevice_DrawPrimitiveStrided(iface, patch->numSegs[0] * patch->numSegs[1] * 2, &patch->strided);
    This->stateBlock->gl_primitive_type = old_primitive_type;
    This->currentPatch = NULL;

    /* Destroy uncached patches */
    if(!Handle) {
        HeapFree(GetProcessHeap(), 0, patch->mem);
        HeapFree(GetProcessHeap(), 0, patch);
    }
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_DrawTriPatch(IWineD3DDevice *iface, UINT Handle, CONST float* pNumSegs, CONST WINED3DTRIPATCH_INFO* pTriPatchInfo) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) Handle(%d) noSegs(%p) tripatch(%p)\n", This, Handle, pNumSegs, pTriPatchInfo);
    FIXME("(%p) : Stub\n", This);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_DeletePatch(IWineD3DDevice *iface, UINT Handle) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int i;
    struct WineD3DRectPatch *patch;
    struct list *e;
    TRACE("(%p) Handle(%d)\n", This, Handle);

    i = PATCHMAP_HASHFUNC(Handle);
    LIST_FOR_EACH(e, &This->patches[i]) {
        patch = LIST_ENTRY(e, struct WineD3DRectPatch, entry);
        if(patch->Handle == Handle) {
            TRACE("Deleting patch %p\n", patch);
            list_remove(&patch->entry);
            HeapFree(GetProcessHeap(), 0, patch->mem);
            HeapFree(GetProcessHeap(), 0, patch);
            return WINED3D_OK;
        }
    }

    /* TODO: Write a test for the return value */
    FIXME("Attempt to destroy nonexistent patch\n");
    return WINED3DERR_INVALIDCALL;
}

static IWineD3DSwapChain *get_swapchain(IWineD3DSurface *target) {
    HRESULT hr;
    IWineD3DSwapChain *swapchain;

    hr = IWineD3DSurface_GetContainer(target, &IID_IWineD3DSwapChain, (void **)&swapchain);
    if (SUCCEEDED(hr)) {
        IWineD3DSwapChain_Release((IUnknown *)swapchain);
        return swapchain;
    }

    return NULL;
}

static void color_fill_fbo(IWineD3DDevice *iface, IWineD3DSurface *surface,
        const WINED3DRECT *rect, const float color[4])
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    IWineD3DSwapChain *swapchain;

    swapchain = get_swapchain(surface);
    if (swapchain) {
        GLenum buffer;

        TRACE("Surface %p is onscreen\n", surface);

        ActivateContext(This, surface, CTXUSAGE_RESOURCELOAD);
        ENTER_GL();
        GL_EXTCALL(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0));
        buffer = surface_get_gl_buffer(surface, swapchain);
        glDrawBuffer(buffer);
        checkGLcall("glDrawBuffer()");
    } else {
        TRACE("Surface %p is offscreen\n", surface);

        ActivateContext(This, This->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
        ENTER_GL();
        context_bind_fbo(iface, GL_FRAMEBUFFER_EXT, &This->activeContext->dst_fbo);
        context_attach_surface_fbo(This, GL_FRAMEBUFFER_EXT, 0, surface);
        GL_EXTCALL(glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, 0));
        checkGLcall("glFramebufferRenderbufferEXT");
    }

    if (rect) {
        glEnable(GL_SCISSOR_TEST);
        if(!swapchain) {
            glScissor(rect->x1, rect->y1, rect->x2 - rect->x1, rect->y2 - rect->y1);
        } else {
            glScissor(rect->x1, ((IWineD3DSurfaceImpl *)surface)->currentDesc.Height - rect->y2,
                    rect->x2 - rect->x1, rect->y2 - rect->y1);
        }
        checkGLcall("glScissor");
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_SCISSORRECT);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_SCISSORTESTENABLE));

    glDisable(GL_BLEND);
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_ALPHABLENDENABLE));

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_COLORWRITEENABLE));

    glClearColor(color[0], color[1], color[2], color[3]);
    glClear(GL_COLOR_BUFFER_BIT);
    checkGLcall("glClear");

    if (This->activeContext->current_fbo) {
        context_bind_fbo(iface, GL_FRAMEBUFFER_EXT, &This->activeContext->current_fbo->id);
    } else {
        GL_EXTCALL(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0));
        checkGLcall("glBindFramebuffer()");
    }

    if (swapchain && surface == ((IWineD3DSwapChainImpl *)swapchain)->frontBuffer
            && ((IWineD3DSwapChainImpl *)swapchain)->backBuffer) {
        glDrawBuffer(GL_BACK);
        checkGLcall("glDrawBuffer()");
    }

    LEAVE_GL();
}

static inline DWORD argb_to_fmt(DWORD color, WINED3DFORMAT destfmt) {
    unsigned int r, g, b, a;
    DWORD ret;

    if(destfmt == WINED3DFMT_A8R8G8B8 || destfmt == WINED3DFMT_X8R8G8B8 ||
       destfmt == WINED3DFMT_R8G8B8)
        return color;

    TRACE("Converting color %08x to format %s\n", color, debug_d3dformat(destfmt));

    a = (color & 0xff000000) >> 24;
    r = (color & 0x00ff0000) >> 16;
    g = (color & 0x0000ff00) >>  8;
    b = (color & 0x000000ff) >>  0;

    switch(destfmt)
    {
        case WINED3DFMT_R5G6B5:
            if(r == 0xff && g == 0xff && b == 0xff) return 0xffff;
            r = (r * 32) / 256;
            g = (g * 64) / 256;
            b = (b * 32) / 256;
            ret  = r << 11;
            ret |= g << 5;
            ret |= b;
            TRACE("Returning %08x\n", ret);
            return ret;

        case WINED3DFMT_X1R5G5B5:
        case WINED3DFMT_A1R5G5B5:
            a = (a *  2) / 256;
            r = (r * 32) / 256;
            g = (g * 32) / 256;
            b = (b * 32) / 256;
            ret  = a << 15;
            ret |= r << 10;
            ret |= g <<  5;
            ret |= b <<  0;
            TRACE("Returning %08x\n", ret);
            return ret;

        case WINED3DFMT_A8_UNORM:
            TRACE("Returning %08x\n", a);
            return a;

        case WINED3DFMT_X4R4G4B4:
        case WINED3DFMT_A4R4G4B4:
            a = (a * 16) / 256;
            r = (r * 16) / 256;
            g = (g * 16) / 256;
            b = (b * 16) / 256;
            ret  = a << 12;
            ret |= r <<  8;
            ret |= g <<  4;
            ret |= b <<  0;
            TRACE("Returning %08x\n", ret);
            return ret;

        case WINED3DFMT_R3G3B2:
            r = (r * 8) / 256;
            g = (g * 8) / 256;
            b = (b * 4) / 256;
            ret  = r <<  5;
            ret |= g <<  2;
            ret |= b <<  0;
            TRACE("Returning %08x\n", ret);
            return ret;

        case WINED3DFMT_X8B8G8R8:
        case WINED3DFMT_R8G8B8A8_UNORM:
            ret  = a << 24;
            ret |= b << 16;
            ret |= g <<  8;
            ret |= r <<  0;
            TRACE("Returning %08x\n", ret);
            return ret;

        case WINED3DFMT_A2R10G10B10:
            a = (a *    4) / 256;
            r = (r * 1024) / 256;
            g = (g * 1024) / 256;
            b = (b * 1024) / 256;
            ret  = a << 30;
            ret |= r << 20;
            ret |= g << 10;
            ret |= b <<  0;
            TRACE("Returning %08x\n", ret);
            return ret;

        case WINED3DFMT_R10G10B10A2_UNORM:
            a = (a *    4) / 256;
            r = (r * 1024) / 256;
            g = (g * 1024) / 256;
            b = (b * 1024) / 256;
            ret  = a << 30;
            ret |= b << 20;
            ret |= g << 10;
            ret |= r <<  0;
            TRACE("Returning %08x\n", ret);
            return ret;

        default:
            FIXME("Add a COLORFILL conversion for format %s\n", debug_d3dformat(destfmt));
            return 0;
    }
}

static HRESULT WINAPI IWineD3DDeviceImpl_ColorFill(IWineD3DDevice *iface, IWineD3DSurface *pSurface, CONST WINED3DRECT* pRect, WINED3DCOLOR color) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    IWineD3DSurfaceImpl *surface = (IWineD3DSurfaceImpl *) pSurface;
    WINEDDBLTFX BltFx;
    TRACE("(%p) Colour fill Surface: %p rect: %p color: 0x%08x\n", This, pSurface, pRect, color);

    if (surface->resource.pool != WINED3DPOOL_DEFAULT && surface->resource.pool != WINED3DPOOL_SYSTEMMEM) {
        FIXME("call to colorfill with non WINED3DPOOL_DEFAULT or WINED3DPOOL_SYSTEMMEM surface\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO) {
        const float c[4] = {D3DCOLOR_R(color), D3DCOLOR_G(color), D3DCOLOR_B(color), D3DCOLOR_A(color)};
        color_fill_fbo(iface, pSurface, pRect, c);
        return WINED3D_OK;
    } else {
        /* Just forward this to the DirectDraw blitting engine */
        memset(&BltFx, 0, sizeof(BltFx));
        BltFx.dwSize = sizeof(BltFx);
        BltFx.u5.dwFillColor = argb_to_fmt(color, surface->resource.format_desc->format);
        return IWineD3DSurface_Blt(pSurface, (const RECT *)pRect, NULL, NULL,
                WINEDDBLT_COLORFILL, &BltFx, WINED3DTEXF_NONE);
    }
}

static void WINAPI IWineD3DDeviceImpl_ClearRendertargetView(IWineD3DDevice *iface,
        IWineD3DRendertargetView *rendertarget_view, const float color[4])
{
    IWineD3DResource *resource;
    IWineD3DSurface *surface;
    HRESULT hr;

    hr = IWineD3DRendertargetView_GetResource(rendertarget_view, &resource);
    if (FAILED(hr))
    {
        ERR("Failed to get resource, hr %#x\n", hr);
        return;
    }

    if (IWineD3DResource_GetType(resource) != WINED3DRTYPE_SURFACE)
    {
        FIXME("Only supported on surface resources\n");
        IWineD3DResource_Release(resource);
        return;
    }

    surface = (IWineD3DSurface *)resource;

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
    {
        color_fill_fbo(iface, surface, NULL, color);
    }
    else
    {
        WINEDDBLTFX BltFx;
        WINED3DCOLOR c;

        WARN("Converting to WINED3DCOLOR, this might give incorrect results\n");

        c = ((DWORD)(color[2] * 255.0));
        c |= ((DWORD)(color[1] * 255.0)) << 8;
        c |= ((DWORD)(color[0] * 255.0)) << 16;
        c |= ((DWORD)(color[3] * 255.0)) << 24;

        /* Just forward this to the DirectDraw blitting engine */
        memset(&BltFx, 0, sizeof(BltFx));
        BltFx.dwSize = sizeof(BltFx);
        BltFx.u5.dwFillColor = argb_to_fmt(c, ((IWineD3DSurfaceImpl *)surface)->resource.format_desc->format);
        hr = IWineD3DSurface_Blt(surface, NULL, NULL, NULL, WINEDDBLT_COLORFILL, &BltFx, WINED3DTEXF_NONE);
        if (FAILED(hr))
        {
            ERR("Blt failed, hr %#x\n", hr);
        }
    }

    IWineD3DResource_Release(resource);
}

/* rendertarget and depth stencil functions */
static HRESULT  WINAPI  IWineD3DDeviceImpl_GetRenderTarget(IWineD3DDevice* iface,DWORD RenderTargetIndex, IWineD3DSurface **ppRenderTarget) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    if (RenderTargetIndex >= GL_LIMITS(buffers)) {
        ERR("(%p) : Only %d render targets are supported.\n", This, GL_LIMITS(buffers));
        return WINED3DERR_INVALIDCALL;
    }

    *ppRenderTarget = This->render_targets[RenderTargetIndex];
    TRACE("(%p) : RenderTarget %d Index returning %p\n", This, RenderTargetIndex, *ppRenderTarget);
    /* Note inc ref on returned surface */
    if(*ppRenderTarget != NULL)
        IWineD3DSurface_AddRef(*ppRenderTarget);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetFrontBackBuffers(IWineD3DDevice *iface, IWineD3DSurface *Front, IWineD3DSurface *Back) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSurfaceImpl *FrontImpl = (IWineD3DSurfaceImpl *) Front;
    IWineD3DSurfaceImpl *BackImpl = (IWineD3DSurfaceImpl *) Back;
    IWineD3DSwapChainImpl *Swapchain;
    HRESULT hr;

    TRACE("(%p)->(%p,%p)\n", This, FrontImpl, BackImpl);

    hr = IWineD3DDevice_GetSwapChain(iface, 0, (IWineD3DSwapChain **) &Swapchain);
    if(hr != WINED3D_OK) {
        ERR("Can't get the swapchain\n");
        return hr;
    }

    /* Make sure to release the swapchain */
    IWineD3DSwapChain_Release((IWineD3DSwapChain *) Swapchain);

    if(FrontImpl && !(FrontImpl->resource.usage & WINED3DUSAGE_RENDERTARGET) ) {
        ERR("Trying to set a front buffer which doesn't have WINED3DUSAGE_RENDERTARGET usage\n");
        return WINED3DERR_INVALIDCALL;
    }
    else if(BackImpl && !(BackImpl->resource.usage & WINED3DUSAGE_RENDERTARGET)) {
        ERR("Trying to set a back buffer which doesn't have WINED3DUSAGE_RENDERTARGET usage\n");
        return WINED3DERR_INVALIDCALL;
    }

    if(Swapchain->frontBuffer != Front) {
        TRACE("Changing the front buffer from %p to %p\n", Swapchain->frontBuffer, Front);

        if(Swapchain->frontBuffer)
        {
            IWineD3DSurface_SetContainer(Swapchain->frontBuffer, NULL);
            ((IWineD3DSurfaceImpl *)Swapchain->frontBuffer)->Flags &= ~SFLAG_SWAPCHAIN;
        }
        Swapchain->frontBuffer = Front;

        if(Swapchain->frontBuffer) {
            IWineD3DSurface_SetContainer(Swapchain->frontBuffer, (IWineD3DBase *) Swapchain);
            ((IWineD3DSurfaceImpl *)Swapchain->frontBuffer)->Flags |= SFLAG_SWAPCHAIN;
        }
    }

    if(Back && !Swapchain->backBuffer) {
        /* We need memory for the back buffer array - only one back buffer this way */
        Swapchain->backBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IWineD3DSurface *));
        if(!Swapchain->backBuffer) {
            ERR("Out of memory\n");
            return E_OUTOFMEMORY;
        }
    }

    if(Swapchain->backBuffer[0] != Back) {
        TRACE("Changing the back buffer from %p to %p\n", Swapchain->backBuffer, Back);

        /* What to do about the context here in the case of multithreading? Not sure.
         * This function is called by IDirect3D7::CreateDevice so in theory its initialization code
         */
        ENTER_GL();
        if(!Swapchain->backBuffer[0]) {
            /* GL was told to draw to the front buffer at creation,
             * undo that
             */
            glDrawBuffer(GL_BACK);
            checkGLcall("glDrawBuffer(GL_BACK)");
            /* Set the backbuffer count to 1 because other code uses it to fing the back buffers */
            Swapchain->presentParms.BackBufferCount = 1;
        } else if (!Back) {
            /* That makes problems - disable for now */
            /* glDrawBuffer(GL_FRONT); */
            checkGLcall("glDrawBuffer(GL_FRONT)");
            /* We have lost our back buffer, set this to 0 to avoid confusing other code */
            Swapchain->presentParms.BackBufferCount = 0;
        }
        LEAVE_GL();

        if(Swapchain->backBuffer[0])
        {
            IWineD3DSurface_SetContainer(Swapchain->backBuffer[0], NULL);
            ((IWineD3DSurfaceImpl *)Swapchain->backBuffer[0])->Flags &= ~SFLAG_SWAPCHAIN;
        }
        Swapchain->backBuffer[0] = Back;

        if(Swapchain->backBuffer[0]) {
            IWineD3DSurface_SetContainer(Swapchain->backBuffer[0], (IWineD3DBase *) Swapchain);
            ((IWineD3DSurfaceImpl *)Swapchain->backBuffer[0])->Flags |= SFLAG_SWAPCHAIN;
        } else {
            HeapFree(GetProcessHeap(), 0, Swapchain->backBuffer);
            Swapchain->backBuffer = NULL;
        }

    }

    return WINED3D_OK;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_GetDepthStencilSurface(IWineD3DDevice* iface, IWineD3DSurface **ppZStencilSurface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    *ppZStencilSurface = This->stencilBufferTarget;
    TRACE("(%p) : zStencilSurface  returning %p\n", This,  *ppZStencilSurface);

    if(*ppZStencilSurface != NULL) {
        /* Note inc ref on returned surface */
        IWineD3DSurface_AddRef(*ppZStencilSurface);
        return WINED3D_OK;
    } else {
        return WINED3DERR_NOTFOUND;
    }
}

void stretch_rect_fbo(IWineD3DDevice *iface, IWineD3DSurface *src_surface, WINED3DRECT *src_rect,
        IWineD3DSurface *dst_surface, WINED3DRECT *dst_rect, const WINED3DTEXTUREFILTERTYPE filter, BOOL flip)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    GLbitfield mask = GL_COLOR_BUFFER_BIT; /* TODO: Support blitting depth/stencil surfaces */
    IWineD3DSwapChain *src_swapchain, *dst_swapchain;
    GLenum gl_filter;
    POINT offset = {0, 0};

    TRACE("(%p) : src_surface %p, src_rect %p, dst_surface %p, dst_rect %p, filter %s (0x%08x), flip %u\n",
            This, src_surface, src_rect, dst_surface, dst_rect, debug_d3dtexturefiltertype(filter), filter, flip);
    TRACE("src_rect [%u, %u]->[%u, %u]\n", src_rect->x1, src_rect->y1, src_rect->x2, src_rect->y2);
    TRACE("dst_rect [%u, %u]->[%u, %u]\n", dst_rect->x1, dst_rect->y1, dst_rect->x2, dst_rect->y2);

    switch (filter) {
        case WINED3DTEXF_LINEAR:
            gl_filter = GL_LINEAR;
            break;

        default:
            FIXME("Unsupported filter mode %s (0x%08x)\n", debug_d3dtexturefiltertype(filter), filter);
        case WINED3DTEXF_NONE:
        case WINED3DTEXF_POINT:
            gl_filter = GL_NEAREST;
            break;
    }

    /* Attach src surface to src fbo */
    src_swapchain = get_swapchain(src_surface);
    if (src_swapchain) {
        GLenum buffer = surface_get_gl_buffer(src_surface, src_swapchain);

        TRACE("Source surface %p is onscreen\n", src_surface);
        ActivateContext(This, src_surface, CTXUSAGE_RESOURCELOAD);
        /* Make sure the drawable is up to date. In the offscreen case
         * attach_surface_fbo() implicitly takes care of this. */
        IWineD3DSurface_LoadLocation(src_surface, SFLAG_INDRAWABLE, NULL);

        if(buffer == GL_FRONT) {
            RECT windowsize;
            UINT h;
            ClientToScreen(((IWineD3DSwapChainImpl *)src_swapchain)->win_handle, &offset);
            GetClientRect(((IWineD3DSwapChainImpl *)src_swapchain)->win_handle, &windowsize);
            h = windowsize.bottom - windowsize.top;
            src_rect->x1 -= offset.x; src_rect->x2 -=offset.x;
            src_rect->y1 =  offset.y + h - src_rect->y1;
            src_rect->y2 =  offset.y + h - src_rect->y2;
        } else {
            src_rect->y1 = ((IWineD3DSurfaceImpl *)src_surface)->currentDesc.Height - src_rect->y1;
            src_rect->y2 = ((IWineD3DSurfaceImpl *)src_surface)->currentDesc.Height - src_rect->y2;
        }

        ENTER_GL();
        GL_EXTCALL(glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0));
        glReadBuffer(buffer);
        checkGLcall("glReadBuffer()");
    } else {
        TRACE("Source surface %p is offscreen\n", src_surface);
        ENTER_GL();
        context_bind_fbo(iface, GL_READ_FRAMEBUFFER_EXT, &This->activeContext->src_fbo);
        context_attach_surface_fbo(This, GL_READ_FRAMEBUFFER_EXT, 0, src_surface);
        glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
        checkGLcall("glReadBuffer()");
        GL_EXTCALL(glFramebufferRenderbufferEXT(GL_READ_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, 0));
        checkGLcall("glFramebufferRenderbufferEXT");
    }
    LEAVE_GL();

    /* Attach dst surface to dst fbo */
    dst_swapchain = get_swapchain(dst_surface);
    if (dst_swapchain) {
        GLenum buffer = surface_get_gl_buffer(dst_surface, dst_swapchain);

        TRACE("Destination surface %p is onscreen\n", dst_surface);
        ActivateContext(This, dst_surface, CTXUSAGE_RESOURCELOAD);
        /* Make sure the drawable is up to date. In the offscreen case
         * attach_surface_fbo() implicitly takes care of this. */
        IWineD3DSurface_LoadLocation(dst_surface, SFLAG_INDRAWABLE, NULL);

        if(buffer == GL_FRONT) {
            RECT windowsize;
            UINT h;
            ClientToScreen(((IWineD3DSwapChainImpl *)dst_swapchain)->win_handle, &offset);
            GetClientRect(((IWineD3DSwapChainImpl *)dst_swapchain)->win_handle, &windowsize);
            h = windowsize.bottom - windowsize.top;
            dst_rect->x1 -= offset.x; dst_rect->x2 -=offset.x;
            dst_rect->y1 =  offset.y + h - dst_rect->y1;
            dst_rect->y2 =  offset.y + h - dst_rect->y2;
        } else {
            /* Screen coords = window coords, surface height = window height */
            dst_rect->y1 = ((IWineD3DSurfaceImpl *)dst_surface)->currentDesc.Height - dst_rect->y1;
            dst_rect->y2 = ((IWineD3DSurfaceImpl *)dst_surface)->currentDesc.Height - dst_rect->y2;
        }

        ENTER_GL();
        GL_EXTCALL(glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0));
        glDrawBuffer(buffer);
        checkGLcall("glDrawBuffer()");
    } else {
        TRACE("Destination surface %p is offscreen\n", dst_surface);

        /* No src or dst swapchain? Make sure some context is active(multithreading) */
        if(!src_swapchain) {
            ActivateContext(This, This->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
        }

        ENTER_GL();
        context_bind_fbo(iface, GL_DRAW_FRAMEBUFFER_EXT, &This->activeContext->dst_fbo);
        context_attach_surface_fbo(This, GL_DRAW_FRAMEBUFFER_EXT, 0, dst_surface);
        glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
        checkGLcall("glDrawBuffer()");
        GL_EXTCALL(glFramebufferRenderbufferEXT(GL_DRAW_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, 0));
        checkGLcall("glFramebufferRenderbufferEXT");
    }
    glDisable(GL_SCISSOR_TEST);
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_SCISSORTESTENABLE));

    if (flip) {
        GL_EXTCALL(glBlitFramebufferEXT(src_rect->x1, src_rect->y1, src_rect->x2, src_rect->y2,
                dst_rect->x1, dst_rect->y2, dst_rect->x2, dst_rect->y1, mask, gl_filter));
        checkGLcall("glBlitFramebuffer()");
    } else {
        GL_EXTCALL(glBlitFramebufferEXT(src_rect->x1, src_rect->y1, src_rect->x2, src_rect->y2,
                dst_rect->x1, dst_rect->y1, dst_rect->x2, dst_rect->y2, mask, gl_filter));
        checkGLcall("glBlitFramebuffer()");
    }

    IWineD3DSurface_ModifyLocation(dst_surface, SFLAG_INDRAWABLE, TRUE);

    if (This->activeContext->current_fbo) {
        context_bind_fbo(iface, GL_FRAMEBUFFER_EXT, &This->activeContext->current_fbo->id);
    } else {
        GL_EXTCALL(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0));
        checkGLcall("glBindFramebuffer()");
    }

    /* If we switched from GL_BACK to GL_FRONT above, we need to switch back here */
    if (dst_swapchain && dst_surface == ((IWineD3DSwapChainImpl *)dst_swapchain)->frontBuffer
            && ((IWineD3DSwapChainImpl *)dst_swapchain)->backBuffer) {
        glDrawBuffer(GL_BACK);
        checkGLcall("glDrawBuffer()");
    }
    LEAVE_GL();
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetRenderTarget(IWineD3DDevice *iface, DWORD RenderTargetIndex, IWineD3DSurface *pRenderTarget) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    WINED3DVIEWPORT viewport;

    TRACE("(%p) : Setting rendertarget %d to %p\n", This, RenderTargetIndex, pRenderTarget);

    if (RenderTargetIndex >= GL_LIMITS(buffers)) {
        WARN("(%p) : Unsupported target %u set, returning WINED3DERR_INVALIDCALL(only %u supported)\n",
             This, RenderTargetIndex, GL_LIMITS(buffers));
        return WINED3DERR_INVALIDCALL;
    }

    /* MSDN says that null disables the render target
    but a device must always be associated with a render target
    nope MSDN says that we return invalid call to a null rendertarget with an index of 0
    */
    if (RenderTargetIndex == 0 && pRenderTarget == NULL) {
        FIXME("Trying to set render target 0 to NULL\n");
        return WINED3DERR_INVALIDCALL;
    }
    if (pRenderTarget && !(((IWineD3DSurfaceImpl *)pRenderTarget)->resource.usage & WINED3DUSAGE_RENDERTARGET)) {
        FIXME("(%p)Trying to set the render target to a surface(%p) that wasn't created with a usage of WINED3DUSAGE_RENDERTARGET\n",This ,pRenderTarget);
        return WINED3DERR_INVALIDCALL;
    }

    /* If we are trying to set what we already have, don't bother */
    if (pRenderTarget == This->render_targets[RenderTargetIndex]) {
        TRACE("Trying to do a NOP SetRenderTarget operation\n");
        return WINED3D_OK;
    }
    if(pRenderTarget) IWineD3DSurface_AddRef(pRenderTarget);
    if(This->render_targets[RenderTargetIndex]) IWineD3DSurface_Release(This->render_targets[RenderTargetIndex]);
    This->render_targets[RenderTargetIndex] = pRenderTarget;

    /* Render target 0 is special */
    if(RenderTargetIndex == 0) {
        /* Finally, reset the viewport as the MSDN states. */
        viewport.Height = ((IWineD3DSurfaceImpl *)This->render_targets[0])->currentDesc.Height;
        viewport.Width  = ((IWineD3DSurfaceImpl *)This->render_targets[0])->currentDesc.Width;
        viewport.X      = 0;
        viewport.Y      = 0;
        viewport.MaxZ   = 1.0f;
        viewport.MinZ   = 0.0f;
        IWineD3DDeviceImpl_SetViewport(iface, &viewport);
        /* Make sure the viewport state is dirty, because the render_offscreen thing affects it.
         * SetViewport may catch NOP viewport changes, which would occur when switching between equally sized targets
         */
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_VIEWPORT);
    }
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetDepthStencilSurface(IWineD3DDevice *iface, IWineD3DSurface *pNewZStencil) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    HRESULT  hr = WINED3D_OK;
    IWineD3DSurface *tmp;

    TRACE("(%p) Swapping z-buffer. Old = %p, new = %p\n",This, This->stencilBufferTarget, pNewZStencil);

    if (pNewZStencil == This->stencilBufferTarget) {
        TRACE("Trying to do a NOP SetRenderTarget operation\n");
    } else {
        /** OpenGL doesn't support 'sharing' of the stencilBuffer so we may incur an extra memory overhead
        * depending on the renter target implementation being used.
        * A shared context implementation will share all buffers between all rendertargets (including swapchains),
        * implementations that use separate pbuffers for different swapchains or rendertargets will have to duplicate the
        * stencil buffer and incur an extra memory overhead
         ******************************************************/

        if (This->stencilBufferTarget) {
            if (((IWineD3DSwapChainImpl *)This->swapchains[0])->presentParms.Flags & WINED3DPRESENTFLAG_DISCARD_DEPTHSTENCIL
                    || ((IWineD3DSurfaceImpl *)This->stencilBufferTarget)->Flags & SFLAG_DISCARD) {
                surface_modify_ds_location(This->stencilBufferTarget, SFLAG_DS_DISCARDED);
            } else {
                ActivateContext(This, This->render_targets[0], CTXUSAGE_RESOURCELOAD);
                surface_load_ds_location(This->stencilBufferTarget, SFLAG_DS_OFFSCREEN);
                surface_modify_ds_location(This->stencilBufferTarget, SFLAG_DS_OFFSCREEN);
            }
        }

        tmp = This->stencilBufferTarget;
        This->stencilBufferTarget = pNewZStencil;
        /* should we be calling the parent or the wined3d surface? */
        if (NULL != This->stencilBufferTarget) IWineD3DSurface_AddRef(This->stencilBufferTarget);
        if (NULL != tmp) IWineD3DSurface_Release(tmp);
        hr = WINED3D_OK;

        if((!tmp && pNewZStencil) || (!pNewZStencil && tmp)) {
            /* Swapping NULL / non NULL depth stencil affects the depth and tests */
            IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_ZENABLE));
            IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_STENCILENABLE));
            IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_STENCILWRITEMASK));
        }
    }

    return hr;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_SetCursorProperties(IWineD3DDevice* iface, UINT XHotSpot,
                                                        UINT YHotSpot, IWineD3DSurface *pCursorBitmap) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    /* TODO: the use of Impl is deprecated. */
    IWineD3DSurfaceImpl * pSur = (IWineD3DSurfaceImpl *) pCursorBitmap;
    WINED3DLOCKED_RECT lockedRect;

    TRACE("(%p) : Spot Pos(%u,%u)\n", This, XHotSpot, YHotSpot);

    /* some basic validation checks */
    if(This->cursorTexture) {
        ActivateContext(This, This->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
        ENTER_GL();
        glDeleteTextures(1, &This->cursorTexture);
        LEAVE_GL();
        This->cursorTexture = 0;
    }

    if ( (pSur->currentDesc.Width == 32) && (pSur->currentDesc.Height == 32) )
        This->haveHardwareCursor = TRUE;
    else
        This->haveHardwareCursor = FALSE;

    if(pCursorBitmap) {
        WINED3DLOCKED_RECT rect;

        /* MSDN: Cursor must be A8R8G8B8 */
        if (WINED3DFMT_A8R8G8B8 != pSur->resource.format_desc->format)
        {
            ERR("(%p) : surface(%p) has an invalid format\n", This, pCursorBitmap);
            return WINED3DERR_INVALIDCALL;
        }

        /* MSDN: Cursor must be smaller than the display mode */
        if(pSur->currentDesc.Width > This->ddraw_width ||
           pSur->currentDesc.Height > This->ddraw_height) {
            ERR("(%p) : Surface(%p) is %dx%d pixels, but screen res is %dx%d\n", This, pSur, pSur->currentDesc.Width, pSur->currentDesc.Height, This->ddraw_width, This->ddraw_height);
            return WINED3DERR_INVALIDCALL;
        }

        if (!This->haveHardwareCursor) {
            /* TODO: MSDN: Cursor sizes must be a power of 2 */

            /* Do not store the surface's pointer because the application may
             * release it after setting the cursor image. Windows doesn't
             * addref the set surface, so we can't do this either without
             * creating circular refcount dependencies. Copy out the gl texture
             * instead.
             */
            This->cursorWidth = pSur->currentDesc.Width;
            This->cursorHeight = pSur->currentDesc.Height;
            if (SUCCEEDED(IWineD3DSurface_LockRect(pCursorBitmap, &rect, NULL, WINED3DLOCK_READONLY)))
            {
                const struct GlPixelFormatDesc *glDesc = getFormatDescEntry(WINED3DFMT_A8R8G8B8, &GLINFO_LOCATION);
                char *mem, *bits = rect.pBits;
                GLint intfmt = glDesc->glInternal;
                GLint format = glDesc->glFormat;
                GLint type = glDesc->glType;
                INT height = This->cursorHeight;
                INT width = This->cursorWidth;
                INT bpp = glDesc->byte_count;
                INT i, sampler;

                /* Reformat the texture memory (pitch and width can be
                 * different) */
                mem = HeapAlloc(GetProcessHeap(), 0, width * height * bpp);
                for(i = 0; i < height; i++)
                    memcpy(&mem[width * bpp * i], &bits[rect.Pitch * i], width * bpp);
                IWineD3DSurface_UnlockRect(pCursorBitmap);
                ENTER_GL();

                if(GL_SUPPORT(APPLE_CLIENT_STORAGE)) {
                    glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
                    checkGLcall("glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE)");
                }

                /* Make sure that a proper texture unit is selected */
                GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB));
                checkGLcall("glActiveTextureARB");
                sampler = This->rev_tex_unit_map[0];
                if (sampler != -1) {
                    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_SAMPLER(sampler));
                }
                /* Create a new cursor texture */
                glGenTextures(1, &This->cursorTexture);
                checkGLcall("glGenTextures");
                glBindTexture(GL_TEXTURE_2D, This->cursorTexture);
                checkGLcall("glBindTexture");
                /* Copy the bitmap memory into the cursor texture */
                glTexImage2D(GL_TEXTURE_2D, 0, intfmt, width, height, 0, format, type, mem);
                HeapFree(GetProcessHeap(), 0, mem);
                checkGLcall("glTexImage2D");

                if(GL_SUPPORT(APPLE_CLIENT_STORAGE)) {
                    glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
                    checkGLcall("glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE)");
                }

                LEAVE_GL();
            }
            else
            {
                FIXME("A cursor texture was not returned.\n");
                This->cursorTexture = 0;
            }
        }
        else
        {
            /* Draw a hardware cursor */
            ICONINFO cursorInfo;
            HCURSOR cursor;
            /* Create and clear maskBits because it is not needed for
             * 32-bit cursors.  32x32 bits split into 32-bit chunks == 32
             * chunks. */
            DWORD *maskBits = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                (pSur->currentDesc.Width * pSur->currentDesc.Height / 8));
            IWineD3DSurface_LockRect(pCursorBitmap, &lockedRect, NULL,
                                         WINED3DLOCK_NO_DIRTY_UPDATE |
                                         WINED3DLOCK_READONLY
            );
            TRACE("width: %i height: %i\n", pSur->currentDesc.Width,
                  pSur->currentDesc.Height);

            cursorInfo.fIcon = FALSE;
            cursorInfo.xHotspot = XHotSpot;
            cursorInfo.yHotspot = YHotSpot;
            cursorInfo.hbmMask = CreateBitmap(pSur->currentDesc.Width,
                                              pSur->currentDesc.Height, 1,
                                              1, &maskBits);
            cursorInfo.hbmColor = CreateBitmap(pSur->currentDesc.Width,
                                               pSur->currentDesc.Height, 1,
                                               32, lockedRect.pBits);
            IWineD3DSurface_UnlockRect(pCursorBitmap);
            /* Create our cursor and clean up. */
            cursor = CreateIconIndirect(&cursorInfo);
            SetCursor(cursor);
            if (cursorInfo.hbmMask) DeleteObject(cursorInfo.hbmMask);
            if (cursorInfo.hbmColor) DeleteObject(cursorInfo.hbmColor);
            if (This->hardwareCursor) DestroyCursor(This->hardwareCursor);
            This->hardwareCursor = cursor;
            HeapFree(GetProcessHeap(), 0, maskBits);
        }
    }

    This->xHotSpot = XHotSpot;
    This->yHotSpot = YHotSpot;
    return WINED3D_OK;
}

static void     WINAPI  IWineD3DDeviceImpl_SetCursorPosition(IWineD3DDevice* iface, int XScreenSpace, int YScreenSpace, DWORD Flags) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    TRACE("(%p) : SetPos to (%u,%u)\n", This, XScreenSpace, YScreenSpace);

    This->xScreenSpace = XScreenSpace;
    This->yScreenSpace = YScreenSpace;

    return;

}

static BOOL     WINAPI  IWineD3DDeviceImpl_ShowCursor(IWineD3DDevice* iface, BOOL bShow) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    BOOL oldVisible = This->bCursorVisible;
    POINT pt;

    TRACE("(%p) : visible(%d)\n", This, bShow);

    /*
     * When ShowCursor is first called it should make the cursor appear at the OS's last
     * known cursor position.  Because of this, some applications just repetitively call
     * ShowCursor in order to update the cursor's position.  This behavior is undocumented.
     */
    GetCursorPos(&pt);
    This->xScreenSpace = pt.x;
    This->yScreenSpace = pt.y;

    if (This->haveHardwareCursor) {
        This->bCursorVisible = bShow;
        if (bShow)
            SetCursor(This->hardwareCursor);
        else
            SetCursor(NULL);
    }
    else
    {
        if (This->cursorTexture)
            This->bCursorVisible = bShow;
    }

    return oldVisible;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_TestCooperativeLevel(IWineD3DDevice* iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    IWineD3DResourceImpl *resource;
    TRACE("(%p) : state (%u)\n", This, This->state);

    /* TODO: Implement wrapping of the WndProc so that mimimize and maximize can be monitored and the states adjusted. */
    switch (This->state) {
    case WINED3D_OK:
        return WINED3D_OK;
    case WINED3DERR_DEVICELOST:
        {
            LIST_FOR_EACH_ENTRY(resource, &This->resources, IWineD3DResourceImpl, resource.resource_list_entry) {
                if (resource->resource.pool == WINED3DPOOL_DEFAULT)
                    return WINED3DERR_DEVICENOTRESET;
            }
            return WINED3DERR_DEVICELOST;
        }
    case WINED3DERR_DRIVERINTERNALERROR:
        return WINED3DERR_DRIVERINTERNALERROR;
    }

    /* Unknown state */
    return WINED3DERR_DRIVERINTERNALERROR;
}


static HRESULT  WINAPI  IWineD3DDeviceImpl_EvictManagedResources(IWineD3DDevice* iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    /** FIXME: Resource tracking needs to be done,
    * The closes we can do to this is set the priorities of all managed textures low
    * and then reset them.
     ***********************************************************/
    FIXME("(%p) : stub\n", This);
    return WINED3D_OK;
}

static void updateSurfaceDesc(IWineD3DSurfaceImpl *surface, const WINED3DPRESENT_PARAMETERS* pPresentationParameters)
{
    IWineD3DDeviceImpl *This = surface->resource.wineD3DDevice; /* for GL_SUPPORT */

    /* Reallocate proper memory for the front and back buffer and adjust their sizes */
    if(surface->Flags & SFLAG_DIBSECTION) {
        /* Release the DC */
        SelectObject(surface->hDC, surface->dib.holdbitmap);
        DeleteDC(surface->hDC);
        /* Release the DIB section */
        DeleteObject(surface->dib.DIBsection);
        surface->dib.bitmap_data = NULL;
        surface->resource.allocatedMemory = NULL;
        surface->Flags &= ~SFLAG_DIBSECTION;
    }
    surface->currentDesc.Width = pPresentationParameters->BackBufferWidth;
    surface->currentDesc.Height = pPresentationParameters->BackBufferHeight;
    if (GL_SUPPORT(ARB_TEXTURE_NON_POWER_OF_TWO) || GL_SUPPORT(ARB_TEXTURE_RECTANGLE) ||
        GL_SUPPORT(WINE_NORMALIZED_TEXRECT)) {
        surface->pow2Width = pPresentationParameters->BackBufferWidth;
        surface->pow2Height = pPresentationParameters->BackBufferHeight;
    } else {
        surface->pow2Width = surface->pow2Height = 1;
        while (surface->pow2Width < pPresentationParameters->BackBufferWidth) surface->pow2Width <<= 1;
        while (surface->pow2Height < pPresentationParameters->BackBufferHeight) surface->pow2Height <<= 1;
    }
    surface->glRect.left = 0;
    surface->glRect.top = 0;
    surface->glRect.right = surface->pow2Width;
    surface->glRect.bottom = surface->pow2Height;

    if(surface->glDescription.textureName) {
        ActivateContext(This, This->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
        ENTER_GL();
        glDeleteTextures(1, &surface->glDescription.textureName);
        LEAVE_GL();
        surface->glDescription.textureName = 0;
        surface->Flags &= ~SFLAG_CLIENT;
    }
    if(surface->pow2Width != pPresentationParameters->BackBufferWidth ||
       surface->pow2Height != pPresentationParameters->BackBufferHeight) {
        surface->Flags |= SFLAG_NONPOW2;
    } else  {
        surface->Flags &= ~SFLAG_NONPOW2;
    }
    HeapFree(GetProcessHeap(), 0, surface->resource.heapMemory);
    surface->resource.allocatedMemory = NULL;
    surface->resource.heapMemory = NULL;
    surface->resource.size = IWineD3DSurface_GetPitch((IWineD3DSurface *) surface) * surface->pow2Width;
    /* INDRAWABLE is a sane place for implicit targets after the reset, INSYSMEM is more appropriate for depth stencils. */
    if (surface->resource.usage & WINED3DUSAGE_DEPTHSTENCIL) {
        IWineD3DSurface_ModifyLocation((IWineD3DSurface *) surface, SFLAG_INSYSMEM, TRUE);
    } else {
        IWineD3DSurface_ModifyLocation((IWineD3DSurface *) surface, SFLAG_INDRAWABLE, TRUE);
    }
}

static HRESULT WINAPI reset_unload_resources(IWineD3DResource *resource, void *data) {
    TRACE("Unloading resource %p\n", resource);
    IWineD3DResource_UnLoad(resource);
    IWineD3DResource_Release(resource);
    return S_OK;
}

static BOOL is_display_mode_supported(IWineD3DDeviceImpl *This, const WINED3DPRESENT_PARAMETERS *pp)
{
    UINT i, count;
    WINED3DDISPLAYMODE m;
    HRESULT hr;

    /* All Windowed modes are supported, as is leaving the current mode */
    if(pp->Windowed) return TRUE;
    if(!pp->BackBufferWidth) return TRUE;
    if(!pp->BackBufferHeight) return TRUE;

    count = IWineD3D_GetAdapterModeCount(This->wineD3D, This->adapter->num, WINED3DFMT_UNKNOWN);
    for(i = 0; i < count; i++) {
        memset(&m, 0, sizeof(m));
        hr = IWineD3D_EnumAdapterModes(This->wineD3D, This->adapter->num, WINED3DFMT_UNKNOWN, i, &m);
        if(FAILED(hr)) {
            ERR("EnumAdapterModes failed\n");
        }
        if(m.Width == pp->BackBufferWidth && m.Height == pp->BackBufferHeight) {
            /* Mode found, it is supported */
            return TRUE;
        }
    }
    /* Mode not found -> not supported */
    return FALSE;
}

void delete_opengl_contexts(IWineD3DDevice *iface, IWineD3DSwapChain *swapchain_iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    IWineD3DSwapChainImpl *swapchain = (IWineD3DSwapChainImpl *) swapchain_iface;
    UINT i;
    IWineD3DBaseShaderImpl *shader;

    IWineD3DDevice_EnumResources(iface, reset_unload_resources, NULL);
    LIST_FOR_EACH_ENTRY(shader, &This->shaders, IWineD3DBaseShaderImpl, baseShader.shader_list_entry) {
        This->shader_backend->shader_destroy((IWineD3DBaseShader *) shader);
    }

    ENTER_GL();
    if(This->depth_blt_texture) {
        glDeleteTextures(1, &This->depth_blt_texture);
        This->depth_blt_texture = 0;
    }
    if (This->depth_blt_rb) {
        GL_EXTCALL(glDeleteRenderbuffersEXT(1, &This->depth_blt_rb));
        This->depth_blt_rb = 0;
        This->depth_blt_rb_w = 0;
        This->depth_blt_rb_h = 0;
    }
    LEAVE_GL();

    This->blitter->free_private(iface);
    This->frag_pipe->free_private(iface);
    This->shader_backend->shader_free_private(iface);

    ENTER_GL();
    for (i = 0; i < GL_LIMITS(textures); i++) {
        /* Textures are recreated below */
        glDeleteTextures(1, &This->dummyTextureName[i]);
        checkGLcall("glDeleteTextures(1, &This->dummyTextureName[i])");
        This->dummyTextureName[i] = 0;
    }
    LEAVE_GL();

    while(This->numContexts) {
        DestroyContext(This, This->contexts[0]);
    }
    This->activeContext = NULL;
    HeapFree(GetProcessHeap(), 0, swapchain->context);
    swapchain->context = NULL;
    swapchain->num_contexts = 0;
}

HRESULT create_primary_opengl_context(IWineD3DDevice *iface, IWineD3DSwapChain *swapchain_iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    IWineD3DSwapChainImpl *swapchain = (IWineD3DSwapChainImpl *) swapchain_iface;
    HRESULT hr;
    IWineD3DSurfaceImpl *target;

    /* Recreate the primary swapchain's context */
    swapchain->context = HeapAlloc(GetProcessHeap(), 0, sizeof(*swapchain->context));
    if(swapchain->backBuffer) {
        target = (IWineD3DSurfaceImpl *) swapchain->backBuffer[0];
    } else {
        target = (IWineD3DSurfaceImpl *) swapchain->frontBuffer;
    }
    swapchain->context[0] = CreateContext(This, target, swapchain->win_handle, FALSE,
                                          &swapchain->presentParms);
    swapchain->num_contexts = 1;
    This->activeContext = swapchain->context[0];

    create_dummy_textures(This);

    hr = This->shader_backend->shader_alloc_private(iface);
    if(FAILED(hr)) {
        ERR("Failed to recreate shader private data\n");
        goto err_out;
    }
    hr = This->frag_pipe->alloc_private(iface);
    if(FAILED(hr)) {
        TRACE("Fragment pipeline private data couldn't be allocated\n");
        goto err_out;
    }
    hr = This->blitter->alloc_private(iface);
    if(FAILED(hr)) {
        TRACE("Blitter private data couldn't be allocated\n");
        goto err_out;
    }

    return WINED3D_OK;

err_out:
    This->blitter->free_private(iface);
    This->frag_pipe->free_private(iface);
    This->shader_backend->shader_free_private(iface);
    return hr;
}

static HRESULT WINAPI IWineD3DDeviceImpl_Reset(IWineD3DDevice* iface, WINED3DPRESENT_PARAMETERS* pPresentationParameters) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    IWineD3DSwapChainImpl *swapchain;
    HRESULT hr;
    BOOL DisplayModeChanged = FALSE;
    WINED3DDISPLAYMODE mode;
    TRACE("(%p)\n", This);

    hr = IWineD3DDevice_GetSwapChain(iface, 0, (IWineD3DSwapChain **) &swapchain);
    if(FAILED(hr)) {
        ERR("Failed to get the first implicit swapchain\n");
        return hr;
    }

    if(!is_display_mode_supported(This, pPresentationParameters)) {
        WARN("Rejecting Reset() call because the requested display mode is not supported\n");
        WARN("Requested mode: %d, %d\n", pPresentationParameters->BackBufferWidth,
             pPresentationParameters->BackBufferHeight);
        IWineD3DSwapChain_Release((IWineD3DSwapChain *)swapchain);
        return WINED3DERR_INVALIDCALL;
    }

    /* Is it necessary to recreate the gl context? Actually every setting can be changed
     * on an existing gl context, so there's no real need for recreation.
     *
     * TODO: Figure out how Reset influences resources in D3DPOOL_DEFAULT, D3DPOOL_SYSTEMMEMORY and D3DPOOL_MANAGED
     *
     * TODO: Figure out what happens to explicit swapchains, or if we have more than one implicit swapchain
     */
    TRACE("New params:\n");
    TRACE("BackBufferWidth = %d\n", pPresentationParameters->BackBufferWidth);
    TRACE("BackBufferHeight = %d\n", pPresentationParameters->BackBufferHeight);
    TRACE("BackBufferFormat = %s\n", debug_d3dformat(pPresentationParameters->BackBufferFormat));
    TRACE("BackBufferCount = %d\n", pPresentationParameters->BackBufferCount);
    TRACE("MultiSampleType = %d\n", pPresentationParameters->MultiSampleType);
    TRACE("MultiSampleQuality = %d\n", pPresentationParameters->MultiSampleQuality);
    TRACE("SwapEffect = %d\n", pPresentationParameters->SwapEffect);
    TRACE("hDeviceWindow = %p\n", pPresentationParameters->hDeviceWindow);
    TRACE("Windowed = %s\n", pPresentationParameters->Windowed ? "true" : "false");
    TRACE("EnableAutoDepthStencil = %s\n", pPresentationParameters->EnableAutoDepthStencil ? "true" : "false");
    TRACE("Flags = %08x\n", pPresentationParameters->Flags);
    TRACE("FullScreen_RefreshRateInHz = %d\n", pPresentationParameters->FullScreen_RefreshRateInHz);
    TRACE("PresentationInterval = %d\n", pPresentationParameters->PresentationInterval);

    /* No special treatment of these parameters. Just store them */
    swapchain->presentParms.SwapEffect = pPresentationParameters->SwapEffect;
    swapchain->presentParms.Flags = pPresentationParameters->Flags;
    swapchain->presentParms.PresentationInterval = pPresentationParameters->PresentationInterval;
    swapchain->presentParms.FullScreen_RefreshRateInHz = pPresentationParameters->FullScreen_RefreshRateInHz;

    /* What to do about these? */
    if(pPresentationParameters->BackBufferCount != 0 &&
        pPresentationParameters->BackBufferCount != swapchain->presentParms.BackBufferCount) {
        ERR("Cannot change the back buffer count yet\n");
    }
    if(pPresentationParameters->BackBufferFormat != WINED3DFMT_UNKNOWN &&
        pPresentationParameters->BackBufferFormat != swapchain->presentParms.BackBufferFormat) {
        ERR("Cannot change the back buffer format yet\n");
    }
    if(pPresentationParameters->hDeviceWindow != NULL &&
        pPresentationParameters->hDeviceWindow != swapchain->presentParms.hDeviceWindow) {
        ERR("Cannot change the device window yet\n");
    }
    if (pPresentationParameters->EnableAutoDepthStencil && !This->auto_depth_stencil_buffer) {
        WARN("Auto depth stencil enabled, but no auto depth stencil present, returning WINED3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* Reset the depth stencil */
    if (pPresentationParameters->EnableAutoDepthStencil)
        IWineD3DDevice_SetDepthStencilSurface(iface, This->auto_depth_stencil_buffer);
    else
        IWineD3DDevice_SetDepthStencilSurface(iface, NULL);

    delete_opengl_contexts(iface, (IWineD3DSwapChain *) swapchain);

    if(pPresentationParameters->Windowed) {
        mode.Width = swapchain->orig_width;
        mode.Height = swapchain->orig_height;
        mode.RefreshRate = 0;
        mode.Format = swapchain->presentParms.BackBufferFormat;
    } else {
        mode.Width = pPresentationParameters->BackBufferWidth;
        mode.Height = pPresentationParameters->BackBufferHeight;
        mode.RefreshRate = pPresentationParameters->FullScreen_RefreshRateInHz;
        mode.Format = swapchain->presentParms.BackBufferFormat;
    }

    /* Should Width == 800 && Height == 0 set 800x600? */
    if(pPresentationParameters->BackBufferWidth != 0 && pPresentationParameters->BackBufferHeight != 0 &&
       (pPresentationParameters->BackBufferWidth != swapchain->presentParms.BackBufferWidth ||
        pPresentationParameters->BackBufferHeight != swapchain->presentParms.BackBufferHeight))
    {
        UINT i;

        if(!pPresentationParameters->Windowed) {
            DisplayModeChanged = TRUE;
        }
        swapchain->presentParms.BackBufferWidth = pPresentationParameters->BackBufferWidth;
        swapchain->presentParms.BackBufferHeight = pPresentationParameters->BackBufferHeight;

        updateSurfaceDesc((IWineD3DSurfaceImpl *)swapchain->frontBuffer, pPresentationParameters);
        for(i = 0; i < swapchain->presentParms.BackBufferCount; i++) {
            updateSurfaceDesc((IWineD3DSurfaceImpl *)swapchain->backBuffer[i], pPresentationParameters);
        }
        if(This->auto_depth_stencil_buffer) {
            updateSurfaceDesc((IWineD3DSurfaceImpl *)This->auto_depth_stencil_buffer, pPresentationParameters);
        }
    }

    if((pPresentationParameters->Windowed && !swapchain->presentParms.Windowed) ||
       (swapchain->presentParms.Windowed && !pPresentationParameters->Windowed) ||
        DisplayModeChanged) {

        IWineD3DDevice_SetDisplayMode(iface, 0, &mode);

        if(swapchain->win_handle && !pPresentationParameters->Windowed) {
            if(swapchain->presentParms.Windowed) {
                /* switch from windowed to fs */
                IWineD3DDeviceImpl_SetupFullscreenWindow(iface, swapchain->win_handle,
                                                         pPresentationParameters->BackBufferWidth,
                                                         pPresentationParameters->BackBufferHeight);
            } else {
                /* Fullscreen -> fullscreen mode change */
                MoveWindow(swapchain->win_handle, 0, 0,
                           pPresentationParameters->BackBufferWidth, pPresentationParameters->BackBufferHeight,
                           TRUE);
            }
        } else if(swapchain->win_handle && !swapchain->presentParms.Windowed) {
            /* Fullscreen -> windowed switch */
            IWineD3DDeviceImpl_RestoreWindow(iface, swapchain->win_handle);
        }
        swapchain->presentParms.Windowed = pPresentationParameters->Windowed;
    } else if(!pPresentationParameters->Windowed) {
        DWORD style = This->style, exStyle = This->exStyle;
        /* If we're in fullscreen, and the mode wasn't changed, we have to get the window back into
         * the right position. Some applications(Battlefield 2, Guild Wars) move it and then call
         * Reset to clear up their mess. Guild Wars also loses the device during that.
         */
        This->style = 0;
        This->exStyle = 0;
        IWineD3DDeviceImpl_SetupFullscreenWindow(iface, swapchain->win_handle,
                                                 pPresentationParameters->BackBufferWidth,
                                                 pPresentationParameters->BackBufferHeight);
        This->style = style;
        This->exStyle = exStyle;
    }

    TRACE("Resetting stateblock\n");
    IWineD3DStateBlock_Release((IWineD3DStateBlock *)This->updateStateBlock);
    IWineD3DStateBlock_Release((IWineD3DStateBlock *)This->stateBlock);

    /* Note: No parent needed for initial internal stateblock */
    hr = IWineD3DDevice_CreateStateBlock(iface, WINED3DSBT_INIT, (IWineD3DStateBlock **)&This->stateBlock, NULL);
    if (FAILED(hr)) ERR("Resetting the stateblock failed with error 0x%08x\n", hr);
    else TRACE("Created stateblock %p\n", This->stateBlock);
    This->updateStateBlock = This->stateBlock;
    IWineD3DStateBlock_AddRef((IWineD3DStateBlock *)This->updateStateBlock);

    hr = IWineD3DStateBlock_InitStartupStateBlock((IWineD3DStateBlock *) This->stateBlock);
    if(FAILED(hr)) {
        ERR("Resetting the stateblock failed with error 0x%08x\n", hr);
    }

    hr = create_primary_opengl_context(iface, (IWineD3DSwapChain *) swapchain);
    IWineD3DSwapChain_Release((IWineD3DSwapChain *) swapchain);

    /* All done. There is no need to reload resources or shaders, this will happen automatically on the
     * first use
     */
    return hr;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetDialogBoxMode(IWineD3DDevice *iface, BOOL bEnableDialogs) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    /** FIXME: always true at the moment **/
    if(!bEnableDialogs) {
        FIXME("(%p) Dialogs cannot be disabled yet\n", This);
    }
    return WINED3D_OK;
}


static HRESULT  WINAPI  IWineD3DDeviceImpl_GetCreationParameters(IWineD3DDevice *iface, WINED3DDEVICE_CREATION_PARAMETERS *pParameters) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    TRACE("(%p) : pParameters %p\n", This, pParameters);

    *pParameters = This->createParms;
    return WINED3D_OK;
}

static void WINAPI IWineD3DDeviceImpl_SetGammaRamp(IWineD3DDevice * iface, UINT iSwapChain, DWORD Flags, CONST WINED3DGAMMARAMP* pRamp) {
    IWineD3DSwapChain *swapchain;

    TRACE("Relaying  to swapchain\n");

    if (IWineD3DDeviceImpl_GetSwapChain(iface, iSwapChain, &swapchain) == WINED3D_OK) {
        IWineD3DSwapChain_SetGammaRamp(swapchain, Flags, pRamp);
        IWineD3DSwapChain_Release(swapchain);
    }
    return;
}

static void WINAPI IWineD3DDeviceImpl_GetGammaRamp(IWineD3DDevice *iface, UINT iSwapChain, WINED3DGAMMARAMP* pRamp) {
    IWineD3DSwapChain *swapchain;

    TRACE("Relaying  to swapchain\n");

    if (IWineD3DDeviceImpl_GetSwapChain(iface, iSwapChain, &swapchain) == WINED3D_OK) {
        IWineD3DSwapChain_GetGammaRamp(swapchain, pRamp);
        IWineD3DSwapChain_Release(swapchain);
    }
    return;
}


/** ********************************************************
*   Notification functions
** ********************************************************/
/** This function must be called in the release of a resource when ref == 0,
* the contents of resource must still be correct,
* any handles to other resource held by the caller must be closed
* (e.g. a texture should release all held surfaces because telling the device that it's been released.)
 *****************************************************/
static void IWineD3DDeviceImpl_AddResource(IWineD3DDevice *iface, IWineD3DResource *resource){
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : Adding Resource %p\n", This, resource);
    list_add_head(&This->resources, &((IWineD3DResourceImpl *) resource)->resource.resource_list_entry);
}

static void IWineD3DDeviceImpl_RemoveResource(IWineD3DDevice *iface, IWineD3DResource *resource){
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : Removing resource %p\n", This, resource);

    list_remove(&((IWineD3DResourceImpl *) resource)->resource.resource_list_entry);
}


static void WINAPI IWineD3DDeviceImpl_ResourceReleased(IWineD3DDevice *iface, IWineD3DResource *resource){
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    WINED3DRESOURCETYPE type = IWineD3DResource_GetType(resource);
    int counter;

    TRACE("(%p) : resource %p\n", This, resource);

    context_resource_released(iface, resource, type);

    switch (type) {
        /* TODO: check front and back buffers, rendertargets etc..  possibly swapchains? */
        case WINED3DRTYPE_SURFACE: {
            unsigned int i;

            /* Cleanup any FBO attachments if d3d is enabled */
            if(This->d3d_initialized) {
                if((IWineD3DSurface *)resource == This->lastActiveRenderTarget) {
                    IWineD3DSwapChainImpl *swapchain = This->swapchains ? (IWineD3DSwapChainImpl *) This->swapchains[0] : NULL;

                    TRACE("Last active render target destroyed\n");
                    /* Find a replacement surface for the currently active back buffer. The context manager does not do NULL
                     * checks, so switch to a valid target as long as the currently set surface is still valid. Use the
                     * surface of the implicit swpchain. If that is the same as the destroyed surface the device is destroyed
                     * and the lastActiveRenderTarget member shouldn't matter
                     */
                    if(swapchain) {
                        if(swapchain->backBuffer && swapchain->backBuffer[0] != (IWineD3DSurface *)resource) {
                            TRACE("Activating primary back buffer\n");
                            ActivateContext(This, swapchain->backBuffer[0], CTXUSAGE_RESOURCELOAD);
                        } else if(!swapchain->backBuffer && swapchain->frontBuffer != (IWineD3DSurface *)resource) {
                            /* Single buffering environment */
                            TRACE("Activating primary front buffer\n");
                            ActivateContext(This, swapchain->frontBuffer, CTXUSAGE_RESOURCELOAD);
                        } else {
                            TRACE("Device is being destroyed, setting lastActiveRenderTarget = 0xdeadbabe\n");
                            /* Implicit render target destroyed, that means the device is being destroyed
                             * whatever we set here, it shouldn't matter
                             */
                            This->lastActiveRenderTarget = (IWineD3DSurface *) 0xdeadbabe;
                        }
                    } else {
                        /* May happen during ddraw uninitialization */
                        TRACE("Render target set, but swapchain does not exist!\n");
                        This->lastActiveRenderTarget = (IWineD3DSurface *) 0xdeadcafe;
                    }
                }

                for (i = 0; i < GL_LIMITS(buffers); ++i) {
                    if (This->render_targets[i] == (IWineD3DSurface *)resource) {
                        This->render_targets[i] = NULL;
                    }
                }
                if (This->stencilBufferTarget == (IWineD3DSurface *)resource) {
                    This->stencilBufferTarget = NULL;
                }
            }

            break;
        }
        case WINED3DRTYPE_TEXTURE:
        case WINED3DRTYPE_CUBETEXTURE:
        case WINED3DRTYPE_VOLUMETEXTURE:
                for (counter = 0; counter < MAX_COMBINED_SAMPLERS; counter++) {
                    if (This->stateBlock != NULL && This->stateBlock->textures[counter] == (IWineD3DBaseTexture *)resource) {
                        WARN("Texture being released is still by a stateblock, Stage = %u Texture = %p\n", counter, resource);
                        This->stateBlock->textures[counter] = NULL;
                    }
                    if (This->updateStateBlock != This->stateBlock ){
                        if (This->updateStateBlock->textures[counter] == (IWineD3DBaseTexture *)resource) {
                            WARN("Texture being released is still by a stateblock, Stage = %u Texture = %p\n", counter, resource);
                            This->updateStateBlock->textures[counter] = NULL;
                        }
                    }
                }
        break;
        case WINED3DRTYPE_VOLUME:
        /* TODO: nothing really? */
        break;
        case WINED3DRTYPE_VERTEXBUFFER:
        {
            int streamNumber;
            TRACE("Cleaning up stream pointers\n");

            for(streamNumber = 0; streamNumber < MAX_STREAMS; streamNumber ++){
                /* FINDOUT: should a warn be generated if were recording and updateStateBlock->streamSource is lost?
                FINDOUT: should changes.streamSource[StreamNumber] be set ?
                */
                if (This->updateStateBlock != NULL ) { /* ==NULL when device is being destroyed */
                    if ((IWineD3DResource *)This->updateStateBlock->streamSource[streamNumber] == resource) {
                        FIXME("Vertex buffer released while bound to a state block, stream %d\n", streamNumber);
                        This->updateStateBlock->streamSource[streamNumber] = 0;
                        /* Set changed flag? */
                    }
                }
                if (This->stateBlock != NULL ) { /* only happens if there is an error in the application, or on reset/release (because we don't manage internal tracking properly) */
                    if ((IWineD3DResource *)This->stateBlock->streamSource[streamNumber] == resource) {
                        TRACE("Vertex buffer released while bound to a state block, stream %d\n", streamNumber);
                        This->stateBlock->streamSource[streamNumber] = 0;
                    }
                }
            }
        }
        break;
        case WINED3DRTYPE_INDEXBUFFER:
        if (This->updateStateBlock != NULL ) { /* ==NULL when device is being destroyed */
            if (This->updateStateBlock->pIndexData == (IWineD3DIndexBuffer *)resource) {
                This->updateStateBlock->pIndexData =  NULL;
            }
        }
        if (This->stateBlock != NULL ) { /* ==NULL when device is being destroyed */
            if (This->stateBlock->pIndexData == (IWineD3DIndexBuffer *)resource) {
                This->stateBlock->pIndexData =  NULL;
            }
        }
        break;

        case WINED3DRTYPE_BUFFER:
            /* Nothing to do, yet.*/
            break;

        default:
        FIXME("(%p) unknown resource type %p %u\n", This, resource, IWineD3DResource_GetType(resource));
        break;
    }


    /* Remove the resource from the resourceStore */
    IWineD3DDeviceImpl_RemoveResource(iface, resource);

    TRACE("Resource released\n");

}

static HRESULT WINAPI IWineD3DDeviceImpl_EnumResources(IWineD3DDevice *iface, D3DCB_ENUMRESOURCES pCallback, void *pData) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    IWineD3DResourceImpl *resource, *cursor;
    HRESULT ret;
    TRACE("(%p)->(%p,%p)\n", This, pCallback, pData);

    LIST_FOR_EACH_ENTRY_SAFE(resource, cursor, &This->resources, IWineD3DResourceImpl, resource.resource_list_entry) {
        TRACE("enumerating resource %p\n", resource);
        IWineD3DResource_AddRef((IWineD3DResource *) resource);
        ret = pCallback((IWineD3DResource *) resource, pData);
        if(ret == S_FALSE) {
            TRACE("Canceling enumeration\n");
            break;
        }
    }
    return WINED3D_OK;
}

/**********************************************************
 * IWineD3DDevice VTbl follows
 **********************************************************/

const IWineD3DDeviceVtbl IWineD3DDevice_Vtbl =
{
    /*** IUnknown methods ***/
    IWineD3DDeviceImpl_QueryInterface,
    IWineD3DDeviceImpl_AddRef,
    IWineD3DDeviceImpl_Release,
    /*** IWineD3DDevice methods ***/
    IWineD3DDeviceImpl_GetParent,
    /*** Creation methods**/
    IWineD3DDeviceImpl_CreateBuffer,
    IWineD3DDeviceImpl_CreateVertexBuffer,
    IWineD3DDeviceImpl_CreateIndexBuffer,
    IWineD3DDeviceImpl_CreateStateBlock,
    IWineD3DDeviceImpl_CreateSurface,
    IWineD3DDeviceImpl_CreateRendertargetView,
    IWineD3DDeviceImpl_CreateTexture,
    IWineD3DDeviceImpl_CreateVolumeTexture,
    IWineD3DDeviceImpl_CreateVolume,
    IWineD3DDeviceImpl_CreateCubeTexture,
    IWineD3DDeviceImpl_CreateQuery,
    IWineD3DDeviceImpl_CreateSwapChain,
    IWineD3DDeviceImpl_CreateVertexDeclaration,
    IWineD3DDeviceImpl_CreateVertexDeclarationFromFVF,
    IWineD3DDeviceImpl_CreateVertexShader,
    IWineD3DDeviceImpl_CreatePixelShader,
    IWineD3DDeviceImpl_CreatePalette,
    /*** Odd functions **/
    IWineD3DDeviceImpl_Init3D,
    IWineD3DDeviceImpl_InitGDI,
    IWineD3DDeviceImpl_Uninit3D,
    IWineD3DDeviceImpl_UninitGDI,
    IWineD3DDeviceImpl_SetMultithreaded,
    IWineD3DDeviceImpl_EvictManagedResources,
    IWineD3DDeviceImpl_GetAvailableTextureMem,
    IWineD3DDeviceImpl_GetBackBuffer,
    IWineD3DDeviceImpl_GetCreationParameters,
    IWineD3DDeviceImpl_GetDeviceCaps,
    IWineD3DDeviceImpl_GetDirect3D,
    IWineD3DDeviceImpl_GetDisplayMode,
    IWineD3DDeviceImpl_SetDisplayMode,
    IWineD3DDeviceImpl_GetNumberOfSwapChains,
    IWineD3DDeviceImpl_GetRasterStatus,
    IWineD3DDeviceImpl_GetSwapChain,
    IWineD3DDeviceImpl_Reset,
    IWineD3DDeviceImpl_SetDialogBoxMode,
    IWineD3DDeviceImpl_SetCursorProperties,
    IWineD3DDeviceImpl_SetCursorPosition,
    IWineD3DDeviceImpl_ShowCursor,
    IWineD3DDeviceImpl_TestCooperativeLevel,
    /*** Getters and setters **/
    IWineD3DDeviceImpl_SetClipPlane,
    IWineD3DDeviceImpl_GetClipPlane,
    IWineD3DDeviceImpl_SetClipStatus,
    IWineD3DDeviceImpl_GetClipStatus,
    IWineD3DDeviceImpl_SetCurrentTexturePalette,
    IWineD3DDeviceImpl_GetCurrentTexturePalette,
    IWineD3DDeviceImpl_SetDepthStencilSurface,
    IWineD3DDeviceImpl_GetDepthStencilSurface,
    IWineD3DDeviceImpl_SetGammaRamp,
    IWineD3DDeviceImpl_GetGammaRamp,
    IWineD3DDeviceImpl_SetIndices,
    IWineD3DDeviceImpl_GetIndices,
    IWineD3DDeviceImpl_SetBaseVertexIndex,
    IWineD3DDeviceImpl_GetBaseVertexIndex,
    IWineD3DDeviceImpl_SetLight,
    IWineD3DDeviceImpl_GetLight,
    IWineD3DDeviceImpl_SetLightEnable,
    IWineD3DDeviceImpl_GetLightEnable,
    IWineD3DDeviceImpl_SetMaterial,
    IWineD3DDeviceImpl_GetMaterial,
    IWineD3DDeviceImpl_SetNPatchMode,
    IWineD3DDeviceImpl_GetNPatchMode,
    IWineD3DDeviceImpl_SetPaletteEntries,
    IWineD3DDeviceImpl_GetPaletteEntries,
    IWineD3DDeviceImpl_SetPixelShader,
    IWineD3DDeviceImpl_GetPixelShader,
    IWineD3DDeviceImpl_SetPixelShaderConstantB,
    IWineD3DDeviceImpl_GetPixelShaderConstantB,
    IWineD3DDeviceImpl_SetPixelShaderConstantI,
    IWineD3DDeviceImpl_GetPixelShaderConstantI,
    IWineD3DDeviceImpl_SetPixelShaderConstantF,
    IWineD3DDeviceImpl_GetPixelShaderConstantF,
    IWineD3DDeviceImpl_SetRenderState,
    IWineD3DDeviceImpl_GetRenderState,
    IWineD3DDeviceImpl_SetRenderTarget,
    IWineD3DDeviceImpl_GetRenderTarget,
    IWineD3DDeviceImpl_SetFrontBackBuffers,
    IWineD3DDeviceImpl_SetSamplerState,
    IWineD3DDeviceImpl_GetSamplerState,
    IWineD3DDeviceImpl_SetScissorRect,
    IWineD3DDeviceImpl_GetScissorRect,
    IWineD3DDeviceImpl_SetSoftwareVertexProcessing,
    IWineD3DDeviceImpl_GetSoftwareVertexProcessing,
    IWineD3DDeviceImpl_SetStreamSource,
    IWineD3DDeviceImpl_GetStreamSource,
    IWineD3DDeviceImpl_SetStreamSourceFreq,
    IWineD3DDeviceImpl_GetStreamSourceFreq,
    IWineD3DDeviceImpl_SetTexture,
    IWineD3DDeviceImpl_GetTexture,
    IWineD3DDeviceImpl_SetTextureStageState,
    IWineD3DDeviceImpl_GetTextureStageState,
    IWineD3DDeviceImpl_SetTransform,
    IWineD3DDeviceImpl_GetTransform,
    IWineD3DDeviceImpl_SetVertexDeclaration,
    IWineD3DDeviceImpl_GetVertexDeclaration,
    IWineD3DDeviceImpl_SetVertexShader,
    IWineD3DDeviceImpl_GetVertexShader,
    IWineD3DDeviceImpl_SetVertexShaderConstantB,
    IWineD3DDeviceImpl_GetVertexShaderConstantB,
    IWineD3DDeviceImpl_SetVertexShaderConstantI,
    IWineD3DDeviceImpl_GetVertexShaderConstantI,
    IWineD3DDeviceImpl_SetVertexShaderConstantF,
    IWineD3DDeviceImpl_GetVertexShaderConstantF,
    IWineD3DDeviceImpl_SetViewport,
    IWineD3DDeviceImpl_GetViewport,
    IWineD3DDeviceImpl_MultiplyTransform,
    IWineD3DDeviceImpl_ValidateDevice,
    IWineD3DDeviceImpl_ProcessVertices,
    /*** State block ***/
    IWineD3DDeviceImpl_BeginStateBlock,
    IWineD3DDeviceImpl_EndStateBlock,
    /*** Scene management ***/
    IWineD3DDeviceImpl_BeginScene,
    IWineD3DDeviceImpl_EndScene,
    IWineD3DDeviceImpl_Present,
    IWineD3DDeviceImpl_Clear,
    IWineD3DDeviceImpl_ClearRendertargetView,
    /*** Drawing ***/
    IWineD3DDeviceImpl_SetPrimitiveType,
    IWineD3DDeviceImpl_GetPrimitiveType,
    IWineD3DDeviceImpl_DrawPrimitive,
    IWineD3DDeviceImpl_DrawIndexedPrimitive,
    IWineD3DDeviceImpl_DrawPrimitiveUP,
    IWineD3DDeviceImpl_DrawIndexedPrimitiveUP,
    IWineD3DDeviceImpl_DrawPrimitiveStrided,
    IWineD3DDeviceImpl_DrawIndexedPrimitiveStrided,
    IWineD3DDeviceImpl_DrawRectPatch,
    IWineD3DDeviceImpl_DrawTriPatch,
    IWineD3DDeviceImpl_DeletePatch,
    IWineD3DDeviceImpl_ColorFill,
    IWineD3DDeviceImpl_UpdateTexture,
    IWineD3DDeviceImpl_UpdateSurface,
    IWineD3DDeviceImpl_GetFrontBufferData,
    /*** object tracking ***/
    IWineD3DDeviceImpl_ResourceReleased,
    IWineD3DDeviceImpl_EnumResources
};

const DWORD SavedPixelStates_R[NUM_SAVEDPIXELSTATES_R] = {
    WINED3DRS_ALPHABLENDENABLE   ,
    WINED3DRS_ALPHAFUNC          ,
    WINED3DRS_ALPHAREF           ,
    WINED3DRS_ALPHATESTENABLE    ,
    WINED3DRS_BLENDOP            ,
    WINED3DRS_COLORWRITEENABLE   ,
    WINED3DRS_DESTBLEND          ,
    WINED3DRS_DITHERENABLE       ,
    WINED3DRS_FILLMODE           ,
    WINED3DRS_FOGDENSITY         ,
    WINED3DRS_FOGEND             ,
    WINED3DRS_FOGSTART           ,
    WINED3DRS_LASTPIXEL          ,
    WINED3DRS_SHADEMODE          ,
    WINED3DRS_SRCBLEND           ,
    WINED3DRS_STENCILENABLE      ,
    WINED3DRS_STENCILFAIL        ,
    WINED3DRS_STENCILFUNC        ,
    WINED3DRS_STENCILMASK        ,
    WINED3DRS_STENCILPASS        ,
    WINED3DRS_STENCILREF         ,
    WINED3DRS_STENCILWRITEMASK   ,
    WINED3DRS_STENCILZFAIL       ,
    WINED3DRS_TEXTUREFACTOR      ,
    WINED3DRS_WRAP0              ,
    WINED3DRS_WRAP1              ,
    WINED3DRS_WRAP2              ,
    WINED3DRS_WRAP3              ,
    WINED3DRS_WRAP4              ,
    WINED3DRS_WRAP5              ,
    WINED3DRS_WRAP6              ,
    WINED3DRS_WRAP7              ,
    WINED3DRS_ZENABLE            ,
    WINED3DRS_ZFUNC              ,
    WINED3DRS_ZWRITEENABLE
};

const DWORD SavedPixelStates_T[NUM_SAVEDPIXELSTATES_T] = {
    WINED3DTSS_ALPHAARG0             ,
    WINED3DTSS_ALPHAARG1             ,
    WINED3DTSS_ALPHAARG2             ,
    WINED3DTSS_ALPHAOP               ,
    WINED3DTSS_BUMPENVLOFFSET        ,
    WINED3DTSS_BUMPENVLSCALE         ,
    WINED3DTSS_BUMPENVMAT00          ,
    WINED3DTSS_BUMPENVMAT01          ,
    WINED3DTSS_BUMPENVMAT10          ,
    WINED3DTSS_BUMPENVMAT11          ,
    WINED3DTSS_COLORARG0             ,
    WINED3DTSS_COLORARG1             ,
    WINED3DTSS_COLORARG2             ,
    WINED3DTSS_COLOROP               ,
    WINED3DTSS_RESULTARG             ,
    WINED3DTSS_TEXCOORDINDEX         ,
    WINED3DTSS_TEXTURETRANSFORMFLAGS
};

const DWORD SavedPixelStates_S[NUM_SAVEDPIXELSTATES_S] = {
    WINED3DSAMP_ADDRESSU         ,
    WINED3DSAMP_ADDRESSV         ,
    WINED3DSAMP_ADDRESSW         ,
    WINED3DSAMP_BORDERCOLOR      ,
    WINED3DSAMP_MAGFILTER        ,
    WINED3DSAMP_MINFILTER        ,
    WINED3DSAMP_MIPFILTER        ,
    WINED3DSAMP_MIPMAPLODBIAS    ,
    WINED3DSAMP_MAXMIPLEVEL      ,
    WINED3DSAMP_MAXANISOTROPY    ,
    WINED3DSAMP_SRGBTEXTURE      ,
    WINED3DSAMP_ELEMENTINDEX
};

const DWORD SavedVertexStates_R[NUM_SAVEDVERTEXSTATES_R] = {
    WINED3DRS_AMBIENT                       ,
    WINED3DRS_AMBIENTMATERIALSOURCE         ,
    WINED3DRS_CLIPPING                      ,
    WINED3DRS_CLIPPLANEENABLE               ,
    WINED3DRS_COLORVERTEX                   ,
    WINED3DRS_DIFFUSEMATERIALSOURCE         ,
    WINED3DRS_EMISSIVEMATERIALSOURCE        ,
    WINED3DRS_FOGDENSITY                    ,
    WINED3DRS_FOGEND                        ,
    WINED3DRS_FOGSTART                      ,
    WINED3DRS_FOGTABLEMODE                  ,
    WINED3DRS_FOGVERTEXMODE                 ,
    WINED3DRS_INDEXEDVERTEXBLENDENABLE      ,
    WINED3DRS_LIGHTING                      ,
    WINED3DRS_LOCALVIEWER                   ,
    WINED3DRS_MULTISAMPLEANTIALIAS          ,
    WINED3DRS_MULTISAMPLEMASK               ,
    WINED3DRS_NORMALIZENORMALS              ,
    WINED3DRS_PATCHEDGESTYLE                ,
    WINED3DRS_POINTSCALE_A                  ,
    WINED3DRS_POINTSCALE_B                  ,
    WINED3DRS_POINTSCALE_C                  ,
    WINED3DRS_POINTSCALEENABLE              ,
    WINED3DRS_POINTSIZE                     ,
    WINED3DRS_POINTSIZE_MAX                 ,
    WINED3DRS_POINTSIZE_MIN                 ,
    WINED3DRS_POINTSPRITEENABLE             ,
    WINED3DRS_RANGEFOGENABLE                ,
    WINED3DRS_SPECULARMATERIALSOURCE        ,
    WINED3DRS_TWEENFACTOR                   ,
    WINED3DRS_VERTEXBLEND                   ,
    WINED3DRS_CULLMODE                      ,
    WINED3DRS_FOGCOLOR
};

const DWORD SavedVertexStates_T[NUM_SAVEDVERTEXSTATES_T] = {
    WINED3DTSS_TEXCOORDINDEX         ,
    WINED3DTSS_TEXTURETRANSFORMFLAGS
};

const DWORD SavedVertexStates_S[NUM_SAVEDVERTEXSTATES_S] = {
    WINED3DSAMP_DMAPOFFSET
};

void IWineD3DDeviceImpl_MarkStateDirty(IWineD3DDeviceImpl *This, DWORD state) {
    DWORD rep = This->StateTable[state].representative;
    DWORD idx;
    BYTE shift;
    UINT i;
    WineD3DContext *context;

    if(!rep) return;
    for(i = 0; i < This->numContexts; i++) {
        context = This->contexts[i];
        if(isStateDirty(context, rep)) continue;

        context->dirtyArray[context->numDirtyEntries++] = rep;
        idx = rep >> 5;
        shift = rep & 0x1f;
        context->isStateDirty[idx] |= (1 << shift);
    }
}

void get_drawable_size_pbuffer(IWineD3DSurfaceImpl *This, UINT *width, UINT *height) {
    IWineD3DDeviceImpl *dev = This->resource.wineD3DDevice;
    /* The drawable size of a pbuffer render target is the current pbuffer size
     */
    *width = dev->pbufferWidth;
    *height = dev->pbufferHeight;
}

void get_drawable_size_fbo(IWineD3DSurfaceImpl *This, UINT *width, UINT *height) {
    /* The drawable size of a fbo target is the opengl texture size, which is the power of two size
     */
    *width = This->pow2Width;
    *height = This->pow2Height;
}

void get_drawable_size_backbuffer(IWineD3DSurfaceImpl *This, UINT *width, UINT *height) {
    IWineD3DDeviceImpl *dev = This->resource.wineD3DDevice;
    /* The drawable size of a backbuffer / aux buffer offscreen target is the size of the
     * current context's drawable, which is the size of the back buffer of the swapchain
     * the active context belongs to. The back buffer of the swapchain is stored as the
     * surface the context belongs to.
     */
    *width = ((IWineD3DSurfaceImpl *) dev->activeContext->surface)->currentDesc.Width;
    *height = ((IWineD3DSurfaceImpl *) dev->activeContext->surface)->currentDesc.Height;
}
