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

class CBrowseUIModule : public CComModule
{
public:
};


BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_AutoComplete, CAutoComplete)
OBJECT_ENTRY(CLSID_ACLMulti, CACLMulti)
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
OBJECT_ENTRY(CLSID_ExplorerBand, CExplorerBand)
OBJECT_ENTRY(CLSID_ProgressDialog, CProgressDialog)
END_OBJECT_MAP()

CBrowseUIModule                             gModule;
CAtlWinModule                               gWinModule;

void *operator new (size_t, void *buf)
{
    return buf;
}

/*************************************************************************
 * BROWSEUI DllMain
 */
STDAPI_(BOOL) DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%x %p\n", hInstance, dwReason, fImpLoad);

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        /* HACK - the global constructors don't run, so I placement new them here */
        new (&gModule) CBrowseUIModule;
        new (&gWinModule) CAtlWinModule;
        new (&_AtlBaseModule) CAtlBaseModule;
        new (&_AtlComModule) CAtlComModule;

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
