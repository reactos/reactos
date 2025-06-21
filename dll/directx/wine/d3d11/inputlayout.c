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

#include "d3d11_private.h"
#include "winternl.h"
#include <vkd3d_shader.h>

WINE_DEFAULT_DEBUG_CHANNEL(d3d11);

static BOOL is_vs_sysval_semantic(const struct vkd3d_shader_signature_element *e)
{
    return !stricmp(e->semantic_name, "sv_instanceid") || !stricmp(e->semantic_name, "sv_vertexid");
}

static unsigned int find_input_element(const D3D11_INPUT_ELEMENT_DESC *element_descs, unsigned int element_count,
        const struct vkd3d_shader_signature_element *ise)
{
    const D3D11_INPUT_ELEMENT_DESC *f;
    unsigned int i;

    if (ise->stream_index)
        return element_count;

    for (i = 0; i < element_count; ++i)
    {
        f = &element_descs[i];
        if (!stricmp(ise->semantic_name, f->SemanticName) && ise->semantic_index == f->SemanticIndex)
            return i;
    }
    return element_count;
}

static enum wined3d_input_classification wined3d_input_classification_from_d3d11(D3D11_INPUT_CLASSIFICATION slot_class)
{
    return (enum wined3d_input_classification)slot_class;
}

static HRESULT d3d11_input_layout_to_wined3d_declaration(const D3D11_INPUT_ELEMENT_DESC *element_descs,
        UINT element_count, const void *shader_byte_code, SIZE_T shader_byte_code_length,
        struct wined3d_vertex_element **wined3d_elements)
{
    const struct vkd3d_shader_code dxbc = {shader_byte_code, shader_byte_code_length};
    struct vkd3d_shader_signature_element *ise;
    const D3D11_INPUT_ELEMENT_DESC *f;
    struct vkd3d_shader_signature is;
    unsigned int i, index;
    int ret;

    if ((ret = vkd3d_shader_parse_input_signature(&dxbc, &is, NULL)) < 0)
    {
        ERR("Failed to extract input signature, ret %d.\n", ret);
        return E_FAIL;
    }

    if (!(*wined3d_elements = calloc(element_count, sizeof(**wined3d_elements))))
    {
        ERR("Failed to allocate wined3d vertex element array memory.\n");
        vkd3d_shader_free_shader_signature(&is);
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < element_count; ++i)
    {
        struct wined3d_vertex_element *e = &(*wined3d_elements)[i];

        f = &element_descs[i];

        e->format = wined3dformat_from_dxgi_format(f->Format);
        e->input_slot = f->InputSlot;
        e->offset = f->AlignedByteOffset;
        e->output_slot = WINED3D_OUTPUT_SLOT_UNUSED;
        e->input_slot_class = wined3d_input_classification_from_d3d11(f->InputSlotClass);
        e->instance_data_step_rate = f->InstanceDataStepRate;
        e->method = WINED3D_DECL_METHOD_DEFAULT;
        e->usage = 0;
        e->usage_idx = 0;
    }

    ise = is.elements;

    for (i = 0; i < is.element_count; ++i)
    {
        if ((index = find_input_element(element_descs, element_count, &ise[i])) < element_count)
        {
            (*wined3d_elements)[index].output_slot = ise[i].register_index;
        }
        else if (!is_vs_sysval_semantic(&ise[i]))
        {
            WARN("Input element %s%u not found in shader signature.\n", ise[i].semantic_name, ise[i].semantic_index);
            free(*wined3d_elements);
            vkd3d_shader_free_shader_signature(&is);
            return E_INVALIDARG;
        }
    }

    vkd3d_shader_free_shader_signature(&is);

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

    TRACE("%p increasing refcount to %lu.\n", layout, refcount);

    if (refcount == 1)
    {
        ID3D11Device2_AddRef(layout->device);
        wined3d_vertex_declaration_incref(layout->wined3d_decl);
    }

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_input_layout_Release(ID3D11InputLayout *iface)
{
    struct d3d_input_layout *layout = impl_from_ID3D11InputLayout(iface);
    ULONG refcount = InterlockedDecrement(&layout->refcount);

    TRACE("%p decreasing refcount to %lu.\n", layout, refcount);

    if (!refcount)
    {
        ID3D11Device2 *device = layout->device;
        wined3d_vertex_declaration_decref(layout->wined3d_decl);
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
    free(parent);
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
        WARN("Failed to create wined3d vertex declaration elements, hr %#lx.\n", hr);
        wined3d_private_store_cleanup(&layout->private_store);
        wined3d_mutex_unlock();
        return hr;
    }

    hr = wined3d_vertex_declaration_create(device->wined3d_device, wined3d_elements, element_count,
            layout, &d3d_input_layout_wined3d_parent_ops, &layout->wined3d_decl);
    free(wined3d_elements);
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d vertex declaration, hr %#lx.\n", hr);
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

    if (!element_descs) return E_INVALIDARG;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d_input_layout_init(object, device, element_descs, element_count,
            shader_byte_code, shader_byte_code_length)))
    {
        WARN("Failed to initialise input layout, hr %#lx.\n", hr);
        free(object);
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
