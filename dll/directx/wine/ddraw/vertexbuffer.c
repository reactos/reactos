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

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

static inline struct d3d_vertex_buffer *impl_from_IDirect3DVertexBuffer7(IDirect3DVertexBuffer7 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_vertex_buffer, IDirect3DVertexBuffer7_iface);
}

/*****************************************************************************
 * IUnknown Methods
 *****************************************************************************/

static HRESULT WINAPI d3d_vertex_buffer7_QueryInterface(IDirect3DVertexBuffer7 *iface, REFIID riid, void  **obj)
{
    struct d3d_vertex_buffer *buffer = impl_from_IDirect3DVertexBuffer7(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualGUID(&IID_IUnknown, riid))
    {
        IDirect3DVertexBuffer7_AddRef(iface);
        *obj = iface;
        return S_OK;
    }
    if (IsEqualGUID(&IID_IDirect3DVertexBuffer7, riid) && buffer->version == 7)
    {
        IDirect3DVertexBuffer7_AddRef(iface);
        *obj = iface;
        return S_OK;
    }
    if (IsEqualGUID(&IID_IDirect3DVertexBuffer, riid) && buffer->version == 3)
    {
        IDirect3DVertexBuffer7_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d_vertex_buffer7_AddRef(IDirect3DVertexBuffer7 *iface)
{
    struct d3d_vertex_buffer *buffer = impl_from_IDirect3DVertexBuffer7(iface);
    ULONG ref = InterlockedIncrement(&buffer->ref);

    TRACE("%p increasing refcount to %lu.\n", buffer, ref);

    return ref;
}

static ULONG WINAPI d3d_vertex_buffer7_Release(IDirect3DVertexBuffer7 *iface)
{
    struct d3d_vertex_buffer *buffer = impl_from_IDirect3DVertexBuffer7(iface);
    ULONG ref = InterlockedDecrement(&buffer->ref);
    struct d3d_device *device;

    TRACE("%p decreasing refcount to %lu.\n", buffer, ref);

    if (!ref)
    {
        /* D3D7 vertex buffers don't stay bound in the device, they are passed
         * as a parameter to DrawPrimitiveVB. DrawPrimitiveVB sets them as the
         * stream source in wined3d and they should get unset there before
         * they are destroyed. */
        wined3d_mutex_lock();

        LIST_FOR_EACH_ENTRY(device, &buffer->ddraw->d3ddevice_list, struct d3d_device, ddraw_entry)
        {
            if (device->stateblock_state->streams[0].buffer == buffer->wined3d_buffer)
                wined3d_stateblock_set_stream_source(device->state, 0, NULL, 0, 0);
        }

        wined3d_vertex_declaration_decref(buffer->wined3d_declaration);
        wined3d_buffer_decref(buffer->wined3d_buffer);
        wined3d_mutex_unlock();

        if (buffer->version == 7)
            IDirectDraw7_Release(&buffer->ddraw->IDirectDraw7_iface);

        free(buffer);
    }

    return ref;
}

/*****************************************************************************
 * IDirect3DVertexBuffer Methods
 *****************************************************************************/

static HRESULT d3d_vertex_buffer_create_wined3d_buffer(struct d3d_vertex_buffer *buffer, BOOL dynamic,
        struct wined3d_buffer **wined3d_buffer)
{
    struct wined3d_buffer_desc desc;

    desc.byte_width = buffer->size;
    desc.usage = WINED3DUSAGE_STATICDECL | WINED3DUSAGE_VIDMEM_ACCOUNTING;
    if (dynamic)
        desc.usage |= WINED3DUSAGE_DYNAMIC;
    desc.bind_flags = WINED3D_BIND_VERTEX_BUFFER;
    if (buffer->sysmem)
        desc.access = WINED3D_RESOURCE_ACCESS_CPU | WINED3D_RESOURCE_ACCESS_MAP_R | WINED3D_RESOURCE_ACCESS_MAP_W;
    else
        desc.access = WINED3D_RESOURCE_ACCESS_GPU | WINED3D_RESOURCE_ACCESS_MAP_R | WINED3D_RESOURCE_ACCESS_MAP_W;
    desc.misc_flags = 0;
    desc.structure_byte_stride = 0;

    return wined3d_buffer_create(buffer->ddraw->wined3d_device, &desc,
            NULL, buffer, &ddraw_null_wined3d_parent_ops, wined3d_buffer);
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
    struct wined3d_map_desc wined3d_map_desc;
    HRESULT hr;

    TRACE("iface %p, flags %#lx, data %p, data_size %p.\n", iface, flags, data, data_size);

    if (buffer->version != 7)
        flags &= ~(DDLOCK_NOOVERWRITE | DDLOCK_DISCARDCONTENTS);

    if (buffer->discarded)
        flags &= ~DDLOCK_DISCARDCONTENTS;

    if (!(flags & DDLOCK_WAIT))
        flags |= DDLOCK_DONOTWAIT;
    if (flags & DDLOCK_DISCARDCONTENTS)
    {
        buffer->discarded = true;
        if (!buffer->dynamic)
        {
            struct wined3d_buffer *new_buffer;
            wined3d_mutex_lock();
            hr = d3d_vertex_buffer_create_wined3d_buffer(buffer, TRUE, &new_buffer);
            if (SUCCEEDED(hr))
            {
                buffer->dynamic = TRUE;
                wined3d_buffer_decref(buffer->wined3d_buffer);
                buffer->wined3d_buffer = new_buffer;
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
        wined3d_resource = wined3d_buffer_get_resource(buffer->wined3d_buffer);
        wined3d_resource_get_desc(wined3d_resource, &wined3d_desc);
        *data_size = wined3d_desc.size;
    }

    hr = wined3d_resource_map(wined3d_buffer_get_resource(buffer->wined3d_buffer),
            0, &wined3d_map_desc, NULL, wined3dmapflags_from_ddrawmapflags(flags));
    *data = wined3d_map_desc.data;

    wined3d_mutex_unlock();

    return hr;
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
    wined3d_resource_unmap(wined3d_buffer_get_resource(buffer->wined3d_buffer), 0);
    return D3D_OK;
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
 *         unchanged vertices
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
    struct d3d_device *device_impl = dst_buffer_impl->version == 7
            ? unsafe_impl_from_IDirect3DDevice7(device)
            : unsafe_impl_from_IDirect3DDevice3((IDirect3DDevice3 *)device);
    BOOL old_clip, do_clip, old_lighting, do_lighting;
    const struct wined3d_stateblock_state *state;
    HRESULT hr;

    TRACE("iface %p, vertex_op %#lx, dst_idx %lu, count %lu, src_buffer %p, src_idx %lu, device %p, flags %#lx.\n",
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

    state = device_impl->stateblock_state;

    /* WineD3D doesn't know d3d7 vertex operation, it uses
     * render states instead. Set the render states according to
     * the vertex ops
     */
    do_clip = !!(vertex_op & D3DVOP_CLIP);
    old_clip = !!state->rs[WINED3D_RS_CLIPPING];
    if (do_clip != old_clip)
        wined3d_stateblock_set_render_state(device_impl->state, WINED3D_RS_CLIPPING, do_clip);

    old_lighting = !!state->rs[WINED3D_RS_LIGHTING];
    if (dst_buffer_impl->version == 3)
        do_lighting = device_impl->material && (src_buffer_impl->fvf & D3DFVF_NORMAL)
                && (vertex_op & D3DVOP_LIGHT);
    else
        do_lighting = old_lighting && (vertex_op & D3DVOP_LIGHT);

    if (do_lighting != old_lighting)
        wined3d_stateblock_set_render_state(device_impl->state, WINED3D_RS_LIGHTING, do_lighting);

    wined3d_stateblock_set_stream_source(device_impl->state,
            0, src_buffer_impl->wined3d_buffer, 0, get_flexible_vertex_size(src_buffer_impl->fvf));
    wined3d_stateblock_set_vertex_declaration(device_impl->state, src_buffer_impl->wined3d_declaration);
    hr = wined3d_device_process_vertices(device_impl->wined3d_device, device_impl->state, src_idx, dst_idx,
            count, dst_buffer_impl->wined3d_buffer, NULL, flags, dst_buffer_impl->fvf);

    /* Restore the states if needed */
    if (do_clip != old_clip)
        wined3d_stateblock_set_render_state(device_impl->state, WINED3D_RS_CLIPPING, old_clip);
    if (do_lighting != old_lighting)
        wined3d_stateblock_set_render_state(device_impl->state, WINED3D_RS_LIGHTING, old_lighting);

    wined3d_mutex_unlock();

    return hr;
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
    wined3d_resource = wined3d_buffer_get_resource(buffer->wined3d_buffer);
    wined3d_resource_get_desc(wined3d_resource, &wined3d_desc);
    wined3d_mutex_unlock();

    /* Now fill the desc structure */
    desc->dwCaps = buffer->Caps;
    desc->dwFVF = buffer->fvf;
    desc->dwNumVertices = wined3d_desc.size / get_flexible_vertex_size(buffer->fvf);

    return D3D_OK;
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

    TRACE("iface %p, device %p, flags %#lx.\n", iface, device, flags);

    if (!hide)
    {
        FIXME("iface %p, device %p, flags %#lx stub!\n", iface, device, flags);
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
    FIXME("iface %p, vertex_op %#lx, dst_idx %lu, count %lu, data %p, fvf %#lx, device %p, flags %#lx stub!\n",
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

HRESULT d3d_vertex_buffer_create(struct d3d_vertex_buffer **vertex_buf,
        struct ddraw *ddraw, D3DVERTEXBUFFERDESC *desc)
{
    struct d3d_vertex_buffer *buffer;
    HRESULT hr = D3D_OK;

    TRACE("Vertex buffer description:\n");
    TRACE("    dwSize %lu\n", desc->dwSize);
    TRACE("    dwCaps %#lx\n", desc->dwCaps);
    TRACE("    FVF %#lx\n", desc->dwFVF);
    TRACE("    dwNumVertices %lu\n", desc->dwNumVertices);

    if (!(buffer = calloc(1, sizeof(*buffer))))
        return DDERR_OUTOFMEMORY;

    buffer->IDirect3DVertexBuffer7_iface.lpVtbl = &d3d_vertex_buffer7_vtbl;
    buffer->ref = 1;
    buffer->version = ddraw->d3dversion;
    if (buffer->version == 7)
        IDirectDraw7_AddRef(&ddraw->IDirectDraw7_iface);
    buffer->ddraw = ddraw;
    buffer->Caps = desc->dwCaps;
    buffer->fvf = desc->dwFVF;
    buffer->size = get_flexible_vertex_size(desc->dwFVF) * desc->dwNumVertices;

    /* ddraw4 vertex buffers ignore DISCARD and NOOVERWRITE, even on
     * pretransformed geometry, which means that a GPU-based buffer cannot
     * perform well.
     *
     * While at least one contemporaneous card (Geforce 4) does seem to show a
     * difference in its performance characteristics based on whether
     * D3DVBCAPS_SYSTEMMEMORY is set, it also doesn't *improve* performance to
     * use a non-SYSTEMMEMORY buffer with ddraw4. For wined3d it should always
     * be better to use sysmem.
     *
     * This improves performance in Prince of Persia 3D. */
    buffer->sysmem = ((buffer->Caps & D3DVBCAPS_SYSTEMMEMORY) || buffer->version < 7);

    wined3d_mutex_lock();

    if (FAILED(hr = d3d_vertex_buffer_create_wined3d_buffer(buffer, FALSE, &buffer->wined3d_buffer)))
    {
        WARN("Failed to create wined3d vertex buffer, hr %#lx.\n", hr);
        if (hr == WINED3DERR_INVALIDCALL)
            hr = DDERR_INVALIDPARAMS;
        goto end;
    }

    if (!(buffer->wined3d_declaration = ddraw_find_decl(ddraw, desc->dwFVF)))
    {
        ERR("Failed to find vertex declaration for fvf %#lx.\n", desc->dwFVF);
        wined3d_buffer_decref(buffer->wined3d_buffer);
        hr = DDERR_INVALIDPARAMS;
        goto end;
    }
    wined3d_vertex_declaration_incref(buffer->wined3d_declaration);

end:
    wined3d_mutex_unlock();
    if (hr == D3D_OK)
        *vertex_buf = buffer;
    else
        free(buffer);

    return hr;
}

struct d3d_vertex_buffer *unsafe_impl_from_IDirect3DVertexBuffer7(IDirect3DVertexBuffer7 *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d_vertex_buffer7_vtbl);

    return impl_from_IDirect3DVertexBuffer7(iface);
}
