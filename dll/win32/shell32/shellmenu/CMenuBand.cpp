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
#include "shellmenu.h"
#include <windowsx.h>
#include <commoncontrols.h>
#include <shlwapi_undoc.h>

#include "CMenuBand.h"
#include "CMenuToolbars.h"
#include "CMenuFocusManager.h"

WINE_DEFAULT_DEBUG_CHANNEL(CMenuBand);

#undef UNIMPLEMENTED

#define UNIMPLEMENTED TRACE("%s is UNIMPLEMENTED!\n", __FUNCTION__)

CMenuBand::CMenuBand() :
    m_staticToolbar(NULL),
    m_SFToolbar(NULL),
    m_site(NULL),
    m_psmc(NULL),
    m_subMenuChild(NULL),
    m_subMenuParent(NULL),
    m_childBand(NULL),
    m_parentBand(NULL),
    m_hmenu(NULL),
    m_menuOwner(NULL),
    m_useBigIcons(FALSE),
    m_topLevelWindow(NULL),
    m_hotBar(NULL),
    m_hotItem(-1),
    m_popupBar(NULL),
    m_popupItem(-1),
    m_Show(FALSE),
    m_shellBottom(FALSE),
    m_trackedPopup(NULL),
    m_trackedHwnd(NULL)
{
    m_focusManager = CMenuFocusManager::AcquireManager();
}

CMenuBand::~CMenuBand()
{
    CMenuFocusManager::ReleaseManager(m_focusManager);

    delete m_staticToolbar;
    delete m_SFToolbar;

    if (m_hmenu)
        DestroyMenu(m_hmenu);
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
    {
        *ppsmc = m_psmc;
        if (*ppsmc)
            (*ppsmc)->AddRef();
    }

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
    HRESULT hr;

    TRACE("CMenuBand::SetMenu called, hmenu=%p; hwnd=%p, flags=%x\n", hmenu, hwnd, dwFlags);

    BOOL created = FALSE;

    if (m_hmenu && m_hmenu != hmenu)
    {
        DestroyMenu(m_hmenu);
        m_hmenu = NULL;
    }

    m_hmenu = hmenu;
    m_menuOwner = hwnd;

    if (m_hmenu && m_staticToolbar == NULL)
    {
        m_staticToolbar = new CMenuStaticToolbar(this);
        created = true;
    }

    if (m_staticToolbar)
    {
        hr = m_staticToolbar->SetMenu(hmenu, hwnd, dwFlags);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    if (m_site)
    {
        HWND hwndParent;

        hr = m_site->GetWindow(&hwndParent);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        if (created)
        {
            hr = m_staticToolbar->CreateToolbar(hwndParent, m_dwFlags);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            hr = m_staticToolbar->FillToolbar();
        }
        else
        {
            hr = m_staticToolbar->FillToolbar(TRUE);
        }
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
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = m_site->GetWindow(&hwndParent);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (!::IsWindow(hwndParent))
        return E_FAIL;

    if (m_staticToolbar != NULL)
    {
        hr = m_staticToolbar->CreateToolbar(hwndParent, m_dwFlags);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        hr = m_staticToolbar->FillToolbar();
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    if (m_SFToolbar != NULL)
    {
        hr = m_SFToolbar->CreateToolbar(hwndParent, m_dwFlags);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        hr = m_SFToolbar->FillToolbar();
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    hr = IUnknown_QueryService(m_site, SID_SMenuPopup, IID_PPV_ARG(IMenuPopup, &m_subMenuParent));
    if (hr != E_NOINTERFACE && FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComPtr<IOleWindow> pTopLevelWindow;
    hr = IUnknown_QueryService(m_site, SID_STopLevelBrowser, IID_PPV_ARG(IOleWindow, &pTopLevelWindow));
    if (SUCCEEDED(hr))
    {
        hr = pTopLevelWindow->GetWindow(&m_topLevelWindow);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }
    else
    {
        m_topLevelWindow = hwndParent;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::GetSite(REFIID riid, PVOID *ppvSite)
{
    if (m_site == NULL)
        return E_FAIL;

    return m_site->QueryInterface(riid, ppvSite);
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetWindow(HWND *phwnd)
{
    if (m_SFToolbar != NULL)
        return m_SFToolbar->GetWindow(phwnd);

    if (m_staticToolbar != NULL)
        return m_staticToolbar->GetWindow(phwnd);

    if (phwnd) *phwnd = NULL;

    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CMenuBand::OnPosRectChangeDB(RECT *prc)
{
    SIZE maxStatic = { 0 };
    SIZE maxShlFld = { 0 };
    HRESULT hr = S_OK;

    if (m_staticToolbar != NULL)
        hr = m_staticToolbar->GetSizes(NULL, &maxStatic, NULL);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (m_SFToolbar != NULL)
        hr = m_SFToolbar->GetSizes(NULL, &maxShlFld, NULL);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (m_staticToolbar == NULL && m_SFToolbar == NULL)
        return E_FAIL;

    int sy = min(prc->bottom - prc->top, maxStatic.cy + maxShlFld.cy);

    int syStatic = maxStatic.cy;
    int syShlFld = sy - syStatic;

    // TODO: Windows has a more complex system to decide ordering.
    // Because we only support two toolbars at once, this is enough for us.
    if (m_shellBottom)
    {
        // Static menu on top
        if (m_SFToolbar)
        {
            m_SFToolbar->SetPosSize(
                prc->left,
                prc->top + syStatic,
                prc->right - prc->left,
                syShlFld);
        }
        if (m_staticToolbar)
        {
            m_staticToolbar->SetPosSize(
                prc->left,
                prc->top,
                prc->right - prc->left,
                syStatic);
        }
    }
    else
    {
        // Folder menu on top
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
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::GetBandInfo(
    DWORD dwBandID,
    DWORD dwViewMode,
    DESKBANDINFO *pdbi)
{
    SIZE minStatic = { 0 };
    SIZE minShlFld = { 0 };
    SIZE maxStatic = { 0 };
    SIZE maxShlFld = { 0 };
    SIZE intStatic = { 0 };
    SIZE intShlFld = { 0 };

    HRESULT hr = S_OK;

    if (m_staticToolbar != NULL)
        hr = m_staticToolbar->GetSizes(&minStatic, &maxStatic, &intStatic);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (m_SFToolbar != NULL)
        hr = m_SFToolbar->GetSizes(&minShlFld, &maxShlFld, &intShlFld);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (m_staticToolbar == NULL && m_SFToolbar == NULL)
        return E_FAIL;

    if (m_dwFlags & SMINIT_VERTICAL)
    {
        pdbi->ptMinSize.x = max(minStatic.cx, minShlFld.cx) + 20;
        pdbi->ptMinSize.y = minStatic.cy + minShlFld.cy;
        pdbi->ptMaxSize.x = max(maxStatic.cx, maxShlFld.cx) + 20;
        pdbi->ptMaxSize.y = maxStatic.cy + maxShlFld.cy;
        pdbi->dwModeFlags = DBIMF_VARIABLEHEIGHT;
    }
    else
    {
        pdbi->ptMinSize.x = minStatic.cx + minShlFld.cx;
        pdbi->ptMinSize.y = max(minStatic.cy, minShlFld.cy);
        pdbi->ptMaxSize.x = maxStatic.cx + maxShlFld.cx;
        pdbi->ptMaxSize.y = max(maxStatic.cy, maxShlFld.cy);
    }
    pdbi->ptIntegral.x = max(intStatic.cx, intShlFld.cx);
    pdbi->ptIntegral.y = max(intStatic.cy, intShlFld.cy);
    pdbi->ptActual = pdbi->ptMinSize;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::ShowDW(BOOL fShow)
{
    HRESULT hr = S_OK;

    if (m_Show == fShow)
        return S_OK;

    m_Show = fShow;

    if (m_staticToolbar != NULL)
    {
        hr = m_staticToolbar->ShowDW(fShow);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    if (m_SFToolbar != NULL)
    {
        hr = m_SFToolbar->ShowDW(fShow);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    if (fShow)
    {
        hr = _CallCB(SMC_INITMENU, 0, 0);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }
    else if (m_parentBand)
    {
        m_parentBand->SetClient(NULL);
    }

    if (_IsPopup() == S_OK)
    {
        if (fShow)
            hr = m_focusManager->PushMenuPopup(this);
        else
            hr = m_focusManager->PopMenuPopup(this);
    }
    else
    {
        if (fShow)
            hr = m_focusManager->PushMenuBar(this);
        else
            hr = m_focusManager->PopMenuBar(this);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::CloseDW(DWORD dwReserved)
{
    if (m_subMenuChild)
    {
        m_subMenuChild->OnSelect(MPOS_CANCELLEVEL);
    }

    if (m_subMenuChild)
    {
        TRACE("Child object should have removed itself.\n");
    }

    ShowDW(FALSE);

    if (m_staticToolbar != NULL)
    {
        m_staticToolbar->Close();
    }

    if (m_SFToolbar != NULL)
    {
        m_SFToolbar->Close();
    }

    if (m_site) m_site.Release();
    if (m_subMenuChild) m_subMenuChild.Release();
    if (m_subMenuParent) m_subMenuParent.Release();
    if (m_childBand) m_childBand.Release();
    if (m_parentBand) m_parentBand.Release();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    HRESULT hr;

    if (m_subMenuParent)
    {
        hr = m_subMenuParent->SetSubMenu(this, fActivate);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    if (fActivate)
    {
        CComPtr<IOleWindow> pTopLevelWindow;
        hr = IUnknown_QueryService(m_site, SID_SMenuPopup, IID_PPV_ARG(IOleWindow, &pTopLevelWindow));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        hr = pTopLevelWindow->GetWindow(&m_topLevelWindow);
        if (FAILED_UNEXPECTEDLY(hr))
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
        else if (nCmdID == 5) // select an item
        {
            if (nCmdexecopt == 0) // first
            {
                _KeyboardItemChange(VK_HOME);
            }
            else // last
            {
                _KeyboardItemChange(VK_END);
            }
            return S_FALSE;
        }

        return S_FALSE;
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
    // When called from outside, this is straightforward:
    // Things that a submenu needs to know, are spread down, and
    // things that the parent needs to know, are spread up. No drama.
    // The fun is in _MenuItemSelect (internal method).
    switch (dwSelectType)
    {
    case MPOS_CHILDTRACKING:
        if (!m_subMenuParent)
            break;
        // TODO: Cancel timers?
        return m_subMenuParent->OnSelect(dwSelectType);
    case MPOS_SELECTLEFT:
        if (m_subMenuChild)
            m_subMenuChild->OnSelect(MPOS_CANCELLEVEL);
        if (!m_subMenuParent)
            break;
        return m_subMenuParent->OnSelect(dwSelectType);
    case MPOS_SELECTRIGHT:
        if (!m_subMenuParent)
            break;
        return m_subMenuParent->OnSelect(dwSelectType);
    case MPOS_EXECUTE:
    case MPOS_FULLCANCEL:
        if (m_subMenuChild)
            m_subMenuChild->OnSelect(dwSelectType);
        if (!m_subMenuParent)
            break;
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

// Used by the focus manager to update the child band pointer
HRESULT CMenuBand::_SetChildBand(CMenuBand * child)
{
    m_childBand = child;
    if (!child)
    {
        _ChangePopupItem(NULL, -1);
    }
    return S_OK;
}

// User by the focus manager to update the parent band pointer
HRESULT CMenuBand::_SetParentBand(CMenuBand * parent)
{
    m_parentBand = parent;
    return S_OK;
}

HRESULT CMenuBand::_IsPopup()
{
    return !(m_dwFlags & SMINIT_VERTICAL);
}

HRESULT CMenuBand::_IsTracking()
{
    return m_popupBar != NULL;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetClient(IUnknown *punkClient)
{
    CComPtr<IMenuPopup> child = m_subMenuChild;

    m_subMenuChild = NULL;

    if (child)
    {
        IUnknown_SetSite(child, NULL);
        child.Release();
    }

    if (!punkClient)
    {
        return S_OK;
    }

    return punkClient->QueryInterface(IID_PPV_ARG(IMenuPopup, &m_subMenuChild));
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetClient(IUnknown **ppunkClient)
{
    if (!ppunkClient)
        return E_POINTER;
    *ppunkClient = NULL;

    if (m_subMenuChild)
    {
        *ppunkClient = m_subMenuChild;
        (*ppunkClient)->AddRef();
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::IsMenuMessage(MSG *pmsg)
{
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuBand::TranslateMenuMessage(MSG *pmsg, LRESULT *plRet)
{
    if (pmsg->message == WM_ACTIVATE && _IsPopup() == S_FALSE)
    {
        if (m_staticToolbar)
            m_staticToolbar->Invalidate();
        if (m_SFToolbar)
            m_SFToolbar->Invalidate();
    }

    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetShellFolder(IShellFolder *psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags)
{
    if (!psf)
        return E_INVALIDARG;

    if (m_SFToolbar == NULL)
    {
        m_SFToolbar = new CMenuSFToolbar(this);
    }

    HRESULT hr = m_SFToolbar->SetShellFolder(psf, pidlFolder, hKey, dwFlags);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    m_shellBottom = (dwFlags & SMSET_BOTTOM) != 0;

    if (m_site)
    {
        HWND hwndParent;

        hr = m_site->GetWindow(&hwndParent);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        hr = m_SFToolbar->CreateToolbar(hwndParent, m_dwFlags);
        if (FAILED_UNEXPECTEDLY(hr))
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

HRESULT STDMETHODCALLTYPE CMenuBand::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    if (theResult)
        *theResult = 0;

    if (uMsg == WM_WININICHANGE && wParam == SPI_SETFLATMENU)
    {
        BOOL bFlatMenus;
        SystemParametersInfo(SPI_GETFLATMENU, 0, &bFlatMenus, 0);
        AdjustForTheme(bFlatMenus);

        if (m_staticToolbar)
            m_staticToolbar->OnWinEvent(hWnd, uMsg, wParam, lParam, theResult);

        if (m_SFToolbar)
            m_SFToolbar->OnWinEvent(hWnd, uMsg, wParam, lParam, theResult);

        return S_OK;
    }

    if (m_staticToolbar && m_staticToolbar->IsWindowOwner(hWnd) == S_OK)
    {
        return m_staticToolbar->OnWinEvent(hWnd, uMsg, wParam, lParam, theResult);
    }

    if (m_SFToolbar && m_SFToolbar->IsWindowOwner(hWnd) == S_OK)
    {
        return m_SFToolbar->OnWinEvent(hWnd, uMsg, wParam, lParam, theResult);
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

    SMDATA smData = { 0 };
    smData.punk = static_cast<IShellMenu2*>(this);
    smData.uId = id;
    smData.uIdParent = m_uId;
    smData.uIdAncestor = m_uIdAncestor;
    smData.pidlItem = pidl;
    smData.hwnd = m_menuOwner ? m_menuOwner : m_topLevelWindow;
    smData.hmenu = m_hmenu;
    if (m_SFToolbar)
        m_SFToolbar->GetShellFolder(NULL, &smData.pidlFolder, IID_PPV_ARG(IShellFolder, &smData.psf));
    HRESULT hr = m_psmc->CallbackSM(&smData, uMsg, wParam, lParam);
    ILFree(smData.pidlFolder);
    if (smData.psf)
        smData.psf->Release();
    return hr;
}

HRESULT CMenuBand::_TrackSubMenu(HMENU popup, INT x, INT y, RECT& rcExclude)
{
    TPMPARAMS params = { sizeof(TPMPARAMS), rcExclude };
    UINT      flags  = TPM_VERPOSANIMATION | TPM_VERTICAL | TPM_LEFTALIGN;
    HWND      hwnd   = m_menuOwner ? m_menuOwner : m_topLevelWindow;

    m_trackedPopup = popup;
    m_trackedHwnd = hwnd;

    m_focusManager->PushTrackedPopup(popup);
    ::TrackPopupMenuEx(popup, flags, x, y, hwnd, &params);
    m_focusManager->PopTrackedPopup(popup);

    m_trackedPopup = NULL;
    m_trackedHwnd = NULL;

    _DisableMouseTrack(FALSE);

    return S_OK;
}

HRESULT CMenuBand::_TrackContextMenu(IContextMenu * contextMenu, INT x, INT y)
{
    HRESULT hr;
    UINT uCommand;

    // Ensure that the menu doesn't disappear on us
    CComPtr<IContextMenu> ctxMenu = contextMenu;

    HMENU popup = CreatePopupMenu();

    if (popup == NULL)
        return E_FAIL;

    TRACE("Before Query\n");
    UINT cmf = CMF_NORMAL;
    if (GetKeyState(VK_SHIFT) < 0)
        cmf |= CMF_EXTENDEDVERBS;

    const UINT idCmdFirst = 100, idCmdLast = 0xffff;
    hr = contextMenu->QueryContextMenu(popup, 0, idCmdFirst, idCmdLast, cmf);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        TRACE("Query failed\n");
        DestroyMenu(popup);
        return hr;
    }

    HWND hwnd = m_menuOwner ? m_menuOwner : m_topLevelWindow;

    m_focusManager->PushTrackedPopup(popup);

    TRACE("Before Tracking\n");
    uCommand = ::TrackPopupMenuEx(popup, TPM_RETURNCMD, x, y, hwnd, NULL);

    m_focusManager->PopTrackedPopup(popup);

    if (uCommand != 0)
    {
        _MenuItemSelect(MPOS_FULLCANCEL);

        TRACE("Before InvokeCommand\n");
        CMINVOKECOMMANDINFO cmi = { sizeof(cmi), 0, hwnd };
        cmi.lpVerb = MAKEINTRESOURCEA(uCommand - idCmdFirst);
        if (GetKeyState(VK_SHIFT) < 0)
            cmi.fMask |= CMIC_MASK_SHIFT_DOWN;
        if (GetKeyState(VK_CONTROL) < 0)
            cmi.fMask |= CMIC_MASK_CONTROL_DOWN;
        cmi.nShow = SW_SHOW;
        hr = contextMenu->InvokeCommand(&cmi);
        TRACE("InvokeCommand returned hr=%08x\n", hr);
    }
    else
    {
        TRACE("TrackPopupMenu failed. Code=%d, LastError=%d\n", uCommand, GetLastError());
        hr = S_FALSE;
    }

    DestroyMenu(popup);
    return hr;
}

HRESULT CMenuBand::_GetTopLevelWindow(HWND*topLevel)
{
    *topLevel = m_topLevelWindow;
    return S_OK;
}

HRESULT CMenuBand::_ChangeHotItem(CMenuToolbarBase * tb, INT id, DWORD dwFlags)
{
    if (m_hotBar == tb && m_hotItem == id)
        return S_FALSE;

    TRACE("Hot item changed from %p %p, to %p %p\n", m_hotBar, m_hotItem, tb, id);

    _KillPopupTimers();

    m_hotBar = tb;
    m_hotItem = id;
    if (m_staticToolbar) m_staticToolbar->ChangeHotItem(tb, id, dwFlags);
    if (m_SFToolbar) m_SFToolbar->ChangeHotItem(tb, id, dwFlags);

    _MenuItemSelect(MPOS_CHILDTRACKING);

    return S_OK;
}

HRESULT CMenuBand::_ChangePopupItem(CMenuToolbarBase * tb, INT id)
{
    TRACE("Popup item changed from %p %p, to %p %p\n", m_popupBar, m_popupItem, tb, id);

    m_popupBar = tb;
    m_popupItem = id;
    if (m_staticToolbar) m_staticToolbar->ChangePopupItem(tb, id);
    if (m_SFToolbar) m_SFToolbar->ChangePopupItem(tb, id);

    return S_OK;
}

HRESULT  CMenuBand::_KeyboardItemChange(DWORD change)
{
    HRESULT hr;
    CMenuToolbarBase *tb = m_hotBar;

    if (!tb)
    {
        // If no hot item was selected choose the appropriate toolbar
        if (change == VK_UP || change == VK_END)
        {
            if (m_staticToolbar)
                tb = m_staticToolbar;
            else
                tb = m_SFToolbar;
        }
        else if (change == VK_DOWN || change == VK_HOME)
        {
            if (m_SFToolbar)
                tb = m_SFToolbar;
            else
                tb = m_staticToolbar;
        }
    }

    // Ask the first toolbar to change
    hr = tb->KeyboardItemChange(change);

    if (hr != S_FALSE)
        return hr;

    // Select the second toolbar based on the first
    if (tb == m_SFToolbar && m_staticToolbar)
        tb = m_staticToolbar;
    else if (m_SFToolbar)
        tb = m_SFToolbar;

    if (!tb)
        return hr;

    // Ask the second toolbar to change
    return tb->KeyboardItemChange(change == VK_DOWN ? VK_HOME : VK_END);
}

HRESULT CMenuBand::_MenuItemSelect(DWORD changeType)
{
    // Needed to prevent the this point from vanishing mid-function
    CComPtr<CMenuBand> safeThis = this;
    HRESULT hr;

    if (m_dwFlags & SMINIT_VERTICAL)
    {
        switch (changeType)
        {
        case VK_UP:
        case VK_DOWN:
            return _KeyboardItemChange(changeType);

            // TODO: Left/Right across multi-column menus, if they ever work.
        case VK_LEFT:
            changeType = MPOS_SELECTLEFT;
            break;
        case VK_RIGHT:
            changeType = MPOS_SELECTRIGHT;
            break;
        }
    }
    else
    {
        // In horizontal menubars, left/right are equivalent to vertical's up/down
        switch (changeType)
        {
        case VK_LEFT:
            hr = _KeyboardItemChange(VK_UP);
            if (hr != S_FALSE)
                return hr;
        case VK_RIGHT:
            hr = _KeyboardItemChange(VK_DOWN);
            if (hr != S_FALSE)
                return hr;
        }
    }

    // In this context, the parent is the CMenuDeskBar, so when it bubbles upward,
    // it is notifying the deskbar, and not the the higher-level menu.
    // Same for the child: since it points to a CMenuDeskBar, it's not just recursing.
    switch (changeType)
    {
    case MPOS_EXECUTE:
    {
        CMenuToolbarBase * tb = m_hotBar;
        int item = m_hotItem;
        tb->PrepareExecuteItem(item);
        if (m_subMenuParent)
        {
            m_subMenuParent->OnSelect(changeType);
        }
        TRACE("Menu closed, executing item...\n");
        tb->ExecuteItem();
        break;
    }
    case MPOS_SELECTLEFT:
        if (m_parentBand && m_parentBand->_IsPopup()==S_FALSE)
            return m_parentBand->_MenuItemSelect(VK_LEFT);
        if (m_subMenuChild)
            return m_subMenuChild->OnSelect(MPOS_CANCELLEVEL);
        if (!m_subMenuParent)
            return S_OK;
        return m_subMenuParent->OnSelect(MPOS_CANCELLEVEL);

    case MPOS_SELECTRIGHT:
        if (m_hotBar && m_hotItem >= 0 && m_hotBar->PopupItem(m_hotItem, TRUE) == S_OK)
            return S_FALSE;
        if (m_parentBand)
            return m_parentBand->_MenuItemSelect(VK_RIGHT);
        if (!m_subMenuParent)
            return S_OK;
        return m_subMenuParent->OnSelect(MPOS_SELECTRIGHT);

    default:
        if (!m_subMenuParent)
            return S_OK;
        return m_subMenuParent->OnSelect(changeType);
    }

    return S_OK;
}

HRESULT CMenuBand::_CancelCurrentPopup()
{
    if (m_subMenuChild)
    {
        HRESULT hr = m_subMenuChild->OnSelect(MPOS_CANCELLEVEL);
        return hr;
    }

    if (m_trackedPopup)
    {
        ::SendMessage(m_trackedHwnd, WM_CANCELMODE, 0, 0);
        return S_OK;
    }

    return S_FALSE;
}

HRESULT CMenuBand::_OnPopupSubMenu(IShellMenu * childShellMenu, POINTL * pAt, RECTL * pExclude, BOOL keyInitiated)
{
    HRESULT hr = 0;
    CComPtr<IBandSite> pBandSite;
    CComPtr<IDeskBar> pDeskBar;

    // Create the necessary objects
    hr = CMenuSite_CreateInstance(IID_PPV_ARG(IBandSite, &pBandSite));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = CMenuDeskBar_CreateInstance(IID_PPV_ARG(IDeskBar, &pDeskBar));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pDeskBar->SetClient(pBandSite);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pBandSite->AddBand(childShellMenu);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    //
    CComPtr<IMenuPopup> popup;
    hr = pDeskBar->QueryInterface(IID_PPV_ARG(IMenuPopup, &popup));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    m_subMenuChild = popup;

    if (m_subMenuParent)
        IUnknown_SetSite(popup, m_subMenuParent);
    else
        IUnknown_SetSite(popup, m_site);

    DWORD flags = MPPF_RIGHT;

    if (keyInitiated && m_dwFlags & SMINIT_VERTICAL)
        flags |= MPPF_INITIALSELECT;

    popup->Popup(pAt, pExclude, flags);

    return S_OK;
}

HRESULT CMenuBand::_BeforeCancelPopup()
{
    if (m_staticToolbar)
        m_staticToolbar->BeforeCancelPopup();
    if (m_SFToolbar)
        m_SFToolbar->BeforeCancelPopup();
    return S_OK;
}

HRESULT CMenuBand::_DisableMouseTrack(BOOL bDisable)
{
    if (m_staticToolbar)
        m_staticToolbar->DisableMouseTrack(bDisable);
    if (m_SFToolbar)
        m_SFToolbar->DisableMouseTrack(bDisable);
    return S_OK;
}

HRESULT CMenuBand::_KillPopupTimers()
{
    HRESULT hr = S_OK;
    if (m_staticToolbar)
        hr = m_staticToolbar->KillPopupTimer();
    if (FAILED(hr))
        return hr;

    if (m_SFToolbar)
        hr = m_SFToolbar->KillPopupTimer();

    return hr;
}

HRESULT CMenuBand::_MenuBarMouseDown(HWND hwnd, INT item, BOOL isLButton)
{
    if (m_staticToolbar && m_staticToolbar->IsWindowOwner(hwnd) == S_OK)
        m_staticToolbar->MenuBarMouseDown(item, isLButton);
    if (m_SFToolbar && m_SFToolbar->IsWindowOwner(hwnd) == S_OK)
        m_SFToolbar->MenuBarMouseDown(item, isLButton);
    return S_OK;
}

HRESULT CMenuBand::_MenuBarMouseUp(HWND hwnd, INT item, BOOL isLButton)
{
    if (m_staticToolbar && m_staticToolbar->IsWindowOwner(hwnd) == S_OK)
        m_staticToolbar->MenuBarMouseUp(item, isLButton);
    if (m_SFToolbar && m_SFToolbar->IsWindowOwner(hwnd) == S_OK)
        m_SFToolbar->MenuBarMouseUp(item, isLButton);
    return S_OK;
}

HRESULT CMenuBand::_HasSubMenu()
{
    return m_popupBar ? S_OK : S_FALSE;
}

HRESULT CMenuBand::AdjustForTheme(BOOL bFlatStyle)
{
    return IUnknown_QueryServiceExec(m_site, SID_SMenuPopup, &CGID_MenuDeskBar, 4, bFlatStyle, NULL, NULL);
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
    if (m_popupBar)
        return S_OK;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuBand::TranslateAcceleratorIO(LPMSG lpMsg)
{
    // TODO: Alt down -> toggle menu focus
    return S_FALSE;
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

HRESULT STDMETHODCALLTYPE CMenuBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
{
    UNIMPLEMENTED;
    return S_OK;
}

extern "C"
HRESULT WINAPI RSHELL_CMenuBand_CreateInstance(REFIID riid, LPVOID *ppv)
{
    return ShellObjectCreator<CMenuBand>(riid, ppv);
}
