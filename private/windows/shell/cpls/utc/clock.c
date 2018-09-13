/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    clock.c

Abstract:

    This module implements the clock control for the Date/Time applet.

Revision History:

--*/



//
//  Include Files.
//

#include "timedate.h"
#include "rc.h"
#include "clock.h"




//
//  Constant Declarations.
//

#define TIMER_ID             1

#define SECONDSCALE          80
#define HHAND                TRUE
#define MHAND                FALSE
#define MAXBLOBWIDTH         25

#define REPAINT              0
#define HANDPAINT            1

#define OPEN_TLEN            450    /* < half second */

#define MINDESIREDHEIGHT     3




//
//  Macro Definitions.
//

#ifdef WIN32
  #define MoveTo(hdc, x, y)       MoveToEx(hdc, x, y, NULL)
  #define GetWindowInt(w, o)      GetWindowLong(w, o)
  #define SetWindowInt(w, o, i)   SetWindowLong(w, o, i)
#else
  #define GetWindowInt(w, o)      GetWindowWord(w, o)
  #define SetWindowInt(w, o, i)   SetWindowWord(w, o, i)
#endif




//
//  Typedef Declarations.
//

typedef struct
{
    int     hour;                   // 0 - 11 hours for analog clock
    int     minute;
    int     second;

} TIME;

typedef struct
{
    HWND    hWnd;               // Us.

    HWND    hwndGetTime;        // window to provide get/set time

    // Brushes
    HBRUSH  hbrColorWindow;
    HBRUSH  hbrBtnHighlight;
    HBRUSH  hbrForeground;
    HBRUSH  hbrBlobColor;

    // Pens
    HPEN    hpenForeground;
    HPEN    hpenBackground;
    HPEN    hpenBlobHlt;

    // Dimensions of clock
    RECT    clockRect;
    int     clockRadius;
    int     HorzRes;
    int     VertRes;
    int     aspectD;
    int     aspectN;

    // Position of clock
    POINT   clockCenter;

    // BUGBUG this should be SYSTEMTIME
    TIME    oTime;
    TIME    nTime;

} CLOCKSTR, *PCLOCKSTR;

typedef struct
{
    SHORT x;
    SHORT y;

} TRIG;


//
//  Array containing the sine and cosine values for hand positions.
//
POINT rCircleTable[] =
{
    { 0,     -7999},
    { 836,   -7956},
    { 1663,  -7825},
    { 2472,  -7608},
    { 3253,  -7308},
    { 3999,  -6928},
    { 4702,  -6472},
    { 5353,  -5945},
    { 5945,  -5353},
    { 6472,  -4702},
    { 6928,  -4000},
    { 7308,  -3253},
    { 7608,  -2472},
    { 7825,  -1663},
    { 7956,  -836 },

    { 8000,  0    },
    { 7956,  836  },
    { 7825,  1663 },
    { 7608,  2472 },
    { 7308,  3253 },
    { 6928,  4000 },
    { 6472,  4702 },
    { 5945,  5353 },
    { 5353,  5945 },
    { 4702,  6472 },
    { 3999,  6928 },
    { 3253,  7308 },
    { 2472,  7608 },
    { 1663,  7825 },
    { 836,   7956 },

    {  0,    7999 },
    { -836,  7956 },
    { -1663, 7825 },
    { -2472, 7608 },
    { -3253, 7308 },
    { -4000, 6928 },
    { -4702, 6472 },
    { -5353, 5945 },
    { -5945, 5353 },
    { -6472, 4702 },
    { -6928, 3999 },
    { -7308, 3253 },
    { -7608, 2472 },
    { -7825, 1663 },
    { -7956, 836  },

    { -7999, -0   },
    { -7956, -836 },
    { -7825, -1663},
    { -7608, -2472},
    { -7308, -3253},
    { -6928, -4000},
    { -6472, -4702},
    { -5945, -5353},
    { -5353, -5945},
    { -4702, -6472},
    { -3999, -6928},
    { -3253, -7308},
    { -2472, -7608},
    { -1663, -7825},
    { -836 , -7956},
};




//
//  Function prototypes.
//

LRESULT CALLBACK ClockWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void ClockCreate(HWND hWnd, PCLOCKSTR np);
void ClockTimer(HWND hWnd, UINT msg, PCLOCKSTR np);
void ClockPaint(PCLOCKSTR np, HDC hDC, int hint);
void ClockTimerInterval( HWND hWnd, PCLOCKSTR np );
void CompClockDim(HWND hWnd, PCLOCKSTR np);
void CreateTools(PCLOCKSTR np);
void DeleteTools(PCLOCKSTR np);
void DrawFace(HDC hDC, PCLOCKSTR np);
void DrawFatHand( HDC hDC, int pos, HPEN hPen, BOOL hHand, PCLOCKSTR np);
void DrawHand( HDC hDC, int pos, HPEN hPen, int scale, int patMode, PCLOCKSTR np);





////////////////////////////////////////////////////////////////////////////
//
//  ClockInit
//
//  Registers the clock class.
//
////////////////////////////////////////////////////////////////////////////

TCHAR const c_szClockClass[] = CLOCK_CLASS;

BOOL ClockInit(
    HINSTANCE hInstance)
{
    WNDCLASS wc;

    if (!GetClassInfo(hInstance, c_szClockClass, &wc))
    {
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = sizeof(PCLOCKSTR);
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = NULL;
        wc.hIcon         = NULL;
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = c_szClockClass;
        wc.hInstance     = hInstance;
        wc.style         = CS_VREDRAW | CS_HREDRAW ;
        wc.lpfnWndProc   = ClockWndProc;

        return (RegisterClass(&wc));
    }
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetTimeClock
//
//  Gets the time that we should display on the clock.
//  The client could have specified a function to call to get this
//  or an HWND to pass a message to to get it from.
//
////////////////////////////////////////////////////////////////////////////

void GetTimeClock(
    TIME *pt,
    PCLOCKSTR np)
{
    SYSTEMTIME st;

    //
    //  Call our time provider or default to GetTime.
    //
    if (np->hwndGetTime)
    {
        SendMessage( np->hwndGetTime,
                     CLM_UPDATETIME,
                     CLF_GETTIME,
                     (LPARAM)(LPSYSTEMTIME)&st );
        pt->hour = st.wHour % 12;
        pt->minute = st.wMinute;
        pt->second = st.wSecond;
    }
    else
    {
#ifdef WIN32
        GetLocalTime(&st);

        pt->hour = st.wHour;
        pt->minute = st.wMinute;
        pt->second = st.wSecond;
#else
        //
        // No function call back and no HWND callback.
        //
        GetTime();
        pt->hour = wDateTime[HOUR] % 12;
        pt->minute = wDateTime[MINUTE];
        pt->second = wDateTime[SECOND];
#endif
     }
}


////////////////////////////////////////////////////////////////////////////
//
//  CreateTools
//
////////////////////////////////////////////////////////////////////////////

void CreateTools(
    PCLOCKSTR np)
{
    #define BLOB_COLOR  RGB(0, 128, 128)

    np->hbrForeground   = GetSysColorBrush(COLOR_BTNSHADOW);
    np->hbrColorWindow  = GetSysColorBrush(COLOR_BTNFACE);
    np->hbrBlobColor    = CreateSolidBrush( BLOB_COLOR );
    np->hbrBtnHighlight = GetSysColorBrush(COLOR_BTNHIGHLIGHT);
    np->hpenForeground  = CreatePen(0, 1, GetSysColor(COLOR_WINDOWTEXT));
    np->hpenBackground  = CreatePen(0, 1, GetSysColor(COLOR_BTNFACE));
    np->hpenBlobHlt     = CreatePen(0, 1, RGB(0, 255, 255));
}


////////////////////////////////////////////////////////////////////////////
//
//  DeleteTools
//
////////////////////////////////////////////////////////////////////////////

void DeleteTools(
    PCLOCKSTR np)
{
//  DeleteObject(np->hbrForeground);
//  DeleteObject(np->hbrColorWindow);
    DeleteObject(np->hbrBlobColor);
//  DeleteObject(np->hbrBtnHighlight);
    DeleteObject(np->hpenForeground);
    DeleteObject(np->hpenBackground);
    DeleteObject(np->hpenBlobHlt);
}


////////////////////////////////////////////////////////////////////////////
//
//  CompClockDim
//
//  Calculates the clock dimensions.
//
////////////////////////////////////////////////////////////////////////////

void CompClockDim(
    HWND hWnd,
    PCLOCKSTR np)
{
    int i;
    int tWidth;
    int tHeight;

    tWidth = np->clockRect.right - np->clockRect.left;
    tHeight = np->clockRect.bottom - np->clockRect.top;

    if (tWidth > MulDiv(tHeight,np->aspectD,np->aspectN))
    {
        i = MulDiv(tHeight, np->aspectD, np->aspectN);
        np->clockRect.left += (tWidth - i) / 2;
        np->clockRect.right = np->clockRect.left + i;
    }
    else
    {
        i = MulDiv(tWidth, np->aspectN, np->aspectD);
        np->clockRect.top += (tHeight - i) / 2;
        np->clockRect.bottom = np->clockRect.top + i;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  ClockTimerInterval
//
//  Sets the timer interval. Two things affect this interval:
//    1) if the window is iconic, or
//    2) if seconds option has been disabled
//  In both cases, timer ticks occur every half-minute. Otherwise, timer
//  every half-second.
//
////////////////////////////////////////////////////////////////////////////

void ClockTimerInterval(
    HWND hWnd,
    PCLOCKSTR np)
{
    //
    //  Update every 1/2 second in the opened state.
    //
    KillTimer(hWnd, TIMER_ID);
    SetTimer(hWnd, TIMER_ID, OPEN_TLEN, 0L);
}


////////////////////////////////////////////////////////////////////////////
//
//  ClockSize
//
//  Sizes the clock to the specified size.
//
////////////////////////////////////////////////////////////////////////////

void ClockSize(
    PCLOCKSTR np,
    int newWidth,
    int newHeight)
{
    SetRect(&np->clockRect, 0, 0, newWidth, newHeight);
    CompClockDim(np->hWnd, np);
    ClockTimerInterval(np->hWnd, np);
}


////////////////////////////////////////////////////////////////////////////
//
//  DrawFace
//
//  Draws the clock face.
//
////////////////////////////////////////////////////////////////////////////

void DrawFace(
    HDC hDC,
    PCLOCKSTR np)
{
    int i;
    RECT tRect;
    LPPOINT ppt;
    int blobHeight, blobWidth;

    blobWidth = MulDiv( MAXBLOBWIDTH,
                        (np->clockRect.right - np->clockRect.left),
                        np->HorzRes );
    blobHeight = MulDiv(blobWidth, np->aspectN, np->aspectD);

    if (blobHeight < 2)
    {
        blobHeight = 1;
    }

    if (blobWidth < 2)
    {
        blobWidth = 2;
    }

    InflateRect(&np->clockRect, -(blobHeight / 2), -(blobWidth / 2));

    np->clockRadius = (np->clockRect.right - np->clockRect.left - 8) / 2;
    np->clockCenter.y = np->clockRect.top +
                        ((np->clockRect.bottom - np->clockRect.top) / 2) - 1;
    np->clockCenter.x = np->clockRect.left + np->clockRadius + 3;

    for (i = 0; i < 60; i++)
    {
        ppt = rCircleTable + i;

        tRect.top  = MulDiv(ppt->y, np->clockRadius, 8000) + np->clockCenter.y;
        tRect.left = MulDiv(ppt->x, np->clockRadius, 8000) + np->clockCenter.x;

        if (i % 5)
        {
            //
            //  Draw a dot.
            //
            if (blobWidth > 2 && blobHeight >= 2)
            {
                tRect.right = tRect.left + 2;
                tRect.bottom = tRect.top + 2;
                FillRect(hDC, &tRect, GetStockObject(WHITE_BRUSH));
                OffsetRect(&tRect, -1, -1);
                FillRect(hDC, &tRect, np->hbrForeground);
                tRect.left++;
                tRect.top++;
                FillRect(hDC, &tRect, np->hbrColorWindow);
            }
        }
        else
        {
            tRect.right = tRect.left + blobWidth;
            tRect.bottom = tRect.top + blobHeight;
            OffsetRect(&tRect, -(blobWidth / 2) , -(blobHeight / 2));

            SelectObject(hDC, GetStockObject(BLACK_PEN));
            SelectObject(hDC, np->hbrBlobColor);

            Rectangle(hDC, tRect.left, tRect.top, tRect.right, tRect.bottom);
            SelectObject(hDC, np->hpenBlobHlt);
            MoveTo(hDC, tRect.left, tRect.bottom - 1);
            LineTo(hDC, tRect.left, tRect.top);
            LineTo(hDC, tRect.right - 1, tRect.top);
        }
    }

    InflateRect(&np->clockRect, blobHeight / 2, blobWidth / 2);
}


////////////////////////////////////////////////////////////////////////////
//
//  DrawHand
//
//  Draws the hands of the clock.
//
////////////////////////////////////////////////////////////////////////////

void DrawHand(
    HDC hDC,
    int pos,
    HPEN hPen,
    int scale,
    int patMode,
    PCLOCKSTR np)
{
    LPPOINT lppt;
    int radius;

    MoveTo(hDC, np->clockCenter.x, np->clockCenter.y);
    radius = MulDiv(np->clockRadius, scale, 100);
    lppt = rCircleTable + pos;
    SetROP2(hDC, patMode);
    SelectObject(hDC, hPen);

    LineTo( hDC,
            np->clockCenter.x + MulDiv(lppt->x, radius, 8000),
            np->clockCenter.y + MulDiv(lppt->y, radius, 8000) );
}


////////////////////////////////////////////////////////////////////////////
//
//  Adjust
//
////////////////////////////////////////////////////////////////////////////

void Adjust(
    POINT *rgpt,
    int cPoint,
    int iDelta)
{
    int i;

    for (i = 0; i < cPoint; i++)
    {
        rgpt[i].x += iDelta;
        rgpt[i].y += iDelta;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  DrawFatHand
//
//  Draws either hour or minute hand.
//
////////////////////////////////////////////////////////////////////////////

void DrawFatHand(
    HDC hDC,
    int pos,
    HPEN hPen,
    BOOL hHand,
    PCLOCKSTR np)
{
    int m;
    int n;
    int scale;

    TRIG tip;
    TRIG stip;
    BOOL fErase;
    POINT rgpt[4];
    HBRUSH hbrInit;

    SetROP2(hDC, 13);
    fErase = (hPen == np->hpenBackground);

    SelectObject(hDC, hPen);

    scale = hHand ? 7 : 5;

    n = (pos + 15) % 60;
    m = MulDiv(np->clockRadius, scale, 100);

    stip.y = (SHORT)MulDiv(rCircleTable[n].y, m, 8000);
    stip.x = (SHORT)MulDiv(rCircleTable[n].x, m, 8000);

    scale = hHand ? 65 : 80;
    tip.y = (SHORT)MulDiv(rCircleTable[pos % 60].y, MulDiv(np->clockRadius, scale, 100), 8000);
    tip.x = (SHORT)MulDiv(rCircleTable[pos % 60].x, MulDiv(np->clockRadius, scale, 100), 8000);

    rgpt[0].x = np->clockCenter.x + stip.x;
    rgpt[0].y = np->clockCenter.y + stip.y;
    rgpt[1].x = np->clockCenter.x + tip.x;
    rgpt[1].y = np->clockCenter.y + tip.y;
    rgpt[2].x = np->clockCenter.x - stip.x;
    rgpt[2].y = np->clockCenter.y - stip.y;

    scale = hHand ? 15 : 20;

    n = (pos + 30) % 60;
    m = MulDiv(np->clockRadius, scale, 100);
    tip.y = (SHORT)MulDiv(rCircleTable[n].y, m, 8000);
    tip.x = (SHORT)MulDiv(rCircleTable[n].x, m, 8000);

    rgpt[3].x = np->clockCenter.x + tip.x;
    rgpt[3].y = np->clockCenter.y + tip.y;

    SelectObject(hDC, GetStockObject(NULL_PEN));

    if (fErase)
    {
        hbrInit = SelectObject(hDC, np->hbrColorWindow);
    }

    Adjust(rgpt, 4, -2);

    if (!fErase)
    {
        hbrInit = SelectObject(hDC, np->hbrBtnHighlight);
    }

    Polygon(hDC, rgpt, 4);

    if (!fErase)
    {
        SelectObject(hDC, np->hbrForeground);
    }

    Adjust(rgpt, 4, 4);
    Polygon(hDC, rgpt, 4);

    Adjust(rgpt, 4, -2);

    if (!fErase)
    {
        SelectObject(hDC, np->hbrBlobColor);
    }

    Polygon(hDC, rgpt, 4);

    //
    //  If we selected a brush in, reset it now.
    //
    if (fErase)
    {
        SelectObject(hDC, hbrInit);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  ClockPaint
//
//  Only paints the clock.
//
//  It assumes you have set nTime already.  This allows it to be called by
//  the timer or by the client.
//
////////////////////////////////////////////////////////////////////////////

void ClockPaint(
    PCLOCKSTR np,
    HDC hDC,
    int hint)
{
    SetBkMode(hDC, TRANSPARENT);

    if (hint == REPAINT)
    {
        //
        //  If doing a full repaint, we do not advance the time.
        //  Otherwise we will create artifacts when there is a clipping
        //  region.
        //
        DrawFace(hDC, np);

        DrawFatHand(hDC, np->oTime.hour * 5 + (np->oTime.minute / 12), np->hpenForeground, HHAND,np);
        DrawFatHand(hDC, np->oTime.minute, np->hpenForeground, MHAND,np);

        //
        //  Draw the second hand.
        //
        DrawHand(hDC, np->oTime.second, np->hpenBackground, SECONDSCALE, R2_NOT,np);
    }
    else if (hint == HANDPAINT)
    {
        DrawHand(hDC, np->oTime.second, np->hpenBackground, SECONDSCALE, R2_NOT, np);

        if (np->nTime.minute != np->oTime.minute || np->nTime.hour != np->oTime.hour)
        {
            DrawFatHand(hDC, np->oTime.minute, np->hpenBackground, MHAND, np);
            DrawFatHand(hDC, np->oTime.hour * 5 + (np->oTime.minute / 12), np->hpenBackground, HHAND,np);
            DrawFatHand(hDC, np->nTime.minute, np->hpenForeground, MHAND, np);
            DrawFatHand(hDC, (np->nTime.hour) * 5 + (np->nTime.minute / 12), np->hpenForeground, HHAND, np);
        }

        DrawHand(hDC, np->nTime.second, np->hpenBackground, SECONDSCALE, R2_NOT, np);
        np->oTime = np->nTime;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  ClockTimer
//
//  Update the clock.  Called on a timer tick.
//
////////////////////////////////////////////////////////////////////////////

void ClockTimer(
    HWND hWnd,
    UINT msg,
    PCLOCKSTR np)
{
    HDC hDC;

    GetTimeClock(&np->nTime, np);

    if ((np->nTime.second == np->oTime.second) &&
        (np->nTime.minute == np->oTime.minute) &&
        (np->nTime.hour == np->oTime.hour))
    {
        return;
    }

    hDC = GetDC(hWnd);
    ClockPaint(np, hDC, HANDPAINT);
    ReleaseDC(hWnd, hDC);
}


////////////////////////////////////////////////////////////////////////////
//
//  ClockCreate
//
////////////////////////////////////////////////////////////////////////////

void ClockCreate(
    HWND hWnd,
    PCLOCKSTR np)
{
    int i;
    HDC hDC;
    int HorzSize;
    int VertSize;
    LPPOINT lppt;

    hDC = GetDC(hWnd);
    np->VertRes = GetDeviceCaps(hDC, VERTRES);
    np->HorzRes = GetDeviceCaps(hDC, HORZRES);
    VertSize= GetDeviceCaps(hDC, VERTSIZE);
    HorzSize= GetDeviceCaps(hDC, HORZSIZE);
    ReleaseDC(hWnd, hDC);
    np->aspectN = MulDiv(np->VertRes, 100, VertSize);
    np->aspectD = MulDiv(np->HorzRes, 100, HorzSize);

    //
    //  Instance stuff.
    //
    np->hWnd = hWnd;

    CreateTools(np);

    //
    //  Scale cosines for aspect ratio if this is the first instance.
    //
    for (i = 0; i < 60; i++)
    {
        lppt = rCircleTable + i;
        lppt->y = MulDiv(lppt->y, np->aspectN, np->aspectD);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  ClockWndProc
//
//  Deals with the clock messages.
//
////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK ClockWndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PCLOCKSTR np = (PCLOCKSTR)GetWindowInt(hWnd, 0);

    switch (message)
    {
        case ( WM_DESTROY ) :
        {
            if (np)
            {
                KillTimer(hWnd, TIMER_ID);
                DeleteTools(np);
                LocalFree((HLOCAL)np);
                SetWindowInt(hWnd, 0, 0);
            }
            break;
        }
        case ( WM_CREATE ) :
        {
            //
            //  Allocate the instance data space.
            //
            np = (PCLOCKSTR)LocalAlloc(LPTR, sizeof(CLOCKSTR));
            if (!np)
            {
                return (-1);
            }

            SetWindowInt(hWnd, 0, (int)np);

            ClockCreate(hWnd, np);

            //
            //  Loop if control panel time being changed.
            //
            GetTimeClock(&(np->nTime), np);
            do
            {
                GetTimeClock(&(np->oTime), np);
            } while (np->nTime.second == np->oTime.second &&
                     np->nTime.minute == np->oTime.minute &&
                     np->nTime.hour == np->oTime.hour);

            SetTimer(hWnd, TIMER_ID, OPEN_TLEN, 0L);

            ClockSize( np,
                       ((LPCREATESTRUCT)lParam)->cx,
                       ((LPCREATESTRUCT)lParam)->cy );
            break;
        }
        case ( WM_SIZE ) :
        {
            if (np)
            {
                ClockSize(np, LOWORD(lParam), HIWORD(lParam));
            }
            break;
        }
        case ( WM_PAINT ) :
        {
            PAINTSTRUCT ps;

            BeginPaint(hWnd, &ps);
            GetTimeClock(&(np->nTime), np);
            ClockPaint(np, ps.hdc, REPAINT);
            EndPaint(hWnd, &ps);

            break;
        }
        case ( WM_TIMECHANGE ) :
        {
            //
            //  I'm not top level - so I wont get this message.
            //
            InvalidateRect(hWnd, NULL, TRUE);
            if (np->hwndGetTime)
            {
                SYSTEMTIME System;

                GetTime();
                System.wHour = wDateTime[HOUR];
                System.wMinute = wDateTime[MINUTE];
                System.wSecond = wDateTime[SECOND];
                SendMessage( np->hwndGetTime,
                             CLM_UPDATETIME,
                             CLF_SETTIME,
                             (LPARAM)(LPSYSTEMTIME)&System );
            }

            // fall thru...
        }
        case ( WM_TIMER ) :
        {
            ClockTimer(hWnd, wParam, np);
            break;
        }
        case ( WM_SYSCOLORCHANGE ) :
        {
            DeleteTools(np);
            CreateTools(np);
            break;
        }
        case ( CLM_UPDATETIME ) :
        {
            //
            //  Force the clock to repaint. lParam will point to a
            //  SYSTEMTIME struct.
            //
            switch (wParam)
            {
                case ( CLF_SETTIME ) :
                {
                    //
                    //  Caller wants us to reflect a new time.
                    //
                    HDC hDC;
                    LPSYSTEMTIME lpSysTime = (LPSYSTEMTIME)lParam;

                    np->nTime.hour   = lpSysTime->wHour;
                    np->nTime.minute = lpSysTime->wMinute;
                    np->nTime.second = lpSysTime->wSecond;

                    hDC = GetDC(hWnd);
                    ClockPaint(np, hDC, HANDPAINT);
                    ReleaseDC(hWnd, hDC);
                    break;
                }
                case ( CLF_GETTIME ) :
                {
                    //
                    //  Caller wants to know what we think the time is.
                    //
                    LPSYSTEMTIME lpSysTime = (LPSYSTEMTIME)lParam;

                    lpSysTime->wHour = np->nTime.hour;
                    lpSysTime->wMinute = np->nTime.minute;
                    lpSysTime->wSecond = np->nTime.second;
                    break;
                }
            }
            break;
        }
        case ( CLM_TIMEHWND ) :
        {
            //
            //  Get/Set the HWND that we ask to provide the time.
            //
            switch (wParam)
            {
                case ( CLF_SETHWND ) :
                {
                    //
                    //  Caller wants us to reflect a new time.
                    //
                    np->hwndGetTime = (HWND)lParam;
                    break;
                }
                case ( CLF_GETTIME ) :
                {
                    //
                    //  Caller wants to know what we think the time is.
                    //
                    *((HWND *)lParam) = np->hwndGetTime;
                    break;
                }
            }
            break;
        }
        default :
        {
            return ( DefWindowProc(hWnd, message, wParam, lParam) );
        }
    }

    return (0);
}
