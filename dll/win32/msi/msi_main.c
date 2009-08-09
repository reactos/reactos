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
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "shlwapi.h"
#include "oleauto.h"
#include "msipriv.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static LONG dll_count;

/* the UI level */
INSTALLUILEVEL gUILevel = INSTALLUILEVEL_BASIC;
HWND           gUIhwnd = 0;
INSTALLUI_HANDLERA gUIHandlerA = NULL;
INSTALLUI_HANDLERW gUIHandlerW = NULL;
DWORD gUIFilter = 0;
LPVOID gUIContext = NULL;
WCHAR gszLogFile[MAX_PATH];
HINSTANCE msi_hInstance;

static WCHAR msi_path[MAX_PATH];
static ITypeLib *msi_typelib;

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
        break;
    case DLL_PROCESS_DETACH:
        if (msi_typelib) ITypeLib_Release( msi_typelib );
        msi_dialog_unregister_class();
        msi_free_handle_table();
        break;
    }
    return TRUE;
}

static CRITICAL_SECTION MSI_typelib_cs;
static CRITICAL_SECTION_DEBUG MSI_typelib_cs_debug =
{
    0, 0, &MSI_typelib_cs,
    { &MSI_typelib_cs_debug.ProcessLocksList,
      &MSI_typelib_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": MSI_typelib_cs") }
};
static CRITICAL_SECTION MSI_typelib_cs = { &MSI_typelib_cs_debug, -1, 0, 0, 0, 0 };

ITypeLib *get_msi_typelib( LPWSTR *path )
{
    EnterCriticalSection( &MSI_typelib_cs );

    if (!msi_typelib)
    {
        TRACE("loading typelib\n");

        if (GetModuleFileNameW( msi_hInstance, msi_path, MAX_PATH ))
            LoadTypeLib( msi_path, &msi_typelib );
    }

    LeaveCriticalSection( &MSI_typelib_cs );

    if (path)
        *path = msi_path;

    if (msi_typelib)
        ITypeLib_AddRef( msi_typelib );

    return msi_typelib;
}

typedef struct tagIClassFactoryImpl {
    const IClassFactoryVtbl *lpVtbl;
    HRESULT (*create_object)( IUnknown*, LPVOID* );
} IClassFactoryImpl;

static HRESULT WINAPI MsiCF_QueryInterface(LPCLASSFACTORY iface,
                REFIID riid,LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

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
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
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

static IClassFactoryImpl MsiServer_CF = { &MsiCF_Vtbl, create_msiserver };
static IClassFactoryImpl WineMsiCustomRemote_CF = { &MsiCF_Vtbl, create_msi_custom_remote };
static IClassFactoryImpl WineMsiRemotePackage_CF = { &MsiCF_Vtbl, create_msi_remote_package };

/******************************************************************
 * DllGetClassObject          [MSI.@]
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if ( IsEqualCLSID (rclsid, &CLSID_IMsiServerX2) )
    {
        *ppv = &MsiServer_CF;
        return S_OK;
    }

    if ( IsEqualCLSID (rclsid, &CLSID_IWineMsiRemoteCustomAction) )
    {
        *ppv = &WineMsiCustomRemote_CF;
        return S_OK;
    }

    if ( IsEqualCLSID (rclsid, &CLSID_IWineMsiRemotePackage) )
    {
        *ppv = &WineMsiRemotePackage_CF;
        return S_OK;
    }

    if( IsEqualCLSID (rclsid, &CLSID_IMsiServerMessage) ||
        IsEqualCLSID (rclsid, &CLSID_IMsiServer) ||
        IsEqualCLSID (rclsid, &CLSID_IMsiServerX1) ||
        IsEqualCLSID (rclsid, &CLSID_IMsiServerX3) )
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
