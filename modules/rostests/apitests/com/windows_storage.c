/*
 * PROJECT:         ReactOS API Tests
 * LICENSE:         MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:         COM interface test for windows.storage classes
 * COPYRIGHT:       Copyright 2025 Carl Bialorucki <carl.bialorucki@reactos.org>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces[] =
{
    {
        ID_NAME(CLSID_Internet, NTDDI_WIN10, NTDDI_MAX),
        {
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IObjectWithBackReferences },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IShellFolder2 },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IShellFolder },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IPersistFolder2 },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IPersistFolder },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IPersist },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IBrowserFrameOptions },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_MruLongList, NTDDI_WIN10, NTDDI_MAX),
        {
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_MruPidlList, NTDDI_WIN10, NTDDI_MAX),
        {
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_MyComputer, NTDDI_WIN10, NTDDI_MAX),
        {
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IPersistFolder2 },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IPersistFolder },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IPersist },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IPersistPropertyBag },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IItemNameLimits },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_INewItemAdvisor },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IContextMenuCB },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IFolderFilter },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IShellIcon },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IStorage },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IObjectWithBackReferences },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IShellFolder2 },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IShellFolder },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IShellIconOverlay },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IDelegateHostItemContainer },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IBackReferencedObject },
        }
    },
    {
        ID_NAME(CLSID_QueryAssociations, NTDDI_WIN10, NTDDI_MAX),
        {
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IQueryAssociations },
        }
    },
    {
        ID_NAME(CLSID_SharedTaskScheduler, NTDDI_WIN10, NTDDI_MAX),
        {
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IShellTaskScheduler },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IMarshal},
        }
    },
#if 0 // This crashes when un-initializing it. See ROSTESTS-405.
    {
        ID_NAME(CLSID_ShellDesktop, NTDDI_WIN10, NTDDI_MAX),
        {
            { NTDDI_WIN10,        NTDDI_MAX,          &CLSID_ShellDesktop },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IStorage },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IPersistFolder2 },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IPersistFolder },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IPersist },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IContextMenuCB },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IItemNameLimits },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IOleCommandTarget },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IObjectWithBackReferences },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IFolderFilter },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IShellIcon },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IShellFolder },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IShellFolder2 },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IShellIconOverlay },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IDelegateHostItemContainer },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IBackReferencedObject },
        }
    },
#endif
    {
        ID_NAME(CLSID_ShellFSFolder, NTDDI_WIN10, NTDDI_MAX),
        {
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IShellFolder2 },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IShellFolder },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IShellIcon },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IShellIconOverlay },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IPersistFolder3 },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IPersistFolder2 },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IPersistFolder },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IPersist },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IStorage },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IItemNameLimits },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IContextMenuCB },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IOleCommandTarget },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IPersistPropertyBag },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IParentAndItem },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IObjectWithBackReferences },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IRemoteComputer },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IFolderType },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IBackReferencedObject },
        }
    },
    {
        ID_NAME(CLSID_ShellItem, NTDDI_WIN10, NTDDI_MAX),
        {
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IMarshal },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IShellItem2 },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IShellItem },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IPersistIDList },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IParentAndItem },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IPersistStream },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IPersist },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IObjectWithBackReferences },
        },
        L"Both",
    },
    {
        ID_NAME(CLSID_ShellLink, NTDDI_WIN10, NTDDI_MAX),
        {
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IMarshal },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IShellLinkA },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IShellLinkW },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IPersistStream },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IPersistFile },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IPersist },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IShellExtInit },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IContextMenu3 },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IContextMenu2 },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IContextMenu },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IDropTarget },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IQueryInfo },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IShellLinkDataList },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IExtractIconA },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IExtractIconW },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IExtractImage2 },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IExtractImage },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IPersistPropertyBag },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IServiceProvider },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IPropertyStore },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_ICustomizeInfoTip },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_ISLTracker },
        },
        L"Both",
    },
    {
        ID_NAME(CLSID_ShellTaskScheduler, NTDDI_WIN10, NTDDI_MAX),
        {
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IShellTaskScheduler },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN10,        NTDDI_MAX,          &IID_IMarshal},
        },
    },
};

START_TEST(windows_storage)
{
    TestClassesEx(L"windows.storage",
                  ExpectedInterfaces, RTL_NUMBER_OF(ExpectedInterfaces),
                  NTDDI_WIN10, NTDDI_MAX,
                  TRUE);
}
