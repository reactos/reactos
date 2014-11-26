/*
 * ReactOS Explorer
 *
 * Copyright 2006 - 2007 Thomas Weidenmueller <w3seek@reactos.org>
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

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

/*
 * SysPagerWnd
 */
static const WCHAR szSysPagerWndClass [] = TEXT("SysPager");

// Data comes from shell32/systray.cpp -> TrayNotifyCDS_Dummy
typedef struct _SYS_PAGER_COPY_DATA
{
    DWORD           cookie;
    DWORD           notify_code;
    NOTIFYICONDATA  nicon_data;
} SYS_PAGER_COPY_DATA, *PSYS_PAGER_COPY_DATA;

class CNotifyToolbar :
    public CToolbar<NOTIFYICONDATA>
{
    static const int ICON_SIZE = 16;

    HIMAGELIST SysIcons;
    int VisibleButtonCount;

public:
    CNotifyToolbar() :
        SysIcons(NULL),
        VisibleButtonCount(0)
    {
    }

    ~CNotifyToolbar()
    {
    }

    int GetVisibleButtonCount()
    {
        return VisibleButtonCount;
    }

    int FindItemByIconData(IN CONST NOTIFYICONDATA *iconData, NOTIFYICONDATA ** pdata)
    {
        int count = GetButtonCount();

        for (int i = 0; i < count; i++)
        {
            NOTIFYICONDATA * data;

            data = GetItemData(i);

            if (data->hWnd == iconData->hWnd &&
                data->uID == iconData->uID)
            {
                if (pdata)
                    *pdata = data;
                return i;
            }
        }

        return -1;
    }

    VOID AddButton(IN CONST NOTIFYICONDATA *iconData)
    {
        TBBUTTON tbBtn;
        NOTIFYICONDATA * notifyItem;
        WCHAR text [] = TEXT("");

        int index = FindItemByIconData(iconData, &notifyItem);
        if (index >= 0)
        {
            UpdateButton(iconData);
            return;
        }

        notifyItem = new NOTIFYICONDATA();
        ZeroMemory(notifyItem, sizeof(*notifyItem));

        notifyItem->hWnd = iconData->hWnd;
        notifyItem->uID = iconData->uID;

        tbBtn.fsState = TBSTATE_ENABLED;
        tbBtn.fsStyle = BTNS_NOPREFIX;
        tbBtn.dwData = (DWORD_PTR)notifyItem;
        tbBtn.iString = (INT_PTR) text;
        tbBtn.idCommand = GetButtonCount();

        if (iconData->uFlags & NIF_MESSAGE)
        {
            notifyItem->uCallbackMessage = iconData->uCallbackMessage;
        }

        if (iconData->uFlags & NIF_ICON)
        {
            tbBtn.iBitmap = ImageList_AddIcon(SysIcons, iconData->hIcon);
        }

        if (iconData->uFlags & NIF_TIP)
        {
            StringCchCopy(notifyItem->szTip, _countof(notifyItem->szTip), iconData->szTip);
        }

        VisibleButtonCount++;
        if (iconData->uFlags & NIF_STATE)
        {
            notifyItem->dwState &= ~iconData->dwStateMask;
            notifyItem->dwState |= (iconData->dwState & iconData->dwStateMask);
            if (notifyItem->dwState & NIS_HIDDEN)
            {
                tbBtn.fsState |= TBSTATE_HIDDEN;
                VisibleButtonCount--;
            }

        }

        /* TODO: support NIF_INFO, NIF_GUID, NIF_REALTIME, NIF_SHOWTIP */

        CToolbar::AddButton(&tbBtn);
        SetButtonSize(ICON_SIZE, ICON_SIZE);
    }

    VOID UpdateButton(IN CONST NOTIFYICONDATA *iconData)
    {
        NOTIFYICONDATA * notifyItem;
        TBBUTTONINFO tbbi = { 0 };

        int index = FindItemByIconData(iconData, &notifyItem);
        if (index < 0)
        {
            AddButton(iconData);
            return;
        }

        tbbi.cbSize = sizeof(tbbi);
        tbbi.dwMask = TBIF_BYINDEX | TBIF_COMMAND;
        tbbi.idCommand = index;

        if (iconData->uFlags & NIF_MESSAGE)
        {
            notifyItem->uCallbackMessage = iconData->uCallbackMessage;
        }

        if (iconData->uFlags & NIF_ICON)
        {
            tbbi.dwMask |= TBIF_IMAGE;
            tbbi.iImage = ImageList_AddIcon(SysIcons, iconData->hIcon);
        }

        if (iconData->uFlags & NIF_TIP)
        {
            StringCchCopy(notifyItem->szTip, _countof(notifyItem->szTip), iconData->szTip);
        }

        if (iconData->uFlags & NIF_STATE)
        {
            if (iconData->dwStateMask & NIS_HIDDEN &&
                (notifyItem->dwState & NIS_HIDDEN) != (iconData->dwState & NIS_HIDDEN))
            {
                tbbi.dwMask |= TBIF_STATE;
                if (iconData->dwState & NIS_HIDDEN)
                {
                    tbbi.fsState |= TBSTATE_HIDDEN;
                    VisibleButtonCount--;
                }
                else
                {
                    tbbi.fsState &= ~TBSTATE_HIDDEN;
                    VisibleButtonCount++;
                }
            }

            notifyItem->dwState &= ~iconData->dwStateMask;
            notifyItem->dwState |= (iconData->dwState & iconData->dwStateMask);
        }

        /* TODO: support NIF_INFO, NIF_GUID, NIF_REALTIME, NIF_SHOWTIP */

        SetButtonInfo(index, &tbbi);
    }

    VOID RemoveButton(IN CONST NOTIFYICONDATA *iconData)
    {
        NOTIFYICONDATA * notifyItem;

        int index = FindItemByIconData(iconData, &notifyItem);
        if (index < 0)
            return;

        DeleteButton(index);
        delete notifyItem;
    }

    VOID GetTooltipText(int index, LPTSTR szTip, DWORD cchTip)
    {
        NOTIFYICONDATA * notifyItem;
        notifyItem = GetItemData(index);

        if (notifyItem)
        {
            StringCchCopy(szTip, cchTip, notifyItem->szTip);
        }
    }

private:

    VOID SendMouseEvent(IN WORD wIndex, IN UINT uMsg, IN WPARAM wParam)
    {
        static LPCWSTR eventNames [] = {
            L"WM_MOUSEMOVE",
            L"WM_LBUTTONDOWN",
            L"WM_LBUTTONUP",
            L"WM_LBUTTONDBLCLK",
            L"WM_RBUTTONDOWN",
            L"WM_RBUTTONUP",
            L"WM_RBUTTONDBLCLK",
            L"WM_MBUTTONDOWN",
            L"WM_MBUTTONUP",
            L"WM_MBUTTONDBLCLK",
            L"WM_MOUSEWHEEL",
            L"WM_XBUTTONDOWN",
            L"WM_XBUTTONUP",
            L"WM_XBUTTONDBLCLK"
        };

        NOTIFYICONDATA * notifyItem = GetItemData(wIndex);

        if (!::IsWindow(notifyItem->hWnd))
            return;

        if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
        {
            TRACE("Sending message %S from button %d to %p (msg=%x, w=%x, l=%x)...\n",
                     eventNames[uMsg - WM_MOUSEFIRST], wIndex,
                     notifyItem->hWnd, notifyItem->uCallbackMessage, notifyItem->uID, uMsg);
        }

        DWORD pid;
        GetWindowThreadProcessId(notifyItem->hWnd, &pid);

        if (pid == GetCurrentProcessId() ||
            (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST))
        {
            PostMessage(notifyItem->hWnd,
                        notifyItem->uCallbackMessage,
                        notifyItem->uID,
                        uMsg);
        }
        else
        {
            SendMessage(notifyItem->hWnd,
                        notifyItem->uCallbackMessage,
                        notifyItem->uID,
                        uMsg);
        }
    }

    LRESULT OnMouseEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        
        INT iBtn = HitTest(&pt);

        if (iBtn >= 0)
        {
            SendMouseEvent(iBtn, uMsg, wParam);
        }

        bHandled = FALSE;
        return FALSE;
    }

    LRESULT OnTooltipShow(INT uCode, LPNMHDR hdr, BOOL& bHandled)
    {
        RECT rcTip, rcItem;
        GetWindowRect(hdr->hwndFrom, &rcTip);

        SIZE szTip = { rcTip.right - rcTip.left, rcTip.bottom - rcTip.top };

        INT iBtn = GetHotItem();

        if (iBtn >= 0)
        {
            MONITORINFO monInfo = { 0 };
            HMONITOR hMon = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);

            monInfo.cbSize = sizeof(monInfo);

            if (hMon)
                GetMonitorInfo(hMon, &monInfo);
            else
                GetWindowRect(GetDesktopWindow(), &monInfo.rcMonitor);

            GetItemRect(iBtn, &rcItem);

            POINT ptItem = { rcItem.left, rcItem.top };
            SIZE szItem = { rcItem.right - rcItem.left, rcItem.bottom - rcItem.top };
            ClientToScreen(m_hWnd, &ptItem);

            ptItem.x += szItem.cx / 2;
            ptItem.y -= szTip.cy;

            if (ptItem.x + szTip.cx > monInfo.rcMonitor.right)
                ptItem.x = monInfo.rcMonitor.right - szTip.cx;

            if (ptItem.y + szTip.cy > monInfo.rcMonitor.bottom)
                ptItem.y = monInfo.rcMonitor.bottom - szTip.cy;

            if (ptItem.x < monInfo.rcMonitor.left)
                ptItem.x = monInfo.rcMonitor.left;

            if (ptItem.y < monInfo.rcMonitor.top)
                ptItem.y = monInfo.rcMonitor.top;

            TRACE("ptItem { %d, %d }\n", ptItem.x, ptItem.y);

            ::SetWindowPos(hdr->hwndFrom, NULL, ptItem.x, ptItem.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

            return TRUE;
        }

        bHandled = FALSE;
        return 0;
    }


public:
    BEGIN_MSG_MAP(CNotifyToolbar)
        MESSAGE_RANGE_HANDLER(WM_MOUSEFIRST, WM_MOUSELAST, OnMouseEvent)
        NOTIFY_CODE_HANDLER(TTN_SHOW, OnTooltipShow)
    END_MSG_MAP()

    void Initialize(HWND hWndParent)
    {
        DWORD styles =
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN |
            TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_WRAPABLE | TBSTYLE_TRANSPARENT |
            CCS_TOP | CCS_NORESIZE | CCS_NOPARENTALIGN | CCS_NODIVIDER;

        SubclassWindow(Create(hWndParent, styles));

        SetWindowTheme(m_hWnd, L"TrayNotify", NULL);

        SysIcons = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 1000);
        SetImageList(SysIcons);

        SetButtonSize(ICON_SIZE, ICON_SIZE);
    }
};

class CSysPagerWnd :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CWindowImpl < CSysPagerWnd, CWindow, CControlWinTraits >
{
    CNotifyToolbar Toolbar;

public:
    CSysPagerWnd() {}
    virtual ~CSysPagerWnd() {}

    LRESULT DrawBackground(HDC hdc)
    {
        RECT rect;

        GetClientRect(&rect);
        DrawThemeParentBackground(m_hWnd, hdc, &rect);

        return TRUE;
    }

    LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        HDC hdc = (HDC) wParam;

        if (!IsAppThemed())
        {
            bHandled = FALSE;
            return 0;
        }

        return DrawBackground(hdc);
    }

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        Toolbar.Initialize(m_hWnd);
        return TRUE;
    }

    LRESULT NotifyMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        PCOPYDATASTRUCT cpData = (PCOPYDATASTRUCT) lParam;
        if (cpData->dwData == 1)
        {
            SYS_PAGER_COPY_DATA * data;
            NOTIFYICONDATA *iconData;
            HWND parentHWND;
            RECT windowRect;
            parentHWND = GetParent();
            parentHWND = ::GetParent(parentHWND);
            ::GetClientRect(parentHWND, &windowRect);

            data = (PSYS_PAGER_COPY_DATA) cpData->lpData;
            iconData = &data->nicon_data;

            switch (data->notify_code)
            {
            case NIM_ADD:
            {
                Toolbar.AddButton(iconData);
                break;
            }
            case NIM_MODIFY:
            {
                Toolbar.UpdateButton(iconData);
                break;
            }
            case NIM_DELETE:
            {
                Toolbar.RemoveButton(iconData);
                break;
            }
            default:
                TRACE("NotifyMessage received with unknown code %d.\n", data->notify_code);
                break;
            }
            SendMessage(parentHWND,
                WM_SIZE,
                0,
                MAKELONG(windowRect.right - windowRect.left,
                windowRect.bottom - windowRect.top));
        }

        return TRUE;
    }

    void GetSize(IN WPARAM wParam, IN PSIZE size)
    {
        INT rows = 0;
        int VisibleButtonCount = Toolbar.GetVisibleButtonCount();

        if (wParam) /* horizontal */
        {
            rows = size->cy / 24;
            if (rows == 0)
                rows++;
            size->cx = (VisibleButtonCount + rows - 1) / rows * 24;
        }
        else
        {
            rows = size->cx / 24;
            if (rows == 0)
                rows++;
            size->cy = (VisibleButtonCount + rows - 1) / rows * 24;
        }
    }

    LRESULT OnGetInfoTip(INT uCode, LPNMHDR hdr, BOOL& bHandled)
    {
        NMTBGETINFOTIPW * nmtip = (NMTBGETINFOTIPW *) hdr;
        Toolbar.GetTooltipText(nmtip->iItem, nmtip->pszText, nmtip->cchTextMax);
        return TRUE;
    }

    LRESULT OnCustomDraw(INT uCode, LPNMHDR hdr, BOOL& bHandled)
    {
        NMCUSTOMDRAW * cdraw = (NMCUSTOMDRAW *) hdr;
        switch (cdraw->dwDrawStage)
        {
        case CDDS_PREPAINT:
            return CDRF_NOTIFYITEMDRAW;

        case CDDS_ITEMPREPAINT:
            return TBCDRF_NOBACKGROUND | TBCDRF_NOEDGES | TBCDRF_NOOFFSET | TBCDRF_NOMARK | TBCDRF_NOETCHEDEFFECT;
        }
        return TRUE;
    }

    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LRESULT Ret = TRUE;
        SIZE szClient;
        szClient.cx = LOWORD(lParam);
        szClient.cy = HIWORD(lParam);

        Ret = DefWindowProc(uMsg, wParam, lParam);

        if (Toolbar)
        {
            TBMETRICS tbm;
            tbm.cbSize = sizeof(tbm);
            tbm.dwMask = TBMF_BARPAD | TBMF_BUTTONSPACING;
            tbm.cxBarPad = tbm.cyBarPad = 0;
            tbm.cxButtonSpacing = 0;
            tbm.cyButtonSpacing = 0;

            Toolbar.SetMetrics(&tbm);

            Toolbar.SetWindowPos(NULL, 0, 0, szClient.cx, szClient.cy, SWP_NOZORDER);
            Toolbar.AutoSize();

            RECT rc;
            Toolbar.GetClientRect(&rc);

            SIZE szBar = { rc.right - rc.left, rc.bottom - rc.top };

            INT xOff = (szClient.cx - szBar.cx) / 2;
            INT yOff = (szClient.cy - szBar.cy) / 2;

            Toolbar.SetWindowPos(NULL, xOff, yOff, szBar.cx, szBar.cy, SWP_NOZORDER);
        }
        return Ret;
    }

    DECLARE_WND_CLASS_EX(szSysPagerWndClass, CS_DBLCLKS, COLOR_3DFACE)

    BEGIN_MSG_MAP(CTaskSwitchWnd)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        NOTIFY_CODE_HANDLER(TBN_GETINFOTIPW, OnGetInfoTip)
        NOTIFY_CODE_HANDLER(NM_CUSTOMDRAW, OnCustomDraw)
    END_MSG_MAP()

    HWND _Init(IN HWND hWndParent, IN BOOL bVisible)
    {
        DWORD dwStyle;

        /* Create the window. The tray window is going to move it to the correct
            position and resize it as needed. */
        dwStyle = WS_CHILD | WS_CLIPSIBLINGS;
        if (bVisible)
            dwStyle |= WS_VISIBLE;

        Create(hWndParent, 0, NULL, dwStyle);

        if (!m_hWnd)
        {
            return NULL;
        }

        SetWindowTheme(m_hWnd, L"TrayNotify", NULL);

        return m_hWnd;
    }
};

/*
 * TrayClockWnd
 */

static const WCHAR szTrayClockWndClass [] = TEXT("TrayClockWClass");

#define ID_TRAYCLOCK_TIMER  0
#define ID_TRAYCLOCK_TIMER_INIT 1

static const struct
{
    BOOL IsTime;
    DWORD dwFormatFlags;
    LPCTSTR lpFormat;
} ClockWndFormats [] = {
        { TRUE, 0, NULL },
        { FALSE, 0, TEXT("dddd") },
        { FALSE, DATE_SHORTDATE, NULL }
};

#define CLOCKWND_FORMAT_COUNT (sizeof(ClockWndFormats) / sizeof(ClockWndFormats[0]))

#define TRAY_CLOCK_WND_SPACING_X    0
#define TRAY_CLOCK_WND_SPACING_Y    0

class CTrayClockWnd :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CWindowImpl < CTrayClockWnd, CWindow, CControlWinTraits >
{
    HWND hWndNotify;
    HFONT hFont;
    COLORREF textColor;
    RECT rcText;
    SYSTEMTIME LocalTime;

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
    CTrayClockWnd() :
        hWndNotify(NULL),
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
        ZeroMemory(szLines, sizeof(LineSizes));
    }
    virtual ~CTrayClockWnd() { }

    LRESULT OnThemeChanged()
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
        }
        else
        {
            NONCLIENTMETRICS ncm = { 0 };
            ncm.cbSize = sizeof(ncm);
            SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, FALSE);

            hFont = CreateFontIndirectW(&ncm.lfMessageFont);

            textColor = RGB(0, 0, 0);
        }

        SetFont(hFont, FALSE);

        CloseThemeData(clockTheme);

        return TRUE;
    }

    LRESULT OnThemeChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        return OnThemeChanged();
    }

    BOOL MeasureLines()
    {
        HDC hDC;
        HFONT hPrevFont;
        INT c, i;
        BOOL bRet = TRUE;

        hDC = GetDC(m_hWnd);
        if (hDC != NULL)
        {
            if (hFont)
                hPrevFont = (HFONT) SelectObject(hDC, hFont);

            for (i = 0; i != CLOCKWND_FORMAT_COUNT && bRet; i++)
            {
                if (szLines[i][0] != TEXT('\0') &&
                    !GetTextExtentPoint(hDC, szLines[i], _tcslen(szLines[i]),
                    &LineSizes[i]))
                {
                    bRet = FALSE;
                    break;
                }
            }

            if (hFont)
                SelectObject(hDC, hPrevFont);

            ReleaseDC(m_hWnd, hDC);

            if (bRet)
            {
                LineSpacing = 0;

                /* calculate the line spacing */
                for (i = 0, c = 0; i != CLOCKWND_FORMAT_COUNT; i++)
                {
                    if (LineSizes[i].cx > 0)
                    {
                        LineSpacing += LineSizes[i].cy;
                        c++;
                    }
                }

                if (c > 0)
                {
                    /* We want a spaceing of 1/2 line */
                    LineSpacing = (LineSpacing / c) / 2;
                }

                return TRUE;
            }
        }

        return FALSE;
    }

    WORD GetMinimumSize(IN BOOL Horizontal, IN OUT PSIZE pSize)
    {
        WORD iLinesVisible = 0;
        INT i;
        SIZE szMax = { 0, 0 };

        if (!LinesMeasured)
            LinesMeasured = MeasureLines();

        if (!LinesMeasured)
            return 0;

        for (i = 0;
            i != CLOCKWND_FORMAT_COUNT;
            i++)
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
                if (LineSizes[i].cx > szMax.cx - (2 * TRAY_CLOCK_WND_SPACING_X))
                    szMax.cx = LineSizes[i].cx + (2 * TRAY_CLOCK_WND_SPACING_X);
            }
        }

        szMax.cx += 2 * TRAY_CLOCK_WND_SPACING_X;
        szMax.cy += 2 * TRAY_CLOCK_WND_SPACING_Y;

        *pSize = szMax;

        return iLinesVisible;
    }


    VOID        UpdateWnd()
    {
        SIZE szPrevCurrent;
        INT BufSize, iRet, i;
        RECT rcClient;

        ZeroMemory(LineSizes,
            sizeof(LineSizes));

        szPrevCurrent = CurrentSize;

        for (i = 0;
            i != CLOCKWND_FORMAT_COUNT;
            i++)
        {
            szLines[i][0] = TEXT('\0');
            BufSize = sizeof(szLines[0]) / sizeof(szLines[0][0]);

            if (ClockWndFormats[i].IsTime)
            {
                iRet = GetTimeFormat(LOCALE_USER_DEFAULT,
                    AdvancedSettings.bShowSeconds ? ClockWndFormats[i].dwFormatFlags : TIME_NOSECONDS,
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

        if (IsWindowVisible(m_hWnd))
        {
            InvalidateRect(NULL, TRUE);

            if (hWndNotify != NULL &&
                (szPrevCurrent.cx != CurrentSize.cx ||
                szPrevCurrent.cy != CurrentSize.cy))
            {
                NMHDR nmh;

                nmh.hwndFrom = m_hWnd;
                nmh.idFrom = GetWindowLongPtr(m_hWnd, GWLP_ID);
                nmh.code = NTNWM_REALIGN;

                SendMessage(hWndNotify,
                    WM_NOTIFY,
                    (WPARAM) nmh.idFrom,
                    (LPARAM) &nmh);
            }
        }
    }

    VOID        Update()
    {
        GetLocalTime(&LocalTime);
        UpdateWnd();
    }

    UINT        CalculateDueTime()
    {
        UINT uiDueTime;

        /* Calculate the due time */
        GetLocalTime(&LocalTime);
        uiDueTime = 1000 - (UINT) LocalTime.wMilliseconds;
        if (AdvancedSettings.bShowSeconds)
            uiDueTime += (UINT) LocalTime.wSecond * 100;
        else
            uiDueTime += (59 - (UINT) LocalTime.wSecond) * 1000;

        if (uiDueTime < USER_TIMER_MINIMUM || uiDueTime > USER_TIMER_MAXIMUM)
            uiDueTime = 1000;
        else
        {
            /* Add an artificial delay of 0.05 seconds to make sure the timer
               doesn't fire too early*/
            uiDueTime += 50;
        }

        return uiDueTime;
    }

    BOOL        ResetTime()
    {
        UINT uiDueTime;
        BOOL Ret;

        /* Disable all timers */
        if (IsTimerEnabled)
        {
            KillTimer(ID_TRAYCLOCK_TIMER);
            IsTimerEnabled = FALSE;
        }

        if (IsInitTimerEnabled)
        {
            KillTimer(ID_TRAYCLOCK_TIMER_INIT);
        }

        uiDueTime = CalculateDueTime();

        /* Set the new timer */
        Ret = SetTimer(ID_TRAYCLOCK_TIMER_INIT, uiDueTime, NULL) != 0;
        IsInitTimerEnabled = Ret;

        /* Update the time */
        Update();

        return Ret;
    }

    VOID        CalibrateTimer()
    {
        UINT uiDueTime;
        BOOL Ret;
        UINT uiWait1, uiWait2;

        /* Kill the initialization timer */
        KillTimer(ID_TRAYCLOCK_TIMER_INIT);
        IsInitTimerEnabled = FALSE;

        uiDueTime = CalculateDueTime();

        if (AdvancedSettings.bShowSeconds)
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

            /* Update the time */
            Update();
        }
        else
        {
            /* Recalibrate the timer and recalculate again when the current
               minute/second ends. */
            ResetTime();
        }
    }

    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        /* Disable all timers */
        if (IsTimerEnabled)
        {
            KillTimer(ID_TRAYCLOCK_TIMER);
        }

        if (IsInitTimerEnabled)
        {
            KillTimer(ID_TRAYCLOCK_TIMER_INIT);
        }

        return TRUE;
    }

    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        RECT rcClient;
        HFONT hPrevFont;
        int iPrevBkMode, i, line;

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

            rcClient.left = (rcClient.right / 2) - (CurrentSize.cx / 2);
            rcClient.top = (rcClient.bottom / 2) - (CurrentSize.cy / 2);
            rcClient.right = rcClient.left + CurrentSize.cx;
            rcClient.bottom = rcClient.top + CurrentSize.cy;

            for (i = 0, line = 0;
                i != CLOCKWND_FORMAT_COUNT && line < VisibleLines;
                i++)
            {
                if (LineSizes[i].cx != 0)
                {
                    TextOut(hDC,
                        rcClient.left + (CurrentSize.cx / 2) - (LineSizes[i].cx / 2) +
                        TRAY_CLOCK_WND_SPACING_X,
                        rcClient.top + TRAY_CLOCK_WND_SPACING_Y,
                        szLines[i],
                        _tcslen(szLines[i]));

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

    VOID SetFont(IN HFONT hNewFont, IN BOOL bRedraw)
    {
        hFont = hNewFont;
        LinesMeasured = MeasureLines();
        if (bRedraw)
        {
            InvalidateRect(NULL, TRUE);
        }
    }

    LRESULT DrawBackground(HDC hdc)
    {
        RECT rect;

        GetClientRect(&rect);
        DrawThemeParentBackground(m_hWnd, hdc, &rect);

        return TRUE;
    }

    LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        HDC hdc = (HDC) wParam;

        if (!IsAppThemed())
        {
            bHandled = FALSE;
            return 0;
        }

        return DrawBackground(hdc);
    }

    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
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

    LRESULT OnGetMinimumSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        IsHorizontal = (BOOL) wParam;

        return (LRESULT) GetMinimumSize((BOOL) wParam, (PSIZE) lParam) != 0;
    }

    LRESULT OnUpdateTime(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        return (LRESULT) ResetTime();
    }

    LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        return HTTRANSPARENT;
    }

    LRESULT OnSetFont(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        SetFont((HFONT) wParam, (BOOL) LOWORD(lParam));
        return TRUE;
    }

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        ResetTime();
        return TRUE;
    }

    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        SIZE szClient;

        szClient.cx = LOWORD(lParam);
        szClient.cy = HIWORD(lParam);

        VisibleLines = GetMinimumSize(IsHorizontal, &szClient);
        CurrentSize = szClient;

        InvalidateRect(NULL, TRUE);
        return TRUE;
    }

    DECLARE_WND_CLASS_EX(szTrayClockWndClass, CS_DBLCLKS, COLOR_3DFACE)

    BEGIN_MSG_MAP(CTaskSwitchWnd)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_PRINTCLIENT, OnPaint)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTest)
        MESSAGE_HANDLER(TCWM_GETMINIMUMSIZE, OnGetMinimumSize)
        MESSAGE_HANDLER(TCWM_UPDATETIME, OnUpdateTime)

    END_MSG_MAP()

    HWND _Init(IN HWND hWndParent, IN BOOL bVisible)
    {
        IsHorizontal = TRUE;

        hWndNotify = hWndParent;

        /* Create the window. The tray window is going to move it to the correct
            position and resize it as needed. */
        DWORD dwStyle = WS_CHILD | WS_CLIPSIBLINGS;
        if (bVisible)
            dwStyle |= WS_VISIBLE;

        Create(hWndParent, 0, NULL, dwStyle);

        if (m_hWnd != NULL)
        {
            SetWindowTheme(m_hWnd, L"TrayNotify", NULL);
            OnThemeChanged();
        }

        return m_hWnd;

    }
};

/*
 * TrayNotifyWnd
 */

static const WCHAR szTrayNotifyWndClass [] = TEXT("TrayNotifyWnd");

#define TRAY_NOTIFY_WND_SPACING_X   2
#define TRAY_NOTIFY_WND_SPACING_Y   2

class CTrayNotifyWnd :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CWindowImpl < CTrayNotifyWnd, CWindow, CControlWinTraits >
{
    HWND hWndNotify;

    CSysPagerWnd * m_pager;
    CTrayClockWnd * m_clock;

    CComPtr<ITrayWindow> TrayWindow;

    HTHEME TrayTheme;
    SIZE szTrayClockMin;
    SIZE szTrayNotify;
    MARGINS ContentMargin;
    HFONT hFontClock;
    union
    {
        DWORD dwFlags;
        struct
        {
            DWORD HideClock : 1;
            DWORD IsHorizontal : 1;
        };
    };

public:
    CTrayNotifyWnd() :
        hWndNotify(NULL),
        m_pager(NULL),
        m_clock(NULL),
        TrayTheme(NULL),
        hFontClock(NULL),
        dwFlags(0)
    {
        ZeroMemory(&szTrayClockMin, sizeof(szTrayClockMin));
        ZeroMemory(&szTrayNotify, sizeof(szTrayNotify));
        ZeroMemory(&ContentMargin, sizeof(ContentMargin));
    }
    virtual ~CTrayNotifyWnd() { }

    LRESULT OnThemeChanged()
    {
        if (TrayTheme)
            CloseThemeData(TrayTheme);

        if (IsThemeActive())
            TrayTheme = OpenThemeData(m_hWnd, L"TrayNotify");
        else
            TrayTheme = NULL;

        if (TrayTheme)
        {
            SetWindowExStyle(m_hWnd, WS_EX_STATICEDGE, 0);

            GetThemeMargins(TrayTheme,
                NULL,
                TNP_BACKGROUND,
                0,
                TMT_CONTENTMARGINS,
                NULL,
                &ContentMargin);
        }
        else
        {
            SetWindowExStyle(m_hWnd, WS_EX_STATICEDGE, WS_EX_STATICEDGE);

            ContentMargin.cxLeftWidth = 0;
            ContentMargin.cxRightWidth = 0;
            ContentMargin.cyTopHeight = 0;
            ContentMargin.cyBottomHeight = 0;
        }

        return TRUE;
    }

    LRESULT OnThemeChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        return OnThemeChanged();
    }

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        m_clock = new CTrayClockWnd();
        m_clock->_Init(m_hWnd, !HideClock);

        m_pager = new CSysPagerWnd();
        m_pager->_Init(m_hWnd, !HideClock);

        OnThemeChanged();

        return TRUE;
    }

    BOOL GetMinimumSize(IN BOOL Horizontal, IN OUT PSIZE pSize)
    {
        SIZE szClock = { 0, 0 };
        SIZE szTray = { 0, 0 };

        IsHorizontal = Horizontal;
        if (IsHorizontal)
            SetWindowTheme(m_hWnd, L"TrayNotifyHoriz", NULL);
        else
            SetWindowTheme(m_hWnd, L"TrayNotifyVert", NULL);

        if (!HideClock)
        {
            if (Horizontal)
            {
                szClock.cy = pSize->cy - 2 * TRAY_NOTIFY_WND_SPACING_Y;
                if (szClock.cy <= 0)
                    goto NoClock;
            }
            else
            {
                szClock.cx = pSize->cx - 2 * TRAY_NOTIFY_WND_SPACING_X;
                if (szClock.cx <= 0)
                    goto NoClock;
            }

            m_clock->SendMessage(TCWM_GETMINIMUMSIZE, (WPARAM) Horizontal, (LPARAM) &szClock);

            szTrayClockMin = szClock;
        }
        else
        NoClock:
        szTrayClockMin = szClock;

        if (Horizontal)
        {
            szTray.cy = pSize->cy - 2 * TRAY_NOTIFY_WND_SPACING_Y;
        }
        else
        {
            szTray.cx = pSize->cx - 2 * TRAY_NOTIFY_WND_SPACING_X;
        }

        m_pager->GetSize(Horizontal, &szTray);

        szTrayNotify = szTray;

        if (Horizontal)
        {
            pSize->cx = 2 * TRAY_NOTIFY_WND_SPACING_X;

            if (!HideClock)
                pSize->cx += TRAY_NOTIFY_WND_SPACING_X + szTrayClockMin.cx;

            pSize->cx += szTray.cx;
        }
        else
        {
            pSize->cy = 2 * TRAY_NOTIFY_WND_SPACING_Y;

            if (!HideClock)
                pSize->cy += TRAY_NOTIFY_WND_SPACING_Y + szTrayClockMin.cy;

            pSize->cy += szTray.cy;
        }

        pSize->cy += ContentMargin.cyTopHeight + ContentMargin.cyBottomHeight;
        pSize->cx += ContentMargin.cxLeftWidth + ContentMargin.cxRightWidth;

        return TRUE;
    }

    VOID Size(IN const SIZE *pszClient)
    {
        if (!HideClock)
        {
            POINT ptClock;
            SIZE szClock;

            if (IsHorizontal)
            {
                ptClock.x = pszClient->cx - TRAY_NOTIFY_WND_SPACING_X - szTrayClockMin.cx;
                ptClock.y = TRAY_NOTIFY_WND_SPACING_Y;
                szClock.cx = szTrayClockMin.cx;
                szClock.cy = pszClient->cy - (2 * TRAY_NOTIFY_WND_SPACING_Y);
            }
            else
            {
                ptClock.x = TRAY_NOTIFY_WND_SPACING_X;
                ptClock.y = pszClient->cy - TRAY_NOTIFY_WND_SPACING_Y - szTrayClockMin.cy;
                szClock.cx = pszClient->cx - (2 * TRAY_NOTIFY_WND_SPACING_X);
                szClock.cy = szTrayClockMin.cy;
            }

            m_clock->SetWindowPos(
                NULL,
                ptClock.x,
                ptClock.y,
                szClock.cx,
                szClock.cy,
                SWP_NOZORDER);

            if (IsHorizontal)
            {
                ptClock.x -= szTrayNotify.cx;
            }
            else
            {
                ptClock.y -= szTrayNotify.cy;
            }

            m_pager->SetWindowPos(
                NULL,
                ptClock.x,
                ptClock.y,
                szTrayNotify.cx,
                szTrayNotify.cy,
                SWP_NOZORDER);
        }
    }

    LRESULT DrawBackground(HDC hdc)
    {
        RECT rect;

        GetClientRect(&rect);

        if (TrayTheme)
        {
            if (IsThemeBackgroundPartiallyTransparent(TrayTheme, TNP_BACKGROUND, 0))
            {
                DrawThemeParentBackground(m_hWnd, hdc, &rect);
            }

            DrawThemeBackground(TrayTheme, hdc, TNP_BACKGROUND, 0, &rect, 0);
        }

        return TRUE;
    }

    LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        HDC hdc = (HDC) wParam;

        if (!TrayTheme)
        {
            bHandled = FALSE;
            return 0;
        }

        return DrawBackground(hdc);
    }

    LRESULT NotifyMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (m_pager)
        {
            m_pager->NotifyMsg(uMsg, wParam, lParam, bHandled);
        }

        return TRUE;
    }

    BOOL GetClockRect(OUT PRECT rcClock)
    {
        if (!IsWindowVisible(m_clock->m_hWnd))
            return FALSE;

        return GetWindowRect(m_clock->m_hWnd, rcClock);
    }

    LRESULT OnGetMinimumSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        return (LRESULT) GetMinimumSize((BOOL) wParam, (PSIZE) lParam);
    }

    LRESULT OnUpdateTime(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (m_clock != NULL)
        {
            /* Forward the message to the tray clock window procedure */
            return m_clock->OnUpdateTime(uMsg, wParam, lParam, bHandled);
        }
        return FALSE;
    }

    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        SIZE szClient;

        szClient.cx = LOWORD(lParam);
        szClient.cy = HIWORD(lParam);

        Size(&szClient);

        return TRUE;
    }

    LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        return HTTRANSPARENT;
    }

    LRESULT OnShowClock(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        BOOL PrevHidden = HideClock;
        HideClock = (wParam == 0);

        if (m_clock != NULL && PrevHidden != HideClock)
        {
            m_clock->ShowWindow(HideClock ? SW_HIDE : SW_SHOW);
        }

        return (LRESULT) (!PrevHidden);
    }

    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        const NMHDR *nmh = (const NMHDR *) lParam;

        if (nmh->hwndFrom == m_clock->m_hWnd)
        {
            /* Pass down notifications */
            return m_clock->SendMessage(WM_NOTIFY, wParam, lParam);
        }

        return FALSE;
    }

    LRESULT OnSetFont(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (m_clock != NULL)
        {
            m_clock->SendMessageW(WM_SETFONT, wParam, lParam);
        }

        bHandled = FALSE;
        return FALSE;
    }

    DECLARE_WND_CLASS_EX(szTrayNotifyWndClass, CS_DBLCLKS, COLOR_3DFACE)

    BEGIN_MSG_MAP(CTaskSwitchWnd)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_THEMECHANGED, OnThemeChanged)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTest)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        MESSAGE_HANDLER(WM_SETFONT, OnSetFont)
        MESSAGE_HANDLER(TNWM_GETMINIMUMSIZE, OnGetMinimumSize)
        MESSAGE_HANDLER(TNWM_UPDATETIME, OnUpdateTime)
        MESSAGE_HANDLER(TNWM_SHOWCLOCK, OnShowClock)
    END_MSG_MAP()

    HWND _Init(IN OUT ITrayWindow *TrayWindow, IN BOOL bHideClock)
    {
        HWND hWndTrayWindow;

        hWndTrayWindow = TrayWindow->GetHWND();
        if (hWndTrayWindow == NULL)
            return NULL;

        this->TrayWindow = TrayWindow;
        this->HideClock = bHideClock;
        this->hWndNotify = hWndTrayWindow;

        DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
        return Create(hWndTrayWindow, 0, NULL, dwStyle, WS_EX_STATICEDGE);
    }
};

static CTrayNotifyWnd * g_Instance;

HWND CreateTrayNotifyWnd(IN OUT ITrayWindow *Tray, BOOL bHideClock)
{
    // TODO: Destroy after the window is destroyed
    g_Instance = new CTrayNotifyWnd();

    return g_Instance->_Init(Tray, bHideClock);
}

VOID
TrayNotify_NotifyMsg(WPARAM wParam, LPARAM lParam)
{
    BOOL bDummy;
    g_Instance->NotifyMsg(0, wParam, lParam, bDummy);
}

BOOL
TrayNotify_GetClockRect(OUT PRECT rcClock)
{
    return g_Instance->GetClockRect(rcClock);
}
