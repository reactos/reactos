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

static const IHlinkVtbl              hlvt;
static const IPersistStreamVtbl      psvt;
static const IDataObjectVtbl         dovt;

typedef struct
{
    const IHlinkVtbl    *lpVtbl;
    LONG                ref;

    const IPersistStreamVtbl    *lpPSVtbl;
    const IDataObjectVtbl       *lpDOVtbl;

    LPWSTR              FriendlyName;
    LPWSTR              Location;
    LPWSTR              Target;
    LPWSTR              TargetFrameName;
    IMoniker            *Moniker;
    IHlinkSite          *Site;
    DWORD               SiteData;
    BOOL                absolute;
} HlinkImpl;


static inline HlinkImpl* HlinkImpl_from_IPersistStream( IPersistStream* iface)
{
    return (HlinkImpl*) ((CHAR*)iface - FIELD_OFFSET(HlinkImpl, lpPSVtbl));
}

static inline HlinkImpl* HlinkImpl_from_IDataObject( IDataObject* iface)
{
    return (HlinkImpl*) ((CHAR*)iface - FIELD_OFFSET(HlinkImpl, lpDOVtbl));
}

static inline void __GetMoniker(HlinkImpl* This, IMoniker** moniker)
{
    *moniker = NULL;
    if (This->Moniker)
    {
        *moniker = This->Moniker;
        if (*moniker)
            IMoniker_AddRef(*moniker);
    }
    else if (This->Site)
    {
        IHlinkSite_GetMoniker(This->Site, This->SiteData,
                OLEGETMONIKER_FORCEASSIGN, OLEWHICHMK_CONTAINER,
                (LPVOID)moniker);
    }
}

HRESULT WINAPI HLink_Constructor(IUnknown *pUnkOuter, REFIID riid,
        LPVOID *ppv)
{
    HlinkImpl * hl;

    TRACE("unkOut=%p riid=%s\n", pUnkOuter, debugstr_guid(riid));
    *ppv = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    hl = heap_alloc_zero(sizeof(HlinkImpl));
    if (!hl)
        return E_OUTOFMEMORY;

    hl->ref = 1;
    hl->lpVtbl = &hlvt;
    hl->lpPSVtbl = &psvt;
    hl->lpDOVtbl = &dovt;

    *ppv = hl;
    return S_OK;
}

static HRESULT WINAPI IHlink_fnQueryInterface(IHlink* iface, REFIID riid,
        LPVOID *ppvObj)
{
    HlinkImpl  *This = (HlinkImpl*)iface;

    TRACE ("(%p)->(%s,%p)\n", This, debugstr_guid (riid), ppvObj);

    *ppvObj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || (IsEqualIID(riid, &IID_IHlink)))
        *ppvObj = This;
    else if (IsEqualIID(riid, &IID_IPersistStream))
        *ppvObj = (LPVOID*)&(This->lpPSVtbl);
    else if (IsEqualIID(riid, &IID_IDataObject))
        *ppvObj = (LPVOID*)&(This->lpDOVtbl);

    if (*ppvObj)
    {
        IUnknown_AddRef((IUnknown*)(*ppvObj));
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI IHlink_fnAddRef (IHlink* iface)
{
    HlinkImpl  *This = (HlinkImpl*)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(count=%u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IHlink_fnRelease (IHlink* iface)
{
    HlinkImpl  *This = (HlinkImpl*)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(count=%u)\n", This, refCount + 1);
    if (refCount)
        return refCount;

    TRACE("-- destroying IHlink (%p)\n", This);
    heap_free(This->FriendlyName);
    heap_free(This->Target);
    heap_free(This->TargetFrameName);
    heap_free(This->Location);
    if (This->Moniker)
        IMoniker_Release(This->Moniker);
    if (This->Site)
        IHlinkSite_Release(This->Site);
    heap_free(This);
    return 0;
}

static HRESULT WINAPI IHlink_fnSetHlinkSite( IHlink* iface,
        IHlinkSite* pihlSite, DWORD dwSiteData)
{
    HlinkImpl  *This = (HlinkImpl*)iface;

    TRACE("(%p)->(%p %i)\n", This, pihlSite, dwSiteData);

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
    HlinkImpl  *This = (HlinkImpl*)iface;

    TRACE("(%p)->(%p %p)\n", This, ppihlSite, pdwSiteData);

    *ppihlSite = This->Site;
    *pdwSiteData = This->SiteData;

    if (This->Site)
        IHlinkSite_AddRef(This->Site);

    return S_OK;
}

static HRESULT WINAPI IHlink_fnSetMonikerReference( IHlink* iface,
        DWORD rfHLSETF, IMoniker *pmkTarget, LPCWSTR pwzLocation)
{
    HlinkImpl  *This = (HlinkImpl*)iface;

    FIXME("(%p)->(%i %p %s)\n", This, rfHLSETF, pmkTarget,
            debugstr_w(pwzLocation));

    if (This->Moniker)
        IMoniker_Release(This->Moniker);

    This->Moniker = pmkTarget;
    if (This->Moniker)
    {
        LPOLESTR display_name;
        IMoniker_AddRef(This->Moniker);
        IMoniker_GetDisplayName(This->Moniker, NULL, NULL, &display_name);
        This->absolute = display_name && strchrW(display_name, ':');
        CoTaskMemFree(display_name);
    }

    heap_free(This->Location);
    This->Location = hlink_strdupW( pwzLocation );

    return S_OK;
}

static HRESULT WINAPI IHlink_fnSetStringReference(IHlink* iface,
        DWORD grfHLSETF, LPCWSTR pwzTarget, LPCWSTR pwzLocation)
{
    HlinkImpl  *This = (HlinkImpl*)iface;

    TRACE("(%p)->(%i %s %s)\n", This, grfHLSETF, debugstr_w(pwzTarget),
            debugstr_w(pwzLocation));

    if (grfHLSETF & HLINKSETF_TARGET)
    {
        heap_free(This->Target);
        This->Target = hlink_strdupW( pwzTarget );
    }
    if (grfHLSETF & HLINKSETF_LOCATION)
    {
        heap_free(This->Location);
        This->Location = hlink_strdupW( pwzLocation );
    }

    return S_OK;
}

static HRESULT WINAPI IHlink_fnGetMonikerReference(IHlink* iface,
        DWORD dwWhichRef, IMoniker **ppimkTarget, LPWSTR *ppwzLocation)
{
    HlinkImpl  *This = (HlinkImpl*)iface;

    TRACE("(%p) -> (%i %p %p)\n", This, dwWhichRef, ppimkTarget,
            ppwzLocation);

    if(ppimkTarget)
        __GetMoniker(This, ppimkTarget);

    if (ppwzLocation)
        IHlink_GetStringReference(iface, dwWhichRef, NULL, ppwzLocation);

    return S_OK;
}

static HRESULT WINAPI IHlink_fnGetStringReference (IHlink* iface,
        DWORD dwWhichRef, LPWSTR *ppwzTarget, LPWSTR *ppwzLocation)
{
    HlinkImpl  *This = (HlinkImpl*)iface;

    FIXME("(%p) -> (%i %p %p)\n", This, dwWhichRef, ppwzTarget, ppwzLocation);

    if (ppwzTarget)
    {
        *ppwzTarget = hlink_co_strdupW( This->Target );

        if (!This->Target)
        {
            IMoniker* mon;
            __GetMoniker(This, &mon);
            if (mon)
            {
                IBindCtx *pbc;

                CreateBindCtx( 0, &pbc);
                IMoniker_GetDisplayName(mon, pbc, NULL, ppwzTarget);
                IBindCtx_Release(pbc);
                IMoniker_Release(mon);
            }
            else
                FIXME("Unhandled case, no set Target and no moniker\n");
        }
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
    HlinkImpl  *This = (HlinkImpl*)iface;

    TRACE("(%p) -> (%s)\n", This, debugstr_w(pwzFriendlyName));

    heap_free(This->FriendlyName);
    This->FriendlyName = hlink_strdupW( pwzFriendlyName );

    return S_OK;
}

static HRESULT WINAPI IHlink_fnGetFriendlyName (IHlink* iface,
        DWORD grfHLFNAMEF, LPWSTR* ppwzFriendlyName)
{
    HlinkImpl  *This = (HlinkImpl*)iface;

    TRACE("(%p) -> (%i %p)\n", This, grfHLFNAMEF, ppwzFriendlyName);

    /* FIXME: Only using explicitly set and cached friendly names */

    if (This->FriendlyName)
        *ppwzFriendlyName = hlink_co_strdupW( This->FriendlyName );
    else
    {
        IMoniker *moniker;
        __GetMoniker(This, &moniker);
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
    HlinkImpl  *This = (HlinkImpl*)iface;
    TRACE("(%p)->(%s)\n", This, debugstr_w(pwzTargetFramename));

    heap_free(This->TargetFrameName);
    This->TargetFrameName = hlink_strdupW( pwzTargetFramename );

    return S_OK;
}

static HRESULT WINAPI IHlink_fnGetTargetFrameName(IHlink* iface,
        LPWSTR *ppwzTargetFrameName)
{
    HlinkImpl  *This = (HlinkImpl*)iface;

    TRACE("(%p)->(%p)\n", This, ppwzTargetFrameName);
    *ppwzTargetFrameName = hlink_co_strdupW( This->TargetFrameName );

    return S_OK;
}

static HRESULT WINAPI IHlink_fnGetMiscStatus(IHlink* iface, DWORD* pdwStatus)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IHlink_fnNavigate(IHlink* iface, DWORD grfHLNF, LPBC pbc,
        IBindStatusCallback *pbsc, IHlinkBrowseContext *phbc)
{
    HlinkImpl  *This = (HlinkImpl*)iface;
    IMoniker *mon = NULL;

    FIXME("Semi-Stub:(%p)->(%i %p %p %p)\n", This, grfHLNF, pbc, pbsc, phbc);

    if (This->Site)
        IHlinkSite_ReadyToNavigate(This->Site, This->SiteData, 0);

    __GetMoniker(This, &mon);
    TRACE("Moniker %p\n", mon);

    if (mon)
    {
        IBindCtx *bcxt;
        IHlinkTarget *target = NULL;
        HRESULT r = S_OK;

        CreateBindCtx(0, &bcxt);

        RegisterBindStatusCallback(bcxt, pbsc, NULL, 0);

        r = IMoniker_BindToObject(mon, bcxt, NULL, &IID_IHlinkTarget,
                (LPVOID*)&target);
        TRACE("IHlinkTarget returned 0x%x\n", r);
        if (r == S_OK)
        {
            IHlinkTarget_SetBrowseContext(target, phbc);
            IHlinkTarget_Navigate(target, grfHLNF, This->Location);
            IHlinkTarget_Release(target);
        }
        else
        {
            static const WCHAR szOpen[] = {'o','p','e','n',0};
            LPWSTR target = NULL;

            r = IHlink_GetStringReference(iface, HLINKGETREF_DEFAULT, &target, NULL);
            if (SUCCEEDED(r) && target)
            {
                ShellExecuteW(NULL, szOpen, target, NULL, NULL, SW_SHOW);
                CoTaskMemFree(target);
            }
        }

        RevokeBindStatusCallback(bcxt, pbsc);

        IBindCtx_Release(bcxt);
        IMoniker_Release(mon);
    }

    if (This->Site)
        IHlinkSite_OnNavigationComplete(This->Site, This->SiteData, 0, 0, NULL);

    TRACE("Finished Navigation\n");
    return S_OK;
}

static HRESULT WINAPI IHlink_fnSetAdditonalParams(IHlink* iface,
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
    IHlink_fnSetAdditonalParams,
    IHlink_fnGetAdditionalParams
};

static HRESULT WINAPI IDataObject_fnQueryInterface(IDataObject* iface,
        REFIID riid, LPVOID *ppvObj)
{
    HlinkImpl *This = HlinkImpl_from_IDataObject(iface);
    TRACE("%p\n", This);
    return IHlink_QueryInterface((IHlink*)This, riid, ppvObj);
}

static ULONG WINAPI IDataObject_fnAddRef (IDataObject* iface)
{
    HlinkImpl *This = HlinkImpl_from_IDataObject(iface);
    TRACE("%p\n", This);
    return IHlink_AddRef((IHlink*)This);
}

static ULONG WINAPI IDataObject_fnRelease (IDataObject* iface)
{
    HlinkImpl *This = HlinkImpl_from_IDataObject(iface);
    TRACE("%p\n", This);
    return IHlink_Release((IHlink*)This);
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
    HlinkImpl *This = HlinkImpl_from_IPersistStream(iface);
    TRACE("(%p)\n", This);
    return IHlink_QueryInterface((IHlink*)This, riid, ppvObj);
}

static ULONG WINAPI IPersistStream_fnAddRef (IPersistStream* iface)
{
    HlinkImpl *This = HlinkImpl_from_IPersistStream(iface);
    TRACE("(%p)\n", This);
    return IHlink_AddRef((IHlink*)This);
}

static ULONG WINAPI IPersistStream_fnRelease (IPersistStream* iface)
{
    HlinkImpl *This = HlinkImpl_from_IPersistStream(iface);
    TRACE("(%p)\n", This);
    return IHlink_Release((IHlink*)This);
}

static HRESULT WINAPI IPersistStream_fnGetClassID(IPersistStream* iface,
        CLSID* pClassID)
{
    HlinkImpl *This = HlinkImpl_from_IPersistStream(iface);
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

    len = strlenW(str) + 1;

    hr = IStream_Write(pStm, &len, sizeof(len), NULL);
    if (FAILED(hr)) return hr;

    hr = IStream_Write(pStm, str, len * sizeof(WCHAR), NULL);
    if (FAILED(hr)) return hr;

    return S_OK;
}

static inline ULONG size_hlink_string(LPCWSTR str)
{
    return sizeof(DWORD) + (strlenW(str) + 1) * sizeof(WCHAR);
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

    TRACE("read len %d\n", len);

    str = heap_alloc(len * sizeof(WCHAR));
    if (!str) return E_OUTOFMEMORY;

    hr = IStream_Read(pStm, str, len * sizeof(WCHAR), &read);
    if (FAILED(hr))
    {
        heap_free(str);
        return hr;
    }
    if (read != len * sizeof(WCHAR))
    {
        heap_free(str);
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
    HlinkImpl *This = HlinkImpl_from_IPersistStream(iface);

    r = IStream_Read(pStm, hdr, sizeof(hdr), &read);
    if (read != sizeof(hdr) || (hdr[0] != HLINK_SAVE_MAGIC))
    {
        r = E_FAIL;
        goto end;
    }
    if (hdr[1] & ~HLINK_SAVE_ALL)
        FIXME("unknown flag(s) 0x%x\n", hdr[1] & ~HLINK_SAVE_ALL);

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
        This->absolute = hdr[1] & HLINK_SAVE_MONIKER_IS_ABSOLUTE ? TRUE : FALSE;
    }

    if (hdr[1] & HLINK_SAVE_LOCATION_PRESENT)
    {
        TRACE("loading location\n");
        r = read_hlink_string(pStm, &This->Location);
        if (FAILED(r)) goto end;
    }

end:
    TRACE("Load Result 0x%x (%p)\n", r, This->Moniker);

    return r;
}

static HRESULT WINAPI IPersistStream_fnSave(IPersistStream* iface,
        IStream* pStm, BOOL fClearDirty)
{
    HRESULT r = E_FAIL;
    HlinkImpl *This = HlinkImpl_from_IPersistStream(iface);
    DWORD hdr[2];
    IMoniker *moniker;

    TRACE("(%p) Moniker(%p)\n", This, This->Moniker);

    __GetMoniker(This, &moniker);

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
    TRACE("Save Result 0x%x\n", r);

    return r;
}

static HRESULT WINAPI IPersistStream_fnGetSizeMax(IPersistStream* iface,
        ULARGE_INTEGER* pcbSize)
{
    HRESULT r = E_FAIL;
    HlinkImpl *This = HlinkImpl_from_IPersistStream(iface);
    IMoniker *moniker;

    TRACE("(%p) Moniker(%p)\n", This, This->Moniker);

    pcbSize->QuadPart = sizeof(DWORD)*2;

    if (This->TargetFrameName)
        pcbSize->QuadPart += size_hlink_string(This->TargetFrameName);

    if (This->FriendlyName)
        pcbSize->QuadPart += size_hlink_string(This->FriendlyName);

    __GetMoniker(This, &moniker);
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
