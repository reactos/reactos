/*
 * Copyright 2012 Vincent Povirk for CodeWeavers
 * Copyright 2012 Dmitry Timoshkov
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

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "objbase.h"
#include "propvarutil.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

typedef struct MetadataHandler {
    IWICMetadataWriter IWICMetadataWriter_iface;
    LONG ref;
    IWICPersistStream IWICPersistStream_iface;
    IWICStreamProvider IWICStreamProvider_iface;
    const MetadataHandlerVtbl *vtable;
    MetadataItem *items;
    DWORD item_count;
    CRITICAL_SECTION lock;
} MetadataHandler;

static inline MetadataHandler *impl_from_IWICMetadataWriter(IWICMetadataWriter *iface)
{
    return CONTAINING_RECORD(iface, MetadataHandler, IWICMetadataWriter_iface);
}

static inline MetadataHandler *impl_from_IWICPersistStream(IWICPersistStream *iface)
{
    return CONTAINING_RECORD(iface, MetadataHandler, IWICPersistStream_iface);
}

static inline MetadataHandler *impl_from_IWICStreamProvider(IWICStreamProvider *iface)
{
    return CONTAINING_RECORD(iface, MetadataHandler, IWICStreamProvider_iface);
}

static void MetadataHandler_FreeItems(MetadataHandler *This)
{
    DWORD i;

    for (i=0; i<This->item_count; i++)
    {
        PropVariantClear(&This->items[i].schema);
        PropVariantClear(&This->items[i].id);
        PropVariantClear(&This->items[i].value);
    }

    free(This->items);
    This->items = NULL;
    This->item_count = 0;
}

static HRESULT MetadataHandlerEnum_Create(MetadataHandler *parent, DWORD index,
    IWICEnumMetadataItem **ppIEnumMetadataItem);

static HRESULT WINAPI MetadataHandler_QueryInterface(IWICMetadataWriter *iface, REFIID iid,
    void **ppv)
{
    MetadataHandler *This = impl_from_IWICMetadataWriter(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICMetadataReader, iid) ||
        (IsEqualIID(&IID_IWICMetadataWriter, iid) && This->vtable->is_writer))
    {
        *ppv = &This->IWICMetadataWriter_iface;
    }
    else if (IsEqualIID(&IID_IPersist, iid) ||
             IsEqualIID(&IID_IPersistStream, iid) ||
             IsEqualIID(&IID_IWICPersistStream, iid))
    {
        *ppv = &This->IWICPersistStream_iface;
    }
    else if (IsEqualIID(&IID_IWICStreamProvider, iid))
    {
        *ppv = &This->IWICStreamProvider_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI MetadataHandler_AddRef(IWICMetadataWriter *iface)
{
    MetadataHandler *This = impl_from_IWICMetadataWriter(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI MetadataHandler_Release(IWICMetadataWriter *iface)
{
    MetadataHandler *This = impl_from_IWICMetadataWriter(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        MetadataHandler_FreeItems(This);
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI MetadataHandler_GetMetadataHandlerInfo(IWICMetadataWriter *iface,
    IWICMetadataHandlerInfo **ppIHandler)
{
    HRESULT hr;
    IWICComponentInfo *component_info;
    MetadataHandler *This = impl_from_IWICMetadataWriter(iface);

    TRACE("%p,%p\n", iface, ppIHandler);

    hr = CreateComponentInfo(This->vtable->clsid, &component_info);
    if (FAILED(hr)) return hr;

    hr = IWICComponentInfo_QueryInterface(component_info, &IID_IWICMetadataHandlerInfo,
        (void **)ppIHandler);

    IWICComponentInfo_Release(component_info);
    return hr;
}

static HRESULT WINAPI MetadataHandler_GetMetadataFormat(IWICMetadataWriter *iface,
    GUID *pguidMetadataFormat)
{
    HRESULT hr;
    IWICMetadataHandlerInfo *metadata_info;

    TRACE("%p,%p\n", iface, pguidMetadataFormat);

    if (!pguidMetadataFormat) return E_INVALIDARG;

    hr = MetadataHandler_GetMetadataHandlerInfo(iface, &metadata_info);
    if (FAILED(hr)) return hr;

    hr = IWICMetadataHandlerInfo_GetMetadataFormat(metadata_info, pguidMetadataFormat);
    IWICMetadataHandlerInfo_Release(metadata_info);

    return hr;
}

static HRESULT WINAPI MetadataHandler_GetCount(IWICMetadataWriter *iface,
    UINT *pcCount)
{
    MetadataHandler *This = impl_from_IWICMetadataWriter(iface);

    TRACE("%p,%p\n", iface, pcCount);

    if (!pcCount) return E_INVALIDARG;

    *pcCount = This->item_count;
    return S_OK;
}

static HRESULT WINAPI MetadataHandler_GetValueByIndex(IWICMetadataWriter *iface,
    UINT index, PROPVARIANT *schema, PROPVARIANT *id, PROPVARIANT *value)
{
    HRESULT hr = S_OK;
    MetadataHandler *This = impl_from_IWICMetadataWriter(iface);

    TRACE("%p,%u,%p,%p,%p\n", iface, index, schema, id, value);

    EnterCriticalSection(&This->lock);

    if (index >= This->item_count)
    {
        LeaveCriticalSection(&This->lock);
        return E_INVALIDARG;
    }

    if (schema)
        hr = PropVariantCopy(schema, &This->items[index].schema);

    if (SUCCEEDED(hr) && id)
        hr = PropVariantCopy(id, &This->items[index].id);

    if (SUCCEEDED(hr) && value)
        hr = PropVariantCopy(value, &This->items[index].value);

    LeaveCriticalSection(&This->lock);
    return hr;
}

static HRESULT WINAPI MetadataHandler_GetValue(IWICMetadataWriter *iface,
    const PROPVARIANT *schema, const PROPVARIANT *id, PROPVARIANT *value)
{
    UINT i;
    HRESULT hr = WINCODEC_ERR_PROPERTYNOTFOUND;
    MetadataHandler *This = impl_from_IWICMetadataWriter(iface);

    TRACE("(%p,%s,%s,%p)\n", iface, wine_dbgstr_variant((const VARIANT *)schema), wine_dbgstr_variant((const VARIANT *)id), value);

    if (!id) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    for (i = 0; i < This->item_count; i++)
    {
        if (schema && This->items[i].schema.vt != VT_EMPTY)
        {
            if (PropVariantCompareEx(schema, &This->items[i].schema, 0, PVCF_USESTRCMPI) != 0) continue;
        }

        if (PropVariantCompareEx(id, &This->items[i].id, 0, PVCF_USESTRCMPI) != 0) continue;

        hr = value ? PropVariantCopy(value, &This->items[i].value) : S_OK;
        break;
    }

    LeaveCriticalSection(&This->lock);
    return hr;
}

static HRESULT WINAPI MetadataHandler_GetEnumerator(IWICMetadataWriter *iface,
    IWICEnumMetadataItem **ppIEnumMetadata)
{
    MetadataHandler *This = impl_from_IWICMetadataWriter(iface);
    TRACE("(%p,%p)\n", iface, ppIEnumMetadata);
    return MetadataHandlerEnum_Create(This, 0, ppIEnumMetadata);
}

static HRESULT WINAPI MetadataHandler_SetValue(IWICMetadataWriter *iface,
    const PROPVARIANT *pvarSchema, const PROPVARIANT *pvarId, const PROPVARIANT *pvarValue)
{
    FIXME("(%p,%p,%p,%p): stub\n", iface, pvarSchema, pvarId, pvarValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataHandler_SetValueByIndex(IWICMetadataWriter *iface,
    UINT nIndex, const PROPVARIANT *pvarSchema, const PROPVARIANT *pvarId, const PROPVARIANT *pvarValue)
{
    FIXME("(%p,%u,%p,%p,%p): stub\n", iface, nIndex, pvarSchema, pvarId, pvarValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataHandler_RemoveValue(IWICMetadataWriter *iface,
    const PROPVARIANT *pvarSchema, const PROPVARIANT *pvarId)
{
    FIXME("(%p,%p,%p): stub\n", iface, pvarSchema, pvarId);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataHandler_RemoveValueByIndex(IWICMetadataWriter *iface,
    UINT nIndex)
{
    FIXME("(%p,%u): stub\n", iface, nIndex);
    return E_NOTIMPL;
}

static const IWICMetadataWriterVtbl MetadataHandler_Vtbl = {
    MetadataHandler_QueryInterface,
    MetadataHandler_AddRef,
    MetadataHandler_Release,
    MetadataHandler_GetMetadataFormat,
    MetadataHandler_GetMetadataHandlerInfo,
    MetadataHandler_GetCount,
    MetadataHandler_GetValueByIndex,
    MetadataHandler_GetValue,
    MetadataHandler_GetEnumerator,
    MetadataHandler_SetValue,
    MetadataHandler_SetValueByIndex,
    MetadataHandler_RemoveValue,
    MetadataHandler_RemoveValueByIndex
};

static HRESULT WINAPI MetadataHandler_PersistStream_QueryInterface(IWICPersistStream *iface,
    REFIID iid, void **ppv)
{
    MetadataHandler *This = impl_from_IWICPersistStream(iface);
    return IWICMetadataWriter_QueryInterface(&This->IWICMetadataWriter_iface, iid, ppv);
}

static ULONG WINAPI MetadataHandler_PersistStream_AddRef(IWICPersistStream *iface)
{
    MetadataHandler *This = impl_from_IWICPersistStream(iface);
    return IWICMetadataWriter_AddRef(&This->IWICMetadataWriter_iface);
}

static ULONG WINAPI MetadataHandler_PersistStream_Release(IWICPersistStream *iface)
{
    MetadataHandler *This = impl_from_IWICPersistStream(iface);
    return IWICMetadataWriter_Release(&This->IWICMetadataWriter_iface);
}

static HRESULT WINAPI MetadataHandler_GetClassID(IWICPersistStream *iface,
    CLSID *pClassID)
{
    FIXME("(%p,%p): stub\n", iface, pClassID);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataHandler_IsDirty(IWICPersistStream *iface)
{
    FIXME("(%p): stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataHandler_Load(IWICPersistStream *iface,
    IStream *pStm)
{
    MetadataHandler *This = impl_from_IWICPersistStream(iface);
    TRACE("(%p,%p)\n", iface, pStm);
    return IWICPersistStream_LoadEx(&This->IWICPersistStream_iface, pStm, NULL, WICPersistOptionDefault);
}

static HRESULT WINAPI MetadataHandler_Save(IWICPersistStream *iface,
    IStream *pStm, BOOL fClearDirty)
{
    FIXME("(%p,%p,%i): stub\n", iface, pStm, fClearDirty);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataHandler_GetSizeMax(IWICPersistStream *iface,
    ULARGE_INTEGER *pcbSize)
{
    FIXME("(%p,%p): stub\n", iface, pcbSize);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataHandler_LoadEx(IWICPersistStream *iface,
    IStream *stream, const GUID *pguidPreferredVendor, DWORD dwPersistOptions)
{
    MetadataHandler *This = impl_from_IWICPersistStream(iface);
    HRESULT hr = S_OK;
    MetadataItem *new_items=NULL;
    DWORD item_count=0;

    TRACE("(%p,%p,%s,%lx)\n", iface, stream, debugstr_guid(pguidPreferredVendor), dwPersistOptions);

    EnterCriticalSection(&This->lock);

    if (stream)
    {
        hr = This->vtable->fnLoad(stream, pguidPreferredVendor, dwPersistOptions,
                &new_items, &item_count);
    }

    if (SUCCEEDED(hr))
    {
        MetadataHandler_FreeItems(This);
        This->items = new_items;
        This->item_count = item_count;
    }

    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI MetadataHandler_SaveEx(IWICPersistStream *iface,
    IStream *pIStream, DWORD dwPersistOptions, BOOL fClearDirty)
{
    FIXME("(%p,%p,%lx,%i): stub\n", iface, pIStream, dwPersistOptions, fClearDirty);
    return E_NOTIMPL;
}

static const IWICPersistStreamVtbl MetadataHandler_PersistStream_Vtbl = {
    MetadataHandler_PersistStream_QueryInterface,
    MetadataHandler_PersistStream_AddRef,
    MetadataHandler_PersistStream_Release,
    MetadataHandler_GetClassID,
    MetadataHandler_IsDirty,
    MetadataHandler_Load,
    MetadataHandler_Save,
    MetadataHandler_GetSizeMax,
    MetadataHandler_LoadEx,
    MetadataHandler_SaveEx
};

static HRESULT WINAPI metadatahandler_stream_provider_QueryInterface(IWICStreamProvider *iface, REFIID iid, void **ppv)
{
    MetadataHandler *handler = impl_from_IWICStreamProvider(iface);
    return IWICMetadataWriter_QueryInterface(&handler->IWICMetadataWriter_iface, iid, ppv);
}

static ULONG WINAPI metadatahandler_stream_provider_AddRef(IWICStreamProvider *iface)
{
    MetadataHandler *handler = impl_from_IWICStreamProvider(iface);
    return IWICMetadataWriter_AddRef(&handler->IWICMetadataWriter_iface);
}

static ULONG WINAPI metadatahandler_stream_provider_Release(IWICStreamProvider *iface)
{
    MetadataHandler *handler = impl_from_IWICStreamProvider(iface);
    return IWICMetadataWriter_Release(&handler->IWICMetadataWriter_iface);
}

static HRESULT WINAPI metadatahandler_stream_provider_GetStream(IWICStreamProvider *iface, IStream **stream)
{
    FIXME("%p, %p stub\n", iface, stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI metadatahandler_stream_provider_GetPersistOptions(IWICStreamProvider *iface, DWORD *options)
{
    FIXME("%p, %p stub\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT WINAPI metadatahandler_stream_provider_GetPreferredVendorGUID(IWICStreamProvider *iface, GUID *guid)
{
    FIXME("%p, %p stub\n", iface, guid);

    return E_NOTIMPL;
}

static HRESULT WINAPI metadatahandler_stream_provider_RefreshStream(IWICStreamProvider *iface)
{
    FIXME("%p stub\n", iface);

    return E_NOTIMPL;
}

static const IWICStreamProviderVtbl MetadataHandler_StreamProvider_Vtbl =
{
    metadatahandler_stream_provider_QueryInterface,
    metadatahandler_stream_provider_AddRef,
    metadatahandler_stream_provider_Release,
    metadatahandler_stream_provider_GetStream,
    metadatahandler_stream_provider_GetPersistOptions,
    metadatahandler_stream_provider_GetPreferredVendorGUID,
    metadatahandler_stream_provider_RefreshStream,
};

HRESULT MetadataReader_Create(const MetadataHandlerVtbl *vtable, REFIID iid, void** ppv)
{
    MetadataHandler *This;
    HRESULT hr;

    TRACE("%s\n", debugstr_guid(vtable->clsid));

    *ppv = NULL;

    This = malloc(sizeof(MetadataHandler));
    if (!This) return E_OUTOFMEMORY;

    This->IWICMetadataWriter_iface.lpVtbl = &MetadataHandler_Vtbl;
    This->IWICPersistStream_iface.lpVtbl = &MetadataHandler_PersistStream_Vtbl;
    This->IWICStreamProvider_iface.lpVtbl = &MetadataHandler_StreamProvider_Vtbl;
    This->ref = 1;
    This->vtable = vtable;
    This->items = NULL;
    This->item_count = 0;

#ifdef __REACTOS__
    InitializeCriticalSection(&This->lock);
#else
    InitializeCriticalSectionEx(&This->lock, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
#endif
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": MetadataHandler.lock");

    hr = IWICMetadataWriter_QueryInterface(&This->IWICMetadataWriter_iface, iid, ppv);

    IWICMetadataWriter_Release(&This->IWICMetadataWriter_iface);

    return hr;
}

typedef struct MetadataHandlerEnum {
    IWICEnumMetadataItem IWICEnumMetadataItem_iface;
    LONG ref;
    MetadataHandler *parent;
    DWORD index;
} MetadataHandlerEnum;

static inline MetadataHandlerEnum *impl_from_IWICEnumMetadataItem(IWICEnumMetadataItem *iface)
{
    return CONTAINING_RECORD(iface, MetadataHandlerEnum, IWICEnumMetadataItem_iface);
}

static HRESULT WINAPI MetadataHandlerEnum_QueryInterface(IWICEnumMetadataItem *iface, REFIID iid,
    void **ppv)
{
    MetadataHandlerEnum *This = impl_from_IWICEnumMetadataItem(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICEnumMetadataItem, iid))
    {
        *ppv = &This->IWICEnumMetadataItem_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI MetadataHandlerEnum_AddRef(IWICEnumMetadataItem *iface)
{
    MetadataHandlerEnum *This = impl_from_IWICEnumMetadataItem(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI MetadataHandlerEnum_Release(IWICEnumMetadataItem *iface)
{
    MetadataHandlerEnum *This = impl_from_IWICEnumMetadataItem(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        IWICMetadataWriter_Release(&This->parent->IWICMetadataWriter_iface);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI MetadataHandlerEnum_Next(IWICEnumMetadataItem *iface,
    ULONG celt, PROPVARIANT *rgeltSchema, PROPVARIANT *rgeltId,
    PROPVARIANT *rgeltValue, ULONG *pceltFetched)
{
    MetadataHandlerEnum *This = impl_from_IWICEnumMetadataItem(iface);
    ULONG new_index;
    HRESULT hr=S_FALSE;
    ULONG i;
    ULONG fetched;

    TRACE("(%p,%li)\n", iface, celt);

    if (!pceltFetched)
        pceltFetched = &fetched;

    EnterCriticalSection(&This->parent->lock);

    if (This->index >= This->parent->item_count)
    {
        *pceltFetched = 0;
        LeaveCriticalSection(&This->parent->lock);
        return S_FALSE;
    }

    new_index = min(This->parent->item_count, This->index + celt);
    *pceltFetched = new_index - This->index;

    if (rgeltSchema)
    {
        for (i=0; SUCCEEDED(hr) && i < *pceltFetched; i++)
            hr = PropVariantCopy(&rgeltSchema[i], &This->parent->items[i+This->index].schema);
    }

    for (i=0; SUCCEEDED(hr) && i < *pceltFetched; i++)
        hr = PropVariantCopy(&rgeltId[i], &This->parent->items[i+This->index].id);

    if (rgeltValue)
    {
        for (i=0; SUCCEEDED(hr) && i < *pceltFetched; i++)
            hr = PropVariantCopy(&rgeltValue[i], &This->parent->items[i+This->index].value);
    }

    if (SUCCEEDED(hr))
    {
        This->index = new_index;
    }

    LeaveCriticalSection(&This->parent->lock);

    return hr;
}

static HRESULT WINAPI MetadataHandlerEnum_Skip(IWICEnumMetadataItem *iface,
    ULONG celt)
{
    MetadataHandlerEnum *This = impl_from_IWICEnumMetadataItem(iface);

    EnterCriticalSection(&This->parent->lock);

    This->index += celt;

    LeaveCriticalSection(&This->parent->lock);

    return S_OK;
}

static HRESULT WINAPI MetadataHandlerEnum_Reset(IWICEnumMetadataItem *iface)
{
    MetadataHandlerEnum *This = impl_from_IWICEnumMetadataItem(iface);

    EnterCriticalSection(&This->parent->lock);

    This->index = 0;

    LeaveCriticalSection(&This->parent->lock);

    return S_OK;
}

static HRESULT WINAPI MetadataHandlerEnum_Clone(IWICEnumMetadataItem *iface,
    IWICEnumMetadataItem **ppIEnumMetadataItem)
{
    MetadataHandlerEnum *This = impl_from_IWICEnumMetadataItem(iface);
    HRESULT hr;

    EnterCriticalSection(&This->parent->lock);

    hr = MetadataHandlerEnum_Create(This->parent, This->index, ppIEnumMetadataItem);

    LeaveCriticalSection(&This->parent->lock);

    return hr;
}

static const IWICEnumMetadataItemVtbl MetadataHandlerEnum_Vtbl = {
    MetadataHandlerEnum_QueryInterface,
    MetadataHandlerEnum_AddRef,
    MetadataHandlerEnum_Release,
    MetadataHandlerEnum_Next,
    MetadataHandlerEnum_Skip,
    MetadataHandlerEnum_Reset,
    MetadataHandlerEnum_Clone
};

static HRESULT MetadataHandlerEnum_Create(MetadataHandler *parent, DWORD index,
    IWICEnumMetadataItem **ppIEnumMetadataItem)
{
    MetadataHandlerEnum *This;

    if (!ppIEnumMetadataItem) return E_INVALIDARG;

    *ppIEnumMetadataItem = NULL;

    This = malloc(sizeof(MetadataHandlerEnum));
    if (!This) return E_OUTOFMEMORY;

    IWICMetadataWriter_AddRef(&parent->IWICMetadataWriter_iface);

    This->IWICEnumMetadataItem_iface.lpVtbl = &MetadataHandlerEnum_Vtbl;
    This->ref = 1;
    This->parent = parent;
    This->index = index;

    *ppIEnumMetadataItem = &This->IWICEnumMetadataItem_iface;

    return S_OK;
}

static HRESULT LoadUnknownMetadata(IStream *input, const GUID *preferred_vendor,
    DWORD persist_options, MetadataItem **items, DWORD *item_count)
{
    HRESULT hr;
    MetadataItem *result;
    STATSTG stat;
    BYTE *data;
    ULONG bytesread;

    TRACE("\n");

    hr = IStream_Stat(input, &stat, STATFLAG_NONAME);
    if (FAILED(hr))
        return hr;

    data = CoTaskMemAlloc(stat.cbSize.QuadPart);
    if (!data) return E_OUTOFMEMORY;

    hr = IStream_Read(input, data, stat.cbSize.QuadPart, &bytesread);
    if (bytesread != stat.cbSize.QuadPart) hr = E_FAIL;
    if (hr != S_OK)
    {
        CoTaskMemFree(data);
        return hr;
    }

    result = calloc(1, sizeof(MetadataItem));
    if (!result)
    {
        CoTaskMemFree(data);
        return E_OUTOFMEMORY;
    }

    PropVariantInit(&result[0].schema);
    PropVariantInit(&result[0].id);
    PropVariantInit(&result[0].value);

    result[0].value.vt = VT_BLOB;
    result[0].value.blob.cbSize = bytesread;
    result[0].value.blob.pBlobData = data;

    *items = result;
    *item_count = 1;

    return S_OK;
}

static const MetadataHandlerVtbl UnknownMetadataReader_Vtbl = {
    0,
    &CLSID_WICUnknownMetadataReader,
    LoadUnknownMetadata
};

HRESULT UnknownMetadataReader_CreateInstance(REFIID iid, void** ppv)
{
    return MetadataReader_Create(&UnknownMetadataReader_Vtbl, iid, ppv);
}

#define SWAP_USHORT(x) do { if (!native_byte_order) (x) = RtlUshortByteSwap(x); } while(0)
#define SWAP_ULONG(x) do { if (!native_byte_order) (x) = RtlUlongByteSwap(x); } while(0)
#define SWAP_ULONGLONG(x) do { if (!native_byte_order) (x) = RtlUlonglongByteSwap(x); } while(0)

struct IFD_entry
{
    SHORT id;
    SHORT type;
    ULONG count;
    LONG  value;
};

#define IFD_BYTE 1
#define IFD_ASCII 2
#define IFD_SHORT 3
#define IFD_LONG 4
#define IFD_RATIONAL 5
#define IFD_SBYTE 6
#define IFD_UNDEFINED 7
#define IFD_SSHORT 8
#define IFD_SLONG 9
#define IFD_SRATIONAL 10
#define IFD_FLOAT 11
#define IFD_DOUBLE 12
#define IFD_IFD 13

static int tag_to_vt(SHORT tag)
{
    static const int tag2vt[] =
    {
        VT_EMPTY, /* 0 */
        VT_UI1,   /* IFD_BYTE 1 */
        VT_LPSTR, /* IFD_ASCII 2 */
        VT_UI2,   /* IFD_SHORT 3 */
        VT_UI4,   /* IFD_LONG 4 */
        VT_UI8,   /* IFD_RATIONAL 5 */
        VT_I1,    /* IFD_SBYTE 6 */
        VT_BLOB,  /* IFD_UNDEFINED 7 */
        VT_I2,    /* IFD_SSHORT 8 */
        VT_I4,    /* IFD_SLONG 9 */
        VT_I8,    /* IFD_SRATIONAL 10 */
        VT_R4,    /* IFD_FLOAT 11 */
        VT_R8,    /* IFD_DOUBLE 12 */
        VT_BLOB,  /* IFD_IFD 13 */
    };
    return (tag > 0 && tag <= 13) ? tag2vt[tag] : VT_BLOB;
}

static HRESULT load_IFD_entry(IStream *input, const struct IFD_entry *entry,
                              MetadataItem *item, BOOL native_byte_order)
{
    ULONG count, value, i, bytesread;
    SHORT type;
    LARGE_INTEGER pos;
    HRESULT hr;

    item->schema.vt = VT_EMPTY;
    item->id.vt = VT_UI2;
    item->id.uiVal = entry->id;
    SWAP_USHORT(item->id.uiVal);

    count = entry->count;
    SWAP_ULONG(count);
    type = entry->type;
    SWAP_USHORT(type);
    item->value.vt = tag_to_vt(type);
    value = entry->value;
    SWAP_ULONG(value);

    switch (type)
    {
     case IFD_BYTE:
     case IFD_SBYTE:
        if (!count) count = 1;

        if (count <= 4)
        {
            const BYTE *data = (const BYTE *)&entry->value;

            if (count == 1)
                item->value.bVal = data[0];
            else
            {
                item->value.vt |= VT_VECTOR;
                item->value.caub.cElems = count;
                item->value.caub.pElems = CoTaskMemAlloc(count);
                memcpy(item->value.caub.pElems, data, count);
            }
            break;
        }

        item->value.vt |= VT_VECTOR;
        item->value.caub.cElems = count;
        item->value.caub.pElems = CoTaskMemAlloc(count);
        if (!item->value.caub.pElems) return E_OUTOFMEMORY;

        pos.QuadPart = value;
        hr = IStream_Seek(input, pos, SEEK_SET, NULL);
        if (FAILED(hr))
        {
            CoTaskMemFree(item->value.caub.pElems);
            return hr;
        }
        hr = IStream_Read(input, item->value.caub.pElems, count, &bytesread);
        if (bytesread != count) hr = E_FAIL;
        if (hr != S_OK)
        {
            CoTaskMemFree(item->value.caub.pElems);
            return hr;
        }
        break;
    case IFD_SHORT:
    case IFD_SSHORT:
        if (!count) count = 1;

        if (count <= 2)
        {
            const SHORT *data = (const SHORT *)&entry->value;

            if (count == 1)
            {
                item->value.uiVal = data[0];
                SWAP_USHORT(item->value.uiVal);
            }
            else
            {
                item->value.vt |= VT_VECTOR;
                item->value.caui.cElems = count;
                item->value.caui.pElems = CoTaskMemAlloc(count * 2);
                memcpy(item->value.caui.pElems, data, count * 2);
                for (i = 0; i < count; i++)
                    SWAP_USHORT(item->value.caui.pElems[i]);
            }
            break;
        }

        item->value.vt |= VT_VECTOR;
        item->value.caui.cElems = count;
        item->value.caui.pElems = CoTaskMemAlloc(count * 2);
        if (!item->value.caui.pElems) return E_OUTOFMEMORY;

        pos.QuadPart = value;
        hr = IStream_Seek(input, pos, SEEK_SET, NULL);
        if (FAILED(hr))
        {
            CoTaskMemFree(item->value.caui.pElems);
            return hr;
        }
        hr = IStream_Read(input, item->value.caui.pElems, count * 2, &bytesread);
        if (bytesread != count * 2) hr = E_FAIL;
        if (hr != S_OK)
        {
            CoTaskMemFree(item->value.caui.pElems);
            return hr;
        }
        for (i = 0; i < count; i++)
            SWAP_USHORT(item->value.caui.pElems[i]);
        break;
    case IFD_LONG:
    case IFD_SLONG:
    case IFD_FLOAT:
        if (!count) count = 1;

        if (count == 1)
        {
            item->value.ulVal = value;
            break;
        }

        item->value.vt |= VT_VECTOR;
        item->value.caul.cElems = count;
        item->value.caul.pElems = CoTaskMemAlloc(count * 4);
        if (!item->value.caul.pElems) return E_OUTOFMEMORY;

        pos.QuadPart = value;
        hr = IStream_Seek(input, pos, SEEK_SET, NULL);
        if (FAILED(hr))
        {
            CoTaskMemFree(item->value.caul.pElems);
            return hr;
        }
        hr = IStream_Read(input, item->value.caul.pElems, count * 4, &bytesread);
        if (bytesread != count * 4) hr = E_FAIL;
        if (hr != S_OK)
        {
            CoTaskMemFree(item->value.caul.pElems);
            return hr;
        }
        for (i = 0; i < count; i++)
            SWAP_ULONG(item->value.caul.pElems[i]);
        break;
    case IFD_RATIONAL:
    case IFD_SRATIONAL:
    case IFD_DOUBLE:
        if (!count)
        {
            FIXME("IFD field type %d, count 0\n", type);
            item->value.vt = VT_EMPTY;
            break;
        }

        if (count == 1)
        {
            ULONGLONG ull;

            pos.QuadPart = value;
            hr = IStream_Seek(input, pos, SEEK_SET, NULL);
            if (FAILED(hr)) return hr;

            hr = IStream_Read(input, &ull, sizeof(ull), &bytesread);
            if (bytesread != sizeof(ull)) hr = E_FAIL;
            if (hr != S_OK) return hr;

            item->value.uhVal.QuadPart = ull;

            if (type == IFD_DOUBLE)
                SWAP_ULONGLONG(item->value.uhVal.QuadPart);
            else
            {
                SWAP_ULONG(item->value.uhVal.LowPart);
                SWAP_ULONG(item->value.uhVal.HighPart);
            }
            break;
        }
        else
        {
            item->value.vt |= VT_VECTOR;
            item->value.cauh.cElems = count;
            item->value.cauh.pElems = CoTaskMemAlloc(count * 8);
            if (!item->value.cauh.pElems) return E_OUTOFMEMORY;

            pos.QuadPart = value;
            hr = IStream_Seek(input, pos, SEEK_SET, NULL);
            if (FAILED(hr))
            {
                CoTaskMemFree(item->value.cauh.pElems);
                return hr;
            }
            hr = IStream_Read(input, item->value.cauh.pElems, count * 8, &bytesread);
            if (bytesread != count * 8) hr = E_FAIL;
            if (hr != S_OK)
            {
                CoTaskMemFree(item->value.cauh.pElems);
                return hr;
            }
            for (i = 0; i < count; i++)
            {
                if (type == IFD_DOUBLE)
                    SWAP_ULONGLONG(item->value.cauh.pElems[i].QuadPart);
                else
                {
                    SWAP_ULONG(item->value.cauh.pElems[i].LowPart);
                    SWAP_ULONG(item->value.cauh.pElems[i].HighPart);
                }
            }
        }
        break;
    case IFD_ASCII:
        item->value.pszVal = CoTaskMemAlloc(count + 1);
        if (!item->value.pszVal) return E_OUTOFMEMORY;

        if (count <= 4)
        {
            const char *data = (const char *)&entry->value;
            memcpy(item->value.pszVal, data, count);
            item->value.pszVal[count] = 0;
            break;
        }

        pos.QuadPart = value;
        hr = IStream_Seek(input, pos, SEEK_SET, NULL);
        if (FAILED(hr))
        {
            CoTaskMemFree(item->value.pszVal);
            return hr;
        }
        hr = IStream_Read(input, item->value.pszVal, count, &bytesread);
        if (bytesread != count) hr = E_FAIL;
        if (hr != S_OK)
        {
            CoTaskMemFree(item->value.pszVal);
            return hr;
        }
        item->value.pszVal[count] = 0;
        break;
    case IFD_UNDEFINED:
        if (!count)
        {
            FIXME("IFD field type %d, count 0\n", type);
            item->value.vt = VT_EMPTY;
            break;
        }

        item->value.blob.pBlobData = CoTaskMemAlloc(count);
        if (!item->value.blob.pBlobData) return E_OUTOFMEMORY;

        item->value.blob.cbSize = count;

        if (count <= 4)
        {
            const char *data = (const char *)&entry->value;
            memcpy(item->value.blob.pBlobData, data, count);
            break;
        }

        pos.QuadPart = value;
        hr = IStream_Seek(input, pos, SEEK_SET, NULL);
        if (FAILED(hr))
        {
            CoTaskMemFree(item->value.blob.pBlobData);
            return hr;
        }
        hr = IStream_Read(input, item->value.blob.pBlobData, count, &bytesread);
        if (bytesread != count) hr = E_FAIL;
        if (hr != S_OK)
        {
            CoTaskMemFree(item->value.blob.pBlobData);
            return hr;
        }
        break;
    default:
        FIXME("loading field of type %d, count %lu is not implemented\n", type, count);
        break;
    }
    return S_OK;
}

static HRESULT LoadIfdMetadata(IStream *input, const GUID *preferred_vendor,
    DWORD persist_options, MetadataItem **items, DWORD *item_count)
{
    HRESULT hr;
    MetadataItem *result;
    USHORT count, i;
    struct IFD_entry *entry;
    BOOL native_byte_order = TRUE;
    ULONG bytesread;

    TRACE("\n");

#ifdef WORDS_BIGENDIAN
    if (persist_options & WICPersistOptionLittleEndian)
#else
    if (persist_options & WICPersistOptionBigEndian)
#endif
        native_byte_order = FALSE;

    hr = IStream_Read(input, &count, sizeof(count), &bytesread);
    if (bytesread != sizeof(count)) hr = E_FAIL;
    if (hr != S_OK) return hr;

    SWAP_USHORT(count);

    entry = malloc(count * sizeof(*entry));
    if (!entry) return E_OUTOFMEMORY;

    hr = IStream_Read(input, entry, count * sizeof(*entry), &bytesread);
    if (bytesread != count * sizeof(*entry)) hr = E_FAIL;
    if (hr != S_OK)
    {
        free(entry);
        return hr;
    }

    /* limit number of IFDs to 4096 to avoid infinite loop */
    for (i = 0; i < 4096; i++)
    {
        ULONG next_ifd_offset;
        LARGE_INTEGER pos;
        USHORT next_ifd_count;

        hr = IStream_Read(input, &next_ifd_offset, sizeof(next_ifd_offset), &bytesread);
        if (bytesread != sizeof(next_ifd_offset)) hr = E_FAIL;
        if (hr != S_OK) break;

        SWAP_ULONG(next_ifd_offset);
        if (!next_ifd_offset) break;

        pos.QuadPart = next_ifd_offset;
        hr = IStream_Seek(input, pos, SEEK_SET, NULL);
        if (FAILED(hr)) break;

        hr = IStream_Read(input, &next_ifd_count, sizeof(next_ifd_count), &bytesread);
        if (bytesread != sizeof(next_ifd_count)) hr = E_FAIL;
        if (hr != S_OK) break;

        SWAP_USHORT(next_ifd_count);

        pos.QuadPart = next_ifd_count * sizeof(*entry);
        hr = IStream_Seek(input, pos, SEEK_CUR, NULL);
        if (FAILED(hr)) break;
    }

    if (hr != S_OK || i == 4096)
    {
        free(entry);
        return WINCODEC_ERR_BADMETADATAHEADER;
    }

    result = calloc(count, sizeof(*result));
    if (!result)
    {
        free(entry);
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < count; i++)
    {
        hr = load_IFD_entry(input, &entry[i], &result[i], native_byte_order);
        if (FAILED(hr))
        {
            free(entry);
            free(result);
            return hr;
        }
    }

    free(entry);

    *items = result;
    *item_count = count;

    return S_OK;
}

static const MetadataHandlerVtbl IfdMetadataReader_Vtbl = {
    0,
    &CLSID_WICIfdMetadataReader,
    LoadIfdMetadata
};

HRESULT IfdMetadataReader_CreateInstance(REFIID iid, void **ppv)
{
    return MetadataReader_Create(&IfdMetadataReader_Vtbl, iid, ppv);
}

