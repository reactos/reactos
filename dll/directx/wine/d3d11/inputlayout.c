/*
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

#include "d3d11_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d11);

static struct wined3d_shader_signature_element *shader_find_signature_element(const struct wined3d_shader_signature *s,
        const char *semantic_name, unsigned int semantic_idx, unsigned int stream_idx)
{
    struct wined3d_shader_signature_element *e = s->elements;
    unsigned int i;

    for (i = 0; i < s->element_count; ++i)
    {
        if (!_strnicmp(e[i].semantic_name, semantic_name, -1) && e[i].semantic_idx == semantic_idx
                && e[i].stream_idx == stream_idx)
            return &e[i];
    }

    return NULL;
}

static HRESULT d3d11_input_layout_to_wined3d_declaration(const D3D11_INPUT_ELEMENT_DESC *element_descs,
        UINT element_count, const void *shader_byte_code, SIZE_T shader_byte_code_length,
        struct wined3d_vertex_element **wined3d_elements)
{
    struct wined3d_shader_signature is;
    unsigned int i;
    HRESULT hr;

    if (FAILED(hr = wined3d_extract_shader_input_signature_from_dxbc(&is, shader_byte_code, shader_byte_code_length)))
    {
        ERR("Failed to extract input signature.\n");
        return E_FAIL;
    }

    if (!(*wined3d_elements = heap_calloc(element_count, sizeof(**wined3d_elements))))
    {
        ERR("Failed to allocate wined3d vertex element array memory.\n");
        heap_free(is.elements);
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < element_count; ++i)
    {
        struct wined3d_vertex_element *e = &(*wined3d_elements)[i];
        const D3D11_INPUT_ELEMENT_DESC *f = &element_descs[i];
        struct wined3d_shader_signature_element *element;

        e->format = wined3dformat_from_dxgi_format(f->Format);
        e->input_slot = f->InputSlot;
        e->offset = f->AlignedByteOffset;
        e->output_slot = WINED3D_OUTPUT_SLOT_UNUSED;
        e->input_slot_class = f->InputSlotClass;
        e->instance_data_step_rate = f->InstanceDataStepRate;
        e->method = WINED3D_DECL_METHOD_DEFAULT;
        e->usage = 0;
        e->usage_idx = 0;

        if ((element = shader_find_signature_element(&is, f->SemanticName, f->SemanticIndex, 0)))
            e->output_slot = element->register_idx;
        else
            WARN("Unused input element %u.\n", i);
    }

    heap_free(is.elements);

    return S_OK;
}

/* ID3D11InputLayout methods */

static inline struct d3d_input_layout *impl_from_ID3D11InputLayout(ID3D11InputLayout *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_input_layout, ID3D11InputLayout_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_input_layout_QueryInterface(ID3D11InputLayout *iface,
        REFIID riid, void **object)
{
    struct d3d_input_layout *layout = impl_from_ID3D11InputLayout(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11InputLayout)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D11InputLayout_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_ID3D10InputLayout)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild))
    {
        ID3D10InputLayout_AddRef(&layout->ID3D10InputLayout_iface);
        *object = &layout->ID3D10InputLayout_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_input_layout_AddRef(ID3D11InputLayout *iface)
{
    struct d3d_input_layout *layout = impl_from_ID3D11InputLayout(iface);
    ULONG refcount = InterlockedIncrement(&layout->refcount);

    TRACE("%p increasing refcount to %u.\n", layout, refcount);

    if (refcount == 1)
    {
        ID3D11Device2_AddRef(layout->device);
        wined3d_mutex_lock();
        wined3d_vertex_declaration_incref(layout->wined3d_decl);
        wined3d_mutex_unlock();
    }

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_input_layout_Release(ID3D11InputLayout *iface)
{
    struct d3d_input_layout *layout = impl_from_ID3D11InputLayout(iface);
    ULONG refcount = InterlockedDecrement(&layout->refcount);

    TRACE("%p decreasing refcount to %u.\n", layout, refcount);

    if (!refcount)
    {
        ID3D11Device2 *device = layout->device;

        wined3d_mutex_lock();
        wined3d_vertex_declaration_decref(layout->wined3d_decl);
        wined3d_mutex_unlock();

        ID3D11Device2_Release(device);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_input_layout_GetDevice(ID3D11InputLayout *iface,
        ID3D11Device **device)
{
    struct d3d_input_layout *layout = impl_from_ID3D11InputLayout(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device_AddRef(*device = (ID3D11Device *)layout->device);
}

static HRESULT STDMETHODCALLTYPE d3d11_input_layout_GetPrivateData(ID3D11InputLayout *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_input_layout *layout = impl_from_ID3D11InputLayout(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&layout->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_input_layout_SetPrivateData(ID3D11InputLayout *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_input_layout *layout = impl_from_ID3D11InputLayout(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&layout->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_input_layout_SetPrivateDataInterface(ID3D11InputLayout *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_input_layout *layout = impl_from_ID3D11InputLayout(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&layout->private_store, guid, data);
}

static const struct ID3D11InputLayoutVtbl d3d11_input_layout_vtbl =
{
    /* IUnknown methods */
    d3d11_input_layout_QueryInterface,
    d3d11_input_layout_AddRef,
    d3d11_input_layout_Release,
    /* ID3D11DeviceChild methods */
    d3d11_input_layout_GetDevice,
    d3d11_input_layout_GetPrivateData,
    d3d11_input_layout_SetPrivateData,
    d3d11_input_layout_SetPrivateDataInterface,
};

/* ID3D10InputLayout methods */

static inline struct d3d_input_layout *impl_from_ID3D10InputLayout(ID3D10InputLayout *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_input_layout, ID3D10InputLayout_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_input_layout_QueryInterface(ID3D10InputLayout *iface,
        REFIID riid, void **object)
{
    struct d3d_input_layout *layout = impl_from_ID3D10InputLayout(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return d3d11_input_layout_QueryInterface(&layout->ID3D11InputLayout_iface, riid, object);
}

static ULONG STDMETHODCALLTYPE d3d10_input_layout_AddRef(ID3D10InputLayout *iface)
{
    struct d3d_input_layout *layout = impl_from_ID3D10InputLayout(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_input_layout_AddRef(&layout->ID3D11InputLayout_iface);
}

static ULONG STDMETHODCALLTYPE d3d10_input_layout_Release(ID3D10InputLayout *iface)
{
    struct d3d_input_layout *layout = impl_from_ID3D10InputLayout(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_input_layout_Release(&layout->ID3D11InputLayout_iface);
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_input_layout_GetDevice(ID3D10InputLayout *iface, ID3D10Device **device)
{
    struct d3d_input_layout *layout = impl_from_ID3D10InputLayout(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device2_QueryInterface(layout->device, &IID_ID3D10Device, (void **)device);
}

static HRESULT STDMETHODCALLTYPE d3d10_input_layout_GetPrivateData(ID3D10InputLayout *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_input_layout *layout = impl_from_ID3D10InputLayout(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&layout->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_input_layout_SetPrivateData(ID3D10InputLayout *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_input_layout *layout = impl_from_ID3D10InputLayout(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&layout->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_input_layout_SetPrivateDataInterface(ID3D10InputLayout *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_input_layout *layout = impl_from_ID3D10InputLayout(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&layout->private_store, guid, data);
}

static const struct ID3D10InputLayoutVtbl d3d10_input_layout_vtbl =
{
    /* IUnknown methods */
    d3d10_input_layout_QueryInterface,
    d3d10_input_layout_AddRef,
    d3d10_input_layout_Release,
    /* ID3D10DeviceChild methods */
    d3d10_input_layout_GetDevice,
    d3d10_input_layout_GetPrivateData,
    d3d10_input_layout_SetPrivateData,
    d3d10_input_layout_SetPrivateDataInterface,
};

static void STDMETHODCALLTYPE d3d_input_layout_wined3d_object_destroyed(void *parent)
{
    struct d3d_input_layout *layout = parent;

    wined3d_private_store_cleanup(&layout->private_store);
    heap_free(parent);
}

static const struct wined3d_parent_ops d3d_input_layout_wined3d_parent_ops =
{
    d3d_input_layout_wined3d_object_destroyed,
};

static HRESULT d3d_input_layout_init(struct d3d_input_layout *layout, struct d3d_device *device,
        const D3D11_INPUT_ELEMENT_DESC *element_descs, UINT element_count,
        const void *shader_byte_code, SIZE_T shader_byte_code_length)
{
    struct wined3d_vertex_element *wined3d_elements;
    HRESULT hr;

    layout->ID3D11InputLayout_iface.lpVtbl = &d3d11_input_layout_vtbl;
    layout->ID3D10InputLayout_iface.lpVtbl = &d3d10_input_layout_vtbl;
    layout->refcount = 1;
    wined3d_mutex_lock();
    wined3d_private_store_init(&layout->private_store);

    if (FAILED(hr = d3d11_input_layout_to_wined3d_declaration(element_descs, element_count,
            shader_byte_code, shader_byte_code_length, &wined3d_elements)))
    {
        WARN("Failed to create wined3d vertex declaration elements, hr %#x.\n", hr);
        wined3d_private_store_cleanup(&layout->private_store);
        wined3d_mutex_unlock();
        return hr;
    }

    hr = wined3d_vertex_declaration_create(device->wined3d_device, wined3d_elements, element_count,
            layout, &d3d_input_layout_wined3d_parent_ops, &layout->wined3d_decl);
    heap_free(wined3d_elements);
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d vertex declaration, hr %#x.\n", hr);
        wined3d_private_store_cleanup(&layout->private_store);
        wined3d_mutex_unlock();
        return hr;
    }
    wined3d_mutex_unlock();

    ID3D11Device2_AddRef(layout->device = &device->ID3D11Device2_iface);

    return S_OK;
}

HRESULT d3d_input_layout_create(struct d3d_device *device,
        const D3D11_INPUT_ELEMENT_DESC *element_descs, UINT element_count,
        const void *shader_byte_code, SIZE_T shader_byte_code_length,
        struct d3d_input_layout **layout)
{
    struct d3d_input_layout *object;
    HRESULT hr;

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d_input_layout_init(object, device, element_descs, element_count,
            shader_byte_code, shader_byte_code_length)))
    {
        WARN("Failed to initialize input layout, hr %#x.\n", hr);
        heap_free(object);
        return hr;
    }

    TRACE("Created input layout %p.\n", object);
    *layout = object;

    return S_OK;
}

struct d3d_input_layout *unsafe_impl_from_ID3D11InputLayout(ID3D11InputLayout *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d11_input_layout_vtbl);

    return impl_from_ID3D11InputLayout(iface);
}

struct d3d_input_layout *unsafe_impl_from_ID3D10InputLayout(ID3D10InputLayout *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d10_input_layout_vtbl);

    return impl_from_ID3D10InputLayout(iface);
}
