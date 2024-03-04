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
    public CComCoClass<CMenuBand, &CLSID_MenuBand>,
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

    DECLARE_REGISTRY_RESOURCEID(IDR_MENUBAND)
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
    STDMETHOD(GetBandInfo)(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi) override;

    // *** IDockingWindow methods ***
    STDMETHOD(ShowDW)(BOOL fShow) override;
    STDMETHOD(CloseDW)(DWORD dwReserved) override;
    STDMETHOD(ResizeBorderDW)(LPCRECT prcBorder, IUnknown *punkToolbarSite, BOOL fReserved) override;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow)(HWND *phwnd) override;
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode) override;

    // *** IObjectWithSite methods ***
    STDMETHOD(SetSite)(IUnknown *pUnkSite) override;
    STDMETHOD(GetSite)(REFIID riid, PVOID *ppvSite) override;

    // *** IInputObject methods ***
    STDMETHOD(UIActivateIO)(BOOL fActivate, LPMSG lpMsg) override;
    STDMETHOD(HasFocusIO)() override;
    STDMETHOD(TranslateAcceleratorIO)(LPMSG lpMsg) override;

    // *** IPersistStream methods ***
    STDMETHOD(IsDirty)() override;
    STDMETHOD(Load)(IStream *pStm) override;
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty) override;
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize) override;

    // *** IPersist methods ***
    STDMETHOD(GetClassID)(CLSID *pClassID) override;

    // *** IOleCommandTarget methods ***
    STDMETHOD(QueryStatus)(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText) override;
    STDMETHOD(Exec)(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut) override;

    // *** IServiceProvider methods ***
    STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void **ppvObject) override;

    // *** IMenuPopup methods ***
    STDMETHOD(Popup)(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags) override;
    STDMETHOD(OnSelect)(DWORD dwSelectType) override;
    STDMETHOD(SetSubMenu)(IMenuPopup *pmp, BOOL fSet) override;

    // *** IDeskBar methods ***
    STDMETHOD(SetClient)(IUnknown *punkClient) override;
    STDMETHOD(GetClient)(IUnknown **ppunkClient) override;
    STDMETHOD(OnPosRectChangeDB)(RECT *prc) override;

    // *** IMenuBand methods ***
    STDMETHOD(IsMenuMessage)(MSG *pmsg) override;
    STDMETHOD(TranslateMenuMessage)(MSG *pmsg, LRESULT *plRet) override;

    // *** IShellMenu methods ***
    STDMETHOD(Initialize)(IShellMenuCallback *psmc, UINT uId, UINT uIdAncestor, DWORD dwFlags) override;
    STDMETHOD(GetMenuInfo)(IShellMenuCallback **ppsmc, UINT *puId, UINT *puIdAncestor, DWORD *pdwFlags) override;
    STDMETHOD(SetShellFolder)(IShellFolder *psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags) override;
    STDMETHOD(GetShellFolder)(DWORD *pdwFlags, LPITEMIDLIST *ppidl, REFIID riid, void **ppv) override;
    STDMETHOD(SetMenu)(HMENU hmenu, HWND hwnd, DWORD dwFlags) override;
    STDMETHOD(GetMenu)(HMENU *phmenu, HWND *phwnd, DWORD *pdwFlags) override;
    STDMETHOD(InvalidateItem)(LPSMDATA psmd, DWORD dwFlags) override;
    STDMETHOD(GetState)(LPSMDATA psmd) override;
    STDMETHOD(SetMenuToolbar)(IUnknown *punk, DWORD dwFlags) override;

    // *** IWinEventHandler methods ***
    STDMETHOD(OnWinEvent)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult) override;
    STDMETHOD(IsWindowOwner)(HWND hWnd) override;

    // *** IShellMenu2 methods ***
    STDMETHOD(GetSubMenu)(THIS) override;
    STDMETHOD(SetToolbar)(THIS) override;
    STDMETHOD(SetMinWidth)(THIS) override;
    STDMETHOD(SetNoBorder)(THIS) override;
    STDMETHOD(SetTheme)(THIS) override;

    // *** IShellMenuAcc methods ***
    STDMETHOD(GetTop)(THIS) override;
    STDMETHOD(GetBottom)(THIS) override;
    STDMETHOD(GetTracked)(THIS) override;
    STDMETHOD(GetParentSite)(THIS) override;
    STDMETHOD(GetState)(THIS) override;
    STDMETHOD(DoDefaultAction)(THIS) override;
    STDMETHOD(IsEmpty)(THIS) override;

    HRESULT _CallCBWithItemId(UINT Id, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT _CallCBWithItemPidl(LPITEMIDLIST pidl, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT _TrackSubMenu(HMENU popup, INT x, INT y, RECT& rcExclude);
    HRESULT _TrackContextMenu(IContextMenu * popup, INT x, INT y);
    HRESULT _GetTopLevelWindow(HWND*topLevel);
    HRESULT _ChangeHotItem(CMenuToolbarBase * tb, INT id, DWORD dwFlags);
    HRESULT _ChangePopupItem(CMenuToolbarBase * tb, INT id);
    HRESULT _MenuItemSelect(DWORD changeType);
    HRESULT _CancelCurrentPopup();
    HRESULT _OnPopupSubMenu(IShellMenu * childShellMenu, POINTL * pAt, RECTL * pExclude, BOOL mouseInitiated);
    HRESULT _BeforeCancelPopup();
    HRESULT _DisableMouseTrack(BOOL bDisable);
    HRESULT _SetChildBand(CMenuBand * child);
    HRESULT _SetParentBand(CMenuBand * parent);
    HRESULT _IsPopup();
    HRESULT _IsTracking();
    HRESULT _KillPopupTimers();
    HRESULT _MenuBarMouseDown(HWND hwnd, INT item, BOOL isLButton);
    HRESULT _MenuBarMouseUp(HWND hwnd, INT item, BOOL isLButton);
    HRESULT _HasSubMenu();

    HRESULT AdjustForTheme(BOOL bFlatStyle);

    BOOL UseBigIcons()
    {
        return m_useBigIcons;
    }

private:
    HRESULT _KeyboardItemChange(DWORD change);
    HRESULT _CallCB(UINT uMsg, WPARAM wParam, LPARAM lParam, UINT id = 0, LPITEMIDLIST pidl = NULL);
};
