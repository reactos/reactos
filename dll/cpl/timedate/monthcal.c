/*
 * PROJECT:     ReactOS Timedate Control Panel
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/timedate/monthcal.c
 * PURPOSE:     Calander implementation
 * COPYRIGHT:   Copyright 2006 Thomas Weidenmueller <w3seek@reactos.com>
 *
 */

#include "timedate.h"

#include <windowsx.h>

static const WCHAR szMonthCalWndClass[] = L"MonthCalWnd";

#define MONTHCAL_HEADERBG   COLOR_INACTIVECAPTION
#define MONTHCAL_HEADERFG   COLOR_INACTIVECAPTIONTEXT
#define MONTHCAL_CTRLBG     COLOR_WINDOW
#define MONTHCAL_CTRLFG     COLOR_WINDOWTEXT
#define MONTHCAL_SELBG      COLOR_ACTIVECAPTION
#define MONTHCAL_SELFG      COLOR_CAPTIONTEXT
#define MONTHCAL_DISABLED_HEADERBG  COLOR_INACTIVECAPTION
#define MONTHCAL_DISABLED_HEADERFG  COLOR_INACTIVECAPTIONTEXT
#define MONTHCAL_DISABLED_CTRLBG    COLOR_WINDOW
#define MONTHCAL_DISABLED_CTRLFG    COLOR_WINDOWTEXT
#define MONTHCAL_DISABLED_SELBG     COLOR_INACTIVECAPTION
#define MONTHCAL_DISABLED_SELFG     COLOR_INACTIVECAPTIONTEXT

#define ID_DAYTIMER 1

typedef struct _MONTHCALWND
{
    HWND hSelf;
    HWND hNotify;
    WORD Day;
    WORD Month;
    WORD Year;
    WORD FirstDayOfWeek;
    BYTE Days[6][7];
    WCHAR Week[7];
    SIZE CellSize;
    SIZE ClientSize;

    HFONT hFont;
    HBRUSH hbHeader;
    HBRUSH hbSelection;

    DWORD UIState;
    UINT Changed : 1;
    UINT DayTimerSet : 1;
    UINT Enabled : 1;
    UINT HasFocus : 1;
} MONTHCALWND, *PMONTHCALWND;

static LRESULT
MonthCalNotifyControlParent(IN PMONTHCALWND infoPtr,
                            IN UINT code,
                            IN OUT PVOID data)
{
    LRESULT Ret = 0;

    if (infoPtr->hNotify != NULL)
    {
        LPNMHDR pnmh = (LPNMHDR)data;

        pnmh->hwndFrom = infoPtr->hSelf;
        pnmh->idFrom = GetWindowLongPtrW(infoPtr->hSelf,
                                         GWLP_ID);
        pnmh->code = code;

        Ret = SendMessageW(infoPtr->hNotify,
                           WM_NOTIFY,
                           (WPARAM)pnmh->idFrom,
                           (LPARAM)pnmh);
    }

    return Ret;
}

/*
 * For the year range 1..9999
 * return 1 if is leap year otherwise 0
 */
static WORD LeapYear(IN WORD Year)
{
	return
#ifdef WITH_1752
		(Year <= 1752) ? !(Year % 4) :
#endif
		!(Year % 4) && ((Year % 100) || !(Year % 400));
}

static WORD
MonthCalMonthLength(IN WORD Month,
                    IN WORD Year)
{
    const BYTE MonthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 0};

    if(Month == 2)
        return MonthDays[Month - 1] + LeapYear(Year);
    else
    {
#ifdef WITH_1752
        if ((Year == 1752) && (Month == 9))
	   return 19; // Special case: September 1752 has no 3rd-13th
	else
#endif
     	   return MonthDays[Month - 1];
    }
}

static WORD
MonthCalWeekInMonth(IN WORD Day,
                    IN WORD DayOfWeek)
{
    return (Day - DayOfWeek + 5) / 7;
}

static WORD
MonthCalDayOfWeek(IN PMONTHCALWND infoPtr,
                  IN WORD Day,
                  IN WORD Month,
                  IN WORD Year)
{
    const BYTE DayOfWeek[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    WORD Ret;

    Year -= (Month < 3);
    Ret = (Year + (Year / 4) - (Year / 100) + (Year / 400) + DayOfWeek[Month - 1] + Day + 6) % 7;

    return (7 + Ret - infoPtr->FirstDayOfWeek) % 7;
}

static WORD
MonthCalFirstDayOfWeek(VOID)
{
    WCHAR szBuf[2] = {0};
    WORD Ret = 0;

    if (GetLocaleInfoW(LOCALE_USER_DEFAULT,
                       LOCALE_IFIRSTDAYOFWEEK,
                       szBuf,
                       sizeof(szBuf) / sizeof(szBuf[0])) != 0)
    {
        Ret = (WORD)(szBuf[0] - TEXT('0'));
    }

    return Ret;
}

static BOOL
MonthCalValidDate(IN WORD Day,
                  IN WORD Month,
                  IN WORD Year)
{
    if (Month < 1 || Month > 12 ||
        Day == 0 || Day > MonthCalMonthLength(Month,
                                              Year) ||
        Year < 1899 || Year > 9999)
    {
        return FALSE;
    }

    return TRUE;
}

static VOID
MonthCalUpdate(IN PMONTHCALWND infoPtr)
{
    PBYTE pDay, pDayEnd;
    WORD DayOfWeek, MonthLength, d = 0;
    SIZE NewCellSize;
    BOOL RepaintHeader = FALSE;

    NewCellSize.cx = infoPtr->ClientSize.cx / 7;
    NewCellSize.cy = infoPtr->ClientSize.cy / 7;

    if (infoPtr->CellSize.cx != NewCellSize.cx ||
        infoPtr->CellSize.cy != NewCellSize.cy)
    {
        infoPtr->CellSize = NewCellSize;
        RepaintHeader = TRUE;
    }

    /* Update the days layout of the current month */
    ZeroMemory(infoPtr->Days,
               sizeof(infoPtr->Days));

    DayOfWeek = MonthCalDayOfWeek(infoPtr,
                                  1,
                                  infoPtr->Month,
                                  infoPtr->Year);

    MonthLength = MonthCalMonthLength(infoPtr->Month,
                                      infoPtr->Year);

    pDay = &infoPtr->Days[0][DayOfWeek];
    pDayEnd = pDay + MonthLength;
    while (pDay != pDayEnd)
    {
        *(pDay++) = (BYTE)++d;
    }

    /* Repaint the control */
    if (RepaintHeader)
    {
        InvalidateRect(infoPtr->hSelf,
                       NULL,
                       TRUE);
    }
    else
    {
        RECT rcClient;

        rcClient.left = 0;
        rcClient.top = infoPtr->CellSize.cy;
        rcClient.right = infoPtr->ClientSize.cx;
        rcClient.bottom = infoPtr->ClientSize.cy;

        InvalidateRect(infoPtr->hSelf,
                       &rcClient,
                       TRUE);
    }
}

static VOID
MonthCalSetupDayTimer(IN PMONTHCALWND infoPtr)
{
    SYSTEMTIME LocalTime = {0};
    UINT uElapse;

    /* Update the current date */
    GetLocalTime(&LocalTime);

    /* Calculate the number of remaining milliseconds until midnight */
    uElapse = 1000 - (UINT)LocalTime.wMilliseconds;
    uElapse += (59 - (UINT)LocalTime.wSecond) * 1000;
    uElapse += (59 - (UINT)LocalTime.wMinute) * 60 * 1000;
    uElapse += (23 - (UINT)LocalTime.wHour) * 60 * 60 * 1000;

    /* Setup the new timer */
    if (SetTimer(infoPtr->hSelf,
                 ID_DAYTIMER,
                 uElapse,
                 NULL) != 0)
    {
        infoPtr->DayTimerSet = TRUE;
    }
}

static VOID
MonthCalReload(IN PMONTHCALWND infoPtr)
{
    WCHAR szBuf[64];
    UINT i;

    infoPtr->UIState = (DWORD)SendMessageW(GetAncestor(infoPtr->hSelf,
                                                       GA_PARENT),
                                            WM_QUERYUISTATE,
                                            0,
                                            0);

    /* Cache the configuration */
    infoPtr->FirstDayOfWeek = MonthCalFirstDayOfWeek();

    infoPtr->hbHeader = GetSysColorBrush(infoPtr->Enabled ? MONTHCAL_HEADERBG : MONTHCAL_DISABLED_HEADERBG);
    infoPtr->hbSelection = GetSysColorBrush(infoPtr->Enabled ? MONTHCAL_SELBG : MONTHCAL_DISABLED_SELBG);

    for (i = 0; i < 7; i++)
    {
        if (GetLocaleInfoW(LOCALE_USER_DEFAULT,
                           LOCALE_SABBREVDAYNAME1 +
                               ((i + infoPtr->FirstDayOfWeek) % 7),
                           szBuf,
                           sizeof(szBuf) / sizeof(szBuf[0])) != 0)
        {
            infoPtr->Week[i] = szBuf[0];
        }
    }

    /* Update the control */
    MonthCalUpdate(infoPtr);
}

static BOOL
MonthCalGetDayRect(IN PMONTHCALWND infoPtr,
                   IN WORD Day,
                   OUT RECT *rcCell)
{
    if (Day >= 1 && Day <= MonthCalMonthLength(infoPtr->Month,
                                               infoPtr->Year))
    {
        WORD DayOfWeek;

        DayOfWeek = MonthCalDayOfWeek(infoPtr,
                                      Day,
                                      infoPtr->Month,
                                      infoPtr->Year);

        rcCell->left = DayOfWeek * infoPtr->CellSize.cx;
        rcCell->top = (MonthCalWeekInMonth(Day,
                                           DayOfWeek) + 1) * infoPtr->CellSize.cy;
        rcCell->right = rcCell->left + infoPtr->CellSize.cx;
        rcCell->bottom = rcCell->top + infoPtr->CellSize.cy;

        return TRUE;
    }

    return FALSE;
}

static VOID
MonthCalChange(IN PMONTHCALWND infoPtr)
{
    infoPtr->Changed = TRUE;

    /* Kill the day timer */
    if (infoPtr->DayTimerSet)
    {
        KillTimer(infoPtr->hSelf,
                  ID_DAYTIMER);
        infoPtr->DayTimerSet = FALSE;
    }
}


static BOOL
MonthCalSetDate(IN PMONTHCALWND infoPtr,
                IN WORD Day,
                IN WORD Month,
                IN WORD Year)
{
    NMMCCSELCHANGE sc;
    BOOL Ret = FALSE;

    sc.OldDay = infoPtr->Day;
    sc.OldMonth = infoPtr->Month;
    sc.OldYear = infoPtr->Year;
    sc.NewDay = Day;
    sc.NewMonth = Month;
    sc.NewYear = Year;

    /* Notify the parent */
    if (!MonthCalNotifyControlParent(infoPtr,
                                     MCCN_SELCHANGE,
                                     &sc))
    {
        /* Check if we actually need to update */
        if (infoPtr->Month != sc.NewMonth ||
            infoPtr->Year != sc.NewYear)
        {
            infoPtr->Day = sc.NewDay;
            infoPtr->Month = sc.NewMonth;
            infoPtr->Year = sc.NewYear;

            MonthCalChange(infoPtr);

            /* Repaint the entire control */
            MonthCalUpdate(infoPtr);

            Ret = TRUE;
        }
        else if (infoPtr->Day != sc.NewDay)
        {
            RECT rcUpdate;

            infoPtr->Day = sc.NewDay;

            MonthCalChange(infoPtr);

            if (MonthCalGetDayRect(infoPtr,
                                   sc.OldDay,
                                   &rcUpdate))
            {
                /* Repaint the day cells that need to be updated */
                InvalidateRect(infoPtr->hSelf,
                               &rcUpdate,
                               TRUE);
                if (MonthCalGetDayRect(infoPtr,
                                       sc.NewDay,
                                       &rcUpdate))
                {
                    InvalidateRect(infoPtr->hSelf,
                                   &rcUpdate,
                                   TRUE);
                }
            }

            Ret = TRUE;
        }
    }

    return Ret;
}

static VOID
MonthCalSetLocalTime(IN PMONTHCALWND infoPtr,
                     OUT SYSTEMTIME *Time)
{
    NMMCCAUTOUPDATE au;
    SYSTEMTIME LocalTime = {0};

    GetLocalTime(&LocalTime);

    au.SystemTime = LocalTime;
    if (!MonthCalNotifyControlParent(infoPtr,
                                     MCCN_AUTOUPDATE,
                                     &au))
    {
        if (MonthCalSetDate(infoPtr,
                            LocalTime.wDay,
                            LocalTime.wMonth,
                            LocalTime.wYear))
        {
            infoPtr->Changed = FALSE;
        }
    }

    /* Kill the day timer */
    if (infoPtr->DayTimerSet)
    {
        KillTimer(infoPtr->hSelf,
                  ID_DAYTIMER);
        infoPtr->DayTimerSet = FALSE;
    }

    /* Setup the new day timer */
    MonthCalSetupDayTimer(infoPtr);

    if (Time != NULL)
    {
        *Time = LocalTime;
    }
}

static VOID
MonthCalRepaintDay(IN PMONTHCALWND infoPtr,
                   IN WORD Day)
{
    RECT rcCell;

    if (MonthCalGetDayRect(infoPtr,
                           Day,
                           &rcCell))
    {
        InvalidateRect(infoPtr->hSelf,
                       &rcCell,
                       TRUE);
    }
}

static VOID
MonthCalPaint(IN PMONTHCALWND infoPtr,
              IN HDC hDC,
              IN LPRECT prcUpdate)
{
    LONG x, y;
    RECT rcCell;
    COLORREF crOldText, crOldCtrlText = CLR_INVALID;
    HFONT hOldFont;
    INT iOldBkMode;

#if MONTHCAL_CTRLBG != MONTHCAL_DISABLED_CTRLBG
    if (!infoPtr->Enabled)
    {
        FillRect(hDC,
                 prcUpdate,
                 GetSysColorBrush(MONTHCAL_DISABLED_CTRLBG));
    }
#endif

    iOldBkMode = SetBkMode(hDC,
                           TRANSPARENT);
    hOldFont = (HFONT)SelectObject(hDC,
                                   infoPtr->hFont);

    for (y = prcUpdate->top / infoPtr->CellSize.cy;
         y <= prcUpdate->bottom / infoPtr->CellSize.cy && y < 7;
         y++)
    {
        rcCell.top = y * infoPtr->CellSize.cy;
        rcCell.bottom = rcCell.top + infoPtr->CellSize.cy;

        if (y == 0)
        {
            RECT rcHeader;

            /* Paint the header */
            rcHeader.left = prcUpdate->left;
            rcHeader.top = rcCell.top;
            rcHeader.right = prcUpdate->right;
            rcHeader.bottom = rcCell.bottom;

            FillRect(hDC,
                     &rcHeader,
                     infoPtr->hbHeader);

            crOldText = SetTextColor(hDC,
                                     GetSysColor(infoPtr->Enabled ? MONTHCAL_HEADERFG : MONTHCAL_DISABLED_HEADERFG));

            for (x = prcUpdate->left / infoPtr->CellSize.cx;
                 x <= prcUpdate->right / infoPtr->CellSize.cx && x < 7;
                 x++)
            {
                rcCell.left = x * infoPtr->CellSize.cx;
                rcCell.right = rcCell.left + infoPtr->CellSize.cx;

                /* Write the first letter of each weekday */
                DrawTextW(hDC,
                          &infoPtr->Week[x],
                          1,
                          &rcCell,
                          DT_SINGLELINE | DT_NOPREFIX | DT_CENTER | DT_VCENTER);
            }

            SetTextColor(hDC,
                         crOldText);
        }
        else
        {
            if (crOldCtrlText == CLR_INVALID)
            {
                crOldCtrlText = SetTextColor(hDC,
                                             GetSysColor(infoPtr->Enabled ? MONTHCAL_CTRLFG : MONTHCAL_DISABLED_CTRLFG));
            }

            for (x = prcUpdate->left / infoPtr->CellSize.cx;
                 x <= prcUpdate->right / infoPtr->CellSize.cx && x < 7;
                 x++)
            {
                UINT Day = infoPtr->Days[y - 1][x];

                rcCell.left = x * infoPtr->CellSize.cx;
                rcCell.right = rcCell.left + infoPtr->CellSize.cx;

                /* Write the day number */
                if (Day != 0 && Day < 100)
                {
                    WCHAR szDay[3];
                    INT szDayLen;
                    RECT rcText;
                    SIZE TextSize;

                    szDayLen = swprintf(szDay,
                                         L"%lu",
                                         Day);

                    if (GetTextExtentPoint32W(hDC,
                                              szDay,
                                              szDayLen,
                                              &TextSize))
                    {
                        RECT rcHighlight = { 0, 0, 0, 0 };

                        rcText.left = rcCell.left + (infoPtr->CellSize.cx / 2) - (TextSize.cx / 2);
                        rcText.top = rcCell.top + (infoPtr->CellSize.cy / 2) - (TextSize.cy / 2);
                        rcText.right = rcText.left + TextSize.cx;
                        rcText.bottom = rcText.top + TextSize.cy;

                        if (Day == infoPtr->Day)
                        {
                            SIZE TextSel;

                            TextSel.cx = (infoPtr->CellSize.cx * 2) / 3;
                            TextSel.cy = (infoPtr->CellSize.cy * 3) / 4;

                            if (TextSel.cx < rcText.right - rcText.left)
                                TextSel.cx = rcText.right - rcText.left;
                            if (TextSel.cy < rcText.bottom - rcText.top)
                                TextSel.cy = rcText.bottom - rcText.top;

                            rcHighlight.left = rcCell.left + (infoPtr->CellSize.cx / 2) - (TextSel.cx / 2);
                            rcHighlight.right = rcHighlight.left + TextSel.cx;
                            rcHighlight.top = rcCell.top + (infoPtr->CellSize.cy / 2) - (TextSel.cy / 2);
                            rcHighlight.bottom = rcHighlight.top + TextSel.cy;

                            InflateRect(&rcHighlight,
                                        GetSystemMetrics(SM_CXFOCUSBORDER),
                                        GetSystemMetrics(SM_CYFOCUSBORDER));

                            if (!FillRect(hDC,
                                          &rcHighlight,
                                          infoPtr->hbSelection))
                            {
                                goto FailNoHighlight;
                            }

                            /* Highlight the selected day */
                            crOldText = SetTextColor(hDC,
                                                     GetSysColor(infoPtr->Enabled ? MONTHCAL_SELFG : MONTHCAL_DISABLED_SELFG));
                        }
                        else
                        {
FailNoHighlight:
                            /* Don't change the text color, we're not highlighting it... */
                            crOldText = CLR_INVALID;
                        }

                        TextOutW(hDC,
                                 rcText.left,
                                 rcText.top,
                                 szDay,
                                 szDayLen);

                        if (Day == infoPtr->Day && crOldText != CLR_INVALID)
                        {
                            if (infoPtr->HasFocus && infoPtr->Enabled && !(infoPtr->UIState & UISF_HIDEFOCUS))
                            {
                                COLORREF crOldBk;

                                crOldBk = SetBkColor(hDC,
                                                     GetSysColor(infoPtr->Enabled ? MONTHCAL_SELBG : MONTHCAL_DISABLED_SELBG));

                                DrawFocusRect(hDC,
                                              &rcHighlight);

                                SetBkColor(hDC,
                                           crOldBk);
                            }

                            SetTextColor(hDC,
                                         crOldText);
                        }
                    }
                }
            }
        }
    }

    if (crOldCtrlText != CLR_INVALID)
    {
        SetTextColor(hDC,
                     crOldCtrlText);
    }

    SetBkMode(hDC,
              iOldBkMode);
    SelectObject(hDC,
                 (HGDIOBJ)hOldFont);
}

static HFONT
MonthCalChangeFont(IN PMONTHCALWND infoPtr,
                   IN HFONT hFont,
                   IN BOOL Redraw)
{
    HFONT hOldFont = infoPtr->hFont;
    infoPtr->hFont = hFont;

    if (Redraw)
    {
        InvalidateRect(infoPtr->hSelf,
                       NULL,
                       TRUE);
    }

    return hOldFont;
}

static WORD
MonthCalPtToDay(IN PMONTHCALWND infoPtr,
                IN INT x,
                IN INT y)
{
    WORD Ret = 0;

    if (infoPtr->CellSize.cx != 0 && infoPtr->CellSize.cy != 0 &&
        x >= 0 && y >= 0)
    {
        x /= infoPtr->CellSize.cx;
        y /= infoPtr->CellSize.cy;

        if (x < 7 && y != 0 && y < 7)
        {
            Ret = (WORD)infoPtr->Days[y - 1][x];
        }
    }

    return Ret;
}

static LRESULT CALLBACK
MonthCalWndProc(IN HWND hwnd,
                IN UINT uMsg,
                IN WPARAM wParam,
                IN LPARAM lParam)
{
    PMONTHCALWND infoPtr;
    LRESULT Ret = 0;

    infoPtr = (PMONTHCALWND)GetWindowLongPtrW(hwnd,
                                              0);

    if (infoPtr == NULL && uMsg != WM_CREATE)
    {
        goto HandleDefaultMessage;
    }

    switch (uMsg)
    {
#if MONTHCAL_CTRLBG != MONTHCAL_DISABLED_CTRLBG
        case WM_ERASEBKGND:
            Ret = !infoPtr->Enabled;
            break;
#endif

        case WM_PAINT:
        case WM_PRINTCLIENT:
        {
            if (infoPtr->CellSize.cx != 0 && infoPtr->CellSize.cy != 0)
            {
                PAINTSTRUCT ps;
                HDC hDC;

                if (wParam != 0)
                {
                    if (!GetUpdateRect(hwnd,
                                       &ps.rcPaint,
                                       TRUE))
                    {
                        break;
                    }
                    hDC = (HDC)wParam;
                }
                else
                {
                    hDC = BeginPaint(hwnd,
                                     &ps);
                    if (hDC == NULL)
                    {
                        break;
                    }
                }

                MonthCalPaint(infoPtr,
                              hDC,
                              &ps.rcPaint);

                if (wParam == 0)
                {
                    EndPaint(hwnd,
                             &ps);
                }
            }
            break;
        }

        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        {
            WORD SelDay;

            SelDay = MonthCalPtToDay(infoPtr,
                                     GET_X_LPARAM(lParam),
                                     GET_Y_LPARAM(lParam));
            if (SelDay != 0 && SelDay != infoPtr->Day)
            {
                MonthCalSetDate(infoPtr,
                                SelDay,
                                infoPtr->Month,
                                infoPtr->Year);
            }

            /* Fall through */
        }

        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        {
            if (!infoPtr->HasFocus)
            {
                SetFocus(hwnd);
            }
            break;
        }

        case WM_KEYDOWN:
        {
            WORD NewDay = 0;

            switch (wParam)
            {
                case VK_UP:
                {
                    if (infoPtr->Day > 7)
                    {
                        NewDay = infoPtr->Day - 7;
                    }
                    break;
                }

                case VK_DOWN:
                {
                    if (infoPtr->Day + 7 <= MonthCalMonthLength(infoPtr->Month,
                                                                infoPtr->Year))
                    {
                        NewDay = infoPtr->Day + 7;
                    }
                    break;
                }

                case VK_LEFT:
                {
                    if (infoPtr->Day > 1)
                    {
                        NewDay = infoPtr->Day - 1;
                    }
                    break;
                }

                case VK_RIGHT:
                {
                    if (infoPtr->Day < MonthCalMonthLength(infoPtr->Month,
                                                           infoPtr->Year))
                    {
                        NewDay = infoPtr->Day + 1;
                    }
                    break;
                }
            }

            /* Update the selection */
            if (NewDay != 0)
            {
                MonthCalSetDate(infoPtr,
                                NewDay,
                                infoPtr->Month,
                                infoPtr->Year);
            }

            goto HandleDefaultMessage;
        }

        case WM_GETDLGCODE:
        {
            INT virtKey;

            virtKey = (lParam != 0 ? (INT)((LPMSG)lParam)->wParam : 0);
            switch (virtKey)
            {
                case VK_TAB:
                {
                    /* Change the UI status */
                    SendMessageW(GetAncestor(hwnd,
                                             GA_PARENT),
                                 WM_CHANGEUISTATE,
                                 MAKEWPARAM(UIS_INITIALIZE,
                                            0),
                                 0);
                    break;
                }
            }

            Ret |= DLGC_WANTARROWS;
            break;
        }

        case WM_SETFOCUS:
        {
            infoPtr->HasFocus = TRUE;
            MonthCalRepaintDay(infoPtr,
                               infoPtr->Day);
            break;
        }

        case WM_KILLFOCUS:
        {
            infoPtr->HasFocus = FALSE;
            MonthCalRepaintDay(infoPtr,
                               infoPtr->Day);
            break;
        }

        case WM_UPDATEUISTATE:
        {
            DWORD OldUIState;

            Ret = DefWindowProcW(hwnd,
                                 uMsg,
                                 wParam,
                                 lParam);

            OldUIState = infoPtr->UIState;
            switch (LOWORD(wParam))
            {
                case UIS_SET:
                    infoPtr->UIState |= HIWORD(wParam);
                    break;

                case UIS_CLEAR:
                    infoPtr->UIState &= ~HIWORD(wParam);
                    break;
            }

            if (infoPtr->UIState != OldUIState)
            {
                MonthCalRepaintDay(infoPtr,
                                   infoPtr->Day);
            }
            break;
        }

        case MCCM_SETDATE:
        {
            WORD Day, Month, Year, DaysCount;

            Day = LOWORD(wParam);
            Month = HIWORD(wParam);
            Year = LOWORD(lParam);

            if (Day == (WORD)-1)
                Day = infoPtr->Day;
            if (Month == (WORD)-1)
                Month = infoPtr->Month;
            if (Year == (WORD)-1)
                Year = infoPtr->Year;

            DaysCount = MonthCalMonthLength(Month,
                                            Year);
            if (Day > DaysCount)
                Day = DaysCount;

            if (MonthCalValidDate(Day,
                                  Month,
                                  Year))
            {
                if (Day != infoPtr->Day ||
                    Month != infoPtr->Month ||
                    Year != infoPtr->Year)
                {
                    Ret = MonthCalSetDate(infoPtr,
                                          Day,
                                          Month,
                                          Year);
                }
            }
            break;
        }

        case MCCM_GETDATE:
        {
            LPSYSTEMTIME lpSystemTime = (LPSYSTEMTIME)wParam;

            lpSystemTime->wYear = infoPtr->Year;
            lpSystemTime->wMonth = infoPtr->Month;
            lpSystemTime->wDay = infoPtr->Day;

            Ret = TRUE;
            break;
        }

        case MCCM_RESET:
        {
            MonthCalSetLocalTime(infoPtr,
                                 NULL);
            Ret = TRUE;
            break;
        }

        case MCCM_CHANGED:
        {
            Ret = infoPtr->Changed;
            break;
        }

        case WM_TIMER:
        {
            switch (wParam)
            {
                case ID_DAYTIMER:
                {
                    /* Kill the timer */
                    KillTimer(hwnd,
                              ID_DAYTIMER);
                    infoPtr->DayTimerSet = FALSE;

                    if (!infoPtr->Changed)
                    {
                        /* Update the system time and setup the new day timer */
                        MonthCalSetLocalTime(infoPtr,
                                             NULL);

                        /* Update the control */
                        MonthCalUpdate(infoPtr);
                    }
                    break;
                }
            }
            break;
        }

        case WM_SETFONT:
        {
            Ret = (LRESULT)MonthCalChangeFont(infoPtr,
                                              (HFONT)wParam,
                                              (BOOL)LOWORD(lParam));
            break;
        }

        case WM_SIZE:
        {
            infoPtr->ClientSize.cx = LOWORD(lParam);
            infoPtr->ClientSize.cy = HIWORD(lParam);
            infoPtr->CellSize.cx = infoPtr->ClientSize.cx / 7;
            infoPtr->CellSize.cy = infoPtr->ClientSize.cy / 7;

            /* Repaint the control */
            InvalidateRect(hwnd,
                           NULL,
                           TRUE);
            break;
        }

        case WM_GETFONT:
        {
            Ret = (LRESULT)infoPtr->hFont;
            break;
        }

        case WM_ENABLE:
        {
            infoPtr->Enabled = ((BOOL)wParam != FALSE);
            MonthCalReload(infoPtr);
            break;
        }

        case WM_STYLECHANGED:
        {
            if (wParam == GWL_STYLE)
            {
                unsigned int OldEnabled = infoPtr->Enabled;
                infoPtr->Enabled = !(((LPSTYLESTRUCT)lParam)->styleNew & WS_DISABLED);

                if (OldEnabled != infoPtr->Enabled)
                {
                    MonthCalReload(infoPtr);
                }
            }
            break;
        }

        case WM_CREATE:
        {
            infoPtr = (MONTHCALWND*) HeapAlloc(GetProcessHeap(),
                                0,
                                sizeof(MONTHCALWND));
            if (infoPtr == NULL)
            {
                Ret = (LRESULT)-1;
                break;
            }

            SetWindowLongPtrW(hwnd,
                              0,
                              (LONG_PTR)infoPtr);

            ZeroMemory(infoPtr,
                       sizeof(MONTHCALWND));

            infoPtr->hSelf = hwnd;
            infoPtr->hNotify = ((LPCREATESTRUCTW)lParam)->hwndParent;
            infoPtr->Enabled = !(((LPCREATESTRUCTW)lParam)->style & WS_DISABLED);

            MonthCalSetLocalTime(infoPtr,
                                 NULL);

            MonthCalReload(infoPtr);
            break;
        }

        case WM_DESTROY:
        {
            HeapFree(GetProcessHeap(),
                     0,
                     infoPtr);
            SetWindowLongPtrW(hwnd,
                              0,
                              (DWORD_PTR)NULL);
            break;
        }

        default:
        {
HandleDefaultMessage:
            Ret = DefWindowProcW(hwnd,
                                 uMsg,
                                 wParam,
                                 lParam);
            break;
        }
    }

    return Ret;
}

BOOL
RegisterMonthCalControl(IN HINSTANCE hInstance)
{
    WNDCLASSW wc = {0};

    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = MonthCalWndProc;
    wc.cbWndExtra = sizeof(PMONTHCALWND);
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(NULL,
                             (LPWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(MONTHCAL_CTRLBG + 1);
    wc.lpszClassName = szMonthCalWndClass;

    return RegisterClassW(&wc) != 0;
}

VOID
UnregisterMonthCalControl(IN HINSTANCE hInstance)
{
    UnregisterClassW(szMonthCalWndClass,
                     hInstance);
}
