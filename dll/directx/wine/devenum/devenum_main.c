/*
 * Device Enumeration
 *
 * Copyright (C) 2002 John K. Hohm
 * Copyright (C) 2002 Robert Shearman
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

#include "devenum_private.h"
#include "rpcproxy.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(devenum);

DECLSPEC_HIDDEN LONG dll_refs;
static HINSTANCE devenum_instance;

#ifdef __REACTOS__
static void DEVENUM_RegisterQuartz(void);
#endif

/***********************************************************************
 *		DllEntryPoint
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%x %p\n", hinstDLL, fdwReason, fImpLoad);

    switch(fdwReason) {
    case DLL_PROCESS_ATTACH:
        devenum_instance = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);
	break;
    }
    return TRUE;
}

struct class_factory
{
    IClassFactory IClassFactory_iface;
    IUnknown *obj;
};

static inline struct class_factory *impl_from_IClassFactory( IClassFactory *iface )
{
    return CONTAINING_RECORD( iface, struct class_factory, IClassFactory_iface );
}

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID iid, void **obj)
{
    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(iid), obj);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IClassFactory))
    {
        IClassFactory_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;
    WARN("no interface for %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    DEVENUM_LockModule();
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    DEVENUM_UnlockModule();
    return 1;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface,
    IUnknown *outer, REFIID iid, void **obj)
{
    struct class_factory *This = impl_from_IClassFactory( iface );

    TRACE("(%p, %s, %p)\n", outer, debugstr_guid(iid), obj);

    if (!obj) return E_POINTER;

    if (outer) return CLASS_E_NOAGGREGATION;

    return IUnknown_QueryInterface(This->obj, iid, obj);
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL lock)
{
    if (lock)
        DEVENUM_LockModule();
    else
        DEVENUM_UnlockModule();
    return S_OK;
}

static const IClassFactoryVtbl ClassFactory_vtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static struct class_factory create_devenum_cf = { { &ClassFactory_vtbl }, (IUnknown *)&DEVENUM_CreateDevEnum };
static struct class_factory device_moniker_cf = { { &ClassFactory_vtbl }, (IUnknown *)&DEVENUM_ParseDisplayName };

/***********************************************************************
 *		DllGetClassObject (DEVENUM.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void **obj)
{
    TRACE("(%s, %s, %p)\n", debugstr_guid(clsid), debugstr_guid(iid), obj);

    *obj = NULL;

    if (IsEqualGUID(clsid, &CLSID_SystemDeviceEnum))
        return IClassFactory_QueryInterface(&create_devenum_cf.IClassFactory_iface, iid, obj);
    else if (IsEqualGUID(clsid, &CLSID_CDeviceMoniker))
        return IClassFactory_QueryInterface(&device_moniker_cf.IClassFactory_iface, iid, obj);

    FIXME("class %s not available\n", debugstr_guid(clsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *		DllCanUnloadNow (DEVENUM.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return dll_refs != 0 ? S_FALSE : S_OK;
}

/***********************************************************************
 *		DllRegisterServer (DEVENUM.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    HRESULT res;
    IFilterMapper2 * pMapper = NULL;
    LPVOID mapvptr;

    TRACE("\n");

    res = __wine_register_resources( devenum_instance );
    if (FAILED(res))
        return res;

#ifdef __REACTOS__
    /* Quartz is needed for IFilterMapper2 */
    DEVENUM_RegisterQuartz();
#endif

/*** ActiveMovieFilter Categories ***/

    CoInitialize(NULL);
    
    res = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC,
                           &IID_IFilterMapper2,  &mapvptr);
    if (SUCCEEDED(res))
    {
        static const WCHAR friendlyvidcap[] = {'V','i','d','e','o',' ','C','a','p','t','u','r','e',' ','S','o','u','r','c','e','s',0};
        static const WCHAR friendlydshow[] = {'D','i','r','e','c','t','S','h','o','w',' ','F','i','l','t','e','r','s',0};
        static const WCHAR friendlyvidcomp[] = {'V','i','d','e','o',' ','C','o','m','p','r','e','s','s','o','r','s',0};
        static const WCHAR friendlyaudcap[] = {'A','u','d','i','o',' ','C','a','p','t','u','r','e',' ','S','o','u','r','c','e','s',0};
        static const WCHAR friendlyaudcomp[] = {'A','u','d','i','o',' ','C','o','m','p','r','e','s','s','o','r','s',0};
        static const WCHAR friendlyaudrend[] = {'A','u','d','i','o',' ','R','e','n','d','e','r','e','r','s',0};
        static const WCHAR friendlymidirend[] = {'M','i','d','i',' ','R','e','n','d','e','r','e','r','s',0};
        static const WCHAR friendlyextrend[] = {'E','x','t','e','r','n','a','l',' ','R','e','n','d','e','r','e','r','s',0};
        static const WCHAR friendlydevctrl[] = {'D','e','v','i','c','e',' ','C','o','n','t','r','o','l',' ','F','i','l','t','e','r','s',0};

        pMapper = mapvptr;

        IFilterMapper2_CreateCategory(pMapper, &CLSID_VideoInputDeviceCategory, MERIT_DO_NOT_USE, friendlyvidcap);
        IFilterMapper2_CreateCategory(pMapper, &CLSID_LegacyAmFilterCategory, MERIT_NORMAL, friendlydshow);
        IFilterMapper2_CreateCategory(pMapper, &CLSID_VideoCompressorCategory, MERIT_DO_NOT_USE, friendlyvidcomp);
        IFilterMapper2_CreateCategory(pMapper, &CLSID_AudioInputDeviceCategory, MERIT_DO_NOT_USE, friendlyaudcap);
        IFilterMapper2_CreateCategory(pMapper, &CLSID_AudioCompressorCategory, MERIT_DO_NOT_USE, friendlyaudcomp);
        IFilterMapper2_CreateCategory(pMapper, &CLSID_AudioRendererCategory, MERIT_NORMAL, friendlyaudrend);
        IFilterMapper2_CreateCategory(pMapper, &CLSID_MidiRendererCategory, MERIT_NORMAL, friendlymidirend);
        IFilterMapper2_CreateCategory(pMapper, &CLSID_TransmitCategory, MERIT_DO_NOT_USE, friendlyextrend);
        IFilterMapper2_CreateCategory(pMapper, &CLSID_DeviceControlCategory, MERIT_DO_NOT_USE, friendlydevctrl);

        IFilterMapper2_Release(pMapper);
    }

    CoUninitialize();

    return res;
}

/***********************************************************************
 *		DllUnregisterServer (DEVENUM.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    FIXME("stub!\n");
    return __wine_unregister_resources( devenum_instance );
}

#ifdef __REACTOS__

typedef HRESULT (WINAPI *DllRegisterServer_func)(void);

/* calls DllRegisterServer() for the Quartz DLL */
static void DEVENUM_RegisterQuartz(void)
{
    HANDLE hDLL = LoadLibraryA("quartz.dll");
    DllRegisterServer_func pDllRegisterServer = NULL;
    if (hDLL)
        pDllRegisterServer = (DllRegisterServer_func)GetProcAddress(hDLL, "DllRegisterServer");
    if (pDllRegisterServer)
    {
        HRESULT hr = pDllRegisterServer();
        if (FAILED(hr))
            ERR("Failed to register Quartz. Error was 0x%x)\n", hr);
    }
}

#endif
