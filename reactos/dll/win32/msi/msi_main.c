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
 *    	DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        msi_hInstance = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);
        msi_dialog_register_class();
        break;
    case DLL_PROCESS_DETACH:
        msi_dialog_unregister_class();
        msi_free_handle_table();
        break;
    }
    return TRUE;
}

typedef struct tagIClassFactoryImpl
{
    const IClassFactoryVtbl *lpVtbl;
} IClassFactoryImpl;

static HRESULT WINAPI MsiCF_QueryInterface(LPCLASSFACTORY iface,
                REFIID riid,LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    FIXME("%p %s %p\n",This,debugstr_guid(riid),ppobj);
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

    FIXME("%p %p %s %p\n", This, pOuter, debugstr_guid(riid), ppobj);
    return E_FAIL;
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

static IClassFactoryImpl Msi_CF = { &MsiCF_Vtbl };

/******************************************************************
 * DllGetClassObject          [MSI.@]
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if( IsEqualCLSID (rclsid, &CLSID_IMsiServer) ||
        IsEqualCLSID (rclsid, &CLSID_IMsiServerMessage) ||
        IsEqualCLSID (rclsid, &CLSID_IMsiServerX1) ||
        IsEqualCLSID (rclsid, &CLSID_IMsiServerX2) ||
        IsEqualCLSID (rclsid, &CLSID_IMsiServerX3) )
    {
        *ppv = (LPVOID) &Msi_CF;
        return S_OK;
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
