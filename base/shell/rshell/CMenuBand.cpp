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
#include "precomp.h"
#include <windowsx.h>
#include <CommonControls.h>
#include <shlwapi_undoc.h>

#include "CMenuBand.h"
#include "CMenuToolbars.h"
#include "CMenuFocusManager.h"

WINE_DEFAULT_DEBUG_CHANNEL(CMenuBand);

extern "C"
HRESULT CMenuBand_Constructor(REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

    CMenuBand * site = new CComObject<CMenuBand>();

    if (!site)
        return E_OUTOFMEMORY;

    HRESULT hr = site->QueryInterface(riid, ppv);

    if (FAILED(hr))
        site->Release();

    return hr;
}

CMenuBand::CMenuBand() :
    m_staticToolbar(NULL),
    m_SFToolbar(NULL),
    m_site(NULL),
    m_psmc(NULL),
    m_subMenuChild(NULL),
    m_useBigIcons(FALSE),
    m_hotBar(NULL),
    m_hotItem(-1),
    m_popupItem(-1)
{
    m_focusManager = CMenuFocusManager::AcquireManager();

    m_marlett = CreateFont(
        0, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, FF_DONTCARE, L"Marlett");
}

CMenuBand::~CMenuBand()
{
    CMenuFocusManager::ReleaseManager(m_focusManager);

    if (m_staticToolbar)
        delete m_staticToolbar;

    if (m_SFToolbar)
        delete m_SFToolbar;

    DeleteObject(m_marlett);
}

HRESULT STDMETHODCALLTYPE  CMenuBand::Initialize(
    IShellMenuCallback *psmc,
    UINT uId,
    UINT uIdAncestor,
    DWORD dwFlags)
{
    if (m_psmc != psmc)
        m_psmc = psmc;
    m_uId = uId;
    m_uIdAncestor = uIdAncestor;
    m_dwFlags = dwFlags;

    if (m_psmc)
    {
        _CallCB(SMC_CREATE, 0, reinterpret_cast<LPARAM>(&m_UserData));
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::GetMenuInfo(
    IShellMenuCallback **ppsmc,
    UINT *puId,
    UINT *puIdAncestor,
    DWORD *pdwFlags)
{
    if (!pdwFlags) // maybe?
        return E_INVALIDARG;

    if (ppsmc)
        *ppsmc = m_psmc;

    if (puId)
        *puId = m_uId;

    if (puIdAncestor)
        *puIdAncestor = m_uIdAncestor;

    *pdwFlags = m_dwFlags;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::SetMenu(
    HMENU hmenu,
    HWND hwnd,
    DWORD dwFlags)
{
    if (m_staticToolbar == NULL)
    {
        m_staticToolbar = new CMenuStaticToolbar(this);
    }
    m_hmenu = hmenu;
    m_menuOwner;

    HRESULT hr = m_staticToolbar->SetMenu(hmenu, hwnd, dwFlags);
    if (FAILED(hr))
        return hr;

    if (m_site)
    {
        HWND hwndParent;

        hr = m_site->GetWindow(&hwndParent);
        if (FAILED(hr))
            return hr;

        hr = m_staticToolbar->CreateToolbar(hwndParent, m_dwFlags);
        if (FAILED(hr))
            return hr;

        hr = m_staticToolbar->FillToolbar();
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::GetMenu(
    HMENU *phmenu,
    HWND *phwnd,
    DWORD *pdwFlags)
{
    if (m_staticToolbar == NULL)
        return E_FAIL;

    return m_staticToolbar->GetMenu(phmenu, phwnd, pdwFlags);
}

HRESULT STDMETHODCALLTYPE  CMenuBand::SetSite(IUnknown *pUnkSite)
{
    HWND    hwndParent;
    HRESULT hr;

    m_site = NULL;

    if (pUnkSite == NULL)
        return S_OK;

    hwndParent = NULL;
    hr = pUnkSite->QueryInterface(IID_PPV_ARG(IOleWindow, &m_site));
    if (FAILED(hr))
        return hr;

    hr = m_site->GetWindow(&hwndParent);
    if (FAILED(hr))
        return hr;

    if (!::IsWindow(hwndParent))
        return E_FAIL;

    if (m_staticToolbar != NULL)
    {
        hr = m_staticToolbar->CreateToolbar(hwndParent, m_dwFlags);
        if (FAILED(hr))
            return hr;

        hr = m_staticToolbar->FillToolbar();
        if (FAILED(hr))
            return hr;
    }

    if (m_SFToolbar != NULL)
    {
        hr = m_SFToolbar->CreateToolbar(hwndParent, m_dwFlags);
        if (FAILED(hr))
            return hr;

        hr = m_SFToolbar->FillToolbar();
        if (FAILED(hr))
            return hr;
    }

    hr = IUnknown_QueryService(m_site, SID_SMenuPopup, IID_PPV_ARG(IMenuPopup, &m_subMenuParent));
    if (FAILED(hr))
        return hr;

    CComPtr<IOleWindow> pTopLevelWindow;
    hr = IUnknown_QueryService(m_site, SID_STopLevelBrowser, IID_PPV_ARG(IOleWindow, &pTopLevelWindow));
    if (FAILED(hr))
        return hr;

    return pTopLevelWindow->GetWindow(&m_topLevelWindow);
}

HRESULT STDMETHODCALLTYPE  CMenuBand::GetSite(REFIID riid, PVOID *ppvSite)
{
    if (m_site == NULL)
        return E_FAIL;

    return m_site->QueryInterface(riid, ppvSite);
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetWindow(
    HWND *phwnd)
{
    if (m_SFToolbar != NULL)
        return m_SFToolbar->GetWindow(phwnd);

    if (m_staticToolbar != NULL)
        return m_staticToolbar->GetWindow(phwnd);

    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CMenuBand::OnPosRectChangeDB(RECT *prc)
{
    SIZE sizeStatic = { 0 };
    SIZE sizeShlFld = { 0 };
    HRESULT hr = S_OK;

    if (m_staticToolbar != NULL)
        hr = m_staticToolbar->GetIdealSize(sizeStatic);
    if (FAILED(hr))
        return hr;

    if (m_SFToolbar != NULL)
        hr = m_SFToolbar->GetIdealSize(sizeShlFld);
    if (FAILED(hr))
        return hr;

    if (m_staticToolbar == NULL && m_SFToolbar == NULL)
        return E_FAIL;

    int sy = min(prc->bottom - prc->top, sizeStatic.cy + sizeShlFld.cy);

    int syStatic = sizeStatic.cy;
    int syShlFld = sy - syStatic;

    if (m_SFToolbar)
    {
        m_SFToolbar->SetPosSize(
            prc->left, 
            prc->top, 
            prc->right - prc->left, 
            syShlFld);
    }
    if (m_staticToolbar)
    {
        m_staticToolbar->SetPosSize(
            prc->left, 
            prc->top + syShlFld, 
            prc->right - prc->left, 
            syStatic);
    }

    return S_OK;
}
HRESULT STDMETHODCALLTYPE  CMenuBand::GetBandInfo(
    DWORD dwBandID,
    DWORD dwViewMode,
    DESKBANDINFO *pdbi)
{
    SIZE sizeStatic = { 0 };
    SIZE sizeShlFld = { 0 };

    HRESULT hr = S_OK;

    if (m_staticToolbar != NULL)
        hr = m_staticToolbar->GetIdealSize(sizeStatic);
    if (FAILED(hr))
        return hr;

    if (m_SFToolbar != NULL)
        hr = m_SFToolbar->GetIdealSize(sizeShlFld);
    if (FAILED(hr))
        return hr;

    if (m_staticToolbar == NULL && m_SFToolbar == NULL)
        return E_FAIL;

    pdbi->ptMaxSize.x = max(sizeStatic.cx, sizeShlFld.cx) + 20;
    pdbi->ptMaxSize.y = sizeStatic.cy + sizeShlFld.cy;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::ShowDW(BOOL fShow)
{
    HRESULT hr = S_OK;

    if (m_staticToolbar != NULL)
        hr = m_staticToolbar->ShowWindow(fShow);
    if (FAILED(hr))
        return hr;
    if (m_SFToolbar != NULL)
        hr = m_SFToolbar->ShowWindow(fShow);
    if (FAILED(hr))
        return hr;

    if (fShow)
    {
        hr = _CallCB(SMC_INITMENU, 0, 0);
        if (FAILED(hr))
            return hr;
    }

    if (fShow)
        hr = m_focusManager->PushMenu(this);
    else
        hr = m_focusManager->PopMenu(this);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::CloseDW(DWORD dwReserved)
{
    ShowDW(FALSE);

    if (m_staticToolbar != NULL)
        return m_staticToolbar->Close();

    if (m_SFToolbar != NULL)
        return m_SFToolbar->Close();

    return S_OK;
}
HRESULT STDMETHODCALLTYPE CMenuBand::ResizeBorderDW(LPCRECT prcBorder, IUnknown *punkToolbarSite, BOOL fReserved)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::ContextSensitiveHelp(BOOL fEnterMode)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    HRESULT hr;

    hr = m_subMenuParent->SetSubMenu(this, fActivate);
    if (FAILED(hr))
        return hr;

    if (fActivate)
    {
        CComPtr<IOleWindow> pTopLevelWindow;
        hr = IUnknown_QueryService(m_site, SID_SMenuPopup, IID_PPV_ARG(IOleWindow, &pTopLevelWindow));
        if (FAILED(hr))
            return hr;

        hr = pTopLevelWindow->GetWindow(&m_topLevelWindow);
        if (FAILED(hr))
            return hr;
    }
    else
    {
        m_topLevelWindow = NULL;
    }

    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    if (!pguidCmdGroup)
        return E_FAIL;

    if (IsEqualGUID(*pguidCmdGroup, CLSID_MenuBand))
    {
        if (nCmdID == 16) // set (big) icon size
        {
            this->m_useBigIcons = nCmdexecopt == 2;
            return S_OK;
        }
        else if (nCmdID == 19) // popup-related
        {
            return S_FALSE;
        }
    }

    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    if (IsEqualIID(guidService, SID_SMenuBandChild) ||
        IsEqualIID(guidService, SID_SMenuBandBottom) ||
        IsEqualIID(guidService, SID_SMenuBandBottomSelected))
        return this->QueryInterface(riid, ppvObject);
    WARN("Unknown service requested %s\n", wine_dbgstr_guid(&guidService));
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE CMenuBand::Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::OnSelect(DWORD dwSelectType)
{
    switch (dwSelectType)
    {
    case MPOS_CHILDTRACKING:
        // TODO: Cancel timers?
        return m_subMenuParent->OnSelect(dwSelectType);
    case MPOS_SELECTLEFT:
        if (m_subMenuChild)
            m_subMenuChild->OnSelect(MPOS_CANCELLEVEL);
        return m_subMenuParent->OnSelect(dwSelectType);
    case MPOS_SELECTRIGHT:
        return m_subMenuParent->OnSelect(dwSelectType);
    case MPOS_EXECUTE:
    case MPOS_FULLCANCEL:
        if (m_subMenuChild)
            m_subMenuChild->OnSelect(dwSelectType);
        return m_subMenuParent->OnSelect(dwSelectType);
    case MPOS_CANCELLEVEL:
        if (m_subMenuChild)
            m_subMenuChild->OnSelect(dwSelectType);
        break;
    }
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetSubMenu(IMenuPopup *pmp, BOOL fSet)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetClient(IUnknown *punkClient)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetClient(IUnknown **ppunkClient)
{
    // HACK, so I can test for a submenu in the DeskBar
    //UNIMPLEMENTED;
    if (ppunkClient)
    {
        if (m_subMenuChild)
            *ppunkClient = m_subMenuChild;
        else
            *ppunkClient = NULL;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::IsMenuMessage(MSG *pmsg)
{
    //UNIMPLEMENTED;
    //return S_OK;
    return S_FALSE;
    //return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBand::TranslateMenuMessage(MSG *pmsg, LRESULT *plRet)
{
    //UNIMPLEMENTED;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetShellFolder(IShellFolder *psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags)
{
    if (m_SFToolbar == NULL)
    {
        m_SFToolbar = new CMenuSFToolbar(this);
    }

    HRESULT hr = m_SFToolbar->SetShellFolder(psf, pidlFolder, hKey, dwFlags);
    if (FAILED(hr))
        return hr;

    if (m_site)
    {
        HWND hwndParent;

        hr = m_site->GetWindow(&hwndParent);
        if (FAILED(hr))
            return hr;

        hr = m_SFToolbar->CreateToolbar(hwndParent, m_dwFlags);
        if (FAILED(hr))
            return hr;

        hr = m_SFToolbar->FillToolbar();
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetShellFolder(DWORD *pdwFlags, LPITEMIDLIST *ppidl, REFIID riid, void **ppv)
{
    if (m_SFToolbar)
        return m_SFToolbar->GetShellFolder(pdwFlags, ppidl, riid, ppv);
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CMenuBand::InvalidateItem(LPSMDATA psmd, DWORD dwFlags)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetState(LPSMDATA psmd)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetMenuToolbar(IUnknown *punk, DWORD dwFlags)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    RECT rc;
    HDC hdc;
    HBRUSH bgBrush;
    HBRUSH hotBrush;
    NMHDR * hdr;
    NMTBCUSTOMDRAW * cdraw;
    NMTBHOTITEM * hot;
    NMMOUSE * rclick;
    NMPGCALCSIZE* csize;
    TBBUTTONINFO btni;
    BOOL useFlatMenus = FALSE;
    COLORREF clrText;
    COLORREF clrTextHighlight;


    *theResult = 0;
    switch (uMsg)
    {
    case WM_COMMAND:

        if (m_staticToolbar && m_staticToolbar->IsWindowOwner(hWnd) == S_OK)
        {
            return m_staticToolbar->OnCommand(wParam, lParam, theResult);
        }

        if (m_SFToolbar && m_SFToolbar->IsWindowOwner(hWnd) == S_OK)
        {
            return m_SFToolbar->OnCommand(wParam, lParam, theResult);
        }

        return S_OK;

    case WM_NOTIFY:
        hdr = reinterpret_cast<LPNMHDR>(lParam);
        switch (hdr->code)
        {
        case PGN_CALCSIZE:
            csize = reinterpret_cast<LPNMPGCALCSIZE>(hdr);

            if (m_staticToolbar && m_staticToolbar->IsWindowOwner(hWnd) == S_OK)
            {
                SIZE tbs;
                m_staticToolbar->GetIdealSize(tbs);
                if (csize->dwFlag == PGF_CALCHEIGHT)
                {
                    csize->iHeight = tbs.cy;
                }
                else if (csize->dwFlag == PGF_CALCWIDTH)
                {
                    csize->iHeight = tbs.cx;
                }
            }

            if (m_SFToolbar && m_SFToolbar->IsWindowOwner(hWnd) == S_OK)
            {
                SIZE tbs;
                m_SFToolbar->GetIdealSize(tbs);
                if (csize->dwFlag == PGF_CALCHEIGHT)
                {
                    csize->iHeight = tbs.cy;
                }
                else if (csize->dwFlag == PGF_CALCWIDTH)
                {
                    csize->iHeight = tbs.cx;
                }
            }
            return S_OK;

        case TBN_DROPDOWN:

            if (m_staticToolbar && m_staticToolbar->IsWindowOwner(hWnd) == S_OK)
            {
                WPARAM wp = reinterpret_cast<LPNMTOOLBAR>(hdr)->iItem;
                return m_staticToolbar->OnCommand(wp, 0, theResult);
            }

            if (m_SFToolbar && m_SFToolbar->IsWindowOwner(hWnd) == S_OK)
            {
                WPARAM wp = reinterpret_cast<LPNMTOOLBAR>(hdr)->iItem;
                return m_SFToolbar->OnCommand(wp, 0, theResult);
            }

            return S_OK;

        case TBN_HOTITEMCHANGE:
            hot = reinterpret_cast<LPNMTBHOTITEM>(hdr);

            if (m_staticToolbar && m_staticToolbar->IsWindowOwner(hWnd) == S_OK)
            {
                return m_staticToolbar->OnHotItemChange(hot);
            }

            if (m_SFToolbar && m_SFToolbar->IsWindowOwner(hWnd) == S_OK)
            {
                return m_SFToolbar->OnHotItemChange(hot);
            }

            return S_OK;

        case NM_RCLICK:
            rclick = reinterpret_cast<LPNMMOUSE>(hdr);

            if (m_staticToolbar && m_staticToolbar->IsWindowOwner(hWnd) == S_OK)
            {
                return m_staticToolbar->OnContextMenu(rclick);
            }

            if (m_SFToolbar && m_SFToolbar->IsWindowOwner(hWnd) == S_OK)
            {
                return m_SFToolbar->OnContextMenu(rclick);
            }

            return S_OK;

        case NM_CUSTOMDRAW:
            cdraw = reinterpret_cast<LPNMTBCUSTOMDRAW>(hdr);
            switch (cdraw->nmcd.dwDrawStage)
            {
            case CDDS_PREPAINT:
                *theResult = CDRF_NOTIFYITEMDRAW;
                return S_OK;

            case CDDS_ITEMPREPAINT:

                SystemParametersInfo(SPI_GETFLATMENU, 0, &useFlatMenus, 0);

                clrText = GetSysColor(COLOR_MENUTEXT);
                clrTextHighlight = GetSysColor(COLOR_HIGHLIGHTTEXT);

                bgBrush = GetSysColorBrush(COLOR_MENU);
                hotBrush = GetSysColorBrush(useFlatMenus ? COLOR_MENUHILIGHT : COLOR_HIGHLIGHT);

                rc = cdraw->nmcd.rc;
                hdc = cdraw->nmcd.hdc;

                if (((INT)cdraw->nmcd.dwItemSpec == m_hotItem ||
                    (m_hotItem < 0 && (INT)cdraw->nmcd.dwItemSpec == m_popupItem)))
                {
                    cdraw->nmcd.uItemState = CDIS_HOT;
                }

                switch (cdraw->nmcd.uItemState)
                {
                case CDIS_HOT:
                case CDIS_FOCUS:
                    FillRect(hdc, &rc, hotBrush);
                    SetTextColor(hdc, clrTextHighlight);
                    cdraw->clrText = clrTextHighlight;
                    break;
                default:
                    FillRect(hdc, &rc, bgBrush);
                    SetTextColor(hdc, clrText);
                    cdraw->clrText = clrText;
                    break;
                }

                *theResult = CDRF_NOTIFYPOSTPAINT | TBCDRF_NOBACKGROUND | TBCDRF_NOEDGES | TBCDRF_NOOFFSET | TBCDRF_NOMARK | 0x00800000; // FIXME: the last bit is Vista+, for debugging only
                return S_OK;

            case CDDS_ITEMPOSTPAINT:
                btni.cbSize = sizeof(btni);
                btni.dwMask = TBIF_STYLE;
                SendMessage(hWnd, TB_GETBUTTONINFO, cdraw->nmcd.dwItemSpec, reinterpret_cast<LPARAM>(&btni));
                if (btni.fsStyle & BTNS_DROPDOWN)
                {
                    SelectObject(cdraw->nmcd.hdc, m_marlett);
                    WCHAR text [] = L"8";
                    SetBkMode(cdraw->nmcd.hdc, TRANSPARENT);
                    DrawTextEx(cdraw->nmcd.hdc, text, 1, &cdraw->nmcd.rc, DT_NOCLIP | DT_VCENTER | DT_RIGHT | DT_SINGLELINE, NULL);
                }
                *theResult = TRUE;
                return S_OK;
            }
            return S_OK;
        }
        return S_OK;
    }

    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuBand::IsWindowOwner(HWND hWnd)
{
    if (m_staticToolbar && m_staticToolbar->IsWindowOwner(hWnd) == S_OK)
        return S_OK;

    if (m_SFToolbar && m_SFToolbar->IsWindowOwner(hWnd) == S_OK)
        return S_OK;

    return S_FALSE;
}

HRESULT CMenuBand::_CallCBWithItemId(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return _CallCB(uMsg, wParam, lParam, id);
}

HRESULT CMenuBand::_CallCBWithItemPidl(LPITEMIDLIST pidl, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return _CallCB(uMsg, wParam, lParam, 0, pidl);
}

HRESULT CMenuBand::_CallCB(UINT uMsg, WPARAM wParam, LPARAM lParam, UINT id, LPITEMIDLIST pidl)
{
    if (!m_psmc)
        return S_FALSE;

    HWND hwnd;
    GetWindow(&hwnd);

    SMDATA smData = { 0 };
    smData.punk = static_cast<IShellMenu2*>(this);
    smData.uId = id;
    smData.uIdParent = m_uId;
    smData.uIdAncestor = m_uIdAncestor;
    smData.hwnd = hwnd;
    smData.pidlItem = pidl;
    if (m_staticToolbar)
    {
        smData.hmenu = m_hmenu;
    }
    smData.pvUserData = NULL;
    if (m_SFToolbar)
        m_SFToolbar->GetShellFolder(NULL, &smData.pidlFolder, IID_PPV_ARG(IShellFolder, &smData.psf));
    HRESULT hr = m_psmc->CallbackSM(&smData, uMsg, wParam, lParam);
    ILFree(smData.pidlFolder);
    if (smData.psf)
        smData.psf->Release();
    return hr;
}

HRESULT CMenuBand::_TrackSubMenuUsingTrackPopupMenu(HMENU popup, INT x, INT y)
{
    ::TrackPopupMenu(popup, 0, x, y, 0, m_menuOwner, NULL);
    return S_OK;
}

HRESULT CMenuBand::_GetTopLevelWindow(HWND*topLevel)
{
    *topLevel = m_topLevelWindow;
    return S_OK;
}

HRESULT CMenuBand::_OnHotItemChanged(CMenuToolbarBase * tb, INT id)
{
    if (m_hotBar && m_hotBar != tb)
        m_hotBar->ChangeHotItem(-1);
    m_hotBar = tb;
    m_hotItem = id;
    if (m_staticToolbar) m_staticToolbar->InvalidateDraw();
    if (m_SFToolbar) m_SFToolbar->InvalidateDraw();
    return S_OK;
}

HRESULT CMenuBand::_MenuItemHotTrack(DWORD changeType)
{
    HRESULT hr;

    if (changeType == VK_DOWN)
    {
        if (m_SFToolbar && (m_hotBar == m_SFToolbar || m_hotBar == NULL))
        {
            hr = m_SFToolbar->ChangeHotItem(VK_DOWN);
            if (hr == S_FALSE)
            {
                if (m_staticToolbar)
                    return m_staticToolbar->ChangeHotItem(VK_HOME);
                else
                    return m_SFToolbar->ChangeHotItem(VK_HOME);
            }
            return hr;
        }
        else if (m_staticToolbar && m_hotBar == m_staticToolbar)
        {
            hr = m_staticToolbar->ChangeHotItem(VK_DOWN);
            if (hr == S_FALSE)
            {
                if (m_SFToolbar)
                    return m_SFToolbar->ChangeHotItem(VK_HOME);
                else
                    return m_staticToolbar->ChangeHotItem(VK_HOME);
            }
            return hr;
        }
    }
    else if (changeType == VK_UP)
    {
        if (m_staticToolbar && (m_hotBar == m_staticToolbar || m_hotBar == NULL))
        {
            hr = m_staticToolbar->ChangeHotItem(VK_UP);
            if (hr == S_FALSE)
            {
                if (m_SFToolbar)
                    return m_SFToolbar->ChangeHotItem(VK_END);
                else
                    return m_staticToolbar->ChangeHotItem(VK_END);
            }
            return hr;
        }
        else if (m_SFToolbar && m_hotBar == m_SFToolbar)
        {
            hr = m_SFToolbar->ChangeHotItem(VK_UP);
            if (hr == S_FALSE)
            {
                if (m_staticToolbar)
                    return m_staticToolbar->ChangeHotItem(VK_END);
                else
                    return m_SFToolbar->ChangeHotItem(VK_END);
            }
            return hr;
        }
    }
    else if (changeType == MPOS_SELECTLEFT)
    {
        if (m_subMenuChild)
            m_subMenuChild->OnSelect(MPOS_CANCELLEVEL);
        return m_subMenuParent->OnSelect(MPOS_CANCELLEVEL);
    }
    else if (changeType == MPOS_SELECTRIGHT)
    {
        if (m_hotBar && m_hotItem >= 0)
        {
            if (m_hotBar->HasSubMenu(m_hotItem)==S_OK)
            {
                m_hotBar->PopupItem(m_hotItem);
                return S_FALSE;
            }
        }
        return m_subMenuParent->OnSelect(changeType);
    }
    else
    {
        return m_subMenuParent->OnSelect(changeType);
    }
    return S_OK;
}

HRESULT CMenuBand::_OnPopupSubMenu(INT popupItem, IMenuPopup * popup, POINTL * pAt, RECTL * pExclude)
{
    if (m_subMenuChild)
    {
        HRESULT hr = m_subMenuChild->OnSelect(MPOS_CANCELLEVEL);
        if (FAILED(hr))
            return hr;
    }
    m_popupItem = popupItem;
    m_subMenuChild = popup;
    if (popup)
    {
        IUnknown_SetSite(popup, m_subMenuParent);
        popup->Popup(pAt, pExclude, MPPF_RIGHT);
    }
    if (m_staticToolbar) m_staticToolbar->InvalidateDraw();
    if (m_SFToolbar) m_SFToolbar->InvalidateDraw();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetSubMenu(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetToolbar(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetMinWidth(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetNoBorder(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetTheme(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetTop(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetBottom(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetTracked(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetParentSite(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetState(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::DoDefaultAction(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::IsEmpty(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::HasFocusIO()
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::TranslateAcceleratorIO(LPMSG lpMsg)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::IsDirty()
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::Load(IStream *pStm)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::Save(IStream *pStm, BOOL fClearDirty)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetClassID(CLSID *pClassID)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    UNIMPLEMENTED;
    return S_OK;
}
