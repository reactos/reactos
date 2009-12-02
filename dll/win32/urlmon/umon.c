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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

typedef struct {
    const IMonikerVtbl *lpIMonikerVtbl;

    LONG ref;

    LPOLESTR URLName; /* URL string identified by this URLmoniker */
} URLMoniker;

#define MONIKER_THIS(iface) DEFINE_THIS(URLMoniker, IMoniker, iface)

static HRESULT WINAPI URLMoniker_QueryInterface(IMoniker *iface, REFIID riid, void **ppv)
{
    URLMoniker *This = MONIKER_THIS(iface);

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
    }else {
        WARN("(%p)->(%s,%p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IMoniker_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI URLMoniker_AddRef(IMoniker *iface)
{
    URLMoniker *This = MONIKER_THIS(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n",This, refCount);

    return refCount;
}

static ULONG WINAPI URLMoniker_Release(IMoniker *iface)
{
    URLMoniker *This = MONIKER_THIS(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n",This, refCount);

    if (!refCount) {
        heap_free(This->URLName);
        heap_free(This);

        URLMON_UnlockModule();
    }

    return refCount;
}

static HRESULT WINAPI URLMoniker_GetClassID(IMoniker *iface, CLSID *pClassID)
{
    URLMoniker *This = MONIKER_THIS(iface);

    TRACE("(%p,%p)\n", This, pClassID);

    if(!pClassID)
        return E_POINTER;

    /* Windows always returns CLSID_StdURLMoniker */
    *pClassID = CLSID_StdURLMoniker;
    return S_OK;
}

static HRESULT WINAPI URLMoniker_IsDirty(IMoniker *iface)
{
    URLMoniker *This = MONIKER_THIS(iface);

    TRACE("(%p)\n",This);

    /* Note that the OLE-provided implementations of the IPersistStream::IsDirty
       method in the OLE-provided moniker interfaces always return S_FALSE because
       their internal state never changes. */
    return S_FALSE;
}

static HRESULT WINAPI URLMoniker_Load(IMoniker* iface,IStream* pStm)
{
    URLMoniker *This = MONIKER_THIS(iface);
    HRESULT res;
    ULONG size;
    ULONG got;

    TRACE("(%p,%p)\n",This,pStm);

    if(!pStm)
        return E_INVALIDARG;

    /*
     * NOTE
     *  Writes a ULONG containing length of unicode string, followed
     *  by that many unicode characters
     */
    res = IStream_Read(pStm, &size, sizeof(ULONG), &got);
    if(SUCCEEDED(res)) {
        if(got == sizeof(ULONG)) {
            heap_free(This->URLName);
            This->URLName = heap_alloc(size);
            if(!This->URLName)
                res = E_OUTOFMEMORY;
            else {
                res = IStream_Read(pStm, This->URLName, size, NULL);
                This->URLName[size/sizeof(WCHAR) - 1] = 0;
            }
        }
        else
            res = E_FAIL;
    }

    return res;
}

static HRESULT WINAPI URLMoniker_Save(IMoniker *iface, IStream* pStm, BOOL fClearDirty)
{
    URLMoniker *This = MONIKER_THIS(iface);
    HRESULT res;
    ULONG size;

    TRACE("(%p,%p,%d)\n", This, pStm, fClearDirty);

    if(!pStm)
        return E_INVALIDARG;

    size = (strlenW(This->URLName) + 1)*sizeof(WCHAR);
    res=IStream_Write(pStm,&size,sizeof(ULONG),NULL);
    if(SUCCEEDED(res))
        res=IStream_Write(pStm,This->URLName,size,NULL);

    return res;

}

static HRESULT WINAPI URLMoniker_GetSizeMax(IMoniker* iface, ULARGE_INTEGER *pcbSize)
{
    URLMoniker *This = MONIKER_THIS(iface);

    TRACE("(%p,%p)\n",This,pcbSize);

    if(!pcbSize)
        return E_INVALIDARG;

    pcbSize->QuadPart = sizeof(ULONG) + ((strlenW(This->URLName)+1) * sizeof(WCHAR));
    return S_OK;
}

static HRESULT WINAPI URLMoniker_BindToObject(IMoniker *iface, IBindCtx* pbc, IMoniker *pmkToLeft,
        REFIID riid, void **ppv)
{
    URLMoniker *This = MONIKER_THIS(iface);
    IRunningObjectTable *obj_tbl;
    HRESULT hres;

    TRACE("(%p)->(%p,%p,%s,%p): stub\n", This, pbc, pmkToLeft, debugstr_guid(riid), ppv);

    hres = IBindCtx_GetRunningObjectTable(pbc, &obj_tbl);
    if(SUCCEEDED(hres)) {
        FIXME("use running object table\n");
        IRunningObjectTable_Release(obj_tbl);
    }

    return bind_to_object(iface, This->URLName, pbc, riid, ppv);
}

static HRESULT WINAPI URLMoniker_BindToStorage(IMoniker* iface, IBindCtx* pbc,
        IMoniker* pmkToLeft, REFIID riid, void **ppvObject)
{
    URLMoniker *This = MONIKER_THIS(iface);

    TRACE("(%p)->(%p %p %s %p)\n", This, pbc, pmkToLeft, debugstr_guid(riid), ppvObject);

    if(pmkToLeft)
        FIXME("Unsupported pmkToLeft\n");

    return bind_to_storage(This->URLName, pbc, riid, ppvObject);
}

static HRESULT WINAPI URLMoniker_Reduce(IMoniker *iface, IBindCtx *pbc,
        DWORD dwReduceHowFar, IMoniker **ppmkToLeft, IMoniker **ppmkReduced)
{
    URLMoniker *This = MONIKER_THIS(iface);
    
    TRACE("(%p,%p,%d,%p,%p)\n", This, pbc, dwReduceHowFar, ppmkToLeft, ppmkReduced);

    if(!ppmkReduced)
        return E_INVALIDARG;

    IMoniker_AddRef(iface);
    *ppmkReduced = iface;
    return MK_S_REDUCED_TO_SELF;
}

static HRESULT WINAPI URLMoniker_ComposeWith(IMoniker *iface, IMoniker *pmkRight,
        BOOL fOnlyIfNotGeneric, IMoniker **ppmkComposite)
{
    URLMoniker *This = MONIKER_THIS(iface);
    FIXME("(%p)->(%p,%d,%p): stub\n",This,pmkRight,fOnlyIfNotGeneric,ppmkComposite);
    return E_NOTIMPL;
}

static HRESULT WINAPI URLMoniker_Enum(IMoniker *iface, BOOL fForward, IEnumMoniker **ppenumMoniker)
{
    URLMoniker *This = MONIKER_THIS(iface);

    TRACE("(%p,%d,%p)\n", This, fForward, ppenumMoniker);

    if(!ppenumMoniker)
        return E_INVALIDARG;

    /* Does not support sub-monikers */
    *ppenumMoniker = NULL;
    return S_OK;
}

static HRESULT WINAPI URLMoniker_IsEqual(IMoniker *iface, IMoniker *pmkOtherMoniker)
{
    URLMoniker *This = MONIKER_THIS(iface);
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
    IUnknown_Release(bind);
    return res;
}


static HRESULT WINAPI URLMoniker_Hash(IMoniker *iface, DWORD *pdwHash)
{
    URLMoniker *This = MONIKER_THIS(iface);
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
    URLMoniker *This = MONIKER_THIS(iface);
    FIXME("(%p)->(%p,%p,%p): stub\n",This,pbc,pmkToLeft,pmkNewlyRunning);
    return E_NOTIMPL;
}

static HRESULT WINAPI URLMoniker_GetTimeOfLastChange(IMoniker *iface,
        IBindCtx *pbc, IMoniker *pmkToLeft, FILETIME *pFileTime)
{
    URLMoniker *This = MONIKER_THIS(iface);
    FIXME("(%p)->(%p,%p,%p): stub\n", This, pbc, pmkToLeft, pFileTime);
    return E_NOTIMPL;
}

static HRESULT WINAPI URLMoniker_Inverse(IMoniker *iface, IMoniker **ppmk)
{
    URLMoniker *This = MONIKER_THIS(iface);
    TRACE("(%p,%p)\n",This,ppmk);
    return MK_E_NOINVERSE;
}

static HRESULT WINAPI URLMoniker_CommonPrefixWith(IMoniker *iface, IMoniker *pmkOther, IMoniker **ppmkPrefix)
{
    URLMoniker *This = MONIKER_THIS(iface);
    FIXME("(%p)->(%p,%p): stub\n",This,pmkOther,ppmkPrefix);
    return E_NOTIMPL;
}

static HRESULT WINAPI URLMoniker_RelativePathTo(IMoniker *iface, IMoniker *pmOther, IMoniker **ppmkRelPath)
{
    URLMoniker *This = MONIKER_THIS(iface);
    FIXME("(%p)->(%p,%p): stub\n",This,pmOther,ppmkRelPath);
    return E_NOTIMPL;
}

static HRESULT WINAPI URLMoniker_GetDisplayName(IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft,
        LPOLESTR *ppszDisplayName)
{
    URLMoniker *This = MONIKER_THIS(iface);
    int len;
    
    TRACE("(%p,%p,%p,%p)\n", This, pbc, pmkToLeft, ppszDisplayName);
    
    if(!ppszDisplayName)
        return E_INVALIDARG;

    if(!This->URLName)
        return E_OUTOFMEMORY;

    /* FIXME: If this is a partial URL, try and get a URL moniker from SZ_URLCONTEXT in the bind context,
        then look at pmkToLeft to try and complete the URL
    */
    len = lstrlenW(This->URLName)+1;
    *ppszDisplayName = CoTaskMemAlloc(len*sizeof(WCHAR));
    if(!*ppszDisplayName)
        return E_OUTOFMEMORY;
    lstrcpyW(*ppszDisplayName, This->URLName);
    return S_OK;
}

static HRESULT WINAPI URLMoniker_ParseDisplayName(IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft,
        LPOLESTR pszDisplayName, ULONG *pchEaten, IMoniker **ppmkOut)
{
    URLMoniker *This = MONIKER_THIS(iface);
    FIXME("(%p)->(%p,%p,%p,%p,%p): stub\n",This,pbc,pmkToLeft,pszDisplayName,pchEaten,ppmkOut);
    return E_NOTIMPL;
}

static HRESULT WINAPI URLMoniker_IsSystemMoniker(IMoniker *iface, DWORD *pwdMksys)
{
    URLMoniker *This = MONIKER_THIS(iface);

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

static URLMoniker *alloc_moniker(void)
{
    URLMoniker *ret;

    ret = heap_alloc(sizeof(URLMoniker));
    if(!ret)
        return NULL;

    ret->lpIMonikerVtbl = &URLMonikerVtbl;
    ret->ref = 1;
    ret->URLName = NULL;

    return ret;
}

static HRESULT URLMoniker_Init(URLMoniker *This, LPCOLESTR lpszLeftURLName, LPCOLESTR lpszURLName)
{
    HRESULT hres;
    DWORD sizeStr = 0;

    TRACE("(%p,%s,%s)\n",This,debugstr_w(lpszLeftURLName),debugstr_w(lpszURLName));

    This->URLName = heap_alloc(INTERNET_MAX_URL_LENGTH*sizeof(WCHAR));

    if(lpszLeftURLName)
        hres = CoInternetCombineUrl(lpszLeftURLName, lpszURLName, URL_FILE_USE_PATHURL,
                This->URLName, INTERNET_MAX_URL_LENGTH, &sizeStr, 0);
    else
        hres = CoInternetParseUrl(lpszURLName, PARSE_CANONICALIZE, URL_FILE_USE_PATHURL,
                This->URLName, INTERNET_MAX_URL_LENGTH, &sizeStr, 0);

    if(FAILED(hres)) {
        heap_free(This->URLName);
        return hres;
    }

    URLMON_LockModule();

    if(sizeStr != INTERNET_MAX_URL_LENGTH)
        This->URLName = heap_realloc(This->URLName, (sizeStr+1)*sizeof(WCHAR));

    TRACE("URLName = %s\n", debugstr_w(This->URLName));

    return S_OK;
}

HRESULT StdURLMoniker_Construct(IUnknown *outer, void **ppv)
{
    TRACE("(%p %p)\n", outer, ppv);

    *ppv = alloc_moniker();
    return *ppv ? S_OK : E_OUTOFMEMORY;
}

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
    URLMoniker *obj;
    HRESULT hres;
    LPOLESTR lefturl = NULL;

    TRACE("(%p, %s, %p, %08x)\n", pmkContext, debugstr_w(szURL), ppmk, dwFlags);

    if (dwFlags & URL_MK_UNIFORM) FIXME("ignoring flag URL_MK_UNIFORM\n");

    if(!(obj = alloc_moniker()))
	return E_OUTOFMEMORY;

    if(pmkContext) {
        IBindCtx* bind;
        DWORD dwMksys = 0;
        IMoniker_IsSystemMoniker(pmkContext, &dwMksys);
        if(dwMksys == MKSYS_URLMONIKER && SUCCEEDED(CreateBindCtx(0, &bind))) {
            IMoniker_GetDisplayName(pmkContext, bind, NULL, &lefturl);
            TRACE("lefturl = %s\n", debugstr_w(lefturl));
            IBindCtx_Release(bind);
        }
    }
        
    hres = URLMoniker_Init(obj, lefturl, szURL);
    CoTaskMemFree(lefturl);
    if(SUCCEEDED(hres))
	hres = URLMoniker_QueryInterface((IMoniker*)obj, &IID_IMoniker, (void**)ppmk);
    IMoniker_Release((IMoniker*)obj);
    return hres;
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

    TRACE("(%p %08x %p %s %p)\n", pmk, grfOpt, pbsc, debugstr_guid(iidResult), ppvResult);

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

    if(is_registered_protocol(szDisplayName)) {
        HRESULT hres;

        hres = CreateURLMoniker(NULL, szDisplayName, ppmk);
        if(SUCCEEDED(hres)) {
            *pchEaten = strlenW(szDisplayName);
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

    TRACE("(%p %s %p %d %d %p)\n", lpUnkCaller, debugstr_a(szURL), szFileName,
            dwBufLength, dwReserved, pBSC);

    if(szURL) {
        len = MultiByteToWideChar(CP_ACP, 0, szURL, -1, NULL, 0);
        url = heap_alloc(len*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, szURL, -1, url, len);
    }

    if(szFileName)
        file_name = heap_alloc(dwBufLength*sizeof(WCHAR));

    hres = URLDownloadToCacheFileW(lpUnkCaller, url, file_name, dwBufLength*sizeof(WCHAR),
            dwReserved, pBSC);

    if(SUCCEEDED(hres) && file_name)
        WideCharToMultiByte(CP_ACP, 0, file_name, -1, szFileName, dwBufLength, NULL, NULL);

    heap_free(url);
    heap_free(file_name);

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

    static WCHAR header[] = {
        'H','T','T','P','/','1','.','0',' ','2','0','0',' ',
        'O','K','\\','r','\\','n','\\','r','\\','n',0
    };

    TRACE("(%p, %s, %p, %d, %d, %p)\n", lpUnkCaller, debugstr_w(szURL),
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

    if (strlenW(cache_path) > dwBufLength)
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
    FIXME("stub\n");
    return E_NOTIMPL;
}

/***********************************************************************
 *           HlinkSimpleNavigateToString (URLMON.@)
 */
HRESULT WINAPI HlinkSimpleNavigateToString( LPCWSTR szTarget,
    LPCWSTR szLocation, LPCWSTR szTargetFrameName, IUnknown *pUnk,
    IBindCtx *pbc, IBindStatusCallback *pbsc, DWORD grfHLNF, DWORD dwReserved)
{
    FIXME("%s\n", debugstr_w( szTarget ) );
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
