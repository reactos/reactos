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

WINE_DEFAULT_DEBUG_CHANNEL(d3d11);

/* ID3D11VertexShader methods */

static inline struct d3d_vertex_shader *impl_from_ID3D11VertexShader(ID3D11VertexShader *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_vertex_shader, ID3D11VertexShader_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_vertex_shader_QueryInterface(ID3D11VertexShader *iface,
        REFIID riid, void **object)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D11VertexShader(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11VertexShader)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D11VertexShader_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_ID3D10VertexShader)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild))
    {
        IUnknown_AddRef(&shader->ID3D10VertexShader_iface);
        *object = &shader->ID3D10VertexShader_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_vertex_shader_AddRef(ID3D11VertexShader *iface)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D11VertexShader(iface);
    ULONG refcount = InterlockedIncrement(&shader->refcount);

    TRACE("%p increasing refcount to %lu.\n", shader, refcount);

    if (refcount == 1)
    {
        ID3D11Device2_AddRef(shader->device);
        wined3d_shader_incref(shader->wined3d_shader);
    }

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_vertex_shader_Release(ID3D11VertexShader *iface)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D11VertexShader(iface);
    ULONG refcount = InterlockedDecrement(&shader->refcount);

    TRACE("%p decreasing refcount to %lu.\n", shader, refcount);

    if (!refcount)
    {
        ID3D11Device2 *device = shader->device;

        wined3d_shader_decref(shader->wined3d_shader);
        /* Release the device last, it may cause the wined3d device to be
         * destroyed. */
        ID3D11Device2_Release(device);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_vertex_shader_GetDevice(ID3D11VertexShader *iface,
        ID3D11Device **device)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D11VertexShader(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (ID3D11Device *)shader->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_vertex_shader_GetPrivateData(ID3D11VertexShader *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D11VertexShader(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_vertex_shader_SetPrivateData(ID3D11VertexShader *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D11VertexShader(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_vertex_shader_SetPrivateDataInterface(ID3D11VertexShader *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D11VertexShader(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&shader->private_store, guid, data);
}

static const struct ID3D11VertexShaderVtbl d3d11_vertex_shader_vtbl =
{
    /* IUnknown methods */
    d3d11_vertex_shader_QueryInterface,
    d3d11_vertex_shader_AddRef,
    d3d11_vertex_shader_Release,
    /* ID3D11DeviceChild methods */
    d3d11_vertex_shader_GetDevice,
    d3d11_vertex_shader_GetPrivateData,
    d3d11_vertex_shader_SetPrivateData,
    d3d11_vertex_shader_SetPrivateDataInterface,
};

/* ID3D10VertexShader methods */

static inline struct d3d_vertex_shader *impl_from_ID3D10VertexShader(ID3D10VertexShader *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_vertex_shader, ID3D10VertexShader_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_vertex_shader_QueryInterface(ID3D10VertexShader *iface,
        REFIID riid, void **object)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D10VertexShader(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return d3d11_vertex_shader_QueryInterface(&shader->ID3D11VertexShader_iface, riid, object);
}

static ULONG STDMETHODCALLTYPE d3d10_vertex_shader_AddRef(ID3D10VertexShader *iface)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D10VertexShader(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_vertex_shader_AddRef(&shader->ID3D11VertexShader_iface);
}

static ULONG STDMETHODCALLTYPE d3d10_vertex_shader_Release(ID3D10VertexShader *iface)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D10VertexShader(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_vertex_shader_Release(&shader->ID3D11VertexShader_iface);
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_vertex_shader_GetDevice(ID3D10VertexShader *iface, ID3D10Device **device)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D10VertexShader(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device2_QueryInterface(shader->device, &IID_ID3D10Device, (void **)device);
}

static HRESULT STDMETHODCALLTYPE d3d10_vertex_shader_GetPrivateData(ID3D10VertexShader *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D10VertexShader(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_vertex_shader_SetPrivateData(ID3D10VertexShader *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D10VertexShader(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_vertex_shader_SetPrivateDataInterface(ID3D10VertexShader *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_vertex_shader *shader = impl_from_ID3D10VertexShader(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&shader->private_store, guid, data);
}

static const struct ID3D10VertexShaderVtbl d3d10_vertex_shader_vtbl =
{
    /* IUnknown methods */
    d3d10_vertex_shader_QueryInterface,
    d3d10_vertex_shader_AddRef,
    d3d10_vertex_shader_Release,
    /* ID3D10DeviceChild methods */
    d3d10_vertex_shader_GetDevice,
    d3d10_vertex_shader_GetPrivateData,
    d3d10_vertex_shader_SetPrivateData,
    d3d10_vertex_shader_SetPrivateDataInterface,
};

static void STDMETHODCALLTYPE d3d_vertex_shader_wined3d_object_destroyed(void *parent)
{
    struct d3d_vertex_shader *shader = parent;

    wined3d_private_store_cleanup(&shader->private_store);
    free(parent);
}

static const struct wined3d_parent_ops d3d_vertex_shader_wined3d_parent_ops =
{
    d3d_vertex_shader_wined3d_object_destroyed,
};

static HRESULT d3d_vertex_shader_init(struct d3d_vertex_shader *shader, struct d3d_device *device,
        const void *byte_code, SIZE_T byte_code_length)
{
    struct wined3d_shader_desc desc;
    HRESULT hr;

    shader->ID3D11VertexShader_iface.lpVtbl = &d3d11_vertex_shader_vtbl;
    shader->ID3D10VertexShader_iface.lpVtbl = &d3d10_vertex_shader_vtbl;
    shader->refcount = 1;
    wined3d_mutex_lock();
    wined3d_private_store_init(&shader->private_store);

    desc.byte_code = byte_code;
    desc.byte_code_size = byte_code_length;
    if (FAILED(hr = wined3d_shader_create_vs(device->wined3d_device, &desc, shader,
            &d3d_vertex_shader_wined3d_parent_ops, &shader->wined3d_shader)))
    {
        WARN("Failed to create wined3d vertex shader, hr %#lx.\n", hr);
        wined3d_private_store_cleanup(&shader->private_store);
        wined3d_mutex_unlock();
        return E_INVALIDARG;
    }
    wined3d_mutex_unlock();

    ID3D11Device2_AddRef(shader->device = &device->ID3D11Device2_iface);

    return S_OK;
}

HRESULT d3d_vertex_shader_create(struct d3d_device *device, const void *byte_code, SIZE_T byte_code_length,
        struct d3d_vertex_shader **shader)
{
    struct d3d_vertex_shader *object;
    HRESULT hr;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d_vertex_shader_init(object, device, byte_code, byte_code_length)))
    {
        WARN("Failed to initialise vertex shader, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created vertex shader %p.\n", object);
    *shader = object;

    return S_OK;
}

struct d3d_vertex_shader *unsafe_impl_from_ID3D11VertexShader(ID3D11VertexShader *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d11_vertex_shader_vtbl);

    return impl_from_ID3D11VertexShader(iface);
}

struct d3d_vertex_shader *unsafe_impl_from_ID3D10VertexShader(ID3D10VertexShader *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d10_vertex_shader_vtbl);

    return impl_from_ID3D10VertexShader(iface);
}

/* ID3D11HullShader methods */

static inline struct d3d11_hull_shader *impl_from_ID3D11HullShader(ID3D11HullShader *iface)
{
    return CONTAINING_RECORD(iface, struct d3d11_hull_shader, ID3D11HullShader_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_hull_shader_QueryInterface(ID3D11HullShader *iface,
        REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11HullShader)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D11HullShader_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_hull_shader_AddRef(ID3D11HullShader *iface)
{
    struct d3d11_hull_shader *shader = impl_from_ID3D11HullShader(iface);
    ULONG refcount = InterlockedIncrement(&shader->refcount);

    TRACE("%p increasing refcount to %lu.\n", shader, refcount);

    if (refcount == 1)
    {
        ID3D11Device2_AddRef(shader->device);
        wined3d_shader_incref(shader->wined3d_shader);
    }

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_hull_shader_Release(ID3D11HullShader *iface)
{
    struct d3d11_hull_shader *shader = impl_from_ID3D11HullShader(iface);
    ULONG refcount = InterlockedDecrement(&shader->refcount);

    TRACE("%p decreasing refcount to %lu.\n", shader, refcount);

    if (!refcount)
    {
        ID3D11Device2 *device = shader->device;
        wined3d_shader_decref(shader->wined3d_shader);
        /* Release the device last, it may cause the wined3d device to be
         * destroyed. */
        ID3D11Device2_Release(device);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_hull_shader_GetDevice(ID3D11HullShader *iface,
        ID3D11Device **device)
{
    struct d3d11_hull_shader *shader = impl_from_ID3D11HullShader(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (ID3D11Device *)shader->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_hull_shader_GetPrivateData(ID3D11HullShader *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d11_hull_shader *shader = impl_from_ID3D11HullShader(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_hull_shader_SetPrivateData(ID3D11HullShader *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d11_hull_shader *shader = impl_from_ID3D11HullShader(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_hull_shader_SetPrivateDataInterface(ID3D11HullShader *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d11_hull_shader *shader = impl_from_ID3D11HullShader(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&shader->private_store, guid, data);
}

static const struct ID3D11HullShaderVtbl d3d11_hull_shader_vtbl =
{
    /* IUnknown methods */
    d3d11_hull_shader_QueryInterface,
    d3d11_hull_shader_AddRef,
    d3d11_hull_shader_Release,
    /* ID3D11DeviceChild methods */
    d3d11_hull_shader_GetDevice,
    d3d11_hull_shader_GetPrivateData,
    d3d11_hull_shader_SetPrivateData,
    d3d11_hull_shader_SetPrivateDataInterface,
};

static void STDMETHODCALLTYPE d3d11_hull_shader_wined3d_object_destroyed(void *parent)
{
    struct d3d11_hull_shader *shader = parent;

    wined3d_private_store_cleanup(&shader->private_store);
    free(parent);
}

static const struct wined3d_parent_ops d3d11_hull_shader_wined3d_parent_ops =
{
    d3d11_hull_shader_wined3d_object_destroyed,
};

static HRESULT d3d11_hull_shader_init(struct d3d11_hull_shader *shader, struct d3d_device *device,
        const void *byte_code, SIZE_T byte_code_length)
{
    struct wined3d_shader_desc desc;
    HRESULT hr;

    shader->ID3D11HullShader_iface.lpVtbl = &d3d11_hull_shader_vtbl;
    shader->refcount = 1;
    wined3d_mutex_lock();
    wined3d_private_store_init(&shader->private_store);

    desc.byte_code = byte_code;
    desc.byte_code_size = byte_code_length;
    if (FAILED(hr = wined3d_shader_create_hs(device->wined3d_device, &desc, shader,
            &d3d11_hull_shader_wined3d_parent_ops, &shader->wined3d_shader)))
    {
        WARN("Failed to create wined3d hull shader, hr %#lx.\n", hr);
        wined3d_private_store_cleanup(&shader->private_store);
        wined3d_mutex_unlock();
        return E_INVALIDARG;
    }
    wined3d_mutex_unlock();

    ID3D11Device2_AddRef(shader->device = &device->ID3D11Device2_iface);

    return S_OK;
}

HRESULT d3d11_hull_shader_create(struct d3d_device *device, const void *byte_code, SIZE_T byte_code_length,
        struct d3d11_hull_shader **shader)
{
    struct d3d11_hull_shader *object;
    HRESULT hr;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d11_hull_shader_init(object, device, byte_code, byte_code_length)))
    {
        free(object);
        return hr;
    }

    TRACE("Created hull shader %p.\n", object);
    *shader = object;

    return S_OK;
}

struct d3d11_hull_shader *unsafe_impl_from_ID3D11HullShader(ID3D11HullShader *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d11_hull_shader_vtbl);

    return impl_from_ID3D11HullShader(iface);
}

/* ID3D11DomainShader methods */

static inline struct d3d11_domain_shader *impl_from_ID3D11DomainShader(ID3D11DomainShader *iface)
{
    return CONTAINING_RECORD(iface, struct d3d11_domain_shader, ID3D11DomainShader_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_domain_shader_QueryInterface(ID3D11DomainShader *iface,
        REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11DomainShader)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D11DomainShader_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_domain_shader_AddRef(ID3D11DomainShader *iface)
{
    struct d3d11_domain_shader *shader = impl_from_ID3D11DomainShader(iface);
    ULONG refcount = InterlockedIncrement(&shader->refcount);

    TRACE("%p increasing refcount to %lu.\n", shader, refcount);

    if (refcount == 1)
    {
        ID3D11Device2_AddRef(shader->device);
        wined3d_shader_incref(shader->wined3d_shader);
    }

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_domain_shader_Release(ID3D11DomainShader *iface)
{
    struct d3d11_domain_shader *shader = impl_from_ID3D11DomainShader(iface);
    ULONG refcount = InterlockedDecrement(&shader->refcount);

    TRACE("%p decreasing refcount to %lu.\n", shader, refcount);

    if (!refcount)
    {
        ID3D11Device2 *device = shader->device;
        wined3d_shader_decref(shader->wined3d_shader);
        /* Release the device last, it may cause the wined3d device to be
         * destroyed. */
        ID3D11Device2_Release(device);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_domain_shader_GetDevice(ID3D11DomainShader *iface,
        ID3D11Device **device)
{
    struct d3d11_domain_shader *shader = impl_from_ID3D11DomainShader(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (ID3D11Device *)shader->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_domain_shader_GetPrivateData(ID3D11DomainShader *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d11_domain_shader *shader = impl_from_ID3D11DomainShader(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_domain_shader_SetPrivateData(ID3D11DomainShader *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d11_domain_shader *shader = impl_from_ID3D11DomainShader(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_domain_shader_SetPrivateDataInterface(ID3D11DomainShader *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d11_domain_shader *shader = impl_from_ID3D11DomainShader(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&shader->private_store, guid, data);
}

static const struct ID3D11DomainShaderVtbl d3d11_domain_shader_vtbl =
{
    /* IUnknown methods */
    d3d11_domain_shader_QueryInterface,
    d3d11_domain_shader_AddRef,
    d3d11_domain_shader_Release,
    /* ID3D11DeviceChild methods */
    d3d11_domain_shader_GetDevice,
    d3d11_domain_shader_GetPrivateData,
    d3d11_domain_shader_SetPrivateData,
    d3d11_domain_shader_SetPrivateDataInterface,
};

static void STDMETHODCALLTYPE d3d11_domain_shader_wined3d_object_destroyed(void *parent)
{
    struct d3d11_domain_shader *shader = parent;

    wined3d_private_store_cleanup(&shader->private_store);
    free(parent);
}

static const struct wined3d_parent_ops d3d11_domain_shader_wined3d_parent_ops =
{
    d3d11_domain_shader_wined3d_object_destroyed,
};

static HRESULT d3d11_domain_shader_init(struct d3d11_domain_shader *shader, struct d3d_device *device,
        const void *byte_code, SIZE_T byte_code_length)
{
    struct wined3d_shader_desc desc;
    HRESULT hr;

    shader->ID3D11DomainShader_iface.lpVtbl = &d3d11_domain_shader_vtbl;
    shader->refcount = 1;
    wined3d_mutex_lock();
    wined3d_private_store_init(&shader->private_store);

    desc.byte_code = byte_code;
    desc.byte_code_size = byte_code_length;
    if (FAILED(hr = wined3d_shader_create_ds(device->wined3d_device, &desc, shader,
            &d3d11_domain_shader_wined3d_parent_ops, &shader->wined3d_shader)))
    {
        WARN("Failed to create wined3d domain shader, hr %#lx.\n", hr);
        wined3d_private_store_cleanup(&shader->private_store);
        wined3d_mutex_unlock();
        return E_INVALIDARG;
    }
    wined3d_mutex_unlock();

    ID3D11Device2_AddRef(shader->device = &device->ID3D11Device2_iface);

    return S_OK;
}

HRESULT d3d11_domain_shader_create(struct d3d_device *device, const void *byte_code, SIZE_T byte_code_length,
        struct d3d11_domain_shader **shader)
{
    struct d3d11_domain_shader *object;
    HRESULT hr;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d11_domain_shader_init(object, device, byte_code, byte_code_length)))
    {
        free(object);
        return hr;
    }

    TRACE("Created domain shader %p.\n", object);
    *shader = object;

    return S_OK;
}

struct d3d11_domain_shader *unsafe_impl_from_ID3D11DomainShader(ID3D11DomainShader *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d11_domain_shader_vtbl);

    return impl_from_ID3D11DomainShader(iface);
}

/* ID3D11GeometryShader methods */

static inline struct d3d_geometry_shader *impl_from_ID3D11GeometryShader(ID3D11GeometryShader *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_geometry_shader, ID3D11GeometryShader_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_geometry_shader_QueryInterface(ID3D11GeometryShader *iface,
        REFIID riid, void **object)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D11GeometryShader(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11GeometryShader)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D11GeometryShader_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_ID3D10GeometryShader)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild))
    {
        ID3D10GeometryShader_AddRef(&shader->ID3D10GeometryShader_iface);
        *object = &shader->ID3D10GeometryShader_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_geometry_shader_AddRef(ID3D11GeometryShader *iface)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D11GeometryShader(iface);
    ULONG refcount = InterlockedIncrement(&shader->refcount);

    TRACE("%p increasing refcount to %lu.\n", shader, refcount);

    if (refcount == 1)
    {
        ID3D11Device2_AddRef(shader->device);
        wined3d_shader_incref(shader->wined3d_shader);
    }

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_geometry_shader_Release(ID3D11GeometryShader *iface)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D11GeometryShader(iface);
    ULONG refcount = InterlockedDecrement(&shader->refcount);

    TRACE("%p decreasing refcount to %lu.\n", shader, refcount);

    if (!refcount)
    {
        ID3D11Device2 *device = shader->device;
        wined3d_shader_decref(shader->wined3d_shader);
        /* Release the device last, it may cause the wined3d device to be
         * destroyed. */
        ID3D11Device2_Release(device);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_geometry_shader_GetDevice(ID3D11GeometryShader *iface,
        ID3D11Device **device)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D11GeometryShader(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (ID3D11Device *)shader->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_geometry_shader_GetPrivateData(ID3D11GeometryShader *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D11GeometryShader(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_geometry_shader_SetPrivateData(ID3D11GeometryShader *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D11GeometryShader(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_geometry_shader_SetPrivateDataInterface(ID3D11GeometryShader *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D11GeometryShader(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&shader->private_store, guid, data);
}

static const struct ID3D11GeometryShaderVtbl d3d11_geometry_shader_vtbl =
{
    /* IUnknown methods */
    d3d11_geometry_shader_QueryInterface,
    d3d11_geometry_shader_AddRef,
    d3d11_geometry_shader_Release,
    /* ID3D11DeviceChild methods */
    d3d11_geometry_shader_GetDevice,
    d3d11_geometry_shader_GetPrivateData,
    d3d11_geometry_shader_SetPrivateData,
    d3d11_geometry_shader_SetPrivateDataInterface,
};

/* ID3D10GeometryShader methods */

static inline struct d3d_geometry_shader *impl_from_ID3D10GeometryShader(ID3D10GeometryShader *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_geometry_shader, ID3D10GeometryShader_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_geometry_shader_QueryInterface(ID3D10GeometryShader *iface,
        REFIID riid, void **object)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D10GeometryShader(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return d3d11_geometry_shader_QueryInterface(&shader->ID3D11GeometryShader_iface, riid, object);
}

static ULONG STDMETHODCALLTYPE d3d10_geometry_shader_AddRef(ID3D10GeometryShader *iface)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D10GeometryShader(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_geometry_shader_AddRef(&shader->ID3D11GeometryShader_iface);
}

static ULONG STDMETHODCALLTYPE d3d10_geometry_shader_Release(ID3D10GeometryShader *iface)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D10GeometryShader(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_geometry_shader_Release(&shader->ID3D11GeometryShader_iface);
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_geometry_shader_GetDevice(ID3D10GeometryShader *iface, ID3D10Device **device)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D10GeometryShader(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device2_QueryInterface(shader->device, &IID_ID3D10Device, (void **)device);
}

static HRESULT STDMETHODCALLTYPE d3d10_geometry_shader_GetPrivateData(ID3D10GeometryShader *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D10GeometryShader(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_geometry_shader_SetPrivateData(ID3D10GeometryShader *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D10GeometryShader(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_geometry_shader_SetPrivateDataInterface(ID3D10GeometryShader *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_geometry_shader *shader = impl_from_ID3D10GeometryShader(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&shader->private_store, guid, data);
}

static const struct ID3D10GeometryShaderVtbl d3d10_geometry_shader_vtbl =
{
    /* IUnknown methods */
    d3d10_geometry_shader_QueryInterface,
    d3d10_geometry_shader_AddRef,
    d3d10_geometry_shader_Release,
    /* ID3D10DeviceChild methods */
    d3d10_geometry_shader_GetDevice,
    d3d10_geometry_shader_GetPrivateData,
    d3d10_geometry_shader_SetPrivateData,
    d3d10_geometry_shader_SetPrivateDataInterface,
};

static void STDMETHODCALLTYPE d3d_geometry_shader_wined3d_object_destroyed(void *parent)
{
    struct d3d_geometry_shader *shader = parent;

    wined3d_private_store_cleanup(&shader->private_store);
    free(parent);
}

static const struct wined3d_parent_ops d3d_geometry_shader_wined3d_parent_ops =
{
    d3d_geometry_shader_wined3d_object_destroyed,
};

static HRESULT validate_stream_output_entries(const D3D11_SO_DECLARATION_ENTRY *entries, unsigned int entry_count,
        const unsigned int *buffer_strides, unsigned int buffer_stride_count, D3D_FEATURE_LEVEL feature_level)
{
    unsigned int i, j;

    for (i = 0; i < entry_count; ++i)
    {
        const D3D11_SO_DECLARATION_ENTRY *e = &entries[i];

        TRACE("Stream: %u, semantic: %s, semantic idx: %u, start component: %u, "
                "component count %u, output slot %u.\n",
                e->Stream, debugstr_a(e->SemanticName), e->SemanticIndex,
                e->StartComponent, e->ComponentCount, e->OutputSlot);

        if (e->Stream >= D3D11_SO_STREAM_COUNT)
        {
            WARN("Invalid stream %u.\n", e->Stream);
            return E_INVALIDARG;
        }
        if (e->Stream && feature_level < D3D_FEATURE_LEVEL_11_0)
        {
            WARN("Invalid stream %u for feature level %#x.\n", e->Stream, feature_level);
            return E_INVALIDARG;
        }
        if (e->Stream)
        {
            FIXME("Streams not implemented yet.\n");
            return E_INVALIDARG;
        }
        if (e->OutputSlot >= D3D11_SO_BUFFER_SLOT_COUNT)
        {
            WARN("Invalid output slot %u.\n", e->OutputSlot);
            return E_INVALIDARG;
        }

        if (!e->SemanticName)
        {
            if (e->SemanticIndex)
            {
                WARN("Invalid semantic idx %u for stream output gap.\n", e->SemanticIndex);
                return E_INVALIDARG;
            }
            if (e->StartComponent || !e->ComponentCount)
            {
                WARN("Invalid stream output gap %u-%u.\n", e->StartComponent, e->ComponentCount);
                return E_INVALIDARG;
            }
        }
        else
        {
            if (e->StartComponent > 3 || e->ComponentCount > 4 || !e->ComponentCount
                    || e->StartComponent + e->ComponentCount > 4)
            {
                WARN("Invalid component range %u-%u.\n", e->StartComponent, e->ComponentCount);
                return E_INVALIDARG;
            }
        }
    }

    for (i = 0; i < entry_count; ++i)
    {
        const D3D11_SO_DECLARATION_ENTRY *e1 = &entries[i];
        if (!e1->SemanticName) /* gap */
            continue;

        for (j = i + 1; j < entry_count; ++j)
        {
            const D3D11_SO_DECLARATION_ENTRY *e2 = &entries[j];
            if (!e2->SemanticName) /* gap */
                continue;

            if (e1->Stream == e2->Stream
                    && !stricmp(e1->SemanticName, e2->SemanticName)
                    && e1->SemanticIndex == e2->SemanticIndex
                    && e1->StartComponent < e2->StartComponent + e2->ComponentCount
                    && e1->StartComponent + e1->ComponentCount > e2->StartComponent)
            {
                WARN("Stream output elements %u and %u overlap.\n", i, j);
                return E_INVALIDARG;
            }
        }
    }

    for (i = 0; i < D3D11_SO_STREAM_COUNT; ++i)
    {
        unsigned int current_stride[D3D11_SO_BUFFER_SLOT_COUNT] = {0};
        unsigned int element_count[D3D11_SO_BUFFER_SLOT_COUNT] = {0};
        unsigned int gap_count[D3D11_SO_BUFFER_SLOT_COUNT] = {0};

        for (j = 0; j < entry_count; ++j)
        {
            const D3D11_SO_DECLARATION_ENTRY *e = &entries[j];

            if (e->Stream != i)
                continue;
            current_stride[e->OutputSlot] += 4 * e->ComponentCount;
            ++element_count[e->OutputSlot];
            if (!e->SemanticName)
                ++gap_count[e->OutputSlot];
        }

        for (j = 0; j < D3D11_SO_BUFFER_SLOT_COUNT; ++j)
        {
            if (!element_count[j])
                continue;
            if (element_count[j] == gap_count[j])
            {
                WARN("Stream %u, output slot %u contains only gaps.\n", i, j);
                return E_INVALIDARG;
            }
            if (buffer_stride_count)
            {
                if (buffer_stride_count <= j)
                {
                    WARN("Buffer strides are required for all buffer slots.\n");
                    return E_INVALIDARG;
                }
                if (buffer_strides[j] < current_stride[j] || buffer_strides[j] % 4)
                {
                    WARN("Invalid stride %u for buffer slot %u.\n", buffer_strides[j], j);
                    return E_INVALIDARG;
                }
            }
        }

        if (!i && feature_level < D3D_FEATURE_LEVEL_11_0 && element_count[0] != entry_count)
        {
            for (j = 0; j < ARRAY_SIZE(element_count); ++j)
            {
                if (element_count[j] > 1)
                {
                    WARN("Only one element per output slot is allowed.\n");
                    return E_INVALIDARG;
                }
            }
        }
    }

    return S_OK;
}

static HRESULT d3d_geometry_shader_init(struct d3d_geometry_shader *shader,
        struct d3d_device *device, const void *byte_code, SIZE_T byte_code_length,
        const D3D11_SO_DECLARATION_ENTRY *so_entries, unsigned int so_entry_count,
        const unsigned int *buffer_strides, unsigned int buffer_stride_count,
        unsigned int rasterizer_stream)
{
    struct wined3d_stream_output_desc so_desc;
    struct wined3d_shader_desc desc;
    unsigned int i;
    HRESULT hr;

    if (so_entry_count > D3D11_SO_STREAM_COUNT * D3D11_SO_OUTPUT_COMPONENT_COUNT)
    {
        WARN("Entry count %u is greater than %u.\n",
                so_entry_count, D3D11_SO_STREAM_COUNT * D3D11_SO_OUTPUT_COMPONENT_COUNT);
        return E_INVALIDARG;
    }
    if (so_entries && !so_entry_count)
    {
        WARN("Invalid SO entry count %u.\n", so_entry_count);
        return E_INVALIDARG;
    }
    if (rasterizer_stream != D3D11_SO_NO_RASTERIZED_STREAM && rasterizer_stream >= D3D11_SO_STREAM_COUNT)
    {
        WARN("Invalid rasterizer stream %u.\n", rasterizer_stream);
        return E_INVALIDARG;
    }
    if (device->state->feature_level < D3D_FEATURE_LEVEL_11_0)
    {
        if (rasterizer_stream)
        {
            WARN("Invalid rasterizer stream %u for feature level %#x.\n",
                    rasterizer_stream, device->state->feature_level);
            return E_INVALIDARG;
        }
        if (buffer_stride_count && buffer_stride_count != 1)
        {
            WARN("Invalid buffer stride count %u for feature level %#x.\n",
                    buffer_stride_count, device->state->feature_level);
            return E_INVALIDARG;
        }
    }

    if (FAILED(hr = validate_stream_output_entries(so_entries, so_entry_count,
            buffer_strides, buffer_stride_count, device->state->feature_level)))
        return hr;

    desc.byte_code = byte_code;
    desc.byte_code_size = byte_code_length;

    memset(&so_desc, 0, sizeof(so_desc));
    if (so_entries)
    {
        so_desc.elements = (const struct wined3d_stream_output_element *)so_entries;
        so_desc.element_count = so_entry_count;
        for (i = 0; i < min(buffer_stride_count, ARRAY_SIZE(so_desc.buffer_strides)); ++i)
            so_desc.buffer_strides[i] = buffer_strides[i];
        so_desc.buffer_stride_count = buffer_stride_count;
        so_desc.rasterizer_stream_idx = rasterizer_stream;
    }

    shader->ID3D11GeometryShader_iface.lpVtbl = &d3d11_geometry_shader_vtbl;
    shader->ID3D10GeometryShader_iface.lpVtbl = &d3d10_geometry_shader_vtbl;
    shader->refcount = 1;
    wined3d_mutex_lock();
    wined3d_private_store_init(&shader->private_store);

    if (FAILED(hr = wined3d_shader_create_gs(device->wined3d_device, &desc, so_entries ? &so_desc : NULL,
            shader, &d3d_geometry_shader_wined3d_parent_ops, &shader->wined3d_shader)))
    {
        WARN("Failed to create wined3d geometry shader, hr %#lx.\n", hr);
        wined3d_private_store_cleanup(&shader->private_store);
        wined3d_mutex_unlock();
        return E_INVALIDARG;
    }
    wined3d_mutex_unlock();

    ID3D11Device2_AddRef(shader->device = &device->ID3D11Device2_iface);

    return S_OK;
}

HRESULT d3d_geometry_shader_create(struct d3d_device *device, const void *byte_code, SIZE_T byte_code_length,
        const D3D11_SO_DECLARATION_ENTRY *so_entries, unsigned int so_entry_count,
        const unsigned int *buffer_strides, unsigned int buffer_stride_count, unsigned int rasterizer_stream,
        struct d3d_geometry_shader **shader)
{
    struct d3d_geometry_shader *object;
    HRESULT hr;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d_geometry_shader_init(object, device, byte_code, byte_code_length,
            so_entries, so_entry_count, buffer_strides, buffer_stride_count, rasterizer_stream)))
    {
        WARN("Failed to initialise geometry shader, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created geometry shader %p.\n", object);
    *shader = object;

    return S_OK;
}

struct d3d_geometry_shader *unsafe_impl_from_ID3D11GeometryShader(ID3D11GeometryShader *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d11_geometry_shader_vtbl);

    return impl_from_ID3D11GeometryShader(iface);
}

struct d3d_geometry_shader *unsafe_impl_from_ID3D10GeometryShader(ID3D10GeometryShader *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d10_geometry_shader_vtbl);

    return impl_from_ID3D10GeometryShader(iface);
}

/* ID3D11PixelShader methods */

static inline struct d3d_pixel_shader *impl_from_ID3D11PixelShader(ID3D11PixelShader *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_pixel_shader, ID3D11PixelShader_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_pixel_shader_QueryInterface(ID3D11PixelShader *iface,
        REFIID riid, void **object)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D11PixelShader(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11PixelShader)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D11PixelShader_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_ID3D10PixelShader)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild))
    {
        IUnknown_AddRef(&shader->ID3D10PixelShader_iface);
        *object = &shader->ID3D10PixelShader_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_pixel_shader_AddRef(ID3D11PixelShader *iface)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D11PixelShader(iface);
    ULONG refcount = InterlockedIncrement(&shader->refcount);

    TRACE("%p increasing refcount to %lu.\n", shader, refcount);

    if (refcount == 1)
    {
        ID3D11Device2_AddRef(shader->device);
        wined3d_shader_incref(shader->wined3d_shader);
    }

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_pixel_shader_Release(ID3D11PixelShader *iface)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D11PixelShader(iface);
    ULONG refcount = InterlockedDecrement(&shader->refcount);

    TRACE("%p decreasing refcount to %lu.\n", shader, refcount);

    if (!refcount)
    {
        ID3D11Device2 *device = shader->device;
        wined3d_shader_decref(shader->wined3d_shader);
        /* Release the device last, it may cause the wined3d device to be
         * destroyed. */
        ID3D11Device2_Release(device);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_pixel_shader_GetDevice(ID3D11PixelShader *iface,
        ID3D11Device **device)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D11PixelShader(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (ID3D11Device *)shader->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_pixel_shader_GetPrivateData(ID3D11PixelShader *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D11PixelShader(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_pixel_shader_SetPrivateData(ID3D11PixelShader *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D11PixelShader(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_pixel_shader_SetPrivateDataInterface(ID3D11PixelShader *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D11PixelShader(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&shader->private_store, guid, data);
}

static const struct ID3D11PixelShaderVtbl d3d11_pixel_shader_vtbl =
{
    /* IUnknown methods */
    d3d11_pixel_shader_QueryInterface,
    d3d11_pixel_shader_AddRef,
    d3d11_pixel_shader_Release,
    /* ID3D11DeviceChild methods */
    d3d11_pixel_shader_GetDevice,
    d3d11_pixel_shader_GetPrivateData,
    d3d11_pixel_shader_SetPrivateData,
    d3d11_pixel_shader_SetPrivateDataInterface,
};

/* ID3D10PixelShader methods */

static inline struct d3d_pixel_shader *impl_from_ID3D10PixelShader(ID3D10PixelShader *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_pixel_shader, ID3D10PixelShader_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_pixel_shader_QueryInterface(ID3D10PixelShader *iface,
        REFIID riid, void **object)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D10PixelShader(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return d3d11_pixel_shader_QueryInterface(&shader->ID3D11PixelShader_iface, riid, object);
}

static ULONG STDMETHODCALLTYPE d3d10_pixel_shader_AddRef(ID3D10PixelShader *iface)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D10PixelShader(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_pixel_shader_AddRef(&shader->ID3D11PixelShader_iface);
}

static ULONG STDMETHODCALLTYPE d3d10_pixel_shader_Release(ID3D10PixelShader *iface)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D10PixelShader(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_pixel_shader_Release(&shader->ID3D11PixelShader_iface);
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_pixel_shader_GetDevice(ID3D10PixelShader *iface, ID3D10Device **device)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D10PixelShader(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device2_QueryInterface(shader->device, &IID_ID3D10Device, (void **)device);
}

static HRESULT STDMETHODCALLTYPE d3d10_pixel_shader_GetPrivateData(ID3D10PixelShader *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D10PixelShader(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_pixel_shader_SetPrivateData(ID3D10PixelShader *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D10PixelShader(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_pixel_shader_SetPrivateDataInterface(ID3D10PixelShader *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_pixel_shader *shader = impl_from_ID3D10PixelShader(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&shader->private_store, guid, data);
}

static const struct ID3D10PixelShaderVtbl d3d10_pixel_shader_vtbl =
{
    /* IUnknown methods */
    d3d10_pixel_shader_QueryInterface,
    d3d10_pixel_shader_AddRef,
    d3d10_pixel_shader_Release,
    /* ID3D10DeviceChild methods */
    d3d10_pixel_shader_GetDevice,
    d3d10_pixel_shader_GetPrivateData,
    d3d10_pixel_shader_SetPrivateData,
    d3d10_pixel_shader_SetPrivateDataInterface,
};

static void STDMETHODCALLTYPE d3d_pixel_shader_wined3d_object_destroyed(void *parent)
{
    struct d3d_pixel_shader *shader = parent;

    wined3d_private_store_cleanup(&shader->private_store);
    free(parent);
}

static const struct wined3d_parent_ops d3d_pixel_shader_wined3d_parent_ops =
{
    d3d_pixel_shader_wined3d_object_destroyed,
};

static HRESULT d3d_pixel_shader_init(struct d3d_pixel_shader *shader, struct d3d_device *device,
        const void *byte_code, SIZE_T byte_code_length)
{
    struct wined3d_shader_desc desc;
    HRESULT hr;

    shader->ID3D11PixelShader_iface.lpVtbl = &d3d11_pixel_shader_vtbl;
    shader->ID3D10PixelShader_iface.lpVtbl = &d3d10_pixel_shader_vtbl;
    shader->refcount = 1;
    wined3d_mutex_lock();
    wined3d_private_store_init(&shader->private_store);

    desc.byte_code = byte_code;
    desc.byte_code_size = byte_code_length;
    if (FAILED(hr = wined3d_shader_create_ps(device->wined3d_device, &desc, shader,
            &d3d_pixel_shader_wined3d_parent_ops, &shader->wined3d_shader)))
    {
        WARN("Failed to create wined3d pixel shader, hr %#lx.\n", hr);
        wined3d_private_store_cleanup(&shader->private_store);
        wined3d_mutex_unlock();
        return E_INVALIDARG;
    }
    wined3d_mutex_unlock();

    ID3D11Device2_AddRef(shader->device = &device->ID3D11Device2_iface);

    return S_OK;
}

HRESULT d3d_pixel_shader_create(struct d3d_device *device, const void *byte_code, SIZE_T byte_code_length,
        struct d3d_pixel_shader **shader)
{
    struct d3d_pixel_shader *object;
    HRESULT hr;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d_pixel_shader_init(object, device, byte_code, byte_code_length)))
    {
        WARN("Failed to initialise pixel shader, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created pixel shader %p.\n", object);
    *shader = object;

    return S_OK;
}

struct d3d_pixel_shader *unsafe_impl_from_ID3D11PixelShader(ID3D11PixelShader *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d11_pixel_shader_vtbl);

    return impl_from_ID3D11PixelShader(iface);
}

struct d3d_pixel_shader *unsafe_impl_from_ID3D10PixelShader(ID3D10PixelShader *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d10_pixel_shader_vtbl);

    return impl_from_ID3D10PixelShader(iface);
}

/* ID3D11ComputeShader methods */

static inline struct d3d11_compute_shader *impl_from_ID3D11ComputeShader(ID3D11ComputeShader *iface)
{
    return CONTAINING_RECORD(iface, struct d3d11_compute_shader, ID3D11ComputeShader_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_compute_shader_QueryInterface(ID3D11ComputeShader *iface,
        REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11ComputeShader)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D11ComputeShader_AddRef(*object = iface);
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_compute_shader_AddRef(ID3D11ComputeShader *iface)
{
    struct d3d11_compute_shader *shader = impl_from_ID3D11ComputeShader(iface);
    ULONG refcount = InterlockedIncrement(&shader->refcount);

    TRACE("%p increasing refcount to %lu.\n", shader, refcount);

    if (refcount == 1)
    {
        ID3D11Device2_AddRef(shader->device);
        wined3d_shader_incref(shader->wined3d_shader);
    }

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_compute_shader_Release(ID3D11ComputeShader *iface)
{
    struct d3d11_compute_shader *shader = impl_from_ID3D11ComputeShader(iface);
    ULONG refcount = InterlockedDecrement(&shader->refcount);

    TRACE("%p decreasing refcount to %lu.\n", shader, refcount);

    if (!refcount)
    {
        ID3D11Device2 *device = shader->device;
        wined3d_shader_decref(shader->wined3d_shader);
        /* Release the device last, it may cause the wined3d device to be
         * destroyed. */
        ID3D11Device2_Release(device);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_compute_shader_GetDevice(ID3D11ComputeShader *iface,
        ID3D11Device **device)
{
    struct d3d11_compute_shader *shader = impl_from_ID3D11ComputeShader(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device_AddRef(*device = (ID3D11Device *)shader->device);
}

static HRESULT STDMETHODCALLTYPE d3d11_compute_shader_GetPrivateData(ID3D11ComputeShader *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d11_compute_shader *shader = impl_from_ID3D11ComputeShader(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_compute_shader_SetPrivateData(ID3D11ComputeShader *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d11_compute_shader *shader = impl_from_ID3D11ComputeShader(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&shader->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_compute_shader_SetPrivateDataInterface(ID3D11ComputeShader *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d11_compute_shader *shader = impl_from_ID3D11ComputeShader(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&shader->private_store, guid, data);
}

static const struct ID3D11ComputeShaderVtbl d3d11_compute_shader_vtbl =
{
    /* IUnknown methods */
    d3d11_compute_shader_QueryInterface,
    d3d11_compute_shader_AddRef,
    d3d11_compute_shader_Release,
    /* ID3D11DeviceChild methods */
    d3d11_compute_shader_GetDevice,
    d3d11_compute_shader_GetPrivateData,
    d3d11_compute_shader_SetPrivateData,
    d3d11_compute_shader_SetPrivateDataInterface,
};

static void STDMETHODCALLTYPE d3d11_compute_shader_wined3d_object_destroyed(void *parent)
{
    struct d3d11_compute_shader *shader = parent;

    wined3d_private_store_cleanup(&shader->private_store);
    free(parent);
}

static const struct wined3d_parent_ops d3d11_compute_shader_wined3d_parent_ops =
{
    d3d11_compute_shader_wined3d_object_destroyed,
};

static HRESULT d3d11_compute_shader_init(struct d3d11_compute_shader *shader, struct d3d_device *device,
        const void *byte_code, SIZE_T byte_code_length)
{
    struct wined3d_shader_desc desc;
    HRESULT hr;

    shader->ID3D11ComputeShader_iface.lpVtbl = &d3d11_compute_shader_vtbl;
    shader->refcount = 1;
    wined3d_mutex_lock();
    wined3d_private_store_init(&shader->private_store);

    desc.byte_code = byte_code;
    desc.byte_code_size = byte_code_length;
    if (FAILED(hr = wined3d_shader_create_cs(device->wined3d_device, &desc, shader,
            &d3d11_compute_shader_wined3d_parent_ops, &shader->wined3d_shader)))
    {
        WARN("Failed to create wined3d compute shader, hr %#lx.\n", hr);
        wined3d_private_store_cleanup(&shader->private_store);
        wined3d_mutex_unlock();
        return E_INVALIDARG;
    }
    wined3d_mutex_unlock();

    ID3D11Device2_AddRef(shader->device = &device->ID3D11Device2_iface);

    return S_OK;
}

HRESULT d3d11_compute_shader_create(struct d3d_device *device, const void *byte_code, SIZE_T byte_code_length,
        struct d3d11_compute_shader **shader)
{
    struct d3d11_compute_shader *object;
    HRESULT hr;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d11_compute_shader_init(object, device, byte_code, byte_code_length)))
    {
        free(object);
        return hr;
    }

    TRACE("Created compute shader %p.\n", object);
    *shader = object;

    return S_OK;
}

struct d3d11_compute_shader *unsafe_impl_from_ID3D11ComputeShader(ID3D11ComputeShader *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d11_compute_shader_vtbl);

    return impl_from_ID3D11ComputeShader(iface);
}

/* ID3D11ClassLinkage methods */

static inline struct d3d11_class_linkage *impl_from_ID3D11ClassLinkage(ID3D11ClassLinkage *iface)
{
    return CONTAINING_RECORD(iface, struct d3d11_class_linkage, ID3D11ClassLinkage_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_class_linkage_QueryInterface(ID3D11ClassLinkage *iface,
        REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11ClassLinkage)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D11ClassLinkage_AddRef(*object = iface);
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_class_linkage_AddRef(ID3D11ClassLinkage *iface)
{
    struct d3d11_class_linkage *class_linkage = impl_from_ID3D11ClassLinkage(iface);
    ULONG refcount = InterlockedIncrement(&class_linkage->refcount);

    TRACE("%p increasing refcount to %lu.\n", class_linkage, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_class_linkage_Release(ID3D11ClassLinkage *iface)
{
    struct d3d11_class_linkage *class_linkage = impl_from_ID3D11ClassLinkage(iface);
    ULONG refcount = InterlockedDecrement(&class_linkage->refcount);

    TRACE("%p decreasing refcount to %lu.\n", class_linkage, refcount);

    if (!refcount)
    {
        ID3D11Device2 *device = class_linkage->device;

        wined3d_private_store_cleanup(&class_linkage->private_store);
        free(class_linkage);

        ID3D11Device2_Release(device);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_class_linkage_GetDevice(ID3D11ClassLinkage *iface,
        ID3D11Device **device)
{
    struct d3d11_class_linkage *class_linkage = impl_from_ID3D11ClassLinkage(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device_AddRef(*device = (ID3D11Device *)class_linkage->device);
}

static HRESULT STDMETHODCALLTYPE d3d11_class_linkage_GetPrivateData(ID3D11ClassLinkage *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d11_class_linkage *class_linkage = impl_from_ID3D11ClassLinkage(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&class_linkage->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_class_linkage_SetPrivateData(ID3D11ClassLinkage *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d11_class_linkage *class_linkage = impl_from_ID3D11ClassLinkage(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&class_linkage->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_class_linkage_SetPrivateDataInterface(ID3D11ClassLinkage *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d11_class_linkage *class_linkage = impl_from_ID3D11ClassLinkage(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&class_linkage->private_store, guid, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_class_linkage_GetClassInstance(ID3D11ClassLinkage *iface,
        const char *instance_name, UINT instance_index, ID3D11ClassInstance **class_instance)
{
    FIXME("iface %p, instance_name %s, instance_index %u, class_instance %p stub!\n",
            iface, debugstr_a(instance_name), instance_index, class_instance);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_class_linkage_CreateClassInstance(ID3D11ClassLinkage *iface,
        const char *type_name, UINT cb_offset, UINT cb_vector_offset, UINT texture_offset,
        UINT sampler_offset, ID3D11ClassInstance **class_instance)
{
    FIXME("iface %p, type_name %s, cb_offset %u, cb_vector_offset %u, texture_offset %u, "
            "sampler_offset %u, class_instance %p stub!\n",
            iface, debugstr_a(type_name), cb_offset, cb_vector_offset, texture_offset,
            sampler_offset, class_instance);

    return E_NOTIMPL;
}

static const struct ID3D11ClassLinkageVtbl d3d11_class_linkage_vtbl =
{
    /* IUnknown methods */
    d3d11_class_linkage_QueryInterface,
    d3d11_class_linkage_AddRef,
    d3d11_class_linkage_Release,
    /* ID3D11DeviceChild methods */
    d3d11_class_linkage_GetDevice,
    d3d11_class_linkage_GetPrivateData,
    d3d11_class_linkage_SetPrivateData,
    d3d11_class_linkage_SetPrivateDataInterface,
    /* ID3D11ClassLinkage methods */
    d3d11_class_linkage_GetClassInstance,
    d3d11_class_linkage_CreateClassInstance,
};

HRESULT d3d11_class_linkage_create(struct d3d_device *device, struct d3d11_class_linkage **class_linkage)
{
    struct d3d11_class_linkage *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->ID3D11ClassLinkage_iface.lpVtbl = &d3d11_class_linkage_vtbl;
    object->refcount = 1;
    wined3d_private_store_init(&object->private_store);

    ID3D11Device2_AddRef(object->device = &device->ID3D11Device2_iface);

    TRACE("Created class linkage %p.\n", object);
    *class_linkage = object;

    return S_OK;
}
