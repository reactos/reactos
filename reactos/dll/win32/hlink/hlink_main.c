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

#include <winreg.h>
#include <rpcproxy.h>

static HINSTANCE instance;

typedef HRESULT (*LPFNCREATEINSTANCE)(IUnknown*, REFIID, LPVOID*);

typedef struct
{
    IClassFactory      IClassFactory_iface;
    LPFNCREATEINSTANCE lpfnCI;
} CFImpl;

static inline CFImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, CFImpl, IClassFactory_iface);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p %d %p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        instance = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);
        break;
    }
    return TRUE;
}

/***********************************************************************
 *             DllCanUnloadNow (HLINK.@)
 */
HRESULT WINAPI DllCanUnloadNow( void )
{
    return S_FALSE;
}

/***********************************************************************
 *             HlinkCreateFromMoniker (HLINK.@)
 */
HRESULT WINAPI HlinkCreateFromMoniker( IMoniker *pimkTrgt, LPCWSTR pwzLocation,
        LPCWSTR pwzFriendlyName, IHlinkSite* pihlsite, DWORD dwSiteData,
        IUnknown* piunkOuter, REFIID riid, void** ppvObj)
{
    IHlink *hl = NULL;
    HRESULT r;

    TRACE("%p %s %s %p %i %p %s %p\n", pimkTrgt, debugstr_w(pwzLocation),
            debugstr_w(pwzFriendlyName), pihlsite, dwSiteData, piunkOuter,
            debugstr_guid(riid), ppvObj);

    r = CoCreateInstance(&CLSID_StdHlink, piunkOuter, CLSCTX_INPROC_SERVER, riid, (LPVOID*)&hl);
    if (FAILED(r))
        return r;

    IHlink_SetMonikerReference(hl, HLINKSETF_LOCATION | HLINKSETF_TARGET, pimkTrgt, pwzLocation);

    if (pwzFriendlyName)
        IHlink_SetFriendlyName(hl, pwzFriendlyName);
    if (pihlsite)
        IHlink_SetHlinkSite(hl, pihlsite, dwSiteData);

    *ppvObj = hl;

    TRACE("Returning %i\n",r);

    return r;
}

/***********************************************************************
 *             HlinkCreateFromString (HLINK.@)
 */
HRESULT WINAPI HlinkCreateFromString( LPCWSTR pwzTarget, LPCWSTR pwzLocation,
        LPCWSTR pwzFriendlyName, IHlinkSite* pihlsite, DWORD dwSiteData,
        IUnknown* piunkOuter, REFIID riid, void** ppvObj)
{
    IHlink *hl = NULL;
    HRESULT r;
    WCHAR *hash, *tgt;
    const WCHAR *loc;

    TRACE("%s %s %s %p %i %p %s %p\n", debugstr_w(pwzTarget),
            debugstr_w(pwzLocation), debugstr_w(pwzFriendlyName), pihlsite,
            dwSiteData, piunkOuter, debugstr_guid(riid), ppvObj);

    r = CoCreateInstance(&CLSID_StdHlink, piunkOuter, CLSCTX_INPROC_SERVER, riid, (LPVOID*)&hl);
    if (FAILED(r))
        return r;

    if (pwzTarget)
    {
        hash = strchrW(pwzTarget, '#');
        if (hash)
        {
            if (hash == pwzTarget)
                tgt = NULL;
            else
            {
                int tgt_len = hash - pwzTarget;
                tgt = heap_alloc((tgt_len + 1) * sizeof(WCHAR));
                if (!tgt)
                    return E_OUTOFMEMORY;
                memcpy(tgt, pwzTarget, tgt_len * sizeof(WCHAR));
                tgt[tgt_len] = 0;
            }
            if (!pwzLocation)
                loc = hash + 1;
            else
                loc = pwzLocation;
        }
        else
        {
            tgt = hlink_strdupW(pwzTarget);
            if (!tgt)
                return E_OUTOFMEMORY;
            loc = pwzLocation;
        }
    }
    else
    {
        tgt = NULL;
        loc = pwzLocation;
    }

    IHlink_SetStringReference(hl, HLINKSETF_TARGET | HLINKSETF_LOCATION, tgt, loc);

    heap_free(tgt);

    if (pwzFriendlyName)
        IHlink_SetFriendlyName(hl, pwzFriendlyName);

    if (pihlsite)
        IHlink_SetHlinkSite(hl, pihlsite, dwSiteData);

    TRACE("Returning %i\n",r);
    *ppvObj = hl;

    return r;
}


/***********************************************************************
 *             HlinkCreateBrowseContext (HLINK.@)
 */
HRESULT WINAPI HlinkCreateBrowseContext( IUnknown* piunkOuter, REFIID riid, void** ppvObj)
{
    TRACE("%p %s %p\n", piunkOuter, debugstr_guid(riid), ppvObj);
    return CoCreateInstance(&CLSID_StdHlinkBrowseContext, piunkOuter, CLSCTX_INPROC_SERVER, riid, ppvObj);
}

/***********************************************************************
 *             HlinkNavigate (HLINK.@)
 */
HRESULT WINAPI HlinkNavigate(IHlink *phl, IHlinkFrame *phlFrame,
        DWORD grfHLNF, LPBC pbc, IBindStatusCallback *pbsc,
        IHlinkBrowseContext *phlbc)
{
    HRESULT r = S_OK;

    TRACE("%p %p %i %p %p %p\n", phl, phlFrame, grfHLNF, pbc, pbsc, phlbc);

    if (phlFrame)
        r = IHlinkFrame_Navigate(phlFrame, grfHLNF, pbc, pbsc, phl);
    else if (phl)
        r = IHlink_Navigate(phl, grfHLNF, pbc, pbsc, phlbc);

    return r;
}

/***********************************************************************
 *             HlinkOnNavigate (HLINK.@)
 */
HRESULT WINAPI HlinkOnNavigate( IHlinkFrame *phlFrame,
        IHlinkBrowseContext* phlbc, DWORD grfHLNF, IMoniker *pmkTarget,
        LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName, ULONG* puHLID)
{
    HRESULT r;

    TRACE("%p %p %i %p %s %s %p\n",phlFrame, phlbc, grfHLNF, pmkTarget,
            debugstr_w(pwzLocation), debugstr_w(pwzFriendlyName), puHLID);

    r = IHlinkBrowseContext_OnNavigateHlink(phlbc, grfHLNF, pmkTarget,
            pwzLocation, pwzFriendlyName, puHLID);

    if (phlFrame)
        r = IHlinkFrame_OnNavigate(phlFrame,grfHLNF,pmkTarget, pwzLocation,
                pwzFriendlyName, 0);

    return r;
}

/***********************************************************************
 *             HlinkCreateFromData (HLINK.@)
 */
HRESULT WINAPI HlinkCreateFromData(IDataObject *piDataObj,
        IHlinkSite *pihlsite, DWORD dwSiteData, IUnknown *piunkOuter,
        REFIID riid, void **ppvObj)
{
    FIXME("%p, %p, %d, %p, %s, %p\n", piDataObj, pihlsite, dwSiteData,
           piunkOuter, debugstr_guid(riid), ppvObj);
    *ppvObj = NULL;
    return E_NOTIMPL;
}

/***********************************************************************
 *             HlinkQueryCreateFromData (HLINK.@)
 */
HRESULT WINAPI HlinkQueryCreateFromData(IDataObject* piDataObj)
{
    FIXME("%p\n", piDataObj);
    return E_NOTIMPL;
}

/***********************************************************************
 *             HlinkNavigateToStringReference (HLINK.@)
 */
HRESULT WINAPI HlinkNavigateToStringReference( LPCWSTR pwzTarget,
        LPCWSTR pwzLocation, IHlinkSite *pihlsite, DWORD dwSiteData,
        IHlinkFrame *pihlframe, DWORD grfHLNF, LPBC pibc,
        IBindStatusCallback *pibsc, IHlinkBrowseContext *pihlbc)
{
    HRESULT r;
    IHlink *hlink = NULL;

    TRACE("%s %s %p %08x %p %08x %p %p %p\n",
          debugstr_w(pwzTarget), debugstr_w(pwzLocation), pihlsite,
          dwSiteData, pihlframe, grfHLNF, pibc, pibsc, pihlbc);

    r = HlinkCreateFromString( pwzTarget, pwzLocation, NULL, pihlsite,
                               dwSiteData, NULL, &IID_IHlink, (LPVOID*) &hlink );
    if (SUCCEEDED(r)) {
        r = HlinkNavigate(hlink, pihlframe, grfHLNF, pibc, pibsc, pihlbc);
        IHlink_Release(hlink);
    }

    return r;
}

/***********************************************************************
 *             HlinkIsShortcut (HLINK.@)
 */
HRESULT WINAPI HlinkIsShortcut(LPCWSTR pwzFileName)
{
    int len;

    static const WCHAR url_ext[] = {'.','u','r','l',0};

    TRACE("(%s)\n", debugstr_w(pwzFileName));

    if(!pwzFileName)
        return E_INVALIDARG;

    len = strlenW(pwzFileName)-4;
    if(len < 0)
        return S_FALSE;

    return strcmpiW(pwzFileName+len, url_ext) ? S_FALSE : S_OK;
}

/***********************************************************************
 *             HlinkGetSpecialReference (HLINK.@)
 */
HRESULT WINAPI HlinkGetSpecialReference(ULONG uReference, LPWSTR *ppwzReference)
{
    DWORD res, type, size = 100;
    LPCWSTR value_name;
    WCHAR *buf;
    HKEY hkey;

    static const WCHAR start_pageW[] = {'S','t','a','r','t',' ','P','a','g','e',0};
    static const WCHAR search_pageW[] = {'S','e','a','r','c','h',' ','P','a','g','e',0};

    static const WCHAR ie_main_keyW[] =
        {'S','o','f','t','w','a','r','e',
         '\\','M','i','c','r','o','s','o','f','t','\\',
         'I','n','t','e','r','n','e','t',' ','E','x','p','l','o','r','e','r',
         '\\','M','a','i','n',0};

    TRACE("(%u %p)\n", uReference, ppwzReference);

    *ppwzReference = NULL;

    switch(uReference) {
    case HLSR_HOME:
        value_name = start_pageW;
        break;
    case HLSR_SEARCHPAGE:
        value_name = search_pageW;
        break;
    case HLSR_HISTORYFOLDER:
        return E_NOTIMPL;
    default:
        return E_INVALIDARG;
    }

    res = RegOpenKeyW(HKEY_CURRENT_USER, ie_main_keyW, &hkey);
    if(res != ERROR_SUCCESS) {
        WARN("Could not open key: %u\n", res);
        return HRESULT_FROM_WIN32(res);
    }

    buf = CoTaskMemAlloc(size);
    res = RegQueryValueExW(hkey, value_name, NULL, &type, (PBYTE)buf, &size);
    buf = CoTaskMemRealloc(buf, size);
    if(res == ERROR_MORE_DATA)
        res = RegQueryValueExW(hkey, value_name, NULL, &type, (PBYTE)buf, &size);
    RegCloseKey(hkey);
    if(res != ERROR_SUCCESS) {
        WARN("Could not query value %s: %u\n", debugstr_w(value_name), res);
        CoTaskMemFree(buf);
        return HRESULT_FROM_WIN32(res);
    }

    *ppwzReference = buf;
    return S_OK;
}

/***********************************************************************
 *             HlinkTranslateURL (HLINK.@)
 */
HRESULT WINAPI HlinkTranslateURL(LPCWSTR pwzURL, DWORD grfFlags, LPWSTR *ppwzTranslatedURL)
{
    FIXME("(%s %08x %p)\n", debugstr_w(pwzURL), grfFlags, ppwzTranslatedURL);
    return E_NOTIMPL;
}

/***********************************************************************
 *             HlinkUpdateStackItem (HLINK.@)
 */
HRESULT WINAPI HlinkUpdateStackItem(IHlinkFrame *frame, IHlinkBrowseContext *bc,
        ULONG hlid, IMoniker *target, LPCWSTR location, LPCWSTR friendly_name)
{
    HRESULT hr;

    TRACE("(%p %p 0x%x %p %s %s)\n", frame, bc, hlid, target, debugstr_w(location), debugstr_w(friendly_name));

    if (!frame && !bc) return E_INVALIDARG;

    if (frame)
        hr = IHlinkFrame_UpdateHlink(frame, hlid, target, location, friendly_name);
    else
        hr = IHlinkBrowseContext_UpdateHlink(bc, hlid, target, location, friendly_name);

    return hr;
}

/***********************************************************************
 *             HlinkParseDisplayName (HLINK.@)
 */
HRESULT WINAPI HlinkParseDisplayName(LPBC pibc, LPCWSTR pwzDisplayName, BOOL fNoForceAbs,
        ULONG *pcchEaten, IMoniker **ppimk)
{
    static const WCHAR file_colonW[] = {'f','i','l','e',':'};
    ULONG eaten = 0;
    HRESULT hres;

    TRACE("(%p %s %x %p %p)\n", pibc, debugstr_w(pwzDisplayName), fNoForceAbs, pcchEaten, ppimk);

    if(fNoForceAbs)
        FIXME("Unsupported fNoForceAbs\n");

    if(!strncmpiW(pwzDisplayName, file_colonW, sizeof(file_colonW)/sizeof(WCHAR))) {
        pwzDisplayName += sizeof(file_colonW)/sizeof(WCHAR);
        eaten += sizeof(file_colonW)/sizeof(WCHAR);

        while(*pwzDisplayName == '/') {
            pwzDisplayName++;
            eaten++;
        }
    }else {
        hres = MkParseDisplayNameEx(pibc, pwzDisplayName, pcchEaten, ppimk);
        if(SUCCEEDED(hres))
            return hres;

        hres = MkParseDisplayName(pibc, pwzDisplayName, pcchEaten, ppimk);
        if(SUCCEEDED(hres))
            return hres;
    }

    hres = CreateFileMoniker(pwzDisplayName, ppimk);
    if(SUCCEEDED(hres))
        *pcchEaten = eaten + strlenW(pwzDisplayName);

    return hres;
}

/***********************************************************************
 *             HlinkResolveMonikerForData (HLINK.@)
 */
HRESULT WINAPI HlinkResolveMonikerForData(LPMONIKER pimkReference, DWORD reserved, LPBC pibc,
        ULONG cFmtetc, FORMATETC *rgFmtetc, IBindStatusCallback *pibsc, LPMONIKER pimkBase)
{
    LPOLESTR name = NULL;
    IBindCtx *bctx;
    DWORD mksys = 0;
    void *obj = NULL;
    HRESULT hres;

    TRACE("(%p %x %p %d %p %p %p)\n", pimkReference, reserved, pibc, cFmtetc, rgFmtetc, pibsc, pimkBase);

    if(cFmtetc || rgFmtetc || pimkBase)
        FIXME("Unsupported args\n");

    hres = RegisterBindStatusCallback(pibc, pibsc, NULL /* FIXME */, 0);
    if(FAILED(hres))
        return hres;

    hres = IMoniker_IsSystemMoniker(pimkReference, &mksys);
    if(SUCCEEDED(hres) && mksys != MKSYS_URLMONIKER)
        WARN("sysmk = %x\n", mksys);

    /* FIXME: What is it for? */
    CreateBindCtx(0, &bctx);
    hres = IMoniker_GetDisplayName(pimkReference, bctx, NULL, &name);
    IBindCtx_Release(bctx);
    if(SUCCEEDED(hres)) {
        TRACE("got display name %s\n", debugstr_w(name));
        CoTaskMemFree(name);
    }

    return IMoniker_BindToStorage(pimkReference, pibc, NULL, &IID_IUnknown, &obj);
}

/***********************************************************************
 *             HlinkClone (HLINK.@)
 */
HRESULT WINAPI HlinkClone(IHlink *hlink, REFIID riid, IHlinkSite *hls,
        DWORD site_data, void **obj)
{
    IMoniker *mk, *clone_mk = NULL;
    WCHAR *loc, *name = NULL;
    HRESULT hres;

    if(!hlink || !riid || !obj)
        return E_INVALIDARG;

    *obj = NULL;

    hres = IHlink_GetMonikerReference(hlink, HLINKGETREF_DEFAULT, &mk, &loc);
    if(FAILED(hres))
        return hres;

    if(mk) {
        IStream *strm;
        LARGE_INTEGER lgint;

        hres = CreateStreamOnHGlobal(NULL, TRUE, &strm);
        if(FAILED(hres)) {
            IMoniker_Release(mk);
            goto cleanup;
        }

        hres = OleSaveToStream((IPersistStream*)mk, strm);
        if(FAILED(hres)) {
            IStream_Release(strm);
            IMoniker_Release(mk);
            goto cleanup;
        }
        IMoniker_Release(mk);

        lgint.QuadPart = 0;
        hres = IStream_Seek(strm, lgint, STREAM_SEEK_SET, NULL);
        if(FAILED(hres)) {
            IStream_Release(strm);
            goto cleanup;
        }

        hres = OleLoadFromStream(strm, &IID_IMoniker, (void**)&clone_mk);
        IStream_Release(strm);
        if(FAILED(hres))
            goto cleanup;
    }

    hres = IHlink_GetFriendlyName(hlink, HLFNAMEF_DEFAULT, &name);
    if(FAILED(hres))
        goto cleanup;

    hres = HlinkCreateFromMoniker(clone_mk, loc, name, hls, site_data, NULL,
            &IID_IHlink, obj);

cleanup:
    if(clone_mk)
        IMoniker_Release(clone_mk);
    CoTaskMemFree(loc);
    CoTaskMemFree(name);
    return hres;
}

static HRESULT WINAPI HLinkCF_fnQueryInterface ( LPCLASSFACTORY iface,
        REFIID riid, LPVOID *ppvObj)
{
    CFImpl *This = impl_from_IClassFactory(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppvObj);

    *ppvObj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IClassFactory))
    {
        *ppvObj = &This->IClassFactory_iface;
        return S_OK;
    }

    TRACE("-- E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI HLinkCF_fnAddRef (LPCLASSFACTORY iface)
{
    return 2;
}

static ULONG WINAPI HLinkCF_fnRelease(LPCLASSFACTORY iface)
{
    return 1;
}

static HRESULT WINAPI HLinkCF_fnCreateInstance( LPCLASSFACTORY iface,
        LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObject)
{
    CFImpl *This = impl_from_IClassFactory(iface);

    TRACE("%p->(%p,%s,%p)\n", This, pUnkOuter, debugstr_guid(riid), ppvObject);

    *ppvObject = NULL;

    return This->lpfnCI(pUnkOuter, riid, ppvObject);
}

static HRESULT WINAPI HLinkCF_fnLockServer(LPCLASSFACTORY iface, BOOL fLock)
{
    FIXME("%p %d\n", iface, fLock);
    return E_NOTIMPL;
}

static const IClassFactoryVtbl hlcfvt =
{
    HLinkCF_fnQueryInterface,
    HLinkCF_fnAddRef,
    HLinkCF_fnRelease,
    HLinkCF_fnCreateInstance,
    HLinkCF_fnLockServer
};

static CFImpl HLink_cf = { { &hlcfvt }, HLink_Constructor };
static CFImpl HLinkBrowseContext_cf = { { &hlcfvt }, HLinkBrowseContext_Constructor };

/***********************************************************************
 *             DllGetClassObject (HLINK.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    IClassFactory   *pcf = NULL;

    TRACE("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);

    if (!ppv)
        return E_INVALIDARG;
    *ppv = NULL;

    if (IsEqualIID(rclsid, &CLSID_StdHlink))
        pcf = &HLink_cf.IClassFactory_iface;
    else if (IsEqualIID(rclsid, &CLSID_StdHlinkBrowseContext))
        pcf = &HLinkBrowseContext_cf.IClassFactory_iface;
    else
        return CLASS_E_CLASSNOTAVAILABLE;

    return IClassFactory_QueryInterface(pcf, iid, ppv);
}

/***********************************************************************
 *		DllRegisterServer (HLINK.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( instance );
}

/***********************************************************************
 *		DllUnregisterServer (HLINK.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( instance );
}
