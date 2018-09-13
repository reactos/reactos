/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    calendar.c

Abstract:

    This module implements the calendar control for the Date/Time applet.

Revision History:

--*/



//
//  Include Files.
//

#include "timedate.h"
#include "rc.h"




//
//  Constant Declarations.
//

#define DEF_FIRST_WEEKDAY   (6)

#define cBorderX             5
#define cBorderY             3
#define cBorderSelect        1

#define IS_FE_LANGUAGE(p)    (((p) == LANG_CHINESE)  ||         \
                              ((p) == LANG_JAPANESE) ||         \
                              ((p) == LANG_KOREAN))




//
//  Typedef Declarations.
//

//
//  Struture for global data.
//
typedef struct _CALINFO
{
    HWND    hwnd;       // the hwnd
    HFONT   hfontCal;   // the font to use
    BOOL    fFocus;     // do we have the focus
    int     cxBlank;    // size of a blank
    int     cxChar;     // the width of digits
    int     cyChar;     // the height of digits
} CALINFO, *PCALINFO;




////////////////////////////////////////////////////////////////////////////
//
//  GetFirstDayOfAnyWeek
//
//  For this function ONLY:
//    0 = Monday
//    6 = Sunday
//
////////////////////////////////////////////////////////////////////////////

int GetFirstDayOfAnyWeek()
{
    static int iDay = -1;

    if (iDay < 0)
    {
        TCHAR ch[2] = { 0 };

        *ch = TEXT('0') + DEF_FIRST_WEEKDAY;

        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK, ch, 2);

        iDay = ( ((*ch >= TEXT('0')) && (*ch <= TEXT('6')))
                     ? ((int)*ch - TEXT('0'))
                     : DEF_FIRST_WEEKDAY );
    }

    return (iDay);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetLocalWeekday
//
////////////////////////////////////////////////////////////////////////////

int GetLocalWeekday()
{
    //
    //  Convert local first day to 0==sunday and subtract from today.
    //
    return ((wDateTime[WEEKDAY] + 7 - ((GetFirstDayOfAnyWeek() + 1) % 7)) % 7);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetFirstDayOfTheMonth
//
////////////////////////////////////////////////////////////////////////////

int GetFirstDayOfTheMonth()
{
    return ((GetLocalWeekday() + 8 - (wDateTime[DAY] % 7)) % 7);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetDaysOfTheMonth
//
////////////////////////////////////////////////////////////////////////////

int GetDaysOfTheMonth(
    int iMonth)
{
    int cDays;
    int nYear;

    //
    //  Calculate the number of days in the current month -
    //  add one if this is a leap year and the month is February.
    //
    if (iMonth <= 7)
    {
        cDays = 30 + (iMonth % 2);
    }
    else
    {
        cDays = 31 - (iMonth % 2);
    }

    if (iMonth == 2)
    {
        cDays = 28;
        nYear = wDateTime[YEAR];
        if ((nYear % 4 == 0) && ((nYear % 100 != 0) || (nYear % 400 == 0)))
        {
            cDays++;
        }
    }

    return (cDays);
}


////////////////////////////////////////////////////////////////////////////
//
//  AdjustDeltaDay
//
//  Alters the variables in wDeltaDateTime, allowing a CANCEL button to
//  perform its job by resetting the time as if it had never been touched.
//  GetTime() and GetDate() should already have been called.
//
////////////////////////////////////////////////////////////////////////////

void AdjustDeltaDay(
    HWND hwnd,
    int iDay)
{
    GetDate();
    GetTime();

    if (wDateTime[DAY] != iDay)
    {
        wDeltaDateTime[DAY] += iDay - wDateTime[DAY];
        wPrevDateTime[DAY] = wDateTime[DAY] = iDay;
        SetDate();
        GetDate();

        //
        //  Let our parent know that we changed.
        //
        FORWARD_WM_COMMAND( GetParent(hwnd),
                            GetWindowLong(hwnd, GWL_ID),
                            hwnd,
                            CBN_SELCHANGE,
                            SendMessage );
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  ChangeCurrentDate
//
//  If we pass in iNewCol < 0, we simply want to invalidate todays date.
//  This is used when we gain and lose focus.
//
////////////////////////////////////////////////////////////////////////////

void ChangeCurrentDate(
    PCALINFO pci,
    int iNewCol,
    int iNewRow)
{
    int iFirstDay, iRow, iColumn;
    RECT rc, rcT;

    GetClientRect(pci->hwnd, &rc);
    iFirstDay = GetFirstDayOfTheMonth();
    iColumn = (wDateTime[DAY] - 1 + iFirstDay) % 7;
    iRow = 1 + ((wDateTime[DAY] - 1 + iFirstDay) / 7);

    rcT.left = (((rc.right - rc.left) * iColumn) / 7) + cBorderX - cBorderSelect;
    rcT.right = rcT.left + (pci->cxChar * 2) + (2 * cBorderSelect);
    rcT.top = ((rc.bottom - rc.top) * iRow ) / 7 + cBorderY - cBorderSelect;
    rcT.bottom = rcT.top + pci->cyChar + (2 * cBorderSelect);

    InvalidateRect(pci->hwnd, &rcT, FALSE);

    if (iNewCol >= 0)
    {
        AdjustDeltaDay(pci->hwnd, ((iNewRow - 1) * 7) + iNewCol + 1 - iFirstDay);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CalendarPaint
//
////////////////////////////////////////////////////////////////////////////

#ifdef UNICODE
  #define NUM_DBCS_CHARS     1         // 1 Unicode char
#else
  #define NUM_DBCS_CHARS     2         // 1 lead byte, 1 trail byte
#endif

BOOL CalendarPaint(
    PCALINFO pci,
    HWND hwnd)
{
    RECT rc, rcT;
    PAINTSTRUCT ps;
    HDC hdc;
    int iWeek, iWeekDay, iDay, iMaxDays;
    int iFirstDayOfWeek, iFirstDayOfMonth;
    TCHAR pszDate[3];
    TCHAR szShortDay[10];
    DWORD dwbkColor;
    COLORREF o_TextColor;
    LCID Locale;
    LANGID LangID = GetUserDefaultLangID();
    BOOL IsFELang = IS_FE_LANGUAGE(PRIMARYLANGID(LangID));

    iFirstDayOfMonth = GetFirstDayOfTheMonth();
    iMaxDays = GetDaysOfTheMonth(wDateTime[MONTH]);
    iDay = 1;
    pszDate[0] = TEXT(' ');
    pszDate[1] = TEXT('0');

    //
    //  Paint the background of the dates page.
    //
    hdc = BeginPaint(hwnd, &ps);
    GetClientRect(hwnd, &rc);
    FillRect(hdc, &rc, GetSysColorBrush(COLOR_WINDOW));

    //
    //  The day specifier.
    //
    rcT.left = rc.left;
    rcT.right = rc.right;
    rcT.top = rc.top;
    rcT.bottom = rc.top + ((rc.bottom - rc.top) / 7);
    FillRect(hdc, &rcT, GetSysColorBrush(COLOR_INACTIVECAPTION));

    //
    //  Fill the page.
    //
    SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
    SelectFont(hdc, pci->hfontCal);
    SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));

    //
    //  See if we need to calculate the size of characters.
    //
    if (pci->cxChar == 0)
    {
        DrawText(hdc, TEXT("0"), 1, &rcT, DT_CALCRECT);
        pci->cxChar = rcT.right - rcT.left;
        pci->cyChar = rcT.bottom - rcT.top;

        DrawText(hdc, TEXT(" "), 1, &rcT, DT_CALCRECT);
        pci->cxBlank = rcT.right - rcT.left;
    }

    for (iWeek = 1; (iWeek < 7); iWeek++)
    {
        for (iWeekDay = iFirstDayOfMonth;
             (iWeekDay < 7) && (iDay <= iMaxDays);
             iWeekDay++)
        {
            rcT.left = ((((rc.right - rc.left) * iWeekDay) / 7)) + cBorderX;
            rcT.top = (((rc.bottom - rc.top) * iWeek) / 7) + cBorderY;
            rcT.right = rcT.left + 20;
            rcT.bottom = rcT.top + 20;

            if (pszDate[1] == TEXT('9'))
            {
                pszDate[1] = TEXT('0');

                if (pszDate[0] == TEXT(' '))
                {
                    pszDate[0] = TEXT('1');
                }
                else
                {
                    pszDate[0] = pszDate[0] + 1;
                }
            }
            else
            {
                pszDate[1] = pszDate[1] + 1;
            }

            if (wDateTime[DAY] == iDay)
            {
                dwbkColor = GetBkColor(hdc);
                SetBkColor(hdc, GetSysColor(COLOR_ACTIVECAPTION));
                o_TextColor = GetTextColor(hdc);
                SetTextColor(hdc, GetSysColor(COLOR_CAPTIONTEXT));
            }

            ExtTextOut( hdc,
                        rcT.left,
                        rcT.top,
                        0,
                        &rcT,
                        (LPTSTR)pszDate,
                        2,
                        NULL );

            //
            //  If we drew it inverted - put it back.
            //
            if (wDateTime[DAY] == iDay)
            {
                //
                //  If we have the focus we also need to draw the focus
                //  rectangle for this item.
                //
                if (pci->fFocus)
                {
                    rcT.bottom = rcT.top + pci->cyChar;

                    if (iDay <= 9)
                    {
                        rcT.right = rcT.left + pci->cxChar + pci->cxBlank;
                    }
                    else
                    {
                        rcT.right = rcT.left + 2 * pci->cxChar;
                    }

                    DrawFocusRect(hdc, &rcT);
                }

                SetBkColor(hdc, dwbkColor);
                SetTextColor(hdc, o_TextColor);
            }

            iFirstDayOfMonth = 0;
            iDay++;
        }
    }

    //
    //  Set the FONT color for the SMTWTFS line.
    //
    dwbkColor = SetBkColor(hdc, GetSysColor(COLOR_INACTIVECAPTION));
    SetTextColor(hdc, GetSysColor(COLOR_INACTIVECAPTIONTEXT));

    iFirstDayOfWeek = GetFirstDayOfAnyWeek();

    if (!IsFELang)
    {
        //
        //  Not a FE locale.
        //
        //  If it's Arabic, then we want to use the US locale to get the
        //  first letter of the abbreviated day name to display in the
        //  calendar.
        //
        Locale = (PRIMARYLANGID(LangID) == LANG_ARABIC)
                   ? MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), SORT_DEFAULT)
                   : LOCALE_USER_DEFAULT;

        for (iWeekDay = 0; (iWeekDay < 7); iWeekDay++)
        {
            GetLocaleInfo( Locale,
                           LOCALE_SABBREVDAYNAME1 + (iWeekDay + iFirstDayOfWeek) % 7,
                           szShortDay,
                           sizeof(szShortDay) / sizeof(TCHAR) );

            if (*szShortDay)
            {
                *szShortDay = (TCHAR)CharUpper((LPTSTR)(DWORD)(TBYTE)*szShortDay);
            }

            TextOut( hdc,
                     (((rc.right - rc.left) * iWeekDay) / 7) + cBorderX,
                     cBorderY,
                     szShortDay,
                     1 );
        }
    }
    else
    {
        //
        //  FE Locale.
        //
        for (iWeekDay = 0; (iWeekDay < 7); iWeekDay++)
        {
            GetLocaleInfo( LOCALE_USER_DEFAULT,
                           LOCALE_SABBREVDAYNAME1 + (iWeekDay + iFirstDayOfWeek) % 7,
                           szShortDay,
                           sizeof(szShortDay) / sizeof(TCHAR) );

#ifndef UNICODE
            if (*szShortDay && !IsDBCSLeadByte(*szShortDay))
#else
            if (*szShortDay)
#endif
            {
                *szShortDay = (TCHAR)CharUpper((LPTSTR)(DWORD)(TBYTE)*szShortDay);
            }

            if ((PRIMARYLANGID(LangID) == LANG_CHINESE) &&
                (lstrlen(szShortDay) == 3 * NUM_DBCS_CHARS))
            {
                TextOut( hdc,
                         (((rc.right - rc.left) * iWeekDay) / 7) + cBorderX,
                         cBorderY,
                         (LangID == MAKELANGID( LANG_CHINESE,
                                                SUBLANG_CHINESE_HONGKONG ))
                           ? szShortDay
                           : szShortDay + (2 * NUM_DBCS_CHARS),
                         1 * NUM_DBCS_CHARS );
            }
            else
            {
                TextOut( hdc,
                         (((rc.right - rc.left) * iWeekDay) / 7) + cBorderX,
                         cBorderY,
                         szShortDay,
                         lstrlen(szShortDay) );
            }
        }
    }

    SetBkColor(hdc, dwbkColor);
    EndPaint(hwnd, &ps);
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  IsValidClick
//
////////////////////////////////////////////////////////////////////////////

BOOL IsValidClick(
    HWND hwnd,
    int x,
    int y)
{
    int iT;

    if (y == 0)
    {
        return (FALSE);
    }

    iT = GetFirstDayOfTheMonth();

    if ((y == 1) && (x < iT))
    {
        return (FALSE);
    }

    iT += GetDaysOfTheMonth(wDateTime[MONTH]) - 1;

    if (y > ((iT / 7) + 1))
    {
        return (FALSE);
    }

    if ((y == ((iT / 7) + 1)) && (x > (iT % 7)))
    {
        return (FALSE);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  HandleDateChange
//
////////////////////////////////////////////////////////////////////////////

BOOL HandleDateChange(
    PCALINFO pci,
    int x,
    int y)
{
    RECT rc, rcT;
    int ix, iy;

    GetClientRect(pci->hwnd, &rc);

    ix = (x * 7) / (rc.right - rc.left);
    iy = (y * 7) / (rc.bottom - rc.top);

    if (IsValidClick(pci->hwnd, ix, iy))
    {
        rcT.left = (((rc.right - rc.left) * ix)/ 7) + cBorderX - cBorderSelect;
        rcT.right = rcT.left + (2 * pci->cxChar) + (2 * cBorderSelect);
        rcT.top = ((rc.bottom - rc.top) * iy) / 7 + cBorderY - cBorderSelect;
        rcT.bottom = rcT.top + pci->cyChar + (2 * cBorderSelect);

        InvalidateRect(pci->hwnd, &rcT, FALSE);
        ChangeCurrentDate( pci,
                           (x * 7) / (rc.right - rc.left),
                           (y * 7) / (rc.bottom - rc.top) );

        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  HandleKeyDown
//
////////////////////////////////////////////////////////////////////////////

void HandleKeyDown(
    PCALINFO pci,
    int vk,
    LPARAM lParam)
{
    RECT rcT;

    //
    //  First thing, lets try to figure out what the current x and y is.
    //
    int ix = GetLocalWeekday();
    int iy = (wDateTime[DAY] + GetFirstDayOfTheMonth() - 1) / 7;

    switch (vk)
    {
        case ( VK_LEFT ) :
        {
            ix--;
            if (ix < 0)
            {
                ix = 6;
                iy--;
            }
            break;
        }
        case ( VK_RIGHT ) :
        {
            ix++;
            if (ix == 7)
            {
                ix = 0;
                iy++;
            }
            break;
        }
        case ( VK_UP ) :
        {
            iy--;
            break;
        }
        case ( VK_DOWN ) :
        {
            iy++;
            break;
        }
        default :
        {
            //
            //  Ignore the character.
            //
            return;
        }
    }

    //
    //  The y's are offset for the days of the week.
    //
    iy++;
    if (!IsValidClick(pci->hwnd, ix, iy))
    {
        return;
    }

    GetClientRect(pci->hwnd, &rcT);
    rcT.left = ((rcT.right * ix) / 7) + cBorderX - cBorderSelect;
    rcT.right = rcT.left + (2 * pci->cxChar) + (2 * cBorderSelect);
    rcT.top = (rcT.bottom * iy) / 7 + cBorderY - cBorderSelect;
    rcT.bottom = rcT.top + pci->cyChar + (2 * cBorderSelect);

    InvalidateRect(pci->hwnd, &rcT, FALSE);

    //
    //  First try, simply call to change the date.
    //
    ChangeCurrentDate(pci, ix, iy);
}


////////////////////////////////////////////////////////////////////////////
//
//  CalWndProc
//
////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK CalWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PCALINFO pci;

    pci = (PCALINFO)GetWindowLong(hwnd, 0);

    switch (message)
    {
        case ( WM_CREATE ) :
        {
            pci = (PCALINFO)LocalAlloc(LPTR, sizeof(CALINFO));
            if (pci == 0)
            {
                return (-1);
            }

            pci->hwnd = hwnd;
            SetWindowLong(hwnd, 0, (LONG)pci);

            GetDate();
            break;
        }
        case ( WM_NCDESTROY ) :
        {
            if (pci)
            {
                LocalFree((HLOCAL)pci);
            }
            break;
        }
        case ( WM_SETFONT ) :
        {
            if (wParam)
            {
                pci->hfontCal = (HFONT)wParam;
                pci->cxChar = 0;
            }
            break;
        }
        case ( WM_PAINT ) :
        {
            CalendarPaint(pci, hwnd);
            break;
        }
        case ( WM_LBUTTONDOWN ) :
        {
            SetFocus(hwnd);
            HandleDateChange(pci, LOWORD(lParam), HIWORD(lParam));
            break;
        }
        case ( WM_SETFOCUS ) :
        {
            pci->fFocus = TRUE;
            ChangeCurrentDate(pci, -1, -1);
            break;
        }
        case ( WM_KILLFOCUS ) :
        {
            pci->fFocus = FALSE;
            ChangeCurrentDate(pci, -1, -1);
            break;
        }
        case ( WM_KEYDOWN ) :
        {
            HandleKeyDown(pci, wParam, lParam);
            break;
        }
        case ( WM_GETDLGCODE ) :
        {
            return (DLGC_WANTARROWS);
            break;
        }
        default :
        {
            return ( DefWindowProc(hwnd, message, wParam, lParam) );
        }
    }
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  CalendarInit
//
////////////////////////////////////////////////////////////////////////////

TCHAR const c_szCalClass[] = CALENDAR_CLASS;

BOOL CalendarInit(
    HANDLE hInstance)
{
    WNDCLASS wc;

    if (!GetClassInfo(hInstance, c_szCalClass, &wc))
    {
        wc.style         = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = sizeof(PCALINFO);
        wc.hCursor       = NULL;
        wc.hbrBackground = NULL;
        wc.hIcon         = NULL;
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = c_szCalClass;
        wc.hInstance     = hInstance;
        wc.lpfnWndProc   = CalWndProc;

        return (RegisterClass(&wc));
    }
    return (TRUE);
}
