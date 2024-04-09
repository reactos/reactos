/*
 * Copyright 2008-2012 Henri Verbeet for CodeWeavers
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

#define NONAMELESSUNION
#include "d3d11_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d11);

static void STDMETHODCALLTYPE d3d_null_wined3d_object_destroyed(void *parent) {}

static const struct wined3d_parent_ops d3d_null_wined3d_parent_ops =
{
    d3d_null_wined3d_object_destroyed,
};

/* ID3D11DeviceContext - immediate context methods */

static inline struct d3d11_immediate_context *impl_from_ID3D11DeviceContext1(ID3D11DeviceContext1 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d11_immediate_context, ID3D11DeviceContext1_iface);
}

static inline struct d3d_device *device_from_immediate_ID3D11DeviceContext1(ID3D11DeviceContext1 *iface)
{
    struct d3d11_immediate_context *context = impl_from_ID3D11DeviceContext1(iface);
    return CONTAINING_RECORD(context, struct d3d_device, immediate_context);
}

static HRESULT STDMETHODCALLTYPE d3d11_immediate_context_QueryInterface(ID3D11DeviceContext1 *iface,
        REFIID iid, void **out)
{
    struct d3d11_immediate_context *context = impl_from_ID3D11DeviceContext1(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID3D11DeviceContext1)
            || IsEqualGUID(iid, &IID_ID3D11DeviceContext)
            || IsEqualGUID(iid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        *out = &context->ID3D11DeviceContext1_iface;
    }
    else if (IsEqualGUID(iid, &IID_ID3D11Multithread))
    {
        *out = &context->ID3D11Multithread_iface;
    }
    else
    {
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        *out = NULL;
        return E_NOINTERFACE;
    }

    ID3D11DeviceContext1_AddRef(iface);
    return S_OK;
}

static ULONG STDMETHODCALLTYPE d3d11_immediate_context_AddRef(ID3D11DeviceContext1 *iface)
{
    struct d3d11_immediate_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    ULONG refcount = InterlockedIncrement(&context->refcount);

    TRACE("%p increasing refcount to %u.\n", context, refcount);

    if (refcount == 1)
    {
        ID3D11Device2_AddRef(&device->ID3D11Device2_iface);
    }

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_immediate_context_Release(ID3D11DeviceContext1 *iface)
{
    struct d3d11_immediate_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    ULONG refcount = InterlockedDecrement(&context->refcount);

    TRACE("%p decreasing refcount to %u.\n", context, refcount);

    if (!refcount)
    {
        ID3D11Device2_Release(&device->ID3D11Device2_iface);
    }

    return refcount;
}

static void d3d11_immediate_context_get_constant_buffers(ID3D11DeviceContext1 *iface,
        enum wined3d_shader_type type, UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    wined3d_mutex_lock();
    for (i = 0; i < buffer_count; ++i)
    {
        struct wined3d_buffer *wined3d_buffer;
        struct d3d_buffer *buffer_impl;

        if (!(wined3d_buffer = wined3d_device_get_constant_buffer(device->wined3d_device,
                type, start_slot + i)))
        {
            buffers[i] = NULL;
            continue;
        }

        buffer_impl = wined3d_buffer_get_parent(wined3d_buffer);
        buffers[i] = &buffer_impl->ID3D11Buffer_iface;
        ID3D11Buffer_AddRef(buffers[i]);
    }
    wined3d_mutex_unlock();
}

static void d3d11_immediate_context_set_constant_buffers(ID3D11DeviceContext1 *iface,
        enum wined3d_shader_type type, UINT start_slot, UINT buffer_count, ID3D11Buffer *const *buffers)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    wined3d_mutex_lock();
    for (i = 0; i < buffer_count; ++i)
    {
        struct d3d_buffer *buffer = unsafe_impl_from_ID3D11Buffer(buffers[i]);

        wined3d_device_set_constant_buffer(device->wined3d_device, type, start_slot + i,
                buffer ? buffer->wined3d_buffer : NULL);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_GetDevice(ID3D11DeviceContext1 *iface, ID3D11Device **device)
{
    struct d3d_device *device_object = device_from_immediate_ID3D11DeviceContext1(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (ID3D11Device *)&device_object->ID3D11Device2_iface;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_immediate_context_GetPrivateData(ID3D11DeviceContext1 *iface, REFGUID guid,
        UINT *data_size, void *data)
{
    struct d3d11_immediate_context *context = impl_from_ID3D11DeviceContext1(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&context->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_immediate_context_SetPrivateData(ID3D11DeviceContext1 *iface, REFGUID guid,
        UINT data_size, const void *data)
{
    struct d3d11_immediate_context *context = impl_from_ID3D11DeviceContext1(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&context->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_immediate_context_SetPrivateDataInterface(ID3D11DeviceContext1 *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d11_immediate_context *context = impl_from_ID3D11DeviceContext1(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&context->private_store, guid, data);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_VSSetConstantBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer *const *buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d11_immediate_context_set_constant_buffers(iface, WINED3D_SHADER_TYPE_VERTEX, start_slot,
            buffer_count, buffers);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_PSSetShaderResources(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11ShaderResourceView *const *views)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n",
            iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct d3d_shader_resource_view *view = unsafe_impl_from_ID3D11ShaderResourceView(views[i]);

        wined3d_device_set_ps_resource_view(device->wined3d_device, start_slot + i,
                view ? view->wined3d_view : NULL);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_PSSetShader(ID3D11DeviceContext1 *iface,
        ID3D11PixelShader *shader, ID3D11ClassInstance *const *class_instances, UINT class_instance_count)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct d3d_pixel_shader *ps = unsafe_impl_from_ID3D11PixelShader(shader);

    TRACE("iface %p, shader %p, class_instances %p, class_instance_count %u.\n",
            iface, shader, class_instances, class_instance_count);

    if (class_instances)
        FIXME("Dynamic linking is not implemented yet.\n");

    wined3d_mutex_lock();
    wined3d_device_set_pixel_shader(device->wined3d_device, ps ? ps->wined3d_shader : NULL);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_PSSetSamplers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT sampler_count, ID3D11SamplerState *const *samplers)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    wined3d_mutex_lock();
    for (i = 0; i < sampler_count; ++i)
    {
        struct d3d_sampler_state *sampler = unsafe_impl_from_ID3D11SamplerState(samplers[i]);

        wined3d_device_set_ps_sampler(device->wined3d_device, start_slot + i,
                sampler ? sampler->wined3d_sampler : NULL);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_VSSetShader(ID3D11DeviceContext1 *iface,
        ID3D11VertexShader *shader, ID3D11ClassInstance *const *class_instances, UINT class_instance_count)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct d3d_vertex_shader *vs = unsafe_impl_from_ID3D11VertexShader(shader);

    TRACE("iface %p, shader %p, class_instances %p, class_instance_count %u.\n",
            iface, shader, class_instances, class_instance_count);

    if (class_instances)
        FIXME("Dynamic linking is not implemented yet.\n");

    wined3d_mutex_lock();
    wined3d_device_set_vertex_shader(device->wined3d_device, vs ? vs->wined3d_shader : NULL);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_DrawIndexed(ID3D11DeviceContext1 *iface,
        UINT index_count, UINT start_index_location, INT base_vertex_location)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);

    TRACE("iface %p, index_count %u, start_index_location %u, base_vertex_location %d.\n",
            iface, index_count, start_index_location, base_vertex_location);

    wined3d_mutex_lock();
    wined3d_device_set_base_vertex_index(device->wined3d_device, base_vertex_location);
    wined3d_device_draw_indexed_primitive(device->wined3d_device, start_index_location, index_count);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_Draw(ID3D11DeviceContext1 *iface,
        UINT vertex_count, UINT start_vertex_location)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);

    TRACE("iface %p, vertex_count %u, start_vertex_location %u.\n",
            iface, vertex_count, start_vertex_location);

    wined3d_mutex_lock();
    wined3d_device_draw_primitive(device->wined3d_device, start_vertex_location, vertex_count);
    wined3d_mutex_unlock();
}

static HRESULT STDMETHODCALLTYPE d3d11_immediate_context_Map(ID3D11DeviceContext1 *iface, ID3D11Resource *resource,
        UINT subresource_idx, D3D11_MAP map_type, UINT map_flags, D3D11_MAPPED_SUBRESOURCE *mapped_subresource)
{
    struct wined3d_resource *wined3d_resource;
    struct wined3d_map_desc map_desc;
    HRESULT hr;

    TRACE("iface %p, resource %p, subresource_idx %u, map_type %u, map_flags %#x, mapped_subresource %p.\n",
            iface, resource, subresource_idx, map_type, map_flags, mapped_subresource);

    if (map_flags)
        FIXME("Ignoring map_flags %#x.\n", map_flags);

    wined3d_resource = wined3d_resource_from_d3d11_resource(resource);

    wined3d_mutex_lock();
    hr = wined3d_resource_map(wined3d_resource, subresource_idx,
            &map_desc, NULL, wined3d_map_flags_from_d3d11_map_type(map_type));
    wined3d_mutex_unlock();

    mapped_subresource->pData = map_desc.data;
    mapped_subresource->RowPitch = map_desc.row_pitch;
    mapped_subresource->DepthPitch = map_desc.slice_pitch;

    return hr;
}

static void STDMETHODCALLTYPE d3d11_immediate_context_Unmap(ID3D11DeviceContext1 *iface, ID3D11Resource *resource,
        UINT subresource_idx)
{
    struct wined3d_resource *wined3d_resource;

    TRACE("iface %p, resource %p, subresource_idx %u.\n", iface, resource, subresource_idx);

    wined3d_resource = wined3d_resource_from_d3d11_resource(resource);

    wined3d_mutex_lock();
    wined3d_resource_unmap(wined3d_resource, subresource_idx);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_PSSetConstantBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer *const *buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d11_immediate_context_set_constant_buffers(iface, WINED3D_SHADER_TYPE_PIXEL, start_slot,
            buffer_count, buffers);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_IASetInputLayout(ID3D11DeviceContext1 *iface,
        ID3D11InputLayout *input_layout)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct d3d_input_layout *layout = unsafe_impl_from_ID3D11InputLayout(input_layout);

    TRACE("iface %p, input_layout %p.\n", iface, input_layout);

    wined3d_mutex_lock();
    wined3d_device_set_vertex_declaration(device->wined3d_device, layout ? layout->wined3d_decl : NULL);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_IASetVertexBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer *const *buffers, const UINT *strides, const UINT *offsets)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p, strides %p, offsets %p.\n",
            iface, start_slot, buffer_count, buffers, strides, offsets);

    wined3d_mutex_lock();
    for (i = 0; i < buffer_count; ++i)
    {
        struct d3d_buffer *buffer = unsafe_impl_from_ID3D11Buffer(buffers[i]);

        wined3d_device_set_stream_source(device->wined3d_device, start_slot + i,
                buffer ? buffer->wined3d_buffer : NULL, offsets[i], strides[i]);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_IASetIndexBuffer(ID3D11DeviceContext1 *iface,
        ID3D11Buffer *buffer, DXGI_FORMAT format, UINT offset)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct d3d_buffer *buffer_impl = unsafe_impl_from_ID3D11Buffer(buffer);

    TRACE("iface %p, buffer %p, format %s, offset %u.\n",
            iface, buffer, debug_dxgi_format(format), offset);

    wined3d_mutex_lock();
    wined3d_device_set_index_buffer(device->wined3d_device,
            buffer_impl ? buffer_impl->wined3d_buffer : NULL,
            wined3dformat_from_dxgi_format(format), offset);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_DrawIndexedInstanced(ID3D11DeviceContext1 *iface,
        UINT instance_index_count, UINT instance_count, UINT start_index_location, INT base_vertex_location,
        UINT start_instance_location)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);

    TRACE("iface %p, instance_index_count %u, instance_count %u, start_index_location %u, "
            "base_vertex_location %d, start_instance_location %u.\n",
            iface, instance_index_count, instance_count, start_index_location,
            base_vertex_location, start_instance_location);

    wined3d_mutex_lock();
    wined3d_device_set_base_vertex_index(device->wined3d_device, base_vertex_location);
    wined3d_device_draw_indexed_primitive_instanced(device->wined3d_device, start_index_location,
            instance_index_count, start_instance_location, instance_count);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_DrawInstanced(ID3D11DeviceContext1 *iface,
        UINT instance_vertex_count, UINT instance_count, UINT start_vertex_location, UINT start_instance_location)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);

    TRACE("iface %p, instance_vertex_count %u, instance_count %u, start_vertex_location %u, "
            "start_instance_location %u.\n",
            iface, instance_vertex_count, instance_count, start_vertex_location,
            start_instance_location);

    wined3d_mutex_lock();
    wined3d_device_draw_primitive_instanced(device->wined3d_device, start_vertex_location,
            instance_vertex_count, start_instance_location, instance_count);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_GSSetConstantBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer *const *buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d11_immediate_context_set_constant_buffers(iface, WINED3D_SHADER_TYPE_GEOMETRY, start_slot,
            buffer_count, buffers);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_GSSetShader(ID3D11DeviceContext1 *iface,
        ID3D11GeometryShader *shader, ID3D11ClassInstance *const *class_instances, UINT class_instance_count)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct d3d_geometry_shader *gs = unsafe_impl_from_ID3D11GeometryShader(shader);

    TRACE("iface %p, shader %p, class_instances %p, class_instance_count %u.\n",
            iface, shader, class_instances, class_instance_count);

    if (class_instances)
        FIXME("Dynamic linking is not implemented yet.\n");

    wined3d_mutex_lock();
    wined3d_device_set_geometry_shader(device->wined3d_device, gs ? gs->wined3d_shader : NULL);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_IASetPrimitiveTopology(ID3D11DeviceContext1 *iface,
        D3D11_PRIMITIVE_TOPOLOGY topology)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    enum wined3d_primitive_type primitive_type;
    unsigned int patch_vertex_count;

    TRACE("iface %p, topology %#x.\n", iface, topology);

    wined3d_primitive_type_from_d3d11_primitive_topology(topology, &primitive_type, &patch_vertex_count);

    wined3d_mutex_lock();
    wined3d_device_set_primitive_type(device->wined3d_device, primitive_type, patch_vertex_count);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_VSSetShaderResources(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11ShaderResourceView *const *views)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n", iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct d3d_shader_resource_view *view = unsafe_impl_from_ID3D11ShaderResourceView(views[i]);

        wined3d_device_set_vs_resource_view(device->wined3d_device, start_slot + i,
                view ? view->wined3d_view : NULL);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_VSSetSamplers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT sampler_count, ID3D11SamplerState *const *samplers)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    wined3d_mutex_lock();
    for (i = 0; i < sampler_count; ++i)
    {
        struct d3d_sampler_state *sampler = unsafe_impl_from_ID3D11SamplerState(samplers[i]);

        wined3d_device_set_vs_sampler(device->wined3d_device, start_slot + i,
                sampler ? sampler->wined3d_sampler : NULL);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_Begin(ID3D11DeviceContext1 *iface,
        ID3D11Asynchronous *asynchronous)
{
    struct d3d_query *query = unsafe_impl_from_ID3D11Asynchronous(asynchronous);
    HRESULT hr;

    TRACE("iface %p, asynchronous %p.\n", iface, asynchronous);

    wined3d_mutex_lock();
    if (FAILED(hr = wined3d_query_issue(query->wined3d_query, WINED3DISSUE_BEGIN)))
        ERR("Failed to issue query, hr %#x.\n", hr);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_End(ID3D11DeviceContext1 *iface,
        ID3D11Asynchronous *asynchronous)
{
    struct d3d_query *query = unsafe_impl_from_ID3D11Asynchronous(asynchronous);
    HRESULT hr;

    TRACE("iface %p, asynchronous %p.\n", iface, asynchronous);

    wined3d_mutex_lock();
    if (FAILED(hr = wined3d_query_issue(query->wined3d_query, WINED3DISSUE_END)))
        ERR("Failed to issue query, hr %#x.\n", hr);
    wined3d_mutex_unlock();
}

static HRESULT STDMETHODCALLTYPE d3d11_immediate_context_GetData(ID3D11DeviceContext1 *iface,
        ID3D11Asynchronous *asynchronous, void *data, UINT data_size, UINT data_flags)
{
    struct d3d_query *query = unsafe_impl_from_ID3D11Asynchronous(asynchronous);
    unsigned int wined3d_flags;
    HRESULT hr;

    TRACE("iface %p, asynchronous %p, data %p, data_size %u, data_flags %#x.\n",
            iface, asynchronous, data, data_size, data_flags);

    if (!data && data_size)
        return E_INVALIDARG;

    wined3d_flags = wined3d_getdata_flags_from_d3d11_async_getdata_flags(data_flags);

    wined3d_mutex_lock();
    if (!data_size || wined3d_query_get_data_size(query->wined3d_query) == data_size)
    {
        hr = wined3d_query_get_data(query->wined3d_query, data, data_size, wined3d_flags);
        if (hr == WINED3DERR_INVALIDCALL)
            hr = DXGI_ERROR_INVALID_CALL;
    }
    else
    {
        WARN("Invalid data size %u.\n", data_size);
        hr = E_INVALIDARG;
    }
    wined3d_mutex_unlock();

    return hr;
}

static void STDMETHODCALLTYPE d3d11_immediate_context_SetPredication(ID3D11DeviceContext1 *iface,
        ID3D11Predicate *predicate, BOOL value)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct d3d_query *query;

    TRACE("iface %p, predicate %p, value %#x.\n", iface, predicate, value);

    query = unsafe_impl_from_ID3D11Query((ID3D11Query *)predicate);

    wined3d_mutex_lock();
    wined3d_device_set_predication(device->wined3d_device, query ? query->wined3d_query : NULL, value);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_GSSetShaderResources(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11ShaderResourceView *const *views)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n", iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct d3d_shader_resource_view *view = unsafe_impl_from_ID3D11ShaderResourceView(views[i]);

        wined3d_device_set_gs_resource_view(device->wined3d_device, start_slot + i,
                view ? view->wined3d_view : NULL);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_GSSetSamplers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT sampler_count, ID3D11SamplerState *const *samplers)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    wined3d_mutex_lock();
    for (i = 0; i < sampler_count; ++i)
    {
        struct d3d_sampler_state *sampler = unsafe_impl_from_ID3D11SamplerState(samplers[i]);

        wined3d_device_set_gs_sampler(device->wined3d_device, start_slot + i,
                sampler ? sampler->wined3d_sampler : NULL);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_OMSetRenderTargets(ID3D11DeviceContext1 *iface,
        UINT render_target_view_count, ID3D11RenderTargetView *const *render_target_views,
        ID3D11DepthStencilView *depth_stencil_view)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct d3d_depthstencil_view *dsv;
    unsigned int i;

    TRACE("iface %p, render_target_view_count %u, render_target_views %p, depth_stencil_view %p.\n",
            iface, render_target_view_count, render_target_views, depth_stencil_view);

    wined3d_mutex_lock();
    for (i = 0; i < render_target_view_count; ++i)
    {
        struct d3d_rendertarget_view *rtv = unsafe_impl_from_ID3D11RenderTargetView(render_target_views[i]);
        wined3d_device_set_rendertarget_view(device->wined3d_device, i, rtv ? rtv->wined3d_view : NULL, FALSE);
    }
    for (; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        wined3d_device_set_rendertarget_view(device->wined3d_device, i, NULL, FALSE);
    }

    dsv = unsafe_impl_from_ID3D11DepthStencilView(depth_stencil_view);
    wined3d_device_set_depth_stencil_view(device->wined3d_device, dsv ? dsv->wined3d_view : NULL);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_OMSetRenderTargetsAndUnorderedAccessViews(
        ID3D11DeviceContext1 *iface, UINT render_target_view_count,
        ID3D11RenderTargetView *const *render_target_views, ID3D11DepthStencilView *depth_stencil_view,
        UINT unordered_access_view_start_slot, UINT unordered_access_view_count,
        ID3D11UnorderedAccessView *const *unordered_access_views, const UINT *initial_counts)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, render_target_view_count %u, render_target_views %p, depth_stencil_view %p, "
            "unordered_access_view_start_slot %u, unordered_access_view_count %u, unordered_access_views %p, "
            "initial_counts %p.\n",
            iface, render_target_view_count, render_target_views, depth_stencil_view,
            unordered_access_view_start_slot, unordered_access_view_count, unordered_access_views,
            initial_counts);

    if (render_target_view_count != D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL)
    {
        d3d11_immediate_context_OMSetRenderTargets(iface, render_target_view_count, render_target_views,
                depth_stencil_view);
    }

    if (unordered_access_view_count != D3D11_KEEP_UNORDERED_ACCESS_VIEWS)
    {
        wined3d_mutex_lock();
        for (i = 0; i < unordered_access_view_start_slot; ++i)
        {
            wined3d_device_set_unordered_access_view(device->wined3d_device, i, NULL, ~0u);
        }
        for (i = 0; i < unordered_access_view_count; ++i)
        {
            struct d3d11_unordered_access_view *view
                    = unsafe_impl_from_ID3D11UnorderedAccessView(unordered_access_views[i]);

            wined3d_device_set_unordered_access_view(device->wined3d_device,
                    unordered_access_view_start_slot + i,
                    view ? view->wined3d_view : NULL, initial_counts ? initial_counts[i] : ~0u);
        }
        for (; unordered_access_view_start_slot + i < D3D11_PS_CS_UAV_REGISTER_COUNT; ++i)
        {
            wined3d_device_set_unordered_access_view(device->wined3d_device,
                    unordered_access_view_start_slot + i, NULL, ~0u);
        }
        wined3d_mutex_unlock();
    }
}

static void STDMETHODCALLTYPE d3d11_immediate_context_OMSetBlendState(ID3D11DeviceContext1 *iface,
        ID3D11BlendState *blend_state, const float blend_factor[4], UINT sample_mask)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    static const float default_blend_factor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    struct d3d_blend_state *blend_state_impl;
    const D3D11_BLEND_DESC *desc;

    TRACE("iface %p, blend_state %p, blend_factor %s, sample_mask 0x%08x.\n",
            iface, blend_state, debug_float4(blend_factor), sample_mask);

    if (!blend_factor)
        blend_factor = default_blend_factor;

    wined3d_mutex_lock();
    wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_MULTISAMPLEMASK, sample_mask);
    if (!(blend_state_impl = unsafe_impl_from_ID3D11BlendState(blend_state)))
    {
        wined3d_device_set_blend_state(device->wined3d_device, NULL,
                (const struct wined3d_color *)blend_factor);
        wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_ALPHABLENDENABLE, FALSE);
        wined3d_device_set_render_state(device->wined3d_device,
                WINED3D_RS_COLORWRITEENABLE, D3D11_COLOR_WRITE_ENABLE_ALL);
        wined3d_device_set_render_state(device->wined3d_device,
                WINED3D_RS_COLORWRITEENABLE1, D3D11_COLOR_WRITE_ENABLE_ALL);
        wined3d_device_set_render_state(device->wined3d_device,
                WINED3D_RS_COLORWRITEENABLE2, D3D11_COLOR_WRITE_ENABLE_ALL);
        wined3d_device_set_render_state(device->wined3d_device,
                WINED3D_RS_COLORWRITEENABLE3, D3D11_COLOR_WRITE_ENABLE_ALL);
        wined3d_mutex_unlock();
        return;
    }

    wined3d_device_set_blend_state(device->wined3d_device, blend_state_impl->wined3d_state,
            (const struct wined3d_color *)blend_factor);
    desc = &blend_state_impl->desc;
    wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_ALPHABLENDENABLE,
            desc->RenderTarget[0].BlendEnable);
    if (desc->RenderTarget[0].BlendEnable)
    {
        const D3D11_RENDER_TARGET_BLEND_DESC *d = &desc->RenderTarget[0];

        wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_SRCBLEND, d->SrcBlend);
        wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_DESTBLEND, d->DestBlend);
        wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_BLENDOP, d->BlendOp);
        wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_SEPARATEALPHABLENDENABLE, TRUE);
        wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_SRCBLENDALPHA, d->SrcBlendAlpha);
        wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_DESTBLENDALPHA, d->DestBlendAlpha);
        wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_BLENDOPALPHA, d->BlendOpAlpha);
    }
    wined3d_device_set_render_state(device->wined3d_device,
            WINED3D_RS_COLORWRITEENABLE, desc->RenderTarget[0].RenderTargetWriteMask);
    wined3d_device_set_render_state(device->wined3d_device,
            WINED3D_RS_COLORWRITEENABLE1, desc->RenderTarget[1].RenderTargetWriteMask);
    wined3d_device_set_render_state(device->wined3d_device,
            WINED3D_RS_COLORWRITEENABLE2, desc->RenderTarget[2].RenderTargetWriteMask);
    wined3d_device_set_render_state(device->wined3d_device,
            WINED3D_RS_COLORWRITEENABLE3, desc->RenderTarget[3].RenderTargetWriteMask);
    wined3d_mutex_unlock();
}

static void set_default_depth_stencil_state(struct wined3d_device *wined3d_device)
{
    wined3d_device_set_render_state(wined3d_device, WINED3D_RS_ZENABLE, TRUE);
    wined3d_device_set_render_state(wined3d_device, WINED3D_RS_ZWRITEENABLE, D3D11_DEPTH_WRITE_MASK_ALL);
    wined3d_device_set_render_state(wined3d_device, WINED3D_RS_ZFUNC, WINED3D_CMP_LESS);
    wined3d_device_set_render_state(wined3d_device, WINED3D_RS_STENCILENABLE, FALSE);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_OMSetDepthStencilState(ID3D11DeviceContext1 *iface,
        ID3D11DepthStencilState *depth_stencil_state, UINT stencil_ref)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    const D3D11_DEPTH_STENCILOP_DESC *front, *back;
    const D3D11_DEPTH_STENCIL_DESC *desc;

    TRACE("iface %p, depth_stencil_state %p, stencil_ref %u.\n",
            iface, depth_stencil_state, stencil_ref);

    wined3d_mutex_lock();
    device->stencil_ref = stencil_ref;
    if (!(device->depth_stencil_state = unsafe_impl_from_ID3D11DepthStencilState(depth_stencil_state)))
    {
        set_default_depth_stencil_state(device->wined3d_device);
        wined3d_mutex_unlock();
        return;
    }

    desc = &device->depth_stencil_state->desc;

    front = &desc->FrontFace;
    back = &desc->BackFace;

    wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_ZENABLE, desc->DepthEnable);
    if (desc->DepthEnable)
    {
        wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_ZWRITEENABLE, desc->DepthWriteMask);
        wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_ZFUNC, desc->DepthFunc);
    }

    wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_STENCILENABLE, desc->StencilEnable);
    if (desc->StencilEnable)
    {
        wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_STENCILMASK, desc->StencilReadMask);
        wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_STENCILWRITEMASK, desc->StencilWriteMask);
        wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_STENCILREF, stencil_ref);

        wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_STENCILFAIL, front->StencilFailOp);
        wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_STENCILZFAIL, front->StencilDepthFailOp);
        wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_STENCILPASS, front->StencilPassOp);
        wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_STENCILFUNC, front->StencilFunc);
        if (front->StencilFailOp != back->StencilFailOp
                || front->StencilDepthFailOp != back->StencilDepthFailOp
                || front->StencilPassOp != back->StencilPassOp
                || front->StencilFunc != back->StencilFunc)
        {
            wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_TWOSIDEDSTENCILMODE, TRUE);
            wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_BACK_STENCILFAIL, back->StencilFailOp);
            wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_BACK_STENCILZFAIL,
                    back->StencilDepthFailOp);
            wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_BACK_STENCILPASS, back->StencilPassOp);
            wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_BACK_STENCILFUNC, back->StencilFunc);
        }
        else
        {
            wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_TWOSIDEDSTENCILMODE, FALSE);
        }
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_SOSetTargets(ID3D11DeviceContext1 *iface, UINT buffer_count,
        ID3D11Buffer *const *buffers, const UINT *offsets)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int count, i;

    TRACE("iface %p, buffer_count %u, buffers %p, offsets %p.\n", iface, buffer_count, buffers, offsets);

    count = min(buffer_count, D3D11_SO_BUFFER_SLOT_COUNT);
    wined3d_mutex_lock();
    for (i = 0; i < count; ++i)
    {
        struct d3d_buffer *buffer = unsafe_impl_from_ID3D11Buffer(buffers[i]);

        wined3d_device_set_stream_output(device->wined3d_device, i,
                buffer ? buffer->wined3d_buffer : NULL, offsets ? offsets[i] : 0);
    }
    for (; i < D3D11_SO_BUFFER_SLOT_COUNT; ++i)
    {
        wined3d_device_set_stream_output(device->wined3d_device, i, NULL, 0);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_DrawAuto(ID3D11DeviceContext1 *iface)
{
    FIXME("iface %p stub!\n", iface);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_DrawIndexedInstancedIndirect(ID3D11DeviceContext1 *iface,
        ID3D11Buffer *buffer, UINT offset)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct d3d_buffer *d3d_buffer;

    TRACE("iface %p, buffer %p, offset %u.\n", iface, buffer, offset);

    d3d_buffer = unsafe_impl_from_ID3D11Buffer(buffer);

    wined3d_mutex_lock();
    wined3d_device_draw_indexed_primitive_instanced_indirect(device->wined3d_device,
            d3d_buffer->wined3d_buffer, offset);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_DrawInstancedIndirect(ID3D11DeviceContext1 *iface,
        ID3D11Buffer *buffer, UINT offset)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct d3d_buffer *d3d_buffer;

    TRACE("iface %p, buffer %p, offset %u.\n", iface, buffer, offset);

    d3d_buffer = unsafe_impl_from_ID3D11Buffer(buffer);

    wined3d_mutex_lock();
    wined3d_device_draw_primitive_instanced_indirect(device->wined3d_device,
            d3d_buffer->wined3d_buffer, offset);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_Dispatch(ID3D11DeviceContext1 *iface,
        UINT thread_group_count_x, UINT thread_group_count_y, UINT thread_group_count_z)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);

    TRACE("iface %p, thread_group_count_x %u, thread_group_count_y %u, thread_group_count_z %u.\n",
            iface, thread_group_count_x, thread_group_count_y, thread_group_count_z);

    wined3d_mutex_lock();
    wined3d_device_dispatch_compute(device->wined3d_device,
            thread_group_count_x, thread_group_count_y, thread_group_count_z);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_DispatchIndirect(ID3D11DeviceContext1 *iface,
        ID3D11Buffer *buffer, UINT offset)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct d3d_buffer *buffer_impl;

    TRACE("iface %p, buffer %p, offset %u.\n", iface, buffer, offset);

    buffer_impl = unsafe_impl_from_ID3D11Buffer(buffer);

    wined3d_mutex_lock();
    wined3d_device_dispatch_compute_indirect(device->wined3d_device,
            buffer_impl->wined3d_buffer, offset);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_RSSetState(ID3D11DeviceContext1 *iface,
        ID3D11RasterizerState *rasterizer_state)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct d3d_rasterizer_state *rasterizer_state_impl;
    const D3D11_RASTERIZER_DESC *desc;
    union
    {
        DWORD d;
        float f;
    } scale_bias, const_bias;

    TRACE("iface %p, rasterizer_state %p.\n", iface, rasterizer_state);

    wined3d_mutex_lock();
    if (!(rasterizer_state_impl = unsafe_impl_from_ID3D11RasterizerState(rasterizer_state)))
    {
        wined3d_device_set_rasterizer_state(device->wined3d_device, NULL);
        wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_FILLMODE, WINED3D_FILL_SOLID);
        wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_CULLMODE, WINED3D_CULL_BACK);
        wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_SLOPESCALEDEPTHBIAS, 0);
        wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_DEPTHBIAS, 0);
        wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_SCISSORTESTENABLE, FALSE);
        wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_MULTISAMPLEANTIALIAS, FALSE);
        wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_ANTIALIASEDLINEENABLE, FALSE);
        wined3d_mutex_unlock();
        return;
    }

    wined3d_device_set_rasterizer_state(device->wined3d_device, rasterizer_state_impl->wined3d_state);

    desc = &rasterizer_state_impl->desc;
    wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_FILLMODE, desc->FillMode);
    wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_CULLMODE, desc->CullMode);
    scale_bias.f = desc->SlopeScaledDepthBias;
    const_bias.f = desc->DepthBias;
    wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_SLOPESCALEDEPTHBIAS, scale_bias.d);
    wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_DEPTHBIAS, const_bias.d);
    wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_SCISSORTESTENABLE, desc->ScissorEnable);
    wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_MULTISAMPLEANTIALIAS, desc->MultisampleEnable);
    wined3d_device_set_render_state(device->wined3d_device,
            WINED3D_RS_ANTIALIASEDLINEENABLE, desc->AntialiasedLineEnable);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_RSSetViewports(ID3D11DeviceContext1 *iface,
        UINT viewport_count, const D3D11_VIEWPORT *viewports)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct wined3d_viewport wined3d_vp[WINED3D_MAX_VIEWPORTS];
    unsigned int i;

    TRACE("iface %p, viewport_count %u, viewports %p.\n", iface, viewport_count, viewports);

    if (viewport_count > ARRAY_SIZE(wined3d_vp))
        return;

    for (i = 0; i < viewport_count; ++i)
    {
        wined3d_vp[i].x = viewports[i].TopLeftX;
        wined3d_vp[i].y = viewports[i].TopLeftY;
        wined3d_vp[i].width = viewports[i].Width;
        wined3d_vp[i].height = viewports[i].Height;
        wined3d_vp[i].min_z = viewports[i].MinDepth;
        wined3d_vp[i].max_z = viewports[i].MaxDepth;
    }

    wined3d_mutex_lock();
    wined3d_device_set_viewports(device->wined3d_device, viewport_count, wined3d_vp);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_RSSetScissorRects(ID3D11DeviceContext1 *iface,
        UINT rect_count, const D3D11_RECT *rects)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);

    TRACE("iface %p, rect_count %u, rects %p.\n", iface, rect_count, rects);

    if (rect_count > WINED3D_MAX_VIEWPORTS)
        return;

    wined3d_mutex_lock();
    wined3d_device_set_scissor_rects(device->wined3d_device, rect_count, rects);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_CopySubresourceRegion(ID3D11DeviceContext1 *iface,
        ID3D11Resource *dst_resource, UINT dst_subresource_idx, UINT dst_x, UINT dst_y, UINT dst_z,
        ID3D11Resource *src_resource, UINT src_subresource_idx, const D3D11_BOX *src_box)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct wined3d_resource *wined3d_dst_resource, *wined3d_src_resource;
    struct wined3d_box wined3d_src_box;

    TRACE("iface %p, dst_resource %p, dst_subresource_idx %u, dst_x %u, dst_y %u, dst_z %u, "
            "src_resource %p, src_subresource_idx %u, src_box %p.\n",
            iface, dst_resource, dst_subresource_idx, dst_x, dst_y, dst_z,
            src_resource, src_subresource_idx, src_box);

    if (src_box)
        wined3d_box_set(&wined3d_src_box, src_box->left, src_box->top,
                src_box->right, src_box->bottom, src_box->front, src_box->back);

    wined3d_dst_resource = wined3d_resource_from_d3d11_resource(dst_resource);
    wined3d_src_resource = wined3d_resource_from_d3d11_resource(src_resource);
    wined3d_mutex_lock();
    wined3d_device_copy_sub_resource_region(device->wined3d_device, wined3d_dst_resource, dst_subresource_idx,
            dst_x, dst_y, dst_z, wined3d_src_resource, src_subresource_idx, src_box ? &wined3d_src_box : NULL, 0);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_CopyResource(ID3D11DeviceContext1 *iface,
        ID3D11Resource *dst_resource, ID3D11Resource *src_resource)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct wined3d_resource *wined3d_dst_resource, *wined3d_src_resource;

    TRACE("iface %p, dst_resource %p, src_resource %p.\n", iface, dst_resource, src_resource);

    wined3d_dst_resource = wined3d_resource_from_d3d11_resource(dst_resource);
    wined3d_src_resource = wined3d_resource_from_d3d11_resource(src_resource);
    wined3d_mutex_lock();
    wined3d_device_copy_resource(device->wined3d_device, wined3d_dst_resource, wined3d_src_resource);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_UpdateSubresource(ID3D11DeviceContext1 *iface,
        ID3D11Resource *resource, UINT subresource_idx, const D3D11_BOX *box,
        const void *data, UINT row_pitch, UINT depth_pitch)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct wined3d_resource *wined3d_resource;
    struct wined3d_box wined3d_box;

    TRACE("iface %p, resource %p, subresource_idx %u, box %p, data %p, row_pitch %u, depth_pitch %u.\n",
            iface, resource, subresource_idx, box, data, row_pitch, depth_pitch);

    if (box)
        wined3d_box_set(&wined3d_box, box->left, box->top, box->right, box->bottom, box->front, box->back);

    wined3d_resource = wined3d_resource_from_d3d11_resource(resource);
    wined3d_mutex_lock();
    wined3d_device_update_sub_resource(device->wined3d_device, wined3d_resource,
            subresource_idx, box ? &wined3d_box : NULL, data, row_pitch, depth_pitch, 0);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_CopyStructureCount(ID3D11DeviceContext1 *iface,
        ID3D11Buffer *dst_buffer, UINT dst_offset, ID3D11UnorderedAccessView *src_view)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct d3d11_unordered_access_view *uav;
    struct d3d_buffer *buffer_impl;

    TRACE("iface %p, dst_buffer %p, dst_offset %u, src_view %p.\n",
            iface, dst_buffer, dst_offset, src_view);

    buffer_impl = unsafe_impl_from_ID3D11Buffer(dst_buffer);
    uav = unsafe_impl_from_ID3D11UnorderedAccessView(src_view);

    wined3d_mutex_lock();
    wined3d_device_copy_uav_counter(device->wined3d_device,
            buffer_impl->wined3d_buffer, dst_offset, uav->wined3d_view);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_ClearRenderTargetView(ID3D11DeviceContext1 *iface,
        ID3D11RenderTargetView *render_target_view, const float color_rgba[4])
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct d3d_rendertarget_view *view = unsafe_impl_from_ID3D11RenderTargetView(render_target_view);
    const struct wined3d_color color = {color_rgba[0], color_rgba[1], color_rgba[2], color_rgba[3]};
    HRESULT hr;

    TRACE("iface %p, render_target_view %p, color_rgba %s.\n",
            iface, render_target_view, debug_float4(color_rgba));

    if (!view)
        return;

    wined3d_mutex_lock();
    if (FAILED(hr = wined3d_device_clear_rendertarget_view(device->wined3d_device, view->wined3d_view, NULL,
            WINED3DCLEAR_TARGET, &color, 0.0f, 0)))
        ERR("Failed to clear view, hr %#x.\n", hr);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_ClearUnorderedAccessViewUint(ID3D11DeviceContext1 *iface,
        ID3D11UnorderedAccessView *unordered_access_view, const UINT values[4])
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct d3d11_unordered_access_view *view;

    TRACE("iface %p, unordered_access_view %p, values {%u, %u, %u, %u}.\n",
            iface, unordered_access_view, values[0], values[1], values[2], values[3]);

    view = unsafe_impl_from_ID3D11UnorderedAccessView(unordered_access_view);
    wined3d_mutex_lock();
    wined3d_device_clear_unordered_access_view_uint(device->wined3d_device,
            view->wined3d_view, (const struct wined3d_uvec4 *)values);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_ClearUnorderedAccessViewFloat(ID3D11DeviceContext1 *iface,
        ID3D11UnorderedAccessView *unordered_access_view, const float values[4])
{
    FIXME("iface %p, unordered_access_view %p, values %s stub!\n",
            iface, unordered_access_view, debug_float4(values));
}

static void STDMETHODCALLTYPE d3d11_immediate_context_ClearDepthStencilView(ID3D11DeviceContext1 *iface,
        ID3D11DepthStencilView *depth_stencil_view, UINT flags, FLOAT depth, UINT8 stencil)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct d3d_depthstencil_view *view = unsafe_impl_from_ID3D11DepthStencilView(depth_stencil_view);
    DWORD wined3d_flags;
    HRESULT hr;

    TRACE("iface %p, depth_stencil_view %p, flags %#x, depth %.8e, stencil %u.\n",
            iface, depth_stencil_view, flags, depth, stencil);

    if (!view)
        return;

    wined3d_flags = wined3d_clear_flags_from_d3d11_clear_flags(flags);

    wined3d_mutex_lock();
    if (FAILED(hr = wined3d_device_clear_rendertarget_view(device->wined3d_device, view->wined3d_view, NULL,
            wined3d_flags, NULL, depth, stencil)))
        ERR("Failed to clear view, hr %#x.\n", hr);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_GenerateMips(ID3D11DeviceContext1 *iface,
        ID3D11ShaderResourceView *view)
{
    struct d3d_shader_resource_view *srv = unsafe_impl_from_ID3D11ShaderResourceView(view);

    TRACE("iface %p, view %p.\n", iface, view);

    wined3d_mutex_lock();
    wined3d_shader_resource_view_generate_mipmaps(srv->wined3d_view);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_SetResourceMinLOD(ID3D11DeviceContext1 *iface,
        ID3D11Resource *resource, FLOAT min_lod)
{
    FIXME("iface %p, resource %p, min_lod %f stub!\n", iface, resource, min_lod);
}

static FLOAT STDMETHODCALLTYPE d3d11_immediate_context_GetResourceMinLOD(ID3D11DeviceContext1 *iface,
        ID3D11Resource *resource)
{
    FIXME("iface %p, resource %p stub!\n", iface, resource);

    return 0.0f;
}

static void STDMETHODCALLTYPE d3d11_immediate_context_ResolveSubresource(ID3D11DeviceContext1 *iface,
        ID3D11Resource *dst_resource, UINT dst_subresource_idx,
        ID3D11Resource *src_resource, UINT src_subresource_idx,
        DXGI_FORMAT format)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct wined3d_resource *wined3d_dst_resource, *wined3d_src_resource;
    enum wined3d_format_id wined3d_format;

    TRACE("iface %p, dst_resource %p, dst_subresource_idx %u, "
            "src_resource %p, src_subresource_idx %u, format %s.\n",
            iface, dst_resource, dst_subresource_idx,
            src_resource, src_subresource_idx, debug_dxgi_format(format));

    wined3d_dst_resource = wined3d_resource_from_d3d11_resource(dst_resource);
    wined3d_src_resource = wined3d_resource_from_d3d11_resource(src_resource);
    wined3d_format = wined3dformat_from_dxgi_format(format);
    wined3d_mutex_lock();
    wined3d_device_resolve_sub_resource(device->wined3d_device,
            wined3d_dst_resource, dst_subresource_idx,
            wined3d_src_resource, src_subresource_idx, wined3d_format);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_ExecuteCommandList(ID3D11DeviceContext1 *iface,
        ID3D11CommandList *command_list, BOOL restore_state)
{
    FIXME("iface %p, command_list %p, restore_state %#x stub!\n", iface, command_list, restore_state);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_HSSetShaderResources(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11ShaderResourceView *const *views)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n",
            iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct d3d_shader_resource_view *view = unsafe_impl_from_ID3D11ShaderResourceView(views[i]);

        wined3d_device_set_hs_resource_view(device->wined3d_device, start_slot + i,
                view ? view->wined3d_view : NULL);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_HSSetShader(ID3D11DeviceContext1 *iface,
        ID3D11HullShader *shader, ID3D11ClassInstance *const *class_instances, UINT class_instance_count)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct d3d11_hull_shader *hs = unsafe_impl_from_ID3D11HullShader(shader);

    TRACE("iface %p, shader %p, class_instances %p, class_instance_count %u.\n",
            iface, shader, class_instances, class_instance_count);

    if (class_instances)
        FIXME("Dynamic linking is not implemented yet.\n");

    wined3d_mutex_lock();
    wined3d_device_set_hull_shader(device->wined3d_device, hs ? hs->wined3d_shader : NULL);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_HSSetSamplers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT sampler_count, ID3D11SamplerState *const *samplers)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    wined3d_mutex_lock();
    for (i = 0; i < sampler_count; ++i)
    {
        struct d3d_sampler_state *sampler = unsafe_impl_from_ID3D11SamplerState(samplers[i]);

        wined3d_device_set_hs_sampler(device->wined3d_device, start_slot + i,
                sampler ? sampler->wined3d_sampler : NULL);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_HSSetConstantBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer *const *buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d11_immediate_context_set_constant_buffers(iface, WINED3D_SHADER_TYPE_HULL, start_slot,
            buffer_count, buffers);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_DSSetShaderResources(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11ShaderResourceView *const *views)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n",
            iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct d3d_shader_resource_view *view = unsafe_impl_from_ID3D11ShaderResourceView(views[i]);

        wined3d_device_set_ds_resource_view(device->wined3d_device, start_slot + i,
                view ? view->wined3d_view : NULL);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_DSSetShader(ID3D11DeviceContext1 *iface,
        ID3D11DomainShader *shader, ID3D11ClassInstance *const *class_instances, UINT class_instance_count)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct d3d11_domain_shader *ds = unsafe_impl_from_ID3D11DomainShader(shader);

    TRACE("iface %p, shader %p, class_instances %p, class_instance_count %u.\n",
            iface, shader, class_instances, class_instance_count);

    if (class_instances)
        FIXME("Dynamic linking is not implemented yet.\n");

    wined3d_mutex_lock();
    wined3d_device_set_domain_shader(device->wined3d_device, ds ? ds->wined3d_shader : NULL);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_DSSetSamplers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT sampler_count, ID3D11SamplerState *const *samplers)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    wined3d_mutex_lock();
    for (i = 0; i < sampler_count; ++i)
    {
        struct d3d_sampler_state *sampler = unsafe_impl_from_ID3D11SamplerState(samplers[i]);

        wined3d_device_set_ds_sampler(device->wined3d_device, start_slot + i,
                sampler ? sampler->wined3d_sampler : NULL);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_DSSetConstantBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer *const *buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d11_immediate_context_set_constant_buffers(iface, WINED3D_SHADER_TYPE_DOMAIN, start_slot,
            buffer_count, buffers);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_CSSetShaderResources(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11ShaderResourceView *const *views)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n",
            iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct d3d_shader_resource_view *view = unsafe_impl_from_ID3D11ShaderResourceView(views[i]);

        wined3d_device_set_cs_resource_view(device->wined3d_device, start_slot + i,
                view ? view->wined3d_view : NULL);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_CSSetUnorderedAccessViews(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11UnorderedAccessView *const *views, const UINT *initial_counts)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p, initial_counts %p.\n",
            iface, start_slot, view_count, views, initial_counts);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct d3d11_unordered_access_view *view = unsafe_impl_from_ID3D11UnorderedAccessView(views[i]);

        wined3d_device_set_cs_uav(device->wined3d_device, start_slot + i,
                view ? view->wined3d_view : NULL, initial_counts ? initial_counts[i] : ~0u);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_CSSetShader(ID3D11DeviceContext1 *iface,
        ID3D11ComputeShader *shader, ID3D11ClassInstance *const *class_instances, UINT class_instance_count)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct d3d11_compute_shader *cs = unsafe_impl_from_ID3D11ComputeShader(shader);

    TRACE("iface %p, shader %p, class_instances %p, class_instance_count %u.\n",
            iface, shader, class_instances, class_instance_count);

    if (class_instances)
        FIXME("Dynamic linking is not implemented yet.\n");

    wined3d_mutex_lock();
    wined3d_device_set_compute_shader(device->wined3d_device, cs ? cs->wined3d_shader : NULL);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_CSSetSamplers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT sampler_count, ID3D11SamplerState *const *samplers)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    wined3d_mutex_lock();
    for (i = 0; i < sampler_count; ++i)
    {
        struct d3d_sampler_state *sampler = unsafe_impl_from_ID3D11SamplerState(samplers[i]);

        wined3d_device_set_cs_sampler(device->wined3d_device, start_slot + i,
                sampler ? sampler->wined3d_sampler : NULL);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_CSSetConstantBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer *const *buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d11_immediate_context_set_constant_buffers(iface, WINED3D_SHADER_TYPE_COMPUTE, start_slot,
            buffer_count, buffers);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_VSGetConstantBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d11_immediate_context_get_constant_buffers(iface, WINED3D_SHADER_TYPE_VERTEX, start_slot,
            buffer_count, buffers);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_PSGetShaderResources(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11ShaderResourceView **views)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n",
            iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct wined3d_shader_resource_view *wined3d_view;
        struct d3d_shader_resource_view *view_impl;

        if (!(wined3d_view = wined3d_device_get_ps_resource_view(device->wined3d_device, start_slot + i)))
        {
            views[i] = NULL;
            continue;
        }

        view_impl = wined3d_shader_resource_view_get_parent(wined3d_view);
        views[i] = &view_impl->ID3D11ShaderResourceView_iface;
        ID3D11ShaderResourceView_AddRef(views[i]);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_PSGetShader(ID3D11DeviceContext1 *iface,
        ID3D11PixelShader **shader, ID3D11ClassInstance **class_instances, UINT *class_instance_count)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct wined3d_shader *wined3d_shader;
    struct d3d_pixel_shader *shader_impl;

    TRACE("iface %p, shader %p, class_instances %p, class_instance_count %p.\n",
            iface, shader, class_instances, class_instance_count);

    if (class_instances || class_instance_count)
        FIXME("Dynamic linking not implemented yet.\n");
    if (class_instance_count)
        *class_instance_count = 0;

    wined3d_mutex_lock();
    if (!(wined3d_shader = wined3d_device_get_pixel_shader(device->wined3d_device)))
    {
        wined3d_mutex_unlock();
        *shader = NULL;
        return;
    }

    shader_impl = wined3d_shader_get_parent(wined3d_shader);
    wined3d_mutex_unlock();
    *shader = &shader_impl->ID3D11PixelShader_iface;
    ID3D11PixelShader_AddRef(*shader);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_PSGetSamplers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT sampler_count, ID3D11SamplerState **samplers)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    wined3d_mutex_lock();
    for (i = 0; i < sampler_count; ++i)
    {
        struct wined3d_sampler *wined3d_sampler;
        struct d3d_sampler_state *sampler_impl;

        if (!(wined3d_sampler = wined3d_device_get_ps_sampler(device->wined3d_device, start_slot + i)))
        {
            samplers[i] = NULL;
            continue;
        }

        sampler_impl = wined3d_sampler_get_parent(wined3d_sampler);
        samplers[i] = &sampler_impl->ID3D11SamplerState_iface;
        ID3D11SamplerState_AddRef(samplers[i]);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_VSGetShader(ID3D11DeviceContext1 *iface,
        ID3D11VertexShader **shader, ID3D11ClassInstance **class_instances, UINT *class_instance_count)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct d3d_vertex_shader *shader_impl;
    struct wined3d_shader *wined3d_shader;

    TRACE("iface %p, shader %p, class_instances %p, class_instance_count %p.\n",
            iface, shader, class_instances, class_instance_count);

    if (class_instances || class_instance_count)
        FIXME("Dynamic linking not implemented yet.\n");
    if (class_instance_count)
        *class_instance_count = 0;

    wined3d_mutex_lock();
    if (!(wined3d_shader = wined3d_device_get_vertex_shader(device->wined3d_device)))
    {
        wined3d_mutex_unlock();
        *shader = NULL;
        return;
    }

    shader_impl = wined3d_shader_get_parent(wined3d_shader);
    wined3d_mutex_unlock();
    *shader = &shader_impl->ID3D11VertexShader_iface;
    ID3D11VertexShader_AddRef(*shader);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_PSGetConstantBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d11_immediate_context_get_constant_buffers(iface, WINED3D_SHADER_TYPE_PIXEL, start_slot,
            buffer_count, buffers);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_IAGetInputLayout(ID3D11DeviceContext1 *iface,
        ID3D11InputLayout **input_layout)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct wined3d_vertex_declaration *wined3d_declaration;
    struct d3d_input_layout *input_layout_impl;

    TRACE("iface %p, input_layout %p.\n", iface, input_layout);

    wined3d_mutex_lock();
    if (!(wined3d_declaration = wined3d_device_get_vertex_declaration(device->wined3d_device)))
    {
        wined3d_mutex_unlock();
        *input_layout = NULL;
        return;
    }

    input_layout_impl = wined3d_vertex_declaration_get_parent(wined3d_declaration);
    wined3d_mutex_unlock();
    *input_layout = &input_layout_impl->ID3D11InputLayout_iface;
    ID3D11InputLayout_AddRef(*input_layout);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_IAGetVertexBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers, UINT *strides, UINT *offsets)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p, strides %p, offsets %p.\n",
            iface, start_slot, buffer_count, buffers, strides, offsets);

    wined3d_mutex_lock();
    for (i = 0; i < buffer_count; ++i)
    {
        struct wined3d_buffer *wined3d_buffer = NULL;
        struct d3d_buffer *buffer_impl;

        if (FAILED(wined3d_device_get_stream_source(device->wined3d_device, start_slot + i,
                &wined3d_buffer, &offsets[i], &strides[i])))
        {
            FIXME("Failed to get vertex buffer %u.\n", start_slot + i);
            if (strides)
                strides[i] = 0;
            if (offsets)
                offsets[i] = 0;
        }

        if (!wined3d_buffer)
        {
            buffers[i] = NULL;
            continue;
        }

        buffer_impl = wined3d_buffer_get_parent(wined3d_buffer);
        ID3D11Buffer_AddRef(buffers[i] = &buffer_impl->ID3D11Buffer_iface);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_IAGetIndexBuffer(ID3D11DeviceContext1 *iface,
        ID3D11Buffer **buffer, DXGI_FORMAT *format, UINT *offset)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    enum wined3d_format_id wined3d_format;
    struct wined3d_buffer *wined3d_buffer;
    struct d3d_buffer *buffer_impl;

    TRACE("iface %p, buffer %p, format %p, offset %p.\n", iface, buffer, format, offset);

    wined3d_mutex_lock();
    wined3d_buffer = wined3d_device_get_index_buffer(device->wined3d_device, &wined3d_format, offset);
    *format = dxgi_format_from_wined3dformat(wined3d_format);
    if (!wined3d_buffer)
    {
        wined3d_mutex_unlock();
        *buffer = NULL;
        return;
    }

    buffer_impl = wined3d_buffer_get_parent(wined3d_buffer);
    wined3d_mutex_unlock();
    ID3D11Buffer_AddRef(*buffer = &buffer_impl->ID3D11Buffer_iface);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_GSGetConstantBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d11_immediate_context_get_constant_buffers(iface, WINED3D_SHADER_TYPE_GEOMETRY, start_slot,
            buffer_count, buffers);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_GSGetShader(ID3D11DeviceContext1 *iface,
        ID3D11GeometryShader **shader, ID3D11ClassInstance **class_instances, UINT *class_instance_count)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct d3d_geometry_shader *shader_impl;
    struct wined3d_shader *wined3d_shader;

    TRACE("iface %p, shader %p, class_instances %p, class_instance_count %p.\n",
            iface, shader, class_instances, class_instance_count);

    if (class_instances || class_instance_count)
        FIXME("Dynamic linking not implemented yet.\n");
    if (class_instance_count)
        *class_instance_count = 0;

    wined3d_mutex_lock();
    if (!(wined3d_shader = wined3d_device_get_geometry_shader(device->wined3d_device)))
    {
        wined3d_mutex_unlock();
        *shader = NULL;
        return;
    }

    shader_impl = wined3d_shader_get_parent(wined3d_shader);
    wined3d_mutex_unlock();
    *shader = &shader_impl->ID3D11GeometryShader_iface;
    ID3D11GeometryShader_AddRef(*shader);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_IAGetPrimitiveTopology(ID3D11DeviceContext1 *iface,
        D3D11_PRIMITIVE_TOPOLOGY *topology)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    enum wined3d_primitive_type primitive_type;
    unsigned int patch_vertex_count;

    TRACE("iface %p, topology %p.\n", iface, topology);

    wined3d_mutex_lock();
    wined3d_device_get_primitive_type(device->wined3d_device, &primitive_type, &patch_vertex_count);
    wined3d_mutex_unlock();

    d3d11_primitive_topology_from_wined3d_primitive_type(primitive_type, patch_vertex_count, topology);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_VSGetShaderResources(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11ShaderResourceView **views)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n", iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct wined3d_shader_resource_view *wined3d_view;
        struct d3d_shader_resource_view *view_impl;

        if (!(wined3d_view = wined3d_device_get_vs_resource_view(device->wined3d_device, start_slot + i)))
        {
            views[i] = NULL;
            continue;
        }

        view_impl = wined3d_shader_resource_view_get_parent(wined3d_view);
        views[i] = &view_impl->ID3D11ShaderResourceView_iface;
        ID3D11ShaderResourceView_AddRef(views[i]);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_VSGetSamplers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT sampler_count, ID3D11SamplerState **samplers)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    wined3d_mutex_lock();
    for (i = 0; i < sampler_count; ++i)
    {
        struct wined3d_sampler *wined3d_sampler;
        struct d3d_sampler_state *sampler_impl;

        if (!(wined3d_sampler = wined3d_device_get_vs_sampler(device->wined3d_device, start_slot + i)))
        {
            samplers[i] = NULL;
            continue;
        }

        sampler_impl = wined3d_sampler_get_parent(wined3d_sampler);
        samplers[i] = &sampler_impl->ID3D11SamplerState_iface;
        ID3D11SamplerState_AddRef(samplers[i]);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_GetPredication(ID3D11DeviceContext1 *iface,
        ID3D11Predicate **predicate, BOOL *value)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct wined3d_query *wined3d_predicate;
    struct d3d_query *predicate_impl;

    TRACE("iface %p, predicate %p, value %p.\n", iface, predicate, value);

    wined3d_mutex_lock();
    if (!(wined3d_predicate = wined3d_device_get_predication(device->wined3d_device, value)))
    {
        wined3d_mutex_unlock();
        *predicate = NULL;
        return;
    }

    predicate_impl = wined3d_query_get_parent(wined3d_predicate);
    wined3d_mutex_unlock();
    *predicate = (ID3D11Predicate *)&predicate_impl->ID3D11Query_iface;
    ID3D11Predicate_AddRef(*predicate);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_GSGetShaderResources(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11ShaderResourceView **views)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n", iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct wined3d_shader_resource_view *wined3d_view;
        struct d3d_shader_resource_view *view_impl;

        if (!(wined3d_view = wined3d_device_get_gs_resource_view(device->wined3d_device, start_slot + i)))
        {
            views[i] = NULL;
            continue;
        }

        view_impl = wined3d_shader_resource_view_get_parent(wined3d_view);
        views[i] = &view_impl->ID3D11ShaderResourceView_iface;
        ID3D11ShaderResourceView_AddRef(views[i]);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_GSGetSamplers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT sampler_count, ID3D11SamplerState **samplers)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    wined3d_mutex_lock();
    for (i = 0; i < sampler_count; ++i)
    {
        struct d3d_sampler_state *sampler_impl;
        struct wined3d_sampler *wined3d_sampler;

        if (!(wined3d_sampler = wined3d_device_get_gs_sampler(device->wined3d_device, start_slot + i)))
        {
            samplers[i] = NULL;
            continue;
        }

        sampler_impl = wined3d_sampler_get_parent(wined3d_sampler);
        samplers[i] = &sampler_impl->ID3D11SamplerState_iface;
        ID3D11SamplerState_AddRef(samplers[i]);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_OMGetRenderTargets(ID3D11DeviceContext1 *iface,
        UINT render_target_view_count, ID3D11RenderTargetView **render_target_views,
        ID3D11DepthStencilView **depth_stencil_view)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct wined3d_rendertarget_view *wined3d_view;

    TRACE("iface %p, render_target_view_count %u, render_target_views %p, depth_stencil_view %p.\n",
            iface, render_target_view_count, render_target_views, depth_stencil_view);

    wined3d_mutex_lock();
    if (render_target_views)
    {
        struct d3d_rendertarget_view *view_impl;
        unsigned int i;

        for (i = 0; i < render_target_view_count; ++i)
        {
            if (!(wined3d_view = wined3d_device_get_rendertarget_view(device->wined3d_device, i))
                    || !(view_impl = wined3d_rendertarget_view_get_parent(wined3d_view)))
            {
                render_target_views[i] = NULL;
                continue;
            }

            render_target_views[i] = &view_impl->ID3D11RenderTargetView_iface;
            ID3D11RenderTargetView_AddRef(render_target_views[i]);
        }
    }

    if (depth_stencil_view)
    {
        struct d3d_depthstencil_view *view_impl;

        if (!(wined3d_view = wined3d_device_get_depth_stencil_view(device->wined3d_device))
                || !(view_impl = wined3d_rendertarget_view_get_parent(wined3d_view)))
        {
            *depth_stencil_view = NULL;
        }
        else
        {
            *depth_stencil_view = &view_impl->ID3D11DepthStencilView_iface;
            ID3D11DepthStencilView_AddRef(*depth_stencil_view);
        }
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_OMGetRenderTargetsAndUnorderedAccessViews(
        ID3D11DeviceContext1 *iface,
        UINT render_target_view_count, ID3D11RenderTargetView **render_target_views,
        ID3D11DepthStencilView **depth_stencil_view,
        UINT unordered_access_view_start_slot, UINT unordered_access_view_count,
        ID3D11UnorderedAccessView **unordered_access_views)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct wined3d_unordered_access_view *wined3d_view;
    struct d3d11_unordered_access_view *view_impl;
    unsigned int i;

    TRACE("iface %p, render_target_view_count %u, render_target_views %p, depth_stencil_view %p, "
            "unordered_access_view_start_slot %u, unordered_access_view_count %u, "
            "unordered_access_views %p.\n",
            iface, render_target_view_count, render_target_views, depth_stencil_view,
            unordered_access_view_start_slot, unordered_access_view_count, unordered_access_views);

    if (render_target_views || depth_stencil_view)
        d3d11_immediate_context_OMGetRenderTargets(iface, render_target_view_count,
                render_target_views, depth_stencil_view);

    if (unordered_access_views)
    {
        wined3d_mutex_lock();
        for (i = 0; i < unordered_access_view_count; ++i)
        {
            if (!(wined3d_view = wined3d_device_get_unordered_access_view(device->wined3d_device,
                    unordered_access_view_start_slot + i)))
            {
                unordered_access_views[i] = NULL;
                continue;
            }

            view_impl = wined3d_unordered_access_view_get_parent(wined3d_view);
            unordered_access_views[i] = &view_impl->ID3D11UnorderedAccessView_iface;
            ID3D11UnorderedAccessView_AddRef(unordered_access_views[i]);
        }
        wined3d_mutex_unlock();
    }
}

static void STDMETHODCALLTYPE d3d11_immediate_context_OMGetBlendState(ID3D11DeviceContext1 *iface,
        ID3D11BlendState **blend_state, FLOAT blend_factor[4], UINT *sample_mask)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct wined3d_blend_state *wined3d_state;
    struct d3d_blend_state *blend_state_impl;

    TRACE("iface %p, blend_state %p, blend_factor %p, sample_mask %p.\n",
            iface, blend_state, blend_factor, sample_mask);

    wined3d_mutex_lock();
    if ((wined3d_state = wined3d_device_get_blend_state(device->wined3d_device,
            (struct wined3d_color *)blend_factor)))
    {
        blend_state_impl = wined3d_blend_state_get_parent(wined3d_state);
        ID3D11BlendState_AddRef(*blend_state = &blend_state_impl->ID3D11BlendState_iface);
    }
    else
    {
        *blend_state = NULL;
    }
    *sample_mask = wined3d_device_get_render_state(device->wined3d_device, WINED3D_RS_MULTISAMPLEMASK);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_OMGetDepthStencilState(ID3D11DeviceContext1 *iface,
        ID3D11DepthStencilState **depth_stencil_state, UINT *stencil_ref)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);

    TRACE("iface %p, depth_stencil_state %p, stencil_ref %p.\n",
            iface, depth_stencil_state, stencil_ref);

    if ((*depth_stencil_state = device->depth_stencil_state
            ? &device->depth_stencil_state->ID3D11DepthStencilState_iface : NULL))
        ID3D11DepthStencilState_AddRef(*depth_stencil_state);
    *stencil_ref = device->stencil_ref;
}

static void STDMETHODCALLTYPE d3d11_immediate_context_SOGetTargets(ID3D11DeviceContext1 *iface,
        UINT buffer_count, ID3D11Buffer **buffers)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, buffer_count %u, buffers %p.\n", iface, buffer_count, buffers);

    wined3d_mutex_lock();
    for (i = 0; i < buffer_count; ++i)
    {
        struct wined3d_buffer *wined3d_buffer;
        struct d3d_buffer *buffer_impl;

        if (!(wined3d_buffer = wined3d_device_get_stream_output(device->wined3d_device, i, NULL)))
        {
            buffers[i] = NULL;
            continue;
        }

        buffer_impl = wined3d_buffer_get_parent(wined3d_buffer);
        buffers[i] = &buffer_impl->ID3D11Buffer_iface;
        ID3D11Buffer_AddRef(buffers[i]);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_RSGetState(ID3D11DeviceContext1 *iface,
        ID3D11RasterizerState **rasterizer_state)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct d3d_rasterizer_state *rasterizer_state_impl;
    struct wined3d_rasterizer_state *wined3d_state;

    TRACE("iface %p, rasterizer_state %p.\n", iface, rasterizer_state);

    wined3d_mutex_lock();
    if ((wined3d_state = wined3d_device_get_rasterizer_state(device->wined3d_device)))
    {
        rasterizer_state_impl = wined3d_rasterizer_state_get_parent(wined3d_state);
        ID3D11RasterizerState_AddRef(*rasterizer_state = &rasterizer_state_impl->ID3D11RasterizerState_iface);
    }
    else
    {
        *rasterizer_state = NULL;
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_RSGetViewports(ID3D11DeviceContext1 *iface,
        UINT *viewport_count, D3D11_VIEWPORT *viewports)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct wined3d_viewport wined3d_vp[WINED3D_MAX_VIEWPORTS];
    unsigned int actual_count = ARRAY_SIZE(wined3d_vp), i;

    TRACE("iface %p, viewport_count %p, viewports %p.\n", iface, viewport_count, viewports);

    if (!viewport_count)
        return;

    wined3d_mutex_lock();
    wined3d_device_get_viewports(device->wined3d_device, &actual_count, viewports ? wined3d_vp : NULL);
    wined3d_mutex_unlock();

    if (!viewports)
    {
        *viewport_count = actual_count;
        return;
    }

    if (*viewport_count > actual_count)
        memset(&viewports[actual_count], 0, (*viewport_count - actual_count) * sizeof(*viewports));

    *viewport_count = min(actual_count, *viewport_count);
    for (i = 0; i < *viewport_count; ++i)
    {
        viewports[i].TopLeftX = wined3d_vp[i].x;
        viewports[i].TopLeftY = wined3d_vp[i].y;
        viewports[i].Width = wined3d_vp[i].width;
        viewports[i].Height = wined3d_vp[i].height;
        viewports[i].MinDepth = wined3d_vp[i].min_z;
        viewports[i].MaxDepth = wined3d_vp[i].max_z;
    }
}

static void STDMETHODCALLTYPE d3d11_immediate_context_RSGetScissorRects(ID3D11DeviceContext1 *iface,
        UINT *rect_count, D3D11_RECT *rects)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int actual_count;

    TRACE("iface %p, rect_count %p, rects %p.\n", iface, rect_count, rects);

    if (!rect_count)
        return;

    actual_count = *rect_count;

    wined3d_mutex_lock();
    wined3d_device_get_scissor_rects(device->wined3d_device, &actual_count, rects);
    wined3d_mutex_unlock();

    if (!rects)
    {
        *rect_count = actual_count;
        return;
    }

    if (*rect_count > actual_count)
        memset(&rects[actual_count], 0, (*rect_count - actual_count) * sizeof(*rects));
}

static void STDMETHODCALLTYPE d3d11_immediate_context_HSGetShaderResources(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11ShaderResourceView **views)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n", iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct wined3d_shader_resource_view *wined3d_view;
        struct d3d_shader_resource_view *view_impl;

        if (!(wined3d_view = wined3d_device_get_hs_resource_view(device->wined3d_device, start_slot + i)))
        {
            views[i] = NULL;
            continue;
        }

        view_impl = wined3d_shader_resource_view_get_parent(wined3d_view);
        ID3D11ShaderResourceView_AddRef(views[i] = &view_impl->ID3D11ShaderResourceView_iface);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_HSGetShader(ID3D11DeviceContext1 *iface,
        ID3D11HullShader **shader, ID3D11ClassInstance **class_instances, UINT *class_instance_count)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct d3d11_hull_shader *shader_impl;
    struct wined3d_shader *wined3d_shader;

    TRACE("iface %p, shader %p, class_instances %p, class_instance_count %p.\n",
            iface, shader, class_instances, class_instance_count);

    if (class_instances || class_instance_count)
        FIXME("Dynamic linking not implemented yet.\n");
    if (class_instance_count)
        *class_instance_count = 0;

    wined3d_mutex_lock();
    if (!(wined3d_shader = wined3d_device_get_hull_shader(device->wined3d_device)))
    {
        wined3d_mutex_unlock();
        *shader = NULL;
        return;
    }

    shader_impl = wined3d_shader_get_parent(wined3d_shader);
    wined3d_mutex_unlock();
    ID3D11HullShader_AddRef(*shader = &shader_impl->ID3D11HullShader_iface);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_HSGetSamplers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT sampler_count, ID3D11SamplerState **samplers)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    wined3d_mutex_lock();
    for (i = 0; i < sampler_count; ++i)
    {
        struct wined3d_sampler *wined3d_sampler;
        struct d3d_sampler_state *sampler_impl;

        if (!(wined3d_sampler = wined3d_device_get_hs_sampler(device->wined3d_device, start_slot + i)))
        {
            samplers[i] = NULL;
            continue;
        }

        sampler_impl = wined3d_sampler_get_parent(wined3d_sampler);
        ID3D11SamplerState_AddRef(samplers[i] = &sampler_impl->ID3D11SamplerState_iface);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_HSGetConstantBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d11_immediate_context_get_constant_buffers(iface, WINED3D_SHADER_TYPE_HULL, start_slot,
            buffer_count, buffers);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_DSGetShaderResources(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11ShaderResourceView **views)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n",
            iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct wined3d_shader_resource_view *wined3d_view;
        struct d3d_shader_resource_view *view_impl;

        if (!(wined3d_view = wined3d_device_get_ds_resource_view(device->wined3d_device, start_slot + i)))
        {
            views[i] = NULL;
            continue;
        }

        view_impl = wined3d_shader_resource_view_get_parent(wined3d_view);
        ID3D11ShaderResourceView_AddRef(views[i] = &view_impl->ID3D11ShaderResourceView_iface);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_DSGetShader(ID3D11DeviceContext1 *iface,
        ID3D11DomainShader **shader, ID3D11ClassInstance **class_instances, UINT *class_instance_count)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct d3d11_domain_shader *shader_impl;
    struct wined3d_shader *wined3d_shader;

    TRACE("iface %p, shader %p, class_instances %p, class_instance_count %p.\n",
            iface, shader, class_instances, class_instance_count);

    if (class_instances || class_instance_count)
        FIXME("Dynamic linking not implemented yet.\n");
    if (class_instance_count)
        *class_instance_count = 0;

    wined3d_mutex_lock();
    if (!(wined3d_shader = wined3d_device_get_domain_shader(device->wined3d_device)))
    {
        wined3d_mutex_unlock();
        *shader = NULL;
        return;
    }

    shader_impl = wined3d_shader_get_parent(wined3d_shader);
    wined3d_mutex_unlock();
    ID3D11DomainShader_AddRef(*shader = &shader_impl->ID3D11DomainShader_iface);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_DSGetSamplers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT sampler_count, ID3D11SamplerState **samplers)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    wined3d_mutex_lock();
    for (i = 0; i < sampler_count; ++i)
    {
        struct wined3d_sampler *wined3d_sampler;
        struct d3d_sampler_state *sampler_impl;

        if (!(wined3d_sampler = wined3d_device_get_ds_sampler(device->wined3d_device, start_slot + i)))
        {
            samplers[i] = NULL;
            continue;
        }

        sampler_impl = wined3d_sampler_get_parent(wined3d_sampler);
        ID3D11SamplerState_AddRef(samplers[i] = &sampler_impl->ID3D11SamplerState_iface);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_DSGetConstantBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d11_immediate_context_get_constant_buffers(iface, WINED3D_SHADER_TYPE_DOMAIN, start_slot,
            buffer_count, buffers);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_CSGetShaderResources(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11ShaderResourceView **views)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n", iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct wined3d_shader_resource_view *wined3d_view;
        struct d3d_shader_resource_view *view_impl;

        if (!(wined3d_view = wined3d_device_get_cs_resource_view(device->wined3d_device, start_slot + i)))
        {
            views[i] = NULL;
            continue;
        }

        view_impl = wined3d_shader_resource_view_get_parent(wined3d_view);
        ID3D11ShaderResourceView_AddRef(views[i] = &view_impl->ID3D11ShaderResourceView_iface);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_CSGetUnorderedAccessViews(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11UnorderedAccessView **views)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n", iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct wined3d_unordered_access_view *wined3d_view;
        struct d3d11_unordered_access_view *view_impl;

        if (!(wined3d_view = wined3d_device_get_cs_uav(device->wined3d_device, start_slot + i)))
        {
            views[i] = NULL;
            continue;
        }

        view_impl = wined3d_unordered_access_view_get_parent(wined3d_view);
        ID3D11UnorderedAccessView_AddRef(views[i] = &view_impl->ID3D11UnorderedAccessView_iface);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_CSGetShader(ID3D11DeviceContext1 *iface,
        ID3D11ComputeShader **shader, ID3D11ClassInstance **class_instances, UINT *class_instance_count)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct d3d11_compute_shader *shader_impl;
    struct wined3d_shader *wined3d_shader;

    TRACE("iface %p, shader %p, class_instances %p, class_instance_count %p.\n",
            iface, shader, class_instances, class_instance_count);

    if (class_instances || class_instance_count)
        FIXME("Dynamic linking not implemented yet.\n");
    if (class_instance_count)
        *class_instance_count = 0;

    wined3d_mutex_lock();
    if (!(wined3d_shader = wined3d_device_get_compute_shader(device->wined3d_device)))
    {
        wined3d_mutex_unlock();
        *shader = NULL;
        return;
    }

    shader_impl = wined3d_shader_get_parent(wined3d_shader);
    wined3d_mutex_unlock();
    ID3D11ComputeShader_AddRef(*shader = &shader_impl->ID3D11ComputeShader_iface);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_CSGetSamplers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT sampler_count, ID3D11SamplerState **samplers)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    wined3d_mutex_lock();
    for (i = 0; i < sampler_count; ++i)
    {
        struct wined3d_sampler *wined3d_sampler;
        struct d3d_sampler_state *sampler_impl;

        if (!(wined3d_sampler = wined3d_device_get_cs_sampler(device->wined3d_device, start_slot + i)))
        {
            samplers[i] = NULL;
            continue;
        }

        sampler_impl = wined3d_sampler_get_parent(wined3d_sampler);
        ID3D11SamplerState_AddRef(samplers[i] = &sampler_impl->ID3D11SamplerState_iface);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_CSGetConstantBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d11_immediate_context_get_constant_buffers(iface, WINED3D_SHADER_TYPE_COMPUTE, start_slot,
            buffer_count, buffers);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_ClearState(ID3D11DeviceContext1 *iface)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    static const float blend_factor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    unsigned int i, j;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_device_set_vertex_shader(device->wined3d_device, NULL);
    wined3d_device_set_hull_shader(device->wined3d_device, NULL);
    wined3d_device_set_domain_shader(device->wined3d_device, NULL);
    wined3d_device_set_geometry_shader(device->wined3d_device, NULL);
    wined3d_device_set_pixel_shader(device->wined3d_device, NULL);
    wined3d_device_set_compute_shader(device->wined3d_device, NULL);
    for (i = 0; i < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
    {
        wined3d_device_set_vs_sampler(device->wined3d_device, i, NULL);
        wined3d_device_set_hs_sampler(device->wined3d_device, i, NULL);
        wined3d_device_set_ds_sampler(device->wined3d_device, i, NULL);
        wined3d_device_set_gs_sampler(device->wined3d_device, i, NULL);
        wined3d_device_set_ps_sampler(device->wined3d_device, i, NULL);
        wined3d_device_set_cs_sampler(device->wined3d_device, i, NULL);
    }
    for (i = 0; i < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        wined3d_device_set_vs_resource_view(device->wined3d_device, i, NULL);
        wined3d_device_set_hs_resource_view(device->wined3d_device, i, NULL);
        wined3d_device_set_ds_resource_view(device->wined3d_device, i, NULL);
        wined3d_device_set_gs_resource_view(device->wined3d_device, i, NULL);
        wined3d_device_set_ps_resource_view(device->wined3d_device, i, NULL);
        wined3d_device_set_cs_resource_view(device->wined3d_device, i, NULL);
    }
    for (i = 0; i < WINED3D_SHADER_TYPE_COUNT; ++i)
    {
        for (j = 0; j < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++j)
            wined3d_device_set_constant_buffer(device->wined3d_device, i, j, NULL);
    }
    for (i = 0; i < D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        wined3d_device_set_stream_source(device->wined3d_device, i, NULL, 0, 0);
    }
    wined3d_device_set_index_buffer(device->wined3d_device, NULL, WINED3DFMT_UNKNOWN, 0);
    wined3d_device_set_vertex_declaration(device->wined3d_device, NULL);
    wined3d_device_set_primitive_type(device->wined3d_device, WINED3D_PT_UNDEFINED, 0);
    for (i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        wined3d_device_set_rendertarget_view(device->wined3d_device, i, NULL, FALSE);
    }
    wined3d_device_set_depth_stencil_view(device->wined3d_device, NULL);
    for (i = 0; i < D3D11_PS_CS_UAV_REGISTER_COUNT; ++i)
    {
        wined3d_device_set_unordered_access_view(device->wined3d_device, i, NULL, ~0u);
        wined3d_device_set_cs_uav(device->wined3d_device, i, NULL, ~0u);
    }
    ID3D11DeviceContext1_OMSetDepthStencilState(iface, NULL, 0);
    ID3D11DeviceContext1_OMSetBlendState(iface, NULL, blend_factor, D3D11_DEFAULT_SAMPLE_MASK);
    ID3D11DeviceContext1_RSSetViewports(iface, 0, NULL);
    ID3D11DeviceContext1_RSSetScissorRects(iface, 0, NULL);
    ID3D11DeviceContext1_RSSetState(iface, NULL);
    for (i = 0; i < D3D11_SO_BUFFER_SLOT_COUNT; ++i)
    {
        wined3d_device_set_stream_output(device->wined3d_device, i, NULL, 0);
    }
    wined3d_device_set_predication(device->wined3d_device, NULL, FALSE);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_Flush(ID3D11DeviceContext1 *iface)
{
    FIXME("iface %p stub!\n", iface);
}

static D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE d3d11_immediate_context_GetType(ID3D11DeviceContext1 *iface)
{
    TRACE("iface %p.\n", iface);

    return D3D11_DEVICE_CONTEXT_IMMEDIATE;
}

static UINT STDMETHODCALLTYPE d3d11_immediate_context_GetContextFlags(ID3D11DeviceContext1 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT STDMETHODCALLTYPE d3d11_immediate_context_FinishCommandList(ID3D11DeviceContext1 *iface,
        BOOL restore, ID3D11CommandList **command_list)
{
    FIXME("iface %p, restore %#x, command_list %p stub!\n", iface, restore, command_list);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d3d11_immediate_context_CopySubresourceRegion1(ID3D11DeviceContext1 *iface,
        ID3D11Resource *dst_resource, UINT dst_subresource_idx, UINT dst_x, UINT dst_y, UINT dst_z,
        ID3D11Resource *src_resource, UINT src_subresource_idx, const D3D11_BOX *src_box, UINT flags)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct wined3d_resource *wined3d_dst_resource, *wined3d_src_resource;
    struct wined3d_box wined3d_src_box;

    TRACE("iface %p, dst_resource %p, dst_subresource_idx %u, dst_x %u, dst_y %u, dst_z %u, "
            "src_resource %p, src_subresource_idx %u, src_box %p, flags %#x.\n",
            iface, dst_resource, dst_subresource_idx, dst_x, dst_y, dst_z,
            src_resource, src_subresource_idx, src_box, flags);

    if (src_box)
        wined3d_box_set(&wined3d_src_box, src_box->left, src_box->top,
                src_box->right, src_box->bottom, src_box->front, src_box->back);

    wined3d_dst_resource = wined3d_resource_from_d3d11_resource(dst_resource);
    wined3d_src_resource = wined3d_resource_from_d3d11_resource(src_resource);
    wined3d_mutex_lock();
    wined3d_device_copy_sub_resource_region(device->wined3d_device, wined3d_dst_resource, dst_subresource_idx,
            dst_x, dst_y, dst_z, wined3d_src_resource, src_subresource_idx, src_box ? &wined3d_src_box : NULL, flags);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_UpdateSubresource1(ID3D11DeviceContext1 *iface,
        ID3D11Resource *resource, UINT subresource_idx, const D3D11_BOX *box, const void *data,
        UINT row_pitch, UINT depth_pitch, UINT flags)
{
    struct d3d_device *device = device_from_immediate_ID3D11DeviceContext1(iface);
    struct wined3d_resource *wined3d_resource;
    struct wined3d_box wined3d_box;

    TRACE("iface %p, resource %p, subresource_idx %u, box %p, data %p, row_pitch %u, depth_pitch %u, flags %#x.\n",
            iface, resource, subresource_idx, box, data, row_pitch, depth_pitch, flags);

    if (box)
        wined3d_box_set(&wined3d_box, box->left, box->top, box->right, box->bottom,
                box->front, box->back);

    wined3d_resource = wined3d_resource_from_d3d11_resource(resource);
    wined3d_mutex_lock();
    wined3d_device_update_sub_resource(device->wined3d_device, wined3d_resource, subresource_idx,
            box ? &wined3d_box : NULL, data, row_pitch, depth_pitch, flags);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_immediate_context_DiscardResource(ID3D11DeviceContext1 *iface,
        ID3D11Resource *resource)
{
    FIXME("iface %p, resource %p stub!\n", iface, resource);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_DiscardView(ID3D11DeviceContext1 *iface, ID3D11View *view)
{
    FIXME("iface %p, view %p stub!\n", iface, view);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_VSSetConstantBuffers1(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer * const *buffers, const UINT *first_constant,
        const UINT *num_constants)
{
    FIXME("iface %p, start_slot %u, buffer_count %u, buffers %p, first_constant %p, num_constants %p stub!\n",
            iface, start_slot, buffer_count, buffers, first_constant, num_constants);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_HSSetConstantBuffers1(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer * const *buffers, const UINT *first_constant,
        const UINT *num_constants)
{
    FIXME("iface %p, start_slot %u, buffer_count %u, buffers %p, first_constant %p, num_constants %p stub!\n",
            iface, start_slot, buffer_count, buffers, first_constant, num_constants);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_DSSetConstantBuffers1(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer * const *buffers, const UINT *first_constant,
        const UINT *num_constants)
{
    FIXME("iface %p, start_slot %u, buffer_count %u, buffers %p, first_constant %p, num_constants %p stub!\n",
            iface, start_slot, buffer_count, buffers, first_constant, num_constants);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_GSSetConstantBuffers1(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer * const *buffers, const UINT *first_constant,
        const UINT *num_constants)
{
    FIXME("iface %p, start_slot %u, buffer_count %u, buffers %p, first_constant %p, num_constants %p stub!\n",
            iface, start_slot, buffer_count, buffers, first_constant, num_constants);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_PSSetConstantBuffers1(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer * const *buffers, const UINT *first_constant,
        const UINT *num_constants)
{
    FIXME("iface %p, start_slot %u, buffer_count %u, buffers %p, first_constant %p, num_constants %p stub!\n",
            iface, start_slot, buffer_count, buffers, first_constant, num_constants);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_CSSetConstantBuffers1(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer * const *buffers, const UINT *first_constant,
        const UINT *num_constants)
{
    FIXME("iface %p, start_slot %u, buffer_count %u, buffers %p, first_constant %p, num_constants %p stub!\n",
            iface, start_slot, buffer_count, buffers, first_constant, num_constants);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_VSGetConstantBuffers1(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers, UINT *first_constant, UINT *num_constants)
{
    FIXME("iface %p, start_slot %u, buffer_count %u, buffers %p, first_constant %p, num_constants %p stub!\n",
            iface, start_slot, buffer_count, buffers, first_constant, num_constants);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_HSGetConstantBuffers1(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers, UINT *first_constant, UINT *num_constants)
{
    FIXME("iface %p, start_slot %u, buffer_count %u, buffers %p, first_constant %p, num_constants %p stub!\n",
            iface, start_slot, buffer_count, buffers, first_constant, num_constants);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_DSGetConstantBuffers1(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers, UINT *first_constant, UINT *num_constants)
{
    FIXME("iface %p, start_slot %u, buffer_count %u, buffers %p, first_constant %p, num_constants %p stub!\n",
            iface, start_slot, buffer_count, buffers, first_constant, num_constants);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_GSGetConstantBuffers1(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers, UINT *first_constant, UINT *num_constants)
{
    FIXME("iface %p, start_slot %u, buffer_count %u, buffers %p, first_constant %p, num_constants %p stub!\n",
            iface, start_slot, buffer_count, buffers, first_constant, num_constants);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_PSGetConstantBuffers1(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers, UINT *first_constant, UINT *num_constants)
{
    FIXME("iface %p, start_slot %u, buffer_count %u, buffers %p, first_constant %p, num_constants %p stub!\n",
            iface, start_slot, buffer_count, buffers, first_constant, num_constants);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_CSGetConstantBuffers1(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers, UINT *first_constant, UINT *num_constants)
{
    FIXME("iface %p, start_slot %u, buffer_count %u, buffers %p, first_constant %p, num_constants %p stub!\n",
            iface, start_slot, buffer_count, buffers, first_constant, num_constants);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_SwapDeviceContextState(ID3D11DeviceContext1 *iface,
        ID3DDeviceContextState *state, ID3DDeviceContextState **prev_state)
{
    FIXME("iface %p, state %p, prev_state %p stub!\n", iface, state, prev_state);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_ClearView(ID3D11DeviceContext1 *iface, ID3D11View *view,
        const FLOAT color[4], const D3D11_RECT *rect, UINT num_rects)
{
    FIXME("iface %p, view %p, color %p, rect %p, num_rects %u stub!\n", iface, view, color, rect, num_rects);
}

static void STDMETHODCALLTYPE d3d11_immediate_context_DiscardView1(ID3D11DeviceContext1 *iface, ID3D11View *view,
        const D3D11_RECT *rects, UINT num_rects)
{
    FIXME("iface %p, view %p, rects %p, num_rects %u stub!\n", iface, view, rects, num_rects);
}

static const struct ID3D11DeviceContext1Vtbl d3d11_immediate_context_vtbl =
{
    /* IUnknown methods */
    d3d11_immediate_context_QueryInterface,
    d3d11_immediate_context_AddRef,
    d3d11_immediate_context_Release,
    /* ID3D11DeviceChild methods */
    d3d11_immediate_context_GetDevice,
    d3d11_immediate_context_GetPrivateData,
    d3d11_immediate_context_SetPrivateData,
    d3d11_immediate_context_SetPrivateDataInterface,
    /* ID3D11DeviceContext methods */
    d3d11_immediate_context_VSSetConstantBuffers,
    d3d11_immediate_context_PSSetShaderResources,
    d3d11_immediate_context_PSSetShader,
    d3d11_immediate_context_PSSetSamplers,
    d3d11_immediate_context_VSSetShader,
    d3d11_immediate_context_DrawIndexed,
    d3d11_immediate_context_Draw,
    d3d11_immediate_context_Map,
    d3d11_immediate_context_Unmap,
    d3d11_immediate_context_PSSetConstantBuffers,
    d3d11_immediate_context_IASetInputLayout,
    d3d11_immediate_context_IASetVertexBuffers,
    d3d11_immediate_context_IASetIndexBuffer,
    d3d11_immediate_context_DrawIndexedInstanced,
    d3d11_immediate_context_DrawInstanced,
    d3d11_immediate_context_GSSetConstantBuffers,
    d3d11_immediate_context_GSSetShader,
    d3d11_immediate_context_IASetPrimitiveTopology,
    d3d11_immediate_context_VSSetShaderResources,
    d3d11_immediate_context_VSSetSamplers,
    d3d11_immediate_context_Begin,
    d3d11_immediate_context_End,
    d3d11_immediate_context_GetData,
    d3d11_immediate_context_SetPredication,
    d3d11_immediate_context_GSSetShaderResources,
    d3d11_immediate_context_GSSetSamplers,
    d3d11_immediate_context_OMSetRenderTargets,
    d3d11_immediate_context_OMSetRenderTargetsAndUnorderedAccessViews,
    d3d11_immediate_context_OMSetBlendState,
    d3d11_immediate_context_OMSetDepthStencilState,
    d3d11_immediate_context_SOSetTargets,
    d3d11_immediate_context_DrawAuto,
    d3d11_immediate_context_DrawIndexedInstancedIndirect,
    d3d11_immediate_context_DrawInstancedIndirect,
    d3d11_immediate_context_Dispatch,
    d3d11_immediate_context_DispatchIndirect,
    d3d11_immediate_context_RSSetState,
    d3d11_immediate_context_RSSetViewports,
    d3d11_immediate_context_RSSetScissorRects,
    d3d11_immediate_context_CopySubresourceRegion,
    d3d11_immediate_context_CopyResource,
    d3d11_immediate_context_UpdateSubresource,
    d3d11_immediate_context_CopyStructureCount,
    d3d11_immediate_context_ClearRenderTargetView,
    d3d11_immediate_context_ClearUnorderedAccessViewUint,
    d3d11_immediate_context_ClearUnorderedAccessViewFloat,
    d3d11_immediate_context_ClearDepthStencilView,
    d3d11_immediate_context_GenerateMips,
    d3d11_immediate_context_SetResourceMinLOD,
    d3d11_immediate_context_GetResourceMinLOD,
    d3d11_immediate_context_ResolveSubresource,
    d3d11_immediate_context_ExecuteCommandList,
    d3d11_immediate_context_HSSetShaderResources,
    d3d11_immediate_context_HSSetShader,
    d3d11_immediate_context_HSSetSamplers,
    d3d11_immediate_context_HSSetConstantBuffers,
    d3d11_immediate_context_DSSetShaderResources,
    d3d11_immediate_context_DSSetShader,
    d3d11_immediate_context_DSSetSamplers,
    d3d11_immediate_context_DSSetConstantBuffers,
    d3d11_immediate_context_CSSetShaderResources,
    d3d11_immediate_context_CSSetUnorderedAccessViews,
    d3d11_immediate_context_CSSetShader,
    d3d11_immediate_context_CSSetSamplers,
    d3d11_immediate_context_CSSetConstantBuffers,
    d3d11_immediate_context_VSGetConstantBuffers,
    d3d11_immediate_context_PSGetShaderResources,
    d3d11_immediate_context_PSGetShader,
    d3d11_immediate_context_PSGetSamplers,
    d3d11_immediate_context_VSGetShader,
    d3d11_immediate_context_PSGetConstantBuffers,
    d3d11_immediate_context_IAGetInputLayout,
    d3d11_immediate_context_IAGetVertexBuffers,
    d3d11_immediate_context_IAGetIndexBuffer,
    d3d11_immediate_context_GSGetConstantBuffers,
    d3d11_immediate_context_GSGetShader,
    d3d11_immediate_context_IAGetPrimitiveTopology,
    d3d11_immediate_context_VSGetShaderResources,
    d3d11_immediate_context_VSGetSamplers,
    d3d11_immediate_context_GetPredication,
    d3d11_immediate_context_GSGetShaderResources,
    d3d11_immediate_context_GSGetSamplers,
    d3d11_immediate_context_OMGetRenderTargets,
    d3d11_immediate_context_OMGetRenderTargetsAndUnorderedAccessViews,
    d3d11_immediate_context_OMGetBlendState,
    d3d11_immediate_context_OMGetDepthStencilState,
    d3d11_immediate_context_SOGetTargets,
    d3d11_immediate_context_RSGetState,
    d3d11_immediate_context_RSGetViewports,
    d3d11_immediate_context_RSGetScissorRects,
    d3d11_immediate_context_HSGetShaderResources,
    d3d11_immediate_context_HSGetShader,
    d3d11_immediate_context_HSGetSamplers,
    d3d11_immediate_context_HSGetConstantBuffers,
    d3d11_immediate_context_DSGetShaderResources,
    d3d11_immediate_context_DSGetShader,
    d3d11_immediate_context_DSGetSamplers,
    d3d11_immediate_context_DSGetConstantBuffers,
    d3d11_immediate_context_CSGetShaderResources,
    d3d11_immediate_context_CSGetUnorderedAccessViews,
    d3d11_immediate_context_CSGetShader,
    d3d11_immediate_context_CSGetSamplers,
    d3d11_immediate_context_CSGetConstantBuffers,
    d3d11_immediate_context_ClearState,
    d3d11_immediate_context_Flush,
    d3d11_immediate_context_GetType,
    d3d11_immediate_context_GetContextFlags,
    d3d11_immediate_context_FinishCommandList,
    /* ID3D11DeviceContext1 methods */
    d3d11_immediate_context_CopySubresourceRegion1,
    d3d11_immediate_context_UpdateSubresource1,
    d3d11_immediate_context_DiscardResource,
    d3d11_immediate_context_DiscardView,
    d3d11_immediate_context_VSSetConstantBuffers1,
    d3d11_immediate_context_HSSetConstantBuffers1,
    d3d11_immediate_context_DSSetConstantBuffers1,
    d3d11_immediate_context_GSSetConstantBuffers1,
    d3d11_immediate_context_PSSetConstantBuffers1,
    d3d11_immediate_context_CSSetConstantBuffers1,
    d3d11_immediate_context_VSGetConstantBuffers1,
    d3d11_immediate_context_HSGetConstantBuffers1,
    d3d11_immediate_context_DSGetConstantBuffers1,
    d3d11_immediate_context_GSGetConstantBuffers1,
    d3d11_immediate_context_PSGetConstantBuffers1,
    d3d11_immediate_context_CSGetConstantBuffers1,
    d3d11_immediate_context_SwapDeviceContextState,
    d3d11_immediate_context_ClearView,
    d3d11_immediate_context_DiscardView1,
};

/* ID3D11Multithread methods */

static inline struct d3d11_immediate_context *impl_from_ID3D11Multithread(ID3D11Multithread *iface)
{
    return CONTAINING_RECORD(iface, struct d3d11_immediate_context, ID3D11Multithread_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_multithread_QueryInterface(ID3D11Multithread *iface,
        REFIID iid, void **out)
{
    struct d3d11_immediate_context *context = impl_from_ID3D11Multithread(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    return d3d11_immediate_context_QueryInterface(&context->ID3D11DeviceContext1_iface, iid, out);
}

static ULONG STDMETHODCALLTYPE d3d11_multithread_AddRef(ID3D11Multithread *iface)
{
    struct d3d11_immediate_context *context = impl_from_ID3D11Multithread(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_immediate_context_AddRef(&context->ID3D11DeviceContext1_iface);
}

static ULONG STDMETHODCALLTYPE d3d11_multithread_Release(ID3D11Multithread *iface)
{
    struct d3d11_immediate_context *context = impl_from_ID3D11Multithread(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_immediate_context_Release(&context->ID3D11DeviceContext1_iface);
}

static void STDMETHODCALLTYPE d3d11_multithread_Enter(ID3D11Multithread *iface)
{
    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
}

static void STDMETHODCALLTYPE d3d11_multithread_Leave(ID3D11Multithread *iface)
{
    TRACE("iface %p.\n", iface);

    wined3d_mutex_unlock();
}

static BOOL STDMETHODCALLTYPE d3d11_multithread_SetMultithreadProtected(
        ID3D11Multithread *iface, BOOL enable)
{
    FIXME("iface %p, enable %#x stub!\n", iface, enable);

    return TRUE;
}

static BOOL STDMETHODCALLTYPE d3d11_multithread_GetMultithreadProtected(ID3D11Multithread *iface)
{
    FIXME("iface %p stub!\n", iface);

    return TRUE;
}

static const struct ID3D11MultithreadVtbl d3d11_multithread_vtbl =
{
    d3d11_multithread_QueryInterface,
    d3d11_multithread_AddRef,
    d3d11_multithread_Release,
    d3d11_multithread_Enter,
    d3d11_multithread_Leave,
    d3d11_multithread_SetMultithreadProtected,
    d3d11_multithread_GetMultithreadProtected,
};

static void d3d11_immediate_context_init(struct d3d11_immediate_context *context, struct d3d_device *device)
{
    context->ID3D11DeviceContext1_iface.lpVtbl = &d3d11_immediate_context_vtbl;
    context->ID3D11Multithread_iface.lpVtbl = &d3d11_multithread_vtbl;
    context->refcount = 1;

    ID3D11Device2_AddRef(&device->ID3D11Device2_iface);

    wined3d_private_store_init(&context->private_store);
}

static void d3d11_immediate_context_destroy(struct d3d11_immediate_context *context)
{
    wined3d_private_store_cleanup(&context->private_store);
}

/* ID3D11Device methods */

static HRESULT STDMETHODCALLTYPE d3d11_device_QueryInterface(ID3D11Device2 *iface, REFIID iid, void **out)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    return IUnknown_QueryInterface(device->outer_unk, iid, out);
}

static ULONG STDMETHODCALLTYPE d3d11_device_AddRef(ID3D11Device2 *iface)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    return IUnknown_AddRef(device->outer_unk);
}

static ULONG STDMETHODCALLTYPE d3d11_device_Release(ID3D11Device2 *iface)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    return IUnknown_Release(device->outer_unk);
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateBuffer(ID3D11Device2 *iface, const D3D11_BUFFER_DESC *desc,
        const D3D11_SUBRESOURCE_DATA *data, ID3D11Buffer **buffer)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d_buffer *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, data %p, buffer %p.\n", iface, desc, data, buffer);

    if (FAILED(hr = d3d_buffer_create(device, desc, data, &object)))
        return hr;

    *buffer = &object->ID3D11Buffer_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateTexture1D(ID3D11Device2 *iface,
        const D3D11_TEXTURE1D_DESC *desc, const D3D11_SUBRESOURCE_DATA *data, ID3D11Texture1D **texture)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d_texture1d *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, data %p, texture %p.\n", iface, desc, data, texture);

    if (FAILED(hr = d3d_texture1d_create(device, desc, data, &object)))
        return hr;

    *texture = &object->ID3D11Texture1D_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateTexture2D(ID3D11Device2 *iface,
        const D3D11_TEXTURE2D_DESC *desc, const D3D11_SUBRESOURCE_DATA *data, ID3D11Texture2D **texture)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d_texture2d *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, data %p, texture %p.\n", iface, desc, data, texture);

    if (FAILED(hr = d3d_texture2d_create(device, desc, data, &object)))
        return hr;

    *texture = &object->ID3D11Texture2D_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateTexture3D(ID3D11Device2 *iface,
        const D3D11_TEXTURE3D_DESC *desc, const D3D11_SUBRESOURCE_DATA *data, ID3D11Texture3D **texture)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d_texture3d *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, data %p, texture %p.\n", iface, desc, data, texture);

    if (FAILED(hr = d3d_texture3d_create(device, desc, data, &object)))
        return hr;

    *texture = &object->ID3D11Texture3D_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateShaderResourceView(ID3D11Device2 *iface,
        ID3D11Resource *resource, const D3D11_SHADER_RESOURCE_VIEW_DESC *desc, ID3D11ShaderResourceView **view)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d_shader_resource_view *object;
    HRESULT hr;

    TRACE("iface %p, resource %p, desc %p, view %p.\n", iface, resource, desc, view);

    if (!resource)
        return E_INVALIDARG;

    if (FAILED(hr = d3d_shader_resource_view_create(device, resource, desc, &object)))
        return hr;

    *view = &object->ID3D11ShaderResourceView_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateUnorderedAccessView(ID3D11Device2 *iface,
        ID3D11Resource *resource, const D3D11_UNORDERED_ACCESS_VIEW_DESC *desc, ID3D11UnorderedAccessView **view)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d11_unordered_access_view *object;
    HRESULT hr;

    TRACE("iface %p, resource %p, desc %p, view %p.\n", iface, resource, desc, view);

    if (FAILED(hr = d3d11_unordered_access_view_create(device, resource, desc, &object)))
        return hr;

    *view = &object->ID3D11UnorderedAccessView_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateRenderTargetView(ID3D11Device2 *iface,
        ID3D11Resource *resource, const D3D11_RENDER_TARGET_VIEW_DESC *desc, ID3D11RenderTargetView **view)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d_rendertarget_view *object;
    HRESULT hr;

    TRACE("iface %p, resource %p, desc %p, view %p.\n", iface, resource, desc, view);

    if (!resource)
        return E_INVALIDARG;

    if (FAILED(hr = d3d_rendertarget_view_create(device, resource, desc, &object)))
        return hr;

    *view = &object->ID3D11RenderTargetView_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateDepthStencilView(ID3D11Device2 *iface,
        ID3D11Resource *resource, const D3D11_DEPTH_STENCIL_VIEW_DESC *desc, ID3D11DepthStencilView **view)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d_depthstencil_view *object;
    HRESULT hr;

    TRACE("iface %p, resource %p, desc %p, view %p.\n", iface, resource, desc, view);

    if (FAILED(hr = d3d_depthstencil_view_create(device, resource, desc, &object)))
        return hr;

    *view = &object->ID3D11DepthStencilView_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateInputLayout(ID3D11Device2 *iface,
        const D3D11_INPUT_ELEMENT_DESC *element_descs, UINT element_count, const void *shader_byte_code,
        SIZE_T shader_byte_code_length, ID3D11InputLayout **input_layout)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d_input_layout *object;
    HRESULT hr;

    TRACE("iface %p, element_descs %p, element_count %u, shader_byte_code %p, shader_byte_code_length %lu, "
            "input_layout %p.\n", iface, element_descs, element_count, shader_byte_code,
            shader_byte_code_length, input_layout);

    if (FAILED(hr = d3d_input_layout_create(device, element_descs, element_count,
            shader_byte_code, shader_byte_code_length, &object)))
        return hr;

    *input_layout = &object->ID3D11InputLayout_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateVertexShader(ID3D11Device2 *iface, const void *byte_code,
        SIZE_T byte_code_length, ID3D11ClassLinkage *class_linkage, ID3D11VertexShader **shader)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d_vertex_shader *object;
    HRESULT hr;

    TRACE("iface %p, byte_code %p, byte_code_length %lu, class_linkage %p, shader %p.\n",
            iface, byte_code, byte_code_length, class_linkage, shader);

    if (class_linkage)
        FIXME("Class linkage is not implemented yet.\n");

    if (FAILED(hr = d3d_vertex_shader_create(device, byte_code, byte_code_length, &object)))
        return hr;

    *shader = &object->ID3D11VertexShader_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateGeometryShader(ID3D11Device2 *iface, const void *byte_code,
        SIZE_T byte_code_length, ID3D11ClassLinkage *class_linkage, ID3D11GeometryShader **shader)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d_geometry_shader *object;
    HRESULT hr;

    TRACE("iface %p, byte_code %p, byte_code_length %lu, class_linkage %p, shader %p.\n",
            iface, byte_code, byte_code_length, class_linkage, shader);

    if (class_linkage)
        FIXME("Class linkage is not implemented yet.\n");

    if (FAILED(hr = d3d_geometry_shader_create(device, byte_code, byte_code_length,
            NULL, 0, NULL, 0, 0, &object)))
        return hr;

    *shader = &object->ID3D11GeometryShader_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateGeometryShaderWithStreamOutput(ID3D11Device2 *iface,
        const void *byte_code, SIZE_T byte_code_length, const D3D11_SO_DECLARATION_ENTRY *so_entries,
        UINT entry_count, const UINT *buffer_strides, UINT strides_count, UINT rasterizer_stream,
        ID3D11ClassLinkage *class_linkage, ID3D11GeometryShader **shader)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d_geometry_shader *object;
    HRESULT hr;

    TRACE("iface %p, byte_code %p, byte_code_length %lu, so_entries %p, entry_count %u, "
            "buffer_strides %p, strides_count %u, rasterizer_stream %u, class_linkage %p, shader %p.\n",
            iface, byte_code, byte_code_length, so_entries, entry_count, buffer_strides, strides_count,
            rasterizer_stream, class_linkage, shader);

    if (class_linkage)
        FIXME("Class linkage is not implemented yet.\n");

    if (FAILED(hr = d3d_geometry_shader_create(device, byte_code, byte_code_length,
            so_entries, entry_count, buffer_strides, strides_count, rasterizer_stream, &object)))
    {
        *shader = NULL;
        return hr;
    }

    *shader = &object->ID3D11GeometryShader_iface;

    return hr;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreatePixelShader(ID3D11Device2 *iface, const void *byte_code,
        SIZE_T byte_code_length, ID3D11ClassLinkage *class_linkage, ID3D11PixelShader **shader)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d_pixel_shader *object;
    HRESULT hr;

    TRACE("iface %p, byte_code %p, byte_code_length %lu, class_linkage %p, shader %p.\n",
            iface, byte_code, byte_code_length, class_linkage, shader);

    if (class_linkage)
        FIXME("Class linkage is not implemented yet.\n");

    if (FAILED(hr = d3d_pixel_shader_create(device, byte_code, byte_code_length, &object)))
        return hr;

    *shader = &object->ID3D11PixelShader_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateHullShader(ID3D11Device2 *iface, const void *byte_code,
        SIZE_T byte_code_length, ID3D11ClassLinkage *class_linkage, ID3D11HullShader **shader)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d11_hull_shader *object;
    HRESULT hr;

    TRACE("iface %p, byte_code %p, byte_code_length %lu, class_linkage %p, shader %p.\n",
            iface, byte_code, byte_code_length, class_linkage, shader);

    if (class_linkage)
        FIXME("Class linkage is not implemented yet.\n");

    if (FAILED(hr = d3d11_hull_shader_create(device, byte_code, byte_code_length, &object)))
        return hr;

    *shader = &object->ID3D11HullShader_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateDomainShader(ID3D11Device2 *iface, const void *byte_code,
        SIZE_T byte_code_length, ID3D11ClassLinkage *class_linkage, ID3D11DomainShader **shader)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d11_domain_shader *object;
    HRESULT hr;

    TRACE("iface %p, byte_code %p, byte_code_length %lu, class_linkage %p, shader %p.\n",
            iface, byte_code, byte_code_length, class_linkage, shader);

    if (class_linkage)
        FIXME("Class linkage is not implemented yet.\n");

    if (FAILED(hr = d3d11_domain_shader_create(device, byte_code, byte_code_length, &object)))
        return hr;

    *shader = &object->ID3D11DomainShader_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateComputeShader(ID3D11Device2 *iface, const void *byte_code,
        SIZE_T byte_code_length, ID3D11ClassLinkage *class_linkage, ID3D11ComputeShader **shader)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d11_compute_shader *object;
    HRESULT hr;

    TRACE("iface %p, byte_code %p, byte_code_length %lu, class_linkage %p, shader %p.\n",
            iface, byte_code, byte_code_length, class_linkage, shader);

    if (class_linkage)
        FIXME("Class linkage is not implemented yet.\n");

    if (FAILED(hr = d3d11_compute_shader_create(device, byte_code, byte_code_length, &object)))
        return hr;

    *shader = &object->ID3D11ComputeShader_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateClassLinkage(ID3D11Device2 *iface,
        ID3D11ClassLinkage **class_linkage)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d11_class_linkage *object;
    HRESULT hr;

    TRACE("iface %p, class_linkage %p.\n", iface, class_linkage);

    if (FAILED(hr = d3d11_class_linkage_create(device, &object)))
        return hr;

    *class_linkage = &object->ID3D11ClassLinkage_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateBlendState(ID3D11Device2 *iface,
        const D3D11_BLEND_DESC *desc, ID3D11BlendState **blend_state)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d_blend_state *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, blend_state %p.\n", iface, desc, blend_state);

    if (FAILED(hr = d3d_blend_state_create(device, desc, &object)))
        return hr;

    *blend_state = &object->ID3D11BlendState_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateDepthStencilState(ID3D11Device2 *iface,
        const D3D11_DEPTH_STENCIL_DESC *desc, ID3D11DepthStencilState **depth_stencil_state)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d_depthstencil_state *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, depth_stencil_state %p.\n", iface, desc, depth_stencil_state);

    if (FAILED(hr = d3d_depthstencil_state_create(device, desc, &object)))
        return hr;

    *depth_stencil_state = &object->ID3D11DepthStencilState_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateRasterizerState(ID3D11Device2 *iface,
        const D3D11_RASTERIZER_DESC *desc, ID3D11RasterizerState **rasterizer_state)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d_rasterizer_state *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, rasterizer_state %p.\n", iface, desc, rasterizer_state);

    if (FAILED(hr = d3d_rasterizer_state_create(device, desc, &object)))
        return hr;

    *rasterizer_state = &object->ID3D11RasterizerState_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateSamplerState(ID3D11Device2 *iface,
        const D3D11_SAMPLER_DESC *desc, ID3D11SamplerState **sampler_state)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d_sampler_state *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, sampler_state %p.\n", iface, desc, sampler_state);

    if (FAILED(hr = d3d_sampler_state_create(device, desc, &object)))
        return hr;

    *sampler_state = &object->ID3D11SamplerState_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateQuery(ID3D11Device2 *iface,
        const D3D11_QUERY_DESC *desc, ID3D11Query **query)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d_query *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, query %p.\n", iface, desc, query);

    if (FAILED(hr = d3d_query_create(device, desc, FALSE, &object)))
        return hr;

    if (query)
    {
        *query = &object->ID3D11Query_iface;
        return S_OK;
    }

    ID3D11Query_Release(&object->ID3D11Query_iface);
    return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreatePredicate(ID3D11Device2 *iface, const D3D11_QUERY_DESC *desc,
        ID3D11Predicate **predicate)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d_query *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, predicate %p.\n", iface, desc, predicate);

    if (FAILED(hr = d3d_query_create(device, desc, TRUE, &object)))
        return hr;

    if (predicate)
    {
        *predicate = (ID3D11Predicate *)&object->ID3D11Query_iface;
        return S_OK;
    }

    ID3D11Query_Release(&object->ID3D11Query_iface);
    return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateCounter(ID3D11Device2 *iface, const D3D11_COUNTER_DESC *desc,
        ID3D11Counter **counter)
{
    FIXME("iface %p, desc %p, counter %p stub!\n", iface, desc, counter);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateDeferredContext(ID3D11Device2 *iface, UINT flags,
        ID3D11DeviceContext **context)
{
    FIXME("iface %p, flags %#x, context %p stub!\n", iface, flags, context);

    *context = NULL;
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_OpenSharedResource(ID3D11Device2 *iface, HANDLE resource, REFIID iid,
        void **out)
{
    FIXME("iface %p, resource %p, iid %s, out %p stub!\n", iface, resource, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CheckFormatSupport(ID3D11Device2 *iface, DXGI_FORMAT format,
        UINT *format_support)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct wined3d_device_creation_parameters params;
    enum wined3d_format_id wined3d_format;
    D3D_FEATURE_LEVEL feature_level;
    struct wined3d *wined3d;
    unsigned int i;

    static const struct
    {
        enum wined3d_resource_type rtype;
        unsigned int bind_flags;
        D3D11_FORMAT_SUPPORT flag;
    }
    flag_mapping[] =
    {
        {WINED3D_RTYPE_TEXTURE_1D, WINED3D_BIND_SHADER_RESOURCE, D3D11_FORMAT_SUPPORT_TEXTURE1D},
        {WINED3D_RTYPE_TEXTURE_2D, WINED3D_BIND_SHADER_RESOURCE, D3D11_FORMAT_SUPPORT_TEXTURE2D},
        {WINED3D_RTYPE_TEXTURE_3D, WINED3D_BIND_SHADER_RESOURCE, D3D11_FORMAT_SUPPORT_TEXTURE3D},
        {WINED3D_RTYPE_NONE,       WINED3D_BIND_RENDER_TARGET,   D3D11_FORMAT_SUPPORT_RENDER_TARGET},
        {WINED3D_RTYPE_NONE,       WINED3D_BIND_DEPTH_STENCIL,   D3D11_FORMAT_SUPPORT_DEPTH_STENCIL},
    };
    HRESULT hr;

    FIXME("iface %p, format %u, format_support %p partial-stub!\n", iface, format, format_support);

    wined3d_format = wined3dformat_from_dxgi_format(format);
    if (format && !wined3d_format)
    {
        WARN("Invalid format %#x.\n", format);
        *format_support = 0;
        return E_FAIL;
    }

    *format_support = 0;

    wined3d_mutex_lock();
    feature_level = device->feature_level;
    wined3d = wined3d_device_get_wined3d(device->wined3d_device);
    wined3d_device_get_creation_parameters(device->wined3d_device, &params);
    for (i = 0; i < ARRAY_SIZE(flag_mapping); ++i)
    {
        hr = wined3d_check_device_format(wined3d, params.adapter_idx, params.device_type,
                WINED3DFMT_UNKNOWN, 0, flag_mapping[i].bind_flags, flag_mapping[i].rtype, wined3d_format);
        if (hr == WINED3DERR_NOTAVAILABLE || hr == WINED3DOK_NOMIPGEN)
            continue;
        if (hr != WINED3D_OK)
        {
            WARN("Failed to check device format support, hr %#x.\n", hr);
            wined3d_mutex_unlock();
            return E_FAIL;
        }

        *format_support |= flag_mapping[i].flag;
    }
    wined3d_mutex_unlock();

    if (*format_support & (D3D11_FORMAT_SUPPORT_TEXTURE1D
            | D3D11_FORMAT_SUPPORT_TEXTURE2D | D3D11_FORMAT_SUPPORT_TEXTURE3D))
    {
        *format_support |= D3D11_FORMAT_SUPPORT_SHADER_LOAD;
        *format_support |= D3D11_FORMAT_SUPPORT_SHADER_SAMPLE;

        if (feature_level >= D3D_FEATURE_LEVEL_10_1)
            *format_support |= D3D11_FORMAT_SUPPORT_SHADER_GATHER;

        if (*format_support & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL)
        {
            if (feature_level >= D3D_FEATURE_LEVEL_10_0)
                *format_support |= D3D11_FORMAT_SUPPORT_SHADER_SAMPLE_COMPARISON;

            if (feature_level >= D3D_FEATURE_LEVEL_10_1)
                *format_support |= D3D11_FORMAT_SUPPORT_SHADER_GATHER_COMPARISON;
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CheckMultisampleQualityLevels(ID3D11Device2 *iface,
        DXGI_FORMAT format, UINT sample_count, UINT *quality_level_count)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct wined3d_device_creation_parameters params;
    struct wined3d *wined3d;
    HRESULT hr;

    TRACE("iface %p, format %s, sample_count %u, quality_level_count %p.\n",
            iface, debug_dxgi_format(format), sample_count, quality_level_count);

    if (!quality_level_count)
        return E_INVALIDARG;

    *quality_level_count = 0;

    if (!sample_count)
        return E_FAIL;
    if (sample_count == 1)
    {
        *quality_level_count = 1;
        return S_OK;
    }
    if (sample_count > D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT)
        return E_FAIL;

    wined3d_mutex_lock();
    wined3d = wined3d_device_get_wined3d(device->wined3d_device);
    wined3d_device_get_creation_parameters(device->wined3d_device, &params);
    hr = wined3d_check_device_multisample_type(wined3d, params.adapter_idx, params.device_type,
            wined3dformat_from_dxgi_format(format), TRUE, sample_count, quality_level_count);
    wined3d_mutex_unlock();

    if (hr == WINED3DERR_INVALIDCALL)
        return E_INVALIDARG;
    if (hr == WINED3DERR_NOTAVAILABLE)
        return S_OK;
    return hr;
}

static void STDMETHODCALLTYPE d3d11_device_CheckCounterInfo(ID3D11Device2 *iface, D3D11_COUNTER_INFO *info)
{
    FIXME("iface %p, info %p stub!\n", iface, info);
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CheckCounter(ID3D11Device2 *iface, const D3D11_COUNTER_DESC *desc,
        D3D11_COUNTER_TYPE *type, UINT *active_counter_count, char *name, UINT *name_length,
        char *units, UINT *units_length, char *description, UINT *description_length)
{
    FIXME("iface %p, desc %p, type %p, active_counter_count %p, name %p, name_length %p, "
            "units %p, units_length %p, description %p, description_length %p stub!\n",
            iface, desc, type, active_counter_count, name, name_length,
            units, units_length, description, description_length);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CheckFeatureSupport(ID3D11Device2 *iface, D3D11_FEATURE feature,
        void *feature_support_data, UINT feature_support_data_size)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct wined3d_caps wined3d_caps;
    HRESULT hr;

    TRACE("iface %p, feature %u, feature_support_data %p, feature_support_data_size %u.\n",
            iface, feature, feature_support_data, feature_support_data_size);

    switch (feature)
    {
        case D3D11_FEATURE_THREADING:
        {
            D3D11_FEATURE_DATA_THREADING *threading_data = feature_support_data;
            if (feature_support_data_size != sizeof(*threading_data))
            {
                WARN("Invalid data size.\n");
                return E_INVALIDARG;
            }

            /* We lie about the threading support to make Tomb Raider 2013 and
             * Deus Ex: Human Revolution happy. */
            FIXME("Returning fake threading support data.\n");
            threading_data->DriverConcurrentCreates = TRUE;
            threading_data->DriverCommandLists = TRUE;
            return S_OK;
        }

        case D3D11_FEATURE_DOUBLES:
        {
            D3D11_FEATURE_DATA_DOUBLES *doubles_data = feature_support_data;
            if (feature_support_data_size != sizeof(*doubles_data))
            {
                WARN("Invalid data size.\n");
                return E_INVALIDARG;
            }

            wined3d_mutex_lock();
            hr = wined3d_device_get_device_caps(device->wined3d_device, &wined3d_caps);
            wined3d_mutex_unlock();
            if (FAILED(hr))
            {
                WARN("Failed to get device caps, hr %#x.\n", hr);
                return hr;
            }

            doubles_data->DoublePrecisionFloatShaderOps = wined3d_caps.shader_double_precision;
            return S_OK;
        }

        case D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS:
        {
            D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS *options = feature_support_data;
            if (feature_support_data_size != sizeof(*options))
            {
                WARN("Invalid data size.\n");
                return E_INVALIDARG;
            }

            options->ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x = FALSE;
            return S_OK;
        }

        case D3D11_FEATURE_D3D11_OPTIONS:
        {
            D3D11_FEATURE_DATA_D3D11_OPTIONS *options = feature_support_data;
            if (feature_support_data_size != sizeof(*options))
            {
                WARN("Invalid data size.\n");
                return E_INVALIDARG;
            }

            FIXME("Returning fake Options support data.\n");
            options->OutputMergerLogicOp = FALSE;
            options->UAVOnlyRenderingForcedSampleCount = FALSE;
            options->DiscardAPIsSeenByDriver = FALSE;
            options->FlagsForUpdateAndCopySeenByDriver = FALSE;
            options->ClearView = FALSE;
            options->CopyWithOverlap = FALSE;
            options->ConstantBufferPartialUpdate = FALSE;
            options->ConstantBufferOffsetting = FALSE;
            options->MapNoOverwriteOnDynamicConstantBuffer = FALSE;
            options->MapNoOverwriteOnDynamicBufferSRV = FALSE;
            options->MultisampleRTVWithForcedSampleCountOne = FALSE;
            options->SAD4ShaderInstructions = FALSE;
            options->ExtendedDoublesShaderInstructions = FALSE;
            options->ExtendedResourceSharing = FALSE;
            return S_OK;
        }

        case D3D11_FEATURE_D3D11_OPTIONS1:
        {
            D3D11_FEATURE_DATA_D3D11_OPTIONS1 *options = feature_support_data;
            if (feature_support_data_size != sizeof(*options))
            {
                WARN("Invalid data size.\n");
                return E_INVALIDARG;
            }

            FIXME("Returning fake Options1 support data.\n");
            options->TiledResourcesTier = D3D11_TILED_RESOURCES_NOT_SUPPORTED;
            options->MinMaxFiltering = FALSE;
            options->ClearViewAlsoSupportsDepthOnlyFormats = FALSE;
            options->MapOnDefaultBuffers = FALSE;
            return S_OK;
        }

        case D3D11_FEATURE_D3D11_OPTIONS3:
        {
            D3D11_FEATURE_DATA_D3D11_OPTIONS3 *options = feature_support_data;
            if (feature_support_data_size != sizeof(*options))
            {
                WARN("Invalid data size.\n");
                return E_INVALIDARG;
            }

            wined3d_mutex_lock();
            hr = wined3d_device_get_device_caps(device->wined3d_device, &wined3d_caps);
            wined3d_mutex_unlock();
            if (FAILED(hr))
            {
                WARN("Failed to get device caps, hr %#x.\n", hr);
                return hr;
            }

            options->VPAndRTArrayIndexFromAnyShaderFeedingRasterizer
                    = wined3d_caps.viewport_array_index_any_shader;
            return S_OK;
        }

        case D3D11_FEATURE_ARCHITECTURE_INFO:
        {
            D3D11_FEATURE_DATA_ARCHITECTURE_INFO *options = feature_support_data;
            if (feature_support_data_size != sizeof(*options))
            {
                WARN("Invalid data size.\n");
                return E_INVALIDARG;
            }

            FIXME("Returning fake data architecture info.\n");
            options->TileBasedDeferredRenderer = FALSE;
            return S_OK;
        }

        default:
            FIXME("Unhandled feature %#x.\n", feature);
            return E_NOTIMPL;
    }
}

static HRESULT STDMETHODCALLTYPE d3d11_device_GetPrivateData(ID3D11Device2 *iface, REFGUID guid,
        UINT *data_size, void *data)
{
    IDXGIDevice *dxgi_device;
    HRESULT hr;

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    if (FAILED(hr = ID3D11Device2_QueryInterface(iface, &IID_IDXGIDevice, (void **)&dxgi_device)))
        return hr;
    hr = IDXGIDevice_GetPrivateData(dxgi_device, guid, data_size, data);
    IDXGIDevice_Release(dxgi_device);

    return hr;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_SetPrivateData(ID3D11Device2 *iface, REFGUID guid,
        UINT data_size, const void *data)
{
    IDXGIDevice *dxgi_device;
    HRESULT hr;

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    if (FAILED(hr = ID3D11Device2_QueryInterface(iface, &IID_IDXGIDevice, (void **)&dxgi_device)))
        return hr;
    hr = IDXGIDevice_SetPrivateData(dxgi_device, guid, data_size, data);
    IDXGIDevice_Release(dxgi_device);

    return hr;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_SetPrivateDataInterface(ID3D11Device2 *iface, REFGUID guid,
        const IUnknown *data)
{
    IDXGIDevice *dxgi_device;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    if (FAILED(hr = ID3D11Device2_QueryInterface(iface, &IID_IDXGIDevice, (void **)&dxgi_device)))
        return hr;
    hr = IDXGIDevice_SetPrivateDataInterface(dxgi_device, guid, data);
    IDXGIDevice_Release(dxgi_device);

    return hr;
}

static D3D_FEATURE_LEVEL STDMETHODCALLTYPE d3d11_device_GetFeatureLevel(ID3D11Device2 *iface)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);

    TRACE("iface %p.\n", iface);

    return device->feature_level;
}

static UINT STDMETHODCALLTYPE d3d11_device_GetCreationFlags(ID3D11Device2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_GetDeviceRemovedReason(ID3D11Device2 *iface)
{
    WARN("iface %p stub!\n", iface);

    return S_OK;
}

static void STDMETHODCALLTYPE d3d11_device_GetImmediateContext(ID3D11Device2 *iface,
        ID3D11DeviceContext **immediate_context)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);

    TRACE("iface %p, immediate_context %p.\n", iface, immediate_context);

    *immediate_context = (ID3D11DeviceContext *)&device->immediate_context.ID3D11DeviceContext1_iface;
    ID3D11DeviceContext_AddRef(*immediate_context);
}

static HRESULT STDMETHODCALLTYPE d3d11_device_SetExceptionMode(ID3D11Device2 *iface, UINT flags)
{
    FIXME("iface %p, flags %#x stub!\n", iface, flags);

    return E_NOTIMPL;
}

static UINT STDMETHODCALLTYPE d3d11_device_GetExceptionMode(ID3D11Device2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static void STDMETHODCALLTYPE d3d11_device_GetImmediateContext1(ID3D11Device2 *iface,
        ID3D11DeviceContext1 **immediate_context)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);

    TRACE("iface %p, immediate_context %p.\n", iface, immediate_context);

    *immediate_context = &device->immediate_context.ID3D11DeviceContext1_iface;
    ID3D11DeviceContext1_AddRef(*immediate_context);
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateDeferredContext1(ID3D11Device2 *iface, UINT flags,
        ID3D11DeviceContext1 **context)
{
    FIXME("iface %p, flags %#x, context %p stub!\n", iface, flags, context);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateBlendState1(ID3D11Device2 *iface,
        const D3D11_BLEND_DESC1 *desc, ID3D11BlendState1 **state)
{
    FIXME("iface %p, desc %p, state %p stub!\n", iface, desc, state);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateRasterizerState1(ID3D11Device2 *iface,
        const D3D11_RASTERIZER_DESC1 *desc, ID3D11RasterizerState1 **state)
{
    FIXME("iface %p, desc %p, state %p stub!\n", iface, desc, state);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateDeviceContextState(ID3D11Device2 *iface, UINT flags,
        const D3D_FEATURE_LEVEL *feature_levels, UINT feature_levels_count, UINT sdk_version,
        REFIID emulated_interface, D3D_FEATURE_LEVEL *chosen_feature_level, ID3DDeviceContextState **state)
{
    FIXME("iface %p, flags %#x, feature_levels %p, feature_level_count %u, sdk_version %u, "
            "emulated_interface %s, chosen_feature_level %p, state %p stub!\n", iface, flags, feature_levels,
            feature_levels_count, sdk_version, debugstr_guid(emulated_interface), chosen_feature_level, state);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_OpenSharedResource1(ID3D11Device2 *iface, HANDLE handle,
        REFIID iid, void **resource)
{
    FIXME("iface %p, handle %p, iid %s, resource %p stub!\n", iface, handle, debugstr_guid(iid), resource);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_OpenSharedResourceByName(ID3D11Device2 *iface, const WCHAR *name,
        DWORD access, REFIID iid, void **resource)
{
    FIXME("iface %p, name %s, access %#x, iid %s, resource %p stub!\n", iface, debugstr_w(name), access,
            debugstr_guid(iid), resource);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d3d11_device_GetImmediateContext2(ID3D11Device2 *iface,
        ID3D11DeviceContext2 **context)
{
    FIXME("iface %p, context %p stub!\n", iface, context);
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateDeferredContext2(ID3D11Device2 *iface,
        UINT flags, ID3D11DeviceContext2 **context)
{
    FIXME("iface %p, flags %#x, context %p stub!\n", iface, flags, context);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d3d11_device_GetResourceTiling(ID3D11Device2 *iface,
        ID3D11Resource *resource, UINT *tile_count, D3D11_PACKED_MIP_DESC *mip_desc,
        D3D11_TILE_SHAPE *tile_shape, UINT *subresource_tiling_count, UINT first_subresource_tiling,
        D3D11_SUBRESOURCE_TILING *subresource_tiling)
{
    FIXME("iface %p, resource %p, tile_count %p, mip_desc %p, tile_shape %p, "
            "subresource_tiling_count %p, first_subresource_tiling %u, subresource_tiling %p stub!\n",
            iface, resource, tile_count, mip_desc, tile_shape,
            subresource_tiling_count, first_subresource_tiling, subresource_tiling);
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CheckMultisampleQualityLevels1(ID3D11Device2 *iface,
        DXGI_FORMAT format, UINT sample_count, UINT flags, UINT *quality_level_count)
{
    FIXME("iface %p, format %#x, sample_count %u, flags %#x, quality_level_count %p stub!\n",
            iface, format, sample_count, flags, quality_level_count);

    return E_NOTIMPL;
}

static const struct ID3D11Device2Vtbl d3d11_device_vtbl =
{
    /* IUnknown methods */
    d3d11_device_QueryInterface,
    d3d11_device_AddRef,
    d3d11_device_Release,
    /* ID3D11Device methods */
    d3d11_device_CreateBuffer,
    d3d11_device_CreateTexture1D,
    d3d11_device_CreateTexture2D,
    d3d11_device_CreateTexture3D,
    d3d11_device_CreateShaderResourceView,
    d3d11_device_CreateUnorderedAccessView,
    d3d11_device_CreateRenderTargetView,
    d3d11_device_CreateDepthStencilView,
    d3d11_device_CreateInputLayout,
    d3d11_device_CreateVertexShader,
    d3d11_device_CreateGeometryShader,
    d3d11_device_CreateGeometryShaderWithStreamOutput,
    d3d11_device_CreatePixelShader,
    d3d11_device_CreateHullShader,
    d3d11_device_CreateDomainShader,
    d3d11_device_CreateComputeShader,
    d3d11_device_CreateClassLinkage,
    d3d11_device_CreateBlendState,
    d3d11_device_CreateDepthStencilState,
    d3d11_device_CreateRasterizerState,
    d3d11_device_CreateSamplerState,
    d3d11_device_CreateQuery,
    d3d11_device_CreatePredicate,
    d3d11_device_CreateCounter,
    d3d11_device_CreateDeferredContext,
    d3d11_device_OpenSharedResource,
    d3d11_device_CheckFormatSupport,
    d3d11_device_CheckMultisampleQualityLevels,
    d3d11_device_CheckCounterInfo,
    d3d11_device_CheckCounter,
    d3d11_device_CheckFeatureSupport,
    d3d11_device_GetPrivateData,
    d3d11_device_SetPrivateData,
    d3d11_device_SetPrivateDataInterface,
    d3d11_device_GetFeatureLevel,
    d3d11_device_GetCreationFlags,
    d3d11_device_GetDeviceRemovedReason,
    d3d11_device_GetImmediateContext,
    d3d11_device_SetExceptionMode,
    d3d11_device_GetExceptionMode,
    /* ID3D11Device1 methods */
    d3d11_device_GetImmediateContext1,
    d3d11_device_CreateDeferredContext1,
    d3d11_device_CreateBlendState1,
    d3d11_device_CreateRasterizerState1,
    d3d11_device_CreateDeviceContextState,
    d3d11_device_OpenSharedResource1,
    d3d11_device_OpenSharedResourceByName,
    /* ID3D11Device2 methods */
    d3d11_device_GetImmediateContext2,
    d3d11_device_CreateDeferredContext2,
    d3d11_device_GetResourceTiling,
    d3d11_device_CheckMultisampleQualityLevels1,
};

/* Inner IUnknown methods */

static inline struct d3d_device *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_device, IUnknown_inner);
}

static HRESULT STDMETHODCALLTYPE d3d_device_inner_QueryInterface(IUnknown *iface, REFIID riid, void **out)
{
    struct d3d_device *device = impl_from_IUnknown(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_ID3D11Device2)
            || IsEqualGUID(riid, &IID_ID3D11Device1)
            || IsEqualGUID(riid, &IID_ID3D11Device)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        *out = &device->ID3D11Device2_iface;
    }
    else if (IsEqualGUID(riid, &IID_ID3D10Device1)
            || IsEqualGUID(riid, &IID_ID3D10Device))
    {
        *out = &device->ID3D10Device1_iface;
    }
    else if (IsEqualGUID(riid, &IID_ID3D10Multithread))
    {
        *out = &device->ID3D10Multithread_iface;
    }
    else if (IsEqualGUID(riid, &IID_IWineDXGIDeviceParent))
    {
        *out = &device->IWineDXGIDeviceParent_iface;
    }
    else
    {
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG STDMETHODCALLTYPE d3d_device_inner_AddRef(IUnknown *iface)
{
    struct d3d_device *device = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&device->refcount);

    TRACE("%p increasing refcount to %u.\n", device, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d_device_inner_Release(IUnknown *iface)
{
    struct d3d_device *device = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&device->refcount);

    TRACE("%p decreasing refcount to %u.\n", device, refcount);

    if (!refcount)
    {
        d3d11_immediate_context_destroy(&device->immediate_context);
        if (device->wined3d_device)
        {
            wined3d_mutex_lock();
            wined3d_device_decref(device->wined3d_device);
            wined3d_mutex_unlock();
        }
        wine_rb_destroy(&device->sampler_states, NULL, NULL);
        wine_rb_destroy(&device->rasterizer_states, NULL, NULL);
        wine_rb_destroy(&device->depthstencil_states, NULL, NULL);
        wine_rb_destroy(&device->blend_states, NULL, NULL);
    }

    return refcount;
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_device_QueryInterface(ID3D10Device1 *iface, REFIID iid,
        void **out)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    return IUnknown_QueryInterface(device->outer_unk, iid, out);
}

static ULONG STDMETHODCALLTYPE d3d10_device_AddRef(ID3D10Device1 *iface)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    return IUnknown_AddRef(device->outer_unk);
}

static ULONG STDMETHODCALLTYPE d3d10_device_Release(ID3D10Device1 *iface)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    return IUnknown_Release(device->outer_unk);
}

/* ID3D10Device methods */

static void d3d10_device_get_constant_buffers(ID3D10Device1 *iface,
        enum wined3d_shader_type type, UINT start_slot, UINT buffer_count, ID3D10Buffer **buffers)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    unsigned int i;

    wined3d_mutex_lock();
    for (i = 0; i < buffer_count; ++i)
    {
        struct wined3d_buffer *wined3d_buffer;
        struct d3d_buffer *buffer_impl;

        if (!(wined3d_buffer = wined3d_device_get_constant_buffer(device->wined3d_device,
                type, start_slot + i)))
        {
            buffers[i] = NULL;
            continue;
        }

        buffer_impl = wined3d_buffer_get_parent(wined3d_buffer);
        buffers[i] = &buffer_impl->ID3D10Buffer_iface;
        ID3D10Buffer_AddRef(buffers[i]);
    }
    wined3d_mutex_unlock();
}

static void d3d10_device_set_constant_buffers(ID3D10Device1 *iface,
        enum wined3d_shader_type type, UINT start_slot, UINT buffer_count, ID3D10Buffer *const *buffers)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    unsigned int i;

    wined3d_mutex_lock();
    for (i = 0; i < buffer_count; ++i)
    {
        struct d3d_buffer *buffer = unsafe_impl_from_ID3D10Buffer(buffers[i]);

        wined3d_device_set_constant_buffer(device->wined3d_device, type, start_slot + i,
                buffer ? buffer->wined3d_buffer : NULL);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_VSSetConstantBuffers(ID3D10Device1 *iface,
        UINT start_slot, UINT buffer_count, ID3D10Buffer *const *buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d10_device_set_constant_buffers(iface, WINED3D_SHADER_TYPE_VERTEX, start_slot,
            buffer_count, buffers);
}

static void STDMETHODCALLTYPE d3d10_device_PSSetShaderResources(ID3D10Device1 *iface,
        UINT start_slot, UINT view_count, ID3D10ShaderResourceView *const *views)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n",
            iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct d3d_shader_resource_view *view = unsafe_impl_from_ID3D10ShaderResourceView(views[i]);

        wined3d_device_set_ps_resource_view(device->wined3d_device, start_slot + i,
                view ? view->wined3d_view : NULL);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_PSSetShader(ID3D10Device1 *iface,
        ID3D10PixelShader *shader)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_pixel_shader *ps = unsafe_impl_from_ID3D10PixelShader(shader);

    TRACE("iface %p, shader %p\n", iface, shader);

    wined3d_mutex_lock();
    wined3d_device_set_pixel_shader(device->wined3d_device, ps ? ps->wined3d_shader : NULL);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_PSSetSamplers(ID3D10Device1 *iface,
        UINT start_slot, UINT sampler_count, ID3D10SamplerState *const *samplers)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    wined3d_mutex_lock();
    for (i = 0; i < sampler_count; ++i)
    {
        struct d3d_sampler_state *sampler = unsafe_impl_from_ID3D10SamplerState(samplers[i]);

        wined3d_device_set_ps_sampler(device->wined3d_device, start_slot + i,
                sampler ? sampler->wined3d_sampler : NULL);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_VSSetShader(ID3D10Device1 *iface,
        ID3D10VertexShader *shader)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_vertex_shader *vs = unsafe_impl_from_ID3D10VertexShader(shader);

    TRACE("iface %p, shader %p\n", iface, shader);

    wined3d_mutex_lock();
    wined3d_device_set_vertex_shader(device->wined3d_device, vs ? vs->wined3d_shader : NULL);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_DrawIndexed(ID3D10Device1 *iface, UINT index_count,
        UINT start_index_location, INT base_vertex_location)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);

    TRACE("iface %p, index_count %u, start_index_location %u, base_vertex_location %d.\n",
            iface, index_count, start_index_location, base_vertex_location);

    wined3d_mutex_lock();
    wined3d_device_set_base_vertex_index(device->wined3d_device, base_vertex_location);
    wined3d_device_draw_indexed_primitive(device->wined3d_device, start_index_location, index_count);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_Draw(ID3D10Device1 *iface, UINT vertex_count,
        UINT start_vertex_location)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);

    TRACE("iface %p, vertex_count %u, start_vertex_location %u\n",
            iface, vertex_count, start_vertex_location);

    wined3d_mutex_lock();
    wined3d_device_draw_primitive(device->wined3d_device, start_vertex_location, vertex_count);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_PSSetConstantBuffers(ID3D10Device1 *iface,
        UINT start_slot, UINT buffer_count, ID3D10Buffer *const *buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d10_device_set_constant_buffers(iface, WINED3D_SHADER_TYPE_PIXEL, start_slot,
            buffer_count, buffers);
}

static void STDMETHODCALLTYPE d3d10_device_IASetInputLayout(ID3D10Device1 *iface,
        ID3D10InputLayout *input_layout)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_input_layout *layout = unsafe_impl_from_ID3D10InputLayout(input_layout);

    TRACE("iface %p, input_layout %p\n", iface, input_layout);

    wined3d_mutex_lock();
    wined3d_device_set_vertex_declaration(device->wined3d_device,
            layout ? layout->wined3d_decl : NULL);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_IASetVertexBuffers(ID3D10Device1 *iface, UINT start_slot,
        UINT buffer_count, ID3D10Buffer *const *buffers, const UINT *strides, const UINT *offsets)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p, strides %p, offsets %p\n",
            iface, start_slot, buffer_count, buffers, strides, offsets);

    wined3d_mutex_lock();
    for (i = 0; i < buffer_count; ++i)
    {
        struct d3d_buffer *buffer = unsafe_impl_from_ID3D10Buffer(buffers[i]);

        wined3d_device_set_stream_source(device->wined3d_device, start_slot + i,
                buffer ? buffer->wined3d_buffer : NULL, offsets[i], strides[i]);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_IASetIndexBuffer(ID3D10Device1 *iface,
        ID3D10Buffer *buffer, DXGI_FORMAT format, UINT offset)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_buffer *buffer_impl = unsafe_impl_from_ID3D10Buffer(buffer);

    TRACE("iface %p, buffer %p, format %s, offset %u.\n",
            iface, buffer, debug_dxgi_format(format), offset);

    wined3d_mutex_lock();
    wined3d_device_set_index_buffer(device->wined3d_device,
            buffer_impl ? buffer_impl->wined3d_buffer : NULL,
            wined3dformat_from_dxgi_format(format), offset);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_DrawIndexedInstanced(ID3D10Device1 *iface,
        UINT instance_index_count, UINT instance_count, UINT start_index_location,
        INT base_vertex_location, UINT start_instance_location)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);

    TRACE("iface %p, instance_index_count %u, instance_count %u, start_index_location %u, "
            "base_vertex_location %d, start_instance_location %u.\n",
            iface, instance_index_count, instance_count, start_index_location,
            base_vertex_location, start_instance_location);

    wined3d_mutex_lock();
    wined3d_device_set_base_vertex_index(device->wined3d_device, base_vertex_location);
    wined3d_device_draw_indexed_primitive_instanced(device->wined3d_device, start_index_location,
            instance_index_count, start_instance_location, instance_count);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_DrawInstanced(ID3D10Device1 *iface,
        UINT instance_vertex_count, UINT instance_count,
        UINT start_vertex_location, UINT start_instance_location)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);

    TRACE("iface %p, instance_vertex_count %u, instance_count %u, start_vertex_location %u, "
            "start_instance_location %u.\n", iface, instance_vertex_count, instance_count,
            start_vertex_location, start_instance_location);

    wined3d_mutex_lock();
    wined3d_device_draw_primitive_instanced(device->wined3d_device, start_vertex_location,
            instance_vertex_count, start_instance_location, instance_count);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_GSSetConstantBuffers(ID3D10Device1 *iface,
        UINT start_slot, UINT buffer_count, ID3D10Buffer *const *buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d10_device_set_constant_buffers(iface, WINED3D_SHADER_TYPE_GEOMETRY, start_slot,
            buffer_count, buffers);
}

static void STDMETHODCALLTYPE d3d10_device_GSSetShader(ID3D10Device1 *iface, ID3D10GeometryShader *shader)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_geometry_shader *gs = unsafe_impl_from_ID3D10GeometryShader(shader);

    TRACE("iface %p, shader %p.\n", iface, shader);

    wined3d_mutex_lock();
    wined3d_device_set_geometry_shader(device->wined3d_device, gs ? gs->wined3d_shader : NULL);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_IASetPrimitiveTopology(ID3D10Device1 *iface,
        D3D10_PRIMITIVE_TOPOLOGY topology)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);

    TRACE("iface %p, topology %s.\n", iface, debug_d3d10_primitive_topology(topology));

    wined3d_mutex_lock();
    wined3d_device_set_primitive_type(device->wined3d_device, (enum wined3d_primitive_type)topology, 0);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_VSSetShaderResources(ID3D10Device1 *iface,
        UINT start_slot, UINT view_count, ID3D10ShaderResourceView *const *views)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n",
            iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct d3d_shader_resource_view *view = unsafe_impl_from_ID3D10ShaderResourceView(views[i]);

        wined3d_device_set_vs_resource_view(device->wined3d_device, start_slot + i,
                view ? view->wined3d_view : NULL);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_VSSetSamplers(ID3D10Device1 *iface,
        UINT start_slot, UINT sampler_count, ID3D10SamplerState *const *samplers)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    wined3d_mutex_lock();
    for (i = 0; i < sampler_count; ++i)
    {
        struct d3d_sampler_state *sampler = unsafe_impl_from_ID3D10SamplerState(samplers[i]);

        wined3d_device_set_vs_sampler(device->wined3d_device, start_slot + i,
                sampler ? sampler->wined3d_sampler : NULL);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_SetPredication(ID3D10Device1 *iface, ID3D10Predicate *predicate, BOOL value)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_query *query;

    TRACE("iface %p, predicate %p, value %#x.\n", iface, predicate, value);

    query = unsafe_impl_from_ID3D10Query((ID3D10Query *)predicate);
    wined3d_mutex_lock();
    wined3d_device_set_predication(device->wined3d_device, query ? query->wined3d_query : NULL, value);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_GSSetShaderResources(ID3D10Device1 *iface,
        UINT start_slot, UINT view_count, ID3D10ShaderResourceView *const *views)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n",
            iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct d3d_shader_resource_view *view = unsafe_impl_from_ID3D10ShaderResourceView(views[i]);

        wined3d_device_set_gs_resource_view(device->wined3d_device, start_slot + i,
                view ? view->wined3d_view : NULL);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_GSSetSamplers(ID3D10Device1 *iface,
        UINT start_slot, UINT sampler_count, ID3D10SamplerState *const *samplers)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    wined3d_mutex_lock();
    for (i = 0; i < sampler_count; ++i)
    {
        struct d3d_sampler_state *sampler = unsafe_impl_from_ID3D10SamplerState(samplers[i]);

        wined3d_device_set_gs_sampler(device->wined3d_device, start_slot + i,
                sampler ? sampler->wined3d_sampler : NULL);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_OMSetRenderTargets(ID3D10Device1 *iface,
        UINT render_target_view_count, ID3D10RenderTargetView *const *render_target_views,
        ID3D10DepthStencilView *depth_stencil_view)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_depthstencil_view *dsv;
    unsigned int i;

    TRACE("iface %p, render_target_view_count %u, render_target_views %p, depth_stencil_view %p.\n",
            iface, render_target_view_count, render_target_views, depth_stencil_view);

    wined3d_mutex_lock();
    for (i = 0; i < render_target_view_count; ++i)
    {
        struct d3d_rendertarget_view *rtv = unsafe_impl_from_ID3D10RenderTargetView(render_target_views[i]);

        wined3d_device_set_rendertarget_view(device->wined3d_device, i,
                rtv ? rtv->wined3d_view : NULL, FALSE);
    }
    for (; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        wined3d_device_set_rendertarget_view(device->wined3d_device, i, NULL, FALSE);
    }

    dsv = unsafe_impl_from_ID3D10DepthStencilView(depth_stencil_view);
    wined3d_device_set_depth_stencil_view(device->wined3d_device,
            dsv ? dsv->wined3d_view : NULL);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_OMSetBlendState(ID3D10Device1 *iface,
        ID3D10BlendState *blend_state, const float blend_factor[4], UINT sample_mask)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_blend_state *blend_state_object;

    TRACE("iface %p, blend_state %p, blend_factor %s, sample_mask 0x%08x.\n",
            iface, blend_state, debug_float4(blend_factor), sample_mask);

    blend_state_object = unsafe_impl_from_ID3D10BlendState(blend_state);
    d3d11_immediate_context_OMSetBlendState(&device->immediate_context.ID3D11DeviceContext1_iface,
            blend_state_object ? &blend_state_object->ID3D11BlendState_iface : NULL, blend_factor, sample_mask);
}

static void STDMETHODCALLTYPE d3d10_device_OMSetDepthStencilState(ID3D10Device1 *iface,
        ID3D10DepthStencilState *depth_stencil_state, UINT stencil_ref)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_depthstencil_state *ds_state_object;

    TRACE("iface %p, depth_stencil_state %p, stencil_ref %u.\n",
            iface, depth_stencil_state, stencil_ref);

    ds_state_object = unsafe_impl_from_ID3D10DepthStencilState(depth_stencil_state);
    d3d11_immediate_context_OMSetDepthStencilState(&device->immediate_context.ID3D11DeviceContext1_iface,
            ds_state_object ? &ds_state_object->ID3D11DepthStencilState_iface : NULL, stencil_ref);
}

static void STDMETHODCALLTYPE d3d10_device_SOSetTargets(ID3D10Device1 *iface,
        UINT target_count, ID3D10Buffer *const *targets, const UINT *offsets)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    unsigned int count, i;

    TRACE("iface %p, target_count %u, targets %p, offsets %p.\n", iface, target_count, targets, offsets);

    count = min(target_count, D3D10_SO_BUFFER_SLOT_COUNT);
    wined3d_mutex_lock();
    for (i = 0; i < count; ++i)
    {
        struct d3d_buffer *buffer = unsafe_impl_from_ID3D10Buffer(targets[i]);

        wined3d_device_set_stream_output(device->wined3d_device, i,
                buffer ? buffer->wined3d_buffer : NULL, offsets[i]);
    }

    for (i = count; i < D3D10_SO_BUFFER_SLOT_COUNT; ++i)
    {
        wined3d_device_set_stream_output(device->wined3d_device, i, NULL, 0);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_DrawAuto(ID3D10Device1 *iface)
{
    FIXME("iface %p stub!\n", iface);
}

static void STDMETHODCALLTYPE d3d10_device_RSSetState(ID3D10Device1 *iface, ID3D10RasterizerState *rasterizer_state)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_rasterizer_state *rasterizer_state_object;

    TRACE("iface %p, rasterizer_state %p.\n", iface, rasterizer_state);

    rasterizer_state_object = unsafe_impl_from_ID3D10RasterizerState(rasterizer_state);
    d3d11_immediate_context_RSSetState(&device->immediate_context.ID3D11DeviceContext1_iface,
            rasterizer_state_object ? &rasterizer_state_object->ID3D11RasterizerState_iface : NULL);
}

static void STDMETHODCALLTYPE d3d10_device_RSSetViewports(ID3D10Device1 *iface,
        UINT viewport_count, const D3D10_VIEWPORT *viewports)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct wined3d_viewport wined3d_vp[WINED3D_MAX_VIEWPORTS];
    unsigned int i;

    TRACE("iface %p, viewport_count %u, viewports %p.\n", iface, viewport_count, viewports);

    if (viewport_count > ARRAY_SIZE(wined3d_vp))
        return;

    for (i = 0; i < viewport_count; ++i)
    {
        wined3d_vp[i].x = viewports[i].TopLeftX;
        wined3d_vp[i].y = viewports[i].TopLeftY;
        wined3d_vp[i].width = viewports[i].Width;
        wined3d_vp[i].height = viewports[i].Height;
        wined3d_vp[i].min_z = viewports[i].MinDepth;
        wined3d_vp[i].max_z = viewports[i].MaxDepth;
    }

    wined3d_mutex_lock();
    wined3d_device_set_viewports(device->wined3d_device, viewport_count, wined3d_vp);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_RSSetScissorRects(ID3D10Device1 *iface,
        UINT rect_count, const D3D10_RECT *rects)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);

    TRACE("iface %p, rect_count %u, rects %p.\n", iface, rect_count, rects);

    if (rect_count > WINED3D_MAX_VIEWPORTS)
        return;

    wined3d_mutex_lock();
    wined3d_device_set_scissor_rects(device->wined3d_device, rect_count, rects);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_CopySubresourceRegion(ID3D10Device1 *iface,
        ID3D10Resource *dst_resource, UINT dst_subresource_idx, UINT dst_x, UINT dst_y, UINT dst_z,
        ID3D10Resource *src_resource, UINT src_subresource_idx, const D3D10_BOX *src_box)
{
    struct wined3d_resource *wined3d_dst_resource, *wined3d_src_resource;
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct wined3d_box wined3d_src_box;

    TRACE("iface %p, dst_resource %p, dst_subresource_idx %u, dst_x %u, dst_y %u, dst_z %u, "
            "src_resource %p, src_subresource_idx %u, src_box %p.\n",
            iface, dst_resource, dst_subresource_idx, dst_x, dst_y, dst_z,
            src_resource, src_subresource_idx, src_box);

    if (src_box)
        wined3d_box_set(&wined3d_src_box, src_box->left, src_box->top,
                src_box->right, src_box->bottom, src_box->front, src_box->back);

    wined3d_dst_resource = wined3d_resource_from_d3d10_resource(dst_resource);
    wined3d_src_resource = wined3d_resource_from_d3d10_resource(src_resource);
    wined3d_mutex_lock();
    wined3d_device_copy_sub_resource_region(device->wined3d_device, wined3d_dst_resource, dst_subresource_idx,
            dst_x, dst_y, dst_z, wined3d_src_resource, src_subresource_idx, src_box ? &wined3d_src_box : NULL, 0);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_CopyResource(ID3D10Device1 *iface,
        ID3D10Resource *dst_resource, ID3D10Resource *src_resource)
{
    struct wined3d_resource *wined3d_dst_resource, *wined3d_src_resource;
    struct d3d_device *device = impl_from_ID3D10Device(iface);

    TRACE("iface %p, dst_resource %p, src_resource %p.\n", iface, dst_resource, src_resource);

    wined3d_dst_resource = wined3d_resource_from_d3d10_resource(dst_resource);
    wined3d_src_resource = wined3d_resource_from_d3d10_resource(src_resource);
    wined3d_mutex_lock();
    wined3d_device_copy_resource(device->wined3d_device, wined3d_dst_resource, wined3d_src_resource);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_UpdateSubresource(ID3D10Device1 *iface,
        ID3D10Resource *resource, UINT subresource_idx, const D3D10_BOX *box,
        const void *data, UINT row_pitch, UINT depth_pitch)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    ID3D11Resource *d3d11_resource;

    TRACE("iface %p, resource %p, subresource_idx %u, box %p, data %p, row_pitch %u, depth_pitch %u.\n",
            iface, resource, subresource_idx, box, data, row_pitch, depth_pitch);

    ID3D10Resource_QueryInterface(resource, &IID_ID3D11Resource, (void **)&d3d11_resource);
    d3d11_immediate_context_UpdateSubresource(&device->immediate_context.ID3D11DeviceContext1_iface,
            d3d11_resource, subresource_idx, (const D3D11_BOX *)box, data, row_pitch, depth_pitch);
    ID3D11Resource_Release(d3d11_resource);
}

static void STDMETHODCALLTYPE d3d10_device_ClearRenderTargetView(ID3D10Device1 *iface,
        ID3D10RenderTargetView *render_target_view, const float color_rgba[4])
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_rendertarget_view *view = unsafe_impl_from_ID3D10RenderTargetView(render_target_view);
    const struct wined3d_color color = {color_rgba[0], color_rgba[1], color_rgba[2], color_rgba[3]};
    HRESULT hr;

    TRACE("iface %p, render_target_view %p, color_rgba %s.\n",
            iface, render_target_view, debug_float4(color_rgba));

    if (!view)
        return;

    wined3d_mutex_lock();
    if (FAILED(hr = wined3d_device_clear_rendertarget_view(device->wined3d_device, view->wined3d_view, NULL,
            WINED3DCLEAR_TARGET, &color, 0.0f, 0)))
        ERR("Failed to clear view, hr %#x.\n", hr);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_ClearDepthStencilView(ID3D10Device1 *iface,
        ID3D10DepthStencilView *depth_stencil_view, UINT flags, FLOAT depth, UINT8 stencil)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_depthstencil_view *view = unsafe_impl_from_ID3D10DepthStencilView(depth_stencil_view);
    DWORD wined3d_flags;
    HRESULT hr;

    TRACE("iface %p, depth_stencil_view %p, flags %#x, depth %.8e, stencil %u.\n",
            iface, depth_stencil_view, flags, depth, stencil);

    if (!view)
        return;

    wined3d_flags = wined3d_clear_flags_from_d3d11_clear_flags(flags);

    wined3d_mutex_lock();
    if (FAILED(hr = wined3d_device_clear_rendertarget_view(device->wined3d_device, view->wined3d_view, NULL,
            wined3d_flags, NULL, depth, stencil)))
        ERR("Failed to clear view, hr %#x.\n", hr);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_GenerateMips(ID3D10Device1 *iface,
        ID3D10ShaderResourceView *view)
{
    struct d3d_shader_resource_view *srv = unsafe_impl_from_ID3D10ShaderResourceView(view);

    TRACE("iface %p, view %p.\n", iface, view);

    wined3d_mutex_lock();
    wined3d_shader_resource_view_generate_mipmaps(srv->wined3d_view);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_ResolveSubresource(ID3D10Device1 *iface,
        ID3D10Resource *dst_resource, UINT dst_subresource_idx,
        ID3D10Resource *src_resource, UINT src_subresource_idx, DXGI_FORMAT format)
{
    struct wined3d_resource *wined3d_dst_resource, *wined3d_src_resource;
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    enum wined3d_format_id wined3d_format;

    TRACE("iface %p, dst_resource %p, dst_subresource_idx %u, "
            "src_resource %p, src_subresource_idx %u, format %s.\n",
            iface, dst_resource, dst_subresource_idx,
            src_resource, src_subresource_idx, debug_dxgi_format(format));

    wined3d_dst_resource = wined3d_resource_from_d3d10_resource(dst_resource);
    wined3d_src_resource = wined3d_resource_from_d3d10_resource(src_resource);
    wined3d_format = wined3dformat_from_dxgi_format(format);
    wined3d_mutex_lock();
    wined3d_device_resolve_sub_resource(device->wined3d_device,
            wined3d_dst_resource, dst_subresource_idx,
            wined3d_src_resource, src_subresource_idx, wined3d_format);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_VSGetConstantBuffers(ID3D10Device1 *iface,
        UINT start_slot, UINT buffer_count, ID3D10Buffer **buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d10_device_get_constant_buffers(iface, WINED3D_SHADER_TYPE_VERTEX, start_slot, buffer_count,
            buffers);
}

static void STDMETHODCALLTYPE d3d10_device_PSGetShaderResources(ID3D10Device1 *iface,
        UINT start_slot, UINT view_count, ID3D10ShaderResourceView **views)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n",
            iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct wined3d_shader_resource_view *wined3d_view;
        struct d3d_shader_resource_view *view_impl;

        if (!(wined3d_view = wined3d_device_get_ps_resource_view(device->wined3d_device, start_slot + i)))
        {
            views[i] = NULL;
            continue;
        }

        view_impl = wined3d_shader_resource_view_get_parent(wined3d_view);
        views[i] = (ID3D10ShaderResourceView *)&view_impl->ID3D10ShaderResourceView1_iface;
        ID3D10ShaderResourceView_AddRef(views[i]);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_PSGetShader(ID3D10Device1 *iface, ID3D10PixelShader **shader)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_pixel_shader *shader_impl;
    struct wined3d_shader *wined3d_shader;

    TRACE("iface %p, shader %p.\n", iface, shader);

    wined3d_mutex_lock();
    if (!(wined3d_shader = wined3d_device_get_pixel_shader(device->wined3d_device)))
    {
        wined3d_mutex_unlock();
        *shader = NULL;
        return;
    }

    shader_impl = wined3d_shader_get_parent(wined3d_shader);
    wined3d_mutex_unlock();
    *shader = &shader_impl->ID3D10PixelShader_iface;
    ID3D10PixelShader_AddRef(*shader);
}

static void STDMETHODCALLTYPE d3d10_device_PSGetSamplers(ID3D10Device1 *iface,
        UINT start_slot, UINT sampler_count, ID3D10SamplerState **samplers)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    wined3d_mutex_lock();
    for (i = 0; i < sampler_count; ++i)
    {
        struct d3d_sampler_state *sampler_impl;
        struct wined3d_sampler *wined3d_sampler;

        if (!(wined3d_sampler = wined3d_device_get_ps_sampler(device->wined3d_device, start_slot + i)))
        {
            samplers[i] = NULL;
            continue;
        }

        sampler_impl = wined3d_sampler_get_parent(wined3d_sampler);
        samplers[i] = &sampler_impl->ID3D10SamplerState_iface;
        ID3D10SamplerState_AddRef(samplers[i]);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_VSGetShader(ID3D10Device1 *iface, ID3D10VertexShader **shader)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_vertex_shader *shader_impl;
    struct wined3d_shader *wined3d_shader;

    TRACE("iface %p, shader %p.\n", iface, shader);

    wined3d_mutex_lock();
    if (!(wined3d_shader = wined3d_device_get_vertex_shader(device->wined3d_device)))
    {
        wined3d_mutex_unlock();
        *shader = NULL;
        return;
    }

    shader_impl = wined3d_shader_get_parent(wined3d_shader);
    wined3d_mutex_unlock();
    *shader = &shader_impl->ID3D10VertexShader_iface;
    ID3D10VertexShader_AddRef(*shader);
}

static void STDMETHODCALLTYPE d3d10_device_PSGetConstantBuffers(ID3D10Device1 *iface,
        UINT start_slot, UINT buffer_count, ID3D10Buffer **buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d10_device_get_constant_buffers(iface, WINED3D_SHADER_TYPE_PIXEL, start_slot, buffer_count,
            buffers);
}

static void STDMETHODCALLTYPE d3d10_device_IAGetInputLayout(ID3D10Device1 *iface, ID3D10InputLayout **input_layout)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct wined3d_vertex_declaration *wined3d_declaration;
    struct d3d_input_layout *input_layout_impl;

    TRACE("iface %p, input_layout %p.\n", iface, input_layout);

    wined3d_mutex_lock();
    if (!(wined3d_declaration = wined3d_device_get_vertex_declaration(device->wined3d_device)))
    {
        wined3d_mutex_unlock();
        *input_layout = NULL;
        return;
    }

    input_layout_impl = wined3d_vertex_declaration_get_parent(wined3d_declaration);
    wined3d_mutex_unlock();
    *input_layout = &input_layout_impl->ID3D10InputLayout_iface;
    ID3D10InputLayout_AddRef(*input_layout);
}

static void STDMETHODCALLTYPE d3d10_device_IAGetVertexBuffers(ID3D10Device1 *iface,
        UINT start_slot, UINT buffer_count, ID3D10Buffer **buffers, UINT *strides, UINT *offsets)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p, strides %p, offsets %p.\n",
            iface, start_slot, buffer_count, buffers, strides, offsets);

    wined3d_mutex_lock();
    for (i = 0; i < buffer_count; ++i)
    {
        struct wined3d_buffer *wined3d_buffer = NULL;
        struct d3d_buffer *buffer_impl;

        if (FAILED(wined3d_device_get_stream_source(device->wined3d_device, start_slot + i,
                &wined3d_buffer, &offsets[i], &strides[i])))
            ERR("Failed to get vertex buffer.\n");

        if (!wined3d_buffer)
        {
            buffers[i] = NULL;
            continue;
        }

        buffer_impl = wined3d_buffer_get_parent(wined3d_buffer);
        buffers[i] = &buffer_impl->ID3D10Buffer_iface;
        ID3D10Buffer_AddRef(buffers[i]);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_IAGetIndexBuffer(ID3D10Device1 *iface,
        ID3D10Buffer **buffer, DXGI_FORMAT *format, UINT *offset)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    enum wined3d_format_id wined3d_format;
    struct wined3d_buffer *wined3d_buffer;
    struct d3d_buffer *buffer_impl;

    TRACE("iface %p, buffer %p, format %p, offset %p.\n", iface, buffer, format, offset);

    wined3d_mutex_lock();
    wined3d_buffer = wined3d_device_get_index_buffer(device->wined3d_device, &wined3d_format, offset);
    *format = dxgi_format_from_wined3dformat(wined3d_format);
    if (!wined3d_buffer)
    {
        wined3d_mutex_unlock();
        *buffer = NULL;
        return;
    }

    buffer_impl = wined3d_buffer_get_parent(wined3d_buffer);
    wined3d_mutex_unlock();
    *buffer = &buffer_impl->ID3D10Buffer_iface;
    ID3D10Buffer_AddRef(*buffer);
}

static void STDMETHODCALLTYPE d3d10_device_GSGetConstantBuffers(ID3D10Device1 *iface,
        UINT start_slot, UINT buffer_count, ID3D10Buffer **buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d10_device_get_constant_buffers(iface, WINED3D_SHADER_TYPE_GEOMETRY, start_slot, buffer_count,
            buffers);
}

static void STDMETHODCALLTYPE d3d10_device_GSGetShader(ID3D10Device1 *iface, ID3D10GeometryShader **shader)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_geometry_shader *shader_impl;
    struct wined3d_shader *wined3d_shader;

    TRACE("iface %p, shader %p.\n", iface, shader);

    wined3d_mutex_lock();
    if (!(wined3d_shader = wined3d_device_get_geometry_shader(device->wined3d_device)))
    {
        wined3d_mutex_unlock();
        *shader = NULL;
        return;
    }

    shader_impl = wined3d_shader_get_parent(wined3d_shader);
    wined3d_mutex_unlock();
    *shader = &shader_impl->ID3D10GeometryShader_iface;
    ID3D10GeometryShader_AddRef(*shader);
}

static void STDMETHODCALLTYPE d3d10_device_IAGetPrimitiveTopology(ID3D10Device1 *iface,
        D3D10_PRIMITIVE_TOPOLOGY *topology)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);

    TRACE("iface %p, topology %p.\n", iface, topology);

    wined3d_mutex_lock();
    wined3d_device_get_primitive_type(device->wined3d_device, (enum wined3d_primitive_type *)topology, NULL);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_VSGetShaderResources(ID3D10Device1 *iface,
        UINT start_slot, UINT view_count, ID3D10ShaderResourceView **views)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n",
            iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct wined3d_shader_resource_view *wined3d_view;
        struct d3d_shader_resource_view *view_impl;

        if (!(wined3d_view = wined3d_device_get_vs_resource_view(device->wined3d_device, start_slot + i)))
        {
            views[i] = NULL;
            continue;
        }

        view_impl = wined3d_shader_resource_view_get_parent(wined3d_view);
        views[i] = (ID3D10ShaderResourceView *)&view_impl->ID3D10ShaderResourceView1_iface;
        ID3D10ShaderResourceView_AddRef(views[i]);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_VSGetSamplers(ID3D10Device1 *iface,
        UINT start_slot, UINT sampler_count, ID3D10SamplerState **samplers)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    wined3d_mutex_lock();
    for (i = 0; i < sampler_count; ++i)
    {
        struct d3d_sampler_state *sampler_impl;
        struct wined3d_sampler *wined3d_sampler;

        if (!(wined3d_sampler = wined3d_device_get_vs_sampler(device->wined3d_device, start_slot + i)))
        {
            samplers[i] = NULL;
            continue;
        }

        sampler_impl = wined3d_sampler_get_parent(wined3d_sampler);
        samplers[i] = &sampler_impl->ID3D10SamplerState_iface;
        ID3D10SamplerState_AddRef(samplers[i]);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_GetPredication(ID3D10Device1 *iface,
        ID3D10Predicate **predicate, BOOL *value)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct wined3d_query *wined3d_predicate;
    struct d3d_query *predicate_impl;

    TRACE("iface %p, predicate %p, value %p.\n", iface, predicate, value);

    wined3d_mutex_lock();
    if (!(wined3d_predicate = wined3d_device_get_predication(device->wined3d_device, value)))
    {
        wined3d_mutex_unlock();
        *predicate = NULL;
        return;
    }

    predicate_impl = wined3d_query_get_parent(wined3d_predicate);
    wined3d_mutex_unlock();
    *predicate = (ID3D10Predicate *)&predicate_impl->ID3D10Query_iface;
    ID3D10Predicate_AddRef(*predicate);
}

static void STDMETHODCALLTYPE d3d10_device_GSGetShaderResources(ID3D10Device1 *iface,
        UINT start_slot, UINT view_count, ID3D10ShaderResourceView **views)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n",
            iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct wined3d_shader_resource_view *wined3d_view;
        struct d3d_shader_resource_view *view_impl;

        if (!(wined3d_view = wined3d_device_get_gs_resource_view(device->wined3d_device, start_slot + i)))
        {
            views[i] = NULL;
            continue;
        }

        view_impl = wined3d_shader_resource_view_get_parent(wined3d_view);
        views[i] = (ID3D10ShaderResourceView *)&view_impl->ID3D10ShaderResourceView1_iface;
        ID3D10ShaderResourceView_AddRef(views[i]);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_GSGetSamplers(ID3D10Device1 *iface,
        UINT start_slot, UINT sampler_count, ID3D10SamplerState **samplers)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    wined3d_mutex_lock();
    for (i = 0; i < sampler_count; ++i)
    {
        struct d3d_sampler_state *sampler_impl;
        struct wined3d_sampler *wined3d_sampler;

        if (!(wined3d_sampler = wined3d_device_get_gs_sampler(device->wined3d_device, start_slot + i)))
        {
            samplers[i] = NULL;
            continue;
        }

        sampler_impl = wined3d_sampler_get_parent(wined3d_sampler);
        samplers[i] = &sampler_impl->ID3D10SamplerState_iface;
        ID3D10SamplerState_AddRef(samplers[i]);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_OMGetRenderTargets(ID3D10Device1 *iface,
        UINT view_count, ID3D10RenderTargetView **render_target_views, ID3D10DepthStencilView **depth_stencil_view)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct wined3d_rendertarget_view *wined3d_view;

    TRACE("iface %p, view_count %u, render_target_views %p, depth_stencil_view %p.\n",
            iface, view_count, render_target_views, depth_stencil_view);

    wined3d_mutex_lock();
    if (render_target_views)
    {
        struct d3d_rendertarget_view *view_impl;
        unsigned int i;

        for (i = 0; i < view_count; ++i)
        {
            if (!(wined3d_view = wined3d_device_get_rendertarget_view(device->wined3d_device, i))
                    || !(view_impl = wined3d_rendertarget_view_get_parent(wined3d_view)))
            {
                render_target_views[i] = NULL;
                continue;
            }

            render_target_views[i] = &view_impl->ID3D10RenderTargetView_iface;
            ID3D10RenderTargetView_AddRef(render_target_views[i]);
        }
    }

    if (depth_stencil_view)
    {
        struct d3d_depthstencil_view *view_impl;

        if (!(wined3d_view = wined3d_device_get_depth_stencil_view(device->wined3d_device))
                || !(view_impl = wined3d_rendertarget_view_get_parent(wined3d_view)))
        {
            *depth_stencil_view = NULL;
        }
        else
        {
            *depth_stencil_view = &view_impl->ID3D10DepthStencilView_iface;
            ID3D10DepthStencilView_AddRef(*depth_stencil_view);
        }
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_OMGetBlendState(ID3D10Device1 *iface,
        ID3D10BlendState **blend_state, FLOAT blend_factor[4], UINT *sample_mask)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    ID3D11BlendState *d3d11_blend_state;

    TRACE("iface %p, blend_state %p, blend_factor %p, sample_mask %p.\n",
            iface, blend_state, blend_factor, sample_mask);

    d3d11_immediate_context_OMGetBlendState(&device->immediate_context.ID3D11DeviceContext1_iface,
            &d3d11_blend_state, blend_factor, sample_mask);

    if (d3d11_blend_state)
        *blend_state = (ID3D10BlendState *)&impl_from_ID3D11BlendState(d3d11_blend_state)->ID3D10BlendState1_iface;
    else
        *blend_state = NULL;
}

static void STDMETHODCALLTYPE d3d10_device_OMGetDepthStencilState(ID3D10Device1 *iface,
        ID3D10DepthStencilState **depth_stencil_state, UINT *stencil_ref)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    ID3D11DepthStencilState *d3d11_iface;

    TRACE("iface %p, depth_stencil_state %p, stencil_ref %p.\n",
            iface, depth_stencil_state, stencil_ref);

    d3d11_immediate_context_OMGetDepthStencilState(&device->immediate_context.ID3D11DeviceContext1_iface,
            &d3d11_iface, stencil_ref);

    if (d3d11_iface)
        *depth_stencil_state = &impl_from_ID3D11DepthStencilState(d3d11_iface)->ID3D10DepthStencilState_iface;
    else
        *depth_stencil_state = NULL;
}

static void STDMETHODCALLTYPE d3d10_device_SOGetTargets(ID3D10Device1 *iface,
        UINT buffer_count, ID3D10Buffer **buffers, UINT *offsets)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    unsigned int i;

    TRACE("iface %p, buffer_count %u, buffers %p, offsets %p.\n",
            iface, buffer_count, buffers, offsets);

    wined3d_mutex_lock();
    for (i = 0; i < buffer_count; ++i)
    {
        struct wined3d_buffer *wined3d_buffer;
        struct d3d_buffer *buffer_impl;

        if (!(wined3d_buffer = wined3d_device_get_stream_output(device->wined3d_device, i, &offsets[i])))
        {
            buffers[i] = NULL;
            continue;
        }

        buffer_impl = wined3d_buffer_get_parent(wined3d_buffer);
        buffers[i] = &buffer_impl->ID3D10Buffer_iface;
        ID3D10Buffer_AddRef(buffers[i]);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_RSGetState(ID3D10Device1 *iface, ID3D10RasterizerState **rasterizer_state)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_rasterizer_state *rasterizer_state_impl;
    struct wined3d_rasterizer_state *wined3d_state;

    TRACE("iface %p, rasterizer_state %p.\n", iface, rasterizer_state);

    wined3d_mutex_lock();
    if ((wined3d_state = wined3d_device_get_rasterizer_state(device->wined3d_device)))
    {
        rasterizer_state_impl = wined3d_rasterizer_state_get_parent(wined3d_state);
        ID3D10RasterizerState_AddRef(*rasterizer_state = &rasterizer_state_impl->ID3D10RasterizerState_iface);
    }
    else
    {
        *rasterizer_state = NULL;
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_device_RSGetViewports(ID3D10Device1 *iface,
        UINT *viewport_count, D3D10_VIEWPORT *viewports)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct wined3d_viewport wined3d_vp[WINED3D_MAX_VIEWPORTS];
    unsigned int actual_count = ARRAY_SIZE(wined3d_vp), i;

    TRACE("iface %p, viewport_count %p, viewports %p.\n", iface, viewport_count, viewports);

    if (!viewport_count)
        return;

    wined3d_mutex_lock();
    wined3d_device_get_viewports(device->wined3d_device, &actual_count, viewports ? wined3d_vp : NULL);
    wined3d_mutex_unlock();

    if (!viewports)
    {
        *viewport_count = actual_count;
        return;
    }

    if (*viewport_count > actual_count)
        memset(&viewports[actual_count], 0, (*viewport_count - actual_count) * sizeof(*viewports));

    *viewport_count = min(actual_count, *viewport_count);
    for (i = 0; i < *viewport_count; ++i)
    {
        viewports[i].TopLeftX = wined3d_vp[i].x;
        viewports[i].TopLeftY = wined3d_vp[i].y;
        viewports[i].Width = wined3d_vp[i].width;
        viewports[i].Height = wined3d_vp[i].height;
        viewports[i].MinDepth = wined3d_vp[i].min_z;
        viewports[i].MaxDepth = wined3d_vp[i].max_z;
    }
}

static void STDMETHODCALLTYPE d3d10_device_RSGetScissorRects(ID3D10Device1 *iface, UINT *rect_count, D3D10_RECT *rects)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    unsigned int actual_count;

    TRACE("iface %p, rect_count %p, rects %p.\n", iface, rect_count, rects);

    if (!rect_count)
        return;

    actual_count = *rect_count;

    wined3d_mutex_lock();
    wined3d_device_get_scissor_rects(device->wined3d_device, &actual_count, rects);
    wined3d_mutex_unlock();

    if (!rects)
    {
        *rect_count = actual_count;
        return;
    }

    if (*rect_count > actual_count)
        memset(&rects[actual_count], 0, (*rect_count - actual_count) * sizeof(*rects));
}

static HRESULT STDMETHODCALLTYPE d3d10_device_GetDeviceRemovedReason(ID3D10Device1 *iface)
{
    TRACE("iface %p.\n", iface);

    /* In the current implementation the device is never removed, so we can
     * just return S_OK here. */

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_device_SetExceptionMode(ID3D10Device1 *iface, UINT flags)
{
    FIXME("iface %p, flags %#x stub!\n", iface, flags);

    return E_NOTIMPL;
}

static UINT STDMETHODCALLTYPE d3d10_device_GetExceptionMode(ID3D10Device1 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT STDMETHODCALLTYPE d3d10_device_GetPrivateData(ID3D10Device1 *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d11_device_GetPrivateData(&device->ID3D11Device2_iface, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_device_SetPrivateData(ID3D10Device1 *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d11_device_SetPrivateData(&device->ID3D11Device2_iface, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_device_SetPrivateDataInterface(ID3D10Device1 *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d11_device_SetPrivateDataInterface(&device->ID3D11Device2_iface, guid, data);
}

static void STDMETHODCALLTYPE d3d10_device_ClearState(ID3D10Device1 *iface)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);

    TRACE("iface %p.\n", iface);

    d3d11_immediate_context_ClearState(&device->immediate_context.ID3D11DeviceContext1_iface);
}

static void STDMETHODCALLTYPE d3d10_device_Flush(ID3D10Device1 *iface)
{
    FIXME("iface %p stub!\n", iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CreateBuffer(ID3D10Device1 *iface,
        const D3D10_BUFFER_DESC *desc, const D3D10_SUBRESOURCE_DATA *data, ID3D10Buffer **buffer)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    D3D11_BUFFER_DESC d3d11_desc;
    struct d3d_buffer *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, data %p, buffer %p.\n", iface, desc, data, buffer);

    d3d11_desc.ByteWidth = desc->ByteWidth;
    d3d11_desc.Usage = d3d11_usage_from_d3d10_usage(desc->Usage);
    d3d11_desc.BindFlags = d3d11_bind_flags_from_d3d10_bind_flags(desc->BindFlags);
    d3d11_desc.CPUAccessFlags = d3d11_cpu_access_flags_from_d3d10_cpu_access_flags(desc->CPUAccessFlags);
    d3d11_desc.MiscFlags = d3d11_resource_misc_flags_from_d3d10_resource_misc_flags(desc->MiscFlags);
    d3d11_desc.StructureByteStride = 0;

    if (FAILED(hr = d3d_buffer_create(device, &d3d11_desc, (const D3D11_SUBRESOURCE_DATA *)data, &object)))
        return hr;

    *buffer = &object->ID3D10Buffer_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CreateTexture1D(ID3D10Device1 *iface,
        const D3D10_TEXTURE1D_DESC *desc, const D3D10_SUBRESOURCE_DATA *data, ID3D10Texture1D **texture)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    D3D11_TEXTURE1D_DESC d3d11_desc;
    struct d3d_texture1d *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, data %p, texture %p.\n", iface, desc, data, texture);

    d3d11_desc.Width = desc->Width;
    d3d11_desc.MipLevels = desc->MipLevels;
    d3d11_desc.ArraySize = desc->ArraySize;
    d3d11_desc.Format = desc->Format;
    d3d11_desc.Usage = d3d11_usage_from_d3d10_usage(desc->Usage);
    d3d11_desc.BindFlags = d3d11_bind_flags_from_d3d10_bind_flags(desc->BindFlags);
    d3d11_desc.CPUAccessFlags = d3d11_cpu_access_flags_from_d3d10_cpu_access_flags(desc->CPUAccessFlags);
    d3d11_desc.MiscFlags = d3d11_resource_misc_flags_from_d3d10_resource_misc_flags(desc->MiscFlags);

    if (FAILED(hr = d3d_texture1d_create(device, &d3d11_desc, (const D3D11_SUBRESOURCE_DATA *)data, &object)))
        return hr;

    *texture = &object->ID3D10Texture1D_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CreateTexture2D(ID3D10Device1 *iface,
        const D3D10_TEXTURE2D_DESC *desc, const D3D10_SUBRESOURCE_DATA *data,
        ID3D10Texture2D **texture)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    D3D11_TEXTURE2D_DESC d3d11_desc;
    struct d3d_texture2d *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, data %p, texture %p.\n", iface, desc, data, texture);

    d3d11_desc.Width = desc->Width;
    d3d11_desc.Height = desc->Height;
    d3d11_desc.MipLevels = desc->MipLevels;
    d3d11_desc.ArraySize = desc->ArraySize;
    d3d11_desc.Format = desc->Format;
    d3d11_desc.SampleDesc = desc->SampleDesc;
    d3d11_desc.Usage = d3d11_usage_from_d3d10_usage(desc->Usage);
    d3d11_desc.BindFlags = d3d11_bind_flags_from_d3d10_bind_flags(desc->BindFlags);
    d3d11_desc.CPUAccessFlags = d3d11_cpu_access_flags_from_d3d10_cpu_access_flags(desc->CPUAccessFlags);
    d3d11_desc.MiscFlags = d3d11_resource_misc_flags_from_d3d10_resource_misc_flags(desc->MiscFlags);

    if (FAILED(hr = d3d_texture2d_create(device, &d3d11_desc, (const D3D11_SUBRESOURCE_DATA *)data, &object)))
        return hr;

    *texture = &object->ID3D10Texture2D_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CreateTexture3D(ID3D10Device1 *iface,
        const D3D10_TEXTURE3D_DESC *desc, const D3D10_SUBRESOURCE_DATA *data,
        ID3D10Texture3D **texture)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    D3D11_TEXTURE3D_DESC d3d11_desc;
    struct d3d_texture3d *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, data %p, texture %p.\n", iface, desc, data, texture);

    d3d11_desc.Width = desc->Width;
    d3d11_desc.Height = desc->Height;
    d3d11_desc.Depth = desc->Depth;
    d3d11_desc.MipLevels = desc->MipLevels;
    d3d11_desc.Format = desc->Format;
    d3d11_desc.Usage = d3d11_usage_from_d3d10_usage(desc->Usage);
    d3d11_desc.BindFlags = d3d11_bind_flags_from_d3d10_bind_flags(desc->BindFlags);
    d3d11_desc.CPUAccessFlags = d3d11_cpu_access_flags_from_d3d10_cpu_access_flags(desc->CPUAccessFlags);
    d3d11_desc.MiscFlags = d3d11_resource_misc_flags_from_d3d10_resource_misc_flags(desc->MiscFlags);

    if (FAILED(hr = d3d_texture3d_create(device, &d3d11_desc, (const D3D11_SUBRESOURCE_DATA *)data, &object)))
        return hr;

    *texture = &object->ID3D10Texture3D_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CreateShaderResourceView1(ID3D10Device1 *iface,
        ID3D10Resource *resource, const D3D10_SHADER_RESOURCE_VIEW_DESC1 *desc, ID3D10ShaderResourceView1 **view)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_shader_resource_view *object;
    ID3D11Resource *d3d11_resource;
    HRESULT hr;

    TRACE("iface %p, resource %p, desc %p, view %p.\n", iface, resource, desc, view);

    if (!resource)
        return E_INVALIDARG;

    if (FAILED(hr = ID3D10Resource_QueryInterface(resource, &IID_ID3D11Resource, (void **)&d3d11_resource)))
    {
        ERR("Resource does not implement ID3D11Resource.\n");
        return E_FAIL;
    }

    hr = d3d_shader_resource_view_create(device, d3d11_resource, (const D3D11_SHADER_RESOURCE_VIEW_DESC *)desc,
            &object);
    ID3D11Resource_Release(d3d11_resource);
    if (FAILED(hr))
        return hr;

    *view = &object->ID3D10ShaderResourceView1_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CreateShaderResourceView(ID3D10Device1 *iface,
        ID3D10Resource *resource, const D3D10_SHADER_RESOURCE_VIEW_DESC *desc, ID3D10ShaderResourceView **view)
{
    TRACE("iface %p, resource %p, desc %p, view %p.\n", iface, resource, desc, view);

    return d3d10_device_CreateShaderResourceView1(iface, resource,
            (const D3D10_SHADER_RESOURCE_VIEW_DESC1 *)desc, (ID3D10ShaderResourceView1 **)view);
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CreateRenderTargetView(ID3D10Device1 *iface,
        ID3D10Resource *resource, const D3D10_RENDER_TARGET_VIEW_DESC *desc, ID3D10RenderTargetView **view)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_rendertarget_view *object;
    ID3D11Resource *d3d11_resource;
    HRESULT hr;

    TRACE("iface %p, resource %p, desc %p, view %p.\n", iface, resource, desc, view);

    if (!resource)
        return E_INVALIDARG;

    if (FAILED(hr = ID3D10Resource_QueryInterface(resource, &IID_ID3D11Resource, (void **)&d3d11_resource)))
    {
        ERR("Resource does not implement ID3D11Resource.\n");
        return E_FAIL;
    }

    hr = d3d_rendertarget_view_create(device, d3d11_resource, (const D3D11_RENDER_TARGET_VIEW_DESC *)desc, &object);
    ID3D11Resource_Release(d3d11_resource);
    if (FAILED(hr))
        return hr;

    *view = &object->ID3D10RenderTargetView_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CreateDepthStencilView(ID3D10Device1 *iface,
        ID3D10Resource *resource, const D3D10_DEPTH_STENCIL_VIEW_DESC *desc, ID3D10DepthStencilView **view)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    D3D11_DEPTH_STENCIL_VIEW_DESC d3d11_desc;
    struct d3d_depthstencil_view *object;
    ID3D11Resource *d3d11_resource;
    HRESULT hr;

    TRACE("iface %p, resource %p, desc %p, view %p.\n", iface, resource, desc, view);

    if (desc)
    {
        d3d11_desc.Format = desc->Format;
        d3d11_desc.ViewDimension = desc->ViewDimension;
        d3d11_desc.Flags = 0;
        memcpy(&d3d11_desc.u, &desc->u, sizeof(d3d11_desc.u));
    }

    if (FAILED(hr = ID3D10Resource_QueryInterface(resource, &IID_ID3D11Resource, (void **)&d3d11_resource)))
    {
        ERR("Resource does not implement ID3D11Resource.\n");
        return E_FAIL;
    }

    hr = d3d_depthstencil_view_create(device, d3d11_resource, desc ? &d3d11_desc : NULL, &object);
    ID3D11Resource_Release(d3d11_resource);
    if (FAILED(hr))
        return hr;

    *view = &object->ID3D10DepthStencilView_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CreateInputLayout(ID3D10Device1 *iface,
        const D3D10_INPUT_ELEMENT_DESC *element_descs, UINT element_count,
        const void *shader_byte_code, SIZE_T shader_byte_code_length,
        ID3D10InputLayout **input_layout)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_input_layout *object;
    HRESULT hr;

    TRACE("iface %p, element_descs %p, element_count %u, shader_byte_code %p, "
            "shader_byte_code_length %lu, input_layout %p\n",
            iface, element_descs, element_count, shader_byte_code,
            shader_byte_code_length, input_layout);

    if (FAILED(hr = d3d_input_layout_create(device, (const D3D11_INPUT_ELEMENT_DESC *)element_descs, element_count,
            shader_byte_code, shader_byte_code_length, &object)))
        return hr;

    *input_layout = &object->ID3D10InputLayout_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CreateVertexShader(ID3D10Device1 *iface,
        const void *byte_code, SIZE_T byte_code_length, ID3D10VertexShader **shader)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_vertex_shader *object;
    HRESULT hr;

    TRACE("iface %p, byte_code %p, byte_code_length %lu, shader %p.\n",
            iface, byte_code, byte_code_length, shader);

    if (FAILED(hr = d3d_vertex_shader_create(device, byte_code, byte_code_length, &object)))
        return hr;

    *shader = &object->ID3D10VertexShader_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CreateGeometryShader(ID3D10Device1 *iface,
        const void *byte_code, SIZE_T byte_code_length, ID3D10GeometryShader **shader)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_geometry_shader *object;
    HRESULT hr;

    TRACE("iface %p, byte_code %p, byte_code_length %lu, shader %p.\n",
            iface, byte_code, byte_code_length, shader);

    if (FAILED(hr = d3d_geometry_shader_create(device, byte_code, byte_code_length,
            NULL, 0, NULL, 0, 0, &object)))
        return hr;

    *shader = &object->ID3D10GeometryShader_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CreateGeometryShaderWithStreamOutput(ID3D10Device1 *iface,
        const void *byte_code, SIZE_T byte_code_length, const D3D10_SO_DECLARATION_ENTRY *output_stream_decls,
        UINT output_stream_decl_count, UINT output_stream_stride, ID3D10GeometryShader **shader)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    D3D11_SO_DECLARATION_ENTRY *so_entries = NULL;
    struct d3d_geometry_shader *object;
    unsigned int i, stride_count = 1;
    HRESULT hr;

    TRACE("iface %p, byte_code %p, byte_code_length %lu, output_stream_decls %p, "
            "output_stream_decl_count %u, output_stream_stride %u, shader %p.\n",
            iface, byte_code, byte_code_length, output_stream_decls,
            output_stream_decl_count, output_stream_stride, shader);

    if (!output_stream_decl_count && output_stream_stride)
    {
        WARN("Stride must be 0 when declaration entry count is 0.\n");
        *shader = NULL;
        return E_INVALIDARG;
    }

    if (output_stream_decl_count
            && !(so_entries = heap_calloc(output_stream_decl_count, sizeof(*so_entries))))
    {
        ERR("Failed to allocate D3D11 SO declaration array memory.\n");
        *shader = NULL;
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < output_stream_decl_count; ++i)
    {
        so_entries[i].Stream = 0;
        so_entries[i].SemanticName = output_stream_decls[i].SemanticName;
        so_entries[i].SemanticIndex = output_stream_decls[i].SemanticIndex;
        so_entries[i].StartComponent = output_stream_decls[i].StartComponent;
        so_entries[i].ComponentCount = output_stream_decls[i].ComponentCount;
        so_entries[i].OutputSlot = output_stream_decls[i].OutputSlot;

        if (output_stream_decls[i].OutputSlot)
        {
            stride_count = 0;
            if (output_stream_stride)
            {
                WARN("Stride must be 0 when multiple output slots are used.\n");
                heap_free(so_entries);
                *shader = NULL;
                return E_INVALIDARG;
            }
        }
    }

    hr = d3d_geometry_shader_create(device, byte_code, byte_code_length,
            so_entries, output_stream_decl_count, &output_stream_stride, stride_count, 0, &object);
    heap_free(so_entries);
    if (FAILED(hr))
    {
        *shader = NULL;
        return hr;
    }

    *shader = &object->ID3D10GeometryShader_iface;

    return hr;
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CreatePixelShader(ID3D10Device1 *iface,
        const void *byte_code, SIZE_T byte_code_length, ID3D10PixelShader **shader)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_pixel_shader *object;
    HRESULT hr;

    TRACE("iface %p, byte_code %p, byte_code_length %lu, shader %p.\n",
            iface, byte_code, byte_code_length, shader);

    if (FAILED(hr = d3d_pixel_shader_create(device, byte_code, byte_code_length, &object)))
        return hr;

    *shader = &object->ID3D10PixelShader_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CreateBlendState1(ID3D10Device1 *iface,
        const D3D10_BLEND_DESC1 *desc, ID3D10BlendState1 **blend_state)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_blend_state *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, blend_state %p.\n", iface, desc, blend_state);

    if (FAILED(hr = d3d_blend_state_create(device, (const D3D11_BLEND_DESC *)desc, &object)))
        return hr;

    *blend_state = &object->ID3D10BlendState1_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CreateBlendState(ID3D10Device1 *iface,
        const D3D10_BLEND_DESC *desc, ID3D10BlendState **blend_state)
{
    D3D10_BLEND_DESC1 d3d10_1_desc;
    unsigned int i;

    TRACE("iface %p, desc %p, blend_state %p.\n", iface, desc, blend_state);

    if (!desc)
        return E_INVALIDARG;

    d3d10_1_desc.AlphaToCoverageEnable = desc->AlphaToCoverageEnable;
    d3d10_1_desc.IndependentBlendEnable = FALSE;
    for (i = 0; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT - 1; ++i)
    {
        if (desc->BlendEnable[i] != desc->BlendEnable[i + 1]
                || desc->RenderTargetWriteMask[i] != desc->RenderTargetWriteMask[i + 1])
            d3d10_1_desc.IndependentBlendEnable = TRUE;
    }

    for (i = 0; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        d3d10_1_desc.RenderTarget[i].BlendEnable = desc->BlendEnable[i];
        d3d10_1_desc.RenderTarget[i].SrcBlend = desc->SrcBlend;
        d3d10_1_desc.RenderTarget[i].DestBlend = desc->DestBlend;
        d3d10_1_desc.RenderTarget[i].BlendOp = desc->BlendOp;
        d3d10_1_desc.RenderTarget[i].SrcBlendAlpha = desc->SrcBlendAlpha;
        d3d10_1_desc.RenderTarget[i].DestBlendAlpha = desc->DestBlendAlpha;
        d3d10_1_desc.RenderTarget[i].BlendOpAlpha = desc->BlendOpAlpha;
        d3d10_1_desc.RenderTarget[i].RenderTargetWriteMask = desc->RenderTargetWriteMask[i];
    }

    return d3d10_device_CreateBlendState1(iface, &d3d10_1_desc, (ID3D10BlendState1 **)blend_state);
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CreateDepthStencilState(ID3D10Device1 *iface,
        const D3D10_DEPTH_STENCIL_DESC *desc, ID3D10DepthStencilState **depth_stencil_state)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_depthstencil_state *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, depth_stencil_state %p.\n", iface, desc, depth_stencil_state);

    if (FAILED(hr = d3d_depthstencil_state_create(device, (const D3D11_DEPTH_STENCIL_DESC *)desc, &object)))
        return hr;

    *depth_stencil_state = &object->ID3D10DepthStencilState_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CreateRasterizerState(ID3D10Device1 *iface,
        const D3D10_RASTERIZER_DESC *desc, ID3D10RasterizerState **rasterizer_state)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_rasterizer_state *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, rasterizer_state %p.\n", iface, desc, rasterizer_state);

    if (FAILED(hr = d3d_rasterizer_state_create(device, (const D3D11_RASTERIZER_DESC *)desc, &object)))
        return hr;

    *rasterizer_state = &object->ID3D10RasterizerState_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CreateSamplerState(ID3D10Device1 *iface,
        const D3D10_SAMPLER_DESC *desc, ID3D10SamplerState **sampler_state)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_sampler_state *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, sampler_state %p.\n", iface, desc, sampler_state);

    if (FAILED(hr = d3d_sampler_state_create(device, (const D3D11_SAMPLER_DESC *)desc, &object)))
        return hr;

    *sampler_state = &object->ID3D10SamplerState_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CreateQuery(ID3D10Device1 *iface,
        const D3D10_QUERY_DESC *desc, ID3D10Query **query)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_query *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, query %p.\n", iface, desc, query);

    if (FAILED(hr = d3d_query_create(device, (const D3D11_QUERY_DESC *)desc, FALSE, &object)))
        return hr;

    if (query)
    {
        *query = &object->ID3D10Query_iface;
        return S_OK;
    }

    ID3D10Query_Release(&object->ID3D10Query_iface);
    return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CreatePredicate(ID3D10Device1 *iface,
        const D3D10_QUERY_DESC *desc, ID3D10Predicate **predicate)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_query *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, predicate %p.\n", iface, desc, predicate);

    if (FAILED(hr = d3d_query_create(device, (const D3D11_QUERY_DESC *)desc, TRUE, &object)))
        return hr;

    if (predicate)
    {
        *predicate = (ID3D10Predicate *)&object->ID3D10Query_iface;
        return S_OK;
    }

    ID3D10Query_Release(&object->ID3D10Query_iface);
    return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CreateCounter(ID3D10Device1 *iface,
        const D3D10_COUNTER_DESC *desc, ID3D10Counter **counter)
{
    FIXME("iface %p, desc %p, counter %p stub!\n", iface, desc, counter);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CheckFormatSupport(ID3D10Device1 *iface,
        DXGI_FORMAT format, UINT *format_support)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);

    TRACE("iface %p, format %s, format_support %p.\n",
            iface, debug_dxgi_format(format), format_support);

    return d3d11_device_CheckFormatSupport(&device->ID3D11Device2_iface, format, format_support);
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CheckMultisampleQualityLevels(ID3D10Device1 *iface,
        DXGI_FORMAT format, UINT sample_count, UINT *quality_level_count)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);

    TRACE("iface %p, format %s, sample_count %u, quality_level_count %p.\n",
            iface, debug_dxgi_format(format), sample_count, quality_level_count);

    return d3d11_device_CheckMultisampleQualityLevels(&device->ID3D11Device2_iface, format,
            sample_count, quality_level_count);
}

static void STDMETHODCALLTYPE d3d10_device_CheckCounterInfo(ID3D10Device1 *iface, D3D10_COUNTER_INFO *counter_info)
{
    FIXME("iface %p, counter_info %p stub!\n", iface, counter_info);
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CheckCounter(ID3D10Device1 *iface,
        const D3D10_COUNTER_DESC *desc, D3D10_COUNTER_TYPE *type, UINT *active_counters, char *name,
        UINT *name_length, char *units, UINT *units_length, char *description, UINT *description_length)
{
    FIXME("iface %p, desc %p, type %p, active_counters %p, name %p, name_length %p, "
            "units %p, units_length %p, description %p, description_length %p stub!\n",
            iface, desc, type, active_counters, name, name_length,
            units, units_length, description, description_length);

    return E_NOTIMPL;
}

static UINT STDMETHODCALLTYPE d3d10_device_GetCreationFlags(ID3D10Device1 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT STDMETHODCALLTYPE d3d10_device_OpenSharedResource(ID3D10Device1 *iface,
        HANDLE resource_handle, REFIID guid, void **resource)
{
    FIXME("iface %p, resource_handle %p, guid %s, resource %p stub!\n",
            iface, resource_handle, debugstr_guid(guid), resource);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d3d10_device_SetTextFilterSize(ID3D10Device1 *iface, UINT width, UINT height)
{
    FIXME("iface %p, width %u, height %u stub!\n", iface, width, height);
}

static void STDMETHODCALLTYPE d3d10_device_GetTextFilterSize(ID3D10Device1 *iface, UINT *width, UINT *height)
{
    FIXME("iface %p, width %p, height %p stub!\n", iface, width, height);
}

static D3D10_FEATURE_LEVEL1 STDMETHODCALLTYPE d3d10_device_GetFeatureLevel(ID3D10Device1 *iface)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);

    TRACE("iface %p.\n", iface);

    return device->feature_level;
}

static const struct ID3D10Device1Vtbl d3d10_device1_vtbl =
{
    /* IUnknown methods */
    d3d10_device_QueryInterface,
    d3d10_device_AddRef,
    d3d10_device_Release,
    /* ID3D10Device methods */
    d3d10_device_VSSetConstantBuffers,
    d3d10_device_PSSetShaderResources,
    d3d10_device_PSSetShader,
    d3d10_device_PSSetSamplers,
    d3d10_device_VSSetShader,
    d3d10_device_DrawIndexed,
    d3d10_device_Draw,
    d3d10_device_PSSetConstantBuffers,
    d3d10_device_IASetInputLayout,
    d3d10_device_IASetVertexBuffers,
    d3d10_device_IASetIndexBuffer,
    d3d10_device_DrawIndexedInstanced,
    d3d10_device_DrawInstanced,
    d3d10_device_GSSetConstantBuffers,
    d3d10_device_GSSetShader,
    d3d10_device_IASetPrimitiveTopology,
    d3d10_device_VSSetShaderResources,
    d3d10_device_VSSetSamplers,
    d3d10_device_SetPredication,
    d3d10_device_GSSetShaderResources,
    d3d10_device_GSSetSamplers,
    d3d10_device_OMSetRenderTargets,
    d3d10_device_OMSetBlendState,
    d3d10_device_OMSetDepthStencilState,
    d3d10_device_SOSetTargets,
    d3d10_device_DrawAuto,
    d3d10_device_RSSetState,
    d3d10_device_RSSetViewports,
    d3d10_device_RSSetScissorRects,
    d3d10_device_CopySubresourceRegion,
    d3d10_device_CopyResource,
    d3d10_device_UpdateSubresource,
    d3d10_device_ClearRenderTargetView,
    d3d10_device_ClearDepthStencilView,
    d3d10_device_GenerateMips,
    d3d10_device_ResolveSubresource,
    d3d10_device_VSGetConstantBuffers,
    d3d10_device_PSGetShaderResources,
    d3d10_device_PSGetShader,
    d3d10_device_PSGetSamplers,
    d3d10_device_VSGetShader,
    d3d10_device_PSGetConstantBuffers,
    d3d10_device_IAGetInputLayout,
    d3d10_device_IAGetVertexBuffers,
    d3d10_device_IAGetIndexBuffer,
    d3d10_device_GSGetConstantBuffers,
    d3d10_device_GSGetShader,
    d3d10_device_IAGetPrimitiveTopology,
    d3d10_device_VSGetShaderResources,
    d3d10_device_VSGetSamplers,
    d3d10_device_GetPredication,
    d3d10_device_GSGetShaderResources,
    d3d10_device_GSGetSamplers,
    d3d10_device_OMGetRenderTargets,
    d3d10_device_OMGetBlendState,
    d3d10_device_OMGetDepthStencilState,
    d3d10_device_SOGetTargets,
    d3d10_device_RSGetState,
    d3d10_device_RSGetViewports,
    d3d10_device_RSGetScissorRects,
    d3d10_device_GetDeviceRemovedReason,
    d3d10_device_SetExceptionMode,
    d3d10_device_GetExceptionMode,
    d3d10_device_GetPrivateData,
    d3d10_device_SetPrivateData,
    d3d10_device_SetPrivateDataInterface,
    d3d10_device_ClearState,
    d3d10_device_Flush,
    d3d10_device_CreateBuffer,
    d3d10_device_CreateTexture1D,
    d3d10_device_CreateTexture2D,
    d3d10_device_CreateTexture3D,
    d3d10_device_CreateShaderResourceView,
    d3d10_device_CreateRenderTargetView,
    d3d10_device_CreateDepthStencilView,
    d3d10_device_CreateInputLayout,
    d3d10_device_CreateVertexShader,
    d3d10_device_CreateGeometryShader,
    d3d10_device_CreateGeometryShaderWithStreamOutput,
    d3d10_device_CreatePixelShader,
    d3d10_device_CreateBlendState,
    d3d10_device_CreateDepthStencilState,
    d3d10_device_CreateRasterizerState,
    d3d10_device_CreateSamplerState,
    d3d10_device_CreateQuery,
    d3d10_device_CreatePredicate,
    d3d10_device_CreateCounter,
    d3d10_device_CheckFormatSupport,
    d3d10_device_CheckMultisampleQualityLevels,
    d3d10_device_CheckCounterInfo,
    d3d10_device_CheckCounter,
    d3d10_device_GetCreationFlags,
    d3d10_device_OpenSharedResource,
    d3d10_device_SetTextFilterSize,
    d3d10_device_GetTextFilterSize,
    d3d10_device_CreateShaderResourceView1,
    d3d10_device_CreateBlendState1,
    d3d10_device_GetFeatureLevel,
};

static const struct IUnknownVtbl d3d_device_inner_unknown_vtbl =
{
    /* IUnknown methods */
    d3d_device_inner_QueryInterface,
    d3d_device_inner_AddRef,
    d3d_device_inner_Release,
};

/* ID3D10Multithread methods */

static inline struct d3d_device *impl_from_ID3D10Multithread(ID3D10Multithread *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_device, ID3D10Multithread_iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_multithread_QueryInterface(ID3D10Multithread *iface, REFIID iid, void **out)
{
    struct d3d_device *device = impl_from_ID3D10Multithread(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    return IUnknown_QueryInterface(device->outer_unk, iid, out);
}

static ULONG STDMETHODCALLTYPE d3d10_multithread_AddRef(ID3D10Multithread *iface)
{
    struct d3d_device *device = impl_from_ID3D10Multithread(iface);

    TRACE("iface %p.\n", iface);

    return IUnknown_AddRef(device->outer_unk);
}

static ULONG STDMETHODCALLTYPE d3d10_multithread_Release(ID3D10Multithread *iface)
{
    struct d3d_device *device = impl_from_ID3D10Multithread(iface);

    TRACE("iface %p.\n", iface);

    return IUnknown_Release(device->outer_unk);
}

static void STDMETHODCALLTYPE d3d10_multithread_Enter(ID3D10Multithread *iface)
{
    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
}

static void STDMETHODCALLTYPE d3d10_multithread_Leave(ID3D10Multithread *iface)
{
    TRACE("iface %p.\n", iface);

    wined3d_mutex_unlock();
}

static BOOL STDMETHODCALLTYPE d3d10_multithread_SetMultithreadProtected(ID3D10Multithread *iface, BOOL enable)
{
    FIXME("iface %p, enable %#x stub!\n", iface, enable);

    return TRUE;
}

static BOOL STDMETHODCALLTYPE d3d10_multithread_GetMultithreadProtected(ID3D10Multithread *iface)
{
    FIXME("iface %p stub!\n", iface);

    return TRUE;
}

static const struct ID3D10MultithreadVtbl d3d10_multithread_vtbl =
{
    d3d10_multithread_QueryInterface,
    d3d10_multithread_AddRef,
    d3d10_multithread_Release,
    d3d10_multithread_Enter,
    d3d10_multithread_Leave,
    d3d10_multithread_SetMultithreadProtected,
    d3d10_multithread_GetMultithreadProtected,
};

/* IWineDXGIDeviceParent IUnknown methods */

static inline struct d3d_device *device_from_dxgi_device_parent(IWineDXGIDeviceParent *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_device, IWineDXGIDeviceParent_iface);
}

static HRESULT STDMETHODCALLTYPE dxgi_device_parent_QueryInterface(IWineDXGIDeviceParent *iface,
        REFIID iid, void **out)
{
    struct d3d_device *device = device_from_dxgi_device_parent(iface);
    return IUnknown_QueryInterface(device->outer_unk, iid, out);
}

static ULONG STDMETHODCALLTYPE dxgi_device_parent_AddRef(IWineDXGIDeviceParent *iface)
{
    struct d3d_device *device = device_from_dxgi_device_parent(iface);
    return IUnknown_AddRef(device->outer_unk);
}

static ULONG STDMETHODCALLTYPE dxgi_device_parent_Release(IWineDXGIDeviceParent *iface)
{
    struct d3d_device *device = device_from_dxgi_device_parent(iface);
    return IUnknown_Release(device->outer_unk);
}

static struct wined3d_device_parent * STDMETHODCALLTYPE dxgi_device_parent_get_wined3d_device_parent(
        IWineDXGIDeviceParent *iface)
{
    struct d3d_device *device = device_from_dxgi_device_parent(iface);
    return &device->device_parent;
}

static const struct IWineDXGIDeviceParentVtbl d3d_dxgi_device_parent_vtbl =
{
    /* IUnknown methods */
    dxgi_device_parent_QueryInterface,
    dxgi_device_parent_AddRef,
    dxgi_device_parent_Release,
    /* IWineDXGIDeviceParent methods */
    dxgi_device_parent_get_wined3d_device_parent,
};

static inline struct d3d_device *device_from_wined3d_device_parent(struct wined3d_device_parent *device_parent)
{
    return CONTAINING_RECORD(device_parent, struct d3d_device, device_parent);
}

static void CDECL device_parent_wined3d_device_created(struct wined3d_device_parent *device_parent,
        struct wined3d_device *wined3d_device)
{
    struct d3d_device *device = device_from_wined3d_device_parent(device_parent);

    TRACE("device_parent %p, wined3d_device %p.\n", device_parent, wined3d_device);

    wined3d_device_incref(wined3d_device);
    device->wined3d_device = wined3d_device;

    device->feature_level = wined3d_device_get_feature_level(wined3d_device);

    set_default_depth_stencil_state(wined3d_device);
}

static void CDECL device_parent_mode_changed(struct wined3d_device_parent *device_parent)
{
    TRACE("device_parent %p.\n", device_parent);
}

static void CDECL device_parent_activate(struct wined3d_device_parent *device_parent, BOOL activate)
{
    TRACE("device_parent %p, activate %#x.\n", device_parent, activate);
}

static HRESULT CDECL device_parent_texture_sub_resource_created(struct wined3d_device_parent *device_parent,
        enum wined3d_resource_type type, struct wined3d_texture *wined3d_texture, unsigned int sub_resource_idx,
        void **parent, const struct wined3d_parent_ops **parent_ops)
{
    TRACE("device_parent %p, type %#x, wined3d_texture %p, sub_resource_idx %u, parent %p, parent_ops %p.\n",
            device_parent, type, wined3d_texture, sub_resource_idx, parent, parent_ops);

    *parent = NULL;
    *parent_ops = &d3d_null_wined3d_parent_ops;

    return S_OK;
}

static HRESULT CDECL device_parent_create_swapchain_texture(struct wined3d_device_parent *device_parent,
        void *container_parent, const struct wined3d_resource_desc *wined3d_desc, DWORD texture_flags,
        struct wined3d_texture **wined3d_texture)
{
    struct d3d_device *device = device_from_wined3d_device_parent(device_parent);
    struct d3d_texture2d *texture;
    ID3D11Texture2D *texture_iface;
    D3D11_TEXTURE2D_DESC desc;
    HRESULT hr;

    TRACE("device_parent %p, container_parent %p, wined3d_desc %p, texture_flags %#x, wined3d_texture %p.\n",
            device_parent, container_parent, wined3d_desc, texture_flags, wined3d_texture);

    desc.Width = wined3d_desc->width;
    desc.Height = wined3d_desc->height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = dxgi_format_from_wined3dformat(wined3d_desc->format);
    desc.SampleDesc.Count = wined3d_desc->multisample_type ? wined3d_desc->multisample_type : 1;
    desc.SampleDesc.Quality = wined3d_desc->multisample_quality;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = d3d11_bind_flags_from_wined3d(wined3d_desc->bind_flags);
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    if (texture_flags & WINED3D_TEXTURE_CREATE_GET_DC)
    {
        desc.MiscFlags |= D3D11_RESOURCE_MISC_GDI_COMPATIBLE;
        texture_flags &= ~WINED3D_TEXTURE_CREATE_GET_DC;
    }

    if (texture_flags)
        FIXME("Unhandled flags %#x.\n", texture_flags);

    if (FAILED(hr = d3d11_device_CreateTexture2D(&device->ID3D11Device2_iface,
            &desc, NULL, &texture_iface)))
    {
        WARN("Failed to create 2D texture, hr %#x.\n", hr);
        return hr;
    }

    texture = impl_from_ID3D11Texture2D(texture_iface);

    *wined3d_texture = texture->wined3d_texture;
    wined3d_texture_incref(*wined3d_texture);
    ID3D11Texture2D_Release(&texture->ID3D11Texture2D_iface);

    return S_OK;
}

static const struct wined3d_device_parent_ops d3d_wined3d_device_parent_ops =
{
    device_parent_wined3d_device_created,
    device_parent_mode_changed,
    device_parent_activate,
    device_parent_texture_sub_resource_created,
    device_parent_create_swapchain_texture,
};

static int d3d_sampler_state_compare(const void *key, const struct wine_rb_entry *entry)
{
    const D3D11_SAMPLER_DESC *ka = key;
    const D3D11_SAMPLER_DESC *kb = &WINE_RB_ENTRY_VALUE(entry, const struct d3d_sampler_state, entry)->desc;

    return memcmp(ka, kb, sizeof(*ka));
}

static int d3d_blend_state_compare(const void *key, const struct wine_rb_entry *entry)
{
    const D3D11_BLEND_DESC *ka = key;
    const D3D11_BLEND_DESC *kb = &WINE_RB_ENTRY_VALUE(entry, const struct d3d_blend_state, entry)->desc;

    return memcmp(ka, kb, sizeof(*ka));
}

static int d3d_depthstencil_state_compare(const void *key, const struct wine_rb_entry *entry)
{
    const D3D11_DEPTH_STENCIL_DESC *ka = key;
    const D3D11_DEPTH_STENCIL_DESC *kb = &WINE_RB_ENTRY_VALUE(entry,
            const struct d3d_depthstencil_state, entry)->desc;

    return memcmp(ka, kb, sizeof(*ka));
}

static int d3d_rasterizer_state_compare(const void *key, const struct wine_rb_entry *entry)
{
    const D3D11_RASTERIZER_DESC *ka = key;
    const D3D11_RASTERIZER_DESC *kb = &WINE_RB_ENTRY_VALUE(entry, const struct d3d_rasterizer_state, entry)->desc;

    return memcmp(ka, kb, sizeof(*ka));
}

void d3d_device_init(struct d3d_device *device, void *outer_unknown)
{
    device->IUnknown_inner.lpVtbl = &d3d_device_inner_unknown_vtbl;
    device->ID3D11Device2_iface.lpVtbl = &d3d11_device_vtbl;
    device->ID3D10Device1_iface.lpVtbl = &d3d10_device1_vtbl;
    device->ID3D10Multithread_iface.lpVtbl = &d3d10_multithread_vtbl;
    device->IWineDXGIDeviceParent_iface.lpVtbl = &d3d_dxgi_device_parent_vtbl;
    device->device_parent.ops = &d3d_wined3d_device_parent_ops;
    device->refcount = 1;
    /* COM aggregation always takes place */
    device->outer_unk = outer_unknown;

    d3d11_immediate_context_init(&device->immediate_context, device);
    ID3D11DeviceContext1_Release(&device->immediate_context.ID3D11DeviceContext1_iface);

    wine_rb_init(&device->blend_states, d3d_blend_state_compare);
    wine_rb_init(&device->depthstencil_states, d3d_depthstencil_state_compare);
    wine_rb_init(&device->rasterizer_states, d3d_rasterizer_state_compare);
    wine_rb_init(&device->sampler_states, d3d_sampler_state_compare);
}
