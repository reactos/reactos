/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         COM interface test for shell32 classes
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces_WS03[] =
{
    {
        ID_NAME(CLSID_ActiveDesktop),
        {
            {    0x0,    0x0,   &IID_IActiveDesktop },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IActiveDesktopP },
            {    0x8,   0x10,   &IID_IADesktopP2 },
            {    0xc,   0x18,   &IID_IPropertyBag },
        }
    },
    {
        ID_NAME(CLSID_CDBurn),
        {
            {    0x0,    0x0,   &IID_IObjectWithSite },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x8,   0x10,   &IID_IShellExtInit },
            {    0xc,   0x18,   &IID_IContextMenu },
            {   0x10,   0x20,   &IID_IShellPropSheetExt },
            {   0x14,   0x28,   &IID_IDiscMasterProgressEvents },
            {   0x18,   0x30,   &IID_IDropTarget },
            {   0x1c,   0x38,   &IID_IPersistFile },
            {   0x20,   0x40,   &IID_IOleCommandTarget },
            {   0x24,   0x48,   &IID_ICDBurn },
            {   0x28,   0x50,   &IID_ICDBurnPriv },
            {   0x2c,   0x58,   &IID_IPersistPropertyBag },
            {   0x30,   0x60,   &IID_IDriveFolderExtOld },
            {   0x34,   0x68,   &IID_INamespaceWalkCB },
            {   0x3c,   0x78,   &IID_IServiceProvider },
            {   0x40,   0x80,   &IID_ITransferAdviseSinkPriv },
            {   0x44,   0x88,   &IID_IQueryCancelAutoPlay },
        }
    },
    {
        ID_NAME(CLSID_ControlPanel),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {   0x10,   0x20,   &IID_IPersistFolder2 },
            {   0x10,   0x20,       &IID_IPersistFolder },
            {   0x10,   0x20,           &IID_IPersist },
            { FARAWY, FARAWY,   &IID_IShellIconOverlay },
            { FARAWY, FARAWY,   &IID_IShellFolder2 },
            { FARAWY, FARAWY,       &IID_IShellFolder },
        }
    },
    {
        ID_NAME(CLSID_CopyToMenu),
        {
            {    0x0,    0x0,   &IID_IContextMenu3 },
            {    0x0,    0x0,       &IID_IContextMenu2 },
            {    0x0,    0x0,           &IID_IContextMenu },
            {    0x0,    0x0,               &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellExtInit },
            {    0x8,   0x10,   &IID_IObjectWithSite },
            {   0x10,   0x20,   &IID_IFolderFilter },
        }
    },
    {
        ID_NAME(CLSID_DeskMovr),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {   0x70,   0xc0,   &IID_IDeskMovr },
            {   0x74,   0xc8,   &IID_IOleObject },
            {   0x78,   0xd0,   &IID_IPersistPropertyBag },
            {   0x7c,   0xd8,   &IID_IOleControl },
            {   0x80,   0xe0,   &IID_IOleInPlaceActiveObject },
            {   0x84,   0xe8,   &IID_IViewObjectEx },
            {   0x84,   0xe8,       &IID_IViewObject2 },
            {   0x84,   0xe8,           &IID_IViewObject },
            {   0x88,   0xf0,   &IID_IOleWindow },
            {   0x88,   0xf0,       &IID_IOleInPlaceObject },
            {   0x88,   0xf0,           &IID_IOleInPlaceObjectWindowless },
            {   0x8c,   0xf8,   &IID_IQuickActivate },
        }
    },
    {
        ID_NAME(CLSID_DragDropHelper),
        {
            {    0x0,    0x0,   &IID_IDragSourceHelper },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IDropTargetHelper },
        }
    },
    {
        ID_NAME(CLSID_FadeTask),
        {
            {    0x0,    0x0,   &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_FileSearchBand),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {   0x74,   0xa8,   &IID_IOleContainer},
            {   0x78,   0xb0,   &IID_IOleInPlaceFrame},
            {   0x78,   0xb0,       &IID_IOleWindow },
            {   0xb0,  0x118,   &IID_IDockingWindow },
            {   0xb0,  0x118,       &IID_IDeskBand },
            {   0xb4,  0x120,   &IID_IInputObject },
            {   0xb8,  0x128,   &IID_IObjectWithSite },
            {   0xbc,  0x130,   &IID_IContextMenu},
            {   0xc0,  0x138,   &IID_IServiceProvider},
            {   0xc8,  0x148,   &IID_ISearchBar},
            {   0xcc,  0x150,   &IID_IFileSearchBand },
            {   0xcc,  0x150,       &IID_IDispatch },
            {   0xd0,  0x158,   &IID_IBandNavigate},
            {   0xd8,  0x168,   &IID_IPersist },
        }
    },
    {
        ID_NAME(CLSID_FindFolder),
        {
            {    0x0,    0x0,   &IID_IShellFolder2 },
            {    0x0,    0x0,       &IID_IShellFolder },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellIcon },
            {    0x8,   0x10,   &IID_IShellIconOverlay },
            {    0xc,   0x18,   &IID_IPersistFolder2 },
            {    0xc,   0x18,       &IID_IPersistFolder },
            {    0xc,   0x18,           &IID_IPersist },
        }
    },
    {
        ID_NAME(CLSID_FolderItem),
        {
            //{    0x0,    0x0,   &CLSID_ShellFolderItem }, // broken QueryInterface that doesn't add a reference
            {    0x0,    0x0,       &IID_FolderItem2 },
            {    0x0,    0x0,           &IID_FolderItem },
            {    0x0,    0x0,               &IID_IDispatch },
            {    0x0,    0x0,                   &IID_IUnknown },
            {    0x4,    0x8,   &IID_IPersistFolder2 },
            {    0x4,    0x8,       &IID_IPersistFolder },
            {    0x4,    0x8,           &IID_IPersist },
            {    0x8,   0x10,   &IID_IObjectSafety },
            {   0x10,   0x20,   &IID_IParentAndItem },
        }
    },
    {
        ID_NAME(CLSID_FolderItemsFDF),
        {
            {    0x0,    0x0,   &IID_FolderItems3 },
            //{    0x0,    0x0,       &IID_FolderItems2 }, ????
            {    0x0,    0x0,           &IID_FolderItems },
            {    0x0,    0x0,               &IID_IDispatch },
            {    0x0,    0x0,                   &IID_IUnknown },
            {    0x4,    0x8,   &IID_IPersistFolder },
            {    0x8,   0x10,   &IID_IObjectSafety },
        }
    },
    {
        ID_NAME(CLSID_FolderShortcut),
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
        ID_NAME(CLSID_FolderViewHost),
        {
            {    0x0,    0x0,   &IID_IFolderViewHost },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IServiceProvider },
            {    0x8,   0x10,   &IID_IOleWindow },
            {    0xc,   0x18,   &IID_IFolderView },
            {   0x10,   0x20,   &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_ISFBand),
        {
            {  -0xac, -0x100,   &IID_IDeskBand },
            {  -0xac, -0x100,       &IID_IDockingWindow },
            {  -0xac, -0x100,           &IID_IOleWindow },
            {  -0xa8,  -0xf8,   &IID_IObjectWithSite },
            {  -0xa0,  -0xe8,   &IID_IInputObject },
            {  -0x9c,  -0xe0,   &IID_IPersistStream },
            {  -0x9c,  -0xe0,       &IID_IPersist },
            {  -0x98,  -0xd8,   &IID_IOleCommandTarget },
            {  -0x94,  -0xd0,   &IID_IServiceProvider },
            {  -0x78,  -0xa0,   &IID_IWinEventHandler },
            {  -0x74,  -0x98,   &IID_IShellChangeNotify },
            {  -0x70,  -0x90,   &IID_IDropTarget },
            {   -0x4,   -0x8,   &IID_IContextMenu },
            {    0x0,    0x0,   &IID_IShellFolderBand },
            {    0x0,    0x0,       &IID_IUnknown },
            {   0x94,   0xf8,   &IID_IFolderBandPriv },
        }
    },
    {
        ID_NAME(CLSID_MenuBand),
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
            {    0x0,    0x0,   &IID_IMenuPopup },
            {    0x0,    0x0,       &IID_IDeskBar },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IMenuBand },
            {    0x8,   0x10,   &IID_IShellMenu2 },
            {    0x8,   0x10,       &IID_IShellMenu },
            {    0xc,   0x18,   &IID_IWinEventHandler },
            {   0x10,   0x20,   &IID_IShellMenuAcc },
        }
    },
    {
        ID_NAME(CLSID_MenuBandSite),
        {
            {    0x0,    0x0,   &IID_IBandSite },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IDeskBarClient },
            {    0x4,    0x8,       &IID_IOleWindow },
            {    0x8,   0x10,   &IID_IOleCommandTarget },
            {    0xc,   0x18,   &IID_IInputObject },
            {   0x10,   0x20,   &IID_IInputObjectSite },
            {   0x14,   0x28,   &IID_IWinEventHandler },
            {   0x18,   0x30,   &IID_IServiceProvider },
        }
    },
    {
        ID_NAME(CLSID_MenuDeskBar),
        {
            {  -0x48,  -0x80,   &IID_IOleCommandTarget },
            {  -0x44,  -0x78,   &IID_IServiceProvider },
            {  -0x40,  -0x70,   &IID_IDeskBar },
            {  -0x40,  -0x70,       &IID_IOleWindow },
            {  -0x3c,  -0x68,   &IID_IInputObjectSite },
            {  -0x38,  -0x60,   &IID_IInputObject },
            {    0x0,    0x0,   &IID_IMenuPopup },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IObjectWithSite },
            {    0x8,   0x10,   &IID_IBanneredBar },
            {    0xc,   0x18,   &IID_IInitializeObject },
        }
    },
#if 0 // This is registered to shell32, but can't be instantiated
    {
        ID_NAME_CANNOT_INSTANTIATE(CLSID_MenuToolbarBase),
        {
            {    0x0,    0x0,   &IID_IUnknown },
        }
    },
#endif
    {
        ID_NAME(CLSID_MergedFolder),
        {
            {   -0x8,  -0x10,   &IID_IShellFolder2 },
            {   -0x4,   -0x8,   &IID_IStorage },
            {    0x0,    0x0,   &IID_IAugmentedShellFolder2 },
            {    0x0,    0x0,       &IID_IAugmentedShellFolder },
            {    0x0,    0x0,           &IID_IShellFolder },
            {    0x0,    0x0,               &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellService },
            {    0x8,   0x10,   &IID_ITranslateShellChangeNotify },
            {    0xc,   0x18,   &IID_IPersistFolder2 },
            {    0xc,   0x18,       &IID_IPersistFolder },
            {    0xc,   0x18,           &IID_IPersist },
            {   0x10,   0x20,   &IID_IPersistPropertyBag },
            {   0x14,   0x28,   &IID_IShellIconOverlay },
            {   0x18,   0x30,   &IID_ICompositeFolder },
            {   0x1c,   0x38,   &IID_IItemNameLimits },
        }
    },
    {
        ID_NAME(CLSID_MoveToMenu),
        {
            {    0x0,    0x0,   &IID_IContextMenu3 },
            {    0x0,    0x0,       &IID_IContextMenu2 },
            {    0x0,    0x0,           &IID_IContextMenu },
            {    0x0,    0x0,               &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellExtInit },
            {    0x8,   0x10,   &IID_IObjectWithSite },
            {   0x10,   0x20,   &IID_IFolderFilter },
        }
    },
    {
        ID_NAME(CLSID_MyComputer),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {   0x10,   0x20,   &IID_IPersistFolder2 },
            {   0x10,   0x20,       &IID_IPersistFolder },
            {   0x10,   0x20,           &IID_IPersist },
            {   0x10,   0x20,           &IID_IPersistFreeThreadedObject },
            { FARAWY, FARAWY,   &IID_IShellIconOverlay },
            { FARAWY, FARAWY,   &IID_IShellFolder2 },
            { FARAWY, FARAWY,       &IID_IShellFolder },
        }
    },
    {
        ID_NAME(CLSID_MyDocuments),
        {
            {   -0x4,   -0x8,   &IID_IPersistFolder },
            {   -0x4,   -0x8,       &IID_IPersist },
            {    0x0,    0x0,   &IID_IShellFolder2 },
            {    0x0,    0x0,       &IID_IShellFolder },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellIconOverlay },
            { FARAWY, FARAWY,   &IID_IShellIcon },
            { FARAWY, FARAWY,   &IID_IPersistFolder3 },
            { FARAWY, FARAWY,       &IID_IPersistFolder2 },
            { FARAWY, FARAWY,   &IID_IStorage },
            { FARAWY, FARAWY,   &IID_IContextMenuCB },
            { FARAWY, FARAWY,   &IID_IOleCommandTarget },
            { FARAWY, FARAWY,   &IID_IItemNameLimits },
            { FARAWY, FARAWY,   &IID_IPropertySetStorage },
        }
    },
    {
        ID_NAME(CLSID_NetworkPlaces),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {   0x10,   0x20,   &IID_IPersistFolder3 },
            {   0x10,   0x20,       &IID_IPersistFolder2 },
            {   0x10,   0x20,           &IID_IPersistFolder },
            {   0x10,   0x20,               &IID_IPersist },
            {   0x10,   0x20,               &IID_IPersistFreeThreadedObject },
            { FARAWY, FARAWY,   &IID_IShellIconOverlay },
            { FARAWY, FARAWY,   &IID_IShellFolder2 },
            { FARAWY, FARAWY,       &IID_IShellFolder },
        }
    },
    {
        ID_NAME(CLSID_NewMenu),
        {
            {   -0xc,  -0x18,   &IID_IObjectWithSite },
            {   -0x4,   -0x8,   &IID_IContextMenu3 },
            {   -0x4,   -0x8,       &IID_IContextMenu2 },
            {   -0x4,   -0x8,           &IID_IContextMenu },
            {    0x0,    0x0,   &IID_IShellExtInit },
            {    0x0,    0x0,       &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_PersonalStartMenu),
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
            {    0x0,    0x0,   &IID_IMenuPopup },
            {    0x0,    0x0,       &IID_IDeskBar },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IMenuBand },
            {    0x8,   0x10,   &IID_IShellMenu2 },
            {    0x8,   0x10,       &IID_IShellMenu },
            {    0xc,   0x18,   &IID_IWinEventHandler },
            {   0x10,   0x20,   &IID_IShellMenuAcc },
        }
    },
    {
        ID_NAME(CLSID_Printers),
        {
            {   -0xc,  -0x18,   &IID_IRemoteComputer },
            {   -0x4,   -0x8,   &IID_IFolderNotify },
            {    0x0,    0x0,   &IID_IShellFolder2 },
            {    0x0,    0x0,       &IID_IShellFolder },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IPersistFolder2 },
            {    0x4,    0x8,       &IID_IPersistFolder },
            {    0x4,    0x8,           &IID_IPersist },
            {    0x8,   0x10,   &IID_IContextMenuCB },
            {    0xc,   0x18,   &IID_IShellIconOverlay },
        }
    },
    {
        ID_NAME(CLSID_QueryAssociations),
        {
            {    0x0,    0x0,   &IID_IAssociationArrayOld },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IAssociationArrayInitialize },
            {    0x8,   0x10,   &IID_IQueryAssociations },
        }
    },
    {
        ID_NAME(CLSID_QuickLinks),
        {
            {  -0xac, -0x100,   &IID_IDeskBand },
            {  -0xac, -0x100,       &IID_IDockingWindow },
            {  -0xac, -0x100,           &IID_IOleWindow },
            {  -0xa8,  -0xf8,   &IID_IObjectWithSite },
            {  -0xa0,  -0xe8,   &IID_IInputObject },
            {  -0x9c,  -0xe0,   &IID_IPersistStream },
            {  -0x9c,  -0xe0,       &IID_IPersist },
            {  -0x98,  -0xd8,   &IID_IOleCommandTarget },
            {  -0x94,  -0xd0,   &IID_IServiceProvider },
            {  -0x78,  -0xa0,   &IID_IWinEventHandler },
            {  -0x74,  -0x98,   &IID_IShellChangeNotify },
            {  -0x70,  -0x90,   &IID_IDropTarget },
            {   -0x4,   -0x8,   &IID_IContextMenu },
            {    0x0,    0x0,   &IID_IShellFolderBand },
            {    0x0,    0x0,       &IID_IUnknown },
            {   0x94,   0xf8,   &IID_IFolderBandPriv },
        }
    },
    {
        ID_NAME(CLSID_RecycleBin),
        {
            {    0x0,    0x0,   &IID_IPersistFolder2 },
            {    0x0,    0x0,       &IID_IPersistFolder },
            //{    0x0,    0x0,           &IID_IPersist },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellFolder2 },
            {    0x4,    0x8,       &IID_IShellFolder },
            {    0x8,   0x10,   &IID_IContextMenu },
            {    0xc,   0x18,   &IID_IShellPropSheetExt },
            {   0x10,   0x20,   &IID_IShellExtInit },
        }
    },
    {
        ID_NAME(CLSID_SendToMenu),
        {
            {   -0x4,   -0x8,   &IID_IContextMenu3 },
            {   -0x4,   -0x8,       &IID_IContextMenu2 },
            {   -0x4,   -0x8,           &IID_IContextMenu },
            {    0x0,    0x0,   &IID_IShellExtInit },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IOleWindow },
        }
    },
    {
        ID_NAME(CLSID_Shell),
        {
            {    0x0,    0x0,   &IID_IShellDispatch4 },
            {    0x0,    0x0,       &IID_IShellDispatch3 },
            {    0x0,    0x0,           &IID_IShellDispatch2 },
            {    0x0,    0x0,               &IID_IShellDispatch },
            {    0x0,    0x0,                   &IID_IDispatch },
            {    0x0,    0x0,                       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IObjectSafety },
            {   0x20,   0x40,   &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_ShellDesktop),
        {
            {   -0x8,  -0x10,   &CLSID_ShellDesktop },
            {   -0x8,  -0x10,       &IID_IObjectWithSite },
            {    0x0,    0x0,   &IID_IUnknown },
            {    0x4,    0x8,   &IID_IStorage },
            {    0x8,   0x10,   &IID_IPersistFolder2 },
            {    0x8,   0x10,       &IID_IPersistFolder },
            {    0x8,   0x10,           &IID_IPersist },
            {    0xc,   0x18,   &IID_IShellIcon },
            {   0x14,   0x28,   &IID_IContextMenuCB },
            {   0x18,   0x30,   &IID_ITranslateShellChangeNotify },
            {   0x1c,   0x38,   &IID_IItemNameLimits },
            {   0x20,   0x40,   &IID_IOleCommandTarget },
            { FARAWY, FARAWY,   &IID_IShellIconOverlay },
            { FARAWY, FARAWY,   &IID_IShellFolder2 },
            { FARAWY, FARAWY,       &IID_IShellFolder },
        }
    },
    {
        ID_NAME(CLSID_ShellFSFolder),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {    0xc,   0x18,   &IID_IShellFolder2 },
            {    0xc,   0x18,       &IID_IShellFolder },
            {   0x10,   0x20,   &IID_IShellIcon },
            {   0x14,   0x28,   &IID_IShellIconOverlay },
            {   0x18,   0x30,   &IID_IPersistFolder3 },
            {   0x18,   0x30,       &IID_IPersistFolder2 },
            {   0x18,   0x30,           &IID_IPersistFolder },
            {   0x18,   0x30,               &IID_IPersist },
            {   0x18,   0x30,               &IID_IPersistFreeThreadedObject },
            {   0x1c,   0x38,   &IID_IStorage },
            {   0x24,   0x48,   &IID_IPropertySetStorage },
            {   0x28,   0x50,   &IID_IItemNameLimits },
            {   0x2c,   0x58,   &IID_IContextMenuCB },
            {   0x34,   0x68,   &IID_IOleCommandTarget },
        }
    },
    {
        ID_NAME(CLSID_ShellFldSetExt),
        {
            {    0x0,    0x0,   &IID_IShellPropSheetExt },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellExtInit },
            {    0x8,   0x10,   &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_ShellFolderView),
        {
            {    0x0,    0x0,   &IID_IShellFolderViewDual2 },
            {    0x0,    0x0,       &IID_IShellFolderViewDual },
            {    0x0,    0x0,           &IID_IDispatch },
            {    0x0,    0x0,               &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellService },
            {    0x8,   0x10,   &IID_IServiceProvider },
            {    0xc,   0x18,   &IID_IObjectSafety },
            {   0x14,   0x28,   &IID_IObjectWithSite },
            {   0x1c,   0x38,   &IID_IConnectionPointContainer },
        }
    },
    {
        ID_NAME(CLSID_ShellFolderViewOC),
        {
            {    0x0,    0x0,   &IID_IFolderViewOC },
            {    0x0,    0x0,       &IID_IDispatch },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IProvideClassInfo2 },
            {    0x4,    0x8,       &IID_IProvideClassInfo },
            {    0x8,   0x10,   &IID_IObjectSafety },
            {   0x10,   0x20,   &IID_IConnectionPointContainer },
            {   0x88,   0xf0,   &IID_IPersistStreamInit },
            {   0x88,   0xf0,       &IID_IPersist },
            {   0x8c,   0xf8,   &IID_IOleControl },
            {   0x90,  0x100,   &IID_IOleObject },
            {   0x94,  0x108,   &IID_IOleInPlaceActiveObject },
            {   0x98,  0x110,   &IID_IOleInPlaceObjectWindowless },
            {   0x98,  0x110,       &IID_IOleInPlaceObject },
            {   0x98,  0x110,           &IID_IOleWindow },
        }
    },
    {
        ID_NAME(CLSID_ShellItem),
        {
            {    0x0,    0x0,   &IID_IShellItem },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IPersistIDList },
            {    0x8,   0x10,   &IID_IParentAndItem },
        }
    },
    {
        ID_NAME(CLSID_ShellLink),
        {
            {    0x0,    0x0,   &IID_IShellLinkA },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellLinkW },
            {    0x8,   0x10,   &IID_IPersistStream },
            {    0xc,   0x18,   &IID_IPersistFile },
            {   0x10,   0x20,   &IID_IShellExtInit },
            {   0x14,   0x28,   &IID_IContextMenu3 },
            {   0x14,   0x28,       &IID_IContextMenu2 },
            {   0x14,   0x28,           &IID_IContextMenu },
            {   0x18,   0x30,   &IID_IDropTarget },
            {   0x1c,   0x38,   &IID_IQueryInfo },
            {   0x20,   0x40,   &IID_IShellLinkDataList },
            {   0x24,   0x48,   &IID_IExtractIconA },
            {   0x28,   0x50,   &IID_IExtractIconW },
            {   0x2c,   0x58,   &IID_IExtractImage2 },
            {   0x2c,   0x58,       &IID_IExtractImage },
            {   0x30,   0x60,   &IID_IPersistPropertyBag },
            {   0x34,   0x68,   &IID_IServiceProvider },
            {   0x38,   0x70,   &IID_IFilter },
            {   0x3c,   0x78,   &IID_IObjectWithSite },
            {   0x44,   0x88,   &IID_ICustomizeInfoTip },
            { FARAWY, FARAWY,   &IID_ISLTracker },
        }
    },
#if 0 // Apparently we can only get this through Folder.Items().GetLink
    {
        ID_NAME(CLSID_ShellLinkObject),
        {
            {    0x0,    0x0,       &IID_IUnknown },
        }
    },
#endif
    {
        ID_NAME(CLSID_StartMenu),
        {
            {  -0x48,  -0x80,   &IID_IOleCommandTarget },
            {  -0x44,  -0x78,   &IID_IServiceProvider },
            {  -0x40,  -0x70,   &IID_IDeskBar },
            {  -0x40,  -0x70,       &IID_IOleWindow },
            {  -0x3c,  -0x68,   &IID_IInputObjectSite },
            {  -0x38,  -0x60,   &IID_IInputObject },
            {    0x0,    0x0,   &IID_IMenuPopup },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IObjectWithSite },
            {    0x8,   0x10,   &IID_IBanneredBar },
            {    0xc,   0x18,   &IID_IInitializeObject },
        }
    },
    {
        ID_NAME(CLSID_StartMenuPin),
        {
            {    0x0,    0x0,   &IID_IShellExtInit },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IContextMenu },
            {    0xc,   0x18,   &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_Thumbnail),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {    0x4,    0x8,   &IID_IParentAndItem },
            {    0x8,   0x10,   &IID_IObjectWithSite },
        },
    },
    {
        ID_NAME(CLSID_TrackShellMenu),
        {
            {    0x0,    0x0,   &IID_ITrackShellMenu },
            {    0x0,    0x0,       &IID_IShellMenu },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellMenu2 },
            {    0x8,   0x10,   &IID_IObjectWithSite },
            {    0xc,   0x18,   &IID_IServiceProvider },
        }
    },
    {
        ID_NAME(CLSID_UserNotification),
        {
            {    0x0,    0x0,   &IID_IUserNotification },
            {    0x0,    0x0,       &IID_IUnknown },
        }
    },
};

static const CLASS_AND_INTERFACES ExpectedInterfaces_Vista[] =
{
    {
        ID_NAME(CLSID_ActiveDesktop),
        {
            {    0x0,    0x0,   &IID_IActiveDesktop },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IActiveDesktopP },
            {    0x8,   0x10,   &IID_IADesktopP2 },
            {    0xc,   0x18,   &IID_IPropertyBag },
        }
    },
    {
        ID_NAME(CLSID_CDBurn),
        {
            {    0x0,    0x0,   &IID_IObjectWithSite },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x8,   0x10,   &IID_IShellExtInit },
            {    0xc,   0x18,   &IID_IContextMenu },
            {   0x10,   0x20,   &IID_IShellPropSheetExt },
            {   0x14,   0x28,       &IID_IDropTarget },
            {   0x18,   0x30,   &IID_IPersistFile },
            {   0x1c,   0x38,   &IID_IOleCommandTarget },
            {   0x20,   0x40,   &IID_ICDBurn },
            {   0x28,   0x50,   &IID_IPersistPropertyBag },
            {   0x2c,   0x58,   &IID_IDriveFolderExt},
            {   0x30,   0x60,   &IID_INamespaceWalkCB },
            {   0x38,   0x70,   &IID_IServiceProvider },
            {   0x3c,   0x78,   &IID_IQueryCancelAutoPlay },
        }
    },
    {
        ID_NAME(CLSID_ControlPanel),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {   0x10,   0x20,   &IID_IPersistFolder2 },
            {   0x10,   0x20,       &IID_IPersistFolder },
            {   0x10,   0x20,           &IID_IPersist },
            {   0x14,   0x28,   &IID_IFolderType },
            {   0x18,   0x30,   &IID_IContextMenuCB },
            {   0x1c,   0x38,   &IID_IRegItemCustomAttributes },
            {   0x24,   0x48,   &IID_IControlPanelEnumerator },
            {   0x28,   0x50,   &IID_IRegItemCustomEnumerator},
            {   0x2c,   0x58,   &IID_IAliasRegistrationCallback },
            {   0x94, FARAWY,   &IID_IObjectWithBackReferences },
            {   0xa8, FARAWY,   &IID_ITransferProvider },
            {   0xac, FARAWY,   &IID_IDelegateHostItemContainer },
            {   0xb0, FARAWY,   &IID_IBackReferencedObject },
            {   0x9c, FARAWY,   &IID_IRegItemFolder},
            {   0xa4, FARAWY,   &IID_IShellIconOverlay },
            {   0x98, FARAWY,   &IID_IShellFolder2 },
            {   0x98, FARAWY,       &IID_IShellFolder },
        }
    },
    {
        ID_NAME(CLSID_CopyToMenu),
        {
            {    0x0,    0x0,   &IID_IContextMenu3 },
            {    0x0,    0x0,       &IID_IContextMenu2 },
            {    0x0,    0x0,           &IID_IContextMenu },
            {    0x0,    0x0,               &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellExtInit },
            {    0x8,   0x10,   &IID_IObjectWithSite },
            {   0x10,   0x20,   &IID_IFolderFilter },
        }
    },
    {
        ID_NAME(CLSID_DragDropHelper),
        {
            {    0x0,    0x0,   &IID_IDragSourceHelper },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IDropTargetHelper },
        }
    },
    {
        ID_NAME(CLSID_FadeTask),
        {
            {    0x0,    0x0,   &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_FolderItem),
        {
            {    0x0,    0x0,       &IID_FolderItem2 },
            {    0x0,    0x0,           &IID_FolderItem },
            {    0x0,    0x0,               &IID_IDispatch },
            {    0x0,    0x0,                   &IID_IUnknown },
            {    0x4,    0x8,   &IID_IPersistFolder2 },
            {    0x4,    0x8,       &IID_IPersistFolder },
            {    0x4,    0x8,           &IID_IPersist },
            {    0x8,   0x10,   &IID_IObjectSafety },
            {   0x10,   0x20,   &IID_IParentAndItem },
        }
    },
    {
        ID_NAME(CLSID_FolderItemsFDF),
        {
            {    0x0,    0x0,   &IID_FolderItems3 },
            {    0x0,    0x0,           &IID_FolderItems },
            {    0x0,    0x0,               &IID_IDispatch },
            {    0x0,    0x0,                   &IID_IUnknown },
            {    0x4,    0x8,   &IID_IPersistFolder },
            {    0x4,    0x8,       &IID_IPersist },
            {    0x8,   0x10,   &IID_IObjectSafety },
        }
    },
    {
        ID_NAME(CLSID_FolderShortcut),
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
        ID_NAME(CLSID_FolderViewHost),
        {
            {    0x0,    0x0,   &IID_IShellBrowser },
            {    0x0,    0x0,       &IID_IOleWindow },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IExplorerBrowser },
            {    0x8,   0x10,   &IID_ICommDlgBrowser3 },
            {    0x8,   0x10,       &IID_ICommDlgBrowser2 },
            {    0x8,   0x10,           &IID_ICommDlgBrowser },
            {    0xc,   0x18,   &IID_IServiceProvider },
            {   0x20,   0x40,   &IID_IObjectWithSite },
            {   0x28,   0x50,   &IID_IOleInPlaceUIWindow },
            {   0x2c,   0x58,   &IID_IShellBrowserService_Vista},
            {   0x30,   0x60,   &IID_IConnectionPointContainer },
            {   0x34,   0x68,   &IID_IPersistHistory },
            {   0x34,   0x68,       &IID_IPersist },
            {   0x38,   0x70,   &IID_IInputObject },
            {   0x44,   0x88,   &IID_IFolderFilterSite },
            {   0x48,   0x90,   &IID_IUrlHistoryNotify },
            {   0x48,   0x90,       &IID_IOleCommandTarget },
            {   0x50,   0xa0,   &IID_IFolderViewHost },
            {   0x58,   0xb0,   &IID_INamespaceWalkCB2 },
            {   0x58,   0xb0,       &IID_INamespaceWalkCB },
        }
    },
    {
        ID_NAME(CLSID_ISFBand),
        {
            {  -0xb0, -0x108,   &IID_IObjectWithSite },
            {  -0xa8,  -0xf8,   &IID_IDeskBand },
            {  -0xa8,  -0xf8,       &IID_IDockingWindow },
            {  -0xa8,  -0xf8,           &IID_IOleWindow },
            {  -0xa4,  -0xf0,   &IID_IInputObject },
            {  -0xa0,  -0xe8,   &IID_IPersistStream },
            {  -0xa0,  -0xe8,       &IID_IPersist },
            {  -0x9c,  -0xe0,   &IID_IOleCommandTarget },
            {  -0x98,  -0xd8,   &IID_IServiceProvider },
            {  -0x78,  -0xa0,   &IID_IWinEventHandler },
            {  -0x74,  -0x98,   &IID_IShellChangeNotify },
            {  -0x70,  -0x90,   &IID_IDropTarget },
            {   -0x4,   -0x8,   &IID_IContextMenu },
            {    0x0,    0x0,   &IID_IShellFolderBand },
            {    0x0,    0x0,       &IID_IUnknown },
            {   0x80,   0xd0,   &IID_IFolderBandPriv },
        }
    },
    {
        ID_NAME(CLSID_MenuBand),
        {
            {  -0x34,  -0x60,   &IID_IObjectWithSite },
            {  -0x2c,  -0x50,   &IID_IDeskBand },
            {  -0x2c,  -0x50,       &IID_IDockingWindow },
            {  -0x2c,  -0x50,           &IID_IOleWindow },
            {  -0x28,  -0x48,   &IID_IInputObject },
            {  -0x24,  -0x40,   &IID_IPersistStream },
            {  -0x24,  -0x40,       &IID_IPersist },
            {  -0x20,  -0x38,   &IID_IOleCommandTarget },
            {  -0x1c,  -0x30,   &IID_IServiceProvider },
            {    0x0,    0x0,   &IID_IMenuPopup },
            {    0x0,    0x0,       &IID_IDeskBar },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IMenuBand },
            {    0x8,   0x10,   &IID_IShellMenu2 },
            {    0x8,   0x10,       &IID_IShellMenu },
            {    0xc,   0x18,   &IID_IWinEventHandler },
        }
    },
    {
        ID_NAME(CLSID_MenuBandSite),
        {
            {    0x0,    0x0,   &IID_IBandSite },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IDeskBarClient },
            {    0x4,    0x8,       &IID_IOleWindow },
            {    0x8,   0x10,   &IID_IOleCommandTarget },
            {    0xc,   0x18,   &IID_IInputObject },
            {   0x10,   0x20,   &IID_IInputObjectSite },
            {   0x14,   0x28,   &IID_IWinEventHandler },
            {   0x18,   0x30,   &IID_IServiceProvider },
        }
    },
    {
        ID_NAME(CLSID_MenuDeskBar),
        {
            {  -0x4c,  -0x88,   &IID_IOleCommandTarget },
            {  -0x48,  -0x80,   &IID_IServiceProvider },
            {  -0x44,  -0x78,   &IID_IDeskBar },
            {  -0x44,  -0x78,       &IID_IOleWindow },
            {  -0x40,  -0x70,   &IID_IInputObjectSite },
            {  -0x3c,  -0x68,   &IID_IInputObject },
            {    0x0,    0x0,   &IID_IMenuPopup },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IObjectWithSite },
            {    0x8,   0x10,   &IID_IBanneredBar },
            {    0xc,   0x18,   &IID_IInitializeObject },
        }
    },
    {
        ID_NAME(CLSID_MergedFolder),
        {
            {  -0x28,  -0x50,   &IID_IShellFolder2 },
            {  -0x24,  -0x48,   &IID_IStorage },
            {  -0x20,  -0x40,   &IID_IShellFolder },
            {  -0x18,  -0x30,   &IID_IFolderType },
            {    0x0,    0x0,   &IID_IShellService },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_ITranslateShellChangeNotify },
            {    0x8,   0x10,   &IID_IObjectWithBackReferences },
            {    0xc,   0x18,   &IID_ILocalizableItemParent },
            {  -0x1c,  -0x38,   &IID_IPersistFolder2 },
            {  -0x1c,  -0x38,       &IID_IPersistFolder },
            {  -0x1c,  -0x38,           &IID_IPersist },
        }
    },
    {
        ID_NAME(CLSID_MoveToMenu),
        {
            {    0x0,    0x0,   &IID_IContextMenu3 },
            {    0x0,    0x0,       &IID_IContextMenu2 },
            {    0x0,    0x0,           &IID_IContextMenu },
            {    0x0,    0x0,               &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellExtInit },
            {    0x8,   0x10,   &IID_IObjectWithSite },
            {   0x10,   0x20,   &IID_IFolderFilter },
        }
    },
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
            {    0x0,    0x0,   &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_MyComputer),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {   0x10,   0x20,   &IID_IPersistFolder2 },
            {   0x10,   0x20,       &IID_IPersistFolder },
            {   0x10,   0x20,           &IID_IPersist },
            {   0x18,   0x30,   &IID_IItemNameLimits },
            {   0x20,   0x40,   &IID_IContextMenuCB },
            {   0x24,   0x48,   &IID_IFolderProperties },
            {   0x28,   0x50,   &IID_IFolderWithSearchRoot },
            {   0x1c,   0x38,   &IID_INewItemAdvisor },
            { FARAWY, FARAWY,   &IID_IObjectWithBackReferences },
            { FARAWY, FARAWY,   &IID_IShellFolder2 },
            { FARAWY, FARAWY,       &IID_IShellFolder },
            { FARAWY, FARAWY,   &IID_IRegItemFolder },
            { FARAWY, FARAWY,   &IID_IShellIconOverlay },
            { FARAWY, FARAWY,   &IID_ITransferProvider },
            { FARAWY, FARAWY,   &IID_IDelegateHostItemContainer },
            { FARAWY, FARAWY,   &IID_IBackReferencedObject },
        }
    },
    {
        ID_NAME(CLSID_MyDocuments),
        {
            {   -0x4,   -0x8,   &IID_IPersistFolder },
            {   -0x4,   -0x8,       &IID_IPersist },
            {    0x0,    0x0,   &IID_IShellFolder2 },
            {    0x0,    0x0,       &IID_IShellFolder },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellIconOverlay },
            { FARAWY, FARAWY,   &IID_IObjectWithSite },
            { FARAWY, FARAWY,   &IID_IShellIcon },
            { FARAWY, FARAWY,   &IID_IPersistFolder3 },
            { FARAWY, FARAWY,       &IID_IPersistFolder2 },
            { FARAWY, FARAWY,   &IID_IStorage },
            { FARAWY, FARAWY,   &IID_ILocalizableItemParent },
            { FARAWY, FARAWY,   &IID_IItemNameLimits },
            { FARAWY, FARAWY,   &IID_IContextMenuCB },
            { FARAWY, FARAWY,   &IID_IOleCommandTarget },
            { FARAWY, FARAWY,   &IID_IPersistPropertyBag },
            { FARAWY, FARAWY,   &IID_IObjectWithBackReferences },
            { FARAWY, FARAWY,   &IID_IRemoteComputer },
            { FARAWY, FARAWY,   &IID_IFolderType },
            { FARAWY, FARAWY,   &IID_IAliasRegistrationCallback },
            { FARAWY, FARAWY,   &IID_IBackReferencedObject },
        }
    },
    {
        ID_NAME(CLSID_NetworkPlaces),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {   0x10,   0x20,   &IID_IPersistFolder3 },
            {   0x10,   0x20,       &IID_IPersistFolder2 },
            {   0x10,   0x20,           &IID_IPersistFolder },
            {   0x10,   0x20,               &IID_IPersist },
            {   0x1c,   0x38,   &IID_IContextMenuCB },
            {   0x20,   0x40,   &IID_IFolderWithSearchRoot },
            {   0x24,   0x48,   &IID_INewItemAdvisor },
            { FARAWY, FARAWY,   &IID_IObjectWithBackReferences },
            { FARAWY, FARAWY,   &IID_IShellFolder },
            { FARAWY, FARAWY,       &IID_IShellFolder2 },
            { FARAWY, FARAWY,   &IID_IRegItemFolder },
            { FARAWY, FARAWY,   &IID_IShellIconOverlay },
            { FARAWY, FARAWY,   &IID_ITransferProvider },
            { FARAWY, FARAWY,   &IID_IDelegateHostItemContainer },
            { FARAWY, FARAWY,   &IID_IBackReferencedObject },
        }
    },
    {
        ID_NAME(CLSID_NewMenu),
        {
            {   -0xc,  -0x18,   &IID_IObjectWithSite },
            {   -0x4,   -0x8,   &IID_IContextMenu3 },
            {   -0x4,   -0x8,       &IID_IContextMenu2 },
            {   -0x4,   -0x8,           &IID_IContextMenu },
            {    0x0,    0x0,   &IID_IShellExtInit },
            {    0x0,    0x0,       &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_PersonalStartMenu),
        {
            {  -0x30,  -0x50,   &IID_IObjectWithSite },
            {    0x0,    0x0,   &IID_IShellItemFilter },
            {    0x0,    0x0,       &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_Printers),
        {
            {   -0x8,  -0x10,   &IID_IObjectWithBackReferences },
            {    0x0,    0x0,   &IID_IUnknown },
            {    0xc,   0x18,   &IID_IRemoteComputer },
            {   0x14,   0x28,   &IID_IFolderNotify },
            {   0x1c,   0x38,   &IID_IPersistFolder2 },
            {   0x1c,   0x38,       &IID_IPersistFolder },
            {   0x1c,   0x38,           &IID_IPersist },
            {   0x20,   0x40,   &IID_IContextMenuCB },
            {   0x28,   0x50,   &IID_IResolveShellLink },
            {   0x2c,   0x58,   &IID_IFolderType },
            {   0xec,  0x168,   &IID_IShellFolder2 },
            {   0xec,  0x168,       &IID_IShellFolder },
            {   0xf0,  0x170,   &IID_IRegItemFolder },
            {   0xf8,  0x180,   &IID_IShellIconOverlay },
            {   0xfc,  0x188,   &IID_ITransferProvider },
            {  0x100,  0x190,   &IID_IDelegateHostItemContainer },
            {  0x104,  0x198,   &IID_IBackReferencedObject },
        }
    },
    {
        ID_NAME(CLSID_QueryAssociations),
        {
            {    0x0,    0x0,   &IID_IAssociationArray },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IQueryAssociations },
            {    0x8,   0x10,   &IID_IObjectWithAssociationList },
        }
    },
    {
        ID_NAME(CLSID_QuickLinks),
        {
            {  -0xb0, -0x108,   &IID_IObjectWithSite },
            {  -0xa8,  -0xf8,   &IID_IDeskBand },
            {  -0xa8,  -0xf8,       &IID_IDockingWindow },
            {  -0xa8,  -0xf8,           &IID_IOleWindow },
            {  -0xa4,  -0xf0,   &IID_IInputObject },
            {  -0xa0,  -0xe8,   &IID_IPersistStream },
            {  -0xa0,  -0xe8,       &IID_IPersist },
            {  -0x9c,  -0xe0,   &IID_IOleCommandTarget },
            {  -0x98,  -0xd8,   &IID_IServiceProvider },
            {  -0x78,  -0xa0,   &IID_IWinEventHandler },
            {  -0x74,  -0x98,   &IID_IShellChangeNotify },
            {  -0x70,  -0x90,   &IID_IDropTarget },
            {   -0x4,   -0x8,   &IID_IContextMenu },
            {    0x0,    0x0,   &IID_IShellFolderBand },
            {    0x0,    0x0,       &IID_IUnknown },
            {   0x80,   0xd0,   &IID_IFolderBandPriv },
        }
    },
    {
        ID_NAME(CLSID_RecycleBin),
        {
            {    0x0,    0x0,   &IID_IPersistFolder2 },
            {    0x0,    0x0,       &IID_IPersistFolder },
            {    0x0,    0x0,           &IID_IPersist },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellFolder2 },
            {    0x4,    0x8,       &IID_IShellFolder },
            {    0x8,   0x10,   &IID_IContextMenu },
            {    0xc,   0x18,   &IID_IShellPropSheetExt },
            {   0x10,   0x20,   &IID_IShellExtInit },
            {   0x14,   0x28,   &IID_IContextMenuCB },
            {   0x18,   0x30,   &IID_IFolderType },
            {   0x1c,   0x38,   &IID_IObjectWithBackReferences },
        }
    },
    {
        ID_NAME(CLSID_SendToMenu),
        {
            {   -0xc,  -0x18,   &IID_IObjectWithSite },
            {   -0x4,   -0x8,   &IID_IContextMenu3 },
            {   -0x4,   -0x8,       &IID_IContextMenu2 },
            {   -0x4,   -0x8,           &IID_IContextMenu },
            {    0x0,    0x0,   &IID_IShellExtInit },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IOleWindow },
            {    0x8,   0x10,   &IID_IServiceProvider },
        }
    },
    {
        ID_NAME(CLSID_Shell),
        {
            {    0x0,    0x0,   &IID_IShellDispatch5 },
            {    0x0,    0x0,       &IID_IShellDispatch4 },
            {    0x0,    0x0,           &IID_IShellDispatch3 },
            {    0x0,    0x0,               &IID_IShellDispatch2 },
            {    0x0,    0x0,                   &IID_IShellDispatch },
            {    0x0,    0x0,                       &IID_IDispatch },
            {    0x0,    0x0,                           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IObjectSafety },
            {   0x20,   0x40,   &IID_IObjectWithSite },
        }
    },
#if 0 // This is correct for Vista, but cannot be un-initalized
    {
        ID_NAME_CANNOT_INSTANTIATE(CLSID_ShellDesktop),
        {
            {   -0xc,  -0x18,   &CLSID_ShellDesktop },
            {   -0xc,  -0x18,       &IID_IObjectWithSite },
            {    0x0,    0x0,   &IID_IUnknown },
            {   0x10,   0x20,   &IID_IStorage },
            {   0x14,   0x28,   &IID_IPersistFolder2 },
            {   0x14,   0x28,       &IID_IPersistFolder },
            {   0x14,   0x28,           &IID_IPersist },
            {   0x18,   0x30,   &IID_IFolderWithSearchRoot },
            {   0x1c,   0x38,   &IID_IShellIcon },
            {   0x24,   0x48,   &IID_IContextMenuCB },
            {   0x28,   0x50,   &IID_ITranslateShellChangeNotify },
            {   0x2c,   0x58,   &IID_IItemNameLimits },
            {   0x30,   0x60,   &IID_IOleCommandTarget },
            {   0x34,   0x68,   &IID_IObjectWithBackReferences },
            {   0x38,   0x70,   &IID_ILocalizableItemParent },
            { FARAWY, FARAWY,   &IID_IShellFolder },
            { FARAWY, FARAWY,   &IID_IShellFolder2 },
            { FARAWY, FARAWY,   &IID_IRegItemFolder },
            { FARAWY, FARAWY,   &IID_IShellIconOverlay },
            { FARAWY, FARAWY,   &IID_ITransferProvider },
            { FARAWY, FARAWY,   &IID_IDelegateHostItemContainer },
            { FARAWY, FARAWY,   &IID_IBackReferencedObject },
        }
    },
#endif
    {
        ID_NAME(CLSID_ShellFSFolder),
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
        ID_NAME(CLSID_ShellFldSetExt),
        {
            {    0x0,    0x0,   &IID_IShellPropSheetExt },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellExtInit },
            {    0x8,   0x10,   &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_ShellFolderView),
        {
            {    0x0,    0x0,   &IID_IShellFolderViewDual3 },
            {    0x0,    0x0,       &IID_IShellFolderViewDual2 },
            {    0x0,    0x0,           &IID_IShellFolderViewDual },
            {    0x0,    0x0,               &IID_IDispatch },
            {    0x0,    0x0,                   &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellService },
            {    0x8,   0x10,   &IID_IServiceProvider },
            {    0xc,   0x18,   &IID_IObjectSafety },
            {   0x14,   0x28,   &IID_IObjectWithSite },
            {   0x1c,   0x38,   &IID_IConnectionPointContainer },
        }
    },
    {
        ID_NAME(CLSID_ShellFolderViewOC),
        {
            {    0x0,    0x0,   &IID_IFolderViewOC },
            {    0x0,    0x0,       &IID_IDispatch },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IProvideClassInfo2 },
            {    0x4,    0x8,       &IID_IProvideClassInfo },
            {    0x8,   0x10,   &IID_IObjectSafety },
            {   0x10,   0x20,   &IID_IConnectionPointContainer },
            {   0x88,   0xf0,   &IID_IPersistStreamInit },
            {   0x88,   0xf0,       &IID_IPersist },
            {   0x90,  0x100,   &IID_IOleControl },
            {   0x94,  0x108,   &IID_IOleObject },
            {   0x98,  0x110,   &IID_IOleInPlaceActiveObject },
            {   0x9c,  0x118,   &IID_IOleInPlaceObjectWindowless },
            {   0x9c,  0x118,       &IID_IOleInPlaceObject },
            {   0x9c,  0x118,           &IID_IOleWindow },
        }
    },
    {
        ID_NAME(CLSID_ShellItem),
        {
            {    0x0,    0x0,   &IID_IShellItem2 },
            {    0x0,    0x0,       &IID_IShellItem },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x8,   0x10,   &IID_IPersistIDList },
            {    0xc,   0x18,   &IID_IParentAndItem },
            {   0x10,   0x20,   &IID_IMarshal },
            {   0x14,   0x28,   &IID_IPersistStream },
            {   0x14,   0x28,       &IID_IPersist },
        }
    },
    {
        ID_NAME(CLSID_ShellLink),
        {
            {    0x0,    0x0,   &IID_IShellLinkA },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellLinkW },
            {    0x8,   0x10,   &IID_IPersistStream },
            {   0x10,   0x20,   &IID_IPersistFile },
            {   0x10,   0x20,       &IID_IPersist },
            {   0x14,   0x28,   &IID_IShellExtInit },
            {   0x18,   0x30,   &IID_IContextMenu3 },
            {   0x18,   0x30,       &IID_IContextMenu2 },
            {   0x18,   0x30,           &IID_IContextMenu },
            {   0x1c,   0x38,   &IID_IDropTarget },
            {   0x20,   0x40,   &IID_IQueryInfo },
            {   0x24,   0x48,   &IID_IShellLinkDataList },
            {   0x28,   0x50,   &IID_IExtractIconA },
            {   0x2c,   0x58,   &IID_IExtractIconW },
            {   0x30,   0x60,   &IID_IExtractImage2 },
            {   0x30,   0x60,       &IID_IExtractImage },
            {   0x34,   0x68,   &IID_IPersistPropertyBag },
            {   0x38,   0x70,   &IID_IServiceProvider },
            {   0x3c,   0x78,   &IID_IFilter },
            {   0x40,   0x80,   &IID_IObjectWithSite },
            {   0x48,   0x90,   &IID_IPropertyStore },
            {   0x4c,   0x98,   &IID_ICustomizeInfoTip },
            {   0x50,   0xa0,   &IID_IPropertyBag },
            { FARAWY, FARAWY,   &IID_ISLTracker },
        }
    },
#if 0 // Crashes on Vista when un-initializing 
    {
        ID_NAME_CANNOT_INSTANTIATE(CLSID_ShellLinkObject),
        {
            {    0x0,    0x0,       &IID_IUnknown },
        }
    },
    {
        ID_NAME_CANNOT_INSTANTIATE(CLSID_StartMenu),
        {
            {  -0x48,  -0x90,   &IID_IOleCommandTarget },
            {  -0x44,  -0x88,   &IID_IServiceProvider },
            {  -0x40,  -0x80,   &IID_IDeskBar },
            {  -0x40,  -0x80,       &IID_IOleWindow },
            {  -0x3c,  -0x78,   &IID_IInputObjectSite },
            {  -0x38,  -0x70,   &IID_IInputObject },
            {    0x0,    0x0,   &IID_IMenuPopup },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IObjectWithSite },
            {    0x8,   0x10,   &IID_IBanneredBar },
            {    0xc,   0x18,   &IID_IInitializeObject },
        }
    },
#endif
    {
        ID_NAME(CLSID_StartMenuPin),
        {
            {  -0x24,  -0x48,   &IID_IShellExtInit },
            {  -0x20,  -0x40,   &IID_IContextMenu },
            {  -0x1c,  -0x38,   &IID_IPinnedListOld },
            {  -0x18,  -0x30,   &IID_IObjectWithSite },
            {    0x0,    0x0,   &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_Thumbnail),
        {
            {   -0x8,  -0x10,   &IID_IObjectWithSite },
            {    0x0,    0x0,   &IID_IUnknown },
        },
    },
    {
        ID_NAME(CLSID_TrackShellMenu),
        {
            {    0x0,    0x0,   &IID_ITrackShellMenu },
            {    0x0,    0x0,       &IID_IShellMenu },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellMenu2 },
            {    0x8,   0x10,   &IID_IObjectWithSite },
            {    0xc,   0x18,   &IID_IServiceProvider },
        }
    },
    {
        ID_NAME(CLSID_UserNotification),
        {
            {    0x0,    0x0,   &IID_IUserNotification },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IUserNotification2 },
        }
    },
};

static const CLASS_AND_INTERFACES ExpectedInterfaces_Win7[] =
{
    {
        ID_NAME(CLSID_ActiveDesktop),
        {
            {    0x0,    0x0,   &IID_IActiveDesktop },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IActiveDesktopP },
            {    0x8,   0x10,   &IID_IADesktopP2 },
            {    0xc,   0x18,   &IID_IPropertyBag },
        }
    },
    {
        ID_NAME(CLSID_CDBurn),
        {
            {    0x0,    0x0,   &IID_IObjectWithSite },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x8,   0x10,   &IID_IShellExtInit },
            {    0xc,   0x18,   &IID_IContextMenu },
            {   0x10,   0x20,   &IID_IShellPropSheetExt },
            {   0x14,   0x28,       &IID_IDropTarget },
            {   0x18,   0x30,   &IID_IPersistFile },
            {   0x1c,   0x38,   &IID_ICDBurn },
            {   0x24,   0x48,   &IID_IPersistPropertyBag },
            {   0x28,   0x50,   &IID_IDriveFolderExt},
            {   0x2c,   0x58,   &IID_INamespaceWalkCB },
            {   0x34,   0x68,   &IID_IServiceProvider },
            {   0x38,   0x70,   &IID_IQueryCancelAutoPlay },
        }
    },
    {
        ID_NAME(CLSID_ControlPanel),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {   0x10,   0x20,   &IID_IPersistFolder2 },
            {   0x10,   0x20,       &IID_IPersistFolder },
            {   0x10,   0x20,           &IID_IPersist },
            {   0x14,   0x28,   &IID_IFolderType },
            {   0x18,   0x30,   &IID_IContextMenuCB },
            {   0x1c,   0x38,   &IID_IRegItemCustomAttributes },
            { FARAWY, FARAWY,   &IID_IObjectWithBackReferences },
            { FARAWY, FARAWY,   &IID_IShellFolder2 },
            { FARAWY, FARAWY,       &IID_IShellFolder },
            { FARAWY, FARAWY,   &IID_IRegItemFolder},
            { FARAWY, FARAWY,   &IID_IShellIcon },
            { FARAWY, FARAWY,   &IID_IShellIconOverlay },
            { FARAWY, FARAWY,   &IID_ITransferProvider },
            { FARAWY, FARAWY,   &IID_IDelegateHostItemContainer },
            { FARAWY, FARAWY,   &IID_IBackReferencedObject },
            { FARAWY, FARAWY,   &IID_IStorage }
        }
    },
    {
        ID_NAME(CLSID_CopyToMenu),
        {
            {    0x0,    0x0,   &IID_IContextMenu },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellExtInit },
            {    0x8,   0x10,   &IID_IObjectWithSite },
            {   0x10,   0x20,   &IID_IFolderFilter },
        }
    },
    {
        ID_NAME(CLSID_DragDropHelper),
        {
            {    0x0,    0x0,   &IID_IDragSourceHelper },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IDropTargetHelper },
        }
    },
    {
        ID_NAME(CLSID_FadeTask),
        {
            {    0x0,    0x0,   &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_FolderItem),
        {
            {    0x0,    0x0,       &IID_FolderItem2 },
            {    0x0,    0x0,           &IID_FolderItem },
            {    0x0,    0x0,               &IID_IDispatch },
            {    0x0,    0x0,                   &IID_IUnknown },
            {    0x4,    0x8,   &IID_IPersistFolder2 },
            {    0x4,    0x8,       &IID_IPersistFolder },
            {    0x4,    0x8,           &IID_IPersist },
            {    0x8,   0x10,   &IID_IObjectSafety },
            {   0x10,   0x20,   &IID_IParentAndItem },
        }
    },
    {
        ID_NAME(CLSID_FolderItemsFDF),
        {
            {    0x0,    0x0,   &IID_FolderItems3 },
            {    0x0,    0x0,           &IID_FolderItems },
            {    0x0,    0x0,               &IID_IDispatch },
            {    0x0,    0x0,                   &IID_IUnknown },
            {    0x4,    0x8,   &IID_IPersistFolder },
            {    0x4,    0x8,       &IID_IPersist },
            {    0x8,   0x10,   &IID_IObjectSafety },
        }
    },
    {
        ID_NAME(CLSID_FolderShortcut),
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
            {   0x34,   0x68,   &IID_IShellIconOverlay },
        }
    },
    {
        ID_NAME(CLSID_FolderViewHost),
        {
            {    0x0,    0x0,   &IID_IShellBrowser },
            {    0x0,    0x0,       &IID_IOleWindow },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x8,   0x10,   &IID_IExplorerBrowser },
            {    0xc,   0x18,   &IID_ICommDlgBrowser3 },
            {    0xc,   0x18,       &IID_ICommDlgBrowser2 },
            {    0xc,   0x18,           &IID_ICommDlgBrowser },
            {   0x10,   0x20,   &IID_IServiceProvider },
            {   0x24,   0x48,   &IID_IObjectWithSite },
            {   0x2c,   0x58,   &IID_IOleInPlaceUIWindow },
            {   0x30,   0x60,   &IID_IShellBrowserService },
            {   0x34,   0x68,   &IID_IConnectionPointContainer },
            {   0x38,   0x70,   &IID_IPersistHistory },
            {   0x38,   0x70,       &IID_IPersist },
            {   0x3c,   0x78,   &IID_IInputObject },
            {   0x48,   0x90,   &IID_IFolderFilterSite },
            {   0x4c,   0x98,   &IID_IUrlHistoryNotify },
            {   0x4c,   0x98,       &IID_IOleCommandTarget },
            {   0x54,   0xa8,   &IID_IFolderViewHost },
            {   0x5c,   0xb8,   &IID_INamespaceWalkCB2 },
            {   0x5c,   0xb8,       &IID_INamespaceWalkCB },
        }
    },
    {
        ID_NAME(CLSID_ISFBand),
        {
            {  -0xb0, -0x108,   &IID_IObjectWithSite },
            {  -0xa8,  -0xf8,   &IID_IDeskBand },
            {  -0xa8,  -0xf8,       &IID_IDockingWindow },
            {  -0xa8,  -0xf8,           &IID_IOleWindow },
            {  -0xa4,  -0xf0,   &IID_IInputObject },
            {  -0xa0,  -0xe8,   &IID_IPersistStream },
            {  -0xa0,  -0xe8,       &IID_IPersist },
            {  -0x9c,  -0xe0,   &IID_IOleCommandTarget },
            {  -0x98,  -0xd8,   &IID_IServiceProvider },
            {  -0x78,  -0xa0,   &IID_IWinEventHandler },
            {  -0x74,  -0x98,   &IID_IShellChangeNotify },
            {  -0x70,  -0x90,   &IID_IDropTarget },
            {   -0x4,   -0x8,   &IID_IContextMenu },
            {    0x0,    0x0,   &IID_IShellFolderBand },
            {    0x0,    0x0,       &IID_IUnknown },
            {   0x88,   0xd8,   &IID_IFolderBandPriv },
        }
    },
    {
        ID_NAME(CLSID_MenuBand),
        {
            {  -0x34,  -0x60,   &IID_IObjectWithSite },
            {  -0x2c,  -0x50,   &IID_IDeskBand },
            {  -0x2c,  -0x50,       &IID_IDockingWindow },
            {  -0x2c,  -0x50,           &IID_IOleWindow },
            {  -0x28,  -0x48,   &IID_IInputObject },
            {  -0x24,  -0x40,   &IID_IPersistStream },
            {  -0x24,  -0x40,       &IID_IPersist },
            {  -0x20,  -0x38,   &IID_IOleCommandTarget },
            {  -0x1c,  -0x30,   &IID_IServiceProvider },
            {    0x0,    0x0,   &IID_IMenuPopup },
            {    0x0,    0x0,       &IID_IDeskBar },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IMenuBand },
            {    0x8,   0x10,   &IID_IShellMenu2 },
            {    0x8,   0x10,       &IID_IShellMenu },
            {    0xc,   0x18,   &IID_IWinEventHandler },
        }
    },
    {
        ID_NAME(CLSID_MenuBandSite),
        {
            {    0x0,    0x0,   &IID_IBandSite },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IDeskBarClient },
            {    0x4,    0x8,       &IID_IOleWindow },
            {    0x8,   0x10,   &IID_IOleCommandTarget },
            {    0xc,   0x18,   &IID_IInputObject },
            {   0x10,   0x20,   &IID_IInputObjectSite },
            {   0x14,   0x28,   &IID_IWinEventHandler },
            {   0x18,   0x30,   &IID_IServiceProvider },
        }
    },
    {
        ID_NAME(CLSID_MenuDeskBar),
        {
            {  -0x4c,  -0x88,   &IID_IOleCommandTarget },
            {  -0x48,  -0x80,   &IID_IServiceProvider },
            {  -0x44,  -0x78,   &IID_IDeskBar },
            {  -0x44,  -0x78,       &IID_IOleWindow },
            {  -0x40,  -0x70,   &IID_IInputObjectSite },
            {  -0x3c,  -0x68,   &IID_IInputObject },
            {    0x0,    0x0,   &IID_IMenuPopup },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IObjectWithSite },
            {    0x8,   0x10,   &IID_IBanneredBar },
            {    0xc,   0x18,   &IID_IInitializeObject },
        }
    },
    {
        ID_NAME(CLSID_MergedFolder),
        {
            {  -0x14,  -0x28,   &IID_IShellFolder2 },
            {  -0x10,  -0x20,   &IID_IStorage },
            {   -0xc,  -0x18,   &IID_IShellFolder },
            {   -0x8,  -0x10,   &IID_IPersistFolder2 },
            {   -0x8,  -0x10,       &IID_IPersistFolder },
            {   -0x8,  -0x10,           &IID_IPersist },
            {   -0x4,   -0x8,   &IID_IFolderType },
            {    0x0,    0x0,   &IID_IShellService },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x8,   0x10,   &IID_IObjectWithBackReferences },
            {    0xc,   0x18,   &IID_ILocalizableItemParent },
        }
    },
    {
        ID_NAME(CLSID_MoveToMenu),
        {
            {    0x0,    0x0,   &IID_IContextMenu },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellExtInit },
            {    0x8,   0x10,   &IID_IObjectWithSite },
            {   0x10,   0x20,   &IID_IFolderFilter },
        }
    },
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
            {    0x0,    0x0,   &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_MyComputer),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {   0x10,   0x20,   &IID_IPersistFolder2 },
            {   0x10,   0x20,       &IID_IPersistFolder },
            {   0x10,   0x20,           &IID_IPersist },
            {   0x18,   0x30,   &IID_IItemNameLimits },
            {   0x20,   0x40,   &IID_IContextMenuCB },
            {   0x24,   0x48,   &IID_IFolderProperties },
            {   0x1c,   0x38,   &IID_INewItemAdvisor },
            {   0x74,   0xd8,   &IID_IShellIcon },
            {   0x94,  0x118,   &IID_IStorage },
            { FARAWY, FARAWY,   &IID_IObjectWithBackReferences },
            { FARAWY, FARAWY,   &IID_IShellFolder2 },
            { FARAWY, FARAWY,       &IID_IShellFolder },
            { FARAWY, FARAWY,   &IID_IRegItemFolder },
            { FARAWY, FARAWY,   &IID_IShellIconOverlay },
            { FARAWY, FARAWY,   &IID_ITransferProvider },
            { FARAWY, FARAWY,   &IID_IDelegateHostItemContainer },
            { FARAWY, FARAWY,   &IID_IBackReferencedObject },
        }
    },
    {
        ID_NAME(CLSID_MyDocuments),
        {
            {   -0x4,   -0x8,   &IID_IPersistFolder },
            {   -0x4,   -0x8,       &IID_IPersist },
            {    0x0,    0x0,   &IID_IShellFolder2 },
            {    0x0,    0x0,       &IID_IShellFolder },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellIconOverlay },
            { FARAWY, FARAWY,   &IID_IObjectWithSite },
            { FARAWY, FARAWY,   &IID_IShellIcon },
            { FARAWY, FARAWY,   &IID_IPersistFolder3 },
            { FARAWY, FARAWY,       &IID_IPersistFolder2 },
            { FARAWY, FARAWY,   &IID_IStorage },
            { FARAWY, FARAWY,   &IID_ILocalizableItemParent },
            { FARAWY, FARAWY,   &IID_IItemNameLimits },
            { FARAWY, FARAWY,   &IID_IContextMenuCB },
            { FARAWY, FARAWY,   &IID_IOleCommandTarget },
            { FARAWY, FARAWY,   &IID_IPersistPropertyBag },
            { FARAWY, FARAWY,   &IID_IObjectWithBackReferences },
            { FARAWY, FARAWY,   &IID_IRemoteComputer },
            { FARAWY, FARAWY,   &IID_IFolderType },
            { FARAWY, FARAWY,   &IID_IBackReferencedObject },
            { FARAWY, FARAWY,   &IID_IParentAndItem },
        }
    },
    {
        ID_NAME(CLSID_NetworkPlaces),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {   0x10,   0x20,   &IID_IPersistFolder3 },
            {   0x10,   0x20,       &IID_IPersistFolder2 },
            {   0x10,   0x20,           &IID_IPersistFolder },
            {   0x10,   0x20,               &IID_IPersist },
            {   0x20,   0x40,   &IID_IContextMenuCB },
            {   0x28,   0x50,   &IID_INewItemAdvisor },
            {   0x94, FARAWY,   &IID_IShellIcon },
            {   0xb4, FARAWY,   &IID_IStorage },
            { FARAWY, FARAWY,   &IID_IObjectWithBackReferences },
            { FARAWY, FARAWY,   &IID_IShellFolder },
            { FARAWY, FARAWY,       &IID_IShellFolder2 },
            { FARAWY, FARAWY,   &IID_IRegItemFolder },
            { FARAWY, FARAWY,   &IID_IShellIconOverlay },
            { FARAWY, FARAWY,   &IID_ITransferProvider },
            { FARAWY, FARAWY,   &IID_IDelegateHostItemContainer },
            { FARAWY, FARAWY,   &IID_IBackReferencedObject },
        }
    },
    {
        ID_NAME(CLSID_NewMenu),
        {
            {   -0xc,  -0x18,   &IID_IObjectWithSite },
            {   -0x4,   -0x8,   &IID_IContextMenu3 },
            {   -0x4,   -0x8,       &IID_IContextMenu2 },
            {   -0x4,   -0x8,           &IID_IContextMenu },
            {    0x0,    0x0,   &IID_IShellExtInit },
            {    0x0,    0x0,       &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_PersonalStartMenu),
        {
            {  -0x34,  -0x58,   &IID_IObjectWithSite },
            {    0x0,    0x0,   &IID_IShellItemFilter },
            {    0x0,    0x0,       &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_QueryAssociations),
        {
            {    0x0,    0x0,   &IID_IAssociationArray },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IQueryAssociations },
            {    0x8,   0x10,   &IID_IObjectWithAssociationList },
        }
    },
    {
        ID_NAME(CLSID_QuickLinks),
        {
            {  -0xb0, -0x108,   &IID_IObjectWithSite },
            {  -0xa8,  -0xf8,   &IID_IDeskBand },
            {  -0xa8,  -0xf8,       &IID_IDockingWindow },
            {  -0xa8,  -0xf8,           &IID_IOleWindow },
            {  -0xa4,  -0xf0,   &IID_IInputObject },
            {  -0xa0,  -0xe8,   &IID_IPersistStream },
            {  -0xa0,  -0xe8,       &IID_IPersist },
            {  -0x9c,  -0xe0,   &IID_IOleCommandTarget },
            {  -0x98,  -0xd8,   &IID_IServiceProvider },
            {  -0x78,  -0xa0,   &IID_IWinEventHandler },
            {  -0x74,  -0x98,   &IID_IShellChangeNotify },
            {  -0x70,  -0x90,   &IID_IDropTarget },
            {   -0x4,   -0x8,   &IID_IContextMenu },
            {    0x0,    0x0,   &IID_IShellFolderBand },
            {    0x0,    0x0,       &IID_IUnknown },
            {   0x88,   0xd8,   &IID_IFolderBandPriv },
        }
    },
    {
        ID_NAME(CLSID_RecycleBin),
        {
            {    0x0,    0x0,   &IID_IPersistFolder2 },
            {    0x0,    0x0,       &IID_IPersistFolder },
            {    0x0,    0x0,           &IID_IPersist },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellFolder2 },
            {    0x4,    0x8,       &IID_IShellFolder },
            {    0x8,   0x10,   &IID_IShellIconOverlay },
            {    0xc,   0x18,   &IID_IShellPropSheetExt },
            {   0x10,   0x20,   &IID_IShellExtInit },
            {   0x14,   0x28,   &IID_IContextMenuCB },
            {   0x18,   0x30,   &IID_IFolderType },
            {   0x1c,   0x38,   &IID_IObjectWithBackReferences },
        }
    },
    {
        ID_NAME(CLSID_SendToMenu),
        {
            {   -0xc,  -0x18,   &IID_IObjectWithSite },
            {   -0x4,   -0x8,   &IID_IContextMenu3 },
            {   -0x4,   -0x8,       &IID_IContextMenu2 },
            {   -0x4,   -0x8,           &IID_IContextMenu },
            {    0x0,    0x0,   &IID_IShellExtInit },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IOleWindow },
            {    0x8,   0x10,   &IID_INamespaceWalkCB2 },
            {    0x8,   0x10,       &IID_INamespaceWalkCB },
            {    0xc,   0x18,   &IID_IServiceProvider },
            { FARAWY, FARAWY,   &IID_IMarshal },
        }
    },
    {
        ID_NAME(CLSID_Shell),
        {
            {    0x0,    0x0,   &IID_IShellDispatch5 },
            {    0x0,    0x0,       &IID_IShellDispatch4 },
            {    0x0,    0x0,           &IID_IShellDispatch3 },
            {    0x0,    0x0,               &IID_IShellDispatch2 },
            {    0x0,    0x0,                   &IID_IShellDispatch },
            {    0x0,    0x0,                       &IID_IDispatch },
            {    0x0,    0x0,                           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IObjectSafety },
            {   0x20,   0x40,   &IID_IObjectWithSite },
        }
    },
#if 0 // This is correct for Win7, but crashes on un-initialization
    {
        ID_NAME(CLSID_ShellDesktop),
        {
            {   -0xc,  -0x18,   &CLSID_ShellDesktop },
            {   -0xc,  -0x18,       &IID_IObjectWithSite },
            {    0x0,    0x0,   &IID_IUnknown },
            {   0x10,   0x20,   &IID_IStorage },
            {   0x14,   0x28,   &IID_IPersistFolder2 },
            {   0x14,   0x28,       &IID_IPersistFolder },
            {   0x14,   0x28,           &IID_IPersist },
            {   0x24,   0x48,   &IID_IContextMenuCB },
            {   0x28,   0x50,   &IID_IItemNameLimits },
            {   0x2c,   0x58,   &IID_IOleCommandTarget },
            {   0x30,   0x60,   &IID_IObjectWithBackReferences },
            {   0x34,   0x68,   &IID_ILocalizableItemParent },
            { FARAWY, FARAWY,   &IID_IShellIcon },
            { FARAWY, FARAWY,   &IID_IShellFolder },
            { FARAWY, FARAWY,   &IID_IShellFolder2 },
            { FARAWY, FARAWY,   &IID_IRegItemFolder },
            { FARAWY, FARAWY,   &IID_IShellIconOverlay },
            { FARAWY, FARAWY,   &IID_ITransferProvider },
            { FARAWY, FARAWY,   &IID_IDelegateHostItemContainer },
            { FARAWY, FARAWY,   &IID_IBackReferencedObject },
        }
    },
#endif
{
        ID_NAME(CLSID_ShellFSFolder),
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
        }
    },
    {
        ID_NAME(CLSID_ShellFldSetExt),
        {
            {    0x0,    0x0,   &IID_IShellPropSheetExt },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellExtInit },
            {    0x8,   0x10,   &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_ShellFolderView),
        {
            {    0x0,    0x0,   &IID_IShellFolderViewDual3 },
            {    0x0,    0x0,       &IID_IShellFolderViewDual2 },
            {    0x0,    0x0,           &IID_IShellFolderViewDual },
            {    0x0,    0x0,               &IID_IDispatch },
            {    0x0,    0x0,                   &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellService },
            {    0x8,   0x10,   &IID_IServiceProvider },
            {    0xc,   0x18,   &IID_IObjectSafety },
            {   0x14,   0x28,   &IID_IObjectWithSite },
            {   0x1c,   0x38,   &IID_IConnectionPointContainer },
        }
    },
    {
        ID_NAME(CLSID_ShellFolderViewOC),
        {
            {    0x0,    0x0,   &IID_IFolderViewOC },
            {    0x0,    0x0,       &IID_IDispatch },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IProvideClassInfo2 },
            {    0x4,    0x8,       &IID_IProvideClassInfo },
            {    0x8,   0x10,   &IID_IObjectSafety },
            {   0x10,   0x20,   &IID_IConnectionPointContainer },
            {   0x88,   0xf0,   &IID_IPersistStreamInit },
            {   0x88,   0xf0,       &IID_IPersist },
            {   0x90,  0x100,   &IID_IOleControl },
            {   0x94,  0x108,   &IID_IOleObject },
            {   0x98,  0x110,   &IID_IOleInPlaceActiveObject },
            {   0x9c,  0x118,   &IID_IOleInPlaceObjectWindowless },
            {   0x9c,  0x118,       &IID_IOleInPlaceObject },
            {   0x9c,  0x118,           &IID_IOleWindow },
        }
    },
    {
        ID_NAME(CLSID_ShellItem),
        {
            {   -0xc,  -0x18,   &IID_IMarshal },
            {   -0x8,  -0x10,   &IID_IShellItem2 },
            {   -0x8,  -0x10,       &IID_IShellItem },
            {    0x0,    0x0,   &IID_IPersistIDList },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IParentAndItem },
            {    0x8,   0x10,   &IID_IPersistStream },
            {    0x8,   0x10,       &IID_IPersist },
            {   0x14,   0x28,   &IID_IObjectWithBackReferences },
        },
        L"Both",
    },
    {
        ID_NAME(CLSID_ShellLink),
        {
            {  -0x30,  -0x60,   &IID_IMarshal },
            {  -0x2c,  -0x58,   &IID_IShellLinkA },
            {  -0x28,  -0x50,   &IID_IShellLinkW },
            {  -0x24,  -0x48,   &IID_IPersistStream },
            {  -0x1c,  -0x38,   &IID_IPersistFile },
            {  -0x1c,  -0x38,       &IID_IPersist },
            {  -0x18,  -0x30,   &IID_IShellExtInit },
            {  -0x14,  -0x28,   &IID_IContextMenu3 },
            {  -0x14,  -0x28,       &IID_IContextMenu2 },
            {  -0x14,  -0x28,           &IID_IContextMenu },
            {  -0x10,  -0x20,   &IID_IDropTarget },
            {   -0xc,  -0x18,   &IID_IQueryInfo },
            {   -0x8,  -0x10,   &IID_IShellLinkDataList },
            {   -0x4,   -0x8,   &IID_IExtractIconA },
            {    0x0,    0x0,   &IID_IExtractIconW },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IExtractImage2 },
            {    0x4,    0x8,       &IID_IExtractImage },
            {    0x8,   0x10,   &IID_IPersistPropertyBag },
            {    0xc,   0x18,   &IID_IServiceProvider },
            {   0x10,   0x20,   &IID_IObjectWithSite },
            {   0x18,   0x30,   &IID_IPropertyStore },
            {   0x1c,   0x38,   &IID_ICustomizeInfoTip },
            { FARAWY, FARAWY,   &IID_ISLTracker },
        },
        L"Both",
    },
    {
        ID_NAME(CLSID_StartMenuPin),
        {
            {  -0x28,  -0x50,   &IID_IShellExtInit },
            {  -0x24,  -0x48,   &IID_IContextMenu },
            {  -0x20,  -0x40,   &IID_IPinnedList },
            {  -0x1c,  -0x38,   &IID_IObjectWithSite },
            {    0x0,    0x0,   &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_Thumbnail),
        {
            {   -0x8,  -0x10,   &IID_IObjectWithSite },
            {    0x0,    0x0,   &IID_IUnknown },
        },
    },
    {
        ID_NAME(CLSID_TrackShellMenu),
        {
            {    0x0,    0x0,   &IID_ITrackShellMenu },
            {    0x0,    0x0,       &IID_IShellMenu },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellMenu2 },
            {    0x8,   0x10,   &IID_IObjectWithSite },
            {    0xc,   0x18,   &IID_IServiceProvider },
        }
    },
    {
        ID_NAME(CLSID_UserNotification),
        {
            {    0x0,    0x0,   &IID_IUserNotification },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IUserNotification2 },
        }
    },
};

static const CLASS_AND_INTERFACES ExpectedInterfaces_Win8[] =
{
    {
        ID_NAME(CLSID_ActiveDesktop),
        {
            {    0x0,    0x0,   &IID_IActiveDesktop },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IActiveDesktopP },
            {    0x8,   0x10,   &IID_IADesktopP2 },
            {    0xc,   0x18,   &IID_IPropertyBag },
        }
    },
    {
        ID_NAME(CLSID_CDBurn),
        {
            {    0x0,    0x0,   &IID_IObjectWithSite },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x8,   0x10,   &IID_IShellExtInit },
            {    0xc,   0x18,   &IID_IContextMenu },
            {   0x10,   0x20,   &IID_IShellPropSheetExt },
            {   0x14,   0x28,       &IID_IDropTarget },
            {   0x18,   0x30,   &IID_IPersistFile },
            {   0x1c,   0x38,   &IID_ICDBurn },
            {   0x24,   0x48,   &IID_IPersistPropertyBag },
            {   0x28,   0x50,   &IID_IDriveFolderExt},
            {   0x2c,   0x58,   &IID_INamespaceWalkCB },
            {   0x34,   0x68,   &IID_IServiceProvider },
            {   0x38,   0x70,   &IID_IQueryCancelAutoPlay },
        }
    },
    {
        ID_NAME(CLSID_ControlPanel),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {   0x10,   0x20,   &IID_IPersistFolder2 },
            {   0x10,   0x20,       &IID_IPersistFolder },
            {   0x10,   0x20,           &IID_IPersist },
            {   0x14,   0x28,   &IID_IFolderType },
            {   0x18,   0x30,   &IID_IContextMenuCB },
            {   0x1c,   0x38,   &IID_IRegItemCustomAttributes },
            { FARAWY, FARAWY,   &IID_IObjectWithBackReferences },
            { FARAWY, FARAWY,   &IID_IShellFolder2 },
            { FARAWY, FARAWY,       &IID_IShellFolder },
            { FARAWY, FARAWY,   &IID_IRegItemFolder},
            { FARAWY, FARAWY,   &IID_IShellIcon },
            { FARAWY, FARAWY,   &IID_IShellIconOverlay },
            { FARAWY, FARAWY,   &IID_IDelegateHostItemContainer },
            { FARAWY, FARAWY,   &IID_IBackReferencedObject },
            { FARAWY, FARAWY,   &IID_IStorage }
        }
    },
    {
        ID_NAME(CLSID_CopyToMenu),
        {
            {    0x0,    0x0,   &IID_IContextMenu },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellExtInit },
            {    0x8,   0x10,   &IID_IObjectWithSite },
            {   0x10,   0x20,   &IID_IFolderFilter },
        }
    },
    {
        ID_NAME(CLSID_DragDropHelper),
        {
            {    0x0,    0x0,   &IID_IDragSourceHelper },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IDropTargetHelper },
        }
    },
    {
        ID_NAME(CLSID_FadeTask),
        {
            {    0x0,    0x0,   &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_FolderItem),
        {
            {    0x0,    0x0,       &IID_FolderItem2 },
            {    0x0,    0x0,           &IID_FolderItem },
            {    0x0,    0x0,               &IID_IDispatch },
            {    0x0,    0x0,                   &IID_IUnknown },
            {    0x4,    0x8,   &IID_IPersistFolder2 },
            {    0x4,    0x8,       &IID_IPersistFolder },
            {    0x4,    0x8,           &IID_IPersist },
            {    0x8,   0x10,   &IID_IObjectSafety },
            {   0x10,   0x20,   &IID_IParentAndItem },
        }
    },
    {
        ID_NAME(CLSID_FolderItemsFDF),
        {
            {    0x0,    0x0,   &IID_FolderItems3 },
            {    0x0,    0x0,           &IID_FolderItems },
            {    0x0,    0x0,               &IID_IDispatch },
            {    0x0,    0x0,                   &IID_IUnknown },
            {    0x4,    0x8,   &IID_IPersistFolder },
            {    0x4,    0x8,       &IID_IPersist },
        }
    },
    {
        ID_NAME(CLSID_FolderShortcut),
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
            {   0x34,   0x68,   &IID_IShellIconOverlay },
        }
    },
    {
        ID_NAME(CLSID_FolderViewHost),
        {
            {    0x0,    0x0,   &IID_IShellBrowser },
            {    0x0,    0x0,       &IID_IOleWindow },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x8,   0x10,   &IID_IExplorerBrowser },
            {    0xc,   0x18,   &IID_ICommDlgBrowser3 },
            {    0xc,   0x18,       &IID_ICommDlgBrowser2 },
            {    0xc,   0x18,           &IID_ICommDlgBrowser },
            {   0x10,   0x20,   &IID_IServiceProvider },
            {   0x30,   0x60,   &IID_IObjectWithSite },
            {   0x38,   0x70,   &IID_IOleInPlaceUIWindow },
            {   0x3c,   0x78,   &IID_IShellBrowserService },
            {   0x40,   0x80,   &IID_IConnectionPointContainer },
            {   0x44,   0x88,   &IID_IPersistHistory },
            {   0x44,   0x88,       &IID_IPersist },
            {   0x48,   0x90,   &IID_IInputObject },
            {   0x54,   0xa8,   &IID_IFolderFilterSite },
            {   0x58,   0xb0,   &IID_IUrlHistoryNotify },
            {   0x58,   0xb0,       &IID_IOleCommandTarget },
            {   0x60,   0xc0,   &IID_IFolderViewHost },
            {   0x68,   0xd0,   &IID_INamespaceWalkCB2 },
            {   0x68,   0xd0,       &IID_INamespaceWalkCB },
        }
    },
    {
        ID_NAME(CLSID_ISFBand),
        {
            {  -0xb0, -0x108,   &IID_IObjectWithSite },
            {  -0xa8,  -0xf8,   &IID_IDeskBand },
            {  -0xa8,  -0xf8,       &IID_IDockingWindow },
            {  -0xa8,  -0xf8,           &IID_IOleWindow },
            {  -0xa4,  -0xf0,   &IID_IInputObject },
            {  -0xa0,  -0xe8,   &IID_IPersistStream },
            {  -0xa0,  -0xe8,       &IID_IPersist },
            {  -0x9c,  -0xe0,   &IID_IOleCommandTarget },
            {  -0x98,  -0xd8,   &IID_IServiceProvider },
            {  -0x78,  -0xa0,   &IID_IWinEventHandler },
            {  -0x74,  -0x98,   &IID_IShellChangeNotify },
            {  -0x70,  -0x90,   &IID_IDropTarget },
            {   -0x4,   -0x8,   &IID_IContextMenu },
            {    0x0,    0x0,   &IID_IShellFolderBand },
            {    0x0,    0x0,       &IID_IUnknown },
            {   0x88,   0xd8,   &IID_IFolderBandPriv },
        }
    },
    {
        ID_NAME(CLSID_MenuBand),
        {
            {  -0x34,  -0x60,   &IID_IObjectWithSite },
            {  -0x2c,  -0x50,   &IID_IDeskBand },
            {  -0x2c,  -0x50,       &IID_IDockingWindow },
            {  -0x2c,  -0x50,           &IID_IOleWindow },
            {  -0x28,  -0x48,   &IID_IInputObject },
            {  -0x24,  -0x40,   &IID_IPersistStream },
            {  -0x24,  -0x40,       &IID_IPersist },
            {  -0x20,  -0x38,   &IID_IOleCommandTarget },
            {  -0x1c,  -0x30,   &IID_IServiceProvider },
            {    0x0,    0x0,   &IID_IMenuPopup },
            {    0x0,    0x0,       &IID_IDeskBar },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IMenuBand },
            {    0x8,   0x10,   &IID_IShellMenu2 },
            {    0x8,   0x10,       &IID_IShellMenu },
            {    0xc,   0x18,   &IID_IWinEventHandler },
        }
    },
    {
        ID_NAME(CLSID_MenuBandSite),
        {
            {    0x0,    0x0,   &IID_IBandSite },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IDeskBarClient },
            {    0x4,    0x8,       &IID_IOleWindow },
            {    0x8,   0x10,   &IID_IOleCommandTarget },
            {    0xc,   0x18,   &IID_IInputObject },
            {   0x10,   0x20,   &IID_IInputObjectSite },
            {   0x14,   0x28,   &IID_IWinEventHandler },
            {   0x18,   0x30,   &IID_IServiceProvider },
        }
    },
    {
        ID_NAME(CLSID_MenuDeskBar),
        {
            {  -0x4c,  -0x88,   &IID_IOleCommandTarget },
            {  -0x48,  -0x80,   &IID_IServiceProvider },
            {  -0x44,  -0x78,   &IID_IDeskBar },
            {  -0x44,  -0x78,       &IID_IOleWindow },
            {  -0x40,  -0x70,   &IID_IInputObjectSite },
            {  -0x3c,  -0x68,   &IID_IInputObject },
            {    0x0,    0x0,   &IID_IMenuPopup },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IObjectWithSite },
            {    0x8,   0x10,   &IID_IBanneredBar },
            {    0xc,   0x18,   &IID_IInitializeObject },
        }
    },
    {
        ID_NAME(CLSID_MergedFolder),
        {
            {  -0x14,  -0x28,   &IID_IShellFolder2 },
            {  -0x10,  -0x20,   &IID_IStorage },
            {   -0xc,  -0x18,   &IID_IShellFolder },
            {   -0x8,  -0x10,   &IID_IPersistFolder2 },
            {   -0x8,  -0x10,       &IID_IPersistFolder },
            {   -0x8,  -0x10,           &IID_IPersist },
            {   -0x4,   -0x8,   &IID_IFolderType },
            {    0x0,    0x0,   &IID_IShellService },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x8,   0x10,   &IID_IObjectWithBackReferences },
        }
    },
    {
        ID_NAME(CLSID_MoveToMenu),
        {
            {    0x0,    0x0,   &IID_IContextMenu },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellExtInit },
            {    0x8,   0x10,   &IID_IObjectWithSite },
            {   0x10,   0x20,   &IID_IFolderFilter },
        }
    },
    {
        ID_NAME(CLSID_MruLongList),
        {
            {    0x0,    0x0,   &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_MruPidlList),
        {
            {    0x0,    0x0,   &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_MyComputer),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {    0xc,   0x18,   &IID_IObjectWithSite },
            {   0x18,   0x30,   &IID_IPersistFolder2 },
            {   0x18,   0x30,       &IID_IPersistFolder },
            {   0x18,   0x30,           &IID_IPersist },
            {   0x1c,   0x38,   &IID_IPersistPropertyBag },
            {   0x24,   0x48,   &IID_IItemNameLimits },
            {   0x28,   0x50,   &IID_INewItemAdvisor },
            {   0x2c,   0x58,   &IID_IContextMenuCB },
            {   0x8c,  0x108,   &IID_IShellIcon },
            {   0xac,  0x148,   &IID_IStorage },
            { FARAWY, FARAWY,   &IID_IObjectWithBackReferences },
            { FARAWY, FARAWY,   &IID_IShellFolder2 },
            { FARAWY, FARAWY,       &IID_IShellFolder },
            { FARAWY, FARAWY,   &IID_IRegItemFolder },
            { FARAWY, FARAWY,   &IID_IShellIconOverlay },
            { FARAWY, FARAWY,   &IID_IDelegateHostItemContainer },
            { FARAWY, FARAWY,   &IID_IBackReferencedObject },
        }
    },
    {
        ID_NAME(CLSID_MyDocuments),
        {
            {   -0x4,   -0x8,   &IID_IPersistFolder },
            {   -0x4,   -0x8,       &IID_IPersist },
            {    0x0,    0x0,   &IID_IShellFolder2 },
            {    0x0,    0x0,       &IID_IShellFolder },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellIconOverlay },
            { FARAWY, FARAWY,   &IID_IObjectWithSite },
            { FARAWY, FARAWY,   &IID_IShellIcon },
            { FARAWY, FARAWY,   &IID_IPersistFolder3 },
            { FARAWY, FARAWY,       &IID_IPersistFolder2 },
            { FARAWY, FARAWY,   &IID_IStorage },
            { FARAWY, FARAWY,   &IID_IItemNameLimits },
            { FARAWY, FARAWY,   &IID_IContextMenuCB },
            { FARAWY, FARAWY,   &IID_IOleCommandTarget },
            { FARAWY, FARAWY,   &IID_IPersistPropertyBag },
            { FARAWY, FARAWY,   &IID_IObjectWithBackReferences },
            { FARAWY, FARAWY,   &IID_IRemoteComputer },
            { FARAWY, FARAWY,   &IID_IFolderType },
            { FARAWY, FARAWY,   &IID_IBackReferencedObject },
            { FARAWY, FARAWY,   &IID_IParentAndItem },
        }
    },
    {
        ID_NAME(CLSID_NetworkPlaces),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {    0xc,   0x18,   &IID_IObjectWithSite },
            {   0x18,   0x30,   &IID_IPersistFolder3 },
            {   0x18,   0x30,       &IID_IPersistFolder2 },
            {   0x18,   0x30,           &IID_IPersistFolder },
            {   0x18,   0x30,               &IID_IPersist },
            {   0x28,   0x50,   &IID_IContextMenuCB },
            {   0x30,   0x60,   &IID_INewItemAdvisor },
            {   0xa4, FARAWY,   &IID_IShellIcon },
            {   0xc4, FARAWY,   &IID_IStorage },
            { FARAWY, FARAWY,   &IID_IObjectWithBackReferences },
            { FARAWY, FARAWY,   &IID_IShellFolder },
            { FARAWY, FARAWY,       &IID_IShellFolder2 },
            { FARAWY, FARAWY,   &IID_IRegItemFolder },
            { FARAWY, FARAWY,   &IID_IShellIconOverlay },
            { FARAWY, FARAWY,   &IID_IDelegateHostItemContainer },
            { FARAWY, FARAWY,   &IID_IBackReferencedObject },
        }
    },
    {
        ID_NAME(CLSID_NewMenu),
        {
            {   -0xc,  -0x18,   &IID_IObjectWithSite },
            {   -0x4,   -0x8,   &IID_IContextMenu3 },
            {   -0x4,   -0x8,       &IID_IContextMenu2 },
            {   -0x4,   -0x8,           &IID_IContextMenu },
            {    0x0,    0x0,   &IID_IShellExtInit },
            {    0x0,    0x0,       &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_PersonalStartMenu),
        {
            {  -0x30,  -0x50,   &IID_IObjectWithSite },
            {    0x0,    0x0,   &IID_IShellItemFilter },
            {    0x0,    0x0,       &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_QueryAssociations),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {    0x4,    0x8,   &IID_IQueryAssociations },
        }
    },
    {
        ID_NAME(CLSID_QuickLinks),
        {
            {  -0xb0, -0x108,   &IID_IObjectWithSite },
            {  -0xa8,  -0xf8,   &IID_IDeskBand },
            {  -0xa8,  -0xf8,       &IID_IDockingWindow },
            {  -0xa8,  -0xf8,           &IID_IOleWindow },
            {  -0xa4,  -0xf0,   &IID_IInputObject },
            {  -0xa0,  -0xe8,   &IID_IPersistStream },
            {  -0xa0,  -0xe8,       &IID_IPersist },
            {  -0x9c,  -0xe0,   &IID_IOleCommandTarget },
            {  -0x98,  -0xd8,   &IID_IServiceProvider },
            {  -0x78,  -0xa0,   &IID_IWinEventHandler },
            {  -0x74,  -0x98,   &IID_IShellChangeNotify },
            {  -0x70,  -0x90,   &IID_IDropTarget },
            {   -0x4,   -0x8,   &IID_IContextMenu },
            {    0x0,    0x0,   &IID_IShellFolderBand },
            {    0x0,    0x0,       &IID_IUnknown },
            {   0x88,   0xd8,   &IID_IFolderBandPriv },
        }
    },
    {
        ID_NAME(CLSID_RecycleBin),
        {
            {    0x0,    0x0,   &IID_IPersistFolder2 },
            {    0x0,    0x0,       &IID_IPersistFolder },
            {    0x0,    0x0,           &IID_IPersist },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellFolder2 },
            {    0x4,    0x8,       &IID_IShellFolder },
            {    0x8,   0x10,   &IID_IShellIconOverlay },
            {    0xc,   0x18,   &IID_IShellPropSheetExt },
            {   0x10,   0x20,   &IID_IShellExtInit },
            {   0x14,   0x28,   &IID_IContextMenuCB },
            {   0x18,   0x30,   &IID_IFolderType },
            {   0x1c,   0x38,   &IID_IObjectWithBackReferences },
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
        ID_NAME(CLSID_SendToMenu),
        {
            {   -0xc,  -0x18,   &IID_IObjectWithSite },
            {   -0x4,   -0x8,   &IID_IContextMenu3 },
            {   -0x4,   -0x8,       &IID_IContextMenu2 },
            {   -0x4,   -0x8,           &IID_IContextMenu },
            {    0x0,    0x0,   &IID_IShellExtInit },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IOleWindow },
            {    0x8,   0x10,   &IID_INamespaceWalkCB2 },
            {    0x8,   0x10,       &IID_INamespaceWalkCB },
            {    0xc,   0x18,   &IID_IServiceProvider },
            { FARAWY, FARAWY,   &IID_IMarshal },
        }
    },
    {
        ID_NAME(CLSID_Shell),
        {
            {    0x0,    0x0,   &IID_IShellDispatch6 },
            {    0x0,    0x0,       &IID_IShellDispatch5 },
            {    0x0,    0x0,           &IID_IShellDispatch4 },
            {    0x0,    0x0,               &IID_IShellDispatch3 },
            {    0x0,    0x0,                   &IID_IShellDispatch2 },
            {    0x0,    0x0,                       &IID_IShellDispatch },
            {    0x0,    0x0,                           &IID_IDispatch },
            {    0x0,    0x0,                               &IID_IUnknown },
            {    0x4,    0x8,   &IID_IObjectSafety },
            {   0x20,   0x40,   &IID_IObjectWithSite },
        }
    },
#if 0 // These are correct for Win8.1, but crash when un-initalized
    {
        ID_NAME(CLSID_ShellDesktop),
        {
            {   -0xc,  -0x18,   &CLSID_ShellDesktop },
            {   -0xc,  -0x18,       &IID_IObjectWithSite },
            {    0x0,    0x0,   &IID_IUnknown },
            {   0x10,   0x20,   &IID_IStorage },
            {   0x14,   0x28,   &IID_IPersistFolder2 },
            {   0x14,   0x28,       &IID_IPersistFolder },
            {   0x14,   0x28,           &IID_IPersist },
            {   0x24,   0x48,   &IID_IContextMenuCB },
            {   0x28,   0x50,   &IID_IItemNameLimits },
            {   0x2c,   0x58,   &IID_IOleCommandTarget },
            {   0x30,   0x60,   &IID_IObjectWithBackReferences },
            { FARAWY, FARAWY,   &IID_IShellIcon },
            { FARAWY, FARAWY,   &IID_IShellFolder },
            { FARAWY, FARAWY,   &IID_IShellFolder2 },
            { FARAWY, FARAWY,   &IID_IRegItemFolder },
            { FARAWY, FARAWY,   &IID_IShellIconOverlay },
            { FARAWY, FARAWY,   &IID_IDelegateHostItemContainer },
            { FARAWY, FARAWY,   &IID_IBackReferencedObject },
        }
    },
    {
        ID_NAME(CLSID_ShellFSFolder),
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
        }
    },
#endif
    {
        ID_NAME(CLSID_ShellFldSetExt),
        {
            {    0x0,    0x0,   &IID_IShellPropSheetExt },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellExtInit },
            {    0x8,   0x10,   &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_ShellFolderView),
        {
            {    0x0,    0x0,   &IID_IShellFolderViewDual3 },
            {    0x0,    0x0,       &IID_IShellFolderViewDual2 },
            {    0x0,    0x0,           &IID_IShellFolderViewDual },
            {    0x0,    0x0,               &IID_IDispatch },
            {    0x0,    0x0,                   &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellService },
            {    0x8,   0x10,   &IID_IServiceProvider },
            {    0xc,   0x18,   &IID_IObjectSafety },
            {   0x14,   0x28,   &IID_IObjectWithSite },
            {   0x1c,   0x38,   &IID_IConnectionPointContainer },
        }
    },
    {
        ID_NAME(CLSID_ShellFolderViewOC),
        {
            {    0x0,    0x0,   &IID_IFolderViewOC },
            {    0x0,    0x0,       &IID_IDispatch },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IProvideClassInfo2 },
            {    0x4,    0x8,       &IID_IProvideClassInfo },
            {    0x8,   0x10,   &IID_IObjectSafety },
            {   0x10,   0x20,   &IID_IConnectionPointContainer },
            {   0x88,   0xf0,   &IID_IPersistStreamInit },
            {   0x88,   0xf0,       &IID_IPersist },
            {   0x90,  0x100,   &IID_IOleControl },
            {   0x94,  0x108,   &IID_IOleObject },
            {   0x98,  0x110,   &IID_IOleInPlaceActiveObject },
            {   0x9c,  0x118,   &IID_IOleInPlaceObjectWindowless },
            {   0x9c,  0x118,       &IID_IOleInPlaceObject },
            {   0x9c,  0x118,           &IID_IOleWindow },
        }
    },
    {
        ID_NAME(CLSID_ShellItem),
        {
            {   -0xc,  -0x18,   &IID_IMarshal },
            {   -0x8,  -0x10,   &IID_IShellItem2 },
            {   -0x8,  -0x10,       &IID_IShellItem },
            {    0x0,    0x0,   &IID_IPersistIDList },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IParentAndItem },
            {    0x8,   0x10,   &IID_IPersistStream },
            {    0x8,   0x10,       &IID_IPersist },
            {   0x14,   0x28,   &IID_IObjectWithBackReferences },
        },
        L"Both",
    },
    {
        ID_NAME(CLSID_ShellLink),
        {
            {  -0x30,  -0x60,   &IID_IMarshal },
            {  -0x2c,  -0x58,   &IID_IShellLinkA },
            {  -0x28,  -0x50,   &IID_IShellLinkW },
            {  -0x24,  -0x48,   &IID_IPersistStream },
            {  -0x1c,  -0x38,   &IID_IPersistFile },
            {  -0x1c,  -0x38,       &IID_IPersist },
            {  -0x18,  -0x30,   &IID_IShellExtInit },
            {  -0x14,  -0x28,   &IID_IContextMenu3 },
            {  -0x14,  -0x28,       &IID_IContextMenu2 },
            {  -0x14,  -0x28,           &IID_IContextMenu },
            {  -0x10,  -0x20,   &IID_IDropTarget },
            {   -0xc,  -0x18,   &IID_IQueryInfo },
            {   -0x8,  -0x10,   &IID_IShellLinkDataList },
            {   -0x4,   -0x8,   &IID_IExtractIconA },
            {    0x0,    0x0,   &IID_IExtractIconW },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IExtractImage2 },
            {    0x4,    0x8,       &IID_IExtractImage },
            {    0x8,   0x10,   &IID_IPersistPropertyBag },
            {    0xc,   0x18,   &IID_IServiceProvider },
            {   0x10,   0x20,   &IID_IObjectWithSite },
            {   0x18,   0x30,   &IID_IPropertyStore },
            {   0x1c,   0x38,   &IID_ICustomizeInfoTip },
            { FARAWY, FARAWY,   &IID_ISLTracker },
        },
        L"Both",
    },
    {
        ID_NAME(CLSID_StartMenuPin),
        {
            {  -0x2c,  -0x58,   &IID_IShellExtInit },
            {  -0x28,  -0x50,   &IID_IContextMenu },
            {  -0x24,  -0x48,   &IID_IPinnedList },
            {  -0x1c,  -0x38,   &IID_IObjectWithSite },
            {    0x0,    0x0,   &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_Thumbnail),
        {
            {   -0x8,  -0x10,   &IID_IObjectWithSite },
            {    0x0,    0x0,   &IID_IUnknown },
        },
    },
    {
        ID_NAME(CLSID_TrackShellMenu),
        {
            {    0x0,    0x0,   &IID_ITrackShellMenu },
            {    0x0,    0x0,       &IID_IShellMenu },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellMenu2 },
            {    0x8,   0x10,   &IID_IObjectWithSite },
            {    0xc,   0x18,   &IID_IServiceProvider },
        }
    },
    {
        ID_NAME(CLSID_UserNotification),
        {
            {    0x0,    0x0,   &IID_IUserNotification },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IUserNotification2 },
        }
    },
};

static const CLASS_AND_INTERFACES ExpectedInterfaces_Win10_1607[] =
{
    {
        ID_NAME(CLSID_ActiveDesktop),
        {
            {    0x0,    0x0,   &IID_IActiveDesktop },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IActiveDesktopP },
            {    0x8,   0x10,   &IID_IADesktopP2 },
            {    0xc,   0x18,   &IID_IPropertyBag },
        }
    },
    {
        ID_NAME(CLSID_CDBurn),
        {
            {    0x0,    0x0,   &IID_IObjectWithSite },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x8,   0x10,   &IID_IShellExtInit },
            {    0xc,   0x18,   &IID_IContextMenu },
            {   0x10,   0x20,   &IID_IShellPropSheetExt },
            {   0x14,   0x28,       &IID_IDropTarget },
            {   0x18,   0x30,   &IID_IPersistFile },
            {   0x1c,   0x38,   &IID_ICDBurn },
            {   0x24,   0x48,   &IID_IPersistPropertyBag },
            {   0x28,   0x50,   &IID_IDriveFolderExt},
            {   0x2c,   0x58,   &IID_INamespaceWalkCB },
            {   0x34,   0x68,   &IID_IServiceProvider },
            {   0x38,   0x70,   &IID_IQueryCancelAutoPlay },
        }
    },
    {
        ID_NAME(CLSID_ControlPanel),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {   0x10,   0x20,   &IID_IPersistFolder2 },
            {   0x10,   0x20,       &IID_IPersistFolder },
            {   0x10,   0x20,           &IID_IPersist },
            {   0x14,   0x28,   &IID_IFolderType },
            {   0x18,   0x30,   &IID_IContextMenuCB },
            {   0x1c,   0x38,   &IID_IRegItemCustomAttributes },
            { FARAWY, FARAWY,   &IID_IObjectWithBackReferences },
            { FARAWY, FARAWY,   &IID_IShellFolder2 },
            { FARAWY, FARAWY,       &IID_IShellFolder },
            { FARAWY, FARAWY,   &IID_IShellIcon },
            { FARAWY, FARAWY,   &IID_IShellIconOverlay },
            { FARAWY, FARAWY,   &IID_IDelegateHostItemContainer },
            { FARAWY, FARAWY,   &IID_IBackReferencedObject },
            { FARAWY, FARAWY,   &IID_IStorage }
        }
    },
    {
        ID_NAME(CLSID_CopyToMenu),
        {
            {    0x0,    0x0,   &IID_IContextMenu },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellExtInit },
            {    0x8,   0x10,   &IID_IObjectWithSite },
            {   0x10,   0x20,   &IID_IFolderFilter },
        }
    },
    {
        ID_NAME(CLSID_DragDropHelper),
        {
            {    0x0,    0x0,   &IID_IDragSourceHelper },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IDropTargetHelper },
        }
    },
    {
        ID_NAME(CLSID_FadeTask),
        {
            {    0x0,    0x0,   &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_FolderItem),
        {
            {    0x0,    0x0,       &IID_FolderItem2 },
            {    0x0,    0x0,           &IID_FolderItem },
            {    0x0,    0x0,               &IID_IDispatch },
            {    0x0,    0x0,                   &IID_IUnknown },
            {    0x4,    0x8,   &IID_IPersistFolder2 },
            {    0x4,    0x8,       &IID_IPersistFolder },
            {    0x4,    0x8,           &IID_IPersist },
            {    0x8,   0x10,   &IID_IObjectSafety },
            {   0x10,   0x20,   &IID_IParentAndItem },
        }
    },
    {
        ID_NAME(CLSID_FolderItemsFDF),
        {
            {    0x0,    0x0,   &IID_FolderItems3 },
            {    0x0,    0x0,           &IID_FolderItems },
            {    0x0,    0x0,               &IID_IDispatch },
            {    0x0,    0x0,                   &IID_IUnknown },
            {    0x4,    0x8,   &IID_IPersistFolder },
            {    0x4,    0x8,       &IID_IPersist },
        }
    },
    {
        ID_NAME(CLSID_FolderShortcut),
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
            {   0x34,   0x68,   &IID_IShellIconOverlay },
        }
    },
    {
        ID_NAME(CLSID_FolderViewHost),
        {
            {    0x0,    0x0,   &IID_IShellBrowser },
            {    0x0,    0x0,       &IID_IOleWindow },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x8,   0x10,   &IID_IExplorerBrowser },
            {    0xc,   0x18,   &IID_ICommDlgBrowser3 },
            {    0xc,   0x18,       &IID_ICommDlgBrowser2 },
            {    0xc,   0x18,           &IID_ICommDlgBrowser },
            {   0x10,   0x20,   &IID_IServiceProvider },
            {   0x30,   0x60,   &IID_IObjectWithSite },
            {   0x38,   0x70,   &IID_IOleInPlaceUIWindow },
            {   0x3c,   0x78,   &IID_IShellBrowserService },
            {   0x40,   0x80,   &IID_IConnectionPointContainer },
            {   0x44,   0x88,   &IID_IPersistHistory },
            {   0x44,   0x88,       &IID_IPersist },
            {   0x48,   0x90,   &IID_IInputObject },
            {   0x54,   0xa8,   &IID_IFolderFilterSite },
            {   0x58,   0xb0,   &IID_IUrlHistoryNotify },
            {   0x58,   0xb0,       &IID_IOleCommandTarget },
            {   0x60,   0xc0,   &IID_IFolderViewHost },
            {   0x68,   0xd0,   &IID_INamespaceWalkCB2 },
            {   0x68,   0xd0,       &IID_INamespaceWalkCB },
        }
    },
    {
        ID_NAME(CLSID_ISFBand),
        {
            {  -0xb4, -0x110,   &IID_IObjectWithSite },
            {  -0xac, -0x100,   &IID_IDeskBand },
            {  -0xac, -0x100,       &IID_IDockingWindow },
            {  -0xac, -0x100,           &IID_IOleWindow },
            {  -0xa8,  -0xf8,   &IID_IInputObject },
            {  -0xa4,  -0xf0,   &IID_IPersistStream },
            {  -0xa4,  -0xf0,       &IID_IPersist },
            {  -0xa0,  -0xe8,   &IID_IOleCommandTarget },
            {  -0x9c,  -0xe0,   &IID_IServiceProvider },
            {  -0x7c,  -0xa8,   &IID_IWinEventHandler },
            {  -0x78,  -0xa0,   &IID_IShellChangeNotify },
            {  -0x74,  -0x98,   &IID_IDropTarget },
            {   -0x4,   -0x8,   &IID_IContextMenu },
            {    0x0,    0x0,   &IID_IShellFolderBand },
            {    0x0,    0x0,       &IID_IUnknown },
            {   0x88,   0xd8,   &IID_IFolderBandPriv },
        }
    },
    {
        ID_NAME(CLSID_MenuBand),
        {
            {  -0x34,  -0x60,   &IID_IObjectWithSite },
            {  -0x2c,  -0x50,   &IID_IDeskBand },
            {  -0x2c,  -0x50,       &IID_IDockingWindow },
            {  -0x2c,  -0x50,           &IID_IOleWindow },
            {  -0x28,  -0x48,   &IID_IInputObject },
            {  -0x24,  -0x40,   &IID_IPersistStream },
            {  -0x24,  -0x40,       &IID_IPersist },
            {  -0x20,  -0x38,   &IID_IOleCommandTarget },
            {  -0x1c,  -0x30,   &IID_IServiceProvider },
            {    0x0,    0x0,   &IID_IMenuPopup },
            {    0x0,    0x0,       &IID_IDeskBar },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IMenuBand },
            {    0x8,   0x10,   &IID_IShellMenu2 },
            {    0x8,   0x10,       &IID_IShellMenu },
            {    0xc,   0x18,   &IID_IWinEventHandler },
        }
    },
    {
        ID_NAME(CLSID_MenuBandSite),
        {
            {    0x0,    0x0,   &IID_IBandSite },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IDeskBarClient },
            {    0x4,    0x8,       &IID_IOleWindow },
            {    0x8,   0x10,   &IID_IOleCommandTarget },
            {    0xc,   0x18,   &IID_IInputObject },
            {   0x10,   0x20,   &IID_IInputObjectSite },
            {   0x14,   0x28,   &IID_IWinEventHandler },
            {   0x18,   0x30,   &IID_IServiceProvider },
        }
    },
    {
        ID_NAME(CLSID_MenuDeskBar),
        {
            {  -0x4c,  -0x88,   &IID_IOleCommandTarget },
            {  -0x48,  -0x80,   &IID_IServiceProvider },
            {  -0x44,  -0x78,   &IID_IDeskBar },
            {  -0x44,  -0x78,       &IID_IOleWindow },
            {  -0x40,  -0x70,   &IID_IInputObjectSite },
            {  -0x3c,  -0x68,   &IID_IInputObject },
            {    0x0,    0x0,   &IID_IMenuPopup },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IObjectWithSite },
            {    0x8,   0x10,   &IID_IBanneredBar },
            {    0xc,   0x18,   &IID_IInitializeObject },
        }
    },
    {
        ID_NAME(CLSID_MergedFolder),
        {
            {  -0x14,  -0x28,   &IID_IShellFolder2 },
            {  -0x10,  -0x20,   &IID_IStorage },
            {   -0xc,  -0x18,   &IID_IShellFolder },
            {   -0x8,  -0x10,   &IID_IPersistFolder2 },
            {   -0x8,  -0x10,       &IID_IPersistFolder },
            {   -0x8,  -0x10,           &IID_IPersist },
            {   -0x4,   -0x8,   &IID_IFolderType },
            {    0x0,    0x0,   &IID_IShellService },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x8,   0x10,   &IID_IObjectWithBackReferences },
        }
    },
    {
        ID_NAME(CLSID_MoveToMenu),
        {
            {    0x0,    0x0,   &IID_IContextMenu },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellExtInit },
            {    0x8,   0x10,   &IID_IObjectWithSite },
            {   0x10,   0x20,   &IID_IFolderFilter },
        }
    },
    {
        ID_NAME(CLSID_MyDocuments),
        {
            {   -0x4,   -0x8,   &IID_IPersistFolder },
            {   -0x4,   -0x8,       &IID_IPersist },
            {    0x0,    0x0,   &IID_IShellFolder2 },
            {    0x0,    0x0,       &IID_IShellFolder },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellIconOverlay },
            { FARAWY, FARAWY,   &IID_IObjectWithSite },
            { FARAWY, FARAWY,   &IID_IShellIcon },
            { FARAWY, FARAWY,   &IID_IPersistFolder3 },
            { FARAWY, FARAWY,       &IID_IPersistFolder2 },
            { FARAWY, FARAWY,   &IID_IStorage },
            { FARAWY, FARAWY,   &IID_IItemNameLimits },
            { FARAWY, FARAWY,   &IID_IContextMenuCB },
            { FARAWY, FARAWY,   &IID_IOleCommandTarget },
            { FARAWY, FARAWY,   &IID_IPersistPropertyBag },
            { FARAWY, FARAWY,   &IID_IObjectWithBackReferences },
            { FARAWY, FARAWY,   &IID_IRemoteComputer },
            { FARAWY, FARAWY,   &IID_IFolderType },
            { FARAWY, FARAWY,   &IID_IBackReferencedObject },
            { FARAWY, FARAWY,   &IID_IParentAndItem },
        }
    },
    {
        ID_NAME(CLSID_NetworkPlaces),
        {
            {    0x0,    0x0,   &IID_IUnknown },
            {    0xc,   0x18,   &IID_IObjectWithSite },
            {   0x18,   0x30,   &IID_IPersistFolder3 },
            {   0x18,   0x30,       &IID_IPersistFolder2 },
            {   0x18,   0x30,           &IID_IPersistFolder },
            {   0x18,   0x30,               &IID_IPersist },
            {   0x28,   0x50,   &IID_IContextMenuCB },
            {   0x30,   0x60,   &IID_INewItemAdvisor },
            {   0xa4,  0x128,   &IID_IShellIcon },
            {   0xc8,  0x170,   &IID_IStorage },
            { FARAWY, FARAWY,   &IID_IObjectWithBackReferences },
            { FARAWY, FARAWY,   &IID_IShellFolder },
            { FARAWY, FARAWY,       &IID_IShellFolder2 },
            { FARAWY, FARAWY,   &IID_IShellIconOverlay },
            { FARAWY, FARAWY,   &IID_IDelegateHostItemContainer },
            { FARAWY, FARAWY,   &IID_IBackReferencedObject },
        }
    },
    {
        ID_NAME(CLSID_NewMenu),
        {
            {   -0xc,  -0x18,   &IID_IObjectWithSite },
            {   -0x4,   -0x8,   &IID_IContextMenu3 },
            {   -0x4,   -0x8,       &IID_IContextMenu2 },
            {   -0x4,   -0x8,           &IID_IContextMenu },
            {    0x0,    0x0,   &IID_IShellExtInit },
            {    0x0,    0x0,       &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_QuickLinks),
        {
            {  -0xac, -0x100,   &IID_IDeskBand },
            {  -0xac, -0x100,       &IID_IDockingWindow },
            {  -0xac, -0x100,           &IID_IOleWindow },
            {  -0xb4, -0x110,   &IID_IObjectWithSite },
            {  -0xa8,  -0xf8,   &IID_IInputObject },
            {  -0xa4,  -0xf0,   &IID_IPersistStream },
            {  -0xa4,  -0xf0,       &IID_IPersist },
            {  -0xa0,  -0xe8,   &IID_IOleCommandTarget },
            {  -0x9c,  -0xe0,   &IID_IServiceProvider },
            {  -0x7c,  -0xa8,   &IID_IWinEventHandler },
            {  -0x78,  -0xa0,   &IID_IShellChangeNotify },
            {  -0x74,  -0x98,   &IID_IDropTarget },
            {   -0x4,   -0x8,   &IID_IContextMenu },
            {    0x0,    0x0,   &IID_IShellFolderBand },
            {    0x0,    0x0,       &IID_IUnknown },
            {   0x88,   0xd8,   &IID_IFolderBandPriv },
        }
    },
    {
        ID_NAME(CLSID_RecycleBin),
        {
            {    0x0,    0x0,   &IID_IPersistFolder2 },
            {    0x0,    0x0,       &IID_IPersistFolder },
            {    0x0,    0x0,           &IID_IPersist },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellFolder2 },
            {    0x4,    0x8,       &IID_IShellFolder },
            {    0x8,   0x10,   &IID_IShellIconOverlay },
            {    0xc,   0x18,   &IID_IShellPropSheetExt },
            {   0x10,   0x20,   &IID_IShellExtInit },
            {   0x14,   0x28,   &IID_IContextMenuCB },
            {   0x18,   0x30,   &IID_IFolderType },
            {   0x1c,   0x38,   &IID_IObjectWithBackReferences },
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
        ID_NAME(CLSID_SendToMenu),
        {
            {   -0xc,  -0x18,   &IID_IObjectWithSite },
            {   -0x4,   -0x8,   &IID_IContextMenu3 },
            {   -0x4,   -0x8,       &IID_IContextMenu2 },
            {   -0x4,   -0x8,           &IID_IContextMenu },
            {    0x0,    0x0,   &IID_IShellExtInit },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IOleWindow },
            {    0x8,   0x10,   &IID_INamespaceWalkCB2 },
            {    0x8,   0x10,       &IID_INamespaceWalkCB },
            {    0xc,   0x18,   &IID_IServiceProvider },
            { FARAWY, FARAWY,   &IID_IMarshal },
        }
    },
    {
        ID_NAME(CLSID_Shell),
        {
            {    0x0,    0x0,   &IID_IShellDispatch6 },
            {    0x0,    0x0,       &IID_IShellDispatch5 },
            {    0x0,    0x0,           &IID_IShellDispatch4 },
            {    0x0,    0x0,               &IID_IShellDispatch3 },
            {    0x0,    0x0,                   &IID_IShellDispatch2 },
            {    0x0,    0x0,                       &IID_IShellDispatch },
            {    0x0,    0x0,                           &IID_IDispatch },
            {    0x0,    0x0,                               &IID_IUnknown },
            {    0x4,    0x8,   &IID_IObjectSafety },
            {   0x20,   0x40,   &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_ShellFldSetExt),
        {
            {    0x0,    0x0,   &IID_IShellPropSheetExt },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellExtInit },
            {    0x8,   0x10,   &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_ShellFolderView),
        {
            {    0x0,    0x0,   &IID_IShellFolderViewDual3 },
            {    0x0,    0x0,       &IID_IShellFolderViewDual2 },
            {    0x0,    0x0,           &IID_IShellFolderViewDual },
            {    0x0,    0x0,               &IID_IDispatch },
            {    0x0,    0x0,                   &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellService },
            {    0x8,   0x10,   &IID_IServiceProvider },
            {    0xc,   0x18,   &IID_IObjectSafety },
            {   0x14,   0x28,   &IID_IObjectWithSite },
            {   0x1c,   0x38,   &IID_IConnectionPointContainer },
        }
    },
    {
        ID_NAME(CLSID_ShellFolderViewOC),
        {
            {    0x0,    0x0,   &IID_IFolderViewOC },
            {    0x0,    0x0,       &IID_IDispatch },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IProvideClassInfo2 },
            {    0x4,    0x8,       &IID_IProvideClassInfo },
            {    0x8,   0x10,   &IID_IObjectSafety },
            {   0x10,   0x20,   &IID_IConnectionPointContainer },
            {   0x88,   0xf0,   &IID_IPersistStreamInit },
            {   0x88,   0xf0,       &IID_IPersist },
            {   0x90,  0x100,   &IID_IOleControl },
            {   0x94,  0x108,   &IID_IOleObject },
            {   0x98,  0x110,   &IID_IOleInPlaceActiveObject },
            {   0x9c,  0x118,   &IID_IOleInPlaceObjectWindowless },
            {   0x9c,  0x118,       &IID_IOleInPlaceObject },
            {   0x9c,  0x118,           &IID_IOleWindow },
        }
    },
    {
        ID_NAME(CLSID_StartMenuPin),
        {
            {  -0x2c,  -0x58,   &IID_IShellExtInit },
            {  -0x28,  -0x50,   &IID_IContextMenu },
            {  -0x1c,  -0x38,   &IID_IObjectWithSite },
            {    0x0,    0x0,   &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_Thumbnail),
        {
            {   -0x8,  -0x10,   &IID_IObjectWithSite },
            {    0x0,    0x0,   &IID_IUnknown },
        },
    },
    {
        ID_NAME(CLSID_TrackShellMenu),
        {
            {    0x0,    0x0,   &IID_ITrackShellMenu },
            {    0x0,    0x0,       &IID_IShellMenu },
            {    0x0,    0x0,           &IID_IUnknown },
            {    0x4,    0x8,   &IID_IShellMenu2 },
            {    0x8,   0x10,   &IID_IObjectWithSite },
            {    0xc,   0x18,   &IID_IServiceProvider },
        }
    },
    {
        ID_NAME(CLSID_UserNotification),
        {
            {    0x0,    0x0,   &IID_IUserNotification },
            {    0x0,    0x0,       &IID_IUnknown },
            {    0x4,    0x8,   &IID_IUserNotification2 },
        }
    },
};

START_TEST(shell32)
{
    if (GetNTVersion() <= _WIN32_WINNT_WS03)
        TestClasses(L"shell32", ExpectedInterfaces_WS03, RTL_NUMBER_OF(ExpectedInterfaces_WS03));
    else if (GetNTVersion() <= _WIN32_WINNT_VISTA)
        TestClasses(L"shell32", ExpectedInterfaces_Vista, RTL_NUMBER_OF(ExpectedInterfaces_Vista));
    else if (GetNTVersion() <= _WIN32_WINNT_WIN7)
        TestClasses(L"shell32", ExpectedInterfaces_Win7, RTL_NUMBER_OF(ExpectedInterfaces_Win7));
    else if (GetNTVersion() <= _WIN32_WINNT_WINBLUE)
        TestClasses(L"shell32", ExpectedInterfaces_Win8, RTL_NUMBER_OF(ExpectedInterfaces_Win8));
    else
        TestClasses(L"shell32", ExpectedInterfaces_Win10_1607, RTL_NUMBER_OF(ExpectedInterfaces_Win10_1607));
}
