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
        ID_NAME(CLSID_ACLMulti),
        {
            {    0x0,   &IID_IEnumString },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_IObjMgr },
            {    0x8,   &IID_IACList },
        }
    },
    {
        ID_NAME(CLSID_ACListISF),
        {
            {    0x0,   &IID_IEnumString },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_IACList2 },
            {    0x4,       &IID_IACList },
            {    0xc,   &IID_IShellService },
            {   0x10,   &IID_IPersistFolder },
        }
    },
    {
        ID_NAME(CLSID_AddressEditBox),
        {
            {    0x0,   &IID_IWinEventHandler },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_IDispatch },
            {    0x8,   &IID_IAddressBand },
            {    0xc,   &IID_IAddressEditBox },
            {   0x10,   &IID_IOleCommandTarget },
            {   0x14,   &IID_IPersistStream },
            {   0x18,   &IID_IShellService },
        }
    },
    {
        ID_NAME(CLSID_AugmentedShellFolder),
        {
            {    0x0,   &IID_IAugmentedShellFolder2 },
            {    0x0,       &IID_IAugmentedShellFolder },
            {    0x0,           &IID_IShellFolder },
            {    0x0,               &IID_IUnknown },
            {    0x4,   &IID_IShellService },
        }
    },
    {
        ID_NAME(CLSID_AugmentedShellFolder2),
        {
            {    0x0,   &IID_IAugmentedShellFolder2 },
            {    0x0,       &IID_IAugmentedShellFolder },
            {    0x0,           &IID_IShellFolder },
            {    0x0,               &IID_IUnknown },
            {    0x4,   &IID_IShellFolder2 },
            {    0x8,   &IID_IShellService },
            {   0x10,   &IID_IDropTarget },
        }
    },
    {
        ID_NAME(CLSID_AutoComplete),
        {
            {    0x0,   &IID_IAutoComplete2 },
            {    0x0,       &IID_IAutoComplete },
            {    0x0,           &IID_IUnknown },
            {    0x8,   &IID_IEnumString },
            {    0xc,   &IID_IAccessible },
            {    0xc,       &IID_IDispatch },
        }
    },
    {
        ID_NAME(CLSID_BandProxy),
        {
            {    0x0,   &IID_IBandProxy },
            {    0x0,       &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_BrandBand),
        {
            {    0x0,   &IID_IDeskBand },
            {    0x0,       &IID_IDockingWindow },
            {    0x0,           &IID_IOleWindow },
            {    0x0,               &IID_IUnknown },
            {    0x4,   &IID_IObjectWithSite },
            {    0xc,   &IID_IInputObject },
            {   0x10,   &IID_IPersistStream },
            {   0x10,       &IID_IPersist },
            {   0x14,   &IID_IOleCommandTarget },
            {   0x18,   &IID_IServiceProvider },
            {   0x30,   &IID_IWinEventHandler },
            {   0x34,   &IID_IDispatch },
        }
    },
    {
        ID_NAME(CLSID_BandSiteMenu),
        {
            {    0x0,   &IID_IContextMenu3 },
            {    0x0,       &IID_IContextMenu2 },
            {    0x0,           &IID_IContextMenu },
            {    0x0,               &IID_IUnknown },
            {    0x4,   &IID_IShellService },
        }
    },
    {
        ID_NAME(CLSID_CCommonBrowser),
        {
            {    0x0,   &IID_IUnknown },
            {    0xc,   &IID_IShellBrowser },
            {    0xc,       &IID_IOleWindow },
            {   0x10,   &IID_IBrowserService3 },
            {   0x10,       &IID_IBrowserService2 },
            {   0x10,           &IID_IBrowserService },
            {   0x14,   &IID_IServiceProvider },
            {   0x18,   &IID_IOleCommandTarget },
            {   0x1c,   &IID_IDockingWindowSite },
            {   0x20,   &IID_IDockingWindowFrame },
            {   0x24,   &IID_IInputObjectSite },
            {   0x28,   &IID_IDropTarget },
            {   0x2c,   &IID_IShellBrowserService },
        }
    },
    {
        ID_NAME(CLSID_CRegTreeOptions),
        {
            {    0x0,   &IID_IRegTreeOptions },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_DeskBar),
        {
            {  -0xb8,   &IID_IOleCommandTarget },
            {  -0xb4,   &IID_IServiceProvider },
            {  -0xb0,   &IID_IDeskBar },
            {  -0xb0,       &IID_IOleWindow },
            {  -0xac,   &IID_IInputObjectSite },
            {  -0xa8,   &IID_IInputObject },
            {  -0x70,   &IID_IDockingWindow },
            {  -0x6c,   &IID_IObjectWithSite },
            {  -0x68,   &IID_IPersistStreamInit },
            {  -0x68,   &IID_IPersistStream },
            {  -0x68,       &IID_IPersist },
            {  -0x64,   &IID_IPersistPropertyBag },
            {    0x0,   &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_DeskBarApp),
        {
            {  -0xb8,   &IID_IOleCommandTarget },
            {  -0xb4,   &IID_IServiceProvider },
            {  -0xb0,   &IID_IDeskBar },
            {  -0xb0,       &IID_IOleWindow },
            {  -0xac,   &IID_IInputObjectSite },
            {  -0xa8,   &IID_IInputObject },
            {  -0x70,   &IID_IDockingWindow },
            {  -0x6c,   &IID_IObjectWithSite },
            {  -0x68,   &IID_IPersistStreamInit },
            {  -0x68,   &IID_IPersistStream },
            {  -0x68,       &IID_IPersist },
            {  -0x64,   &IID_IPersistPropertyBag },
            {    0x0,   &IID_IUnknown },
            {    0x8,   &IID_IContextMenu3 },
            {    0x8,       &IID_IContextMenu2 },
            {    0x8,           &IID_IContextMenu },
        }
    },
    {
        ID_NAME(CLSID_GlobalFolderSettings),
        {
            {    0x0,   &IID_IGlobalFolderSettings },
            {    0x0,       &IID_IUnknown },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_InternetToolbar),
        {
            {  -0x54,   &IID_IOleCommandTarget },
            {  -0x50,   &IID_IServiceProvider },
            {  -0x4c,   &IID_IDeskBar },
            {  -0x4c,       &IID_IOleWindow },
            {  -0x48,   &IID_IInputObjectSite },
            {  -0x44,   &IID_IInputObject },
            {   -0xc,   &IID_IDockingWindow },
            {   -0x8,   &IID_IObjectWithSite },
            {   -0x4,   &IID_IExplorerToolbar },
            {    0x0,   &IID_IDispatch },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_IPersistStreamInit },
            {    0x8,   &IID_IShellChangeNotify },
        }
    },
    {
        ID_NAME(CLSID_RebarBandSite),
        {
            {    0x0,   &IID_IUnknown },
            {    0xc,   &IID_IBandSite },
            {   0x10,   &IID_IInputObjectSite },
            {   0x14,   &IID_IInputObject },
            {   0x18,   &IID_IDeskBarClient },
            {   0x18,       &IID_IOleWindow },
            {   0x1c,   &IID_IWinEventHandler },
            {   0x20,   &IID_IPersistStream },
            {   0x20,       &IID_IPersist },
            {   0x24,   &IID_IDropTarget },
            {   0x28,   &IID_IServiceProvider },
            {   0x2c,   &IID_IBandSiteHelper },
            {   0x30,   &IID_IOleCommandTarget },
        }
    },
    {
        ID_NAME(CLSID_SH_AddressBand),
        {
            {    0x0,   &IID_IDeskBand },
            {    0x0,       &IID_IDockingWindow },
            {    0x0,           &IID_IOleWindow },
            {    0x0,               &IID_IUnknown },
            {    0x4,   &IID_IObjectWithSite },
            {    0xc,   &IID_IInputObject },
            {   0x10,   &IID_IPersistStream },
            {   0x10,       &IID_IPersist },
            {   0x14,   &IID_IOleCommandTarget },
            {   0x18,   &IID_IServiceProvider },
            {   0x30,   &IID_IWinEventHandler },
            {   0x34,   &IID_IAddressBand },
            {   0x38,   &IID_IInputObjectSite },
        }
    },
    {
        ID_NAME(CLSID_ShellSearchExt),
        {
            {    0x0,   &IID_IContextMenu },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_ProgressDialog),
        {
            {    0x0,   &IID_IProgressDialog },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_IOleWindow },
            {   0x10,   &IID_IObjectWithSite },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_SharedTaskScheduler),
        {
            {    0x0,   &IID_IShellTaskScheduler },
            {    0x0,       &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_BackgroundTaskScheduler),
        {
            {    0x0,   &IID_IShellTaskScheduler },
            {    0x0,       &IID_IUnknown },
        }
    }
};
static const INT ExpectedInterfaceCount = RTL_NUMBER_OF(ExpectedInterfaces);

START_TEST(browseui)
{
    TestClasses(L"browseui", ExpectedInterfaces, ExpectedInterfaceCount);
}
