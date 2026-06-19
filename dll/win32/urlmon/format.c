/*
 * Copyright 2005 Jacek Caban
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

#include "urlmon_main.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

static WCHAR wszEnumFORMATETC[] = L"_EnumFORMATETC_";

typedef struct {
    IEnumFORMATETC IEnumFORMATETC_iface;

    FORMATETC *fetc;
    UINT fetc_cnt;
    UINT it;

    LONG ref;
} EnumFORMATETC;

static inline EnumFORMATETC *impl_from_IEnumFORMATETC(IEnumFORMATETC *iface)
{
    return CONTAINING_RECORD(iface, EnumFORMATETC, IEnumFORMATETC_iface);
}

static IEnumFORMATETC *EnumFORMATETC_Create(UINT cfmtetc, const FORMATETC *rgfmtetc, UINT it);

static HRESULT WINAPI EnumFORMATETC_QueryInterface(IEnumFORMATETC *iface, REFIID riid, void **ppv)
{
    EnumFORMATETC *This = impl_from_IEnumFORMATETC(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IEnumFORMATETC, riid)) {
        IEnumFORMATETC_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }

    WARN("not supported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI EnumFORMATETC_AddRef(IEnumFORMATETC *iface)
{
    EnumFORMATETC *This = impl_from_IEnumFORMATETC(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%ld\n", This, ref);
    return ref;
}

static ULONG WINAPI EnumFORMATETC_Release(IEnumFORMATETC *iface)
{
    EnumFORMATETC *This = impl_from_IEnumFORMATETC(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        free(This->fetc);
        free(This);

        URLMON_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI EnumFORMATETC_Next(IEnumFORMATETC *iface, ULONG celt,
        FORMATETC *rgelt, ULONG *pceltFetched)
{
    EnumFORMATETC *This = impl_from_IEnumFORMATETC(iface);
    ULONG cnt;

    TRACE("(%p)->(%ld %p %p)\n", This, celt, rgelt, pceltFetched);

    if(!rgelt)
        return E_INVALIDARG;

    if(This->it >= This->fetc_cnt || !celt) {
        if(pceltFetched)
            *pceltFetched = 0;
        return celt ? S_FALSE : S_OK;
    }

    cnt = This->fetc_cnt-This->it > celt ? celt : This->fetc_cnt-This->it;

    memcpy(rgelt, This->fetc+This->it, cnt*sizeof(FORMATETC));
    This->it += cnt;

    if(pceltFetched)
        *pceltFetched = cnt;

    return cnt == celt ? S_OK : S_FALSE;
}

static HRESULT WINAPI EnumFORMATETC_Skip(IEnumFORMATETC *iface, ULONG celt)
{
    EnumFORMATETC *This = impl_from_IEnumFORMATETC(iface);

    TRACE("(%p)->(%ld)\n", This, celt);

    This->it += celt;
    return This->it > This->fetc_cnt ? S_FALSE : S_OK;
}

static HRESULT WINAPI EnumFORMATETC_Reset(IEnumFORMATETC *iface)
{
    EnumFORMATETC *This = impl_from_IEnumFORMATETC(iface);

    TRACE("(%p)\n", This);

    This->it = 0;
    return S_OK;
}

static HRESULT WINAPI EnumFORMATETC_Clone(IEnumFORMATETC *iface, IEnumFORMATETC **ppenum)
{
    EnumFORMATETC *This = impl_from_IEnumFORMATETC(iface);

    TRACE("(%p)->(%p)\n", This, ppenum);

    if(!ppenum)
        return E_INVALIDARG;

    *ppenum = EnumFORMATETC_Create(This->fetc_cnt, This->fetc, This->it);
    return S_OK;
}

static const IEnumFORMATETCVtbl EnumFORMATETCVtbl = {
    EnumFORMATETC_QueryInterface,
    EnumFORMATETC_AddRef,
    EnumFORMATETC_Release,
    EnumFORMATETC_Next,
    EnumFORMATETC_Skip,
    EnumFORMATETC_Reset,
    EnumFORMATETC_Clone
};

static IEnumFORMATETC *EnumFORMATETC_Create(UINT cfmtetc, const FORMATETC *rgfmtetc, UINT it)
{
    EnumFORMATETC *ret = malloc(sizeof(EnumFORMATETC));

    URLMON_LockModule();

    ret->IEnumFORMATETC_iface.lpVtbl = &EnumFORMATETCVtbl;
    ret->ref = 1;
    ret->it = it;
    ret->fetc_cnt = cfmtetc;

    ret->fetc = malloc(cfmtetc * sizeof(FORMATETC));
    memcpy(ret->fetc, rgfmtetc, cfmtetc*sizeof(FORMATETC));

    return &ret->IEnumFORMATETC_iface;
}

/**********************************************************
 *      CreateFormatEnumerator (urlmon.@)
 */
HRESULT WINAPI CreateFormatEnumerator(UINT cfmtetc, FORMATETC *rgfmtetc,
        IEnumFORMATETC** ppenumfmtetc)
{
    TRACE("(%d %p %p)\n", cfmtetc, rgfmtetc, ppenumfmtetc);

    if(!ppenumfmtetc)
        return E_INVALIDARG;
    if(!cfmtetc)
        return E_FAIL;

    *ppenumfmtetc = EnumFORMATETC_Create(cfmtetc, rgfmtetc, 0);
    return S_OK;
}

/**********************************************************
 *      RegisterFormatEnumerator (urlmon.@)
 */
HRESULT WINAPI RegisterFormatEnumerator(LPBC pBC, IEnumFORMATETC *pEFetc, DWORD reserved)
{
    TRACE("(%p %p %ld)\n", pBC, pEFetc, reserved);

    if(reserved)
        WARN("reserved != 0\n");

    if(!pBC || !pEFetc)
        return E_INVALIDARG;

    return IBindCtx_RegisterObjectParam(pBC, wszEnumFORMATETC, (IUnknown*)pEFetc);
}

/**********************************************************
 *      RevokeFormatEnumerator (urlmon.@)
 */
HRESULT WINAPI RevokeFormatEnumerator(LPBC pbc, IEnumFORMATETC *pEFetc)
{
    TRACE("(%p %p)\n", pbc, pEFetc);

    if(!pbc)
        return E_INVALIDARG;

    return IBindCtx_RevokeObjectParam(pbc, wszEnumFORMATETC);
}
