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

static HRESULT STDMETHODCALLTYPE dxgi_adapter_QueryInterface(IWineDXGIAdapter *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IWineDXGIAdapter)
            || IsEqualGUID(iid, &IID_IDXGIAdapter4)
            || IsEqualGUID(iid, &IID_IDXGIAdapter3)
            || IsEqualGUID(iid, &IID_IDXGIAdapter2)
            || IsEqualGUID(iid, &IID_IDXGIAdapter1)
            || IsEqualGUID(iid, &IID_IDXGIAdapter)
            || IsEqualGUID(iid, &IID_IDXGIObject)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dxgi_adapter_AddRef(IWineDXGIAdapter *iface)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);
    ULONG refcount = InterlockedIncrement(&adapter->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxgi_adapter_Release(IWineDXGIAdapter *iface)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);
    ULONG refcount = InterlockedDecrement(&adapter->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        wined3d_private_store_cleanup(&adapter->private_store);
        IWineDXGIFactory_Release(&adapter->factory->IWineDXGIFactory_iface);
        heap_free(adapter);
    }

    return refcount;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_SetPrivateData(IWineDXGIAdapter *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_set_private_data(&adapter->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_SetPrivateDataInterface(IWineDXGIAdapter *iface,
        REFGUID guid, const IUnknown *object)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);

    TRACE("iface %p, guid %s, object %p.\n", iface, debugstr_guid(guid), object);

    return dxgi_set_private_data_interface(&adapter->private_store, guid, object);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetPrivateData(IWineDXGIAdapter *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_get_private_data(&adapter->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetParent(IWineDXGIAdapter *iface, REFIID iid, void **parent)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);

    TRACE("iface %p, iid %s, parent %p.\n", iface, debugstr_guid(iid), parent);

    return IWineDXGIFactory_QueryInterface(&adapter->factory->IWineDXGIFactory_iface, iid, parent);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_EnumOutputs(IWineDXGIAdapter *iface,
        UINT output_idx, IDXGIOutput **output)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);
    struct dxgi_output *output_object;
    HRESULT hr;

    TRACE("iface %p, output_idx %u, output %p.\n", iface, output_idx, output);

    if (output_idx > 0)
    {
        *output = NULL;
        return DXGI_ERROR_NOT_FOUND;
    }

    if (FAILED(hr = dxgi_output_create(adapter, &output_object)))
    {
        *output = NULL;
        return hr;
    }

    *output = (IDXGIOutput *)&output_object->IDXGIOutput4_iface;

    TRACE("Returning output %p.\n", *output);

    return S_OK;
}

static HRESULT dxgi_adapter_get_desc(struct dxgi_adapter *adapter, DXGI_ADAPTER_DESC3 *desc)
{
    char description[ARRAY_SIZE(desc->Description)];
    struct wined3d_adapter_identifier adapter_id;
    HRESULT hr;

    adapter_id.driver_size = 0;
    adapter_id.description = description;
    adapter_id.description_size = sizeof(description);
    adapter_id.device_name_size = 0;

    if (FAILED(hr = wined3d_get_adapter_identifier(adapter->factory->wined3d, adapter->ordinal, 0, &adapter_id)))
        return hr;

    if (!MultiByteToWideChar(CP_ACP, 0, description, -1, desc->Description, ARRAY_SIZE(description)))
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
    desc->SharedSystemMemory = adapter_id.shared_system_memory;
    desc->AdapterLuid = adapter_id.adapter_luid;
    desc->Flags = 0;
    desc->GraphicsPreemptionGranularity = 0; /* FIXME */
    desc->ComputePreemptionGranularity = 0; /* FIXME */

    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetDesc(IWineDXGIAdapter *iface, DXGI_ADAPTER_DESC *desc)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);
    DXGI_ADAPTER_DESC3 desc3;
    HRESULT hr;

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
        return E_INVALIDARG;

    if (SUCCEEDED(hr = dxgi_adapter_get_desc(adapter, &desc3)))
        memcpy(desc, &desc3, sizeof(*desc));

    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_CheckInterfaceSupport(IWineDXGIAdapter *iface,
        REFGUID guid, LARGE_INTEGER *umd_version)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);
    struct wined3d_adapter_identifier adapter_id;
    struct wined3d_caps caps;
    struct wined3d *wined3d;
    HRESULT hr;

    TRACE("iface %p, guid %s, umd_version %p.\n", iface, debugstr_guid(guid), umd_version);

    /* This method works only for D3D10 interfaces. */
    if (!(IsEqualGUID(guid, &IID_IDXGIDevice)
            || IsEqualGUID(guid, &IID_ID3D10Device)
            || IsEqualGUID(guid, &IID_ID3D10Device1)))
    {
        WARN("Returning DXGI_ERROR_UNSUPPORTED for %s.\n", debugstr_guid(guid));
        return DXGI_ERROR_UNSUPPORTED;
    }

    adapter_id.driver_size = 0;
    adapter_id.description_size = 0;
    adapter_id.device_name_size = 0;

    wined3d_mutex_lock();
    wined3d = adapter->factory->wined3d;
    hr = wined3d_get_device_caps(wined3d, adapter->ordinal, WINED3D_DEVICE_TYPE_HAL, &caps);
    if (SUCCEEDED(hr))
        hr = wined3d_get_adapter_identifier(wined3d, adapter->ordinal, 0, &adapter_id);
    wined3d_mutex_unlock();

    if (FAILED(hr))
        return hr;
    if (caps.max_feature_level < WINED3D_FEATURE_LEVEL_10)
        return DXGI_ERROR_UNSUPPORTED;

    if (umd_version)
        *umd_version = adapter_id.driver_version;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetDesc1(IWineDXGIAdapter *iface, DXGI_ADAPTER_DESC1 *desc)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);
    DXGI_ADAPTER_DESC3 desc3;
    HRESULT hr;

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
        return E_INVALIDARG;

    if (SUCCEEDED(hr = dxgi_adapter_get_desc(adapter, &desc3)))
        memcpy(desc, &desc3, sizeof(*desc));

    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetDesc2(IWineDXGIAdapter *iface, DXGI_ADAPTER_DESC2 *desc)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);
    DXGI_ADAPTER_DESC3 desc3;
    HRESULT hr;

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
        return E_INVALIDARG;

    if (SUCCEEDED(hr = dxgi_adapter_get_desc(adapter, &desc3)))
        memcpy(desc, &desc3, sizeof(*desc));

    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_RegisterHardwareContentProtectionTeardownStatusEvent(
        IWineDXGIAdapter *iface, HANDLE event, DWORD *cookie)
{
    FIXME("iface %p, event %p, cookie %p stub!\n", iface, event, cookie);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE dxgi_adapter_UnregisterHardwareContentProtectionTeardownStatus(
        IWineDXGIAdapter *iface, DWORD cookie)
{
    FIXME("iface %p, cookie %#x stub!\n", iface, cookie);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_QueryVideoMemoryInfo(IWineDXGIAdapter *iface,
        UINT node_index, DXGI_MEMORY_SEGMENT_GROUP segment_group, DXGI_QUERY_VIDEO_MEMORY_INFO *info)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);
    struct wined3d_adapter_identifier adapter_id;
    static unsigned int once;
    HRESULT hr;

    TRACE("iface %p, node_index %u, segment_group %#x, info %p.\n",
            iface, node_index, segment_group, info);

    if (!once++)
        FIXME("Returning fake video memory info.\n");

    if (node_index)
        FIXME("Ignoring node index %u.\n", node_index);

    adapter_id.driver_size = 0;
    adapter_id.description_size = 0;
    adapter_id.device_name_size = 0;

    if (FAILED(hr = wined3d_get_adapter_identifier(adapter->factory->wined3d, adapter->ordinal, 0, &adapter_id)))
        return hr;

    switch (segment_group)
    {
        case DXGI_MEMORY_SEGMENT_GROUP_LOCAL:
            info->Budget = adapter_id.video_memory;
            info->CurrentUsage = 0;
            info->AvailableForReservation = adapter_id.video_memory / 2;
            info->CurrentReservation = 0;
            break;
        case DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL:
            memset(info, 0, sizeof(*info));
            break;
        default:
            WARN("Invalid memory segment group %#x.\n", segment_group);
            return E_INVALIDARG;
    }

    TRACE("Budget 0x%s, usage 0x%s, available for reservation 0x%s, reservation 0x%s.\n",
            wine_dbgstr_longlong(info->Budget), wine_dbgstr_longlong(info->CurrentUsage),
            wine_dbgstr_longlong(info->AvailableForReservation), wine_dbgstr_longlong(info->CurrentReservation));

    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_SetVideoMemoryReservation(IWineDXGIAdapter *iface,
        UINT node_index, DXGI_MEMORY_SEGMENT_GROUP segment_group, UINT64 reservation)
{
    FIXME("iface %p, node_index %u, segment_group %#x, reservation 0x%s stub!\n",
            iface, node_index, segment_group, wine_dbgstr_longlong(reservation));

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_RegisterVideoMemoryBudgetChangeNotificationEvent(
        IWineDXGIAdapter *iface, HANDLE event, DWORD *cookie)
{
    FIXME("iface %p, event %p, cookie %p stub!\n", iface, event, cookie);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE dxgi_adapter_UnregisterVideoMemoryBudgetChangeNotification(
        IWineDXGIAdapter *iface, DWORD cookie)
{
    FIXME("iface %p, cookie %#x stub!\n", iface, cookie);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetDesc3(IWineDXGIAdapter *iface, DXGI_ADAPTER_DESC3 *desc)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
        return E_INVALIDARG;

    return dxgi_adapter_get_desc(adapter, desc);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_get_adapter_info(IWineDXGIAdapter *iface,
        struct wine_dxgi_adapter_info *info)
{
    struct dxgi_adapter *adapter = impl_from_IWineDXGIAdapter(iface);
    struct wined3d_adapter_identifier adapter_id;
    HRESULT hr;

    TRACE("iface %p, info %p.\n", iface, info);

    memset(&adapter_id, 0, sizeof(adapter_id));
    if (SUCCEEDED(hr = wined3d_get_adapter_identifier(adapter->factory->wined3d, adapter->ordinal, 0, &adapter_id)))
    {
        info->driver_uuid = adapter_id.driver_uuid;
        info->device_uuid = adapter_id.device_uuid;
        info->vendor_id = adapter_id.vendor_id;
        info->device_id = adapter_id.device_id;
        info->luid = adapter_id.adapter_luid;
    }

    return hr;
}

static const struct IWineDXGIAdapterVtbl dxgi_adapter_vtbl =
{
    dxgi_adapter_QueryInterface,
    dxgi_adapter_AddRef,
    dxgi_adapter_Release,
    dxgi_adapter_SetPrivateData,
    dxgi_adapter_SetPrivateDataInterface,
    dxgi_adapter_GetPrivateData,
    dxgi_adapter_GetParent,
    /* IDXGIAdapter methods */
    dxgi_adapter_EnumOutputs,
    dxgi_adapter_GetDesc,
    dxgi_adapter_CheckInterfaceSupport,
    /* IDXGIAdapter1 methods */
    dxgi_adapter_GetDesc1,
    /* IDXGIAdapter2 methods */
    dxgi_adapter_GetDesc2,
    /* IDXGIAdapter3 methods */
    dxgi_adapter_RegisterHardwareContentProtectionTeardownStatusEvent,
    dxgi_adapter_UnregisterHardwareContentProtectionTeardownStatus,
    dxgi_adapter_QueryVideoMemoryInfo,
    dxgi_adapter_SetVideoMemoryReservation,
    dxgi_adapter_RegisterVideoMemoryBudgetChangeNotificationEvent,
    dxgi_adapter_UnregisterVideoMemoryBudgetChangeNotification,
    /* IDXGIAdapter4 methods */
    dxgi_adapter_GetDesc3,
    /* IWineDXGIAdapter methods */
    dxgi_adapter_get_adapter_info,
};

struct dxgi_adapter *unsafe_impl_from_IDXGIAdapter(IDXGIAdapter *iface)
{
    IWineDXGIAdapter *wine_adapter;
    struct dxgi_adapter *adapter;
    HRESULT hr;

    if (!iface)
        return NULL;
    if (FAILED(hr = IDXGIAdapter_QueryInterface(iface, &IID_IWineDXGIAdapter, (void **)&wine_adapter)))
    {
        ERR("Failed to get IWineDXGIAdapter interface, hr %#x.\n", hr);
        return NULL;
    }
    assert(wine_adapter->lpVtbl == &dxgi_adapter_vtbl);
    adapter = CONTAINING_RECORD(wine_adapter, struct dxgi_adapter, IWineDXGIAdapter_iface);
    IWineDXGIAdapter_Release(wine_adapter);
    return adapter;
}

static void dxgi_adapter_init(struct dxgi_adapter *adapter, struct dxgi_factory *factory, UINT ordinal)
{
    adapter->IWineDXGIAdapter_iface.lpVtbl = &dxgi_adapter_vtbl;
    adapter->refcount = 1;
    wined3d_private_store_init(&adapter->private_store);
    adapter->ordinal = ordinal;
    adapter->factory = factory;
    IWineDXGIFactory_AddRef(&adapter->factory->IWineDXGIFactory_iface);
}

HRESULT dxgi_adapter_create(struct dxgi_factory *factory, UINT ordinal, struct dxgi_adapter **adapter)
{
    if (!(*adapter = heap_alloc(sizeof(**adapter))))
        return E_OUTOFMEMORY;

    dxgi_adapter_init(*adapter, factory, ordinal);
    return S_OK;
}
