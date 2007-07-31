/*
 * PROJECT:     ReactOS Timedate Control Panel
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/timedate/clock.c
 * PURPOSE:     Draws the analog clock
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *              Copyright 2007 Eric Kohl
 */

/* Code based on clock.c from Programming Windows, Charles Petzold */

#include <timedate.h>

typedef struct _CLOCKDATA
{
    HBRUSH hGreyBrush;
    HPEN hGreyPen;
    INT cxClient;
    INT cyClient;
    SYSTEMTIME stCurrent;
    SYSTEMTIME stPrevious;
    BOOL bTimer;
} CLOCKDATA, *PCLOCKDATA;


#define TWOPI (2 * 3.14159)

static const WCHAR szClockWndClass[] = L"ClockWndClass";


static VOID
SetIsotropic(HDC hdc, PCLOCKDATA pClockData)
{
    /* set isotropic mode */
     SetMapMode(hdc, MM_ISOTROPIC);
     /* position axis in centre of window */
     SetViewportOrgEx(hdc, pClockData->cxClient / 2, pClockData->cyClient / 2, NULL);
}


static VOID
RotatePoint(POINT pt[], INT iNum, INT iAngle)
{
     INT i;
     POINT ptTemp;

     for (i = 0 ; i < iNum ; i++)
     {
          ptTemp.x = (INT) (pt[i].x * cos (TWOPI * iAngle / 360) +
               pt[i].y * sin (TWOPI * iAngle / 360));

          ptTemp.y = (INT) (pt[i].y * cos (TWOPI * iAngle / 360) -
               pt[i].x * sin (TWOPI * iAngle / 360));

          pt[i] = ptTemp;
     }
}


static VOID
DrawClock(HDC hdc, PCLOCKDATA pClockData)
{
     INT iAngle;
     POINT pt[3];
     HBRUSH hBrushOld;
     HPEN hPenOld = NULL;

     /* grey brush to fill the dots */
     hBrushOld = SelectObject(hdc, pClockData->hGreyBrush);

     hPenOld = GetCurrentObject(hdc, OBJ_PEN);

     for (iAngle = 0; iAngle < 360; iAngle += 6)
     {
          /* starting coords */
          pt[0].x = 0;
          pt[0].y = 180;

          /* rotate start coords */
          RotatePoint(pt, 1, iAngle);

          /* determine whether it's a big dot or a little dot
           * i.e. 1-4 or 5, 6-9 or 10, 11-14 or 15 */
          if (iAngle % 5)
          {
                pt[2].x = pt[2].y = 7;
                SelectObject(hdc, pClockData->hGreyPen);
          }
          else
          {
              pt[2].x = pt[2].y = 16;
              SelectObject(hdc, GetStockObject(BLACK_PEN));
          }

          pt[0].x -= pt[2].x / 2;
          pt[0].y -= pt[2].y / 2;

          pt[1].x  = pt[0].x + pt[2].x;
          pt[1].y  = pt[0].y + pt[2].y;

          Ellipse(hdc, pt[0].x, pt[0].y, pt[1].x, pt[1].y);
     }

     SelectObject(hdc, hBrushOld);
     SelectObject(hdc, hPenOld);
}


static VOID
DrawHands(HDC hdc, SYSTEMTIME * pst, BOOL fChange)
{
     static POINT pt[3][5] = { {{0, -30}, {20, 0}, {0, 100}, {-20, 0}, {0, -30}},
                               {{0, -40}, {10, 0}, {0, 160}, {-10, 0}, {0, -40}},
                               {{0,   0}, { 0, 0}, {0,   0}, {  0, 0}, {0, 160}} };
     INT i, iAngle[3];
     POINT ptTemp[3][5];

     /* Black pen for outline, white brush for fill */
     SelectObject(hdc, GetStockObject(BLACK_PEN));
     SelectObject(hdc, GetStockObject(WHITE_BRUSH));

     iAngle[0] = (pst->wHour * 30) % 360 + pst->wMinute / 2;
     iAngle[1] = pst->wMinute * 6;
     iAngle[2] = pst->wSecond * 6;

     CopyMemory(ptTemp, pt, sizeof(pt));

     for (i = fChange ? 0 : 2; i < 3; i++)
     {
          RotatePoint(ptTemp[i], 5, iAngle[i]);

          Polygon(hdc, ptTemp[i], 5);
     }
}


static LRESULT CALLBACK
ClockWndProc(HWND hwnd,
             UINT uMsg,
             WPARAM wParam,
             LPARAM lParam)
{
    PCLOCKDATA pClockData;
    HDC hdc;
    PAINTSTRUCT ps;

    pClockData = (PCLOCKDATA)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_CREATE:
            pClockData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CLOCKDATA));
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pClockData);

            pClockData->hGreyPen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
            pClockData->hGreyBrush = CreateSolidBrush(RGB(128, 128, 128));

            SetTimer(hwnd, ID_TIMER, 1000, NULL);
            pClockData->bTimer = TRUE;
            GetLocalTime(&pClockData->stCurrent);
            pClockData->stPrevious = pClockData->stCurrent;
            break;

        case WM_SIZE:
            pClockData->cxClient = LOWORD(lParam);
            pClockData->cyClient = HIWORD(lParam);
            break;

        case WM_TIMER:
            GetLocalTime(&pClockData->stCurrent);
            InvalidateRect(hwnd, NULL, TRUE);
            pClockData->stPrevious = pClockData->stCurrent;
            break;

        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);
            SetIsotropic(hdc, pClockData);
            DrawClock(hdc, pClockData);
            DrawHands(hdc, &pClockData->stPrevious, TRUE);
            EndPaint(hwnd, &ps);
            break;

        case WM_DESTROY:
            DeleteObject(pClockData->hGreyPen);
            DeleteObject(pClockData->hGreyBrush);

            if (pClockData->bTimer)
                KillTimer(hwnd, ID_TIMER);

            HeapFree(GetProcessHeap(), 0, pClockData);
            break;

        case CLM_SETTIME:
            /* Stop the timer if it is still running */
            if (pClockData->bTimer)
            {
                KillTimer(hwnd, ID_TIMER);
                pClockData->bTimer = FALSE;
            }

            /* Set the current time */
            CopyMemory(&pClockData->stPrevious, (LPSYSTEMTIME)lParam, sizeof(SYSTEMTIME));

            /* Redraw the clock */
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        default:
            DefWindowProcW(hwnd,
                           uMsg,
                           wParam,
                           lParam);
    }

    return TRUE;
}


BOOL
RegisterClockControl(VOID)
{
    WNDCLASSEXW wc = {0};

    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = ClockWndProc;
    wc.hInstance = hApplet;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = szClockWndClass;

    return RegisterClassExW(&wc) != (ATOM)0;
}


VOID
UnregisterClockControl(VOID)
{
    UnregisterClassW(szClockWndClass,
                     hApplet);
}
