/*
 * Copyright 2008 Henri Verbeet for CodeWeavers
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

#include "dxgi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxgi);

static inline struct dxgi_adapter *impl_from_IWineDXGIAdapter(IWineDXGIAdapter *iface)
{
    return CONTAINING_RECORD(iface, struct dxgi_adapter, IWineDXGIAdapter_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE dxgi_adapter_QueryInterface(IWineDXGIAdapter *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IDXGIObject)
            || IsEqualGUID(riid, &IID_IDXGIAdapter)
            || IsEqualGUID(riid, &IID_IWineDXGIAdapter))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dxgi_adapter_AddRef(IWineDXGIAdapter *iface)
{
    struct dxgi_adapter *This = impl_from_IWineDXGIAdapter(iface);
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxgi_adapter_Release(IWineDXGIAdapter *iface)
{
    struct dxgi_adapter *This = impl_from_IWineDXGIAdapter(iface);
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %u\n", This, refcount);

    if (!refcount)
    {
        IDXGIOutput_Release(This->output);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refcount;
}

/* IDXGIObject methods */

static HRESULT STDMETHODCALLTYPE dxgi_adapter_SetPrivateData(IWineDXGIAdapter *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    FIXME("iface %p, guid %s, data_size %u, data %p stub!\n", iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_SetPrivateDataInterface(IWineDXGIAdapter *iface,
        REFGUID guid, const IUnknown *object)
{
    FIXME("iface %p, guid %s, object %p stub!\n", iface, debugstr_guid(guid), object);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetPrivateData(IWineDXGIAdapter *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    FIXME("iface %p, guid %s, data_size %p, data %p stub!\n", iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetParent(IWineDXGIAdapter *iface, REFIID riid, void **parent)
{
    struct dxgi_adapter *This = impl_from_IWineDXGIAdapter(iface);

    TRACE("iface %p, riid %s, parent %p\n", iface, debugstr_guid(riid), parent);

    return IWineDXGIFactory_QueryInterface(This->parent, riid, parent);
}

/* IDXGIAdapter methods */

static HRESULT STDMETHODCALLTYPE dxgi_adapter_EnumOutputs(IWineDXGIAdapter *iface,
        UINT output_idx, IDXGIOutput **output)
{
    struct dxgi_adapter *This = impl_from_IWineDXGIAdapter(iface);

    TRACE("iface %p, output_idx %u, output %p.\n", iface, output_idx, output);

    if (output_idx > 0)
    {
        *output = NULL;
        return DXGI_ERROR_NOT_FOUND;
    }

    *output = This->output;
    IDXGIOutput_AddRef(*output);

    TRACE("Returning output %p.\n", output);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetDesc(IWineDXGIAdapter *iface, DXGI_ADAPTER_DESC *desc)
{
    struct dxgi_adapter *This = impl_from_IWineDXGIAdapter(iface);
    struct wined3d_adapter_identifier adapter_id;
    char description[128];
    struct wined3d *wined3d;
    HRESULT hr;

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc) return E_INVALIDARG;

    wined3d = IWineDXGIFactory_get_wined3d(This->parent);
    adapter_id.driver_size = 0;
    adapter_id.description = description;
    adapter_id.description_size = sizeof(description);
    adapter_id.device_name_size = 0;

    EnterCriticalSection(&dxgi_cs);
    hr = wined3d_get_adapter_identifier(wined3d, This->ordinal, 0, &adapter_id);
    wined3d_decref(wined3d);
    LeaveCriticalSection(&dxgi_cs);

    if (SUCCEEDED(hr))
    {
        if (!MultiByteToWideChar(CP_ACP, 0, description, -1, desc->Description, 128))
        {
            DWORD err = GetLastError();
            ERR("Failed to translate description %s (%#x).\n", debugstr_a(description), err);
            hr = E_FAIL;
        }

        desc->VendorId = adapter_id.vendor_id;
        desc->DeviceId = adapter_id.device_id;
        desc->SubSysId = adapter_id.subsystem_id;
        desc->Revision = adapter_id.revision;
        desc->DedicatedVideoMemory = adapter_id.video_memory;
        desc->DedicatedSystemMemory = 0; /* FIXME */
        desc->SharedSystemMemory = 0; /* FIXME */
        memcpy(&desc->AdapterLuid, &adapter_id.adapter_luid, sizeof(desc->AdapterLuid));
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_CheckInterfaceSupport(IWineDXGIAdapter *iface,
        REFGUID guid, LARGE_INTEGER *umd_version)
{
    FIXME("iface %p, guid %s, umd_version %p stub!\n", iface, debugstr_guid(guid), umd_version);

    return E_NOTIMPL;
}

/* IWineDXGIAdapter methods */

static UINT STDMETHODCALLTYPE dxgi_adapter_get_ordinal(IWineDXGIAdapter *iface)
{
    struct dxgi_adapter *This = impl_from_IWineDXGIAdapter(iface);

    TRACE("iface %p, returning %u\n", iface, This->ordinal);

    return This->ordinal;
}

static const struct IWineDXGIAdapterVtbl dxgi_adapter_vtbl =
{
    /* IUnknown methods */
    dxgi_adapter_QueryInterface,
    dxgi_adapter_AddRef,
    dxgi_adapter_Release,
    /* IDXGIObject methods */
    dxgi_adapter_SetPrivateData,
    dxgi_adapter_SetPrivateDataInterface,
    dxgi_adapter_GetPrivateData,
    dxgi_adapter_GetParent,
    /* IDXGIAdapter methods */
    dxgi_adapter_EnumOutputs,
    dxgi_adapter_GetDesc,
    dxgi_adapter_CheckInterfaceSupport,
    /* IWineDXGIAdapter methods */
    dxgi_adapter_get_ordinal,
};

HRESULT dxgi_adapter_init(struct dxgi_adapter *adapter, IWineDXGIFactory *parent, UINT ordinal)
{
    struct dxgi_output *output;

    adapter->IWineDXGIAdapter_iface.lpVtbl = &dxgi_adapter_vtbl;
    adapter->parent = parent;
    adapter->refcount = 1;
    adapter->ordinal = ordinal;

    output = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*output));
    if (!output)
    {
        return E_OUTOFMEMORY;
    }
    dxgi_output_init(output, adapter);
    adapter->output = &output->IDXGIOutput_iface;

    return S_OK;
}
