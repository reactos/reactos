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

    ::MoveWindow(m_hwndDefCompFrame, GripperWidth + 2, 7, nWidth, nHeight, TRUE);
    ::ShowWindow(m_hwndDefCompFrame, (bShow ? SW_SHOWNOACTIVATE : SW_HIDE));
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
    if (!::IsWindow(m_hwndDefCompFrame))
        return FALSE;

    RECT rc;
    ::GetWindowRect(m_hwndDefCompFrame, &rc);
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
    if (::IsWindow(m_hwndDefCompFrame))
    {
        RECT rc;
        ::GetWindowRect(m_hwndDefCompFrame, &rc);
        MyScreenToClient(NULL, &rc);

        POINT pt = { x, y };
        if (::PtInRect(&rc, pt))
            ::SendMessage(m_hwndDefCompFrame, 0x7E8, 0, 0);
    }

    CUIFWindow::HandleMouseMsg(uMsg, x, y);
}

/***********************************************************************/

// For GetWindowLongPtr/SetWindowLongPtr
#define UIGWLP_HIMC 0
#define UIGWLP_UI   sizeof(HIMC)
#define UIGWLP_SIZE (UIGWLP_UI + sizeof(UI*))

/// @unimplemented
void UIComposition::OnImeStartComposition(CicIMCLock& imcLock, HWND hUIWnd)
{
    //FIXME
}

/// @unimplemented
void UIComposition::OnImeCompositionUpdate(CicIMCLock& imcLock)
{
    //FIXME
}

/// @unimplemented
void UIComposition::OnImeEndComposition()
{
    //FIXME
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

/// @unimplemented
void UIComposition::OnDestroy()
{
    //FIXME
}

/// @unimplemented
LRESULT CALLBACK
UIComposition::CompWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_CREATE)
        return -1; // FIXME
    return 0;
}

/***********************************************************************/

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
    m_pComp = new(cicNoThrow) UIComposition();
    if (!m_pComp)
        return E_OUTOFMEMORY;

    ::SetWindowLongPtrW(m_hWnd, UIGWLP_UI, (LONG_PTR)this);
    return S_OK;
}

/// @implemented
void UI::_Destroy()
{
    m_pComp->OnDestroy();
    ::SetWindowLongPtrW(m_hWnd, UIGWLP_UI, 0);
}

/// @implemented
void UI::OnCreate(HWND hWnd)
{
    UI *pUI = (UI*)::GetWindowLongPtrW(hWnd, UIGWLP_UI);
    if (pUI)
        return;
    pUI = new(cicNoThrow) UI(hWnd);
    if (pUI)
        pUI->_Create();
}

/// @implemented
void UI::OnDestroy(HWND hWnd)
{
    UI *pUI = (UI*)::GetWindowLongPtrW(hWnd, UIGWLP_UI);
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
    static LRESULT CALLBACK ImeUIMsImeMouseHandler(HWND hWnd, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ImeUIMsImeModeBiasHandler(HWND hWnd, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ImeUIMsImeReconvertRequest(HWND hWnd, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ImeUIWndProcWorker(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

/// @unimplemented
LRESULT CALLBACK
CIMEUIWindowHandler::ImeUIMsImeMouseHandler(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    return 0; //FIXME
}

/// @unimplemented
LRESULT CALLBACK
CIMEUIWindowHandler::ImeUIMsImeModeBiasHandler(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    return 0; //FIXME
}

/// @unimplemented
LRESULT CALLBACK
CIMEUIWindowHandler::ImeUIMsImeReconvertRequest(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    return 0; //FIXME
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

/// @unimplemented
LRESULT CALLBACK
CIMEUIWindowHandler::ImeUIWndProcWorker(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TLS *pTLS = TLS::GetTLS();
    if (pTLS && (pTLS->m_dwSystemInfoFlags & IME_SYSINFO_WINLOGON))
    {
        if (uMsg == WM_CREATE)
            return -1;
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
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
            HIMC hIMC = (HIMC)GetWindowLongPtrW(hWnd, UIGWLP_HIMC);
            UI* pUI = (UI*)GetWindowLongPtrW(hWnd, UIGWLP_UI);
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
                        //FIXME
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
                    //FIXME
                    ::KillTimer(hWnd, wParam);
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
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
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

/// @unimplemented
BOOL RegisterImeClass(VOID)
{
    WNDCLASSEXW wcx;

    if (!GetClassInfoExW(g_hInst, L"MSCTFIME UI", &wcx))
    {
        ZeroMemory(&wcx, sizeof(wcx));
        wcx.cbSize          = sizeof(WNDCLASSEXW);
        wcx.cbWndExtra      = UIGWLP_SIZE;
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
        wcx.cbWndExtra      = sizeof(DWORD);
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
