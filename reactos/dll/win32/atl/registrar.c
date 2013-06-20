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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <objbase.h>

//#include "oaidl.h"
//#include "rpcproxy.h"
#include <atlbase.h>

#include <wine/debug.h>
//#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(atl);

/**************************************************************
 * ClassFactory implementation
 */

static HRESULT WINAPI RegistrarCF_QueryInterface(IClassFactory *iface, REFIID riid, void **ppvObject)
{
    TRACE("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppvObject);

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IClassFactory, riid)) {
        *ppvObject = iface;
        IClassFactory_AddRef( iface );
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI RegistrarCF_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI RegistrarCF_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI RegistrarCF_CreateInstance(IClassFactory *iface, LPUNKNOWN pUnkOuter,
                                                REFIID riid, void **ppv)
{
    IRegistrar *registrar;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);

    if(pUnkOuter) {
        *ppv = NULL;
        return CLASS_E_NOAGGREGATION;
    }

    hres = AtlCreateRegistrar(&registrar);
    if(FAILED(hres))
        return hres;

    hres = IRegistrar_QueryInterface(registrar, riid, ppv);
    IRegistrar_Release(registrar);
    return hres;
}

static HRESULT WINAPI RegistrarCF_LockServer(IClassFactory *iface, BOOL lock)
{
    TRACE("(%p)->(%x)\n", iface, lock);
    return S_OK;
}

static const IClassFactoryVtbl IRegistrarCFVtbl = {
    RegistrarCF_QueryInterface,
    RegistrarCF_AddRef,
    RegistrarCF_Release,
    RegistrarCF_CreateInstance,
    RegistrarCF_LockServer
};

static IClassFactory RegistrarCF = { &IRegistrarCFVtbl };

/**************************************************************
 * DllGetClassObject (ATL.2)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, LPVOID *ppvObject)
{
    TRACE("(%s %s %p)\n", debugstr_guid(clsid), debugstr_guid(riid), ppvObject);

    if(IsEqualGUID(&CLSID_Registrar, clsid))
        return IClassFactory_QueryInterface( &RegistrarCF, riid, ppvObject );

    FIXME("Not supported class %s\n", debugstr_guid(clsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

extern HINSTANCE hInst;

static HRESULT do_register_dll_server(IRegistrar *pRegistrar, LPCOLESTR wszDll,
                                      LPCOLESTR wszId, BOOL do_register,
                                      const struct _ATL_REGMAP_ENTRY* pMapEntries)
{
    IRegistrar *registrar;
    HRESULT hres;
    const struct _ATL_REGMAP_ENTRY *pMapEntry;

    static const WCHAR wszModule[] = {'M','O','D','U','L','E',0};
    static const WCHAR wszRegistry[] = {'R','E','G','I','S','T','R','Y',0};

    if(pRegistrar) {
        registrar = pRegistrar;
    }else {
        hres = AtlCreateRegistrar(&registrar);
        if(FAILED(hres))
            return hres;
    }

    IRegistrar_AddReplacement(registrar, wszModule, wszDll);

    for (pMapEntry = pMapEntries; pMapEntry && pMapEntry->szKey; pMapEntry++)
        IRegistrar_AddReplacement(registrar, pMapEntry->szKey, pMapEntry->szData);

    if(do_register)
        hres = IRegistrar_ResourceRegisterSz(registrar, wszDll, wszId, wszRegistry);
    else
        hres = IRegistrar_ResourceUnregisterSz(registrar, wszDll, wszId, wszRegistry);

    if(registrar != pRegistrar)
        IRegistrar_Release(registrar);
    return hres;
}

static HRESULT do_register_server(BOOL do_register)
{
    static const WCHAR CLSID_RegistrarW[] =
            {'C','L','S','I','D','_','R','e','g','i','s','t','r','a','r',0};
    static const WCHAR atl_dllW[] = {'a','t','l','.','d','l','l',0};

    WCHAR clsid_str[40];
    const struct _ATL_REGMAP_ENTRY reg_map[] = {{CLSID_RegistrarW, clsid_str}, {NULL,NULL}};

    StringFromGUID2(&CLSID_Registrar, clsid_str, sizeof(clsid_str)/sizeof(WCHAR));
    return do_register_dll_server(NULL, atl_dllW, MAKEINTRESOURCEW(101), do_register, reg_map);
}

/***********************************************************************
 *           AtlModuleUpdateRegistryFromResourceD         [ATL.@]
 *
 */
HRESULT WINAPI AtlModuleUpdateRegistryFromResourceD(_ATL_MODULEW* pM, LPCOLESTR lpszRes,
		BOOL bRegister, struct _ATL_REGMAP_ENTRY* pMapEntries, IRegistrar* pReg)
{
    HINSTANCE lhInst = pM->m_hInst;
    /* everything inside this function below this point
     * should go into atl71.AtlUpdateRegistryFromResourceD
     */
    WCHAR module_name[MAX_PATH];

    if(!GetModuleFileNameW(lhInst, module_name, MAX_PATH)) {
        FIXME("hinst %p: did not get module name\n",
        lhInst);
        return E_FAIL;
    }

    TRACE("%p (%s), %s, %d, %p, %p\n", hInst, debugstr_w(module_name),
	debugstr_w(lpszRes), bRegister, pMapEntries, pReg);

    return do_register_dll_server(pReg, module_name, lpszRes, bRegister, pMapEntries);
}

/***********************************************************************
 *              DllRegisterServer (ATL.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    /* Note: we can't use __wine_register_server here because it uses CLSID_Registrar which isn't registred yet */
    return do_register_server(TRUE);
}

/***********************************************************************
 *              DllUnRegisterServer (ATL.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return do_register_server(FALSE);
}

/***********************************************************************
 *              DllCanUnloadNow (ATL.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}
