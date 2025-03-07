/*
 * PROJECT:     ReactOS header
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Undocumented shell interface
 * COPYRIGHT:   Copyright 2009 Andrew Hill <ash77 at domain reactos.org>
 *              Copyright 2013 Dominik Hornung
 *              Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include <shlwapi_undoc.h> // For ASSOCQUERY

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagSLOTITEMDATA
{
    DWORD dwFlags;
    UINT cbData;
    LPVOID pvData;
} SLOTITEMDATA, *PSLOTITEMDATA;

typedef INT (CALLBACK *SLOTCOMPARE)(LPCVOID pvData1, LPCVOID pvData2, UINT cbData);

/*****************************************************************************
 * New shellstate structure
 */
struct SHELLSTATE2
{
	SHELLSTATE								oldState;
	long									newState1;
	long									newState2;
};

/*****************************************************************************
 * Header for persisted view state in cabinet windows
 */
struct persistState
{
	long									dwSize;
	long									browseType;
	long									alwaysZero;
	long									browserIndex;
	CLSID									persistClass;
	ULONG									pidlSize;
};

/*****************************************************************************
 * CGID_Explorer (IShellBrowser OLECMD IDs)
 */
#define SBCMDID_EXPLORERBARFOLDERS 35 // Query/Toggle
#define SBCMDID_MIXEDZONE 39
#define SBCMDID_ONVIEWMOVETOTOP 60
//SBCMDID_ENABLESHOWTREE ?
//SBCMDID_SHOWCONTROL ?
//SBCMDID_CANCELNAVIGATION ?
//SBCMDID_MAYSAVECHANGES ?
//SBCMDID_SETHLINKFRAME ?
//SBCMDID_ENABLESTOP ?
//SBCMDID_SELECTHISTPIDL ?
//SBCMDID_GETPANE ? // This is in the official SDK but only the panes are defined
#define PANE_NONE       ((DWORD)-1)
#define PANE_ZONE       1
#define PANE_OFFLINE    2
#define PANE_PRINTER    3
#define PANE_SSL        4
#define PANE_NAVIGATION 5
#define PANE_PROGRESS   6
#if (_WIN32_IE >= _WIN32_IE_IE60)
#define PANE_PRIVACY    7
#endif

/*****************************************************************************
 * CGID_DefView OLECMD IDs
 */
#define DVCMDID_SET_DEFAULTFOLDER_SETTINGS 0
#define DVCMDID_RESET_DEFAULTFOLDER_SETTINGS 1

/*****************************************************************************
 * IInitializeObject interface
 */
#undef  INTERFACE
#define INTERFACE IInitializeObject

DECLARE_INTERFACE_(IInitializeObject, IUnknown)//, "4622AD16-FF23-11d0-8D34-00A0C90F2719")
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(Initialize)(THIS) PURE;
};
#undef INTERFACE
#if !defined(__cplusplus) || defined(CINTERFACE)
#define IInitializeObject_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IInitializeObject_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IInitializeObject_Release(T) (T)->lpVtbl->Release(T)
#define IInitializeObject_Initialize(T) (T)->lpVtbl->Initialize(T)
#endif


/*****************************************************************************
 * IBanneredBar interface
 */
enum
{
    BMICON_LARGE = 0,
    BMICON_SMALL
};
#define INTERFACE IBanneredBar
DECLARE_INTERFACE_(IBanneredBar, IUnknown)//, "596A9A94-013E-11d1-8D34-00A0C90F2719")
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(SetIconSize)(THIS_ DWORD iIcon) PURE;
    STDMETHOD(GetIconSize)(THIS_ DWORD* piIcon) PURE;
    STDMETHOD(SetBitmap)(THIS_ HBITMAP hBitmap) PURE;
    STDMETHOD(GetBitmap)(THIS_ HBITMAP* phBitmap) PURE;

};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IBanneredBar_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IBanneredBar_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IBanneredBar_Release(T) (T)->lpVtbl->Release(T)
#define IBanneredBar_SetIconSize(T,a) (T)->lpVtbl->SetIconSize(T,a)
#define IBanneredBar_GetIconSize(T,a) (T)->lpVtbl->GetIconSize(T,a)
#define IBanneredBar_SetBitmap(T,a) (T)->lpVtbl->SetBitmap(T,a)
#define IBanneredBar_GetBitmap(T,a) (T)->lpVtbl->GetBitmap(T,a)
#endif

/*****************************************************************************
 * IGlobalFolderSettings interface
 */
struct DEFFOLDERSETTINGS
{
    enum { SIZE_NT4 = 8, SIZE_IE4 = 36, SIZE_XP = 40 };
    enum { VER_98 = 0, VER_2000 = 3, VER_XP = 4 }; // Win98SE with IE5 writes 0, not 3 as the version
    UINT Statusbar : 1; // "StatusBarOther" is the new location for this
    UINT Toolbar : 1; // Not used when Explorer uses ReBar
    FOLDERSETTINGS FolderSettings;
    SHELLVIEWID vid;
    UINT Version;
    UINT Counter; // Incremented every time default folder settings are applied. Invalidates a cache?
    UINT ViewPriority; // VIEW_PRIORITY_*
};

#undef  INTERFACE
#define INTERFACE   IGlobalFolderSettings
DECLARE_INTERFACE_(IGlobalFolderSettings, IUnknown)
{
    /*** IUnknown ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IGlobalFolderSettings ***/
    STDMETHOD(Get)(THIS_ struct DEFFOLDERSETTINGS *pFDS, UINT cb) PURE;
    STDMETHOD(Set)(THIS_ const struct DEFFOLDERSETTINGS *pFDS, UINT cb, UINT unknown) PURE;
};
#undef INTERFACE

/*****************************************************************************
 * IStartMenuCallback interface
 */
#define INTERFACE IStartMenuCallback
DECLARE_INTERFACE_(IStartMenuCallback, IOleWindow)
{
    /*** IUnknown ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IOleWindow methods ***/
    STDMETHOD_(HRESULT,GetWindow)(THIS_ HWND*) PURE;
    STDMETHOD_(HRESULT,ContextSensitiveHelp)(THIS_ BOOL) PURE;
    /*** IStartMenuCallback ***/
    STDMETHOD_(HRESULT,Execute)(THIS_ IShellFolder*,LPCITEMIDLIST) PURE;
    STDMETHOD_(HRESULT,Unknown)(THIS_ PVOID,PVOID,PVOID,PVOID) PURE;
    STDMETHOD_(HRESULT,AppendMenu)(THIS_ HMENU*) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IStartMenuCallback_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IStartMenuCallback_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IStartMenuCallback_Release(T) (T)->lpVtbl->Release(T)
#define IStartMenuCallback_GetWindow(T,a) (T)->lpVtbl->GetWindow(T,a)
#define IStartMenuCallback_ContextSensitiveHelp(T,a) (T)->lpVtbl->ContextSensitiveHelp(T,a)
#define IStartMenuCallback_Execute(T,a,b) (T)->lpVtbl->Execute(T,a,b)
#define IStartMenuCallback_Unknown(T,a,b,c,d) (T)->lpVtbl->Unknown(T,a,b,c,d)
#define IStartMenuCallback_AppendMenu(T,a) (T)->lpVtbl->AppendMenu(T,a)
#endif

/*****************************************************************************
 * IBandSiteStreamCallback interface
 */
#define INTERFACE IBandSiteStreamCallback
DECLARE_INTERFACE_(IBandSiteStreamCallback, IUnknown)
{
    /*** IUnknown ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IBandSiteStreamCallback ***/
    STDMETHOD_(HRESULT,OnLoad)(THIS_ IStream *pStm, REFIID riid, PVOID *pvObj) PURE;
    STDMETHOD_(HRESULT,OnSave)(THIS_ IUnknown *pUnk, IStream *pStm) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IBandSiteStreamCallback_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IBandSiteStreamCallback_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IBandSiteStreamCallback_Release(T) (T)->lpVtbl->Release(T)
#define IBandSiteStreamCallback_OnLoad(T,a,b,c) (T)->lpVtbl->OnLoad(T,a,b,c)
#define IBandSiteStreamCallback_OnSave(T,a,b) (T)->lpVtbl->OnSave(T,a,b)
#endif

/*****************************************************************************
 * IShellDesktopTray interface
 */
#define INTERFACE IShellDesktopTray
DECLARE_INTERFACE_(IShellDesktopTray, IUnknown)
{
    /*** IUnknown ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IShellDesktopTray ***/
    STDMETHOD_(ULONG,GetState)(THIS) PURE;
    STDMETHOD(GetTrayWindow)(THIS_ HWND*) PURE;
    STDMETHOD(RegisterDesktopWindow)(THIS_ HWND) PURE;
    STDMETHOD(Unknown)(THIS_ DWORD,DWORD) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IShellDesktopTray_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IShellDesktopTray_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IShellDesktopTray_Release(T) (T)->lpVtbl->Release(T)
#define IShellDesktopTray_GetState(T) (T)->lpVtbl->GetState(T)
#define IShellDesktopTray_GetTrayWindow(T,a) (T)->lpVtbl->GetTrayWindow(T,a)
#define IShellDesktopTray_RegisterDesktopWindow(T,a) (T)->lpVtbl->RegisterDesktopWindow(T,a)
#define IShellDesktopTray_Unknown(T,a,b) (T)->lpVtbl->Unknown(T,a,b)
#endif

/*****************************************************************************
 * INscTree interface
 */
#define INTERFACE INscTree
DECLARE_INTERFACE_(INscTree, IUnknown)
{
    /*** IUnknown ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** INscTree ***/
	STDMETHOD(CreateTree)(THIS_ long paramC, long param10, long param14) PURE;
	STDMETHOD(Initialize)(THIS_ long paramC, long param10, long param14) PURE;
	STDMETHOD(ShowWindow)(THIS_ long paramC) PURE;
	STDMETHOD(Refresh)(THIS) PURE;
	STDMETHOD(GetSelectedItem)(THIS_ long paramC, long param10) PURE;
	STDMETHOD(SetSelectedItem)(THIS_ long paramC, long param10, long param14, long param18) PURE;
	STDMETHOD(GetNscMode)(THIS_ long paramC) PURE;
	STDMETHOD(SetNscMode)(THIS_ long paramC) PURE;
	STDMETHOD(GetSelectedItemName)(THIS_ long paramC, long param10) PURE;
	STDMETHOD(BindToSelectedItemParent)(THIS_ long paramC, long param10, long param14) PURE;
	STDMETHOD(InLabelEdit)(THIS) PURE;
	STDMETHOD(RightPaneNavigationStarted)(THIS_ long paramC) PURE;
	STDMETHOD(RightPaneNavigationFinished)(THIS_ long paramC) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define INscTree_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define INscTree_AddRef(T) (T)->lpVtbl->AddRef(T)
#define INscTree_Release(T) (T)->lpVtbl->Release(T)
#define INscTree_CreateTree(T,a,b,c) (T)->lpVtbl->CreateTree(T,a,b,c)
#define INscTree_Initialize(T,a,b,c) (T)->lpVtbl->Initialize(T,a,b,c)
#define INscTree_ShowWindow(T,a) (T)->lpVtbl->ShowWindow(T,a)
#define INscTree_Refresh(T) (T)->lpVtbl->Refresh(T)
#define INscTree_GetSelectedItem(T,a,b) (T)->lpVtbl->GetSelectedItem(T,a,b)
#define INscTree_SetSelectedItem(T,a,b,c,d) (T)->lpVtbl->SetSelectedItem(T,a,b,c,d)
#define INscTree_GetNscMode(T,a) (T)->lpVtbl->GetNscMode(T,a)
#define INscTree_SetNscMode(T,a) (T)->lpVtbl->SetNscMode(T,a)
#define INscTree_GetSelectedItemName(T,a,b) (T)->lpVtbl->GetSelectedItemName(T,a,b)
#define INscTree_BindToSelectedItemParent(T,a,b,c) (T)->lpVtbl->BindToSelectedItemParent(T,a,b,c)
#define INscTree_InLabelEdit(T) (T)->lpVtbl->InLabelEdit(T)
#define INscTree_RightPaneNavigationStarted(T,a) (T)->lpVtbl->RightPaneNavigationStarted(T,a)
#define INscTree_RightPaneNavigationFinished(T,a) (T)->lpVtbl->RightPaneNavigationFinished(T,a)
#endif

/*****************************************************************************
 * INscTree2 interface
 */
#define INTERFACE INscTree2
DECLARE_INTERFACE_(INscTree2, INscTree)
{
	 /*** IUnknown ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	/*** INscTree ***/
	STDMETHOD(CreateTree)(THIS_ long paramC, long param10, long param14) PURE;
	STDMETHOD(Initialize)(THIS_ long paramC, long param10, long param14) PURE;
	STDMETHOD(ShowWindow)(THIS_ long paramC) PURE;
	STDMETHOD(Refresh)(THIS) PURE;
	STDMETHOD(GetSelectedItem)(THIS_ long paramC, long param10) PURE;
	STDMETHOD(SetSelectedItem)(THIS_ long paramC, long param10, long param14, long param18) PURE;
	STDMETHOD(GetNscMode)(THIS_ long paramC) PURE;
	STDMETHOD(SetNscMode)(THIS_ long paramC) PURE;
	STDMETHOD(GetSelectedItemName)(THIS_ long paramC, long param10) PURE;
	STDMETHOD(BindToSelectedItemParent)(THIS_ long paramC, long param10, long param14) PURE;
	STDMETHOD(InLabelEdit)(THIS) PURE;
	STDMETHOD(RightPaneNavigationStarted)(THIS_ long paramC) PURE;
	STDMETHOD(RightPaneNavigationFinished)(THIS_ long paramC) PURE;
	/*** INscTree2 ***/
	STDMETHOD(CreateTree2)(THIS_ long paramC, long param10, long param14, long param18) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define INscTree2_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define INscTree2_AddRef(T) (T)->lpVtbl->AddRef(T)
#define INscTree2_Release(T) (T)->lpVtbl->Release(T)
#define INscTree2_CreateTree(T,a,b,c) (T)->lpVtbl->CreateTree(T,a,b,c)
#define INscTree2_Initialize(T,a,b,c) (T)->lpVtbl->Initialize(T,a,b,c)
#define INscTree2_ShowWindow(T,a) (T)->lpVtbl->ShowWindow(T,a)
#define INscTree2_Refresh(T) (T)->lpVtbl->Refresh(T)
#define INscTree2_GetSelectedItem(T,a,b) (T)->lpVtbl->GetSelectedItem(T,a,b)
#define INscTree2_SetSelectedItem(T,a,b,c,d) (T)->lpVtbl->SetSelectedItem(T,a,b,c,d)
#define INscTree2_GetNscMode(T,a) (T)->lpVtbl->GetNscMode(T,a)
#define INscTree2_SetNscMode(T,a) (T)->lpVtbl->SetNscMode(T,a)
#define INscTree2_GetSelectedItemName(T,a,b) (T)->lpVtbl->GetSelectedItemName(T,a,b)
#define INscTree2_BindToSelectedItemParent(T,a,b,c) (T)->lpVtbl->BindToSelectedItemParent(T,a,b,c)
#define INscTree2_InLabelEdit(T) (T)->lpVtbl->InLabelEdit(T)
#define INscTree2_RightPaneNavigationStarted(T,a) (T)->lpVtbl->RightPaneNavigationStarted(T,a)
#define INscTree2_RightPaneNavigationFinished(T,a) (T)->lpVtbl->RightPaneNavigationFinished(T,a)
#define INscTree2_CreateTree2(T,a,b,c,d) (T)->lpVtbl->CreateTree2(T,a,b,c,d)
#endif

/*****************************************************************************
 * IAddressEditBox interface
 */
#define INTERFACE IAddressEditBox
DECLARE_INTERFACE_(IAddressEditBox, IUnknown)
{
	 /*** IUnknown ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	 /*** IAddressEditBox ***/
	STDMETHOD(Init)(THIS_ HWND comboboxEx, HWND editControl, long param14, IUnknown *param18) PURE;
	STDMETHOD(SetCurrentDir)(THIS_ long paramC) PURE;
	STDMETHOD(ParseNow)(THIS_ long paramC) PURE;
	STDMETHOD(Execute)(THIS_ long paramC) PURE;
	STDMETHOD(Save)(THIS_ long paramC) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IAddressEditBox_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IAddressEditBox_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IAddressEditBox_Release(T) (T)->lpVtbl->Release(T)
#define IAddressEditBox_Init(T,a,b,c,d) (T)->lpVtbl->Init(T,a,b,c,d)
#define IAddressEditBox_SetCurrentDir(T,a) (T)->lpVtbl->SetCurrentDir(T,a)
#define IAddressEditBox_ParseNow(T,a) (T)->lpVtbl->ParseNow(T,a)
#define IAddressEditBox_Execute(T,a) (T)->lpVtbl->Execute(T,a)
#define IAddressEditBox_Save(T,a) (T)->lpVtbl->Save(T,a)
#endif

/*****************************************************************************
 * IBandProxy interface
 */
#define INTERFACE IBandProxy
DECLARE_INTERFACE_(IBandProxy, IUnknown)
{
	 /*** IUnknown ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	 /*** IBandProxy ***/
	STDMETHOD(SetSite)(THIS_ IUnknown *paramC) PURE;
	STDMETHOD(CreateNewWindow)(THIS_ long paramC) PURE;
	STDMETHOD(GetBrowserWindow)(THIS_ IUnknown **paramC) PURE;
	STDMETHOD(IsConnected)(THIS) PURE;
	STDMETHOD(NavigateToPIDL)(THIS_ LPCITEMIDLIST pidl) PURE;
	STDMETHOD(NavigateToURL)(THIS_ long paramC, long param10) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IBandProxy_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IBandProxy_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IBandProxy_Release(T) (T)->lpVtbl->Release(T)
#define IBandProxy_SetSite(T,a) (T)->lpVtbl->SetSite(T,a)
#define IBandProxy_CreateNewWindow(T,a) (T)->lpVtbl->CreateNewWindow(T,a)
#define IBandProxy_GetBrowserWindow(T,a) (T)->lpVtbl->GetBrowserWindow(T,a)
#define IBandProxy_IsConnected(T) (T)->lpVtbl->IsConnected(T)
#define IBandProxy_NavigateToPIDL(T,a) (T)->lpVtbl->NavigateToPIDL(T,a)
#define IBandProxy_NavigateToURL(T,a,b) (T)->lpVtbl->NavigateToURL(T,a,b)
#endif

/*****************************************************************************
 * IExplorerToolbar interface
 */
#define INTERFACE IExplorerToolbar
DECLARE_INTERFACE_(IExplorerToolbar, IUnknown)
{
	 /*** IUnknown ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	 /*** IExplorerToolbar ***/
	STDMETHOD(SetCommandTarget)(THIS_ IUnknown *theTarget, GUID *category, long param14) PURE;
	STDMETHOD(Unknown1)(THIS) PURE;
	STDMETHOD(AddButtons)(THIS_ const GUID *pguidCmdGroup, long buttonCount, TBBUTTON *buttons) PURE;
	STDMETHOD(AddString)(THIS_ const GUID *pguidCmdGroup, HINSTANCE param10, LPCTSTR param14, long *param18) PURE;
	STDMETHOD(GetButton)(THIS_ const GUID *paramC, long param10, long param14) PURE;
	STDMETHOD(GetState)(THIS_ const GUID *pguidCmdGroup, long commandID, long *theState) PURE;
	STDMETHOD(SetState)(THIS_ const GUID *pguidCmdGroup, long commandID, long theState) PURE;
	STDMETHOD(AddBitmap)(THIS_ const GUID *pguidCmdGroup, long param10, long buttonCount, TBADDBITMAP *lParam, long *newIndex, COLORREF param20) PURE;
	STDMETHOD(GetBitmapSize)(THIS_ long *paramC) PURE;
	STDMETHOD(SendToolbarMsg)(THIS_ const GUID *pguidCmdGroup, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *result) PURE;
	STDMETHOD(SetImageList)(THIS_ const GUID *pguidCmdGroup, HIMAGELIST param10, HIMAGELIST param14, HIMAGELIST param18) PURE;
	STDMETHOD(ModifyButton)(THIS_ const GUID *paramC, long param10, long param14) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IExplorerToolbar_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IExplorerToolbar_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IExplorerToolbar_Release(T) (T)->lpVtbl->Release(T)
#define IExplorerToolbar_SetCommandTarget(T,a,b,c) (T)->lpVtbl->SetCommandTarget(T,a,b,c)
#define IExplorerToolbar_Unknown1(T) (T)->lpVtbl->Unknown1(T)
#define IExplorerToolbar_AddButtons(T,a,b,c) (T)->lpVtbl->AddButtons(T,a,b,c)
#define IExplorerToolbar_AddString(T,a,b,c,d) (T)->lpVtbl->AddString(T,a,b,c,d)
#define IExplorerToolbar_GetButton(T,a,b,c) (T)->lpVtbl->GetButton(T,a,b,c)
#define IExplorerToolbar_GetState(T,a,b,c) (T)->lpVtbl->GetState(T,a,b,c)
#define IExplorerToolbar_SetState(T,a,b,c) (T)->lpVtbl->SetState(T,a,b,c)
#define IExplorerToolbar_AddBitmap(T,a,b,c,d,e,f) (T)->lpVtbl->AddBitmap(T,a,b,c,d,e,f)
#define IExplorerToolbar_GetBitmapSize(T,a) (T)->lpVtbl->GetBitmapSize(T,a)
#define IExplorerToolbar_SendToolbarMsg(T,a,b,c,d,e) (T)->lpVtbl->SendToolbarMsg(T,a,b,c,d,e)
#define IExplorerToolbar_SetImageList(T,a,b,c,d) (T)->lpVtbl->SetImageList(T,a,b,c,d)
#define IExplorerToolbar_ModifyButton(T,a,b,c) (T)->lpVtbl->ModifyButton(T,a,b,c)
#endif

/*****************************************************************************
 * IRegTreeOptions interface
 */
typedef enum tagWALK_TREE_CMD
{
	WALK_TREE_OPTION0 = 0,
	WALK_TREE_OPTION1 = 1,
	WALK_TREE_OPTION2 = 2,
	WALK_TREE_OPTION3 = 3
} WALK_TREE_CMD;

#define INTERFACE IRegTreeOptions
DECLARE_INTERFACE_(IRegTreeOptions, IUnknown)
{
	 /*** IUnknown ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	 /*** IRegTreeOptions ***/
	STDMETHOD(InitTree)(THIS_ HWND paramC, HKEY param10, char const *param14, char const *param18) PURE;
	STDMETHOD(WalkTree)(THIS_ WALK_TREE_CMD paramC) PURE;
	STDMETHOD(ToggleItem)(THIS_ HTREEITEM paramC) PURE;
	STDMETHOD(ShowHelp)(THIS_ HTREEITEM paramC, unsigned long param10) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IRegTreeOptions_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IRegTreeOptions_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IRegTreeOptions_Release(T) (T)->lpVtbl->Release(T)
#define IRegTreeOptions_InitTree(T,a,b,c,d) (T)->lpVtbl->InitTree(T,a,b,c,d)
#define IRegTreeOptions_WalkTree(T,a) (T)->lpVtbl->WalkTree(T,a)
#define IRegTreeOptions_ToggleItem(T,a) (T)->lpVtbl->ToggleItem(T,a)
#define IRegTreeOptions_ShowHelp(T,a,b) (T)->lpVtbl->ShowHelp(T,a,b)
#endif

/*****************************************************************************
 * IBandNavigate interface
 */
#define INTERFACE IBandNavigate
DECLARE_INTERFACE_(IBandNavigate, IUnknown)
{
	 /*** IUnknown ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	 /*** IBandNavigate ***/
	STDMETHOD(Select)(THIS_ LPCITEMIDLIST pidl) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IBandNavigate_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IBandNavigate_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IBandNavigate_Release(T) (T)->lpVtbl->Release(T)
#define IBandNavigate_Select(T,a) (T)->lpVtbl->Select(T,a)
#endif

/*****************************************************************************
 * INamespaceProxy interface
 */
#define INTERFACE INamespaceProxy
DECLARE_INTERFACE_(INamespaceProxy, IUnknown)
{
	 /*** IUnknown ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	 /*** INamespaceProxy ***/
	STDMETHOD(GetNavigateTarget)(THIS_ _In_ PCIDLIST_ABSOLUTE pidl, _Out_ PIDLIST_ABSOLUTE *ppidlTarget, _Out_ ULONG *pulAttrib) PURE;
	STDMETHOD(Invoke)(THIS_ _In_ PCIDLIST_ABSOLUTE pidl) PURE;
	STDMETHOD(OnSelectionChanged)(THIS_ _In_ PCIDLIST_ABSOLUTE pidl) PURE;
	STDMETHOD(RefreshFlags)(THIS_ _Out_ DWORD *pdwStyle, _Out_ DWORD *pdwExStyle, _Out_ DWORD *dwEnum) PURE;
	STDMETHOD(CacheItem)(THIS_ _In_ PCIDLIST_ABSOLUTE pidl) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define INamespaceProxy_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define INamespaceProxy_AddRef(T) (T)->lpVtbl->AddRef(T)
#define INamespaceProxy_Release(T) (T)->lpVtbl->Release(T)
#define INamespaceProxy_GetNavigateTarget(T,a,b,c) (T)->lpVtbl->GetNavigateTarget(T,a,b,c)
#define INamespaceProxy_Invoke(T,a) (T)->lpVtbl->Invoke(T,a)
#define INamespaceProxy_OnSelectionChanged(T,a) (T)->lpVtbl->OnSelectionChanged(T,a)
#define INamespaceProxy_RefreshFlags(T,a,b,c) (T)->lpVtbl->RefreshFlags(T,a,b,c)
#define INamespaceProxy_CacheItem(T,a) (T)->lpVtbl->CacheItem(T,a)
#endif

/*****************************************************************************
 * IShellMenu2 interface
 */
#define INTERFACE IShellMenu2
DECLARE_INTERFACE_(IShellMenu2, IShellMenu)
{
	 /*** IUnknown ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	 /*** IShellMenu ***/
	STDMETHOD(Initialize)(THIS_ IShellMenuCallback *psmc, UINT uId, UINT uIdAncestor, DWORD dwFlags) PURE;
	STDMETHOD(GetMenuInfo)(THIS_ IShellMenuCallback **ppsmc, UINT *puId, UINT *puIdAncestor, DWORD *pdwFlags) PURE;
	STDMETHOD(SetShellFolder)(THIS_ IShellFolder *psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags) PURE;
	STDMETHOD(GetShellFolder)(THIS_ DWORD *pdwFlags, LPITEMIDLIST *ppidl, REFIID riid, void **ppv) PURE;
	STDMETHOD(SetMenu)(THIS_ HMENU hmenu, HWND hwnd, DWORD dwFlags) PURE;
	STDMETHOD(GetMenu)(THIS_ HMENU *phmenu, HWND *phwnd, DWORD *pdwFlags) PURE;
	STDMETHOD(InvalidateItem)(THIS_ LPSMDATA psmd, DWORD dwFlags) PURE;
	STDMETHOD(GetState)(THIS_ LPSMDATA psmd) PURE;
	STDMETHOD(SetMenuToolbar)(THIS_ IUnknown *punk, DWORD dwFlags) PURE;
	 /*** IShellMenu2 ***/
	STDMETHOD(GetSubMenu)(THIS) PURE;
	STDMETHOD(SetToolbar)(THIS) PURE;
	STDMETHOD(SetMinWidth)(THIS) PURE;
	STDMETHOD(SetNoBorder)(THIS) PURE;
	STDMETHOD(SetTheme)(THIS) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IShellMenu2_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IShellMenu2_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IShellMenu2_Release(T) (T)->lpVtbl->Release(T)
#define IShellMenu2_Initialize(T,a,b,c,d) (T)->lpVtbl->Initialize(T,a,b,c,d)
#define IShellMenu2_GetMenuInfo(T,a,b,c,d) (T)->lpVtbl->GetMenuInfo(T,a,b,c,d)
#define IShellMenu2_SetShellFolder(T,a,b,c,d) (T)->lpVtbl->SetShellFolder(T,a,b,c,d)
#define IShellMenu2_GetShellFolder(T,a,b,c,d) (T)->lpVtbl->GetShellFolder(T,a,b,c,d)
#define IShellMenu2_SetMenu(T,a,b,c) (T)->lpVtbl->SetMenu(T,a,b,c)
#define IShellMenu2_GetMenu(T,a,b,c) (T)->lpVtbl->GetMenu(T,a,b,c)
#define IShellMenu2_InvalidateItem(T,a,b) (T)->lpVtbl->InvalidateItem(T,a,b)
#define IShellMenu2_GetState(T,a) (T)->lpVtbl->GetState(T,a)
#define IShellMenu2_SetMenuToolbar(T,a,b) (T)->lpVtbl->SetMenuToolbar(T,a,b)
#define IShellMenu2_GetSubMenu(T) (T)->lpVtbl->GetSubMenu(T)
#define IShellMenu2_SetToolbar(T) (T)->lpVtbl->SetToolbar(T)
#define IShellMenu2_SetMinWidth(T) (T)->lpVtbl->SetMinWidth(T)
#define IShellMenu2_SetNoBorder(T) (T)->lpVtbl->SetNoBorder(T)
#define IShellMenu2_SetTheme(T) (T)->lpVtbl->SetTheme(T)
#endif

/*****************************************************************************
 * IWinEventHandler interface
 */
#define INTERFACE IWinEventHandler
DECLARE_INTERFACE_(IWinEventHandler, IUnknown)
{
	 /*** IUnknown ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	 /*** IWinEventHandler ***/
	STDMETHOD(OnWinEvent)(THIS_ HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult) PURE;
	STDMETHOD(IsWindowOwner)(THIS_ HWND hWnd) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IWinEventHandler_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IWinEventHandler_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IWinEventHandler_Release(T) (T)->lpVtbl->Release(T)
#define IWinEventHandler_OnWinEvent(T,a,b,c,d,e) (T)->lpVtbl->OnWinEvent(T,a,b,c,d,e)
#define IWinEventHandler_IsWindowOwner(T,a) (T)->lpVtbl->IsWindowOwner(T,a)
#endif

/*****************************************************************************
 * IAddressBand interface
 */
#define INTERFACE IAddressBand
DECLARE_INTERFACE_(IAddressBand, IUnknown)
{
	 /*** IUnknown ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	 /*** IAddressBand ***/
	STDMETHOD(FileSysChange)(THIS_ long param8, long paramC) PURE;
	STDMETHOD(Refresh)(THIS_ long param8) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IAddressBand_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IAddressBand_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IAddressBand_Release(T) (T)->lpVtbl->Release(T)
#define IAddressBand_FileSysChange(T,a,b) (T)->lpVtbl->FileSysChange(T,a,b)
#define IAddressBand_Refresh(T,a) (T)->lpVtbl->Refresh(T,a)
#endif

/*****************************************************************************
 * IShellMenuAcc interface
 */
#define INTERFACE IShellMenuAcc
DECLARE_INTERFACE_(IShellMenuAcc, IUnknown)
{
	 /*** IUnknown ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	 /*** IShellMenuAcc ***/
	STDMETHOD(GetTop)(THIS) PURE;
	STDMETHOD(GetBottom)(THIS) PURE;
	STDMETHOD(GetTracked)(THIS) PURE;
	STDMETHOD(GetParentSite)(THIS) PURE;
	STDMETHOD(GetState)(THIS) PURE;
	STDMETHOD(DoDefaultAction)(THIS) PURE;
	STDMETHOD(GetSubMenu)(THIS) PURE;
	STDMETHOD(IsEmpty)(THIS) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IShellMenuAcc_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IShellMenuAcc_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IShellMenuAcc_Release(T) (T)->lpVtbl->Release(T)
#define IShellMenuAcc_GetTop(T) (T)->lpVtbl->GetTop(T)
#define IShellMenuAcc_GetBottom(T) (T)->lpVtbl->GetBottom(T)
#define IShellMenuAcc_GetTracked(T) (T)->lpVtbl->GetTracked(T)
#define IShellMenuAcc_GetParentSite(T) (T)->lpVtbl->GetParentSite(T)
#define IShellMenuAcc_GetState(T) (T)->lpVtbl->GetState(T)
#define IShellMenuAcc_DoDefaultAction(T) (T)->lpVtbl->DoDefaultAction(T)
#define IShellMenuAcc_GetSubMenu(T) (T)->lpVtbl->GetSubMenu(T)
#define IShellMenuAcc_IsEmpty(T) (T)->lpVtbl->IsEmpty(T)
#endif

/*****************************************************************************
 * IAddressBand interface
 */
#define INTERFACE IBandSiteHelper
DECLARE_INTERFACE_(IBandSiteHelper, IUnknown)
{
	 /*** IUnknown ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	 /*** IBandSiteHelper ***/
	STDMETHOD(LoadFromStreamBS)(THIS_ IStream *, REFGUID, void **) PURE;
	STDMETHOD(SaveToStreamBS)(THIS_ IUnknown *, IStream *) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IBandSiteHelper_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IBandSiteHelper_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IBandSiteHelper_Release(T) (T)->lpVtbl->Release(T)
#define IBandSiteHelper_LoadFromStreamBS(T,a,b) (T)->lpVtbl->LoadFromStreamBS(T,a,b)
#define IBandSiteHelper_SaveToStreamBS(T,a,b) (T)->lpVtbl->SaveToStreamBS(T,a,b)
#endif

/*****************************************************************************
 * IAddressBand interface
 */
#define INTERFACE IShellBrowserService
DECLARE_INTERFACE_(IShellBrowserService, IUnknown)
{
	 /*** IUnknown ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	 /*** IShellBrowserService ***/
	STDMETHOD(GetPropertyBag)(THIS_ long flags, REFIID riid, void **ppvObject) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IShellBrowserService_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IShellBrowserService_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IShellBrowserService_Release(T) (T)->lpVtbl->Release(T)
#define IShellBrowserService_GetPropertyBag(T,a,b,c) (T)->lpVtbl->GetPropertyBag(T,a,b,c)
#endif

/*****************************************************************************
 * IMruDataList interface
 */
#define INTERFACE IMruDataList
DECLARE_INTERFACE_(IMruDataList, IUnknown)
{
    /*** IUnknown ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IMruDataList ***/
    STDMETHOD(InitData)(THIS_ UINT, UINT, HKEY, LPCWSTR, SLOTCOMPARE) PURE;
    STDMETHOD(AddData)(THIS_ LPCVOID , DWORD, UINT*) PURE;
    STDMETHOD(FindData)(THIS_ LPCVOID , DWORD, UINT*) PURE;
    STDMETHOD(GetData)(THIS_ UINT, LPVOID, DWORD) PURE;
    STDMETHOD(QueryInfo)(THIS_ UINT, UINT*, DWORD*) PURE;
    STDMETHOD(Delete)(THIS_ UINT) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IMruDataList_QueryInterface(T,a,b)  (T)->lpVtbl->QueryInterface(T,a,b)
#define IMruDataList_AddRef(T)              (T)->lpVtbl->AddRef(T)
#define IMruDataList_Release(T)             (T)->lpVtbl->Release(T)
#define IMruDataList_InitData(T,a,b,c,d,e)  (T)->lpVtbl->InitData(T,a,b,c,d,e)
#define IMruDataList_AddData(T,a,b,c)       (T)->lpVtbl->AddData(T,a,b,c)
#define IMruDataList_FindData(T,a,b,c)      (T)->lpVtbl->FindData(T,a,b,c)
#define IMruDataList_GetData(T,a,b,c)       (T)->lpVtbl->GetData(T,a,b,c)
#define IMruDataList_QueryInfo(T,a,b,c)     (T)->lpVtbl->QueryInfo(T,a,b,c)
#define IMruDataList_Delete(T,a)            (T)->lpVtbl->Delete(T,a)
#endif

/*****************************************************************************
 * IMruPidlList interface
 */
#define INTERFACE IMruPidlList
DECLARE_INTERFACE_(IMruPidlList, IUnknown)
{
    /*** IUnknown ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IMruPidlList ***/
    STDMETHOD(InitList)(THIS_ UINT, HKEY, LPCWSTR) PURE;
    STDMETHOD(UsePidl)(THIS_ LPCITEMIDLIST, UINT*) PURE;
    STDMETHOD(QueryPidl)(THIS_ LPCITEMIDLIST, UINT, UINT*, UINT*) PURE;
    STDMETHOD(PruneKids)(THIS_ LPCITEMIDLIST) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IMruPidlList_QueryInterface(T,a,b)  (T)->lpVtbl->QueryInterface(T,a,b)
#define IMruPidlList_AddRef(T)              (T)->lpVtbl->AddRef(T)
#define IMruPidlList_Release(T)             (T)->lpVtbl->Release(T)
#define IMruPidlList_InitList(T,a,b,c)      (T)->lpVtbl->InitList(T,a,b,c)
#define IMruPidlList_UsePidl(T,a,b)         (T)->lpVtbl->UsePidl(T,a,b)
#define IMruPidlList_QueryPidl(T,a,b,c,d)   (T)->lpVtbl->QueryPidl(T,a,b,c,d)
#define IMruPidlList_PruneKids(T,a)         (T)->lpVtbl->PruneKids(T,a)
#endif

/*****************************************************************************
 * ITrayPriv interface
 */
#define INTERFACE ITrayPriv
DECLARE_INTERFACE_(ITrayPriv, IUnknown)
{
    /*** IUnknown ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IOleWindow methods ***/
    STDMETHOD_(HRESULT,GetWindow)(THIS_ HWND*) PURE;
    STDMETHOD_(HRESULT,ContextSensitiveHelp)(THIS_ BOOL) PURE;
    /*** ITrayPriv ***/
    STDMETHOD_(HRESULT,Execute)(THIS_ IShellFolder*,LPCITEMIDLIST) PURE;
    STDMETHOD_(HRESULT,Unknown)(THIS_ PVOID,PVOID,PVOID,PVOID) PURE;
    STDMETHOD_(HRESULT,AppendMenu)(THIS_ HMENU*) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define ITrayPriv_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define ITrayPriv_AddRef(T) (T)->lpVtbl->AddRef(T)
#define ITrayPriv_Release(T) (T)->lpVtbl->Release(T)
#define ITrayPriv_GetWindow(T,a) (T)->lpVtbl->GetWindow(T,a)
#define ITrayPriv_ContextSensitiveHelp(T,a) (T)->lpVtbl->ContextSensitiveHelp(T,a)
#define ITrayPriv_Execute(T,a,b) (T)->lpVtbl->Execute(T,a,b)
#define ITrayPriv_Unknown(T,a,b,c,d) (T)->lpVtbl->Unknown(T,a,b,c,d)
#define ITrayPriv_AppendMenu(T,a) (T)->lpVtbl->AppendMenu(T,a)
#endif

/*****************************************************************************
 * IAssociationElement interface
 *
 * @see IAssociationElementOld
 * @see https://www.geoffchappell.com/studies/windows/shell/shlwapi/interfaces/iassociationelement.htm
 */
#define INTERFACE IAssociationElement
DECLARE_INTERFACE_(IAssociationElement, IUnknown) // {D8F6AD5B-B44F-4BCC-88FD-EB3473DB7502}
{
    /*** IUnknown ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IAssociationElement ***/
    STDMETHOD(QueryString)(THIS_ ASSOCQUERY query, PCWSTR key, PWSTR *ppszValue) PURE;
    STDMETHOD(QueryDword)(THIS_ ASSOCQUERY query, PCWSTR key, DWORD *pdwValue) PURE;
    STDMETHOD(QueryGuid)(THIS_ ASSOCQUERY query, PCWSTR key, GUID *pguid) PURE;
    STDMETHOD(QueryExists)(THIS_ ASSOCQUERY query, PCWSTR key) PURE;
    STDMETHOD(QueryDirect)(THIS_ ASSOCQUERY query, PCWSTR key, FLAGGED_BYTE_BLOB **ppBlob) PURE;
    STDMETHOD(QueryObject)(THIS_ ASSOCQUERY query, PCWSTR key, REFIID riid, PVOID *ppvObj) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IAssociationElement_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IAssociationElement_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IAssociationElement_Release(T) (T)->lpVtbl->Release(T)
#define IAssociationElement_QueryString(T,a,b,c) (T)->lpVtbl->QueryString(T,a,b,c)
#define IAssociationElement_QueryDword(T,a,b,c) (T)->lpVtbl->QueryDword(T,a,b,c)
#define IAssociationElement_QueryGuid(T,a,b,c) (T)->lpVtbl->QueryGuid(T,a,b,c)
#define IAssociationElement_QueryExists(T,a,b) (T)->lpVtbl->QueryExists(T,a,b)
#define IAssociationElement_QueryDirect(T,a,b,c) (T)->lpVtbl->QueryDirect(T,a,b,c)
#define IAssociationElement_QueryObject(T,a,b,c,d) (T)->lpVtbl->QueryObject(T,a,b,c,d)
#endif

/*****************************************************************************
 * IEnumAssociationElements interface
 *
 * @see https://www.geoffchappell.com/studies/windows/shell/shell32/interfaces/ienumassociationelements.htm
 */
#define INTERFACE IEnumAssociationElements
DECLARE_INTERFACE_(IEnumAssociationElements, IUnknown) // {A6B0FB57-7523-4439-9425-EBE99823B828}
{
    /*** IUnknown ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IEnumAssociationElements ***/
    STDMETHOD(Next)(THIS_ ULONG celt, IAssociationElement *pElement, ULONG *pceltFetched) PURE;
    STDMETHOD(Skip)(THIS_ ULONG celt) PURE;
    STDMETHOD(Reset)(THIS) PURE;
    STDMETHOD(Clone)(THIS_ IEnumAssociationElements **ppNew) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IEnumAssociationElements_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IEnumAssociationElements_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IEnumAssociationElements_Release(T) (T)->lpVtbl->Release(T)
#define IEnumAssociationElements_Next(T,a,b,c) (T)->lpVtbl->Next(T,a,b,c)
#define IEnumAssociationElements_Skip(T,a) (T)->lpVtbl->Skip(T,a)
#define IEnumAssociationElements_Reset(T) (T)->lpVtbl->Reset(T)
#define IEnumAssociationElements_Clone(T,a) (T)->lpVtbl->Clone(T,a)
#endif

/*****************************************************************************
 * IAssociationArrayOld interface
 *
 * @see IAssociationArray
 * @see https://www.geoffchappell.com/studies/windows/shell/shell32/interfaces/iassociationarray.htm
 */
#define INTERFACE IAssociationArrayOld
DECLARE_INTERFACE_(IAssociationArrayOld, IUnknown) // {3B877E3C-67DE-4F9A-B29B-17D0A1521C6A}
{
    /*** IUnknown ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IAssociationArrayOld ***/
    STDMETHOD(EnumElements)(THIS_ ULONG flags, IEnumAssociationElements **ppElement) PURE;
    STDMETHOD(QueryString)(THIS_ ULONG flags, ASSOCQUERY query, PCWSTR key, PWSTR *ppszValue) PURE;
    STDMETHOD(QueryDword)(THIS_ ULONG flags, ASSOCQUERY query, PCWSTR key, DWORD *pdwValue) PURE;
    STDMETHOD(QueryExists)(THIS_ ULONG flags, ASSOCQUERY query, PCWSTR key) PURE;
    STDMETHOD(QueryDirect)(THIS_ ULONG flags, ASSOCQUERY query, PCWSTR key, FLAGGED_BYTE_BLOB **ppBlob) PURE;
    STDMETHOD(QueryObject)(THIS_ ULONG flags, ASSOCQUERY query, PCWSTR key, REFIID riid, PVOID *ppvObj) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IAssociationArrayOld_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IAssociationArrayOld_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IAssociationArrayOld_Release(T) (T)->lpVtbl->Release(T)
#define IAssociationArrayOld_EnumElements(T,a,b) (T)->lpVtbl->EnumElements(T,a,b)
#define IAssociationArrayOld_QueryString(T,a,b,c,d) (T)->lpVtbl->QueryString(T,a,b,c,d)
#define IAssociationArrayOld_QueryDword(T,a,b,c,d) (T)->lpVtbl->QueryDword(T,a,b,c,d)
#define IAssociationArrayOld_QueryExists(T,a,b,c) (T)->lpVtbl->QueryExists(T,a,b,c)
#define IAssociationArrayOld_QueryDirect(T,a,b,c,d) (T)->lpVtbl->QueryDirect(T,a,b,c,d)
#define IAssociationArrayOld_QueryObject(T,a,b,c,d,e) (T)->lpVtbl->QueryObject(T,a,b,c,d,e)
#endif

/*****************************************************************************
 * IAssociationArray interface
 *
 * @see IAssociationArrayOld
 * @see https://www.geoffchappell.com/studies/windows/shell/shell32/interfaces/iassociationarray.htm
 */
#define INTERFACE IAssociationArray
DECLARE_INTERFACE_(IAssociationArray, IUnknown) // {19ADBAFD-1C5F-4FC7-94EE-846702DFB58B}
{
    /*** IUnknown ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IAssociationArray ***/
    STDMETHOD(QueryString)(THIS_ ASSOCQUERY query, PCWSTR key, PWSTR *ppszValue) PURE;
    STDMETHOD(QueryDword)(THIS_ ASSOCQUERY query, PCWSTR key, DWORD *pdwValue) PURE;
    STDMETHOD(QueryGuid)(THIS_ ASSOCQUERY query, PCWSTR key, GUID *pguid) PURE;
    STDMETHOD(QueryExists)(THIS_ ASSOCQUERY query, PCWSTR key) PURE;
    STDMETHOD(QueryDirect)(THIS_ ASSOCQUERY query, PCWSTR key, FLAGGED_BYTE_BLOB **ppBlob) PURE;
    STDMETHOD(QueryObject)(THIS_ ASSOCQUERY query, PCWSTR key, REFIID riid, PVOID *ppvObj) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IAssociationArray_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IAssociationArray_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IAssociationArray_Release(T) (T)->lpVtbl->Release(T)
#define IAssociationArray_QueryString(T,a,b,c) (T)->lpVtbl->QueryString(T,a,b,c)
#define IAssociationArray_QueryDword(T,a,b,c) (T)->lpVtbl->QueryDword(T,a,b,c)
#define IAssociationArray_QueryGuid(T,a,b,c) (T)->lpVtbl->QueryGuid(T,a,b,c)
#define IAssociationArray_QueryExists(T,a,b) (T)->lpVtbl->QueryExists(T,a,b)
#define IAssociationArray_QueryDirect(T,a,b,c) (T)->lpVtbl->QueryDirect(T,a,b,c)
#define IAssociationArray_QueryObject(T,a,b,c,d) (T)->lpVtbl->QueryObject(T,a,b,c,d)
#endif

HANDLE WINAPI SHCreateDesktop(IShellDesktopTray*);
BOOL WINAPI SHDesktopMessageLoop(HANDLE);
HRESULT WINAPI SHCreateFileDataObject(PCIDLIST_ABSOLUTE pidlFolder, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, IDataObject* pDataInner, IDataObject** ppDataObj);

#ifdef __cplusplus
} /* extern "C" */
#endif
