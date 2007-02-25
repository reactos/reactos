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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

HRESULT get_protocol_iface(LPCWSTR url, IUnknown **ret)
{
    WCHAR schema[64], str_clsid[64];
    HKEY hkey = NULL;
    DWORD res, type, size, schema_len;
    CLSID clsid;
    LPWSTR wszKey;
    HRESULT hres;

    static const WCHAR wszProtocolsKey[] =
        {'P','R','O','T','O','C','O','L','S','\\','H','a','n','d','l','e','r','\\'};
    static const WCHAR wszCLSID[] = {'C','L','S','I','D',0};

    hres = CoInternetParseUrl(url, PARSE_SCHEMA, 0, schema, sizeof(schema)/sizeof(schema[0]),
            &schema_len, 0);
    if(FAILED(hres) || !schema_len)
        return E_FAIL;

    wszKey = HeapAlloc(GetProcessHeap(), 0, sizeof(wszProtocolsKey)+(schema_len+1)*sizeof(WCHAR));
    memcpy(wszKey, wszProtocolsKey, sizeof(wszProtocolsKey));
    memcpy(wszKey + sizeof(wszProtocolsKey)/sizeof(WCHAR), schema, (schema_len+1)*sizeof(WCHAR));

    res = RegOpenKeyW(HKEY_CLASSES_ROOT, wszKey, &hkey);
    HeapFree(GetProcessHeap(), 0, wszKey);
    if(res != ERROR_SUCCESS) {
        TRACE("Could not open key %s\n", debugstr_w(wszKey));
        return E_FAIL;
    }
    
    size = sizeof(str_clsid);
    res = RegQueryValueExW(hkey, wszCLSID, NULL, &type, (LPBYTE)str_clsid, &size);
    RegCloseKey(hkey);
    if(res != ERROR_SUCCESS || type != REG_SZ) {
        WARN("Could not get protocol CLSID res=%ld\n", res);
        return E_FAIL;
    }

    hres = CLSIDFromString(str_clsid, &clsid);
    if(FAILED(hres)) {
        WARN("CLSIDFromString failed: %08lx\n", hres);
        return hres;
    }

    return CoGetClassObject(&clsid, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void**)ret);
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
    FIXME("(%p %s %s %ld %p %ld)\n", pCF, debugstr_guid(rclsid), debugstr_w(pwzProtocol),
            cPatterns, ppwzPatterns, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetSession_UnregisterNameSpace(IInternetSession *iface,
        IClassFactory *pCF, LPCWSTR pszProtocol)
{
    FIXME("(%p %s)\n", pCF, debugstr_w(pszProtocol));
    return E_NOTIMPL;
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
    FIXME("(%p %s %p %p %p %08lx)\n", pBC, debugstr_w(szUrl), pUnkOuter, ppUnk,
            ppOInetProt, dwOption);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetSession_SetSessionOption(IInternetSession *iface,
        DWORD dwOption, LPVOID pBuffer, DWORD dwBufferLength, DWORD dwReserved)
{
    FIXME("(%08lx %p %ld %ld)\n", dwOption, pBuffer, dwBufferLength, dwReserved);
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
    TRACE("(%ld %p %ld)\n", dwSessionMode, ppIInternetSession, dwReserved);

    if(dwSessionMode)
        ERR("dwSessionMode=%ld\n", dwSessionMode);
    if(dwReserved)
        ERR("dwReserved=%ld\n", dwReserved);

    *ppIInternetSession = &InternetSession;
    return S_OK;
}
