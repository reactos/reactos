/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         COM interface test for browseui classes
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces[] =
{
    {
        ID_NAME(CLSID_ACLCustomMRU),
        {
            {    0x0,    0x0,   &IID_IEnumString },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IACList },
            {    0x8,   0x10,   &IID_IACLCustomMRU },
        },
    },
    {
        ID_NAME(CLSID_ACLHistory),
        {
            {    0x0,    0x0,   &IID_IEnumString },
            {    0x0,    0x0,       &IID_IUnknown },
        },
    },
    {
        ID_NAME(CLSID_ACLMRU),
        {
            {    0x0,    0x0,   &IID_IEnumString },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IACList },
            {    0x8,   0x10,   &IID_IACLCustomMRU },
        },
    },
    {
        ID_NAME(CLSID_ACLMulti),
        {
            {    0x0,    0x0,   &IID_IEnumString },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IObjMgr },
            {    0x8,   0x10,   &IID_IACList },
        }
    },
    {
        ID_NAME(CLSID_ACListISF),
        {
            {    0x0,    0x0,   &IID_IEnumString },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IACList2 },
            {    0x4,    0x8,       &IID_IACList },
            {    0x8,   0x10,   &IID_ICurrentWorkingDirectory },
            {    0xc,   0x18,   &IID_IShellService },
            {   0x10,   0x20,   &IID_IPersistFolder },
        }
    },
    {
        ID_NAME(CLSID_AddressEditBox),
        {
            {    0x0,    0x0,   &IID_IWinEventHandler },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IDispatch },
            {    0x8,   0x10,   &IID_IAddressBand },
            {    0xc,   0x18,   &IID_IAddressEditBox },
            {   0x10,   0x20,   &IID_IOleCommandTarget },
            {   0x14,   0x28,   &IID_IPersistStream },
            {   0x18,   0x30,   &IID_IShellService },
        }
    },
    {
        ID_NAME(CLSID_AugmentedShellFolder),
        {
            {    0x0,    0x0,   &IID_IAugmentedShellFolder2 },
            {    0x0,    0x0,       &IID_IAugmentedShellFolder },
            {    0x0,    0x0,           &IID_IShellFolder },
            {    0x0,    0x0,               &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellService },
            {    0x8,   0x10,   &IID_ITranslateShellChangeNotify },
        }
    },
    {
        ID_NAME(CLSID_AugmentedShellFolder2),
        {
            {    0x0,    0x0,   &IID_IAugmentedShellFolder2 },
            {    0x0,    0x0,       &IID_IAugmentedShellFolder },
            {    0x0,    0x0,           &IID_IShellFolder },
            {    0x0,    0x0,               &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellFolder2 },
            {    0x8,   0x10,   &IID_IShellService },
            {    0xc,   0x18,   &IID_ITranslateShellChangeNotify },
            {   0x10,   0x20,   &IID_IDropTarget },
        }
    },
    {
        ID_NAME(CLSID_AutoComplete),
        {
            {    0x0,    0x0,   &IID_IAutoComplete2 },
            {    0x0,    0x0,       &IID_IAutoComplete },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IAutoCompleteDropDown },
            {    0x8,   0x10,   &IID_IEnumString },
            {    0xc,   0x18,   &IID_IAccessible },
            {    0xc,   0x18,       &IID_IDispatch },
        }
    },
    {
        ID_NAME(CLSID_BackgroundTaskScheduler),
        {
            {    0x0,    0x0,   &IID_IShellTaskScheduler },
            {    0x0,    0x0,       &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_BandProxy),
        {
            {    0x0,    0x0,   &IID_IBandProxy },
            {    0x0,    0x0,       &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_BandSiteMenu),
        {
            {    0x0,    0x0,   &IID_IContextMenu3 },
            {    0x0,    0x0,       &IID_IContextMenu2 },
            {    0x0,    0x0,           &IID_IContextMenu },
            {    0x0,    0x0,               &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellService },
        }
    },
    {
        ID_NAME(CLSID_BrandBand),
        {
            {    0x0,    0x0,   &IID_IDeskBand },
            {    0x0,    0x0,       &IID_IDockingWindow },
            {    0x0,    0x0,           &IID_IOleWindow },
            {    0x0,    0x0,               &IID_IUnknown },
            {    0x4,    0x8,   &IID_IObjectWithSite },
            {    0xc,   0x18,   &IID_IInputObject },
            {   0x10,   0x20,   &IID_IPersistStream },
            {   0x10,   0x20,       &IID_IPersist },
            {   0x14,   0x28,   &IID_IOleCommandTarget },
            {   0x18,   0x30,   &IID_IServiceProvider },
            {   0x30,   0x60,   &IID_IWinEventHandler },
            {   0x34,   0x68,   &IID_IDispatch },
        }
    },
    {
        ID_NAME(CLSID_BrowserBand),
        {
            {  -0x30,  -0x60,   &IID_IDeskBand },
            {  -0x30,  -0x60,       &IID_IDockingWindow },
            {  -0x30,  -0x60,           &IID_IOleWindow },
            {  -0x2c,  -0x58,   &IID_IObjectWithSite },
            {  -0x24,  -0x48,   &IID_IInputObject },
            {  -0x20,  -0x40,   &IID_IPersistStream },
            {  -0x20,  -0x40,       &IID_IPersist },
            {  -0x1c,  -0x38,   &IID_IOleCommandTarget },
            {  -0x18,  -0x30,   &IID_IServiceProvider },
            {    0x0,    0x0,   &IID_IContextMenu },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IWinEventHandler },
            {    0x8,   0x10,   &IID_IDispatch },
            {   0x10,   0x20,   &IID_IPersistPropertyBag },
        },
    },
    {
        ID_NAME(CLSID_CCommonBrowser),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {    0xc,   0x18,   &IID_IShellBrowser },
            {    0xc,   0x18,       &IID_IOleWindow },
            {   0x10,   0x20,   &IID_IBrowserService3 },
            {   0x10,   0x20,       &IID_IBrowserService2 },
            {   0x10,   0x20,           &IID_IBrowserService },
            {   0x14,   0x28,   &IID_IServiceProvider },
            {   0x18,   0x30,   &IID_IOleCommandTarget },
            {   0x1c,   0x38,   &IID_IDockingWindowSite },
            {   0x20,   0x40,   &IID_IDockingWindowFrame },
            {   0x24,   0x48,   &IID_IInputObjectSite },
            {   0x28,   0x50,   &IID_IDropTarget },
            {   0x2c,   0x58,   &IID_IShellBrowserService },
        }
    },
    {
        ID_NAME(CLSID_CDockingBarPropertyBag),
        {
            {    0x0,    0x0,   &IID_IPropertyBag },
            {    0x0,    0x0,       &IID_IUnknown },
        },
    },
    {
        ID_NAME(CLSID_CRegTreeOptions),
        {
            {    0x0,    0x0,   &IID_IRegTreeOptions },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_CommBand),
        {
            {  -0x30,  -0x60,   &IID_IDeskBand },
            {  -0x30,  -0x60,       &IID_IDockingWindow },
            {  -0x30,  -0x60,           &IID_IOleWindow },
            {  -0x2c,  -0x58,   &IID_IObjectWithSite },
            {  -0x24,  -0x48,   &IID_IInputObject },
            {  -0x20,  -0x40,   &IID_IPersistStream },
            {  -0x20,  -0x40,       &IID_IPersist },
            {  -0x1c,  -0x38,   &IID_IOleCommandTarget },
            {  -0x18,  -0x30,   &IID_IServiceProvider },
            {    0x0,    0x0,   &IID_IContextMenu },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IWinEventHandler },
            {    0x8,   0x10,   &IID_IDispatch },
            {   0x10,   0x20,   &IID_IPersistPropertyBag },
        },
    },
    {
        ID_NAME(CLSID_DeskBar),
        {
            {  -0xb8, -0x170,   &IID_IOleCommandTarget },
            {  -0xb4, -0x168,   &IID_IServiceProvider },
            {  -0xb0, -0x160,   &IID_IDeskBar },
            {  -0xb0, -0x160,       &IID_IOleWindow },
            {  -0xac, -0x158,   &IID_IInputObjectSite },
            {  -0xa8, -0x150,   &IID_IInputObject },
            {  -0x70,  -0xe0,   &IID_IDockingWindow },
            {  -0x6c,  -0xd8,   &IID_IObjectWithSite },
            {  -0x68,  -0xd0,   &IID_IPersistStreamInit },
            {  -0x68,  -0xd0,   &IID_IPersistStream },
            {  -0x68,  -0xd0,       &IID_IPersist },
            {  -0x64,  -0xc8,   &IID_IPersistPropertyBag },
            {    0x0,    0x0,   &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_DeskBarApp),
        {
            {  -0xb8, -0x170,   &IID_IOleCommandTarget },
            {  -0xb4, -0x168,   &IID_IServiceProvider },
            {  -0xb0, -0x160,   &IID_IDeskBar },
            {  -0xb0, -0x160,       &IID_IOleWindow },
            {  -0xac, -0x158,   &IID_IInputObjectSite },
            {  -0xa8, -0x150,   &IID_IInputObject },
            {  -0x70,  -0xe0,   &IID_IDockingWindow },
            {  -0x6c,  -0xd8,   &IID_IObjectWithSite },
            {  -0x68,  -0xd0,   &IID_IPersistStreamInit },
            {  -0x68,  -0xd0,   &IID_IPersistStream },
            {  -0x68,  -0xd0,       &IID_IPersist },
            {  -0x64,  -0xc8,   &IID_IPersistPropertyBag },
            {    0x0,    0x0,   &IID_IUnknown },
            {    0x8,   0x10,   &IID_IContextMenu3 },
            {    0x8,   0x10,       &IID_IContextMenu2 },
            {    0x8,   0x10,           &IID_IContextMenu },
        }
    },
    {
        ID_NAME(CLSID_GlobalFolderSettings),
        {
            {    0x0,    0x0,   &IID_IGlobalFolderSettings },
            {    0x0,    0x0,       &IID_IUnknown },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_ImageListCache),
        {
            {    0x0,    0x0,   &IID_IUnknown },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_ImgCtxThumbnailExtractor),
        {
            {    0x0,    0x0,   &IID_IExtractImage2 },
            {    0x0,    0x0,       &IID_IExtractImage },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x8,   0x10,   &IID_IPersistFile },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_InternetToolbar),
        {
            {  -0x54,  -0xa8,   &IID_IOleCommandTarget },
            {  -0x50,  -0xa0,   &IID_IServiceProvider },
            {  -0x4c,  -0x98,   &IID_IDeskBar },
            {  -0x4c,  -0x98,       &IID_IOleWindow },
            {  -0x48,  -0x90,   &IID_IInputObjectSite },
            {  -0x44,  -0x88,   &IID_IInputObject },
            {   -0xc,  -0x18,   &IID_IDockingWindow },
            {   -0x8,  -0x10,   &IID_IObjectWithSite },
            {   -0x4,   -0x8,   &IID_IExplorerToolbar },
            {    0x0,    0x0,   &DIID_DWebBrowserEvents },
            {    0x0,    0x0,   &IID_IDispatch },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IPersistStreamInit },
            {    0x8,   0x10,   &IID_IShellChangeNotify },
        }
    },
    {
        ID_NAME(CLSID_ProgressDialog),
        {
            {    0x0,    0x0,   &IID_IProgressDialog },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IOleWindow },
            {    0x8,   0x10,   &IID_IActionProgressDialog },
            {    0xc,   0x18,   &IID_IActionProgress },
            {   0x10,   0x20,   &IID_IObjectWithSite },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_RebarBandSite),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {    0xc,   0x18,   &IID_IBandSite },
            {   0x10,   0x20,   &IID_IInputObjectSite },
            {   0x14,   0x28,   &IID_IInputObject },
            {   0x18,   0x30,   &IID_IDeskBarClient },
            {   0x18,   0x30,       &IID_IOleWindow },
            {   0x1c,   0x38,   &IID_IWinEventHandler },
            {   0x20,   0x40,   &IID_IPersistStream },
            {   0x20,   0x40,       &IID_IPersist },
            {   0x24,   0x48,   &IID_IDropTarget },
            {   0x28,   0x50,   &IID_IServiceProvider },
            {   0x2c,   0x58,   &IID_IBandSiteHelper },
            {   0x30,   0x60,   &IID_IOleCommandTarget },
        }
    },
    {
        ID_NAME(CLSID_SH_AddressBand),
        {
            {    0x0,    0x0,   &IID_IDeskBand },
            {    0x0,    0x0,       &IID_IDockingWindow },
            {    0x0,    0x0,           &IID_IOleWindow },
            {    0x0,    0x0,               &IID_IUnknown },
            {    0x4,    0x8,   &IID_IObjectWithSite },
            {    0xc,   0x18,   &IID_IInputObject },
            {   0x10,   0x20,   &IID_IPersistStream },
            {   0x10,   0x20,       &IID_IPersist },
            {   0x14,   0x28,   &IID_IOleCommandTarget },
            {   0x18,   0x30,   &IID_IServiceProvider },
            {   0x30,   0x60,   &IID_IWinEventHandler },
            {   0x34,   0x68,   &IID_IAddressBand },
            {   0x38,   0x70,   &IID_IInputObjectSite },
        }
    },
    {
        ID_NAME(CLSID_SH_SearchBand),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {   0x74,   0xe8,   &IID_IOleContainer },
            {   0x78,   0xf0,   &IID_IOleInPlaceFrame },
            //{   0x78,   0xf0,       &IID_IOleInPlaceUIWindow },
            {   0x78,   0xf0,           &IID_IOleWindow },
            {   0xb0,  0x160,   &IID_IDeskBand },
            {   0xb0,  0x160,       &IID_IDockingWindow },
            {   0xb4,  0x168,   &IID_IInputObject },
            {   0xb8,  0x170,   &IID_IObjectWithSite },
            {   0xbc,  0x178,   &IID_IContextMenu },
            {   0xc0,  0x180,   &IID_IServiceProvider },
            {   0xc8,  0x190,   &IID_ISearchBar },
            {   0xcc,  0x198,   &IID_IFileSearchBand },
            {   0xcc,  0x198,       &IID_IDispatch },
            {   0xd0,  0x1a0,   &IID_IBandNavigate },
            {   0xd8,  0x1b0,   &IID_IPersist },
        }
    },
    {
        ID_NAME(CLSID_SharedTaskScheduler),
        {
            {    0x0,    0x0,   &IID_IShellTaskScheduler },
            {    0x0,    0x0,       &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_ShellSearchExt),
        {
            {    0x0,    0x0,   &IID_IContextMenu },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_ShellTaskScheduler),
        {
            {    0x0,    0x0,   &IID_IShellTaskScheduler },
            {    0x0,    0x0,       &IID_IUnknown },
        },
    },
    {
        ID_NAME(CLSID_UserAssist),
        {
            {    0x0,    0x0,   &IID_IUnknown },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_WebSearchExt),
        {
            {    0x0,    0x0,   &IID_IContextMenu },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IObjectWithSite },
        },
    },
};

START_TEST(browseui)
{
    TestClasses(L"browseui", ExpectedInterfaces, RTL_NUMBER_OF(ExpectedInterfaces));
}
