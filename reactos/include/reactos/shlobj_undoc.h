/*
 * ReactOS undocumented shell interface
 *
 * Copyright 2009 Andrew Hill <ash77 at domain reactos.org>
 * Copyright 2013 Dominik Hornung
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __SHLOBJ_UNDOC__H
#define __SHLOBJ_UNDOC__H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#ifdef __cplusplus
#   define IID_PPV_ARG(Itype, ppType) IID_##Itype, reinterpret_cast<void**>((static_cast<Itype**>(ppType)))
#   define IID_NULL_PPV_ARG(Itype, ppType) IID_##Itype, NULL, reinterpret_cast<void**>((static_cast<Itype**>(ppType)))
#else
#   define IID_PPV_ARG(Itype, ppType) IID_##Itype, (void**)(ppType)
#   define IID_NULL_PPV_ARG(Itype, ppType) IID_##Itype, NULL, (void**)(ppType)
#endif

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
	long					offset0;
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
	STDMETHOD(Get)(THIS_ struct DEFFOLDERSETTINGS *buffer, int theSize) PURE;
	STDMETHOD(Set)(THIS_ const struct DEFFOLDERSETTINGS *buffer, int theSize, unsigned int param14) PURE;
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
	STDMETHOD(Select)(THIS_ long paramC) PURE;
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
	STDMETHOD(GetNavigateTarget)(THIS_ long paramC, long param10, long param14) PURE;
	STDMETHOD(Invoke)(THIS_ long paramC) PURE;
	STDMETHOD(OnSelectionChanged)(THIS_ long paramC) PURE;
	STDMETHOD(RefreshFlags)(THIS_ long paramC, long param10, long param14) PURE;
	STDMETHOD(CacheItem)(THIS_ long paramC) PURE;
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
 * Shell32 resources
 */
// these resources are in shell32.dll
#define IDB_GOBUTTON_NORMAL			0x0e6
#define IDB_GOBUTTON_HOT			0x0e7

// band ids in internet toolbar
#define ITBBID_MENUBAND				1
#define ITBBID_BRANDBAND			5
#define ITBBID_TOOLSBAND			2
#define ITBBID_ADDRESSBAND			4

// commands in the CGID_PrivCITCommands command group handled by the internet toolbar
// there seems to be some support for hiding the menubar and an auto hide feature that are
// unavailable in the UI
#define ITID_TEXTLABELS				3
#define ITID_TOOLBARBANDSHOWN		4
#define ITID_ADDRESSBANDSHOWN		5
#define ITID_LINKSBANDSHOWN			6
#define ITID_MENUBANDSHOWN			12
#define ITID_AUTOHIDEENABLED		13
#define ITID_CUSTOMIZEENABLED		20
#define ITID_TOOLBARLOCKED			27

// commands in the CGID_BrandCmdGroup command group handled by the brand band
#define BBID_STARTANIMATION			1
#define BBID_STOPANIMATION			2

// undocumented flags for IShellMenu::SetShellFolder
#define SMSET_UNKNOWN08				0x08
#define SMSET_UNKNOWN10				0x10

BOOL WINAPI ILGetDisplayNameEx(IShellFolder *psf, LPCITEMIDLIST pidl, LPVOID path, DWORD type);

/* type parameter for ILGetDisplayNameEx() */
#define ILGDN_FORPARSING  0
#define ILGDN_NORMAL      1
#define ILGDN_INFOLDER    2

BOOL WINAPI FileIconInit(BOOL bFullInit);
void WINAPI ShellDDEInit(BOOL bInit);
DWORD WINAPI WinList_Init(void);

HANDLE WINAPI SHCreateDesktop(IShellDesktopTray*);
BOOL WINAPI SHDesktopMessageLoop(HANDLE);

#define WM_GETISHELLBROWSER (WM_USER+7)
BOOL WINAPI SetShellWindow(HWND);
BOOL WINAPI SetShellWindowEx(HWND, HWND);
BOOL WINAPI RegisterShellHook(HWND, DWORD);
IStream* WINAPI SHGetViewStream(LPCITEMIDLIST, DWORD, LPCTSTR, LPCTSTR, LPCTSTR);
BOOL WINAPI SHIsEmptyStream(IStream*);

typedef struct tagCREATEMRULISTA
{
    DWORD cbSize;
    DWORD nMaxItems;
    DWORD dwFlags;
    HKEY hKey;
    LPCSTR lpszSubKey;
    PROC lpfnCompare;
} CREATEMRULISTA, *LPCREATEMRULISTA;
typedef struct tagCREATEMRULISTW
{
    DWORD cbSize;
    DWORD nMaxItems;
    DWORD dwFlags;
    HKEY hKey;
    LPCWSTR lpszSubKey;
    PROC lpfnCompare;
} CREATEMRULISTW, *LPCREATEMRULISTW;

#define MRU_STRING  0x0
#define MRU_BINARY  0x1
#define MRU_CACHEWRITE  0x2

HANDLE WINAPI CreateMRUListW(LPCREATEMRULISTW);
HANDLE WINAPI CreateMRUListA(LPCREATEMRULISTA);
INT WINAPI AddMRUData(HANDLE,LPCVOID,DWORD);
INT WINAPI FindMRUData(HANDLE,LPCVOID,DWORD,LPINT);
VOID WINAPI FreeMRUList(HANDLE);

INT WINAPI AddMRUStringW(HANDLE hList, LPCWSTR lpszString);
INT WINAPI AddMRUStringA(HANDLE hList, LPCSTR lpszString);
BOOL WINAPI DelMRUString(HANDLE hList, INT nItemPos);
INT WINAPI FindMRUStringW(HANDLE hList, LPCWSTR lpszString, LPINT lpRegNum);
INT WINAPI FindMRUStringA(HANDLE hList, LPCSTR lpszString, LPINT lpRegNum);
HANDLE WINAPI CreateMRUListLazyW(const CREATEMRULISTW *lpcml, DWORD dwParam2,
                                  DWORD dwParam3, DWORD dwParam4);
HANDLE WINAPI CreateMRUListLazyA(const CREATEMRULISTA *lpcml, DWORD dwParam2,
                                  DWORD dwParam3, DWORD dwParam4);
INT WINAPI EnumMRUListW(HANDLE hList, INT nItemPos, LPVOID lpBuffer,
                         DWORD nBufferSize);
INT WINAPI EnumMRUListA(HANDLE hList, INT nItemPos, LPVOID lpBuffer,
                         DWORD nBufferSize);

#define DC_NOSENDMSG 0x2000
BOOL WINAPI DrawCaptionTempA(HWND,HDC,const RECT*,HFONT,HICON,LPCSTR,UINT);
BOOL WINAPI DrawCaptionTempW(HWND,HDC,const RECT*,HFONT,HICON,LPCWSTR,UINT);

#ifdef UNICODE
typedef CREATEMRULISTW CREATEMRULIST, *PCREATEMRULIST;
#define CreateMRUList   CreateMRUListW
#define DrawCaptionTemp DrawCaptionTempW
#else
typedef CREATEMRULISTA CREATEMRULIST, *PCREATEMRULIST;
#define CreateMRUList   CreateMRUListA
#define DrawCaptionTemp DrawCaptionTempA
#endif

HRESULT WINAPI SHInvokeDefaultCommand(HWND,IShellFolder*,LPCITEMIDLIST);

HRESULT WINAPI SHPropertyBag_ReadPOINTL(IPropertyBag*,LPCWSTR,POINTL*);

HRESULT WINAPI SHGetPerScreenResName(OUT LPWSTR lpResName,
                                     IN INT cchResName,
                                     IN DWORD dwReserved);

HRESULT WINAPI SHPropertyBag_ReadStream(IPropertyBag*,LPCWSTR,IStream**);

HWND WINAPI SHCreateWorkerWindowA(LONG wndProc, HWND hWndParent, DWORD dwExStyle,
                        DWORD dwStyle, HMENU hMenu, LONG z);

HWND WINAPI SHCreateWorkerWindowW(LONG wndProc, HWND hWndParent, DWORD dwExStyle,
                        DWORD dwStyle, HMENU hMenu, LONG z);
#ifdef UNICODE
#define SHCreateWorkerWindow SHCreateWorkerWindowW
#else
#define SHCreateWorkerWindow SHCreateWorkerWindowA
#endif

/*****************************************************************************
 * Shell Link
 */
#include <pshpack1.h>

typedef struct tagSHELL_LINK_HEADER
{
    /* The size of this structure (always 0x0000004C) */
    DWORD dwSize;
    /* CLSID = class identifier (always 00021401-0000-0000-C000-000000000046) */
    CLSID clsid;
    /* Flags (SHELL_LINK_DATA_FLAGS) */
    DWORD dwFlags;
    /* Informations about the link target: */
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeLow; /* only the least significant 32 bits */
    /* The index of an icon (signed?) */
    DWORD nIconIndex;
    /* The expected window state of an application launched by the link */
    DWORD nShowCommand;
    /* The keystrokes used to launch the application */
    WORD wHotKey;
    /* Reserved (must be zero) */
    WORD wReserved1;
    DWORD dwReserved2;
    DWORD dwReserved3;
} SHELL_LINK_HEADER, *LPSHELL_LINK_HEADER;

/*****************************************************************************
 * SHELL_LINK_INFOA/W
 * If cbHeaderSize == 0x0000001C then use SHELL_LINK_INFOA
 * If cbHeaderSize >= 0x00000024 then use SHELL_LINK_INFOW
 */
typedef struct tagSHELL_LINK_INFOA
{
    /* Size of the link info data */
    DWORD cbSize;
    /* Size of this structure (ANSI: = 0x0000001C) */
    DWORD cbHeaderSize;
    /* Specifies which fields are present/populated (SLI_*) */
    DWORD dwFlags;
    /* Offset of the VolumeID field (SHELL_LINK_INFO_VOLUME_ID) */
    DWORD cbVolumeIDOffset;
    /* Offset of the LocalBasePath field (ANSI, NULL-terminated string) */
    DWORD cbLocalBasePathOffset;
    /* Offset of the CommonNetworkRelativeLink field (SHELL_LINK_INFO_CNR_LINK) */
    DWORD cbCommonNetworkRelativeLinkOffset;
    /* Offset of the CommonPathSuffix field (ANSI, NULL-terminated string) */
    DWORD cbCommonPathSuffixOffset;
} SHELL_LINK_INFOA, *LPSHELL_LINK_INFOA;

typedef struct tagSHELL_LINK_INFOW
{
    /* Size of the link info data */
    DWORD cbSize;
    /* Size of this structure (Unicode: >= 0x00000024) */
    DWORD cbHeaderSize;
    /* Specifies which fields are present/populated (SLI_*) */
    DWORD dwFlags;
    /* Offset of the VolumeID field (SHELL_LINK_INFO_VOLUME_ID) */
    DWORD cbVolumeIDOffset;
    /* Offset of the LocalBasePath field (ANSI, NULL-terminated string) */
    DWORD cbLocalBasePathOffset;
    /* Offset of the CommonNetworkRelativeLink field (SHELL_LINK_INFO_CNR_LINK) */
    DWORD cbCommonNetworkRelativeLinkOffset;
    /* Offset of the CommonPathSuffix field (ANSI, NULL-terminated string) */
    DWORD cbCommonPathSuffixOffset;
    /* Offset of the LocalBasePathUnicode field (Unicode, NULL-terminated string) */
    DWORD cbLocalBasePathUnicodeOffset;
    /* Offset of the CommonPathSuffixUnicode field (Unicode, NULL-terminated string) */
    DWORD cbCommonPathSuffixUnicodeOffset;
} SHELL_LINK_INFOW, *LPSHELL_LINK_INFOW;

/* VolumeID, LocalBasePath, LocalBasePathUnicode(cbHeaderSize >= 0x24) are present */
#define SLI_VALID_LOCAL   0x00000001
/* CommonNetworkRelativeLink is present */
#define SLI_VALID_NETWORK 0x00000002

/*****************************************************************************
 * SHELL_LINK_INFO_VOLUME_IDA/W
 * If cbVolumeLabelOffset != 0x00000014 (should be 0x00000010) then use 
 * SHELL_LINK_INFO_VOLUME_IDA
 * If cbVolumeLabelOffset == 0x00000014 then use SHELL_LINK_INFO_VOLUME_IDW
 */
typedef struct tagSHELL_LINK_INFO_VOLUME_IDA
{
    /* Size of the VolumeID field (> 0x00000010) */
    DWORD cbSize;
    /* Drive type of the drive the link target is stored on (DRIVE_*) */
    DWORD dwDriveType;
    /* Serial number of the volume the link target is stored on */
    DWORD nDriveSerialNumber;
    /* Offset of the volume label (ANSI, NULL-terminated string).
       Must be != 0x00000014 (see tagSHELL_LINK_INFO_VOLUME_IDW) */
    DWORD cbVolumeLabelOffset;
} SHELL_LINK_INFO_VOLUME_IDA, *LPSHELL_LINK_INFO_VOLUME_IDA;

typedef struct tagSHELL_LINK_INFO_VOLUME_IDW
{
    /* Size of the VolumeID field (> 0x00000010) */
    DWORD cbSize;
    /* Drive type of the drive the link target is stored on (DRIVE_*) */
    DWORD dwDriveType;
    /* Serial number of the volume the link target is stored on */
    DWORD nDriveSerialNumber;
    /* Offset of the volume label (ANSI, NULL-terminated string).
       If the value of this field is 0x00000014, ignore it and use
       cbVolumeLabelUnicodeOffset! */
    DWORD cbVolumeLabelOffset;
    /* Offset of the volume label (Unicode, NULL-terminated string).
       If the value of the VolumeLabelOffset field is not 0x00000014,
       this field must be ignored (==> it doesn't exists ==> ANSI). */
    DWORD cbVolumeLabelUnicodeOffset;
} SHELL_LINK_INFO_VOLUME_IDW, *LPSHELL_LINK_INFO_VOLUME_IDW;

/*****************************************************************************
 * SHELL_LINK_INFO_CNR_LINKA/W (CNR = Common Network Relative)
 * If cbNetNameOffset == 0x00000014 then use SHELL_LINK_INFO_CNR_LINKA
 * If cbNetNameOffset > 0x00000014 then use SHELL_LINK_INFO_CNR_LINKW
 */
typedef struct tagSHELL_LINK_INFO_CNR_LINKA
{
    /* Size of the CommonNetworkRelativeLink field (>= 0x00000014) */
    DWORD cbSize;
    /* Specifies which fields are present/populated (SLI_CNR_*) */
    DWORD dwFlags;
    /* Offset of the NetName field (ANSI, NULL–terminated string) */
    DWORD cbNetNameOffset;
    /* Offset of the DeviceName field (ANSI, NULL–terminated string) */
    DWORD cbDeviceNameOffset;
    /* Type of the network provider (WNNC_NET_* defined in winnetwk.h) */
    DWORD dwNetworkProviderType;
} SHELL_LINK_INFO_CNR_LINKA, *LPSHELL_LINK_INFO_CNR_LINKA;

typedef struct tagSHELL_LINK_INFO_CNR_LINKW
{
    /* Size of the CommonNetworkRelativeLink field (>= 0x00000014) */
    DWORD cbSize;
    /* Specifies which fields are present/populated (SLI_CNR_*) */
    DWORD dwFlags;
    /* Offset of the NetName field (ANSI, NULL–terminated string) */
    DWORD cbNetNameOffset;
    /* Offset of the DeviceName field (ANSI, NULL–terminated string) */
    DWORD cbDeviceNameOffset;
    /* Type of the network provider (WNNC_NET_* defined in winnetwk.h) */
    DWORD dwNetworkProviderType;
    /* Offset of the NetNameUnicode field (Unicode, NULL–terminated string) */
    DWORD cbNetNameUnicodeOffset;
    /* Offset of the DeviceNameUnicode field (Unicode, NULL–terminated string) */
    DWORD cbDeviceNameUnicodeOffset;
} SHELL_LINK_INFO_CNR_LINKW, *LPSHELL_LINK_INFO_CNR_LINKW;

/* DeviceName is present */
#define SLI_CNR_VALID_DEVICE   0x00000001
/* NetworkProviderType is present */
#define SLI_CNR_VALID_NET_TYPE 0x00000002

/*****************************************************************************
 * Shell Link Extra Data (IShellLinkDataList)
 */
typedef struct tagEXP_TRACKER
{
    /* .cbSize = 0x00000060, .dwSignature = 0xa0000003 */
    DATABLOCK_HEADER dbh;
    /* Length >= 0x00000058 */
    DWORD nLength;
    /* Must be 0x00000000 */
    DWORD nVersion;
    /* NetBIOS name (ANSI, unused bytes are set to zero) */
    CHAR szMachineID[16]; /* "variable" >= 16 (?) */
    /* Some GUIDs for the Link Tracking service (from the FS?) */
    GUID guidDroidVolume;
    GUID guidDroidObject;
    GUID guidDroidBirthVolume;
    GUID guidDroidBirthObject;
} EXP_TRACKER, *LPEXP_TRACKER;

typedef struct tagEXP_SHIM
{
    /* .cbSize >= 0x00000088, .dwSignature = 0xa0000008 */
    DATABLOCK_HEADER dbh;
    /* Name of a shim layer to apply (Unicode, unused bytes are set to zero) */
    WCHAR szwLayerName[64]; /* "variable" >= 64 */
} EXP_SHIM, *LPEXP_SHIM;

typedef struct tagEXP_KNOWN_FOLDER
{
    /* .cbSize = 0x0000001c, .dwSignature = 0xa000000b */
    DATABLOCK_HEADER dbh;
    /* A GUID value that identifies a known folder */
    GUID guidKnownFolder;
    /* Specifies the location of the ItemID of the first child
       segment of the IDList specified by guidKnownFolder */
    DWORD cbOffset;
} EXP_KNOWN_FOLDER, *LPEXP_KNOWN_FOLDER;

typedef struct tagEXP_VISTA_ID_LIST
{
    /* .cbSize >= 0x0000000a, .dwSignature = 0xa000000c */
    DATABLOCK_HEADER dbh;
    /* Specifies an alternate IDList that can be used instead 
       of the "normal" IDList (SLDF_HAS_ID_LIST) */
    /* LPITEMIDLIST pIDList; (variable) */
} EXP_VISTA_ID_LIST, *LPEXP_VISTA_ID_LIST;

#define EXP_TRACKER_SIG       0xa0000003
#define EXP_SHIM_SIG          0xa0000008
#define EXP_KNOWN_FOLDER_SIG  0xa000000b
#define EXP_VISTA_ID_LIST_SIG 0xa000000c

#include <poppack.h>

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif // __SHLOBJ_UNDOC__H
