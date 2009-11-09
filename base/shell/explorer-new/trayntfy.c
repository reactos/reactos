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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <precomp.h>

/*
 * TrayClockWnd
 */

static const TCHAR szTrayClockWndClass[] = TEXT("TrayClockWClass");

#define ID_TRAYCLOCK_TIMER  0
#define ID_TRAYCLOCK_TIMER_INIT 1

static const struct
{
    BOOL IsTime;
    DWORD dwFormatFlags;
    LPCTSTR lpFormat;
} ClockWndFormats[] = {
    {TRUE, TIME_NOSECONDS, NULL},
    {FALSE, 0, TEXT("dddd")},
    {FALSE, DATE_SHORTDATE, NULL},
};

#define CLOCKWND_FORMAT_COUNT (sizeof(ClockWndFormats) / sizeof(ClockWndFormats[0]))

#define TRAY_CLOCK_WND_SPACING_X    0
#define TRAY_CLOCK_WND_SPACING_Y    0

typedef struct _TRAY_CLOCK_WND_DATA
{
    HWND hWnd;
    HWND hWndNotify;
    HFONT hFont;
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
    TCHAR szLines[CLOCKWND_FORMAT_COUNT][48];
} TRAY_CLOCK_WND_DATA, *PTRAY_CLOCK_WND_DATA;

static BOOL
TrayClockWnd_MeasureLines(IN OUT PTRAY_CLOCK_WND_DATA This)
{
    HDC hDC;
    HFONT hPrevFont;
    INT c, i;
    BOOL bRet = TRUE;

    hDC = GetDC(This->hWnd);
    if (hDC != NULL)
    {
        hPrevFont = SelectObject(hDC,
                                 This->hFont);

        for (i = 0;
             i != CLOCKWND_FORMAT_COUNT && bRet;
             i++)
        {
            if (This->szLines[i][0] != TEXT('\0') &&
                !GetTextExtentPoint(hDC,
                                    This->szLines[i],
                                    _tcslen(This->szLines[i]),
                                    &This->LineSizes[i]))
            {
                bRet = FALSE;
                break;
            }
        }

        SelectObject(hDC,
                     hPrevFont);

        ReleaseDC(This->hWnd,
                  hDC);

        if (bRet)
        {
            This->LineSpacing = 0;

            /* calculate the line spacing */
            for (i = 0, c = 0;
                 i != CLOCKWND_FORMAT_COUNT;
                 i++)
            {
                if (This->LineSizes[i].cx > 0)
                {
                    This->LineSpacing += This->LineSizes[i].cy;
                    c++;
                }
            }

            if (c > 0)
            {
                /* We want a spaceing of 1/2 line */
                This->LineSpacing = (This->LineSpacing / c) / 2;
            }

            return TRUE;
        }
    }

    return FALSE;
}

static WORD
TrayClockWnd_GetMinimumSize(IN OUT PTRAY_CLOCK_WND_DATA This,
                            IN BOOL Horizontal,
                            IN OUT PSIZE pSize)
{
    WORD iLinesVisible = 0;
    INT i;
    SIZE szMax = { 0, 0 };

    if (!This->LinesMeasured)
        This->LinesMeasured = TrayClockWnd_MeasureLines(This);

    if (!This->LinesMeasured)
        return 0;

    for (i = 0;
         i != CLOCKWND_FORMAT_COUNT;
         i++)
    {
        if (This->LineSizes[i].cx != 0)
        {
            if (iLinesVisible > 0)
            {
                if (Horizontal)
                {
                    if (szMax.cy + This->LineSizes[i].cy + (LONG)This->LineSpacing >
                        pSize->cy - (2 * TRAY_CLOCK_WND_SPACING_Y))
                    {
                        break;
                    }
                }
                else
                {
                    if (This->LineSizes[i].cx > pSize->cx - (2 * TRAY_CLOCK_WND_SPACING_X))
                        break;
                }

                /* Add line spacing */
                szMax.cy += This->LineSpacing;
            }

            iLinesVisible++;

            /* Increase maximum rectangle */
            szMax.cy += This->LineSizes[i].cy;
            if (This->LineSizes[i].cx > szMax.cx - (2 * TRAY_CLOCK_WND_SPACING_X))
                szMax.cx = This->LineSizes[i].cx + (2 * TRAY_CLOCK_WND_SPACING_X);
        }
    }

    szMax.cx += 2 * TRAY_CLOCK_WND_SPACING_X;
    szMax.cy += 2 * TRAY_CLOCK_WND_SPACING_Y;

    *pSize = szMax;

    return iLinesVisible;
}


static VOID
TrayClockWnd_UpdateWnd(IN OUT PTRAY_CLOCK_WND_DATA This)
{
    SIZE szPrevCurrent;
    INT BufSize, iRet, i;
    RECT rcClient;

    ZeroMemory(This->LineSizes,
               sizeof(This->LineSizes));

    szPrevCurrent = This->CurrentSize;

    for (i = 0;
         i != CLOCKWND_FORMAT_COUNT;
         i++)
    {
        This->szLines[i][0] = TEXT('\0');
        BufSize = sizeof(This->szLines[0]) / sizeof(This->szLines[0][0]);

        if (ClockWndFormats[i].IsTime)
        {
            iRet = GetTimeFormat(LOCALE_USER_DEFAULT,
                                 ClockWndFormats[i].dwFormatFlags,
                                 &This->LocalTime,
                                 ClockWndFormats[i].lpFormat,
                                 This->szLines[i],
                                 BufSize);
        }
        else
        {
            iRet = GetDateFormat(LOCALE_USER_DEFAULT,
                                 ClockWndFormats[i].dwFormatFlags,
                                 &This->LocalTime,
                                 ClockWndFormats[i].lpFormat,
                                 This->szLines[i],
                                 BufSize);
        }

        if (iRet != 0 && i == 0)
        {
            /* Set the window text to the time only */
            SetWindowText(This->hWnd,
                          This->szLines[i]);
        }
    }

    This->LinesMeasured = TrayClockWnd_MeasureLines(This);

    if (This->LinesMeasured &&
        GetClientRect(This->hWnd,
                      &rcClient))
    {
        SIZE szWnd;

        szWnd.cx = rcClient.right;
        szWnd.cy = rcClient.bottom;

        This->VisibleLines = TrayClockWnd_GetMinimumSize(This,
                                                         This->IsHorizontal,
                                                         &szWnd);
        This->CurrentSize = szWnd;
    }

    if (IsWindowVisible(This->hWnd))
    {
        InvalidateRect(This->hWnd,
                       NULL,
                       TRUE);

        if (This->hWndNotify != NULL &&
            (szPrevCurrent.cx != This->CurrentSize.cx ||
             szPrevCurrent.cy != This->CurrentSize.cy))
        {
            NMHDR nmh;

            nmh.hwndFrom = This->hWnd;
            nmh.idFrom = GetWindowLongPtr(This->hWnd,
                                          GWL_ID);
            nmh.code = NTNWM_REALIGN;

            SendMessage(This->hWndNotify,
                        WM_NOTIFY,
                        (WPARAM)nmh.idFrom,
                        (LPARAM)&nmh);
        }
    }
}

static VOID
TrayClockWnd_Update(IN OUT PTRAY_CLOCK_WND_DATA This)
{
    GetLocalTime(&This->LocalTime);
    TrayClockWnd_UpdateWnd(This);
}

static UINT
TrayClockWnd_CalculateDueTime(IN OUT PTRAY_CLOCK_WND_DATA This)
{
    UINT uiDueTime;

    /* Calculate the due time */
    GetLocalTime(&This->LocalTime);
    uiDueTime = 1000 - (UINT)This->LocalTime.wMilliseconds;
    uiDueTime += (59 - (UINT)This->LocalTime.wSecond) * 1000;

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

static BOOL
TrayClockWnd_ResetTime(IN OUT PTRAY_CLOCK_WND_DATA This)
{
    UINT uiDueTime;
    BOOL Ret;

    /* Disable all timers */
    if (This->IsTimerEnabled)
    {
        KillTimer(This->hWnd,
                  ID_TRAYCLOCK_TIMER);
        This->IsTimerEnabled = FALSE;
    }

    if (This->IsInitTimerEnabled)
    {
        KillTimer(This->hWnd,
                  ID_TRAYCLOCK_TIMER_INIT);
    }

    uiDueTime = TrayClockWnd_CalculateDueTime(This);

    /* Set the new timer */
    Ret = SetTimer(This->hWnd,
                   ID_TRAYCLOCK_TIMER_INIT,
                   uiDueTime,
                   NULL) != 0;
    This->IsInitTimerEnabled = Ret;

    /* Update the time */
    TrayClockWnd_Update(This);

    return Ret;
}

static VOID
TrayClockWnd_CalibrateTimer(IN OUT PTRAY_CLOCK_WND_DATA This)
{
    UINT uiDueTime;
    BOOL Ret;

    /* Kill the initialization timer */
    KillTimer(This->hWnd,
              ID_TRAYCLOCK_TIMER_INIT);
    This->IsInitTimerEnabled = FALSE;

    uiDueTime = TrayClockWnd_CalculateDueTime(This);

    if (uiDueTime > (60 * 1000) - 200)
    {
        /* The update of the clock will be up to 200 ms late, but that's
           acceptable. We're going to setup a timer that fires every
           minute. */
        Ret = SetTimer(This->hWnd,
                       ID_TRAYCLOCK_TIMER,
                       60 * 1000,
                       NULL) != 0;
        This->IsTimerEnabled = Ret;

        /* Update the time */
        TrayClockWnd_Update(This);
    }
    else
    {
        /* Recalibrate the timer and recalculate again when the current
           minute ends. */
        TrayClockWnd_ResetTime(This);
    }
}

static VOID
TrayClockWnd_NCDestroy(IN OUT PTRAY_CLOCK_WND_DATA This)
{
    /* Disable all timers */
    if (This->IsTimerEnabled)
    {
        KillTimer(This->hWnd,
                  ID_TRAYCLOCK_TIMER);
    }

    if (This->IsInitTimerEnabled)
    {
        KillTimer(This->hWnd,
                  ID_TRAYCLOCK_TIMER_INIT);
    }

    /* Free allocated resources */
    SetWindowLongPtr(This->hWnd,
                     0,
                     0);
    HeapFree(hProcessHeap,
             0,
             This);
}

static VOID
TrayClockWnd_Paint(IN OUT PTRAY_CLOCK_WND_DATA This,
                   IN HDC hDC)
{
    RECT rcClient;
    HFONT hPrevFont;
    int iPrevBkMode, i, line;

    if (This->LinesMeasured &&
        GetClientRect(This->hWnd,
                      &rcClient))
    {
        iPrevBkMode = SetBkMode(hDC,
                                TRANSPARENT);

        hPrevFont = SelectObject(hDC,
                                 This->hFont);

        rcClient.left = (rcClient.right / 2) - (This->CurrentSize.cx / 2);
        rcClient.top = (rcClient.bottom / 2) - (This->CurrentSize.cy / 2);
        rcClient.right = rcClient.left + This->CurrentSize.cx;
        rcClient.bottom = rcClient.top + This->CurrentSize.cy;

        for (i = 0, line = 0;
             i != CLOCKWND_FORMAT_COUNT && line < This->VisibleLines;
             i++)
        {
            if (This->LineSizes[i].cx != 0)
            {
                TextOut(hDC,
                        rcClient.left + (This->CurrentSize.cx / 2) - (This->LineSizes[i].cx / 2) +
                            TRAY_CLOCK_WND_SPACING_X,
                        rcClient.top + TRAY_CLOCK_WND_SPACING_Y,
                        This->szLines[i],
                        _tcslen(This->szLines[i]));

                rcClient.top += This->LineSizes[i].cy + This->LineSpacing;
                line++;
            }
        }

        SelectObject(hDC,
                     hPrevFont);

        SetBkMode(hDC,
                  iPrevBkMode);
    }
}

static VOID
TrayClockWnd_SetFont(IN OUT PTRAY_CLOCK_WND_DATA This,
                     IN HFONT hNewFont,
                     IN BOOL bRedraw)
{
    This->hFont = hNewFont;
    This->LinesMeasured = TrayClockWnd_MeasureLines(This);
    if (bRedraw)
    {
        InvalidateRect(This->hWnd,
                       NULL,
                       TRUE);
    }
}

static LRESULT CALLBACK
TrayClockWndProc(IN HWND hwnd,
                 IN UINT uMsg,
                 IN WPARAM wParam,
                 IN LPARAM lParam)
{
    PTRAY_CLOCK_WND_DATA This = NULL;
    LRESULT Ret = FALSE;

    if (uMsg != WM_NCCREATE)
    {
        This = (PTRAY_CLOCK_WND_DATA)GetWindowLongPtr(hwnd,
                                                      0);
    }

    if (This != NULL || uMsg == WM_NCCREATE)
    {
        switch (uMsg)
        {
            case WM_PAINT:
            case WM_PRINTCLIENT:
            {
                PAINTSTRUCT ps;
                HDC hDC = (HDC)wParam;

                if (wParam == 0)
                {
                    hDC = BeginPaint(This->hWnd,
                                     &ps);
                }

                if (hDC != NULL)
                {
                    TrayClockWnd_Paint(This,
                                       hDC);

                    if (wParam == 0)
                    {
                        EndPaint(This->hWnd,
                                 &ps);
                    }
                }
                break;
            }

            case WM_TIMER:
                switch (wParam)
                {
                    case ID_TRAYCLOCK_TIMER:
                        TrayClockWnd_Update(This);
                        break;

                    case ID_TRAYCLOCK_TIMER_INIT:
                        TrayClockWnd_CalibrateTimer(This);
                        break;
                }
                break;

            case WM_NCHITTEST:
                /* We want the user to be able to drag the task bar when clicking the clock */
                Ret = HTTRANSPARENT;
                break;

            case TCWM_GETMINIMUMSIZE:
            {
                This->IsHorizontal = (BOOL)wParam;

                Ret = (LRESULT)TrayClockWnd_GetMinimumSize(This,
                                                           (BOOL)wParam,
                                                           (PSIZE)lParam) != 0;
                break;
            }

            case TCWM_UPDATETIME:
            {
                Ret = (LRESULT)TrayClockWnd_ResetTime(This);
                break;
            }

            case WM_NCCREATE:
            {
                LPCREATESTRUCT CreateStruct = (LPCREATESTRUCT)lParam;
                This = (PTRAY_CLOCK_WND_DATA)CreateStruct->lpCreateParams;
                This->hWnd = hwnd;
                This->hWndNotify = CreateStruct->hwndParent;

                SetWindowLongPtr(hwnd,
                                 0,
                                 (LONG_PTR)This);

                return TRUE;
            }

            case WM_SETFONT:
            {
                TrayClockWnd_SetFont(This,
                                     (HFONT)wParam,
                                     (BOOL)LOWORD(lParam));
                break;
            }

            case WM_CREATE:
                TrayClockWnd_ResetTime(This);
                break;

            case WM_NCDESTROY:
                TrayClockWnd_NCDestroy(This);
                break;

            case WM_SIZE:
            {
                SIZE szClient;

                szClient.cx = LOWORD(lParam);
                szClient.cy = HIWORD(lParam);
                This->VisibleLines = TrayClockWnd_GetMinimumSize(This,
                                                                 This->IsHorizontal,
                                                                 &szClient);
                This->CurrentSize = szClient;

                InvalidateRect(This->hWnd,
                               NULL,
                               TRUE);
                break;
            }

            default:
                Ret = DefWindowProc(hwnd,
                                    uMsg,
                                    wParam,
                                    lParam);
                break;
        }
    }

    return Ret;
}

static HWND
CreateTrayClockWnd(IN HWND hWndParent,
                   IN BOOL bVisible)
{
    PTRAY_CLOCK_WND_DATA TcData;
    DWORD dwStyle;
    HWND hWnd = NULL;

    TcData = HeapAlloc(hProcessHeap,
                       0,
                       sizeof(*TcData));
    if (TcData != NULL)
    {
        ZeroMemory(TcData,
                   sizeof(*TcData));

        TcData->IsHorizontal = TRUE;
        /* Create the window. The tray window is going to move it to the correct
           position and resize it as needed. */
        dwStyle = WS_CHILD | WS_CLIPSIBLINGS;
        if (bVisible)
            dwStyle |= WS_VISIBLE;

        hWnd = CreateWindowEx(0,
                              szTrayClockWndClass,
                              NULL,
                              dwStyle,
                              0,
                              0,
                              0,
                              0,
                              hWndParent,
                              NULL,
                              hExplorerInstance,
                              (LPVOID)TcData);

        if (hWnd == NULL)
        {
            HeapFree(hProcessHeap,
                     0,
                     TcData);
        }
    }

    return hWnd;

}

static BOOL
RegisterTrayClockWndClass(VOID)
{
    WNDCLASS wcTrayClock;

    wcTrayClock.style = CS_DBLCLKS;
    wcTrayClock.lpfnWndProc = TrayClockWndProc;
    wcTrayClock.cbClsExtra = 0;
    wcTrayClock.cbWndExtra = sizeof(PTRAY_CLOCK_WND_DATA);
    wcTrayClock.hInstance = hExplorerInstance;
    wcTrayClock.hIcon = NULL;
    wcTrayClock.hCursor = LoadCursor(NULL,
                                     IDC_ARROW);
    wcTrayClock.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wcTrayClock.lpszMenuName = NULL;
    wcTrayClock.lpszClassName = szTrayClockWndClass;

    return RegisterClass(&wcTrayClock) != 0;
}

static VOID
UnregisterTrayClockWndClass(VOID)
{
    UnregisterClass(szTrayClockWndClass,
                    hExplorerInstance);
}

/*
 * TrayNotifyWnd
 */

static const TCHAR szTrayNotifyWndClass[] = TEXT("TrayNotifyWnd");

#define TRAY_NOTIFY_WND_SPACING_X   2
#define TRAY_NOTIFY_WND_SPACING_Y   2

typedef struct _TRAY_NOTIFY_WND_DATA
{
    HWND hWnd;
    HWND hWndTrayClock;
    HWND hWndNotify;
    SIZE szTrayClockMin;
    SIZE szNonClient;
    ITrayWindow *TrayWindow;
    union
    {
        DWORD dwFlags;
        struct
        {
            DWORD HideClock : 1;
            DWORD IsHorizontal : 1;
        };
    };
} TRAY_NOTIFY_WND_DATA, *PTRAY_NOTIFY_WND_DATA;

static VOID
TrayNotifyWnd_UpdateStyle(IN OUT PTRAY_NOTIFY_WND_DATA This)
{
    RECT rcClient = { 0, 0, 0, 0 };

    if (AdjustWindowRectEx(&rcClient,
                           GetWindowLongPtr(This->hWnd,
                                            GWL_STYLE),
                           FALSE,
                           GetWindowLongPtr(This->hWnd,
                                            GWL_EXSTYLE)))
    {
        This->szNonClient.cx = rcClient.right - rcClient.left;
        This->szNonClient.cy = rcClient.bottom - rcClient.top;
    }
    else
        This->szNonClient.cx = This->szNonClient.cy = 0;
}

static VOID
TrayNotifyWnd_Create(IN OUT PTRAY_NOTIFY_WND_DATA This)
{
    This->hWndTrayClock = CreateTrayClockWnd(This->hWnd,
                                             !This->HideClock);

    TrayNotifyWnd_UpdateStyle(This);
}

static VOID
TrayNotifyWnd_NCDestroy(IN OUT PTRAY_NOTIFY_WND_DATA This)
{
    SetWindowLongPtr(This->hWnd,
                     0,
                     0);
    HeapFree(hProcessHeap,
             0,
             This);
}

static BOOL
TrayNotifyWnd_GetMinimumSize(IN OUT PTRAY_NOTIFY_WND_DATA This,
                             IN BOOL Horizontal,
                             IN OUT PSIZE pSize)
{
    This->IsHorizontal = Horizontal;

    if (!This->HideClock)
    {
        SIZE szClock = { 0, 0 };

        if (Horizontal)
        {
            szClock.cy = pSize->cy - This->szNonClient.cy - (2 * TRAY_NOTIFY_WND_SPACING_Y);
            if (szClock.cy <= 0)
                goto NoClock;
        }
        else
        {
            szClock.cx = pSize->cx - This->szNonClient.cx - (2 * TRAY_NOTIFY_WND_SPACING_X);
            if (szClock.cx <= 0)
                goto NoClock;
        }

        SendMessage(This->hWndTrayClock,
                    TCWM_GETMINIMUMSIZE,
                    (WPARAM)Horizontal,
                    (LPARAM)&szClock);

        This->szTrayClockMin = szClock;
    }
    else
NoClock:
        This->szTrayClockMin = This->szNonClient;

    if (Horizontal)
    {
        pSize->cx = This->szNonClient.cx + (2 * TRAY_NOTIFY_WND_SPACING_X);

        if (!This->HideClock)
            pSize->cx += TRAY_NOTIFY_WND_SPACING_X + This->szTrayClockMin.cx;
    }
    else
    {
        pSize->cy = This->szNonClient.cy + (2 * TRAY_NOTIFY_WND_SPACING_Y);

        if (!This->HideClock)
            pSize->cy += TRAY_NOTIFY_WND_SPACING_Y + This->szTrayClockMin.cy;
    }

    return TRUE;
}

static VOID
TrayNotifyWnd_Size(IN OUT PTRAY_NOTIFY_WND_DATA This,
                   IN const SIZE *pszClient)
{
    if (!This->HideClock)
    {
        POINT ptClock;
        SIZE szClock;

        if (This->IsHorizontal)
        {
            ptClock.x = pszClient->cx - TRAY_NOTIFY_WND_SPACING_X - This->szTrayClockMin.cx;
            ptClock.y = TRAY_NOTIFY_WND_SPACING_Y;
            szClock.cx = This->szTrayClockMin.cx;
            szClock.cy = pszClient->cy - (2 * TRAY_NOTIFY_WND_SPACING_Y);
        }
        else
        {
            ptClock.x = TRAY_NOTIFY_WND_SPACING_X;
            ptClock.y = pszClient->cy - TRAY_NOTIFY_WND_SPACING_Y - This->szTrayClockMin.cy;
            szClock.cx = pszClient->cx - (2 * TRAY_NOTIFY_WND_SPACING_X);
            szClock.cy = This->szTrayClockMin.cy;
        }

        SetWindowPos(This->hWndTrayClock,
                     NULL,
                     ptClock.x,
                     ptClock.y,
                     szClock.cx,
                     szClock.cy,
                     SWP_NOZORDER);
    }
}

static LRESULT CALLBACK
TrayNotifyWndProc(IN HWND hwnd,
                  IN UINT uMsg,
                  IN WPARAM wParam,
                  IN LPARAM lParam)
{
    PTRAY_NOTIFY_WND_DATA This = NULL;
    LRESULT Ret = FALSE;

    if (uMsg != WM_NCCREATE)
    {
        This = (PTRAY_NOTIFY_WND_DATA)GetWindowLongPtr(hwnd,
                                                       0);
    }

    if (This != NULL || uMsg == WM_NCCREATE)
    {
        switch (uMsg)
        {
            case TNWM_GETMINIMUMSIZE:
            {
                Ret = (LRESULT)TrayNotifyWnd_GetMinimumSize(This,
                                                            (BOOL)wParam,
                                                            (PSIZE)lParam);
                break;
            }

            case TNWM_UPDATETIME:
            {
                if (This->hWndTrayClock != NULL)
                {
                    /* Forward the message to the tray clock window procedure */
                    Ret = TrayClockWndProc(This->hWndTrayClock,
                                           TCWM_UPDATETIME,
                                           wParam,
                                           lParam);
                }
                break;
            }

            case WM_SIZE:
            {
                SIZE szClient;

                szClient.cx = LOWORD(lParam);
                szClient.cy = HIWORD(lParam);

                TrayNotifyWnd_Size(This,
                                   &szClient);
                break;
            }

            case WM_NCHITTEST:
                /* We want the user to be able to drag the task bar when clicking the
                   tray notification window */
                Ret = HTTRANSPARENT;
                break;

            case TNWM_SHOWCLOCK:
            {
                BOOL PrevHidden = This->HideClock;
                This->HideClock = (wParam == 0);

                if (This->hWndTrayClock != NULL && PrevHidden != This->HideClock)
                {
                    ShowWindow(This->hWndTrayClock,
                               This->HideClock ? SW_HIDE : SW_SHOW);
                }

                Ret = (LRESULT)(!PrevHidden);
                break;
            }

            case WM_NOTIFY:
            {
                const NMHDR *nmh = (const NMHDR *)lParam;

                if (nmh->hwndFrom == This->hWndTrayClock)
                {
                    /* Pass down notifications */
                    Ret = SendMessage(This->hWndNotify,
                                      WM_NOTIFY,
                                      wParam,
                                      lParam);
                }
                break;
            }

            case WM_SETFONT:
            {
                if (This->hWndTrayClock != NULL)
                {
                    SendMessage(This->hWndTrayClock,
                                WM_SETFONT,
                                wParam,
                                lParam);
                }
                goto HandleDefaultMessage;
            }

            case WM_NCCREATE:
            {
                LPCREATESTRUCT CreateStruct = (LPCREATESTRUCT)lParam;
                This = (PTRAY_NOTIFY_WND_DATA)CreateStruct->lpCreateParams;
                This->hWnd = hwnd;
                This->hWndNotify = CreateStruct->hwndParent;

                SetWindowLongPtr(hwnd,
                                 0,
                                 (LONG_PTR)This);

                return TRUE;
            }

            case WM_CREATE:
                TrayNotifyWnd_Create(This);
                break;

            case WM_NCDESTROY:
                TrayNotifyWnd_NCDestroy(This);
                break;

            default:
HandleDefaultMessage:
                Ret = DefWindowProc(hwnd,
                                    uMsg,
                                    wParam,
                                    lParam);
                break;
        }
    }

    return Ret;
}

HWND
CreateTrayNotifyWnd(IN OUT ITrayWindow *TrayWindow,
                    IN BOOL bHideClock)
{
    PTRAY_NOTIFY_WND_DATA TnData;
    HWND hWndTrayWindow;
    HWND hWnd = NULL;

    hWndTrayWindow = ITrayWindow_GetHWND(TrayWindow);
    if (hWndTrayWindow == NULL)
        return NULL;

    TnData = HeapAlloc(hProcessHeap,
                       0,
                       sizeof(*TnData));
    if (TnData != NULL)
    {
        ZeroMemory(TnData,
                   sizeof(*TnData));

        TnData->TrayWindow = TrayWindow;
        TnData->HideClock = bHideClock;

        /* Create the window. The tray window is going to move it to the correct
           position and resize it as needed. */
        hWnd = CreateWindowEx(WS_EX_STATICEDGE,
                              szTrayNotifyWndClass,
                              NULL,
                              WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                              0,
                              0,
                              0,
                              0,
                              hWndTrayWindow,
                              NULL,
                              hExplorerInstance,
                              (LPVOID)TnData);

        if (hWnd == NULL)
        {
            HeapFree(hProcessHeap,
                     0,
                     TnData);
        }
    }

    return hWnd;
}

BOOL
RegisterTrayNotifyWndClass(VOID)
{
    WNDCLASS wcTrayWnd;
    BOOL Ret;

    wcTrayWnd.style = CS_DBLCLKS;
    wcTrayWnd.lpfnWndProc = TrayNotifyWndProc;
    wcTrayWnd.cbClsExtra = 0;
    wcTrayWnd.cbWndExtra = sizeof(PTRAY_NOTIFY_WND_DATA);
    wcTrayWnd.hInstance = hExplorerInstance;
    wcTrayWnd.hIcon = NULL;
    wcTrayWnd.hCursor = LoadCursor(NULL,
                                   IDC_ARROW);
    wcTrayWnd.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wcTrayWnd.lpszMenuName = NULL;
    wcTrayWnd.lpszClassName = szTrayNotifyWndClass;

    Ret = RegisterClass(&wcTrayWnd) != 0;

    if (Ret)
    {
        Ret = RegisterTrayClockWndClass();
        if (!Ret)
        {
            UnregisterClass(szTrayNotifyWndClass,
                            hExplorerInstance);
        }
    }

    return Ret;
}

VOID
UnregisterTrayNotifyWndClass(VOID)
{
    UnregisterTrayClockWndClass();

    UnregisterClass(szTrayNotifyWndClass,
                    hExplorerInstance);
}
