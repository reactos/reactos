/////////////////////////////////////////////////////////////////////////////
// WND.CPP
//
// Implementation of CWindow
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     08/26/96    Created
/////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "saver.h"
#include "wnd.h"

#define TF_IME      TF_ALWAYS

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
extern HINSTANCE    g_hIMM;
extern IMMASSOCPROC g_pfnIMMProc;
    // IME

/////////////////////////////////////////////////////////////////////////////
// Module variables
/////////////////////////////////////////////////////////////////////////////
static const TCHAR s_szWindowClassName[] = TEXT("WindowsScreenSaverClass");

/////////////////////////////////////////////////////////////////////////////
// CWindow
/////////////////////////////////////////////////////////////////////////////
ATOM CWindow::m_atomWndClass = NULL;

CWindow::CWindow
(
)
{
    m_hPrevIMC = 0;
    m_hWnd = NULL;
}

CWindow::~CWindow
(
)
{
    if  (
        (m_hWnd != NULL)
        &&
        IsWindow(m_hWnd)
        )
    {
        DestroyWindow(m_hWnd);
    }

    ASSERT(m_hPrevIMC == 0);
}

/////////////////////////////////////////////////////////////////////////////
// CWindow::InitWndClass
/////////////////////////////////////////////////////////////////////////////
void CWindow::InitWndClass
(
)
{
    WNDCLASS wcWndClass;

    wcWndClass.style = CS_HREDRAW | CS_VREDRAW;
    wcWndClass.lpfnWndProc = (WNDPROC)GenericWndProc;
    wcWndClass.hInstance = _pModule->GetModuleInstance();
    wcWndClass.cbClsExtra = 0;
    wcWndClass.cbWndExtra = sizeof(DWORD);
    wcWndClass.hIcon = NULL;
    wcWndClass.hCursor = LoadCursor(NULL, IDC_ARROW);;
    wcWndClass.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wcWndClass.lpszMenuName = NULL;       
    wcWndClass.lpszClassName = s_szWindowClassName;

    m_atomWndClass = RegisterClass(&wcWndClass);
}

/////////////////////////////////////////////////////////////////////////////
// CWindow::Create
/////////////////////////////////////////////////////////////////////////////
BOOL CWindow::Create
(
    LPCSTR          lpszWindowName,
    DWORD           dwStyle,
    const RECT &    rect,
    HWND            hwndParent,
    UINT            nID
)
{
    return CreateEx(lpszWindowName, 0, dwStyle, rect, hwndParent, nID);
}

/////////////////////////////////////////////////////////////////////////////
// CWindow::CreateEx
/////////////////////////////////////////////////////////////////////////////
BOOL CWindow::CreateEx
(
    LPCSTR          lpszWindowName,
    DWORD           dwExStyle,
    DWORD           dwStyle,
    const RECT &    rect,
    HWND            hwndParent,
    UINT            nID
)
{
    if (m_atomWndClass == NULL)
        InitWndClass();

    // NOTE: m_hWnd will get set in OnCreate().

    EVAL(CreateWindowEx(dwExStyle,
                        s_szWindowClassName,
                        lpszWindowName,
                        dwStyle,
                        rect.left,
                        rect.top,
                        rect.right - rect.left,
                        rect.bottom - rect.top,
                        hwndParent,
                        (HMENU)nID,
                        _pModule->GetModuleInstance(),
                        (LPVOID)this) != NULL);

    ASSERT(m_hWnd != NULL);

    return (m_hWnd != NULL);
}

/////////////////////////////////////////////////////////////////////////////
// CWindow::SysPalChanged
/////////////////////////////////////////////////////////////////////////////
void CWindow::SysPalChanged
(
)
{
    // Redraw the current window.
    SysPalChangedCallback(m_hWnd, NULL);

    // Redraw all child windows, the system palette has changed.
    EnumChildWindows(m_hWnd, SysPalChangedCallback, NULL);
}

/////////////////////////////////////////////////////////////////////////////
// CWindow::OnCreate
/////////////////////////////////////////////////////////////////////////////
BOOL CWindow::OnCreate
(
    CREATESTRUCT * pcs
)
{
    // Disable IME for all windows of this class.
    if ((g_hIMM != NULL) && (g_pfnIMMProc != NULL))
    {
        m_hPrevIMC = g_pfnIMMProc(m_hWnd, (HIMC)NULL);

        TraceMsg(TF_IME, "m_hPrevIMC = 0x%.8X", m_hPrevIMC);
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CWindow::OnDestroy
/////////////////////////////////////////////////////////////////////////////
void CWindow::OnDestroy
(
)
{
    if ((g_hIMM != NULL)  && (g_pfnIMMProc != NULL))
    {
        if (m_hPrevIMC != 0)
        {
            g_pfnIMMProc(m_hWnd, m_hPrevIMC);
            m_hPrevIMC = 0;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// Helper functions
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// SysPalChangedCallback
/////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK SysPalChangedCallback
(
    HWND    hWnd,
    LPARAM  lParam
)
{
    if (IsWindowVisible(hWnd))
        RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);

    return TRUE;    // Continue enumeration
}

/////////////////////////////////////////////////////////////////////////////
// GenericWndProc
/////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK GenericWndProc
(
    HWND    hWnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    CWindow *   pWnd = (CWindow *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    BOOL        bHandled = FALSE;

    switch (uMsg)
    {
        case WM_CREATE:
        {
            EVAL((pWnd = (CWindow *)((CREATESTRUCT *)lParam)->lpCreateParams) != NULL);

            // Make sure m_hWnd in initalized for OnCreate().
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pWnd);
            pWnd->m_hWnd = hWnd;

            return (pWnd->OnCreate((CREATESTRUCT *)lParam) ? 0 : -1);
        }

        case WM_ERASEBKGND:
        {
            if (pWnd != NULL)
                return pWnd->OnEraseBkgnd((HDC)wParam);
            else
                return FALSE;
        }

        case WM_KEYDOWN:
        {
            ASSERT(pWnd != NULL);

            pWnd->OnKeyDown((UINT)wParam, LOWORD(lParam), HIWORD(lParam));
            return 0;
        }

        case WM_KEYUP:
        {
            ASSERT(pWnd != NULL);

            pWnd->OnKeyUp((UINT)wParam, LOWORD(lParam), HIWORD(lParam));
            return 0;
        }

        case WM_QUERYNEWPALETTE:
        {
            ASSERT(pWnd != NULL);

            return pWnd->OnQueryNewPalette();
        }

        case WM_PALETTECHANGED:
        {
            ASSERT(pWnd != NULL);

            pWnd->OnPaletteChanged((HWND)wParam);
            return 0;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;

            ASSERT(pWnd != NULL);

            HDC hDC = BeginPaint(hWnd, &ps);
            pWnd->OnPaint(hDC, &ps);
            EndPaint(hWnd, &ps);

            return 0;
        }

        case WM_PARENTNOTIFY:
        {
            ASSERT(pWnd != NULL);
            pWnd->OnParentNotify(LOWORD(wParam), lParam);
            return 0;
        }

        case WM_SHOWWINDOW:
        {
            if (pWnd != NULL)
                pWnd->OnShowWindow((BOOL)wParam, (int)lParam);
            return 0;
        }

        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        {
            ASSERT(pWnd != NULL);
            pWnd->OnMouseButtonDown((UINT)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        }

        case WM_LBUTTONUP:
            return 0;

        case WM_COMMAND:
        {
            ASSERT(pWnd != NULL);
            return (pWnd->OnCommand(wParam, lParam) ? 0 : 1);
        }

        case WM_NOTIFY:
        {
            LRESULT lResult = 0;

            ASSERT(pWnd != NULL);
            return (pWnd->OnNotify(wParam, lParam, &lResult) ? lResult : 0);
        }

        case WM_MOUSEMOVE:
        {
            ASSERT(pWnd != NULL);
            pWnd->OnMouseMove((UINT)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        }

        case WM_TIMER:
        {
            ASSERT(pWnd != NULL);
            pWnd->OnTimer((UINT)wParam);
            return 0;
        }

        case WM_NCDESTROY:
        {
            ASSERT(pWnd != NULL);
            pWnd->OnNCDestroy();

            pWnd->m_hWnd = NULL;
            return 0;
        }

        case WM_CLOSE:
        {
            ASSERT(pWnd != NULL);
            return pWnd->OnClose();
        }

        case WM_DESTROY:
        {
            ASSERT(pWnd != NULL);
            pWnd->OnDestroy();
            return 0;
        }

        default:
        {
            LRESULT lResult;

            if  (
                (pWnd != NULL)
                &&
                ((uMsg >= WM_USER) && (uMsg <= 0x7FFF))
                &&
                ((lResult = pWnd->OnUserMessage(hWnd, uMsg, wParam, lParam)) != -1)
                )
            {
                return lResult;
            }

            break;
        }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
