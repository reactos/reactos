/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         COM interface test for shdocvw classes
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces_WS03[] =
{
    {
        ID_NAME(CLSID_AdminFolderShortcut),
        {
            {    0x0,    0x0,   &IID_IShellFolder2 },
            {    0x0,    0x0,       &IID_IShellFolder },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IPersistFolder3 },
            {    0x4,    0x8,       &IID_IPersistFolder2 },
            {    0x4,    0x8,           &IID_IPersistFolder },
            {    0x4,    0x8,               &IID_IPersist },
            {    0x8,   0x10,   &IID_IShellLinkA },
            {    0xc,   0x18,   &IID_IShellLinkW },
            {   0x10,   0x20,   &IID_IPersistFile },
            {   0x14,   0x28,   &IID_IExtractIconW },
            {   0x18,   0x30,   &IID_IQueryInfo },
            {   0x20,   0x40,   &IID_IPersistStream },
            {   0x20,   0x40,   &IID_IPersistStreamInit },
            {   0x24,   0x48,   &IID_IPersistPropertyBag },
            {   0x28,   0x50,   &IID_IBrowserFrameOptions },
        }
    },
    {
        ID_NAME(CLSID_ExplorerBand),
        {
            {  -0xb4, -0x108,   &IID_IDeskBand },
            {  -0xb4, -0x108,       &IID_IDockingWindow },
            {  -0xb4, -0x108,           &IID_IOleWindow },
            {  -0xb0, -0x100,   &IID_IObjectWithSite },
            {  -0xa8,  -0xf0,   &IID_IInputObject },
            {  -0xa4,  -0xe8,   &IID_IPersistStream },
            {  -0xa4,  -0xe8,       &IID_IPersist },
            {  -0xa0,  -0xe0,   &IID_IOleCommandTarget },
            {  -0x9c,  -0xd8,   &IID_IServiceProvider },
            {  -0x84,  -0xb0,   &IID_IContextMenu },
            {  -0x80,  -0xa8,   &IID_IBandNavigate },
            {  -0x7c,  -0xa0,   &IID_IWinEventHandler },
            {  -0x78,  -0x98,   &IID_INamespaceProxy },
            {    0x0,    0x0,   &IID_IDispatch },
            {    0x0,    0x0,       &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_FontsFolderShortcut),
        {
            {    0x0,    0x0,   &IID_IShellFolder2 },
            {    0x0,    0x0,       &IID_IShellFolder },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IPersistFolder3 },
            {    0x4,    0x8,       &IID_IPersistFolder2 },
            {    0x4,    0x8,           &IID_IPersistFolder },
            {    0x4,    0x8,               &IID_IPersist },
            {    0x8,   0x10,   &IID_IShellLinkA },
            {    0xc,   0x18,   &IID_IShellLinkW },
            {   0x10,   0x20,   &IID_IPersistFile },
            {   0x14,   0x28,   &IID_IExtractIconW },
            {   0x18,   0x30,   &IID_IQueryInfo },
            {   0x20,   0x40,   &IID_IPersistStream },
            {   0x20,   0x40,   &IID_IPersistStreamInit },
            {   0x24,   0x48,   &IID_IPersistPropertyBag },
            {   0x28,   0x50,   &IID_IBrowserFrameOptions },
        }
    },
#if 0 // E_OUTOFMEMORY?
    {
        ID_NAME(CLSID_ShellDispatchInproc),
        {
            {    0x0,    0x0,   &IID_IUnknown },
        }
    },
#endif
    {
        ID_NAME(CLSID_MruLongList),
        {
            {    0x0,    0x0,   &IID_IMruDataList },
            {    0x0,    0x0,   &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_MruPidlList),
        {
            {    0x0,    0x0,   &IID_IMruPidlList },
            {    0x0,    0x0,   &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_SH_FavBand),
        {
            {  -0x30,  -0x58,   &IID_IDeskBand },
            {  -0x30,  -0x58,       &IID_IDockingWindow },
            {  -0x30,  -0x58,           &IID_IOleWindow },
            {  -0x2c,  -0x50,   &IID_IObjectWithSite },
            {  -0x24,  -0x40,   &IID_IInputObject },
            {  -0x20,  -0x38,   &IID_IPersistStream },
            {  -0x20,  -0x38,       &IID_IPersist },
            {  -0x1c,  -0x30,   &IID_IOleCommandTarget },
            {  -0x18,  -0x28,   &IID_IServiceProvider },
            {    0x0,    0x0,   &IID_IContextMenu },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IBandNavigate },
            {    0x8,   0x10,   &IID_IWinEventHandler },
            {    0xc,   0x18,   &IID_INamespaceProxy },
        }
    },
    {
        ID_NAME(CLSID_SH_HistBand),
        {
            {  -0xb4, -0x108,   &IID_IDeskBand },
            {  -0xb4, -0x108,       &IID_IDockingWindow },
            {  -0xb4, -0x108,           &IID_IOleWindow },
            {  -0xb0, -0x100,   &IID_IObjectWithSite },
            {  -0xa8,  -0xf0,   &IID_IInputObject },
            {  -0xa4,  -0xe8,   &IID_IPersistStream },
            {  -0xa4,  -0xe8,       &IID_IPersist },
            {  -0xa0,  -0xe0,   &IID_IOleCommandTarget },
            {  -0x9c,  -0xd8,   &IID_IServiceProvider },
            {  -0x84,  -0xb0,   &IID_IContextMenu },
            {  -0x80,  -0xa8,   &IID_IBandNavigate },
            {  -0x7c,  -0xa0,   &IID_IWinEventHandler },
            {  -0x78,  -0x98,   &IID_INamespaceProxy },
            {    0x0,    0x0,   &IID_IShellFolderSearchableCallback },
            {    0x0,    0x0,       &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_SearchAssistantOC),
        {
            {    0x0,    0x0,   &IID_ISearchAssistantOC3 },
            {    0x0,    0x0,       &IID_ISearchAssistantOC },
            {    0x0,    0x0,           &IID_IDispatch },
            {    0x0,    0x0,               &IID_IUnknown },
            {    0x4,    0x8,   &IID_IProvideClassInfo2 },
            {    0x4,    0x8,       &IID_IProvideClassInfo },
            {    0x8,   0x10,   &IID_IQuickActivate },
            {    0xc,   0x18,   &IID_IOleControl },
            {   0x10,   0x20,   &IID_IOleObject },
            {   0x14,   0x28,   &IID_IOleInPlaceActiveObject },
            {   0x18,   0x30,   &IID_IViewObjectEx },
            {   0x18,   0x30,       &IID_IViewObject2 },
            {   0x18,   0x30,           &IID_IViewObject },
            {   0x1c,   0x38,   &IID_IOleInPlaceObjectWindowless },
            {   0x1c,   0x38,       &IID_IOleInPlaceObject },
            {   0x1c,   0x38,           &IID_IOleWindow },
            {   0x20,   0x40,   &IID_IDataObject },
            {   0x30,   0x60,   &IID_IConnectionPointContainer },
            {   0x34,   0x68,   &IID_IObjectSafety },
            {   0x3c,   0x78,   &IID_IOleCommandTarget },
            {   0x40,   0x80,   &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_SearchBand),
        {
            {    0x0,    0x0,   &IID_IContextMenu },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellExtInit },
            {    0x8,   0x10,   &IID_IPersistPropertyBag },
            {    0x8,   0x10,       &IID_IPersist },
        }
    },
    {
        ID_NAME(CLSID_ShellSearchAssistantOC),
        {
            {    0x0,    0x0,   &IID_ISearchAssistantOC3 },
            {    0x0,    0x0,       &IID_ISearchAssistantOC },
            {    0x0,    0x0,           &IID_IDispatch },
            {    0x0,    0x0,               &IID_IUnknown },
            {    0x4,    0x8,   &IID_IProvideClassInfo2 },
            {    0x4,    0x8,       &IID_IProvideClassInfo },
            {    0x8,   0x10,   &IID_IQuickActivate },
            {    0xc,   0x18,   &IID_IOleControl },
            {   0x10,   0x20,   &IID_IOleObject },
            {   0x14,   0x28,   &IID_IOleInPlaceActiveObject },
            {   0x18,   0x30,   &IID_IViewObjectEx },
            {   0x18,   0x30,       &IID_IViewObject2 },
            {   0x18,   0x30,           &IID_IViewObject },
            {   0x1c,   0x38,   &IID_IOleInPlaceObjectWindowless },
            {   0x1c,   0x38,       &IID_IOleInPlaceObject },
            {   0x1c,   0x38,           &IID_IOleWindow },
            {   0x20,   0x40,   &IID_IDataObject },
            {   0x30,   0x60,   &IID_IConnectionPointContainer },
            {   0x34,   0x68,   &IID_IObjectSafety },
            {   0x3c,   0x78,   &IID_IOleCommandTarget },
            {   0x40,   0x80,   &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_ShellShellNameSpace),
        {
            {    0x0,    0x0,   &IID_IShellNameSpace },
            {    0x0,    0x0,       &IID_IShellFavoritesNameSpace },
            {    0x0,    0x0,           &IID_IDispatch },
            {    0x0,    0x0,               &IID_IUnknown },
            {    0x4,    0x8,   &IID_IProvideClassInfo2 },
            {    0x4,    0x8,       &IID_IProvideClassInfo },
            {    0x8,   0x10,   &IID_IPersistStreamInit },
            {    0xc,   0x18,   &IID_IPersistPropertyBag },
            {   0x10,   0x20,   &IID_IQuickActivate },
            {   0x14,   0x28,   &IID_IOleControl },
            {   0x18,   0x30,   &IID_IOleObject },
            {   0x1c,   0x38,   &IID_IOleInPlaceActiveObject },
            {   0x20,   0x40,   &IID_IViewObjectEx },
            {   0x20,   0x40,       &IID_IViewObject2 },
            {   0x20,   0x40,           &IID_IViewObject },
            {   0x24,   0x48,   &IID_IOleInPlaceObjectWindowless },
            {   0x24,   0x48,       &IID_IOleInPlaceObject },
            {   0x24,   0x48,           &IID_IOleWindow },
            {   0x28,   0x50,   &IID_ISpecifyPropertyPages },
            {   0x38,   0x70,   &IID_IConnectionPointContainer },
            {   0x3c,   0x78,   &IID_IShellChangeNotify },
            {   0x40,   0x80,   &IID_IDropTarget },
            {   0xb4,  0x118,   &IID_IObjectWithSite },
            {   0xbc,  0x128,   &IID_INSCTree2 },
            {   0xbc,  0x128,       &IID_INSCTree },
            {   0xc0,  0x130,   &IID_IWinEventHandler },
            {   0xc4,  0x138,   &IID_IShellBrowser },
            {   0xc8,  0x140,   &IID_IFolderFilterSite },
        }
    },
    {
        ID_NAME(CLSID_TaskbarList),
        {
            {    0x0,    0x0,   &IID_ITaskbarList2 },
            {    0x0,    0x0,       &IID_ITaskbarList },
            {    0x0,    0x0,           &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_AttachmentServices ),
        {
            {    0x0,    0x0,   &IID_IAttachmentExecute },
            {    0x0,    0x0,       &IID_IUnknown },
        }
    },
};

static const CLASS_AND_INTERFACES ExpectedInterfaces_Vista[] =
{
    {
        ID_NAME(CLSID_AdminFolderShortcut),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {    0xc,   0x18,   &IID_IObjectWithSite },
            {   0x14,   0x28,   &IID_IShellFolder2 },
            {   0x14,   0x28,       &IID_IShellFolder },
            {   0x18,   0x30,   &IID_IShellIcon },
            {   0x1c,   0x38,   &IID_IShellIconOverlay },
            {   0x20,   0x40,   &IID_IPersistFolder3 },
            {   0x20,   0x40,       &IID_IPersistFolder2 },
            {   0x20,   0x40,           &IID_IPersistFolder },
            {   0x20,   0x40,               &IID_IPersist },
            {   0x24,   0x48,   &IID_IStorage },
            {   0x28,   0x50,   &IID_ILocalizableItemParent },
            {   0x2c,   0x58,   &IID_IItemNameLimits },
            {   0x30,   0x60,   &IID_IContextMenuCB },
            {   0x3c,   0x78,   &IID_IOleCommandTarget },
            {   0x40,   0x80,   &IID_IPersistPropertyBag },
            {   0x50,   0xa0,   &IID_IObjectWithBackReferences },
            {   0x54,   0xa8,   &IID_IRemoteComputer },
            {   0x5c,   0xb8,   &IID_IFolderType },
            {   0x64,   0xc8,   &IID_IAliasRegistrationCallback },
            {   0x70,   0xe0,   &IID_IBackReferencedObject },
        }
    },
    {
        ID_NAME(CLSID_FontsFolderShortcut),
        {
            {    0x0,    0x0,   &IID_IShellFolder2 },
            {    0x0,    0x0,       &IID_IShellFolder },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IPersistFolder3 },
            {    0x4,    0x8,       &IID_IPersistFolder2 },
            {    0x4,    0x8,           &IID_IPersistFolder },
            {    0x4,    0x8,               &IID_IPersist },
            {    0x8,   0x10,   &IID_IShellLinkA },
            {    0xc,   0x18,   &IID_IShellLinkW },
            {   0x10,   0x20,   &IID_IPersistFile },
            {   0x14,   0x28,   &IID_IExtractIconW },
            {   0x18,   0x30,   &IID_IQueryInfo },
            {   0x1c,   0x38,   &IID_IPersistStream },
            {   0x1c,   0x38,   &IID_IPersistStreamInit },
            {   0x20,   0x40,   &IID_IPersistPropertyBag },
            {   0x24,   0x48,   &IID_IBrowserFrameOptions },
            {   0x2c,   0x58,   &IID_IFolderWithSearchRoot },
            {   0x34,   0x68,   &IID_IShellIconOverlay },

        }
    },
    {
        ID_NAME(CLSID_SearchBand),
        {
            {    0x0,    0x0,   &IID_IContextMenu },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellExtInit },
            {    0x8,   0x10,   &IID_IPersistPropertyBag },
            {    0x8,   0x10,       &IID_IPersist },
            {    0xc,   0x18,   &IID_IShellFolder },
        }
    },
    {
        ID_NAME(CLSID_ShellShellNameSpace),
        {
            {    0x0,    0x0,   &IID_IShellNameSpace },
            {    0x0,    0x0,       &IID_IShellFavoritesNameSpace },
            {    0x0,    0x0,           &IID_IDispatch },
            {    0x0,    0x0,               &IID_IUnknown },
            {    0x4,    0x8,   &IID_IProvideClassInfo2 },
            {    0x4,    0x8,       &IID_IProvideClassInfo },
            {    0x8,   0x10,   &IID_IPersistStreamInit },
            {   0x10,   0x20,   &IID_IPersistPropertyBag },
            {   0x14,   0x28,   &IID_IQuickActivate },
            {   0x18,   0x30,   &IID_IOleControl },
            {   0x1c,   0x38,   &IID_IOleObject },
            {   0x20,   0x40,   &IID_IOleInPlaceActiveObject },
            {   0x24,   0x48,   &IID_IViewObjectEx },
            {   0x24,   0x48,       &IID_IViewObject2 },
            {   0x24,   0x48,           &IID_IViewObject },
            {   0x28,   0x50,   &IID_IOleInPlaceObjectWindowless },
            {   0x28,   0x50,       &IID_IOleInPlaceObject },
            {   0x28,   0x50,           &IID_IOleWindow },
            {   0x38,   0x70,   &IID_IConnectionPointContainer },
            {   0x3c,   0x78,   &IID_IDropTarget },
            {   0xb4,  0x118,   &IID_IObjectWithSite },
            {   0xbc,  0x128,   &IID_INSCTree2 },
            {   0xbc,  0x128,       &IID_INSCTree },
            {   0xc0,  0x130,   &IID_IWinEventHandler },
            {   0xc4,  0x138,   &IID_IShellBrowser },
            {   0xc8,  0x140,   &IID_IFolderFilterSite },
            {   0xcc,  0x148,   &IID_INewMenuClient },
            {   0xd0,  0x150,   &IID_IServiceProvider },
            {   0xd4,  0x158,   &IID_INameSpaceTreeControl },
            {   0xe4,  0x178,   &IID_IVisualProperties }
        }
    },
    {
        ID_NAME(CLSID_TaskbarList),
        {
            {    0x0,    0x0,   &IID_ITaskbarList2 },
            {    0x0,    0x0,       &IID_ITaskbarList },
            {    0x0,    0x0,           &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_AttachmentServices ),
        {
            {    0x0,    0x0,   &IID_IAttachmentExecute },
            {    0x0,    0x0,       &IID_IUnknown },
        }
    },
};

static const CLASS_AND_INTERFACES ExpectedInterfaces_Win7[] =
{
    {
        ID_NAME(CLSID_AdminFolderShortcut),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {    0xc,   0x18,   &IID_IObjectWithSite },
            {   0x14,   0x28,   &IID_IShellFolder2 },
            {   0x14,   0x28,       &IID_IShellFolder },
            {   0x18,   0x30,   &IID_IShellIcon },
            {   0x1c,   0x38,   &IID_IShellIconOverlay },
            {   0x20,   0x40,   &IID_IPersistFolder3 },
            {   0x20,   0x40,       &IID_IPersistFolder2 },
            {   0x20,   0x40,           &IID_IPersistFolder },
            {   0x20,   0x40,               &IID_IPersist },
            {   0x24,   0x48,   &IID_IStorage },
            {   0x28,   0x50,   &IID_ILocalizableItemParent },
            {   0x2c,   0x58,   &IID_IItemNameLimits },
            {   0x30,   0x60,   &IID_IContextMenuCB },
            {   0x3c,   0x78,   &IID_IOleCommandTarget },
            {   0x40,   0x80,   &IID_IPersistPropertyBag },
            {   0x44,   0x88,   &IID_IParentAndItem },
            {   0x50,   0xa0,   &IID_IObjectWithBackReferences },
            {   0x54,   0xa8,   &IID_IRemoteComputer },
            {   0x60,   0xc0,   &IID_IFolderType },
            {   0x70,   0xe0,   &IID_IBackReferencedObject },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_SearchBand),
        {
            {    0x0,    0x0,   &IID_IContextMenu },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellExtInit },
            {    0x8,   0x10,   &IID_IPersistPropertyBag },
            {    0x8,   0x10,       &IID_IPersist },
            {    0xc,   0x18,   &IID_IShellFolder },
        }
    },
    {
        ID_NAME(CLSID_AttachmentServices ),
        {
            {    0x0,    0x0,   &IID_IAttachmentExecute },
            {    0x0,    0x0,       &IID_IUnknown },
        }
    },
};

static const CLASS_AND_INTERFACES ExpectedInterfaces_Win8[] =
{
    {
        ID_NAME(CLSID_AdminFolderShortcut),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {    0xc,   0x18,   &IID_IObjectWithSite },
            {   0x14,   0x28,   &IID_IShellFolder2 },
            {   0x14,   0x28,       &IID_IShellFolder },
            {   0x18,   0x30,   &IID_IShellIcon },
            {   0x1c,   0x38,   &IID_IShellIconOverlay },
            {   0x20,   0x40,   &IID_IPersistFolder3 },
            {   0x20,   0x40,       &IID_IPersistFolder2 },
            {   0x20,   0x40,           &IID_IPersistFolder },
            {   0x20,   0x40,               &IID_IPersist },
            {   0x24,   0x48,   &IID_IStorage },
            {   0x2c,   0x58,   &IID_IItemNameLimits },
            {   0x30,   0x60,   &IID_IContextMenuCB },
            {   0x3c,   0x78,   &IID_IOleCommandTarget },
            {   0x40,   0x80,   &IID_IPersistPropertyBag },
            {   0x44,   0x88,   &IID_IParentAndItem },
            {   0x50,   0xa0,   &IID_IObjectWithBackReferences },
            {   0x54,   0xa8,   &IID_IRemoteComputer },
            {   0x60,   0xc0,   &IID_IFolderType },
            {   0x70,   0xe0,   &IID_IBackReferencedObject },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_AttachmentServices ),
        {
            {    0x0,    0x0,   &IID_IAttachmentExecute },
            {    0x0,    0x0,       &IID_IUnknown },
        }
    },
};

START_TEST(shdocvw)
{
    if (GetNTVersion() <= _WIN32_WINNT_WS03)
        TestClasses(L"shdocvw", ExpectedInterfaces_WS03, RTL_NUMBER_OF(ExpectedInterfaces_WS03));
    else if (GetNTVersion() <= _WIN32_WINNT_VISTA)
        TestClasses(L"shdocvw", ExpectedInterfaces_Vista, RTL_NUMBER_OF(ExpectedInterfaces_Vista));
    else if (GetNTVersion() <= _WIN32_WINNT_WIN7)
        TestClasses(L"shdocvw", ExpectedInterfaces_Win7, RTL_NUMBER_OF(ExpectedInterfaces_Win7));
    else
        TestClasses(L"shdocvw", ExpectedInterfaces_Win8, RTL_NUMBER_OF(ExpectedInterfaces_Win8));
}
