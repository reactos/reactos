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

/*
Implements the navigation band of the cabinet window
*/

#include "precomp.h"
#include <commoncontrols.h>
#include <shlwapi_undoc.h>
#include <shellapi.h>

/*
TODO:
****Add tooltip notify handler
  **Properly implement GetBandInfo
    Implement Exec
    Implement QueryService
    Implement Load
    Implement Save
*/

// Unique GUID of the DLL where this CAddressBand is implemented so we can tell if it's really us
static const GUID THISMODULE_GUID = { 0x60ebab6e, 0x2e4b, 0x42f6, { 0x8a,0xbc,0x80,0x73,0x1c,0xa6,0x42,0x02} };

CAddressBand::CAddressBand()
{
    fEditControl = NULL;
    fGoButton = NULL;
    fComboBox = NULL;
    fGoButtonShown = false;
}

CAddressBand::~CAddressBand()
{
}

BOOL CAddressBand::ShouldShowGoButton()
{
    return SHRegGetBoolUSValueW(L"Software\\Microsoft\\Internet Explorer\\Main", L"ShowGoButton", FALSE, TRUE);
}

BOOL CAddressBand::IsGoButtonVisible(IUnknown *pUnkBand)
{
    CComPtr<IAddressBand> pAB;
    IUnknown_QueryService(pUnkBand, THISMODULE_GUID, IID_PPV_ARG(IAddressBand, &pAB));
    if (pAB)
        return static_cast<CAddressBand*>(pAB.p)->fGoButtonShown;
    return ShouldShowGoButton(); // We don't know, return the global state
}

void CAddressBand::FocusChange(BOOL bFocus)
{
//    m_bFocus = bFocus;

    //Inform the input object site that the focus has changed.
    if (fSite)
    {
#if 0
        fSite->OnFocusChangeIS((IDockingWindow *)this, bFocus);
#endif
    }
}

HRESULT STDMETHODCALLTYPE CAddressBand::GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi)
{
    if (!m_hWnd || !pdbi)  return E_FAIL;  /* Verify window exists */
    if (pdbi->dwMask & DBIM_MINSIZE)
    {
        if (fGoButtonShown)
            pdbi->ptMinSize.x = 100;
        else
            pdbi->ptMinSize.x = 150;
        pdbi->ptMinSize.y = 22;
    }
    if (pdbi->dwMask & DBIM_MAXSIZE)
    {
        pdbi->ptMaxSize.x = 0;
        pdbi->ptMaxSize.y = 0;
    }
    if (pdbi->dwMask & DBIM_INTEGRAL)
    {
        pdbi->ptIntegral.x = 0;
        pdbi->ptIntegral.y = 0;
    }
    if (pdbi->dwMask & DBIM_ACTUAL)
    {
        if (fGoButtonShown)
            pdbi->ptActual.x = 100;
        else
            pdbi->ptActual.x = 150;
        pdbi->ptActual.y = 22;
    }
    if (pdbi->dwMask & DBIM_TITLE)
    {
        if (!LoadStringW(_AtlBaseModule.GetResourceInstance(), IDS_ADDRESSBANDLABEL, pdbi->wszTitle, _countof(pdbi->wszTitle)))
            return HRESULT_FROM_WIN32(GetLastError());
    }

    if (pdbi->dwMask & DBIM_MODEFLAGS)
        pdbi->dwModeFlags = DBIMF_UNDELETEABLE;
    if (pdbi->dwMask & DBIM_BKCOLOR)
        pdbi->crBkgnd = 0;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CAddressBand::SetSite(IUnknown *pUnkSite)
{
    CComPtr<IShellService>                  shellService;
    HWND                                    parentWindow;
    HWND                                    combobox;
    HRESULT                                 hResult;
    IImageList                              *piml;

    if (pUnkSite == NULL)
    {
        fSite.Release();
        return S_OK;
    }

    fSite.Release();

    hResult = pUnkSite->QueryInterface(IID_PPV_ARG(IDockingWindowSite, &fSite));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    // get window handle of parent
    parentWindow = NULL;
    hResult = IUnknown_GetWindow(fSite, &parentWindow);

    if (!::IsWindow(parentWindow))
        return E_FAIL;

    // create combo box ex
    combobox = CreateWindowEx(WS_EX_TOOLWINDOW, WC_COMBOBOXEXW, NULL, WS_CHILD | WS_VISIBLE |
        WS_CLIPCHILDREN | WS_TABSTOP | CCS_NODIVIDER | CCS_NOMOVEY | CBS_OWNERDRAWFIXED,
                    0, 0, 500, 250, parentWindow, (HMENU)IDM_TOOLBARS_ADDRESSBAR, _AtlBaseModule.GetModuleInstance(), 0);
    if (combobox == NULL)
        return E_FAIL;
    SubclassWindow(combobox);

    HRESULT hr = SHGetImageList(SHIL_SMALL, IID_PPV_ARG(IImageList, &piml));
    if (FAILED_UNEXPECTEDLY(hr))
    {
        SendMessageW(combobox, CBEM_SETIMAGELIST, 0, 0);
    }
    else
    {
        SendMessageW(combobox, CBEM_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(piml));
    }

    SendMessage(CBEM_SETEXTENDEDSTYLE,
        CBES_EX_CASESENSITIVE | CBES_EX_NOSIZELIMIT, CBES_EX_CASESENSITIVE | CBES_EX_NOSIZELIMIT);

    fEditControl = reinterpret_cast<HWND>(SendMessage(CBEM_GETEDITCONTROL, 0, 0));
    fComboBox = reinterpret_cast<HWND>(SendMessage(CBEM_GETCOMBOCONTROL, 0, 0));
    hResult = CAddressEditBox_CreateInstance(IID_PPV_ARG(IAddressEditBox, &fAddressEditBox));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    hResult = fAddressEditBox->QueryInterface(IID_PPV_ARG(IShellService, &shellService));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = fAddressEditBox->Init(combobox, fEditControl, 8, fSite /*(IAddressBand *)this*/);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = shellService->SetOwner(fSite);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    fGoButtonShown = ShouldShowGoButton();
    if (fGoButtonShown)
        CreateGoButton();

    return hResult;
}

HRESULT STDMETHODCALLTYPE CAddressBand::GetSite(REFIID riid, void **ppvSite)
{
    if (fSite == NULL)
        return E_FAIL;
    return fSite->QueryInterface(riid, ppvSite);
}

HRESULT STDMETHODCALLTYPE CAddressBand::GetWindow(HWND *lphwnd)
{
    if (lphwnd == NULL)
        return E_POINTER;
    *lphwnd = m_hWnd;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CAddressBand::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::CloseDW(DWORD dwReserved)
{
    ShowDW(FALSE);

    if (IsWindow())
        DestroyWindow();

    m_hWnd = NULL;

    CComPtr<IShellService> pservice;
    HRESULT hres = fAddressEditBox->QueryInterface(IID_PPV_ARG(IShellService, &pservice));
    if (SUCCEEDED(hres ))
        pservice->SetOwner(NULL);

    if (fAddressEditBox) fAddressEditBox.Release();
    if (fSite) fSite.Release();

    if (m_himlNormal)
        ImageList_Destroy(m_himlNormal);

    if (m_himlHot)
        ImageList_Destroy(m_himlHot);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CAddressBand::ResizeBorderDW(
    const RECT *prcBorder, IUnknown *punkToolbarSite, BOOL fReserved)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::ShowDW(BOOL fShow)
{
    if (m_hWnd)
    {
        if (fShow)
            ShowWindow(SW_SHOW);
        else
            ShowWindow(SW_HIDE);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CAddressBand::QueryStatus(
    const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText)
{
    return IUnknown_QueryStatus(fAddressEditBox, *pguidCmdGroup, cCmds, prgCmds, pCmdText);
}

HRESULT STDMETHODCALLTYPE CAddressBand::Exec(const GUID *pguidCmdGroup,
    DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    // incomplete
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::HasFocusIO()
{
    if (GetFocus() == fEditControl || SendMessage(CB_GETDROPPEDSTATE, 0, 0))
        return S_OK;
    return S_FALSE;
}

static WCHAR GetAccessKeyFromText(WCHAR chAccess, LPCWSTR pszText)
{
    for (const WCHAR *pch = pszText; *pch != UNICODE_NULL; ++pch)
    {
        if (*pch == L'&' && pch[1] == L'&')
        {
            /* Skip the first '&', the second is skipped by the for-loop */
            ++pch;
            continue;
        }
        if (*pch == L'&')
        {
            ++pch;
            chAccess = *pch;
            break;
        }
    }

    ::CharUpperBuffW(&chAccess, 1);
    return chAccess;
}

static WCHAR GetAddressBarAccessKey(WCHAR chAccess)
{
    static WCHAR s_chCache = 0;
    static LANGID s_ThreadLocale = 0;
    if (s_chCache && s_ThreadLocale == ::GetThreadLocale())
        return s_chCache;

    WCHAR szText[80];
    if (!LoadStringW(_AtlBaseModule.GetResourceInstance(), IDS_ADDRESSBANDLABEL,
                     szText, _countof(szText)))
    {
        return chAccess;
    }

    s_chCache = GetAccessKeyFromText(chAccess, szText);
    s_ThreadLocale = ::GetThreadLocale();
    return s_chCache;
}

HRESULT STDMETHODCALLTYPE CAddressBand::TranslateAcceleratorIO(LPMSG lpMsg)
{
    // Enable Address bar access key (Alt+D)
    switch (lpMsg->message)
    {
        case WM_SYSKEYDOWN:
        case WM_SYSCHAR:
        {
            WCHAR chAccess = GetAddressBarAccessKey(L'D');
            if (lpMsg->wParam == chAccess)
            {
                ::PostMessageW(fEditControl, EM_SETSEL, 0, -1);
                ::SetFocus(fEditControl);
                return S_FALSE;
            }
            break;
        }
    }

    if (lpMsg->hwnd == fEditControl)
    {
        switch (lpMsg->message)
        {
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_SYSCOMMAND:
        case WM_SYSDEADCHAR:
        case WM_SYSCHAR:
            return S_FALSE;
        }

        TranslateMessage(lpMsg);
        DispatchMessage(lpMsg);
        return S_OK;
    }
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CAddressBand::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    if (fActivate)
    {
        IUnknown_OnFocusChangeIS(fSite, static_cast<IDeskBand *>(this), fActivate);
        SetFocus();
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CAddressBand::OnWinEvent(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    CComPtr<IWinEventHandler>               winEventHandler;
    HRESULT                                 hResult;
    RECT                                    rect;

    if (theResult)
        *theResult = 0;

    switch (uMsg)
    {
        case WM_WININICHANGE:
            break;
        case WM_COMMAND:
            if (wParam == IDM_TOOLBARS_GOBUTTON)
            {
                fGoButtonShown = !IsGoButtonVisible(static_cast<IAddressBand*>(this));
                SHRegSetUSValueW(L"Software\\Microsoft\\Internet Explorer\\Main", L"ShowGoButton", REG_SZ, fGoButtonShown ? (LPVOID)L"yes" : (LPVOID)L"no", fGoButtonShown ? 8 : 6, SHREGSET_FORCE_HKCU);
                if (!fGoButton)
                    CreateGoButton();
                ::ShowWindow(fGoButton,fGoButtonShown ? SW_HIDE : SW_SHOW);
                GetWindowRect(&rect);
                SendMessage(m_hWnd,WM_SIZE,0,MAKELPARAM(rect.right-rect.left,rect.bottom-rect.top));
                // broadcast change notification to all explorer windows
            }
            break;
    }
    hResult = fAddressEditBox->QueryInterface(IID_PPV_ARG(IWinEventHandler, &winEventHandler));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    return winEventHandler->OnWinEvent(hWnd, uMsg, wParam, lParam, theResult);
}

HRESULT STDMETHODCALLTYPE CAddressBand::IsWindowOwner(HWND hWnd)
{
    CComPtr<IWinEventHandler>               winEventHandler;
    HRESULT                                 hResult;

    if (fAddressEditBox)
    {
        hResult = fAddressEditBox->QueryInterface(IID_PPV_ARG(IWinEventHandler, &winEventHandler));
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        return winEventHandler->IsWindowOwner(hWnd);
    }
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CAddressBand::FileSysChange(long param8, long paramC)
{
    CComPtr<IAddressBand>                   addressBand;
    HRESULT                                 hResult;

    hResult = fAddressEditBox->QueryInterface(IID_PPV_ARG(IAddressBand, &addressBand));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    return addressBand->FileSysChange(param8, paramC);
}

HRESULT STDMETHODCALLTYPE CAddressBand::Refresh(long param8)
{
    CComPtr<IAddressBand>                   addressBand;
    HRESULT                                 hResult;

    hResult = fAddressEditBox->QueryInterface(IID_PPV_ARG(IAddressBand, &addressBand));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    return addressBand->Refresh(param8);
}

HRESULT STDMETHODCALLTYPE CAddressBand::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    if (guidService == THISMODULE_GUID)
        return QueryInterface(riid, ppvObject);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::OnFocusChangeIS(IUnknown *punkObj, BOOL fSetFocus)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::GetClassID(CLSID *pClassID)
{
    if (pClassID == NULL)
        return E_POINTER;
    *pClassID = CLSID_SH_AddressBand;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CAddressBand::IsDirty()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::Load(IStream *pStm)
{
    // incomplete
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::Save(IStream *pStm, BOOL fClearDirty)
{
    // incomplete
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressBand::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    // incomplete
    return E_NOTIMPL;
}

LRESULT CAddressBand::OnNotifyClick(WPARAM wParam, NMHDR *notifyHeader, BOOL &bHandled)
{
    if (notifyHeader->hwndFrom == fGoButton)
    {
        fAddressEditBox->Execute(0);
    }
    return 0;
}

LRESULT CAddressBand::OnTipText(UINT idControl, NMHDR *notifyHeader, BOOL &bHandled)
{
    if (notifyHeader->hwndFrom == fGoButton)
    {
        WCHAR szText[MAX_PATH];
        WCHAR szFormat[MAX_PATH];
        LPNMTBGETINFOTIP pGIT = (LPNMTBGETINFOTIP)notifyHeader;

        if (::GetWindowTextW(fEditControl, szText, _countof(szText)))
        {
            LoadStringW(_AtlBaseModule.GetResourceInstance(), IDS_GOBUTTONTIPTEMPLATE, szFormat, _countof(szFormat));
            wnsprintf(pGIT->pszText, pGIT->cchTextMax, szFormat, szText);
        }
        else
            *pGIT->pszText = 0;
    }
    return 0;
}

LRESULT CAddressBand::OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    POINT                                   pt;
    POINT                                   ptOrig;
    HWND                                    parentWindow;
    LRESULT                                 result;

    if (fGoButtonShown == false)
    {
        bHandled = FALSE;
        return 0;
    }
    pt.x = 0;
    pt.y = 0;
    parentWindow = GetParent();
    ::MapWindowPoints(m_hWnd, parentWindow, &pt, 1);
    OffsetWindowOrgEx(reinterpret_cast<HDC>(wParam), pt.x, pt.y, &ptOrig);
    result = SendMessage(parentWindow, WM_ERASEBKGND, wParam, 0);
    SetWindowOrgEx(reinterpret_cast<HDC>(wParam), ptOrig.x, ptOrig.y, NULL);
    if (result == 0)
    {
        bHandled = FALSE;
        return 0;
    }
    return result;
}

LRESULT CAddressBand::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    RECT                                    goButtonBounds;
    RECT                                    buttonBounds;
    long                                    buttonWidth;
    long                                    buttonHeight;
    RECT                                    comboBoxBounds;
    long                                    newHeight;
    long                                    newWidth;

    if (fGoButtonShown == false)
    {
        bHandled = FALSE;
        return 0;
    }

    newHeight = HIWORD(lParam);
    newWidth = LOWORD(lParam);

    if (!fGoButton)
        CreateGoButton();

    SendMessage(fGoButton, TB_GETITEMRECT, 0, reinterpret_cast<LPARAM>(&buttonBounds));
    buttonWidth = buttonBounds.right - buttonBounds.left;
    buttonHeight = buttonBounds.bottom - buttonBounds.top;

    DefWindowProc(WM_SIZE, wParam, MAKELONG(newWidth - buttonWidth - 2, newHeight));
    ::GetWindowRect(fComboBox, &comboBoxBounds);
    ::SetWindowPos(fGoButton, NULL, newWidth - buttonWidth, (comboBoxBounds.bottom - comboBoxBounds.top - buttonHeight) / 2,
                    buttonWidth, buttonHeight, SWP_NOOWNERZORDER | SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);

    goButtonBounds.left = newWidth - buttonWidth;
    goButtonBounds.top = 0;
    goButtonBounds.right = newWidth - buttonWidth;
    goButtonBounds.bottom = newHeight;
    InvalidateRect(&goButtonBounds, TRUE);

    SendMessage(fComboBox, CB_SETDROPPEDWIDTH, 200, 0);
    return 0;
}

LRESULT CAddressBand::OnWindowPosChanging(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    RECT                                    goButtonBounds;
    RECT                                    buttonBounds;
    long                                    buttonWidth;
    long                                    buttonHeight;
    RECT                                    comboBoxBounds;
    WINDOWPOS                               positionInfoCopy;
    long                                    newHeight;
    long                                    newWidth;

    if (!fGoButtonShown)
    {
        bHandled = FALSE;
        return 0;
    }

    if (!fGoButton)
        CreateGoButton();

    positionInfoCopy = *reinterpret_cast<WINDOWPOS *>(lParam);
    newHeight = positionInfoCopy.cy;
    newWidth = positionInfoCopy.cx;

    SendMessage(fGoButton, TB_GETITEMRECT, 0, reinterpret_cast<LPARAM>(&buttonBounds));

    buttonWidth = buttonBounds.right - buttonBounds.left;
    buttonHeight = buttonBounds.bottom - buttonBounds.top;
    positionInfoCopy.cx = newWidth - 2 - buttonWidth;
    DefWindowProc(WM_WINDOWPOSCHANGING, wParam, reinterpret_cast<LPARAM>(&positionInfoCopy));
    ::GetWindowRect(fComboBox, &comboBoxBounds);
    ::SetWindowPos(fGoButton, NULL, newWidth - buttonWidth, (comboBoxBounds.bottom - comboBoxBounds.top - buttonHeight) / 2,
                    buttonWidth, buttonHeight, SWP_NOOWNERZORDER | SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);

    goButtonBounds.left = newWidth - buttonWidth;
    goButtonBounds.top = 0;
    goButtonBounds.right = newWidth - buttonWidth;
    goButtonBounds.bottom = newHeight;
    InvalidateRect(&goButtonBounds, TRUE);

    SendMessage(fComboBox, CB_SETDROPPEDWIDTH, 200, 0);
    return 0;
}

void CAddressBand::CreateGoButton()
{
    const TBBUTTON buttonInfo [] = { { 0, 1, TBSTATE_ENABLED, 0 } };
    HINSTANCE             shellInstance;

    shellInstance = _AtlBaseModule.GetResourceInstance();
    m_himlNormal = ImageList_LoadImageW(shellInstance, MAKEINTRESOURCEW(IDB_GOBUTTON_NORMAL),
                                        20, 0, RGB(255, 0, 255), IMAGE_BITMAP, LR_CREATEDIBSECTION);
    m_himlHot = ImageList_LoadImageW(shellInstance, MAKEINTRESOURCEW(IDB_GOBUTTON_HOT),
                                     20, 0, RGB(255, 0, 255), IMAGE_BITMAP, LR_CREATEDIBSECTION);

    fGoButton = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAMEW, 0, WS_CHILD | WS_CLIPSIBLINGS |
                               WS_CLIPCHILDREN | TBSTYLE_LIST | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_NODIVIDER |
                               CCS_NOPARENTALIGN | CCS_NORESIZE,
                               0, 0, 0, 0, m_hWnd, NULL, _AtlBaseModule.GetModuleInstance(), NULL);
    SendMessage(fGoButton, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessage(fGoButton, TB_SETMAXTEXTROWS, 1, 0);
    if (m_himlNormal)
        SendMessage(fGoButton, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(m_himlNormal));
    if (m_himlHot)
        SendMessage(fGoButton, TB_SETHOTIMAGELIST, 0, reinterpret_cast<LPARAM>(m_himlHot));
    SendMessage(fGoButton, TB_ADDSTRINGW,
                reinterpret_cast<WPARAM>(_AtlBaseModule.GetResourceInstance()), IDS_GOBUTTONLABEL);
    SendMessage(fGoButton, TB_ADDBUTTONSW, 1, (LPARAM) &buttonInfo);
}
