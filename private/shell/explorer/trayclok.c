//---------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation 1991-1996
//
// Put a clock in a window.
//---------------------------------------------------------------------------
#include "cabinet.h"
#include "trayclok.h"
#ifndef WINNT
#include <pbt.h>
#endif

//---------------------------------------------------------------------------
extern HFONT     g_hfontCapNormal;
extern TRAYSTUFF g_ts;

TCHAR g_szTimeFmt[40] = TEXT(""); // The format string to pass to GetFormatTime
TCHAR g_szCurTime[40] = TEXT(""); // The current string.
int   g_cchCurTime = 0;
WORD  g_wLastHour;                // wHour from local time of last clock tick
WORD  g_wLastMinute;              // wMinute from local time of last clock tick
BOOL  g_fClockRunning = FALSE;
BOOL  g_fClockClipped = FALSE;

//--------------------------------------------------------------------------
void ClockCtl_UpdateLastHour(void)
{
    SYSTEMTIME st;

    //
    // Grab the time so we don't try to refresh the timezone information now.
    //
    GetLocalTime(&st);
    g_wLastHour = st.wHour;
    g_wLastMinute = st.wMinute;
}

//--------------------------------------------------------------------------
void ClockCtl_EnableTimer(HWND hwnd, DWORD dtNextTick)
{
    if (dtNextTick)
    {
        SetTimer(hwnd, 0, dtNextTick, NULL);
        g_fClockRunning = TRUE;
    }
    else if (g_fClockRunning)
    {
        g_fClockRunning = FALSE;
        KillTimer(hwnd, 0);
    }
}

//--------------------------------------------------------------------------
LRESULT ClockCtl_HandleCreate(HWND hwnd)
{
    ClockCtl_UpdateLastHour();
    return 1;
}

//---------------------------------------------------------------------------
LRESULT ClockCtl_HandleDestroy(HWND hwnd)
{
    ClockCtl_EnableTimer(hwnd, 0);
    return 1;
}

//---------------------------------------------------------------------------
DWORD ClockCtl_RecalcCurTime(HWND hwnd)
{
    SYSTEMTIME st;

    //
    // Current time.
    //
    GetLocalTime(&st);

    //
    // Don't recalc the text if the time hasn't changed yet.
    //
    if ((st.wMinute != g_wLastMinute) || (st.wHour != g_wLastHour) ||
        !*g_szCurTime)
    {
        g_wLastMinute = st.wMinute;

        //
        // Text for the current time.
        //
        g_cchCurTime = GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS,
            &st, g_szTimeFmt, g_szCurTime, ARRAYSIZE(g_szCurTime));

        // Don't count the NULL terminator.
        if (g_cchCurTime > 0)
            g_cchCurTime--;

        //
        // Update our window text so accessibility apps can see.  Since we
        // don't have a caption USER won't try to paint us or anything, it
        // will just set the text and fire an event if any accessibility
        // clients are listening...
        //
        SetWindowText(hwnd, g_szCurTime);

        //
        // Do a timezone check about once an hour (Win95).
        //
        if (st.wHour != g_wLastHour)
        {
#ifndef WINNT
            PostMessage(hwnd, TCM_TIMEZONEHACK, 0, 0L);
#endif
            g_wLastHour = st.wHour;
        }
    }

    //
    // Return number of milliseconds till we need to be called again.
    //
    return 1000UL * (60 - st.wSecond);
}

extern void Task_InitGlobalFonts();

void ClockCtl_EnsureFontsInitialized()
{
    if (!g_hfontCapNormal)
        Task_InitGlobalFonts();
}

//---------------------------------------------------------------------------
LRESULT ClockCtl_DoPaint(HWND hwnd, BOOL fPaint)
{
    PAINTSTRUCT ps;
    RECT rcClient, rcClip = {0};
    DWORD dtNextTick;
    BOOL fDoTimer;
    HDC hdc;

    //
    // If we are asked to paint and the clock is not running then start it.
    // Otherwise wait until we get a clock tick to recompute the time etc.
    //
    fDoTimer = !fPaint || !g_fClockRunning;

    //
    // Get a DC to paint with.
    //
    if (fPaint)
        hdc = BeginPaint(hwnd, &ps);
    else
        hdc = GetDC(hwnd);

    ClockCtl_EnsureFontsInitialized();

    //
    // Update the time if we need to.
    //
    if (fDoTimer || !*g_szCurTime)
    {
        dtNextTick = ClockCtl_RecalcCurTime(hwnd);

        ASSERT(dtNextTick);
    }

    //
    // Paint the clock face if we are not clipped or if we got a real
    // paint message for the window.  We want to avoid turning off the
    // timer on paint messages (regardless of clip region) because this
    // implies the window is visible in some way. If we guessed wrong, we
    // will turn off the timer next timer tick anyway so no big deal.
    //
    if (GetClipBox(hdc, &rcClip) != NULLREGION || fPaint)
    {
        HFONT hfontOld;
        SIZE size;
        int x, y;

        //
        // Draw the text centered in the window.
        //
        GetClientRect(hwnd, &rcClient);

        if (g_hfontCapNormal)
            hfontOld = SelectObject(hdc, g_hfontCapNormal);

        SetBkColor(hdc, GetSysColor(COLOR_3DFACE));
        SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));

        GetTextExtentPoint(hdc, g_szCurTime, g_cchCurTime, &size);

        x = max((rcClient.right - size.cx)/2, 0);
        y = max((rcClient.bottom - size.cy)/2, 0);
        ExtTextOut(hdc, x, y, ETO_OPAQUE,
                   &rcClient, g_szCurTime, g_cchCurTime, NULL);

        //  figure out if the time is clipped
        g_fClockClipped = (size.cx > rcClient.right || size.cy > rcClient.bottom);
        
        if (g_hfontCapNormal)
            SelectObject(hdc, hfontOld);
    }
    else
    {
        //
        // We are obscured so make sure we turn off the clock.
        //
        dtNextTick = 0;
        fDoTimer = TRUE;
    }

    //
    // Release our paint DC.
    //
    if (fPaint)
        EndPaint(hwnd, &ps);
    else
        ReleaseDC(hwnd, hdc);

    //
    // Reset/Kill the timer.
    //
    if (fDoTimer)
    {
        ClockCtl_EnableTimer(hwnd, dtNextTick);

        //
        // If we just killed the timer becuase we were clipped when it arrived,
        // make sure that we are really clipped by invalidating ourselves once.
        //
        if (!dtNextTick && !fPaint)
            InvalidateRect(hwnd, NULL, FALSE);
    }

    return 0;
}

//--------------------------------------------------------------------------
void ClockCtl_Reset(HWND hwnd)
{
    //
    // Reset the clock by killing the timer and invalidating.
    // Everything will be updated when we try to paint.
    //
    ClockCtl_EnableTimer(hwnd, 0);
    InvalidateRect(hwnd, NULL, FALSE);
}

//--------------------------------------------------------------------------
LRESULT ClockCtl_HandleTimeChange(HWND hwnd)
{
    *g_szCurTime = 0;   // Force a text recalc.
    ClockCtl_UpdateLastHour();
    ClockCtl_Reset(hwnd);
    return 1;
}

#define szSlop TEXT("00")

//---------------------------------------------------------------------------
LRESULT ClockCtl_CalcMinSize(HWND hWnd)
{
    RECT rc;
    HDC  hdc;
    HFONT hfontOld;
    SYSTEMTIME st={0};  // Initialize to 0...
    SIZE sizeAM;
    SIZE sizePM;
    TCHAR szTime[40];
    int cch;

    if (!(GetWindowLong(hWnd, GWL_STYLE) & WS_VISIBLE))
        return 0L;

    if (g_szTimeFmt[0] == TEXT('\0'))
    {
        if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STIMEFORMAT, g_szTimeFmt,
            ARRAYSIZE(g_szTimeFmt)) == 0)
        {
            TraceMsg(TF_ERROR, "c.ccms: GetLocalInfo Failed %d.", GetLastError());
        }

        *g_szCurTime = 0; // Force the text to be recomputed.
    }

    hdc = GetDC(hWnd);
    if (!hdc)
        return(0L);

    ClockCtl_EnsureFontsInitialized();

    if (g_hfontCapNormal)
        hfontOld = SelectObject(hdc, g_hfontCapNormal);

    // We need to get the AM and the PM sizes...
    // We append Two 0s and end to add slop into size

    st.wHour=11;
    cch = GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st,
            g_szTimeFmt, szTime, ARRAYSIZE(szTime) - ARRAYSIZE(szSlop));
    lstrcat(szTime, szSlop);
    GetTextExtentPoint(hdc, szTime, cch+2, &sizeAM);

    st.wHour=23;
    cch = GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st,
            g_szTimeFmt, szTime, ARRAYSIZE(szTime) - ARRAYSIZE(szSlop));
    lstrcat(szTime, szSlop);
    GetTextExtentPoint(hdc, szTime, cch+2, &sizePM);

    if (g_hfontCapNormal)
        SelectObject(hdc, hfontOld);

    ReleaseDC(hWnd, hdc);

    // Now lets set up our rectangle...
    // The width is 6 digits (a digit slop on both ends + size of
    // : or sep and max AM or PM string...)
    SetRect(&rc, 0, 0, max(sizeAM.cx, sizePM.cx),
            max(sizeAM.cy, sizePM.cy) + 4 * g_cyBorder);

    AdjustWindowRectEx(&rc, GetWindowLong(hWnd, GWL_STYLE), FALSE,
            GetWindowLong(hWnd, GWL_EXSTYLE));

    // make sure we're at least the size of other buttons:
    if (rc.bottom - rc.top <  g_cySize + g_cyEdge)
        rc.bottom = rc.top + g_cySize + g_cyEdge;

    return MAKELRESULT((rc.right - rc.left),
            (rc.bottom - rc.top));
}

//---------------------------------------------------------------------------
LRESULT ClockCtl_HandleIniChange(HWND hWnd, WPARAM wParam, LPTSTR pszSection)
{
    // Only process certain sections...
    if ((pszSection == NULL) || (lstrcmpi(pszSection, TEXT("intl")) == 0) ||
        (wParam == SPI_SETICONTITLELOGFONT))
    {
        TOOLINFO ti;

        g_szTimeFmt[0] = TEXT('\0');      // Go reread the format.

        // And make sure we have it recalc...
        ClockCtl_CalcMinSize(hWnd);

        ti.cbSize = SIZEOF(ti);
        ti.uFlags = 0;
        ti.hwnd = v_hwndTray;
        ti.uId = (UINT_PTR)hWnd;
        ti.lpszText = LPSTR_TEXTCALLBACK;
        SendMessage(g_ts.hwndTrayTips, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);

        ClockCtl_Reset(hWnd);
    }

    return 0;
}

LRESULT ClockCtl_OnNeedText(HWND hWnd, LPTOOLTIPTEXT lpttt)
{
    int iDateFormat = DATE_LONGDATE;

    //
    //  This code is really squirly.  We don't know if the time has been
    //  clipped until we actually try to paint it, since the clip logic
    //  is in the WM_PAINT handler...  Go figure...
    //
    if (!*g_szCurTime)
    {
        InvalidateRect(hWnd, NULL, FALSE);
        UpdateWindow(hWnd);
    }

    //
    // If the current user locale is any BiDi locale, then
    // Make the date reading order it RTL. SetBiDiDateFlags only adds
    // DATE_RTLREADING if the locale is BiDi. (see apithk.c for more info). [samera]
    //
    SetBiDiDateFlags(&iDateFormat);

    if (g_fClockClipped)
    {
        // we need to put the time in here too
        TCHAR sz[80];
        GetDateFormat(LOCALE_USER_DEFAULT, iDateFormat, NULL, NULL, sz, ARRAYSIZE(sz));
        wnsprintf(lpttt->szText, ARRAYSIZE(lpttt->szText), TEXT("%s %s"), g_szCurTime, sz);
    }
    else
    {
        GetDateFormat(LOCALE_USER_DEFAULT, iDateFormat, NULL, NULL, lpttt->szText, ARRAYSIZE(lpttt->szText));
    }

    return TRUE;
}


//---------------------------------------------------------------------------
LRESULT CALLBACK ClockCtl_WndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    switch (wMsg)
    {
    case WM_CALCMINSIZE:
        return ClockCtl_CalcMinSize(hWnd);

    case WM_CREATE:
        return ClockCtl_HandleCreate(hWnd);

    case WM_DESTROY:
        return ClockCtl_HandleDestroy(hWnd);

    case WM_PAINT:
    case WM_TIMER:
        return ClockCtl_DoPaint(hWnd, (wMsg == WM_PAINT));

    case WM_GETTEXT:
        //
        // Update the text if we are not running and somebody wants it.
        //
        if (!g_fClockRunning)
            ClockCtl_RecalcCurTime(hWnd);
        goto DoDefault;

    case WM_WININICHANGE:
        return ClockCtl_HandleIniChange(hWnd, wParam, (LPTSTR)lParam);

#ifndef WINNT
    case WM_POWERBROADCAST:
        switch (wParam)
        {
        case PBT_APMRESUMECRITICAL:
        case PBT_APMRESUMESTANDBY:
        case PBT_APMRESUMESUSPEND:
            goto TimeChanged;
        }
        break;
#endif

    case WM_POWER:
        //
        // a critical resume does not generate a WM_POWERBROADCAST
        // to windows for some reason, but it does generate a old
        // WM_POWER message.
        //
        if (wParam == PWR_CRITICALRESUME)
            goto TimeChanged;
        break;

    TimeChanged:
    case WM_TIMECHANGE:
        return ClockCtl_HandleTimeChange(hWnd);

    case WM_NCHITTEST:
        return(HTTRANSPARENT);

    case TCM_TIMEZONEHACK:
        DoDaylightCheck(FALSE);
        break;

    case WM_SHOWWINDOW:
        if (wParam)
            break;
        // fall through
    case TCM_RESET:
        ClockCtl_Reset(hWnd);
        break;

    case WM_NOTIFY:
    {
        NMHDR *pnm = (NMHDR*)lParam;
        switch (pnm->code)
        {
        case TTN_NEEDTEXT:
            return ClockCtl_OnNeedText(hWnd, (LPTOOLTIPTEXT)lParam);
            break;
        }
        break;
    }

    default:
    DoDefault:
        return (DefWindowProc(hWnd, wMsg, wParam, lParam));
    }

    return 0;
}

//---------------------------------------------------------------------------
// Register the clock class.
BOOL ClockCtl_Class(HINSTANCE hinst)
{
    WNDCLASS wc;

    ZeroMemory(&wc, SIZEOF(wc));
    wc.lpszClassName = WC_TRAYCLOCK;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = (WNDPROC)ClockCtl_WndProc;
    wc.hInstance = hinst;
    //wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
    //wc.lpszMenuName  = NULL;
    //wc.cbClsExtra = 0;
    //wc.cbWndExtra = 0;

    return RegisterClass(&wc);
}


/*
 ** ClockCtl_Create
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

HWND ClockCtl_Create(HWND hwndParent, UINT uID, HINSTANCE hInst)
{
    return(CreateWindow(WC_TRAYCLOCK,
        NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, 0, 0, 0, 0,
        hwndParent, (HMENU)uID, hInst, NULL));
}
