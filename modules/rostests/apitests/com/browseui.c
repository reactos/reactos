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
        ID_NAME(CLSID_ACLCustomMRU, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IEnumString },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
            { NTDDI_MIN, NTDDI_MAX, &IID_IACList },
            { NTDDI_MIN, NTDDI_MAX, &IID_IACLCustomMRU },
        },
    },
    {
        ID_NAME(CLSID_ACLHistory, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IEnumString },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
        },
    },
    {
        ID_NAME(CLSID_ACLMRU, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IEnumString },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
            { NTDDI_MIN, NTDDI_MAX, &IID_IACList },
            { NTDDI_MIN, NTDDI_MAX, &IID_IACLCustomMRU },
        },
    },
    {
        ID_NAME(CLSID_ACLMulti, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IEnumString },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
            { NTDDI_MIN, NTDDI_MAX, &IID_IObjMgr },
            { NTDDI_MIN, NTDDI_MAX, &IID_IACList },
        }
    },
    {
        ID_NAME(CLSID_ACListISF, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IEnumString },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
            { NTDDI_MIN, NTDDI_MAX, &IID_IACList2 },
            { NTDDI_MIN, NTDDI_MAX, &IID_IACList },
            { NTDDI_MIN, NTDDI_MAX, &IID_ICurrentWorkingDirectory },
            { NTDDI_MIN, NTDDI_MAX, &IID_IShellService },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersistFolder },
        }
    },
    {
        ID_NAME(CLSID_AddressEditBox, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IWinEventHandler },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDispatch },
            { NTDDI_MIN, NTDDI_MAX, &IID_IAddressBand },
            { NTDDI_MIN, NTDDI_MAX, &IID_IAddressEditBox },
            { NTDDI_MIN, NTDDI_MAX, &IID_IOleCommandTarget },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersistStream },
            { NTDDI_MIN, NTDDI_MAX, &IID_IShellService },
        }
    },
    {
        ID_NAME(CLSID_AugmentedShellFolder, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IAugmentedShellFolder2 },
            { NTDDI_MIN, NTDDI_MAX, &IID_IAugmentedShellFolder },
            { NTDDI_MIN, NTDDI_MAX, &IID_IShellFolder },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
            { NTDDI_MIN, NTDDI_MAX, &IID_IShellService },
            { NTDDI_MIN, NTDDI_MAX, &IID_ITranslateShellChangeNotify },
        }
    },
    {
        ID_NAME(CLSID_AugmentedShellFolder2, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IAugmentedShellFolder2 },
            { NTDDI_MIN, NTDDI_MAX, &IID_IAugmentedShellFolder },
            { NTDDI_MIN, NTDDI_MAX, &IID_IShellFolder },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
            { NTDDI_MIN, NTDDI_MAX, &IID_IShellFolder2 },
            { NTDDI_MIN, NTDDI_MAX, &IID_IShellService },
            { NTDDI_MIN, NTDDI_MAX, &IID_ITranslateShellChangeNotify },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDropTarget },
        }
    },
    {
        ID_NAME(CLSID_AutoComplete, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IAutoComplete2 },
            { NTDDI_MIN, NTDDI_MAX, &IID_IAutoComplete },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
            { NTDDI_MIN, NTDDI_MAX, &IID_IAutoCompleteDropDown },
            { NTDDI_MIN, NTDDI_MAX, &IID_IEnumString },
            { NTDDI_MIN, NTDDI_MAX, &IID_IAccessible },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDispatch },
        }
    },
    {
        ID_NAME(CLSID_BackgroundTaskScheduler, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IShellTaskScheduler },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_BandProxy, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IBandProxy },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_BandSiteMenu, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IContextMenu3 },
            { NTDDI_MIN, NTDDI_MAX, &IID_IContextMenu2 },
            { NTDDI_MIN, NTDDI_MAX, &IID_IContextMenu },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
            { NTDDI_MIN, NTDDI_MAX, &IID_IShellService },
        }
    },
    {
        ID_NAME(CLSID_BrandBand, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IDeskBand },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDockingWindow },
            { NTDDI_MIN, NTDDI_MAX, &IID_IOleWindow },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
            { NTDDI_MIN, NTDDI_MAX, &IID_IObjectWithSite },
            { NTDDI_MIN, NTDDI_MAX, &IID_IInputObject },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersistStream },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersist },
            { NTDDI_MIN, NTDDI_MAX, &IID_IOleCommandTarget },
            { NTDDI_MIN, NTDDI_MAX, &IID_IServiceProvider },
            { NTDDI_MIN, NTDDI_MAX, &IID_IWinEventHandler },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDispatch },
        }
    },
    {
        ID_NAME(CLSID_BrowserBand, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IDeskBand },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDockingWindow },
            { NTDDI_MIN, NTDDI_MAX, &IID_IOleWindow },
            { NTDDI_MIN, NTDDI_MAX, &IID_IObjectWithSite },
            { NTDDI_MIN, NTDDI_MAX, &IID_IInputObject },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersistStream },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersist },
            { NTDDI_MIN, NTDDI_MAX, &IID_IOleCommandTarget },
            { NTDDI_MIN, NTDDI_MAX, &IID_IServiceProvider },
            { NTDDI_MIN, NTDDI_MAX, &IID_IContextMenu },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
            { NTDDI_MIN, NTDDI_MAX, &IID_IWinEventHandler },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDispatch },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersistPropertyBag },
        },
    },
    {
        ID_NAME(CLSID_CCommonBrowser, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
            { NTDDI_MIN, NTDDI_MAX, &IID_IShellBrowser },
            { NTDDI_MIN, NTDDI_MAX, &IID_IOleWindow },
            { NTDDI_MIN, NTDDI_MAX, &IID_IBrowserService3 },
            { NTDDI_MIN, NTDDI_MAX, &IID_IBrowserService2 },
            { NTDDI_MIN, NTDDI_MAX, &IID_IBrowserService },
            { NTDDI_MIN, NTDDI_MAX, &IID_IServiceProvider },
            { NTDDI_MIN, NTDDI_MAX, &IID_IOleCommandTarget },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDockingWindowSite },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDockingWindowFrame },
            { NTDDI_MIN, NTDDI_MAX, &IID_IInputObjectSite },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDropTarget },
            { NTDDI_MIN, NTDDI_MAX, &IID_IShellBrowserService },
        }
    },
    {
        ID_NAME(CLSID_CDockingBarPropertyBag, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IPropertyBag },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
        },
    },
    {
        ID_NAME(CLSID_CRegTreeOptions, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IRegTreeOptions },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
            { NTDDI_MIN, NTDDI_MAX, &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_CommBand, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IDeskBand },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDockingWindow },
            { NTDDI_MIN, NTDDI_MAX, &IID_IOleWindow },
            { NTDDI_MIN, NTDDI_MAX, &IID_IObjectWithSite },
            { NTDDI_MIN, NTDDI_MAX, &IID_IInputObject },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersistStream },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersist },
            { NTDDI_MIN, NTDDI_MAX, &IID_IOleCommandTarget },
            { NTDDI_MIN, NTDDI_MAX, &IID_IServiceProvider },
            { NTDDI_MIN, NTDDI_MAX, &IID_IContextMenu },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
            { NTDDI_MIN, NTDDI_MAX, &IID_IWinEventHandler },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDispatch },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersistPropertyBag },
        },
    },
    {
        ID_NAME(CLSID_DeskBar, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IOleCommandTarget },
            { NTDDI_MIN, NTDDI_MAX, &IID_IServiceProvider },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDeskBar },
            { NTDDI_MIN, NTDDI_MAX, &IID_IOleWindow },
            { NTDDI_MIN, NTDDI_MAX, &IID_IInputObjectSite },
            { NTDDI_MIN, NTDDI_MAX, &IID_IInputObject },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDockingWindow },
            { NTDDI_MIN, NTDDI_MAX, &IID_IObjectWithSite },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersistStreamInit },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersistStream },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersist },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersistPropertyBag },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_DeskBarApp, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IOleCommandTarget },
            { NTDDI_MIN, NTDDI_MAX, &IID_IServiceProvider },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDeskBar },
            { NTDDI_MIN, NTDDI_MAX, &IID_IOleWindow },
            { NTDDI_MIN, NTDDI_MAX, &IID_IInputObjectSite },
            { NTDDI_MIN, NTDDI_MAX, &IID_IInputObject },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDockingWindow },
            { NTDDI_MIN, NTDDI_MAX, &IID_IObjectWithSite },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersistStreamInit },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersistStream },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersist },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersistPropertyBag },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
            { NTDDI_MIN, NTDDI_MAX, &IID_IContextMenu3 },
            { NTDDI_MIN, NTDDI_MAX, &IID_IContextMenu2 },
            { NTDDI_MIN, NTDDI_MAX, &IID_IContextMenu },
        }
    },
    {
        ID_NAME(CLSID_GlobalFolderSettings, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IGlobalFolderSettings },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_ImageListCache, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_ImgCtxThumbnailExtractor, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IExtractImage2 },
            { NTDDI_MIN, NTDDI_MAX, &IID_IExtractImage },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersistFile },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_InternetToolbar, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IOleCommandTarget },
            { NTDDI_MIN, NTDDI_MAX, &IID_IServiceProvider },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDeskBar },
            { NTDDI_MIN, NTDDI_MAX, &IID_IOleWindow },
            { NTDDI_MIN, NTDDI_MAX, &IID_IInputObjectSite },
            { NTDDI_MIN, NTDDI_MAX, &IID_IInputObject },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDockingWindow },
            { NTDDI_MIN, NTDDI_MAX, &IID_IObjectWithSite },
            { NTDDI_MIN, NTDDI_MAX, &IID_IExplorerToolbar },
            { NTDDI_MIN, NTDDI_MAX, &DIID_DWebBrowserEvents },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDispatch },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersistStreamInit },
            { NTDDI_MIN, NTDDI_MAX, &IID_IShellChangeNotify },
        }
    },
    {
        ID_NAME(CLSID_ProgressDialog, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IProgressDialog },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
            { NTDDI_MIN, NTDDI_MAX, &IID_IOleWindow },
            { NTDDI_MIN, NTDDI_MAX, &IID_IActionProgressDialog },
            { NTDDI_MIN, NTDDI_MAX, &IID_IActionProgress },
            { NTDDI_MIN, NTDDI_MAX, &IID_IObjectWithSite },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_RebarBandSite, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
            { NTDDI_MIN, NTDDI_MAX, &IID_IBandSite },
            { NTDDI_MIN, NTDDI_MAX, &IID_IInputObjectSite },
            { NTDDI_MIN, NTDDI_MAX, &IID_IInputObject },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDeskBarClient },
            { NTDDI_MIN, NTDDI_MAX, &IID_IOleWindow },
            { NTDDI_MIN, NTDDI_MAX, &IID_IWinEventHandler },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersistStream },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersist },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDropTarget },
            { NTDDI_MIN, NTDDI_MAX, &IID_IServiceProvider },
            { NTDDI_MIN, NTDDI_MAX, &IID_IBandSiteHelper },
            { NTDDI_MIN, NTDDI_MAX, &IID_IOleCommandTarget },
        }
    },
    {
        ID_NAME(CLSID_SH_AddressBand, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IDeskBand },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDockingWindow },
            { NTDDI_MIN, NTDDI_MAX, &IID_IOleWindow },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
            { NTDDI_MIN, NTDDI_MAX, &IID_IObjectWithSite },
            { NTDDI_MIN, NTDDI_MAX, &IID_IInputObject },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersistStream },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersist },
            { NTDDI_MIN, NTDDI_MAX, &IID_IOleCommandTarget },
            { NTDDI_MIN, NTDDI_MAX, &IID_IServiceProvider },
            { NTDDI_MIN, NTDDI_MAX, &IID_IWinEventHandler },
            { NTDDI_MIN, NTDDI_MAX, &IID_IAddressBand },
            { NTDDI_MIN, NTDDI_MAX, &IID_IInputObjectSite },
        }
    },
    {
        ID_NAME(CLSID_SH_SearchBand, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
            { NTDDI_MIN, NTDDI_MAX, &IID_IOleContainer },
            { NTDDI_MIN, NTDDI_MAX, &IID_IOleInPlaceFrame },
            //{ NTDDI_MIN, NTDDI_MAX, &IID_IOleInPlaceUIWindow },
            { NTDDI_MIN, NTDDI_MAX, &IID_IOleWindow },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDeskBand },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDockingWindow },
            { NTDDI_MIN, NTDDI_MAX, &IID_IInputObject },
            { NTDDI_MIN, NTDDI_MAX, &IID_IObjectWithSite },
            { NTDDI_MIN, NTDDI_MAX, &IID_IContextMenu },
            { NTDDI_MIN, NTDDI_MAX, &IID_IServiceProvider },
            { NTDDI_MIN, NTDDI_MAX, &IID_ISearchBar },
            { NTDDI_MIN, NTDDI_MAX, &IID_IFileSearchBand },
            { NTDDI_MIN, NTDDI_MAX, &IID_IDispatch },
            { NTDDI_MIN, NTDDI_MAX, &IID_IBandNavigate },
            { NTDDI_MIN, NTDDI_MAX, &IID_IPersist },
        }
    },
    {
        ID_NAME(CLSID_SharedTaskScheduler, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IShellTaskScheduler },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_ShellSearchExt, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IContextMenu },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
            { NTDDI_MIN, NTDDI_MAX, &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_ShellTaskScheduler, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IShellTaskScheduler },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
        },
    },
    {
        ID_NAME(CLSID_UserAssist, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_WebSearchExt, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN, NTDDI_MAX, &IID_IContextMenu },
            { NTDDI_MIN, NTDDI_MAX, &IID_IUnknown },
            { NTDDI_MIN, NTDDI_MAX, &IID_IObjectWithSite },
        },
    },
};

START_TEST(browseui)
{
    TestClasses(L"browseui", ExpectedInterfaces, RTL_NUMBER_OF(ExpectedInterfaces));
}
