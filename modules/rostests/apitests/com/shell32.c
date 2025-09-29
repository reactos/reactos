/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         COM interface test for shell32 classes
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 *                  Carl Bialorucki <carl.bialorucki@reactos.org>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces[] =
{
    {
        ID_NAME(CLSID_ACLCustomMRU, NTDDI_WIN7, NTDDI_MAX),
        {
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IEnumString },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IACList },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IACLCustomMRU },
        },
    },
    {
        ID_NAME(CLSID_ACLHistory, NTDDI_WIN7, NTDDI_MAX),
        {
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IEnumString },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IUnknown },
        },
    },
    {
        ID_NAME(CLSID_ACLMRU, NTDDI_WIN7, NTDDI_MAX),
        {
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IEnumString },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IACList },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IACLCustomMRU },
        },
    },
    {
        ID_NAME(CLSID_ACLMulti, NTDDI_WIN7, NTDDI_MAX),
        {
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IEnumString },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IObjMgr },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IACList },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IPersist },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IPersistFolder },
        }
    },
    {
        ID_NAME(CLSID_ACListISF, NTDDI_WIN7, NTDDI_MAX),
        {
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IEnumString },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IACList2 },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IACList },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_ICurrentWorkingDirectory },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IPersistFolder },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IPersistIDList },
        }
    },
    {
        ID_NAME(CLSID_ActiveDesktop, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IActiveDesktop },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IActiveDesktopP },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IADesktopP2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPropertyBag },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_AutoComplete, NTDDI_WIN7, NTDDI_MAX),
        {
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IAutoComplete2 },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IAutoComplete },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IAutoCompleteDropDown },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IEnumString },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IAccessible },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IDispatch },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IShellService },
        }
    },
    {
        ID_NAME(CLSID_BackgroundTaskScheduler, NTDDI_WIN7, NTDDI_VISTASP4),
        {
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IShellTaskScheduler },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IMarshal },
        }
    },
    {
        ID_NAME(CLSID_BandProxy, NTDDI_WIN7, NTDDI_MAX),
        {
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IServiceProvider },
        }
    },
    {
        ID_NAME(CLSID_CDBurn, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_ICDBurnPriv },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDiscMasterProgressEvents },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDriveFolderExtOld },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_ITransferAdviseSinkPriv },

            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IOleCommandTarget },

            { NTDDI_MIN,          NTDDI_MAX,          &IID_ICDBurn },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IContextMenu },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDropTarget },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_INamespaceWalkCB },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFile },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistPropertyBag },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IQueryCancelAutoPlay },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellExtInit },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellPropSheetExt },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },

            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IDriveFolderExt },
        }
    },
    {
        ID_NAME(CLSID_ControlPanel, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersist },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellIconOverlay },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolder },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolder2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },

            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IAliasRegistrationCallback },
            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IControlPanelEnumerator },
            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IRegItemCustomEnumerator },

            { NTDDI_VISTA,        NTDDI_WIN7SP1,      &IID_ITransferProvider },

            { NTDDI_VISTA,        NTDDI_WINBLUE,      &IID_IRegItemFolder },

            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IBackReferencedObject },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IContextMenuCB },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IDelegateHostItemContainer },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IFolderType },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IObjectWithBackReferences },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IRegItemCustomAttributes },

            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IShellIcon },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IStorage },
        }
    },
    {
        ID_NAME(CLSID_CopyToMenu, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IContextMenu2 },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IContextMenu3 },

            { NTDDI_MIN,          NTDDI_MAX,          &IID_IContextMenu },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IFolderFilter },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellExtInit },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_CRegTreeOptions, NTDDI_WIN7, NTDDI_MAX),
        {
            { NTDDI_WIN7,         NTDDI_WIN7SP1,      &IID_IRegTreeOptions },

            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_DeskMovr, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDeskMovr },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleObject },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersistPropertyBag },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleControl },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleInPlaceActiveObject },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleInPlaceObject },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleInPlaceObjectWindowless },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IQuickActivate },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IViewObjectEx },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IViewObject2 },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IViewObject },
        }
    },
    {
        ID_NAME(CLSID_DragDropHelper, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDragSourceHelper },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDropTargetHelper },
        }
    },
    {
        ID_NAME(CLSID_FadeTask, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_FileSearchBand, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleClientSite },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleControlSite },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleInPlaceSite },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDockingWindow },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDeskBand },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IInputObject },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IFileSearchBand },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDispatch },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersist },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersistStream },
        }
    },
    {
        ID_NAME(CLSID_FindFolder, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellFolder2 },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellFolder },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellIcon },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellIconOverlay },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersistFolder2 },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersistFolder },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersist },
        }
    },
    {
        ID_NAME(CLSID_FolderItem, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_FolderItem2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_FolderItem },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDispatch },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersist },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectSafety },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IParentAndItem },
        }
    },
    {
        ID_NAME(CLSID_FolderItemsFDF, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_IObjectSafety },

            { NTDDI_MIN,          NTDDI_MAX,          &IID_FolderItems3 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_FolderItems },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDispatch },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder },

            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IPersist },
        }
    },
    {
        ID_NAME(CLSID_FolderShortcut, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolder2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolder },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder3 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersist },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellLinkA },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellLinkW },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFile },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IExtractIconW },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IQueryInfo },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistStream },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistStreamInit },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistPropertyBag },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IBrowserFrameOptions },

            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IFolderWithSearchRoot },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IShellIconOverlay },
        }
    },
    {
        ID_NAME(CLSID_FolderViewHost, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IFolderView },

            { NTDDI_MIN,          NTDDI_MAX,          &IID_IFolderViewHost },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectWithSite },

            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IShellBrowserService4 },

            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IShellBrowser },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IExplorerBrowser },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_ICommDlgBrowser3 },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_ICommDlgBrowser2 },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_ICommDlgBrowser },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IOleInPlaceUIWindow },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IConnectionPointContainer },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IPersistHistory },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IPersist },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IInputObject },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IFolderFilterSite },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IUrlHistoryNotify },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IOleCommandTarget },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_INamespaceWalkCB2 },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_INamespaceWalkCB },

            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IShellBrowserService },
        }
    },
    {
        ID_NAME(CLSID_ISFBand, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDeskBand },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDockingWindow },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IInputObject },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistStream },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersist },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IWinEventHandler },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellChangeNotify },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDropTarget },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IContextMenu },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolderBand },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IFolderBandPriv },
        }
    },
    {
        ID_NAME(CLSID_MenuBand, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellMenuAcc },

            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDeskBand },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDockingWindow },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IInputObject },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistStream },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersist },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IMenuPopup },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDeskBar },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IMenuBand },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellMenu2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellMenu },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IWinEventHandler },
        }
    },
    {
        ID_NAME(CLSID_MenuBandSite, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IBandSite },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDeskBarClient },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IInputObject },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IInputObjectSite },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IWinEventHandler },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IServiceProvider },
        }
    },
    {
        ID_NAME(CLSID_MenuDeskBar, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDeskBar },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IInputObjectSite },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IInputObject },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IMenuPopup },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IBanneredBar },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IInitializeObject },
        }
    },
#if 0 // This is registered to shell32, but can't be instantiated
    {
        ID_NAME(CLSID_MenuToolbarBase, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IUnknown },
        }
    },
#endif
    {
        ID_NAME(CLSID_MergedFolder, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersistPropertyBag },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellIconOverlay },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_ICompositeFolder },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IItemNameLimits },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_ITranslateShellChangeNotify },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IAugmentedShellFolder2 },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IAugmentedShellFolder },

            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolder2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IStorage },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolder },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellService },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersist },

            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_ITranslateShellChangeNotify },

            { NTDDI_VISTA,        NTDDI_WIN7SP1,      &IID_ILocalizableItemParent },

            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IFolderType },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IObjectWithBackReferences },
        }
    },
    {
        ID_NAME(CLSID_MoveToMenu, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IContextMenu3 },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IContextMenu2 },

            { NTDDI_MIN,          NTDDI_MAX,          &IID_IContextMenu },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellExtInit },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IFolderFilter },
        }
    },
    {
        ID_NAME(CLSID_MruLongList, NTDDI_VISTA, NTDDI_WINBLUE),
        {
            { NTDDI_VISTA,        NTDDI_WIN7SP1,      &IID_IMruDataList },

            { NTDDI_VISTA,        NTDDI_WINBLUE,      &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_MruPidlList, NTDDI_VISTA, NTDDI_WINBLUE),
        {
            { NTDDI_VISTA,        NTDDI_WINBLUE,      &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_MyComputer, NTDDI_MIN, NTDDI_WINBLUE),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersistFreeThreadedObject },

            { NTDDI_MIN,          NTDDI_WINBLUE,      &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_WINBLUE,      &IID_IPersistFolder2 },
            { NTDDI_MIN,          NTDDI_WINBLUE,      &IID_IPersistFolder },
            { NTDDI_MIN,          NTDDI_WINBLUE,      &IID_IPersist },
            { NTDDI_MIN,          NTDDI_WINBLUE,      &IID_IShellIconOverlay },
            { NTDDI_MIN,          NTDDI_WINBLUE,      &IID_IShellFolder2 },
            { NTDDI_MIN,          NTDDI_WINBLUE,      &IID_IShellFolder },

            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IFolderWithSearchRoot },

            { NTDDI_VISTA,        NTDDI_WIN7SP1,      &IID_IFolderProperties },
            { NTDDI_VISTA,        NTDDI_WIN7SP1,      &IID_ITransferProvider },

            { NTDDI_VISTA,        NTDDI_WINBLUE,      &IID_IItemNameLimits },
            { NTDDI_VISTA,        NTDDI_WINBLUE,      &IID_IContextMenuCB },
            { NTDDI_VISTA,        NTDDI_WINBLUE,      &IID_INewItemAdvisor },
            { NTDDI_VISTA,        NTDDI_WINBLUE,      &IID_IObjectWithBackReferences },
            { NTDDI_VISTA,        NTDDI_WINBLUE,      &IID_IRegItemFolder },
            { NTDDI_VISTA,        NTDDI_WINBLUE,      &IID_IDelegateHostItemContainer },
            { NTDDI_VISTA,        NTDDI_WINBLUE,      &IID_IBackReferencedObject },

            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IShellIcon },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IStorage },

            { NTDDI_WIN8,         NTDDI_WINBLUE,      &IID_IObjectWithSite },
            { NTDDI_WIN8,         NTDDI_WINBLUE,      &IID_IPersistPropertyBag },
        }
    },
    {
        ID_NAME(CLSID_MyDocuments, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPropertySetStorage },

            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersist },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolder2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolder },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellIconOverlay },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellIcon },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder3 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IStorage },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IContextMenuCB },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IItemNameLimits },

            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IAliasRegistrationCallback },

            { NTDDI_VISTA,        NTDDI_WIN7SP1,      &IID_ILocalizableItemParent },

            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IPersistPropertyBag },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IObjectWithBackReferences },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IRemoteComputer },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IFolderType },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IBackReferencedObject },

            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IParentAndItem },
        }
    },
    {
        ID_NAME(CLSID_NetworkPlaces, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersistFreeThreadedObject },

            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder3 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersist },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellIconOverlay },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolder2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolder },

            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IFolderWithSearchRoot },

            { NTDDI_VISTA,        NTDDI_WIN7SP1,      &IID_ITransferProvider },

            { NTDDI_VISTA,        NTDDI_WINBLUE,      &IID_IRegItemFolder },

            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IContextMenuCB },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_INewItemAdvisor },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IObjectWithBackReferences },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IBackReferencedObject },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IDelegateHostItemContainer },

            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IShellIcon},
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IStorage},

            { NTDDI_WIN8,         NTDDI_MAX,          &IID_IObjectWithSite},
        }
    },
    {
        ID_NAME(CLSID_NewMenu, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IContextMenu3 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IContextMenu2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IContextMenu },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellExtInit },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_PersonalStartMenu, NTDDI_MIN, NTDDI_WINBLUE),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDeskBand },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDockingWindow },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IInputObject },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersistStream },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersist },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IMenuPopup },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDeskBar },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IMenuBand },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellMenu2 },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellMenu },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IWinEventHandler },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellMenuAcc },

            { NTDDI_MIN,          NTDDI_WINBLUE,      &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_WINBLUE,      &IID_IUnknown },

            { NTDDI_VISTA,        NTDDI_WINBLUE,      &IID_IShellItemFilter },
        }
    },
    {
        ID_NAME(CLSID_Printers, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IRemoteComputer },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IFolderNotify },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IShellFolder2 },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IShellFolder },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersistFolder2 },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersistFolder },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersist },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IContextMenuCB },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IShellIconOverlay },

            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IObjectWithBackReferences },
            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IResolveShellLink },
            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IFolderType },
            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IRegItemFolder },
            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_ITransferProvider },
            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IDelegateHostItemContainer },
            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IBackReferencedObject },
        }
    },
    {
        ID_NAME(CLSID_ProgressDialog, NTDDI_VISTA, NTDDI_MAX),
        {
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IProgressDialog },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IOleWindow },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IActionProgressDialog },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IActionProgress },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IObjectWithSite },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_QueryAssociations, NTDDI_MIN, NTDDI_WINBLUE),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IAssociationArrayOld },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IAssociationArrayInitialize },

            { NTDDI_MIN,          NTDDI_WINBLUE,      &IID_IQueryAssociations },
            { NTDDI_MIN,          NTDDI_WINBLUE,      &IID_IUnknown },

            { NTDDI_VISTA,        NTDDI_WIN7SP1,      &IID_IAssociationArray },
            { NTDDI_VISTA,        NTDDI_WIN7SP1,      &IID_IObjectWithAssociationList },
        }
    },
    {
        ID_NAME(CLSID_QuickLinks, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDeskBand },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDockingWindow },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IInputObject },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistStream },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersist },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IWinEventHandler },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellChangeNotify },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDropTarget },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IContextMenu },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolderBand },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IFolderBandPriv },
        }
    },
    {
        ID_NAME(CLSID_RecycleBin, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IContextMenu },

            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellPropSheetExt },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellExtInit },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolder2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolder },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },

            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IPersist },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IContextMenuCB },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IFolderType },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IObjectWithBackReferences },
  
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IShellIconOverlay },
        }
    },
    {
        ID_NAME(CLSID_SearchBand, NTDDI_WIN8, NTDDI_MAX),
        {
            { NTDDI_WIN8,         NTDDI_MAX,          &IID_IContextMenu },
            { NTDDI_WIN8,         NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN8,         NTDDI_MAX,          &IID_IShellExtInit },
            { NTDDI_WIN8,         NTDDI_MAX,          &IID_IPersistPropertyBag },
            { NTDDI_WIN8,         NTDDI_MAX,          &IID_IPersist },
            { NTDDI_WIN8,         NTDDI_MAX,          &IID_IShellFolder },
        }
    },
    {
        ID_NAME(CLSID_SendToMenu, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IContextMenu3 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IContextMenu2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IContextMenu },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellExtInit },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleWindow },

            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IServiceProvider },

            { NTDDI_WIN7,         NTDDI_MAX,          &IID_INamespaceWalkCB2 },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_INamespaceWalkCB },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IMarshal },
        }
    },
    {
        ID_NAME(CLSID_SharedTaskScheduler, NTDDI_WIN7, NTDDI_WINBLUE),
        {
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IShellTaskScheduler },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IUnknown },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IMarshal},
        }
    },
    {
        ID_NAME(CLSID_Shell, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellDispatch4 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellDispatch3 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellDispatch2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellDispatch },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDispatch },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectSafety },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectWithSite },

            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IShellDispatch5 },

            { NTDDI_WIN8,         NTDDI_MAX,          &IID_IShellDispatch6 },
        }
    },
    {
        /* CLSID_ShellDesktop is also registered to shell32 on Windows Vista, 7, and 8.1,
         * but instantiating it crashes the test on CoUninitialize. */
        ID_NAME(CLSID_ShellDesktop, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &CLSID_ShellDesktop },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IStorage },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersistFolder2 },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersistFolder },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersist },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellIcon },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IContextMenuCB },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_ITranslateShellChangeNotify },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IItemNameLimits },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellIconOverlay },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellFolder2 },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellFolder },
        }
    },
    {
        /* CLSID_ShellFSFolder is also registered to shell32 on
         * Windows 8.1, but instantiating it crashes the test on CoUninitialize. */
        ID_NAME(CLSID_ShellFSFolder, NTDDI_MIN, NTDDI_WIN7SP1),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersistFreeThreadedObject },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPropertySetStorage },

            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_IShellFolder2 },
            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_IShellFolder },
            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_IShellIcon },
            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_IShellIconOverlay },
            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_IPersistFolder3 },
            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_IPersistFolder2 },
            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_IPersistFolder },
            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_IPersist },
            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_IStorage },
            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_IItemNameLimits },
            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_IContextMenuCB },
            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_IOleCommandTarget },

            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IAliasRegistrationCallback },

            { NTDDI_VISTA,        NTDDI_WIN7SP1,      &IID_IObjectWithSite },
            { NTDDI_VISTA,        NTDDI_WIN7SP1,      &IID_ILocalizableItemParent },
            { NTDDI_VISTA,        NTDDI_WIN7SP1,      &IID_IPersistPropertyBag },
            { NTDDI_VISTA,        NTDDI_WIN7SP1,      &IID_IObjectWithBackReferences },
            { NTDDI_VISTA,        NTDDI_WIN7SP1,      &IID_IRemoteComputer },
            { NTDDI_VISTA,        NTDDI_WIN7SP1,      &IID_IFolderType },
            { NTDDI_VISTA,        NTDDI_WIN7SP1,      &IID_IBackReferencedObject },

            { NTDDI_WIN7,         NTDDI_WIN7SP1,      &IID_IParentAndItem },
        }
    },
    {
        ID_NAME(CLSID_ShellFldSetExt, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellPropSheetExt },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellExtInit },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_ShellFolderView, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolderViewDual2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolderViewDual },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDispatch },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellService },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectSafety },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IConnectionPointContainer },

            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IShellFolderViewDual3 },
        }
    },
    {
        ID_NAME(CLSID_ShellFolderViewOC, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IFolderViewOC },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IDispatch },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IProvideClassInfo2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IProvideClassInfo },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectSafety },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IConnectionPointContainer },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistStreamInit },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersist },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleControl },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleObject },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleInPlaceActiveObject },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleInPlaceObjectWindowless },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleInPlaceObject },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IOleWindow },
        }
    },
    /* CLSID_ShellItem has two entries because the threading model changed between versions. */
    {
        ID_NAME(CLSID_ShellItem, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IShellItem },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersistIDList },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IParentAndItem },

            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IShellItem2 },
            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IMarshal },
            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IPersistStream },
            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IPersist },
        }
    },
    {
        ID_NAME(CLSID_ShellItem, NTDDI_WIN7, NTDDI_WINBLUE),
        {
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IMarshal },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IShellItem2 },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IShellItem },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IPersistIDList },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IUnknown },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IParentAndItem },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IPersistStream },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IPersist },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IObjectWithBackReferences },
        },
        L"Both",
    },
    /* CLSID_ShellLink has two entries because the threading model changed between versions. */
    {
        ID_NAME(CLSID_ShellLink, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IShellLinkA },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IShellLinkW },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersistStream },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersistFile },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IShellExtInit },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IContextMenu3 },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IContextMenu2 },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IContextMenu },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDropTarget },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IQueryInfo },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IShellLinkDataList },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IExtractIconA },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IExtractIconW },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IExtractImage2 },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IExtractImage },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersistPropertyBag },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IFilter },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_ICustomizeInfoTip },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_ISLTracker },

            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IPersist },
            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IPropertyStore },
            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IPropertyBag },
        }
    },
    {
        ID_NAME(CLSID_ShellLink, NTDDI_WIN7, NTDDI_WINBLUE),
        {
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IMarshal },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IShellLinkA },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IShellLinkW },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IPersistStream },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IPersistFile },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IPersist },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IShellExtInit },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IContextMenu3 },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IContextMenu2 },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IContextMenu },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IDropTarget },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IQueryInfo },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IShellLinkDataList },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IExtractIconA },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IExtractIconW },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IUnknown },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IExtractImage2 },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IExtractImage },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IPersistPropertyBag },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IServiceProvider },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IObjectWithSite },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IPropertyStore },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_ICustomizeInfoTip },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_ISLTracker },
        },
        L"Both",
    },
#if 0 // Apparently we can only get this through Folder.Items().GetLink
    {
        ID_NAME(CLSID_ShellLinkObject, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
        }
    },
#endif
    {
        ID_NAME(CLSID_ShellSearchExt, NTDDI_WIN7, NTDDI_MAX),
        {
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IContextMenu },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_ShellTaskScheduler, NTDDI_WIN7, NTDDI_WINBLUE),
        {
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IShellTaskScheduler },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IUnknown },
            { NTDDI_WIN7,         NTDDI_WINBLUE,      &IID_IMarshal},
        },
    },
    {
        /* CLSID_StartMenu is also registered to shell32 on Windows Vista,
         * but it crashes the test on CoUninitialize. */
        ID_NAME(CLSID_StartMenu, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDeskBar },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IInputObjectSite },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IInputObject },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IMenuPopup },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IBanneredBar },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IInitializeObject },
        }
    },
    {
        ID_NAME(CLSID_StartMenuPin, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellExtInit },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IContextMenu },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectWithSite },

            {NTDDI_VISTA,         NTDDI_VISTASP4,     &IID_IPinnedListOld},

            {NTDDI_WIN7,          NTDDI_WINBLUE,      &IID_IPinnedList},
        }
    },
    {
        ID_NAME(CLSID_Thumbnail, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },

            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IParentAndItem },
        },
    },
    {
        ID_NAME(CLSID_TrackShellMenu, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_ITrackShellMenu },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellMenu },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellMenu2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IServiceProvider },
        }
    },
    {
        ID_NAME(CLSID_UserAssist, NTDDI_WIN7, NTDDI_MAX),
        {
            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IUnknown },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_UserNotification, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUserNotification },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },

            {NTDDI_VISTA,         NTDDI_MAX,          &IID_IUserNotification2 },
        }
    },
};

START_TEST(shell32)
{
    TestClasses(L"shell32", ExpectedInterfaces, RTL_NUMBER_OF(ExpectedInterfaces));
}
