/*
 * PROJECT:         ReactOS API Tests
 * LICENSE:         MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:         COM interface test for explorerframe server
 * COPYRIGHT:       Copyright 2025 Carl Bialorucki <carl.bialorucki@reactos.org>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces[] =
{
    {
        ID_NAME(CLSID_BandSiteMenu, NTDDI_WIN7, NTDDI_MAX),
        {
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IContextMenu3 },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IContextMenu2 },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IContextMenu },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IShellService },
        }
    },
    {
        ID_NAME(CLSID_CDockingBarPropertyBag, NTDDI_WIN7, NTDDI_MAX),
        {
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IPropertyBag },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IUnknown },
        },
    },
    {
        ID_NAME(CLSID_DeskBarApp, NTDDI_WIN7, NTDDI_MAX),
        {
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IOleCommandTarget },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IServiceProvider },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IDeskBar },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IOleWindow },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IInputObjectSite },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IInputObject },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IDockingWindow },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IPersistStreamInit },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IPersistStream },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IPersist },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IPersistPropertyBag },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IContextMenu3 },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IContextMenu2 },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IContextMenu },
        }
    },
    {
        ID_NAME(CLSID_GlobalFolderSettings, NTDDI_WIN7, NTDDI_MAX),
        {
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IGlobalFolderSettings },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IUnknown },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_InternetToolbar, NTDDI_WIN7, NTDDI_MAX),
        {
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IOleCommandTarget },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IServiceProvider },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IDeskBar },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IOleWindow },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IInputObjectSite },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IInputObject },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IDockingWindow },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IPersistStreamInit },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IBandHost },
        }
    },
    {
        ID_NAME(CLSID_RebarBandSite, NTDDI_WIN7, NTDDI_MAX),
        {
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IBandSite },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IInputObjectSite },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IInputObject },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IDeskBarClient },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IOleWindow },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IWinEventHandler },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IPersistStream },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IPersist },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IDropTarget },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IServiceProvider },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IBandSiteHelper },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IOleCommandTarget },
        }
    },
    {
        ID_NAME(CLSID_SH_AddressBand, NTDDI_WIN7, NTDDI_MAX),
        {
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IDeskBand },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IDockingWindow },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IOleWindow },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IInputObject },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IPersistStream },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IPersist },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IOleCommandTarget },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IServiceProvider },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IWinEventHandler },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IInputObjectSite },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IShellChangeNotify },
        }
    },
    {
        ID_NAME(CLSID_TaskbarList, NTDDI_WIN7, NTDDI_MAX),
        {
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_ITaskbarList3 },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_ITaskbarList4 },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_ITaskbarList2 },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_ITaskbarList },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IUnknown },
        }
    },
};

START_TEST(explorerframe)
{
    TestClassesEx(L"explorerframe",
                  ExpectedInterfaces, RTL_NUMBER_OF(ExpectedInterfaces),
                  NTDDI_WIN7, NTDDI_MAX,
                  FALSE);
}
