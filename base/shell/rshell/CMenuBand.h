/*
* Shell Menu Band
*
* Copyright 2014 David Quintana
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
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
*/
#pragma once

class CMenuToolbarBase;
class CMenuStaticToolbar;
class CMenuSFToolbar;
class CMenuFocusManager;

class CMenuBand :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDeskBand,
    public IObjectWithSite,
    public IInputObject,
    public IPersistStream,
    public IOleCommandTarget,
    public IServiceProvider,
    public IMenuPopup,
    public IMenuBand,
    public IShellMenu2,
    public IWinEventHandler,
    public IShellMenuAcc
{
private:
    CMenuFocusManager  * m_focusManager;
    CMenuStaticToolbar * m_staticToolbar;
    CMenuSFToolbar     * m_SFToolbar;

    CComPtr<IOleWindow>         m_site;
    CComPtr<IShellMenuCallback> m_psmc;
    CComPtr<IMenuPopup>         m_subMenuChild;
    CComPtr<IMenuPopup>         m_subMenuParent;
    CComPtr<CMenuBand>          m_childBand;
    CComPtr<CMenuBand>          m_parentBand;

    UINT  m_uId;
    UINT  m_uIdAncestor;
    DWORD m_dwFlags;
    PVOID m_UserData;
    HMENU m_hmenu;
    HWND  m_menuOwner;

    BOOL m_useBigIcons;
    HWND m_topLevelWindow;

    CMenuToolbarBase * m_hotBar;
    INT                m_hotItem;
    CMenuToolbarBase * m_popupBar;
    INT                m_popupItem;

    BOOL m_Show;
    BOOL m_shellBottom;

    HMENU m_trackedPopup;
    HWND m_trackedHwnd;

public:
    CMenuBand();
    virtual ~CMenuBand();

    DECLARE_NOT_AGGREGATABLE(CMenuBand)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CMenuBand)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBar, IMenuPopup)
        COM_INTERFACE_ENTRY_IID(IID_IShellMenu, IShellMenu)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IDockingWindow, IDockingWindow)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBand, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
        COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
        COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersistStream)
        COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
        COM_INTERFACE_ENTRY_IID(IID_IMenuPopup, IMenuPopup)
        COM_INTERFACE_ENTRY_IID(IID_IMenuBand, IMenuBand)
        COM_INTERFACE_ENTRY_IID(IID_IShellMenu2, IShellMenu2)
        COM_INTERFACE_ENTRY_IID(IID_IWinEventHandler, IWinEventHandler)
        COM_INTERFACE_ENTRY_IID(IID_IShellMenuAcc, IShellMenuAcc)
    END_COM_MAP()

    // *** IDeskBand methods ***
    virtual HRESULT STDMETHODCALLTYPE GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi);

    // *** IDockingWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE ShowDW(BOOL fShow);
    virtual HRESULT STDMETHODCALLTYPE CloseDW(DWORD dwReserved);
    virtual HRESULT STDMETHODCALLTYPE ResizeBorderDW(LPCRECT prcBorder, IUnknown *punkToolbarSite, BOOL fReserved);

    // *** IOleWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *phwnd);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

    // *** IObjectWithSite methods ***
    virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown *pUnkSite);
    virtual HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, PVOID *ppvSite);

    // *** IInputObject methods ***
    virtual HRESULT STDMETHODCALLTYPE UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    virtual HRESULT STDMETHODCALLTYPE HasFocusIO();
    virtual HRESULT STDMETHODCALLTYPE TranslateAcceleratorIO(LPMSG lpMsg);

    // *** IPersistStream methods ***
    virtual HRESULT STDMETHODCALLTYPE IsDirty();
    virtual HRESULT STDMETHODCALLTYPE Load(IStream *pStm);
    virtual HRESULT STDMETHODCALLTYPE Save(IStream *pStm, BOOL fClearDirty);
    virtual HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER *pcbSize);

    // *** IPersist methods ***
    virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);

    // *** IOleCommandTarget methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

    // *** IServiceProvider methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

    // *** IMenuPopup methods ***
    virtual HRESULT STDMETHODCALLTYPE Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags);
    virtual HRESULT STDMETHODCALLTYPE OnSelect(DWORD dwSelectType);
    virtual HRESULT STDMETHODCALLTYPE SetSubMenu(IMenuPopup *pmp, BOOL fSet);

    // *** IDeskBar methods ***
    virtual HRESULT STDMETHODCALLTYPE SetClient(IUnknown *punkClient);
    virtual HRESULT STDMETHODCALLTYPE GetClient(IUnknown **ppunkClient);
    virtual HRESULT STDMETHODCALLTYPE OnPosRectChangeDB(RECT *prc);

    // *** IMenuBand methods ***
    virtual HRESULT STDMETHODCALLTYPE IsMenuMessage(MSG *pmsg);
    virtual HRESULT STDMETHODCALLTYPE TranslateMenuMessage(MSG *pmsg, LRESULT *plRet);

    // *** IShellMenu methods ***
    virtual HRESULT STDMETHODCALLTYPE Initialize(IShellMenuCallback *psmc, UINT uId, UINT uIdAncestor, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE GetMenuInfo(IShellMenuCallback **ppsmc, UINT *puId, UINT *puIdAncestor, DWORD *pdwFlags);
    virtual HRESULT STDMETHODCALLTYPE SetShellFolder(IShellFolder *psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE GetShellFolder(DWORD *pdwFlags, LPITEMIDLIST *ppidl, REFIID riid, void **ppv);
    virtual HRESULT STDMETHODCALLTYPE SetMenu(HMENU hmenu, HWND hwnd, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE GetMenu(HMENU *phmenu, HWND *phwnd, DWORD *pdwFlags);
    virtual HRESULT STDMETHODCALLTYPE InvalidateItem(LPSMDATA psmd, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE GetState(LPSMDATA psmd);
    virtual HRESULT STDMETHODCALLTYPE SetMenuToolbar(IUnknown *punk, DWORD dwFlags);

    // *** IWinEventHandler methods ***
    virtual HRESULT STDMETHODCALLTYPE OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult);
    virtual HRESULT STDMETHODCALLTYPE IsWindowOwner(HWND hWnd);

    // *** IShellMenu2 methods ***
    virtual HRESULT STDMETHODCALLTYPE GetSubMenu(THIS);
    virtual HRESULT STDMETHODCALLTYPE SetToolbar(THIS);
    virtual HRESULT STDMETHODCALLTYPE SetMinWidth(THIS);
    virtual HRESULT STDMETHODCALLTYPE SetNoBorder(THIS);
    virtual HRESULT STDMETHODCALLTYPE SetTheme(THIS);

    // *** IShellMenuAcc methods ***
    virtual HRESULT STDMETHODCALLTYPE GetTop(THIS);
    virtual HRESULT STDMETHODCALLTYPE GetBottom(THIS);
    virtual HRESULT STDMETHODCALLTYPE GetTracked(THIS);
    virtual HRESULT STDMETHODCALLTYPE GetParentSite(THIS);
    virtual HRESULT STDMETHODCALLTYPE GetState(THIS);
    virtual HRESULT STDMETHODCALLTYPE DoDefaultAction(THIS);
    virtual HRESULT STDMETHODCALLTYPE IsEmpty(THIS);

    HRESULT _CallCBWithItemId(UINT Id, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT _CallCBWithItemPidl(LPITEMIDLIST pidl, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT _TrackSubMenu(HMENU popup, INT x, INT y, RECT& rcExclude);
    HRESULT _TrackContextMenu(IContextMenu * popup, INT x, INT y);
    HRESULT _GetTopLevelWindow(HWND*topLevel);
    HRESULT _ChangeHotItem(CMenuToolbarBase * tb, INT id, DWORD dwFlags);
    HRESULT _ChangePopupItem(CMenuToolbarBase * tb, INT id);
    HRESULT _MenuItemHotTrack(DWORD changeType);
    HRESULT _CancelCurrentPopup();
    HRESULT _OnPopupSubMenu(IShellMenu * childShellMenu, POINTL * pAt, RECTL * pExclude, BOOL mouseInitiated);
    HRESULT _BeforeCancelPopup();
    HRESULT _DisableMouseTrack(BOOL bDisable);
    HRESULT _SetChildBand(CMenuBand * child);
    HRESULT _SetParentBand(CMenuBand * parent);
    HRESULT _IsPopup();
    HRESULT _IsTracking();
    HRESULT _KillPopupTimers();
    HRESULT _MenuBarMouseDown(HWND hwnd, INT item);
    HRESULT _MenuBarMouseUp(HWND hwnd, INT item);
    HRESULT _HasSubMenu();

    BOOL UseBigIcons()
    {
        return m_useBigIcons;
    }

private:
    HRESULT _KeyboardItemChange(DWORD change);
    HRESULT _CallCB(UINT uMsg, WPARAM wParam, LPARAM lParam, UINT id = 0, LPITEMIDLIST pidl = NULL);
};
