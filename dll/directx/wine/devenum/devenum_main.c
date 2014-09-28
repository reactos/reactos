/*
 *	exported dll functions for devenum.dll
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

#include <rpcproxy.h>

DECLSPEC_HIDDEN LONG dll_refs;
DECLSPEC_HIDDEN HINSTANCE DEVENUM_hInstance;

typedef struct
{
    REFCLSID clsid;
    LPCWSTR friendly_name;
    BOOL instance;
} register_info;

#ifdef __REACTOS__
static void DEVENUM_RegisterQuartz(void);
#endif

/***********************************************************************
 *		Global string constant definitions
 */
const WCHAR clsid_keyname[6] = { 'C', 'L', 'S', 'I', 'D', 0 };

/***********************************************************************
 *		DllEntryPoint
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%x %p\n", hinstDLL, fdwReason, fImpLoad);

    switch(fdwReason) {
    case DLL_PROCESS_ATTACH:
        DEVENUM_hInstance = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);
	break;
    }
    return TRUE;
}

/***********************************************************************
 *		DllGetClassObject (DEVENUM.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    TRACE("(%s, %s, %p)\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);

    *ppv = NULL;

    /* FIXME: we should really have two class factories.
     * Oh well - works just fine as it is */
    if (IsEqualGUID(rclsid, &CLSID_SystemDeviceEnum) ||
        IsEqualGUID(rclsid, &CLSID_CDeviceMoniker))
        return IClassFactory_QueryInterface(&DEVENUM_ClassFactory.IClassFactory_iface, iid, ppv);

    FIXME("CLSID: %s, IID: %s\n", debugstr_guid(rclsid), debugstr_guid(iid));
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

    res = __wine_register_resources( DEVENUM_hInstance );

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
    return __wine_unregister_resources( DEVENUM_hInstance );
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
