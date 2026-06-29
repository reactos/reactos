/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         COM interface test for browseui classes
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 *                  Carl Bialorucki <carl.bialorucki@reactos.org>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces[] =
{
    {
        ID_NAME(CLSID_ACLCustomMRU, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IEnumString },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IACList },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IACLCustomMRU },
        },
    },
    {
        ID_NAME(CLSID_ACLHistory, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IEnumString },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
        },
    },
    {
        ID_NAME(CLSID_ACLMRU, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IEnumString },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IACList },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IACLCustomMRU },
        },
    },
    {
        ID_NAME(CLSID_ACLMulti, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IEnumString },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IObjMgr },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IACList },
        }
    },
    {
        ID_NAME(CLSID_ACListISF, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellService },

            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IEnumString },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IACList2 },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IACList },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_ICurrentWorkingDirectory },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersistFolder },

            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IPersistIDList },
        }
    },
    {
        ID_NAME(CLSID_AddressEditBox, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IWinEventHandler },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDispatch },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IAddressBand },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IAddressEditBox },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersistStream },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellService },
        }
    },
    {
        ID_NAME(CLSID_AugmentedShellFolder, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IAugmentedShellFolder2 },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IAugmentedShellFolder },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellFolder },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellService },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_ITranslateShellChangeNotify },
        }
    },
    {
        ID_NAME(CLSID_AugmentedShellFolder2, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IAugmentedShellFolder2 },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IAugmentedShellFolder },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellFolder },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellFolder2 },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellService },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_ITranslateShellChangeNotify },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDropTarget },
        }
    },
    {
        ID_NAME(CLSID_AutoComplete, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IAutoComplete2 },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IAutoComplete },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IAutoCompleteDropDown },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IEnumString },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IAccessible },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDispatch },

            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IShellService },
        }
    },
    {
        ID_NAME(CLSID_BackgroundTaskScheduler, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IShellTaskScheduler },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_BandProxy, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IBandProxy },

            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_BandSiteMenu, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IContextMenu3 },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IContextMenu2 },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IContextMenu },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IShellService },
        }
    },
    {
        ID_NAME(CLSID_BrandBand, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDeskBand },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDockingWindow },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IInputObject },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersistStream },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersist },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IWinEventHandler },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDispatch },
        }
    },
    {
        ID_NAME(CLSID_BrowserBand, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDeskBand },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDockingWindow },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IInputObject },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersistStream },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersist },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IContextMenu },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IWinEventHandler },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDispatch },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersistPropertyBag },
        },
    },
    {
        ID_NAME(CLSID_CCommonBrowser, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellBrowser },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IBrowserService3 },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IBrowserService2 },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IBrowserService },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDockingWindowSite },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDockingWindowFrame },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IInputObjectSite },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDropTarget },
        }
    },
    {
        ID_NAME(CLSID_CDockingBarPropertyBag, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPropertyBag },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
        },
    },
    {
        ID_NAME(CLSID_CRegTreeOptions, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IRegTreeOptions },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_CommBand, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IWinEventHandler },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDeskBand },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDockingWindow },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IInputObject },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersistStream },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersist },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IContextMenu },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDispatch },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersistPropertyBag },
        },
    },
    {
        ID_NAME(CLSID_DeskBar, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDeskBar },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IInputObjectSite },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IInputObject },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDockingWindow },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersistStreamInit },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersistStream },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersist },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersistPropertyBag },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_DeskBarApp, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDeskBar },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IInputObjectSite },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IInputObject },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDockingWindow },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersistStreamInit },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersistStream },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersist },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersistPropertyBag },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IContextMenu3 },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IContextMenu2 },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IContextMenu },
        }
    },
    {
        ID_NAME(CLSID_GlobalFolderSettings, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IGlobalFolderSettings },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_ImageListCache, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IUnknown },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_ImgCtxThumbnailExtractor, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IExtractImage2 },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IExtractImage },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersistFile },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_InternetToolbar, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IExplorerToolbar },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellChangeNotify },

            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDispatch },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &DIID_DWebBrowserEvents },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDeskBar },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IInputObjectSite },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IInputObject },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDockingWindow },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersistStreamInit },

            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IBandHost },
        }
    },
    {
        ID_NAME(CLSID_ProgressDialog, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IProgressDialog },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IActionProgressDialog },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IActionProgress },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IObjectWithSite },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_RebarBandSite, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IBandSite },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IInputObjectSite },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IInputObject },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDeskBarClient },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IWinEventHandler },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersistStream },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersist },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDropTarget },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IBandSiteHelper },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IOleCommandTarget },
        }
    },
    {
        ID_NAME(CLSID_SH_AddressBand, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IAddressBand },

            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDeskBand },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDockingWindow },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IInputObject },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersistStream },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersist },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IWinEventHandler },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IInputObjectSite },

            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IShellChangeNotify },
        }
    },
    {
        ID_NAME(CLSID_SH_SearchBand, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleContainer },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleInPlaceFrame },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_ISearchBar },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IFileSearchBand },

            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDeskBand },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDockingWindow },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IInputObject },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IContextMenu },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDispatch },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IBandNavigate },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersist },

            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IOleCommandTarget },
            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IPersistPropertyBag },
            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IPersistStream },
            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IWinEventHandler },
        }
    },
    {
        ID_NAME(CLSID_SharedTaskScheduler, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IShellTaskScheduler },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_ShellSearchExt, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IContextMenu },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_ShellTaskScheduler, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IShellTaskScheduler },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
        },
    },
    {
        ID_NAME(CLSID_UserAssist, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_WebSearchExt, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,     &IID_IContextMenu },
            { NTDDI_MIN,          NTDDI_WS03SP4,     &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_WS03SP4,     &IID_IObjectWithSite },
        },
    },
};

START_TEST(browseui)
{
    TestClassesEx(L"browseui",
                  ExpectedInterfaces, RTL_NUMBER_OF(ExpectedInterfaces),
                  NTDDI_MIN, NTDDI_VISTASP4,
                  FALSE);
}
