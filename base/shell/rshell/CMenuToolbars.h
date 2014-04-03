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


class CMenuToolbarBase
{
private:
    HWND  m_hwnd;        // May be the pager
    HFONT m_marlett;
    BOOL  m_useFlatMenus;
    WNDPROC m_SubclassOld;
    BOOL m_disableMouseTrack;
    BOOL m_timerEnabled;

protected:
    CMenuBand * m_menuBand;
    HWND        m_hwndToolbar;
    DWORD       m_dwMenuFlags;
    BOOL        m_hasIdealSize;
    SIZE        m_idealSize;
    BOOL        m_usePager;
    CMenuToolbarBase * m_hotBar;
    INT                m_hotItem;
    CMenuToolbarBase * m_popupBar;
    INT                m_popupItem;

    DWORD m_initFlags;
    BOOL m_isTracking;

private:
    static LRESULT CALLBACK s_SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
    CMenuToolbarBase(CMenuBand *menuBand, BOOL usePager);
    virtual ~CMenuToolbarBase();

    HRESULT IsWindowOwner(HWND hwnd);
    HRESULT CreateToolbar(HWND hwndParent, DWORD dwFlags);
    HRESULT GetWindow(HWND *phwnd);
    HRESULT ShowWindow(BOOL fShow);
    HRESULT Close();

    HRESULT OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult);

    HRESULT ChangeHotItem(CMenuToolbarBase * toolbar, INT item, DWORD dwFlags);
    HRESULT ChangePopupItem(CMenuToolbarBase * toolbar, INT item);

    HRESULT PopupSubMenu(UINT itemId, UINT index, IShellMenu* childShellMenu);
    HRESULT PopupSubMenu(UINT itemId, UINT index, HMENU menu);
    HRESULT DoContextMenu(IContextMenu* contextMenu);

    HRESULT KeyboardItemChange(DWORD changeType);
    HRESULT OnHotItemChange(const NMTBHOTITEM * hot, LRESULT * theResult);

    HRESULT IsTrackedItem(INT index);
    HRESULT ChangeTrackedItem(INT index, BOOL wasTracking);

    HRESULT GetIdealSize(SIZE& size);
    HRESULT SetPosSize(int x, int y, int cx, int cy);

    void InvalidateDraw();

    HRESULT DisableMouseTrack(BOOL bDisable);

    virtual HRESULT FillToolbar(BOOL clearFirst=FALSE) = 0;
    virtual HRESULT OnContextMenu(NMMOUSE * rclick) = 0;

    HRESULT CancelCurrentPopup();
    HRESULT PopupItem(INT iItem);
    HRESULT GetDataFromId(INT iItem, INT* pIndex, DWORD_PTR* pData);

    HRESULT KillPopupTimer();

protected:
    virtual HRESULT OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult);

    virtual HRESULT OnDeletingButton(const NMTOOLBAR * tb) = 0;
    virtual HRESULT InternalPopupItem(INT iItem, INT index, DWORD_PTR dwData) = 0;
    virtual HRESULT InternalHasSubMenu(INT iItem, INT index, DWORD_PTR dwData) = 0;

    virtual HRESULT GetInfoTip(LPWSTR pszText, INT cchTextMax, INT iItem, INT index, DWORD_PTR dwData)=0;

    LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HRESULT AddButton(DWORD commandId, LPCWSTR caption, BOOL hasSubMenu, INT iconId, DWORD_PTR buttonData, BOOL last);
    HRESULT AddSeparator(BOOL last);
    HRESULT AddPlaceholder();

    HRESULT UpdateImageLists();

private:
    HRESULT OnPagerCalcSize(LPNMPGCALCSIZE csize);
    HRESULT OnPopupTimer(DWORD timerId);
    HRESULT OnCustomDraw(LPNMTBCUSTOMDRAW cdraw, LRESULT * theResult);
    HRESULT OnGetInfoTip(NMTBGETINFOTIP * tip);
};

class CMenuStaticToolbar :
    public CMenuToolbarBase
{
private:
    HMENU m_hmenu;

public:
    CMenuStaticToolbar(CMenuBand *menuBand);
    virtual ~CMenuStaticToolbar() {}

    HRESULT SetMenu(HMENU hmenu, HWND hwnd, DWORD dwFlags);
    HRESULT GetMenu(HMENU *phmenu, HWND *phwnd, DWORD *pdwFlags);

    virtual HRESULT FillToolbar(BOOL clearFirst=FALSE);
    virtual HRESULT OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult);
    virtual HRESULT OnContextMenu(NMMOUSE * rclick);

protected:
    virtual HRESULT OnDeletingButton(const NMTOOLBAR * tb);

    virtual HRESULT GetInfoTip(LPWSTR pszText, INT cchTextMax, INT iItem, INT index, DWORD_PTR dwData);

    virtual HRESULT InternalPopupItem(INT iItem, INT index, DWORD_PTR dwData);
    virtual HRESULT InternalHasSubMenu(INT iItem, INT index, DWORD_PTR dwData);
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
    virtual HRESULT OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult);
    virtual HRESULT OnContextMenu(NMMOUSE * rclick);

protected:
    virtual HRESULT OnDeletingButton(const NMTOOLBAR * tb);

    virtual HRESULT GetInfoTip(LPWSTR pszText, INT cchTextMax, INT iItem, INT index, DWORD_PTR dwData);

    virtual HRESULT InternalPopupItem(INT iItem, INT index, DWORD_PTR dwData);
    virtual HRESULT InternalHasSubMenu(INT iItem, INT index, DWORD_PTR dwData);
};
