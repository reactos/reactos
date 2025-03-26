/*
 * PROJECT:     ReactOS msctfime.ime
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     User Interface of msctfime.ime
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "msctfime.h"

WINE_DEFAULT_DEBUG_CHANNEL(msctfime);

/***********************************************************************/

UINT WM_MSIME_SERVICE = 0;
UINT WM_MSIME_UIREADY = 0;
UINT WM_MSIME_RECONVERTREQUEST = 0;
UINT WM_MSIME_RECONVERT = 0;
UINT WM_MSIME_DOCUMENTFEED = 0;
UINT WM_MSIME_QUERYPOSITION = 0;
UINT WM_MSIME_MODEBIAS = 0;
UINT WM_MSIME_SHOWIMEPAD = 0;
UINT WM_MSIME_MOUSE = 0;
UINT WM_MSIME_KEYMAP = 0;

/// @implemented
BOOL IsMsImeMessage(_In_ UINT uMsg)
{
    return (uMsg == WM_MSIME_SERVICE ||
            uMsg == WM_MSIME_UIREADY ||
            uMsg == WM_MSIME_RECONVERTREQUEST ||
            uMsg == WM_MSIME_RECONVERT ||
            uMsg == WM_MSIME_DOCUMENTFEED ||
            uMsg == WM_MSIME_QUERYPOSITION ||
            uMsg == WM_MSIME_MODEBIAS ||
            uMsg == WM_MSIME_SHOWIMEPAD ||
            uMsg == WM_MSIME_MOUSE ||
            uMsg == WM_MSIME_KEYMAP);
}

/// @implemented
BOOL RegisterMSIMEMessage(VOID)
{
    // Using ANSI (A) version here can reduce binary size.
    WM_MSIME_SERVICE = RegisterWindowMessageA("MSIMEService");
    WM_MSIME_UIREADY = RegisterWindowMessageA("MSIMEUIReady");
    WM_MSIME_RECONVERTREQUEST = RegisterWindowMessageA("MSIMEReconvertRequest");
    WM_MSIME_RECONVERT = RegisterWindowMessageA("MSIMEReconvert");
    WM_MSIME_DOCUMENTFEED = RegisterWindowMessageA("MSIMEDocumentFeed");
    WM_MSIME_QUERYPOSITION = RegisterWindowMessageA("MSIMEQueryPosition");
    WM_MSIME_MODEBIAS = RegisterWindowMessageA("MSIMEModeBias");
    WM_MSIME_SHOWIMEPAD = RegisterWindowMessageA("MSIMEShowImePad");
    WM_MSIME_MOUSE = RegisterWindowMessageA("MSIMEMouseOperation");
    WM_MSIME_KEYMAP = RegisterWindowMessageA("MSIMEKeyMap");
    return (WM_MSIME_SERVICE &&
            WM_MSIME_UIREADY &&
            WM_MSIME_RECONVERTREQUEST &&
            WM_MSIME_RECONVERT &&
            WM_MSIME_DOCUMENTFEED &&
            WM_MSIME_QUERYPOSITION &&
            WM_MSIME_MODEBIAS &&
            WM_MSIME_SHOWIMEPAD &&
            WM_MSIME_MOUSE &&
            WM_MSIME_KEYMAP);
}

/***********************************************************************/

/// @implemented
CDefCompFrameGripper::CDefCompFrameGripper(
    CDefCompFrameWindow *pDefCompFrameWindow,
    LPCRECT prc,
    DWORD style) : CUIFGripper(pDefCompFrameWindow, prc, style)
{
    m_pDefCompFrameWindow = pDefCompFrameWindow;
}

/***********************************************************************/

/// @implemented
CCompFinalizeButton::CCompFinalizeButton(
    CCompFrameWindow *pParent,
    DWORD nObjectID,
    LPCRECT prc,
    DWORD style,
    DWORD dwButtonFlags,
    LPCWSTR pszText)
    : CUIFToolbarButton(pParent, nObjectID, prc, style, dwButtonFlags, pszText)
{
    m_pCompFrameWindow = pParent;
}

/// @implemented
CCompFinalizeButton::~CCompFinalizeButton()
{
    HICON hIcon = CUIFToolbarButton::GetIcon();
    if (hIcon)
    {
        ::DestroyIcon(hIcon);
        CUIFToolbarButton::SetIcon(NULL);
    }
}

/// @implemented
void CCompFinalizeButton::OnLeftClick()
{
    HIMC hIMC = m_pCompFrameWindow->m_hIMC;
    if (hIMC)
        ::ImmNotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
}

/***********************************************************************/

/// @implemented
CCompFrameWindow::CCompFrameWindow(HIMC hIMC, DWORD style)
    : CUIFWindow(g_hInst, style)
{
    m_hIMC = hIMC;
}

/***********************************************************************/

/// @implemented
CCompButtonFrameWindow::CCompButtonFrameWindow(HIMC hIMC, DWORD style)
    : CCompFrameWindow(hIMC, style)
{
}

/// @implemented
void CCompButtonFrameWindow::Init()
{
    if (m_pFinalizeButton)
        return;

    RECT rc = { 0, 0, 0, 0 };
    m_pFinalizeButton = new(cicNoThrow) CCompFinalizeButton(this, 0, &rc, 0, 0x10000, NULL);

    m_pFinalizeButton->Initialize();
    m_pFinalizeButton->Init();

    HICON hIcon = (HICON)::LoadImageW(g_hInst, MAKEINTRESOURCEW(IDI_DOWN), IMAGE_ICON, 16, 16, 0);
    m_pFinalizeButton->SetIcon(hIcon);

    WCHAR szText[256];
    LoadStringW(g_hInst, IDS_FINALIZE_STRING, szText, _countof(szText));
    m_pFinalizeButton->SetToolTip(szText);

    AddUIObj(m_pFinalizeButton);
}

/// @implemented
void CCompButtonFrameWindow::MoveShow(LONG x, LONG y, BOOL bShow)
{
    INT nWidth = m_Margins.cxRightWidth + m_Margins.cxLeftWidth + 18;
    INT nHeight = m_Margins.cyBottomHeight + m_Margins.cyTopHeight + 18;
    Move(x, y, nWidth + 4, nHeight + 4);

    if (m_pFinalizeButton)
    {
        RECT rc = { 1, 1, nWidth + 1, nHeight + 1 };
        m_pFinalizeButton->SetRect(&rc);
    }

    Show(bShow);
}

/// @implemented
STDMETHODIMP_(void) CCompButtonFrameWindow::OnCreate(HWND hWnd)
{
    ::SetWindowTheme(hWnd, L"TOOLBAR", NULL);

    ZeroMemory(&m_Margins, sizeof(m_Margins));

    CUIFTheme theme;
    theme.m_hTheme = NULL;
    theme.m_iPartId = 1;
    theme.m_iStateId = 0;
    theme.m_pszClassList = L"TOOLBAR";
    if (SUCCEEDED(theme.InternalOpenThemeData(hWnd)))
        theme.GetThemeMargins(NULL, 1, 3602, NULL, &m_Margins);
}

/***********************************************************************/

/// @implemented
CDefCompFrameWindow::CDefCompFrameWindow(HIMC hIMC, DWORD style)
    : CCompFrameWindow(hIMC, style)
{
    LoadPosition();
    m_iPartId = 1;
    m_iStateId = 1;
    m_pszClassList = L"TASKBAR";
}

/// @implemented
CDefCompFrameWindow::~CDefCompFrameWindow()
{
    SavePosition();
}

/// @implemented
void CDefCompFrameWindow::Init()
{
    RECT rc;

    if (!m_pGripper)
    {
        ZeroMemory(&rc, sizeof(rc));
        m_pGripper = new(cicNoThrow) CDefCompFrameGripper(this, &rc, 0);
        m_pGripper->Initialize();
        AddUIObj(m_pGripper);
    }

    if (!m_pFinalizeButton)
    {
        ZeroMemory(&rc, sizeof(rc));
        m_pFinalizeButton = new(cicNoThrow) CCompFinalizeButton(this, 0, &rc, 0, 0x10000, NULL);
        m_pFinalizeButton->Initialize();
        m_pFinalizeButton->Init();

        HICON hIcon = (HICON)LoadImageW(g_hInst, MAKEINTRESOURCEW(IDI_DOWN), IMAGE_ICON, 16, 16, 0);
        m_pFinalizeButton->SetIcon(hIcon);

        WCHAR szText[256];
        ::LoadStringW(g_hInst, IDS_FINALIZE_STRING, szText, _countof(szText));
        SetToolTip(szText);

        AddUIObj(m_pFinalizeButton);
    }
}

/// @implemented
INT CDefCompFrameWindow::GetGripperWidth()
{
    if (!m_pGripper || FAILED(m_pGripper->EnsureThemeData(m_hWnd)))
        return 5;

    INT ret = -1;
    HDC hDC = ::GetDC(m_hWnd);
    SIZE partSize;
    if (SUCCEEDED(m_pGripper->GetThemePartSize(hDC, 1, 0, TS_TRUE, &partSize)))
        ret = partSize.cx + 4;

    ::ReleaseDC(m_hWnd, hDC);

    return ((ret < 0) ? 5 : ret);
}

/// @implemented
void CDefCompFrameWindow::MyScreenToClient(LPPOINT ppt, LPRECT prc)
{
    if (ppt)
        ::ScreenToClient(m_hWnd, ppt);

    if (prc)
    {
        ::ScreenToClient(m_hWnd, (LPPOINT)prc);
        ::ScreenToClient(m_hWnd, (LPPOINT)&prc->right);
    }
}

/// @implemented
void CDefCompFrameWindow::SetCompStrRect(INT nWidth, INT nHeight, BOOL bShow)
{
    INT GripperWidth = GetGripperWidth();

    RECT rc;
    ::GetWindowRect(m_hWnd, &rc);

    Move(rc.left, rc.top, GripperWidth + nWidth + 24, nHeight + 10);

    if (m_pGripper)
    {
        rc = { 2, 3, GripperWidth + 2, nHeight + 7 };
        m_pGripper->SetRect(&rc);
    }

    if (m_pFinalizeButton)
    {
        rc = {
            GripperWidth + nWidth + 4,
            3,
            m_Margins.cxLeftWidth + m_Margins.cxRightWidth + GripperWidth + nWidth + 22,
            m_Margins.cyBottomHeight + m_Margins.cyTopHeight + 21
        };
        m_pFinalizeButton->SetRect(&rc);
    }

    Show(bShow);

    ::MoveWindow(m_hwndCompStr, GripperWidth + 2, 7, nWidth, nHeight, TRUE);
    ::ShowWindow(m_hwndCompStr, (bShow ? SW_SHOWNOACTIVATE : SW_HIDE));
}

/// @implemented
void CDefCompFrameWindow::LoadPosition()
{
    DWORD x = 0, y = 0;

    LSTATUS error;
    CicRegKey regKey;
    error = regKey.Open(HKEY_CURRENT_USER,
                        TEXT("SOFTWARE\\Microsoft\\CTF\\CUAS\\DefaultCompositionWindow"));
    if (error == ERROR_SUCCESS)
    {
        regKey.QueryDword(TEXT("Left"), &x);
        regKey.QueryDword(TEXT("Top"), &y);
    }

    Move(x, y, 0, 0);
}

/// @implemented
void CDefCompFrameWindow::SavePosition()
{
    LSTATUS error;
    CicRegKey regKey;
    error = regKey.Create(HKEY_CURRENT_USER,
                          TEXT("SOFTWARE\\Microsoft\\CTF\\CUAS\\DefaultCompositionWindow"));
    if (error == ERROR_SUCCESS)
    {
        regKey.SetDword(TEXT("Left"), m_nLeft);
        regKey.SetDword(TEXT("Top"), m_nTop);
    }
}

/// @implemented
STDMETHODIMP_(void) CDefCompFrameWindow::OnCreate(HWND hWnd)
{
    ::SetWindowTheme(hWnd, L"TASKBAR", NULL);

    ZeroMemory(&m_Margins, sizeof(m_Margins));

    CUIFTheme theme;
    theme.m_hTheme = NULL;
    theme.m_iPartId = 1;
    theme.m_iStateId = 0;
    theme.m_pszClassList = L"TOOLBAR";
    if (SUCCEEDED(theme.InternalOpenThemeData(hWnd)))
        GetThemeMargins(NULL, 1, 3602, NULL, &m_Margins);
}

/// @implemented
STDMETHODIMP_(BOOL) CDefCompFrameWindow::OnSetCursor(UINT uMsg, LONG x, LONG y)
{
    if (!::IsWindow(m_hwndCompStr))
        return FALSE;

    RECT rc;
    ::GetWindowRect(m_hwndCompStr, &rc);
    MyScreenToClient(NULL, &rc);
    POINT pt = { x, y };
    return ::PtInRect(&rc, pt);
}

/// @implemented
STDMETHODIMP_(LRESULT)
CDefCompFrameWindow::OnWindowPosChanged(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CicIMCLock imcLock(m_hIMC);
    if (SUCCEEDED(imcLock.m_hr))
        ::SendMessage(imcLock.get().hWnd, WM_IME_NOTIFY, 0xF, (LPARAM)m_hIMC);
    return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/// @implemented
STDMETHODIMP_(void) CDefCompFrameWindow::HandleMouseMsg(UINT uMsg, LONG x, LONG y)
{
    if (::IsWindow(m_hwndCompStr))
    {
        RECT rc;
        ::GetWindowRect(m_hwndCompStr, &rc);
        MyScreenToClient(NULL, &rc);

        POINT pt = { x, y };
        if (::PtInRect(&rc, pt))
            ::SendMessage(m_hwndCompStr, 0x7E8, 0, 0);
    }

    CUIFWindow::HandleMouseMsg(uMsg, x, y);
}

/***********************************************************************/

/// @implemented
POLYTEXTW *CPolyText::GetPolyAt(INT iItem)
{
    return &m_PolyTextArray[iItem];
}

/// @implemented
HRESULT CPolyText::ShiftPolyText(INT xDelta, INT yDelta)
{
    for (size_t iItem = 0; iItem < m_PolyTextArray.size(); ++iItem)
    {
        POLYTEXTW *pPolyText = &m_PolyTextArray[iItem];
        pPolyText->x += xDelta;
        pPolyText->y += yDelta;
        ::OffsetRect((LPRECT)&pPolyText->rcl, xDelta, yDelta);
    }
    return S_OK;
}

/// @implemented
HRESULT CPolyText::RemoveLastLine(BOOL bHorizontal)
{
    size_t iItem, cItems = m_PolyTextArray.size();
    if (!cItems)
        return E_FAIL;

    POLYTEXTW *pData1 = &m_PolyTextArray[cItems - 1];

    for (iItem = 0; iItem < cItems; ++iItem)
    {
        POLYTEXTW *pData2 = &m_PolyTextArray[iItem];
        if (bHorizontal)
        {
            if (pData1->x == pData2->x)
                break;
        }
        else
        {
            if (pData1->y == pData2->y)
                break;
        }
    }

    if (iItem >= cItems)
        return E_FAIL;

    m_PolyTextArray.Remove(iItem, cItems - iItem);
    m_ValueArray.Remove(iItem, cItems - iItem);
    return S_OK;
}

/// @implemented
void CPolyText::RemoveAll()
{
    m_PolyTextArray.clear();
    m_ValueArray.clear();
}

/***********************************************************************/

/// @implemented
void COMPWND::_ClientToScreen(LPRECT prc)
{
    ::ClientToScreen(m_hWnd, (LPPOINT)prc);
    ::ClientToScreen(m_hWnd, (LPPOINT)&prc->right);
}

/***********************************************************************/

// For GetWindowLongPtr/SetWindowLongPtr
#define UICOMP_GWLP_INDEX 0
#define UICOMP_GWLP_SIZE  (UICOMP_GWLP_INDEX + sizeof(INT))

/// @unimplemented
UIComposition::UIComposition(HWND hwndParent)
{
}

/// @implemented
UIComposition::~UIComposition()
{
    DestroyCompositionWindow();

    if (m_hFont1)
    {
        ::DeleteObject(m_hFont1);
        m_hFont1 = NULL;
    }

    if (m_hFont2)
    {
        ::DeleteObject(m_hFont2);
        m_hFont2 = NULL;
    }

    if (m_strCompStr)
    {
        cicMemFree(m_strCompStr);
        m_strCompStr = NULL;
    }

    m_cchCompStr = 0;
}

// @implemented
BOOL UIComposition::SendMessageToUI(CicIMCLock& imcLock, WPARAM wParam, LPARAM lParam)
{
    HWND hImeWnd = ImmGetDefaultIMEWnd(0);
    if (!::IsWindow(hImeWnd))
        return TRUE;
    TLS *pTLS = TLS::GetTLS();
    LRESULT ret;
    if (pTLS && pTLS->m_cWnds > 1)
        ret = ::SendMessage(imcLock.get().hWnd, WM_IME_NOTIFY, wParam, lParam);
    else
        ret = ::SendMessage(hImeWnd, WM_IME_NOTIFY, wParam, lParam);
    return !ret;
}

/// @implemented
HRESULT UIComposition::CreateDefFrameWnd(HWND hwndParent, HIMC hIMC)
{
    if (!m_pDefCompFrameWindow)
    {
        m_pDefCompFrameWindow = new(cicNoThrow) CDefCompFrameWindow(hIMC, 0x800000A4);
        if (!m_pDefCompFrameWindow)
            return E_OUTOFMEMORY;
        if (!m_pDefCompFrameWindow->Initialize())
        {
            delete m_pDefCompFrameWindow;
            m_pDefCompFrameWindow = NULL;
            return E_FAIL;
        }

        m_pDefCompFrameWindow->Init();
    }

    m_pDefCompFrameWindow->CreateWnd(hwndParent);
    return S_OK;
}

/// @implemented
HRESULT UIComposition::CreateCompButtonWnd(HWND hwndParent, HIMC hIMC)
{
    TLS *pTLS = TLS::GetTLS();
    if (!pTLS || !pTLS->NonEACompositionEnabled())
        return S_OK;

    if (IsEALang(0))
    {
        if (m_pCompButtonFrameWindow)
        {
            delete m_pCompButtonFrameWindow;
            m_pCompButtonFrameWindow = NULL;
        }
        return S_OK;
    }

    if (!m_pCompButtonFrameWindow)
    {
        m_pCompButtonFrameWindow = new(cicNoThrow) CCompButtonFrameWindow(hIMC, 0x800000B4);
        if (!m_pCompButtonFrameWindow)
            return E_OUTOFMEMORY;

        if (!m_pCompButtonFrameWindow->Initialize())
        {
            if (m_pCompButtonFrameWindow)
            {
                delete m_pCompButtonFrameWindow;
                m_pCompButtonFrameWindow = NULL;
            }
            return E_FAIL;
        }

        m_pCompButtonFrameWindow->Init();
    }

    m_pCompButtonFrameWindow->CreateWnd(hwndParent);
    return S_OK;
}

/// @implemented
HRESULT UIComposition::CreateCompositionWindow(CicIMCLock& imcLock, HWND hwndParent)
{
    if (FAILED(imcLock.m_hr))
        return imcLock.m_hr;

    if (!::IsWindow(hwndParent) || m_bHasCompWnd)
        return E_FAIL;

    for (INT iCompStr = 0; iCompStr < 3; ++iCompStr)
    {
        DWORD style = WS_POPUP | WS_DISABLED;
        HWND hwndCompStr = ::CreateWindowExW(0, L"MSCTFIME Composition", NULL, style,
                                             0, 0, 0, 0, hwndParent, NULL, g_hInst, NULL);
        m_CompStrs[iCompStr].m_hWnd = hwndCompStr;
        ::SetWindowLongPtrW(hwndCompStr, GWLP_USERDATA, (LONG_PTR)this);
        ::SetWindowLongPtrW(hwndCompStr, UICOMP_GWLP_INDEX, iCompStr);
        m_CompStrs[iCompStr].m_Caret.CreateCaret(hwndCompStr, m_CaretSize);
    }

    HRESULT hr = CreateCompButtonWnd(hwndParent, imcLock.m_hIMC);
    if (FAILED(hr))
    {
        DestroyCompositionWindow();
        return E_OUTOFMEMORY;
    }

    hr = CreateDefFrameWnd(hwndParent, imcLock.m_hIMC);
    if (FAILED(hr))
    {
        DestroyCompositionWindow();
        return E_OUTOFMEMORY;
    }

    DWORD style = WS_CHILD | WS_DISABLED;
    HWND hwndCompStr = ::CreateWindowExW(WS_EX_CLIENTEDGE, L"MSCTFIME Composition", NULL, style,
                                         0, 0, 0, 0, *m_pDefCompFrameWindow, NULL, g_hInst, NULL);
    if (!hwndCompStr)
    {
        DestroyCompositionWindow();
        return E_OUTOFMEMORY;
    }

    m_CompStrs[3].m_hWnd = hwndCompStr;
    m_pDefCompFrameWindow->m_hwndCompStr = hwndCompStr;
    ::SetWindowLongPtrW(hwndCompStr, GWLP_USERDATA, (LONG_PTR)this);
    ::SetWindowLongPtrW(hwndCompStr, UICOMP_GWLP_INDEX, -1);
    m_CompStrs[3].m_Caret.CreateCaret(hwndCompStr, m_CaretSize);
    m_bHasCompWnd = TRUE;

    return S_OK;
}

/// @implemented
HRESULT UIComposition::DestroyCompositionWindow()
{
    for (INT i = 0; i < 4; ++i)
    {
        COMPWND *pCompStr = &m_CompStrs[i];
        pCompStr->m_Caret.DestroyCaret();
        if (::IsWindow(pCompStr->m_hWnd))
        {
            DestroyWindow(pCompStr->m_hWnd);
            pCompStr->m_PolyText.RemoveAll();
        }
        pCompStr->m_hWnd = NULL;
    }

    if (m_pCompButtonFrameWindow)
    {
        ::DestroyWindow(*m_pCompButtonFrameWindow);
        delete m_pCompButtonFrameWindow;
        m_pCompButtonFrameWindow = NULL;
    }

    if (m_pDefCompFrameWindow)
    {
        ::DestroyWindow(*m_pDefCompFrameWindow);
        delete m_pDefCompFrameWindow;
        m_pDefCompFrameWindow = NULL;
    }

    return 0;
}

// @implemented
BOOL UIComposition::InquireImeUIWndState(CicIMCLock& imcLock)
{
    BOOL bValue = FALSE;
    UIComposition::SendMessageToUI(imcLock, 0x11u, (LPARAM)&bValue);
    return bValue;
}

/// @implemented
HRESULT UIComposition::UpdateShowCompWndFlag(CicIMCLock& imcLock, DWORD *pdwCompStrLen)
{
    if (FAILED(imcLock.m_hr))
        return imcLock.m_hr;
    if (!::IsWindow(imcLock.get().hWnd))
        return E_FAIL;

    CicIMCCLock<COMPOSITIONSTRING> compStr(imcLock.get().hCompStr);
    if (FAILED(compStr.m_hr))
        return compStr.m_hr;

    if ((m_dwUnknown56[0] & 0x80000000) && compStr.get().dwCompStrLen)
        m_bHasCompStr = TRUE;
    else
        m_bHasCompStr = FALSE;

    if (pdwCompStrLen)
        *pdwCompStrLen = compStr.get().dwCompStrLen;

    return S_OK;
}

/// @implemented
HRESULT UIComposition::UpdateFont(CicIMCLock& imcLock)
{
    if (FAILED(imcLock.m_hr))
        return imcLock.m_hr;

    if (m_hFont1)
        ::DeleteObject(m_hFont1);
    if (m_hFont2)
        ::DeleteObject(m_hFont2);

    LOGFONTW lf = imcLock.get().lfFont.W;
    m_hFont2 = ::CreateFontIndirectW(&lf);

    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    BOOL bVertical = (lf.lfFaceName[0] == L'@');
    if (bVertical)
    {
        MoveMemory(lf.lfFaceName, &lf.lfFaceName[1], sizeof(lf.lfFaceName) - sizeof(WCHAR));
        lf.lfFaceName[_countof(lf.lfFaceName) - 1] = UNICODE_NULL;
    }
    m_hFont1 = ::CreateFontIndirectW(&lf);

    return S_OK;
}

// @implemented
void UIComposition::OnTimer(HWND hWnd)
{
    INT iCompStr = (INT)::GetWindowLongPtrW(hWnd, UICOMP_GWLP_INDEX);
    if (iCompStr == -1)
        m_CompStrs[3].m_Caret.OnTimer();
    else
        m_CompStrs[iCompStr].m_Caret.OnTimer();
}

/// @implemented
BOOL UIComposition::GetImeUIWndTextExtent(CicIMCLock& imcLock, LPARAM lParam)
{
    return !UIComposition::SendMessageToUI(imcLock, 0x14, lParam);
}

/// @implemented
LPWSTR UIComposition::GetCompStrBuffer(INT cchStr)
{
    if (!m_strCompStr)
    {
        m_strCompStr = (LPWSTR)cicMemAllocClear((cchStr + 1) * sizeof(WCHAR));
        m_cchCompStr = cchStr;
    }
    if (m_cchCompStr < cchStr)
    {
        m_strCompStr = (LPWSTR)cicMemReAlloc(m_strCompStr, (cchStr + 1) * sizeof(WCHAR));
        m_cchCompStr = cchStr;
    }
    return m_strCompStr;
}

/// @unimplemented
void UIComposition::OnImeStartComposition(CicIMCLock& imcLock, HWND hUIWnd)
{
    //FIXME
}

/// @implemented
HRESULT UIComposition::OnImeCompositionUpdate(CicIMCLock& imcLock)
{
    m_dwUnknown56[0] |= 0x8000;
    return UIComposition::UpdateShowCompWndFlag(imcLock, NULL);
}

/// @unimplemented
HRESULT UIComposition::OnImeEndComposition()
{
    m_dwUnknown56[0] = 0;
    return DestroyCompositionWindow();
}

/// @unimplemented
void UIComposition::OnImeSetContext(CicIMCLock& imcLock, HWND hUIWnd, WPARAM wParam, LPARAM lParam)
{
    //FIXME
}

/// @unimplemented
void UIComposition::OnPaintTheme(WPARAM wParam)
{
    //FIXME
}

/// @implemented
HRESULT UIComposition::OnDestroy()
{
    return DestroyCompositionWindow();
}

/// @unimplemented
LRESULT CALLBACK
UIComposition::CompWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_CREATE)
        return -1; // FIXME
    return 0;
}

/// @implemented
HRESULT UIComposition::OnImeNotifySetCompositionWindow(CicIMCLock& imcLock)
{
    return UpdateCompositionRect(imcLock);
}

/// @unimplemented
HRESULT UIComposition::UpdateCompositionRect(CicIMCLock& imcLock)
{
    return E_NOTIMPL;
}

/// @implemented
INT UIComposition::GetLevelFromIMC(CicIMCLock& imcLock)
{
    DWORD dwStyle = imcLock.get().cfCompForm.dwStyle;
    if (dwStyle == CFS_DEFAULT)
        return 1;
    if (!(dwStyle & (CFS_FORCE_POSITION | CFS_POINT | CFS_RECT)))
        return 0;

    RECT rc;
    ::GetClientRect(imcLock.get().hWnd, &rc);
    if (!::PtInRect(&rc, imcLock.get().cfCompForm.ptCurrentPos))
        return 1;

    INPUTCONTEXT *pIC = &imcLock.get();
    if ((pIC->cfCompForm.dwStyle & CFS_RECT) &&
        (pIC->cfCompForm.rcArea.top == pIC->cfCompForm.rcArea.bottom) &&
        (pIC->cfCompForm.rcArea.left == pIC->cfCompForm.rcArea.right))
    {
        return 1;
    }

    return 2;
}

/// @implemented
HRESULT UIComposition::OnImeSetContextAfter(CicIMCLock& imcLock)
{
    if ((!m_pDefCompFrameWindow || !::IsWindow(*m_pDefCompFrameWindow)) && !::IsWindow(m_CompStrs[0].m_hWnd))
    {
        m_dwUnknown56[0] &= ~0x8000;
        return S_OK;
    }

    if (FAILED(imcLock.m_hr))
        return imcLock.m_hr;

    INT Level = GetLevelFromIMC(imcLock);
    if ((Level == 1 || Level == 2) && (m_dwUnknown56[0] & 0x80000000) && m_dwUnknown56[1])
    {
        DWORD dwCompStrLen = 0;
        UpdateShowCompWndFlag(imcLock, &dwCompStrLen);

        if (Level == 1)
        {
            ::ShowWindow(*m_pDefCompFrameWindow, (m_bHasCompStr ? SW_SHOWNOACTIVATE : SW_HIDE));
        }
        else if (Level == 2 && !m_bHasCompStr && dwCompStrLen)
        {
            for (INT iCompStr = 0; iCompStr < 3; ++iCompStr)
            {
                m_CompStrs[iCompStr].m_Caret.HideCaret();
                ::ShowWindow(m_CompStrs[iCompStr].m_Caret, SW_HIDE);
            }
        }
    }
    else
    {
        ::ShowWindow(*m_pDefCompFrameWindow, SW_HIDE);

        for (INT iCompStr = 0; iCompStr < 3; ++iCompStr)
        {
            m_CompStrs[iCompStr].m_Caret.HideCaret();
            ::ShowWindow(m_CompStrs[iCompStr].m_Caret, SW_HIDE);
        }
    }

    return S_OK;
}

/***********************************************************************/

// For GetWindowLongPtr/SetWindowLongPtr
#define UI_GWLP_HIMC 0
#define UI_GWLP_UI   sizeof(HIMC)
#define UI_GWLP_SIZE (UI_GWLP_UI + sizeof(UI*))

/// @implemented
UI::UI(HWND hWnd) : m_hWnd(hWnd)
{
}

/// @implemented
UI::~UI()
{
    delete m_pComp;
}

/// @implemented
HRESULT UI::_Create()
{
    m_pComp = new(cicNoThrow) UIComposition(m_hWnd);
    if (!m_pComp)
        return E_OUTOFMEMORY;

    ::SetWindowLongPtrW(m_hWnd, UI_GWLP_UI, (LONG_PTR)this);
    return S_OK;
}

/// @implemented
void UI::_Destroy()
{
    m_pComp->OnDestroy();
    ::SetWindowLongPtrW(m_hWnd, UI_GWLP_UI, 0);
}

/// @implemented
void UI::OnCreate(HWND hWnd)
{
    UI *pUI = (UI*)::GetWindowLongPtrW(hWnd, UI_GWLP_UI);
    if (pUI)
        return;
    pUI = new(cicNoThrow) UI(hWnd);
    if (pUI)
        pUI->_Create();
}

/// @implemented
void UI::OnDestroy(HWND hWnd)
{
    UI *pUI = (UI*)::GetWindowLongPtrW(hWnd, UI_GWLP_UI);
    if (!pUI)
        return;

    pUI->_Destroy();
    delete pUI;
}

/// @implemented
void UI::OnImeSetContext(CicIMCLock& imcLock, WPARAM wParam, LPARAM lParam)
{
    m_pComp->OnImeSetContext(imcLock, m_hWnd, wParam, lParam);
}

/***********************************************************************/

struct CIMEUIWindowHandler
{
    static LRESULT CALLBACK ImeUIMsImeHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static HRESULT CALLBACK ImeUIMsImeMouseHandler(HWND hWnd, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ImeUIMsImeModeBiasHandler(HWND hWnd, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ImeUIMsImeReconvertRequest(HWND hWnd, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ImeUIWndProcWorker(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static HRESULT CALLBACK ImeUIDelayedReconvertFuncCall(HWND hWnd);
    static HRESULT CALLBACK ImeUIOnLayoutChange(LPARAM lParam);
    static HRESULT CALLBACK ImeUIPrivateHandler(UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ImeUINotifyHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

/// @implemented
HRESULT CALLBACK
CIMEUIWindowHandler::ImeUIMsImeMouseHandler(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    if ((BYTE)wParam == 0xFF)
        return TRUE;

    CicIMCLock imcLock((HIMC)lParam);
    if (FAILED(imcLock.m_hr))
        return imcLock.m_hr;

    CicIMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCtfImeContext);
    if (FAILED(imeContext.m_hr))
        return FALSE;

    HRESULT hr = E_FAIL;
    CicInputContext *pCicIC = imeContext.get().m_pCicIC;
    if (pCicIC)
    {
        UINT keys = 0;
        if (wParam & MK_LBUTTON)
            keys |= 0x1;
        if (wParam & MK_SHIFT)
            keys |= 0x10;
        if (wParam & MK_RBUTTON)
            keys |= 0x2;
        if (wParam & MK_MBUTTON)
            keys |= 0x780000;
        else if (wParam & MK_ALT)
            keys |= 0xFF880000;
        hr = pCicIC->MsImeMouseHandler(HIWORD(wParam), HIBYTE(LOWORD(wParam)), keys, imcLock);
    }

    return hr;
}

/// @implemented
HRESULT CALLBACK CIMEUIWindowHandler::ImeUIOnLayoutChange(LPARAM lParam)
{
    CicIMCLock imcLock((HIMC)lParam);
    if (FAILED(imcLock.m_hr))
        return S_OK;

    CicIMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCtfImeContext);
    if (FAILED(imeContext.m_hr))
        return S_OK;

    CicInputContext *pCicIC = imeContext.get().m_pCicIC;
    if (pCicIC)
    {
        auto pContextOwnerServices = pCicIC->m_pContextOwnerServices;
        pContextOwnerServices->AddRef();
        pContextOwnerServices->OnLayoutChange();
        pContextOwnerServices->Release();
    }

    return S_OK;
}

/// @implemented
HRESULT CALLBACK
CIMEUIWindowHandler::ImeUIPrivateHandler(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CicIMCLock imcLock((HIMC)lParam);
    if (FAILED(imcLock.m_hr))
        return imcLock.m_hr;

    CicIMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCtfImeContext);
    if (FAILED(imeContext.m_hr))
        return imeContext.m_hr;

    CicInputContext *pCicIC = imeContext.get().m_pCicIC;
    if (pCicIC && wParam == 0x10)
        pCicIC->EscbClearDocFeedBuffer(imcLock, TRUE);

    return S_OK;
}

/// @implemented
LRESULT CALLBACK
CIMEUIWindowHandler::ImeUIMsImeModeBiasHandler(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    if (!wParam)
        return TRUE;

    CicIMCLock imcLock((HIMC)lParam);
    if (FAILED(imcLock.m_hr))
        return imcLock.m_hr;

    CicIMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCtfImeContext);
    if (FAILED(imeContext.m_hr))
        return FALSE;

    CicInputContext *pCicIC = imeContext.get().m_pCicIC;
    if (!pCicIC)
        return FALSE;

    if (wParam == 1)
    {
        if (lParam == 0 || lParam == 1 || lParam == 4 || lParam == 0x10000)
        {
            GUID guid = pCicIC->m_ModeBias.ConvertModeBias((DWORD)lParam);
            pCicIC->m_ModeBias.SetModeBias(guid);

            pCicIC->m_dwUnknown7[2] |= 1;
            pCicIC->m_pContextOwnerServices->AddRef();
            pCicIC->m_pContextOwnerServices->OnAttributeChange(GUID_PROP_MODEBIAS);
            pCicIC->m_pContextOwnerServices->Release();
            return TRUE;
        }
    }
    else if (wParam == 2)
    {
        return pCicIC->m_ModeBias.ConvertModeBias(pCicIC->m_ModeBias.m_guid);
    }

    return FALSE;
}

/// @implemented
LRESULT CALLBACK
CIMEUIWindowHandler::ImeUIMsImeReconvertRequest(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    if (wParam == 0x10000000)
        return TRUE;

    HIMC hIMC = (HIMC)::GetWindowLongPtrW(hWnd, UI_GWLP_HIMC);
    CicIMCLock imcLock(hIMC);
    if (FAILED(imcLock.m_hr))
        return FALSE;

    CicIMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCtfImeContext);
    if (FAILED(imeContext.m_hr))
        return FALSE;

    CicInputContext *pCicIC = imeContext.get().m_pCicIC;
    TLS *pTLS = TLS::GetTLS();
    if (!pCicIC || !pTLS)
        return FALSE;

    auto pThreadMgr = pTLS->m_pThreadMgr;
    auto pProfile = pTLS->m_pProfile;
    if (!pThreadMgr || !pProfile)
        return FALSE;

    UINT uCodePage = 0;
    pProfile->GetCodePageA(&uCodePage);
    pCicIC->SetupReconvertString(imcLock, pThreadMgr, uCodePage, WM_MSIME_RECONVERT, FALSE);
    pCicIC->EndReconvertString(imcLock);
    return TRUE;
}

/// @implemented
LRESULT CALLBACK
CIMEUIWindowHandler::ImeUIMsImeHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_MSIME_MOUSE)
        return ImeUIMsImeMouseHandler(hWnd, wParam, lParam);
    if (uMsg == WM_MSIME_MODEBIAS)
        return ImeUIMsImeModeBiasHandler(hWnd, wParam, lParam);
    if (uMsg == WM_MSIME_RECONVERTREQUEST)
        return ImeUIMsImeReconvertRequest(hWnd, wParam, lParam);
    if (uMsg == WM_MSIME_SERVICE)
    {
        TLS *pTLS = TLS::GetTLS();
        if (pTLS && pTLS->m_pProfile)
        {
            LANGID LangID;
            pTLS->m_pProfile->GetLangId(&LangID);
            if (PRIMARYLANGID(LangID) == LANG_KOREAN)
                return FALSE;
        }
        return TRUE;
    }
    return 0;
}

/// @implemented
HRESULT CALLBACK
CIMEUIWindowHandler::ImeUIDelayedReconvertFuncCall(HWND hWnd)
{
    HIMC hIMC = (HIMC)::GetWindowLongPtrW(hWnd, UI_GWLP_HIMC);
    CicIMCLock imcLock(hIMC);
    if (FAILED(imcLock.m_hr))
        return imcLock.m_hr;
    CicIMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCtfImeContext);
    if (FAILED(imeContext.m_hr))
        return imeContext.m_hr;
    CicInputContext *pCicIC = imeContext.get().m_pCicIC;
    if (pCicIC)
        pCicIC->DelayedReconvertFuncCall(imcLock);
    return S_OK;
}

/// @unimplemented
LRESULT CALLBACK
CIMEUIWindowHandler::ImeUINotifyHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HIMC hIMC = (HIMC)::GetWindowLongPtrW(hWnd, UI_GWLP_HIMC);
    CicIMCLock imcLock(hIMC);
    if (FAILED(imcLock.m_hr))
        return imcLock.m_hr;
    CicIMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCtfImeContext);
    if (FAILED(imeContext.m_hr))
        return imeContext.m_hr;

    CicInputContext *pCicIC = imeContext.get().m_pCicIC;
    if (pCicIC)
    {
        switch (wParam)
        {
            case IMN_CLOSECANDIDATE:
            {
                pCicIC->m_bCandidateOpen = FALSE;
                HWND hImeWnd = ::ImmGetDefaultIMEWnd(NULL);
                if (::IsWindow(hImeWnd))
                    ::PostMessage(hImeWnd, WM_IME_NOTIFY, 0x10, (LPARAM)hIMC);
                break;
            }
            case IMN_OPENCANDIDATE:
            {
                pCicIC->m_bCandidateOpen = TRUE;
                pCicIC->ClearPrevCandidatePos();
                break;
            }
            case IMN_SETCANDIDATEPOS:
            {
                //FIXME
                break;
            }
            case IMN_SETCOMPOSITIONFONT:
            {
                //FIXME
                break;
            }
            case IMN_SETCOMPOSITIONWINDOW:
            {
                //FIXME
                break;
            }
            case 0xF:
            {
                CIMEUIWindowHandler::ImeUIOnLayoutChange(lParam);
                break;
            }
            case 0x10:
            {
                CIMEUIWindowHandler::ImeUIPrivateHandler(uMsg, 0x10, lParam);
                break;
            }
            case 0x13:
            {
                CIMEUIWindowHandler::ImeUIOnLayoutChange(lParam);
                break;
            }
            case 0x14:
            {
                //FIXME
                break;
            }
            case 0x16:
            {
                ::SetTimer(hWnd, 2, 100, NULL);
                break;
            }
            case 0x17:
            {
                return (LRESULT)hWnd;
            }
            case 0x10D:
            {
                //FIXME
                break;
            }
            case 0x10E:
                pCicIC->m_dwQueryPos = 0;
                break;
        }
    }

    return 0;
}

/// @implemented
LRESULT CALLBACK
CIMEUIWindowHandler::ImeUIWndProcWorker(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TLS *pTLS = TLS::GetTLS();
    if (pTLS && (pTLS->m_dwSystemInfoFlags & IME_SYSINFO_WINLOGON))
    {
        if (uMsg == WM_CREATE)
            return -1;
        return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    switch (uMsg)
    {
        case WM_CREATE:
        {
            UI::OnCreate(hWnd);
            break;
        }
        case WM_DESTROY:
        case WM_ENDSESSION:
        {
            UI::OnDestroy(hWnd);
            break;
        }
        case WM_IME_STARTCOMPOSITION:
        case WM_IME_COMPOSITION:
        case WM_IME_ENDCOMPOSITION:
        case WM_IME_SETCONTEXT:
        case WM_IME_NOTIFY:
        case WM_IME_SELECT:
        case WM_TIMER:
        {
            HIMC hIMC = (HIMC)GetWindowLongPtrW(hWnd, UI_GWLP_HIMC);
            UI* pUI = (UI*)GetWindowLongPtrW(hWnd, UI_GWLP_UI);
            CicIMCLock imcLock(hIMC);
            switch (uMsg)
            {
                case WM_IME_STARTCOMPOSITION:
                {
                    pUI->m_pComp->OnImeStartComposition(imcLock, pUI->m_hWnd);
                    break;
                }
                case WM_IME_COMPOSITION:
                {
                    if (lParam & GCS_COMPSTR)
                    {
                        pUI->m_pComp->OnImeCompositionUpdate(imcLock);
                        ::SetTimer(hWnd, 0, 10, NULL);
                        pUI->m_pComp->m_bInComposition = TRUE;
                    }
                    break;
                }
                case WM_IME_ENDCOMPOSITION:
                {
                    ::KillTimer(hWnd, 0);
                    pUI->m_pComp->OnImeEndComposition();
                    break;
                }
                case WM_IME_SETCONTEXT:
                {
                    pUI->OnImeSetContext(imcLock, wParam, lParam);
                    ::KillTimer(hWnd, 1);
                    ::SetTimer(hWnd, 1, 300, NULL);
                    break;
                }
                case WM_TIMER:
                {
                    switch (wParam)
                    {
                        case 0:
                            ::KillTimer(hWnd, wParam);
                            pUI->m_pComp->m_bInComposition = FALSE;
                            pUI->m_pComp->OnImeNotifySetCompositionWindow(imcLock);
                            break;
                        case 1:
                            ::KillTimer(hWnd, wParam);
                            pUI->m_pComp->OnImeSetContextAfter(imcLock);
                            break;
                        case 2:
                            ::KillTimer(hWnd, wParam);
                            CIMEUIWindowHandler::ImeUIDelayedReconvertFuncCall(hWnd);
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case WM_IME_NOTIFY:
                case WM_IME_SELECT:
                default:
                {
                    pUI->m_pComp->OnPaintTheme(wParam);
                    break;
                }
            }
            break;
        }
        default:
        {
            if (IsMsImeMessage(uMsg))
                return CIMEUIWindowHandler::ImeUIMsImeHandler(hWnd, uMsg, wParam, lParam);
            return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
        }
    }

    return 0;
}

/***********************************************************************/

/// @implemented
EXTERN_C LRESULT CALLBACK
UIWndProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    return CIMEUIWindowHandler::ImeUIWndProcWorker(hWnd, uMsg, wParam, lParam);
}

/***********************************************************************/

/// @implemented
BOOL RegisterImeClass(VOID)
{
    WNDCLASSEXW wcx;

    if (!GetClassInfoExW(g_hInst, L"MSCTFIME UI", &wcx))
    {
        ZeroMemory(&wcx, sizeof(wcx));
        wcx.cbSize          = sizeof(WNDCLASSEXW);
        wcx.cbWndExtra      = UI_GWLP_SIZE;
        wcx.hIcon           = LoadIconW(0, (LPCWSTR)IDC_ARROW);
        wcx.hInstance       = g_hInst;
        wcx.hCursor         = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
        wcx.hbrBackground   = (HBRUSH)GetStockObject(NULL_BRUSH);
        wcx.style           = CS_IME | CS_GLOBALCLASS;
        wcx.lpfnWndProc     = UIWndProc;
        wcx.lpszClassName   = L"MSCTFIME UI";
        if (!RegisterClassExW(&wcx))
            return FALSE;
    }

    if (!GetClassInfoExW(g_hInst, L"MSCTFIME Composition", &wcx))
    {
        ZeroMemory(&wcx, sizeof(wcx));
        wcx.cbSize          = sizeof(WNDCLASSEXW);
        wcx.cbWndExtra      = UICOMP_GWLP_SIZE;
        wcx.hIcon           = NULL;
        wcx.hInstance       = g_hInst;
        wcx.hCursor         = LoadCursorW(NULL, (LPCWSTR)IDC_IBEAM);
        wcx.hbrBackground   = (HBRUSH)GetStockObject(NULL_BRUSH);
        wcx.style           = CS_IME | CS_HREDRAW | CS_VREDRAW;
        wcx.lpfnWndProc     = UIComposition::CompWndProc;
        wcx.lpszClassName   = L"MSCTFIME Composition";
        if (!RegisterClassExW(&wcx))
            return FALSE;
    }

    return TRUE;
}

/// @implemented
VOID UnregisterImeClass(VOID)
{
    WNDCLASSEXW wcx;

    GetClassInfoExW(g_hInst, L"MSCTFIME UI", &wcx);
    UnregisterClassW(L"MSCTFIME UI", g_hInst);
    DestroyIcon(wcx.hIcon);
    DestroyIcon(wcx.hIconSm);

    GetClassInfoExW(g_hInst, L"MSCTFIME Composition", &wcx);
    UnregisterClassW(L"MSCTFIME Composition", g_hInst);
    DestroyIcon(wcx.hIcon);
    DestroyIcon(wcx.hIconSm);
}
