/*
 * UrlMon
 *
 * Copyright (c) 2000 Patrik Stridvall
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
#include "winerror.h"
#include "wtypes.h"
#define NO_SHLWAPI_REG
#include "shlwapi.h"

#include "wine/debug.h"

#include "winuser.h"
#include "urlmon.h"
#include "urlmon_main.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

HINSTANCE URLMON_hInstance = 0;

/***********************************************************************
 *		DllMain (URLMON.init)
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%lx %p\n", hinstDLL, fdwReason, fImpLoad);

    switch(fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        URLMON_hInstance = hinstDLL;
	break;

    case DLL_PROCESS_DETACH:
        URLMON_hInstance = 0;
	break;
    }
    return TRUE;
}


/***********************************************************************
 *		DllInstall (URLMON.@)
 */
HRESULT WINAPI URLMON_DllInstall(BOOL bInstall, LPCWSTR cmdline)
{
  FIXME("(%s, %s): stub\n", bInstall?"TRUE":"FALSE",
	debugstr_w(cmdline));

  return S_OK;
}

/***********************************************************************
 *		DllCanUnloadNow (URLMON.@)
 */
HRESULT WINAPI URLMON_DllCanUnloadNow(void)
{
    FIXME("(void): stub\n");

    return S_FALSE;
}



/******************************************************************************
 * Urlmon ClassFactory
 */
typedef struct {
    IClassFactory ITF_IClassFactory;

    DWORD ref;
    HRESULT (*pfnCreateInstance)(IUnknown *pUnkOuter, LPVOID *ppObj);
} IClassFactoryImpl;

struct object_creation_info
{
    const CLSID *clsid;
    HRESULT (*pfnCreateInstance)(IUnknown *pUnkOuter, LPVOID *ppObj);
};
 
static const struct object_creation_info object_creation[] =
{
    { &CLSID_InternetSecurityManager, &SecManagerImpl_Construct },
    { &CLSID_InternetZoneManager, ZoneMgrImpl_Construct }
};

static HRESULT WINAPI
CF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
	|| IsEqualGUID(riid, &IID_IClassFactory))
    {
	IClassFactory_AddRef(iface);
	*ppobj = This;
	return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI CF_AddRef(LPCLASSFACTORY iface)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI CF_Release(LPCLASSFACTORY iface)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
	HeapFree(GetProcessHeap(), 0, This);

    return ref;
}


static HRESULT WINAPI CF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter,
                                        REFIID riid, LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    HRESULT hres;
    LPUNKNOWN punk;
    
    TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);

    *ppobj = NULL;
    if(SUCCEEDED(hres = This->pfnCreateInstance(pOuter, (LPVOID *) &punk))) {
        hres = IUnknown_QueryInterface(punk, riid, ppobj);
        IUnknown_Release(punk);
    }
    return hres;
}

static HRESULT WINAPI CF_LockServer(LPCLASSFACTORY iface,BOOL dolock)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    FIXME("(%p)->(%d),stub!\n",This,dolock);
    return S_OK;
}

static IClassFactoryVtbl CF_Vtbl =
{
    CF_QueryInterface,
    CF_AddRef,
    CF_Release,
    CF_CreateInstance,
    CF_LockServer
};

/*******************************************************************************
 * DllGetClassObject [URLMON.@]
 * Retrieves class object from a DLL object
 *
 * NOTES
 *    Docs say returns STDAPI
 *
 * PARAMS
 *    rclsid [I] CLSID for the class object
 *    riid   [I] Reference to identifier of interface for class object
 *    ppv    [O] Address of variable to receive interface pointer for riid
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: CLASS_E_CLASSNOTAVAILABLE, E_OUTOFMEMORY, E_INVALIDARG,
 *             E_UNEXPECTED
 */

DWORD WINAPI URLMON_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    int i;
    IClassFactoryImpl *factory;
    
    TRACE("(%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    
    if ( !IsEqualGUID( &IID_IClassFactory, riid )
	 && ! IsEqualGUID( &IID_IUnknown, riid) )
	return E_NOINTERFACE;

    for (i=0; i < sizeof(object_creation)/sizeof(object_creation[0]); i++)
    {
	if (IsEqualGUID(object_creation[i].clsid, rclsid))
	    break;
    }

    if (i == sizeof(object_creation)/sizeof(object_creation[0]))
    {
	FIXME("%s: no class found.\n", debugstr_guid(rclsid));
	return CLASS_E_CLASSNOTAVAILABLE;
    }

    factory = HeapAlloc(GetProcessHeap(), 0, sizeof(*factory));
    if (factory == NULL) return E_OUTOFMEMORY;

    factory->ITF_IClassFactory.lpVtbl = &CF_Vtbl;
    factory->ref = 1;

    factory->pfnCreateInstance = object_creation[i].pfnCreateInstance;

    *ppv = &(factory->ITF_IClassFactory);
    return S_OK;
}


/***********************************************************************
 *		DllRegisterServerEx (URLMON.@)
 */
HRESULT WINAPI URLMON_DllRegisterServerEx(void)
{
    FIXME("(void): stub\n");

    return E_FAIL;
}

/**************************************************************************
 *                 UrlMkSetSessionOption (URLMON.@)
 */
HRESULT WINAPI UrlMkSetSessionOption(DWORD dwOption, LPVOID *pBuffer, DWORD dwBufferLength,
 					DWORD Reserved)
{
    FIXME("(%#lx, %p, %#lx): stub\n", dwOption, pBuffer, dwBufferLength);

    return S_OK;
}

/**************************************************************************
 *                 UrlMkGetSessionOption (URLMON.@)
 */
HRESULT WINAPI UrlMkGetSessionOption(DWORD dwOption, LPVOID *pBuffer, DWORD dwBufferLength,
                                        DWORD* pdwBufferLength, DWORD dwReserved)
{
    FIXME("(%#lx, %p, %#lx, %p): stub\n", dwOption, pBuffer, dwBufferLength, pdwBufferLength);

    return S_OK;
}

/**************************************************************************
 *                 ObtainUserAgentString (URLMON.@)
 */
HRESULT WINAPI ObtainUserAgentString(DWORD dwOption, LPCSTR pcszUAOut, DWORD *cbSize)
{
    FIXME("(%ld, %p, %p): stub\n", dwOption, pcszUAOut, cbSize);

    if(dwOption) {
      ERR("dwOption: %ld, must be zero\n", dwOption);
    }

    return S_OK;
}

HRESULT WINAPI CoInternetCombineUrl(LPCWSTR pwzBaseUrl, LPCWSTR pwzRelativeUrl, DWORD dwCombineFlags,
                                    LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved)
{
    HRESULT hres;
    DWORD size = cchResult;
    
    TRACE("(%s,%s,0x%08lx,%p,%ld,%p,%ld)\n", debugstr_w(pwzBaseUrl), debugstr_w(pwzRelativeUrl), dwCombineFlags,
          pwzResult, cchResult, pcchResult, dwReserved);
    hres = UrlCombineW(pwzBaseUrl, pwzRelativeUrl, pwzResult, &size, dwCombineFlags);
    if(pcchResult) *pcchResult = size;
    return hres;
}

HRESULT WINAPI CoInternetCompareUrl(LPCWSTR pwzUrl1, LPCWSTR pwzUrl2, DWORD dwCompareFlags)
{
    TRACE("(%s,%s,%08lx)\n", debugstr_w(pwzUrl1), debugstr_w(pwzUrl2), dwCompareFlags);
    return UrlCompareW(pwzUrl1, pwzUrl2, dwCompareFlags)==0?S_OK:S_FALSE;
}

/**************************************************************************
 *                 IsValidURL (URLMON.@)
 * 
 * Determines if a specified string is a valid URL.
 *
 * PARAMS
 *  pBC        [I] ignored, must be NULL.
 *  szURL      [I] string that represents the URL in question.
 *  dwReserved [I] reserved and must be zero.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: S_FALSE.
 *  returns E_INVALIDARG if one or more of the args is invalid.
 *
 * TODO:
 *  test functionality against windows to see what a valid URL is.
 */
HRESULT WINAPI IsValidURL(LPBC pBC, LPCWSTR szURL, DWORD dwReserved)
{
    FIXME("(%p, %s, %ld): stub\n", pBC, debugstr_w(szURL), dwReserved);
    
    if (pBC != NULL || dwReserved != 0)
        return E_INVALIDARG;
    
    return S_OK;
}
