/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         COM interface test for shell32 classes
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "com_apitest.h"

#define NDEBUG
#include <debug.h>

static const CLASS_AND_INTERFACES ExpectedInterfaces[] =
{
    {
        ID_NAME(CLSID_CDBurn),
        {
            {    0x0,   &IID_IObjectWithSite },
            {    0x0,       &IID_IUnknown },
            {    0x8,   &IID_IShellExtInit },
            {    0xc,   &IID_IContextMenu },
            {   0x10,   &IID_IShellPropSheetExt },
            {   0x18,   &IID_IDropTarget },
            {   0x1c,   &IID_IPersistFile },
            {   0x20,   &IID_IOleCommandTarget },
            {   0x24,   &IID_ICDBurn },
            {   0x2c,   &IID_IPersistPropertyBag },
            {   0x3c,   &IID_IServiceProvider },
        }
    },
    {
        ID_NAME(CLSID_ControlPanel),
        {
            {    0x0,   &IID_IUnknown },
            {   0x10,   &IID_IPersistFolder2 },
            {   0x10,       &IID_IPersistFolder },
            {   0x10,           &IID_IPersist },
            { FARAWY,   &IID_IShellIconOverlay },
            { FARAWY,   &IID_IShellFolder2 },
            { FARAWY,       &IID_IShellFolder },
        }
    },
    {
        ID_NAME(CLSID_CopyToMenu),
        {
            {    0x0,   &IID_IContextMenu3 },
            {    0x0,       &IID_IContextMenu2 },
            {    0x0,           &IID_IContextMenu },
            {    0x0,               &IID_IUnknown },
            {    0x4,   &IID_IShellExtInit },
            {    0x8,   &IID_IObjectWithSite },
            {   0x10,   &IID_IFolderFilter },
        }
    },
    {
        ID_NAME(CLSID_DeskMovr),
        {
            {    0x0,   &IID_IUnknown },
            {   0x70,   &IID_IDeskMovr },
            {   0x74,   &IID_IOleObject },
            {   0x78,   &IID_IPersistPropertyBag },
            {   0x80,   &IID_IOleInPlaceActiveObject },
            {   0x84,   &IID_IViewObjectEx },
            {   0x84,       &IID_IViewObject2 },
            {   0x84,           &IID_IViewObject },
            {   0x88,   &IID_IOleWindow },
            {   0x88,       &IID_IOleInPlaceObject },
            {   0x88,           &IID_IOleInPlaceObjectWindowless },
        }
    },
    {
        ID_NAME(CLSID_DragDropHelper),
        {
            {    0x0,   &IID_IDragSourceHelper },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_IDropTargetHelper },
        }
    },
    {
        ID_NAME(CLSID_FadeTask),
        {
            {    0x0,   &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_FileSearchBand),
        {
            {    0x0,   &IID_IFileSearchBand },
            {    0x0,       &IID_IDispatch },
            {    0x0,           &IID_IUnknown },
            {    0x4,   &IID_IObjectWithSite },
            {    0x8,   &IID_IPersistStream },
            {    0x8,       &IID_IPersist },
            {    0xc,   &IID_IDeskBand },
            {    0xc,       &IID_IDockingWindow },
            {    0xc,           &IID_IOleWindow },
            {   0x10,   &IID_IInputObject },
            {   0x18,   &IID_IOleInPlaceSite },
        }
    },
    {
        ID_NAME(CLSID_FolderItem),
        {
            //{    0x0,   &CLSID_ShellFolderItem }, // broken QueryInterface that doesn't add a reference
            {    0x0,       &IID_FolderItem2 },
            {    0x0,           &IID_FolderItem },
            {    0x0,               &IID_IDispatch },
            {    0x0,                   &IID_IUnknown },
            {    0x4,   &IID_IPersistFolder2 },
            {    0x4,       &IID_IPersistFolder },
            {    0x4,           &IID_IPersist },
            {    0x8,   &IID_IObjectSafety },
        }
    },
    {
        ID_NAME(CLSID_FolderItemsFDF),
        {
            {    0x0,   &IID_FolderItems3 },
            //{    0x0,       &IID_FolderItems2 }, ????
            {    0x0,           &IID_FolderItems },
            {    0x0,               &IID_IDispatch },
            {    0x0,                   &IID_IUnknown },
            {    0x4,   &IID_IPersistFolder },
            {    0x8,   &IID_IObjectSafety },
        }
    },
    {
        ID_NAME(CLSID_FolderShortcut),
        {
            {    0x0,   &IID_IShellFolder2 },
            {    0x0,       &IID_IShellFolder },
            {    0x0,           &IID_IUnknown },
            {    0x4,   &IID_IPersistFolder3 },
            {    0x4,       &IID_IPersistFolder2 },
            {    0x4,           &IID_IPersistFolder },
            {    0x4,               &IID_IPersist },
            {    0x8,   &IID_IShellLinkA },
            {    0xc,   &IID_IShellLinkW },
            {   0x10,   &IID_IPersistFile },
            {   0x14,   &IID_IExtractIconW },
            {   0x18,   &IID_IQueryInfo },
            {   0x20,   &IID_IPersistStream },
            {   0x20,   &IID_IPersistStreamInit },
            {   0x24,   &IID_IPersistPropertyBag },
            {   0x28,   &IID_IBrowserFrameOptions },
        }
    },
    {
        ID_NAME(CLSID_FolderViewHost),
        {
            {    0x0,   &IID_IFolderViewHost },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_IServiceProvider },
            {    0x8,   &IID_IOleWindow },
            {    0xc,   &IID_IFolderView },
            {   0x10,   &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_ISFBand),
        {
            {  -0xac,   &IID_IDeskBand },
            {  -0xac,       &IID_IDockingWindow },
            {  -0xac,           &IID_IOleWindow },
            {  -0xa8,   &IID_IObjectWithSite },
            {  -0xa0,   &IID_IInputObject },
            {  -0x9c,   &IID_IPersistStream },
            {  -0x9c,       &IID_IPersist },
            {  -0x98,   &IID_IOleCommandTarget },
            {  -0x94,   &IID_IServiceProvider },
            {  -0x78,   &IID_IWinEventHandler },
            {  -0x74,   &IID_IShellChangeNotify },
            {  -0x70,   &IID_IDropTarget },
            {   -0x4,   &IID_IContextMenu },
            {    0x0,   &IID_IShellFolderBand },
            {    0x0,       &IID_IUnknown },
            {   0x94,   &IID_IFolderBandPriv },
        }
    },
    {
        ID_NAME(CLSID_MenuBand),
        {
            {  -0x30,   &IID_IDeskBand },
            {  -0x30,       &IID_IDockingWindow },
            {  -0x30,           &IID_IOleWindow },
            {  -0x2c,   &IID_IObjectWithSite },
            {  -0x24,   &IID_IInputObject },
            {  -0x20,   &IID_IPersistStream },
            {  -0x20,       &IID_IPersist },
            {  -0x1c,   &IID_IOleCommandTarget },
            {  -0x18,   &IID_IServiceProvider },
            {    0x0,   &IID_IMenuPopup },
            {    0x0,       &IID_IDeskBar },
            {    0x0,           &IID_IUnknown },
            {    0x4,   &IID_IMenuBand },
            {    0x8,   &IID_IShellMenu2 },
            {    0x8,       &IID_IShellMenu },
            {    0xc,   &IID_IWinEventHandler },
            {   0x10,   &IID_IShellMenuAcc },
        }
    },
    {
        ID_NAME(CLSID_MenuBandSite),
        {
            {    0x0,   &IID_IBandSite },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_IDeskBarClient },
            {    0x4,       &IID_IOleWindow },
            {    0x8,   &IID_IOleCommandTarget },
            {    0xc,   &IID_IInputObject },
            {   0x10,   &IID_IInputObjectSite },
            {   0x14,   &IID_IWinEventHandler },
            {   0x18,   &IID_IServiceProvider },
        }
    },
    {
        ID_NAME(CLSID_MenuDeskBar),
        {
            {  -0x48,   &IID_IOleCommandTarget },
            {  -0x44,   &IID_IServiceProvider },
            {  -0x40,   &IID_IDeskBar },
            {  -0x40,       &IID_IOleWindow },
            {  -0x3c,   &IID_IInputObjectSite },
            {  -0x38,   &IID_IInputObject },
            {    0x0,   &IID_IMenuPopup },
            {    0x0,           &IID_IUnknown },
            {    0x4,   &IID_IObjectWithSite },
            {    0x8,   &IID_IBanneredBar },
            {    0xc,   &IID_IInitializeObject },
        }
    },
#if 0 // This is registered to shell32, but can't be instanciated
    {
        ID_NAME(CLSID_MenuToolbarBase),
        {
            {    0x0,   &IID_IUnknown },
        }
    },
#endif
    {
        ID_NAME(CLSID_MergedFolder),
        {
            {   -0x8,   &IID_IShellFolder2 },
            {   -0x4,   &IID_IStorage },
            {    0x0,   &IID_IAugmentedShellFolder2 },
            {    0x0,       &IID_IAugmentedShellFolder },
            {    0x0,           &IID_IShellFolder },
            {    0x0,               &IID_IUnknown },
            {    0x4,   &IID_IShellService },
            {    0x8,   &IID_ITranslateShellChangeNotify },
            {    0xc,   &IID_IPersistFolder2 },
            {    0xc,       &IID_IPersistFolder },
            {    0xc,           &IID_IPersist },
            {   0x10,   &IID_IPersistPropertyBag },
            {   0x14,   &IID_IShellIconOverlay },
        }
    },
    {
        ID_NAME(CLSID_MoveToMenu),
        {
            {    0x0,   &IID_IContextMenu3 },
            {    0x0,       &IID_IContextMenu2 },
            {    0x0,           &IID_IContextMenu },
            {    0x0,               &IID_IUnknown },
            {    0x4,   &IID_IShellExtInit },
            {    0x8,   &IID_IObjectWithSite },
            {   0x10,   &IID_IFolderFilter },
        }
    },
    {
        ID_NAME(CLSID_MyComputer),
        {
            {    0x0,   &IID_IUnknown },
            {   0x10,   &IID_IPersistFolder2 },
            {   0x10,       &IID_IPersistFolder },
            {   0x10,           &IID_IPersist },
            {   0x10,           &IID_IPersistFreeThreadedObject },
            { FARAWY,   &IID_IShellIconOverlay },
            { FARAWY,   &IID_IShellFolder2 },
            { FARAWY,       &IID_IShellFolder },
        }
    },
    {
        ID_NAME(CLSID_MyDocuments),
        {
            {   -0x4,   &IID_IPersistFolder },
            {   -0x4,       &IID_IPersist },
            {    0x0,   &IID_IShellFolder2 },
            {    0x0,       &IID_IShellFolder },
            {    0x0,           &IID_IUnknown },
            {    0x4,   &IID_IShellIconOverlay },
            { FARAWY,   &IID_IShellIcon },
            { FARAWY,   &IID_IPersistFolder3 },
            { FARAWY,       &IID_IPersistFolder2 },
            { FARAWY,   &IID_IStorage },
            { FARAWY,   &IID_IContextMenuCB },
            { FARAWY,   &IID_IOleCommandTarget },
        }
    },
    {
        ID_NAME(CLSID_NetworkPlaces),
        {
            {    0x0,   &IID_IUnknown },
            {   0x10,   &IID_IPersistFolder3 },
            {   0x10,       &IID_IPersistFolder2 },
            {   0x10,           &IID_IPersistFolder },
            {   0x10,               &IID_IPersist },
            {   0x10,               &IID_IPersistFreeThreadedObject },
            { FARAWY,   &IID_IShellIconOverlay },
            { FARAWY,   &IID_IShellFolder2 },
            { FARAWY,       &IID_IShellFolder },
        }
    },
    {
        ID_NAME(CLSID_NewMenu),
        {
            {   -0xc,   &IID_IObjectWithSite },
            {   -0x4,   &IID_IContextMenu3 },
            {   -0x4,       &IID_IContextMenu2 },
            {   -0x4,           &IID_IContextMenu },
            {    0x0,   &IID_IShellExtInit },
            {    0x0,       &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_PersonalStartMenu),
        {
            {  -0x30,   &IID_IDeskBand },
            {  -0x30,       &IID_IDockingWindow },
            {  -0x30,           &IID_IOleWindow },
            {  -0x2c,   &IID_IObjectWithSite },
            {  -0x24,   &IID_IInputObject },
            {  -0x20,   &IID_IPersistStream },
            {  -0x20,       &IID_IPersist },
            {  -0x1c,   &IID_IOleCommandTarget },
            {  -0x18,   &IID_IServiceProvider },
            {    0x0,   &IID_IMenuPopup },
            {    0x0,       &IID_IDeskBar },
            {    0x0,           &IID_IUnknown },
            {    0x4,   &IID_IMenuBand },
            {    0x8,   &IID_IShellMenu2 },
            {    0x8,       &IID_IShellMenu },
            {    0xc,   &IID_IWinEventHandler },
            {   0x10,   &IID_IShellMenuAcc },
        }
    },
    {
        ID_NAME(CLSID_Printers),
        {
            {   -0xc,   &IID_IRemoteComputer },
            {    0x0,   &IID_IShellFolder2 },
            {    0x0,       &IID_IShellFolder },
            {    0x0,           &IID_IUnknown },
            {    0x4,   &IID_IPersistFolder2 },
            {    0x4,       &IID_IPersistFolder },
            {    0x4,           &IID_IPersist },
            {    0x8,   &IID_IContextMenuCB },
            {    0xc,   &IID_IShellIconOverlay },
        }
    },
    {
        ID_NAME(CLSID_QueryAssociations),
        {
            {    0x0,   &IID_IUnknown },
            {    0x8,   &IID_IQueryAssociations },
        }
    },
    {
        ID_NAME(CLSID_QuickLinks),
        {
            {  -0xac,   &IID_IDeskBand },
            {  -0xac,       &IID_IDockingWindow },
            {  -0xac,           &IID_IOleWindow },
            {  -0xa8,   &IID_IObjectWithSite },
            {  -0xa0,   &IID_IInputObject },
            {  -0x9c,   &IID_IPersistStream },
            {  -0x9c,       &IID_IPersist },
            {  -0x98,   &IID_IOleCommandTarget },
            {  -0x94,   &IID_IServiceProvider },
            {  -0x78,   &IID_IWinEventHandler },
            {  -0x74,   &IID_IShellChangeNotify },
            {  -0x70,   &IID_IDropTarget },
            {   -0x4,   &IID_IContextMenu },
            {    0x0,   &IID_IShellFolderBand },
            {    0x0,       &IID_IUnknown },
            {   0x94,   &IID_IFolderBandPriv },
        }
    },
    {
        ID_NAME(CLSID_RecycleBin),
        {
            {    0x0,   &IID_IPersistFolder2 },
            {    0x0,       &IID_IPersistFolder },
            //{    0x0,           &IID_IPersist },
            {    0x0,           &IID_IUnknown },
            {    0x4,   &IID_IShellFolder2 },
            {    0x4,       &IID_IShellFolder },
            {    0x8,   &IID_IContextMenu },
            {    0xc,   &IID_IShellPropSheetExt },
            {   0x10,   &IID_IShellExtInit },
        }
    },
    {
        ID_NAME(CLSID_SendToMenu),
        {
            {   -0x4,   &IID_IContextMenu3 },
            {   -0x4,       &IID_IContextMenu2 },
            {   -0x4,           &IID_IContextMenu },
            {    0x0,   &IID_IShellExtInit },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_IOleWindow },
        }
    },
    {
        ID_NAME(CLSID_Shell),
        {
            {    0x0,   &IID_IShellDispatch4 },
            {    0x0,       &IID_IShellDispatch3 },
            {    0x0,           &IID_IShellDispatch2 },
            {    0x0,               &IID_IShellDispatch },
            {    0x0,                   &IID_IDispatch },
            {    0x0,                       &IID_IUnknown },
            {    0x4,   &IID_IObjectSafety },
            {   0x20,   &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_ShellDesktop),
        {
            {   -0x8,   &CLSID_ShellDesktop },
            {   -0x8,       &IID_IObjectWithSite },
            {    0x0,   &IID_IUnknown },
            {    0x4,   &IID_IStorage },
            {    0x8,   &IID_IPersistFolder2 },
            {    0x8,       &IID_IPersistFolder },
            {    0x8,           &IID_IPersist },
            {    0xc,   &IID_IShellIcon },
            {   0x14,   &IID_IContextMenuCB },
            {   0x18,   &IID_ITranslateShellChangeNotify },
            {   0x20,   &IID_IOleCommandTarget },
            { FARAWY,   &IID_IShellIconOverlay },
            { FARAWY,   &IID_IShellFolder2 },
            { FARAWY,       &IID_IShellFolder },
        }
    },
    {
        ID_NAME(CLSID_ShellFSFolder),
        {
            {    0x0,   &IID_IUnknown },
            {    0xc,   &IID_IShellFolder2 },
            {    0xc,       &IID_IShellFolder },
            {   0x10,   &IID_IShellIcon },
            {   0x14,   &IID_IShellIconOverlay },
            {   0x18,   &IID_IPersistFolder3 },
            {   0x18,       &IID_IPersistFolder2 },
            {   0x18,           &IID_IPersistFolder },
            {   0x18,               &IID_IPersist },
            {   0x18,               &IID_IPersistFreeThreadedObject },
            {   0x1c,   &IID_IStorage },
            {   0x2c,   &IID_IContextMenuCB },
            {   0x34,   &IID_IOleCommandTarget },
        }
    },
    {
        ID_NAME(CLSID_ShellFldSetExt),
        {
            {    0x0,   &IID_IShellPropSheetExt },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_IShellExtInit },
            {    0x8,   &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_ShellFolderView),
        {
            {    0x0,   &IID_IShellFolderViewDual2 },
            {    0x0,       &IID_IShellFolderViewDual },
            {    0x0,           &IID_IDispatch },
            {    0x0,               &IID_IUnknown },
            {    0x4,   &IID_IShellService },
            {    0x8,   &IID_IServiceProvider },
            {    0xc,   &IID_IObjectSafety },
            {   0x14,   &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_ShellFolderViewOC),
        {
            {    0x0,   &IID_IFolderViewOC },
            {    0x0,       &IID_IDispatch },
            {    0x0,           &IID_IUnknown },
            {    0x8,   &IID_IObjectSafety },
            {   0x88,   &IID_IPersistStreamInit },
            {   0x88,       &IID_IPersist },
            {   0x90,   &IID_IOleObject },
            {   0x94,   &IID_IOleInPlaceActiveObject },
            {   0x98,   &IID_IOleInPlaceObjectWindowless },
            {   0x98,       &IID_IOleInPlaceObject },
            {   0x98,           &IID_IOleWindow },
        }
    },
    {
        ID_NAME(CLSID_ShellItem),
        {
            {    0x0,   &IID_IShellItem },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_IPersistIDList },
        }
    },
    {
        ID_NAME(CLSID_ShellLink),
        {
            {    0x0,   &IID_IShellLinkA },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_IShellLinkW },
            {    0x8,   &IID_IPersistStream },
            {    0xc,   &IID_IPersistFile },
            {   0x10,   &IID_IShellExtInit },
            {   0x14,   &IID_IContextMenu3 },
            {   0x14,       &IID_IContextMenu2 },
            {   0x14,           &IID_IContextMenu },
            {   0x18,   &IID_IDropTarget },
            {   0x1c,   &IID_IQueryInfo },
            {   0x20,   &IID_IShellLinkDataList },
            {   0x24,   &IID_IExtractIconA },
            {   0x28,   &IID_IExtractIconW },
            {   0x2c,   &IID_IExtractImage2 },
            {   0x2c,       &IID_IExtractImage },
            {   0x30,   &IID_IPersistPropertyBag },
            {   0x34,   &IID_IServiceProvider },
            {   0x3c,   &IID_IObjectWithSite },
        }
    },
#if 0 // Apparently we can only get this through Folder.Items().GetLink
    {
        ID_NAME(CLSID_ShellLinkObject),
        {
            {    0x0,       &IID_IUnknown },
        }
    },
#endif
    {
        ID_NAME(CLSID_StartMenu),
        {
            {  -0x48,   &IID_IOleCommandTarget },
            {  -0x44,   &IID_IServiceProvider },
            {  -0x40,   &IID_IDeskBar },
            {  -0x40,       &IID_IOleWindow },
            {  -0x3c,   &IID_IInputObjectSite },
            {  -0x38,   &IID_IInputObject },
            {    0x0,   &IID_IMenuPopup },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_IObjectWithSite },
            {    0x8,   &IID_IBanneredBar },
            {    0xc,   &IID_IInitializeObject },
        }
    },
    {
        ID_NAME(CLSID_StartMenuPin),
        {
            {    0x0,   &IID_IShellExtInit },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_IContextMenu },
            {    0xc,   &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_TrackShellMenu),
        {
            {    0x0,   &IID_ITrackShellMenu },
            {    0x0,       &IID_IShellMenu },
            {    0x0,           &IID_IUnknown },
            {    0x4,   &IID_IShellMenu2 },
            {    0x8,   &IID_IObjectWithSite },
            {    0xc,   &IID_IServiceProvider },
        }
    },
};
static const INT ExpectedInterfaceCount = RTL_NUMBER_OF(ExpectedInterfaces);

START_TEST(shell32)
{
    TestClasses(L"shell32", ExpectedInterfaces, ExpectedInterfaceCount);
}
