/*
 * PROJECT:     ReactOS Explorer
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Quick Settings panel (Windows 11 style): notifications, a
 *              calendar, network and Bluetooth toggles, brightness and the
 *              battery state, in one flyout above the clock. The volume has
 *              its own tray icon.
 */

#include "precomp.h"
#include <iphlpapi.h>

#define QS_CLASS        L"ReactOS_QuickSettings"
#define QS_WIDTH        360     /* at 96 DPI */
#define QS_PAD          16
#define QS_TOGGLE       48      /* round toggle buttons */
#define QS_ROW_H        40
#define QS_GAP          12

#define QS_CAL_CELL     38      /* calendar day cell */

/* ---- the panel ---------------------------------------------------------- */

class CQuickSettings :
    public CWindowImpl<CQuickSettings, CWindow, CWinTraits<WS_POPUP | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW | WS_EX_TOPMOST>>
{
    HFONT m_hFont = NULL, m_hFontBold = NULL, m_hFontSmall = NULL;
    HWND m_hwndTip = NULL;
    /* calendar (drawn here, Windows 11 style): shown month, and its parts */
    INT m_CalYear = 0, m_CalMonth = 0;
    RECT m_rcCal = { 0 }, m_rcCalTitle = { 0 }, m_rcCalPrev = { 0 }, m_rcCalNext = { 0 };
    BOOL m_bConnected = FALSE;
    WCHAR m_szNetwork[128] = L"", m_szBattery[64] = L"";
    RECT m_rcNetTile = { 0 }, m_rcBtTile = { 0 }, m_rcBrightRow = { 0 },
         m_rcNotif = { 0 }, m_rcCalendarHdr = { 0 }, m_rcFooterDate = { 0 }, m_rcFooterSettings = { 0 };
    INT m_cyTotal = 0;
    INT m_Dpi = 96;

public:
    DWORD m_dwHiddenTick = 0;

    DECLARE_WND_CLASS_EX(QS_CLASS, CS_DROPSHADOW, COLOR_WINDOW)

    INT S(INT v) { return MulDiv(v, m_Dpi, 96); }

    VOID CreateFonts()
    {
        LOGFONTW lf = { 0 };
        HDC hdc = ::GetDC(NULL);
        m_Dpi = GetDeviceCaps(hdc, LOGPIXELSY);
        ::ReleaseDC(NULL, hdc);

        StringCchCopyW(lf.lfFaceName, _countof(lf.lfFaceName), L"Selawik");
        lf.lfQuality = DEFAULT_QUALITY;
        lf.lfHeight = -MulDiv(9, m_Dpi, 72);
        lf.lfWeight = FW_NORMAL;
        m_hFont = CreateFontIndirectW(&lf);
        lf.lfWeight = FW_SEMIBOLD;
        StringCchCopyW(lf.lfFaceName, _countof(lf.lfFaceName), L"Selawik Semibold");
        m_hFontBold = CreateFontIndirectW(&lf);
        lf.lfWeight = FW_NORMAL;
        StringCchCopyW(lf.lfFaceName, _countof(lf.lfFaceName), L"Selawik");
        lf.lfHeight = -MulDiv(8, m_Dpi, 72);
        m_hFontSmall = CreateFontIndirectW(&lf);
    }

    VOID RefreshStatus()
    {
        m_CalYear = 0;      /* the calendar opens on the current month */

        /* Network: connected if an adapter has an address */
        ULONG cb = 0;
        m_bConnected = FALSE;
        StringCchCopyW(m_szNetwork, _countof(m_szNetwork), L"Not connected");
        if (GetAdaptersInfo(NULL, &cb) == ERROR_BUFFER_OVERFLOW)
        {
            PIP_ADAPTER_INFO pInfo = (PIP_ADAPTER_INFO)HeapAlloc(GetProcessHeap(), 0, cb);
            if (pInfo && GetAdaptersInfo(pInfo, &cb) == NO_ERROR)
            {
                for (PIP_ADAPTER_INFO p = pInfo; p; p = p->Next)
                {
                    if (strcmp(p->IpAddressList.IpAddress.String, "0.0.0.0") != 0)
                    {
                        m_bConnected = TRUE;
                        StringCchPrintfW(m_szNetwork, _countof(m_szNetwork), L"Connected (%S)",
                                         p->IpAddressList.IpAddress.String);
                        break;
                    }
                }
            }
            if (pInfo)
                HeapFree(GetProcessHeap(), 0, pInfo);
        }

        /* Battery */
        SYSTEM_POWER_STATUS sps;
        if (GetSystemPowerStatus(&sps) && !(sps.BatteryFlag & 128) && sps.BatteryLifePercent <= 100)
        {
            StringCchPrintfW(m_szBattery, _countof(m_szBattery), L"Battery %u%%%s", sps.BatteryLifePercent,
                             sps.ACLineStatus == 1 ? L", charging" : L"");
        }
        else
        {
            StringCchCopyW(m_szBattery, _countof(m_szBattery), L"Plugged in");
        }
    }

    /* Positions of everything, and the window height.
     * Top to bottom like Windows 11: notifications, calendar, quick toggles
     * (round icon buttons), brightness and volume sliders, footer. */
    VOID Layout()
    {
        INT W = S(QS_WIDTH), x = S(QS_PAD), y = S(QS_PAD);

        /* notifications */
        SetRect(&m_rcNotif, x, y, W - S(QS_PAD), y + S(64));
        y += S(64) + S(QS_GAP);

        /* calendar */
        SetRect(&m_rcCalendarHdr, x, y, W - S(QS_PAD), y + S(28));
        y += S(28);
        /* month title with the ˄ ˅ buttons on its right, weekday names, 6 weeks */
        SetRect(&m_rcCal, x, y, W - S(QS_PAD), y + S(36) + S(28) + 6 * S(QS_CAL_CELL));
        SetRect(&m_rcCalTitle, x, y, m_rcCal.right - S(80), y + S(36));
        SetRect(&m_rcCalNext, m_rcCal.right - S(36), y + S(2), m_rcCal.right, y + S(34));
        SetRect(&m_rcCalPrev, m_rcCalNext.left - S(38), y + S(2), m_rcCalNext.left - S(2), y + S(34));
        y = m_rcCal.bottom + S(QS_GAP);

        /* toggles: icon buttons with a short caption below */
        SetRect(&m_rcNetTile, x, y, x + S(QS_TOGGLE), y + S(QS_TOGGLE));
        SetRect(&m_rcBtTile, x + S(QS_TOGGLE) + S(QS_GAP), y,
                x + 2 * S(QS_TOGGLE) + S(QS_GAP), y + S(QS_TOGGLE));
        y += S(QS_TOGGLE) + S(20) + S(QS_GAP) / 2;

        /* brightness slider with its icon on the left (volume stays in the taskbar) */
        SetRect(&m_rcBrightRow, x, y, W - S(QS_PAD), y + S(QS_ROW_H));
        y += S(QS_ROW_H) + S(QS_GAP) / 2;

        /* footer */
        SetRect(&m_rcFooterDate, x, y, W / 2, y + S(32));
        SetRect(&m_rcFooterSettings, W / 2, y, W - S(QS_PAD), y + S(32));
        y += S(32) + S(QS_PAD) / 2;

        m_cyTotal = y;
    }

    /* Hovering a toggle tells its state ("Connected (10.0.2.15)") */
    VOID UpdateTips()
    {
        TTTOOLINFOW ti = { sizeof(ti), TTF_SUBCLASS, m_hWnd, 1 };
        ti.rect = m_rcNetTile;
        ti.lpszText = m_szNetwork;
        ::SendMessageW(m_hwndTip, TTM_DELTOOLW, 0, (LPARAM)&ti);
        ::SendMessageW(m_hwndTip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
        ti.uId = 2;
        ti.rect = m_rcBtTile;
        ti.lpszText = (LPWSTR)L"Bluetooth: not available";
        ::SendMessageW(m_hwndTip, TTM_DELTOOLW, 0, (LPARAM)&ti);
        ::SendMessageW(m_hwndTip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
    }

    VOID ShowAboveTaskbar()
    {
        RECT rcWork;
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &rcWork, 0);
        Layout();
        UpdateTips();
        INT W = S(QS_WIDTH), H = min(m_cyTotal, (INT)(rcWork.bottom - rcWork.top) - S(24));
        SetWindowPos(HWND_TOPMOST, rcWork.right - W - S(12), rcWork.bottom - H - S(12), W, H, SWP_SHOWWINDOW);

        /* Rounded corners like Windows 11 */
        SetWindowRgn(CreateRoundRectRgn(0, 0, W + 1, H + 1, S(16), S(16)), TRUE);
        ::SetForegroundWindow(m_hWnd);
        InvalidateRect(NULL, TRUE);
    }

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_WIN95_CLASSES };
        InitCommonControlsEx(&icc);
        CreateFonts();

        m_hwndTip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                                    0, 0, 0, 0, m_hWnd, NULL, hExplorerInstance, NULL);
        return 0;
    }

    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        DeleteObject(m_hFont);
        DeleteObject(m_hFontBold);
        DeleteObject(m_hFontSmall);
        return 0;
    }

    VOID Card(HDC hdc, const RECT *prc, COLORREF crFill)
    {
        HBRUSH hbr = CreateSolidBrush(crFill);
        HPEN hpen = CreatePen(PS_SOLID, 1, UIC(RGB(229, 229, 229)));
        HGDIOBJ ob = SelectObject(hdc, hbr), op = SelectObject(hdc, hpen);
        RoundRect(hdc, prc->left, prc->top, prc->right, prc->bottom, S(8), S(8));
        SelectObject(hdc, ob);
        SelectObject(hdc, op);
        DeleteObject(hbr);
        DeleteObject(hpen);
    }

    VOID Text(HDC hdc, LPCWSTR psz, RECT rc, HFONT hFont, COLORREF cr, UINT uFormat)
    {
        SelectObject(hdc, hFont);
        SetTextColor(hdc, cr);
        DrawTextW(hdc, psz, -1, &rc, uFormat | DT_NOPREFIX);
    }

    /* ---- small line-art glyphs, drawn so they scale with the DPI ---- */

    HPEN GlyphPen(COLORREF cr)
    {
        LOGBRUSH lb = { BS_SOLID, cr, 0 };
        return ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND | PS_JOIN_ROUND, max(2, S(2)), &lb, 0, NULL);
    }

    /* Wi-Fi: a dot and three arcs opening upwards */
    VOID GlyphNetwork(HDC hdc, INT cx, INT cy, COLORREF cr)
    {
        HPEN hpen = GlyphPen(cr);
        HGDIOBJ op = SelectObject(hdc, hpen), ob = SelectObject(hdc, GetStockObject(NULL_BRUSH));
        INT by = cy + S(7);
        for (INT r = S(5); r <= S(15); r += S(5))
            Arc(hdc, cx - r, by - r, cx + r, by + r, cx + r, by - r * 7 / 10, cx - r, by - r * 7 / 10);
        SelectObject(hdc, ob);
        HBRUSH hbr = CreateSolidBrush(cr);
        ob = SelectObject(hdc, hbr);
        SelectObject(hdc, GetStockObject(NULL_PEN));
        Ellipse(hdc, cx - S(2), by - S(2), cx + S(3), by + S(3));
        SelectObject(hdc, ob);
        SelectObject(hdc, op);
        DeleteObject(hbr);
        DeleteObject(hpen);
    }

    /* Bluetooth rune */
    VOID GlyphBluetooth(HDC hdc, INT cx, INT cy, COLORREF cr)
    {
        HPEN hpen = GlyphPen(cr);
        HGDIOBJ op = SelectObject(hdc, hpen);
        POINT pt[] = {
            { cx - S(6), cy - S(6) }, { cx + S(6), cy + S(5) }, { cx, cy + S(11) },
            { cx, cy - S(11) }, { cx + S(6), cy - S(5) }, { cx - S(6), cy + S(6) } };
        Polyline(hdc, pt, _countof(pt));
        SelectObject(hdc, op);
        DeleteObject(hpen);
    }

    /* Sun: circle and eight rays */
    VOID GlyphSun(HDC hdc, INT cx, INT cy, COLORREF cr)
    {
        static const INT d[8][2] = { {0,-1}, {7,-7}, {1,0}, {7,7}, {0,1}, {-7,7}, {-1,0}, {-7,-7} };
        HPEN hpen = GlyphPen(cr);
        HGDIOBJ op = SelectObject(hdc, hpen), ob = SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Ellipse(hdc, cx - S(4), cy - S(4), cx + S(5), cy + S(5));
        for (INT i = 0; i < 8; i++)
        {
            /* diagonal rays use 7/10 so all rays have the same length */
            INT fx = abs(d[i][0]) == 7 ? d[i][0] : d[i][0] * 10, fy = abs(d[i][1]) == 7 ? d[i][1] : d[i][1] * 10;
            MoveToEx(hdc, cx + S(7) * fx / 10, cy + S(7) * fy / 10, NULL);
            LineTo(hdc, cx + S(10) * fx / 10, cy + S(10) * fy / 10);
        }
        SelectObject(hdc, op);
        SelectObject(hdc, ob);
        DeleteObject(hpen);
    }

    /* Windows 11 slider: thin track, accent filled part, round thumb */
    VOID Slider(HDC hdc, INT x1, INT x2, INT y, INT Percent, BOOL bEnabled)
    {
        COLORREF crFill = bEnabled ? GetSysColor(COLOR_HOTLIGHT) : UIC(RGB(170, 170, 170));
        INT xPos = x1 + MulDiv(x2 - x1, Percent, 100);
        RECT rcTrack = { x1, y - S(2), x2, y + S(2) };
        HBRUSH hbr = CreateSolidBrush(UIC(RGB(200, 200, 200)));
        FillRect(hdc, &rcTrack, hbr);
        DeleteObject(hbr);
        rcTrack.right = xPos;
        hbr = CreateSolidBrush(crFill);
        FillRect(hdc, &rcTrack, hbr);
        DeleteObject(hbr);

        HBRUSH hbrThumb = CreateSolidBrush(UIC(RGB(255, 255, 255)));
        HPEN hpen = CreatePen(PS_SOLID, 1, UIC(RGB(200, 200, 200)));
        HGDIOBJ ob = SelectObject(hdc, hbrThumb), op = SelectObject(hdc, hpen);
        Ellipse(hdc, xPos - S(10), y - S(10), xPos + S(10), y + S(10));
        SelectObject(hdc, ob);
        DeleteObject(hbrThumb);
        hbr = CreateSolidBrush(crFill);
        SelectObject(hdc, hbr);
        SelectObject(hdc, GetStockObject(NULL_PEN));
        Ellipse(hdc, xPos - S(5), y - S(5), xPos + S(6), y + S(6));
        SelectObject(hdc, ob);
        SelectObject(hdc, op);
        DeleteObject(hbr);
        DeleteObject(hpen);
    }

    /* Round toggle button (accent filled when on) with a caption below */
    VOID Toggle(HDC hdc, const RECT *prc, BOOL bOn, BOOL bAvailable, INT Glyph, LPCWSTR pszCaption)
    {
        COLORREF crGlyph = bOn ? RGB(255, 255, 255) : (bAvailable ? UIC(RGB(26, 26, 26)) : UIC(RGB(160, 160, 160)));
        HBRUSH hbr = CreateSolidBrush(bOn ? GetSysColor(COLOR_HOTLIGHT) : UIC(RGB(251, 251, 251)));
        HPEN hpen = CreatePen(PS_SOLID, 1, bOn ? GetSysColor(COLOR_HOTLIGHT) : UIC(RGB(222, 222, 222)));
        HGDIOBJ ob = SelectObject(hdc, hbr), op = SelectObject(hdc, hpen);
        RoundRect(hdc, prc->left, prc->top, prc->right, prc->bottom, S(12), S(12));
        SelectObject(hdc, ob);
        SelectObject(hdc, op);
        DeleteObject(hbr);
        DeleteObject(hpen);

        INT cx = (prc->left + prc->right) / 2, cy = (prc->top + prc->bottom) / 2;
        if (Glyph == 0)
            GlyphNetwork(hdc, cx, cy, crGlyph);
        else
            GlyphBluetooth(hdc, cx, cy, crGlyph);

        RECT rc = { prc->left - S(QS_GAP) / 2, prc->bottom + S(2), prc->right + S(QS_GAP) / 2, prc->bottom + S(20) };
        Text(hdc, pszCaption, rc, m_hFontSmall, UIC(RGB(64, 64, 64)), DT_SINGLELINE | DT_CENTER | DT_TOP | DT_END_ELLIPSIS);
    }

    /* ---- calendar ---- */

    static INT DaysInMonth(INT Year, INT Month)
    {
        static const INT Days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
        BOOL bLeap = (Year % 4 == 0 && Year % 100 != 0) || Year % 400 == 0;
        return Days[Month - 1] + (Month == 2 && bLeap);
    }

    /* 0 = Sunday */
    static INT DayOfWeek(INT Year, INT Month, INT Day)
    {
        static const INT t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
        if (Month < 3)
            Year--;
        return (Year + Year / 4 - Year / 100 + Year / 400 + t[Month - 1] + Day) % 7;
    }

    VOID ShowMonth(INT Delta)
    {
        INT m = m_CalYear * 12 + (m_CalMonth - 1) + Delta;
        m_CalYear = m / 12;
        m_CalMonth = m % 12 + 1;
        InvalidateRect(&m_rcCal, FALSE);
    }

    /* small chevron, pointing up (bUp) or down */
    VOID Chevron(HDC hdc, const RECT *prc, BOOL bUp, COLORREF cr)
    {
        LOGBRUSH lb = { BS_SOLID, cr, 0 };
        HPEN hpen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND | PS_JOIN_ROUND, max(1, S(1) + S(1) / 2), &lb, 0, NULL);
        HGDIOBJ op = SelectObject(hdc, hpen);
        INT cx = (prc->left + prc->right) / 2, cy = (prc->top + prc->bottom) / 2, d = S(4);
        POINT pt[3] = { { cx - d, cy + (bUp ? d / 2 : -d / 2) }, { cx, cy + (bUp ? -d / 2 : d / 2) },
                        { cx + d, cy + (bUp ? d / 2 : -d / 2) } };
        Polyline(hdc, pt, 3);
        SelectObject(hdc, op);
        DeleteObject(hpen);
    }

    VOID PaintCalendar(HDC hdc)
    {
        WCHAR sz[64];
        SYSTEMTIME st, today;
        GetLocalTime(&today);
        if (!m_CalYear)
        {
            m_CalYear = today.wYear;
            m_CalMonth = today.wMonth;
        }

        HBRUSH hbr = CreateSolidBrush(UIC(RGB(243, 243, 243)));
        FillRect(hdc, &m_rcCal, hbr);
        DeleteObject(hbr);

        /* "October 2026" on the left, ˄ ˅ on the right */
        ZeroMemory(&st, sizeof(st));
        st.wYear = (WORD)m_CalYear;
        st.wMonth = (WORD)m_CalMonth;
        st.wDay = 1;
        GetDateFormatW(LOCALE_USER_DEFAULT, DATE_YEARMONTH, &st, NULL, sz, _countof(sz));
        RECT rc = m_rcCalTitle;
        rc.left += S(6);
        Text(hdc, sz, rc, m_hFontBold, UIC(RGB(26, 26, 26)), DT_SINGLELINE | DT_VCENTER);
        Chevron(hdc, &m_rcCalPrev, TRUE, UIC(RGB(64, 64, 64)));
        Chevron(hdc, &m_rcCalNext, FALSE, UIC(RGB(64, 64, 64)));

        /* weekday names, two letters: Su Mo Tu ... */
        INT cxCell = (m_rcCal.right - m_rcCal.left) / 7, x0 = m_rcCal.left + ((m_rcCal.right - m_rcCal.left) - 7 * cxCell) / 2;
        INT y = m_rcCal.top + S(36);
        for (INT i = 0; i < 7; i++)
        {
            WCHAR szDay[16];
            GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SABBREVDAYNAME1 + (i + 6) % 7, szDay, _countof(szDay));
            szDay[2] = UNICODE_NULL;
            RECT rcDay = { x0 + i * cxCell, y, x0 + (i + 1) * cxCell, y + S(28) };
            Text(hdc, szDay, rcDay, m_hFontSmall, UIC(RGB(26, 26, 26)), DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        }
        y += S(28);

        /* 6 weeks; days of the previous/next month dimmed; today in an accent circle */
        INT nFirst = DayOfWeek(m_CalYear, m_CalMonth, 1);
        INT PrevYear = m_CalMonth == 1 ? m_CalYear - 1 : m_CalYear, PrevMonth = m_CalMonth == 1 ? 12 : m_CalMonth - 1;
        INT nDays = DaysInMonth(m_CalYear, m_CalMonth), nPrevDays = DaysInMonth(PrevYear, PrevMonth);
        for (INT cell = 0; cell < 42; cell++)
        {
            INT Day = cell - nFirst + 1;
            BOOL bThisMonth = Day >= 1 && Day <= nDays;
            if (Day < 1)
                Day += nPrevDays;
            else if (Day > nDays)
                Day -= nDays;
            RECT rcCell = { x0 + (cell % 7) * cxCell, y + (cell / 7) * S(QS_CAL_CELL),
                            x0 + (cell % 7 + 1) * cxCell, y + (cell / 7 + 1) * S(QS_CAL_CELL) };
            BOOL bToday = bThisMonth && Day == today.wDay && m_CalMonth == today.wMonth && m_CalYear == today.wYear;
            COLORREF crText = bThisMonth ? UIC(RGB(26, 26, 26)) : UIC(RGB(150, 150, 150));
            if (bToday)
            {
                INT r = min(cxCell, S(QS_CAL_CELL)) / 2 - S(2);
                INT cx = (rcCell.left + rcCell.right) / 2, cy = (rcCell.top + rcCell.bottom) / 2;
                HBRUSH hbrToday = CreateSolidBrush(GetSysColor(COLOR_HOTLIGHT));
                HGDIOBJ ob = SelectObject(hdc, hbrToday), op = SelectObject(hdc, GetStockObject(NULL_PEN));
                Ellipse(hdc, cx - r, cy - r, cx + r + 1, cy + r + 1);
                SelectObject(hdc, ob);
                SelectObject(hdc, op);
                DeleteObject(hbrToday);
                crText = RGB(255, 255, 255);   /* on the accent circle */
            }
            StringCchPrintfW(sz, _countof(sz), L"%d", Day);
            Text(hdc, sz, rcCell, bToday ? m_hFontBold : m_hFont, crText, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        }
    }

    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(&ps);
        RECT rc, rcClient;
        WCHAR sz[128];

        GetClientRect(&rcClient);
        HBRUSH hbrBack = CreateSolidBrush(UIC(RGB(243, 243, 243)));
        FillRect(hdc, &rcClient, hbrBack);
        DeleteObject(hbrBack);
        SetBkMode(hdc, TRANSPARENT);

        /* Toggles */
        Toggle(hdc, &m_rcNetTile, m_bConnected, TRUE, 0, m_bConnected ? L"Network" : L"Offline");
        Toggle(hdc, &m_rcBtTile, FALSE, FALSE, 1, L"Bluetooth");

        /* Brightness: drawn slider, disabled when the display has no brightness control */
        GlyphSun(hdc, m_rcBrightRow.left + S(12), (m_rcBrightRow.top + m_rcBrightRow.bottom) / 2, UIC(RGB(160, 160, 160)));
        Slider(hdc, m_rcBrightRow.left + S(36), m_rcBrightRow.right - S(8),
               (m_rcBrightRow.top + m_rcBrightRow.bottom) / 2, 100, FALSE);

        /* Notifications */
        Card(hdc, &m_rcNotif, UIC(RGB(251, 251, 251)));
        rc = m_rcNotif;
        InflateRect(&rc, -S(12), -S(8));
        Text(hdc, L"Notifications", rc, m_hFontBold, UIC(RGB(26, 26, 26)), DT_SINGLELINE | DT_TOP);
        Text(hdc, L"No new notifications", rc, m_hFontSmall, UIC(RGB(96, 96, 96)), DT_SINGLELINE | DT_BOTTOM);

        /* Calendar header: today's date */
        SYSTEMTIME st;
        GetLocalTime(&st);
        GetDateFormatW(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, sz, _countof(sz));
        Text(hdc, sz, m_rcCalendarHdr, m_hFontBold, UIC(RGB(26, 26, 26)), DT_SINGLELINE | DT_VCENTER);

        PaintCalendar(hdc);

        /* Footer */
        Text(hdc, m_szBattery, m_rcFooterDate, m_hFontSmall, UIC(RGB(96, 96, 96)), DT_SINGLELINE | DT_VCENTER);
        Text(hdc, L"All settings \x203A", m_rcFooterSettings, m_hFont, GetSysColor(COLOR_HOTLIGHT),
             DT_SINGLELINE | DT_VCENTER | DT_RIGHT);

        /* Thin border */
        HPEN hpen = CreatePen(PS_SOLID, 1, UIC(RGB(200, 200, 200)));
        HGDIOBJ op = SelectObject(hdc, hpen), ob = SelectObject(hdc, GetStockObject(NULL_BRUSH));
        RoundRect(hdc, 0, 0, rcClient.right, rcClient.bottom, S(16), S(16));
        SelectObject(hdc, op);
        SelectObject(hdc, ob);
        DeleteObject(hpen);

        EndPaint(&ps);
        return 0;
    }

    LRESULT OnCtlColor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        static HBRUSH s_hbr = CreateSolidBrush(UIC(RGB(243, 243, 243)));
        SetBkColor((HDC)wParam, UIC(RGB(243, 243, 243)));
        return (LRESULT)s_hbr;
    }

    LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        if (PtInRect(&m_rcNetTile, pt))
        {
            Hide();
            ShellExecuteW(NULL, NULL, L"rundll32.exe", L"shell32.dll,Control_RunDLL ncpa.cpl", NULL, SW_SHOWNORMAL);
        }
        else if (PtInRect(&m_rcCalPrev, pt))
        {
            ShowMonth(-1);
        }
        else if (PtInRect(&m_rcCalNext, pt))
        {
            ShowMonth(1);
        }
        else if (PtInRect(&m_rcCalendarHdr, pt))
        {
            Hide();
            ShellExecuteW(NULL, NULL, L"rundll32.exe", L"shell32.dll,Control_RunDLL timedate.cpl", NULL, SW_SHOWNORMAL);
        }
        else if (PtInRect(&m_rcFooterSettings, pt))
        {
            Hide();
            ShellExecuteW(NULL, NULL, L"settings.exe", NULL, NULL, SW_SHOWNORMAL);
        }
        return 0;
    }

    VOID Hide()
    {
        ShowWindow(SW_HIDE);
        m_dwHiddenTick = GetTickCount();
    }

    LRESULT OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        /* Like Windows 11: clicking anywhere else closes the panel */
        if (LOWORD(wParam) == WA_INACTIVE && IsWindowVisible())
            Hide();
        return 0;
    }

    LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (wParam == VK_ESCAPE)
            Hide();
        return 0;
    }

    BEGIN_MSG_MAP(CQuickSettings)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColor)
        MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
        MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
    END_MSG_MAP()
};

static CQuickSettings *g_pQuickSettings = NULL;

/* Click on the clock (or the system icons): open, or close if open */
VOID QuickSettings_Toggle(HWND hwndOwner)
{
    if (!g_pQuickSettings)
    {
        g_pQuickSettings = new CQuickSettings();
        if (!g_pQuickSettings->Create(hwndOwner, NULL, NULL))
        {
            delete g_pQuickSettings;
            g_pQuickSettings = NULL;
            return;
        }
    }

    /* The click that deactivated (= closed) the panel must not reopen it */
    if (g_pQuickSettings->IsWindowVisible() || GetTickCount() - g_pQuickSettings->m_dwHiddenTick < 400)
    {
        g_pQuickSettings->Hide();
        return;
    }

    g_pQuickSettings->RefreshStatus();
    g_pQuickSettings->ShowAboveTaskbar();
}
