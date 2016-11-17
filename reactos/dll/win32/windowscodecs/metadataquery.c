/*
 * Copyright 2016 Andrew Eikum for CodeWeavers
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

#include "wincodecs_private.h"

typedef struct {
    IWICMetadataQueryReader IWICMetadataQueryReader_iface;

    LONG ref;

    IWICMetadataBlockReader *block;
} QueryReader;

static inline QueryReader *impl_from_IWICMetadataQueryReader(IWICMetadataQueryReader *iface)
{
    return CONTAINING_RECORD(iface, QueryReader, IWICMetadataQueryReader_iface);
}

static HRESULT WINAPI mqr_QueryInterface(IWICMetadataQueryReader *iface, REFIID riid,
        void **ppvObject)
{
    QueryReader *This = impl_from_IWICMetadataQueryReader(iface);

    TRACE("(%p,%s,%p)\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_IWICMetadataQueryReader))
        *ppvObject = &This->IWICMetadataQueryReader_iface;
    else
        *ppvObject = NULL;

    if (*ppvObject)
    {
        IUnknown_AddRef((IUnknown*)*ppvObject);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI mqr_AddRef(IWICMetadataQueryReader *iface)
{
    QueryReader *This = impl_from_IWICMetadataQueryReader(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) refcount=%u\n", This, ref);
    return ref;
}

static ULONG WINAPI mqr_Release(IWICMetadataQueryReader *iface)
{
    QueryReader *This = impl_from_IWICMetadataQueryReader(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) refcount=%u\n", This, ref);
    if (!ref)
    {
        IWICMetadataBlockReader_Release(This->block);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static HRESULT WINAPI mqr_GetContainerFormat(IWICMetadataQueryReader *iface,
        GUID *pguidContainerFormat)
{
    QueryReader *This = impl_from_IWICMetadataQueryReader(iface);
    FIXME("(%p,%p)\n", This, pguidContainerFormat);
    return E_NOTIMPL;
}

static HRESULT WINAPI mqr_GetLocation(IWICMetadataQueryReader *iface,
        UINT cchMaxLength, WCHAR *wzNamespace, UINT *pcchActualLength)
{
    QueryReader *This = impl_from_IWICMetadataQueryReader(iface);
    FIXME("(%p,%u,%p,%p)\n", This, cchMaxLength, wzNamespace, pcchActualLength);
    return E_NOTIMPL;
}

static HRESULT WINAPI mqr_GetMetadataByName(IWICMetadataQueryReader *iface,
        LPCWSTR wzName, PROPVARIANT *pvarValue)
{
    QueryReader *This = impl_from_IWICMetadataQueryReader(iface);
    FIXME("(%p,%s,%p)\n", This, wine_dbgstr_w(wzName), pvarValue);
    return WINCODEC_ERR_PROPERTYNOTFOUND;
}

static HRESULT WINAPI mqr_GetEnumerator(IWICMetadataQueryReader *iface,
        IEnumString **ppIEnumString)
{
    QueryReader *This = impl_from_IWICMetadataQueryReader(iface);
    FIXME("(%p,%p)\n", This, ppIEnumString);
    return E_NOTIMPL;
}

static IWICMetadataQueryReaderVtbl mqr_vtbl = {
    mqr_QueryInterface,
    mqr_AddRef,
    mqr_Release,
    mqr_GetContainerFormat,
    mqr_GetLocation,
    mqr_GetMetadataByName,
    mqr_GetEnumerator
};

HRESULT MetadataQueryReader_CreateInstance(IWICMetadataBlockReader *mbr, IWICMetadataQueryReader **out)
{
    QueryReader *obj;

    obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*obj));
    if (!obj)
        return E_OUTOFMEMORY;

    obj->IWICMetadataQueryReader_iface.lpVtbl = &mqr_vtbl;
    obj->ref = 1;

    IWICMetadataBlockReader_AddRef(mbr);
    obj->block = mbr;

    *out = &obj->IWICMetadataQueryReader_iface;

    return S_OK;
}
