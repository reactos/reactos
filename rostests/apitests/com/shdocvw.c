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
    {
        ID_NAME(CLSID_AdminFolderShortcut),
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
        ID_NAME(CLSID_ExplorerBand),
        {
            {  -0xb4,   &IID_IDeskBand },
            {  -0xb4,       &IID_IDockingWindow },
            {  -0xb4,           &IID_IOleWindow },
            {  -0xb0,   &IID_IObjectWithSite },
            {  -0xa8,   &IID_IInputObject },
            {  -0xa4,   &IID_IPersistStream },
            {  -0xa4,       &IID_IPersist },
            {  -0xa0,   &IID_IOleCommandTarget },
            {  -0x9c,   &IID_IServiceProvider },
            {  -0x84,   &IID_IContextMenu },
            {  -0x80,   &IID_IBandNavigate },
            {  -0x7c,   &IID_IWinEventHandler },
            {  -0x78,   &IID_INamespaceProxy },
            {    0x0,   &IID_IDispatch },
            {    0x0,       &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_FontsFolderShortcut),
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
#if 0 // E_OUTOFMEMORY?
    {
        ID_NAME(CLSID_ShellDispatchInproc),
        {
            {    0x0,   &IID_IUnknown },
        }
    },
#endif
    {
        ID_NAME(CLSID_MruLongList),
        {
            {    0x0,   &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_SH_FavBand),
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
            {    0x0,   &IID_IContextMenu },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_IBandNavigate },
            {    0x8,   &IID_IWinEventHandler },
            {    0xc,   &IID_INamespaceProxy },
        }
    },
    {
        ID_NAME(CLSID_SH_HistBand),
        {
            {  -0xb4,   &IID_IDeskBand },
            {  -0xb4,       &IID_IDockingWindow },
            {  -0xb4,           &IID_IOleWindow },
            {  -0xb0,   &IID_IObjectWithSite },
            {  -0xa8,   &IID_IInputObject },
            {  -0xa4,   &IID_IPersistStream },
            {  -0xa4,       &IID_IPersist },
            {  -0xa0,   &IID_IOleCommandTarget },
            {  -0x9c,   &IID_IServiceProvider },
            {  -0x84,   &IID_IContextMenu },
            {  -0x80,   &IID_IBandNavigate },
            {  -0x7c,   &IID_IWinEventHandler },
            {  -0x78,   &IID_INamespaceProxy },
            {    0x0,   &IID_IShellFolderSearchableCallback },
            {    0x0,       &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_SearchAssistantOC),
        {
            {    0x0,   &IID_ISearchAssistantOC3 },
            {    0x0,       &IID_ISearchAssistantOC },
            {    0x0,           &IID_IDispatch },
            {    0x0,               &IID_IUnknown },
            {    0x4,   &IID_IProvideClassInfo2 },
            {    0x4,       &IID_IProvideClassInfo },
            {    0x8,   &IID_IQuickActivate },
            {    0xc,   &IID_IOleControl },
            {   0x10,   &IID_IOleObject },
            {   0x14,   &IID_IOleInPlaceActiveObject },
            {   0x18,   &IID_IViewObjectEx },
            {   0x18,       &IID_IViewObject2 },
            {   0x18,           &IID_IViewObject },
            {   0x1c,   &IID_IOleInPlaceObjectWindowless },
            {   0x1c,       &IID_IOleInPlaceObject },
            {   0x1c,           &IID_IOleWindow },
            {   0x20,   &IID_IDataObject },
            {   0x30,   &IID_IConnectionPointContainer },
            {   0x34,   &IID_IObjectSafety },
            {   0x3c,   &IID_IOleCommandTarget },
            {   0x40,   &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_SearchBand),
        {
            {    0x0,   &IID_IContextMenu },
            {    0x0,       &IID_IUnknown },
            {    0x4,   &IID_IShellExtInit },
            {    0x8,   &IID_IPersistPropertyBag },
            {    0x8,       &IID_IPersist },
        }
    },
    {
        ID_NAME(CLSID_ShellSearchAssistantOC),
        {
            {    0x0,   &IID_ISearchAssistantOC3 },
            {    0x0,       &IID_ISearchAssistantOC },
            {    0x0,           &IID_IDispatch },
            {    0x0,               &IID_IUnknown },
            {    0x4,   &IID_IProvideClassInfo2 },
            {    0x4,       &IID_IProvideClassInfo },
            {    0x8,   &IID_IQuickActivate },
            {    0xc,   &IID_IOleControl },
            {   0x10,   &IID_IOleObject },
            {   0x14,   &IID_IOleInPlaceActiveObject },
            {   0x18,   &IID_IViewObjectEx },
            {   0x18,       &IID_IViewObject2 },
            {   0x18,           &IID_IViewObject },
            {   0x1c,   &IID_IOleInPlaceObjectWindowless },
            {   0x1c,       &IID_IOleInPlaceObject },
            {   0x1c,           &IID_IOleWindow },
            {   0x20,   &IID_IDataObject },
            {   0x30,   &IID_IConnectionPointContainer },
            {   0x34,   &IID_IObjectSafety },
            {   0x3c,   &IID_IOleCommandTarget },
            {   0x40,   &IID_IObjectWithSite },
        }
    },
    {
        ID_NAME(CLSID_ShellShellNameSpace),
        {
            {    0x0,   &IID_IShellNameSpace },
            {    0x0,       &IID_IShellFavoritesNameSpace },
            {    0x0,           &IID_IDispatch },
            {    0x0,               &IID_IUnknown },
            {    0x4,   &IID_IProvideClassInfo2 },
            {    0x4,       &IID_IProvideClassInfo },
            {    0x8,   &IID_IPersistStreamInit },
            {    0xc,   &IID_IPersistPropertyBag },
            {   0x10,   &IID_IQuickActivate },
            {   0x14,   &IID_IOleControl },
            {   0x18,   &IID_IOleObject },
            {   0x1c,   &IID_IOleInPlaceActiveObject },
            {   0x20,   &IID_IViewObjectEx },
            {   0x20,       &IID_IViewObject2 },
            {   0x20,           &IID_IViewObject },
            {   0x24,   &IID_IOleInPlaceObjectWindowless },
            {   0x24,       &IID_IOleInPlaceObject },
            {   0x24,           &IID_IOleWindow },
            {   0x28,   &IID_ISpecifyPropertyPages },
            {   0x38,   &IID_IConnectionPointContainer },
            {   0x3c,   &IID_IShellChangeNotify },
            {   0x40,   &IID_IDropTarget },
            {   0xb4,   &IID_IObjectWithSite },
            {   0xbc,   &IID_INSCTree2 },
            {   0xbc,       &IID_INSCTree },
            {   0xc0,   &IID_IWinEventHandler },
            {   0xc4,   &IID_IShellBrowser },
            {   0xc8,   &IID_IFolderFilterSite },
        }
    },
    {
        ID_NAME(CLSID_TaskbarList),
        {
            {    0x0,   &IID_ITaskbarList2 },
            {    0x0,       &IID_ITaskbarList },
            {    0x0,           &IID_IUnknown },
        }
    },
    {
        ID_NAME(CLSID_AttachmentServices ),
        {
            {    0x0,   &IID_IAttachmentExecute },
            {    0x0,       &IID_IUnknown },
        }
    },
};
static const INT ExpectedInterfaceCount = RTL_NUMBER_OF(ExpectedInterfaces);

START_TEST(shdocvw)
{
    TestClasses(L"shdocvw", ExpectedInterfaces, ExpectedInterfaceCount);
}
