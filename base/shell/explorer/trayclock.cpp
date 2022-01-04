/*
 * ReactOS Explorer
 *
 * Copyright 2006 - 2007 Thomas Weidenmueller <w3seek@reactos.org>
 * Copyright 2018 Ged Murphy <gedmurphy@reactos.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"

/*
 * TrayClockWnd
 */

const struct
{
    BOOL IsTime;
    DWORD dwFormatFlags;
    LPCWSTR lpFormat;
} ClockWndFormats[] = {
    { TRUE, 0, NULL },
    { FALSE, 0, L"dddd" },
    { FALSE, DATE_SHORTDATE, NULL }
};
const UINT ClockWndFormatsCount = _ARRAYSIZE(ClockWndFormats);

#define CLOCKWND_FORMAT_COUNT ClockWndFormatsCount

static const WCHAR szTrayClockWndClass[] = L"TrayClockWClass";

class CTrayClockWnd :
    public CComCoClass<CTrayClockWnd>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CWindowImpl < CTrayClockWnd, CWindow, CControlWinTraits >,
    public IOleWindow
{
    HFONT hFont;
    COLORREF textColor;
    RECT rcText;
    SYSTEMTIME LocalTime;
    CTooltips m_tooltip;

    union
    {
        DWORD dwFlags;
        struct
        {
            DWORD IsTimerEnabled : 1;
            DWORD IsInitTimerEnabled : 1;
            DWORD LinesMeasured : 1;
            DWORD IsHorizontal : 1;
        };
    };
    DWORD LineSpacing;
    SIZE CurrentSize;
    WORD VisibleLines;
    SIZE LineSizes[CLOCKWND_FORMAT_COUNT];
    WCHAR szLines[CLOCKWND_FORMAT_COUNT][48];

public:
    CTrayClockWnd();
    virtual ~CTrayClockWnd();

private:
    LRESULT OnThemeChanged();
    LRESULT OnThemeChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    BOOL MeasureLines();
    WORD GetMinimumSize(IN BOOL Horizontal, IN OUT PSIZE pSize);
    VOID UpdateWnd();
    VOID Update();
    UINT CalculateDueTime();
    BOOL ResetTime();
    VOID CalibrateTimer();
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    VOID SetFont(IN HFONT hNewFont, IN BOOL bRedraw);
    LRESULT DrawBackground(HDC hdc);
    LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnGetMinimumSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetFont(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTaskbarSettingsChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnLButtonDblClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

public:

    HRESULT WINAPI GetWindow(HWND* phwnd)
    {
        if (!phwnd)
            return E_INVALIDARG;
        *phwnd = m_hWnd;
        return S_OK;
    }

    HRESULT WINAPI ContextSensitiveHelp(BOOL fEnterMode)
    {
        return E_NOTIMPL;
    }

    DECLARE_NOT_AGGREGATABLE(CTrayClockWnd)

    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CTrayClockWnd)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
    END_COM_MAP()

    DECLARE_WND_CLASS_EX(szTrayClockWndClass, CS_DBLCLKS, COLOR_3DFACE)

    BEGIN_MSG_MAP(CTrayClockWnd)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_PRINTCLIENT, OnPaint)
        MESSAGE_HANDLER(WM_THEMECHANGED, OnThemeChanged)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_SETFONT, OnSetFont)
        MESSAGE_HANDLER(TNWM_GETMINIMUMSIZE, OnGetMinimumSize)
        MESSAGE_HANDLER(TWM_SETTINGSCHANGED, OnTaskbarSettingsChanged)
        MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDblClick)
    END_MSG_MAP()

    HRESULT Initialize(IN HWND hWndParent);
};

#define ID_TRAYCLOCK_TIMER  0
#define ID_TRAYCLOCK_TIMER_INIT 1

#define TRAY_CLOCK_WND_SPACING_X    5
#define TRAY_CLOCK_WND_SPACING_Y    0

CTrayClockWnd::CTrayClockWnd() :
        hFont(NULL),
        dwFlags(0),
        LineSpacing(0),
        VisibleLines(0)
{
    ZeroMemory(&textColor, sizeof(textColor));
    ZeroMemory(&rcText, sizeof(rcText));
    ZeroMemory(&LocalTime, sizeof(LocalTime));
    ZeroMemory(&CurrentSize, sizeof(CurrentSize));
    ZeroMemory(LineSizes, sizeof(LineSizes));
    ZeroMemory(szLines, sizeof(szLines));
}
CTrayClockWnd::~CTrayClockWnd() { }

LRESULT CTrayClockWnd::OnThemeChanged()
{
    LOGFONTW clockFont;
    HTHEME clockTheme;
    HFONT hFont;

    clockTheme = OpenThemeData(m_hWnd, L"Clock");

    if (clockTheme)
    {
        GetThemeFont(clockTheme,
            NULL,
            CLP_TIME,
            0,
            TMT_FONT,
            &clockFont);

        hFont = CreateFontIndirectW(&clockFont);

        GetThemeColor(clockTheme,
            CLP_TIME,
            0,
            TMT_TEXTCOLOR,
            &textColor);

        if (this->hFont != NULL)
            DeleteObject(this->hFont);

        SetFont(hFont, FALSE);
    }
    else
    {
        /* We don't need to set a font here, our parent will use
            * WM_SETFONT to set the right one when themes are not enabled. */
        textColor = RGB(0, 0, 0);
    }

    CloseThemeData(clockTheme);

    return TRUE;
}

LRESULT CTrayClockWnd::OnThemeChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return OnThemeChanged();
}

BOOL CTrayClockWnd::MeasureLines()
{
    HDC hDC;
    HFONT hPrevFont;
    UINT c, i;
    BOOL bRet = TRUE;

    hDC = GetDC();
    if (hDC != NULL)
    {
        if (hFont)
            hPrevFont = (HFONT) SelectObject(hDC, hFont);

        for (i = 0; i < CLOCKWND_FORMAT_COUNT && bRet; i++)
        {
            if (szLines[i][0] != L'\0' &&
                !GetTextExtentPointW(hDC, szLines[i], wcslen(szLines[i]),
                                        &LineSizes[i]))
            {
                bRet = FALSE;
                break;
            }
        }

        if (hFont)
            SelectObject(hDC, hPrevFont);

        ReleaseDC(hDC);

        if (bRet)
        {
            LineSpacing = 0;

            /* calculate the line spacing */
            for (i = 0, c = 0; i < CLOCKWND_FORMAT_COUNT; i++)
            {
                if (LineSizes[i].cx > 0)
                {
                    LineSpacing += LineSizes[i].cy;
                    c++;
                }
            }

            if (c > 0)
            {
                /* We want a spacing of 1/2 line */
                LineSpacing = (LineSpacing / c) / 2;
            }

            return TRUE;
        }
    }

    return FALSE;
}

WORD CTrayClockWnd::GetMinimumSize(IN BOOL Horizontal, IN OUT PSIZE pSize)
{
    WORD iLinesVisible = 0;
    UINT i;
    SIZE szMax = { 0, 0 };

    if (!LinesMeasured)
        LinesMeasured = MeasureLines();

    if (!LinesMeasured)
        return 0;

    for (i = 0; i < CLOCKWND_FORMAT_COUNT; i++)
    {
        if (LineSizes[i].cx != 0)
        {
            if (iLinesVisible > 0)
            {
                if (Horizontal)
                {
                    if (szMax.cy + LineSizes[i].cy + (LONG) LineSpacing >
                        pSize->cy - (2 * TRAY_CLOCK_WND_SPACING_Y))
                    {
                        break;
                    }
                }
                else
                {
                    if (LineSizes[i].cx > pSize->cx - (2 * TRAY_CLOCK_WND_SPACING_X))
                        break;
                }

                /* Add line spacing */
                szMax.cy += LineSpacing;
            }

            iLinesVisible++;

            /* Increase maximum rectangle */
            szMax.cy += LineSizes[i].cy;
            if (LineSizes[i].cx > szMax.cx)
                szMax.cx = LineSizes[i].cx;
        }
    }

    szMax.cx += 2 * TRAY_CLOCK_WND_SPACING_X;
    szMax.cy += 2 * TRAY_CLOCK_WND_SPACING_Y;

    *pSize = szMax;

    return iLinesVisible;
}

VOID CTrayClockWnd::UpdateWnd()
{
    SIZE szPrevCurrent;
    UINT BufSize, i;
    INT iRet;
    RECT rcClient;

    ZeroMemory(LineSizes, sizeof(LineSizes));

    szPrevCurrent = CurrentSize;

    for (i = 0; i < CLOCKWND_FORMAT_COUNT; i++)
    {
        szLines[i][0] = L'\0';
        BufSize = _countof(szLines[0]);

        if (ClockWndFormats[i].IsTime)
        {
            iRet = GetTimeFormat(LOCALE_USER_DEFAULT,
                g_TaskbarSettings.bShowSeconds ? ClockWndFormats[i].dwFormatFlags : TIME_NOSECONDS,
                &LocalTime,
                ClockWndFormats[i].lpFormat,
                szLines[i],
                BufSize);
        }
        else
        {
            iRet = GetDateFormat(LOCALE_USER_DEFAULT,
                ClockWndFormats[i].dwFormatFlags,
                &LocalTime,
                ClockWndFormats[i].lpFormat,
                szLines[i],
                BufSize);
        }

        if (iRet != 0 && i == 0)
        {
            /* Set the window text to the time only */
            SetWindowText(szLines[i]);
        }
    }

    LinesMeasured = MeasureLines();

    if (LinesMeasured &&
        GetClientRect(&rcClient))
    {
        SIZE szWnd;

        szWnd.cx = rcClient.right;
        szWnd.cy = rcClient.bottom;

        VisibleLines = GetMinimumSize(IsHorizontal, &szWnd);
        CurrentSize = szWnd;
    }

    if (IsWindowVisible())
    {
        InvalidateRect(NULL, TRUE);

        if (szPrevCurrent.cx != CurrentSize.cx ||
            szPrevCurrent.cy != CurrentSize.cy)
        {
            /* Ask the parent to resize */
            NMHDR nmh = {GetParent(), 0, NTNWM_REALIGN};
            GetParent().SendMessage(WM_NOTIFY, 0, (LPARAM) &nmh);
        }
    }

    int iDateLength = GetDateFormat(LOCALE_USER_DEFAULT,
                                          DATE_LONGDATE,
                                          &LocalTime,
                                          NULL,
                                          NULL,
                                          0);
    if (iDateLength <= 0)
    {
        return;
    }

    WCHAR* szDate = new WCHAR[iDateLength];
    if (GetDateFormat(LOCALE_USER_DEFAULT,
                      DATE_LONGDATE,
                      &LocalTime,
                      NULL,
                      szDate,
                      iDateLength) > 0)
    {
        m_tooltip.UpdateTipText(m_hWnd,
                                reinterpret_cast<UINT_PTR>(m_hWnd),
                                szDate);
    }
    delete[] szDate;
}

VOID CTrayClockWnd::Update()
{
    GetLocalTime(&LocalTime);
    UpdateWnd();
}

UINT CTrayClockWnd::CalculateDueTime()
{
    UINT uiDueTime;

    GetLocalTime(&LocalTime);
    uiDueTime = 1000 - (UINT) LocalTime.wMilliseconds;
    if (!g_TaskbarSettings.bShowSeconds)
        uiDueTime += (59 - (UINT) LocalTime.wSecond) * 1000;

    return uiDueTime;
}

BOOL CTrayClockWnd::ResetTime()
{
    UINT uiDueTime;
    BOOL Ret;

    /* Disable all timers */
    if (IsTimerEnabled)
    {
        KillTimer(ID_TRAYCLOCK_TIMER);
        IsTimerEnabled = FALSE;
    }
    else if (IsInitTimerEnabled)
    {
        KillTimer(ID_TRAYCLOCK_TIMER_INIT);
    }

    uiDueTime = CalculateDueTime();

    /* Set the new timer */
    Ret = SetTimer(ID_TRAYCLOCK_TIMER_INIT, uiDueTime, NULL) != 0;
    IsInitTimerEnabled = Ret;

    return Ret;
}

VOID CTrayClockWnd::CalibrateTimer()
{
    UINT uiDueTime;
    BOOL Ret;
    UINT uiWait1, uiWait2;

    /* Kill the initialization timer */
    KillTimer(ID_TRAYCLOCK_TIMER_INIT);
    IsInitTimerEnabled = FALSE;

    uiDueTime = CalculateDueTime();

    if (g_TaskbarSettings.bShowSeconds)
    {
        uiWait1 = 1000 - 200;
        uiWait2 = 1000;
    }
    else
    {
        uiWait1 = 60 * 1000 - 200;
        uiWait2 = 60 * 1000;
    }

    if (uiDueTime > uiWait1)
    {
        /* The update of the clock will be up to 200 ms late, but that's
            acceptable. We're going to setup a timer that fires depending
            uiWait2. */
        Ret = SetTimer(ID_TRAYCLOCK_TIMER, uiWait2, NULL) != 0;
        IsTimerEnabled = Ret;
    }
    else
    {
        /* Recalibrate the timer and recalculate again when the current
            minute/second ends. */
        ResetTime();
    }

    /* Update the time */
    Update();
}

LRESULT CTrayClockWnd::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    /* Disable all timers */
    if (IsTimerEnabled)
    {
        KillTimer(ID_TRAYCLOCK_TIMER);
    }
    else if (IsInitTimerEnabled)
    {
        KillTimer(ID_TRAYCLOCK_TIMER_INIT);
    }

    return TRUE;
}

LRESULT CTrayClockWnd::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rcClient;
    HFONT hPrevFont;
    INT iPrevBkMode;
    UINT i, line;

    PAINTSTRUCT ps;
    HDC hDC = (HDC) wParam;

    if (wParam == 0)
    {
        hDC = BeginPaint(&ps);
    }

    if (hDC == NULL)
        return FALSE;

    if (LinesMeasured &&
        GetClientRect(&rcClient))
    {
        iPrevBkMode = SetBkMode(hDC, TRANSPARENT);

        SetTextColor(hDC, textColor);

        hPrevFont = (HFONT) SelectObject(hDC, hFont);

        rcClient.top = (rcClient.bottom - CurrentSize.cy) / 2;
        rcClient.bottom = rcClient.top + CurrentSize.cy;

        for (i = 0, line = 0;
                i < CLOCKWND_FORMAT_COUNT && line < VisibleLines;
                i++)
        {
            if (LineSizes[i].cx != 0)
            {
                TextOut(hDC,
                    (rcClient.right - LineSizes[i].cx) / 2,
                    rcClient.top + TRAY_CLOCK_WND_SPACING_Y,
                    szLines[i],
                    wcslen(szLines[i]));

                rcClient.top += LineSizes[i].cy + LineSpacing;
                line++;
            }
        }

        SelectObject(hDC, hPrevFont);

        SetBkMode(hDC, iPrevBkMode);
    }

    if (wParam == 0)
    {
        EndPaint(&ps);
    }

    return TRUE;
}

VOID CTrayClockWnd::SetFont(IN HFONT hNewFont, IN BOOL bRedraw)
{
    hFont = hNewFont;
    LinesMeasured = MeasureLines();
    if (bRedraw)
    {
        InvalidateRect(NULL, TRUE);
    }
}

LRESULT CTrayClockWnd::DrawBackground(HDC hdc)
{
    RECT rect;

    GetClientRect(&rect);
    DrawThemeParentBackground(m_hWnd, hdc, &rect);

    return TRUE;
}

LRESULT CTrayClockWnd::OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HDC hdc = (HDC) wParam;

    if (!IsAppThemed())
    {
        bHandled = FALSE;
        return 0;
    }

    return DrawBackground(hdc);
}

LRESULT CTrayClockWnd::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    switch (wParam)
    {
    case ID_TRAYCLOCK_TIMER:
        Update();
        break;

    case ID_TRAYCLOCK_TIMER_INIT:
        CalibrateTimer();
        break;
    }
    return TRUE;
}

LRESULT CTrayClockWnd::OnGetMinimumSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    IsHorizontal = (BOOL) wParam;

    return (LRESULT) GetMinimumSize((BOOL) wParam, (PSIZE) lParam) != 0;
}

LRESULT CTrayClockWnd::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return GetParent().SendMessage(uMsg, wParam, lParam);
}

LRESULT CTrayClockWnd::OnSetFont(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SetFont((HFONT) wParam, (BOOL) LOWORD(lParam));
    return TRUE;
}

LRESULT CTrayClockWnd::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_tooltip.Create(m_hWnd, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP);

    TOOLINFOW ti = { 0 };
    ti.cbSize = TTTOOLINFOW_V1_SIZE;
    ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    ti.hwnd = m_hWnd;
    ti.uId = reinterpret_cast<UINT_PTR>(m_hWnd);
    ti.lpszText = NULL;
    ti.lParam = NULL;

    m_tooltip.AddTool(&ti);

    if (!g_TaskbarSettings.sr.HideClock)
    {
        ResetTime();
    }

    /* Update the time */
    Update();

    return TRUE;
}

LRESULT CTrayClockWnd::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SIZE szClient;

    szClient.cx = LOWORD(lParam);
    szClient.cy = HIWORD(lParam);

    VisibleLines = GetMinimumSize(IsHorizontal, &szClient);
    CurrentSize = szClient;

    InvalidateRect(NULL, TRUE);
    return TRUE;
}

LRESULT CTrayClockWnd::OnTaskbarSettingsChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    BOOL bRealign = FALSE;

    TaskbarSettings* newSettings = (TaskbarSettings*)lParam;
    if (newSettings->bShowSeconds != g_TaskbarSettings.bShowSeconds)
    {
        g_TaskbarSettings.bShowSeconds = newSettings->bShowSeconds;
        if (!g_TaskbarSettings.sr.HideClock)
        {
            bRealign = TRUE;

            ResetTime();
        }
    }

    if (newSettings->sr.HideClock != g_TaskbarSettings.sr.HideClock)
    {
        g_TaskbarSettings.sr.HideClock = newSettings->sr.HideClock;
        ShowWindow(g_TaskbarSettings.sr.HideClock ? SW_HIDE : SW_SHOW);
        bRealign = TRUE;

        if (g_TaskbarSettings.sr.HideClock)
        {
            /* Disable all timers */
            if (IsTimerEnabled)
            {
                KillTimer(ID_TRAYCLOCK_TIMER);
                IsTimerEnabled = FALSE;
            }
            else if (IsInitTimerEnabled)
            {
                KillTimer(ID_TRAYCLOCK_TIMER_INIT);
                IsInitTimerEnabled = FALSE;
            }
        }
        else
        {
            ResetTime();
        }
    }

    if (bRealign)
    {
        /* Ask the parent to resize */
        NMHDR nmh = {GetParent(), 0, NTNWM_REALIGN};
        GetParent().SendMessage(WM_NOTIFY, 0, (LPARAM) &nmh);
        Update();
    }
    return 0;
}

LRESULT CTrayClockWnd::OnLButtonDblClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (IsWindowVisible())
    {
        //FIXME: use SHRunControlPanel
        ShellExecuteW(m_hWnd, NULL, L"timedate.cpl", NULL, NULL, SW_NORMAL);
    }
    return TRUE;
}

HRESULT CTrayClockWnd::Initialize(IN HWND hWndParent)
{
    IsHorizontal = TRUE;

    /* Create the window. The tray window is going to move it to the correct
        position and resize it as needed. */
    DWORD dwStyle = WS_CHILD | WS_CLIPSIBLINGS;
    if (!g_TaskbarSettings.sr.HideClock)
        dwStyle |= WS_VISIBLE;

    Create(hWndParent, 0, NULL, dwStyle);
    if (!m_hWnd)
        return E_FAIL;

    SetWindowTheme(m_hWnd, L"TrayNotify", NULL);

    return S_OK;

};

HRESULT CTrayClockWnd_CreateInstance(HWND hwndParent, REFIID riid, void **ppv)
{
    return ShellObjectCreatorInit<CTrayClockWnd>(hwndParent, riid, ppv);
}
