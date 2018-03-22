/* DirectInput 8
 *
 * Copyright 2002 TransGaming Technologies Inc.
 * Copyright 2006 Roderick Colenbrander
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

#include "config.h"
#include <assert.h>
#include <stdarg.h>
#include <string.h>

#define COBJMACROS

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "dinput.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

static HINSTANCE instance;
static LONG dll_count;

/*
 * Dll lifetime tracking declaration
 */
static void LockModule(void)
{
    InterlockedIncrement(&dll_count);
}

static void UnlockModule(void)
{
    InterlockedDecrement(&dll_count);
}

/******************************************************************************
 *	DirectInput8Create (DINPUT8.@)
 */
HRESULT WINAPI DECLSPEC_HOTPATCH DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riid, LPVOID *ppDI, LPUNKNOWN punkOuter) {
    IDirectInputA *pDI;
    HRESULT hr, hrCo;

    TRACE("hInst (%p), dwVersion: %d, riid (%s), punkOuter (%p)\n", hinst, dwVersion, debugstr_guid(riid), punkOuter);

    if (!ppDI)
        return E_POINTER;

    if (!IsEqualGUID(&IID_IDirectInput8A, riid) &&
        !IsEqualGUID(&IID_IDirectInput8W, riid) &&
        !IsEqualGUID(&IID_IUnknown, riid))
    {
        *ppDI = NULL;
        return DIERR_NOINTERFACE;
    }

    hrCo = CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_DirectInput, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectInputA, (void **)&pDI);

    /* Ensure balance of calls. */
    if (SUCCEEDED(hrCo))
        CoUninitialize();

    if (FAILED(hr)) {
        ERR("CoCreateInstance failed with hr = 0x%08x\n", hr);
        return hr;
    }

    hr = IDirectInput_QueryInterface(pDI, riid, ppDI);
    IDirectInput_Release(pDI);

    if (FAILED(hr))
        return hr;

    /* When aggregation is used (punkOuter!=NULL) the application needs to manually call Initialize. */
    if(punkOuter == NULL && IsEqualGUID(&IID_IDirectInput8A, riid)) {
        IDirectInput8A *DI = *ppDI;

        hr = IDirectInput8_Initialize(DI, hinst, dwVersion);
        if (FAILED(hr))
        {
            IDirectInput8_Release(DI);
            *ppDI = NULL;
            return hr;
        }
    }

    if(punkOuter == NULL && IsEqualGUID(&IID_IDirectInput8W, riid)) {
        IDirectInput8W *DI = *ppDI;

        hr = IDirectInput8_Initialize(DI, hinst, dwVersion);
        if (FAILED(hr))
        {
            IDirectInput8_Release(DI);
            *ppDI = NULL;
            return hr;
        }
    }

    return S_OK;
}

/*******************************************************************************
 * DirectInput8 ClassFactory
 */
typedef struct
{
    /* IUnknown fields */
    IClassFactory IClassFactory_iface;
} IClassFactoryImpl;

static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

static HRESULT WINAPI DI8CF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    FIXME("%p %s %p\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI DI8CF_AddRef(LPCLASSFACTORY iface) {
    LockModule();
    return 2;
}

static ULONG WINAPI DI8CF_Release(LPCLASSFACTORY iface) {
    UnlockModule();
    return 1;
}

static HRESULT WINAPI DI8CF_CreateInstance(LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj) {
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);

    TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);
    if( IsEqualGUID( &IID_IDirectInput8A, riid ) || IsEqualGUID( &IID_IDirectInput8W, riid ) || IsEqualGUID( &IID_IUnknown, riid )) {
        IDirectInputA *ppDI;
        HRESULT hr;

        hr = CoCreateInstance(&CLSID_DirectInput, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectInputA, (void **)&ppDI);
        if (FAILED(hr))
            return hr;

        hr = IDirectInput_QueryInterface(ppDI, riid, ppobj);
        IDirectInput_Release(ppDI);

        return hr;
    }

    ERR("(%p,%p,%s,%p) Interface not found!\n",This,pOuter,debugstr_guid(riid),ppobj);    
    return E_NOINTERFACE;
}

static HRESULT WINAPI DI8CF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
    TRACE("(%p)->(%d)\n", iface, dolock);

    if(dolock)
        LockModule();
    else
        UnlockModule();

    return S_OK;
}

static const IClassFactoryVtbl DI8CF_Vtbl = {
    DI8CF_QueryInterface,
    DI8CF_AddRef,
    DI8CF_Release,
    DI8CF_CreateInstance,
    DI8CF_LockServer
};
static IClassFactoryImpl DINPUT8_CF = { { &DI8CF_Vtbl } };


/***********************************************************************
 *		DllCanUnloadNow (DINPUT8.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return dll_count == 0 ? S_OK : S_FALSE;
}

/***********************************************************************
 *		DllGetClassObject (DINPUT8.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("(%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    if ( IsEqualCLSID( &IID_IClassFactory, riid ) ) {
        *ppv = &DINPUT8_CF;
        IClassFactory_AddRef((IClassFactory*)*ppv);
        return S_OK;
    }

    FIXME("(%s,%s,%p): no interface found.\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *		DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD reason, LPVOID lpv)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        instance = hInstDLL;
        DisableThreadLibraryCalls( hInstDLL );
        break;
    }
    return TRUE;
}

/***********************************************************************
 *		DllRegisterServer (DINPUT8.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( instance );
}

/***********************************************************************
 *		DllUnregisterServer (DINPUT8.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( instance );
}
