/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         COM interface test for ieframe classes
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces[] =
{
    {
        ID_NAME(CLSID_IE_SearchBand),
        {
            {  -0x30,  -0x60,   &IID_IObjectWithSite },
            {  -0x28,  -0x50,   &IID_IDeskBand },
            {  -0x28,  -0x50,       &IID_IDockingWindow },
            {  -0x28,  -0x50,           &IID_IOleWindow },
            {  -0x24,  -0x48,   &IID_IInputObject },
            {  -0x20,  -0x40,   &IID_IPersistStream },
            {  -0x20,  -0x40,       &IID_IPersist },
            {  -0x1c,  -0x38,   &IID_IOleCommandTarget },
            {  -0x18,  -0x30,   &IID_IServiceProvider },
            {    0x0,    0x0,   &IID_IContextMenu },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x8,   0x10,   &IID_IDispatch },
            {   0x10,   0x20,   &IID_IPersistPropertyBag },
            {   0x8c,  0x118,   &IID_IBandNavigate },
        }
    },
    {
        ID_NAME(CLSID_Internet),
        {
            {  -0x18,  -0x30,   &IID_IObjectWithBackReferences },
            {  -0x14,  -0x28,   &IID_IShellFolder2 },
            {  -0x14,  -0x28,       &IID_IShellFolder },
            {  -0x10,  -0x20,   &IID_IPersistFolder2 },
            {  -0x10,  -0x20,       &IID_IPersistFolder },
            {  -0x10,  -0x20,           &IID_IPersist },
            {   -0xc,  -0x18,   &IID_IBrowserFrameOptions },
            {    0x0,    0x0,   &IID_IContextMenu },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellExtInit },
        }
    },
    {
        ID_NAME(CLSID_CUrlHistory),
        {
            {    0x0,    0x0,   &IID_IUrlHistoryStg2 },
            {    0x0,    0x0,       &IID_IUrlHistoryStg },
            {    0x0,    0x0,           &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_CURLSearchHook),
        {
            {    0x0,    0x0,   &IID_IURLSearchHook2 },
            {    0x0,    0x0,       &IID_IURLSearchHook },
            {    0x0,    0x0,           &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_InternetShortcut),
        {
            {   -0xc,  -0x18,   &IID_IDataObject },
            {   -0x8,  -0x10,   &IID_IContextMenu2 },
            {   -0x8,  -0x10,       &IID_IContextMenu },
            {   -0x4,   -0x8,   &IID_IExtractIconA },
            {    0x0,    0x0,   &IID_IExtractIconW },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IPersistFile },
            {    0x4,    0x8,       &IID_IPersist },
            {    0x8,   0x10,   &IID_IPersistStream },
            {    0xc,   0x18,   &IID_IShellExtInit },
            {   0x10,   0x20,   &IID_IShellLinkA },
            {   0x14,   0x28,   &IID_IShellLinkW },
            {   0x18,   0x30,   &IID_IShellPropSheetExt },
            {   0x1c,   0x38,   &IID_IPropertySetStorage },
            {   0x20,   0x40,   &IID_INewShortcutHookA },
            {   0x24,   0x48,   &IID_INewShortcutHookW },
            {   0x30,   0x60,   &IID_IQueryInfo },
            {   0x38,   0x70,   &IID_IObjectWithSite },
            {   0x44,   0x88,   &IID_IOleCommandTarget },
            {   0x48,   0x90,   &IID_IServiceProvider },
            {   0x4c,   0x98,   &IID_IPropertyStore },
            {   0x50,   0xa0,   &IID_IInitializeWithFile },
            {   0x54,   0xa8,   &IID_IInitializeWithBindCtx },
        }
    },
    {
        ID_NAME(CLSID_ShellUIHelper),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {    0xc,   0x18,   &IID_IObjectWithSite },
            {   0x10,   0x20,   &IID_IObjectSafety },
            {   0x14,   0x28,   &IID_IShellUIHelper2 },
            {   0x14,   0x28,       &IID_IShellUIHelper },
            {   0x14,   0x28,           &IID_IDispatch },
            {   0x18,   0x30,   &IID_IDispatchEx },
        }
    },
    {
        ID_NAME(CLSID_ShellNameSpace),
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
            {   0xb4,  0x168,   &IID_IObjectWithSite },
            {   0xbc,  0x178,   &IID_INSCTree },
            {   0xc4,  0x188,   &IID_IShellBrowser },
            {   0xc8,  0x190,   &IID_IFolderFilterSite },
            {   0xcc,  0x198,   &IID_INewMenuClient },
            {   0xd0,  0x1a0,   &IID_IServiceProvider },
            {   0xd4,  0x1a8,   &IID_INameSpaceTreeControl },
            {   0xe0,  0x1c0,   &IID_IVisualProperties },
            {   0xe4,  0x1c8,   &IID_IWinEventHandler },
        }
    },
    {
        ID_NAME(CLSID_ShellWindows),
        {
            {  -0xa0, -0x140,   &IID_IMarshal2 },
            {  -0xa0, -0x140,       &IID_IMarshal },
            {  -0x20,  -0x40,   &IID_IClientSecurity },
            {  -0x18,  -0x30,   &IID_IRpcOptions },
            {   -0xc,  -0x18,   &IID_ICallFactory },
            {   -0x8,  -0x10,   &IID_IForegroundTransfer },
            {    0x0,    0x0,   &IID_IMultiQI },
            {    0x0,    0x0,       &IID_IUnknown },
            { FARAWY, FARAWY,   &IID_IShellWindows },
            { FARAWY, FARAWY,   &IID_IDispatch },
            { FARAWY, FARAWY,   &IID_IConnectionPointContainer },
        }
    },
    {
        ID_NAME(CLSID_WebBrowser),
        {
            {  -0x20,  -0x40,   &IID_IPersist },
            {  -0x1c,  -0x38,   &IID_IOleObject },
            {  -0x18,  -0x30,   &IID_IViewObject2 },
            {  -0x18,  -0x30,       &IID_IViewObject },
            {  -0x14,  -0x28,   &IID_IDataObject },
            {  -0x10,  -0x20,   &IID_IOleInPlaceObject },
            {  -0x10,  -0x20,       &IID_IOleWindow },
            {   -0xc,  -0x18,   &IID_IOleInPlaceActiveObject },
            {    0x0,    0x0,   &IID_IUnknown },
            {   0x98,  0x130,   &IID_IPersistStream },
            {   0x98,  0x130,   &IID_IPersistStreamInit },
            {   0x9c,  0x138,   &IID_IPersistPropertyBag },
            {   0xa0,  0x140,   &IID_IOleControl },
            {   0xa8,  0x150,   &IID_IProvideClassInfo2 },
            {   0xa8,  0x150,       &IID_IProvideClassInfo },
            {   0xac,  0x158,   &IID_IConnectionPointContainer },
            {  0x120,  0x240,   &IID_IWebBrowser2 },
            {  0x120,  0x240,       &IID_IWebBrowserApp },
            {  0x120,  0x240,           &IID_IWebBrowser },
            {  0x120,  0x240,               &IID_IDispatch },
            {  0x130,  0x260,   &IID_IOleCommandTarget },
            {  0x134,  0x268,   &IID_IObjectSafety },
            {  0x13c,  0x278,   &IID_ITargetEmbedding },
            {  0x140,  0x280,   &IID_IPersistStorage },
            {  0x144,  0x288,   &IID_IPersistHistory },
            { FARAWY, FARAWY,   &IID_IShellService },
            { FARAWY, FARAWY,   &IID_IServiceProvider },
            { FARAWY, FARAWY,   &IID_IHlinkFrame },
            { FARAWY, FARAWY,   &IID_IUrlHistoryNotify },
            { FARAWY, FARAWY,   &IID_ITargetFrame2 },
            { FARAWY, FARAWY,   &IID_ITargetNotify },
            { FARAWY, FARAWY,   &IID_ITargetFramePriv2 },
            { FARAWY, FARAWY,       &IID_ITargetFramePriv },
            { FARAWY, FARAWY,   &IID_IEFrameAuto },
            { FARAWY, FARAWY,   &IID_IWebBrowserPriv },
            { FARAWY, FARAWY,   &IID_IWebBrowserPriv2 },
            { FARAWY, FARAWY,   &IID_ISecMgrCacheSeedTarget },
            { FARAWY, FARAWY,   &IID_ITargetFrame },
        }
    },
    {
        ID_NAME(CLSID_WebBrowser_V1),
        {
            {  -0x20,  -0x40,   &IID_IPersist },
            {  -0x1c,  -0x38,   &IID_IOleObject },
            {  -0x18,  -0x30,   &IID_IViewObject2 },
            {  -0x18,  -0x30,       &IID_IViewObject },
            {  -0x14,  -0x28,   &IID_IDataObject },
            {  -0x10,  -0x20,   &IID_IOleInPlaceObject },
            {  -0x10,  -0x20,       &IID_IOleWindow },
            {   -0xc,  -0x18,   &IID_IOleInPlaceActiveObject },
            {    0x0,    0x0,   &IID_IUnknown },
            {   0x98,  0x130,   &IID_IPersistStream },
            {   0x98,  0x130,   &IID_IPersistStreamInit },
            {   0x9c,  0x138,   &IID_IPersistPropertyBag },
            {   0xa0,  0x140,   &IID_IOleControl },
            {   0xa8,  0x150,   &IID_IProvideClassInfo2 },
            {   0xa8,  0x150,       &IID_IProvideClassInfo },
            {   0xac,  0x158,   &IID_IConnectionPointContainer },
            {  0x120,  0x240,   &IID_IWebBrowser2 },
            {  0x120,  0x240,       &IID_IWebBrowserApp },
            {  0x120,  0x240,           &IID_IWebBrowser },
            {  0x120,  0x240,               &IID_IDispatch },
            {  0x130,  0x260,   &IID_IOleCommandTarget },
            {  0x134,  0x268,   &IID_IObjectSafety },
            {  0x13c,  0x278,   &IID_ITargetEmbedding },
            {  0x140,  0x280,   &IID_IPersistStorage },
            {  0x144,  0x288,   &IID_IPersistHistory },
            { FARAWY, FARAWY,   &IID_IShellService },
            { FARAWY, FARAWY,   &IID_IServiceProvider },
            { FARAWY, FARAWY,   &IID_IHlinkFrame },
            { FARAWY, FARAWY,   &IID_IUrlHistoryNotify },
            { FARAWY, FARAWY,   &IID_ITargetFrame2 },
            { FARAWY, FARAWY,   &IID_ITargetNotify },
            { FARAWY, FARAWY,   &IID_ITargetFramePriv2 },
            { FARAWY, FARAWY,       &IID_ITargetFramePriv },
            { FARAWY, FARAWY,   &IID_IEFrameAuto },
            { FARAWY, FARAWY,   &IID_IWebBrowserPriv },
            { FARAWY, FARAWY,   &IID_IWebBrowserPriv2 },
            { FARAWY, FARAWY,   &IID_ISecMgrCacheSeedTarget },
            { FARAWY, FARAWY,   &IID_ITargetFrame },
        }
    },
};

START_TEST(ieframe)
{
    TestClasses(L"ieframe", ExpectedInterfaces, RTL_NUMBER_OF(ExpectedInterfaces));
}
