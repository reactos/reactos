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

class CMenuBand;
class CMenuFocusManager;

#define WM_USER_ISTRACKEDITEM (WM_APP+41)
#define WM_USER_CHANGETRACKEDITEM (WM_APP+42)

class CMenuToolbarBase :
    public CWindowImplBaseT< CToolbar<DWORD_PTR>, CControlWinTraits >
{
    CContainedWindow m_pager;
private:
    HFONT   m_marlett;
    BOOL    m_useFlatMenus;
    BOOL    m_disableMouseTrack;
    BOOL    m_timerEnabled;

protected:
    CMenuBand * m_menuBand;
    DWORD       m_dwMenuFlags;
    BOOL        m_hasSizes;
    SIZE        m_idealSize;
    SIZE        m_itemSize;
    BOOL        m_usePager;
    CMenuToolbarBase * m_hotBar;
    INT                m_hotItem;
    CMenuToolbarBase * m_popupBar;
    INT                m_popupItem;

    DWORD m_initFlags;
    BOOL m_isTrackingPopup;

    INT m_executeIndex;
    INT m_executeItem;
    DWORD_PTR m_executeData;

    BOOL m_cancelingPopup;

public:
    CMenuToolbarBase(CMenuBand *menuBand, BOOL usePager);
    virtual ~CMenuToolbarBase();

    HRESULT IsWindowOwner(HWND hwnd);
    HRESULT CreateToolbar(HWND hwndParent, DWORD dwFlags);
    HRESULT GetWindow(HWND *phwnd);
    HRESULT ShowDW(BOOL fShow);
    HRESULT Close();

    HRESULT OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult);

    HRESULT ChangeHotItem(CMenuToolbarBase * toolbar, INT item, DWORD dwFlags);
    HRESULT ChangePopupItem(CMenuToolbarBase * toolbar, INT item);

    HRESULT PopupSubMenu(UINT itemId, UINT index, IShellMenu* childShellMenu, BOOL keyInitiated);
    HRESULT PopupSubMenu(UINT itemId, UINT index, HMENU menu);
    HRESULT TrackContextMenu(IContextMenu* contextMenu, POINT pt);

    HRESULT KeyboardItemChange(DWORD changeType);

    HRESULT PrepareExecuteItem(INT iItem);
    HRESULT ExecuteItem();

    HRESULT GetSizes(SIZE* pMinSize, SIZE* pMaxSize, SIZE* pIntegralSize);
    HRESULT SetPosSize(int x, int y, int cx, int cy);

    void InvalidateDraw();

    HRESULT DisableMouseTrack(BOOL bDisable);

    virtual HRESULT FillToolbar(BOOL clearFirst=FALSE) = 0;

    HRESULT CancelCurrentPopup();
    HRESULT PopupItem(INT iItem, BOOL keyInitiated);
    HRESULT GetDataFromId(INT iItem, INT* pIndex, DWORD_PTR* pData);

    HRESULT KillPopupTimer();

    HRESULT MenuBarMouseDown(INT iIndex, BOOL isLButton);
    HRESULT MenuBarMouseUp(INT iIndex, BOOL isLButton);
    HRESULT ProcessClick(INT iItem);
    HRESULT ProcessContextMenu(INT iItem);
    HRESULT BeforeCancelPopup();

protected:
    virtual HRESULT OnDeletingButton(const NMTOOLBAR * tb) = 0;

    virtual HRESULT InternalGetTooltip(INT iItem, INT index, DWORD_PTR dwData, LPWSTR pszText, INT cchTextMax) = 0;
    virtual HRESULT InternalExecuteItem(INT iItem, INT index, DWORD_PTR dwData) = 0;
    virtual HRESULT InternalPopupItem(INT iItem, INT index, DWORD_PTR dwData, BOOL keyInitiated) = 0;
    virtual HRESULT InternalHasSubMenu(INT iItem, INT index, DWORD_PTR dwData) = 0;
    virtual HRESULT InternalContextMenu(INT iItem, INT index, DWORD_PTR dwData, POINT pt) = 0;

    HRESULT AddButton(DWORD commandId, LPCWSTR caption, BOOL hasSubMenu, INT iconId, DWORD_PTR buttonData, BOOL last);
    HRESULT AddSeparator(BOOL last);
    HRESULT AddPlaceholder();
    HRESULT ClearToolbar();

    HWND GetToolbar() { return m_hWnd; }

private:
    HRESULT UpdateImageLists();

    HRESULT OnPagerCalcSize(LPNMPGCALCSIZE csize);
    HRESULT OnCustomDraw(LPNMTBCUSTOMDRAW cdraw, LRESULT * theResult);
    HRESULT OnGetInfoTip(NMTBGETINFOTIP * tip);

    LRESULT IsTrackedItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT ChangeTrackedItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnWinEventWrap(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    HRESULT OnPopupTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    BEGIN_MSG_MAP(CMenuToolbarBase)
        MESSAGE_HANDLER(WM_USER_ISTRACKEDITEM, IsTrackedItem)
        MESSAGE_HANDLER(WM_USER_CHANGETRACKEDITEM, ChangeTrackedItem)
        MESSAGE_HANDLER(WM_COMMAND, OnWinEventWrap)
        MESSAGE_HANDLER(WM_NOTIFY, OnWinEventWrap)
        MESSAGE_HANDLER(WM_TIMER, OnPopupTimer)
    END_MSG_MAP()
};

class CMenuStaticToolbar :
    public CMenuToolbarBase
{
private:
    HMENU m_hmenu;
    HWND m_hwndMenu;

public:
    CMenuStaticToolbar(CMenuBand *menuBand);
    virtual ~CMenuStaticToolbar();

    HRESULT SetMenu(HMENU hmenu, HWND hwnd, DWORD dwFlags);
    HRESULT GetMenu(HMENU *phmenu, HWND *phwnd, DWORD *pdwFlags);

    virtual HRESULT FillToolbar(BOOL clearFirst=FALSE);

protected:
    virtual HRESULT OnDeletingButton(const NMTOOLBAR * tb);

    virtual HRESULT InternalGetTooltip(INT iItem, INT index, DWORD_PTR dwData, LPWSTR pszText, INT cchTextMax);
    virtual HRESULT InternalExecuteItem(INT iItem, INT index, DWORD_PTR dwData);
    virtual HRESULT InternalPopupItem(INT iItem, INT index, DWORD_PTR dwData, BOOL keyInitiated);
    virtual HRESULT InternalHasSubMenu(INT iItem, INT index, DWORD_PTR dwData);
    virtual HRESULT InternalContextMenu(INT iItem, INT index, DWORD_PTR dwData, POINT pt);
};

class CMenuSFToolbar :
    public CMenuToolbarBase
{
private:
    CComPtr<IShellFolder> m_shellFolder;
    LPCITEMIDLIST  m_idList;
    HKEY           m_hKey;

public:
    CMenuSFToolbar(CMenuBand *menuBand);
    virtual ~CMenuSFToolbar();

    HRESULT SetShellFolder(IShellFolder *psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags);
    HRESULT GetShellFolder(DWORD *pdwFlags, LPITEMIDLIST *ppidl, REFIID riid, void **ppv);

    virtual HRESULT FillToolbar(BOOL clearFirst=FALSE);

protected:
    virtual HRESULT OnDeletingButton(const NMTOOLBAR * tb);

    virtual HRESULT InternalGetTooltip(INT iItem, INT index, DWORD_PTR dwData, LPWSTR pszText, INT cchTextMax);
    virtual HRESULT InternalExecuteItem(INT iItem, INT index, DWORD_PTR dwData);
    virtual HRESULT InternalPopupItem(INT iItem, INT index, DWORD_PTR dwData, BOOL keyInitiated);
    virtual HRESULT InternalHasSubMenu(INT iItem, INT index, DWORD_PTR dwData);
    virtual HRESULT InternalContextMenu(INT iItem, INT index, DWORD_PTR dwData, POINT pt);
};
