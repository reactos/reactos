/*
 * PROJECT:     shell32
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/shell32/CMenuBand.c
 * PURPOSE:     menu band implementation
 * PROGRAMMERS: Giannis Adamopoulos (gadamopoulos@reactos.org)
 */

#include "precomp.h"

#include <windowsx.h>

WINE_DEFAULT_DEBUG_CHANNEL(CMenuBand);


BOOL
AllocAndGetMenuString(HMENU hMenu, UINT ItemIDByPosition, WCHAR** String)
{
    int Length;

    Length = GetMenuStringW(hMenu, ItemIDByPosition, NULL, 0, MF_BYPOSITION);

    if(!Length)
        return FALSE;

    /* Also allocate space for the terminating NULL character */
    ++Length;
    *String = (PWSTR)HeapAlloc(GetProcessHeap(), 0, Length * sizeof(WCHAR));

    GetMenuStringW(hMenu, ItemIDByPosition, *String, Length, MF_BYPOSITION);

    return TRUE;
}

CMenuStaticToolbar::CMenuStaticToolbar(CMenuBand *menuBand)
{
    m_menuBand = menuBand;


    m_menuBand = NULL;
    m_hwnd = NULL;
    m_hmenu = NULL;
    m_hwndOwner = NULL;
    m_dwMenuFlags = NULL;
}

HRESULT  CMenuStaticToolbar::GetMenu(
    HMENU *phmenu,
    HWND *phwnd,
    DWORD *pdwFlags)
{
    *phmenu = m_hmenu;
    *phwnd = m_hwndOwner;
    *pdwFlags = m_dwMenuFlags;

    return S_OK;
}

HRESULT  CMenuStaticToolbar::SetMenu(
    HMENU hmenu,
    HWND hwnd,
    DWORD dwFlags)
{
    if (!hwnd)
        return E_FAIL;

    m_hmenu = hmenu;
    m_hwndOwner = hwnd;
    m_dwMenuFlags = dwFlags;

    return S_OK;
}

HRESULT CMenuStaticToolbar::ShowWindow(BOOL fShow)
{
    ::ShowWindow(m_hwnd, fShow ? SW_SHOW : SW_HIDE);

    return S_OK;
}

HRESULT CMenuStaticToolbar::Close()
{
    DestroyWindow(m_hwnd);
    m_hwnd = NULL;
    return S_OK;
}

HRESULT CMenuStaticToolbar::CreateToolbar(HWND hwndParent, DWORD dwFlags)
{
    HWND hwndToolbar;
    hwndToolbar = CreateWindowEx(TBSTYLE_EX_DOUBLEBUFFER, TOOLBARCLASSNAMEW, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
                WS_CLIPCHILDREN | TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT | TBSTYLE_REGISTERDROP | TBSTYLE_LIST | TBSTYLE_FLAT |
                CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_TOP, 0, 0, 500, 20, m_hwndOwner, NULL,
                _AtlBaseModule.GetModuleInstance(), 0);
    if (hwndToolbar == NULL)
        return E_FAIL;

    ::SetParent(hwndToolbar, hwndParent);

    m_hwnd = hwndToolbar;

    /* Identify the version of the used Common Controls DLL by sending the size of the TBBUTTON structure */
    SendMessageW(m_hwnd, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    if (dwFlags & SMINIT_TOPLEVEL)
    {
        /* Hide the placeholders for the button images */
        SendMessageW(m_hwnd, TB_SETIMAGELIST, 0, 0);
    }

    return S_OK;
}

HRESULT CMenuStaticToolbar::FillToolbar()
{
    TBBUTTON tbb = {0};
    int i;
    PWSTR MenuString;

    tbb.fsState = TBSTATE_ENABLED;
    tbb.fsStyle = BTNS_DROPDOWN | BTNS_AUTOSIZE;

    for(i = 0; i < GetMenuItemCount(m_hmenu); i++)
    {
        if(!AllocAndGetMenuString(m_hmenu, i, &MenuString))
            return E_OUTOFMEMORY;

        tbb.idCommand = i;
        tbb.iString = (INT_PTR)MenuString;

        SendMessageW(m_hwnd, TB_ADDBUTTONS, 1, (LPARAM)(LPTBBUTTON)&tbb);
        HeapFree(GetProcessHeap(), 0, MenuString);
    }

    return S_OK;
}

HRESULT CMenuStaticToolbar::GetWindow(HWND *phwnd)
{
    if (!phwnd)
        return E_FAIL;

    *phwnd = m_hwnd;

    return S_OK;
}

CMenuBand::CMenuBand()
{
    m_site = NULL;
    m_psmc = NULL;
    m_staticToolbar = NULL;
}

CMenuBand::~CMenuBand()
{
    if (m_site)
        m_site->Release();

    if (m_psmc)
        m_psmc->Release();

    if (m_staticToolbar)
        delete m_staticToolbar;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::Initialize(
    IShellMenuCallback *psmc,
    UINT uId,
    UINT uIdAncestor,
    DWORD dwFlags)
{
    if(m_psmc)
        m_psmc->Release();

    m_psmc = psmc;
    m_uId = uId;
    m_uIdAncestor = uIdAncestor;
    m_dwFlags = dwFlags;

    if (m_psmc)
        m_psmc->AddRef();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::GetMenuInfo(
    IShellMenuCallback **ppsmc,
    UINT *puId,
    UINT *puIdAncestor,
    DWORD *pdwFlags)
{
    *ppsmc = m_psmc;
    *puId = m_uId;
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
        m_staticToolbar = new CMenuStaticToolbar(this);

    HRESULT hResult = m_staticToolbar->SetMenu(hmenu, hwnd, dwFlags);
    if (FAILED(hResult))
        return hResult;

    if (m_site)
    {
        HWND hwndParent;

        hResult = m_site->GetWindow(&hwndParent);
        if (FAILED(hResult))
            return hResult;

        hResult = m_staticToolbar->CreateToolbar(hwndParent, m_dwFlags);
        if (FAILED(hResult))
            return hResult;

        hResult = m_staticToolbar->FillToolbar();
    }

    return hResult;
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
    HWND                    hwndParent;
    HRESULT                 hResult;

    if (m_site != NULL)
        m_site->Release();

    if (pUnkSite == NULL)
        return S_OK;

    hwndParent = NULL;
    hResult = pUnkSite->QueryInterface(IID_PPV_ARG(IOleWindow, &m_site));
    if (SUCCEEDED(hResult))
    {
        m_site->GetWindow(&hwndParent);
        m_site->Release();
    }
    if (!::IsWindow(hwndParent))
        return E_FAIL;

    if (m_staticToolbar != NULL)
    {
        m_staticToolbar->CreateToolbar(hwndParent, m_dwFlags);
        m_staticToolbar->FillToolbar();
    }

    return S_OK;
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
    if (m_staticToolbar != NULL)
        return m_staticToolbar->GetWindow(phwnd);

    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::GetBandInfo(
    DWORD dwBandID,
    DWORD dwViewMode,
    DESKBANDINFO *pdbi)
{
    SIZE size;
    HWND hwnd;
    HRESULT hResult;

    /* FIXME */
    if (m_staticToolbar == NULL)
        return E_FAIL;
    hResult = m_staticToolbar->GetWindow(&hwnd);
    if (FAILED(hResult))
        return hResult;
    if (hwnd == NULL)
        return E_FAIL;

    if (pdbi->dwMask & DBIM_MINSIZE)
    {
        SendMessageW( hwnd, TB_GETIDEALSIZE, TRUE, (LPARAM)&size);

        pdbi->ptMinSize.x = 0;
        pdbi->ptMinSize.y = size.cy;
    }
    if (pdbi->dwMask & DBIM_MAXSIZE)
    {
        SendMessageW( hwnd, TB_GETMAXSIZE, 0, (LPARAM)&size);

        pdbi->ptMaxSize.x = size.cx;
        pdbi->ptMaxSize.y = size.cy;
    }
    if (pdbi->dwMask & DBIM_INTEGRAL)
    {
        pdbi->ptIntegral.x = 0;
        pdbi->ptIntegral.y = 0;
    }
    if (pdbi->dwMask & DBIM_ACTUAL)
    {
        SendMessageW( hwnd, TB_GETIDEALSIZE, TRUE, (LPARAM)&size);
        SendMessageW( hwnd, TB_GETIDEALSIZE, FALSE, (LPARAM)&size);

        pdbi->ptActual.x = size.cx;
        pdbi->ptActual.y = size.cy;
    }
    if (pdbi->dwMask & DBIM_TITLE)
        wcscpy(pdbi->wszTitle, L"");
    if (pdbi->dwMask & DBIM_MODEFLAGS)
        pdbi->dwModeFlags = DBIMF_UNDELETEABLE;
    if (pdbi->dwMask & DBIM_BKCOLOR)
        pdbi->crBkgnd = 0;
    return S_OK;
}

/* IDockingWindow */
HRESULT STDMETHODCALLTYPE  CMenuBand::ShowDW(BOOL fShow)
{        
    if (m_staticToolbar != NULL)
        return m_staticToolbar->ShowWindow(fShow);

    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CMenuBand::CloseDW(DWORD dwReserved)
{
    ShowDW(FALSE);

    if (m_staticToolbar != NULL)
        return m_staticToolbar->Close();

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

HRESULT STDMETHODCALLTYPE CMenuBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[ ], OLECMDTEXT *pCmdText)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::Popup(POINTL *ppt,  RECTL *prcExclude, MP_POPUPFLAGS dwFlags)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::OnSelect(DWORD dwSelectType)
{
    UNIMPLEMENTED;
    return S_OK;
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
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::OnPosRectChangeDB(RECT *prc)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::IsMenuMessage(MSG *pmsg)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::TranslateMenuMessage(MSG *pmsg, LRESULT *plRet)
{
    UNIMPLEMENTED;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetShellFolder(IShellFolder *psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetShellFolder(DWORD *pdwFlags, LPITEMIDLIST *ppidl, REFIID riid, void **ppv)
{
    UNIMPLEMENTED;
    return S_OK;
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
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::IsWindowOwner(HWND hWnd)
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

