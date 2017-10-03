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
            {  -0x30,   &IID_IObjectWithSite },
            {  -0x28,   &IID_IDeskBand },
            {  -0x28,       &IID_IDockingWindow },
            {  -0x28,           &IID_IOleWindow },
            {  -0x24,   &IID_IInputObject },
            {  -0x20,   &IID_IPersistStream },
            {  -0x20,       &IID_IPersist },
            {  -0x1c,   &IID_IOleCommandTarget },
            {  -0x18,   &IID_IServiceProvider },
            {    0x0,   &IID_IContextMenu },
            {    0x0,       &IID_IUnknown },
            {    0x8,   &IID_IDispatch },
            {   0x10,   &IID_IPersistPropertyBag },
            {   0x8c,   &IID_IBandNavigate },
        }
    },
    {
        ID_NAME(CLSID_Internet),
        {
            {  -0x18,   &IID_IObjectWithBackReferences },
            {  -0x14,   &IID_IShellFolder2 },
            {  -0x14,       &IID_IShellFolder },
            {  -0x10,   &IID_IPersistFolder2 },
            {  -0x10,       &IID_IPersistFolder },
            {  -0x10,           &IID_IPersist },
            {   -0xc,   &IID_IBrowserFrameOptions },
            {    0x0,   &IID_IContextMenu },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_IShellExtInit },
        }
    },
    {
        ID_NAME(CLSID_CUrlHistory),
        {
            {    0x0,   &IID_IUrlHistoryStg2 },
            {    0x0,       &IID_IUrlHistoryStg },
            {    0x0,           &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_CURLSearchHook),
        {
            {    0x0,   &IID_IURLSearchHook2 },
            {    0x0,       &IID_IURLSearchHook },
            {    0x0,           &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_InternetShortcut),
        {
            {   -0xc,   &IID_IDataObject },
            {   -0x8,   &IID_IContextMenu2 },
            {   -0x8,       &IID_IContextMenu },
            {   -0x4,   &IID_IExtractIconA },
            {    0x0,   &IID_IExtractIconW },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_IPersistFile },
            {    0x4,       &IID_IPersist },
            {    0x8,   &IID_IPersistStream },
            {    0xc,   &IID_IShellExtInit },
            {   0x10,   &IID_IShellLinkA },
            {   0x14,   &IID_IShellLinkW },
            {   0x18,   &IID_IShellPropSheetExt },
            {   0x1c,   &IID_IPropertySetStorage },
            {   0x20,   &IID_INewShortcutHookA },
            {   0x24,   &IID_INewShortcutHookW },
            {   0x30,   &IID_IQueryInfo },
            {   0x38,   &IID_IObjectWithSite },
            {   0x44,   &IID_IOleCommandTarget },
            {   0x48,   &IID_IServiceProvider },
            {   0x4c,   &IID_IPropertyStore },
            {   0x50,   &IID_IInitializeWithFile },
            {   0x54,   &IID_IInitializeWithBindCtx },
        }
    },
    {
        ID_NAME(CLSID_ShellUIHelper),
        {
            {    0x0,   &IID_IUnknown },
            {    0xc,   &IID_IObjectWithSite },
            {   0x10,   &IID_IObjectSafety },
            {   0x14,   &IID_IShellUIHelper2 },
            {   0x14,       &IID_IShellUIHelper },
            {   0x14,           &IID_IDispatch },
            {   0x18,   &IID_IDispatchEx },
        }
    },
    {
        ID_NAME(CLSID_ShellNameSpace),
        {
            {    0x0,   &IID_IShellNameSpace },
            {    0x0,       &IID_IShellFavoritesNameSpace },
            {    0x0,           &IID_IDispatch },
            {    0x0,               &IID_IUnknown },
            {    0x4,   &IID_IProvideClassInfo2 },
            {    0x4,       &IID_IProvideClassInfo },
            {    0x8,   &IID_IPersistStreamInit },
            {   0x10,   &IID_IPersistPropertyBag },
            {   0x14,   &IID_IQuickActivate },
            {   0x18,   &IID_IOleControl },
            {   0x1c,   &IID_IOleObject },
            {   0x20,   &IID_IOleInPlaceActiveObject },
            {   0x24,   &IID_IViewObjectEx },
            {   0x24,       &IID_IViewObject2 },
            {   0x24,           &IID_IViewObject },
            {   0x28,   &IID_IOleInPlaceObjectWindowless },
            {   0x28,       &IID_IOleInPlaceObject },
            {   0x28,           &IID_IOleWindow },
            {   0x38,   &IID_IConnectionPointContainer },
            {   0x3c,   &IID_IDropTarget },
            {   0xb4,   &IID_IObjectWithSite },
            {   0xbc,   &IID_INSCTree },
            {   0xc4,   &IID_IShellBrowser },
            {   0xc8,   &IID_IFolderFilterSite },
            {   0xcc,   &IID_INewMenuClient },
            {   0xd0,   &IID_IServiceProvider },
            {   0xd4,   &IID_INameSpaceTreeControl },
            {   0xe0,   &IID_IVisualProperties },
            {   0xe4,   &IID_IWinEventHandler },
        }
    },
    {
        ID_NAME(CLSID_ShellWindows),
        {
            {  -0xa0,   &IID_IMarshal2 },
            {  -0xa0,       &IID_IMarshal },
            {  -0x20,   &IID_IClientSecurity },
            {  -0x18,   &IID_IRpcOptions },
            {   -0xc,   &IID_ICallFactory },
            {   -0x8,   &IID_IForegroundTransfer },
            {    0x0,   &IID_IMultiQI },
            {    0x0,       &IID_IUnknown },
            { FARAWY,   &IID_IShellWindows },
            { FARAWY,   &IID_IDispatch },
            { FARAWY,   &IID_IConnectionPointContainer },
        }
    },
    {
        ID_NAME(CLSID_WebBrowser),
        {
            {  -0x20,   &IID_IPersist },
            {  -0x1c,   &IID_IOleObject },
            {  -0x18,   &IID_IViewObject2 },
            {  -0x18,       &IID_IViewObject },
            {  -0x14,   &IID_IDataObject },
            {  -0x10,   &IID_IOleInPlaceObject },
            {  -0x10,       &IID_IOleWindow },
            {   -0xc,   &IID_IOleInPlaceActiveObject },
            {    0x0,   &IID_IUnknown },
            {   0x98,   &IID_IPersistStream },
            {   0x98,   &IID_IPersistStreamInit },
            {   0x9c,   &IID_IPersistPropertyBag },
            {   0xa0,   &IID_IOleControl },
            {   0xa8,   &IID_IProvideClassInfo2 },
            {   0xa8,       &IID_IProvideClassInfo },
            {   0xac,   &IID_IConnectionPointContainer },
            {  0x120,   &IID_IWebBrowser2 },
            {  0x120,       &IID_IWebBrowserApp },
            {  0x120,           &IID_IWebBrowser },
            {  0x120,               &IID_IDispatch },
            {  0x130,   &IID_IOleCommandTarget },
            {  0x134,   &IID_IObjectSafety },
            {  0x13c,   &IID_ITargetEmbedding },
            {  0x140,   &IID_IPersistStorage },
            {  0x144,   &IID_IPersistHistory },
            { FARAWY,   &IID_IShellService },
            { FARAWY,   &IID_IServiceProvider },
            { FARAWY,   &IID_IHlinkFrame },
            { FARAWY,   &IID_IUrlHistoryNotify },
            { FARAWY,   &IID_ITargetFrame2 },
            { FARAWY,   &IID_ITargetNotify },
            { FARAWY,   &IID_ITargetFramePriv2 },
            { FARAWY,       &IID_ITargetFramePriv },
            { FARAWY,   &IID_IEFrameAuto },
            { FARAWY,   &IID_IWebBrowserPriv },
            { FARAWY,   &IID_IWebBrowserPriv2 },
            { FARAWY,   &IID_ISecMgrCacheSeedTarget },
            { FARAWY,   &IID_ITargetFrame },
        }
    },
    {
        ID_NAME(CLSID_WebBrowser_V1),
        {
            {  -0x20,   &IID_IPersist },
            {  -0x1c,   &IID_IOleObject },
            {  -0x18,   &IID_IViewObject2 },
            {  -0x18,       &IID_IViewObject },
            {  -0x14,   &IID_IDataObject },
            {  -0x10,   &IID_IOleInPlaceObject },
            {  -0x10,       &IID_IOleWindow },
            {   -0xc,   &IID_IOleInPlaceActiveObject },
            {    0x0,   &IID_IUnknown },
            {   0x98,   &IID_IPersistStream },
            {   0x98,   &IID_IPersistStreamInit },
            {   0x9c,   &IID_IPersistPropertyBag },
            {   0xa0,   &IID_IOleControl },
            {   0xa8,   &IID_IProvideClassInfo2 },
            {   0xa8,       &IID_IProvideClassInfo },
            {   0xac,   &IID_IConnectionPointContainer },
            {  0x120,   &IID_IWebBrowser2 },
            {  0x120,       &IID_IWebBrowserApp },
            {  0x120,           &IID_IWebBrowser },
            {  0x120,               &IID_IDispatch },
            {  0x130,   &IID_IOleCommandTarget },
            {  0x134,   &IID_IObjectSafety },
            {  0x13c,   &IID_ITargetEmbedding },
            {  0x140,   &IID_IPersistStorage },
            {  0x144,   &IID_IPersistHistory },
            { FARAWY,   &IID_IShellService },
            { FARAWY,   &IID_IServiceProvider },
            { FARAWY,   &IID_IHlinkFrame },
            { FARAWY,   &IID_IUrlHistoryNotify },
            { FARAWY,   &IID_ITargetFrame2 },
            { FARAWY,   &IID_ITargetNotify },
            { FARAWY,   &IID_ITargetFramePriv2 },
            { FARAWY,       &IID_ITargetFramePriv },
            { FARAWY,   &IID_IEFrameAuto },
            { FARAWY,   &IID_IWebBrowserPriv },
            { FARAWY,   &IID_IWebBrowserPriv2 },
            { FARAWY,   &IID_ISecMgrCacheSeedTarget },
            { FARAWY,   &IID_ITargetFrame },
        }
    },
};
static const INT ExpectedInterfaceCount = RTL_NUMBER_OF(ExpectedInterfaces);

START_TEST(ieframe)
{
    TestClasses(L"ieframe", ExpectedInterfaces, ExpectedInterfaceCount);
}
