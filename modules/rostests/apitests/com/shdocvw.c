/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         COM interface test for shdocvw classes
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces[] =
{
    /* CLSID_AdminFolderShortcut has two entries here because the threading model
     * changed between Windows versions. */
    {
        ID_NAME(CLSID_AdminFolderShortcut, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellLinkA },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellLinkW },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersistFile },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IExtractIconW },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IQueryInfo },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersistStream },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersistStreamInit },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IBrowserFrameOptions },

            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolder2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolder },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder3 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersist },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistPropertyBag },

            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IAliasRegistrationCallback },

            { NTDDI_VISTA,        NTDDI_WIN7SP1,      &IID_ILocalizableItemParent },

            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IShellIcon },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IShellIconOverlay },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IStorage },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IItemNameLimits },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IContextMenuCB },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IOleCommandTarget },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IOleCommandTarget },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IObjectWithBackReferences },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IRemoteComputer },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IFolderType },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IBackReferencedObject },
        }
    },
    {
        ID_NAME(CLSID_AdminFolderShortcut, NTDDI_WIN7, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolder2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IShellFolder },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder3 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder2 },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistFolder },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersist },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IPersistPropertyBag },

            { NTDDI_VISTA,        NTDDI_WIN7SP1,      &IID_ILocalizableItemParent },

            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IObjectWithSite },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IShellIcon },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IShellIconOverlay },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IStorage },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IItemNameLimits },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IContextMenuCB },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IOleCommandTarget },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IOleCommandTarget },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IObjectWithBackReferences },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IRemoteComputer },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IFolderType },
            { NTDDI_VISTA,        NTDDI_MAX,          &IID_IBackReferencedObject },

            { NTDDI_WIN7,         NTDDI_MAX,          &IID_IParentAndItem },
        },
        L"Both"
    },
    {
        ID_NAME(CLSID_ExplorerBand, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,         NTDDI_WS03SP4,       &IID_IDeskBand },
            { NTDDI_MIN,         NTDDI_WS03SP4,       &IID_IDockingWindow },
            { NTDDI_MIN,         NTDDI_WS03SP4,       &IID_IOleWindow },
            { NTDDI_MIN,         NTDDI_WS03SP4,       &IID_IObjectWithSite },
            { NTDDI_MIN,         NTDDI_WS03SP4,       &IID_IInputObject },
            { NTDDI_MIN,         NTDDI_WS03SP4,       &IID_IPersistStream },
            { NTDDI_MIN,         NTDDI_WS03SP4,       &IID_IPersist },
            { NTDDI_MIN,         NTDDI_WS03SP4,       &IID_IOleCommandTarget },
            { NTDDI_MIN,         NTDDI_WS03SP4,       &IID_IServiceProvider },
            { NTDDI_MIN,         NTDDI_WS03SP4,       &IID_IContextMenu },
            { NTDDI_MIN,         NTDDI_WS03SP4,       &IID_IBandNavigate },
            { NTDDI_MIN,         NTDDI_WS03SP4,       &IID_IWinEventHandler },
            { NTDDI_MIN,         NTDDI_WS03SP4,       &IID_INamespaceProxy },
            { NTDDI_MIN,         NTDDI_WS03SP4,       &IID_IDispatch },
            { NTDDI_MIN,         NTDDI_WS03SP4,       &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_FontsFolderShortcut, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,         NTDDI_VISTASP4,      &IID_IShellFolder2 },
            { NTDDI_MIN,         NTDDI_VISTASP4,      &IID_IShellFolder },
            { NTDDI_MIN,         NTDDI_VISTASP4,      &IID_IUnknown },
            { NTDDI_MIN,         NTDDI_VISTASP4,      &IID_IPersistFolder3 },
            { NTDDI_MIN,         NTDDI_VISTASP4,      &IID_IPersistFolder2 },
            { NTDDI_MIN,         NTDDI_VISTASP4,      &IID_IPersistFolder },
            { NTDDI_MIN,         NTDDI_VISTASP4,      &IID_IPersist },
            { NTDDI_MIN,         NTDDI_VISTASP4,      &IID_IShellLinkA },
            { NTDDI_MIN,         NTDDI_VISTASP4,      &IID_IShellLinkW },
            { NTDDI_MIN,         NTDDI_VISTASP4,      &IID_IPersistFile },
            { NTDDI_MIN,         NTDDI_VISTASP4,      &IID_IExtractIconW },
            { NTDDI_MIN,         NTDDI_VISTASP4,      &IID_IQueryInfo },
            { NTDDI_MIN,         NTDDI_VISTASP4,      &IID_IPersistStream },
            { NTDDI_MIN,         NTDDI_VISTASP4,      &IID_IPersistStreamInit },
            { NTDDI_MIN,         NTDDI_VISTASP4,      &IID_IPersistPropertyBag },
            { NTDDI_MIN,         NTDDI_VISTASP4,      &IID_IBrowserFrameOptions },

            { NTDDI_VISTA,       NTDDI_VISTASP4,      &IID_IFolderWithSearchRoot },
            { NTDDI_VISTA,       NTDDI_VISTASP4,      &IID_IShellIconOverlay },
        }
    },
#if 0 // E_OUTOFMEMORY?
    {
        ID_NAME(CLSID_ShellDispatchInproc, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IUnknown },
        }
    },
#endif
    {
        ID_NAME(CLSID_MruLongList, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IMruDataList },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_MruPidlList, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IMruPidlList },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_SH_FavBand, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDeskBand },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDockingWindow },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IInputObject },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersistStream },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersist },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IContextMenu },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IBandNavigate },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IWinEventHandler },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_INamespaceProxy },
        }
    },
    {
        ID_NAME(CLSID_SH_HistBand, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDeskBand },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDockingWindow },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IInputObject },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersistStream },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IPersist },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IServiceProvider },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IContextMenu },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IBandNavigate },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IWinEventHandler },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_INamespaceProxy },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellFolderSearchableCallback },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_SearchAssistantOC, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_ISearchAssistantOC3 },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_ISearchAssistantOC },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDispatch },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IProvideClassInfo2 },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IProvideClassInfo },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IQuickActivate },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleControl },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleObject },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleInPlaceActiveObject },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IViewObjectEx },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IViewObject2 },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IViewObject },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleInPlaceObjectWindowless },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleInPlaceObject },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDataObject },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IConnectionPointContainer },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IObjectSafety },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_SearchBand, NTDDI_MIN, NTDDI_WIN7SP1),
        {
            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_IContextMenu },
            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_IShellExtInit },
            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_IPersistPropertyBag },
            { NTDDI_MIN,          NTDDI_WIN7SP1,      &IID_IPersist },

            { NTDDI_VISTA,        NTDDI_WIN7SP1,      &IID_IShellFolder },
        }
    },
    {
        ID_NAME(CLSID_ShellSearchAssistantOC, NTDDI_MIN, NTDDI_WS03SP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_ISearchAssistantOC3 },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_ISearchAssistantOC },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDispatch },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IProvideClassInfo2 },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IProvideClassInfo },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IQuickActivate },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleControl },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleObject },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleInPlaceActiveObject },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IViewObjectEx },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IViewObject2 },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IViewObject },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleInPlaceObjectWindowless },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleInPlaceObject },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IDataObject },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IConnectionPointContainer },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IObjectSafety },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IOleCommandTarget },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_ShellShellNameSpace, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_IShellChangeNotify },
            { NTDDI_MIN,          NTDDI_WS03SP4,      &IID_ISpecifyPropertyPages },

            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IShellNameSpace },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IShellFavoritesNameSpace },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDispatch },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IProvideClassInfo2 },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IProvideClassInfo },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersistStreamInit },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IPersistPropertyBag },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IQuickActivate },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IOleControl },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IOleObject },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IOleInPlaceActiveObject },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IViewObjectEx },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IViewObject2 },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IViewObject },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IOleInPlaceObjectWindowless },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IOleInPlaceObject },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IOleWindow },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IConnectionPointContainer },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IDropTarget },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IObjectWithSite },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_INSCTree2 },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_INSCTree },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IWinEventHandler },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IShellBrowser },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IFolderFilterSite },

            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_INewMenuClient },
            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IServiceProvider },
            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_INameSpaceTreeControl },
            { NTDDI_VISTA,        NTDDI_VISTASP4,     &IID_IVisualProperties },
        }
    },
    {
        ID_NAME(CLSID_TaskbarList, NTDDI_MIN, NTDDI_VISTASP4),
        {
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_ITaskbarList2 },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_ITaskbarList },
            { NTDDI_MIN,          NTDDI_VISTASP4,     &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_AttachmentServices, NTDDI_MIN, NTDDI_MAX),
        {
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IAttachmentExecute },
            { NTDDI_MIN,          NTDDI_MAX,          &IID_IUnknown },
        }
    },
};

START_TEST(shdocvw)
{
    TestClasses(L"shdocvw", ExpectedInterfaces, RTL_NUMBER_OF(ExpectedInterfaces));
}
