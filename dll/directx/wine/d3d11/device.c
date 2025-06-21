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
#define WINE_NO_NAMELESS_EXTENSION
#include "d3d11_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d11);

static BOOL d3d_array_reserve(void **elements, SIZE_T *capacity, SIZE_T count, SIZE_T size)
{
    SIZE_T max_capacity, new_capacity;
    void *new_elements;

    if (count <= *capacity)
        return TRUE;

    max_capacity = ~(SIZE_T)0 / size;
    if (count > max_capacity)
        return FALSE;

    new_capacity = max(1, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = count;

    if (!(new_elements = realloc(*elements, new_capacity * size)))
        return FALSE;

    *elements = new_elements;
    *capacity = new_capacity;
    return TRUE;
}

static void STDMETHODCALLTYPE d3d_null_wined3d_object_destroyed(void *parent) {}

static const struct wined3d_parent_ops d3d_null_wined3d_parent_ops =
{
    d3d_null_wined3d_object_destroyed,
};

static inline BOOL d3d_device_is_d3d10_active(struct d3d_device *device)
{
    return !device->state
                || IsEqualGUID(&device->state->emulated_interface, &IID_ID3D10Device)
                || IsEqualGUID(&device->state->emulated_interface, &IID_ID3D10Device1);
}

static D3D_FEATURE_LEVEL d3d_feature_level_from_wined3d(enum wined3d_feature_level level)
{
    return (D3D_FEATURE_LEVEL)level;
}

/* ID3DDeviceContextState methods */

static inline struct d3d_device_context_state *impl_from_ID3DDeviceContextState(ID3DDeviceContextState *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_device_context_state, ID3DDeviceContextState_iface);
}

static HRESULT STDMETHODCALLTYPE d3d_device_context_state_QueryInterface(ID3DDeviceContextState *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID3DDeviceContextState)
            || IsEqualGUID(iid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID3DDeviceContextState_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;

    return E_NOINTERFACE;
}

static ULONG d3d_device_context_state_private_addref(struct d3d_device_context_state *state)
{
    ULONG refcount = InterlockedIncrement(&state->private_refcount);

    TRACE("%p increasing private refcount to %lu.\n", state, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d_device_context_state_AddRef(ID3DDeviceContextState *iface)
{
    struct d3d_device_context_state *state = impl_from_ID3DDeviceContextState(iface);
    ULONG refcount = InterlockedIncrement(&state->refcount);

    TRACE("%p increasing refcount to %lu.\n", state, refcount);

    if (refcount == 1)
    {
        d3d_device_context_state_private_addref(state);
        ID3D11Device2_AddRef(state->device);
    }

    return refcount;
}

static void d3d_device_remove_context_state(struct d3d_device *device, struct d3d_device_context_state *state)
{
    unsigned int i;

    for (i = 0; i < device->context_state_count; ++i)
    {
        if (device->context_states[i] != state)
            continue;

        if (i != device->context_state_count - 1)
            device->context_states[i] = device->context_states[device->context_state_count - 1];
        --device->context_state_count;
        break;
    }
}

static void d3d_device_context_state_private_release(struct d3d_device_context_state *state)
{
    ULONG refcount = InterlockedDecrement(&state->private_refcount);
    struct d3d_device_context_state_entry *entry;
    struct d3d_device *device;
    unsigned int i;

    TRACE("%p decreasing private refcount to %lu.\n", state, refcount);

    if (!refcount)
    {
        wined3d_private_store_cleanup(&state->private_store);
        for (i = 0; i < state->entry_count; ++i)
        {
            entry = &state->entries[i];
            device = entry->device;

            if (entry->wined3d_state != wined3d_device_get_state(device->wined3d_device))
                wined3d_state_destroy(entry->wined3d_state);

            d3d_device_remove_context_state(device, state);
        }
        free(state->entries);
        wined3d_device_decref(state->wined3d_device);
        free(state);
    }
}

static ULONG STDMETHODCALLTYPE d3d_device_context_state_Release(ID3DDeviceContextState *iface)
{
    struct d3d_device_context_state *state = impl_from_ID3DDeviceContextState(iface);
    ULONG refcount = InterlockedDecrement(&state->refcount);

    TRACE("%p decreasing refcount to %lu.\n", state, refcount);

    if (!refcount)
    {
        ID3D11Device2_Release(state->device);
        d3d_device_context_state_private_release(state);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d_device_context_state_GetDevice(ID3DDeviceContextState *iface, ID3D11Device **device)
{
    struct d3d_device_context_state *state = impl_from_ID3DDeviceContextState(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (ID3D11Device *)state->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d_device_context_state_GetPrivateData(ID3DDeviceContextState *iface, REFGUID guid,
        UINT *data_size, void *data)
{
    struct d3d_device_context_state *state = impl_from_ID3DDeviceContextState(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d_device_context_state_SetPrivateData(ID3DDeviceContextState *iface, REFGUID guid,
        UINT data_size, const void *data)
{
    struct d3d_device_context_state *state = impl_from_ID3DDeviceContextState(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d_device_context_state_SetPrivateDataInterface(ID3DDeviceContextState *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_device_context_state *state = impl_from_ID3DDeviceContextState(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&state->private_store, guid, data);
}

static const struct ID3DDeviceContextStateVtbl d3d_device_context_state_vtbl =
{
    /* IUnknown methods */
    d3d_device_context_state_QueryInterface,
    d3d_device_context_state_AddRef,
    d3d_device_context_state_Release,
    /* ID3D11DeviceChild methods */
    d3d_device_context_state_GetDevice,
    d3d_device_context_state_GetPrivateData,
    d3d_device_context_state_SetPrivateData,
    d3d_device_context_state_SetPrivateDataInterface,
    /* ID3DDeviceContextState methods */
};

static struct d3d_device_context_state_entry *d3d_device_context_state_get_entry(
        struct d3d_device_context_state *state, struct d3d_device *device)
{
    unsigned int i;

    for (i = 0; i < state->entry_count; ++i)
    {
        if (state->entries[i].device == device)
            return &state->entries[i];
    }

    return NULL;
}

static BOOL d3d_device_context_state_add_entry(struct d3d_device_context_state *state,
        struct d3d_device *device, struct wined3d_state *wined3d_state)
{
    struct d3d_device_context_state_entry *entry;

    if (!d3d_array_reserve((void **)&state->entries, &state->entries_size,
            state->entry_count + 1, sizeof(*state->entries)))
        return FALSE;

    if (!d3d_array_reserve((void **)&device->context_states, &device->context_states_size,
            device->context_state_count + 1, sizeof(*device->context_states)))
        return FALSE;

    entry = &state->entries[state->entry_count++];
    entry->device = device;
    entry->wined3d_state = wined3d_state;

    device->context_states[device->context_state_count++] = state;

    return TRUE;
}

static void d3d_device_context_state_remove_entry(struct d3d_device_context_state *state, struct d3d_device *device)
{
    struct d3d_device_context_state_entry *entry;
    unsigned int i;

    for (i = 0; i < state->entry_count; ++i)
    {
        entry = &state->entries[i];
        if (entry->device != device)
            continue;

        if (entry->wined3d_state != wined3d_device_get_state(device->wined3d_device))
            wined3d_state_destroy(entry->wined3d_state);

        if (i != state->entry_count)
            state->entries[i] = state->entries[state->entry_count - 1];
        --state->entry_count;

        break;
    }
}

static struct wined3d_state *d3d_device_context_state_get_wined3d_state(struct d3d_device_context_state *state,
        struct d3d_device *device)
{
    struct d3d_device_context_state_entry *entry;
    struct wined3d_state *wined3d_state;

    if ((entry = d3d_device_context_state_get_entry(state, device)))
        return entry->wined3d_state;

    if (FAILED(wined3d_state_create(device->wined3d_device,
            (enum wined3d_feature_level *)&state->feature_level, 1, &wined3d_state)))
        return NULL;

    if (!d3d_device_context_state_add_entry(state, device, wined3d_state))
    {
        wined3d_state_destroy(wined3d_state);
        return NULL;
    }

    return wined3d_state;
}

static void d3d_device_context_state_init(struct d3d_device_context_state *state,
        struct d3d_device *device, D3D_FEATURE_LEVEL feature_level, REFIID emulated_interface)
{
    state->ID3DDeviceContextState_iface.lpVtbl = &d3d_device_context_state_vtbl;
    state->refcount = state->private_refcount = 0;

    wined3d_private_store_init(&state->private_store);

    state->feature_level = feature_level;
    state->emulated_interface = *emulated_interface;
    wined3d_device_incref(state->wined3d_device = device->wined3d_device);
    state->device = &device->ID3D11Device2_iface;

    d3d_device_context_state_AddRef(&state->ID3DDeviceContextState_iface);
}

/* ID3D11CommandList methods */

static inline struct d3d11_command_list *impl_from_ID3D11CommandList(ID3D11CommandList *iface)
{
    return CONTAINING_RECORD(iface, struct d3d11_command_list, ID3D11CommandList_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_command_list_QueryInterface(ID3D11CommandList *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID3D11CommandList)
            || IsEqualGUID(iid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID3D11CommandList_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;

    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_command_list_AddRef(ID3D11CommandList *iface)
{
    struct d3d11_command_list *list = impl_from_ID3D11CommandList(iface);
    ULONG refcount = InterlockedIncrement(&list->refcount);

    TRACE("%p increasing refcount to %lu.\n", list, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_command_list_Release(ID3D11CommandList *iface)
{
    struct d3d11_command_list *list = impl_from_ID3D11CommandList(iface);
    ULONG refcount = InterlockedDecrement(&list->refcount);

    TRACE("%p decreasing refcount to %lu.\n", list, refcount);

    if (!refcount)
    {
        wined3d_command_list_decref(list->wined3d_list);
        wined3d_private_store_cleanup(&list->private_store);
        ID3D11Device2_Release(list->device);
        free(list);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_command_list_GetDevice(ID3D11CommandList *iface, ID3D11Device **device)
{
    struct d3d11_command_list *list = impl_from_ID3D11CommandList(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (ID3D11Device *)list->device;
    ID3D11Device2_AddRef(list->device);
}

static HRESULT STDMETHODCALLTYPE d3d11_command_list_GetPrivateData(ID3D11CommandList *iface, REFGUID guid,
        UINT *data_size, void *data)
{
    struct d3d11_command_list *list = impl_from_ID3D11CommandList(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&list->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_command_list_SetPrivateData(ID3D11CommandList *iface, REFGUID guid,
        UINT data_size, const void *data)
{
    struct d3d11_command_list *list = impl_from_ID3D11CommandList(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&list->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_command_list_SetPrivateDataInterface(ID3D11CommandList *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d11_command_list *list = impl_from_ID3D11CommandList(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&list->private_store, guid, data);
}

static UINT STDMETHODCALLTYPE d3d11_command_list_GetContextFlags(ID3D11CommandList *iface)
{
    TRACE("iface %p.\n", iface);

    return 0;
}

static const struct ID3D11CommandListVtbl d3d11_command_list_vtbl =
{
    /* IUnknown methods */
    d3d11_command_list_QueryInterface,
    d3d11_command_list_AddRef,
    d3d11_command_list_Release,
    /* ID3D11DeviceChild methods */
    d3d11_command_list_GetDevice,
    d3d11_command_list_GetPrivateData,
    d3d11_command_list_SetPrivateData,
    d3d11_command_list_SetPrivateDataInterface,
    /* ID3D11CommandList methods */
    d3d11_command_list_GetContextFlags,
};

static struct d3d11_command_list *unsafe_impl_from_ID3D11CommandList(ID3D11CommandList *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d11_command_list_vtbl);
    return impl_from_ID3D11CommandList(iface);
}

static void d3d11_device_context_cleanup(struct d3d11_device_context *context)
{
    wined3d_private_store_cleanup(&context->private_store);
}

/* ID3D11DeviceContext - immediate context methods */

static inline struct d3d11_device_context *impl_from_ID3D11DeviceContext1(ID3D11DeviceContext1 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d11_device_context, ID3D11DeviceContext1_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_device_context_QueryInterface(ID3D11DeviceContext1 *iface,
        REFIID iid, void **out)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID3D11DeviceContext1)
            || IsEqualGUID(iid, &IID_ID3D11DeviceContext)
            || IsEqualGUID(iid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        *out = &context->ID3D11DeviceContext1_iface;
    }
    else if (context->type == D3D11_DEVICE_CONTEXT_IMMEDIATE && IsEqualGUID(iid, &IID_ID3D11Multithread))
    {
        *out = &context->ID3D11Multithread_iface;
    }
    else if (context->type == D3D11_DEVICE_CONTEXT_IMMEDIATE && IsEqualGUID(iid, &IID_ID3D11VideoContext))
    {
        *out = &context->ID3D11VideoContext_iface;
    }
    else if (IsEqualGUID(iid, &IID_ID3DUserDefinedAnnotation))
    {
        *out = &context->ID3DUserDefinedAnnotation_iface;
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

static ULONG STDMETHODCALLTYPE d3d11_device_context_AddRef(ID3D11DeviceContext1 *iface)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    ULONG refcount = InterlockedIncrement(&context->refcount);

    TRACE("%p increasing refcount to %lu.\n", context, refcount);

    if (refcount == 1)
    {
        ID3D11Device2_AddRef(&context->device->ID3D11Device2_iface);
    }

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_device_context_Release(ID3D11DeviceContext1 *iface)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    ULONG refcount = InterlockedDecrement(&context->refcount);

    TRACE("%p decreasing refcount to %lu.\n", context, refcount);

    if (!refcount)
    {
        ID3D11Device2 *device = &context->device->ID3D11Device2_iface;
        if (context->type != D3D11_DEVICE_CONTEXT_IMMEDIATE)
        {
            wined3d_deferred_context_destroy(context->wined3d_context);
            d3d11_device_context_cleanup(context);
            free(context);
        }
        ID3D11Device2_Release(device);
    }

    return refcount;
}

static void d3d11_device_context_get_constant_buffers(ID3D11DeviceContext1 *iface, enum wined3d_shader_type type,
        unsigned int start_slot, unsigned int buffer_count, ID3D11Buffer **buffers,
        unsigned int *offsets, unsigned int *counts)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    unsigned int i;

    wined3d_mutex_lock();
    for (i = 0; i < buffer_count; ++i)
    {
        struct wined3d_constant_buffer_state state;
        struct d3d_buffer *buffer_impl;

        wined3d_device_context_get_constant_buffer(context->wined3d_context, type, start_slot + i, &state);

        if (offsets)
            offsets[i] = state.offset / sizeof(struct wined3d_vec4);
        if (counts)
            counts[i] = state.size / sizeof(struct wined3d_vec4);

        if (!state.buffer)
        {
            buffers[i] = NULL;
            continue;
        }

        buffer_impl = wined3d_buffer_get_parent(state.buffer);
        buffers[i] = &buffer_impl->ID3D11Buffer_iface;
        ID3D11Buffer_AddRef(buffers[i]);
    }
    wined3d_mutex_unlock();
}

static void d3d11_device_context_set_constant_buffers(ID3D11DeviceContext1 *iface, enum wined3d_shader_type type,
        unsigned int start_slot, unsigned int buffer_count, ID3D11Buffer *const *buffers,
        const unsigned int *offsets, const unsigned int *counts)
{
    static const unsigned int alignment = D3D11_COMMONSHADER_CONSTANT_BUFFER_PARTIAL_UPDATE_EXTENTS_BYTE_ALIGNMENT;
    struct wined3d_constant_buffer_state wined3d_buffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    unsigned int i;

    if (buffer_count > ARRAY_SIZE(wined3d_buffers))
    {
        WARN("Buffer count %u exceeds limit; ignoring call.\n", buffer_count);
        return;
    }

    if (!offsets != !counts)
    {
        WARN("Got offsets pointer %p but counts pointer %p; ignoring call.\n", offsets, counts);
        return;
    }

    for (i = 0; i < buffer_count; ++i)
    {
        struct d3d_buffer *buffer = unsafe_impl_from_ID3D11Buffer(buffers[i]);

        if (offsets && (offsets[i] & (alignment - 1)))
        {
            WARN("Offset %u is not aligned.\n", offsets[i]);
            return;
        }

        if (counts && (counts[i] & (alignment - 1)))
        {
            WARN("Count %u is not aligned.\n", counts[i]);
            return;
        }

        wined3d_buffers[i].buffer = buffer ? buffer->wined3d_buffer : NULL;
        wined3d_buffers[i].offset = (offsets ? offsets[i] : 0) * sizeof(struct wined3d_vec4);
        wined3d_buffers[i].size = (counts ? counts[i] : WINED3D_MAX_CONSTANT_BUFFER_SIZE) * sizeof(struct wined3d_vec4);
    }

    wined3d_device_context_set_constant_buffers(context->wined3d_context,
            type, start_slot, buffer_count, wined3d_buffers);
}

static void d3d11_device_context_set_shader_resource_views(ID3D11DeviceContext1 *iface, enum wined3d_shader_type type,
        unsigned int start_slot, unsigned int count, ID3D11ShaderResourceView *const *views)
{
    struct wined3d_shader_resource_view *wined3d_views[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    unsigned int i;

    if (count > ARRAY_SIZE(wined3d_views))
    {
        WARN("View count %u exceeds limit; ignoring call.\n", count);
        return;
    }

    for (i = 0; i < count; ++i)
    {
        struct d3d_shader_resource_view *view = unsafe_impl_from_ID3D11ShaderResourceView(views[i]);

        wined3d_views[i] = view ? view->wined3d_view : NULL;
    }

    wined3d_device_context_set_shader_resource_views(context->wined3d_context,
            type, start_slot, count, wined3d_views);
}

static void d3d11_device_context_set_samplers(ID3D11DeviceContext1 *iface, enum wined3d_shader_type type,
        unsigned int start_slot, unsigned int count, ID3D11SamplerState *const *samplers)
{
    struct wined3d_sampler *wined3d_samplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    unsigned int i;

    if (count > ARRAY_SIZE(wined3d_samplers))
    {
        WARN("Sampler count %u exceeds limit; ignoring call.\n", count);
        return;
    }

    for (i = 0; i < count; ++i)
    {
        struct d3d_sampler_state *sampler = unsafe_impl_from_ID3D11SamplerState(samplers[i]);

        wined3d_samplers[i] = sampler ? sampler->wined3d_sampler : NULL;
    }

    wined3d_device_context_set_samplers(context->wined3d_context, type, start_slot, count, wined3d_samplers);
}

static void STDMETHODCALLTYPE d3d11_device_context_GetDevice(ID3D11DeviceContext1 *iface, ID3D11Device **device)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (ID3D11Device *)&context->device->ID3D11Device2_iface;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_device_context_GetPrivateData(ID3D11DeviceContext1 *iface, REFGUID guid,
        UINT *data_size, void *data)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&context->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_device_context_SetPrivateData(ID3D11DeviceContext1 *iface, REFGUID guid,
        UINT data_size, const void *data)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&context->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_device_context_SetPrivateDataInterface(ID3D11DeviceContext1 *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&context->private_store, guid, data);
}

static void STDMETHODCALLTYPE d3d11_device_context_VSSetConstantBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer *const *buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d11_device_context_set_constant_buffers(iface, WINED3D_SHADER_TYPE_VERTEX, start_slot,
            buffer_count, buffers, NULL, NULL);
}

static void STDMETHODCALLTYPE d3d11_device_context_PSSetShaderResources(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11ShaderResourceView *const *views)
{
    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n",
            iface, start_slot, view_count, views);

    d3d11_device_context_set_shader_resource_views(iface, WINED3D_SHADER_TYPE_PIXEL, start_slot, view_count, views);
}

static void STDMETHODCALLTYPE d3d11_device_context_PSSetShader(ID3D11DeviceContext1 *iface,
        ID3D11PixelShader *shader, ID3D11ClassInstance *const *class_instances, UINT class_instance_count)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d_pixel_shader *ps = unsafe_impl_from_ID3D11PixelShader(shader);

    TRACE("iface %p, shader %p, class_instances %p, class_instance_count %u.\n",
            iface, shader, class_instances, class_instance_count);

    if (class_instance_count)
        FIXME("Dynamic linking is not implemented yet.\n");

    wined3d_device_context_set_shader(context->wined3d_context, WINED3D_SHADER_TYPE_PIXEL,
            ps ? ps->wined3d_shader : NULL);
}

static void STDMETHODCALLTYPE d3d11_device_context_PSSetSamplers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT sampler_count, ID3D11SamplerState *const *samplers)
{
    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    d3d11_device_context_set_samplers(iface, WINED3D_SHADER_TYPE_PIXEL, start_slot, sampler_count, samplers);
}

static void STDMETHODCALLTYPE d3d11_device_context_VSSetShader(ID3D11DeviceContext1 *iface,
        ID3D11VertexShader *shader, ID3D11ClassInstance *const *class_instances, UINT class_instance_count)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d_vertex_shader *vs = unsafe_impl_from_ID3D11VertexShader(shader);

    TRACE("iface %p, shader %p, class_instances %p, class_instance_count %u.\n",
            iface, shader, class_instances, class_instance_count);

    if (class_instance_count)
        FIXME("Dynamic linking is not implemented yet.\n");

    wined3d_device_context_set_shader(context->wined3d_context, WINED3D_SHADER_TYPE_VERTEX,
            vs ? vs->wined3d_shader : NULL);
}

static void STDMETHODCALLTYPE d3d11_device_context_DrawIndexed(ID3D11DeviceContext1 *iface,
        UINT index_count, UINT start_index_location, INT base_vertex_location)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);

    TRACE("iface %p, index_count %u, start_index_location %u, base_vertex_location %d.\n",
            iface, index_count, start_index_location, base_vertex_location);

    wined3d_device_context_draw_indexed(context->wined3d_context,
            base_vertex_location, start_index_location, index_count, 0, 0);
}

static void STDMETHODCALLTYPE d3d11_device_context_Draw(ID3D11DeviceContext1 *iface,
        UINT vertex_count, UINT start_vertex_location)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);

    TRACE("iface %p, vertex_count %u, start_vertex_location %u.\n",
            iface, vertex_count, start_vertex_location);

    wined3d_device_context_draw(context->wined3d_context, start_vertex_location, vertex_count, 0, 0);
}

static HRESULT STDMETHODCALLTYPE d3d11_device_context_Map(ID3D11DeviceContext1 *iface, ID3D11Resource *resource,
        UINT subresource_idx, D3D11_MAP map_type, UINT map_flags, D3D11_MAPPED_SUBRESOURCE *mapped_subresource)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct wined3d_resource *wined3d_resource;
    struct wined3d_map_desc map_desc;
    HRESULT hr;

    TRACE("iface %p, resource %p, subresource_idx %u, map_type %u, map_flags %#x, mapped_subresource %p.\n",
            iface, resource, subresource_idx, map_type, map_flags, mapped_subresource);

    if (map_flags)
        FIXME("Ignoring map_flags %#x.\n", map_flags);

    mapped_subresource->pData = NULL;

    if (context->type != D3D11_DEVICE_CONTEXT_IMMEDIATE
            && map_type != D3D11_MAP_WRITE_DISCARD && map_type != D3D11_MAP_WRITE_NO_OVERWRITE)
        return E_INVALIDARG;

    wined3d_resource = wined3d_resource_from_d3d11_resource(resource);

    if (SUCCEEDED(hr = wined3d_device_context_map(context->wined3d_context, wined3d_resource, subresource_idx,
            &map_desc, NULL, wined3d_map_flags_from_d3d11_map_type(map_type))))
    {
        mapped_subresource->pData = map_desc.data;
        mapped_subresource->RowPitch = map_desc.row_pitch;
        mapped_subresource->DepthPitch = map_desc.slice_pitch;
    }

    return hr;
}

static void STDMETHODCALLTYPE d3d11_device_context_Unmap(ID3D11DeviceContext1 *iface, ID3D11Resource *resource,
        UINT subresource_idx)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct wined3d_resource *wined3d_resource;

    TRACE("iface %p, resource %p, subresource_idx %u.\n", iface, resource, subresource_idx);

    wined3d_resource = wined3d_resource_from_d3d11_resource(resource);

    wined3d_device_context_unmap(context->wined3d_context, wined3d_resource, subresource_idx);
}

static void STDMETHODCALLTYPE d3d11_device_context_PSSetConstantBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer *const *buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d11_device_context_set_constant_buffers(iface, WINED3D_SHADER_TYPE_PIXEL, start_slot,
            buffer_count, buffers, NULL, NULL);
}

static void STDMETHODCALLTYPE d3d11_device_context_IASetInputLayout(ID3D11DeviceContext1 *iface,
        ID3D11InputLayout *input_layout)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d_input_layout *layout = unsafe_impl_from_ID3D11InputLayout(input_layout);

    TRACE("iface %p, input_layout %p.\n", iface, input_layout);

    wined3d_device_context_set_vertex_declaration(context->wined3d_context, layout ? layout->wined3d_decl : NULL);
}

static void STDMETHODCALLTYPE d3d11_device_context_IASetVertexBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer *const *buffers, const UINT *strides, const UINT *offsets)
{
    struct wined3d_stream_state streams[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p, strides %p, offsets %p.\n",
            iface, start_slot, buffer_count, buffers, strides, offsets);

    if (buffer_count > ARRAY_SIZE(streams))
    {
        WARN("Buffer count %u exceeds limit.\n", buffer_count);
        buffer_count = ARRAY_SIZE(streams);
    }

    for (i = 0; i < buffer_count; ++i)
    {
        struct d3d_buffer *buffer = unsafe_impl_from_ID3D11Buffer(buffers[i]);

        streams[i].buffer = buffer ? buffer->wined3d_buffer : NULL;
        streams[i].offset = offsets[i];
        streams[i].stride = strides[i];
        streams[i].frequency = 1;
        streams[i].flags = 0;
    }

    wined3d_device_context_set_stream_sources(context->wined3d_context, start_slot, buffer_count, streams);
}

static void STDMETHODCALLTYPE d3d11_device_context_IASetIndexBuffer(ID3D11DeviceContext1 *iface,
        ID3D11Buffer *buffer, DXGI_FORMAT format, UINT offset)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d_buffer *buffer_impl = unsafe_impl_from_ID3D11Buffer(buffer);

    TRACE("iface %p, buffer %p, format %s, offset %u.\n",
            iface, buffer, debug_dxgi_format(format), offset);

    wined3d_device_context_set_index_buffer(context->wined3d_context,
            buffer_impl ? buffer_impl->wined3d_buffer : NULL,
            wined3dformat_from_dxgi_format(format), offset);
}

static void STDMETHODCALLTYPE d3d11_device_context_DrawIndexedInstanced(ID3D11DeviceContext1 *iface,
        UINT instance_index_count, UINT instance_count, UINT start_index_location, INT base_vertex_location,
        UINT start_instance_location)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);

    TRACE("iface %p, instance_index_count %u, instance_count %u, start_index_location %u, "
            "base_vertex_location %d, start_instance_location %u.\n",
            iface, instance_index_count, instance_count, start_index_location,
            base_vertex_location, start_instance_location);

    wined3d_device_context_draw_indexed(context->wined3d_context, base_vertex_location,
            start_index_location, instance_index_count, start_instance_location, instance_count);
}

static void STDMETHODCALLTYPE d3d11_device_context_DrawInstanced(ID3D11DeviceContext1 *iface,
        UINT instance_vertex_count, UINT instance_count, UINT start_vertex_location, UINT start_instance_location)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);

    TRACE("iface %p, instance_vertex_count %u, instance_count %u, start_vertex_location %u, "
            "start_instance_location %u.\n",
            iface, instance_vertex_count, instance_count, start_vertex_location,
            start_instance_location);

    wined3d_device_context_draw(context->wined3d_context, start_vertex_location,
            instance_vertex_count, start_instance_location, instance_count);
}

static void STDMETHODCALLTYPE d3d11_device_context_GSSetConstantBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer *const *buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d11_device_context_set_constant_buffers(iface, WINED3D_SHADER_TYPE_GEOMETRY, start_slot,
            buffer_count, buffers, NULL, NULL);
}

static void STDMETHODCALLTYPE d3d11_device_context_GSSetShader(ID3D11DeviceContext1 *iface,
        ID3D11GeometryShader *shader, ID3D11ClassInstance *const *class_instances, UINT class_instance_count)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d_geometry_shader *gs = unsafe_impl_from_ID3D11GeometryShader(shader);

    TRACE("iface %p, shader %p, class_instances %p, class_instance_count %u.\n",
            iface, shader, class_instances, class_instance_count);

    if (class_instance_count)
        FIXME("Dynamic linking is not implemented yet.\n");

    wined3d_device_context_set_shader(context->wined3d_context, WINED3D_SHADER_TYPE_GEOMETRY,
            gs ? gs->wined3d_shader : NULL);
}

static void STDMETHODCALLTYPE d3d11_device_context_IASetPrimitiveTopology(ID3D11DeviceContext1 *iface,
        D3D11_PRIMITIVE_TOPOLOGY topology)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    enum wined3d_primitive_type primitive_type;
    unsigned int patch_vertex_count;

    TRACE("iface %p, topology %#x.\n", iface, topology);

    wined3d_primitive_type_from_d3d11_primitive_topology(topology, &primitive_type, &patch_vertex_count);

    wined3d_device_context_set_primitive_type(context->wined3d_context, primitive_type, patch_vertex_count);
}

static void STDMETHODCALLTYPE d3d11_device_context_VSSetShaderResources(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11ShaderResourceView *const *views)
{
    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n", iface, start_slot, view_count, views);

    d3d11_device_context_set_shader_resource_views(iface, WINED3D_SHADER_TYPE_VERTEX, start_slot, view_count, views);
}

static void STDMETHODCALLTYPE d3d11_device_context_VSSetSamplers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT sampler_count, ID3D11SamplerState *const *samplers)
{
    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    d3d11_device_context_set_samplers(iface, WINED3D_SHADER_TYPE_VERTEX, start_slot, sampler_count, samplers);
}

static void STDMETHODCALLTYPE d3d11_device_context_Begin(ID3D11DeviceContext1 *iface,
        ID3D11Asynchronous *asynchronous)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d_query *query = unsafe_impl_from_ID3D11Asynchronous(asynchronous);

    TRACE("iface %p, asynchronous %p.\n", iface, asynchronous);

    wined3d_device_context_issue_query(context->wined3d_context, query->wined3d_query, WINED3DISSUE_BEGIN);
}

static void STDMETHODCALLTYPE d3d11_device_context_End(ID3D11DeviceContext1 *iface,
        ID3D11Asynchronous *asynchronous)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d_query *query = unsafe_impl_from_ID3D11Asynchronous(asynchronous);

    TRACE("iface %p, asynchronous %p.\n", iface, asynchronous);

    wined3d_device_context_issue_query(context->wined3d_context, query->wined3d_query, WINED3DISSUE_END);
}

static HRESULT STDMETHODCALLTYPE d3d11_device_context_GetData(ID3D11DeviceContext1 *iface,
        ID3D11Asynchronous *asynchronous, void *data, UINT data_size, UINT data_flags)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d_query *query = unsafe_impl_from_ID3D11Asynchronous(asynchronous);
    unsigned int wined3d_flags;
    HRESULT hr;

    TRACE("iface %p, asynchronous %p, data %p, data_size %u, data_flags %#x.\n",
            iface, asynchronous, data, data_size, data_flags);

    if (context->type != D3D11_DEVICE_CONTEXT_IMMEDIATE)
        return DXGI_ERROR_INVALID_CALL;

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

static void STDMETHODCALLTYPE d3d11_device_context_SetPredication(ID3D11DeviceContext1 *iface,
        ID3D11Predicate *predicate, BOOL value)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d_query *query;

    TRACE("iface %p, predicate %p, value %#x.\n", iface, predicate, value);

    query = unsafe_impl_from_ID3D11Query((ID3D11Query *)predicate);

    wined3d_device_context_set_predication(context->wined3d_context, query ? query->wined3d_query : NULL, value);
}

static void STDMETHODCALLTYPE d3d11_device_context_GSSetShaderResources(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11ShaderResourceView *const *views)
{
    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n", iface, start_slot, view_count, views);

    d3d11_device_context_set_shader_resource_views(iface, WINED3D_SHADER_TYPE_GEOMETRY, start_slot, view_count, views);
}

static void STDMETHODCALLTYPE d3d11_device_context_GSSetSamplers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT sampler_count, ID3D11SamplerState *const *samplers)
{
    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    d3d11_device_context_set_samplers(iface, WINED3D_SHADER_TYPE_GEOMETRY, start_slot, sampler_count, samplers);
}

static void STDMETHODCALLTYPE d3d11_device_context_OMSetRenderTargets(ID3D11DeviceContext1 *iface,
        UINT rtv_count, ID3D11RenderTargetView *const *rtvs, ID3D11DepthStencilView *depth_stencil_view)
{
    struct wined3d_rendertarget_view *wined3d_rtvs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {0};
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d_depthstencil_view *dsv;
    unsigned int i;

    TRACE("iface %p, rtv_count %u, rtvs %p, depth_stencil_view %p.\n", iface, rtv_count, rtvs, depth_stencil_view);

    if (rtv_count > ARRAY_SIZE(wined3d_rtvs))
    {
        WARN("View count %u exceeds limit.\n", rtv_count);
        rtv_count = ARRAY_SIZE(wined3d_rtvs);
    }

    for (i = 0; i < rtv_count; ++i)
    {
        struct d3d_rendertarget_view *rtv = unsafe_impl_from_ID3D11RenderTargetView(rtvs[i]);

        wined3d_rtvs[i] = rtv ? rtv->wined3d_view : NULL;
    }

    dsv = unsafe_impl_from_ID3D11DepthStencilView(depth_stencil_view);

    wined3d_device_context_set_render_targets_and_unordered_access_views(context->wined3d_context,
            ARRAY_SIZE(wined3d_rtvs), wined3d_rtvs, dsv ? dsv->wined3d_view : NULL, ~0u, NULL, NULL);
}

static void STDMETHODCALLTYPE d3d11_device_context_OMSetRenderTargetsAndUnorderedAccessViews(
        ID3D11DeviceContext1 *iface, UINT render_target_view_count, ID3D11RenderTargetView *const *render_target_views,
        ID3D11DepthStencilView *depth_stencil_view, UINT uav_start_idx, UINT uav_count,
        ID3D11UnorderedAccessView *const *uavs, const UINT *initial_counts)
{
    struct d3d_depthstencil_view *dsv = unsafe_impl_from_ID3D11DepthStencilView(depth_stencil_view);
    struct wined3d_rendertarget_view *wined3d_rtvs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {0};
    struct wined3d_unordered_access_view *wined3d_uavs[D3D11_PS_CS_UAV_REGISTER_COUNT] = {0};
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    unsigned int wined3d_initial_counts[D3D11_PS_CS_UAV_REGISTER_COUNT];
    unsigned int i;

    TRACE("iface %p, render_target_view_count %u, render_target_views %p, depth_stencil_view %p, "
            "uav_start_idx %u, uav_count %u, uavs %p, initial_counts %p.\n",
            iface, render_target_view_count, render_target_views, depth_stencil_view,
            uav_start_idx, uav_count, uavs, initial_counts);

    if (render_target_view_count == D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL)
        render_target_view_count = ~0u;
    else
    {
        if (render_target_view_count > ARRAY_SIZE(wined3d_rtvs))
        {
            WARN("View count %u exceeds limit.\n", render_target_view_count);
            render_target_view_count = ARRAY_SIZE(wined3d_rtvs);
        }

        for (i = 0; i < render_target_view_count; ++i)
        {
            struct d3d_rendertarget_view *rtv = unsafe_impl_from_ID3D11RenderTargetView(render_target_views[i]);

            wined3d_rtvs[i] = rtv ? rtv->wined3d_view : NULL;
        }
    }

    if (uav_count == D3D11_KEEP_UNORDERED_ACCESS_VIEWS)
        uav_count = ~0u;
    else
    {
        if (!wined3d_bound_range(uav_start_idx, uav_count, ARRAY_SIZE(wined3d_uavs)))
        {
            WARN("View count %u exceeds limit; ignoring call.\n", uav_count);
            return;
        }

        memset(wined3d_initial_counts, 0xff, sizeof(wined3d_initial_counts));

        for (i = 0; i < uav_count; ++i)
        {
            struct d3d11_unordered_access_view *view =
                    unsafe_impl_from_ID3D11UnorderedAccessView(uavs[i]);

            wined3d_uavs[uav_start_idx + i] = view ? view->wined3d_view : NULL;
            wined3d_initial_counts[uav_start_idx + i] = initial_counts ? initial_counts[i] : ~0u;
        }
    }

    wined3d_device_context_set_render_targets_and_unordered_access_views(context->wined3d_context,
            render_target_view_count == ~0u ? ~0u : ARRAY_SIZE(wined3d_rtvs), wined3d_rtvs,
            dsv ? dsv->wined3d_view : NULL, uav_count == ~0u ? ~0u : ARRAY_SIZE(wined3d_uavs), wined3d_uavs,
            wined3d_initial_counts);
}

static void STDMETHODCALLTYPE d3d11_device_context_OMSetBlendState(ID3D11DeviceContext1 *iface,
        ID3D11BlendState *blend_state, const float blend_factor[4], UINT sample_mask)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    static const float default_blend_factor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    struct d3d_blend_state *blend_state_impl;

    TRACE("iface %p, blend_state %p, blend_factor %s, sample_mask 0x%08x.\n",
            iface, blend_state, debug_float4(blend_factor), sample_mask);

    if (!blend_factor)
        blend_factor = default_blend_factor;

    if (!(blend_state_impl = unsafe_impl_from_ID3D11BlendState(blend_state)))
        wined3d_device_context_set_blend_state(context->wined3d_context, NULL,
                (const struct wined3d_color *)blend_factor, sample_mask);
    else
        wined3d_device_context_set_blend_state(context->wined3d_context, blend_state_impl->wined3d_state,
                (const struct wined3d_color *)blend_factor, sample_mask);
}

static void STDMETHODCALLTYPE d3d11_device_context_OMSetDepthStencilState(ID3D11DeviceContext1 *iface,
        ID3D11DepthStencilState *depth_stencil_state, UINT stencil_ref)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d_depthstencil_state *state_impl;

    TRACE("iface %p, depth_stencil_state %p, stencil_ref %u.\n",
            iface, depth_stencil_state, stencil_ref);

    if (!(state_impl = unsafe_impl_from_ID3D11DepthStencilState(depth_stencil_state)))
    {
        wined3d_device_context_set_depth_stencil_state(context->wined3d_context, NULL, stencil_ref);
        return;
    }

    wined3d_device_context_set_depth_stencil_state(context->wined3d_context, state_impl->wined3d_state, stencil_ref);
}

static void STDMETHODCALLTYPE d3d11_device_context_SOSetTargets(ID3D11DeviceContext1 *iface, UINT buffer_count,
        ID3D11Buffer *const *buffers, const UINT *offsets)
{
    struct wined3d_stream_output outputs[WINED3D_MAX_STREAM_OUTPUT_BUFFERS] = {0};
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    unsigned int count, i;

    TRACE("iface %p, buffer_count %u, buffers %p, offsets %p.\n", iface, buffer_count, buffers, offsets);

    count = min(buffer_count, ARRAY_SIZE(outputs));
    for (i = 0; i < count; ++i)
    {
        struct d3d_buffer *buffer = unsafe_impl_from_ID3D11Buffer(buffers[i]);

        outputs[i].buffer = buffer ? buffer->wined3d_buffer : NULL;
        outputs[i].offset = offsets ? offsets[i] : 0;
    }

    wined3d_device_context_set_stream_outputs(context->wined3d_context, outputs);
}

static void STDMETHODCALLTYPE d3d11_device_context_DrawAuto(ID3D11DeviceContext1 *iface)
{
    FIXME("iface %p stub!\n", iface);
}

static void STDMETHODCALLTYPE d3d11_device_context_DrawIndexedInstancedIndirect(ID3D11DeviceContext1 *iface,
        ID3D11Buffer *buffer, UINT offset)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d_buffer *d3d_buffer;

    TRACE("iface %p, buffer %p, offset %u.\n", iface, buffer, offset);

    d3d_buffer = unsafe_impl_from_ID3D11Buffer(buffer);

    wined3d_device_context_draw_indirect(context->wined3d_context, d3d_buffer->wined3d_buffer, offset, true);
}

static void STDMETHODCALLTYPE d3d11_device_context_DrawInstancedIndirect(ID3D11DeviceContext1 *iface,
        ID3D11Buffer *buffer, UINT offset)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d_buffer *d3d_buffer;

    TRACE("iface %p, buffer %p, offset %u.\n", iface, buffer, offset);

    d3d_buffer = unsafe_impl_from_ID3D11Buffer(buffer);

    wined3d_device_context_draw_indirect(context->wined3d_context, d3d_buffer->wined3d_buffer, offset, false);
}

static void STDMETHODCALLTYPE d3d11_device_context_Dispatch(ID3D11DeviceContext1 *iface,
        UINT thread_group_count_x, UINT thread_group_count_y, UINT thread_group_count_z)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);

    TRACE("iface %p, thread_group_count_x %u, thread_group_count_y %u, thread_group_count_z %u.\n",
            iface, thread_group_count_x, thread_group_count_y, thread_group_count_z);

    wined3d_device_context_dispatch(context->wined3d_context,
            thread_group_count_x, thread_group_count_y, thread_group_count_z);
}

static void STDMETHODCALLTYPE d3d11_device_context_DispatchIndirect(ID3D11DeviceContext1 *iface,
        ID3D11Buffer *buffer, UINT offset)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d_buffer *buffer_impl;

    TRACE("iface %p, buffer %p, offset %u.\n", iface, buffer, offset);

    buffer_impl = unsafe_impl_from_ID3D11Buffer(buffer);

    wined3d_device_context_dispatch_indirect(context->wined3d_context, buffer_impl->wined3d_buffer, offset);
}

static void STDMETHODCALLTYPE d3d11_device_context_RSSetState(ID3D11DeviceContext1 *iface,
        ID3D11RasterizerState *rasterizer_state)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d_rasterizer_state *rasterizer_state_impl;

    TRACE("iface %p, rasterizer_state %p.\n", iface, rasterizer_state);

    rasterizer_state_impl = unsafe_impl_from_ID3D11RasterizerState(rasterizer_state);
    wined3d_device_context_set_rasterizer_state(context->wined3d_context,
            rasterizer_state_impl ? rasterizer_state_impl->wined3d_state : NULL);
}

static void STDMETHODCALLTYPE d3d11_device_context_RSSetViewports(ID3D11DeviceContext1 *iface,
        UINT viewport_count, const D3D11_VIEWPORT *viewports)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
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

    wined3d_device_context_set_viewports(context->wined3d_context, viewport_count, wined3d_vp);
}

static void STDMETHODCALLTYPE d3d11_device_context_RSSetScissorRects(ID3D11DeviceContext1 *iface,
        UINT rect_count, const D3D11_RECT *rects)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);

    TRACE("iface %p, rect_count %u, rects %p.\n", iface, rect_count, rects);

    if (rect_count > WINED3D_MAX_VIEWPORTS)
        return;

    wined3d_device_context_set_scissor_rects(context->wined3d_context, rect_count, rects);
}

static void STDMETHODCALLTYPE d3d11_device_context_CopySubresourceRegion(ID3D11DeviceContext1 *iface,
        ID3D11Resource *dst_resource, UINT dst_subresource_idx, UINT dst_x, UINT dst_y, UINT dst_z,
        ID3D11Resource *src_resource, UINT src_subresource_idx, const D3D11_BOX *src_box)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct wined3d_resource *wined3d_dst_resource, *wined3d_src_resource;
    struct wined3d_box wined3d_src_box;

    TRACE("iface %p, dst_resource %p, dst_subresource_idx %u, dst_x %u, dst_y %u, dst_z %u, "
            "src_resource %p, src_subresource_idx %u, src_box %p.\n",
            iface, dst_resource, dst_subresource_idx, dst_x, dst_y, dst_z,
            src_resource, src_subresource_idx, src_box);

    if (!dst_resource || !src_resource)
        return;

    if (src_box)
        wined3d_box_set(&wined3d_src_box, src_box->left, src_box->top,
                src_box->right, src_box->bottom, src_box->front, src_box->back);

    wined3d_dst_resource = wined3d_resource_from_d3d11_resource(dst_resource);
    wined3d_src_resource = wined3d_resource_from_d3d11_resource(src_resource);
    wined3d_device_context_copy_sub_resource_region(context->wined3d_context, wined3d_dst_resource, dst_subresource_idx,
            dst_x, dst_y, dst_z, wined3d_src_resource, src_subresource_idx, src_box ? &wined3d_src_box : NULL, 0);
}

static void STDMETHODCALLTYPE d3d11_device_context_CopyResource(ID3D11DeviceContext1 *iface,
        ID3D11Resource *dst_resource, ID3D11Resource *src_resource)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct wined3d_resource *wined3d_dst_resource, *wined3d_src_resource;

    TRACE("iface %p, dst_resource %p, src_resource %p.\n", iface, dst_resource, src_resource);

    wined3d_dst_resource = wined3d_resource_from_d3d11_resource(dst_resource);
    wined3d_src_resource = wined3d_resource_from_d3d11_resource(src_resource);
    wined3d_device_context_copy_resource(context->wined3d_context, wined3d_dst_resource, wined3d_src_resource);
}

static void STDMETHODCALLTYPE d3d11_device_context_UpdateSubresource(ID3D11DeviceContext1 *iface,
        ID3D11Resource *resource, UINT subresource_idx, const D3D11_BOX *box,
        const void *data, UINT row_pitch, UINT depth_pitch)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct wined3d_resource *wined3d_resource;
    struct wined3d_box wined3d_box;

    TRACE("iface %p, resource %p, subresource_idx %u, box %p, data %p, row_pitch %u, depth_pitch %u.\n",
            iface, resource, subresource_idx, box, data, row_pitch, depth_pitch);

    if (box)
        wined3d_box_set(&wined3d_box, box->left, box->top, box->right, box->bottom, box->front, box->back);

    wined3d_resource = wined3d_resource_from_d3d11_resource(resource);
    wined3d_device_context_update_sub_resource(context->wined3d_context, wined3d_resource,
            subresource_idx, box ? &wined3d_box : NULL, data, row_pitch, depth_pitch, 0);
}

static void STDMETHODCALLTYPE d3d11_device_context_CopyStructureCount(ID3D11DeviceContext1 *iface,
        ID3D11Buffer *dst_buffer, UINT dst_offset, ID3D11UnorderedAccessView *src_view)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d11_unordered_access_view *uav;
    struct d3d_buffer *buffer_impl;

    TRACE("iface %p, dst_buffer %p, dst_offset %u, src_view %p.\n",
            iface, dst_buffer, dst_offset, src_view);

    buffer_impl = unsafe_impl_from_ID3D11Buffer(dst_buffer);
    uav = unsafe_impl_from_ID3D11UnorderedAccessView(src_view);

    wined3d_device_context_copy_uav_counter(context->wined3d_context,
            buffer_impl->wined3d_buffer, dst_offset, uav->wined3d_view);
}

static void STDMETHODCALLTYPE d3d11_device_context_ClearRenderTargetView(ID3D11DeviceContext1 *iface,
        ID3D11RenderTargetView *render_target_view, const float color_rgba[4])
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d_rendertarget_view *view = unsafe_impl_from_ID3D11RenderTargetView(render_target_view);
    const struct wined3d_color color = {color_rgba[0], color_rgba[1], color_rgba[2], color_rgba[3]};
    HRESULT hr;

    TRACE("iface %p, render_target_view %p, color_rgba %s.\n",
            iface, render_target_view, debug_float4(color_rgba));

    if (!view)
        return;

    if (FAILED(hr = wined3d_device_context_clear_rendertarget_view(context->wined3d_context, view->wined3d_view, NULL,
            WINED3DCLEAR_TARGET, &color, 0.0f, 0)))
        ERR("Failed to clear view, hr %#lx.\n", hr);
}

static void STDMETHODCALLTYPE d3d11_device_context_ClearUnorderedAccessViewUint(ID3D11DeviceContext1 *iface,
        ID3D11UnorderedAccessView *unordered_access_view, const UINT values[4])
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d11_unordered_access_view *view;

    TRACE("iface %p, unordered_access_view %p, values {%u, %u, %u, %u}.\n",
            iface, unordered_access_view, values[0], values[1], values[2], values[3]);

    view = unsafe_impl_from_ID3D11UnorderedAccessView(unordered_access_view);
    wined3d_device_context_clear_uav_uint(context->wined3d_context,
            view->wined3d_view, (const struct wined3d_uvec4 *)values);
}

static void STDMETHODCALLTYPE d3d11_device_context_ClearUnorderedAccessViewFloat(ID3D11DeviceContext1 *iface,
        ID3D11UnorderedAccessView *unordered_access_view, const float values[4])
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d11_unordered_access_view *view;

    TRACE("iface %p, unordered_access_view %p, values %s.\n",
            iface, unordered_access_view, debug_float4(values));

    view = unsafe_impl_from_ID3D11UnorderedAccessView(unordered_access_view);
    wined3d_device_context_clear_uav_float(context->wined3d_context,
            view->wined3d_view, (const struct wined3d_vec4 *)values);
}

static void STDMETHODCALLTYPE d3d11_device_context_ClearDepthStencilView(ID3D11DeviceContext1 *iface,
        ID3D11DepthStencilView *depth_stencil_view, UINT flags, FLOAT depth, UINT8 stencil)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d_depthstencil_view *view = unsafe_impl_from_ID3D11DepthStencilView(depth_stencil_view);
    DWORD wined3d_flags;
    HRESULT hr;

    TRACE("iface %p, depth_stencil_view %p, flags %#x, depth %.8e, stencil %u.\n",
            iface, depth_stencil_view, flags, depth, stencil);

    if (!view)
        return;

    wined3d_flags = wined3d_clear_flags_from_d3d11_clear_flags(flags);

    if (FAILED(hr = wined3d_device_context_clear_rendertarget_view(context->wined3d_context, view->wined3d_view, NULL,
            wined3d_flags, NULL, depth, stencil)))
        ERR("Failed to clear view, hr %#lx.\n", hr);
}

static void STDMETHODCALLTYPE d3d11_device_context_GenerateMips(ID3D11DeviceContext1 *iface,
        ID3D11ShaderResourceView *view)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d_shader_resource_view *srv = unsafe_impl_from_ID3D11ShaderResourceView(view);

    TRACE("iface %p, view %p.\n", iface, view);

    wined3d_device_context_generate_mipmaps(context->wined3d_context, srv->wined3d_view);
}

static void STDMETHODCALLTYPE d3d11_device_context_SetResourceMinLOD(ID3D11DeviceContext1 *iface,
        ID3D11Resource *resource, FLOAT min_lod)
{
    FIXME("iface %p, resource %p, min_lod %f stub!\n", iface, resource, min_lod);
}

static FLOAT STDMETHODCALLTYPE d3d11_device_context_GetResourceMinLOD(ID3D11DeviceContext1 *iface,
        ID3D11Resource *resource)
{
    FIXME("iface %p, resource %p stub!\n", iface, resource);

    return 0.0f;
}

static void STDMETHODCALLTYPE d3d11_device_context_ResolveSubresource(ID3D11DeviceContext1 *iface,
        ID3D11Resource *dst_resource, UINT dst_subresource_idx,
        ID3D11Resource *src_resource, UINT src_subresource_idx,
        DXGI_FORMAT format)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct wined3d_resource *wined3d_dst_resource, *wined3d_src_resource;
    enum wined3d_format_id wined3d_format;

    TRACE("iface %p, dst_resource %p, dst_subresource_idx %u, "
            "src_resource %p, src_subresource_idx %u, format %s.\n",
            iface, dst_resource, dst_subresource_idx,
            src_resource, src_subresource_idx, debug_dxgi_format(format));

    wined3d_dst_resource = wined3d_resource_from_d3d11_resource(dst_resource);
    wined3d_src_resource = wined3d_resource_from_d3d11_resource(src_resource);
    wined3d_format = wined3dformat_from_dxgi_format(format);
    wined3d_device_context_resolve_sub_resource(context->wined3d_context,
            wined3d_dst_resource, dst_subresource_idx,
            wined3d_src_resource, src_subresource_idx, wined3d_format);
}

static void STDMETHODCALLTYPE d3d11_device_context_ExecuteCommandList(ID3D11DeviceContext1 *iface,
        ID3D11CommandList *command_list, BOOL restore_state)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d11_command_list *list_impl = unsafe_impl_from_ID3D11CommandList(command_list);

    TRACE("iface %p, command_list %p, restore_state %#x.\n", iface, command_list, restore_state);

    wined3d_device_context_execute_command_list(context->wined3d_context, list_impl->wined3d_list, !!restore_state);
}

static void STDMETHODCALLTYPE d3d11_device_context_HSSetShaderResources(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11ShaderResourceView *const *views)
{
    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n",
            iface, start_slot, view_count, views);

    d3d11_device_context_set_shader_resource_views(iface, WINED3D_SHADER_TYPE_HULL, start_slot, view_count, views);
}

static void STDMETHODCALLTYPE d3d11_device_context_HSSetShader(ID3D11DeviceContext1 *iface,
        ID3D11HullShader *shader, ID3D11ClassInstance *const *class_instances, UINT class_instance_count)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d11_hull_shader *hs = unsafe_impl_from_ID3D11HullShader(shader);

    TRACE("iface %p, shader %p, class_instances %p, class_instance_count %u.\n",
            iface, shader, class_instances, class_instance_count);

    if (class_instance_count)
        FIXME("Dynamic linking is not implemented yet.\n");

    wined3d_device_context_set_shader(context->wined3d_context, WINED3D_SHADER_TYPE_HULL,
            hs ? hs->wined3d_shader : NULL);
}

static void STDMETHODCALLTYPE d3d11_device_context_HSSetSamplers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT sampler_count, ID3D11SamplerState *const *samplers)
{
    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    d3d11_device_context_set_samplers(iface, WINED3D_SHADER_TYPE_HULL, start_slot, sampler_count, samplers);
}

static void STDMETHODCALLTYPE d3d11_device_context_HSSetConstantBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer *const *buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d11_device_context_set_constant_buffers(iface, WINED3D_SHADER_TYPE_HULL, start_slot,
            buffer_count, buffers, NULL, NULL);
}

static void STDMETHODCALLTYPE d3d11_device_context_DSSetShaderResources(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11ShaderResourceView *const *views)
{
    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n",
            iface, start_slot, view_count, views);

    d3d11_device_context_set_shader_resource_views(iface, WINED3D_SHADER_TYPE_DOMAIN, start_slot, view_count, views);
}

static void STDMETHODCALLTYPE d3d11_device_context_DSSetShader(ID3D11DeviceContext1 *iface,
        ID3D11DomainShader *shader, ID3D11ClassInstance *const *class_instances, UINT class_instance_count)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d11_domain_shader *ds = unsafe_impl_from_ID3D11DomainShader(shader);

    TRACE("iface %p, shader %p, class_instances %p, class_instance_count %u.\n",
            iface, shader, class_instances, class_instance_count);

    if (class_instance_count)
        FIXME("Dynamic linking is not implemented yet.\n");

    wined3d_device_context_set_shader(context->wined3d_context, WINED3D_SHADER_TYPE_DOMAIN,
            ds ? ds->wined3d_shader : NULL);
}

static void STDMETHODCALLTYPE d3d11_device_context_DSSetSamplers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT sampler_count, ID3D11SamplerState *const *samplers)
{
    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    d3d11_device_context_set_samplers(iface, WINED3D_SHADER_TYPE_DOMAIN, start_slot, sampler_count, samplers);
}

static void STDMETHODCALLTYPE d3d11_device_context_DSSetConstantBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer *const *buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d11_device_context_set_constant_buffers(iface, WINED3D_SHADER_TYPE_DOMAIN, start_slot,
            buffer_count, buffers, NULL, NULL);
}

static void STDMETHODCALLTYPE d3d11_device_context_CSSetShaderResources(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11ShaderResourceView *const *views)
{
    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n",
            iface, start_slot, view_count, views);

    d3d11_device_context_set_shader_resource_views(iface, WINED3D_SHADER_TYPE_COMPUTE, start_slot, view_count, views);
}

static void STDMETHODCALLTYPE d3d11_device_context_CSSetUnorderedAccessViews(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11UnorderedAccessView *const *views, const UINT *initial_counts)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct wined3d_unordered_access_view *wined3d_views[D3D11_PS_CS_UAV_REGISTER_COUNT];
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p, initial_counts %p.\n",
            iface, start_slot, view_count, views, initial_counts);

    if (view_count > ARRAY_SIZE(wined3d_views))
    {
        WARN("View count %u exceeds limit; ignoring call.\n", view_count);
        return;
    }

    for (i = 0; i < view_count; ++i)
    {
        struct d3d11_unordered_access_view *view = unsafe_impl_from_ID3D11UnorderedAccessView(views[i]);

        wined3d_views[i] = view ? view->wined3d_view : NULL;
    }

    wined3d_device_context_set_unordered_access_views(context->wined3d_context, WINED3D_PIPELINE_COMPUTE,
            start_slot, view_count, wined3d_views, initial_counts);
}

static void STDMETHODCALLTYPE d3d11_device_context_CSSetShader(ID3D11DeviceContext1 *iface,
        ID3D11ComputeShader *shader, ID3D11ClassInstance *const *class_instances, UINT class_instance_count)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d11_compute_shader *cs = unsafe_impl_from_ID3D11ComputeShader(shader);

    TRACE("iface %p, shader %p, class_instances %p, class_instance_count %u.\n",
            iface, shader, class_instances, class_instance_count);

    if (class_instance_count)
        FIXME("Dynamic linking is not implemented yet.\n");

    wined3d_device_context_set_shader(context->wined3d_context, WINED3D_SHADER_TYPE_COMPUTE,
            cs ? cs->wined3d_shader : NULL);
}

static void STDMETHODCALLTYPE d3d11_device_context_CSSetSamplers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT sampler_count, ID3D11SamplerState *const *samplers)
{
    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    d3d11_device_context_set_samplers(iface, WINED3D_SHADER_TYPE_COMPUTE, start_slot, sampler_count, samplers);
}

static void STDMETHODCALLTYPE d3d11_device_context_CSSetConstantBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer *const *buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d11_device_context_set_constant_buffers(iface, WINED3D_SHADER_TYPE_COMPUTE, start_slot,
            buffer_count, buffers, NULL, NULL);
}

static void STDMETHODCALLTYPE d3d11_device_context_VSGetConstantBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d11_device_context_get_constant_buffers(iface, WINED3D_SHADER_TYPE_VERTEX, start_slot,
            buffer_count, buffers, NULL, NULL);
}

static void STDMETHODCALLTYPE d3d11_device_context_PSGetShaderResources(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11ShaderResourceView **views)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n",
            iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct wined3d_shader_resource_view *wined3d_view;
        struct d3d_shader_resource_view *view_impl;

        if (!(wined3d_view = wined3d_device_context_get_shader_resource_view(
                context->wined3d_context, WINED3D_SHADER_TYPE_PIXEL, start_slot + i)))
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

static void STDMETHODCALLTYPE d3d11_device_context_PSGetShader(ID3D11DeviceContext1 *iface,
        ID3D11PixelShader **shader, ID3D11ClassInstance **class_instances, UINT *class_instance_count)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct wined3d_shader *wined3d_shader;
    struct d3d_pixel_shader *shader_impl;

    TRACE("iface %p, shader %p, class_instances %p, class_instance_count %p.\n",
            iface, shader, class_instances, class_instance_count);

    if (class_instance_count)
        *class_instance_count = 0;

    wined3d_mutex_lock();
    if (!(wined3d_shader = wined3d_device_context_get_shader(context->wined3d_context, WINED3D_SHADER_TYPE_PIXEL)))
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

static void STDMETHODCALLTYPE d3d11_device_context_PSGetSamplers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT sampler_count, ID3D11SamplerState **samplers)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    wined3d_mutex_lock();
    for (i = 0; i < sampler_count; ++i)
    {
        struct wined3d_sampler *wined3d_sampler;
        struct d3d_sampler_state *sampler_impl;

        if (!(wined3d_sampler = wined3d_device_context_get_sampler(
                context->wined3d_context, WINED3D_SHADER_TYPE_PIXEL, start_slot + i)))
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

static void STDMETHODCALLTYPE d3d11_device_context_VSGetShader(ID3D11DeviceContext1 *iface,
        ID3D11VertexShader **shader, ID3D11ClassInstance **class_instances, UINT *class_instance_count)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d_vertex_shader *shader_impl;
    struct wined3d_shader *wined3d_shader;

    TRACE("iface %p, shader %p, class_instances %p, class_instance_count %p.\n",
            iface, shader, class_instances, class_instance_count);

    if (class_instance_count)
        *class_instance_count = 0;

    wined3d_mutex_lock();
    if (!(wined3d_shader = wined3d_device_context_get_shader(context->wined3d_context, WINED3D_SHADER_TYPE_VERTEX)))
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

static void STDMETHODCALLTYPE d3d11_device_context_PSGetConstantBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d11_device_context_get_constant_buffers(iface, WINED3D_SHADER_TYPE_PIXEL, start_slot,
            buffer_count, buffers, NULL, NULL);
}

static void STDMETHODCALLTYPE d3d11_device_context_IAGetInputLayout(ID3D11DeviceContext1 *iface,
        ID3D11InputLayout **input_layout)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct wined3d_vertex_declaration *wined3d_declaration;
    struct d3d_input_layout *input_layout_impl;

    TRACE("iface %p, input_layout %p.\n", iface, input_layout);

    wined3d_mutex_lock();
    if (!(wined3d_declaration = wined3d_device_context_get_vertex_declaration(context->wined3d_context)))
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

static void STDMETHODCALLTYPE d3d11_device_context_IAGetVertexBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers, UINT *strides, UINT *offsets)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p, strides %p, offsets %p.\n",
            iface, start_slot, buffer_count, buffers, strides, offsets);

    wined3d_mutex_lock();
    for (i = 0; i < buffer_count; ++i)
    {
        struct wined3d_buffer *wined3d_buffer = NULL;
        struct d3d_buffer *buffer_impl;

        if (FAILED(wined3d_device_context_get_stream_source(context->wined3d_context, start_slot + i,
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

static void STDMETHODCALLTYPE d3d11_device_context_IAGetIndexBuffer(ID3D11DeviceContext1 *iface,
        ID3D11Buffer **buffer, DXGI_FORMAT *format, UINT *offset)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    enum wined3d_format_id wined3d_format;
    struct wined3d_buffer *wined3d_buffer;
    struct d3d_buffer *buffer_impl;

    TRACE("iface %p, buffer %p, format %p, offset %p.\n", iface, buffer, format, offset);

    wined3d_mutex_lock();
    wined3d_buffer = wined3d_device_context_get_index_buffer(context->wined3d_context, &wined3d_format, offset);
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

static void STDMETHODCALLTYPE d3d11_device_context_GSGetConstantBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d11_device_context_get_constant_buffers(iface, WINED3D_SHADER_TYPE_GEOMETRY, start_slot,
            buffer_count, buffers, NULL, NULL);
}

static void STDMETHODCALLTYPE d3d11_device_context_GSGetShader(ID3D11DeviceContext1 *iface,
        ID3D11GeometryShader **shader, ID3D11ClassInstance **class_instances, UINT *class_instance_count)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d_geometry_shader *shader_impl;
    struct wined3d_shader *wined3d_shader;

    TRACE("iface %p, shader %p, class_instances %p, class_instance_count %p.\n",
            iface, shader, class_instances, class_instance_count);

    if (class_instance_count)
        *class_instance_count = 0;

    wined3d_mutex_lock();
    if (!(wined3d_shader = wined3d_device_context_get_shader(context->wined3d_context, WINED3D_SHADER_TYPE_GEOMETRY)))
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

static void STDMETHODCALLTYPE d3d11_device_context_IAGetPrimitiveTopology(ID3D11DeviceContext1 *iface,
        D3D11_PRIMITIVE_TOPOLOGY *topology)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    enum wined3d_primitive_type primitive_type;
    unsigned int patch_vertex_count;

    TRACE("iface %p, topology %p.\n", iface, topology);

    wined3d_mutex_lock();
    wined3d_device_context_get_primitive_type(context->wined3d_context, &primitive_type, &patch_vertex_count);
    wined3d_mutex_unlock();

    d3d11_primitive_topology_from_wined3d_primitive_type(primitive_type, patch_vertex_count, topology);
}

static void STDMETHODCALLTYPE d3d11_device_context_VSGetShaderResources(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11ShaderResourceView **views)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n", iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct wined3d_shader_resource_view *wined3d_view;
        struct d3d_shader_resource_view *view_impl;

        if (!(wined3d_view = wined3d_device_context_get_shader_resource_view(
                context->wined3d_context, WINED3D_SHADER_TYPE_VERTEX, start_slot + i)))
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

static void STDMETHODCALLTYPE d3d11_device_context_VSGetSamplers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT sampler_count, ID3D11SamplerState **samplers)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    wined3d_mutex_lock();
    for (i = 0; i < sampler_count; ++i)
    {
        struct wined3d_sampler *wined3d_sampler;
        struct d3d_sampler_state *sampler_impl;

        if (!(wined3d_sampler = wined3d_device_context_get_sampler(
                context->wined3d_context, WINED3D_SHADER_TYPE_VERTEX, start_slot + i)))
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

static void STDMETHODCALLTYPE d3d11_device_context_GetPredication(ID3D11DeviceContext1 *iface,
        ID3D11Predicate **predicate, BOOL *value)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct wined3d_query *wined3d_predicate;
    struct d3d_query *predicate_impl;

    TRACE("iface %p, predicate %p, value %p.\n", iface, predicate, value);

    wined3d_mutex_lock();
    if (!(wined3d_predicate = wined3d_device_context_get_predication(context->wined3d_context, value)))
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

static void STDMETHODCALLTYPE d3d11_device_context_GSGetShaderResources(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11ShaderResourceView **views)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n", iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct wined3d_shader_resource_view *wined3d_view;
        struct d3d_shader_resource_view *view_impl;

        if (!(wined3d_view = wined3d_device_context_get_shader_resource_view(
                context->wined3d_context, WINED3D_SHADER_TYPE_GEOMETRY, start_slot + i)))
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

static void STDMETHODCALLTYPE d3d11_device_context_GSGetSamplers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT sampler_count, ID3D11SamplerState **samplers)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    wined3d_mutex_lock();
    for (i = 0; i < sampler_count; ++i)
    {
        struct d3d_sampler_state *sampler_impl;
        struct wined3d_sampler *wined3d_sampler;

        if (!(wined3d_sampler = wined3d_device_context_get_sampler(
                context->wined3d_context, WINED3D_SHADER_TYPE_GEOMETRY, start_slot + i)))
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

static void STDMETHODCALLTYPE d3d11_device_context_OMGetRenderTargets(ID3D11DeviceContext1 *iface,
        UINT render_target_view_count, ID3D11RenderTargetView **render_target_views,
        ID3D11DepthStencilView **depth_stencil_view)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
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
            if (!(wined3d_view = wined3d_device_context_get_rendertarget_view(context->wined3d_context, i))
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

        if (!(wined3d_view = wined3d_device_context_get_depth_stencil_view(context->wined3d_context))
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

static void STDMETHODCALLTYPE d3d11_device_context_OMGetRenderTargetsAndUnorderedAccessViews(
        ID3D11DeviceContext1 *iface,
        UINT render_target_view_count, ID3D11RenderTargetView **render_target_views,
        ID3D11DepthStencilView **depth_stencil_view,
        UINT unordered_access_view_start_slot, UINT unordered_access_view_count,
        ID3D11UnorderedAccessView **unordered_access_views)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct wined3d_unordered_access_view *wined3d_view;
    struct d3d11_unordered_access_view *view_impl;
    unsigned int i;

    TRACE("iface %p, render_target_view_count %u, render_target_views %p, depth_stencil_view %p, "
            "unordered_access_view_start_slot %u, unordered_access_view_count %u, "
            "unordered_access_views %p.\n",
            iface, render_target_view_count, render_target_views, depth_stencil_view,
            unordered_access_view_start_slot, unordered_access_view_count, unordered_access_views);

    if (render_target_views || depth_stencil_view)
        d3d11_device_context_OMGetRenderTargets(iface, render_target_view_count,
                render_target_views, depth_stencil_view);

    if (unordered_access_views)
    {
        wined3d_mutex_lock();
        for (i = 0; i < unordered_access_view_count; ++i)
        {
            if (!(wined3d_view = wined3d_device_context_get_unordered_access_view(context->wined3d_context,
                    WINED3D_PIPELINE_GRAPHICS, unordered_access_view_start_slot + i)))
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

static void STDMETHODCALLTYPE d3d11_device_context_OMGetBlendState(ID3D11DeviceContext1 *iface,
        ID3D11BlendState **blend_state, FLOAT blend_factor[4], UINT *sample_mask)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct wined3d_blend_state *wined3d_state;
    struct d3d_blend_state *blend_state_impl;
    unsigned int tmp_sample_mask;
    float tmp_blend_factor[4];

    TRACE("iface %p, blend_state %p, blend_factor %p, sample_mask %p.\n",
            iface, blend_state, blend_factor, sample_mask);

    wined3d_mutex_lock();
    if (!blend_factor) blend_factor = tmp_blend_factor;
    if (!sample_mask) sample_mask = &tmp_sample_mask;
    wined3d_state = wined3d_device_context_get_blend_state(context->wined3d_context,
            (struct wined3d_color *)blend_factor, sample_mask);
    if (blend_state)
    {
        if (wined3d_state)
        {
            blend_state_impl = wined3d_blend_state_get_parent(wined3d_state);
            ID3D11BlendState_AddRef(*blend_state = (ID3D11BlendState *)&blend_state_impl->ID3D11BlendState1_iface);
        }
        else
            *blend_state = NULL;
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_device_context_OMGetDepthStencilState(ID3D11DeviceContext1 *iface,
        ID3D11DepthStencilState **depth_stencil_state, UINT *stencil_ref)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct wined3d_depth_stencil_state *wined3d_state;
    struct d3d_depthstencil_state *state_impl;
    UINT stencil_ref_tmp;

    TRACE("iface %p, depth_stencil_state %p, stencil_ref %p.\n",
            iface, depth_stencil_state, stencil_ref);

    wined3d_mutex_lock();
    if (!stencil_ref) stencil_ref = &stencil_ref_tmp;
    wined3d_state = wined3d_device_context_get_depth_stencil_state(context->wined3d_context, stencil_ref);
    if (depth_stencil_state)
    {
        if (wined3d_state)
        {
            state_impl = wined3d_depth_stencil_state_get_parent(wined3d_state);
            ID3D11DepthStencilState_AddRef(*depth_stencil_state = &state_impl->ID3D11DepthStencilState_iface);
        }
        else
        {
            *depth_stencil_state = NULL;
        }
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_device_context_SOGetTargets(ID3D11DeviceContext1 *iface,
        UINT buffer_count, ID3D11Buffer **buffers)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, buffer_count %u, buffers %p.\n", iface, buffer_count, buffers);

    wined3d_mutex_lock();
    for (i = 0; i < buffer_count; ++i)
    {
        struct wined3d_buffer *wined3d_buffer;
        struct d3d_buffer *buffer_impl;

        if (!(wined3d_buffer = wined3d_device_context_get_stream_output(context->wined3d_context, i, NULL)))
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

static void STDMETHODCALLTYPE d3d11_device_context_RSGetState(ID3D11DeviceContext1 *iface,
        ID3D11RasterizerState **rasterizer_state)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d_rasterizer_state *rasterizer_state_impl;
    struct wined3d_rasterizer_state *wined3d_state;

    TRACE("iface %p, rasterizer_state %p.\n", iface, rasterizer_state);

    wined3d_mutex_lock();
    if ((wined3d_state = wined3d_device_context_get_rasterizer_state(context->wined3d_context)))
    {
        rasterizer_state_impl = wined3d_rasterizer_state_get_parent(wined3d_state);
        *rasterizer_state = (ID3D11RasterizerState *)&rasterizer_state_impl->ID3D11RasterizerState1_iface;
        ID3D11RasterizerState_AddRef(*rasterizer_state);
    }
    else
    {
        *rasterizer_state = NULL;
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_device_context_RSGetViewports(ID3D11DeviceContext1 *iface,
        UINT *viewport_count, D3D11_VIEWPORT *viewports)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct wined3d_viewport wined3d_vp[WINED3D_MAX_VIEWPORTS];
    unsigned int actual_count = ARRAY_SIZE(wined3d_vp), i;

    TRACE("iface %p, viewport_count %p, viewports %p.\n", iface, viewport_count, viewports);

    if (!viewport_count)
        return;

    wined3d_mutex_lock();
    wined3d_device_context_get_viewports(context->wined3d_context, &actual_count, viewports ? wined3d_vp : NULL);
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

static void STDMETHODCALLTYPE d3d11_device_context_RSGetScissorRects(ID3D11DeviceContext1 *iface,
        UINT *rect_count, D3D11_RECT *rects)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    unsigned int actual_count;

    TRACE("iface %p, rect_count %p, rects %p.\n", iface, rect_count, rects);

    if (!rect_count)
        return;

    actual_count = *rect_count;

    wined3d_mutex_lock();
    wined3d_device_context_get_scissor_rects(context->wined3d_context, &actual_count, rects);
    wined3d_mutex_unlock();

    if (rects && *rect_count > actual_count)
        memset(&rects[actual_count], 0, (*rect_count - actual_count) * sizeof(*rects));
    *rect_count = actual_count;
}

static void STDMETHODCALLTYPE d3d11_device_context_HSGetShaderResources(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11ShaderResourceView **views)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n", iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct wined3d_shader_resource_view *wined3d_view;
        struct d3d_shader_resource_view *view_impl;

        if (!(wined3d_view = wined3d_device_context_get_shader_resource_view(
                context->wined3d_context, WINED3D_SHADER_TYPE_HULL, start_slot + i)))
        {
            views[i] = NULL;
            continue;
        }

        view_impl = wined3d_shader_resource_view_get_parent(wined3d_view);
        ID3D11ShaderResourceView_AddRef(views[i] = &view_impl->ID3D11ShaderResourceView_iface);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_device_context_HSGetShader(ID3D11DeviceContext1 *iface,
        ID3D11HullShader **shader, ID3D11ClassInstance **class_instances, UINT *class_instance_count)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d11_hull_shader *shader_impl;
    struct wined3d_shader *wined3d_shader;

    TRACE("iface %p, shader %p, class_instances %p, class_instance_count %p.\n",
            iface, shader, class_instances, class_instance_count);

    if (class_instance_count)
        *class_instance_count = 0;

    wined3d_mutex_lock();
    if (!(wined3d_shader = wined3d_device_context_get_shader(context->wined3d_context, WINED3D_SHADER_TYPE_HULL)))
    {
        wined3d_mutex_unlock();
        *shader = NULL;
        return;
    }

    shader_impl = wined3d_shader_get_parent(wined3d_shader);
    wined3d_mutex_unlock();
    ID3D11HullShader_AddRef(*shader = &shader_impl->ID3D11HullShader_iface);
}

static void STDMETHODCALLTYPE d3d11_device_context_HSGetSamplers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT sampler_count, ID3D11SamplerState **samplers)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    wined3d_mutex_lock();
    for (i = 0; i < sampler_count; ++i)
    {
        struct wined3d_sampler *wined3d_sampler;
        struct d3d_sampler_state *sampler_impl;

        if (!(wined3d_sampler = wined3d_device_context_get_sampler(
                context->wined3d_context, WINED3D_SHADER_TYPE_HULL, start_slot + i)))
        {
            samplers[i] = NULL;
            continue;
        }

        sampler_impl = wined3d_sampler_get_parent(wined3d_sampler);
        ID3D11SamplerState_AddRef(samplers[i] = &sampler_impl->ID3D11SamplerState_iface);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_device_context_HSGetConstantBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d11_device_context_get_constant_buffers(iface, WINED3D_SHADER_TYPE_HULL, start_slot,
            buffer_count, buffers, NULL, NULL);
}

static void STDMETHODCALLTYPE d3d11_device_context_DSGetShaderResources(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11ShaderResourceView **views)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n",
            iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct wined3d_shader_resource_view *wined3d_view;
        struct d3d_shader_resource_view *view_impl;

        if (!(wined3d_view = wined3d_device_context_get_shader_resource_view(
                context->wined3d_context, WINED3D_SHADER_TYPE_DOMAIN, start_slot + i)))
        {
            views[i] = NULL;
            continue;
        }

        view_impl = wined3d_shader_resource_view_get_parent(wined3d_view);
        ID3D11ShaderResourceView_AddRef(views[i] = &view_impl->ID3D11ShaderResourceView_iface);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_device_context_DSGetShader(ID3D11DeviceContext1 *iface,
        ID3D11DomainShader **shader, ID3D11ClassInstance **class_instances, UINT *class_instance_count)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d11_domain_shader *shader_impl;
    struct wined3d_shader *wined3d_shader;

    TRACE("iface %p, shader %p, class_instances %p, class_instance_count %p.\n",
            iface, shader, class_instances, class_instance_count);

    if (class_instance_count)
        *class_instance_count = 0;

    wined3d_mutex_lock();
    if (!(wined3d_shader = wined3d_device_context_get_shader(context->wined3d_context, WINED3D_SHADER_TYPE_DOMAIN)))
    {
        wined3d_mutex_unlock();
        *shader = NULL;
        return;
    }

    shader_impl = wined3d_shader_get_parent(wined3d_shader);
    wined3d_mutex_unlock();
    ID3D11DomainShader_AddRef(*shader = &shader_impl->ID3D11DomainShader_iface);
}

static void STDMETHODCALLTYPE d3d11_device_context_DSGetSamplers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT sampler_count, ID3D11SamplerState **samplers)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    wined3d_mutex_lock();
    for (i = 0; i < sampler_count; ++i)
    {
        struct wined3d_sampler *wined3d_sampler;
        struct d3d_sampler_state *sampler_impl;

        if (!(wined3d_sampler = wined3d_device_context_get_sampler(
                context->wined3d_context, WINED3D_SHADER_TYPE_DOMAIN, start_slot + i)))
        {
            samplers[i] = NULL;
            continue;
        }

        sampler_impl = wined3d_sampler_get_parent(wined3d_sampler);
        ID3D11SamplerState_AddRef(samplers[i] = &sampler_impl->ID3D11SamplerState_iface);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_device_context_DSGetConstantBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d11_device_context_get_constant_buffers(iface, WINED3D_SHADER_TYPE_DOMAIN, start_slot,
            buffer_count, buffers, NULL, NULL);
}

static void STDMETHODCALLTYPE d3d11_device_context_CSGetShaderResources(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11ShaderResourceView **views)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n", iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct wined3d_shader_resource_view *wined3d_view;
        struct d3d_shader_resource_view *view_impl;

        if (!(wined3d_view = wined3d_device_context_get_shader_resource_view(
                context->wined3d_context, WINED3D_SHADER_TYPE_COMPUTE, start_slot + i)))
        {
            views[i] = NULL;
            continue;
        }

        view_impl = wined3d_shader_resource_view_get_parent(wined3d_view);
        ID3D11ShaderResourceView_AddRef(views[i] = &view_impl->ID3D11ShaderResourceView_iface);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_device_context_CSGetUnorderedAccessViews(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT view_count, ID3D11UnorderedAccessView **views)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n", iface, start_slot, view_count, views);

    wined3d_mutex_lock();
    for (i = 0; i < view_count; ++i)
    {
        struct wined3d_unordered_access_view *wined3d_view;
        struct d3d11_unordered_access_view *view_impl;

        if (!(wined3d_view = wined3d_device_context_get_unordered_access_view(
                context->wined3d_context, WINED3D_PIPELINE_COMPUTE, start_slot + i)))
        {
            views[i] = NULL;
            continue;
        }

        view_impl = wined3d_unordered_access_view_get_parent(wined3d_view);
        ID3D11UnorderedAccessView_AddRef(views[i] = &view_impl->ID3D11UnorderedAccessView_iface);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_device_context_CSGetShader(ID3D11DeviceContext1 *iface,
        ID3D11ComputeShader **shader, ID3D11ClassInstance **class_instances, UINT *class_instance_count)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d11_compute_shader *shader_impl;
    struct wined3d_shader *wined3d_shader;

    TRACE("iface %p, shader %p, class_instances %p, class_instance_count %p.\n",
            iface, shader, class_instances, class_instance_count);

    if (class_instance_count)
        *class_instance_count = 0;

    wined3d_mutex_lock();
    if (!(wined3d_shader = wined3d_device_context_get_shader(context->wined3d_context, WINED3D_SHADER_TYPE_COMPUTE)))
    {
        wined3d_mutex_unlock();
        *shader = NULL;
        return;
    }

    shader_impl = wined3d_shader_get_parent(wined3d_shader);
    wined3d_mutex_unlock();
    ID3D11ComputeShader_AddRef(*shader = &shader_impl->ID3D11ComputeShader_iface);
}

static void STDMETHODCALLTYPE d3d11_device_context_CSGetSamplers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT sampler_count, ID3D11SamplerState **samplers)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    wined3d_mutex_lock();
    for (i = 0; i < sampler_count; ++i)
    {
        struct wined3d_sampler *wined3d_sampler;
        struct d3d_sampler_state *sampler_impl;

        if (!(wined3d_sampler = wined3d_device_context_get_sampler(
                context->wined3d_context, WINED3D_SHADER_TYPE_COMPUTE, start_slot + i)))
        {
            samplers[i] = NULL;
            continue;
        }

        sampler_impl = wined3d_sampler_get_parent(wined3d_sampler);
        ID3D11SamplerState_AddRef(samplers[i] = &sampler_impl->ID3D11SamplerState_iface);
    }
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_device_context_CSGetConstantBuffers(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p.\n",
            iface, start_slot, buffer_count, buffers);

    d3d11_device_context_get_constant_buffers(iface, WINED3D_SHADER_TYPE_COMPUTE, start_slot,
            buffer_count, buffers, NULL, NULL);
}

static void STDMETHODCALLTYPE d3d11_device_context_ClearState(ID3D11DeviceContext1 *iface)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);

    TRACE("iface %p.\n", iface);

    wined3d_device_context_reset_state(context->wined3d_context);
}

static void STDMETHODCALLTYPE d3d11_device_context_Flush(ID3D11DeviceContext1 *iface)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);

    TRACE("iface %p.\n", iface);

    wined3d_device_context_flush(context->wined3d_context);
}

static D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE d3d11_device_context_GetType(ID3D11DeviceContext1 *iface)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);

    TRACE("iface %p.\n", iface);

    return context->type;
}

static UINT STDMETHODCALLTYPE d3d11_device_context_GetContextFlags(ID3D11DeviceContext1 *iface)
{
    TRACE("iface %p.\n", iface);

    return 0;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_context_FinishCommandList(ID3D11DeviceContext1 *iface,
        BOOL restore, ID3D11CommandList **command_list)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d11_command_list *object;
    HRESULT hr;

    TRACE("iface %p, restore %#x, command_list %p.\n", iface, restore, command_list);

    if (context->type == D3D11_DEVICE_CONTEXT_IMMEDIATE)
    {
        WARN("Attempt to record command list on an immediate context; returning DXGI_ERROR_INVALID_CALL.\n");
        return DXGI_ERROR_INVALID_CALL;
    }

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = wined3d_deferred_context_record_command_list(context->wined3d_context,
            !!restore, &object->wined3d_list)))
    {
        WARN("Failed to record wined3d command list, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    object->ID3D11CommandList_iface.lpVtbl = &d3d11_command_list_vtbl;
    object->refcount = 1;
    object->device = &context->device->ID3D11Device2_iface;
    wined3d_private_store_init(&object->private_store);

    ID3D11Device2_AddRef(object->device);

    TRACE("Created command list %p.\n", object);
    *command_list = &object->ID3D11CommandList_iface;

    return S_OK;
}

static void STDMETHODCALLTYPE d3d11_device_context_CopySubresourceRegion1(ID3D11DeviceContext1 *iface,
        ID3D11Resource *dst_resource, UINT dst_subresource_idx, UINT dst_x, UINT dst_y, UINT dst_z,
        ID3D11Resource *src_resource, UINT src_subresource_idx, const D3D11_BOX *src_box, UINT flags)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct wined3d_resource *wined3d_dst_resource, *wined3d_src_resource;
    struct wined3d_box wined3d_src_box;

    TRACE("iface %p, dst_resource %p, dst_subresource_idx %u, dst_x %u, dst_y %u, dst_z %u, "
            "src_resource %p, src_subresource_idx %u, src_box %p, flags %#x.\n",
            iface, dst_resource, dst_subresource_idx, dst_x, dst_y, dst_z,
            src_resource, src_subresource_idx, src_box, flags);

    if (!dst_resource || !src_resource)
        return;

    if (src_box)
        wined3d_box_set(&wined3d_src_box, src_box->left, src_box->top,
                src_box->right, src_box->bottom, src_box->front, src_box->back);

    wined3d_dst_resource = wined3d_resource_from_d3d11_resource(dst_resource);
    wined3d_src_resource = wined3d_resource_from_d3d11_resource(src_resource);
    wined3d_device_context_copy_sub_resource_region(context->wined3d_context, wined3d_dst_resource, dst_subresource_idx,
            dst_x, dst_y, dst_z, wined3d_src_resource, src_subresource_idx, src_box ? &wined3d_src_box : NULL, flags);
}

static void STDMETHODCALLTYPE d3d11_device_context_UpdateSubresource1(ID3D11DeviceContext1 *iface,
        ID3D11Resource *resource, UINT subresource_idx, const D3D11_BOX *box, const void *data,
        UINT row_pitch, UINT depth_pitch, UINT flags)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct wined3d_resource *wined3d_resource;
    struct wined3d_box wined3d_box;

    TRACE("iface %p, resource %p, subresource_idx %u, box %p, data %p, row_pitch %u, depth_pitch %u, flags %#x.\n",
            iface, resource, subresource_idx, box, data, row_pitch, depth_pitch, flags);

    if (box)
        wined3d_box_set(&wined3d_box, box->left, box->top, box->right, box->bottom,
                box->front, box->back);

    wined3d_resource = wined3d_resource_from_d3d11_resource(resource);
    wined3d_device_context_update_sub_resource(context->wined3d_context, wined3d_resource, subresource_idx,
            box ? &wined3d_box : NULL, data, row_pitch, depth_pitch, flags);
}

static void STDMETHODCALLTYPE d3d11_device_context_DiscardResource(ID3D11DeviceContext1 *iface,
        ID3D11Resource *resource)
{
    FIXME("iface %p, resource %p stub!\n", iface, resource);
}

static void STDMETHODCALLTYPE d3d11_device_context_DiscardView(ID3D11DeviceContext1 *iface, ID3D11View *view)
{
    FIXME("iface %p, view %p stub!\n", iface, view);
}

static void STDMETHODCALLTYPE d3d11_device_context_VSSetConstantBuffers1(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer * const *buffers, const UINT *first_constant,
        const UINT *num_constants)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p, first_constant %p, num_constants %p.\n",
            iface, start_slot, buffer_count, buffers, first_constant, num_constants);

    d3d11_device_context_set_constant_buffers(iface, WINED3D_SHADER_TYPE_VERTEX, start_slot,
            buffer_count, buffers, first_constant, num_constants);
}

static void STDMETHODCALLTYPE d3d11_device_context_HSSetConstantBuffers1(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer * const *buffers, const UINT *first_constant,
        const UINT *num_constants)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p, first_constant %p, num_constants %p.\n",
            iface, start_slot, buffer_count, buffers, first_constant, num_constants);

    d3d11_device_context_set_constant_buffers(iface, WINED3D_SHADER_TYPE_HULL, start_slot,
            buffer_count, buffers, first_constant, num_constants);
}

static void STDMETHODCALLTYPE d3d11_device_context_DSSetConstantBuffers1(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer * const *buffers, const UINT *first_constant,
        const UINT *num_constants)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p, first_constant %p, num_constants %p.\n",
            iface, start_slot, buffer_count, buffers, first_constant, num_constants);

    d3d11_device_context_set_constant_buffers(iface, WINED3D_SHADER_TYPE_DOMAIN, start_slot,
            buffer_count, buffers, first_constant, num_constants);
}

static void STDMETHODCALLTYPE d3d11_device_context_GSSetConstantBuffers1(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer * const *buffers, const UINT *first_constant,
        const UINT *num_constants)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p, first_constant %p, num_constants %p.\n",
            iface, start_slot, buffer_count, buffers, first_constant, num_constants);

    d3d11_device_context_set_constant_buffers(iface, WINED3D_SHADER_TYPE_GEOMETRY, start_slot,
            buffer_count, buffers, first_constant, num_constants);
}

static void STDMETHODCALLTYPE d3d11_device_context_PSSetConstantBuffers1(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer * const *buffers, const UINT *first_constant,
        const UINT *num_constants)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p, first_constant %p, num_constants %p.\n",
            iface, start_slot, buffer_count, buffers, first_constant, num_constants);

    d3d11_device_context_set_constant_buffers(iface, WINED3D_SHADER_TYPE_PIXEL, start_slot,
            buffer_count, buffers, first_constant, num_constants);
}

static void STDMETHODCALLTYPE d3d11_device_context_CSSetConstantBuffers1(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer * const *buffers, const UINT *first_constant,
        const UINT *num_constants)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p, first_constant %p, num_constants %p.\n",
            iface, start_slot, buffer_count, buffers, first_constant, num_constants);

    d3d11_device_context_set_constant_buffers(iface, WINED3D_SHADER_TYPE_COMPUTE, start_slot,
            buffer_count, buffers, first_constant, num_constants);
}

static void STDMETHODCALLTYPE d3d11_device_context_VSGetConstantBuffers1(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers, UINT *first_constant, UINT *num_constants)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p, first_constant %p, num_constants %p.\n",
            iface, start_slot, buffer_count, buffers, first_constant, num_constants);

    d3d11_device_context_get_constant_buffers(iface, WINED3D_SHADER_TYPE_VERTEX, start_slot,
            buffer_count, buffers, first_constant, num_constants);
}

static void STDMETHODCALLTYPE d3d11_device_context_HSGetConstantBuffers1(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers, UINT *first_constant, UINT *num_constants)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p, first_constant %p, num_constants %p.\n",
            iface, start_slot, buffer_count, buffers, first_constant, num_constants);

    d3d11_device_context_get_constant_buffers(iface, WINED3D_SHADER_TYPE_HULL, start_slot,
            buffer_count, buffers, first_constant, num_constants);
}

static void STDMETHODCALLTYPE d3d11_device_context_DSGetConstantBuffers1(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers, UINT *first_constant, UINT *num_constants)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p, first_constant %p, num_constants %p.\n",
            iface, start_slot, buffer_count, buffers, first_constant, num_constants);

    d3d11_device_context_get_constant_buffers(iface, WINED3D_SHADER_TYPE_DOMAIN, start_slot,
            buffer_count, buffers, first_constant, num_constants);
}

static void STDMETHODCALLTYPE d3d11_device_context_GSGetConstantBuffers1(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers, UINT *first_constant, UINT *num_constants)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p, first_constant %p, num_constants %p.\n",
            iface, start_slot, buffer_count, buffers, first_constant, num_constants);

    d3d11_device_context_get_constant_buffers(iface, WINED3D_SHADER_TYPE_GEOMETRY, start_slot,
            buffer_count, buffers, first_constant, num_constants);
}

static void STDMETHODCALLTYPE d3d11_device_context_PSGetConstantBuffers1(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers, UINT *first_constant, UINT *num_constants)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p, first_constant %p, num_constants %p.\n",
            iface, start_slot, buffer_count, buffers, first_constant, num_constants);

    d3d11_device_context_get_constant_buffers(iface, WINED3D_SHADER_TYPE_PIXEL, start_slot,
            buffer_count, buffers, first_constant, num_constants);
}

static void STDMETHODCALLTYPE d3d11_device_context_CSGetConstantBuffers1(ID3D11DeviceContext1 *iface,
        UINT start_slot, UINT buffer_count, ID3D11Buffer **buffers, UINT *first_constant, UINT *num_constants)
{
    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p, first_constant %p, num_constants %p.\n",
            iface, start_slot, buffer_count, buffers, first_constant, num_constants);

    d3d11_device_context_get_constant_buffers(iface, WINED3D_SHADER_TYPE_COMPUTE, start_slot,
            buffer_count, buffers, first_constant, num_constants);
}

static void STDMETHODCALLTYPE d3d11_device_context_SwapDeviceContextState(ID3D11DeviceContext1 *iface,
        ID3DDeviceContextState *state, ID3DDeviceContextState **prev)
{
    struct d3d11_device_context *context = impl_from_ID3D11DeviceContext1(iface);
    struct d3d_device_context_state *state_impl, *prev_impl;
    struct d3d_device *device = context->device;
    struct wined3d_state *wined3d_state;
    static unsigned int once;

    TRACE("iface %p, state %p, prev %p.\n", iface, state, prev);

    if (prev)
        *prev = NULL;

    if (context->type != D3D11_DEVICE_CONTEXT_IMMEDIATE)
    {
        WARN("SwapDeviceContextState is not allowed on a deferred context.\n");
        return;
    }

    if (!state)
        return;

    wined3d_mutex_lock();

    prev_impl = device->state;
    state_impl = impl_from_ID3DDeviceContextState(state);
    if (!(wined3d_state = d3d_device_context_state_get_wined3d_state(state_impl, device)))
        ERR("Failed to get wined3d state for device context state %p.\n", state_impl);
    wined3d_device_context_set_state(context->wined3d_context, wined3d_state);

    if (prev)
        ID3DDeviceContextState_AddRef(*prev = &prev_impl->ID3DDeviceContextState_iface);

    d3d_device_context_state_private_addref(state_impl);
    device->state = state_impl;
    d3d_device_context_state_private_release(prev_impl);

    if (d3d_device_is_d3d10_active(device) && !once++)
        FIXME("D3D10 interface emulation not fully implemented yet!\n");
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d11_device_context_ClearView(ID3D11DeviceContext1 *iface, ID3D11View *view,
        const FLOAT color[4], const D3D11_RECT *rect, UINT num_rects)
{
    FIXME("iface %p, view %p, color %p, rect %p, num_rects %u stub!\n", iface, view, color, rect, num_rects);
}

static void STDMETHODCALLTYPE d3d11_device_context_DiscardView1(ID3D11DeviceContext1 *iface, ID3D11View *view,
        const D3D11_RECT *rects, UINT num_rects)
{
    FIXME("iface %p, view %p, rects %p, num_rects %u stub!\n", iface, view, rects, num_rects);
}

static const struct ID3D11DeviceContext1Vtbl d3d11_device_context_vtbl =
{
    /* IUnknown methods */
    d3d11_device_context_QueryInterface,
    d3d11_device_context_AddRef,
    d3d11_device_context_Release,
    /* ID3D11DeviceChild methods */
    d3d11_device_context_GetDevice,
    d3d11_device_context_GetPrivateData,
    d3d11_device_context_SetPrivateData,
    d3d11_device_context_SetPrivateDataInterface,
    /* ID3D11DeviceContext methods */
    d3d11_device_context_VSSetConstantBuffers,
    d3d11_device_context_PSSetShaderResources,
    d3d11_device_context_PSSetShader,
    d3d11_device_context_PSSetSamplers,
    d3d11_device_context_VSSetShader,
    d3d11_device_context_DrawIndexed,
    d3d11_device_context_Draw,
    d3d11_device_context_Map,
    d3d11_device_context_Unmap,
    d3d11_device_context_PSSetConstantBuffers,
    d3d11_device_context_IASetInputLayout,
    d3d11_device_context_IASetVertexBuffers,
    d3d11_device_context_IASetIndexBuffer,
    d3d11_device_context_DrawIndexedInstanced,
    d3d11_device_context_DrawInstanced,
    d3d11_device_context_GSSetConstantBuffers,
    d3d11_device_context_GSSetShader,
    d3d11_device_context_IASetPrimitiveTopology,
    d3d11_device_context_VSSetShaderResources,
    d3d11_device_context_VSSetSamplers,
    d3d11_device_context_Begin,
    d3d11_device_context_End,
    d3d11_device_context_GetData,
    d3d11_device_context_SetPredication,
    d3d11_device_context_GSSetShaderResources,
    d3d11_device_context_GSSetSamplers,
    d3d11_device_context_OMSetRenderTargets,
    d3d11_device_context_OMSetRenderTargetsAndUnorderedAccessViews,
    d3d11_device_context_OMSetBlendState,
    d3d11_device_context_OMSetDepthStencilState,
    d3d11_device_context_SOSetTargets,
    d3d11_device_context_DrawAuto,
    d3d11_device_context_DrawIndexedInstancedIndirect,
    d3d11_device_context_DrawInstancedIndirect,
    d3d11_device_context_Dispatch,
    d3d11_device_context_DispatchIndirect,
    d3d11_device_context_RSSetState,
    d3d11_device_context_RSSetViewports,
    d3d11_device_context_RSSetScissorRects,
    d3d11_device_context_CopySubresourceRegion,
    d3d11_device_context_CopyResource,
    d3d11_device_context_UpdateSubresource,
    d3d11_device_context_CopyStructureCount,
    d3d11_device_context_ClearRenderTargetView,
    d3d11_device_context_ClearUnorderedAccessViewUint,
    d3d11_device_context_ClearUnorderedAccessViewFloat,
    d3d11_device_context_ClearDepthStencilView,
    d3d11_device_context_GenerateMips,
    d3d11_device_context_SetResourceMinLOD,
    d3d11_device_context_GetResourceMinLOD,
    d3d11_device_context_ResolveSubresource,
    d3d11_device_context_ExecuteCommandList,
    d3d11_device_context_HSSetShaderResources,
    d3d11_device_context_HSSetShader,
    d3d11_device_context_HSSetSamplers,
    d3d11_device_context_HSSetConstantBuffers,
    d3d11_device_context_DSSetShaderResources,
    d3d11_device_context_DSSetShader,
    d3d11_device_context_DSSetSamplers,
    d3d11_device_context_DSSetConstantBuffers,
    d3d11_device_context_CSSetShaderResources,
    d3d11_device_context_CSSetUnorderedAccessViews,
    d3d11_device_context_CSSetShader,
    d3d11_device_context_CSSetSamplers,
    d3d11_device_context_CSSetConstantBuffers,
    d3d11_device_context_VSGetConstantBuffers,
    d3d11_device_context_PSGetShaderResources,
    d3d11_device_context_PSGetShader,
    d3d11_device_context_PSGetSamplers,
    d3d11_device_context_VSGetShader,
    d3d11_device_context_PSGetConstantBuffers,
    d3d11_device_context_IAGetInputLayout,
    d3d11_device_context_IAGetVertexBuffers,
    d3d11_device_context_IAGetIndexBuffer,
    d3d11_device_context_GSGetConstantBuffers,
    d3d11_device_context_GSGetShader,
    d3d11_device_context_IAGetPrimitiveTopology,
    d3d11_device_context_VSGetShaderResources,
    d3d11_device_context_VSGetSamplers,
    d3d11_device_context_GetPredication,
    d3d11_device_context_GSGetShaderResources,
    d3d11_device_context_GSGetSamplers,
    d3d11_device_context_OMGetRenderTargets,
    d3d11_device_context_OMGetRenderTargetsAndUnorderedAccessViews,
    d3d11_device_context_OMGetBlendState,
    d3d11_device_context_OMGetDepthStencilState,
    d3d11_device_context_SOGetTargets,
    d3d11_device_context_RSGetState,
    d3d11_device_context_RSGetViewports,
    d3d11_device_context_RSGetScissorRects,
    d3d11_device_context_HSGetShaderResources,
    d3d11_device_context_HSGetShader,
    d3d11_device_context_HSGetSamplers,
    d3d11_device_context_HSGetConstantBuffers,
    d3d11_device_context_DSGetShaderResources,
    d3d11_device_context_DSGetShader,
    d3d11_device_context_DSGetSamplers,
    d3d11_device_context_DSGetConstantBuffers,
    d3d11_device_context_CSGetShaderResources,
    d3d11_device_context_CSGetUnorderedAccessViews,
    d3d11_device_context_CSGetShader,
    d3d11_device_context_CSGetSamplers,
    d3d11_device_context_CSGetConstantBuffers,
    d3d11_device_context_ClearState,
    d3d11_device_context_Flush,
    d3d11_device_context_GetType,
    d3d11_device_context_GetContextFlags,
    d3d11_device_context_FinishCommandList,
    /* ID3D11DeviceContext1 methods */
    d3d11_device_context_CopySubresourceRegion1,
    d3d11_device_context_UpdateSubresource1,
    d3d11_device_context_DiscardResource,
    d3d11_device_context_DiscardView,
    d3d11_device_context_VSSetConstantBuffers1,
    d3d11_device_context_HSSetConstantBuffers1,
    d3d11_device_context_DSSetConstantBuffers1,
    d3d11_device_context_GSSetConstantBuffers1,
    d3d11_device_context_PSSetConstantBuffers1,
    d3d11_device_context_CSSetConstantBuffers1,
    d3d11_device_context_VSGetConstantBuffers1,
    d3d11_device_context_HSGetConstantBuffers1,
    d3d11_device_context_DSGetConstantBuffers1,
    d3d11_device_context_GSGetConstantBuffers1,
    d3d11_device_context_PSGetConstantBuffers1,
    d3d11_device_context_CSGetConstantBuffers1,
    d3d11_device_context_SwapDeviceContextState,
    d3d11_device_context_ClearView,
    d3d11_device_context_DiscardView1,
};

static struct d3d11_device_context *impl_from_ID3D11VideoContext(ID3D11VideoContext *iface)
{
    return CONTAINING_RECORD(iface, struct d3d11_device_context, ID3D11VideoContext_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_video_context_QueryInterface(ID3D11VideoContext *iface,
        REFIID iid, void **out)
{
    struct d3d11_device_context *context = impl_from_ID3D11VideoContext(iface);

    return d3d11_device_context_QueryInterface(&context->ID3D11DeviceContext1_iface, iid, out);
}

static ULONG STDMETHODCALLTYPE d3d11_video_context_AddRef(ID3D11VideoContext *iface)
{
    struct d3d11_device_context *context = impl_from_ID3D11VideoContext(iface);

    return d3d11_device_context_AddRef(&context->ID3D11DeviceContext1_iface);
}

static ULONG STDMETHODCALLTYPE d3d11_video_context_Release(ID3D11VideoContext *iface)
{
    struct d3d11_device_context *context = impl_from_ID3D11VideoContext(iface);

    return d3d11_device_context_Release(&context->ID3D11DeviceContext1_iface);
}

static void STDMETHODCALLTYPE d3d11_video_context_GetDevice(ID3D11VideoContext *iface, ID3D11Device **device)
{
    struct d3d11_device_context *context = impl_from_ID3D11VideoContext(iface);

    return d3d11_device_context_GetDevice(&context->ID3D11DeviceContext1_iface, device);
}

static HRESULT STDMETHODCALLTYPE d3d11_video_context_GetPrivateData(
        ID3D11VideoContext *iface, REFGUID guid, UINT *size, void *data)
{
    struct d3d11_device_context *context = impl_from_ID3D11VideoContext(iface);

    return d3d11_device_context_GetPrivateData(&context->ID3D11DeviceContext1_iface, guid, size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_video_context_SetPrivateData(
        ID3D11VideoContext *iface, REFGUID guid, UINT size, const void *data)
{
    struct d3d11_device_context *context = impl_from_ID3D11VideoContext(iface);

    return d3d11_device_context_SetPrivateData(&context->ID3D11DeviceContext1_iface, guid, size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_video_context_SetPrivateDataInterface(
        ID3D11VideoContext *iface, REFGUID guid, const IUnknown *data)
{
    struct d3d11_device_context *context = impl_from_ID3D11VideoContext(iface);

    return d3d11_device_context_SetPrivateDataInterface(&context->ID3D11DeviceContext1_iface, guid, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_video_context_GetDecoderBuffer(ID3D11VideoContext *iface,
        ID3D11VideoDecoder *decoder, D3D11_VIDEO_DECODER_BUFFER_TYPE type, UINT *size, void **buffer)
{
    FIXME("iface %p, decoder %p, type %#x, size %p, buffer %p, stub!\n", iface, decoder, type, size, buffer);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_context_ReleaseDecoderBuffer(ID3D11VideoContext *iface,
        ID3D11VideoDecoder *decoder, D3D11_VIDEO_DECODER_BUFFER_TYPE type)
{
    FIXME("iface %p, decoder %p, type %#x, stub!\n", iface, decoder, type);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_context_DecoderBeginFrame(ID3D11VideoContext *iface,
        ID3D11VideoDecoder *decoder, ID3D11VideoDecoderOutputView *view, UINT key_size, const void *key)
{
    FIXME("iface %p, decoder %p, view %p, key_size %u, key %p, stub!\n", iface, decoder, view, key_size, key);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_context_DecoderEndFrame(
        ID3D11VideoContext *iface, ID3D11VideoDecoder *decoder)
{
    FIXME("iface %p, decoder %p, stub!\n", iface, decoder);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_context_SubmitDecoderBuffers(ID3D11VideoContext *iface,
        ID3D11VideoDecoder *decoder, UINT count, const D3D11_VIDEO_DECODER_BUFFER_DESC *buffers)
{
    FIXME("iface %p, decoder %p, count %u, buffers %p, stub!\n", iface, decoder, count, buffers);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_context_DecoderExtension(ID3D11VideoContext *iface,
        ID3D11VideoDecoder *decoder, const D3D11_VIDEO_DECODER_EXTENSION *extension)
{
    FIXME("iface %p, decoder %p, extension %p, stub!\n", iface, decoder, extension);
    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorSetOutputTargetRect(
        ID3D11VideoContext *iface, ID3D11VideoProcessor *processor, BOOL enable, const RECT *rect)
{
    FIXME("iface %p, processor %p, enable %d, rect %s, stub!\n", iface, processor, enable, wine_dbgstr_rect(rect));
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorSetOutputBackgroundColor(
        ID3D11VideoContext *iface, ID3D11VideoProcessor *processor, BOOL yuv, const D3D11_VIDEO_COLOR *color)
{
    FIXME("iface %p, processor %p, yuv %d, color {%.8e, %.8e, %.8e, %.8e}, stub!\n",
            iface, processor, yuv, color->u.RGBA.R, color->u.RGBA.G, color->u.RGBA.B, color->u.RGBA.A);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorSetOutputColorSpace(ID3D11VideoContext *iface,
        ID3D11VideoProcessor *processor, const D3D11_VIDEO_PROCESSOR_COLOR_SPACE *space)
{
    FIXME("iface %p, processor %p, space %p, stub!\n", iface, processor, space);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorSetOutputAlphaFillMode(ID3D11VideoContext *iface,
        ID3D11VideoProcessor *processor, D3D11_VIDEO_PROCESSOR_ALPHA_FILL_MODE mode, UINT stream_idx)
{
    FIXME("iface %p, processor %p, mode %#x, stream_idx %u, stub!\n", iface, processor, mode, stream_idx);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorSetOutputConstriction(
        ID3D11VideoContext *iface, ID3D11VideoProcessor *processor, BOOL enable, SIZE size)
{
    FIXME("iface %p, processor %p, enable %d, size (%lux%lu), stub!\n", iface, processor, enable, size.cx, size.cy);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorSetOutputStereoMode(
        ID3D11VideoContext *iface, ID3D11VideoProcessor *processor, BOOL enable)
{
    FIXME("iface %p, processor %p, enable %d, stub!\n", iface, processor, enable);
}

static HRESULT STDMETHODCALLTYPE d3d11_video_context_VideoProcessorSetOutputExtension(ID3D11VideoContext *iface,
        ID3D11VideoProcessor *processor, const GUID *guid, UINT size, void *data)
{
    FIXME("iface %p, processor %p, guid %s, size %u, data %p, stub!\n",
            iface, processor, debugstr_guid(guid), size, data);
    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorGetOutputTargetRect(
        ID3D11VideoContext *iface, ID3D11VideoProcessor *processor, BOOL *enabled, RECT *rect)
{
    FIXME("iface %p, processor %p, enabled %p, rect %p, stub!\n", iface, processor, enabled, rect);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorGetOutputBackgroundColor(
        ID3D11VideoContext *iface, ID3D11VideoProcessor *processor, BOOL *yuv, D3D11_VIDEO_COLOR *color)
{
    FIXME("iface %p, processor %p, yuv %p, color %p, stub!\n", iface, processor, yuv, color);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorGetOutputColorSpace(
        ID3D11VideoContext *iface, ID3D11VideoProcessor *processor, D3D11_VIDEO_PROCESSOR_COLOR_SPACE *space)
{
    FIXME("iface %p, processor %p, space %p, stub!\n", iface, processor, space);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorGetOutputAlphaFillMode(ID3D11VideoContext *iface,
        ID3D11VideoProcessor *processor, D3D11_VIDEO_PROCESSOR_ALPHA_FILL_MODE *mode, UINT *stream_idx)
{
    FIXME("iface %p, processor %p, mode %p, stream_idx %p, stub!\n", iface, processor, mode, stream_idx);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorGetOutputConstriction(
        ID3D11VideoContext *iface, ID3D11VideoProcessor *processor, BOOL *enabled, SIZE *size)
{
    FIXME("iface %p, processor %p, enabled %p, size %p, stub!\n", iface, processor, enabled, size);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorGetOutputStereoMode(
        ID3D11VideoContext *iface, ID3D11VideoProcessor *processor, BOOL *enabled)
{
    FIXME("iface %p, processor %p, enabled %p, stub!\n", iface, processor, enabled);
}

static HRESULT STDMETHODCALLTYPE d3d11_video_context_VideoProcessorGetOutputExtension(
        ID3D11VideoContext *iface, ID3D11VideoProcessor *processor, const GUID *guid, UINT size, void *data)
{
    FIXME("iface %p, processor %p, guid %s, size %u, data %p, stub!\n",
            iface, processor, debugstr_guid(guid), size, data);
    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorSetStreamFrameFormat(ID3D11VideoContext *iface,
        ID3D11VideoProcessor *processor, UINT stream_idx, D3D11_VIDEO_FRAME_FORMAT format)
{
    FIXME("iface %p, processor %p, stream_idx %u, format %#x, stub!\n", iface, processor, stream_idx, format);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorSetStreamColorSpace(ID3D11VideoContext *iface,
        ID3D11VideoProcessor *processor, UINT stream_idx, const D3D11_VIDEO_PROCESSOR_COLOR_SPACE *space)
{
    FIXME("iface %p, processor %p, stream_idx %u, space %p, stub!\n", iface, processor, stream_idx, space);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorSetStreamOutputRate(
        ID3D11VideoContext *iface, ID3D11VideoProcessor *processor, UINT stream_idx,
        D3D11_VIDEO_PROCESSOR_OUTPUT_RATE rate, BOOL repeat, const DXGI_RATIONAL *custom_rate)
{
    FIXME("iface %p, processor %p, stream_idx %u, rate %#x, repeat %d, custom_rate %u/%u, stub!\n",
            iface, processor, stream_idx, rate, repeat, custom_rate->Numerator, custom_rate->Denominator);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorSetStreamSourceRect(ID3D11VideoContext *iface,
        ID3D11VideoProcessor *processor, UINT stream_idx, BOOL enable, const RECT *rect)
{
    FIXME("iface %p, processor %p, stream_idx %u, enable %d, rect %s, stub!\n",
            iface, processor, stream_idx, enable, wine_dbgstr_rect(rect));
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorSetStreamDestRect(ID3D11VideoContext *iface,
        ID3D11VideoProcessor *processor, UINT stream_idx, BOOL enable, const RECT *rect)
{
    FIXME("iface %p, processor %p, stream_idx %u, enable %d, rect %s, stub!\n",
            iface, processor, stream_idx, enable, wine_dbgstr_rect(rect));
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorSetStreamAlpha(ID3D11VideoContext *iface,
        ID3D11VideoProcessor *processor, UINT stream_idx, BOOL enable, float alpha)
{
    FIXME("iface %p, processor %p, stream_idx %u, enable %d, alpha %.8e, stub!\n",
            iface, processor, stream_idx, enable, alpha);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorSetStreamPalette(ID3D11VideoContext *iface,
        ID3D11VideoProcessor *processor, UINT stream_idx, UINT entry_count, const UINT *entries)
{
    FIXME("iface %p, processor %p, stream_idx %u, entry_count %u, entries %p, stub!\n",
            iface, processor, stream_idx, entry_count, entries);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorSetStreamPixelAspectRatio(
        ID3D11VideoContext *iface, ID3D11VideoProcessor *processor, UINT stream_idx,
        BOOL enable, const DXGI_RATIONAL *src_ratio, const DXGI_RATIONAL *dst_ratio)
{
    FIXME("iface %p, processor %p, stream_idx %u, enable %d, src_ratio %u/%u, dst_ratio %u/%u stub!\n",
            iface, processor, stream_idx, enable, src_ratio->Numerator, src_ratio->Denominator,
            dst_ratio->Numerator, dst_ratio->Denominator);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorSetStreamLumaKey(ID3D11VideoContext *iface,
        ID3D11VideoProcessor *processor, UINT stream_idx, BOOL enable, float lower, float upper)
{
    FIXME("iface %p, processor %p, stream_idx %u, enable %d, lower %.8e, upper %.8e, stub!\n",
            iface, processor, stream_idx, enable, lower, upper);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorSetStreamStereoFormat(ID3D11VideoContext *iface,
        ID3D11VideoProcessor *processor, UINT stream_idx, BOOL enable, D3D11_VIDEO_PROCESSOR_STEREO_FORMAT format,
        BOOL left_view_frame0, BOOL base_view_frame0, D3D11_VIDEO_PROCESSOR_STEREO_FLIP_MODE flip_mode, int mono_offset)
{
    FIXME("iface %p, processor %p, stream_idx %u, enable %d, format %#x,"
            " left_view_frame0 %d, base_view_frame0 %d, flip_mode %#x, mono_offset %d, stub!\n",
            iface, processor, stream_idx, enable, format, left_view_frame0, base_view_frame0, flip_mode, mono_offset);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorSetStreamAutoProcessingMode(
        ID3D11VideoContext *iface, ID3D11VideoProcessor *processor, UINT stream_idx, BOOL enable)
{
    FIXME("iface %p, processor %p, stream_idx %u, enable %d, stub!\n", iface, processor, stream_idx, enable);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorSetStreamFilter(ID3D11VideoContext *iface,
        ID3D11VideoProcessor *processor, UINT stream_idx, D3D11_VIDEO_PROCESSOR_FILTER filter, BOOL enable, int level)
{
    FIXME("iface %p, processor %p, stream_idx %u, filter %#x, enable %d, level %d, stub!\n",
            iface, processor, stream_idx, filter, enable, level);
}

static HRESULT STDMETHODCALLTYPE d3d11_video_context_VideoProcessorSetStreamExtension(ID3D11VideoContext *iface,
        ID3D11VideoProcessor *processor, UINT stream_idx, const GUID *guid, UINT size, void *data)
{
    FIXME("iface %p, processor %p, stream_idx %u, guid %s, size %u, data %p, stub!\n",
            iface, processor, stream_idx, debugstr_guid(guid), size, data);
    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorGetStreamFrameFormat(ID3D11VideoContext *iface,
        ID3D11VideoProcessor *processor, UINT stream_idx, D3D11_VIDEO_FRAME_FORMAT *format)
{
    FIXME("iface %p, processor %p, stream_idx %u, format %p, stub!\n", iface, processor, stream_idx, format);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorGetStreamColorSpace(ID3D11VideoContext *iface,
        ID3D11VideoProcessor *processor, UINT stream_idx, D3D11_VIDEO_PROCESSOR_COLOR_SPACE *space)
{
    FIXME("iface %p, processor %p, stream_idx %u, space %p, stub!\n", iface, processor, stream_idx, space);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorGetStreamOutputRate(
        ID3D11VideoContext *iface, ID3D11VideoProcessor *processor, UINT stream_idx,
        D3D11_VIDEO_PROCESSOR_OUTPUT_RATE *rate, BOOL *repeat, DXGI_RATIONAL *custom_rate)
{
    FIXME("iface %p, processor %p, stream_idx %u, rate %p, repeat %p, custom_rate %p, stub!\n",
            iface, processor, stream_idx, rate, repeat, custom_rate);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorGetStreamSourceRect(ID3D11VideoContext *iface,
        ID3D11VideoProcessor *processor, UINT stream_idx, BOOL *enabled, RECT *rect)
{
    FIXME("iface %p, processor %p, stream_idx %u, enabled %p, rect %p, stub!\n",
            iface, processor, stream_idx, enabled, rect);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorGetStreamDestRect(ID3D11VideoContext *iface,
        ID3D11VideoProcessor *processor, UINT stream_idx, BOOL *enabled, RECT *rect)
{
    FIXME("iface %p, processor %p, stream_idx %u, enabled %p, rect %p, stub!\n",
            iface, processor, stream_idx, enabled, rect);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorGetStreamAlpha(ID3D11VideoContext *iface,
        ID3D11VideoProcessor *processor, UINT stream_idx, BOOL *enabled, float *alpha)
{
    FIXME("iface %p, processor %p, stream_idx %u, enabled %p, alpha %p, stub!\n",
            iface, processor, stream_idx, enabled, alpha);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorGetStreamPalette(ID3D11VideoContext *iface,
        ID3D11VideoProcessor *processor, UINT stream_idx, UINT count, UINT *entries)
{
    FIXME("iface %p, processor %p, stream_idx %u, count %u, entries %p, stub!\n",
            iface, processor, stream_idx, count, entries);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorGetStreamPixelAspectRatio(
        ID3D11VideoContext *iface, ID3D11VideoProcessor *processor, UINT stream_idx,
        BOOL *enabled, DXGI_RATIONAL *src_ratio, DXGI_RATIONAL *dst_ratio)
{
    FIXME("iface %p, processor %p, stream_idx %u, enabled %p, src_ratio %p, dst_ratio %p, stub!\n",
            iface, processor, stream_idx, enabled, src_ratio, dst_ratio);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorGetStreamLumaKey(ID3D11VideoContext *iface,
        ID3D11VideoProcessor *processor, UINT stream_idx, BOOL *enabled, float *lower, float *upper)
{
    FIXME("iface %p, processor %p, stream_idx %u, enabled %p, lower %p, upper %p, stub!\n",
            iface, processor, stream_idx, enabled, lower, upper);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorGetStreamStereoFormat(
        ID3D11VideoContext *iface, ID3D11VideoProcessor *processor, UINT stream_idx, BOOL *enabled,
        D3D11_VIDEO_PROCESSOR_STEREO_FORMAT *format, BOOL *left_view_frame0,
        BOOL *base_view_frame0, D3D11_VIDEO_PROCESSOR_STEREO_FLIP_MODE *flip_mode, int *mono_offset)
{
    FIXME("iface %p, processor %p, stream_idx %u, enabled %p, format %p, left_view_frame0 %p,"
            " base_view_frame0 %p flip_mode %p, mono_offset %p, stub!\n",
            iface, processor, stream_idx, enabled, format, left_view_frame0, base_view_frame0, flip_mode, mono_offset);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorGetStreamAutoProcessingMode(
        ID3D11VideoContext *iface, ID3D11VideoProcessor *processor, UINT stream_idx, BOOL *enabled)
{
    FIXME("iface %p, processor %p, stream_idx %u, enabled %p, stub!\n", iface, processor, stream_idx, enabled);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorGetStreamFilter(
        ID3D11VideoContext *iface, ID3D11VideoProcessor *processor, UINT stream_idx,
        D3D11_VIDEO_PROCESSOR_FILTER filter, BOOL *enabled, int *level)
{
    FIXME("iface %p, processor %p, stream_idx %u, filter %#x, enabled %p, level %p, stub!\n",
            iface, processor, stream_idx, filter, enabled, level);
}

static HRESULT STDMETHODCALLTYPE d3d11_video_context_VideoProcessorGetStreamExtension(ID3D11VideoContext *iface,
        ID3D11VideoProcessor *processor, UINT stream_idx, const GUID *guid, UINT size, void *data)
{
    FIXME("iface %p, processor %p, stream_idx %u, guid %s, size %u, data %p, stub!\n",
            iface, processor, stream_idx, debugstr_guid(guid), size, data);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_context_VideoProcessorBlt(
        ID3D11VideoContext *iface, ID3D11VideoProcessor *processor, ID3D11VideoProcessorOutputView *view,
        UINT frame_idx, UINT stream_count, const D3D11_VIDEO_PROCESSOR_STREAM *streams)
{
    FIXME("iface %p, processor %p, view %p, frame_idx %u, stream_count %u, streams %p, stub!\n",
            iface, processor, view, frame_idx, stream_count, streams);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_context_NegotiateCryptoSessionKeyExchange(
        ID3D11VideoContext *iface, ID3D11CryptoSession *session, UINT size, void *data)
{
    FIXME("iface %p, session %p, size %u, data %p, stub!\n", iface, session, size, data);
    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d3d11_video_context_EncryptionBlt(
        ID3D11VideoContext *iface, ID3D11CryptoSession *session,
        ID3D11Texture2D *src_surface, ID3D11Texture2D *dst_surface, UINT iv_size, void *iv)
{
    FIXME("iface %p, session %p, src_surface %p, dst_surface %p, iv_size %u, iv %p, stub!\n",
            iface, session, src_surface, dst_surface, iv_size, iv);
}

static void STDMETHODCALLTYPE d3d11_video_context_DecryptionBlt(ID3D11VideoContext *iface,
        ID3D11CryptoSession *session, ID3D11Texture2D *src_surface, ID3D11Texture2D *dst_surface,
        D3D11_ENCRYPTED_BLOCK_INFO *block_info, UINT key_size, const void *key, UINT iv_size, void *iv)
{
    FIXME("iface %p, session %p, src_surface %p, dst_surface %p, block_info %p,"
            " key_size %u, key %p, iv_size %u, iv %p, stub!\n",
            iface, session, src_surface, dst_surface, block_info, key_size, key, iv_size, iv);
}

static void STDMETHODCALLTYPE d3d11_video_context_StartSessionKeyRefresh(ID3D11VideoContext *iface,
        ID3D11CryptoSession *session, UINT random_number_size, void *random_number)
{
    FIXME("iface %p, session %p, random_number_size %u, random_number %p, stub!\n",
            iface, session, random_number_size, random_number);
}

static void STDMETHODCALLTYPE d3d11_video_context_FinishSessionKeyRefresh(
        ID3D11VideoContext *iface, ID3D11CryptoSession *session)
{
    FIXME("iface %p, session %p, stub!\n", iface, session);
}

static HRESULT STDMETHODCALLTYPE d3d11_video_context_GetEncryptionBltKey(
        ID3D11VideoContext *iface, ID3D11CryptoSession *session, UINT size, void *key)
{
    FIXME("iface %p, session %p, size %u, key %p, stub!\n", iface, session, size, key);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_context_NegotiateAuthenticatedChannelKeyExchange(
        ID3D11VideoContext *iface, ID3D11AuthenticatedChannel *channel, UINT size, void *data)
{
    FIXME("iface %p, channel %p, size %u, data %p, stub!\n", iface, channel, size, data);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_context_QueryAuthenticatedChannel(ID3D11VideoContext *iface,
        ID3D11AuthenticatedChannel *channel, UINT input_size, const void *input, UINT output_size, void *output)
{
    FIXME("iface %p, channel %p, input_size %u, input %p, output_size %u, output %p stub!\n",
            iface, channel, input_size, input, output_size, output);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_context_ConfigureAuthenticatedChannel(
        ID3D11VideoContext *iface, ID3D11AuthenticatedChannel *channel,
        UINT input_size, const void *input, D3D11_AUTHENTICATED_CONFIGURE_OUTPUT *output)
{
    FIXME("iface %p, channel %p, input_size %u, input %p, output %p, stub!\n",
            iface, channel, input_size, input, output);
    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorSetStreamRotation(ID3D11VideoContext *iface,
        ID3D11VideoProcessor *processor, UINT stream_idx, BOOL enable, D3D11_VIDEO_PROCESSOR_ROTATION rotation)
{
    FIXME("iface %p, processor %p, stream_idx %u, enable %d, rotation %#x, stub!\n",
            iface, processor, stream_idx, enable, rotation);
}

static void STDMETHODCALLTYPE d3d11_video_context_VideoProcessorGetStreamRotation(ID3D11VideoContext *iface,
        ID3D11VideoProcessor *processor, UINT stream_idx, BOOL *enable, D3D11_VIDEO_PROCESSOR_ROTATION *rotation)
{
    FIXME("iface %p, processor %p, stream_idx %u, enable %p, rotation %p, stub!\n",
            iface, processor, stream_idx, enable, rotation);
}

static const ID3D11VideoContextVtbl d3d11_video_context_vtbl =
{
    d3d11_video_context_QueryInterface,
    d3d11_video_context_AddRef,
    d3d11_video_context_Release,
    d3d11_video_context_GetDevice,
    d3d11_video_context_GetPrivateData,
    d3d11_video_context_SetPrivateData,
    d3d11_video_context_SetPrivateDataInterface,
    d3d11_video_context_GetDecoderBuffer,
    d3d11_video_context_ReleaseDecoderBuffer,
    d3d11_video_context_DecoderBeginFrame,
    d3d11_video_context_DecoderEndFrame,
    d3d11_video_context_SubmitDecoderBuffers,
    d3d11_video_context_DecoderExtension,
    d3d11_video_context_VideoProcessorSetOutputTargetRect,
    d3d11_video_context_VideoProcessorSetOutputBackgroundColor,
    d3d11_video_context_VideoProcessorSetOutputColorSpace,
    d3d11_video_context_VideoProcessorSetOutputAlphaFillMode,
    d3d11_video_context_VideoProcessorSetOutputConstriction,
    d3d11_video_context_VideoProcessorSetOutputStereoMode,
    d3d11_video_context_VideoProcessorSetOutputExtension,
    d3d11_video_context_VideoProcessorGetOutputTargetRect,
    d3d11_video_context_VideoProcessorGetOutputBackgroundColor,
    d3d11_video_context_VideoProcessorGetOutputColorSpace,
    d3d11_video_context_VideoProcessorGetOutputAlphaFillMode,
    d3d11_video_context_VideoProcessorGetOutputConstriction,
    d3d11_video_context_VideoProcessorGetOutputStereoMode,
    d3d11_video_context_VideoProcessorGetOutputExtension,
    d3d11_video_context_VideoProcessorSetStreamFrameFormat,
    d3d11_video_context_VideoProcessorSetStreamColorSpace,
    d3d11_video_context_VideoProcessorSetStreamOutputRate,
    d3d11_video_context_VideoProcessorSetStreamSourceRect,
    d3d11_video_context_VideoProcessorSetStreamDestRect,
    d3d11_video_context_VideoProcessorSetStreamAlpha,
    d3d11_video_context_VideoProcessorSetStreamPalette,
    d3d11_video_context_VideoProcessorSetStreamPixelAspectRatio,
    d3d11_video_context_VideoProcessorSetStreamLumaKey,
    d3d11_video_context_VideoProcessorSetStreamStereoFormat,
    d3d11_video_context_VideoProcessorSetStreamAutoProcessingMode,
    d3d11_video_context_VideoProcessorSetStreamFilter,
    d3d11_video_context_VideoProcessorSetStreamExtension,
    d3d11_video_context_VideoProcessorGetStreamFrameFormat,
    d3d11_video_context_VideoProcessorGetStreamColorSpace,
    d3d11_video_context_VideoProcessorGetStreamOutputRate,
    d3d11_video_context_VideoProcessorGetStreamSourceRect,
    d3d11_video_context_VideoProcessorGetStreamDestRect,
    d3d11_video_context_VideoProcessorGetStreamAlpha,
    d3d11_video_context_VideoProcessorGetStreamPalette,
    d3d11_video_context_VideoProcessorGetStreamPixelAspectRatio,
    d3d11_video_context_VideoProcessorGetStreamLumaKey,
    d3d11_video_context_VideoProcessorGetStreamStereoFormat,
    d3d11_video_context_VideoProcessorGetStreamAutoProcessingMode,
    d3d11_video_context_VideoProcessorGetStreamFilter,
    d3d11_video_context_VideoProcessorGetStreamExtension,
    d3d11_video_context_VideoProcessorBlt,
    d3d11_video_context_NegotiateCryptoSessionKeyExchange,
    d3d11_video_context_EncryptionBlt,
    d3d11_video_context_DecryptionBlt,
    d3d11_video_context_StartSessionKeyRefresh,
    d3d11_video_context_FinishSessionKeyRefresh,
    d3d11_video_context_GetEncryptionBltKey,
    d3d11_video_context_NegotiateAuthenticatedChannelKeyExchange,
    d3d11_video_context_QueryAuthenticatedChannel,
    d3d11_video_context_ConfigureAuthenticatedChannel,
    d3d11_video_context_VideoProcessorSetStreamRotation,
    d3d11_video_context_VideoProcessorGetStreamRotation,
};

/* ID3D11Multithread methods */

static inline struct d3d11_device_context *impl_from_ID3D11Multithread(ID3D11Multithread *iface)
{
    return CONTAINING_RECORD(iface, struct d3d11_device_context, ID3D11Multithread_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_multithread_QueryInterface(ID3D11Multithread *iface,
        REFIID iid, void **out)
{
    struct d3d11_device_context *context = impl_from_ID3D11Multithread(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    return d3d11_device_context_QueryInterface(&context->ID3D11DeviceContext1_iface, iid, out);
}

static ULONG STDMETHODCALLTYPE d3d11_multithread_AddRef(ID3D11Multithread *iface)
{
    struct d3d11_device_context *context = impl_from_ID3D11Multithread(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_device_context_AddRef(&context->ID3D11DeviceContext1_iface);
}

static ULONG STDMETHODCALLTYPE d3d11_multithread_Release(ID3D11Multithread *iface)
{
    struct d3d11_device_context *context = impl_from_ID3D11Multithread(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_device_context_Release(&context->ID3D11DeviceContext1_iface);
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

/* ID3DUserDefinedAnnotation methods */

static inline struct d3d11_device_context *impl_from_ID3DUserDefinedAnnotation(ID3DUserDefinedAnnotation *iface)
{
    return CONTAINING_RECORD(iface, struct d3d11_device_context, ID3DUserDefinedAnnotation_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_user_defined_annotation_QueryInterface(ID3DUserDefinedAnnotation *iface,
        REFIID iid, void **out)
{
    struct d3d11_device_context *context = impl_from_ID3DUserDefinedAnnotation(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    return d3d11_device_context_QueryInterface(&context->ID3D11DeviceContext1_iface, iid, out);
}

static ULONG STDMETHODCALLTYPE d3d11_user_defined_annotation_AddRef(ID3DUserDefinedAnnotation *iface)
{
    struct d3d11_device_context *context = impl_from_ID3DUserDefinedAnnotation(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_device_context_AddRef(&context->ID3D11DeviceContext1_iface);
}

static ULONG STDMETHODCALLTYPE d3d11_user_defined_annotation_Release(ID3DUserDefinedAnnotation *iface)
{
    struct d3d11_device_context *context = impl_from_ID3DUserDefinedAnnotation(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_device_context_Release(&context->ID3D11DeviceContext1_iface);
}

static int STDMETHODCALLTYPE d3d11_user_defined_annotation_BeginEvent(ID3DUserDefinedAnnotation *iface,
        const WCHAR *name)
{
    TRACE("iface %p, name %s.\n", iface, debugstr_w(name));

    return -1;
}

static int STDMETHODCALLTYPE d3d11_user_defined_annotation_EndEvent(ID3DUserDefinedAnnotation *iface)
{
    TRACE("iface %p.\n", iface);

    return -1;
}

static void STDMETHODCALLTYPE d3d11_user_defined_annotation_SetMarker(ID3DUserDefinedAnnotation *iface,
        const WCHAR *name)
{
    TRACE("iface %p, name %s.\n", iface, debugstr_w(name));
}

static BOOL STDMETHODCALLTYPE d3d11_user_defined_annotation_GetStatus(ID3DUserDefinedAnnotation *iface)
{
    TRACE("iface %p.\n", iface);

    return FALSE;
}

static const struct ID3DUserDefinedAnnotationVtbl d3d11_user_defined_annotation_vtbl =
{
    d3d11_user_defined_annotation_QueryInterface,
    d3d11_user_defined_annotation_AddRef,
    d3d11_user_defined_annotation_Release,
    d3d11_user_defined_annotation_BeginEvent,
    d3d11_user_defined_annotation_EndEvent,
    d3d11_user_defined_annotation_SetMarker,
    d3d11_user_defined_annotation_GetStatus,
};

static void d3d11_device_context_init(struct d3d11_device_context *context, struct d3d_device *device,
        D3D11_DEVICE_CONTEXT_TYPE type)
{
    context->ID3D11DeviceContext1_iface.lpVtbl = &d3d11_device_context_vtbl;
    context->ID3D11Multithread_iface.lpVtbl = &d3d11_multithread_vtbl;
    context->ID3D11VideoContext_iface.lpVtbl = &d3d11_video_context_vtbl;
    context->ID3DUserDefinedAnnotation_iface.lpVtbl = &d3d11_user_defined_annotation_vtbl;
    context->refcount = 1;
    context->type = type;

    context->device = device;
    ID3D11Device2_AddRef(&device->ID3D11Device2_iface);

    wined3d_private_store_init(&context->private_store);
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

    if (FAILED(hr = d3d_texture2d_create(device, desc, NULL, data, &object)))
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

    *view = NULL;

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

    *view = NULL;

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

    *view = NULL;

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

    *view = NULL;

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

    TRACE("iface %p, element_descs %p, element_count %u, shader_byte_code %p, shader_byte_code_length %Iu, "
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

    TRACE("iface %p, byte_code %p, byte_code_length %Iu, class_linkage %p, shader %p.\n",
            iface, byte_code, byte_code_length, class_linkage, shader);

    *shader = NULL;

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

    TRACE("iface %p, byte_code %p, byte_code_length %Iu, class_linkage %p, shader %p.\n",
            iface, byte_code, byte_code_length, class_linkage, shader);

    *shader = NULL;

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

    TRACE("iface %p, byte_code %p, byte_code_length %Iu, so_entries %p, entry_count %u, "
            "buffer_strides %p, strides_count %u, rasterizer_stream %u, class_linkage %p, shader %p.\n",
            iface, byte_code, byte_code_length, so_entries, entry_count, buffer_strides, strides_count,
            rasterizer_stream, class_linkage, shader);

    *shader = NULL;

    if (class_linkage)
        FIXME("Class linkage is not implemented yet.\n");

    if (FAILED(hr = d3d_geometry_shader_create(device, byte_code, byte_code_length,
            so_entries, entry_count, buffer_strides, strides_count, rasterizer_stream, &object)))
    {
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

    TRACE("iface %p, byte_code %p, byte_code_length %Iu, class_linkage %p, shader %p.\n",
            iface, byte_code, byte_code_length, class_linkage, shader);

    *shader = NULL;

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

    TRACE("iface %p, byte_code %p, byte_code_length %Iu, class_linkage %p, shader %p.\n",
            iface, byte_code, byte_code_length, class_linkage, shader);

    *shader = NULL;

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

    TRACE("iface %p, byte_code %p, byte_code_length %Iu, class_linkage %p, shader %p.\n",
            iface, byte_code, byte_code_length, class_linkage, shader);

    *shader = NULL;

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

    TRACE("iface %p, byte_code %p, byte_code_length %Iu, class_linkage %p, shader %p.\n",
            iface, byte_code, byte_code_length, class_linkage, shader);

    *shader = NULL;

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

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateBlendState1(ID3D11Device2 *iface,
        const D3D11_BLEND_DESC1 *desc, ID3D11BlendState1 **state)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d_blend_state *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, state %p.\n", iface, desc, state);

    if (FAILED(hr = d3d_blend_state_create(device, desc, &object)))
        return hr;

    *state = &object->ID3D11BlendState1_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateBlendState(ID3D11Device2 *iface,
        const D3D11_BLEND_DESC *desc, ID3D11BlendState **blend_state)
{
    D3D11_BLEND_DESC1 d3d11_1_desc;
    unsigned int i;

    TRACE("iface %p, desc %p, blend_state %p.\n", iface, desc, blend_state);

    if (!desc)
        return E_INVALIDARG;

    d3d11_1_desc.AlphaToCoverageEnable = desc->AlphaToCoverageEnable;
    d3d11_1_desc.IndependentBlendEnable = desc->IndependentBlendEnable;
    for (i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        d3d11_1_desc.RenderTarget[i].BlendEnable = desc->RenderTarget[i].BlendEnable;
        d3d11_1_desc.RenderTarget[i].LogicOpEnable = FALSE;
        d3d11_1_desc.RenderTarget[i].SrcBlend = desc->RenderTarget[i].SrcBlend;
        d3d11_1_desc.RenderTarget[i].DestBlend = desc->RenderTarget[i].DestBlend;
        d3d11_1_desc.RenderTarget[i].BlendOp = desc->RenderTarget[i].BlendOp;
        d3d11_1_desc.RenderTarget[i].SrcBlendAlpha = desc->RenderTarget[i].SrcBlendAlpha;
        d3d11_1_desc.RenderTarget[i].DestBlendAlpha = desc->RenderTarget[i].DestBlendAlpha;
        d3d11_1_desc.RenderTarget[i].BlendOpAlpha = desc->RenderTarget[i].BlendOpAlpha;
        d3d11_1_desc.RenderTarget[i].LogicOp = D3D11_LOGIC_OP_COPY;
        d3d11_1_desc.RenderTarget[i].RenderTargetWriteMask = desc->RenderTarget[i].RenderTargetWriteMask;
    }

    return d3d11_device_CreateBlendState1(iface, &d3d11_1_desc, (ID3D11BlendState1 **)blend_state);
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
    D3D11_RASTERIZER_DESC1 desc1;
    HRESULT hr;

    TRACE("iface %p, desc %p, rasterizer_state %p.\n", iface, desc, rasterizer_state);

    if (!desc)
        return E_INVALIDARG;

    memcpy(&desc1, desc, sizeof(*desc));
    desc1.ForcedSampleCount = 0;

    if (FAILED(hr = d3d_rasterizer_state_create(device, &desc1, &object)))
        return hr;

    *rasterizer_state = (ID3D11RasterizerState *)&object->ID3D11RasterizerState1_iface;

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

static HRESULT d3d11_deferred_context_create(struct d3d_device *device,
        UINT flags, struct d3d11_device_context **context)
{
    struct d3d11_device_context *object;
    HRESULT hr;

    if (flags)
        FIXME("Ignoring flags %#x.\n", flags);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;
    d3d11_device_context_init(object, device, D3D11_DEVICE_CONTEXT_DEFERRED);

    if (FAILED(hr = wined3d_deferred_context_create(device->wined3d_device, &object->wined3d_context)))
    {
        WARN("Failed to create wined3d deferred context, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created deferred context %p.\n", object);
    *context = object;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateDeferredContext(ID3D11Device2 *iface, UINT flags,
        ID3D11DeviceContext **context)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d11_device_context *object;
    HRESULT hr;

    TRACE("iface %p, flags %#x, context %p.\n", iface, flags, context);

    if (FAILED(hr = d3d11_deferred_context_create(device, flags, &object)))
        return hr;

    *context = (ID3D11DeviceContext *)&object->ID3D11DeviceContext1_iface;
    return S_OK;
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
    struct wined3d_adapter *wined3d_adapter;
    enum wined3d_format_id wined3d_format;
    D3D_FEATURE_LEVEL feature_level;
    struct wined3d *wined3d;
    unsigned int i;

    static const struct
    {
        enum wined3d_resource_type rtype;
        unsigned int bind_flags;
        unsigned int usage;
        D3D11_FORMAT_SUPPORT flag;
    }
    flag_mapping[] =
    {
        {WINED3D_RTYPE_BUFFER,     WINED3D_BIND_SHADER_RESOURCE, 0, D3D11_FORMAT_SUPPORT_BUFFER},
        {WINED3D_RTYPE_BUFFER,     WINED3D_BIND_VERTEX_BUFFER,   0, D3D11_FORMAT_SUPPORT_IA_VERTEX_BUFFER},
        {WINED3D_RTYPE_BUFFER,     WINED3D_BIND_INDEX_BUFFER,    0, D3D11_FORMAT_SUPPORT_IA_INDEX_BUFFER},
        {WINED3D_RTYPE_TEXTURE_1D, WINED3D_BIND_SHADER_RESOURCE, 0, D3D11_FORMAT_SUPPORT_TEXTURE1D},
        {WINED3D_RTYPE_TEXTURE_2D, WINED3D_BIND_SHADER_RESOURCE, 0, D3D11_FORMAT_SUPPORT_TEXTURE2D},
        {WINED3D_RTYPE_TEXTURE_3D, WINED3D_BIND_SHADER_RESOURCE, 0, D3D11_FORMAT_SUPPORT_TEXTURE3D},
        {WINED3D_RTYPE_NONE,       WINED3D_BIND_RENDER_TARGET,   0, D3D11_FORMAT_SUPPORT_RENDER_TARGET},
        {WINED3D_RTYPE_NONE,       WINED3D_BIND_DEPTH_STENCIL,   0, D3D11_FORMAT_SUPPORT_DEPTH_STENCIL},
        {WINED3D_RTYPE_NONE,       WINED3D_BIND_UNORDERED_ACCESS, 0, D3D11_FORMAT_SUPPORT_TYPED_UNORDERED_ACCESS_VIEW},
        {WINED3D_RTYPE_TEXTURE_2D, WINED3D_BIND_SHADER_RESOURCE, WINED3DUSAGE_QUERY_WRAPANDMIP, D3D11_FORMAT_SUPPORT_MIP},
        {WINED3D_RTYPE_TEXTURE_2D, WINED3D_BIND_SHADER_RESOURCE, WINED3DUSAGE_QUERY_GENMIPMAP, D3D11_FORMAT_SUPPORT_MIP_AUTOGEN},
        {WINED3D_RTYPE_TEXTURE_2D, WINED3D_BIND_RENDER_TARGET, WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING, D3D11_FORMAT_SUPPORT_BLENDABLE},
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
    feature_level = device->state->feature_level;
    wined3d = wined3d_device_get_wined3d(device->wined3d_device);
    wined3d_device_get_creation_parameters(device->wined3d_device, &params);
    wined3d_adapter = wined3d_get_adapter(wined3d, params.adapter_idx);
    for (i = 0; i < ARRAY_SIZE(flag_mapping); ++i)
    {
        hr = wined3d_check_device_format(wined3d, wined3d_adapter, params.device_type,
                WINED3DFMT_UNKNOWN, flag_mapping[i].usage, flag_mapping[i].bind_flags, flag_mapping[i].rtype, wined3d_format);
        if (hr == WINED3DERR_NOTAVAILABLE || hr == WINED3DOK_NOMIPGEN)
            continue;
        if (hr != WINED3D_OK)
        {
            WARN("Failed to check device format support, hr %#lx.\n", hr);
            wined3d_mutex_unlock();
            return E_FAIL;
        }

        *format_support |= flag_mapping[i].flag;
    }
    wined3d_mutex_unlock();

    if (feature_level < D3D_FEATURE_LEVEL_10_0)
        *format_support &= ~D3D11_FORMAT_SUPPORT_BUFFER;

    if (*format_support & (D3D11_FORMAT_SUPPORT_TEXTURE1D
            | D3D11_FORMAT_SUPPORT_TEXTURE2D | D3D11_FORMAT_SUPPORT_TEXTURE3D))
    {
        *format_support |= D3D11_FORMAT_SUPPORT_SHADER_LOAD;
        *format_support |= D3D11_FORMAT_SUPPORT_SHADER_SAMPLE;
        *format_support |= D3D11_FORMAT_SUPPORT_TEXTURECUBE;

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

    /* d3d11 requires 4 and 8 sample counts support for formats reported to
     * support multisample. */
    if (wined3d_check_device_multisample_type(wined3d_adapter, params.device_type, wined3d_format,
            TRUE, WINED3D_MULTISAMPLE_4_SAMPLES, NULL) == WINED3D_OK &&
            wined3d_check_device_multisample_type(wined3d_adapter, params.device_type, wined3d_format,
            TRUE, WINED3D_MULTISAMPLE_8_SAMPLES, NULL) == WINED3D_OK)
    {
        *format_support |= D3D11_FORMAT_SUPPORT_MULTISAMPLE_RESOLVE
                | D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET
                | D3D11_FORMAT_SUPPORT_MULTISAMPLE_LOAD;
    }

    return *format_support ? S_OK : E_FAIL;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CheckMultisampleQualityLevels(ID3D11Device2 *iface,
        DXGI_FORMAT format, UINT sample_count, UINT *quality_level_count)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct wined3d_device_creation_parameters params;
    struct wined3d_adapter *wined3d_adapter;
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
    wined3d_adapter = wined3d_get_adapter(wined3d, params.adapter_idx);
    hr = wined3d_check_device_multisample_type(wined3d_adapter, params.device_type,
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
                WARN("Failed to get device caps, hr %#lx.\n", hr);
                return hr;
            }

            doubles_data->DoublePrecisionFloatShaderOps = wined3d_caps.shader_double_precision;
            return S_OK;
        }

        case D3D11_FEATURE_D3D9_OPTIONS:
        {
            D3D11_FEATURE_DATA_D3D9_OPTIONS *options = feature_support_data;
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
                WARN("Failed to get device caps, hr %#lx.\n", hr);
                return hr;
            }

            options->FullNonPow2TextureSupport = !(wined3d_caps.TextureCaps & WINED3DPTEXTURECAPS_POW2);
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

            wined3d_mutex_lock();
            hr = wined3d_device_get_device_caps(device->wined3d_device, &wined3d_caps);
            wined3d_mutex_unlock();
            if (FAILED(hr))
            {
                WARN("Failed to get device caps, hr %#lx.\n", hr);
                return hr;
            }

            options->ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x
                    = wined3d_caps.max_feature_level >= WINED3D_FEATURE_LEVEL_11;
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
            options->ConstantBufferPartialUpdate = TRUE;
            options->ConstantBufferOffsetting = TRUE;
            options->MapNoOverwriteOnDynamicConstantBuffer = TRUE;
            options->MapNoOverwriteOnDynamicBufferSRV = TRUE;
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

        case D3D11_FEATURE_D3D11_OPTIONS2:
        {
            D3D11_FEATURE_DATA_D3D11_OPTIONS2 *options = feature_support_data;
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
                WARN("Failed to get device caps, hr %#lx.\n", hr);
                return hr;
            }

            options->PSSpecifiedStencilRefSupported = wined3d_caps.stencil_export;
            options->TypedUAVLoadAdditionalFormats = FALSE;
            options->ROVsSupported = FALSE;
            options->ConservativeRasterizationTier = D3D11_CONSERVATIVE_RASTERIZATION_NOT_SUPPORTED;
            options->TiledResourcesTier = D3D11_TILED_RESOURCES_NOT_SUPPORTED;
            options->MapOnDefaultTextures = FALSE;
            options->StandardSwizzle = FALSE;
            options->UnifiedMemoryArchitecture = FALSE;
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
                WARN("Failed to get device caps, hr %#lx.\n", hr);
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

        case D3D11_FEATURE_FORMAT_SUPPORT:
        {
            D3D11_FEATURE_DATA_FORMAT_SUPPORT *data = feature_support_data;
            if (feature_support_data_size != sizeof(*data))
            {
                WARN("Invalid size %u for D3D11_FEATURE_FORMAT_SUPPORT.\n", feature_support_data_size);
                return E_INVALIDARG;
            }

            return d3d11_device_CheckFormatSupport(iface, data->InFormat, &data->OutFormatSupport);
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

    return device->state->feature_level;
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
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d11_device_context *object;
    HRESULT hr;

    TRACE("iface %p, flags %#x, context %p.\n", iface, flags, context);

    if (FAILED(hr = d3d11_deferred_context_create(device, flags, &object)))
        return hr;

    *context = &object->ID3D11DeviceContext1_iface;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateRasterizerState1(ID3D11Device2 *iface,
        const D3D11_RASTERIZER_DESC1 *desc, ID3D11RasterizerState1 **state)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d_rasterizer_state *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, state %p.\n", iface, desc, state);

    if (!desc)
        return E_INVALIDARG;

    if (FAILED(hr = d3d_rasterizer_state_create(device, desc, &object)))
        return hr;

    *state = &object->ID3D11RasterizerState1_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_device_CreateDeviceContextState(ID3D11Device2 *iface, UINT flags,
        const D3D_FEATURE_LEVEL *feature_levels, UINT feature_level_count, UINT sdk_version,
        REFIID emulated_interface, D3D_FEATURE_LEVEL *chosen_feature_level, ID3DDeviceContextState **state)
{
    struct d3d_device *device = impl_from_ID3D11Device2(iface);
    struct d3d_device_context_state *state_impl;
    struct wined3d_state *wined3d_state;
    D3D_FEATURE_LEVEL feature_level;
    HRESULT hr = E_INVALIDARG;

    TRACE("iface %p, flags %#x, feature_levels %p, feature_level_count %u, "
            "sdk_version %u, emulated_interface %s, chosen_feature_level %p, state %p.\n",
            iface, flags, feature_levels, feature_level_count,
            sdk_version, debugstr_guid(emulated_interface), chosen_feature_level, state);

    if (flags)
        FIXME("Ignoring flags %#x.\n", flags);

    wined3d_mutex_lock();

    if (!feature_level_count)
        goto fail;

    if (FAILED(hr = wined3d_state_create(device->wined3d_device,
            (const enum wined3d_feature_level *)feature_levels, feature_level_count, &wined3d_state)))
        goto fail;
    feature_level = d3d_feature_level_from_wined3d(wined3d_state_get_feature_level(wined3d_state));

    if (chosen_feature_level)
        *chosen_feature_level = feature_level;

    if (!state)
    {
        wined3d_state_destroy(wined3d_state);
        wined3d_mutex_unlock();
        return S_FALSE;
    }

    if (!(state_impl = calloc(1, sizeof(*state_impl))))
    {
        wined3d_state_destroy(wined3d_state);
        hr = E_OUTOFMEMORY;
        goto fail;
    }

    d3d_device_context_state_init(state_impl, device, feature_level, emulated_interface);
    if (!d3d_device_context_state_add_entry(state_impl, device, wined3d_state))
    {
        wined3d_state_destroy(wined3d_state);
        ID3DDeviceContextState_Release(&state_impl->ID3DDeviceContextState_iface);
        hr = E_FAIL;
        goto fail;
    }

    *state = &state_impl->ID3DDeviceContextState_iface;
    device->d3d11_only = FALSE;
    wined3d_mutex_unlock();

    return S_OK;

fail:
    wined3d_mutex_unlock();
    if (chosen_feature_level)
        *chosen_feature_level = 0;
    if (state)
        *state = NULL;
    return hr;
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
    FIXME("iface %p, name %s, access %#lx, iid %s, resource %p stub!\n", iface, debugstr_w(name), access,
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
    else if (!device->d3d11_only
            && (IsEqualGUID(riid, &IID_ID3D10Device1)
            || IsEqualGUID(riid, &IID_ID3D10Device)))
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
    else if (IsEqualGUID(riid, &IID_ID3D11VideoDevice) || IsEqualGUID(riid, &IID_ID3D11VideoDevice1))
    {
        *out = &device->ID3D11VideoDevice1_iface;
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

    TRACE("%p increasing refcount to %lu.\n", device, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d_device_inner_Release(IUnknown *iface)
{
    struct d3d_device *device = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&device->refcount);
    unsigned int i;

    TRACE("%p decreasing refcount to %lu.\n", device, refcount);

    if (!refcount)
    {
        if (device->state)
            d3d_device_context_state_private_release(device->state);
        for (i = 0; i < device->context_state_count; ++i)
        {
            d3d_device_context_state_remove_entry(device->context_states[i], device);
        }
        free(device->context_states);
        d3d11_device_context_cleanup(&device->immediate_context);
        if (device->wined3d_device)
        {
            wined3d_device_decref(device->wined3d_device);
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
        struct wined3d_constant_buffer_state state;
        struct d3d_buffer *buffer_impl;

        wined3d_device_context_get_constant_buffer(device->immediate_context.wined3d_context,
                type, start_slot + i, &state);

        if (!state.buffer)
        {
            buffers[i] = NULL;
            continue;
        }

        buffer_impl = wined3d_buffer_get_parent(state.buffer);
        buffers[i] = &buffer_impl->ID3D10Buffer_iface;
        ID3D10Buffer_AddRef(buffers[i]);
    }
    wined3d_mutex_unlock();
}

static void d3d10_device_set_constant_buffers(ID3D10Device1 *iface, enum wined3d_shader_type type,
        unsigned int start_slot, unsigned int buffer_count, ID3D10Buffer *const *buffers)
{
    struct wined3d_constant_buffer_state wined3d_buffers[D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    unsigned int i;

    if (buffer_count > ARRAY_SIZE(wined3d_buffers))
    {
        WARN("Buffer count %u exceeds limit; ignoring call.\n", buffer_count);
        return;
    }

    for (i = 0; i < buffer_count; ++i)
    {
        struct d3d_buffer *buffer = unsafe_impl_from_ID3D10Buffer(buffers[i]);

        wined3d_buffers[i].buffer = buffer ? buffer->wined3d_buffer : NULL;
        wined3d_buffers[i].offset = 0;
        wined3d_buffers[i].size = WINED3D_MAX_CONSTANT_BUFFER_SIZE * sizeof(struct wined3d_vec4);
    }

    wined3d_device_context_set_constant_buffers(device->immediate_context.wined3d_context,
            type, start_slot, buffer_count, wined3d_buffers);
}

static void d3d10_device_set_shader_resource_views(ID3D10Device1 *iface, enum wined3d_shader_type type,
        unsigned int start_slot, unsigned int count, ID3D10ShaderResourceView *const *views)
{
    struct wined3d_shader_resource_view *wined3d_views[D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    unsigned int i;

    if (count > ARRAY_SIZE(wined3d_views))
    {
        WARN("View count %u exceeds limit; ignoring call.\n", count);
        return;
    }

    for (i = 0; i < count; ++i)
    {
        struct d3d_shader_resource_view *view = unsafe_impl_from_ID3D10ShaderResourceView(views[i]);

        wined3d_views[i] = view ? view->wined3d_view : NULL;
    }

    wined3d_device_context_set_shader_resource_views(device->immediate_context.wined3d_context,
            type, start_slot, count, wined3d_views);
}

static void d3d10_device_set_samplers(ID3D10Device1 *iface, enum wined3d_shader_type type,
        unsigned int start_slot, unsigned int count, ID3D10SamplerState *const *samplers)
{
    struct wined3d_sampler *wined3d_samplers[D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT];
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    unsigned int i;

    if (count > ARRAY_SIZE(wined3d_samplers))
    {
        WARN("Sampler count %u exceeds limit; ignoring call.\n", count);
        return;
    }

    for (i = 0; i < count; ++i)
    {
        struct d3d_sampler_state *sampler = unsafe_impl_from_ID3D10SamplerState(samplers[i]);

        wined3d_samplers[i] = sampler ? sampler->wined3d_sampler : NULL;
    }

    wined3d_device_context_set_samplers(device->immediate_context.wined3d_context,
            type, start_slot, count, wined3d_samplers);
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
    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n",
            iface, start_slot, view_count, views);

    d3d10_device_set_shader_resource_views(iface, WINED3D_SHADER_TYPE_PIXEL, start_slot, view_count, views);
}

static void STDMETHODCALLTYPE d3d10_device_PSSetShader(ID3D10Device1 *iface,
        ID3D10PixelShader *shader)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_pixel_shader *ps = unsafe_impl_from_ID3D10PixelShader(shader);

    TRACE("iface %p, shader %p\n", iface, shader);

    wined3d_device_context_set_shader(device->immediate_context.wined3d_context,
            WINED3D_SHADER_TYPE_PIXEL, ps ? ps->wined3d_shader : NULL);
}

static void STDMETHODCALLTYPE d3d10_device_PSSetSamplers(ID3D10Device1 *iface,
        UINT start_slot, UINT sampler_count, ID3D10SamplerState *const *samplers)
{
    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    d3d10_device_set_samplers(iface, WINED3D_SHADER_TYPE_PIXEL, start_slot, sampler_count, samplers);
}

static void STDMETHODCALLTYPE d3d10_device_VSSetShader(ID3D10Device1 *iface,
        ID3D10VertexShader *shader)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_vertex_shader *vs = unsafe_impl_from_ID3D10VertexShader(shader);

    TRACE("iface %p, shader %p\n", iface, shader);

    wined3d_device_context_set_shader(device->immediate_context.wined3d_context,
            WINED3D_SHADER_TYPE_VERTEX, vs ? vs->wined3d_shader : NULL);
}

static void STDMETHODCALLTYPE d3d10_device_DrawIndexed(ID3D10Device1 *iface, UINT index_count,
        UINT start_index_location, INT base_vertex_location)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);

    TRACE("iface %p, index_count %u, start_index_location %u, base_vertex_location %d.\n",
            iface, index_count, start_index_location, base_vertex_location);

    wined3d_device_context_draw_indexed(device->immediate_context.wined3d_context,
            base_vertex_location, start_index_location, index_count, 0, 0);
}

static void STDMETHODCALLTYPE d3d10_device_Draw(ID3D10Device1 *iface, UINT vertex_count,
        UINT start_vertex_location)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);

    TRACE("iface %p, vertex_count %u, start_vertex_location %u\n",
            iface, vertex_count, start_vertex_location);

    wined3d_device_context_draw(device->immediate_context.wined3d_context, start_vertex_location, vertex_count, 0, 0);
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

    wined3d_device_context_set_vertex_declaration(device->immediate_context.wined3d_context,
            layout ? layout->wined3d_decl : NULL);
}

static void STDMETHODCALLTYPE d3d10_device_IASetVertexBuffers(ID3D10Device1 *iface, UINT start_slot,
        UINT buffer_count, ID3D10Buffer *const *buffers, const UINT *strides, const UINT *offsets)
{
    struct wined3d_stream_state streams[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    unsigned int i;

    TRACE("iface %p, start_slot %u, buffer_count %u, buffers %p, strides %p, offsets %p\n",
            iface, start_slot, buffer_count, buffers, strides, offsets);

    if (buffer_count > ARRAY_SIZE(streams))
    {
        WARN("Buffer count %u exceeds limit.\n", buffer_count);
        buffer_count = ARRAY_SIZE(streams);
    }

    for (i = 0; i < buffer_count; ++i)
    {
        struct d3d_buffer *buffer = unsafe_impl_from_ID3D10Buffer(buffers[i]);

        streams[i].buffer = buffer ? buffer->wined3d_buffer : NULL;
        streams[i].offset = offsets[i];
        streams[i].stride = strides[i];
        streams[i].frequency = 1;
        streams[i].flags = 0;
    }

    wined3d_device_context_set_stream_sources(device->immediate_context.wined3d_context,
            start_slot, buffer_count, streams);
}

static void STDMETHODCALLTYPE d3d10_device_IASetIndexBuffer(ID3D10Device1 *iface,
        ID3D10Buffer *buffer, DXGI_FORMAT format, UINT offset)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_buffer *buffer_impl = unsafe_impl_from_ID3D10Buffer(buffer);

    TRACE("iface %p, buffer %p, format %s, offset %u.\n",
            iface, buffer, debug_dxgi_format(format), offset);

    wined3d_device_context_set_index_buffer(device->immediate_context.wined3d_context,
            buffer_impl ? buffer_impl->wined3d_buffer : NULL,
            wined3dformat_from_dxgi_format(format), offset);
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

    wined3d_device_context_draw_indexed(device->immediate_context.wined3d_context, base_vertex_location,
            start_index_location, instance_index_count, start_instance_location, instance_count);
}

static void STDMETHODCALLTYPE d3d10_device_DrawInstanced(ID3D10Device1 *iface,
        UINT instance_vertex_count, UINT instance_count,
        UINT start_vertex_location, UINT start_instance_location)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);

    TRACE("iface %p, instance_vertex_count %u, instance_count %u, start_vertex_location %u, "
            "start_instance_location %u.\n", iface, instance_vertex_count, instance_count,
            start_vertex_location, start_instance_location);

    wined3d_device_context_draw(device->immediate_context.wined3d_context, start_vertex_location,
            instance_vertex_count, start_instance_location, instance_count);
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

    wined3d_device_context_set_shader(device->immediate_context.wined3d_context,
            WINED3D_SHADER_TYPE_GEOMETRY, gs ? gs->wined3d_shader : NULL);
}

static void STDMETHODCALLTYPE d3d10_device_IASetPrimitiveTopology(ID3D10Device1 *iface,
        D3D10_PRIMITIVE_TOPOLOGY topology)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);

    TRACE("iface %p, topology %s.\n", iface, debug_d3d10_primitive_topology(topology));

    wined3d_device_context_set_primitive_type(device->immediate_context.wined3d_context,
            (enum wined3d_primitive_type)topology, 0);
}

static void STDMETHODCALLTYPE d3d10_device_VSSetShaderResources(ID3D10Device1 *iface,
        UINT start_slot, UINT view_count, ID3D10ShaderResourceView *const *views)
{
    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n",
            iface, start_slot, view_count, views);

    d3d10_device_set_shader_resource_views(iface, WINED3D_SHADER_TYPE_VERTEX, start_slot, view_count, views);
}

static void STDMETHODCALLTYPE d3d10_device_VSSetSamplers(ID3D10Device1 *iface,
        UINT start_slot, UINT sampler_count, ID3D10SamplerState *const *samplers)
{
    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    d3d10_device_set_samplers(iface, WINED3D_SHADER_TYPE_VERTEX, start_slot, sampler_count, samplers);
}

static void STDMETHODCALLTYPE d3d10_device_SetPredication(ID3D10Device1 *iface, ID3D10Predicate *predicate, BOOL value)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_query *query;

    TRACE("iface %p, predicate %p, value %#x.\n", iface, predicate, value);

    query = unsafe_impl_from_ID3D10Query((ID3D10Query *)predicate);
    wined3d_device_context_set_predication(device->immediate_context.wined3d_context,
            query ? query->wined3d_query : NULL, value);
}

static void STDMETHODCALLTYPE d3d10_device_GSSetShaderResources(ID3D10Device1 *iface,
        UINT start_slot, UINT view_count, ID3D10ShaderResourceView *const *views)
{
    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n",
            iface, start_slot, view_count, views);

    d3d10_device_set_shader_resource_views(iface, WINED3D_SHADER_TYPE_GEOMETRY, start_slot, view_count, views);
}

static void STDMETHODCALLTYPE d3d10_device_GSSetSamplers(ID3D10Device1 *iface,
        UINT start_slot, UINT sampler_count, ID3D10SamplerState *const *samplers)
{
    TRACE("iface %p, start_slot %u, sampler_count %u, samplers %p.\n",
            iface, start_slot, sampler_count, samplers);

    d3d10_device_set_samplers(iface, WINED3D_SHADER_TYPE_GEOMETRY, start_slot, sampler_count, samplers);
}

static void STDMETHODCALLTYPE d3d10_device_OMSetRenderTargets(ID3D10Device1 *iface,
        UINT rtv_count, ID3D10RenderTargetView *const *rtvs, ID3D10DepthStencilView *depth_stencil_view)
{
    struct wined3d_rendertarget_view *wined3d_rtvs[D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT] = {0};
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_depthstencil_view *dsv;
    unsigned int i;

    TRACE("iface %p, rtv_count %u, rtvs %p, depth_stencil_view %p.\n", iface, rtv_count, rtvs, depth_stencil_view);

    if (rtv_count > ARRAY_SIZE(wined3d_rtvs))
    {
        WARN("View count %u exceeds limit.\n", rtv_count);
        rtv_count = ARRAY_SIZE(wined3d_rtvs);
    }

    for (i = 0; i < rtv_count; ++i)
    {
        struct d3d_rendertarget_view *rtv = unsafe_impl_from_ID3D10RenderTargetView(rtvs[i]);

        wined3d_rtvs[i] = rtv ? rtv->wined3d_view : NULL;
    }

    dsv = unsafe_impl_from_ID3D10DepthStencilView(depth_stencil_view);

    wined3d_device_context_set_render_targets_and_unordered_access_views(device->immediate_context.wined3d_context,
            ARRAY_SIZE(wined3d_rtvs), wined3d_rtvs, dsv ? dsv->wined3d_view : NULL, ~0u, NULL, NULL);
}

static void STDMETHODCALLTYPE d3d10_device_OMSetBlendState(ID3D10Device1 *iface,
        ID3D10BlendState *blend_state, const float blend_factor[4], UINT sample_mask)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_blend_state *blend_state_object;

    TRACE("iface %p, blend_state %p, blend_factor %s, sample_mask 0x%08x.\n",
            iface, blend_state, debug_float4(blend_factor), sample_mask);

    blend_state_object = unsafe_impl_from_ID3D10BlendState(blend_state);
    d3d11_device_context_OMSetBlendState(&device->immediate_context.ID3D11DeviceContext1_iface,
            blend_state_object ? (ID3D11BlendState *)&blend_state_object->ID3D11BlendState1_iface : NULL,
            blend_factor, sample_mask);
}

static void STDMETHODCALLTYPE d3d10_device_OMSetDepthStencilState(ID3D10Device1 *iface,
        ID3D10DepthStencilState *depth_stencil_state, UINT stencil_ref)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_depthstencil_state *ds_state_object;

    TRACE("iface %p, depth_stencil_state %p, stencil_ref %u.\n",
            iface, depth_stencil_state, stencil_ref);

    ds_state_object = unsafe_impl_from_ID3D10DepthStencilState(depth_stencil_state);
    d3d11_device_context_OMSetDepthStencilState(&device->immediate_context.ID3D11DeviceContext1_iface,
            ds_state_object ? &ds_state_object->ID3D11DepthStencilState_iface : NULL, stencil_ref);
}

static void STDMETHODCALLTYPE d3d10_device_SOSetTargets(ID3D10Device1 *iface,
        UINT target_count, ID3D10Buffer *const *targets, const UINT *offsets)
{
    struct wined3d_stream_output outputs[WINED3D_MAX_STREAM_OUTPUT_BUFFERS] = {0};
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    unsigned int count, i;

    TRACE("iface %p, target_count %u, targets %p, offsets %p.\n", iface, target_count, targets, offsets);

    count = min(target_count, ARRAY_SIZE(outputs));
    for (i = 0; i < count; ++i)
    {
        struct d3d_buffer *buffer = unsafe_impl_from_ID3D10Buffer(targets[i]);

        outputs[i].buffer = buffer ? buffer->wined3d_buffer : NULL;
        outputs[i].offset = offsets ? offsets[i] : 0;
    }

    wined3d_device_context_set_stream_outputs(device->immediate_context.wined3d_context, outputs);
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
    d3d11_device_context_RSSetState(&device->immediate_context.ID3D11DeviceContext1_iface,
            rasterizer_state_object ? (ID3D11RasterizerState *)&rasterizer_state_object->ID3D11RasterizerState1_iface : NULL);
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

    wined3d_device_context_set_viewports(device->immediate_context.wined3d_context, viewport_count, wined3d_vp);
}

static void STDMETHODCALLTYPE d3d10_device_RSSetScissorRects(ID3D10Device1 *iface,
        UINT rect_count, const D3D10_RECT *rects)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);

    TRACE("iface %p, rect_count %u, rects %p.\n", iface, rect_count, rects);

    if (rect_count > WINED3D_MAX_VIEWPORTS)
        return;

    wined3d_device_context_set_scissor_rects(device->immediate_context.wined3d_context, rect_count, rects);
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

    if (!dst_resource || !src_resource)
        return;

    if (src_box)
        wined3d_box_set(&wined3d_src_box, src_box->left, src_box->top,
                src_box->right, src_box->bottom, src_box->front, src_box->back);

    wined3d_dst_resource = wined3d_resource_from_d3d10_resource(dst_resource);
    wined3d_src_resource = wined3d_resource_from_d3d10_resource(src_resource);
    wined3d_device_context_copy_sub_resource_region(device->immediate_context.wined3d_context,
            wined3d_dst_resource, dst_subresource_idx, dst_x, dst_y, dst_z,
            wined3d_src_resource, src_subresource_idx, src_box ? &wined3d_src_box : NULL, 0);
}

static void STDMETHODCALLTYPE d3d10_device_CopyResource(ID3D10Device1 *iface,
        ID3D10Resource *dst_resource, ID3D10Resource *src_resource)
{
    struct wined3d_resource *wined3d_dst_resource, *wined3d_src_resource;
    struct d3d_device *device = impl_from_ID3D10Device(iface);

    TRACE("iface %p, dst_resource %p, src_resource %p.\n", iface, dst_resource, src_resource);

    wined3d_dst_resource = wined3d_resource_from_d3d10_resource(dst_resource);
    wined3d_src_resource = wined3d_resource_from_d3d10_resource(src_resource);
    wined3d_device_context_copy_resource(device->immediate_context.wined3d_context,
            wined3d_dst_resource, wined3d_src_resource);
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
    d3d11_device_context_UpdateSubresource(&device->immediate_context.ID3D11DeviceContext1_iface,
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

    if (FAILED(hr = wined3d_device_context_clear_rendertarget_view(device->immediate_context.wined3d_context,
            view->wined3d_view, NULL, WINED3DCLEAR_TARGET, &color, 0.0f, 0)))
        ERR("Failed to clear view, hr %#lx.\n", hr);
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

    if (FAILED(hr = wined3d_device_context_clear_rendertarget_view(device->immediate_context.wined3d_context,
            view->wined3d_view, NULL, wined3d_flags, NULL, depth, stencil)))
        ERR("Failed to clear view, hr %#lx.\n", hr);
}

static void STDMETHODCALLTYPE d3d10_device_GenerateMips(ID3D10Device1 *iface,
        ID3D10ShaderResourceView *view)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_shader_resource_view *srv = unsafe_impl_from_ID3D10ShaderResourceView(view);

    TRACE("iface %p, view %p.\n", iface, view);

    wined3d_device_context_generate_mipmaps(device->immediate_context.wined3d_context, srv->wined3d_view);
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
    wined3d_device_context_resolve_sub_resource(device->immediate_context.wined3d_context,
            wined3d_dst_resource, dst_subresource_idx,
            wined3d_src_resource, src_subresource_idx, wined3d_format);
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

        if (!(wined3d_view = wined3d_device_context_get_shader_resource_view(
                device->immediate_context.wined3d_context, WINED3D_SHADER_TYPE_PIXEL, start_slot + i)))
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
    if (!(wined3d_shader = wined3d_device_context_get_shader(device->immediate_context.wined3d_context,
            WINED3D_SHADER_TYPE_PIXEL)))
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

        if (!(wined3d_sampler = wined3d_device_context_get_sampler(
                device->immediate_context.wined3d_context, WINED3D_SHADER_TYPE_PIXEL, start_slot + i)))
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
    if (!(wined3d_shader = wined3d_device_context_get_shader(device->immediate_context.wined3d_context,
            WINED3D_SHADER_TYPE_VERTEX)))
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
    if (!(wined3d_declaration = wined3d_device_context_get_vertex_declaration(
            device->immediate_context.wined3d_context)))
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

        if (FAILED(wined3d_device_context_get_stream_source(device->immediate_context.wined3d_context, start_slot + i,
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
    wined3d_buffer = wined3d_device_context_get_index_buffer(
            device->immediate_context.wined3d_context, &wined3d_format, offset);
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
    if (!(wined3d_shader = wined3d_device_context_get_shader(device->immediate_context.wined3d_context,
            WINED3D_SHADER_TYPE_GEOMETRY)))
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
    wined3d_device_context_get_primitive_type(device->immediate_context.wined3d_context,
            (enum wined3d_primitive_type *)topology, NULL);
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

        if (!(wined3d_view = wined3d_device_context_get_shader_resource_view(
                device->immediate_context.wined3d_context, WINED3D_SHADER_TYPE_VERTEX, start_slot + i)))
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

        if (!(wined3d_sampler = wined3d_device_context_get_sampler(
                device->immediate_context.wined3d_context, WINED3D_SHADER_TYPE_VERTEX, start_slot + i)))
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
    if (!(wined3d_predicate = wined3d_device_context_get_predication(device->immediate_context.wined3d_context, value)))
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

        if (!(wined3d_view = wined3d_device_context_get_shader_resource_view(
                device->immediate_context.wined3d_context, WINED3D_SHADER_TYPE_GEOMETRY, start_slot + i)))
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

        if (!(wined3d_sampler = wined3d_device_context_get_sampler(
                device->immediate_context.wined3d_context, WINED3D_SHADER_TYPE_GEOMETRY, start_slot + i)))
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
            if (!(wined3d_view = wined3d_device_context_get_rendertarget_view(
                    device->immediate_context.wined3d_context, i))
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

        if (!(wined3d_view = wined3d_device_context_get_depth_stencil_view(device->immediate_context.wined3d_context))
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

    d3d11_device_context_OMGetBlendState(&device->immediate_context.ID3D11DeviceContext1_iface,
            &d3d11_blend_state, blend_factor, sample_mask);

    if (blend_state)
    {
        if (d3d11_blend_state)
        {
            *blend_state = (ID3D10BlendState *)&impl_from_ID3D11BlendState1(
                    (ID3D11BlendState1 *)d3d11_blend_state)->ID3D10BlendState1_iface;
            ID3D10BlendState_AddRef(*blend_state);
        }
        else
            *blend_state = NULL;
    }

    if (d3d11_blend_state)
        ID3D11BlendState_Release(d3d11_blend_state);
}

static void STDMETHODCALLTYPE d3d10_device_OMGetDepthStencilState(ID3D10Device1 *iface,
        ID3D10DepthStencilState **depth_stencil_state, UINT *stencil_ref)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    ID3D11DepthStencilState *d3d11_iface = NULL;

    TRACE("iface %p, depth_stencil_state %p, stencil_ref %p.\n",
            iface, depth_stencil_state, stencil_ref);

    d3d11_device_context_OMGetDepthStencilState(&device->immediate_context.ID3D11DeviceContext1_iface,
            &d3d11_iface, stencil_ref);

    if (depth_stencil_state)
    {
        if (d3d11_iface)
        {
            *depth_stencil_state = &impl_from_ID3D11DepthStencilState(d3d11_iface)->ID3D10DepthStencilState_iface;
            ID3D10DepthStencilState_AddRef(*depth_stencil_state);
        }
        else
            *depth_stencil_state = NULL;
    }

    if (d3d11_iface)
        ID3D11DepthStencilState_Release(d3d11_iface);
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

        if (!(wined3d_buffer = wined3d_device_context_get_stream_output(
                device->immediate_context.wined3d_context, i, &offsets[i])))
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
    if ((wined3d_state = wined3d_device_context_get_rasterizer_state(device->immediate_context.wined3d_context)))
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
    wined3d_device_context_get_viewports(device->immediate_context.wined3d_context,
            &actual_count, viewports ? wined3d_vp : NULL);
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
    wined3d_device_context_get_scissor_rects(device->immediate_context.wined3d_context, &actual_count, rects);
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

    d3d11_device_context_ClearState(&device->immediate_context.ID3D11DeviceContext1_iface);
}

static void STDMETHODCALLTYPE d3d10_device_Flush(ID3D10Device1 *iface)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);

    TRACE("iface %p.\n", iface);

    wined3d_device_context_flush(device->immediate_context.wined3d_context);
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

    if (FAILED(hr = d3d_texture2d_create(device, &d3d11_desc, NULL, (const D3D11_SUBRESOURCE_DATA *)data, &object)))
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

    *view = NULL;

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

    *view = NULL;

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

static D3D11_DSV_DIMENSION d3d11_dsv_dimension_from_d3d10(D3D10_DSV_DIMENSION dim)
{
    return (D3D11_DSV_DIMENSION)dim;
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

    *view = NULL;

    if (desc)
    {
        d3d11_desc.Format = desc->Format;
        d3d11_desc.ViewDimension = d3d11_dsv_dimension_from_d3d10(desc->ViewDimension);
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
            "shader_byte_code_length %Iu, input_layout %p.\n",
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

    TRACE("iface %p, byte_code %p, byte_code_length %Iu, shader %p.\n",
            iface, byte_code, byte_code_length, shader);

    *shader = NULL;

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

    TRACE("iface %p, byte_code %p, byte_code_length %Iu, shader %p.\n",
            iface, byte_code, byte_code_length, shader);

    *shader = NULL;

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

    TRACE("iface %p, byte_code %p, byte_code_length %Iu, output_stream_decls %p, "
            "output_stream_decl_count %u, output_stream_stride %u, shader %p.\n",
            iface, byte_code, byte_code_length, output_stream_decls,
            output_stream_decl_count, output_stream_stride, shader);

    *shader = NULL;

    if (!output_stream_decl_count && output_stream_stride)
    {
        WARN("Stride must be 0 when declaration entry count is 0.\n");
        return E_INVALIDARG;
    }

    if (output_stream_decl_count
            && !(so_entries = calloc(output_stream_decl_count, sizeof(*so_entries))))
    {
        ERR("Failed to allocate D3D11 SO declaration array memory.\n");
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
                free(so_entries);
                return E_INVALIDARG;
            }
        }
    }

    hr = d3d_geometry_shader_create(device, byte_code, byte_code_length,
            so_entries, output_stream_decl_count, &output_stream_stride, stride_count, 0, &object);
    free(so_entries);
    if (FAILED(hr))
        return hr;

    *shader = &object->ID3D10GeometryShader_iface;

    return hr;
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CreatePixelShader(ID3D10Device1 *iface,
        const void *byte_code, SIZE_T byte_code_length, ID3D10PixelShader **shader)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    struct d3d_pixel_shader *object;
    HRESULT hr;

    TRACE("iface %p, byte_code %p, byte_code_length %Iu, shader %p.\n",
            iface, byte_code, byte_code_length, shader);

    *shader = NULL;

    if (FAILED(hr = d3d_pixel_shader_create(device, byte_code, byte_code_length, &object)))
        return hr;

    *shader = &object->ID3D10PixelShader_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_device_CreateBlendState1(ID3D10Device1 *iface,
        const D3D10_BLEND_DESC1 *desc, ID3D10BlendState1 **blend_state)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);
    ID3D11BlendState *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, blend_state %p.\n", iface, desc, blend_state);

    if (FAILED(hr = d3d11_device_CreateBlendState(&device->ID3D11Device2_iface, (const D3D11_BLEND_DESC *)desc, &object)))
        return hr;

    *blend_state = &impl_from_ID3D11BlendState1((ID3D11BlendState1 *)object)->ID3D10BlendState1_iface;
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
    D3D11_RASTERIZER_DESC1 desc1;
    HRESULT hr;

    TRACE("iface %p, desc %p, rasterizer_state %p.\n", iface, desc, rasterizer_state);

    if (!desc)
        return E_INVALIDARG;

    memcpy(&desc1, desc, sizeof(*desc));
    desc1.ForcedSampleCount = 0;

    if (FAILED(hr = d3d_rasterizer_state_create(device, &desc1, &object)))
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

static D3D10_FEATURE_LEVEL1 d3d10_feature_level1_from_d3d_feature_level(D3D_FEATURE_LEVEL level)
{
    return (D3D10_FEATURE_LEVEL1)level;
}

static D3D10_FEATURE_LEVEL1 STDMETHODCALLTYPE d3d10_device_GetFeatureLevel(ID3D10Device1 *iface)
{
    struct d3d_device *device = impl_from_ID3D10Device(iface);

    TRACE("iface %p.\n", iface);

    return d3d10_feature_level1_from_d3d_feature_level(device->state->feature_level);
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

static struct d3d_device *impl_from_ID3D11VideoDevice1(ID3D11VideoDevice1 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_device, ID3D11VideoDevice1_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_video_device_QueryInterface(ID3D11VideoDevice1 *iface,
        REFIID iid, void **out)
{
    struct d3d_device *device = impl_from_ID3D11VideoDevice1(iface);
    return IUnknown_QueryInterface(device->outer_unk, iid, out);
}

static ULONG STDMETHODCALLTYPE d3d11_video_device_AddRef(ID3D11VideoDevice1 *iface)
{
    struct d3d_device *device = impl_from_ID3D11VideoDevice1(iface);
    return IUnknown_AddRef(device->outer_unk);
}

static ULONG STDMETHODCALLTYPE d3d11_video_device_Release(ID3D11VideoDevice1 *iface)
{
    struct d3d_device *device = impl_from_ID3D11VideoDevice1(iface);
    return IUnknown_Release(device->outer_unk);
}

static HRESULT STDMETHODCALLTYPE d3d11_video_device_CreateVideoDecoder(ID3D11VideoDevice1 *iface,
        const D3D11_VIDEO_DECODER_DESC *desc, const D3D11_VIDEO_DECODER_CONFIG *config, ID3D11VideoDecoder **decoder)
{
    struct d3d_device *device = impl_from_ID3D11VideoDevice1(iface);
    struct d3d_video_decoder *object;
    HRESULT hr;

    FIXME("iface %p, desc %p, config %p, decoder %p, stub!\n", iface, desc, config, decoder);

    if (FAILED(hr = d3d_video_decoder_create(device, desc, config, &object)))
        return hr;

    *decoder = &object->ID3D11VideoDecoder_iface;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_device_CreateVideoProcessor(ID3D11VideoDevice1 *iface,
        ID3D11VideoProcessorEnumerator *enumerator, UINT rate_conversion_index, ID3D11VideoProcessor **processor)
{
    FIXME("iface %p, enumerator %p, rate_conversion_index %u, processor %p, stub!\n",
            iface, enumerator, rate_conversion_index, processor);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_device_CreateAuthenticatedChannel(ID3D11VideoDevice1 *iface,
        D3D11_AUTHENTICATED_CHANNEL_TYPE type, ID3D11AuthenticatedChannel **channel)
{
    FIXME("iface %p, type %#x, channel %p, stub!\n", iface, type, channel);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_device_CreateCryptoSession(ID3D11VideoDevice1 *iface,
        const GUID *encryption_type, const GUID *profile, const GUID *key_exchange_type,
        ID3D11CryptoSession **session)
{
    FIXME("iface %p, encryption_type %s, profile %s, key_exchange_type %s, session %p, stub!\n",
            iface, debugstr_guid(encryption_type), debugstr_guid(profile), debugstr_guid(key_exchange_type), session);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_device_CreateVideoDecoderOutputView(ID3D11VideoDevice1 *iface,
        ID3D11Resource *resource, const D3D11_VIDEO_DECODER_OUTPUT_VIEW_DESC *desc, ID3D11VideoDecoderOutputView **view)
{
    struct d3d_device *device = impl_from_ID3D11VideoDevice1(iface);
    struct d3d_video_decoder_output_view *object;
    HRESULT hr;

    TRACE("iface %p, resource %p, desc %p, view %p.\n", iface, resource, desc, view);

    *view = NULL;

    if (!resource)
        return E_INVALIDARG;

    if (FAILED(hr = d3d_video_decoder_output_view_create(device, resource, desc, &object)))
        return hr;

    *view = &object->ID3D11VideoDecoderOutputView_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_device_CreateVideoProcessorInputView(
        ID3D11VideoDevice1 *iface, ID3D11Resource *resource, ID3D11VideoProcessorEnumerator *enumerator,
        const D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC *desc, ID3D11VideoProcessorInputView **view)
{
    FIXME("iface %p, resource %p, enumerator %p, desc %p, view %p, stub!\n", iface, resource, enumerator, desc, view);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_device_CreateVideoProcessorOutputView(
        ID3D11VideoDevice1 *iface, ID3D11Resource *resource, ID3D11VideoProcessorEnumerator *enumerator,
        const D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC *desc, ID3D11VideoProcessorOutputView **view)
{
    FIXME("iface %p, resource %p, enumerator %p, desc %p, view %p, stub!\n", iface, resource, enumerator, desc, view);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_device_CreateVideoProcessorEnumerator(ID3D11VideoDevice1 *iface,
        const D3D11_VIDEO_PROCESSOR_CONTENT_DESC *desc, ID3D11VideoProcessorEnumerator **enumerator)
{
    FIXME("iface %p, desc %p, enumerator %p, stub!\n", iface, desc, enumerator);
    return E_NOTIMPL;
}

static UINT STDMETHODCALLTYPE d3d11_video_device_GetVideoDecoderProfileCount(ID3D11VideoDevice1 *iface)
{
    FIXME("iface %p, stub!\n", iface);
    return 0;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_device_GetVideoDecoderProfile(
        ID3D11VideoDevice1 *iface, UINT index, GUID *profile)
{
    FIXME("iface %p, index %u, profile %p, stub!\n", iface, index, profile);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_device_CheckVideoDecoderFormat(
        ID3D11VideoDevice1 *iface, const GUID *profile, DXGI_FORMAT format, BOOL *supported)
{
    FIXME("iface %p, profile %s, format %#x, supported %p, stub!\n", iface, debugstr_guid(profile), format, supported);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_device_GetVideoDecoderConfigCount(ID3D11VideoDevice1 *iface,
        const D3D11_VIDEO_DECODER_DESC *desc, UINT *count)
{
    FIXME("iface %p, desc %p, count %p, stub!\n", iface, desc, count);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_device_GetVideoDecoderConfig(ID3D11VideoDevice1 *iface,
        const D3D11_VIDEO_DECODER_DESC *desc, UINT index, D3D11_VIDEO_DECODER_CONFIG *config)
{
    FIXME("iface %p, desc %p, index %u, config %p, stub!\n", iface, desc, index, config);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_device_GetContentProtectionCaps(ID3D11VideoDevice1 *iface,
        const GUID *encryption_type, const GUID *profile, D3D11_VIDEO_CONTENT_PROTECTION_CAPS *caps)
{
    FIXME("iface %p, encryption_type %s, profile %s, caps %p, stub!\n",
            iface, debugstr_guid(encryption_type), debugstr_guid(profile), caps);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_device_CheckCryptoKeyExchange(ID3D11VideoDevice1 *iface,
        const GUID *encryption_type, const GUID *profile, UINT index, GUID *key_exchange_type)
{
    FIXME("iface %p, encryption_type %s, profile %s, index %u, key_exchange_type %p, stub!\n",
            iface, debugstr_guid(encryption_type), debugstr_guid(profile), index, key_exchange_type);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_device_SetPrivateData(
        ID3D11VideoDevice1 *iface, REFGUID guid, UINT size, const void *data)
{
    TRACE("iface %p, guid %s, size %u, data %p.\n", iface, debugstr_guid(guid), size, data);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_device_SetPrivateDataInterface(
        ID3D11VideoDevice1 *iface, REFGUID guid, const IUnknown *data)
{
    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_device_GetCryptoSessionPrivateDataSize(
        ID3D11VideoDevice1 *iface, const GUID *encryption_type, const GUID *profile,
        const GUID *key_exchange_type, UINT *input_size, UINT *output_size)
{
    FIXME("iface %p, encryption_type %s, profile %s, key_exchange_type %s, input_size %p, output_size %p, stub!\n",
            iface, debugstr_guid(encryption_type), debugstr_guid(profile),
            debugstr_guid(key_exchange_type), input_size, output_size);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_device_GetVideoDecoderCaps(
        ID3D11VideoDevice1 *iface, const GUID *profile, UINT width, UINT height,
        const DXGI_RATIONAL *framerate, UINT bitrate, const GUID *encryption_type, UINT *caps)
{
    TRACE("iface %p, profile %s, width %u, height %u, framerate %u/%u, bitrate %u, encryption_type %s, caps %p, stub!\n",
            iface, debugstr_guid(profile), width, height, framerate->Numerator, framerate->Denominator,
            bitrate, debugstr_guid(encryption_type), caps);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_device_CheckVideoDecoderDownsampling(ID3D11VideoDevice1 *iface,
        const D3D11_VIDEO_DECODER_DESC *input_desc, DXGI_COLOR_SPACE_TYPE input_colour_space,
        const D3D11_VIDEO_DECODER_CONFIG *config, const DXGI_RATIONAL *framerate,
        const D3D11_VIDEO_SAMPLE_DESC *output_desc, BOOL *supported, BOOL *real_time_hint)
{
    TRACE("iface %p, input_desc %p, input_colour_space %#x, config %p,"
            " framerate %u/%u, output_desc %p, supported %p, real_time_hint %p, stub!\n",
            iface, input_desc, input_colour_space, config,
            framerate->Numerator, framerate->Denominator, output_desc, supported, real_time_hint);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_device_RecommendVideoDecoderDownsampleParameters(ID3D11VideoDevice1 *iface,
        const D3D11_VIDEO_DECODER_DESC *input_desc, DXGI_COLOR_SPACE_TYPE input_colour_space,
        const D3D11_VIDEO_DECODER_CONFIG *config, const DXGI_RATIONAL *framerate,
        D3D11_VIDEO_SAMPLE_DESC *output_desc)
{
    TRACE("iface %p, input_desc %p, input_colour_space %#x, config %p, framerate %u/%u, output_desc %p, stub!\n",
            iface, input_desc, input_colour_space, config, framerate->Numerator, framerate->Denominator, output_desc);
    return E_NOTIMPL;
}

static const struct ID3D11VideoDevice1Vtbl d3d11_video_device1_vtbl =
{
    d3d11_video_device_QueryInterface,
    d3d11_video_device_AddRef,
    d3d11_video_device_Release,
    d3d11_video_device_CreateVideoDecoder,
    d3d11_video_device_CreateVideoProcessor,
    d3d11_video_device_CreateAuthenticatedChannel,
    d3d11_video_device_CreateCryptoSession,
    d3d11_video_device_CreateVideoDecoderOutputView,
    d3d11_video_device_CreateVideoProcessorInputView,
    d3d11_video_device_CreateVideoProcessorOutputView,
    d3d11_video_device_CreateVideoProcessorEnumerator,
    d3d11_video_device_GetVideoDecoderProfileCount,
    d3d11_video_device_GetVideoDecoderProfile,
    d3d11_video_device_CheckVideoDecoderFormat,
    d3d11_video_device_GetVideoDecoderConfigCount,
    d3d11_video_device_GetVideoDecoderConfig,
    d3d11_video_device_GetContentProtectionCaps,
    d3d11_video_device_CheckCryptoKeyExchange,
    d3d11_video_device_SetPrivateData,
    d3d11_video_device_SetPrivateDataInterface,
    d3d11_video_device_GetCryptoSessionPrivateDataSize,
    d3d11_video_device_GetVideoDecoderCaps,
    d3d11_video_device_CheckVideoDecoderDownsampling,
    d3d11_video_device_RecommendVideoDecoderDownsampleParameters,
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

static HRESULT STDMETHODCALLTYPE dxgi_device_parent_register_swapchain_texture(IWineDXGIDeviceParent *iface,
        struct wined3d_texture *wined3d_texture, unsigned int texture_flags, IDXGISurface **ret_surface)
{
    struct d3d_device *device = device_from_dxgi_device_parent(iface);
    struct wined3d_resource_desc wined3d_desc;
    struct d3d_texture2d *object;
    D3D11_TEXTURE2D_DESC desc;
    HRESULT hr;

    wined3d_resource_get_desc(wined3d_texture_get_resource(wined3d_texture), &wined3d_desc);

    desc.Width = wined3d_desc.width;
    desc.Height = wined3d_desc.height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = dxgi_format_from_wined3dformat(wined3d_desc.format);
    desc.SampleDesc.Count = wined3d_desc.multisample_type ? wined3d_desc.multisample_type : 1;
    desc.SampleDesc.Quality = wined3d_desc.multisample_quality;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = d3d11_bind_flags_from_wined3d(wined3d_desc.bind_flags);
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    if (texture_flags & WINED3D_TEXTURE_CREATE_GET_DC)
    {
        desc.MiscFlags |= D3D11_RESOURCE_MISC_GDI_COMPATIBLE;
        texture_flags &= ~WINED3D_TEXTURE_CREATE_GET_DC;
    }

    if (texture_flags)
        FIXME("Unhandled flags %#x.\n", texture_flags);

    if (FAILED(hr = d3d_texture2d_create(device, &desc, wined3d_texture, NULL, &object)))
        return hr;

    hr = IUnknown_QueryInterface(object->dxgi_resource, &IID_IDXGISurface, (void **)ret_surface);
    ID3D11Texture2D_Release(&object->ID3D11Texture2D_iface);
    return hr;
}

static const struct IWineDXGIDeviceParentVtbl d3d_dxgi_device_parent_vtbl =
{
    /* IUnknown methods */
    dxgi_device_parent_QueryInterface,
    dxgi_device_parent_AddRef,
    dxgi_device_parent_Release,
    /* IWineDXGIDeviceParent methods */
    dxgi_device_parent_get_wined3d_device_parent,
    dxgi_device_parent_register_swapchain_texture,
};

static inline struct d3d_device *device_from_wined3d_device_parent(struct wined3d_device_parent *device_parent)
{
    return CONTAINING_RECORD(device_parent, struct d3d_device, device_parent);
}

static void CDECL device_parent_wined3d_device_created(struct wined3d_device_parent *device_parent,
        struct wined3d_device *wined3d_device)
{
    struct d3d_device *device = device_from_wined3d_device_parent(device_parent);
    struct d3d_device_context_state *state;
    struct wined3d_state *wined3d_state;
    D3D_FEATURE_LEVEL feature_level;

    TRACE("device_parent %p, wined3d_device %p.\n", device_parent, wined3d_device);

    wined3d_device_incref(wined3d_device);
    device->wined3d_device = wined3d_device;
    device->immediate_context.wined3d_context = wined3d_device_get_immediate_context(wined3d_device);

    wined3d_state = wined3d_device_get_state(device->wined3d_device);
    feature_level = d3d_feature_level_from_wined3d(wined3d_state_get_feature_level(wined3d_state));

    if (!(state = calloc(1, sizeof(*state))))
    {
        ERR("Failed to create the initial device context state.\n");
        return;
    }

    d3d_device_context_state_init(state, device, feature_level,
            device->d3d11_only ? &IID_ID3D11Device2 : &IID_ID3D10Device1);

    device->state = state;
    if (!d3d_device_context_state_add_entry(state, device, wined3d_state))
        ERR("Failed to add entry for wined3d state %p, device %p.\n", wined3d_state, device);

    d3d_device_context_state_private_addref(state);
    ID3DDeviceContextState_Release(&state->ID3DDeviceContextState_iface);
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

static const struct wined3d_device_parent_ops d3d_wined3d_device_parent_ops =
{
    device_parent_wined3d_device_created,
    device_parent_mode_changed,
    device_parent_activate,
    device_parent_texture_sub_resource_created,
};

static int d3d_sampler_state_compare(const void *key, const struct wine_rb_entry *entry)
{
    const D3D11_SAMPLER_DESC *ka = key;
    const D3D11_SAMPLER_DESC *kb = &WINE_RB_ENTRY_VALUE(entry, const struct d3d_sampler_state, entry)->desc;

    return memcmp(ka, kb, sizeof(*ka));
}

static int d3d_blend_state_compare(const void *key, const struct wine_rb_entry *entry)
{
    const D3D11_BLEND_DESC1 *ka = key;
    const D3D11_BLEND_DESC1 *kb = &WINE_RB_ENTRY_VALUE(entry, const struct d3d_blend_state, entry)->desc;

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
    const D3D11_RASTERIZER_DESC1 *ka = key;
    const D3D11_RASTERIZER_DESC1 *kb = &WINE_RB_ENTRY_VALUE(entry, const struct d3d_rasterizer_state, entry)->desc;

    return memcmp(ka, kb, sizeof(*ka));
}

void d3d_device_init(struct d3d_device *device, void *outer_unknown)
{
    device->IUnknown_inner.lpVtbl = &d3d_device_inner_unknown_vtbl;
    device->ID3D11Device2_iface.lpVtbl = &d3d11_device_vtbl;
    device->ID3D10Device1_iface.lpVtbl = &d3d10_device1_vtbl;
    device->ID3D10Multithread_iface.lpVtbl = &d3d10_multithread_vtbl;
    device->ID3D11VideoDevice1_iface.lpVtbl = &d3d11_video_device1_vtbl;
    device->IWineDXGIDeviceParent_iface.lpVtbl = &d3d_dxgi_device_parent_vtbl;
    device->device_parent.ops = &d3d_wined3d_device_parent_ops;
    device->refcount = 1;
    /* COM aggregation always takes place */
    device->outer_unk = outer_unknown;
    device->d3d11_only = FALSE;
    device->state = NULL;

    d3d11_device_context_init(&device->immediate_context, device, D3D11_DEVICE_CONTEXT_IMMEDIATE);
    ID3D11DeviceContext1_Release(&device->immediate_context.ID3D11DeviceContext1_iface);

    wine_rb_init(&device->blend_states, d3d_blend_state_compare);
    wine_rb_init(&device->depthstencil_states, d3d_depthstencil_state_compare);
    wine_rb_init(&device->rasterizer_states, d3d_rasterizer_state_compare);
    wine_rb_init(&device->sampler_states, d3d_sampler_state_compare);
}
