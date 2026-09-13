/*
 * UrlMon
 *
 * Copyright 1999 Ulrich Czekalla for Corel Corporation
 * Copyright 2002 Huw D M Davies for CodeWeavers
 * Copyright 2005 Jacek Caban for CodeWeavers
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

#include "winreg.h"
#include "shlwapi.h"
#include "hlink.h"
#include "shellapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

typedef struct {
    IMoniker      IMoniker_iface;
    IUriContainer IUriContainer_iface;

    LONG ref;

    IUri *uri;
    BSTR URLName;
} URLMoniker;

static inline URLMoniker *impl_from_IMoniker(IMoniker *iface)
{
    return CONTAINING_RECORD(iface, URLMoniker, IMoniker_iface);
}

static HRESULT WINAPI URLMoniker_QueryInterface(IMoniker *iface, REFIID riid, void **ppv)
{
    URLMoniker *This = impl_from_IMoniker(iface);

    if(!ppv)
	return E_INVALIDARG;

    if(IsEqualIID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = iface;
    }else if(IsEqualIID(&IID_IPersist, riid)) {
        TRACE("(%p)->(IID_IPersist %p)\n", This, ppv);
        *ppv = iface;
    }else if(IsEqualIID(&IID_IPersistStream,riid)) {
        TRACE("(%p)->(IID_IPersistStream %p)\n", This, ppv);
        *ppv = iface;
    }else if(IsEqualIID(&IID_IMoniker, riid)) {
        TRACE("(%p)->(IID_IMoniker %p)\n", This, ppv);
        *ppv = iface;
    }else if(IsEqualIID(&IID_IAsyncMoniker, riid)) {
        TRACE("(%p)->(IID_IAsyncMoniker %p)\n", This, ppv);
        *ppv = iface;
    }else if(IsEqualIID(&IID_IUriContainer, riid)) {
        TRACE("(%p)->(IID_IUriContainer %p)\n", This, ppv);
        *ppv = &This->IUriContainer_iface;
    }else {
        WARN("(%p)->(%s,%p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI URLMoniker_AddRef(IMoniker *iface)
{
    URLMoniker *This = impl_from_IMoniker(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%lu\n",This, refCount);

    return refCount;
}

static ULONG WINAPI URLMoniker_Release(IMoniker *iface)
{
    URLMoniker *This = impl_from_IMoniker(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%lu\n",This, refCount);

    if (!refCount) {
        if(This->uri)
            IUri_Release(This->uri);
        SysFreeString(This->URLName);
        free(This);

        URLMON_UnlockModule();
    }

    return refCount;
}

static HRESULT WINAPI URLMoniker_GetClassID(IMoniker *iface, CLSID *pClassID)
{
    URLMoniker *This = impl_from_IMoniker(iface);

    TRACE("(%p,%p)\n", This, pClassID);

    if(!pClassID)
        return E_POINTER;

    /* Windows always returns CLSID_StdURLMoniker */
    *pClassID = CLSID_StdURLMoniker;
    return S_OK;
}

static HRESULT WINAPI URLMoniker_IsDirty(IMoniker *iface)
{
    URLMoniker *This = impl_from_IMoniker(iface);

    TRACE("(%p)\n",This);

    /* Note that the OLE-provided implementations of the IPersistStream::IsDirty
       method in the OLE-provided moniker interfaces always return S_FALSE because
       their internal state never changes. */
    return S_FALSE;
}

static HRESULT WINAPI URLMoniker_Load(IMoniker* iface,IStream* pStm)
{
    URLMoniker *This = impl_from_IMoniker(iface);
    WCHAR *new_uri_str;
    IUri *new_uri;
    BSTR new_url;
    ULONG size;
    ULONG got;
    HRESULT hres;

    TRACE("(%p,%p)\n",This,pStm);

    if(!pStm)
        return E_INVALIDARG;

    /*
     * NOTE
     *  Writes a ULONG containing length of unicode string, followed
     *  by that many unicode characters
     */
    hres = IStream_Read(pStm, &size, sizeof(ULONG), &got);
    if(FAILED(hres))
        return hres;
    if(got != sizeof(ULONG))
        return E_FAIL;

    new_uri_str = malloc(size + sizeof(WCHAR));
    if(!new_uri_str)
        return E_OUTOFMEMORY;

    hres = IStream_Read(pStm, new_uri_str, size, NULL);
    new_uri_str[size/sizeof(WCHAR)] = 0;
    if(SUCCEEDED(hres))
        hres = CreateUri(new_uri_str, 0, 0, &new_uri);
    free(new_uri_str);
    if(FAILED(hres))
        return hres;

    hres = IUri_GetDisplayUri(new_uri, &new_url);
    if(FAILED(hres)) {
        IUri_Release(new_uri);
        return hres;
    }

    SysFreeString(This->URLName);
    if(This->uri)
        IUri_Release(This->uri);

    This->uri = new_uri;
    This->URLName = new_url;
    return S_OK;
}

static HRESULT WINAPI URLMoniker_Save(IMoniker *iface, IStream* pStm, BOOL fClearDirty)
{
    URLMoniker *This = impl_from_IMoniker(iface);
    HRESULT res;
    ULONG size;

    TRACE("(%p,%p,%d)\n", This, pStm, fClearDirty);

    if(!pStm)
        return E_INVALIDARG;

    size = (SysStringLen(This->URLName) + 1)*sizeof(WCHAR);
    res=IStream_Write(pStm,&size,sizeof(ULONG),NULL);
    if(SUCCEEDED(res))
        res=IStream_Write(pStm,This->URLName,size,NULL);

    return res;

}

static HRESULT WINAPI URLMoniker_GetSizeMax(IMoniker* iface, ULARGE_INTEGER *pcbSize)
{
    URLMoniker *This = impl_from_IMoniker(iface);

    TRACE("(%p,%p)\n",This,pcbSize);

    if(!pcbSize)
        return E_INVALIDARG;

    pcbSize->QuadPart = sizeof(ULONG) + ((SysStringLen(This->URLName)+1) * sizeof(WCHAR));
    return S_OK;
}

static HRESULT WINAPI URLMoniker_BindToObject(IMoniker *iface, IBindCtx* pbc, IMoniker *pmkToLeft,
        REFIID riid, void **ppv)
{
    URLMoniker *This = impl_from_IMoniker(iface);
    IRunningObjectTable *obj_tbl;
    HRESULT hres;

    TRACE("(%p)->(%p,%p,%s,%p)\n", This, pbc, pmkToLeft, debugstr_guid(riid), ppv);

    hres = IBindCtx_GetRunningObjectTable(pbc, &obj_tbl);
    if(SUCCEEDED(hres)) {
        hres = IRunningObjectTable_IsRunning(obj_tbl, &This->IMoniker_iface);
        if(hres == S_OK) {
            IUnknown *unk = NULL;

            TRACE("Found in running object table\n");

            hres = IRunningObjectTable_GetObject(obj_tbl, &This->IMoniker_iface, &unk);
            if(SUCCEEDED(hres)) {
                hres = IUnknown_QueryInterface(unk, riid, ppv);
                IUnknown_Release(unk);
            }

            IRunningObjectTable_Release(obj_tbl);
            return hres;
        }

        IRunningObjectTable_Release(obj_tbl);
    }

    if(!This->uri) {
        *ppv = NULL;
        return MK_E_SYNTAX;
    }

    return bind_to_object(&This->IMoniker_iface, This->uri, pbc, riid, ppv);
}

static HRESULT WINAPI URLMoniker_BindToStorage(IMoniker* iface, IBindCtx* pbc,
        IMoniker* pmkToLeft, REFIID riid, void **ppvObject)
{
    URLMoniker *This = impl_from_IMoniker(iface);

    TRACE("(%p)->(%p %p %s %p)\n", This, pbc, pmkToLeft, debugstr_guid(riid), ppvObject);

    if(ppvObject) *ppvObject = NULL;

    if(!pbc || !ppvObject) return E_INVALIDARG;

    if(pmkToLeft)
        FIXME("Unsupported pmkToLeft\n");

    if(!This->uri)
        return MK_E_SYNTAX;

    return bind_to_storage(This->uri, pbc, riid, ppvObject);
}

static HRESULT WINAPI URLMoniker_Reduce(IMoniker *iface, IBindCtx *pbc,
        DWORD dwReduceHowFar, IMoniker **ppmkToLeft, IMoniker **ppmkReduced)
{
    URLMoniker *This = impl_from_IMoniker(iface);

    TRACE("(%p,%p,%ld,%p,%p)\n", This, pbc, dwReduceHowFar, ppmkToLeft, ppmkReduced);

    if(!ppmkReduced)
        return E_INVALIDARG;

    IMoniker_AddRef(iface);
    *ppmkReduced = iface;
    return MK_S_REDUCED_TO_SELF;
}

static HRESULT WINAPI URLMoniker_ComposeWith(IMoniker *iface, IMoniker *right,
        BOOL only_if_not_generic, IMoniker **composite)
{
    HRESULT res;
    IUri *right_uri;
    IUriContainer *uri_container;

    TRACE("(%p)->(%p,%d,%p)\n", iface, right, only_if_not_generic, composite);

    if (!right || !composite) return E_INVALIDARG;

    res = IMoniker_QueryInterface(right, &IID_IUriContainer, (void**)&uri_container);
    if (SUCCEEDED(res)){
        res = IUriContainer_GetIUri(uri_container, &right_uri);
        if (SUCCEEDED(res)) res = CreateURLMonikerEx2(iface, right_uri, composite, 0);
        IUriContainer_Release(uri_container);
        return res;
    }

    if(only_if_not_generic) return MK_E_NEEDGENERIC;
    return CreateGenericComposite(iface, right, composite);
}

static HRESULT WINAPI URLMoniker_Enum(IMoniker *iface, BOOL fForward, IEnumMoniker **ppenumMoniker)
{
    URLMoniker *This = impl_from_IMoniker(iface);

    TRACE("(%p,%d,%p)\n", This, fForward, ppenumMoniker);

    if(!ppenumMoniker)
        return E_INVALIDARG;

    /* Does not support sub-monikers */
    *ppenumMoniker = NULL;
    return S_OK;
}

static HRESULT WINAPI URLMoniker_IsEqual(IMoniker *iface, IMoniker *pmkOtherMoniker)
{
    URLMoniker *This = impl_from_IMoniker(iface);
    CLSID clsid;
    LPOLESTR urlPath;
    IBindCtx* bind;
    HRESULT res;

    TRACE("(%p,%p)\n",This, pmkOtherMoniker);

    if(pmkOtherMoniker==NULL)
        return E_INVALIDARG;

    IMoniker_GetClassID(pmkOtherMoniker,&clsid);

    if(!IsEqualCLSID(&clsid,&CLSID_StdURLMoniker))
        return S_FALSE;

    res = CreateBindCtx(0,&bind);
    if(FAILED(res))
        return res;

    res = S_FALSE;
    if(SUCCEEDED(IMoniker_GetDisplayName(pmkOtherMoniker,bind,NULL,&urlPath))) {
        int result = lstrcmpiW(urlPath, This->URLName);
        CoTaskMemFree(urlPath);
        if(result == 0)
            res = S_OK;
    }
    IBindCtx_Release(bind);
    return res;
}


static HRESULT WINAPI URLMoniker_Hash(IMoniker *iface, DWORD *pdwHash)
{
    URLMoniker *This = impl_from_IMoniker(iface);
    int  h = 0,i,skip,len;
    int  off = 0;
    LPOLESTR val;

    TRACE("(%p,%p)\n",This,pdwHash);

    if(!pdwHash)
        return E_INVALIDARG;

    val = This->URLName;
    len = lstrlenW(val);

    if(len < 16) {
        for(i = len ; i > 0; i--) {
            h = (h * 37) + val[off++];
        }
    }else {
        /* only sample some characters */
        skip = len / 8;
        for(i = len; i > 0; i -= skip, off += skip) {
            h = (h * 39) + val[off];
        }
    }
    *pdwHash = h;
    return S_OK;
}

static HRESULT WINAPI URLMoniker_IsRunning(IMoniker* iface, IBindCtx* pbc,
        IMoniker *pmkToLeft, IMoniker *pmkNewlyRunning)
{
    URLMoniker *This = impl_from_IMoniker(iface);
    FIXME("(%p)->(%p,%p,%p): stub\n",This,pbc,pmkToLeft,pmkNewlyRunning);
    return E_NOTIMPL;
}

static HRESULT WINAPI URLMoniker_GetTimeOfLastChange(IMoniker *iface,
        IBindCtx *pbc, IMoniker *pmkToLeft, FILETIME *pFileTime)
{
    URLMoniker *This = impl_from_IMoniker(iface);
    FIXME("(%p)->(%p,%p,%p): stub\n", This, pbc, pmkToLeft, pFileTime);
    return E_NOTIMPL;
}

static HRESULT WINAPI URLMoniker_Inverse(IMoniker *iface, IMoniker **ppmk)
{
    URLMoniker *This = impl_from_IMoniker(iface);
    TRACE("(%p,%p)\n",This,ppmk);
    return MK_E_NOINVERSE;
}

static HRESULT WINAPI URLMoniker_CommonPrefixWith(IMoniker *iface, IMoniker *pmkOther, IMoniker **ppmkPrefix)
{
    URLMoniker *This = impl_from_IMoniker(iface);
    FIXME("(%p)->(%p,%p): stub\n",This,pmkOther,ppmkPrefix);
    return E_NOTIMPL;
}

static HRESULT WINAPI URLMoniker_RelativePathTo(IMoniker *iface, IMoniker *pmOther, IMoniker **ppmkRelPath)
{
    URLMoniker *This = impl_from_IMoniker(iface);
    FIXME("(%p)->(%p,%p): stub\n",This,pmOther,ppmkRelPath);
    return E_NOTIMPL;
}

static HRESULT WINAPI URLMoniker_GetDisplayName(IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft,
        LPOLESTR *ppszDisplayName)
{
    URLMoniker *This = impl_from_IMoniker(iface);
    int len;
    
    TRACE("(%p,%p,%p,%p)\n", This, pbc, pmkToLeft, ppszDisplayName);
    
    if(!ppszDisplayName)
        return E_INVALIDARG;

    if(!This->URLName)
        return E_OUTOFMEMORY;

    /* FIXME: If this is a partial URL, try and get a URL moniker from SZ_URLCONTEXT in the bind context,
        then look at pmkToLeft to try and complete the URL
    */
    len = SysStringLen(This->URLName)+1;
    *ppszDisplayName = CoTaskMemAlloc(len*sizeof(WCHAR));
    if(!*ppszDisplayName)
        return E_OUTOFMEMORY;
    lstrcpyW(*ppszDisplayName, This->URLName);
    return S_OK;
}

static HRESULT WINAPI URLMoniker_ParseDisplayName(IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft,
        LPOLESTR pszDisplayName, ULONG *pchEaten, IMoniker **ppmkOut)
{
    URLMoniker *This = impl_from_IMoniker(iface);
    FIXME("(%p)->(%p,%p,%p,%p,%p): stub\n",This,pbc,pmkToLeft,pszDisplayName,pchEaten,ppmkOut);
    return E_NOTIMPL;
}

static HRESULT WINAPI URLMoniker_IsSystemMoniker(IMoniker *iface, DWORD *pwdMksys)
{
    URLMoniker *This = impl_from_IMoniker(iface);

    TRACE("(%p,%p)\n",This,pwdMksys);

    if(!pwdMksys)
        return E_INVALIDARG;

    *pwdMksys = MKSYS_URLMONIKER;
    return S_OK;
}

static const IMonikerVtbl URLMonikerVtbl =
{
    URLMoniker_QueryInterface,
    URLMoniker_AddRef,
    URLMoniker_Release,
    URLMoniker_GetClassID,
    URLMoniker_IsDirty,
    URLMoniker_Load,
    URLMoniker_Save,
    URLMoniker_GetSizeMax,
    URLMoniker_BindToObject,
    URLMoniker_BindToStorage,
    URLMoniker_Reduce,
    URLMoniker_ComposeWith,
    URLMoniker_Enum,
    URLMoniker_IsEqual,
    URLMoniker_Hash,
    URLMoniker_IsRunning,
    URLMoniker_GetTimeOfLastChange,
    URLMoniker_Inverse,
    URLMoniker_CommonPrefixWith,
    URLMoniker_RelativePathTo,
    URLMoniker_GetDisplayName,
    URLMoniker_ParseDisplayName,
    URLMoniker_IsSystemMoniker
};

static inline URLMoniker *impl_from_IUriContainer(IUriContainer *iface)
{
    return CONTAINING_RECORD(iface, URLMoniker, IUriContainer_iface);
}

static HRESULT WINAPI UriContainer_QueryInterface(IUriContainer *iface, REFIID riid, void **ppv)
{
    URLMoniker *This = impl_from_IUriContainer(iface);
    return IMoniker_QueryInterface(&This->IMoniker_iface, riid, ppv);
}

static ULONG WINAPI UriContainer_AddRef(IUriContainer *iface)
{
    URLMoniker *This = impl_from_IUriContainer(iface);
    return IMoniker_AddRef(&This->IMoniker_iface);
}

static ULONG WINAPI UriContainer_Release(IUriContainer *iface)
{
    URLMoniker *This = impl_from_IUriContainer(iface);
    return IMoniker_Release(&This->IMoniker_iface);
}

static HRESULT WINAPI UriContainer_GetIUri(IUriContainer *iface, IUri **ppIUri)
{
    URLMoniker *This = impl_from_IUriContainer(iface);

    TRACE("(%p)->(%p)\n", This, ppIUri);

    if(!This->uri) {
        *ppIUri = NULL;
        return S_FALSE;
    }

    IUri_AddRef(This->uri);
    *ppIUri = This->uri;
    return S_OK;
}

static const IUriContainerVtbl UriContainerVtbl = {
    UriContainer_QueryInterface,
    UriContainer_AddRef,
    UriContainer_Release,
    UriContainer_GetIUri
};

static HRESULT create_moniker(IUri *uri, URLMoniker **ret)
{
    URLMoniker *mon;
    HRESULT hres;

    mon = malloc(sizeof(*mon));
    if(!mon)
        return E_OUTOFMEMORY;

    mon->IMoniker_iface.lpVtbl = &URLMonikerVtbl;
    mon->IUriContainer_iface.lpVtbl = &UriContainerVtbl;
    mon->ref = 1;

    if(uri) {
        /* FIXME: try to avoid it */
        hres = IUri_GetDisplayUri(uri, &mon->URLName);
        if(FAILED(hres)) {
            free(mon);
            return hres;
        }

        IUri_AddRef(uri);
        mon->uri = uri;
    }else {
        mon->URLName = NULL;
        mon->uri = NULL;
    }

    URLMON_LockModule();
    *ret = mon;
    return S_OK;
}

HRESULT StdURLMoniker_Construct(IUnknown *outer, void **ppv)
{
    URLMoniker *mon;
    HRESULT hres;

    TRACE("(%p %p)\n", outer, ppv);

    hres = create_moniker(NULL, &mon);
    if(FAILED(hres))
        return hres;

    *ppv = &mon->IMoniker_iface;
    return S_OK;
}

static const DWORD create_flags_map[3] = {
    Uri_CREATE_FILE_USE_DOS_PATH,  /* URL_MK_LEGACY */
    0,                             /* URL_MK_UNIFORM */
    Uri_CREATE_NO_CANONICALIZE     /* URL_MK_NO_CANONICALIZE */
};

static const DWORD combine_flags_map[3] = {
    URL_FILE_USE_PATHURL,  /* URL_MK_LEGACY */
    0,                     /* URL_MK_UNIFORM */
    URL_DONT_SIMPLIFY      /* URL_MK_NO_CANONICALIZE */
};

/***********************************************************************
 *           CreateURLMonikerEx (URLMON.@)
 *
 * Create a url moniker.
 *
 * PARAMS
 *    pmkContext [I] Context
 *    szURL      [I] Url to create the moniker for
 *    ppmk       [O] Destination for created moniker.
 *    dwFlags    [I] Flags.
 *
 * RETURNS
 *    Success: S_OK. ppmk contains the created IMoniker object.
 *    Failure: MK_E_SYNTAX if szURL is not a valid url, or
 *             E_OUTOFMEMORY if memory allocation fails.
 */
HRESULT WINAPI CreateURLMonikerEx(IMoniker *pmkContext, LPCWSTR szURL, IMoniker **ppmk, DWORD dwFlags)
{
    IUri *uri, *base_uri = NULL;
    URLMoniker *obj;
    HRESULT hres;

    TRACE("(%p, %s, %p, %08lx)\n", pmkContext, debugstr_w(szURL), ppmk, dwFlags);

    if (ppmk)
        *ppmk = NULL;

    if (!szURL || !ppmk)
        return E_INVALIDARG;

    if(dwFlags >= ARRAY_SIZE(create_flags_map)) {
        FIXME("Unsupported flags %lx\n", dwFlags);
        return E_INVALIDARG;
    }

    if(pmkContext) {
        IUriContainer *uri_container;

        hres = IMoniker_QueryInterface(pmkContext, &IID_IUriContainer, (void**)&uri_container);
        if(SUCCEEDED(hres)) {
            hres = IUriContainer_GetIUri(uri_container, &base_uri);
            IUriContainer_Release(uri_container);
            if(FAILED(hres))
                return hres;
        }
    }

    if(base_uri) {
        hres = CoInternetCombineUrlEx(base_uri, szURL, combine_flags_map[dwFlags], &uri, 0);
        IUri_Release(base_uri);
    }else {
        hres = CreateUri(szURL, Uri_CREATE_ALLOW_RELATIVE|Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME|create_flags_map[dwFlags], 0, &uri);
    }
    if(FAILED(hres))
        return hres;

    hres = create_moniker(uri, &obj);
    IUri_Release(uri);
    if(FAILED(hres))
	return hres;

    *ppmk = &obj->IMoniker_iface;
    return S_OK;
}

/***********************************************************************
 *           CreateURLMonikerEx2 (URLMON.@)
 */
HRESULT WINAPI CreateURLMonikerEx2(IMoniker *pmkContext, IUri *pUri, IMoniker **ppmk, DWORD dwFlags)
{
    IUri *context_uri = NULL, *uri;
    IUriContainer *uri_container;
    URLMoniker *ret;
    HRESULT hres;

    TRACE("(%p %p %p %lx)\n", pmkContext, pUri, ppmk, dwFlags);

    if (ppmk)
        *ppmk = NULL;

    if (!pUri || !ppmk)
        return E_INVALIDARG;

    if(dwFlags >= ARRAY_SIZE(create_flags_map)) {
        FIXME("Unsupported flags %lx\n", dwFlags);
        return E_INVALIDARG;
    }

    if(pmkContext) {
        hres = IMoniker_QueryInterface(pmkContext, &IID_IUriContainer, (void**)&uri_container);
        if(SUCCEEDED(hres)) {
            hres = IUriContainer_GetIUri(uri_container, &context_uri);
            if(FAILED(hres))
                context_uri = NULL;
            IUriContainer_Release(uri_container);
        }
    }

    if(context_uri) {
        hres = CoInternetCombineIUri(context_uri, pUri, combine_flags_map[dwFlags], &uri, 0);
        IUri_Release(context_uri);
        if(FAILED(hres))
            return hres;
    }else {
        uri = pUri;
        IUri_AddRef(uri);
    }

    hres = create_moniker(uri, &ret);
    IUri_Release(uri);
    if(FAILED(hres))
        return hres;

    *ppmk = &ret->IMoniker_iface;
    return S_OK;
}

/**********************************************************************
 *           CreateURLMoniker (URLMON.@)
 *
 * Create a url moniker.
 *
 * PARAMS
 *    pmkContext [I] Context
 *    szURL      [I] Url to create the moniker for
 *    ppmk       [O] Destination for created moniker.
 *
 * RETURNS
 *    Success: S_OK. ppmk contains the created IMoniker object.
 *    Failure: MK_E_SYNTAX if szURL is not a valid url, or
 *             E_OUTOFMEMORY if memory allocation fails.
 */
HRESULT WINAPI CreateURLMoniker(IMoniker *pmkContext, LPCWSTR szURL, IMoniker **ppmk)
{
    return CreateURLMonikerEx(pmkContext, szURL, ppmk, URL_MK_LEGACY);
}

/***********************************************************************
 *           IsAsyncMoniker (URLMON.@)
 */
HRESULT WINAPI IsAsyncMoniker(IMoniker *pmk)
{
    IUnknown *am;
    
    TRACE("(%p)\n", pmk);
    if(!pmk)
        return E_INVALIDARG;
    if(SUCCEEDED(IMoniker_QueryInterface(pmk, &IID_IAsyncMoniker, (void**)&am))) {
        IUnknown_Release(am);
        return S_OK;
    }
    return S_FALSE;
}

/***********************************************************************
 *           BindAsyncMoniker (URLMON.@)
 *
 * Bind a bind status callback to an asynchronous URL Moniker.
 *
 * PARAMS
 *  pmk           [I] Moniker object to bind status callback to
 *  grfOpt        [I] Options, seems not used
 *  pbsc          [I] Status callback to bind
 *  iidResult     [I] Interface to return
 *  ppvResult     [O] Resulting asynchronous moniker object
 *
 * RETURNS
 *    Success: S_OK.
 *    Failure: E_INVALIDARG, if any argument is invalid, or
 *             E_OUTOFMEMORY if memory allocation fails.
 */
HRESULT WINAPI BindAsyncMoniker(IMoniker *pmk, DWORD grfOpt, IBindStatusCallback *pbsc, REFIID iidResult, LPVOID *ppvResult)
{
    LPBC pbc = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p %08lx %p %s %p)\n", pmk, grfOpt, pbsc, debugstr_guid(iidResult), ppvResult);

    if (pmk && ppvResult)
    {
        *ppvResult = NULL;

        hr = CreateAsyncBindCtx(0, pbsc, NULL, &pbc);
        if (hr == NOERROR)
        {
            hr = IMoniker_BindToObject(pmk, pbc, NULL, iidResult, ppvResult);
            IBindCtx_Release(pbc);
        }
    }
    return hr;
}

/***********************************************************************
 *           MkParseDisplayNameEx (URLMON.@)
 */
HRESULT WINAPI MkParseDisplayNameEx(IBindCtx *pbc, LPCWSTR szDisplayName, ULONG *pchEaten, LPMONIKER *ppmk)
{
    TRACE("(%p %s %p %p)\n", pbc, debugstr_w(szDisplayName), pchEaten, ppmk);

    if (!pbc || !szDisplayName || !*szDisplayName || !pchEaten || !ppmk)
        return E_INVALIDARG;

    if(is_registered_protocol(szDisplayName)) {
        HRESULT hres;

        hres = CreateURLMoniker(NULL, szDisplayName, ppmk);
        if(SUCCEEDED(hres)) {
            *pchEaten = lstrlenW(szDisplayName);
            return hres;
        }
    }

    return MkParseDisplayName(pbc, szDisplayName, pchEaten, ppmk);
}


/***********************************************************************
 *           URLDownloadToCacheFileA (URLMON.@)
 */
HRESULT WINAPI URLDownloadToCacheFileA(LPUNKNOWN lpUnkCaller, LPCSTR szURL, LPSTR szFileName,
        DWORD dwBufLength, DWORD dwReserved, LPBINDSTATUSCALLBACK pBSC)
{
    LPWSTR url = NULL, file_name = NULL;
    int len;
    HRESULT hres;

    TRACE("(%p %s %p %ld %ld %p)\n", lpUnkCaller, debugstr_a(szURL), szFileName,
            dwBufLength, dwReserved, pBSC);

    if(szURL) {
        len = MultiByteToWideChar(CP_ACP, 0, szURL, -1, NULL, 0);
        url = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, szURL, -1, url, len);
    }

    if(szFileName)
        file_name = malloc(dwBufLength * sizeof(WCHAR));

    hres = URLDownloadToCacheFileW(lpUnkCaller, url, file_name, dwBufLength*sizeof(WCHAR),
            dwReserved, pBSC);

    if(SUCCEEDED(hres) && file_name)
        WideCharToMultiByte(CP_ACP, 0, file_name, -1, szFileName, dwBufLength, NULL, NULL);

    free(url);
    free(file_name);

    return hres;
}

/***********************************************************************
 *           URLDownloadToCacheFileW (URLMON.@)
 */
HRESULT WINAPI URLDownloadToCacheFileW(LPUNKNOWN lpUnkCaller, LPCWSTR szURL, LPWSTR szFileName,
                DWORD dwBufLength, DWORD dwReserved, LPBINDSTATUSCALLBACK pBSC)
{
    WCHAR cache_path[MAX_PATH + 1];
    FILETIME expire, modified;
    HRESULT hr;
    LPWSTR ext;

    static WCHAR header[] = L"HTTP/1.0 200 OK\\r\\n\\r\\n";

    TRACE("(%p, %s, %p, %ld, %ld, %p)\n", lpUnkCaller, debugstr_w(szURL),
          szFileName, dwBufLength, dwReserved, pBSC);

    if (!szURL || !szFileName)
        return E_INVALIDARG;

    ext = PathFindExtensionW(szURL);

    if (!CreateUrlCacheEntryW(szURL, 0, ext, cache_path, 0))
        return E_FAIL;

    hr = URLDownloadToFileW(lpUnkCaller, szURL, cache_path, 0, pBSC);
    if (FAILED(hr))
        return hr;

    expire.dwHighDateTime = 0;
    expire.dwLowDateTime = 0;
    modified.dwHighDateTime = 0;
    modified.dwLowDateTime = 0;

    if (!CommitUrlCacheEntryW(szURL, cache_path, expire, modified, NORMAL_CACHE_ENTRY,
                              header, sizeof(header), NULL, NULL))
        return E_FAIL;

    if (lstrlenW(cache_path) > dwBufLength)
        return E_OUTOFMEMORY;

    lstrcpyW(szFileName, cache_path);

    return S_OK;
}

/***********************************************************************
 *           HlinkSimpleNavigateToMoniker (URLMON.@)
 */
HRESULT WINAPI HlinkSimpleNavigateToMoniker(IMoniker *pmkTarget,
    LPCWSTR szLocation, LPCWSTR szTargetFrameName, IUnknown *pUnk,
    IBindCtx *pbc, IBindStatusCallback *pbsc, DWORD grfHLNF, DWORD dwReserved)
{
    LPWSTR target;
    HRESULT hres;

    TRACE("\n");

    hres = IMoniker_GetDisplayName(pmkTarget, pbc, 0, &target);
    if(hres == S_OK)
        hres = HlinkSimpleNavigateToString( target, szLocation, szTargetFrameName,
                                            pUnk, pbc, pbsc, grfHLNF, dwReserved );
    CoTaskMemFree(target);

    return hres;
}

/***********************************************************************
 *           HlinkSimpleNavigateToString (URLMON.@)
 */
HRESULT WINAPI HlinkSimpleNavigateToString( LPCWSTR szTarget,
    LPCWSTR szLocation, LPCWSTR szTargetFrameName, IUnknown *pUnk,
    IBindCtx *pbc, IBindStatusCallback *pbsc, DWORD grfHLNF, DWORD dwReserved)
{
    FIXME("%s %s %s %p %p %p %lu %lu partial stub\n", debugstr_w( szTarget ), debugstr_w( szLocation ),
          debugstr_w( szTargetFrameName ), pUnk, pbc, pbsc, grfHLNF, dwReserved);

    /* undocumented: 0 means HLNF_OPENINNEWWINDOW*/
    if (!grfHLNF) grfHLNF = HLNF_OPENINNEWWINDOW;

    if (grfHLNF == HLNF_OPENINNEWWINDOW)
    {
        SHELLEXECUTEINFOW sei;

        memset(&sei, 0, sizeof(sei));
        sei.cbSize = sizeof(sei);
        sei.lpVerb = L"open";
        sei.nShow = SW_SHOWNORMAL;
        sei.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NO_CONSOLE;
        sei.lpFile = szTarget;

        if (ShellExecuteExW(&sei)) return S_OK;
    }

    return E_NOTIMPL;
}

/***********************************************************************
 *           HlinkNavigateString (URLMON.@)
 */
HRESULT WINAPI HlinkNavigateString( IUnknown *pUnk, LPCWSTR szTarget )
{
    TRACE("%p %s\n", pUnk, debugstr_w( szTarget ) );
    return HlinkSimpleNavigateToString( 
               szTarget, NULL, NULL, pUnk, NULL, NULL, 0, 0 );
}

/***********************************************************************
 *           GetSoftwareUpdateInfo (URLMON.@)
 */
HRESULT WINAPI GetSoftwareUpdateInfo( LPCWSTR szDistUnit, LPSOFTDISTINFO psdi )
{
    FIXME("%s %p\n", debugstr_w(szDistUnit), psdi );
    return E_FAIL;
}
