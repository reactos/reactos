/* Direct3D Vertex Buffer
 * Copyright (c) 2002 Lionel ULMER
 * Copyright (c) 2006 Stefan DÖSINGER
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

#include "ddraw_private.h"

static inline struct d3d_vertex_buffer *impl_from_IDirect3DVertexBuffer(IDirect3DVertexBuffer *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_vertex_buffer, IDirect3DVertexBuffer_iface);
}

static inline struct d3d_vertex_buffer *impl_from_IDirect3DVertexBuffer7(IDirect3DVertexBuffer7 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_vertex_buffer, IDirect3DVertexBuffer7_iface);
}

/*****************************************************************************
 * IUnknown Methods
 *****************************************************************************/

/*****************************************************************************
 * IDirect3DVertexBuffer7::QueryInterface
 *
 * The QueryInterface Method for Vertex Buffers
 * For a link to QueryInterface rules, see IDirectDraw7::QueryInterface
 *
 * Params
 *  riid: Queried Interface id
 *  obj: Address to return the interface pointer
 *
 * Returns:
 *  S_OK on success
 *  E_NOINTERFACE if the interface wasn't found
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_vertex_buffer7_QueryInterface(IDirect3DVertexBuffer7 *iface, REFIID riid, void  **obj)
{
    struct d3d_vertex_buffer *buffer = impl_from_IDirect3DVertexBuffer7(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), obj);

    /* By default, set the object pointer to NULL */
    *obj = NULL;

    if ( IsEqualGUID( &IID_IUnknown,  riid ) )
    {
        IDirect3DVertexBuffer7_AddRef(iface);
        *obj = iface;
        TRACE("  Creating IUnknown interface at %p.\n", *obj);
        return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DVertexBuffer, riid ) )
    {
        IDirect3DVertexBuffer7_AddRef(iface);
        *obj = &buffer->IDirect3DVertexBuffer_iface;
        TRACE("  Creating IDirect3DVertexBuffer interface %p\n", *obj);
        return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DVertexBuffer7, riid ) )
    {
        IDirect3DVertexBuffer7_AddRef(iface);
        *obj = iface;
        TRACE("  Creating IDirect3DVertexBuffer7 interface %p\n", *obj);
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static HRESULT WINAPI d3d_vertex_buffer1_QueryInterface(IDirect3DVertexBuffer *iface, REFIID riid, void **obj)
{
    struct d3d_vertex_buffer *buffer = impl_from_IDirect3DVertexBuffer(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), obj);

    return d3d_vertex_buffer7_QueryInterface(&buffer->IDirect3DVertexBuffer7_iface, riid, obj);
}

/*****************************************************************************
 * IDirect3DVertexBuffer7::AddRef
 *
 * AddRef for Vertex Buffers
 *
 * Returns:
 *  The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI d3d_vertex_buffer7_AddRef(IDirect3DVertexBuffer7 *iface)
{
    struct d3d_vertex_buffer *buffer = impl_from_IDirect3DVertexBuffer7(iface);
    ULONG ref = InterlockedIncrement(&buffer->ref);

    TRACE("%p increasing refcount to %u.\n", buffer, ref);

    return ref;
}

static ULONG WINAPI d3d_vertex_buffer1_AddRef(IDirect3DVertexBuffer *iface)
{
    struct d3d_vertex_buffer *buffer = impl_from_IDirect3DVertexBuffer(iface);

    TRACE("iface %p.\n", iface);

    return d3d_vertex_buffer7_AddRef(&buffer->IDirect3DVertexBuffer7_iface);
}


/*****************************************************************************
 * IDirect3DVertexBuffer7::Release
 *
 * Release for Vertex Buffers
 *
 * Returns:
 *  The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI d3d_vertex_buffer7_Release(IDirect3DVertexBuffer7 *iface)
{
    struct d3d_vertex_buffer *buffer = impl_from_IDirect3DVertexBuffer7(iface);
    ULONG ref = InterlockedDecrement(&buffer->ref);

    TRACE("%p decreasing refcount to %u.\n", buffer, ref);

    if (ref == 0)
    {
        struct wined3d_buffer *curVB = NULL;
        UINT offset, stride;

        /* D3D7 Vertex buffers don't stay bound in the device, they are passed
         * as a parameter to drawPrimitiveVB. DrawPrimitiveVB sets them as the
         * stream source in wined3d, and they should get unset there before
         * they are destroyed. */
        wined3d_mutex_lock();
        wined3d_device_get_stream_source(buffer->ddraw->wined3d_device,
                0, &curVB, &offset, &stride);
        if (curVB == buffer->wineD3DVertexBuffer)
            wined3d_device_set_stream_source(buffer->ddraw->wined3d_device, 0, NULL, 0, 0);

        wined3d_vertex_declaration_decref(buffer->wineD3DVertexDeclaration);
        wined3d_buffer_decref(buffer->wineD3DVertexBuffer);
        wined3d_mutex_unlock();

        HeapFree(GetProcessHeap(), 0, buffer);
    }

    return ref;
}

static ULONG WINAPI d3d_vertex_buffer1_Release(IDirect3DVertexBuffer *iface)
{
    struct d3d_vertex_buffer *buffer = impl_from_IDirect3DVertexBuffer(iface);

    TRACE("iface %p.\n", iface);

    return d3d_vertex_buffer7_Release(&buffer->IDirect3DVertexBuffer7_iface);
}

/*****************************************************************************
 * IDirect3DVertexBuffer Methods
 *****************************************************************************/

static HRESULT d3d_vertex_buffer_create_wined3d_buffer(struct d3d_vertex_buffer *buffer, BOOL dynamic,
        struct wined3d_buffer **wined3d_buffer)
{
    DWORD usage = WINED3DUSAGE_STATICDECL;
    enum wined3d_pool pool;

    if (buffer->Caps & D3DVBCAPS_SYSTEMMEMORY)
        pool = WINED3D_POOL_SYSTEM_MEM;
    else
        pool = WINED3D_POOL_DEFAULT;

    if (buffer->Caps & D3DVBCAPS_WRITEONLY)
        usage |= WINED3DUSAGE_WRITEONLY;
    if (dynamic)
        usage |= WINED3DUSAGE_DYNAMIC;

    return wined3d_buffer_create_vb(buffer->ddraw->wined3d_device,
        buffer->size, usage, pool, buffer, &ddraw_null_wined3d_parent_ops,
        wined3d_buffer);
}

/*****************************************************************************
 * IDirect3DVertexBuffer7::Lock
 *
 * Locks the vertex buffer and returns a pointer to the vertex data
 * Locking vertex buffers is similar to locking surfaces, because Windows
 * uses surfaces to store vertex data internally (According to the DX sdk)
 *
 * Params:
 *  Flags: Locking flags. Relevant here are DDLOCK_READONLY, DDLOCK_WRITEONLY,
 *         DDLOCK_DISCARDCONTENTS and DDLOCK_NOOVERWRITE.
 *  Data:  Returns a pointer to the vertex data
 *  Size:  Returns the size of the buffer if not NULL
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Data is NULL
 *  D3DERR_VERTEXBUFFEROPTIMIZED if called on an optimized buffer(WineD3D)
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_vertex_buffer7_Lock(IDirect3DVertexBuffer7 *iface,
        DWORD flags, void **data, DWORD *data_size)
{
    struct d3d_vertex_buffer *buffer = impl_from_IDirect3DVertexBuffer7(iface);
    struct wined3d_resource_desc wined3d_desc;
    struct wined3d_resource *wined3d_resource;
    HRESULT hr;
    DWORD wined3d_flags = 0;

    TRACE("iface %p, flags %#x, data %p, data_size %p.\n", iface, flags, data, data_size);

    /* Writeonly: Pointless. Event: Unsupported by native according to the sdk
     * nosyslock: Not applicable
     */
    if (!(flags & DDLOCK_WAIT))
        wined3d_flags |= WINED3D_MAP_DONOTWAIT;
    if (flags & DDLOCK_READONLY)
        wined3d_flags |= WINED3D_MAP_READONLY;
    if (flags & DDLOCK_NOOVERWRITE)
        wined3d_flags |= WINED3D_MAP_NOOVERWRITE;
    if (flags & DDLOCK_DISCARDCONTENTS)
    {
        wined3d_flags |= WINED3D_MAP_DISCARD;

        if (!buffer->dynamic)
        {
            struct wined3d_buffer *new_buffer;
            wined3d_mutex_lock();
            hr = d3d_vertex_buffer_create_wined3d_buffer(buffer, TRUE, &new_buffer);
            if (SUCCEEDED(hr))
            {
                buffer->dynamic = TRUE;
                wined3d_buffer_decref(buffer->wineD3DVertexBuffer);
                buffer->wineD3DVertexBuffer = new_buffer;
            }
            else
            {
                WARN("Failed to create a dynamic buffer\n");
            }
            wined3d_mutex_unlock();
        }
    }

    wined3d_mutex_lock();
    if (data_size)
    {
        /* Get the size, for returning it, and for locking */
        wined3d_resource = wined3d_buffer_get_resource(buffer->wineD3DVertexBuffer);
        wined3d_resource_get_desc(wined3d_resource, &wined3d_desc);
        *data_size = wined3d_desc.size;
    }

    hr = wined3d_buffer_map(buffer->wineD3DVertexBuffer, 0, 0, (BYTE **)data, wined3d_flags);

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d_vertex_buffer1_Lock(IDirect3DVertexBuffer *iface,
        DWORD flags, void **data, DWORD *data_size)
{
    struct d3d_vertex_buffer *buffer = impl_from_IDirect3DVertexBuffer(iface);

    TRACE("iface %p, flags %#x, data %p, data_size %p.\n", iface, flags, data, data_size);

    return d3d_vertex_buffer7_Lock(&buffer->IDirect3DVertexBuffer7_iface, flags, data, data_size);
}

/*****************************************************************************
 * IDirect3DVertexBuffer7::Unlock
 *
 * Unlocks a vertex Buffer
 *
 * Returns:
 *  D3D_OK on success
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_vertex_buffer7_Unlock(IDirect3DVertexBuffer7 *iface)
{
    struct d3d_vertex_buffer *buffer = impl_from_IDirect3DVertexBuffer7(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_buffer_unmap(buffer->wineD3DVertexBuffer);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_vertex_buffer1_Unlock(IDirect3DVertexBuffer *iface)
{
    struct d3d_vertex_buffer *buffer = impl_from_IDirect3DVertexBuffer(iface);

    TRACE("iface %p.\n", iface);

    return d3d_vertex_buffer7_Unlock(&buffer->IDirect3DVertexBuffer7_iface);
}


/*****************************************************************************
 * IDirect3DVertexBuffer7::ProcessVertices
 *
 * Processes untransformed Vertices into a transformed or optimized vertex
 * buffer. It can also perform other operations, such as lighting or clipping
 *
 * Params
 *  VertexOp: Operation(s) to perform: D3DVOP_CLIP, _EXTENTS, _LIGHT, _TRANSFORM
 *  DestIndex: Index in the destination buffer(This), where the vertices are
 *             placed
 *  Count: Number of Vertices in the Source buffer to process
 *  SrcBuffer: Source vertex buffer
 *  SrcIndex: Index of the first vertex in the src buffer to process
 *  D3DDevice: Device to use for transformation
 *  Flags: 0 for default, D3DPV_DONOTCOPYDATA to prevent copying
 *         unchaned vertices
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS If D3DVOP_TRANSFORM wasn't passed
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_vertex_buffer7_ProcessVertices(IDirect3DVertexBuffer7 *iface,
        DWORD vertex_op, DWORD dst_idx, DWORD count, IDirect3DVertexBuffer7 *src_buffer,
        DWORD src_idx, IDirect3DDevice7 *device, DWORD flags)
{
    struct d3d_vertex_buffer *dst_buffer_impl = impl_from_IDirect3DVertexBuffer7(iface);
    struct d3d_vertex_buffer *src_buffer_impl = unsafe_impl_from_IDirect3DVertexBuffer7(src_buffer);
    struct d3d_device *device_impl = unsafe_impl_from_IDirect3DDevice7(device);
    BOOL oldClip, doClip;
    HRESULT hr;

    TRACE("iface %p, vertex_op %#x, dst_idx %u, count %u, src_buffer %p, src_idx %u, device %p, flags %#x.\n",
            iface, vertex_op, dst_idx, count, src_buffer, src_idx, device, flags);

    /* Vertex operations:
     * D3DVOP_CLIP: Clips vertices outside the viewing frustrum. Needs clipping information
     * in the vertex buffer (Buffer may not be created with D3DVBCAPS_DONOTCLIP)
     * D3DVOP_EXTENTS: Causes the screen extents to be updated when rendering the vertices
     * D3DVOP_LIGHT: Lights the vertices
     * D3DVOP_TRANSFORM: Transform the vertices. This flag is necessary
     *
     * WineD3D only transforms and clips the vertices by now, so EXTENTS and LIGHT
     * are not implemented. Clipping is disabled ATM, because of unsure conditions.
     */
    if (!(vertex_op & D3DVOP_TRANSFORM))
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();

    /* WineD3D doesn't know d3d7 vertex operation, it uses
     * render states instead. Set the render states according to
     * the vertex ops
     */
    doClip = !!(vertex_op & D3DVOP_CLIP);
    oldClip = wined3d_device_get_render_state(device_impl->wined3d_device, WINED3D_RS_CLIPPING);
    if (doClip != oldClip)
        wined3d_device_set_render_state(device_impl->wined3d_device, WINED3D_RS_CLIPPING, doClip);

    wined3d_device_set_stream_source(device_impl->wined3d_device,
            0, src_buffer_impl->wineD3DVertexBuffer, 0, get_flexible_vertex_size(src_buffer_impl->fvf));
    wined3d_device_set_vertex_declaration(device_impl->wined3d_device, src_buffer_impl->wineD3DVertexDeclaration);
    hr = wined3d_device_process_vertices(device_impl->wined3d_device, src_idx, dst_idx,
            count, dst_buffer_impl->wineD3DVertexBuffer, NULL, flags, dst_buffer_impl->fvf);

    /* Restore the states if needed */
    if (doClip != oldClip)
        wined3d_device_set_render_state(device_impl->wined3d_device, WINED3D_RS_CLIPPING, oldClip);

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d_vertex_buffer1_ProcessVertices(IDirect3DVertexBuffer *iface,
        DWORD vertex_op, DWORD dst_idx, DWORD count, IDirect3DVertexBuffer *src_buffer,
        DWORD src_idx, IDirect3DDevice3 *device, DWORD flags)
{
    struct d3d_vertex_buffer *dst_buffer_impl = impl_from_IDirect3DVertexBuffer(iface);
    struct d3d_vertex_buffer *src_buffer_impl = unsafe_impl_from_IDirect3DVertexBuffer(src_buffer);
    struct d3d_device *device_impl = unsafe_impl_from_IDirect3DDevice3(device);

    TRACE("iface %p, vertex_op %#x, dst_idx %u, count %u, src_buffer %p, src_idx %u, device %p, flags %#x.\n",
            iface, vertex_op, dst_idx, count, src_buffer, src_idx, device, flags);

    return d3d_vertex_buffer7_ProcessVertices(&dst_buffer_impl->IDirect3DVertexBuffer7_iface, vertex_op,
            dst_idx, count, &src_buffer_impl->IDirect3DVertexBuffer7_iface, src_idx,
            device_impl ? &device_impl->IDirect3DDevice7_iface : NULL, flags);
}

/*****************************************************************************
 * IDirect3DVertexBuffer7::GetVertexBufferDesc
 *
 * Returns the description of a vertex buffer
 *
 * Params:
 *  Desc: Address to write the description to
 *
 * Returns
 *  DDERR_INVALIDPARAMS if Desc is NULL
 *  D3D_OK on success
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_vertex_buffer7_GetVertexBufferDesc(IDirect3DVertexBuffer7 *iface, D3DVERTEXBUFFERDESC *desc)
{
    struct d3d_vertex_buffer *buffer = impl_from_IDirect3DVertexBuffer7(iface);
    struct wined3d_resource_desc wined3d_desc;
    struct wined3d_resource *wined3d_resource;

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc) return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    wined3d_resource = wined3d_buffer_get_resource(buffer->wineD3DVertexBuffer);
    wined3d_resource_get_desc(wined3d_resource, &wined3d_desc);
    wined3d_mutex_unlock();

    /* Now fill the desc structure */
    desc->dwCaps = buffer->Caps;
    desc->dwFVF = buffer->fvf;
    desc->dwNumVertices = wined3d_desc.size / get_flexible_vertex_size(buffer->fvf);

    return D3D_OK;
}

static HRESULT WINAPI d3d_vertex_buffer1_GetVertexBufferDesc(IDirect3DVertexBuffer *iface, D3DVERTEXBUFFERDESC *desc)
{
    struct d3d_vertex_buffer *buffer = impl_from_IDirect3DVertexBuffer(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    return d3d_vertex_buffer7_GetVertexBufferDesc(&buffer->IDirect3DVertexBuffer7_iface, desc);
}


/*****************************************************************************
 * IDirect3DVertexBuffer7::Optimize
 *
 * Converts an unoptimized vertex buffer into an optimized buffer
 *
 * Params:
 *  D3DDevice: Device for which this buffer is optimized
 *  Flags: Not used, should be set to 0
 *
 * Returns
 *  D3D_OK, because it's a stub
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_vertex_buffer7_Optimize(IDirect3DVertexBuffer7 *iface,
        IDirect3DDevice7 *device, DWORD flags)
{
    struct d3d_vertex_buffer *buffer = impl_from_IDirect3DVertexBuffer7(iface);
    static BOOL hide = FALSE;

    TRACE("iface %p, device %p, flags %#x.\n", iface, device, flags);

    if (!hide)
    {
        FIXME("iface %p, device %p, flags %#x stub!\n", iface, device, flags);
        hide = TRUE;
    }

    /* We could forward this call to WineD3D and take advantage
     * of it once we use OpenGL vertex buffers
     */
    wined3d_mutex_lock();
    buffer->Caps |= D3DVBCAPS_OPTIMIZED;
    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI d3d_vertex_buffer1_Optimize(IDirect3DVertexBuffer *iface,
        IDirect3DDevice3 *device, DWORD flags)
{
    struct d3d_vertex_buffer *buffer = impl_from_IDirect3DVertexBuffer(iface);
    struct d3d_device *device_impl = unsafe_impl_from_IDirect3DDevice3(device);

    TRACE("iface %p, device %p, flags %#x.\n", iface, device, flags);

    return d3d_vertex_buffer7_Optimize(&buffer->IDirect3DVertexBuffer7_iface,
            device_impl ? &device_impl->IDirect3DDevice7_iface : NULL, flags);
}

/*****************************************************************************
 * IDirect3DVertexBuffer7::ProcessVerticesStrided
 *
 * This method processes untransformed strided vertices into a processed
 * or optimized vertex buffer.
 *
 * For more details on the parameters, see
 * IDirect3DVertexBuffer7::ProcessVertices
 *
 * Params:
 *  VertexOp: Operations to perform
 *  DestIndex: Destination index to write the vertices to
 *  Count: Number of input vertices
 *  StrideData: Array containing the input vertices
 *  VertexTypeDesc: Vertex Description or source index?????????
 *  D3DDevice: IDirect3DDevice7 to use for processing
 *  Flags: Can be D3DPV_DONOTCOPYDATA to avoid copying unmodified vertices
 *
 * Returns
 *  D3D_OK on success, or DDERR_*
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_vertex_buffer7_ProcessVerticesStrided(IDirect3DVertexBuffer7 *iface,
        DWORD vertex_op, DWORD dst_idx, DWORD count, D3DDRAWPRIMITIVESTRIDEDDATA *data,
        DWORD fvf, IDirect3DDevice7 *device, DWORD flags)
{
    FIXME("iface %p, vertex_op %#x, dst_idx %u, count %u, data %p, fvf %#x, device %p, flags %#x stub!\n",
            iface, vertex_op, dst_idx, count, data, fvf, device, flags);

    return DD_OK;
}

/*****************************************************************************
 * The VTables
 *****************************************************************************/

static const struct IDirect3DVertexBuffer7Vtbl d3d_vertex_buffer7_vtbl =
{
    d3d_vertex_buffer7_QueryInterface,
    d3d_vertex_buffer7_AddRef,
    d3d_vertex_buffer7_Release,
    d3d_vertex_buffer7_Lock,
    d3d_vertex_buffer7_Unlock,
    d3d_vertex_buffer7_ProcessVertices,
    d3d_vertex_buffer7_GetVertexBufferDesc,
    d3d_vertex_buffer7_Optimize,
    d3d_vertex_buffer7_ProcessVerticesStrided,
};

static const struct IDirect3DVertexBufferVtbl d3d_vertex_buffer1_vtbl =
{
    d3d_vertex_buffer1_QueryInterface,
    d3d_vertex_buffer1_AddRef,
    d3d_vertex_buffer1_Release,
    d3d_vertex_buffer1_Lock,
    d3d_vertex_buffer1_Unlock,
    d3d_vertex_buffer1_ProcessVertices,
    d3d_vertex_buffer1_GetVertexBufferDesc,
    d3d_vertex_buffer1_Optimize,
};

HRESULT d3d_vertex_buffer_create(struct d3d_vertex_buffer **vertex_buf,
        struct ddraw *ddraw, D3DVERTEXBUFFERDESC *desc)
{
    struct d3d_vertex_buffer *buffer;
    HRESULT hr = D3D_OK;

    TRACE("Vertex buffer description:\n");
    TRACE("    dwSize %u\n", desc->dwSize);
    TRACE("    dwCaps %#x\n", desc->dwCaps);
    TRACE("    FVF %#x\n", desc->dwFVF);
    TRACE("    dwNumVertices %u\n", desc->dwNumVertices);

    buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*buffer));
    if (!buffer)
        return DDERR_OUTOFMEMORY;

    buffer->IDirect3DVertexBuffer7_iface.lpVtbl = &d3d_vertex_buffer7_vtbl;
    buffer->IDirect3DVertexBuffer_iface.lpVtbl = &d3d_vertex_buffer1_vtbl;
    buffer->ref = 1;

    buffer->ddraw = ddraw;
    buffer->Caps = desc->dwCaps;
    buffer->fvf = desc->dwFVF;
    buffer->size = get_flexible_vertex_size(desc->dwFVF) * desc->dwNumVertices;

    wined3d_mutex_lock();

    hr = d3d_vertex_buffer_create_wined3d_buffer(buffer, FALSE, &buffer->wineD3DVertexBuffer);
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d vertex buffer, hr %#x.\n", hr);
        if (hr == WINED3DERR_INVALIDCALL)
            hr = DDERR_INVALIDPARAMS;
        goto end;
    }

    buffer->wineD3DVertexDeclaration = ddraw_find_decl(ddraw, desc->dwFVF);
    if (!buffer->wineD3DVertexDeclaration)
    {
        ERR("Failed to find vertex declaration for fvf %#x.\n", desc->dwFVF);
        wined3d_buffer_decref(buffer->wineD3DVertexBuffer);
        hr = DDERR_INVALIDPARAMS;
        goto end;
    }
    wined3d_vertex_declaration_incref(buffer->wineD3DVertexDeclaration);

end:
    wined3d_mutex_unlock();
    if (hr == D3D_OK)
        *vertex_buf = buffer;
    else
        HeapFree(GetProcessHeap(), 0, buffer);

    return hr;
}

struct d3d_vertex_buffer *unsafe_impl_from_IDirect3DVertexBuffer(IDirect3DVertexBuffer *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d_vertex_buffer1_vtbl);

    return impl_from_IDirect3DVertexBuffer(iface);
}

struct d3d_vertex_buffer *unsafe_impl_from_IDirect3DVertexBuffer7(IDirect3DVertexBuffer7 *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d_vertex_buffer7_vtbl);

    return impl_from_IDirect3DVertexBuffer7(iface);
}
