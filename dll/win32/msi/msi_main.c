/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2006 Mike McCormack for CodeWeavers
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
#include "winreg.h"
#include "shlwapi.h"
#include "oleauto.h"
#include "rpcproxy.h"
#include "msipriv.h"
#include "msiserver.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static LONG dll_count;

/* the UI level */
INSTALLUILEVEL           gUILevel         = INSTALLUILEVEL_DEFAULT;
HWND                     gUIhwnd          = 0;
INSTALLUI_HANDLERA       gUIHandlerA      = NULL;
INSTALLUI_HANDLERW       gUIHandlerW      = NULL;
INSTALLUI_HANDLER_RECORD gUIHandlerRecord = NULL;
DWORD                    gUIFilter        = 0;
DWORD                    gUIFilterRecord  = 0;
LPVOID                   gUIContext       = NULL;
LPVOID                   gUIContextRecord = NULL;
WCHAR                   *gszLogFile       = NULL;
HINSTANCE msi_hInstance;


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

/******************************************************************
 *      DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        msi_hInstance = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);
        IsWow64Process( GetCurrentProcess(), &is_wow64 );
        break;
    case DLL_PROCESS_DETACH:
        if (lpvReserved) break;
        msi_dialog_unregister_class();
        msi_free_handle_table();
        free( gszLogFile );
        release_typelib();
        break;
    }
    return TRUE;
}

struct class_factory
{
    IClassFactory IClassFactory_iface;
    HRESULT (*create_object)( IUnknown *, void ** );
};

static inline struct class_factory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, struct class_factory, IClassFactory_iface);
}

static HRESULT WINAPI MsiCF_QueryInterface(LPCLASSFACTORY iface,
                REFIID riid,LPVOID *ppobj)
{
    struct class_factory *This = impl_from_IClassFactory(iface);

    TRACE("%p %s %p\n",This,debugstr_guid(riid),ppobj);

    if( IsEqualCLSID( riid, &IID_IUnknown ) ||
        IsEqualCLSID( riid, &IID_IClassFactory ) )
    {
        IClassFactory_AddRef( iface );
        *ppobj = iface;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI MsiCF_AddRef(LPCLASSFACTORY iface)
{
    LockModule();
    return 2;
}

static ULONG WINAPI MsiCF_Release(LPCLASSFACTORY iface)
{
    UnlockModule();
    return 1;
}

static HRESULT WINAPI MsiCF_CreateInstance(LPCLASSFACTORY iface,
    LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj)
{
    struct class_factory *This = impl_from_IClassFactory(iface);
    IUnknown *unk = NULL;
    HRESULT r;

    TRACE("%p %p %s %p\n", This, pOuter, debugstr_guid(riid), ppobj);

    r = This->create_object( pOuter, (LPVOID*) &unk );
    if (SUCCEEDED(r))
    {
        r = IUnknown_QueryInterface( unk, riid, ppobj );
        IUnknown_Release( unk );
    }
    return r;
}

static HRESULT WINAPI MsiCF_LockServer(LPCLASSFACTORY iface, BOOL dolock)
{
    TRACE("%p %d\n", iface, dolock);

    if (dolock)
        LockModule();
    else
        UnlockModule();

    return S_OK;
}

static const IClassFactoryVtbl MsiCF_Vtbl =
{
    MsiCF_QueryInterface,
    MsiCF_AddRef,
    MsiCF_Release,
    MsiCF_CreateInstance,
    MsiCF_LockServer
};

static struct class_factory MsiServer_CF = { { &MsiCF_Vtbl }, create_msiserver };

/******************************************************************
 * DllGetClassObject          [MSI.@]
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if ( IsEqualCLSID (rclsid, &CLSID_MsiInstaller) )
    {
        *ppv = &MsiServer_CF;
        return S_OK;
    }

    if( IsEqualCLSID (rclsid, &CLSID_MsiServerMessage) ||
        IsEqualCLSID (rclsid, &CLSID_MsiServer) ||
        IsEqualCLSID (rclsid, &CLSID_PSFactoryBuffer) ||
        IsEqualCLSID (rclsid, &CLSID_MsiServerX3) )
    {
        FIXME("create %s object\n", debugstr_guid( rclsid ));
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}

/******************************************************************
 * DllGetVersion              [MSI.@]
 */
HRESULT WINAPI DllGetVersion(DLLVERSIONINFO *pdvi)
{
    TRACE("%p\n",pdvi);

    if (pdvi->cbSize < sizeof(DLLVERSIONINFO))
        return E_INVALIDARG;

    pdvi->dwMajorVersion = MSI_MAJORVERSION;
    pdvi->dwMinorVersion = MSI_MINORVERSION;
    pdvi->dwBuildNumber = MSI_BUILDNUMBER;
    pdvi->dwPlatformID = DLLVER_PLATFORM_WINDOWS;

    return S_OK;
}

/******************************************************************
 * DllCanUnloadNow            [MSI.@]
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return dll_count == 0 ? S_OK : S_FALSE;
}
