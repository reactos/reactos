/*
 * Implementation of class factory for IE Web Browser
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
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

#include <string.h>
#include <stdio.h>

#include "shdocvw.h"
#include "winreg.h"
#include "advpub.h"
#include "isguids.h"

#include "winver.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

/**********************************************************************
 * Implement the WebBrowser class factory
 *
 * (Based on implementation in ddraw/main.c)
 */

#define FACTORY(x) ((IClassFactory*) &(x)->lpClassFactoryVtbl)

typedef struct
{
    /* IUnknown fields */
    const IClassFactoryVtbl *lpClassFactoryVtbl;
    HRESULT (*cf)(LPUNKNOWN, REFIID, LPVOID *);
    LONG ref;
} IClassFactoryImpl;


/**********************************************************************
 * WBCF_QueryInterface (IUnknown)
 */
static HRESULT WINAPI WBCF_QueryInterface(LPCLASSFACTORY iface,
                                          REFIID riid, LPVOID *ppobj)
{
    TRACE("(%s %p)\n", debugstr_guid(riid), ppobj);

    if (!ppobj)
        return E_POINTER;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IClassFactory, riid)) {
        *ppobj = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    WARN("Not supported interface %s\n", debugstr_guid(riid));

    *ppobj = NULL;
    return E_NOINTERFACE;
}

/************************************************************************
 * WBCF_AddRef (IUnknown)
 */
static ULONG WINAPI WBCF_AddRef(LPCLASSFACTORY iface)
{
    SHDOCVW_LockModule();

    return 2; /* non-heap based object */
}

/************************************************************************
 * WBCF_Release (IUnknown)
 */
static ULONG WINAPI WBCF_Release(LPCLASSFACTORY iface)
{
    SHDOCVW_UnlockModule();

    return 1; /* non-heap based object */
}

/************************************************************************
 * WBCF_CreateInstance (IClassFactory)
 */
static HRESULT WINAPI WBCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter,
                                          REFIID riid, LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *) iface;
    return This->cf(pOuter, riid, ppobj);
}

/************************************************************************
 * WBCF_LockServer (IClassFactory)
 */
static HRESULT WINAPI WBCF_LockServer(LPCLASSFACTORY iface, BOOL dolock)
{
    TRACE("(%d)\n", dolock);

    if (dolock)
        SHDOCVW_LockModule();
    else
        SHDOCVW_UnlockModule();
    
    return S_OK;
}

static const IClassFactoryVtbl WBCF_Vtbl =
{
    WBCF_QueryInterface,
    WBCF_AddRef,
    WBCF_Release,
    WBCF_CreateInstance,
    WBCF_LockServer
};

/*************************************************************************
 *              DllGetClassObject (SHDOCVW.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    static IClassFactoryImpl WB1ClassFactory = {&WBCF_Vtbl, WebBrowserV1_Create};
    static IClassFactoryImpl WB2ClassFactory = {&WBCF_Vtbl, WebBrowserV2_Create};
    static IClassFactoryImpl CUHClassFactory = {&WBCF_Vtbl, CUrlHistory_Create};
    static IClassFactoryImpl ISCClassFactory = {&WBCF_Vtbl, InternetShortcut_Create};

    TRACE("\n");

    if(IsEqualGUID(&CLSID_WebBrowser, rclsid))
        return IClassFactory_QueryInterface(FACTORY(&WB2ClassFactory), riid, ppv);

    if(IsEqualGUID(&CLSID_WebBrowser_V1, rclsid))
        return IClassFactory_QueryInterface(FACTORY(&WB1ClassFactory), riid, ppv);

    if(IsEqualGUID(&CLSID_CUrlHistory, rclsid))
        return IClassFactory_QueryInterface(FACTORY(&CUHClassFactory), riid, ppv);

    if(IsEqualGUID(&CLSID_InternetShortcut, rclsid))
        return IClassFactory_QueryInterface(FACTORY(&ISCClassFactory), riid, ppv);

    /* As a last resort, figure if the CLSID belongs to a 'Shell Instance Object' */
    return SHDOCVW_GetShellInstanceObjectClassObject(rclsid, riid, ppv);
}

HRESULT register_class_object(BOOL do_reg)
{
    HRESULT hres;

    static DWORD cookie;
    static IClassFactoryImpl IEClassFactory = {&WBCF_Vtbl, InternetExplorer_Create};

    if(do_reg) {
        hres = CoRegisterClassObject(&CLSID_InternetExplorer, (IUnknown*)FACTORY(&IEClassFactory),
                                     CLSCTX_SERVER, REGCLS_MULTIPLEUSE|REGCLS_SUSPENDED, &cookie);
        if (FAILED(hres)) {
            ERR("failed to register object %08x\n", hres);
            return hres;
        }

        hres = CoResumeClassObjects();
        if(SUCCEEDED(hres))
            return hres;

        ERR("failed to resume object %08x\n", hres);
    }

    return CoRevokeClassObject(cookie);
}

static HRESULT reg_install(LPCSTR section, STRTABLEA *strtable)
{
    HRESULT (WINAPI *pRegInstall)(HMODULE hm, LPCSTR pszSection, const STRTABLEA* pstTable);
    HMODULE hadvpack;
    HRESULT hres;

    static const WCHAR advpackW[] = {'a','d','v','p','a','c','k','.','d','l','l',0};

    hadvpack = LoadLibraryW(advpackW);
    pRegInstall = (void *)GetProcAddress(hadvpack, "RegInstall");

    hres = pRegInstall(shdocvw_hinstance, section, strtable);

    FreeLibrary(hadvpack);
    return hres;
}

static const GUID CLSID_MicrosoftBrowserArchitecture =
    {0xa5e46e3a, 0x8849, 0x11d1, {0x9d, 0x8c, 0x00, 0xc0, 0x4f, 0xc9, 0x9d, 0x61}};
static const GUID CLSID_MruLongList =
    {0x53bd6b4e, 0x3780, 0x4693, {0xaf, 0xc3, 0x71, 0x61, 0xc2, 0xf3, 0xee, 0x9c}};

#define INF_SET_CLSID(clsid)                  \
    do                                        \
    {                                         \
        static CHAR name[] = "CLSID_" #clsid; \
                                              \
        pse[i].pszName = name;                \
        clsids[i++] = &CLSID_ ## clsid;       \
    } while (0)

static HRESULT register_server(BOOL doregister)
{
    STRTABLEA strtable;
    STRENTRYA pse[14];
    static CLSID const *clsids[14];
    unsigned int i = 0;
    HRESULT hres;

    INF_SET_CLSID(CUrlHistory);
    INF_SET_CLSID(Internet);
    INF_SET_CLSID(InternetExplorer);
    INF_SET_CLSID(InternetShortcut);
    INF_SET_CLSID(MicrosoftBrowserArchitecture);
    INF_SET_CLSID(MruLongList);
    INF_SET_CLSID(SearchAssistantOC);
    INF_SET_CLSID(ShellNameSpace);
    INF_SET_CLSID(ShellSearchAssistantOC);
    INF_SET_CLSID(ShellShellNameSpace);
    INF_SET_CLSID(ShellUIHelper);
    INF_SET_CLSID(ShellWindows);
    INF_SET_CLSID(WebBrowser);
    INF_SET_CLSID(WebBrowser_V1);

    for(i = 0; i < sizeof(pse)/sizeof(pse[0]); i++) {
        pse[i].pszValue = HeapAlloc(GetProcessHeap(), 0, 39);
        sprintf(pse[i].pszValue, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                clsids[i]->Data1, clsids[i]->Data2, clsids[i]->Data3, clsids[i]->Data4[0],
                clsids[i]->Data4[1], clsids[i]->Data4[2], clsids[i]->Data4[3], clsids[i]->Data4[4],
                clsids[i]->Data4[5], clsids[i]->Data4[6], clsids[i]->Data4[7]);
    }

    strtable.cEntries = sizeof(pse)/sizeof(pse[0]);
    strtable.pse = pse;

    hres = reg_install(doregister ? "RegisterDll" : "UnregisterDll", &strtable);

    for(i=0; i < sizeof(pse)/sizeof(pse[0]); i++)
        HeapFree(GetProcessHeap(), 0, pse[i].pszValue);

    return hres;
}

#undef INF_SET_CLSID

/***********************************************************************
 *          DllRegisterServer (shdocvw.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    ITypeLib *typelib;
    HRESULT hres;

    static const WCHAR shdocvwW[] = {'s','h','d','o','c','v','w','.','d','l','l',0};

    hres = register_server(TRUE);
    if(FAILED(hres))
        return hres;

    hres = LoadTypeLibEx(shdocvwW, REGKIND_REGISTER, &typelib);
    if(FAILED(hres)) {
        ERR("Could not load typelib: %08x\n", hres);
        return hres;
    }

    ITypeLib_Release(typelib);

    return hres;
}

/***********************************************************************
 *          DllUnregisterServer (shdocvw.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    HRESULT hres;

    hres = register_server(FALSE);
    if(FAILED(hres))
        return hres;

    return UnRegisterTypeLib(&LIBID_SHDocVw, 1, 1, LOCALE_SYSTEM_DEFAULT, SYS_WIN32);
}

static BOOL check_native_ie(void)
{
    static const WCHAR cszPath[] = {'b','r','o','w','s','e','u','i','.','d','l','l',0};
    DWORD handle,size;
    BOOL ret = TRUE;

    size = GetFileVersionInfoSizeW(cszPath,&handle);
    if (size)
    {
        LPVOID buf;
        LPWSTR lpFileDescription;
        UINT dwBytes;
        static const WCHAR cszFD[] = {'\\','S','t','r','i','n','g','F','i','l','e','I','n','f','o','\\','0','4','0','9','0','4','e','4','\\','F','i','l','e','D','e','s','c','r','i','p','t','i','o','n',0};
        static const WCHAR cszWine[] = {'W','i','n','e',0};

        buf = HeapAlloc(GetProcessHeap(),0,size);
        GetFileVersionInfoW(cszPath,0,size,buf);

        if (VerQueryValueW(buf, cszFD, (LPVOID*)&lpFileDescription, &dwBytes) &&
            strstrW(lpFileDescription,cszWine))
                ret = FALSE;

        HeapFree(GetProcessHeap(), 0, buf);
    }

    return ret;
}

DWORD register_iexplore(BOOL doregister)
{
    HRESULT hres;
    if (check_native_ie())
    {
        TRACE("Native IE detected, not doing registration\n");
        return S_OK;
    }
    hres = reg_install(doregister ? "RegisterIE" : "UnregisterIE", NULL);
    return FAILED(hres);
}
