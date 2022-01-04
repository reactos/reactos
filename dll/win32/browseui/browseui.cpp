/*
 * ReactOS Explorer
 *
 * Copyright 2009 Andrew Hill <ash77 at domain reactos.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "precomp.h"


HRESULT CAddressBand_CreateInstance(REFIID riid, void **ppv)
{
#if USE_CUSTOM_ADDRESSBAND
    return ShellObjectCreator<CAddressBand>(riid, ppv);
#else
    return CoCreateInstance(CLSID_SH_AddressBand, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IUnknown, toolsBar));
#endif
}

HRESULT CAddressEditBox_CreateInstance(REFIID riid, void **ppv)
{
#if USE_CUSTOM_ADDRESSEDITBOX
    return ShellObjectCreator<CAddressEditBox>(riid, ppv);
#else
    return CoCreateInstance(CLSID_AddressEditBox, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(riid, &ppv));
#endif
}

HRESULT CBandProxy_CreateInstance(REFIID riid, void **ppv)
{
#if USE_CUSTOM_BANDPROXY
    return ShellObjectCreator<CBandProxy>(riid, ppv);
#else
    return CoCreateInstance(CLSID_BandProxy, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(riid, &ppv));
#endif
}

HRESULT CBrandBand_CreateInstance(REFIID riid, void **ppv)
{
#if USE_CUSTOM_BRANDBAND
    return ShellObjectCreator<CBrandBand>(riid, ppv);
#else
    return CoCreateInstance(CLSID_BrandBand, NULL, CLSCTX_INPROC_SERVER, riid, ppv);
#endif
}

HRESULT CSearchBar_CreateInstance(REFIID riid, LPVOID *ppv)
{
#if USE_CUSTOM_SEARCHBAND
    return ShellObjectCreator<CSearchBar>(riid, ppv);
#else
    return CoCreateInstance(CLSID_FileSearchBand, NULL, CLSCTX_INPROC_SERVER, riid, ppv);
#endif
}

HRESULT CExplorerBand_CreateInstance(REFIID riid, LPVOID *ppv)
{
#if USE_CUSTOM_EXPLORERBAND
    return ShellObjectCreator<CExplorerBand>(riid, ppv);
#else
    return CoCreateInstance(CLSID_ExplorerBand, NULL, CLSCTX_INPROC_SERVER, riid, ppv);
#endif
}

HRESULT CInternetToolbar_CreateInstance(REFIID riid, void **ppv)
{
#if USE_CUSTOM_INTERNETTOOLBAR
    return ShellObjectCreator<CInternetToolbar>(riid, ppv);
#else
    return CoCreateInstance(CLSID_InternetToolbar, NULL, CLSCTX_INPROC_SERVER, riid, ppv);
#endif
}

typedef HRESULT(WINAPI * PMENUBAND_CREATEINSTANCE)(REFIID riid, void **ppv);
typedef HRESULT(WINAPI * PMERGEDFOLDER_CREATEINSTANCE)(REFIID riid, void **ppv);

HRESULT CMergedFolder_CreateInstance(REFIID riid, void **ppv)
{
#if USE_CUSTOM_MERGEDFOLDER
    HMODULE hRShell = GetModuleHandle(L"rshell.dll");
    if (!hRShell)
        hRShell = LoadLibrary(L"rshell.dll");

    if (hRShell)
    {
        PMERGEDFOLDER_CREATEINSTANCE pCMergedFolder_CreateInstance = (PMERGEDFOLDER_CREATEINSTANCE)
             GetProcAddress(hRShell, "CMergedFolder_CreateInstance");

        if (pCMergedFolder_CreateInstance)
        {
            return pCMergedFolder_CreateInstance(riid, ppv);
        }
    }
#endif
    return CoCreateInstance(CLSID_MergedFolder, NULL, CLSCTX_INPROC_SERVER, riid, ppv);
}

HRESULT CMenuBand_CreateInstance(REFIID iid, LPVOID *ppv)
{
#if USE_CUSTOM_MENUBAND
    HMODULE hRShell = GetModuleHandleW(L"rshell.dll");

    if (!hRShell)
        hRShell = LoadLibraryW(L"rshell.dll");

    if (hRShell)
    {
        PMENUBAND_CREATEINSTANCE func = (PMENUBAND_CREATEINSTANCE) GetProcAddress(hRShell, "CMenuBand_CreateInstance");
        if (func)
        {
            return func(iid , ppv);
        }
    }
#endif
    return CoCreateInstance(CLSID_MenuBand, NULL, CLSCTX_INPROC_SERVER, iid, ppv);
}


class CBrowseUIModule : public CComModule
{
public:
};


BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_ACLCustomMRU, CACLCustomMRU)
OBJECT_ENTRY(CLSID_AutoComplete, CAutoComplete)
OBJECT_ENTRY(CLSID_ACLHistory, CACLHistory)
OBJECT_ENTRY(CLSID_ACLMulti, CACLMulti)
OBJECT_ENTRY(CLSID_ACListISF, CACListISF)
OBJECT_ENTRY(CLSID_SH_AddressBand, CAddressBand)
OBJECT_ENTRY(CLSID_AddressEditBox, CAddressEditBox)
OBJECT_ENTRY(CLSID_BandProxy, CBandProxy)
OBJECT_ENTRY(CLSID_RebarBandSite, CBandSite)
OBJECT_ENTRY(CLSID_BandSiteMenu, CBandSiteMenu)
OBJECT_ENTRY(CLSID_BrandBand, CBrandBand)
OBJECT_ENTRY(CLSID_CCommonBrowser, CCommonBrowser)
OBJECT_ENTRY(CLSID_GlobalFolderSettings, CGlobalFolderSettings)
OBJECT_ENTRY(CLSID_InternetToolbar, CInternetToolbar)
OBJECT_ENTRY(CLSID_CRegTreeOptions, CRegTreeOptions)
OBJECT_ENTRY(CLSID_ShellTaskScheduler, CShellTaskScheduler)
OBJECT_ENTRY(CLSID_TaskbarList, CTaskbarList)
OBJECT_ENTRY(CLSID_ExplorerBand, CExplorerBand)
OBJECT_ENTRY(CLSID_FileSearchBand, CSearchBar)
OBJECT_ENTRY(CLSID_ProgressDialog, CProgressDialog)
OBJECT_ENTRY(CLSID_ISFBand, CISFBand)
OBJECT_ENTRY(CLSID_FindFolder, CFindFolder)
OBJECT_ENTRY(CLSID_UserAssist, CUserAssist)
END_OBJECT_MAP()

CBrowseUIModule                             gModule;
CAtlWinModule                               gWinModule;

/*************************************************************************
 * BROWSEUI DllMain
 */
STDAPI_(BOOL) DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%x %p\n", hInstance, dwReason, fImpLoad);

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        gModule.Init(ObjectMap, hInstance, NULL);
        DisableThreadLibraryCalls (hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        gModule.Term();
    }
    return TRUE;
}

/***********************************************************************
 *              DllCanUnloadNow (BROWSEUI.@)
 */
STDAPI DllCanUnloadNow()
{
    return gModule.DllCanUnloadNow();
}

/***********************************************************************
 *              DllGetClassObject (BROWSEUI.@)
 */
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    return gModule.DllGetClassObject(rclsid, riid, ppv);
}

/***********************************************************************
 *              DllRegisterServer (BROWSEUI.@)
 */
STDAPI DllRegisterServer()
{
    return gModule.DllRegisterServer(FALSE);
}

/***********************************************************************
 *              DllUnregisterServer (BROWSEUI.@)
 */
STDAPI DllUnregisterServer()
{
    return gModule.DllUnregisterServer(FALSE);
}

/***********************************************************************
 *              DllGetVersion (BROWSEUI.@)
 */
STDAPI DllGetVersion(DLLVERSIONINFO *info)
{
    if (info->cbSize != sizeof(DLLVERSIONINFO)) FIXME("support DLLVERSIONINFO2\n");

    /* this is what IE6 on Windows 98 reports */
    info->dwMajorVersion = 6;
    info->dwMinorVersion = 0;
    info->dwBuildNumber = 2600;
    info->dwPlatformID = DLLVER_PLATFORM_WINDOWS;

    return NOERROR;
}
