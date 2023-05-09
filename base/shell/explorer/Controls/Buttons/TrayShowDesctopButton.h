// This window class name is CONFIRMED on Win10 by WinHier.
static const WCHAR szTrayShowDesktopButton[] = L"TrayShowDesktopButtonWClass";

// Кнопка «Показать рабочий стол» на краю панели задач
class TrayShowDesktopButton : public CWindowImpl<TrayShowDesktopButton, CWindow, CControlWinTraits>
{
    LONG m_nClickedTime;
    BOOL m_bHovering;
    HTHEME m_hTheme;

  public:
    OnDraw(HDC, LPRECT)
    DECLARE_WND_CLASS_EX(szTrayShowDesktopButton, CS_HREDRAW | CS_VREDRAW, COLOR_3DFACE)

    TrayShowDesktopButton() : m_nClickedTime(0), m_bHovering(FALSE)
    {
    }

    INT
    WidthOrHeight() const
    {
#define SHOW_DESKTOP_MINIMUM_WIDTH 3
        INT cxy = 2 * ::GetSystemMetrics(SM_CXEDGE);
        return max(cxy, SHOW_DESKTOP_MINIMUM_WIDTH);
    }

    HRESULT
    DoCreate(HWND hwndParent)
    {
        DWORD style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;
        Create(hwndParent, NULL, NULL, style);
        if (!m_hWnd)
            return E_FAIL;

        ::SetWindowTheme(m_hWnd, L"TaskBar", NULL);
        return S_OK;
    }

    LRESULT
    OnClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        // The actual action can be delayed as an expected behaviour.
        // But a too late action is an unexpected behaviour.
        LONG nTime0 = m_nClickedTime;
        LONG nTime1 = ::GetMessageTime();
        if (nTime1 - nTime0 >= 600) // Ignore after 0.6 sec
            return 0;

        // Show/Hide Desktop
        GetParent().SendMessage(WM_COMMAND, TRAYCMD_TOGGLE_DESKTOP, 0);
        return 0;
    }

#define TSDB_CLICK (WM_USER + 100)

    // This function is called from OnLButtonDown and parent.
    VOID
    Click()
    {
        // The actual action can be delayed as an expected behaviour.
        m_nClickedTime = ::GetMessageTime();
        PostMessage(TSDB_CLICK, 0, 0);
    }

    LRESULT
    OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        Click(); // Left-click
        return 0;
    }

    LRESULT
    OnSettingChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        if (m_hTheme)
            ::CloseThemeData(m_hTheme);

        m_hTheme = ::OpenThemeData(m_hWnd, L"TaskBar");
        InvalidateRect(NULL, TRUE);
        return 0;
    }

    // This function is called from OnPaint and parent.
    VOID
    OnDraw(HDC hdc, LPRECT prc);

    LRESULT
    OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        RECT rc;
        GetClientRect(&rc);

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(&ps);
        OnDraw(hdc, &rc);
        EndPaint(&ps);
        return 0;
    }

    BOOL
    PtInButton(POINT pt)
    {
        if (!IsWindow())
            return FALSE;
        RECT rc;
        GetWindowRect(&rc);
        INT cxEdge = ::GetSystemMetrics(SM_CXEDGE), cyEdge = ::GetSystemMetrics(SM_CYEDGE);
        ::InflateRect(&rc, max(cxEdge, 1), max(cyEdge, 1));
        return ::PtInRect(&rc, pt);
    }

#define SHOW_DESKTOP_TIMER_ID 999
#define SHOW_DESKTOP_TIMER_INTERVAL 200

    VOID
    StartHovering()
    {
        if (m_bHovering)
            return;

        m_bHovering = TRUE;
        SetTimer(SHOW_DESKTOP_TIMER_ID, SHOW_DESKTOP_TIMER_INTERVAL, NULL);
        InvalidateRect(NULL, TRUE);
        GetParent().PostMessage(WM_NCPAINT, 0, 0);
    }

    LRESULT
    OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        StartHovering();
        return 0;
    }

    LRESULT
    OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        if (wParam != SHOW_DESKTOP_TIMER_ID || !m_bHovering)
            return 0;

        POINT pt;
        ::GetCursorPos(&pt);
        if (!PtInButton(pt)) // The end of hovering?
        {
            m_bHovering = FALSE;
            KillTimer(SHOW_DESKTOP_TIMER_ID);
            InvalidateRect(NULL, TRUE);
            GetParent().PostMessage(WM_NCPAINT, 0, 0);
        }

        return 0;
    }

    LRESULT
    OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        if (m_hTheme)
        {
            CloseThemeData(m_hTheme);
            m_hTheme = NULL;
        }
        return 0;
    }

    BEGIN_MSG_MAP(TrayShowDesktopButton)
    MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
    MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChanged)
    MESSAGE_HANDLER(WM_THEMECHANGED, OnSettingChanged)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(TSDB_CLICK, OnClick)
    END_MSG_MAP()
};

