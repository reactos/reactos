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
#include "dinput.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);
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
HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riid, LPVOID *ppDI, LPUNKNOWN punkOuter) {
    HRESULT hr;

    TRACE("hInst (%p), dwVersion: %d, riid (%s), punkOuter (%p))\n", hinst, dwVersion, debugstr_guid(riid), punkOuter);

    /* The specified version needs to be dinput8 (0x800) or higher */
    if(dwVersion < 0x800)
        return DIERR_OLDDIRECTINPUTVERSION;

    if( !(IsEqualGUID(&IID_IDirectInput8A, riid) || IsEqualGUID(&IID_IDirectInput8W, riid) || IsEqualGUID(&IID_IUnknown, riid)) )
        return DIERR_INVALIDPARAM;

    CoInitialize(NULL);
    
    hr = CoCreateInstance( &CLSID_DirectInput8, punkOuter, CLSCTX_INPROC_SERVER, riid, ppDI);
    if(FAILED(hr)) {
        ERR("CoCreateInstance failed with hr = %d; Try running wineprefixcreate to fix it.\n", hr);
        return DIERR_INVALIDPARAM;
    }

    CoUninitialize();

    /* When aggregation is used (punkOuter!=NULL) the application needs to manually call Initialize. */
    if(punkOuter == NULL && IsEqualGUID(&IID_IDirectInput8A, riid)) {
        LPDIRECTINPUTA DI = *ppDI;
        IDirectInput8_Initialize(DI, hinst, dwVersion);
    }

    if(punkOuter == NULL && IsEqualGUID(&IID_IDirectInput8W, riid)) {
        LPDIRECTINPUTW DI = *ppDI;
        IDirectInput8_Initialize(DI, hinst, dwVersion);
    }

    return S_OK;
}

/*******************************************************************************
 * DirectInput8 ClassFactory
 */
typedef struct
{
    /* IUnknown fields */
    const IClassFactoryVtbl    *lpVtbl;
} IClassFactoryImpl;

static HRESULT WINAPI DI8CF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
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
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

    TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);
    if( IsEqualGUID( &IID_IDirectInput8A, riid ) || IsEqualGUID( &IID_IDirectInput8W, riid ) || IsEqualGUID( &IID_IUnknown, riid )) {
        return DirectInputCreateEx(0, DIRECTINPUT_VERSION, riid, ppobj, pOuter);
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
static IClassFactoryImpl DINPUT8_CF = { &DI8CF_Vtbl };


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
