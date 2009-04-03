/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

#include "initguid.h"

#include "jscript.h"

#include "winreg.h"
#include "advpub.h"
#include "activaut.h"
#include "objsafe.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

LONG module_ref = 0;

static const CLSID CLSID_JScript =
    {0xf414c260,0x6ac0,0x11cf,{0xb6,0xd1,0x00,0xaa,0x00,0xbb,0xbb,0x58}};
static const CLSID CLSID_JScriptAuthor =
    {0xf414c261,0x6ac0,0x11cf,{0xb6,0xd1,0x00,0xaa,0x00,0xbb,0xbb,0x58}};
static const CLSID CLSID_JScriptEncode =
    {0xf414c262,0x6ac0,0x11cf,{0xb6,0xd1,0x00,0xaa,0x00,0xbb,0xbb,0x58}};

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

static HINSTANCE jscript_hinstance;

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IClassFactory, riid)) {
        TRACE("(%p)->(IID_IClassFactory %p)\n", iface, ppv);
        *ppv = iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 1;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    TRACE("(%p)->(%x)\n", iface, fLock);

    if(fLock)
        lock_module();
    else
        unlock_module();

    return S_OK;
}

static const IClassFactoryVtbl JScriptFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    JScriptFactory_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory JScriptFactory = { &JScriptFactoryVtbl };

/******************************************************************
 *              DllMain (jscript.@)
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    TRACE("(%p %d %p)\n", hInstDLL, fdwReason, lpv);

    switch(fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstDLL);
        jscript_hinstance = hInstDLL;
        break;
    }

    return TRUE;
}

/***********************************************************************
 *		DllGetClassObject	(jscript.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    if(IsEqualGUID(&CLSID_JScript, rclsid)) {
        TRACE("(CLSID_JScript %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&JScriptFactory, riid, ppv);
    }

    FIXME("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *          DllCanUnloadNow (jscript.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    TRACE("() ref=%d\n", module_ref);

    return module_ref ? S_FALSE : S_OK;
}

/***********************************************************************
 *              register_inf
 */

#define INF_SET_ID(id)             \
    do                             \
    {                              \
        static CHAR name[] = #id;  \
                                   \
        pse[i].pszName = name;     \
        clsids[i++] = &id;         \
    } while (0)

static HRESULT register_inf(BOOL doregister)
{
    HRESULT hres;
    HMODULE hAdvpack;
    HRESULT (WINAPI *pRegInstall)(HMODULE hm, LPCSTR pszSection, const STRTABLEA* pstTable);
    STRTABLEA strtable;
    STRENTRYA pse[7];
    static CLSID const *clsids[7];
    unsigned int i = 0;

    static const WCHAR advpackW[] = {'a','d','v','p','a','c','k','.','d','l','l',0};

    INF_SET_ID(CLSID_JScript);
    INF_SET_ID(CLSID_JScriptAuthor);
    INF_SET_ID(CLSID_JScriptEncode);
    INF_SET_ID(CATID_ActiveScript);
    INF_SET_ID(CATID_ActiveScriptParse);
    INF_SET_ID(CATID_ActiveScriptEncode);
    INF_SET_ID(CATID_ActiveScriptAuthor);

    for(i = 0; i < sizeof(pse)/sizeof(pse[0]); i++) {
        pse[i].pszValue = HeapAlloc(GetProcessHeap(), 0, 39);
        sprintf(pse[i].pszValue, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                clsids[i]->Data1, clsids[i]->Data2, clsids[i]->Data3, clsids[i]->Data4[0],
                clsids[i]->Data4[1], clsids[i]->Data4[2], clsids[i]->Data4[3], clsids[i]->Data4[4],
                clsids[i]->Data4[5], clsids[i]->Data4[6], clsids[i]->Data4[7]);
    }

    strtable.cEntries = sizeof(pse)/sizeof(pse[0]);
    strtable.pse = pse;

    hAdvpack = LoadLibraryW(advpackW);
    pRegInstall = (void *)GetProcAddress(hAdvpack, "RegInstall");

    hres = pRegInstall(jscript_hinstance, doregister ? "RegisterDll" : "UnregisterDll", &strtable);

    for(i=0; i < sizeof(pse)/sizeof(pse[0]); i++)
        HeapFree(GetProcessHeap(), 0, pse[i].pszValue);

    return hres;
}

#undef INF_SET_CLSID

/***********************************************************************
 *          DllRegisterServer (jscript.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    TRACE("()\n");
    return register_inf(TRUE);
}

/***********************************************************************
 *          DllUnregisterServer (jscript.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    TRACE("()\n");
    return register_inf(FALSE);
}
