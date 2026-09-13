/*
 * Implementation of hyperlinking (hlink.dll)
 *
 * Copyright 2005 Aric Stewart for CodeWeavers
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

#include "hlink_private.h"

#include "shellapi.h"
#include "hlguids.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(hlink);

#define HLINK_SAVE_MAGIC    0x00000002
#define HLINK_SAVE_MONIKER_PRESENT      0x01
#define HLINK_SAVE_MONIKER_IS_ABSOLUTE  0x02
#define HLINK_SAVE_LOCATION_PRESENT     0x08
#define HLINK_SAVE_FRIENDLY_PRESENT     0x10
/* 0x20, 0x40 unknown */
#define HLINK_SAVE_TARGET_FRAME_PRESENT 0x80
/* known flags */
#define HLINK_SAVE_ALL (HLINK_SAVE_TARGET_FRAME_PRESENT|HLINK_SAVE_FRIENDLY_PRESENT|HLINK_SAVE_LOCATION_PRESENT|0x04|HLINK_SAVE_MONIKER_IS_ABSOLUTE|HLINK_SAVE_MONIKER_PRESENT)

typedef struct
{
    IHlink              IHlink_iface;
    LONG                ref;

    IPersistStream      IPersistStream_iface;
    IDataObject         IDataObject_iface;

    LPWSTR              FriendlyName;
    LPWSTR              Location;
    LPWSTR              TargetFrameName;
    IMoniker            *Moniker;
    IHlinkSite          *Site;
    DWORD               SiteData;
    BOOL                absolute;

    IBindStatusCallback IBindStatusCallback_iface;
    IBindStatusCallback *bind_callback;
    IBindCtx *async_bind_ctx;
    DWORD async_flags;
    IHlinkBrowseContext *async_browse_ctx;
} HlinkImpl;

static inline HlinkImpl *impl_from_IHlink(IHlink *iface)
{
    return CONTAINING_RECORD(iface, HlinkImpl, IHlink_iface);
}


static inline HlinkImpl* impl_from_IPersistStream( IPersistStream* iface)
{
    return CONTAINING_RECORD(iface, HlinkImpl, IPersistStream_iface);
}

static inline HlinkImpl* impl_from_IDataObject( IDataObject* iface)
{
    return CONTAINING_RECORD(iface, HlinkImpl, IDataObject_iface);
}

static HRESULT __GetMoniker(HlinkImpl* This, IMoniker** moniker,
        DWORD ref_type)
{
    HRESULT hres;

    if (ref_type == HLINKGETREF_DEFAULT)
        ref_type = HLINKGETREF_RELATIVE;

    if (This->Moniker)
    {
        DWORD mktype = MKSYS_NONE;

        hres = IMoniker_IsSystemMoniker(This->Moniker, &mktype);
        if (hres == S_OK && mktype != MKSYS_NONE)
        {
            *moniker = This->Moniker;
            IMoniker_AddRef(*moniker);
            return S_OK;
        }
    }

    if (ref_type == HLINKGETREF_ABSOLUTE && This->Site)
    {
        IMoniker *hls_moniker;

        hres = IHlinkSite_GetMoniker(This->Site, This->SiteData,
                OLEGETMONIKER_FORCEASSIGN, OLEWHICHMK_CONTAINER, &hls_moniker);
        if (FAILED(hres))
            return hres;

        if (This->Moniker)
        {
            hres = IMoniker_ComposeWith(hls_moniker, This->Moniker, FALSE,
                    moniker);
            IMoniker_Release(hls_moniker);
            return hres;
        }

        *moniker = hls_moniker;
        return S_OK;
    }

    *moniker = This->Moniker;
    if (*moniker)
        IMoniker_AddRef(*moniker);

    return S_OK;
}

static HRESULT WINAPI IHlink_fnQueryInterface(IHlink* iface, REFIID riid,
        LPVOID *ppvObj)
{
    HlinkImpl  *This = impl_from_IHlink(iface);

    TRACE ("(%p)->(%s,%p)\n", This, debugstr_guid (riid), ppvObj);

    *ppvObj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || (IsEqualIID(riid, &IID_IHlink)))
        *ppvObj = &This->IHlink_iface;
    else if (IsEqualIID(riid, &IID_IPersistStream))
        *ppvObj = &This->IPersistStream_iface;
    else if (IsEqualIID(riid, &IID_IDataObject))
        *ppvObj = &This->IDataObject_iface;

    if (*ppvObj)
    {
        IUnknown_AddRef((IUnknown*)(*ppvObj));
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI IHlink_fnAddRef (IHlink* iface)
{
    HlinkImpl  *This = impl_from_IHlink(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(count=%lu)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IHlink_fnRelease (IHlink* iface)
{
    HlinkImpl  *This = impl_from_IHlink(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(count=%lu)\n", This, refCount + 1);
    if (refCount)
        return refCount;

    TRACE("-- destroying IHlink (%p)\n", This);
    free(This->FriendlyName);
    free(This->TargetFrameName);
    free(This->Location);
    if (This->Moniker)
        IMoniker_Release(This->Moniker);
    if (This->Site)
        IHlinkSite_Release(This->Site);
    free(This);
    return 0;
}

static HRESULT WINAPI IHlink_fnSetHlinkSite( IHlink* iface,
        IHlinkSite* pihlSite, DWORD dwSiteData)
{
    HlinkImpl  *This = impl_from_IHlink(iface);

    TRACE("(%p)->(%p %li)\n", This, pihlSite, dwSiteData);

    if (This->Site)
        IHlinkSite_Release(This->Site);

    This->Site = pihlSite;
    if (This->Site)
        IHlinkSite_AddRef(This->Site);

    This->SiteData = dwSiteData;

    return S_OK;
}

static HRESULT WINAPI IHlink_fnGetHlinkSite( IHlink* iface,
        IHlinkSite** ppihlSite, DWORD *pdwSiteData)
{
    HlinkImpl  *This = impl_from_IHlink(iface);

    TRACE("(%p)->(%p %p)\n", This, ppihlSite, pdwSiteData);

    *ppihlSite = This->Site;

    if (This->Site) {
        IHlinkSite_AddRef(This->Site);
        *pdwSiteData = This->SiteData;
    }

    return S_OK;
}

static HRESULT WINAPI IHlink_fnSetMonikerReference( IHlink* iface,
        DWORD rfHLSETF, IMoniker *pmkTarget, LPCWSTR pwzLocation)
{
    HlinkImpl  *This = impl_from_IHlink(iface);

    TRACE("(%p)->(%li %p %s)\n", This, rfHLSETF, pmkTarget,
            debugstr_w(pwzLocation));

    if(rfHLSETF == 0)
        return E_INVALIDARG;
    if(!(rfHLSETF & (HLINKSETF_TARGET | HLINKSETF_LOCATION)))
        return rfHLSETF;

    if(rfHLSETF & HLINKSETF_TARGET){
        if (This->Moniker)
            IMoniker_Release(This->Moniker);

        This->Moniker = pmkTarget;
        if (This->Moniker)
        {
            IBindCtx *pbc;
            LPOLESTR display_name;
            IMoniker_AddRef(This->Moniker);
            CreateBindCtx( 0, &pbc);
            IMoniker_GetDisplayName(This->Moniker, pbc, NULL, &display_name);
            IBindCtx_Release(pbc);
            This->absolute = display_name && wcschr(display_name, ':');
            CoTaskMemFree(display_name);
        }
    }

    if(rfHLSETF & HLINKSETF_LOCATION){
        free(This->Location);
        This->Location = wcsdup( pwzLocation );
    }

    return S_OK;
}

static HRESULT WINAPI IHlink_fnSetStringReference(IHlink* iface,
        DWORD grfHLSETF, LPCWSTR pwzTarget, LPCWSTR pwzLocation)
{
    HlinkImpl  *This = impl_from_IHlink(iface);

    TRACE("(%p)->(%li %s %s)\n", This, grfHLSETF, debugstr_w(pwzTarget),
            debugstr_w(pwzLocation));

    if(grfHLSETF > (HLINKSETF_TARGET | HLINKSETF_LOCATION) &&
            grfHLSETF < -(HLINKSETF_TARGET | HLINKSETF_LOCATION))
        return grfHLSETF;

    if (grfHLSETF & HLINKSETF_TARGET)
    {
        if (This->Moniker)
        {
            IMoniker_Release(This->Moniker);
            This->Moniker = NULL;
        }
        if (pwzTarget && *pwzTarget)
        {
            IMoniker *pMon;
            IBindCtx *pbc = NULL;
            ULONG eaten;
            HRESULT r;

            r = CreateBindCtx(0, &pbc);
            if (FAILED(r))
                return E_OUTOFMEMORY;

            r = MkParseDisplayName(pbc, pwzTarget, &eaten, &pMon);
            IBindCtx_Release(pbc);

            if (FAILED(r))
            {
                LPCWSTR p = wcschr(pwzTarget, ':');
                if (p && (p - pwzTarget > 1))
                    r = CreateURLMoniker(NULL, pwzTarget, &pMon);
                else
                    r = CreateFileMoniker(pwzTarget, &pMon);
                if (FAILED(r))
                {
                    ERR("couldn't create moniker for %s, failed with error 0x%08lx\n",
                        debugstr_w(pwzTarget), r);
                    return r;
                }
            }

            IHlink_SetMonikerReference(iface, HLINKSETF_TARGET, pMon, NULL);
            IMoniker_Release(pMon);
        }
    }

    if (grfHLSETF & HLINKSETF_LOCATION)
    {
        free(This->Location);
        This->Location = NULL;
        if (pwzLocation && *pwzLocation)
            This->Location = wcsdup( pwzLocation );
    }

    return S_OK;
}

static HRESULT WINAPI IHlink_fnGetMonikerReference(IHlink* iface,
        DWORD dwWhichRef, IMoniker **ppimkTarget, LPWSTR *ppwzLocation)
{
    HlinkImpl  *This = impl_from_IHlink(iface);

    TRACE("(%p) -> (%li %p %p)\n", This, dwWhichRef, ppimkTarget,
            ppwzLocation);

    if (ppimkTarget)
    {
        HRESULT hres = __GetMoniker(This, ppimkTarget, dwWhichRef);
        if (FAILED(hres))
        {
            if (ppwzLocation)
                *ppwzLocation = NULL;
            return hres;
        }
    }

    if (ppwzLocation)
        IHlink_GetStringReference(iface, dwWhichRef, NULL, ppwzLocation);

    return S_OK;
}

static HRESULT WINAPI IHlink_fnGetStringReference (IHlink* iface,
        DWORD dwWhichRef, LPWSTR *ppwzTarget, LPWSTR *ppwzLocation)
{
    HlinkImpl  *This = impl_from_IHlink(iface);

    TRACE("(%p) -> (%li %p %p)\n", This, dwWhichRef, ppwzTarget, ppwzLocation);

    if(dwWhichRef != -1 && dwWhichRef & ~(HLINKGETREF_DEFAULT | HLINKGETREF_ABSOLUTE | HLINKGETREF_RELATIVE))
    {
        if(ppwzTarget)
            *ppwzTarget = NULL;
        if(ppwzLocation)
            *ppwzLocation = NULL;
        return E_INVALIDARG;
    }

    if (ppwzTarget)
    {
        IMoniker* mon;
        HRESULT hres = __GetMoniker(This, &mon, dwWhichRef);
        if (FAILED(hres))
        {
            if (ppwzLocation)
                *ppwzLocation = NULL;
            return hres;
        }
        if (mon)
        {
            IBindCtx *pbc;

            CreateBindCtx( 0, &pbc);
            IMoniker_GetDisplayName(mon, pbc, NULL, ppwzTarget);
            IBindCtx_Release(pbc);
            IMoniker_Release(mon);
        }
        else
            *ppwzTarget = NULL;
    }
    if (ppwzLocation)
        *ppwzLocation = hlink_co_strdupW( This->Location );

    TRACE("(Target: %s Location: %s)\n",
            (ppwzTarget)?debugstr_w(*ppwzTarget):"<NULL>",
            (ppwzLocation)?debugstr_w(*ppwzLocation):"<NULL>");

    return S_OK;
}

static HRESULT WINAPI IHlink_fnSetFriendlyName (IHlink *iface,
        LPCWSTR pwzFriendlyName)
{
    HlinkImpl  *This = impl_from_IHlink(iface);

    TRACE("(%p) -> (%s)\n", This, debugstr_w(pwzFriendlyName));

    free(This->FriendlyName);
    This->FriendlyName = wcsdup( pwzFriendlyName );

    return S_OK;
}

static HRESULT WINAPI IHlink_fnGetFriendlyName (IHlink* iface,
        DWORD grfHLFNAMEF, LPWSTR* ppwzFriendlyName)
{
    HlinkImpl  *This = impl_from_IHlink(iface);

    TRACE("(%p) -> (%li %p)\n", This, grfHLFNAMEF, ppwzFriendlyName);

    /* FIXME: Only using explicitly set and cached friendly names */

    if (This->FriendlyName)
        *ppwzFriendlyName = hlink_co_strdupW( This->FriendlyName );
    else
    {
        IMoniker *moniker;
        HRESULT hres = __GetMoniker(This, &moniker, HLINKGETREF_DEFAULT);
        if (FAILED(hres))
        {
            *ppwzFriendlyName = NULL;
            return hres;
        }
        if (moniker)
        {
            IBindCtx *bcxt;
            CreateBindCtx(0, &bcxt);

            IMoniker_GetDisplayName(moniker, bcxt, NULL, ppwzFriendlyName);
            IBindCtx_Release(bcxt);
            IMoniker_Release(moniker);
        }
        else
            *ppwzFriendlyName = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI IHlink_fnSetTargetFrameName(IHlink* iface,
        LPCWSTR pwzTargetFramename)
{
    HlinkImpl  *This = impl_from_IHlink(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(pwzTargetFramename));

    free(This->TargetFrameName);
    This->TargetFrameName = wcsdup( pwzTargetFramename );

    return S_OK;
}

static HRESULT WINAPI IHlink_fnGetTargetFrameName(IHlink* iface,
        LPWSTR *ppwzTargetFrameName)
{
    HlinkImpl  *This = impl_from_IHlink(iface);

    TRACE("(%p)->(%p)\n", This, ppwzTargetFrameName);

    if(!This->TargetFrameName) {
        *ppwzTargetFrameName = NULL;
        return S_FALSE;
    }

    *ppwzTargetFrameName = hlink_co_strdupW( This->TargetFrameName );
    if(!*ppwzTargetFrameName)
        return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI IHlink_fnGetMiscStatus(IHlink* iface, DWORD* pdwStatus)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IHlink_fnNavigate(IHlink *iface, DWORD flags, IBindCtx *user_bind_ctx,
        IBindStatusCallback *bind_callback, IHlinkBrowseContext *browse_ctx)
{
    HlinkImpl *This = impl_from_IHlink(iface);
    IMoniker *mon = NULL;
    HRESULT r;

    TRACE("hlink %p, flags %#lx, user_bind_ctx %p, bind_callback %p, browse_ctx %p.\n",
            This, flags, user_bind_ctx, bind_callback, browse_ctx);

    if (This->async_bind_ctx)
        return E_UNEXPECTED;

    r = __GetMoniker(This, &mon, HLINKGETREF_ABSOLUTE);
    TRACE("Moniker %p\n", mon);

    if (SUCCEEDED(r))
    {
        IBindCtx *bind_ctx = NULL;
        IUnknown *unk = NULL;
        IHlinkTarget *target;

        if (browse_ctx)
        {
            r = IHlinkBrowseContext_GetObject(browse_ctx, mon, TRUE, &unk);
            if (r != S_OK)
            {
                CreateBindCtx(0, &bind_ctx);
                RegisterBindStatusCallback(bind_ctx, &This->IBindStatusCallback_iface, NULL, 0);
                This->bind_callback = bind_callback;
                r = IMoniker_BindToObject(mon, bind_ctx, NULL, &IID_IUnknown, (void**)&unk);
                if (r == MK_S_ASYNCHRONOUS)
                {
                    This->async_bind_ctx = bind_ctx;
                    This->async_flags = flags;
                    if (bind_callback)
                        IBindStatusCallback_AddRef(bind_callback);
                    IHlinkBrowseContext_AddRef(This->async_browse_ctx = browse_ctx);
                    IMoniker_Release(mon);
                    return r;
                }
            }
            if (r == S_OK)
            {
                r = IUnknown_QueryInterface(unk, &IID_IHlinkTarget, (void **)&target);
                IUnknown_Release(unk);
            }
            if (r == S_OK)
            {
                if (bind_ctx) IHlinkTarget_SetBrowseContext(target, browse_ctx);
                r = IHlinkTarget_Navigate(target, flags, This->Location);
                IHlinkTarget_Release(target);
            }

            RevokeBindStatusCallback(bind_ctx, &This->IBindStatusCallback_iface);
            if (bind_ctx) IBindCtx_Release(bind_ctx);
        }
        else
        {
            LPWSTR target = NULL;

            r = IHlink_GetStringReference(iface, HLINKGETREF_DEFAULT, &target, NULL);
            if (SUCCEEDED(r) && target)
            {
                ShellExecuteW(NULL, L"open", target, NULL, NULL, SW_SHOW);
                CoTaskMemFree(target);
                r = DRAGDROP_S_DROP;
            }
        }
        IMoniker_Release(mon);
    }

    if (This->Site)
        IHlinkSite_OnNavigationComplete(This->Site, This->SiteData, 0, r, NULL);

    TRACE("Finished Navigation\n");
    return r;
}

static HRESULT WINAPI IHlink_fnSetAdditionalParams(IHlink* iface,
        LPCWSTR pwzAdditionalParams)
{
    TRACE("Not implemented in native IHlink\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IHlink_fnGetAdditionalParams(IHlink* iface,
        LPWSTR* ppwzAdditionalParams)
{
    TRACE("Not implemented in native IHlink\n");
    return E_NOTIMPL;
}

static const IHlinkVtbl hlvt =
{
    IHlink_fnQueryInterface,
    IHlink_fnAddRef,
    IHlink_fnRelease,
    IHlink_fnSetHlinkSite,
    IHlink_fnGetHlinkSite,
    IHlink_fnSetMonikerReference,
    IHlink_fnGetMonikerReference,
    IHlink_fnSetStringReference,
    IHlink_fnGetStringReference,
    IHlink_fnSetFriendlyName,
    IHlink_fnGetFriendlyName,
    IHlink_fnSetTargetFrameName,
    IHlink_fnGetTargetFrameName,
    IHlink_fnGetMiscStatus,
    IHlink_fnNavigate,
    IHlink_fnSetAdditionalParams,
    IHlink_fnGetAdditionalParams
};

static HRESULT WINAPI IDataObject_fnQueryInterface(IDataObject* iface,
        REFIID riid, LPVOID *ppvObj)
{
    HlinkImpl *This = impl_from_IDataObject(iface);
    TRACE("%p\n", This);
    return IHlink_QueryInterface(&This->IHlink_iface, riid, ppvObj);
}

static ULONG WINAPI IDataObject_fnAddRef (IDataObject* iface)
{
    HlinkImpl *This = impl_from_IDataObject(iface);
    TRACE("%p\n", This);
    return IHlink_AddRef(&This->IHlink_iface);
}

static ULONG WINAPI IDataObject_fnRelease (IDataObject* iface)
{
    HlinkImpl *This = impl_from_IDataObject(iface);
    TRACE("%p\n", This);
    return IHlink_Release(&This->IHlink_iface);
}

static HRESULT WINAPI IDataObject_fnGetData(IDataObject* iface,
        FORMATETC* pformatetcIn, STGMEDIUM* pmedium)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IDataObject_fnGetDataHere(IDataObject* iface,
        FORMATETC* pformatetc, STGMEDIUM* pmedium)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IDataObject_fnQueryGetData(IDataObject* iface,
        FORMATETC* pformatetc)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IDataObject_fnGetConicalFormatEtc(IDataObject* iface,
        FORMATETC* pformatetcIn, FORMATETC* pformatetcOut)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IDataObject_fnSetData(IDataObject* iface,
        FORMATETC* pformatetc, STGMEDIUM* pmedium, BOOL fRelease)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IDataObject_fnEnumFormatEtc(IDataObject* iface,
        DWORD dwDirection, IEnumFORMATETC** ppenumFormatEtc)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IDataObject_fnDAdvise(IDataObject* iface,
        FORMATETC* pformatetc, DWORD advf, IAdviseSink* pAdvSink,
        DWORD* pdwConnection)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IDataObject_fnDUnadvise(IDataObject* iface,
        DWORD dwConnection)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IDataObject_fnEnumDAdvise(IDataObject* iface,
        IEnumSTATDATA** ppenumAdvise)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const IDataObjectVtbl dovt =
{
    IDataObject_fnQueryInterface,
    IDataObject_fnAddRef,
    IDataObject_fnRelease,
    IDataObject_fnGetData,
    IDataObject_fnGetDataHere,
    IDataObject_fnQueryGetData,
    IDataObject_fnGetConicalFormatEtc,
    IDataObject_fnSetData,
    IDataObject_fnEnumFormatEtc,
    IDataObject_fnDAdvise,
    IDataObject_fnDUnadvise,
    IDataObject_fnEnumDAdvise
};

static HRESULT WINAPI IPersistStream_fnQueryInterface(IPersistStream* iface,
        REFIID riid, LPVOID *ppvObj)
{
    HlinkImpl *This = impl_from_IPersistStream(iface);
    TRACE("(%p)\n", This);
    return IHlink_QueryInterface(&This->IHlink_iface, riid, ppvObj);
}

static ULONG WINAPI IPersistStream_fnAddRef (IPersistStream* iface)
{
    HlinkImpl *This = impl_from_IPersistStream(iface);
    TRACE("(%p)\n", This);
    return IHlink_AddRef(&This->IHlink_iface);
}

static ULONG WINAPI IPersistStream_fnRelease (IPersistStream* iface)
{
    HlinkImpl *This = impl_from_IPersistStream(iface);
    TRACE("(%p)\n", This);
    return IHlink_Release(&This->IHlink_iface);
}

static HRESULT WINAPI IPersistStream_fnGetClassID(IPersistStream* iface,
        CLSID* pClassID)
{
    HlinkImpl *This = impl_from_IPersistStream(iface);
    TRACE("(%p)\n", This);
    *pClassID = CLSID_StdHlink;
    return S_OK;
}

static HRESULT WINAPI IPersistStream_fnIsDirty(IPersistStream* iface)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT write_hlink_string(IStream *pStm, LPCWSTR str)
{
    DWORD len;
    HRESULT hr;

    TRACE("(%p, %s)\n", pStm, debugstr_w(str));

    len = lstrlenW(str) + 1;

    hr = IStream_Write(pStm, &len, sizeof(len), NULL);
    if (FAILED(hr)) return hr;

    hr = IStream_Write(pStm, str, len * sizeof(WCHAR), NULL);
    if (FAILED(hr)) return hr;

    return S_OK;
}

static inline ULONG size_hlink_string(LPCWSTR str)
{
    return sizeof(DWORD) + (lstrlenW(str) + 1) * sizeof(WCHAR);
}

static HRESULT read_hlink_string(IStream *pStm, LPWSTR *out_str)
{
    LPWSTR str;
    DWORD len;
    ULONG read;
    HRESULT hr;

    hr = IStream_Read(pStm, &len, sizeof(len), &read);
    if (FAILED(hr)) return hr;
    if (read != sizeof(len)) return STG_E_READFAULT;

    TRACE("read len %ld\n", len);

    str = malloc(len * sizeof(WCHAR));
    if (!str) return E_OUTOFMEMORY;

    hr = IStream_Read(pStm, str, len * sizeof(WCHAR), &read);
    if (FAILED(hr))
    {
        free(str);
        return hr;
    }
    if (read != len * sizeof(WCHAR))
    {
        free(str);
        return STG_E_READFAULT;
    }
    TRACE("read string %s\n", debugstr_w(str));

    *out_str = str;
    return S_OK;
}

static HRESULT WINAPI IPersistStream_fnLoad(IPersistStream* iface,
        IStream* pStm)
{
    HRESULT r;
    DWORD hdr[2];
    DWORD read;
    HlinkImpl *This = impl_from_IPersistStream(iface);

    r = IStream_Read(pStm, hdr, sizeof(hdr), &read);
    if (read != sizeof(hdr) || (hdr[0] != HLINK_SAVE_MAGIC))
    {
        r = E_FAIL;
        goto end;
    }
    if (hdr[1] & ~HLINK_SAVE_ALL)
        FIXME("unknown flag(s) 0x%lx\n", hdr[1] & ~HLINK_SAVE_ALL);

    if (hdr[1] & HLINK_SAVE_TARGET_FRAME_PRESENT)
    {
        TRACE("loading target frame name\n");
        r = read_hlink_string(pStm, &This->TargetFrameName);
        if (FAILED(r)) goto end;
    }

    if (hdr[1] & HLINK_SAVE_FRIENDLY_PRESENT)
    {
        TRACE("loading target friendly name\n");
        if (!(hdr[1] & 0x4))
            FIXME("0x4 flag not present with friendly name flag - not sure what this means\n");
        r = read_hlink_string(pStm, &This->FriendlyName);
        if (FAILED(r)) goto end;
    }

    if (hdr[1] & HLINK_SAVE_MONIKER_PRESENT)
    {
        TRACE("loading moniker\n");
        r = OleLoadFromStream(pStm, &IID_IMoniker, (LPVOID*)&(This->Moniker));
        if (FAILED(r))
            goto end;
        This->absolute = (hdr[1] & HLINK_SAVE_MONIKER_IS_ABSOLUTE) != 0;
    }

    if (hdr[1] & HLINK_SAVE_LOCATION_PRESENT)
    {
        TRACE("loading location\n");
        r = read_hlink_string(pStm, &This->Location);
        if (FAILED(r)) goto end;
    }

end:
    TRACE("Load Result 0x%lx (%p)\n", r, This->Moniker);

    return r;
}

static HRESULT WINAPI IPersistStream_fnSave(IPersistStream* iface,
        IStream* pStm, BOOL fClearDirty)
{
    HRESULT r;
    HlinkImpl *This = impl_from_IPersistStream(iface);
    DWORD hdr[2];
    IMoniker *moniker;

    TRACE("(%p) Moniker(%p)\n", This, This->Moniker);

    r = __GetMoniker(This, &moniker, HLINKGETREF_DEFAULT);
    if (FAILED(r))
        return r;
    r = E_FAIL;

    hdr[0] = HLINK_SAVE_MAGIC;
    hdr[1] = 0;

    if (moniker)
        hdr[1] |= HLINK_SAVE_MONIKER_PRESENT;
    if (This->absolute)
        hdr[1] |= HLINK_SAVE_MONIKER_IS_ABSOLUTE;
    if (This->Location)
        hdr[1] |= HLINK_SAVE_LOCATION_PRESENT;
    if (This->FriendlyName)
        hdr[1] |= HLINK_SAVE_FRIENDLY_PRESENT | 4 /* FIXME */;
    if (This->TargetFrameName)
        hdr[1] |= HLINK_SAVE_TARGET_FRAME_PRESENT;

    IStream_Write(pStm, hdr, sizeof(hdr), NULL);

    if (This->TargetFrameName)
    {
        r = write_hlink_string(pStm, This->TargetFrameName);
        if (FAILED(r)) goto end;
    }

    if (This->FriendlyName)
    {
        r = write_hlink_string(pStm, This->FriendlyName);
        if (FAILED(r)) goto end;
    }

    if (moniker)
    {
        IPersistStream* monstream;

        monstream = NULL;
        IMoniker_QueryInterface(moniker, &IID_IPersistStream,
                (LPVOID*)&monstream);
        if (monstream)
        {
            r = OleSaveToStream(monstream, pStm);
            IPersistStream_Release(monstream);
        }
        if (FAILED(r)) goto end;
    }

    if (This->Location)
    {
        r = write_hlink_string(pStm, This->Location);
        if (FAILED(r)) goto end;
    }

end:
    if (moniker) IMoniker_Release(moniker);
    TRACE("Save Result 0x%lx\n", r);

    return r;
}

static HRESULT WINAPI IPersistStream_fnGetSizeMax(IPersistStream* iface,
        ULARGE_INTEGER* pcbSize)
{
    HRESULT r;
    HlinkImpl *This = impl_from_IPersistStream(iface);
    IMoniker *moniker;

    TRACE("(%p) Moniker(%p)\n", This, This->Moniker);

    pcbSize->QuadPart = sizeof(DWORD)*2;

    if (This->TargetFrameName)
        pcbSize->QuadPart += size_hlink_string(This->TargetFrameName);

    if (This->FriendlyName)
        pcbSize->QuadPart += size_hlink_string(This->FriendlyName);

    r = __GetMoniker(This, &moniker, HLINKGETREF_DEFAULT);
    if (FAILED(r))
        return r;
    r = E_FAIL;

    if (moniker)
    {
        IPersistStream* monstream = NULL;
        IMoniker_QueryInterface(moniker, &IID_IPersistStream,
                (LPVOID*)&monstream);
        if (monstream)
        {
            ULARGE_INTEGER mon_size;
            r = IPersistStream_GetSizeMax(monstream, &mon_size);
            pcbSize->QuadPart += mon_size.QuadPart;
            IPersistStream_Release(monstream);
        }
        IMoniker_Release(moniker);
    }

    if (This->Location)
        pcbSize->QuadPart += size_hlink_string(This->Location);

    return r;
}

static const IPersistStreamVtbl psvt =
{
    IPersistStream_fnQueryInterface,
    IPersistStream_fnAddRef,
    IPersistStream_fnRelease,
    IPersistStream_fnGetClassID,
    IPersistStream_fnIsDirty,
    IPersistStream_fnLoad,
    IPersistStream_fnSave,
    IPersistStream_fnGetSizeMax,
};

static HlinkImpl *impl_from_IBindStatusCallback(IBindStatusCallback *iface)
{
    return CONTAINING_RECORD(iface, HlinkImpl, IBindStatusCallback_iface);
}

static HRESULT WINAPI bind_callback_QueryInterface(IBindStatusCallback *iface, REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IBindStatusCallback))
    {
        IBindStatusCallback_AddRef(*out = iface);
        return S_OK;
    }

    WARN("No interface for %s.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI bind_callback_AddRef(IBindStatusCallback *iface)
{
    HlinkImpl *hlink = impl_from_IBindStatusCallback(iface);
    return IHlink_AddRef(&hlink->IHlink_iface);
}

static ULONG WINAPI bind_callback_Release(IBindStatusCallback *iface)
{
    HlinkImpl *hlink = impl_from_IBindStatusCallback(iface);
    return IHlink_Release(&hlink->IHlink_iface);
}

static HRESULT WINAPI bind_callback_OnStartBinding(IBindStatusCallback *iface,
        DWORD reserved, IBinding *binding)
{
    HlinkImpl *hlink = impl_from_IBindStatusCallback(iface);

    TRACE("hlink %p, reserved %#lx, binding %p.\n", hlink, reserved, binding);

    if (hlink->bind_callback)
        return IBindStatusCallback_OnStartBinding(hlink->bind_callback, reserved, binding);
    return S_OK;
}

static HRESULT WINAPI bind_callback_GetPriority(IBindStatusCallback *iface, LONG *priority)
{
    FIXME("iface %p, priority %p, stub!\n", iface, priority);
    return E_NOTIMPL;
}

static HRESULT WINAPI bind_callback_OnLowResource(IBindStatusCallback *iface, DWORD reserved)
{
    HlinkImpl *hlink = impl_from_IBindStatusCallback(iface);

    TRACE("hlink %p, reserved %#lx.\n", hlink, reserved);

    if (hlink->bind_callback)
        return IBindStatusCallback_OnLowResource(hlink->bind_callback, reserved);
    return S_OK;
}

static HRESULT WINAPI bind_callback_OnProgress(IBindStatusCallback *iface,
        ULONG progress, ULONG max, ULONG status, const WCHAR *text)
{
    HlinkImpl *hlink = impl_from_IBindStatusCallback(iface);

    TRACE("hlink %p, progress %lu, max %lu, status %lu, text %s.\n",
            hlink, progress, max, status, debugstr_w(text));

    if (hlink->bind_callback)
        return IBindStatusCallback_OnProgress(hlink->bind_callback, progress, max, status, text);
    return S_OK;
}

static HRESULT WINAPI bind_callback_OnStopBinding(IBindStatusCallback *iface,
        HRESULT hr, const WCHAR *error)
{
    HlinkImpl *hlink = impl_from_IBindStatusCallback(iface);

    TRACE("hlink %p, hr %#lx, error %s.\n", hlink, hr, debugstr_w(error));

    if (hlink->bind_callback)
        IBindStatusCallback_OnStopBinding(hlink->bind_callback, hr, error);

    if (hlink->async_bind_ctx)
    {
        if (hlink->bind_callback)
            IBindStatusCallback_Release(hlink->bind_callback);
        RevokeBindStatusCallback(hlink->async_bind_ctx, iface);
        IBindCtx_Release(hlink->async_bind_ctx);
        IHlinkBrowseContext_Release(hlink->async_browse_ctx);
        hlink->async_bind_ctx = NULL;
    }
    return S_OK;
}

static HRESULT WINAPI bind_callback_GetBindInfo(IBindStatusCallback *iface,
        DWORD *bind_flags, BINDINFO *bind_info)
{
    HlinkImpl *hlink = impl_from_IBindStatusCallback(iface);

    TRACE("hlink %p, bind_flags %p, bind_info %p.\n", hlink, bind_flags, bind_info);

    if (hlink->bind_callback)
        return IBindStatusCallback_GetBindInfo(hlink->bind_callback, bind_flags, bind_info);
    return S_OK;
}

static HRESULT WINAPI bind_callback_OnDataAvailable(IBindStatusCallback *iface,
        DWORD flags, DWORD size, FORMATETC *formatetc, STGMEDIUM *stgmed)
{
    FIXME("iface %p, flags %#lx, size %ld, formatetc %p, stgmed %p, stub!\n",
            iface, flags, size, formatetc, stgmed);
    return E_NOTIMPL;
}

static HRESULT WINAPI bind_callback_OnObjectAvailable(IBindStatusCallback *iface,
        REFIID iid, IUnknown *unk)
{
    HlinkImpl *hlink = impl_from_IBindStatusCallback(iface);
    IHlinkTarget *target;
    HRESULT hr;

    TRACE("hlink %p, iid %s, unk %p.\n", hlink, debugstr_guid(iid), unk);

    if (hlink->bind_callback)
        IBindStatusCallback_OnObjectAvailable(hlink->bind_callback, iid, unk);

    if (hlink->async_bind_ctx)
    {
        hr = IUnknown_QueryInterface(unk, &IID_IHlinkTarget, (void **)&target);
        if (FAILED(hr))
            return hr;

        IHlinkTarget_SetBrowseContext(target, hlink->async_browse_ctx);
        hr = IHlinkTarget_Navigate(target, hlink->async_flags, hlink->Location);
        IHlinkTarget_Release(target);

        if (hlink->Site)
            IHlinkSite_OnNavigationComplete(hlink->Site, hlink->SiteData, 0, hr, NULL);

        return hr;
    }

    return S_OK;
}

static const IBindStatusCallbackVtbl bind_callback_vtbl =
{
    bind_callback_QueryInterface,
    bind_callback_AddRef,
    bind_callback_Release,
    bind_callback_OnStartBinding,
    bind_callback_GetPriority,
    bind_callback_OnLowResource,
    bind_callback_OnProgress,
    bind_callback_OnStopBinding,
    bind_callback_GetBindInfo,
    bind_callback_OnDataAvailable,
    bind_callback_OnObjectAvailable,
};

HRESULT HLink_Constructor(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    HlinkImpl * hl;
    HRESULT hr;

    TRACE("unkOut=%p riid=%s\n", pUnkOuter, debugstr_guid(riid));
    *ppv = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    if (!(hl = calloc(1, sizeof(*hl))))
        return E_OUTOFMEMORY;

    hl->ref = 1;
    hl->IHlink_iface.lpVtbl = &hlvt;
    hl->IPersistStream_iface.lpVtbl = &psvt;
    hl->IDataObject_iface.lpVtbl = &dovt;
    hl->IBindStatusCallback_iface.lpVtbl = &bind_callback_vtbl;

    hr = IHlink_QueryInterface(&hl->IHlink_iface, riid, ppv);
    IHlink_Release(&hl->IHlink_iface);

    return hr;
}
