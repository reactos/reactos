/*
 * Main DLL interface to Queue Manager (BITS)
 *
 * Background Intelligent Transfer Service (BITS) interface.  Dll is named
 * qmgr for backwards compatibility with early versions of BITS.
 *
 * Copyright 2007 Google (Roy Shea)
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

#include <stdio.h>

#include "objbase.h"
#include "winuser.h"
#include "winreg.h"
#include "advpub.h"
#include "olectl.h"
#include "winsvc.h"

#include "bits.h"
#include "qmgr.h"
#include "initguid.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qmgr);

/* Handle to the base address of this DLL */
static HINSTANCE hInst;

/* Other GUIDs used by this module */
DEFINE_GUID(CLSID_BackgroundCopyQMgr, 0x69AD4AEE, 0x51BE, 0x439b, 0xA9,0x2C, 0x86,0xAE,0x49,0x0E,0x8B,0x30);

/* Entry point for DLL */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;  /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            hInst = hinstDLL;
            break;
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}

static HRESULT init_register_strtable(STRTABLEA *strtable)
{
#define CLSID_EXPANSION_ENTRY(id) { "CLSID_" #id, &CLSID_ ## id }
    static const struct {
        const char *name;
        const CLSID *clsid;
    } expns[] =  {
        CLSID_EXPANSION_ENTRY(BackgroundCopyQMgr),
        CLSID_EXPANSION_ENTRY(BackgroundCopyManager)
    };
#undef CLSID_EXPANSION_ENTRY
    static STRENTRYA pse[sizeof expns / sizeof expns[0]];
    DWORD i;

    strtable->cEntries = sizeof pse / sizeof pse[0];
    strtable->pse = pse;
    for (i = 0; i < strtable->cEntries; i++) {
        static const char dummy_sample[] = "{12345678-1234-1234-1234-123456789012}";
        const CLSID *clsid = expns[i].clsid;
        pse[i].pszName = qmgr_strdup(expns[i].name);
        pse[i].pszValue = HeapAlloc(GetProcessHeap(), 0, sizeof dummy_sample);
        if (!pse[i].pszName || !pse[i].pszValue)
            return E_OUTOFMEMORY;
        sprintf(pse[i].pszValue, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                clsid->Data1, clsid->Data2, clsid->Data3, clsid->Data4[0],
                clsid->Data4[1], clsid->Data4[2], clsid->Data4[3], clsid->Data4[4],
                clsid->Data4[5], clsid->Data4[6], clsid->Data4[7]);
    }

    return S_OK;
}

static void cleanup_register_strtable(STRTABLEA *strtable)
{
    DWORD i;
    for (i = 0; i < strtable->cEntries; i++) {
        HeapFree(GetProcessHeap(), 0, strtable->pse[i].pszName);
        HeapFree(GetProcessHeap(), 0, strtable->pse[i].pszValue);
        if (!strtable->pse[i].pszName || !strtable->pse[i].pszValue)
            return;
    }
}

static HRESULT register_service(BOOL do_register)
{
    static const WCHAR name[] = { 'B','I','T','S', 0 };
    static const WCHAR path[] = { 's','v','c','h','o','s','t','.','e','x','e',
                                  ' ','-','k',' ','n','e','t','s','v','c','s', 0 };
    SC_HANDLE scm, service;

    scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!scm)
        return SELFREG_E_CLASS;

    if (do_register)
        service = CreateServiceW(scm, name, name, SERVICE_ALL_ACCESS,
                                 SERVICE_WIN32_OWN_PROCESS,
                                 SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
                                 path, NULL, NULL, NULL, NULL, NULL);
    else
        service = OpenServiceW(scm, name, DELETE);


    CloseServiceHandle(scm);
    if (service)
    {
        if (!do_register) DeleteService(service);
        CloseServiceHandle(service);
    }
    return S_OK;
}

/* Use an INF file to register or unregister the DLL */
static HRESULT register_server(BOOL do_register)
{
    HRESULT hr;
    STRTABLEA strtable;
    HMODULE hAdvpack;
    HRESULT (WINAPI *pRegInstall)(HMODULE hm, LPCSTR pszSection, const STRTABLEA* pstTable);
    static const WCHAR wszAdvpack[] = {'a','d','v','p','a','c','k','.','d','l','l',0};

    TRACE("(%x)\n", do_register);

    hr = register_service(do_register);
    if (FAILED(hr)) {
        ERR("register_service failed: %d\n", GetLastError());
        return hr;
    }

    hAdvpack = LoadLibraryW(wszAdvpack);
    pRegInstall = (void *)GetProcAddress(hAdvpack, "RegInstall");

    hr = init_register_strtable(&strtable);
    if (SUCCEEDED(hr))
        hr = pRegInstall(hInst, do_register ? "RegisterDll" : "UnregisterDll",
                         &strtable);
    cleanup_register_strtable(&strtable);

    if (FAILED(hr))
        ERR("RegInstall failed: %08x\n", hr);

    return hr;
}

HRESULT WINAPI DllRegisterServer(void)
{
    return register_server(TRUE);
}

HRESULT WINAPI DllUnregisterServer(void)
{
    return register_server(FALSE);
}
