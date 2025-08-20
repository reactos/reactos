/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         COM interface test for ieframe classes
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 *                  Carl Bialorucki <carl.bialorucki@reactos.org>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces[] =
{
    {
        ID_NAME(CLSID_CommBand, NTDDI_WIN7, NTDDI_MAX),
        {
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IDeskBand },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IDockingWindow },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IOleWindow },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IInputObject },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IPersistStream },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IPersist },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IOleCommandTarget },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IServiceProvider },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IContextMenu },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IDispatch },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IPersistPropertyBag },
        },
    },
    {
        ID_NAME(CLSID_CUrlHistory, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUrlHistoryStg2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUrlHistoryStg },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_CURLSearchHook, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IURLSearchHook2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IURLSearchHook },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_IE_SearchBand, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDeskBand },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDockingWindow },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IInputObject },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersistStream },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersist },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IContextMenu },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDispatch },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersistPropertyBag },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IBandNavigate },
        }
    },
    {
        ID_NAME(CLSID_Internet, NTDDI_MIN, NTDDI_WINBLUE),
        {
            { NTDDI_MIN,          NTDDI_WINBLUE,      &IID_IObjectWithBackReferences },
            { NTDDI_MIN,          NTDDI_WINBLUE,      &IID_IShellFolder2 },
            { NTDDI_MIN,          NTDDI_WINBLUE,      &IID_IShellFolder },
            { NTDDI_MIN,          NTDDI_WINBLUE,      &IID_IPersistFolder2 },
            { NTDDI_MIN,          NTDDI_WINBLUE,      &IID_IPersistFolder },
            { NTDDI_MIN,          NTDDI_WINBLUE,      &IID_IPersist },
            { NTDDI_MIN,          NTDDI_WINBLUE,      &IID_IBrowserFrameOptions },
            { NTDDI_MIN,          NTDDI_WINBLUE,      &IID_IContextMenu },
            { NTDDI_MIN,          NTDDI_WINBLUE,      &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_WINBLUE,      &IID_IShellExtInit },
        }
    },
    {
        ID_NAME(CLSID_InternetShortcut, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDataObject },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IContextMenu2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IContextMenu },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IExtractIconA },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IExtractIconW },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFile },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersist },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistStream },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellExtInit },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellLinkA },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellLinkW },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellPropSheetExt },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPropertySetStorage },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INewShortcutHookA },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INewShortcutHookW },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IQueryInfo },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPropertyStore },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IInitializeWithFile },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IInitializeWithBindCtx },
        }
    },
    {
        ID_NAME(CLSID_ShellUIHelper, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectSafety },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellUIHelper2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellUIHelper },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDispatch },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDispatchEx },
        }
    },
    {
        ID_NAME(CLSID_ShellNameSpace, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellNameSpace },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFavoritesNameSpace },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDispatch },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IProvideClassInfo2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IProvideClassInfo },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistStreamInit },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistPropertyBag },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IQuickActivate },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleControl },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleObject },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleInPlaceActiveObject },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IViewObjectEx },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IViewObject2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IViewObject },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleInPlaceObjectWindowless },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleInPlaceObject },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IConnectionPointContainer },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDropTarget },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INSCTree },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellBrowser },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IFolderFilterSite },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INewMenuClient },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INameSpaceTreeControl },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IVisualProperties },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IWinEventHandler },
        }
    },
    {
        ID_NAME(CLSID_ShellWindows, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IMarshal2 },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IMarshal },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IClientSecurity },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IRpcOptions },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_ICallFactory },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IForegroundTransfer },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IMultiQI },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IShellWindows },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDispatch },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IConnectionPointContainer },

            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IServiceProvider },
        }
    },
    {
        ID_NAME(CLSID_WebBrowser, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IEFrameAuto },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IWebBrowserPriv2 },

#ifdef _WIN64 // IID_IWebBrowserPriv is missing from this class on Vista x64
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IWebBrowserPriv },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IWebBrowserPriv },
#else
            { NTDDI_MIN,          NTDDI_WINBLUE,      &IID_IWebBrowserPriv },
#endif

            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersist },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleObject },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IViewObject2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IViewObject },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDataObject },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleInPlaceObject },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleInPlaceActiveObject },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistStream },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistStreamInit },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistPropertyBag },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleControl },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IProvideClassInfo2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IProvideClassInfo },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IConnectionPointContainer },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IWebBrowser2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IWebBrowserApp },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IWebBrowser },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDispatch },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectSafety },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_ITargetEmbedding },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistStorage },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistHistory },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellService },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IHlinkFrame },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUrlHistoryNotify },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_ITargetFrame2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_ITargetNotify },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_ITargetFramePriv2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_ITargetFramePriv },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_ISecMgrCacheSeedTarget },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_ITargetFrame },
        }
    },
    {
        ID_NAME(CLSID_WebBrowser_V1, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IEFrameAuto },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IWebBrowserPriv2 },

#ifdef _WIN64 // IID_IWebBrowserPriv is missing from this class on Vista x64
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IWebBrowserPriv },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IWebBrowserPriv },
#else
            { NTDDI_MIN,          NTDDI_WINBLUE,      &IID_IWebBrowserPriv },
#endif

            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersist },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleObject },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IViewObject2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IViewObject },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDataObject },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleInPlaceObject },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleInPlaceActiveObject },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistStream },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistStreamInit },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistPropertyBag },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleControl },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IProvideClassInfo2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IProvideClassInfo },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IConnectionPointContainer },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IWebBrowser2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IWebBrowserApp },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IWebBrowser },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDispatch },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectSafety },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_ITargetEmbedding },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistStorage },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistHistory },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellService },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IHlinkFrame },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUrlHistoryNotify },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_ITargetFrame2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_ITargetNotify },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_ITargetFramePriv2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_ITargetFramePriv },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_ISecMgrCacheSeedTarget },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_ITargetFrame },
        }
    },
};

START_TEST(ieframe)
{
    TestClasses(L"ieframe", ExpectedInterfaces, RTL_NUMBER_OF(ExpectedInterfaces));
}
