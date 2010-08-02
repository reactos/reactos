/*
 * Copyright (C) 2008 Google (Roy Shea)
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

#include "mstask_private.h"
#include "winreg.h"
#include "advpub.h"

#include "wine/debug.h"


WINE_DEFAULT_DEBUG_CHANNEL(mstask);

static HINSTANCE hInst;
LONG dll_ref = 0;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            hInst = hinstDLL;
            break;
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    TRACE("(%s %s %p)\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);

    if (IsEqualGUID(rclsid, &CLSID_CTaskScheduler)) {
        return IClassFactory_QueryInterface((LPCLASSFACTORY)&MSTASK_ClassFactory, iid, ppv);
    }

    FIXME("Not supported class: %s\n", debugstr_guid(rclsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return dll_ref != 0 ? S_FALSE : S_OK;
}

static inline char *mstask_strdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *d = HeapAlloc(GetProcessHeap(), 0, n);
    return d ? memcpy(d, s, n) : NULL;
}

static HRESULT init_register_strtable(STRTABLEA *strtable)
{
#define CLSID_EXPANSION_ENTRY(id) { "CLSID_" #id, &CLSID_ ## id }
    static const struct
    {
        const char *name;
        const CLSID *clsid;
    }
    expns[] =
    {
        CLSID_EXPANSION_ENTRY(CTaskScheduler),
        CLSID_EXPANSION_ENTRY(CTask)
    };
#undef CLSID_EXPANSION_ENTRY
    static STRENTRYA pse[sizeof expns / sizeof expns[0]];
    unsigned int i;

    strtable->cEntries = sizeof pse / sizeof pse[0];
    strtable->pse = pse;
    for (i = 0; i < strtable->cEntries; i++)
    {
        static const char dummy_sample[] =
                "{12345678-1234-1234-1234-123456789012}";
        const CLSID *clsid = expns[i].clsid;
        pse[i].pszName = mstask_strdup(expns[i].name);
        pse[i].pszValue = HeapAlloc(GetProcessHeap(), 0, sizeof dummy_sample);
        if (!pse[i].pszName || !pse[i].pszValue)
            return E_OUTOFMEMORY;
        sprintf(pse[i].pszValue,
                "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                clsid->Data1, clsid->Data2, clsid->Data3, clsid->Data4[0],
                clsid->Data4[1], clsid->Data4[2], clsid->Data4[3],
                clsid->Data4[4], clsid->Data4[5], clsid->Data4[6],
                clsid->Data4[7]);
    }

    return S_OK;
}

static void cleanup_register_strtable(STRTABLEA *strtable)
{
    unsigned int i;
    for (i = 0; i < strtable->cEntries; i++)
    {
        HeapFree(GetProcessHeap(), 0, strtable->pse[i].pszName);
        HeapFree(GetProcessHeap(), 0, strtable->pse[i].pszValue);
        if (!strtable->pse[i].pszName || !strtable->pse[i].pszValue)
            return;
    }
}

static HRESULT register_mstask(BOOL do_register)
{
    HRESULT hr;
    STRTABLEA strtable;
    HMODULE hAdvpack;
    HRESULT (WINAPI *pRegInstall)(HMODULE hm,
            LPCSTR pszSection, const STRTABLEA* pstTable);
    static const WCHAR wszAdvpack[] =
            {'a','d','v','p','a','c','k','.','d','l','l',0};

    TRACE("(%x)\n", do_register);

    hAdvpack = LoadLibraryW(wszAdvpack);
    pRegInstall = (void *)GetProcAddress(hAdvpack, "RegInstall");

    hr = init_register_strtable(&strtable);
    if (SUCCEEDED(hr))
        hr = pRegInstall(hInst, do_register ? "RegisterDll" : "UnregisterDll",
                &strtable);
    cleanup_register_strtable(&strtable);

    if (FAILED(hr))
        WINE_ERR("RegInstall failed: %08x\n", hr);

    return hr;
}

HRESULT WINAPI DllRegisterServer(void)
{
    return register_mstask(TRUE);
}

HRESULT WINAPI DllUnregisterServer(void)
{
    return register_mstask(FALSE);
}
