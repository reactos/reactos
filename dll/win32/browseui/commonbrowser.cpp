/*
 * ReactOS Explorer
 *
 * Copyright 2009 Andrew Hill <ash77 at domain reactos.org>
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

#include "precomp.h"

CCommonBrowser::CCommonBrowser()
{
}

CCommonBrowser::~CCommonBrowser()
{
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds,
    OLECMD prgCmds[  ], OLECMDTEXT *pCmdText)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
    DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::GetParentSite(IOleInPlaceSite **ppipsite)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::SetTitle(IShellView *psv, LPCWSTR pszName)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::GetTitle(IShellView *psv, LPWSTR pszName, DWORD cchName)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::GetOleObject(IOleObject **ppobjv)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::GetTravelLog(ITravelLog **pptl)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::ShowControlWindow(UINT id, BOOL fShow)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::IsControlWindowShown(UINT id, BOOL *pfShown)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::IEGetDisplayName(LPCITEMIDLIST pidl, LPWSTR pwszName, UINT uFlags)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::IEParseDisplayName(UINT uiCP, LPCWSTR pwszPath, LPITEMIDLIST *ppidlOut)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::DisplayParseError(HRESULT hres, LPCWSTR pwszPath)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::NavigateToPidl(LPCITEMIDLIST pidl, DWORD grfHLNF)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::SetNavigateState(BNSTATE bnstate)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::GetNavigateState(BNSTATE *pbnstate)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::NotifyRedirect(IShellView *psv, LPCITEMIDLIST pidl, BOOL *pfDidBrowse)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::UpdateWindowList()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::UpdateBackForwardState()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::SetFlags(DWORD dwFlags, DWORD dwFlagMask)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::GetFlags(DWORD *pdwFlags)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::CanNavigateNow( void)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::GetPidl(LPITEMIDLIST *ppidl)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::SetReferrer(LPCITEMIDLIST pidl)
{
    return E_NOTIMPL;
}

DWORD STDMETHODCALLTYPE CCommonBrowser::GetBrowserIndex()
{
    return 0;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::GetBrowserByIndex(DWORD dwID, IUnknown **ppunk)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::GetHistoryObject(IOleObject **ppole, IStream **pstm, IBindCtx **ppbc)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::SetHistoryObject(IOleObject *pole, BOOL fIsLocalAnchor)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::CacheOLEServer(IOleObject *pole)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::GetSetCodePage(VARIANT *pvarIn, VARIANT *pvarOut)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::OnHttpEquiv(IShellView *psv, BOOL fDone, VARIANT *pvarargIn, VARIANT *pvarargOut)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::GetPalette(HPALETTE *hpal)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::RegisterWindow(BOOL fForceRegister, int swc)
{
    return E_NOTIMPL;
}

LRESULT STDMETHODCALLTYPE CCommonBrowser::WndProcBS(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::SetAsDefFolderSettings()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::GetViewRect(RECT *prc)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::OnSize(WPARAM wParam)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::OnCreate(struct tagCREATESTRUCTW *pcs)
{
    return E_NOTIMPL;
}

LRESULT STDMETHODCALLTYPE CCommonBrowser::OnCommand(WPARAM wParam, LPARAM lParam)
{
    return 0;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::OnDestroy()
{
    return E_NOTIMPL;
}

LRESULT STDMETHODCALLTYPE CCommonBrowser::OnNotify(struct tagNMHDR *pnm)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::OnSetFocus()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::OnFrameWindowActivateBS(BOOL fActive)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::ReleaseShellView()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::ActivatePendingView()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::CreateViewWindow(
    IShellView *psvNew, IShellView *psvOld, LPRECT prcView, HWND *phwnd)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::CreateBrowserPropSheetExt(REFIID riid, void **ppv)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::GetViewWindow(HWND *phwndView)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::GetBaseBrowserData(LPCBASEBROWSERDATA *pbbd)
{
    return E_NOTIMPL;
}

LPBASEBROWSERDATA CCommonBrowser::PutBaseBrowserData()
{
    return NULL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::InitializeTravelLog(ITravelLog *ptl, DWORD dw)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::SetTopBrowser()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::Offline(int iCmd)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::AllowViewResize(BOOL f)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::SetActivateState(UINT u)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::UpdateSecureLockIcon(int eSecureLock)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::InitializeDownloadManager()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::InitializeTransitionSite()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_Initialize(HWND hwnd, IUnknown *pauto)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_CancelPendingNavigationAsync( void)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_CancelPendingView()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_MaySaveChanges()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_PauseOrResumeView(BOOL fPaused)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_DisableModeless()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_NavigateToPidl(LPCITEMIDLIST pidl, DWORD grfHLNF, DWORD dwFlags)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_TryShell2Rename(IShellView *psv, LPCITEMIDLIST pidlNew)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_SwitchActivationNow()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_ExecChildren(IUnknown *punkBar, BOOL fBroadcast,
    const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_SendChildren(
    HWND hwndBar, BOOL fBroadcast, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::GetFolderSetData(struct tagFolderSetData *pfsd)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_OnFocusChange(UINT itb)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::v_ShowHideChildWindows(BOOL fChildOnly)
{
    return E_NOTIMPL;
}

UINT STDMETHODCALLTYPE CCommonBrowser::_get_itbLastFocus()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_put_itbLastFocus(UINT itbLastFocus)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_UIActivateView(UINT uState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_GetViewBorderRect(RECT *prc)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_UpdateViewRectSize()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_ResizeNextBorder(UINT itb)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_ResizeView()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_GetEffectiveClientArea(LPRECT lprectBorder, HMONITOR hmon)
{
    return E_NOTIMPL;
}

IStream *STDMETHODCALLTYPE CCommonBrowser::v_GetViewStream(LPCITEMIDLIST pidl, DWORD grfMode, LPCWSTR pwszName)
{
    return NULL;
}

LRESULT STDMETHODCALLTYPE CCommonBrowser::ForwardViewMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::SetAcceleratorMenu(HACCEL hacc)
{
    return E_NOTIMPL;
}

int STDMETHODCALLTYPE CCommonBrowser::_GetToolbarCount()
{
    return 0;
}

LPTOOLBARITEM STDMETHODCALLTYPE CCommonBrowser::_GetToolbarItem(int itb)
{
    return NULL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_SaveToolbars(IStream *pstm)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_LoadToolbars(IStream *pstm)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_CloseAndReleaseToolbars(BOOL fClose)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::v_MayGetNextToolbarFocus(LPMSG lpMsg, UINT itbNext,
    int citb, LPTOOLBARITEM *pptbi, HWND *phwnd)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_ResizeNextBorderHelper(UINT itb, BOOL bUseHmonitor)
{
    return E_NOTIMPL;
}

UINT STDMETHODCALLTYPE CCommonBrowser::_FindTBar(IUnknown *punkSrc)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_SetFocus(LPTOOLBARITEM ptbi, HWND hwnd, LPMSG lpMsg)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::v_MayTranslateAccelerator(MSG *pmsg)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_GetBorderDWHelper(IUnknown *punkSrc, LPRECT lprectBorder, BOOL bUseHmonitor)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::v_CheckZoneCrossing(LPCITEMIDLIST pidl)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::_PositionViewWindow(HWND, RECT *)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::IEParseDisplayNameEx(
    UINT, PCWSTR, DWORD, LPITEMIDLIST *)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::InsertMenusSB(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::SetMenuSB(HMENU hmenuShared, HOLEMENU holemenuRes, HWND hwndActiveObject)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::RemoveMenusSB(HMENU hmenuShared)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::SetStatusTextSB(LPCOLESTR pszStatusText)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::EnableModelessSB(BOOL fEnable)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::TranslateAcceleratorSB(MSG *pmsg, WORD wID)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::BrowseObject(LPCITEMIDLIST pidl, UINT wFlags)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::GetViewStateStream(DWORD grfMode, IStream **ppStrm)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::GetControlWindow(UINT id, HWND *lphwnd)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::SendControlMsg(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::QueryActiveShellView(struct IShellView **ppshv)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::OnViewWindowActive(struct IShellView *ppshv)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::SetToolbarItems(LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::GetPropertyBag(long flags, REFIID riid, void **ppvObject)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::GetWindow(HWND *lphwnd)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::GetBorderDW(IUnknown* punkObj, LPRECT prcBorder)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::RequestBorderSpaceDW(IUnknown* punkObj, LPCBORDERWIDTHS pbw)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::SetBorderSpaceDW(IUnknown* punkObj, LPCBORDERWIDTHS pbw)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::AddToolbar(IUnknown *punkSrc, LPCWSTR pwszItem, DWORD dwAddFlags)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::RemoveToolbar(IUnknown *punkSrc, DWORD dwRemoveFlags)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::FindToolbar(LPCWSTR pwszItem, REFIID riid, void **ppv)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::OnFocusChangeIS(IUnknown *punkObj, BOOL fSetFocus)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::DragLeave()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCommonBrowser::Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    return E_NOTIMPL;
}
