/*
 * Copyright 2005-2006 Jacek Caban for CodeWeavers
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"
#include "urlmon.h"
#include "urlmon_main.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

typedef struct name_space {
    LPWSTR protocol;
    IClassFactory *cf;
    CLSID clsid;

    struct name_space *next;
} name_space;

static name_space *name_space_list = NULL;

static name_space *find_name_space(LPCWSTR protocol)
{
    name_space *iter;

    for(iter = name_space_list; iter; iter = iter->next) {
        if(!strcmpW(iter->protocol, protocol))
            return iter;
    }

    return NULL;
}

static HRESULT get_protocol_cf(LPCWSTR schema, DWORD schema_len, CLSID *pclsid, IClassFactory **ret)
{
    WCHAR str_clsid[64];
    HKEY hkey = NULL;
    DWORD res, type, size;
    CLSID clsid;
    LPWSTR wszKey;
    HRESULT hres;

    static const WCHAR wszProtocolsKey[] =
        {'P','R','O','T','O','C','O','L','S','\\','H','a','n','d','l','e','r','\\'};
    static const WCHAR wszCLSID[] = {'C','L','S','I','D',0};

    wszKey = urlmon_alloc(sizeof(wszProtocolsKey)+(schema_len+1)*sizeof(WCHAR));
    memcpy(wszKey, wszProtocolsKey, sizeof(wszProtocolsKey));
    memcpy(wszKey + sizeof(wszProtocolsKey)/sizeof(WCHAR), schema, (schema_len+1)*sizeof(WCHAR));

    res = RegOpenKeyW(HKEY_CLASSES_ROOT, wszKey, &hkey);
    urlmon_free(wszKey);
    if(res != ERROR_SUCCESS) {
        TRACE("Could not open protocol handler key\n");
        return E_FAIL;
    }
    
    size = sizeof(str_clsid);
    res = RegQueryValueExW(hkey, wszCLSID, NULL, &type, (LPBYTE)str_clsid, &size);
    RegCloseKey(hkey);
    if(res != ERROR_SUCCESS || type != REG_SZ) {
        WARN("Could not get protocol CLSID res=%d\n", res);
        return E_FAIL;
    }

    hres = CLSIDFromString(str_clsid, &clsid);
    if(FAILED(hres)) {
        WARN("CLSIDFromString failed: %08x\n", hres);
        return hres;
    }

    if(pclsid)
        *pclsid = clsid;

    return CoGetClassObject(&clsid, CLSCTX_INPROC_SERVER, NULL, &IID_IClassFactory, (void**)ret);
}

IInternetProtocolInfo *get_protocol_info(LPCWSTR url)
{
    IInternetProtocolInfo *ret = NULL;
    IClassFactory *cf;
    name_space *ns;
    WCHAR schema[64];
    DWORD schema_len;
    HRESULT hres;

    hres = CoInternetParseUrl(url, PARSE_SCHEMA, 0, schema, sizeof(schema)/sizeof(schema[0]),
            &schema_len, 0);
    if(FAILED(hres) || !schema_len)
        return NULL;

    ns = find_name_space(schema);
    if(ns) {
        hres = IClassFactory_QueryInterface(ns->cf, &IID_IInternetProtocolInfo, (void**)&ret);
        if(SUCCEEDED(hres))
            return ret;

        hres = IClassFactory_CreateInstance(ns->cf, NULL, &IID_IInternetProtocolInfo, (void**)&ret);
        if(SUCCEEDED(hres))
            return ret;
    }

    hres = get_protocol_cf(schema, schema_len, NULL, &cf);
    if(FAILED(hres))
        return NULL;

    hres = IClassFactory_QueryInterface(cf, &IID_IInternetProtocolInfo, (void**)&ret);
    if(FAILED(hres))
        IClassFactory_CreateInstance(cf, NULL, &IID_IInternetProtocolInfo, (void**)&ret);
    IClassFactory_Release(cf);

    return ret;
}

HRESULT get_protocol_handler(LPCWSTR url, CLSID *clsid, IClassFactory **ret)
{
    name_space *ns;
    WCHAR schema[64];
    DWORD schema_len;
    HRESULT hres;

    hres = CoInternetParseUrl(url, PARSE_SCHEMA, 0, schema, sizeof(schema)/sizeof(schema[0]),
            &schema_len, 0);
    if(FAILED(hres) || !schema_len)
        return schema_len ? hres : E_FAIL;

    ns = find_name_space(schema);
    if(ns) {
        *ret = ns->cf;
        IClassFactory_AddRef(*ret);
        if(clsid)
            *clsid = ns->clsid;
        return S_OK;
    }

    return get_protocol_cf(schema, schema_len, clsid, ret);
}

static HRESULT WINAPI InternetSession_QueryInterface(IInternetSession *iface,
        REFIID riid, void **ppv)
{
    TRACE("(%s %p)\n", debugstr_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IInternetSession, riid)) {
        *ppv = iface;
        IInternetSession_AddRef(iface);
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI InternetSession_AddRef(IInternetSession *iface)
{
    TRACE("()\n");
    URLMON_LockModule();
    return 2;
}

static ULONG WINAPI InternetSession_Release(IInternetSession *iface)
{
    TRACE("()\n");
    URLMON_UnlockModule();
    return 1;
}

static HRESULT WINAPI InternetSession_RegisterNameSpace(IInternetSession *iface,
        IClassFactory *pCF, REFCLSID rclsid, LPCWSTR pwzProtocol, ULONG cPatterns,
        const LPCWSTR *ppwzPatterns, DWORD dwReserved)
{
    name_space *new_name_space;
    int size;

    TRACE("(%p %s %s %d %p %d)\n", pCF, debugstr_guid(rclsid), debugstr_w(pwzProtocol),
          cPatterns, ppwzPatterns, dwReserved);

    if(cPatterns || ppwzPatterns)
        FIXME("patterns not supported\n");
    if(dwReserved)
        WARN("dwReserved = %d\n", dwReserved);

    if(!pCF || !pwzProtocol)
        return E_INVALIDARG;

    new_name_space = urlmon_alloc(sizeof(name_space));

    size = (strlenW(pwzProtocol)+1)*sizeof(WCHAR);
    new_name_space->protocol = urlmon_alloc(size);
    memcpy(new_name_space->protocol, pwzProtocol, size);

    IClassFactory_AddRef(pCF);
    new_name_space->cf = pCF;
    new_name_space->clsid = *rclsid;

    new_name_space->next = name_space_list;
    name_space_list = new_name_space;
    return S_OK;
}

static HRESULT WINAPI InternetSession_UnregisterNameSpace(IInternetSession *iface,
        IClassFactory *pCF, LPCWSTR pszProtocol)
{
    name_space *iter, *last = NULL;

    TRACE("(%p %s)\n", pCF, debugstr_w(pszProtocol));

    if(!pCF || !pszProtocol)
        return E_INVALIDARG;

    for(iter = name_space_list; iter; iter = iter->next) {
        if(iter->cf == pCF && !strcmpW(iter->protocol, pszProtocol))
            break;
        last = iter;
    }

    if(!iter)
        return S_OK;

    if(last)
        last->next = iter->next;
    else
        name_space_list = iter->next;

    IClassFactory_Release(iter->cf);
    urlmon_free(iter->protocol);
    urlmon_free(iter);

    return S_OK;
}

static HRESULT WINAPI InternetSession_RegisterMimeFilter(IInternetSession *iface,
        IClassFactory *pCF, REFCLSID rclsid, LPCWSTR pwzType)
{
    FIXME("(%p %s %s)\n", pCF, debugstr_guid(rclsid), debugstr_w(pwzType));
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetSession_UnregisterMimeFilter(IInternetSession *iface,
        IClassFactory *pCF, LPCWSTR pwzType)
{
    FIXME("(%p %s)\n", pCF, debugstr_w(pwzType));
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetSession_CreateBinding(IInternetSession *iface,
        LPBC pBC, LPCWSTR szUrl, IUnknown *pUnkOuter, IUnknown **ppUnk,
        IInternetProtocol **ppOInetProt, DWORD dwOption)
{
    TRACE("(%p %s %p %p %p %08x)\n", pBC, debugstr_w(szUrl), pUnkOuter, ppUnk,
            ppOInetProt, dwOption);

    if(pBC || pUnkOuter || ppUnk || dwOption)
        FIXME("Unsupported arguments\n");

    return create_binding_protocol(szUrl, ppOInetProt);
}

static HRESULT WINAPI InternetSession_SetSessionOption(IInternetSession *iface,
        DWORD dwOption, LPVOID pBuffer, DWORD dwBufferLength, DWORD dwReserved)
{
    FIXME("(%08x %p %d %d)\n", dwOption, pBuffer, dwBufferLength, dwReserved);
    return E_NOTIMPL;
}

static const IInternetSessionVtbl InternetSessionVtbl = {
    InternetSession_QueryInterface,
    InternetSession_AddRef,
    InternetSession_Release,
    InternetSession_RegisterNameSpace,
    InternetSession_UnregisterNameSpace,
    InternetSession_RegisterMimeFilter,
    InternetSession_UnregisterMimeFilter,
    InternetSession_CreateBinding,
    InternetSession_SetSessionOption
};

static IInternetSession InternetSession = { &InternetSessionVtbl };

/***********************************************************************
 *           CoInternetGetSession (URLMON.@)
 *
 * Create a new internet session and return an IInternetSession interface
 * representing it.
 *
 * PARAMS
 *    dwSessionMode      [I] Mode for the internet session
 *    ppIInternetSession [O] Destination for creates IInternetSession object
 *    dwReserved         [I] Reserved, must be 0.
 *
 * RETURNS
 *    Success: S_OK. ppIInternetSession contains the IInternetSession interface.
 *    Failure: E_INVALIDARG, if any argument is invalid, or
 *             E_OUTOFMEMORY if memory allocation fails.
 */
HRESULT WINAPI CoInternetGetSession(DWORD dwSessionMode, IInternetSession **ppIInternetSession,
        DWORD dwReserved)
{
    TRACE("(%d %p %d)\n", dwSessionMode, ppIInternetSession, dwReserved);

    if(dwSessionMode)
        ERR("dwSessionMode=%d\n", dwSessionMode);
    if(dwReserved)
        ERR("dwReserved=%d\n", dwReserved);

    IInternetSession_AddRef(&InternetSession);
    *ppIInternetSession = &InternetSession;
    return S_OK;
}

/**************************************************************************
 *                 UrlMkGetSessionOption (URLMON.@)
 */
static BOOL get_url_encoding(HKEY root, DWORD *encoding)
{
    DWORD size = sizeof(DWORD), res, type;
    HKEY hkey;

    static const WCHAR wszKeyName[] = 
        {'S','O','F','T','W','A','R','E',
         '\\','M','i','c','r','o','s','o','f','t',
         '\\','W','i','n','d','o','w','s',
         '\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n',
         '\\','I','n','t','e','r','n','e','t',' ','S','e','t','t','i','n','g','s',0};
    static const WCHAR wszUrlEncoding[] = {'U','r','l','E','n','c','o','d','i','n','g',0};

    res = RegOpenKeyW(root, wszKeyName, &hkey);
    if(res != ERROR_SUCCESS)
        return FALSE;

    res = RegQueryValueExW(hkey, wszUrlEncoding, NULL, &type, (LPBYTE)encoding, &size);
    RegCloseKey(hkey);

    return res == ERROR_SUCCESS;
}

HRESULT WINAPI UrlMkGetSessionOption(DWORD dwOption, LPVOID pBuffer, DWORD dwBufferLength,
                                     DWORD* pdwBufferLength, DWORD dwReserved)
{
    TRACE("(%x, %p, %d, %p)\n", dwOption, pBuffer, dwBufferLength, pdwBufferLength);

    if(dwReserved)
        WARN("dwReserved = %d\n", dwReserved);

    switch(dwOption) {
    case URLMON_OPTION_URL_ENCODING: {
        DWORD encoding = 0;

        if(!pBuffer || dwBufferLength < sizeof(DWORD) || !pdwBufferLength)
            return E_INVALIDARG;

        if(!get_url_encoding(HKEY_CURRENT_USER, &encoding))
            get_url_encoding(HKEY_LOCAL_MACHINE, &encoding);

        *pdwBufferLength = sizeof(DWORD);
        *(DWORD*)pBuffer = encoding ? URL_ENCODING_DISABLE_UTF8 : URL_ENCODING_ENABLE_UTF8;
        return S_OK;
    }
    default:
        FIXME("unsupported option %x\n", dwOption);
    }

    return E_INVALIDARG;
}
